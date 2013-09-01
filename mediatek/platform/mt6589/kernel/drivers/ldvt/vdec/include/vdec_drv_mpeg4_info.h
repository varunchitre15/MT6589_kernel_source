#ifndef _VDEC_DRV_MPEG4_INFO_H_
#define _VDEC_DRV_MPEG4_INFO_H_

#include "drv_common.h"

#include "vdec_common_if.h"
#include "vdec_usage.h"

#include "vdec_info_mpeg.h"
#include "vdec_info_common.h"

#define DISP_DUMMY_FRM
#define VDEC_DEC_DUMMY_FRM
#define VDEC_USE_MW_DXINFO

#define VDEC_SUPPORT_HIGH_FRAME_RATE

#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8550)  
#define VDEC_MPEG4_SUPPORT_RESYNC_MARKER
#endif

/******************************************************************************
* brief MPEG4 log monitor level
******************************************************************************/
#define VDEC_MPEG4_SYSTEM_MONITOR     1
#define VDEC_MPEG4_ERROR_MONITOR     2
#define VDEC_MPEG4_PTS_MONITOR         4
#define VDEC_MPEG4_SEQ_HEADER_PARSER_MONITOR   6
#define VDEC_MPEG4_VOP_HEADER_PARSER_MONITOR   7
#define VDEC_MPEG4_ALL_MONITOR   9



/******************************************************************************
* Local definition
******************************************************************************/
/*! \name MPEG4 Video Start Code
* @{
*/
/// Video Start Code
#define VIDEO_START_CODE                      0x00000100
/// Video Object Start Code Min
#define VIDEO_OBJECT_START_CODE_MIN  0x100
/// Video Object Start Code Max
#define VIDEO_OBJECT_START_CODE_MAX  0x11F
/// Video Object Layer Start Code Min
#define VIDEO_OBJECT_LAYER_START_CODE_MIN           0x120
/// Video Object Start Code Max
#define VIDEO_OBJECT_LAYER_START_CODE_MAX           0x12F
/// Visual Object Sequence Start Code
#define VISUAL_OBJECT_SEQUENCE_START_CODE               0x1B0
/// Visual Object Sequence End Code
#define VISUAL_OBJECT_SEQUENCE_END_CODE                 0x1B1
/// User Data Start Code
#define USER_DATA_START_CODE         0x1B2
/// Group of Vop Start Code
#define GROUP_OF_VOP_START_CODE              0x1B3
/// Video Session Error Code
#define VIDEO_SESSION_ERROR_CODE     0x1B4
/// Visual Object Start Code
#define VISUAL_OBJECT_START_CODE     0x1B5
#define VOP_START_CODE               0x1B6
//Reserved 0x1B7, 0x1B8, 0x1B9
/// FBA Object Start Code
#define FBA_OBJ_START_CODE           0X1BA
/// FBA Object Plane Start Code
#define FBA_OBJ_PLANE_START_CODE     0x1BB
/// Mesh Object Start Code
#define MESH_OBJ_START_CODE          0x1BC
/// Mesh Object Plane Start Code
#define MESH_OBJ_PLANE_START_CODE    0x1BD
/// Still Texture Object Start Code
#define STILL_TEXT_OBJ_START_CODE    0x1BE
/// Texture Spatial Layer Start Code
#define TEXT_SPT_LAYER_START_CODE    0x1BF
/// Texture SNR Layer Start Code
#define TEXT_SNR_LAYER_START_CODE    0x1C0
/// Texture Tile Start Code
#define TEXT_TIL_START_CODE    0x1C1
/// Texture Shape Layer Start Code
#define TEXT_SHP_LAYER_START_CODE    0x1C2
/// Stuffing Start Code
#define STUFFING_START_CODE          0x1C3
//Reserved 0x1C4, 0x1C5
/// System Start Code Min
#define SYSTEM_START_CODE_MIN        0x1C6
/// System Start Code Max
#define SYSTEM_START_CODE_MAX        0x1CF
/// Short Video Start Code Mask
#define SHORT_VIDEO_START_MASK            0xfffffc00
/// Short Video Start Code Marker
#define SHORT_VIDEO_START_MARKER          0x00008000

#define SORENSON_H263_START_MASK         0xffff8000

#define SORENSON_H263_START_MARKER     0x00008000
/*! @} */

/// Video Object Type Definition
//Reserved                           0x0
#define VIDEO_ID                 0x1
#define STILL_TEXT_ID          0x2
#define MESH_ID                  0x3
#define FBA_ID                     0x4
#define MESH_3D_ID             0x5
#define EXTENDED_PAR          0xf
/// Video Shape  Definition: Regular, Binary, Binary Only, GrayScale
#define RECTANGULAR_SHAPE    0x0
#define BINARY_SHAPE              0x1
#define BINARY_ONLY_SHAPE    0x2
#define GRAYSCALE_SHAPE        0x3
/// Sprite Mode: Not Used, Static, GMC
#define SPRITE_NOTE_USED        0x0
#define SPRITE_STATIC               0x1
#define SPRITE_GMC                   0x2
#define SPRITE_RESERVED          0x3
/// Default Video Horizontal Size
#define DEFAULT_H_SIZE      720
/// Default Video Vertical Size
#define DEFAULT_V_SIZE      480
/// MPEG4 Decoder Picture Buffer Number
#define MPEG4_DPB_NUM 3
/// Sprite Warping Error Code
#define WARPING_PT_ERR      0x00000402

#define fgIsMPEG4VideoStartCode(arg)  ((arg & 0xFFFFFF00) == VIDEO_START_CODE)
#define fgPureIsoM4v()         (FALSE)
#define u4Div2Slash(v1, v2) (((v1)+(v2)/2)/(v2))

#ifndef VDEC_DEC_DUMMY_FRM

#define fgIsMPEG4RefPic(arg)   ( ((arg&0xFF) == I_VOP) || \
                                                ((arg&0xFF) == SH_I_VOP) || \
                             	                ((arg&0xFF) == DX3_I_FRM) || \
                                                ((arg&0xFF) == P_TYPE) || \
                                                ((arg&0xFF) == P_VOP) || \
                                                ((arg&0xFF) == S_VOP) ||\
                                                ((arg&0xFF) == SH_I_VOP)||\
                                                ((arg&0xFF) == SH_P_VOP) ||\
                                                ((arg&0xFF) == DUMMY_FRM) ||\
                                                ((arg&0xFF) == DX3_P_FRM)) 
                                                
#else
#define fgIsMPEG4RefPic(arg)   ( ((arg&0xFF) == I_VOP) || \
                                                ((arg&0xFF) == SH_I_VOP) || \
                             	                ((arg&0xFF) == DX3_I_FRM) || \
                                                ((arg&0xFF) == P_TYPE) || \
                                                ((arg&0xFF) == P_VOP) || \
                                                ((arg&0xFF) == S_VOP) ||\
                                                ((arg&0xFF) == SH_I_VOP)||\
                                                ((arg&0xFF) == SH_P_VOP) ||\
                                                ((arg&0xFF) == DX3_P_FRM)) 

#endif                                                  
/*! \name MPEG4 Working Buffer Size
* @{
* \ingroup Private
*/
#if 0
#ifdef WRITE_FULL_DCAC_DATA
#define DCAC_SZ           ((((1920 / 16) * 4) * ((1088 / 16) * 4)) * 4) // (MBx * 4) * (MBy * 4) = 25920
#else
#define DCAC_SZ              ((((1920 / 16) * 4) * (4)) * 4) // (MBx * 4) * (4) = 7680
#endif
#define MVEC_SZ           (((1920 / 16) * 4 * (1088 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB1_SZ           (((1920 / 16) * 4 * (1088 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB2_SZ           (((1920 / 16) * 4 * (1088 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define BCODE_SZ          (((1920 / 16) * 2) * 4) // (MBx * 2) = 90

#else

#ifdef WRITE_FULL_DCAC_DATA
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)  
#define DCAC_SZ_W          ((1920 / 16) * 4)
#define DCAC_SZ_H           ((1088 / 16) * 4 * 4) // (MBx * 4) * (MBy * 4) = 25920
#else//>=MT8580
#define DCAC_SZ_W          ((4096 / 16) * 4)
#define DCAC_SZ_H           ((2048 / 16) * 4 * 4) // (MBx * 4) * (MBy * 4) = 25920
#endif
#else
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
#define DCAC_SZ_W          ((1920 / 16) * 4) // (MBx * 4) * (4) = 7680
#define DCAC_SZ_H           (4 * 4)
#else//>=MT8580
#define DCAC_SZ_W          ((4096 / 16) * 4) // (MBx * 4) * (4) = 7680
#define DCAC_SZ_H           (4 * 4)
#endif
#endif

#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8580)
#define MVEC_SZ_W          ((1920 / 16) *4) // (MBx * 4) * (MBy) = 6480
#define MVEC_SZ_H           ((1088 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB1_SZ_W          ((1920 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB1_SZ_H           ((1088 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB2_SZ_W          ((1920 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB2_SZ_H           ((1088 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BCODE_SZ_W        ((1920 / 16) * 2) // (MBx * 2) = 90
#define BCODE_SZ_H         (4) // (MBx * 2) = 90
#else
#define MVEC_SZ_W          ((4096 / 16) *4) // (MBx * 4) * (MBy) = 6480
#define MVEC_SZ_H           ((2048 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB1_SZ_W          ((4096 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB1_SZ_H           ((2048 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB2_SZ_W          ((4096 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BMB2_SZ_H           ((2048 / 16) * 4) // (MBx * 4) * (MBy) = 6480
#define BCODE_SZ_W        ((4096 / 16) * 8) // (MBx * 8) = 90,is difference with <MT8580 IC
#define BCODE_SZ_H         (4) // (MBx * 2) = 90

#endif
#endif
/*! @} */

/// Quantization Matrix Mode                                                
typedef enum
{
  MPEG4_QMATRIX_INTER = 00,        ///< Inter Quant Matrix
  MPEG4_QMATRIX_INTRA = 1,         ///< Intra Quant Matrix
  MPEG4_QMATRIX_GRAY = 2,          ///< Gray Scale Quant Matrix
}MPEG4_QMATRIX_TYPE_T;
/// Sprite Transmit Mode
typedef enum
{
  SPRITE_TRANS_STOP = 0,             ///< Sprite Transmittion Stop
  SPRITE_TRANS_PEICE = 1,            ///< Sprite Transmittion Piece
  SPRITE_TRANS_UPDATE = 2,         ///< Sprite Transmittion Update  
   SPRITE_TRANS_PAUSE = 3,          ///< Sprite Transmittion Pause
}MPEG4_SPRITE_TRANS_MODE_T;

/*! \name MPEG4 Header Parsing Struct
* @{
* \ingroup Header
*/
/// Visual Object Sequence Header Field
typedef struct _MPEG4_VOS_HDR_FIELD_T
{
	/* 8 Bit */
	UINT32								:	24;  ///< Reserved
	UINT32			u4ProfileLevel		:	8;	///< Profile And Level
	
} MPEG4_VOS_HDR_FIELD_T;

/// Visual Object Sequence Header Union
typedef union _MPEG4_VOS_HDR_UNION_T
{
	UINT32						au4Reg[1];
	MPEG4_VOS_HDR_FIELD_T		rField;
} MPEG4_VOS_HDR_UNION_T;

/// Visual Object Header Field
typedef struct _MPEG4_VISUAL_OBJECT_HDR_FIELD_T
{
	/* 12/5 Bit */
	UINT32						:	20;                ///< Reserved
	UINT32			u4VoType	:	 4;                ///< Visaul Object Type
	UINT32			u4VoPrior	:	 3;                ///< Visual Object Priority
	UINT32			u4VoVerid	:	 4;                ///< Visual Object Version Identify
	UINT32			fgVoId          :	 1;                ///< Visual Object Identifier
} MPEG4_VISUAL_OBJECT_HDR_FIELD_T;
/// Visual Object Header Union
typedef union _MPEG4_VISUAL_OBJECT_HDR_UNION_T
{
	UINT32							au4Reg[1];
	MPEG4_VISUAL_OBJECT_HDR_FIELD_T	rField;
} MPEG4_VISUAL_OBJECT_HDR_UNION_T;
/// Video Signal Type Header Field
typedef struct _MPEG4_VIDEO_SIGNAL_HDR_FIELD_T
{
	/* 30/1 Bit */
	UINT32                                 :	2;               ///< Reserved
	UINT32          u4MatrixCoef    :	8;               ///< Matrix coefficients used in deriving luma and chroma from RGB primaries,
	UINT32		u4TransChar	    :	8;               ///< The opto-electronic transfer characteristic of the source picture
	UINT32		u4ColorPrim	    :	8;               ///< Chromaticity coordinates of the source primaries
	UINT32		fgColorDsc	    :	1;               ///< Indicates the presence of colour_primaries, transfer_characteristics and matrix_coefficients in the bitstream.
	UINT32		u4VideoRng	    :	1;               ///< Indicates the black level and range of the luminance and chrominance signals.
	UINT32		u4VideoFmt	    :	3;               ///< Indicating the representation of the pictures format
	UINT32		fgSignalType	    :	1;               ///< Indicates the presence of video_signal_type information. 
} MPEG4_VIDEO_SIGNAL_HDR_FIELD_T;
/// Video Signal Type Header Union
typedef union _MPEG4_VIDEO_SIGNAL_HDR_UNION_T
{
	UINT32							au4Reg[1];
	MPEG4_VIDEO_SIGNAL_HDR_FIELD_T	rField;
} MPEG4_VIDEO_SIGNAL_HDR_UNION_T;

/// Video Object Layer Header Field
typedef struct _MPEG4_VOL_HDR_FIELD_T
{
	/* 17 Bit */	
	UINT32                                   : 15;               ///< Reserved
	UINT32		u4VolPrior          :  3;               ///< The priority of the video object layer.
	UINT32		u4VolVerid	      :  4;               ///< The version number of the video object layer.
	UINT32		fgIsOlId              :  1;               ///< When set to '1' indicates that version identification and priority is specified for the visual object layer.
	UINT32		u4VoTypeInd      :  8;               ///< Constrains the bitstream to use tools from the indicated object type only
	UINT32		fgRandAccess     :  1;               ///< '1' to indicate that every VOP in this VOL is individually decodable.
      /* 25 Bits*/
      UINT32                                   :  7;               ///< Reserved
      UINT32      fgFixedVopRate      :  1;               ///< Indicates that all VOPs are coded with a fixed VOP rate.
      UINT32      fgMark7                 :  1;               ///< Marker Bit
      UINT32      u4VopTimeIncRes   :  16;             ///< The number of evenly spaced subintervals.
      UINT32      fgMark6                 :   1;               ///< Marker Bit
      UINT32      u4VolShapeExt       :  4;               ///< The number (up to 3) and type of auxiliary components that can be used.
      UINT32      u4VolShape            :  2;               ///< Identifies the shape type of a video object layer
	/* 16 Bits*/
	UINT32                                   : 16;               ///< Reserved
	UINT32      u4FixVopTimeInc    : 16;                ///< This value represents the number of ticks between two successive VOPs in the display order.
	/* 29 Bits*/
	UINT32                                   :  3;               ///< Reserved
	UINT32      fgMark10                :  1;               ///< Marker Bit
	UINT32      u4VolHeight            : 13;               ///< The height of the displayable part of the luma in pixel units.
	UINT32      fgMark9                  : 1;               ///< Marker Bit
	UINT32      u4VolWidth             : 13;               ///< The width of the displayable part of the luma in pixel units.
	UINT32      fgMark8                  :  1;               ///< Marker bit
	/* 4  Bits*/
	UINT32                                   : 28;               ///< Reserved
	UINT32      u4SpriteEnable       :  2;               ///< Define sprite mode
	UINT32      fgObmcDisable       :  1;               ///< Disables overlapped block motion compensation.
	UINT32      fgInterlaced           :  1;               ///< When set to ¡§1¡¨ indicates that the VOP may contain interlaced video.

       /* 14 Bits*/
       UINT32                                    :18;               ///< Reserved
       UINT32     ucQuantType            : 1;               ///< 
       UINT32     fgLinearComp           : 1;               ///< 
       UINT32     fgComposeMethod    : 1;               ///< 
       UINT32     fgNoGrayQuantUpd   : 1;               ///< 
       UINT32     u4BitsPerPixel          : 4;               ///< 
       UINT32     u4QuantPrecision     : 4;               ///< 
       UINT32     fgNot8Bit                 : 1;               ///< 
       UINT32     fgSadctDisable         : 1;               ///< 
} MPEG4_VOL_HDR_FIELD_T;
/// Video Object Layer Header Union
typedef union _MPEG4_VOL_HDR_UNION_T
{
	UINT32							au4Reg[6];
	MPEG4_VOL_HDR_FIELD_T	             rField;
} MPEG4_VOL_HDR_UNION_T;

/// Aspect Ratio Information Header Field
typedef struct _MPEG4_ASPECT_RATIO_HDR_FIELD_T
{
	/* 20 Bits*/
      UINT32                                    :12;               ///< Reserved
      UINT32		u4ParHeight        : 8;               ///< Indicates the vertical size of pixel aspect ratio.
      UINT32		u4ParWidth         : 8;               ///< Indicates the horizontal size of pixel aspect ratio.
      UINT32		u4AspectRatio	: 4;               ///< Defines the value of pixel aspect ratio.
} MPEG4_ASPECT_RATIO_HDR_FIELD_T;
/// Aspect Ratio Information Header Union
typedef union _MPEG4_ASPECT_RATIO_HDR_UNION_T
{
	UINT32					au4Reg[1];
	MPEG4_ASPECT_RATIO_HDR_FIELD_T	rField;
} MPEG4_ASPECT_RATIO_HDR_UNION_T;

/// Video Object Layer Control Parameter Header Field
typedef struct _MPEG4_VOL_CTRL_HDR_FIELD_T
{
	/* 21 Bits*/
      UINT32                                   :11;               ///< Reserved
      UINT32      fgMark1                 :  1;               ///< Marker Bit
      UINT32      u4FirstHalfBR         :15;               ///< Bitrate of the bitstream measured in units of 400 bits/second, rounded upwards. (MSB)
	UINT32      fgVbvParm             : 1;               ///< Indicates presence of following VBV parameters
	UINT32      u4LowDelay           : 1;               ///< Default value is 0 for visual object types that support B-VOP otherwise it is 1.
      UINT32      u4ChromaFmt        : 2;               ///< The chrominance format
	UINT32      fgVolCtrlParm        : 1;               ///< Indicates presence of the following parameters: chroma_format, low_delay, and vbv_parameters.
	/* 32 Bits*/
	UINT32      fgMark3                 : 1;               ///<  Marker Bit
	UINT32      u4FirstVbvSize       : 15;             ///< MSB of vbv_buffer_size
	UINT32      fgMark2                 : 1;               ///< Marker Bit
	UINT32      u4LatterHalfBR       : 15;             ///< Bitrate of the bitstream measured in units of 400 bits/second, rounded upwards. (LSB)
		
      /* 31 Bits*/
      UINT32                                    : 1;               ///< Reserved
      UINT32      fgMark5                  : 1;               ///< Marker bit
      UINT32      u4LatterVbvOccu    : 15;              ///<  LSB of vbv_occupancy
      UINT32      fgMark4                  :  1;              ///< Marker bit
      UINT32      u4FirstVbvOccu      : 11;              ///< MSB of vbv_occupancy
      UINT32      u4LatterVbvSize     :  3;               ///< LSB of vbv_buffer_size
} MPEG4_VOL_CTRL_HDR_FIELD_T;

/// Video Object Layer Control Parameter Header Union
typedef union _MPEG4_VOL_CTRL_HDR_UNION_T
{
	UINT32					au4Reg[3];
	MPEG4_VOL_CTRL_HDR_FIELD_T	rField;
} MPEG4_VOL_CTRL_HDR_UNION_T;

/// New Prediction Header Field
typedef struct _MPEG4_NEW_PRED_HDR_FIELD_T
{
	/* 5 Bits*/
      UINT32                                            : 27;
      UINT32      fgReducedResVopEnable  : 1;
	UINT32      u4NewPredSegType         : 1;
      UINT32      u4ReqUpStreamMsgType  : 2;
	UINT32      fgNewPredEnable             : 1;
} MPEG4_NEW_PRED_HDR_FIELD_T;

/// New Prediction Header Union
typedef union _MPEG4_NEW_PRED_HDR_UNION_T
{
	UINT32					au4Reg[1];
	MPEG4_NEW_PRED_HDR_FIELD_T	rField;
} MPEG4_NEW_PRED_HDR_UNION_T;

/// Scalability Header Field
typedef struct _MPEG4_SCAL_HDR_FIELD_T
{
	/* 27 Bits*/
      UINT32                                          : 5;
	UINT32     fgEnhacementType        : 1;
	UINT32     u4VerSampleFactorM      : 5;
	UINT32     u4VerSampleFactorN      : 5;
	UINT32     u4HorSampleFactorM      : 5;
      UINT32     u4HorSampleFactorN       : 5;
	UINT32     u4RefLayerSampleDirec   : 1;
      UINT32     u4RefLayerId                   : 4;
	UINT32     u4HierarchyType             : 1;

	/* 22 Bits*/
	UINT32                                          : 10;
	UINT32     u4ShapeVerSampM         : 5;
	UINT32     u4ShapeVerSampN         : 5;
	UINT32     u4ShapeHorSampM         : 5;
	UINT32     u4ShapeHorSampN         : 5;
	UINT32     u4UseRefText                 : 1;
	UINT32     u4UseRefShape              : 1;
} MPEG4_SCAL_HDR_FIELD_T;
/// Scalability Header Field
typedef union _MPEG4_SCAL_HDR_UNION_T
{
	UINT32					au4Reg[2];
	MPEG4_SCAL_HDR_FIELD_T	rField;
} MPEG4_SCAL_HDR_UNION_T;

/// Group of Video Object Plane Header Field
typedef struct _MPEG4_GOP_HDR_FIELD_T
{
	/* 20 Bit */
	UINT32								:	12;
	UINT32			fgBrokenLink		      :	1;
	UINT32			fgClosedGop			:	1;
	UINT32			u4Second			:	6;
	UINT32			fgMark				:	1;
	UINT32			u4Minute			:	6;
	UINT32			u4Hour				:	5;
} MPEG4_GOP_HDR_FIELD_T;

/// Group of Video Object Plane Header Union
typedef union _MPEG4_GOP_HDR_UNION_T
{
	UINT32					au4Reg[1];
	MPEG4_GOP_HDR_FIELD_T	rField;
} MPEG4_GOP_HDR_UNION_T;

/// Video Object Plane Short Header Field
typedef struct _MPEG4_VOP_SHORT_HDR_FIELD_T
{
  /* 27 Bits*/
  UINT32                                  : 5;
  UINT32    u4ZeroBit                : 1;
  UINT32    u4VopQuant            : 5;
  UINT32    u4FourResZeroBits   : 4;
  UINT32    u4PicCdTp               : 1;
  UINT32    u4SourceFmt           : 3;
  UINT32    fgFullPicFreezeRel   : 1;
  UINT32    fgDocCameraInd      : 1;
  UINT32    fgSplitScreenInd       : 1;  
  UINT32    ucZeroBit                  : 1;
  UINT32    fgMark                      : 1;
  UINT32    u4TempRef                :8;
  	
} MPEG4_VOP_SHORT_HDR_FIELD_T;

/// Video Object Plane Short Header Union
typedef union _MPEG4_VOP_SHORT_HDR_UNION_T
{
	UINT32					             au4Reg[1];
	MPEG4_VOP_SHORT_HDR_FIELD_T	rField;
} MPEG4_VOP_SHORT_HDR_UNION_T;
/*! @} */

/*! \name Struct for MPEG4 Driver Info
* @{
*/
/// Visaul Object Sequence Information 
typedef struct _MEPG4_VOS_HDR_T
{    
    BOOL    fgVosHdrValid;               ///< Indication wheter VOS header is valid or not
    UCHAR   ucProfileLevel;              ///< Profile and Level
} MEPG4_VOS_HDR_T;

/// Visaul Object Object Information
typedef struct _MEPG4_VISUAL_OBJECT_HDR_T
{    
    BOOL     fgVoHdrValid;              ///< Is visual oject header valid
    BOOL     fgIsVoId;                    ///< flag to inform there is visual Object version info or not
    UCHAR   ucVoVerid;                  ///< visual object version id
    UCHAR   ucVoPrior;                   ///< visual object priority
    UCHAR   ucVoType;                   ///< visual object type
    
    BOOL     fgSignalType;              ///< flag to inform there is signal type info or not
    UCHAR   ucVideoFmt;               ///< video signal format
    UCHAR   ucVideoRange;            ///< the black level and range of the luminance and chrominance signals.
    BOOL     fgColorDes;                ///< flag to inform there is color description or not
    UCHAR   ucColorPrimaries;        ///< the chromaticity coordinates of the source primaries
    UCHAR   ucTransferChar;          ///< opto-electronic transfer characteristic of the source picture
    UCHAR   ucMatrixCoef;             ///< the matrix coefficients used in deriving luma and chroma from RGB
    
} MEPG4_VO_HDR_T;

/// Visaul Object Sequence Layer Information
typedef struct _MEPG4_VOL_HDR_T
{    
    BOOL        fgVolHdrValid;           ///< Is Vol header valid or not
    BOOL        fgRandAccVol;           ///< flag may be set to ¡§1¡¨ to indicate that every VOP in this VOL is individually decodable.
    UCHAR      ucVoTypeInd;            ///< video object type indication
    BOOL 	     fgIsOlId;                   ///< '1' indicates that version id and priority is specified for the vOL
    UCHAR	     ucVolVerid;               ///< Vol version identification
    UCHAR	     ucVolPrior ;               ///< Vol priority
    UCHAR       u2FrmRatCod;
    
    
    UCHAR      ucAspectRatio;          ///< Defines the value of pixel aspect ratio.
    UCHAR      ucParWidth;              ///< Horizontal size of pixel aspect ratio.
    UCHAR      ucParHeight;	            ///< Vertical size of pixel aspect ratio.
    
    BOOL        fgVolCtrlParm;          ///< '1' indicates presence of chroma_format, low_delay, and vbv_parameters.
    UCHAR      ucChromaFmt;          ///< the chrominance format
    UCHAR      ucLowDelay;             ///< '1' indicates the VOL contains no B-VOPs.
    BOOL        fgVbvParm;              ///< '1' indicates presence of VBV parameters
    UINT16     u2FirstBitRate;
    UINT16     u2LatterBitRate;
    UINT16     u2FirstVbvBufSize;
    UCHAR      ucLatterVbvBufSize;
    UINT16     u2FirstVbvOccu;
    UINT16     u2LatterVbvOccu;
    
    UINT16     u2VolShape;
    UCHAR      ucVolShapeExt;
    UINT16     u2VopTimeIncRes;
    UCHAR      ucVopTimeIncrementResolutionBits;
    BOOL        fgFixedVopRate;
    UINT16     u2FixVopTimeInc; 
    
    UINT16     u2VolWidth;
    UINT16     u2VolHeight;
    
    BOOL       fgInterlaced;
    BOOL       fgProgressiveFrm;
    BOOL       fgProgressiveSeq;
    
    BOOL       fgObmcDisable;
    UCHAR     ucSpriteMode;
    UCHAR     ucNoSpriteWarpPoint;
    UCHAR     ucSpriteAccuracy;
    BOOL       fgSpriteBrightChange;
    BOOL       fgLowLatencySpriteEn;
    UCHAR     ucEffectiveWarpingPoints;

    BOOL       fgSadctDisable;
    BOOL       fgNot8Bit;
    UCHAR     ucQuantPrecision;
    UCHAR     ucBitsPerPixel;

    BOOL       fgNoGrayQuantUpd;
    BOOL       fgCompositeMethod;
    BOOL       fgLinearComp;

    UCHAR     ucQuantType;
    
    UCHAR     ucQuarterSample;
    BOOL       fgComplexityEstDisable;

    UCHAR     fgResyncMarkerDisable;
    BOOL       fgDataPartition;
    BOOL       fgReversibleVLC;
    
    BOOL       fgNewpredEnable;
    BOOL       fgReducedResVopEnable;
    BOOL       fgScalability;
    BOOL       fgEnhancementType;
} MEPG4_VOL_HDR_T;

/// Gropu of Video Object Plane Information
typedef struct _MEPG4_GOP_HDR_T
{
    BOOL                     fgGopHdrValid;
    BOOL                     fgClosedGop;
    BOOL                     fgBrokenLink;
    UINT32			u4Second;
    UINT32			u4Minute;
    UINT32			u4Hour;
} MEPG4_GOP_HDR_T;

/// Video Object Plane Information
typedef struct _MEPG4_VOP_HDR_T
{
    BOOL       fgVopHdrValid;
    UCHAR     ucPreVopType;
    UCHAR     ucVopCdTp;
    UCHAR     ucPicStruct;
    UINT16    u2VopTimeIncrement;
    BOOL       fgVopCoded;
    UCHAR     ucVopRoundingType;
    BOOL       fgVopReducedRes;

    UINT32    u4TimeBase;
    UINT32    u4RefPicTimeBase;
    UINT32    u4CurrDispTime;
    UINT32    u4PrevDispTime;
    UINT32    u4NextDispTime;    
    
    UCHAR     ucIntraDcVlcThr;    	
    BOOL       fgTopFieldFirst;
    BOOL       fgAltVerScanFlag;

    UINT32    u2VopQuant;
    UCHAR     ucFcodeFwd;
    UCHAR     ucFcodeBwd;

    BOOL       fgSorensonH263;
    BOOL       fgVopShortHdrValid;
    UCHAR     ucTempRef;
    BOOL       fgSplitScreenInd;
    BOOL       fgDocCameraInd;
    BOOL       fgFullPicFreezeRel;
    UCHAR     ucSourceFmt;
    UCHAR     ucPreTempRef;

    BOOL       fgRepFirstFld;
    
    INT32      i4WarpingMv[4][2];
    VDEC_INFO_MPEG_DIR_MODE_T     rDirMode;

    //Divx3 Header
    UINT32   u4Divx3SetPos;
    UCHAR    ucFrameMode;
    BOOL      fgAltIAcChromDct;
    BOOL      fgAltIAcChromDctIdx;
    BOOL      fgAltIAcLumDct;
    BOOL      fgAltIAcLumDctIdx;
    BOOL      fgAltIDcDct;
    BOOL      fgHasSkip;
    BOOL      fgAltPAcDct;
    BOOL      fgAltPAcDctIdx;
    BOOL      fgAltPDcDct;
    BOOL      fgAltMv;
    BOOL      fgSwitchRounding;
    
} MEPG4_VOP_HDR_T;

/// Divx Detail Information
typedef struct _MEPG4_DIVX_INFO_T
{
    UINT32     u4UserDataCodecVersion;
    UINT32     u4UserDataBuildNumber;
    BOOL        fgDivXM4v;
    BOOL        fgUseDivXInfo;
    BOOL        fgIsDivXFile;
} MEPG4_DIVX_INFO_T;

/// Decoded Picture Buffer Status Management
typedef enum
{
  MPEG4_DPB_STATUS_EMPTY = 0,         ///< DPB is  Empty and Free
  MPEG4_DPB_STATUS_READY,               ///<  DPB is ready for decoding
  MPEG4_DPB_STATUS_DECODING,         ///< DPB is used by decoder
  MPEG4_DPB_STATUS_DECODED,          ///< DPB contains decoded frame
  MPEG4_DPB_STATUS_OUTPUTTED,       ///< DPB was sent to display queue
  MPEG4_DPB_STATUS_FLD_DECODED,   ///< 1st field wad decoded
  MPEG4_DPB_STATUS_DEC_REF,           ///< DBP is used for reference
  MPEG4_DPB_STATUS_OUT_REF,           ///< DBP was displayed and is used for reference
}MPEG4_DPB_COND_T;

/// Decoded Picture Buffer Index
typedef enum
{
  MPEG4_DPB_FBUF_UNKNOWN = VDEC_FB_ID_UNKNOWN,
  MPEG4_DPB_FREF_FBUF = 0,
  MPEG4_DPB_BREF_FBUF = 1,
  MPEG4_DPB_WORKING_FBUF= 2,
}MPEG4_DPB_IDX_T;

/// Decoded Picture Buffer Information
typedef struct _MPEG4_DBP_INFO_T
{
	BOOL fgVirtualDec;
    UCHAR ucDpbFbId;
    MPEG4_DPB_COND_T eDpbStatus;
}MPEG4_DBP_INFO_T;
/*! @} */

/*! \name MPEG4 Driver Information
* @{
*/
/// \ingroup API
typedef struct _MEPG4_DEC_PRM_T
{   
    BOOL fgLoadIntraMatrix;		///<  Is load_intra_quantizer_matrix;
    BOOL fgLoadNonIntraMatrix;	///<  Is load_non_intra_quantizer_matrix;    
    UINT32 *pu4IntraMatrix;         ///<  Intra quantizer matrix pointer
    UINT32 *pu4NonIntraMatrix;   ///<  Inter quantizer matrix pointer
    UINT32 u4IntraMatrixLen;        ///<  Intra quantizer matrix length
    UINT32 u4NonIntraMatrixLen;  ///<  Inter quantizer matrix length
    UINT32 u4MatrixId;                 ///< Quantizer matrix index before matrix updated
    UINT32 u4BitCount;                 ///< Consumed bits counter
#if 0    
    UINT32 u4SkipCount;
    UINT32 u4DummyCount;
    INT64   u8BasePTS;
    INT64 i8PredPTS;
    INT64 i8PTSDuration;
#endif    
    UCHAR ucDCACFbId;
    UCHAR ucMVECFbId;
    UCHAR ucBMB1FbId;
    UCHAR ucBMB2FbId;
    UCHAR ucBCODEFbId;
    INT64 i8BasePTS;
    INT64 i8LatestRealPTS;
    INT64 i8DiffCnt;
    INT64 i8DropIPCnt;
    INT64 i8DropBCnt;
    INT64 i8DummyCnt;
    UINT32 u4CurrentQMatrixId;   ///< Current quantizer matrix index
    UINT32 u4QMatrixCounter;      ///< Quantizer matrix count
    MPEG4_DPB_IDX_T eDpbOutputId;         ///< DPB output index: Unknown, FRef, BFre, Working
    MPEG4_DBP_INFO_T arMPEG4DpbInfo[MPEG4_DPB_NUM];     // FbId and Dbp status
    MPEG4_DBP_INFO_T arMPEG4DebInfo[MPEG4_DPB_NUM];     //Deblocking buffer info
    VDEC_NORM_INFO_T *prVDecNormInfo;      ///< Normal info: CodecType, EsId, VldId, BsId, LastPicTp, etc.
    VDEC_PIC_INFO_T *prVDecPicInfo;             ///< Picture info: PicStruct, PicType, PicW, PicH, FifoStart, etc.
    VDEC_FBM_INFO_T    *prVDecFbmInfo;
   // VID_DEC_SEQUENCE_INFO_T rPrevSeqInfo;           ///< Visual object Sequence Header    
    MEPG4_VOS_HDR_T rMPEG4VosHdr;           ///< Visual object Sequence Header
    MEPG4_VO_HDR_T rMPEG4VoHdr;              ///< Visual Object Header
    MEPG4_VOL_HDR_T rMPEG4VolHdr;            ///< Video Object Layer Header
    MEPG4_GOP_HDR_T rMPEG4GopHdr;          ///< Group of Video Object Plane Header
    MEPG4_VOP_HDR_T rMPEG4VopHdr;          ///< Video Object Plane Header
    MEPG4_DIVX_INFO_T rMPEG4DivxInfo;       ///< Divx Info: Version, Build number
    VDEC_INFO_MPEG_VFIFO_PRM_T rVDecMPEGVFifoPrm;      ///< FifoSa, FifoEa
    VDEC_INFO_MPEG_BS_INIT_PRM_T rVDecMPEGBsInitPrm[2];  ///< Barrel Shifter: Rd pointer, Wr pointer,  FifoSa, FifoEa
    VDEC_INFO_MPEG_QANTMATRIX_T rVDecMPEGIntraQM;        ///< Intra quantizer array
    VDEC_INFO_MPEG_QANTMATRIX_T rVDecMPEGNonIntraQM;  ///< Inter quantizer array
    VDEC_INFO_MPEG_ERR_INFO_T rVDecMPEGErrInfo;              ///< Erro Info
    VDEC_INFO_DEC_PRM_T rVDecNormHalPrm;                        ///< HAL parameter: picstruct, pictype, picW, picH
    UCHAR ucLastOutFbId;
    UCHAR ucDummyRefId;
    BOOL   ucDummyRefFbLock;
    UCHAR ucOverSpecNtfyCnt;
    BOOL   fgDivXSupport;
    BOOL   fgDivXHTSupport;
    BOOL   fgDivXUltraSupport;
    BOOL   fgDivXHDSupport;	
#ifdef VDEC_SUPPORT_HIGH_FRAME_RATE
    BOOL   fgSeqUnknownFrameRate;
    BOOL   fgUseHighFrameRate;
    UINT32 u4RealFrameDuration;
#endif
    UINT32 u4XvidCodecVersion;           // XVID codec version
    BOOL   fgIsXvidCodec;                    // Video codec is XVID
} MPEG4_DRV_INFO_T;
/*! @} */

/******************************************************************************
* Function prototype
******************************************************************************/
//extern UINT32 _au4CurrentQMatrixId[MPV_MAX_VLD];
//extern UINT32 _au4QMatrixCounter[MPV_MAX_VLD];


extern void vMPEG4Dx3SufxChk(VDEC_ES_INFO_T* prVDecEsInfo);
extern BOOL fgMPEG4ChkRealEnd(MPEG4_DRV_INFO_T *prMpeg4DrvInfo);

/// \ingroup Header
extern INT32  vMPEG4Parser(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VParseType);

extern void vSetMpeg4InitValue(VDEC_ES_INFO_T* prVDecEsInfo);

/******************************************************************************
* Local macro
******************************************************************************/
/// \ingroup VLD
#define INVERSE_ENDIAN(value)    	\
		(((value & 0xFF) << 24) + ((value & 0xFF00) << 8) + ((value & 0xFF0000) >> 8) + ((value & 0xFF000000) >> 24))
/// \ingroup VLD
#define CCSIZE(wp, rp, bufsize) \
        (((wp) >= (rp)) ? ((wp) - (rp)) : (((bufsize) + (wp)) - (rp)))

/// \ingroup VLD
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

/*! \name MPEG4 Driver API
* @{
*/
/// \ingroup API 
/// This function allocates memory for driver information
/// - This API can should be called in the begining.
/// .
/// \return None.
extern void vMPEG4InitProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function parse mpeg4 bitstream headers
/// - This API can should be called before starting to decode.
/// .
/// \return parsing result. Please reference to Vdec_errcode.h
extern INT32 i4MPEG4VParseProc(
                       UCHAR ucEsId,     ///< [IN] the ID of the elementary stream
                       UINT32 u4VParseType ///< [IN] the specified pic type or header
);

/// This function checks the parsing result.
/// \return VDEC_NONE_ERROR: parse OK. Else: please reference to Vdec_errcode.h
extern BOOL fgMPEG4VParseChkProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function updates information to Frame Buffer Group
/// \return VDEC_NONE_ERROR: udate OK. Else: please reference to Vdec_errcode.h
extern INT32 i4MPEG4UpdInfoToFbg(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function triggers Video decoder hardware
///\ return None.
extern void vMPEG4StartToDecProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function is interrrupt service routine
///\ return None.
extern void vMPEG4ISR(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);
/// This function checks the decoded picture is complete or not
/// - This API will check decoded picture in MB_X and MB_Y
/// .
///\ return TRUE: decode OK, FALSE: decoding failed, drop it
extern BOOL fgIsMPEG4DecEnd(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function checks the decoded error type and count
/// - This API will check decoded error type and count
/// .
///\ return TRUE: decode error, FALSE: decode correct
extern BOOL fgIsMPEG4DecErr(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function checks is there any error code happened in HW
///\ return TRUE: decode OK, FALSE: decoding failed, drop it
extern BOOL fgMPEG4ResultChk(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function sets up frame buffer index ready for display
///\ return TRUE: buffer index for display OK, FALSE: cannot get buffer for display
extern BOOL fgIsMPEG4InsToDispQ(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function gets one decoded frame buffer and send to display
///\ return TRUE: get frame buffer OK, FALSE: Cannot get buffer for display
extern BOOL fgIsMPEG4GetFrmToDispQ(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);

/// This function does ending procedure
///\ return None.
extern void vMPEG4EndProc(
                       UCHAR ucEsId     ///< [IN] the ID of the elementary stream
);
/// This function flushes all decoded picture in DPB Buffer
///\ return TRUE: flush OK, FALSE: cannot get buffer for display
extern BOOL fgMPEG4FlushDPB(
                       UCHAR ucEsId,     ///< [IN] the ID of the elementary stream
                       BOOL fgWithOutput   ///< [IN] '1' indicates to output decoded frame
);
/// This function will release resources accupied by decoder
///\ return None.
extern void vMPEG4ReleaseProc(
                       UCHAR ucEsId,     ///< [IN] the ID of the elementary stream
                       BOOL fgResetHW
);

extern void vMPEG4SetMcBufAddr(UCHAR ucFbgId, VDEC_ES_INFO_T* prVDecEsInfo);
extern void vMPEG4DpbBufCopy(MPEG4_DRV_INFO_T *prMpeg4DrvInfo, UCHAR ucTarDpbBuf, UCHAR ucSrcDpbBuf);
extern INT32 i4MPEG4OutputProc(UCHAR ucEsId, VDEC_ES_INFO_T* prVDecEsInfo);
extern void vSetMpeg2Var(MPEG4_DRV_INFO_T *prMpeg4DrvInfo);
extern void vMPEG4SetColorPrimaries(MPEG4_DRV_INFO_T *prMPEG4DrvDecInfo, UINT32 u4ColorPrimaries);
extern void vMPEG4SetSampleAsp(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4MPEG4Asp);
extern void vMPEG4SetFrameTimingInfo(MPEG4_DRV_INFO_T *prMPEG4DrvDecInfo, UINT16 u2FrameRate);
extern void _MPEG4SetDecPrm(MPEG4_DRV_INFO_T *prMpeg4DrvInfo);
extern void vMPEG4CalGmcMv(MPEG4_DRV_INFO_T *prMpeg4DrvInfo);
#ifdef FBM_ALLOC_SUPPORT
extern void vFreeMpeg4WorkingArea(VDEC_ES_INFO_T *prVDecEsInfo);
#endif
extern void vMPEG4SetDownScaleParam(MPEG4_DRV_INFO_T *prMpeg4DrvInfo, BOOL fgEnable);

/// This function will return vdec driver interface pointer
///\ return VDEC_DRV_IF*
extern VDEC_DRV_IF* VDec_GetMPEG4If(void);
/*! @} */
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
extern void vMPEG4SetLetterBoxParam(MPEG4_DRV_INFO_T *prMpeg4DrvInfo);
#endif

#ifdef MPV_DUMP_FBUF
extern void VDec_Dump_Data(UINT32 u4StartAddr, UINT32 u4FileSize, UINT32 u4FileCnt, UCHAR* pucAddStr);
#endif

#ifdef DRV_VDEC_SUPPORT_FBM_OVERLAY
extern BOOL fgMPEG4NeedDoDscl(UCHAR ucEsId);
#endif

#endif
