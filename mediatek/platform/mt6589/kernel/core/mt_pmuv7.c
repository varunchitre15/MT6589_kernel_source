#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include "mach/pmu_v7.h"

static void smp_pmu_stop(void);
static void smp_pmu_start(void);
static void smp_pmu_reset(void);
static void smp_pmu_enable_event(void);
static void smp_pmu_read_counter(void);

static int event_mask = 0x8000003f;

struct arm_pmu armv7pmu = {
        .enable                 = smp_pmu_enable_event,
        .read_counter           = smp_pmu_read_counter,
        .start                  = smp_pmu_start,
        .stop                   = smp_pmu_stop,
        .reset                  = smp_pmu_reset,
        .num_events             = NUMBER_OF_EVENT,
        .id                     = ARM_PMU_ID_CA9,
        .name                   = "ARMv7 Cortex-A9",
        .perf_cfg               = {	.event_cfg  = {	ARMV7_IFETCH_MISS, ARMV7_INST_OUT_OF_RENAME_STAGE, 
                                                    ARMV7_DCACHE_REFILL, ARMV7_DCACHE_ACCESS,
                                                    ARMV7_FP_EXECUTED_INST, ARMV7_NEON_EXECUTED_INST
                                                   },
				                  },
        .perf_data              = {	.cnt_val 	= {{0,0,0,0,0,0,0}, {0,0,0,0,0,0,0}},
					                .overflow	= {0, 0},
				                  },
};

static inline unsigned long armv7_pmnc_read(void)
{
        u32 val;
        asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
        return val;
}

static inline void armv7_pmnc_write(unsigned long val)
{
        val &= ARMV7_PMNC_MASK;
        isb();
        asm volatile("mcr p15, 0, %0, c9, c12, 0" : : "r"(val));
}

static inline int armv7_pmnc_has_overflowed(unsigned long pmnc)
{
        return pmnc & ARMV7_OVERFLOWED_MASK;
}

static inline int armv7_pmnc_counter_has_overflowed(unsigned long pmnc,
                                        int counter)
{
        int ret = 0;

        if (counter == ARMV7_CYCLE_COUNTER)
                ret = pmnc & ARMV7_FLAG_C;
        else if ((counter >= ARMV7_COUNTER0) && (counter <= ARMV7_COUNTER_LAST))
                ret = pmnc & ARMV7_FLAG_P(counter);
        else
                pr_err("CPU%u checking wrong counter %d overflow status\n",
                        raw_smp_processor_id(), counter);

        return ret;
}

static inline int armv7_pmnc_select_counter(unsigned int idx)
{
        u32 val;

        if ((idx < ARMV7_COUNTER0) || (idx > ARMV7_COUNTER_LAST)) {
                pr_err("CPU%u selecting wrong PMNC counter"
                        " %d\n", raw_smp_processor_id(), idx);
                return -1;
        }

        val = (idx - ARMV7_EVENT_CNT_TO_CNTx) & ARMV7_SELECT_MASK;
        asm volatile("mcr p15, 0, %0, c9, c12, 5" : : "r" (val));
        isb();

        return idx;
}

static inline u32 armv7pmu_read_counter(int idx)
{
        unsigned long value = 0;

        if (idx == ARMV7_CYCLE_COUNTER)
                asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (value));
        else if ((idx >= ARMV7_COUNTER0) && (idx <= ARMV7_COUNTER_LAST)) {
                if (armv7_pmnc_select_counter(idx) == idx)
                        asm volatile("mrc p15, 0, %0, c9, c13, 2"
                                     : "=r" (value));
        } else
                pr_err("CPU%u reading wrong counter %d\n",
                        raw_smp_processor_id(), idx);

        return value;
}

static inline void armv7pmu_write_counter(int idx, u32 value)
{
        if (idx == ARMV7_CYCLE_COUNTER)
                asm volatile("mcr p15, 0, %0, c9, c13, 0" : : "r" (value));
        else if ((idx >= ARMV7_COUNTER0) && (idx <= ARMV7_COUNTER_LAST)) {
                if (armv7_pmnc_select_counter(idx) == idx)
                        asm volatile("mcr p15, 0, %0, c9, c13, 2"
                                     : : "r" (value));
        } else
                pr_err("CPU%u writing wrong counter %d\n",
                        raw_smp_processor_id(), idx);
}

static inline void armv7_pmnc_write_evtsel(unsigned int idx, u32 val)
{
        if (armv7_pmnc_select_counter(idx) == idx) {
                val &= ARMV7_EVTSEL_MASK;
                asm volatile("mcr p15, 0, %0, c9, c13, 1" : : "r" (val));
        }
}

static inline u32 armv7_pmnc_enable_counter(unsigned int idx)
{
        u32 val;

        if ((idx != ARMV7_CYCLE_COUNTER) &&
            ((idx < ARMV7_COUNTER0) || (idx > ARMV7_COUNTER_LAST))) {
                pr_err("CPU%u enabling wrong PMNC counter"
                        " %d\n", raw_smp_processor_id(), idx);
                return -1;
        }

        if (idx == ARMV7_CYCLE_COUNTER)
                val = ARMV7_CNTENS_C;
        else
                val = ARMV7_CNTENS_P(idx);

        asm volatile("mcr p15, 0, %0, c9, c12, 1" : : "r" (val));

        return idx;
}

static inline u32 armv7_pmnc_disable_counter(unsigned int idx)
{
        u32 val;


        if ((idx != ARMV7_CYCLE_COUNTER) &&
            ((idx < ARMV7_COUNTER0) || (idx > ARMV7_COUNTER_LAST))) {
                pr_err("CPU%u disabling wrong PMNC counter"
                        " %d\n", raw_smp_processor_id(), idx);
                return -1;
        }

        if (idx == ARMV7_CYCLE_COUNTER)
                val = ARMV7_CNTENC_C;
        else
                val = ARMV7_CNTENC_P(idx);

        asm volatile("mcr p15, 0, %0, c9, c12, 2" : : "r" (val));

        return idx;
}

#if 0
static inline u32 armv7_pmnc_enable_interrupt(unsigned int idx)
{
        u32 val;

        if ((idx != ARMV7_CYCLE_COUNTER) &&
            ((idx < ARMV7_COUNTER0) || (idx > ARMV7_COUNTER_LAST))) {
                pr_err("CPU%u enabling wrong PMNC counter"
                        " interrupt enable %d\n", raw_smp_processor_id(), idx);
                return -1;
        }

        if (idx == ARMV7_CYCLE_COUNTER)
                val = ARMV7_INTENS_C;
        else
                val = ARMV7_INTENS_P(idx);

        asm volatile("mcr p15, 0, %0, c9, c14, 1" : : "r" (val));

        return idx;
}

static inline u32 armv7_pmnc_disable_interrupt(unsigned int idx)
{
        u32 val;

        if ((idx != ARMV7_CYCLE_COUNTER) &&
            ((idx < ARMV7_COUNTER0) || (idx > ARMV7_COUNTER_LAST))) {
                pr_err("CPU%u disabling wrong PMNC counter"
                        " interrupt enable %d\n", raw_smp_processor_id(), idx);
                return -1;
        }

        if (idx == ARMV7_CYCLE_COUNTER)
                val = ARMV7_INTENC_C;
        else
                val = ARMV7_INTENC_P(idx);

        asm volatile("mcr p15, 0, %0, c9, c14, 2" : : "r" (val));

        return idx;
}
#endif

static u32 armv7_pmnc_get_overflow_status(void)
{
        u32 val;
        /* Read */
        asm volatile("mrc p15, 0, %0, c9, c12, 3" : "=r" (val));

        /* Write to clear flags */
        val &= ARMV7_FLAG_MASK;
        asm volatile("mcr p15, 0, %0, c9, c12, 3" : : "r" (val));
		
		return val;        
}

#ifdef DEBUG
static void armv7_pmnc_dump_regs(void)
{
        u32 val;
        unsigned int cnt;

        printk(KERN_INFO "PMNC registers dump:\n");

        asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r" (val));
        printk(KERN_INFO "PMNC  =0x%08x\n", val);

        asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r" (val));
        printk(KERN_INFO "CNTENS=0x%08x\n", val);

        asm volatile("mrc p15, 0, %0, c9, c14, 1" : "=r" (val));
        printk(KERN_INFO "INTENS=0x%08x\n", val);

        asm volatile("mrc p15, 0, %0, c9, c12, 3" : "=r" (val));
        printk(KERN_INFO "FLAGS =0x%08x\n", val);

        asm volatile("mrc p15, 0, %0, c9, c12, 5" : "=r" (val));
        printk(KERN_INFO "SELECT=0x%08x\n", val);

        asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (val));
        printk(KERN_INFO "CCNT  =0x%08x\n", val);

        for (cnt = ARMV7_COUNTER0; cnt < ARMV7_COUNTER_LAST; cnt++) {
                armv7_pmnc_select_counter(cnt);
                asm volatile("mrc p15, 0, %0, c9, c13, 2" : "=r" (val));
                printk(KERN_INFO "CNT[%d] count =0x%08x\n",
                        cnt-ARMV7_EVENT_CNT_TO_CNTx, val);
                asm volatile("mrc p15, 0, %0, c9, c13, 1" : "=r" (val));
                printk(KERN_INFO "CNT[%d] evtsel=0x%08x\n",
                        cnt-ARMV7_EVENT_CNT_TO_CNTx, val);
        }
}
#endif

static void armv7pmu_enable_event(u32 config , int idx)
{
       
       /*
        * Enable counter and interrupt, and set the counter to count
        * the event that we're interested in.
        */
 

       /*
        * Disable counter
         */
        armv7_pmnc_disable_counter(idx);

        /*
         * Set event (if destined for PMNx counters)
         * We don't need to set the event if it's a cycle count
         */
        if (idx != ARMV7_CYCLE_COUNTER)
                armv7_pmnc_write_evtsel(idx, config);

        /*
         * Enable interrupt for this counter
         */
        //armv7_pmnc_enable_interrupt(idx);

        /*
         * Enable counter
         */
        armv7_pmnc_enable_counter(idx);


}
            
static void armv7pmu_disable_event(u32 config, int idx)
{

        /*
         * Disable counter and interrupt
         */


        /*
         * Disable counter
         */
        armv7_pmnc_disable_counter(idx);

        /*
         * Disable interrupt for this counter
         */
        //armv7_pmnc_disable_interrupt(idx);


}

static void armv7pmu_start(void *info)
{
        /* Enable all counters */
        armv7_pmnc_write(armv7_pmnc_read() | ARMV7_PMNC_E);
}

static void armv7pmu_stop(void *info)
{
        /* Disable all counters */
        armv7_pmnc_write(armv7_pmnc_read() & ~ARMV7_PMNC_E);
}

static void armv7pmu_reset(void *info)
{
        
        /* The counter and interrupt enable registers are unknown at reset. */
        //for (idx = 0; idx < NUMBER_OF_EVENT; ++idx)
        //        armv7pmu_disable_event(NULL, idx);
		
		//armv7_pmnc_disable_counter(ARMV7_CYCLE_COUNTER);
		
        /* Initialize & Reset PMNC: C and P bits */
        armv7_pmnc_write(ARMV7_PMNC_P | ARMV7_PMNC_C);
}

static void armv7pmu_enable(void *info)
{
	int idx;
	struct  pmu_cfg *p_cfg= (struct  pmu_cfg *) &armv7pmu.perf_cfg;
	
	armv7pmu_reset(NULL);
	
	if(event_mask >> 31)
		armv7_pmnc_enable_counter(ARMV7_CYCLE_COUNTER);
	else
		armv7_pmnc_disable_counter(ARMV7_CYCLE_COUNTER);
	
	for (idx = 0; idx < NUMBER_OF_EVENT; idx++) {
		if( (event_mask >> idx) & EVENT_MASK )
			armv7pmu_enable_event(p_cfg->event_cfg[idx], idx);
		else
			armv7pmu_disable_event(0, idx);
	}  	
}

static void armv7pmu_read_all_counter(void *info)
{
	int idx, cpu = raw_smp_processor_id();
	struct  pmu_data *p_data = (struct  pmu_data *) &armv7pmu.perf_data;
	
	for (idx = 0; idx < NUMBER_OF_EVENT + 1; idx++) {
    	p_data->cnt_val[cpu][idx] = armv7pmu_read_counter(idx);
   	}
   	
   	p_data->overflow[cpu] = armv7_pmnc_get_overflow_status();
}

static void smp_pmu_stop(void)
{
#ifdef CONFIG_SMP
	int i; 
	if(armv7pmu.multicore)
	{
		for(i = 0; i < NUMBER_OF_CPU; i++)
			mtk_smp_call_function_single(i, armv7pmu_stop, NULL, 1);
	} else
#endif
		armv7pmu_stop(NULL);
}

static void smp_pmu_start(void)
{
#ifdef CONFIG_SMP 
	int i;
	if(armv7pmu.multicore)
	{
		for(i = 0; i < NUMBER_OF_CPU; i++)
			mtk_smp_call_function_single(i, armv7pmu_start, NULL, 1);
	} else
#endif
		armv7pmu_start(NULL);
}

static void smp_pmu_reset(void)
{
#ifdef CONFIG_SMP 
        int i;
	if(armv7pmu.multicore)
	{
		for(i = 0; i < NUMBER_OF_CPU; i++)
			mtk_smp_call_function_single(i, armv7pmu_reset, NULL, 1);
	} else
#endif
		armv7pmu_reset(NULL);
}

static void smp_pmu_enable_event(void)
{
#ifdef CONFIG_SMP 	
        int i;
	if(armv7pmu.multicore)
	{
		for(i = 0; i < NUMBER_OF_CPU; i++)
			mtk_smp_call_function_single(i, armv7pmu_enable, NULL, 1);
	} else
#endif
		armv7pmu_enable(NULL);
}

/*static void smp_pmu_disable_event(void)
{
	
}*/

static void smp_pmu_read_counter(void)
{
#ifdef CONFIG_SMP 	
        int i;
	if(armv7pmu.multicore)
	{
		for(i = 0; i < NUMBER_OF_CPU; i++)
			mtk_smp_call_function_single(i, armv7pmu_read_all_counter, NULL, 1);
	} else
#endif
		armv7pmu_read_all_counter(NULL);
}

int register_pmu(struct arm_pmu **p_pmu)
{

	*p_pmu = &armv7pmu;	
	return 0;
}   


void unregister_pmu(struct arm_pmu **p_pmu)
{
	*p_pmu = NULL;
}

  
         
