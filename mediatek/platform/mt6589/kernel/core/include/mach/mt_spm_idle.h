#ifndef _MT_SPM_IDLE_
#define _MT_SPM_IDLE_

#include <linux/kernel.h>

//#ifdef MTK_SPM_MCDI_ENABLE
// for IPI, Hotplug and Idle thread
//#define SPM_MCDI_FUNC
//#endif

#define clc_emerg(fmt, args...)     printk(KERN_EMERG "[CLC] " fmt, ##args)
#define clc_alert(fmt, args...)     printk(KERN_ALERT "[CLC] " fmt, ##args)
#define clc_crit(fmt, args...)      printk(KERN_CRIT "[CLC] " fmt, ##args)
#define clc_error(fmt, args...)     printk(KERN_ERR "[CLC] " fmt, ##args)
#define clc_warning(fmt, args...)   printk(KERN_WARNING "[CLC] " fmt, ##args)
#define clc_notice(fmt, args...)    printk(KERN_NOTICE "[CLC] " fmt, ##args)
#define clc_info(fmt, args...)      printk(KERN_INFO "[CLC] " fmt, ##args)
#define clc_debug(fmt, args...)     printk(KERN_DEBUG "[CLC] " fmt, ##args)


void spm_check_core_status_before(u32 target_core);
void spm_check_core_status_after(u32 target_core);
void spm_hot_plug_in_before(u32 target_core);
void spm_hot_plug_out_after(u32 target_core);
void spm_disable_sodi(void);
void spm_enable_sodi(void);
void spm_mcdi_wfi(void);

#endif
