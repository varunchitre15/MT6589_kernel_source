#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>


#include "camera_custom_errcode.h"
//#include "msdk_nvram_camera_exp.h"
#include "camera_custom_nvram.h"
#include "camera_custom_flashlight.h"
//#include "msdk_flash_light_exp.h"
//#include "image_sensor.h"
//#include "camera_3A_para_mt9p012.h"
//#include "msdk_comm_define_exp.h"
//#include "msdk_isp_exp.h"

//#include <windows.h>
//#include <ceddk.h>
//#include <ceddk_exp.h>

//#include "msdk_isp_exp.h"


/*************************************************************************
*	FLASH_LIGHT_FUNC_STRUCT functions for Windows Mobile
*************************************************************************/
UINT32	FlashLightOpen(void)
{
	return ERROR_NONE;
}	/* FlashLightOpen() */

UINT32	FlashLightFeatureControl(FLASH_LIGHT_FEATURE_ENUM FeatureId, UINT8 *pFeatureParaIn,
								 UINT16 FeatureParaInLen)
{
	return ERROR_NONE;
}	/* FlashLightControl() */

UINT32	FlashLightClose(void)
{
	return ERROR_NONE;
}	/* FlashLightClose() */

FLASH_LIGHT_FUNCTION_STRUCT FlashLightFuncMap=
{
	FlashLightOpen,
	FlashLightFeatureControl,
	FlashLightClose
};

FLASHLIGHT_TYPE_ENUM FlashLightInit(PFLASH_LIGHT_FUNCTION_STRUCT *pfFunc)
{
	*pfFunc=&FlashLightFuncMap;

	return FLASHLIGHT_LED_TORCH;
}

