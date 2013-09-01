/*
 * trace context switch
 *
 * Copyright (C) 2007 Steven Rostedt <srostedt@redhat.com>
 *
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/kallsyms.h>
#include <linux/uaccess.h>
#include <linux/ftrace.h>
#include <trace/events/sched.h>

#include "trace.h"

#ifdef CONFIG_MTK_SCHED_TRACERS
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <linux/interrupt.h>
int get_irq_pid(int irq);
void tracing_record_isr(int irq, const char* name);
void save_isr_name(void);
#endif
static struct trace_array	*ctx_trace;
static int __read_mostly	tracer_enabled;
static int			sched_ref;
static DEFINE_MUTEX(sched_register_mutex);
int			sched_stopped = 1;


void
tracing_sched_switch_trace(struct trace_array *tr,
			   struct task_struct *prev,
			   struct task_struct *next,
			   unsigned long flags, int pc)
{
	struct ftrace_event_call *call = &event_context_switch;
	struct ring_buffer *buffer = tr->buffer;
	struct ring_buffer_event *event;
	struct ctx_switch_entry *entry;

	event = trace_buffer_lock_reserve(buffer, TRACE_CTX,
					  sizeof(*entry), flags, pc);
	if (!event)
		return;
	entry	= ring_buffer_event_data(event);
	entry->prev_pid			= prev->pid;
	entry->prev_prio		= prev->prio;
	entry->prev_state		= prev->state;
	entry->next_pid			= next->pid;
	entry->next_prio		= next->prio;
	entry->next_state		= next->state;
	entry->next_cpu	= task_cpu(next);

        if(entry->prev_state == TASK_UNINTERRUPTIBLE && prev->in_iowait == 1)
        {
            entry -> prev_state = MTK_TASK_IO_WAIT;
        }

	if (!filter_check_discard(call, entry, buffer, event))
		trace_buffer_unlock_commit(buffer, event, flags, pc);
}

static void
probe_sched_switch(void *ignore, struct task_struct *prev, struct task_struct *next)
{
	struct trace_array_cpu *data;
	unsigned long flags;
	int cpu;
	int pc;

	if (unlikely(!sched_ref))
		return;

	tracing_record_cmdline(prev);
	tracing_record_cmdline(next);

	if (!tracer_enabled || sched_stopped)
		return;

	pc = preempt_count();
	local_irq_save(flags);
	cpu = raw_smp_processor_id();
	data = ctx_trace->data[cpu];

	if (likely(!atomic_read(&data->disabled)))
		tracing_sched_switch_trace(ctx_trace, prev, next, flags, pc);

	local_irq_restore(flags);
}

void
tracing_sched_wakeup_trace(struct trace_array *tr,
			   struct task_struct *wakee,
			   struct task_struct *curr,
			   unsigned long flags, int pc)
{
	struct ftrace_event_call *call = &event_wakeup;
	struct ring_buffer_event *event;
	struct ctx_switch_entry *entry;
	struct ring_buffer *buffer = tr->buffer;

	event = trace_buffer_lock_reserve(buffer, TRACE_WAKE,
					  sizeof(*entry), flags, pc);
	if (!event)
		return;
	entry	= ring_buffer_event_data(event);
	entry->prev_pid			= curr->pid;
	entry->prev_prio		= curr->prio;
	entry->prev_state		= curr->state;
	entry->next_pid			= wakee->pid;
	entry->next_prio		= wakee->prio;
	entry->next_state		= wakee->state;
	entry->next_cpu			= task_cpu(wakee);

        if(entry->next_state == TASK_UNINTERRUPTIBLE && wakee->in_iowait == 1)
        {
            entry->next_state= MTK_TASK_IO_WAIT;
        }

	if (!filter_check_discard(call, entry, buffer, event))
		ring_buffer_unlock_commit(buffer, event);
	ftrace_trace_stack(tr->buffer, flags, 6, pc);
	ftrace_trace_userstack(tr->buffer, flags, pc);
}

static void
probe_sched_wakeup(void *ignore, struct task_struct *wakee, int success)
{
	struct trace_array_cpu *data;
	unsigned long flags;
	int cpu, pc;

	if (unlikely(!sched_ref))
		return;

	tracing_record_cmdline(current);

	if (!tracer_enabled || sched_stopped)
		return;

	pc = preempt_count();
	local_irq_save(flags);
	cpu = raw_smp_processor_id();
	data = ctx_trace->data[cpu];

	if (likely(!atomic_read(&data->disabled)))
		tracing_sched_wakeup_trace(ctx_trace, wakee, current,
					   flags, pc);

	local_irq_restore(flags);
}
#ifdef CONFIG_MTK_SCHED_TRACERS
void
tracing_int_switch(struct trace_array *tr,
			   struct task_struct *curr,int irq, int isr_enter,
			   unsigned long flags, int pc)
{
	struct ftrace_event_call *call = &event_context_switch;
	struct ring_buffer *buffer = tr->buffer;
	struct ring_buffer_event *event;
	struct ctx_switch_entry *entry;
	int irq_pid;

	event = trace_buffer_lock_reserve(buffer, TRACE_CTX,
					  sizeof(*entry), flags, pc);
	if (!event)
		return;
	
	entry	= ring_buffer_event_data(event);
	irq_pid = get_irq_pid(irq);
	((struct trace_entry*)entry)->pid = (isr_enter ==1)?curr->pid:irq_pid;
	entry->prev_pid			= (isr_enter ==1)?curr->pid:irq_pid;
	entry->prev_prio		= (isr_enter ==1)?curr->prio:120;
	entry->prev_state		= (isr_enter ==1)?curr->state:0;
	entry->next_pid			= (isr_enter ==1)?irq_pid:curr->pid;
	entry->next_prio		= (isr_enter ==1)?120:curr->prio;
	entry->next_state		= (isr_enter ==1)?0:curr->state;
	entry->next_cpu	= task_cpu(curr);

	if (!filter_check_discard(call, entry, buffer, event))
		trace_buffer_unlock_commit(buffer, event, flags, pc);
}

static void
probe_int_switch(void *ignore, struct task_struct *curr,
			int irq, int isr_enter)
{
	struct trace_array_cpu *data;
	unsigned long flags;
	int cpu;
	int pc;
	if (unlikely(!sched_ref))
		return;

	tracing_record_cmdline(curr);
	if (!tracer_enabled || sched_stopped)
		return;

	pc = preempt_count();
	local_irq_save(flags);
	cpu = raw_smp_processor_id();
	data = ctx_trace->data[cpu];

	if (likely(!atomic_read(&data->disabled)))
		tracing_int_switch(ctx_trace, curr, irq,isr_enter, flags, pc);

	local_irq_restore(flags);
}

void
tracing_int_nest(struct trace_array *tr,
			 int prev_irq, int next_irq, 
			   unsigned long flags, int pc)
{
	struct ftrace_event_call *call = &event_context_switch;
	struct ring_buffer *buffer = tr->buffer;
	struct ring_buffer_event *event;
	struct ctx_switch_entry *entry;
	int prev_irq_pid, next_irq_pid;


	event = trace_buffer_lock_reserve(buffer, TRACE_CTX,
					  sizeof(*entry), flags, pc);
	if (!event)
		return;
	
	entry	= ring_buffer_event_data(event);
	prev_irq_pid = get_irq_pid(prev_irq);
	next_irq_pid = get_irq_pid(next_irq);
	((struct trace_entry*)entry)->pid = prev_irq_pid;
	entry->prev_pid			= prev_irq_pid;
	entry->prev_prio		= 120;
	entry->prev_state		= 0;
	entry->next_pid			= next_irq_pid;
	entry->next_prio		= 120;
	entry->next_state		= 0;
	entry->next_cpu	= 0;

	if (!filter_check_discard(call, entry, buffer, event))
		trace_buffer_unlock_commit(buffer, event, flags, pc);
}

static void
probe_int_nest(void *ignore, int prev_irq,
			int next_irq)
{
	struct trace_array_cpu *data;
	unsigned long flags;
	int cpu;
	int pc;
	if (unlikely(!sched_ref))
		return;

	if (!tracer_enabled || sched_stopped)
		return;

	pc = preempt_count();
	local_irq_save(flags);
	cpu = raw_smp_processor_id();
	data = ctx_trace->data[cpu];

	if (likely(!atomic_read(&data->disabled)))
		tracing_int_nest(ctx_trace, prev_irq, next_irq, flags, pc);

	local_irq_restore(flags);
}

#endif
static int tracing_sched_register(void)
{
	int ret;

	ret = register_trace_sched_wakeup(probe_sched_wakeup, NULL);
	if (ret) {
		pr_info("wakeup trace: Couldn't activate tracepoint"
			" probe to kernel_sched_wakeup\n");
		return ret;
	}

	ret = register_trace_sched_wakeup_new(probe_sched_wakeup, NULL);
	if (ret) {
		pr_info("wakeup trace: Couldn't activate tracepoint"
			" probe to kernel_sched_wakeup_new\n");
		goto fail_deprobe;
	}

	ret = register_trace_sched_switch(probe_sched_switch, NULL);
	if (ret) {
		pr_info("sched trace: Couldn't activate tracepoint"
			" probe to kernel_sched_switch\n");
		goto fail_deprobe_wake_new;
	}
#ifdef CONFIG_MTK_SCHED_TRACERS
	ret = register_trace_int_switch(probe_int_switch, NULL);
	if (ret) {
		pr_info("sched trace: Couldn't activate tracepoint"
			" probe to kernel_int_switch\n");
		goto fail_deprobe_trace_int_switch;
	}
	ret = register_trace_int_nest(probe_int_nest, NULL);
	if (ret) {
		pr_info("sched trace: Couldn't activate tracepoint"
			" probe to kernel_int_nest\n");
		goto fail_deprobe_trace_int_nest;
	}	
#endif

	return ret;
#ifdef CONFIG_MTK_SCHED_TRACERS
fail_deprobe_trace_int_nest:
	unregister_trace_int_nest(probe_int_nest, NULL);
fail_deprobe_trace_int_switch:
	unregister_trace_int_switch(probe_int_switch, NULL);
#endif
fail_deprobe_wake_new:
	unregister_trace_sched_wakeup_new(probe_sched_wakeup, NULL);
fail_deprobe:
	unregister_trace_sched_wakeup(probe_sched_wakeup, NULL);
	return ret;
}

static void tracing_sched_unregister(void)
{
	unregister_trace_sched_switch(probe_sched_switch, NULL);
	unregister_trace_sched_wakeup_new(probe_sched_wakeup, NULL);
	unregister_trace_sched_wakeup(probe_sched_wakeup, NULL);
#ifdef CONFIG_MTK_SCHED_TRACERS
	unregister_trace_int_switch(probe_int_switch, NULL);
	unregister_trace_int_nest(probe_int_nest, NULL);
#endif
}

static void tracing_start_sched_switch(void)
{
	mutex_lock(&sched_register_mutex);
	if (!(sched_ref++))
		tracing_sched_register();
	mutex_unlock(&sched_register_mutex);
}

static void tracing_stop_sched_switch(void)
{
	mutex_lock(&sched_register_mutex);
	if (!(--sched_ref))
		tracing_sched_unregister();
	mutex_unlock(&sched_register_mutex);
}

void tracing_start_cmdline_record(void)
{
	tracing_start_sched_switch();
}

void tracing_stop_cmdline_record(void)
{
	tracing_stop_sched_switch();
}

/**
 * tracing_start_sched_switch_record - start tracing context switches
 *
 * Turns on context switch tracing for a tracer.
 */
void tracing_start_sched_switch_record(void)
{
	if (unlikely(!ctx_trace)) {
		WARN_ON(1);
		return;
	}

	tracing_start_sched_switch();

	mutex_lock(&sched_register_mutex);
	tracer_enabled++;
	mutex_unlock(&sched_register_mutex);
}

/**
 * tracing_stop_sched_switch_record - start tracing context switches
 *
 * Turns off context switch tracing for a tracer.
 */
void tracing_stop_sched_switch_record(void)
{
	mutex_lock(&sched_register_mutex);
	tracer_enabled--;
	WARN_ON(tracer_enabled < 0);
	mutex_unlock(&sched_register_mutex);

	tracing_stop_sched_switch();
}

/**
 * tracing_sched_switch_assign_trace - assign a trace array for ctx switch
 * @tr: trace array pointer to assign
 *
 * Some tracers might want to record the context switches in their
 * trace. This function lets those tracers assign the trace array
 * to use.
 */
void tracing_sched_switch_assign_trace(struct trace_array *tr)
{
	ctx_trace = tr;
}
static void stop_sched_trace(struct trace_array *tr)
{
    tracing_stop_sched_switch_record();
}

static int sched_switch_trace_init(struct trace_array *tr)
{
    ctx_trace = tr;
    tracing_reset_online_cpus(tr);
    tracing_start_sched_switch_record();
#ifdef CONFIG_MTK_SCHED_TRACERS
    save_isr_name();
#endif	
    return 0;
}

static void sched_switch_trace_reset(struct trace_array *tr)
{
    if (sched_ref)
	stop_sched_trace(tr);
}

static void sched_switch_trace_start(struct trace_array *tr)
{
    sched_stopped = 0;
}

static void sched_switch_trace_stop(struct trace_array *tr)
{
    sched_stopped = 1;
}

static struct tracer sched_switch_trace __read_mostly =
{
    .name	= "sched_switch",
    .init	= sched_switch_trace_init,
    .reset	= sched_switch_trace_reset,
    .start	= sched_switch_trace_start,
    .stop	= sched_switch_trace_stop,
    .wait_pipe	= poll_wait_pipe,
#ifdef CONFIG_FTRACE_SELFTEST
    .selftest    = trace_selftest_startup_sched_switch,
#endif
};

#ifdef CONFIG_MTK_SCHED_TRACERS
void save_isr_name()
{
    int i;
    struct irqaction * action;
    unsigned long flags;

    for (i = 0; i< NR_IRQS; i++) {
        raw_spin_lock_irqsave(&irq_desc[i].lock, flags);
        action = irq_desc[i].action;
        if (!action){
            raw_spin_unlock_irqrestore(&irq_desc[i].lock, flags);
            continue;
        }
        tracing_record_isr(i, action->name);
        printk(KERN_DEBUG "IRQ[%d] Handler:%s\n",i,action->name);
        raw_spin_unlock_irqrestore(&irq_desc[i].lock, flags);
    }
#ifdef CONFIG_SMP
	tracing_record_isr(2,"IPI_TIMER");
	tracing_record_isr(3,"IPI_RESCHEDULE");
	tracing_record_isr(4,"IPI_CALL_FUNC");
	tracing_record_isr(5,"IPI_CALL_FUNC_SINGLE");
	tracing_record_isr(6,"IPI_CPU_STOP");
	tracing_record_isr(7,"IPI_CPU_BACKTRACE");
	tracing_record_isr(29,"LOCAL_TIMER");
	tracing_record_isr(30,"LOCAL_WDT");
#endif
}
#endif
__init static int init_sched_switch_trace(void)
{
    return register_tracer(&sched_switch_trace);
}
device_initcall(init_sched_switch_trace);

