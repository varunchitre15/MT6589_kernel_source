
#ifndef _VDEC_HAL_IF_H264_H_
#define _VDEC_HAL_IF_H264_H_

#include "../include/vdec_info_h264.h"
#include "../include/vdec_info_common.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#endif

//#include "vdec_hw_common.h"
//#include "vdec_verify_typedef.h"
//#include "vdec_verify_keydef.h"
//#include "typedef.h"


/*! \name Video Decoder HAL H264 Interface
* @{
*/

/// Initialize video decoder hardware
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_H264_InitVDecHW(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_INIT_PRM_T *prH264VDecInitPrm      ///< [IN] Pointer to video fifo address information
);


/// Read Barrel Shifter after shifting
/// \return Value of barrel shifter input window after shifting
UINT32 u4VDEC_HAL_H264_ShiftGetBitStream(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                  ///< [IN] Shift bits number
);


/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_H264_GetBitStreamShift(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);


/// Read Barrel Shifter before shifting
/// \return  Most significant (32 - u4ShiftBits) bits of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_H264_GetRealBitStream(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);


/// Read Barrel Shifter before shifting
/// \return  MSB of barrel shifter input window before shifting
BOOL bVDEC_HAL_H264_GetBitStreamFlg(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID                                     ///< [IN] Video decoder hardware ID 
);


/// Do UE variable length decoding
/// \return  Input window after UE variable length decoding
UINT32 u4VDEC_HAL_H264_UeCodeNum(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID                                     ///< [IN] Video decoder hardware ID 
);


/// Do SE variable length decoding
/// \return  Input window after SE variable length decoding
INT32 i4VDEC_HAL_H264_SeCodeNum(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID                                     ///< [IN] Video decoder hardware ID 
);


/// Get next start code
/// \return Current input window of vld while finding start code 
UINT32 u4VDEC_HAL_H264_GetStartCode(
    UINT32 u4BSID,                                     ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID 
); 

/// Get next start code
/// \return Current input window of vld while finding start code 
UINT32 u4VDEC_HAL_H264_GetStartCode_8530(
    UINT32 u4BSID,                                     ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID 
); 


/// Initialize barrel shifter with byte alignment
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_H264_InitBarrelShifter(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                     ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm
);


/// Read current read pointer
/// \return Current read pointer with byte alignment
UINT32 u4VDEC_HAL_H264_ReadRdPtr(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4VFIFOSa,
    UINT32 *pu4Bits                                     ///< [OUT] Read pointer with remained bits
);


/// Align read pointer to byte,word or double word
/// \return None
void vVDEC_HAL_H264_AlignRdPtr(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4AlignType                                  ///< [IN] Align type
);


/// Read barrel shifter bitcount after initializing 
/// \return Bitcount counted by HAL
UINT32 u4VDEC_HAL_H264_GetBitcount(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter ID
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Decode scaling list
/// \return None
void vVDEC_HAL_H264_ScalingList(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter ID
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    CHAR *pcScalingList,                                  ///< [OUT] Pointer to value of scaling list
    UINT32 u4SizeOfScalingList,                      ///< [IN] Size of scaling list
    BOOL *pfgUseDefaultScalingMatrixFlag        ///< [OUT] Pointer to flag to use default scaling list or not
);


/// Write scaling list to HW
/// \return None
void vVDEC_HAL_H264_WriteScalingList(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4Idx,                                          ///< [IN] Index of scaling list
    CHAR *pcSlicePtr                                      ///<[IN] Pointer to list data
);


/// Reference list reordering
/// \return None
void vVDEC_HAL_H264_Reording(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Decode prediction weighting table
/// \return None
void vVDEC_HAL_H264_PredWeightTable(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Remove traling bits to byte align
/// \return None
void vVDEC_HAL_H264_TrailingBits(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter ID
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Check whether there is more rbsp data
/// \return Is morw Rbsp data or not
BOOL bVDEC_HAL_H264_IsMoreRbspData(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter ID
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Set HW registers to initialize P reference list
/// \return None
void vVDEC_HAL_H264_InitPRefList(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    BOOL fgPicFrm,
    UINT32 u4MaxFrameNum,                         ///< [IN] Maximium frame number
    UINT32 u4CurrPicNum
);


/// Set HW registers related with P reference list
/// \return None
void vVDEC_HAL_H264_SetPRefPicListReg(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo     ///< [IN] Pointer to struct of P reference picutre list information
);


/// Set POC number to HW registers
/// \return None
void vVDEC_HAL_H264_SetPOC(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_POC_PRM_T *prPOCInfo     ///< [IN] Pointer to struct of POC information
);


/// Set HW registers to initialize B reference list
/// \return None
void vVDEC_HAL_H264_InitBRefList(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Set HW registers related with B reference list
/// \return B0 list and B1 list are equal or not
BOOL bVDEC_HAL_H264_SetBRefPicListReg(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_B_REF_PRM_T *prBRefPicListInfo     ///< [IN] Pointer to struct of B reference picutre list information
);


/// Swap B1 reference list1 
/// \return None
void vVDEC_HAL_H264_B1ListSwap(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    BOOL fgIsFrmPic                                      ///< [IN] Flag of frame picture or not
);


/// Set SPS data to HW
/// \return None
void vVDEC_HAL_H264_SetSPSAVLD(
    UINT32 u4VDecID,                                   ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_SPS_T *prSPS           ///< [IN] Pointer to struct of sequence parameter set
);


/// Set PPS data to HW
/// \return None
void vVDEC_HAL_H264_SetPPSAVLD(
    UINT32 u4VDecID,                                   ///< [IN] Video decoder hardware ID
    BOOL fgUserScalingMatrixPresentFlag,
    BOOL *pfgUserScalingListPresentFlag,
    VDEC_INFO_H264_PPS_T *prPPS           ///< [IN] Pointer to struct of picutre parameter set
);


/// Set part of slice header data to HW
/// \return None
void vVDEC_HAL_H264_SetSHDRAVLD1(
    UINT32 u4VDecID,                                              ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_SLICE_HDR_T *prSliceHdr        ///< [IN] Pointer to struct of picutre parameter set
);


/// Set film gram hardware registers
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_H264_FGTSetting(
    UINT32 u4VDecID,                                                            ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_FGT_PRM_T *prFGTPrm            ///< [IN] Pointer to H264 film gram Information
);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_H264_DecStart(
    UINT32 u4VDecID,
    VDEC_INFO_DEC_PRM_T *prDecPrm              ///< [IN] Pointer to H264 decode Information
);


/// Read current decoded mbx and mby
/// \return None
void vVDEC_HAL_H264_GetMbxMby(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 *pu4Mbx,                                      ///< [OUT] Pointer to current decoded macroblock in x axis
    UINT32 *pu4Mby                                       ///< [OUT] Pointer to current decoded macroblock in y axis
);


/// Check if all video decoder modules are finish
/// \return TRUE: Finish, FALSE: Not yet
BOOL fgVDEC_HAL_H264_DecPicComplete(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);



/// Read h264 error message after decoding end
/// \return h264 decode error message
UINT32 u4VDEC_HAL_H264_GetErrMsg(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Check h264 error info
/// \return h264 decode error check result
BOOL fgVDEC_HAL_H264_ChkErrInfo(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter ID
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4DecErrInfo,                               ///< [IN] Err Info
    UINT32 u4ECLevel                               ///< [IN] Check the EC level   
);

#if VDEC_MVC_SUPPORT
/// Set if VDec HW with MVC mode
/// \return None
void vVDEC_HAL_H264_MVC_Switch(
    UINT32 u4VDecID,                                        ///< [IN] Video decoder hardware ID
    BOOL fgIsMVCDec                                   ///< [IN] enable/disable MVC mode  
);


/// Set HW registers related with InterVire reference list
/// \return None
void vVDEC_HAL_H264_SetInterViewPRefPicListReg(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo     ///< [IN] Pointer to struct of P reference picutre list information
);

/// Set HW registers related with InterVire reference list
/// \return None
void vVDEC_HAL_H264_SetInterViewB0RefPicListReg(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo     ///< [IN] Pointer to struct of P reference picutre list information
);

/// Set HW registers related with InterVire reference list
/// \return None
void vVDEC_HAL_H264_SetInterViewB1RefPicListReg(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_H264_P_REF_PRM_T *prPRefPicListInfo     ///< [IN] Pointer to struct of P reference picutre list information
);

#endif

void vVDEC_HAL_H264_VDec_PowerDown(UCHAR u4VDecID);

#ifdef MPV_DUMP_H264_CHKSUM
/// Compare decode checksum with golden, only for verification
/// \return True for match, false for mismatch
void vVDEC_HAL_H264_VDec_ReadCheckSum1(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);
/// Compare decode checksum with golden, only for verification
/// \return True for match, false for mismatch
void vVDEC_HAL_H264_VDec_ReadCheckSum2(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);
#endif

//
/*! @} */

#if (CONFIG_DRV_VERIFY_SUPPORT)
/// Dump H264 video decoder registers, only for verification
/// \return None
void vVDEC_HAL_H264_VDec_DumpReg(
     UINT32 u4VDecID
);

/// Read H264 video decoder finish register, only for verification
/// \return Register value
UINT32 u4VDEC_HAL_H264_VDec_ReadFinishFlag(
    UINT32 u4VDecID
);


/// Read H264 video decoder checksum registers, only for verification
/// \return None
void vVDEC_HAL_H264_VDec_ReadCheckSum(
    UINT32 u4VDecID,
    UINT32 *pu4CheckSum
);


/// Compare decode checksum with golden, only for verification
/// \return True for match, false for mismatch
BOOL fgVDEC_HAL_H264_VDec_CompCheckSum(
    UINT32 *pu4DecCheckSum,
    UINT32 *pu4GoldenCheckSum
);

#endif

//
/*! @} */


#endif //#ifndef _HAL_VDEC_H264_IF_H_

