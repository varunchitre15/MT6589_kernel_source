
#ifndef _VDEC_DRV_MPEG2_INFO_H_
#define _VDEC_DRV_MPEG2_INFO_H_

//#include "x_os.h"
//#include "x_bim.h"
//#include "x_assert.h"
//#include "x_timer.h"

#include "drv_common.h"

#include "vdec_common_if.h"
#include "vdec_usage.h"

#include "vdec_info_mpeg.h"
#include "vdec_info_common.h"

/******************************************************************************
* Local definition
******************************************************************************/
// start code
#define PICTURE_START_CODE          0x100
#define SLICE_START_CODE_MIN      0x101
#define SLICE_START_CODE_MAX      0x1AF
#define USER_DATA_START_CODE     0x1B2
#define SEQUENCE_HEADER_CODE     0x1B3
#define SEQUENCE_ERROR_CODE       0x1B4
#define EXTENSION_START_CODE      0x1B5
#define SEQUENCE_END_CODE           0x1B7
#define GROUP_START_CODE             0x1B8
#define SYSTEM_START_CODE_MIN   0x1B9
#define SYSTEM_START_CODE_MAX   0x1FF

// extension start code IDs
#define SEQUENCE_EXTENSION_ID                        1
#define SEQUENCE_DISPLAY_EXTENSION_ID        2
#define QUANT_MATRIX_EXTENSION_ID               3
#define COPYRIGHT_EXTENSION_ID                      4
#define SEQUENCE_SCALABLE_EXTENSION_ID      5
#define PICTURE_DISPLAY_EXTENSION_ID           7
#define PICTURE_CODING_EXTENSION_ID            8
#define PICTURE_SPATIAL_SCALABLE_EXTENSION_ID    9
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID  10

#define MAX_RETRY_COUNT            30
#define MAX_USER_DATA_SIZE      200

#define LEVEL_ID_LOW        10
#define LEVEL_ID_MAIN       8
#define LEVEL_ID_HIGH_1440  6
#define LEVEL_ID_HIGH       4

#define DEFAULT_H_SIZE      720
#define DEFAULT_V_SIZE      480

#define SCTE_CC  0x1

#define MPEG2_DPB_NUM 3
#define MPEG2_MAX_EDPB_NUM 30
#define MPEG2_DFB_NUM 120

#define MPEG1_MAX_HSIZE  352
#define MPEG1_MAX_VSIZE  288

#define MPEG2_PANSCAN_VECTOR_NS 3

#define fgIsMPEG2SeqHdr(arg)  (arg & SEQ_HDR)
#define fgIsMPEG2GopHdr(arg)  (arg & GOP_HDR)
#define fgIsMPEG2SeqEnd(arg)   (arg & SEQ_END)
#define fgIsVOBUStill(arg1, arg2)   ((arg1&AUTO_PAUSE)&&(arg2==VID_DEC_SPEED_TYPE_NORMAL))


#define DVD24P_SEQ_STEP 4
// Sequence Hdr
typedef struct _MPEG2_SEQ_HDR_FIELD_T
{
	/* 32 Bit */
	UINT32			u4FrameRate			:	4;
	UINT32			u4AspectRatio		:	4;
	UINT32			u4VSize				:	12;
	UINT32			u4HSize				:	12;

	/* 32 Bit */
	UINT32			fgLoadNonIntra		:	1;
	UINT32			fgLoadIntra			:	1;
	UINT32			fgContrained		:	1;
	UINT32			u4VbvSize			:	10;
	UINT32			fgMark				:	1;
	UINT32			u4BitRate			:	18;
} MPEG2_SEQ_HDR_FIELD_T;

typedef union _MPEG2_SEQ_HDR_UNION_T
{


	UINT32						au4Reg[2];
	MPEG2_SEQ_HDR_FIELD_T		rField;
} MPEG2_SEQ_HDR_UNION_T;

// Sequence Externsion Hdr
typedef struct _MPEG2_SEQ_EXT_HDR_FIELD_T
{
	/* 32 Bit */
	UINT32			fgMark				:	1;
	UINT32			u4BitRateExt		:	12;
	UINT32			u4VSizeExt			:	2;
	UINT32			u4HSizeExt			:	2;
	UINT32			u4ChromaFmt			:	2;
	UINT32			fgPrgsSeq			:	1;
	UINT32			u4LevelId			:	4;
	UINT32			u4ProfileId			: 	4;
	UINT32			u4ExtId				:	4;

	/* 16 Bit */
	UINT32								:	16;
	UINT32			u4FrameRateExtD		:	5;
	UINT32			u4FrameRateExtN		:	2;
	UINT32			fgLowDelay			:	1;
	UINT32			u4VbvSizeExt		:	8;
} MPEG2_SEQ_EXT_HDR_FIELD_T;

typedef union _MPEG2_SEQ_EXT_HDR_UNION_T
{


	UINT32						au4Reg[2];
	MPEG2_SEQ_EXT_HDR_FIELD_T	rField;
} MPEG2_SEQ_EXT_HDR_UNION_T;

// Sequence Display Externsion Hdr
typedef struct _MPEG2_SEQ_DISP_EXT_HDR_FIELD_T
{
	/* 32/8 Bit */
	UINT32			u4MatrixCoef		:	8;
	UINT32			u4TrasferChar		:	8;
	UINT32			u4ColorPrimary		:	8;
	UINT32			fgColorDesc			:	1;
	UINT32			u4VideoFmt			:	3;
	UINT32			u4ExtId				:	4;

	/* 29 Bit */
	UINT32								:	3;
	UINT32			u4DispVSize			:	14;
	UINT32			fgMark				:	1;
	UINT32			u4DispHSize			:	14;
} MPEG2_SEQ_DISP_EXT_HDR_FIELD_T;

typedef union _MPEG2_SEQ_DISP_EXT_HDR_UNION_T
{


	UINT32							au4Reg[2];
	MPEG2_SEQ_DISP_EXT_HDR_FIELD_T	rField;
} MPEG2_SEQ_DISP_EXT_HDR_UNION_T;


// GOP Hdr
typedef struct _MPEG2_GOP_HDR_FIELD_T
{
	/* 27 Bit */
	UINT32								:	5;
	UINT32			fgBrokenLink		:	1;
	UINT32			fgClosedGop			:	1;
	UINT32			u4Picture			:	6;
	UINT32			u4Second			:	6;
	UINT32			fgMark				:	1;
	UINT32			u4Minute			:	6;
	UINT32			u4Hour				:	5;
	UINT32			fgDropFrame			:	1;
} MPEG2_GOP_HDR_FIELD_T;

typedef union _MPEG2_GOP_HDR_UNION_T
{
	UINT32					au4Reg[1];
	MPEG2_GOP_HDR_FIELD_T	rField;
} MPEG2_GOP_HDR_UNION_T;

// Picture Hdr
typedef struct _MPEG2_PIC_HDR_FIELD_T
{
	/* 29 Bit */
	UINT32								:	3;
	UINT32			u4VbvDelay			:	16;
	UINT32			u4PicType			:	3;
	UINT32			u4TemporalRef		:	10;
} MPEG2_PIC_HDR_FIELD_T;

typedef union _MPEG2_PIC_HDR_UNION_T
{
	UINT32					au4Reg[1];
	MPEG2_PIC_HDR_FIELD_T	rField;
} MPEG2_PIC_HDR_UNION_T;

// P PIC Hdr
typedef struct _MPEG2_P_PIC_HDR_FIELD_T
{
	/* 4 Bit */
	UINT32								:	28;
	UINT32			u4ForwardFCode		:	3;
	UINT32			fgFullPelForward	:	1;
} MPEG2_P_PIC_HDR_FIELD_T;

typedef union _MPEG2_P_PIC_HDR_UNION_T
{
	UINT32					au4Reg[1];
	MPEG2_P_PIC_HDR_FIELD_T	rField;
} MPEG2_P_PIC_HDR_UNION_T;

// B PIC Hdr
typedef struct _MPEG2_B_PIC_HDR_FIELD_T
{
	/* 8 Bit */
	UINT32								:	24;
	UINT32			u4BackwardFCode		:	3;
	UINT32			fgFullPelBackward	:	1;
	UINT32			u4ForwardFCode		:	3;
	UINT32			fgFullPelForward	:	1;
} MPEG2_B_PIC_HDR_FIELD_T;

typedef union _MPEG2_B_PIC_HDR_UNION_T
{
	UINT32					au4Reg[1];
	MPEG2_B_PIC_HDR_FIELD_T	rField;
} MPEG2_B_PIC_HDR_UNION_T;

// Picture Externsion Hdr
typedef struct _MPEG2_PIC_EXT_HDR_FIELD_T
{
	/* 32 Bit */
	UINT32			fgChroma420			:	1;
	UINT32			fgRff				:	1;
	UINT32			fgAlternateScan		:	1;
	UINT32			fgIntraVlc			:	1;
	UINT32			fgQScale			:	1;
	UINT32			fgConcealmentMV		:	1;
	UINT32			fgFramePred			:	1;
	UINT32			fgTff				:	1;
	UINT32			u4PicStruture		:	2;
	UINT32			u4IntraDcPrecision	:	2;
	UINT32			u4FCode11			:	4;
	UINT32			u4FCode10			:	4;
	UINT32			u4FCode01			:	4;
	UINT32			u4FCode00			:	4;
	UINT32			u4ExtId				:	4;

	/* 22 Bit */
	UINT32								:	10;
	UINT32			u4SubCarrierPhase	:	8;
	UINT32			u4BurstAmplitude	:	7;
	UINT32			fgSubCarrier		:	1;
	UINT32			u4FieldSequence		:	3;
	UINT32			fgVAxis				:	1;
	UINT32			fgCompositeDisp		:	1;
	UINT32			fgPrgsFrame			:	1;
} MPEG2_PIC_EXT_HDR_FIELD_T;

typedef union _MPEG2_PIC_EXT_HDR_UNION_T
{
	UINT32						au4Reg[2];
	MPEG2_PIC_EXT_HDR_FIELD_T	rField;
} MPEG2_PIC_EXT_HDR_UNION_T;

// Picture Display Externsion Hdr
typedef struct _MPEG2_PIC_DISP_EXT_HDR_FIELD_T
{
	/* 17 Bit */
	UINT32								:	15;
	UINT32			fgMark				:	1;
	UINT32			u4Offset			:	16;
} MPEG2_PIC_DISP_EXT_HDR_FIELD_T;

typedef union _MPEG2_PIC_DISP_EXT_HDR_UNION_T
{


	UINT32							au4Reg[1];
	MPEG2_PIC_DISP_EXT_HDR_FIELD_T	rField;
} MPEG2_PIC_DISP_EXT_HDR_UNION_T;

// C_o_p_y_R_i_g_h_t E_x_t_e_n_s_i_o_n H_d_r
typedef struct _MPEG2_COPYRIGHT_EXT_HDR_FIELD_T
{
	/* 22 Bit */
	UINT32								:	10;
	UINT32			fgMark1				:	1;
	UINT32								:	7;
	UINT32			fgOriginal			:	1;
	UINT32			u4CopyRightId		:	8;
	UINT32			fgCopyFlag			:	1;
	UINT32			u4ExtId				:	4;

	/* 21 Bit */
	UINT32								:	11;
	UINT32			fgMark2				:	1;
	UINT32			u4CopyRightNs1		:	20;

	/* 23 Bit */
	UINT32								:	9;
	UINT32			fgMark3				:	1;
	UINT32			u4CopyRightNs2		:	22;

	/* 22 Bit */
	UINT32								:	10;
	UINT32			u4CopyRightNs3		:	22;
} MPEG2_COPYRIGHT_EXT_HDR_FIELD_T;

typedef union _MPEG2_COPYRIGHT_EXT_HDR_UNION_T
{


	UINT32							au4Reg[4];
	MPEG2_COPYRIGHT_EXT_HDR_FIELD_T	rField;
} MPEG2_COPYRIGHT_EXT_HDR_UNION_T;

// Sequence Header Structure 
typedef struct _MEPG2_SEQ_HDR_T
{
    BOOL    fgSeqHdrValid;
    
    UINT16 u2HSize;     			// horizontal size value in Sequence Header
    UINT16 u2VSize;      			// vertical size value in Sequence Header
    UCHAR		ucAspRatInf;    		// aspect_ratio_information
    UCHAR		ucFrmRatCod;  			// frame_rate_code
    UINT32		u4BitRatVal;         	//  bit_rate_value;
    //    BOOL		fgLoadIntraMatrix;		//  load_intra_quantizer_matrix;
    //    BOOL		fgLoadNonIntraMatrix;	//  load_non_intra_quantizer_matrix;
    BOOL fgProgressiveSeq;
    UCHAR		ucCrmaFmt;				//chroma_format;
    //BOOL		fgClosedGop;
    //BOOL		fgBrokenLink;

    UCHAR		ucColourPrimaries;		// BT.709 or BT.601

    UINT16 u2DispHSize;     			// Disp horizontal size value in Sequence Header
    UINT16 u2DispVSize;      			// Disp vertical size value in Sequence Header

    UINT8   u1Profile;
    UINT8   u1Level;
} MEPG2_SEQ_HDR_T;

typedef struct _MEPG2_GOP_HDR_T
{
    BOOL    fgGopHdrValid;
    BOOL		fgClosedGop;
    BOOL		fgBrokenLink;
    UINT32			u4Picture			:	6;
    UINT32			u4Second			:	6;
    UINT32			u4Minute			:	6;
    UINT32			u4Hour				:	5;
} MEPG2_GOP_HDR_T;

// Picture & GOP Header Structure 
typedef struct _MEPG2_PIC_HDR_T
{
    BOOL    fgPicHdrValid;
    
    UINT32	u8PTS;				//  90 KHz PTS value, 33/32 bit ? 
    UINT16	u2TemporalRef;

    UCHAR	ucPicCdTp;
    UCHAR	ucFullPelFordVec;
    UCHAR	ucFordFCode;
    UCHAR	ucFullPelBackVec;
    UCHAR	ucBackFCode;
    UCHAR	ucfcode[2][2];
    UCHAR	ucIntraDcPre;
    UCHAR	ucPicStruct;	
    BOOL	fgTopFldFirst;
    BOOL	fgFrmPredFrmDct;
    BOOL	fgConcMotionVec; 
    BOOL	fgQScaleType; 
    BOOL	fgIntraVlcFmt; 
    BOOL	fgAltScan;
    BOOL	fgRepFirstFld;
    BOOL	fgProgressiveFrm;

    UCHAR	ucPSVectorNum;	
    INT16	ai2HOffset[MPEG2_PANSCAN_VECTOR_NS];
    INT16	ai2VOffset[MPEG2_PANSCAN_VECTOR_NS];
} MEPG2_PIC_HDR_T;

typedef enum
{
  MPEG2_DPB_STATUS_EMPTY = 0,   // Free
  MPEG2_DPB_STATUS_READY,         // After Get          
  MPEG2_DPB_STATUS_DECODING,   // After Lock                
  MPEG2_DPB_STATUS_DECODED,     // After UnLock
  MPEG2_DPB_STATUS_OUTPUTTED,     // After Output
  MPEG2_DPB_STATUS_FLD_DECODED,   // After 1fld UnLock
  MPEG2_DPB_STATUS_DEC_REF,
  MPEG2_DPB_STATUS_OUT_REF,  
}MPEG2_DPB_COND_T;

typedef enum
{
  MPEG2_DPB_FBUF_UNKNOWN = VDEC_FB_ID_UNKNOWN,
  MPEG2_DPB_FREF_FBUF = 0,
  MPEG2_DPB_BREF_FBUF = 1,                   
  MPEG2_DPB_WORKING_FBUF = 2,                   
}MPEG2_DPB_IDX_T;


typedef struct _MPEG2_DBP_INFO_T
{
    BOOL fgVirtualDec;
    UCHAR ucDpbFbId;
    MPEG2_DPB_COND_T eDpbStatus;
    UINT64 u8Pts;
    UINT64 u8Offset;
}MPEG2_DBP_INFO_T;

typedef struct _MEPG2_DEC_PRM_T
{
    UCHAR ucSeqIErrCnt;                 //Count of continous I fram error
    UCHAR ucDecFld;
    UCHAR uc2ndFld;
    #if CONFIG_DRV_DVD_24P_SUPPORT
    UINT8  u1DVD24PSeqStep;
    BOOL   fgValidDVD24P;
    #endif
    BOOL fgLoadIntraMatrix;		//  load_intra_quantizer_matrix;
    BOOL fgLoadNonIntraMatrix;	//  load_non_intra_quantizer_matrix;    
    UINT32 *pu4IntraMatrix;
    UINT32 *pu4NonIntraMatrix;
    UINT32 u4MatrixId;
    UINT32 u4BitCount;
    INT64 i8BasePTS;    
    UINT32 u4PreFrmTempRef;
    UINT64 u8PreFrmOffset;
    INT64 i8LatestRealPTS;    
    UINT32 u4LatestRealTempRef;
    INT64 i8PredPTS;    
    INT64 i8PTSDuration;
    INT64 u8LastCCPTS;
    UINT32 u4CurrentQMatrixId;
    UINT32 u4QMatrixCounter;
    MPEG2_DPB_IDX_T eDpbOutputId;        
    MPEG2_DBP_INFO_T arMPEG2DpbInfo[MPEG2_DPB_NUM];     // 0: FRef, 1: BRef, 2:Working
    MPEG2_DBP_INFO_T arMPEG2DebInfo[MPEG2_DPB_NUM];     // Deblocking buffer info
    VDEC_NORM_INFO_T *prVDecNormInfo;
    VDEC_PIC_INFO_T *prVDecPicInfo;
    VDEC_FBM_INFO_T    *prVDecFbmInfo;    
    VID_DEC_PB_MODE_T    *prVDecPBInfo;
    MEPG2_SEQ_HDR_T rMPEG2SeqHdr;
    MEPG2_GOP_HDR_T rMPEG2GopHdr;
    MEPG2_PIC_HDR_T rMPEG2PicHdr;
    VDEC_INFO_MPEG_VFIFO_PRM_T rVDecMPEGVFifoPrm;
    VDEC_INFO_MPEG_BS_INIT_PRM_T rVDecMPEGBsInitPrm[2];
    VDEC_INFO_MPEG_QANTMATRIX_T rVDecMPEGIntraQM;
    VDEC_INFO_MPEG_QANTMATRIX_T rVDecMPEGNonIntraQM;
    VDEC_INFO_MPEG_ERR_INFO_T rVDecMPEGErrInfo;
    VDEC_INFO_DEC_PRM_T rVDecNormHalPrm;
#ifdef VDEC_SR_SUPPORT
    UCHAR                    ucGopRefCnt;
    UCHAR                    ucMPEG2CurrSeqId;
    UCHAR                    ucMPEG2CurrGopId;
    UCHAR                    ucMPEG2SeqHdrCnt;
    UCHAR                    ucMPEG2GopHdrCnt;
    UCHAR                    ucMPEG2GopCntOfSeq;
    MEPG2_SEQ_HDR_T  rMPEG2StoreSeqHdr[MPEG2_MAX_SEQ_NUM];
    MEPG2_GOP_HDR_T  rMPEG2StoreGopHdr[MPEG2_MAX_GOP_NUM];
    MPEG2_DBP_INFO_T arMPEG2EDpbInfo[MPEG2_MAX_EDPB_NUM];     // 0: FRef, 1: BRef, 2:Working
    MPEG_DFB_INFO_T arMPEG2DFBInfo[MPEG2_DFB_NUM];
    BOOL fgStoreLoadIntraMatrix[MPEG2_MAX_SEQ_NUM];
    BOOL fgStoreLoadNonIntraMatrix[MPEG2_MAX_SEQ_NUM];
    UINT32 *pu4StoreIntraMatrix[MPEG2_MAX_SEQ_NUM];
    UINT32 *pu4StoreNonIntraMatrix[MPEG2_MAX_SEQ_NUM];
    //UINT32 u4MPEG2DFBIdx;
#endif    
} MEPG2_DRV_INFO_T;

/******************************************************************************
* Function prototype
******************************************************************************/
//extern UINT32 _au4CurrentQMatrixId[MPV_MAX_VLD];
//extern UINT32 _au4QMatrixCounter[MPV_MAX_VLD];

extern INT32 vMPEG2Parser(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VParseType);
extern void vMPEG2SetColorPrimaries(MEPG2_DRV_INFO_T *prMPEG2DrvDecInfo, UINT32 u4ColorPrimaries);
extern void vMPEG2SetSampleAsp(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4MPEG2Asp);
extern void vMPEG2SetFrameTimingInfo(VDEC_ES_INFO_T* prVDecEsInfo, UCHAR ucFrameRate);



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


extern void vMPEG2InitProc(UCHAR ucEsId);
extern INT32 i4MPEG2VParseProc(UCHAR ucEsId, UINT32 u4VParseType);
extern BOOL fgMPEG2VParseChkProc(UCHAR ucEsId);
extern INT32 i4MPEG2UpdInfoToFbg(UCHAR ucEsId);
extern void vMPEG2StartToDecProc(UCHAR ucEsId);
extern void vMPEG2ISR(UCHAR ucEsId);
extern BOOL fgIsMPEG2DecEnd(UCHAR ucEsId);
extern BOOL fgIsMPEG2DecErr(UCHAR ucEsId);
extern BOOL fgMPEG2ResultChk(UCHAR ucEsId);
extern BOOL fgIsMPEG2InsToDispQ(UCHAR ucEsId);
extern BOOL fgIsMPEG2GetFrmToDispQ(UCHAR ucEsId);
extern void vMPEG2EndProc(UCHAR ucEsId);
extern BOOL fgMPEG2FlushDPB(UCHAR ucEsId, BOOL fgWithOutput);    
extern void vMPEG2ReleaseProc(UCHAR ucEsId, BOOL fgResetHW);
extern void vMPEG2SetMcBufAddr(UCHAR ucFbgId, VDEC_ES_INFO_T* prVDecEsInfo);
extern void vMPEG2DpbBufCopy(MEPG2_DRV_INFO_T *prMpeg2DrvInfo, UCHAR ucTarDpbBuf, UCHAR ucSrcDpbBuf);
extern INT32 i4MPEG2OutputProc(UCHAR ucEsId, VDEC_ES_INFO_T* prVDecEsInfo);
extern void vMPEG2SetDownScaleParam(MEPG2_DRV_INFO_T *prMpeg2DrvInfo, BOOL fgEnable);
extern void vFreeMPEG2WorkingArea(VDEC_ES_INFO_T *prVDecEsInfo);
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
extern void vMPEG2SetLetterBoxParam(MEPG2_DRV_INFO_T *prMpeg2DrvInfo);
#endif

#ifdef VDEC_SR_SUPPORT
extern BOOL fgMPEG2GenEDPB(UCHAR ucEsId);
extern BOOL fgMPEG2RestoreEDPB(UCHAR ucEsId, BOOL fgRestore);
extern BOOL fgIsMPEG2GetSRFrmToDispQ(UCHAR ucEsId, BOOL fgSeqEnd, BOOL fgRefPic);
extern void vMPEG2GetSeqFirstTarget(UCHAR ucEsId);
extern void vMPEG2ReleaseSRDrvInfo(UCHAR ucEsId);
extern BOOL fgMPEG2GetDFBInfo(UCHAR ucEsId, void **prDFBInfo);
extern void vMPEG2RestoreSeqInfo(UCHAR ucEsId);
extern BOOL fgMPEG2RvsDone(UCHAR ucEsId);
extern void vMPEG2ReleaseEDPB(UCHAR ucEsId);
extern void pvMPEG2BackupInfo(UCHAR ucEsId);

extern void vMPEG2EDpbPutBuf(VDEC_ES_INFO_T* prVDecEsInfo, MEPG2_DRV_INFO_T *prMpeg2DrvInfo,  UCHAR ucSrcDpbBuf);
extern void vMPEG2EDpbGetBuf(VDEC_ES_INFO_T* prVDecEsInfo, MEPG2_DRV_INFO_T *prMpeg2DrvInfo,  UCHAR ucTarDpbBuf);
extern INT32 i4MPEG2OutputProcSR(VDEC_ES_INFO_T* prVDecEsInfo);
extern void vMPEG2SetDFBInfo(VDEC_ES_INFO_T* prVDecEsInfo, BOOL fgIsRef);
extern void vMPEGUpdPts(VDEC_ES_INFO_T* prVDecEsInfo, MEPG2_DRV_INFO_T *prMpeg2DrvInfo, BOOL fgRealOutput);
extern void vMPEGUpdPtsSR(VDEC_ES_INFO_T* prVDecEsInfo, MEPG2_DRV_INFO_T *prMpeg2DrvInfo, BOOL fgRealOutput);
#endif

extern VDEC_DRV_IF* VDec_GetMPEG2If(void);

#ifdef MPV_DUMP_FBUF
extern void VDec_Dump_Data(UINT32 u4StartAddr, UINT32 u4FileSize, UINT32 u4FileCnt, UCHAR* pucAddStr);
#endif

#ifdef DRV_VDEC_SUPPORT_FBM_OVERLAY
extern BOOL fgMPEG2NeedDoDscl(UCHAR ucEsId);
#endif

#endif
