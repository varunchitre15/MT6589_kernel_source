#ifndef _CAMERA_CUSTOM_HDR_H_
#define _CAMERA_CUSTOM_HDR_H_

#include "camera_custom_types.h"	// For MUINT*/MINT*/MVOID/MBOOL type definitions.

#define CUST_HDR_DEBUG          0   // Enable this will dump HDR Debug Information into SDCARD
#define HDR_DEBUG_OUTPUT_FOLDER		"/storage/sdcard1/"	// For ALPS.JB.
/**************************************************************************
 *                      D E F I N E S / M A C R O S                       *
 **************************************************************************/
// For HDR Customer Parameters

// [Core Number]
//     - Value Range: 1(For single-core)/2(For multi-core).
#define CUST_HDR_CORE_NUMBER	2

// [Capture Algorithm]
//     - When CUST_HDR_CAPTURE_ALGORITHM==0,
//          Alway take 3 pictures
//     - When CUST_HDR_CAPTURE_ALGORITHM==1,
//          it means if there is less than HDR_NEOverExp_Percent/1000 pixels
//          over saturation in 0EV, we capture 2 frames instead.
//     - When CUST_HDR_CAPTURE_ALGORITHM==2,
//          Always take 2 pictures
#define CUST_HDR_CAPTURE_ALGORITHM   1
#define CUST_HDR_NEOverExp_Percent   15

// [Prolonged VD]
//     - Value Range: 1(default)~ (depend on sensor characteristics).
#define CUST_HDR_PROLONGED_VD	1

// [BRatio]
//     - Higher value:
//         - Decrease the degree of artifact (halo-like around object boundary).
//         - Avoid non-continued edge in fusion result.
//         - Decrease little dynamic range.
//         - Decrease sharpness.
//     - Value Range: 1 (non-blur) ~ 160. (Default: 40).
#define CUST_HDR_BRATIO		40

// [Gain]
//     - Higher value increase sharpness, but also increase noise.
//     - Value Range: 256 ~ 512. (Default: 256 (1x gain)) (384/256 = 1.5x gain)
#define CUST_HDR_GAIN_00	384
#define CUST_HDR_GAIN_01	384
#define CUST_HDR_GAIN_02	384
#define CUST_HDR_GAIN_03	256
#define CUST_HDR_GAIN_04	256
#define CUST_HDR_GAIN_05	256
#define CUST_HDR_GAIN_06	256
#define CUST_HDR_GAIN_07	256
#define CUST_HDR_GAIN_08	256
#define CUST_HDR_GAIN_09	256
#define CUST_HDR_GAIN_10	256

// [F Control]
//     -Higher value decreases the degree of flare, but also decrease parts of the image details.
//     - Value Range: 0 ~ 50. (Default: 10)
#define CUST_HDR_BOTTOM_FRATIO		10
//     - Value Range: 0 ~ 50. (Default: 10)
#define CUST_HDR_TOP_FRATIO		10
//     - Value Range: 0 ~ 24. (Default: 16)
#define CUST_HDR_BOTTOM_FBOUND		16
//     - Value Range: 0 ~ 24. (Default: 16)
#define CUST_HDR_TOP_FBOUND		16

// [De-halo Control]
//     - Higher value reduce more halo for sky, but also reduce more dynamic range in some parts of the image.
//     - Value Range: 0 (off) ~ 255. (Default: 245)
#define CUST_HDR_TH_HIGH				245

// [Noise Control]
//     - Higher value reduce more noise, but also reduce dynamic range in low light region.
//     - Value Range: 0 (off) ~ 255. (Default: 25)
#define CUST_HDR_TH_LOW					0

// [Level Subtract]
//     - Value Range: 0 (less low-frequency halo, less dynamic range) or 1 (more low-frequency halo, more dynamic range). (Default: 0)
#define CUST_HDR_TARGET_LEVEL_SUB		0


/**************************************************************************
 *     E N U M / S T R U C T / T Y P E D E F    D E C L A R A T I O N     *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *        P U B L I C    F U N C T I O N    D E C L A R A T I O N         *
 **************************************************************************/
MUINT32 CustomHdrCoreNumberGet(void);
MUINT32 CustomHdrProlongedVdGet(void);
MUINT32 CustomHdrBRatioGet(void);
MUINT32 CustomHdrGainArrayGet(MUINT32 u4ArrayIndex);
double CustomHdrBottomFRatioGet(void);
double CustomHdrTopFRatioGet(void);
MUINT32 CustomHdrBottomFBoundGet(void);
MUINT32 CustomHdrTopFBoundGet(void);
MINT32 CustomHdrThHighGet(void);
MINT32 CustomHdrThLowGet(void);
MUINT32 CustomHdrTargetLevelSubGet(void);

/*******************************************************************************
* HDR exposure setting
*******************************************************************************/
typedef struct HDRExpSettingInputParam_S
{
    MUINT32 u4MaxSensorAnalogGain; // 1x=1024
    MUINT32 u4MaxAEExpTimeInUS;    // unit: us
    MUINT32 u4MinAEExpTimeInUS;     // unit: us
    MUINT32 u4ShutterLineTime;    // unit: 1/1000 us
    MUINT32 u4MaxAESensorGain;     // 1x=1024
    MUINT32 u4MinAESensorGain;     // 1x=1024
    MUINT32 u4ExpTimeInUS0EV;      // unit: us
    MUINT32 u4SensorGain0EV;       // 1x=1024
    MUINT8  u1FlareOffset0EV;
    MUINT32 u4Histogram[128];
} HDRExpSettingInputParam_T;

typedef struct HDRExpSettingOutputParam_S
{
    MUINT32 u4OutputFrameNum;     // Output frame number (2 or 3)
    MUINT32 u4ExpTimeInUS[3];     // unit: us; [0]-> short exposure; [1]: 0EV; [2]: long exposure
    MUINT32 u4SensorGain[3];      // 1x=1204; [0]-> short exposure; [1]: 0EV; [2]: long exposure
    MUINT8  u1FlareOffset[3];     // [0]-> short exposure; [1]: 0EV; [2]: long exposure
    MUINT32 u4FinalGainDiff[2];   // 1x=1024; [0]: Between short exposure and 0EV; [1]: Between 0EV and long exposure
    MUINT32 u4TargetTone; //Decide the curve to decide target tone
} HDRExpSettingOutputParam_T;

MVOID getHDRExpSetting(const HDRExpSettingInputParam_T& rInput, HDRExpSettingOutputParam_T& rOutput);

/**************************************************************************
 *                   C L A S S    D E C L A R A T I O N                   *
 **************************************************************************/




#endif	// _CAMERA_CUSTOM_HDR_H_

