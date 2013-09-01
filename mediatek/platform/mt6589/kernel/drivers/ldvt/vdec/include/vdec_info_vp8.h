
#ifndef _VDEC_INFO_VP8_H_
#define _VDEC_INFO_VP8_H_

#define VP8_USE_SMALLQT

#define VP8_HEADERPARSE_HWACCELATOR

#define VDEC_VP8_WRAPPER_OFF 0                      //[Jackal Chen] If width or height > 2048, enable this!
#define VDEC_VP8_WEBP_SUPPORT 0                     //[Jackal Chen] If we will decode Webp, enable this!
//VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION only valid when VDEC_VP8_WEBP_SUPPORT = 1
#define VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION 0     //[Jackal Chen] If we will decode Webp with ME2 integration, enable this!
//VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION only valid when VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION = 1
#define VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION 0   //[Jackal Chen] If we will decode Webp with ME2 integration, enable this!

#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION
typedef enum {
	vVerResult_MB_ROW_DONE = 0,
	vVerResult_FRAME_DONE = 1,
	vVerResult_TIMEOUT = 2
} vVerResult;
#endif

#if VDEC_VP8_WEBP_SUPPORT
#define VP8_MB_ROW_MODE_SUPPORT 0   //[Jackal Chen]NO USED!
#else
#define VP8_MB_ROW_MODE_SUPPORT 0   //[Jackal Chen]NO USED!
#endif
#define VP8_I_FRM    0 //Key Frame
#define VP8_P_FRM   1 // Prediction Frame

#define VP8_VLD1 (0)
#define VP8_VLD2 (1)
#define VP8_BINTRAMODES (VP8_BPRED_MODE_B_HU+1)  // 10
#define VP8_SUBMVPREFS (1+VP8_BPRED_MODE_NEW4X4-VP8_BPRED_MODE_LEFT4X4)
#define VP8_YMODES (VP8_MBPRED_MODE_B+1)
#define VP8_UVMODES (VP8_MBPRED_MODE_TM+1)
#define VP8_MVREFS (1+VP8_MBPRED_MODE_SPLITMV-VP8_MBPRED_MODE_NEARESTMV)
#define VP8_BLOCK_TYPES (4)
#define VP8_COEF_BANDS (8)
#define VP8_PREV_COEF_CONTEXTS (3)
#define VP8_COEF_TOKENS (12)
#define VP8_PROB_TYPE(pBrob,btype) (pBrob+btype*VP8_COEF_BANDS*VP8_PREV_COEF_CONTEXTS*(VP8_COEF_TOKENS-1))
#define VP8_PROB_BAND(pBtype,band) (pBtype+band*VP8_PREV_COEF_CONTEXTS*(VP8_COEF_TOKENS-1))
#define VP8_PROB_CTXT(pBand,ctxtype) (pBand+ctxtype*(VP8_COEF_TOKENS-1))
#define VP8_PROB_TOKEN(pCtxt,token) (pCtxt+token)

#define VDEC_VP8_WAIT_DISP_TIME (1500)
#define WAIT_THRD 0x1000
#define DEC_RETRY_NUM 8000

//#define VP8_KEY_FRAME (0)
//#define VP8_INTER_FRAME (1)
#define VP8_MINQ (0)
#define VP8_MAXQ (127)
#define VP8_QINDEX_RANGE (VP8_MAXQ+1)
#define VP8_SEGMENT_ALTQ (0x01)
#define VP8_SEGMENT_ALTLF (0x02)
#define MB_FEATURE_TREE_PROBS (3)
#define MAX_MB_SEGMENTS (4)
#define MAX_REF_LF_DELTAS (4)
#define MAX_MODE_LF_DELTAS (4)
#define MAX_VP8_DATAPARTITION (8)
#define MAX_FILTER_LVL (63)
#define VP8_IVF_FILE_HEADER_SZ		0x20
#define VP8_IVF_FRAME_HEADER_SZ		0x0C

#define VDEC_GET_FLAGVAL(value,flg) (((value)&1)<<flg)
#define VDEC_GET_RANGEVAL(value,s,e) (((value)&((1<<(e-s+1))-1))<<s)
#define VDEC_REG_GET_VALUE(value,s,e) (((value)>>s)&((1<<(e-s+1))-1))
#define VDEC_REG_SET_VALUE(reg,value,s,e) (reg=(reg&(~((1<<(e-s+1))<<s)))|(VDEC_GET_RANGEVAL(value,s,e)))
#define VDEC_REG_GET_FLAG(value,p) (((value)>>p)&0x1)
#define VDEC_REG_SET_FLAG(reg,value,p)  (reg=(reg&(~(1<<p)))|(VDEC_GET_FLAGVAL(value,p)))
#define VDEC_VP8_DQINDEXCLAMP(index) (index>127 ? 127 : (index<0 ? 0 : index))


#define VDEC_CLRFLG(flg,index) ((flg) &= ~(1<<(index)))
#define VDEC_SETFLG(flg,index) ((flg) |= (1<<(index)))
#define VDEC_FLGSET(flg,index) (((flg)>>(index))&1)
#define VDEC_SETFLG_COND(flg,index,cond) ((cond) ? VDEC_SETFLG(flg,index) : VDEC_CLRFLG(flg,index))
#define VDEC_MEMALIGN(ptr,align) (ptr+=(align-(((UINT32)ptr)&(align-1))))
#define VDEC_RPOS_INC(src,len,startaddr,endaddr) (src= (src+(len)>=endaddr) ? (startaddr+((src)+(len)-(endaddr))) : ((src)+(len)))
#define VDEC_INTEGER(integer,src,len,startaddr,endaddr) \
{  \
   UCHAR uByte,uIntLen=0; \
   integer=0; \
   while(uIntLen<len) \
   {  \
      uByte=*((UCHAR *)src); \
      integer=(integer<<8)|uByte; \
      uIntLen++;\
      VDEC_RPOS_INC(src,1,startaddr,endaddr); \
   } \
}

#define VDEC_INTREVERSE(integer,len) \
{ \
  UINT32 u4Value=0,uIndex=0; \
  while(uIndex<len) \
  {  \
    u4Value=u4Value<<8|((integer&(0xff<<(uIndex*8)))>>(uIndex*8)); \
    uIndex++; \
  } \
  integer=u4Value;\
}


typedef enum
{
  VP8PARAM_NO_LPF,
  VP8PARAM_SIMPLER_LPF,
  VP8PARAM_BILINER_MCFILTER,
  VP8PARAM_FULL_PIXEL,
  VP8PARAM_COLOR,
  VP8PARAM_CLAMP_TYPE,
  VP8PARAM_SEGMENT_ENABLE,
  VP8PARAM_SEGMENT_UPDATE_MAP,
  VP8PARAM_SEGMENT_UPDATE_DATA,
  VP8PARAM_SEGMENT_ABSDATA, // 0--Delta data, 1--Abs data.
  VP8PARAM_MODEREF_IFDELTA_UPDATE,
  VP8PARAM_MODEREF_LFDELTA_ENABLE,
  VP8PARAM_REFRESH_GOLDEN,
  VP8PARAM_REFRESH_ALTRF,
  VP8PARAM_QINDEX_UPDATE,
  VP8PARAM_REFRESH_PROBS,
  VP8PARAM_REFRESH_LASTFRAME,
  VP8PARAM_NOCOEF_SKIP,
}VDEC_PARAM_VP8FLAG_T;

typedef enum
{
  VP8_MBPRED_MODE_DC,
  VP8_MBPRED_MODE_V,
  VP8_MBPRED_MODE_H,
  VP8_MBPRED_MODE_TM,
  VP8_MBPRED_MODE_B,
  VP8_MBPRED_MODE_NEARESTMV,
  VP8_MBPRED_MODE_NEARMV,
  VP8_MBPRED_MODE_ZEROMV,
  VP8_MBPRED_MODE_NEWMV,
  VP8_MBPRED_MODE_SPLITMV,
  VP8_MBPRED_MODE_MAX
}VDEC_VP8_MB_PREDICTION_MODE_T;

typedef enum
{
  VP8_BPRED_MODE_B_DC,
  VP8_BPRED_MODE_B_TM,
  VP8_BPRED_MODE_B_VE,
  VP8_BPRED_MODE_B_HE,
  VP8_BPRED_MODE_B_LD,
  VP8_BPRED_MODE_B_RD,
  VP8_BPRED_MODE_B_VR,
  VP8_BPRED_MODE_B_VL,
  VP8_BPRED_MODE_B_HD,
  VP8_BPRED_MODE_B_HU,
  VP8_BPRED_MODE_LEFT4X4,
  VP8_BPRED_MODE_ABOVE4X4,
  VP8_BPRED_MODE_ZEOR4X4,
  VP8_BPRED_MODE_NEW4X4,
  VP8_BPRED_MODE_MAX
}VDEC_VP8_B_PREDICTION_MODE_T;

typedef enum
{
  VP8_MVREF_INTRA_FRAME,
  VP8_MVREF_LAST_FRAME,
  VP8_MVREF_GOLDEN_FRAME,
  VP8_MVREF_ALTREF_FRAME,
  VP8_MVREF_MAX
}VDEC_VP8_MVREF_FRAME_T;

typedef enum
{
  VP8_QTYPE_Y1AC=0,
  VP8_QTYPE_Y1DC,
  VP8_QTYPE_Y2DC,
  VP8_QTYPE_Y2AC,
  VP8_QTYPE_UVDC,
  VP8_QTYPE_UVAC,
  VP8_QTYPE_MAX
}VDEC_VP8_QTYPE_T;

typedef enum
{
  VP8_QTABLE_Y1,
  VP8_QTABLE_Y2,
  VP8_QTABLE_UV,
  VP8_QTALBE_MAX
}VDEC_VP8_QTABLE_T;

typedef enum
{
  VP8_MBLVL_ALT_Q,
  VP8_MBLVL_ALT_LF,
  VP8_MBLVL_MAX
}VDEC_VP8_MBLVL_FEATURE;
typedef enum
{
  VP8_LF_TYPE_NORMAL=0,
  VP8_LF_TYPE_SIMPLE
}VDEC_VP8_FILTER_TYPE_T;

typedef enum
{
  VP8_FRAME_CURRENT=0,
  VP8_FRAME_LAST,
  VP8_FRAME_GOLD,
  VP8_FRAME_ALT,
  VP8_FRAME_NO_UPD
}VDEC_VP8_FRAME_REFRESH_T;

typedef enum
{
  VDEC_VP8_PIC_STATE,
  VDEC_VP8_MB_STATE,
  VDEC_VP8_COEF_STATE
}VDEC_VP8_DECSTATE_T;

typedef enum
{
  VP8_MVDEF_MV_MAX=1024,
  VP8_MVDEF_MVVALS=(2*VP8_MVDEF_MV_MAX)+1,
  VP8_MVDEF_MVLONG_WIDHT=10,
  VP8_MVDEF_MVNUM_SHORT=8,
  VP8_MVDEF_MVPIS_SHORT=0,
  VP8_MVDEF_MVPSIGN,
  VP8_MVDEF_MVPSHORT,
  VP8_MVDEF_MVPBITS=VP8_MVDEF_MVPSHORT+VP8_MVDEF_MVNUM_SHORT-1,  // value is 9
  VP8_MVDEF_MVPCOUNT=VP8_MVDEF_MVPBITS+VP8_MVDEF_MVLONG_WIDHT   //value is 19
}VDEC_VP8_MVDEF_T;


typedef enum
{
  VP8_KEEPALT = 0,
  VP8_LAST2ALT,
  VP8_GOLD2ALT,
  VP8_WORK2ALT
}VDEC_VP8_ALTSTATE_T;

typedef enum
{
  VP8_KEEPGOLD = 0,
  VP8_LAST2GOLD,
  VP8_ALT2GOLD,
  VP8_WORK2GOLD
}VDEC_VP8_GOLDSTATE_T;

typedef enum
{
  VP8_KEEPLAST = 0,
  VP8_WORK2LAST,
}VDEC_VP8_LASTSTATE_T;

typedef struct _VDEC_VP8_UPDST_T_
{
  VDEC_VP8_ALTSTATE_T eAltState;
  VDEC_VP8_GOLDSTATE_T eGoldState;
  VDEC_VP8_LASTSTATE_T eLastState;
}VDEC_VP8_UPDST_T;



typedef struct _VDEC_INFO_VP8_BS_INIT_PRM_T_
{
    UINT32 u4ReadPointer;              
    UINT32 u4WritePointer;
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_VP8_BS_INIT_PRM_T;

typedef struct _VDEC_INFO_VP8_VFIFO_PRM_T_
{
    UINT32 u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32 u4VFifoEa;                 ///< Video Fifo memory end address 
}VDEC_INFO_VP8_VFIFO_PRM_T;

typedef struct _VDEC_INFO_VP8_FRAME_BUF_SA_T_
{
    UINT32  u4Pic0YSa;
    UINT32  u4Pic1YSa;
    UINT32  u4Pic2YSa;
    UINT32  u4Pic3YSa;

    UINT32  u4PpYBufSa;
    UINT32  u4PpCBufSa;
}VDEC_INFO_VP8_FRAME_BUF_SA_T;


typedef struct _VDEC_HAL_DEC_VP8_ERR_INFO_T_
{    
    UINT32 u4YoutRangeErr;                                ///< Video decode error count
    UINT32 u4XoutRangeErr;                                ///< Video decode error mb row
    UINT32 u4BlkCoefErr;                               ///< Video decode error type
    UINT32 u4BitCnt1stPartErr; 
    UINT32 u4BitCnt2stPartErr; 
    UINT32 u4MCBusyOverflowErr; 
    UINT32 u4MDecTimoutInt;     
    UINT32 u4Vp8ErrCnt;
}VDEC_INFO_VP8_ERR_INFO_T;

typedef struct _VDEC_INFO_VP8_PP_INFO_T_
{
    BOOL     fgPpEnable;
    UINT8    u1PpLevel;
    UINT8    au1MBqp[4];
    UINT32  u4PpYBufSa;
    UINT32  u4PpCBufSa;
}VDEC_INFO_VP8_PP_INFO_T;

typedef struct
{
  UINT8 Prob[VP8_MVDEF_MVPCOUNT];
}VDEC_VP8_MV_CONTEXT_T;

typedef struct
{
  //UINT8 BModeProb[VP8_BINTRAMODES-1];
  UINT8 YModeProb[VP8_YMODES-1];
  UINT8 UVModeProb[VP8_UVMODES-1];
  //UINT8 SubMVRefProb[VP8_SUBMVPREFS-1];
  UINT8 CoefProbs[VP8_BLOCK_TYPES][VP8_COEF_BANDS][VP8_PREV_COEF_CONTEXTS][VP8_COEF_TOKENS-1];
  VDEC_VP8_MV_CONTEXT_T MVC[2];
  //VDEC_VP8_MV_CONTEXT_T PRE_MVC[2];
}VDEC_VP8FRAME_CONTEXT_T;

typedef struct
{
  UINT32 u4FlagParam;
  UINT32 u4LastParam;
  INT8 SegmentFeatureData[VP8_MBLVL_MAX][MAX_MB_SEGMENTS];
  UINT8 SegmentTreeProbs[MB_FEATURE_TREE_PROBS];
  INT8 RefLFDeltas[MAX_REF_LF_DELTAS];
  INT8 ModeLFDeltas[MAX_MODE_LF_DELTAS];
  INT8 RefFrameSignBias[VP8_MVREF_MAX];
  VDEC_VP8_FILTER_TYPE_T eLoopFilterType;
  INT8 iLoopFilterLvl;
  INT8 iSharpLvl;
  UINT8 uDataPartitionNum;
  UINT8 uDataPartitionToken;
  UINT8 uSkipFalseProb;
  UINT8 uIntraProb;
  UINT8 uLastProb;
  UINT8 uGoldenProb;
  UINT8 uCopyBuf2Gf; // 0:no 1:last-->gf 2: arf-->gf
  UINT8 uCopyBuf2Arf;  // 0:no 1:laast-->arf   2:gf-->arf
  INT16 QIndexInfo[VP8_QTYPE_MAX];
  UINT8 *puWorkingBuf;
  VDEC_VP8FRAME_CONTEXT_T rLastFC;
  VDEC_VP8FRAME_CONTEXT_T rCurFc;
}VDEC_PARAM_VP8DEC_T;

typedef struct _VDEC_INFO_VP8_FRM_HDR_T_
{
   UINT8 uFrameType;
   UINT8 uShowFrame;
   UINT8 uHScale;
   UINT8 uVScale;
   UINT32 u4Width;
   UINT32 u4Height;
   UINT8 uVersion;
   UINT32 u4ReadPointer; //[20101223][Youguo Li] marked for 8580 drv_emualtion build failed.
   UINT32 u4VldStartPos;
   UINT32 u4FifoStart;
   UINT32 u4FifoEnd;
   UINT32 u4FirstPartLen;
   UINT32 u4FrameSize;
   UINT32 u4WritePos;
   UINT32 u4GldYAddr;
   UINT32 u4AlfYAddr;
   UINT32 u4LstYAddr;
   UINT32 u4CurYAddr;
   UINT32 u4CurCAddr;
   UINT32 u4VLDWrapper;
   UINT32 u4SegIdWrapper;
   UINT32 u4PPWrapperY;
   UINT32 u4PPWrapperC;
   UINT32 u4VLDWrapperWrok;
   UINT32 u4PPWrapperWrok;
   VDEC_PARAM_VP8DEC_T rVp8DecParam;
   VDEC_INFO_VP8_FRAME_BUF_SA_T rVp8FrameBufSa;
    //Driver Flow Control
   UCHAR    ucPicStruct;
   UCHAR    ucColorPrimaries;
   UINT16    u2FrmRatCod;
   UCHAR    ucAspectRatio;
}VDEC_INFO_VP8_FRM_HDR_T;

#ifdef VDEC_VP8_HWDEBLOCK
#define VDEC_PP_ENABLE TRUE
#else
#define VDEC_PP_ENABLE FALSE    
#endif

#define VDEC_VP8WAIT_TIME (500)
#define VDEC_VP8DEF_FRAME_RATE (30000)
#define IPBMode 0x0
#define IPMode  0x1
#define IMode   0x2

#define VP8_NO_PIC 0
#define VP8_TOP_FIELD 1
#define VP8_BOTTOM_FIELD 2
#define VP8_FRAME 3

typedef enum
{
  VP8_DEC_FLG_UNSUPPORT=0,
  VP8_DEC_FLG_INITED,
  VP8_DEC_FLG_ENDCALPTS,
  VP8_DEC_FLG_DECERROR,
  VP8_DEC_FLG_VALIDPTS,
  VP8_DEC_FLG_BSPWKBUF,
  VP8_DEC_FLG_NEWGD,
  VP8_DEC_FLG_LOCKED
} VP8_DEC_FLAG;

typedef struct
{
  BOOL  fgRefreshGd;
  BOOL  fgKeyFrame;
  BOOL  fgLastKeyFrame;
  BOOL  fgInitedDec;
  UINT32 u4WPtr;
  UINT32 u4AddrMode;
  UCHAR ucFbgId;
  UCHAR ucFbgType;
  UCHAR ucSyncMode;
  UCHAR ucSkipMode;
  UCHAR ucCurFbId;
  UCHAR ucLastFbId;
  UCHAR ucDbkFbId;
  UCHAR ucGoldenFbId;
  UCHAR ucAltFbId;
  UCHAR ucDispFbId;
  UINT32 u4NewWidth;
  UINT32 u4NewHeight;
  UINT32 u4Width;
  UINT32 u4Height;
  UINT32 u4Flag;
  UINT32 u4ReadPtr;
  UINT32 u4LastReadPtr;
  UINT32 u4DispPts;
  UINT32 u4Rate;
  UINT32 u4FrameCounter;
  UINT32 u4FbmLineSize;
  UINT32 u4DataOffset;
  UINT32 u4WorkBuf;
  UINT32 u4RefPts;
  UINT32 u4DeltaPTS;
  INT32  i4DecRet;
  UINT64 u8Offset;
#ifdef CC_VP8_EMULATION
  UINT32 u4CrcVal[2][4];
#endif
  INT8 RefTime[16];
}VP8_DEC_PARAM_T;

typedef struct
{
  VP8_DEC_PARAM_T rDecParam;
  VDEC_INFO_VP8_FRM_HDR_T rVp8FrmHdr;
  UINT8 uEsId;
}VDEC_VP8_INFO_T;

typedef struct _VDEC_INFO_VP8_DEC_PRM_T_
{    
    UINT32  u4FRefBufIdx;
    INT32    i4MemBase;
    BOOL    fgAdobeMode;
    VDEC_INFO_VP8_FRM_HDR_T *prFrmHdr;
    VDEC_INFO_VP8_FRAME_BUF_SA_T rVp8FrameBufSa;
    VDEC_INFO_VP8_PP_INFO_T           rVp8PpInfo;
} VDEC_INFO_VP8_DEC_PRM_T;


#endif //#ifndef _VDEC_INFO_VP8H_

