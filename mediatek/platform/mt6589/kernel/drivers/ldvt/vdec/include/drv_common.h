
#if ! defined(DRV_COMMON_H)
#define DRV_COMMON_H

//#define USE_2_ES
/////////////////////////////////////////////////////////////////////////////////////
//                                              Below added for BD_P                                                                //
/////////////////////////////////////////////////////////////////////////////////////
//#include "x_typedef.h"
//#include "drv_config.h"
//#include "dram_model.h"
//#include "chip_ver.h"

//#include "sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif


#define NEW_PAUSE_MODE 1

// For FLV media use sorenson H263 codec @2009/06/26
#if 0//CONFIG_DRV_SUPPORT_SORENSON_H263
#define DRV_SUPPORT_SORENSON_H263  (1)
#else
#define DRV_SUPPORT_SORENSON_H263  (0)
#endif

#if DRV_SUPPORT_SORENSON_H263
#define DRV_SORENSON_H263_HW_HDR_DETECTION 1
#else
#define DRV_SORENSON_H263_HW_HDR_DETECTION 0
#endif

//#define DRV_HIGH_BITRATE_PROC_CFG  (1)
#if CONFIG_DRV_HIGH_BITRATE_SPECIAL_PROC
#define CONFIG_DRV_SUPPORT_CMD_Q_TX (1) // Can open directly later without considering high bitrate
#else
#define CONFIG_DRV_SUPPORT_CMD_Q_TX (0)
#endif

#if CONFIG_DRV_SUPPORT_CMD_Q_TX
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
#define DMX_MAX_TX_CNT_FOR_CMD_Q  (50)
#else
#define DMX_MAX_TX_CNT_FOR_CMD_Q  (50)
#endif

#define LPDMX_CMD_Q_TX_REG_DW_IDX  (19)

typedef struct
{
  UINT32 u4TxOfst;
  UINT32 u4TxLen;
} CMDQ_TX_ENTRY_T;
#else
// See PTX_CMDQ_NUM in dmx_verify.h ???
// Can use DMX_MAX_TX_CNT_FOR_CMD_Q instead of PTX_CMDQ_NUM ???
#define DMX_MAX_TX_CNT_FOR_CMD_Q  (40) // (1)
#endif

// Can remove following 2 define if all ready @2009/01/06
#if CONFIG_DRV_SUPPORT_RM
#define CONFIG_DRV_SUPPORT_RM_VID_DYNC_MEM (1)
#define CONFIG_DRV_SUPPORT_RM_COOK_AUD (1)
#else
#define CONFIG_DRV_SUPPORT_RM_VID_DYNC_MEM (0)
#define CONFIG_DRV_SUPPORT_RM_COOK_AUD (0)
#endif

#define CONFIG_DRV_SUPPORT_SKYPE (0)
#define CONFIG_DRV_XVID_ENABLE (1)
#if CONFIG_DRV_XVID_ENABLE
#define VC_XVID_ENABLE
#endif


/// Supported address swap mode in driver layer
typedef enum
{
  ASM_0 = 0,           ///< 8520 no address swap
  ASM_1,                 ///< 8520 address swap mode 1
  ASM_2,                  ///< 8520 address swap mode 2
  ASM_3,                 ///< 5351 address swap mode 0
  ASM_4,                 ///< 5351 address swap mode 1
  ASM_5,                 ///< 5351 address swap mode 2
  ASM_6,                 ///< 5351 address swap mode 3
}DRV_ASM;

/// Supported frame buffer type in driver layer
typedef enum
{
  FBT_420_RS = 0,     ///< YCbCr 420 raster scan
  FBT_420_BK,           ///< YCbCr 420 block
  FBT_422_RS,           ///< YCbCr 422 raster scan
  FBT_422_BK,           ///< YCbCr 422 block
  FBT_420_BK_YCBIND,      ///< YCbCr 420 block, Y C memory are bound, for H.264 request
  FBT_420_BK_YONLY,        ///< YCbCr 420 block, Y memory only, no CbCr, for H.264 request
  FBT_WORKSPACE,    ///< One continue memory, like JPEG working space
  FBT_PBBUF,   ///< One continue memory, overlay with one HD main buffer
  FBT_BGIMG  /// < One continue memory, overlay with whole sub buffer
} DRV_FBTYPE;


//#define DRV_SUPPORT_ADDRESS_SWAP
#define DRV_ADDRESS_SWAP_MODE ASM_5

#define DRV_ADDRESS_SWAP_OFF ASM_0

#define DRV_SUPPORT_DEC_ERR_DROP_LEVEL

//#define DRV_SUPPORT_VDEC_DOWN_SCALE

#define DRV_SUPPORT_FRC_VAR

#if (CONFIG_DRAM256_MODEL || (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8550) || (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560))
#define DRV_VDEC_VDP_RACING
#endif

#define DRV_VDP_SUPPORT_ONE_SOURCE_TWO_DISPLAY

#ifdef DRV_VDEC_VDP_RACING
#define DRV_VDEC_SUPPORT_FBM_OVERLAY
#endif

#if CONFIG_DRAM256_MODEL

#define DRV_FBM_ORIG_DSCL_OVERLAP
#define DRV_FBM_GENERAL_MEM_ALLOC
#ifdef DRV_FBM_GENERAL_MEM_ALLOC
#if !(CONFIG_SYS_MEM_PHASE3 || CONFIG_DRV_3D_384_SUPPORT || CONFIG_DRV_DRAM256_PLUS_NEWFEATURE)
#define DRV_PBBUF_FRMBUF_OVERLAP
#endif
#endif

#endif

#if CONFIG_DRAM256_MODEL
#if !CONFIG_DRV_3D_384_SUPPORT
#define DRV_BKIMG_FRMBUF_OVERLAP
#endif
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    #define VDEC_3D_ONE_HW    VDEC_3D_RACING
#else
#define VDEC_3D_ONE_HW    0
#endif


#if ( (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8550) || (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) )
#define VDEC_PIP_WITH_ONE_HW
#endif


#define VDEC_3D_RACING CONFIG_DRV_3D_384_SUPPORT

#if  (defined(DRV_VDEC_VDP_RACING) || defined(VDEC_PIP_WITH_ONE_HW) || (VDEC_3D_RACING))
#define VDEC_PIP_NEW_FLOW 1
#else
#define VDEC_PIP_NEW_FLOW 0
#endif


//#define DRV_NEW_CHIP_BOUNDING
// *********************************************************************
// Video Codec
// *********************************************************************
/*! \name Video Codec
* @{
*/
typedef enum
{
  VC_UNKNOW = 0,       ///< unknow type, used for debug
  VC_MPEG2,                ///< mpeg 1/2
  VC_MPEG4,                ///< mpeg 4
  VC_XVID,                  ///<  xvid
  VC_DIVX311,             ///< Divx 3.11
  VC_DIVX4,                  ///< Divx 4
  VC_DIVX6,                  ///< Divx 5/6
  VC_WMV1,                    ///< WMV7
  VC_WMV2,                    ///< WMV8
  VC_WMV3,                    ///< WMV9
  VC_VC1,                    ///< VC1
  VC_H263,                    ///< H.263
  #if 1 //DRV_SUPPORT_SORENSON_H263
  VC_H263_SORENSON,		      ///< H.263 Sorenson version
  #endif
  VC_H264,                    ///< H.264
  VC_RV30,                    ///< Real Video 8
  VC_RV40,                    ///< Real Video 9,10
  VC_MJPG,                    ///<motion jpeg
  VC_VP6,                    ///VP6
}VCodeC;
/*! @} */
#define SetVCodec(u4PicType, eVCodec)  (u4PicType = (u4PicType & 0xFFFF00FF) | (eVCodec << 8))
#define GetVCodec(u4PicType)  ((u4PicType & 0x0000FF00) >> 8)
#define CkeckVPicType(u4Type, u4LastType)  (u4Type & u4LastType & 0xFFFF00FF)

#define IsMpeg2Pic(u4PicType) (((u4PicType & 0x0000FF00) >> 8) == VC_MPEG2)

#define IsM4vPic(u4PicType) ((((u4PicType & 0x0000FF00) >> 8) == VC_MPEG4) || \
                             (((u4PicType & 0x0000FF00) >> 8) == VC_DIVX311) || \
                             (((u4PicType & 0x0000FF00) >> 8)== VC_DIVX4) || \
                             (((u4PicType & 0x0000FF00) >> 8) == VC_DIVX6) || \
                             (((u4PicType & 0x0000FF00) >> 8) == VC_H263) || \
      										   (((u4PicType & 0x0000FF00) >> 8) == VC_H263_SORENSON))

#define IsDivxPic(u4PicType) ((((u4PicType & 0x0000FF00) >> 8) == VC_DIVX311) || \
                             (((u4PicType & 0x0000FF00) >> 8)== VC_DIVX4) || \
                             (((u4PicType & 0x0000FF00) >> 8) == VC_DIVX6))

#define IsWMVPic(u4PicType) ((((u4PicType & 0x0000FF00) >> 8) == VC_WMV1) || \
                             (((u4PicType & 0x0000FF00) >> 8) == VC_WMV2) || \
                             (((u4PicType & 0x0000FF00) >> 8)== VC_WMV3) || \
                             (((u4PicType & 0x0000FF00) >> 8) == VC_VC1))

#define IsH264Pic(u4PicType) (((u4PicType & 0x0000FF00) >> 8) == VC_H264)

#define IsH264RealPic(u4PicType) (((u4PicType & 0xFF) == P_SLICE) || ((u4PicType & 0xFF) == B_SLICE) ||\
	                        ((u4PicType & 0xFF) == I_SLICE) || ((u4PicType & 0xFF) == SP_SLICE) || ((u4PicType & 0xFF) == SI_SLICE) ||\
	                        ((u4PicType & 0xFF) == P_ALL_SLICE) || ((u4PicType & 0xFF) == B_ALL_SLICE) || ((u4PicType & 0xFF) == I_ALL_SLICE) ||\
	                        ((u4PicType & 0xFF) == SP_ALL_SLICE) || ((u4PicType & 0xFF) == SI_ALL_SLICE))

// *********************************************************************
// Picture Coding Type
// *********************************************************************
/*! \name Extra Type of Picture information
* @{
*/
#define SEQ_HDR       1 << 16    ///< Access unit include a sequence header
#define GOP_HDR       1 << 17    ///< Access unit include a GOP header
#define SEQ_END       1 << 18    ///< Access unit include a sequence end
#define ANGLE_END     1 << 19    ///< Access unit include a angle end
#define SEQ_PS       1 << 20    ///< Access unit include a sequence parameter set in H264
#define PIC_PS       1 << 21    ///< Access unit include a picture parameter set in H264
#define SEI             1 << 22    ///< Access unit include a supplement enhancement information in H264
#define REF_PIC       1 << 23    ///< Access unit is a reference picture in H264
#define IDR_PIC       1 << 24    ///< Access unit is a IDR picture in H264
#define ENTRY_PTR   1 << 25    ///< Access unit is a Entry Pointer in WMV
#define AUTO_PAUSE 1<<26
#define MULTISLICE_PIC    1<<27    ///Access unit is a multi-slice picture (only for H264)
#define SUB_SEQ_PS    1<<28    ///Access unit is a multi-slice picture (only for H264)
#define PREFIX_NAL    1<<29    ///Access unit is a multi-slice picture (only for H264)
#define ANCHOR_PIC    1<<30    ///Access unit is a anchor picture (only for H264)

/*! @} */

/*! \name MPEG2 Picture Coding Type
* @{
*/
#define I_TYPE                  1
#define P_TYPE                 2
#define B_TYPE                 3
#define D_TYPE                 4
#define DUMMY_TYPE        5        // used for record mp2 into AVI file in MTK recorder
//#define V_SEQ_HDR          8
//#define V_GOP_HDR          9
//#define V_SEQ_END          10
//#define AGL_SEQ_END       11
/*! @} */

/*! \name MPEG4 Picture Coding Type
* @{
*/
#define VIS_OBJ                0x8b   ///< visual_object_start_code,      000001B5
#define VID_OBJ_LAY       0x85   ///< video_object_layer_start_code, 000001[20-2f]
//#define VID_OBJ             0x84  ///< video_object_start_code,       000001[00~1f]
#define GOVOP                   0x89  ///< group_of_vop_start_code,       000001B3
#define I_VOP                    0x80  ///< vop_start_code                 000001B6
#define P_VOP                   0x81  ///< vop_start_code                 000001B6
#define B_VOP                    0x82  ///< vop_start_code                 000001B6
#define S_VOP                    0x83  ///< vop_start_code                 000001B6
#define SH_I_VOP              0x98  ///< short_video_start_marker
#define SH_P_VOP              0x99  ///< short_video_start_marker
#define DX3_I_FRM            0xf0  ///< generated by firmware
#define DX3_P_FRM           0xf1  ///< generated by firmware
#define DUMMY_FRM          0xf2  ///< generated by firmware, used for dummy frame, like vop_coded 0
/*! @} */

/*! \name H264 Picture Coding Type
* @{
*/
#define I_SLICE                0x64   ///< slice type 2 as I slice
#define P_SLICE                0x65   ///< slice type 0 as P slice
#define B_SLICE                0x66   ///< slice type 1 as B slice
#define SI_SLICE               0x67   ///< slice type 4 as SI slice
#define SP_SLICE              0x68   ///< slice type 3 as SP slice
#define I_ALL_SLICE         0x6a   ///< slice type 7 as I slice, all I type slices in this picture
#define P_ALL_SLICE         0x6b   ///< slice type 5 as P slice, all P type slices in this picture
#define B_ALL_SLICE         0x6c   ///< slice type 6 as B slice, all B type slices in this picture
#define SI_ALL_SLICE        0x6d   ///< slice type 9 as I slice, all SI type slices in this picture
#define SP_ALL_SLICE        0x6f   ///< slice type b as P slice, all SP type slices in this picture
/*! @} */

/*! \name WMV Picture Coding Type
* @{
*/
#define IVOP           0xa0
#define PVOP          0xa1
#define BVOP          0xa2
#define BIVOP         0xa3
#define SKIPFRAME  0xa4
/*! @} */

/*! \name Real Video Picture Coding Type
* @{
*/
#define INTRAPIC                        0xB0
#define FORCED_INTRAPIC          0xB1
#define INTERPIC                         0xB2
#define TRUEBPIC                        0xB3
/*! @} */

/*! \name MJPG Coding Type
* @{
*/
#define MJPG_I_FRM                      0xC0
/*! @} */

/*! \name AVS Picture Coding Type
* @{
*/
#define I_PIC                        0x0
#define P_PIC                       0x1
#define B_PIC                       0x2
/*! @} */


/*! \name VP6 Picture Coding Type
* @{
*/
#define VP6_I_FRM              0x0
#define VP6_P_FRM              0x1
/*! @} */

/*! \name Picture Coding Type Marco
* @{
*/

#define fgIsIType_MP2(arg)    ((arg&0xFF) == I_TYPE)

#define fgIsIType_MP4(arg)    (((arg&0xFF) == I_VOP) || \
                                                 ((arg&0xFF) == SH_I_VOP) || \
                                                 ((arg&0xFF) == DX3_I_FRM))

#define fgIsIType_AVC(arg)    ((arg & IDR_PIC) || \
                                                ((arg&0xFF) == I_ALL_SLICE) || \
                                                ((arg&0xFF) == I_SLICE) || \
                                                ((arg&0xFF) == SI_SLICE) || \
                                                ((arg&0xFF) == SI_ALL_SLICE))

#define fgIsIType_WMV(arg)   ((arg&0xFF) == IVOP)

#define fgIsPType_MP2(arg)   (((arg&0xFF) == P_TYPE) || \
	                                         ((arg&0xFF) == DUMMY_TYPE))

#define fgIsPType_AVC(arg)   (((arg&0xFF) == P_SLICE) || \
                                                ((arg&0xFF) == SP_SLICE) || \
                                                ((arg&0xFF) == P_ALL_SLICE) || \
                                                ((arg&0xFF) == SP_ALL_SLICE))

#define fgIsPType_WMV(arg)   (((arg&0xFF) == PVOP) || \
                                                   ((arg&0xFF) == SKIPFRAME))

#define fgIsBType_MP2(arg)   ((arg&0xFF) == B_TYPE)

#if 0
#define fgIsPType_MP4(arg)   (((arg&0xFF) == P_VOP) || \
                                                ((arg&0xFF) == S_VOP) || \
                                                ((arg&0xFF) == SH_P_VOP) || \
                                                ((arg&0xFF) == DX3_P_FRM) || \
                                                ((arg&0xFF) == DUMMY_FRM))
#define fgIsBType_MP4(arg)   ((arg&0xFF) == B_VOP)

#else
#define fgIsPType_MP4(arg)   (((arg&0xFF) == P_VOP) || \
                                                ((arg&0xFF) == S_VOP) || \
                                                ((arg&0xFF) == SH_P_VOP) || \
                                                ((arg&0xFF) == DX3_P_FRM))

#define fgIsBType_MP4(arg)   ((arg&0xFF) == B_VOP || \
                             ((arg&0xFF) == DUMMY_FRM))
#endif

#define fgIsBType_AVC(arg)   (((arg&0xFF) == B_SLICE) || \
                                                ((arg&0xFF) == SP_SLICE) || \
                                                ((arg&0xFF) == B_ALL_SLICE))

#define fgIsBType_WMV(arg)   (((arg&0xFF) == BVOP) || \
                                                 ((arg&0xFF) == BIVOP))


#define fgIsSeqHdr(arg)  ((arg & SEQ_HDR) || \
	                                   (arg & SEQ_PS) || \
	                                   (arg & SUB_SEQ_PS) || \
                                          ((arg&0xFF) == VIS_OBJ) || \
                                          ((arg&0xFF) == VID_OBJ_LAY) || \
                                          ((arg&0xFF) == SH_I_VOP) || \
                                          ((arg&0xFF) == DX3_I_FRM))
#define fgIsGopHdr(arg)  ((arg & GOP_HDR) || \
                                          ((arg&0xFF) == GOVOP))
#define fgIsSeqEnd(arg)  (arg & SEQ_END)
#define fgIsAngleEnd(arg)  (arg & ANGLE_END)

#define fgIsIType(arg)    (((arg&0xFF) == I_TYPE) || \
                                         ((arg&0xFF) == I_VOP) || \
                                         ((arg&0xFF) == SH_I_VOP) || \
                                         ((arg&0xFF) == DX3_I_FRM) || \
                                         (arg & IDR_PIC) || \
                                         ((arg&0xFF) == I_ALL_SLICE) || \
                                         ((arg&0xFF) == I_SLICE) || \
                                         ((arg&0xFF) == SI_SLICE) || \
                                         ((arg&0xFF) == SI_ALL_SLICE) || \
                                         ((arg&0xFF) == IVOP) || \
                                         ((arg&0xFF) == INTRAPIC) || \
                                         ((arg&0xFF) == FORCED_INTRAPIC) || \
                                         ((arg&0xFF) == VP6_I_FRM)   )
                                         
#if 0
#define fgIsPType(arg)   (((arg&0xFF) == P_TYPE) || \
                                         ((arg&0xFF) == P_VOP) || \
                                         ((arg&0xFF) == S_VOP) || \
                                         ((arg&0xFF) == SH_P_VOP) || \
                                         ((arg&0xFF) == DX3_P_FRM) || \
                                         ((arg&0xFF) == DUMMY_TYPE) || \
                                         ((arg&0xFF) == DUMMY_FRM) || \
                                         ((arg&0xFF) == PVOP) || \
                                         ((arg&0xFF) == P_SLICE) || \
                                         ((arg&0xFF) == SP_SLICE) || \
                                         ((arg&0xFF) == P_ALL_SLICE) || \
                                         ((arg&0xFF) == SP_ALL_SLICE) || \
                                         ((arg&0xFF) == SKIPFRAME))
#define fgIsBType(arg)   (((arg&0xFF) == B_TYPE) || \
                                         ((arg&0xFF) == B_VOP) || \
                                         ((arg&0xFF) == BVOP) || \
                                         ((arg&0xFF) == B_SLICE) || \
                                         ((arg&0xFF) == SP_SLICE) || \
                                         ((arg&0xFF) == B_ALL_SLICE) || \
                                         ((arg&0xFF) == BIVOP))
#else
#define fgIsPType(arg)   (((arg&0xFF) == P_TYPE) || \
                                         ((arg&0xFF) == P_VOP) || \
                                         ((arg&0xFF) == S_VOP) || \
                                         ((arg&0xFF) == SH_P_VOP) || \
                                         ((arg&0xFF) == DX3_P_FRM) || \
                                         ((arg&0xFF) == DUMMY_TYPE) || \
                                         ((arg&0xFF) == PVOP) || \
                                         ((arg&0xFF) == P_SLICE) || \
                                         ((arg&0xFF) == SP_SLICE) || \
                                         ((arg&0xFF) == P_ALL_SLICE) || \
                                         ((arg&0xFF) == SP_ALL_SLICE) || \
                                         ((arg&0xFF) == SKIPFRAME) || \
                                         ((arg&0xFF) == INTERPIC) || \
                                         ((arg&0xFF) == VP6_P_FRM)   )
                                         //((arg&0xFF) == SKIPFRAME))

#define fgIsBType(arg)   (((arg&0xFF) == B_TYPE) || \
                                         ((arg&0xFF) == B_VOP) || \
                                         ((arg&0xFF) == DUMMY_FRM) || \
                                         ((arg&0xFF) == BVOP) || \
                                         ((arg&0xFF) == B_SLICE) || \
                                         ((arg&0xFF) == SP_SLICE) || \
                                         ((arg&0xFF) == B_ALL_SLICE) || \
                                         ((arg&0xFF) == BIVOP) || \
                                         ((arg&0xFF) == TRUEBPIC))
                                         //((arg&0xFF) == BIVOP))
#endif

#define fgIsM4vPic(arg)  (((arg&0xFF) & 0x80) > 0)
#define fgIsRefType(arg)  ((arg & REF_PIC) || \
                                       (fgIsPType(arg)) || \
                                       (fgIsIType(arg)))
#define fgIsH264IDRType(arg)  (arg & IDR_PIC)

//For some BDAV, all I frame type are I_SLICE, and which contain sequence header data.
//For fix BDP00049920. Jie Zhang(MTK40414)@20100910
#define fgIsH264IType(arg)  ((arg & IDR_PIC) || ((arg&0xFF) == I_ALL_SLICE) || ((arg&0xFF) == I_SLICE))
#define fgIsH264FileDataIType(arg)  ((arg & IDR_PIC) || ((arg&0xFF) == I_ALL_SLICE) || ((arg&0xFF) == I_SLICE))
#define fgIsH264GopEntry(arg)  (((arg & SEQ_PS) && (fgIsH264IType(arg))) || ((arg & SUB_SEQ_PS) && (arg & ANCHOR_PIC)))
#define fgIsH264FileDataGopEntry(arg)  ((arg & SEQ_PS) && (fgIsH264FileDataIType(arg)))

#define fgIsM4vShort(arg)  (((arg&0xFF) == SH_I_VOP) || \
	                                     ((arg&0xFF) == SH_P_VOP))

#define SetPicType(u4PicType, ePicType)  (u4PicType = (u4PicType & 0xFFFFFF00) | (ePicType))
#define GetPicType(u4PicType)  (u4PicType & 0x000000FF)
#define fgIsStillPic(u4PicType)   (((fgIsSeqHdr(u4PicType) && fgIsGopHdr(u4PicType)) || ((u4PicType & SEQ_PS) && (u4PicType& PIC_PS))) && (fgIsIType(u4PicType) || (u4PicType & ANCHOR_PIC)) && fgIsSeqEnd(u4PicType))

#define fgIsDummyPic(arg)  ((arg&0xFF) == DUMMY_FRM)
/*! @} */


// *********************************************************************
// Picture Structure
// *********************************************************************
/*! \name Picture Structure
* @{
*/
#define TOP_FLD_PIC       1
#define BTM_FLD_PIC       2
#define FRM_PIC           3
// The follow 2 define use in Reference Field Picture
#define TWO_FLDPIC_TOPFIRST  4
#define TWO_FLDPIC_BTMFIRST  5
#define ERR_PIC_STRUCT       0xFF
/*! @} */

// *********************************************************************
// Frame rate code
// *********************************************************************
/*! \name Frame rate code
* @{
*/
// frame_rate_code in Table 6-4 of 13818-2
#define FRC_23_976   1
#define FRC_24           2
#define FRC_25           3
#define FRC_29_97     4
#define FRC_30           5
#define FRC_50           6
#define FRC_59_94     7
#define FRC_60           8
// Reserved by 13818-2 and defined here for other frame rates
#define FRC_1             9
#define FRC_5             10
#define FRC_8             11
#define FRC_10           12
#define FRC_12           13
#define FRC_15           14
#define FRC_16           15
#define FRC_17           16
#define FRC_18           17
#define FRC_20           18
// Reserved by WMV and defined here for other frame rates
#define FRC_2             19
#define FRC_6             20
#define FRC_48           21
#define FRC_70           22
#define FRC_120         23
#define FRC_VAR         24
#define FRC_MAX        (FRC_VAR+1)
/*! @} */

// *********************************************************************
// TV system
// *********************************************************************
/*! \name Tv system
* @{
*/
#define TVS_NTSC       1
#define TVS_PAL         2
/*! @} */

// *********************************************************************
// Aspect ratio
// *********************************************************************
/*! \name Aspect ratio
* @{
*/
#define ASP_UNKNOW            0     // unknow
#define ASP_1_1            1     // SAR, 1:1
#define ASP_4_3            2     // DAR, 4:3 PAN-SCAN (NORMAL)
#define ASP_16_9          3     // DAR, 16:9 FULL
#define ASP_221_1        4     // DAR, 2.21:1
#define ASP_4_3_LB            5     // DAR, 4:3 LB
#define ASP_16_9_NORMAL          6     // DAR, 16:9 NORMAL
#define ASP_UNDEFINED	7
#define ASP_MAX_ASPECT_RATIO	8
/*! @} */


/*! \name Source aspect ratio
* @{
*/
#define ENUM_SRC_ASPECT_RATIO

typedef enum {
    SRC_ASP_UNKNOW = 0,
    SRC_ASP_1_1,
    SRC_ASP_4_3_FULL,
    SRC_ASP_14_9_LB,
    SRC_ASP_14_9_LB_T,
    SRC_ASP_16_9_LB,
    SRC_ASP_16_9_LB_T,
    SRC_ASP_16_9_LB_G,
    SRC_ASP_14_9_FULL,
    SRC_ASP_16_9_FULL,
    SRC_ASP_221_1,
    SRC_ASP_16_9_PS,
    SRC_ASP_UNDEFINED,
    SRC_ASP_CUSTOMIZED,
    SRC_ASP_MAX
} SOURCE_ASPECT_RATIO_T;
/*! @} */

typedef enum {
  HDMI_PICTURE_4_3 = 0,
  HDMI_PICTURE_16_9
} HDMI_PICTURE_ASPECT_RATIO_T;


//the HDMI_AFD_FORMT_T is defined by CEA-861D spec Table 88 AFD coding, kenny 2008/7/1
typedef enum {
  HDMI_BOX_16_9 = 0x02, //box 16:9 (top)
  HDMI_BOX_14_9 = 0x03, //box 14:9 (top)
  HDMI_BOX_OVER_16_9 = 0x04, //box > 16:9 (center)
  HDMI_AS_PICTURE_AR = 0x08, //As the coded frame
  HDMI_4_3_CENTER = 0x09, //4:3 (center)
  HDMI_16_9_CENTER = 0x0a, //16:9 (center)
  HDMI_14_9_CENTER = 0x0b, //14:9 (center)
  HDMI_4_3_WITH_14_9_CENTER = 0x0d, //4:3 (with shoot & protect 14:9 center)
  HDMI_16_9_WITH_14_9_CENTER = 0x0e, //16:9 (with shoot & protect 14:9 center)
  HDMI_4_3_WITH_4_3_CENTER = 0x0f, //16:9 (with shoot & protect 4:3 center)

} HDMI_AFD_FORMT_T;




// Colour Primary
#define COLOR_PRIMARY_709				1
#define COLOR_PRIMARY_601				2


// *********************************************************************
// Pic Flag Info
// *********************************************************************
/*! \name Pic Flag Info
* @{
*/
#define FBG_FLG_ALL                                 (0xFFFFFFFF)
#define FBG_FLG_PROGRESSIVE_SEQ         (0x1 << 0)     ///<  progressive seq
#define FBG_FLG_PROGRESSIVE_FRM         (0x1 << 1)      ///<  progressive frm
#define FBG_FLG_TOP_FLD_FIRST              (0x1 << 2)      ///<  Top field first
#define FBG_FLG_REPEAT_1ST_FLD            (0x1 << 3)      ///<  repeat 1st field
#define FBG_FLG_B_PIC_IN_RA                 (0x1 << 4)      ///<  B pic in Random Access
#define FBG_FLG_1_FLD_PIC                     (0x1 << 5)      ///<  frame buffer constructed by 1 field pic
#define FBG_FLG_2_FLD_PIC                     (0x1 << 6)      ///<  frame buffer constructed by 2 field pic
#define FBG_FLG_NIPB_2_IPB                   (0x1 << 7)      ///<  1 pic decode from !IPB to IPB
#define FBG_FLG_WITH_XVYCC                 (0x1 << 8)      ///<  XVYCC Bitstream
#define FBG_FLG_DRIP_PIC                       (0x1 << 9)      ///<  Drip picture
#define FBG_FLG_OPEN_B                         (0x1 << 10)      ///<  Open B picture
#define FBG_FLG_ADR_SWAP_ON             (0x1 << 11)     ///<  Address swap mode on/off
#define FBG_FLG_RASTER_SCAN_MODE                    (0x1 << 12)     ///<  Address swap mode
#define FBG_FLG_PTS_CHK                       (0x1 << 13)     ///<  PTS needs to check
#define FBG_FLG_FORCE_DISP                (0x1 << 14)     ///<  Force disp
#define FBG_FLG_PTS_RESET                   (0x1 << 15)     ///<  PTS reset
#define FBG_FLG_INTERLACE_FRM          (0x1 << 16)     ///<  Interlaced Frm
#define FBG_FLG_REAL_PROGRESSIVE_FRM     (0x1 << 17)     ///<  BD and Progressive frame
#define FBG_FLG_USE_PTS                        (0x1 << 18)
#define FBG_FLG_NR_PROCESSED                    (0x1 << 19)
#define FBG_FLG_DISPLAY_REF                 (0x1 << 20)
#define FBG_FLG_BD_DISC                         (0x1 << 21)
#define FBG_FLG_AVC_CODEC                    (0x1 << 22)
#define FBG_FLG_DECODING                    (0x1 << 23)
#define FBG_FLG_L_SIGHT                    (0x1 << 24)      ///<  MVC for L sight frame
#define FBG_FLG_R_SIGHT                    (0x1 << 25)      ///<  MVC for R sight frame
#define FBG_FLG_SVCD_DISC                    (0x1 << 26)      ///<  MVC for R sight frame
#define FBG_FLG_LAST_TFX_FRAME                  (0x1 << 27)
#define FBG_FLG_SHARP_PROCESSED              (0x1 << 28)
#define FBG_FLG_BDJ_FRAME              (0x1 << 29)
#define FBG_FLG_DISABLE_VPQSOFTSTATISTICS        (0x1 << 30)    ///< Disable VPQ SoftStatistics Mode for Skype
#define FBG_FLG_STILLPIC                               (0x1 << 31)    ///< Still Picture Flag for Skype Jpeg Content Sharing

/*! @} */

/*! \name Picture Coding Type Marco
* @{
*/
#define fgIsFbFlagSet(arg1, arg2)  (arg1 & arg2)
#define vSetFbFlag(arg1, arg2)  (arg1 |= arg2)
#define vClrFbFlag(arg1, arg2)  (arg1 &= (~arg2))

#define fgIs3DSource(arg)  (arg & (FBG_FLG_L_SIGHT | FBG_FLG_R_SIGHT))

// *********************************************************************
// VDSCL Flag Info
// *********************************************************************
/*! \name Pic Flag Info
* @{
*/
#define VDSCL_FLG_ALL                                 0xFFFFFFFF
#define VDSCL_FLG_NO_VERTICAL_SUGGEST         0x1 << 0     ///<  no vertical support when field pic
/*! @} */

/*! \name Picture Coding Type Marco
* @{
*/
#define fgIsVDSCLFlagSet(arg1, arg2)  (arg1 & arg2)
#define vSetVDSCLFlag(arg1, arg2)  (arg1 |= arg2)
#define vClrVDSCLFlag(arg1, arg2)  (arg1 &= (~arg2))

// *********************************************************************
// Invalid timestamp
// *********************************************************************
/*! \name Invalid timestamp
* @{
*/
#define INVALID_TIMESTAMP   (-1LL)
#define INVALID_DURATION     (-1)
/*! @} */


// *********************************************************************
// Block size
// *********************************************************************
/*! \name Block size
* @{
*/
#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
#define BLOCKSIZE_H					       16
#else
#define BLOCKSIZE_H					       16
#endif
#define BLOCKSIZE_V			   		       32
//#define DRV_ALIGN_MASK(value, mask)			(((value) + (mask - 1)) & (~(mask - 1)))
#define DRV_ALIGN_MASK(value, mask)			((((value) + ((mask) - 1)) / (mask)) * (mask))
/*! @} */

// *********************************************************************
// DSP reserved fifo size
// *********************************************************************
/*! \name DSP reserved fifo size
* @{
*/
#define DSP_RESERVED_AUDIO_MAX_SZ               24576  //24k byte  /*!< (Legacy Playback Special) */
#define DSP_RESERVED_AUDIO_LPCM_SZ              24576  //24k byte
#define DSP_RESERVED_AUDIO_AC_3_SZ              24576  //24k byte
#define DSP_RESERVED_AUDIO_DTS_SZ               24576  //24k byte
#define DSP_RESERVED_AUDIO_DOLBY_LOSSLESS_SZ    24576  //24k byte
#define DSP_RESERVED_AUDIO_DD_PLUS_PRI_SZ       24576  //24k byte
#define DSP_RESERVED_AUDIO_DTS_HD_NO_XLL_SZ     24576  //24k byte
#define DSP_RESERVED_AUDIO_DTS_HD_XLL_SZ        24576  //24k byte
#define DSP_RESERVED_AUDIO_DD_PLUS_SEC_SZ       24576  //24k byte
#define DSP_RESERVED_AUDIO_DTS_HD_LBR_SZ        24576  //24k byte
#define DSP_RESERVED_AUDIO_MPEG_SZ              24576  //24k byte
#define DSP_RESERVED_AUDIO_DOLBY_TRUE_HD_COMPATIBLE_MODE_SZ 24576  //24k byte

// *********************************************************************
// SOURCE TYPE Info
// *********************************************************************
/*! \name SOURCE TYPE Info
* @{
*/

#if CONFIG_DRV_3D_SUPPORT   
typedef  enum
{
 SRC_2D_VIDEO = 0,                     ///< legacy 2D video
 SRC_3D_VIDEO_FA,        ///< alternate frame;
 SRC_3D_VIDEO_TAB,    ///< top and bottom;
 SRC_3D_VIDEO_SBS    ///< side by side; 
} DRV_3D_SOURCE_TYPE_T;
#endif

// *********************************************************************
// xvYCC Info
// *********************************************************************
/*! \name xvYCC Info
* @{
*/
/// xvYCC data
typedef  enum
{
  XVYCC_TYPE_NONE = 0,
  XVYCC_TYPE_AVCHD = 1,
  XVYCC_TYPE_AVCHD_FORCE = 2,
} XVYCC_TYPE_T;

typedef struct
{
    XVYCC_TYPE_T exvYCCType;
    UINT32 u4RedData;
    UINT32 u4GreenData;
    UINT32 u4BlueData;
}XVYCC_INFO_T;



// *********************************************************************
// Disc Type
// *********************************************************************
/*! \name Disc Type
* @{
*/
typedef enum
{
  DT_BD = 0,            ///< BD
  DT_VCD,                ///< VCD / SVCD
  DT_DVD_VIDEO,    ///< DVD video
  DT_DVD_AUDIO,    ///< DVD audio
  DT_DVD_MVR,        ///< DVD -VR
  DT_DVD_PVR,         ///< DVD + VR
  DT_DATADISC,      ///< Data Disc
  DT_NRD,                ///< Netflix
  DT_FLV,                ///< FLV
  DT_SKY,                ///< Skype
#if (1 == CONFIG_DRV_OMX_SUPPORT)
  DT_OMX,            /// < OMX >
#endif
  DT_UNKNOW         ///< unknow
}DiscType;
/*! @} */

//#define fgIsSVCDData(eDiscType, fgMPEG2)  (FALSE)
#define fgIsSVCDData(eDiscType, fgMPEG2)  ((eDiscType == DT_VCD) && fgMPEG2)
#define fgIsBDData(eDiscType)  (eDiscType == DT_BD)
#define fgIsFileData(eDiscType)  (eDiscType == DT_DATADISC)
#define fgIsDVDData(eDiscType)  (eDiscType == DT_DVD_VIDEO || eDiscType == DT_DVD_AUDIO || eDiscType == DT_DVD_MVR || eDiscType == DT_DVD_PVR)
#define fgIsProgressiveBDData(eDiscType, fgProgressFrm)  ((eDiscType == DT_BD) && fgProgressFrm)

#ifdef __cplusplus
}
#endif

#endif //DRV_COMMON_H

