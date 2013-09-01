#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include <linux/xlog.h>

#define HIB_CORE_DEBUG 0
#define _TAG_HIB_M "HIB/CORE"
extern bool console_suspend_enabled; // from printk.c
#if (HIB_CORE_DEBUG)
#undef hib_log
#define hib_log(fmt, ...)	if (!console_suspend_enabled) xlog_printk(ANDROID_LOG_WARN, _TAG_HIB_M, fmt, ##__VA_ARGS__);
#else
#define hib_log(fmt, ...)
#endif
#undef hib_warn
#define hib_warn(fmt, ...)  if (!console_suspend_enabled) xlog_printk(ANDROID_LOG_WARN, _TAG_HIB_M, fmt,  ##__VA_ARGS__);
#undef hib_err
#define hib_err(fmt, ...)   if (!console_suspend_enabled) xlog_printk(ANDROID_LOG_ERROR, _TAG_HIB_M, fmt,  ##__VA_ARGS__);

#define HIB_USRPROGRAM "/system/bin/ipohctl"

#ifdef CONFIG_PM_AUTOSLEEP

/* kernel/power/autosleep.c */
extern int pm_autosleep_lock(void);
extern void pm_autosleep_unlock(void);
extern suspend_state_t pm_autosleep_state(void);

#else /* !CONFIG_PM_AUTOSLEEP */

static inline int pm_autosleep_lock(void) { return 0; }
static inline void pm_autosleep_unlock(void) {}
static inline suspend_state_t pm_autosleep_state(void) { return PM_SUSPEND_ON; }

#endif /* !CONFIG_PM_AUTOSLEEP */

#ifdef CONFIG_PM_WAKELOCKS

/* kernel/power/wakelock.c */
extern int pm_wake_lock(const char *buf);
extern int pm_wake_unlock(const char *buf);

#endif /* !CONFIG_PM_WAKELOCKS */


bool system_is_hibernating = false;
EXPORT_SYMBOL(system_is_hibernating);

enum {
    HIB_FAILED_TO_SHUTDOWN = 0,
    HIB_FAILED_TO_S2RAM,
};
static int hibernation_failed_action = HIB_FAILED_TO_S2RAM;

#define MAX_HIB_FAILED_CNT 3
static int hib_failed_cnt = 0;

//----------------------------------//
//------ userspace programs ---------//
//----------------------------------//
static void usr_restore_func(struct work_struct *data)
{
    static char *envp[] = {
        "HOME=/data",
        "TERM=vt100",
        "PATH=/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin",
        NULL };
    static char *argv[] = {
        HIB_USRPROGRAM,
        "--mode=restore",
        NULL, NULL, NULL, NULL, NULL, NULL
    };
    int retval;

    // start ipo booting
    hib_log("call userspace program '%s %s'\n", argv[0], argv[1]);
    retval = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    if (retval && retval != 256)
        hib_err("Failed to launch userspace program '%s %s': "
               "Error %d\n", argv[0], argv[1], retval);
}
//DECLARE_DELAYED_WORK(usr_restore_work, usr_restore_func);

// trigger userspace recover for hibernation failed
static void usr_recover_func(struct work_struct *data)
{
    static char *envp[] = {
        "HOME=/data",
        "TERM=vt100",
        "PATH=/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin",
        NULL };
    static char *argv[] = {
        HIB_USRPROGRAM,
        "--mode=recover",
        NULL, NULL, NULL, NULL, NULL, NULL
    };
    int retval;

    hib_log("call userspace program '%s %s'\n", argv[0],argv[1]);
    retval = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    if (retval && retval != 256)
        hib_err("Failed to launch userspace program '%s %s': "
               "Error %d\n", argv[0], argv[1], retval);
}

extern int hybrid_sleep_mode(void);
// NOTICE: this function MUST be called under autosleep_lock (in autosleep.c) is locked!!
int mtk_hibernate_via_autosleep(suspend_state_t *autosleep_state)
{
    int err = 0;
    hib_log("entering hibernation state(%d)\n", *autosleep_state);
    err = hibernate();
    if (err) {
        hib_warn("@@@@@@@@@@@@@@@@@@@@@@@@@\n@@_Hibernation Failed_@@@\n@@@@@@@@@@@@@@@@@@@@@@@@@\n");
        if (hibernation_failed_action == HIB_FAILED_TO_SHUTDOWN) {
            kernel_power_off();
            kernel_halt();
            BUG();
        } else if (hibernation_failed_action == HIB_FAILED_TO_S2RAM) {
            hib_warn("hibernation failed: so changing state(%d->%d) err(%d)\n", *autosleep_state, PM_SUSPEND_MEM, err);
            if (++hib_failed_cnt >= MAX_HIB_FAILED_CNT)
                hibernation_failed_action = HIB_FAILED_TO_SHUTDOWN;
            // userspace recover if hibernation failed
            usr_recover_func(NULL);
            *autosleep_state = PM_SUSPEND_MEM;
            system_is_hibernating = false;
         } else {
            hib_err("@@@@@@@@@@@@@@@@@@\n@_FATAL ERROR !!!_\n@@@@@@@@@@@@@@@@@@@\n");
            BUG();
        }
    } else {
        if (hybrid_sleep_mode()) {
            hib_warn("hybrid sleep mode so changing state(%d->%d)\n", *autosleep_state, PM_SUSPEND_MEM);
            *autosleep_state = PM_SUSPEND_MEM; //continue suspend to ram if hybrid sleep mode
        } else {
            hib_warn("hibernation succeeded: so changing state(%d->%d) err(%d) \n", *autosleep_state, PM_SUSPEND_ON, err);
            hib_warn("start trigger ipod\n");
            //usr_bootanim_start(NULL);
            //schedule_delayed_work(&usr_restore_work, HZ*0.05);
            usr_restore_func(NULL);
            *autosleep_state = PM_SUSPEND_ON; // if this is not set, it will recursively do hibernating!!
        }
        hib_failed_cnt = 0;
        pm_wake_lock("IPOD_HIB_WAKELOCK");
        system_is_hibernating = false;
    }

    return err;
}
EXPORT_SYMBOL(mtk_hibernate_via_autosleep);

// called by echo "disk" > /sys/power/state
int mtk_hibernate(void)
{
    int err = 0;

    hib_log("entering hibernation\n");
    err = hibernate();
    if (err) {
        hib_warn("@@@@@@@@@@@@@@@@@@@@@@@@@\n@@_Hibernation Failed_@@@\n@@@@@@@@@@@@@@@@@@@@@@@@@\n");
        if (hibernation_failed_action == HIB_FAILED_TO_SHUTDOWN) {
            kernel_power_off();
            kernel_halt();
            BUG();
        } else if (hibernation_failed_action == HIB_FAILED_TO_S2RAM) {
            hib_warn("hibernation failed, suspend to ram instead!\n");
            if (++hib_failed_cnt >= MAX_HIB_FAILED_CNT)
                hibernation_failed_action = HIB_FAILED_TO_SHUTDOWN;
            // userspace recover if hibernation failed
            usr_recover_func(NULL);
            system_is_hibernating = false;
        } else {
            hib_err("@@@@@@@@@@@@@@@@@@\n@_FATAL ERROR !!!_\n@@@@@@@@@@@@@@@@@@@\n");
            BUG();
        }
    } else {
        if (!hybrid_sleep_mode()) {
            hib_warn("start trigger ipod\n");
            //schedule_delayed_work(&usr_bootanim_start_work, HZ*1.0);
            //schedule_delayed_work(&usr_restore_work, HZ*0.05);
            //usr_bootanim_start(NULL);
            usr_restore_func(NULL);
        }
        hib_failed_cnt = 0;
        pm_wake_lock("IPOD_HIB_WAKELOCK");
        system_is_hibernating = false;
    }

    return err;
}
EXPORT_SYMBOL(mtk_hibernate);

int pre_hibernate(void)
{
    int err = 0;

    // flag to prevent suspend to ram
    system_is_hibernating = true;

    pm_wake_unlock("IPOD_HIB_WAKELOCK");

    // user space program stuffs before hibernation start

    return err;
}
EXPORT_SYMBOL(pre_hibernate);

int mtk_hibernate_abort(void)
{
    toi_abort_hibernate();
    return 0;
}
EXPORT_SYMBOL(mtk_hibernate_abort);
