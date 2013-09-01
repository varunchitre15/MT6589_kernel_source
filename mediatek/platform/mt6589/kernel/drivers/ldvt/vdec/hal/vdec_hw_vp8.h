
#ifndef _VDEC_HW_VP8_H_
#define _VDEC_HW_VP8_H_
//#include "typedef.h"
//#include "vdec_info_common.h"



// *********************************************************************
//  Video Decoder HW Functions
// *********************************************************************
void vVDecWriteVP8VLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val, UINT32 u4BSID);
UINT32 u4VDecReadVP8VLD(UINT32 u4VDecID, UINT32 u4Addr);
void vVDecWriteVP8MC(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
UINT32 u4VDecReadVP8MC(UINT32 u4VDecID, UINT32 u4Addr);
void vVDecWriteVP8MV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
UINT32 u4VDecReadVP8MV(UINT32 u4VDecID, UINT32 u4Addr);
UINT32 u4VDecVP8VLDGetBits(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit);
void vVDecWriteVP8VLD2(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
UINT32 u4VDecReadVP8VLD2(UINT32 u4VDecID, UINT32 u4Addr);
UINT32 u4VDecWaitVP8GetBitsReady(UINT32 u4VDecID, UINT32 u4BSID);
UINT32 u4VDecReadVP8VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa);
void vVDecWriteVP8PP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
UINT32 u4VDecReadVP8PP(UINT32 u4VDecID, UINT32 u4Addr);
UINT32 u4VDecVP8BOOLGetBits(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit);
BOOL fgVDecWaitVP8VldFetchOk(UINT32 u4BSID, UINT32 u4VDecID, UINT8 u1VP8VLD);
UINT32 u4VDEC_VP8_VLDReadLiteral(UINT32 u4VDecID, UINT32 u4Bits);
void vVDec_VP8_HwAccCoefProbUpdate(UINT32 u4VDecID);
void vVDEC_VP8_HwAccMVProbUpdate(UINT32 u4VDecID);

#endif

