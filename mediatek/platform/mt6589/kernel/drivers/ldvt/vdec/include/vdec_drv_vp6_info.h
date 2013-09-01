
#ifndef _VDEC_DRV_VP6_INFO_H_
#define _VDEC_DRV_VP6_INFO_H_

#include "drv_common.h"

#include "vdec_common_if.h"
#include "vdec_usage.h"

#include "vdec_info_vp6.h"
#include "vdec_info_common.h"



/******************************************************************************
* Local definition
******************************************************************************/
/// VP6 Decoder Picture Buffer Number
#define VP6_DPB_NUM 3

/*! \name Struct for VP6 Driver Info
* @{
*/
/// Decoded Picture Buffer Status Management
typedef enum
{
  VP6_DPB_STATUS_EMPTY = 0,         ///< DPB is  Empty and Free
  VP6_DPB_STATUS_READY,               ///<  DPB is ready for decoding
  VP6_DPB_STATUS_DECODING,         ///< DPB is used by decoder
  VP6_DPB_STATUS_DECODED,          ///< DPB contains decoded frame
  VP6_DPB_STATUS_OUTPUTTED,       ///< DPB was sent to display queue
  VP6_DPB_STATUS_FLD_DECODED,   ///< 1st field wad decoded
  VP6_DPB_STATUS_DEC_REF,           ///< DBP is used for reference
  VP6_DPB_STATUS_OUT_REF,           ///< DBP was displayed and is used for reference
}VP6_DPB_COND_T;

/// Decoded Picture Buffer Index
typedef enum
{
  VP6_DPB_FBUF_UNKNOWN = VDEC_FB_ID_UNKNOWN,
  VP6_DPB_GOLD_FBUF = 0,
  VP6_DPB_PREV_FBUF = 1,
  VP6_DPB_WORKING_FBUF= 2,
}VP6_DPB_IDX_T;

/// Decoded Picture Buffer Information
typedef struct _VP6_DBP_INFO_T
{
    BOOL fgVirtualDec;
    UCHAR ucDpbFbId;
    VP6_DPB_COND_T eDpbStatus;
}VP6_DBP_INFO_T;
/*! @} */

/*! \name VP6 Driver Information
* @{
*/
/// \ingroup API
typedef struct _VP6_DEC_PRM_T
{   
    UINT32 u4BitCount;                 ///< Consumed bits counter
    INT64 i8BasePTS;
    INT64 i8LatestRealPTS;
    INT64 i8DiffCnt;
    INT64 i8DropIPCnt;
    INT64 i8DropBCnt;
    INT64 i8DummyCnt;   
    VP6_DPB_IDX_T          eDpbOutputId;         ///< DPB output index: Unknown, FRef, BFre, Working
    VP6_DBP_INFO_T        arVP6DpbInfo[VP6_DPB_NUM];     //Deblocking buffer info
    VDEC_INFO_VP6_FRM_HDR_T rVDecVP6FrmHdr;
    VDEC_NORM_INFO_T  *prVDecNormInfo;      ///< Normal info: CodecType, EsId, VldId, BsId, LastPicTp, etc.
    VDEC_PIC_INFO_T     *prVDecPicInfo;             ///< Picture info: PicStruct, PicType, PicW, PicH, FifoStart, etc.
    VDEC_FBM_INFO_T     *prVDecFbmInfo;
   // VID_DEC_SEQUENCE_INFO_T        rPrevSeqInfo;           ///< Visual object Sequence Header    
    VDEC_INFO_VP6_VFIFO_PRM_T     rVDecVP6VFifoPrm;      ///< FifoSa, FifoEa
    VDEC_INFO_VP6_BS_INIT_PRM_T  rVDecVP6BsInitPrm[2];  ///< Barrel Shifter: Rd pointer, Wr pointer,  FifoSa, FifoEa
    VDEC_INFO_VP6_ERR_INFO_T       rVDecVP6ErrInfo;              ///< Erro Info
    VDEC_INFO_DEC_PRM_T               rVDecNormHalPrm;                        ///< HAL parameter: picstruct, pictype, picW, picH
    UCHAR ucLastOutFbId;
#ifdef VDEC_SUPPORT_HIGH_FRAME_RATE
    BOOL   fgSeqUnknownFrameRate;
    BOOL   fgUseHighFrameRate;
    UINT32 u4RealFrameDuration;
#endif
} VP6_DRV_INFO_T;
/*! @} */

/******************************************************************************
* Function prototype
******************************************************************************/
/// \ingroup Header
//extern INT32  vVp6Parser(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VParseType);
//extern void vSetVp6InitValue(VDEC_ES_INFO_T* prVDecEsInfo);


/*! \name VP6 Driver API
* @{
*/
/// \ingroup API 
/// This function allocates memory for driver information
/// - This API can should be called in the begining.
/// .
/// \return None.
extern void vVP6InitProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function parse vp6 bitstream headers
/// - This API can should be called before starting to decode.
/// .
/// \return parsing result. Please reference to Vdec_errcode.h
extern INT32 i4VP6VParseProc(
                       UCHAR ucEsId,     ///< [IN] the ID of the elementary stream
                       UINT32 u4VParseType ///< [IN] the specified pic type or header
);

/// This function checks the parsing result.
/// \return VDEC_NONE_ERROR: parse OK. Else: please reference to Vdec_errcode.h
extern BOOL fgVP6VParseChkProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function updates information to Frame Buffer Group
/// \return VDEC_NONE_ERROR: udate OK. Else: please reference to Vdec_errcode.h
extern INT32 i4VP6UpdInfoToFbg(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function triggers Video decoder hardware
///\ return None.
extern void vVP6StartToDecProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function is interrrupt service routine
///\ return None.
extern void vVP6ISR(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);
/// This function checks the decoded picture is complete or not
/// - This API will check decoded picture in MB_X and MB_Y
/// .
///\ return TRUE: decode OK, FALSE: decoding failed, drop it
extern BOOL fgIsVP6DecEnd(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function checks the decoded error type and count
/// - This API will check decoded error type and count
/// .
///\ return TRUE: decode error, FALSE: decode correct
extern BOOL fgIsVP6DecErr(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function checks is there any error code happened in HW
///\ return TRUE: decode OK, FALSE: decoding failed, drop it
extern BOOL fgVP6ResultChk(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function sets up frame buffer index ready for display
///\ return TRUE: buffer index for display OK, FALSE: cannot get buffer for display
extern BOOL fgIsVP6InsToDispQ(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function gets one decoded frame buffer and send to display
///\ return TRUE: get frame buffer OK, FALSE: Cannot get buffer for display
extern BOOL fgIsVP6GetFrmToDispQ(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function does ending procedure
///\ return None.
extern void vVP6EndProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);
/// This function flushes all decoded picture in DPB Buffer
///\ return TRUE: flush OK, FALSE: cannot get buffer for display
extern BOOL fgVP6FlushDPB(
                       UCHAR ucEsId,     ///< [IN] the ID of the elementary stream
                       BOOL fgWithOutput   ///< [IN] '1' indicates to output decoded frame
);
/// This function will release resources accupied by decoder
///\ return None.
extern void vVP6ReleaseProc(
                       UCHAR ucEsId,      ///< [IN] the ID of the elementary stream
                       BOOL fgResetHW
);

extern INT32 vVP6Parser(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VParseType);
extern void vVP6SetMcBufAddr(UCHAR ucFbgId, VDEC_ES_INFO_T* prVDecEsInfo);
extern void vVP6DpbBufCopy(VP6_DRV_INFO_T *prVp6DrvInfo, UCHAR ucTarDpbBuf, UCHAR ucSrcDpbBuf);
extern INT32 i4VP6OutputProc(UCHAR ucEsId, VDEC_ES_INFO_T* prVDecEsInfo);
extern void vVP6SetColorPrimaries(VP6_DRV_INFO_T *prVP6DrvDecInfo, UINT32 u4ColorPrimaries);
extern void vVP6SetSampleAsp(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VP6Asp);
extern void vVP6SetFrameTimingInfo(VP6_DRV_INFO_T *prVP6DrvDecInfo, UINT16 u2FrameRate);
extern void _VP6SetDecPrm(VP6_DRV_INFO_T *prVp6DrvInfo);
extern void vVP6CalGmcMv(VP6_DRV_INFO_T *prVp6DrvInfo);
#ifdef FBM_ALLOC_SUPPORT
extern void vFreeVp6WorkingArea(VDEC_ES_INFO_T *prVDecEsInfo);
#endif
extern void vVP6SetDownScaleParam(VP6_DRV_INFO_T *prVp6DrvInfo, BOOL fgEnable);

#ifdef DRV_VDEC_SUPPORT_FBM_OVERLAY
extern BOOL fgVP6NeedDoDscl(UCHAR ucEsId);
#endif

/// This function will return vdec driver interface pointer
///\ return VDEC_DRV_IF*
extern VDEC_DRV_IF* VDec_GetVP6If(void);
/*! @} */

#endif
