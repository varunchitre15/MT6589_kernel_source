
#ifndef _VDEC_DRV_JPEG_INFO_H_
#define _VDEC_DRV_JPEG_INFO_H_

//#include "x_os.h"
//#include "x_bim.h"
//#include "x_assert.h"
//#include "x_timer.h"

#include "drv_common.h"

#include "vdec_common_if.h"
#include "vdec_usage.h"

//#include "vdec_info_mpeg.h"
#include "vdec_info_common.h"

/******************************************************************************
* Local definition
******************************************************************************/



extern void vJPEGInitProc(UCHAR ucEsId);
extern INT32 i4JPEGVParseProc(UCHAR ucEsId, UINT32 u4VParseType);
extern BOOL fgJPEGVParseChkProc(UCHAR ucEsId);
extern INT32 i4JPEGUpdInfoToFbg(UCHAR ucEsId);
extern void vJPEGStartToDecProc(UCHAR ucEsId);
extern void vJPEGISR(UCHAR ucEsId);
extern BOOL fgIsJPEGDecEnd(UCHAR ucEsId);
extern BOOL fgIsJPEGDecErr(UCHAR ucEsId);
extern BOOL fgJPEGResultChk(UCHAR ucEsId);
extern BOOL fgIsJPEGInsToDispQ(UCHAR ucEsId);
extern BOOL fgIsJPEGGetFrmToDispQ(UCHAR ucEsId);
extern void vJPEGEndProc(UCHAR ucEsId);
extern BOOL fgJPEGFlushDPB(UCHAR ucEsId, BOOL fgWithOutput);    
extern void vJPEGReleaseProc(UCHAR ucEsId, BOOL fgResetHw);
extern INT32 i4JPEGParser(VDEC_ES_INFO_T* prVDecEsInfo, UINT32 u4VParseType);


extern VDEC_DRV_IF* VDec_GetJPEGIf(void);


#endif // _VDEC_DRV_JPEG_INFO_H_
