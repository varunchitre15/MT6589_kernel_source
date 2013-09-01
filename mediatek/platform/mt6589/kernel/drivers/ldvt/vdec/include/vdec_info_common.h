
#ifndef _VDEC_INFO_COMMON_H_
#define _VDEC_INFO_COMMON_H_

#include "type.h"
#include <mach/mt_typedefs.h>
#include "vdec_info_h264.h"
#include "vdec_info_wmv.h"
#include "vdec_info_mpeg.h"
#include "vdec_info_rm.h"
#include "vdec_info_vp6.h"
#include "vdec_info_vp8.h"
#include "vdec_info_avs.h"
#include <linux/kernel.h>
#include <asm/memory.h>

int UTIL_Printf( const char * format, ... );
int sprintf ( char * str, const char * format, ... );

#define CONFIG_DRV_VERIFY_SUPPORT 1
#define CONFIG_DRV_FPGA_BOARD 1
#if CONFIG_DRV_FPGA_BOARD
#define IRQ_DISABLE
#define CAPTURE_ESA_LOG
#ifdef CAPTURE_ESA_LOG
#define CAPTURE_ALL_IN_ONE
#define WRITE_ESA_PER_FRAME 0
#endif
#define VMMU_SUPPORT 0
#endif
#define CONFIG_DRV_LINUX 1
#define VDEC_MVC_SUPPORT 0
#define START_CODE 0x00000001

///*****Please Note : if you define the MEM_ALLOCATE_IOREMAP, please also modify the following file*****
///ALPS_SW\TRUNK\ALPS\alps\mediatek\config\mt8320fpga_ldvt\autoconfig\kconfig\project
///Change point : CONFIG_MAX_DRAM_SIZE_SUPPORT=0x10000000 ==> 0x08000000
#define MEM_ALLOCATE_IOREMAP 1
    #if MEM_ALLOCATE_IOREMAP

    #define PHYSICAL(addr)                                  m4u_v2p(addr)
//    #define VIRTUAL(addr)                                   __va(addr)

    #else
    #define PHYSICAL(addr)                                  __pa(addr)
    #define VIRTUAL(addr)                                   __va(addr)

    #endif

#define FILEBUF_SZ 1024*400
#define CRC_SZ 4*1024
#if MEM_ALLOCATE_IOREMAP
#if VMMU_SUPPORT
#define VMMU_SZ 0x400000 // need 64k align
#define VMMU_SA 0x05000000 //0x08000000 
#define FILELIST_SA (VMMU_SA+VMMU_SZ)
#else
#define FILELIST_SA 0x9A000000//0x05000000 //0x08000000
#endif
#define VFIFO_SA (FILELIST_SA+FILE_LIST_SZ)
#define GOLDY_SA (VFIFO_SA+V_FIFO_SZ)
#define GOLDC_SA (GOLDY_SA+GOLD_Y_SZ)
#define CRCBUF_SA (GOLDC_SA+GOLD_C_SZ)
#ifdef CAPTURE_ESA_LOG
#define ESALOG_SZ 1024*1024
#define ESALOGTOTAL_SZ 4*1024*1024
#define ESALOG_SA (CRCBUF_SA+CRC_SZ)
#define FILEBUF_SA (ESALOG_SA+ESALOG_SZ)
#else
#define FILEBUF_SA (CRCBUF_SA+CRC_SZ)
#endif
#define WORKBUF_SA (FILEBUF_SA+FILEBUF_SZ)


//#define WORKBUF_SA 0x0D800000
#define ARFBUF_SA (WORKBUF_SA+PIC_Y_SZ+PIC_C_SZ)
#if VDEC_VP8_WRAPPER_OFF
#define GLDBUF_SA (ARFBUF_SA+DEC_PP_Y_SZ+DEC_PP_C_SZ)
#define LSTBUF_SA (GLDBUF_SA+DEC_PP_Y_SZ+DEC_PP_C_SZ)
#define VLDWRAP_SA (LSTBUF_SA+DEC_PP_Y_SZ+DEC_PP_C_SZ)
#define PPWRAPY_SA (VLDWRAP_SA+VLD_PRED_SZ)
#define PPWRAPC_SA (PPWRAPY_SA+PP_WRAPY_SZ)
#define SEGIDWRAP_SA (PPWRAPC_SA+PP_WRAPC_SZ)

#define PPYBUF_SA (LSTBUF_SA+DEC_PP_Y_SZ+DEC_PP_C_SZ)
#else
#define GLDBUF_SA (ARFBUF_SA+PIC_Y_SZ+PIC_C_SZ)
#define LSTBUF_SA (GLDBUF_SA+PIC_Y_SZ+PIC_C_SZ)
#define PPYBUF_SA (LSTBUF_SA+PIC_Y_SZ+PIC_C_SZ)
#endif
#define PPCBUF_SA (PPYBUF_SA+DEC_PP_Y_SZ)
#define VLDWRAPWORK_SA (PPCBUF_SA+DEC_PP_C_SZ)
#define PPWRAPWORK_SA (VLDWRAPWORK_SA+VLD_WRAP_SZ)


//For AVS
#define AVS_PIC_Y0 0x0D800000   //(PPWRAPWORK_SA + V_WRAP_SA)
//#define AVS_PIC_C0 (AVS_PIC_Y0 + PIC_Y_SZ)
#define AVS_PIC_Y1 (AVS_PIC_Y0 + PIC_Y_SZ + PIC_C_SZ)
//#define AVS_PIC_C1 (AVS_PIC_Y1 + PIC_Y_SZ)
#define AVS_PIC_Y2 (AVS_PIC_Y1 + PIC_Y_SZ + PIC_C_SZ)
//#define AVS_PIC_C2 (AVS_PIC_Y2 + PIC_Y_SZ)
#define AVS_PIC_Y3 (AVS_PIC_Y2 + PIC_Y_SZ + PIC_C_SZ)
//#define AVS_PIC_C3 (AVS_PIC_Y3 + PIC_Y_SZ)
#define AVS_VLD_PRED_BUF (AVS_PIC_Y3 + PIC_Y_SZ + PIC_C_SZ)
#define AVS_VLD_MV1_BUF (AVS_VLD_PRED_BUF + AVS_VLD_PRED_SZ)
#define AVS_VLD_MV2_BUF (AVS_VLD_MV1_BUF + AVS_VLD_MV_SZ)
#define AVS_ADDSWAP_BUF (AVS_VLD_MV2_BUF + AVS_VLD_MV_SZ)
#define AVS_VLDWRAPWORK_SA (AVS_ADDSWAP_BUF + ADDSWAP_BUF_SZ)
#define AVS_PPWRAPWORK_SA (AVS_VLDWRAPWORK_SA + VLD_WRAP_SZ)
#endif


typedef struct _VDEC_PARAM_T_
{
  UINT32 u4InstanceId;
  UINT32 u4Mode;
  BOOL fgMVCType;
}VDEC_PARAM_T;

#define NO_PIC 0
#define TOP_FIELD 1
#define BOTTOM_FIELD 2
#define FRAME 3
#define TOP_BOTTOM_FIELD 4
#define CONFIG_CHIP_VER_CURR  80
#define CONFIG_CHIP_VER_MT8530 30
#define CONFIG_CHIP_VER_MT8550 50
#define CONFIG_CHIP_VER_MT8555 55
#define CONFIG_CHIP_VER_MT8560 60
#define CONFIG_CHIP_VER_MT8580 80

#define VDEC_8320_SUPPORT 1
#define VDEC_6589_SUPPORT 1
#define VDEC_6582_SUPPORT 0
#if VDEC_8320_SUPPORT
#define WMV_8320_SUPPORT  1
#define AVC_8320_SUPPORT  1
#else
#define WMV_8320_SUPPORT  0
#define AVC_8320_SUPPORT  0
#endif
#if VDEC_6589_SUPPORT
#define MPEG4_6589_SUPPORT  1
//#define MPEG4_6589_ERROR_CONCEAL
#else
#define MPEG4_6589_SUPPORT  0
#endif

#if VDEC_6582_SUPPORT
#define MPEG4_6582_SUPPORT  1
#else
#define MPEG4_6582_SUPPORT  0
#endif

#define MPEG4_6582_NORMAL_DEBUG_MODE 0

#define WMV_LOG_TMP 0
#define AVC_LOG_TMP 0

#define AVC_NEW_CRC_COMPARE  1

#define VDEC_VER_COMPARE_CRC 1

#define MT6582_L2_EMULATION 0 // 0: No L2$ emulation, 1: L2$ emulation with DRAM, 2: Use L2$ for VLD & PP Wrapper

//#define VDEC_SIM_DUMP
#ifdef VDEC_SIM_DUMP
#define REG_LOG_NEW
#endif

#define H263_USE_SORENSON 0

typedef enum
{
    VDEC_UNKNOWN = 0xFF,
    VDEC_MPEG       = 0x0,                 ///< MPEG Deocde Request
    VDEC_MPEG1      = 0x1,               ///< MPEG1 Deocde Request
    VDEC_MPEG2      = 0x2,               ///< MPEG2 Deocde Request
    VDEC_DIVX3       = 0x3,              ///< MPEG3 Deocde Request
    VDEC_MPEG4      = 0x4,              ///< MPEG4 Deocde Request
    VDEC_WMV         = 0x10,            /// < WMV Decode Request
    VDEC_WMV1       = 0x11,             ///< WMV7 Deocde Request
    VDEC_WMV2       = 0x12,            ///< WMV8 Deocde Request
    VDEC_WMV3       = 0x13,            ///< WMV9 Deocde Request
    VDEC_VC1          = 0x111,          ///< VC1 Deocde Request
    VDEC_H264        = 0x264,           ///< H264 Deocde Request
    VDEC_H263        = 0x263,           ///< H263 Deocde Request
    VDEC_RM          = 0x300,             ///< RM Decode Request
    VDEC_VP6         = 0x600,             ///< VP6 Decode Request
    VDEC_AVS         = 0x700,             ///< AVS Decode Request
    VDEC_VP8         = 0x800,             ///< VP8 Decode Request
    VDEC_JPG         = 0x900              ///< JPG Decode Request
}VDEC_CODEC_T;


/// Enumerate read pointer align type
typedef enum
{
    BYTE_ALIGN = 0,          ///< BYTE Align Request
    WORD_ALIGN,              ///< WORD Align Request
    DWRD_ALIGN,              ///< UINT32 Align Request
} RPTR_ALIGN_TYPE;


typedef struct _VDEC_INFO_VDSCL_PRM_T_
{
    BOOL     fgDSCLEn; // 1: 709 to 601
    BOOL     fgEnColorCvt; // 1: 709 to 601
    BOOL     fgYOnly;
    BOOL     fgMbaff;
    UCHAR   ucPicStruct;
    UCHAR   ucScrAgent;  // 0:MC 2:PP 3:FGT
    UCHAR   ucSpectType;
    UCHAR   ucScanType; //0: block 1:raster
    UCHAR   ucVdoFmt; // 0:420 1:422
    UCHAR   ucAddrSwapMode;
    UINT32  u4DispW;
    UINT32  u4SrcWidth;
    UINT32  u4SrcHeight;
    UINT32  u4SrcYOffH;
    UINT32  u4SrcYOffV;
    UINT32  u4SrcCOffH;
    UINT32  u4SrcCOffV;
    UINT32  u4TrgBufLen;
    UINT32  u4TrgWidth;
    UINT32  u4TrgHeight;
    UINT32  u4TrgOffH;
    UINT32  u4TrgOffV;
    UINT32   u4TrgYAddr;
    UINT32   u4TrgCAddr;
    UINT32   u4WorkAddr;
    UINT32   u4QY;
    UINT32   u4QC;
    UINT32   u4R_norm;
    UINT32   u4OffY;
    UINT32   u4OffC;

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    UINT32   u4SclYOffH;
    UINT32   u4SclCOffH;
    UINT32   u4SclYOffV;
    UINT32   u4SclCOffV;

    BOOL     fgLumaKeyEn;
    UINT16   u2LumaKeyValue;
#endif
}VDEC_INFO_VDSCL_PRM_T;


#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
typedef struct _VDEC_INFO_LBD_PRM_T_
{
    UINT32 u4UpBoundStart;
    UINT32 u4UpBoundEnd;
    UINT32 u4LowBoundStart;
    UINT32 u4LowBoundEnd;
    UINT32 u4LeftBound;
    UINT32 u4RightBound;
    UINT32 u4YOffset;
    UINT32 u4COffset;
    UINT32 u4YValueThd;
    UINT32 u4CValueThd;
    UINT32 u4YCntThd;
    UINT32 u4CCntThd;
}VDEC_INFO_LBD_PRM_T;
#endif


typedef struct _VDEC_INFO_DEC_PRM_T_
{
    UCHAR   ucPicStruct;
    UCHAR   ucPicType;
    UCHAR   ucDecFBufIdx;
    UCHAR   ucAddrSwapMode;
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)    //mtk40110 Qing.Li 2010/11/25, to fix mpeg4 DCAC Pred bug
    UCHAR   ucMpegSpecType;    //1:mpeg12, 2:mpeg4&h263, 3:divx 
#endif
    UINT32  u4PicBW;
    UINT32  u4PicW;
    UINT32  u4PicH;
    VDEC_INFO_VDSCL_PRM_T rDownScalerPrm;
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
    VDEC_INFO_LBD_PRM_T rLBDPrm;
#endif
    void *prVDecCodecHalPrm;
    #if(CONFIG_DRV_VERIFY_SUPPORT)
    union
    {
      VDEC_INFO_H264_DEC_PRM_T rVDecH264DecPrm;
      VDEC_INFO_WMV_DEC_PRM_T rVDecWMVDecPrm;
      VDEC_INFO_MPEG_DEC_PRM_T rVDecMPEGDecPrm;
      VDEC_INFO_RM_DEC_PRM_T rVDecRMDecPrm;
      VDEC_INFO_VP6_DEC_PRM_T rVDecVP6DecPrm;
      VDEC_INFO_AVS_DEC_PRM_T rVDecAVSDecPrm;
      VDEC_INFO_VP8_DEC_PRM_T rVDecVP8DecPrm;
    } SpecDecPrm;
    #endif

}VDEC_INFO_DEC_PRM_T;

#endif //#ifndef _HAL_VDEC_COMMON_IF_H_

