
#ifndef _VDEC_DRV_AVS_INFO_H_
#define _VDEC_DRV_AVS_INFO_H_

//#include "x_os.h"
//#include "x_bim.h"
//#include "x_assert.h"
//#include "x_timer.h"

#include "drv_common.h"

#include "vdec_common_if.h"
#include "vdec_usage.h"

#include "vdec_info_avs.h"
#include "vdec_info_common.h"

#include "../hal/vdec_hal_if_avs.h"


// Data unit type
#define AVS_SEQ_HDR_SC          0x1B0
#define AVS_SEQ_END_SC          0x1B1
#define AVS_I_PICTURE_SC        0x1B3
#define AVS_PB_PICTURE_SC     0x1B6
#define AVS_USER_DATA_SC      0x1B2
#define AVS_EXTENSION_SC       0x1B5
#define AVS_VIDEO_EDIT_SC      0x1B7
#define AVS_SLICE_SC_MIN        0x100
#define AVS_SLICE_SC_MAX       0x1AF


typedef struct _AVS_DRV_INFO_T
{
    UCHAR ucAVSDpbOutputFbId;
    UCHAR ucPredFbId;
    UINT32 u4PredSa;
    UINT32 u4CurrStartCodeAddr;
    UINT32 u4BitCount;
    UINT32 u4DecErrInfo;
    INT32 i4LatestIPOC;
    INT64 i8BasePTS;
    INT64 i8LatestRealPTS;
    INT32 i4PreFrmPOC;
    INT32 i4LatestRealPOC;
    UINT32 u4LatestSPSId;
}AVS_DRV_INFO_T;


extern void vAVSInitProc(UCHAR ucEsId);
extern INT32 i4AVSVParseProc(UCHAR ucEsId, UINT32 u4VParseType);
extern BOOL fgAVSVParseChkProc(UCHAR ucEsId);
extern INT32 i4AVSUpdInfoToFbg(UCHAR ucEsId);
extern void vAVSStartToDecProc(UCHAR ucEsId);
extern void vAVSISR(UCHAR ucEsId);
extern BOOL fgIsAVSDecEnd(UCHAR ucEsId);
extern BOOL fgIsAVSDecErr(UCHAR ucEsId);
extern BOOL fgAVSResultChk(UCHAR ucEsId);
extern BOOL fgIsAVSInsToDispQ(UCHAR ucEsId);
extern BOOL fgIsAVSGetFrmToDispQ(UCHAR ucEsId);
extern void vAVSEndProc(UCHAR ucEsId);
extern BOOL fgAVSFlushDPB(UCHAR ucEsId, BOOL fgWithOutput);
extern void vAVSReleaseProc(UCHAR ucEsId);



// vdec_drv_AVS_parse.c
extern void vSeq_Par_Set_Rbsp(UINT32 u4BSID,UINT32 u4VDecID, AVS_DRV_INFO_T *prAVSDrvDecInfo);
extern void vPic_Par_Set_Rbsp(UINT32 u4BSID, UINT32 u4VDecID, AVS_DRV_INFO_T *prAVSDrvDecInfo);
extern void vSEI_Rbsp(UINT32 u4BSID, UINT32 u4VDecID, AVS_DRV_INFO_T  *prAVSDrvDecInfo);
extern INT32 i4SlimParseSliceHeader(UINT32 u4VDecID, AVS_DRV_INFO_T  *prAVSDrvDecInfo);
extern INT32 i4ParseSliceHeader(UINT32 u4VDecID, AVS_DRV_INFO_T  *prAVSDrvDecInfo);


#endif
