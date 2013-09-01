
#include <stdlib.h>
#include <stdio.h>
#include "camera_custom_if.h"

//<2013/05/29-25486-alberthsiao, [ATS00160376]Add Model and Brand name to picture EXIF
#include <cutils/properties.h>
namespace NSCamCustom
{
/*******************************************************************************
* 
*******************************************************************************/


/*******************************************************************************
* custom exif
*******************************************************************************/
#define EN_CUSTOM_EXIF_INFO
MINT32 custom_SetExif(void **ppCustomExifTag)
{
#ifdef EN_CUSTOM_EXIF_INFO
#define CUSTOM_EXIF_STRING_MAKE  "custom make"
#define CUSTOM_EXIF_STRING_MODEL "custom model"
#define CUSTOM_EXIF_STRING_SOFTWARE "custom software"
static customExifInfo_t exifTag = {CUSTOM_EXIF_STRING_MAKE,CUSTOM_EXIF_STRING_MODEL,CUSTOM_EXIF_STRING_SOFTWARE};
//<2013/06/06-25777-alberthsiao, Fix long version name issue
char value[100],i;
//>2013/06/06-25777-alberthsiao
property_get("ro.product.brand", value, "Arima");
for (i=0;i<32;i++)
   exifTag.strMake[i]=value[i];
property_get("ro.product.model", value, "Pelican");
for (i=0;i<32;i++)
   exifTag.strModel[i]=value[i];
property_get("ro.build.id", value, "J.0.34");
for (i=0;i<32;i++)
   exifTag.strSoftware[i]=value[i];
//>2013/05/29-25486-alberthsiao
    if (0 != ppCustomExifTag) {
        *ppCustomExifTag = (void*)&exifTag;
    }
    return 0;
#else
    return -1;
#endif
}
//
customExif_t const&
getCustomExif()
{
    static customExif_t inst = {
        bEnCustom       :   false,  // default value: false.
        u4ExpProgram    :   0,      // default value: 0.    '0' means not defined, '1' manual control, '2' program normal
    };
    return inst;
}
//
MINT32 get_atv_disp_delay(MINT32 mode)
{
    return ((ATV_MODE_NTSC == mode)?ATV_MODE_NTSC_DELAY:((ATV_MODE_PAL == mode)?ATV_MODE_PAL_DELAY:0));
}

MINT32 get_atv_input_data()
{
    return ATV_INPUT_DATA_FORMAT;
}


/*******************************************************************************
* Author : cotta
* Functionality : custom flashlight gain between preview/capture flash
*******************************************************************************/
#define FLASHLIGHT_CALI_LED_GAIN_PRV_TO_CAP_10X 10
MUINT32 custom_GetFlashlightGain10X(void)
{   
    // x10 , 1 mean 0.1x gain    
    //10 means no difference. use torch mode for preflash and cpaflash
    //> 10 means capture flashlight is lighter than preflash light. < 10 is opposite condition.    
    return (MUINT32)FLASHLIGHT_CALI_LED_GAIN_PRV_TO_CAP_10X;
}

MUINT32 custom_BurstFlashlightGain10X(void)
{
    return (MUINT32)FLASHLIGHT_CALI_LED_GAIN_PRV_TO_CAP_10X;
}
/*******************************************************************************
* Author : Jiale
* Functionality : custom yuv flashlight threshold
*******************************************************************************/
#define FLASHLIGHT_YUV_THRESHOlD 3.0
double custom_GetYuvFlashlightThreshold(void)
{    
    return (double)FLASHLIGHT_YUV_THRESHOlD;
}

/*******************************************************************************
* Author : Jiale
* Functionality : custom yuv sensor convergence frame count
*******************************************************************************/
#define FLASHLIGHT_YUV_CONVERGENCE_FRAME 7
MINT32 custom_GetYuvFlashlightFrameCnt(void)
{    
    return (int)FLASHLIGHT_YUV_CONVERGENCE_FRAME;
}

/*******************************************************************************
* Author : CD
* Functionality : custom yuv sensor preflash duty
*******************************************************************************/
#define FLASHLIGHT_YUV_NORMAL_LEVEL 12
MINT32 custom_GetYuvFlashlightDuty(void)
{    
    return (int)FLASHLIGHT_YUV_NORMAL_LEVEL;
}

/*******************************************************************************
* Author : CD
* Functionality : custom yuv sensor capture flash duty (high current mode)
*******************************************************************************/
#define FLASHLIGHT_YUV_MAIN_HI_LEVEL 12
MINT32 custom_GetYuvFlashlightHighCurrentDuty(void)
{
    // if FLASHLIGHT_CALI_LED_GAIN_PRV_TO_CAP_10X > 10 (high current mode),
    // it means capture flashlight is lighter than preflash light.
    // In this case, you need to specify the level for capture flash accordingly.
    return (int)FLASHLIGHT_YUV_MAIN_HI_LEVEL;
}

/*******************************************************************************
* Author : CD
* Functionality : custom yuv sensor capture flash timeout (high current mode)
*******************************************************************************/
#define FLASHLIGHT_YUV_MAIN_HI_TIMEOUT 500
MINT32 custom_GetYuvFlashlightHighCurrentTimeout(void)
{
    // if FLASHLIGHT_CALI_LED_GAIN_PRV_TO_CAP_10X > 10 (high current mode),
    // it means capture flashlight is lighter than preflash light.
    // In this case, you may need to set the timeout in ms in case of LED burning out.
    return (int)FLASHLIGHT_YUV_MAIN_HI_TIMEOUT;
}


/*******************************************************************************
* Author : CD
* Functionality : custom yuv sensor flashlight step
*******************************************************************************/
#define FLASHLIGHT_YUV_STEP 7
MINT32 custom_GetYuvFlashlightStep(void)
{    
    return (int)FLASHLIGHT_YUV_STEP;
}


/*******************************************************************************
* 
*******************************************************************************/

};  //NSCamCustom

