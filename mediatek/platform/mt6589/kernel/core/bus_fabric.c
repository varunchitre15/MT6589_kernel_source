#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/list.h>

#include <mach/mt_reg_base.h>
#include <mach/sync_write.h>
#include <mach/bus_fabric.h>

void ap_md_bus_config(void)
{
  unsigned int top_axi_bus_ctrl;
  
  top_axi_bus_ctrl = readl(TOP_AXI_BUS_CTRL);
  top_axi_bus_ctrl &= 0xFFFFF5AD;
  writel(top_axi_bus_ctrl, TOP_AXI_BUS_CTRL);  
}

EXPORT_SYMBOL(ap_md_bus_config);
