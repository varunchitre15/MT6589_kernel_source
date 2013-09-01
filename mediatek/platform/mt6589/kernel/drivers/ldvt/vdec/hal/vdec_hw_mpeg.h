
#ifndef _VDEC_HW_MPEG_H_
#define _VDEC_HW_MPEG_H_
//#include <mach/mt6575_typedefs.h>
//#include "vdec_info_common.h"



// *********************************************************************
//  Video Decoder HW Functions
// *********************************************************************
extern void vVLDDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *ptDecPrm);
extern void vVLDDx3Dec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *ptDecPrm);
extern void vVLDM4vDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *ptDecPrm, BOOL fgBVop);
extern void vVDecMpegCommSetting (UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
extern void vMCSetOutputBuf(UINT32 u4VDecID, UINT32 u4OutBufIdx, UINT32 u4FRefBufIdx);
extern void VDecDumpMP4Register(UINT32 u4VDecID);
extern void VDecDumpMpegRegister(UINT32 u4VDecID,BOOL fgTriggerAB);

#endif

