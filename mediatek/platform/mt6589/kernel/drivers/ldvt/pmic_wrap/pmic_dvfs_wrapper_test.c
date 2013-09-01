#include <linux/delay.h>
#include <mach/mt_typedefs.h>
#include "tc_pwrap_ldvt.h"
#include <linux/smp.h>
#include <mach/mt_pmic_wrap.h>
#define AP_DVFS_CON_SET         (0xF0006604)
#define PWRAP_BASE              (0xF000F000)
#define PMIC_WRAP_DVFS_ADR0	    (PWRAP_BASE + 0xF4)
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

#define MAX_RETRY_COUNT         (100)

int mtk_pmic_dvfs_wrapper_test(int loop_count)
{
    int i = 0, try_count = 0, ap_dvfs_con = 0;
    //PWRAPFUC();
    DRV_WriteReg16(PMIC_WRAP_DVFS_ADR0, 0x021A);
    DRV_WriteReg16(PMIC_WRAP_DVFS_ADR1, 0x021A);
    DRV_WriteReg16(PMIC_WRAP_DVFS_ADR2, 0x021A);
    DRV_WriteReg16(PMIC_WRAP_DVFS_ADR3, 0x021A);
    DRV_WriteReg16(PMIC_WRAP_DVFS_ADR4, 0x021A);
    DRV_WriteReg16(PMIC_WRAP_DVFS_ADR5, 0x021A);
    DRV_WriteReg16(PMIC_WRAP_DVFS_ADR6, 0x021A);
    DRV_WriteReg16(PMIC_WRAP_DVFS_ADR7, 0x021A);

    DRV_WriteReg16(PMIC_WRAP_DVFS_WDATA0, 0x58);
    DRV_WriteReg16(PMIC_WRAP_DVFS_WDATA1, 0x58);
    DRV_WriteReg16(PMIC_WRAP_DVFS_WDATA2, 0x58);
    DRV_WriteReg16(PMIC_WRAP_DVFS_WDATA3, 0x58);
    DRV_WriteReg16(PMIC_WRAP_DVFS_WDATA4, 0x58);
    DRV_WriteReg16(PMIC_WRAP_DVFS_WDATA5, 0x58);
    DRV_WriteReg16(PMIC_WRAP_DVFS_WDATA6, 0x58);
    DRV_WriteReg16(PMIC_WRAP_DVFS_WDATA7, 0x58);

    while (loop_count--)
    {
        for (i = 0; i < 8; i++)
        {
            ap_dvfs_con = DRV_Reg32(AP_DVFS_CON_SET);
            DRV_WriteReg32(AP_DVFS_CON_SET, (ap_dvfs_con & ~(0x7)) | i);

            while ((DRV_Reg32(AP_DVFS_CON_SET) & 0x80000000) == 0)
            {
                try_count++;

                if (try_count >= MAX_RETRY_COUNT)
                {

    switch ( raw_smp_processor_id())
				  {
      case 0:
				    g_spm_fail_cpu0++;
        break;
      case 1:
			        g_spm_fail_cpu1++;
        break;
      case 2:
        g_spm_fail_cpu2++;
        break;
      case 3:
        g_spm_fail_cpu3++;
        break;
      default:
        break;
				  }
                    //PWRAPLOG("FAIL: no response from PMIC wrapper\n");
                    return -1;
                }

                //PWRAPLOG("wait for PMIC response an ACK signal, retry count = %d\n", try_count);
                msleep(10);
            }
            try_count = 0;
        }
    }

    switch ( raw_smp_processor_id())
				{
      case 0:
				  g_spm_pass_cpu0++;
        break;
      case 1:
			      g_spm_pass_cpu1++;
        break;
      case 2:
        g_spm_pass_cpu2++;
        break;
      case 3:
        g_spm_pass_cpu3++;
        break;
      default:
        break;
				}
    //PWRAPLOG("PASS: PMIC DVFS Wrapper stress pass\n");
    return 0;
}