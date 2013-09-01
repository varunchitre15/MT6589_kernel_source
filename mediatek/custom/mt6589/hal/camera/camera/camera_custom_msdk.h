
#ifndef __CAMERA_CUSTOM_MSDK_H
#define __CAMERA_CUSTOM_MSDK_H

#include "camera_custom_nvram.h"
#include "camera_custom_lens.h"


//FIXME , Should not export function outside.
extern MSDK_LENS_INIT_FUNCTION_STRUCT LensInitFunc[MAX_NUM_OF_SUPPORT_LENS];
extern MUINT32 gMainLensIdx;
extern MUINT32 gSubLensIdx;

/*******************************************************************************
*
********************************************************************************/
MUINT32 cameraCustomInit();
void GetCameraDefaultPara(MUINT32 SensorId,
						  PNVRAM_CAMERA_ISP_PARAM_STRUCT pCameraISPDefault,
						  PNVRAM_CAMERA_3A_STRUCT pCamera3ADefault,
						  PNVRAM_CAMERA_SHADING_STRUCT pCameraShadingDefault,
						  PAE_PLINETABLE_STRUCT pCameraAEPlineData);


MUINT32 GetCameraCalData(MUINT32 SensorId, MUINT32* pGetSensorCalData); // sl 11041000
MUINT32 LensCustomInit();
MUINT32 LensCustomSetIndex(MUINT32 a_u4CurrIdx);
MUINT32 LensCustomGetInitFunc(MSDK_LENS_INIT_FUNCTION_STRUCT *a_pLensInitFunc);
void GetLensDefaultPara(PNVRAM_LENS_PARA_STRUCT pLensParaDefault);


#endif // __CAMERA_CUSTOM_MSDK_H


