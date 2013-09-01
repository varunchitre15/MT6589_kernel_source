#ifndef _VDEC_VERIFY_VPARSER_WMV_H_
#define _VDEC_VERIFY_VPARSER_WMV_H_

#include <mach/mt_typedefs.h>

UINT32 u4DecodeVOLHead_WMV3(UINT32 u4InstID);
void vRCVFileHeader(UINT32 u4InstID);
UINT32 u4DecodeVOLHead_WMV12(UINT32 u4InstID);
BOOL fgVParserProcWMV(UINT32 u4InstID);
void vWMVSearchSliceStartCode(UINT32 u4InstID);
void AdjustReconRange(UINT32 u4InstID);
void UpdateVopheaderParam(UINT32 u4InstID);


#endif // _PR_EMU_H_

