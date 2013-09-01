
#ifndef _VDEC_INFO_H264_H_
#define _VDEC_INFO_H264_H_

//#include "drv_config.h"
//#include "drv_vdec.h"

#include "vdec_info_common.h"
#include "vdec_usage.h"
#define VDEC_MVC_SUPPORT 0

typedef enum 
{
    SEI_BUFFERING_PERIOD = 0,
    SEI_PIC_TIMING,                        // 1
    SEI_PAN_SCAN_RECT,                        // 2
    SEI_FILLER_PAYLOAD,                        // 3
    SEI_USER_DATA_REGISTERED_ITU_T_T35,                        // 4
    SEI_USER_DATA_UNREGISTERED,                        // 5
    SEI_RECOVERY_POINT,                        // 6
    SEI_DEC_REF_PIC_MARKING_REPETITION,                        // 7
    SEI_SPARE_PIC,                        // 8
    SEI_SCENE_INFO,                        // 9
    SEI_SUB_SEQ_INFO,                        // 10
    SEI_SUB_SEQ_LAYER_CHARACTERISTICS,                        // 11
    SEI_SUB_SEQ_CHARACTERISTICS,                        // 12
    SEI_FULL_FRAME_FREEZE,                        // 13
    SEI_FULL_FRAME_FREEZE_RELEASE,                        // 14
    SEI_FULL_FRAME_SNAPSHOT,                        // 15
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,                        // 16
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,                        // 17
    SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,                        // 18
    SEI_FILM_GRAIN_CHARACTERISTICS,                        // 19
    SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,                        // 20
    SEI_STEREO_VIDEO_INFO,                        // 21
    SEI_MVC_SCALABLE_NESTING = 37,                        // 37
    SEI_MAX_ELEMENTS  //!< number of maximum syntax elements 22
} SEI_type;


typedef struct _VDEC_INFO_H264_FGT_PRM_T_
{
    UCHAR    ucMBXSize;
    UCHAR    ucMBYSize;  
    UCHAR    ucDataScr;                                            ///< 0:MC 1:PP
    UCHAR  *pucFGTScrYAddr;
    UCHAR  *pucFGTScrCAddr;
    UCHAR  *pucFGTTrgYAddr;
    UCHAR  *pucFGTTrgCAddr;  
    UINT32   u4Ctrl; 
}VDEC_INFO_H264_FGT_PRM_T;


#define MAXIMUMVALUEOFcpb_cnt   32
typedef struct _VDEC_INFO_H264_HRD_PRM_T_
{
    UINT32  u4CpbCntMinus1;                                                         // ue(v)
    UINT32  u4BitRateScale;                                                           // u(4)
    UINT32  u4CpbSizeScale;                                                         // u(4)
    UINT32  u4BitRateValueMinus1 [MAXIMUMVALUEOFcpb_cnt];     // ue(v)
    UINT32  u4CpbSizeValueMinus1 [MAXIMUMVALUEOFcpb_cnt];    // ue(v)
    BOOL     fgCbrFlag[MAXIMUMVALUEOFcpb_cnt];                         // u(1)
    UINT32  u4InitialCpbRemovalDelayLengthMinus1;                // u(5)
    UINT32  u4CpbRemovalDelayLengthMinus1;                          // u(5)
    UINT32  u4DpbOutputDelayLengthMinus1;                             // u(5)
    UINT32  u4TimeOffsetLength;                                                 // u(5)
}VDEC_INFO_H264_HRD_PRM_T;


typedef struct _VDEC_INFO_H264_VUI_PRM_T_
{
    BOOL     fgAspectRatioInfoPresentFlag;                              // u(1)
    UINT32  u4AspectRatioIdc;                                                     // u(8)
    UINT32  u4SarWidth;                                                                // u(16)
    UINT32  u4SarHeight;                                                              // u(16)
    BOOL     fgOverscanInfoPresentFlag;                                   // u(1)
    BOOL     fgOverscanAppropriateFlag;                                    // u(1)
    BOOL     fgVideoSignalTypePresentFlag;                            // u(1)
    UINT32  u4VideoFormat;                                                         // u(3)
    BOOL     fgVideoFullRangeFlag;                                            // u(1)
    BOOL     fgColourDescriptionPresentFlag;                           // u(1)
    UINT32  u4ColourPrimaries;                                                   // u(8)
    UINT32  u4TransferCharacteristics;                                       // u(8)
    UINT32  u4MatrixCoefficients;                                               // u(8)
    BOOL     fgChromaLocationInfoPresentFlag;                      // u(1)
    UINT32  u4ChromaSampleLocTypeTopField;                    // ue(v)
    UINT32  u4ChromaSampleLocTypeBottomField;             // ue(v)
    BOOL     fgTimingInfoPresentFlag;                                      // u(1)
    UINT32  u4NumUnitsInTick;                                                // u(32)
    UINT32  u4TimeScale;                                                            // u(32)
    BOOL     fgFixedFrameRateFlag;                                          // u(1)
    BOOL     fgNalHrdParametersPresentFlag;                       // u(1)
    VDEC_INFO_H264_HRD_PRM_T tNalHrdParameters;                            // hrd_paramters_t
    BOOL     fgVclHrdParametersPresentFlag;                      // u(1)
    VDEC_INFO_H264_HRD_PRM_T tVclHrdParameters;                            // hrd_paramters_t
    // if ((nal_hrd_parameters_present_flag || (vcl_hrd_parameters_present_flag))
    BOOL     fgLowDelayHrdFlag;                                               // u(1)
    BOOL     fgPicStructPresentFlag;                                        // u(1)
    BOOL     fgBitstreamRestrictionFlag;                                    // u(1)
    BOOL     fgMotionVectorsOverPicBoundariesFlag;        // u(1)
    UINT32  u4MaxBytesPerPicDenom;                                // ue(v)
    UINT32  u4MaxBitsPerMbDenom;                                   // ue(v)
    UINT32  u4Log2MaxMvLengthVertical;                           // ue(v)
    UINT32  u4Log2MaxMvLengthHorizontal;                       // ue(v)
    UINT32  u4NumReorderFrames;                                         // ue(v)
    UINT32  u4MaxDecFrameBuffering;                                // ue(v)
}VDEC_INFO_H264_VUI_PRM_T;

#if VDEC_MVC_SUPPORT
#define MAX_MVC_VIEW_ID 5
#define MAX_MVC_REF_FRM_NUM 5
#define MAX_MVC_APPICABLE_OP_NUM 5 // Spec is 0~1023
typedef struct _VDEC_INFO_MVC_SPS_EXTENSION_T_
{
    UINT16 ucNumViewsMinus1;
    UINT16 aucViewId[MAX_MVC_VIEW_ID];
    UINT16 aucNumAnchorRefsL0[MAX_MVC_VIEW_ID];
    UINT16 aucAnchorRefL0[MAX_MVC_VIEW_ID][MAX_MVC_REF_FRM_NUM];
    UINT16 aucNumAnchorRefsL1[MAX_MVC_VIEW_ID];
    UINT16 aucAnchorRefL1[MAX_MVC_VIEW_ID][MAX_MVC_REF_FRM_NUM];
    UINT16 aucNumNonAnchorRefsL0[MAX_MVC_VIEW_ID];
    UINT16 aucNonAnchorRefL0[MAX_MVC_VIEW_ID][MAX_MVC_REF_FRM_NUM];
    UINT16 aucNumNonAnchorRefsL1[MAX_MVC_VIEW_ID];
    UINT16 aucNonAnchorRefL1[MAX_MVC_VIEW_ID][MAX_MVC_REF_FRM_NUM];
    UINT16 ucNumLevelValuesSignalledMinus1;
    UINT16 aucLevelIdc[MAX_MVC_VIEW_ID];
    UINT16 au2NumApplicableOpsMinus1[MAX_MVC_VIEW_ID];
    UINT16 aucApplicableOpTemporalId[MAX_MVC_VIEW_ID][MAX_MVC_APPICABLE_OP_NUM];
    UINT16 au2ApplicableOpNumTargetViewsMinus1[MAX_MVC_VIEW_ID][MAX_MVC_APPICABLE_OP_NUM];    
    UINT16 au2ApplicableOpTargetViewsId[MAX_MVC_VIEW_ID][MAX_MVC_APPICABLE_OP_NUM][MAX_MVC_VIEW_ID];    
    UINT16 au2ApplicableOpNumViewsMinus1[MAX_MVC_VIEW_ID][MAX_MVC_APPICABLE_OP_NUM];    
}VDEC_INFO_MVC_SPS_EXTENSION_T;

typedef struct _VDEC_INFO_MVC_VUI_EXTENSION_T_
{
    UINT32 u4NumOpsMinus1;
    UCHAR ucTemporalId[MAX_MVC_APPICABLE_OP_NUM];
    UCHAR ucNumTargetOutputViewsMinus1[MAX_MVC_APPICABLE_OP_NUM];
    UCHAR aucViewId[MAX_MVC_APPICABLE_OP_NUM][MAX_MVC_VIEW_ID];
    BOOL fgTimingInfoPresentFlag[MAX_MVC_APPICABLE_OP_NUM];
    UINT32 u4NumUnitsInTick[MAX_MVC_APPICABLE_OP_NUM];
    UINT32 u4TimeScale[MAX_MVC_APPICABLE_OP_NUM];
    BOOL fgFixedFrameRateFlag[MAX_MVC_APPICABLE_OP_NUM];
    BOOL fgNalHrdParametersPresentFlag[MAX_MVC_APPICABLE_OP_NUM];
    BOOL fgVclHrdParametersPresentFlag[MAX_MVC_APPICABLE_OP_NUM];
    BOOL fgLowDelayHrdFlag[MAX_MVC_APPICABLE_OP_NUM];
    BOOL fgPicStructPresetFlag[MAX_MVC_APPICABLE_OP_NUM];
}VDEC_INFO_MVC_VUI_EXTENSION_T;
#endif

#define MAXnum_ref_frames_in_pic_order_cnt_cycle  256
typedef struct _VDEC_INFO_H264_SPS_T_ 
{
    BOOL     fgSPSValid;                  // indicates the parameter set is valid
    UINT32  u4ProfileIdc;                                                          // u(8)
    BOOL     fgConstrainedSet0Flag;                                        // u(1)
    BOOL     fgConstrainedSet1Flag;                                        // u(1)
    BOOL     fgConstrainedSet2Flag;                                        // u(1)
    BOOL     fgConstrainedSet3Flag;                                        // u(1)
    BOOL     fgConstrainedSet4Flag;                                        // u(1)
    UINT32  u4LevelIdc;                                                            // u(8)
    UINT32  u4SeqParameterSetId;                                      // ue(v)
    UINT32  u4ChromaFormatIdc;                                           // ue(v)
    BOOL     fgResidualColorTransformFlag;
    UINT32  u4BitDepthLumaMinus8;                                  // ue(v)
    UINT32  u4BitDepthChromaMinus8;                              // ue(v)
    BOOL     fgQpprimeYZeroTransformBypassFlag;
  
    BOOL    fgSeqScalingMatrixPresentFlag;                     // u(1)
    BOOL    fgSeqScalingListPresentFlag[8];                      // u(1)
    CHAR    cScalingList4x4[6][16];                                        // se(v)
    CHAR    cScalingList8x8[2][64];                                        // se(v)
    BOOL    fgUseDefaultScalingMatrix4x4Flag[6];
    BOOL    fgUseDefaultScalingMatrix8x8Flag[2];
  
    UINT32  u4Log2MaxFrameNumMinus4;                       // ue(v)
    UINT32  u4MaxFrameNum;
    UINT32  u4PicOrderCntType;
    UINT32  u4Log2MaxPicOrderCntLsbMinus4;              // ue(v)
    BOOL     fgDeltaPicOrderAlwaysZeroFlag;                   // u(1)
    INT32    i4OffsetForNonRefPic;                                     // se(v)
    INT32    i4OffsetForTopToBottomField;                       // se(v)
    UINT32  u4NumRefFramesInPicOrderCntCycle;       // ue(v)
    INT32    i4OffsetForRefFrame[MAXnum_ref_frames_in_pic_order_cnt_cycle];   // se(v)
    UINT32  u4NumRefFrames;                                              // ue(v)
    BOOL     fgGapsInFrameNumValueAllowedFlag;      // u(1)
    UINT32  u4PicWidthInMbsMinus1;                              // ue(v)
    UINT32  u4PicHeightInMapUnitsMinus1;                  // ue(v)
    BOOL     fgFrameMbsOnlyFlag;                                     // u(1)
    BOOL     fgMbAdaptiveFrameFieldFlag;                      // u(1)
    BOOL     fgDirect8x8InferenceFlag;                             // u(1)
    BOOL     fgFrameCroppingFlag;                                      // u(1)
    UINT32  u4FrameCropLeftOffset;                                  // ue(v)
    UINT32  u4FrameCropRightOffset;                                // ue(v)
    UINT32  u4FrameCropTopOffset;                                  // ue(v)
    UINT32  u4FrameCropBottomOffset;                            // ue(v)
    UINT32  u4CropUnitX;                                                // 1 or 2
    UINT32  u4CropUnitY;                                                // 1 or 2 or 4
    BOOL     fgVuiParametersPresentFlag;                       // u(1)
    VDEC_INFO_H264_VUI_PRM_T rVUI;                // vui_seq_parameters_t
#if VDEC_MVC_SUPPORT
    BOOL     fgBitEqualToOne;                       // u(1)
    VDEC_INFO_MVC_SPS_EXTENSION_T rMvcSPS;
    BOOL     fgMvcVuiParametersPresentFlag;                       // u(1)
    VDEC_INFO_MVC_VUI_EXTENSION_T rMvcVUI;
#endif    
}VDEC_INFO_H264_SPS_T;


#define MAXnum_slice_groups_minus1  8
typedef struct _VDEC_INFO_H264_PPS_T_
{
    BOOL     fgPPSValid;                  // indicates the parameter set is valid
    //UINT32   pic_parameter_set_id;                                     // ue(v)
    UINT32  u4SeqParameterSetId;                                  // ue(v)
    BOOL     fgEntropyCodingModeFlag;                            // u(1)
  
    BOOL     fgTransform8x8ModeFlag;                             // u(1)
  
    BOOL     fgPicScalingMatrixPresentFlag;                   // u(1)
    BOOL     fgPicScalingListPresentFlag[8];                   // u(1)
    CHAR     cScalingList4x4[6][16];                                     // se(v)
    CHAR     cScalingList8x8[2][64];                                     // se(v)
    BOOL     fgUseDefaultScalingMatrix4x4Flag[6];
    BOOL     fgUseDefaultScalingMatrix8x8Flag[2];
  
    // if( pic_order_cnt_type < 2 )  in the sequence parameter set
    BOOL     fgPicOrderPresentFlag;                                   // u(1)
    UINT32  u4NumSliceGroupsMinus1;                            // ue(v)
    UINT32  u4SliceGroupMapType;                                  // ue(v)
    // if( slice_group_map_type = = 0 )
    UINT32  u4RunLengthMinus1[MAXnum_slice_groups_minus1]; // ue(v)
    // else if( slice_group_map_type = = 2 )
    UINT32  u4TopLeft[MAXnum_slice_groups_minus1];              // ue(v)
    UINT32  u4BottomRight[MAXnum_slice_groups_minus1];     // ue(v)
    // else if( slice_group_map_type = = 3 || 4 || 5
    BOOL     fgSliceGroupChangeDirectionFlag;               // u(1)
    UINT32  u4SliceGroupChangeRateMinus1;                 // ue(v)
    // else if( slice_group_map_type = = 6 )
    UINT32  u4PicSizeInMapUnitsMinus1;                     // ue(v)
    UINT32  *pu4SliceGroupId;                                              // complete MBAmap u(v)
    UINT32  u4NumRefIdxL0ActiveMinus1;                    // ue(v)
    UINT32  u4NumRefIdxL1ActiveMinus1;                     // ue(v)
    BOOL     fgWeightedPredFlag;                                         // u(1)
    UINT32  u4WeightedBipredIdc;                                      // u(2)
    INT32    i4PicInitQpMinus26;                                      // se(v)
    INT32    i4PicInitQsMinus26;                                      // se(v)
    INT32    i4ChromaQpIndexOffset;                               // se(v)
  
    INT32    i4SecondChromaQpIndexOffset;                  // se(v)
  
    BOOL     fgDeblockingFilterControlPresentFlag;         // u(1)
    BOOL     fgConstrainedIntraPredFlag;                          // u(1)
    BOOL     fgRedundantPicCntPresentFlag;                   // u(1)
} VDEC_INFO_H264_PPS_T;


typedef struct _VDEC_INFO_H264_SLICE_HDR_T_
{
    UINT32  u4FirstMbInSlice;
    UINT32  u4SliceType;
    UINT32  u4PPSID;    
    UINT32  u4FrameNum;
    BOOL     fgFieldPicFlag;
    BOOL     fgBottomFieldFlag;
  
    UINT32  u4IdrPicId;
    INT32    i4PicOrderCntLsb;
    INT32    i4PicOrderCntMsb;
    INT32    i4DeltaPicOrderCntBottom;
    INT32    i4DeltaPicOrderCnt[2];
    UINT32  u4RedundantPicCnt;
    BOOL     fgDirectSpatialMvPredFlag;
    BOOL     fgNumRefIdxActiveOverrideFlag;
    UINT32  u4NumRefIdxL0ActiveMinus1;
    UINT32  u4NumRefIdxL1ActiveMinus1;	
  
    BOOL     fgRefPicListReorderingFlagL0;
    BOOL     fgRefPicListReorderingFlagL1;
    UINT32  u4ReorderingOfPicNumsIdc;
    UINT32  u4AbsDiffPicNumMinus1;
    UINT32  u4LongTermPicNum;
  
    UINT32  u4CabacInitIdc;
    INT32    i4SliceQpDelta;
  
    UINT32  u4LumaLog2WeightDenom;
    UINT32  u4ChromaLog2WeightDenom;
    BOOL     fgLumaWeightL0Flag;
    BOOL     fgLumaWeightL1Flag;
    BOOL     fgChromaWeightL0Flag;
    BOOL     fgChromaWeightL1Flag;
    UINT32  u4LumaWeightL0[32];
    UINT32  u4LumaOffsetl0[32];
    UINT32  u4ChromaWeightL0[32][2];
    UINT32  u4ChromaOffsetL0[32][2];
    UINT32  u4LumaWeightL1[32];
    UINT32  u4LumaOffsetL1[32];
    UINT32  u4ChromaWeightL1[32][2];
    UINT32  u4ChromaOffsetL1[32][2];
  
    BOOL     fgNoOutputOfPriorPicsFlag;
    BOOL     fgLongTermReferenceFlag;
    BOOL     fgAdaptiveRefPicMarkingModeFlag;
  
    UINT32  u4MemoryManagementControlOperation[32];
    UINT32  u4DifferencOfPicNumsMinus1;
    UINT32  u4LongTermFrameIdx;
    UINT32  u4MaxLongTermFrameIdxPlus1;
  
    BOOL     fgSpForSwitchFlag;
    INT32    i4SliceQsDelta;
    UINT32  u4DisableDeblockingFilterIdc;
    INT32    i4SliceAlphaC0OffsetDiv2;
    INT32    i4SliceBetaOffsetDiv2;
    UINT32  u4SliceGroupChangeCycle;
  
     BOOL    fgMmco5;
}VDEC_INFO_H264_SLICE_HDR_T;

typedef struct _VDEC_INFO_SEI_REC_POINT_T_
{
    UINT32  u4RecoveryFrameCnt;
    BOOL    fgExactMatchFlag;
    BOOL    fgBrokenLinkFlag;    
    UCHAR  ucChangingSliceGroupIdc;
}VDEC_INFO_SEI_REC_POINT_T;

typedef enum
{
    H264_FRAME = 0,   
    H264_TOP_FLD = 1,
    H264_BOT_FLD = 2,
    H264_TWO_FLD_TOP_FLD_1ST = 3,
    H264_TWO_FLD_BOT_FLD_1ST = 4,
    H264_THREE_FLD_TOP_FLD_REP = 5,
    H264_THREE_FLD_BOT_FLD_REP = 6,
    H264_FRAME_DOUBLING = 7,       
    H264_FRAME_TRIPLING = 8,           
}H264_PiC_STRUCT_T;

typedef struct _VDEC_INFO_SEI_PIC_TIMING_T_
{
    UINT32  u4CpbRemovalDelay;
    UINT32  u4DpbOutputDelay;    
    UINT32  u4PicStruct;
    BOOL    fgClockTimeStampFlag[3];
    UINT32  u4CtType;
    BOOL    fgNuitFieldBasedFlag;    
    UINT32  u4CountingType;    
    BOOL    fgFullTimestampFlag;    
    BOOL    fgDiscontinuityFlag;    
    BOOL    fgCntDroppedFlag;        
    UINT32  u4NFrames;    
    BOOL    fgSecondsFlag;        
    UINT32  u4SecondsValue;    
    BOOL    fgMinutesFlag;        
    UINT32  u4MinutesValue;        
    BOOL    fgHoursFlag;        
    UINT32  u4HoursValue;        
    UINT32  u4TimeOffset;        
}VDEC_INFO_SEI_PIC_TIMING_T;

typedef struct _VDEC_INFO_SEI_XVYCC_T_
{
    UINT32  u4RedData;
    UINT32  u4GreenData;
    UINT32  u4BlueData;    
}VDEC_INFO_SEI_XVYCC_T;

typedef struct _VDEC_INFO_H264_SEI_T_
{
    VDEC_INFO_SEI_REC_POINT_T    rRecoverPointInfo;
    VDEC_INFO_SEI_PIC_TIMING_T    rPicTimingInfo;
    VDEC_INFO_SEI_XVYCC_T    rxvYCCInfo;
    BOOL     fgFilmGrainCharacteristicsCancelFlag;
    UINT32  u4ModelId;
    BOOL     fgSeparateColourDescriptionPresentFlag;
    UINT32  u4FilmGrainBitDepthLumaMinus8;
    UINT32  u4FilmGrainBitDepthChromaMinus8;  
    BOOL     fgFilmGrainFullRangeFlag;
    UINT32  u4FilmGrainColourPrimaries;
    UINT32  u4FilmGrainTransferCharacteristics;
    UINT32  u4FilmGrainMatrixCoefficients;
    UINT32  u4BlendingModeId;
    UINT32  u4Log2ScaleFactor;
    BOOL     fgCompModelPresentFlag[3];
    UINT32  u4NumIntensityIntervalsMinus1[3];
    UINT32  u4NumModelValuesMinus1[3];
    UINT32  u4IntensityIntervalLowerBound[3][256];
    UINT32  u4IntensityIntervalUpperBound[3][256];
    UCHAR   *pucCompModelValue;
    UINT32  u4FilmGrainCharacteristicsRepetitionPeriod;  
}VDEC_INFO_H264_SEI_T;

#if VDEC_MVC_SUPPORT
typedef struct _VDEC_INFO_MVC_EXTENSION_T_
{
    BOOL     fgSvcExtensionFlag;
#if MVC_PATCH_1
    BOOL     fgIdrFlag;
#else
    BOOL     fgNonIdrFlag;
#endif
    UCHAR  ucPriorityId;
    UINT32  u4ViewId;    
    UCHAR  ucTemporalId;        
    BOOL     fgAnchorPicFlag;
    BOOL     fgInterViewFlag;
    BOOL     fgReservedOneBit;
}VDEC_INFO_MVC_EXTENSION_T;
#endif

typedef struct _VDEC_INFO_H264_LAST_INFO_T_
{
    BOOL    fgLastMmco5;
    UCHAR ucLastNalUnitType;
    UCHAR  ucLastPicStruct;
    UCHAR  ucLastSPSId;
    UCHAR  ucLastSPSLevel;
    INT32   i4LastPOC;  
    INT32   i4LastTFldPOC;
    INT32   i4LastBFldPOC;
    INT32   i4LastRefPOC;  
    INT32   i4LastRefTFldPOC;
    INT32   i4LastRefBFldPOC;
    INT32   i4LastRefPOCMsb;
    INT32   i4LastRefPOCLsb;  
    INT32   i4LastFrameNumOffset;  
    UINT32 u4LastFrameNum; 
    UINT32 u4LastPicW;
    UINT32 u4LastPicH;  
#if VDEC_MVC_SUPPORT
    UCHAR  ucLastDpbId;
    UINT32  u4LastViewId;    
    BOOL     fgLastAnchorPicFlag;
    BOOL     fgLastInterViewFlag;
#endif    
}VDEC_INFO_H264_LAST_INFO_T;

typedef enum
{
    H264_DPB_STATUS_EMPTY = 0,   // Free
    H264_DPB_STATUS_READY,         // After Get          
    H264_DPB_STATUS_DECODING,   // After Lock                
    H264_DPB_STATUS_DECODED,     // After UnLock
    H264_DPB_STATUS_OUTPUTTED,     // After Output
    H264_DPB_STATUS_FLD_DECODED,   // After 1fld UnLock
    H264_DPB_STATUS_DEC_REF,     // LOCK for decoded but ref needed
    H264_DPB_STATUS_FLD_DEC_REF,     // LOCK for decoded but ref needed
    H264_DPB_STATUS_OUT_REF,     // LOCK for outputted but ref needed
#ifdef DRV_VDEC_VDP_RACING
    H264_DPB_STATUS_OUT_DECODING,   // After Lock
    H264_DPB_STATUS_OUT_FLD_DEC,
#endif
}H264_DPB_COND_T;

typedef struct _VDEC_INFO_H264_FBUF_INFO_T_
{ 
    BOOL     fgNonExisting;
    
    H264_DPB_COND_T eH264DpbStatus;
    
    UCHAR   ucFBufStatus;  // 1:Top decoded, 2: Bottom decoded
    UCHAR   ucFBufRefType;
    UCHAR   ucTFldRefType;
    UCHAR   ucBFldRefType;
  
    UCHAR   ucFbmFbId;
  
    UINT32  u4FrameNum;
    INT32    i4FrameNumWrap;
    
    INT32    i4PicNum;
    INT32    i4TFldPicNum;
    INT32    i4BFldPicNum;  
    INT32    i4LongTermPicNum;
    INT32    i4TFldLongTermPicNum;
    INT32    i4BFldLongTermPicNum;
    
    UINT32  u4YStartAddr;
    UINT32  u4CAddrOffset;
    
    UCHAR   ucMvFbId;
    UINT32  u4MvStartAddr;
    
    INT32    i4POC;
    INT32    i4TFldPOC;
    INT32    i4BFldPOC;
  
    UINT32  u4TFldPara;
    UINT32  u4BFldPara;   // record for MV
                                 // bit 0: field_pic_flag
                                 // bit 1: mbaff_flag
    UINT32  u4FrameCnt;
                                 
    BOOL fgVirtualDec;                                 
    UINT64  u8Pts;
    UINT64  u8Offset;    

    UCHAR  ucH264DFBListIdx;

#if VDEC_MVC_SUPPORT
    UINT32  u4ViewId;
#endif

    #if 1//(CONFIG_DRV_VERIFY_SUPPORT)
    UINT32  u4W;
    UINT32  u4H;  
    UINT32  u4DecOrder;
    UINT32  u4DramPicSize;  // change name to u4CAddrOffset and add u4MVStartAddr TODO:071021
    UINT32  u4DramPicArea; // maybe will be removed TODO:071021
    UINT32  u4Addr;  // change name to u4YStartAddr   TODO:071021

    #else
    #endif

    UINT32   u4LongTermFrameIdx;
    UINT32   u4TFldLongTermFrameIdx;
    UINT32   u4BFldLongTermFrameIdx;
    
    //UCHAR   ucLongTermFrameIdx;
    //UCHAR   ucTFldLongTermFrameIdx;
    //UCHAR   ucBFldLongTermFrameIdx;

}VDEC_INFO_H264_FBUF_INFO_T;

#define H264_MAX_PIC_LIST_NUM 32
typedef struct _VDEC_INFO_H264_REF_PIC_LIST_T_
{  
    UINT32  u4RefPicCnt;
    UCHAR   ucPicType[H264_MAX_PIC_LIST_NUM];
    UCHAR   ucRefType[H264_MAX_PIC_LIST_NUM];
    UINT32  u4FBufIdx[H264_MAX_PIC_LIST_NUM];  
}VDEC_INFO_H264_REF_PIC_LIST_T;


typedef struct _VDEC_INFO_H264_BS_INIT_PRM_T_
{
    UINT32  u4VLDRdPtr;
    UINT32  u4VLDWrPtr;
    UINT32  u4VFifoSa;                 ///< Video Fifo memory start address    
    UINT32  u4VFifoEa;                 ///< Video Fifo memory end address
    UINT32  u4PredSa;
}VDEC_INFO_H264_BS_INIT_PRM_T;


typedef struct _VDEC_INFO_H264_INIT_PRM_T_
{
    UINT32  u4FGSeedbase;
    UINT32  u4CompModelValue; 
    UINT32  u4FGDatabase;
}VDEC_INFO_H264_INIT_PRM_T;


typedef struct _VDEC_INFO_H264_P_REF_PRM_T_
{
    UCHAR   ucFBufIdx;
    UINT32  u4FBufInfo;
    UINT32  u4ListIdx;
    UINT32  u4FBufYStartAddr;
    UINT32  u4FBufCAddrOffset;
    UINT32  u4FBufMvStartAddr;
    INT32    i4TFldPOC;
    INT32    i4BFldPOC;
    UINT32  u4TFldPara;
    UINT32  u4BFldPara;
    INT32    i4TFldLongTermPicNum;
    INT32    i4BFldLongTermPicNum;
    INT32    i4LongTermPicNum;
    INT32    i4PicNum;
    INT32    i4TFldPicNum;
    INT32    i4BFldPicNum;

#if VDEC_MVC_SUPPORT
    UINT32  u4ViewId;
#endif

    #if (CONFIG_DRV_VERIFY_SUPPORT)
    //UINT32  u4FBufAddr;
    #endif
}VDEC_INFO_H264_P_REF_PRM_T;


typedef struct _VDEC_INFO_H264_POC_PRM_T_
{
    UCHAR   ucPicStruct;
    BOOL     fgIsFrmPic;
    INT32    i4POC;
    INT32    i4TFldPOC;
    INT32    i4BFldPOC;
}VDEC_INFO_H264_POC_PRM_T;


typedef struct _VDEC_INFO_H264_B_REF_PRM_T_
{
    UCHAR   ucFBufIdx;
    UINT32  u4FBufInfo;
    UINT32  u4ListIdx;
    UINT32  u4FBufYStartAddr; 
    UINT32  u4FBufCAddrOffset;
    UINT32  u4FBufMvStartAddr;
    INT32    i4TFldPOC;
    INT32    i4BFldPOC;
    UINT32  u4TFldPara;
    UINT32  u4BFldPara;
    INT32    i4TFldLongTermPicNum;
    INT32    i4BFldLongTermPicNum;
    INT32    i4LongTermPicNum;
    INT32    i4PicNum;
    INT32    i4TFldPicNum;
    INT32    i4BFldPicNum;
    UCHAR  ucFBufIdx1;
    UINT32  u4ListIdx1;
    UINT32  u4FBufYStartAddr1;
    UINT32  u4FBufCAddrOffset1;
    UINT32  u4FBufMvStartAddr1;
    INT32    i4LongTermPicNum1;
    INT32    i4PicNum1;
    INT32    i4TFldPOC1;
    INT32    i4BFldPOC1;
    UINT32  u4TFldPara1;
    UINT32  u4BFldPara1;

#if VDEC_MVC_SUPPORT
    UINT32  u4ViewId;
#endif

    #if(CONFIG_DRV_VERIFY_SUPPORT)
    //UINT32  bFBufIdx1;
    //UINT32  u4FBufAddr1;
    //UINT32  u4FBufAddr; // change name to u4FBufYStartAddr   TODO:071021
    //UINT32  u4DramPicSize;  // change name to u4FBufCAddrOffset and add u4FBufMVStartAddr   TODO:071021
    //UINT32  u4DramPicSize1;

    #endif
}VDEC_INFO_H264_B_REF_PRM_T;


typedef struct _VDEC_INFO_H264_DEC_PRM_T_
{    
    UCHAR   ucNalRefIdc;
    UCHAR   ucNalUnitType;
    BOOL     fgIsIDRPic;
    BOOL    fgIsRecoveryPoint;    
    BOOL     fgIsFrmPic;
    UINT32   u4MaxLongTermFrameIdx;
    //UCHAR bMaxUsedFBufNum;
    UCHAR   ucMaxFBufNum;
    UINT32  u4RealPicH;
    // Decode picture setting
    INT32    i4FrmNumOffset;
    UCHAR ucECLevel;
    BOOL     fgUserScalingMatrixPresentFlag;                   // u(1)
    BOOL     fgUserScalingListPresentFlag[8];                   // u(1)
    
    VDEC_INFO_H264_SPS_T *prSPS;
    VDEC_INFO_H264_PPS_T *prPPS;
    VDEC_INFO_H264_SLICE_HDR_T *prSliceHdr;  
    VDEC_INFO_H264_SEI_T *prSEI;
#if VDEC_MVC_SUPPORT
    VDEC_INFO_MVC_EXTENSION_T rMvcExtInfo;
#endif
    VDEC_INFO_H264_FGT_PRM_T *prFGTPrm;
    VDEC_INFO_H264_LAST_INFO_T rLastInfo;

    VDEC_INFO_H264_FBUF_INFO_T *prCurrFBufInfo;

    #if(1)
    BOOL  fgIsReduceMVBuffer;
    #endif
    BOOL  fgIsAllegMvcCfg;
    
    #if 1//AVC_8320_SUPPORT
    
    UINT32 u4VLDWrapperWrok;
    UINT32 u4PPWrapperWrok;
    #endif
}VDEC_INFO_H264_DEC_PRM_T;



#endif //#ifndef _HAL_VDEC_H264_IF_H_

