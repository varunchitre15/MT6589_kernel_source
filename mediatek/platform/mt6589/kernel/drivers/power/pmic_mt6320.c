/*****************************************************************************
 *
 * Filename:
 * ---------
 *    pmic_mt6320.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 *   This Module defines PMIC functions
 *
 * Author:
 * -------
 * James Lo
 *
 ****************************************************************************/
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>

#include <asm/uaccess.h>

#include <mach/pmic_mt6320_sw.h>
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>
#include <mach/mt_pm_ldo.h>
//#include <mach/mt6320_pmic_feature_api.h>
#include <mach/eint.h>
#include <mach/mt_pmic_wrap.h>
#include <mach/mt_gpio.h>
#include <mach/mtk_rtc.h>
#include <mach/mt_spm_mtcmos.h>

#include "mt6320_battery.h"

#include <mtk_kpd.h>

#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
#include <mach/mt_boot.h>
#include <mach/system.h>
#include "mach/mt_gpt.h"
#endif
#if 1
#include <mach/mt_clkmgr.h>
#else
//#include <mach/mt_clock_manager.h>

#include "mach/mt_clkmgr.h"

extern int get_gpu_power_src(void);

//----------------------------------------------------------------------test
#define MT65XX_UPLL 3
static void enable_pll(int id, char *mod_name)
{
    printk("enable_pll is not ready.\n");
}
static void disable_pll(int id, char *mod_name)
{
    printk("disable_pll is not ready.\n");
}
//----------------------------------------------------------------------
#endif

#include <cust_gpio_usage.h>
#include <cust_eint.h>
//----------------------------------------------------------------------test
#if 0
#define CUST_EINT_POLARITY_LOW              0
#define CUST_EINT_POLARITY_HIGH             1
#define CUST_EINT_DEBOUNCE_DISABLE          0
#define CUST_EINT_DEBOUNCE_ENABLE           1
#define CUST_EINT_EDGE_SENSITIVE            0
#define CUST_EINT_LEVEL_SENSITIVE           1

#define CUST_EINT_MT6320_PMIC_NUM              3
#define CUST_EINT_MT6320_PMIC_DEBOUNCE_CN      1
#define CUST_EINT_MT6320_PMIC_POLARITY         CUST_EINT_POLARITY_HIGH
#define CUST_EINT_MT6320_PMIC_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_MT6320_PMIC_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE
#endif
//----------------------------------------------------------------------

//==============================================================================
// PMIC related define
//==============================================================================
#define VOLTAGE_FULL_RANGE     1200
#define ADC_PRECISE         1024 // 10 bits

static DEFINE_MUTEX(pmic_adc_mutex);

//==============================================================================
// Extern
//==============================================================================
extern int g_R_BAT_SENSE;
extern int g_R_I_SENSE;
extern int g_R_CHARGER_1;
extern int g_R_CHARGER_2;

extern int bat_thread_kthread(void *x);
extern void charger_hv_detect_sw_workaround_init(void);
extern int accdet_irq_handler(void);
extern void accdet_auxadc_switch(int enable);

#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
extern void mt_power_off(void);
static kal_bool long_pwrkey_press = false;
static unsigned long timer_pre = 0; 
static unsigned long timer_pos = 0; 
#define LONG_PWRKEY_PRESS_TIME 		500*1000000    //500ms
#endif
//==============================================================================
// PMIC lock/unlock APIs
//==============================================================================
void pmic_lock(void)
{
}

void pmic_unlock(void)
{
}


kal_uint32 upmu_get_reg_value(kal_uint32 reg)
{
    U32 ret=0;
    U32 reg_val=0;

    //printk("[upmu_get_reg_value] \n");
    ret=pmic_read_interface(reg, &reg_val, 0xFFFF, 0x0);
    
    return reg_val;
}
EXPORT_SYMBOL(upmu_get_reg_value);

void upmu_set_reg_value(kal_uint32 reg, kal_uint32 reg_val)
{
    U32 ret=0;

    //printk("[upmu_set_reg_value] \n");
    ret=pmic_config_interface(reg, reg_val, 0xFFFF, 0x0);    
}

void pmic_thermal_dump_reg(void)
{
    printk("[%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x, [%x]=%x\n", 
        0x540, upmu_get_reg_value(0x540),
        0x542, upmu_get_reg_value(0x542),
        0x554, upmu_get_reg_value(0x554),
        0x558, upmu_get_reg_value(0x558),
        0x128, upmu_get_reg_value(0x128),
        0x12A, upmu_get_reg_value(0x12A),
        0x52E, upmu_get_reg_value(0x52E),
        0x530, upmu_get_reg_value(0x530),
        0x406, upmu_get_reg_value(0x406),
        0x022, upmu_get_reg_value(0x022),
        0x030, upmu_get_reg_value(0x030),
        0x032, upmu_get_reg_value(0x032),
        0x036, upmu_get_reg_value(0x036)
        );
}

//==============================================================================
// PMIC-AUXADC 
//==============================================================================
extern int Enable_BATDRV_LOG;

int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd)
{
    kal_int32 u4Sample_times = 0;
    kal_int32 u4channel[8] = {0,0,0,0,0,0,0,0};
    kal_int32 adc_result=0;
    kal_int32 adc_result_temp=0;
    kal_int32 r_val_temp=0;    
    kal_int32 count=0;
    kal_int32 count_time_out=1000;
    kal_int32 ret_data=0;

    mutex_lock(&pmic_adc_mutex);

    if(dwChannel==1)
    {
        upmu_set_rg_source_ch0_norm_sel(1);
        upmu_set_rg_source_ch0_lbat_sel(1);
        dwChannel=0;
    }

    /*
        0 : V_BAT
        1 : V_I_Sense
        2 : V_Charger
        3 : V_TBAT
        4~7 : reserved    
    */
    upmu_set_rg_auxadc_chsel(dwChannel);

    upmu_set_rg_avg_num(0x3);

    if(dwChannel==3)
    {
        upmu_set_rg_buf_pwd_on(1);
        upmu_set_rg_buf_pwd_b(1);
        upmu_set_baton_tdet_en(1);
        msleep(20);
    }

    if(dwChannel==4)
    {
        upmu_set_rg_vbuf_en(1);
        upmu_set_rg_vbuf_byp(0);
    
        if(trimd==2)
        {            
            upmu_set_rg_vbuf_calen(0); /* For T_PMIC*/
            upmu_set_rg_spl_num(0x1E);            
            trimd=1;
        }
        else
        {            
            upmu_set_rg_vbuf_calen(1); /* For T_BAT*/
        }

        //Duo to HW limitation
        msleep(1);
    }

    if(dwChannel==5)
    {
        accdet_auxadc_switch(1);
    }

    u4Sample_times=0;
    
    do
    {
        upmu_set_rg_auxadc_start(0);
        upmu_set_rg_auxadc_start(1);

        //Duo to HW limitation
        udelay(50);

        count=0;
        ret_data=0;

        switch(dwChannel){         
            case 0:    
                while( upmu_get_rg_adc_rdy_c0() != 1 )
                {    
                    if( (count++) > count_time_out)
                    {
                        xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
                        break;
                    }
                }
                if(trimd == 1) {
                    ret_data = upmu_get_rg_adc_out_c0_trim();
                } else {            
                    ret_data = upmu_get_rg_adc_out_c0();
                }
                break;
            case 1:    
                while( upmu_get_rg_adc_rdy_c1() != 1 )
                {    
                    if( (count++) > count_time_out)
                    {
                        xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
                        break;
                    }
                }
                if(trimd == 1) {
                    ret_data = upmu_get_rg_adc_out_c1_trim();
                } else {            
                    ret_data = upmu_get_rg_adc_out_c1();
                }
                break;
            case 2:    
                while( upmu_get_rg_adc_rdy_c2() != 1 )
                {    
                    if( (count++) > count_time_out)
                    {
                        xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
                        break;
                    }
                }
                if(trimd == 1) {
                    ret_data = upmu_get_rg_adc_out_c2_trim();
                } else {            
                    ret_data = upmu_get_rg_adc_out_c2();
                }
                break;
            case 3:    
                while( upmu_get_rg_adc_rdy_c3() != 1 )
                {    
                    if( (count++) > count_time_out)
                    {
                        xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
                        break;
                    }
                }
                if(trimd == 1) {
                    ret_data = upmu_get_rg_adc_out_c3_trim();
                } else {            
                    ret_data = upmu_get_rg_adc_out_c3();
                }
                break;
            case 4:    
                while( upmu_get_rg_adc_rdy_c4() != 1 )
                {    
                    if( (count++) > count_time_out)
                    {
                        xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
                        break;
                    }
                }
                if(trimd == 1) {
                    ret_data = upmu_get_rg_adc_out_c4_trim();
                } else {            
                    ret_data = upmu_get_rg_adc_out_c4();
                }
                break;
            case 5:    
                while( upmu_get_rg_adc_rdy_c5() != 1 )
                {    
                    if( (count++) > count_time_out)
                    {
                        xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
                        break;
                    }
                }
                if(trimd == 1) {
                    ret_data = upmu_get_rg_adc_out_c5_trim();
                } else {            
                    ret_data = upmu_get_rg_adc_out_c5();
                }
                break;
            case 6:    
                while( upmu_get_rg_adc_rdy_c6() != 1 )
                {    
                    if( (count++) > count_time_out)
                    {
                        xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
                        break;
                    }
                }
                if(trimd == 1) {
                    ret_data = upmu_get_rg_adc_out_c6_trim();
                } else {            
                    ret_data = upmu_get_rg_adc_out_c6();
                }
                break;
            case 7:    
                while( upmu_get_rg_adc_rdy_c7() != 1 )
                {    
                    if( (count++) > count_time_out)
                    {
                        xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[IMM_GetOneChannelValue_PMIC] (%d) Time out!\n", dwChannel);
                        break;
                    }
                }
                if(trimd == 1) {
                    ret_data = upmu_get_rg_adc_out_c7_trim();
                } else {            
                    ret_data = upmu_get_rg_adc_out_c7();
                }
                break;    
            default:
                xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
                mutex_unlock(&pmic_adc_mutex);
                return -1;
                break;
        }

        u4channel[dwChannel] += ret_data;

        u4Sample_times++;

        if (Enable_BATDRV_LOG == 1)
        {
            //debug
            xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[AUXADC] u4Sample_times=%d, ret_data=%d, u4channel[%d]=%d.\n", 
                u4Sample_times, ret_data, dwChannel, u4channel[dwChannel]);
        }
        
    }while (u4Sample_times < deCount);

    /* Value averaging  */ 
    u4channel[dwChannel] = u4channel[dwChannel] / deCount;
    adc_result_temp = u4channel[dwChannel];

    switch(dwChannel){         
        case 0:                
            r_val_temp = g_R_BAT_SENSE;            
            adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
            break;
        case 1:    
            r_val_temp = g_R_I_SENSE;
            adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
            break;
        case 2:    
            r_val_temp = (((g_R_CHARGER_1+g_R_CHARGER_2)*100)/g_R_CHARGER_2);
            adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
            break;
        case 3:    
            r_val_temp = 1;
            adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
            break;
        case 4:    
            r_val_temp = 1;
            adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
            break;
        case 5:    
            r_val_temp = 1;
            adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
            break;
        case 6:    
            r_val_temp = 1;
            adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
            break;
        case 7:    
            r_val_temp = 1;
            adc_result = (adc_result_temp*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;
            break;    
        default:
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[AUXADC] Invalid channel value(%d,%d)\n", dwChannel, trimd);
            mutex_unlock(&pmic_adc_mutex);
            return -1;
            break;
    }

    if (Enable_BATDRV_LOG == 1)
    {
        //debug
        xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[AUXADC] adc_result_temp=%d, adc_result=%d, r_val_temp=%d.\n", 
                adc_result_temp, adc_result, r_val_temp);
    }

    count=0;

    if(dwChannel==0)
    {
        upmu_set_rg_source_ch0_norm_sel(0);
        upmu_set_rg_source_ch0_lbat_sel(0);
    }

    if(dwChannel==3)
    {
        upmu_set_baton_tdet_en(0);     
        upmu_set_rg_buf_pwd_b(0);
        upmu_set_rg_buf_pwd_on(0);
    }

    if(dwChannel==4)
    {
        //upmu_set_rg_vbuf_en(0);
        //upmu_set_rg_vbuf_byp(0);
        upmu_set_rg_vbuf_calen(0);
    }

    if(dwChannel==5)
    {
        accdet_auxadc_switch(0);
    }

    upmu_set_rg_spl_num(0x1);

    mutex_unlock(&pmic_adc_mutex);

    return adc_result;
    
}

//==============================================================================
// PMIC-Charger Type Detection
//==============================================================================
CHARGER_TYPE g_ret = CHARGER_UNKNOWN;
kal_uint32 g_charger_in_flag = 0;
kal_uint32 g_first_check=0;

CHARGER_TYPE hw_charger_type_detection(void)
{
    CHARGER_TYPE ret                 = CHARGER_UNKNOWN;
    
#if defined(CONFIG_POWER_EXT)    
    ret = STANDARD_HOST;
#else
    unsigned int USB_U2PHYACR6_2     = 0xF122081A;
    unsigned int USBPHYRegs          = 0xF1220800; //U2B20_Base+0x800
    U16 bLineState_B                 = 0;
    U32 wChargerAvail                = 0;
    U32 bLineState_C                 = 0;
    U32 ret_val                      = 0;
    U32 reg_val                      = 0;

    //msleep(400);
    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : start!\r\n");

/********* Step 0.0 : enable USB memory and clock *********/
    enable_pll(UNIVPLL, "USB_PLL");
    hwPowerOn(MT65XX_POWER_LDO_VUSB,VOL_DEFAULT,"VUSB_LDO");
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[hw_charger_type_detection] enable VUSB and UPLL before connect\n");

/********* Step 1.0 : PMU_BC11_Detect_Init ***************/        
    SETREG16(USB_U2PHYACR6_2,0x80); //bit 7 = 1 : switch to PMIC        
    
    //BC11_RST=1
    ret_val=pmic_config_interface(CHR_CON18,0x1,PMIC_RG_BC11_RST_MASK,PMIC_RG_BC11_RST_SHIFT); 
    //BC11_BB_CTRL=1
    ret_val=pmic_config_interface(CHR_CON18,0x1,PMIC_RG_BC11_BB_CTRL_MASK,PMIC_RG_BC11_BB_CTRL_SHIFT);
    
    //RG_BC11_BIAS_EN=1    
    ret_val=pmic_config_interface(CHR_CON19,0x1,PMIC_RG_BC11_BIAS_EN_MASK,PMIC_RG_BC11_BIAS_EN_SHIFT); 
    //RG_BC11_VSRC_EN[1:0]=00
    ret_val=pmic_config_interface(CHR_CON18,0x0,PMIC_RG_BC11_VSRC_EN_MASK,PMIC_RG_BC11_VSRC_EN_SHIFT); 
    //RG_BC11_VREF_VTH = 0
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_VREF_VTH_MASK,PMIC_RG_BC11_VREF_VTH_SHIFT); 
    //RG_BC11_CMP_EN[1.0] = 00
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_CMP_EN_MASK,PMIC_RG_BC11_CMP_EN_SHIFT);
    //RG_BC11_IPU_EN[1.0] = 00
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_IPU_EN_MASK,PMIC_RG_BC11_IPU_EN_SHIFT);
    //RG_BC11_IPD_EN[1.0] = 00
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_IPD_EN_MASK,PMIC_RG_BC11_IPD_EN_SHIFT);

    //ret_val=pmic_read_interface(CHR_CON18,&reg_val,0xFFFF,0);        
    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Reg[0x%x]=%x, ", CHR_CON18, reg_val);
    //ret_val=pmic_read_interface(CHR_CON19,&reg_val,0xFFFF,0);        
    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Reg[0x%x]=%x \n", CHR_CON19, reg_val);

/********* Step A *************************************/
    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step A\r\n");
    
    //RG_BC11_IPU_EN[1.0] = 00
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_IPU_EN_MASK,PMIC_RG_BC11_IPU_EN_SHIFT);
    
    SETREG16(USBPHYRegs+0x1C,0x1000);//RG_PUPD_BIST_EN = 1    
    CLRREG16(USBPHYRegs+0x1C,0x0400);//RG_EN_PD_DM=0
    
    //RG_BC11_VSRC_EN[1.0] = 10 
    ret_val=pmic_config_interface(CHR_CON18,0x2,PMIC_RG_BC11_VSRC_EN_MASK,PMIC_RG_BC11_VSRC_EN_SHIFT); 
    //RG_BC11_IPD_EN[1:0] = 01
    ret_val=pmic_config_interface(CHR_CON19,0x1,PMIC_RG_BC11_IPD_EN_MASK,PMIC_RG_BC11_IPD_EN_SHIFT);
    //RG_BC11_VREF_VTH = 0
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_VREF_VTH_MASK,PMIC_RG_BC11_VREF_VTH_SHIFT);
    //RG_BC11_CMP_EN[1.0] = 01
    ret_val=pmic_config_interface(CHR_CON19,0x1,PMIC_RG_BC11_CMP_EN_MASK,PMIC_RG_BC11_CMP_EN_SHIFT);

//<2013/5/2-24504-jessicatseng, [5860][ATS00159273][Ar]PER-GST-<5860_Battery> -<It takes too much time to plug-in the USB cable and the phone show is charging now compare with>
//<2013/3/29-23361-jessicatseng, [Pelican] Add re-check charger type 
#if defined(RECHECK_CHARGER_TYPE)    	
    mdelay(100);
#else
    mdelay(100);
#endif
//>2013/3/29-23361-jessicatseng
//>2013/5/2-24504-jessicatseng
        
    ret_val=pmic_read_interface(CHR_CON18,&wChargerAvail,PMIC_RGS_BC11_CMP_OUT_MASK,PMIC_RGS_BC11_CMP_OUT_SHIFT); 
    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step A : wChargerAvail=%x\r\n", wChargerAvail);
    
    //RG_BC11_VSRC_EN[1:0]=00
    ret_val=pmic_config_interface(CHR_CON18,0x0,PMIC_RG_BC11_VSRC_EN_MASK,PMIC_RG_BC11_VSRC_EN_SHIFT); 
    //RG_BC11_IPD_EN[1.0] = 00
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_IPD_EN_MASK,PMIC_RG_BC11_IPD_EN_SHIFT);
    //RG_BC11_CMP_EN[1.0] = 00
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_CMP_EN_MASK,PMIC_RG_BC11_CMP_EN_SHIFT);
    
    mdelay(50);
    
    if(wChargerAvail==1)
    {
/********* Step B *************************************/
        //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step B\r\n");

        //RG_BC11_IPU_EN[1:0]=10
        ret_val=pmic_config_interface(CHR_CON19,0x2,PMIC_RG_BC11_IPU_EN_MASK,PMIC_RG_BC11_IPU_EN_SHIFT);        

//<2013/5/2-24504-jessicatseng, [5860][ATS00159273][Ar]PER-GST-<5860_Battery> -<It takes too much time to plug-in the USB cable and the phone show is charging now compare with>
//<2013/3/29-23361-jessicatseng, [Pelican] Add re-check charger type 
#if defined(RECHECK_CHARGER_TYPE)    	
        mdelay(80);
#else
        mdelay(80);
#endif        
//>2013/3/29-23361-jessicatseng
//>2013/5/2-24504-jessicatseng
        
        bLineState_B = INREG16(USBPHYRegs+0x76);
        //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step B : bLineState_B=%x\r\n", bLineState_B);
        if(bLineState_B & 0x80)
        {
            ret = STANDARD_CHARGER;
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step B : STANDARD CHARGER!\r\n");
        }
        else
        {
            ret = CHARGING_HOST;
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step B : Charging Host!\r\n");
        }
    }
    else
    {
/********* Step C *************************************/
        //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step C\r\n");

        //RG_BC11_IPU_EN[1:0]=01
        ret_val=pmic_config_interface(CHR_CON19,0x1,PMIC_RG_BC11_IPU_EN_MASK,PMIC_RG_BC11_IPU_EN_SHIFT);
        //RG_BC11_CMP_EN[1.0] = 01
        ret_val=pmic_config_interface(CHR_CON19,0x1,PMIC_RG_BC11_CMP_EN_MASK,PMIC_RG_BC11_CMP_EN_SHIFT);
        
        //ret_val=pmic_read_interface(CHR_CON19,&reg_val,0xFFFF,0);        
        //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step C : Reg[0x%x]=%x\r\n", CHR_CON19, reg_val);        
        
//<2013/5/2-24504-jessicatseng, [5860][ATS00159273][Ar]PER-GST-<5860_Battery> -<It takes too much time to plug-in the USB cable and the phone show is charging now compare with>
//<2013/3/29-23361-jessicatseng, [Pelican] Add re-check charger type 
#if defined(RECHECK_CHARGER_TYPE)    	
        mdelay(80);
#else
        mdelay(80);
#endif
//>2013/3/29-23361-jessicatseng
//>2013/5/2-24504-jessicatseng
                
        ret_val=pmic_read_interface(CHR_CON18,&bLineState_C,0xFFFF,0);
        //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step C : bLineState_C=%x\r\n", bLineState_C);
        if(bLineState_C & 0x0080)
        {
            ret = NONSTANDARD_CHARGER;
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step C : UNSTANDARD CHARGER!!!\r\n");
            
            //RG_BC11_IPU_EN[1:0]=10
            ret_val=pmic_config_interface(CHR_CON19,0x2,PMIC_RG_BC11_IPU_EN_MASK,PMIC_RG_BC11_IPU_EN_SHIFT);
            
            mdelay(80);
        }
        else
        {
            ret = STANDARD_HOST;
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : step C : Standard USB Host!!\r\n");
        }
    }
/********* Finally setting *******************************/

    //RG_BC11_VSRC_EN[1:0]=00
    ret_val=pmic_config_interface(CHR_CON18,0x0,PMIC_RG_BC11_VSRC_EN_MASK,PMIC_RG_BC11_VSRC_EN_SHIFT); 
    //RG_BC11_VREF_VTH = 0
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_VREF_VTH_MASK,PMIC_RG_BC11_VREF_VTH_SHIFT);
    //RG_BC11_CMP_EN[1.0] = 00
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_CMP_EN_MASK,PMIC_RG_BC11_CMP_EN_SHIFT);
    //RG_BC11_IPU_EN[1.0] = 00
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_IPU_EN_MASK,PMIC_RG_BC11_IPU_EN_SHIFT);
    //RG_BC11_IPD_EN[1.0] = 00
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_IPD_EN_MASK,PMIC_RG_BC11_IPD_EN_SHIFT);
    //RG_BC11_BIAS_EN=0
    ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_BIAS_EN_MASK,PMIC_RG_BC11_BIAS_EN_SHIFT); 
    
    CLRREG16(USB_U2PHYACR6_2,0x80); //bit 7 = 0 : switch to USB

    hwPowerDown(MT65XX_POWER_LDO_VUSB,"VUSB_LDO");
    disable_pll(UNIVPLL,"USB_PLL");
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[hw_charger_type_detection] disable VUSB and UPLL before disconnect\n");

    if( (ret==STANDARD_HOST) || (ret==CHARGING_HOST) )
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "mt_charger_type_detection : SW workaround for USB\r\n");
        //RG_BC11_BB_CTRL=1
        ret_val=pmic_config_interface(CHR_CON18,0x1,PMIC_RG_BC11_BB_CTRL_MASK,PMIC_RG_BC11_BB_CTRL_SHIFT);
        //RG_BC11_BIAS_EN=1
        ret_val=pmic_config_interface(CHR_CON19,0x1,PMIC_RG_BC11_BIAS_EN_MASK,PMIC_RG_BC11_BIAS_EN_SHIFT); 
        //RG_BC11_VSRC_EN[1.0] = 11        
        ret_val=pmic_config_interface(CHR_CON18,0x3,PMIC_RG_BC11_VSRC_EN_MASK,PMIC_RG_BC11_VSRC_EN_SHIFT);
        //check
        ret_val=pmic_read_interface(CHR_CON18,&reg_val,0xFFFF,0);
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Reg[0x%x]=0x%x\n", CHR_CON18, reg_val);
        ret_val=pmic_read_interface(CHR_CON19,&reg_val,0xFFFF,0);
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Reg[0x%x]=0x%x\n", CHR_CON19, reg_val);
    }        
#endif

    //step4:done, ret the type    
    return ret;
}

#if defined(CONFIG_POWER_EXT)

CHARGER_TYPE mt_charger_type_detection(void)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[mt_charger_type_detection] In FPGA/EVB, return USB.\r\n");
    return STANDARD_HOST;
}
EXPORT_SYMBOL(mt_charger_type_detection);

#else

CHARGER_TYPE mt_charger_type_detection(void)
{
    if( g_first_check == 0 )
    {
        g_first_check = 1;
        g_ret = hw_charger_type_detection();
    }
    else
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[mt_charger_type_detection] Got data !!, %d, %d\r\n", g_charger_in_flag, g_first_check);
    }

    return g_ret;
}
EXPORT_SYMBOL(mt_charger_type_detection);

#endif

int get_charger_type(void)
{
    return g_ret;
}
EXPORT_SYMBOL(get_charger_type);

void upmu_interrupt_chrdet_int_en(kal_uint32 val)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[upmu_interrupt_chrdet_int_en] val=%d.\r\n", val);

    upmu_set_rg_int_en_chrdet(val);
}
EXPORT_SYMBOL(upmu_interrupt_chrdet_int_en);

//==============================================================================
// PMIC Interrupt service
//==============================================================================
int pmic_thread_timeout=0;
static DEFINE_MUTEX(pmic_mutex);
static DECLARE_WAIT_QUEUE_HEAD(pmic_thread_wq);

extern int g_chr_event;
extern struct wake_lock battery_suspend_lock;
extern void wake_up_bat (void);
extern int bat_volt_check_point;
extern int g_bat_init_flag;

void wake_up_pmic(void)
{
    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[wake_up_pmic]\r\n");
    pmic_thread_timeout = 1;
    wake_up(&pmic_thread_wq);
}
EXPORT_SYMBOL(wake_up_pmic);

#define WAKE_LOCK_INITIALIZED            (1U << 8)

#if defined(CONFIG_POWER_EXT)
extern void mt_usb_connect(void);
extern void mt_usb_disconnect(void);
#endif
extern void BAT_UpdateChargerStatus(void);

void do_chrdet_int_task(void)
{
    U32 ret=0;
    U32 ret_val=0;
    U32 reg_val=0;
    
    ret=upmu_get_rgs_chrdet();
	BAT_UpdateChargerStatus();	
    if(ret==1)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[do_chrdet_int_task] charger exist!\n");
        g_charger_in_flag = 1;

        #if defined(CONFIG_POWER_EXT)
        mt_usb_connect();
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[do_chrdet_int_task] call mt_usb_connect() in EVB\n");
        #endif
    }
    else
    {
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
		if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT || g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT)
		{
			xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_thread_kthread] Unplug Charger/USB In Kernel Power Off Charging Mode!  Shutdown OS!\r\n");
			mt_power_off();
		}
#endif
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[do_chrdet_int_task] charger NOT exist!\n");
        g_charger_in_flag = 0;
        g_first_check = 0;

        //RG_BC11_BB_CTRL=1
        ret_val=pmic_config_interface(CHR_CON18,0x1,PMIC_RG_BC11_BB_CTRL_MASK,PMIC_RG_BC11_BB_CTRL_SHIFT);
        //RG_BC11_BIAS_EN=0
        ret_val=pmic_config_interface(CHR_CON19,0x0,PMIC_RG_BC11_BIAS_EN_MASK,PMIC_RG_BC11_BIAS_EN_SHIFT);
        //RG_BC11_VSRC_EN[1:0]=00
        ret_val=pmic_config_interface(CHR_CON18,0x0,PMIC_RG_BC11_VSRC_EN_MASK,PMIC_RG_BC11_VSRC_EN_SHIFT);
        //check
        ret_val=pmic_read_interface(CHR_CON18,&reg_val,0xFFFF,0);
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Reg[0x%x]=0x%x\n", CHR_CON18, reg_val);
        ret_val=pmic_read_interface(CHR_CON19,&reg_val,0xFFFF,0);
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Reg[0x%x]=0x%x\n", CHR_CON19, reg_val);

        #if defined(CONFIG_POWER_EXT)
        mt_usb_disconnect();
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[do_chrdet_int_task] call mt_usb_disconnect() in EVB\n");
        #endif
    }
    
    #if defined(CONFIG_POWER_EXT)
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[do_chrdet_int_task] Environment=FPGA/EVB\n");
    #else
    wake_lock(&battery_suspend_lock);
    g_chr_event = 1;

    if(g_bat_init_flag==1)
        wake_up_bat();
    else
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[do_chrdet_int_task] battery thread not ready, will do after bettery init.\n");    
    #endif
}

void cust_pmic_interrupt_en_setting(void)
{
    upmu_set_rg_int_en_ov(0);
    upmu_set_rg_int_en_chrdet(1);
    upmu_set_rg_int_en_bvalid_det(0);
    upmu_set_rg_int_en_vbaton_undet(0);
    upmu_set_rg_int_en_thr_h(0);
    upmu_set_rg_int_en_thr_l(0);
    upmu_set_rg_int_en_pwrkey(1);
    upmu_set_rg_int_en_watchdog(0);
    upmu_set_rg_int_en_fg_bat_h(0);
    upmu_set_rg_int_en_fg_bat_l(0);
    upmu_set_rg_int_en_bat_h(0);
    upmu_set_rg_int_en_bat_l(0);
    upmu_set_rg_int_en_spkr(0);
    upmu_set_rg_int_en_spkl(0);
    upmu_set_rg_int_en_spkr_ab(0);
    upmu_set_rg_int_en_spkl_ab(0);
    
    upmu_set_rg_int_en_vrf18_2(0);
    upmu_set_rg_int_en_vrf18(0);
    upmu_set_rg_int_en_vpa(0);
    upmu_set_rg_int_en_vio18(0);
    upmu_set_rg_int_en_vm(0);
    upmu_set_rg_int_en_vcore(0);
    upmu_set_rg_int_en_vsram(0);
    upmu_set_rg_int_en_vproc(0);
    upmu_set_rg_int_en_rtc(1);
    upmu_set_rg_int_en_audio(0);
    //upmu_set_rg_int_en_accdet(1);
    upmu_set_rg_int_en_homekey(1);
    upmu_set_rg_int_en_ldo(0);    
}

void spkl_ab_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[spkl_ab_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,0);    
}
void spkr_ab_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[spkr_ab_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,1);    
}
void spkl_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[spkl_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,2);    
}
void spkr_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[spkr_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,3);    
}
void bat_l_int_handler(void)
{
    kal_uint32 ret=0;
    
    kal_uint32 lbat_debounce_count_max=0;
    kal_uint32 lbat_debounce_count_min=0;
    
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[bat_l_int_handler]....\n");

    upmu_set_rg_lbat_irq_en_max(0);
	upmu_set_rg_lbat_irq_en_min(0);
    upmu_set_rg_lbat_en_max(0);
    upmu_set_rg_lbat_en_min(0);

    lbat_debounce_count_max = upmu_get_rg_lbat_debounce_count_max();
    lbat_debounce_count_min = upmu_get_rg_lbat_debounce_count_min();

    printk("[bat_l_int_handler] (lbat_debounce_count_max=%d, lbat_debounce_count_min=%d) \n", 
    lbat_debounce_count_max, lbat_debounce_count_min);
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,4);
}
void bat_h_int_handler(void)
{
    kal_uint32 ret=0;

    kal_uint32 lbat_debounce_count_max=0;
    kal_uint32 lbat_debounce_count_min=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[bat_h_int_handler]....\n");

    upmu_set_rg_int_en_bat_h(0);

    upmu_set_rg_lbat_irq_en_max(0);
    upmu_set_rg_lbat_irq_en_min(0);
    upmu_set_rg_lbat_en_max(0);
    upmu_set_rg_lbat_en_min(0);

    lbat_debounce_count_max = upmu_get_rg_lbat_debounce_count_max();
    lbat_debounce_count_min = upmu_get_rg_lbat_debounce_count_min();

    printk("[bat_h_int_handler] (lbat_debounce_count_max=%d, lbat_debounce_count_min=%d) \n", 
    lbat_debounce_count_max, lbat_debounce_count_min);

    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,5);
}
void fg_bat_l_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[fg_bat_l_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,6);    
}
void fg_bat_h_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[fg_bat_h_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,7);    
}
void watchdog_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[watchdog_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,8);    
}
void pwrkey_int_handler(void)
{
    kal_uint32 ret=0;

    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pwrkey_int_handler]....\n");

    if(upmu_get_pwrkey_deb()==1)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pwrkey_int_handler] Release pwrkey\n");
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
		if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT)
		{
				timer_pos = sched_clock();
				if(timer_pos - timer_pre >= LONG_PWRKEY_PRESS_TIME)
				{
					long_pwrkey_press = true;
				}
				xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_thread_kthread] timer_pos = %ld, timer_pre = %ld, timer_pos-timer_pre = %ld, long_pwrkey_press = %d\r\n",timer_pos, timer_pre, timer_pos-timer_pre, long_pwrkey_press);
				if(long_pwrkey_press)   //500ms
				{
					xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_thread_kthread] Power Key Pressed during kernel power off charging, reboot OS\r\n");
					arch_reset(0, NULL);
				}
		}
#endif
        kpd_pwrkey_pmic_handler(0x0);
    }
    else
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pwrkey_int_handler] Press pwrkey\n");
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
		if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT)
		{
			timer_pre = sched_clock();
		}
#endif
        kpd_pwrkey_pmic_handler(0x1);
    }
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,9);    
}
void thr_l_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[thr_l_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,10);    
}
void thr_h_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[thr_h_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,11);    
}
void vbaton_undet_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[vbaton_undet_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,12);    
}
void bvalid_det_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[bvalid_det_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS0,0x1,0x1,13);    
}
void chrdet_int_handler(void)
{
    // read clear, already read
    
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[chrdet_int_handler]....\n");
    
    do_chrdet_int_task();
}
void ov_int_handler(void)
{
    // read clear, already read
    
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[ov_int_handler]....\n");
}
void ldo_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[ldo_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,0);

    upmu_set_rg_int_en_ldo(0);
    mdelay(1);
}   
void homekey_int_handler(void)
{
    kal_uint32 ret=0;

    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[homekey_int_handler]....\n");

    if(upmu_get_homekey_deb()==1)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[homekey_int_handler] Release HomeKey\r\n");
        kpd_pmic_rstkey_handler(0x0);
    }
    else
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[homekey_int_handler] Press HomeKey\r\n");
        kpd_pmic_rstkey_handler(0x1);
    }
    
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,1);    
}    
void accdet_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[accdet_int_handler]....\n");
	
    ret = accdet_irq_handler();
	if(0 == ret){
		xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[accdet_int_handler] don't finished\n");
	}
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,2);    
}    
void audio_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[audio_int_handler]....\n");
    
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,3);    
}    
void rtc_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[rtc_int_handler]....\n");
    rtc_irq_handler();
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,4);    
}    
void vproc_int_handler(void)
{
    kal_uint32 ret=0;
    
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[vproc_int_handler]....\n");

    upmu_set_rg_pwmoc_ck_pdn(1);

    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,8);

    upmu_set_rg_int_en_vproc(0);
    mdelay(1);
}    
void vsram_int_handler(void)
{
    kal_uint32 ret=0;
    
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[vsram_int_handler]....\n");
    
    upmu_set_rg_pwmoc_ck_pdn(1);

    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,9);

    upmu_set_rg_int_en_vsram(0);
    mdelay(1);
}    
void vcore_int_handler(void)
{
    kal_uint32 ret=0;
    
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[vcore_int_handler]....\n");

    upmu_set_rg_pwmoc_ck_pdn(1);

    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,10);

    upmu_set_rg_int_en_vcore(0);
    mdelay(1);
}    
void vm_int_handler(void)
{
    kal_uint32 ret=0;
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[vm_int_handler]....\n");

    upmu_set_rg_pwmoc_ck_pdn(1);
    
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,11);    

    upmu_set_rg_int_en_vm(0);
    mdelay(1);
}    
void vio18_int_handler(void)
{
    kal_uint32 ret=0;
    
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[vio18_int_handler]....\n");

    upmu_set_rg_pwmoc_ck_pdn(1);
    
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,12);

    upmu_set_rg_int_en_vio18(0);
    mdelay(1);
}    
void vpa_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[vpa_int_handler]....\n");

    upmu_set_rg_pwmoc_ck_pdn(1);
    
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,13);

    upmu_set_rg_int_en_vpa(0);
    mdelay(1);
}    
void vrf18_int_handler(void)
{
    kal_uint32 ret=0;
    
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[vrf18_int_handler]....\n");

    upmu_set_rg_pwmoc_ck_pdn(1);
    
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,14);

    upmu_set_rg_int_en_vrf18(0);
    mdelay(1);
}    
void vrf18_2_int_handler(void)
{
    kal_uint32 ret=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[vrf18_2_int_handler]....\n");

    upmu_set_rg_pwmoc_ck_pdn(1);
    
    ret=pmic_config_interface(INT_STATUS1,0x1,0x1,15);    

    upmu_set_rg_int_en_vrf18_2(0);
    mdelay(1);
}

static int pmic_thread_kthread(void *x)
{
    kal_uint32 ret=0;
    //kal_uint32 ret_val=0;
    //kal_uint32 reg_val=0;
    kal_uint32 int_status_val_0=0;
    kal_uint32 int_status_val_1=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_thread_kthread] enter\n");

    /* Run on a process content */
    while (1) {
        mutex_lock(&pmic_mutex);

        //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_thread_kthread] running\n");

        //--------------------------------------------------------------------------------
        ret=pmic_read_interface(INT_STATUS0,(&int_status_val_0),0xFFFF,0x0);
        //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[INT] int_status_val_0=0x%x\n", int_status_val_0);

        if( (((int_status_val_0)&(0x0001))>>0) == 1 )  { spkl_ab_int_handler();      }
        if( (((int_status_val_0)&(0x0002))>>1) == 1 )  { spkr_ab_int_handler();      }         
        if( (((int_status_val_0)&(0x0004))>>2) == 1 )  { spkl_int_handler();         }
        if( (((int_status_val_0)&(0x0008))>>3) == 1 )  { spkr_int_handler();         }
        if( (((int_status_val_0)&(0x0010))>>4) == 1 )  { bat_l_int_handler();        }
        if( (((int_status_val_0)&(0x0020))>>5) == 1 )  { bat_h_int_handler();        }
        if( (((int_status_val_0)&(0x0040))>>6) == 1 )  { fg_bat_l_int_handler();     }
        if( (((int_status_val_0)&(0x0080))>>7) == 1 )  { fg_bat_h_int_handler();     }
        if( (((int_status_val_0)&(0x0100))>>8) == 1 )  { watchdog_int_handler();     }
        if( (((int_status_val_0)&(0x0200))>>9) == 1 )  { pwrkey_int_handler();       }
        if( (((int_status_val_0)&(0x0400))>>10) == 1 ) { thr_l_int_handler();        }
        if( (((int_status_val_0)&(0x0800))>>11) == 1 ) { thr_h_int_handler();        }
        if( (((int_status_val_0)&(0x1000))>>12) == 1 ) { vbaton_undet_int_handler(); }
        if( (((int_status_val_0)&(0x2000))>>13) == 1 ) { bvalid_det_int_handler();   }
        if( (((int_status_val_0)&(0x4000))>>14) == 1 ) { chrdet_int_handler();       }
        if( (((int_status_val_0)&(0x8000))>>15) == 1 ) { ov_int_handler();           }                       
        //--------------------------------------------------------------------------------
        ret=pmic_read_interface(INT_STATUS1,(&int_status_val_1),0xFFFF,0x0);
        //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[INT] int_status_val_1=0x%x\n", int_status_val_1);

        if( (((int_status_val_1)&(0x0001))>>0) == 1 )  { ldo_int_handler();          }
        if( (((int_status_val_1)&(0x0002))>>1) == 1 )  { homekey_int_handler();      }
        if( (((int_status_val_1)&(0x0004))>>2) == 1 )  { accdet_int_handler();       }
        if( (((int_status_val_1)&(0x0008))>>3) == 1 )  { audio_int_handler();        }
        if( (((int_status_val_1)&(0x0010))>>4) == 1 )  { rtc_int_handler();          }
        if( (((int_status_val_1)&(0x0020))>>5) == 1 )  { printk("Undefined PMIC INT 5\n");        }
        if( (((int_status_val_1)&(0x0040))>>6) == 1 )  { printk("Undefined PMIC INT 6\n");        }
        if( (((int_status_val_1)&(0x0080))>>7) == 1 )  { printk("Undefined PMIC INT 7\n");        }
        if( (((int_status_val_1)&(0x0100))>>8) == 1 )  { vproc_int_handler();        }
        if( (((int_status_val_1)&(0x0200))>>9) == 1 )  { vsram_int_handler();        }
        if( (((int_status_val_1)&(0x0400))>>10) == 1 ) { vcore_int_handler();        }
        if( (((int_status_val_1)&(0x0800))>>11) == 1 ) { vm_int_handler();           }
        if( (((int_status_val_1)&(0x1000))>>12) == 1 ) { vio18_int_handler();        }
        if( (((int_status_val_1)&(0x2000))>>13) == 1 ) { vpa_int_handler();          }
        if( (((int_status_val_1)&(0x4000))>>14) == 1 ) { vrf18_int_handler();        }
        if( (((int_status_val_1)&(0x8000))>>15) == 1 ) { vrf18_2_int_handler();      }                   
        //--------------------------------------------------------------------------------

        mdelay(1);
        
        mt65xx_eint_unmask(CUST_EINT_MT6320_PMIC_NUM);

        //set INT_EN, in PMIC_EINT_SETTING()
        cust_pmic_interrupt_en_setting();

        mutex_unlock(&pmic_mutex);

        wait_event(pmic_thread_wq, pmic_thread_timeout);

        pmic_thread_timeout=0;
    }

    return 0;
}

void mt6320_pmic_eint_irq(void)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "6320 get int\n");

    //pmic internal
    wake_up_pmic();

    return ;
}

void PMIC_EINT_SETTING(void)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[PMIC_EINT_SETTING] start: CUST_EINT_MT6320_PMIC_NUM=%d\n",CUST_EINT_MT6320_PMIC_NUM);

    //ON/OFF interrupt
    cust_pmic_interrupt_en_setting();

    //GPIO Setting for early porting
    //mt_set_gpio_mode(GPIO37,GPIO_MODE_01); //EINT3 mode 1 on GPIO37
    mt_set_gpio_mode(GPIO_PMIC_EINT_PIN,GPIO_PMIC_EINT_PIN_M_EINT);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[CUST_EINT] %d,%d for usage\n", GPIO_PMIC_EINT_PIN, GPIO_PMIC_EINT_PIN_M_EINT);

    //EINT setting
    mt65xx_eint_set_sens(       CUST_EINT_MT6320_PMIC_NUM,
                                CUST_EINT_MT6320_PMIC_SENSITIVE);
    mt65xx_eint_set_polarity(   CUST_EINT_MT6320_PMIC_NUM,
                                CUST_EINT_MT6320_PMIC_POLARITY);        // set positive polarity
    mt65xx_eint_set_hw_debounce(CUST_EINT_MT6320_PMIC_NUM,
                                CUST_EINT_MT6320_PMIC_DEBOUNCE_CN);     // set debounce time
    mt65xx_eint_registration(   CUST_EINT_MT6320_PMIC_NUM,
                                CUST_EINT_MT6320_PMIC_DEBOUNCE_EN,
                                CUST_EINT_MT6320_PMIC_POLARITY,
                                mt6320_pmic_eint_irq,
                                0);

    mt65xx_eint_unmask(CUST_EINT_MT6320_PMIC_NUM);

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[CUST_EINT] CUST_EINT_MT6320_PMIC_NUM=%d\n", CUST_EINT_MT6320_PMIC_NUM);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[CUST_EINT] CUST_EINT_MT6320_PMIC_DEBOUNCE_CN=%d\n", CUST_EINT_MT6320_PMIC_DEBOUNCE_CN);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[CUST_EINT] CUST_EINT_MT6320_PMIC_POLARITY=%d\n", CUST_EINT_MT6320_PMIC_POLARITY);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[CUST_EINT] CUST_EINT_MT6320_PMIC_SENSITIVE=%d\n", CUST_EINT_MT6320_PMIC_SENSITIVE);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[CUST_EINT] CUST_EINT_MT6320_PMIC_DEBOUNCE_EN=%d\n", CUST_EINT_MT6320_PMIC_DEBOUNCE_EN);
}

void PMIC_DUMP_ALL_Register(void)
{
    kal_uint32 i=0;
    kal_uint32 ret=0;
    kal_uint32 reg_val=0;

    for (i=0;i<0xFFFF;i++)
    {
        ret=pmic_read_interface(i,&reg_val,0xFFFF,0);
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Reg[0x%x]=0x%x\n", i, reg_val);
    }
}

//==============================================================================
// PMIC read/write APIs
//==============================================================================
#define CONFIG_PMIC_HW_ACCESS_EN

static DEFINE_SPINLOCK(pmic_access_lock);

U32 pmic_read_interface (U32 RegNum, U32 *val, U32 MASK, U32 SHIFT)
{
    U32 return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
    U32 pmic6320_reg = 0;
    U32 rdata;
    unsigned long flags;

    spin_lock_irqsave(&pmic_access_lock, flags);

    //mt6320_read_byte(RegNum, &pmic6320_reg);
    return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
    pmic6320_reg=rdata;
    if(return_value!=0)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
        spin_unlock_irqrestore(&pmic_access_lock, flags);
        return return_value;
    }
    //xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[pmic_read_interface] Reg[%x]=0x%x\n", RegNum, pmic6320_reg);

    pmic6320_reg &= (MASK << SHIFT);
    *val = (pmic6320_reg >> SHIFT);
    //xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[pmic_read_interface] val=0x%x\n", *val);

    spin_unlock_irqrestore(&pmic_access_lock, flags);
#else
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_read_interface] Can not access HW PMIC\n");
#endif

    return return_value;
}

U32 pmic_config_interface (U32 RegNum, U32 val, U32 MASK, U32 SHIFT)
{
    U32 return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
    U32 pmic6320_reg = 0;
    U32 rdata;
    unsigned long flags;

    spin_lock_irqsave(&pmic_access_lock, flags);

    //1. mt6320_read_byte(RegNum, &pmic6320_reg);
    return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
    pmic6320_reg=rdata;
    if(return_value!=0)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
        spin_unlock_irqrestore(&pmic_access_lock, flags);
        return return_value;
    }
    //xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic6320_reg);

    pmic6320_reg &= ~(MASK << SHIFT);
    pmic6320_reg |= (val << SHIFT);

    //2. mt6320_write_byte(RegNum, pmic6320_reg);
    return_value= pwrap_wacs2(1, (RegNum), pmic6320_reg, &rdata);
    if(return_value!=0)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
        spin_unlock_irqrestore(&pmic_access_lock, flags);
        return return_value;
    }
    //xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic6320_reg);

    #if 0
    //3. Double Check
    //mt6320_read_byte(RegNum, &pmic6320_reg);
    return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
    pmic6320_reg=rdata;
    if(return_value!=0)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_config_interface] Reg[%x]= pmic_wrap write data fail\n", RegNum);
        spin_unlock_irqrestore(&pmic_access_lock, flags);
        return return_value;
    }
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic6320_reg);
    #endif

    spin_unlock_irqrestore(&pmic_access_lock, flags);
#else
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_config_interface] Can not access HW PMIC\n");
#endif

    return return_value;
}

//==============================================================================
// mt-pmic dev_attr APIs
//==============================================================================
U32 g_reg_value=0;
static ssize_t show_pmic_access(struct device *dev,struct device_attribute *attr, char *buf)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[show_pmic_access] 0x%x\n", g_reg_value);
    return sprintf(buf, "%u\n", g_reg_value);
}
static ssize_t store_pmic_access(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int ret=0;
    char *pvalue = NULL;
    U32 reg_value = 0;
    U32 reg_address = 0;
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[store_pmic_access] \n");
    if(buf != NULL && size != 0)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[store_pmic_access] buf is %s and size is %d \n",buf,size);
        reg_address = simple_strtoul(buf,&pvalue,16);

        if(size > 5)
        {
            reg_value = simple_strtoul((pvalue+1),NULL,16);
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[store_pmic_access] write PMU reg 0x%x with value 0x%x !\n",reg_address,reg_value);
            ret=pmic_config_interface(reg_address, reg_value, 0xFFFF, 0x0);
        }
        else
        {
            ret=pmic_read_interface(reg_address, &g_reg_value, 0xFFFF, 0x0);
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[store_pmic_access] read PMU reg 0x%x with value 0x%x !\n",reg_address,g_reg_value);
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[store_pmic_access] Please use \"cat pmic_access\" to get value\r\n");
        }
    }
    return size;
}
static DEVICE_ATTR(pmic_access, 0664, show_pmic_access, store_pmic_access); //664

//==============================================================================
// LDO EN APIs
//==============================================================================
void dct_pmic_VIO28_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VIO28_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_vio28_en(1);
    }
    else
    {
        upmu_set_vio28_en(0);
    }
}

void dct_pmic_VUSB_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VUSB_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vusb_en(1);
    }
    else
    {
        upmu_set_rg_vusb_en(0);
    }
}

void dct_pmic_VMC1_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VMC1_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vmc1_en(1);
    }
    else
    {
        upmu_set_rg_vmc1_en(0);
    }
}

void dct_pmic_VMCH1_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VMCH1_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vmch1_en(1);
    }
    else
    {
        upmu_set_rg_vmch1_en(0);
    }
}

void dct_pmic_VEMC_3V3_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VEMC_3V3_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vemc_3v3_en(1);
    }
    else
    {
        upmu_set_rg_vemc_3v3_en(0);
    }
}

void dct_pmic_VEMC_1V8_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VEMC_1V8_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vemc_1v8_en(1);
    }
    else
    {
        upmu_set_rg_vemc_1v8_en(0);
    }
}

void dct_pmic_VGP1_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VGP1_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vgp1_en(1);
    }
    else
    {
        upmu_set_rg_vgp1_en(0);
    }
}

void dct_pmic_VGP2_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VGP2_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vgp2_en(1);
    }
    else
    {
        upmu_set_rg_vgp2_en(0);
    }
}

void dct_pmic_VGP3_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VGP3_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vgp3_en(1);
    }
    else
    {
        upmu_set_rg_vgp3_en(0);
    }
}

void dct_pmic_VGP4_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VGP4_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vgp4_en(1);
    }
    else
    {
        upmu_set_rg_vgp4_en(0);
    }
}

void dct_pmic_VGP5_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VGP5_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vgp5_en(1);
    }
    else
    {
        upmu_set_rg_vgp5_en(0);
    }
}

void dct_pmic_VGP6_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VGP6_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vgp6_en(1);
    }
    else
    {
        upmu_set_rg_vgp6_en(0);
    }
}

void dct_pmic_VSIM1_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VSIM1_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vsim1_en(1);
    }
    else
    {
        upmu_set_rg_vsim1_en(0);
    }
}

void dct_pmic_VSIM2_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VSIM2_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vsim2_en(1);
    }
    else
    {
        upmu_set_rg_vsim2_en(0);
    }
}

void dct_pmic_VIBR_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VIBR_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vibr_en(1);
    }
    else
    {
        upmu_set_rg_vibr_en(0);
    }
}

void dct_pmic_VRTC_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VRTC_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_vrtc_en(1);
    }
    else
    {
        upmu_set_vrtc_en(0);
    }
}

void dct_pmic_VAST_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VAST_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vast_en(1);
    }
    else
    {
        upmu_set_rg_vast_en(0);
    }
}

void dct_pmic_VRF28_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VRF28_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vrf28_en(1);
    }
    else
    {
        upmu_set_rg_vrf28_en(0);
    }
}

void dct_pmic_VRF28_2_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VRF28_2_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vrf28_2_en(1);
    }
    else
    {
        upmu_set_rg_vrf28_2_en(0);
    }
}

void dct_pmic_VTCXO_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VTCXO_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vtcxo_en(1);
    }
    else
    {
        upmu_set_rg_vtcxo_en(0);
    }
}

void dct_pmic_VTCXO_2_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VTCXO_2_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vtcxo_2_en(1);
    }
    else
    {
        upmu_set_rg_vtcxo_2_en(0);
    }
}

void dct_pmic_VA_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VA_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_va_en(1);
    }
    else
    {
        upmu_set_rg_va_en(0);
    }
}

void dct_pmic_VA28_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VA28_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_va28_en(1);
    }
    else
    {
        upmu_set_rg_va28_en(0);
    }
}

void dct_pmic_VCAMA_enable(kal_bool dctEnable)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VCAMA_enable] %d\n", dctEnable);

    if(dctEnable == KAL_TRUE)
    {
        upmu_set_rg_vcama_en(1);
    }
    else
    {
        upmu_set_rg_vcama_en(0);
    }
}

//==============================================================================
// LDO SEL APIs
//==============================================================================
void dct_pmic_VIO28_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VIO28_enable] No vlotage can setting!\n");
}

void dct_pmic_VUSB_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VUSB_sel] No vlotage can setting!\n");
}

void dct_pmic_VMC1_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VMC1_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vmc1_vosel(1);}
    else if(volt == VOL_3300){ upmu_set_rg_vmc1_vosel(1);}
    else if(volt == VOL_1800){ upmu_set_rg_vmc1_vosel(0);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VMCH1_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VMCH1_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vmch1_vosel(1);}
    else if(volt == VOL_3000){ upmu_set_rg_vmch1_vosel(0);}
    else if(volt == VOL_3300){ upmu_set_rg_vmch1_vosel(1);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VEMC_3V3_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VEMC_3V3_sel] value=%d \n", volt);
    
    if(volt == VOL_DEFAULT)  { upmu_set_rg_vemc_3v3_vosel(1);}
    else if(volt == VOL_3000){ upmu_set_rg_vemc_3v3_vosel(0);}
    else if(volt == VOL_3300){ upmu_set_rg_vemc_3v3_vosel(1);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }    
}

void dct_pmic_VEMC_1V8_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VEMC_1V8_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vemc_1v8_vosel(3);}
    else if(volt == VOL_1200){ upmu_set_rg_vemc_1v8_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vemc_1v8_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vemc_1v8_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vemc_1v8_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vemc_1v8_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vemc_1v8_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vemc_1v8_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vemc_1v8_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VGP1_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VGP1_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vgp1_vosel(5);}
    else if(volt == VOL_1200){ upmu_set_rg_vgp1_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vgp1_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vgp1_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vgp1_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vgp1_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vgp1_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vgp1_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vgp1_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VGP2_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VGP2_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vgp2_vosel(5);}
    else if(volt == VOL_1200){ upmu_set_rg_vgp2_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vgp2_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vgp2_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vgp2_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vgp2_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vgp2_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vgp2_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vgp2_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VGP3_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VGP3_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vgp3_vosel(5);}
    else if(volt == VOL_1200){ upmu_set_rg_vgp3_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vgp3_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vgp3_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vgp3_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vgp3_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vgp3_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vgp3_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vgp3_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VGP4_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VGP4_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vgp4_vosel(5);}
    else if(volt == VOL_1200){ upmu_set_rg_vgp4_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vgp4_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vgp4_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vgp4_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vgp4_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vgp4_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vgp4_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vgp4_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VGP5_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VGP5_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vgp5_vosel(5);}
    else if(volt == VOL_1200){ upmu_set_rg_vgp5_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vgp5_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vgp5_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vgp5_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vgp5_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vgp5_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vgp5_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vgp5_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VGP6_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VGP6_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vgp6_vosel(5);}
    else if(volt == VOL_1200){ upmu_set_rg_vgp6_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vgp6_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vgp6_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vgp6_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vgp6_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vgp6_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vgp6_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vgp6_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VSIM1_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VSIM1_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vsim1_vosel(5);}
    else if(volt == VOL_1200){ upmu_set_rg_vsim1_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vsim1_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vsim1_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vsim1_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vsim1_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vsim1_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vsim1_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vsim1_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VSIM2_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VSIM2_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vsim2_vosel(5);}
    else if(volt == VOL_1200){ upmu_set_rg_vsim2_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vsim2_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vsim2_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vsim2_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vsim2_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vsim2_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vsim2_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vsim2_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VIBR_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VIBR_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vibr_vosel(5);}
    else if(volt == VOL_1200){ upmu_set_rg_vibr_vosel(0);}
    else if(volt == VOL_1300){ upmu_set_rg_vibr_vosel(1);}
    else if(volt == VOL_1500){ upmu_set_rg_vibr_vosel(2);}
    else if(volt == VOL_1800){ upmu_set_rg_vibr_vosel(3);}
    else if(volt == VOL_2500){ upmu_set_rg_vibr_vosel(4);}
    else if(volt == VOL_2800){ upmu_set_rg_vibr_vosel(5);}
    else if(volt == VOL_3000){ upmu_set_rg_vibr_vosel(6);}
    else if(volt == VOL_3300){ upmu_set_rg_vibr_vosel(7);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VRTC_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VRTC_sel] No vlotage can setting!\n");
}

void dct_pmic_VAST_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VAST_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vast_vosel(0);}
    else if(volt == VOL_1200){ upmu_set_rg_vast_vosel(0);}
    else if(volt == VOL_1100){ upmu_set_rg_vast_vosel(1);}
    else if(volt == VOL_1000){ upmu_set_rg_vast_vosel(2);}
    else if(volt == VOL_0900){ upmu_set_rg_vast_vosel(3);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VRF28_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VRF28_sel] value=%d \n", volt);
    
    if(volt == VOL_DEFAULT)  { upmu_set_rg_vrf28_vosel(1);}
    else if(volt == VOL_2800){ upmu_set_rg_vrf28_vosel(1);}
    else if(volt == VOL_1800){ upmu_set_rg_vrf28_vosel(0);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VRF28_2_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VRF28_2_sel] value=%d \n", volt);
    
    if(volt == VOL_DEFAULT)  { upmu_set_rg_vrf28_2_vosel(1);}
    else if(volt == VOL_2800){ upmu_set_rg_vrf28_2_vosel(1);}
    else if(volt == VOL_1800){ upmu_set_rg_vrf28_2_vosel(0);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VTCXO_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VTCXO_sel] No vlotage can setting!\n");
}

void dct_pmic_VTCXO_2_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VTCXO_2_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vtcxo_2_vosel(1);}
    else if(volt == VOL_2800){ upmu_set_rg_vtcxo_2_vosel(1);}
    else if(volt == VOL_1800){ upmu_set_rg_vtcxo_2_vosel(0);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VA_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VA_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_va_vosel(0);}
    else if(volt == VOL_1800){ upmu_set_rg_va_vosel(0);}
    else if(volt == VOL_2500){ upmu_set_rg_va_vosel(1);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

void dct_pmic_VA28_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[dct_pmic_VA28_sel] No vlotage can setting!\n");
}

void dct_pmic_VCAMA_sel(kal_uint32 volt)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[dct_pmic_VCAMA_sel] value=%d \n", volt);

    if(volt == VOL_DEFAULT)     { upmu_set_rg_vcama_vosel(3);}
    else if(volt == VOL_1500){ upmu_set_rg_vcama_vosel(0);}
    else if(volt == VOL_1800){ upmu_set_rg_vcama_vosel(1);}
    else if(volt == VOL_2500){ upmu_set_rg_vcama_vosel(2);}
    else if(volt == VOL_2800){ upmu_set_rg_vcama_vosel(3);}
    else{
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Error Setting %d. DO nothing.\r\n", volt);
    }
}

//==============================================================================
// LDO EN & SEL common API
//==============================================================================
void pmic_ldo_enable(MT65XX_POWER powerId, kal_bool powerEnable)
{
    //xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[pmic_ldo_enable] Receive powerId %d, action is %d\n", powerId, powerEnable);

    //Need integrate with DCT : using DCT APIs

    if(     powerId == MT65XX_POWER_LDO_VIO28)      { dct_pmic_VIO28_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VUSB)       { dct_pmic_VUSB_enable(powerEnable); }
//<2013/04/30-24462-stevenchen, [Pelican][drv] Turn off VMC_3V3 for saving power.
//<2013/01/18-20565-stevenchen, Fix micro SD can't be detected.
    else if(powerId == MT65XX_POWER_LDO_VMC1)	    { dct_pmic_VMC1_enable(powerEnable); }
    else if( (powerId == MT65XX_POWER_LDO_VMCH1) && (powerEnable==1) )
    { 
	dct_pmic_VMCH1_enable(powerEnable); 
    }
//>2013/01/18-20565-stevenchen
//>2013/04/30-24462-stevenchen
    else if(powerId == MT65XX_POWER_LDO_VEMC_3V3)    { dct_pmic_VEMC_3V3_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VEMC_1V8)    { dct_pmic_VEMC_1V8_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VGP1)        { dct_pmic_VGP1_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VGP2)        { dct_pmic_VGP2_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VGP3)        { dct_pmic_VGP3_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VGP4)        { dct_pmic_VGP4_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VGP5)        { dct_pmic_VGP5_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VGP6)        { dct_pmic_VGP6_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VSIM1)        { dct_pmic_VSIM1_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VSIM2)        { dct_pmic_VSIM2_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VIBR)        { dct_pmic_VIBR_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VRTC)        { dct_pmic_VRTC_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VAST)        { dct_pmic_VAST_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VRF28)        { dct_pmic_VRF28_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VRF28_2)    { dct_pmic_VRF28_2_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VTCXO)        { dct_pmic_VTCXO_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VTCXO_2)    { dct_pmic_VTCXO_2_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VA)            { dct_pmic_VA_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VA28)        { dct_pmic_VA28_enable(powerEnable); }
    else if(powerId == MT65XX_POWER_LDO_VCAMA)        { dct_pmic_VCAMA_enable(powerEnable); }
    else
    {
        xlog_printk(ANDROID_LOG_WARN, "Power/PMIC", "[pmic_ldo_enable] UnKnown powerId (%d)\n", powerId);
    }
}

void pmic_ldo_vol_sel(MT65XX_POWER powerId, MT65XX_POWER_VOLTAGE powerVolt)
{
    //xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[pmic_ldo_vol_sel] Receive powerId %d, action is %d\n", powerId, powerVolt);

    //Need integrate with DCT : using DCT APIs

    if(     powerId == MT65XX_POWER_LDO_VIO28)      {    dct_pmic_VIO28_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VUSB)       {    dct_pmic_VUSB_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VMC1)        {    dct_pmic_VMC1_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VMCH1)        {    dct_pmic_VMCH1_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VEMC_3V3)    {    dct_pmic_VEMC_3V3_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VEMC_1V8)    {    dct_pmic_VEMC_1V8_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VGP1)        {    dct_pmic_VGP1_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VGP2)        {    dct_pmic_VGP2_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VGP3)        {    dct_pmic_VGP3_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VGP4)        {    dct_pmic_VGP4_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VGP5)        {    dct_pmic_VGP5_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VGP6)        {    dct_pmic_VGP6_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VSIM1)        {    dct_pmic_VSIM1_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VSIM2)        {    dct_pmic_VSIM2_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VIBR)        {    dct_pmic_VIBR_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VRTC)        {    dct_pmic_VRTC_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VAST)        {    dct_pmic_VAST_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VRF28)        {    dct_pmic_VRF28_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VRF28_2)    {    dct_pmic_VRF28_2_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VTCXO)        {    dct_pmic_VTCXO_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VTCXO_2)    {    dct_pmic_VTCXO_2_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VA)            {    dct_pmic_VA_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VA28)        {    dct_pmic_VA28_sel(powerVolt); }
    else if(powerId == MT65XX_POWER_LDO_VCAMA)        {    dct_pmic_VCAMA_sel(powerVolt); }
    else
    {
        xlog_printk(ANDROID_LOG_WARN, "Power/PMIC", "[pmic_ldo_ldo_vol_sel] UnKnown powerId (%d)\n", powerId);
    }
}

//==============================================================================
// EM
//==============================================================================
static ssize_t show_BUCK_VPROC_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x214;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 13);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VPROC_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VPROC_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VPROC_STATUS, 0664, show_BUCK_VPROC_STATUS, store_BUCK_VPROC_STATUS);

static ssize_t show_BUCK_VSRAM_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x23A;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 13);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VSRAM_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VSRAM_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VSRAM_STATUS, 0664, show_BUCK_VSRAM_STATUS, store_BUCK_VSRAM_STATUS);

static ssize_t show_BUCK_VCORE_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x266;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 13);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VCORE_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VCORE_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VCORE_STATUS, 0664, show_BUCK_VCORE_STATUS, store_BUCK_VCORE_STATUS);

static ssize_t show_BUCK_VM_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x28C;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 13);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VM_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VM_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VM_STATUS, 0664, show_BUCK_VM_STATUS, store_BUCK_VM_STATUS);

static ssize_t show_BUCK_VIO18_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x30E;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 13);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VIO18_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VIO18_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VIO18_STATUS, 0664, show_BUCK_VIO18_STATUS, store_BUCK_VIO18_STATUS);

static ssize_t show_BUCK_VPA_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x334;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 13);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VPA_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VPA_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VPA_STATUS, 0664, show_BUCK_VPA_STATUS, store_BUCK_VPA_STATUS);

static ssize_t show_BUCK_VRF18_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x35E;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 13);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VRF18_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VRF18_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VRF18_STATUS, 0664, show_BUCK_VRF18_STATUS, store_BUCK_VRF18_STATUS);

static ssize_t show_BUCK_VRF18_2_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x388;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 13);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VRF18_2_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VRF18_2_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VRF18_2_STATUS, 0664, show_BUCK_VRF18_2_STATUS, store_BUCK_VRF18_2_STATUS);

static ssize_t show_LDO_VIO28_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x420;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VIO28_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VIO28_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VIO28_STATUS, 0664, show_LDO_VIO28_STATUS, store_LDO_VIO28_STATUS);

static ssize_t show_LDO_VUSB_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x422;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VUSB_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VUSB_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VUSB_STATUS, 0664, show_LDO_VUSB_STATUS, store_LDO_VUSB_STATUS);

static ssize_t show_LDO_VMC1_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x424;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VMC1_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VMC1_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VMC1_STATUS, 0664, show_LDO_VMC1_STATUS, store_LDO_VMC1_STATUS);

static ssize_t show_LDO_VMCH1_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x426;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VMCH1_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VMCH1_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VMCH1_STATUS, 0664, show_LDO_VMCH1_STATUS, store_LDO_VMCH1_STATUS);

static ssize_t show_LDO_VEMC_3V3_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x428;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VEMC_3V3_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VEMC_3V3_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VEMC_3V3_STATUS, 0664, show_LDO_VEMC_3V3_STATUS, store_LDO_VEMC_3V3_STATUS);

static ssize_t show_LDO_VEMC_1V8_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x462;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VEMC_1V8_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VEMC_1V8_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VEMC_1V8_STATUS, 0664, show_LDO_VEMC_1V8_STATUS, store_LDO_VEMC_1V8_STATUS);

static ssize_t show_LDO_VGP1_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x42A;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP1_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP1_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP1_STATUS, 0664, show_LDO_VGP1_STATUS, store_LDO_VGP1_STATUS);

static ssize_t show_LDO_VGP2_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x42C;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP2_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP2_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP2_STATUS, 0664, show_LDO_VGP2_STATUS, store_LDO_VGP2_STATUS);

static ssize_t show_LDO_VGP3_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x42E;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP3_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP3_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP3_STATUS, 0664, show_LDO_VGP3_STATUS, store_LDO_VGP3_STATUS);

static ssize_t show_LDO_VGP4_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x430;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP4_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP4_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP4_STATUS, 0664, show_LDO_VGP4_STATUS, store_LDO_VGP4_STATUS);

static ssize_t show_LDO_VGP5_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x432;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP5_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP5_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP5_STATUS, 0664, show_LDO_VGP5_STATUS, store_LDO_VGP5_STATUS);

static ssize_t show_LDO_VGP6_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x434;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP6_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP6_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP6_STATUS, 0664, show_LDO_VGP6_STATUS, store_LDO_VGP6_STATUS);

static ssize_t show_LDO_VSIM1_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x436;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VSIM1_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VSIM1_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VSIM1_STATUS, 0664, show_LDO_VSIM1_STATUS, store_LDO_VSIM1_STATUS);

static ssize_t show_LDO_VSIM2_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x438;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VSIM2_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VSIM2_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VSIM2_STATUS, 0664, show_LDO_VSIM2_STATUS, store_LDO_VSIM2_STATUS);

static ssize_t show_LDO_VIBR_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x466;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VIBR_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VIBR_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VIBR_STATUS, 0664, show_LDO_VIBR_STATUS, store_LDO_VIBR_STATUS);

static ssize_t show_LDO_VRTC_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x43A;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VRTC_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VRTC_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VRTC_STATUS, 0664, show_LDO_VRTC_STATUS, store_LDO_VRTC_STATUS);

static ssize_t show_LDO_VAST_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x444;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VAST_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VAST_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VAST_STATUS, 0664, show_LDO_VAST_STATUS, store_LDO_VAST_STATUS);

static ssize_t show_LDO_VRF28_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x400;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VRF28_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VRF28_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VRF28_STATUS, 0664, show_LDO_VRF28_STATUS, store_LDO_VRF28_STATUS);

static ssize_t show_LDO_VRF28_2_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x41A;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VRF28_2_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VRF28_2_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VRF28_2_STATUS, 0664, show_LDO_VRF28_2_STATUS, store_LDO_VRF28_2_STATUS);

static ssize_t show_LDO_VTCXO_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x402;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VTCXO_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VTCXO_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VTCXO_STATUS, 0664, show_LDO_VTCXO_STATUS, store_LDO_VTCXO_STATUS);

static ssize_t show_LDO_VTCXO_2_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x41C;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VTCXO_2_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VTCXO_2_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VTCXO_2_STATUS, 0664, show_LDO_VTCXO_2_STATUS, store_LDO_VTCXO_2_STATUS);

static ssize_t show_LDO_VA_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x404;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VA_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VA_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VA_STATUS, 0664, show_LDO_VA_STATUS, store_LDO_VA_STATUS);

static ssize_t show_LDO_VA28_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x406;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VA28_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VA28_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VA28_STATUS, 0664, show_LDO_VA28_STATUS, store_LDO_VA28_STATUS);

static ssize_t show_LDO_VCAMA_STATUS(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x408;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 15);
    ret_value = reg_val;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VCAMA_STATUS : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VCAMA_STATUS(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VCAMA_STATUS, 0664, show_LDO_VCAMA_STATUS, store_LDO_VCAMA_STATUS);

// voltage ---------------------------------------------------------------------------------

static ssize_t show_BUCK_VPROC_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x21E;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x7F, 0);
    ret_value = 70000 + (reg_val*625);
    ret_value = ret_value / 100;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VPROC_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VPROC_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VPROC_VOLTAGE, 0664, show_BUCK_VPROC_VOLTAGE, store_BUCK_VPROC_VOLTAGE);

static ssize_t show_BUCK_VSRAM_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x244;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x7F, 0);
    ret_value = 70000 + (reg_val*625);
    ret_value = ret_value / 100;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VSRAM_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VSRAM_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VSRAM_VOLTAGE, 0664, show_BUCK_VSRAM_VOLTAGE, store_BUCK_VSRAM_VOLTAGE);

static ssize_t show_BUCK_VCORE_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x270;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x7F, 0);
    ret_value = 70000 + (reg_val*625);
    ret_value = ret_value / 100;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VCORE_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VCORE_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VCORE_VOLTAGE, 0664, show_BUCK_VCORE_VOLTAGE, store_BUCK_VCORE_VOLTAGE);

static ssize_t show_BUCK_VM_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x296;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x7F, 0);
    ret_value = 70000 + (reg_val*625);
    ret_value = ret_value / 100;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VM_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VM_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VM_VOLTAGE, 0664, show_BUCK_VM_VOLTAGE, store_BUCK_VM_VOLTAGE);

static ssize_t show_BUCK_VIO18_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x318;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x1F, 0);
    ret_value = 1500 + (reg_val*20);    
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VIO18_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VIO18_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VIO18_VOLTAGE, 0664, show_BUCK_VIO18_VOLTAGE, store_BUCK_VIO18_VOLTAGE);

static ssize_t show_BUCK_VPA_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x33E;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x3F, 0);
    ret_value = 500 + (reg_val*50);
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VPA_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VPA_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VPA_VOLTAGE, 0664, show_BUCK_VPA_VOLTAGE, store_BUCK_VPA_VOLTAGE);

static ssize_t show_BUCK_VRF18_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x368;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x1F, 0);
    ret_value = 1050 + (reg_val*25);
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VRF18_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VRF18_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VRF18_VOLTAGE, 0664, show_BUCK_VRF18_VOLTAGE, store_BUCK_VRF18_VOLTAGE);

static ssize_t show_BUCK_VRF18_2_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x392;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x1F, 0);
    ret_value = 1050 + (reg_val*25);
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] BUCK_VRF18_2_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_BUCK_VRF18_2_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(BUCK_VRF18_2_VOLTAGE, 0664, show_BUCK_VRF18_2_VOLTAGE, store_BUCK_VRF18_2_VOLTAGE);

static ssize_t show_LDO_VIO28_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
            
    ret_value = 2800;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VIO28_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VIO28_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VIO28_VOLTAGE, 0664, show_LDO_VIO28_VOLTAGE, store_LDO_VIO28_VOLTAGE);

static ssize_t show_LDO_VUSB_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
   
    ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VUSB_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VUSB_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VUSB_VOLTAGE, 0664, show_LDO_VUSB_VOLTAGE, store_LDO_VUSB_VOLTAGE);

static ssize_t show_LDO_VMC1_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x44A;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 4);
    if(reg_val == 0)
        ret_value = 1800;
    else if(reg_val == 1)
        ret_value = 3300;        
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VMC1_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VMC1_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VMC1_VOLTAGE, 0664, show_LDO_VMC1_VOLTAGE, store_LDO_VMC1_VOLTAGE);

static ssize_t show_LDO_VMCH1_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x44C;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 7);
    if(reg_val == 0)
        ret_value = 3000;
    else if(reg_val == 1)
        ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VMCH1_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VMCH1_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VMCH1_VOLTAGE, 0664, show_LDO_VMCH1_VOLTAGE, store_LDO_VMCH1_VOLTAGE);

static ssize_t show_LDO_VEMC_3V3_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x44E;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 7);
    if(reg_val == 0)
        ret_value = 3000;
    else if(reg_val == 1)
        ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VEMC_3V3_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VEMC_3V3_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VEMC_3V3_VOLTAGE, 0664, show_LDO_VEMC_3V3_VOLTAGE, store_LDO_VEMC_3V3_VOLTAGE);

static ssize_t show_LDO_VEMC_1V8_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x464;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300;        
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VEMC_1V8_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VEMC_1V8_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VEMC_1V8_VOLTAGE, 0664, show_LDO_VEMC_1V8_VOLTAGE, store_LDO_VEMC_1V8_VOLTAGE);

static ssize_t show_LDO_VGP1_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x450;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300; 
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP1_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP1_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP1_VOLTAGE, 0664, show_LDO_VGP1_VOLTAGE, store_LDO_VGP1_VOLTAGE);

static ssize_t show_LDO_VGP2_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x452;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300; 
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP2_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP2_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP2_VOLTAGE, 0664, show_LDO_VGP2_VOLTAGE, store_LDO_VGP2_VOLTAGE);

static ssize_t show_LDO_VGP3_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x454;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP3_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP3_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP3_VOLTAGE, 0664, show_LDO_VGP3_VOLTAGE, store_LDO_VGP3_VOLTAGE);

static ssize_t show_LDO_VGP4_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x456;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP4_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP4_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP4_VOLTAGE, 0664, show_LDO_VGP4_VOLTAGE, store_LDO_VGP4_VOLTAGE);

static ssize_t show_LDO_VGP5_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x458;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP5_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP5_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP5_VOLTAGE, 0664, show_LDO_VGP5_VOLTAGE, store_LDO_VGP5_VOLTAGE);

static ssize_t show_LDO_VGP6_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x45A;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VGP6_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VGP6_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VGP6_VOLTAGE, 0664, show_LDO_VGP6_VOLTAGE, store_LDO_VGP6_VOLTAGE);

static ssize_t show_LDO_VSIM1_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x45C;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VSIM1_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VSIM1_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VSIM1_VOLTAGE, 0664, show_LDO_VSIM1_VOLTAGE, store_LDO_VSIM1_VOLTAGE);

static ssize_t show_LDO_VSIM2_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x45E;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VSIM2_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VSIM2_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VSIM2_VOLTAGE, 0664, show_LDO_VSIM2_VOLTAGE, store_LDO_VSIM2_VOLTAGE);

static ssize_t show_LDO_VIBR_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x468;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x07, 5);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1300;
    else if(reg_val == 2)
        ret_value = 1500;
    else if(reg_val == 3)
        ret_value = 1800;        
    else if(reg_val == 4)
        ret_value = 2500;
    else if(reg_val == 5)
        ret_value = 2800;
    else if(reg_val == 6)
        ret_value = 3000;
    else if(reg_val == 7)
        ret_value = 3300;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VIBR_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VIBR_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VIBR_VOLTAGE, 0664, show_LDO_VIBR_VOLTAGE, store_LDO_VIBR_VOLTAGE);

static ssize_t show_LDO_VRTC_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    ret_value = 2800;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VRTC_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VRTC_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VRTC_VOLTAGE, 0664, show_LDO_VRTC_VOLTAGE, store_LDO_VRTC_VOLTAGE);

static ssize_t show_LDO_VAST_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x444;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x03, 13);
    if(reg_val == 0)
        ret_value = 1200;
    else if(reg_val == 1)
        ret_value = 1100;
    else if(reg_val == 2)
        ret_value = 1000;
    else if(reg_val == 3)
        ret_value = 900;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VAST_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VAST_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VAST_VOLTAGE, 0664, show_LDO_VAST_VOLTAGE, store_LDO_VAST_VOLTAGE);

static ssize_t show_LDO_VRF28_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x412;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 3);
    if(reg_val == 0)
        ret_value = 1800;
    else if(reg_val == 1)
        ret_value = 2800;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VRF28_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VRF28_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VRF28_VOLTAGE, 0664, show_LDO_VRF28_VOLTAGE, store_LDO_VRF28_VOLTAGE);

static ssize_t show_LDO_VRF28_2_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x418;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 3);
    if(reg_val == 0)
        ret_value = 1800;
    else if(reg_val == 1)
        ret_value = 2800;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VRF28_2_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VRF28_2_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VRF28_2_VOLTAGE, 0664, show_LDO_VRF28_2_VOLTAGE, store_LDO_VRF28_2_VOLTAGE);

static ssize_t show_LDO_VTCXO_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    ret_value = 2800;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VTCXO_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VTCXO_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VTCXO_VOLTAGE, 0664, show_LDO_VTCXO_VOLTAGE, store_LDO_VTCXO_VOLTAGE);

static ssize_t show_LDO_VTCXO_2_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x416;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 3);
    if(reg_val == 0)
        ret_value = 1800;
    else if(reg_val == 1)
        ret_value = 2800;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VTCXO_2_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VTCXO_2_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VTCXO_2_VOLTAGE, 0664, show_LDO_VTCXO_2_VOLTAGE, store_LDO_VTCXO_2_VOLTAGE);

static ssize_t show_LDO_VA_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x410;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x01, 6);
    if(reg_val == 0)
        ret_value = 1800;
    else if(reg_val == 1)
        ret_value = 2500;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VA_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VA_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VA_VOLTAGE, 0664, show_LDO_VA_VOLTAGE, store_LDO_VA_VOLTAGE);

static ssize_t show_LDO_VA28_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    ret_value = 2800;
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VA28_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VA28_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VA28_VOLTAGE, 0664, show_LDO_VA28_VOLTAGE, store_LDO_VA28_VOLTAGE);

static ssize_t show_LDO_VCAMA_VOLTAGE(struct device *dev,struct device_attribute *attr, char *buf)
{
    kal_uint32 ret_value=0;
    
    kal_uint32 ret=0;
    kal_uint32 reg_address=0x414;
    kal_uint32 reg_val=0;
    
    ret = pmic_read_interface(reg_address, &reg_val, 0x03, 6);
    if(reg_val == 0)
        ret_value = 1500;
    else if(reg_val == 1)
        ret_value = 1800;
    else if(reg_val == 2)
        ret_value = 2500;
    else if(reg_val == 3)
        ret_value = 2800;        
    
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] LDO_VCAMA_VOLTAGE : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_LDO_VCAMA_VOLTAGE(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    xlog_printk(ANDROID_LOG_DEBUG, "Power/PMIC", "[EM] Not Support Write Function\n");    
    return size;
}
static DEVICE_ATTR(LDO_VCAMA_VOLTAGE, 0664, show_LDO_VCAMA_VOLTAGE, store_LDO_VCAMA_VOLTAGE);


//==============================================================================
// PMIC6320 device driver
//==============================================================================
void ldo_service_test(void)
{
    hwPowerOn(MT65XX_POWER_LDO_VIO28,    VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VUSB,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VMC1,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VMCH1,    VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VEMC_3V3, VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VGP1,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VGP2,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VGP3,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VGP4,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VGP5,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VGP6,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VSIM1,    VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VSIM2,    VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VIBR,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VRTC,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VAST,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VRF28,    VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VRF28_2,  VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VTCXO,    VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VTCXO_2,  VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VA,       VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VA28,     VOL_DEFAULT, "ldo_test");
    hwPowerOn(MT65XX_POWER_LDO_VCAMA,    VOL_DEFAULT, "ldo_test");

    hwPowerDown(MT65XX_POWER_LDO_VIO28,     "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VUSB,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VMC1,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VMCH1,     "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VEMC_3V3,  "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,  "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VSIM1,     "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VSIM2,     "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VRTC,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VAST,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VRF28,     "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VRF28_2,   "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VTCXO,     "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VTCXO_2,   "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VA,        "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VA28,      "ldo_test");
    hwPowerDown(MT65XX_POWER_LDO_VCAMA,     "ldo_test");
}

void PMIC_INIT_SETTING_V1(void)
{
    U32 chip_version = 0;
    U32 ret=0;

    chip_version = upmu_get_cid();

    if(chip_version >= PMIC6320_E2_CID_CODE)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[Kernel_PMIC_INIT_SETTING_V1] PMIC Chip = %x\n",chip_version);
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[Kernel_PMIC_INIT_SETTING_V1] 20121220 (>=2)\n");

        //put init setting from DE/SA
        
        ret = pmic_config_interface(0x254,0x58,0x7F,8); // [14:8]: VSRAM_VOSEL_ON_HB; Set before 0x0236
        ret = pmic_config_interface(0x254,0x38,0x7F,0); // [6:0]: VSRAM_VOSEL_ON_LB; Set before 0x0236
        ret = pmic_config_interface(0x256,0x38,0x7F,0); // [6:0]: VSRAM_VOSEL_SLEEP_LB; Set before 0x0236
        ret = pmic_config_interface(0x2,0xB,0xF,4); // [7:4]: RG_VCDT_HV_VTH; 7V OVP
        ret = pmic_config_interface(0xC,0x2,0x7,1); // [3:1]: RG_VBAT_OV_VTH; VBAT_OV=4.3V
        ret = pmic_config_interface(0x1A,0x3,0xF,0); // [3:0]: RG_CHRWDT_TD; align to 6250's
        ret = pmic_config_interface(0x2A,0x0,0x7,4); // [6:4]: RG_CSDAC_STP; align to 6250's setting
        ret = pmic_config_interface(0x2E,0x1,0x1,7); // [7:7]: RG_ULC_DET_EN; 
        ret = pmic_config_interface(0x2E,0x1,0x1,6); // [6:6]: RG_HWCV_EN; 
        ret = pmic_config_interface(0x2E,0x1,0x1,2); // [2:2]: RG_CSDAC_MODE; 
        ret = pmic_config_interface(0x14C,0x0,0xF,12); // [15:12]: RG_SIMLS2_SRST_CONF; Set PAD_SIMLS2_SRST slow skew
        ret = pmic_config_interface(0x14C,0x0,0xF,8); // [11:8]: RG_SIMLS2_SCLK_CONF; Set PAD_SIMLS2_SCLK slow skew
        ret = pmic_config_interface(0x14C,0x0,0xF,4); // [7:4]: RG_SIMLS1_SRST_CONF; Set PAD_SIMLS1_SRST slow skew
        ret = pmic_config_interface(0x14C,0x0,0xF,0); // [3:0]: RG_SIMLS1_SCLK_CONF; Set PAD_SIMLS1_SCLK slow skew
        ret = pmic_config_interface(0x156,0x1,0x1,3); // [3:3]: RG_SMT3; SRCLKEN_MD2 SMT enable
        ret = pmic_config_interface(0x156,0x1,0x1,2); // [2:2]: RG_SMT2; SRCLKEN_PERI SMT enable
        ret = pmic_config_interface(0x156,0x1,0x1,1); // [1:1]: RG_SMT1; SRCVOLTEN SMT enable
        ret = pmic_config_interface(0x15A,0x1,0x1,15); // [15:15]: RG_SMT47; Set PAD_SIMLS2_SCLK schmit input
        ret = pmic_config_interface(0x15A,0x1,0x1,14); // [14:14]: RG_SMT46; Set PAD_SIMLS1_SRST schmit input
        ret = pmic_config_interface(0x15A,0x1,0x1,13); // [13:13]: RG_SMT45; Set PAD_SIMLS1_SCLK schmit input
        ret = pmic_config_interface(0x15C,0x1,0x1,0); // [0:0]: RG_SMT48; Set PAD_SIMLS2_SRST schmit input
        ret = pmic_config_interface(0x174,0x2,0xF,12); // [15:12]: RG_OCTL_SIMLS1_SRST; Set PAD_SIMLS1_SRST driving to 4mA
        ret = pmic_config_interface(0x174,0x2,0xF,8); // [11:8]: RG_OCTL_SIMLS1_SCLK; Set PAD_SIMLS1_SCLK driving to 4mA
        ret = pmic_config_interface(0x176,0x2,0xF,4); // [7:4]: RG_OCTL_SIMLS2_SRST; Set PAD_SIMLS2_SRST driving to 4mA
        ret = pmic_config_interface(0x176,0x2,0xF,0); // [3:0]: RG_OCTL_SIMLS2_SCLK; Set PAD_SIMLS2_SCLK driving to 4mA
        ret = pmic_config_interface(0x178,0x1,0x1,11); // [11:11]: RG_INT_EN_THR_H; Thermal INT enable
        ret = pmic_config_interface(0x178,0x1,0x1,10); // [10:10]: RG_INT_EN_THR_L; Thermal INT enable
        ret = pmic_config_interface(0x204,0x1,0x1,2); // [2:2]: VCORE_PG_H2L_EN; PG H2L debounce enable
        ret = pmic_config_interface(0x204,0x1,0x1,1); // [1:1]: VSRAM_PG_H2L_EN; PG H2L debounce enable
        ret = pmic_config_interface(0x204,0x1,0x1,0); // [0:0]: VPROC_PG_H2L_EN; PG H2L debounce enable
        ret = pmic_config_interface(0x20C,0x2,0x3,4); // [5:4]: QI_VPROC_VSLEEP; 
        ret = pmic_config_interface(0x20C,0x3,0x3,0); // [1:0]: RG_VPROC_SLP; 
        ret = pmic_config_interface(0x210,0x1,0x1,4); // [4:4]: VPROC_TRACK_ON_CTRL; 
        ret = pmic_config_interface(0x210,0x1,0x1,1); // [1:1]: VPROC_VOSEL_CTRL; after 0x0256
        ret = pmic_config_interface(0x216,0x1,0x1,15); // [15:15]: VPROC_SFCHG_REN; 
        ret = pmic_config_interface(0x216,0x11,0x7F,8); // [14:8]: VPROC_SFCHG_RRATE; 
        ret = pmic_config_interface(0x216,0x23,0x7F,0); // [6:0]: VPROC_SFCHG_FRATE; 
        ret = pmic_config_interface(0x22A,0x1,0x1,8); // [8:8]: VPROC_VSLEEP_EN; 
        ret = pmic_config_interface(0x22A,0x1,0x3,4); // [5:4]: VPROC_VOSEL_TRANS_EN; 
        ret = pmic_config_interface(0x22A,0x3,0x3,0); // [1:0]: VPROC_TRANSTD; 
        ret = pmic_config_interface(0x232,0x2,0x3,4); // [5:4]: QI_VSRAM_VSLEEP; 
        ret = pmic_config_interface(0x232,0x3,0xF,0); // [3:0]: RG_VSRAM_SLP; 
        ret = pmic_config_interface(0x236,0x1,0x1,5); // [5:5]: VSRAM_TRACK_SLEEP_CTRL; 
        ret = pmic_config_interface(0x236,0x1,0x1,4); // [4:4]: VSRAM_TRACK_ON_CTRL; 
        ret = pmic_config_interface(0x236,0x1,0x1,1); // [1:1]: VSRAM_VOSEL_CTRL; after 0x0256
        ret = pmic_config_interface(0x23C,0x1,0x1,15); // [15:15]: VSRAM_SFCHG_REN; 
        ret = pmic_config_interface(0x23C,0x11,0x7F,8); // [14:8]: VSRAM_SFCHG_RRATE; 
        ret = pmic_config_interface(0x23C,0x23,0x7F,0); // [6:0]: VSRAM_SFCHG_FRATE; 
        ret = pmic_config_interface(0x250,0x1,0x1,8); // [8:8]: VSRAM_VSLEEP_EN; 
        ret = pmic_config_interface(0x250,0x1,0x3,4); // [5:4]: VSRAM_VOSEL_TRANS_EN; 
        ret = pmic_config_interface(0x250,0x3,0x3,0); // [1:0]: VSRAM_TRANSTD; 
        ret = pmic_config_interface(0x260,0x2,0xF,0); // [7:0]: RG_VCORE_RSV; 
        ret = pmic_config_interface(0x262,0x1,0x1,1); // [1:1]: VCORE_VOSEL_CTRL; 
        ret = pmic_config_interface(0x268,0x1,0x1,15); // [15:15]: VCORE_SFCHG_REN; 
        ret = pmic_config_interface(0x268,0x11,0x7F,8); // [14:8]: VCORE_SFCHG_RRATE; 
        ret = pmic_config_interface(0x268,0x23,0x7F,0); // [6:0]: VCORE_SFCHG_FRATE; 
        ret = pmic_config_interface(0x27C,0x1,0x1,8); // [8:8]: VCORE_VSLEEP_EN; 
        ret = pmic_config_interface(0x27C,0x1,0x3,0); // [1:0]: VCORE_TRANSTD; 
        ret = pmic_config_interface(0x2A2,0x1,0x1,8); // [8:8]: VM_VSLEEP_EN; 
        ret = pmic_config_interface(0x2A2,0x1,0x3,0); // [1:0]: VM_TRANSTD; 
        ret = pmic_config_interface(0x328,0x1,0x3,14); // [15:14]: RG_VPA_ZX_OS; 
        ret = pmic_config_interface(0x328,0x1,0x3,12); // [13:12]: RG_VPA_SLEW; 
        ret = pmic_config_interface(0x328,0x1,0x3,10); // [11:10]: RG_VPA_SLEW_NMOS; 
        ret = pmic_config_interface(0x328,0x2,0x3,8); // [9:8]: RG_VPA_CSL; 
        ret = pmic_config_interface(0x328,0x1,0x3,0); // [1:0]: RG_VPA_RZSEL; 
        ret = pmic_config_interface(0x32C,0x3,0x3,0); // [1:0]: RG_VPA_SLP; 
        ret = pmic_config_interface(0x336,0x1,0x1,15); // [15:15]: VPA_SFCHG_REN; 
        ret = pmic_config_interface(0x336,0x1,0x1,7); // [7:7]: VPA_SFCHG_FEN; 
        ret = pmic_config_interface(0x346,0x3,0x3,4); // [5:4]: VPA_BURSTH_ON; 
        ret = pmic_config_interface(0x346,0x3,0x3,0); // [1:0]: VPA_BURSTH; 
        ret = pmic_config_interface(0x34C,0x1,0x1,0); // [0:0]: VPA_DLC_MAP_EN; 
        ret = pmic_config_interface(0x37C,0x1,0x3,8); // [9:8]: RG_VRF18_2_CSL; 
        ret = pmic_config_interface(0x37C,0x2,0x3,6); // [7:6]: RG_VRF18_2_CSR; 
        ret = pmic_config_interface(0x37C,0x1,0x3,4); // [5:4]: RG_VRF18_2_CC; 
        ret = pmic_config_interface(0x37C,0x4,0x7,0); // [2:0]: RG_VRF18_2_RZSEL; 
        ret = pmic_config_interface(0x37E,0x1,0x1,8); // [8:8]: RG_VRF18_2_MODESET; 
        ret = pmic_config_interface(0x380,0x3,0x3,0); // [1:0]: RG_VRF18_2_SLP; 
        //ret = pmic_config_interface(0x38C,0x4,0x1F,0); // [4:0]: VRF18_2_VOSEL; 
        ret = pmic_config_interface(0x45C,0x0,0x7,5); // [7:5]: RG_VSIM1_VOSEL; PD team advice
        ret = pmic_config_interface(0x45E,0x0,0x7,5); // [7:5]: RG_VSIM2_VOSEL; PD team advice
        ret = pmic_config_interface(0x464,0xE,0xF,8); // [11:8]: RG_VEMC_1V8_CAL; 
        ret = pmic_config_interface(0x466,0x1,0x1,2); // [2:2]: VIBR_THER_SHEN_EN; 
        ret = pmic_config_interface(0x468,0x1,0x1,4); // [4:4]: RG_VIBR_STB_SEL; 
        ret = pmic_config_interface(0x500,0x1,0x1,5); // [5:5]: THR_HWPDN_EN; 125?C HW auto power down enable
        ret = pmic_config_interface(0x502,0x1,0x1,3); // [3:3]: RG_RST_DRVSEL; Driver capability select 15mA to improve ESD
        ret = pmic_config_interface(0x502,0x1,0x1,2); // [2:2]: RG_EN_DRVSEL; Driver capability select 15mA to improve ESD
        ret = pmic_config_interface(0x508,0x1,0x1,0); // [0:0]: DDUVLO_DEB_EN; UVLO debounce (100ns) enable
        ret = pmic_config_interface(0x50C,0x1,0x1,5); // [5:5]: UVLO_L2H_DEB_EN; UVLO debounce (100ns) enable
        ret = pmic_config_interface(0x510,0x1,0x1,1); // [1:1]: STRUP_PWROFF_PREOFF_EN; Power off sequence enable
        ret = pmic_config_interface(0x510,0x1,0x1,0); // [0:0]: STRUP_PWROFF_SEQ_EN; Power off sequence enable
        ret = pmic_config_interface(0x540,0x3,0x7,4); // [6:4]: RG_AVG_NUM; AUXADC average 16 sample selection
        ret = pmic_config_interface(0x55C,0xFC,0xFF,8); // [15:8]: RG_ADC_TRIM_CH_SEL; Efuse trim select
        ret = pmic_config_interface(0x55E,0x1,0x1,1); // [1:1]: FLASH_THER_SHDN_EN; 
        ret = pmic_config_interface(0x564,0x1,0x1,1); // [1:1]: KPLED_THER_SHDN_EN; 
        ret = pmic_config_interface(0x600,0x1,0x1,9); // [9:9]: SPK_THER_SHDN_L_EN; 
        ret = pmic_config_interface(0x604,0x1,0x1,0); // [0:0]: RG_SPK_INTG_RST_L; 
        ret = pmic_config_interface(0x606,0x1,0x1,9); // [9:9]: SPK_THER_SHDN_R_EN; 
        ret = pmic_config_interface(0x612,0x1,0xF,8); // [11:8]: RG_SPKPGA_GAIN; 
        ret = pmic_config_interface(0x714,0x1,0x1,7); // [7:7]: RG_LCLDO_ENC_REMOTE_SENSE_VA28; 
        ret = pmic_config_interface(0x714,0x1,0x1,4); // [4:4]: RG_LCLDO_REMOTE_SENSE_VA33; 
        ret = pmic_config_interface(0x714,0x1,0x1,1); // [1:1]: RG_HCLDO_REMOTE_SENSE_VA33; 
        ret = pmic_config_interface(0x71A,0x1,0x1,15); // [15:15]: RG_NCP_REMOTE_SENSE_VA18; 
        
        #if 0
        //dump register
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x254, upmu_get_reg_value(0x254));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x256, upmu_get_reg_value(0x256));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x2, upmu_get_reg_value(0x2));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0xC, upmu_get_reg_value(0xC));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x1A, upmu_get_reg_value(0x1A));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x2A, upmu_get_reg_value(0x2A));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x2E, upmu_get_reg_value(0x2E));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x14C, upmu_get_reg_value(0x14C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x156, upmu_get_reg_value(0x156));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x15A, upmu_get_reg_value(0x15A));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x15C, upmu_get_reg_value(0x15C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x174, upmu_get_reg_value(0x174));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x176, upmu_get_reg_value(0x176));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x178, upmu_get_reg_value(0x178));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x204, upmu_get_reg_value(0x204));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x20C, upmu_get_reg_value(0x20C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x210, upmu_get_reg_value(0x210));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x216, upmu_get_reg_value(0x216));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x22A, upmu_get_reg_value(0x22A));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x232, upmu_get_reg_value(0x232));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x236, upmu_get_reg_value(0x236));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x23C, upmu_get_reg_value(0x23C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x250, upmu_get_reg_value(0x250));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x260, upmu_get_reg_value(0x260));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x262, upmu_get_reg_value(0x262));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x268, upmu_get_reg_value(0x268));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x27C, upmu_get_reg_value(0x27C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x2A2, upmu_get_reg_value(0x2A2));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x328, upmu_get_reg_value(0x328));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x32C, upmu_get_reg_value(0x32C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x336, upmu_get_reg_value(0x336));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x346, upmu_get_reg_value(0x346));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x34C, upmu_get_reg_value(0x34C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x37C, upmu_get_reg_value(0x37C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x37E, upmu_get_reg_value(0x37E));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x380, upmu_get_reg_value(0x380));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x38C, upmu_get_reg_value(0x38C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x45C, upmu_get_reg_value(0x45C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x45E, upmu_get_reg_value(0x45E));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x464, upmu_get_reg_value(0x464));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x466, upmu_get_reg_value(0x466));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x468, upmu_get_reg_value(0x468));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x500, upmu_get_reg_value(0x500));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x502, upmu_get_reg_value(0x502));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x508, upmu_get_reg_value(0x508));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x50C, upmu_get_reg_value(0x50C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x510, upmu_get_reg_value(0x510));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x540, upmu_get_reg_value(0x540));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x55C, upmu_get_reg_value(0x55C));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x55E, upmu_get_reg_value(0x55E));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x564, upmu_get_reg_value(0x564));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x600, upmu_get_reg_value(0x600));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x604, upmu_get_reg_value(0x604));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x606, upmu_get_reg_value(0x606));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x612, upmu_get_reg_value(0x612));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x714, upmu_get_reg_value(0x714));
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", 0x71A, upmu_get_reg_value(0x71A));
        #endif
    }
    else if(chip_version == PMIC6320_E1_CID_CODE)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[Kernel_PMIC_INIT_SETTING_V1] PMIC Chip = %x\n",chip_version);
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[Kernel_PMIC_INIT_SETTING_V1] 20121029 (1)\n");

        //put init setting from DE/SA
        
        ret = pmic_config_interface(0x254,0x58,0x7F,8); // [14:8]: VSRAM_VOSEL_ON_HB; Set before 0x0236
        ret = pmic_config_interface(0x254,0x38,0x7F,0); // [6:0]: VSRAM_VOSEL_ON_LB; Set before 0x0236
        ret = pmic_config_interface(0x256,0x38,0x7F,0); // [6:0]: VSRAM_VOSEL_SLEEP_LB; Set before 0x0236
        ret = pmic_config_interface(0x2,0xB,0xF,4); // [7:4]: RG_VCDT_HV_VTH; 7V OVP
        ret = pmic_config_interface(0xC,0x2,0x7,1); // [3:1]: RG_VBAT_OV_VTH; VBAT_OV=4.3V
        ret = pmic_config_interface(0x1A,0x3,0xF,0); // [3:0]: RG_CHRWDT_TD; align to 6250's
        ret = pmic_config_interface(0x2A,0x0,0x7,4); // [6:4]: RG_CSDAC_STP; align to 6250's setting
        ret = pmic_config_interface(0x2E,0x1,0x1,7); // [7:7]: RG_ULC_DET_EN; 
        ret = pmic_config_interface(0x2E,0x1,0x1,6); // [6:6]: RG_HWCV_EN; 
        ret = pmic_config_interface(0x2E,0x1,0x1,2); // [2:2]: RG_CSDAC_MODE; 
        ret = pmic_config_interface(0x14C,0x0,0xF,12); // [15:12]: RG_SIMLS2_SRST_CONF; Set PAD_SIMLS2_SRST slow skew
        ret = pmic_config_interface(0x14C,0x0,0xF,8); // [11:8]: RG_SIMLS2_SCLK_CONF; Set PAD_SIMLS2_SCLK slow skew
        ret = pmic_config_interface(0x14C,0x0,0xF,4); // [7:4]: RG_SIMLS1_SRST_CONF; Set PAD_SIMLS1_SRST slow skew
        ret = pmic_config_interface(0x14C,0x0,0xF,0); // [3:0]: RG_SIMLS1_SCLK_CONF; Set PAD_SIMLS1_SCLK slow skew
        ret = pmic_config_interface(0x156,0x1,0x1,3); // [3:3]: RG_SMT3; SRCLKEN_MD2 SMT enable
        ret = pmic_config_interface(0x156,0x1,0x1,2); // [2:2]: RG_SMT2; SRCLKEN_PERI SMT enable
        ret = pmic_config_interface(0x156,0x1,0x1,1); // [1:1]: RG_SMT1; SRCVOLTEN SMT enable
        ret = pmic_config_interface(0x15A,0x1,0x1,15); // [15:15]: RG_SMT47; Set PAD_SIMLS2_SCLK schmit input
        ret = pmic_config_interface(0x15A,0x1,0x1,14); // [14:14]: RG_SMT46; Set PAD_SIMLS1_SRST schmit input
        ret = pmic_config_interface(0x15A,0x1,0x1,13); // [13:13]: RG_SMT45; Set PAD_SIMLS1_SCLK schmit input
        ret = pmic_config_interface(0x15C,0x1,0x1,0); // [0:0]: RG_SMT48; Set PAD_SIMLS2_SRST schmit input
        ret = pmic_config_interface(0x174,0x2,0xF,12); // [15:12]: RG_OCTL_SIMLS1_SRST; Set PAD_SIMLS1_SRST driving to 4mA
        ret = pmic_config_interface(0x174,0x2,0xF,8); // [11:8]: RG_OCTL_SIMLS1_SCLK; Set PAD_SIMLS1_SCLK driving to 4mA
        ret = pmic_config_interface(0x176,0x2,0xF,4); // [7:4]: RG_OCTL_SIMLS2_SRST; Set PAD_SIMLS2_SRST driving to 4mA
        ret = pmic_config_interface(0x176,0x2,0xF,0); // [3:0]: RG_OCTL_SIMLS2_SCLK; Set PAD_SIMLS2_SCLK driving to 4mA
        ret = pmic_config_interface(0x178,0x1,0x1,11); // [11:11]: RG_INT_EN_THR_H; Thermal INT enable
        ret = pmic_config_interface(0x178,0x1,0x1,10); // [10:10]: RG_INT_EN_THR_L; Thermal INT enable
        ret = pmic_config_interface(0x204,0x1,0x1,2); // [2:2]: VCORE_PG_H2L_EN; PG H2L debounce enable
        ret = pmic_config_interface(0x204,0x1,0x1,1); // [1:1]: VSRAM_PG_H2L_EN; PG H2L debounce enable
        ret = pmic_config_interface(0x204,0x1,0x1,0); // [0:0]: VPROC_PG_H2L_EN; PG H2L debounce enable
        ret = pmic_config_interface(0x20C,0x2,0x3,4); // [5:4]: QI_VPROC_VSLEEP; 
        ret = pmic_config_interface(0x20C,0x3,0x3,0); // [1:0]: RG_VPROC_SLP; 
        ret = pmic_config_interface(0x210,0x1,0x1,4); // [4:4]: VPROC_TRACK_ON_CTRL; 
        ret = pmic_config_interface(0x210,0x1,0x1,2); // [2:2]: VPROC_DLC_CTRL; E1 only
        ret = pmic_config_interface(0x210,0x1,0x1,1); // [1:1]: VPROC_VOSEL_CTRL; after 0x0256
        ret = pmic_config_interface(0x216,0x1,0x1,15); // [15:15]: VPROC_SFCHG_REN; 
        ret = pmic_config_interface(0x216,0x11,0x7F,8); // [14:8]: VPROC_SFCHG_RRATE; 
        ret = pmic_config_interface(0x216,0x23,0x7F,0); // [6:0]: VPROC_SFCHG_FRATE; 
        ret = pmic_config_interface(0x224,0x0,0x3,8); // [9:8]: VPROC_DLC_N_SLEEP; E1 only
        ret = pmic_config_interface(0x22A,0x1,0x1,8); // [8:8]: VPROC_VSLEEP_EN; 
        ret = pmic_config_interface(0x22A,0x1,0x3,4); // [5:4]: VPROC_VOSEL_TRANS_EN; 
        ret = pmic_config_interface(0x22A,0x3,0x3,0); // [1:0]: VPROC_TRANSTD; 
        ret = pmic_config_interface(0x232,0x2,0x3,4); // [5:4]: QI_VSRAM_VSLEEP; 
        ret = pmic_config_interface(0x232,0x3,0xF,0); // [3:0]: RG_VSRAM_SLP; 
        ret = pmic_config_interface(0x236,0x1,0x1,5); // [5:5]: VSRAM_TRACK_SLEEP_CTRL; 
        ret = pmic_config_interface(0x236,0x1,0x1,4); // [4:4]: VSRAM_TRACK_ON_CTRL; 
        ret = pmic_config_interface(0x236,0x1,0x1,1); // [1:1]: VSRAM_VOSEL_CTRL; after 0x0256
        ret = pmic_config_interface(0x23C,0x1,0x1,15); // [15:15]: VSRAM_SFCHG_REN; 
        ret = pmic_config_interface(0x23C,0x11,0x7F,8); // [14:8]: VSRAM_SFCHG_RRATE; 
        ret = pmic_config_interface(0x23C,0x23,0x7F,0); // [6:0]: VSRAM_SFCHG_FRATE; 
        ret = pmic_config_interface(0x250,0x1,0x1,8); // [8:8]: VSRAM_VSLEEP_EN; 
        ret = pmic_config_interface(0x250,0x1,0x3,4); // [5:4]: VSRAM_VOSEL_TRANS_EN; 
        ret = pmic_config_interface(0x250,0x3,0x3,0); // [1:0]: VSRAM_TRANSTD; 
        
        //ret = pmic_config_interface(0x260,0xF2,0xFF,0); // [7:0]: RG_VCORE_RSV; 
        ret = pmic_config_interface(0x260,0x2,0xF,0); // [3:0]: RG_VCORE_RSV;
        
        ret = pmic_config_interface(0x262,0x1,0x1,2); // [2:2]: VCORE_DLC_CTRL; E1 only
        ret = pmic_config_interface(0x262,0x1,0x1,1); // [1:1]: VCORE_VOSEL_CTRL; 
        ret = pmic_config_interface(0x268,0x1,0x1,15); // [15:15]: VCORE_SFCHG_REN; 
        ret = pmic_config_interface(0x268,0x11,0x7F,8); // [14:8]: VCORE_SFCHG_RRATE; 
        ret = pmic_config_interface(0x268,0x23,0x7F,0); // [6:0]: VCORE_SFCHG_FRATE; 
        ret = pmic_config_interface(0x276,0x0,0x3,8); // [9:8]: VCORE_DLC_N_SLEEP; E1 only
        ret = pmic_config_interface(0x27C,0x1,0x1,8); // [8:8]: VCORE_VSLEEP_EN; 
        ret = pmic_config_interface(0x27C,0x1,0x3,0); // [1:0]: VCORE_TRANSTD; 
        ret = pmic_config_interface(0x288,0x1,0x1,2); // [2:2]: VM_DLC_CTRL; E1 only
        ret = pmic_config_interface(0x29C,0x0,0x3,8); // [9:8]: VM_DLC_N_SLEEP; E1 only
        ret = pmic_config_interface(0x2A2,0x1,0x1,8); // [8:8]: VM_VSLEEP_EN; 
        ret = pmic_config_interface(0x2A2,0x1,0x3,0); // [1:0]: VM_TRANSTD; 
        ret = pmic_config_interface(0x328,0x1,0x3,10); // [11:10]: RG_VPA_SLEW_NMOS; 
        ret = pmic_config_interface(0x328,0x1,0x3,0); // [1:0]: RG_VPA_RZSEL; 
        ret = pmic_config_interface(0x32C,0x3,0x3,0); // [1:0]: RG_VPA_SLP; 
        ret = pmic_config_interface(0x336,0x1,0x1,15); // [15:15]: VPA_SFCHG_REN; 
        ret = pmic_config_interface(0x336,0x1,0x1,7); // [7:7]: VPA_SFCHG_FEN;         
        ret = pmic_config_interface(0x346,0x3,0x3,0); // [1:0]: VPA_BURSTH; 
        ret = pmic_config_interface(0x34C,0x1,0x1,0); // [0:0]: VPA_DLC_MAP_EN;
        ret = pmic_config_interface(0x45C,0x0,0x7,5); // [7:5]: RG_VSIM1_VOSEL; PD team advice
        ret = pmic_config_interface(0x45E,0x0,0x7,5); // [7:5]: RG_VSIM2_VOSEL; PD team advice
        ret = pmic_config_interface(0x500,0x1,0x1,5); // [5:5]: THR_HWPDN_EN; 125?C HW auto power down enable
        ret = pmic_config_interface(0x502,0x1,0x1,3); // [3:3]: RG_RST_DRVSEL; Driver capability select 15mA to improve ESD
        ret = pmic_config_interface(0x502,0x1,0x1,2); // [2:2]: RG_EN_DRVSEL; Driver capability select 15mA to improve ESD
        ret = pmic_config_interface(0x508,0x1,0x1,0); // [0:0]: DDUVLO_DEB_EN; UVLO debounce (100ns) enable
        ret = pmic_config_interface(0x50C,0x1,0x1,5); // [5:5]: UVLO_L2H_DEB_EN; UVLO debounce (100ns) enable
        ret = pmic_config_interface(0x510,0x1,0x1,1); // [1:1]: STRUP_PWROFF_PREOFF_EN; Power off sequence enable
        ret = pmic_config_interface(0x510,0x1,0x1,0); // [0:0]: STRUP_PWROFF_SEQ_EN; Power off sequence enable
        ret = pmic_config_interface(0x540,0x3,0x7,4); // [6:4]: RG_AVG_NUM; AUXADC average 16 sample selection
        ret = pmic_config_interface(0x55C,0xFC,0xFF,8); // [15:8]: RG_ADC_TRIM_CH_SEL; Efuse trim select
        ret = pmic_config_interface(0x55E,0x1,0x1,1); // [1:1]: FLASH_THER_SHDN_EN; 
        ret = pmic_config_interface(0x564,0x1,0x1,1); // [1:1]: KPLED_THER_SHDN_EN; 
        ret = pmic_config_interface(0x600,0x1,0x1,9); // [9:9]: SPK_THER_SHDN_L_EN; 
        ret = pmic_config_interface(0x604,0x1,0x1,0); // [0:0]: RG_SPK_INTG_RST_L; 
        ret = pmic_config_interface(0x606,0x1,0x1,9); // [9:9]: SPK_THER_SHDN_R_EN; 
        ret = pmic_config_interface(0x612,0x1,0xF,8); // [11:8]: RG_SPKPGA_GAIN; 
        ret = pmic_config_interface(0x714,0x1,0x1,7); // [7:7]: RG_LCLDO_ENC_REMOTE_SENSE_VA28; 
        ret = pmic_config_interface(0x714,0x1,0x1,4); // [4:4]: RG_LCLDO_REMOTE_SENSE_VA33; 
        ret = pmic_config_interface(0x714,0x1,0x1,1); // [1:1]: RG_HCLDO_REMOTE_SENSE_VA33; 
        ret = pmic_config_interface(0x71A,0x1,0x1,15); // [15:15]: RG_NCP_REMOTE_SENSE_VA18; 
    }
    else
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[Kernel_PMIC_INIT_SETTING_V1] Unknown PMIC Chip (%x)\n",chip_version);
    }
}

void PMIC_CUSTOM_SETTING_V1(void)
{    
//<2013/1/17-20471-jessicatseng, [Pelican] Integrate backlight IC LM3533 for PRE-MP SW
    upmu_set_rg_vgp4_vosel(5);
    upmu_set_rg_vgp4_en(1); 
//>2013/1/17-20471-jessicatseng
}

void pmic_low_power_setting(void)
{
    U32 ret=0, reg_val=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] CLKCTL - 20121018 by Juinn-Ting\n");
    ret = pmic_config_interface(TOP_CKCON1  , 0x0F20 , 0xAF20 ,0);

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] BUCK/LDO - 20121009 by Juinn-Ting\n");
    ret = pmic_config_interface(VPROC_CON3  , 0x0020 , 0x0030 ,0);
    ret = pmic_config_interface(VSRAM_CON3  , 0x0020 , 0x0030 ,0);
    ret = pmic_config_interface(VCORE_CON3  , 0x0020 , 0x0030 ,0);

    reg_val = upmu_get_reg_value(VM_CON12);
    if(reg_val == 0x50)
    {
        ret = pmic_config_interface(VM_CON3     , 0x0000 , 0x0030 ,0);
    }
    else if(reg_val == 0x58)
    {
        ret = pmic_config_interface(VM_CON3     , 0x0010 , 0x0030 ,0);
    }
    else if(reg_val == 0x60)
    {
        ret = pmic_config_interface(VM_CON3     , 0x0020 , 0x0030 ,0);
    }
    else if(reg_val == 0x7F)
    {
        ret = pmic_config_interface(VM_CON3     , 0x0030 , 0x0030 ,0);
    }
    else
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[VM_CON3] no change due to VM_CON12=0x%x\n", reg_val);
    }        
    
    ret = pmic_config_interface(VPROC_CON18 , 0x0100 , 0x0100 ,0);
    ret = pmic_config_interface(VSRAM_CON18 , 0x0100 , 0x0100 ,0);
    ret = pmic_config_interface(VCORE_CON18 , 0x0100 , 0x0100 ,0);
    ret = pmic_config_interface(VM_CON18    , 0x0100 , 0x0100 ,0);
    ret = pmic_config_interface(VIO18_CON18 , 0x0100 , 0x0100 ,0);
    ret = pmic_config_interface(VPROC_CON8  , 0x8888 , 0xFFFF ,0);
    ret = pmic_config_interface(VSRAM_CON8  , 0x8888 , 0xFFFF ,0);
    ret = pmic_config_interface(VCORE_CON8  , 0x8888 , 0xFFFF ,0);
    ret = pmic_config_interface(VSRAM_CON19 , 0x0202 , 0x7F7F ,0);
    ret = pmic_config_interface(VSRAM_CON20 , 0x5838 , 0x7F7F ,0);
    ret = pmic_config_interface(VSRAM_CON21 , 0x0018 , 0x007F ,0);
    ret = pmic_config_interface(VPROC_CON11 , 0x0018 , 0x007F ,0);
    ret = pmic_config_interface(VCORE_CON11 , 0x0018 , 0x007F ,0);
    ret = pmic_config_interface(VPROC_CON6  , 0x0000 , 0x0070 ,0);
    ret = pmic_config_interface(VSRAM_CON6  , 0x0000 , 0x0070 ,0);
    ret = pmic_config_interface(VCORE_CON6  , 0x0000 , 0x0070 ,0);
    ret = pmic_config_interface(VPROC_CON5  , 0x0012 , 0x0013 ,0);
    ret = pmic_config_interface(VSRAM_CON5  , 0x0032 , 0x0033 ,0);
    ret = pmic_config_interface(VCORE_CON5  , 0x0002 , 0x0003 ,0);
    ret = pmic_config_interface(VM_CON5     , 0x0000 , 0x0003 ,0);
    ret = pmic_config_interface(VIO18_CON5  , 0x0000 , 0x0003 ,0);
    ret = pmic_config_interface(VRF18_2_CON5, 0x0000 , 0x0003 ,0);
    ret = pmic_config_interface(ANALDO_CON1 , 0x3800 , 0x7800 ,0);
    ret = pmic_config_interface(ANALDO_CON2 , 0x4001 , 0x4031 ,0);
    ret = pmic_config_interface(ANALDO_CON3 , 0x4001 , 0x4031 ,0);
    ret = pmic_config_interface(DIGLDO_CON0 , 0x4001 , 0x4031 ,0);

    //dump register
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", TOP_CKCON1, upmu_get_reg_value(TOP_CKCON1));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VPROC_CON3, upmu_get_reg_value(VPROC_CON3));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VSRAM_CON3, upmu_get_reg_value(VSRAM_CON3));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VCORE_CON3, upmu_get_reg_value(VCORE_CON3));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VM_CON3, upmu_get_reg_value(VM_CON3));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VPROC_CON18, upmu_get_reg_value(VPROC_CON18));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VSRAM_CON18, upmu_get_reg_value(VSRAM_CON18));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VCORE_CON18, upmu_get_reg_value(VCORE_CON18));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VM_CON18, upmu_get_reg_value(VM_CON18));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VIO18_CON18, upmu_get_reg_value(VIO18_CON18));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VPROC_CON8, upmu_get_reg_value(VPROC_CON8));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VSRAM_CON8, upmu_get_reg_value(VSRAM_CON8));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VCORE_CON8, upmu_get_reg_value(VCORE_CON8));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VSRAM_CON19, upmu_get_reg_value(VSRAM_CON19));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VSRAM_CON20, upmu_get_reg_value(VSRAM_CON20));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VSRAM_CON21, upmu_get_reg_value(VSRAM_CON21));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VPROC_CON11, upmu_get_reg_value(VPROC_CON11));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VCORE_CON11, upmu_get_reg_value(VCORE_CON11));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VPROC_CON6, upmu_get_reg_value(VPROC_CON6));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VSRAM_CON6, upmu_get_reg_value(VSRAM_CON6));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VCORE_CON6, upmu_get_reg_value(VCORE_CON6));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VPROC_CON5, upmu_get_reg_value(VPROC_CON5));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VSRAM_CON5, upmu_get_reg_value(VSRAM_CON5));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VCORE_CON5, upmu_get_reg_value(VCORE_CON5));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VM_CON5, upmu_get_reg_value(VM_CON5));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VIO18_CON5, upmu_get_reg_value(VIO18_CON5));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", VRF18_2_CON5, upmu_get_reg_value(VRF18_2_CON5));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", ANALDO_CON1, upmu_get_reg_value(ANALDO_CON1));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", ANALDO_CON2, upmu_get_reg_value(ANALDO_CON2));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", ANALDO_CON3, upmu_get_reg_value(ANALDO_CON3));
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Reg[0x%x]=0x%x\n", DIGLDO_CON0, upmu_get_reg_value(DIGLDO_CON0));
 
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_low_power_setting] Done\n");
}

void pmic_setting_depends_rtc(void)
{
    U32 ret=0;
    
    if( crystal_exist_status() )
    {
        // with 32K
        ret = pmic_config_interface(ANALDO_CON1,    3,    0x7,    12); // [14:12]=3(VTCXO_SRCLK_EN_SEL),
        ret = pmic_config_interface(ANALDO_CON1,    1,    0x1,    11); // [11]=1(VTCXO_ON_CTRL), 
        ret = pmic_config_interface(ANALDO_CON1,    0,    0x1,    1);  // [1]=0(VTCXO_LP_SET), 
        ret = pmic_config_interface(ANALDO_CON1,    0,    0x1,    0);  // [0]=0(VTCXO_LP_SEL),
        
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_setting_depends_rtc] With 32K. Reg[0x%x]=0x%x\n", 
            ANALDO_CON1, upmu_get_reg_value(ANALDO_CON1));
    }
    else
    {
        // without 32K
        ret = pmic_config_interface(ANALDO_CON1,    0,    0x1,    11); // [11]=0(VTCXO_ON_CTRL), 
        ret = pmic_config_interface(ANALDO_CON1,    1,    0x1,    10); // [10]=1(RG_VTCXO_EN), 
        ret = pmic_config_interface(ANALDO_CON1,    3,    0x7,    4);  // [6:4]=3(VTCXO_SRCLK_MODE_SEL), 
        ret = pmic_config_interface(ANALDO_CON1,    1,    0x1,    0);  // [0]=1(VTCXO_LP_SEL),

        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_setting_depends_rtc] Without 32K. Reg[0x%x]=0x%x\n", 
            ANALDO_CON1, upmu_get_reg_value(ANALDO_CON1));
    }
}

int g_gpu_status_bit=1;

int pmic_get_gpu_status_bit_info(void)
{
    return g_gpu_status_bit;
}
EXPORT_SYMBOL(pmic_get_gpu_status_bit_info);

int get_spm_gpu_status(void)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[get_spm_gpu_status] wait spm driver service ready\n");

    return 0;
}

void pmic_vrf18_2_usage_protection(void)
{
    U32 val=0;

    //1:VCORE, 0:VRF18_2
    //g_gpu_status_bit = get_spm_gpu_status();
    //g_gpu_status_bit = test_spm_gpu_power_on();
    g_gpu_status_bit = get_gpu_power_src();

    if(g_gpu_status_bit == 1)
    {
        val=0x1F;
    }
    else
    {
        val=0x4;
    }

    upmu_set_vrf18_2_vosel(val);
    upmu_set_vrf18_2_vosel_on(val);
    upmu_set_vrf18_2_vosel_sleep(val);

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Reg[0x%x]=%x, Reg[0x%x]=%x, Reg[0x%x]=%x, gpu_status_bit=%d\n",
        VRF18_2_CON9,  upmu_get_reg_value(VRF18_2_CON9),
        VRF18_2_CON10, upmu_get_reg_value(VRF18_2_CON10),
        VRF18_2_CON11, upmu_get_reg_value(VRF18_2_CON11), g_gpu_status_bit);
}

void pmic_gpu_power_enable(int power_en)
{
    //if(g_gpu_status_bit == 1)
    if(get_gpu_power_src() == 1)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_gpu_power_enable] gpu is not powered by VRF18_2\n");
    }
    else
    {
        if(power_en == 1)
        {
            upmu_set_vrf18_2_en(1);            
        }
        else
        {
            upmu_set_vrf18_2_en(0);
        }
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_gpu_power_enable] Reg[0x%x]=%x\n", 
            VRF18_2_CON7, upmu_get_reg_value(VRF18_2_CON7));
    }
}
EXPORT_SYMBOL(pmic_gpu_power_enable);

int g_pmic_cid=0;
EXPORT_SYMBOL(g_pmic_cid);

static int pmic_mt6320_probe(struct platform_device *dev)
{
    U32 ret_val=0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "******** MT6320 pmic driver probe!! ********\n" );

    //init battery wakelock for battery/charger event
    wake_lock_init(&battery_suspend_lock, WAKE_LOCK_SUSPEND, "battery wakelock");    
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "init battery wakelock for battery/charger event.\n" );

    //get PMIC CID
    ret_val=upmu_get_cid();
    g_pmic_cid=ret_val;
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "MT6320 PMIC CID=0x%x\n", ret_val );

    //VRF18_2 usage protection
    pmic_vrf18_2_usage_protection();

    //enable rtc 32k to pmic
    rtc_gpio_enable_32k(RTC_GPIO_USER_PMIC);

    //pmic initial setting
    PMIC_INIT_SETTING_V1();
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[PMIC_INIT_SETTING_V1] Done\n");
    PMIC_CUSTOM_SETTING_V1();
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[PMIC_CUSTOM_SETTING_V1] Done\n");

    //pmic low power setting
    pmic_low_power_setting();

    //pmic setting with RTC
    pmic_setting_depends_rtc();
    
    upmu_set_rg_pwrkey_int_sel(1);
    upmu_set_rg_homekey_int_sel(1);
    upmu_set_rg_homekey_puen(1);
    
    //PMIC Interrupt Service
    PMIC_EINT_SETTING();
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[PMIC_EINT_SETTING] Done\n");
    
    kthread_run(pmic_thread_kthread, NULL, "pmic_thread_kthread");
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_thread_kthread] Done\n");

    //Dump register
    //#ifndef USER_BUILD_KERNEL
    //PMIC_DUMP_ALL_Register();
    //#endif

    #if defined(CONFIG_POWER_EXT)
    ret_val = pmic_config_interface(0x002E,0x0010,0x00FF,0);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[pmic_thread_kthread] add for EVB\n");
    #endif

    #if defined(MTK_ENABLE_MD2) && defined(MODEM2_3G)
    //keep VAST setting
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[PMIC] keep VAST due to MTK_ENABLE_MD2 && MODEM2_3G\n");
    #else
        #if defined(MTK_MT8193_SUPPORT)
        //keep VAST setting
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[PMIC] keep VAST due to MTK_MT8193_SUPPORT\n");
        #else
        ret_val=pmic_config_interface(DIGLDO_CON20, 0x0, PMIC_RG_VAST_EN_MASK, PMIC_RG_VAST_EN_SHIFT);
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "[CONFIG_FORCE_OFF_VAST] Reg[0x%x]=%x\n", 
                DIGLDO_CON20, upmu_get_reg_value(DIGLDO_CON20));
        #endif
    #endif

    return 0;
}

static int pmic_mt6320_remove(struct platform_device *dev)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "******** MT6320 pmic driver remove!! ********\n" );

    return 0;
}

static void pmic_mt6320_shutdown(struct platform_device *dev)
{
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "******** MT6320 pmic driver shutdown!! ********\n" );
}

static int pmic_mt6320_suspend(struct platform_device *dev, pm_message_t state)
{
    U32 ret=0;

    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "******** MT6320 pmic driver suspend!! ********\n" );

    //Set PMIC register 0x022A bit[5:4] =00 before system into sleep mode.
    ret = pmic_config_interface(0x22A,0x0,0x3,4);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Suspend: Reg[0x%x]=0x%x\n",0x22A, upmu_get_reg_value(0x22A));

    ret = pmic_config_interface(0x0102,0x1,0x1,3);
    ret = pmic_config_interface(0x0102,0x1,0x1,4);
    ret = pmic_config_interface(0x0102,0x1,0x1,15);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Suspend: Reg[0x%x]=0x%x\n",0x0102, upmu_get_reg_value(0x0102));

    return 0;
}

static int pmic_mt6320_resume(struct platform_device *dev)
{
    U32 ret=0;

    //xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "******** MT6320 pmic driver resume!! ********\n" );

    //Set PMIC register 0x022A bit[5:4] =01 after system resume.
    ret = pmic_config_interface(0x22A,0x1,0x3,4);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Resume: Reg[0x%x]=0x%x\n",0x22A, upmu_get_reg_value(0x22A));

    ret = pmic_config_interface(0x0102,0x0,0x1,3);
    ret = pmic_config_interface(0x0102,0x0,0x1,4);
    ret = pmic_config_interface(0x0102,0x0,0x1,15);
    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "Resume: Reg[0x%x]=0x%x\n",0x0102, upmu_get_reg_value(0x0102));

    return 0;
}

struct platform_device pmic_mt6320_device = {
    .name   = "pmic_mt6320",
    .id        = -1,
};

static struct platform_driver pmic_mt6320_driver = {
    .probe        = pmic_mt6320_probe,
    .remove        = pmic_mt6320_remove,
    .shutdown    = pmic_mt6320_shutdown,
    //#ifdef CONFIG_PM
    .suspend    = pmic_mt6320_suspend,
    .resume        = pmic_mt6320_resume,
    //#endif
    .driver     = {
        .name = "pmic_mt6320",
    },
};

//==============================================================================
// PMIC6320 device driver
//==============================================================================
static int mt_pmic_probe(struct platform_device *dev)
{
    int ret_device_file = 0;

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "******** mt_pmic_probe!! ********\n" );

    ret_device_file = device_create_file(&(dev->dev), &dev_attr_pmic_access);

    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VPROC_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VSRAM_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VCORE_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VM_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VIO18_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VPA_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VRF18_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VRF18_2_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VIO28_STATUS);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VUSB_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VMC1_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VMCH1_STATUS);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VEMC_3V3_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VEMC_1V8_STATUS);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP1_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP2_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP3_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP4_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP5_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP6_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VSIM1_STATUS);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VSIM2_STATUS);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VIBR_STATUS);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VRTC_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VAST_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VRF28_STATUS);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VRF28_2_STATUS); 
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VTCXO_STATUS); 
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VTCXO_2_STATUS); 
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VA_STATUS);      
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VA28_STATUS);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VCAMA_STATUS);
    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VPROC_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VSRAM_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VCORE_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VM_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VIO18_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VPA_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VRF18_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_BUCK_VRF18_2_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VIO28_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VUSB_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VMC1_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VMCH1_VOLTAGE);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VEMC_3V3_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VEMC_1V8_VOLTAGE);
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP1_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP2_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP3_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP4_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP5_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VGP6_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VSIM1_VOLTAGE);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VSIM2_VOLTAGE);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VIBR_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VRTC_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VAST_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VRF28_VOLTAGE);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VRF28_2_VOLTAGE); 
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VTCXO_VOLTAGE);   
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VTCXO_2_VOLTAGE); 
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VA_VOLTAGE);      
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VA28_VOLTAGE);    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_LDO_VCAMA_VOLTAGE);  


    return 0;
}

struct platform_device mt_pmic_device = {
    .name   = "mt-pmic",
    .id        = -1,
};

static struct platform_driver mt_pmic_driver = {
    .probe        = mt_pmic_probe,
    .driver     = {
        .name = "mt-pmic",
    },
};

//==============================================================================
// PMIC6320 mudule init/exit
//==============================================================================
static int __init pmic_mt6320_init(void)
{
    int ret;

    // PMIC device driver register
    ret = platform_device_register(&pmic_mt6320_device);
    if (ret) {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[pmic_mt6320_init] Unable to device register(%d)\n", ret);
        return ret;
    }
    ret = platform_driver_register(&pmic_mt6320_driver);
    if (ret) {
        xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[pmic_mt6320_init] Unable to register driver (%d)\n", ret);
        return ret;
    }

    // PMIC user space access interface
    ret = platform_device_register(&mt_pmic_device);
    if (ret) {
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[pmic_mt6320_init] Unable to device register(%d)\n", ret);
            return ret;
    }
    ret = platform_driver_register(&mt_pmic_driver);
    if (ret) {
            xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[pmic_mt6320_init] Unable to register driver (%d)\n", ret);
            return ret;
    }

    xlog_printk(ANDROID_LOG_INFO, "Power/PMIC", "****[pmic_mt6320_init] Initialization : DONE !!\n");

    return 0;
}

static void __exit pmic_mt6320_exit (void)
{
}

fs_initcall(pmic_mt6320_init);

//module_init(pmic_mt6320_init);
module_exit(pmic_mt6320_exit);

MODULE_AUTHOR("James Lo");
MODULE_DESCRIPTION("MT6320 PMIC Device Driver");
MODULE_LICENSE("GPL");

