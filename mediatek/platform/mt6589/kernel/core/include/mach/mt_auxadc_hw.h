#ifndef _MTK_ADC_HW_H
#define _MTK_ADC_HW_H

#include <mach/mt_reg_base.h>

#define AUXADC_CON0                     (AUXADC_BASE + 0x000)
#define AUXADC_CON1                     (AUXADC_BASE + 0x004)
//#define AUXADC_CON1_SET                 (AUXADC_BASE + 0x008)
//#define AUXADC_CON1_CLR                 (AUXADC_BASE + 0x00C)
#define AUXADC_CON2                     (AUXADC_BASE + 0x010)
//#define AUXADC_CON3                     (AUXADC_BASE + 0x014)
#if 0
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
#endif
#define AUXADC_DAT0                     (AUXADC_BASE + 0x014)
#define AUXADC_DAT1                     (AUXADC_BASE + 0x018)
#define AUXADC_DAT2                     (AUXADC_BASE + 0x01C)
#define AUXADC_DAT3                     (AUXADC_BASE + 0x020)
#define AUXADC_DAT4                     (AUXADC_BASE + 0x024)
#define AUXADC_DAT5                     (AUXADC_BASE + 0x028)
#define AUXADC_DAT6                     (AUXADC_BASE + 0x02C)
#define AUXADC_DAT7                     (AUXADC_BASE + 0x030)
#define AUXADC_DAT8                     (AUXADC_BASE + 0x034)
#define AUXADC_DAT9                     (AUXADC_BASE + 0x038)
#define AUXADC_DAT10                   (AUXADC_BASE + 0x03C)
#define AUXADC_DAT11                   (AUXADC_BASE + 0x040)
#define AUXADC_DAT12                   (AUXADC_BASE + 0x044)
#define AUXADC_DAT13                   (AUXADC_BASE + 0x048)
#define AUXADC_DAT14                   (AUXADC_BASE + 0x04C)
#define AUXADC_DAT15                   (AUXADC_BASE + 0x050)
#if 0
#define AUXADC_DET_VOLT                 (AUXADC_BASE + 0x084)
#define AUXADC_DET_SEL                  (AUXADC_BASE + 0x088)
#define AUXADC_DET_PERIOD               (AUXADC_BASE + 0x08C)
#define AUXADC_DET_DEBT                 (AUXADC_BASE + 0x090)
#define AUXADC_MISC                     (AUXADC_BASE + 0x094)
#define AUXADC_ECC                      (AUXADC_BASE + 0x098)
#define AUXADC_SAMPLE_LIST              (AUXADC_BASE + 0x09c)
#define AUXADC_ABIST_PERIOD             (AUXADC_BASE + 0x0A0)

//-----------------------------------------------------------------------------

/*AUXADC_SYNC on AUXADC_CON0*/
#define AUXADC_SYNC_CHAN(_line)         (0x0001<<_line)   /*Time event 1*/

/*AUXADC_IMM on AUXADC_CON1*/
#define AUXADC_IMM_CHAN(_line)          (0x0001<<_line)

/*AUXADC_SYN on AUXADC_CON2*/
#define AUXADC_SYN_BIT                  (0x0001)         /*Time event 0*/

/*AUXADC_CON3*/
#define AUXADC_CON3_STATUS_MASK         (0x0001)
#define AUXADC_CON3_STATUS_OFFSET       (0)
#define AUXADC_STATUS_BUSY          	(0x01)
#define AUXADC_STATUS_IDLE          	(0x00) 

#define AUXADC_CON3_SOFT_RST_MASK       (0x0080)
#define AUXADC_CON3_SOFT_RST_OFFSET     (7)

#define AUXADC_CON3_AUTOCLR0_MASK       (0x0100)
#define AUXADC_CON3_AUTOCLR0_OFFSET     (8)

#define AUXADC_CON3_AUTOCLR1_MASK       (0x0200)
#define AUXADC_CON3_AUTOCLR1_OFFSET     (9)

#define AUXADC_CON3_PUWAIT_EN_MASK      (0x0800)
#define AUXADC_CON3_PUWAIT_EN_OFFSET    (11)

#define AUXADC_CON3_AUTOSET_MASK        (0x8000)
#define AUXADC_CON3_AUTOSET_OFFSET      (15)
#endif
#endif   /*_MTK_ADC_HW_H*/

