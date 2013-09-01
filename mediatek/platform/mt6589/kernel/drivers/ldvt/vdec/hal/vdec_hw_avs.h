
#ifndef _VDEC_HW_AVS_H_
#define _VDEC_HW_AVS_H_
//#include "typedef.h"
//#include "vdec_info_common.h"
#include "../include/vdec_info_avs.h"



// *********************************************************************
//  Video Decoder HW Functions
// *********************************************************************
extern void vVDecWriteAVSVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVSVLD(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVSMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVSMV(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVSPP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVSPP(UINT32 u4VDecID, UINT32 u4Addr);
extern UINT32 u4VDecAVSVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID);
extern UINT32 u4VDecAVSVLDGetBitS(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit);
extern BOOL fgInitAVSBarrelShift2(UINT32 u4VDecID, VDEC_INFO_AVS_BS_INIT_PRM_T *prHAvsBSInitPrm);
extern BOOL fgInitAVSBarrelShift1(UINT32 u4VDecID, VDEC_INFO_AVS_BS_INIT_PRM_T *prHAvsBSInitPrm);
extern UINT32 u4VDecReadAVSVldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa);
#endif

