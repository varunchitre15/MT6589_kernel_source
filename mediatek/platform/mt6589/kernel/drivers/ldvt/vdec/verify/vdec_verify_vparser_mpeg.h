#ifndef _VDEC_VERIFY_VPARSER_MPEG_H_
#define _VDEC_VERIFY_VPARSER_MPEG_H_

#include <mach/mt_typedefs.h>

UINT32 u4VParserMPEG12(UINT32 u4InstID, BOOL fgInquiry);
void vVerifyMpvSetSeqLayerDecPrm(UINT32 u4InstID);
BOOL fgVerifyMpvPicDimChk(UINT32 u4InstID, UINT32 dWidth, UINT32 dHeight, BOOL fgVParser);

#endif // _PR_EMU_H_

