

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

#define MT65XX_IRQ_LOWBAT_LINE    MT6575_LOWBATTERY_IRQ_ID
#define APCONFIG_BASE 0 // only for build
#define TDMA_TIMER_BASE 0
#define APMCU_CG_CLR0                   (APCONFIG_BASE + 0x0308)
#define APMCU_CG_SET0                   (APCONFIG_BASE + 0x0304)

#define MDMCU_CG_CON0                   (MDCONFIG_BASE + 0x0300)
#define MDMCU_CG_SET0                   (MDCONFIG_BASE + 0x0304)

#define TDMA_AUXEV0                     (TDMA_TIMER_BASE + 0x0400)
#define TDMA_AUXEV1                     (TDMA_TIMER_BASE + 0x0404)

#define AUXADC_CON0                     (AUXADC_BASE + 0x000)
#define AUXADC_CON1                     (AUXADC_BASE + 0x004)
#define AUXADC_CON1_SET                 (AUXADC_BASE + 0x008)
#define AUXADC_CON1_CLR                 (AUXADC_BASE + 0x00C)
#define AUXADC_CON2                     (AUXADC_BASE + 0x010)
#define AUXADC_CON3                     (AUXADC_BASE + 0x014)

#define AUXADC_DAT0                     (AUXADC_BASE + 0x018)
#define AUXADC_DAT1                     (AUXADC_BASE + 0x01C)
#define AUXADC_DAT2                     (AUXADC_BASE + 0x020)
#define AUXADC_DAT3                     (AUXADC_BASE + 0x024)
#define AUXADC_DAT4                     (AUXADC_BASE + 0x028)
#define AUXADC_DAT5                     (AUXADC_BASE + 0x024+0x008)
#define AUXADC_DAT6                     (AUXADC_BASE + 0x028+0x008)
#define AUXADC_DAT7                     (AUXADC_BASE + 0x02C+0x008)
#define AUXADC_DAT8                     (AUXADC_BASE + 0x030+0x008)
#define AUXADC_DAT9                     (AUXADC_BASE + 0x034+0x008)
#define AUXADC_DAT10                    (AUXADC_BASE + 0x038+0x008)
#define AUXADC_DAT11                    (AUXADC_BASE + 0x03C+0x008)
#define AUXADC_DAT12                    (AUXADC_BASE + 0x040+0x008)
#define AUXADC_DAT13                    (AUXADC_BASE + 0x044+0x008)
#define AUXADC_DET_VOLT                 (AUXADC_BASE + 0x084)
#define AUXADC_DET_SEL                  (AUXADC_BASE + 0x088)
#define AUXADC_DET_PERIOD               (AUXADC_BASE + 0x08C)
#define AUXADC_DET_DEBT                 (AUXADC_BASE + 0x090)
#define AUXADC_MISC                     (AUXADC_BASE + 0x094)
#define AUXADC_ECC                      (AUXADC_BASE + 0x098)
#define AUXADC_SAMPLE_LIST              (AUXADC_BASE + 0x09c)
#define AUXADC_ABIST_PERIOD             (AUXADC_BASE + 0x0A0)

#define BASE_VALUE   (100)
#define SET_AUXADC_CON0                 (BASE_VALUE + 1)
#define SET_AUXADC_CON1                 (BASE_VALUE + 2)
#define SET_AUXADC_CON2                 (BASE_VALUE + 3)
#define SET_AUXADC_CON3                 (BASE_VALUE + 4)
#define SET_AUXADC_DAT0                 (BASE_VALUE + 5)
#define SET_AUXADC_DAT1                 (BASE_VALUE + 6)
#define SET_AUXADC_DAT2                 (BASE_VALUE + 7)
#define SET_AUXADC_DAT3                 (BASE_VALUE + 8)
#define SET_AUXADC_DAT4                 (BASE_VALUE + 9)
#define SET_AUXADC_DAT5                 (BASE_VALUE + 10)
#define SET_AUXADC_DAT6                 (BASE_VALUE + 11)
#define SET_AUXADC_DAT7                 (BASE_VALUE + 12)
#define SET_AUXADC_DAT8                 (BASE_VALUE + 13)
#define SET_AUXADC_DAT9                 (BASE_VALUE + 14)
#define SET_AUXADC_DAT10                (BASE_VALUE + 15)
#define SET_AUXADC_DAT11                (BASE_VALUE + 16)
#define SET_AUXADC_DAT12                (BASE_VALUE + 17)
#define SET_AUXADC_DAT13                (BASE_VALUE + 18)
#define SET_AUXADC_DET_VOLT             (BASE_VALUE + 19)
#define SET_AUXADC_DET_SEL              (BASE_VALUE + 20)
#define SET_AUXADC_DET_PERIOD           (BASE_VALUE + 21)
#define SET_AUXADC_DET_DEBT             (BASE_VALUE + 22)
#define SET_AUXADC_TXPWR_CH             (BASE_VALUE + 23)
#define SET_AUXADC_2GTX_CH              (BASE_VALUE + 24)
#define SET_AUXADC_2GTX_DAT0            (BASE_VALUE + 25)
#define SET_AUXADC_2GTX_DAT1            (BASE_VALUE + 26)
#define SET_AUXADC_2GTX_DAT2            (BASE_VALUE + 27)
#define SET_AUXADC_2GTX_DAT3            (BASE_VALUE + 28)
#define SET_AUXADC_2GTX_DAT4            (BASE_VALUE + 29)
#define SET_AUXADC_2GTX_DAT5            (BASE_VALUE + 30)

#define GET_AUXADC_CON0                 (BASE_VALUE + 31)
#define GET_AUXADC_CON1                 (BASE_VALUE + 32)
#define GET_AUXADC_CON2                 (BASE_VALUE + 33)
#define GET_AUXADC_CON3                 (BASE_VALUE + 34)
#define GET_AUXADC_DAT0                 (BASE_VALUE + 35)
#define GET_AUXADC_DAT1                 (BASE_VALUE + 36)
#define GET_AUXADC_DAT2                 (BASE_VALUE + 37)
#define GET_AUXADC_DAT3                 (BASE_VALUE + 38)
#define GET_AUXADC_DAT4                 (BASE_VALUE + 39)
#define GET_AUXADC_DAT5                 (BASE_VALUE + 40)
#define GET_AUXADC_DAT6                 (BASE_VALUE + 41)
#define GET_AUXADC_DAT7                 (BASE_VALUE + 42)
#define GET_AUXADC_DAT8                 (BASE_VALUE + 43)
#define GET_AUXADC_DAT9                 (BASE_VALUE + 44)
#define GET_AUXADC_DAT10                (BASE_VALUE + 45)
#define GET_AUXADC_DAT11                (BASE_VALUE + 46)
#define GET_AUXADC_DAT12                (BASE_VALUE + 47)
#define GET_AUXADC_DAT13                (BASE_VALUE + 48)
#define GET_AUXADC_DET_VOLT             (BASE_VALUE + 49)
#define GET_AUXADC_DET_SEL              (BASE_VALUE + 50)
#define GET_AUXADC_DET_PERIOD           (BASE_VALUE + 51)
#define GET_AUXADC_DET_DEBT             (BASE_VALUE + 52)
#define GET_AUXADC_TXPWR_CH             (BASE_VALUE + 53)
#define GET_AUXADC_2GTX_CH              (BASE_VALUE + 54)
#define GET_AUXADC_2GTX_DAT0            (BASE_VALUE + 55)
#define GET_AUXADC_2GTX_DAT1            (BASE_VALUE + 56)
#define GET_AUXADC_2GTX_DAT2            (BASE_VALUE + 57)
#define GET_AUXADC_2GTX_DAT3            (BASE_VALUE + 58)
#define GET_AUXADC_2GTX_DAT4            (BASE_VALUE + 59)
#define GET_AUXADC_2GTX_DAT5            (BASE_VALUE + 60)

#define SET_ADC_WAKE_SRC	(BASE_VALUE + 61)
#define ENABLE_SYN_MODE                 (BASE_VALUE + 80)
#define DISABLE_SYN_MODE                (BASE_VALUE + 81)

#define ENABLE_ADC_RUN                  (BASE_VALUE + 84)
#define DISABLE_ADC_RUN                 (BASE_VALUE + 85)

#define ENABLE_BG_DETECT                (BASE_VALUE + 86)    
#define DISABLE_BG_DETECT               (BASE_VALUE + 87)   

#define ENABLE_3G_TX                    (BASE_VALUE + 88)    
#define DISABLE_3G_TX                   (BASE_VALUE + 89)  

#define ENABLE_2G_TX                    (BASE_VALUE + 90)    
#define DISABLE_2G_TX                   (BASE_VALUE + 91)  
#define SET_DET_VOLT                    (BASE_VALUE + 92)
#define GET_DET_VOLT                    (BASE_VALUE + 93)
#define SET_DET_PERIOD                  (BASE_VALUE + 94)
#define GET_DET_PERIOD                  (BASE_VALUE + 95)
#define SET_DET_DEBT                    (BASE_VALUE + 96)
#define GET_DET_DEBT                    (BASE_VALUE + 97)

#define ENABLE_ADC_LOG                  (98)
#define DISABLE_ADC_LOG                 (99)
   
#define MT6573_IRQ_LOWBAT_CODE          (81)

#endif

