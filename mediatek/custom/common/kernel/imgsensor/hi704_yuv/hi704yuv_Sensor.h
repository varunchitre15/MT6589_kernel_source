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
 * 07 11 2011 jun.pei
 * [ALPS00059464] hi704 sensor check in
 * .
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

	//follow is define by jun
	/* SENSOR READ/WRITE ID */

#define HI704_IMAGE_SENSOR_QVGA_WIDTH       (320)
#define HI704_IMAGE_SENSOR_QVGA_HEIGHT      (240)
#define HI704_IMAGE_SENSOR_VGA_WIDTH        (640)
#define HI704_IMAGE_SENSOR_VGA_HEIGHT       (480)
#define HI704_IMAGE_SENSOR_SXGA_WIDTH       (1280)
#define HI704_IMAGE_SENSOR_SXGA_HEIGHT      (1024)

#define HI704_IMAGE_SENSOR_FULL_WIDTH	   HI704_IMAGE_SENSOR_VGA_WIDTH  
#define HI704_IMAGE_SENSOR_FULL_HEIGHT	   HI704_IMAGE_SENSOR_VGA_HEIGHT    

#define HI704_IMAGE_SENSOR_PV_WIDTH   HI704_IMAGE_SENSOR_VGA_WIDTH   
#define HI704_IMAGE_SENSOR_PV_HEIGHT  HI704_IMAGE_SENSOR_VGA_HEIGHT

//SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD
#define HI704_VGA_DEFAULT_PIXEL_NUMS		   (656)	
#define HI704_VGA_DEFAULT_LINE_NUMS 		   (500)

#define HI704_QVGA_DEFAULT_PIXEL_NUMS		   (656)	 
#define HI704_QVGA_DEFAULT_LINE_NUMS		   (254)

/* MAX/MIN FRAME RATE (FRAMES PER SEC.) */
#define HI704_MIN_FRAMERATE_5					(50)
#define HI704_MIN_FRAMERATE_7_5 				(75)
#define HI704_MIN_FRAMERATE_10					(100)
#define HI704_MIN_FRAMERATE_15                  (150)

//Video Fixed Framerate
#define HI704_VIDEO_FIX_FRAMERATE_5 			(50)
#define HI704_VIDEO_FIX_FRAMERATE_7_5			(75)
#define HI704_VIDEO_FIX_FRAMERATE_10			(100)
#define HI704_VIDEO_FIX_FRAMERATE_15			(150)
#define HI704_VIDEO_FIX_FRAMERATE_20			(200)
#define HI704_VIDEO_FIX_FRAMERATE_25			(250)
#define HI704_VIDEO_FIX_FRAMERATE_30			(300)


#define HI704_WRITE_ID		0x60
#define HI704_READ_ID		0x61

	//#define HI704_SCCB_SLAVE_ADDR 0x60

typedef struct _SENSOR_INIT_INFO
{
	  kal_uint8 address;
	  kal_uint8 data;
}HI704_SENSOR_INIT_INFO;
typedef enum __VIDEO_MODE__
{
	  HI704_VIDEO_NORMAL = 0,
	  HI704_VIDEO_MPEG4,	  
	  HI704_VIDEO_MAX
} HI704_VIDEO_MODE;

struct HI704_sensor_STRUCT
{    
      kal_bool first_init;
	  kal_bool pv_mode;                 //True: Preview Mode; False: Capture Mode
	  kal_bool night_mode;              //True: Night Mode; False: Auto Mode
	  kal_bool MPEG4_Video_mode;      //Video Mode: MJPEG or MPEG4
	  kal_uint8 mirror;
	  kal_uint32 pv_pclk;               //Preview Pclk
	  kal_uint32 cp_pclk;               //Capture Pclk
	  kal_uint16 pv_dummy_pixels;          //Dummy Pixels
	  kal_uint16 pv_dummy_lines;           //Dummy Lines
	  kal_uint16 cp_dummy_pixels;          //Dummy Pixels
	  kal_uint16 cp_dummy_lines;           //Dummy Lines         
	  kal_uint16 fix_framerate;         //Fixed Framerate
	  kal_uint32 wb;
	  kal_uint32 exposure;
	  kal_uint32 effect;
	  kal_uint32 banding;
	  kal_uint16 pv_line_length;
	  kal_uint16 pv_frame_height;
	  kal_uint16 cp_line_length;
	  kal_uint16 cp_frame_height;
	  kal_uint16 video_current_frame_rate;
};


//export functions
UINT32 HI704Open(void);
UINT32 HI704GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 HI704GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HI704Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HI704FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 HI704Close(void);


#endif /* __SENSOR_H */
