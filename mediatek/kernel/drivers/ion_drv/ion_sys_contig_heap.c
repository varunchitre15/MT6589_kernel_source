
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/xlog.h>
#include <linux/ion.h>
#include "ion_priv.h"
#include <linux/ion_drv.h>
#include <linux/err.h>
#include <linux/scatterlist.h>

extern struct ion_heap *g_ion_heaps[ION_HEAP_IDX_MAX];

static int ion_sys_contig_heap_allocate(struct ion_heap *heap,
                                struct ion_buffer *buffer,
                                unsigned long size, unsigned long align,
                                unsigned long flags)
{
    int ret;
    struct sg_table *table;
    void* pVA = kzalloc(size, GFP_KERNEL);
    buffer->priv_virt = NULL;
    if (IS_ERR_OR_NULL(pVA))
    {
        printk("[ion_sys_contig_heap_allocate]: Error. Allocate buffer failed.\n");
        return -ENOMEM;
    }
    table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
    if (!table)
    {
        vfree(pVA);
        return -ENOMEM;
    }
    ret = sg_alloc_table(table, 1, GFP_KERNEL);
    if (ret)
    {
        vfree(pVA);
        kfree(table);
        return -ENOMEM;
    }
    buffer->priv_virt = pVA;
    sg_set_page(table->sgl, virt_to_page(buffer->priv_virt), buffer->size, 0);
    buffer->sg_table = table;
    return 0;
}

static void ion_sys_contig_heap_free(struct ion_buffer *buffer)
{
    kfree(buffer->priv_virt);
    if (buffer->sg_table)
        sg_free_table(buffer->sg_table);
    kfree(buffer->sg_table);
}

static void *ion_sys_contig_heap_map_kernel(struct ion_heap *heap,
                                    struct ion_buffer *buffer)
{
    return buffer->priv_virt;
}

static void ion_sys_contig_heap_unmap_kernel(struct ion_heap *heap,
                                     struct ion_buffer *buffer)
{
}

static struct sg_table* ion_sys_contig_heap_map_dma(struct ion_heap *heap, struct ion_buffer *buffer)
{
    return buffer->sg_table;
}

static void ion_sys_contig_heap_unmap_dma(struct ion_heap *heap, struct ion_buffer *buffer)
{
}


static int ion_sys_contig_heap_map_user(struct ion_heap *heap, struct ion_buffer *buffer,
                                struct vm_area_struct *vma)
{
    unsigned long pfn = __phys_to_pfn(virt_to_phys(buffer->priv_virt));
    return remap_pfn_range(vma, vma->vm_start, pfn + vma->vm_pgoff,
        vma->vm_end - vma->vm_start,
        vma->vm_page_prot);
}

static int ion_sys_contig_heap_phys(struct ion_heap *heap,
                            struct ion_buffer *buffer,
                            ion_phys_addr_t *addr, size_t *len)
{
    *addr = virt_to_phys(buffer->priv_virt);
    *len = buffer->size;
    return 0;
}

struct ion_heap_ops sys_contig_heap_ops = {
    .allocate = ion_sys_contig_heap_allocate,
    .free = ion_sys_contig_heap_free,
    .map_kernel = ion_sys_contig_heap_map_kernel,
    .unmap_kernel = ion_sys_contig_heap_unmap_kernel,
    .map_dma = ion_sys_contig_heap_map_dma,
    .unmap_dma = ion_sys_contig_heap_unmap_dma,
    .map_user = ion_sys_contig_heap_map_user,
    .phys = ion_sys_contig_heap_phys,
};
