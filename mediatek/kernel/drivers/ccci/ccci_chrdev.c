#include <linux/module.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <ccci.h>


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
static DECLARE_MUTEX(ccci_dev_sem); 
#else
static DEFINE_SEMAPHORE(ccci_dev_sem);
#endif

static LIST_HEAD(ccci_dev_list);
static struct cdev *ccci_chrdev;


extern unsigned long long lg_ch_tx_debug_enable;
extern unsigned long long lg_ch_rx_debug_enable;
extern unsigned int md_ex_type;
unsigned int push_data_fail = 0;

static inline void ccci_recv_array_init(struct recv_array *array,CCCI_BUFF_T *buff,int msg_num)
{	
	array->array=buff;
	array->nr=msg_num;
	array->in_idx=0;
	array->out_idx=0;
	INIT_LIST_HEAD(&array->list);
}

static inline void ccci_client_init(struct ccci_dev_client *client,int ch,pid_t pid)
{
	WARN_ON(client==NULL);
	spin_lock_init(&client->lock);
	atomic_set(&client->user,1);
	client->pid=pid;
	client->ch_num=ch;
	client->used_nr=0;
	client->free_nr=0;
	INIT_LIST_HEAD(&client->buff_list);
	INIT_LIST_HEAD(&client->free_list);
	INIT_LIST_HEAD(&client->dev_list);
	init_waitqueue_head(&client->wait_q);
	client->fasync=NULL;
	client->wakeup_waitq=0;
}

static struct recv_array *ccci_create_array(int nr,gfp_t flags)
{	
	struct recv_array *array=kmalloc(sizeof(struct recv_array),flags);
	CCCI_BUFF_T *buff;
	WARN_ON(nr<=0);
	if (array==NULL)
	{
		CCCI_MSG_INF("chr", "Fail to kmalloc struct recv_array.\n");
		goto out;
	}
	buff=kmalloc(sizeof(CCCI_BUFF_T)*nr,flags);
	if (buff==NULL)
	{
		//CCCI_MSG_INF("chr", "Failed for kmalloc CCCI_BUFF_T (nr=%d).\n",nr);
		goto out_free;
	}
	ccci_recv_array_init(array,buff,nr);
	CCCI_CHR_MSG("Create a recv_array nr=%d\n",array->nr);
	goto out;
out_free:
	kfree(array);
	array=NULL;

out:
	return array;
}



static void release_recv_array(struct recv_array *array)
{
	WARN_ON(array==NULL);
	if (array)
	{
		CCCI_CHR_MSG("Free a recv_array nr=%d\n",array->nr);
		list_del(&array->list);
		if (array->array)	
			kfree(array->array);
		kfree(array);
	}
}

void release_recv_array_list(struct list_head *list,int *nr)
{
	struct recv_array *array,*n;
	WARN_ON(list==NULL);
	if (!list_empty(list))
	{
		list_for_each_entry_safe(array,n,list,list)
		{	
			*nr -=array->nr;
			release_recv_array(array);
		}	
		
	}
}

static void release_client_buffer(struct ccci_dev_client *client)
{
	CCCI_CHR_MSG("Before release used_nr=%d\n",client->used_nr);
	release_recv_array_list(&client->buff_list,&client->used_nr);
	CCCI_CHR_MSG("After release used_nr=%d\n",client->used_nr);

	CCCI_CHR_MSG("Before release free_nr=%d\n",client->free_nr);
	release_recv_array_list(&client->free_list,&client->free_nr);
	CCCI_CHR_MSG("After release free_nr=%d\n",client->free_nr);

}

static void release_client(struct ccci_dev_client *client)
{
	WARN_ON(spin_is_locked(&client->lock)||list_empty(&client->dev_list));
	down(&ccci_dev_sem);
	list_del(&client->dev_list);
	ccci_unregister(client->ch_num);
	release_client_buffer(client);
	kfree(client);
	up(&ccci_dev_sem);
}

static inline void ccci_put_client(struct ccci_dev_client *client)
{
	WARN_ON(client==NULL);
	if (atomic_dec_and_test(&client->user))
		release_client(client);
}

static int ccci_pop_buff(struct ccci_dev_client *client,CCCI_BUFF_T *out_buff)
{
	int ret=0;
	struct recv_array *array;
	array=ccci_get_first(&client->buff_list);
	if (array==NULL) goto out;
	WARN_ON(array&&array->array==NULL);
	if (!recv_array_is_empty(array))
	{
		*out_buff=array->array[array->out_idx];
		ret=sizeof(*out_buff);
		array->out_idx++;
		if (recv_array_is_empty(array))
		{
			if (ccci_is_last_recv(&client->buff_list))
			{
				if (recv_array_is_full(array))
					reset_recv_array(array);
			}
			else{
				WARN_ON(!recv_array_is_full(array));
				client->used_nr -=array->nr;
				if (client->free_nr <= client->used_nr || (client->free_nr+array->nr)<(CCCI_FIFO_MAX_LEN*20))
				{
					CCCI_CHR_MSG("Move array to free list nr=%d\n",array->nr);
					list_move(&array->list,&client->free_list);
					reset_recv_array(array);	
					client->free_nr +=array->nr;		
				}
				else 
				{
					release_recv_array(array);
				}
			}	
		}
	}
out:
	return ret;
}

static int ccci_push_buff(struct ccci_dev_client *client,CCCI_BUFF_T *buff)
{
	int ret=0;
	struct recv_array *array;
	array=ccci_get_last(&client->buff_list);
	WARN_ON(array&&array->array==NULL);
	if (array==NULL||recv_array_is_full(array))
	{
		
		array=ccci_get_first(&client->free_list);
		if (array==NULL)
		{
			if (client->used_nr&&(client->used_nr+client->used_nr/2)>MAX_BUFFER_MESSAGES)
			{
				CCCI_MSG_INF("chr", "Too many message need read out(ch=%d)(%dB)\n",
					client->ch_num, client->used_nr*sizeof(CCCI_BUFF_T));
				ret=-E2BIG;
				goto out;			
			}
			array=ccci_create_array((client->used_nr&&(client->used_nr/2)>CCCI_FIFO_MAX_LEN)?
					(client->used_nr/2):CCCI_FIFO_MAX_LEN*5,GFP_ATOMIC);
			if (array==NULL)
			{
				CCCI_MSG_INF("chr", "Fail alloc recv_array FIFO(%d,%d): 0x%x,0x%x,%d,0x%x \n",
					client->used_nr, client->free_nr, buff->data[0], buff->data[1], 
					buff->channel, buff->reserved);
				ret=-ENOMEM;
				push_data_fail = 1;
				goto out;
			}
		}
		else
		{
			//list_move_tail(&array->list,&client->buff_list);
			//client->used_nr +=array->nr;
			client->free_nr -=array->nr;
			CCCI_CHR_MSG("Move array from free_list to buff_list nr=%d\n",array->nr);
		}
		
		list_move_tail(&array->list,&client->buff_list);
		client->used_nr += array->nr;
	}
	WARN_ON(recv_array_is_full(array));
	array->array[array->in_idx++]=*buff;
	ret=sizeof(*buff);
out:
	
	return ret;
}



static void ccci_chrdev_callback(CCCI_BUFF_T *buff,void *private)
{
	struct ccci_dev_client *client=(struct ccci_dev_client *)private;
	int ret;
	spin_lock_bh(&client->lock);
	ret=ccci_push_buff(client,buff);
	spin_unlock_bh(&client->lock);
//	CCCI_DEBUG("Function ccci_chrdev_callback() callback.(ch=%d) client:%p\n",buff->channel,client);
	
	CCCI_CHR_MSG("callback.(ch=%d) client:%p\n",buff->channel,client);

#if 0	
	if (ret!=sizeof(*buff))
	{
		CCCI_MSG("ccci_push_buff Fail(ch=%d):%d\n",buff->channel,ret);
	}

	do{
		unsigned long flags;
		spin_lock_irqsave(&client->wait_q.lock,flags);
		if (list_empty(&client->wait_q.task_list))
		{
			CCCI_DEBUG("client->wait_q is empty. Pid=%d CH:%d\n",client->pid,client->ch_num);
		}	
		
		spin_unlock_irqrestore(&client->wait_q.lock,flags);
	}while(0);
#endif
	
	if(lg_ch_rx_debug_enable & (1ULL<<buff->channel)){
		CCCI_MSG_INF("chr", "ccci_chrdev_callback: %08X, %08X, %02d, %08X\n",
			buff->data[0], buff->data[1], buff->channel, buff->reserved);
	}

	client->wakeup_waitq = 1;
	wake_up_interruptible(&client->wait_q);
	//wake_up_interruptible_poll(&client->wait_q,POLL_IN);
	
	kill_fasync(&client->fasync, SIGIO, POLL_IN);
	return ;
}

static struct ccci_dev_client *find_get_client(int ch,pid_t pid)
{
	struct ccci_dev_client *client=NULL;
	int ret;
	schedule_timeout(10*HZ);
	down(&ccci_dev_sem);

	list_for_each_entry(client,&ccci_dev_list,dev_list)
	{
		if ( client->ch_num==ch )
		{
			atomic_inc(&client->user);
			break ;
#if 0
			if (client->pid==pid) 
			{
				
				break;
			}
			else 
			{
				CCCI_DEBUG("CH %d has been registerd by pid %d,"
					"This process %s can't be registered (don't support now)\n",
					ch,client->pid,current->comm);
				client=ERR_PTR(-EEXIST);
				break;
			}
#endif
		}
	}
	if (&client->dev_list==&ccci_dev_list)
	{
		CCCI_CHR_MSG("Create a Client for CH%d\n",ch);
		client=kmalloc(sizeof(*client),GFP_KERNEL);
		if (client==NULL)
		{
			CCCI_MSG_INF("chr", "kmalloc for create client fail \n");
			client=ERR_PTR(-ENOMEM);
			goto out;
		}
		ret=ccci_register_with_name_size(ch,"CCCI_CHRDEV",CCCI_FIFO_MAX_LEN*2,ccci_chrdev_callback,client);
		if (ret && (ret != -EEXIST) )
		{
			CCCI_MSG_INF("chr", "register ch fail: %d \n",ret);
			kfree(client);
			client=ERR_PTR(ret);
			goto out;
		}
		ccci_client_init(client,ch,pid);
		list_add(&client->dev_list,&ccci_dev_list);
	}
	

out:
	up(&ccci_dev_sem);
	return client;
}


static int ccci_dev_open(struct inode *inode, struct file *file)
{
	int index=iminor(inode);
	int ret=0;
	struct ccci_dev_client *client=find_get_client(index,current->pid);
	CCCI_CHR_MSG("Open by %s ch:%d\n",current->comm,index);
	if (IS_ERR(client))
	{
		CCCI_MSG_INF("chr", "find client fail \n");
		ret=PTR_ERR(client);
		goto out;
	}
	file->private_data=client;	
	nonseekable_open(inode,file);
out:	
	
	return ret;

}

static int ccci_dev_release(struct inode *inode, struct file *file)
{
	struct ccci_dev_client *client=(struct ccci_dev_client *)file->private_data;
	ccci_put_client(client);
	return 0;
}

static int ccci_dev_fasync(int fd, struct file *file, int on)
{
	struct ccci_dev_client *client=(struct ccci_dev_client *)file->private_data;
	return fasync_helper(fd,file,on,&client->fasync);
}

static unsigned int ccci_dev_poll(struct file *file,poll_table *wait)
{
	struct ccci_dev_client *client=(struct ccci_dev_client *)file->private_data;
	int ret=0;
	struct recv_array *array;
	poll_wait(file,&client->wait_q,wait);
	spin_lock_bh(&client->lock);
	array=ccci_get_first(&client->buff_list);
	if ( array && !recv_array_is_empty(array) )
	{
		 ret |= POLLIN|POLLRDNORM;
	}
	spin_unlock_bh(&client->lock);
	return ret;
}

static ssize_t ccci_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	struct ccci_dev_client *client=(struct ccci_dev_client *)file->private_data;
	int i=count/sizeof(CCCI_BUFF_T);
	int j=0;
	int ret=0;
	int ch=client->ch_num;
	CCCI_BUFF_T *buff=kmalloc(i*sizeof(CCCI_BUFF_T),GFP_KERNEL);
	WARN_ON(count%sizeof(CCCI_BUFF_T));
	
	if (buff==NULL)
	{
		CCCI_MSG_INF("chr", "kmalloc for CCCI_BUFF_T fail \n");
		ret=-ENOMEM;
		goto out;
	}
	if (copy_from_user(buff,buf,i*sizeof(CCCI_BUFF_T)))
	{
		CCCI_MSG_INF("chr", "ccci_dev_write: copy from user fail \n");
		ret=-EFAULT;
		goto out_free;
	}

	if(lg_ch_tx_debug_enable & (1ULL<<buff->channel)) {
		CCCI_MSG_INF("chr", "ccci_dev_write: PID: %d, client: %p, lg_ch: %d\n", 
			client->pid, client, buff->channel);
	}
	
	for (j=0;j<i;j++)
	{
		ret=ccci_write(ch,buff+j);
		if (ret)
		{
			CCCI_MSG_INF("chr", "ccci_write fail: %d \n",ret);
			break;
		}
	}	
	if (j)
	{
		ret=sizeof(*buff)*j;
	}
	
out_free:
	kfree(buff);
out:
	return ret;

}

static ssize_t ccci_dev_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	struct ccci_dev_client *client=(struct ccci_dev_client *)file->private_data;
	int ret=0,i=0;
	CCCI_BUFF_T buff,*u_buff=(CCCI_BUFF_T *)buf;
	int value, n;

	
	n = count/sizeof(CCCI_BUFF_T);
	WARN_ON(client==NULL);
	WARN_ON(count%sizeof(CCCI_BUFF_T));
	
retry:
	
	for (;i<n;i++)
	{
		spin_lock_bh(&client->lock);
		ret=ccci_pop_buff(client,&buff);
		spin_unlock_bh(&client->lock);

		if (ret==sizeof(buff))
		{				
			if (copy_to_user(u_buff+i,&buff,sizeof(buff)))
			{
				CCCI_MSG_INF("chr", "read: [%d:%s]copy_to_user fail: %08X, %08X, %02d, %08X\n", 
					client->pid, current->comm, 
					buff.data[0], buff.data[1], buff.channel, buff.reserved);
				ret= -EFAULT;
				break;
			}
			
			if(push_data_fail || (lg_ch_rx_debug_enable & (1ULL<<buff.channel))){
				CCCI_MSG_INF("chr", "read: [%d:%s] %08X, %08X, %02d, %08X\n", 
					client->pid, current->comm, 
					buff.data[0], buff.data[1], buff.channel, buff.reserved);
				push_data_fail = 0;
			}
		}
		else {
			if (file->f_flags & O_NONBLOCK)
			{	
				ret=-EAGAIN;
				goto out;
			}

			if(push_data_fail)
				CCCI_MSG_INF("chr", "wait for event \n");
			//value = wait_event_interruptible(chardev_waitq, wakeup_waitq[client->ch_num]);
			value = wait_event_interruptible(client->wait_q, client->wakeup_waitq);
			if(value == -ERESTARTSYS) {
				CCCI_CHR_MSG("Interrupted syscall.signal_pend=0x%llx\n",
					*(long long *)current->pending.signal.sig);
				ret = -EINTR;
				goto  out;
			}
			else if(value == 0) {
				client->wakeup_waitq = 0;
				if(push_data_fail || (lg_ch_rx_debug_enable & (1ULL<<buff.channel))){
					CCCI_MSG_INF("chr", "exit from schedule(%d):[%d:%s] \n", 
						client->ch_num, current->pid, current->comm);
				}
				goto retry;
		}
	}
	}

out:
	//finish_wait(&client->wait_q,&wait);
	if (i)
	{
		ret=i*sizeof(CCCI_BUFF_T);
	}

//	spin_unlock_bh(&client->lock);
	return ret;
}

extern int ccci_do_modem_reset(void);
extern int ccci_send_run_time_data(void);
extern int is_modem_debug_ver(void);
extern int send_stop_md_request(void);
extern int send_start_md_request(void);
extern int ccci_stop_modem(void);
extern int ccci_start_modem(void);

static long ccci_dev_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{
	int addr, len, ret = 0;
	CCCI_BUFF_T buff;
	struct ccci_dev_client *client=(struct ccci_dev_client *)file->private_data;

	switch (cmd) {
	case CCCI_IOC_MD_RESET:
		CCCI_MSG_INF("chr", "MD reset ioctl called by %s\n", current->comm);
		ret=reset_md();
		break;

	case CCCI_IOC_PCM_BASE_ADDR:
		ccci_pcm_base_req(&addr, &len);
			ret = put_user((unsigned int)addr, (unsigned int __user *)arg);
		break;

	case CCCI_IOC_PCM_LEN:
		ccci_pcm_base_req(&addr, &len);
			ret = put_user((unsigned int)len, (unsigned int __user *)arg);
		break;
		
	case CCCI_IOC_ALLOC_MD_LOG_MEM :
		ccci_mdlog_base_req(&addr, &len);
		ret = addr;
		break;
		
	case CCCI_IOC_FORCE_MD_ASSERT:
		CCCI_MSG_INF("chr", "Force MD assert ioctl called by %s\n", current->comm);
		CCCI_INIT_MAILBOX(&buff, 0);
		ret = ccci_write_force(CCCI_FORCE_RESET_MODEM_CHANNEL, &buff);
		break;

	case CCCI_IOC_DO_MD_RST:
		if(client->ch_num != CCCI_SYSTEM_RX)
			ret = -1;
		else
			ret = ccci_do_modem_reset();
		break;

	case CCCI_IOC_SEND_RUN_TIME_DATA:
		if(client->ch_num != CCCI_SYSTEM_RX)
			ret = -1;
		else
			ret = ccci_send_run_time_data();
		break;

	case CCCI_IOC_GET_MD_INFO:
		if(client->ch_num != CCCI_SYSTEM_RX)
			ret = -1;
		else{
			if(is_modem_debug_ver())
				addr = 1;
			else
				addr = 0;
			ret = put_user((unsigned int)addr, (unsigned int __user *)arg);
		}
		break;

	case CCCI_IOC_GET_MD_EX_TYPE:
		ret = md_ex_type;
		CCCI_MSG_INF("chr", "get modem exception type=%d\n", md_ex_type);
		break;

	case CCCI_IOC_SEND_STOP_MD_REQUEST:
		CCCI_MSG_INF("chr", "stop MD request ioctl called by %s\n", current->comm);
		ret=send_stop_md_request();
		break;

	case CCCI_IOC_SEND_START_MD_REQUEST:
		CCCI_MSG_INF("chr", "start MD request ioctl called by %s\n", current->comm);
		ret=send_start_md_request();
		break;

	case CCCI_IOC_DO_STOP_MD:
		CCCI_MSG_INF("chr", "stop MD ioctl called by %s\n", current->comm);
		ret=ccci_stop_modem();
		break;

	case CCCI_IOC_DO_START_MD:
		CCCI_MSG_INF("chr", "start MD ioctl called by %s\n", current->comm);
		ret=ccci_start_modem();
		break;

	default:
		ret = -ENOTTY;
		break;
	}

	//  CCCI_DEBUG("ret=%d cmd=0x%x addr=%0x len=%d\n",ret,cmd,addr,len);
	return ret;
}

static int ccci_dev_mmap(struct file *file, struct vm_area_struct *vma)
{
    int  pfn, len; 
    unsigned long addr;
    struct ccci_dev_client *client=(struct ccci_dev_client *)file->private_data;
	
    /* only PCM buffer for PCM channels can be mapped */
    if (client->ch_num == CCCI_PCM_RX || client->ch_num == CCCI_PCM_TX) {
        ccci_pcm_base_req(&addr,&len);
    }
    else if (client->ch_num==CCCI_MD_LOG_RX||client->ch_num==CCCI_MD_LOG_TX)
    {
		ccci_mdlog_base_req(&addr,&len);
    }
    
    CCCI_CHR_MSG("remap addr:0x%lx len:%d  map-len=%lu\n",addr,len,vma->vm_end-vma->vm_start);
    len=(vma->vm_end-vma->vm_start)<len?vma->vm_end-vma->vm_start:len;
    pfn = addr;
    pfn >>= PAGE_SHIFT;
    /* ensure that memory does not get swapped to disk */
    vma->vm_flags |= VM_RESERVED;
    /* ensure non-cacheable */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    if (remap_pfn_range(vma, vma->vm_start, pfn,len, vma->vm_page_prot)) {
        return -EAGAIN;
    }

    return 0;
}


static struct file_operations ccci_chrdev_fops=
{
	.owner = THIS_MODULE,
	.open= ccci_dev_open,
	.read=ccci_dev_read,
	.write=ccci_dev_write,
	.release=ccci_dev_release,
	.unlocked_ioctl=ccci_dev_ioctl,
	.fasync=ccci_dev_fasync,
	.poll=ccci_dev_poll,
	.mmap=ccci_dev_mmap,

};

int __init ccci_chrdev_init(void)
{
	int ret=0;
	if (register_chrdev_region(MKDEV(CCCI_DEV_MAJOR,0),CCCI_MAX_CHANNEL,CCCI_DEV_NAME) != 0)
	{
		CCCI_MSG_INF("chr", "Regsiter CCCI_CHRDEV failed!\n");
		ret=-1;
		goto out;
	}
	ccci_chrdev=cdev_alloc();
	if (ccci_chrdev==NULL)
	{
		CCCI_MSG_INF("chr", "cdev_alloc failed\n");
		ret=-1;
		goto out_err1;
	}
	cdev_init(ccci_chrdev,&ccci_chrdev_fops);
	ccci_chrdev->owner=THIS_MODULE;
	ret=cdev_add(ccci_chrdev,MKDEV(CCCI_DEV_MAJOR,0),CCCI_MAX_CHANNEL);
	if (ret)
	{
		CCCI_MSG_INF("chr", "cdev_add failed\n");
		goto out_err0;
	}
	goto out;
out_err0:
	cdev_del(ccci_chrdev);
out_err1:
	unregister_chrdev_region(MKDEV(CCCI_DEV_MAJOR,0),CCCI_MAX_CHANNEL);
out:
	return ret;
}

void __exit ccci_chrdev_exit(void)
{
	unregister_chrdev_region(MKDEV(CCCI_DEV_MAJOR,0),CCCI_MAX_CHANNEL);
	cdev_del(ccci_chrdev);
}
