#ifndef _VDEC_VERIFY_VPARSER_AVS_H_
#define _VDEC_VERIFY_VPARSER_AVS_H_

#include <mach/mt_typedefs.h>

void vVerInitAVS(UINT32 u4InstID);
UINT32 u4VerVParserAVS(UINT32 u4InstID, BOOL fgInquiry);
void vVerifyVDecSetAVSInfo(UINT32 u4InstID);
void vVerAVSVDecEnd(UINT32 u4InstID);
void vVerAVSDecEndProc(UINT32 u4InstID);
#endif // _PR_EMU_H_

