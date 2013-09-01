
#ifndef _VDEC_DRV_RM_INFO_H_
#define _VDEC_DRV_RM_INFO_H_

//#include "x_os.h"
//#include "x_bim.h"
//#include "x_assert.h"
//#include "x_timer.h"

#include "drv_common.h"

#include "drv_imgresz.h"
#include "drv_imgresz_errcode.h"

#include "vdec_common_if.h"
#include "vdec_usage.h"

#include "vdec_info_rm.h"
#include "vdec_info_common.h"

/******************************************************************************
* Local definition
******************************************************************************/
#define RM_DPB_NUM 3

typedef enum
{
  RM_DPB_STATUS_EMPTY = 0,   // Free
  RM_DPB_STATUS_READY,         // After Get          
  RM_DPB_STATUS_DECODING,   // After Lock                
  RM_DPB_STATUS_DECODED,     // After UnLock
  RM_DPB_STATUS_OUTPUTTED,     // After Output
  RM_DPB_STATUS_FLD_DECODED,   // After 1fld UnLock
  RM_DPB_STATUS_DEC_REF,
  RM_DPB_STATUS_OUT_REF,  
}RM_DPB_COND_T;

typedef enum
{
  RM_DPB_FBUF_UNKNOWN = VDEC_FB_ID_UNKNOWN,
  RM_DPB_FREF_FBUF = 0,
  RM_DPB_BREF_FBUF = 1,                   
  RM_DPB_WORKING_FBUF = 2,                   
}RM_DPB_IDX_T;


typedef struct _RM_DBP_INFO_T
{
    BOOL fgVirtualDec;
    UCHAR ucDpbFbId;
    RM_DPB_COND_T eDpbStatus;
    UINT64 u8Pts;
    UINT64 u8Offset;
}RM_DBP_INFO_T;

typedef struct _RM_DEC_PRM_T
{
    BOOL fgRV40;                              // Is RV9_RV10 ro RV8
    BOOL fgGenRPRRefPic;
            
    UCHAR ucDecFld;
    UCHAR uc2ndFld;
    
    UINT32 u4BitCount;
    INT64 i8BasePTS;    
    INT64 i8LatestRealPTS;    
    INT64 i8DiffCnt;
    INT64 i8DropIPCnt;
    INT64 i8DropBCnt;
    INT64 i8DummyCnt;

    UINT64 u8PreFrmOffset;
    
    RM_DPB_IDX_T eDpbOutputId;        
    RM_DBP_INFO_T arRMDpbInfo[RM_DPB_NUM];     // 0: FRef, 1: BRef, 2:Working
    
    VDEC_NORM_INFO_T *prVDecNormInfo;
    VDEC_PIC_INFO_T *prVDecPicInfo;
    VDEC_FBM_INFO_T    *prVDecFbmInfo;    
    VID_DEC_PB_MODE_T    *prVDecPBInfo;
    
    VDEC_INFO_RM_VFIFO_PRM_T rVDecRMVFifoPrm;
    VDEC_INFO_RM_WORK_BUF_SA_T rVDecRMWorkBufSa;
    VDEC_INFO_RM_BS_INIT_PRM_T rVDecRMBsInitPrm[2];
    VDEC_INFO_RM_ERR_INFO_T rVDecRMErrInfo;
    VDEC_INFO_RM_PICINFO_T rVDecRMPicInfo;
    VDEC_INFO_RM_PICINFO_T rVDecRMNextPicInfo;
    VDEC_INFO_RM_PICINFO_T rVDecRMPrevPicInfo;
    
    VDEC_INFO_DEC_PRM_T rVDecNormHalPrm;

    IMGRESZ_DRV_TICKET_T rImgReszTicket;
    IMGRESZ_DRV_SRC_BUF_INFO_T rSrcBufInfo;
    IMGRESZ_DRV_DST_BUF_INFO_T rDstBufInfo;
    HANDLE_T hRmScaleEvent;
} RM_DRV_INFO_T;

/******************************************************************************
* Function prototype
******************************************************************************/

extern INT32 vRMParser(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VParseType, BOOL fgNextAUPic);
//extern INT32 vRMParser(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VParseType);
extern void vRMSetMcBufAddr(UCHAR ucFbgId, VDEC_ES_INFO_T* prVDecEsInfo);



/******************************************************************************
* Local macro
******************************************************************************/
#define INVERSE_ENDIAN(value)    	\
		(((value & 0xFF) << 24) + ((value & 0xFF00) << 8) + ((value & 0xFF0000) >> 8) + ((value & 0xFF000000) >> 24))

#define CCSIZE(wp, rp, bufsize) \
        (((wp) >= (rp)) ? ((wp) - (rp)) : (((bufsize) + (wp)) - (rp)))


#define INVERSE_BIT_ORDER_8(value)                                 \
{                                                                \
    UCHAR ucTemp = 0;                                            \
    INT32 i4_i;                                                  \
    for( i4_i = 0; i4_i<4; i4_i++)                               \
    {                                                            \
        ucTemp |= (value & (1 << i4_i)) << ((4-i4_i)*2 - 1);     \
    }                                                            \
    for( i4_i = 4; i4_i<8; i4_i++)                               \
    {                                                            \
        ucTemp |= (value & (1 << i4_i)) >> ((i4_i-4)*2 + 1);     \
    }                                                            \
    value = ucTemp;                                              \
}


/******************************************************************************
* Extern Function
******************************************************************************/
extern void vRMDpbBufCopy(RM_DRV_INFO_T *prRMDrvInfo, UCHAR ucTarDpbBuf, UCHAR ucSrcDpbBuf);
extern INT32 i4RMOutputProc(VDEC_ES_INFO_T* prVDecEsInfo);
extern void vFreeRMWorkingArea(VDEC_ES_INFO_T *prVDecEsInfo);

#ifdef RM_DRV_IMAGERESIZER_ENABLE
extern INT32 i4RMRPR_ResourceInit(RM_DRV_INFO_T *prRMDrvInfo);
extern INT32 i4RMRPR_ResourceFree(RM_DRV_INFO_T *prRMDrvInfo);
extern INT32 i4RMRPR_WaitFinish(RM_DRV_INFO_T *prRMDrvInfo);
extern INT32 i4RMRPR_SetTriggerScale(UCHAR ucEsId, RM_DRV_INFO_T *prRMDrvInfo);
#endif //RM_DRV_IMAGERESIZER_ENABLE

extern void vRMInitProc(UCHAR ucEsId);
extern INT32 i4RMVParseProc(UCHAR ucEsId, UINT32 u4VParseType);
extern BOOL fgRMVParseChkProc(UCHAR ucEsId);
extern INT32 i4RMUpdInfoToFbg(UCHAR ucEsId);
extern void vRMStartToDecProc(UCHAR ucEsId);
extern void vRMISR(UCHAR ucEsId);
extern BOOL fgIsRMDecEnd(UCHAR ucEsId);
extern BOOL fgIsRMDecErr(UCHAR ucEsId);
extern BOOL fgRMResultChk(UCHAR ucEsId);
extern BOOL fgIsRMInsToDispQ(UCHAR ucEsId);
extern BOOL fgIsRMGetFrmToDispQ(UCHAR ucEsId);
extern void vRMEndProc(UCHAR ucEsId);
extern BOOL fgRMFlushDPB(UCHAR ucEsId, BOOL fgWithOutput);    
extern void vRMReleaseProc(UCHAR ucEsId, BOOL fgResetHW);

extern VDEC_DRV_IF* VDec_GetRMIf(void);
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
extern void vRMSetLetterBoxParam(RM_DRV_INFO_T *prRMDrvInfo);
#endif


#endif  // _VDEC_DRV_RM_INFO_H_
