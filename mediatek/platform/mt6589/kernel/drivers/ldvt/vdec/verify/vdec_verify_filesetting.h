#ifndef _VDEC_VERIFY_FILESETTING_H_
#define _VDEC_VERIFY_FILESETTING_H_

#include "vdec_info_verify.h"

void vWrite2PC(UINT32 u4InstID, UCHAR bType, UCHAR *pbAddr);
BOOL fgVdecReadFileName(UINT32 u4InstID, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileInfo, VDEC_INFO_VERIFY_FILE_INFO_T *ptFileRecInfo, UINT32 *u4StartComp, UINT32 *u4EndComp , UINT32 *DumpPic);
void vAddStartCode2Dram(UCHAR *pbDramAddr);
void vH264WrData2PC(UINT32 u4InstID, UCHAR *ptAddr, UINT32 u4Size);
void vWMVWrData2PC(UINT32 u4InstID, UCHAR *ptAddr, UINT32 u4Size);
void vMPEGWrData2PC(UINT32 u4InstID, UCHAR *ptAddr, UINT32 u4Size);
void vFilledFBuf(UINT32 u4InstID, UCHAR *ptAddr, UINT32 u4Size);

void vDvWrData2PC(UINT32 u4InstID, UCHAR *ptAddr);

void vGenerateDownScalerGolden(UINT32 u4InstID, UINT32 DecYAddr,UINT32 DecCAddr, UINT32 u4Size);
void vConcateStr(char *strTarFileName, char *strSrcFileName, char *strAddFileName, UINT32 u4Idx);

BOOL vH264_CheckCRCResult(UINT32 u4InstID);
#ifdef LETTERBOX_SUPPORT
void vLBDParaParsing(UINT32 u4InstID);
void vCheckLBDResult(UINT32 u4InstID);
#endif

#endif

