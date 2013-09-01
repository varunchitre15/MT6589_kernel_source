#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/kallsyms.h>
#include <linux/notifier.h>
#include <asm/uaccess.h>
#include "aed.h"

#ifndef PARTIAL_BUILD

static spinlock_t fiq_debugger_test_lock0;
static spinlock_t fiq_debugger_test_lock1;
static int test_case = 0;
static int test_cpu = 0;
static struct task_struct *wk_tsk[NR_CPUS];
extern int nr_cpu_ids;
extern struct atomic_notifier_head panic_notifier_list;

static int force_spinlock(struct notifier_block *this, unsigned long event, void *ptr)
{
	unsigned long flags;
	xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "\n ==> panic flow spinlock deadlock test \n");
	spin_lock_irqsave(&fiq_debugger_test_lock0, flags);
	while(1);
	xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "\n You should not see this \n");
	return 0;
}
static struct notifier_block panic_test = {
	.notifier_call  = force_spinlock,
	.priority	= INT_MAX,
};


static int kwdt_thread_test(void *arg)
{
	struct sched_param param = { .sched_priority = RTPM_PRIO_WDT};
	int cpu;
	unsigned long flags;

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);
	cpu = smp_processor_id();
	xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "\n ==> kwdt_thread_test on CPU %d, test_case = %d \n", cpu, test_case);
	msleep(1000);
	
	if (test_case == 1) {
		if (cpu == test_cpu) {
			xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
				"\n CPU %d : disable preemption and local IRQ forever", cpu);
			spin_lock_irqsave(&fiq_debugger_test_lock0, flags);
	 		while (1);
	 		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
	 			"\n Error : You should not see this ! \n");
		} else {
			xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
				"\n CPU %d : Do nothing and exit \n ", cpu);
		}
	} else if (test_case == 2) {
		if (cpu == test_cpu) {
			msleep(1000);
			xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
				"\n CPU %d : disable preemption and local IRQ forever", cpu);
			spin_lock_irqsave(&fiq_debugger_test_lock0, flags);
	 		while (1);
	 		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, 
	 			"\n Error : You should not see this ! \n");
		} else {
			xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
				"\n CPU %d : disable irq \n ", cpu);
			local_irq_disable();
			while (1);
			xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, 
				"\n Error : You should not see this ! \n");
		}
	} else if (test_case == 3) {
		if (cpu == test_cpu) {
			xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
				"\n CPU %d : register panic notifier and force spinlock deadlock \n", cpu);
			atomic_notifier_chain_register(&panic_notifier_list, &panic_test);
			spin_lock_irqsave(&fiq_debugger_test_lock0, flags);
			while(1);
			xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, 
				"\n Error : You should not see this ! \n");
		} else {
			xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
				"\n CPU %d : Do nothing and exit \n ", cpu);
		}
	} else if (test_case == 4) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
			"\n CPU %d : disable preemption and local IRQ forever \n ", cpu);
		spin_lock_irqsave(&fiq_debugger_test_lock1, flags);
		while (1);
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "\n Error : You should not see this ! \n");
	}
	return 0;
}

static int proc_write_generate_wdt(struct file* file,
					      const char __user *buf, unsigned long count,
					      void *data)
{
	unsigned int i = 0;
	char msg[4];
	unsigned char name[10] = {0};

	if ((count < 2) || (count > sizeof(msg))) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "\n count = %d \n", count);
		return -EINVAL;
	}
	if (copy_from_user(msg, buf, count)) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "copy_from_user error");
		return -EFAULT;
	}
	msg[count] = 0;
	test_case = (unsigned int) msg[0] - '0';
	test_cpu = (unsigned int) msg[2] - '0';
	xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
		"test_case = %d, test_cpu = %d", test_case, test_cpu);
	if ((msg[1] != ':') || (test_case < 1) || (test_case > 4) 
		|| (test_cpu < 0) || (test_cpu > nr_cpu_ids)) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, \
							"WDT test - Usage: [test case number(1~4):test cpu(0~%d)] \n", \
							nr_cpu_ids);
		return -EINVAL;
	}

	if (test_case == 1) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
			"Test 1 : One CPU WDT timeout (smp_send_stop succeed) \n");
	} else if (test_case == 2) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
			"Test 2 : One CPU WDT timeout, other CPU disable irq (smp_send_stop fail in old design) \n");
	} else if (test_case == 3) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
			"Test 3 : WDT timeout but deadlock in panic flow \n");
	} else if (test_case == 4) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, 
			"Test 4 : All CPU WDT timeout (other CPU stop in the loop) \n");
	} else {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "\n Unknown test_case %d \n", test_case);
		return -EINVAL;
	}

	// create kernel threads and bind on every cpu
	for(i = 0; i < nr_cpu_ids; i++) {
		sprintf(name, "wd-test-%d", i);
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "[WDK]thread name: %s\n", name);
		wk_tsk[i] = kthread_create(kwdt_thread_test, &data, name);
		if (IS_ERR(wk_tsk[i])) {
			int ret = PTR_ERR(wk_tsk[i]);
			wk_tsk[i] = NULL;
			return ret;
		}
		kthread_bind(wk_tsk[i], i);
	}

	for(i = 0; i < nr_cpu_ids; i++) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, " wake_up_process(wk_tsk[%d]) \n", i);
		wake_up_process(wk_tsk[i]);
	}

	return count;
}

static int proc_read_generate_wdt(char *page, char **start,
					     off_t off, int count,
					     int *eof, void *data)
{
	return sprintf(page, "WDT test - Usage: [test case number:test cpu] \n");
}

static int proc_read_generate_oops(char *page, char **start,
			     off_t off, int count,
			     int *eof, void *data)
{
	int len;
	BUG();
	len = sprintf(page, "Oops Generated\n");

	return len;
}

extern void aed_md_exception(const int *log, int log_size, const int *phy, 
                                int phy_size, const char* detail);

static int proc_read_generate_ee(char *page, char **start,
				 off_t off, int count,
				 int *eof, void *data)
{
#define TEST_PHY_SIZE 0x10000

	int log[16], i;
	char *ptr;

	memset(log, 0, sizeof(log));
	ptr = kmalloc(TEST_PHY_SIZE, GFP_KERNEL);
	if (ptr == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "proc_read_generate_ee kmalloc fail\n");
		return sprintf(page, "kmalloc fail \n");
	}
	for (i = 0; i < TEST_PHY_SIZE; i++) {
		ptr[i] = (i % 26) + 'A';
	}
	aed_md_exception(log, 0, (int *)ptr, TEST_PHY_SIZE, __FILE__);
	kfree(ptr);

	return sprintf(page, "Modem EE Generated\n");
}

static int proc_read_generate_combo(char *page, char **start,
				 off_t off, int count,
				 int *eof, void *data)
{
#define TEST_PHY_SIZE 0x10000

	int i;
	char *ptr;

	ptr = kmalloc(TEST_PHY_SIZE, GFP_KERNEL);
	if (ptr == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "proc_read_generate_combo kmalloc fail\n");
		return sprintf(page, "kmalloc fail \n");
	}
	for (i = 0; i < TEST_PHY_SIZE; i++) {
		ptr[i] = (i % 26) + 'A';
	}

	aee_kernel_dal_show("Oops, MT662X is generating core dump, please wait up to 5 min \n");
	aed_combo_exception(NULL, 0, (int *)ptr, TEST_PHY_SIZE, __FILE__);
	kfree(ptr);

	return sprintf(page, "Combo EE Generated\n");
}

static int proc_read_generate_kernel_notify(char *page, char **start,
					     off_t off, int count,
					     int *eof, void *data)
{
	return sprintf(page, "Usage: write message with format \"R|W|E:Tag:You Message\" into this file to generate kernel warning\n");
}

static int proc_write_generate_kernel_notify(struct file* file,
					      const char __user *buf, unsigned long count,
					      void *data)
{
	char msg[164], *colon_ptr;

	if (count == 0)	{
		return -EINVAL;
	}

	if ((count < 5) || (count >= sizeof(msg))) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "aed: %s count sould be >= 5 and <= %d bytes.\n", __func__, sizeof(msg));
		return -EINVAL;
	}

	if (copy_from_user(msg, buf, count)) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "aed: %s unable to read message\n", __func__);
		return -EFAULT;
	}
	/* Be safe */
	msg[count] = 0;

        if (msg[1] != ':') {
		return -EINVAL;
        }
	colon_ptr = strchr(&msg[2], ':');
	if ((colon_ptr == NULL) || ((colon_ptr - msg) > 32)) {
		xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "aed: %s cannot find valid module name\n", __func__); 
		return -EINVAL;
	}
	*colon_ptr = 0;
	
	switch (msg[0]) {
	case 'R':
		aee_kernel_reminding(&msg[2], colon_ptr + 1);
		break;

	case 'W':
		aee_kernel_warning(&msg[2], colon_ptr + 1);
		break;

	case 'E':
		aee_kernel_exception(&msg[2], colon_ptr + 1);
		break;

	default:
		return -EINVAL;
	}

	return count;
}

int aed_proc_debug_init(struct proc_dir_entry *aed_proc_dir)
{
	struct proc_dir_entry *aed_proc_generate_oops_file;
	struct proc_dir_entry *aed_proc_generate_ee_file;
	struct proc_dir_entry *aed_proc_generate_combo_file;
	struct proc_dir_entry *aed_proc_generate_ke_file;
	struct proc_dir_entry *aed_proc_generate_wdt_file;

	spin_lock_init(&fiq_debugger_test_lock0);
	spin_lock_init(&fiq_debugger_test_lock1);
	aed_proc_generate_oops_file = create_proc_read_entry("generate-oops", 
							     0400, aed_proc_dir, 
							     proc_read_generate_oops,
							     NULL);
	if (aed_proc_generate_oops_file == NULL) {
	  xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at generate-oops\n");
	  return -ENOMEM;
	}

	aed_proc_generate_ke_file = create_proc_entry("generate-kernel-notify",
						      S_IFREG | 0600, aed_proc_dir);
	if (aed_proc_generate_ke_file == NULL) {
	  xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at generate-kernel-notify\n");
	  return -ENOMEM;
	}
	aed_proc_generate_ke_file->write_proc = proc_write_generate_kernel_notify;
	aed_proc_generate_ke_file->read_proc = proc_read_generate_kernel_notify;
	
	aed_proc_generate_ee_file = create_proc_read_entry("generate-ee", 
							   0400, aed_proc_dir, 
							   proc_read_generate_ee,
							   NULL);
	if(aed_proc_generate_ee_file == NULL) {
	  xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at generate-ee\n");
	  return -ENOMEM;
	}

	aed_proc_generate_combo_file = create_proc_read_entry("generate-combo", 
							   0400, aed_proc_dir, 
							   proc_read_generate_combo,
							   NULL);
	if(aed_proc_generate_combo_file == NULL) {
	  xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at generate-combo\n");
	  return -ENOMEM;
	}

	aed_proc_generate_wdt_file = create_proc_entry("generate-wdt",
						      S_IFREG | 0600, aed_proc_dir);
	if (aed_proc_generate_wdt_file == NULL) {
	  xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at generate-wdt\n");
	  return -ENOMEM;
	}
	aed_proc_generate_wdt_file->write_proc = proc_write_generate_wdt;
	aed_proc_generate_wdt_file->read_proc = proc_read_generate_wdt;

	return 0;
}

int aed_proc_debug_done(struct proc_dir_entry *aed_proc_dir)
{
	remove_proc_entry("generate-oops", aed_proc_dir);
	remove_proc_entry("generate-kernel-notify", aed_proc_dir);
	remove_proc_entry("generate-ee", aed_proc_dir);
	remove_proc_entry("generate-combo", aed_proc_dir);
	remove_proc_entry("generate-wdt", aed_proc_dir);
	return 0;
}

#else

int aed_proc_debug_init(struct proc_dir_entry *aed_proc_dir)
{
	return 0;
}

int aed_proc_debug_done(struct proc_dir_entry *aed_proc_dir)
{
	return 0;
}

#endif
