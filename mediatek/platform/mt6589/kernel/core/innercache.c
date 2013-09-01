#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpu.h>
#include <linux/smp.h>

/*
 * inner_dcache_flush_all: Flush (clean + invalidate) the entire L1 data cache.
 *
 * This can be used ONLY by the M4U driver!!
 * Other drivers should NOT use this function at all!!
 * Others should use DMA-mapping APIs!!
 *
 * After calling the function, the buffer should not be touched anymore.
 * And the M4U driver should then call outer_flush_all() immediately.
 * Here is the example:
 *     // Cannot touch the buffer from here.
 *     inner_dcache_flush_all();
 *     outer_flush_all();
 *     // Can touch the buffer from here.
 * If preemption occurs and the driver cannot guarantee that no other process will touch the buffer,
 * the driver should use LOCK to protect this code segment.
 */
extern void __inner_flush_dcache_all(void);
extern void __inner_flush_dcache_L1(void);
extern void __inner_flush_dcache_L2(void);

void inner_dcache_flush_all()
{
    __inner_flush_dcache_all();
}
void inner_dcache_flush_L1()
{
    __inner_flush_dcache_L1();
}
void inner_dcache_flush_L2()
{
    __inner_flush_dcache_L2();
}
/*
 * smp_inner_dcache_flush_all: Flush (clean + invalidate) the entire L1 data cache.
 *
 * This can be used ONLY by the M4U driver!!
 * Other drivers should NOT use this function at all!!
 * Others should use DMA-mapping APIs!!
 *
 * This is the smp version of inner_dcache_flush_all().
 * It will use IPI to do flush on all CPUs.
 * Must not call this function with disabled interrupts or from a
 * hardware interrupt handler or from a bottom half handler.
 */
void smp_inner_dcache_flush_all(void)
{
    if (in_interrupt()) {
        printk(KERN_ERR "Cannot invoke smp_inner_dcache_flush_all() in interrupt/softirq context\n");
        return ;
    }
    get_online_cpus();

    on_each_cpu(inner_dcache_flush_L1, NULL, true);
    inner_dcache_flush_L2();

    put_online_cpus();
    
}

EXPORT_SYMBOL(inner_dcache_flush_all);
EXPORT_SYMBOL(smp_inner_dcache_flush_all);
