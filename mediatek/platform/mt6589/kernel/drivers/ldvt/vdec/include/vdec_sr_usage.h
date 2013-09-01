
#ifndef _VDEC_SR_USAGE_H_
#define _VDEC_SR_USAGE_H_

#include "drv_vdec.h"

#ifdef VDEC_SR_SUPPORT
#include "x_stl_lib.h"

#include "x_debug.h"
#include "x_assert.h"

#include "vdec_common_if.h"
#include "u_pbinf.h"
#include "vdec_usage.h"

#define SR_MAX_ES 2
#define MPV_REQ_SIZE 2
#define MPV_REQ_Q_SIZE 10
#define SRM_ESID    0
#define SR_ESID_0   2
#ifndef VDEC_OPTIMIZE_RESOURCE
#define SR_ESID_1   5
#else
  #define SR_ESID_1   3
#endif
#define SR_MAX_FB_NUM 30
#define SR_FB_RESERVED_NUM 4

#define MPEG_DFB_NUM 255

#define	VDEC_SR_FWD_STATE  0x1    
#define    VDEC_SR_RVS_STATE   0x2
#define    VDEC_SR_STOP_STATE 0x3
#define    VDEC_SR_IDLE_STATE  0x4

#define    VDEC_FWD_NONE_STATE               0x0
#define    VDEC_FWD_INIT_STATE                 0x1     
#define    VDEC_FWD_GET_NEXT_STATE        0x2
#define    VDEC_FWD_VPARSE_PROC_STATE  0x3
#define    VDEC_FWD_VPARSE_CHK_STATE    0x4
#define    VDEC_FWD_GET_FB_STATE            0x5
#define    VDEC_FWD_START_TO_DEC_STATE 0x6
#define    VDEC_FWD_DECODING_STATE        0x7
#define    VDEC_FWD_GEN_EDPB_STATE        0x8
#define    VDEC_FWD_DONE_STATE               0x9

#define    VDEC_RVS_NONE_STATE               0x0
#define    VDEC_RVS_INIT_STATE                  0x1
#define    VDEC_RVS_OUT_TARGET_STATE     0x2
#define    VDEC_RVS_GET_PREV_STATE         0x3
#define    VDEC_RVS_VPARSE_PROC_STATE   0x4
#define    VDEC_RVS_VPARSE_CHK_STATE     0x5
#define    VDEC_RVS_GET_FB_STATE             0x6
#define    VDEC_RVS_START_TO_DEC_STATE  0x7
#define    VDEC_RVS_DECODING_STATE         0x8   
#define    VDEC_RVS_GET_REF_STATE            0x9
#define    VDEC_RVS_OUT_FB_STATE             0xA
#define    VDEC_RVS_DONE_STATE                0xB

#define SR_INIT                 0x0

#define SR_GETPREV_0       0x1
#define SR_UPDINFO_0       0x2
#define SR_GETFB_0           0x3
#define SR_GETNEXT_0       0x4
#define SR_FWD_WAIT_0    0x5
#define SR_FWD_STOP_REQUEST_0 0x6
#define SR_FWD_DONE_0    0x7
#define SR_RVS_DONE_0     0x8

#define SR_GETPREV_1       0x11
#define SR_UPDINFO_1       0x12
#define SR_GETFB_1           0x13
#define SR_GETNEXT_1       0x14
#define SR_FWD_WAIT_1    0x15
#define SR_FWD_STOP_REQUEST_1 0x16
#define SR_FWD_DONE_1   0x17
#define SR_RVS_DONE_1    0x18

#define SR_REQ_NONE        0xFF
#define SR_MAX_IBC           30

#define fgIsReqByPts(eType)                    (eType == VID_DEC_SR_DATA_REQ_TYPE_PTS)
#define fgIsSR(u2SRState)                     (u2SRState==VDEC_SR_FWD_STATE || u2SRState==VDEC_SR_RVS_STATE)
#define fgIsSRFwdState(u2SRState)       (u2SRState==VDEC_SR_FWD_STATE)
#define fgIsSRRvsState(u2SRState)        (u2SRState==VDEC_SR_RVS_STATE)
#define fgSRActive(ucEsId,ucActiveId)   (ucEsId==ucActiveId)
#define ucEsIdToSrId(ucEsId) ((ucEsId == SR_ESID_0)? (0): (1))
#define ucSrIdToEsId(ucSrId) ((ucSrId == 0)? (SR_ESID_0) : (SR_ESID_1))

typedef enum
{
  VDEC_RVS_GET_UNKNOWN = 0,
  VDEC_RVS_GET_PREV_1 = 1,                     
  VDEC_RVS_GET_NEXT_1 = 2,   
  VDEC_RVS_GET_PREV_2 = 3,
}VDEC_RVS_GET_COND_T;

typedef enum
{
  VDEC_SR_STATE_PASS = 0,                   
  VDEC_SR_WAIT_1_TICK = 1,                     
  VDEC_SR_DROP_PIC = 2,     
  VDEC_SR_INIT_STATE_WAIT = 0x10,             
  VDEC_SR_GET_ESM_INFO_STATE_WAIT,             
  VDEC_SR_VPARSE_CHK_STATE_WAIT,
  VDEC_SR_CREAT_FBG_STATE_WAIT,
  VDEC_SR_GET_FB_STATE_WAIT,
  VDEC_SR_START_TO_DEC_STATE_WAIT,   
  VDEC_SR_DECODING_STATE_WAIT,           
  VDEC_SR_INS_TO_DISP_Q_STATE_WAIT,  
  VDEC_SR_FWD_DONE_STATE_WAIT,
  VDEC_SR_STATE_WAIT_MAX = 0xFF,
}VDEC_SR_COND_T;

typedef enum _VDEC_SR_EVENT_T
{
    VDEC_SR_EVENT_NONE               = 0,
    VDEC_SR_EVENT_CMD                 = 1 << 0,
    VDEC_SR_EVENT_AU_VPIC          = 1 << 1,
    VDEC_SR_EVENT_DEC_END          = 1 << 2,
    VDEC_SR_EVENT_GET_FB            = 1 << 3,
    VDEC_SR_EVENT_OUT_FB            = 1 << 4,
    VDEC_SR_EVENT_MAX               = 0xff,
} VDEC_SR_EVENT_T;

typedef struct _VDEC_REQ_INFO_T
{
    UINT64                                 ui8_target_picture; /* unit in PTS or offset */
    VID_DEC_SR_REBUF_TYPE_T     e_rebuf_type;       /* if the rebuffer request is from previous sequence */
    UINT32                                 ui4_num_of_seq;     /* number of sequence */    
}VDEC_REQ_INFO_T;

typedef struct _MPEG_DFB_INFO_T
{
    UCHAR ucFbId;
    UCHAR ucSeqId;
    UCHAR ucGopId;
    UCHAR ucGopRefCnt;
    BOOL   fgFrmType;
    BOOL   fgFrmDec;
    BOOL   fgFrmRef;
    BOOL   fg1stFldExist;
    BOOL   fg2ndFldExist;
    BOOL   fg1stFldDec;
    BOOL   fg1stFldRef;
    BOOL   fg2ndFldDec;        
    BOOL   fg2ndFldRef;
    BOOL   fgIsDecErr;
    UCHAR ucCurrRvsField;
    UINT64 u8OutFBPTS;
    UINT64 u8OutFBOft;
}MPEG_DFB_INFO_T;

typedef struct _VDEC_SR_INFO_T
{
    VID_DEC_SR_DATA_REQ_TYPE_T e_req_type;
    VDEC_RVS_GET_COND_T            e_rvs_get_cond;
    UCHAR  ucSRMId;
    UINT16  u2SRState;
    UINT16  u2FWDState;
    UINT16  u2RVSState;
    UINT64  u8Target;
    BOOL     fgNextSeq;
    BOOL     fgvideoNotExist;
    UINT64     u8NoVideoSeqOffset;
    BOOL     fgReleaseEDPB;
    UCHAR  ucIBCCnt;
    BOOL     fgIBCSent [SR_MAX_IBC];
    UINT32  u4DFBIdx;
    UCHAR  ucDpbSize;
    UCHAR  ucEdpbSize;    
    UCHAR  ucEdpbStartIdx;
    UCHAR  ucEdpbIdx;
    BOOL    fgEdpbFull;
    BOOL    fgEdpbEmpty;
    UCHAR  ucFBOccupied;
    BOOL    fgTargetFlag;
    BOOL    fgDecErrFlag;
    BOOL    fgOutputTarget;
    BOOL    fgOutputFlag;    
    //UCHAR  ucRefPicCnt;
    UINT64 u8SeqFirstTarget;
    UINT64 u8SeqFirstPts;
    UINT64 u8SeqFirstOffset;
    UINT64 u8EDpbFirstTarget;
    UINT64 u8EDpbFirstPts;
    UINT64 u8EDpbFirstOffset;
    UINT64 u8RequestPts;
    UINT64 u8RequestOffset;    
    UINT16 u2SeqPicCnt;
    BOOL    fgGetFirstPic;
    BOOL    fgForceReqPrevOne;
    INT64    i8EndTime;
    BOOL    fgStreamEnd;
    BOOL    fgNoPrev;
    BOOL    fgOpenGOP;
    BOOL    fgSeqComplete;
    BOOL    fgRvsEnd;
    BOOL    fgFRefEmpty;
    BOOL    fgSeqEndForceOut;
    UINT32 u4LastReadPtr;
    MPEG_DFB_INFO_T arMPEGDFBInfo[MPEG_DFB_NUM];
    BOOL    fgIsFirstPicIVOP;
    UINT64  u8FirstPts;
    UINT64  u8FirstOffset;
    INT64    i8EpmapPTS;
    BOOL     fgDecFlag;
    BOOL    fgFstPicIsNotI;
    IBC_PathInfo*   prIbc;
}VDEC_SR_INFO_T;

typedef struct _SR_IDX_INFO_T_ {
    UINT32 u4StartIdx;
    UINT32 u4CurrIdx;
    UINT32 u4EndIdx;
} SR_IDX_INFO_T;

typedef struct _VDEC_SRM_INFO_T
{
    VID_DEC_SR_DATA_REQ_TYPE_T e_req_type;    
    UINT32                                   ui4_fifo_size;
    BOOL                                     fgIsSR;
    VDEC_REQ_INFO_T                   rReqBufInfo;
    UINT16                                   u2ReqMode;
    UINT16                                   u2PrevReqMode;
    BOOL                                      fgOutputTarget;
    UCHAR                                    ucActiveId;   
    UINT16                                   u2SRFlag[2];
    UINT64                                   u8SRTarget;
    UINT64                                   u8Target[2];
    UCHAR                                    ucTotalFBNum;
    UINT16                                   u2SRState[2];
    SR_IDX_INFO_T                        e_seq_idx[2];    
    SR_IDX_INFO_T                        e_idx_info[2];    
    UCHAR                                    ucFBNumLeft[2];
    UINT32                                   u4StopReBufSrId;
    UINT32                                   u4StopReBufIdx;
    INT32                                    i4NextSRStartIdx;
}VDEC_SRM_INFO_T;

void VDec_SR_Init(void);
void VDec_SR_Termint(void);

//extern UCHAR ucEsIdToSrId(UCHAR ucEsId);
//extern UCHAR ucSrIdToEsId(UCHAR ucSrId);
extern UCHAR ucSrIdToVldId(UCHAR ucSrId);
extern BOOL   fgIs_SrState(UCHAR ucEsId);
extern BOOL   fgIs_SrFwdState(UCHAR ucEsId);
extern BOOL   fgIs_SrRvsState(UCHAR ucEsId);

extern void VDec_CreateSRCmdQ(UCHAR ucSrId);
extern void VDec_SetSRCmd(UCHAR ucSrId, UINT16 u2Mode);
extern void RetrieveSRCmd( UCHAR ucSrId);
extern void VDec_DeleteSRCmdQ(UCHAR ucSrId);
extern void VDEC_LockFBLeftSema(UCHAR ucSrId);
extern void VDEC_UnlockFBLeftSema(UCHAR ucSrId);

extern void VDec_CreateReqQ(void);
extern void VDec_SetSRReq(UCHAR ucEsId, UINT16 u2Mode);
extern void VDec_DeleteReqQ(void);
extern void VDEC_LockActiveSRSema(void);
extern void VDEC_UnlockActiveSRSema(void);

extern void VDec_SRModeProcess(UCHAR ucSrId);
VDEC_SR_COND_T VDec_FWDProc(UCHAR ucSrId);
VDEC_SR_COND_T VDec_RVSProc(UCHAR ucSrId);

extern BOOL fgVDecInitCodecProc(UCHAR ucEsId);
UINT32 u4IsNextPicExisted(UCHAR ucSrId, BOOL fgIsChkBefVParser);
UINT32 u4IsPrevPicExisted(UCHAR ucSrId, BOOL fgIsChkBefVParser);
BOOL fgSR_GetFBufGroup(  UCHAR ucSRMId, UINT8 *pu1FBGID);
BOOL  fgIsSRFBufAvailable(UCHAR ucSrId);
BOOL  fgSRFreeFB (UCHAR ucEsId, UCHAR ucFbId, BOOL fgRefType);
BOOL fgVDec_SR_Update_Esm(UCHAR ucSrEsId, BOOL fgNext);
UINT32 VDec_SR_Get_Idx(UCHAR ucSrEsId, BOOL fgNext);
void VDec_SR_Update_Esm_Seq_Done(UCHAR ucSrEsId);
void vVDec_SR_MoveRdPtr(UCHAR ucSrEsId, UINT32 u4NewAURdIdx);
void vVDec_SR_MoveWrPtr(UCHAR ucEsId, UINT32 u4LastWrIdx);
#endif

#endif
