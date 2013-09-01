
#ifndef _VDEC_DRV_WMV_INFO_H_
#define _VDEC_DRV_WMV_INFO_H_

//#include "x_os.h"
//#include "x_bim.h"
//#include "x_assert.h"
//#include "x_timer.h"

#include "drv_common.h"

#include "vdec_common_if.h"
#include "vdec_usage.h"

#include "vdec_info_wmv.h"
#include "vdec_info_common.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#endif

/******************************************************************************
* Local definition
******************************************************************************/
// WMV start code(s)
#define WMV_SC_PREFIX 0x000001
#define WMV_SC_ENDOFSEQ     0x10A
#define WMV_SC_SLICE        0x10B
#define WMV_SC_FIELD        0x10C
#define WMV_SC_FRAME        0x10D
#define WMV_SC_ENTRY        0x10E
#define WMV_SC_SEQ          0x10F
#define WMV_SC_SLICE_DATA   0x11B
#define WMV_SC_FIELD_DATA   0x11C
#define WMV_SC_FRAME_DATA   0x11D
#define WMV_SC_ENTRY_DATA   0x11E
#define WMV_SC_SEQ_DATA     0x11F

// extension start code IDs


// Working Buffer Define
#if (!CONFIG_DRV_VERIFY_SUPPORT)
  #if 1
#define Mv_1_SZ         ((1920/16)*(1088/16)*4*4)
#define Mv_2_SZ         ((1920/16)*4*4)
#define Bp_1_SZ         (16*(1088/16))
#define Bp_2_SZ         (16*(1088/16))
#define Bp_3_SZ         (16*(1088/16))
#define Bp_4_SZ         (16*(1088/16))
#define Mv_3_SZ         ((1920/16)*4*4)
#define Mv_1_2_SZ      ((1920/16)*(1088/16)*4*4)
#define DCAC_SZ           ((((1920 / 16) * 4) * (4)) * 4) // (MBx * 4) * (4) = 7680
#define DCAC_2_SZ        ((((1920 / 16) * 4) * (4)) * 4)
#define DEC_PP_SZ         ((1920/16)*3*16)
  #else
#define Mv_1_SZ         0x3fc00
#define Mv_2_SZ         0x3fc00
#define Bp_1_SZ         0x10000
#define Bp_2_SZ         0x10000
#define Bp_3_SZ         0x10000
#define Bp_4_SZ         0x10000
#define Mv_3_SZ         0x10000
#define Mv_1_2_SZ      0x10000
#define DCAC_SZ           ((((2048 / 16) * 4) * (4)) * 4) // (MBx * 4) * (4) = 7680
#define DCAC_2_SZ                0x100000
#define DEC_PP_SZ         ((2048/16)*9*8)*2
  #endif
#endif

#define INVALID_32          0xffffffff
#define NO_USE              0xffffffff
#define MAX_RETRY_COUNT1 200*1024
#define WMV_DPB_NUM 3
#define WMV_MAX_EDPB_NUM 30
#define WMV_DFB_NUM 256
#define WMV_MAX_SPS_NUM 30
#define WMV_MAX_EPS_NUM 30
#define WMV_MAX_ICOMPS_NUM 255

#define fgIsWMVSeqHdr(arg)  (arg & SEQ_HDR)
#define fgIsWMVEntryPTR(arg)  (arg & ENTRY_PTR)
#define fgIsWMVSeqEnd(arg)   (arg & SEQ_END)

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)  
#define WMV_NEW_HW_MODE   //the new HW work mode only supportted by MT8550^
#endif

typedef enum // bit plane coding mode
{
  NORM2 = 0,
  NORM6,
  ROWSKIP,
  COLSKIP,
  DIFF2,
  DIFF6,
  RAW,
}WMV_BP_MODE;

typedef enum
{
  WMV_DPB_STATUS_EMPTY = 0,   // Free
  WMV_DPB_STATUS_READY,         // After Get          
  WMV_DPB_STATUS_DECODING,   // After Lock                
  WMV_DPB_STATUS_DECODED,     // After UnLock
  WMV_DPB_STATUS_OUTPUTTED,     // After Output
  WMV_DPB_STATUS_FLD_DECODED,   // After 1fld UnLock
  WMV_DPB_STATUS_DEC_REF,
  WMV_DPB_STATUS_OUT_REF,  
}WMV_DPB_COND_T;

typedef enum
{
  WMV_DPB_FBUF_UNKNOWN = VDEC_FB_ID_UNKNOWN,
  WMV_DPB_FREF_FBUF = 0,
  WMV_DPB_BREF_FBUF = 1,                   
  WMV_DPB_WORKING_FBUF = 2,                   
}WMV_DPB_IDX_T;


typedef struct _WMV_DBP_INFO_T
{
    BOOL fgVirtualDec;
    UCHAR ucDpbFbId;
    WMV_DPB_COND_T eDpbStatus;
    UINT64 u8Pts;
    UINT64 u8Offset;	
}WMV_DBP_INFO_T;

typedef struct _WMV_DEC_PRM_T
{
    UCHAR ucDecFld;
    UCHAR uc2ndFld;   
    UCHAR ucPicStruct;
    UINT32 u4BitCount;
    BOOL    fgCounting;
    UINT32 u4PicHdrBits;
    INT32   i4CodecVersion;
    INT64 i8BasePTS;
    INT64 i8DiffCnt;
    INT64 i8DropIPCnt;
    INT64 i8DropBCnt;
    WMV_DPB_IDX_T eDpbOutputId;        
    WMV_DBP_INFO_T arWMVDpbInfo[WMV_DPB_NUM];     // 0: FRef, 1: BRef, 2:Working
    VDEC_NORM_INFO_T *prVDecNormInfo;
    VDEC_PIC_INFO_T *prVDecPicInfo;
    VDEC_PIC_INFO_T *prVPrevPicInfo;
    VDEC_FBM_INFO_T    *prVDecFbmInfo;
   // VID_DEC_PB_MODE_T    *prVDecPBInfo;
   // VID_DEC_SEQUENCE_INFO_T rCurrSeqInfo;
    
    VDEC_INFO_WMV_SEQ_PRM_T rSPS;
    VDEC_INFO_WMV_ETRY_PRM_T rEPS;
    VDEC_INFO_WMV_PIC_PRM_T rPPS;
    VDEC_INFO_WMV_ICOMP_PRM_T rICOMPS;
    
    VDEC_INFO_WMV_VFIFO_PRM_T rVDecWmvVFifoPrm;
    VDEC_INFO_WMV_BS_INIT_PRM_T rVDecWmvBsInitPrm[2];
    VDEC_INFO_WMV_WORK_BUF_SA_T rVDecWmvWorkBufSa;
    VDEC_INFO_WMV_ERR_INFO_T rVDecWmvErrInfo;
    VDEC_INFO_DEC_PRM_T rVDecNormHalPrm;
	
#ifdef VDEC_SR_SUPPORT
    VDEC_INFO_WMV_WORK_BUF_SA_T rVDecWmvWorkBufSa2;
    UCHAR ucEpsRefCnt;
    UCHAR ucWMVCurrSeqId;
    UCHAR ucWMVCurrEpsId;
    UCHAR ucWMVSeqHdrCnt;
    UCHAR ucWMVEpsHdrCnt;
    UCHAR ucWMVEpsCntOfSps;
    VDEC_INFO_WMV_SEQ_PRM_T rStoreSPS[WMV_MAX_SPS_NUM];
    VDEC_INFO_WMV_ETRY_PRM_T rStoreEPS[WMV_MAX_EPS_NUM];    
	
    WMV_DBP_INFO_T arWMVEDpbInfo[WMV_MAX_EDPB_NUM];     // 0: FRef, 1: BRef, 2:Working
    MPEG_DFB_INFO_T arWMVDFBInfo[WMV_DFB_NUM];
    UCHAR ucWMVCurrICOMPSId;
    VDEC_INFO_WMV_ICOMP_PRM_T rStoreICOMPS[WMV_MAX_ICOMPS_NUM];
    UCHAR arICOMPSIdxTable[WMV_DFB_NUM][3];
    VDEC_INFO_WMV_MV_BUF_SA_T rStoreMv[WMV_MAX_EDPB_NUM];
    VDEC_INFO_WMV_SR_STORE_PRM_T rStoreParam[WMV_DFB_NUM];
#endif	

    BOOL fgNoDecodeSpecial;
} WMV_DRV_INFO_T;

/******************************************************************************
* Proc Function prototype
******************************************************************************/

extern BOOL fgWMVChkRealEnd(WMV_DRV_INFO_T *prWmvDrvInfo);;

extern INT32 i4WMVParser(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VParseType, UCHAR ucEsId);

/******************************************************************************
* Local macro
******************************************************************************/

/******************************************************************************
* IF Function prototype
******************************************************************************/
extern void vWMVInitProc(UCHAR ucEsId);
extern INT32 i4WMVVParseProc(UCHAR ucEsId, UINT32 u4VParseType);
extern BOOL fgWMVVParseChkProc(UCHAR ucEsId);
extern INT32 i4WMVUpdInfoToFbg(UCHAR ucEsId);
extern void vWMVStartToDecProc(UCHAR ucEsId);
extern void vWMVISR(UCHAR ucEsId);
extern BOOL fgIsWMVDecEnd(UCHAR ucEsId);
extern BOOL fgWMVResultChk(UCHAR ucEsId);
extern BOOL fgIsWMVInsToDispQ(UCHAR ucEsId);
extern BOOL fgIsWMVGetFrmToDispQ(UCHAR ucEsId);
extern void vWMVEndProc(UCHAR ucEsId);
extern BOOL fgWMVFlushDPB(UCHAR ucEsId, BOOL fgWithOutput);    
extern void vWMVReleaseProc(UCHAR ucEsId, BOOL fgResetHW);
extern INT32 i4WMVOutputProc(VDEC_ES_INFO_T* prVDecEsInfo);
extern void vWMVSetDownScaleParam(WMV_DRV_INFO_T *prWmvDrvInfo, BOOL fgEnable);
extern void vWMVDpbBufCopy(WMV_DRV_INFO_T *prWmvDrvInfo, UCHAR ucTarDpbBuf, UCHAR ucSrcDpbBuf);
extern void vWMVSetMcBufAddr(UCHAR ucFbgId, VDEC_ES_INFO_T* prVDecEsInfo);
extern void vAllocWMVWorkingArea(/*VDEC_ES_INFO_T *prVDecEsInfo,*/ UCHAR ucEsId);
extern void vFreeWMVWorkingArea(VDEC_ES_INFO_T *prVDecEsInfo);
extern void vInitVParserWMV(WMV_DRV_INFO_T *prWmvDrvInfo);
extern VDEC_DRV_IF* VDec_GetWMVIf(void);
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
extern void vWMVSetLetterBoxParam(WMV_DRV_INFO_T *prWmvDrvInfo);
#endif

#ifdef MPV_DUMP_FBUF
extern void VDec_Dump_Data(UINT32 u4StartAddr, UINT32 u4FileSize, UINT32 u4FileCnt, UCHAR* pucAddStr);
#endif

#ifdef VDEC_SR_SUPPORT

//vdec_drv_wmv_if.c interface
extern BOOL fgWMVGenEDPB(UCHAR ucEsId);
extern BOOL fgWMVRestoreEDPB(UCHAR ucEsId, BOOL fgRestore);
extern BOOL fgIsWMVGetSRFrmToDispQ(UCHAR ucEsId, BOOL fgSeqEnd, BOOL fgRefPic);
extern void vWMVGetSeqFirstTarget(UCHAR ucEsId);
extern void vWMVReleaseSRDrvInfo(UCHAR ucEsId);
extern BOOL fgWMVGetDFBInfo(UCHAR ucEsId, void **prDFBInfo);
extern void vWMVRestoreSeqInfo(UCHAR ucEsId);
extern BOOL fgWMVRvsDone(UCHAR ucEsId);
extern void vWMVReleaseEDPB(UCHAR ucEsId);

//vdec_drv_wmv_proc.c interface


extern void vWMVEDpbPutBuf(VDEC_ES_INFO_T* prVDecEsInfo, WMV_DRV_INFO_T *prWMVDrvInfo,  UCHAR ucSrcDpbBuf);
extern void vWMVEDpbGetBuf(VDEC_ES_INFO_T* prVDecEsInfo, WMV_DRV_INFO_T *prWMVDrvInfo,  UCHAR ucTarDpbBuf);
extern INT32 i4WMVOutputProcSR(VDEC_ES_INFO_T* prVDecEsInfo);
extern void vWMVSetDFBInfo(VDEC_ES_INFO_T* prVDecEsInfo, BOOL fgIsRef);
extern void vWMVUpdPts(VDEC_ES_INFO_T* prVDecEsInfo, WMV_DRV_INFO_T *prWMVDrvInfo, BOOL fgRealOutput);
extern void vWMVUpdPtsSR(VDEC_ES_INFO_T* prVDecEsInfo, WMV_DRV_INFO_T *prWMVDrvInfo, BOOL fgRealOutput);

extern INT32 i4WMVOutputProcSR(VDEC_ES_INFO_T* prVDecEsInfo);
extern void vWMVSetDFBInfo(VDEC_ES_INFO_T* prVDecEsInfo, BOOL fgIsRef);
#endif

#ifdef DRV_VDEC_SUPPORT_FBM_OVERLAY
extern BOOL fgWMVNeedDoDscl(UCHAR ucEsId);
#endif


#endif
