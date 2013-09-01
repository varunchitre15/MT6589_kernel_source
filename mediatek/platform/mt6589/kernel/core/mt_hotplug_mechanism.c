/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*********************************
* include
**********************************/
#include <linux/kernel.h> 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/earlysuspend.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <mach/hotplug.h>
#include <mach/sync_write.h>



/*********************************
* macro
**********************************/
#ifdef CONFIG_HAS_EARLYSUSPEND

#define STATE_INIT                  0
#define STATE_ENTER_EARLY_SUSPEND   1
#define STATE_ENTER_LATE_RESUME     2

#endif //#ifdef CONFIG_HAS_EARLYSUSPEND



/*********************************
* glabal variable
**********************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
static int g_enable = 1;
static struct early_suspend mt_hotplug_mechanism_early_suspend_handler =
{
    .level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 250,
    .suspend = NULL,
    .resume  = NULL,
};
static int g_cur_state = STATE_ENTER_LATE_RESUME;
#endif //#ifdef CONFIG_HAS_EARLYSUSPEND
static int g_test0 = 0;
static int g_test1 = 0;



/*********************************
* extern function
**********************************/
extern void hp_disable_cpu_hp(int disable);
extern struct mutex hp_onoff_mutex;



/*********************************
* early suspend callback function
**********************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void mt_hotplug_mechanism_early_suspend(struct early_suspend *h)
{
    HOTPLUG_INFO("mt_hotplug_mechanism_early_suspend");

    if (g_enable)
    {
        int i = 0;
        
        mutex_lock(&hp_onoff_mutex);
        
        hp_disable_cpu_hp(1);

        for (i = (num_possible_cpus() - 1); i > 0; i--)
        {
            if (cpu_online(i))
                cpu_down(i);
        }
        
        mutex_unlock(&hp_onoff_mutex);
    }

    g_cur_state = STATE_ENTER_EARLY_SUSPEND;

    return;
}
#endif //#ifdef CONFIG_HAS_EARLYSUSPEND



/*******************************
* late resume callback function
********************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void mt_hotplug_mechanism_late_resume(struct early_suspend *h)
{
    HOTPLUG_INFO("mt_hotplug_mechanism_late_resume");

    if (g_enable)
    {
        hp_disable_cpu_hp(0);
    }

    g_cur_state = STATE_ENTER_LATE_RESUME;

    return;
}
#endif //#ifdef CONFIG_HAS_EARLYSUSPEND



/**************************************************************
* mt hotplug mechanism control interface for procfs test0
***************************************************************/
static int mt_hotplug_mechanism_read_test0(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    char *p = buf;
    
    p += sprintf(p, "%d\n", g_test0);
    *eof = 1;
    
    HOTPLUG_INFO("mt_hotplug_mechanism_read_test0, hotplug_cpu_count: %d\n", atomic_read(&hotplug_cpu_count));
    on_each_cpu((smp_call_func_t)dump_stack, NULL, 1);
    
    mt65xx_reg_sync_writel(8, 0xf0200080);
    printk(KERN_EMERG "CPU%u, debug event: 0x%08x, debug monitor: 0x%08x\n", 0, *(volatile u32 *)(0xf0200080), *(volatile u32 *)(0xf0200084));
    mt65xx_reg_sync_writel(9, 0xf0200080);
    printk(KERN_EMERG "CPU%u, debug event: 0x%08x, debug monitor: 0x%08x\n", 1, *(volatile u32 *)(0xf0200080), *(volatile u32 *)(0xf0200084));
    mt65xx_reg_sync_writel(10, 0xf0200080);
    printk(KERN_EMERG "CPU%u, debug event: 0x%08x, debug monitor: 0x%08x\n", 2, *(volatile u32 *)(0xf0200080), *(volatile u32 *)(0xf0200084));
    mt65xx_reg_sync_writel(11, 0xf0200080);
    printk(KERN_EMERG "CPU%u, debug event: 0x%08x, debug monitor: 0x%08x\n", 3, *(volatile u32 *)(0xf0200080), *(volatile u32 *)(0xf0200084));
    
    return p - buf;
}

static int mt_hotplug_mechanism_write_test0(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0, test0 = 0;
    char desc[32];
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    if (sscanf(desc, "%d", &test0) == 1)
    {
        g_test0 = test0;
        return count;
    }
    else
    {
        HOTPLUG_INFO("mt_hotplug_mechanism_write_test0, bad argument\n");
    }
    
    return -EINVAL;
}



/**************************************************************
* mt hotplug mechanism control interface for procfs test1
***************************************************************/
static int mt_hotplug_mechanism_read_test1(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    char *p = buf;
    
    p += sprintf(p, "%d\n", g_test1);
    *eof = 1;
    
    return p - buf;
}

static int mt_hotplug_mechanism_write_test1(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0, test1 = 0;
    char desc[32];
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    if (sscanf(desc, "%d", &test1) == 1)
    {
        g_test1 = test1;
        return count;
    }
    else
    {
        HOTPLUG_INFO("mt_hotplug_mechanism_write_test1, bad argument\n");
    }
    
    return -EINVAL;
}



/*******************************
* kernel module init function
********************************/
static int __init mt_hotplug_mechanism_init(void)
{
    struct proc_dir_entry *entry = NULL;
    struct proc_dir_entry *mt_hotplug_test_dir = NULL;
    
    HOTPLUG_INFO("mt_hotplug_mechanism_init");
    
    mt_hotplug_test_dir = proc_mkdir("mt_hotplug_test", NULL);
    if (!mt_hotplug_test_dir)
    {
        HOTPLUG_INFO("mkdir /proc/mt_hotplug_test failed");
    }
    else
    {
        entry = create_proc_entry("test0", S_IRUGO | S_IWUSR, mt_hotplug_test_dir);
        if (entry)
        {
            entry->read_proc = mt_hotplug_mechanism_read_test0;
            entry->write_proc = mt_hotplug_mechanism_write_test0;
        }
        entry = create_proc_entry("test1", S_IRUGO | S_IWUSR, mt_hotplug_test_dir);
        if (entry)
        {
            entry->read_proc = mt_hotplug_mechanism_read_test1;
            entry->write_proc = mt_hotplug_mechanism_write_test1;
        }
    }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
    mt_hotplug_mechanism_early_suspend_handler.suspend = mt_hotplug_mechanism_early_suspend;
    mt_hotplug_mechanism_early_suspend_handler.resume = mt_hotplug_mechanism_late_resume;
    register_early_suspend(&mt_hotplug_mechanism_early_suspend_handler);
#endif //#ifdef CONFIG_HAS_EARLYSUSPEND

    return 0;
}
module_init(mt_hotplug_mechanism_init);



/*******************************
* kernel module exit function
********************************/
static void __exit mt_hotplug_mechanism_exit(void)
{
    HOTPLUG_INFO("mt_hotplug_mechanism_exit");
}
module_exit(mt_hotplug_mechanism_exit);



/**************************************************************
* mt hotplug mechanism control interface for thermal protect
***************************************************************/
void mt_hotplug_mechanism_thermal_protect(int limited_cpus)
{
    HOTPLUG_INFO("mt_hotplug_mechanism_thermal_protect\n");

}
EXPORT_SYMBOL(mt_hotplug_mechanism_thermal_protect);



#ifdef CONFIG_HAS_EARLYSUSPEND
module_param(g_enable, int, 0644);
#endif //#ifdef CONFIG_HAS_EARLYSUSPEND



MODULE_DESCRIPTION("MediaTek CPU Hotplug Mechanism");
MODULE_LICENSE("GPL");