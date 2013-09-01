
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/dma-mapping.h>
#include <mach/dma.h>   /* FIXME */
#include <mach/board.h> /* FIXME */
#include <mach/mt6573_devs.h>
#include <mach/mt6573_typedefs.h>
//#include <mach/mt6573_pll.h>
//#include <mach/mt6573_gpio.h>
//#include <asm/tcm.h>
#include "mt6573_sd_dvt.h"

#define MSDC_DEBUG	1
#define MSDC_SAY		"[MSDC_DVT]: "
#define MSDC_NAME		"mt6573-dvt-msdc"

#if MSDC_DEBUG
#define msdc_print(fmt, arg...)	printk(MSDC_SAY fmt, ##arg)
#else
#define msdc_print(fmt, arg...)	do {} while (0)
#endif

#define MSDC_GET_CARD_TYPE (1)
#define MSDC_GET_CARD_RCA  (2)

struct msdc_dvt_para{
	int  cmd;
	int  para;
	int  host_number;
	int  result;
};
struct mmc_card* msdc_dvt_card[4]={NULL,NULL,NULL,NULL};
struct mmc_host* msdc_dvt_host[4]={NULL,NULL,NULL,NULL}; 

extern int mmc_msdc_type(int i);
struct mmc_card* mmc_msdc_card(int i);
struct mmc_host* mmc_msdc_host(int i);

static long msdc_dvt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{	
	struct msdc_dvt_para *dvt_cmd = (struct msdc_dvt_para *)arg;
	printk(MSDC_SAY"put in msdc dvt driver ioctl function!!\n");
	
  switch(dvt_cmd->cmd){
  	case MSDC_GET_CARD_TYPE:
  		dvt_cmd->result = mmc_msdc_type(dvt_cmd->host_number);
  		printk(MSDC_SAY "IOCTL getMSDC Slot[%d] card type is :0x%x\n",dvt_cmd->host_number,dvt_cmd->result);
  		break;
   case MSDC_GET_CARD_RCA:
  		msdc_dvt_card[dvt_cmd->host_number] = mmc_msdc_card(dvt_cmd->host_number);
  		printk(MSDC_SAY "IOCTL get MSDC Slot[%d] card rca is :0x%x\n",dvt_cmd->host_number,msdc_dvt_card[dvt_cmd->host_number]->rca);
  		dvt_cmd->result = (int)msdc_dvt_card[dvt_cmd->host_number]->rca;
  		break;
  	default:
  		return -EINVAL;										
  }
  return 0;
}

static int msdc_dvt_open(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations msdc_dvt_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= msdc_dvt_ioctl,
	.open		= msdc_dvt_open,
};

static struct miscdevice msdc_dvt_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= MSDC_NAME,
	.fops	= &msdc_dvt_fops,
};

static int __init msdc_dvt_init(void)
{
	int r;

	printk(MSDC_SAY"register MSDC dvt driver!!\n");
	r = misc_register(&msdc_dvt_dev);
	if (r) {
		printk(MSDC_SAY "register MSDC dvt driver failed (%d)\n", r);
		return r;
	}

	return 0;
}

/* should never be called */
static void __exit msdc_dvt_exit(void)
{
	int r;
	
	printk(MSDC_SAY"unregister MSDC dvt driver!!\n");
	r = misc_deregister(&msdc_dvt_dev);
	if(r){
		printk(MSDC_SAY"unregister MSDC dvt driver failed\n");
	}
}

module_init(msdc_dvt_init);
module_exit(msdc_dvt_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("MT6573 MSDC Driver v0.1");
MODULE_LICENSE("GPL");
