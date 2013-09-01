#ifndef __MT6616_FM_H__
#define __MT6616_FM_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h> // udelay()
#include <linux/device.h> // device_create()
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/version.h>      /* constant of kernel version */
#include <asm/uaccess.h> // get_user()
#include <linux/fm.h>
#include <cust_eint.h>   //for EINT customization
#include <linux/proc_fs.h>
#include <linux/string.h>


#ifdef  MT6516
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_typedefs.h>
#define MT6616_I2C_PORT      2
#define MT65XX_EINT_IRQ_UNMASK MT6516_EINTIRQUnmask
#define MT65XX_EINT_IRQ_MASK MT6516_EINTIRQMask
#define MT65XX_EINT_SET_HW_DEBOUNCE MT6516_EINT_Set_HW_Debounce
#define MT65XX_EINT_SET_SENSITIVITY MT6516_EINT_Set_Sensitivity
#define MT65XX_EINT_REGISTRATION MT6516_EINT_Registration
#else
#include <mach/mt6573_gpio.h>
#include <mach/mt6573_typedefs.h>
#include <cust_gpio_usage.h>
#define MT6616_I2C_PORT      1
#define MT65XX_EINT_IRQ_UNMASK mt65xx_eint_unmask
#define MT65XX_EINT_IRQ_MASK mt65xx_eint_mask
#define MT65XX_EINT_SET_HW_DEBOUNCE mt65xx_eint_set_hw_debounce
#define MT65XX_EINT_SET_SENSITIVITY mt65xx_eint_set_sens
#define MT65XX_EINT_REGISTRATION mt65xx_eint_registration
#endif

/******************DBG level added by DDB************************/
#define D_IOCTL 0x00000010
#define D_BLKC	0x00000020
#define D_G0	0x00000080
#define D_G1	0x00000100
#define D_G2	0x00000200
#define D_G3	0x00000400
#define D_G14	0x00000800
#define D_RAW	0x00001000
#define D_RDS	0x00002000
#define D_G4	0x00004000
#define D_ALL	0xfffffff0

#define L0 0x00000000
#define L1 0x00000001
#define L2 0x00000002
#define L3 0x00000003
#define L4 0x00000004
#define L5 0x00000005
#define L6 0x00000006
#define L7 0x00000007

#define WCN_DBG(flag, fmt, args...) \
	do { \
		if ((((flag)&0x0000000f)<=(g_dbg_level&0x0000000f)) && ((flag)&0xfffffff0)&g_dbg_level) { \
			printk(KERN_ALERT "FM--" #flag "--" fmt, ## args); \
		} \
	} while(0)
/*******************************END*********************************/

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

//customer need customize the I2C port

#define ext_clk               //if define ext_clk use external reference clock  
                              //or mask will use internal
#define FM_RDS_ENABLE        0x01 // 1: enable RDS, 0:disable RDS

#define MT6616_DEV           "MT6616"

/******************************************************************************
 * STRUCTURE DEFINITIONS
 *****************************************************************************/
struct fm {
    uint ref;
    bool chipon;  //MT6616 chip power state
    bool powerup;      //FM module power state
    uint16_t chip_id;
    uint16_t device_id;
    dev_t dev_t;
    uint16_t min_freq; // KHz
    uint16_t max_freq; // KHz
    uint8_t band;      // TODO
    struct class *cls;
    struct device *dev;
    struct cdev cdev;
    struct i2c_client *i2c_client;
    wait_queue_head_t read_wait;
    struct workqueue_struct *fm_workqueue;    /* fm eint handling */
    struct work_struct fm_work;
#if FM_RDS_ENABLE
    bool RDS_Data_ready;
    RDSData_Struct *pstRDSData; 
    struct workqueue_struct *fm_rds_reset_workqueue;    /* fm rds reset handling */
    struct work_struct fm_rds_reset_work;
#endif
};

enum MT6616_REG {
    FM_MAIN_PGSEL = 0x2F,
    FM_MAIN_CG = 0x30,
    FM_MAIN_CG2 = 0x31,
    FM_MAIN_HWVER = 0x32,
    FM_MAIN_CTRL = 0x33,
    FM_MAIN_EN1 = 0x34,
    FM_MAIN_EN2 = 0x35,
    FM_MAIN_CFG1 = 0x36,
    FM_MAIN_CFG2 = 0x37,
    FM_MAIN_MCLKDESENSE = 0x38,
    FM_MAIN_INTR = 0x39,
    FM_MAIN_INTRMASK = 0x3A,
    FM_MAIN_EXTINTRMASK = 0x3B,
    FM_MAIN_DEBUG0 = 0x3C,
    FM_MAIN_GUARD = 0x3D,
    FM_MAIN_RESET = 0x3E,
    FM_MAIN_CHANDETSTAT = 0x3F,    
    FM_DCCOMP1 = 0x40,
    FM_DCCOMP2 = 0x41,
    FM_IQCOMP1 = 0x42,
    FM_IQCOMP2 = 0x43,
    FM_IQCOMP3 = 0x44,
    FM_IQCOMP4 = 0x45,
    FM_RXCALSTAT1 = 0x46,
    FM_RXCALSTAT2 = 0x47,
    FM_RXCALSTAT3 = 0x48,
    FM_RXCALSTAT4 = 0x49,
    FM_MAIN_MCLKDESENSE2 = 0x4A,
    FM_MAIN_MCLKDESENSE3 = 0x4B,    
    FM_CHNLSCAN_CTRL = 0x4D,
    FM_CHNLSCAN_STAT = 0x4E,
    FM_CHNLSCAN_STAT2,
    FM_BITMAP0 = 0x50,
    FM_BITMAP1 = 0x51,
    FM_BITMAP2 = 0x52,
    FM_BITMAP3 = 0x53,
    FM_BITMAP4 = 0x54,
    FM_BITMAP5 = 0x55,
    FM_BITMAP6 = 0x56,
    FM_BITMAP7 = 0x57,
    FM_BITMAP8 = 0x58,
    FM_BITMAP9 = 0x59,
    FM_BITMAPA = 0x5A,
    FM_BITMAPB = 0x5B,
    FM_BITMAPC = 0x5C,
    FM_BITMAPD = 0x5D,
    FM_BITMAPE = 0x5E,
    FM_BITMAPF = 0x5F,
    FM_ADDR_RSSI = 0x74,
    FM_ADDR_PAMD = 0x75,    
    FM_DAC_CON1 = 0x83,
    FM_DAC_CON2 = 0x84,
    FM_FT_CON0 = 0x86,
    FM_FT_CON9 = 0x8F,
    FM_I2S_CON0 = 0x90,  
#if FM_RDS_ENABLE
    FM_RDS_RESET = 0x91,
    FM_RDS_BER_THD_SET_REG = 0xA7,
    FM_RDS_BER_BAD_REG = 0xA8,
    FM_RDS_GOODBK_CNT = 0xAA,
    FM_RDS_BADBK_CNT = 0xAB,
    FM_RDS_DATA_A_E_REG = 0xAE,
    FM_RDS_DATA_B_E_REG = 0xAF,
    FM_RDS_DATA_C_E_REG	= 0xB0,
    FM_RDS_DATA_D_E_REG	= 0xB1,
    FM_RDS_DATA_CRC_FFOST = 0xB2,
    FM_RDS_FFOST_TRIG_TH  =	0xB3,   
#endif
};

//FM_MAIN_CFG1(0x36) && FM_MAIN_CFG2(0x37)
#define MT6616_FM_SEEK_UP       0x0
#define MT6616_FM_SEEK_DOWN     0x01
#define MT6616_FM_SCAN_UP       0x0
#define MT6616_FM_SCAN_DOWN     0x01
#define MT6616_FM_SPACE_INVALID 0x0
#define MT6616_FM_SPACE_50K     0x01
#define MT6616_FM_SPACE_100K    0x02
#define MT6616_FM_SPACE_200K    0x04 

//FM_MAIN_INTR(0x39)
#define FM_INTR_STC_DONE	 0x0001
#define FM_INTR_IQCAL_DONE	 0x0002				
#define FM_INTR_DESENSE_HIT	 0x0004				
#define FM_INTR_CHNL_CHG	 0x0008				
#define FM_INTR_SW_INTR  	 0x0010	
#define FM_INTR_RDS		     0x0020

//FM_MAIN_EXTINTRMASK(0x3B)
#define FM_EXT_STC_DONE_MASK 0x01
#define FM_EXT_RDS_MASK      0x20

//FM_MAIN_CHANDETSTAT(0x3F)
#define FM_MAIN_CHANDET_MASK   0xFFF0  // D4~D15 in address 3FH
#define FM_MAIN_CHANDET_SHIFT  0x04

//FM_ADDR_PAMD(0xF7)
#define FM_RSSI_MASK 		   0xFE00
#define FM_RSSI_SHIFT  		   9

extern void Delayms(uint32_t data);
extern void Delayus(uint32_t data);
extern int  MT6616_read(struct i2c_client *client, uint8_t addr, uint16_t *val);
extern int  MT6616_write(struct i2c_client *client, uint8_t addr, uint16_t val);
extern int  MT6616_set_bits(struct i2c_client *client, uint8_t addr,
                               uint16_t bits, uint16_t mask);
             
#if FM_RDS_ENABLE
extern void MT6616_RDS_BlerCheck(struct fm *fm);
extern bool MT6616_RDS_OnOff(struct fm *fm, bool bFlag);
extern void MT6616_RDS_Eint_Handler(struct fm *fm);
#endif

#endif
