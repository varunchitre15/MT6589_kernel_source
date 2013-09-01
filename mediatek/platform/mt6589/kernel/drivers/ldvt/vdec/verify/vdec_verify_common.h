#ifndef _VDEC_VERIFY_COMMON_H_
#define _VDEC_VERIFY_COMMON_H_

#include "../include/vdec_info_common.h"


void vOutputPOCData(UINT32 dwDecOrder);
UCHAR ucVDecGetMinPOCFBuf(UINT32 u4InstID,VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm, BOOL fgWithEmpty);
void vVerifyClrFBufInfo(UINT32 u4InstID, UINT32 u4FBufIdx);
void vFlushDPB(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm, BOOL fgWithOutput);
void vVerInitVDec(UINT32 u4InstID);
void vVParserProc(UINT32 u4InstID);
void vVDecProc(UINT32 u4InstID);
void vChkVDec(UINT32 u4InstID);

#endif


