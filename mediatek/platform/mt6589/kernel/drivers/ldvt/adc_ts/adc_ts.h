

/*****************************************************************************
 *
 * Filename:
 * ---------
 *    auxadc.h
 *
 * Project:
 * --------
 *   MT6573 DVT
 *
 * Description:
 * ------------
 *   This file is for Auxiliary ADC Unit.
 *
 * Author:
 * -------
 *  Myron Li
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
 
#ifndef _DVT_ADC_TS_H
#define _DVT_ADC_TS_H

#include <mach/mt6575_devs.h>
#include <mach/mt6575_irq.h>
#include <mach/mt6575_reg_base.h>
#include <mach/mt6575_typedefs.h>

#define AUXADC_CON0              (AUXADC_BASE + 0x0000)
#define AUXADC_CON1              (AUXADC_BASE + 0x0004)
#define AUXADC_CON1_SET          (AUXADC_BASE + 0x0008)
#define AUXADC_CON1_CLR          (AUXADC_BASE + 0x000c)
#define AUXADC_CON2              (AUXADC_BASE + 0x0010)
#define AUXADC_CON3              (AUXADC_BASE + 0x0014)
#define AUXADC_DAT0              (AUXADC_BASE + 0x0018) 
#define AUXADC_TS_DEBT0          (AUXADC_BASE + 0x0054)
#define AUXADC_TS_DEBT1          (AUXADC_BASE + 0x0058)
#define AUXADC_TS_CMD            (AUXADC_BASE + 0x005c)
#define AUXADC_TS_ADDR           (AUXADC_BASE + 0x0060)
#define AUXADC_TS_CON0           (AUXADC_BASE + 0x0064)
#define AUXADC_TS_CON1           (AUXADC_BASE + 0x0068)
#define AUXADC_TS_CON2           (AUXADC_BASE + 0x006c)
#define AUXADC_TS_CON3           (AUXADC_BASE + 0x0070)
#define AUXADC_TS_DATA0          (AUXADC_BASE + 0x0074)
#define AUXADC_TS_DATA1          (AUXADC_BASE + 0x0078)
#define AUXADC_TS_DATA2          (AUXADC_BASE + 0x007c)
#define AUXADC_TS_DATA3          (AUXADC_BASE + 0x0080)
#define AUXADC_DET_VOLT          (AUXADC_BASE + 0x0084)
#define AUXADC_DET_SEL           (AUXADC_BASE + 0x0088)
#define AUXADC_DET_PERIOD        (AUXADC_BASE + 0x008c)
#define AUXADC_DET_DEBT          (AUXADC_BASE + 0x0090)
#define AUXADC_MISC              (AUXADC_BASE + 0x0094)
#define AUXADC_ECC               (AUXADC_BASE + 0x0098)
#define AUXADC_SAMPLE_LIST       (AUXADC_BASE + 0x009c)
#define AUXADC_ABIST_PERIOD      (AUXADC_BASE + 0x00a0)
#define AUXADC_TS_RAW_CON        (AUXADC_BASE + 0x0100)
#define AUXADC_TS_AUTO_TIME_INTVL (AUXADC_BASE + 0x0104)
#define AUXADC_TS_RAW_X_DAT0      (AUXADC_BASE + 0x0200)
#define AUXADC_TS_RAW_Y_DAT0      (AUXADC_BASE + 0x0220)
#define AUXADC_TS_RAW_Z1_DAT0     (AUXADC_BASE + 0x0240)
#define AUXADC_TS_RAW_Z2_DAT0     (AUXADC_BASE + 0x0260)

#define AUXADC_CON3_STA_MASK  0x0001
#define AUXADC_TS_DEBT_MASK          0x3fff
#define AUXADC_TS_CMD_PD_MASK        0x0003
#define AUXADC_TS_CMD_PD_YDRV_SH     0x0000
#define AUXADC_TS_CMD_PD_IRQ_SH      0x0001
#define AUXADC_TS_CMD_PD_IRQ         0x0003
#define AUXADC_TS_CMD_SE_DF_MASK     0x0004
#define AUXADC_TS_CMD_DIFFERENTIAL   0x0000
#define AUXADC_TS_CMD_SINGLE_END     0x0004
#define AUXADC_TS_CMD_MODE_MASK      0x0008
#define AUXADC_TS_CMD_MODE_12BIT     0x0000
#define AUXADC_TS_CMD_MODE_10BIT     0x0008
#define AUXADC_TS_CMD_ADDR_MASK      0x0007
#define AUXADC_TS_CMD_ADDR_Y         0x0001
#define AUXADC_TS_CMD_ADDR_Z1        0x0003
#define AUXADC_TS_CMD_ADDR_Z2        0x0004
#define AUXADC_TS_CMD_ADDR_X         0x0005
#define AUXADC_TS_CON_SPL_MASK       0x0001
#define AUXADC_TS_CON_SPL_TRIGGER    0x0001 // data0 reg
#define AUXADC_TS_CON_STATUS_MASK    0x0002
#define AUXADC_TS_DAT0_DAT_MASK      0x03ff
#define AUXADC_TS_DEBOUNCE_TIME      (4*32) /* 20ms */
#define AUXADC_TS_POWER_UP    0x0c000c00/* DIFFERENTIAL | MODE_12BIT | PD_YDRV_SH */
#define AUXADC_TS_SAMPLE_SETTING     0x0000 
#define AUXADC_TS_POSITION_X  1
#define AUXADC_TS_POSITION_Y  2
#define AUXADC_TS_POSITION_Z1 3
#define AUXADC_TS_POSITION_Z2 4
/* AUXADC_TS_CON1 reg */
#define FAV_ADEL_BIT 8
#define FAV_EN_BIT   7
#define FAV_COORDSEL 5
#define FAV_INVALID  4
#define FAV_ASAMP    3
#define FAV_SEL      2
#define FAV_LCNT     0

#define MT65XX_TOUCH_IRQ_LINE MT6575_TS_IRQ_ID
#define MT65XX_TOUCH_BATCH_LINE MT6575_TS_BATCH_IRQ_ID


#define BASE_VALUE   (0)
#define SET_AUXADC_TS_DEBT0          (BASE_VALUE + 1)
#define SET_AUXADC_TS_CMD           (BASE_VALUE + 2)
#define SET_AUXADC_TS_CON           (BASE_VALUE + 3)   
#define SET_AUXADC_TS_DAT0          (BASE_VALUE + 4) 
#define SET_AUXADC_TS_AUTO_CON      (BASE_VALUE + 5)                    
#define SET_AUXADC_TS_AUTO_COUNT    (BASE_VALUE + 6)                    
#define SET_AUXADC_TS_AUTO_X_DAT0   (BASE_VALUE + 7)                    
#define SET_AUXADC_TS_AUTO_Y_DAT0   (BASE_VALUE + 8)                    
#define SET_AUXADC_TS_AUTO_Z1_DAT0  (BASE_VALUE + 9)                     
#define SET_AUXADC_TS_AUTO_Z2_DAT0  (BASE_VALUE + 10)                              

#define GET_AUXADC_TS_DEBT0          (BASE_VALUE + 11)
#define GET_AUXADC_TS_CMD           (BASE_VALUE + 12)
#define GET_AUXADC_TS_CON           (BASE_VALUE + 13)   
#define GET_AUXADC_TS_DAT0          (BASE_VALUE + 14) 
#define GET_AUXADC_TS_AUTO_CON      (BASE_VALUE + 15)                    
#define GET_AUXADC_TS_AUTO_COUNT    (BASE_VALUE + 16)                    
#define GET_AUXADC_TS_AUTO_X_DAT0   (BASE_VALUE + 17)                    
#define GET_AUXADC_TS_AUTO_Y_DAT0   (BASE_VALUE + 18)                    
#define GET_AUXADC_TS_AUTO_Z1_DAT0  (BASE_VALUE + 19)                     
#define GET_AUXADC_TS_AUTO_Z2_DAT0  (BASE_VALUE + 20)        
#define SET_TS_WAKE_SRC	            (BASE_VALUE + 21)
#define GET_REG_DEFAULT             (BASE_VALUE + 22)
#define SET_AUXADC_TS_DEBT1         (BASE_VALUE + 23)
#define SET_RESISTANCE              (BASE_VALUE + 24)
#define SET_S_D_MODE                (BASE_VALUE + 25) 
#define GET_AUXADC_TS_DEBT1         (BASE_VALUE + 26)
#define SET_BIT_MODE                (BASE_VALUE + 27)
#define SET_FAV_MODE                (BASE_VALUE + 28)
#define SET_FAV_INTR_MODE           (BASE_VALUE + 29)
#define SET_FAV_SAMPLING_MODE       (BASE_VALUE + 30)
#define SET_FAV_ACC_MODE            (BASE_VALUE + 31) 
#define SET_FAV_LATENCY             (BASE_VALUE + 32)
#define SET_FAV_NOISE               (BASE_VALUE + 33)
#define SET_RAW_INTR                (BASE_VALUE + 34) 
#define SET_RAW_ABORT               (BASE_VALUE + 35)
#define ENABLE_SAMPLE_ADJUST        (BASE_VALUE + 36)
#define SET_SAMPLE_ADJUST           (BASE_VALUE + 37)

#define SEL_MODE_SW			(BASE_VALUE + 38)
#define SEL_MODE_FAV_SW		(BASE_VALUE + 39)
#define SEL_MODE_FAV_HW		(BASE_VALUE + 40)
#define SEL_MODE_RAW_DATA	(BASE_VALUE + 41)


#define ENABLE_TOUCH_LOG            (98)    
#define DISABLE_TOUCH_LOG           (99)  


#endif   /* MT6573_DVT_ADC_TS_H */

