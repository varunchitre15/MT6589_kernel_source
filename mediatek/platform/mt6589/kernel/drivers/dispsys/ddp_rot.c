#include <linux/kernel.h>
#include "ddp_reg.h"
#include "ddp_rot.h"
#include "ddp_drv.h"
#include "ddp_debug.h"

#define ENABLE_DDP_ROT_LOG

#ifdef ENABLE_DDP_ROT_LOG
#define DDP_ROT_LOG(fmt, arg...) {printk("[ROT]"); printk(fmt, ##arg);}
#else
#define DDP_ROT_LOG()
#endif

// global variable
BOOL g_lb_4b_mode = 0;
BOOL g_buffer_mode = 0;

ROT_SRC_FORMAT g_src_format;
unsigned g_src_width;
unsigned g_src_height;
unsigned g_dst_clip_width;
unsigned g_dst_clip_height;
unsigned g_src_mf_pitch;  //XX: main frame
unsigned g_src_sf_pitch;  //XX: sub frame
unsigned g_src_addr[3];
ROT_DEGREE g_rotation;
unsigned g_flip;
unsigned g_vdo_mode;
unsigned g_vdo_field;

int mb_depth;
int mf_jump;
int mf_sb;
int mb_lp;
int mb_pps;
int mb_w_p;
int mb_bpp;
int mb_pps_ori;

// for sub frame
int sb_depth;
int sf_jump;
int sf_sb;
int sb_lp;
int sb_pps;
int sb_w_p;
int sb_bpp;
int sb_pps_ori;

// start module
int ROTStart(void)
{
    disp_rot_reg_enable(TRUE);

    return 0;
}

// stop module
int ROTStop(void)
{
    disp_rot_reg_enable(FALSE);
    DISP_REG_SET(DISP_REG_ROT_INTERRUPT_ENABLE, 0x0);
    DISP_REG_SET(DISP_REG_ROT_INTERRUPT_STATUS, 0x0);
    return 0;
}

// reset module
int ROTReset(void)
{
    disp_rot_reg_init();
    disp_rot_reg_reset();

    return 0;
}

// configu module
int ROTConfig(int rotateDegree,
              DISP_INTERLACE_FORMAT interlace,
              int flip,
              DISP_COLOR_FORMAT inFormat,
              unsigned int memAddr[3],
              int srcWidth,
              int srcHeight,
              int srcPitch,
              struct DISP_REGION srcROI,
              DISP_COLOR_FORMAT *outFormat)
{
    ROT_DEGREE rot = ROT_DEGREE_0;
    ROT_VDO_MODE src_vdo_mode;
    ROT_VDO_FIELD src_vdo_field = 0;

    switch (rotateDegree)
    {
        case 0:
            rot = ROT_DEGREE_0;
            break;

        case 90:
            rot = ROT_DEGREE_90;
            break;

        case 180:
            rot = ROT_DEGREE_180;
            break;

        case 270:
            rot = ROT_DEGREE_270;
            break;

        default:
            DDP_ROT_LOG("error: unsupport rotate degree %d", rotateDegree);
            break;
    }

    DDP_ROT_LOG("rotate : %d\n", rot);

    switch (interlace)
    {
        case DISP_INTERLACE_FORMAT_NONE:
            src_vdo_mode = ROT_VDO_MODE_FRAME;
            src_vdo_field = ROT_VDO_FIELD_TOP;
            break;

        case DISP_INTERLACE_FORMAT_TOP_FIELD:
            src_vdo_mode = ROT_VDO_MODE_FIELD;
            src_vdo_field = ROT_VDO_FIELD_TOP;
            break;

        case DISP_INTERLACE_FORMAT_BOTTOM_FIELD:
            src_vdo_mode = ROT_VDO_MODE_FIELD;
            src_vdo_field = ROT_VDO_FIELD_BOTTOM;
            break;

        default:
            DDP_ROT_LOG("error: unsupport interlace format %d", interlace);
            break;
    }
    disp_rot_reg_con(rot, flip);  // rotate = 90
    disp_rot_reg_src(disp_rot_format(inFormat),
                     memAddr,       // src addr[3]
                     srcWidth,          // src widht
                     srcHeight,          // src height
                     srcPitch,
                     &srcROI,
                     src_vdo_mode,
                     src_vdo_field);         // ROI(x,y,width, height)

    *outFormat = DISP_COLOR_FORMAT_YUV_444_1P;

    if(interlace != DISP_INTERLACE_FORMAT_NONE && inFormat == DISP_COLOR_FORMAT_YUV_420_2P_VDO_BLK)
    {
        DDP_ROT_LOG("choose filed compact mode");
        //config.dstHeight /= 2;
        //config.dstROI.h /= 2;
    }

    return 0;
}

ROT_SRC_FORMAT disp_rot_format(DISP_COLOR_FORMAT format)
{
    ROT_SRC_FORMAT rot_format = ROT_SRC_FORMAT_SRC_FORMAT_MAX;
    DISP_REG_SET_FIELD(SRC_CON_FLD_SRC_SWAP, DISP_REG_ROT_SRC_CON, 0);

    switch(format)
    {
        case DISP_COLOR_FORMAT_YUV_420_3P       :
            rot_format = ROT_SRC_FORMAT_YCBCR_420_3P_SW_SCAN;
            break;

        case DISP_COLOR_FORMAT_YUV_420_3P_YVU       :
            rot_format = ROT_SRC_FORMAT_YCBCR_420_3P_SW_SCAN;
            DISP_REG_SET_FIELD(SRC_CON_FLD_SRC_SWAP, DISP_REG_ROT_SRC_CON, 2);
            break;

        case DISP_COLOR_FORMAT_YUV_420_2P_YUYV      :
            rot_format = ROT_SRC_FORMAT_YCBCR_420_2P_SW_SCAN;
            break;

        case DISP_COLOR_FORMAT_YUV_420_2P_YVYU      :
            rot_format = ROT_SRC_FORMAT_YCBCR_420_2P_SW_SCAN;
            DISP_REG_SET_FIELD(SRC_CON_FLD_SRC_SWAP, DISP_REG_ROT_SRC_CON, 2);
            break;

        case DISP_COLOR_FORMAT_YUV_420_2P_ISP_BLK  :
            rot_format = ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK;
            break;

        case DISP_COLOR_FORMAT_YUV_420_2P_VDO_BLK  :
            rot_format = ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK;
            break;

        case DISP_COLOR_FORMAT_YUV_422_3P       :
            rot_format = ROT_SRC_FORMAT_YCBCR_422_3P_SW_SCAN;
            break;

        case DISP_COLOR_FORMAT_YUV_422_2P      :
            rot_format = ROT_SRC_FORMAT_YCBCR_422_2P_SW_SCAN;
            break;

        case DISP_COLOR_FORMAT_YUV_422_I       :
            rot_format = ROT_SRC_FORMAT_YCBCR_422_I_SW_SCAN;
            break;

        case DISP_COLOR_FORMAT_YUV_422_I_BLK   :
            rot_format = ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK;
            break;

        case DISP_COLOR_FORMAT_YUV_444_3P       :
            rot_format = ROT_SRC_FORMAT_YCBCR_444_3P_SW_SCAN;
            break;

        case DISP_COLOR_FORMAT_YUV_444_2P      :
            rot_format = ROT_SRC_FORMAT_YCBCR_444_2P_SW_SCAN;
            break;

        case DISP_COLOR_FORMAT_RGB565         :
            rot_format = ROT_SRC_FORMAT_RGB565;
            break;

        case DISP_COLOR_FORMAT_RGB888         :
            rot_format = ROT_SRC_FORMAT_RGB888;
            break;

        case DISP_COLOR_FORMAT_ARGB8888        :
            rot_format = ROT_SRC_FORMAT_XRGB8888;
            break;

        case DISP_COLOR_FORMAT_RGBA8888        :
            rot_format = ROT_SRC_FORMAT_RGBX8888;
            break;

        default:
            DDP_ROT_LOG("unknown format=%d\n", format);
            break;
    }

    return rot_format;
}

/*
rotator register initialization
*/
void disp_rot_reg_init(void)
{
    // GMC control (use default value)
    DISP_REG_SET(DISP_REG_ROT_GMCIF_CON, 0x00000771);
}

/*
rotator control register
*/
void disp_rot_reg_con(ROT_DEGREE rot_degree, int rot_flip)
{
    if (rot_degree > ROT_DEGREE_MAX)
    {
        DDP_ROT_LOG("Error rotation degree\n");
        ASSERT(0);
    }

    if (rot_flip > ROT_FLIP_MAX)
    {
        DDP_ROT_LOG("Error flip parameter\n");
        ASSERT(0);
    }

    // default value
    g_lb_4b_mode = 1;
    g_buffer_mode = 1;
    g_flip = rot_flip;
    g_rotation = rot_degree;

    DISP_REG_SET_FIELD(CON_FLD_LB_4B_MODE     , DISP_REG_ROT_CON, g_lb_4b_mode);
    DISP_REG_SET_FIELD(CON_FLD_BUFFER_MODE    , DISP_REG_ROT_CON, g_buffer_mode);
    DISP_REG_SET_FIELD(CON_FLD_FLIP           , DISP_REG_ROT_CON, rot_flip);
    DISP_REG_SET_FIELD(CON_FLD_ROTATION_DEGREE, DISP_REG_ROT_CON, rot_degree);
}

/*
set main frame background size in byte register
*/
unsigned disp_rot_get_bkgd(unsigned src_format,
                           unsigned src_pitch,
                           unsigned idx) //main frame or sub frame
{
    unsigned mf_bkgd_wb = src_pitch, sf_bkgd_wb = src_pitch/2;

    switch(src_format)
    {
    case ROT_SRC_FORMAT_YCBCR_420_3P_SW_SCAN:            // 0
        mf_bkgd_wb = src_pitch;
        sf_bkgd_wb = mf_bkgd_wb / 2;
        break;

    case ROT_SRC_FORMAT_YCBCR_420_2P_SW_SCAN:            // 1
        mf_bkgd_wb = src_pitch;
        sf_bkgd_wb = mf_bkgd_wb;
        break;

    case ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK:        // 2
        mf_bkgd_wb = src_pitch * 32;
        sf_bkgd_wb = mf_bkgd_wb / 2;
        break;

    case ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK:        // 3
        mf_bkgd_wb = src_pitch * 32;
        sf_bkgd_wb = mf_bkgd_wb / 2;
        break;

    case ROT_SRC_FORMAT_YCBCR_422_3P_SW_SCAN:            // 4
        mf_bkgd_wb = src_pitch;
        sf_bkgd_wb = mf_bkgd_wb / 2;
        break;

    case ROT_SRC_FORMAT_YCBCR_422_2P_SW_SCAN:            // 5
        mf_bkgd_wb = src_pitch;
        sf_bkgd_wb = mf_bkgd_wb;
        break;

    case ROT_SRC_FORMAT_YCBCR_422_I_SW_SCAN:            // 6
        mf_bkgd_wb = src_pitch * 2;
        sf_bkgd_wb = 0;
        break;

    case ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK:        // 7
        mf_bkgd_wb = src_pitch * 64;
        sf_bkgd_wb = 0;
        break;

    case ROT_SRC_FORMAT_YCBCR_444_3P_SW_SCAN:            // 8
        mf_bkgd_wb = src_pitch;
        sf_bkgd_wb = mf_bkgd_wb;
        break;

    case ROT_SRC_FORMAT_YCBCR_444_2P_SW_SCAN:            // 9
        mf_bkgd_wb = src_pitch;
        sf_bkgd_wb = mf_bkgd_wb;
        break;

    case ROT_SRC_FORMAT_RGB565:                        // 12
        mf_bkgd_wb = src_pitch * 2;
        sf_bkgd_wb = 0;
        break;

    case ROT_SRC_FORMAT_RGB888:                        // 13
        mf_bkgd_wb = src_pitch * 3;
        sf_bkgd_wb = 0;
        break;

    case ROT_SRC_FORMAT_XRGB8888:                        // 14
    case ROT_SRC_FORMAT_RGBX8888:                        // 15
        mf_bkgd_wb = src_pitch * 4;
        sf_bkgd_wb = 0;
        break;
    }

    if (idx == 1)
        return sf_bkgd_wb;

    return mf_bkgd_wb;
}

/*
calculate the source addr
*/
int disp_rot_src_offset(int pln, int boff_w, int boff_h)
{
    int src_format, flip, rotation, vdo_mode, vdo_field;
    int mf_pad_h, sf_pad_h;
    int mf_src_w, mf_src_h, mf_bpp, mf_bkgd_wb, mf_bkgd_w;
    int sf_src_w, sf_src_h, sf_bpp, sf_bkgd_wb, sf_bkgd_w;
    int tmp_0, tmp_1;
    int offset;
    //int mf_vdo_frm_0, mf_vdo_frm_1, sf_vdo_frm_0, sf_vdo_frm_1;
    //int mf_vdo_fld_0, mf_vdo_fld_1, sf_vdo_fld_0, sf_vdo_fld_1;
    int mf_base_offset, sf_base_offset;
    int mf_offset, sf_offset;


    flip = g_flip;
    rotation = g_rotation;
    src_format = g_src_format;
    vdo_mode = g_vdo_mode;
    vdo_field = g_vdo_field;

    switch (src_format)
    {
    case ROT_SRC_FORMAT_YCBCR_420_3P_SW_SCAN      : //0
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 1;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = g_src_width/2;
        sf_src_h   = g_src_height/2;
        sf_bpp     = 1;
        sf_bkgd_wb = g_src_mf_pitch/2;
        sf_bkgd_w  = g_src_mf_pitch/2/sf_bpp;
        sf_pad_h   = 1;
        break;

    case ROT_SRC_FORMAT_YCBCR_420_2P_SW_SCAN      : //1
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 1;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = g_src_width/2;
        sf_src_h   = g_src_height/2;
        sf_bpp     = 2;
        sf_bkgd_wb = g_src_mf_pitch;
        sf_bkgd_w  = g_src_mf_pitch/sf_bpp;
        sf_pad_h   = 1;
        break;

    case ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK : //2
    case ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK    : //3
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 1;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/32/mf_bpp;
        mf_pad_h   = 32;
        sf_src_w   = g_src_width/2;
        sf_src_h   = g_src_height/2;
        sf_bpp     = 2;
        sf_bkgd_wb = g_src_mf_pitch/2;
        sf_bkgd_w  = g_src_mf_pitch/2/16/sf_bpp;
        sf_pad_h   = 16;
        break;

    case ROT_SRC_FORMAT_YCBCR_422_3P_SW_SCAN      : //4
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 1;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = g_src_width/2;
        sf_src_h   = g_src_height;
        sf_bpp     = 1;
        sf_bkgd_wb = g_src_mf_pitch/2;
        sf_bkgd_w  = g_src_mf_pitch/2/sf_bpp;
        sf_pad_h   = 1;
        break;

    case ROT_SRC_FORMAT_YCBCR_422_2P_SW_SCAN      : //5
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 1;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = g_src_width/2;
        sf_src_h   = g_src_height;
        sf_bpp     = 2;
        sf_bkgd_wb = g_src_mf_pitch;
        sf_bkgd_w  = g_src_mf_pitch/sf_bpp;
        sf_pad_h   = 1;
        break;

    case ROT_SRC_FORMAT_YCBCR_422_I_SW_SCAN      : //6
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 2;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = 0;
        sf_src_h   = 0;
        sf_bpp     = 0;
        sf_bkgd_wb = 0;
        sf_bkgd_w  = 0;
        sf_pad_h   = 0;
        break;

    case ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK : //7
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 2;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/32/mf_bpp;
        mf_pad_h   = 32;
        sf_src_w   = 0;
        sf_src_h   = 0;
        sf_bpp     = 0;
        sf_bkgd_wb = 0;
        sf_bkgd_w  = 0;
        sf_pad_h   = 0;
        break;

    case ROT_SRC_FORMAT_YCBCR_444_3P_SW_SCAN     : //8
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 1;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = g_src_width;
        sf_src_h   = g_src_height;
        sf_bpp     = 1;
        sf_bkgd_wb = g_src_mf_pitch;
        sf_bkgd_w  = g_src_mf_pitch/sf_bpp;
        sf_pad_h   = 1;
        break;

    case ROT_SRC_FORMAT_YCBCR_444_2P_SW_SCAN    : //9
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 1;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = g_src_width;
        sf_src_h   = g_src_height;
        sf_bpp     = 2;
        sf_bkgd_wb = g_src_mf_pitch*2;
        sf_bkgd_w  = g_src_mf_pitch*2/sf_bpp;
        sf_pad_h   = 1;
        break;


    case ROT_SRC_FORMAT_RGB565      : //12
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 2;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = 0;
        sf_src_h   = 0;
        sf_bpp     = 0;
        sf_bkgd_wb = 0;
        sf_bkgd_w  = 0;
        sf_pad_h   = 0;
        break;

    case ROT_SRC_FORMAT_RGB888      : //13
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 3;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = 0;
        sf_src_h   = 0;
        sf_bpp     = 0;
        sf_bkgd_wb = 0;
        sf_bkgd_w  = 0;
        sf_pad_h   = 0;
        break;

    case ROT_SRC_FORMAT_XRGB8888      : //14
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 4;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = 0;
        sf_src_h   = 0;
        sf_bpp     = 0;
        sf_bkgd_wb = 0;
        sf_bkgd_w  = 0;
        sf_pad_h   = 0;
        break;

    case ROT_SRC_FORMAT_RGBX8888      : //15
        mf_src_w   = g_src_width;
        mf_src_h   = g_src_height;
        mf_bpp     = 4;
        mf_bkgd_wb = g_src_mf_pitch;
        mf_bkgd_w  = g_src_mf_pitch/mf_bpp;
        mf_pad_h   = 1;
        sf_src_w   = 0;
        sf_src_h   = 0;
        sf_bpp     = 0;
        sf_bkgd_wb = 0;
        sf_bkgd_w  = 0;
        sf_pad_h   = 0;
        break;

    default:    // reserved
        DDP_ROT_LOG("ROT: Error in disp_rot_src_offset()\n");
        return 0;
    }

    // address offset
    tmp_0  = ((flip == 0) && (rotation == 1)) || ((flip == 0) && (rotation == 2)) || ((flip == 1) && (rotation == 2)) || ((flip == 1) && (rotation == 3)) ;
    tmp_1  = ((flip == 0) && (rotation == 2)) || ((flip == 0) && (rotation == 3)) || ((flip == 1) && (rotation == 0)) || ((flip == 1) && (rotation == 3)) ;

    /*  //move blk extra offset to h/w regbuf @20110727
    mf_vdo_frm_0   = ( (src_format==ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK) ||
    (src_format==ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK) ||
    (src_format==ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK ) ) *
    ( ((flip == 0) && (rotation == 1)) || ((flip == 1) && (rotation == 2)) ) *512;
    mf_vdo_frm_1   = ( (src_format==ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK) ||
    (src_format==ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK) ||
    (src_format==ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK ) ) *
    ( ((flip == 0) && (rotation == 3)) || ((flip == 1) && (rotation == 0)) ) *512;
    sf_vdo_frm_0   = mf_vdo_frm_0 / 2;
    sf_vdo_frm_1   = mf_vdo_frm_1 / 2;

    mf_vdo_fld_0   = ( ((src_format==ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK)||
    (src_format==ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK)) * (vdo_mode==1) && (vdo_field==1) ) *
    ( ((flip == 0) && (rotation == 0)) || ((flip == 1) && (rotation == 1)) ||
    ((flip == 0) && (rotation == 3)) || ((flip == 1) && (rotation == 0)) ) *256;
    mf_vdo_fld_1   = ( ((src_format==ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK)||
    (src_format==ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK)) * (vdo_mode==1) && (vdo_field==0) ) *
    ( ((flip == 0) && (rotation == 1)) || ((flip == 1) && (rotation == 2)) ||
    ((flip == 0) && (rotation == 2)) || ((flip == 1) && (rotation == 3)) ) *256;
    sf_vdo_fld_0   = mf_vdo_fld_0 / 2;
    sf_vdo_fld_1   = mf_vdo_fld_1 / 2;

    mf_offset      = (mf_bkgd_w * mf_src_h * mf_bpp - mf_bkgd_wb) * tmp_0 + (mf_src_w * mf_bpp * mf_pad_h) * tmp_1
    + mf_vdo_frm_0 - mf_vdo_frm_1 + mf_vdo_fld_0 - mf_vdo_fld_1;
    sf_offset      = (sf_bkgd_w * sf_src_h * sf_bpp - sf_bkgd_wb) * tmp_0 + (sf_src_w * sf_bpp * sf_pad_h) * tmp_1
    + sf_vdo_frm_0 - sf_vdo_frm_1 + sf_vdo_fld_0 - sf_vdo_fld_1;
    */

    //offset from pic left-up start to h/w read starting address
    //tmp_0 = r1/r2/r6/r7    0/5    3/4
    //tmp_1 = r2/r3/r4/r7    1/6    2/7
    mf_offset      = (mf_bkgd_w * mf_src_h * mf_bpp - mf_bkgd_wb) * tmp_0 + (mf_src_w * mf_bpp * mf_pad_h) * tmp_1;
    sf_offset      = (sf_bkgd_w * sf_src_h * sf_bpp - sf_bkgd_wb) * tmp_0 + (sf_src_w * sf_bpp * sf_pad_h) * tmp_1;

    //offset to clip area
    mf_base_offset = (mf_bkgd_w * boff_h + boff_w * mf_pad_h) * mf_bpp;
    sf_base_offset = (sf_bkgd_w * boff_h + boff_w * sf_pad_h) * sf_bpp;

    offset = (pln==0) ? (mf_base_offset + mf_offset) : (sf_base_offset + sf_offset);

    return offset;
}

void disp_rot_reg_src(ROT_SRC_FORMAT src_format,
                      const unsigned src_addr[3],
                      int src_width,
                      int src_height,
                      int src_pitch,
                      const struct DISP_REGION* clipRect,
                      ROT_VDO_MODE src_vdo_mode,
                      ROT_VDO_FIELD src_vdo_field)
{
    int main_offset_x, main_offset_y;
    unsigned sub_offset_x, sub_offset_y;
    int src_base_0, src_base_1, src_base_2;

    struct DISP_REGION clip_region;
    clip_region.x = clipRect->x;
    clip_region.y = clipRect->y;
    clip_region.width = clipRect->width;
    clip_region.height = clipRect->height;

    DDP_ROT_LOG("DISP_ROT config...\n");
    DDP_ROT_LOG(" format : %d\n", src_format);
    DDP_ROT_LOG(" src_addr : %p\n", (void*)src_addr[0]);
    DDP_ROT_LOG(" (w, h, pitch) : %d %d %d\n", src_width, src_height, src_pitch);

    if ((src_format > ROT_SRC_FORMAT_YCBCR_444_2P_SW_SCAN && src_format < ROT_SRC_FORMAT_RGB565) ||
        (src_format > ROT_SRC_FORMAT_SRC_FORMAT_MAX))
    {
        DDP_ROT_LOG("Error source format\n");
        ASSERT(0);
    }

    if (clip_region.x > src_width)
    {
        DDP_ROT_LOG("error: clip_region.x=%d > src_width=%d !\n", clip_region.x, src_width);
        clip_region.x = 0;
        clip_region.width = src_width;
    }

    if (clip_region.y > src_height)
    {
        DDP_ROT_LOG("error: clip_region.y=%d > src_height=%d !\n", clip_region.y, src_height);
        clip_region.y = 0;
        clip_region.height = src_height;
    }

    if (src_width < clip_region.x + clip_region.width)
    {
        DDP_ROT_LOG("src_width(%d) < clip_region.x(%d) + clip_region.w(%d) !\n",
            src_width, clip_region.x, clip_region.width);
        clip_region.width = src_width - clip_region.x;
    }

    if (src_height < clip_region.y + clip_region.height)
    {
        DDP_ROT_LOG("error: src_height(%d) < clip_region.y(%d) + clip_region.h(%d) !\n",
            src_height, clip_region.y, clip_region.height);
        clip_region.height = src_height - clip_region.y;
    }

#if 0
    unsigned src_swap        = ROT_SWAP_NONE;
    unsigned src_pad        = ROT_PAD_MSB;
    unsigned src_cosite        = ROT_COSITE_NONE;
    unsigned src_cus_rep    = ROT_CUS_REP_NONE;
    unsigned src_vdo_field    = ROT_VDO_FIELD_TOP;
    unsigned src_is_y_lsb    = ROT_Y_MSB;
#endif
    //unsigned src_vdo_mode    = ROT_VDO_MODE_FRAME;

    g_src_format        = src_format;
    g_src_mf_pitch        = disp_rot_get_bkgd(src_format, src_pitch, 0);
    g_src_sf_pitch        = disp_rot_get_bkgd(src_format, src_pitch, 1);

    g_src_addr[0]        = src_addr[0];
    g_src_addr[1]        = src_addr[1];
    g_src_addr[2]        = src_addr[2];

/*    if (src_format == ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK)
    {
        src_vdo_mode = ROT_VDO_MODE_FIELD;
    }*/

    g_vdo_mode            = src_vdo_mode;
    g_vdo_field           = src_vdo_field;

    // set register
#if 0
    SRC_CON_SET_IS_Y_LSB(SRC_CON, src_is_y_lsb);
    SRC_CON_SET_VDO_FIELD(SRC_CON, src_vdo_field);
    SRC_CON_SET_CUS_REP(SRC_CON, src_cus_rep);
    SRC_CON_SET_COSITE(SRC_CON, src_cosite);
    SRC_CON_SET_RGB_PAD(SRC_CON, src_pad);
    SRC_CON_SET_SRC_SWAP(SRC_CON, src_swap);
#endif
    DISP_REG_SET_FIELD(SRC_CON_FLD_VDO_MODE  , DISP_REG_ROT_SRC_CON, src_vdo_mode);
    DISP_REG_SET_FIELD(SRC_CON_FLD_VDO_FIELD , DISP_REG_ROT_SRC_CON, src_vdo_field);
    DISP_REG_SET_FIELD(SRC_CON_FLD_SRC_FORMAT, DISP_REG_ROT_SRC_CON, src_format);

    DISP_REG_SET_FIELD(MF_BKGD_SIZE_IN_BYTE_FLD_MF_BKGD_WB, DISP_REG_ROT_MF_BKGD_SIZE_IN_BYTE, g_src_mf_pitch);
    DISP_REG_SET_FIELD(SF_BKGD_SIZE_IN_BYTE_FLD_SF_BKGD_WB, DISP_REG_ROT_SF_BKGD_SIZE_IN_BYTE, g_src_sf_pitch);
    // To-Do: set sub frame pitch
    //

    // for format 3, padding output height to 2 byte align for height
    if (src_format == ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK)
    {
        if (g_rotation == ROT_DEGREE_90 || g_rotation == ROT_DEGREE_270)
        {
            clip_region.height = ALIGN(clip_region.height, 2);
        }
        else
            clip_region.width = ALIGN(clip_region.width, 2);
    }

    g_dst_clip_width    = clip_region.width;
    g_dst_clip_height    = clip_region.height;

    DISP_REG_SET_FIELD(MF_CLIP_SIZE_FLD_MF_CLIP_H, DISP_REG_ROT_MF_CLIP_SIZE, g_dst_clip_height);
    DISP_REG_SET_FIELD(MF_CLIP_SIZE_FLD_MF_CLIP_W, DISP_REG_ROT_MF_CLIP_SIZE, g_dst_clip_width);

    main_offset_x = clip_region.x;
    main_offset_y = clip_region.y;
    if (src_format == ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK ||
        src_format == ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK ||
        src_format == ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK)
    {
        int width, height;
        int offset_w, offset_h;
        width = ((clip_region.x % 16 + clip_region.width) / 16) * 16;
        if ((clip_region.x % 16 + clip_region.width) % 16 != 0)
            width += 16;

        height = ((clip_region.y % 32 + clip_region.height) / 32) * 32;
        if ((clip_region.y % 32 + clip_region.height) % 32 != 0)
            height += 32;

        DISP_REG_SET_FIELD(MF_SRC_SIZE_FLD_MF_SRC_H, DISP_REG_ROT_MF_SRC_SIZE, height);
        DISP_REG_SET_FIELD(MF_SRC_SIZE_FLD_MF_SRC_W, DISP_REG_ROT_MF_SRC_SIZE, width);

        main_offset_x = (clip_region.x / 16) * 16;
        main_offset_y = (clip_region.y / 32) * 32;
        offset_w = clip_region.x - main_offset_x;
        offset_h = clip_region.y - main_offset_y;

        DISP_REG_SET_FIELD(MF_OFFSET_1_FLD_MF_OFFSET_H_1, DISP_REG_ROT_MF_OFFSET_1, offset_h);
        DISP_REG_SET_FIELD(MF_OFFSET_1_FLD_MF_OFFSET_W_1, DISP_REG_ROT_MF_OFFSET_1, offset_w);

        g_src_width = width;
        g_src_height = height;
    }
    else
    {
        int align_width = src_width;   //clip_region.w;
        int align_height = src_height; //clip_region.h;

        switch(src_format)
        {
        case ROT_SRC_FORMAT_YCBCR_420_3P_SW_SCAN:            // 0
            align_width = ALIGN(align_width, 2);
            align_height = ALIGN(align_height, 2);
            break;

        case ROT_SRC_FORMAT_YCBCR_420_2P_SW_SCAN:            // 1
            align_width = ALIGN(align_width, 2);
            align_height = ALIGN(align_height, 2);
            break;

        case ROT_SRC_FORMAT_YCBCR_422_3P_SW_SCAN:            // 4
            align_width = ALIGN(align_width, 2);
            align_height = ALIGN(align_height, 1);
            break;

        case ROT_SRC_FORMAT_YCBCR_422_2P_SW_SCAN:            // 5
            align_width = ALIGN(align_width, 2);
            align_height = ALIGN(align_height, 1);
            break;

        case ROT_SRC_FORMAT_YCBCR_422_I_SW_SCAN:            // 6
            align_width = ALIGN(align_width, 8);
            align_height = ALIGN(align_height, 1);
            break;

        case ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK:        // 7
            align_width = ALIGN(align_width, 8);
            align_height = ALIGN(align_height, 32);
            break;

        default:
            break;
        }

        DISP_REG_SET(DISP_REG_ROT_MF_SRC_SIZE, align_height<<16 | align_width);
        DISP_REG_SET(DISP_REG_ROT_MF_OFFSET_1, clip_region.y<<16 | clip_region.x);

        g_src_width = align_width; //clip_region.w;
        g_src_height = align_height;  //clip_region.h;
    }

    // config address
    // we need to set base_offset and src_offset first
    // because we won't get base_offset later
    sub_offset_x = 0;
    sub_offset_y = 0;
    switch (g_src_format)
    {
    case ROT_SRC_FORMAT_YCBCR_420_3P_SW_SCAN:            // 0
    case ROT_SRC_FORMAT_YCBCR_420_2P_SW_SCAN:            // 1
    case ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK:        // 2
    case ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK:        // 3
        sub_offset_x = main_offset_x / 2;
        sub_offset_y = main_offset_y / 2;
        break;

    case ROT_SRC_FORMAT_YCBCR_422_3P_SW_SCAN:            // 4
    case ROT_SRC_FORMAT_YCBCR_422_2P_SW_SCAN:            // 5
    case ROT_SRC_FORMAT_YCBCR_422_I_SW_SCAN:            // 6
    case ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK:        // 7
        sub_offset_x = main_offset_x / 2;
        sub_offset_y = main_offset_y;
        break;

    case ROT_SRC_FORMAT_YCBCR_444_3P_SW_SCAN:            // 8
    case ROT_SRC_FORMAT_YCBCR_444_2P_SW_SCAN:            // 9
    case ROT_SRC_FORMAT_RGB565:                        // 12
    case ROT_SRC_FORMAT_RGB888:                        // 13
    case ROT_SRC_FORMAT_XRGB8888:                        // 14
    case ROT_SRC_FORMAT_RGBX8888:                        // 15
        sub_offset_x = main_offset_x;
        sub_offset_y = main_offset_y;
        break;
    }

    // for tile mode
    // int src_base_0 = src_addr[0]; // + disp_rot_src_offset(0, main_offset_x, main_offset_y);
    // int src_base_1 = src_addr[1]; // + disp_rot_src_offset(1, sub_offset_x, sub_offset_y);
    // int src_base_2 = src_addr[2]; // + disp_rot_src_offset(2, sub_offset_x, sub_offset_y);

    //
    src_base_0 = src_addr[0] + disp_rot_src_offset(0, main_offset_x, main_offset_y);
    src_base_1 = src_addr[1] + disp_rot_src_offset(1, sub_offset_x, sub_offset_y);
    src_base_2 = src_addr[2] + disp_rot_src_offset(2, sub_offset_x, sub_offset_y);

    DISP_REG_SET(DISP_REG_ROT_SRC_BASE_0, src_base_0);
    DISP_REG_SET(DISP_REG_ROT_SRC_BASE_1, src_base_1);
    DISP_REG_SET(DISP_REG_ROT_SRC_BASE_2, src_base_2);

    if (src_format >= ROT_SRC_FORMAT_RGB565 && src_format <= ROT_SRC_FORMAT_RGBX8888)
    {
        // TRANSFORM (use default value)
#if 1 //modify in 0306, use Lance's new FPGA bit file
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_0, 0x00010000);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_1, 0x00000000);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_2, 0x08020000);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_3, 0x02590132); //C01 C00
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_4, 0x1f530075); //C10 C02
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_5, 0x02001ead); //C12 C11
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_6, 0x1e530200); //C21 C20
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_7, 0x00001fad); //C22

#else
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_0, 0x00010000);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_1, 0x00000000);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_2, 0x08020000);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_3, 0x012c0099);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_4, 0x0faa003a);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_5, 0x01000f57);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_6, 0x0f2a0100);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_7, 0x00000fd7);
#endif
    }
    else
    {
        // TRANSFORM (use default value)
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_0, 0x00000005);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_1, 0x01716444);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_2, 0x1a8338da);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_3, 0x01e70aaa);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_4, 0x09c80495);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_5, 0x09f70f2c);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_6, 0x0db10ae1);
        DISP_REG_SET(DISP_REG_ROT_TRANSFORM_7, 0x0000084a);

    }
}

unsigned int disp_rot_get_main_linbuf_size(void)
{
    if (g_src_format == ROT_SRC_FORMAT_RGB565)
        return mb_lp * (mb_depth + g_buffer_mode) * 2;

    return mb_lp * (mb_depth + g_buffer_mode);
}

unsigned int disp_rot_get_sub_linbuf_size(void)
{
    if (g_src_format == ROT_SRC_FORMAT_RGB565)
        return 0;

    return sb_lp * (sb_depth + g_buffer_mode) * 2;
}

void disp_rot_linebuf_addr(unsigned main_addr, unsigned sub_addr)
{
    if ((main_addr & 0x0F) != 0)
    {
        DDP_ROT_LOG("main_addr not 16 byte alignment\n");
        ASSERT(0);
    }
    if ((sub_addr & 0x0F) != 0)
    {
        DDP_ROT_LOG("sub_addr not 16 byte alignment\n");
        ASSERT(0);
    }

    DISP_REG_SET(DISP_REG_ROT_MB_BASE, main_addr);
    DISP_REG_SET(DISP_REG_ROT_SB_BASE, sub_addr);
}

/*
get the parameter of MF & SF
*/
void disp_rot_set_mfsf_parameter(void)
{
    // for main frame
    int mf_src_w = g_src_width;
    int mf_src_h = g_src_height;
    int lb_4b_mode = g_lb_4b_mode;
    int buffer_mode = g_buffer_mode;

    switch(g_src_format)
    {
    case ROT_SRC_FORMAT_YCBCR_420_3P_SW_SCAN:            // 0
        //horizontal rotation
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 8;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w / 2;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        else    //horizontal rotation
        {
            mb_depth = 64;
            sb_depth = 64;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w / 2;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        break;

    case ROT_SRC_FORMAT_YCBCR_420_2P_SW_SCAN:            // 1
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 8;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth * 2;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;

        }
        else    //horizontal rotation
        {
            mb_depth = 64;
            sb_depth = 64;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth * 2;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        break;

    case ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK:        // 2
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 8;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = 512;
            sf_jump = 256;
            mf_sb = mf_src_w * 8;
            sf_sb = mf_src_w * 8;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = (int)((mb_pps_ori + 15) / 16) * 16;
            sb_pps = (int)((sb_pps_ori + 7) / 8) * 8;
        }
        else    //horizontal rotation
        {
            mb_depth = 16;
            sb_depth = 8;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = 512;
            sf_jump = 256;
            mf_sb = mf_src_w * 8;
            sf_sb = mf_src_w * 8;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = (int)((mb_pps_ori + 3) / 4) * 4;
            sb_pps = (int)((sb_pps_ori + 3) / 4) * 4;
        }
        break;

    case ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK:        // 3
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 8;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = 512;
            sf_jump = 256;
            mf_sb = mf_src_w * 8;
            sf_sb = mf_src_w * 8;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = (int)((mb_pps_ori + 15) / 16) * 16;
            sb_pps = (int)((sb_pps_ori + 7) / 8) * 8;
        }
        else    //horizontal rotation
        {
            mb_depth = 16;
            sb_depth = 8;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = 512;
            sf_jump = 256;
            mf_sb = mf_src_w * 8;
            sf_sb = mf_src_w * 8;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = (int)((mb_pps_ori + 3) / 4) * 4;
            sb_pps = (int)((sb_pps_ori + 3) / 4) * 4;
        }
        break;

    case ROT_SRC_FORMAT_YCBCR_422_3P_SW_SCAN:            // 4
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 8;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w / 2;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        else    //horizontal rotation
        {
            mb_depth = 64;
            sb_depth = 32;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w / 2;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        break;

    case ROT_SRC_FORMAT_YCBCR_422_2P_SW_SCAN:            // 5
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 8;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth * 2;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        else    //horizontal rotation
        {
            mb_depth = 64;
            sb_depth = 32;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth * 2;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        break;

    case ROT_SRC_FORMAT_YCBCR_422_I_SW_SCAN:            // 6
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 8;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth * 2;
            sf_jump = 0;
            mf_sb = mf_src_w * 2;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        else    //horizontal rotation
        {
            mb_depth = 64;
            sb_depth = mb_depth / 2;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth * 2;
            sf_jump = 0;
            mf_sb = mf_src_w * 2;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        break;

    case ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK:        // 7
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 8;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p / 2;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = 512;
            sf_jump = 0;
            mf_sb = mf_src_w * 16;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = (int)((mb_pps_ori + 15) / 16) * 16;
            sb_pps = (int)((sb_pps_ori + 7) / 8) * 8;
        }
        else    //horizontal rotation
        {
            mb_depth = 8;
            sb_depth = 4;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = 512;
            sf_jump = 0;
            mf_sb = mf_src_w * 16;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = (int)((mb_pps_ori + 3) / 4) * 4;
            sb_pps = (int)((sb_pps_ori + 3) / 4) * 4;
        }
        break;

    case ROT_SRC_FORMAT_YCBCR_444_3P_SW_SCAN:            // 8
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 6;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+3)/4)*4 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+3)/4)*4 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        else    //horizontal rotation
        {
            mb_depth = 64;
            sb_depth = 32;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+3)/4)*4 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+3)/4)*4 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        break;

    case ROT_SRC_FORMAT_YCBCR_444_2P_SW_SCAN:            // 9
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 6;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth * 2;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w * 2;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+3)/4)*4 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+3)/4)*4 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        else    //horizontal rotation
        {
            mb_depth = 64;
            sb_depth = 32;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth;
            sf_jump = sb_depth * 2;
            mf_sb = mf_src_w;
            sf_sb = mf_src_w * 2;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+3)/4)*4 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+3)/4)*4 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        break;

    case ROT_SRC_FORMAT_RGB565:                        // 12
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 8;
            sb_depth = 8;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p;
            mb_bpp = 0;
            sb_bpp = 2;
            mf_jump = mb_depth * 2;
            sf_jump = 0;
            mf_sb = mf_src_w * 2;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        else    //horizontal rotation
        {
            mb_depth = 64;
            sb_depth = mb_depth;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p;
            mb_bpp = 0;
            sb_bpp = 2;
            mf_jump = mb_depth * 2;
            sf_jump = 0;
            mf_sb = mf_src_w * 2;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }

        break;

    case ROT_SRC_FORMAT_RGB888:                        // 13
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 6;
            sb_depth = 6;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth * 3;
            sf_jump = 0;
            mf_sb = mf_src_w * 3;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        else    //horizontal rotation
        {
            mb_depth = 32;
            sb_depth = 32;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth * 3;
            sf_jump = 0;
            mf_sb = mf_src_w * 3;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        break;

    case ROT_SRC_FORMAT_XRGB8888:                        // 14
    case ROT_SRC_FORMAT_RGBX8888:                        // 15
        if((g_rotation == ROT_DEGREE_0) || (g_rotation == ROT_DEGREE_180))
        {
            mb_depth = 6;
            sb_depth = 6;
            mb_w_p = mf_src_w;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth * 4;
            sf_jump = 0;
            mf_sb = mf_src_w * 4;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        else    //horizontal rotation
        {
            mb_depth = 8; //32;
            sb_depth = mb_depth ;
            mb_w_p = mf_src_h;
            sb_w_p = mb_w_p;
            mb_bpp = 1;
            sb_bpp = 2;
            mf_jump = mb_depth * 4;
            sf_jump = 0;
            mf_sb = mf_src_w * 4;
            sf_sb = 0;

            mb_pps_ori = (int)((mb_w_p + mb_depth - 1) / mb_depth);
            sb_pps_ori = (int)((sb_w_p + sb_depth - 1) / sb_depth);
            mb_pps = ((lb_4b_mode ? (int)((mb_pps_ori+1)/2)*2 : mb_pps_ori) + 1) & 0xFFFFFFFE;
            sb_pps = ((lb_4b_mode ? (int)((sb_pps_ori+1)/2)*2 : sb_pps_ori) + 1) & 0xFFFFFFFE;
        }
        break;
    }

    mb_lp = mb_pps * (mb_depth + buffer_mode);
    sb_lp = sb_pps * (sb_depth + buffer_mode);

    DISP_REG_SET_FIELD(MF_PAR_FLD_MF_SB     , DISP_REG_ROT_MF_PAR,   mf_sb);
    DISP_REG_SET_FIELD(MF_PAR_FLD_MF_JUMP   , DISP_REG_ROT_MF_PAR,   mf_jump);
    DISP_REG_SET_FIELD(SF_PAR_FLD_SF_SB     , DISP_REG_ROT_SF_PAR,   sf_sb);
    DISP_REG_SET_FIELD(SF_PAR_FLD_SF_JUMP   , DISP_REG_ROT_SF_PAR,   sf_jump);
    DISP_REG_SET_FIELD(MB_DEPTH_FLD_MB_DEPTH, DISP_REG_ROT_MB_DEPTH, mb_depth);
    DISP_REG_SET_FIELD(MB_CON_FLD_MB_LP     , DISP_REG_ROT_MB_CON,   mb_lp);
    DISP_REG_SET_FIELD(MB_CON_FLD_MB_PPS    , DISP_REG_ROT_MB_CON,   mb_pps);
    DISP_REG_SET_FIELD(SB_DEPTH_FLD_SB_DEPTH, DISP_REG_ROT_SB_DEPTH, sb_depth);
    DISP_REG_SET_FIELD(SB_CON_FLD_SB_LP     , DISP_REG_ROT_SB_CON,   sb_lp);
    DISP_REG_SET_FIELD(SB_CON_FLD_SB_PPS    , DISP_REG_ROT_SB_CON,   sb_pps);

}

/*
rotator reg enable
*/
void disp_rot_reg_enable(BOOL enable)
{
    DISP_REG_SET(DISP_REG_ROT_DEBUG_CON, 0x01);

    if (enable)
    {
        unsigned main_buffer_ptr;
        unsigned sub_buffer_ptr;

        disp_rot_set_mfsf_parameter();

#if 1
        // internal sysram must 16 byte alignment
        //int main_size = disp_rot_get_main_linbuf_size();
        //int sub_size = disp_rot_get_sub_linbuf_size();

        main_buffer_ptr = 0x0000;
        if(g_src_format == ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK)
        {
            sub_buffer_ptr = 0x5600;
        }
        else
        {
            sub_buffer_ptr = 0x4400;
        }
        if(g_src_format == ROT_SRC_FORMAT_RGB565)
        {
            sub_buffer_ptr = 0;
        }

        disp_rot_linebuf_addr((unsigned)main_buffer_ptr, (unsigned)sub_buffer_ptr);
#else
        ///TODO: needs get the formula from CS
        #if 0
        *(volatile kal_uint32 *)(DISPSYS_ROT_BASE + 0x88) = 0x05a0000c;
        *(volatile kal_uint32 *)(DISPSYS_ROT_BASE + 0xc0) = 0x00000004;
        *(volatile kal_uint32 *)(DISPSYS_ROT_BASE + 0xc8) = 0x00000000;
        *(volatile kal_uint32 *)(DISPSYS_ROT_BASE + 0xd0) = 0x02580078;
        *(volatile kal_uint32 *)(DISPSYS_ROT_BASE + 0xd8) = 0x00000004;
        *(volatile kal_uint32 *)(DISPSYS_ROT_BASE + 0xe0) = 0x00004400;
        *(volatile kal_uint32 *)(DISPSYS_ROT_BASE + 0xe8) = 0x02580078;
        #else
        DISP_REG_SET(DISP_REG_ROT_MF_PAR, 0x05a0000c);
        DISP_REG_SET(DISP_REG_ROT_MB_DEPTH, 0x00000004);
        DISP_REG_SET(DISP_REG_ROT_MB_BASE, 0x00000000);
        DISP_REG_SET(DISP_REG_ROT_MB_CON, 0x02580078);
        DISP_REG_SET(DISP_REG_ROT_SB_DEPTH, 0x00000004);
        DISP_REG_SET(DISP_REG_ROT_SB_BASE, 0x00004400);
        DISP_REG_SET(DISP_REG_ROT_SB_CON, 0x02580078);
        #endif
#endif

        // interrupt enable under run & frame complete
        DISP_REG_SET_FIELD(INTERRUPT_ENABLE_FLD_UNDERRUN_INT_EN      , DISP_REG_ROT_INTERRUPT_ENABLE, 0);
        DISP_REG_SET_FIELD(INTERRUPT_ENABLE_FLD_REG_UPDATE_INT_EN    , DISP_REG_ROT_INTERRUPT_ENABLE, 0);
        DISP_REG_SET_FIELD(INTERRUPT_ENABLE_FLD_FRAME_COMPLETE_INT_EN, DISP_REG_ROT_INTERRUPT_ENABLE, 0);

        // debug
        // disp_dump_reg(DISP_REG_ROT);
    }

    DISP_REG_SET_FIELD(EN_FLD_ROT_ENABLE, DISP_REG_ROT_EN, enable);
}

/*
rotator reg reset
*/
void disp_rot_reg_reset(void)
{
    unsigned int delay_cnt=0;
    DISP_REG_SET(DISP_REG_ROT_RESET, 1);
    while((DISP_REG_GET(DISP_REG_ROT_MON_STA_1)&0xfff00)!=0x100)
    {
         delay_cnt++;
         if(delay_cnt>10000)
         {
             printk("[DDP] error, disp_rot_reg_reset() timeout! \n");
             disp_dump_reg(DISP_MODULE_ROT);
             //disp_dump_reg(DISP_MODULE_CONFIG); 
             break;
         }
    }
    DISP_REG_SET(DISP_REG_ROT_RESET, 0);    
    DISP_REG_SET(DISP_REG_ROT_GMCIF_CON, 0x00000771);  ///TODO:BIN need check for test case7
}
