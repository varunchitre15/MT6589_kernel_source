#ifndef _MT_PTP_
#define _MT_PTP_

#include <linux/kernel.h>
#include <mach/sync_write.h>

typedef signed char         s8;
typedef unsigned char       u8;
typedef signed short        s16;
typedef unsigned short      u16;
typedef signed int          s32;
typedef unsigned int        u32;
typedef signed long long    s64;
typedef unsigned long long  u64;

u32 PTP_VO_0, PTP_VO_1, PTP_VO_2, PTP_VO_3, PTP_VO_4, PTP_VO_5, PTP_VO_6, PTP_VO_7;
u32 PTP_init2_VO_0, PTP_init2_VO_1, PTP_init2_VO_2, PTP_init2_VO_3, PTP_init2_VO_4, PTP_init2_VO_5, PTP_init2_VO_6, PTP_init2_VO_7;
u32 PTP_INIT_FLAG = 0;
u32 PTP_DCVOFFSET = 0;
u32 PTP_AGEVOFFSET = 0;

#define En_PTP_OD 1
#define PTP_Get_Real_Val 1
#define Set_PMIC_Volt 1
#define En_ISR_log 0

#define ptp_fsm_int_b_IRQ_ID 32

#define ptp_read(addr)		(*(volatile u32 *)(addr))
#define ptp_write(addr, val)	mt65xx_reg_sync_writel(val, addr)

// 6589 PTP register ===========================================
#define PTP_base_addr         (0xf100c000)
#define PTP_ctr_reg_addr     (PTP_base_addr+0x200)

#define PTP_DESCHAR           (PTP_ctr_reg_addr)
#define PTP_TEMPCHAR         (PTP_ctr_reg_addr+0x04)
#define PTP_DETCHAR           (PTP_ctr_reg_addr+0x08)
#define PTP_AGECHAR           (PTP_ctr_reg_addr+0x0c)

#define PTP_DCCONFIG          (PTP_ctr_reg_addr+0x10)
#define PTP_AGECONFIG        (PTP_ctr_reg_addr+0x14)
#define PTP_FREQPCT30         (PTP_ctr_reg_addr+0x18)
#define PTP_FREQPCT74         (PTP_ctr_reg_addr+0x1c)

#define PTP_LIMITVALS          (PTP_ctr_reg_addr+0x20)
#define PTP_VBOOT                 (PTP_ctr_reg_addr+0x24)
#define PTP_DETWINDOW       (PTP_ctr_reg_addr+0x28)
#define PTP_PTPCONFIG         (PTP_ctr_reg_addr+0x2c)

#define PTP_TSCALCS             (PTP_ctr_reg_addr+0x30)
#define PTP_RUNCONFIG         (PTP_ctr_reg_addr+0x34)
#define PTP_PTPEN                 (PTP_ctr_reg_addr+0x38)
#define PTP_INIT2VALS          (PTP_ctr_reg_addr+0x3c)

#define PTP_DCVALUES           (PTP_ctr_reg_addr+0x40)
#define PTP_AGEVALUES         (PTP_ctr_reg_addr+0x44)
#define PTP_VOP30                  (PTP_ctr_reg_addr+0x48)
#define PTP_VOP74                  (PTP_ctr_reg_addr+0x4c)

#define PTP_TEMP                    (PTP_ctr_reg_addr+0x50)
#define PTP_PTPINTSTS           (PTP_ctr_reg_addr+0x54)
#define PTP_PTPINTSTSRAW    (PTP_ctr_reg_addr+0x58)
#define PTP_PTPINTEN             (PTP_ctr_reg_addr+0x5c)

#define PTP_SMSTATE0            (PTP_ctr_reg_addr+0x80)
#define PTP_SMSTATE1            (PTP_ctr_reg_addr+0x84)

// 6589 Thermal Controller register ===========================================
#define PTP_Thermal_ctr_reg_addr (PTP_base_addr)

#define PTP_TEMPMONCTL0                       (PTP_base_addr)
#define PTP_TEMPMONCTL1                       (PTP_base_addr+0x04)
#define PTP_TEMPMONCTL2                       (PTP_base_addr+0x08)
#define PTP_TEMPMONINT                         (PTP_base_addr+0x0c)

#define PTP_TEMPMONINTSTS                   (PTP_base_addr+0x10)
#define PTP_TEMPMONIDET0                     (PTP_base_addr+0x14)
#define PTP_TEMPMONIDET1                     (PTP_base_addr+0x18)
#define PTP_TEMPMONIDET2                     (PTP_base_addr+0x1c)

#define PTP_TEMPH2NTHRE                       (PTP_base_addr+0x24)
#define PTP_TEMPHTHRE                           (PTP_base_addr+0x28)
#define PTP_TEMPCTHRE                            (PTP_base_addr+0x2c)

#define PTP_TEMPOFFSETH                       (PTP_base_addr+0x30)
#define PTP_TEMPOFFSETL                        (PTP_base_addr+0x34)
#define PTP_TEMPMSRCTL0                        (PTP_base_addr+0x38)
#define PTP_TEMPMSRCTL1                        (PTP_base_addr+0x3c)

#define PTP_TEMPAHBPOLL                       (PTP_base_addr+0x40)
#define PTP_TEMPAHBTO                           (PTP_base_addr+0x44)
#define PTP_TEMPADCPNP0                       (PTP_base_addr+0x48)
#define PTP_TEMPADCPNP1                       (PTP_base_addr+0x4c)

#define PTP_TEMPADCPNP2                       (PTP_base_addr+0x50)
#define PTP_TEMPADCMUX                        (PTP_base_addr+0x54)
#define PTP_TEMPADCEXT                         (PTP_base_addr+0x58)
#define PTP_TEMPADCEXT1                       (PTP_base_addr+0x5c)

#define PTP_TEMPADCEN                           (PTP_base_addr+0x60)
#define PTP_TEMPPNPMUXADDR                (PTP_base_addr+0x64)
#define PTP_TEMPADCMUXADDR                (PTP_base_addr+0x68)
#define PTP_TEMPADCEXTADDR                 (PTP_base_addr+0x6c)

#define PTP_TEMPADCEXT1ADDR               (PTP_base_addr+0x70)
#define PTP_TEMPADCENADDR                   (PTP_base_addr+0x74)
#define PTP_TEMPADCVALIDADDR             (PTP_base_addr+0x78)
#define PTP_TEMPADCVOLTADDR                (PTP_base_addr+0x7c)

#define PTP_TEMPRDCTRL                           (PTP_base_addr+0x80)
#define PTP_TEMPADCVALIDMASK              (PTP_base_addr+0x84)
#define PTP_TEMPADCVOLTAGESHIFT        (PTP_base_addr+0x88)
#define PTP_TEMPADCWRITECTRL              (PTP_base_addr+0x8c)

#define PTP_TEMPMSR0                              (PTP_base_addr+0x90)
#define PTP_TEMPMSR1                              (PTP_base_addr+0x94)
#define PTP_TEMPMSR2                              (PTP_base_addr+0x98)

#define PTP_TEMPIMMD0                            (PTP_base_addr+0xa0)
#define PTP_TEMPIMMD1                            (PTP_base_addr+0xa4)
#define PTP_TEMPIMMD2                            (PTP_base_addr+0xa8)

#define PTP_TEMPMONIDET3                      (PTP_base_addr+0xb0)
#define PTP_TEMPADCPNP3                        (PTP_base_addr+0xb4)
#define PTP_TEMPMSR3                              (PTP_base_addr+0xb8)
#define PTP_TEMPIMMD3                            (PTP_base_addr+0xbc)

#define PTP_TEMPPROTCTL                         (PTP_base_addr+0xc0)
#define PTP_TEMPPROTTA                          (PTP_base_addr+0xc4)
#define PTP_TEMPPROTTB                           (PTP_base_addr+0xc8)
#define PTP_TEMPPROTTC                           (PTP_base_addr+0xcc)

#define PTP_TEMPSPARE0                           (PTP_base_addr+0xf0)
#define PTP_TEMPSPARE1                           (PTP_base_addr+0xf4)

#define PTP_PMIC_WRAP_BASE  (0xF000F000)
#define PTP_PMIC_WRAP_DVFS_ADR0             (PTP_PMIC_WRAP_BASE+0xF4)
#define PTP_PMIC_WRAP_DVFS_WDATA0           (PTP_PMIC_WRAP_BASE+0xF8)
#define PTP_PMIC_WRAP_DVFS_ADR1             (PTP_PMIC_WRAP_BASE+0xFC)
#define PTP_PMIC_WRAP_DVFS_WDATA1           (PTP_PMIC_WRAP_BASE+0x100)
#define PTP_PMIC_WRAP_DVFS_ADR2             (PTP_PMIC_WRAP_BASE+0x104)
#define PTP_PMIC_WRAP_DVFS_WDATA2           (PTP_PMIC_WRAP_BASE+0x108)
#define PTP_PMIC_WRAP_DVFS_ADR3             (PTP_PMIC_WRAP_BASE+0x10C)
#define PTP_PMIC_WRAP_DVFS_WDATA3           (PTP_PMIC_WRAP_BASE+0x110)
#define PTP_PMIC_WRAP_DVFS_ADR4             (PTP_PMIC_WRAP_BASE+0x114)
#define PTP_PMIC_WRAP_DVFS_WDATA4           (PTP_PMIC_WRAP_BASE+0x118)
#define PTP_PMIC_WRAP_DVFS_ADR5             (PTP_PMIC_WRAP_BASE+0x11C)
#define PTP_PMIC_WRAP_DVFS_WDATA5           (PTP_PMIC_WRAP_BASE+0x120)
#define PTP_PMIC_WRAP_DVFS_ADR6             (PTP_PMIC_WRAP_BASE+0x124)
#define PTP_PMIC_WRAP_DVFS_WDATA6           (PTP_PMIC_WRAP_BASE+0x128)
#define PTP_PMIC_WRAP_DVFS_ADR7             (PTP_PMIC_WRAP_BASE+0x12C)
#define PTP_PMIC_WRAP_DVFS_WDATA7           (PTP_PMIC_WRAP_BASE+0x130)
 
typedef struct {
    u32 ADC_CALI_EN;
    u32 PTPINITEN;
    u32 PTPMONEN;
    
    u32 MDES;
    u32 BDES;
    u32 DCCONFIG;
    u32 DCMDET;
    u32 DCBDET;
    u32 AGECONFIG;
    u32 AGEM;
    u32 AGEDELTA;
    u32 DVTFIXED;
    u32 VCO;
    u32 MTDES;
    u32 MTS;
    u32 BTS;

    u8 FREQPCT0;
    u8 FREQPCT1;
    u8 FREQPCT2;
    u8 FREQPCT3;
    u8 FREQPCT4;
    u8 FREQPCT5;
    u8 FREQPCT6;
    u8 FREQPCT7;

    u32 DETWINDOW;
    u32 VMAX;
    u32 VMIN;
    u32 DTHI;
    u32 DTLO;
    u32 VBOOT;
    u32 DETMAX;

    u32 DCVOFFSETIN;
    u32 AGEVOFFSETIN;
} PTP_Init_T;

u32 PTP_INIT_01(void);
u32 PTP_INIT_02(void);
u32 PTP_MON_MODE(void);

u32 PTP_get_ptp_level(void);

#endif
