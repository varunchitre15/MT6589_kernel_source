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
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_hw.h"

//<2013/1/18-20610-jessicatseng, [Pelican] Integrate flashlight IC LM3642 for PRE-MP SW
#include <linux/ctype.h>
#include <linux/i2c.h>
#include <mach/upmu_common_sw.h>
//>2013/1/18-20610-jessicatseng

#define USE_UNLOCKED_IOCTL

//<2013/1/18-20610-jessicatseng, [Pelican] Integrate flashlight IC LM3642 for PRE-MP SW
#if defined(FLASHLIGHT_IC_LM3642)
struct i2c_client *flashlight_i2c_client = NULL;
#endif
//>2013/1/18-20610-jessicatseng

//s_add new flashlight driver here
//export funtions
MUINT32 defaultFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc);
MUINT32 dummyFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc);
MUINT32 peakFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc);
MUINT32 torchFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc);
MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc);


KD_FLASHLIGHT_INIT_FUNCTION_STRUCT kdFlashlightList[] =
{
    {KD_DEFAULT_FLASHLIGHT_ID, defaultFlashlightInit},
#if defined(DUMMY_FLASHLIGHT)
	{KD_DUMMY_FLASHLIGHT_ID, dummyFlashlightInit},
#endif
#if defined(PEAK_FLASHLIGHT)
	{KD_PEAK_FLASHLIGHT_ID, peakFlashlightInit},
#endif
#if defined(TORCH_FLASHLIGHT)
	{KD_TORCH_FLASHLIGHT_ID, torchFlashlightInit},
#endif
#if defined(CONSTANT_FLASHLIGHT)
	{KD_CONSTANT_FLASHLIGHT_ID, constantFlashlightInit},
#endif


/*  ADD flashlight driver before this line */
    {0,NULL}, //end of list
};
//e_add new flashlight driver here
/******************************************************************************
 * Definition
******************************************************************************/
#ifndef TRUE
#define TRUE KAL_TRUE
#endif
#ifndef FALSE
#define FALSE KAL_FALSE
#endif

/* device name and major number */
#define FLASHLIGHT_DEVNAME            "kd_camera_flashlight"

#define DELAY_MS(ms) {mdelay(ms);}//unit: ms(10^-3)
#define DELAY_US(us) {mdelay(us);}//unit: us(10^-6)
#define DELAY_NS(ns) {mdelay(ns);}//unit: ns(10^-9)
/*
    non-busy dealy(/kernel/timer.c)(CANNOT be used in interrupt context):
        ssleep(sec)
        msleep(msec)
        msleep_interruptible(msec)

    kernel timer


*/
/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[KD_CAMERA_FLASHLIGHT]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    printk(KERN_INFO PFX "%s: " fmt, __FUNCTION__ ,##arg)

#define PK_WARN(fmt, arg...)        printk(KERN_WARNING PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_NOTICE(fmt, arg...)      printk(KERN_NOTICE PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_INFO(fmt, arg...)        printk(KERN_INFO PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_TRC_FUNC(f)              printk(PFX "<%s>\n", __FUNCTION__);
#define PK_TRC_VERBOSE(fmt, arg...) printk(PFX fmt, ##arg)

#define DEBUG_KD_CAMERA_FLASHLIGHT
#ifdef DEBUG_KD_CAMERA_FLASHLIGHT
#define PK_DBG PK_DBG_FUNC
#define PK_TRC PK_DBG_NONE //PK_TRC_FUNC
#define PK_VER PK_DBG_NONE //PK_TRC_VERBOSE
#define PK_ERR(fmt, arg...)         printk(KERN_ERR PFX "%s: " fmt, __FUNCTION__ ,##arg)
#else
#define PK_DBG(a,...)
#define PK_TRC(a,...)
#define PK_VER(a,...)
#define PK_ERR(a,...)
#endif
/*****************************************************************************

*****************************************************************************/
static FLASHLIGHT_FUNCTION_STRUCT *g_pFlashlightFunc = NULL;
/*****************************************************************************

*****************************************************************************/
MINT32 default_flashlight_open(void *pArg) {
    PK_DBG("[default_flashlight_open] E\n");
    return 0;
}
MINT32 default_flashlight_release(void *pArg) {
    PK_DBG("[default_flashlight_release] E\n");
    return 0;
}
MINT32 default_flashlight_ioctl(MUINT32 cmd, MUINT32 arg) {
    int i4RetValue = 0;
    int iFlashType = (int)FLASHLIGHT_NONE;

    switch(cmd)
    {
        case FLASHLIGHTIOC_G_FLASHTYPE:
            iFlashType = FLASHLIGHT_NONE;
            if(copy_to_user((void __user *) arg , (void*)&iFlashType , _IOC_SIZE(cmd)))
            {
                PK_DBG("[default_flashlight_ioctl] ioctl copy to user failed\n");
                return -EFAULT;
            }
            break;
    	default :
    		PK_DBG("[default_flashlight_ioctl]\n");
    		break;
    }
    return i4RetValue;
}

FLASHLIGHT_FUNCTION_STRUCT	defaultFlashlightFunc=
{
	default_flashlight_open,
	default_flashlight_release,
	default_flashlight_ioctl,
};

UINT32 defaultFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc) { 
    if (pfFunc!=NULL) {
        *pfFunc=&defaultFlashlightFunc;
    }
    return 0;
}
/*******************************************************************************
* kdSetDriver
********************************************************************************/
int kdSetFlashlightDrv(unsigned int *pFlashlightIdx)
{
unsigned int flashlightIdx = *pFlashlightIdx;
    PK_DBG("[kdSetFlashlightDrv] flashlightIdx: %d \n",flashlightIdx);
    
    if (NULL != kdFlashlightList[flashlightIdx].flashlightInit) {
        kdFlashlightList[flashlightIdx].flashlightInit(&g_pFlashlightFunc);
        if (NULL == g_pFlashlightFunc) {
            PK_DBG("[kdSetFlashlightDrv] flashlightIdx init fail\n");
            if (NULL != kdFlashlightList[KD_DEFAULT_FLASHLIGHT_INDEX].flashlightInit) {
                kdFlashlightList[KD_DEFAULT_FLASHLIGHT_INDEX].flashlightInit(&g_pFlashlightFunc);
                if (NULL == g_pFlashlightFunc) {
                    PK_DBG("[kdSetFlashlightDrv] KD_DEFAULT_FLASHLIGHT_INDEX init fail\n");
                    return -EIO;
                }
            }
        }
    }

    //open flashlight driver
    if (g_pFlashlightFunc) {
       g_pFlashlightFunc->flashlight_open(0);
    }

	return 0;
}
/*****************************************************************************

*****************************************************************************/
#ifdef USE_UNLOCKED_IOCTL
static long flashlight_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int flashlight_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
    int i4RetValue = 0;

    //PK_DBG("%x, %x \n",cmd,arg);

    switch(cmd)
    {
        case FLASHLIGHTIOC_X_SET_DRIVER:
            i4RetValue = kdSetFlashlightDrv((unsigned int*)&arg);
            break;
    	default :
    	    if (g_pFlashlightFunc) {
    	        i4RetValue = g_pFlashlightFunc->flashlight_ioctl(cmd,arg);
    	    }
    		break;
    }

    return i4RetValue;
}

static int flashlight_open(struct inode *inode, struct file *file)
{
    int i4RetValue = 0;
    PK_DBG("[flashlight_open] E\n");
    return i4RetValue;
}

static int flashlight_release(struct inode *inode, struct file *file)
{
    PK_DBG("[flashlight_release] E\n");

    if (g_pFlashlightFunc) {
        g_pFlashlightFunc->flashlight_release(0);
        g_pFlashlightFunc = NULL;
    }

    return 0;
}

/*****************************************************************************/
/* Kernel interface */
static struct file_operations flashlight_fops = {
    .owner      = THIS_MODULE,
#ifdef USE_UNLOCKED_IOCTL
    .unlocked_ioctl      = flashlight_ioctl,
#else
    .ioctl      = flashlight_ioctl,
#endif
    .open       = flashlight_open,
    .release    = flashlight_release,
};

/*****************************************************************************
Driver interface
*****************************************************************************/
struct flashlight_data{
    spinlock_t lock;
    wait_queue_head_t read_wait;
    struct semaphore sem;
};
static struct class *flashlight_class = NULL;
static struct device *flashlight_device = NULL;
static struct flashlight_data flashlight_private;
static dev_t flashlight_devno;
static struct cdev flashlight_cdev;
/****************************************************************************/
//<2013/3/19-22985-jessicatseng, [Pelican] Fix issues of flashlight's properties
//<2013/1/18-20610-jessicatseng, [Pelican] Integrate flashlight IC LM3642 for PRE-MP SW
#if defined(FLASHLIGHT_IC_LM3642)
#define GPIO_CAMERA_FLASH_MODE GPIO209
#define GPIO_CAMERA_FLASH_MODE_M_GPIO  GPIO_MODE_00
#define GPIO_CAMERA_FLASH_EN GPIO213
#define GPIO_CAMERA_FLASH_EN_M_GPIO  GPIO_MODE_00

static ssize_t show_flashmode(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char databuf[2];

	PK_DBG("[flashlight] show_flashmode\n");
	printk("[flashlight] show_flashmode\n");

//<2013/3/8-22644-jessicatseng, [Pelican] Using hwPowerOn()/hwPowerDown() function to enable/disable VGP1 for flashlgith driver
	//pmic_ldo_vol_sel(MT65XX_POWER_LDO_VGP1, VOL_1800);
	//pmic_ldo_enable(MT65XX_POWER_LDO_VGP1, 1);
	hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_1800, "LM3642_FLASHLIGHT");
//>2013/3/8-22644-jessicatseng	

	mdelay(2); 

	
	databuf[0] = 0x09;
	databuf[1] = 0x19;
	i2c_master_send(flashlight_i2c_client, (const char*)&databuf, 2);

	databuf[0] = 0x0A;
	databuf[1] = 0x23;
	ret = i2c_master_send(flashlight_i2c_client, (const char*)&databuf, 2);

	mt_set_gpio_mode(GPIO_CAMERA_FLASH_EN, GPIO_CAMERA_FLASH_EN_M_GPIO);
	mt_set_gpio_dir(GPIO_CAMERA_FLASH_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_EN, GPIO_OUT_ONE);

	mdelay(300);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_EN, GPIO_OUT_ZERO);	
	
//<2013/3/8-22644-jessicatseng, [Pelican] Using hwPowerOn()/hwPowerDown() function to enable/disable VGP1 for flashlgith driver
	//pmic_ldo_enable(MT65XX_POWER_LDO_VGP1, 0);
	hwPowerDown(MT65XX_POWER_LDO_VGP1, "LM3642_FLASHLIGHT");
//>2013/3/8-22644-jessicatseng	
	
	return sprintf(buf, "%u\n", ret);
}

static ssize_t show_torchmode(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char databuf[2];

	PK_DBG("[flashlight] show_torchmode\n");
	printk("[flashlight] show_torchmode\n");
	
//<2013/3/8-22644-jessicatseng, [Pelican] Using hwPowerOn()/hwPowerDown() function to enable/disable VGP1 for flashlgith driver
	//pmic_ldo_vol_sel(MT65XX_POWER_LDO_VGP1, VOL_1800);
	//pmic_ldo_enable(MT65XX_POWER_LDO_VGP1, 1);
	hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_1800, "LM3642_FLASHLIGHT");
//>2013/3/8-22644-jessicatseng

	mdelay(2); 

	databuf[0] = 0x09;
	databuf[1] = 0x19;
	i2c_master_send(flashlight_i2c_client, (const char*)&databuf, 2);

	databuf[0] = 0x0A;
	databuf[1] = 0x12;
	ret = i2c_master_send(flashlight_i2c_client, (const char*)&databuf, 2);
	
	mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE, GPIO_CAMERA_FLASH_MODE_M_GPIO);
	mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE, GPIO_OUT_ONE);

	mdelay(300);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE, GPIO_OUT_ZERO);	
	
//<2013/3/8-22644-jessicatseng, [Pelican] Using hwPowerOn()/hwPowerDown() function to enable/disable VGP1 for flashlgith driver
	//pmic_ldo_enable(MT65XX_POWER_LDO_VGP1, 0);
	hwPowerDown(MT65XX_POWER_LDO_VGP1, "LM3642_FLASHLIGHT");
//>2013/3/8-22644-jessicatseng	

	return sprintf(buf, "%u\n", ret);
}

//<2013/1/30-21182-jessicatseng, [Pelican] Add attributes of flashlight for ATS
static ssize_t show_torchmode_on(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char databuf[2];

	PK_DBG("[flashlight] show_torchmode_on\n");
	printk("[flashlight] show_torchmode_on\n");
	
//<2013/3/8-22644-jessicatseng, [Pelican] Using hwPowerOn()/hwPowerDown() function to enable/disable VGP1 for flashlgith driver
	//pmic_ldo_vol_sel(MT65XX_POWER_LDO_VGP1, VOL_1800);
	//pmic_ldo_enable(MT65XX_POWER_LDO_VGP1, 1);
	hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_1800, "LM3642_FLASHLIGHT");
//>2013/3/8-22644-jessicatseng

	mdelay(2); 

	databuf[0] = 0x09;
	databuf[1] = 0x19;
	i2c_master_send(flashlight_i2c_client, (const char*)&databuf, 2);
	
	databuf[0] = 0x0A;
	databuf[1] = 0x12;
	ret = i2c_master_send(flashlight_i2c_client, (const char*)&databuf, 2);

	mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE, GPIO_CAMERA_FLASH_MODE_M_GPIO);
	mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE, GPIO_OUT_ONE);

	//msleep(300);
	//mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE, GPIO_OUT_ZERO);	
	
//<2013/3/8-22644-jessicatseng, [Pelican] Using hwPowerOn()/hwPowerDown() function to enable/disable VGP1 for flashlgith driver
	//pmic_ldo_enable(MT65XX_POWER_LDO_VGP1, 0);
	hwPowerDown(MT65XX_POWER_LDO_VGP1, "LM3642_FLASHLIGHT");
//>2013/3/8-22644-jessicatseng	

	return sprintf(buf, "%u\n", ret);
}
//>2013/3/19-22985-jessicatseng

static ssize_t show_torchmode_off(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char databuf[2];

	PK_DBG("[flashlight] show_torchmode_off\n");
	printk("[flashlight] show_torchmode_off\n");
	
//<2013/3/8-22644-jessicatseng, [Pelican] Using hwPowerOn()/hwPowerDown() function to enable/disable VGP1 for flashlgith driver
	//pmic_ldo_vol_sel(MT65XX_POWER_LDO_VGP1, VOL_1800);
	//pmic_ldo_enable(MT65XX_POWER_LDO_VGP1, 1);
	hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_1800, "LM3642_FLASHLIGHT");
//>2013/3/8-22644-jessicatseng	

	mdelay(2); 

	mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE, GPIO_CAMERA_FLASH_MODE_M_GPIO);
	mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE, GPIO_OUT_ONE);

	databuf[0] = 0x0A;
	databuf[1] = 0x12;
	ret = i2c_master_send(flashlight_i2c_client, (const char*)&databuf, 2);

	//msleep(300);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE, GPIO_OUT_ZERO);	
	
//<2013/3/8-22644-jessicatseng, [Pelican] Using hwPowerOn()/hwPowerDown() function to enable/disable VGP1 for flashlgith driver
	//pmic_ldo_enable(MT65XX_POWER_LDO_VGP1, 0);
	hwPowerDown(MT65XX_POWER_LDO_VGP1, "LM3642_FLASHLIGHT");
//>2013/3/8-22644-jessicatseng	

	return sprintf(buf, "%u\n", ret);
}
//>2013/1/30-21182-jessicatseng

static ssize_t show_frontflashmode(struct device *dev, struct device_attribute *attr, char *buf)
{
	PK_DBG("[flashlight] show_frontflashmode\n");
	printk("[flashlight] show_frontflashmode\n");
	
	upmu_set_flash_dim_duty(0x10);
	upmu_set_flash_sel(2);
	upmu_set_rg_bst_drv_1m_ck_pdn(0);
	upmu_set_flash_en(1);
	msleep(300);
	//upmu_set_rg_bst_drv_1m_ck_pdn(1);
	upmu_set_flash_en(0);
}

//<2013/1/30-21182-jessicatseng, [Pelican] Add attributes of flashlight for ATS
static ssize_t show_front_torchmode_on(struct device *dev, struct device_attribute *attr, char *buf)
{
	PK_DBG("[flashlight] show_front_torchmode_on\n");
	printk("[flashlight] show_front_torchmode_on\n");
	
	upmu_set_flash_dim_duty(0x10);
	upmu_set_flash_sel(2);
	upmu_set_rg_bst_drv_1m_ck_pdn(0);
	upmu_set_flash_en(1);
	//msleep(300);
	//upmu_set_rg_bst_drv_1m_ck_pdn(1);
	//upmu_set_flash_en(0);
}

static ssize_t show_front_torchmode_off(struct device *dev, struct device_attribute *attr, char *buf)
{
	PK_DBG("[flashlight] show_front_torchmode_off\n");
	printk("[flashlight] show_front_torchmode_off\n");
	
	//upmu_set_flash_dim_duty(0x10);
	//upmu_set_flash_sel(2);
	//upmu_set_rg_bst_drv_1m_ck_pdn(0);
	//upmu_set_flash_en(1);
	//msleep(300);
	//upmu_set_rg_bst_drv_1m_ck_pdn(1);
	upmu_set_flash_en(0);
}

static DEVICE_ATTR(flashmode, 0664, show_flashmode, NULL);
static DEVICE_ATTR(torchmode, 0664, show_torchmode, NULL);
static DEVICE_ATTR(torchmode_on, 0664, show_torchmode_on, NULL);
static DEVICE_ATTR(torchmode_off, 0664, show_torchmode_off, NULL);
static DEVICE_ATTR(frontflashmode, 0664, show_frontflashmode, NULL);
static DEVICE_ATTR(front_torchmode_on, 0664, show_front_torchmode_on, NULL);
static DEVICE_ATTR(front_torchmode_off, 0664, show_front_torchmode_off, NULL);

static struct device_attribute *flashlight_class_attrs[] = {
	&dev_attr_flashmode,
	&dev_attr_torchmode,
	&dev_attr_torchmode_on,
	&dev_attr_torchmode_off,
	&dev_attr_frontflashmode,
	&dev_attr_front_torchmode_on,
	&dev_attr_front_torchmode_off
};
//>2013/1/30-21182-jessicatseng

static int flashlight_create_attr(struct device *dev) 
{
	int idx, err = 0;
	int num = (int)(sizeof(flashlight_class_attrs)/sizeof(flashlight_class_attrs[0]));

	if (!dev)
	return -EINVAL;

	printk("[flashlight_probe] create_attr,num=%d\n", num);  

	for (idx = 0; idx < num; idx++)
	{
		if ((err = device_create_file(dev, flashlight_class_attrs[idx])))
		{            
			PK_DBG("device_create_file (%s) = %d\n", flashlight_class_attrs[idx]->attr.name, err);        
			break;
		}
	}

	return err;
}

static int lm3642_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	printk("[flashlight_probe] lm3642_i2c_probe\n");	
	
	flashlight_i2c_client = i2c;
	
	mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE, GPIO_CAMERA_FLASH_MODE_M_GPIO);
	mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE, GPIO_OUT_ZERO);
	
	mt_set_gpio_mode(GPIO_CAMERA_FLASH_EN, GPIO_CAMERA_FLASH_EN_M_GPIO);
	mt_set_gpio_dir(GPIO_CAMERA_FLASH_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_EN, GPIO_OUT_ZERO);

	return 0;
}

static int lm3642_i2c_remove(struct i2c_client *i2c)
{
	mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE, GPIO_CAMERA_FLASH_MODE_M_GPIO);
	mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE, GPIO_OUT_ZERO);
	
	mt_set_gpio_mode(GPIO_CAMERA_FLASH_EN, GPIO_CAMERA_FLASH_EN_M_GPIO);
	mt_set_gpio_dir(GPIO_CAMERA_FLASH_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CAMERA_FLASH_EN, GPIO_OUT_ZERO);
		
	flashlight_i2c_client = NULL;
	i2c_unregister_device(i2c);

	return 0;
}

static const struct i2c_device_id lm3642_i2c_ids[] = {
	{ "lm3642", 0 },
	{ },
};

static struct i2c_board_info __initdata i2c_lm3642={ I2C_BOARD_INFO("lm3642", (0xc6>>1))};////init in module

static struct i2c_driver lm3642_i2c_driver = {
	.driver = {
		   .name = "lm3642",
		   .owner = THIS_MODULE,
	},
	.id_table	= lm3642_i2c_ids,
	.probe		= lm3642_i2c_probe,
	.remove		= __devexit_p(lm3642_i2c_remove),
};
#endif
//>2013/1/18-20610-jessicatseng

#define ALLOC_DEVNO
static int flashlight_probe(struct platform_device *dev)
{
    int ret = 0, err = 0;

	PK_DBG("[flashlight_probe] start\n");

#ifdef ALLOC_DEVNO
    ret = alloc_chrdev_region(&flashlight_devno, 0, 1, FLASHLIGHT_DEVNAME);
    if (ret) {
        PK_ERR("[flashlight_probe] alloc_chrdev_region fail: %d\n", ret);
        goto flashlight_probe_error;
    } else {
        PK_DBG("[flashlight_probe] major: %d, minor: %d\n", MAJOR(flashlight_devno), MINOR(flashlight_devno));
    }
    cdev_init(&flashlight_cdev, &flashlight_fops);
    flashlight_cdev.owner = THIS_MODULE;
    err = cdev_add(&flashlight_cdev, flashlight_devno, 1);
    if (err) {
        PK_ERR("[flashlight_probe] cdev_add fail: %d\n", err);
        goto flashlight_probe_error;
    }
#else
    #define FLASHLIGHT_MAJOR 242
    ret = register_chrdev(FLASHLIGHT_MAJOR, FLASHLIGHT_DEVNAME, &flashlight_fops);
    if (ret != 0) {
        PK_ERR("[flashlight_probe] Unable to register chardev on major=%d (%d)\n", FLASHLIGHT_MAJOR, ret);
        return ret;
    }
    flashlight_devno = MKDEV(FLASHLIGHT_MAJOR, 0);
#endif


    flashlight_class = class_create(THIS_MODULE, "flashlightdrv");
    if (IS_ERR(flashlight_class)) {
        PK_ERR("[flashlight_probe] Unable to create class, err = %d\n", (int)PTR_ERR(flashlight_class));
        goto flashlight_probe_error;
    }

    flashlight_device = device_create(flashlight_class, NULL, flashlight_devno, NULL, FLASHLIGHT_DEVNAME);
    if(NULL == flashlight_device){
        PK_ERR("[flashlight_probe] device_create fail\n");
        goto flashlight_probe_error;
    }

//<2013/1/18-20610-jessicatseng, [Pelican] Integrate flashlight IC LM3642 for PRE-MP SW
#if defined(FLASHLIGHT_IC_LM3642)
    flashlight_create_attr(flashlight_device);
#endif
//>2013/1/14-20208-jessicatseng

    /*initialize members*/
    spin_lock_init(&flashlight_private.lock);
    init_waitqueue_head(&flashlight_private.read_wait);
    //init_MUTEX(&flashlight_private.sem);
    sema_init(&flashlight_private.sem, 1);

    PK_DBG("[flashlight_probe] Done\n");
    return 0;

flashlight_probe_error:
#ifdef ALLOC_DEVNO
    if (err == 0)
        cdev_del(&flashlight_cdev);
    if (ret == 0)
        unregister_chrdev_region(flashlight_devno, 1);
#else
    if (ret == 0)
        unregister_chrdev(MAJOR(flashlight_devno), FLASHLIGHT_DEVNAME);
#endif
    return -1;
}

static int flashlight_remove(struct platform_device *dev)
{

    PK_DBG("[flashlight_probe] start\n");

#ifdef ALLOC_DEVNO
    cdev_del(&flashlight_cdev);
    unregister_chrdev_region(flashlight_devno, 1);
#else
    unregister_chrdev(MAJOR(flashlight_devno), FLASHLIGHT_DEVNAME);
#endif
    device_destroy(flashlight_class, flashlight_devno);
    class_destroy(flashlight_class);

    PK_DBG("[flashlight_probe] Done\n");
    return 0;
}


static struct platform_driver flashlight_platform_driver =
{
    .probe      = flashlight_probe,
    .remove     = flashlight_remove,
    .driver     = {
        .name = FLASHLIGHT_DEVNAME,
		.owner	= THIS_MODULE,
    },
};

static struct platform_device flashlight_platform_device = {
    .name = FLASHLIGHT_DEVNAME,
    .id = 0,
    .dev = {
    }
};

static int __init flashlight_init(void)
{
    int ret = 0;
    PK_DBG("[flashlight_probe] start\n");

//<2013/1/18-20610-jessicatseng, [Pelican] Integrate flashlight IC LM3642 for PRE-MP SW
#if defined(FLASHLIGHT_IC_LM3642)
	i2c_register_board_info(1, &i2c_lm3642, 1);
	if(i2c_add_driver(&lm3642_i2c_driver))
	{
		PK_DBG("[flashlight]add driver error\n");		
		return -1;
	}
#endif
//>2013/1/18-20610-jessicatseng

	ret = platform_device_register (&flashlight_platform_device);
	if (ret) {
        PK_ERR("[flashlight_probe] platform_device_register fail\n");
        return ret;
	}

    ret = platform_driver_register(&flashlight_platform_driver);
	if(ret){
		PK_ERR("[flashlight_probe] platform_driver_register fail\n");
		return ret;
	}

	PK_DBG("[flashlight_probe] done!\n");
    return ret;
}

static void __exit flashlight_exit(void)
{
    PK_DBG("[flashlight_probe] start\n");
    platform_driver_unregister(&flashlight_platform_driver);

//<2013/1/18-20610-jessicatseng, [Pelican] Integrate flashlight IC LM3642 for PRE-MP SW
#if defined(FLASHLIGHT_IC_LM3642)
    i2c_del_driver(&lm3642_i2c_driver);
#endif
//>2013/1/18-20610-jessicatseng

    //to flush work queue
    //flush_scheduled_work();
    PK_DBG("[flashlight_probe] done!\n");
}

/*****************************************************************************/
module_init(flashlight_init);
module_exit(flashlight_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jackie Su <jackie.su@mediatek.com>");
MODULE_DESCRIPTION("Flashlight control Driver");


