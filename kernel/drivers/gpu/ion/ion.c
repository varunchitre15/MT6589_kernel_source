/*
 * drivers/gpu/ion/ion.c
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/device.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/anon_inodes.h>
#include <linux/ion.h>
#include <linux/list.h>
#include <linux/memblock.h>
#include <linux/miscdevice.h>
#include <linux/export.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/dma-buf.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include "ion_priv.h"
#define ION_DEBUG 0
#if ION_DEBUG
#include "ion_debug.h"
#include "ion_debug_db.h"
#include <linux/ion_drv.h>
#endif
/**
 * struct ion_device - the metadata of the ion device node
 * @dev:		the actual misc device
 * @buffers:	an rb tree of all the existing buffers
 * @lock:		lock protecting the buffers & heaps trees
 * @heaps:		list of all the heaps in the system
 * @user_clients:	list of all the clients created from userspace
 */
struct ion_device {
	struct miscdevice dev;
	struct rb_root buffers;
        struct mutex buffer_lock;
        struct rw_semaphore lock;
	struct rb_root heaps;
	long (*custom_ioctl) (struct ion_client *client, unsigned int cmd,
			      unsigned long arg);
	struct rb_root clients;
	struct dentry *debug_root;
};

/**
 * struct ion_client - a process/hw block local address space
 * @node:		node in the tree of all clients
 * @dev:		backpointer to ion device
 * @handles:		an rb tree of all the handles in this client
 * @lock:		lock protecting the tree of handles
 * @heap_mask:		mask of all supported heaps
 * @name:		used for debugging
 * @task:		used for debugging
 *
 * A client represents a list of buffers this client may access.
 * The mutex stored here is used to protect both handles tree
 * as well as the handles themselves, and should be held while modifying either.
 */
struct ion_client {
	struct rb_node node;
	struct ion_device *dev;
	struct rb_root handles;
	struct mutex lock;
	unsigned int heap_mask;
	const char *name;
	struct task_struct *task;
	pid_t pid;
	struct dentry *debug_root;
};

/**
 * ion_handle - a client local reference to a buffer
 * @ref:		reference count
 * @client:		back pointer to the client the buffer resides in
 * @buffer:		pointer to the buffer
 * @node:		node in the client's handle rbtree
 * @kmap_cnt:		count of times this client has mapped to kernel
 * @dmap_cnt:		count of times this client has mapped for dma
 *
 * Modifications to node, map_cnt or mapping should be protected by the
 * lock in the client.  Other fields are never changed after initialization.
 */
struct ion_handle {
	struct kref ref;
	struct ion_client *client;
	struct ion_buffer *buffer;
	struct rb_node node;
	unsigned int kmap_cnt;
};

#if ION_DEBUG
struct ion_client *ion_client_create_user(struct ion_device *dev,unsigned int heap_mask, const char *name);
void ion_client_destroy_user(struct ion_client *client);
struct ion_handle *ion_alloc_user(struct ion_client *client, size_t len,
                             size_t align, unsigned int heap_mask,
                             unsigned int flags);
void ion_free_user(struct ion_client *client, struct ion_handle *handle);
int ion_share_dma_buf_user(struct ion_client *client, struct ion_handle *handle);
struct ion_handle *ion_import_dma_buf_user(struct ion_client *client, int fd);
static void ion_dma_buf_release_user(struct dma_buf *dmabuf);
static void ion_debug_db_create_clentry(pid_t);
static void ion_debug_db_destroy_clentry(pid_t pid);
static void ion_debug_create_db(struct dentry *root);
#endif

bool ion_buffer_fault_user_mappings(struct ion_buffer *buffer)
{
        return ((buffer->flags & ION_FLAG_CACHED) &&
                !(buffer->flags & ION_FLAG_CACHED_NEEDS_SYNC));
}

bool ion_buffer_cached(struct ion_buffer *buffer)
{
        return !!(buffer->flags & ION_FLAG_CACHED);
}

/* this function should only be called while dev->lock is held */
static void ion_buffer_add(struct ion_device *dev,
			   struct ion_buffer *buffer)
{
	struct rb_node **p = &dev->buffers.rb_node;
	struct rb_node *parent = NULL;
	struct ion_buffer *entry;

	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct ion_buffer, node);

		if (buffer < entry) {
			p = &(*p)->rb_left;
		} else if (buffer > entry) {
			p = &(*p)->rb_right;
		} else {
			pr_err("%s: buffer already found.", __func__);
			BUG();
		}
	}

	rb_link_node(&buffer->node, parent, p);
	rb_insert_color(&buffer->node, &dev->buffers);
}

static int ion_buffer_alloc_dirty(struct ion_buffer *buffer);

/* this function should only be called while dev->lock is held */
static struct ion_buffer *ion_buffer_create(struct ion_heap *heap,
				     struct ion_device *dev,
				     unsigned long len,
				     unsigned long align,
				     unsigned long flags)
{
	struct ion_buffer *buffer;
	struct sg_table *table;
	struct scatterlist *sg;
	int i, ret;

	buffer = kzalloc(sizeof(struct ion_buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	buffer->heap = heap;
        buffer->flags = flags;
	kref_init(&buffer->ref);

	ret = heap->ops->allocate(heap, buffer, len, align, flags);
	if (ret) {
		kfree(buffer);
		return ERR_PTR(ret);
	}

	buffer->dev = dev;
	buffer->size = len;

	table = heap->ops->map_dma(heap, buffer);
	if (IS_ERR_OR_NULL(table)) {
		heap->ops->free(buffer);
		kfree(buffer);
		return ERR_PTR(PTR_ERR(table));
	}
	buffer->sg_table = table;
        if (ion_buffer_fault_user_mappings(buffer)) {
                for_each_sg(buffer->sg_table->sgl, sg, buffer->sg_table->nents,
                            i) {
                        if (sg_dma_len(sg) == PAGE_SIZE)
                                continue;
                        pr_err("%s: cached mappings that will be faulted in "
                               "must have pagewise sg_lists\n", __func__);
                        ret = -EINVAL;
                        goto err;
                }

                ret = ion_buffer_alloc_dirty(buffer);
                if (ret)
                        goto err;
	}

	buffer->dev = dev;
	buffer->size = len;
	INIT_LIST_HEAD(&buffer->vmas);
	mutex_init(&buffer->lock);
	/* this will set up dma addresses for the sglist -- it is not
	   technically correct as per the dma api -- a specific
	   device isn't really taking ownership here.  However, in practice on
	   our systems the only dma_address space is physical addresses.
	   Additionally, we can't afford the overhead of invalidating every
	   allocation via dma_map_sg. The implicit contract here is that
	   memory comming from the heaps is ready for dma, ie if it has a
	   cached mapping that mapping has been invalidated */
	for_each_sg(buffer->sg_table->sgl, sg, buffer->sg_table->nents, i)
		sg_dma_address(sg) = sg_phys(sg);
        mutex_lock(&dev->buffer_lock);
	ion_buffer_add(dev, buffer);
        mutex_unlock(&dev->buffer_lock);
	return buffer;
err:
        heap->ops->unmap_dma(heap, buffer);
        heap->ops->free(buffer);
        kfree(buffer);
        return ERR_PTR(ret);
}

static void ion_buffer_destroy(struct kref *kref)
{
	struct ion_buffer *buffer = container_of(kref, struct ion_buffer, ref);
	struct ion_device *dev = buffer->dev;

	if (WARN_ON(buffer->kmap_cnt > 0))
		buffer->heap->ops->unmap_kernel(buffer->heap, buffer);

	buffer->heap->ops->unmap_dma(buffer->heap, buffer);
	buffer->heap->ops->free(buffer);
        mutex_lock(&dev->buffer_lock);
	rb_erase(&buffer->node, &dev->buffers);
        mutex_unlock(&dev->buffer_lock);
  if (ion_buffer_fault_user_mappings(buffer))
	    kfree(buffer->dirty);
	kfree(buffer);
}

static void ion_buffer_get(struct ion_buffer *buffer)
{
	kref_get(&buffer->ref);
}

static int ion_buffer_put(struct ion_buffer *buffer)
{
	return kref_put(&buffer->ref, ion_buffer_destroy);
}

static void ion_buffer_add_to_handle(struct ion_buffer *buffer)
{
        mutex_lock(&buffer->lock);
        buffer->handle_count++;
        mutex_unlock(&buffer->lock);
}

static void ion_buffer_remove_from_handle(struct ion_buffer *buffer)
{
        /*
         * when a buffer is removed from a handle, if it is not in
         * any other handles, copy the taskcomm and the pid of the
         * process it's being removed from into the buffer.  At this
         * point there will be no way to track what processes this buffer is
         * being used by, it only exists as a dma_buf file descriptor.
         * The taskcomm and pid can provide a debug hint as to where this fd
         * is in the system
         */
        mutex_lock(&buffer->lock);
        buffer->handle_count--;
        BUG_ON(buffer->handle_count < 0);
        if (!buffer->handle_count) {
                struct task_struct *task;

                task = current->group_leader;
                get_task_comm(buffer->task_comm, task);
                buffer->pid = task_pid_nr(task);
        }
        mutex_unlock(&buffer->lock);
}

static struct ion_handle *ion_handle_create(struct ion_client *client,
				     struct ion_buffer *buffer)
{
	struct ion_handle *handle;

	handle = kzalloc(sizeof(struct ion_handle), GFP_KERNEL);
	if (!handle)
		return ERR_PTR(-ENOMEM);
	kref_init(&handle->ref);
	rb_init_node(&handle->node);
	handle->client = client;
	ion_buffer_get(buffer);
        ion_buffer_add_to_handle(buffer);
	handle->buffer = buffer;

	return handle;
}

static void ion_handle_kmap_put(struct ion_handle *);

static void ion_handle_destroy(struct kref *kref)
{
	struct ion_handle *handle = container_of(kref, struct ion_handle, ref);
	struct ion_client *client = handle->client;
	struct ion_buffer *buffer = handle->buffer;

	mutex_lock(&buffer->lock);
	while (handle->kmap_cnt)
		ion_handle_kmap_put(handle);
	mutex_unlock(&buffer->lock);

	if (!RB_EMPTY_NODE(&handle->node))
		rb_erase(&handle->node, &client->handles);

        ion_buffer_remove_from_handle(buffer);
	ion_buffer_put(buffer);

	kfree(handle);
}

struct ion_buffer *ion_handle_buffer(struct ion_handle *handle)
{
	return handle->buffer;
}

static void ion_handle_get(struct ion_handle *handle)
{
	kref_get(&handle->ref);
}

static int ion_handle_put(struct ion_handle *handle)
{
	return kref_put(&handle->ref, ion_handle_destroy);
}

static struct ion_handle *ion_handle_lookup(struct ion_client *client,
					    struct ion_buffer *buffer)
{
	struct rb_node *n;

	for (n = rb_first(&client->handles); n; n = rb_next(n)) {
		struct ion_handle *handle = rb_entry(n, struct ion_handle,
						     node);
		if (handle->buffer == buffer)
			return handle;
	}
	return NULL;
}

static bool ion_handle_validate(struct ion_client *client, struct ion_handle *handle)
{
	struct rb_node *n = client->handles.rb_node;

	while (n) {
		struct ion_handle *handle_node = rb_entry(n, struct ion_handle,
							  node);
		if (handle < handle_node)
			n = n->rb_left;
		else if (handle > handle_node)
			n = n->rb_right;
		else
			return true;
	}
	return false;
}

static void ion_handle_add(struct ion_client *client, struct ion_handle *handle)
{
	struct rb_node **p = &client->handles.rb_node;
	struct rb_node *parent = NULL;
	struct ion_handle *entry;

	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct ion_handle, node);

		if (handle < entry)
			p = &(*p)->rb_left;
		else if (handle > entry)
			p = &(*p)->rb_right;
		else
			WARN(1, "%s: buffer already found.", __func__);
	}

	rb_link_node(&handle->node, parent, p);
	rb_insert_color(&handle->node, &client->handles);
}

struct ion_handle *ion_alloc(struct ion_client *client, size_t len,
			     size_t align, unsigned int heap_mask,
			     unsigned int flags)
{
	struct rb_node *n;
	struct ion_handle *handle;
	struct ion_device *dev = client->dev;
	struct ion_buffer *buffer = NULL;

	pr_debug("%s: len %d align %d heap_mask %u flags %x\n", __func__, len,
		 align, heap_mask, flags);
	/*
	 * traverse the list of heaps available in this system in priority
	 * order.  If the heap type is supported by the client, and matches the
	 * request of the caller allocate from it.  Repeat until allocate has
	 * succeeded or all heaps have been tried
	 */
	if (WARN_ON(!len))
		return ERR_PTR(-EINVAL);

	len = PAGE_ALIGN(len);

        down_read(&dev->lock);
	for (n = rb_first(&dev->heaps); n != NULL; n = rb_next(n)) {
		struct ion_heap *heap = rb_entry(n, struct ion_heap, node);
		/* if the client doesn't support this heap type */
		if (!((1 << heap->type) & client->heap_mask))
			continue;
		/* if the caller didn't specify this heap type */
		if (!((1 << heap->id) & heap_mask))
			continue;
		buffer = ion_buffer_create(heap, dev, len, align, flags);
		if (!IS_ERR_OR_NULL(buffer))
			break;
	}
        up_read(&dev->lock);

	if (buffer == NULL)
		return ERR_PTR(-ENODEV);

	if (IS_ERR(buffer))
		return ERR_PTR(PTR_ERR(buffer));

	handle = ion_handle_create(client, buffer);

	/*
	 * ion_buffer_create will create a buffer with a ref_cnt of 1,
	 * and ion_handle_create will take a second reference, drop one here
	 */
	ion_buffer_put(buffer);

	if (!IS_ERR(handle)) {
		mutex_lock(&client->lock);
		ion_handle_add(client, handle);
		mutex_unlock(&client->lock);
	}
#if ION_DEBUG
        {
                ion_sys_record_t record_param;
                record_param.client = client;
                record_param.pid = client->pid;
                record_param.group_id = client->pid;
		record_param.buffer = buffer;
		record_param.handle = handle;
                record_param.action = ION_FUNCTION_ALLOC;
                printk("[KERNEL ION_ALLOC]\n");
                record_param.backtrace_num = get_kernel_backtrace(record_param.backtrace);
		get_kernel_symbol(record_param.backtrace,record_param.backtrace_num,&(record_param.kernel_symbol[0]));
		
                record_ion_info((int)1,&record_param);
		printk("[KERNEL ION_ALLOC] DONE\n");
        }
#endif


	return handle;
}
EXPORT_SYMBOL(ion_alloc);

void ion_free(struct ion_client *client, struct ion_handle *handle)
{
	bool valid_handle;

	BUG_ON(client != handle->client);

	mutex_lock(&client->lock);
	valid_handle = ion_handle_validate(client, handle);

	if (!valid_handle) {
		WARN(1, "%s: invalid handle passed to free.\n", __func__);
                mutex_unlock(&client->lock);
		return;
	}
	ion_handle_put(handle);
        mutex_unlock(&client->lock);
#if ION_DEBUG
        {
                ion_sys_record_t record_param;
                record_param.client = client;
                record_param.pid = client->pid;
                record_param.group_id = client->pid;
		record_param.buffer = handle->buffer;
                record_param.action = ION_FUNCTION_FREE;
                printk("[KERNEL ION_FREE]\n");
                record_param.backtrace_num = get_kernel_backtrace(record_param.backtrace);
 		get_kernel_symbol(record_param.backtrace,record_param.backtrace_num,&(record_param.kernel_symbol[0]));
                record_ion_info((int)1,&record_param);
		printk("[KERNEL ION_FREE] DONE\n");

        }
#endif
}
EXPORT_SYMBOL(ion_free);

int ion_phys(struct ion_client *client, struct ion_handle *handle,
	     ion_phys_addr_t *addr, size_t *len)
{
	struct ion_buffer *buffer;
	int ret;

	mutex_lock(&client->lock);
	if (!ion_handle_validate(client, handle)) {
		mutex_unlock(&client->lock);
		return -EINVAL;
	}

	buffer = handle->buffer;

	if (!buffer->heap->ops->phys) {
		pr_err("%s: ion_phys is not implemented by this heap.\n",
		       __func__);
		mutex_unlock(&client->lock);
		return -ENODEV;
	}
	mutex_unlock(&client->lock);
	ret = buffer->heap->ops->phys(buffer->heap, buffer, addr, len);
	return ret;
}
EXPORT_SYMBOL(ion_phys);

static void *ion_buffer_kmap_get(struct ion_buffer *buffer)
{
	void *vaddr;

	if (buffer->kmap_cnt) {
		buffer->kmap_cnt++;
		return buffer->vaddr;
	}
	vaddr = buffer->heap->ops->map_kernel(buffer->heap, buffer);
	if (IS_ERR_OR_NULL(vaddr))
		return vaddr;
	buffer->vaddr = vaddr;
	buffer->kmap_cnt++;
	return vaddr;
}

static void *ion_handle_kmap_get(struct ion_handle *handle)
{
	struct ion_buffer *buffer = handle->buffer;
	void *vaddr;

	if (handle->kmap_cnt) {
		handle->kmap_cnt++;
		return buffer->vaddr;
	}
	vaddr = ion_buffer_kmap_get(buffer);
	if (IS_ERR_OR_NULL(vaddr))
		return vaddr;
	handle->kmap_cnt++;
	return vaddr;
}

static void ion_buffer_kmap_put(struct ion_buffer *buffer)
{
	buffer->kmap_cnt--;
	if (!buffer->kmap_cnt) {
		buffer->heap->ops->unmap_kernel(buffer->heap, buffer);
		buffer->vaddr = NULL;
	}
}

static void ion_handle_kmap_put(struct ion_handle *handle)
{
	struct ion_buffer *buffer = handle->buffer;

	handle->kmap_cnt--;
	if (!handle->kmap_cnt)
		ion_buffer_kmap_put(buffer);
}

void *ion_map_kernel(struct ion_client *client, struct ion_handle *handle)
{
	struct ion_buffer *buffer;
	void *vaddr;

	mutex_lock(&client->lock);
	if (!ion_handle_validate(client, handle)) {
		pr_err("%s: invalid handle passed to map_kernel.\n",
		       __func__);
		mutex_unlock(&client->lock);
		return ERR_PTR(-EINVAL);
	}

	buffer = handle->buffer;

	if (!handle->buffer->heap->ops->map_kernel) {
		pr_err("%s: map_kernel is not implemented by this heap.\n",
		       __func__);
		mutex_unlock(&client->lock);
		return ERR_PTR(-ENODEV);
	}

	mutex_lock(&buffer->lock);
	vaddr = ion_handle_kmap_get(handle);
	mutex_unlock(&buffer->lock);
	mutex_unlock(&client->lock);
#if ION_DEBUG
        {
                ion_sys_record_t record_param;
                record_param.client = client;
                record_param.pid = client->pid;
                record_param.group_id = client->pid;
                record_param.buffer = handle->buffer;
                record_param.handle = handle;
		record_param.address_type = ADDRESS_KERNEL_VIRTUAL;
		record_param.address = vaddr;
		record_param.length = handle->buffer->size;
                record_param.action = ION_FUNCTION_MMAP;
                printk("[KERNEL ION_MAP_KERNEL]\n");
                record_param.backtrace_num = get_kernel_backtrace(record_param.backtrace);
		get_kernel_symbol(record_param.backtrace,record_param.backtrace_num,&(record_param.kernel_symbol[0]));
                record_ion_info((int)1,&record_param);
		printk("[KERNEL ION_MAP_KERNEL] DONE\n");
        }
#endif

	return vaddr;
}
EXPORT_SYMBOL(ion_map_kernel);

void ion_unmap_kernel(struct ion_client *client, struct ion_handle *handle)
{
	struct ion_buffer *buffer;
#if ION_DEBUG
        {
                ion_sys_record_t record_param;
                record_param.client = client;
                record_param.pid = client->pid;
                record_param.group_id = client->pid;
                record_param.buffer = handle->buffer;
                record_param.handle = handle;
                record_param.address = handle->buffer->vaddr;
                record_param.length = handle->buffer->size;
                record_param.action = ION_FUNCTION_MUNMAP;
                record_param.address_type = ADDRESS_KERNEL_VIRTUAL;
                printk("[KERNEL ION_UMAP_KERNEL]\n");
                record_param.backtrace_num = get_kernel_backtrace(record_param.backtrace);
                get_kernel_symbol(record_param.backtrace,record_param.backtrace_num,&(record_param.kernel_symbol[0]));
                record_ion_info((int)1,&record_param);
                printk("[KERNEL ION_UMAP_KERNEL] DONE\n");

        }
#endif

	mutex_lock(&client->lock);
	buffer = handle->buffer;
	mutex_lock(&buffer->lock);
	ion_handle_kmap_put(handle);
	mutex_unlock(&buffer->lock);
	mutex_unlock(&client->lock);
}
EXPORT_SYMBOL(ion_unmap_kernel);

static int ion_debug_client_show(struct seq_file *s, void *unused)
{
	struct ion_client *client = s->private;
	struct rb_node *n;
	size_t sizes[ION_NUM_HEAPS] = {0};
	const char *names[ION_NUM_HEAPS] = {0};
	int i;

	mutex_lock(&client->lock);
	for (n = rb_first(&client->handles); n; n = rb_next(n)) {
		struct ion_handle *handle = rb_entry(n, struct ion_handle,
						     node);
		enum ion_heap_type type = handle->buffer->heap->type;

		if (!names[type])
			names[type] = handle->buffer->heap->name;
		sizes[type] += handle->buffer->size;
	}
	mutex_unlock(&client->lock);

	seq_printf(s, "%16.16s: %16.16s\n", "heap_name", "size_in_bytes");
	for (i = 0; i < ION_NUM_HEAPS; i++) {
		if (!names[i])
			continue;
		seq_printf(s, "%16.16s: %16u\n", names[i], sizes[i]);
	}
	return 0;
}

static int ion_debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, ion_debug_client_show, inode->i_private);
}

static const struct file_operations debug_client_fops = {
	.open = ion_debug_client_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

struct ion_client *ion_client_create(struct ion_device *dev,
				     unsigned int heap_mask,
				     const char *name)
{
	struct ion_client *client;
	struct task_struct *task;
	struct rb_node **p;
	struct rb_node *parent = NULL;
	struct ion_client *entry;
	char debug_name[64];
	pid_t pid;

	get_task_struct(current->group_leader);
	task_lock(current->group_leader);
	pid = task_pid_nr(current->group_leader);
	/* don't bother to store task struct for kernel threads,
	   they can't be killed anyway */
	if (current->group_leader->flags & PF_KTHREAD) {
		put_task_struct(current->group_leader);
		task = NULL;
	} else {
		task = current->group_leader;
	}
	task_unlock(current->group_leader);

	client = kzalloc(sizeof(struct ion_client), GFP_KERNEL);
	if (!client) {
		if (task)
			put_task_struct(current->group_leader);
		return ERR_PTR(-ENOMEM);
	}

	client->dev = dev;
	client->handles = RB_ROOT;
	mutex_init(&client->lock);
	client->name = name;
	client->heap_mask = heap_mask;
	client->task = task;
	client->pid = pid;

        down_write(&dev->lock);
	p = &dev->clients.rb_node;
	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct ion_client, node);

		if (client < entry)
			p = &(*p)->rb_left;
		else if (client > entry)
			p = &(*p)->rb_right;
	}
	rb_link_node(&client->node, parent, p);
	rb_insert_color(&client->node, &dev->clients);

	snprintf(debug_name, 64, "%u", client->pid);
	client->debug_root = debugfs_create_file(debug_name, 0664,
						 dev->debug_root, client,
						 &debug_client_fops);
        up_write(&dev->lock);
	#if ION_DEBUG
	{
		ion_sys_record_t record_param;
		record_param.client = client;
		record_param.pid = client->pid;
		record_param.group_id = client->pid;
		record_param.buffer = NULL;
		record_param.handle = NULL;
		record_param.action = ION_FUNCTION_CREATE_CLIENT;
		printk("[KERNEL ION_CLIENT_CREATE]\n");
		record_param.backtrace_num = get_kernel_backtrace(record_param.backtrace);
		get_kernel_symbol(record_param.backtrace,record_param.backtrace_num,&(record_param.kernel_symbol[0]));
		record_ion_info((int)1,&record_param);
		printk("[KERNEL ION_CLIENT_CREATE] DONE\n");
/* Create checking_leakage entry */
		ion_debug_db_create_clentry(pid);
	}
	#endif

	return client;
}
EXPORT_SYMBOL(ion_client_create);

void ion_client_destroy(struct ion_client *client)
{
	struct ion_device *dev = client->dev;
	struct rb_node *n;

	pr_debug("%s: %d\n", __func__, __LINE__);
	while ((n = rb_first(&client->handles))) {
		struct ion_handle *handle = rb_entry(n, struct ion_handle,
						     node);
		ion_handle_destroy(&handle->ref);
	}
        down_write(&dev->lock);
	if (client->task)
		put_task_struct(client->task);
	rb_erase(&client->node, &dev->clients);
	debugfs_remove_recursive(client->debug_root);
        up_write(&dev->lock);
#if ION_DEBUG
        {
		printk("[KERNEL ION_CLIENT_DESTROY]\n");
                ion_sys_record_t record_param;
                record_param.client = client;
		record_param.buffer = NULL;
		record_param.handle = NULL;
		record_param.pid = client->pid;
		record_param.group_id = client->pid;
                record_param.action = ION_FUNCTION_DESTROY_CLIENT;
                record_param.backtrace_num = get_kernel_backtrace(record_param.backtrace);
		get_kernel_symbol(record_param.backtrace,record_param.backtrace_num,&(record_param.kernel_symbol[0]));
                record_ion_info((int)1,&record_param);
		printk("[KERNEL ION_CLIENT_DESTROY] DONE\n");

        }

	/* Destroy checking_leakage entry */
	ion_debug_db_destroy_clentry(client->pid);
#endif

	kfree(client);
}
EXPORT_SYMBOL(ion_client_destroy);

struct sg_table *ion_sg_table(struct ion_client *client,
			      struct ion_handle *handle)
{
	struct ion_buffer *buffer;
	struct sg_table *table;

	mutex_lock(&client->lock);
	if (!ion_handle_validate(client, handle)) {
		pr_err("%s: invalid handle passed to map_dma.\n",
		       __func__);
		mutex_unlock(&client->lock);
		return ERR_PTR(-EINVAL);
	}
	buffer = handle->buffer;
	table = buffer->sg_table;
	mutex_unlock(&client->lock);
	return table;
}
EXPORT_SYMBOL(ion_sg_table);

static void ion_buffer_sync_for_device(struct ion_buffer *buffer,
				       struct device *dev,
				       enum dma_data_direction direction);

static struct sg_table *ion_map_dma_buf(struct dma_buf_attachment *attachment,
					enum dma_data_direction direction)
{
	struct dma_buf *dmabuf = attachment->dmabuf;
	struct ion_buffer *buffer = dmabuf->priv;

	ion_buffer_sync_for_device(buffer, attachment->dev, direction);
	return buffer->sg_table;
}

static void ion_unmap_dma_buf(struct dma_buf_attachment *attachment,
			      struct sg_table *table,
			      enum dma_data_direction direction)
{
}

static int ion_buffer_alloc_dirty(struct ion_buffer *buffer)
{
	unsigned long pages = buffer->sg_table->nents;
	unsigned long length = (pages + BITS_PER_LONG - 1)/BITS_PER_LONG;

	buffer->dirty = kzalloc(length * sizeof(unsigned long), GFP_KERNEL);
	if (!buffer->dirty)
		return -ENOMEM;
	return 0;
}

struct ion_vma_list {
	struct list_head list;
	struct vm_area_struct *vma;
};

static void ion_buffer_sync_for_device(struct ion_buffer *buffer,
				       struct device *dev,
				       enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;
	struct ion_vma_list *vma_list;

	pr_debug("%s: syncing for device %s\n", __func__,
		 dev ? dev_name(dev) : "null");

        if (!ion_buffer_fault_user_mappings(buffer))
		return;

	mutex_lock(&buffer->lock);
	for_each_sg(buffer->sg_table->sgl, sg, buffer->sg_table->nents, i) {
		if (!test_bit(i, buffer->dirty))
			continue;
		dma_sync_sg_for_device(dev, sg, 1, dir);
		clear_bit(i, buffer->dirty);
	}
	list_for_each_entry(vma_list, &buffer->vmas, list) {
		struct vm_area_struct *vma = vma_list->vma;

		zap_page_range(vma, vma->vm_start, vma->vm_end - vma->vm_start,
			       NULL);
	}
	mutex_unlock(&buffer->lock);
}

int ion_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct ion_buffer *buffer = vma->vm_private_data;
	struct scatterlist *sg;
	int i;

	mutex_lock(&buffer->lock);
	set_bit(vmf->pgoff, buffer->dirty);

	for_each_sg(buffer->sg_table->sgl, sg, buffer->sg_table->nents, i) {
		if (i != vmf->pgoff)
			continue;
		dma_sync_sg_for_cpu(NULL, sg, 1, DMA_BIDIRECTIONAL);
		vm_insert_page(vma, (unsigned long)vmf->virtual_address,
			       sg_page(sg));
		break;
	}
	mutex_unlock(&buffer->lock);
	return VM_FAULT_NOPAGE;
}

static void ion_vm_open(struct vm_area_struct *vma)
{
	struct ion_buffer *buffer = vma->vm_private_data;
	struct ion_vma_list *vma_list;

	vma_list = kmalloc(sizeof(struct ion_vma_list), GFP_KERNEL);
	if (!vma_list)
		return;
	vma_list->vma = vma;
	mutex_lock(&buffer->lock);
	list_add(&vma_list->list, &buffer->vmas);
	mutex_unlock(&buffer->lock);
	pr_debug("%s: adding %p\n", __func__, vma);
}

static void ion_vm_close(struct vm_area_struct *vma)
{
	struct ion_buffer *buffer = vma->vm_private_data;
	struct ion_vma_list *vma_list, *tmp;

	pr_debug("%s\n", __func__);
	mutex_lock(&buffer->lock);
	list_for_each_entry_safe(vma_list, tmp, &buffer->vmas, list) {
		if (vma_list->vma != vma)
			continue;
		list_del(&vma_list->list);
		kfree(vma_list);
		pr_debug("%s: deleting %p\n", __func__, vma);
		break;
	}
	mutex_unlock(&buffer->lock);
}

struct vm_operations_struct ion_vma_ops = {
	.open = ion_vm_open,
	.close = ion_vm_close,
	.fault = ion_vm_fault,
};

static int ion_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct ion_buffer *buffer = dmabuf->priv;
	int ret = 0;

	if (!buffer->heap->ops->map_user) {
		pr_err("%s: this heap does not define a method for mapping "
		       "to userspace\n", __func__);
		return -EINVAL;
	}

        if (ion_buffer_fault_user_mappings(buffer)) {
                vma->vm_private_data = buffer;
                vma->vm_ops = &ion_vma_ops;
                ion_vm_open(vma);
                return 0;
        }

		//vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

        mutex_lock(&buffer->lock);
        /* now map it to userspace */
        ret = buffer->heap->ops->map_user(buffer->heap, buffer, vma);
        mutex_unlock(&buffer->lock);

	if (ret)
		pr_err("%s: failure mapping buffer to userspace\n",
		       __func__);

	return ret;
}

static void ion_dma_buf_release(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	ion_buffer_put(buffer);
}

static void *ion_dma_buf_kmap(struct dma_buf *dmabuf, unsigned long offset)
{
	struct ion_buffer *buffer = dmabuf->priv;
        return buffer->vaddr + offset * PAGE_SIZE;
}

static void ion_dma_buf_kunmap(struct dma_buf *dmabuf, unsigned long offset,
			       void *ptr)
{
	return;
}

static int ion_dma_buf_begin_cpu_access(struct dma_buf *dmabuf, size_t start,
					size_t len,
					enum dma_data_direction direction)
{
	struct ion_buffer *buffer = dmabuf->priv;
	void *vaddr;

	if (!buffer->heap->ops->map_kernel) {
		pr_err("%s: map kernel is not implemented by this heap.\n",
		       __func__);
		return -ENODEV;
	}

	mutex_lock(&buffer->lock);
	vaddr = ion_buffer_kmap_get(buffer);
	mutex_unlock(&buffer->lock);
	if (IS_ERR(vaddr))
		return PTR_ERR(vaddr);
	if (!vaddr)
		return -ENOMEM;
	return 0;
}

static void ion_dma_buf_end_cpu_access(struct dma_buf *dmabuf, size_t start,
				       size_t len,
				       enum dma_data_direction direction)
{
	struct ion_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	ion_buffer_kmap_put(buffer);
	mutex_unlock(&buffer->lock);
}
#if ION_DEBUG
#define ion_dma_buf_release ion_dma_buf_release_user
#endif
struct dma_buf_ops dma_buf_ops = {
	.map_dma_buf = ion_map_dma_buf,
	.unmap_dma_buf = ion_unmap_dma_buf,
	.mmap = ion_mmap,
	.release = ion_dma_buf_release,
	.begin_cpu_access = ion_dma_buf_begin_cpu_access,
	.end_cpu_access = ion_dma_buf_end_cpu_access,
	.kmap_atomic = ion_dma_buf_kmap,
	.kunmap_atomic = ion_dma_buf_kunmap,
	.kmap = ion_dma_buf_kmap,
	.kunmap = ion_dma_buf_kunmap,
};
#if ION_DEBUG
#undef ion_dma_buf_release
#endif

int ion_share_dma_buf(struct ion_client *client, struct ion_handle *handle)
{
	struct ion_buffer *buffer;
	struct dma_buf *dmabuf;
	bool valid_handle;
	int fd;

	mutex_lock(&client->lock);
	valid_handle = ion_handle_validate(client, handle);
	mutex_unlock(&client->lock);
	if (!valid_handle) {
		WARN(1, "%s: invalid handle passed to share.\n", __func__);
		return -EINVAL;
	}

	buffer = handle->buffer;
	ion_buffer_get(buffer);
	dmabuf = dma_buf_export(buffer, &dma_buf_ops, buffer->size, O_RDWR);
	if (IS_ERR(dmabuf)) {
		ion_buffer_put(buffer);
		return PTR_ERR(dmabuf);
	}
	fd = dma_buf_fd(dmabuf, O_CLOEXEC);
	if (fd < 0)
		dma_buf_put(dmabuf);
#if ION_DEBUG
        {
                ion_sys_record_t record_param;
                record_param.client = client;
                record_param.pid = client->pid;
                record_param.group_id = client->pid;
                record_param.buffer = handle->buffer;
                record_param.handle = handle;
		record_param.fd     = fd;
                record_param.action = ION_FUNCTION_SHARE;
                printk("[KERNEL ION_SHARE]\n");
                record_param.backtrace_num = get_kernel_backtrace(record_param.backtrace);
		get_kernel_symbol(record_param.backtrace,record_param.backtrace_num,&(record_param.kernel_symbol[0]));
                record_ion_info((int)1,&record_param);
		printk("[KERNEL ION_SHARE] DONE\n");

        }
#endif

	return fd;
}
EXPORT_SYMBOL(ion_share_dma_buf);

struct ion_handle *ion_import_dma_buf(struct ion_client *client, int fd)
{
	struct dma_buf *dmabuf;
	struct ion_buffer *buffer;
	struct ion_handle *handle;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf))
		return ERR_PTR(PTR_ERR(dmabuf));
	/* if this memory came from ion */

	if (dmabuf->ops != &dma_buf_ops) {
		pr_err("%s: can not import dmabuf from another exporter\n",
		       __func__);
		dma_buf_put(dmabuf);
		return ERR_PTR(-EINVAL);
	}
	buffer = dmabuf->priv;

	mutex_lock(&client->lock);
	/* if a handle exists for this buffer just take a reference to it */
	handle = ion_handle_lookup(client, buffer);
	if (!IS_ERR_OR_NULL(handle)) {
		ion_handle_get(handle);
		goto end;
	}
	handle = ion_handle_create(client, buffer);
	if (IS_ERR_OR_NULL(handle))
		goto end;
	ion_handle_add(client, handle);
end:
	mutex_unlock(&client->lock);
	dma_buf_put(dmabuf);
#if ION_DEBUG
        if (!IS_ERR_OR_NULL(handle)){
                ion_sys_record_t record_param;
                record_param.client = client;
                record_param.pid = client->pid;
                record_param.group_id = client->pid;
                record_param.buffer = handle->buffer;
                record_param.handle = handle;
                record_param.action = ION_FUNCTION_IMPORT;
                printk("[KERNEL ION_IMPORT]\n");
                record_param.backtrace_num = get_kernel_backtrace(record_param.backtrace);
		get_kernel_symbol(record_param.backtrace,record_param.backtrace_num,&(record_param.kernel_symbol[0]));
                record_ion_info((int)1,&record_param);
		printk("[KERNEL ION_IMPORT] DONE\n");

        }
#endif
	return handle;
}
EXPORT_SYMBOL(ion_import_dma_buf);

static int ion_sync_for_device(struct ion_client *client, int fd)
{
	struct dma_buf *dmabuf;
	struct ion_buffer *buffer;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf))
		return PTR_ERR(dmabuf);

	/* if this memory came from ion */
	if (dmabuf->ops != &dma_buf_ops) {
		pr_err("%s: can not sync dmabuf from another exporter\n",
		       __func__);
		dma_buf_put(dmabuf);
		return -EINVAL;
	}
	buffer = dmabuf->priv;

        dma_sync_sg_for_device(NULL, buffer->sg_table->sgl,
                               buffer->sg_table->nents, DMA_BIDIRECTIONAL);
	dma_buf_put(dmabuf);
	return 0;
}
#if ION_DEBUG
#define ion_alloc ion_alloc_user
#define ion_free  ion_free_user
#define ion_share_dma_buf ion_share_dma_buf_user
#define ion_import_dma_buf ion_import_dma_buf_user
#define ion_client_create ion_client_create_user
#define ion_client_destroy ion_client_destroy_user
#endif
static long ion_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct ion_client *client = filp->private_data;

	switch (cmd) {
	case ION_IOC_ALLOC:
	{
		struct ion_allocation_data data;

		if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
			return -EFAULT;
		data.handle = ion_alloc(client, data.len, data.align,
					     data.heap_mask, data.flags);

		if (IS_ERR(data.handle))
			return PTR_ERR(data.handle);

		if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
			ion_free(client, data.handle);
			return -EFAULT;
		}
		break;
	}
	case ION_IOC_FREE:
	{
		struct ion_handle_data data;
		bool valid;

		if (copy_from_user(&data, (void __user *)arg,
				   sizeof(struct ion_handle_data)))
			return -EFAULT;
		mutex_lock(&client->lock);
		valid = ion_handle_validate(client, data.handle);
		mutex_unlock(&client->lock);
		if (!valid)
			return -EINVAL;
		ion_free(client, data.handle);
		break;
	}
	case ION_IOC_SHARE:
	{
		struct ion_fd_data data;

		if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
			return -EFAULT;
		data.fd = ion_share_dma_buf(client, data.handle);
		if (copy_to_user((void __user *)arg, &data, sizeof(data)))
			return -EFAULT;
        if (data.fd < 0)
            return data.fd;
		break;
	}
	case ION_IOC_IMPORT:
	{
		struct ion_fd_data data;
		int ret = 0;
		if (copy_from_user(&data, (void __user *)arg,
				   sizeof(struct ion_fd_data)))
			return -EFAULT;
		data.handle = ion_import_dma_buf(client, data.fd);
                if (IS_ERR(data.handle)) {
		    ret = PTR_ERR(data.handle);
			data.handle = NULL;
		}
		if (copy_to_user((void __user *)arg, &data,
				 sizeof(struct ion_fd_data)))
			return -EFAULT;
		if (ret < 0)
		    return ret;
		break;
	}
	case ION_IOC_SYNC:
	{
		struct ion_fd_data data;
		if (copy_from_user(&data, (void __user *)arg,
				   sizeof(struct ion_fd_data)))
			return -EFAULT;
		ion_sync_for_device(client, data.fd);
		break;
	}
	case ION_IOC_CUSTOM:
	{
		struct ion_device *dev = client->dev;
		struct ion_custom_data data;

		if (!dev->custom_ioctl)
			return -ENOTTY;
		if (copy_from_user(&data, (void __user *)arg,
				sizeof(struct ion_custom_data)))
			return -EFAULT;
#if ION_DEBUG
{
		ion_sys_data_t Param;
		copy_from_user(&Param, (void __user *)data.arg, sizeof(ion_sys_data_t));
		if(Param.sys_cmd == ION_SYS_RECORD)
		{
			//printk(">> ION FUNCTION (%d)start \n",Param.record_param.action);
		}
}
#endif
		return dev->custom_ioctl(client, data.cmd, data.arg);
	}
	default:
		return -ENOTTY;
	}
	return 0;
}

static int ion_release(struct inode *inode, struct file *file)
{
	struct ion_client *client = file->private_data;

	pr_debug("%s: %d\n", __func__, __LINE__);
	ion_client_destroy(client);
	return 0;
}

static int ion_open(struct inode *inode, struct file *file)
{
	struct miscdevice *miscdev = file->private_data;
	struct ion_device *dev = container_of(miscdev, struct ion_device, dev);
	struct ion_client *client;

	pr_debug("%s: %d\n", __func__, __LINE__);
	client = ion_client_create(dev, -1, "user");
	if (IS_ERR_OR_NULL(client))
		return PTR_ERR(client);
	file->private_data = client;

	return 0;
}

static const struct file_operations ion_fops = {
	.owner          = THIS_MODULE,
	.open           = ion_open,
	.release        = ion_release,
	.unlocked_ioctl = ion_ioctl,
};
#if ION_DEBUG
#undef ion_alloc
#undef ion_free
#undef ion_share_dma_buf
#undef ion_import_dma_buf
#undef ion_client_create
#undef ion_client_destroy
#endif

static size_t ion_debug_heap_total(struct ion_client *client,
				   enum ion_heap_type type)
{
	size_t size = 0;
	struct rb_node *n;

	mutex_lock(&client->lock);
	for (n = rb_first(&client->handles); n; n = rb_next(n)) {
		struct ion_handle *handle = rb_entry(n,
						     struct ion_handle,
						     node);
		if (handle->buffer->heap->type == type)
			size += handle->buffer->size;
	}
	mutex_unlock(&client->lock);
	return size;
}

static int ion_debug_heap_show(struct seq_file *s, void *unused)
{
	struct ion_heap *heap = s->private;
	struct ion_device *dev = heap->dev;
	struct rb_node *n;
        size_t total_size = 0;
        size_t total_orphaned_size = 0;

	seq_printf(s, "%16.s %16.s %16.s\n", "client", "pid", "size");
        seq_printf(s, "----------------------------------------------------\n");

	for (n = rb_first(&dev->clients); n; n = rb_next(n)) {
		struct ion_client *client = rb_entry(n, struct ion_client,
						     node);
		size_t size = ion_debug_heap_total(client, heap->type);
		if (!size)
			continue;
		if (client->task) {
			char task_comm[TASK_COMM_LEN];

			get_task_comm(task_comm, client->task);
			seq_printf(s, "%16.s %16u %16u\n", task_comm,
				   client->pid, size);
		} else {
			seq_printf(s, "%16.s %16u %16u\n", client->name,
				   client->pid, size);
		}
	}
        seq_printf(s, "----------------------------------------------------\n");
        seq_printf(s, "orphaned allocations (info is from last known client):"
                   "\n");
        mutex_lock(&dev->buffer_lock);
        for (n = rb_first(&dev->buffers); n; n = rb_next(n)) {
                struct ion_buffer *buffer = rb_entry(n, struct ion_buffer,
                                                     node);
                if (buffer->heap->type != heap->type)
                        continue;
                total_size += buffer->size;
                if (!buffer->handle_count) {
                        seq_printf(s, "%16.s %16u %16u %d %d\n", buffer->task_comm,
                                   buffer->pid, buffer->size, buffer->kmap_cnt,
                                   atomic_read(&buffer->ref.refcount));
                        total_orphaned_size += buffer->size;
                }
        }
        mutex_unlock(&dev->buffer_lock);
        seq_printf(s, "----------------------------------------------------\n");
        seq_printf(s, "%16.s %16u\n", "total orphaned",
                   total_orphaned_size);
        seq_printf(s, "%16.s %16u\n", "total ", total_size);
        seq_printf(s, "----------------------------------------------------\n");

        if (heap->debug_show)
                heap->debug_show(heap, s, unused);

	return 0;
}

static int ion_debug_heap_open(struct inode *inode, struct file *file)
{
	return single_open(file, ion_debug_heap_show, inode->i_private);
}

static const struct file_operations debug_heap_fops = {
	.open = ion_debug_heap_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void ion_device_add_heap(struct ion_device *dev, struct ion_heap *heap)
{
	struct rb_node **p = &dev->heaps.rb_node;
	struct rb_node *parent = NULL;
	struct ion_heap *entry;

	if (!heap->ops->allocate || !heap->ops->free || !heap->ops->map_dma ||
	    !heap->ops->unmap_dma)
		pr_err("%s: can not add heap with invalid ops struct.\n",
		       __func__);

	heap->dev = dev;
        down_write(&dev->lock);
	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct ion_heap, node);

		if (heap->id < entry->id) {
			p = &(*p)->rb_left;
		} else if (heap->id > entry->id ) {
			p = &(*p)->rb_right;
		} else {
			pr_err("%s: can not insert multiple heaps with "
				"id %d\n", __func__, heap->id);
			goto end;
		}
	}

	rb_link_node(&heap->node, parent, p);
	rb_insert_color(&heap->node, &dev->heaps);
	debugfs_create_file(heap->name, 0664, dev->debug_root, heap,
			    &debug_heap_fops);
end:
        up_write(&dev->lock);
}

struct ion_device *ion_device_create(long (*custom_ioctl)
				     (struct ion_client *client,
				      unsigned int cmd,
				      unsigned long arg))
{
	struct ion_device *idev;
	int ret;

	idev = kzalloc(sizeof(struct ion_device), GFP_KERNEL);
	if (!idev)
		return ERR_PTR(-ENOMEM);

	idev->dev.minor = MISC_DYNAMIC_MINOR;
	idev->dev.name = "ion";
	idev->dev.fops = &ion_fops;
	idev->dev.parent = NULL;
	ret = misc_register(&idev->dev);
	if (ret) {
		pr_err("ion: failed to register misc device.\n");
		return ERR_PTR(ret);
	}

	idev->debug_root = debugfs_create_dir("ion", NULL);
	if (IS_ERR_OR_NULL(idev->debug_root))
		pr_err("ion: failed to create debug files.\n");

	idev->custom_ioctl = custom_ioctl;
	idev->buffers = RB_ROOT;
        mutex_init(&idev->buffer_lock);
        init_rwsem(&idev->lock);
	idev->heaps = RB_ROOT;
	idev->clients = RB_ROOT;
#if ION_DEBUG
	/* Create ION Debug DB Root */
	ion_debug_create_db(idev->debug_root);
#endif
	return idev;
}

void ion_device_destroy(struct ion_device *dev)
{
	misc_deregister(&dev->dev);
	/* XXX need to free the heaps and clients ? */
	kfree(dev);
}

void __init ion_reserve(struct ion_platform_data *data)
{
	int i, ret;

	for (i = 0; i < data->nr; i++) {
		if (data->heaps[i].size == 0)
			continue;
		ret = memblock_reserve(data->heaps[i].base,
				       data->heaps[i].size);
		if (ret)
			pr_err("memblock reserve of %x@%lx failed\n",
			       data->heaps[i].size,
			       data->heaps[i].base);
	}
}
#if ION_DEBUG
struct ion_client *ion_client_create_user(struct ion_device *dev,
                                     unsigned int heap_mask, const char *name)
{
	struct ion_client *client;
	struct task_struct *task;
	struct rb_node **p;
	struct rb_node *parent = NULL;
	struct ion_client *entry;
	char debug_name[64];
	pid_t pid;

	get_task_struct(current->group_leader);
	task_lock(current->group_leader);
	pid = task_pid_nr(current->group_leader);
	/* don't bother to store task struct for kernel threads,
	   they can't be killed anyway */
	if (current->group_leader->flags & PF_KTHREAD) {
		put_task_struct(current->group_leader);
		task = NULL;
	} else {
		task = current->group_leader;
	}
	task_unlock(current->group_leader);

	client = kzalloc(sizeof(struct ion_client), GFP_KERNEL);
	if (!client) {
		if (task)
			put_task_struct(current->group_leader);
		return ERR_PTR(-ENOMEM);
	}

	client->dev = dev;
	client->handles = RB_ROOT;
	mutex_init(&client->lock);
	client->name = name;
	client->heap_mask = heap_mask;
	client->task = task;
	client->pid = pid;

        down_write(&dev->lock);
	p = &dev->clients.rb_node;
	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct ion_client, node);

		if (client < entry)
			p = &(*p)->rb_left;
		else if (client > entry)
			p = &(*p)->rb_right;
	}
	rb_link_node(&client->node, parent, p);
	rb_insert_color(&client->node, &dev->clients);

	snprintf(debug_name, 64, "%u", client->pid);
	client->debug_root = debugfs_create_file(debug_name, 0664,
						 dev->debug_root, client,
						 &debug_client_fops);
        up_write(&dev->lock);
	#if ION_DEBUG
	{
/* Create checking_leakage entry */
		ion_debug_db_create_clentry(pid);
		snprintf(debug_name, 64, "%u", client->pid);
		client->debug_root = debugfs_create_file(debug_name, 0664,
						 dev->debug_root, client,
						 &debug_client_fops);
	}
	#endif
	

	return client;
}

void ion_client_destroy_user(struct ion_client *client)
{
 	struct ion_device *dev = client->dev;
	struct rb_node *n;

	pr_debug("%s: %d\n", __func__, __LINE__);
	while ((n = rb_first(&client->handles))) {
		struct ion_handle *handle = rb_entry(n, struct ion_handle,
						     node);
		ion_handle_destroy(&handle->ref);
	}
        down_write(&dev->lock);
	if (client->task)
		put_task_struct(client->task);
	rb_erase(&client->node, &dev->clients);
	debugfs_remove_recursive(client->debug_root);
        up_write(&dev->lock);
#if ION_DEBUG
	/* Destroy checking_leakage entry */
	ion_debug_db_destroy_clentry(client->pid);
#endif

	kfree(client);
}

struct ion_handle *ion_alloc_user(struct ion_client *client, size_t len,
                             size_t align, unsigned int heap_mask,
                             unsigned int flags)
{
	struct rb_node *n;
	struct ion_handle *handle;
	struct ion_device *dev = client->dev;
	struct ion_buffer *buffer = NULL;

	pr_debug("%s: len %d align %d heap_mask %u flags %x\n", __func__, len,
		 align, heap_mask, flags);
	/*
	 * traverse the list of heaps available in this system in priority
	 * order.  If the heap type is supported by the client, and matches the
	 * request of the caller allocate from it.  Repeat until allocate has
	 * succeeded or all heaps have been tried
	 */
	if (WARN_ON(!len))
		return ERR_PTR(-EINVAL);

	len = PAGE_ALIGN(len);

        down_read(&dev->lock);
	for (n = rb_first(&dev->heaps); n != NULL; n = rb_next(n)) {
		struct ion_heap *heap = rb_entry(n, struct ion_heap, node);
		/* if the client doesn't support this heap type */
		if (!((1 << heap->type) & client->heap_mask))
			continue;
		/* if the caller didn't specify this heap type */
		if (!((1 << heap->id) & heap_mask))
			continue;
		buffer = ion_buffer_create(heap, dev, len, align, flags);
		if (!IS_ERR_OR_NULL(buffer))
			break;
	}
        up_read(&dev->lock);

	if (buffer == NULL)
		return ERR_PTR(-ENODEV);

	if (IS_ERR(buffer))
		return ERR_PTR(PTR_ERR(buffer));

	handle = ion_handle_create(client, buffer);
	/*
	 * ion_buffer_create will create a buffer with a ref_cnt of 1,
	 * and ion_handle_create will take a second reference, drop one here
	 */
	ion_buffer_put(buffer);

	if (!IS_ERR(handle)) {
		mutex_lock(&client->lock);
		ion_handle_add(client, handle);
		mutex_unlock(&client->lock);
	}


	return handle;
}


void ion_free_user(struct ion_client *client, struct ion_handle *handle)
{
 	bool valid_handle;

	BUG_ON(client != handle->client);

	mutex_lock(&client->lock);
	valid_handle = ion_handle_validate(client, handle);

	if (!valid_handle) {
		WARN(1, "%s: invalid handle passed to free.\n", __func__);
                mutex_unlock(&client->lock);
		return;
	}
	ion_handle_put(handle);
        mutex_unlock(&client->lock);
}

int ion_share_dma_buf_user(struct ion_client *client, struct ion_handle *handle)
{
	struct ion_buffer *buffer;
	struct dma_buf *dmabuf;
	bool valid_handle;
	int fd;

	mutex_lock(&client->lock);
	valid_handle = ion_handle_validate(client, handle);
	mutex_unlock(&client->lock);
	if (!valid_handle) {
		WARN(1, "%s: invalid handle passed to share.\n", __func__);
		return -EINVAL;
	}

	buffer = handle->buffer;
	ion_buffer_get(buffer);
	dmabuf = dma_buf_export(buffer, &dma_buf_ops, buffer->size, O_RDWR);
	if (IS_ERR(dmabuf)) {
		ion_buffer_put(buffer);
		return PTR_ERR(dmabuf);
	}
	fd = dma_buf_fd(dmabuf, O_CLOEXEC);
	if (fd < 0)
		dma_buf_put(dmabuf);
	return fd;
}

struct ion_handle *ion_import_dma_buf_user(struct ion_client *client, int fd)
{
 	struct dma_buf *dmabuf;
	struct ion_buffer *buffer;
	struct ion_handle *handle;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf))
		return ERR_PTR(PTR_ERR(dmabuf));
	/* if this memory came from ion */

	if (dmabuf->ops != &dma_buf_ops) {
		pr_err("%s: can not import dmabuf from another exporter\n",
		       __func__);
		dma_buf_put(dmabuf);
		return ERR_PTR(-EINVAL);
	}
	buffer = dmabuf->priv;

	mutex_lock(&client->lock);
	/* if a handle exists for this buffer just take a reference to it */
	handle = ion_handle_lookup(client, buffer);
	if (!IS_ERR_OR_NULL(handle)) {
		ion_handle_get(handle);
		goto end;
	}
	handle = ion_handle_create(client, buffer);
	if (IS_ERR_OR_NULL(handle))
		goto end;
	ion_handle_add(client, handle);

end:
	mutex_unlock(&client->lock);
	dma_buf_put(dmabuf);
	return handle;
}

static void ion_dma_buf_release_user(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	ion_buffer_put(buffer);
}
/*
 * ION Debug assistant function
 */
static void *ion_get_list_from_buffer(struct ion_buffer *buf, unsigned int type)
{
	struct ion_buffer_record *buf_rec = NULL;
	
	/* Get the inuse buffer record */
	buf_rec = ion_get_inuse_buffer_record();
	if (!buf_rec) {
		printk(KERN_WARNING "No inuse buffers!\n");
		return NULL;
	}

	/* Go through it */
	do {
		/* We only need to find out the record with corresponding buffer */
		if (buf_rec->buffer_address == buf) {
			return ion_get_list(LIST_BUFFER,buf_rec, type);
		}
		/* Next record */
		buf_rec = buf_rec->next;
	} while (!!buf_rec);

	return NULL;
}

/*
 * ION Debug assistant function
 */
static void *ion_get_list_from_process(pid_t pid, unsigned int type)
{
        struct ion_process_record *process_rec = NULL;

        /* Get the inuse buffer record */
        process_rec = (struct ion_process_record *)ion_get_inuse_process_usage_record2();
        if (!process_rec) {
                printk(KERN_WARNING "No inuse process!\n");
                return NULL;
        }

        /* Go through it */
        do {
                /* We only need to find out the record with corresponding buffer */
                if (process_rec->pid == pid) {
                        return ion_get_list(LIST_PROCESS,process_rec, type);
                }
                /* Next record */
                process_rec = process_rec->next;
        } while (!!process_rec);

        return NULL;
}

/*
 * ION Debug assistant function
 */
static void *ion_get_client_record(struct ion_client *client)
{
        struct ion_client_usage_record *client_rec = NULL;

        /* Get the inuse buffer record */
        client_rec = ion_get_inuse_client_record();
        if (!client_rec) {
                printk(KERN_WARNING "No inuse client!\n");
                return NULL;
        }

        /* Go through it */
        do {
                /* We only need to find out the record with corresponding buffer */
                if ((client_rec->tracking_info.recordID.client_address == client) && (client_rec->tracking_info.recordID.group_pid == client->pid)) {
                        return (void *)client_rec;
                }
                /* Next record */
                client_rec = client_rec->next;
        } while (!!client_rec);

        return NULL;
}


/*
 * ION Debug DB assistant function of showing backtrace
 */
static void ion_debugdb_show_backtrace(struct seq_file *s, struct ion_record_basic_info *ti, unsigned int sbt)
{
	unsigned int i = 0;
	unsigned int backtrace_count = 0;
	ObjectEntry *tmp = NULL;
	unsigned int stringCount = KSYM_SYMBOL_LEN + 30;
	printk("%s [%d]\n",__FUNCTION__,__LINE__);

	if (ti == NULL) {
		return 0;
	}
	
	printk("%s [%d]\n",__FUNCTION__,__LINE__);
	
	if (sbt == ALLOCATE_BACKTRACE_INFO) {
		tmp = (ObjectEntry *)ti->allocate_backtrace;
		if (tmp == NULL)
			return 0;
	printk("%s [%d]\n",__FUNCTION__,__LINE__);
		backtrace_count = tmp->numEntries;
	} else if (sbt == RELEASE_BACKTRACE_INFO) {
		tmp = (ObjectEntry *)ti->release_backtrace;
		if(tmp == NULL)
			return 0;
	printk("%s [%d]\n",__FUNCTION__,__LINE__);
		backtrace_count = tmp->numEntries;
	}

	printk("%s [%d] backtrace_count = (%d)\n",__FUNCTION__,__LINE__,backtrace_count);
	if (backtrace_count != 0) {
		seq_printf(s, "%15s\n", "[BACKTRACE]");
	}

	for (i = 0;i < backtrace_count;i++) {
		char tmpString[stringCount];
		ion_get_backtrace_info(ti, tmpString, stringCount, i, sbt);
		seq_printf(s, "%10s %s", "::", tmpString);
	}
}

/*
 * ION Debug DB file operations
 */

extern struct ion_device *g_ion_device;
static int ion_debug_dbcl_show(struct seq_file *s, void *unused)
{
	unsigned long key = s->private;
	pid_t raw_key;
	enum dbcl_types type;

	struct ion_device *dev = g_ion_device;
	struct rb_node *cn, *hn;
	struct ion_client *client;
	struct ion_handle *handle;
	struct ion_buffer *buffer;
	int client_cnt = 0, buffer_cnt = 0;
	void *record = NULL;

	/*
	 *  Here is an introduction about how we convert key to raw_key.
	 *
	 *  Firstly, we have following observations,
	 *  1. Process IDs have a maximum bound of pid_max, which is rarely larger than PID_MAX_DEFAULT(0x8000).

	 *  (No-use)2. Kernel modules often have higher value than 0xbf000000 and are page-aligned.
	 *  (No-use)3. Other kernel parts often have higher value than 0xc0000000.
	 *
	 *  Based on above observations, we can using following rules to change raw_key to key & vice versa.
	 *  1. For processes, we use ((dbcl_types << 16) | raw_key) as the key, in which raw_key equals Process ID.
	 
	 *  (No-use)2. For kernel modules, we use (raw_key | dbcl_types) as the key, in which raw_key is the virtual address the module is resident in.
	 *  (No-use)3. For other kernel parts, we use dbcl_types as the key.
	 *
	 */

#if 0
	if (unlikely(key >= 0xbf000000)) {
		/* Rarely-used case */
	} else if (likely(key >= 0x8000)) {
		type = key >> 16;
		raw_key = key & 0xffff;
	} else {
		/* Rarely-used case */
	}
#endif

	/* Which type */
	type = key >> 16;
	
	/* Which process */
	raw_key = key & 0xffff;
	seq_printf(s, "Process [%d]\n", raw_key);

	/* Which type */
	switch (type) {
		case DBCL_CLIENT:
			/* Lv1 - all clients
			 * Lv2 - all client-handles
			 * Lv3 - all client-handle-buffers
			 */
			printk(KERN_INFO "DBCL_CLIENT\n");
			{
				/* Lv1 - all clients */
				for (cn = rb_first(&dev->clients); cn; cn = rb_next(cn)) {
					client = rb_entry(cn, struct ion_client, node);
					/* Matched clients */
					if (client->pid == raw_key) {
						seq_printf(s, "%-8s[%2d] %12p\n", "client", client_cnt++, client);
						mutex_lock(&client->lock);
						/* Lv2 - all client-handles */
						for (hn = rb_first(&client->handles); hn; hn = rb_next(hn)) {
							handle = rb_entry(hn, struct ion_handle, node);
							seq_printf(s, "%10s[%2d] kmap_cnt(%d)\n", "handle", buffer_cnt, handle->kmap_cnt);
							/* Lv3 - all client-handle-buffers */
							buffer = handle->buffer;
							mutex_lock(&buffer->lock);
							seq_printf(s, "%10s[%2d] heap(%s) flags(%d) size(%d) kmap_cnt(%d) kvaddr(0x%x)\n",
		   						      "buffer", buffer_cnt++, buffer->heap->name, buffer->flags, 
								      buffer->size, buffer->kmap_cnt, buffer->vaddr);
							mutex_unlock(&buffer->lock);
						}
						mutex_unlock(&client->lock);
					}
				}	
			}
			break;
		case DBCL_BUFFER:
			/* Lv1 - all buffers
			 * Lv2 - all buffer-usage
			 * */
			printk(KERN_INFO "DBCL_BUFFER\n");
			{
				struct ion_buffer_usage_record *usg_rec;

				/* Find matched clients */
				for (cn = rb_first(&dev->clients); cn; cn = rb_next(cn)) {
					client = rb_entry(cn, struct ion_client, node);
					/* Matched clients */
					if (client->pid == raw_key) {
						mutex_lock(&client->lock);
						/* Lv1 - all buffers */
						for (hn = rb_first(&client->handles); hn; hn = rb_next(hn)) {
							handle = rb_entry(hn, struct ion_handle, node);
							buffer = handle->buffer;
							mutex_lock(&buffer->lock);
							seq_printf(s, "%  s[%2d] size(%d) %12p\n", "buffer", buffer_cnt++, buffer->size, buffer);
							mutex_unlock(&buffer->lock);
							/* Lv2 - all buffer-usage */
							seq_printf(s, "%10s\n","<BUFFER_ALLOCATION_LIST>");
							usg_rec = ion_get_list_from_buffer(buffer, BUFFER_ALLOCATION_LIST);
							while (!!usg_rec) {
								seq_printf(s, "%10s [%d] (%s [%d]) (%s [%d])\n","Process", 
													usg_rec->tracking_info.recordID.pid,
													"GroupLeader", 
													usg_rec->tracking_info.recordID.group_pid,
													"Client",
													usg_rec->tracking_info.recordID.client_address
													);
								/* Show buffer allocation backtrace */
								ion_debugdb_show_backtrace(s, &usg_rec->tracking_info, ALLOCATE_BACKTRACE_INFO);
								/* Next buffer usage record */
								usg_rec = (struct ion_buffer_usage_record *)ion_get_data_from_record((void *)usg_rec, RECORD_NEXT);
							}
							seq_printf(s, "%10s\n","<BUFFER_FREE_LIST>");
							usg_rec = ion_get_list_from_buffer(buffer, BUFFER_FREE_LIST);
							while (!!usg_rec) {
								seq_printf(s, "%10s [%d] (%s [%d])\n","Process", 
													usg_rec->tracking_info.recordID.pid,
													"GroupLeader", 
													usg_rec->tracking_info.recordID.group_pid);
								/* Show buffer free backtrace */
								ion_debugdb_show_backtrace(s, &usg_rec->tracking_info, RELEASE_BACKTRACE_INFO);
								/* Next buffer usage record */
								usg_rec = (struct ion_buffer_usage_record *)ion_get_data_from_record((void *)usg_rec, RECORD_NEXT);
							}
							seq_printf(s, "%10s\n","==================================================");
						}
						mutex_unlock(&client->lock);
					}
				}
			}
			break;
		case DBCL_MMAP:
			/* Lv1 - all buffers 
			 * Lv2 - all buffer-mmaps 
			 */
			printk(KERN_INFO "DBCL_MMAP\n");
			{
				struct ion_address_usage_record *adr_rec;
				struct ion_client_usage_record *client_rec;
				/* Find matched clients */
				for (cn = rb_first(&dev->clients); cn; cn = rb_next(cn)) {
					client = rb_entry(cn, struct ion_client, node);
					/* Matched clients */
					if (client->pid == raw_key) {
						mutex_lock(&client->lock);
						/* Lv1 - all buffers */
						for (hn = rb_first(&client->handles); hn; hn = rb_next(hn)) {
							handle = rb_entry(hn, struct ion_handle, node);
							buffer = handle->buffer;
							mutex_lock(&buffer->lock);
							seq_printf(s, "%-8s[%2d] size(%d) %12p\n",
		   						      "buffer", buffer_cnt++, buffer->size, buffer);
							mutex_unlock(&buffer->lock);
							/* Lv2 - all buffer-mmaps */
							seq_printf(s, "%10s\n","<ADDRESS_ALLOCATION_LIST_IN_KERNELSPACE>");
							adr_rec = ion_get_list_from_buffer(buffer, ADDRESS_ALLOCATION_LIST);
							while (!!adr_rec) {
								seq_printf(s, "%10s [%d] - %10s [0x%x]-[0x%x]%10s [%d]\n",
										"Process", adr_rec->tracking_info.recordID.pid,
										"Address", adr_rec->mapping_address,adr_rec->mapping_address+adr_rec->size,
										"Size", adr_rec->size);
								/* Show address allocation backtrace */
								ion_debugdb_show_backtrace(s, &adr_rec->tracking_info, ALLOCATE_BACKTRACE_INFO);
								/* Next address record */
								adr_rec = (struct ion_address_usage_record *)ion_get_data_from_record((void *)adr_rec, RECORD_NEXT);
							}

							seq_printf(s, "%10s\n","<ADDRESS_FREE_LIST_FROM_IN_KERNELSPACE>");
							adr_rec = ion_get_list_from_buffer(buffer, ADDRESS_FREE_LIST);
							while (!!adr_rec) {
								seq_printf(s, "%10s [%d] - %10s [0x%x]-[0x%x] %10s [%d]\n",
										"Process", adr_rec->tracking_info.recordID.pid,
										"Address", adr_rec->mapping_address,adr_rec->mapping_address+adr_rec->size,
										"Size", adr_rec->size);
								/* Show address release backtrace */
								ion_debugdb_show_backtrace(s, &adr_rec->tracking_info, RELEASE_BACKTRACE_INFO);
								/* Next address record */
								adr_rec = (struct ion_address_usage_record *)ion_get_data_from_record((void *)adr_rec, RECORD_NEXT);
							}
						
						}
						        client_rec = (struct ion_client_usage_record *)ion_get_client_record(client);
                                                        printk("[MMAP] prepare to get mmap in client_rec  %x input is %x groupd id is %d\n",client_rec,client,client->pid);
                                                        if(client_rec != NULL)
                                                        {
                                                                printk("[MMAP] client pid is %d\n",client_rec->tracking_info.recordID.pid);
                                                                adr_rec = ion_get_list_from_process(client_rec->tracking_info.recordID.pid, ADDRESS_ALLOCATION_LIST);
								if(adr_rec != NULL)
									seq_printf(s, "%10s\n","<ADDRESS_ALLOCATION_LIST_IN_USERSPACE>");
                                                                while (!!adr_rec) {
                                                                seq_printf(s, "%10s [%d] - %10s [0x%x]-[0x%x] %10s [%d]\n",
                                                                                "Process", adr_rec->tracking_info.recordID.pid,
                                                                                "Address", adr_rec->mapping_address,adr_rec->mapping_address+adr_rec->size,
                                                                                "Size", adr_rec->size);
                                                                        /* Show address allocation backtrace */
                                                                        ion_debugdb_show_backtrace(s, &adr_rec->tracking_info, ALLOCATE_BACKTRACE_INFO);
                                                                        /* Next address record */
                                                                        adr_rec = (struct ion_address_usage_record *)ion_get_data_from_record((void *)adr_rec, RECORD_NEXT);
                                                                }
                                                                adr_rec = ion_get_list_from_process(client_rec->tracking_info.recordID.pid, ADDRESS_FREE_LIST);
                                                               	if(adr_rec != NULL)
									seq_printf(s, "%10s\n","<ADDRESS_FREE_LIST_IN_USERSPACE>"); 
                                                                while (!!adr_rec) {
                                                                        seq_printf(s, "%10s [%d] - %10s [0x%x]-[0x%x] %10s [%d]\n",
                                                                                "Process", adr_rec->tracking_info.recordID.pid,
                                                                                "Address", adr_rec->mapping_address,adr_rec->mapping_address+adr_rec->size,
                                                                                "Size", adr_rec->size);
                                                                        /* Show address release backtrace */
                                                                        ion_debugdb_show_backtrace(s, &adr_rec->tracking_info, RELEASE_BACKTRACE_INFO);
                                                                        /* Next address record */
                                                                        adr_rec = (struct ion_address_usage_record *)ion_get_data_from_record((void *)adr_rec, RECORD_NEXT);
                                                                }

                                                        }

						mutex_unlock(&client->lock);
					}
				}
			}
			break;
		case DBCL_FD:
			/* Lv1 - all buffers
			 * Lv2 - all buffer-fds
			 */
			printk(KERN_INFO "DBCL_FD\n");
			{
				struct ion_fd_usage_record *fd_rec;
				struct ion_client_usage_record *client_rec;

				/* Find matched clients */
				for (cn = rb_first(&dev->clients); cn; cn = rb_next(cn)) {
					client = rb_entry(cn, struct ion_client, node);
					/* Matched clients */
					if (client->pid == raw_key) {
						mutex_lock(&client->lock);
						/* Lv1 - all buffers */
					 	client_rec = (struct ion_client_usage_record *)ion_get_client_record(client);
                                                printk("[FD] client_rec  %x input is %x groupd id is %d\n",client_rec,client,client->pid);
                                                if(client_rec != NULL)
                                                {
                                                	printk("[FD] client pid is %d\n",client_rec->tracking_info.recordID.pid);
                                                	fd_rec = ion_get_list_from_process(client_rec->tracking_info.recordID.pid, FD_ALLOCATION_LIST);
                                                	if(fd_rec != NULL)
                                                		seq_printf(s, "%10s\n","<FD_ALLOCATION_LIST>");
                                                        printk("[FD] get fd_rec %x\n",fd_rec);
                                                        while (!!fd_rec) {
                                                                seq_printf(s, "%10s [%d] %10s [%d]\n",
                                                                "Process", fd_rec->tracking_info.recordID.pid,
                                                                "fd", fd_rec->fd);
                                                                /* Show address allocation backtrace */
                                                                ion_debugdb_show_backtrace(s, &fd_rec->tracking_info, ALLOCATE_BACKTRACE_INFO);
                                                                /* Next address record */
                                                                fd_rec = (struct ion_address_usage_record *)ion_get_data_from_record((void *)fd_rec, RECORD_NEXT);
                                                 	}
                                                        fd_rec = ion_get_list_from_process(client_rec->tracking_info.recordID.pid, FD_FREE_LIST);
                                                        if(fd_rec != NULL)
                                                        	seq_printf(s, "%10s\n","<FD_FREE_LIST>");
                                                        while (!!fd_rec) {
                                                                seq_printf(s, "%10s [%d] %10s [%d]\n",
                                                                "Process", fd_rec->tracking_info.recordID.pid,
                                                                "fd", fd_rec->fd);
                                                        	/* Show address release backtrace */
                                                        	ion_debugdb_show_backtrace(s, &fd_rec->tracking_info, RELEASE_BACKTRACE_INFO);
                                                        	/* Next address record */
                                                        	fd_rec = (struct ion_address_usage_record *)ion_get_data_from_record((void *)fd_rec, RECORD_NEXT);
                                                        }
                                                 }
						mutex_unlock(&client->lock);
					}
				}
			}
			break;
		default:
			break;
	}
	
	return 0;
}

static int ion_debug_dbcl_open(struct inode *inode, struct file *file)
{
	return single_open(file, ion_debug_dbcl_show, inode->i_private);
}


static const struct file_operations debug_dbcl_fops = {
	.open = ion_debug_dbcl_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ion_debug_dbis_show(struct seq_file *s, void *unused)
{
	unsigned long type = s->private;
	struct ion_device *dev = g_ion_device;
	struct rb_node *cn, *hn;
	struct ion_client *client;
	struct ion_handle *handle;
	struct ion_buffer *buffer;
	int client_cnt = 0, buffer_cnt = 0;

	struct ion_buffer_record *buf_rec = NULL;
	
	/* History records */
	if (type >= (unsigned long)DBIS_DIR) {
		printk(KERN_INFO "ION Debug History Records\n");
		buf_rec = ion_get_freed_buffer_record();

		type -= (unsigned long)DBIS_DIR;
#if 0
		switch ((enum dbis_types)type) {
			case DBIS_BUFFERS:
				printk(KERN_INFO "DBIS_BUFFERS\n");
				break;
			case DBIS_MMAPS:
				printk(KERN_INFO "DBIS_MMAPS\n");
				break;
			case DBIS_FDS:
				printk(KERN_INFO "DBIS_FDS\n");
				break;
			default:
				break;
		}
		goto ret;
#endif
	} else {
		printk(KERN_INFO "ION Debug Non-History Records\n");
		buf_rec = ion_get_inuse_buffer_record();
	}

	/* Non-history records */
	switch ((enum dbis_types)type) {
		case DBIS_HEAPS:
			printk(KERN_INFO "DBIS_HEAPS\n");
			/* We don't need this information. This is provided by ion_debug_heap_show */
			break;
		case DBIS_CLIENTS:
			printk(KERN_INFO "DBIS_CLIENTS\n");
			{
				/* All clients */
				for (cn = rb_first(&dev->clients); cn; cn = rb_next(cn)) {
					client = rb_entry(cn, struct ion_client, node);
					seq_printf(s, "\n%8s[%2d] %12p PID[%d]\n", "client", client_cnt++, client, client->pid);
					mutex_lock(&client->lock);
					/* All client-handles */
					for (hn = rb_first(&client->handles); hn; hn = rb_next(hn)) {
						handle = rb_entry(hn, struct ion_handle, node);
						seq_printf(s, "%10s[%2d] kmap_cnt(%d)\n", "handle", buffer_cnt, handle->kmap_cnt);
						/* All client-handle-buffers */
						buffer = handle->buffer;
						mutex_lock(&buffer->lock);
						seq_printf(s, "%10s[%2d] heap(%s) address(0x%x) flags(%d) size(%d) kmap_cnt(%d) kvaddr(0x%x)\n",
								"buffer", buffer_cnt++, buffer->heap->name,buffer,buffer->flags, 
								buffer->size, buffer->kmap_cnt, buffer->vaddr);
						mutex_unlock(&buffer->lock);
					}
					mutex_unlock(&client->lock);
					buffer_cnt = 0;
				}	
			}
			break;
		case DBIS_BUFFERS:
			printk(KERN_INFO "DBIS_BUFFERS\n");
			{
				struct ion_buffer_usage_record *usg_rec;

#if 0
				buf_rec = ion_get_inuse_buffer_record();
#endif
				while (buf_rec != NULL) {
					seq_printf(s, "%8s[%2d] size(%d) %12p\n", "buffer", buffer_cnt++, buf_rec->buffer->size, buf_rec->buffer);
					/* Allocation */
					seq_printf(s, "%30s\n","<BUFFER_ALLOCATION_LIST>");
					usg_rec = ion_get_list(LIST_BUFFER,buf_rec, BUFFER_ALLOCATION_LIST);
					while (!!usg_rec) {				
						seq_printf(s, "%30s [%d] (%15s [%d])\n","Process", usg_rec->tracking_info.recordID.pid,
											"GroupLeader", usg_rec->tracking_info.recordID.group_pid);

						/* Show buffer allocation backtrace */
						ion_debugdb_show_backtrace(s, &usg_rec->tracking_info, ALLOCATE_BACKTRACE_INFO);
						/* Next buffer usage record */
						usg_rec = (struct ion_buffer_usage_record *)ion_get_data_from_record((void *)usg_rec, RECORD_NEXT);
					}
					/* Free */
					seq_printf(s, "%30s\n","<BUFFER_FREE_LIST>");

					usg_rec = ion_get_list(LIST_BUFFER,buf_rec, BUFFER_FREE_LIST);
					while (!!usg_rec) {
						seq_printf(s, "%30s [%d] (%15s [%d])\n","Process", usg_rec->tracking_info.recordID.pid,
											"GroupLeader", usg_rec->tracking_info.recordID.group_pid);
						/* Show buffer free backtrace */
						ion_debugdb_show_backtrace(s, &usg_rec->tracking_info, RELEASE_BACKTRACE_INFO);
						/* Next buffer usage record */
						usg_rec = (struct ion_buffer_usage_record *)ion_get_data_from_record((void *)usg_rec, RECORD_NEXT);
					}
					/* Next record */
					buf_rec = buf_rec->next;
				}
			}
			break;
		case DBIS_MMAPS:
			printk(KERN_INFO "DBIS_MMAPS\n");
			{
				struct ion_address_usage_record *adr_rec;

#if 0
				buf_rec = ion_get_inuse_buffer_record();
#endif
				while (buf_rec != NULL) {
					seq_printf(s, "%8s[%2d] size(%d) %12p\n", "buffer", buffer_cnt++, buf_rec->buffer->size, buf_rec->buffer);
					/* Mapping */
					seq_printf(s, "%30s\n","<ADDRESS_ALLOCATION_LIST>");
					adr_rec = ion_get_list(LIST_BUFFER,buf_rec, ADDRESS_ALLOCATION_LIST);
					while (!!adr_rec) {
						seq_printf(s, "%30s [%d] - %20s [0x%x] - %10s [%d]\n",
								"Process", adr_rec->tracking_info.recordID.pid,
								"Address", adr_rec->mapping_address,
								"Size", adr_rec->size);
						/* Show address allocation backtrace */
						ion_debugdb_show_backtrace(s, &adr_rec->tracking_info, ALLOCATE_BACKTRACE_INFO);
						/* Next address record */
						adr_rec = (struct ion_address_usage_record *)ion_get_data_from_record((void *)adr_rec, RECORD_NEXT);
					}
					/* Unmapping */
					seq_printf(s, "%30s\n","<ADDRESS_FREE_LIST>");
					adr_rec = ion_get_list(LIST_BUFFER,buf_rec, ADDRESS_FREE_LIST);
					while (!!adr_rec) {
						seq_printf(s, "%30s [%d] - %20s [0x%x] - %10s [%d]\n",
								"Process", adr_rec->tracking_info.recordID.pid,
								"Address", adr_rec->mapping_address,
								"Size", adr_rec->size);
						/* Show address release backtrace */
						ion_debugdb_show_backtrace(s, &adr_rec->tracking_info, RELEASE_BACKTRACE_INFO);
						/* Next address record */
						adr_rec = (struct ion_address_usage_record *)ion_get_data_from_record((void *)adr_rec, RECORD_NEXT);
					}
					/* Next record */
					buf_rec = buf_rec->next;
				}
			}
			break;
		case DBIS_FDS:
			printk(KERN_INFO "DBIS_FDS\n");
			{
				struct ion_fd_usage_record *fd_rec;

#if 0
				buf_rec = ion_get_inuse_buffer_record();
#endif
				while (buf_rec != NULL) {
					seq_printf(s, "%8s[%2d] size(%d) %12p\n", "buffer", buffer_cnt++, buf_rec->buffer->size, buf_rec->buffer);
					/* FD */
					fd_rec = ion_get_list(LIST_BUFFER,buf_rec, FD_ALLOCATION_LIST);
					while (!!fd_rec) {
						seq_printf(s, "%30s [%d] - %20s [0x%x]\n",
								"Process", fd_rec->tracking_info.recordID.pid,
								"Fd", fd_rec->fd);
						/* Show fd allocation backtrace */
						ion_debugdb_show_backtrace(s, &fd_rec->tracking_info, ALLOCATE_BACKTRACE_INFO);
						/* Next fd record */
						fd_rec = (struct ion_fd_usage_record *)ion_get_data_from_record((void *)fd_rec, RECORD_NEXT);
					}
					fd_rec = ion_get_list(LIST_BUFFER,buf_rec, FD_FREE_LIST);
					while (!!fd_rec) {
						seq_printf(s, "%30s [%d] - %20s [0x%x]\n",
								"Process", fd_rec->tracking_info.recordID.pid,
								"Fd", fd_rec->fd);
						/* Show fd release backtrace */
						ion_debugdb_show_backtrace(s, &fd_rec->tracking_info, RELEASE_BACKTRACE_INFO);
						/* Next fd record */
						fd_rec = (struct ion_fd_usage_record *)ion_get_data_from_record((void *)fd_rec, RECORD_NEXT);
					}
					/* Next record */
					buf_rec = buf_rec->next;
				}
			}
			break;
		case DBIS_PIDS:
			printk(KERN_INFO "DBIS_PIDS\n");
			{ 
				struct dbis_process_entry proclist = {.pid = -1, .clients = NULL, .next = NULL};
				struct dbis_process_entry *pe = NULL;
				struct dbis_client_entry *ce = NULL;
				
				/* Firstly, we should go through all clients. */
				for (cn = rb_first(&dev->clients); cn; cn = rb_next(cn)) {
					client = rb_entry(cn, struct ion_client, node);
					dbis_insert_proc_clients(&proclist, client, client->pid);
				}
				
				/* Now we can go through all processes using ION. */
				pe = proclist.next;
				while (pe != NULL) {
					seq_printf(s, "%s[%d]\n","Process", pe->pid);
					/* Go through all clients for this pe */
					ce = pe->clients;
					while (ce != NULL) {
						client = ce->client;

						/* Show all client information */
						seq_printf(s, "\n%8s[%2d] %12p\n", "client", client_cnt++, client);
						mutex_lock(&client->lock);
						/* All client-handles */
						for (hn = rb_first(&client->handles); hn; hn = rb_next(hn)) {
							handle = rb_entry(hn, struct ion_handle, node);
							seq_printf(s, "%10s[%2d] kmap_cnt(%d)\n", "handle", buffer_cnt, handle->kmap_cnt);
							/* All client-handle-buffers */
							buffer = handle->buffer;
							mutex_lock(&buffer->lock);
							seq_printf(s, "%10s[%2d] heap(%s) flags(%d) size(%d) kmap_cnt(%d) kvaddr(0x%x)\n",
									"buffer", buffer_cnt++, buffer->heap->name, buffer->flags, 
									buffer->size, buffer->kmap_cnt, buffer->vaddr);				
							mutex_unlock(&buffer->lock);
						}
						mutex_unlock(&client->lock);
						buffer_cnt = 0;
						
						/* Next ce */
						ce = ce->next;
					}
					/* Next pe */
					pe = pe->next;
				}

				/* Finally, delete all entries in proclist */
				destroy_proclist(&proclist);
			}
			break;
#if 0
		case DBIS_MODS:
			printk(KERN_INFO "DBIS_MODS\n");
			break;
#endif
		default:
			break;
	}

ret:
	return 0;
}

static int ion_debug_dbis_open(struct inode *inode, struct file *file)
{
	return single_open(file, ion_debug_dbis_show, inode->i_private);
}

static const struct file_operations debug_dbis_fops = {
	.open = ion_debug_dbis_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void ion_debug_create_db(struct dentry *root)
{
	int index;

	printk(KERN_INFO, "\n%s start!\n\n",__FUNCTION__);

	/* Create checking_leakage folder */
	debug_db_root.checking_leakage = debugfs_create_dir("checking_leakage", root);
	INIT_LIST_HEAD(&debug_db_root.dbcl.child);

	/* Create ion_statistics folder & its children */
	debug_db_root.ion_statistics = debugfs_create_dir("ion_statistics", root);
	for (index = 0; index < _TOTAL_DBIS; ++index) {
		if (dbis_child_attr[index].attr == DBIS_FILE) {
			debug_db_root.dbis.child[index] 
				= debugfs_create_file(dbis_child_attr[index].name, 0444, debug_db_root.ion_statistics, index, &debug_dbis_fops);
		} else {/* This is only for history now. */
			debug_db_root.dbis.child[index] = debugfs_create_dir(dbis_child_attr[index].name, debug_db_root.ion_statistics);
			/* buffers - Use (DBIS_BUFFERS + DBIS_DIR) to identify history/buffers */
			debug_db_root.dbis.history_record[0]
				= debugfs_create_file(dbis_child_attr[DBIS_BUFFERS].name, 0444, 
						debug_db_root.dbis.child[index], (DBIS_BUFFERS + DBIS_DIR), &debug_dbis_fops);
			/* mmaps - Use (DBIS_MMAPS + DBIS_DIR) to identify history/mmaps */
			debug_db_root.dbis.history_record[1]
				= debugfs_create_file(dbis_child_attr[DBIS_MMAPS].name, 0444, 
						debug_db_root.dbis.child[index], (DBIS_MMAPS + DBIS_DIR), &debug_dbis_fops);
			/* fds - Use (DBIS_fdS + DBIS_DIR) to identify history/fds */
			debug_db_root.dbis.history_record[2]
				= debugfs_create_file(dbis_child_attr[DBIS_FDS].name, 0444, 
						debug_db_root.dbis.child[index], (DBIS_FDS + DBIS_DIR), &debug_dbis_fops);
		}
	}

	printk(KERN_INFO, "\n%s complete!\n\n",__FUNCTION__);
}

static void ion_debug_db_create_clentry(pid_t pid)
{
	struct list_head *pos, *n;
	struct dbcl_child *found;
	char process_id[6];
	int index;
	
	/* Check whether pid is in the cl list*/
	list_for_each_safe(pos, n, &debug_db_root.dbcl.child) {
		found = list_entry(pos, struct dbcl_child, entry);
		if ((pid_t)found->raw_key == pid) {
			/* We have found one. */
			atomic_inc(&found->refcount);
			return;
		}
	}

	/* No existing entry */
	found = kmalloc(sizeof(struct dbcl_child), GFP_KERNEL);
	found->raw_key = pid;
	snprintf(process_id, 6, "%d", pid);
	found->root = debugfs_create_dir(process_id, debug_db_root.checking_leakage);
	for (index = 0; index < _TOTAL_DBCL; ++index) {
		found->type[index] = debugfs_create_file(dbcl_child_name[index], 0444, found->root, ((index << 16) | pid), &debug_dbcl_fops);
	}
	atomic_set(&found->refcount, 1);
	list_add_tail(&found->entry, &debug_db_root.dbcl.child);
}

static void ion_debug_db_destroy_clentry(pid_t pid)
{
	struct list_head *pos, *n;
	struct dbcl_child *found;
	
	/* Check whether pid is in the cl list*/
	list_for_each_safe(pos, n, &debug_db_root.dbcl.child) {
		found = list_entry(pos, struct dbcl_child, entry);
		if ((pid_t)found->raw_key == pid) {
			/* We have found one. */
			if (atomic_dec_and_test(&found->refcount)) {
				/* Delete list entry, remove corresponding debugfs dir/files, free memory. */
				list_del(&found->entry);
				debugfs_remove_recursive(found->root);
				kfree(found);
			}
			return;
		}
	}

	printk(KERN_DEBUG "Oh!!!!!\n");
}
#endif

