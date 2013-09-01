
#ifndef _VDEC_HW_H264_H_
#define _VDEC_HW_H264_H_
//#include "typedef.h"
//#include "vdec_info_common.h"



// *********************************************************************
//  Video Decoder HW Functions
// *********************************************************************
extern void vVDecWriteAVCVLD(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCVLD(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCMV(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCMV(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWriteAVCFG(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
extern UINT32 u4VDecReadAVCFG(UINT32 u4VDecID, UINT32 u4Addr);
extern UINT32 u4VDecAVCVLDShiftBits(UINT32 u4BSID, UINT32 u4VDecID);
extern UINT32 u4VDecAVCVLDGetBitS(UINT32 u4BSID,UINT32 u4VDecID,UINT32 dShiftBit);
extern BOOL fgInitH264BarrelShift2(UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm);
extern BOOL fgInitH264BarrelShift1(UINT32 u4VDecID, VDEC_INFO_H264_BS_INIT_PRM_T *prH264BSInitPrm);
extern void vInitFgtHWSetting(UINT32 u4VDecID, VDEC_INFO_H264_INIT_PRM_T *prH264VDecInitPrm);
extern UINT32 u4VDecReadH264VldRPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 *pu4Bits, UINT32 u4VFIFOSa);
extern UINT32 u4VDecReadPP(UINT32 u4VDecID, UINT32 u4Addr);
extern void vVDecWritePP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);


#endif

