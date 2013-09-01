/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Header file of Sensor driver
 *
 *
 * Author:
 * -------
 *   Anyuan Huang (MTK70663)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 03 09 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Add the SIV120B sensor driver for MT6575 LDVT.
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H

#include "image_sensor.h"//get IMAGE_SENSOR_DRVNAME
#define IMAGE_SENSOR_DRVNAME SENSOR_DRVNAME_SIV120B_YUV

/* START GRAB PIXEL OFFSET */
#define SIV120D_IMAGE_SENSOR_START_GRAB_X   (1)
#define SIV120D_IMAGE_SENSOR_START_GRAB_Y   (1)

/* SENSOR PV SIZE */
#define SIV120D_IMAGE_SENSOR_PV_WIDTH       (640 - 16)
#define SIV120D_IMAGE_SENSOR_PV_HEIGHT      (480 - 12)


/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define SIV120D_PERIOD_PIXEL_NUMS          (652 + 144 + 3)/* Active + HST + 3;default pixel#(w/o dummy pixels) in VGA mode*/
#define SIV120D_PERIOD_LINE_NUMS           (490 + 9)      /* Active + 9 ;default line#(w/o dummy lines) in VGA mode*/

#define SIV120D_BLANK_REGISTER_LIMITATION   0xFFF

/*50Hz,60Hz*/
#define SIV120D_NUM_50HZ                    (50 * 2)
#define SIV120D_NUM_60HZ                    (60 * 2)
#define SIV120D_CLK_1MHZ                    (1000000)

/* FRAME RATE UNIT */
#define SIV120D_FRAME_RATE_UNIT              10
#define SIV120D_FPS(x)                       (SIV120D_FRAME_RATE_UNIT * (x))

/* SENSOR READ/WRITE ID */
#define SIV120D_WRITE_ID                     0x66

/* SENSOR CHIP VERSION */
#define SIV120B_SENSOR_VERSION               0x11
#define SIV120D_SENSOR_VERSION               0x13

//export functions
UINT32 SIV120DOpen(void);
UINT32 SIV120DGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 SIV120DGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 SIV120DControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 SIV120DFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 SIV120DClose(void);
#endif /* __SENSOR_H */
