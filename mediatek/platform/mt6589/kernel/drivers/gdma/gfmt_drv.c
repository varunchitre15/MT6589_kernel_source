

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


#include "gfmt_drv.h"
#include "gfmt_drv_6589_common.h"




//#define USE_SYSRAM
#if 0
#define GFMT_MSG(...)   xlog_printk(ANDROID_LOG_DEBUG, "xlog/gfmt", __VA_ARGS__)
#define GFMT_WRN(...)   xlog_printk(ANDROID_LOG_WARN,  "xlog/gfmt", __VA_ARGS__)
#define GFMT_ERR(...)   xlog_printk(ANDROID_LOG_ERROR, "xlog/gfmt", __VA_ARGS__)
#else
#define GFMT_MSG printk
  #define GFMT_WRN printk
  #define GFMT_ERR printk
#endif
#define GFMT_DEVNAME "mtk_gfmt"

#define TABLE_SIZE 4096

#define GFMT_PROCESS 0x1

//#define FPGA_VERSION
#include "gdma_drv_6589_reg.h"
//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

// global function
extern kal_uint32 _gfmt_only_int_status;


// device and driver
static dev_t gfmt_devno;
static struct cdev *gfmt_cdev;
static struct class *gfmt_class = NULL;

// gfmt
static wait_queue_head_t gfmt_wait_queue;
static spinlock_t gdma_fmt_lock;
static int fmt_status = 0;

#if 0
static unsigned char gStartISP;
static unsigned char gGDMALink;
static unsigned char gFMT_en;
static unsigned char gISP_TDRI_en;
static unsigned char gISP_yuv_en;
#endif

#define GDMA_FMT_IRQ

//--------------------------------------------------------------------------
// GFMT Common Function
//--------------------------------------------------------------------------


#ifdef FPGA_VERSION

void gfmt_drv_power_on(void)
{  
    GFMT_MSG("GFMT Power On\n");
}

void gfmt_drv_power_off(void)
{  
    GFMT_MSG("GFMT Power Off\n");
}


#else

  #ifdef GDMA_FMT_IRQ
    static irqreturn_t gfmt_drv_isr(int irq, void *dev_id)
    {
        //GFMT_MSG("GFMT Interrupt\n");
        
    
        if(irq == CAMERA_ISP_IRQ3_ID)
        {
    
            if(gfmt_isr_lisr() == 0)
            {
                wake_up_interruptible(&gfmt_wait_queue);
            }
        }
    
        return IRQ_HANDLED;
    }
  #endif

void gfmt_drv_power_on(void)
{  

    //REG_GDMA_MM_REG_MASK = 0;   
   
#ifdef FOR_COMPILE   
    BOOL ret;
    ret = enable_clock(MT65XX_PDN_MM_GDMA_FMT,"GFMT");
    NOT_REFERENCED(ret);
#endif
}

void gfmt_drv_power_off(void)
{  
#ifdef FOR_COMPILE   
    BOOL ret;
    ret = disable_clock(MT65XX_PDN_MM_GDMA_FMT,"GFMT");
    NOT_REFERENCED(ret);
#endif
}
  

#endif



static int gfmt_drv_init(void)
{


    int retValue;
    spin_lock(&gdma_fmt_lock);
    if(fmt_status != 0)
    {
        GFMT_WRN("GFMT is busy\n");
        retValue = -EBUSY;
    }    
    else
    {
        fmt_status = 1;
        retValue = 0;    
    }   
    spin_unlock(&gdma_fmt_lock);

    if(retValue == 0)
    {
        gfmt_drv_power_on();

        gfmt_drv_reset();

    }

    return retValue;

}

static void gfmt_drv_deinit(void)
{
    if(fmt_status != 0)
    {

        spin_lock(&gdma_fmt_lock);
        fmt_status = 0;
        spin_unlock(&gdma_fmt_lock);

        gfmt_drv_reset();

        gfmt_drv_power_off();
    }

}



//--------------------------------------------------------------------------
// GFMT CTL IOCTRL FUNCTION
//--------------------------------------------------------------------------


static int gfmt_ioctl( unsigned int cmd, unsigned long arg, struct file *file)
//static int GDMA_Ioctl(struct inode * a_pstInode, struct file * file, unsigned int cmd, unsigned long arg)
{
   
    unsigned int timeout = 0x4FFFFF;
#if 0
    unsigned int idx;
    unsigned int addr;
#endif    
    GDMA_DRV_FMT_IN fmtParam;
//    GDMA_DRV_CTL_IN ctlParam;
//    CONFIG_DRV_CDP_IN ispParam;
    
    
    GFMT_DRV_OUT outParams;    

    unsigned int*       pStatus;
    unsigned int        convResult;
    //unsigned int        chksum;
    //unsigned char       whichBank;
    long timeout_jiff;
    unsigned int startISP = 0;     
    unsigned int regVal = 0;
    unsigned int FMT_en = 1;
    unsigned int fmtOnly = 1;
    
    pStatus = (unsigned int*)file->private_data;

    if (NULL == pStatus)
    {
        GFMT_MSG("Private data is null in flush operation. SOME THING WRONG??\n");
        return -EFAULT;
    }    
    
    switch(cmd)
    {     
      case GFMT_IOCTL_INIT:
            GFMT_MSG("GFMT Initial and Lock\n");
            if(gfmt_drv_init() == 0)
            {
                *pStatus = GFMT_PROCESS;
            }         
         
         break; 

      case GFMT_IOCTL_RESET:  /* OT:OK */
          GFMT_MSG("GFMT Reset\n");
          gfmt_drv_reset();
          break;              
      case GFMT_IOCTL_CONFIG_FMT:
      {  
          GFMT_MSG("GFMT Config\n");
          if(*pStatus != GFMT_PROCESS)
          {
              GFMT_MSG("Permission Denied! This process can not access GFMT\n");
              return -EFAULT;
          }
          if(fmt_status == 0)
          {
              GFMT_MSG("GFMT is unlocked!!");
              *pStatus = 0;
              return -EFAULT;
          }           
          if (copy_from_user(&fmtParam, (void*)arg, sizeof(GDMA_DRV_FMT_IN)))
          {
              GFMT_MSG("Copy from user failed\n");
              return -EFAULT;
          }
    
          gfmt_config_fmt(fmtParam);

          //*(volatile unsigned int *)( 0xF5004150 ) = 0x0001FFFF;

          
#if 1          
          if(!fmtParam.gfLinkGDMA && fmtParam.go_flag){
#if 1
          *(volatile unsigned int *)( 0xF5004150 ) = 0x0001FFFF;
#endif             
            regVal = REG_ISP_CTL_EN2;
            regVal = (regVal & ~REG_ISP_CTL_EN2_FMT_EN_MASK) | (FMT_en << REG_ISP_CTL_EN2_FMT_EN_SHIFT); // FMT Enable
            REG_ISP_CTL_EN2 = regVal;            
            GFMT_MSG("config GFMT driver go_flag %d!!\n", fmtParam.go_flag);
          }          
          
          if(!fmtParam.gfLinkGDMA && fmtParam.go_flag){
            regVal = REG_ISP_STA_CTL; 
            GFMT_MSG("config GFMT driver go_flag %d!!\n", fmtParam.go_flag);
            regVal = (regVal & ~REG_ISP_STR_CTL_GDMA_MASK) | (fmtOnly << REG_ISP_STR_CTL_GDMA_SHIFT);
            regVal = (regVal & ~REG_ISP_STR_CTL_ISP_MASK) | (startISP << REG_ISP_STR_CTL_ISP_SHIFT); // ISP Start
            REG_ISP_STA_CTL = regVal;
          }
#endif          
      }break;
      case GFMT_IOCTL_WAIT:
    
      
        GFMT_MSG("GFMT Wait\n");
        
        if(*pStatus != GFMT_PROCESS)
        {
            GFMT_MSG("Permission Denied! This process can not access GFMT\n");
            return -EFAULT;
        }
        if(fmt_status == 0)
        {
            GFMT_MSG("GFMT is unlocked!!");
            *pStatus = 0;
            return -EFAULT;
        } 
        if(copy_from_user(&outParams, (void *)arg, sizeof(GFMT_DRV_OUT)))
        {
            GFMT_WRN("GFMT : Copy from user error\n");
            return -EFAULT;
        }
        //set timeout
        timeout_jiff = outParams.timeout* HZ / 1000;
        GFMT_MSG("GFMT Time Jiffies : %ld\n", timeout_jiff);          
        
        
#ifdef FPGA_VERSION         
        while ((REG_GDMA_FMT_INTEN & CHK_GFMT_DONE_MASK) == 0)
        {
            timeout--;
            if (timeout == 0)
                break;
        }
        if (timeout == 0)
        {
            GFMT_MSG("GFMT Timeout\n");
            gfmt_drv_dump_reg();
            return -EFAULT;
        }
#else

            //if(outParams.timeout >= 5000){
            if(outParams.timeout & 0x01){
               timeout = outParams.timeout ;
                   
              GFMT_MSG("Polling GFMT Status, (%x), L:%d!!", timeout, __LINE__);              
              do
              {
                  _gfmt_only_int_status = REG_GDMA_FMT_INTEN;
                 timeout--;
              } while((_gfmt_only_int_status & CHK_GFMT_DONE_MASK)== 0 && timeout != 0);                              
              if(timeout == 0) GFMT_MSG("Polling GFMT Status TIMEOUT!!\n");              
            }else{
              GFMT_MSG("Polling GFMT Status, (%ld), L:%d!!", timeout_jiff, __LINE__);  
              wait_event_interruptible_timeout(gfmt_wait_queue, _gfmt_only_int_status, timeout_jiff);
            }
#endif        
            convResult = gfmt_drv_get_result();
            
            GFMT_MSG("Decode Result : %d, status %d!\n", convResult, _gfmt_only_int_status );

            //_gfmt_only_int_status = 0;
            if(convResult != 0)
            {
                gfmt_drv_dump_reg();
            }
            if(copy_to_user(outParams.result, &convResult, sizeof(unsigned int)))
            {
                GFMT_WRN("GFMT : Copy to user error (result)\n");
                return -EFAULT;            
            }
                
            
        break;


      
      
      case GFMT_IOCTL_DEINIT:
            GFMT_MSG("GFMT Deinitial and UnLock\n");
          // copy input parameters
          if(*pStatus != GFMT_PROCESS)
          {
              GFMT_MSG("Permission Denied! This process can not access GFMT");
              return -EFAULT;
          }

          if(fmt_status == 0)
          {
              GFMT_MSG("GFMT status is available, HOW COULD THIS HAPPEN ??");
              *pStatus = 0;
              return -EFAULT;
          }
          gfmt_drv_deinit();
          *pStatus = 0;   
          break;

      case GFMT_IOCTL_DUMP_REG:
          gfmt_drv_dump_reg();
        break;      
      
#if 0      
{
//      case GFMT_IOCTL_START :
//               
//          printk("Trigger ISP: %d, Link GFMT: %d,FMT %d, Enable GFMT: %d, YUV_DMA %d!!\n", gStartISP, gGDMALink, gFMT_en ,ispParam.gdmaEn, gISP_yuv_en);
//          gfmt_start(gGDMALink, gStartISP, gFMT_en, gISP_yuv_en);
//          
//        break; 
//      case GFMT_IOCTL_CONFIG_CTL:
//      {  
//          printk("GFMT CTL Config\n");
//          if (copy_from_user(&ctlParam, (void*)arg, sizeof(GDMA_DRV_CTL_IN)))
//          {
//              printk("Copy from user failed\n");
//              return -EFAULT;
//          }
//          if(ctlParam.isSrcGray){
//            gISP_yuv_en = 4; //disable cb/cr DMA
//          }
//           
//          gfmt_config_ctl(ctlParam);
//      }break;    
//      case GFMT_IOCTL_RW_REG:
//          gfmt_rw_reg();
//        break;
//
//      case GFMT_IOCTL_CONFIG_ISP:
//      {
//          printk("GFMT ISP Config\n");
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
//                   gfmt_isp_setup_tdri();
//              }else{
//                 gfmt_isp_setup();  
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
//      case GFMT_IOCTL_RESET:
//      
//          if (gfmt_reset_isp() < 0)
//              return -EFAULT;
//        break;
//      case GFMT_IOCTL_INIT:
//      
//          addr = CAM_REG_BASE + 0x0000;
//          *(volatile unsigned int *)(addr) = 0;
//          
//          if (gfmt_reset_isp() < 0)
//              return -EFAULT;        
//        break;
}
#endif        
      default:
          printk("GFMT NO THIS IOCTL COMMAND, %d!!\n", cmd);
        break;
    }
    return 0;


}


#if 0
static int gfmt_ioctl_from_dec(unsigned int cmd, unsigned long arg, struct file *file)
{
    unsigned int*       pStatus;
    unsigned int        convResult;
    unsigned int        chksum;
    unsigned char       whichBank;
    long timeout_jiff;
    GDMA_DEC_DRV_IN dec_params;
    GDMA_DEC_CONFIG_ROW dec_row_params ;

    unsigned int timeout = 0x1FFFFF;

    
    GDMA_DEC_DRV_OUT outParams;

    pStatus = (unsigned int*)file->private_data;

    if (NULL == pStatus)
    {
        GFMT_MSG("Private data is null in flush operation. SOME THING WRONG??\n");
        return -EFAULT;
    }
    switch(cmd)
    { 
            // initial and reset GFMT control
        case GFMT_IOCTL_INIT:   /* OT:OK */
            GFMT_MSG("GFMT Initial and Lock\n");
            if(gfmt_drv_init() == 0)
            {
                *pStatus = GFMT_PROCESS;
            }
            break;
            
        case GFMT_IOCTL_RESET:  /* OT:OK */
            GFMT_MSG("GFMT Reset\n");
            gfmt_drv_reset();
            break;
            
        case GFMT_IOCTL_CONFIG:
            GFMT_MSG("GFMT Configure Hardware\n");
            if(*pStatus != GFMT_PROCESS)
            {
                GFMT_MSG("Permission Denied! This process can not access \n");
                return -EFAULT;
            }
            if(fmt_status == 0)
            {
                GFMT_MSG("GFMT  is unlocked!!");
                *pStatus = 0;
                return -EFAULT;
            }
            if(copy_from_user(&dec_params, (void *)arg, sizeof(GDMA_DEC_DRV_IN)))
            {
                GFMT_MSG("GFMT  : Copy from user error\n");
                return -EFAULT;
            }

               
            if (gfmt_drv_dec_set_config_data(&dec_params) < 0)
                return -EFAULT;

            break;
            
            case GFMT_IOCTL_RESUME:
            GFMT_MSG("GFMT  RESUME Hardware\n");            
              if(*pStatus != GFMT_PROCESS)
              {
                  GFMT_MSG("Permission Denied! This process can not access \n");
                  return -EFAULT;
              }
              if(fmt_status == 0)
              {
                  GFMT_MSG("GFMT  is unlocked!!");
                  *pStatus = 0;
                  return -EFAULT;
              }
              if(copy_from_user(&dec_row_params, (void *)arg, sizeof(GDMA_DEC_CONFIG_ROW)))
              {
                  GFMT_MSG("GFMT  : Copy from user error\n");
                  return -EFAULT;
              }
              
              gfmt_drv_dec_set_dst_bank0( dec_row_params.decRowBuf[0], dec_row_params.decRowBuf[1], dec_row_params.decRowBuf[2]);
      
              gfmt_drv_dec_set_pause_mcu_idx(dec_row_params.pauseMCU -1) ;
            
              gfmt_drv_dec_resume(BIT_INQST_MASK_PAUSE);
            
            break;
            
            


        case GFMT_IOCTL_START:    /* OT:OK */
            GFMT_MSG("GFMT  Start\n");
            
            //Debug: printk("0xF0: 0x%08x\n", *(volatile unsigned int*)(GDMA_DEC_BASE + 0xF0));
            
            gfmt_drv_dec_start();
            break;
            
        case GFMT_IOCTL_WAIT:
            GFMT_MSG("GFMT  Wait\n");


            if(*pStatus != GFMT_PROCESS)
            {
                GFMT_WRN("Permission Denied! This process can not access ");
                return -EFAULT;
            }
            if(fmt_status == 0)
            {
                GFMT_WRN("GFMT status is available, HOW COULD THIS HAPPEN ??");
                *pStatus = 0;
                return -EFAULT;
            }           
            if(copy_from_user(&outParams, (void *)arg, sizeof(GDMA_DEC_DRV_OUT)))
            {
                GFMT_WRN("GFMT : Copy from user error\n");
                return -EFAULT;
            }

            //set timeout
            timeout_jiff = outParams.timeout* HZ / 1000;
            GFMT_MSG("GFMT Time Jiffies : %ld\n", timeout_jiff);   
#ifdef FPGA_VERSION
//#if 1

            GFMT_MSG("Polling GFMT Status");

            do
            {
                _gfmt_only_int_status = REG_JPGDEC_INTERRUPT_STATUS;
            } while(_gfmt_only_int_status == 0);
#else

            if(outParams.timeout >= 5000){
                   
              GFMT_MSG("Polling GFMT Status");              
              do
              {
                  _gfmt_only_int_status = REG_JPGDEC_INTERRUPT_STATUS;
                 timeout--;
              } while(_gfmt_only_int_status == 0 && timeout != 0);                              
              if(timeout == 0) GFMT_MSG("Polling GFMT Status TIMEOUT!!\n");              
            }else{
              GFMT_MSG("Polling GFMT Status");
              wait_event_interruptible_timeout(gfmt_wait_queue, _gfmt_only_int_status, timeout_jiff);
            }
#endif
            
            convResult = gfmt_drv_dec_get_result();


#if 0                                
            if( !dec_params.pauseRow_en ){
              if (gfmt_drv_dec_wait(&dec_params) == 0)
                  return -EFAULT;               
            }else{
              if (gfmt_drv_dec_wait_one_row(&dec_params) == 0)
                  return -EFAULT;                              
            }
#endif            

            GFMT_MSG("Decode Result : %d, status %d!\n", convResult, _gfmt_only_int_status );
            //_gfmt_only_int_status = 0;
            if(convResult >= 2)
            {
                gfmt_drv_dec_dump_reg();
            }
            if(copy_to_user(outParams.result, &convResult, sizeof(unsigned int)))
            {
                GFMT_WRN("GFMT : Copy to user error (result)\n");
                return -EFAULT;            
            }
    
            break;


        case GFMT_IOCTL_BREAK:
            if (gfmt_drv_dec_break() < 0)
                return -EFAULT;
            break;
            
        case GFMT_IOCTL_DUMP_REG:
            gfmt_drv_dec_dump_reg();
            break;


        case GFMT_IOCTL_DEINIT:
            // copy input parameters
            if(*pStatus != GFMT_PROCESS)
            {
                printk("Permission Denied! This process can not access encoder");
                return -EFAULT;
            }

            if(fmt_status == 0)
            {
                printk("GDMA_CTL status is available, HOW COULD THIS HAPPEN ??");
                *pStatus = 0;
                return -EFAULT;
            }
            gfmt_drv_deinit();
            *pStatus = 0;   
            break;
#ifdef FOR_COMPILE            
        case GFMT_IOCTL_RW_REG: /* OT:OK */
            gfmt_drv_dec_rw_reg();
            break;
#endif
        default:
            printk("GFMT DEC IOCTL NO THIS COMMAND\n");
            break;
    }
    return 0;
}
#endif




//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
//static int gfmt_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
static long gfmt_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {

        case GFMT_IOCTL_CONFIG_FMT  : 
        case GFMT_IOCTL_START       : 
        case GFMT_IOCTL_WAIT        : 
        case GFMT_IOCTL_RESET       : 
        case GFMT_IOCTL_INIT        : 
        case GFMT_IOCTL_DUMP_REG    : 
        case GFMT_IOCTL_DEINIT      : 
         
         
          return gfmt_ioctl(cmd, arg, file);
  


        default :
        //case GFMT_IOCTL_CONFIG_ISP  : 
        //case GFMT_IOCTL_RW_REG      : 
        //case GFMT_IOCTL_CONFIG_CTL  : 
         
            break; 
    }
    
    return -EINVAL;
}

static int gfmt_open(struct inode *inode, struct file *file)
{
    unsigned int *pStatus;
    //Allocate and initialize private data
    file->private_data = kmalloc(sizeof(unsigned int) , GFP_ATOMIC);

    if(NULL == file->private_data)
    {
        GFMT_WRN("Not enough entry for GFMT open operation\n");
        return -ENOMEM;
    }

    pStatus = (unsigned int *)file->private_data;
    *pStatus = 0;
    
    return 0;
}

static ssize_t gfmt_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    GFMT_MSG("gfmt driver read\n");
    return 0;
}

static int gfmt_release(struct inode *inode, struct file *file)
{
    if(NULL != file->private_data)
    {
        kfree(file->private_data);
        file->private_data = NULL;
    }
    return 0;
}

static int gfmt_flush(struct file * a_pstFile , fl_owner_t a_id)
{
    unsigned int *pStatus;

    pStatus = (unsigned int*)a_pstFile->private_data;

    if(NULL == pStatus)
    {
        GFMT_WRN("Private data is null in flush operation. HOW COULD THIS HAPPEN ??\n");
        return -EFAULT;
    }

    if (*pStatus == GFMT_PROCESS)
    {
        if(fmt_status != 0) 
        {
            GFMT_WRN("Error! Enable error handling for gfmt !");
            gfmt_drv_deinit();
        }
    }


    return 0;
}

/* Kernel interface */
static struct file_operations gfmt_fops = {
	.owner		= THIS_MODULE,
	//.ioctl		= gfmt_ioctl,
	.unlocked_ioctl = gfmt_unlocked_ioctl,
	.open		= gfmt_open,
	.release	= gfmt_release,
	.flush		= gfmt_flush,
	.read       = gfmt_read,
};

static int gfmt_probe(struct platform_device *pdev)
{
    struct class_device;
    
	int ret;
    struct class_device *class_dev = NULL;
    
    GFMT_MSG("-------------gfmt driver probe-------\n");
	ret = alloc_chrdev_region(&gfmt_devno, 0, 1, GFMT_DEVNAME);

	if(ret)
	{
	    GFMT_ERR("Error: Can't Get Major number for GFMT Device\n");
	}
	else
	{
	    GFMT_MSG("Get GFMT Device Major number (%d)\n", gfmt_devno);
    }

	gfmt_cdev = cdev_alloc();
    gfmt_cdev->owner = THIS_MODULE;
	gfmt_cdev->ops = &gfmt_fops;

	ret = cdev_add(gfmt_cdev, gfmt_devno, 1);

    gfmt_class = class_create(THIS_MODULE, GFMT_DEVNAME);
    class_dev = (struct class_device *)device_create(gfmt_class, NULL, gfmt_devno, NULL, GFMT_DEVNAME);

    spin_lock_init(&gdma_fmt_lock);

    // initial , register ISR
    fmt_status = 0;
    _gfmt_only_int_status = 0;

#ifdef GDMA_FMT_IRQ
    init_waitqueue_head(&gfmt_wait_queue);  
    

    enable_irq(CAMERA_ISP_IRQ3_ID);
    if(request_irq(CAMERA_ISP_IRQ3_ID, gfmt_drv_isr, IRQF_TRIGGER_LOW, "gfmt_driver" , NULL))
    {
        GFMT_ERR("GFMT Driver request irq failed\n");
    }
#endif
	GFMT_MSG("GFMT Probe Done\n");

	NOT_REFERENCED(class_dev);
	return 0;
}

static int gfmt_remove(struct platform_device *pdev)
{
	GFMT_MSG("GFMT remove\n");
	//unregister_chrdev(GDMA_MAJOR, GDMA_DEVNAME);
#ifdef GDMA_FMT_IRQ
	free_irq(CAMERA_ISP_IRQ3_ID, NULL);
#endif
	GFMT_MSG("Done\n");
	return 0;
}

static void gfmt_shutdown(struct platform_device *pdev)
{
	GFMT_MSG("GFMT shutdown\n");
	/* Nothing yet */
}

/* PM suspend */
static int gfmt_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    gfmt_drv_deinit();
    return 0;
}

/* PM resume */
static int gfmt_resume(struct platform_device *pdev)
{
    return 0;
}


static struct platform_driver gfmt_driver = {
	.probe		= gfmt_probe,
	.remove		= gfmt_remove,
	.shutdown	= gfmt_shutdown,
	.suspend	= gfmt_suspend,
	.resume		= gfmt_resume,
	.driver     = {
	              .name = GFMT_DEVNAME,
	},
};

static void gfmt_device_release(struct device *dev)
{
	// Nothing to release? 
}

static u64 gfmt_dmamask = ~(u32)0;

static struct platform_device gfmt_device = {
	.name	 = GFMT_DEVNAME,
	.id      = 0,
	.dev     = {
		.release = gfmt_device_release,
		.dma_mask = &gfmt_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources = 0,
};

static int __init gfmt_init(void)
{
    int ret;

    GFMT_MSG("GFMT initialize\n");
	
    GFMT_MSG("Register the GFMT device\n");
	if(platform_device_register(&gfmt_device))
	{
        GFMT_ERR("failed to register gfmt device\n");
        ret = -ENODEV;
        return ret;
	}

    GFMT_MSG("Register the GFMT driver\n");    
    if(platform_driver_register(&gfmt_driver))
    {
        GFMT_ERR("failed to register gfmt driver\n");
        platform_device_unregister(&gfmt_device);
        ret = -ENODEV;
        return ret;
    }

    return 0;
}

static void __exit gfmt_exit(void)
{
    cdev_del(gfmt_cdev);
    unregister_chrdev_region(gfmt_devno, 1);
	  //GFMT_MSG("Unregistering driver\n");
    platform_driver_unregister(&gfmt_driver);
	  platform_device_unregister(&gfmt_device);
	
	  device_destroy(gfmt_class, gfmt_devno);
	  class_destroy(gfmt_class);
	  
	  GFMT_MSG("Done\n");
}

module_init(gfmt_init);
module_exit(gfmt_exit);
MODULE_AUTHOR("Hao-Ting Huang <otis.huang@mediatek.com>");
MODULE_DESCRIPTION("MT6573 GFMT Driver");
MODULE_LICENSE("GPL");
