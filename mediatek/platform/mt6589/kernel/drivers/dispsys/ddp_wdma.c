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
#include <linux/aee.h>

#include <linux/xlog.h>

#include <asm/io.h>
#include <asm/system.h>

#include <mach/mt_typedefs.h>

#include "ddp_matrix_para.h"
#include "ddp_reg.h"
#include "ddp_wdma.h"

#define DISP_INDEX_OFFSET 0x1000

enum WDMA_COLOR_SPACE {
    WDMA_COLOR_SPACE_RGB = 0,
    WDMA_COLOR_SPACE_YUV,
};


int WDMAStart(unsigned idx) {
    
    
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_INTEN, 0x03);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_EN, 0x01);

    return 0;
}

int WDMAStop(unsigned idx) {
        

    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_INTEN, 0x00);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_EN, 0x00);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_INTSTA, 0x00);
    
    return 0;
}

int WDMAReset(unsigned idx) {
    
    
    unsigned int delay_cnt = 0;
    
    //WDMA_RST = 0x00;
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_RST , 0x01);            // soft reset
    while((DISP_REG_GET(DISP_INDEX_OFFSET*idx+DISP_REG_WDMA_RST)&0x1)!=0)
    {
         delay_cnt++;
         if(delay_cnt>10000)
         {
             printk("[DDP] error, WDMAReset(%d) timeout! \n", idx);
             break;
         }
    }
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_RST , 0x00);
    
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CFG , 0x00);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_SRC_SIZE , 0x00);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CLIP_SIZE , 0x00);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CLIP_COORD , 0x00);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_DST_ADDR , 0x00);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_DST_W_IN_BYTE , 0x00);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_ALPHA , 0x00);          // clear regs

    return 0;
}

int WDMAConfigUV(unsigned idx, unsigned int uAddr, unsigned int vAddr, unsigned int dstWidth)
{
    unsigned int bpp=1;

    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_DST_U_ADDR, uAddr);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_DST_V_ADDR, vAddr);
    DISP_REG_SET_FIELD(WDMA_BUF_ADDR_FLD_UV_Pitch, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_DST_UV_PITCH, dstWidth * bpp/2);

    return 0;
}

int WDMAConfig(unsigned idx,
               unsigned inputFormat, unsigned srcWidth, unsigned srcHeight,
               unsigned clipX, unsigned clipY, unsigned clipWidth, unsigned clipHeight,
               unsigned outputFormat, unsigned dstAddress, unsigned dstWidth,               
               bool useSpecifiedAlpha, unsigned char alpha) {

    unsigned int output_format=0;
    unsigned int byte_swap=0;
    unsigned int rgb_swap=0;
    unsigned int uv_swap=0;

    unsigned input_color_space;                         // check input format color space
    unsigned output_color_space;                        // check output format color space
    unsigned mode = 0xdeaddead;    
    unsigned bpp;
    
    ASSERT((WDMA_INPUT_FORMAT_ARGB == inputFormat) ||
           (WDMA_INPUT_FORMAT_YUV444 == inputFormat));    

    // should use OVL alpha instead of sw config
    useSpecifiedAlpha = 0;
    
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_SRC_SIZE, srcHeight<<16 | srcWidth);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CLIP_COORD, clipY<<16 | clipX);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CLIP_SIZE, clipHeight<<16 | clipWidth);

    DISP_REG_SET_FIELD(WDMA_CFG_FLD_In_Format, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CFG, inputFormat);


    switch(outputFormat) {
        case WDMA_OUTPUT_FORMAT_RGB565:
        case WDMA_OUTPUT_FORMAT_RGB888:
        case WDMA_OUTPUT_FORMAT_ARGB:
        case WDMA_OUTPUT_FORMAT_XRGB:
        case WDMA_OUTPUT_FORMAT_UYVY:
        case WDMA_OUTPUT_FORMAT_YUV444:
        case WDMA_OUTPUT_FORMAT_UYVY_BLK:
        case WDMA_OUTPUT_FORMAT_YUV420_P:
            output_format = outputFormat;
            byte_swap =  0;
            rgb_swap =  0;
            uv_swap =  0;
            break;
        case WDMA_OUTPUT_FORMAT_BGR888:
            output_format =  WDMA_OUTPUT_FORMAT_RGB888;
            byte_swap =  0;
            rgb_swap =  1;
            uv_swap =  0;            
            break;
        case WDMA_OUTPUT_FORMAT_BGRA:
            output_format =  WDMA_OUTPUT_FORMAT_ARGB;
            byte_swap =  1;
            rgb_swap =  0;
            uv_swap =  0;
            break;
        case WDMA_OUTPUT_FORMAT_ABGR:
            output_format =  WDMA_OUTPUT_FORMAT_ARGB;
            byte_swap =  0;
            rgb_swap =  1;
            uv_swap =  0;
            break;
        case WDMA_OUTPUT_FORMAT_RGBA:
            output_format =  WDMA_OUTPUT_FORMAT_ARGB;
            byte_swap =  1;
            rgb_swap =  1;
            uv_swap =  0;
            break;
        default:
            ASSERT(0);       // invalid format
    }
    DISP_REG_SET_FIELD(WDMA_CFG_FLD_Out_Format, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CFG, output_format);
    DISP_REG_SET_FIELD(WDMA_CFG_FLD_BYTE_SWAP,  idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CFG, byte_swap);
    DISP_REG_SET_FIELD(WDMA_CFG_FLD_RGB_SWAP,   idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CFG, rgb_swap);
    DISP_REG_SET_FIELD(WDMA_CFG_FLD_UV_SWAP,    idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CFG, uv_swap);


    // set DNSP for UYVY and YUV_3P format for better quality
    if(outputFormat==WDMA_OUTPUT_FORMAT_UYVY ||
       outputFormat==WDMA_OUTPUT_FORMAT_YUV420_P)
    {
       DISP_REG_SET_FIELD(WDMA_CFG_FLD_DNSP_SEL, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CFG, 1);
    }
    else
    {
        DISP_REG_SET_FIELD(WDMA_CFG_FLD_DNSP_SEL, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_CFG, 0);
    }
    
    switch (inputFormat) {
        case WDMA_INPUT_FORMAT_ARGB:
            input_color_space = WDMA_COLOR_SPACE_RGB;
            break;
        case WDMA_INPUT_FORMAT_YUV444:
            input_color_space = WDMA_COLOR_SPACE_YUV;
            break;
        default:
            ASSERT(0);
    }

    switch (outputFormat) {
        case WDMA_OUTPUT_FORMAT_RGB565:
        case WDMA_OUTPUT_FORMAT_RGB888:
        case WDMA_OUTPUT_FORMAT_ARGB:
        case WDMA_OUTPUT_FORMAT_XRGB:
        case WDMA_OUTPUT_FORMAT_BGR888:
        case WDMA_OUTPUT_FORMAT_BGRA:
        case WDMA_OUTPUT_FORMAT_ABGR:
        case WDMA_OUTPUT_FORMAT_RGBA:
            output_color_space = WDMA_COLOR_SPACE_RGB;
            break;
        case WDMA_OUTPUT_FORMAT_UYVY:
        case WDMA_OUTPUT_FORMAT_YUV444:
        case WDMA_OUTPUT_FORMAT_UYVY_BLK:
        case WDMA_OUTPUT_FORMAT_YUV420_P:
            output_color_space = WDMA_COLOR_SPACE_YUV;
            break;
        default:
            ASSERT(0);
    }

    if(WDMA_COLOR_SPACE_RGB == input_color_space &&
       WDMA_COLOR_SPACE_YUV == output_color_space) {        // RGB to YUV required       
        mode = RGB2YUV_601;
    }
    else if(WDMA_COLOR_SPACE_YUV == input_color_space &&    // YUV to RGB required
            WDMA_COLOR_SPACE_RGB == output_color_space) {        
        mode = YUV2RGB_601_16_16;
    }

    if(TABLE_NO > mode) {                                           // set matrix as mode
        DISP_REG_SET_FIELD(WDMA_C00_FLD_C00, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_C00, coef[mode][0][0]);
        DISP_REG_SET_FIELD(WDMA_C00_FLD_C01, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_C00, coef[mode][0][1]);
        DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_C02, coef[mode][0][2]);

        DISP_REG_SET_FIELD(WDMA_C10_FLD_C10, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_C10, coef[mode][1][0]);
        DISP_REG_SET_FIELD(WDMA_C10_FLD_C11, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_C10, coef[mode][1][1]);
        DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_C12 , coef[mode][1][2]);

        DISP_REG_SET_FIELD(WDMA_C20_FLD_C20, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_C20, coef[mode][2][0]);
        DISP_REG_SET_FIELD(WDMA_C20_FLD_C21, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_C20, coef[mode][2][1]);
        DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_C22 , coef[mode][2][2]);

        DISP_REG_SET_FIELD(WDMA_PRE_ADD0_FLD_PRE_ADD_0, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_PRE_ADD0, coef[mode][3][0]);
        DISP_REG_SET_FIELD(WDMA_PRE_ADD0_FLD_SIGNED_0, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_PRE_ADD0, 0);
        DISP_REG_SET_FIELD(WDMA_PRE_ADD0_FLD_PRE_ADD_1, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_PRE_ADD0, coef[mode][3][1]);
        DISP_REG_SET_FIELD(WDMA_PRE_ADD0_FLD_SIGNED_1, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_PRE_ADD0, 0);

        DISP_REG_SET_FIELD(WDMA_PRE_ADD2_FLD_PRE_ADD_2, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_PRE_ADD2, coef[mode][3][2]);
        DISP_REG_SET_FIELD(WDMA_PRE_ADD2_FLD_SIGNED_2, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_PRE_ADD2, 0);

        DISP_REG_SET_FIELD(WDMA_POST_ADD0_FLD_POST_ADD_0, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_POST_ADD0, coef[mode][4][0]);
        DISP_REG_SET_FIELD(WDMA_POST_ADD0_FLD_POST_ADD_1, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_POST_ADD0, coef[mode][4][1]);
        DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_POST_ADD2 , coef[mode][4][2]);
    }


    switch(outputFormat) {
        case WDMA_OUTPUT_FORMAT_RGB565:
        case WDMA_OUTPUT_FORMAT_UYVY_BLK:
        case WDMA_OUTPUT_FORMAT_UYVY:
            bpp = 2;
            break;
        case WDMA_OUTPUT_FORMAT_YUV420_P:
            bpp = 1;
            break;            
        case WDMA_OUTPUT_FORMAT_RGB888:
        case WDMA_OUTPUT_FORMAT_BGR888:
        case WDMA_OUTPUT_FORMAT_YUV444:
            bpp = 3;
            break;
        case WDMA_OUTPUT_FORMAT_ARGB:
        case WDMA_OUTPUT_FORMAT_XRGB:
        case WDMA_OUTPUT_FORMAT_BGRA:
        case WDMA_OUTPUT_FORMAT_ABGR:            
        case WDMA_OUTPUT_FORMAT_RGBA:
            bpp = 4;
            break;
        default:
            ASSERT(0);  // invalid format
    }
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_DST_ADDR, dstAddress);
    DISP_REG_SET(idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_DST_W_IN_BYTE, dstWidth * bpp);
    DISP_REG_SET_FIELD(WDMA_ALPHA_FLD_A_Sel,   idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_ALPHA, useSpecifiedAlpha);
    DISP_REG_SET_FIELD(WDMA_ALPHA_FLD_A_Value, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_ALPHA, alpha);
    
    return 0;
}

void WDMAWait(unsigned idx)
{
    // polling interrupt status
    unsigned int delay_cnt = 0;
    while((DISP_REG_GET(0x1000*idx+DISP_REG_WDMA_INTSTA) & 0x1) != 0x1)
    {
        delay_cnt++;
        msleep(1);
        if(delay_cnt>100)
        {
            printk("[DDP] error:WDMA%dWait timeout \n", idx);
            break;
        }
    }
    DISP_REG_SET(0x1000*idx+DISP_REG_WDMA_INTSTA , 0x0);

}

void WDMASlowMode(unsigned int idx, 
                          unsigned int enable, 
                          unsigned int level, 
                          unsigned int cnt,
                          unsigned int threadhold)
{
    
    DISP_REG_SET_FIELD(WDMA_SMI_CON_FLD_Slow_Enable, idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_SMI_CON, enable&0x01);
    DISP_REG_SET_FIELD(WDMA_SMI_CON_FLD_Slow_Count,  idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_SMI_CON, cnt&0xff);
    DISP_REG_SET_FIELD(WDMA_SMI_CON_FLD_Slow_Level,  idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_SMI_CON, level&0x7);
    DISP_REG_SET_FIELD(WDMA_SMI_CON_FLD_Threshold,   idx*DISP_INDEX_OFFSET+DISP_REG_WDMA_SMI_CON, threadhold&0xf);

}
