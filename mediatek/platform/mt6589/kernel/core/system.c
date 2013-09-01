#include <linux/kernel.h>
#include <linux/string.h>

#include <mach/mtk_rtc.h>
extern void wdt_arch_reset(char);

/* Fix-me! Add for porting because mark mt6589_dcm.c*/
#if 0
void arch_idle(void)
{
  //do nothing
}
#endif

void arch_reset(char mode, const char *cmd)
{
    char reboot = 0;
    printk("arch_reset: cmd = %s\n", cmd ? : "NULL");

    if (cmd && !strcmp(cmd, "charger")) {
        /* do nothing */
    } else if (cmd && !strcmp(cmd, "recovery")) {
        rtc_mark_recovery();
    } 
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
	else if (cmd && !strcmp(cmd, "kpoc")){
		rtc_mark_kpoc();
	}
#endif
    else {
    	reboot = 1;
    }
    wdt_arch_reset(reboot);

}

