#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <mach/pmic_mt6320_sw.h>

extern u32 PTP_INIT_01_API(void);

static ssize_t track_vsram_show(struct device_driver *driver, char *buf)
{
    return 0;
}

static ssize_t track_vsram_store(struct device_driver *driver, const char *buf, size_t count)
{
    int ret;

    /*
     * NoteXXX: Set PMIC registers below to start to track Vsram.
     *          The usage of pmic_config_interface() is:
     *          pmic_config_interface(address, value , mask, shift)
     */

    ret = pmic_config_interface(0x216, 0x8, 0x7f, 8);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x216, 0x8, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x23c, 0x8, 0x7f, 8);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x23c, 0x8, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x268, 0x8, 0x7f, 8);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x268, 0x8, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x216, 0x1, 0x1, 15);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x216, 0x1, 0x1, 7);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x23c, 0x1, 0x1, 15);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x23c, 0x1, 0x1, 7);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x268, 0x1, 0x1, 15);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x268, 0x1, 0x1, 7);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x252, 0x2, 0x7f, 8);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x252, 0x2, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x254, 0x60, 0x7f, 8);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x254, 0x38, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x256, 0x18, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x210, 0x1, 0x1, 4);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x210, 0x1, 0x1, 1);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x236, 0x1, 0x1, 5);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x236, 0x1, 0x1, 4);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x236, 0x1, 0x1, 1);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x21a, 0x58, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x21c, 0x18, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x240, 0x58, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x242, 0x38, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x26c, 0x38, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x26e, 0x18, 0x7f, 0);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x212, 0x0, 0x7, 4);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x210, 0x1, 0x1, 1);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x238, 0x0, 0x7, 4);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x236, 0x1, 0x1, 1);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x264, 0x0, 0x7, 4);
    if (ret) {
        printk("Fail to set PMIC\n");
    }
    ret = pmic_config_interface(0x262, 0x1, 0x1, 1);
    if (ret) {
        printk("Fail to set PMIC\n");
    }

    return count;
}

DRIVER_ATTR(track_vsram, 0644, track_vsram_show, track_vsram_store);

static ssize_t ptp_od_show(struct device_driver *driver, char *buf)
{
    volatile u32 *clc_temp_p;
    u32 val;

    clc_temp_p = (volatile u32 *)PTP_INIT_01_API();

    /* only need bit 31 ~ bit 16 of the read data*/
    val = clc_temp_p[0];
    val >>= 16;
    val &= 0x0000FFFF;

    return snprintf(buf, PAGE_SIZE, "0x%x\n", val);
}

static ssize_t ptp_od_store(struct device_driver *driver, const char *buf, size_t count)
{
    return count;
}

DRIVER_ATTR(ptp_od, 0644, ptp_od_show, ptp_od_store);

static ssize_t chip_id_show(struct device_driver *driver, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "0x%x\n", *(volatile u32 *)0xF0009070);
}

static ssize_t chip_id_store(struct device_driver *driver, const char *buf, size_t count)
{
    return count;
}

DRIVER_ATTR(chip_id, 0644, chip_id_show, chip_id_store);

struct mssv_driver
{
    struct device_driver driver;
    const struct platform_device_id *id_table;
};

static struct mssv_driver mssv_driver =
{
    .driver =
    {
        .name = "mssv",
        .bus = &platform_bus_type,
        .owner = THIS_MODULE,
    },
    .id_table = NULL,
};

int mssv_init(void)
{
    int ret;

    ret = driver_register(&mssv_driver.driver);
    if (ret) {
        printk("Fail to register mssv_driver\n");
        return ret;
    }

    ret = driver_create_file(&mssv_driver.driver, &driver_attr_track_vsram);
    if (ret) {
        printk("Fail to create mssv_driver sysfs files\n");
        return ret;
    }

    ret = driver_create_file(&mssv_driver.driver, &driver_attr_ptp_od);
    if (ret) {
        printk("Fail to create mssv_driver sysfs files\n");
        return ret;
    }

    ret = driver_create_file(&mssv_driver.driver, &driver_attr_chip_id);
    if (ret) {
        printk("Fail to create mssv_driver sysfs files\n");
        return ret;
    }

    return 0;
}

arch_initcall(mssv_init);
