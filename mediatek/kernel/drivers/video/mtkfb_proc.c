#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include "disp_drv.h"


#define MTKFB_PROC_INTERFACE_NAME "mtkfb_size"

static struct proc_dir_entry *mtkfb_kop_entry;

static int mtkfb_size_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = page;
	int len = 0, fb_size;

	fb_size = DISP_GetVRamSize();  //unit: Byte
	printk("%s fb_size:%d\n", __func__, fb_size);
	p += sprintf(p, "%d\n", fb_size);
	*start = page + off;

	len = p - page;
	if (len > off)
		len -= off;
	else
		len = 0;

	return (len < count) ? len : count;
}

int mtkfb_proc_intf_register(void)
{
	printk("%s: register mtk fb proc interface\n", __func__);
        
	mtkfb_kop_entry = create_proc_entry(MTKFB_PROC_INTERFACE_NAME, 0, NULL);
	if (mtkfb_kop_entry) {
		mtkfb_kop_entry->read_proc = mtkfb_size_read;
		mtkfb_kop_entry->write_proc = NULL;
	}
	
	return 0;
}

void mtkfb_proc_intf_unregister(void)
{
	printk("%s: unregister mtk fb proc interface\n", __func__);

	if (mtkfb_kop_entry != NULL) {
		remove_proc_entry(MTKFB_PROC_INTERFACE_NAME, mtkfb_kop_entry);
		mtkfb_kop_entry = NULL;
	}
}

static int __init mtkfb_proc_intf_init(void)
{
	printk("%s\n", __func__);
    return mtkfb_proc_intf_register();
}

static void __exit mtkfb_proc_intf_exit(void)
{
    printk("%s\n", __func__);
    mtkfb_proc_intf_unregister();
}

module_init(mtkfb_proc_intf_init);
module_exit(mtkfb_proc_intf_exit);

MODULE_DESCRIPTION("MediaTek framebuffer proc interface");
MODULE_AUTHOR("MediaTek Inc.");
MODULE_LICENSE("GPL");

