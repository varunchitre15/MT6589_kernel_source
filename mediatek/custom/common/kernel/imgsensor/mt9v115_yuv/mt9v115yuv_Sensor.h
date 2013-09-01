/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Header file of Sensor driver
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
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H

#define MT9V115_WRITE_ID                      (0x7A)

/* SENSOR REGISTER DEFINE */
#define MT9V115_ID_REG                          (0x0000)
 
/* sensor size */
#define MT9V115_IMAGE_SENSOR_VGA_WIDTH          (640)
#define MT9V115_IMAGE_SENSOR_VGA_HEIGHT         (480)

 /* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define MT9V115_PERIOD_PIXEL_NUMS               (652 + 147 + 1)/* Active + HST + 1 */
#define MT9V115_PERIOD_LINE_NUMS                (492 + 9)      /* Active + 9 */

#define MT9V115_BLANK_REGISTER_LIMITATION       0x3FF

/*50Hz,60Hz*/
#define MT9V115_NUM_50HZ                        (50 * 2)
#define MT9V115_NUM_60HZ                        (60 * 2)

/* FRAME RATE UNIT */
#define MT9V115_FRAME_RATE_UNIT                 (10)

/* MAX CAMERA FRAME RATE */
#define MT9V115_MAX_CAMERA_FPS                  (MT9V115_FRAME_RATE_UNIT * 30)


//export functions
UINT32 MT9V115Open(void);
UINT32 MT9V115GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 MT9V115GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 MT9V115Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 MT9V115FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 MT9V115Close(void);


#endif /* __SENSOR_H */
