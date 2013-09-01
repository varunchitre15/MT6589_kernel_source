

#ifndef __TVC_DRV_H__
#define __TVC_DRV_H__

#include "disp_drv.h"
#include "tv_def.h"


#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------

#define TVC_CHECK_RET(expr)             \
    do {                                \
        TVC_STATUS ret = (expr);        \
        ASSERT(TVC_STATUS_OK == ret);   \
    } while (0)

// ---------------------------------------------------------------------------

typedef enum
{
   TVC_STATUS_OK = 0,

   TVC_STATUS_ERROR,
} TVC_STATUS;


typedef enum
{
    TVC_RGB565     = 0,
    TVC_YUV420_PLANAR = 1,
    TVC_UYVY422 = 2,
    TVC_YUV420_BLK = 3,
} TVC_SRC_FORMAT;


typedef enum
{
    TVC_NTSC  = 0, // 525 lines
    TVC_PAL_M = 1, // 525 lines
    TVC_PAL_C = 2, // 625 lines
    TVC_PAL   = 3, // 625 lines
} TVC_TV_TYPE;


// ---------------------------------------------------------------------------

TVC_STATUS TVC_Init(void);
TVC_STATUS TVC_Deinit(void);

TVC_STATUS TVC_PowerOn(void);
TVC_STATUS TVC_PowerOff(void);

TVC_STATUS TVC_Enable(void);
TVC_STATUS TVC_Disable(void);

TVC_STATUS TVC_SetTvType(TVC_TV_TYPE type);

TVC_STATUS TVC_SetSrcFormat(TVC_SRC_FORMAT format);
TVC_STATUS TVC_SetSrcRGBAddr(UINT32 address);
TVC_STATUS TVC_SetSrcYUVAddr(UINT32 Y, UINT32 U, UINT32 V);
TVC_STATUS TVC_SetSrcSize(UINT32 width, UINT32 height);
TVC_STATUS TVC_SetTarSize(UINT32 width, UINT32 height);
TVC_STATUS TVC_SetTarSizeForHQA(BOOL enable);

TVC_STATUS TVC_CommitChanges(BOOL blocking);
UINT32     TVC_GetWorkingAddr(void);
TVC_STATUS TVC_AllocMva(unsigned int va, unsigned int size, unsigned int* mva);
TVC_STATUS TVC_DeallocMva(unsigned int va, unsigned int size, unsigned int mva);
TVC_STATUS TVC_CheckFormat(TVC_SRC_FORMAT format);

// Debug
TVC_STATUS TVC_DumpRegisters(void);

void TVC_ConfigSize(unsigned int src_width, unsigned int src_height, unsigned int tar_width, unsigned int tar_height);
void TVC_SetCheckLineOffset(UINT32 offset);


// ---------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __TVC_DRV_H__
