
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
#include "gfmt_drv.h"
#include "gdma_drv_6589_reg.h"

extern void wait_pr(void);

kal_uint32 _gfmt_only_int_status = 0;

#if 1
int gfmt_isr_lisr(void)
{
    unsigned int tmp, tmp1 ;
    
    tmp1 = REG_GDMA_FMT_INTEN;
    tmp = tmp1 & CHK_GFMT_DONE_MASK ;
   
   if (tmp)
   {
      _gfmt_only_int_status = tmp ;
      REG_GDMA_FMT_INTEN = tmp1;
      return 0 ;
   }
     

   return -1;
}

#else
//write one clear
int gfmt_isr_lisr(void)
{
    
    _gfmt_only_int_status = REG_GDMA_FMT_INTEN;

   //printk("enter jpeg_isr_dec_lisr %d, mode %d!!\n", _jpeg_dec_int_status, _jpeg_dec_mode);

   if (_gfmt_only_int_status & CHK_GFMT_DONE_MASK)
   {  
   
        /// clear the interrupt status register
        IMG_REG_WRITE(0, REG_ADDR_GDMA_FMT_INTEN);    //REG_GDMA_FMT_INTEN = 0;        
        return 0;

   }

   return -1;
}
#endif

void writeReg(unsigned int offset, unsigned int value)
{
    unsigned int addr;
#if 0
    if(offset >= 0x09000 && offset <= 0x0B000){
      printk("SKIP_REG_WR: %08x %08x\n", offset, value);
      wait_pr();         
      return ;
   }
#endif    

    addr = CAM_REG_BASE + offset;
    *((volatile unsigned int*)addr) = value;

    //printk("%08x %08x\n", addr, value);
    wait_pr();
}



void gfmt_drv_dump_reg(void)
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

//dump FMT register
    printk("=======================================\n\r\n\r");    

    printk("  GDMA_FMT Register Dump\n\r");
    index = 0xF5004160 ;
    regValue = 0x00008502 ;
    *(volatile unsigned int *)( index ) = regValue ;
    printk("SET addr %03x = %08x\n", index, regValue);        
    wait_pr();
    
    index = 0xF5004164 ;
    regValue = *(volatile unsigned int *)( index );
    printk("GET addr %03x = %08x\n", index, regValue);        
    wait_pr();

    index = 0xF5004160 ;
    regValue = 0x00008902 ;
    *(volatile unsigned int *)( index ) = regValue ;
    printk("SET addr %03x = %08x\n", index, regValue);        
    wait_pr();
    
    index = 0xF5004164 ;
    regValue = *(volatile unsigned int *)( index );
    printk("GET addr %03x = %08x\n", index, regValue);        
    wait_pr();

    index = 0xF5004160 ;
    regValue = 0x00008402 ;
    *(volatile unsigned int *)( index ) = regValue ;
    printk("SET addr %03x = %08x\n", index, regValue);        
    wait_pr();
    
    //bits[11] : done status
    
    
    index = 0xF5004164 ;
    regValue = *(volatile unsigned int *)( index );
    printk("GET addr %03x = %08x\n", index, regValue);        
    wait_pr();
    printk("=======================================\n\r\n\r");   

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


 


}



void gfmt_drv_reset(void)
{

   //unsigned int addr;
   //addr = CAM_REG_BASE + 0x0000;
   //*(volatile unsigned int *)(addr) = 0;

     writeReg(0x00004E90, 0x00000F00);	/* 0x15004E90: CAM_FMT_IOTYPE */      
     writeReg(0x00004E94, 0x00000220);	/* 0x15004E94: CAM_FMT_MCUMODE */     
     writeReg(0x00004E98, 0x00000220);	/* 0x15004E98: CAM_FMT_WEBPMODE */    
     writeReg(0x00004E9C, 0x02008883);	/* 0x15004E9C: CAM_FMT_ULTRAMODE */   
     writeReg(0x00004EA0, 0x00000800);	/* 0x15004EA0: CAM_FMT_MIFMODE */     
//   writeReg(0x00004EA4, 0x00028028);	/* 0x15004EA4: CAM_FMT_SRCBUFL */     
     writeReg(0x00004EA8, 0x00000000);	/* 0x15004EA8: CAM_FMT_WINTFON */     
//   writeReg(0x00004EAC, 0x00014028);	/* 0x15004EAC: CAM_FMT_TGBUFL */      
//   writeReg(0x00004EB0, 0x0007BEF0);	/* 0x15004EB0: CAM_FMT_YSRCB1 */      
//   writeReg(0x00004EB4, 0x000CBF00);	/* 0x15004EB4: CAM_FMT_CBSRCB1 */     
//   writeReg(0x00004EB8, 0x18C7BB53);	/* 0x15004EB8: CAM_FMT_PREULTRAMODE */
//   writeReg(0x00004EBC, 0x000F3F04);	/* 0x15004EBC: CAM_FMT_YTGB1 */       
//   writeReg(0x00004EC0, 0x00143F08);	/* 0x15004EC0: CAM_FMT_CBTGB1 */      
//   writeReg(0x00004EC4, 0x00157F0C);	/* 0x15004EC4: CAM_FMT_CRTGB1 */      
//   writeReg(0x00004EC8, 0x000F6704);	/* 0x15004EC8: CAM_FMT_YTGB2 */       
//   writeReg(0x00004ECC, 0x00145308);	/* 0x15004ECC: CAM_FMT_CBTGB2 */      
//   writeReg(0x00004ED0, 0x0015930C);	/* 0x15004ED0: CAM_FMT_CRTGB2 */      
//   writeReg(0x00004ED4, 0x028001E0);	/* 0x15004ED4: CAM_FMT_YSRCSZ */      
     writeReg(0x00004ED8, 0x00100D00);	/* 0x15004ED8: CAM_FMT_RANGE */       
     writeReg(0x00004EDC, 0x00000010);	/* 0x15004EDC: CAM_FMT_INTEN */     

#if 0   
   if (gfmt_reset_isp() < 0)
       return -EFAULT;    
       
#endif
       
}



void gfmt_drv_set_io_type(unsigned int isLinkGDMA, unsigned int isInterlaced, unsigned int isTopField, unsigned int fixValue)
{
    unsigned int u4Value;
    //fixValue = 0x0f ;
    u4Value = (isLinkGDMA << 17) |  (isInterlaced << 4) | (isTopField << 2) | ((fixValue&0x0f)<<8);
    
    IMG_REG_WRITE(u4Value, REG_ADDR_GDMA_FMT_IOTYPE);    
    
}


void gfmt_drv_set_ultra_mode(unsigned int ultra_value)
{
    
    IMG_REG_WRITE(ultra_value, REG_ADDR_GDMA_FMT_ULTRA_MODE);    //REG_GDMA_FMT_ULTRA_MODE = ultra_value;
}


    

void gfmt_drv_set_dram_burst_length(unsigned int burst)
{
    
    IMG_REG_WRITE((burst << REG_GDMA_BST_LIM_SHIFT), REG_ADDR_GDMA_FMT_MIFMODE);    //REG_GDMA_FMT_MIFMODE = (burst << REG_GDMA_BST_LIM_SHIFT);
}

    
void gfmt_drv_set_webp_mode(unsigned int isWebp)
{
    
    IMG_REG_WRITE((isWebp), REG_ADDR_GDMA_FMT_WINTFON);    //REG_GDMA_FMT_WINTFON = isWebp;
}
    
    
void gfmt_drv_set_input_video_row_height(unsigned int height_Y, unsigned int height_C)
{
    unsigned int u4Value;

    
    u4Value = ((REG_GDMA_MCUMODE_MCUVFAC_MASK & height_Y)<<8) | ((REG_GDMA_MCUMODE_MCUVFAC_MASK & height_C) <<4 );
    
    IMG_REG_WRITE((u4Value), REG_ADDR_GDMA_FMT_MCUMODE);    
}



void gfmt_drv_set_input_webp_row_height(unsigned int height_Y, unsigned int height_C)
{
    unsigned int u4Value;


    u4Value = (height_Y << REG_GDMA_FMT_WEBPMODE_MCUY_SHIFT) | (height_C << REG_GDMA_FMT_WEBPMODE_MCUC_SHIFT);
    
    IMG_REG_WRITE((u4Value), REG_ADDR_GDMA_FMT_WEBPMODE); 
}



void gfmt_drv_set_pixel_range(unsigned int rangeMapY, unsigned int rangeMapC, unsigned int rangeReduceEn, unsigned int rangeMapEn)
{
    unsigned int u4Value;

    
    u4Value = (rangeMapY << 16) | (rangeMapC << 8) | (rangeReduceEn << 4) | rangeMapEn;  
    
    IMG_REG_WRITE((u4Value), REG_ADDR_GDMA_FMT_RANGE); 
}

void gfmt_drv_set_src_mem_stride(unsigned int stride_Y, unsigned int stride_UV)
{
    unsigned int u4Value;


    u4Value = (stride_UV << REG_GDMA_SRCBUFL_C_SHIFT) | (stride_Y & REG_GDMA_SRCBUFL_MASK);
    
    IMG_REG_WRITE((u4Value), REG_ADDR_GDMA_FMT_SRCBUFL); 
   
}


void gfmt_drv_set_src_buffer(unsigned int addr_Y, unsigned int addr_UV)
{
   
   
   IMG_REG_WRITE((addr_Y ), REG_ADDR_GDMA_FMT_YSRCB1 ); //REG_GDMA_FMT_YSRCB1  = addr_Y ;
   IMG_REG_WRITE((addr_UV), REG_ADDR_GDMA_FMT_CBSRCB1); //REG_GDMA_FMT_CBSRCB1 = addr_UV ;
   
}

void gfmt_drv_set_dst_mem_stride(unsigned int stride_Y, unsigned int stride_UV)
{
    unsigned int u4Value;

    
    u4Value = (stride_UV << REG_GDMA_TGTBUFL_C_SHIFT) | (REG_GDMA_TGTBUFL_MASK & stride_Y); 
    
    IMG_REG_WRITE((u4Value ), REG_ADDR_GDMA_FMT_TGBUFL ); 
   
}


void gfmt_drv_set_dst_bank0(unsigned int addr_Y, unsigned int addr_U,unsigned int addr_V)
{
   
   
   IMG_REG_WRITE((addr_Y ), REG_ADDR_GDMA_FMT_YTGB1  );  //REG_GDMA_FMT_YTGB1  = addr_Y ;
   IMG_REG_WRITE((addr_U ), REG_ADDR_GDMA_FMT_CBTGB1 );  //REG_GDMA_FMT_CBTGB1 = addr_U ;
   IMG_REG_WRITE((addr_V ), REG_ADDR_GDMA_FMT_CRTGB1 );  //REG_GDMA_FMT_CRTGB1 = addr_V ;
   
}


void gfmt_drv_set_dst_bank1(unsigned int addr_Y, unsigned int addr_U,unsigned int addr_V)
{
   
   
   IMG_REG_WRITE((addr_Y ), REG_ADDR_GDMA_FMT_YTGB2  );  //REG_GDMA_FMT_YTGB2  = addr_Y ;
   IMG_REG_WRITE((addr_U ), REG_ADDR_GDMA_FMT_CBTGB2 );  //REG_GDMA_FMT_CBTGB2 = addr_U ;
   IMG_REG_WRITE((addr_V ), REG_ADDR_GDMA_FMT_CRTGB2 );  //REG_GDMA_FMT_CRTGB2 = addr_V ;
   
}


void gfmt_drv_set_src_image_info(unsigned int width, unsigned int height)
{
    unsigned int u4Value;

   
    u4Value = (width << YSRCSZ_HI_SHIFT) | (height & YSRCSZ_LO_MASK);
    
    IMG_REG_WRITE((u4Value ), REG_ADDR_GDMA_FMT_YSRCSZ  );  
  
}

void gfmt_drv_set_fmt_only( unsigned int isFmtOnly)
{
    unsigned int ret ;
    unsigned int u4Value;

    //unsigned int startISP = 0;
    
    IMG_REG_WRITE((0 ), REG_ADDR_GDMA_FMT_INTEN  );  //REG_GDMA_FMT_INTEN = 0;
    ret = REG_GDMA_FMT_INTEN ;

    u4Value = REG_GDMA_FMT_INTEN ;
    u4Value |= ((1 << 8) | (isFmtOnly<<4));  //set write one clear
    IMG_REG_WRITE(( u4Value ), REG_ADDR_GDMA_FMT_INTEN  );
    
  
}


kal_uint32 gfmt_drv_get_result(void)
{
    if(_gfmt_only_int_status & CHK_GFMT_DONE_MASK)
    {
        return 0;
    }
 

    return 1;
}


int gfmt_config_fmt(GDMA_DRV_FMT_IN param)
{
   
  gfmt_drv_set_io_type(param.gfLinkGDMA, param.fieldCompactOuput, param.isTopField, param.io_fix_value /*0x0f*/);   

  gfmt_drv_set_src_image_info(param.gfSrcWidth, param.gfSrcHeight);

  gfmt_drv_set_webp_mode(param.webpEn) ;
  
  gfmt_drv_set_dram_burst_length(param.burst) ;

  gfmt_drv_set_input_video_row_height(param.gfDstBufHeight[0], param.gfDstBufHeight[1]) ;
   
  gfmt_drv_set_input_webp_row_height(param.webpMcuHeight_Y, param.webpMcuHeight_C);
   
  gfmt_drv_set_src_buffer( param.gfSrcBufAddr_Y, param.gfSrcBufAddr_C);

  gfmt_drv_set_src_mem_stride(param.gfSrcBufStride_Y, param.gfSrcBufStride_Y);
  
  gfmt_drv_set_dst_bank0( param.gfDstBufBank0[0], param.gfDstBufBank0[1], param.gfDstBufBank0[2]);
  
  gfmt_drv_set_dst_bank1( param.gfDstBufBank1[0], param.gfDstBufBank1[1], param.gfDstBufBank1[2]);

  gfmt_drv_set_dst_mem_stride(param.gfDstBufStride_Y, param.gfDstBufStride_C);
  
  gfmt_drv_set_pixel_range(param.rangeMapY, param.rangeMapC, param.rangeReduceEn, param.rangeMapEn);
  
  gfmt_drv_set_fmt_only(!param.gfLinkGDMA);
  return true;
   
}



//int gdma_config_fmt(GDMA_DRV_FMT_IN param)
//{
//    unsigned int regVal;
//
//    regVal = param.gfLinkGDMA << 17 |  param.fieldCompactOuput << 4 | param.isTopField << 2 | (0x0F00);
//    REG_GDMA_FMT_IOTYPE = regVal;
//
//    regVal = ((REG_GDMA_MCUMODE_MCUVFAC_MASK & param.gfDstBufHeight[0])<<8) | ((REG_GDMA_MCUMODE_MCUVFAC_MASK & param.gfDstBufHeight[1]) <<4 );
//    REG_GDMA_FMT_MCUMODE = regVal;
//
//    REG_GDMA_FMT_MIFMODE = param.burst;
//    
//    regVal = (REG_GDMA_SRCBUFL_MASK & param.gfSrcBufStride_Y) | ((REG_GDMA_SRCBUFL_MASK & param.gfSrcBufStride_Y) << 12);
//    REG_GDMA_FMT_SRCBUFL = regVal;
//
//    REG_GDMA_FMT_MIFMODE = (param.burst << REG_GDMA_BST_LIM_SHIFT);
//
//    regVal = (REG_GDMA_TGTBUFL_MASK & param.gfDstBufStride_Y) | (param.gfDstBufStride_C << REG_GDMA_TGTBUFL_C_SHIFT);
//    REG_GDMA_FMT_TGBUFL = regVal;
//
//    regVal = param.webpEn;
//    REG_GDMA_FMT_WINTFON = regVal;
//
//    regVal = (param.webpMcuHeight_Y << REG_GDMA_FMT_WEBPMODE_MCUY_SHIFT) | (param.webpMcuHeight_C << REG_GDMA_FMT_WEBPMODE_MCUC_SHIFT);
//    REG_GDMA_FMT_WEBPMODE = regVal;	
//	
//    regVal = (param.gfSrcBufStride_C << REG_GDMA_SRCBUFL_C_SHIFT) | (param.gfSrcBufStride_Y & REG_GDMA_SRCBUFL_MASK);
//    REG_GDMA_FMT_SRCBUFL = regVal;
//
//    REG_GDMA_FMT_YSRCB1 = param.gfSrcBufAddr_Y;
//    REG_GDMA_FMT_CBSRCB1 = param.gfSrcBufAddr_C;
//
//    REG_GDMA_FMT_YTGB1 = param.gfDstBufBank0[0];
//    REG_GDMA_FMT_CBTGB1 = param.gfDstBufBank0[1];
//    REG_GDMA_FMT_CRTGB1 = param.gfDstBufBank0[2];
//
//    REG_GDMA_FMT_YTGB2 = param.gfDstBufBank1[0];
//    REG_GDMA_FMT_CBTGB2 = param.gfDstBufBank1[1];
//    REG_GDMA_FMT_CRTGB2 = param.gfDstBufBank1[2];
//
//    regVal = (param.yWidth << YSRCSZ_HI_SHIFT) | (param.yHeight & YSRCSZ_LO_MASK);
//    REG_GDMA_FMT_YSRCSZ = regVal;
//
//    regVal = (param.rangeMapY << 16) | (param.rangeMapC << 8) | (param.rangeReduceEn << 4) | param.rangeMapEn;
//    REG_GDMA_FMT_RANGE = regVal;
//
//    REG_GDMA_FMT_INTEN |= (1 << 8);  //set write one clear
//
//    return 0;
//
//}


