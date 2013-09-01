#ifndef __MATV_H__
#define __MATV_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>

#include "cust_matv.h"



#define MATV_DEVNAME "MATV"
#define MATV_IOC_MAGIC    'a'

//below is control message
#define TEST_MATV_PRINT         _IO(MATV_IOC_MAGIC,  0x00)
#define MATV_READ               _IOW(MATV_IOC_MAGIC, 0x01, unsigned int)
#define MATV_WRITE              _IOW(MATV_IOC_MAGIC, 0x02, unsigned int)
#define MATV_SET_PWR            _IOW(MATV_IOC_MAGIC, 0x03, unsigned int)
#define MATV_SET_RST            _IOW(MATV_IOC_MAGIC, 0x04, unsigned int)
#define MATV_SET_STRAP          _IOW(MATV_IOC_MAGIC, 0x05, unsigned int)
#define MATV_SLEEP              _IOW(MATV_IOC_MAGIC, 0x06, unsigned int)
#define MATV_SET_TP_MODE        _IOW(MATV_IOC_MAGIC, 0x07, unsigned int)


#endif //__MATV_H__