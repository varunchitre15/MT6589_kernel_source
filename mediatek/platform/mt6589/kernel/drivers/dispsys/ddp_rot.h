#ifndef _DDP_ROT_API_H_
#define _DDP_ROT_API_H_

#include <mach/mt_typedefs.h>
#include "ddp_drv.h"

typedef enum
{
    ROT_DEGREE_0    = 0,
    ROT_DEGREE_90   = 1,
    ROT_DEGREE_180  = 2,
    ROT_DEGREE_270  = 3,

    ROT_DEGREE_MAX  = ROT_DEGREE_270
}ROT_DEGREE;

typedef enum
{
    ROT_FLIP_DISABLE = 0,
    ROT_FLIP_ENABLE  = 1,

    ROT_FLIP_MAX     = ROT_FLIP_ENABLE
}ROT_FLIP;

typedef enum
{
    ROT_SRC_FORMAT_YCBCR_420_3P_SW_SCAN            =  0, // 3 plane
    ROT_SRC_FORMAT_YCBCR_420_2P_SW_SCAN            =  1, // 2 plane
    ROT_SRC_FORMAT_YCBCR_420_2P_HW_ISP_BLOCK       =  2,
    ROT_SRC_FORMAT_YCBCR_420_2P_HW_VDO_BLOCK       =  3,
    ROT_SRC_FORMAT_YCBCR_422_3P_SW_SCAN            =  4,
    ROT_SRC_FORMAT_YCBCR_422_2P_SW_SCAN            =  5,
    ROT_SRC_FORMAT_YCBCR_422_I_SW_SCAN             =  6, // 1 plane
    ROT_SRC_FORMAT_YCBCR_422_I_HW_ISP_BLOCK        =  7,
    ROT_SRC_FORMAT_YCBCR_444_3P_SW_SCAN            =  8,
    ROT_SRC_FORMAT_YCBCR_444_2P_SW_SCAN            =  9,
    ROT_SRC_FORMAT_RGB565                          = 12,
    ROT_SRC_FORMAT_RGB888                          = 13,
    ROT_SRC_FORMAT_XRGB8888                        = 14,
    ROT_SRC_FORMAT_RGBX8888                        = 15,

    ROT_SRC_FORMAT_SRC_FORMAT_MAX                  = ROT_SRC_FORMAT_RGBX8888
}ROT_SRC_FORMAT;

typedef enum
{
    ROT_SWAP_NONE   = 0,
    ROT_SWAP_VUY    = 1,
    ROT_SWAP_YVU    = 2,
    ROT_SWAP_UVY    = 3,
    ROT_SWAP_UYV    = 4,
    ROT_SWAP_VYU    = 5,

    ROT_SWAP_MAX    = ROT_SWAP_VYU
}ROT_SWAP;

typedef enum
{
    ROT_PAD_MSB     = 0,
    ROT_PAD_ZERO    = 1,

    ROT_PAD_MAX     = ROT_PAD_ZERO
}ROT_PAD;

typedef enum
{
    ROT_COSITE_NONE        = 0,
    ROT_COSITED            = 1,

    ROT_COSITE_MAX         = ROT_COSITED
}ROT_COSITE;

typedef enum
{
    ROT_CUS_REP_NONE        = 0,
    ROT_CUS_REP_H_FILTER    = 1,
    ROT_CUS_REP_V_FILTER    = 2,
    ROT_CUS_REP_VH_FILTER   = 3,

    ROT_CUS_REP_MAX         = ROT_CUS_REP_VH_FILTER
}ROT_CUS_REP;

typedef enum
{
    ROT_VDO_MODE_FRAME      = 0,
    ROT_VDO_MODE_FIELD      = 1,

    ROT_VDO_MODE_MAX        = ROT_VDO_MODE_FIELD
}ROT_VDO_MODE;

typedef enum
{
    ROT_VDO_FIELD_TOP        = 0,
    ROT_VDO_FIELD_BOTTOM     = 1,

    ROT_VDO_FIELD_MAX        = ROT_VDO_FIELD_BOTTOM
}ROT_VDO_FIELD;

typedef enum
{
    ROT_Y_MSB                = 0,
    ROT_Y_LSB                = 1,

    ROT_IS_Y_LSB_MAX        = ROT_Y_LSB
}ROT_IS_Y_LSB;

typedef enum
{
    ROT_FRAME_COMPLETE_INT  = 0x01,
    ROT_REG_UPDATE_INT      = 0x02,
    ROT_UNDERRUN_INT        = 0x04,

    ROT_INTERRUPT_ALL       = ROT_FRAME_COMPLETE_INT | ROT_REG_UPDATE_INT | ROT_UNDERRUN_INT,

    ROT_INTERRUPT_MAX       = ROT_INTERRUPT_ALL
}ROT_INTERRUPT;

// start module
int ROTStart(void);

// stop module
int ROTStop(void);

// reset module
int ROTReset(void);

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
        DISP_COLOR_FORMAT *outFormat);

ROT_SRC_FORMAT disp_rot_format(DISP_COLOR_FORMAT format);
void disp_rot_reg_init(void);
void disp_rot_reg_con(ROT_DEGREE rot_degree, int rot_flip);
void disp_rot_reg_src(ROT_SRC_FORMAT src_format,
                      const unsigned src_addr[3],
                      int src_width,
                      int src_height,
                      int src_pitch,
                      const struct DISP_REGION* clipRect,
                      ROT_VDO_MODE src_vdo_mode,
                      ROT_VDO_FIELD src_vdo_field);
unsigned int disp_rot_get_main_linbuf_size(void);
unsigned int disp_rot_get_sub_linbuf_size(void);
void disp_rot_linebuf_addr(unsigned main_addr, unsigned sub_addr);
void disp_rot_set_mfsf_parameter(void);
void disp_rot_reg_enable(BOOL enable);
void disp_rot_reg_reset(void);

#define EN_FLD_ROT_ENABLE                                      REG_FLD(1, 0)

#define RESET_FLD_WARM_RESET                                   REG_FLD(1, 0)

#define INTERRUPT_ENABLE_FLD_UNDERRUN_INT_EN                   REG_FLD(1, 2)
#define INTERRUPT_ENABLE_FLD_REG_UPDATE_INT_EN                 REG_FLD(1, 1)
#define INTERRUPT_ENABLE_FLD_FRAME_COMPLETE_INT_EN             REG_FLD(1, 0)

#define INTERRUPT_STATUS_FLD_UNDERRUN_INT                      REG_FLD(1, 2)
#define INTERRUPT_STATUS_FLD_REG_UPDATE_INT                    REG_FLD(1, 1)
#define INTERRUPT_STATUS_FLD_FRAME_COMPLETE_INT                REG_FLD(1, 0)

#define CON_FLD_LB_4B_MODE                                     REG_FLD(1, 12)
#define CON_FLD_BUFFER_MODE                                    REG_FLD(1, 8)
#define CON_FLD_FLIP                                           REG_FLD(1, 6)
#define CON_FLD_ROTATION_DEGREE                                REG_FLD(2, 4)

#define GMCIF_CON_FLD_THROTTLE_PERIOD                          REG_FLD(12, 20)
#define GMCIF_CON_FLD_THROTTLE_EN                              REG_FLD(1, 19)
#define GMCIF_CON_FLD_ULTRA_EN_INT                             REG_FLD(1, 16)
#define GMCIF_CON_FLD_ULTRA_EN                                 REG_FLD(4, 12)
#define GMCIF_CON_FLD_WRITE_REQUEST_TYPE                       REG_FLD(3, 8)
#define GMCIF_CON_FLD_READ_REQUEST_TYPE                        REG_FLD(3, 4)
#define GMCIF_CON_FLD_COMMAND_DIV                              REG_FLD(1, 0)

#define SRC_CON_FLD_IS_Y_LSB                                   REG_FLD(1, 16)
#define SRC_CON_FLD_VDO_FIELD                                  REG_FLD(1, 13)
#define SRC_CON_FLD_VDO_MODE                                   REG_FLD(1, 12)
#define SRC_CON_FLD_CUS_REP                                    REG_FLD(2, 9)
#define SRC_CON_FLD_COSITE                                     REG_FLD(1, 8)
#define SRC_CON_FLD_RGB_PAD                                    REG_FLD(1, 7)
#define SRC_CON_FLD_SRC_SWAP                                   REG_FLD(3, 4)
#define SRC_CON_FLD_SRC_FORMAT                                 REG_FLD(4, 0)

#define SRC_BASE_0_FLD_SRC_BASE_0                              REG_FLD(32, 0)

#define SRC_BASE_1_FLD_SRC_BASE_1                              REG_FLD(32, 0)

#define SRC_BASE_2_FLD_SRC_BASE_2                              REG_FLD(32, 0)

#define MF_BKGD_SIZE_IN_BYTE_FLD_MF_BKGD_WB                    REG_FLD(19, 0)

#define MF_SRC_SIZE_FLD_MF_SRC_H                               REG_FLD(11, 16)
#define MF_SRC_SIZE_FLD_MF_SRC_W                               REG_FLD(11, 0)

#define MF_CLIP_SIZE_FLD_MF_CLIP_H                             REG_FLD(11, 16)
#define MF_CLIP_SIZE_FLD_MF_CLIP_W                             REG_FLD(11, 0)

#define MF_OFFSET_1_FLD_MF_OFFSET_H_1                          REG_FLD(5, 16)
#define MF_OFFSET_1_FLD_MF_OFFSET_W_1                          REG_FLD(4, 0)

#define MF_PAR_FLD_MF_SB                                       REG_FLD(15, 16)
#define MF_PAR_FLD_MF_JUMP                                     REG_FLD(10, 0)

#define SF_BKGD_SIZE_IN_BYTE_FLD_SF_BKGD_WB                    REG_FLD(19, 0)

#define SF_PAR_FLD_SF_SB                                       REG_FLD(15, 16)
#define SF_PAR_FLD_SF_JUMP                                     REG_FLD(10, 0)

#define MB_DEPTH_FLD_MB_DEPTH                                  REG_FLD(7, 0)

#define MB_BASE_FLD_MB_BASE                                    REG_FLD(32, 0)

#define MB_CON_FLD_MB_LP                                       REG_FLD(12, 16)
#define MB_CON_FLD_MB_PPS                                      REG_FLD(11, 0)

#define SB_DEPTH_FLD_SB_DEPTH                                  REG_FLD(7, 0)

#define SB_BASE_FLD_SB_BASE                                    REG_FLD(32, 0)

#define SB_CON_FLD_SB_LP                                       REG_FLD(12, 16)
#define SB_CON_FLD_SB_PPS                                      REG_FLD(11, 0)

#define VC1_RANGE_FLD_VC1_MAP_PARA_C                           REG_FLD(5, 16)
#define VC1_RANGE_FLD_VC1_MAP_PARA_Y                           REG_FLD(5, 8)
#define VC1_RANGE_FLD_VC1_MAP_EN                               REG_FLD(1, 4)
#define VC1_RANGE_FLD_VC1_RED_EN                               REG_FLD(1, 0)

#define TRANSFORM_0_FLD_TRANS_EN                               REG_FLD(1, 16)
#define TRANSFORM_0_FLD_OUT_S_2                                REG_FLD(1, 10)
#define TRANSFORM_0_FLD_OUT_S_1                                REG_FLD(1, 9)
#define TRANSFORM_0_FLD_OUT_S_0                                REG_FLD(1, 8)
#define TRANSFORM_0_FLD_CLAMP_S_2                              REG_FLD(1, 6)
#define TRANSFORM_0_FLD_CLAMP_S_1                              REG_FLD(1, 5)
#define TRANSFORM_0_FLD_CLAMP_S_0                              REG_FLD(1, 4)
#define TRANSFORM_0_FLD_IN_S_2                                 REG_FLD(1, 2)
#define TRANSFORM_0_FLD_IN_S_1                                 REG_FLD(1, 1)
#define TRANSFORM_0_FLD_IN_S_0                                 REG_FLD(1, 0)

#define TRANSFORM_1_FLD_IN_OFFSET_2                            REG_FLD(9, 20)
#define TRANSFORM_1_FLD_IN_OFFSET_1                            REG_FLD(9, 10)
#define TRANSFORM_1_FLD_IN_OFFSET_0                            REG_FLD(9, 0)

#define TRANSFORM_2_FLD_OUT_OFFSET_2                           REG_FLD(9, 20)
#define TRANSFORM_2_FLD_OUT_OFFSET_1                           REG_FLD(9, 10)
#define TRANSFORM_2_FLD_OUT_OFFSET_0                           REG_FLD(9, 0)

#define TRANSFORM_3_FLD_C_01                                   REG_FLD(12, 16)
#define TRANSFORM_3_FLD_C_00                                   REG_FLD(12, 0)

#define TRANSFORM_4_FLD_C_10                                   REG_FLD(12, 16)
#define TRANSFORM_4_FLD_C_02                                   REG_FLD(12, 0)

#define TRANSFORM_5_FLD_C_12                                   REG_FLD(12, 16)
#define TRANSFORM_5_FLD_C_11                                   REG_FLD(12, 0)

#define TRANSFORM_6_FLD_C_21                                   REG_FLD(12, 16)
#define TRANSFORM_6_FLD_C_20                                   REG_FLD(12, 0)

#define TRANSFORM_7_FLD_C_22                                   REG_FLD(12, 0)

#define RESV_DUMMY_0_FLD_RESV_DUMMY_0                          REG_FLD(32, 0)

#define CHKS_EXTR_FLD_CHKS_EXTR_CRC                            REG_FLD(24, 8)
#define CHKS_EXTR_FLD_CHKS_EXTR_CLR                            REG_FLD(1, 0)

#define CHKS_INTW_FLD_CHKS_INTW_CRC                            REG_FLD(24, 8)
#define CHKS_INTW_FLD_CHKS_INTW_CLR                            REG_FLD(1, 0)

#define CHKS_INTR_FLD_CHKS_INTR_CRC                            REG_FLD(24, 8)
#define CHKS_INTR_FLD_CHKS_INTR_CLR                            REG_FLD(1, 0)

#define CHKS_ROTO_FLD_CHKS_ROTO_CRC                            REG_FLD(24, 8)
#define CHKS_ROTO_FLD_CHKS_ROTO_CLR                            REG_FLD(1, 0)

#define CHKS_SRIY_FLD_CHKS_SRIY_CRC                            REG_FLD(24, 8)
#define CHKS_SRIY_FLD_CHKS_SRIY_CLR                            REG_FLD(1, 0)

#define CHKS_SRIU_FLD_CHKS_SRIU_CRC                            REG_FLD(24, 8)
#define CHKS_SRIU_FLD_CHKS_SRIU_CLR                            REG_FLD(1, 0)

#define CHKS_SRIV_FLD_CHKS_SRIV_CRC                            REG_FLD(24, 8)
#define CHKS_SRIV_FLD_CHKS_SRIV_CLR                            REG_FLD(1, 0)

#define CHKS_SROY_FLD_CHKS_SROY_CRC                            REG_FLD(24, 8)
#define CHKS_SROY_FLD_CHKS_SROY_CLR                            REG_FLD(1, 0)

#define CHKS_SROU_FLD_CHKS_SROU_CRC                            REG_FLD(24, 8)
#define CHKS_SROU_FLD_CHKS_SROU_CLR                            REG_FLD(1, 0)

#define CHKS_SROV_FLD_CHKS_SROV_CRC                            REG_FLD(24, 8)
#define CHKS_SROV_FLD_CHKS_SROV_CLR                            REG_FLD(1, 0)

#define CHKS_VUPI_FLD_CHKS_VUPI_CRC                            REG_FLD(24, 8)
#define CHKS_VUPI_FLD_CHKS_VUPI_CLR                            REG_FLD(1, 0)

#define CHKS_VUPO_FLD_CHKS_VUPO_CRC                            REG_FLD(24, 8)
#define CHKS_VUPO_FLD_CHKS_VUPO_CLR                            REG_FLD(1, 0)

#define DEBUG_CON_FLD_BUF_RESV                                 REG_FLD(5, 4)
#define DEBUG_CON_FLD_CHKS_CRC_EN                              REG_FLD(1, 0)

#define MON_STA_0_FLD_MON_STA_0                                REG_FLD(32, 0)

#define MON_STA_1_FLD_MON_STA_1                                REG_FLD(32, 0)

#define MON_STA_2_FLD_MON_STA_2                                REG_FLD(32, 0)

#define MON_STA_3_FLD_MON_STA_3                                REG_FLD(32, 0)

#define MON_STA_4_FLD_MON_STA_4                                REG_FLD(32, 0)

#define MON_STA_5_FLD_MON_STA_5                                REG_FLD(32, 0)

#define MON_STA_6_FLD_MON_STA_6                                REG_FLD(32, 0)

#define MON_STA_7_FLD_MON_STA_7                                REG_FLD(32, 0)

#define MON_STA_8_FLD_MON_STA_8                                REG_FLD(32, 0)

#define MON_STA_9_FLD_MON_STA_9                                REG_FLD(32, 0)

#define MON_STA_10_FLD_MON_STA_10                              REG_FLD(32, 0)

#define MON_STA_11_FLD_MON_STA_11                              REG_FLD(32, 0)

#define MON_STA_12_FLD_MON_STA_12                              REG_FLD(32, 0)

#define MON_STA_13_FLD_MON_STA_13                              REG_FLD(32, 0)

#define MON_STA_14_FLD_MON_STA_14                              REG_FLD(32, 0)

#define MON_STA_15_FLD_MON_STA_15                              REG_FLD(32, 0)

#define MON_STA_16_FLD_MON_STA_16                              REG_FLD(32, 0)

#define MON_STA_17_FLD_MON_STA_17                              REG_FLD(32, 0)

#define MON_STA_18_FLD_MON_STA_18                              REG_FLD(32, 0)

#define MON_STA_19_FLD_MON_STA_19                              REG_FLD(32, 0)

#define MON_STA_20_FLD_MON_STA_20                              REG_FLD(32, 0)

#define MON_STA_21_FLD_MON_STA_21                              REG_FLD(32, 0)

#define MON_STA_22_FLD_MON_STA_22                              REG_FLD(32, 0)

#endif
