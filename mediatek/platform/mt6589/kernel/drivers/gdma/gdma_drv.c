

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <linux/xlog.h>

#include <asm/io.h>


#include <mach/irqs.h>

#include <mach/mt_reg_base.h>

#include <mach/mt_irq.h>

#include <mach/mt_clkmgr.h>


//============================================================




#include <linux/interrupt.h>
//#include <asm/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
//#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/aee.h>
#include <linux/timer.h>
#include <linux/disp_assert_layer.h>


//Arch dependent files
#include <asm/mach/map.h>
#include <mach/sync_write.h>


#include <mach/mt_boot.h>
// #include <asm/tcm.h>
#include <asm/cacheflush.h>
#include <asm/system.h>
//#include <linux/mm.h>
#include <linux/pagemap.h>









//==========================================================

#include "gdma_drv.h"
#include "gdma_drv_6589_common.h"




//#define USE_SYSRAM
#define GDMA_MSG(...)   xlog_printk(ANDROID_LOG_DEBUG, "xlog/gdma", __VA_ARGS__)
#define GDMA_WRN(...)   xlog_printk(ANDROID_LOG_WARN,  "xlog/gdma", __VA_ARGS__)
#define GDMA_ERR(...)   xlog_printk(ANDROID_LOG_ERROR, "xlog/gdma", __VA_ARGS__)
//#define GDMA_MSG printk
#define GDMA_DEVNAME "mtk_gdma"

#define TABLE_SIZE 4096

#define GDMA_CTL_PROCESS 0x1
//#define GDMA_ENC_PROCESS 0x2

//#define FPGA_VERSION
#include "gdma_drv_6589_reg.h"
//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

// global function
//extern kal_uint32 _gdma_ctr_int_status;
unsigned int _gdma_ctr_int_status ;
extern kal_uint32 _gdma_ctr_mode ;

// device and driver
static dev_t gdma_devno;
static struct cdev *gdma_cdev;
static struct class *gdma_class = NULL;

// gdma

//static wait_queue_head_t gdma_wait_queue;
static spinlock_t gdma_ctl_lock;
static int ctr_status = 0;

#ifdef GDMA_INC_FPGA
static unsigned char gStartISP;
static unsigned char gGDMALink;
static unsigned char gFMT_en;
static unsigned char gISP_TDRI_en;
static unsigned char gISP_yuv_en;
#endif

//--------------------------------------------------------------------------
// GDMA Common Function
//--------------------------------------------------------------------------


#ifdef FPGA_VERSION

void gdma_drv_power_on(void)
{  
    GDMA_MSG("GDMA Power On\n");
}

void gdma_drv_power_off(void)
{  
    GDMA_MSG("GDMA Power Off\n");
}


#else

  //#ifdef GDMA_IRQ
  //  static irqreturn_t gdma_drv_isr(int irq, void *dev_id)
  //  {
  //      GDMA_MSG("GDMA Interrupt\n");
  //      
  //  
  //      if(irq == MT6575_GDMA_IRQ_ID)
  //      {
  //  
  //          if(gdma_isr_lisr() == 0)
  //          {
  //              wake_up_interruptible(&gdma_wait_queue);
  //          }
  //      }
  //  
  //      return IRQ_HANDLED;
  //  }
  //#endif

void gdma_drv_power_on(void)
{  

    //REG_GDMA_MM_REG_MASK = 0;   
   
#ifdef FOR_COMPILE   
    BOOL ret;
    ret = enable_clock(MT65XX_PDN_MM_GDMA_DEC,"GDMA");
    NOT_REFERENCED(ret);
#endif
}

void gdma_drv_power_off(void)
{  
#ifdef FOR_COMPILE   
    BOOL ret;
    ret = disable_clock(MT65XX_PDN_MM_GDMA_DEC,"GDMA");
    NOT_REFERENCED(ret);
#endif
}
  

#endif



static int gdma_drv_init(void)
{


    int retValue;
    spin_lock(&gdma_ctl_lock);
    if(ctr_status != 0)
    {
        GDMA_WRN("GDMA is busy\n");
        retValue = -EBUSY;
    }    
    else
    {
        ctr_status = 1;
        retValue = 0;    
    }   
    spin_unlock(&gdma_ctl_lock);

    if(retValue == 0)
    {
        gdma_drv_power_on();

        gdma_drv_reset();

    }

    return retValue;

}

static void gdma_drv_deinit(void)
{
    if(ctr_status != 0)
    {

        spin_lock(&gdma_ctl_lock);
        ctr_status = 0;
        spin_unlock(&gdma_ctl_lock);

        gdma_drv_reset();

        gdma_drv_power_off();
    }

}




//--------------------------------------------------------------------------
// GDMA CTL IOCTRL FUNCTION
//--------------------------------------------------------------------------


static int gdma_ctl_ioctl( unsigned int cmd, unsigned long arg, struct file *file)
//static int GDMA_Ioctl(struct inode * a_pstInode, struct file * file, unsigned int cmd, unsigned long arg)
{
#ifdef GDMA_INC_FPGA    
    unsigned int timeout = 0x4FFFFF;
    unsigned int idx;
    unsigned int addr;
#endif    
    //GDMA_DRV_FMT_IN fmtParam;
    GDMA_DRV_CTL_IN ctlParam;
    //CONFIG_DRV_CDP_IN ispParam;

    unsigned int*       pStatus;
    //unsigned int        chksum;
    //unsigned char       whichBank;

    
    pStatus = (unsigned int*)file->private_data;

    if (NULL == pStatus)
    {
        GDMA_MSG("Private data is null in flush operation. SOME THING WRONG??\n");
        return -EFAULT;
    }    
    
    switch(cmd)
    {     
      case GDMA_IOCTL_INIT:
            GDMA_MSG("GDMA Initial and Lock\n");
            if(gdma_drv_init() == 0)
            {
                *pStatus = GDMA_CTL_PROCESS;
            }         
         
         break; 

      case GDMA_IOCTL_RESET:  /* OT:OK */
          GDMA_MSG("GDMA Reset\n");
          gdma_drv_reset();
          break;       
          
      case GDMA_IOCTL_DEINIT:
          GDMA_MSG("GDMA Deinit Hardware\n");
          // copy input parameters
          if(*pStatus != GDMA_CTL_PROCESS)
          {
              printk("Permission Denied! This process can not access GDMA_CTL");
              return -EFAULT;
          }

          if(ctr_status == 0)
          {
              printk("GDMA_CTL status is available, HOW COULD THIS HAPPEN ??");
              *pStatus = 0;
              return -EFAULT;
          }
          gdma_drv_deinit();
          *pStatus = 0;   
          break;          
          
      case GDMA_IOCTL_CONFIG_CTL:
        
         
          GDMA_MSG("GDMA Configure Hardware\n");
          if(*pStatus != GDMA_CTL_PROCESS)
          {
              GDMA_MSG("Permission Denied! This process can not access GDMA\n");
              return -EFAULT;
          }
          if(ctr_status == 0)
          {
              GDMA_MSG("GDMA is unlocked!!");
              *pStatus = 0;
              return -EFAULT;
          }         
          if (copy_from_user(&ctlParam, (void*)arg, sizeof(GDMA_DRV_CTL_IN)))
          {
              printk("Copy from user failed\n");
              return -EFAULT;
          }
          #ifdef GDMA_INC_FPGA          
            if(ctlParam.isSrcGray){
              gISP_yuv_en = 4; //disable cb/cr DMA
            }
          #endif          
           
          gdma_config_ctl(&ctlParam);
        break;   

      
      case GDMA_IOCTL_RW_REG:
          gdma_rw_reg();
        break;
        
      case GDMA_IOCTL_DUMP_REG:
          GDMA_MSG("GDMA Dump Hardware Reg\n");
          gdma_dump_reg();
        break;

#ifdef GDMA_INC_FPGA        
{
//      case GDMA_IOCTL_INIT:{
//      
//          addr = CAM_REG_BASE + 0x0000;
//          *(volatile unsigned int *)(addr) = 0;
//          
//          if (gdma_reset_isp() < 0)
//              return -EFAULT;        
//        }break;
//        
//      case GDMA_IOCTL_RESET:{
//      
//          if (gdma_reset_isp() < 0)
//              return -EFAULT;
//        }break;
//
//      case GDMA_IOCTL_CONFIG_ISP:{
//          printk("GDMA ISP Config\n");
//          if (copy_from_user(&ispParam, (void*)arg, sizeof(CONFIG_DRV_CDP_IN)))
//          {
//              printk("Copy from user failed\n");
//              return -EFAULT;
//          }
//          gStartISP = ispParam.startISP;     
//          gGDMALink = ispParam.linkGDMA;
//          gFMT_en   = ispParam.fmtEn;
//          gISP_TDRI_en = ispParam.isp_TDRIen ;
//          gISP_yuv_en = 7; //default enable YUV DMA
//      
//          if (gStartISP){
//              if(gISP_TDRI_en ){
//                 if( !ispParam.isp_TDRI_resume)
//                   gdma_isp_setup_tdri();
//              }else{
//                 gdma_isp_setup();  
//              }
//          }
//          
//          printk("ISP modify: %d\n", ispParam.numModify);
//          for (idx = 0; idx < ispParam.numModify; idx++)
//          {
//              printk("%08x %08x\n", ispParam.modifyOffset[idx], ispParam.modifyValue[idx]);
//              wait_pr();
//      
//              write_reg(ispParam.modifyOffset[idx], ispParam.modifyValue[idx]);
//          }
//      }break;
//        
//      
//      case GDMA_IOCTL_START :{
//               
//          printk(" Link GDMA: %d, Trigger ISP: %d, FMT %d, Enable GDMA: %d, YUV_DMA %d!!\n", gGDMALink, gStartISP, gFMT_en ,ispParam.gdmaEn, gISP_yuv_en);
//          gdma_start(gGDMALink, gStartISP, gFMT_en, gISP_yuv_en);
//        }break;      
//
//      case GDMA_IOCTL_CONFIG_FMT:{  
//          printk("GDMA FMT Config\n");
//          if (copy_from_user(&fmtParam, (void*)arg, sizeof(GDMA_DRV_FMT_IN)))
//          {
//              printk("Copy from user failed\n");
//              return -EFAULT;
//          }
//          gdma_config_fmt(fmtParam);
//      }break;
}      
#endif 
        
      default:
          printk("GDMA NO THIS IOCTL COMMAND, %d!!\n", cmd);
        break;
    }
    return 0;
}



//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
//static int gdma_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
static long gdma_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {

        case GDMA_IOCTL_CONFIG_CTL  : 
        case GDMA_IOCTL_WAIT        : 
        case GDMA_IOCTL_RESET       : 
        case GDMA_IOCTL_INIT        : 
        case GDMA_IOCTL_DUMP_REG    : 
        case GDMA_IOCTL_DEINIT :
        //case GDMA_IOCTL_START       : 
        //case GDMA_IOCTL_CONFIG_FMT  : 
        //case GDMA_IOCTL_CONFIG_ISP  : 
        //case GDMA_IOCTL_RW_REG      : 
        
          return gdma_ctl_ioctl(cmd, arg, file);
  


        default :
            break; 
    }
    
    return -EINVAL;
}

static int gdma_open(struct inode *inode, struct file *file)
{
    unsigned int *pStatus;
    //Allocate and initialize private data
    file->private_data = kmalloc(sizeof(unsigned int) , GFP_ATOMIC);

    if(NULL == file->private_data)
    {
        GDMA_WRN("Not enough entry for GDMA open operation\n");
        return -ENOMEM;
    }

    pStatus = (unsigned int *)file->private_data;
    *pStatus = 0;
    
    return 0;
}

static ssize_t gdma_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    GDMA_MSG("gdma driver read\n");
    return 0;
}

static int gdma_release(struct inode *inode, struct file *file)
{
    if(NULL != file->private_data)
    {
        kfree(file->private_data);
        file->private_data = NULL;
    }
    return 0;
}

static int gdma_flush(struct file * a_pstFile , fl_owner_t a_id)
{
    unsigned int *pStatus;

    pStatus = (unsigned int*)a_pstFile->private_data;

    if(NULL == pStatus)
    {
        GDMA_WRN("Private data is null in flush operation. HOW COULD THIS HAPPEN ??\n");
        return -EFAULT;
    }

    if (*pStatus == GDMA_CTL_PROCESS)
    {
        if(ctr_status != 0) 
        {
            GDMA_WRN("Error! Enable error handling for gdma !!");
            gdma_drv_deinit();
        }
    }


    return 0;
}

/* Kernel interface */
static struct file_operations gdma_fops = {
	.owner		= THIS_MODULE,
	//.ioctl		= gdma_ioctl,
	.unlocked_ioctl = gdma_unlocked_ioctl,
	.open		= gdma_open,
	.release	= gdma_release,
	.flush		= gdma_flush,
	.read       = gdma_read,
};

static int gdma_probe(struct platform_device *pdev)
{
    struct class_device;
    
	int ret;
    struct class_device *class_dev = NULL;
    
    GDMA_MSG("-------------gdma driver probe-------\n");
	ret = alloc_chrdev_region(&gdma_devno, 0, 1, GDMA_DEVNAME);

	if(ret)
	{
	    GDMA_ERR("Error: Can't Get Major number for GDMA Device\n");
	}
	else
	{
	    GDMA_MSG("Get GDMA Device Major number (%d)\n", gdma_devno);
    }

	gdma_cdev = cdev_alloc();
    gdma_cdev->owner = THIS_MODULE;
	gdma_cdev->ops = &gdma_fops;

	ret = cdev_add(gdma_cdev, gdma_devno, 1);

    gdma_class = class_create(THIS_MODULE, GDMA_DEVNAME);
    class_dev = (struct class_device *)device_create(gdma_class, NULL, gdma_devno, NULL, GDMA_DEVNAME);

    spin_lock_init(&gdma_ctl_lock);

    // initial GDMA, register ISR
    ctr_status = 0;
    _gdma_ctr_int_status = 0;

//#ifdef GDMA_IRQ
//    init_waitqueue_head(&gdma_wait_queue);  
//    
//
//    enable_irq(MT6575_GDMA_IRQ_ID);
//    if(request_irq(MT6575_GDMA_IRQ_ID, gdma_drv_isr, IRQF_TRIGGER_LOW, "gdma_driver" , NULL))
//    {
//        GDMA_ERR("GDMA Driver request irq failed\n");
//    }
//#endif
	GDMA_MSG("GDMA Probe Done\n");

	NOT_REFERENCED(class_dev);
	return 0;
}

static int gdma_remove(struct platform_device *pdev)
{
	GDMA_MSG("GDMA remove\n");
	//unregister_chrdev(GDMA_MAJOR, GDMA_DEVNAME);
//#ifdef GDMA_IRQ
//	free_irq(MT6575_GDMA_IRQ_ID, NULL);
//#endif
	GDMA_MSG("Done\n");
	return 0;
}

static void gdma_shutdown(struct platform_device *pdev)
{
	GDMA_MSG("GDMA shutdown\n");
	/* Nothing yet */
}

/* PM suspend */
static int gdma_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    gdma_drv_deinit();
    return 0;
}

/* PM resume */
static int gdma_resume(struct platform_device *pdev)
{
    return 0;
}


static struct platform_driver gdma_driver = {
	.probe		= gdma_probe,
	.remove		= gdma_remove,
	.shutdown	= gdma_shutdown,
	.suspend	= gdma_suspend,
	.resume		= gdma_resume,
	.driver     = {
	              .name = GDMA_DEVNAME,
	},
};

static void gdma_device_release(struct device *dev)
{
	// Nothing to release? 
}

static u64 gdma_dmamask = ~(u32)0;

static struct platform_device gdma_device = {
	.name	 = GDMA_DEVNAME,
	.id      = 0,
	.dev     = {
		.release = gdma_device_release,
		.dma_mask = &gdma_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources = 0,
};

static int __init gdma_init(void)
{
    int ret;

    GDMA_MSG("GDMA initialize\n");
	
    GDMA_MSG("Register the GDMA device\n");
	if(platform_device_register(&gdma_device))
	{
        GDMA_ERR("failed to register gdma device\n");
        ret = -ENODEV;
        return ret;
	}

    GDMA_MSG("Register the GDMA driver\n");    
    if(platform_driver_register(&gdma_driver))
    {
        GDMA_ERR("failed to register gdma driver\n");
        platform_device_unregister(&gdma_device);
        ret = -ENODEV;
        return ret;
    }

    return 0;
}

static void __exit gdma_exit(void)
{
    cdev_del(gdma_cdev);
    unregister_chrdev_region(gdma_devno, 1);
	  //GDMA_MSG("Unregistering driver\n");
    platform_driver_unregister(&gdma_driver);
	  platform_device_unregister(&gdma_device);
	
	  device_destroy(gdma_class, gdma_devno);
	  class_destroy(gdma_class);
	  
	  GDMA_MSG("Done\n");
}

module_init(gdma_init);
module_exit(gdma_exit);
MODULE_AUTHOR("Hao-Ting Huang <otis.huang@mediatek.com>");
MODULE_DESCRIPTION("MT6573 GDMA Driver");
MODULE_LICENSE("GPL");
