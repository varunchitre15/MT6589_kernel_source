
#ifndef _VDEC_HW_COMMON_H_
#define _VDEC_HW_COMMON_H_

#include "../include/vdec_usage.h"
#include "../include/vdec_info_common.h"
//#include "x_hal_ic.h"
//PANDA
#include "../include/vdec_common_if.h"
//#include <mach/hardware.h>
#include <mach/mt_reg_base.h>
#include <mach/sync_write.h>

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#endif

#define VP6_MULTI_STREAM
#define VP6_ALPHA_SUPPORT

#if ((CONFIG_DRV_VERIFY_SUPPORT || CONFIG_DRV_FPGA_BOARD))
#define VDEC_DRV_PARSER 0
#else
#define VDEC_DRV_PARSER 1
#endif

//#define VDEC_SIM_DUMP
//#define VP8_REG_DUMP
#define MEM_PAUSE_SUPPORT
//#define IO_BASE_ADDRESS                     0x70000000L
// *********************************************************************
// Video Decoder Registers define
// *********************************************************************
#define VLD_REG_OFFSET0   0x1000

#define VLD_TOP_REG_OFFSET0   (VLD_REG_OFFSET0 + 0x800)

#define MC_REG_OFFSET0    0x2000
#define VDSCL_REG_OFFSET0          0x2b800 /// no use 
#define AVC_VLD_REG_OFFSET0      0x3000
#define AVC_MV_REG_OFFSET0       0x4000
#define AVC_FG_REG_OFFSET0        0x31000 ///no use
#define DV_REG_OFFSET0        0x0

#define WMV_DCAC_REG_OFFSET0     0x5700 ///

#define WMV_MV_REG_OFFSET0       0x4000 /// 

#define VLD_REG_OFFSET1   0x2E000

#define VLD_TOP_REG_OFFSET1   (VLD_REG_OFFSET1 + 0x800)

#define MC_REG_OFFSET1    0x2F000
#define VDSCL_REG_OFFSET1          0x30800
#define AVC_VLD_REG_OFFSET1      0x30000
#define AVC_MV_REG_OFFSET1       0x31000
#define AVC_FG_REG_OFFSET1        0x32000
#define DV_REG_OFFSET1        0x32000
#define WMV_DCAC_REG_OFFSET1  0x34000
#define WMV_MV_REG_OFFSET1    0x31000


#define AVS_VLD_REG_OFFSET0    0x6000   //AVS VLD 0x1602-6000
#define AVS_VLD_REG_OFFSET1    0x2D000

#define AVS_PP_REG_OFFSET0    0x5000    //PP 0x16025000
#define AVS_PP_REG_OFFSET1      0x2C000

//RM IO Base
#define RM_VLD_REG_OFFSET0      (AVC_VLD_REG_OFFSET0 + 0xC00)
#define RM_VDEC_PP_BASE	      0x5000

#define VDEC_CRC_REG_OFFSET0    DV_REG_OFFSET0
#define VDEC_CRC_REG_EN              0x4
      #define VDEC_CRC_EN                      0x1
      #define VDEC_CRC_SRC_MC              (0x0 << 1)
      #define VDEC_CRC_SRC_PP              (0x1 << 1)
#define VDEC_CRC_Y_CHKSUM0       0x8
#define VDEC_CRC_Y_CHKSUM1       0xC
#define VDEC_CRC_Y_CHKSUM2       0x10
#define VDEC_CRC_Y_CHKSUM3       0x14
#define VDEC_CRC_C_CHKSUM0       0x18
#define VDEC_CRC_C_CHKSUM1       0x1C
#define VDEC_CRC_C_CHKSUM2       0x20
#define VDEC_CRC_C_CHKSUM3       0x24

#define VDEC_DV_INT_CFG          0xA4
#define VDEC_DV_LAT_BUF_BYPASS          0xF0
#define VDEC_DV_DEC_BREAK          0x100
	#define VDEC_DV_DEC_BREAK_EN          (0x01)
#define VDEC_DV_DEC_BREAK_STATUS          0x104
	#define VDEC_BREAK_OK_0			(0x01)
	#define VDEC_BREAK_OK_1			(0x01<<4)

#define VP6_VLD_REG_OFFSET0    0x7000
#define VP6_VLD_REG_OFFSET1    0x2E000
#define VP6_MV_REG_OFFSET0     0x4000
#define VP6_MV_REG_OFFSET1     0x2A000
#define VP6_PP_REG_OFFSET0     0x5000
#define VP6_DCAC_REG_OFFSET0   0x5700

#define VP6_VLD2_REG_OFFSET (VP6_VLD_REG_OFFSET0 + 0x800)   //MULTI-STREAM PANDA
#define VP6_VLD2_SHIFT_OFFSET (VP6_VLD_REG_OFFSET0 + 0x400)   //MULTI-STREAM PANDA

//VP8
#define VP8_VLD_REG_OFFSET0	(AVS_VLD_REG_OFFSET0+0x800)
#define VP8_MV_REG_OFFSET0	AVC_MV_REG_OFFSET0
#define VP8_VLD2_REG_OFFSET0	(VP6_VLD_REG_OFFSET0+0x800)
#define VP8_PP_REG_OFFSET0    0x5000
#define VP8_VLD_REG_OFFSET1	(AVS_VLD_REG_OFFSET1+0x800)
#define VP8_MV_REG_OFFSET1	AVC_MV_REG_OFFSET1
#define VP8_VLD2_REG_OFFSET1	(VP6_VLD_REG_OFFSET1+0x800)
#define VP8_PP_REG_OFFSET1    0x2C000

#define RW_PDN_CTRL                   0x0
#define RW_SYS_CLK_SEL                   0x84

// *********************************************************************
// VLD Registers define
// *********************************************************************
// Barral Shifter from 0x00 - 0x80
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
#define RW_VLD_TOP_TIMEOUT_ST_CLR              0x04
   #define VLD_TOP_TIMEOUT_CLR			((unsigned)0x01 << 0)
#define RW_VLD_TOP_PRED_ADDR                 0x28
#define RW_VLD_TOP_PRED_SRAM_CFG            0x2C // 11x4
   #define RW_PRED_SRAM_CFG_OUTRANGE_SUPPORT_FLAG 31
   #define RW_PRED_SRAM_CFG_SRAM_BYPASS_FLAG 30
#define RW_VLD_TOP_SEGID_DRAM_ADDR          0x3c // 15x4
#define RW_VLD_TOP_PRED_MODE                0x40 // 16x4
   #define RW_NUM_ROW_SEGID_DATA_M1_S 0
   #define RW_NUM_ROW_SEGID_DATA_M1_E 2
   #define RW_DRAM_BURST_MODE_S 4
   #define RW_DRAM_BURST_MODE_E 5
#define RW_VLD_TOP_WEBP_CTRL                0x48 // 18x4
   #define RW_BUFCTRL_ON_FLAG  0
   #define RW_RESIZE_INTER_ON_FLAG 4
#define RW_VLD_TOP_MB_DECSTART              0x4c // 19x4

#define RO_VLD_TOP_TIMEOUT_THD              0x50
   #define VLD_TOP_TIMEOUT_THD                 0xffffffff 
   
#define RW_VLD_TOP_TIMEOUT_SW              0x54
   #define VLD_TOP_DEC_CYCLE_EN          ((unsigned)0x1 << 0)
   
#define RW_VLD_TOP_BUSY_THRESHOLD  0x58
#define RW_VLD_TOP_ERR_CONCEAL             0x5C
#define RW_VLD_TOP_ERR_TYPE_ENABLE          0x64

#define RW_VLD_TOP_PIC_MB_SIZE_M1	0x68
#define RW_VLD_TOP_PIC_PIX_SIZE      0x70

    #define RW_PIC_WIDTH_IN_MBS_S 0
    #define RW_PIC_WIDTH_IN_MBS_E 9
    #define RW_PIC_HEIGHT_IN_MBS_S 16
    #define RW_PIC_HEIGHT_IN_MBS_E 25
#define RW_VLD_TOP_TOTAL_MBS_IN_PIC     0x6C

#define RW_VLD_TOP_VP8_ENHANCE     0x7C
#define RO_VLD_TOP_DEC_CYCLE                 0x80
#define RW_VLD_TOP_MV_DC_PRED_END_ADDR       0x94
#endif
#define RO_VLD_BARL           0x00
#define RW_VLD_VDOUFM     0x84
    #define VLD_VDOUFM       ((unsigned)0x1 << 0)
    #define VLD_MXOFF         ((unsigned)0x1 << 8)
    #define VLD_ENSTCNT      ((unsigned)0x1 << 9)
    #define VLD_AATO           ((unsigned)0x1 << 10)
  
// Type 18 [BIT14] out_of_picture error
// Type 1  [BIT15] block pixel overflow
// Type 2  [BIT16] cbp number mismatch
// Type 3  [BIT17] dct_dc_size_lum table error
// Type 4  [BIT18] dct_dc_size_chrom table error
// Type 5  [BIT19] slice-layer syntax error
// Type 6  [BIT20] end_of_block syntax rooer
// Type 7  [BIT21] macroblock address increment table error
// Type 8  [BIT22] coded_block_pattern table error
// Type 9  [BIT23] insufficient slice error
// Type 10 [BIT24] slice-level quantization scale error
// Type 11 [BIT25] other slice-level syntax error
// Type 12 [BIT26] macroblock address overflow error
// Type 13 [BIT27] mb_type table error
// Type 14 [BIT28] motion type error
// Type 15 [BIT29] motion code error
// Type 16 [BIT30] marker syntax error
// Type 17 [BIT31] MC-IDCT busy overflow
  #define ERR_OUT_PIC                 ((unsigned)0x1 << 14)
  #define ERR_BL_PIX                   ((unsigned)0x1 << 15)
  #define ERR_CBP_NS                  ((unsigned)0x1 << 16)
  #define ERR_DCT_Y_TAB             ((unsigned)0x1 << 17)
  #define ERR_DCT_C_TAB             ((unsigned)0x1 << 18)
  #define ERR_SL_SYN                   ((unsigned)0x1 << 19)
  #define ERR_EOB_SYN                ((unsigned)0x1 << 20)
  #define ERR_MB_ADD_INC_TAB   ((unsigned)0x1 << 21)
  #define ERR_CBP_TAB                ((unsigned)0x1 << 22)
  #define ERR_INS_SL                   ((unsigned)0x1 << 23)
  #define ERR_SL_QSCALE             ((unsigned)0x1 << 24)
  #define ERR_SL_SYN2                ((unsigned)0x1 << 25)
  #define ERR_MB_ADD                 ((unsigned)0x1 << 26)
  #define ERR_MB_TBL                  ((unsigned)0x1 << 27)
  #define ERR_MOT_TP                  ((unsigned)0x1 << 28)
  #define ERR_MOT_CD                 ((unsigned)0x1 << 29)
  #define ERR_MKB                       ((unsigned)0x1 << 30)
  #define ERR_MCIDCT_BSY          ((unsigned)0x1 << 31)

#define VLD_ERR_MONITOR  (ERR_BL_PIX + ERR_CBP_NS + ERR_DCT_Y_TAB + \
                          ERR_DCT_C_TAB + ERR_SL_SYN + ERR_EOB_SYN + \
                          ERR_MB_ADD_INC_TAB + ERR_CBP_TAB + ERR_INS_SL + \
                          ERR_SL_QSCALE + ERR_SL_SYN2 + ERR_MB_ADD + \
                          ERR_MB_TBL + ERR_MOT_TP + ERR_MOT_CD + \
                          ERR_MKB + ERR_MCIDCT_BSY + ERR_OUT_PIC)

  // Error Type Define
  #define ETP_OUT_PIC                 18
  #define ETP_BL_PIX                    1
  #define ETP_CBP_NS                   2
  #define ETP_DCT_Y_TAB             3
  #define ETP_DCT_C_TAB             4
  #define ETP_SL_SYN                   5
  #define ETP_EOB_SYN                6
  #define ETP_MB_ADD_INC_TAB   7
  #define ETP_CBP_TAB                 8
  #define ETP_INS_SL                    9
  #define ETP_SL_QSCALE             10
  #define ETP_SL_SYN2                 11
  #define ETP_MB_ADD                  12
  #define ETP_MB_TBL                   13
  #define ETP_MOT_TP                   14
  #define ETP_MOT_CD                  15
  #define ETP_MKB                        16
  #define ETP_MCIDCT_BSY           17

#define RW_MBSTART_DCAC_SWITCH     0x1F0

#define RW_VLD_PARA        0x88
#define RW_VLD_PROC        0x8C
    #define VLD_SSCBIT          ((unsigned)0x1 << 16)
    #define VLD_SSCBYTE      ((unsigned)0x1 << 17)
    #define VLD_ABRT            ((unsigned)0x1 << 18)
    #define VLD_PSUP            ((unsigned)0x1 << 19)
    #define VLD_INIFET          ((unsigned)0x1 << 20)
    #define VLD_MBDATA       ((unsigned)0x1 << 21)  // for 1389 MP ECO
    #define VLD_PDHW           ((unsigned)0x1 << 22)
    #define VLD_INIBR           ((unsigned)0x1 << 23)
    #define VLD_PERRCON      ((unsigned)0x1 << 24)
    #define VLD_RTERR          ((unsigned)0x1 << 25)
    #define VLD_EOFR            ((unsigned)0x1 << 26)
    #define VLD_IGBCL          ((unsigned)0x1 << 27)
    #define VLD_DECTOP       ((unsigned)0x1 << 29)
    #define VLD_DECBTM       ((unsigned)0x2 << 29)
    #define VLD_FPRBS          ((unsigned)0x3 << 29)
    #define VLD_B21EN          ((unsigned)0x1 << 31)

#define RW_VLD_PICSZ           0x90
#define RW_VLD_MBROWPRM  0x94
#define RW_VLD_DIGMBSA      0x98
#define RW_VLD_SCALE          0x9C
    #define H_SCALE_1_1    0
    #define H_SCALE_1_2    1
    #define H_SCALE_1_4    2
    #define H_SCALE_3_4    3
    #define H_SCALE_3_8    4
    #define V_SCALE_1_1    0
    #define V_SCALE_1_2    1
    #define V_SCALE_1_4    2

#define RW_VLD_TABLIM         0xA0
#define RW_VLD_TABLNIM       0xA4
#define RW_VLD_DIGMBYOFF   0xA8
#define RW_VLD_PSUPCTR       0xAC
#define RW_VLD_RPTR             0xB0
#define RW_VLD_VSTART         0xB4
#define RW_VLD_VEND            0xB8
#define RO_VLD_BLKFIN          0xBC

#define RW_VLD_RDY_SWTICH   0xC0
   #define READY_TO_RISC         (0x1 << 17)
   #define READY_TO_RISC_1     (0x1 << 18)
   #define READY_TO_RISC_2     (0x1 << 20)
#define RW_VLD_PWRSAVE        0xC4
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#define RW_VLD_PIC_W_MB        0xC8
#endif
#if (CONFIG_DRV_FPGA_BOARD)
#define RW_VLD_BS_SPEEDUP      0xCC
#endif
#define RW_VLD_ASYNC            0xD0
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
  #define VLD_WR_ENABLE      ((unsigned)0x1 << 18)
#endif
#define RW_VLD_TIMEOUT         0xDC
#define RO_VLD_FSBI               0xE0
#define RO_VLD_ERRFG             0xE4
#define RO_VLD_FETCHOK         0xE8
  #define VLD_FETCH_OK            ((unsigned)0x1 << 0)
  #define VLD_DRAM_REQ_FIN    ((unsigned)0x1 << 2)

#define RO_VLD_VBAR          0xEC
#define RO_VLD_INPUTWND  0xF0
#define RO_VLD_SRAMCTRL  0xF4
    #define AA_FIT_TARGET_SCLK (1<<0)
    #define AA_FIT_TARGET_D (1<<1)
    #define PROCESS_FLAG (1<<15)

#define RO_VLD_VWPTR       0xF8
#define RO_VLD_VRPTR        0xFC
#define RW_VLD_WAITT       0x100
#define WO_VLD_FDEC        0x104
    #define VLD_PIC_COMPLETE        ((unsigned)0x1 << 0)
    #define VLD_RELOAD_INTRA        ((unsigned)0x1 << 8)
    #define VLD_RELOAD_NONINTRA  ((unsigned)0x1 << 9)
    #define VLD_RST_QMATRIX         ((unsigned)0x1 << 10)

#define WO_VLD_SRST         0x108
#define WO_VLD_WPTR        0x110
  #define  VLD_CLEAR_PROCESS_EN   ((unsigned)0x1 << 0)
  #define VLD_RISC_WR_EN      ((unsigned)0x1 << 1)
#define WO_VLD_BBUFRST   0x10C
    #define VLD_MVOF_DET_DISABLE  (((unsigned)0x1 << 4) | ((unsigned)0x1 << 5) | ((unsigned)0x1 << 11))  //mv overflow detection disable bit
#define RO_VLD_SUM           0x114
#define RW_VLD_DIGEST_BOUNDARY  0x1A4
#define VLD_RESYNC_MARK_ECO    0x2

// Display Initialization
#define RW_VLD_RESET_COMMOM_SRAM   0x2EC
// *********************************************************************
// MPEG4 relate Registers define
// *********************************************************************
#define RW_VLD_MP4_HDR          0x1C4
#define RW_VLD_MP4_FLG          0x1C8
#define RW_VLD_LDSR                0x1CC
#define RW_VLD_LDSH                0x1D0
#define RW_VLD_QUANT              0x1D4
#define RW_VLD_M4SH                0x1D8
#define RW_VLD_DX3                  0x1DC
#define RO_VLD_MP4DECBSY       0x1E0
#define RW_VLD_DECSTART         0x1E4
#define RW_VLD_BREF                 0x1E8
#define RO_VLD_MP4DECERR        0x1F4
#define RW_VLD_DCACWK            0x1FC

#if (MPEG4_6589_SUPPORT)
//6589NEW 2.3
#define RW_VLD_MP4_DATA_PARTITION_SZ    0x2BC
#endif

#define RW_VLD_DVX_BOUND_MODE     0x2C0
#define RW_VLD_DVX_BOUND_R1     0x2C4
#define RW_VLD_DVX_BOUND_R2     0x2C8
#define RW_VLD_DVX_BOUND_R3     0x2CC

#define RW_VLD_MP4_DATA_PARTITION 0x2D0

#if (MPEG4_6589_SUPPORT)
//6589NEW 2.4-2.6
#define RW_VLD_MP4_NOT_CODED_SA         0x2D4
#define RW_VLD_MP4_DATA_PARTITION_SA    0x2D8
#define RW_VLD_MP4_DCAC_NOT_CODED_SZ    0x2DC
#endif

#define	RW_VLD_MP4_ERR_MASK		0x2E0
#define	RW_VLD_MP4_ERR_DET2		0x2F8
#define	RW_VLD_MP4_ERR_DET1		0x2F4

#if (MPEG4_6582_SUPPORT)
#define RW_VLD_MP4_DCAC_NOT_CODED_LOOP_SZ   0x2FC
#endif


// HHKuo's
#define RW_VLD_DCACSA             0x300
#define RW_VLD_DCACPITCH        0x304
#define RW_VLD_MVSA                 0x308
#define RW_VLD_DIRE_MD            0x30C
#define RW_VLD_BMB1                 0x310
#define RW_VLD_BMB2                 0x314
#define RW_VLD_BCODE_SA          0x318
#define RW_VLD_DIRE_MD_IL        0x31C
#define RW_VLD_DCMVSEL            0x320
   #define LITE_BREF_I_VOP          (0x1 << 18)
#define RW_VLD_DRAMMBSEL        0x324
#define RW_VLD_ADDREXTEND       0x328

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) //mtk40343 9/7/2010
#define RW_VLD_MVDRAM			0x36C
#define RW_VLD_MB1SA			0x370
#define RW_VLD_MB2SA			0x374
#define RW_VLD_COSA			0x378
#endif

#define RO_VLD_DCACBSY             0x380
#define RO_VLD_DCDRAM              0x384
#define RO_VLD_DRAMWR             0x394

#define RW_VLD_2ND_PROC              0x48C
#define RW_VLD_2ND_RPTR               0x4B0
#define RW_VLD_2ND_VSTART           0x4B4
#define RW_VLD_2ND_VEND              0x4B8
#define RW_VLD_2ND_RDY_SWTICH   0x4C0
#define RO_VLD_2ND_FETCHOK         0x4E8
#define RO_VLD_2ND_VBAR               0x4EC
#define RO_VLD_2ND_VRPTR              0x4FC

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) //mtk40343 9/7/2010
#define RW_VLD_PIC_SIZE        0x870
#define RW_VLD_MB_SIZE		 0x868
#endif

#define  RW_VLD_RESYNC_MARK      0x254

#if (MPEG4_6589_SUPPORT)
//6589NEW 3.1
#define RW_PP_DIVX3_DFT_PRED     0x704

//NEW6589 4.1 - 4.12
#define RW_AMV_MP4_MV_DIRECT_SA             0x20C
#define RW_AMV_MP4_COL_IVOP                 0x218
#define RW_AMV_MP4_TOP_FIELD_QUART_SAMPLE   0x2D8
#define RW_AMV_MP4_GMC_Y_MV                 0x2DC
#define RW_AMV_MP4_VTRBD                    0x2E0
#define RW_AMV_MP4_VTRBD_I                  0x2E4
#define RW_AMV_MP4_VTRBD_I_PLUS_1           0x2E8
#define RW_AMV_MP4_VTRBD_I_MINUS_1          0x2EC
#define RW_AMV_MP4_VTRBD_RATIO              0x2F0
#define RW_AMV_MP4_VTRBD_I_RATIO            0x2F4
#define RW_AMV_MP4_VTRBD_I_RATIO_2          0x2F8
#define RW_AMV_MP4_VTRBD_I_RATIO_3          0x2FC
#endif

/* JPEG decoding */
#define WO_VLD_RDY                  0x114
// *********************************************************************
// H.264 related VLD Registers define
// *********************************************************************
// KB's scaling list
#define RW_VLD_SCL_ADDR      0x260 
#define RW_VLD_SCL_DATA      0x264

// ****************************
// WMV related VLD registers define
// ****************************
#define RW_VLD_SRAM_TEST_WRITE_ADDR  0xCC
#define RW_VLD_SRAM_TEST_WRITE_DATA  0xD0
#define RW_VLD_BITCOUNT             0x114
#define RW_VLD_WMVDEC              0x208
    #define WMV_DEC_START           (0x1)
    #define WMV_BP_DEC_START      (0x1 << 8)
#define RW_VLD_WMV_MODE          0x20C
#define RW_VLD_WMV_PARA1         0x210
#define RW_VLD_WMV_PARA2         0x214
#define RW_VLD_WMV_OVC            0x218
#define RW_VLD_WMV_LOAD_SUM   0x21C
#define RW_VLD_WMV_PARA3         0x220
#define RW_VLD_WMV_BP               0x224
#define RW_VLD_MODE                   0x228
#define RW_VLD_WMV_QM              0x230
#define RO_VLD_BP_DEC_END         0x234
    #define BD_DEC_FINISH             (0x1 << 8)
#define RW_VLD_WMV_TAB             0x238
#define RW_VLD_WMV_ABS             0x23C
#define RO_VLD_PIC_DEC_END        0x240
    #define PIC_DEC_FINISH             (0x1)
#define RW_VLD_WMV_PUD             0x244
#define RW_VLD_WMV_PBN             0x248
#define RO_VLD_WMV_ERRFG          0x250
#define RW_VLD_WMV_SLICE_IDX0    0x274
#define RW_VLD_WMV_SLICE_IDX1    0x278
#define RW_VLD_WMV_SLICE_IDX2    0x27C
#define RO_VLD_WMV_ERRCNT           0x284
#define RO_VLD_WMV_ERRTYP           0x288

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
#define RW_VLD_WMV_NEW_CTRL                  0x28C
       #define WMV_NEW_FLG_EN                                   (1 << 0)
       #define WMV_PRED_MOD_EN                                 (1 << 1)
       #define WMV_PRED_RD_SYNC_DISABLE                (1 << 3)
       #define WMV_DRAM_BURST_MODE_EN                  (1 << 4)
       #define WMV_BP_USE_PRED_RD_EN                      (1 << 5)
 
#define RW_VLD_WMV_DCAC_NEW_ADDR       0x290
#define RW_VLD_WMV_BP0_NEW_ADDR         0x29C
#define RW_VLD_WMV_BP1_NEW_ADDR         0x2A0
#define RW_VLD_WMV_BP2_NEW_ADDR         0x2A4
#define RW_VLD_WMV_OW_PRED                    0x2A8
#define RW_VLD_WMV_OW_BP                         0x2AC
       #define WMV_NUM_OW_BP_0                                0
       #define WMV_NUM_OW_BP_1                                8
       #define WMV_NUM_OW_BP_2                               16
#endif

#define RW_VLD_BP1_ADDR            0x318 // the same as RW_VLD_BCODE_SA
#define RW_VLD_DCMVSEL              0x320
#define RW_VLD_MBDRAM_SEL        0x324
#define RW_VLD_BP2_ADDR            0x32C
#define RW_VLD_BP3_ADDR            0x330
#define RW_VLD_BP4_ADDR            0x334
#define RW_VLD_MVVOP_SEL          0x338
#define RW_VLD_MVF1                   0x33C
#define RW_VLD_MVF2                   0x340
#define RW_VLD_MVF3                   0x344
#define RW_VLD_MVF4                   0x348
#define RW_VLD_MV2_ADDR           0x34C
#define RW_VLD_VOP_TYPE            0x350
#define RW_VLD_DCAC2_ADDR       0x354
#define RW_VLD_MV3_ADDR           0x358

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
#define RW_TOPVLD_WMV_PICSIZE_MB          0x68
#define RW_TOPVLD_WMV_PICSIZE                 0x70
#define RW_TOPVLD_WMV_DCAC_NEW_ADDR       0x28
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
#define RW_VLD_PIC_MB_SIZE_M1      0x68
#define VLD_TOP_PIC_HEIGHT_IN_MBS_POS 16
#endif


#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
// *********************************************************************
// WMV NEW MV Registers define
// *********************************************************************
#define RW_WMV_MV_NEW_ADDR             0x20C
#define RW_WMV_BWDREF_PIC_TYPE     0x218        
#define RW_WMV_MV_DIRECT_SCL_FAC   0x21C
       #define WMV_IRAT_0                                        0
       #define WMV_IRAT_1                                        16       
#define RW_WMV_MV_CFG                         0x224
       #define WMV_FIRST_FIELD                               28
       #define WMV_BWDREF_DIST                             24
       #define WMV_FWDREF_DIST                             20
       #define WMV_USE_OPP_REF_FIELD                  16
       #define WMV_TWO_REF_PIC                              12
       #define WMV_POST_RESCENT                             9
       #define WMV_FAST_UV                                       8
       #define WMV_MV_RNG_IDX                                 4
       #define WMV_QPIXEL                                          0

// *********************************************************************
// DCAC Registers define
// *********************************************************************
#define  RW_WMV_DCAC_RULEB         0x0
#define  RW_WMV_DCAC_PRED           0x4
#endif

// *********************************************************************
// MC Registers define
// *********************************************************************
#define RW_MC_R1Y           0x00
#define RW_MC_R1C           0x04
#define RW_MC_R2Y           0x08
#define RW_MC_R2C           0x0C
#define RW_MC_BY            0x10
#define RW_MC_BC            0x14
#define RW_MC_FWDP        0x18
#define RW_MC_DECFLD     0x1C

#define RW_MC_2FLDMD     0x20
#define RW_MC_OPBUF       0x24
#define RO_MC_MBX           0x28
#define RO_MC_MBY           0x2C
#define RO_MC_BMBX         0x30
#define RO_MC_BMBY         0x34
#define RO_MC_MOTION	  0x38
#define RO_MC_MVVALID	  0x3C

#define RO_MC_MV1X		 0x40
#define RO_MC_MV1Y		 0x44
#define RO_MC_MV2X		 0x48
#define RO_MC_MV2Y		 0x4C
#define RO_MC_MV3X		 0x50
#define RO_MC_MV3Y		 0x54
#define RO_MC_MV4X		 0x58
#define RO_MC_MV4Y		 0x5C

#define RO_MC_REFFLD      0x60
#define RO_MC_MBINTRA    0x64
#define RO_MC_FRMDCT     0x68
#define RO_MC_CBP           0x6C
#define RW_MC_HREFP       0x70
#define RW_MC_WMBDIG    0x74
#define RW_MC_DIGMBX     0x78
#define RW_MC_DIGMBY     0x7C

#define RW_MC_DIGY        0x80
#define RW_MC_DIGC        0x84
#define RW_MC_DIGWD      0x88
#define RW_MC_YDCS        0x8C

#define RW_MC_ADDRSWAP   0x90
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    #define ADDRSWAP_OFF            (0x2 << 1)
    #define ADDRSWAP_DDR3           (0x2 << 0)
#else
    #define ADDRSWAP_OFF            (0x0 << 0)
    #define ADDRSWAP_MODE_1      (0x1 << 0)
    #define ADDRSWAP_MODE_2      (0x2 << 0)
#endif 
#define RO_MC_STATUS         0x94
#define RW_MC_DISSOLVE     0x98
#define RW_MC_BY1              0x9C   //first B pic buffer ptr Y
#define RW_MC_BC1              0xA0   //first B pic C
#define RW_MC_BY2              0xA4   //second B pic buffer ptr Y
#define RW_MC_BC2              0xA8   //second B pic C

#define RW_MC_PS_CMB11         0xAC //cmb1 intra 1xy 2xy
#define RW_MC_PS_CMB12         0xB0 //cmb1 intra 3xy 4xy
#define RW_MC_PS_CMB13         0xB4 //cmb1 intra 5xy 6xy
#define RW_MC_PS_CMB14         0xB8 //cmb1 intra 7xy 8xy
#define RW_MC_PS_CMB15         0xBC //cmb1 intra 9xy 10xy
#define RW_MC_PS_CMB16         0xC0 //cmb1 MVxy , IDCTxy
#define RW_MC_PS_CMB21         0xC4 //cmb2 intra 1xy 2xy
#define RW_MC_PS_CMB22         0xC8 //cmb2 intra 3xy 4xy
#define RW_MC_PS_CMB23         0xCC //cmb2 intra 5xy 6xy
#define RW_MC_PS_CMB24         0xD0 //cmb2 intra 7xy 8xy
#define RW_MC_PS_CMB25         0xD4 //cmb2 intra 9xy 10xy
#define RW_MC_PS_CMB26         0xD8 //cmb2 MVxy , IDCTxy
#define RW_MC_PS_CMB_INTRA_THD 0xDC //CMB intra dc threshold
#define RW_MC_PS_CMB_MV_MAX     0xE0 //CMB maximun MV
    #define  PS_CMB_MV1_VALID        ((unsigned)(0x1) << 30)
    #define  PS_CMB_MV2_VALID        ((unsigned)(0x1) << 29)
#define RW_MC_PS_MP_BOUND         0xE4//motion plane IDCT diff curve upper bound
#define RW_MC_PS_MP_DIFF             0xE8//motion plane IDCT diffrence curve level
#define RW_MC_PS_MP_DIFF1            0xEC//motion plane IDCT diffrence curve level
#define RW_MC_PS_MP_IDCT_CNT      0xF0//motion plane IDCT count threshold
#define RW_MC_PS_CMB_IDCT_MAX   0xF4//CMB maximun IDCT abs sum
    #define  PS_CMB_IDCT1_VALID      ((unsigned)(0x1) << 18)
    #define  PS_CMB_IDCT2_VALID      ((unsigned)(0x1) << 17)
#define RW_MC_PS_CMB_CTRL           0xF8//CMB control register
    #define  PS_CMB1_CLR                  ((unsigned)(0x1) << 7)//clear cmb group1
    #define  PS_CMB2_CLR                  ((unsigned)(0x1) << 6)//clear cmb group2
    #define  PS_CMB_SEL2                  ((unsigned)(0x1) << 5)// 1:select 2 ,0:select cmb group 1
    #define  PS_SRC_DETECT                ((unsigned)(0x1) << 4)//film detect on
    #define  PS_MAD_ON                     ((unsigned)(0x1) << 3)//motion plane on
    #define  PS_CMB_INTRA                 ((unsigned)(0x1) << 2)//cmb select intra on
    #define  PS_CMB_IDCT                   ((unsigned)(0x1) << 1)//cmb select idct on
    #define  PS_CMB_MV                     ((unsigned)(0x1) << 0)//cmb select mv on

#define RW_MC_PS_MP_READ       0xFC//motion plane read ptr
#define RW_MC_PS_MP_WRITE     0x100//motion plane write ptr
#define RW_MC_PS_DINTRA1       0x104//Dinra 1-12
#define RW_MC_PS_DINTRA2       0x108
#define RW_MC_PS_DINTRA3       0x10C
#define RW_MC_PS_DINTRA4       0x110
#define RW_MC_PS_DINTRA5       0x114
#define RW_MC_PS_DINTRA6       0x118
#define RW_MC_PS_DINTRA7       0x11C
#define RW_MC_PS_DINTRA8       0x120
#define RW_MC_PS_DINTRA9       0x124
#define RW_MC_PS_DINTRA10      0x128
#define RW_MC_PS_DINTRA11      0x12C
#define RW_MC_PS_DINTRA12      0x130
#define RW_MC_PS_DINTER1        0x134//Dinter 1-12
#define RW_MC_PS_DINTER2        0x138
#define RW_MC_PS_DINTER3        0x13C
#define RW_MC_PS_DINTER4        0x140
#define RW_MC_PS_DINTER5        0x144
#define RW_MC_PS_DINTER6        0x148
#define RW_MC_PS_DINTER7        0x14C
#define RW_MC_PS_DINTER8        0x150
#define RW_MC_PS_DINTER9        0x154
#define RW_MC_PS_DINTER10      0x158
#define RW_MC_PS_DINTER11      0x15C
#define RW_MC_PS_DINTER12      0x160
#define RW_MC_PS_DT1           0x164//Dt 1-12
#define RW_MC_PS_DT2           0x168
#define RW_MC_PS_DT3           0x16C
#define RW_MC_PS_DT4           0x170
#define RW_MC_PS_DT5           0x174
#define RW_MC_PS_DT6           0x178
#define RW_MC_PS_DT7           0x17C
#define RW_MC_PS_DT8           0x180
#define RW_MC_PS_DT9           0x184
#define RW_MC_PS_DT10          0x188
#define RW_MC_PS_DT11          0x18C
#define RW_MC_PS_DT12          0x190
#define RW_MC_PS_DB1           0x194//Db 1-12
#define RW_MC_PS_DB2           0x198
#define RW_MC_PS_DB3           0x19C
#define RW_MC_PS_DB4           0x1A0
#define RW_MC_PS_DB5           0x1A4
#define RW_MC_PS_DB6           0x1A8
#define RW_MC_PS_DB7           0x1AC
#define RW_MC_PS_DB8           0x1B0
#define RW_MC_PS_DB9           0x1B4
#define RW_MC_PS_DB10          0x1B8
#define RW_MC_PS_DB11          0x1BC
#define RW_MC_PS_DB12          0x1C0
#define RW_MC_PS_DIFF_CTRL     0x1C4//D calculation control
    #define  PS_BUF_ID                 ((unsigned)(0x1) << 27)//current pic buffer id
    #define  PS_LAST_BUF_ID        ((unsigned)(0x1) << 25)//last pic buffer id
    #define  PS_MP_CNT_CLR         ((unsigned)(0x1) << 3)//clear motion table count
    #define  PS_DINTERA_ON         ((unsigned)(0x1) << 2)//enable Dinter, Dintra
    #define  PS_DTB_ON                ((unsigned)(0x1) << 1)//enable Dt, Db
    #define  PS_D_START               ((unsigned)(0x1) << 0)//start to calculate D values
#define RW_MC_PS_DRAM_MODE   0x1C8//read/write Dram address mode
#define RW_MC_QPEL_TYPE          0x1FC
   #define TYPE_14496               0
   #define TYPE_MOMUSYS          1
#define RW_MC_QUARTER_SAMPLE   0x200
#define RW_MC_ROUNDING_CTRL      0x204
#define RW_MC_UMV_PIC_WIDTH     0x208
#define RW_MC_UMV_PIC_HEIGHT    0x20C
#define RW_MC_WARP_POINT_NUM   0x210
#define RW_MC_GMC_Y_MV              0x214
#define RW_MC_GMC_C_MV              0x218
#define RW_MC_CBCR_MV_TYPE       0x21C
   #define QPEL_4MV_CMV_14496         (0x0)
   #define QPEL_4MV_CMV_DIVX503      (0x1)
   #define QPEL_FIELD_CMV_14496       (0x0 << 8)
   #define QPEL_FIELD_CMV_DIVX503    (0x1 << 8)
   #define QPEL_FRAME_CMV_14496      (0x0 << 16)
   #define QPEL_FRAME_CMV_DIVX503  (0x1 << 16)
   #define QPEL_FRAME_CMV_DIVXOLD  (0x2 << 16)

// post-processing registers
#define RW_MC_PP_ENABLE        0x220
#define RW_MC_PP_Y_ADDR        0x224
#define RW_MC_PP_C_ADDR        0x228
#define RW_MC_PP_MB_WIDTH    0x22C
#define RW_MC_BREF                  0x230
   #define NML_BREF_I_VOP         (0x1 << 5)
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
   #define RW_MC_PP_QP_TYPE       0x700
#else
   #define RW_MC_PP_QP_TYPE       0x234
#endif
   #define SEL_GIVEN_QP            (0x0 << 8)
   #define SEL_USER_QP              (0x1 << 8)
   #define SEL_LUT_QP                (0x2 << 8)
#define RW_MC_PP_DBLK_MODE  0x238
   #define DBLK_14496                (0x0 << 16)
   #define DBLK_DIVX                  (0x1 << 16)
   #define DBLK_V_THEN_H          (0x0 << 8)
   #define DBLK_H_THEN_V          (0x1 << 8)
   #define DBLK_Y                        (0x1 << 1)
   #define DBLK_C                        (0x1)
#define RW_MC_PP_DBLK_THD      0x23C
#define RW_MC_PP_DBLK_DIR      0x240
   #define DBLK_H_ONLY              (0x1 << 8)
   #define DBLK_V_ONLY              (0x1)
#define RW_MC_PP_DRING_MODE    0x244
   #define COMB_WITH_DBLK           (0x1 << 16)
   #define MAX_DIFF_MOMUSYS       (0x0 << 8)
   #define MAX_DIFF_14496            (0x1 << 8)
   #define DRING_Y                         (0x1 << 1)
   #define DRING_C                         (0x1)
#define RW_MC_PP_DRING_THD      0x248
#define RW_MC_PP_INTERLACE       0x24C
   #define INTERLACE_DRING           (0x1 << 8)
   #define INTERLACE_DBLK             (0x1)
#define RW_MC_PP_WB_BY_POST    0x250
#define RW_MC_PP_LOW_BW           0x254
#define RW_MC_PP_SEQ                  0x258
   #define DBLK_THEN_DRING           (0x0)
   #define DRING_THEN_DBLK           (0x1)
#define RW_MC_PP_X_RANGE          0x260
#define RW_MC_PP_Y_RANGE          0x264
#define RW_MC_PP_MODE               0x268
   #define WMV_MODE                     (0x0)
   #define H264_MODE                     (0x1)

#define RW_MC_WMV9_PRE_PROC    0x270
    #define PP_NO_SCALE                  (0x0)
    #define PP_SCALE_DOWN             (0x1)
    #define PP_SCALE_UP                  (0x1 << 1)
#define RW_MC_WMV9_ICOMP_EN    0x274
    #define ICOMP_C_OFF                 (0x0)
    #define ICOMP_C_EN                   (0x1)
    #define ICOMP_Y_OFF                 (0x0 << 1)
    #define ICOMP_Y_EN                   (0x1 << 1)
#define RW_MC_ISCALE1_X1            0x278
#define RW_MC_ISCALE1_X3            0x27C
#define RW_MC_ISCALE1_X5            0x280
#define RW_MC_ISCALE1_X7            0x284
#define RW_MC_ISCALE1_X9            0x288
#define RW_MC_ISCALE1_X11           0x28C
#define RW_MC_ISCALE1_X13           0x290
#define RW_MC_ISCALE1_X15           0x294
#define RW_MC_YSHIFT_OFF1           0x298
#define RW_MC_CSHIFT_OFF1           0x29C
#define RW_MC_FILTER_TYPE           0x2A0
    #define C_BILINEAR                     (0x0)
    #define C_BICUBIC                      (0x1)
    #define Y_BILINEAR                     (0x0 << 1)
    #define Y_BICUBIC                      (0x1 << 1)
#define RW_MC_WRITE_BUS_TYPE    0x2A4
    #define CLIP_0_255_TYPE            0
    #define UNCLIP_TYPE                   1
#define RW_MC_INTRA_BLK_ADD128 0x2a8
    #define ADD128_OFF                    0
    #define ADD128_ON                      1
#define RW_MC_FAST_UVMC             0x2B4
    #define FAST_UVMC_EN                (0x1)
#define RW_MC_OVL_SMTH_FILTER   0x2BC
    #define OVL_OFF                         (0x0)
    #define OVL_EN                           (0x1)
#define RW_MC_COND_OVL_FILTER   0x2C0
    #define COND_OVL_OFF               (0x0)
    #define COND_OVL_EN                 (0x1)
#define RW_MC_PP_DBLK_OPT          0x2C4
    #define CHK_INTRA0                     (0x1)
    #define CHK_BLK3                        (0x1 << 8)
    #define DBLK_FLG_4x4                 (0x1 << 16)
    #define VC1_ALL_SET_ONE            (CHK_INTRA0 + CHK_BLK3 + DBLK_FLG_4x4)
    #define NOT_VC1_ALL_SET_ZERO   (0)
#define RW_MC_PP_DBLK_Y_ADDR     0x2C8
#define RW_MC_PP_DBLK_C_ADDR     0x2CC
#define RW_MC_WMV8_MIX_PEL        0x2D0
    #define MIX_PEL_MC_EN               (0x0)
#define RW_MC_RNG_PARA                0x2E4 //This register is not used in current hardware any more
#define RW_MC_ISCALE2_X1             0x2EC
#define RW_MC_ISCALE2_X3             0x2F0
#define RW_MC_ISCALE2_X5             0x2F4
#define RW_MC_ISCALE2_X7             0x2F8
#define RW_MC_ISCALE2_X9             0x2FC
#define RW_MC_ISCALE2_X11           0x300
#define RW_MC_ISCALE2_X13           0x304
#define RW_MC_ISCALE2_X15           0x308
#define RW_MC_YSHIFT_OFF2           0x30C
#define RW_MC_CSHIFT_OFF2           0x310
#define RW_MC_ICOMP_TYPE            0x314
    #define NO_ICOMP                 0
    #define FRAME_ICOMP           1
    #define TOP_FLD_ICOMP        2
    #define BTM_FLD_ICOMP        3
    #define BOTH_FLD_ICOMP      4
#define RW_MC_ISCALE3_X1       0x318
#define RW_MC_ISCALE3_X3       0x31C
#define RW_MC_ISCALE3_X5       0x320
#define RW_MC_ISCALE3_X7       0x324
#define RW_MC_ISCALE3_X9       0x328
#define RW_MC_ISCALE3_X11      0x32C
#define RW_MC_ISCALE3_X13      0x330
#define RW_MC_ISCALE3_X15      0x334
#define RW_MC_YSHIFT_OFF3      0x338
#define RW_MC_CSHIFT_OFF3      0x33C
#define RW_MC_ICOMP2_EN         0x340
#define RW_MC_ISCALE4_X1        0x344
#define RW_MC_ISCALE4_X3        0x348
#define RW_MC_ISCALE4_X5        0x34C
#define RW_MC_ISCALE4_X7        0x350
#define RW_MC_ISCALE4_X9        0x354
#define RW_MC_ISCALE4_X11      0x358
#define RW_MC_ISCALE4_X13      0x35C
#define RW_MC_ISCALE4_X15      0x360
#define RW_MC_YSHIFT_OFF4      0x364
#define RW_MC_CSHIFT_OFF4      0x368
#define RW_MC_ISCALE5_X1        0x36C
#define RW_MC_ISCALE5_X3        0x370
#define RW_MC_ISCALE5_X5        0x374
#define RW_MC_ISCALE5_X7        0x378
#define RW_MC_ISCALE5_X9        0x37C
#define RW_MC_ISCALE5_X11      0x380
#define RW_MC_ISCALE5_X13      0x384
#define RW_MC_ISCALE5_X15      0x388
#define RW_MC_YSHIFT_OFF5      0x38C
#define RW_MC_CSHIFT_OFF5      0x390
#define RW_MC_BWD_ICOMP_FLD    0x394
    #define BWD_TOP_FLD           (0x0)
    #define BWD_BTM_FLD          (0x1)
#define RW_MC_SAME_ICOMP       0x398
    #define NO_USE_SAME_ICOPM1_FOR_FRAME    (0x0)
    #define USE_SAME_ICOMP1_FOR_FRAME          (0x1)
#define RW_MC_REF_PIC_TYPE     0x39C
  #define BWD_PROG_REF_PIC     (0x0)
  #define BWD_INTLCE_REF_PIC   (0x1)
  #define FWD_PROG_REF_PIC     (0x0 << 16)
  #define FWD_INTLCE_REF_PIC   (0x1 << 16)
#define RW_MC_ISCALE6_X1       0x3A0
#define RW_MC_ISCALE6_X3       0x3A4
#define RW_MC_ISCALE6_X5       0x3A8
#define RW_MC_ISCALE6_X7       0x3AC
#define RW_MC_ISCALE6_X9       0x3B0
#define RW_MC_ISCALE6_X11     0x3B4
#define RW_MC_ISCALE6_X13     0x3B8
#define RW_MC_ISCALE6_X15     0x3BC
#define RW_MC_YSHIFT_OFF6     0x3C0
#define RW_MC_CSHIFT_OFF6     0x3C4
#define RW_MC_SAME_REF_PIC   0x3CC
#define RW_MC_P_LIST0             0x3DC // ~0x458
#define RW_MC_B_LIST0             0x45C // ~0x4d8
#define RW_MC_B_LIST1             0x4DC // ~0x558
#define RW_AMC_CBCR_OFFSET  0x55C
#define RW_AMC_Y_OUT_ADDR   0x560
#define RW_AMC_P_LIST0_FLD    0x564
#define RW_AMC_B_LIST0_FLD    0x568
#define RW_AMC_B_LIST1_FLD    0x56C
#define RW_MC_WRAPPER_SWITCH   0x57C
#define RW_MC_IRATIO                 0x690    //RM Codec
#define RO_MC_DRAM_PEAK         0x770

#define RO_MC_DRAM_CYCLE		0x778
#define RW_MC_PARA_BEHAVIOR    0x998

#if (CONFIG_DRV_FPGA_BOARD)
#define RW_MC_MODE_CTL  0x5E4
    #define MC_MV_MERGE_OFF    (0x0 << 10)
    #define MC_DRAM_REQ_MERGE_OFF    (0x0 << 12)
    #define MC_DRAM_REQ_DELAY_1T    (0x0 << 16)
    #define MC_QIU_BANK8    (0x1 << 17)
    #define MC_QIU_BANK4    (0x0 << 18)
#endif    

//PANDA
#define RW_MC_VLD_WRAPPER_ADDR  0x93C
    #define MC_VLD_WRAPPER_READ     (0x0ul<<16)
    #define MC_VLD_WRAPPER_WRITE    (0x1ul<<16)
#define RW_MC_VLD_WRAPPER_DATA  0x940

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
   #define RW_MC_PIC_W_MB        0x980
#define RW_MC_MVDCAC_SEL	   0x80C
#endif
   #define RW_MC_VLD_WRAPPER        0xA20
   #define RW_MC_PP_WRAPPER        0xA24

#define RW_MC_PREFETCH			0xA2C

// *********************************************************************
// NBM MC Registers define
// *********************************************************************
#define RW_MC_NBM_CTRL         0x768
    #define RW_MC_NBM_OFF      (0x1 << 8)

// *********************************************************************
// DDR3 Switch MC Registers define
// *********************************************************************
#define RW_MC_DDR_CTRL0         0x7FC
#define RW_MC_DDR_CTRL1         0x800
#define RW_MC_DDR3_EN             0x834


// *********************************************************************
// AVC VLD Registers define
// *********************************************************************
// Barral Shifter from 0x00 - 0x80
#define RO_AVLD_BARL      0x00
#define RW_AVLD_CTRL      0x84
   #define AVC_EN              (0x1)
   #define AVC_DEC_CYCLE_EN          (0x1<<13)
   #define AVC_ERR_BYPASS          ((unsigned)(0x3) << 14)       
   #define AVC_NON_SPEC_SWITCH          ((unsigned)(0x1) << 15) 
   #define AVC_ERR_CONCEALMENT          ((unsigned)(0x2) << 22)       
 #ifdef MEM_PAUSE_SUPPORT
   #define AVLD_MEM_PAUSE_MOD_EN (0x1<<27)
 #else 
   #define AVLD_MEM_PAUSE_MOD_EN (0x0<<27)
 #endif 
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
   #define AVC_RDY_WITH_CNT                 ((unsigned)(0x3) << 1)  //in search start code, it should be set as 3
#else  
   #define AVC_RDY_WITH_CNT                 ((unsigned)(0x0) << 1)
#endif   
   #define AVC_RDY_CNT_THD      (0x0 << 3) 
   #define AVC_RBSP_CHK_INV             ((unsigned)(0x0) << 28)
   #define AVC_SUM6_APPEND_INV       ((unsigned)(0x1) << 29)
 #ifdef MEM_PAUSE_SUPPORT
   #define AVC_READ_FLAG_CHK_INV    ((unsigned)(0x0) << 30)
   #define AVC_NOT_CHK_DATA_VALID  ((unsigned)(0x1) << 31)
 #else
   #define AVC_READ_FLAG_CHK_INV    ((unsigned)(0x1) << 30)
   #define AVC_NOT_CHK_DATA_VALID  ((unsigned)(0x0) << 31)
 #endif
#define RW_AVLD_SPS              0x88

#define RW_AVLD_PIC_SIZE      0x8C
#define AVLD_PIC_HEIGHT_IN_MBS_POS 8
#define FW_FST_MB_IN_SLICE_MASK	0x1FFFF

#define RW_AVLD_PPS_1           0x90
#define RW_AVLD_PPS_2           0x94
#define RW_AVLD_SHDR_1         0x98
#define RW_AVLD_SHDR_2         0x9C
#define RW_AVLD_MAX_PIC_NUM     0xA0
#define RW_AVLD_CUR_PIC_NUM      0xA4
#define RW_AVLD_REORD_P0_RPL     0xA8
#define RW_AVLD_REORD_B0_RPL     0x128
#define RW_AVLD_REORD_B1_RPL     0x1A8
#define RW_AVLD_RESET_PIC_NUM    0x228
   #define RESET_PIC_NUM 0x1
#define RW_AVLD_RPL_REORD                 0x22C
#define RW_AVLD_WEIGHT_PRED_TBL      0x230
#define RW_AVLD_INIT_CTX_SRAM           0x234
#define RW_AVLD_PROC                          0x238 
#define RO_AVLD_UE                               0x23C 
#define RO_AVLD_SE                               0x240         // 2's complement
#define RW_AVLD_PRED_ADDR                 0x244
#define RO_AVLD_STATUS                        0x25c
   #define RO_AVLD_STALL                       0x2000000
#define RO_AVLD_TIMEOUT_THD              0x268
   #define AVLD_TIMEOUT_THD                 0xffffffff 
#define RO_AVLD_ERR_MESSAGE              0x270
#define RO_AVLD_COMPLETE                   0x274
#define RO_AVLD_STATE_INFO                 0x278
     #define AVLD_PIC_FINISH                ((unsigned)(0x1) << 31)
#define RW_AVLD_2ND_BARL_CTRL          0x27C
     #define AVLD_2ND_BARL_EN 0x3 
#define RW_AVLD_ERR_MASK                   0x280
     #define AVLD_MB_END_CHK                ((unsigned)(0x1) << 14)
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
     #define AVLD_4BLOCKS_SKIP_CHK      ((unsigned)(0x1) << 21)
#endif
#define RO_AVLD_ERR_ACCUMULATOR      0x284
    #define NO_NEXT_START_CODE          ((unsigned)(0x1) << 3)
    #define CABAC_ALIGN_BIT_ERR          ((unsigned)(0x1) << 8)
    #define CABAC_ZERO_WORD_ERR       ((unsigned)(0x1) << 14)
#define RO_AVLD_DEC_CYCLE                 0x288
#define RW_AVLD_RESET_SUM                 0x28C
     #define AVLD_RESET_SUM_OFF   0x0
     #define AVLD_RESET_SUM_ON     0x1
#define RW_AVLD_MC_BUSY_THRESHOLD  0x290

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#define RW_AVLD_RM03R                         0x2C8
     #define RO_ALVD_FIND_03                  ((unsigned)(0x1) << 11)
     #define VIEW_REORDER_SWITCH           ((unsigned)(0x1) << 13)
     #define MVC_SWITCH                           ((unsigned)(0x1) << 14)
     #define HEADER_EXT_SWITCH                           ((unsigned)(0x1) << 16)
     #define REORDER_MVC_SWITCH           ((unsigned)(0x1) << 17)
#define RW_AVLD_FSSR                            0x2D8
     #define FW_SEARCH_START_CODE       ((unsigned)(0x1) << 0)
#endif

#if VDEC_MVC_SUPPORT
#define RW_AVLD_MVC_REFS_L0              0x2F0
#define RW_AVLD_MVC_REFS_L1              0x370
#define RW_AVLD_MAX_VIEW_IDX_L0              0x3F0
#define RW_AVLD_MAX_VIEW_IDX_L1              0x3F4
#endif

#define RO_AVLD_2ND_BARL                    0x800
#define RO_AVLD_2ND_UE                        0x888
#define RO_AVLD_2ND_SE                        0x88c         // 2's complement
#define RW_AVLD_2ND_CTRL                    0x884

// *********************************************************************
// AVC MV Registers define
// *********************************************************************
#define RW_AMV_P_REF_PARA        0             // ~0x7C
#define RW_AMV_B0_REF_PARA      0x80      // ~0xFC
#define RW_AMV_B1_REF_PARA      0x100    // ~0x17C
#define RW_AMV_B1_REF_ADDR      0x180    // ~0x1FC
#define RW_AMV_CURR_POC           0x200
#define RW_AMV_CURR_TFLD_POC  0x204
#define RW_AMV_CURR_BFLD_POC  0x208
#define RW_AMV_WR_ADDR            0x20C
#if VDEC_MVC_SUPPORT
#define RW_AMV_MVC_CTRL            0x244
    #define FW_NO_MV_MVC       ((unsigned)(0x1) << 4)
#endif
#define RW_AMV_REDUCE_BMV      0x220
      #define EN_AMV_REDUCE_BMV      (0x1 << 5)
      #define EN_AMV_ALLEG_MVC_CFG (0x1 << 6)
// *********************************************************************
// AVC FilmGrain Registers define
// *********************************************************************
#define RW_FGT_MODE                    0x00
    #define FGT_EN  0x1 << 0
    #define FGT_SCR_PP  0x1 << 1
    #define FGT_VDSCL_BUSY_EN 0x1<<4
#define RW_FGT_SEED_ADDR           0x04
#define RW_FGT_SEI_ADDR_A          0x08
#define RW_FGT_SEI_ADDR_B          0x0C
#define RW_FGT_DATABASE_ADDR   0x10
#define RW_FGT_OUT_Y_ADDR         0x14
#define RW_FGT_OUT_C_ADDR         0x18
#define RW_FGT_IN_Y_ADDR           0x1C
#define RW_FGT_IN_C_ADDR           0x20
#define RW_FGT_MB_SIZE               0x24
#define RW_FGT_DRAM_CTRL           0x28
#define RW_FGT_SEI_CTRL_A          0x2c
    #define FGT_CANCLE_FLG_A  0x1 << 0
#define RW_FGT_SEI_CTRL_B          0x30
#define RW_FGT_CTRL_STATE          0x34

// *********************************************************************
// RM VLD Registers define
// *********************************************************************
#define RW_RMVLD_CTRL        0x84
  #define CFG_RM_CDECODER 0x1 << 0

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
#define RW_RM_TOPVLD_PICSIZE1   0x70    //PIC_WIDTH & PIC_HEIGHT
#define RW_RM_TOPVLD_PICSIZE2   0x68   //PIC_WIDTH in mb & PIC_HEIGHT in mb
#define RW_RM_TOPVLD_PICSIZE3   0x6C  //Total mbs in Pic
#define RW_RM_TOPVLD_TIMEOUT    0x50
#define RW_RM_TOPVLD_TIMEOUT_INT_SWITCH    0x54
#define RW_RM_TOPVLD_PREDADDR      0x28
#define RW_RM_TOPVLD_PREDADDR_MODE    0x2C
#endif

#define RW_RMVLD_PICSIZE1   0x88

#define RW_RMVLD_PICSIZE2   0x8C


#define RW_RMVLD_PICHDRPARA1    0x90
#define RW_RMVLD_PICHDRPARA2    0x94

#define RW_RMVLD_TIMEOUT        0x98
#define RW_RMVLD_PREDADDR      0x9C


#define RW_RMVLD_STARTDEC       0xA8

#define RW_RMVLD_ERRTYPE              0xC0
#define RW_RMVLD_ERRTYPEACCUM   0xC4

#define RW_RMVLD_VLDOK              0xCC

#define RW_RMVLD_PICFINISH       0xD0
#define RW_RMVLD_SLICENUM        0xF4


#define RW_RMVLD_BCNTRST    0x108
  #define CFG_RM_RST0 0x0 << 0
  #define CFG_RM_RST1 0x1 << 0


#define RW_RMVLD_SLICEBCNT    0x110


#define RW_RMVLD_CTRL2            0x324



// *********************************************************************
// RM MV Registers define
// *********************************************************************
#define RW_RMMV_CPLOCPICTYPE    0x218
#define RW_RMMV_IRATIO0              0x21C





// *********************************************************************
// RM PP Registers define
// *********************************************************************
#define RW_RMPP_PARAM                0x8
#define RW_RMPP_PQ                       0x1C


// *********************************************************************
// Letter Box Registers define
// *********************************************************************
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
#define RW_VEC_LBOX_UP_CFG                  0x88
#define RW_VEC_LBOX_LOW_CFG              0x8C
#define RW_VEC_LBOX_SIDE_CFG              0x90
#define RW_VEC_LBOX_THD_OFFSET       0x94
#define RW_VEC_LBOX_CNT_THD               0x98
#define RO_VEC_LBOX_Y_LINE                     0x9C
#define RO_VEC_LBOX_C_LINE                     0xA0
#endif


// *********************************************************************
// VDSCL Registers define
// *********************************************************************
#define RW_VDSCL_ACTIVATE                0x00
    #define RW_VDSCL_EN                      (1<<0)
    #define RW_VDSCL_CONVERT             (1<<2)
    #define RW_VDSCL_SW_RESET           (3<<4)
#define RW_VDSCL_TYPE                       0x04
    #define RW_VDSCL_PIC_STRU            (3<<0)
    #define RW_VDSCL_SRC_MC              (0<<2)
    #define RW_VDSCL_SRC_PP               (2<<2)
    #define RW_VDSCL_SRC_FG               (4<<2)
    #define RW_VDSCL_SPEC_MPEG         (0<<5)
    #define RW_VDSCL_SPEC_WMV          (1<<5)
    #define RW_VDSCL_SPEC_264            (2<<5)
    #define RW_VDSCL_Y_ONLY               (1<<7)
    #define RW_VDSCL_RASTER_SCAN      (1<<16)
#define RW_VDSCL_MEM_IF                   0x08
    #define RW_VDSCL_ADDR_SWAP_MODE1        (2<<0)
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    #define RW_VDSCL_ADDR_SWAP_MODE2        (7<<0)
#else    
    #define RW_VDSCL_ADDR_SWAP_MODE2        (3<<0)	
#endif
#define RW_VDSCL_BOUND                    0x0C
    #define RW_VDSCL_BOUND_PAD_CR   (0xFF<<0)
    #define RW_VDSCL_BOUND_PAD_CB   (0xFF<<8)
    #define RW_VDSCL_BOUND_PAD_Y     (0xFF<<16)
    #define RW_VDSCL_BOUND_PAD_EN   (1<<24)
    #define RW_VDSCL_BOUND_NO_PAD   (1<<25)
#define RW_VDSCL_SRC_SIZE                 0x10
    #define RW_VDSCL_SRC_WIDTH         (0xFFF<<16)
    #define RW_VDSCL_SRC_HEIGHT        (0xFFF<<0)
#define RW_VDSCL_SRC_OFFSET            0x14  
#define RW_VDSCL_TG_BUF_LEN            0x18
    #define RW_VDSCL_TRG_BUF_LEN      (0xFF<<0)
#define RW_VDSCL_TG_SIZE                  0x1C
    #define RW_VDSCL_TG_WIDTH           (0xFFF<<16)
    #define RW_VDSCL_TG_HEIGHT          (0xFFF<<0)
#define RW_VDSCL_TG_OFFSET              0x20
#define RW_VDSCL_TG_Y_OW_BASE       0x24
    #define RW_VDSCL_TG_Y_OW_ADDR_BASE  (0xFFFFFFF<<0)
#define RW_VDSCL_TG_C_OW_BASE                0x28
    #define RW_VDSCL_TG_C_OW_ADDR_BASE  (0xFFFFFFF<<0)
#define RW_VDSCL_H_SCL_Y                     0x2C
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530) 
        #define RW_VDSCL_H_SCL_Y_FACTOR   ((UINT32)(0xFFFF)<<0)
        #define RW_VDSCL_H_SCL_Y_OFFSET   ((UINT32)(0xFFFF)<<16)
    #else
        #define RW_VDSCL_H_SCL_Y_FACTOR   (0xFFF<<0)
        #define RW_VDSCL_H_SCL_Y_OFFSET   (0xFFF<<16)
    #endif
#define RW_VDSCL_H_SCL_C                     0x30
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530) 
        #define RW_VDSCL_H_SCL_C_FACTOR   ((UINT32)(0xFFFF)<<0)
        #define RW_VDSCL_H_SCL_C_OFFSET   ((UINT32)(0xFFFF)<<16)
    #else
        #define RW_VDSCL_H_SCL_C_FACTOR   (0xFFF<<0)
        #define RW_VDSCL_H_SCL_C_OFFSET   (0xFFF<<16)
    #endif
#define RW_VDSCL_V_SCL_Y                    0x34
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530) 
        #define RW_VDSCL_V_SCL_Y_FACTOR   ((UINT32)(0xFFFF)<<0)
        #define RW_VDSCL_V_SCL_Y_OFFSET   ((UINT32)(0xFFFF)<<16)
    #else
        #define RW_VDSCL_V_SCL_Y_FACTOR   (0xFFF<<0)
        #define RW_VDSCL_V_SCL_Y_OFFSET   (0xFFF<<16)
    #endif
#define RW_VDSCL_V_SCL_C                    0x38
     #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530) 
        #define RW_VDSCL_V_SCL_C_FACTOR   ((UINT32)(0xFFFF)<<0)
        #define RW_VDSCL_V_SCL_C_OFFSET    ((UINT32)(0xFFFF)<<16)   
     #else
        #define RW_VDSCL_V_SCL_C_FACTOR   (0xFFF<<0)
        #define RW_VDSCL_V_SCL_C_OFFSET    (0xFFF<<16)
     #endif
#define RW_VDSCL_TMP_OW_BASE            0x3C
    #define RW_VDSCL_TMP_OW_ADDR_BASE   (0xFFFFFFF<<0)
#define RW_VDSCL_TFSCLOFT                         0x70
#define RW_VDSCL_LUMA_KEY                                0x74
    #define RW_VDSCL_LK_EN                           (0x1<<0)
#define RO_VDSCL_DONE                                0x7C
    #define RO_VDSCL_DONE_FRAME                (0x1<<0)


// *********************************************************************
// VP6 VLD Registers define
// *********************************************************************
//Picture Layer Parameter
// Barral Shifter from 0x00 - 0x80
//Temp workaround for HW
#define RO_VP6_VLD_BARL            0x00
#define RW_VP6_INIT_BOOL          0x84
    #define RW_VP6_BOOL_EN            (1<<0)
#define RW_VP6_BCPR                   0x88
#define RW_VP6_PCI_PAR             0x8C
#define RW_VP6_FLAG                      (1<<0)

//PANDA
#ifdef VP6_MULTI_STREAM
#define RW_VP6_VLD_ETC              0x158
#else
#define RW_VP6_VLD_ETC              0xDC
#endif
    #define ETC_COEFF_ERROR         (0x1<<0)
    #define ETC_MC_TIMEOUT          (0x1<<1)
    #define ETC_MV_TIMEOUT          (0x1<<2)
    #define ETC_DCAC_TIMEOUT        (0x1<<3)
    #define ETC_HUFF_ERR            (0x1<<4)
#define RW_PIC_MB_SIZE_M1           0x68
#define RW_PIC_PIX_SIZE             0x70
#define RW_VP6_KEY_FRM                (1<<1)
#define RW_VP6_RTN_ERR		         (1<<2)
#define RW_VP6_EERE                       (1<<3)
#define RW_VP6_DAEC                      (1<<4)
#define RW_VP6_HUFFMAN              (1<<5)
#define RW_VP6_MULTI                    (1<<6)
//Get Bits Function
#define RW_VP6_GBFR                   0x94
#define RW_VLD_VP6DEC               0xA0
#define RO_VP6_PIC_DEC_END      0xA8
    #define VP6_DEC_START                (1<<0)
#define RW_VP6_VLD_WARR          0xC8
    //MULTI-STREAM PANDA
    #define VP6_CTX_WARR_READ       (0x0ul<<8)
    #define VP6_CTX_WARR_WRITE      (0x1ul<<8)
    #define VP6_CTX_WARR_BOOL       (0x0ul<<12)
    #define VP6_CTX_WARR_HUFFMAN    (0x1ul<<12)
#define RW_VP6_VLD_FCVR1          0xCC
#define RW_VP6_VLD_FCVR2          0xD0
#define RW_VP6_VLD_FCVR3          0xD4
#define RW_VP6_VLD_FCVR4          0xD8
//MULTI-STREAM PANDA
#ifdef VP6_MULTI_STREAM
#define RW_VP6_VLD_WWFR              0x158
#else
#define RW_VP6_VLD_WWFR          0xDC
#endif
    #define WWFR_COEFF_ERROR         (0x1<<0)
    #define WWFR_MC_TIMEOUT          (0x1<<1)
    #define WWFR_MV_TIMEOUT          (0x1<<2)
    #define WWFR_DCAC_TIMEOUT        (0x1<<3)
    #define WWFR_HUFF_ERR            (0x1<<4)

#define RO_VP6_VLD_ERR               0xF4

//MULTI-STREAM PANDA
#define RW_VP6_VLD_PROB_UPD         0x140

#define RW_VP6_PAD_ZERO_MODE        0x14C
#define RW_VP6_PIC_BYTE_CNT            0x150
#define RW_VP6_PAD_ZERO_MODE2      0x164
#define RW_VP6_PIC_BYTE_CNT2          0x168


#define VLD_WRAPPER_MP_TYPE_PROB    840
#define VLD_WRAPPER_UPD_MP_TYPE 936
#define VLD_WRAPPER_DC_Y        945
#define VLD_WRAPPER_DC_C        946
#define VLD_WRAPPER_RUN_0       947
#define VLD_WRAPPER_RUN_1       948
#define VLD_WRAPPER_AC_BASE     949 //949~984
#define VLD_WRAPPER_AC_BASE_END 984

#define CTX_BOOL_DC_Y           0
#define CTX_BOOL_DC_C           1
#define CTX_BOOL_RUN_0          2
#define CTX_BOOL_RUN_1          3
#define CTX_BOOL_AC_BASE        4   //4~39

#define CTX_HUFF_AC_BASE        0   //0~23
#define CTX_HUFF_DC_Y           24
#define CTX_HUFF_DC_C           25
#define CTX_HUFF_RUN_0          26
#define CTX_HUFF_RUN_1          27
#define CTX_HUFF_EOB_RUN        28
//~MULTI-STREAM PANDA

#define RO_VP6_VLD_DDR3_Finish  0x24C
   #define  VP6_DDR3_FINISH             (1<<30)

//#define RW_VP6_QUANT_REG         0x200
#define RW_VP6_QUANT_REG         0x500  //20110728
#define RW_VP6_BKDOOR                0x85C
    #define    VP6_ADOBE_EN           (1<<12)
    #define    VP6_2FLIMIT_ZERO     (1<<8)
    #define    VP6_DBK_START          (1<<4)
    #define    VP6_CBCR_MAKE_RND  (1<<0)
#define RW_VP6_LOOPDBK              0x860
#define RW_VP6_MV_TH                  0x864
#define RW_VP6_AUTOSEL              0x868
#define RW_VP6_COEF0_P1            0x86C
#define RW_VP6_COEF0_P2            0x870
#define RW_VP6_COEF1_P1            0x874
#define RW_VP6_COEF1_P2            0x878
#define RW_VP6_COEF2_P1            0x87C
#define RW_VP6_COEF2_P2            0x880
#define RW_VP6_COEF3_P1            0x884
#define RW_VP6_COEF3_P2            0x888

//VP6 MC
#define RW_VP6_MC_RY                  0x0
#define RW_VP6_MC_RC                  0x4
#define RW_VP6_MC_GY                  0x8
#define RW_VP6_MC_GC                  0xC
#define RW_VP6_MC_DY                  0x9C
#define RW_VP6_MC_DC                  0xA0
//~VP6 MC

//VP6 PP
#define RW_VP6_MC_PP_QP_TYPE  0x234
//~VP6 PP

// *********************************************************************
// VP8 VLD Registers define
// *********************************************************************
//Picture Layer Parameter
#define RO_VP8_VBSR1 	0x00        // 0--32x4  Barrel shift read
#define RO_VP8_VRLR 	0x84     // 33--40x4 read literal
#define RO_VP8_VBSR2 	0x200   // 128--160x4 second barrel shift read
#define RW_VP8_CTRL   	0xA4   // 41x4 control register
   #define RW_VP8_VP8FLAG 	0
   #define RO_VP8_SUM1_S 	8
   #define RO_VP8_SUM1_E 	13
   #define RO_VP8_SUM2_S 	16
   #define RO_VP8_SUM2_E 	21

#define RW_VP8_HEADER2 	0xA8  // 42x4     
   #define RW_VP8_PICTYPE_FLAG 	0
   #define RW_VP8_VERSION_S 	1
   #define RW_VP8_VERSION_E 	3
   #define RW_VP8_FSTPB_S 		8
   #define RW_VP8_FSTPB_E 		26

#define RW_VP8_HEADER3 	0xAC  // 43x4
   #define RW_VP8_CLTYPE_FLAG 	0
   #define RW_VP8_SEGEN_FLAG 	4
   #define RW_VP8_UMBSM_FLAG 	8
   #define RW_VP8_MTP_S 			12
   #define RW_VP8_MTP_E 			13
   #define RW_VP8_MBSKIP_FLAG 	16

#define RW_VP8_PICPROB 		0xB0  // 44x4
   #define RW_VP8_PSKIPF_S 	0
   #define RW_VP8_PSKIPF_E 	7
   #define RW_VP8_PINTRA_S 	8
   #define RW_VP8_PINTRA_E 	15
   #define RW_VP8_PLAST_S 	16
   #define RW_VP8_PLAST_E 	23
   #define RW_VP8_PGF_S 		24
   #define RW_VP8_PGF_E 		31

#define RW_VP8_FWPROB 		0xB4 // 45x4 0--7 bit the probability of firmware read_lit(n)
#define RW_VP8_HDR  		0xB8  // 46x4 0 bit triger start decode

#define RW_VP8_COEFR0		0xC0  //48X4
#define RW_VP8_COEFR1		0xC4
#define RW_VP8_COEFR2		0xC8
#define RW_VP8_COEFR3		0xCC
#define RW_VP8_COEFR4		0xD0
#define RW_VP8_COEFR5		0xD4
#define RW_VP8_COEFR6		0xD8
#define RW_VP8_COEFR7		0xDC

#define RW_VP8_COEFPB0 	0xE0  // 56 X4
#define RW_VP8_COEFPB1 	0xE4
#define RW_VP8_COEFPB2 	0xE8
#define RW_VP8_COEFPB3 	0xEC
#define RW_VP8_COEFPB4 	0xF0
#define RW_VP8_COEFPB5 	0xF4
#define RW_VP8_COEFPB6 	0xF8
#define RW_VP8_COEFPB7 	0xFC

#define RW_VP8_BSASET		0x100 // 64X4   0--8 bit   Bus_SRAM address setting
#define RW_VP8_BSDSET 		0x104 //65x4    0--31bit  Bus_SRAM write data setting
#define RW_VP8_CSASET 		0x108 // 66X4  0--7bit  CTX_SRAM address setting
#define RW_VP8_CSDSET 		0x10C // 67x4  0--31bit CTX_SRAM write data setting

#define RO_VP8_BINIT1  		0x110 //68x4 0--31bit bool0 init
#define RO_VP8_BINIT2  		0x114 //69x4 0--31bit bool1 init

#define RW_VP8_MVPROB 		0x120  //72x4
    #define RW_VP8_PMVISX_S	0
    #define RW_VP8_PMVISX_E	7
    #define RW_VP8_PMVISY_S	8
    #define RW_VP8_PMVISY_E	15
    #define RW_VP8_PMVSX_S	16
    #define RW_VP8_PMVSX_E	23
    #define RW_VP8_PMVSY_S	24
    #define RW_VP8_PMVSY_E	31

#define RW_VP8_STR 			0x140  // 80x4
    #define RW_VP8_STR1_FLAG 0
    #define RW_VP8_STR2_FLAG 8

#define RW_VP8_BCRT 		0x144 // 81x4 
    #define RW_VP8_BCRT1_FLAG 0
    #define RW_VP8_BCRT2_FLAG 8
    
#define RW_VP8_CPUT 		0x148   //82x4  0bit trigger coef_prob_update
#define RW_VP8_MVPUT 		0x14C  //83x4 0bit trigger mv_prob_update 
#define RO_VP8_PICFSM 		0x150 //84x4 0--18bit  pic state
#define RO_VP8_MBFSM 		0x154  //85x4 0--22 mb_state
#define RO_VP8_COEFFSM 	0x158 //86x4 0--2bit coeff_dec_state

#define RO_VP8_ERTR 		0x15C  //87x4 0--31 error type register
#define RO_VP8_ERRXY 		0x160  //88x4  
    #define RO_VP8_ERRY_S 	0
    #define RO_VP8_ERRY_E 	8
    #define RO_VP8_ERRX_S 	16
    #define RO_VP8_ERRX_E 	24

#define RO_VP8_RECKSM 		0x164 //89x4 0--31bite residual check sum
#define RO_VP8_LTHR1 		0x168  //90x4 0--31 barrel_shifter_latch2
#define RO_VP8_LTHR2 		0x16C  //91x4 0--31 second barrel_shifter_latch2

#define RO_VP8_VOKR 		0x170 //92x4 0bit vld ok :1 means that vld decode whole picture
#define RO_VP8_PFR  			0x174 // 93x4 0bit  picture finish: 1 means that vdec decode whole picture

#define RO_VP8_STATUS 		0x178 // 94X4

#define RO_VP8_XY 			0x17C //95x4
    #define RO_VP8_CURRY_S 	0
    #define RO_VP8_CURRY_E 	8
    #define RO_VP8_CURRX_S 	16
    #define RO_VP8_CURRX_E 	24
#define RO_VP8_HWA 			0x180 // 96X4 0bit hw is in process

// MV Register
#define RW_VP8_MV_RFSB 	0x2D4  // 181x4

//MC Register
#define RW_MC_PIC1Y_ADD 	0x3E0  // 248x4
#define RW_MC_PIC2Y_ADD 	0x3E4
#define RW_MC_PIC3Y_ADD 	0x3E8
#define RW_MC_LUMA_SIZE 	0x934  //589x4
#define RW_MC_VP8SETTING 	0x97C //607x4
    #define RW_MC_VP8FULLPEL_FLAG 8
    #define RW_MC_BILINEAR_OR_SIXTAP_FLAG 4
    #define RW_MC_SUPERBITSCAN_OFF_FLAG 0
#define RW_MC_VP8RANDOMERRSET 	0x9F8 //638x4
    #define RW_MC_VP8RANDOM_ERR 1

//PP Register
#define RW_PP_VP8PARA  		0x20 // 8x4
    #define RW_PP_VP8ENABLE_FLAG 0
    #define RW_PP_FLTTYPE_FLAG 4  // 0:normal 1: simple
    #define RW_PP_MODEREF_DELTA_ENABLE_FLAG 8
    #define RW_PP_FRAMETYPE_FLAG 12 // 0:Key 1:Inter
    #define RW_PP_SHARPLVL_S 16
    #define RW_PP_SHARPLVL_E 18
#define RW_PP_VP8FILTER_LEVEL 0x24 // 9X4
    #define RW_PP_FILTER_LV0_S 0
    #define RW_PP_FILTER_LV0_E 5
    #define RW_PP_FILTER_LV1_S 8
    #define RW_PP_FILTER_LV1_E 13
    #define RW_PP_FILTER_LV2_S 16
    #define RW_PP_FILTER_LV2_E 21
    #define RW_PP_FILTER_LV3_S 24
    #define RW_PP_FILTER_LV3_E 29
#define RW_PP_VP8REFDELTA 0x28 // 10x4
#define RW_PP_VP8MODEDELTA 0x2C // 11x4
    #define RW_PP_DELTA_ID0_S 0
    #define RW_PP_DELTA_ID0_E 6
    #define RW_PP_DELTA_ID1_S 8
    #define RW_PP_DELTA_ID1_E 14
    #define RW_PP_DELTA_ID2_S 16
    #define RW_PP_DELTA_ID2_E 22
    #define RW_PP_DELTA_ID3_S 24
    #define RW_PP_DELTA_ID3_E 30
#define RW_PP_SPECIAL_MODE 0x30 // 12x4
#define RW_PP_REG_17  0x44  // 17x4
#define RW_PP_BRC_OFFSET 0x70 // 28x4

// *********************************************************************
// AVS relate Registers definitions
// *********************************************************************
#define RW_AVS_VLD_COMPLETE_CLR 0xac

#define RW_AVS_VLD_HW_TIMEOUT 0xB4
#ifdef CC_AVS_EMULATION
#define AVS_HW_TIMEOUT 0x66684B
#else
#define AVS_HW_TIMEOUT 0xCCD097
#endif

#define RO_AVS_VLD_COMPLETE        0x120    
#define RO_AVS_VLD_PIC_FINISH     0x124

#define RW_AVS_VLD_QUANT_TABLE_POS      0x260 
#define RW_AVS_VLD_QUANT_TABLE              0x264

#define RW_AVS_VLD_CTRL 0x84
#define RW_AVS_VLD_PIC_SIZE 0x88
#define RW_AVS_VLD_PHP 0x8C
#define RW_AVS_VLD_ERR_CONCEALMENT 0x90
    #define AVS_ERR_CONCEALMENT_ENABLE 0x201
    #define AVS_ERR_CONCEALMENT_DISABLE 0
    
#define RW_AVS_VLD_PRED_BASE_ADDR 0x98

#define RW_AVS_VLD_DECSTART 0x9C
#define RW_AVS_VLD_CLR_BYTE_COUNT 0xC0
#define RW_VLD_MCPICSZ    0xC8

#define RW_AVS_VLD_BITSTUFFING_SWITCH 0xC4
#define RW_AVS_VLD_MODE_SWITCH 0xF4

#define RW_AVS_VLD_BYTE_COUNT 0x138

#define RW_MC_UMV_PIC_WIDTH     0x208
#define RW_MC_UMV_PIC_HEIGHT    0x20C

#define RW_MC_DEC_Y               0x9C   // output decoder buffer for y
#define RW_MC_DEC_C               0xA0   // output decoder buffer for cbcr

#define RW_AVS_MV_ADDR 0x20C

#define RW_AVS_MC_PP_ENABLE 0x220

#define RW_AVS_MC_DISABLE_WRITE 0x250

#define RW_AVS_MC_P_LIST0_PIC0_Y_ADDR 0x3DC
#define RW_AVS_MC_P_LIST0_PIC1_Y_ADDR 0x3E0
#define RW_AVS_MC_P_LIST0_PIC2_Y_ADDR 0x3E4
#define RW_AVS_MC_P_LIST0_PIC3_Y_ADDR 0x3E8

#define RW_AVS_MC_B_LIST0_PIC0_Y_ADDR 0x45C
#define RW_AVS_MC_B_LIST0_PIC1_Y_ADDR 0x460
#define RW_AVS_MC_B_LIST1_PIC0_Y_ADDR 0x4DC
#define RW_AVS_MC_B_LIST1_PIC1_Y_ADDR 0x4E0

#define RW_AVS_MC_P_FIELD_DESCRIPTION 0x564

#define RW_AVS_MC_B_REF 0x930

#define RW_AVS_MV_BW_I_REF 0x218

#define RW_AVS_BLK_FW_DISTANCE_1_0 0x2A8
#define RW_AVS_BLK_FW_DISTANCE_3_2 0x2AC
#define RW_AVS_BLK_BW_DISTANCE_1_0 0x2B0
#define RW_AVS_BLK_FW_MED_RATIO_1_0 0x2B4
#define RW_AVS_MV_FW_MED_RATIO_3_2 0x2B8
#define RW_AVS_MV_BW_MED_RATIO_1_0 0x2BC
#define RW_AVS_MV_DIRECT_RATIO_1_0 0x2C0
#define RW_AVS_MV_DIRECT_RATIO_3_2 0x2C4

#define RW_AVS_MC_LUMA_SIZE 0x934

#define RW_AVS_PP_AVS_MODE 0x0
#define RW_AVS_PP_LUMA_ADDR 0x224
#define RW_AVS_PP_CHROM_ADDR 0x228

#define RO_AVS_VLD_UE    0xC8 
#define RO_AVS_VLD_SE     0xCC         // 2's complement

#define RW_AVS_FIND_STARTCODE   0xD0
#define RW_AVS_FIND_NEXT_STARTCODE 0xD4

#define RO_AVS_ERROR_TYPE 0x114 
#define RO_AVS_ERROR_ACC 0x118 
#define RO_AVS_TIMEOUT_CNT 0x11C 
#define RO_AVLD_SMKR            0x250


// *********************************************************************
// Video Constant define
// *********************************************************************
// Referenc Buf / B Buf / Digest Buf / Post Processing Buf Index
#define MPV_REF_BUF_0       0
#define MPV_REF_BUF_1       1
#define MPV_B_BUF_0         2
#define MPV_B_BUF_1         3

#define MC_DIG_BUF          2

// Picture Field control for MC
#define MC_TOP_FLD          0
#define MC_BOTTOM_FLD       1

#define MC_2ND_FLD_OFF      0
#define MC_2ND_FLD_ON       1


typedef enum {
    PIC_STRUCT_FRAME            = 0,
    PIC_STRUCT_TOP                = 1,
    PIC_STRUCT_BOTTOM          = 2,
    PIC_STRUCT_TOP_BOTTOM   = 3
} EPicStruct;


typedef enum {
    PAD_MODE_NO_PAD,
    PAD_MODE_EXTEND,
    PAD_MODE_COLOR
} EPadMode;

typedef enum {
    VID_COLOR_TRANSPARENT   =  0x000000,
    VID_COLOR_BLACK              =  0x808010,
    VID_COLOR_BLUE                =  0x72D423,
    VID_COLOR_RED                 =  0xD46441,
    VID_COLOR_MAGENTA         =  0xC6B854,
    VID_COLOR_GREEN             =  0x3A4870,
    VID_COLOR_CYAN              =  0x2C9C83,
    VID_COLOR_YELLOW          =  0x8E2CA2,
    VID_COLOR_GRAY              =  0x8080B4,
    VID_COLOR_WHITE            =  0x8080EB
} EVidColor;

#define IO_BASE VDEC_BASE ///TODO:

// *********************************************************************
// Video Decoder Macros
// *********************************************************************
#define vWriteReg(dAddr, dVal)  mt65xx_reg_sync_writel(dVal, IO_BASE + dAddr) //*(volatile UINT32 *)(IO_BASE + dAddr) = dVal
#define u4ReadReg(dAddr)        *(volatile UINT32 *)(IO_BASE + dAddr)
#define vWriteGconReg(dAddr, dVal)  mt65xx_reg_sync_writel(dVal, VDEC_GCON_BASE + dAddr)
#define u4ReadGconReg(dAddr)  *(volatile UINT32 *)(VDEC_GCON_BASE + dAddr)
#define vWriteGenReg(dAddr, dVal)  mt65xx_reg_sync_writel(dVal, dAddr) //*(volatile UINT32 *)(IO_BASE + dAddr) = dVal
#define u4ReadGenReg(dAddr)        *(volatile UINT32 *)(dAddr)


// *********************************************************************
//  Common Video Decoder HW Functions
// *********************************************************************
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
extern void vVDecWriteTopVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadTopVLD(UINT32 u4VDecID, UINT32 u4Addr);
#endif
extern void vVDecWriteMISC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadMISC(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadVLD(UINT32 u4VDecID, UINT32 u4Addr);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
extern void vVDecWriteVLDTOP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadVLDTOP(UINT32 u4VDecID, UINT32 u4Addr);
#endif
extern void vVDecWriteMC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadMC(UINT32 u4VDecID, UINT32 u4Addr);
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
extern void vVDecResetHW(UINT32 u4VDecID);
#else
extern void vVDecResetHW(UINT32 u4VDecID, UINT32 u4VDecType);
#endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
extern void vVDecWriteDV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadDV(UINT32 u4VDecID, UINT32 u4Addr);
#endif
extern void vVDecSetVLDVFIFO(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFifoSa, UINT32 u4VFifoEa);
extern UINT32 u4VDecReadVldRPtrEx(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa, const char *szFunction, INT32 i4Line);
extern BOOL fgVDecIsVLDFetchOk(UINT32 u4BSID, UINT32 u4VDecID);
extern BOOL fgVDecWaitVldFetchOk(UINT32 u4BSID, UINT32 u4VDecID);
extern UINT32 u4VDecVLDGetBitS(UINT32 u4BSID, UINT32 u4VDecID, UINT32 dShiftBit);

#define u4VDecReadVldRPtr(u4BSID, u4VDecID, pu4Bits, u4VFIFOSa)  u4VDecReadVldRPtrEx(u4BSID, u4VDecID, pu4Bits, u4VFIFOSa, __FUNCTION__, __LINE__)


#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
extern void vVLDLoadQuanMat(UINT32 u4VDecID, BOOL fgIntra);
#endif

extern UINT32 u4VDec03Number(UINT32 u4Addr, UINT32 u4Range);
extern void vMCSetOutputBuf(UINT32 u4VDecID, UINT32 u4OutBufIdx, UINT32 u4FRefBufIdx);
extern UINT32 u4ReadVDSCL(UINT32 u4VDecID, UINT32 u4Addr);
#ifdef VDEC_SIM_DUMP
extern void vVDecSimDump(UINT32 u4VDecID, UINT32 u4OffsetAddr, UINT32 u4Addr, UINT32 u4Val);
extern void vVDecSimWDump(const char *szFunction, UINT32 u4VDecID, const char *szOffset, UINT32 u4Addr, UINT32 u4Val, UINT32 u4BaseReg);
extern void vVDecSimRDump(const char *szFunction, UINT32 u4VDecID, const char *szOffset, UINT32 u4Addr, UINT32 u4Val, UINT32 u4BaseReg);

#define vVDecSimDumpW(u4VDecID, szOffset, u4Addr, u4Val) vVDecSimWDump(__FUNCTION__, u4VDecID, #szOffset, u4Addr, u4Val, szOffset)
#define vVDecSimDumpR(u4VDecID, szOffset, u4Addr, u4Val) vVDecSimRDump(__FUNCTION__, u4VDecID, #szOffset, u4Addr, u4Val, szOffset)

#else
#define vVDecSimDumpW(u4VDecID, szOffset, u4Addr, u4Val)
#define vVDecSimDumpR(u4VDecID, szOffset, u4Addr, u4Val)
#endif

extern void vVDecWriteCRC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadCRC(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecEnableCRC(UINT32 u4VDecID,BOOL fgEnable,BOOL fgCRCFromMC);

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
extern void vVDecPowerDownHW(UINT32 u4VDecID);
#endif
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) &&(!CONFIG_DRV_FPGA_BOARD))
extern void vVDecSetVldMcClk(UINT32 u4VDecID,UINT32 u4CodecType);
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
extern void vVDECSetLetetrBoxDetPrm(UINT32 u4VDecID, VDEC_INFO_LBD_PRM_T *prLBDPrm);
extern void vVDECReadLetetrBoxDetResult(UINT32 u4VDecID, UINT32 *u4YResult, UINT32 *u4CResult);
#endif

// *********************************************************************
//  Down Scaler HW Functions
// *********************************************************************
extern void vVDECSetDownScalerPrm(UINT32 u4VDecID, VDEC_INFO_VDSCL_PRM_T *prDownScalerPrm);

// BYTE 0 first       [BYTE0, BYTE1, BYTE2, BYTE3]
#define BYTE3(arg)          (*((BYTE *)&(arg) + 3))
#define BYTE2(arg)          (*((BYTE *)&(arg) + 2))
#define BYTE1(arg)          (*((BYTE *)&(arg) + 1))
#define BYTE0(arg)          (* (BYTE *)&(arg))
#define INVERSE_ENDIAN(value)    	\
		(((value & 0xFF) << 24) + ((value & 0xFF00) << 8) + ((value & 0xFF0000) >> 8) + ((value & 0xFF000000) >> 24))

#endif

