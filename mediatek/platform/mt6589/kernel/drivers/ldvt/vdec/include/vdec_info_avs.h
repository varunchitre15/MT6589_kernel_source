
#ifndef _VDEC_INFO_AVS_H_
#define _VDEC_INFO_AVS_H_

//#include "drv_config.h"
//#include "drv_vdec.h"

#if(!CONFIG_DRV_VERIFY_SUPPORT)
#include "vdec_usage.h"
#include "vdec_info_common.h"
#else
//#include "x_stl_lib.h"
//#include "x_assert.h"
//#include "u_pbinf.h"
#endif

#define REF_LIST_0 0
#define REF_LIST_1 1
#define REF_LIST_2 2
#define AVS_MAX_FRM_BUFNUM 22
#define AVS_MAX_REF_PICNUM 3

#define AVS_PREV_P_IDX 1
#define AVS_LAST_P_IDX 2
#define AVS_PREV_FW_IDX 0
#define AVS_FW_REF_IDX 1
#define AVS_BW_REF_IDX 2
#define AVS_CURR_WORK_IDX 3
#define AVS_DPB_NUM  4

typedef struct _VDEC_INFO_AVS_VFIFO_PRM_T_
{
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_AVS_VFIFO_PRM_T;

typedef struct _VDEC_INFO_AVS_BS_INIT_PRM_T_
{
    UINT32  u4VLDRdPtr;
    UINT32  u4VLDWrPtr;
    UINT32  u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32  u4VFifoEa;                 ///< Video Fifo memory end address
    UINT32  u4WritePointer;
}VDEC_INFO_AVS_BS_INIT_PRM_T;

typedef struct _VDEC_INFO_AVS_SEQ_HDR_T_
{
    UINT32 u4ProfileID;
    UINT32 u4LevelID;
    UINT32 u4IsProgSeq;
    UINT32 u4HSize;
    UINT32 u4VSize;
    UINT32 u4ChromaFmt;
    UINT32 u4SamplePrec;
    UINT32 u4AspRatio;
    UINT32 u4FrmRate ;
    UINT32 u4BitRateL;
    UINT32 u4MarketBitBR;
    UINT32 u4BitRateU;
    UINT32 u4LowDelay;
    UINT32 u4MarkerBit;
    UINT32 u4BBVSize;
    UINT32 u4RsvBits;
    UINT32 u4IsValid;
    UINT32 u2WidthDec;
    UINT32 u2HeightDec;
    UINT32 u4LastHSize;
    UINT32 u4LastVSize;
}VDEC_INFO_AVS_SEQ_HDR_T;

typedef struct _VDEC_INFO_AVS_PIC_HDR_T_	
{
    UINT32 u4BBVDelay;
    UINT32 u4TimeCodeFg;
    UINT32 u4TimeCode;
    UINT32 u4MarkerBit;
    UINT32 u4BBVCheckTimes;
    UINT32 u4PicCodingType;
    UINT32 u4PicDistance;
    UINT32 u4ProgFrm;
    UINT32 u4PicStruct;
    UINT32 u4AdvPredModeDisable;
    UINT32 u4TFT;
    UINT32 u4RFF;
    UINT32 u4FixedPicQP;
    UINT32 u4PicQP;
    UINT32 u4PicRefFg;
    UINT32 u4NoForwardRefFg;
    UINT32 u4SkipModeFg;
    UINT32 u4RsvBits;
    UINT32 u4LoopFilterDisable;
    UINT32 u4LoopFilterParamFg;
    UINT32 u4IsValid;    //stuffing bit
    INT32 i4AlphaCOffset;
    INT32 i4BetaOffset;
    BOOL  fgIsIPic;
    BOOL  fgSecField;
}VDEC_INFO_AVS_PIC_HDR_T;

typedef struct _VDEC_INFO_AVS_SLICE_HDR_T_
{
    UINT16 u2VPosExt;
    UINT16 u2FixedSliceQP;
    UINT16 u2SliceQP;
    UINT16 u2SliceWeightFg;
    UINT16 u2MBWeightFg;
    UINT8 u1LumaScale[2];
    INT8 i1LumaShift[2];
    UINT8 u1ChromaScale[2];
    INT8 i1ChromaShift[2];
} VDEC_INFO_AVS_SLICE_DHR_T;

typedef struct _VDEC_INFO_AVS_WORK_BUF_SA_T_
{
    UINT32  u4PredSa;
    UINT32  u4Mv1;
    UINT32  u4Mv2;
}VDEC_INFO_AVS_WORK_BUF_SA_T;

typedef struct _VDEC_INFO_AVS_FRAME_BUF_SA_T_
{
    UINT32  u4Pic0YSa;
    UINT32  u4Pic0CSa;
    UINT32  u4Pic1YSa;
    UINT32  u4Pic1CSa;
    UINT32  u4Pic2YSa;
    UINT32  u4Pic2CSa;
}VDEC_INFO_AVS_FRAME_BUF_SA_T;

typedef struct _VDEC_INFO_AVS_FBUF_INFO_T_
{ 
    BOOL     fgNonExisting;
    BOOL     fgIsBufRef;
    BOOL     fgIsErr;
    BOOL     fgIsNotDisp;    
    UCHAR   ucFbId;

    UINT8 u1FBufStatus;  // 1:Top decoded, 2: Bottom decoded
    UINT8 u1FBufRefType;
    UINT8 u1TFldRefType;
    UINT8 u1BFldRefType;

    UINT32 u4FrameNum;
    UINT32 u4SliceType;
    INT32 i4FrameNumWrap;
        
    UINT32  u4YAddr;
    UINT32  u4CAddrOffset;
            
    BOOL fgVirtualDec;                                 
    UINT64  u8Pts;
    UINT64  u8Offset;

    // Picture coding type
    UINT8 u1PicCodingType;

    // Frame picture distance
    UINT32 u4PicDistance;

    #if(CONFIG_DRV_VERIFY_SUPPORT)
    UINT32  u4W;
    UINT32  u4H;  
    UINT32  u4DecOrder;
    UINT32  u4DramPicSize;  // change name to u4CAddrOffset and add u4MVStartAddr TODO:071021
    UINT32  u4DramPicArea; // maybe will be removed TODO:071021
    UINT32  u4Addr;  // change name to u4YStartAddr   TODO:071021
    #endif    
} VDEC_INFO_AVS_FBUF_INFO_T;

typedef struct _VDEC_INFO_AVS_REF_BUF_T
{ 
    BOOL fgValid;

    //FBM
    UCHAR ucFbId;

    // Fb Index
    UINT8 u1DecFBufIdx;
}VDEC_INFO_AVS_REF_BUF_T;

typedef struct _VDEC_INFO_AVS_DEC_PRM_T_
{    
    UINT32  u4FRefBufIdx;
    INT32    i4MemBase;
    UINT8    u1LastRefIdx;
    UCHAR   u1DecFBufIdx;
    BOOL     fgEnPP;
    UINT32   u4MaxFBufNum;
    // Decoding buffer
    UCHAR* pucDecWorkBufY;
    UCHAR* pucDecWorkBufC;
    UCHAR* pucDecWorkBufMV;

    //For 8320  PANDA
    UINT32 u4VLDWrapperWrok;
    UINT32 u4PPWrapperWrok;
    
    VDEC_INFO_AVS_SEQ_HDR_T *prSeqHdr;
    VDEC_INFO_AVS_PIC_HDR_T *prPicHdr;
    VDEC_INFO_AVS_WORK_BUF_SA_T rAvsWorkBufSa;
    VDEC_INFO_AVS_FRAME_BUF_SA_T rAvsFrameBufSa;
    VDEC_INFO_AVS_FBUF_INFO_T arFBufInfo[AVS_DPB_NUM];
    //VDEC_INFO_AVS_REF_BUF_T arRefFBufInfo[AVS_DPB_NUM];
    VDEC_INFO_AVS_FBUF_INFO_T *prCurrFBufInfo;
}VDEC_INFO_AVS_DEC_PRM_T;

typedef struct _VDEC_HAL_DEC_AVS_ERR_INFO_T_
{    
    UINT32 u4AvsErrCnt;                                ///< Video decode error count
    UINT32 u4AvsErrRow;                                ///< Video decode error mb row
    UINT32 u4AvsErrType;                               ///< Video decode error type
    UINT16 u2AvsMBErrCnt; 
}VDEC_INFO_AVS_ERR_INFO_T;

#endif //#ifndef _HAL_VDEC_AVS_IF_H_

