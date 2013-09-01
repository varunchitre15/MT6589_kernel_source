
#ifndef _VDEC_HAL_IF_AVS_H_
#define _VDEC_HAL_IF_AVS_H_

#include "../include/vdec_info_avs.h"
#include "../include/vdec_info_common.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#endif

//#include "vdec_hw_common.h"
//#include "vdec_verify_typedef.h"
//#include "vdec_verify_keydef.h"
//#include "typedef.h"


/*! \name Video Decoder HAL AVS Interface
* @{
*/

/// Initialize video decoder hardware
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_AVS_InitVDecHW(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_AVS_VFIFO_PRM_T *prAVSVDecInitPrm      ///< [IN] Pointer to video fifo address information
);


/// Read Barrel Shifter after shifting
/// \return Value of barrel shifter input window after shifting
UINT32 u4VDEC_HAL_AVS_ShiftGetBitStream(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                  ///< [IN] Shift bits number
);


/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_AVS_GetBitStreamShift(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);


/// Read Barrel Shifter before shifting
/// \return  Most significant (32 - u4ShiftBits) bits of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_AVS_GetRealBitStream(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);

INT32 i4VDEC_HAL_AVS_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_AVS_BS_INIT_PRM_T *prAVSBSInitPrm);

/// Read Barrel Shifter before shifting
/// \return  MSB of barrel shifter input window before shifting
BOOL bVDEC_HAL_AVS_GetBitStreamFlg(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID                                     ///< [IN] Video decoder hardware ID 
);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_AVS_DecStart(
    UINT32 u4BsID,
    UINT32 u4VDecID,
    VDEC_INFO_DEC_PRM_T *prDecPrm              ///< [IN] Pointer to AVS decode Information
);


/// Read current decoded mbx and mby
/// \return None
void vVDEC_HAL_AVS_GetMbxMby(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 *pu4Mbx,                                      ///< [OUT] Pointer to current decoded macroblock in x axis
    UINT32 *pu4Mby                                       ///< [OUT] Pointer to current decoded macroblock in y axis
);

void vVDEC_HAL_AVS_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_AVS_ERR_INFO_T *prAvsErrInfo);
UINT32 u4VDEC_HAL_AVS_GetErrType(UINT32 u4VDecID, VDEC_INFO_AVS_ERR_INFO_T *prAvsErrInfo);

/// Check if all video decoder modules are finish
/// \return TRUE: Finish, FALSE: Not yet
BOOL fgVDEC_HAL_AVS_DecPicComplete(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);



/// Read AVS error message after decoding end
/// \return AVS decode error message
UINT32 u4VDEC_HAL_AVS_GetErrMsg(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Check AVS error info
/// \return AVS decode error check result
BOOL fgVDEC_HAL_AVS_ChkErrInfo(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter ID
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4DecErrInfo,                               ///< [IN] Err Info
    UINT32 u4ECLevel                               ///< [IN] Check the EC level   
);

UINT32 u4VDEC_HAL_AVS_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits);
void vVDEC_HAL_AVS_VDec_PowerDown(UCHAR u4VDecID);

void vVDEC_HAL_AVS_BitStuff_Check(UINT32 u4VDecID, UINT32 u4Val);
UINT32 u4VDEC_AvsUeCodeNum(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_AvsSeCodeNum(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_AVS_Search_SC(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_AVS_Search_SliceSC(UINT32 u4BSID, UINT32 u4VDecID);
void vVDEC_HAL_AVS_HW_Reset(UINT32 u4BSID, UINT32 u4VDecID);
INT32 i4VDEC_HAL_AVS_DecStart(UINT32 u4BsID, UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
INT32 i4VDEC_HAL_AVS_SetBSInfo(UINT32 u4BSID, UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *prDecParam, VDEC_INFO_AVS_BS_INIT_PRM_T rAvsBSInitPrm);
//
/*! @} */

#if (CONFIG_DRV_VERIFY_SUPPORT)
/// Dump AVS video decoder registers, only for verification
/// \return None
void vVDEC_HAL_AVS_VDec_DumpReg(
     UINT32 u4VDecID
);

/// Read AVS video decoder finish register, only for verification
/// \return Register value
UINT32 u4VDEC_HAL_AVS_VDec_ReadFinishFlag(
    UINT32 u4VDecID
);

void u4VDEC_HAL_AVS_VDec_ClearInt(UINT32 u4VDecID);

/// Read AVS video decoder checksum registers, only for verification
/// \return None
void vVDEC_HAL_AVS_VDec_ReadCheckSum(
    UINT32 u4VDecID,
    UINT32 *pu4CheckSum
);


/// Compare decode checksum with golden, only for verification
/// \return True for match, false for mismatch
BOOL fgVDEC_HAL_AVS_VDec_CompCheckSum(
    UINT32 *pu4DecCheckSum,
    UINT32 *pu4GoldenCheckSum
);

#endif

//
/*! @} */


#endif //#ifndef _HAL_VDEC_AVS_IF_H_

