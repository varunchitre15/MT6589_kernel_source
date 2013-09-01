/*****************************************************************************
 *
 * Filename:
 * ---------
 *   MSDK_lens_exp.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Definition and declaration of the interface of Lens module
 *
 *
 * Author:
 * -------
 *   PC Huang (MTK02204)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * Jul 28 2009 mtk02204
 * [DUMA00009824] [Camera Driver] Check in customized driver of flash light
 * 
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#ifndef __CAMERA_CUSTOM_FLASHLIGHT_H_
#define __CAMERA_CUSTOM_FLASHLIGHT_H_

//#include "msdk_isp_feature_exp.h"

//flash type enum
typedef enum
{
    FLASHLIGHT_NONE = 0,
    FLASHLIGHT_LED_ONOFF,           // LED always on/off
    FLASHLIGHT_LED_CONSTANT,    // CONSTANT type LED
    FLASHLIGHT_LED_PEAK,             // peak strobe type LED
    FLASHLIGHT_LED_TORCH,          // LED turn on when switch FLASH_ON
    FLASHLIGHT_XENON_SCR,        // SCR strobe type Xenon
    FLASHLIGHT_XENON_IGBT        // IGBT strobe type Xenon
}   FLASHLIGHT_TYPE_ENUM;

typedef enum
{
    FLASH_LIGHT_FEATURE_POWER_ON,
    FLASH_LIGHT_FEATURE_POWER_OFF,
    FLASH_LIGHT_FEATURE_MAX
}   FLASH_LIGHT_FEATURE_ENUM;

/* flash light interface */
/* function pointer that provided to MSDK, all return value are declared as UINT32 are
   reserved for returing error code if necessary. Currently, it is not necessary, please return
   ERROR_NONE. */
typedef struct
{
   UINT32 (* FlashLightOpen)(void);
   UINT32 (* FlashLightFeatureControl) (FLASH_LIGHT_FEATURE_ENUM FeatureId, UINT8 *pFeatureParaIn, UINT16 FeatureParaInLen);
   UINT32 (* FlashLightClose)(void);
} FLASH_LIGHT_FUNCTION_STRUCT, *PFLASH_LIGHT_FUNCTION_STRUCT;

FLASHLIGHT_TYPE_ENUM FlashLightInit(PFLASH_LIGHT_FUNCTION_STRUCT *pfFunc);

#endif /* __MSDK_FLASH_LIGHT_EXP_H_ */




