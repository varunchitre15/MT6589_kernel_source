#if defined(MTK_BT_SUPPORT) || defined(MTK_WLAN_SUPPORT)

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <cust_eint.h>


#define BTWLAN_EM_DEBUG_ENANLE

#define BTWLAN_EM_ALERT(f, s...) \
    do { \
        printk(KERN_ALERT "BTEM " f, ## s); \
    } while(0)

#ifdef BTWLAN_EM_DEBUG_ENANLE
#define BTWLAN_EM_DEBUG(f, s...) \
    do { \
        printk(KERN_ALERT "BTEM " f, ## s); \
    } while(0)
#else
#define BTWLAN_EM_DEBUG(f, s...)
#endif

/****************************************************************************** 
 * CONSTANT DEFINITIONS
******************************************************************************/
#define BTWLANEM_NAME                "btwlan_em"
#define BTWLANEM_DEVNAME             "/dev/btwlan_em"
#define BTWLAN_EM_IOC_MAGIC          0xf6
#define BTWLAN_EM_IOCTL_SET_BTPWR    _IOWR(BTWLAN_EM_IOC_MAGIC, 0, uint32_t)
#define BTWLAN_EM_IOCTL_SET_WIFIPWR  _IOWR(BTWLAN_EM_IOC_MAGIC, 1, uint32_t)
#define BT_IOCTL_SET_EINT            _IOWR(BTWLAN_EM_IOC_MAGIC, 2, uint32_t)


extern void mt_bt_power_on(void);
extern void mt_bt_power_off(void);
extern void mt_wifi_power_on(void);
extern void mt_wifi_power_off(void);

#ifdef MT6516
extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void MT6516_EINTIRQMask(unsigned int line);
#define mt65xx_eint_mask(line) MT6516_EINTIRQMask(line)
#define mt65xx_eint_unmask(line) MT6516_EINTIRQUnmask(line)

#else 
extern void mt65xx_eint_mask(unsigned int eint_num);
extern void mt65xx_eint_unmask(unsigned int eint_num);
#endif


wait_queue_head_t eint_wait;
int eint_gen;
int eint_mask;

/******************************************************************************
 * STRUCTURE DEFINITIONS
 *****************************************************************************/
struct btwlan_em {
    bool powerup;
    dev_t dev_t;
    struct class *cls;
    struct device *dev;
    struct cdev cdev;
    struct mutex sem;
};

static struct btwlan_em *pbtwlan_em = NULL;

/*****************************************************************************
* btwaln_em_open
*****************************************************************************/
static int btwlan_em_open(struct inode *inode, struct file *file)
{
    BTWLAN_EM_DEBUG("btwlan_em_open\n");
    eint_gen = 0;
    eint_mask = 0;
    return 0;
}

/*****************************************************************************
* btwaln_em_ioctl
*****************************************************************************/
static int btwlan_em_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    BTWLAN_EM_DEBUG("btwlan_em_ioctl ++\n");
    
    if(!pbtwlan_em)
    {
        BTWLAN_EM_ALERT("btwlan_em_ioctl failed get valid struct\n");
        return -EFAULT;
    }
    
    switch(cmd)
    {
        case BTWLAN_EM_IOCTL_SET_BTPWR:
        {
            unsigned long btpwr = 0;
            if (copy_from_user(&btpwr, (void*)arg, sizeof(unsigned long)))
                return -EFAULT;
                
            BTWLAN_EM_DEBUG("BTWLAN_EM_IOCTL_SET_BTPWR:%d\n", (int)btpwr);
            
            mutex_lock(&pbtwlan_em->sem);
            if (btpwr){
                mt_bt_power_on();
            }
            else{
                mt_bt_power_off();
            }
            mutex_unlock(&pbtwlan_em->sem);
            
            break;
        }
        case BTWLAN_EM_IOCTL_SET_WIFIPWR:
        {
            unsigned long wifipwr = 0;
            if (copy_from_user(&wifipwr, (void*)arg, sizeof(unsigned long)))
                return -EFAULT;
                
            BTWLAN_EM_DEBUG("BTWLAN_EM_IOCTL_SET_WIFIPWR:%d\n", (int)wifipwr);
            
            mutex_lock(&pbtwlan_em->sem);
            if (wifipwr){
                mt_wifi_power_on();
            }
            else{
                mt_wifi_power_off();
            }
            mutex_unlock(&pbtwlan_em->sem);
            
            break;
        }
        case BT_IOCTL_SET_EINT:
        {
            unsigned long bt_eint = 0;
            if (copy_from_user(&bt_eint, (void*)arg, sizeof(unsigned long)))
                return -EFAULT;
                
            BTWLAN_EM_DEBUG("BT_IOCTL_SET_EINT:%d\n", bt_eint);
            
            mutex_lock(&pbtwlan_em->sem);
            if (bt_eint){
                mt65xx_eint_unmask(CUST_EINT_BT_NUM);
                BTWLAN_EM_DEBUG("Set enable BT EINT\n");
            }
            else{
                mt65xx_eint_mask(CUST_EINT_BT_NUM);
                BTWLAN_EM_DEBUG("Set disable BT EINT\n");
                eint_mask = 1;
                wake_up_interruptible(&eint_wait);
            }
            mutex_unlock(&pbtwlan_em->sem);
            break;
        }
        
        default:
            BTWLAN_EM_ALERT("btwlan_em_ioctl not support\n");
            return -EPERM;
    }
    
    BTWLAN_EM_DEBUG("btwlan_em_ioctl --\n");
    return 0;
}

/*****************************************************************************
* btwaln_em_release
*****************************************************************************/
static int btwlan_em_release(struct inode *inode, struct file *file)
{
    BTWLAN_EM_DEBUG("btwlan_em_release\n");
    eint_gen = 0;
    eint_mask = 0;
    return 0;
}

static unsigned int btwlan_em_poll(struct file *file, poll_table *wait)
{
    uint32_t mask = 0;
    
    BTWLAN_EM_DEBUG("btwlan_em_poll:%d %d ++\n", eint_gen, eint_mask);
    //poll_wait(file, &eint_wait, wait);
    wait_event_interruptible(eint_wait, (eint_gen == 1 || eint_mask == 1));
    BTWLAN_EM_DEBUG("btwlan_em_poll:%d %d --\n", eint_gen, eint_mask);
    if(eint_gen == 1){
        mask = POLLIN|POLLRDNORM;
        eint_gen = 0;
    }
    else if (eint_mask == 1){
        mask = POLLERR;
        eint_mask = 0;
    }
    
    return mask;
}

/*****************************************************************************
* Kernel interface
*****************************************************************************/
static struct file_operations btwlan_em_fops = {
    .owner      = THIS_MODULE,
//    .ioctl      = btwlan_em_ioctl,
    .unlocked_ioctl = btwlan_em_ioctl,
    .open       = btwlan_em_open,
    .release    = btwlan_em_release,
    .poll       = btwlan_em_poll,
};

/*****************************************************************************/
static int __init btwlan_em_init(void)
{
    int ret = -1, err = -1;
    
    BTWLAN_EM_DEBUG("btwlan_em_init ++\n");
    if (!(pbtwlan_em = kzalloc(sizeof(struct btwlan_em), GFP_KERNEL)))
    {
        BTWLAN_EM_ALERT("btwlan_em_init allocate dev struct failed\n");
        err = -ENOMEM;
        goto ERR_EXIT;
    }
    
    ret = alloc_chrdev_region(&pbtwlan_em->dev_t, 0, 1, BTWLANEM_NAME);
    if (ret) {
        BTWLAN_EM_ALERT("alloc dev_t failed\n");
        goto ERR_EXIT;
    }
    
    BTWLAN_EM_DEBUG("alloc %s:%d:%d\n", BTWLANEM_NAME, MAJOR(pbtwlan_em->dev_t), MINOR(pbtwlan_em->dev_t));
    
    cdev_init(&pbtwlan_em->cdev, &btwlan_em_fops);
    
    pbtwlan_em->cdev.owner = THIS_MODULE;
    pbtwlan_em->cdev.ops = &btwlan_em_fops;
    
    err = cdev_add(&pbtwlan_em->cdev, pbtwlan_em->dev_t, 1);
    if (err) {
        BTWLAN_EM_ALERT("alloc dev_t failed\n");
        goto ERR_EXIT;
    }
    
    pbtwlan_em->cls = class_create(THIS_MODULE, BTWLANEM_NAME);
    if (IS_ERR(pbtwlan_em->cls)) {
        err = PTR_ERR(pbtwlan_em->cls);
        BTWLAN_EM_ALERT("class_create err:%d\n", err);
        goto ERR_EXIT;
    }
    pbtwlan_em->dev = device_create(pbtwlan_em->cls, NULL, pbtwlan_em->dev_t, NULL, BTWLANEM_NAME);
    mutex_init(&pbtwlan_em->sem);
    
    init_waitqueue_head(&eint_wait);
    
    BTWLAN_EM_DEBUG("btwlan_em_init:%d --\n", ret);
    return 0;
    
ERR_EXIT:
    if (err == 0)
        cdev_del(&pbtwlan_em->cdev);
    if (ret == 0)
        unregister_chrdev_region(pbtwlan_em->dev_t, 1);
        
    if (pbtwlan_em){
        kfree(pbtwlan_em);
        pbtwlan_em = NULL;
    }     
    return -1;
}

EXPORT_SYMBOL(eint_wait);
EXPORT_SYMBOL(eint_gen);
/*****************************************************************************/
static void __exit btwlan_em_exit(void)
{
    BTWLAN_EM_DEBUG("btwlan_em_exit ++\n");
    
    if (pbtwlan_em){
        cdev_del(&pbtwlan_em->cdev);
        
        unregister_chrdev_region(pbtwlan_em->dev_t, 1);
        device_destroy(pbtwlan_em->cls, pbtwlan_em->dev_t);
        
        class_destroy(pbtwlan_em->cls);
        mutex_destroy(&pbtwlan_em->sem);
        
        kfree(pbtwlan_em);
        pbtwlan_em = NULL;
        
        BTWLAN_EM_DEBUG("btwlan_em_exit release source\n");
    }
    
    BTWLAN_EM_DEBUG("btwlan_em_exit --\n");
}

module_init(btwlan_em_init);
module_exit(btwlan_em_exit);

MODULE_AUTHOR("Chunhui <Chunhui.li@mediatek.com>");
MODULE_DESCRIPTION("Bluetooth and Wifi engineer mode power control driver");
MODULE_LICENSE("GPL");

#endif
