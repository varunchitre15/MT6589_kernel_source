#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <mach/mt_reg_base.h>
#include "mach/sync_write.h"
#include "mach/mt_boot.h"

struct mt_dsizer_driver {

        struct device_driver driver;
        const struct platform_device_id *id_table;
};

static struct mt_dsizer_driver mt_dsizer_drv = {
    .driver = {
	.name = "dsizer",
	.bus = &platform_bus_type,
	.owner = THIS_MODULE,
    },
    .id_table= NULL,
};

/*
 * dsizer_show: To show usage.
 */
static ssize_t dsizer_show(struct device_driver *driver, char *buf)
{
        unsigned int val;
        val = readl(0xF0001200);
        if (val & 0x1)
            return snprintf(buf, PAGE_SIZE, "outstanding disable\n");
        else
            return snprintf(buf, PAGE_SIZE, "outstanding enable\n");
}

/*
 * dsizer_store: To select dsizer test case.
 */
static ssize_t dsizer_store(struct device_driver *driver, const char *buf,
			      size_t count)
{
	char *p = (char *)buf;
	unsigned int num;
        unsigned int val;
        unsigned int ooo_addr = 0xF0001200;
        val = readl(ooo_addr);
	num = simple_strtoul(p, &p, 10);
        switch(num){
            case 0:
                val &= ~(0x1);
                mt65xx_reg_sync_writel(val, ooo_addr);
                break;
            case 1:
                val |= 0x1;
                mt65xx_reg_sync_writel(val, ooo_addr);
                break;
            default:
                break;
        }

	return count;
}
DRIVER_ATTR(dsizer, 0664, dsizer_show, dsizer_store);
int mt_dsizer_init(void){
        int ret;
        unsigned int val;
        unsigned int ooo_addr = 0xF0001200;
        CHIP_SW_VER ver = mt_get_chip_sw_ver();
        val = readl(ooo_addr);
        printk("[DSI] MCI_Downsizer init\n");
        if (CHIP_SW_VER_02 > ver) 
        {
            printk("[DSI] Enable Downsizer \n");
            val |= (0x1);
        }
        mt65xx_reg_sync_writel(val, ooo_addr);
        ret = driver_register(&mt_dsizer_drv.driver);
	ret = driver_create_file(&mt_dsizer_drv.driver, &driver_attr_dsizer);
        if (ret == 0)
            printk("[DSI] MCI_Downsizer init done.\n");
	return 0;

}
arch_initcall(mt_dsizer_init);
