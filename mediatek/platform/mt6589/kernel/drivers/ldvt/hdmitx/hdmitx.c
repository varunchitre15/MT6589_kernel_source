/*****************************************************************************/
/* Copyright (c) 2009 NXP Semiconductors BV                                  */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, using version 2 of the License.             */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307       */
/* USA.                                                                      */
/*                                                                           */
/*****************************************************************************/
//#if defined(MTK_HDMI_SUPPORT)
#if 1
#define TMFL_TDA19989
#define _tx_c_
#include <linux/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/earlysuspend.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/vmalloc.h>
#include <linux/disp_assert_layer.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/dma.h>
#include <mach/irqs.h>

#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/earlysuspend.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/dma.h>
#include <mach/irqs.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>

#include "hdmi_drv.h"
#include "mach/reg_base.h"
#include "mach/sync_write.h"
#include "lcd_drv.h"
#include "lcd_reg.h"
#include "dsi_reg.h"
#include "dpi_drv.h"
#include "dpi_reg.h"
#include "mach/eint.h"
#include "mach/irqs.h"
#include "hdmitx.h"
#define OUTREG32(x, y) {/*printk("[hdmi]write 0x%08x to 0x%08x\n", (y), (x)); */__OUTREG32((x),(y))}
#define __OUTREG32(x,y) {*(unsigned int*)(x)=(y);}

static size_t hdmi_log_on = true;
static struct switch_dev hdmi_switch_data;
#define HDMI_LOG(fmt, arg...) \
	do { \
		if (hdmi_log_on) {printk("[hdmi]%s,#%d ", __func__, __LINE__); printk(fmt, ##arg);} \
	}while (0)
        
#define HDMI_FUNC()	\
    do { \
        if(hdmi_log_on) printk("[hdmi] %s\n", __func__); \
    }while (0)
                
#define HDMI_LINE()	\
    do { \
        if (hdmi_log_on) {printk("[hdmi]%s,%d ", __func__, __LINE__); printk(fmt, ##arg);} \
    }while (0)


static int hdmi_release(struct inode *inode, struct file *file)
{
    printk("[hdmi]%s\n", __func__);
	return 0;
}

static int hdmi_open(struct inode *inode, struct file *file)
{ 
    printk("[hdmi]%s\n", __func__);
	return 0;
}

static long hdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	int r = 0;

	HDMI_LOG("cmd=0x%08x, arg=0x%08x\n", cmd, arg);

    HDMI_LOG("ioctl finished\n");
	return r;    
}

static int hdmi_remove(struct platform_device *pdev)
{
	return 0;
}

#define HDMI_DEVNAME "hdmitx"


static dev_t hdmi_devno;
static struct cdev *hdmi_cdev;
static struct class *hdmi_class = NULL;
static void __exit hdmi_exit(void)
{
	device_destroy(hdmi_class, hdmi_devno);    
	class_destroy(hdmi_class);
	cdev_del(hdmi_cdev);
	unregister_chrdev_region(hdmi_devno, 1);

}


struct file_operations hdmi_fops = {
	.owner   = THIS_MODULE,
	.unlocked_ioctl   = hdmi_ioctl,
	.open    = hdmi_open,    
	.release = hdmi_release,
};

static int hdmi_probe(struct platform_device *pdev)
{
    	printk("[hdmi]%s\n", __func__);
    	int ret = 0;
	struct class_device *class_dev = NULL;

	ret = alloc_chrdev_region(&hdmi_devno, 0, 1, HDMI_DEVNAME);
	if(ret)
	{
		printk("[hdmi]alloc_chrdev_region fail\n");
		return -1;
	}

	hdmi_cdev = cdev_alloc();
	hdmi_cdev->owner = THIS_MODULE;
	hdmi_cdev->ops = &hdmi_fops;
	ret = cdev_add(hdmi_cdev, hdmi_devno, 1);
	hdmi_class = class_create(THIS_MODULE, HDMI_DEVNAME);
	class_dev = (struct class_device *)device_create(hdmi_class, NULL, hdmi_devno,	NULL, HDMI_DEVNAME);

	printk("[hdmi][%s] current=0x%08x\n", __func__, current);

    	return 0;
}

static struct platform_driver hdmi_driver = {
    .probe  = hdmi_probe,
    .remove = hdmi_remove,
    .driver = { .name = HDMI_DEVNAME }
};

extern int hdmi_drv_audio_config(HDMI_AUDIO_FORMAT aformat);
extern int hdmi_drv_video_config(HDMI_VIDEO_RESOLUTION vformat, HDMI_VIDEO_INPUT_FORMAT vin, HDMI_VIDEO_OUTPUT_FORMAT vout);
extern int hdmi_drv_init(void);
extern void hdmi_drv_power_on(void);
extern void hdmi_drv_resume(void);

extern void hdmi_drv_set_colorbar(void);
extern void hdmi_drv_test_main(void);
static int __init hdmi_init(void)
{
	printk("[hdmi]%s\n", __func__);

	int ret = 0;

    if (platform_driver_register(&hdmi_driver)) 
    {
        printk("[hdmi]failed to register mtkfb driver\n");
        return -1;
    }  


	hdmi_drv_test_main();
	return 0;
}


module_init(hdmi_init);
module_exit(hdmi_exit);
MODULE_AUTHOR("Xuecheng, Zhang <xuecheng.zhang@mediatek.com>");
MODULE_DESCRIPTION("HDMI Driver");
MODULE_LICENSE("GPL");

#endif
