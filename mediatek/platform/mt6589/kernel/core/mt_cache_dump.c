#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/trace_seq.h>
#include <linux/ftrace_event.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>

#include <linux/cpu.h>
#include <linux/smp.h>

extern void __inner_clean_dcache_L1();
extern void __inner_clean_dcache_L2();
extern void __enable_dcache();
extern void __disable_dcache();
extern void __enable_icache();
extern void __disable_icache();

void dump_data_cache_L1(){
    unsigned int cache_tag;
    unsigned int cache_level = 0;
    unsigned int cache_way;
    unsigned int cache_set;
    unsigned int elem_idx;
    unsigned int cache_data1,cache_data2,cache_data3;
    unsigned int tag_address;
    unsigned int sec;
    unsigned int tag_MOESI;
    unsigned int dirty_MOESI;
    unsigned int outer_memory_att;
    unsigned int data_cache_size = 64;
    unsigned int max_cache_level = 2;
    unsigned int max_cache_set = 0x80;
    unsigned int max_cache_way = 0x4;
    unsigned int tmp;

    __inner_clean_dcache_L1();
    __disable_dcache();
    printk("[CPU%] Dump L1 D cache...\n",smp_processor_id());
    printk("ADDRESS\tSEC\tSET\tWAY\tMOESI\t00\t04\t08\t0C\t10\t14\t18\t1C\t20\t24\t28\t2C\t30\t34\t38\t3C\n");
    for (cache_set = 0; cache_set < max_cache_set; cache_set++)
    {
        printk("cache_set:%d\n",cache_set);
        for (cache_way = 0; cache_way < max_cache_way; cache_way++)
        {
            printk("cache_way:%d\n",cache_way);
            cache_tag = (cache_way << 30) | (cache_set << 6);
            asm volatile(
              "MCR p15, 2, %0, c0, c0, 0\n"
              "MCR p15, 3, %1, c15, c2, 0\n"
              "MRC p15, 3, %2, c15, c0, 0\n"
              "MRC p15, 3, %3, c15, c0, 1\n"
              : "+r" (cache_level),"+r" (cache_tag),"+r" (cache_data1), "+r" (cache_data2)
              :
              : "cc"
              );
            tag_address = (cache_data2 & 0x1FFFFFFF) << 11;
            sec = (cache_data2 & 0x20000000)>>29;
            tag_MOESI = (cache_data2 & 0xC0000000)>>30;
            dirty_MOESI = cache_data1 & 0x3;
            outer_memory_att = cache_data1 & 0x1C;

            printk("%x\t %x\t %x\t %x\t %x:%x\t",tag_address+cache_set*data_cache_size,sec,cache_set,cache_way,tag_MOESI,dirty_MOESI);
            for (elem_idx = 0; elem_idx < 8; elem_idx++)
            {
                cache_tag = (cache_way << 30) | (cache_set << 6) | (elem_idx << 3);
                asm volatile(
                  "MCR p15, 2, %0, c0, c0, 0\n"
                  "MCR p15, 3, %1, c15, c4, 0\n"
                  "MRC p15, 3, %2, c15, c0, 0\n"
                  "MRC p15, 3, %3, c15, c0, 1\n"
                  : "+r" (cache_level),"+r" (cache_tag),"+r" (cache_data1), "+r" (cache_data2)

                  :
                  : "cc"
                  );
                printk("%x\t %x\t",cache_data1,cache_data2);
            }
            printk("\n");
        }
    }
    __enable_dcache();
}
void dump_data_cache_L2(){
    unsigned int cache_tag;
    unsigned int cache_level = 2;
    unsigned int cache_way;
    unsigned int cache_set;
    unsigned int elem_idx;
    unsigned int cache_data1,cache_data2,cache_data3;
    unsigned int tag_address;
    unsigned int sec;
    unsigned int tag_MOESI;
    unsigned int dirty_MOESI;
    unsigned int outer_memory_att;
    unsigned int data_cache_size = 64;
    unsigned int max_cache_level = 2;
    unsigned int max_cache_set = 0x800;
    unsigned int max_cache_way = 0x8;
    unsigned int tmp;

    __inner_clean_dcache_L2();
    __disable_dcache();
    printk("[CPU%] Dump L2 D cache...\n",smp_processor_id());
    printk("ADDRESS\tSEC\tSET\tWAY\tMOESI\t00\t04\t08\t0C\t10\t14\t18\t1C\t20\t24\t28\t2C\t30\t34\t38\t3C\n");

    for (cache_set = 0; cache_set < max_cache_set; cache_set++)
    {
        printk("cache_set:%d\n",cache_set);
        for (cache_way = 0; cache_way < max_cache_way; cache_way++)
        {
            printk("cache_way:%d\n",cache_way);
            cache_tag = (cache_way << 30) | (cache_set << 6);
            asm volatile(
              "MCR p15, 2, %0, c0, c0, 0\n"
              "MCR p15, 3, %1, c15, c2, 0\n"
              "MRC p15, 3, %2, c15, c0, 0\n"
              "MRC p15, 3, %3, c15, c0, 1\n"
              : "+r" (cache_level),"+r" (cache_tag),"+r" (cache_data1), "+r" (cache_data2)
              :
              : "cc"
              );
            tag_address = (cache_data2 & 0x1FFFFFFF) << 11;
            sec = (cache_data2 & 0x20000000)>>29;
            tag_MOESI = (cache_data2 & 0xC0000000)>>30;
            dirty_MOESI = cache_data1 & 0x3;
            outer_memory_att = cache_data1 & 0x1C;

            printk("%x\t %x\t %x\t %x\t %x:%x\t",tag_address+cache_set*data_cache_size,sec,cache_set,cache_way,tag_MOESI,dirty_MOESI);
            for (elem_idx = 0; elem_idx < 8; elem_idx++)
            {
                cache_tag = (cache_way << 30) | (cache_set << 6) | (elem_idx << 3);
                asm volatile(
                  "MCR p15, 2, %0, c0, c0, 0\n"
                  "MCR p15, 3, %1, c15, c4, 0\n"
                  "MRC p15, 3, %2, c15, c0, 0\n"
                  "MRC p15, 3, %3, c15, c0, 1\n"
                  : "+r" (cache_level),"+r" (cache_tag),"+r" (cache_data1), "+r" (cache_data2)

                  :
                  : "cc"
                  );
                printk("%x\t %x\t",cache_data1,cache_data2);
            }
            printk("\n");
        }
    }
    __enable_dcache();
}
void dump_inst_cache(){
    unsigned int cache_tag;
    unsigned int cache_level = 1;
    unsigned int cache_way;
    unsigned int cache_set;
    unsigned int elem_idx;
    unsigned int cache_data1,cache_data2,cache_data3;
    unsigned int valid;
    unsigned int tag_address;
    unsigned int sec;
    unsigned int arm_state;
    unsigned int inst_cache_size = 32;

    __disable_icache();
    printk("[CPU%] Dump L1 I cache...\n",smp_processor_id());
    printk("ADDRESS\tSEC\tSET\tWAY\tVALID\tarm_state\t00\t04\t08\t0C\t10\t14\t18\t1C\n");
    for (cache_set = 0; cache_set < 512; cache_set++)
    {
        for (cache_way = 0; cache_way < 2; cache_way++)
        {
            cache_tag = (cache_way << 31) | (cache_set << 5);
            asm volatile(
              "MCR p15, 2, %0, c0, c0, 0\n"
              "MCR p15, 3, %1, c15, c2, 1\n"
              "MRC p15, 3, %2, c15, c0, 0\n"
              "MRC p15, 3, %3, c15, c0, 1\n"
              "MRC p15, 3, %4, c15, c0, 2\n"
              : "+r" (cache_level),"+r" (cache_tag),"+r" (cache_data1) ,"+r" (cache_data2) ,"+r" (cache_data3)
              :
              : "cc"
              );
            valid = (cache_data1 & 0x40000000) >> 30;
            arm_state = (cache_data1 & 0x20000000) >> 29;
            sec = (cache_data1 & 0x10000000) >> 28;
            tag_address =  (cache_data1 & 0xFFFFFFF) << 12;
            if (!valid)
                continue;
            printk("%x\t%x\t%x\t%x\t%x\t%x\t",tag_address+cache_set*inst_cache_size,sec,cache_set,cache_way,valid,arm_state);
            for (elem_idx = 0; elem_idx < 8; elem_idx++)
            {
                cache_tag = (cache_way << 31) | (cache_set << 5) | (elem_idx << 2);
                asm volatile(
                  "MCR p15, 2, %0, c0, c0, 0\n"
                  "MCR p15, 3, %1, c15, c4, 1\n"
                  "MRC p15, 3, %2, c15, c0, 0\n"
                  "MRC p15, 3, %3, c15, c0, 1\n"
                  : "+r" (cache_level),"+r" (cache_tag),"+r" (cache_data1), "+r" (cache_data2)
                  :
                  : "cc"
                  );
                cache_data1 &= ~(0x3 << 16);
                cache_data2 &= ~(0x3 << 16);
                printk("%x%x\t",cache_data1,cache_data2);
            }
            printk("\n");
        }
    }
    __enable_icache();
}


/*
 * 
 * Return 0.
 */
int mt_cache_dump_init(void)
{

    return 0;
}

/*
 * mt_cache_dump_deinit: 
 * Return 0.
 */
int mt_cache_dump_deinit(void)
{
    return 0;
}

static ssize_t cache_dump_store(struct device_driver *driver, const char *buf,
                              size_t count)
{
        char *p = (char *)buf;
        unsigned int num;

        num = simple_strtoul(p, &p, 10);
        get_online_cpus();
        switch(num)
        {
            case 1:
                    on_each_cpu(dump_data_cache_L1, NULL, true);
                    dump_data_cache_L2();
                    break;
            case 2:
                    on_each_cpu(dump_inst_cache, NULL, true);
                    break;
            case 3:
                    on_each_cpu(dump_inst_cache, NULL, true);
                    on_each_cpu(dump_data_cache_L1, NULL, true);
                    dump_data_cache_L2();
                    break;
        }
        put_online_cpus();
        return count;
}
static ssize_t cache_dump_show(struct device_driver *driver, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "1:dump L1 D cache and L2 unified cache\n");

}

DRIVER_ATTR(cache_dump, 0644, cache_dump_show, cache_dump_store);

static struct device_driver mt_cache_dump_drv = 
{
    .name = "mt_cache_dump",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};



/*
 * mt_mon_mod_init: module init function
 */
static int __init mt_cache_dump_mod_init(void)
{
    int ret;
	
    /* register driver and create sysfs files */
    ret = driver_register(&mt_cache_dump_drv);

    if (ret) {
        printk("fail to register mt_cache_dump_drv\n");
        return ret;
    }
    ret = driver_create_file(&mt_cache_dump_drv, &driver_attr_cache_dump);

    if (ret) {
        printk("fail to create mt_cache_dump sysfs files\n");

        return ret;
    }


    return 0;
}

/*
 * mt_cache_dump_mod_exit: module exit function
 */
static void __exit mt_cache_dump_mod_exit(void)
{
}
module_init(mt_cache_dump_mod_init);
module_exit(mt_cache_dump_mod_exit);

