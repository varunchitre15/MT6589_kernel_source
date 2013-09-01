

#ifndef __TVE_DRV_H__
#define __TVE_DRV_H__

#include "disp_drv.h"
#include "tv_def.h"


#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------

#define TVE_CHECK_RET(expr)             \
    do {                                \
        TVE_STATUS ret = (expr);        \
        ASSERT(TVE_STATUS_OK == ret);   \
    } while (0)

// ---------------------------------------------------------------------------

typedef enum
{
   TVE_STATUS_OK = 0,

   TVE_STATUS_ERROR,
} TVE_STATUS;


typedef enum
{
    TVE_NTSC  = 0, // 525 lines
    TVE_PAL_M = 1, // 525 lines
    TVE_PAL_C = 2, // 625 lines
    TVE_PAL   = 3, // 625 lines
} TVE_TV_TYPE;


// ---------------------------------------------------------------------------

TVE_STATUS TVE_Init(void);
TVE_STATUS TVE_Deinit(void);

TVE_STATUS TVE_PowerOn(void);
TVE_STATUS TVE_PowerOff(void);

TVE_STATUS TVE_Enable(void);
TVE_STATUS TVE_Disable(void);

TVE_STATUS TVE_SetTvType(TVE_TV_TYPE type);
TVE_STATUS TVE_EnableColorBar(BOOL enable);

TVE_STATUS TVE_ResetDefaultSettings(void);


// Debug
TVE_STATUS TVE_DumpRegisters(void);

// ---------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __TVE_DRV_H__
