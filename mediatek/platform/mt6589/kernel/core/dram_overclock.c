#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <asm/cacheflush.h>
#include <asm/outercache.h>
#include <asm/system.h>
#include <asm/delay.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_freqhopping.h>
#include <mach/emi_bwl.h>
#include <mach/mt_typedefs.h>
#include <mach/memory.h>

#define DRAMC_WRITE_REG(val,offset)     do{ \
                                      (*(volatile unsigned int *)(DRAMC0_BASE + (offset))) = (unsigned int)(val); \
                                      (*(volatile unsigned int *)(DDRPHY_BASE + (offset))) = (unsigned int)(val); \
                                      (*(volatile unsigned int *)(DRAMC_NAO_BASE + (offset))) = (unsigned int)(val); \
                                      }while(0)

static int dram_clk;
static DEFINE_SPINLOCK(lock);

__attribute__ ((__section__ (".sram.func"))) int sram_set_dram(int clk)
{
    /* set ac timing */
    if(clk == 293) {
        DRAMC_WRITE_REG( 0x778844D5     , 0x0  );
        DRAMC_WRITE_REG( 0xC0064301     , 0x7C );
        DRAMC_WRITE_REG( 0x9F0C8CA0     , 0x44 );
        DRAMC_WRITE_REG( 0x03406348     , 0x8  );
        DRAMC_WRITE_REG( 0x11662742     , 0x1DC);
        DRAMC_WRITE_REG( 0x01001010     , 0x1E8);
        DRAMC_WRITE_REG( 0x17000000     , 0xFC );
        udelay(10); 
    }
    return 0;
}
static void enable_gpu(void)
{
    enable_clock(MT_CG_MFG_HYD, "MFG");
    enable_clock(MT_CG_MFG_G3D, "MFG");
    enable_clock(MT_CG_MFG_MEM, "MFG");
    enable_clock(MT_CG_MFG_AXI, "MFG");
}
static void disable_gpu(void)
{
    disable_clock(MT_CG_MFG_AXI, "MFG");
    disable_clock(MT_CG_MFG_MEM, "MFG");
    disable_clock(MT_CG_MFG_G3D, "MFG");
    disable_clock(MT_CG_MFG_HYD, "MFG");
}
static ssize_t dram_overclock_show(struct device_driver *driver, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%d,%d\n",get_ddr_type(), mt_fh_get_dramc());
}
static ssize_t dram_overclock_store(struct device_driver *driver, const char *buf, size_t count)
{
    int clk, ret = 0;
   
    clk = simple_strtol(buf, 0, 10);
    dram_clk = mt_fh_get_dramc();
    if(clk == dram_clk) {
        printk(KERN_ERR "dram_clk:%d, is equal to user inpu clk:%d\n", dram_clk, clk);
        return count;
    }
    spin_lock(&lock);
    ret = sram_set_dram(clk);
    if(ret < 0)
        printk(KERN_ERR "dram overclock in sram failed:%d, clk:%d\n", ret, clk);
    spin_unlock(&lock);
    ret = mt_fh_dram_overclock(clk);
    if(ret < 0)
        printk(KERN_ERR "dram overclock failed:%d, clk:%d\n", ret, clk);
    printk(KERN_INFO "In %s pass, dram_clk:%d, clk:%d\n", __func__, dram_clk, clk);
    return count;
}
extern unsigned int RESERVED_MEM_SIZE_FOR_TEST_3D;
extern unsigned int FB_SIZE_EXTERN;
extern unsigned int get_max_DRAM_size (void);
static ssize_t ftm_dram_3d_show(struct device_driver *driver, char *buf)
{
    unsigned int pa_3d_base = PHYS_OFFSET + get_max_DRAM_size() - RESERVED_MEM_SIZE_FOR_TEST_3D - FB_SIZE_EXTERN;
    return snprintf(buf, PAGE_SIZE, "%u\n", pa_3d_base);
}
static ssize_t ftm_dram_3d_store(struct device_driver *driver, const char *buf, size_t count)
{
    return count;
}
static ssize_t ftm_dram_mtcmos_show(struct device_driver *driver, char *buf)
{
    return 0;
}
static ssize_t ftm_dram_mtcmos_store(struct device_driver *driver, const char *buf, size_t count)
{
    int enable;
    enable = simple_strtol(buf, 0, 10);
    if(enable == 1) {
        enable_gpu();
        printk(KERN_INFO "enable in %s, enable:%d\n", __func__, enable);
    } else if(enable == 0) {
        disable_gpu();
        printk(KERN_INFO "enable in %s, disable:%d\n", __func__, enable);
    } else
        printk(KERN_ERR "dram overclock failed:%s, enable:%d\n", __func__, enable);
    return count;
}

DRIVER_ATTR(emi_clk_test, 0664, dram_overclock_show, dram_overclock_store);
DRIVER_ATTR(emi_clk_3d_test, 0664, ftm_dram_3d_show, ftm_dram_3d_store);
DRIVER_ATTR(emi_clk_mtcmos_test, 0664, ftm_dram_mtcmos_show, ftm_dram_mtcmos_store);

static struct device_driver dram_overclock_drv =
{
    .name = "emi_clk_test",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

extern char __ssram_text, _sram_start, __esram_text;
int __init dram_overclock_init(void)
{
    int ret;
    unsigned char * dst = &__ssram_text;
    unsigned char * src =  &_sram_start;

    ret = driver_register(&dram_overclock_drv);
    if (ret) {
        printk(KERN_ERR "fail to create the dram_overclock driver\n");
        return ret;
    }
    ret = driver_create_file(&dram_overclock_drv, &driver_attr_emi_clk_test);
    if (ret) {
        printk(KERN_ERR "fail to create the dram_overclock sysfs files\n");
        return ret;
    }
    ret = driver_create_file(&dram_overclock_drv, &driver_attr_emi_clk_3d_test);
    if (ret) {
        printk(KERN_ERR "fail to create the ftm_dram_3d_drv sysfs files\n");
        return ret;
    }
    ret = driver_create_file(&dram_overclock_drv, &driver_attr_emi_clk_mtcmos_test);
    if (ret) {
        printk(KERN_ERR "fail to create the ftm_dram_mtcmos_drv sysfs files\n");
        return ret;
    }

    for (dst = &__ssram_text ; dst < (unsigned char *)&__esram_text ; dst++,src++) {
        *dst = *src;
    }

    return 0;
}

arch_initcall(dram_overclock_init);
