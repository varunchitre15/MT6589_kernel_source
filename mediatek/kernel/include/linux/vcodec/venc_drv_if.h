

/** 
 * @file 
 *   venc_drv_if.h
 *
 * @par Project:
 *   MFlexVideo 
 *
 * @par Description:
 *   video encode driver interface
 *
 * @par Author:
 *   Jackal Chen (mtk02532)
 *
 * @par $Revision: #29 $
 * @par $Modtime:$
 * @par $Log:$
 *
 */

#ifndef __VENC_DRV_IF_H__
#define __VENC_DRV_IF_H__

/*=============================================================================
 *                              Include Files
 *===========================================================================*/

#include "val_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 *                              Type definition
 *===========================================================================*/

/**
 * @par Enumeration
 *   VENC_DRV_QUERY_TYPE_T
 * @par Description
 *   This is the item used for query driver
 */ 
typedef enum __VENC_DRV_QUERY_TYPE_T
{
    VENC_DRV_QUERY_TYPE_NONE,               ///<: Default value (not used)
    VENC_DRV_QUERY_TYPE_VIDEO_FORMAT,       ///<: Query the driver capability 
    VENC_DRV_QUERY_TYPE_PROPERTY,           ///<: Get the driver property
    VENC_DRV_QUERY_TYPE_MCI_SUPPORTED,   		// <: Query if the codec support MCI
    VENC_DRV_QUERY_TYPE_CHIP_NAME,          ///<: Query chip name
    VENC_DRV_QUERY_TYPE_MAX = 0xFFFFFFFF    ///<: Max VENC_DRV_QUERY_TYPE_T value
} VENC_DRV_QUERY_TYPE_T;

/**
 * @par Enumeration
 *   VENC_DRV_YUV_FORMAT_T
 * @par Description
 *   This is the item used for input YUV buffer format 
 */ 
typedef enum __VENC_DRV_YUV_FORMAT_T
{
    VENC_DRV_YUV_FORMAT_NONE,               ///<: Default value (not used)
    VENC_DRV_YUV_FORMAT_GRAY,               ///<: GRAY YUV format
    VENC_DRV_YUV_FORMAT_422,                ///<: 422 YUV format
    VENC_DRV_YUV_FORMAT_420,                ///<: 420 YUV format
    VENC_DRV_YUV_FORMAT_411,                ///<: 411 YUV format
    VENC_DRV_YUV_FORMAT_YV12,               ///<: YV12 YUV format
    VENC_DRV_YUV_FORMAT_MAX = 0xFFFFFFFF    ///<: Max VENC_DRV_YUV_FORMAT_T value
} VENC_DRV_YUV_FORMAT_T;

/**
 * @par Enumeration
 *   VENC_DRV_VIDEO_FORMAT_T
 * @par Description
 *   This is the item used for encode video format
 */ 
typedef enum __VENC_DRV_VIDEO_FORMAT_T
{
    VENC_DRV_VIDEO_FORMAT_NONE,             ///<: Default value (not used)
    VENC_DRV_VIDEO_FORMAT_MPEG4,            ///<: MPEG4 video format
    VENC_DRV_VIDEO_FORMAT_MPEG4_SHORT,      ///<: MPEG4_SHORT (H.263 baseline profile) video format
    VENC_DRV_VIDEO_FORMAT_H263,             ///<: H.263 video format
    VENC_DRV_VIDEO_FORMAT_H264,             ///<: H.264 video format
    VENC_DRV_VIDEO_FORMAT_WMV9,             ///<: WMV9 video format
    VENC_DRV_VIDEO_FORMAT_VC1,              ///<: VC1 video format
    VENC_DRV_VIDEO_FORMAT_VP8,              ///<: VP8 video format
    VENC_DRV_VIDEO_FORMAT_MAX = 0xFFFFFFFF  ///<: Max VENC_DRV_VIDEO_FORMAT_T value
} VENC_DRV_VIDEO_FORMAT_T;

/**
 * @par Enumeration
 *   VENC_DRV_FRAME_RATE_T
 * @par Description
 *   This is the item used for encode frame rate
 */ 
typedef enum __VENC_DRV_FRAME_RATE_T
{
    VENC_DRV_FRAME_RATE_NONE    = 0,          ///<: Default value (not used)
    VENC_DRV_FRAME_RATE_7_5     = 75,         ///<: 7.5
    VENC_DRV_FRAME_RATE_10      = 10,         ///<: 10
    VENC_DRV_FRAME_RATE_15      = 15,         ///<: 15
    VENC_DRV_FRAME_RATE_20      = 20,         ///<: 20
    VENC_DRV_FRAME_RATE_25      = 25,         ///<: 25
    VENC_DRV_FRAME_RATE_29_97   = 2997,       ///<: 29.97
    VENC_DRV_FRAME_RATE_30      = 30,         ///<: 30
    VENC_DRV_FRAME_RATE_60      = 60,         ///<: 60
    VENC_DRV_FRAME_RATE_MAX     = 0xFFFFFFFF  ///<: Max VENC_DRV_FRAME_RATE_T value
} VENC_DRV_FRAME_RATE_T;

/**
 * @par Enumeration
 *   VENC_DRV_START_OPT_T
 * @par Description
 *   This is the item used for encode frame type
 */ 
typedef enum __VENC_DRV_START_OPT_T
{
    VENC_DRV_START_OPT_NONE,                    ///<: Default value (not used)
    VENC_DRV_START_OPT_ENCODE_SEQUENCE_HEADER,  ///<: Encode a Sequence header
    VENC_DRV_START_OPT_ENCODE_SEQUENCE_HEADER_H264_SPS,     ///<: Encode a Sequence header H264 SPS
    VENC_DRV_START_OPT_ENCODE_SEQUENCE_HEADER_H264_PPS,     ///<: Encode a Sequence header H264 PPS
    VENC_DRV_START_OPT_ENCODE_FRAME,            ///<: Encode a frame
    VENC_DRV_START_OPT_ENCODE_KEY_FRAME,        ///<: Encode a key frame
    VENC_DRV_START_OPT_ENCODE_FINAL,            ///<: Final encode (Only use to encode final frame)
    VENC_DRV_START_OPT_MAX = 0xFFFFFFFF         ///<: Max VENC_DRV_START_OPT_T value
} VENC_DRV_START_OPT_T;

/**
 * @par Enumeration
 *   VENC_DRV_MESSAGE_T
 * @par Description
 *   This is the item used for encode frame status
 */ 
typedef enum __VENC_DRV_MESSAGE_T
{
    VENC_DRV_MESSAGE_NONE,                  ///<: Default value (not used)
    VENC_DRV_MESSAGE_OK,                    ///<: Encode ok
    VENC_DRV_MESSAGE_ERR,                   ///<: Encode error
    VENC_DRV_MESSAGE_TIMEOUT,               ///<: Encode timeout
    VENC_DRV_MESSAGE_MAX = 0xFFFFFFFF       ///<: Max VENC_DRV_MESSAGE_T value
} VENC_DRV_MESSAGE_T;

/**
 * @par Enumeration
 *   VENC_DRV_H264_VIDEO_PROFILE_T
 * @par Description
 *   This is the item used for h.264 encoder profile capability
 */ 
typedef enum __VENC_DRV_H264_VIDEO_PROFILE_T
{
	VENC_DRV_H264_VIDEO_PROFILE_UNKNOWN					= 0,            ///<: Default value (not used)
	VENC_DRV_H264_VIDEO_PROFILE_BASELINE				= (1<<0),       ///<: Baseline
	VENC_DRV_H264_VIDEO_PROFILE_CONSTRAINED_BASELINE    = (1<<1),       ///<: Constrained Baseline
	VENC_DRV_H264_VIDEO_PROFILE_MAIN					= (1<<2),       ///<: Main
	VENC_DRV_H264_VIDEO_PROFILE_EXTENDED				= (1<<3),       ///<: Extended
	VENC_DRV_H264_VIDEO_PROFILE_HIGH					= (1<<4),       ///<: High
	VENC_DRV_H264_VIDEO_PROFILE_HIGH_10				    = (1<<5),       ///<: High 10
	VENC_DRV_H264_VIDEO_PROFILE_HIGH422				    = (1<<6),       ///<: High 422
	VENC_DRV_H264_VIDEO_PROFILE_HIGH444				    = (1<<7),       ///<: High 444
	VENC_DRV_H264_VIDEO_PROFILE_HIGH_10_INTRA			= (1<<8),       ///<: High 10 Intra (Amendment 2)
	VENC_DRV_H264_VIDEO_PROFILE_HIGH422_INTRA			= (1<<9),       ///<: High 422 Intra (Amendment 2)
	VENC_DRV_H264_VIDEO_PROFILE_HIGH444_INTRA			= (1<<10),      ///<: High 444 Intra (Amendment 2)
	VENC_DRV_H264_VIDEO_PROFILE_CAVLC444_INTRA			= (1<<11),      ///<: CAVLC 444 Intra (Amendment 2)
	VENC_DRV_H264_VIDEO_PROFILE_HIGH444_PREDICTIVE		= (1<<12),      ///<: High 444 Predictive (Amendment 2)
	VENC_DRV_H264_VIDEO_PROFILE_SCALABLE_BASELINE		= (1<<13),      ///<: Scalable Baseline (Amendment 3)
	VENC_DRV_H264_VIDEO_PROFILE_SCALABLE_HIGH			= (1<<14),      ///<: Scalable High (Amendment 3)
	VENC_DRV_H264_VIDEO_PROFILE_SCALABLE_HIGH_INTRA     = (1<<15),      ///<: Scalable High Intra (Amendment 3)
	VENC_DRV_H264_VIDEO_PROFILE_MULTIVIEW_HIGH			= (1<<16),      ///<: Multiview High (Corrigendum 1 (2009))
    VENC_DRV_H264_VIDEO_PROFILE_MAX                     = 0xFFFFFFFF    ///<: Max VENC_DRV_H264_VIDEO_PROFILE_T value
}VENC_DRV_H264_VIDEO_PROFILE_T;

/**
 * @par Enumeration
 *   VENC_DRV_MPEG_VIDEO_PROFILE_T
 * @par Description
 *   This is the item used for h.263, mpeg2, mpeg4 encoder profile capability
 */
typedef enum __VENC_DRV_MPEG_VIDEO_PROFILE_T
{
    VENC_DRV_MPEG_VIDEO_PROFILE_UNKNOWN					= 0,            ///<: Default value (not used)
	VENC_DRV_MPEG_VIDEO_PROFILE_H263_0                  = (1<<0),       ///<: H.263 0
	VENC_DRV_MPEG_VIDEO_PROFILE_H263_1                  = (1<<1),       ///<: H.263 1
	VENC_DRV_MPEG_VIDEO_PROFILE_H263_2                  = (1<<2),       ///<: H.263 2
	VENC_DRV_MPEG_VIDEO_PROFILE_H263_3                  = (1<<3),       ///<: H.263 3
	VENC_DRV_MPEG_VIDEO_PROFILE_H263_4                  = (1<<4),       ///<: H.263 4
	VENC_DRV_MPEG_VIDEO_PROFILE_H263_5                  = (1<<5),       ///<: H.263 5
	VENC_DRV_MPEG_VIDEO_PROFILE_H263_6                  = (1<<6),       ///<: H.263 6
	VENC_DRV_MPEG_VIDEO_PROFILE_H263_7                  = (1<<7),       ///<: H.263 7
	VENC_DRV_MPEG_VIDEO_PROFILE_H263_8                  = (1<<8),       ///<: H.263 8
	VENC_DRV_MPEG_VIDEO_PROFILE_MPEG2_SIMPLE            = (1<<9),       ///<: MPEG2 Simple
	VENC_DRV_MPEG_VIDEO_PROFILE_MPEG2_MAIN              = (1<<10),      ///<: MPEG2 Main
	VENC_DRV_MPEG_VIDEO_PROFILE_MPEG2_SNR               = (1<<11),      ///<: MPEG2 SNR
	VENC_DRV_MPEG_VIDEO_PROFILE_MPEG2_SPATIAL           = (1<<12),      ///<: MPEG2 Spatial
	VENC_DRV_MPEG_VIDEO_PROFILE_MPEG2_HIGH              = (1<<13),      ///<: MPEG2 High
	VENC_DRV_MPEG_VIDEO_PROFILE_MPEG4_SIMPLE            = (1<<14),      ///<: MPEG4 Simple
	VENC_DRV_MPEG_VIDEO_PROFILE_MPEG4_ADVANCED_SIMPLE   = (1<<15),      ///<: MPEG4 Advanced Simple
    VENC_DRV_MPEG_VIDEO_PROFILE_MAX                     = 0xFFFFFFFF    ///<: Max VENC_DRV_MPEG_VIDEO_PROFILE_T value
}VENC_DRV_MPEG_VIDEO_PROFILE_T;

/**
 * @par Enumeration
 *   VENC_DRV_MS_VIDEO_PROFILE_T
 * @par Description
 *   This is the item used for MS encoder profile capability
 */
typedef enum __VENC_DRV_MS_VIDEO_PROFILE_T
{
    VENC_DRV_MS_VIDEO_PROFILE_UNKNOWN	    = 0,            ///<: Default value (not used)
	VENC_DRV_MS_VIDEO_PROFILE_VC1_SIMPLE    = (1<<0),       ///<: VC1 Simple
	VENC_DRV_MS_VIDEO_PROFILE_VC1_MAIN      = (1<<1),       ///<: VC1 Main
	VENC_DRV_MS_VIDEO_PROFILE_VC1_ADVANCED  = (1<<2),       ///<: VC1 Advanced
	VENC_DRV_MS_VIDEO_PROFILE_WMV9_SIMPLE   = (1<<3),       ///<: WMV9 Simple
	VENC_DRV_MS_VIDEO_PROFILE_WMV9_MAIN     = (1<<4),       ///<: WMV9 Main
	VENC_DRV_MS_VIDEO_PROFILE_WMV9_COMPLEX  = (1<<5),       ///<: WMV9 Complex
    VENC_DRV_MS_VIDEO_PROFILE_MAX           = 0xFFFFFFFF    ///<: Max VENC_DRV_MS_VIDEO_PROFILE_T value
} VENC_DRV_MS_VIDEO_PROFILE_T;

/**
 * @par Enumeration
 *   VENC_DRV_VIDEO_LEVEL_T
 * @par Description
 *   This is the item used for encoder level capability
 */
typedef enum __VENC_DRV_VIDEO_LEVEL_T
{
	VENC_DRV_VIDEO_LEVEL_UNKNOWN = 0,       ///<: Default value (not used)
	VENC_DRV_VIDEO_LEVEL_0,				    ///<: VC1
	VENC_DRV_VIDEO_LEVEL_1,				    ///<: H264, VC1, MPEG4
	VENC_DRV_VIDEO_LEVEL_1b,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_1_1,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_1_2,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_1_3,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_2,				    ///<: H264, VC1, MPEG4
	VENC_DRV_VIDEO_LEVEL_2_1,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_2_2,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_3,				    ///<: H264, VC1, MPEG4
	VENC_DRV_VIDEO_LEVEL_3_1,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_3_2,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_4,				    ///<: H264, VC1
	VENC_DRV_VIDEO_LEVEL_4_1,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_4_2,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_5,				    ///<: H264
	VENC_DRV_VIDEO_LEVEL_5_1,			    ///<: H264
	VENC_DRV_VIDEO_LEVEL_LOW,			    ///<: VC1, MPEG2
	VENC_DRV_VIDEO_LEVEL_MEDIUM,		    ///<: VC1, MPEG2
	VENC_DRV_VIDEO_LEVEL_HIGH1440,		    ///<: MPEG2
	VENC_DRV_VIDEO_LEVEL_HIGH,			    ///<: VC1, MPEG2
    VENC_DRV_VIDEO_LEVEL_MAX = 0xFFFFFFFF   ///<: Max VENC_DRV_VIDEO_LEVEL_T value
} VENC_DRV_VIDEO_LEVEL_T;

/**
 * @par Enumeration
 *   VENC_DRV_RESOLUTION_T
 * @par Description
 *   This is the item used for encoder resolution capability
 */
typedef enum __VENC_DRV_RESOLUTION_T
{
	VENC_DRV_RESOLUTION_UNKNOWN = 0,                ///<: Default value (not used)
    VENC_DRV_RESOLUTION_SUPPORT_QCIF,               ///<: CIF
    VENC_DRV_RESOLUTION_SUPPORT_QVGA,               ///<: QVGA
    VENC_DRV_RESOLUTION_SUPPORT_CIF,                ///<: QCIF
    VENC_DRV_RESOLUTION_SUPPORT_HVGA,               ///<: HVGA: 480x320    
    VENC_DRV_RESOLUTION_SUPPORT_VGA,               ///<: VGA: 640x480
    VENC_DRV_RESOLUTION_SUPPORT_480I,               ///<: 480I
    VENC_DRV_RESOLUTION_SUPPORT_480P,               ///<: 480P
    VENC_DRV_RESOLUTION_SUPPORT_576I,               ///<: 576I
    VENC_DRV_RESOLUTION_SUPPORT_576P,               ///<: 480P
    VENC_DRV_RESOLUTION_SUPPORT_FWVGA,               ///<: FWVGA: 864x480    
    VENC_DRV_RESOLUTION_SUPPORT_720I,               ///<: 720I
    VENC_DRV_RESOLUTION_SUPPORT_720P,               ///<: 720P
    VENC_DRV_RESOLUTION_SUPPORT_1080I,              ///<: 1080I
    VENC_DRV_RESOLUTION_SUPPORT_1080P,              ///<: 1080P
    VENC_DRV_RESOLUTION_SUPPORT_MAX = 0xFFFFFFFF    ///<: Max VENC_DRV_RESOLUTION_T value
} VENC_DRV_RESOLUTION_T;

/**
 * @par Enumeration
 *   VENC_DRV_SET_TYPE_T
 * @par Description
 *   This is the input parameter for\n
 *   eVEncDrvSetParam()\n
 */
typedef enum __VENC_DRV_SET_TYPE_T
{
    VENC_DRV_SET_TYPE_UNKONW = 0,       ///<: Default value (not used)
    VENC_DRV_SET_TYPE_RST,              ///<: Set reset
    VENC_DRV_SET_TYPE_CB,	            ///<: Set callback function
    VENC_DRV_SET_TYPE_PARAM_RC,	        ///<: Set rate control parameter
    VENC_DRV_SET_TYPE_PARAM_ME,	        ///<: Set motion estimation parameter
    VENC_DRV_SET_TYPE_PARAM_EIS,		///<: Set EIS parameter
    VENC_DRV_SET_TYPE_PARAM_ENC,	    ///<: Set encoder parameters such as I-frame period, etc.
    VENC_DRV_SET_TYPE_STATISTIC_ON,     ///<: Enable statistic function
    VENC_DRV_SET_TYPE_STATISTIC_OFF,    ///<: Disable statistic function
    VENC_DRV_SET_TYPE_SET_OMX_TIDS,
    VENC_DRV_SET_TYPE_MPEG4_SHORT,
    VENC_DRV_SET_TYPE_FORCE_INTRA_ON,
    VENC_DRV_SET_TYPE_FORCE_INTRA_OFF,
    VENC_DRV_SET_TYPE_TIME_LAPSE,
    VENC_DRV_SET_TYPE_ALLOC_WORK_BUF,
    VENC_DRV_SET_TYPE_FREE_WORK_BUF,
#ifdef MTK_USES_VR_DYNAMIC_QUALITY_MECHANISM    
    VENC_DRV_SET_TYPE_ADJUST_BITRATE,
#endif
    VENC_DRV_SET_TYPE_I_FRAME_INTERVAL,
    VENC_DRV_SET_TYPE_WFD_MODE,         ///<: Set Wifi-Display Mode
    VENC_DRV_SET_TYPE_RECORD_SIZE,
    VENC_DRV_SET_TYPE_USE_MCI_BUF,      ///<: Set to use MCI buffer
    VENC_DRV_SET_TYPE_MAX = 0xFFFFFFFF  ///<: Max VENC_DRV_SET_TYPE_T value
} VENC_DRV_SET_TYPE_T;

/**
 * @par Enumeration
 *   VENC_DRV_GET_TYPE_T
 * @par Description
 *   This is the input parameter for\n 
 *   eVEncDrvGetParam()\n
 */
typedef enum __VENC_DRV_GET_TYPE_T
{
    VENC_DRV_GET_TYPE_UNKONW = 0,       ///<: Default value (not used)
    VENC_DRV_GET_TYPE_PARAM_RC,	        ///<: Get rate control parameter
    VENC_DRV_GET_TYPE_PARAM_ME,	        ///<: Get motion estimation parameter
    VENC_DRV_GET_TYPE_PARAM_EIS,		///<: Get EIS parameter
    VENC_DRV_GET_TYPE_PARAM_ENC,	    ///<: Get encoder parameters such as I-frame period, etc.
    VENC_DRV_GET_TYPE_STATISTIC,     	///<: Get statistic.
    VENC_DRV_GET_TYPE_GET_CPU_LOADING_INFO,       ///< query the cpu loading info from kernel driver
    VENC_DRV_GET_TYPE_GET_YUV_FORMAT, ///<: 
    VENC_DRV_GET_TYPE_MAX = 0xFFFFFFFF  ///<: Max VENC_DRV_GET_TYPE_MAX value
} VENC_DRV_GET_TYPE_T;

/**
 * @par Enumeration
 *   VENC_DRV_MRESULT_T
 * @par Description
 *   This is the return value for\n
 *   eVEncDrvXXX()\n
 */
typedef enum __VENC_DRV_MRESULT_T
{
    VENC_DRV_MRESULT_OK = 0,            ///<: Return Success
    VENC_DRV_MRESULT_FAIL,              ///<: Return Fail
    VENC_DRV_MRESULT_MAX = 0x0FFFFFFF   ///<: Max VENC_DRV_MRESULT_T value
} VENC_DRV_MRESULT_T;

typedef enum __VENC_DRV_COLOR_FORMAT_T
{
  VENC_DRV_COLOR_FORMAT_YUV420,
  VENC_DRV_COLOR_FORMAT_YV12,
} VENC_DRV_COLOR_FORMAT_T;

typedef struct __VENC_DRV_YUV_STRIDE_T
{
    unsigned int u4YStride;
    unsigned int u4UVStride;
} VENC_DRV_YUV_STRIDE_T;

/**
 * @par Structure
 *   VENC_DRV_QUERY_VIDEO_FORMAT_T
 * @par Description
 *   This is a input parameter for\n 
 *   eVEncDrvQueryCapability()\n
 */
typedef struct __VENC_DRV_QUERY_VIDEO_FORMAT_T
{
	VENC_DRV_VIDEO_FORMAT_T eVideoFormat;   ///<: video format capability   
	VAL_UINT32_T            u4Profile;      ///<: video profile capability (VENC_DRV_H264_VIDEO_PROFILE_T, VENC_DRV_MPEG_VIDEO_PROFILE_T, VENC_DRV_MS_VIDEO_PROFILE_T)
	VENC_DRV_VIDEO_LEVEL_T  eLevel;         ///<: video level capability
	VENC_DRV_RESOLUTION_T   eResolution;    ///<: video resolution capability
	VAL_UINT32_T            u4Width;        ///<: video width capability
	VAL_UINT32_T            u4Height;       ///<: video height capability
	VAL_UINT32_T            u4Bitrate;      ///<: video bitrate capability
    VAL_UINT32_T            u4FrameRate;      ///<: video FrameRate capability, 15, 30,...
} VENC_DRV_QUERY_VIDEO_FORMAT_T;

/**
 * @par Structure
 *   P_VENC_DRV_QUERY_VIDEO_FORMAT_T
 * @par Description
 *   This is the pointer of VENC_DRV_QUERY_VIDEO_FORMAT_T
 */
typedef VENC_DRV_QUERY_VIDEO_FORMAT_T   *P_VENC_DRV_QUERY_VIDEO_FORMAT_T;

/**
 * @par Structure
 *   VENC_DRV_PARAM_ENC_T
 * @par Description
 *   This is the encoder settings and used as input or output parameter for\n
 *   eVEncDrvSetParam() or eVEncDrvGetParam()\n
 */
typedef struct __VENC_DRV_PARAM_ENC_T
{
    VENC_DRV_YUV_FORMAT_T   eVEncFormat;            ///<: YUV format
    VAL_UINT32_T            u4Profile;          ///<: Profile
    VAL_UINT32_T            u4Level;            ///<: Level
    VAL_UINT32_T            u4Width;            ///<: Image Width
    VAL_UINT32_T            u4Height;           ///<: Image Height
    VAL_UINT32_T            u4BufWidth;         ///<: Buffer Width
    VAL_UINT32_T            u4BufHeight;        ///<: Buffer Heigh
    VAL_UINT32_T            u4NumPFrm;          ///<: The number of P frame between two I frame.
    VAL_UINT32_T            u4NumBFrm;          ///<: The number of B frame between two reference frame.
    VENC_DRV_FRAME_RATE_T   eFrameRate;         ///<: Frame rate
    VAL_BOOL_T              fgInterlace;        ///<: Interlace coding.    
    VAL_VOID_T              *pvExtraEnc;        ///<: For VENC_DRV_PARAM_ENC_H264_T or ...  
    VAL_MEMORY_T            rExtraEncMem;       ///<: Extra Encoder Memory Info
    VAL_BOOL_T              fgUseMCI;           ///<: Use MCI
} VENC_DRV_PARAM_ENC_T;

/**
 * @par Structure
 *   VENC_DRV_PARAM_ENC_EXTRA_T
 * @par Description
 *   This is the encoder settings and used as input or output parameter for\n
 *   eVEncDrvSetParam() or eVEncDrvGetParam()\n
 */
typedef struct __VENC_DRV_PARAM_ENC_EXTRA_T
{
    VAL_UINT32_T            u4IntraFrameRate;          ///<: Intra frame rate
    VAL_UINT32_T            u4BitRate;                        ///<: BitRate kbps
} VENC_DRV_PARAM_ENC_EXTRA_T, *pVENC_DRV_PARAM_ENC_EXTRA_T;


/**
 * @par Structure
 *   P_VENC_DRV_PARAM_ENC_T
 * @par Description
 *   This is the pointer of VENC_DRV_PARAM_ENC_T
 */
typedef VENC_DRV_PARAM_ENC_T    *P_VENC_DRV_PARAM_ENC_T;

/**
 * @par Structure
 *   VENC_DRV_PARAM_EIS_T
 * @par Description
 *   This is the EIS information and used as input or output parameter for\n
 *   eVEncDrvSetParam() or eVEncDrvGetParam()\n 
 */
typedef struct __VENC_DRV_PARAM_EIS_T
{
    VAL_BOOL_T      fgEISEnable;            ///<: EIS Enable/disable.
    VAL_UINT32_T    u4EISFrameWidth;        ///<: EIS FrameWidth
    VAL_UINT32_T    u4EISFrameHeight;       ///<: EIS FrameHeight
    VAL_UINT32_T    u4GMV_X;                ///<: Golbal Motion Vector (GMV) of the VOP Frame used for EIS
    VAL_UINT32_T    u4GMV_Y;                ///<: Golbal Motion Vector (GMV) of the VOP Frame used for EIS
} VENC_DRV_PARAM_EIS_T;

/**
 * @par Structure
 *   P_VENC_DRV_PARAM_EIS_T
 * @par Description
 *   This is the pointer of VENC_DRV_PARAM_EIS_T
 */
typedef VENC_DRV_PARAM_EIS_T     *P_VENC_DRV_PARAM_EIS_T;

/**
 * @par Structure
 *   VENC_DRV_EIS_INPUT_T
 * @par Description
 *   This is EIS information and used as items for VENC_DRV_PARAM_FRM_BUF_T
 */
typedef struct __VENC_DRV_EIS_INPUT_T
{
    VAL_UINT32_T    u4X;    ///<: Start coordination X
    VAL_UINT32_T    u4Y;    ///<: Start coordination Y
} VENC_DRV_EIS_INPUT_T;

/**
 * @par Structure
 *   P_VENC_DRV_EIS_INPUT_T
 * @par Description
 *   This is the pointer of VENC_DRV_EIS_INPUT_T
 */
typedef VENC_DRV_EIS_INPUT_T     *P_VENC_DRV_EIS_INPUT_T;

/**
 * @par Structure
 *   VENC_DRV_TIMESTAMP_T
 * @par Description
 *   This is timestamp information and used as items for\n
 *   VENC_DRV_PARAM_FRM_BUF_T and VENC_DRV_PARAM_BS_BUF_T
 */
typedef struct __VENC_DRV_TIMESTAMP_T
{
    VAL_UINT32_T    u4TimeStamp[2]; ///<: Timestamp information    
} VENC_DRV_TIMESTAMP_T;

/**
 * @par Structure
 *   P_VENC_DRV_TIMESTAMP_T
 * @par Description
 *   This is the pointer of VENC_DRV_TIMESTAMP_T
 */
typedef VENC_DRV_TIMESTAMP_T     *P_VENC_DRV_TIMESTAMP_T;

/**
 * @par Structure
 *   VENC_DRV_PARAM_FRM_BUF_T
 * @par Description
 *   This is frame buffer information and used as input parameter for\n
 *   eVEncDrvEncode()\n
 */
typedef struct __VENC_DRV_PARAM_FRM_BUF_T
{
    VAL_MEM_ADDR_T          rFrmBufAddr;    ///<: Frame buffer address
    VAL_MEM_ADDR_T          rCoarseAddr;    ///<: Coarse address
    VENC_DRV_TIMESTAMP_T	rTimeStamp;     ///<: Timestamp information
    VENC_DRV_EIS_INPUT_T    rEISInput;      ///<: EIS information
} VENC_DRV_PARAM_FRM_BUF_T;

/**
 * @par Structure
 *   P_VENC_DRV_PARAM_FRM_BUF_T
 * @par Description
 *   This is the pointer of VENC_DRV_PARAM_FRM_BUF_T
 */
typedef VENC_DRV_PARAM_FRM_BUF_T     *P_VENC_DRV_PARAM_FRM_BUF_T;

/**
 * @par Structure
 *   VENC_DRV_PARAM_BS_BUF_T
 * @par Description
 *   This is bitstream buffer information and used as input parameter for\n
 *   eVEncDrvEncode()\n
 */
typedef struct __VENC_DRV_PARAM_BS_BUF_T
{
    VAL_MEM_ADDR_T          rBSAddr;        ///<: Bitstream buffer address
    VAL_UINT32_T            u4BSStartVA;    ///<: Bitstream fill start address
    VAL_UINT32_T            u4BSSize;       ///<: Bitstream size (filled bitstream in bytes)    
    VENC_DRV_TIMESTAMP_T    rTimeStamp;     ///<: Time stamp information
} VENC_DRV_PARAM_BS_BUF_T;

/**
 * @par Structure
 *   P_VENC_DRV_PARAM_BS_BUF_T
 * @par Description
 *   This is the pointer of VENC_DRV_PARAM_BS_BUF_T
 */
typedef VENC_DRV_PARAM_BS_BUF_T     *P_VENC_DRV_PARAM_BS_BUF_T;

/**
 * @par Structure
 *   VENC_DRV_DONE_RESULT_T
 * @par Description
 *   This is callback and return information and used as output parameter for\n
 *   eVEncDrvEncode()\n
 */
typedef struct __VENC_DRV_DONE_RESULT_T
{
    VENC_DRV_MESSAGE_T          eMessage;       ///<: Message, such as success or error code
    P_VENC_DRV_PARAM_BS_BUF_T   prBSBuf;        ///<: Bitstream information
    P_VENC_DRV_PARAM_FRM_BUF_T  prFrmBuf;       ///<: Input frame buffer information. if address is null, don't use this buffer, else reuse
    VAL_BOOL_T			        fgIsKeyFrm;     ///<: output is key frame or not
    VAL_UINT32_T                u4HWEncodeTime; ///<: HW encode Time
} VENC_DRV_DONE_RESULT_T;

/**
 * @par Structure
 *   P_VENC_DRV_DONE_RESULT_T
 * @par Description
 *   This is the pointer of VENC_DRV_DONE_RESULT_T
 */
typedef VENC_DRV_DONE_RESULT_T     *P_VENC_DRV_DONE_RESULT_T;

/**
 * @par Structure
 *   VENC_DRV_PROPERTY_T
 * @par Description
 *   This is property information and used as output parameter for\n
 *   eVEncDrvQueryCapability()\n
 */
typedef struct __VENC_DRV_PROPERTY_T
{
    VAL_UINT32_T    u4BufAlign;         ///<: Buffer alignment requirement
    VAL_UINT32_T    u4BufUnitSize;      ///<: Buffer unit size is N bytes (e.g., 8, 16, or 64 bytes per unit.)
    VAL_UINT32_T    u4ExtraBufSize;     ///<: Extra buffer size in initial stage
    VAL_BOOL_T      fgOutputRingBuf;    ///<: Output is ring buffer
    VAL_BOOL_T      fgCoarseMESupport;  ///<: Support ME coarse search
    VAL_BOOL_T      fgEISSupport;       ///<: Support EIS
} VENC_DRV_PROPERTY_T;

/**
 * @par Structure
 *   P_VENC_DRV_PROPERTY_T
 * @par Description
 *   This is the pointer of VENC_DRV_PROPERTY_T
 */
typedef VENC_DRV_PROPERTY_T     *P_VENC_DRV_PROPERTY_T;

/**
 * @par Structure
 *   VENC_DRV_STATISTIC_T
 * @par Description
 *   This is statistic information and used as output parameter for\n
 *   eVEncDrvGetParam()\n 
 */
typedef struct __VENC_DRV_STATISTIC_T
{
    VAL_UINT32_T    u4EncTimeMax;   ///<: Encode one frame time. Max
    VAL_UINT32_T    u4EncTimeMin;   ///<: Encode one frame time. Min
    VAL_UINT32_T    u4EncTimeAvg;   ///<: Encode one frame time. Average
    VAL_UINT32_T    u4EncTimeSum;   ///<: Encode one frame time. Sum
} VENC_DRV_STATISTIC_T;

/**
 * @par Structure
 *   P_VENC_DRV_STATISTIC_T
 * @par Description
 *   This is the pointer of VENC_DRV_STATISTIC_T
 */
typedef VENC_DRV_STATISTIC_T     *P_VENC_DRV_STATISTIC_T;

/*=============================================================================
 *                             Function Declaration
 *===========================================================================*/

/**
* @par Function       
*   eVEncDrvQueryCapability
* @par Description    
*   Query the driver capability
* @param              
*   a_eType         [IN/OUT]    The VENC_DRV_QUERY_TYPE_T structure
* @param              
*   a_pvInParam     [IN]        The input parameter
* @param              
*   a_pvOutParam    [OUT]       The output parameter
* @par Returns        
*   VENC_DRV_MRESULT_T
*/
VENC_DRV_MRESULT_T  eVEncDrvQueryCapability(
                        VENC_DRV_QUERY_TYPE_T a_eType, 
                        VAL_VOID_T *a_pvInParam, 
                        VAL_VOID_T *a_pvOutParam
);

/**
* @par Function       
*   eVEncDrvCreate
* @par Description    
*   Create the driver handle
* @param              
*   a_phHandle      [OUT]       The driver handle
* @param              
*   a_eVideoFormat  [IN]        The VENC_DRV_VIDEO_FORMAT_T structure
* @par Returns        
*   VENC_DRV_MRESULT_T
*/
VENC_DRV_MRESULT_T  eVEncDrvCreate(
                        VAL_HANDLE_T *a_phHandle, 
                        VENC_DRV_VIDEO_FORMAT_T a_eVideoFormat
);

/**
* @par Function       
*   eVEncDrvRelease
* @par Description    
*   Release the driver handle
* @param              
*   a_hHandle       [IN]        The driver handle
* @param              
*   a_eVideoFormat  [IN]        The VENC_DRV_VIDEO_FORMAT_T structure
* @par Returns        
*   VENC_DRV_MRESULT_T
*/
VENC_DRV_MRESULT_T  eVEncDrvRelease(
                        VAL_HANDLE_T a_hHandle,
                        VENC_DRV_VIDEO_FORMAT_T a_eVideoFormat
);

/**
* @par Function       
*   eVEncDrvInit
* @par Description    
*   Init the driver setting, alloc working memory ... etc.
* @param              
*   a_hHandle       [IN]        The driver handle
* @par Returns        
*   VENC_DRV_MRESULT_T
*/
VENC_DRV_MRESULT_T  eVEncDrvInit(
                        VAL_HANDLE_T a_hHandle
);

/**
* @par Function       
*   eVEncDrvDeInit
* @par Description    
*   DeInit the driver setting, free working memory ... etc.
* @param              
*   a_hHandle       [IN]        The driver handle
* @par Returns        
*   VENC_DRV_MRESULT_T
*/
VENC_DRV_MRESULT_T  eVEncDrvDeInit(
                        VAL_HANDLE_T a_hHandle
);

/**
* @par Function       
*   eVEncDrvSetParam
* @par Description    
*   Set parameter to driver
* @param              
*   a_hHandle       [IN]        The driver handle
* @param              
*   a_eType         [IN]        The VENC_DRV_SET_TYPE_T structure
* @param              
*   a_pvInParam     [IN]        The input parameter
* @param              
*   a_pvOutParam    [OUT]       The output parameter
* @par Returns        
*   VENC_DRV_MRESULT_T
*/
VENC_DRV_MRESULT_T  eVEncDrvSetParam(
                        VAL_HANDLE_T a_hHandle, 
                        VENC_DRV_SET_TYPE_T a_eType, 
                        VAL_VOID_T *a_pvInParam, 
                        VAL_VOID_T *a_pvOutParam
);

/**
* @par Function       
*   eVEncDrvGetParam
* @par Description    
*   Get parameter from driver
* @param              
*   a_hHandle       [IN]        The driver handle
* @param              
*   a_eType         [IN]        The VENC_DRV_SET_TYPE_T structure
* @param              
*   a_pvInParam     [IN]        The input parameter
* @param              
*   a_pvOutParam    [OUT]       The output parameter
* @par Returns        
*   VENC_DRV_MRESULT_T
*/
VENC_DRV_MRESULT_T  eVEncDrvGetParam(
                        VAL_HANDLE_T a_hHandle, 
                        VENC_DRV_GET_TYPE_T a_eType, 
                        VAL_VOID_T *a_pvInParam, 
                        VAL_VOID_T *a_pvOutParam
);

/**
* @par Function       
*   eVEncDrvEncode
* @par Description    
*   Encode frame
* @param              
*   a_hHandle       [IN]        The driver handle
* @param              
*   a_eOpt          [IN]        The VENC_DRV_START_OPT_T structure
* @param              
*   a_prFrmBuf      [IN]        The input frame buffer with VENC_DRV_PARAM_FRM_BUF_T structure
* @param              
*   a_prBSBuf       [IN]        The input bitstream buffer with VENC_DRV_PARAM_BS_BUF_T structure
* @param              
*   a_prResult      [OUT]       The output result with VENC_DRV_DONE_RESULT_T structure
* @par Returns        
*   VENC_DRV_MRESULT_T
*/
VENC_DRV_MRESULT_T  eVEncDrvEncode(
                        VAL_HANDLE_T a_hHandle, 
                        VENC_DRV_START_OPT_T a_eOpt, 
                        VENC_DRV_PARAM_FRM_BUF_T *a_prFrmBuf, 
                        VENC_DRV_PARAM_BS_BUF_T *a_prBSBuf, 
                        VENC_DRV_DONE_RESULT_T *a_prResult
);

typedef struct __VENC_HYB_ENCSETTING
{

	// used in SetParameter 
	VAL_UINT32_T    u4Width;   
	VAL_UINT32_T    u4Height;  
	VAL_UINT32_T    u4IntraVOPRate;  //u4NumPFrm;   
	VAL_UINT32_T    eFrameRate;  
	VAL_UINT32_T    u4VEncBitrate;
	VAL_UINT32_T    u4QualityLevel;
	VAL_UINT32_T   	u4ShortHeaderMode;
	VAL_UINT32_T   	u4CodecType;	// mepg4, h263, h264...
	VAL_UINT32_T   	u4RotateAngle;

	// used in QueryFunctions
	VENC_DRV_COLOR_FORMAT_T     eVEncFormat;   		// YUV420, I420 .....
	VENC_DRV_YUV_STRIDE_T       rVCodecYUVStride;
	VAL_UINT32_T	u4Profile;
	VAL_UINT32_T	u4Level;   
	VAL_UINT32_T	u4BufWidth; 
	VAL_UINT32_T	u4BufHeight; 
	VAL_UINT32_T	u4NumBFrm;	 
	VAL_UINT32_T	fgInterlace; 

	// used in Query
	VAL_UINT32_T    u4InitQ;
	VAL_UINT32_T    u4MinQ;
	VAL_UINT32_T    u4MaxQ;
	VAL_UINT32_T    u4Algorithm;
	VAL_UINT32_T    u4_Rate_Hard_Limit;
	VAL_UINT32_T    u4RateBalance;
    VAL_UINT32_T    u4ForceIntraEnable;
#ifdef  MTK_USES_VR_DYNAMIC_QUALITY_MECHANISM   
	VAL_UINT32_T    u4VEncMinBitrate; //Min bit-rate   
#endif	

    // hardware dependent function settings
    VAL_BOOL_T      fgUseMCI;
}VENC_HYBRID_ENCSETTING;

typedef struct VENC_BS_s{
		VAL_UINT8_T                     *u4BS_addr;			
		VAL_UINT8_T                     *u4BS_addr_PA;			
		VAL_UINT32_T                    u4BSSize;	 
		VAL_UINT32_T                    u4BS_frmSize;	
		VAL_UINT32_T                    u4BS_frmCount;			
		VAL_UINT32_T                     u4BS_index;			
		VAL_UINT32_T                     u4BS_preindex;			
		VAL_UINT32_T                    u4Fillcnt;		
		VAL_UINT32_T                    Handle;
}VENC_BS_T;


#ifdef __cplusplus
}
#endif

#endif /* __VENC_DRV_IF_H__ */
