
#ifndef _VDEC_HAL_IF_WMV_H_
#define _VDEC_HAL_IF_WMV_H_

#include "../include/vdec_info_wmv.h"
#include "../include/vdec_info_common.h"

/*! \name Video Decoder HAL WMV Interface
* @{
*/

/// Initialize video decoder hardware
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_WMV_InitVDecHW(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_WMV_VFIFO_PRM_T *prWmvVFifoInitPrm      ///< [IN] Pointer to video fifo address information
);


/// Read Barrel Shifter after shifting
/// \return Value of barrel shifter input window after shifting
UINT32 u4VDEC_HAL_WMV_ShiftGetBitStream(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                  ///< [IN] Shift bits number
);


/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
#if 0   //VDEC_REMOVE_UNUSED_FUNC
UINT32 u4VDEC_HAL_WMV_GetBitStreamShift(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);
#endif

/// Read Barrel Shifter before shifting
/// \return  Most significant (32 - u4ShiftBits) bits of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_WMV_GetRealBitStream(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);


/// Initialize barrel shifter with byte alignment
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_WMV_InitBarrelShifter(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                     ///< [IN] Video decoder hardware ID
    VDEC_INFO_WMV_BS_INIT_PRM_T *prWmvBSInitPrm,
    BOOL fgIsVC1
);


/// Read current read pointer
/// \return Current read pointer with byte alignment
UINT32 u4VDEC_HAL_WMV_ReadRdPtr(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4VFIFOSa,
    UINT32 *pu4Bits                                     ///< [OUT] Read pointer with remained bits
);


/// Align read pointer to byte,word or double word
/// \return None
void vVDEC_HAL_WMV_AlignRdPtr(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4VFIFOSa,
    UINT32 u4AlignType                                  ///< [IN] Align type
);


/// Set HW to decode bit plane
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_WMV_HWDecBP(
    UINT32 u4VDecID,                                             ///< [IN] Video decoder hardware ID
    UINT32 u4BpType,                                             ///< [IN] Bit plane type
    VDEC_INFO_WMV_DEC_BP_PRM_T *prWmvDecBpPrm   ///< [IN] Pointer to WMV bit plane decoding information struct
);


/// Read barrel shifter bitcount after initializing 
/// \return Bitcount counted by HAL
#if 0   //VDEC_REMOVE_UNUSED_FUNC
UINT32 u4VDEC_HAL_WMV_GetBitcount(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter ID
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);
#endif

/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_WMV_DecStart(
    UINT32 u4VDecID,
    VDEC_INFO_DEC_PRM_T *prDecPrm            ///< [IN] Pointer to WMV decode Information
);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_WMV_VPStart(
    UINT32 u4VDecID,
    VDEC_INFO_DEC_PRM_T *prDecPrm            ///< [IN] Pointer to WMV decode Information
);


/// Read current decoded mbx and mby
/// \return None
void vVDEC_HAL_WMV_GetMbxMby(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 *pu4Mbx,                                      ///< [OUT] Pointer to current decoded macroblock in x axis
    UINT32 *pu4Mby                                       ///< [OUT] Pointer to current decoded macroblock in y axis
);


/// Read error count after decoding end
/// \return None
void vVDEC_HAL_WMV_GetErrInfo(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_WMV_ERR_INFO_T *prWmvErrInfo          ///< [OUT] Pointer to error information
);


/// Read WMV error type after decoding end
/// \return WMV decode error type
UINT32 u4VDEC_HAL_WMV_GetErrType(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// WMV Dec end process, only for verification
/// \return None
void vVDEC_HAL_WMV_DecEndProcess(
    UINT32 u4VDecID
);

void vVDEC_HAL_WMV_VDec_DumpReg(UINT32 u4VDecID, BOOL fgBefore);

#if CONFIG_DRV_VERIFY_SUPPORT
void vVDEC_HAL_WMV_VDec_ReadCheckSum(UINT32 u4VDecID, UINT32 *pu4DecCheckSum);

BOOL fgVDEC_HAL_WMV_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum);

UINT32 u4VDEC_HAL_WMV_VDec_ReadFinishFlag(UINT32 u4VDecID);
#endif

//
/*! @} */


#endif //#ifndef _HAL_VDEC_WMV_IF_H_

