

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/xlog.h>

#include "devinfo.h"

#define DEVINFO_TAG "DEVINFO"


/***************************************************************************** 
* FUNCTION DEFINITION 
*****************************************************************************/
static struct cdev devinfo_cdev;
static dev_t devinfo_dev;
static int dev_open(struct inode *inode, struct file *filp);
static int dev_release(struct inode *inode, struct file *filp);
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations devinfo_fops = {        
    .open = dev_open,        
    .release = dev_release,        
    .unlocked_ioctl   = dev_ioctl,
    .owner = THIS_MODULE,
    };


static int dev_open(struct inode *inode, struct file *filp){
    return 0;
}

static int dev_release(struct inode *inode, struct file *filp){        
    return 0;
}

/************************************************************************** 
*  DEV DRIVER IOCTL 
**************************************************************************/ 
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    u32 index = 0;
    int err   = 0;
    int ret   = 0;
    u32 data_size = g_devinfo_data_size;
    u32 data_read = 0;

    /* ---------------------------------- */
    /* IOCTL                              */
    /* ---------------------------------- */
    if (_IOC_TYPE(cmd) != DEV_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > DEV_IOC_MAXNR)
        return -ENOTTY;
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;
    
    switch (cmd) {

        /* ---------------------------------- */
        /* get dev info data                  */
        /* ---------------------------------- */
        case READ_DEV_DATA:
            //xlog_printk(ANDROID_LOG_INFO, DEVINFO_TAG ,"%s CMD - READ_DEV_DATA\n",MODULE_NAME);
            if(copy_from_user((void *)&index, (void __user *)arg, sizeof(u32)))
            {              
                return -EFAULT;            
            }
            //xlog_printk(ANDROID_LOG_INFO, DEVINFO_TAG ,"%s READ_DEV_DATA IDX:%d\n",MODULE_NAME, index);

            if (index < data_size){
                data_read = get_devinfo_with_index(index);
                ret = copy_to_user((void __user *)arg, (void *)&(data_read), sizeof(u32));
            }else{
                xlog_printk(ANDROID_LOG_ERROR, DEVINFO_TAG ,"%s Error! Data index larger than data size. index:%d, size:%d\n",MODULE_NAME, 
                    index, data_size);
                return -EFAULT;            
            }
        break;       
    }
    
    return 0;
}




/******************************************************************************
 * devinfo_init
 * 
 * DESCRIPTION: 
 *   Init the device driver ! 
 * 
 * PARAMETERS: 
 *   None
 * 
 * RETURNS: 
 *   0 for success
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
static int __init devinfo_init(void)
{
    int ret = 0;
    devinfo_dev = MKDEV(MAJOR_DEV_NUM, 0);

	xlog_printk(ANDROID_LOG_INFO, DEVINFO_TAG ," init\n");    

	ret = register_chrdev_region(devinfo_dev, 1, DEV_NAME );
	if (ret)    
	{        
	    xlog_printk(ANDROID_LOG_ERROR, DEVINFO_TAG ,"[%s] register device failed, ret:%d\n", MODULE_NAME, ret);
	    return ret;
    }

	/* initialize the device structure and register the device  */
    cdev_init(&devinfo_cdev, &devinfo_fops);
    devinfo_cdev.owner = THIS_MODULE;
    if ((ret = cdev_add(&devinfo_cdev, devinfo_dev  , 1)) < 0) 
    {
        xlog_printk(ANDROID_LOG_ERROR, DEVINFO_TAG ,"[%s] could not allocate chrdev for the device, ret:%d\n", MODULE_NAME, ret);
        return ret;
    }
		
	return 0;
}

/******************************************************************************
 * devinfo_exit
 * 
 * DESCRIPTION: 
 *   Free the device driver ! 
 * 
 * PARAMETERS: 
 *   None
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
static void __exit devinfo_exit(void)
{
	cdev_del(&devinfo_cdev);        
	unregister_chrdev_region(devinfo_dev, 1);
}

module_init(devinfo_init);
module_exit(devinfo_exit);
MODULE_LICENSE("GPL");

