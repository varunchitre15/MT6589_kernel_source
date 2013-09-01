#include <linux/module.h>
#include <linux/slab.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <linux/utsname.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/mt_sched_mon.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <mach/fiq_smp_call.h>
#include <mach/irqs.h>

#include "aee-common.h"

//#undef WDT_DEBUG_VERBOSE
#define WDT_DEBUG_VERBOSE

#ifdef WDT_DEBUG_VERBOSE
extern int dump_localtimer_info(char* buffer, int size);
extern int dump_idle_info(char *buffer, int size);
#endif

#define THREAD_INFO(sp) ((struct thread_info *) \
				((unsigned long)(sp) & ~(THREAD_SIZE - 1)))

#ifdef CONFIG_SCHED_DEBUG
extern int sysrq_sched_debug_show_at_KE(void);
#endif

enum wk_wdt_type {
	WK_WDT_LOC_TYPE,
	WK_WDT_EXT_TYPE,
	WK_WDT_LOC_TYPE_NOLOCK,
	WK_WDT_EXT_TYPE_NOLOCK,	
};
extern void mtk_wdt_restart(enum wk_wdt_type type);

#ifdef CONFIG_SMP
extern void dump_log_idle(void);
extern void irq_raise_softirq(const struct cpumask *mask, unsigned int irq);
#endif
extern void mt_fiq_printf(const char *fmt, ...);

#define WDT_PERCPU_LOG_SIZE 	1024
#define WDT_LOG_DEFAULT_SIZE 	4096

extern int debug_locks;
/* NR_CPUS may not eaqual to real cpu numbers, alloc buffer at initialization */
static char* wdt_percpu_log_buf[NR_CPUS];
static int wdt_percpu_log_length[NR_CPUS];
static char wdt_log_buf[WDT_LOG_DEFAULT_SIZE];
static int wdt_log_length = 0;
static atomic_t wdt_enter_fiq;

static struct {
	#define WDT_SAVE_STACK_SIZE 256
	char bin_buf[WDT_SAVE_STACK_SIZE];
	int real_len;
	unsigned long top;
	unsigned long bottom;
} stacks_buffer_bin[NR_CPUS];

static struct {
   struct pt_regs regs;
   int real_len;
} regs_buffer_bin[NR_CPUS];


int in_fiq_handler(void)
{
	return atomic_read(&wdt_enter_fiq);
}

void aee_wdt_dump_info(void)
{
	char *printk_buf = wdt_log_buf;
	struct task_struct *task ;
	int i;
	task = &init_task ;

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_KE_WDT_INFO);
	if (wdt_log_length == 0) {
		printk(KERN_ERR "\n No log for WDT \n");
		mt_dump_sched_traces();
		#ifdef CONFIG_SCHED_DEBUG
		sysrq_sched_debug_show_at_KE();
		#endif
		return;
	}

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_KE_WDT_PERCPU);
	printk(KERN_ERR "==========================================");
	for (i=0; i< num_possible_cpus(); i++) {
		if ((wdt_percpu_log_buf[i]) && (wdt_percpu_log_length[i])) {
			//printk(KERN_ERR "=====> wdt_percpu_log_buf[%d], length=%d ", i, wdt_percpu_log_length[i]);
			printk(KERN_ERR "%s", wdt_percpu_log_buf[i]);
			printk(KERN_ERR "==========================================");
		}
	}
	aee_rr_rec_fiq_step(AEE_FIQ_STEP_KE_WDT_LOG);
	// printk temporary buffer printk_buf[1024]. To avoid char loss, add 4 bytes here
	while (wdt_log_length > 0) {
		printk(KERN_ERR "%s", printk_buf);
		printk_buf += 1020;
		wdt_log_length -= 1020;
	}

	#ifdef CONFIG_SCHED_DEBUG
	aee_rr_rec_fiq_step(AEE_FIQ_STEP_KE_SCHED_DEBUG);
	sysrq_sched_debug_show_at_KE();
	#endif

	for_each_process(task)
	{
		if (task->state == 0)
		{
			printk(KERN_ERR "PID: %d, name: %s\n", task->pid, task->comm);
			show_stack(task, NULL);
			printk(KERN_ERR "\n");
		}
	}

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_KE_WDT_DONE);
}

void aee_wdt_percpu_printf(int cpu, const char *fmt, ...)
{
	va_list args;
	
	if (wdt_percpu_log_buf[cpu]==NULL)
		return;

	va_start(args, fmt);
	wdt_percpu_log_length[cpu] += vsnprintf((wdt_percpu_log_buf[cpu] + wdt_percpu_log_length[cpu]), 
						(WDT_PERCPU_LOG_SIZE - wdt_percpu_log_length[cpu]), fmt, args);
	va_end(args);
}

void aee_wdt_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	wdt_log_length += vsnprintf((wdt_log_buf+wdt_log_length), 
						(sizeof(wdt_log_buf) - wdt_log_length), fmt, args);
	va_end(args);
}

/* save registers in bin buffer, may comes from various cpu */
static void aee_dump_cpu_reg_bin(int cpu, void *regs_ptr)
{
	memcpy(&(regs_buffer_bin[cpu].regs), regs_ptr, sizeof(struct pt_regs)); 
	regs_buffer_bin[cpu].real_len = sizeof(struct pt_regs);

	aee_wdt_percpu_printf(cpu, "pc  : %08lx, lr : %08lx, cpsr : %08lx\n",
				((struct pt_regs *)regs_ptr)->ARM_pc,
				((struct pt_regs *)regs_ptr)->ARM_lr,
				((struct pt_regs *)regs_ptr)->ARM_cpsr);
	aee_wdt_percpu_printf(cpu, "sp  : %08lx, ip : %08lx, fp : %08lx\n",
				((struct pt_regs *)regs_ptr)->ARM_sp,
				((struct pt_regs *)regs_ptr)->ARM_ip,
				((struct pt_regs *)regs_ptr)->ARM_fp);
	aee_wdt_percpu_printf(cpu, "r10 : %08lx, r9 : %08lx, r8 : %08lx\n",
				((struct pt_regs *)regs_ptr)->ARM_r10,
				((struct pt_regs *)regs_ptr)->ARM_r9,
				((struct pt_regs *)regs_ptr)->ARM_r8);
	aee_wdt_percpu_printf(cpu, "r7  : %08lx, r6 : %08lx, r5 : %08lx\n",
				((struct pt_regs *)regs_ptr)->ARM_r7,
				((struct pt_regs *)regs_ptr)->ARM_r6,
				((struct pt_regs *)regs_ptr)->ARM_r5);
	aee_wdt_percpu_printf(cpu, "r4  : %08lx, r3 : %08lx, r2 : %08lx\n",
				((struct pt_regs *)regs_ptr)->ARM_r4,
				((struct pt_regs *)regs_ptr)->ARM_r3,
				((struct pt_regs *)regs_ptr)->ARM_r2);
	aee_wdt_percpu_printf(cpu, "r1  : %08lx, r0 : %08lx\n",
				((struct pt_regs *)regs_ptr)->ARM_r1,
				((struct pt_regs *)regs_ptr)->ARM_r0);
	return;
}

/* dump the stack into buffer */
static void aee_wdt_dump_stack_bin(unsigned int cpu, unsigned long bottom, unsigned long top)
{
	int count = 0;
	unsigned long p;

	stacks_buffer_bin[cpu].real_len = 
		aee_dump_stack_top_binary(stacks_buffer_bin[cpu].bin_buf, 
					sizeof(stacks_buffer_bin[cpu].bin_buf), bottom, top);
	stacks_buffer_bin[cpu].top = top;
	stacks_buffer_bin[cpu].bottom = bottom;

	/* should check stack address in kernel range */
	if (bottom & 3) {
		aee_wdt_percpu_printf(cpu, "%s bottom unaligned %08lx \n", __func__, bottom);
		return;
	}
	if (!((bottom >= (PAGE_OFFSET + THREAD_SIZE)) && 
		(bottom <= (PAGE_OFFSET + get_memory_size()))) ){
		aee_wdt_percpu_printf(cpu, "%s bottom out of kernel addr space %08lx \n", __func__, bottom);
		return;
	}
	if (!((top >= (PAGE_OFFSET + THREAD_SIZE)) && 
		  (top <= (PAGE_OFFSET + get_memory_size()))) ){
		aee_wdt_percpu_printf(cpu, "%s top out of kernel addr space %08lx \n", __func__, top);
		return;
	}

	aee_wdt_percpu_printf(cpu, "stack (0x%08lx to 0x%08lx)\n",bottom, top);
	for (p = bottom; p < top; p += 4) {
		unsigned long val;
		if(count == 0)
		{
			aee_wdt_percpu_printf(cpu, "%04lx: ", p & 0xffff);
		}
		val = *((unsigned long *)(p));
		aee_wdt_percpu_printf(cpu, "%08lx ",val);

		count++;
		if(count == 8)
		{
			aee_wdt_percpu_printf(cpu, "\n");
			count = 0;
		}
	}

	return;
}

/* save binary register and stack value into ram console */
static void aee_save_reg_stack_sram(void)
{
	int i;
	char reg_buf[100];
	char stack_buf[100];
	
	for (i = 0; i< num_possible_cpus(); i++) {
		if (regs_buffer_bin[i].real_len != 0) {
			snprintf(reg_buf, sizeof(reg_buf), 
					"\ncpu %d r0->r10 fp ip sp lr pc cpsr orig_r0\n", i);
			aee_sram_fiq_log(reg_buf);
			aee_sram_fiq_save_bin((char *)&(regs_buffer_bin[i].regs), 
								regs_buffer_bin[i].real_len);
		}
		if (stacks_buffer_bin[i].real_len > 0) {
			snprintf(stack_buf, sizeof(stack_buf), "\ncpu %d stack [%08lx %08lx]\n",
					 i, stacks_buffer_bin[i].bottom, stacks_buffer_bin[i].top);
			aee_sram_fiq_log(stack_buf);
			aee_sram_fiq_save_bin(stacks_buffer_bin[i].bin_buf, 
								  stacks_buffer_bin[i].real_len);
		}
	}
}

#ifdef CONFIG_SMP
#ifdef CONFIG_FIQ_GLUE
void aee_fiq_ipi_cpu_stop(void *arg, void *regs, void *svc_sp)
{
	int cpu = 0;
	register int sp asm("sp");
	struct pt_regs *ptregs = (struct pt_regs *)regs;

	asm volatile("mov %0, %1\n\t"
				"mov fp, %2\n\t"
				: "=r" (sp)
				: "r" (svc_sp), "r" (ptregs->ARM_fp)
				);

	asm volatile("MRC p15,0,%0,c0,c0,5\n"
				 "AND %0,%0,#0xf\n"
				 : "+r" (cpu)
				 :
				 : "cc");

	aee_wdt_percpu_printf(cpu, "CPU%u: stopping by FIQ \n", cpu);
	aee_dump_cpu_reg_bin(cpu, regs);
	aee_wdt_dump_stack_bin(cpu, ((struct pt_regs *)regs)->ARM_sp, 
			((struct pt_regs *)regs)->ARM_sp + 256);

	set_cpu_online(cpu, false);
	local_fiq_disable();
	local_irq_disable();

	while (1)
		cpu_relax();
}

void aee_smp_send_stop(void)
{
	unsigned long timeout;
	struct cpumask mask;
	int cpu = 0;

	cpumask_copy(&mask, cpu_online_mask);
	asm volatile("MRC p15,0,%0,c0,c0,5\n"
				 "AND %0,%0,#0xf\n"
				 : "+r" (cpu)
				 :
				 : "cc");
	cpumask_clear_cpu(cpu, &mask);
	mt_fiq_printf("\n fiq_smp_call_function \n");
	fiq_smp_call_function(aee_fiq_ipi_cpu_stop, NULL, 0);

	// Wait up to one second for other CPUs to stop 
	timeout = USEC_PER_SEC;
	while (num_online_cpus() > 1 && timeout--)
		udelay(1);

	if (num_online_cpus() > 1) {
		unsigned int *log, len;
		int i, l;
		extern int get_fiq_isr_log(int cpu, unsigned int *log, unsigned int *len);
		aee_wdt_printf("WDT: failed to stop other CPUs in FIQ \n");
		for (i = 0; i < NR_CPUS; i++) {
			if (!get_fiq_isr_log(i, (unsigned int *)(&log), &len)) {
				aee_wdt_printf("fiq_isr_log_%d: ", i);
				for (l = 0; l < (len / 4); l++) {
					aee_wdt_printf("0x%x,", *(log + l));
				}
				aee_wdt_printf("\n");
			}
		}
	}
}
#else

void aee_smp_send_stop(void)
{
	unsigned long timeout;
	struct cpumask mask;
	int cpu = 0;

	cpumask_copy(&mask, cpu_online_mask);
	asm volatile("MRC p15,0,%0,c0,c0,5\n"
				 "AND %0,%0,#0xf\n"
				 : "+r" (cpu)
				 :
				 : "cc");
	cpumask_clear_cpu(cpu, &mask);
	irq_raise_softirq(&mask, IPI_CPU_STOP);

	// Wait up to one second for other CPUs to stop 
	timeout = USEC_PER_SEC;
	while (num_online_cpus() > 1 && timeout--)
		udelay(1);

	if (num_online_cpus() > 1)
		aee_wdt_printf("WDT: failed to stop other CPUs\n");
}
#endif	// #ifdef CONFIG_FIQ_GLUE
#endif	// #ifdef CONFIG_SMP

void aee_wdt_irq_info(void)
{
	unsigned long long t;
	unsigned long nanosec_rem;

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_IRQ_KICK);
	mtk_wdt_restart(WK_WDT_EXT_TYPE_NOLOCK);

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_IRQ_SMP_STOP);
	#ifdef CONFIG_SMP
	aee_smp_send_stop();
	#endif

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_IRQ_STACK);
	aee_save_reg_stack_sram();

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_IRQ_TIME);
	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000);
	aee_wdt_printf("\nQwdt at [%5lu.%06lu] ", (unsigned long) t, nanosec_rem / 1000);
	aee_wdt_printf(", preempt_count=0x%08lx \n", preempt_count());

	#ifdef WDT_DEBUG_VERBOSE
	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_IRQ_GIC);
	mt_irq_dump();
	
	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_IRQ_LOCALTIMER);
	wdt_log_length += dump_localtimer_info((wdt_log_buf+wdt_log_length), (sizeof(wdt_log_buf) - wdt_log_length));

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_IRQ_IDLE);
	wdt_log_length += dump_idle_info((wdt_log_buf+wdt_log_length), (sizeof(wdt_log_buf) - wdt_log_length));
	#endif

	#ifdef CONFIG_MT_SCHED_MONITOR
	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_IRQ_SCHED);
	mt_aee_dump_sched_traces();
	#endif
	aee_sram_fiq_log(wdt_log_buf);

	/* avoid lock prove to dump_stack in __debug_locks_off() */
	xchg(&debug_locks, 0);
	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_IRQ_DONE);

	BUG();
}

#if defined(CONFIG_FIQ_DEBUGGER)

void aee_wdt_fiq_info(void *arg, void *regs, void *svc_sp)
{
	register int sp asm("sp");
	struct pt_regs *ptregs = (struct pt_regs *)regs;
	int cpu = 0;

	asm volatile("mov %0, %1\n\t"
				"mov fp, %2\n\t"
				: "=r" (sp)
				: "r" (svc_sp), "r" (ptregs->ARM_fp)
				);

	asm volatile("MRC p15,0,%0,c0,c0,5\n"
				 "AND %0,%0,#0xf\n"
				 : "+r" (cpu)
				 :
				 : "cc");

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_FIQ_INFO);
	mt_fiq_printf("\n Triggered :cpu-%d\n", cpu);
	aee_wdt_percpu_printf(cpu, "CPU %d FIQ: Watchdog time out\n", cpu);
	aee_dump_cpu_reg_bin(cpu, regs);

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_FIQ_STACK);
	aee_wdt_dump_stack_bin(cpu, ((struct pt_regs *)regs)->ARM_sp, 
			((struct pt_regs *)regs)->ARM_sp + 256);

	if (atomic_xchg(&wdt_enter_fiq, 1) != 0) {
		aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_FIQ_LOOP);
		aee_wdt_percpu_printf(cpu, "Other CPU already enter WDT FIQ handler \n");
		// loop forever here to avoid SMP deadlock risk during panic flow
		while(1);
	}

	aee_rr_rec_fiq_step(AEE_FIQ_STEP_WDT_FIQ_DONE);
	aee_wdt_irq_info();
}
#endif  /* CONFIG_FIQ_DEBUGGER */

static int __init aee_wdt_init(void)
{
	int i;
	atomic_set(&wdt_enter_fiq, 0);
	for (i=0; i< num_possible_cpus(); i++) {
		wdt_percpu_log_buf[i] = kzalloc(WDT_PERCPU_LOG_SIZE, GFP_KERNEL);
		if (wdt_percpu_log_buf[i]==NULL) {
			printk(KERN_ERR "\n aee_common_init : kmalloc fail \n");
		}
		wdt_percpu_log_length[i] = 0;
	}
	memset(wdt_log_buf, 0, sizeof(wdt_log_buf));
	memset(regs_buffer_bin, 0, sizeof(regs_buffer_bin));
	memset(stacks_buffer_bin, 0, sizeof(stacks_buffer_bin));
	return 0;
}
module_init(aee_wdt_init);
