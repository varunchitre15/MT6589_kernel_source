
#ifndef _VDEC_HW_RM_H_

#include "../include/vdec_info_rm.h"


#define _VDEC_HW_RM_H_
#define VDEC_RM_USE_NORMAL_HAL_HW

// *********************************************************************
//  Video Decoder HW Functions
// *********************************************************************

EXTERN void vVDecWriteRMVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
EXTERN UINT32 u4VDecReadRMVLD(UINT32 u4VDecID, UINT32 u4Addr);
EXTERN void vVDecWriteRMPP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
EXTERN UINT32 u4VDecReadRMPP(UINT32 u4VDecID, UINT32 u4Addr);
EXTERN void vVDecWriteRMMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
EXTERN UINT32 u4VDecReadRMMV(UINT32 u4VDecID, UINT32 u4Addr);
EXTERN void vRM_VldSoftReset(UINT32 u4VDecID);
EXTERN void vRM_MvInit(UINT32 u4VDecID, UINT32 u4MVHwWorkBuf);
EXTERN void vRM_McInit(UINT32 u4VDecID);
EXTERN void vRM_PpInit(UINT32 u4VDecID);
EXTERN void vRM_VldInit(UINT32 u4VDecID, UINT32 u4VldPredHwWorkBuf);
EXTERN UINT32 u4RM_VDecVLDGetBitS(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBit);
EXTERN INT32 i4RM_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_RM_BS_INIT_PRM_T *prMpegBSInitPrm);
#ifdef RM_CRCCHECKFLOW_SUPPORT
EXTERN void vVDecWriteRMCRC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
#endif //RM_CRCCHECKFLOW_SUPPORT

#endif  //_VDEC_HW_RM_H_

