
#ifndef _VDEC_DRV_VP8_INFO_H_
#define _VDEC_DRV_VP8_INFO_H_

#include "drv_common.h"

#include "vdec_common_if.h"
#include "vdec_usage.h"

#include "vdec_info_vp8.h"
#include "vdec_info_common.h"



/******************************************************************************
* Local definition
******************************************************************************/
/// VP8 Decoder Picture Buffer Number
#define VP8_DPB_NUM 4

/*! \name Struct for VP8 Driver Info
* @{
*/
/// Decoded Picture Buffer Status Management
typedef enum
{
  VP8_DPB_STATUS_EMPTY = 0,         ///< DPB is  Empty and Free
  VP8_DPB_STATUS_READY,               ///<  DPB is ready for decoding
  VP8_DPB_STATUS_DECODING,         ///< DPB is used by decoder
  VP8_DPB_STATUS_DECODED,          ///< DPB contains decoded frame
  VP8_DPB_STATUS_OUTPUTTED,       ///< DPB was sent to display queue
  VP8_DPB_STATUS_FLD_DECODED,   ///< 1st field wad decoded
  VP8_DPB_STATUS_DEC_REF,           ///< DBP is used for reference
  VP8_DPB_STATUS_OUT_REF,           ///< DBP was displayed and is used for reference
}VP8_DPB_COND_T;

/// Decoded Picture Buffer Index
typedef enum
{
  VP8_DPB_FBUF_UNKNOWN = VDEC_FB_ID_UNKNOWN,
  VP8_DPB_GOLD_FBUF = 0,
  VP8_DPB_LAST_FBUF = 1,
  VP8_DPB_ALT_FBUF= 2,
  VP8_DPB_WORKING_FBUF= 3,
}VP8_DPB_IDX_T;

/// Decoded Picture Buffer Information
typedef struct _VP8_DBP_INFO_T
{
    BOOL fgVirtualDec;
    UCHAR ucDpbFbId;
    VP8_DPB_COND_T eDpbStatus;
}VP8_DBP_INFO_T;
/*! @} */

/*! \name VP8 Driver Information
* @{
*/
/// \ingroup API
typedef struct _VP8_DRV_INFO_T
{   
    UINT32 u4BitCount;                 ///< Consumed bits counter
    INT64 i8BasePTS;
    INT64 i8LatestRealPTS;
    INT64 i8DiffCnt;
    INT64 i8DropIPCnt;
    INT64 i8DropBCnt;
    INT64 i8DummyCnt;   
    VP8_DPB_IDX_T          eDpbOutputId;         ///< DPB output index: Unknown, FRef, BFre, Working
    VP8_DBP_INFO_T        arVP8DpbInfo[VP8_DPB_NUM];     //Deblocking buffer info
    VDEC_INFO_VP8_FRM_HDR_T rVDecVP8FrmHdr;
    VDEC_NORM_INFO_T  *prVDecNormInfo;      ///< Normal info: CodecType, EsId, VldId, BsId, LastPicTp, etc.
    VDEC_PIC_INFO_T     *prVDecPicInfo;             ///< Picture info: PicStruct, PicType, PicW, PicH, FifoStart, etc.
    VDEC_FBM_INFO_T     *prVDecFbmInfo;
//    VID_DEC_SEQUENCE_INFO_T        rPrevSeqInfo;           ///< Visual object Sequence Header    
    VDEC_INFO_VP8_VFIFO_PRM_T     rVDecVP8VFifoPrm;      ///< FifoSa, FifoEa
    VDEC_INFO_VP8_ERR_INFO_T       rVDecVP8ErrInfo;              ///< Erro Info
    VDEC_INFO_DEC_PRM_T               rVDecNormHalPrm;                        ///< HAL parameter: picstruct, pictype, picW, picH
    VDEC_VP8_UPDST_T rVDecStatusUpdate;
    UCHAR ucLastOutFbId;
#ifdef VDEC_SUPPORT_HIGH_FRAME_RATE
    BOOL   fgSeqUnknownFrameRate;
    BOOL   fgUseHighFrameRate;
    UINT32 u4RealFrameDuration;
#endif
} VP8_DRV_INFO_T;
/*! @} */

/******************************************************************************
* Function prototype
******************************************************************************/
/// \ingroup Header
extern INT32  vVP8Parser(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VParseType);
//extern void vSetVP8InitValue(VDEC_ES_INFO_T* prVDecEsInfo);


/*! \name VP8 Driver API
* @{
*/
/// \ingroup API 
/// This function allocates memory for driver information
/// - This API can should be called in the begining.
/// .
/// \return None.
extern void vVP8InitProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function parse VP8 bitstream headers
/// - This API can should be called before starting to decode.
/// .
/// \return parsing result. Please reference to Vdec_errcode.h
extern INT32 i4VP8VParseProc(
                       UCHAR ucEsId,     ///< [IN] the ID of the elementary stream
                       UINT32 u4VParseType ///< [IN] the specified pic type or header
);

/// This function checks the parsing result.
/// \return VDEC_NONE_ERROR: parse OK. Else: please reference to Vdec_errcode.h
extern BOOL fgVP8VParseChkProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function updates information to Frame Buffer Group
/// \return VDEC_NONE_ERROR: udate OK. Else: please reference to Vdec_errcode.h
extern INT32 i4VP8UpdInfoToFbg(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function triggers Video decoder hardware
///\ return None.
extern void vVP8StartToDecProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function is interrrupt service routine
///\ return None.
extern void vVP8ISR(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);
/// This function checks the decoded picture is complete or not
/// - This API will check decoded picture in MB_X and MB_Y
/// .
///\ return TRUE: decode OK, FALSE: decoding failed, drop it
extern BOOL fgIsVP8DecEnd(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function checks the decoded error type and count
/// - This API will check decoded error type and count
/// .
///\ return TRUE: decode error, FALSE: decode correct
extern BOOL fgIsVP8DecErr(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function checks is there any error code happened in HW
///\ return TRUE: decode OK, FALSE: decoding failed, drop it
extern BOOL fgVP8ResultChk(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function sets up frame buffer index ready for display
///\ return TRUE: buffer index for display OK, FALSE: cannot get buffer for display
extern BOOL fgIsVP8InsToDispQ(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function gets one decoded frame buffer and send to display
///\ return TRUE: get frame buffer OK, FALSE: Cannot get buffer for display
extern BOOL fgIsVP8GetFrmToDispQ(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function does ending procedure
///\ return None.
extern void vVP8EndProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);
/// This function flushes all decoded picture in DPB Buffer
///\ return TRUE: flush OK, FALSE: cannot get buffer for display
extern BOOL fgVP8FlushDPB(
                       UCHAR ucEsId,     ///< [IN] the ID of the elementary stream
                       BOOL fgWithOutput   ///< [IN] '1' indicates to output decoded frame
);
/// This function will release resources accupied by decoder
///\ return None.
extern void vVP8ReleaseProc(
                       UCHAR ucEsId,     ///< [IN] the ID of the elementary stream
                       BOOL fgResetHW
);

extern UINT32 u4Vp8DecInit(VDEC_INFO_VP8_FRM_HDR_T *pVp8DecInfo);
extern void vVP8SetMcBufAddr(UCHAR ucFbgId, VDEC_ES_INFO_T* prVDecEsInfo);
extern void vVP8UpdateBufInfo(VDEC_ES_INFO_T* prVDecEsInfo);
extern VOID vVp8DecFinish(VDEC_INFO_VP8_FRM_HDR_T *pVp8DecInfo);
extern void vVP8DpbBufCopy(VP8_DRV_INFO_T *prVP8DrvInfo, UCHAR ucTarDpbBuf, UCHAR ucSrcDpbBuf);
extern INT32 i4VP8OutputProc(UCHAR ucEsId, VDEC_ES_INFO_T* prVDecEsInfo);
extern void vVP8SetColorPrimaries(VP8_DRV_INFO_T *prVP8DrvDecInfo, UINT32 u4ColorPrimaries);
extern void vVP8SetSampleAsp(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VP8Asp);
extern void vVP8SetFrameTimingInfo(VP8_DRV_INFO_T *prVP8DrvDecInfo, UINT16 u2FrameRate);
extern void _VP8SetDecPrm(VP8_DRV_INFO_T *prVP8DrvInfo);
extern void vVP8CalGmcMv(VP8_DRV_INFO_T *prVP8DrvInfo);
#ifdef FBM_ALLOC_SUPPORT
extern void vFreeVP8WorkingArea(VDEC_ES_INFO_T *prVDecEsInfo);
#endif

extern void vVP8SetDownScaleParam(VP8_DRV_INFO_T *prVP8DrvInfo, BOOL fgEnable);
#ifdef DRV_VDEC_SUPPORT_FBM_OVERLAY
extern BOOL fgVP8NeedDoDscl(UCHAR ucEsId);
#endif

/// This function will return vdec driver interface pointer
///\ return VDEC_DRV_IF*
#ifdef VDEC_VP8_SUPPORT
extern VDEC_DRV_IF* VDec_GetVP8If(void);
#endif
/*! @} */

#endif
