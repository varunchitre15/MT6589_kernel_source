
#ifndef _VDEC_DRV_H264_INFO_H_
#define _VDEC_DRV_H264_INFO_H_

//#include "x_os.h"
//#include "x_bim.h"
//#include "x_assert.h"
//#include "x_timer.h"

#include "drv_common.h"

#include "vdec_common_if.h"
#include "vdec_usage.h"

#include "vdec_info_h264.h"
#include "vdec_info_common.h"

#include "vdec_hal_if_h264.h"

#define OUT_OF_FILE 0x000001
//#define FORBIDEN_ERR 0x000002
#define DEC_INIT_FAILED 0x000003

#define NON_IDR_SLICE 0x01
#define IDR_SLICE 0x05
#define H264_SEI 0x06
#define H264_SPS 0x07
#define H264_PPS 0x08
#define H264_END_SEQ 0x0A
#define H264_PREFIX_NAL 0x0E
#define H264_SUB_SPS 0x0F
#define H264_SLICE_EXT 0x14

// Slice_type
#define H264_P_Slice 0
#define H264_B_Slice 1
#define H264_I_Slice 2
#define H264_SP_Slice 3
#define H264_SI_Slice 4
#define H264_P_Slice_ALL 5
#define H264_B_Slice_ALL 6
#define H264_I_Slice_ALL 7
#define H264_SP_Slice_ALL 8
#define H264_SI_Slice_ALL 9

#define NREF_PIC 0
#define SREF_PIC 1
#define LREF_PIC 2

//AVC Profile IDC definitions
#define BASELINE_PROFILE					66      //!< YUV 4:2:0/8  "Baseline"
#define MAIN_PROFILE						77      //!< YUV 4:2:0/8  "Main"
#define EXTENDED_PROFILE					88      //!< YUV 4:2:0/8  "Extended"
#define FREXT_HP_PROFILE					100		//!< YUV 4:2:0/8 "High"
#define FREXT_Hi10P_PROFILE					110		//!< YUV 4:2:0/10 "High 10"
#define FREXT_Hi422_PROFILE					122		//!< YUV 4:2:2/10 "High 4:2:2"
#define FREXT_Hi444_PROFILE					244		//!< YUV 4:4:4/14 "High 4:4:4"
#define FREXT_CAVLC444_PROFILE				44      //!< YUV 4:4:4/14 "CAVLC 4:4:4"
#define SCALABLE_BASELINE_PROFILE	83		//!< Scalable Baseline profile
#define SCALABLE_HIGH_PROFILE		86		//!< Scalable High profile
#define MULTIVIEW_HIGH_PROFILE		118		//!< Multiview High profile
#define STEREO_HIGH_PROFILE		128		//!< Stereo High profile

#define YUV400 0
#define YUV420 1
#define YUV422 2
#define YUV444 3

#define H264_MAX_FB_NUM 17
#define H264_MAX_REF_PIC_LIST_NUM 3
#define H264_DPB_FBUF_UNKNOWN 0xFF
#define H264_FRM_IDX_UNKNOWN 0xFFFFFFFF
#define H264_FRM_NUM_UNKNOWN 0xFFFFFFFF
#define H264_FRM_NUM_WRAP_UNKNOWN 0x7FFFFFFF
#define H264_MAX_POC 0x7FFFFFFF
#define H264_MIN_POC 0x80000001
#define H264_MAX_PIC_NUM 0xEFFFFFFF

#define SubWidthC  [4]= { 1, 2, 2, 1};
#define SubHeightC [4]= { 1, 2, 1, 1};

#define H264_MAX_SPS_NUM 32
#define H264_MAX_PPS_NUM 256

#define H264_MAX_REF_LIST_NUM 6

#define fgIsH264SeqEnd(arg)   (arg & SEQ_END)

typedef enum _H264_DPB_SIZE_T
{
    H264_LEVEL_1_0 = 10,
    H264_LEVEL_1_b = 9,
    H264_LEVEL_1_1 = 11,
    H264_LEVEL_1_2 = 12,    
    H264_LEVEL_1_3 = 13,
    H264_LEVEL_2_0 = 20,
    H264_LEVEL_2_1 = 21,
    H264_LEVEL_2_2 = 22,
    H264_LEVEL_3_0 = 30,
    H264_LEVEL_3_1 = 31,
    H264_LEVEL_3_2 = 32,
    H264_LEVEL_4_0 = 40,
    H264_LEVEL_4_1 = 41,
    H264_LEVEL_5_0 = 50,
    H264_LEVEL_5_1 = 51,
}H264_DPB_SIZE;

typedef enum _H264_RPIC_LIST
{
    H264_P_SREF_TFLD = 0,
    H264_P_SREF_BFLD = 1,
    H264_B0_SREF_TFLD = 2,
    H264_B0_SREF_BFLD = 3,
    H264_B1_SREF_TFLD = 4,
    H264_B1_SREF_BFLD = 5,
    H264_P_LREF_TFLD = 6,
    H264_P_LREF_BFLD = 7,    
    H264_B_LREF_TFLD = 8,
    H264_B_LREF_BFLD = 9,   
}H264_RPIC_LIST;

typedef enum _H264_DPB_SEARCH_T
{
    H264_DPB_S_EMPTY = 0,
    H264_DPB_S_DECODED = 1,
    H264_DPB_S_DECODED_NO_CURR = 2,
    H264_DPB_S_DECODED_NO_CURR_IP_ONLY = 3,
#if VDEC_SKYPE_SUPPORT    
    H264_DPB_S_SKYPE_ORDER = 4,
#endif
}H264_DPB_SEARCH_T;

typedef struct VDEC_INFO_H264_AU_T_
{
    UCHAR  ucPicStruct;
    UCHAR  ucDpbId;         // Idx in DPB                
    UINT32 u4PicCdType;
    INT32   i4POC;  
    UCHAR ucH264OwnFBUFListIdx;
    UCHAR ucH264RefFBUFListIdx[H264_MAX_FB_NUM];    
    UCHAR ucH264OutputListWrIdx;    
    UCHAR ucH264OutputListRdIdx;    
    UCHAR arH264OutputList[H264_MAX_FB_NUM];   
    UINT32 u4SlicePPSID;       
    VDEC_INFO_H264_PPS_T rH264AUPPSInfo;
    VDEC_INFO_H264_SPS_T rH264AUSPSInfo;
    VDEC_INFO_H264_LAST_INFO_T rH264AULastInfo;
}VDEC_INFO_H264_AU_T;

#ifdef VDEC_SR_SUPPORT
#define H264_MAX_DFB_NUM MPEG_DFB_NUM
typedef struct VDEC_INFO_H264_DFB_LSIT_INFO_T_
{
    UCHAR ucH264DFBListWrIdx;
    UCHAR ucH264DFBListRdIdx;
    UCHAR ucH264MaxBLPtsDFBIdx;
    UCHAR ucH264EDPBLastRemoveIdx;    
    UCHAR ucH264LastReconstructDFBIdx;
    UINT64 u8H264MaxBLPts;
    VDEC_INFO_H264_AU_T arH264DFBList[2][H264_MAX_DFB_NUM]; 
    VDEC_INFO_H264_FBUF_INFO_T arH264DFBFbInfo[2][H264_MAX_DFB_NUM];
    FBM_FRAMEINFO arH264DFBFrameInfo[H264_MAX_DFB_NUM];
}VDEC_INFO_H264_DFB_LSIT_INFO_T;
#endif

typedef struct _H264_DRV_INFO_T
{
    UCHAR ucH264DpbOutputFbId;
    UCHAR ucPredFbId;
    UINT32 u4PredSa;
    UINT32 u4CurrStartCodeAddr;
    UINT32 u4BitCount;
    UINT32 u4DecErrInfo;
    INT32 i4LatestIPOC;
    INT64 i8BasePTS;
    INT64 i8LatestRealPTS;
    INT32 i4PreFrmPOC;
    INT32 i4LatestRealPOC;
    UINT32 u4LatestSPSId;
    VDEC_INFO_H264_SPS_T arH264SPS[H264_MAX_SPS_NUM];
    VDEC_INFO_H264_PPS_T arH264PPS[H264_MAX_PPS_NUM];
    VDEC_INFO_H264_SLICE_HDR_T rH264SliceHdr;
    VDEC_INFO_H264_SEI_T rH264SEI;
    VDEC_INFO_H264_FBUF_INFO_T arH264FbInfo[H264_MAX_FB_NUM];
#if VDEC_MVC_SUPPORT
    VDEC_INFO_H264_FBUF_INFO_T rH264PrevFbInfo;
#endif
#ifdef VDEC_SR_SUPPORT
    VDEC_INFO_H264_FBUF_INFO_T arH264EDpbInfo[MAX_EDPB_NUM];     // 0: FRef, 1: BRef, 2:Working
    //VDEC_INFO_H264_DFB_LSIT_INFO_T rH264DFBInfo;
    VDEC_INFO_H264_DFB_LSIT_INFO_T *ptH264DFBInfo;
#endif
    VDEC_INFO_H264_REF_PIC_LIST_T arH264RefPicList[H264_MAX_REF_LIST_NUM];
    VDEC_INFO_H264_P_REF_PRM_T rPRefPicListInfo;
    VDEC_INFO_H264_B_REF_PRM_T rBRefPicListInfo;
    VDEC_INFO_H264_BS_INIT_PRM_T rBsInitPrm;
    VDEC_NORM_INFO_T *prVDecNormInfo;
    VDEC_FBM_INFO_T *prVDecFbmInfo;
    VDEC_PIC_INFO_T *prVDecPicInfo;
    VDEC_INFO_DEC_PRM_T rVDecNormHalPrm;    
    void *prH264MvStartAddr;
#ifdef VDEC_SR_SUPPORT
    VDEC_SR_INFO_T *prVDecSRInfo;
#endif    
#if  (defined(DRV_VDEC_VDP_RACING) || defined(VDEC_PIP_WITH_ONE_HW))
    BOOL   fgIsRacing;
    UCHAR ucDecFrmBufCnt;
    BOOL   fgIsReadyPreOutput;
    BOOL   fgPreInsOutFBuf;
    UINT32 u4OutIdx;
    VDEC_INFO_H264_FBUF_INFO_T  rH264OutFbInfo[2];
    
    INT32   i4BaseOutPOC;
    UCHAR   ucWaitClrPicRefIno_FbId;
#endif

   //Patch for .mp4 file
   BOOL    fgHeaderDefineByCFA;
   UINT32 u4SPSHeaderCnt;
   UINT32 u4PPSHeaderCnt;

#if VDEC_MVC_SUPPORT
   VDEC_OFFSET_METADATA_INFO_T* prVDecOMTData;
#endif  
}H264_DRV_INFO_T;

#if 0
static VDEC_INFO_H264_SPS_T _arH264SPS[MPV_MAX_VLD][MAX_SPS_NUM];
static VDEC_INFO_H264_PPS_T _arH264PPS[MPV_MAX_VLD][MAX_PPS_NUM];
static VDEC_INFO_H264_SLICE_HDR_T _rH264SliceHdr[MPV_MAX_VLD];
static VDEC_INFO_H264_DEC_PRM_T _rH264DecPrm[MPV_MAX_VLD];
static VDEC_INFO_H264_FBUF_INFO_T _arH264FbInfo[MPV_MAX_VLD][H264_MAX_FB_NUM];
static VDEC_INFO_H264_FBUF_INFO_T *_prH264CurrFbInfo[MPV_MAX_VLD];
static VDEC_INFO_H264_REF_PIC_LIST_T _arH264RefPicList[MPV_MAX_VLD][6];
static VDEC_INFO_H264_P_REF_PRM_T _arPRefPicListInfo[MPV_MAX_VLD];
static VDEC_INFO_H264_B_REF_PRM_T _arBRefPicListInfo[MPV_MAX_VLD];
#endif
    
#define fgIsRefPic(arg) ((arg > 0))
#define fgIsIDRPic(arg) ((arg == IDR_SLICE))
//#define fgIsFrmPic(arg) ((_rH264DecPrm[arg].bPicStruct == FRAME))  prVDecH264DecPrm->fgIsFrmPic

#define fgIsISlice(bType) ((bType == H264_I_Slice) ||(bType == H264_SI_Slice) || (bType == H264_I_Slice_ALL))
#define fgIsPSlice(bType) ((bType == H264_P_Slice) ||(bType == H264_SP_Slice) || (bType == H264_P_Slice_ALL))
#define fgIsBSlice(bType) ((bType == H264_B_Slice) || (bType == H264_B_Slice_ALL))

#if VDEC_MVC_SUPPORT
#define fgIsMVCBaseView(arg) (arg == VDEC_MVC_BASE)
#define fgIsMVCDepView(arg) (arg == VDEC_MVC_DEP)
#define fgIsMVCType(arg)  (fgIsMVCBaseView(arg)  || fgIsMVCDepView(arg))
#endif

extern void vH264InitProc(UCHAR ucEsId);
extern INT32 i4H264VParseProc(UCHAR ucEsId, UINT32 u4VParseType);
extern BOOL fgH264VParseChkProc(UCHAR ucEsId);
extern INT32 i4H264UpdInfoToFbg(UCHAR ucEsId);
extern void vH264StartToDecProc(UCHAR ucEsId);
extern void vH264ISR(UCHAR ucEsId);
extern BOOL fgIsH264DecEnd(UCHAR ucEsId);
extern BOOL fgIsH264DecErr(UCHAR ucEsId);
extern BOOL fgH264ResultChk(UCHAR ucEsId);
extern BOOL fgIsH264InsToDispQ(UCHAR ucEsId);
extern BOOL fgIsH264GetFrmToDispQ(UCHAR ucEsId);
extern void vH264EndProc(UCHAR ucEsId);
extern BOOL fgH264FlushDPB(UCHAR ucEsId, BOOL fgWithOutput);
extern void vH264ReleaseProc(UCHAR ucEsId, BOOL fgResetHW);

#ifdef VDEC_SR_SUPPORT
extern BOOL fgH264GenEDPB(UCHAR ucEsId);
extern BOOL fgH264RestoreEDPB(UCHAR ucEsId, BOOL fgRestore);
extern BOOL fgIsH264GetSRFrmToDispQ(UCHAR ucEsId, BOOL fgSeqEnd, BOOL fgRefPic);
extern void vH264GetSeqFirstTarget(UCHAR ucEsId);
extern void vH264ReleaseSRDrvInfo(UCHAR ucEsId);
extern BOOL fgH264GetDFBInfo(UCHAR ucEsId, void **prDFBInfo);
extern void vH264RestoreSeqInfo(UCHAR ucEsId);
extern BOOL fgH264RvsDone(UCHAR ucEsId);
extern void vH264ReleaseEDPB(UCHAR ucEsId);
#endif

#if  (defined(DRV_VDEC_VDP_RACING) || defined(VDEC_PIP_WITH_ONE_HW))
extern BOOL fgIsH264PreInsToDispQ(UCHAR ucEsId);
extern BOOL fgIsH264PreGetFrmToDispQ(UCHAR ucEsId);
extern BOOL fgIsH264FrmBufReadyForDisp(UCHAR ucEsId);
extern BOOL fgIsH264PreInsToDispQ(UCHAR ucEsId);
extern BOOL fgIsH264SetFBufInfo(UCHAR ucEsId);
#endif

// vdec_drv_h264_parse.c
extern void vSeq_Par_Set_Rbsp(UINT32 u4BSID,UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
extern INT32 vPic_Par_Set_Rbsp(UINT32 u4BSID, UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
extern void vSEI_Rbsp(UINT32 u4BSID, UINT32 u4VDecID, H264_DRV_INFO_T  *prH264DrvDecInfo);
extern INT32 i4SlimParseSliceHeader(UINT32 u4VDecID, H264_DRV_INFO_T  *prH264DrvDecInfo);
extern INT32 i4ParseSliceHeader(UINT32 u4VDecID, H264_DRV_INFO_T  *prH264DrvDecInfo);

// vdec_drv_h264_decode.c
extern void vFlushBufInfo(H264_DRV_INFO_T *prH264DrvInfo);
extern void vFlushAllSetData(H264_DRV_INFO_T *prH264DrvInfo);
extern UCHAR ucH264GetMaxFBufNum(VDEC_INFO_H264_DEC_PRM_T *prVDecH264DecPrm, UINT32 u4PicWidth, UINT32 u4PicHeight, BOOL fgIsBDDisc);
extern void vPrepareFBufInfo(H264_DRV_INFO_T *prH264DrvInfo);
extern BOOL fgIsNonRefFbuf(VDEC_INFO_H264_FBUF_INFO_T *tFBufInfo);
extern void vSetPicRefType(UCHAR ucPicStruct, UCHAR ucRefType, VDEC_INFO_H264_FBUF_INFO_T *tFBufInfo);
extern void vAdapRefPicmarkingProce(UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvInfo);
extern void vSlidingWindowProce(H264_DRV_INFO_T *prH264DrvInfo);
extern UCHAR ucGetPicRefType(UCHAR ucPicStruct, VDEC_INFO_H264_FBUF_INFO_T *tFBufInfo);
extern UINT32 u4VDecGetMinPOCFBuf(VDEC_ES_INFO_T *prVDecEsInfo, H264_DPB_SEARCH_T eDPBSearchType);
extern void vFlushBufRefInfo(H264_DRV_INFO_T *prH264DrvInfo, BOOL fgIsForceFree);
#if (!defined(DRV_VDEC_VDP_RACING) && !defined(VDEC_PIP_WITH_ONE_HW))
extern UINT32 u4VDecOutputMinPOCFBuf(VDEC_ES_INFO_T *prVDecEsInfo, UINT32 u4MinPOCFBufIdx);
#else
extern UINT32 u4VDecOutputMinPOCFBuf(VDEC_ES_INFO_T *prVDecEsInfo, UINT32 u4MinPOCFBufIdx, BOOL fgFreeBuf, BOOL fgOutBase);
#endif
#ifdef FBM_ALLOC_SUPPORT
extern void vFreeH264WorkingArea(VDEC_ES_INFO_T *prVDecEsInfo);
#endif
extern void vVDecSetPicInfo(UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvInfo);
extern void vH264SetDownScaleParam(H264_DRV_INFO_T *prH264DrvInfo, BOOL fgEnable);
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
extern void vH264SetLetterBoxParam(H264_DRV_INFO_T *prH264DrvInfo);
#endif
extern void vClrPicRefInfo(UCHAR ucPicStruct, H264_DRV_INFO_T *prH264DrvInfo, UINT32 u4FBufIdx);
extern void vH264ChkWhileLines(UCHAR ucEsId);
extern void vClrFBufInfo(H264_DRV_INFO_T *prH264DrvInfo, UINT32 u4FBufIdx);


void vHrdParameters(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H264_HRD_PRM_T *tHrdPara);
void vInitSPS(VDEC_INFO_H264_SPS_T *prSPS);
void vInitSliceHdr(VDEC_INFO_H264_DEC_PRM_T *prVDecH264DecPrm);
void vRef_Pic_List_Reordering(UINT32 u4VDecID, VDEC_INFO_H264_SLICE_HDR_T *prSliceHdr);
void vDec_Ref_Pic_Marking(UINT32 u4VDecID, VDEC_INFO_H264_SLICE_HDR_T *prSliceHdr, BOOL fgIsIDRPic);
//void vInterpretBufferingPeriodInfo(UINT32 u4BSID,UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
void vRecoveryPoint(UINT32 u4BSID,UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
void vPicTiming(UINT32 u4BSID,UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
//void vInterpretFilmGrainCharacteristicsInfo(UINT32 u4BSID,UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
void vH264SetColorPrimaries(H264_DRV_INFO_T *prH264DrvDecInfo, UINT32 u4ColorPrimaries);
void vH264SetSampleAsp(H264_DRV_INFO_T *prH264DrvDecInfo, UINT32 u4H264Asp, UINT32 u4SarWidth, UINT32 u4SarHeight);
void vH264SetFrameTimingInfo(H264_DRV_INFO_T *prH264DrvDecInfo, UINT32 u4NumUnitsInTick, UINT32 u4TimeScale, BOOL fgIsFixFrm);
void vH264CCData(UINT32 u4BSID,UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
void vH264xvYCCData(UINT32 u4BSID,UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);

extern BOOL fgVDecH264FreeFBuf(H264_DRV_INFO_T *prH264DrvInfo, UINT32 u4FBufIdx);

#ifdef VDEC_SR_SUPPORT
extern void vH264InitDFBList(H264_DRV_INFO_T *prH264DrvInfo);
extern BOOL fgH264UpdDFBList(H264_DRV_INFO_T * prH264DrvInfo);
extern void vH264UpdDFBListIdxInfo(H264_DRV_INFO_T *prH264DrvInfo, UCHAR ucDPBFbId, UCHAR ucH264FBUFListIdx);
extern UINT32 u4H264GetMaxPOCBFBuf(H264_DRV_INFO_T *prH264DrvInfo);
extern void vH264EDpbPutBuf(H264_DRV_INFO_T *prH264DrvInfo,  UCHAR ucSrcDpbBuf);
extern UINT32 u4H264SROutputMaxPOCFBuf(VDEC_ES_INFO_T *prVDecEsInfo, UINT32 u4MinPOCFBufIdx);
extern void vH264InitFBufInfo(VDEC_INFO_H264_FBUF_INFO_T *prH264FbufInfo);
extern UINT32 i4H264OutputProcSR(VDEC_ES_INFO_T *prVDecEsInfo, UINT32 u4MinPOCFBufIdx);
extern UINT32 u4VDecGetMaxPOCFBuf(VDEC_ES_INFO_T *prVDecEsInfo, H264_DPB_SEARCH_T eDPBSearchType);
extern void vH264UpdWorkingAreaInfo(H264_DRV_INFO_T *prH264TarDrvInfo, UCHAR ucEDPBSize);

#endif

#ifdef DRV_VDEC_VDP_RACING
extern void vH264SetDpbBufferInfo(UCHAR ucEsId);
extern void vH264SetFBufInfoOnly(UCHAR ucEsId);
extern void vVDecClearExternalBufInfo(H264_DRV_INFO_T *prH264DrvInfo, UINT32 u4FbIdx);
#endif

extern void vH264SetCurrFrameBufferInfo(UCHAR ucEsId);

#ifdef MPV_DUMP_FBUF
extern void VDec_Dump_Data(UINT32 u4StartAddr, UINT32 u4FileSize, UINT32 u4FileCnt, UCHAR* pucAddStr);
#endif

#if VDEC_MVC_SUPPORT
extern BOOL fgIsMVCIDR(H264_DRV_INFO_T *prH264DrvInfo);
extern void vPrefix_Nal_Unit_Rbsp(UINT32 u4BSID, UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
extern void vSubset_Seq_Parameter_Set_Rbsp(UINT32 u4BSID, UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
void vH264OffsetMetadata(UINT32 u4BSID,UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
void vH264MVCScalableNesting(UINT32 u4BSID,UINT32 u4VDecID, H264_DRV_INFO_T *prH264DrvDecInfo);
#endif
#if MVC_PATCH_1
INT32 i4MVCHWPatch1(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm);
#endif

#ifdef DRV_VDEC_SUPPORT_FBM_OVERLAY
extern BOOL fgAVCNeedDoDscl(UCHAR ucEsId);
#endif

//Get mw frame rate for h264-ts file. Jie Zhang@20100806
extern UINT32 u4H264CheckFrameRate(UCHAR ucEsId);
extern BOOL fgH264IsSlideShow(UCHAR ucEsId);
BOOL fgH264CheckIsDivXPlus( UINT32 u4BSID, UINT32 u4VDecID, UCHAR ucEsId);
#endif
