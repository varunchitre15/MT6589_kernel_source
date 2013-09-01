


#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/mm.h>
#include <linux/slab.h>

//#include <mach/mt8320_irq.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_irq.h>
#include <mach/irqs.h>
#include <mach/mt_clkmgr.h>

#include "gdma_drv.h"
#include "gdma_drv_6589_reg.h"

extern void wait_pr(void);



void gdma_wait(void)
{
    unsigned int value;
    
    value = REG_GDMA_FMT_INTEN;
   
   
}

int gdma_reset_isp(void)
{
    unsigned int u4Reg;
    unsigned int u4Timeout = 0x0FFFFFF;

   
    REG_ISP_CTL_SW_CTL = 1;    
    do
    {
        u4Reg = REG_ISP_CTL_SW_CTL;
        u4Timeout--;
        if (u4Timeout == 0)
            break;
    } while(!(u4Reg & 0x2));
    
    
    //REG_ISP_CTL_SW_CTL = 5;
    REG_ISP_CTL_SW_CTL = 4;
    REG_ISP_CTL_SW_CTL = 0;

    if (u4Timeout == 0)
    {
        printk("GDMA Reset Failed\n");
        return -1;
    }


    printk("GDMA SW/HW Reset Done\n");
    
    return 0;
}

void gdma_dump_vdec(void)
{
   
    unsigned int regValue = 0;
    unsigned int index;   

//VLD_TOP (18~45):  0x15011872 ~ 0x15011980
//PP      (17~28):  0x15015068 ~ 0x15015112
//MC      (142)  :  0x15012568  

//VLD_TOP (18~45):  0x15011848 ~ 0x150118B4
//PP      (17~28):  0x15015044 ~ 0x15015070
//MC      (142)  :  0x1501208E
//VP8     (93~96):  0x15016974 ~ 0x15016980  






   
    printk("=======================================\n\r");
    printk("   VDEC_TOP Register Dump\n\r");
    for (index = 0x1848; index <= 0x19B4; index+=4)
    {
        regValue = *(volatile unsigned int *)(0xF5010000 + index);
        printk("addr %03x(%03d) %08x\n", 0x15010000+index, index/4, regValue);        
        wait_pr();
    }

    printk("=======================================\n\r");
    printk("   PP Register Dump\n\r");
    for (index = 0x5044; index <= 0x5070; index+=4)
    {
        regValue = *(volatile unsigned int *)(0xF5010000 + index);
        printk("addr %03x(%03d) %08x\n", 0x15010000+index, index/4, regValue);        
        wait_pr();
    }
   
    printk("=======================================\n\r");
    printk("   MC Register Dump\n\r");
    index = 0x208E ;
    //for (index = 0x1872; index <= 0x1980; index+=4)
    {
        regValue = *(volatile unsigned int *)(0xF5010000 + index);
        printk("addr %03x(%03d) %08x\n", 0x15010000+index, index/4, regValue);        
        wait_pr();
    }   
    
    printk("=======================================\n\r");
    printk("   VP8 Register Dump\n\r");
    for (index = 0x6974; index <= 0x6980; index+=4)
    {
        regValue = *(volatile unsigned int *)(0xF5010000 + index);
        printk("addr %03x(%03d) %08x\n", 0x15010000+index, index/4, regValue);        
        wait_pr();
    }
       
   
}

void gdma_dump_reg(void)
{
    unsigned int regValue = 0;
    unsigned int index;

    printk("=======================================\n\r");
    printk("   GDMA Register Dump\n\r");
    for (index = 0x50; index < REG_GDMA_RANGE; index+=4)
    {
        regValue = *(volatile unsigned int *)(GDMA_REG_BASE + index);
        printk("addr %03x(%03d) %08x\n", index, index/4, regValue);        
        wait_pr();
    }

    printk("0x000 %08x\n", REG_ISP_STA_CTL);wait_pr();
    printk("0x008 %08x\n", REG_ISP_CTL_EN2);wait_pr();
    printk("0x018 %08x\n", REG_ISP_CTL_SEL);wait_pr();
    
    printk("=======================================\n\r\n\r");
    printk("   ISP CTL Register Dump\n\r");
    for (index = 0x000; index < 0x188; index+=4)
    {
        regValue = *(volatile unsigned int *)(ISP_REG_BASE + index);
        printk("addr %03x(%03d) %08x\n", index, index/4, regValue);        
        wait_pr();
    }
    printk("=======================================\n\r\n\r");
    printk("   DISPO Register Dump\n\r");
    for (index = 0xD00; index < 0xDA8; index+=4)
    {
        regValue = *(volatile unsigned int *)(ISP_REG_BASE + index);
        printk("addr %03x(%03d) %08x\n", index, index/4, regValue);        
        wait_pr();
    }
    printk("=======================================\n\r\n\r");

    //dump RDMA REG
    printk("   RDMA Register Dump\n\r");
    for (index = 0x200; index < 0x400; index+=4)
    {
        regValue = *(volatile unsigned int *)(ISP_REG_BASE + index);
        printk("addr %03x(%03d) %08x\n", index, index/4, regValue);        
        wait_pr();
    }
    printk("=======================================\n\r\n\r");    


//dump FMT register
    printk("=======================================\n\r\n\r");    

    printk("  GDMA_FMT Register Dump\n\r");
    index = 0xF5004160 ;
    regValue = 0x00008502 ;
    IMG_REG_WRITE(regValue, index);    //*(volatile unsigned int *)( index ) = regValue ;
    printk("SET addr %03x = %08x\n", index, regValue);        
    wait_pr();
    
    index = 0xF5004164 ;
    regValue = *(volatile unsigned int *)( index );
    printk("GET addr %03x = %08x\n", index, regValue);        
    wait_pr();

    index = 0xF5004160 ;
    regValue = 0x00008402 ;
    IMG_REG_WRITE(regValue, index);    //*(volatile unsigned int *)( index ) = regValue ;
    printk("SET addr %03x = %08x\n", index, regValue);        
    wait_pr();
    
    index = 0xF5004164 ;
    regValue = *(volatile unsigned int *)( index );
    printk("GET addr %03x = %08x\n", index, regValue);        
    wait_pr();
    printk("=======================================\n\r\n\r");    


}



void gdma_rw_reg(void)
{
    unsigned int i;
    unsigned int addr = 0;
    //char msg[256];

    printk("=======================================\n\r");
    printk("   GDMA register RW test!!!!\n\r");

    for (i = 0x50; i < 0xDC; i+=4)
    {
        addr = GDMA_REG_BASE + i;
        printk("addr %03x(%03d) ", i, i/4);
        
        IMG_REG_WRITE(0x00000000, addr);    //*((volatile unsigned int*)addr) = 0x00000000;
        printk("write 0x00000000 read: 0x%08x\n", *((volatile unsigned int*)addr));
        
        IMG_REG_WRITE(0xffffffff, addr);    //*((volatile unsigned int*)addr) = 0xffffffff;
        printk("              write 0xffffffff read: 0x%08x\n", *((volatile unsigned int*)addr));
        wait_pr();
    }

    printk("=======================================\n\r\n\r");

}



void gdma_drv_reset(void)
{
 ;   
}


void gdma_drv_set_ctl_mode(unsigned int isJPEG, unsigned int is422, unsigned int isGray, unsigned int CbCrValue)
{
    unsigned int u4Value;
    
    u4Value = (CbCrValue << 8) | (isGray << 5) | (is422 << 3) | isJPEG ;
    
    IMG_REG_WRITE(u4Value, REG_ADDR_GDMA_CTL_IOTYPE);    //REG_GDMA_CTL_IOTYPE = (CbCrValue << 8) | (isGray << 5) | (is422 << 3) | isJPEG ;    
    
}



void gdma_drv_set_src_buf_height (unsigned int y_h, unsigned int c_h, unsigned int y_h_lst, unsigned int c_h_lst, unsigned int y_h_fst, unsigned int c_h_fst )
{
    unsigned int u4Value;
   
   u4Value =               (    y_h << 28) |(    c_h << 24) 
                         | (y_h_lst << 17) |(c_h_lst << 12)
                         | (y_h_fst <<  5) |(c_h_fst);
                         
   IMG_REG_WRITE(u4Value, REG_ADDR_GDMA_CTL_MCUMODE);    
   
}


void gdma_drv_set_crop_offset(unsigned int initOffset_Y, unsigned int offset_Y, unsigned int initOffset_C, unsigned int offset_C)
{
   unsigned int u4Value;
   u4Value = (initOffset_Y << 16) | offset_Y ;
   IMG_REG_WRITE(u4Value, REG_ADDR_GDMA_CTL_SRCOFST_Y);  //REG_GDMA_CTL_SRCOFST_Y = (initOffset_Y << 16) | offset_Y ;
   u4Value = (initOffset_C << 16) | offset_C ;
   IMG_REG_WRITE(u4Value, REG_ADDR_GDMA_CTL_SRCOFST_C);  //REG_GDMA_CTL_SRCOFST_C = (initOffset_C << 16) | offset_C ;
   
}



void gdma_drv_set_src_bank0(unsigned int addr_Y, unsigned int addr_U,unsigned int addr_V)
{
   
   
   IMG_REG_WRITE(addr_Y, REG_ADDR_GDMA_CTL_YSRCB1  );  //REG_GDMA_CTL_YSRCB1  = addr_Y ;
   IMG_REG_WRITE(addr_U, REG_ADDR_GDMA_CTL_CBSRCB1 );  //REG_GDMA_CTL_CBSRCB1 = addr_U ;
   IMG_REG_WRITE(addr_V, REG_ADDR_GDMA_CTL_CRSRCB1 );  //REG_GDMA_CTL_CRSRCB1 = addr_V ;
   
}


void gdma_drv_set_src_bank1(unsigned int addr_Y, unsigned int addr_U,unsigned int addr_V)
{
   
   
   IMG_REG_WRITE(addr_Y, REG_ADDR_GDMA_CTL_YSRCB2  );  //REG_GDMA_CTL_YSRCB2  = addr_Y ;
   IMG_REG_WRITE(addr_U, REG_ADDR_GDMA_CTL_CBSRCB2 );  //REG_GDMA_CTL_CBSRCB2 = addr_U ;
   IMG_REG_WRITE(addr_V, REG_ADDR_GDMA_CTL_CRSRCB2 );  //REG_GDMA_CTL_CRSRCB2 = addr_V ;
   
}

void gdma_drv_set_src_image_info(unsigned int width, unsigned int height)
{
   
  IMG_REG_WRITE( ((width << 16) | (height)), REG_ADDR_GDMA_CTL_YSRCSZ  );  //REG_GDMA_CTL_YSRCSZ = (width << 16) | (height) ;
  
}

void gdma_drv_set_spare (unsigned int spare)
{
   
  IMG_REG_WRITE( (spare), REG_ADDR_GDMA_CTL_SPARE  );  //REG_GDMA_CTL_SPARE = spare ;
   
}


void gdma_config_ctl(GDMA_DRV_CTL_IN *cfgCTL)
{
   
   gdma_drv_set_ctl_mode(cfgCTL->isSrcJpeg, cfgCTL->gtSrcIs422, cfgCTL->isSrcGray, cfgCTL->cbcrConstant) ;
   
   gdma_drv_set_src_image_info(cfgCTL->gtSrcWidth, cfgCTL->gtSrcHeight) ;
   
   gdma_drv_set_src_buf_height(cfgCTL->gtSrcBufHeight[0], cfgCTL->gtSrcBufHeight[1], 
                               cfgCTL->gtSrcBufLastHeight[0], cfgCTL->gtSrcBufLastHeight[1], cfgCTL->gtSrcBufFstHeight[0], cfgCTL->gtSrcBufFstHeight[1]);
   
   gdma_drv_set_crop_offset(cfgCTL->cropY_initOff, cfgCTL->cropY_Off, cfgCTL->cropC_initOff, cfgCTL->cropC_Off);
   
   gdma_drv_set_src_bank0(cfgCTL->gtSrcBank0[0], cfgCTL->gtSrcBank0[1], cfgCTL->gtSrcBank0[2]);
   
   gdma_drv_set_src_bank1(cfgCTL->gtSrcBank1[0], cfgCTL->gtSrcBank1[1], cfgCTL->gtSrcBank1[2]);
   
   
}


//void gdma_config_ctl(GDMA_DRV_CTL_IN cfgCTL)
//{
//    unsigned int regVal;
//
//    regVal = (cfgCTL.cbcrConstant << 8) | (cfgCTL.isSrcGray << 5) | (cfgCTL.gtSrcIs422 << 3) | cfgCTL.isSrcJpeg ;    
//    REG_GDMA_CTL_IOTYPE = regVal;
//
//    regVal = (cfgCTL.yWidth << 16) | cfgCTL.yHeight;
//    REG_GDMA_CTL_YSRCSZ = regVal;
//
//    regVal = (cfgCTL.mcuYVRatio << 28) | (cfgCTL.mcuCVRatio << 24) | (cfgCTL.lastMcuYCnt << 17) |
//        (cfgCTL.lastMcuCCnt << 12) | (cfgCTL.fstMcuYCnt << 5) | (cfgCTL.fstMcuCCnt);
//    REG_GDMA_CTL_MCUMODE = regVal;
//
//    regVal = (cfgCTL.cropY_initOff << 16) | (cfgCTL.cropY_Off);
//    REG_GDMA_CTL_SRCOFST_Y = regVal;
//
//    regVal = (cfgCTL.cropC_initOff << 16) | (cfgCTL.cropC_Off);
//    REG_GDMA_CTL_SRCOFST_C = regVal;
//
//    REG_GDMA_CTL_YSRCB1 = cfgCTL.gtSrcBank0[0];
//    REG_GDMA_CTL_CBSRCB1 = cfgCTL.gtSrcBank0[1];
//    REG_GDMA_CTL_CRSRCB1 = cfgCTL.gtSrcBank0[2];    
//
//    REG_GDMA_CTL_YSRCB2 = cfgCTL.gtSrcBank1[0];
//    REG_GDMA_CTL_CBSRCB2 = cfgCTL.gtSrcBank1[1];
//    REG_GDMA_CTL_CRSRCB2 = cfgCTL.gtSrcBank1[2];
//}


#if 0
void gdma_start(unsigned char gdmaLink, unsigned char startISP, unsigned char FMT_en, unsigned char colorFormat )
{
    unsigned int regVal;
    unsigned int fmt_only = ((!gdmaLink) && FMT_en) ;
    
    regVal = REG_ISP_CTL_SEL;
    regVal = (regVal & ~REG_ISP_CTL_SEL_GDMA_LNK_MASK) | (gdmaLink << REG_ISP_CTL_SEL_GDMA_LNK_SHIFT);
    REG_ISP_CTL_SEL = regVal;

    if(gdmaLink){
      /** FIX!!!! **/
      regVal = REG_ISP_CTL_DMA_EN;
      //regVal = (regVal) | (REG_ISP_CTL_DMA_EN_IMGI_EN) | REG_ISP_CTL_DMA_EN_VIPI_EN | REG_ISP_CTL_DMA_EN_VIP2I_EN; // 3plane Enable
      
      if( colorFormat & 0x4 )
        regVal = (regVal) | (REG_ISP_CTL_DMA_EN_IMGI_EN) ; 
      else
        regVal = (regVal) & (~REG_ISP_CTL_DMA_EN_IMGI_EN) ; 
        
      if( colorFormat & 0x2 )
        regVal = (regVal)  | REG_ISP_CTL_DMA_EN_VIPI_EN ; 
      else
        regVal = (regVal)  & (~REG_ISP_CTL_DMA_EN_VIPI_EN) ; 
        
      if( colorFormat & 0x1 )         
        regVal = (regVal)  | REG_ISP_CTL_DMA_EN_VIP2I_EN; 
      else
        regVal = (regVal)  & (~REG_ISP_CTL_DMA_EN_VIP2I_EN); 
      
      
      REG_ISP_CTL_DMA_EN = regVal;    
    }

    regVal = REG_ISP_CTL_EN2;
    regVal = (regVal & ~REG_ISP_CTL_EN2_FMT_EN_MASK) | (FMT_en << REG_ISP_CTL_EN2_FMT_EN_SHIFT); // FMT Enable
    REG_ISP_CTL_EN2 = regVal;

    regVal = REG_ISP_CTL_EN2;
    regVal = (regVal & ~REG_ISP_CTL_EN2_GDMA_EN_MASK) | (gdmaLink << REG_ISP_CTL_EN2_GDMA_EN_SHIFT); // GDMA Enable
    REG_ISP_CTL_EN2 = regVal;

    //regVal = REG_ISP_CTL_EN2;
    //regVal = (regVal) | (0 << 21); // prz disable
    //REG_ISP_CTL_EN2 = regVal;
    

    regVal = REG_GDMA_FMT_INTEN;
    regVal = (regVal & ~INT_EN_MASK) | (fmt_only << INT_EN_SHIFT);
    REG_GDMA_FMT_INTEN = regVal;
    
    

    regVal = REG_ISP_STA_CTL;
    printk("GDMA driver start %d!!\n", regVal);
    regVal = (regVal & ~REG_ISP_STR_CTL_GDMA_MASK) | (fmt_only << REG_ISP_STR_CTL_GDMA_SHIFT);
    regVal = (regVal & ~REG_ISP_STR_CTL_ISP_MASK) | (startISP << REG_ISP_STR_CTL_ISP_SHIFT); // ISP Start
    REG_ISP_STA_CTL = regVal;

}
#endif


