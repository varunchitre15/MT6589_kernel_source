#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/printk.h>
#include <linux/ptrace.h>
#include <linux/ratelimit.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/stacktrace.h>
#include <linux/linkage.h>
#include <linux/atomic.h>
#include <linux/module.h>
#include <asm/stacktrace.h>
#include <asm/traps.h>

#include "../common/aee-common.h"
#include "aed.h"

struct bt_sync {
	atomic_t cpus_report;
	atomic_t cpus_lock;
};

static void per_cpu_get_bt(void *info)
{
	struct bt_sync *s = (struct bt_sync *)info;
	atomic_dec(&s->cpus_report);
	while (atomic_read(&s->cpus_lock) == 1);
	atomic_dec(&s->cpus_report);
}

int notrace aed_unwind_frame(struct stackframe *frame, unsigned long stack_address)
{
	unsigned long high, low;
	unsigned long fp = frame->fp;

	/* only go to a higher address on the stack */
	low = frame->sp;
	high = ALIGN(low, THREAD_SIZE);
	if (high != stack_address) {
		printk("%s: sp base(%lx) not equal to process stack base(%lx)\n", __func__, low, stack_address);
		return -EINVAL;
	}
	/* check current frame pointer is within bounds */
	if ((fp < (low + 12)) || ((fp + 4) >= high))
		return -EINVAL;

	/* restore the registers from the stack frame */
	frame->fp = *(unsigned long *)(fp - 12);
	frame->sp = *(unsigned long *)(fp - 8);
	frame->lr = *(unsigned long *)(fp - 4);
	frame->pc = *(unsigned long *)(fp);

	return 0;
}

#define FUNCTION_OFFSET 12
#define MAX_EXCEPTION_FRAME 32
asmlinkage void __sched preempt_schedule_irq(void);

static int aed_walk_stackframe(struct stackframe *frame, struct aee_process_bt *bt, unsigned int stack_address)
{
	int count;
	struct stackframe current_stk;

	memcpy(&current_stk, frame, sizeof(struct stackframe));
	for (count = 0; count < MAX_EXCEPTION_FRAME; count++) {
		unsigned long prev_fp = current_stk.fp;
		int ret;

		bt->entries[bt->nr_entries].pc = current_stk.pc;
		bt->entries[bt->nr_entries].lr = current_stk.lr;
		snprintf(bt->entries[bt->nr_entries].pc_symbol, MAX_AEE_KERNEL_SYMBOL, "%pS", (void *)current_stk.pc);
		snprintf(bt->entries[bt->nr_entries].lr_symbol, MAX_AEE_KERNEL_SYMBOL, "%pS", (void *)current_stk.lr);

		bt->nr_entries++;
		if (bt->nr_entries >= MAX_AEE_KERNEL_BT) {
			break;
		}

		ret = aed_unwind_frame(&current_stk, stack_address);
		/* oops, reached end without exception. return original info */
		if (ret < 0)
			break;

		if (in_exception_text(current_stk.pc) || ((current_stk.pc - FUNCTION_OFFSET) == (unsigned long)preempt_schedule_irq)) {
			struct pt_regs *regs = (struct pt_regs *)(prev_fp + 4);
			
			/* passed exception point, return this if unwinding is sucessful */
			current_stk.pc = regs->ARM_pc;
			current_stk.lr = regs->ARM_lr;
			current_stk.fp = regs->ARM_fp;
			current_stk.sp = regs->ARM_sp;
		}

	}

	if (bt->nr_entries < MAX_AEE_KERNEL_BT) {
		bt->entries[bt->nr_entries].pc = ULONG_MAX;	
		bt->entries[bt->nr_entries++].lr = ULONG_MAX;
	}
	return 0;
}

static void aed_get_bt(struct task_struct *tsk, struct aee_process_bt *bt)
{
	struct stackframe frame;
	int i;
	unsigned int stack_address;

	bt->nr_entries = 0;
	for (i = 0; i < MAX_AEE_KERNEL_BT; i++) {
		bt->entries[i].pc = 0;
		bt->entries[i].lr = 0;
		memset(bt->entries[i].pc_symbol, 0, KSYM_SYMBOL_LEN);
		memset(bt->entries[i].lr_symbol, 0, KSYM_SYMBOL_LEN);
	}

	memset(&frame, 0, sizeof(struct stackframe));
	if (tsk != current) {
		frame.fp = thread_saved_fp(tsk);
		frame.sp = thread_saved_sp(tsk);
		frame.lr = thread_saved_pc(tsk);
		frame.pc = 0xffffffff;
	} else {
		register unsigned long current_sp asm ("sp");

		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.sp = current_sp;
		frame.lr = (unsigned long)__builtin_return_address(0);
		frame.pc = (unsigned long)aed_get_bt;
	}
	stack_address = ALIGN(frame.sp, THREAD_SIZE);
	if ((stack_address >= (PAGE_OFFSET + THREAD_SIZE)) && (stack_address <= (PAGE_OFFSET + get_memory_size())))  {
		aed_walk_stackframe(&frame, bt, stack_address);
	}
	else {
		printk("%s: Invalid sp value %lx\n", __func__, frame.sp);
	}
}

int aed_get_process_bt(struct aee_process_bt *bt)
{
	int nr_cpus, err;
	struct bt_sync s;
	struct task_struct *task;

	if (bt->pid > 0) {
		task = find_task_by_vpid(bt->pid);
		if (task == NULL) {
			return -EINVAL;
		}
	}
	else {
		return -EINVAL;
	}

	err = mutex_lock_killable(&task->signal->cred_guard_mutex);
        if (err)
                return err;
        if (!ptrace_may_access(task, PTRACE_MODE_ATTACH)) {
                mutex_unlock(&task->signal->cred_guard_mutex);
                return -EPERM;
        }
 
	get_online_cpus();
	preempt_disable();

	nr_cpus = num_online_cpus();
	atomic_set(&s.cpus_report, nr_cpus - 1);
	atomic_set(&s.cpus_lock, 1);

	smp_call_function(per_cpu_get_bt, &s, 0);

	while (atomic_read(&s.cpus_report) != 0);

	aed_get_bt(task, bt);

	atomic_set(&s.cpus_report, nr_cpus - 1);
	atomic_set(&s.cpus_lock, 0);
	while (atomic_read(&s.cpus_report) != 0);

	preempt_enable();
	put_online_cpus();

        mutex_unlock(&task->signal->cred_guard_mutex);

	return 0;
}
