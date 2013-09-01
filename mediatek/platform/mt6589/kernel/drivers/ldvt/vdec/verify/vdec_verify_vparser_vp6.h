#ifndef _VDEC_VERIFY_VPARSER_VP6_H_
#define _VDEC_VERIFY_VPARSER_VP6_H_

#include <mach/mt_typedefs.h>

void vVerInitVP6(UINT32 u4InstID);
UINT32 u4VerVParserVP6(UINT32 u4InstID, BOOL fgInquiry);
void vVerifyVDecSetVP6Info(UINT32 u4InstID);
void vVerVP6VDecEnd(UINT32 u4InstID);
void vVerVP6DecEndProc(UINT32 u4InstID);
BOOL fgVP6CRCPatternExist(UINT32 u4InstID);
BOOL fgVP6SmallFolder(UINT32 u4InstID);


#endif // _PR_EMU_H_

