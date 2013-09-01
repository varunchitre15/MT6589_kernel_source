#ifndef _MT_CPUFREQ_H
#define _MT_CPUFREQ_H

#include <linux/module.h>

/*********************
* Clock Mux Register
**********************/
#define TOP_CKMUXSEL    (0xF0001000)
#define TOP_CKDIV1      (0xF0001008)

/****************************
* PMIC Wrapper DVFS Register
*****************************/
#define PWRAP_BASE              (0xF000F000)
#define PMIC_WRAP_DVFS_ADR0     (PWRAP_BASE + 0xF4)
#define PMIC_WRAP_DVFS_WDATA0   (PWRAP_BASE + 0xF8)
#define PMIC_WRAP_DVFS_ADR1     (PWRAP_BASE + 0xFC)
#define PMIC_WRAP_DVFS_WDATA1   (PWRAP_BASE + 0x100)
#define PMIC_WRAP_DVFS_ADR2     (PWRAP_BASE + 0x104)
#define PMIC_WRAP_DVFS_WDATA2   (PWRAP_BASE + 0x108)
#define PMIC_WRAP_DVFS_ADR3     (PWRAP_BASE + 0x10C)
#define PMIC_WRAP_DVFS_WDATA3   (PWRAP_BASE + 0x110)
#define PMIC_WRAP_DVFS_ADR4     (PWRAP_BASE + 0x114)
#define PMIC_WRAP_DVFS_WDATA4   (PWRAP_BASE + 0x118)
#define PMIC_WRAP_DVFS_ADR5     (PWRAP_BASE + 0x11C)
#define PMIC_WRAP_DVFS_WDATA5   (PWRAP_BASE + 0x120)
#define PMIC_WRAP_DVFS_ADR6     (PWRAP_BASE + 0x124)
#define PMIC_WRAP_DVFS_WDATA6   (PWRAP_BASE + 0x128)
#define PMIC_WRAP_DVFS_ADR7     (PWRAP_BASE + 0x12C)
#define PMIC_WRAP_DVFS_WDATA7   (PWRAP_BASE + 0x130)

/****************************
* MT6320 DVS down software solution.
*****************************/
#define CPU_DVS_DOWN_SW_SOL

/*****************
* extern function 
******************/
extern int mt_cpufreq_state_set(int enabled);
extern void mt_cpufreq_thermal_protect(unsigned int limited_power);
void mt_cpufreq_enable_by_ptpod(void);
unsigned int mt_cpufreq_disable_by_ptpod(void);
extern unsigned int mt_cpufreq_max_frequency_by_DVS(unsigned int num);
void mt_cpufreq_return_default_DVS_by_ptpod(void);
extern unsigned int mt_cpufreq_voltage_set_by_ptpod(unsigned int pmic_volt[], unsigned int array_size);
#endif