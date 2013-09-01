
#ifndef _VDEC_HAL_IF_RM_H_
#define _VDEC_HAL_IF_RM_H_

#include "../include/vdec_info_rm.h"
//#include "vdec_info_mpeg.h"
#include "../include/vdec_info_common.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#endif

//#include "vdec_hw_common.h"
//#include "vdec_verify_typedef.h"
//#include "vdec_verify_keydef.h"
//#include "typedef.h"


/*! \name Video Decoder HAL RM Interface
* @{
*/

/// Init RM Vdec HW
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
void vRM_Hal_InitVDecHW(
  UINT32 u4VDecID,                                                               ///< [IN] Video decoder hardware ID
  VDEC_INFO_RM_VFIFO_PRM_T *prRMVFifoPrm                     ///< [IN] VFIFO Information
);

/// Initialize Barrel Shifter HW
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4RM_HAL_InitBarrelShifter(
  UINT32 u4BSID,                                                                     ///< [IN] Barrel shifter hardware ID of one video decoder
  UINT32 u4VDecID,                                                                ///< [IN] Video decoder hardware ID
  VDEC_INFO_RM_BS_INIT_PRM_T *prMpegBSInitPrm         ///< [IN] Shift bits number
);


/// Get Bitstream from BarrelShifter
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_RM_ShiftGetBitStream(
  UINT32 u4BSID,                                                                     ///< [IN] Barrel shifter hardware ID of one video decoder
  UINT32 u4VDecID,                                                                 ///< [IN] Video decoder hardware ID
  UINT32 u4ShiftBits                                                                ///< [IN] Shift bits number
);

#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8560)
/// Trigger RM Decoder
/// \return None
void vVDEC_HAL_RM_TriggerDecode(
  UINT32 u4VDecID,                                                                 ///< [IN] Video decoder hardware ID
  VDEC_INFO_RM_PICINFO_T *prParsingPic                             ///< [IN] Picture Information for Decoding
);
#else
void vVDEC_HAL_RM_TriggerDecode(
  UINT32 u4VDecID,                                                                 ///< [IN] Video decoder hardware ID
  VDEC_INFO_RM_PICINFO_T *prParsingPic,                             ///< [IN] Picture Information for Decoding
  VDEC_INFO_LBD_PRM_T *prLBDInfo
);
#endif


/// VLD Soft Reset
/// \return None
void vRM_Hal_VldSoftReset(
  UINT32 u4VDecID                                                                   ///< [IN] Video decoder hardware ID
);



/// MV HW Init
/// \return None
void vRM_Hal_MvInit( 
  UINT32 u4VDecID,                                                                  ///< [IN] Video decoder hardware ID
  UINT32 u4MVHwWorkBuf                                                        ///< [IN] RM MV Working Buffer
);


/// MC HW Init
/// \return None
void vRM_Hal_McInit(
  UINT32 u4VDecID                                                                   ///< [IN] Video decoder hardware ID
);


/// PP HW Init
/// \return None
void vRM_Hal_PpInit(
  UINT32 u4VDecID                                                                   ///< [IN] Video decoder hardware ID
);


/// VLD Init
/// \return None
void vRM_Hal_VldInit(
  UINT32 u4VDecID,                                                                  ///< [IN] Video decoder hardware ID
  UINT32 u4VldPredWorkBuf                                                    ///< [IN] RM VLD Working Buffer
);



/// RM Init
/// \return None
void vRM_HAL_InitVDecHW(
  UINT32 u4VDecID,                                                                  ///< [IN] Video decoder hardware ID
  VDEC_INFO_RM_VFIFO_PRM_T *prRMVFifoPrm                        ///< [IN] RM VFIFO Information
);


/// Get RM Decode Finish Status
/// \return Value of Decode Finish Status
UINT32 u4VDEC_HAL_RM_VDec_ReadFinishFlag(
  UINT32 u4VDecID                                                                    ///< [IN] Video decoder hardware ID
);


/// Get MB X,Y 
/// \return None
void vVDEC_HAL_RM_GetMbxMby(
  UINT32 u4VDecID,                                                                   ///< [IN] Video decoder hardware ID
  UINT32 *pu4Mbx,                                                                    ///< [OUT] MBx Information
  UINT32 *pu4Mby                                                                     ///< [OUT] MBy Information
);


/// Get Ref QP MB0 Result
void vVDEC_HAL_RM_GetRefQpMb0(
  UINT32 u4VDecID,                                                                  ///< [IN] Video decoder hardware ID
  UINT32 *pu4RefQpMb0                                                           ///< [OUT] RefQpMb0 HW Result
);

/// Get RM HW Erro Info
void vVDEC_HAL_RM_GetErrInfo(
  UINT32 u4VDecID,                                                                  ///< [IN] Video decoder hardware ID
  VDEC_INFO_RM_ERR_INFO_T *prRMErrInfo                             ///< [OUT] HW Error Info
);


/// Get VLD HW Real Read Pointer
UINT32 u4VDEC_HAL_RM_ReadRdPtr(
  UINT32 u4BSID,                                                                      ///< [IN] Barrel shifter hardware ID of one video decoder
  UINT32 u4VDecID,                                                                  ///< [IN] Video decoder hardware ID
  UINT32 u4VFIFOSa,                                                                ///< [IN] VFIFO Start Address
  UINT32 *pu4Bits                                                                     ///< [OUT] Read pointer with remained bits
);


//
/*! @} */


#endif //#ifndef _VDEC_HAL_IF_RM_H_

