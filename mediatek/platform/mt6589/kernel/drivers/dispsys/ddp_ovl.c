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
#include <mach/mt_typedefs.h>

#include "ddp_reg.h"
#include "ddp_ovl.h"
#include "ddp_matrix_para.h"

enum OVL_COLOR_SPACE {
    OVL_COLOR_SPACE_RGB = 0,
    OVL_COLOR_SPACE_YUV,
};

int OVLStart() {
    
    DISP_REG_SET(DISP_REG_OVL_INTEN, 0x0f);
    DISP_REG_SET(DISP_REG_OVL_EN, 0x01);

    return 0;
}

int OVLStop() {
    DISP_REG_SET(DISP_REG_OVL_INTEN, 0x00);
    DISP_REG_SET(DISP_REG_OVL_EN, 0x00);
    DISP_REG_SET(DISP_REG_OVL_INTSTA, 0x00);

    return 0;
}

int OVLReset() {
   
   unsigned int delay_cnt = 0;
   static unsigned int cnt=0;
   printk("[DDP] OVLReset called %d \n", cnt++);
   
   DISP_REG_SET(DISP_REG_OVL_RST, 0x1);              // soft reset
   while((DISP_REG_GET(DISP_REG_OVL_INTSTA)&0x1)!=0)
   {
        delay_cnt++;
        if(delay_cnt>10000)
        {
            printk("[DDP] error, OVLReset() timeout! \n");
            break;
        }
   }
   DISP_REG_SET(DISP_REG_OVL_RST, 0x0);
   
   DISP_REG_SET(DISP_REG_OVL_ROI_SIZE    , 0x00);           // clear regs
   DISP_REG_SET(DISP_REG_OVL_ROI_BGCLR   , 0xffffffff);
   DISP_REG_SET(DISP_REG_OVL_SRC_CON     , 0x00);
   
   DISP_REG_SET(DISP_REG_OVL_L0_CON      , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L0_SRCKEY   , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L0_SRC_SIZE , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L0_OFFSET   , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L0_ADDR     , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L0_PITCH    , 0x00);
   
   DISP_REG_SET(DISP_REG_OVL_L1_CON      , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L1_SRCKEY   , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L1_SRC_SIZE , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L1_OFFSET   , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L1_ADDR     , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L1_PITCH    , 0x00);
   
   DISP_REG_SET(DISP_REG_OVL_L2_CON      , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L2_SRCKEY   , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L2_SRC_SIZE , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L2_OFFSET   , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L2_ADDR     , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L2_PITCH    , 0x00);
   
   DISP_REG_SET(DISP_REG_OVL_L3_CON      , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L3_SRCKEY   , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L3_SRC_SIZE , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L3_OFFSET   , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L3_ADDR     , 0x00);
   DISP_REG_SET(DISP_REG_OVL_L3_PITCH    , 0x00);

    return 0;
}

int OVLROI(unsigned int bgW, 
           unsigned int bgH,
           unsigned int bgColor) 
{
    if((bgW > OVL_MAX_WIDTH) || (bgH > OVL_MAX_HEIGHT))
    {
        printk("error: OVLROI(), exceed OVL max size, w=%d, h=%d \n", bgW, bgH);		
        ASSERT(0);
    }

    DISP_REG_SET(DISP_REG_OVL_ROI_SIZE, bgH<<16 | bgW);

    DISP_REG_SET(DISP_REG_OVL_ROI_BGCLR, bgColor);
    
    return 0;
}

int OVLLayerSwitch(unsigned layer, bool en) {
    
    ASSERT(layer<=3);
    
    switch(layer) {
        case 0:
            DISP_REG_SET_FIELD(SRC_CON_FLD_L0_EN, DISP_REG_OVL_SRC_CON, en);
            break;
        case 1:
            DISP_REG_SET_FIELD(SRC_CON_FLD_L1_EN, DISP_REG_OVL_SRC_CON, en);
            break;
        case 2:
            DISP_REG_SET_FIELD(SRC_CON_FLD_L2_EN, DISP_REG_OVL_SRC_CON, en);
            break;
        case 3:
            DISP_REG_SET_FIELD(SRC_CON_FLD_L3_EN, DISP_REG_OVL_SRC_CON, en);
            break;
        default:
            printk("error: invalid layer=%d \n", layer);           // invalid layer
            ASSERT(0);
    }

    return 0;
}

int OVL3DConfig(unsigned int layer_id, 
                unsigned int en_3d,
                unsigned int landscape,
                unsigned int r_first)
{
    ASSERT(layer_id<=3);
    
    switch(layer_id) {
        case 0:
            DISP_REG_SET_FIELD(L0_CON_FLD_EN_3D,     DISP_REG_OVL_L0_CON, en_3d);
            DISP_REG_SET_FIELD(L0_CON_FLD_LANDSCAPE, DISP_REG_OVL_L0_CON, landscape);
            DISP_REG_SET_FIELD(L0_CON_FLD_R_FIRST,   DISP_REG_OVL_L0_CON, r_first);
            break;
        case 1:
            DISP_REG_SET_FIELD(L1_CON_FLD_EN_3D,     DISP_REG_OVL_L1_CON, en_3d);
            DISP_REG_SET_FIELD(L1_CON_FLD_LANDSCAPE, DISP_REG_OVL_L1_CON, landscape);
            DISP_REG_SET_FIELD(L1_CON_FLD_R_FIRST,   DISP_REG_OVL_L1_CON, r_first);
            break;
        case 2:
            DISP_REG_SET_FIELD(L2_CON_FLD_EN_3D,     DISP_REG_OVL_L2_CON, en_3d);
            DISP_REG_SET_FIELD(L2_CON_FLD_LANDSCAPE, DISP_REG_OVL_L2_CON, landscape);
            DISP_REG_SET_FIELD(L2_CON_FLD_R_FIRST,   DISP_REG_OVL_L2_CON, r_first);
            break;
        case 3:
            DISP_REG_SET_FIELD(L3_CON_FLD_EN_3D,     DISP_REG_OVL_L3_CON, en_3d);
            DISP_REG_SET_FIELD(L3_CON_FLD_LANDSCAPE, DISP_REG_OVL_L3_CON, landscape);
            DISP_REG_SET_FIELD(L3_CON_FLD_R_FIRST,   DISP_REG_OVL_L3_CON, r_first);
            break;
        default:
            printk("error: OVL3DConfig(), invalid layer=%d \n", layer_id);           // invalid layer
            ASSERT(0);
    }
	  
	  return 0;
}
int OVLLayerConfig(unsigned int layer,
                   unsigned int source, 
                   unsigned int fmt, 
                   unsigned int addr, 
                   unsigned int src_x,     // ROI x offset
                   unsigned int src_y,     // ROI y offset
                   unsigned int src_pitch,
                   unsigned int dst_x,     // ROI x offset
                   unsigned int dst_y,     // ROI y offset
                   unsigned int dst_w,     // ROT width
                   unsigned int dst_h,     // ROI height
                   bool keyEn,
                   unsigned int key,   // color key
                   bool aen,       // alpha enable
                   unsigned char alpha) {

    unsigned bpp;
    unsigned input_color_space;
    unsigned mode = 0xdeaddead;                     // yuv to rgb conversion required
    unsigned int rgb_swap = 0;

    ASSERT((dst_w <= OVL_MAX_WIDTH) && (dst_h <= OVL_MAX_HEIGHT));

    if(fmt==OVL_INPUT_FORMAT_ABGR8888  ||
       fmt==OVL_INPUT_FORMAT_PABGR8888 ||
       fmt==OVL_INPUT_FORMAT_xBGR8888 ||
       fmt==OVL_INPUT_FORMAT_BGR888    ||
       fmt==OVL_INPUT_FORMAT_BGR565 )
    {
        rgb_swap = 1;
        fmt -= OVL_COLOR_BASE;
    }
    else
    {
        rgb_swap = 0;
    }
    
    switch (fmt) {
        case OVL_INPUT_FORMAT_ARGB8888:
        case OVL_INPUT_FORMAT_PARGB8888:
        case OVL_INPUT_FORMAT_xRGB8888:
            bpp = 4;
            break;
        case OVL_INPUT_FORMAT_RGB888:
        case OVL_INPUT_FORMAT_YUV444:
            bpp = 3;
            break;
        case OVL_INPUT_FORMAT_RGB565:
        case OVL_INPUT_FORMAT_YUYV:
        case OVL_INPUT_FORMAT_UYVY:
        case OVL_INPUT_FORMAT_YVYU:
        case OVL_INPUT_FORMAT_VYUY:
            bpp = 2;
            break;
        default:
            ASSERT(0);      // invalid input format
    }

    if((source == OVL_LAYER_SOURCE_SCL || source == OVL_LAYER_SOURCE_PQ) &&
       (fmt != OVL_INPUT_FORMAT_YUV444)) {
        printk("error: direct link to OVL only support YUV444! \n" );
        ASSERT(0);                           // direct link support YUV444 only
    }

    if((source == OVL_LAYER_SOURCE_MEM && addr == 0))
    {
        printk("error: source from memory, but addr is 0! \n");
        ASSERT(0);                           // direct link support YUV444 only
    }

    switch (fmt) {
        case OVL_INPUT_FORMAT_ARGB8888:
        case OVL_INPUT_FORMAT_PARGB8888:
        case OVL_INPUT_FORMAT_xRGB8888:
        case OVL_INPUT_FORMAT_RGB888:
        case OVL_INPUT_FORMAT_RGB565:
            input_color_space = OVL_COLOR_SPACE_RGB;
            break;      
        case OVL_INPUT_FORMAT_YUV444:
        case OVL_INPUT_FORMAT_YUYV:
        case OVL_INPUT_FORMAT_UYVY:
        case OVL_INPUT_FORMAT_YVYU:
        case OVL_INPUT_FORMAT_VYUY:
            input_color_space = OVL_COLOR_SPACE_YUV;
            break;
        default:
            ASSERT(0);      // invalid input format
    }


    if (OVL_COLOR_SPACE_YUV == input_color_space) {
        mode = YUV2RGB_601_0_0;
    }
    
    switch(layer) {
        case 0:
            if(source == OVL_LAYER_SOURCE_MEM || source == OVL_LAYER_SOURCE_PQ)
            {
                DISP_REG_SET(DISP_REG_OVL_RDMA0_CTRL, 0x1);
            }
            DISP_REG_SET_FIELD(L0_CON_FLD_LAYER_SRC, DISP_REG_OVL_L0_CON, source);
            DISP_REG_SET_FIELD(L0_CON_FLD_CLRFMT, DISP_REG_OVL_L0_CON, fmt);
            DISP_REG_SET_FIELD(L0_CON_FLD_ALPHA_EN, DISP_REG_OVL_L0_CON, aen);
            DISP_REG_SET_FIELD(L0_CON_FLD_ALPHA, DISP_REG_OVL_L0_CON, alpha);
            DISP_REG_SET_FIELD(L0_CON_FLD_SRCKEY_EN, DISP_REG_OVL_L0_CON, keyEn);
            DISP_REG_SET_FIELD(L0_CON_FLD_RGB_SWAP, DISP_REG_OVL_L0_CON, rgb_swap);
			
            DISP_REG_SET(DISP_REG_OVL_L0_SRC_SIZE, dst_h<<16 | dst_w);

            DISP_REG_SET(DISP_REG_OVL_L0_OFFSET, dst_y<<16 | dst_x);

            DISP_REG_SET(DISP_REG_OVL_L0_ADDR, addr+src_x*bpp+src_y*src_pitch);

            DISP_REG_SET_FIELD(L0_PITCH_FLD_L0_SRC_PITCH, DISP_REG_OVL_L0_PITCH, src_pitch);

            DISP_REG_SET(DISP_REG_OVL_L0_SRCKEY, key);

            if (TABLE_NO > mode) {      // set up L0 YUV to RGB conversion matrix registers
                DISP_REG_SET_FIELD(L0_Y2R_PARA_R0_FLD_C_CF_RMY, DISP_REG_OVL_L0_Y2R_PARA_R0, coef[mode][0][0]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_R0_FLD_C_CF_RMU, DISP_REG_OVL_L0_Y2R_PARA_R0, coef[mode][0][1]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_R1_FLD_C_CF_RMV, DISP_REG_OVL_L0_Y2R_PARA_R1, coef[mode][0][2]);

                DISP_REG_SET_FIELD(L0_Y2R_PARA_G0_FLD_C_CF_GMY, DISP_REG_OVL_L0_Y2R_PARA_G0, coef[mode][1][0]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_G0_FLD_C_CF_GMU, DISP_REG_OVL_L0_Y2R_PARA_G0, coef[mode][1][1]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_G1_FLD_C_CF_GMV, DISP_REG_OVL_L0_Y2R_PARA_G1, coef[mode][1][2]);

                DISP_REG_SET_FIELD(L0_Y2R_PARA_B0_FLD_C_CF_BMY, DISP_REG_OVL_L0_Y2R_PARA_B0, coef[mode][2][0]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_B0_FLD_C_CF_BMU, DISP_REG_OVL_L0_Y2R_PARA_B0, coef[mode][2][1]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_B1_FLD_C_CF_BMV, DISP_REG_OVL_L0_Y2R_PARA_B1, coef[mode][2][2]);

                DISP_REG_SET_FIELD(L0_Y2R_PARA_YUV_A_0_FLD_C_CF_YA, DISP_REG_OVL_L0_Y2R_PARA_YUV_A_0, coef[mode][3][0]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_YUV_A_0_FLD_C_CF_UA, DISP_REG_OVL_L0_Y2R_PARA_YUV_A_0, coef[mode][3][1]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_YUV_A_1_FLD_C_CF_VA, DISP_REG_OVL_L0_Y2R_PARA_YUV_A_1, coef[mode][3][2]);

                DISP_REG_SET_FIELD(L0_Y2R_PARA_RGB_A_0_FLD_C_CF_RA, DISP_REG_OVL_L0_Y2R_PARA_RGB_A_0, coef[mode][4][0]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_RGB_A_0_FLD_C_CF_GA, DISP_REG_OVL_L0_Y2R_PARA_RGB_A_0, coef[mode][4][1]);
                DISP_REG_SET_FIELD(L0_Y2R_PARA_RGB_A_1_FLD_C_CF_BA, DISP_REG_OVL_L0_Y2R_PARA_RGB_A_1, coef[mode][4][2]);
            }
            break;
            
        case 1:
            if(source == OVL_LAYER_SOURCE_MEM || source == OVL_LAYER_SOURCE_PQ)
            {
                DISP_REG_SET(DISP_REG_OVL_RDMA1_CTRL, 0x1);
            }
            DISP_REG_SET_FIELD(L1_CON_FLD_LAYER_SRC, DISP_REG_OVL_L1_CON, source);
            DISP_REG_SET_FIELD(L1_CON_FLD_CLRFMT, DISP_REG_OVL_L1_CON, fmt);
            DISP_REG_SET_FIELD(L1_CON_FLD_ALPHA_EN, DISP_REG_OVL_L1_CON, aen);
            DISP_REG_SET_FIELD(L1_CON_FLD_ALPHA, DISP_REG_OVL_L1_CON, alpha);
            DISP_REG_SET_FIELD(L1_CON_FLD_SRCKEY_EN, DISP_REG_OVL_L1_CON, keyEn);
            DISP_REG_SET_FIELD(L1_CON_FLD_RGB_SWAP, DISP_REG_OVL_L1_CON, rgb_swap);

            DISP_REG_SET(DISP_REG_OVL_L1_SRC_SIZE, dst_h<<16 | dst_w);

            DISP_REG_SET(DISP_REG_OVL_L1_OFFSET, dst_y<<16 | dst_x);

            DISP_REG_SET(DISP_REG_OVL_L1_ADDR, addr+src_x*bpp+src_y*src_pitch);
    
            DISP_REG_SET_FIELD(L1_PITCH_FLD_L1_SRC_PITCH, DISP_REG_OVL_L1_PITCH, src_pitch);

            DISP_REG_SET(DISP_REG_OVL_L1_SRCKEY, key);

            if (TABLE_NO > mode) {      // set up L0 YUV to RGB conversion matrix registers
                DISP_REG_SET_FIELD(L1_Y2R_PARA_R0_FLD_C_CF_RMY, DISP_REG_OVL_L1_Y2R_PARA_R0, coef[mode][0][0]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_R0_FLD_C_CF_RMU, DISP_REG_OVL_L1_Y2R_PARA_R0, coef[mode][0][1]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_R1_FLD_C_CF_RMV, DISP_REG_OVL_L1_Y2R_PARA_R1, coef[mode][0][2]);

                DISP_REG_SET_FIELD(L1_Y2R_PARA_G0_FLD_C_CF_GMY, DISP_REG_OVL_L1_Y2R_PARA_G0, coef[mode][1][0]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_G0_FLD_C_CF_GMU, DISP_REG_OVL_L1_Y2R_PARA_G0, coef[mode][1][1]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_G1_FLD_C_CF_GMV, DISP_REG_OVL_L1_Y2R_PARA_G1, coef[mode][1][2]);

                DISP_REG_SET_FIELD(L1_Y2R_PARA_B0_FLD_C_CF_BMY, DISP_REG_OVL_L1_Y2R_PARA_B0, coef[mode][2][0]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_B0_FLD_C_CF_BMU, DISP_REG_OVL_L1_Y2R_PARA_B0, coef[mode][2][1]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_B1_FLD_C_CF_BMV, DISP_REG_OVL_L1_Y2R_PARA_B1, coef[mode][2][2]);

                DISP_REG_SET_FIELD(L1_Y2R_PARA_YUV_A_0_FLD_C_CF_YA, DISP_REG_OVL_L1_Y2R_PARA_YUV_A_0, coef[mode][3][0]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_YUV_A_0_FLD_C_CF_UA, DISP_REG_OVL_L1_Y2R_PARA_YUV_A_0, coef[mode][3][1]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_YUV_A_1_FLD_C_CF_VA, DISP_REG_OVL_L1_Y2R_PARA_YUV_A_1, coef[mode][3][2]);

                DISP_REG_SET_FIELD(L1_Y2R_PARA_RGB_A_0_FLD_C_CF_RA, DISP_REG_OVL_L1_Y2R_PARA_RGB_A_0, coef[mode][4][0]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_RGB_A_0_FLD_C_CF_GA, DISP_REG_OVL_L1_Y2R_PARA_RGB_A_0, coef[mode][4][1]);
                DISP_REG_SET_FIELD(L1_Y2R_PARA_RGB_A_1_FLD_C_CF_BA, DISP_REG_OVL_L1_Y2R_PARA_RGB_A_1, coef[mode][4][2]);
            }  
            break;
        case 2:
            if(source == OVL_LAYER_SOURCE_MEM || source == OVL_LAYER_SOURCE_PQ)
            {
                DISP_REG_SET(DISP_REG_OVL_RDMA2_CTRL, 0x1);
            }
            DISP_REG_SET_FIELD(L2_CON_FLD_LAYER_SRC, DISP_REG_OVL_L2_CON, source);
            DISP_REG_SET_FIELD(L2_CON_FLD_CLRFMT, DISP_REG_OVL_L2_CON, fmt);
            DISP_REG_SET_FIELD(L2_CON_FLD_ALPHA_EN, DISP_REG_OVL_L2_CON, aen);
            DISP_REG_SET_FIELD(L2_CON_FLD_ALPHA, DISP_REG_OVL_L2_CON, alpha);
            DISP_REG_SET_FIELD(L2_CON_FLD_SRCKEY_EN, DISP_REG_OVL_L2_CON, keyEn);
            DISP_REG_SET_FIELD(L2_CON_FLD_RGB_SWAP, DISP_REG_OVL_L2_CON, rgb_swap);

            DISP_REG_SET(DISP_REG_OVL_L2_SRC_SIZE, dst_h<<16 | dst_w);

            DISP_REG_SET(DISP_REG_OVL_L2_OFFSET, dst_y<<16 | dst_x);

            DISP_REG_SET(DISP_REG_OVL_L2_ADDR, addr+src_x*bpp+src_y*src_pitch);
    
            DISP_REG_SET_FIELD(L2_PITCH_FLD_L2_SRC_PITCH, DISP_REG_OVL_L2_PITCH, src_pitch);

            DISP_REG_SET(DISP_REG_OVL_L2_SRCKEY, key);

            if (TABLE_NO > mode) {      // set up L0 YUV to RGB conversion matrix registers
                DISP_REG_SET_FIELD(L2_Y2R_PARA_R0_FLD_C_CF_RMY, DISP_REG_OVL_L2_Y2R_PARA_R0, coef[mode][0][0]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_R0_FLD_C_CF_RMU, DISP_REG_OVL_L2_Y2R_PARA_R0, coef[mode][0][1]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_R1_FLD_C_CF_RMV, DISP_REG_OVL_L2_Y2R_PARA_R1, coef[mode][0][2]);

                DISP_REG_SET_FIELD(L2_Y2R_PARA_G0_FLD_C_CF_GMY, DISP_REG_OVL_L2_Y2R_PARA_G0, coef[mode][1][0]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_G0_FLD_C_CF_GMU, DISP_REG_OVL_L2_Y2R_PARA_G0, coef[mode][1][1]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_G1_FLD_C_CF_GMV, DISP_REG_OVL_L2_Y2R_PARA_G1, coef[mode][1][2]);

                DISP_REG_SET_FIELD(L2_Y2R_PARA_B0_FLD_C_CF_BMY, DISP_REG_OVL_L2_Y2R_PARA_B0, coef[mode][2][0]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_B0_FLD_C_CF_BMU, DISP_REG_OVL_L2_Y2R_PARA_B0, coef[mode][2][1]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_B1_FLD_C_CF_BMV, DISP_REG_OVL_L2_Y2R_PARA_B1, coef[mode][2][2]);

                DISP_REG_SET_FIELD(L2_Y2R_PARA_YUV_A_0_FLD_C_CF_YA, DISP_REG_OVL_L2_Y2R_PARA_YUV_A_0, coef[mode][3][0]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_YUV_A_0_FLD_C_CF_UA, DISP_REG_OVL_L2_Y2R_PARA_YUV_A_0, coef[mode][3][1]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_YUV_A_1_FLD_C_CF_VA, DISP_REG_OVL_L2_Y2R_PARA_YUV_A_1, coef[mode][3][2]);

                DISP_REG_SET_FIELD(L2_Y2R_PARA_RGB_A_0_FLD_C_CF_RA, DISP_REG_OVL_L2_Y2R_PARA_RGB_A_0, coef[mode][4][0]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_RGB_A_0_FLD_C_CF_GA, DISP_REG_OVL_L2_Y2R_PARA_RGB_A_0, coef[mode][4][1]);
                DISP_REG_SET_FIELD(L2_Y2R_PARA_RGB_A_1_FLD_C_CF_BA, DISP_REG_OVL_L2_Y2R_PARA_RGB_A_1, coef[mode][4][2]);
            }
            break;
            
        case 3:
            if(source == OVL_LAYER_SOURCE_MEM || source == OVL_LAYER_SOURCE_PQ)
            {
                DISP_REG_SET(DISP_REG_OVL_RDMA3_CTRL, 0x1);
            }
            DISP_REG_SET_FIELD(L3_CON_FLD_LAYER_SRC, DISP_REG_OVL_L3_CON, source);
            DISP_REG_SET_FIELD(L3_CON_FLD_CLRFMT, DISP_REG_OVL_L3_CON, fmt);
            DISP_REG_SET_FIELD(L3_CON_FLD_ALPHA_EN, DISP_REG_OVL_L3_CON, aen);
            DISP_REG_SET_FIELD(L3_CON_FLD_ALPHA, DISP_REG_OVL_L3_CON, alpha);
            DISP_REG_SET_FIELD(L3_CON_FLD_SRCKEY_EN, DISP_REG_OVL_L3_CON, keyEn);
            DISP_REG_SET_FIELD(L3_CON_FLD_RGB_SWAP, DISP_REG_OVL_L3_CON, rgb_swap);

            DISP_REG_SET(DISP_REG_OVL_L3_SRC_SIZE, dst_h<<16 | dst_w);

            DISP_REG_SET(DISP_REG_OVL_L3_OFFSET, dst_y<<16 | dst_x);

            DISP_REG_SET(DISP_REG_OVL_L3_ADDR, addr+src_x*bpp+src_y*src_pitch);
    
            DISP_REG_SET_FIELD(L3_PITCH_FLD_L3_SRC_PITCH, DISP_REG_OVL_L3_PITCH, src_pitch);

            DISP_REG_SET(DISP_REG_OVL_L3_SRCKEY, key);

            if (TABLE_NO > mode) {      // set up L0 YUV to RGB conversion matrix registers
                DISP_REG_SET_FIELD(L3_Y2R_PARA_R0_FLD_C_CF_RMY, DISP_REG_OVL_L3_Y2R_PARA_R0, coef[mode][0][0]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_R0_FLD_C_CF_RMU, DISP_REG_OVL_L3_Y2R_PARA_R0, coef[mode][0][1]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_R1_FLD_C_CF_RMV, DISP_REG_OVL_L3_Y2R_PARA_R1, coef[mode][0][2]);

                DISP_REG_SET_FIELD(L3_Y2R_PARA_G0_FLD_C_CF_GMY, DISP_REG_OVL_L3_Y2R_PARA_G0, coef[mode][1][0]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_G0_FLD_C_CF_GMU, DISP_REG_OVL_L3_Y2R_PARA_G0, coef[mode][1][1]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_G1_FLD_C_CF_GMV, DISP_REG_OVL_L3_Y2R_PARA_G1, coef[mode][1][2]);

                DISP_REG_SET_FIELD(L3_Y2R_PARA_B0_FLD_C_CF_BMY, DISP_REG_OVL_L3_Y2R_PARA_B0, coef[mode][2][0]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_B0_FLD_C_CF_BMU, DISP_REG_OVL_L3_Y2R_PARA_B0, coef[mode][2][1]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_B1_FLD_C_CF_BMV, DISP_REG_OVL_L3_Y2R_PARA_B1, coef[mode][2][2]);

                DISP_REG_SET_FIELD(L3_Y2R_PARA_YUV_A_0_FLD_C_CF_YA, DISP_REG_OVL_L3_Y2R_PARA_YUV_A_0, coef[mode][3][0]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_YUV_A_0_FLD_C_CF_UA, DISP_REG_OVL_L3_Y2R_PARA_YUV_A_0, coef[mode][3][1]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_YUV_A_1_FLD_C_CF_VA, DISP_REG_OVL_L3_Y2R_PARA_YUV_A_1, coef[mode][3][2]);

                DISP_REG_SET_FIELD(L3_Y2R_PARA_RGB_A_0_FLD_C_CF_RA, DISP_REG_OVL_L3_Y2R_PARA_RGB_A_0, coef[mode][4][0]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_RGB_A_0_FLD_C_CF_GA, DISP_REG_OVL_L3_Y2R_PARA_RGB_A_0, coef[mode][4][1]);
                DISP_REG_SET_FIELD(L3_Y2R_PARA_RGB_A_1_FLD_C_CF_BA, DISP_REG_OVL_L3_Y2R_PARA_RGB_A_1, coef[mode][4][2]);
            }
            break;
            
        default:
            ASSERT(0);       // invalid layer index
    }

    if(0)//if(w==1080)
    {
        // printk("[DDP]set 1080p ultra \n");
        DISP_REG_SET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING, 0x00f00040);
        DISP_REG_SET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING, 0x00f00040);
        DISP_REG_SET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING, 0x00f00040);
        DISP_REG_SET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING, 0x00f00040);
    }
    
    return 0;
}

void OVLLayerPQControl(unsigned layer, bool pq_en)
{
    if(layer >= 4)
        return;

    if(pq_en)
    {
        DISP_REG_SET(DISP_REG_OVL_DATAPATH_CON, ((1<<(layer + 4))|layer)<<16);
    }
    else
    {
        DISP_REG_SET(DISP_REG_OVL_DATAPATH_CON, 0);
    }
}

void OVLLayerTdshpEn(unsigned layer, bool en)
{
    unsigned int reg = 0;

    if(layer >= 4)
        return;
    
    if(en)
    {
        // OVL inside PQ_OUT_SEL, RDMAx_OUT_SEL
        DISP_REG_SET(DISP_REG_OVL_DATAPATH_CON, ((1<<(layer + 20)) + (layer<<16)));

        // layer source from PQ
        DISP_REG_SET_FIELD(L0_CON_FLD_LAYER_SRC, DISP_REG_OVL_L0_CON+layer*0x20, OVL_LAYER_SOURCE_PQ);

        //
        DISP_REG_SET(DISP_REG_CONFIG_OVL_PQ_OUT_SEL, 1);  // to tdshp
        DISP_REG_SET(DISP_REG_CONFIG_OVL_PQ_IN_SEL, 1);   // from tdshp

        DISP_REG_SET(DISP_REG_CONFIG_TDSHP_SEL, 0);   // from overlay before blending
        DISP_REG_SET(DISP_REG_CONFIG_TDSHP_MOUT_EN, 1);   // to ovl pq input

        // printk("DDP  OVLLayerTdshpEn, reg=0x%x \n", DISP_REG_GET(DISP_REG_OVL_DATAPATH_CON));
    }
    else
    {
        reg = DISP_REG_GET(DISP_REG_OVL_DATAPATH_CON);
        reg &= ~(1<<(layer+20));
        DISP_REG_SET(DISP_REG_OVL_DATAPATH_CON, reg);

        // layer source from PQ
        DISP_REG_SET_FIELD(L0_CON_FLD_LAYER_SRC, DISP_REG_OVL_L0_CON+layer*0x20, OVL_LAYER_SOURCE_MEM);
    }
}

