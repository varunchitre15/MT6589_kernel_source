#ifndef _HOTPLUG
#define _HOTPLUG

#include <linux/xlog.h>
#include <linux/kernel.h>   //printk
#include <asm/atomic.h>
#include <mach/mt_reg_base.h>

/* log */
#define HOTPLUG_LOG_NONE                                0
#define HOTPLUG_LOG_WITH_XLOG                           1
#define HOTPLUG_LOG_WITH_PRINTK                         2

#define HOTPLUG_LOG_PRINT                               HOTPLUG_LOG_WITH_PRINTK

#if (HOTPLUG_LOG_PRINT == HOTPLUG_LOG_NONE)
#define HOTPLUG_INFO(fmt, args...)                    
#elif (HOTPLUG_LOG_PRINT == HOTPLUG_LOG_WITH_XLOG)
#define HOTPLUG_INFO(fmt, args...)                      xlog_printk(ANDROID_LOG_INFO, "Power/hotplug", fmt, ##args)
#elif (HOTPLUG_LOG_PRINT == HOTPLUG_LOG_WITH_PRINTK)
#define HOTPLUG_INFO(fmt, args...)                      printk("[Power/hotplug] "fmt, ##args)
#endif


/* profilling */
//#define CONFIG_HOTPLUG_PROFILING                        
#define CONFIG_HOTPLUG_PROFILING_COUNT                  100


/* register address */
#define BOOT_ADDR                                       (SRAMROM_BASE + 0x0030)


/* register read/write */
#define REG_READ(addr)           (*(volatile u32 *)(addr))
#define REG_WRITE(addr, value)   (*(volatile u32 *)(addr) = (u32)(value))


/* power on/off cpu*/
#define CONFIG_HOTPLUG_WITH_POWER_CTRL


/* global variable */
extern volatile int pen_release;
extern atomic_t hotplug_cpu_count;


/* mtk hotplug mechanism control interface for thermal protect */
//extern void mtk_hotplug_mechanism_thermal_protect(int limited_cpus);


#endif //enf of #ifndef _HOTPLUG