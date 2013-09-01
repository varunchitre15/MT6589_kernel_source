#include <linux/errno.h>    //EPERM
#include <asm/cacheflush.h> //flush_cache_all
#include <mach/hotplug.h>
#ifdef CONFIG_HOTPLUG_WITH_POWER_CTRL
#include <mach/mt_spm_mtcmos.h>
#endif
#include <mach/mt_spm_idle.h>



/* 
 * extern function 
 */
extern void __disable_dcache(void);         //definition in mt_cache_v7.S
extern void __enable_dcache(void);          //definition in mt_cache_v7.S
extern void __inner_clean_dcache_L2(void);  //definition in mt_cache_v7.S
extern void inner_dcache_flush_L1(void);    //definition in inner_cache.c
extern void __switch_to_smp(void);          //definition in mt_hotplug.S
extern void __switch_to_amp(void);          //definition in mt_hotplug.S
#ifdef CONFIG_MTK_WD_KICKER
extern void wk_stop_kick_cpu(int cpu);
#endif



/* 
 * global variable 
 */
atomic_t hotplug_cpu_count = ATOMIC_INIT(1);



/*
 * static function
 */
static inline void cpu_enter_lowpower(unsigned int cpu)
{
    //HOTPLUG_INFO("cpu_enter_lowpower\n");

#ifdef SPM_MCDI_FUNC
    spm_hot_plug_out_after(cpu);
#endif
    
    /* Clear the SCTLR C bit to prevent further data cache allocation */
    __disable_dcache();
    
    /* Clean and invalidate all data from the L1 data cache */
    inner_dcache_flush_L1();
    //Just flush the cache.
    //flush_cache_all();
    
    /* Clean all data from the L2 data cache */
    __inner_clean_dcache_L2();
    
    /* Execute a CLREX instruction */
    __asm__ __volatile__("clrex");
    
    /* Switch the processor from SMP mode to AMP mode by clearing the ACTLR SMP bit */
    __switch_to_amp();
}

static inline void cpu_leave_lowpower(unsigned int cpu)
{
    //HOTPLUG_INFO("cpu_leave_lowpower\n");
    
    /* Set the ACTLR.SMP bit to 1 for SMP mode */
    __switch_to_smp();
    
    /* Enable dcache */
    __enable_dcache();
}

static inline void platform_do_lowpower(unsigned int cpu, int *spurious)
{
    /* Just enter wfi for now. TODO: Properly shut off the cpu. */
    for (;;) {
        
        /* Execute an ISB instruction to ensure that all of the CP15 register changes from the previous steps have been committed */
        isb();
        
        /* Execute a DSB instruction to ensure that all cache, TLB and branch predictor maintenance operations issued by any processor in the multiprocessor device before the SMP bit was cleared have completed */
        dsb();
        
        /*
         * here's the WFI
         */
        __asm__ __volatile__("wfi");
        
        if (pen_release == cpu) {
            /*
             * OK, proper wakeup, we're done
             */
            break;
        }

        /*
         * Getting here, means that we have come out of WFI without
         * having been woken up - this shouldn't happen
         *
         * Just note it happening - when we're woken, we can report
         * its occurrence.
         */
        (*spurious)++;
    }
}





/*
 * platform_cpu_kill:
 * @cpu:
 * Return TBD.
 */
int platform_cpu_kill(unsigned int cpu)
{
    HOTPLUG_INFO("platform_cpu_kill, cpu: %d\n", cpu);

#ifdef CONFIG_HOTPLUG_WITH_POWER_CTRL
    switch(cpu)
    {
        case 1:
            spm_mtcmos_ctrl_cpu1(STA_POWER_DOWN);
            break;
        case 2:
            spm_mtcmos_ctrl_cpu2(STA_POWER_DOWN);
            break;
        case 3:
            spm_mtcmos_ctrl_cpu3(STA_POWER_DOWN);
            break;
        default:
            break;
    }
#endif
    
    atomic_dec(&hotplug_cpu_count);
    
    return 1;
}

/*
 * platform_cpu_die: shutdown a CPU
 * @cpu:
 */
void platform_cpu_die(unsigned int cpu)
{
    int spurious = 0;
    
    HOTPLUG_INFO("platform_cpu_die, cpu: %d\n", cpu);

#ifdef CONFIG_MTK_WD_KICKER
    wk_stop_kick_cpu(cpu);
#endif

    /*
     * we're ready for shutdown now, so do it
     */
    cpu_enter_lowpower(cpu);
    platform_do_lowpower(cpu, &spurious);

    /*
     * bring this CPU back into the world of cache
     * coherency, and then restore interrupts
     */
    cpu_leave_lowpower(cpu);
    
    if (spurious)
        HOTPLUG_INFO("platform_do_lowpower, spurious wakeup call, cpu: %d, spurious: %d\n", cpu, spurious);
}

/*
 * platform_cpu_disable:
 * @cpu:
 * Return error code.
 */
int platform_cpu_disable(unsigned int cpu)
{
    /*
    * we don't allow CPU 0 to be shutdown (it is still too special
    * e.g. clock tick interrupts)
    */
    HOTPLUG_INFO("platform_cpu_disable, cpu: %d\n", cpu);
    return cpu == 0 ? -EPERM : 0;
}
