#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>

#define SEQ_printf(m, x...)	    \
 do {			    \
    if (m)		    \
	seq_printf(m, x);	\
    else		    \
	printk(x);	    \
 } while (0)


#define MTSCHED_DEBUG_ENTRY(name) \
static int mt_##name##_open(struct inode *inode, struct file *file) \
{ \
    return single_open(file, mt_##name##_show, inode->i_private); \
} \
\
static const struct file_operations mt_##name##_fops = { \
    .open = mt_##name##_open, \
    .write = mt_##name##_write,\
    .read = seq_read, \
    .llseek = seq_lseek, \
    .release = single_release, \
}

#define BOOT_STR_SIZE 128
#define BOOT_LOG_NUM 64
struct boot_log_struct{
    u64 timestamp;
    char event[BOOT_STR_SIZE];
}mt_bootprof[BOOT_LOG_NUM];
int boot_log_count = 0;

static DEFINE_MUTEX(mt_bootprof_lock);
static int mt_bootprof_enabled = 0;
static int pl_t = 0, lk_t = 0;

/*
 * Ease the printing of nsec fields:
 */
static long long nsec_high(unsigned long long nsec)
{
    if ((long long)nsec < 0) {
	nsec = -nsec;
	do_div(nsec, 1000000);
	return -nsec;
    }
    do_div(nsec, 1000000);

    return nsec;
}

static unsigned long nsec_low(unsigned long long nsec)
{
    if ((long long)nsec < 0)
	nsec = -nsec;

    return do_div(nsec, 1000000);
}
#define SPLIT_NS(x) nsec_high(x), nsec_low(x)

void log_boot(char *str)
{
    unsigned long long ts;

    if( 0 == mt_bootprof_enabled)
	return;
    ts = sched_clock();
	printk("BOOTPROF:%10Ld.%06ld:%s\n", SPLIT_NS(ts), str);
    if(boot_log_count >= BOOT_LOG_NUM)
	{
		printk("[BOOTPROF] not enuough bootprof buffer\n");
		return;
    }
    mutex_lock(&mt_bootprof_lock);
    mt_bootprof[boot_log_count].timestamp = ts;
    strcpy((char*)&mt_bootprof[boot_log_count].event, str);
    boot_log_count++;
    mutex_unlock(&mt_bootprof_lock);
}


//extern void (*set_intact_mode)(void);
static void mt_bootprof_switch(int on)
{
    mutex_lock(&mt_bootprof_lock);
    if (mt_bootprof_enabled ^ on)
	{
		if (on)
		{
		    mt_bootprof_enabled = 1;
		}
		else
		{
		    mt_bootprof_enabled = 0;		
		}
    }
    mutex_unlock(&mt_bootprof_lock);

}

static ssize_t mt_bootprof_write(struct file *filp, const char *ubuf,
	   size_t cnt, loff_t *data)
{
    char buf[BOOT_STR_SIZE];
    size_t copy_size = cnt;

    if (cnt >= sizeof(buf))
	copy_size = BOOT_STR_SIZE - 1;

    if (copy_from_user(&buf, ubuf, copy_size))
	return -EFAULT;

    if(cnt==1){
	if(buf[0] == '0')
	    mt_bootprof_switch(0);
	else if(buf[0] == '1')
	    mt_bootprof_switch(1);
    }

    buf[copy_size] = 0;
    log_boot(buf);

    return cnt;

}
static int mt_bootprof_show(struct seq_file *m, void *v)
{
    int i;
    SEQ_printf(m, "----------------------------------------\n");
    SEQ_printf(m, "%d	    BOOT PROF (unit:msec)\n", mt_bootprof_enabled);
    SEQ_printf(m, "----------------------------------------\n");

    if (pl_t > 0 && lk_t > 0) {
        SEQ_printf(m, "%10d        : %s\n", pl_t, "preloader");
        SEQ_printf(m, "%10d        : %s\n", lk_t, "lk");
        SEQ_printf(m, "----------------------------------------\n");
    }

    for(i = 0 ;i< boot_log_count;i ++){
	SEQ_printf(m, "%10Ld.%06ld : %s\n", SPLIT_NS(mt_bootprof[i].timestamp), mt_bootprof[i].event);
    }
    SEQ_printf(m, "----------------------------------------\n");
    return 0;
}
/*** Seq operation of mtprof ****/
//MTSCHED_DEBUG_ENTRY(bootprof); 
static int mt_bootprof_open(struct inode *inode, struct file *file) 
{ 
    return single_open(file, mt_bootprof_show, inode->i_private); 
} 

static int __init setup_pl_t(char *str)
{
    pl_t = simple_strtol(str, NULL, 10);
    return 1;
}

__setup("pl_t=", setup_pl_t);

static int __init setup_lk_t(char *str)
{
    lk_t = simple_strtol(str, NULL, 10);
	return 1;
}

__setup("lk_t=", setup_lk_t);

static const struct file_operations mt_bootprof_fops = { 
    .open = mt_bootprof_open, 
    .write = mt_bootprof_write,
    .read = seq_read, 
    .llseek = seq_lseek, 
    .release = single_release, 
};
static int __init init_boot_prof(void)
{
    struct proc_dir_entry *pe;

    pe = proc_create("bootprof", 0664, NULL, &mt_bootprof_fops);
    if (!pe)
	return -ENOMEM;
   // set_intact_mode = NULL;
    mt_bootprof_switch(1);
    return 0;
}
__initcall(init_boot_prof);
