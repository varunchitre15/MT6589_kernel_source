#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>
#include <linux/module.h>
#include <asm/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gfp.h>
#include <asm/io.h>
#include <asm/memory.h>
#include <asm/outercache.h>
#include <linux/spinlock.h>

#include <linux/leds-mt65xx.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include "slt.h"
#include <linux/io.h>
#include <asm/pgtable.h>

extern int fp4_vfp_func_start(int cpu_id);

static int g_iCPU0_PassFail, g_iCPU1_PassFail, g_iCPU2_PassFail, g_iCPU3_PassFail;
static int g_iVfpLoopCount;

static struct device_driver slt_cpu0_vfp_drv = 
{   
    .name = "slt_cpu0_vfp",    
    .bus = &platform_bus_type,  
    .owner = THIS_MODULE,   
};

static struct device_driver slt_cpu1_vfp_drv = 
{   
    .name = "slt_cpu1_vfp",    
    .bus = &platform_bus_type,  
    .owner = THIS_MODULE,   
};

static struct device_driver slt_cpu2_vfp_drv = 
{   
    .name = "slt_cpu2_vfp",    
    .bus = &platform_bus_type,  
    .owner = THIS_MODULE,   
};

static struct device_driver slt_cpu3_vfp_drv = 
{   
    .name = "slt_cpu3_vfp",    
    .bus = &platform_bus_type,  
    .owner = THIS_MODULE,   
};

static struct device_driver slt_vfp_loop_count_drv =
{
    .name = "slt_vfp_loop_count",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

#define DEFINE_SLT_CPU_VFP_SHOW(_N)    \
static ssize_t slt_cpu##_N##_vfp_show(struct device_driver *driver, char *buf) \
{   \
    if(g_iCPU##_N##_PassFail == -1) \
        return snprintf(buf, PAGE_SIZE, "CPU%d VFP - CPU%d is powered off\n",_N,_N); \
    else    \
        return snprintf(buf, PAGE_SIZE, "CPU%d VFP - %s(loop_count = %d)\n", _N, g_iCPU##_N##_PassFail != g_iVfpLoopCount ? "FAIL" : "PASS", g_iCPU##_N##_PassFail);  \
}

DEFINE_SLT_CPU_VFP_SHOW(0)
DEFINE_SLT_CPU_VFP_SHOW(1)
DEFINE_SLT_CPU_VFP_SHOW(2)
DEFINE_SLT_CPU_VFP_SHOW(3)

static ssize_t slt_vfp_loop_count_show(struct device_driver *driver, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "VFP Test Loop Count = %d\n", g_iVfpLoopCount);
}

#define DEFINE_SLT_CPU_VFP_STORE(_N)    \
static ssize_t slt_cpu##_N##_vfp_store(struct device_driver *driver, const char *buf, size_t count)    \
{   \
    unsigned int i, ret;    \
    unsigned long mask; \
    int retry=0;    \
    DEFINE_SPINLOCK(cpu##_N##_lock);    \
    unsigned long cpu##_N##_flags;     \
    \
    g_iCPU##_N##_PassFail = 0;  \
    \
    mask = (1 << _N); /* processor _N */ \
    while(sched_setaffinity(0, (struct cpumask*) &mask) < 0)    \
    {   \
        printk("Could not set cpu%d affinity for current process(%d).\n", _N, retry);  \
        g_iCPU##_N##_PassFail = -1; \
        retry++;    \
        if(retry > 100) \
        {   \
            return count;   \
        }   \
    }   \
    \
    printk("\n>> CPU%d vfp test start (cpu id = %d) <<\n\n", _N, raw_smp_processor_id());  \
    \
    for (i = 0, g_iCPU##_N##_PassFail = 0; i < g_iVfpLoopCount; i++) {    \
        spin_lock_irqsave(&cpu##_N##_lock, cpu##_N##_flags);    \
        ret = fp4_vfp_func_start(_N);    /* 1: PASS, 0:Fail, -1: target CPU power off */  \
        spin_unlock_irqrestore(&cpu##_N##_lock, cpu##_N##_flags);   \
        if(ret != -1)   \
        {   \
             g_iCPU##_N##_PassFail += ret;  \
        }   \
        else    \
        {   \
             g_iCPU##_N##_PassFail = -1;    \
             break; \
        }   \
    }   \
    \
    if (g_iCPU##_N##_PassFail == g_iVfpLoopCount) {    \
        printk("\n>> CPU%d vfp test pass <<\n\n", _N); \
    }else { \
        printk("\n>> CPU%d vfp test fail (loop count = %d)<<\n\n", _N, g_iCPU##_N##_PassFail);  \
    }   \
    \
    return count;   \
}

DEFINE_SLT_CPU_VFP_STORE(0)
DEFINE_SLT_CPU_VFP_STORE(1)
DEFINE_SLT_CPU_VFP_STORE(2)
DEFINE_SLT_CPU_VFP_STORE(3)

static ssize_t slt_vfp_loop_count_store(struct device_driver *driver, const char *buf, size_t count)
{ 
    int result;
     
    if ((result = sscanf(buf, "%d", &g_iVfpLoopCount)) == 1)
    {
        printk("set SLT vfp test loop count = %d successfully\n", g_iVfpLoopCount);
    }
    else
    {
        printk("bad argument!!\n");
        return -EINVAL;
    }
     
    return count;
}
    
DRIVER_ATTR(slt_cpu0_vfp, 0644, slt_cpu0_vfp_show, slt_cpu0_vfp_store);
DRIVER_ATTR(slt_cpu1_vfp, 0644, slt_cpu1_vfp_show, slt_cpu1_vfp_store);
DRIVER_ATTR(slt_cpu2_vfp, 0644, slt_cpu2_vfp_show, slt_cpu2_vfp_store);
DRIVER_ATTR(slt_cpu3_vfp, 0644, slt_cpu3_vfp_show, slt_cpu3_vfp_store);
DRIVER_ATTR(slt_vfp_loop_count, 0644, slt_vfp_loop_count_show, slt_vfp_loop_count_store);

#define DEFINE_SLT_CPU_VFP_INIT(_N)    \
int __init slt_cpu##_N##_vfp_init(void) \
{   \
    int ret;    \
    \
    ret = driver_register(&slt_cpu##_N##_vfp_drv);  \
    if (ret) {  \
        printk("fail to create SLT CPU%d vfp driver\n",_N);    \
    }   \
    else    \
    {   \
        printk("success to create SLT CPU%d vfp driver\n",_N); \
    }   \
    \
    ret = driver_create_file(&slt_cpu##_N##_vfp_drv, &driver_attr_slt_cpu##_N##_vfp);   \
    if (ret) {  \
        printk("fail to create SLT CPU%d vfp sysfs files\n",_N);   \
    }   \
    else    \
    {   \
        printk("success to create SLT CPU%d vfp sysfs files\n",_N);    \
    }   \
    \
    return 0;   \
}

DEFINE_SLT_CPU_VFP_INIT(0)
DEFINE_SLT_CPU_VFP_INIT(1)
DEFINE_SLT_CPU_VFP_INIT(2)
DEFINE_SLT_CPU_VFP_INIT(3)

int __init slt_vfp_loop_count_init(void)
{
    int ret;

    ret = driver_register(&slt_vfp_loop_count_drv);
    if (ret) {
        printk("fail to create vfp loop count driver\n");
    }
    else
    {
        printk("success to create vfp loop count driver\n");
    }
    

    ret = driver_create_file(&slt_vfp_loop_count_drv, &driver_attr_slt_vfp_loop_count);
    if (ret) {
        printk("fail to create vfp loop count sysfs files\n");
    }
    else
    {
        printk("success to create vfp loop count sysfs files\n");
    }

    g_iVfpLoopCount = SLT_LOOP_CNT;
    
    return 0;
}
arch_initcall(slt_cpu0_vfp_init);
arch_initcall(slt_cpu1_vfp_init);
arch_initcall(slt_cpu2_vfp_init);
arch_initcall(slt_cpu3_vfp_init);
arch_initcall(slt_vfp_loop_count_init);
