
#ifndef _VDEC_INFO_MPEG_H_
#define _VDEC_INFO_MPEG_H_

//#include "drv_config.h"
//#include "chip_ver.h"

#include "type.h"
#include <mach/mt_typedefs.h>

enum Vop_Coding_Type
{
    VCT_I = 0x0,
    VCT_P = 0x1,
    VCT_B = 0x2,
    VCT_S = 0x3
};


enum Sprite_Enable
{
    SPRITE_NOT_USED = 0x0,
    STATIC          = 0x1,
    GMC             = 0x2
};

typedef struct _VDEC_INFO_MPEG_DIR_MODE_T_
{
    UINT32 u4Trd;
    UINT32 u4Trb;
    UINT32 u4Trdi;
    UINT32 u4Trbi;
    UINT32 u4TFrm;
} VDEC_INFO_MPEG_DIR_MODE_T;


typedef struct _VDEC_INFO_MPEG_VFIFO_PRM_T_
{
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    UINT32 u4CodeType;                ///< Video decoding type
#endif
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_MPEG_VFIFO_PRM_T;


typedef struct _VDEC_INFO_MPEG_BS_INIT_PRM_T_
{
    UINT32 u4ReadPointer;              
    UINT32 u4WritePointer;
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_MPEG_BS_INIT_PRM_T;


typedef struct _VDEC_INFO_MPEG_QANTMATRIX_T_
{
    UCHAR ucQuantMatrix[64];                           ///< Quantization matrix array     
}VDEC_INFO_MPEG_QANTMATRIX_T;


typedef struct _VDEC_INFO_MPEG2_PIC_PRM_T_
{
    // Picture structure: FRM_PIC, TOP_FLD_PIC, BTM_FLD_PIC
    UCHAR  ucPicStruct;
    // picture coding type: I_TYPE, B_TYPE, P_TYPE
    UCHAR  ucPicCdTp;
    // frame_pred_frame_dct
    UCHAR  ucFrmPredFrmDct;
    // concealment_motion_vectors
    UCHAR  ucConcMotVec;
    // q_scale_type
    UCHAR  ucQScaleType;
    // top field first
    UCHAR  ucTopFldFirst;
    // full_pel_forward_vector
    UCHAR  ucFullPelFordVec;
    // full_pel_backward_vector
    UCHAR  ucFullPelBackVec;
    // intra_vld_format
    UCHAR  ucIntraVlcFmt;
    // intra_dc_precision
    UCHAR  ucIntraDcPre;
    // alternate_scan
    UCHAR  ucAltScan;
  
    UCHAR  pucfcode[2][2];
    UCHAR  ucFordFCode;
    UCHAR  ucBackFCode;
} VDEC_INFO_MPEG2_PIC_PRM_T;


typedef struct _VDEC_INFO_DIVX3_PIC_PRM_T_
{
    UCHAR  ucAltIAcChromDct;
    UCHAR  ucAltIAcChromDctIdx;
    UCHAR  ucAltIAcLumDct;
    UCHAR  ucAltIAcLumDctIdx;
    UCHAR  ucAltIDcDct;
    UCHAR  ucHasSkip;
    UCHAR  ucAltPAcDct;
    UCHAR  ucAltPAcDctIdx;
    UCHAR  ucAltPDcDct;
    UCHAR  ucAltMv;
    UCHAR  ucFrameMode;
    UCHAR  ucSliceBoundary[5];
} VDEC_INFO_DIVX3_PIC_PRM_T;


typedef struct _VDEC_INFO_MPEG4_VOL_PRM_T_
{
    UCHAR  ucShortVideoHeader;
    UINT16 u2VopTimeIncrementResolution;
    UCHAR  ucInterlaced;
    UCHAR  ucQuantType;
    UCHAR  ucQuarterSample;
    UCHAR  ucResyncMarkerDisable;
    UCHAR  ucDataPartitioned;
    UCHAR  ucReversibleVlc;
    UCHAR  ucSourceFormat; // for short video header
    UCHAR  ucSorenson;      //Sorenson H263
} VDEC_INFO_MPEG4_VOL_PRM_T;


typedef struct _VDEC_INFO_MPEG_GMC_PRM_T_
{
    UCHAR  ucEffectiveWarpingPoints;
    INT32   i4GmcYMvX;
    INT32   i4GmcYMvY;
    INT32   i4GmcCMvX;
    INT32   i4GmcCMvY;
} VDEC_INFO_MPEG_GMC_PRM_T;


typedef struct _VDEC_INFO_MPEG4_VOP_PRM_T_
{
    UCHAR  ucIntraDcVlcThr;
    UCHAR  fgTopFldFirst;
    UCHAR  ucFordFCode;
    UCHAR  ucBackFCode;
    UCHAR  ucBRefCdTp;
    UCHAR  fgAlternateVerticalScanFlag;
    VDEC_INFO_MPEG_DIR_MODE_T *prDirMd;   // for direct mode
    VDEC_INFO_MPEG_GMC_PRM_T *prGmcPrm;
} VDEC_INFO_MPEG4_VOP_PRM_T;


typedef struct _VDEC_INFO_M4V_DEC_PRM_T_
{
    VDEC_INFO_MPEG4_VOL_PRM_T *prVol;
    VDEC_INFO_MPEG4_VOP_PRM_T *prVop;
} VDEC_INFO_M4V_DEC_PRM_T;


typedef struct _VDEC_INFO_MPEG4_WORK_BUF_SA_T_
{
    UINT32  u4DcacSa;
    UINT32  u4MvecSa;
    UINT32  u4Bmb1Sa;
    UINT32  u4Bmb2Sa;
    UINT32  u4BcodeSa;
    UINT32  u4VldWrapperSa;
    UINT32  u4PPWrapperSa;
    //6589NEW 2.4, 2.5, 4.1
    UINT32  u4DataPartitionSa;
    UINT32  u4NotCodedSa;
    UINT32  u4MvDirectSa;
}VDEC_INFO_MPEG4_WORK_BUF_SA_T;


#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
typedef struct _VDEC_INFO_MPEG4_WORK_BUF_SZ_T_ 
{
    UINT32  u4DcacSize;
    UINT32  u4BcodeSize;
	UINT32  u4MVSize;
	UINT32  u4MB1Size;
	UINT32  u4MB2Size;
    //6589NEW 2.3, 2.6
    UINT32  u4DataPartitionSize;
    UINT32  u4NotCodedSize;
}VDEC_INFO_MPEG4_WORK_BUF_SZ_T;
#endif


typedef struct _VDEC_INFO_MPEG4_DEC_PRM_T_
{
    // common for MPEG4 and DivX3
    UCHAR  ucVopCdTp;  // in MPEG4 VOP layer
    UCHAR  ucVopQuant; // in MPEG4 VOP layer
    UCHAR  ucVopRoundingType; // in MPEG4 VOP layer
  
    // for unrestricted mv
    UINT32 u4UmvPicW;
    UINT32 u4UmvPicH;
    UINT32 u4QPelType;
    UINT32 u4CMvType;
    VDEC_INFO_MPEG4_WORK_BUF_SA_T rMpeg4WorkBufSa;
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    VDEC_INFO_MPEG4_WORK_BUF_SZ_T rMpeg4WorkBufSize; 
    #endif
    union
    {
        VDEC_INFO_DIVX3_PIC_PRM_T rDx3DecPrm;
        VDEC_INFO_M4V_DEC_PRM_T rM4vDecPrm;
    } rDep;
} VDEC_INFO_MPEG4_DEC_PRM_T;


typedef struct _VDEC_INFO_MPEG_FRAME_BUF_SA_T_
{
    UINT32  u4Pic0YSa;
    UINT32  u4Pic0CSa;
    UINT32  u4Pic1YSa;
    UINT32  u4Pic1CSa;
    UINT32  u4Pic2YSa;
    UINT32  u4Pic2CSa;
}VDEC_INFO_MPEG_FRAME_BUF_SA_T;


typedef struct _VDEC_INFO_MPEG_PP_INFO_T_
{
    BOOL     fgPpEnable;
    BOOL     fgPpDemoEn;
    UINT8    u1PpLevel;
    UINT8    au1MBqp[4];
    UINT32  u4PpYBufSa;
    UINT32  u4PpCBufSa;
}VDEC_INFO_MPEG_PP_INFO_T;


typedef struct _VDEC_INFO_MPEG_DEC_PRM_T_
{
    // MPEG version: 1, 2, 3, 4 (3: DivX 3.11)
    UCHAR  ucMpegVer;
    UCHAR  ucDecFld;
    UINT32  u4FRefBufIdx;
    
    // Decode picture setting
    BOOL    fgB21Mode;
    BOOL    fgRetErr;
    BOOL    fgIgnoreVdo;
    BOOL    fgDec2ndFld;
    UINT32 u4DecXOff;
    UINT32 u4DecYOff;
    UINT32 u4DecW;
    UINT32 u4DecH;
    UINT32 u4MaxMbl;
    UINT32 u4BBufStart;
    VDEC_INFO_MPEG_FRAME_BUF_SA_T rMpegFrameBufSa;
    VDEC_INFO_MPEG_PP_INFO_T rMpegPpInfo;
    union
    {
        VDEC_INFO_MPEG2_PIC_PRM_T rMp2PicPrm;
        VDEC_INFO_MPEG4_DEC_PRM_T rMp4DecPrm;
    } rPicLayer;
} VDEC_INFO_MPEG_DEC_PRM_T;


typedef struct _VDEC_HAL_DEC_MPEG_ERR_INFO_T_
{    
    UINT32 u4MpegErrCnt;                                ///< Video decode error count
    UINT32 u4MpegErrRow;                                ///< Video decode error mb row
    UINT32 u4MpegErrType;                               ///< Video decode error type
    UINT16 u2MpegMBErrCnt; 
}VDEC_INFO_MPEG_ERR_INFO_T;

#endif //#ifndef _VDEC_INFO_MPEG_H_

