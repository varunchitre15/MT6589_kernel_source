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


/* SENSOR VGA SIZE */
#define MT9V113_IMAGE_SENSOR_VGA_WIDTH					(640)
#define MT9V113_IMAGE_SENSOR_VGA_HEIGHT					(480)


/* Sensor vertical blanking & horizontal blanking, for preview and capture mode. */
#define MT9V113_DEFUALT_PREVIEW_LINE_LENGTH  0x034A  //842
#define MT9V113_DEFUALT_PREVIEW_FRAME_LENGTH 0x0206  //518

#define PREVIEW_VISIBLE_PIXELS				(0x28B - 0x04)
#define PREVIEW_VISIBLE_LINES				(0x1EB - 0x04)
#define CAPTURE_VISIBLE_PIXELS				(0x28B - 0x04)
#define CAPTURE_VISIBLE_LINES				(0x1EB - 0x04)


#define PREVIEW_H_BLANKING					(0x034A - PREVIEW_VISIBLE_PIXELS)
#define PREVIEW_V_BLANKING					(0x0206/*0x0276*/ - PREVIEW_VISIBLE_LINES)
#define CAPTURE_H_BLANKING					(0x06FE - CAPTURE_VISIBLE_PIXELS)
#define CAPTURE_V_BLANKING					(0x01FB - CAPTURE_VISIBLE_LINES)


/* Flicker factor to calculate tha minimal shutter width step for 50Hz and 60Hz  */
#define MACRO_50HZ							(100)
#define MACRO_60HZ							(120)

#define FACTOR_50HZ							(MACRO_50HZ * 1000)
#define FACTOR_60HZ							(MACRO_60HZ * 1000)


//Mode state
typedef enum MT9V113_CAMCO_MODE
{		
  MT9V113_CAM_PREVIEW=0,//Camera Preview
  
  MT9V113_CAM_CAPTURE,//Camera Capture

  MT9V113_VIDEO_MPEG4,//Video Mode
  MT9V113_VIDEO_MJPEG,
  
  MT9V113_WEBCAM_CAPTURE,//WebCam
  
  MT9V113_VIDEO_MAX
} MT9V113_Camco_MODE;

struct MT9V113_sensor_struct
{
	kal_uint16 sensor_id;

	kal_uint16 Dummy_Pixels;
	kal_uint16 Dummy_Lines;
	kal_uint32 Preview_PClk;

	kal_uint32 Preview_Lines_In_Frame;  
	kal_uint32 Capture_Lines_In_Frame;

	kal_uint32 Preview_Pixels_In_Line;  
	kal_uint32 Capture_Pixels_In_Line;
	kal_uint16 Preview_Shutter;
	kal_uint16 Capture_Shutter;

	kal_uint16 StartX;
	kal_uint16 StartY;
	kal_uint16 iGrabWidth;
	kal_uint16 iGrabheight;

	kal_uint16 Capture_Size_Width;
	kal_uint16 Capture_Size_Height;
	kal_uint32 Digital_Zoom_Factor;

	kal_uint16 Max_Zoom_Factor;

	kal_uint32 Min_Frame_Rate;
	kal_uint32 Max_Frame_Rate;
	kal_uint32 Fixed_Frame_Rate;
	//kal_bool Night_Mode;
	MT9V113_Camco_MODE Camco_mode;
	AE_FLICKER_MODE_T Banding;

	kal_bool Night_Mode;
};

  
#define MT9V113_WRITE_ID 0x7A
#define MT9V113_READ_ID 0x7B


//export functions
UINT32 MT9V113Open(void);
UINT32 MT9V113GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 MT9V113GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 MT9V113Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 MT9V113FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 MT9V113Close(void);


#endif /* __SENSOR_H */
