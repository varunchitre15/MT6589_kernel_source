
/** 
 * @file 
 *   vdec_drv_if.h 
 *
 * @par Project:
 *   MFlexVideo 
 *
 * @par Description:
 *   Decoder Driver Interface
 *
 * @par Author:
 *   Techien Chen (mtk01790)
 *
 * @par $Revision: #22 $
 * @par $Modtime:$
 * @par $Log:$
 *
 */


#ifndef VDEC_DRV_IF_H
#define VDEC_DRV_IF_H

/*=============================================================================
 *                              Include Files
 *===========================================================================*/

#include "val_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 *                              Definition
 *===========================================================================*/
/**
 * @par Structure
 *  VDEC_DRV_NALU_T
 * @par Description
 *  Buffer Structure
 *  - Store NALU buffer base address
 *  - Store length of NALU buffer
 */

typedef struct
{
    VAL_UINT32_T u4AddrOfNALu;   ///< NALU buffer base address 
    VAL_UINT32_T u4LengthOfNALu; ///< Length of NALU buffer
    void *pReseved;              ///< reserved
}VDEC_DRV_NALU_T;



/**
 * @par Structure
 *  VDEC_DRV_RINGBUF_T
 * @par Description
 *  Ring Buffer Structure
 *  - Store buffer base address
 *  - Store read/write pointer address
 */
typedef struct __VDEC_DRV_RINGBUF_T
{
	VAL_MEM_ADDR_T  rBase;      ///< [IN]Base address of ring buffer
	VAL_UINT32_T    u4Read;     ///< [IN/OUT]Virtual address of read pointer.
	VAL_UINT32_T    u4Write;    ///< [IN]Virtual address of write pointer.
    VAL_UINT32_T u4Timestamp;   ///< store timestamp
} VDEC_DRV_RINGBUF_T;

/**
 * @par Structure
 *  P_VDEC_DRV_RINGBUF_T
 * @par Description
 *  Pointer of VDEC_DRV_RINGBUF_T
 */
typedef VDEC_DRV_RINGBUF_T *P_VDEC_DRV_RINGBUF_T;




/**
 * @par Structure
 *  VDEC_DRV_FRAMEBUF_T
 * @par Description
 *  Frame buffer information
 */
typedef struct __VDEC_DRV_FRAMEBUF_T
{
	VAL_MEM_ADDR_T  rBaseAddr;      ///< [IN/OUT]Base address
	VAL_MEM_ADDR_T  rPostProcAddr;  ///< [IN/OUT]Post process address
	VAL_UINT32_T u4BufWidth;        ///< [IN/OUT]Buffer width
	VAL_UINT32_T u4BufHeight;       ///< [IN/OUT]Buffer height
	VAL_UINT32_T u4DispWidth;       ///< [OUT]Display width
	VAL_UINT32_T u4DispHeight;      ///< [OUT]Display width
	VAL_UINT32_T u4DispPitch;       ///< [OUT]Display pitch
	VAL_UINT32_T u4Timestamp;       ///< [IN/OUT]Timestamp for last decode picture
	VAL_UINT32_T u4AspectRatioW;    ///< [OUT]The horizontal size of the sample aspect ratio.
	VAL_UINT32_T u4AspectRatioH;    ///< [OUT]The vertical size of the sample aspect ratio.
	VAL_UINT32_T u4FrameBufferType; ///< [OUT]One of VDEC_DRV_FBTYPE
	VAL_UINT32_T u4PictureStructure;///< [OUT]One of VDEC_DRV_PIC_STRUCT
	VAL_UINT32_T u4FrameBufferStatus;///< [OUT] One of VDEC_DRV_FBSTSTUS
	VAL_UINT32_T u4Reserved1;       ///< Reserved
	VAL_UINT32_T u4Reserved2;       ///< Reserved
	VAL_UINT32_T u4Reserved3;       ///< Reserved
} VDEC_DRV_FRAMEBUF_T;

/**
 * @par Structure
 *  P_VDEC_DRV_FRAMEBUF_T
 * @par Description
 *  Pointer of VDEC_DRV_FRAMEBUF_T
 */
typedef VDEC_DRV_FRAMEBUF_T *P_VDEC_DRV_FRAMEBUF_T;




/**
 * @par Structure
 *  VDEC_DRV_PICINFO_T
 * @par Description
 *  Picture information
 */
typedef struct __VDEC_DRV_PICINFO_T
{
	VAL_UINT32_T u4Width;             ///< Frame width
	VAL_UINT32_T u4Height;            ///< Frame height
    VAL_UINT32_T u4RealWidth;         ///< Frame real width
	VAL_UINT32_T u4RealHeight;        ///< Frame real height
	VAL_UINT32_T u4Timestamp;         ///< Timestamp for last decode picture
	VAL_UINT32_T u4AspectRatioW;      ///< The horizontal size of the sample aspect ratio
	VAL_UINT32_T u4AspectRatioH;      ///< The vertical size of the sample aspect ratio
	VAL_UINT32_T u4FrameRate;         ///< One of VDEC_DRV_FRAME_RATE
	VAL_UINT32_T u4PictureStructure;  ///< One of VDEC_DRV_PIC_STRUCT
	VAL_UINT32_T u4IsProgressiveOnly; ///< 1: Progressive only. 0: Not progressive only. 
} VDEC_DRV_PICINFO_T;

/**
 * @par Structure
 *  P_VDEC_DRV_PICINFO_T
 * @par Description
 *  Pointer of VDEC_DRV_PICINFO_T
 */
typedef VDEC_DRV_PICINFO_T *P_VDEC_DRV_PICINFO_T;


/**
 * @par Structure
 *  VDEC_DRV_SEQINFO_T
 * @par Description
 *  Sequence information.
 *  - Including Width/Height
 */
typedef struct __VDEC_DRV_SEQINFO_T
{
	VAL_UINT32_T u4Width;      ///< Sequence buffer width
	VAL_UINT32_T u4Height;     ///< Sequence buffer height
	VAL_UINT32_T u4PicWidth;   ///< Sequence display width
	VAL_UINT32_T u4PicHeight;  ///< Sequence display height
      VAL_INT32_T i4AspectRatioWidth;
      VAL_INT32_T i4AspectRatioHeight;
} VDEC_DRV_SEQINFO_T;

/**
 * @par Structure
 *  P_VDEC_DRV_SEQINFO_T
 * @par Description
 *  Pointer of VDEC_DRV_SEQINFO_T
 */
typedef VDEC_DRV_SEQINFO_T *P_VDEC_DRV_SEQINFO_T;

/**
 * @par Structure
 *  VDEC_DRV_STATISTIC_T
 * @par Description
 *  VDecDrv Statistic information
 */
typedef struct __VDEC_DRV_STATISTIC_T
{
    VAL_UINT32_T   u4DecTimeMax;       ///< [Out] Decode one frame period, Max.
    VAL_UINT32_T   u4DecTimeMin;       ///< [Out] Decode one frame period, Min.
    VAL_UINT32_T   u4DecTimeAvg;       ///< [Out] Decode one frame period, Average.
} VDEC_DRV_STATISTIC_T;

/**
 * @par Structure
 *  P_VDEC_DRV_STATISTIC_T
 * @par Description
 *  Pointer of VDEC_DRV_STATISTIC_T
 */
typedef VDEC_DRV_STATISTIC_T *P_VDEC_DRV_STATISTIC_T;

/**
 * @par Structure
 *  VDEC_DRV_FBTYPE_T
 * @par Description
 *  Supported frame buffer type in driver layer
 */
typedef enum
{
	UNKNOWN_FBTYPE = 0,                        ///< Unknown type
	VDEC_DRV_FBTYPE_YUV420_BK_16x1 = (1<<0),   ///< YCbCr 420 block in three planar
	VDEC_DRV_FBTYPE_YUV420_BK_8x2  = (1<<1),   ///< YCbCr 420 block in three planar
	VDEC_DRV_FBTYPE_YUV420_BK_4x4  = (1<<2),   ///< YCbCr 420 block in three planar
	VDEC_DRV_FBTYPE_YUV420_RS      = (1<<3),   ///< YCbCr 420 raster scan in three planar
	VDEC_DRV_FBTYPE_RGB565_RS      = (1<<4)    ///< RGB565 in one planar
} VDEC_DRV_FBTYPE_T;

/**
 * @par Structure
 *  VDEC_DRV_FBSTSTUS
 * @par Description
 *  Supported frame buffer status in driver layer
 */
typedef enum
{
	VDEC_DRV_FBSTSTUS_NORMAL        = 0,            ///< normal type
	VDEC_DRV_FBSTSTUS_REPEAT_LAST   = (1 << 0),     ///< 
	VDEC_DRV_FBSTSTUS_NOT_DISPLAY   = (1 << 1),     ///< YCbCr 420 block in three planar
	VDEC_DRV_FBSTSTUS_NOT_USED      = (1 << 2),     ///< for H.264 multi-slice
} VDEC_DRV_FBSTSTUS;

/**
 * @par Structure
 *  VDEC_DRV_VIDEO_FORMAT_T
 * @par Description
 *  video_format of VDecDrvCreate()
 */
typedef enum
{
	VDEC_DRV_VIDEO_FORMAT_UNKNOWN_VIDEO_FORMAT  = 0,       ///< Unknown video format
	VDEC_DRV_VIDEO_FORMAT_DIVX311               = (1<<0),  ///< Divix 3.11
	VDEC_DRV_VIDEO_FORMAT_DIVX4                 = (1<<1),  ///< Divix 4
	VDEC_DRV_VIDEO_FORMAT_DIVX5                 = (1<<2),  ///< Divix 5
	VDEC_DRV_VIDEO_FORMAT_XVID                  = (1<<3),  ///< Xvid
	VDEC_DRV_VIDEO_FORMAT_MPEG1                 = (1<<4),  ///< MPEG-1
	VDEC_DRV_VIDEO_FORMAT_MPEG2                 = (1<<5),  ///< MPEG-2
	VDEC_DRV_VIDEO_FORMAT_MPEG4                 = (1<<6),  ///< MPEG-4
	VDEC_DRV_VIDEO_FORMAT_H263                  = (1<<7),  ///< H263
	VDEC_DRV_VIDEO_FORMAT_H264                  = (1<<8),  ///< H264
	VDEC_DRV_VIDEO_FORMAT_H265                  = (1<<9),  ///< H265
	VDEC_DRV_VIDEO_FORMAT_WMV7                  = (1<<10), ///< WMV7
	VDEC_DRV_VIDEO_FORMAT_WMV8                  = (1<<11), ///< WMV8
	VDEC_DRV_VIDEO_FORMAT_WMV9                  = (1<<12), ///< WMV9
	VDEC_DRV_VIDEO_FORMAT_VC1                   = (1<<13), ///< VC1
	VDEC_DRV_VIDEO_FORMAT_REALVIDEO8            = (1<<14), ///< RV8
	VDEC_DRV_VIDEO_FORMAT_REALVIDEO9            = (1<<15), ///< RV9
	VDEC_DRV_VIDEO_FORMAT_VP6                   = (1<<16), ///< VP6
	VDEC_DRV_VIDEO_FORMAT_VP7                   = (1<<17), ///< VP7
	VDEC_DRV_VIDEO_FORMAT_VP8                   = (1<<18), ///< VP8
	VDEC_DRV_VIDEO_FORMAT_VP8_WEBP_PICTURE_MODE     = (1<<19), ///< VP8 WEBP PICTURE MODE
	VDEC_DRV_VIDEO_FORMAT_VP8_WEBP_MB_ROW_MODE      = (1<<20), ///< VP8 WEBP ROW MODE
	VDEC_DRV_VIDEO_FORMAT_AVS                       = (1<<21), ///< AVS
	VDEC_DRV_VIDEO_FORMAT_MJPEG                     = (1<<22), ///< Motion JPEG
	VDEC_DRV_VIDEO_FORMAT_S263                      = (1<<23),  ///< Sorenson Spark
	VDEC_DRV_VIDEO_FORMAT_H264HP                    = (1<<24)
} VDEC_DRV_VIDEO_FORMAT_T;

/**
 * @par Structure
 *  VDEC_DRV_H264_VIDEO_PROFILE_T
 * @par Description
 *  H264 Video Profile
 */
typedef enum
{
	VDEC_DRV_H264_VIDEO_PROFILE_UNKNOWN						= 0,      ///< Unknown video profile
	VDEC_DRV_H264_VIDEO_PROFILE_H264_BASELINE				= (1<<0), ///< H264 baseline profile
	VDEC_DRV_H264_VIDEO_PROFILE_H264_CONSTRAINED_BASELINE	= (1<<1), ///< H264 constrained baseline profile
	VDEC_DRV_H264_VIDEO_PROFILE_H264_MAIN					= (1<<2), ///< H264 main profile
	VDEC_DRV_H264_VIDEO_PROFILE_H264_EXTENDED				= (1<<3), ///< H264 extended profile
	VDEC_DRV_H264_VIDEO_PROFILE_H264_HIGH					= (1<<4), ///< H264 high profile
	VDEC_DRV_H264_VIDEO_PROFILE_H264_HIGH_10				= (1<<5), ///< H264 high 10 profile
	VDEC_DRV_H264_VIDEO_PROFILE_H264_HIGH422				= (1<<6), ///< H264 high 422 profile
	VDEC_DRV_H264_VIDEO_PROFILE_H264_HIGH444				= (1<<7), ///< H264 high 444 profile
	VDEC_DRV_H264_VIDEO_PROFILE_H264_HIGH_10_INTRA			= (1<<8), ///< H264 high 10 intra profile in Amendment 2
	VDEC_DRV_H264_VIDEO_PROFILE_H264_HIGH422_INTRA			= (1<<9), ///< H264 high 422 intra profile in Amendment 2
	VDEC_DRV_H264_VIDEO_PROFILE_H264_HIGH444_INTRA			= (1<<10),///< H264 high 444 intra profile in Amendment 2
	VDEC_DRV_H264_VIDEO_PROFILE_H264_CAVLC444_INTRA			= (1<<11),///< H264 CAVLC 444 intra profile in Amendment 2
	VDEC_DRV_H264_VIDEO_PROFILE_H264_HIGH444_PREDICTIVE		= (1<<12),///< H264 high 444 predictive profile in Amendment 2
	VDEC_DRV_H264_VIDEO_PROFILE_H264_SCALABLE_BASELINE		= (1<<13),///< H264 scalable baseline profile in Amendment 3
	VDEC_DRV_H264_VIDEO_PROFILE_H264_SCALABLE_HIGH			= (1<<14),///< H264 scalable high profile in Amendment 3
	VDEC_DRV_H264_VIDEO_PROFILE_H264_SCALABLE_HIGH_INTRA	= (1<<15),///< H264 scalable high intra profile in Amendment 3
	VDEC_DRV_H264_VIDEO_PROFILE_H264_MULTIVIEW_HIGH			= (1<<16) ///< Corrigendum 1 (2009)
}VDEC_DRV_H264_VIDEO_PROFILE_T;

/**
 * @par Structure
 *  VDEC_DRV_MPEG_VIDEO_PROFILE_T
 * @par Description
 *  MPEG Video Profile
 *  - Icluding H263 and MPEG2/4
 */
typedef enum
{
	VDEC_DRV_MPEG_VIDEO_PROFILE_H263_0                    = (1<<0), ///< H263 Profile 0
	VDEC_DRV_MPEG_VIDEO_PROFILE_H263_1                    = (1<<1), ///< H263 Profile 1 
	VDEC_DRV_MPEG_VIDEO_PROFILE_H263_2                    = (1<<2), ///< H263 Profile 2
	VDEC_DRV_MPEG_VIDEO_PROFILE_H263_3                    = (1<<3), ///< H263 Profile 3
	VDEC_DRV_MPEG_VIDEO_PROFILE_H263_4                    = (1<<4), ///< H263 Profile 4 
	VDEC_DRV_MPEG_VIDEO_PROFILE_H263_5                    = (1<<5), ///< H263 Profile 5
	VDEC_DRV_MPEG_VIDEO_PROFILE_H263_6                    = (1<<6), ///< H263 Profile 6
	VDEC_DRV_MPEG_VIDEO_PROFILE_H263_7                    = (1<<7), ///< H263 Profile 7
	VDEC_DRV_MPEG_VIDEO_PROFILE_H263_8                    = (1<<8), ///< H263 Profile 8
	VDEC_DRV_MPEG_VIDEO_PROFILE_MPEG2_SIMPLE              = (1<<9), ///< MPEG2 Simple Profile
	VDEC_DRV_MPEG_VIDEO_PROFILE_MPEG2_MAIN                = (1<<10),///< MPEG2 Main Profile
	VDEC_DRV_MPEG_VIDEO_PROFILE_MPEG2_SNR                 = (1<<11),///< MPEG2 SNR Profile
	VDEC_DRV_MPEG_VIDEO_PROFILE_MPEG2_SPATIAL             = (1<<12),///< MPEG2 Spatial Profile
	VDEC_DRV_MPEG_VIDEO_PROFILE_MPEG2_HIGH                = (1<<13),///< MPEG2 High Profile
	VDEC_DRV_MPEG_VIDEO_PROFILE_MPEG4_SIMPLE              = (1<<14),///< MPEG4 Simple Profile
	VDEC_DRV_MPEG_VIDEO_PROFILE_MPEG4_ADVANCED_SIMPLE     = (1<<15) ///< MPEG4 Advanced Simple Profile
}VDEC_DRV_MPEG_VIDEO_PROFILE_T;

/**
 * @par Structure
 *  VDEC_DRV_MS_VIDEO_PROFILE_T
 * @par Description
 *  Microsoft Video Profile
 */
typedef enum
{
	VDEC_DRV_MS_VIDEO_PROFILE_VC1_SIMPLE                = (1<<0),  ///< VC-1 Simple Profile
	VDEC_DRV_MS_VIDEO_PROFILE_VC1_MAIN                  = (1<<1),  ///< VC-1 Main Profile
	VDEC_DRV_MS_VIDEO_PROFILE_VC1_ADVANCED              = (1<<2),  ///< VC-1 Advanced Profile
	VDEC_DRV_MS_VIDEO_PROFILE_WMV9_SIMPLE               = (1<<3),  ///< WMV9 Simple Profile
	VDEC_DRV_MS_VIDEO_PROFILE_WMV9_MAIN                 = (1<<4),  ///< WMV9 Main Profile
	VDEC_DRV_MS_VIDEO_PROFILE_WMV9_COMPLEX              = (1<<5)   ///< WMV9 Complex Profile
} VDEC_DRV_MS_VIDEO_PROFILE_T;

/**
 * @par Structure
 *  VDEC_DRV_VIDEO_LEVEL_T
 * @par Description
 *  Video Level
 */
typedef enum
{
	VDEC_DRV_VIDEO_LEVEL_UNKNOWN = 0,   ///< Unknown level
	VDEC_DRV_VIDEO_LEVEL_0,				///< Specified by VC1
	VDEC_DRV_VIDEO_LEVEL_1,				///< Specified by H264, VC1, MPEG4
	VDEC_DRV_VIDEO_LEVEL_1b,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_1_1,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_1_2,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_1_3,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_2,				///< Specified by H264, VC1, MPEG4
	VDEC_DRV_VIDEO_LEVEL_2_1,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_2_2,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_3,				///< Specified by H264, VC1, MPEG4
	VDEC_DRV_VIDEO_LEVEL_3_1,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_3_2,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_4,				///< Specified by H264, VC1
	VDEC_DRV_VIDEO_LEVEL_4_1,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_4_2,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_5,				///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_5_1,			///< Specified by H264
	VDEC_DRV_VIDEO_LEVEL_LOW,			///< Specified by MPEG2, VC1
	VDEC_DRV_VIDEO_LEVEL_MEDIUM,		///< Specified by MPEG2, VC1
	VDEC_DRV_VIDEO_LEVEL_HIGH1440,		///< Specified by MPEG2
	VDEC_DRV_VIDEO_LEVEL_HIGH			///< Specified by MPEG2, VC1
} VDEC_DRV_VIDEO_LEVEL_T;

/**
 * @par Structure
 *  VDEC_DRV_RESOLUTION_T
 * @par Description
 *  Video Resolution
 */
typedef enum
{
	VDEC_DRV_RESOLUTION_UNKNOWN = 0,   ///< Unknown resolution
	VDEC_DRV_RESOLUTION_SUPPORT_QCIF,  ///< QCIF
	VDEC_DRV_RESOLUTION_SUPPORT_QVGA,  ///< QVGA
	VDEC_DRV_RESOLUTION_SUPPORT_CIF,   ///< CIF
	VDEC_DRV_RESOLUTION_SUPPORT_480I,  ///< 720x480 interlace
	VDEC_DRV_RESOLUTION_SUPPORT_480P,  ///< 720x480 progressive
	VDEC_DRV_RESOLUTION_SUPPORT_576I,  ///< 720x576 interlace
	VDEC_DRV_RESOLUTION_SUPPORT_576P,  ///< 720x576 progressive
	VDEC_DRV_RESOLUTION_SUPPORT_720P,  ///< 1280x720 progressive
	VDEC_DRV_RESOLUTION_SUPPORT_1080I, ///< 1920x1080 interlace
	VDEC_DRV_RESOLUTION_SUPPORT_1080P  ///< 1920x1080 progressive
} VDEC_DRV_RESOLUTION_T;

/**
 * @par Structure
 *  VDEC_DRV_BUFFER_CONTROL_T
 * @par Description
 *  Type of buffer control
 *  - Here are two types of buffer
 *    - 1.Reference buffer
 *    - 2.Display buffer
 *  - Buffer can be fixed size or derived from memory pool.
 *  - Buffer can be created from internal or external memory.
 */
typedef enum
{
	VDEC_DRV_BUFFER_CONTROL_UNKNOWN						= 0,       ///< Unknown Type
	VDEC_DRV_BUFFER_CONTROL_REF_IS_DISP_EXT             = (1<<0),  ///< Reference frame and Display frame share the same external buffer 
	VDEC_DRV_BUFFER_CONTROL_REF_IS_DISP_INT             = (1<<1),  ///< Reference frame and Display frame share the same internal buffer
	VDEC_DRV_BUFFER_CONTROL_REF_IS_DISP_EXT_POOL        = (1<<2),  ///< Reference frame and Display frame share the same external memory pool
	VDEC_DRV_BUFFER_CONTROL_REF_IS_DISP_INT_POOL        = (1<<3),  ///< Reference frame and Display frame share the same internal memory pool
	VDEC_DRV_BUFFER_CONTROL_REF_EXT_DISP_EXT            = (1<<4),  ///< Reference frame uses external buffer and Display frame use another external buffer
	VDEC_DRV_BUFFER_CONTROL_REF_EXT_DISP_INT            = (1<<5),  ///< Reference frame uses external buffer and Display frame uses internal buffer
	VDEC_DRV_BUFFER_CONTROL_REF_EXT_DISP_EXT_POOL       = (1<<6),  ///< Reference frame uses external buffer and Display frame uses external memory pool
	VDEC_DRV_BUFFER_CONTROL_REF_EXT_DISP_INT_POOL       = (1<<7),  ///< Reference frame uses external buffer and Display frame uses internal memory pool
	VDEC_DRV_BUFFER_CONTROL_REF_EXT_POOL_DISP_EXT       = (1<<8),  ///< Reference frame uses external memory pool and Display frame use external buffer
	VDEC_DRV_BUFFER_CONTROL_REF_EXT_POOL_DISP_INT       = (1<<9),  ///< Reference frame uses external memory pool and Display frame uses internal buffer
	VDEC_DRV_BUFFER_CONTROL_REF_EXT_POOL_DISP_EXT_POOL  = (1<<10), ///< Reference frame uses external memory pool and Display frame uses external memory pool
	VDEC_DRV_BUFFER_CONTROL_REF_EXT_POOL_DISP_INT_POOL  = (1<<11), ///< Reference frame uses external memory pool and Display frame uses internal memory pool
	VDEC_DRV_BUFFER_CONTROL_REF_INT_DISP_EXT            = (1<<12), ///< Reference frame uses internal buffer and Display frame use external buffer
	VDEC_DRV_BUFFER_CONTROL_REF_INT_DISP_INT            = (1<<13), ///< Reference frame uses internal buffer and Display frame uses internal buffer
	VDEC_DRV_BUFFER_CONTROL_REF_INT_DISP_EXT_POOL       = (1<<14), ///< Reference frame uses internal buffer and Display frame uses external memory pool
	VDEC_DRV_BUFFER_CONTROL_REF_INT_DISP_INT_POOL       = (1<<15), ///< Reference frame uses internal buffer and Display frame uses internal memory pool
	VDEC_DRV_BUFFER_CONTROL_REF_INT_POOL_DISP_EXT       = (1<<16), ///< Reference frame uses internal memory pool and Display frame use external buffer
	VDEC_DRV_BUFFER_CONTROL_REF_INT_POOL_DISP_INT       = (1<<17), ///< Reference frame uses internal memory pool and Display frame uses internal buffer
    VDEC_DRV_BUFFER_CONTROL_REF_INT_POOL_DISP_EXT_POOL  = (1<<18), ///< Reference frame uses internal memory pool and Display frame uses external memory pool
    VDEC_DRV_BUFFER_CONTROL_REF_INT_POOL_DISP_INT_POOL  = (1<<19)  ///< Reference frame uses external memory pool and Display frame uses another internal memory pool
} VDEC_DRV_BUFFER_CONTROL_T;

/**
 * @par Structure
 *  VDEC_DRV_DOWNSIZE_RATIO_T
 * @par Description
 *  DownSize Ratio
 *  - The aspect ratio of frame is kept after downsizing.
 */
typedef enum
{
	VDEC_DRV_DOWNSIZE_RATIO_UNKNOWN			      = 0,       ///< Unknown ratio
	VDEC_DRV_DOWNSIZE_RATIO_1_1                   = (1<<0),  ///< Original ratio
	VDEC_DRV_DOWNSIZE_RATIO_1_2                   = (1<<1),  ///< ratio = 1/2
	VDEC_DRV_DOWNSIZE_RATIO_1_3                   = (1<<2),  ///< ratio = 1/3
	VDEC_DRV_DOWNSIZE_RATIO_1_4                   = (1<<3),  ///< ratio = 1/4
	VDEC_DRV_DOWNSIZE_RATIO_1_5                   = (1<<4),  ///< ratio = 1/5
	VDEC_DRV_DOWNSIZE_RATIO_1_6                   = (1<<5),  ///< ratio = 1/6
	VDEC_DRV_DOWNSIZE_RATIO_1_7                   = (1<<6),  ///< ratio = 1/7
	VDEC_DRV_DOWNSIZE_RATIO_1_8                   = (1<<7)   ///< ratio = 1/8
} VDEC_DRV_DOWNSIZE_RATIO_T;

/**
 * @par Structure
 *  VDEC_DRV_PIC_STRUCT_T
 * @par Description
 *  [Unused]Picture Struct
 *  - Consecutive Frame or filed
 *  - Separated  top/bottom field
 */
typedef enum
{
	VDEC_DRV_PIC_STRUCT_UNKNOWN = 0,            ///< Unknown 
	VDEC_DRV_PIC_STRUCT_CONSECUTIVE_FRAME,      ///< Consecutive Frame
	VDEC_DRV_PIC_STRUCT_CONSECUTIVE_TOP_FIELD,  ///< Consecutive top field
	VDEC_DRV_PIC_STRUCT_CONSECUTIVE_BOT_FIELD,  ///< Consecutive bottom field
	VDEC_DRV_PIC_STRUCT_SEPARATED_TOP_FIELD,    ///< Separated  top field
	VDEC_DRV_PIC_STRUCT_SEPARATED_BOT_FIELD     ///< Separated  bottom field
} VDEC_DRV_PIC_STRUCT_T;

/**
 * @par Structure
 *  VDEC_DRV_FRAME_RATE_T
 * @par Description
 *  Frame rate types 
 */
typedef enum
{
	VDEC_DRV_FRAME_RATE_UNKNOWN = 0,  ///< Unknown fps
	VDEC_DRV_FRAME_RATE_23_976,       ///< fps = 24000/1001 (23.976...)
	VDEC_DRV_FRAME_RATE_24,           ///< fps = 24
	VDEC_DRV_FRAME_RATE_25,           ///< fps = 25
	VDEC_DRV_FRAME_RATE_29_97,        ///< fps = 30000/1001 (29.97...)
	VDEC_DRV_FRAME_RATE_30,           ///< fps = 30
	VDEC_DRV_FRAME_RATE_50,           ///< fps = 50
	VDEC_DRV_FRAME_RATE_59_94,        ///< fps = 60000/1001 (59.94...)
	VDEC_DRV_FRAME_RATE_60,           ///< fps = 60
	VDEC_DRV_FRAME_RATE_120,          ///< fps = 120
	VDEC_DRV_FRAME_RATE_1,            ///< fps = 1
	VDEC_DRV_FRAME_RATE_5,            ///< fps = 5
	VDEC_DRV_FRAME_RATE_8,            ///< fps = 8
	VDEC_DRV_FRAME_RATE_10,           ///< fps = 10
	VDEC_DRV_FRAME_RATE_12,           ///< fps = 12
	VDEC_DRV_FRAME_RATE_15,           ///< fps = 15
	VDEC_DRV_FRAME_RATE_16,           ///< fps = 16
	VDEC_DRV_FRAME_RATE_17,           ///< fps = 17
	VDEC_DRV_FRAME_RATE_18,           ///< fps = 18
	VDEC_DRV_FRAME_RATE_20,           ///< fps = 20
	VDEC_DRV_FRAME_RATE_2,            ///< fps = 2
	VDEC_DRV_FRAME_RATE_6,            ///< fps = 6
	VDEC_DRV_FRAME_RATE_48,           ///< fps = 48
	VDEC_DRV_FRAME_RATE_70,           ///< fps = 70
	VDEC_DRV_FRAME_RATE_VARIABLE      ///< fps = VBR
} VDEC_DRV_FRAME_RATE_T;

/**
 * @par Structure
 *  VDEC_DRV_QUERY_TYPE_T
 * @par Description
 *  Input parameters for VDecDrvQueryCapability(), and the data structure (use when handle didn't create yet)
 */
typedef enum {
	VDEC_DRV_QUERY_TYPE_FBTYPE,        ///< Query VDEC_DRV_QUERY_TYPE_FBTYPE
	VDEC_DRV_QUERY_TYPE_VIDEO_FORMAT,  ///< Query VDEC_DRV_QUERY_TYPE_VIDEO_FORMAT
    VDEC_DRV_QUERY_TYPE_PROPERTY,      ///< Query VDEC_DRV_PROPERTY_T
    VDEC_DRV_QUERY_TYPE_CHIP_NAME,     ///< Query VDEC_DRV_QUERY_TYPE_CHIP_NAME
	VDEC_DRV_QUERY_TYPE_BUFFER_CONTROL ///< Query VDEC_DRV_QUERY_TYPE_BUFFER_CONTROL
} VDEC_DRV_QUERY_TYPE_T;



/**
 * @par Structure
 *  VDEC_DRV_VIDEO_FBTYPE_T
 * @par Description
 *  Both input and output of type QUERY_FBTYPE
 */ 
typedef struct __VDEC_DRV_VIDEO_FBTYPE_T
{
	VAL_UINT32_T u4FBType;  ///< VDEC_DRV_FBTYPE
} VDEC_DRV_VIDEO_FBTYPE_T;


/**
 * @par Structure
 *  P_VDEC_DRV_VIDEO_FBTYPE_T
 * @par Description
 *  Pointer of VDEC_DRV_VIDEO_FBTYPE_T
 */
typedef VDEC_DRV_VIDEO_FBTYPE_T *P_VDEC_DRV_VIDEO_FBTYPE_T;




/**
 * @par Structure
 *  VDEC_DRV_QUERY_VIDEO_FORMAT_T
 * @par Description
 *  Both input and output of type QUERY_VIDEO_FORMAT
 */
typedef struct __VDEC_DRV_QUERY_VIDEO_FORMAT_T
{
	VAL_UINT32_T u4VideoFormat;    ///< VDEC_DRV_VIDEO_FORMAT
	VAL_UINT32_T u4Profile;        ///< VDEC_DRV_VIDEO_PROFILE
	VAL_UINT32_T u4Level;          ///< VDEC_DRV_VIDEO_LEVEL
	VAL_UINT32_T u4Resolution;     ///< VDEC_DRV_RESOLUTION
	VAL_UINT32_T u4Width;          ///< Frame Width
	VAL_UINT32_T u4Height;         ///< Frame Height
} VDEC_DRV_QUERY_VIDEO_FORMAT_T;


/**
 * @par Structure
 *  P_VDEC_DRV_QUERY_VIDEO_FORMAT_T
 * @par Description
 *  Pointer of VDEC_DRV_QUERY_VIDEO_FORMAT_T
 */
typedef VDEC_DRV_QUERY_VIDEO_FORMAT_T *P_VDEC_DRV_QUERY_VIDEO_FORMAT_T;


/**
 * @par Structure
 *  VDEC_DRV_QUERY_BUFFER_MODE_T
 * @par Description
 *  Both input and output of type QUERY_BUFFER_CONTROL
 */
typedef struct __VDEC_DRV_QUERY_BUFFER_MODE_T
{
	VAL_UINT32_T u4BufCtrl;        ///< VDEC_DRV_BUFFER_CONTROL
} VDEC_DRV_QUERY_BUFFER_MODE_T;

/**
 * @par Structure
 *  P_VDEC_DRV_QUERY_BUFFER_MODE_T
 * @par Description
 *  Pointer of VDEC_DRV_QUERY_BUFFER_MODE_T
 */
typedef VDEC_DRV_QUERY_BUFFER_MODE_T *P_VDEC_DRV_QUERY_BUFFER_MODE_T;


/**
 * @par Structure
 *  VDEC_DRV_GET_TYPE_T
 * @par Description
 *  VDecDrvGetParam() type in VIDEODEC_T, and the data structure
 */
typedef enum
{
	VDEC_DRV_GET_TYPE_QUERY_REF_POOL_SIZE,      ///< how many buffer size of the reference pool needs in driver
	VDEC_DRV_GET_TYPE_QUERY_DISP_POOL_SIZE,     ///< how many buffer size of the display pool needs in driver
	VDEC_DRV_GET_TYPE_DISP_FRAME_BUFFER,        ///< return a P_VDEC_DRV_FRAMEBUF_T address (especially in display order != decode order)
	VDEC_DRV_GET_TYPE_FREE_FRAME_BUFFER,        ///< return a frame didn't be a reference more (when buffer_mode = REF_IS_DISP_EXT, REF_INT_DISP_EXT or REF_INT_POOL_DISP_EXT)
	VDEC_DRV_GET_TYPE_GET_PICTURE_INFO,         ///< return a pointer address point to P_VDEC_DRV_PICINFO_T
	VDEC_DRV_GET_TYPE_GET_STATISTIC_INFO,       ///< return statistic information.
	VDEC_DRV_GET_TYPE_GET_FRAME_MODE,           ///< return frame mode parameter.
	VDEC_DRV_GET_TYPE_GET_FRAME_CROP_INFO,      ///< return frame crop information.
	VDEC_DRV_GET_TYPE_QUERY_REORDER_ABILITY,    ///< query if driver can re-order the decode order to display order
	VDEC_DRV_GET_TYPE_QUERY_DOWNSIZE_ABILITY,   ///< query if driver can downsize decoded frame
	VDEC_DRV_GET_TYPE_QUERY_RESIZE_ABILITY,     ///< query if driver can resize decoded frame
	VDEC_DRV_GET_TYPE_QUERY_DEBLOCK_ABILITY,    ///< query if driver can do deblocking
	VDEC_DRV_GET_TYPE_QUERY_DEINTERLACE_ABILITY,///< query if driver can do deinterlace
	VDEC_DRV_GET_TYPE_QUERY_DROPFRAME_ABILITY,  ///< query if driver can drop frame
	VDEC_DRV_GET_TYPE_GET_DECODE_STATUS_INFO,   ///< query if driver finish decode one frame but no output (main profile with B frame case.)
	VDEC_DRV_GET_TYPE_GET_PIXEL_FORMAT,         ///< query the driver output pixel format
	VDEC_DRV_GET_TYPE_GET_CPU_LOADING_INFO,     ///< query the cpu loading info from kernel driver
	VDEC_DRV_GET_TYPE_GET_HW_CRC,               ///< query the hw CRC
	VDEC_DRV_GET_TYPE_GET_CODEC_TIDS,            ///< query the thread ids from the codec lib
	VDEC_DRV_GET_TYPE_GET_FRAME_INTERVAL        ///< query frame interval from the codec lib
} VDEC_DRV_GET_TYPE_T;

typedef enum
{
    VDEC_DRV_PIXEL_FORMAT_NONE =   0,
    VDEC_DRV_PIXEL_FORMAT_YUV_420_PLANER,
    VDEC_DRV_PIXEL_FORMAT_YUV_420_PLANER_MTK,
    VDEC_DRV_PIXEL_FORMAT_YUV_YV12
} VDEC_DRV_PIXEL_FORMAT_T;

typedef struct __VDEC_DRV_YUV_STRIDE_T
{
    unsigned int u4YStride;
    unsigned int u4UVStride;
} VDEC_DRV_YUV_STRIDE_T;

/**
 * @par Structure
 *  VDEC_DRV_QUERY_POOL_SIZE_T
 * @par Description
 *   output of type QUERY_REF_POOL_SIZE and QUERY_DISP_POOL_SIZE (input is NULL)
 */
typedef struct __VDEC_DRV_QUERY_POOL_SIZE_T
{
	VAL_UINT32_T u4Size;  ///< buffer size of the memory pool
} VDEC_DRV_QUERY_POOL_SIZE_T;

/**
 * @par Structure
 *  P_VDEC_DRV_QUERY_POOL_SIZE_T
 * @par Description
 *  Pointer of VDEC_DRV_QUERY_POOL_SIZE_T
 */
typedef VDEC_DRV_QUERY_POOL_SIZE_T *P_VDEC_DRV_QUERY_POOL_SIZE_T;


// output of type DISP_FRAME_BUFFER and FREE_FRAME_BUFFER is P_VDEC_DRV_FRAMEBUF_T (input is NULL)
// output of type GET_PICTURE_INFO is P_VDEC_DRV_PICINFO_T (input is NULL)
// both input and output of type QUERY_REORDER_ABILITY are NULL (use return value)



/**
 * @par Structure
 *  VDEC_DRV_QUERY_POOL_DOWNSIZE_T
 * @par Description
 *  output of type QUERY_DOWNSIZE_ABILITY (input is NULL)
 */
typedef struct __VDEC_DRV_QUERY_POOL_DOWNSIZE_T
{
	VAL_UINT32_T u4Ratio;           ///< VDEC_DRV_DOWNSIZE_RATIO 
} VDEC_DRV_QUERY_POOL_DOWNSIZE_T;

/**
 * @par Structure
 *  P_VDEC_DRV_QUERY_POOL_DOWNSIZE_T
 * @par Description
 *  Pointer of VDEC_DRV_QUERY_POOL_DOWNSIZE_T
 */
typedef VDEC_DRV_QUERY_POOL_DOWNSIZE_T *P_VDEC_DRV_QUERY_POOL_DOWNSIZE_T;



/**
 * @par Structure
 *  VDEC_DRV_QUERY_POOL_RESIZE_T
 * @par Description
 *  input of type QUERY_RESIZE_ABILITY (output is NULL, use return value)
 */
typedef struct __VDEC_DRV_QUERY_POOL_RESIZE_T
{
	VAL_UINT32_T u4OutWidth;    ///<Width of buffer
	VAL_UINT32_T u4OutHeight;   ///<Height of buffer 
} VDEC_DRV_QUERY_POOL_RESIZE_T;

/**
 * @par Structure
 *  P_VDEC_DRV_QUERY_POOL_RESIZE_T
 * @par Description
 *  Pointer of VDEC_DRV_QUERY_POOL_RESIZE_T
 */
typedef VDEC_DRV_QUERY_POOL_RESIZE_T *P_VDEC_DRV_QUERY_POOL_RESIZE_T;


// both input and output of type QUERY_DEBLOCK_ABILITY are NULL (use return value)
// both input and output of type QUERY_DERING_ABILITY are NULL (use return value)
// both input and output of type QUERY_DEINTERLACE_ABILITY are NULL (use return value)
// both input and output of type QUERY_DROPFRAME_ABILITY are NULL (use return value)

/**
 * @par Structure
 *  VDEC_DRV_SET_TYPE_T
 * @par Description
 *  VDecDrvSetParam() type in VIDEODEC_T, and the data structure
 */
typedef enum
{
	VDEC_DRV_SET_TYPE_USE_EXT_TIMESTAMP,        ///< =1, use timestamp in sVDEC_DRV_FRAMEBUF_T for the picture
	VDEC_DRV_SET_TYPE_SET_BUFFER_MODE,          ///< value is one of VDEC_DRV_BUFFER_MODE
	VDEC_DRV_SET_TYPE_SET_FRAME_BUFFER_TYPE,    ///< one of VDEC_DRV_FBTYPE, if output type is the same as decode type, buffer mode can be REF_IS_DISP
	VDEC_DRV_SET_TYPE_FREE_FRAME_BFFER,         ///< release buffer if DISP BUFFER is allocated from driver
	VDEC_DRV_SET_TYPE_SET_REF_EXT_POOL_ADDR,    ///< if use REF_EXT_POOL in SET_BUFFER_MODE
	VDEC_DRV_SET_TYPE_SET_DISP_EXT_POOL_ADDR,   ///< if use DISP_EXT_POOL in SET_BUFFER_MODE
	VDEC_DRV_SET_TYPE_SET_DECODE_MODE,          ///< set if drop frame
	VDEC_DRV_SET_TYPE_SET_POST_PROC,            ///< buffer mode cannot set to REF_IS_DISP when using post-processing
    VDEC_DRV_SET_TYPE_SET_STATISTIC_ON,         ///< enable statistic function.
    VDEC_DRV_SET_TYPE_SET_STATISTIC_OFF,        ///< disable statistic function.
    VDEC_DRV_SET_TYPE_SET_FRAME_MODE,           ///< set frame mode
    VDEC_DRV_SET_TYPE_SET_BUF_STATUS_FOR_SPEEDY,///< set buffer status for speedy mode
    VDEC_DRV_SET_TYPE_SET_LAST_DISPLAY_TIME,    ///< set the last display time
    VDEC_DRV_SET_TYPE_SET_CURRENT_PLAY_TIME,     ///< set the current play time
    VDEC_DRV_SET_TYPE_SET_CONCEAL_LEVEL,        ///< error conceal level for decoder
    VDEC_DRV_SET_TYPE_SET_OMX_TIDS,
    VDEC_DRV_SET_TYPE_SET_SWITCH_TVOUT,
    VDEC_DRV_SET_TYPE_SET_CODEC_COLOR_FORAMT,
    VDEC_DRV_SET_TYPE_SET_CODEC_YUV_STRIDE,
    VDEC_DRV_SET_TYPE_SET_FRAMESIZE,            ///< set frame size from caller for MPEG4 decoder
} VDEC_DRV_SET_TYPE_T;

// both input and output of type USE_EXT_TIMESTAMP are NULL



/**
 * @par Structure
 *  VDEC_DRV_SET_BUFFER_MODE_T
 * @par Description
 *  input of type VDEC_DRV_SET_BUFFER_MODE_T (output is NULL, use return value)
 */
typedef struct __VDEC_DRV_SET_BUFFER_MODE_T
{
	VAL_UINT32_T u4BufferMode;       ///< VDEC_DRV_BUFFER_CONTROL
} VDEC_DRV_SET_BUFFER_MODE_T;

// input of type SET_FRAME_BUFFER_TYPE is VDEC_DRV_VIDEO_FBTYPE_T (output is NULL, use return value)

/**
 * @par Structure
 *  P_VDEC_DRV_SET_BUFFER_MODE_T
 * @par Description
 *  Pointer of VDEC_DRV_SET_BUFFER_MODE_T
 */
typedef VDEC_DRV_SET_BUFFER_MODE_T *P_VDEC_DRV_SET_BUFFER_MODE_T;

// input of type SET_FRAME_BUFFER_TYPE is VDEC_DRV_VIDEO_FBTYPE_T (output is NULL, use return value)




/**
 * @par Structure
 *  VDEC_DRV_SET_BUFFER_ADDR_T
 * @par Description
 *  input of type FREE_FRAME_BFFER (buffer_len=NULL, output is NULL, use return value)
 */
typedef struct __VDEC_DRV_SET_BUFFER_ADDR_T
{
	VAL_MEM_ADDR_T rBufferAddr;       ///< buffer memory base address	
} VDEC_DRV_SET_BUFFER_ADDR_T;

/**
 * @par Structure
 *  P_VDEC_DRV_SET_BUFFER_ADDR_T
 * @par Description
 *  Pointer of VDEC_DRV_SET_BUFFER_ADDR_T
 */
typedef VDEC_DRV_SET_BUFFER_ADDR_T *P_VDEC_DRV_SET_BUFFER_ADDR_T;

// input of type SET_REF_EXT_POOL_ADDR and SET_DISP_EXT_POOL_ADDR is VDEC_DRV_SET_BUFFER_ADDR_T (output is NULL, use return value)


/**
 * @par Structure
 *  VDEC_DRV_DECODE_MODE_T
 * @par Description
 *  input of type SET_DECODE_MODE (output is NULL, use return value)
 */
typedef enum
{
	VDEC_DRV_DECODE_MODE_UNKNOWN = 0,       ///< Unknown
	VDEC_DRV_DECODE_MODE_NORMAL,            ///< decode all frames (no drop)
	VDEC_DRV_DECODE_MODE_I_ONLY,            ///< skip P and B frame 
	VDEC_DRV_DECODE_MODE_B_SKIP,            ///< skip B frame 
	VDEC_DRV_DECODE_MODE_DROPFRAME,         ///< display param1 frames & drop param2 frames
	VDEC_DRV_DECODE_MODE_NO_REORDER,        ///< output display ASAP without reroder
	VDEC_DRV_DECODE_MODE_THUMBNAIL          ///< thumbnail mode
} VDEC_DRV_DECODE_MODE_T;



/**
 * @par Structure
 *  VDEC_DRV_SET_DECODE_MODE_T
 * @par Description
 *  [Unused]Set Decode Mode
 */
typedef struct __VDEC_DRV_SET_DECODE_MODE_T
{
	VDEC_DRV_DECODE_MODE_T  eDecodeMode;       ///< one of VDEC_DRV_DECODE_MODE
	VAL_UINT32_T            u4DisplayFrameNum; ///< 0  8  7  6  5  4  3  2  1  1  1  1  1  1  1  1 
	VAL_UINT32_T            u4DropFrameNum;    ///< 0  1  1  1  1  1  1  1  1  2  3  4  5  6  7  8
} VDEC_DRV_SET_DECODE_MODE_T;

/**
 * @par Structure
 *  P_VDEC_DRV_SET_DECODE_MODE_T
 * @par Description
 *  Pointer of VDEC_DRV_SET_DECODE_MODE_T
 */
typedef VDEC_DRV_SET_DECODE_MODE_T *P_VDEC_DRV_SET_DECODE_MODE_T;


/**
 * @par Structure
 *  VDEC_DRV_POST_PROC_T
 * @par Description
 *  input of type SET_POST_PROC (output is NULL, use return value)
 */
typedef enum
{
	VDEC_DRV_POST_PROC_UNKNOWN = 0,  ///< Unknown
	VDEC_DRV_POST_PROC_DISABLE,      ///< Do not do post-processing
	VDEC_DRV_POST_PROC_DOWNSIZE,     ///< Do downsize
	VDEC_DRV_POST_PROC_RESIZE,       ///< Do resize
	VDEC_DRV_POST_PROC_DEBLOCK,      ///< Do deblocking
	VDEC_DRV_POST_PROC_DEINTERLACE   ///< Do deinterlace
} VDEC_DRV_POST_PROC_T;



/**
 * @par Structure
 *  VDEC_DRV_SET_POST_PROC_MODE_T
 * @par Description
 *  Parameters of set post process mode
 */
typedef struct __VDEC_DRV_SET_POST_PROC_MODE_T
{
	VAL_UINT32_T u4PostProcMode;     ///< one of VDEC_DRV_POST_PROC
	VAL_UINT32_T u4DownsizeRatio;    ///< if mode is POST_PROC_DOWNSIZE
	VAL_UINT32_T u4ResizeWidth;      ///< if mode is POST_PROC_RESIZE
	VAL_UINT32_T u4ResizeHeight;     ///< if mode is POST_PROC_RESIZE
} VDEC_DRV_SET_POST_PROC_MODE_T;

/**
 * @par Structure
 *  P_VDEC_DRV_SET_POST_PROC_MODE_T
 * @par Description
 *  Pointer of VDEC_DRV_SET_POST_PROC_MODE_T
 */
typedef VDEC_DRV_SET_POST_PROC_MODE_T *P_VDEC_DRV_SET_POST_PROC_MODE_T;




/**
 * @par Structure
 *  VDEC_DRV_PROPERTY_T
 * @par Description
 *  VDecDrv property information
 */
typedef struct __VDEC_DRV_PROPERTY_T
{
    VAL_UINT32_T              u4BufAlign;             ///< [Out] buffer alignment requirement.
    VAL_UINT32_T              u4BufUnitSize;          ///< [Out] buffer unit size is N bytes . (e.g., 8, 16, or 64 bytes per unit.)
    VAL_BOOL_T                fgPostprocessSupport;   ///< [Out] support post-process.
    struct 
    {
        VAL_UINT32_T  fgOverlay   :1; ///< Overlay flag
        VAL_UINT32_T  fgRotate    :1; ///< Rotate flag
        VAL_UINT32_T  fgResize    :1; ///< Resize flag
        VAL_UINT32_T  fgCrop      :1; ///< Croping flag
    } PostProcCapability; ///< Post process property
} VDEC_DRV_PROPERTY_T;

/**
 * @par Structure
 *  P_VDEC_DRV_PROPERTY_T
 * @par Description
 *  Pointer of VDEC_DRV_PROPERTY_T
 */
typedef VDEC_DRV_PROPERTY_T *P_VDEC_DRV_PROPERTY_T;


/**
 * @par Structure
 *  VDEC_DRV_MRESULT_T
 * @par Description
 *  Driver return type
 */
typedef enum __VDEC_DRV_MRESULT_T
{
    VDEC_DRV_MRESULT_OK = 0,           ///< OK
    VDEC_DRV_MRESULT_FAIL,             ///< Fail
    VDEC_DRV_MRESULT_FATAL,            ///< Fatal error to stop.
    VDEC_DRV_MRESULT_RESOLUTION_CHANGED,    ///< Represent resoluion changed
    VDEC_DRV_MRESULT_MAX = 0x0FFFFFFF
} VDEC_DRV_MRESULT_T;

/*=============================================================================
 *                             Function Declaration
 *===========================================================================*/

// video decoder driver API
// ----------------------------------------------------------------------------

/**
 * @par Function:
 *  eVDecDrvQueryCapability
 * @par Description:
 *    - Query Decode Driver Capability
 *    - Input argument will be compare with driver's capability to check if the query is successful or not. 
 *
 * @param a_eType                [IN]       Driver query type, such as FBType, Video Format, etc.
 * @param a_pvInParam            [IN]       Input parameter for each type of query. 
 * @param a_pvOutParam           [OUT]      Store query result, such as FBType, Video Format, etc. 
 * @par Returns:
 *    - VDEC_DRV_MRESULT_OK:   Query Success
 *    - VDEC_DRV_MRESULT_FAIL: Query Fail
 */
VDEC_DRV_MRESULT_T	eVDecDrvQueryCapability(VDEC_DRV_QUERY_TYPE_T a_eType, VAL_VOID_T *a_pvInParam, VAL_VOID_T *a_pvOutParam);

/**
 * @par Function:
 *  eVDecDrvCreate
 * @par Description:
 *    - Create handle
 *    - Allocate extra data for each driver
 *      - According to the input parameter, "a_eVideoFormat."
 *
 * @param a_phHandle            [IN/OUT]   Driver handle
 * @param a_eVideoFormat        [IN]       Video format, such as MPEG4, H264, etc. 
 * @par Returns:
 *  Reason for return value. Show the default returned value at which condition. 
 *    - VDEC_DRV_MRESULT_OK:   Create handle successfully
 *    - VDEC_DRV_MRESULT_FAIL: Failed to create handle
 */
VDEC_DRV_MRESULT_T	eVDecDrvCreate(VAL_HANDLE_T *a_phHandle, VDEC_DRV_VIDEO_FORMAT_T a_eVideoFormat);

/**
 * @par Function:
 *  eVDecDrvRelease
 * @par Description:
 *    - Release Decode Driver
 *      - Need to perform driver deinit before driver release.
 *    - Procedure of release
 *      - Release extra data
 *      - Release handle
 *
 * @param a_hHandle            [IN]   Handle needed to be released.
 * @par Returns:
 *    - VDEC_DRV_MRESULT_OK:   Release handle successfully.
 *    - VDEC_DRV_MRESULT_FAIL: Failed to release handle.
 */
VDEC_DRV_MRESULT_T	eVDecDrvRelease(VAL_HANDLE_T a_hHandle);

/**
 * @par Function:
 *  eVDecDrvInit
 * @par Description:
 *    - Initialize Decode Driver
 *    - Get width and height of bitstream
 *
 * @param a_hHandle            [IN]        Driver handle
 * @param a_prBitstream        [IN]        Input bitstream for driver initialization 
 * @param a_prSeqinfo          [OUT]       Return width and height of bitstream
 * @par Returns:
 *    - VDEC_DRV_MRESULT_OK:   Init driver successfully.
 *    - VDEC_DRV_MRESULT_FAIL: Failed to init driver.
 */
VDEC_DRV_MRESULT_T	eVDecDrvInit(VAL_HANDLE_T a_hHandle, VDEC_DRV_RINGBUF_T *a_prBitstream, VDEC_DRV_SEQINFO_T *a_prSeqinfo);

/**
 * @par Function:
 *  eVDecDrvDeInit
 * @par Description:
 *    - Deinitialize driver
 *      - Have to deinit driver before release driver 
 *
 * @param a_hHandle            [IN]        Driver handle
 * @par Returns:
 *    - VDEC_DRV_MRESULT_OK:   Deinit driver successfully.
 *    - VDEC_DRV_MRESULT_FAIL: Failed to deinit driver.
 */
VDEC_DRV_MRESULT_T	eVDecDrvDeInit(VAL_HANDLE_T a_hHandle);

/**
 * @par Function:
 *  eVDecDrvGetParam
 * @par Description:
 *    - Get driver's parameter
 *      - Type of parameter can be referred to VDEC_DRV_GET_TYPE_T.
 *
 * @param a_hHandle            [IN]        Driver handle
 * @param a_eType              [IN]        Parameter type
 * @param a_pvInParam          [OUT]       Input argument for query parameter.
 * @param a_pvOutParam         [OUT]       Store output parameter
 * @par Returns:
 *    - VDEC_DRV_MRESULT_OK:   Get parameter successfully.
 *    - VDEC_DRV_MRESULT_FAIL: Failed to get parameter.
 *      - Fail reason might be 
 *        - wrong or unsupported parameter type
 *        - fail to get reference memory pool size.
 */
VDEC_DRV_MRESULT_T	eVDecDrvGetParam(VAL_HANDLE_T a_hHandle, VDEC_DRV_GET_TYPE_T a_eType, VAL_VOID_T *a_pvInParam, VAL_VOID_T *a_pvOutParam);

/**
 * @par Function:
 *  eVDecDrvSetParam
 * @par Description:
 *    - Set driver's parameters
 *
 * @param a_hHandle            [IN]        driver handle
 * @param a_eType              [IN]       parameter type
 * @param a_pvInParam          [IN]       input parameter
 * @param a_pvOutParam         [OUT]       output parameter
 * @par Returns:
 *    - VDEC_DRV_MRESULT_OK:   Get parameter successfully.
 *    - VDEC_DRV_MRESULT_FAIL: Failed to get parameter.
 *      - Fail reason might be 
 *        - wrong or unsupported parameter type
 *        - fail to set parameter
 */
VDEC_DRV_MRESULT_T	eVDecDrvSetParam(VAL_HANDLE_T a_hHandle, VDEC_DRV_SET_TYPE_T a_eType, VAL_VOID_T *a_pvInParam, VAL_VOID_T *a_pvOutParam);

/**
 * @par Function:
 *  eVDecDrvDecode
 * @par Description:
 *    - Trigger Decode
 *      - Need to Provide frame buffer to store unused buffer
 *    - The procedure of decode including:
 *      - Header parsing 
 *      - trigger hw decode
 *    - While we want to decode the last frame, we need to set input bitstream as VAL_NULL and still give free frame buffer.
 *
 * @param a_hHandle            [IN]           driver handle
 * @param a_prBitstream        [IN]           input bitstream
 * @param a_prFramebuf         [IN]           free frame buffer
 * @par Returns:
 *    - VDEC_DRV_MRESULT_OK:   Decode successfully.
 *    - VDEC_DRV_MRESULT_FAIL: Failed to decode.
 */
VDEC_DRV_MRESULT_T	eVDecDrvDecode(VAL_HANDLE_T a_hHandle, VDEC_DRV_RINGBUF_T *a_prBitstream, VDEC_DRV_FRAMEBUF_T *a_prFramebuf);


#ifdef __cplusplus
}
#endif

#endif /* VDEC_DRV_IF_H */
