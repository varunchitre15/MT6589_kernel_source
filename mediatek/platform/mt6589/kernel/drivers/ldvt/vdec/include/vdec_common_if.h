
#ifndef _VDEC_COMMON_IF_H_
#define _VDEC_COMMON_IF_H_

#include <mach/mt_typedefs.h>
#include "u_uerrcode.h"
//#include "x_printf.h"
//#include "x_os.h"
//#include "x_rtos.h"
//#include "x_common.h"

//#include "vdec_config.h"
#include "vdec_errcode.h"
#include "vdec_type.h"
#include "vdec_info_common.h"
#include "drv_common.h"

//#include "drv_vdec.h"
//#include "drv_fbm_if.h"
//#include "drv_fbm_errcode.h"
//#include "drv_esm_if.h"

//#include "drv_config.h"

#define VDSCL_SRC_MC              (0x0<<0)
#define VDSCL_SRC_PP               (0x1<<1)
#define VDSCL_SRC_FG               (0x1<<2)

#define VDSCL_SPEC_MPEG         (0)
#define VDSCL_SPEC_WMV          (1)
#define VDSCL_SPEC_264            (2)

typedef struct _VDEC_DRV_IF
{
//    VDEC_CODEC_T   eVdecCodecType;
    void (* pvVDecInitProc)(UCHAR ucEsId);
    INT32 (* pi4VDecVParseProc)(UCHAR ucEsId, UINT32 u4VParseType);
    BOOL (* pfgVDecVParseChkProc)(UCHAR ucEsId);
    INT32 (* pi4VDecUpdInfoToFbg)(UCHAR ucEsId);
    void (* pvVDecStartToDecProc)(UCHAR ucEsId);
    void (* pvVDecISR)(UCHAR ucEsId);
    BOOL (* pfgIsVDecEnd)(UCHAR ucEsId);    
    BOOL (* pfgIsVDecDecErr)(UCHAR ucEsId);     
    BOOL (* pfgVDecResultChk)(UCHAR ucEsId);
    BOOL (* pfgIsVDecInsToDispQ)(UCHAR ucEsId);
    BOOL (* pfgIsVDecGetFrmToDispQ)(UCHAR ucEsId);
    void (* pvVDecEndProc)(UCHAR ucEsId);  
    BOOL (*pfgVDecFlushDPB)(UCHAR ucEsId, BOOL fgWithOutput);
    void (* pvVDecReleaseProc)(UCHAR ucEsId, BOOL fgResetHW);      
#ifdef VDEC_SR_SUPPORT
    BOOL (* pvVDecGenEDPB)(UCHAR ucEsId);      
    BOOL (* pvVDecRestoreEDPB)(UCHAR ucEsId, BOOL fgRestore);
    BOOL (* pfgIsVDecGetSRFrmToDispQ)(UCHAR ucEsId, BOOL fgSeqEnd, BOOL fgRefPic);
    void (* pvVDecGetSeqFirstTarget)(UCHAR ucEsId);
    void (* pvVDecReleaseSRDrvInfo)(UCHAR ucEsId);      
    BOOL   (* pfgVDecGetDFBInfo)(UCHAR ucEsId, void **prDFBInfo);
    void   (* pvVDecRestoreSeqInfo)(UCHAR ucEsId);
    BOOL (* pfgVDecRvsDone)(UCHAR ucEsId);
    void   (*pvReleaseRefFrame)(UCHAR ucEsId);
    void   (*pvVDecBackupInfo)(UCHAR ucEsId);
#endif
#if  (defined(DRV_VDEC_VDP_RACING) || defined(VDEC_PIP_WITH_ONE_HW))
    BOOL (* pfgIsVDecPreInsToDispQ)(UCHAR ucEsId);
    BOOL (* pfgIsVDecPreGetFrmToDispQ)(UCHAR ucEsId);
    BOOL (* pfgIsVDecReadyForDisp)(UCHAR ucEsId);
    BOOL (* pfgIsVDecSetFBufInfo)(UCHAR ucEsId);
#endif
}VDEC_DRV_IF;

extern VDEC_DRV_IF* VDec_GetMPEG2If(void);
extern VDEC_DRV_IF* VDec_GetH264If(void);
extern VDEC_DRV_IF* VDec_GetMPEG4If(void);
extern VDEC_DRV_IF* VDec_GetWMVIf(void);
extern VDEC_DRV_IF* VDec_GetRMIf(void);
extern VDEC_DRV_IF* VDec_GetJPEGIf(void);
extern VDEC_DRV_IF* VDec_GetVP6If(void);
#ifdef VDEC_VP8_SUPPORT
extern VDEC_DRV_IF* VDec_GetVP8If(void);
#endif
//extern void vVDecSetEsmInfo(UINT32 pu4Handle, Decoder_OpIf* pprDecoderOplIf, UCHAR ucMpvId, UCHAR ucEsId);

#endif
