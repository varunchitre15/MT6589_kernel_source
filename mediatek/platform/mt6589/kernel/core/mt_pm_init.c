#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/xlog.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "mach/irqs.h"
#include "mach/sync_write.h"
#include "mach/mt_reg_base.h"
#include "mach/mt_typedefs.h"
#include "mach/mt_spm.h"
#include "mach/mt_sleep.h"
#include "mach/mt_dcm.h"
#include "mach/mt_clkmgr.h"
#include "mach/mt_cpufreq.h"
#include "mach/mt_gpufreq.h"
#include "mach/mt_dormant.h"

//fix for bring up
extern int mt_clkmgr_bringup_init(void);
extern void mt_idle_init(void);

/*********************************************************************
 * FUNCTION DEFINATIONS
 ********************************************************************/

unsigned int mt_get_emi_freq(void)
{
    unsigned int output_freq = 0, m4pdiv = 0, fbdiv = 0;

    m4pdiv = (DRV_Reg32(0xF001160C) >> 10) & 0x3;

    fbdiv = (DRV_Reg32(0xF0011608) >> 2) & 0x7F;

    if (m4pdiv == 0x1 && (fbdiv == 0x9 || fbdiv == 0x50))
    {
        output_freq = 250000;
    }
    else if (m4pdiv == 0x1 && (fbdiv == 0x7 || fbdiv == 0x40))
    {
        output_freq = 200000;
    }
    else if (m4pdiv == 0x2 && (fbdiv == 0x4 || fbdiv == 0x50))
    {
        output_freq = 125000;
    }
    else if (m4pdiv == 0x2 && (fbdiv == 0x5 || fbdiv == 0x60))
    {
        output_freq = 133250;
    }
    else if (m4pdiv == 0x1 && (fbdiv == 0xa || fbdiv == 0x58))
    {
        output_freq = 266500;
    }
    else
    {
        output_freq = 0;
    }

    return output_freq;
}
EXPORT_SYMBOL(mt_get_emi_freq);

unsigned int mt_get_bus_freq(void)
{
    unsigned int mainpll_con0, mainpll_con1, main_diff;
    unsigned int clk_cfg_0, bus_clk, output_freq;

    clk_cfg_0 = DRV_Reg32(CLK_CFG_0);

    mainpll_con0 = DRV_Reg32(MAINPLL_CON0);
    mainpll_con1 = DRV_Reg32(MAINPLL_CON1);

    main_diff = ((mainpll_con1 >> 12) - 0x8009A) / 2;

    if ((mainpll_con0 & 0xFF) == 0x01)
    {
        output_freq = 1001 + (main_diff * 13); // Mhz
    }
    else // if (mainpll_con0 & 0xFF) == 0x41)
    {
        output_freq = 1001 + (main_diff * 13) / 2; // Mhz
    }

    if ((clk_cfg_0 & 0x7) == 1) // SYSPLL_D3 = MAINPLL / 3 / 2
    {
        bus_clk = ((output_freq * 1000) / 3) / 2;
    }
    else if ((clk_cfg_0 & 0x7) == 2) // SYSPLL_D4 = MAINPLL / 2 / 4
    {
        bus_clk = ((output_freq * 1000) / 2) / 4;
    }
    else if ((clk_cfg_0 & 0x7) == 3) // SYSPLL_D6 = MAINPLL /2 / 6
    {
        bus_clk = ((output_freq * 1000) / 2) / 6;
    }
    else if ((clk_cfg_0 & 0x7) == 4) // UNIVPLL_D5 = UNIVPLL / 5
    {
        bus_clk = (1248 * 1000) / 5;
    }
    else if ((clk_cfg_0 & 0x7) == 5) // UNIVPLL2_D2 = UNIVPLL / 3 / 2
    {
        bus_clk = (1248 * 1000) / 3 / 2;
    }
    else // CLKSQ
    {
        bus_clk = 26 * 1000;
    }

    return bus_clk; // Khz
}
EXPORT_SYMBOL(mt_get_bus_freq);

unsigned int mt_get_cpu_freq(void)
{
    int output = 0;
    unsigned int temp, clk_cfg_3;

    clk_cfg_3 = DRV_Reg32(CLK_CFG_3);

    mt65xx_reg_sync_writel(0x00000001, CLK_CFG_3);

    if ((DRV_Reg32(TOP_CKMUXSEL) & 0xC) == 0)
    {
        temp = (DRV_Reg32(CLK26CALI) & ~0x4) | 0x4;
        mt65xx_reg_sync_writel(temp, CLK26CALI);
    }
    else
    {
        temp = (DRV_Reg32(CLK26CALI) & ~0x4);
        mt65xx_reg_sync_writel(temp, CLK26CALI);
    }

    /* choose clock source */
    mt65xx_reg_sync_writel(0x00000401, CLK_CFG_3);

    /* set clock divider */
    mt65xx_reg_sync_writel(0x03040901, MBIST_CFG_2);

    /* enable frequency meter */
    temp = DRV_Reg32(CLK26CALI);
    mt65xx_reg_sync_writel((temp | 0x1), CLK26CALI);
    mdelay(10);

    printk("ARMPLL_CON1 = 0x%x, TOP_CKMUXSEL = 0x%x, TOP_CKDIV1 = 0x%x\n", DRV_Reg32(ARMPLL_CON1), DRV_Reg32(TOP_CKMUXSEL), DRV_Reg32(TOP_CKDIV1));

    /* wait frequency meter finish */
    while (DRV_Reg32(CLK26CALI) & 0x1)
    {
        printk("wait for frequency meter finish, CLK26CALI = 0x%x\n", DRV_Reg32(CLK26CALI));

        printk("ARMPLL_CON1 = 0x%x, TOP_CKMUXSEL = 0x%x, TOP_CKDIV1 = 0x%x\n", DRV_Reg32(ARMPLL_CON1), DRV_Reg32(TOP_CKMUXSEL), DRV_Reg32(TOP_CKDIV1));

        mdelay(10);
    }

    temp = DRV_Reg32(CLK26CALI) >> 16;

    output = ((26000 * temp) / 1024) * (1 + 1) * (0x09 + 1);

    mt65xx_reg_sync_writel(clk_cfg_3, CLK_CFG_3);

    printk("counter = 0x%x, cpu frequency = %d Khz\n", temp, output);

    return output;
}

static int cpu_speed_dump_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;

    p += sprintf(p, "%d\n", mt_get_cpu_freq());

    len = p - buf;
    return len;
}

static int __init mt_power_management_init(void)
{
    struct proc_dir_entry *entry = NULL;
    struct proc_dir_entry *pm_init_dir = NULL;

    #if !defined (CONFIG_MT6589_FPGA_CA7)
    xlog_printk(ANDROID_LOG_INFO, "Power/PM_INIT", "Bus Frequency = %d KHz\n", mt_get_bus_freq());

    //cpu dormant driver init
    cpu_dormant_init();

    // SPM driver init
    spm_module_init();

    // Sleep driver init (for suspend)
    slp_module_init();

    //fix for bring up
    //mt_clk_mgr_init(); // clock manager init, including clock gating init
    //mt_clkmgr_bringup_init();
    mt_clkmgr_init();

    //mt_pm_log_init(); // power management log init

    mt_dcm_init(); // dynamic clock management init
    mt_idle_init();

    pm_init_dir = proc_mkdir("pm_init", NULL);
    if (!pm_init_dir)
    {
        pr_err("[%s]: mkdir /proc/pm_init failed\n", __FUNCTION__);
    }
    else
    {
        entry = create_proc_entry("cpu_speed_dump", S_IRUGO, pm_init_dir);
        if (entry)
        {
            entry->read_proc = cpu_speed_dump_read;
        }
    }
    #endif
    
    return 0;
}

arch_initcall(mt_power_management_init);

MODULE_DESCRIPTION("MTK Power Management Init Driver");
MODULE_LICENSE("GPL");