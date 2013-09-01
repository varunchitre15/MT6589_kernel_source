
#ifndef _MT6589_THERMAL_H
#define _MT6589_THERMAL_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "mach/sync_write.h"
#include "mach/mt_reg_base.h"
#include "mach/mt_typedefs.h"
//#include "mach/mt6575_auxadc_hw.h"


/*******************************************************************************
 * AUXADC Register Definition
 ******************************************************************************/
#define AUXADC_CON0_V       (AUXADC_BASE + 0x000)	//yes, 0x11003000
#define AUXADC_CON1_V       (AUXADC_BASE + 0x004)
#define AUXADC_CON1_SET_V   (AUXADC_BASE + 0x008)
#define AUXADC_CON1_CLR_V   (AUXADC_BASE + 0x00C)
#define AUXADC_CON2_V       (AUXADC_BASE + 0x010)
//#define AUXADC_CON3_V       (AUXADC_BASE + 0x014)
#define AUXADC_DAT0_V       (AUXADC_BASE + 0x014)
#define AUXADC_DAT1_V       (AUXADC_BASE + 0x018)
#define AUXADC_DAT2_V       (AUXADC_BASE + 0x01C)
#define AUXADC_DAT3_V       (AUXADC_BASE + 0x020)
#define AUXADC_DAT4_V       (AUXADC_BASE + 0x024)
#define AUXADC_DAT5_V       (AUXADC_BASE + 0x028)
#define AUXADC_DAT6_V       (AUXADC_BASE + 0x02C)
#define AUXADC_DAT7_V       (AUXADC_BASE + 0x030)
#define AUXADC_DAT8_V       (AUXADC_BASE + 0x034)
#define AUXADC_DAT9_V       (AUXADC_BASE + 0x038)
#define AUXADC_DAT10_V       (AUXADC_BASE + 0x03C)
#define AUXADC_DAT11_V       (AUXADC_BASE + 0x040)
#define AUXADC_MISC_V       (AUXADC_BASE + 0x094)

#define AUXADC_CON0_P       (AUXADC_BASE + 0x000 - 0xE0000000)
#define AUXADC_CON1_P       (AUXADC_BASE + 0x004 - 0xE0000000)
#define AUXADC_CON1_SET_P   (AUXADC_BASE + 0x008 - 0xE0000000)
#define AUXADC_CON1_CLR_P   (AUXADC_BASE + 0x00C - 0xE0000000)
#define AUXADC_CON2_P       (AUXADC_BASE + 0x010 - 0xE0000000)
//#define AUXADC_CON3_P       (AUXADC_BASE + 0x014 - 0x30000000)
#define AUXADC_DAT0_P       (AUXADC_BASE + 0x014 - 0xE0000000)
#define AUXADC_DAT1_P       (AUXADC_BASE + 0x018 - 0xE0000000)
#define AUXADC_DAT2_P       (AUXADC_BASE + 0x01C - 0xE0000000)
#define AUXADC_DAT3_P       (AUXADC_BASE + 0x020 - 0xE0000000)
#define AUXADC_DAT4_P       (AUXADC_BASE + 0x024 - 0xE0000000)
#define AUXADC_DAT5_P       (AUXADC_BASE + 0x028 - 0xE0000000)
#define AUXADC_DAT6_P       (AUXADC_BASE + 0x02C - 0xE0000000)
#define AUXADC_DAT7_P       (AUXADC_BASE + 0x030 - 0xE0000000)
#define AUXADC_DAT8_P       (AUXADC_BASE + 0x034 - 0xE0000000)
#define AUXADC_DAT9_P       (AUXADC_BASE + 0x038 - 0xE0000000)
#define AUXADC_DAT10_P       (AUXADC_BASE + 0x03C - 0xE0000000)
#define AUXADC_DAT11_P       (AUXADC_BASE + 0x040 - 0xE0000000)

#define AUXADC_MISC_P       (AUXADC_BASE + 0x094 - 0xE0000000)

/*******************************************************************************
 * Peripheral Configuration Register Definition
 ******************************************************************************/
#define PERI_GLOBALCON_RST0 (PERICFG_BASE + 0x000) //yes, 0x10003000

/*******************************************************************************
 * APMixedSys Configuration Register Definition
 ******************************************************************************/
 #define TS_CON0             (APMIXED_BASE + 0x600) //yes 0x10209000
 #define TS_CON1             (APMIXED_BASE + 0x604)
//#define TS_CON0             (APMIXED_BASE + 0x800)
//#define TS_CON1             (APMIXED_BASE + 0x804)
//#define TS_CON2             (APMIXED_BASE + 0x808)
//#define TS_CON3             (APMIXED_BASE + 0x80C)

/*******************************************************************************
 * Thermal Controller Register Definition
 ******************************************************************************/
#define TEMPMONCTL0         (THERMAL_BASE + 0x000) //yes 0x1100c000
#define TEMPMONCTL1         (THERMAL_BASE + 0x004)
#define TEMPMONCTL2         (THERMAL_BASE + 0x008)
#define TEMPMONINT          (THERMAL_BASE + 0x00C)
#define TEMPMONINTSTS       (THERMAL_BASE + 0x010)
#define TEMPMONIDET0        (THERMAL_BASE + 0x014)
#define TEMPMONIDET1        (THERMAL_BASE + 0x018)
#define TEMPMONIDET2        (THERMAL_BASE + 0x01C)
#define TEMPH2NTHRE         (THERMAL_BASE + 0x024)
#define TEMPHTHRE           (THERMAL_BASE + 0x028)
#define TEMPCTHRE           (THERMAL_BASE + 0x02C)
#define TEMPOFFSETH         (THERMAL_BASE + 0x030)
#define TEMPOFFSETL         (THERMAL_BASE + 0x034)
#define TEMPMSRCTL0         (THERMAL_BASE + 0x038)
#define TEMPMSRCTL1         (THERMAL_BASE + 0x03C)
#define TEMPAHBPOLL         (THERMAL_BASE + 0x040)
#define TEMPAHBTO           (THERMAL_BASE + 0x044)
#define TEMPADCPNP0         (THERMAL_BASE + 0x048)
#define TEMPADCPNP1         (THERMAL_BASE + 0x04C)
#define TEMPADCPNP2         (THERMAL_BASE + 0x050)
#define TEMPADCMUX          (THERMAL_BASE + 0x054)
#define TEMPADCEXT          (THERMAL_BASE + 0x058)
#define TEMPADCEXT1         (THERMAL_BASE + 0x05C)
#define TEMPADCEN           (THERMAL_BASE + 0x060)
#define TEMPPNPMUXADDR      (THERMAL_BASE + 0x064)
#define TEMPADCMUXADDR      (THERMAL_BASE + 0x068)
#define TEMPADCEXTADDR      (THERMAL_BASE + 0x06C)
#define TEMPADCEXT1ADDR     (THERMAL_BASE + 0x070)
#define TEMPADCENADDR       (THERMAL_BASE + 0x074)
#define TEMPADCVALIDADDR    (THERMAL_BASE + 0x078)
#define TEMPADCVOLTADDR     (THERMAL_BASE + 0x07C)
#define TEMPRDCTRL          (THERMAL_BASE + 0x080)
#define TEMPADCVALIDMASK    (THERMAL_BASE + 0x084)
#define TEMPADCVOLTAGESHIFT (THERMAL_BASE + 0x088)
#define TEMPADCWRITECTRL    (THERMAL_BASE + 0x08C)
#define TEMPMSR0            (THERMAL_BASE + 0x090)
#define TEMPMSR1            (THERMAL_BASE + 0x094)
#define TEMPMSR2            (THERMAL_BASE + 0x098)
#define TEMPIMMD0           (THERMAL_BASE + 0x0A0)
#define TEMPIMMD1           (THERMAL_BASE + 0x0A4)
#define TEMPIMMD2           (THERMAL_BASE + 0x0A8)

#define TEMPPROTCTL         (THERMAL_BASE + 0x0C0)
#define TEMPPROTTA          (THERMAL_BASE + 0x0C4)
#define TEMPPROTTB          (THERMAL_BASE + 0x0C8)
#define TEMPPROTTC          (THERMAL_BASE + 0x0CC)

#define TEMPSPARE0          (THERMAL_BASE + 0x0F0)
#define TEMPSPARE1          (THERMAL_BASE + 0x0F4)
#define TEMPSPARE2          (THERMAL_BASE + 0x0F8)
#define TEMPSPARE3          (THERMAL_BASE + 0x0FC)

/*******************************************************************************
 * Thermal Controller Register Mask Definition
 ******************************************************************************/
#define THERMAL_ENABLE_SEN0     0x1
#define THERMAL_ENABLE_SEN1     0x2
#define THERMAL_ENABLE_SEN2     0x4
#define THERMAL_MONCTL0_MASK    0x00000007

#define THERMAL_PUNT_MASK       0x00000FFF
#define THERMAL_FSINTVL_MASK    0x03FF0000
#define THERMAL_SPINTVL_MASK    0x000003FF
#define THERMAL_MON_INT_MASK    0x0007FFFF

#define THERMAL_MON_CINTSTS0    0x000001
#define THERMAL_MON_HINTSTS0    0x000002
#define THERMAL_MON_LOINTSTS0   0x000004
#define THERMAL_MON_HOINTSTS0   0x000008
#define THERMAL_MON_NHINTSTS0   0x000010
#define THERMAL_MON_CINTSTS1    0x000020
#define THERMAL_MON_HINTSTS1    0x000040
#define THERMAL_MON_LOINTSTS1   0x000080
#define THERMAL_MON_HOINTSTS1   0x000100
#define THERMAL_MON_NHINTSTS1   0x000200
#define THERMAL_MON_CINTSTS2    0x000400
#define THERMAL_MON_HINTSTS2    0x000800
#define THERMAL_MON_LOINTSTS2   0x001000
#define THERMAL_MON_HOINTSTS2   0x002000
#define THERMAL_MON_NHINTSTS2   0x004000
#define THERMAL_MON_TOINTSTS    0x008000
#define THERMAL_MON_IMMDINTSTS0 0x010000
#define THERMAL_MON_IMMDINTSTS1 0x020000
#define THERMAL_MON_IMMDINTSTS2 0x040000
#define THERMAL_MON_FILTINTSTS0 0x080000
#define THERMAL_MON_FILTINTSTS1 0x100000
#define THERMAL_MON_FILTINTSTS2 0x200000


#define THERMAL_tri_SPM_State0	0x20000000
#define THERMAL_tri_SPM_State1	0x40000000
#define THERMAL_tri_SPM_State2	0x80000000


#define THERMAL_MSRCTL0_MASK    0x00000007
#define THERMAL_MSRCTL1_MASK    0x00000038
#define THERMAL_MSRCTL2_MASK    0x000001C0

//extern int thermal_one_shot_handler(int times);
struct TS_PTPOD
{
	unsigned int ts_MTS;
	unsigned int ts_BTS;
};
extern void get_thermal_slope_intercept(struct TS_PTPOD *ts_info);
extern void set_taklking_flag(bool flag);
extern int mtktscpu_get_cpu_temp(void);
#endif

