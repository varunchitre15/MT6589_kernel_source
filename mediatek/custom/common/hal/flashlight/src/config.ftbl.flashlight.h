#ifndef _MTK_CUSTOM_PROJECT_HAL_IMGSENSOR_SRC_CONFIGFTBLFLASHLIGHT_H_
#define _MTK_CUSTOM_PROJECT_HAL_IMGSENSOR_SRC_CONFIGFTBLFLASHLIGHT_H_
#if 1
//


/*******************************************************************************
 *
 ******************************************************************************/
#define CUSTOM_FLASHLIGHT   "flashlight"
FTABLE_DEFINITION(CUSTOM_FLASHLIGHT)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
FTABLE_SCENE_INDEP()
    //==========================================================================
#if 1
    if  (1 == facing)
    {
        MY_LOGD("facing=yes");
	    FTABLE_CONFIG_AS_TYPE_OF_DEFAULT_VALUES(
	        KEY_AS_(MtkCameraParameters::KEY_FLASH_MODE),
	        SCENE_AS_DEFAULT_SCENE(
	            ITEM_AS_DEFAULT_(MtkCameraParameters::FLASH_MODE_OFF),
	            ITEM_AS_VALUES_(
	            CameraParameters::FLASH_MODE_OFF,
	            )
	        ),
	    )
        return  true;
    }
#endif
    //==========================================================================
#if     defined(CUSTOM_FLASHLIGHT_TYPE_constant_flashlight)
    if  (NSSensorType::eSensorType_RAW==u4SensorType)
    {
    FTABLE_CONFIG_AS_TYPE_OF_DEFAULT_VALUES(
        KEY_AS_(MtkCameraParameters::KEY_FLASH_MODE),
        SCENE_AS_DEFAULT_SCENE(
            ITEM_AS_DEFAULT_(MtkCameraParameters::FLASH_MODE_OFF),
            ITEM_AS_VALUES_(
            CameraParameters::FLASH_MODE_OFF,
            CameraParameters::FLASH_MODE_ON,
            CameraParameters::FLASH_MODE_AUTO,
            CameraParameters::FLASH_MODE_RED_EYE,
            CameraParameters::FLASH_MODE_TORCH,
            )
        ),
        //......................................................................
        #if 1   //  SCENE HDR
        SCENE_AS_(MtkCameraParameters::SCENE_MODE_HDR,
            ITEM_AS_DEFAULT_(MtkCameraParameters::FLASH_MODE_OFF),
            ITEM_AS_VALUES_(
            CameraParameters::FLASH_MODE_OFF,
            )
        )
        #endif
        //......................................................................
        //......................................................................
        #if 1   //  SCENE Fireworks
        SCENE_AS_(MtkCameraParameters::SCENE_MODE_FIREWORKS,
            ITEM_AS_DEFAULT_(MtkCameraParameters::FLASH_MODE_OFF),
            ITEM_AS_VALUES_(
            CameraParameters::FLASH_MODE_OFF,
            )
        )
        #endif
        //......................................................................

    )
    }
    else
    {
    FTABLE_CONFIG_AS_TYPE_OF_DEFAULT_VALUES(
        KEY_AS_(MtkCameraParameters::KEY_FLASH_MODE),
        SCENE_AS_DEFAULT_SCENE(
            ITEM_AS_DEFAULT_(MtkCameraParameters::FLASH_MODE_OFF),
            ITEM_AS_VALUES_(
            CameraParameters::FLASH_MODE_OFF,
            CameraParameters::FLASH_MODE_ON,
            //CameraParameters::FLASH_MODE_AUTO,
            CameraParameters::FLASH_MODE_RED_EYE,
            CameraParameters::FLASH_MODE_TORCH,
            )
        ),
    )
    }
    //==========================================================================
#elif   defined(CUSTOM_FLASHLIGHT_TYPE_peak_flashlight)
    FTABLE_CONFIG_AS_TYPE_OF_DEFAULT_VALUES(
        KEY_AS_(MtkCameraParameters::KEY_FLASH_MODE),
        SCENE_AS_DEFAULT_SCENE(
            ITEM_AS_DEFAULT_(MtkCameraParameters::FLASH_MODE_OFF),
            ITEM_AS_VALUES_(
            CameraParameters::FLASH_MODE_OFF,
            CameraParameters::FLASH_MODE_ON,
            CameraParameters::FLASH_MODE_AUTO,
            CameraParameters::FLASH_MODE_RED_EYE,
            )
        ),
    )
    //==========================================================================
#elif   defined(CUSTOM_FLASHLIGHT_TYPE_torch_flashlight)
    #warning "[torch_flashlight]"
    //==========================================================================
#elif   defined(CUSTOM_FLASHLIGHT_TYPE_dummy_flashlight)
    #warning "[dummy_flashlight]"
    //==========================================================================
#else
    #warning "[else flashlight]"
#endif
    //==========================================================================
END_FTABLE_SCENE_INDEP()
//------------------------------------------------------------------------------
END_FTABLE_DEFINITION()


/*******************************************************************************
 *
 ******************************************************************************/
#define CUSTOM_AFLAMP       "aflamp"
FTABLE_DEFINITION(CUSTOM_AFLAMP)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
FTABLE_SCENE_INDEP()
    //==========================================================================
#if 1
    if  (1 == facing)
    {
        MY_LOGD("facing=1");
        return  true;
    }
#endif
    //==========================================================================
#if     defined(CUSTOM_FLASHLIGHT_TYPE_constant_flashlight)
    if  (NSSensorType::eSensorType_RAW==u4SensorType)
    {
    FTABLE_CONFIG_AS_TYPE_OF_DEFAULT_VALUES(
        KEY_AS_(MtkCameraParameters::KEY_AF_LAMP_MODE),
        SCENE_AS_DEFAULT_SCENE(
            ITEM_AS_DEFAULT_(MtkCameraParameters::FLASH_MODE_OFF),
            ITEM_AS_VALUES_(
            CameraParameters::FLASH_MODE_OFF,
            CameraParameters::FLASH_MODE_ON,
            CameraParameters::FLASH_MODE_AUTO,
            )
        ),
    )
    }
    else
    {
    FTABLE_CONFIG_AS_TYPE_OF_DEFAULT_VALUES(
        KEY_AS_(MtkCameraParameters::KEY_AF_LAMP_MODE),
        SCENE_AS_DEFAULT_SCENE(
            ITEM_AS_DEFAULT_(MtkCameraParameters::FLASH_MODE_OFF),
            ITEM_AS_VALUES_(
            CameraParameters::FLASH_MODE_OFF,
            CameraParameters::FLASH_MODE_ON,
            //CameraParameters::FLASH_MODE_AUTO,
            )
        ),
    )
    }
    //==========================================================================
#else
#endif
    //==========================================================================
END_FTABLE_SCENE_INDEP()
//------------------------------------------------------------------------------
END_FTABLE_DEFINITION()
/*******************************************************************************
 *
 ******************************************************************************/


/*******************************************************************************
 *
 ******************************************************************************/
#endif
#endif //_MTK_CUSTOM_PROJECT_HAL_IMGSENSOR_SRC_CONFIGFTBLFLASHLIGHT_H_

