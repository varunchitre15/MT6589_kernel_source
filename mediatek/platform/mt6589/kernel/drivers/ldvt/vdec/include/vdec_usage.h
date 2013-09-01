
#ifndef _VDEC_USAGE_H_
#define _VDEC_USAGE_H_

//#include "x_stl_lib.h"
//#include "x_assert.h"

#include "vdec_common_if.h"
//#include "u_pbinf.h"


#ifndef CONFIG_DRV_OMX_SUPPORT
#define CONFIG_DRV_OMX_SUPPORT 0
#endif

#if (1 == CONFIG_DRV_OMX_SUPPORT)
#include "omx_vdec_vconv.h"
#endif

#define HW_SEARCH_START_CODE   0

//#include "vdec_mpeg4_info.h"
//#include "vdec_wmv_info.h"
//#include "vdec_h264_info.h"

#define VDEC_DYNAMIC_ALLOC  1

//#define LOG(level, fmt...)			
//#define ASSERT(x)		((void)(x))
//#define VERIFY(x)		((void)(x))
#ifdef VDEC_SR_SUPPORT
#include "vdec_sr_usage.h"

#define MPV_CMD_SIZE 2
#define SR_GET_AU_FAIL                 0
#define SR_GET_AU_CMD                  1
#define SR_GET_AU_PIC                   2
#define SR_GET_AU_RVS_END          3
#define SR_GET_AU_CMD_SEQEND   4
#define SR_GET_CMD_NOT_SEND     5

#define MPEG2_MAX_SEQ_NUM  50
#define MPEG2_MAX_GOP_NUM  50
#endif

#define VDEC_LOG_TIME 0
#define VDEC_DRAM_BANDWIDTH_PEAK 0

typedef enum
{
    VDEC_LOG_DEFAULT      = 1 << 0,
    VDEC_LOG_FrameRate    = 1 << 1,
    VDEC_LOG_LASTIPTS     = 1 << 2,    
    VDEC_LOG_DROPPIC      = 1 << 3,
    VDEC_LOG_ASPECTRatio  = 1 << 4,
    VDEC_LOG_ESM2VDEC     = 1 << 5,
    VDEC_LOG_VDEC2VDP     = 1 << 6,
    VDEC_LOG_H264INOUT    = 1 << 7,
    VDEC_LOG_IBCCMD       = 1 << 8,
    VDEC_LOG_MPEG2        = 1 << 9,
    VDEC_LOG_PICINFO      = 1 << 10,
    VDEC_LOG_PBMODE       = 1 << 11,
    VDEC_LOG_MWGetSet     = 1 << 12,
    VDEC_LOG_LetterBox    = 1 << 13,
    VDEC_LOG_OMX          = 1 << 14,
    VDEC_LOG_STATEMACHINE = 1 << 15,
    VDEC_LOG_VP6          = 1 << 16,
    VDEC_LOG_LEVEL17      = 1 << 17,
    VDEC_LOG_LEVEL18      = 1 << 18,
    VDEC_LOG_LEVEL19      = 1 << 19,
    VDEC_LOG_LEVEL20      = 1 << 20,
    VDEC_LOG_LEVEL21      = 1 << 21,
    VDEC_LOG_LEVEL22      = 1 << 22,
    VDEC_LOG_LEVEL23      = 1 << 23,
    VDEC_LOG_LEVEL24      = 1 << 24,
    VDEC_LOG_LEVEL25      = 1 << 25,
    VDEC_LOG_LEVEL26      = 1 << 26,
    VDEC_LOG_LEVEL27      = 1 << 27,
    VDEC_LOG_LEVEL28      = 1 << 28,
    VDEC_LOG_LEVEL29      = 1 << 29,
    VDEC_LOG_LEVEL30      = 1 << 30,
    VDEC_LOG_LEVEL31      = 1 << 31     
}VDEC_LOG_LEVEL_T;

#if VDEC_MVC_SUPPORT
#define BASE_VIEW_ID   0
#define DEP_VIEW_ID   1

typedef enum
{
    VDEC_MVC_NONE = 0,     
    VDEC_MVC_BASE,
    VDEC_MVC_DEP,                
    VDEC_MVC_MAX = 0xFF,                     
}VDEC_MVC_INFO_T;

typedef enum
{
    MVC_CHK_WAIT_STATE,
    MVC_CHK_FIN_STATE,
    MVC_MAX = 0xFF,                     
}VDEC_MVC_STATE_T;
#endif

typedef enum
{
    VDEC_INIT_STATE = 0,     
    VDEC_GET_ESM_INFO_STATE,
    VDEC_VPARSE_PROC_STATE,                
    VDEC_VPARSE_CHK_STATE,                
    VDEC_START_TO_DEC_STATE,                      
    VDEC_DECODING_STATE,                
    VDEC_INS_TO_DISP_Q_STATE,
#if (defined(DRV_VDEC_VDP_RACING) || defined(VDEC_PIP_WITH_ONE_HW))    
    VDEC_DEC_DECISION_STATE,
    VDEC_GET_FB_STATE,
    VDEC_DEC_NOFBUF_STATE,
    VDEC_PREPARE_FRMBUF_STATE,    
    VDEC_PRE_OUTPUT_FRMBUF_STATE,    
#endif
#if VDEC_MVC_SUPPORT
    MVC_BASE_WAIT_DEP_OUT_STATE = 0x15,
    MVC_DEP_INIT_STATE = 0x20,
    MVC_DEP_WAIT_BASE_DEC_STATE = 0x25,
#endif
    VDEC_STATE_MAX = 0xFF,                     
}VDEC_STATE_T;

typedef enum
{
  VDEC_STATE_PASS = 0,                   
  VDEC_WAIT_1_TICK = 1,                     
  VDEC_DROP_PIC = 2,     
  VDEC_INIT_STATE_WAIT = 0x10,             
  VDEC_GET_ESM_INFO_STATE_WAIT,             
#ifdef VDEC_PIP_WITH_ONE_HW
   VDEC_DEC_DECISION_STATE_WAIT,             
#endif
  VDEC_VPARSE_CHK_STATE_WAIT,             
  VDEC_START_TO_DEC_STATE_WAIT,   
  VDEC_DECODING_STATE_WAIT,           
  VDEC_INS_TO_DISP_Q_STATE_WAIT,  
#ifdef VDEC_SR_SUPPORT
  VDEC_SR_REQ_STATE_WAIT,    
#endif
#if VDEC_MVC_SUPPORT
  MVC_BASE_DEP_STATE_WAIT,    
  MVC_DEP_INIT_STATE_WAIT,    
  MVC_DEP_BASE_STATE_WAIT,
#endif
    VDEC_GET_ESM_INFO_1_TICK_WAIT,
  VDEC_STATE_WAIT_MAX = 0xFF,                     
}VDEC_COND_T;

typedef enum _VDEC_EVENT_T
{
    VDEC_EVENT_NONE               = 0,
    VDEC_EVENT_CMD                 = 1 << 0,
    VDEC_EVENT_AU_VPIC          = 1 << 1,
    VDEC_EVENT_DEC_END          = 1 << 2,
    VDEC_EVENT_GET_FB            = 1 << 3,
    VDEC_EVENT_OUT_FB            = 1 << 4,
#ifdef VDEC_SR_SUPPORT
    VDEC_EVENT_REQ_WAIT        = 1 << 5,
    VDEC_EVENT_CREAT_FBG       = 1 << 6,
    VDEC_EVENT_FWD_WAIT        = 1 << 7,
    VDEC_EVENT_SR0_STOP_WAIT        = 1 << 8,
    VDEC_EVENT_SR1_STOP_WAIT        = 1 << 9,
#endif
#if VDEC_MVC_SUPPORT
    VDEC_EVENT_BASE_WAIT        = 1 << 10,
    VDEC_EVENT_DEP_WAIT        = 1 << 11,
    VDEC_EVENT_DEP_STOP_WAIT    = 1 << 14,
#endif
#ifdef VDEC_PIP_WITH_ONE_HW
    VDEC_EVENT_GET_HW             = 1 << 12,
#endif
#if (CONFIG_DRV_SUPPORT_DVD_AUDIO)
    VDEC_EVENT_WAIT_NEXTPIC     = 1 << 13,
#endif
    VDEC_EVENT_MAX               = 0xff,
} VDEC_EVENT_T;

typedef enum _VDEC_ERR_DROP_LEVEL_T{ 
    VDEC_ERR_DROP_ALL = 0,  /* Level 0: Drop frame if there is error. (Default) */
    VDEC_ERR_DROP_I,            /* Level 1: Ignore P or B frame's error.  If I-VOP is error, then, drop I-VOP. */
    VDEC_ERR_DROP_NONE      /* Level 2: Show every pictures. (I, P, B) */
} VDEC_ERR_DROP_LEVEL_T;

typedef struct _VDEC_COMM_INFO_T
{
    UCHAR   ucVDecErrThd;
} VDEC_COMM_INFO_T;

typedef struct _VDEC_ESM_INFO_T
{
    UCHAR ucMpvId;                       ///<   
    UINT32 u4PicRdIdx;                           
//    IBC_PathInfo *prIBCPathInfo;         // Add only for DRV_IBC_InbandCmdTypeFirstPictureDisplay
    UINT32 pu4Handle;                       ///< [OUT] interface handle
//    Decoder_OpIf* pprDecoderOplIf;    ///< [OUT] Interface for decoder operation 
}VDEC_ESM_INFO_T;

typedef struct _VDEC_FBM_INFO_T
{
    UCHAR ucFbgId;
    UCHAR ucFbgFbNum;
    UCHAR ucFbgDpbNum;
    UCHAR ucFbId;
    UCHAR ucFbDbId;
    UCHAR ucVDSCLFbId; 
    BOOL  fgOnlyNeedFbDb;
    UINT32 u4VDSCLWorkAddr;
#ifdef VDSCL_SIZE_LIMIT_SUPPORT
    UINT8 ucVDSCLMaxFbNum;
#endif
#ifdef VDEC_SR_SUPPORT    
    UCHAR ucVDSCLFbId2;    
    UINT32 u4VDSCLWorkAddr2;
#endif
    UINT32 u4FbgWidth;
    UINT32 u4FbgHeight;
    UINT32  u4FbgAllocWidth;        
    UINT32 u4WaitDispTime;		//    UINT32 u4WaitDispTime;                 
//    VDSCL_INFO_T rVdsclInfo;
    UINT32  u4VdsclFbWidth;
//    FBM_CB  rFbmCCCb;
//    FBM_CB  rFbmCb;
//    FBM_CB  rFbmGetFBCb;
//    FBM_CB  rFbmOutFBCb;
//    FBM_FRAMEINFO *parFbmFrameInfo;
#if  (defined(DRV_VDEC_VDP_RACING) || defined(VDEC_PIP_WITH_ONE_HW))  
    UCHAR ucPrevFbId;
#endif    
}VDEC_FBM_INFO_T;

#if 0
typedef struct _VDEC_NOTIFY_INFO_T
{
    void* pvPtsNfyTag;
    PFN_PTS_NFY_FCT pfPtsNfyFct; 
    void* pvInpNfyTag;
    PFN_ACQUIRE_FCT pfAcquireNfy; 
    PFN_RELEASE_FCT pfReleaseNfy; 
}VDEC_NOTIFY_INFO_T;
#endif

typedef struct _VDEC_SYNC_INFO_T
{
  UCHAR ucSyncMode;
  UCHAR ucStcId;
  BOOL fgSetStartPts;
}VDEC_SYNC_INFO_T;

typedef struct _VDEC_EC_INFO_T
{
  UCHAR ucECMethod;
  UINT32 u4ErrThrsd;
  BOOL fgDetectRefLost;
}VDEC_EC_INFO_T;

#if 0
typedef struct _VDEC_PB_INFO_T
{
  UINT32 u4DecPicType;
  UINT32 u4DecSpeed;
    MPV_FRAME_ACCURATE_TYPE_T       e_frame_accurate_type;
    union 
    {        
        MPV_FRAME_ACCURATE_PTS_INFO_T         t_frame_accurate_pts;
        MPV_FRAME_ACCURATE_NUM_INFO_T         t_frame_accurate_num;    
    } u; 
}VDEC_PB_INFO_T;
#endif

typedef struct _VDEC_MW_INFO_INFO
{
    UINT64  u8LatestIPicOffset;
    UINT64  u8LatestIPicPts;    
    UINT64  u8LatestGopHdrOffset;
    UINT64  u8LatestSeqHdrOffset;    
//    VID_DEC_INQUIRY_TYPE_T eInqType;
//    VID_DEC_TIME_CODE_INFO_T tTimeCode;
//    VID_DEC_SEQUENCE_INFO_T tSeqInfo;
}   VDEC_MW_INFO_T;

typedef struct _VDEC_COMP_INFO_T
{
    UINT16  u2CompId;
    UINT16  u2CompType;
    UINT16  u2FilterId;
    UINT16  u2FilterType;
#ifdef VDEC_SR_SUPPORT
    void    *pvFilterIf;
    void    *pvFilterTag;
#endif
    UINT16  u2SyncCtrlId;
    UINT16  u2SyncCtrlType;
    void    *pvSyncCtrlIf;  
    void    *pvSyncCtrlTag;    
    UINT16  u2VdpId;
    UINT16  u2VdpType;
    void    *pvVdpIf;    
    void    *pvVdpTag;    
}   VDEC_COMP_INFO_T;


typedef struct _VDEC_PIC_INFO_T
{
    UCHAR  ucPicStruct;
    UINT32  u4PicType;
    UINT32  u4PicWidth;
    UINT32  u4PicHeight;    
    UINT32  u4PicAllocWidth;
    UINT32  u4PicAllocHeight;    
    UINT64  u8DTS;
    UINT64  u8PTS;
    UINT64  u8Offset;    
  //  PBINF_V rPbInf;               ///< Playback information
    UINT32  u4DecAddrY;    
    UINT32  u4DecAddrC;
    UINT32  u4VdsclAddrY;    
    UINT32  u4VdsclAddrC;
    UINT32  u4FifoStart;  
    UINT32  u4FifoEnd;    
    UINT32  u4VldReadPtr;    
    UINT32  u4VldWritePtr; 
    UINT32  u4PicEnd;
    UINT32  u4VC1CCSa;
    UINT32  u4CCIdx;
    UCHAR   arCCType[MAX_CC_DATA];
    UINT16 arCCData[MAX_CC_DATA];
    UINT32  u4PicFlag;
    BOOL fgVirtaulDec;    
    BOOL fgForceVirtual;
//    DiscType eDiscType;
    UINT32 u4Duration;
#if (CONFIG_DRV_SUPPORT_DVD_AUDIO)
    BOOL fgBlkFrm;
#endif
#if  (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)  
    UINT32 u4WMVSliceAddr[3];
#endif
    UINT32 u4xvColorR;
    UINT32 u4xvColorG;
    UINT32 u4xvColorB;
//    VDEC_CUR_PIC_SPECIAL_INFO_T rSpecialInfo;

    UINT32 u4RMSliceNum;
    UINT32 auRM4SliceSize[128];
    UINT32 u4EsdIndex;
    UINT32 u4EsdNums;
}VDEC_PIC_INFO_T;

typedef struct _VDEC_IFRAME_INFO_T
{
    BOOL fgVDecNeeded;
    UCHAR *pucIFrmVFifoPtr;
    UINT32  u4IFrmSize;
    UINT32 u4WrIdx;                       ///< [OUT] interface handle
    UINT32 u4Handle;                       ///< [OUT] interface handle
//    Filter_OpIf* pprFilterOpIf;
}   VDEC_IFRAME_INFO_T;


typedef struct _VDEC_NORM_INFO_T
{
//    VDEC_CODEC_T   eVdecCodecType;    
    UCHAR  ucEsId;
    UCHAR  ucVldId;
    UCHAR  ucBsId;    
    UCHAR  ucAddrSwapMode;
    BOOL fgDeblocking;
    BOOL fgDeblockingDemo;
    UINT8 u1DeblockLevel;
    UINT8 au1MBqp[4];
    BOOL fgForceDropLevl;
    VDEC_ERR_DROP_LEVEL_T eVDecDropLevl;
    BOOL fgErrRefExist;
#ifdef ENUM_SRC_ASPECT_RATIO
#else
    UCHAR  ucPicAsp;
#endif
    UCHAR  ucFrameRate;
    UCHAR ucLastPicTp;
    UCHAR ucLastPicStruct;
    BOOL  fgNewPic;
    BOOL fgSeqChg;
    UCHAR  ucHScale;
    UCHAR  ucVScale;
    UINT16 u2LineSize;				// horizontal line size, due to block color mode
    UINT64  u8LatestDecPTS;
    UINT64  u8LatestOutPTS;
    UINT64  u8LastIPTS;
    UINT64  u81stIPTS;
    UINT64  u8PTSOffset;
    UINT32  u8LastIOffset;
//    PBINF_V rLastIPbInf;
    UINT32  u4LatestFrameRate;
    UINT32 u4DisplayRef;
#if 0//def FBM_DIGEST_MODE
    UINT16 u2DigestLineSize;		
    UINT16 u2DigestHSize;     		
    UINT16 u2DigestVSize;      	
#endif
    UINT32  u4PicSize;    
    UINT32  u4SampleWidth;    
    UINT32  u4SampleHeight;        
    UINT32  u4FrameTimingInfo;        
    UINT32  u4FrameDuration;        
    UINT32  u4ColorPrimaries;    

    UINT32  u4DecStatus;    
    UINT32  u4DecFlag;
    UINT32  u4EventRec;
    UINT32 u4DecReadPtr;    

    INT32 i4VParseErrCode;
    INT32 i4VDecodeErrCode;    
    UINT32 u4PrsHdrType;  
    UINT32 u4RefFrmCnt;  
    
    INT32  i4RemainNum;
    UINT32  u4ExpRemNum;
    
    UINT32  u4FFDivisor;
    UINT32  u4FFRemainder;
    UINT32  u4FFTargetCnt;
    UINT32  u4FFDivisorCnt;
    UINT32  u4FFRemainderCnt;

#if VDEC_MVC_SUPPORT
    VDEC_MVC_INFO_T eVDecMvcInfo;
    UINT32  u4MVCStatus;
    BOOL fgMVCDecErr;
    UINT32 u4MVCErrCnt;
#endif
    VDEC_EVENT_T eVDecEvent;
    UINT64 u8PreRefPTS;        

//    VDEC_PATCH_T eVDecPatchChk;
    BOOL    fgIsProgressive;    
    BOOL   fgIsVDPRacing;
    BOOL    fgIsOnlyOneHW;
#if (1 == CONFIG_DRV_OMX_SUPPORT)
    BOOL    fgOmxInit;
#endif

#if (CONFIG_DRV_SUPPORT_DVD_AUDIO)
    INT32 i4AUTargetIdx;
    UINT32 u4FrameOffset;
    BOOL fgQueueBlkFrm;
#endif
//    VID_DEC_DISC_TYPE_T u4DiscType;
    UCHAR ucVDecFrameRate;    
    UINT64 u8NextPts;
}VDEC_NORM_INFO_T;

typedef struct _VDEC_LOG_INFO_T
{
    UINT32 u4FilterDelayCnt;        
    UINT32 u4FbmDelayCnt;            
    UINT32 u4DecTimeoutCnt;
    UINT32 u4MaxDecSec;
    UINT32 u4MaxDecMic;
    UINT32 u4MaxDecPicCnt;
    UINT32 u4LatestRPtr;
    UINT32 u4PrintVideoDataOffset;
    UINT32 u4PrintVideoDataLength;
    BOOL fgForNoUpdRdPtr;
    BOOL fgShowPTSInfo;      
    BOOL fgDramDump;         //Dump dram flag.
    BOOL fgDumpGroupPic;
    BOOL fgDumpGroupVFifo;  //Dump group video vfifo data flag.
    BOOL fgDumpAllVFifo;       //Dump all video vfifo data flag.
    BOOL fgDumpBeforeVParse; //Dump video vfifo data before VParse
    BOOL fgDumpAfterDecode;   //Dump video vfifo data after decode
    BOOL fgPrintVideoData;

    UINT32 u4EmptyReportFreq;    
}VDEC_LOG_INFO_T;

typedef struct _VDEC_BITRATE_INFO_T
{
    UINT32 u4BitrateCntFldNum;        
    UINT32 u4BitrateCntTotalPicLength;
    UINT32 u4BitrateData;
    UINT32 u4BitrateCntDuration;
    UINT64 u8BitratePTSBase;
}VDEC_BITRATE_INFO_T;

#ifdef VDEC_SR_SUPPORT
#define VDEC_MAX_RECORD_ASP_CNT 10
#endif

typedef struct _VDEC_ASP_INFO_T
{
//    SOURCE_ASPECT_RATIO_T eLine23Asp;
    UINT64 u8ICBAspPTS;           
//    SOURCE_ASPECT_RATIO_T eIBCAsp;        
//    SOURCE_ASPECT_RATIO_T eMWAsp;        
//    SOURCE_ASPECT_RATIO_T eCodecAsp;        
//    SOURCE_ASPECT_RATIO_T eCurrAsp;     
    #ifdef VDEC_SR_SUPPORT
//    SOURCE_ASPECT_RATIO_T aeRecordAsp[VDEC_MAX_RECORD_ASP_CNT];
    UINT64 au8AspOffset[VDEC_MAX_RECORD_ASP_CNT];
    UINT8 u1RecordAspIdx;
    #endif
}VDEC_ASP_INFO_T;

typedef struct _VDEC_DEC_RES_INFO_T
{
    UINT32 u4DecResHandle;
}VDEC_DEC_RES_INFO_T;

typedef struct _VDEC_LOG_PIC_INFO_T
{
    INT32   i4ShowErrLevel;
    UINT32  u4LastErrCount;
    UINT64  u8DumpPicPTS;
    UINT64  u8DumpPicOffset;
    UINT64  u8DumpGroupPicStartPTS;
    UINT64  u8DumpGroupPicEndPTS;
    UINT64  u8DumpGroupPicStartOffset;
    UINT64  u8DumpGroupPicEndOffset;
    UINT64  u8DumpGroupVFifoStartPTS;      
    UINT64  u8DumpGroupVFifoEndPTS;
    UINT64  u8DumpGroupVFifoStartOffset;
    UINT64  u8DumpGroupVFifoEndOffset;
    UINT32  u4PosixMem;
    UINT32  u4PosixMemSize;
//    HANDLE_T h_YFileHandle;
//    HANDLE_T h_CFileHandle;
//    HANDLE_T h_VFifoHandle[2];
}VDEC_LOG_PIC_INFO_T;

#ifdef DRV_VDEC_VDP_RACING
typedef struct _VDEC_PIP_INFO_T
{
    BOOL     fgGetAU;
    BOOL     fgDecoding;
    BOOL     fgWaitForHw;
    BOOL     fgNeedDec2ndField;
    BOOL     fgNoFbReParse;
    BOOL     fgNoFbDec;
    BOOL     fgIsCurrParsing;
}VDEC_PIP_INFO_T;
#endif

typedef struct _VDEC_DBGLOG_INFO_T
{
    BOOL fgParsing;
    BOOL fgParsChk;
    BOOL fgStartDec;
    BOOL fgGetFB;
    BOOL fgTriggerHW;
    BOOL fgResultChk;    
    BOOL fgClrFlag;
}VDEC_DBGLOG_INFO_T;

#if VDEC_MVC_SUPPORT
#define MAX_OFT_SEQ 256
#define MAX_GOP_NUM 120
typedef struct _VDEC_OFFSET_METADATA_INFO_T
{
    UINT8 u1Frame_rate;
    UINT64 u8Pts;
    UINT64 u8MaxPts;
    UINT8   u1NuSeq;
    UINT8   u1NumFrame;
    UINT8 *pucOFTInfoAddr;
}VDEC_OFFSET_METADATA_INFO_T;
#endif

#if (1 == CONFIG_DRV_OMX_SUPPORT)
typedef struct _OMX_VDEC_INFO
{
    UINT32 u4SA; /* start address */
    UINT32 u4Used;
    UINT32 u4Size; /* OMX_TOTAL_MEM_SIZE */
    INT32  i4FifoRef; /* Reference cnouter */
    vOmxVdecVConvCbFunc pvCbFunc;
    void *pvCbPrivData;
    VCodeC eCodeCType;
    VCONV_VDEC_OMX_NFY_PARAM rNfyParam;
    VCONV_VDEC_PIC_INFO_T rCurrPicInfo;
    BOOL   fgStopCmd;
    BOOL   fgEOSCmd;
} OMX_VDEC_INFO_T;
#endif

typedef struct _VDEC_ES_INFO_T
{
    VDEC_STATE_T   eVdecState;
//    DRV_FBTYPE eVDecFBType;    //Default: FBT_420_BK_YCBIND, MJPEG: FBT_420_RS
    UCHAR  ucBSNum;    
    UINT16  u2DecMode;
    UINT16  u2PreMode;
    UINT32  u4PicCnt;
    UINT32  u4DecodedPicCnt;
    UINT32  u4ErrPicCnt;
    UINT32  u4DropPicCnt;    
    UINT32  u4OutputPicCnt;
    UINT32  u4RealOutputPicCnt;
    UINT64  u8StartPTS;
    UINT32  u4CalcCount;    
    UINT32  u4FrmCnt;
    UCHAR aucMatrix[MPV_MATRIX_RAW_SIZE];
    VDEC_NORM_INFO_T   rVDecNormInfo;
    VDEC_PIC_INFO_T   rVDecPrevPicInfo;
    VDEC_PIC_INFO_T   rVDecCurrPicInfo;
    BOOL fgIsNextPicInfoExisted;
    VDEC_PIC_INFO_T   rVDecNextPicInfo;       
    VDEC_FBM_INFO_T    rVDecFbmInfo;
    //VDEC_NOTIFY_INFO_T    rVDecNotifyInfo;    
    VDEC_SYNC_INFO_T    rVDecSyncInfo;    
    VDEC_EC_INFO_T    rVDecECInfo;        
    VDEC_MW_INFO_T   rVDecMwInfo;
    VDEC_IFRAME_INFO_T rVDecIFrmInfo;
    VDEC_LOG_INFO_T rVDecLogInfo;
    VDEC_BITRATE_INFO_T rVDecBitRateInfo;
    VDEC_ASP_INFO_T rVDecAspInfo;
//    VID_DEC_PB_MODE_T    *prVDecPBInfo;
#ifdef VDEC_SR_SUPPORT
    VID_DEC_PB_MODE_T      rVDecNewPBInfo;
#endif
    VDEC_COMP_INFO_T *prVDecCompInfo;
    VDEC_DRV_IF *prVDecDrvIf;
    UINT32 *prVDecDrvInfo;
#if VDEC_DYNAMIC_ALLOC
    UINT32** ppu4VDecDrvInfoPoint;
#endif
//    VID_DEC_SEQUENCE_INFO_T rVDecSeqInfo;
#ifdef VDEC_SR_SUPPORT
    VDEC_SRM_INFO_T rVDecSRMInfo;
    VDEC_SR_INFO_T rVDecSRInfo;
    UCHAR aucStoreMatrix[MPEG2_MAX_SEQ_NUM][MPV_MATRIX_RAW_SIZE];
#endif
    VDEC_DEC_RES_INFO_T rVDecDecResInfo;
    BOOL fgReDecode;
    BOOL fgPipExist;
    UCHAR ucReDecCnt;
    UINT32 u4TotalReDecCnt;
    UINT32 u4MaxReDecCnt;
#if  (defined(DRV_VDEC_VDP_RACING) || defined(VDEC_PIP_WITH_ONE_HW))
    BOOL   fgOutputCurrFBuf;
    VDEC_PIP_INFO_T   rVDecPipInfo;
#endif
#if VDEC_MVC_SUPPORT
   VDEC_OFFSET_METADATA_INFO_T rVDecOMTData;
//   VID_VDEC_3D_SOURCE_INFO_T rVDec3DSourceInfo;
#endif   
    INT32  u4LogIdx;
    VDEC_DBGLOG_INFO_T rVDecDbgLog[100];

#if VDEC_LOG_TIME
    UINT32 u4Over33Cnt;
#endif
   UINT64 u8PTSOffset;
   //BOOL fgIdleState;

#ifdef PHILIPS_PQL_SUPPORT
    UINT32 u4QuanCntFldNum;
    UINT32 u4QuantizationSum;
    UINT8   u1QuantizationAvg;
    BOOL fgGeneratedSrcStaAttr;
    UCHAR ucSrcStaticAttr[5];
    UCHAR ucSrcDynamicAttr[2];
#endif
} VDEC_ES_INFO_T;

typedef struct _VDEC_MODELOG_INFO_T
{
    UINT32  u4LogLevel;
//    VID_DEC_CTRL_T  eDecCtrl;
    BOOL    fgDecModeFin;
    UINT16  u2DecMode;
//    VID_DEC_SPEED_TYPE_T    e_speed_type;
    UINT16  u2DesiredWidth; 
    UINT16  u2DesiredHeight;
    UINT16  u2OriginalWidth;
    UINT16  u2OriginalHeight;
}VDEC_MODELOG_INFO_T;

typedef struct _VDEC_INFO_T
{
    UCHAR          ucMode;
//    BOOL            fgCheck[MPV_MAX_ES];
    //x_vid_dec_nfy_fct pfErrNotify;
#ifdef VDEC_UNIFORM_CB_SUPPORT
    VID_DEC_CB_DATA rVDecCbData;
    DRV_CB_REG_INFO_T rVDecCbInfo[MPV_MAX_ES];
#else
//    void* pvDecNfyTag[MPV_MAX_ES];
//    x_vid_dec_nfy_fct pfDecNfyFct[MPV_MAX_ES];
#endif    
} VDEC_INFO_T;

typedef struct _VDEC_EVENT_INFO
{
//    HANDLE_T hEvent;            ///< Event handle
    VDEC_EVENT_T eVDecPreEventWait;
    VDEC_EVENT_T eVDecWaitFor;
}VDEC_EVENT_INFO;


// *********************************************************************
// MPV Structure define
// *********************************************************************

typedef struct _CC_INFO_T
{
	UINT32    u4BufRptr;
	UINT32     u4Size;    	
//	PTS_T      u8Pts;    
	BOOL	fgIsScte;
} CC_INFO_T;


typedef struct _CC_BUF_INFO_T
{
	UINT32		u4Rptr;
	UINT32		u4Wptr;

}CC_BUF_INFO_T;


typedef struct _MPV_CC_T
{
	BOOL fgPlayCc;
	BOOL fgPlayAnlgCc;
	BOOL   fgCcIPTmpIsScte;	
	BOOL fgCcPreRef;	
	UCHAR* pucCcBuf;
	UCHAR* pucAnlgCcBuf;			
	UCHAR* pucCcTmpBuf;
	UCHAR* pucCcAtscIPTmpBuf;
	UCHAR* pucCcScteIPTmpBuf;	
	UINT32 u4CcBufSize;	
	UINT32 u4AnlgCcBufSize;			
	UINT32 u4AtscIPTmpSize;		
	UINT32 u4AtscIPTmpPts;
	UINT32 u4ScteIPTmpSize;		
	UINT32 u4ScteIPTmpPts;	
	CC_INFO_T rCcInfo;
	CC_INFO_T rAnlgCcInfo;	
	CC_BUF_INFO_T rCcBufInfo;	
	CC_BUF_INFO_T rAnlgCcBufInfo;		
//	HANDLE_T hCcQueue;
//	HANDLE_T hAnlgCcQueue;
//	PFN_CC_NFY_FCT pfCcNfyFct; 
//	PFN_CC_NFY_FCT pfAnlgCcNfyFct; 	
	void* pvCcNfyTag;
	void* pvAnlgCcNfyTag;
	
} MPV_CC_T;

// ESM Queue
typedef struct _ESMQ_T
{
	UCHAR					ucDropBNs;
	UCHAR					ucStarted;

	UINT16					u2Count;
	UINT16 					u2UdfNs;
//	HANDLE_T				hMsgQueue;
//	HANDLE_T				hUnderFlowSema;
//	HANDLE_T				hCmdQueue;
//	HANDLE_T				hMutex;

} ESMQ_T;

typedef struct _VDEC_IBCMD_INFO
{
    BOOL     fgSeqEnd;
    BOOL     fgvideoNotExist;
    BOOL     fgNextSeq;
    UINT64  u8NoVideoSeqOffset;
    BOOL     fgForceReqPrevOne;
    BOOL     fgStreamEnd;
    BOOL     fgNoPrev;    
//    INT64     i8EndTime;
    BOOL     fgCmdSendFail;
    BOOL     fgCmdAsp;
//    INT64    i8EpmapPTS;
}VDEC_IBCMD_INFO;

#if VDEC_LOG_TIME
#define VDEC_START_TIME_LOG   TRUE
#define VDEC_END_TIME_LOG       FALSE
#define VDEC_START_WAIT_LOG   TRUE
#define VDEC_END_WAIT_LOG       FALSE

typedef enum _VDEC_PIP_TIME_TYPE_T
{
    VDEC_PIP_TIME_NONE               = 0,
    VDEC_PIP_TIME_AU                   ,
    VDEC_PIP_TIME_AU_DD             ,
    VDEC_PIP_TIME_DD                   ,
    VDEC_PIP_TIME_PRS                  ,
    VDEC_PIP_TIME_PCK                  ,
    VDEC_PIP_TIME_GFB                 ,    
    VDEC_PIP_TIME_STD                 ,
    VDEC_PIP_TIME_RSC                   ,
    VDEC_PIP_TIME_INS                   ,
    VDEC_PIP_TIME_PDQ               ,
    VDEC_PIP_TIME_PQT               ,
    VDEC_PIP_TIME_PINS               ,
    VDEC_PIP_TIME_PPDQ               ,
    VDEC_PIP_TIME_NOFB               ,
    VDEC_PIP_TIME_MAX               = 0xff,
} VDEC_PIP_TIME_TYPE_T;

typedef enum _VDEC_PIP_WAIT_TYPE_T
{
    VDEC_PIP_WAIT_NONE               = 0,
    VDEC_PIP_WAIT_HW                   ,
    VDEC_PIP_WAIT_FB                   ,
    VDEC_PIP_WAIT_DC                   ,
    VDEC_PIP_WAIT_NOFB                   ,
    VDEC_PIP_WAIT_MAX               = 0xff,
} VDEC_PIP_WAIT_TYPE_T;
#endif

// vdec_drv_if.c
extern void VDec_SetSyncStc(UCHAR ucEsId);
extern void vVDecSetSyncCtrlReadyToPlay(VDEC_ES_INFO_T *prVDecEsInfo);
extern void vVDecSetSyncCtrlStop(VDEC_ES_INFO_T *prVDecEsInfo);
extern void vVDecSetSyncCtrlPrintLog(VDEC_ES_INFO_T *prVDecEsInfo, const CHAR* pucLog);
extern VDEC_COMP_INFO_T* VDec_GetCompInfo(UCHAR ucEsId);
extern void VDec_LockVld(UCHAR ucMpvId);
//extern void vVDecSetVdpVdsclInfo(UCHAR ucEsId, VDSCL_INFO_T *prVdsclInfo);
extern void vVDecChkVdpInFlush(UCHAR ucEsId);
extern void VDec_Update_DiscInfo(UCHAR ucEsId);
#if defined(MPV_NO_PARSER) || defined(MPV_DUMP_FBUF)
extern void VDec_Dump_Data(UINT32 u4StartAddr, UINT32 u4FileSize, UINT32 u4FileCnt, UCHAR* pucAddStr);
#endif
//extern BOOL IsPBMode_SR(VID_DEC_SPEED_TYPE_T e_speed_type);
extern UINT16 VDec_GetDecMode(UCHAR ucEsId);
extern void VDec_SetDecMode(UCHAR ucEsId, UINT16 u2DecMode);
extern void vVDecGetDecRes(UCHAR ucEsId);
extern void vVDecReleaseDecRes(UCHAR ucEsId);
#if VDEC_MVC_SUPPORT
extern void VDec_GetOffsetMetaDataAddr(UCHAR ucEsId);
extern void VDec_UpdateOffsetMetaData(UCHAR ucEsId);
#endif

// vdec_drv_esm.c
extern BOOL fgIsEsmIFExisted(UCHAR ucEsId);
extern BOOL fgIsEsmPicExisted(UCHAR ucEsId, UCHAR ucPicChkNum);
extern BOOL fgIsEsmAUExisted(UCHAR ucEsId, BOOL fgIsChkBefVParser);

#if VDEC_REMOVE_UNUSED_FUNC
extern BOOL fgIsVDecCmdExisted(UCHAR ucEsId);
#endif

//extern void vVDecSetEsmInfo(UINT32 pu4Handle, Decoder_OpIf* pprDecoderOplIf, UCHAR ucMpvId, UCHAR ucEsId);
extern VDEC_ESM_INFO_T* vVDecGetEsmInfo(UCHAR ucEsId);
extern VDEC_ES_INFO_T* VDec_GetEsInfo(UCHAR ucEsId);
extern UINT32 vVDecGetEsmHandle(UCHAR ucEsId);
extern void VDec_SetCmd(UCHAR ucEsId, UINT16 u2Mode);
extern void VDec_RetriveCmd(UCHAR ucEsId);
extern void VDec_ClearEsInfo(UCHAR ucEsId);

#if VDEC_REMOVE_UNUSED_FUNC
extern void VDec_ClearSSStopEsInfo(UCHAR ucEsId);
#endif

extern void VDec_Update_Esm(UCHAR ucEsmQId, UINT32 u4Rp);
extern void VDec_CreateEsmSema(UCHAR ucEsId);
extern void VDec_DeleteEsmSema(UCHAR ucEsId);
extern void VDec_ClearVDecInfo(void);
extern void VDec_InitEsInfo(void);
extern void VDec_InitModeLogInfo(void);
extern VDEC_MODELOG_INFO_T* VDec_GetModeLogInfo(UCHAR ucEsId);
extern VDEC_MODELOG_INFO_T* VDec_GetTempModeLogInfo(UCHAR ucEsId);
extern void VDecDiplayModeLogDiff(UCHAR ucEsId);
extern int VDecPrintf(UCHAR ucEsId, UINT32 ucLevel, CHAR *format, ...);
extern BOOL fgIsNoNextStartCode(UCHAR ucEsId, UINT32 ucPicEnd);

extern VDEC_INFO_T* VDec_GetVDecInfo(void);
//extern void vVDecEsm_DataIn_Callback(ESIF_CBEVENT eEvent, void *pvData, void *pvPrivate);
extern void VDecCreateEventGroup(UCHAR ucVDecEsId, CHAR* szBuf);
extern void VDecSetEvent(UCHAR ucVDecEsId, VDEC_EVENT_T eVDecEvent);
extern void VDecWaitEvent(UCHAR ucVDecEsId, VDEC_EVENT_T eVDecEventWaitFor);
extern void VDecClearEvent(UCHAR ucVDecEsId);
extern void VDecDeleteEventGroup(UCHAR ucVDecEsId);

#if VDEC_REMOVE_UNUSED_FUNC
extern MPV_CC_T* MPV_GetMpvCc(UCHAR ucEsId);
extern void MPV_LockCcSema(UCHAR ucEsId);
extern void MPV_UnlockCcSema(UCHAR ucEsId);
extern void MPV_LockAnlgCcSema(UCHAR ucEsId);
extern void MPV_UnlockAnlgCcSema(UCHAR ucEsId);
#endif

// vDec_parse_Proc.c
#if VDEC_REMOVE_UNUSED_FUNC
extern BOOL fgIs1stBSAvailable(void);
#endif

extern BOOL fgIs2ndBSSupport(void);

#if VDEC_REMOVE_UNUSED_FUNC
extern void vVDecVParseProc(UCHAR ucEsId);
extern BOOL fgVDecVParseChkProc(UCHAR ucEsId);
#endif

// vdec_dec_proc.c
extern void VDec_DecInit(void);
extern void VDec_DecUninit(void);
extern void VDec_IsrInit(void);
extern void VDec_IsrStop(void);
extern void VDec_CreateDecSema(UCHAR ucMpvId);
extern void VDec_DeleteDecSema(UCHAR ucMpvId);
extern INT32 i4VDecUpdFbmInfo(UCHAR ucEsId);
extern BOOL fgVdecNeedDeblocking(UCHAR ucEsId);
extern BOOL fgIsFBufAvailable(UCHAR ucEsId);
extern void vVDecStartToDecProc(UCHAR ucEsId);
extern BOOL fgIsVDecEnd(UCHAR ucEsId);

#if VDEC_REMOVE_UNUSED_FUNC
extern BOOL fgVDecResultChk(UCHAR ucEsId);
#endif

extern BOOL fgVDecChkRealEnd(UINT32 u4DecReadPtr,UINT32 u4PicStart,UINT32 u4PicEnd,UINT32 u4FifoStart,UINT32 u4FifoEnd);
extern void vVDecEndProc(UCHAR ucEsId);

#if VDEC_REMOVE_UNUSED_FUNC
extern void vCntPicTime(VDEC_ES_INFO_T *prVDecEsInfo, UINT32 u4Idx, BOOL fgStart);
#endif

extern void vDropPic(UCHAR ucEsId);
extern void VDec_FinDec(UCHAR ucMpvId);
extern void vVDecRecFbStatus(UCHAR ucFbNum, UINT32 u4FbId, UINT32 u4FbStatus);
extern void VDec_CreateVldSema(UCHAR ucMpvId);
extern void VDec_LockVld(UCHAR ucMpvId);
extern void VDec_UnlockVld(UCHAR ucMpvId);
extern void VDec_DeleteVldSema(UCHAR ucMpvId);
extern void vAVSyncTimeLog(VDEC_ES_INFO_T *prVDecEsInfo, UCHAR ucEsId, UINT32 u4Idx, BOOL fgStart);
extern void vStopCmdTimeLog(BOOL fgStart);
extern void vVDecChkFbStatus(UCHAR ucFbNum);

#if VDEC_REMOVE_UNUSED_FUNC
extern void vVDecChkFbEmpty(UCHAR ucFbNum);
#endif

#if VDEC_REMOVE_UNUSED_FUNC
extern void vVDecChkFbRealEmpty(UCHAR ucFbNum, UCHAR ucFbIdx);
#endif

#if VDEC_REMOVE_UNUSED_FUNC
extern void vVDecLog(UINT32 u4LogData);
#endif

#ifdef VDEC_EVENT_TRIGGER            
//extern BOOL fgIsWaitGetFBReady(FBM_CBID eCBID, void *pvArg, void *pvPrivData);
//extern BOOL fgIsWaitOutFBReady(FBM_CBID eCBID, void *pvArg, void *pvPrivData);
#endif
//extern void vSetSkipNum(VDEC_ES_INFO_T *prVDecEsInfo, VID_DEC_PB_MODE_T* prPbInfo);
//extern VOID vVDecAspTransfer(VDEC_ES_INFO_T *prVDecEsInfo, IBC_VideoAspectRatioType aspectRatio);
extern INT32 i4VDecInsToDispQ(VDEC_ES_INFO_T *prVDecEsInfo, UCHAR ucFBGId, UCHAR ucFBId);
//extern void vVDecUniCallback(UCHAR ucEsId, VID_DEC_COND_T eVDecCondition, UINT32 u4Data1, UINT32 u4Data2);
#if VDEC_MVC_SUPPORT
extern void vVDecUpdViewEsInfo(UCHAR ucEsId, VDEC_MVC_INFO_T eMvcInfo);
extern UCHAR VDecGetViewEsId(UCHAR ucViewId);
extern VDEC_ES_INFO_T *VDecGetViewEsInfo(UCHAR ucViewId);
extern BOOL fgIsMvcAlignDrop(VDEC_ES_INFO_T *prVDecEsInfo);
extern void VDec_CreateMvcStopSema(void);
extern void VDec_LockMvcStopSema(UCHAR ucEsId);
extern void VDec_UnlockMvcStopSema(UCHAR ucEsId);
extern void VDec_DeleteMvcStopSema(void);
#endif

extern BOOL fgVDecFillBlkDFB(UCHAR ucEsId);

// vdec_drv_norm_proc.c
extern VDEC_COND_T VDec_NormProc(UCHAR ucEsId);
extern void vVDecReleaseProc(UCHAR ucEsId);

// vdec_hal_if_common.c
extern INT32 i4VDEC_HAL_Common_Init(UINT32 u4ChipID);
extern void vDEC_HAL_COMMON_SetVLDPower(UINT32 u4VDecID, BOOL fgOn);
extern void  vDEC_HAL_COMMON_PowerOn (void );
extern void vDEC_HAL_COMMON_PowerOff (void );
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
extern void vVDEC_HAL_CLK_Set(UINT32 u4CodeType);
#endif

// vdec_drv_dec_proc.c
//extern PATH_TYPE eVDecPath(UINT16 u2CompId);
extern BOOL fgIsVDecPatch(UCHAR ucEsId);
//extern INT32 i4VDecSetCurPicSpecialInfo(UCHAR ucEsId, VDEC_CUR_PIC_SPECIAL_INFO_T *prSepcialInfo);

#ifdef VDEC_SR_SUPPORT
#ifdef VDEC_UNIFORM_CB_SUPPORT
//extern DRV_CB_REG_INFO_T* vVDecGetNfyInfo(UINT16 u2CompId);
#else
//extern VID_DEC_NFY_INFO_T* vVDecGetNfyInfo(UINT16 u2CompId);
#endif
extern VDEC_ESM_INFO_T* vVDecGetEsmInfo(UCHAR ucEsId);
extern void VDec_FwdToSr(UCHAR ucEsId);
extern void VDec_StopToSr(UCHAR ucEsId);
extern void VDec_ClearSRReq(UCHAR ucEsId);
extern VDEC_COND_T     VDec_SRProc(UCHAR ucEsId);
extern BOOL fgVDecIsESIFull(UCHAR ucEsId);
extern void vVDecMoveWrPtr(UCHAR ucEsId, UINT32 u4NewFifoWp, UINT32 u4NewAUWrIdx);
#endif

extern UCHAR ucVDecGetRealAddSwapMode(UCHAR ucAddrSwapMode);
extern UCHAR ucVDecGetRealVdsclAddSwapMode(UCHAR ucAddrSwapMode);
extern UINT32 u4VDecGetCurPictureSize(VDEC_ES_INFO_T * prVDecEsInfo);

#if  (defined(DRV_VDEC_VDP_RACING) || defined(VDEC_PIP_WITH_ONE_HW))    
extern void VDec_CreatePipSema(void);
extern void VDec_DeletePipSema(void);
extern void VDec_LockPipSema(void);
extern void VDec_UnlockPipSema(void);
#endif
extern void vVDecCntQuanFactor(UCHAR ucEsId);

#endif
