#if defined(MTK_MULTIBRIDGE_SUPPORT)
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <mach/mt_gpio.h>




#include "mt8193.h"
#include "mt8193_ckgen.h"
#include "mt8193_gpio.h"



#define MT8193_CKGEN_VFY 1

#define MT8193_CKGEN_DEVNAME "mt8193-ckgen"


static int mt8193_ckgen_probe(struct platform_device *pdev);
static int mt8193_ckgen_suspend(struct platform_device *pdev, pm_message_t state);
static int mt8193_ckgen_resume(struct platform_device *pdev);
static int __devexit mt8193_ckgen_remove(struct platform_device *pdev);

u32 mt8193_ckgen_measure_clk(u32 u4Func);

/* config gpio */
int mt8193_ckgen_gpio_config(int i4GpioNum, int i4Output, int i4High);

/* print gpio input value */
int mt8193_ckgen_gpio_input(int i4GpioNum);


/* test pad level shift */
int mt8193_ckgen_config_pad_level_shift(int i4GroupNum, int i4TurnLow);

/* read and print chip id */
void mt8193_ckgen_chipid(void);


/* measure clock with freq meter */
u32 mt8193_ckgen_measure_clk(u32 u4Func);

u32 mt8193_ckgen_reg_rw_test(u16 addr);

void mt8193_lvds_sys_spm_control(bool power_on);

void mt8193_hdmi_sys_spm_control(bool power_on);

void mt8193_nfi_sys_spm_control(bool power_on);

void mt8193_lvds_ana_pwr_control(bool power_on);

void mt8193_hdmi_ana_pwr_control(bool power_on);

void mt8193_pllgp_ana_pwr_control(bool power_on);

void mt8193_nfi_ana_pwr_control(bool power_on);


extern void mt8193_bus_clk_switch(bool bus_26m_to_32k);




/******************************************************************************
Device driver structure
******************************************************************************/
static struct platform_driver mt8193_ckgen_driver = {
    .probe      = mt8193_ckgen_probe,
    .remove     = mt8193_ckgen_remove,
    .suspend    = mt8193_ckgen_suspend,
    .resume     = mt8193_ckgen_resume,
    .driver     = {
        .name   = "mt8193-ckgen",
        .owner  = THIS_MODULE,
    },
};

static struct class *ckgen_class = NULL;
static struct cdev *ckgen_cdev;



#if MT8193_CKGEN_VFY

static int mt8193_ckgen_release(struct inode *inode, struct file *file);
static int mt8193_ckgen_open(struct inode *inode, struct file *file);
static long mt8193_ckgen_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


extern void mt8193_spm_control_test(int u4Func);


struct file_operations mt8193_ckgen_fops = {
    .owner   = THIS_MODULE,
    .unlocked_ioctl   = mt8193_ckgen_ioctl,
    .open    = mt8193_ckgen_open,
    .release = mt8193_ckgen_release,
};
#endif

extern void msleep(unsigned int msecs);



#if defined(CONFIG_HAS_EARLYSUSPEND)
static void mt8193_ckgen_early_suspend(struct early_suspend *h)
{
    printk("[CKGEN] mt8193_ckgen_early_suspend() enter\n");

#if 0
    // turn off lvds digital
    mt8193_lvds_sys_spm_control(false);
    msleep(5);
    // turn off lvds analog
    mt8193_lvds_ana_pwr_control(false);
    msleep(5);
    // turn off hdmi ana
    //mt8193_hdmi_ana_pwr_control(false);
    msleep(5);
    // turn off hdmi digital
    //mt8193_hdmi_sys_spm_control(false);
    msleep(5);
    // turn off nfi digital
    //mt8193_nfi_sys_spm_control(false);
    msleep(5);
    // turn off nfi analog
    //mt8193_nfi_ana_pwr_control(false);
    msleep(5);
    // turn off pllgp analog
    mt8193_pllgp_ana_pwr_control(false);
    msleep(5);
    // bus clk switch to 32K
    mt8193_bus_clk_switch(true);
#endif

#if 0
    /* add 8193 suspend function here */

    mt8193_pllgp_ana_pwr_control(false);
    //msleep(5);
    // bus clk switch to 32K
    mt8193_bus_clk_switch(true);
#endif


    printk("[CKGEN] mt8193_ckgen_early_suspend() exit\n");
}

static void mt8193_ckgen_late_resume(struct early_suspend *h)
{
    printk("[CKGEN] mt8193_ckgen_late_resume() enter\n");
#if 0
    // bus clk switch to 26M
    mt8193_bus_clk_switch(false);
    msleep(5);
    // turn on pllgp analog
    mt8193_pllgp_ana_pwr_control(true);
    msleep(5);
    // turn on nfi analog
   // mt8193_nfi_ana_pwr_control(true);
    msleep(5);
    // turn on nfi digital
    //mt8193_nfi_sys_spm_control(true);
    msleep(5);
    // turn on hdmi digital
    //mt8193_hdmi_sys_spm_control(true);
    msleep(5);
    // turn on hdmi ana
    //mt8193_hdmi_ana_pwr_control(true);
    msleep(5);
    // turn on lvds analog
    mt8193_lvds_ana_pwr_control(true);
    msleep(5);
    // turn on lvds digital
    mt8193_lvds_sys_spm_control(true);
    #endif

#if 0
    /* add 8193 resume function here */
    // bus clk switch to 26M
    mt8193_bus_clk_switch(false);
    msleep(2);
    // turn on pllgp analog
    mt8193_pllgp_ana_pwr_control(true);
    msleep(2);
#endif

    printk("[CKGEN] mt8193_ckgen_early_resume() exit\n");
}

static struct early_suspend mt8193_ckgen_early_suspend_desc = {
    .level      = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
    .suspend    = mt8193_ckgen_early_suspend,
    .resume     = mt8193_ckgen_late_resume,
};
#endif


#if MT8193_CKGEN_VFY
static int mt8193_ckgen_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int mt8193_ckgen_open(struct inode *inode, struct file *file)
{
    return 0;
}

static char* _mt8193_ckgen_ioctl_spy(unsigned int cmd)
{
    switch(cmd)
    {
        case MTK_MT8193_CKGEN_1:
            return "MTK_MT8193_CKGEN_1";
        case MTK_MT8193_CKGEN_2:
            return "MTK_MT8193_CKGEN_2";
        case MTK_MT8193_CKGEN_SPM_CTRL:
            return "MTK_MT8193_CKGEN_SPM_CTRL";
        case MTK_MT8193_CKGEN_LS_TEST:
            return "MTK_MT8193_CKGEN_LS_TEST";
        case MTK_MT8193_CKGEN_FREQ_METER:
            return "MTK_MT8193_CKGEN_FREQ_METER";
        default:
            return "unknown ioctl command";
    }
}


static long mt8193_ckgen_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;

    int r = 0;

    printk("[CKGEN] cmd=%s, arg=0x%08lx\n", _mt8193_ckgen_ioctl_spy(cmd), arg);

    switch(cmd)
    {
        case MTK_MT8193_CKGEN_1:
        {
           

            break;
        }
        case MTK_MT8193_CKGEN_2:
        {

            break;
        }

        case MTK_MT8193_CKGEN_LS_TEST:
        {
            MT8193_CKGEN_LS_INFO_T tLsInfo;
            
            if (copy_from_user(&tLsInfo, (void __user *)arg, sizeof(tLsInfo)))
            {
                printk("[CKGEN] copy_from_user fails!!\n");
                return -1;
            }

            r = mt8193_ckgen_config_pad_level_shift(tLsInfo.i4GroupNum, tLsInfo.i4TurnLow);

            break;
        }

        case MTK_MT8193_CKGEN_SPM_CTRL:
        {
            int u4Func = 0;
            u4Func = arg;
            mt8193_spm_control_test(u4Func);

            break;
        }

        case MTK_MT8193_CKGEN_FREQ_METER:
        {
            MT8193_CKGEN_FREQ_METER_T t_freq;
            u32 u4Clk = 0;
            

            if (copy_from_user(&t_freq, (void __user *)arg, sizeof(t_freq)))
            {
                printk("[CKGEN] copy_from_user fails!!\n");
                return -1;
            }
                
            u4Clk = mt8193_ckgen_measure_clk(t_freq.u4Func);

            break;
        }

        case MTK_MT8193_GPIO_CTRL:
        {
            MT8193_GPIO_CTRL_T t_gpio;
            u32 u4Val = 0;
            

            if (copy_from_user(&t_gpio, (void __user *)arg, sizeof(t_gpio)))
            {
                printk("[CKGEN] copy_from_user fails!!\n");
                return -1;
            }

            if (t_gpio.u4Mode == MT8193_GPIO_OUTPUT)
            {
                GPIO_Output(t_gpio.u4GpioNum, t_gpio.u4Value);
            }
            else
            {
                GPIO_Config(t_gpio.u4GpioNum, MT8193_GPIO_INPUT, 0);
                u4Val = GPIO_Input(t_gpio.u4GpioNum);
                printk("[CKGEN] GPIO INPUT VALUE IS %d\n", u4Val);
            }

            break;
        }

        case MTK_MT8193_EARLY_SUSPEND:
        {
            
            break;
        }

        case MTK_MT8193_LATE_RESUME:
        {
            
            break;
        }
        
        
        default:
        {
            printk("[CKGEN] arguments error\n");
            break;
        }
    }

    return r;
}

#endif



/******************************************************************************
 * mt8193_ckgen_init
 * 
 * DESCRIPTION: 
 *   Init the device driver ! 
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
static int __init mt8193_ckgen_init(void)
{
    printk("[CKGEN] mt8193_ckgen_init() enter\n");

    int ret = 0;
    ret = platform_driver_register(&mt8193_ckgen_driver);

#if defined(CONFIG_HAS_EARLYSUSPEND)
    register_early_suspend(&mt8193_ckgen_early_suspend_desc);
#endif
    
    printk("[CKGEN] mt8193_ckgen_init() exit\n");

    return (ret);
}

/******************************************************************************
 * mt8193_ckgen_exit
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
static void __exit mt8193_ckgen_exit(void)
{
    platform_driver_unregister(&mt8193_ckgen_driver);
}

static dev_t mt8193_ckgen_devno;
static struct cdev *mt8193_ckgen_cdev;


/******************************************************************************
 * mt8193_ckgen_probe
 * 
 * DESCRIPTION:
 *   register the nand device file operations ! 
 * 
 * PARAMETERS: 
 *   struct platform_device *pdev : device structure
 * 
 * RETURNS: 
 *   0 : Success   
 * 
 * NOTES: 
 *   None
 *
 ******************************************************************************/
static int mt8193_ckgen_probe(struct platform_device *pdev)
{
    printk("[CKGEN] %s\n", __func__);
    
#if MT8193_CKGEN_VFY
    int ret = 0;
    
    /* Allocate device number for hdmi driver */
    ret = alloc_chrdev_region(&mt8193_ckgen_devno, 0, 1, MT8193_CKGEN_DEVNAME);
    if(ret)
    {
        printk("[CKGEN] alloc_chrdev_region fail\n");
        return -1;
    }

#if 0   
    // enable bus clock monitor
    CKGEN_WRITE32(0x48, 0x4);
    CKGEN_WRITE32(0x220, 0x400000);
    CKGEN_WRITE32(0x310, 0x846e002);
#endif

    /* For character driver register to system, device number binded to file operations */
    mt8193_ckgen_cdev = cdev_alloc();
    mt8193_ckgen_cdev->owner = THIS_MODULE;
    mt8193_ckgen_cdev->ops = &mt8193_ckgen_fops;
    ret = cdev_add(mt8193_ckgen_cdev, mt8193_ckgen_devno, 1);

    /* For device number binded to device name(hdmitx), one class is corresponeded to one node */
    ckgen_class = class_create(THIS_MODULE, MT8193_CKGEN_DEVNAME);
    /* mknod /dev/hdmitx */
    ckgen_cdev = (struct class_device *)device_create(ckgen_class, NULL, mt8193_ckgen_devno,    NULL, MT8193_CKGEN_DEVNAME);
    
#endif  
    return 0;
}

/******************************************************************************
 * mt8193_ckgen_remove
 * 
 * DESCRIPTION:
 *   unregister the nand device file operations ! 
 * 
 * PARAMETERS: 
 *   struct platform_device *pdev : device structure
 * 
 * RETURNS: 
 *   0 : Success
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/

static int __devexit mt8193_ckgen_remove(struct platform_device *pdev)
{
    
    return 0;
}


module_init(mt8193_ckgen_init);
module_exit(mt8193_ckgen_exit);

int mt8193_CKGEN_AgtOnClk(e_CLK_T eAgt)
{
    u32 u4Tmp;

    printk("mt8193_CKGEN_AgtOnClk() %d\n", eAgt);

        
    switch (eAgt)
    {
       case e_CLK_NFI:
            u4Tmp = CKGEN_READ32(REG_RW_NFI_CKCFG);
            CKGEN_WRITE32(REG_RW_NFI_CKCFG, u4Tmp & (~CLK_PDN_NFI));
            break;
        case e_CLK_HDMIPLL:
            u4Tmp = CKGEN_READ32(REG_RW_HDMI_PLL_CKCFG);
            CKGEN_WRITE32(REG_RW_HDMI_PLL_CKCFG, u4Tmp & (~CLK_PDN_HDMI_PLL));
            break;
          case e_CLK_HDMIDISP:
            u4Tmp = CKGEN_READ32(REG_RW_HDMI_DISP_CKCFG);
            CKGEN_WRITE32(REG_RW_HDMI_DISP_CKCFG, u4Tmp & (~CLK_PDN_HDMI_DISP));
            break;
          case e_CLK_LVDSDISP:
            u4Tmp = CKGEN_READ32(REG_RW_LVDS_DISP_CKCFG);
            CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp & (~CLK_PDN_LVDS_DISP));
            break;
          case e_CLK_LVDSCTS:
            u4Tmp = CKGEN_READ32(REG_RW_LVDS_CTS_CKCFG);
            CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp & (~CLK_PDN_LVDS_CTS));
            break;
        default:
            return -1;
    }
        
       
    return 0;
}


int mt8193_CKGEN_AgtOffClk(e_CLK_T eAgt)
{
      u32 u4Tmp;

      printk("mt8193_CKGEN_AgtOffClk() %d\n", eAgt);

    switch (eAgt)
    {
        case e_CLK_NFI:
            u4Tmp = CKGEN_READ32(REG_RW_NFI_CKCFG);
            CKGEN_WRITE32(REG_RW_NFI_CKCFG, u4Tmp | CLK_PDN_NFI);
            break;
        case e_CLK_HDMIPLL:
            u4Tmp = CKGEN_READ32(REG_RW_HDMI_PLL_CKCFG);
            CKGEN_WRITE32(REG_RW_HDMI_PLL_CKCFG, u4Tmp | CLK_PDN_HDMI_PLL);
            break;
        case e_CLK_HDMIDISP:
            u4Tmp = CKGEN_READ32(REG_RW_HDMI_DISP_CKCFG);
            CKGEN_WRITE32(REG_RW_HDMI_DISP_CKCFG, u4Tmp | CLK_PDN_HDMI_DISP);
            break;
        case e_CLK_LVDSDISP:
            u4Tmp = CKGEN_READ32(REG_RW_LVDS_DISP_CKCFG);
            CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | CLK_PDN_LVDS_DISP);
            break;
        case e_CLK_LVDSCTS:
            u4Tmp = CKGEN_READ32(REG_RW_LVDS_CTS_CKCFG);
            CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | CLK_PDN_LVDS_CTS);
            break;
        default:
            return -1;
    }
    return 0;
}

int mt8193_CKGEN_AgtSelClk(e_CLK_T eAgt, u32 u4Sel)
{
      u32 u4Tmp;

      printk("mt8193_CKGEN_AgtSelClk() %d\n", eAgt);

      switch (eAgt)
      {
          case e_CLK_NFI:
              u4Tmp = CKGEN_READ32(REG_RW_NFI_CKCFG);
                CKGEN_WRITE32(REG_RW_NFI_CKCFG, u4Tmp | u4Sel);
              break;
          case e_CLK_HDMIPLL:
              u4Tmp = CKGEN_READ32(REG_RW_HDMI_PLL_CKCFG);
                CKGEN_WRITE32(REG_RW_HDMI_PLL_CKCFG, u4Tmp | u4Sel);
              break;
            case e_CLK_HDMIDISP:
              u4Tmp = CKGEN_READ32(REG_RW_HDMI_DISP_CKCFG);
                CKGEN_WRITE32(REG_RW_HDMI_DISP_CKCFG, u4Tmp | u4Sel);
              break;
            case e_CLK_LVDSDISP:
                u4Tmp = CKGEN_READ32(REG_RW_LVDS_DISP_CKCFG);
                CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | u4Sel);
              break;
            case e_CLK_LVDSCTS:
              u4Tmp = CKGEN_READ32(REG_RW_LVDS_CTS_CKCFG);
                CKGEN_WRITE32(REG_RW_LVDS_DISP_CKCFG, u4Tmp | u4Sel);
              break;
          default:
              return -1;
      }
       
    return 0;
}

u32 mt8193_CKGEN_AgtGetClk(e_CLK_T eAgt)
{
    printk("mt8193_CKGEN_AgtGetClk() %d\n", eAgt);
    return 0;
}

int mt8193_ckgen_i2c_write(u16 addr, u32 data)
{
    u32 u4_ret = 0;
    
    printk("mt8193_ckgen_i2c_write() 0x%x; 0x%lx\n", addr, data);
    u4_ret = mt8193_i2c_write(addr, data);
    if (u4_ret != 0)
    {
        printk("mt8193_i2c_read() fails!!!!!!\n");
    }
    return 0;
}

u32 mt8193_ckgen_i2c_read(u16 addr)
{
    u32 u4_val = 0;
    u32 u4_ret = 0;
    printk("mt8193_ckgen_i2c_read() 0x%x; 0x%lx\n", addr, u4_val);
    u4_ret = mt8193_i2c_read(addr, &u4_val);
    if (u4_ret != 0)
    {
        printk("mt8193_i2c_read() fails!!!!!!\n"); 
    }
    printk("mt8193_ckgen_i2c_read() 0x%x; value is 0x%lx\n", addr, u4_val);
    return u4_val;
}

/* freq meter measure clock */
u32 mt8193_ckgen_measure_clk(u32 u4Func)
{
    u32 ui4_delay_cnt = 0x400;
    u32 ui4_result = 0;

    printk("[CKGEN] mt8193_ckgen_measure_clk() %d\n", u4Func);

    /* select source */
    CKGEN_WRITE32(REG_RW_FMETER, (CKGEN_READ32(REG_RW_FMETER)&(~(0xFF<<3)))|(u4Func<<3));
    /* start fmeter */
    CKGEN_WRITE32(REG_RW_FMETER, (CKGEN_READ32(REG_RW_FMETER)|CKGEN_FMETER_RESET));
    /* wait until fmeter done */
    do
    {
        ui4_delay_cnt--;
    }while( (!(CKGEN_READ32(REG_RW_FMETER)&CKGEN_FMETER_DONE)) && ui4_delay_cnt );
    
    ui4_result = CKGEN_READ32(REG_RW_FMETER)>>16;

    printk("[CKGEN] Measure Done CLK [0X%X] for func [%d] delay count [0x%X]\n", ui4_result, u4Func, ui4_delay_cnt);
    
    return (ui4_result);
}


void mt8193_lvds_ana_pwr_control(bool power_on)
{
    u32 u4Tmp = 0;
    if (power_on)
    {
        u4Tmp = CKGEN_READ32(REG_RW_LVDS_ANACFG4);
        u4Tmp &= (~LVDS_ANACFG4_VPlLL_PD);
        CKGEN_WRITE32(REG_RW_LVDS_ANACFG4, u4Tmp);
    }
    else
    {
        u4Tmp = CKGEN_READ32(REG_RW_LVDS_ANACFG4);
        u4Tmp |= (LVDS_ANACFG4_VPlLL_PD);
        CKGEN_WRITE32(REG_RW_LVDS_ANACFG4, u4Tmp);
    }
}

void mt8193_hdmi_ana_pwr_control(bool power_on)
{
    u32 u4Tmp = 0;
    if (power_on)
    {
        u4Tmp = CKGEN_READ32(REG_RW_HDMITX_ANACFG3);
        u4Tmp |= (HDMITX_ANACFG3_BIT20);
        u4Tmp |= (HDMITX_ANACFG3_BIT21);
        CKGEN_WRITE32(REG_RW_HDMITX_ANACFG3, u4Tmp);
    }
    else
    {
        u4Tmp = CKGEN_READ32(REG_RW_HDMITX_ANACFG3);
        u4Tmp &= (~HDMITX_ANACFG3_BIT20);
        u4Tmp &= (~HDMITX_ANACFG3_BIT21);
        CKGEN_WRITE32(REG_RW_HDMITX_ANACFG3, u4Tmp);
    }
}

void mt8193_pllgp_ana_pwr_control(bool power_on)
{
    u32 u4Tmp = 0;
    if (power_on)
    {
        u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG0);
        u4Tmp |= (PLLGP_ANACFG0_PLL1_EN);
        CKGEN_WRITE32(REG_RW_PLLGP_ANACFG0, u4Tmp);
    }
    else
    {
        u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG0);
        u4Tmp &= (~PLLGP_ANACFG0_PLL1_EN);
        CKGEN_WRITE32(REG_RW_PLLGP_ANACFG0, u4Tmp);
    }
}

void mt8193_nfi_ana_pwr_control(bool power_on)
{
    u32 u4Tmp = 0;
    if (power_on)
    {
        u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG2);
        u4Tmp |= (~PLLGP_ANACFG2_PLLGP_BIAS_EN);
        CKGEN_WRITE32(REG_RW_PLLGP_ANACFG2, u4Tmp);
        
        u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG0);
        u4Tmp |= (PLLGP_ANACFG0_PLL1_RESERVED);
        CKGEN_WRITE32(REG_RW_PLLGP_ANACFG0, u4Tmp);
    }
    else
    {
        u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG0);
        u4Tmp &= (~PLLGP_ANACFG0_PLL1_RESERVED);
        CKGEN_WRITE32(REG_RW_PLLGP_ANACFG0, u4Tmp);

        u4Tmp = CKGEN_READ32(REG_RW_PLLGP_ANACFG2);
        u4Tmp &= (~PLLGP_ANACFG2_PLLGP_BIAS_EN);
        CKGEN_WRITE32(REG_RW_PLLGP_ANACFG2, u4Tmp);
    }
}




void mt8193_lvds_sys_spm_control(bool power_on)
{
    u32 u4Tmp = 0;
    u32 u4Tmp2 = 0;
    u32 ui4_delay_cnt = 0x40000;

    if (power_on)
    {
        /* turn on power */
        u4Tmp = CKGEN_READ32(REG_RW_LVDS_PWR_CTRL);
        u4Tmp |= CKGEN_LVDS_PWR_PWR_ON;
        CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

        /* disable reset */
        u4Tmp2 = CKGEN_READ32(REG_RW_LVDS_PWR_RST_B);
        u4Tmp2 |= CKGEN_LVDS_PWR_RST_EN;
        CKGEN_WRITE32(REG_RW_LVDS_PWR_RST_B, u4Tmp2);

        /* disable iso */
        u4Tmp &= (~CKGEN_LVDS_PWR_ISO_EN);
        CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

        /* enable clock */
        u4Tmp &= (~CKGEN_LVDS_PWR_CLK_OFF);
        CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

        /* wait until pwr act */
        do
        {
            ui4_delay_cnt--;
        }while( (!(CKGEN_READ32(REG_RO_PWR_ACT)&CKGEN_LVDS_PWR_ON_ACT)) && ui4_delay_cnt);

        if (ui4_delay_cnt== 0)
        {
            printk("[CKGEN] Did not get power act for LVDS!!!!\n");
        }
        
    }
    else
    {
        /* disable clock */
        u4Tmp = CKGEN_READ32(REG_RW_LVDS_PWR_CTRL);
        u4Tmp |= CKGEN_LVDS_PWR_CLK_OFF;
        CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

        /* enable iso */
        u4Tmp |= CKGEN_LVDS_PWR_ISO_EN;
        CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);

        /* enable reset */
        u4Tmp2 = CKGEN_READ32(REG_RW_LVDS_PWR_RST_B);
        u4Tmp2 &= (~CKGEN_LVDS_PWR_RST_EN);
        CKGEN_WRITE32(REG_RW_LVDS_PWR_RST_B, u4Tmp2);

        /* turn off power */
        u4Tmp &= (~CKGEN_LVDS_PWR_PWR_ON);
        CKGEN_WRITE32(REG_RW_LVDS_PWR_CTRL, u4Tmp);
        
    }
}

void mt8193_hdmi_sys_spm_control(bool power_on)
{
    
    u32 u4Tmp = 0;
    u32 u4Tmp2 = 0;
    u32 ui4_delay_cnt = 0x40000;

    if (power_on)
    {
        /* turn on power */
        u4Tmp = CKGEN_READ32(REG_RW_HDMI_PWR_CTRL);
        u4Tmp |= CKGEN_HDMI_PWR_PWR_ON;
        CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

        /* disable reset */
        u4Tmp2 = CKGEN_READ32(REG_RW_HDMI_PWR_RST_B);
        u4Tmp2 |= CKGEN_HDMI_PWR_RST_EN;
        CKGEN_WRITE32(REG_RW_HDMI_PWR_RST_B, u4Tmp2);

        /* disable iso */
        u4Tmp &= (~CKGEN_HDMI_PWR_ISO_EN);
        CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

        /* enable clock */
        u4Tmp &= (~CKGEN_HDMI_PWR_CLK_OFF);
        CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

        /* wait until pwr act */
        do
        {
            ui4_delay_cnt--;
        }while( (!(CKGEN_READ32(REG_RO_PWR_ACT)&CKGEN_HDMI_PWR_ON_ACT)) && ui4_delay_cnt);

        if (ui4_delay_cnt== 0)
        {
            printk("[CKGEN] Did not get power act for HDMI!!!!\n");
        }
    }
    else
    {
        /* disable clock */
        u4Tmp = CKGEN_READ32(REG_RW_HDMI_PWR_CTRL);
        u4Tmp |= CKGEN_HDMI_PWR_CLK_OFF;
        CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

        /* enable iso */
        u4Tmp |= CKGEN_HDMI_PWR_ISO_EN;
        CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);

        /* enable reset */
        u4Tmp2 = CKGEN_READ32(REG_RW_HDMI_PWR_RST_B);
        u4Tmp2 &= (~CKGEN_HDMI_PWR_RST_EN);
        CKGEN_WRITE32(REG_RW_HDMI_PWR_RST_B, u4Tmp2);

        /* turn off power */
        u4Tmp &= (~CKGEN_HDMI_PWR_PWR_ON);
        CKGEN_WRITE32(REG_RW_HDMI_PWR_CTRL, u4Tmp);
    }
}

void mt8193_nfi_sys_spm_control(bool power_on)
{
    u32 u4Tmp = 0;
    u32 u4Tmp2 = 0;
    u32 ui4_delay_cnt = 0x40000;

    if (power_on)
    {
        /* turn on power */
        u4Tmp = CKGEN_READ32(REG_RW_NFI_PWR_CTRL);
        u4Tmp |= CKGEN_NFI_PWR_PWR_ON;
        CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

        /* disable reset */
        u4Tmp2 = CKGEN_READ32(REG_RW_NFI_PWR_RST_B);
        u4Tmp2 |= CKGEN_NFI_PWR_RST_EN;
        CKGEN_WRITE32(REG_RW_NFI_PWR_RST_B, u4Tmp2);

        /* disable iso */
        u4Tmp &= (~CKGEN_NFI_PWR_ISO_EN);
        CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

        /* enable clock */
        u4Tmp &= (~CKGEN_NFI_PWR_CLK_OFF);
        CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

        /* wait until pwr act */
        do
        {
            ui4_delay_cnt--;
        }while( (!(CKGEN_READ32(REG_RO_PWR_ACT)&CKGEN_NFI_PWR_ON_ACT)) && ui4_delay_cnt);

        if (ui4_delay_cnt== 0)
        {
            printk("[CKGEN] Did not get power act for NFI!!!!\n");
        }
    }
    else
    {
        /* disable clock */
        u4Tmp = CKGEN_READ32(REG_RW_NFI_PWR_CTRL);
        u4Tmp |= CKGEN_NFI_PWR_CLK_OFF;
        CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

        /* enable iso */
        u4Tmp |= CKGEN_NFI_PWR_ISO_EN;
        CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);

        /* enable reset */
        u4Tmp2 = CKGEN_READ32(REG_RW_NFI_PWR_RST_B);
        u4Tmp2 &= (~CKGEN_NFI_PWR_RST_EN);
        CKGEN_WRITE32(REG_RW_NFI_PWR_RST_B, u4Tmp2);

        /* turn off power */
        u4Tmp &= (~CKGEN_NFI_PWR_PWR_ON);
        CKGEN_WRITE32(REG_RW_NFI_PWR_CTRL, u4Tmp);
    }
}


void mt8193_bus_clk_switch(bool bus_26m_to_32k)
{
    u32 u4Tmp = 0;
      
    if (bus_26m_to_32k)
    {
        /* bus clock switch from 26M to 32K */
        mt_set_gpio_mode(GPIO147, 0);
        mt_set_gpio_dir(GPIO147, 1);
        mt_set_gpio_pull_enable(GPIO147, 1);
        mt_set_gpio_pull_select(GPIO147, 1);
        mt_set_gpio_out(GPIO147, 1);

        // u4Tmp = CKGEN_READ32(REG_RW_DCXO_ANACFG9);
        u4Tmp = 0x801025;
        u4Tmp &= (~(DCXO_ANACFG9_BUS_CK_SOURCE_SEL_MASK << DCXO_ANACFG9_BUS_CK_SOURCE_SEL_SHIFT));
        u4Tmp |= (3 << DCXO_ANACFG9_BUS_CK_SOURCE_SEL_SHIFT);
        CKGEN_WRITE32(REG_RW_DCXO_ANACFG9, u4Tmp);

        mt_set_gpio_out(GPIO147, 0);     
    }
    else
    {
         /* bus clock switch from 32K to 26M */

         mt_set_gpio_out(GPIO147, 1);

         msleep(10);

         u4Tmp = CKGEN_READ32(REG_RW_DCXO_ANACFG9);
         u4Tmp &= (~(DCXO_ANACFG9_BUS_CK_SOURCE_SEL_MASK << DCXO_ANACFG9_BUS_CK_SOURCE_SEL_SHIFT));
         CKGEN_WRITE32(REG_RW_DCXO_ANACFG9, u4Tmp);
         
         mt_set_gpio_mode(GPIO147, 1);
    }
}

void mt8193_en_bb_ctrl(bool pd)
{
    // GPIO60 is EN_BB
    // GPIO59 is CK_SEL
    if (pd)
    {
        // pull low
        mt_set_gpio_out(GPIO60, 0); 
        mt_set_gpio_out(GPIO59, 0); 
    }
    else
    {
        // pull high
        mt_set_gpio_out(GPIO59, 1); 
        mt_set_gpio_out(GPIO60, 1); 
    }
}


/******************************************************************************
 * mt8193_ckgen_suspend
 * 
 * DESCRIPTION:
 *   Suspend the nand device! 
 * 
 * PARAMETERS: 
 *   struct platform_device *pdev : device structure
 * 
 * RETURNS: 
 *   0 : Success
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
static int mt8193_ckgen_suspend(struct platform_device *pdev, pm_message_t state)
{
    printk("[CKGEN] mt8193_ckgen_suspend() enter\n");

#if 1
    /* add 8193 suspend function here */

    mt8193_pllgp_ana_pwr_control(false);
    msleep(5);
    // bus clk switch to 32K
    mt8193_bus_clk_switch(true);
    msleep(50);
#endif
    printk("[CKGEN] mt8193_ckgen_suspend() exit\n");
    
    return 0;
}
/******************************************************************************
 * mt8193_ckgen_resume
 * 
 * DESCRIPTION:
 *   Resume the nand device! 
 * 
 * PARAMETERS: 
 *   struct platform_device *pdev : device structure
 * 
 * RETURNS: 
 *   0 : Success
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
static int mt8193_ckgen_resume(struct platform_device *pdev)
{
    printk("[CKGEN] mt8193_ckgen_resume() enter\n");

#if 1
    /* add 8193 resume function here */
    // bus clk switch to 26M
    mt8193_bus_clk_switch(false);
    msleep(2);
    // turn on pllgp analog
    mt8193_pllgp_ana_pwr_control(true);
    msleep(2);
#endif
    printk("[CKGEN] mt8193_ckgen_resume() exit\n");
    
    return 0;
}



#endif
