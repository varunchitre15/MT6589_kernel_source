
#ifndef _VDEC_INFO_VP6_H_
#define _VDEC_INFO_VP6_H_

//#include "drv_config.h"

#if(!CONFIG_DRV_VERIFY_SUPPORT)
#include "vdec_usage.h"
#include "vdec_info_common.h"
#else
#include "x_stl_lib.h"
//#include "x_assert.h"
//#include "u_pbinf.h"
#endif

//#define VP6_I_FRM    0
//#define VP6_P_FRM   1

typedef enum
{
    VP6_PROFILE_SIMPLE = 0,
    VP6_PROFILE_UNDEF_1,   
    VP6_PROFILE_UNDEF_2,
    VP6_PROFILE_ADVANCED
}VP6_PROFILE_T;

typedef struct _VDEC_INFO_VP6_VFIFO_PRM_T_
{
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_VP6_VFIFO_PRM_T;


typedef struct _VDEC_INFO_VP6_BS_INIT_PRM_T_
{
    UINT32 u4ReadPointer;              
    UINT32 u4WritePointer;
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_VP6_BS_INIT_PRM_T;


//MULTI-STREAM PANDA
/* HUFF_NODE for decoder */
typedef struct _HUFF_NODE
{
    UINT16 left; // 1 bit tells whether its a pointer or value
    UINT16 right;// 1 bit tells whether its a pointer or value
} HUFF_NODE;

typedef struct _HUFF_CODE
{
    UINT16 hcode;
    UINT16 len;
} HUFF_CODE;

typedef struct _SORTNODE
{
    INT32 next;
    INT32 freq;
    UINT16 value;
} SORTNODE;
//~MULTI-STREAM PANDA


typedef struct _VDEC_INFO_VP6_FRM_HDR_T_
{
    BOOL    fgFrmHdrValid;
    // frame coding type: I_TYPE, P_TYPE
    UCHAR  ucFrameType;
    // Quantizer setting
    UCHAR  ucDctQMask;
    
    // 0: for one partition, 1 for two partitions
    BOOL    fgMultiStream;
    //0 for BoolCoder, 1 for HuffmanCoder for 2nd data patition.
    BOOL    fgUseHuffman;
    //Version of encoder used to encode frame.
    UCHAR  ucVp3VerNo;
    // 0 for Simple, 3 for Advanced. (1 and 2 undefined)
    VP6_PROFILE_T  ucVpProfile;
    // Reserved.
    UCHAR    ucReserved;
    //Offset to 2nd partion
    UINT16  u2Buff2Offset;
    // Number of rows of 8x8 blocks in unscaled frame.
    UINT16  u2VFragments;
    // Number of cols of 8x8 blocks in unscaled frame.
    UINT16  u2HFragments;
    // Number of rows of 8x8 blocks in scaled frame.
    UINT16  u2OutVFragments;
    // Number of cols of 8x8 blocks in scaled frame.
    UINT16  u2OutHFragments;

    
    // Mode to use for scaling frame.
    UCHAR  ucScalingMode;
    // Advanced Profile Only:
    // 0 Prediction filter type is fixed and specified.
    // 1 Auto-select bi-cubic or bi-linear prediction filter.
    BOOL     fgAutoSelectPMFlag;
    //If Auto-SelectPMFlag == 1 only, threshold on prediction filter variance size.
    UINT32   u4PredictionFilterVarThresh;
    //If Auto-SelectPMFlag == 1 only, threshold on MV size.
    UINT32   u4PredictionFilterMvSizeThresh;
    // If Auto-SelectPMFlag == 0 only, threshold on MV size.
    // 0 use Bi-linear filter.
    // 1 use Bi-cubic filter.
    BOOL      fgBiCubicOrBiLinearFlag;
    // Vp3VerNo == 8 Only, Selector to choose bi-cubic filter coefficients
    UCHAR    ucPreditionFilterAlpha;

    // Inter Frame Header
    // 0: Do not update the Golden Frame with this frame, 1: Decoded frame should become new Golden Frame.
    BOOL      fgRefreshGoldenFrame;
    // Advanced Profile Only: 
    //0: Disable the loop filter, 1: Enable.
    UINT16      u2LoopFilter;
    // Advanced Profile Only:
    // 0: Basic loop filter, 1: De-ringing loop filter.
    BOOL      fgLoopFilterSelector;    

    BOOL      fgParse_Filter_Info;
    UCHAR    ucVrt_Shift;   
    UCHAR    ucFilter_Mode;
    UINT32    u4Sample_Variance_Threshold;
    UINT32    u4Max_Vector_Length;
    UINT32    u4Filter_Selection;

    //HW related
    UINT16    u2Vp56_Filter_Threshold;
    UINT32    u4DQuant_Dc;
    UINT32    u4DQuant_Ac;
    UINT16    u2WidthDec;
    UINT16    u2HeightDec;
    UINT32    u4Mv_Thr_En;
    UINT32    u4Var_Thr_En;
    UINT32    u4BilinearFilter;
    UINT32    u4FrameSize;
    //~HW related

    //Driver Flow Control
    UCHAR    ucPicStruct;
    UCHAR    ucColorPrimaries;
    UINT16    u2FrmRatCod;
    UCHAR    ucAspectRatio;
    //~Driver Flow
} VDEC_INFO_VP6_FRM_HDR_T;


typedef struct _VDEC_INFO_VP6_WORK_BUF_SA_T_
{
    //UINT32  u4Pic0YSa;
    //UINT32  u4Pic0CSa;
    //UINT32  u4Pic1YSa;
    //UINT32  u4Pic1CSa;
    //UINT32  u4Pic2YSa;
    //UINT32  u4Pic2CSa;
}VDEC_INFO_VP6_WORK_BUF_SA_T;

typedef struct _VDEC_INFO_VP6_FRAME_BUF_SA_T_
{
    UINT32  u4Pic0YSa;
    UINT32  u4Pic0CSa;
    UINT32  u4Pic1YSa;
    UINT32  u4Pic1CSa;
    UINT32  u4Pic2YSa;
    UINT32  u4Pic2CSa;

    UINT32  u4PpYBufSa;
    UINT32  u4PpCBufSa;
}VDEC_INFO_VP6_FRAME_BUF_SA_T;

typedef struct _VDEC_INFO_VP6_PP_INFO_T_
{
    BOOL     fgPpEnable;
    UINT8    u1PpLevel;
    UINT8    au1MBqp[4];
    UINT32  u4PpYBufSa;
    UINT32  u4PpCBufSa;
}VDEC_INFO_VP6_PP_INFO_T;

#define VP6_ALPHA_ENABLE   (1 << 0)
#define VP6_ALPHA_FRAME    (1 << 1)

typedef struct _VDEC_INFO_VP6_DEC_PRM_T_
{    
    //UCHAR  ucVopCdTp;  // in VP6 
    //UCHAR  ucVopQuant; // in VP6
//#if CONFIG_DRV_VERIFY_SUPPORT
    UINT32  u4FRefBufIdx;
//#endif
    INT32    i4MemBase;
    BOOL    fgAdobeMode;
    //UINT32  u4VLDWrapperWorkspace;
    //UINT32  u4PPWrapperWorkspace;
    VDEC_INFO_VP6_FRM_HDR_T *prFrmHdr;
    //VDEC_INFO_VP6_WORK_BUF_SA_T rVp6WorkBufSa;   
    VDEC_INFO_VP6_FRAME_BUF_SA_T rVp6FrameBufSa;
    VDEC_INFO_VP6_PP_INFO_T           rVp6PpInfo;

    // Alpha Channel
    UINT8 u1AlphaFlag;
    UINT32 au4VldWrapper[196];
    UINT32 au4Reorder[16];
    //~Alpha Channel

} VDEC_INFO_VP6_DEC_PRM_T;


typedef struct _VDEC_HAL_DEC_VP6_ERR_INFO_T_
{    
    UINT32 u4Vp6ErrCnt;                                ///< Video decode error count
    UINT32 u4Vp6ErrRow;                                ///< Video decode error mb row
    UINT32 u4Vp6ErrType;                               ///< Video decode error type
    UINT16 u2Vp6MBErrCnt; 
}VDEC_INFO_VP6_ERR_INFO_T;

#endif //#ifndef _VDEC_INFO_VP6H_

