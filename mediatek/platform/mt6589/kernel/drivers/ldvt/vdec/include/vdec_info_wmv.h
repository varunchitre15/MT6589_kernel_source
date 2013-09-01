
#ifndef _VDEC_INFO_WMV_H_
#define _VDEC_INFO_WMV_H_

//#include "drv_config.h"

#include "vdec_usage.h"
#include "vdec_info_common.h"

#define MAXHALFQP           8
#define MAXHIGHRATEQP       8
#define MAX3QP              8

#define MIN_BITRATE_MB_TABLE 50
#define MAX_BITRATE_DCPred_IMBInPFrame 128
#define NUMBITS_SLICE_SIZE 5 // maximum 32 MB's
#define NUMBITS_SLICE_SIZE_WMV2 3 // To indicate Processor's #


typedef enum 
{
    ALL_4EDGES = 0,
    DOUBLE_EDGES,
    SINGLE_EDGES,
    ALL_MBS
}tagMbQtProfile;


typedef enum 
{
    Normal = 0,
    RowPredict,
    ColPredict
}SKIPBITCODINGMODE;


enum { LowRate = 1, MidRate, HighRate};

enum {XFORMMODE_8x8, XFORMMODE_8x4, XFORMMODE_4x8, XFORMMODE_MBSWITCH/* pseudo-mode */, XFORMMODE_4x4};


typedef enum 
{
    WMV_Succeeded = 0,
    WMV_Failed,                         // non-specific error; WMVideoDecReset() and try again at next keyframe.
    WMV_BadMemory,                      // Catastrophic error; close codec and reinit before playing more content.
    WMV_NoKeyFrameDecoded,              // A keyframe must be the first frame after starting or a reset.  WMVideoDecReset(), seek to a keyframe & continue.
    WMV_CorruptedBits,                  // Corrupt bitstream;  WMVideoDecReset() then seek to the next keyframe and try again.
    WMV_UnSupportedOutputPixelFormat,   // Try another color space (we generally like YV12, YUY2, and some RGB formats but not all WMV profiles map to all color spaces)
    WMV_UnSupportedCompressedFormat,    // Either FourCC or internal stream info indicates we can't play this clip.
    WMV_InValidArguments,               // Bad Arguement (can also occur when memory is corrupt).
    WMV_BadSource,                      // N/A in a production decoder -- treat as a catastrophic error.
  
    WMV_NoMoreOutput,                   // WMVideoDecGetOutput called when no output is available.  Don't update screen but OK to continue to decode
    WMV_EndOfFrame,                     // WMVDecCBGetData returns this when there is no more data available for this frame.
    WMV_BrokenFrame,                    // Decoder thinks more data is needed but no more is available, treat as WMV_CorruptedBits
  
    WMV_UnSupportedTransform            // Returned by the CallBack if an indicated external output transform is not supported in hardware
                                        // this is a request to the Codec to do the output transform inside the codec as part of GetOutput.
}tagWMVDecodeStatus;


typedef enum 
{
    PROGRESSIVE = 0,
    INTERLACEFRAME,
    INTERLACEFIELD
}tagFrameCodingMode;


typedef enum
{
    NOT_WMV3 = -1,
    WMV3_SIMPLE_PROFILE,
    WMV3_MAIN_PROFILE,
    WMV3_PC_PROFILE,
    WMV3_ADVANCED_PROFILE,
    WMV3_SCREEN
}tagWMVProfile;


typedef enum 
{
    MIXED_MV,
    ALL_1MV,
    ALL_1MV_HALFPEL,
    ALL_1MV_HALFPEL_BILINEAR,
    INTENSITY_COMPENSATION
}tagMvMode;


typedef struct _VDEC_INFO_WMV_DQUANT_PRM_T_
{
    INT32   i4DoubleStepSize;
    INT32   i4StepMinusStepIsEven;
    INT32   i4DoublePlusStepSize;
    INT32   i4DoublePlusStepSizeNeg;
    INT32   i4DCStepSize; // For Intra
} VDEC_INFO_WMV_DQUANT_PRM_T;


typedef struct _VDEC_INFO_WMV_CMVSCALE_T_ 
{
    INT32   i4MaxZone1ScaledFarMVX;
    INT32   i4MaxZone1ScaledFarMVY;
    INT32   i4Zone1OffsetScaledFarMVX;
    INT32   i4Zone1OffsetScaledFarMVY;
    INT32   i4FarFieldScale1;
    INT32   i4FarFieldScale2;
    INT32   i4NearFieldScale;
} VDEC_INFO_WMV_CMVSCALE_T;


typedef struct _VDEC_INFO_WMV_MULTIRES_PRM_T_
{
    INT32   i4FrmWidthSrc;
    INT32   i4FrmHeightSrc;
    INT32   i4WidthDec;
    INT32   iHeightDec;
    UINT32 u4NumMBX;
    UINT32 u4NumMBY;
} VDEC_INFO_WMV_MULTIRES_PRM_T;


typedef struct _VDEC_INFO_WMV_PAN_SCAN_WINDOW_T_
{
    UINT32 u4PanScanHorizOffset;
    UINT32 u4PanScanVertOffset;
    UINT32 u4PanScanWidth;
    UINT32 u4PanScanHeight;
} VDEC_INFO_WMV_PAN_SCAN_WINDOW_T;


typedef struct _VDEC_INFO_WMV_SEQ_PRM_T_ 
{
    INT32   i4Profile;
    INT32   i4WMV3Profile;
    UINT8   u1Level;
    BOOL    fgVC1;
    INT32   i4FrameRate;
    UINT16 u2FrameRateToVdp;
    INT32   i4BitRate;
    BOOL    fgPostProcInfoPresent;
    BOOL    fgBroadcastFlags;
    BOOL    fgInterlacedSource;
    BOOL    fgTemporalFrmCntr;
    BOOL    fgSeqFrameInterpolation;
    BOOL    fgProgSeqFrm;
    BOOL    fgDisplayExt;
    UINT32 u4DispHorizSize;
    UINT32 u4DispVertSize;
    BOOL    fgAspectRatioFlag;
    INT32   i4AspectRatio;
    INT32   i4AspectHorizSize;
    INT32   i4AspectVertSize;
    UINT16 u2AspectRatioToVdp;
    BOOL    fgFrameRateFlag;
    BOOL    fgFrameRateInd;
    INT32   i4FrameRateNr;
    INT32   i4FrameRateDr;
    INT32   i4FrameRateExp;
    BOOL    fgColorFormatFlag;
    INT32   i4ColorPrim;
    INT32   i4TransferChar;
    INT32   i4MatrixCoef;
    BOOL    fgHRDPrmFlag;
    INT32   i4HRDNumLeakyBuckets;
    BOOL    fgFirstFrameAfterSPS;
    // Simple & Main Profile
    BOOL    fgYUV411;
    BOOL    fgSpriteMode;
    BOOL    fgXintra8Switch;
    BOOL    fgMultiresEnabled;
    INT32   i4ResIndex;
    BOOL    fgDCTTableMBEnabled;
    BOOL    fgPreProcRange;
    INT32   i4NumBFrames;
    BOOL    fgRotatedIdct;
    BOOL    fgVCMInfoPresent;
    // WMV7 & WMV8
    BOOL    fgMixedPel;
    BOOL    fgFrmHybridMVOn;
    BOOL    fgXintra8;
    BOOL    fgRndCtrlOn;
    INT32   i4SliceCode;
    BOOL    fgSkipBitCoding;
    BOOL    fgNewPcbPcyTable;
    BOOL    fgCODFlagOn;
    INT32   i4SkipBitModeV87;
    INT32   i4Wmv8BpMode;
    INT32   i4SkipBitCodingMode;
    INT32   i4HufNewPCBPCYDec;
    BOOL    fg16bitXform;
    BOOL    fgStartCode;
    BOOL    fgRTMContent;
    BOOL    fgBetaContent;
    INT32   i4BetaRTMMismatchIndex;
    // Sequence header related
    UINT32  u4MaxCodedWidth;
    UINT32  u4MaxCodedHeight;
    UINT32  u4MaxPicWidthSrc;
    UINT32  u4MaxPicHeightSrc;
    UINT32  u4PicWidthSrc;
    UINT32  u4PicHeightSrc;
    UINT32  u4PicWidthCmp;
    UINT32  u4PicHeightCmp;
    UINT32  u4PicWidthDec;
    UINT32  u4PicHeightDec;
    UINT32  u4NumMBX;
    UINT32  u4NumMBY;
    VDEC_INFO_WMV_MULTIRES_PRM_T rMultiResParams[4];
}VDEC_INFO_WMV_SEQ_PRM_T;


typedef struct _VDEC_INFO_WMV_ETRY_PRM_T_ 
{
    BOOL     fgNewEntryPoint;
    BOOL     fgBrokenLink;
    BOOL     fgClosedEntryPoint;
    BOOL     fgPanScanPresent;
    BOOL     fgRefDistPresent;
    INT32    i4RefFrameDistance;
    BOOL     fgLoopFilter;
    BOOL     fgUVHpelBilinear;
    INT32    i4RangeState;
    INT32    i4ReconRangeStateNew;
    INT32    i4ReconRangeState;
    BOOL     fgExtendedMvMode;
    INT32    i4MVRangeIndex;
    INT32    i4DQuantCodingOn;
    BOOL     fgXformSwitch;
    BOOL     fgSequenceOverlap;
    BOOL     fgExplicitSeqQuantizer;
    BOOL     fgExplicitFrameQuantizer;
    BOOL     fgExplicitQuantizer;
    BOOL     fgNewDCQuant;
    UINT32  u4CodedWidth;
    UINT32  u4CodedHeight;
    BOOL     fgExtendedDeltaMvMode;
    INT32    i4DeltaMVRangeIndex;
    INT32    i4ExtendedDMVX;
    INT32    i4ExtendedDMVY;
    BOOL     fgRangeRedYFlag;
    BOOL     fgRangeRedUVFlag;
    INT32    i4RangeRedY;
    INT32    i4RangeMapUV;
    BOOL    fgResolutionChange;    
}VDEC_INFO_WMV_ETRY_PRM_T;


#define MAXPANSCANWINDOWS 4
#define MAX_MBX 128 // for 1920 pixels, 1920/16=120
#define MAX_MBY 68 // for 1080 pixels, 1080/16=67.5
#define BP_MB_BITS 64 // represent 64 MB in a MB-row
typedef struct _VDEC_INFO_WMV_PIC_PRM_T_ 
{
    UCHAR  ucFrameCodingMode; // (FCM)
    //Interlaced
    BOOL    fgInterlaceV2;
    BOOL    fgFieldMode;
    BOOL    fgInterpolateCurrentFrame;
    BOOL    fgUVProgressiveSubsampling;
    INT32   i4PpMethod;
    INT32   i4FrmCntMod4;
    INT32   i4CurrentField; // 0:TOP, 1:BOTTOM field
    INT32   i4CurrentTemporalField; // 0:1st field or frame picture, 1: 2nd field
    UCHAR  ucPicType; // (PTYPE)
    UCHAR  ucFirstFieldType;
    UCHAR  ucSecondFieldType;
    UCHAR  ucPrevPicType;
    INT32   i4TemporalRef; // (TFCNTR)
    INT32   i4MaxZone1ScaledFarMVX;
    INT32   i4MaxZone1ScaledFarMVY;
    INT32   i4Zone1OffsetScaledFarMVX;
    INT32   i4Zone1OffsetScaledFarMVY;
    INT32   i4FarFieldScale1;
    INT32   i4FarFieldScale2;
    INT32   i4NearFieldScale;
    
    INT32   i4MaxZone1ScaledFarBackMVX;
    INT32   i4MaxZone1ScaledFarBackMVY;
    INT32   i4Zone1OffsetScaledFarBackMVX;
    INT32   i4Zone1OffsetScaledFarBackMVY;
    INT32   i4FarFieldScaleBack1;
    INT32   i4FarFieldScaleBack2;
    INT32   i4NearFieldScaleBack;
    
    BOOL    fgTwoRefPictures;
    BOOL    fgUseSameFieldForRef;
    BOOL    fgUseOppFieldForRef;
    BOOL    fgBackRefUsedHalfPel;
    BOOL    fgBackRefTopFieldHalfPelMode;
    BOOL    fgBackRefBottomFieldHalfPelMode;
    BOOL    fgMvResolution;
    BOOL    fgTopFieldFirst; // (TFF)
    BOOL    fgRepeatFirstField; // (RFF)
    UCHAR  ucRepeatFrameCount; // (RPTFRM)
    UCHAR  ucPSVectorNum;
    VDEC_INFO_WMV_PAN_SCAN_WINDOW_T rPanScanWindowInfo[MAXPANSCANWINDOWS]; // (PS_HOFFSET, PS_VOFFSET, PS_WIDTH, PS_HEIGHT)
    INT32   i4RndCtrl;
    BOOL    fgPostRC1;
    INT32   i4BNumerator;
    INT32   i4BDenominator;
    INT32   i4BFrameReciprocal;
    INT32   i4PicQtIdx; // (PQINDEX)
    INT32   i4StepSize;
    INT32   i4DCStepSize;
    INT32   i4Overlap;
    VDEC_INFO_WMV_DQUANT_PRM_T  rDQuantParam3QPDeadzone[64];
    VDEC_INFO_WMV_DQUANT_PRM_T  rDQuantParam5QPDeadzone[64];
    VDEC_INFO_WMV_DQUANT_PRM_T  *prDQuantParam;
    BOOL     fgHalfStep; // (HALFQP)
    BOOL     fgUse3QPDZQuantizer; // (PQUANTIZER)
    UINT32  u4DCTACInterTableIndx; // (TRANSACFRM)
    UINT32  u4DCTACIntraTableIndx; // (TRANSACFRM2)
    BOOL     fgIntraDCTDCTable; // (TRANSDCTAB)
    BOOL     fgDCTTableMB;
    INT32    i4SlicePicHeaderNum;
    UINT32  u4SlicePicHeaderNumField;
    BOOL     fgDCPredIMBInPFrame;
    UCHAR   ucDQuantBiLevelStepSize;
    BOOL     fgDQuantOn;
    INT32    i4Panning;
    UCHAR   ucDiffQtProfile;
    BOOL     fgDQuantBiLevel;
    INT32    i4X9MVMode;
    INT32    i4LumScale;
    INT32    i4LumShift;
    INT32    i4LumScaleTop;
    INT32    i4LumScaleBottom;
    INT32    i4LumShiftTop;
    INT32    i4LumShiftBottom;
    INT32    i4MBModeTable;
    INT32    i4MvTable;
    INT32    i4CBPTable;
    INT32    i42MVBPTable;
    INT32    i44MVBPTable;
    BOOL     fgMBXformSwitching;
    INT32    i4FrameXformMode;
    UINT32  u4ForwardRefPicType;
    UINT32  u4BackwardRefPicType;
    UINT32  u4BPRawFlag;
    BOOL     fgLuminanceWarp;
    BOOL     fgLuminanceWarpTop;
    BOOL     fgLuminanceWarpBottom;
  //  #if(CONFIG_DRV_VERIFY_SUPPORT)
    BOOL     fgWMVBrokenLink;
 //   #endif
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
    INT32 iForwardRefDistance;
    INT32 iBackwardRefDistance;
#endif
}VDEC_INFO_WMV_PIC_PRM_T;


typedef struct _VDEC_INFO_WMV_VFIFO_PRM_T_
{
    UINT32 u4CodeType;              ///< Video decoding type
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_WMV_VFIFO_PRM_T;


typedef struct _VDEC_INFO_WMV_BS_INIT_PRM_T_
{
    UINT32 u4ReadPointer;              
    UINT32 u4WritePointer;
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_WMV_BS_INIT_PRM_T;


typedef struct _VDEC_INFO_WMV_WORK_BUF_SA_T_
{
    UCHAR   ucMv1FbId;
    UCHAR   ucMv2FbId;
    UCHAR   ucMv3FbId;
    UCHAR   ucMv12FbId;
    UCHAR   ucBp1FbId;
    UCHAR   ucBp2FbId;
    UCHAR   ucBp3FbId;
    UCHAR   ucBp4FbId;
    UCHAR   ucDcacFbId;
    UCHAR   ucDcac2FbId;
    UCHAR   ucPp1FbId;
    UCHAR   ucPp2FbId;
    UINT32  u4Pic0YSa;
    UINT32  u4Pic0CSa;
    UINT32  u4Pic1YSa;
    UINT32  u4Pic1CSa;
    UINT32  u4Pic2YSa;
    UINT32  u4Pic2CSa;
    UINT32  u4Mv1Sa;
    UINT32  u4Mv2Sa;
    UINT32  u4Mv3Sa;
    UINT32  u4Bp1Sa;
    UINT32  u4Bp2Sa;
    UINT32  u4Bp3Sa;
    UINT32  u4Bp4Sa;
    UINT32  u4DcacSa;
    UINT32  u4Dcac2Sa;
    UINT32  u4Mv12Sa;
    UINT32  u4Pp1Sa;
    UINT32  u4Pp2Sa;
    UCHAR  ucRealMv1FbId;
    UCHAR  ucRealMv12FbId;
    
    UCHAR  ucDcacNewFbId;
    UCHAR  ucMvNewFbId; 
    UCHAR  ucBp0NewFbId;
    UCHAR  ucBp1NewFbId;
    UCHAR  ucBp2NewFbId;
    UINT32  u4DcacNewSa;
    UINT32  u4MvNewSa;
    UINT32  u4Bp0NewSa;
    UINT32  u4Bp1NewSa;
    UINT32  u4Bp2NewSa;
    
    #if 1 //(WMV_8320_SUPPORT)
    UINT32 u4VLDWrapperWrok;
    UINT32 u4PPWrapperWrok;
    #endif
}VDEC_INFO_WMV_WORK_BUF_SA_T;


typedef struct _VDEC_INFO_WMV_DEC_BP_PRM_T_
{    
    UCHAR  ucFrameCodingMode;                                         
    UCHAR  ucPicType;             
    INT32   i4CodecVersion;                                
    INT32   i4Wmv8BpMode;   
    UINT32 u4PicHeightSrc;                                  
    UINT32 u4PicHeightDec;                                   
    UINT32 u4NumMBX; 
  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    UINT32 u4NumMBY; 
  #endif
    VDEC_INFO_WMV_WORK_BUF_SA_T rWmvWorkBufSa;
    BOOL    fgWmvMode;
}VDEC_INFO_WMV_DEC_BP_PRM_T;


typedef struct _VDEC_INFO_WMV_ICOMP_T_
{
    INT32   i4Scale;
    INT32   i4Shift;
    INT32   i4Enable;    
} VDEC_INFO_WMV_ICOMP_T;


typedef struct _VDEC_INFO_WMV_ICOMP_SET_T_
{
    VDEC_INFO_WMV_ICOMP_T New;
    VDEC_INFO_WMV_ICOMP_T Old;
} VDEC_INFO_WMV_ICOMP_SET_T;


typedef struct _VDEC_INFO_WMV_ICOMP_PRM_T_
{
    UCHAR  ucFrameTypeLast;
    UCHAR  ucFrameTypeLastTop;
    UCHAR  ucFrameTypeLastBot;
    UCHAR  ucPreProcessFrameStatus;
    INT32   i4BoundaryUMVIcomp;
    INT32   i4SecondFieldParity;
    INT32   i4BoundaryUMVIcompEnable;
    INT32   i4FirstFieldIntensityComp;
    INT32   i4ResetMvDram;
    VDEC_INFO_WMV_ICOMP_SET_T OldTopField;
    VDEC_INFO_WMV_ICOMP_SET_T NewTopField;
    VDEC_INFO_WMV_ICOMP_SET_T OldBotField;
    VDEC_INFO_WMV_ICOMP_SET_T NewBotField; 
}VDEC_INFO_WMV_ICOMP_PRM_T;
    

typedef struct _VDEC_INFO_WMV_DEC_PRM_T_
{  
    INT32    i4CodecVersion;
    INT32    i4MemBase;
    UINT32  u4FRefBufIdx;
    VDEC_INFO_WMV_SEQ_PRM_T *prSPS;
    VDEC_INFO_WMV_ETRY_PRM_T *prEPS;
    VDEC_INFO_WMV_PIC_PRM_T *prPPS;
    VDEC_INFO_WMV_ICOMP_PRM_T *prICOMPS;
    VDEC_INFO_WMV_WORK_BUF_SA_T rWmvWorkBufSa;
#if  (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)  
    UINT32 u4WMVSliceAddr[3];
#endif
    BOOL    fgBwdRefPVOP;
    BOOL    fgWmvMode;
}VDEC_INFO_WMV_DEC_PRM_T;


typedef struct _VDEC_INFO_WMV_ERR_INFO_T_
{    
    UINT32 u4MC_X;
    UINT32 u4MC_Y;
    UINT32 u4WmvErrCnt;                                ///< Video decode error count
    UINT32 u4WmvErrRow;                                ///< Video decode error mb row
    UINT32 u4WmvErrType;                               ///< Video decode error type
}VDEC_INFO_WMV_ERR_INFO_T;
 

 typedef struct _VDEC_INFO_WMV_MV_BUF_SA_T_
{
    UCHAR   ucMv1FbId;
    UINT32   u4Mv1Sa;
    UCHAR   ucMv12FbId;
    UINT32   u4Mv12Sa;
    UCHAR   ucMvNewFbId;
    UINT32   u4MvNewSa;
}VDEC_INFO_WMV_MV_BUF_SA_T;

typedef struct _VDEC_INFO_WMV_SR_STORE_PRM_T_ 
{
    //EPS parts
    BOOL     fgNewEntryPoint;
    INT32    i4RefFrameDistance;
	
    //PPS parts    
    //UCHAR  ucPrevPicType;
    //INT32   i4TemporalRef; // (TFCNTR)    
    //BOOL    fgBackRefUsedHalfPel;
    BOOL    fgBackRefTopFieldHalfPelMode;
    BOOL    fgBackRefBottomFieldHalfPelMode;
    //BOOL     fgMBXformSwitching;
    INT32    i4FrameXformMode;
    UINT32  u4ForwardRefPicType;
    UINT32  u4BackwardRefPicType;
}VDEC_INFO_WMV_SR_STORE_PRM_T;
#endif //#ifndef _HAL_VDEC_WMV_IF_H_

