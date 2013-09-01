#ifndef __MT_SMP_H
#define __MT_SMP_H

#include <linux/cpumask.h>
#include <mach/irqs.h>
extern void irq_raise_softirq(const struct cpumask *mask, unsigned int irq);

/* use Soft IRQ1 as the IPI */
static inline void smp_cross_call(const struct cpumask *mask)
{
    irq_raise_softirq(mask, CPU_BRINGUP_SGI);
}

#endif  /* !__MT_SMP_H */
