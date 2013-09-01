#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rtpm_prio.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/rtpm_prio.h>
#include <linux/rtc.h>
#include <linux/cpu.h>
#include <linux/hrtimer.h>
#ifdef CONFIG_LOCAL_WDT
extern int nr_cpu_ids;
#else
#define nr_cpu_ids		1
#endif


//extern int nr_cpu_ids;
//int enable_clock(int id, unsigned char *name);
static int test_case  = 0;

static int ts_msleep  = 0;
static int  msleep_times = 0;

static int ts_mdelay  = 0;
static int  mdelay_times = 0;

static int ts_udelay  = 0;
static int  udelay_times = 0;

static long ts_hrtimer_sec  = 0;
static unsigned long ts_hrtimer_nsecs = 0;
static int  ts_hrtimer_times = 0;

static struct hrtimer hrtimer_test;
u64 hr_t1=0;
u64 hr_t2=0;

module_param(test_case, int, 00664);

module_param(ts_msleep, int, 00664);
module_param(msleep_times, int, 00664);

module_param(ts_mdelay, int, 00664);
module_param(mdelay_times, int, 00664);

module_param(ts_udelay, int, 00664);
module_param(udelay_times, int, 00664);

module_param(ts_hrtimer_sec, int, 00664);
module_param(ts_hrtimer_nsecs, int, 00664);
module_param(ts_hrtimer_times, int, 00664);

static DEFINE_SPINLOCK(wdt_test_lock0);
static DEFINE_SPINLOCK(wdt_test_lock1);
static struct task_struct *wk_tsk[2];// cpu: 2
static int data;

static int hrtimer_test_case();

static int __cpuinit
cpu_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	int hotcpu = (unsigned long)hcpu;

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
//		watchdog_prepare_cpu(hotcpu);
		break;
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
//		
        if(hotcpu < nr_cpu_ids)
        {
		kthread_bind(wk_tsk[hotcpu], hotcpu);
	      wake_up_process(wk_tsk[hotcpu]);	
		printk("[WDK-test]cpu %d plug on ", hotcpu);
        }
		break;
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		printk("[WDK-test]:start Stop CPU:%d\n", hotcpu);
	
		break;
#endif /* CONFIG_HOTPLUG_CPU */
	}

	/*
	 * hardlockup and softlockup are not important enough
	 * to block cpu bring up.  Just always succeed and
	 * rely on printk output to flag problems.
	 */
	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata cpu_nfb = {
	.notifier_call = cpu_callback
};


static int msleep_test(int length, int times)
{

   u64 t1=0;
   u64 t2=0;
   int i =0;
   t1 = sched_clock();
   for(i=0; i<times; i++)
   {
      msleep(length);
   }
   t2 = sched_clock();
   printk("msleep_test,  msleep(%d) test %d times\n", length,times);
   printk("msleep_test: t1 =%lld, t2=%lld, delta=%lld\n", t1,t2, t2-t1);
   
}

static int udelay_test(int length, int times)
{

   u64 t1=0;
   u64 t2=0;
   int i =0;
   t1 = sched_clock();
   for(i=0; i<times; i++)
   {
      udelay(length);
   }
   t2 = sched_clock();
   printk("udelay_test,  udelay_test(%d) test %d times\n", length,times);

   printk("udelay_test :t1 =%lld, t2=%lld, delta=%lld\n", t1,t2, t2-t1);
   
}

static int mdelay_test(int length, int times)
{

   u64 t1=0;
   u64 t2=0;
   int i =0;
   t1 = sched_clock();
   for(i=0; i<times; i++)
   {
      mdelay(length);
   }
   t2 = sched_clock();
   printk("mdelay_test,  mdelay_test(%d) test %d times\n", length,times);

   printk("mdelay_test:t1 =%lld, t2=%lld, delta=%lld\n", t1,t2, t2-t1);
   
}


#define read_cntpct(cntpct_lo, cntpct_hi)   \
do {    \
    __asm__ __volatile__(   \
    "MRRC p15, 0, %0, %1, c14\n"    \
    :"=r"(cntpct_lo), "=r"(cntpct_hi)   \
    :   \
    :"memory"); \
} while (0)
static int phycical_count_test()
{
  unsigned int cntpct_lo1 = 0;
  unsigned int cntpct_hi1 = 0;
  unsigned int cntpct_lo2 = 0;
  unsigned int cntpct_hi2 = 0;
  printk("phycical_count_test start\n");
  while(1)
  {
     read_cntpct(cntpct_lo1, cntpct_hi1);
     read_cntpct(cntpct_lo2, cntpct_hi2);
	 if(cntpct_hi2 == cntpct_hi1)
	 {
	   if(cntpct_lo2 < cntpct_lo1)
	   {
	     if(0xffff == cntpct_lo1)
	     {
	       printk("fwq 0 by pass bug cntpct_hi1=%u,cntpct_lo1=%u, cntpct_hi2=%u,cntpct_lo2=%u   \n",cntpct_hi1,cntpct_lo1,cntpct_hi2,cntpct_lo2 );
	       continue;
	     }
	     printk("fwq 1 cntpct_hi1=%d,cntpct_lo1=%d, cntpct_hi2=%d,cntpct_lo2=%d   \n",cntpct_hi1,cntpct_lo1,cntpct_hi2,cntpct_lo2 );
	     //break;
	   }
	 }
	 if(cntpct_hi2 < cntpct_hi1)
	 {
	    printk("fwq 2 cntpct_hi1=%d,cntpct_lo1=%d, cntpct_hi2=%d,cntpct_lo2=%d   \n",cntpct_hi1,cntpct_lo1,cntpct_hi2,cntpct_lo2 );
	    //break;
	 }
  }
  printk("phycical_count_test end\n");
  
}
static int kwdt_thread_test(void *arg)
{
	struct sched_param param = { .sched_priority = RTPM_PRIO_WDT};
	int cpu;
	unsigned int flags;

    sched_setscheduler(current, SCHED_FIFO, &param);

    set_current_state(TASK_INTERRUPTIBLE);
    for(;;){ 
  	printk("timer test debug start, cpu:%d\n", cpu);
  	spin_lock(&wdt_test_lock0);
  	cpu = smp_processor_id();
  	spin_unlock(&wdt_test_lock0);

     //phycical_count_test();
	 
	 if(ts_msleep != 0) 
	 {	
	  	//printk("CPU:%d, msleep(%d) test\n", cpu,ts_msleep);
		if(0 != msleep_times)
		{
		   msleep_test(ts_msleep,msleep_times);
		}
		else
		{
		   msleep_test(ts_msleep,1);
		}
	  	 
	 }

	 if(ts_mdelay != 0) 
	 {	
	  	//printk("CPU:%d, mdelay(%d) test\n", cpu,ts_mdelay);
		if(0 != mdelay_times)
		{
		   mdelay_test(ts_mdelay,mdelay_times);
		}
		else
		{
		   mdelay_test(ts_mdelay,1);
		}
	  	 
	 }

	  if(ts_udelay != 0) 
	 {	
	  	//printk("CPU:%d, udelay(%d) test\n", cpu,ts_udelay);
		if(0 != udelay_times)
		{
		   udelay_test(ts_udelay,udelay_times);
		}
		else
		{
		   udelay_test(ts_udelay,1);
		}
	  	 
	 }

	 if(ts_hrtimer_nsecs !=0 || ts_hrtimer_sec !=0)
	 {
	     hrtimer_test_case();
	 }
	
  	msleep(5*1000);//5s
  	printk("timer test debug end, cpu:%d\n", cpu);
  	}
  	return 0;
}

static enum hrtimer_restart hrtimer_test_func(struct hrtimer *timer)
{
        
 		hr_t2 = sched_clock();
        printk("[hrtimer_test_func] t1=%lld,t2 =%lld \n",hr_t1, hr_t2);

		printk("[hrtimer_test_func] delta=%lld,  \n",hr_t2-hr_t1);
 		return HRTIMER_NORESTART;
}

static int hrtimer_test_case()
{  
   int ret =0;
   int value =0; //Unit ms
   printk("hrtimer_test_case:ts_hrtimer_sec=%ld, ts_hrtimer_nsecs=%ld\n", ts_hrtimer_sec,ts_hrtimer_nsecs);
   hr_t1 = sched_clock();
   hrtimer_start(&hrtimer_test, 
							ktime_set(ts_hrtimer_sec, ts_hrtimer_nsecs),
							HRTIMER_MODE_REL);
   
}
static int start_kicker(void)
{

	int i;
	unsigned char name[10] = {0};

	hrtimer_init(&hrtimer_test, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_test.function = hrtimer_test_func;

	for(i = 0; i < 1; i++){
	sprintf(name, "wdtk-%d", i);
	printk("[WDK]:thread name: %s\n", name);
	wk_tsk[i] = kthread_create(kwdt_thread_test, &data, name);
	if (IS_ERR(wk_tsk[i])) {
		int ret = PTR_ERR(wk_tsk[i]);
		wk_tsk[i] = NULL;
		return ret;
	}
	kthread_bind(wk_tsk[i], i);
	wake_up_process(wk_tsk[i]);
}
	return 0;
}
static int __init test_init(void)
{
	//enable_clock(12, "Vfifo");
	
	//register_cpu_notifier(&cpu_nfb);
	start_kicker();
	return 0;
}

static void __init test_exit(void)
{	

}
#if 0
int TimerUT_proc_init(void) {

	struct proc_dir_entry *de = create_proc_entry(PROC_WK, 0660, 0);

	printk("[WDK] Initialize proc\n");

	de->read_proc = wk_proc_cmd_read;
	de->write_proc = wk_proc_cmd_write;

	return 0 ;
}


void TimerUT_proc_exit(void) 
{

	remove_proc_entry(PROC_WK, NULL);

}
#endif
module_init(test_init);
module_exit(test_exit);