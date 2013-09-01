

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   video_custom_sp.h
 *
 * Project:
 * --------
 *  ALPS
 *
 * Description:
 * ------------
 *   This file declared the customerized interface for smart phone
 *
 * Author:
 * -------
 *   Steve Su (mtk01898)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Log$
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#ifndef VIDEO_CUSTOM_SP_H
#define VIDEO_CUSTOM_SP_H

//#include "video_codec_if_sp.h"
#include "vcodec_if.h"

#define ASSERT(expr)                                            \
    do {                                                        \
        if (!(expr))                                            \
            AssertionFailed(__FUNCTION__,__FILE__, __LINE__);   \
    } while (0)

/******************************************************************************
 *
 *
 * decode
 *
 *
******************************************************************************/

typedef enum
{
    CODEC_DEC_NONE      = 0,
    CODEC_DEC_H263      = (0x1<<0),
    CODEC_DEC_MPEG4     = (0x1<<1),
    CODEC_DEC_H264      = (0x1<<2),
    CODEC_DEC_RV        = (0x1<<3),
    CODEC_DEC_VC1       = (0x1<<4),
    CODEC_DEC_VP8       = (0x1<<5),
    CODEC_DEC_MAX       = (0x1<<6)
} VIDEO_DECODER_T;

//VIDEO_DEC_API_T  *GetDecoderAPI(VIDEO_DECODER_T, HANDLE); // HANDLE : wrapper's handle
//VCODEC_DEC_API_T *GetMPEG4DecoderAPI(void);
//VCODEC_DEC_API_T *GetH264DecoderAPI(void);
//VCODEC_DEC_API_T *GetRVDecoderAPI(void);
//VCODEC_DEC_API_T *GetVP8DecoderAPI(void);
//VCODEC_DEC_API_T *GetVC1DecoderAPI(void);


/******************************************************************************
*
*
* encode
*
*
******************************************************************************/

typedef enum
{
    CODEC_ENC_NONE      = 0,
    CODEC_ENC_H263      = (0x1<<0),
    CODEC_ENC_MPEG4     = (0x1<<1),
    CODEC_ENC_H264      = (0x1<<2),
    CODEC_ENC_VP8       = (0x1<<5),
    CODEC_ENC_MAX       = (0x1<<6)
} VIDEO_ENCODER_T;

VIDEO_ENC_API_T *GetEncoderAPI(VIDEO_ENCODER_T eEncType, HANDLE hWrapper, void **ppDrvModule, unsigned int bUseMultiCoreCodec);
VCODEC_ENC_API_T *GetMPEG4EncoderAPI(void);
//VCODEC_ENC_API_T* GetH264EncoderAPI(void);
//VIDEO_ENCODER_API_T *GetVP8EncoderAPI(void);


#endif /* VIDEO_CUSTOM_SP_H */
