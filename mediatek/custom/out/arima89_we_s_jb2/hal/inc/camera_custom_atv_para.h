#ifndef _CAMERA_CUSTOM_ATV_PARA_H_
#define _CAMERA_CUSTOM_ATV_PARA_H_
//
//
namespace NSCamCustom
{

/*******************************************************************************
* ATV disp delay time
*******************************************************************************/

#define ATV_MODE_NTSC 30000
#define ATV_MODE_PAL  25000

#ifdef MTK_MT5192
//unit: us
#define ATV_MODE_NTSC_DELAY 5000
#define ATV_MODE_PAL_DELAY  10000

#else 
#ifdef MTK_MT5193
//unit: us
#define ATV_MODE_NTSC_DELAY 18000
#define ATV_MODE_PAL_DELAY  26000
#else
//unit: us
#define ATV_MODE_NTSC_DELAY 0
#define ATV_MODE_PAL_DELAY  0
#endif

#endif

// 0 meas data pin is 2~9;  1 means data pin is 0~7
#define ATV_INPUT_DATA_FORMAT 1


};  //NSCamCustom
#endif  //  _CAMERA_CUSTOM_IF_

