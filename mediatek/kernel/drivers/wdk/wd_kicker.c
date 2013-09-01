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
#include <linux/aee.h>
#include "wd_kicker.h"

/*************************************************************************
 * Feature configure region 
 *************************************************************************/
#define __ENABLE_WDT_SYSFS__
#define __ENABLE_WDT_AT_INIT__

//------------------------------------------------------------------------
#define PFX "wdk: "
#define DEBUG_WDK	0
#if DEBUG_WDK
#define dbgmsg(msg...) printk(PFX msg)
#else
#define dbgmsg(...)
#endif

#define msg(msg...) printk(KERN_INFO PFX msg);
#define warnmsg(msg...) printk(KERN_WARNING PFX msg);
#define errmsg(msg...) printk(KERN_WARNING PFX msg);

#define MIN_KICK_INTERVAL	 1
#define MAX_KICK_INTERVAL	30
#define PROC_WK "wdk"

static int kwdt_thread(void *arg);
static int start_kicker(void);

static int debug_sleep = 0;
//static int data;
//static spinlock_t lock = SPIN_LOCK_UNLOCKED;
static DEFINE_SPINLOCK(lock);

//Monkey.QHQ
//static struct task_struct *wk_tsk;

#if NR_CPUS == 1
#define nr_cpu_ids		1
#else
extern int nr_cpu_ids;
#endif
#define CPU_NR (nr_cpu_ids)
struct task_struct *wk_tsk[16];// max cpu 16
static unsigned long kick_bit = 0;

//Monkey.QHQ

enum wk_wdt_mode g_wk_wdt_mode = WK_WDT_NORMAL_MODE;
static struct wk_wdt *g_wk_wdt = NULL;
static int g_kinterval = -1;
static int g_timeout = -1;
static int g_need_config = 0;
static int wdt_start = 0;
static int g_enable = 1;



static char cmd_buf[256];

static int wk_proc_cmd_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len;
	len = snprintf(buf, count, "mode interval timeout enable \n%-4d %-9d %-8d %-7d\n", g_wk_wdt_mode, g_kinterval, g_timeout, g_enable);
	return len;
}

static int wk_proc_cmd_write(struct file *file, const char *buf, unsigned long count, void *data)
{
	int ret;
	//static int wdt_start = 0; // Adjust this variable to file view scope 
	int timeout;
	int mode;
	int kinterval;
	int en; //enable or disable ext wdt 1<-->enable 0<-->disable

	if (count == 0)
		return -1;

	if(count > 255) 
		count = 255;

	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;
	
	cmd_buf[count] = '\0';

	dbgmsg("Write %s\n", cmd_buf);

	sscanf(cmd_buf, "%d %d %d %d %d", &mode, &kinterval, &timeout, &debug_sleep,&en);

	printk("[WDK] mode=%d interval=%d timeout=%d enable =%d\n", mode, kinterval, timeout,en);

	if (timeout < kinterval) {
		errmsg("The interval(%d) value should be smaller than timeout value(%d)\n", kinterval, timeout);
		return -1;
	}

	if ((timeout <  MIN_KICK_INTERVAL) || (timeout > MAX_KICK_INTERVAL)) {
		errmsg("The timeout(%d) is invalid (%d - %d)\n", kinterval, MIN_KICK_INTERVAL, MAX_KICK_INTERVAL);
		return -1;
	}

	if ((kinterval <  MIN_KICK_INTERVAL) || (kinterval > MAX_KICK_INTERVAL)) {
		errmsg("The interval(%d) is invalid (%d - %d)\n",kinterval,  MIN_KICK_INTERVAL, MAX_KICK_INTERVAL);
		return -1;
	}

	if (!((mode == WK_WDT_NORMAL_MODE) || 
	      (mode == WK_WDT_EXP_MODE))) {
		errmsg("Tha watchdog kicker wdt mode is not correct\n");
		errmsg("WK_WDT_NORMAL_MODE = %d\n", WK_WDT_NORMAL_MODE);
		errmsg("WK_WDT_EXP_MODE = %d\n", WK_WDT_EXP_MODE);
		return -1;
	}

	if(1 == en)
	{
	  g_wk_wdt->enable(WK_WDT_EN);
	  printk("[WDK] enable wdt \n");
	}
	if(0 == en)
	{
	  g_wk_wdt->enable(WK_WDT_DIS);
	  printk("[WDK] disable wdt \n");
	}
		
		
  spin_lock(&lock);

    g_enable= en;
	g_kinterval = kinterval;
#if 0 //CONFIG_MTK_AEE_FEATURE auto enable or disable WDT isr or hw mode
	g_wk_wdt_mode = WK_WDT_EXP_MODE;
	msg("Enable wdt with interrupt mode only %d\n", g_wk_wdt_mode);
#else
	g_wk_wdt_mode = mode;
#endif
	g_timeout = timeout;
  g_need_config = 1;
  spin_unlock(&lock);
	/* Start once only */
	if (wdt_start == 0) {
		start_kicker();
	}
	spin_lock(&lock);
	wdt_start = 1;
  spin_unlock(&lock);
	return count;
}

static int start_kicker_thread_with_default_setting(void)
{
	int ret = 0;

	spin_lock(&lock);

	g_kinterval = 20;  // default interval: 20s

	g_need_config = 0; // Note, we DO NOT want to call configure function

	wdt_start = 1;     // Start once only
  spin_unlock(&lock);	
	start_kicker();


	return ret;
}

static unsigned int cpus_kick_bit = 0;
void wk_start_kick_cpu(int cpu)
{
	spin_lock(&lock);
	cpus_kick_bit |= (1<<cpu); 
	kick_bit = 0;
	spin_unlock(&lock);
	if(IS_ERR(wk_tsk[cpu]))
	{
		printk("[wdk]wk_task[%d] is NULL\n",cpu);
	}
	else
	{
		kthread_bind(wk_tsk[cpu], cpu);
		printk("[wdk]bind thread[%d] to cpu[%d]\n",wk_tsk[cpu]->pid,cpu);
		wake_up_process(wk_tsk[cpu]);
	}
}

void wk_start_kick_cpu_hotplug(int cpu)
{
	spin_lock(&lock);
	cpus_kick_bit |= (1<<cpu); 
	kick_bit = 0;
	spin_unlock(&lock);
//	kthread_bind(wk_tsk[cpu], cpu);
//	wake_up_process(wk_tsk[cpu]);
}

//EXPORT_SYMBOL(wk_stop_kick_cpu);
/* deep idle or cpu hotplug */
void wk_stop_kick_cpu(int cpu)
{
	spin_lock(&lock);
	cpus_kick_bit &= (~(1<<cpu)); 
//	if(kick_bit == cpus_kick_bit)// if cpu-x kick and stop cpu-x, need notify clear kick_bit
	kick_bit = 0;
	spin_unlock(&lock);	
}
//EXPORT_SYMBOL(wk_stop_kick_cpu);

unsigned int wk_check_kick_bit(void)
{
	return cpus_kick_bit;
}

int wk_proc_init(void) {

	struct proc_dir_entry *de = create_proc_entry(PROC_WK, 0660, 0);

	printk("[WDK] Initialize proc\n");

	de->read_proc = wk_proc_cmd_read;
	de->write_proc = wk_proc_cmd_write;

	return 0 ;
}


void wk_proc_exit(void) 
{

	remove_proc_entry(PROC_WK, NULL);

}

int wk_register_wdt(struct wk_wdt *wk_wdt)
{
	if (!wk_wdt) {
		BUG();
	}

	g_wk_wdt = wk_wdt;

	return 0;
}
EXPORT_SYMBOL(wk_register_wdt);

static int kwdt_thread(void *arg)
{
	
	struct sched_param param = { .sched_priority = RTPM_PRIO_WDT};
	struct rtc_time tm;
	struct timeval tv = {0};
	/* android time */
	struct rtc_time tm_android;
	struct timeval tv_android = {0};
	int cpu = 0;

	int local_bit = 0, loc_need_config = 0, loc_timeout = 0;
	struct wk_wdt *loc_wk_wdt = NULL;
	#ifdef 	CONFIG_LOCAL_WDT
	int delay_boot_time = 5; // after 5s local wdt timeout, external wdt reboot, for dump log  
    #endif
    sched_setscheduler(current, SCHED_FIFO, &param);

    set_current_state(TASK_INTERRUPTIBLE);

	for(;;)
	{
		
		if (kthread_should_stop()) break;
		spin_lock(&lock);		
		cpu = smp_processor_id();
		loc_wk_wdt = g_wk_wdt;
		loc_need_config = g_need_config;
		loc_timeout = g_timeout;
		spin_unlock(&lock);	
		if (loc_wk_wdt && loc_wk_wdt->kick_wdt && loc_wk_wdt->config && g_enable) 
		{	
			if (loc_need_config)
			{
				// external wdt, hw reboot mode
				loc_wk_wdt->config(WK_WDT_EXT_TYPE, WK_WDT_NORMAL_MODE, loc_timeout); 
				#ifdef 	CONFIG_LOCAL_WDT
				loc_wk_wdt->config(WK_WDT_LOC_TYPE, WK_WDT_NORMAL_MODE, loc_timeout - delay_boot_time);
                #endif
				spin_lock(&lock);
				g_need_config = 0;
				spin_unlock(&lock);
			}
			//printk("[WDK]  cpu-task=%d, current_pid=%d\n",  wk_tsk[cpu]->pid,  current->pid);
			if(wk_tsk[cpu]->pid == current->pid)
			{
			   //only process WDT info if thread-x is on cpu-x
			   spin_lock(&lock);
			   local_bit = kick_bit;
			   printk_sched("[WDK], local_bit:0x%x, cpu:%d,RT[%lld]\n", local_bit, cpu,sched_clock());
			   if((local_bit&(1<<cpu)) == 0)
			   {
				//printk("[WDK]: set  WDT kick_bit\n");
				local_bit |= (1<<cpu);
				//aee_rr_rec_wdk_kick_jiffies(jiffies);
			   }
			   printk_sched("[WDK], local_bit:0x%x, cpu:%d, check bit0x:%x,RT[%lld]\n", local_bit, cpu, wk_check_kick_bit(),sched_clock());
			   if(local_bit == wk_check_kick_bit())
			   {
			      printk_sched("[WDK]: kick Ex WDT,RT[%lld]\n",sched_clock());
			      loc_wk_wdt->kick_wdt(WK_WDT_EXT_TYPE);// for KICK external wdt
			      local_bit = 0;
			   }
			   kick_bit = local_bit;
			   spin_unlock(&lock);
			   
               		   #ifdef CONFIG_LOCAL_WDT
			   printk_sched("[WDK]: cpu:%d, kick local wdt,RT[%lld]\n", cpu,sched_clock());
			   // kick local wdt
			   loc_wk_wdt->kick_wdt(WK_WDT_LOC_TYPE); 
			   #endif
			}
		}
		else if(0 == g_enable)
		{
		    printk("WDK stop to kick \n");
		}
		else {
			errmsg("No watch dog driver is hooked\n");
			BUG();
		}
		
		if(wk_tsk[cpu]->pid == current->pid)	
		{
		   #if (DEBUG_WDK==1)
		   msleep(debug_sleep * 1000);
		   dbgmsg("WD kicker woke up %d\n", debug_sleep);
		   #endif
		   do_gettimeofday(&tv);
		   tv_android = tv;
		   rtc_time_to_tm(tv.tv_sec, &tm);
		   tv_android.tv_sec -= sys_tz.tz_minuteswest*60;
		   rtc_time_to_tm(tv_android.tv_sec, &tm_android);
		   printk_sched("[thread:%d][RT:%lld] %d-%02d-%02d %02d:%02d:%02d.%u UTC; android time %d-%02d-%02d %02d:%02d:%02d.%03d\n",  current->pid,sched_clock(),
		   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		   tm.tm_hour, tm.tm_min, tm.tm_sec,
		   (unsigned int) tv.tv_usec,
		   tm_android.tm_year + 1900, tm_android.tm_mon + 1, tm_android.tm_mday,
		   tm_android.tm_hour, tm_android.tm_min, tm_android.tm_sec,
		   (unsigned int) tv_android.tv_usec	);
		}
		
		msleep((g_kinterval) * 1000);

	}
	printk("[WDK] WDT kicker thread stop, cpu:%d, pid:%d\n", cpu, current->pid);
	return 0;
}	

static int start_kicker(void)
{

	int i;
	
	for(i = 0; i < CPU_NR; i++)
	{
		wk_tsk[i] = kthread_create(kwdt_thread, (void *)(unsigned long)i, "wdtk-%d", i);
		if (IS_ERR(wk_tsk[i])) {
			int ret = PTR_ERR(wk_tsk[i]);
			wk_tsk[i] = NULL;
			return ret;
		}	
	wk_start_kick_cpu(i);
      }

	return 0;
}

/******************************************************************************
 * SYSFS support
******************************************************************************/
#ifdef __ENABLE_WDT_SYSFS__
/*---------------------------------------------------------------------------*/
/* Externanl functions */
extern void mtk_wdt_disable(void);
extern int mtk_wdt_get_en_setting(void);
/*---------------------------------------------------------------------------*/
/*define sysfs entry for configuring debug level and sysrq*/
ssize_t mtk_rgu_attr_show(struct kobject *kobj, struct attribute *attr, char *buffer);
ssize_t mtk_rgu_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t size);
ssize_t mtk_rgu_pause_wdt_show(struct kobject *kobj, char *page);
ssize_t mtk_rgu_pause_wdt_store(struct kobject *kobj, const char *page, size_t size);
/*---------------------------------------------------------------------------*/
struct sysfs_ops mtk_rgu_sysfs_ops = {
	.show   = mtk_rgu_attr_show,
	.store  = mtk_rgu_attr_store,
};
/*---------------------------------------------------------------------------*/
struct mtk_rgu_sys_entry {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj, char *page);
	ssize_t (*store)(struct kobject *kobj, const char *page, size_t size);
};
/*---------------------------------------------------------------------------*/
static struct mtk_rgu_sys_entry pause_wdt_entry = {
	{ .name = "pause", .mode = S_IRUGO | S_IWUSR }, // remove  .owner = NULL,  
	mtk_rgu_pause_wdt_show,
	mtk_rgu_pause_wdt_store,
};
/*---------------------------------------------------------------------------*/
struct attribute *mtk_rgu_attributes[] = {
	&pause_wdt_entry.attr,
	NULL,
};
/*---------------------------------------------------------------------------*/
struct kobj_type mtk_rgu_ktype = {
	.sysfs_ops = &mtk_rgu_sysfs_ops,
	.default_attrs = mtk_rgu_attributes,
};
/*---------------------------------------------------------------------------*/
static struct mtk_rgu_sysobj {
	struct kobject kobj;
} rgu_sysobj;
/*---------------------------------------------------------------------------*/
int mtk_rgu_sysfs(void) 
{
	struct mtk_rgu_sysobj *obj = &rgu_sysobj;

	memset(&obj->kobj, 0x00, sizeof(obj->kobj));
    
	obj->kobj.parent = kernel_kobj;
	if (kobject_init_and_add(&obj->kobj, &mtk_rgu_ktype, NULL, "mtk_rgu")) {
		kobject_put(&obj->kobj);
		return -ENOMEM;
	}
	kobject_uevent(&obj->kobj, KOBJ_ADD);

	return 0;
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_rgu_attr_show(struct kobject *kobj, struct attribute *attr, char *buffer) 
{
	struct mtk_rgu_sys_entry *entry = container_of(attr, struct mtk_rgu_sys_entry, attr);
	return entry->show(kobj, buffer);
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_rgu_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t size) 
{
	struct mtk_rgu_sys_entry *entry = container_of(attr, struct mtk_rgu_sys_entry, attr);
	return entry->store(kobj, buffer, size);
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_rgu_pause_wdt_show(struct kobject *kobj, char *buffer) 
{
	int remain = PAGE_SIZE;
	int len;
	char *ptr = buffer;

        len = scnprintf(ptr, remain, "0x%1x\n", !mtk_wdt_get_en_setting());
        ptr += len;
        remain -= len;

	return (PAGE_SIZE-remain);
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_rgu_pause_wdt_store(struct kobject *kobj, const char *buffer, size_t size) 
{
	int pause_wdt;
	int res = sscanf(buffer, "%x", &pause_wdt);

	if (res != 1) {
		printk("%s: expect 1 numbers\n", __FUNCTION__);
	}else{
		// For real case, pause wdt if get value is not zero. Suspend and resume may enable wdt again
		if(pause_wdt)
			mtk_wdt_disable();
	}
	return size;
}
/*---------------------------------------------------------------------------*/
#endif /*__ENABLE_WDT_SYSFS__*/
/*---------------------------------------------------------------------------*/

static int __cpuinit
wk_cpu_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	int hotcpu = (unsigned long)hcpu;

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
//		watchdog_prepare_cpu(hotcpu);
		break;
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
            
		wk_start_kick_cpu(hotcpu);
		
		if (g_wk_wdt && g_wk_wdt->kick_wdt) 
		{
			g_wk_wdt->kick_wdt(WK_WDT_EXT_TYPE);// for KICK external wdt
		}
		else
		{
			printk("[WDK]g_wk_wdt or g_wd_wdt->kick_wdt is NULL");
		}
			
		printk("[WDK]cpu %d plug on wk_start_kick_cpu", hotcpu);
		break;
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		
		if (g_wk_wdt && g_wk_wdt->kick_wdt) 
		{
			g_wk_wdt->kick_wdt(WK_WDT_EXT_TYPE);// for KICK external wdt
		}
		else
		{
			printk("[WDK]g_wk_wdt or g_wd_wdt->kick_wdt is NULL");
		}
		
		printk("[WDK]:Stop CPU:%d\n", hotcpu);
		wk_stop_kick_cpu(hotcpu);
		printk("[WDK]cpu %d plug off, wk_stop_kick_cpu", hotcpu);
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
	.notifier_call = wk_cpu_callback
};

//WDK API
unsigned int get_check_bit(void)
{
   return wk_check_kick_bit();
}
unsigned int get_kick_bit(void)
{
   return kick_bit;
}
//END
static int __init init_wk(void)
{
	#ifdef __ENABLE_WDT_SYSFS__
	mtk_rgu_sysfs();
	#endif 

	#ifdef __ENABLE_WDT_AT_INIT__
	start_kicker_thread_with_default_setting();
	#endif

	wk_proc_init();	
	register_cpu_notifier(&cpu_nfb);

	return 0;
}

static void __exit exit_wk(void)
{
	wk_proc_exit();
	kthread_stop((struct task_struct*)wk_tsk);
}

late_initcall(init_wk);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mediatek inc.");
MODULE_DESCRIPTION("The watchdog kicker");
