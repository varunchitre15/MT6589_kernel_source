/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccci_fs.c
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   MT65XX CCCI FS Proxy Driver
 *
 ****************************************************************************/

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>

#include "ccci.h"
#include "ccci_fs.h"



#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
static spinlock_t          fs_spinlock = SPIN_LOCK_UNLOCKED;
#else
static DEFINE_SPINLOCK(fs_spinlock);
#endif

static dev_t               fs_dev_num;
static struct cdev         fs_cdev;
static fs_stream_buffer_t *fs_buffers;
static int                 fs_buffers_phys_addr;
static struct kfifo        fs_fifo;
//static struct class       *fs_class;
//static struct device      *fs_dev;
static int                 reset_handle;
static struct wake_lock	fs_wake_lock;
DECLARE_WAIT_QUEUE_HEAD(fs_waitq);

#define CCCI_FS_DEVNAME  "ccci_fs"
#define CCCI_FS_MAJOR    178

extern unsigned long long lg_ch_tx_debug_enable;
extern unsigned long long lg_ch_rx_debug_enable;

//enable fs_tx or fs_rx log
unsigned int fs_tx_debug_enable = 0UL; 
unsigned int fs_rx_debug_enable = 0UL; 



#if 0
//#define CCCI_FS_DEBUG_ON
#ifdef CCCI_FS_DEBUG_ON
#define CF_DEBUG(format, ...) printk(format, __VA_ARGS__)
#define CF_DEBUG_FUNC printk("%s\n",__FUNCTION__)
#define CF_INFO(format, ...) printk(format, __VA_ARGS__)
#else
#define CF_DEBUG(format, ...)
#define CF_DEBUG_FUNC
#define CF_INFO(format, ...)
#endif

#endif


//  will be called when modem sends us something.
//  we will then copy it to the tty's buffer.
//  this is essentially the "read" fops.
static void ccci_fs_callback(CCCI_BUFF_T *buff, void *private_data)
{
    unsigned long flag;

    spin_lock_irqsave(&fs_spinlock,flag);
    if (buff->channel == CCCI_FS_RX)
    {
        if(fs_rx_debug_enable)
    	{
	     CCCI_MSG_INF("fs ", "fs_callback: %08X  %08X  %08X\n", 
	     buff->data[0], buff->data[1], buff->reserved);
	}		
        if (kfifo_in(&fs_fifo, (unsigned char *) &buff->reserved, sizeof(buff->reserved)) == sizeof(buff->reserved))
        {
            wake_up_interruptible(&fs_waitq);
        }
        else
        {
            CCCI_MSG_INF("fs ", "Unable to put new request into fifo\n");    
        }
    }
    spin_unlock_irqrestore(&fs_spinlock,flag);
    wake_lock_timeout(&fs_wake_lock, HZ/2);
}


static int ccci_fs_get_index(void)
{
    int ret;
    unsigned long flag;
    
    CCCI_FS_MSG("get_fs_index++\n");
    
    if (wait_event_interruptible(fs_waitq, kfifo_len(&fs_fifo) != 0) != 0)
    {
    	if(fs_rx_debug_enable)
    	    CCCI_MSG_INF("fs ", "fs_get_index: Interrupted by syscall.signal_pend\n");
        return -ERESTARTSYS;
    }
	
    spin_lock_irqsave(&fs_spinlock,flag);
    if (kfifo_out(&fs_fifo, (unsigned char *) &ret, sizeof(int)) != sizeof(int))
    {
        CCCI_MSG_INF("fs ", "get fs index fail from fifo\n");
	spin_unlock_irqrestore(&fs_spinlock,flag);
        return -EFAULT;
    }
    
    spin_unlock_irqrestore(&fs_spinlock,flag);

	if(fs_rx_debug_enable)
		CCCI_MSG_INF("fs ", "fs_index=%d \n", ret);
	
    CCCI_FS_MSG("get_fs_index--\n");
    return ret;
}


static int ccci_fs_send(unsigned long arg)
{
	void __user      *argp;
	CCCI_BUFF_T       stream;
	fs_stream_msg_t   message;
	int ret = 0;
	int xmit_retry = 0;

	CCCI_FS_MSG("ccci_fs_send++\n");
    
	argp = (void __user *) arg;
	if (copy_from_user((void *) &message, argp, sizeof(fs_stream_msg_t)))
	{
		CCCI_MSG_INF("fs ", "ccci_fs_send: copy_from_user fail!\n");
		return -EFAULT;
	}

	stream.data[0]  = fs_buffers_phys_addr + (sizeof(fs_stream_buffer_t) * message.index);
	stream.data[1]  = message.length + 4;
	stream.reserved = message.index;

	if(fs_tx_debug_enable) {
		CCCI_MSG_INF("fs ", "fs_send: %08X %08X %08X\n", 
			stream.data[0], stream.data[1], stream.reserved);
	}

	do{
		ret = ccci_write(CCCI_FS_TX, &stream);
		if(ret == CCCI_SUCCESS)
			break;
		
		if(ret == CCCI_NO_PHY_CHANNEL){
			xmit_retry++;
			msleep(10);
			if( (xmit_retry%10) == 0){
				CCCI_MSG_INF("fs ", "fs_chr has retried %d times\n", xmit_retry);
			}
		}else{
			break;
		}
	}while(1);
	if(ret != CCCI_SUCCESS)
	{
		CCCI_MSG_INF("fs ", "ccci_fs_send fail <ret=%d>: %08X, %08X, %08X\n", 
			ret, stream.data[0], stream.data[1], stream.reserved);
		return ret;
	}

	CCCI_FS_MSG("ccci_fs_send--\n");
    
	return 0;
}


static int ccci_fs_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long off, start, len;

    CCCI_FS_MSG("mmap++\n");
    
    if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
    {
    	CCCI_MSG_INF("fs ", "ccci_fs_mmap: vm_pgoff too large\n");
        return -EINVAL;
    }

    off   = vma->vm_pgoff << PAGE_SHIFT;
    start = (unsigned long) fs_buffers_phys_addr;
    len   = PAGE_ALIGN((start & ~PAGE_MASK) + CCCI_FS_SMEM_SIZE);

    if ((vma->vm_end - vma->vm_start + off) > len)
    {
    	CCCI_MSG_INF("fs ", "ccci_fs_mmap: memory require over ccci_fs_smem size\n");
        return -EINVAL;
    }

    off += start & PAGE_MASK;
    vma->vm_pgoff  = off >> PAGE_SHIFT;
    vma->vm_flags |= VM_RESERVED;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    
    CCCI_FS_MSG("mmap--\n");
    
    return remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot);
}


static long ccci_fs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret;

    switch(cmd)
    {
        case CCCI_FS_IOCTL_GET_INDEX:
            ret = ccci_fs_get_index();
            break;

        case CCCI_FS_IOCTL_SEND:
            ret = ccci_fs_send(arg);
            break;

        default:
			CCCI_MSG_INF("fs ", "ccci_fs_ioctl: unknown ioctl:%d\n", cmd);
            ret = -ENOIOCTLCMD;
            break;
    }
    
    return ret;
}


static int ccci_fs_open(struct inode *inode, struct file *file)
{
    unsigned long flag;

    CCCI_MSG_INF("fs ", "file open by %s\n", current->comm);
    // modem reset registration.
    reset_handle = ccci_reset_register("CCCI_FS");
    ASSERT(reset_handle >= 0);

    // clear kfifo invalid data which may not be processed before last close operation
    spin_lock_irqsave(&fs_spinlock,flag);
    kfifo_reset(&fs_fifo);
    spin_unlock_irqrestore(&fs_spinlock,flag);
    return 0;
}


static int ccci_fs_release(struct inode *inode, struct file *file)
{
    CCCI_MSG_INF("fs ", "file release\n");
    memset(fs_buffers, 0, CCCI_FS_SMEM_SIZE);
    ccci_reset_request(reset_handle);
    
    return 0;
}


static int ccci_fs_start(void)
{
    int size;

    if ( 0 != kfifo_alloc(&fs_fifo,sizeof(unsigned) * CCCI_FS_MAX_BUFFERS, GFP_KERNEL))
    {
        CCCI_MSG_INF("fs ", "ccci_fs_start: kfifo alloc fail \n");
        return -ENOMEM;
    }	
   

    // modem related channel registration.
    ASSERT(ccci_fs_setup((int*)&fs_buffers, &fs_buffers_phys_addr, &size) == CCCI_SUCCESS);
    CCCI_MSG_INF("fs ", "%x:%x:%d\n", (unsigned int)fs_buffers , (unsigned int)fs_buffers_phys_addr, size);
    ASSERT(ccci_register(CCCI_FS_RX, ccci_fs_callback, NULL) == CCCI_SUCCESS);
    ASSERT(ccci_register(CCCI_FS_TX, ccci_fs_callback, NULL) == CCCI_SUCCESS);

    #if 0
    // modem reset registration.
    reset_handle = ccci_reset_register("CCCI_FS");
    ASSERT(reset_handle >= 0);
    #endif

    return 0;
}


static void ccci_fs_stop(void)
{
    if (fs_buffers != NULL)
    {
        kfifo_free(&fs_fifo);
        
        ASSERT(ccci_unregister(CCCI_FS_RX) == CCCI_SUCCESS);
        ASSERT(ccci_unregister(CCCI_FS_TX) == CCCI_SUCCESS);

        fs_buffers = NULL;
	fs_buffers_phys_addr = 0;
    }
}


static struct file_operations fs_fops = 
{
    .owner   = THIS_MODULE,
    .unlocked_ioctl = ccci_fs_ioctl,
    .open    = ccci_fs_open,
    .mmap    = ccci_fs_mmap,
    .release = ccci_fs_release,
};


 int __init ccci_fs_init(void)
{
    int  ret;

    fs_dev_num = MKDEV(CCCI_FS_MAJOR, 0);
    ret        = register_chrdev_region(fs_dev_num, 1, CCCI_FS_DEVNAME);
    
    if (ret)
    {
        CCCI_MSG_INF("fs ", "ccci_fs_init: Register char device failed: dev_num=%d\n", MAJOR(fs_dev_num));
        return ret;
    }
	
    cdev_init(&fs_cdev, &fs_fops);
    fs_cdev.owner = THIS_MODULE;
    fs_cdev.ops   = &fs_fops;
	
    ret = cdev_add(&fs_cdev, fs_dev_num, 1);
    if (ret)
    {
        CCCI_MSG_INF("fs ", "cdev_add fail: dev_num=%d\n", MAJOR(fs_dev_num));
        unregister_chrdev_region(fs_dev_num, 1);        
        return ret;
    }
    
    ret = ccci_fs_start();
    if (ret)
    {
        CCCI_MSG_INF("fs ", "FS initialize fail\n");
        return ret;
    }
    wake_lock_init(&fs_wake_lock, WAKE_LOCK_SUSPEND, "ccci_fs_main"); 
    CCCI_FS_MSG("Init complete, device major number = %d\n", MAJOR(fs_dev_num));
    
    return 0;
}


 void __exit ccci_fs_exit(void)
{
    ccci_fs_stop();

    cdev_del(&fs_cdev);
    unregister_chrdev_region(fs_dev_num, 1);
    wake_lock_destroy(&fs_wake_lock);
}



