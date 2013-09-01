

/******************************************************************************
 *  INCLUDE LINUX HEADER
 ******************************************************************************/
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/slab.h> 
#include <linux/unistd.h> 
#include <linux/sched.h> 
#include <linux/fs.h> 
#include <asm/uaccess.h> 
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <mach/memory.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>

/******************************************************************************
 *  INCLUDE LIBRARY
 ******************************************************************************/
#include <mach/sec_osal.h>
#include "sec_mod.h"

#define SEC_DEV_NAME                "sec"
#define SEC_MAJOR                   182
#define MOD                         "ASP"

#define TRACE_FUNC()                MSG_FUNC(SEC_DEV_NAME)

/**************************************************************************
 *  EXTERNAL VARIABLE
 **************************************************************************/
extern const struct sec_ops         *sec_get_ops(void);
extern bool                         bMsg;
extern struct semaphore             hacc_sem;

/*************************************************************************
 *  GLOBAL VARIABLE
 **************************************************************************/
static struct sec_mod sec           = {0};
static struct cdev                  sec_dev;

#if 0
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
    DECLARE_MUTEX(hacc_sem);           
    DECLARE_MUTEX(mtd_sem);           
    DECLARE_MUTEX(rid_sem);             
    DECLARE_MUTEX(sec_mm_sem);         
    DECLARE_MUTEX(osal_fp_sem);                      
#else // (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))        
    DEFINE_SEMAPHORE(hacc_sem);
    DEFINE_SEMAPHORE(mtd_sem);
    DEFINE_SEMAPHORE(rid_sem);    
    DEFINE_SEMAPHORE(sec_mm_sem);   
    DEFINE_SEMAPHORE(osal_fp_sem);            
#endif
EXPORT_SYMBOL(hacc_sem);
EXPORT_SYMBOL(mtd_sem);
EXPORT_SYMBOL(rid_sem);
EXPORT_SYMBOL(sec_mm_sem);   
EXPORT_SYMBOL(osal_fp_sem);   
#endif

/**************************************************************************
 *  EXTERNAL FUNCTION
 **************************************************************************/
extern int sec_get_random_id(unsigned int *rid);
extern long sec_core_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
extern void sec_core_init (void);
extern void sec_core_exit (void);

/**************************************************************************
 *  SEC DRIVER OPEN
 **************************************************************************/ 
static int sec_open(struct inode *inode, struct file *file)
{
    return 0;
}

/**************************************************************************
 *  SEC DRIVER RELEASE
 **************************************************************************/ 
static int sec_release(struct inode *inode, struct file *file)
{
    return 0;
}

/**************************************************************************
 *  SEC DRIVER IOCTL
 **************************************************************************/ 
static long sec_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return sec_core_ioctl(file, cmd, arg);
}

static struct file_operations sec_fops = {
    .owner   = THIS_MODULE,
    .open    = sec_open,
    .release = sec_release,
    .write   = NULL,
    .read    = NULL,
    .unlocked_ioctl   = sec_ioctl
};

/**************************************************************************
 *  SEC RID PROC FUNCTION
 **************************************************************************/ 
static int sec_proc_rid(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    char *p = page;
    int len = 16; 

    if ((off + len) > PAGE_SIZE)
        return -ENOBUFS;

    if (off > 0)
        return 0;

    if (sec_get_random_id((unsigned int*)(p + off)) != 0)
        return 0;
    
    *eof = 1;
    
    return len;
}

/**************************************************************************
 *  SEC DRIVER INIT
 **************************************************************************/ 
static int sec_init(void)
{
    int ret = 0;
    dev_t id;    

    id = MKDEV(SEC_MAJOR, 0);    
    ret = register_chrdev_region(id, 1, SEC_DEV_NAME);

    if (ret) 
    {
        printk(KERN_ERR "[%s] Regist Failed (%d)\n", SEC_DEV_NAME, ret);
        return ret;
    }

    cdev_init(&sec_dev, &sec_fops);
    sec_dev.owner = THIS_MODULE;

    ret = cdev_add(&sec_dev, id, 1);
    if (ret < 0)
    {
        goto exit;
    }

    sec.id   = id;
    sec.init = 1;
    spin_lock_init(&sec.lock);

    create_proc_read_entry("rid", S_IRUGO, NULL, sec_proc_rid, NULL);

    sec_core_init();

exit:
    if (ret != 0) 
    {
        unregister_chrdev_region(id, 1);
        memset(&sec, 0, sizeof(sec));
    }

    return ret;
}


/**************************************************************************
 *  SEC DRIVER EXIT
 **************************************************************************/ 
static void sec_exit(void)
{    
    remove_proc_entry("rid", NULL);
    cdev_del(&sec_dev);
    unregister_chrdev_region(sec.id, 1);
    memset(&sec, 0, sizeof(sec));
    sec_core_exit();
}

module_init(sec_init);
module_exit(sec_exit);

/**************************************************************************
 *  EXPORT FUNCTION
 **************************************************************************/ 
EXPORT_SYMBOL(sec_get_random_id);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek Inc.");
MODULE_DESCRIPTION("Mediatek Security Module");
