
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/tcm.h>

#include <asm/cacheflush.h>


#include "lcd_drv.h" // Config LCD


#include "tvrot_dvt.h"
#include "tvrot_dvt_datatype.h"

#define TV_DVT_DEBUG	1

#define TV_DVT_NAME	"mtk_tv_dvt"

#if TV_DVT_DEBUG
#define TV_DVT_DBG(fmt, arg...)	printk(TV_SAY fmt, ##arg)
#else
#define TV_DVT_DBG(fmt, arg...)	do {} while (0)
#endif


#define TV_SAY		"[TV_DVT]: "


static unsigned int *pva=NULL;
static unsigned int phy=0;

static unsigned int src_buf_pa;
static unsigned int src_buf_va;

static unsigned int dst_buf_pa;
static unsigned int dst_buf_va;

#define BUFFER_SIZE (100*100*3)

#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 100



void SMI_REG_INIT_FOR_PERFORMACE(void)
{
    printk("%s\n", __func__);

    //frame size
    OUTREG32(0xF2081028, 0x30);
    OUTREG32(0xF2082028, 0x30);
    OUTREG32(0xF2083028, 0x30);
    OUTREG32(0xF20C1028, 0x30);

    //ultra
    OUTREG32(0xF2081024, 0x4000);
    OUTREG32(0xF2082024, 0x4000);
    OUTREG32(0xF2083024, 0x4000);
    OUTREG32(0xF20C1024, 0x4000);
    //ultra
    OUTREG32(0xF2081014, 0x2);
    OUTREG32(0xF2082014, 0x2);
    OUTREG32(0xF2083014, 0x2);
    OUTREG32(0xF20C1014, 0x2);

    //AT
    OUTREG32(0xF2081030, 0x2);
    OUTREG32(0xF2082030, 0x2);
    OUTREG32(0xF2083030, 0x2);
    OUTREG32(0xF20C1030, 0x2);

    //common limiter
    OUTREG32(0xF0008a00, 0x2);
    OUTREG32(0xF0008a04, 0x17e);
    OUTREG32(0xF0008a08, 0x167);
    OUTREG32(0xF0008a0c, 0x134);
    OUTREG32(0xF0008a10, 0x120);

    //larb0 VC
    OUTREG32(0xF2081080, 0x140);
    OUTREG32(0xF2081084, 0x61);
    OUTREG32(0xF2081088, 0x3);

    //larb1 VC
    OUTREG32(0xF2082080, 0x151);
    OUTREG32(0xF2082084, 0x16);
    OUTREG32(0xF2082088, 0x1);

    //larb2 VC
    OUTREG32(0xF2083080, 0x41);
    OUTREG32(0xF2083084, 0x43);
    OUTREG32(0xF2083088, 0x1);

    //larb0 INT MUX
    OUTREG32(0xF2081054, 0x300c08);

}


void TVDVT_init_lcd(void)
{

    UINT32 i;


    LCD_CHECK_RET(LCD_LayerSetAddress(LCD_LAYER_0, src_buf_pa));
    LCD_CHECK_RET(LCD_LayerSetFormat(LCD_LAYER_0, LCD_LAYER_FORMAT_RGB888));
    LCD_CHECK_RET(LCD_LayerSetOffset(LCD_LAYER_0, 0, 0));
    LCD_CHECK_RET(LCD_LayerSetSize(LCD_LAYER_0,  SCREEN_WIDTH,SCREEN_HEIGHT));


    LCD_CHECK_RET(LCD_LayerSetPitch(LCD_LAYER_0, SCREEN_WIDTH * 3));


    LCD_CHECK_RET(LCD_LayerEnable(LCD_LAYER_0, FALSE));
    //LCD_CHECK_RET(LCD_LayerEnable(LCD_LAYER_ALL, FALSE));

    //LCD_CHECK_RET(LCD_LayerSetTriggerMode(LCD_LAYER_ALL, LCD_SW_TRIGGER));
    LCD_CHECK_RET(LCD_LayerSetTriggerMode(LCD_LAYER_0, LCD_SW_TRIGGER));


    LCD_CHECK_RET(LCD_EnableHwTrigger(FALSE));

    LCD_CHECK_RET(LCD_SetBackgroundColor(0));
    LCD_CHECK_RET(LCD_SetRoiWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));

    LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_RGB888));
    LCD_CHECK_RET(LCD_FBSetPitch(SCREEN_WIDTH * 3 ));
    LCD_CHECK_RET(LCD_FBSetStartCoord(0, 0));
/*
    for (i = 0; i < lcm_params.dpi.intermediat_buffer_num; ++ i)
    {
        LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0 + i, s_tmpBuffers[i].pa));
        LCD_CHECK_RET(LCD_FBEnable(LCD_FB_0 + i, TRUE));
    }
*/

    LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0, dst_buf_pa)); //buffer address.
    LCD_CHECK_RET(LCD_FBEnable(LCD_FB_0, TRUE));

    //LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_MEM));
    LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_TVROT));


    LCD_CHECK_RET(LCD_LayerEnable(LCD_LAYER_0, TRUE));


}

void TVDVT_update_screen(void)
{
    LCD_CHECK_RET(LCD_StartTransfer(FALSE));
}
void TVDVT_prepare_source(unsigned int src_pa)
{
    //static char data = 0x10;
    //memset(src_buf_va, data++, BUFFER_SIZE);


    __cpuc_flush_user_all();
	outer_clean_all();
    LCD_CHECK_RET(LCD_LayerSetAddress(LCD_LAYER_0, src_pa));
}
void TVDVT_update_src(void)
{
    static char data = 0x10;
    memset(src_buf_va, data++, BUFFER_SIZE);
    __cpuc_flush_user_all();
	outer_clean_all();

}



// LCDC->TVR->MEMORY

static void tvr_dvt_path_init()
{
    TVDVT_init_lcd();
    TVR_Init();
    TVR_PowerOn();
}


static int tvr_dvt_path_config(TVR_DVT_PATH_PARAM *param)
{

//1.Config TVR
    printk("[TV_DVT] src (%d,%d) (%d,%d,%d,%d) 0x%x\n "
        "         dst (%d,%d) (%d,%d,%d,%d) 0x%x\n"
        "         fmt(%d) rot (%d) auto (%d) flip(%d)\n",
        param->src_img_size.w, param->src_img_size.h,
        param->src_img_roi.x, param->src_img_roi.y,
        param->src_img_roi.w, param->src_img_roi.h, param->src_img_addr,
        param->dst_img_size.w, param->dst_img_size.h,
        param->dst_img_roi.x, param->dst_img_roi.y,
        param->dst_img_roi.w, param->dst_img_roi.h, param->dst_img_addr,
        param->dst_fmt, param->bRotate, param->bAuto, param->bFlip);


//2.Config LCDC
    TVDVT_prepare_source((UINT32)param->src_img_addr);


//3. Config TVR

    TVR_PARAM tvrot_param;
    tvrot_param.srcWidth = param->src_img_size.w;
    tvrot_param.srcHeight = param->src_img_size.h;
    tvrot_param.srcRoi.w = param->src_img_roi.w;
    tvrot_param.srcRoi.h = param->src_img_roi.h;
    tvrot_param.srcRoi.x = param->src_img_roi.x;
    tvrot_param.srcRoi.y = param->src_img_roi.y;
    tvrot_param.outputFormat = param->dst_fmt;
    tvrot_param.rotation = param->bRotate;
    tvrot_param.flip = param->bFlip;
    tvrot_param.dstWidth = param->dst_img_size.w;
    tvrot_param.bAuto = param->bAuto;
    tvrot_param.dstBufAddr[0] = param->dst_img_addr;

    tvrot_param.dstBufNum = param->dst_img_num;

    if (tvrot_param.bAuto)
    {
        tvrot_param.dstBufAddr[1] = param->dst_img_addr + tvrot_param.srcWidth * tvrot_param.srcHeight * 2;
        tvrot_param.dstBufAddr[2] = tvrot_param.dstBufAddr[1] + tvrot_param.srcWidth * tvrot_param.srcHeight * 2;
    }

    TVR_Config(&tvrot_param);



}

void tvr_dvt_path_start()
{
    TVR_Start();
    TVDVT_update_screen();
}

static int tvr_dvt_path_wait_done()
{
    //polling to wait done here
    //msleep(3000);
    TVR_Wait_Done();
    LCD_DumpRegisters();
    TVR_DumpRegisters();

}

static int tvr_dvt_path_stop()
{
    TVR_Stop();
}

static int tvr_dvt_path_deinit()
{
    TVR_Deinit();
}



static int tvr_dvt_ioctl( struct inode * stInode,
						        struct file * stFile,
						        unsigned int cmd,
						        unsigned long param)
{
    int ret = 0;
    //printk("%s\n", __func__);

	switch (cmd)
	{
        case TV_DVT_TVROT_PATH_INIT:
    	{
            printk("[TV_DVT] ioctl cmd: TV_DVT_TVROT_PATH_INIT\n");
            tvr_dvt_path_init();
        	break;
    	}
        case TV_DVT_TVROT_PATH_START:
    	{
            printk("[TV_DVT] ioctl cmd: TV_DVT_TVROT_PATH_START\n");
            tvr_dvt_path_start();
        	break;
    	}
		case TV_DVT_TVROT_PATH_CONFIG:
    	{
            TVR_DVT_PATH_PARAM tvrDvtParam;
            printk("[TV_DVT] ioctl cmd: TV_DVT_TVROT_PATH_CONFIG\n");

            if (copy_from_user(&tvrDvtParam, (void __user *)param, sizeof(tvrDvtParam)))
        	{
            	printk("[TV_DVT]: copy_from_user failed! line:%d \n", __LINE__);
            	ret = -EFAULT;
        	} else {
                tvr_dvt_path_config(&tvrDvtParam);
			}
        	break;
    	}

        case TV_DVT_TVROT_PATH_STOP:
    	{
            printk("[TV_DVT] ioctl cmd: TV_DVT_TVROT_PATH_STOP\n");
            tvr_dvt_path_stop();
        	break;
    	}
        case TV_DVT_TVROT_PATH_WAIT:
    	{
            printk("[TV_DVT] ioctl cmd: TV_DVT_TVROT_PATH_WAIT\n");
            tvr_dvt_path_wait_done();
        	break;
    	}
        case TV_DVT_TVROT_PATH_DEINIT:
    	{
            printk("[TV_DVT] ioctl cmd: TV_DVT_TVROT_PATH_DEINIT\n");
            tvr_dvt_path_deinit();
        	break;
    	}
        case TV_DVT_TVROT_UPDATE_SCREEN:
    	{
            printk("[TV_DVT] ioctl cmd: TV_DVT_TVROT_UPDATE_SCREEN\n");
            TVDVT_update_screen();
        	break;
    	}
        default:
            printk("[TVR_DVT] unknown io ctrl command %d\n", cmd);
            return -1;

	}
    return 0;
}

static int tvr_dvt_open(struct inode *inode, struct file *file)
{
	return 0;
}


static struct file_operations tvr_dvt_dev_fops = {
	.owner		= THIS_MODULE,
	.ioctl      = tvr_dvt_ioctl,
	.open		= tvr_dvt_open,
};

static struct miscdevice tvr_dvt_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= TV_DVT_NAME,
	.fops	= &tvr_dvt_dev_fops,
};

static int __init tvr_dvt_mod_init(void)
{
	int r;
    printk(TV_SAY "register driver TV DVT\n");

	r = misc_register(&tvr_dvt_dev);
	if (r) {
		printk(TV_SAY "register driver failed (%d)\n", r);
		return r;
	}

    // Allocate Source and Dest Buffers

    src_buf_va = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (src_buf_va == NULL)
    {
        printk("unable to allocate memory for TV-out\n");
        return -ENOMEM;
    }
    src_buf_pa = virt_to_phys(src_buf_va);
    printk("[TV] kmalloc src framebuffer PA=0x%08x, VA=0x%08x\n", src_buf_pa, src_buf_va);
    memset(src_buf_va, 0x55, BUFFER_SIZE);

    dst_buf_va = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (dst_buf_va == NULL)
    {
        printk("unable to allocate memory for TV-out\n");
        return -ENOMEM;
    }
    dst_buf_pa = virt_to_phys(dst_buf_va);
    printk("[TV] kmalloc src framebuffer PA=0x%08x, VA=0x%08x\n", dst_buf_pa, dst_buf_va);
    memset(dst_buf_va, 0x0, BUFFER_SIZE);



    TVR_Init();
    TVR_PowerOn();
    SMI_REG_INIT_FOR_PERFORMACE();

	return 0;
}

/* should never be called */
static void __exit tvr_dvt_mod_exit(void)
{
	int r;

    TVR_PowerOff();
    TVR_Deinit();

	r = misc_deregister(&tvr_dvt_dev);
	if(r){
		printk(TV_SAY"unregister driver failed\n");
	}
}



module_init(tvr_dvt_mod_init);
module_exit(tvr_dvt_mod_exit);



MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("MTK TVROT DVT Driver");
MODULE_LICENSE("GPL");

