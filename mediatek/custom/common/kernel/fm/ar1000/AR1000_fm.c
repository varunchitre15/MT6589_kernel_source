#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h> // udelay()
#include <linux/device.h> // device_create()
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/version.h>      /* constant of kernel version */
#include <asm/uaccess.h> // get_user()

#include <linux/fm.h>
#include <mach/mt6516_gpio.h>
#include <linux/proc_fs.h>
#include <linux/string.h>

#define FM_ALERT(f, s...) \
    do { \
        printk(KERN_ALERT "FM " f, ## s); \
    } while(0)

#ifdef FMDEBUG
#define FM_DEBUG(f, s...) \
    do { \
        printk(KERN_ALERT "FM " f, ## s); \
    } while(0)
#else
#define FM_DEBUG(f, s...)
#endif

/******************************************************************************
 * CONSTANT DEFINITIONS
 *****************************************************************************/
#define AR1000_SLAVE_ADDR   0x20

#define AR1000_MASK_STC     0x0020 // Seek/Tune complete D5 in 13H
#define AR1000_MASK_SF      0x0010 // Seek failed D4 in 13H
#define AR1000_MASK_ST      0x0004 // Stereo indicator  D3 in 13H

#define AR1000_MASK_VOL     0x0780 // Volume
#define AR1000_MASK_VOL2    0xf000 // Volume 2

#define AR1000_MASK_RSSI    0xFE00 // RSSI

#define AR1000_MAX_WAIT_CNT 100

#define AR1000_DEV           "AR1000"

//customer need customize the I2C port
#define AR1000_I2C_PORT     2

static struct proc_dir_entry *g_fm_proc = NULL;
static struct fm *g_fm_struct = NULL;

#define FM_PROC_FILE "fm"

/******************************************************************************
 * STRUCTURE DEFINITIONS
 *****************************************************************************/
enum AR1000_REG {
    AR1000_R0 = 0,
    AR1000_R1,
    AR1000_R2,
    AR1000_R3,
    AR1000_R4,
    AR1000_R5,
    AR1000_R6,
    AR1000_R7,
    AR1000_R8,
    AR1000_R9,
    AR1000_R10,
    AR1000_R11,
    AR1000_R12,
    AR1000_R13,
    AR1000_R14,
    AR1000_R15,
    AR1000_R16,
    AR1000_R17,
    AR1000_RSSI,
    AR1000_STATUS,
    AR1000_RBS,
    AR1000_RDS1,
    AR1000_RDS2,
    AR1000_RDS3,
    AR1000_RDS4,
    AR1000_RDS5,
    AR1000_RDS6,
    AR1000_DEVID,
    AR1000_CHIPID,
};

struct vol_tune {
	uint16_t vol;
	uint16_t vol2;
};

static struct vol_tune g_vol_level[16] = 
	{{0xf, 0x0},
	{0xf, 0xc},
	//{0xf, 0xd},
	{0xf, 0xe},
	{0xf, 0xf},
	{0xe, 0xe},
	//{0xe, 0xf},
	{0xd, 0xe},
	//{0xd, 0xf},
	{0xb, 0xf},
	{0xa, 0xf},
	{0x9, 0xf},
	{0x7, 0xf},
	//{0x6, 0xe},
	{0x6, 0xf},
	{0x5, 0xe},
	//{0x5, 0xf},
	{0x3, 0xe},
	//{0x3, 0xf},
	{0x2, 0xf},
	{0x1, 0xf},
	{0x0, 0xf},};

struct fm {
    uint ref;
    bool powerup;
    uint16_t chip_id;
    uint16_t device_id;
    dev_t dev_t;
    uint16_t min_freq; // KHz
    uint16_t max_freq; // KHz
    uint8_t band;   // TODO
    struct class *cls;
    struct device *dev;
    struct cdev cdev;
    struct i2c_client *i2c_client;
};

//clk pin define for FM
/*
#define GPIO_FM_CLK_PIN        (GPIO117)
#define GPIO_FM_CLK_PIN_M_CLK  (GPIO_MODE_01)
#define GPIO_FM_CLK_PIN_M_GPIO (GPIO_MODE_00)
#define GPIO_FM_CLK_PIN_CLK    (CLK_OUT2)
#define GPIO_FM_CLK_PIN_FREQ   (CLK_SRC_F32K)
*/
#define AR1000_clear_hmute(c)  AR1000_set_bits((c), AR1000_R1, 0x0, 0x2)
#define AR1000_enable_hmute(c) AR1000_set_bits((c), AR1000_R1, 0x2, 0x2)

#define AR1000_clear_tune(c)   AR1000_set_bits((c), AR1000_R2, 0x0, 0x200)
#define AR1000_enable_tune(c)  AR1000_set_bits((c), AR1000_R2, 0x200, 0x200)

#define AR1000_clear_seek(c)   AR1000_set_bits((c), AR1000_R3, 0x0, 0x4000)
#define AR1000_enable_seek(c)  AR1000_set_bits((c), AR1000_R3, 0x4000, 0x4000)

/******************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/
extern void fm_low_power_wa(int fmon);

static int AR1000_wait_stc(struct i2c_client *client, uint count);
static int AR1000_read(struct i2c_client *client, uint8_t addr, uint16_t *val);
static int AR1000_write(struct i2c_client *client, uint8_t addr, uint16_t val);
static int AR1000_set_bits(struct i2c_client *client, uint8_t addr,
                             uint16_t bits, uint16_t mask);
static void AR1000_em_test(struct i2c_client *client, uint16_t group_idx, uint16_t item_idx, uint32_t item_value);
static int fm_setup_cdev(struct fm *fm);
static int fm_ops_ioctl(struct inode *inode, struct file *filp,
                          unsigned int cmd, unsigned long arg);
static int fm_ops_open(struct inode *inode, struct file *filp);
static int fm_ops_release(struct inode *inode, struct file *filp);

static int fm_init(struct i2c_client *client);
static int fm_destroy(struct fm *fm);
static int fm_powerup(struct fm *fm, struct fm_tune_parm *parm);
static int fm_powerdown(struct fm *fm);
static int fm_tune(struct fm *fm, struct fm_tune_parm *parm);
static int fm_seek(struct fm *fm, struct fm_seek_parm *parm);
static int fm_setvol(struct fm *fm, uint32_t vol);
static int fm_getvol(struct fm *fm, uint32_t *vol);
static int fm_getrssi(struct fm *fm, uint32_t *rssi);
static int fm_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
static int fm_i2c_attach_adapter(struct i2c_adapter *adapter);
static int fm_i2c_detect(struct i2c_adapter *adapter, int addr, int kind);
static int fm_i2c_detach_client(struct i2c_client *client);
#else
static int fm_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int fm_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int fm_i2c_remove(struct i2c_client *client);
#endif

/******************************************************************************
 * GLOBAL DATA
 *****************************************************************************/
/* Addresses to scan */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
static unsigned short normal_i2c[] = {AR1000_SLAVE_ADDR, I2C_CLIENT_END};
static unsigned short ignore = I2C_CLIENT_END;

static struct i2c_client_address_data AR1000_addr_data = {
    .normal_i2c = normal_i2c,
    .probe = &ignore,
    .ignore = &ignore,
};
#else
static const struct i2c_device_id fm_i2c_id =
{AR1000_DEV, 0};
static unsigned short force[] = {AR1000_I2C_PORT, AR1000_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short * const forces[] = {force, NULL};
static struct i2c_client_address_data addr_data = {.forces = forces};
#endif

static struct i2c_driver AR1000_driver = {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
    .driver = {
        .owner = THIS_MODULE,
        .name = AR1000_DEV,
    },
    .attach_adapter = fm_i2c_attach_adapter,
    .detach_client = fm_i2c_detach_client,
#else
    .probe = fm_i2c_probe,
    .remove = fm_i2c_remove,
    .detect = fm_i2c_detect,
    .driver.name = AR1000_DEV,
    .id_table = &fm_i2c_id,
    .address_data = &addr_data,
#endif
};

static uint16_t AR1000_DATA[18] = {
    0xFF7B,
    0x5915,
    0x10B9,
    0x8010,
    0x0780,
    0x28AB,
    0x6400,
    0x1EE7,
    0x7141,
    0x007D,
    0x81C6,
    0x4F65,
    0x970C,
    0xB845,
    0xFC2D,
    0x8097,
    0x04A1,
    0xE2A7
};

static struct file_operations fm_ops = {
    .owner = THIS_MODULE,
    .ioctl = fm_ops_ioctl,
    .open = fm_ops_open,
    .release = fm_ops_release,
};

static DECLARE_MUTEX(fm_ops_mutex);

/******************************************************************************
 *****************************************************************************/

/*
static void _open_audio_path(void)
{

    volatile uint *ANA_VOL = ( volatile uint *)0xF0060200;
    volatile uint *ANA_REG = ( volatile uint *)0xF0060204;
    volatile uint *ANA_PWR = ( volatile uint *)0xF0060208;

    FM_DEBUG("<= 0x%X=0x%X,0x%X=0x%X, 0x%X=0x%X\n",
        (uint)ANA_REG, *ANA_REG,
        (uint)ANA_VOL, *ANA_VOL,
        (uint)ANA_PWR, *ANA_PWR);

    *ANA_REG = 0x480; // set audio path to wired headset
    *ANA_PWR = 0x1F; // power all on

    FM_DEBUG("=> 0x%X=0x%X,0x%X=0x%X, 0x%X=0x%X\n",
        (uint)ANA_REG, *ANA_REG,
        (uint)ANA_VOL, *ANA_VOL,
        (uint)ANA_PWR, *ANA_PWR);
}
*/

/******************************************************************************
 *****************************************************************************/

#ifdef FMDEBUG
static void AR1000_dump_reg(struct i2c_client *client)
{
    int i;
    int err;
    uint16_t val;

    for (i = 0; i <= AR1000_CHIPID; i++)
    {
        val = 0;
        err = AR1000_read(client, i, &val);
        if (err == 0)
        {
            FM_DEBUG("%2d\t%04X\n", i, val);
        }
        else
        {
            FM_DEBUG("%2d\tXXXX\n", i);
        }
    }
}
#endif // FMDEBUG

/*
 *  AR1000_wait_stc
 */
static int AR1000_wait_stc(struct i2c_client *client, uint count)
{
    uint16_t val = 0;
    int ret;

    while (count--)
    {
        ret = AR1000_read(client, AR1000_STATUS, &val);
        if (ret)
        {
            FM_ALERT("AR1000_wait_stc AR1000_STATUS read failed\n");
	        return -1;
	    }
	    
        if (val & AR1000_MASK_STC)
            return 0;

        msleep(50);

        if (need_resched())
            cond_resched();
    }

    FM_DEBUG("AR1000_wait_stc failed\n");
    return -1;
}

/*
 *  AR1000_read
 */
static int AR1000_read(struct i2c_client *client, uint8_t addr, uint16_t *val)
{
    int n;
    char b[2] = {0};

    // first, send addr to AR1000
    n = i2c_master_send(client, (char*)&addr, 1);
    if (n < 0)
    {
        FM_ALERT("AR1000_read send:0x%X err:%d\n", addr, n);
        return -1;
    }

    // second, receive two byte from AR1000
    n = i2c_master_recv(client, b, 2);
    if (n < 0)
    {
        FM_ALERT("AR1000_read recv:0x%X err:%d\n", addr, n);
        return -1;
    }

    *val = ((uint16_t)b[0] << 8 | (uint16_t)b[1]);

    return 0;
}

/*
 *  AR1000_write
 */
static int AR1000_write(struct i2c_client *client, uint8_t addr, uint16_t val)
{
    int n;
    char b[3];

    b[0] = addr;
    b[1] = (char)(val >> 8);
    b[2] = (char)(val & 0xFF);

    n = i2c_master_send(client, b, 3);
    if (n < 0)
    {
        FM_ALERT("AR1000_write send:0x%X err:%d\n", addr, n);
        return -1;
    }

    return 0;
}

static int AR1000_set_bits(struct i2c_client *client, uint8_t addr,
                             uint16_t bits, uint16_t mask)
{
    int err;
    uint16_t val;

    err = AR1000_read(client, addr, &val);
    if (err)
        return err;

    val = ((val & (~mask)) | bits);

    err = AR1000_write(client, addr, val);
    if (err)
        return err;

    return 0;
}

static void AR1000_em_test(struct i2c_client *client, uint16_t group_idx, uint16_t item_idx, uint32_t item_value)
{
    FM_ALERT("AR1000_em_test  %d:%d:%d\n", group_idx, item_idx, item_value); 
    switch (group_idx)
	{
		case mono:
			if(item_value == 1)
			{
			    AR1000_set_bits(client, AR1000_R1, 0x08, 0x0); //force mono
			}
			else
			{
			    AR1000_set_bits(client, AR1000_R1, 0x0, 0x0008);
			}
			break;
		case stereo:
			if(item_value == 0)
			{
			    AR1000_set_bits(client, AR1000_R1, 0x0, 0x0008);
			}
			else
			{
			    FM_ALERT("AR1000 not support Blend test\n");	
			}
			break;
	    case RSSI_threshold:
	        item_value &= 0x7F;
	        AR1000_set_bits(client, AR1000_R3, item_value, 0x7F);
		    break;		    
	    case Softmute_Enable:
	        if (item_idx)
	        {
	            AR1000_enable_hmute(client);
	        }
	        else
	        {
	            AR1000_clear_hmute(client);       
	        }
            break;
       case De_emphasis:
			if(item_idx == 2) //75us
			{
			    AR1000_set_bits(client, AR1000_R1, 0x04, 0x0);				
			}
			else if(item_idx == 1) //50us
			{
			    AR1000_set_bits(client, AR1000_R1, 0x0, 0x04);			
			}
			else if(item_idx == 0) //0us
			{
			    FM_ALERT("AR1000 not support De_emphasis 0\n");		
			}
		    break;
		    
	   case HL_Side:
			if(item_idx == 2) //H-Side
			{
			    AR1000_set_bits(client, AR1000_R11, 0x8000, 0x0);			    
			}
			else if(item_idx == 1) //L-Side
			{
			    AR1000_set_bits(client, AR1000_R11, 0x0, 0x8000);		
			}
			else if(item_idx == 0) //Auto
			{
			    AR1000_set_bits(client, AR1000_R11, 0x0, 0x8000);
			    FM_ALERT("AR1000 L side for HL Auto\n");			
			}
		    break;
		default:
		    FM_ALERT("AR1000 not support this setting\n");
		    break;   
    }
}

static int fm_setup_cdev(struct fm *fm)
{
    int err;

    err = alloc_chrdev_region(&fm->dev_t, 0, 1, FM_NAME);
    if (err) {
        FM_ALERT("alloc dev_t failed\n");
        return -1;
    }

    FM_ALERT("alloc %s:%d:%d\n", FM_NAME,
                MAJOR(fm->dev_t), MINOR(fm->dev_t));

    cdev_init(&fm->cdev, &fm_ops);

    fm->cdev.owner = THIS_MODULE;
    fm->cdev.ops = &fm_ops;

    err = cdev_add(&fm->cdev, fm->dev_t, 1);
    if (err) {
        FM_ALERT("alloc dev_t failed\n");
        return -1;
    }

    fm->cls = class_create(THIS_MODULE, FM_NAME);
    if (IS_ERR(fm->cls)) {
        err = PTR_ERR(fm->cls);
        FM_ALERT("class_create err:%d\n", err);
        return err;            
    }    
    fm->dev = device_create(fm->cls, NULL, fm->dev_t, NULL, FM_NAME);

    return 0;
}

static int fm_ops_ioctl(struct inode *inode, struct file *filp,
                          unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct fm *fm = container_of(inode->i_cdev, struct fm, cdev);

    FM_DEBUG("%s\n", __func__);

    switch(cmd)
    {
        case FM_IOCTL_POWERUP:
        {
            struct fm_tune_parm parm;
            FM_DEBUG("FM_IOCTL_POWERUP\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//                return -EPERM;

            if (copy_from_user(&parm, (void*)arg, sizeof(struct fm_tune_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_powerup(fm, &parm);
            up(&fm_ops_mutex);
            if (copy_to_user((void*)arg, &parm, sizeof(struct fm_tune_parm)))
                return -EFAULT;
			fm_low_power_wa(1);
            break;
        }

        case FM_IOCTL_POWERDOWN:
	    {
            FM_DEBUG("FM_IOCTL_POWERDOWN\n");
// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//                return -EPERM;
            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_powerdown(fm);
            up(&fm_ops_mutex);
			fm_low_power_wa(0);
            break;
	    }

         // tune (frequency, auto Hi/Lo ON/OFF )
        case FM_IOCTL_TUNE:
        {
            struct fm_tune_parm parm;
            FM_DEBUG("FM_IOCTL_TUNE\n");
// FIXME!
//            if (!capable(CAP_SYS_ADMIN))
//                return -EPERM;

            if (copy_from_user(&parm, (void*)arg, sizeof(struct fm_tune_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_tune(fm, &parm);
            up(&fm_ops_mutex);

            if (copy_to_user((void*)arg, &parm, sizeof(struct fm_tune_parm)))
                return -EFAULT;

            break;
        }

        case FM_IOCTL_SEEK:
        {
            struct fm_seek_parm parm;
            FM_DEBUG("FM_IOCTL_SEEK\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (copy_from_user(&parm, (void*)arg, sizeof(struct fm_seek_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_seek(fm, &parm);
            up(&fm_ops_mutex);

            if (copy_to_user((void*)arg, &parm, sizeof(struct fm_seek_parm)))
                return -EFAULT;

            break;
        }
        
        case FM_IOCTL_SETVOL:
        {
            uint32_t vol;
            FM_DEBUG("FM_IOCTL_SETVOL\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

	        if(copy_from_user(&vol, (void*)arg, sizeof(uint32_t))) {
                FM_ALERT("copy_from_user failed\n");
                return -EFAULT;
            }

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_setvol(fm, vol);
            up(&fm_ops_mutex);

            break;
        }
        
        case FM_IOCTL_GETVOL:
        {
            uint32_t vol;
            FM_DEBUG("FM_IOCTL_GETVOL\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_getvol(fm, &vol);
            up(&fm_ops_mutex);

            if (copy_to_user((void*)arg, &vol, sizeof(uint32_t)))
                return -EFAULT;

            break;
        }

        case FM_IOCTL_MUTE:
        {
            uint32_t bmute;
            unsigned long ul;
            FM_DEBUG("FM_IOCTL_MUTE\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;
            if (ul = copy_from_user(&bmute, (void*)arg, sizeof(uint32_t)))
            {
                FM_DEBUG("copy_from_user mute failed:%d\n", ul);
                return -EFAULT;    
            }
            
            FM_DEBUG("FM_IOCTL_MUTE:%d\n", bmute); 
            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;

            if (bmute)
                ret = AR1000_enable_hmute(fm->i2c_client);
            else
                ret = AR1000_clear_hmute(fm->i2c_client);

            up(&fm_ops_mutex);

            break;
        }

        case FM_IOCTL_GETRSSI:
        {
            uint32_t rssi;
            FM_DEBUG("FM_IOCTL_GETRSSI\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;

            ret = fm_getrssi(fm, &rssi);
            up(&fm_ops_mutex);

            if (copy_to_user((void*)arg, &rssi, sizeof(uint32_t)))
                return -EFAULT;

            break;
        }
        
        case FM_IOCTL_RW_REG:
        {
            struct fm_ctl_parm parm_ctl;
            FM_DEBUG("FM_IOCTL_RW_REG\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (copy_from_user(&parm_ctl, (void*)arg, sizeof(struct fm_ctl_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            
            if(parm_ctl.rw_flag == 0) //write
            {
                ret = AR1000_write(fm->i2c_client, parm_ctl.addr, parm_ctl.val);
            }
            else
            {
                ret = AR1000_read(fm->i2c_client, parm_ctl.addr, &parm_ctl.val);
            }
            
            up(&fm_ops_mutex);
            if ((parm_ctl.rw_flag == 0x01) && (!ret)) // Read success.
            { 
                if (copy_to_user((void*)arg, &parm_ctl, sizeof(struct fm_ctl_parm)))
                    return -EFAULT;
            }
            break;
        }

        case FM_IOCTL_GETCHIPID:
        {
            uint16_t chipid;            

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
                
            //ret = AR1000_read(fm->i2c_client, AR1000_CHIPID, &chipid);
            chipid = fm->chip_id;
            FM_DEBUG("FM_IOCTL_GETCHIPID:%04x\n", chipid);      

            up(&fm_ops_mutex);
            
            if (copy_to_user((void*)arg, &chipid, sizeof(uint16_t)))
                return -EFAULT;
                            
            break;
        }

        case FM_IOCTL_EM_TEST:
        {
            struct fm_em_parm parm_em;
            FM_DEBUG("FM_IOCTL_EM_TEST\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (copy_from_user(&parm_em, (void*)arg, sizeof(struct fm_em_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
           
            AR1000_em_test(fm->i2c_client, parm_em.group_idx, parm_em.item_idx, parm_em.item_value);
            
            up(&fm_ops_mutex);
            
            break;
        }
		case FM_IOCTL_IS_FM_POWERED_UP:
		{
			FM_DEBUG("FM_IOCTL_IS_FM_POWERED_UP");
			uint32_t powerup;
			if (fm->powerup) {
				powerup = 1;
			} else {
				powerup = 0;
			}
			if (copy_to_user((void*)arg, &powerup, sizeof(uint32_t)))
                return -EFAULT;
			break;
		}

        #ifdef FMDEBUG
        case FM_IOCTL_DUMP_REG:
        {
            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            AR1000_dump_reg(fm->i2c_client);
            up(&fm_ops_mutex);

            break;
        }
        #endif

        default:
            return -EPERM;
    }

    return ret;
}

static int fm_ops_open(struct inode *inode, struct file *filp)
{
    struct fm *fm = container_of(inode->i_cdev, struct fm, cdev);

    FM_DEBUG("%s\n", __func__);

    if (down_interruptible(&fm_ops_mutex))
        return -EFAULT;
        
    // TODO: only have to set in the first time?
	// YES!!!!

    fm->ref++;

	if (fm->ref == 1) {
    	mt_set_gpio_mode(GPIO_FM_CLK_PIN, GPIO_FM_CLK_PIN_M_CLK);    
    	mt_set_clock_output(GPIO_FM_CLK_PIN_CLK, GPIO_FM_CLK_PIN_FREQ);
	}

    up(&fm_ops_mutex);

    filp->private_data = fm;

    // TODO: check open flags

    return 0;
}

static int fm_ops_release(struct inode *inode, struct file *filp)
{
    int err = 0;
    struct fm *fm = container_of(inode->i_cdev, struct fm, cdev);

    FM_DEBUG("%s\n", __func__);

    if (down_interruptible(&fm_ops_mutex))
        return -EFAULT;
    fm->ref--;
    if(fm->ref < 1) {
        if(fm->powerup == true) {
            fm_powerdown(fm);           
        }
		mt_set_gpio_mode(GPIO_FM_CLK_PIN, GPIO_MODE_00);
    	mt_set_gpio_dir(GPIO_FM_CLK_PIN, GPIO_DIR_OUT);
    	mt_set_gpio_out(GPIO_FM_CLK_PIN, 0);
    }
    
        up(&fm_ops_mutex);

    return err;
}

static int fm_init(struct i2c_client *client)
{
    int err;
    struct fm *fm = NULL;

    if (!(fm = kzalloc(sizeof(struct fm), GFP_KERNEL)))
    {
        FM_ALERT("-ENOMEM\n");
        err = -ENOMEM;
        goto ERR_EXIT;
    }

    fm->ref = 0;
    fm->powerup = false;
    fm->chip_id = 0x1000;
	g_fm_struct = fm;

    if ((err = fm_setup_cdev(fm)))
    {
        goto ERR_EXIT;
    }

    fm->i2c_client = client;
    i2c_set_clientdata(client, fm);

    // Switch AR1000 to standby mode for power saving
    fm_powerdown(fm);

	/***********Add porc file system*************/

	g_fm_proc = create_proc_entry(FM_PROC_FILE, 0444, NULL);
	if (g_fm_proc == NULL) {
		FM_ALERT("create_proc_entry failed\n");
		err = -ENOMEM;
		goto ERR_EXIT;
	} else {
		g_fm_proc->read_proc = fm_proc_read;
		g_fm_proc->write_proc = NULL;
		//g_fm_proc->owner = THIS_MODULE;
		FM_ALERT("create_proc_entry success\n");
	}

	/********************************************/
    
    FM_DEBUG("fm_powerdown in fm_init\n");
    
    return 0;

ERR_EXIT:
    kfree(fm);

    return err;
}

static int fm_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int cnt= 0;
	FM_ALERT("Enter fm_proc_read.\n");
	if(off != 0)
		return 0;
	struct fm *fm  = g_fm_struct;
	if (fm != NULL && fm->powerup) {
		cnt = sprintf(page, "1\n");
	} else {
		cnt = sprintf(page, "0\n");
	}
	*eof = 1;
	FM_ALERT("Leave fm_proc_read. cnt = %d\n", cnt);
	return cnt;
}


static int fm_destroy(struct fm *fm)
{
    int err = 0;

    FM_DEBUG("%s\n", __func__);

    device_destroy(fm->cls, fm->dev_t);
    class_destroy(fm->cls);

    cdev_del(&fm->cdev);
    unregister_chrdev_region(fm->dev_t, 1);

    fm_powerdown(fm);

	/***********************************/
	remove_proc_entry(FM_PROC_FILE, NULL);
	
	/**********************************/

    // FIXME: any other hardware configuration ?

    // free all memory
    kfree(fm);

    return err;
}

/*
 *  fm_powerup
 */
static int fm_powerup(struct fm *fm, struct fm_tune_parm *parm)
{
    int i;
    struct i2c_client *client = fm->i2c_client;

    if (fm->powerup)
    {
        parm->err = FM_BADSTATUS;
        return -EPERM;
    }

//  fm_powerdown(client); // FIXME: is this necessary ?

    // FIXME: check if already powerup

    // FIXME: the default settings may be refined
    for (i = 1; i < 18; i++)
        (void)AR1000_write(client, i, AR1000_DATA[i]);

    (void)AR1000_write(client, AR1000_R0, AR1000_DATA[AR1000_R0]);

    if (0 != AR1000_wait_stc(client, AR1000_MAX_WAIT_CNT))
    {
        FM_ALERT("pwron failed\n");
        parm->err = FM_FAILED;

        return -EPERM;
    }
    
    (void)AR1000_read(client, AR1000_CHIPID, &fm->chip_id);
    
    if (fm->chip_id != 0x1000 && fm->chip_id != 0x1010)
    {
        FM_ALERT("unknown chipid 0x%x\n", fm->chip_id);
    }

    (void)AR1000_read(client, 0x1B, &fm->device_id);

    FM_DEBUG("pwron ok\n");
    fm->powerup = true;

    if (0 != fm_tune(fm, parm))
    {
        return -EPERM;
    }

    parm->err = FM_SUCCESS;

    return 0;
}

/*
 *  fm_powerdown
 */
static int fm_powerdown(struct fm *fm)
{
    struct i2c_client *client = fm->i2c_client;

    (void)AR1000_write(client, AR1000_R0, 0xFF7A);

    fm->powerup = false;
    FM_ALERT("pwrdown ok\n");

    return 0;
}


/*
 *  fm_seek
 */
static int fm_seek(struct fm *fm, struct fm_seek_parm *parm)
{
    int ret = 0;
    uint16_t val = 0;
    struct i2c_client *client = fm->i2c_client;

    if (!fm->powerup)
    {
        parm->err = FM_BADSTATUS;
        return -EPERM;
    }

    if (parm->space == FM_SPACE_100K)
        val |= 0x2000;
    else if (parm->space == FM_SPACE_200K)
        val |= 0x0000;
    else
    {
        parm->err = FM_EPARM;
        return -EPERM;
    }

    if (parm->band == FM_BAND_UE) {
        val |= 0x0000;
        fm->min_freq = 875;
        fm->max_freq = 1080;
    }
    else if (parm->band == FM_BAND_JAPAN) {
        val |= 0x1000;
        fm->min_freq = 760;
        fm->max_freq = 900;
    }
    else if (parm->band == FM_BAND_JAPANW) {
        val |= 0x1800;
        fm->min_freq = 760;
        fm->max_freq = 1080;
    }
    else
    {
        FM_ALERT("band:%d out of range\n", parm->band);
        parm->err = FM_EPARM;
        return -EPERM;
    }

    if (parm->freq < fm->min_freq || parm->freq > fm->max_freq) {
        FM_ALERT("freq:%d out of range\n", parm->freq);
        parm->err = FM_EPARM;
        return -EPERM;
    }

    if (parm->seekdir == FM_SEEK_UP)
        val |= 0x8000;

    if (parm->seekth > 0x7F) {
        FM_ALERT("seekth:%d out of range\n", parm->seekth);
        parm->err = FM_EPARM;
        return -EPERM;
    }
    val |= parm->seekth;

#ifdef FMDEBUG
    if (parm->seekdir == FM_SEEK_UP)
        FM_DEBUG("seek %d up\n", parm->freq);
    else
        FM_DEBUG("seek %d down\n", parm->freq);
#endif

    // (1) set hmute bit
    AR1000_enable_hmute(client);
    // (2) clear tune bit
    AR1000_clear_tune(client);
    // (3) set chan bits
    AR1000_set_bits(client, AR1000_R2, (parm->freq - 690), 0x1FF);
    // (4) clear seek bit
    AR1000_clear_seek(client);
    // (5) seekup/space/band/seekth bits
    AR1000_set_bits(client, AR1000_R3, val, 0xB87F);

    // (6) enable seek bit
    AR1000_enable_seek(client);
    // (7) wait stc
    ret = AR1000_wait_stc(client, AR1000_MAX_WAIT_CNT);
    // TODO: (8) auto Hi/Lo 

    // (9) clear hmute
    AR1000_clear_hmute(client);

    // (10) update functions
    if (ret == 0) // seek successfully
    {
        ret = AR1000_read(client, AR1000_STATUS, &val);
        if (ret)
        {
            FM_ALERT("fm_seek AR1000_STATUS read failed\n");
            parm->err = FM_SEEK_FAILED;
	        return ret;
	    }
            
        parm->freq = ((val >> 7)&0x01FF) + 690;
        if (val & AR1000_MASK_SF)
        {
            FM_ALERT("fm_seek failed, invalid freq\n");
            parm->err = FM_SEEK_FAILED;
        }
        else
        {
            FM_ALERT("fm_seek success, freq:%d\n", parm->freq);
            parm->err = FM_SUCCESS;
        }
    }

    return ret;
}

static int fm_setvol(struct fm *fm, uint32_t vol)
{
    int ret = 0;
    struct i2c_client *client = fm->i2c_client;

    if (vol > 15)
        vol = 15;

    FM_DEBUG("fm_setvol:%d\n", vol);

    ret = AR1000_set_bits(client, AR1000_R3, (g_vol_level[vol].vol << 7), AR1000_MASK_VOL );
	ret = AR1000_set_bits(client, AR1000_R14, (g_vol_level[vol].vol2 << 12), AR1000_MASK_VOL2 );
    if (ret)
        return -EPERM;

    return 0;
}

static int fm_getvol(struct fm *fm, uint32_t *vol)
{
    int ret = 0;
    uint16_t val;
    struct i2c_client *client = fm->i2c_client;

    ret = AR1000_read(client, AR1000_R3, &val);
    if (ret)
        return -EPERM;

    *vol = (uint32_t)((val & AR1000_MASK_VOL) >> 7);

    *vol = 15 - *vol;

    return 0;
}

static int fm_getrssi(struct fm *fm, uint32_t *rssi)
{
    int ret = 0;
    uint16_t val;
    struct i2c_client *client = fm->i2c_client;

    ret = AR1000_read(client, AR1000_RSSI, &val);
    if (ret)
        return -EPERM;

    val = (uint32_t)((val & AR1000_MASK_RSSI) >> 9);

    FM_DEBUG("rssi value:%d\n", val);
    *rssi = val+6; //dBuV=dBm+113    
    /*
    if (val > 0x30)
	    *rssi = 4;
    else if (val > 0x20)
	    *rssi = 3;
    else if (val > 0x10)
	    *rssi = 2;
    else
	    *rssi = 1;
    FM_DEBUG("rssi level:%d\n", *rssi);
    */
    return 0;
}

/*
 *  fm_tune
 */
static int fm_tune(struct fm *fm, struct fm_tune_parm *parm)
{
    int ret;
    uint16_t val;
    struct i2c_client *client = fm->i2c_client;

    FM_DEBUG("%s\n", __func__);

    if (!fm->powerup)
    {
        parm->err = FM_BADSTATUS;
        return -EPERM;
    } 

    if (parm->space == FM_SPACE_100K)
        val = 0x2000;
    else if (parm->space == FM_SPACE_200K)
        val = 0x0000;
    else
    {
        parm->err = FM_EPARM;
        return -EPERM;
    }

    if (parm->band == FM_BAND_UE) {
        val |= 0x0000;
        fm->min_freq = 875;
        fm->max_freq = 1080;
    }
    else if (parm->band == FM_BAND_JAPAN) {
        val |= 0x1000;
        fm->min_freq = 760;
        fm->max_freq = 900;
    }
    else if (parm->band == FM_BAND_JAPAN) {
        val |= 0x1800;
        fm->min_freq = 760;
        fm->max_freq = 1080;
    }
    else
    {
        parm->err = FM_EPARM;
        return -EPERM;
    }        

    if (unlikely(parm->freq < fm->min_freq || parm->freq > fm->max_freq)) {
        parm->err = FM_EPARM;
        return -EPERM;
    }

    // (1)
    AR1000_enable_hmute(client);

//    AR1000_write(client, AR1000_R17, AR1000_DATA[AR1000_R17]); // FIXME, what's this?

    // (2)
    AR1000_clear_tune(client);

    // (3)
    AR1000_clear_seek(client);

    // (4) Set BAND/SPACE and CHAN
    AR1000_set_bits(client, AR1000_R3, val, 0x3800);    
    AR1000_set_bits(client, AR1000_R2, (parm->freq - 690), 0x1FF);
    FM_DEBUG("fm_tune, freq:%d\n", parm->freq);

    //TODO: (5) (6) (7) autoHiLo

    // (8)
    AR1000_enable_tune(client);

    // (9)
    ret = AR1000_wait_stc(client, AR1000_MAX_WAIT_CNT);

    // (10)
    AR1000_clear_hmute(client);

    // TODO: (11) update functions, read readchan
    if (ret == 0)
    {
        ret = AR1000_read(client, AR1000_STATUS, &val);
        if (ret)
        {
            FM_ALERT("fm_tune AR1000_STATUS read failed\n");
            parm->err = FM_SEEK_FAILED;
	        return ret;
	    }
	    
	    if (val & AR1000_MASK_STC)
	    {
	        parm->freq = ((val >> 7)&0x01FF) + 690;
            parm->err = FM_SUCCESS;
            FM_DEBUG("fm_tune sucess, freq:%d\n", parm->freq);
            return 0;
        }
        else
        {
            FM_ALERT("fm_tune STC done failed\n");
            parm->err = FM_SEEK_FAILED;
	        return -EPERM;
        }
    }
    // FIXME! FIXME! FIXME! FIXME! FIXME! FIXME!
    //_open_audio_path();

    return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
/*
 *  fm_i2c_attach_adapter
 */
static int fm_i2c_attach_adapter(struct i2c_adapter *adapter)
{
    int err = 0;

    if (adapter->id == AR1000_I2C_PORT)
    {
        return i2c_probe(adapter, &AR1000_addr_data, fm_i2c_detect);
    }

    return err;
}

/*
 *  fm_i2c_detect
 *  This function is called by i2c_detect
 */
static int fm_i2c_detect(struct i2c_adapter *adapter, int addr, int kind)
{
    int err;
    struct i2c_client *client = NULL;

    /* skip this since MT6516 shall support all the needed functionalities
    if (!i2c_check_functionality(adapter, xxx))
    {
        FM_DEBUG("i2c_check_functionality failed\n");
        return -ENOTSUPP;
    }
    */

    /* initial i2c client */
    if (!(client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL)))
    {
        FM_ALERT("kzalloc failed\n");
        err = -ENOMEM;
        goto ERR_EXIT;
    }

    client->addr = addr;
    client->adapter = adapter;
    client->driver = &AR1000_driver;
    client->flags = 0;
    strncpy(client->name, "AR1000 FM RADIO", I2C_NAME_SIZE);

    if ((err = fm_init(client)))
    {
        FM_ALERT("fm_init ERR:%d\n", err);
        goto ERR_EXIT;
    }

    if (err = i2c_attach_client(client))
    {
        FM_ALERT("i2c_attach_client ERR:%d\n", err);
        goto ERR_EXIT;
    }

    return 0;

ERR_EXIT:
    kfree(client);

    return err;
}

static int fm_i2c_detach_client(struct i2c_client *client)
{
    int err = 0;
    struct fm *fm = i2c_get_clientdata(client);

    FM_DEBUG("fm_i2c_detach_client\n");

    err = i2c_detach_client(client);
    if (err)
    {
        dev_err(&client->dev, "fm_i2c_detach_client failed\n");
        return err;
    }

    fm_destroy(fm);
    kfree(client);

    return err;
}
#else
static int fm_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int err;
    FM_DEBUG("fm_i2c_probe\n");    
    if ((err = fm_init(client)))
    {
        FM_ALERT("fm_init ERR:%d\n", err);
        goto ERR_EXIT;
    }   
    
    return 0;   
    
ERR_EXIT:
    return err;    
}

static int fm_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    FM_DEBUG("fm_i2c_detect\n");
    strcpy(info->type, AR1000_DEV);
    return 0;
}

static int fm_i2c_remove(struct i2c_client *client)
{
    int err = 0;
    struct fm *fm = i2c_get_clientdata(client);
    
    FM_DEBUG("fm_i2c_remove\n");
    if(fm)
    {    
        fm_destroy(fm);
        fm = NULL;
    }
    
    return err;
}
#endif

static int mt_fm_probe(struct platform_device *pdev)
{
    int err = -1;
    FM_ALERT("mt_fm_probe\n");
	
    // Open I2C driver
    err = i2c_add_driver(&AR1000_driver);
    if (err)
    {
        FM_ALERT("i2c err\n");
    }
      
    return err;   
} 
    
static int mt_fm_remove(struct platform_device *pdev)
{
    FM_ALERT("mt_fm_remove\n");
    i2c_del_driver(&AR1000_driver); 
    
    return 0; 
}


static struct platform_driver mt_fm_dev_drv =
{
    .probe   = mt_fm_probe,
    .remove  = mt_fm_remove,
#if 0//def CONFIG_PM //Not need now   
    .suspend = mt_fm_suspend,
    .resume  = mt_fm_resume,
#endif    
    .driver = {
        .name   = FM_NAME,
        .owner  = THIS_MODULE,    
    }
};

/*
 *  mt_fm_init
 */
static int __init mt_fm_init(void)
{
	int err = 0;

	FM_ALERT("mt_fm_init\n");
	err = platform_driver_register(&mt_fm_dev_drv);
    if (err)
    {
        FM_ALERT("platform_driver_register failed\n");
    }
    
	return err;
}

/*
 *  mt_fm_exit
 */
static void __exit mt_fm_exit(void)
{
    FM_ALERT("mt_fm_exit\n");
    platform_driver_unregister(&mt_fm_dev_drv);
}

module_init(mt_fm_init);
module_exit(mt_fm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek FM Driver");
MODULE_AUTHOR("William Chung <William.Chung@MediaTek.com>");

