

#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

#include <linux/firmware.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>

#include <mach/mt_boot.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_typedefs.h>

//#include <mach/sbchk_base.h>

//<2013/05/08-24737-stevenchen, [Pelican][drv] Create file storing LCM info for ATS use.
#include <mach/mt_gpio.h>
//>2013/05/08-24737-stevenchen

#define MOD "BOOT"

/* hardware version register */
#define VER_BASE            (DEVINFO_BASE)
#define APHW_CODE           (VER_BASE)
#define APHW_SUBCODE        (VER_BASE + 0x04)
#define APHW_VER            (VER_BASE + 0x08)
#define APSW_VER            (VER_BASE + 0x0C)

/* this vairable will be set by mt6589_fixup */
BOOTMODE g_boot_mode __nosavedata = UNKNOWN_BOOT;
boot_reason_t g_boot_reason __nosavedata = BR_UNKNOWN;
META_COM_TYPE g_meta_com_type = META_UNKNOWN_COM;
unsigned int g_meta_com_id = 0;

//<2013/05/22-25191-EricLin, Add sbchk related functions.
extern void sbchk_base(void);
//>2013/05/22-25191-EricLin
struct meta_driver {
    struct device_driver driver;
    const struct platform_device_id *id_table;
};

static struct meta_driver meta_com_type_info =
{
    .driver  = {
        .name = "meta_com_type_info",
        .bus = &platform_bus_type,
        .owner = THIS_MODULE,
    },
    .id_table = NULL,
};

static struct meta_driver meta_com_id_info =
{
    .driver = {
        .name = "meta_com_id_info",
        .bus = &platform_bus_type,
        .owner = THIS_MODULE,
    },
    .id_table = NULL,
};

static ssize_t (*md_show)(char*) = NULL;
static ssize_t (*md_store)(const char*,size_t) = NULL;

void boot_register_md_func(ssize_t (*show)(char*), ssize_t (*store)(const char*,size_t))
{
    md_show = show;
    md_store = store;
}

static ssize_t boot_show(struct kobject *kobj, struct attribute *a, char *buf)
{
	//<<JB
	printk("[boot_show]\n");
	//>>JB
    if (!strncmp(a->name, MD_SYSFS_ATTR, strlen(MD_SYSFS_ATTR)) && md_show) 
    {
        return md_show(buf);
    }
    else if (!strncmp(a->name, INFO_SYSFS_ATTR, strlen(INFO_SYSFS_ATTR)))
    {
        return sprintf(buf, "%04X%04X%04X%04X %x\n", get_chip_code(), get_chip_hw_subcode(),
                            get_chip_hw_ver_code(), get_chip_sw_ver_code(), mt_get_chip_sw_ver());
    }
    else
    {
        return sprintf(buf, "%d\n", g_boot_mode);
    }
}

static ssize_t boot_store(struct kobject *kobj, struct attribute *a, const char *buf, size_t count)
{
	//<<JB
	printk("[boot_store]\n");
	//>>JB
#ifndef MTK_EMMC_SUPPORT
    /* check sbchk engine before booting up modem */
    // FIX-ME : marked for early porting
    //sbchk_base();	
#endif
	
//<2013/05/22-25191-EricLin, Add sbchk related functions.
	sbchk_base();
//>2013/05/22-25191-EricLin
    if (!strncmp(a->name, MD_SYSFS_ATTR, strlen(MD_SYSFS_ATTR)) && md_store) 
    {
        return md_store(buf, count);
    }
    
    return count;
}


/* boot object */
static struct kobject boot_kobj;
static struct sysfs_ops boot_sysfs_ops = {
    .show = boot_show,
    .store = boot_store
};

/* boot attribute */
struct attribute boot_attr = {BOOT_SYSFS_ATTR, 0644};
struct attribute md_attr = {MD_SYSFS_ATTR, 0664};
struct attribute info_attr = {INFO_SYSFS_ATTR, 0644};
static struct attribute *boot_attrs[] = {
    &boot_attr,
    &md_attr,
    &info_attr,
    NULL
};

/* boot type */
static struct kobj_type boot_ktype = {
    .sysfs_ops = &boot_sysfs_ops,
    .default_attrs = boot_attrs
};

/* boot device node */
static dev_t boot_dev_num;
static struct cdev boot_cdev;
static struct file_operations boot_fops = {
    .owner = THIS_MODULE,
    .open = NULL,
    .release = NULL,
    .write = NULL,
    .read = NULL,
    .unlocked_ioctl = NULL
};

/* boot device class */
static struct class *boot_class;
static struct device *boot_device;


/* return boot mode */
BOOTMODE get_boot_mode(void)
{
    return g_boot_mode;
}

/* return hardware version */
unsigned int get_chip_code(void)
{     
    return DRV_Reg32(APHW_CODE);
}

unsigned int get_chip_hw_ver_code(void)
{   
    return DRV_Reg32(APHW_VER);
}

unsigned int get_chip_sw_ver_code(void)
{  
    return DRV_Reg32(APSW_VER);
}

unsigned int get_chip_hw_subcode(void)
{
    return DRV_Reg32(APHW_SUBCODE);
}

CHIP_SW_VER mt_get_chip_sw_ver(void)
{
    return (CHIP_SW_VER)get_chip_sw_ver_code();
}

CHIP_VER get_chip_eco_ver(void) /*TO BE REMOVED*/
{   
    return DRV_Reg32(APHW_VER);
}
/* for convenience, simply check is meta mode or not */
bool is_meta_mode(void)
{   
    if(g_boot_mode == META_BOOT)
    {   
        return true;
    }
    else
    {   
        return false;
    }
}

bool is_advanced_meta_mode(void)
{
    if (g_boot_mode == ADVMETA_BOOT)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool com_is_enable(void)  // usb android will check whether is com port enabled default. in normal boot it is default enabled. 
{	
    if(g_boot_mode == NORMAL_BOOT)
	{	
        return false;
	}
	else
	{	
        return true;
	}
}

static int boot_mode_proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
    char *p = page;
    int len = 0; 
	//<<JB
	printk("[boot_mode_proc]\n");
	//>>JB
    p += sprintf(p, "\n\rMT6589 BOOT MODE : " );
    switch(g_boot_mode)
    {
        case NORMAL_BOOT :
            p += sprintf(p, "NORMAL BOOT\n");
            break;
        case META_BOOT :
            p += sprintf(p, "META BOOT\n");
            break;
        case ADVMETA_BOOT :
            p += sprintf(p, "Advanced META BOOT\n");
            break;   
        case ATE_FACTORY_BOOT :
            p += sprintf(p, "ATE_FACTORY BOOT\n");
            break;
        case ALARM_BOOT :
            p += sprintf(p, "ALARM BOOT\n");
            break;
        default :
            p += sprintf(p, "UNKNOWN BOOT\n");
            break;
    }  
    *start = page + off;
    len = p - page;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;     
}

//<2013/05/08-24737-stevenchen, [Pelican][drv] Create file storing LCM info for ATS use.
static int LCM_vendor_proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
    char *p = page;
    int len = 0; 
    int lcm_id=0;

	//<<JB
	printk("[LCM_vendor_proc]\n");
	//>>JB
    lcm_id = mt_get_gpio_in(GPIO_LCM_ID_PIN);

    if(lcm_id == 0)
    {
    	p += sprintf(p, "Truly LCM\n");         
    }
    else
    {
    	p += sprintf(p, "TCL LCM\n");         
    }

    *start = page + off;
    len = p - page;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;     
}
//>2013/05/08-24737-stevenchen

static ssize_t meta_com_type_show(struct device_driver *driver, char *buf)
{
  return sprintf(buf, "%d\n", g_meta_com_type);
}

static ssize_t meta_com_type_store(struct device_driver *driver, const char *buf, size_t count)
{
  /*Do nothing*/
  return count;
}

DRIVER_ATTR(meta_com_type_info, 0644, meta_com_type_show, meta_com_type_store);


static ssize_t meta_com_id_show(struct device_driver *driver, char *buf)
{
  return sprintf(buf, "%d\n", g_meta_com_id);
}

static ssize_t meta_com_id_store(struct device_driver *driver, const char *buf, size_t count)
{
  /*Do nothing*/
  return count;
}

DRIVER_ATTR(meta_com_id_info, 0644, meta_com_id_show, meta_com_id_store);


static int __init boot_mod_init(void)
{
    int ret;

    /* allocate device major number */
    if (alloc_chrdev_region(&boot_dev_num, 0, 1, BOOT_DEV_NAME) < 0) {
        printk("[%s] fail to register chrdev\n",MOD);
        return -1;
    }	

    /* add character driver */
    cdev_init(&boot_cdev, &boot_fops);
    ret = cdev_add(&boot_cdev, boot_dev_num, 1);
    if (ret < 0) {
        printk("[%s] fail to add cdev\n",MOD);
        return ret;
    }

    /* create class (device model) */
    boot_class = class_create(THIS_MODULE, BOOT_DEV_NAME);
    if (IS_ERR(boot_class)) {
        printk("[%s] fail to create class\n",MOD);
        return (int)boot_class;
    }

    boot_device = device_create(boot_class, NULL, boot_dev_num, NULL, BOOT_DEV_NAME);
    if (IS_ERR(boot_device)) {
        printk("[%s] fail to create device\n",MOD);
        return (int)boot_device;
    }

    /* add kobject */
    ret = kobject_init_and_add(&boot_kobj, &boot_ktype, &(boot_device->kobj), BOOT_SYSFS);
    if (ret < 0) {
        printk("[%s] fail to add kobject\n",MOD);
        return ret;
    }
    
    printk("[%s] CHIP = 0x%04x 0x%04x\n", MOD, get_chip_code(), get_chip_hw_subcode());
    
    /* create proc entry at /proc/boot_mode */
    create_proc_read_entry("boot_mode", S_IRUGO, NULL, boot_mode_proc, NULL);

    if(g_boot_mode == META_BOOT || g_boot_mode == ADVMETA_BOOT || g_boot_mode == ATE_FACTORY_BOOT || g_boot_mode == FACTORY_BOOT)
    {
        /* register driver and create sysfs files */
        ret = driver_register(&meta_com_type_info.driver);
        if (ret) 
        {
            printk("fail to register META COM TYPE driver\n");
        }
        ret = driver_create_file(&meta_com_type_info.driver, &driver_attr_meta_com_type_info);
        if (ret) 
        {
            printk("[BOOT INIT] Fail to create META COM TPYE sysfs file\n");
        }

        ret = driver_register(&meta_com_id_info.driver);
        if (ret) 
        {
            printk("fail to register META COM ID driver\n");
        }
        ret = driver_create_file(&meta_com_id_info.driver, &driver_attr_meta_com_id_info);
        if (ret) 
        {
            printk("[BOOT INIT] Fail to create META COM ID sysfs file\n");
        }
    }    
    
//<2013/05/08-24737-stevenchen, [Pelican][drv] Create file storing LCM info for ATS use.
    create_proc_read_entry("LCM", S_IRUGO, NULL, LCM_vendor_proc, NULL);
//>2013/05/08-24737-stevenchen

    return 0;
}

static void __exit boot_mod_exit(void)
{
    cdev_del(&boot_cdev);
}

module_init(boot_mod_init);
module_exit(boot_mod_exit);
MODULE_DESCRIPTION("MT6589 Boot Information Querying Driver");
MODULE_LICENSE("Proprietary");
EXPORT_SYMBOL(is_meta_mode);
EXPORT_SYMBOL(is_advanced_meta_mode);
EXPORT_SYMBOL(get_boot_mode);
EXPORT_SYMBOL(boot_register_md_func);
EXPORT_SYMBOL(mt_get_chip_sw_ver);
