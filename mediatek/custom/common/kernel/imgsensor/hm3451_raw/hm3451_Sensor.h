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
 * 04 26 2012 guoqing.liu
 * [ALPS00271165] [FPB&ICS Done]modify sensor driver for MT6577
 * add new sensor driver for mt6577.
 *
 * 10 12 2011 jun.pei
 * [ALPS00065936] hm3451 check in patch
 * .
 *
 * 09 10 2010 jackie.su
 * [ALPS00002279] [Need Patch] [Volunteer Patch] ALPS.Wxx.xx Volunteer patch for
 * .10y dual sensor
 *
 * 09 02 2010 jackie.su
 * [ALPS00002279] [Need Patch] [Volunteer Patch] ALPS.Wxx.xx Volunteer patch for
 * .roll back dual sensor
 *
 * 05 05 2010 sean.cheng
 * [ALPS00005476][Performance][Camera] Camera startup time is slow 
 * .
 * Decrease the sensor init time from 200ms to 30ms
 *
 * Dec 21 2009 mtk70508
 * [DUMA00147177] Winmo sensor  and lens driver  modification
 *
 *
 * Aug 5 2009 mtk01051
 * [DUMA00009217] [Camera Driver] CCAP First Check In
 *
 *
 * Apr 7 2009 mtk02204
 * [DUMA00004012] [Camera] Restructure and rename camera related custom folders and folder name of came
 *
 *
 * Mar 26 2009 mtk02204
 * [DUMA00003515] [PC_Lint] Remove PC_Lint check warnings of camera related drivers.
 *
 *
 * Mar 2 2009 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 *
 *
 * Feb 24 2009 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 *
 *
 * Dec 27 2008 MTK01813
 * DUMA_MBJ CheckIn Files
 * created by clearfsimport
 *
 * Dec 10 2008 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 *
 *
 * Oct 27 2008 mtk01051
 * [DUMA00000851] Camera related drivers check in
 * Modify Copyright Header
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H


//
//initial config function
//
int pfInitCfg_VGA(void);
int pfInitCfg_1280X960(void);
int pfInitCfg_FULL(void);

//export functions
UINT32 HM3451Open(void);
UINT32 HM3451Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HM3451FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 HM3451GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HM3451GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 HM3451Close(void);

//#define Sleep(ms) mdelay(ms)
#define Sleep(us) udelay(us)

#define RETAILMSG(x,...)
#define TEXT



//to do :below is defined by jun

/*SENSOR READ/WRITE ID
  SADR = L, use ID_1
  SADR = H, use ID_2 (HM3451)
*/
#define HM3451_WRITE_ID_1									0x48
#define HM3451_READ_ID_1									0x49

#define HM3451_WRITE_ID_2									0x68
#define HM3451_READ_ID_2									0x69

//SENSOR CHIP VERSION
//#define HM3451_SENSOR_ID											0x3451

#define FACTORY_START_ADDR 	0
#define ENGINEER_START_ADDR	10

//For HM3451 CCT Function
//#define CURRENT_MAIN_SENSOR			HM3451_HIMAX
//#define HM3451_FIRST_GRAB_COLOR 		BAYER_B 


typedef enum group_enum {
	   PRE_GAIN=0,
	   CMMCLK_CURRENT,
	   FRAME_RATE_LIMITATION,
	   REGISTER_EDITOR,
	   GROUP_TOTAL_NUMS
} FACTORY_GROUP_ENUM;

typedef enum register_index
{
	SENSOR_BASEGAIN=FACTORY_START_ADDR,
	PRE_GAIN_R_INDEX,
	PRE_GAIN_Gr_INDEX,
	PRE_GAIN_Gb_INDEX,
	PRE_GAIN_B_INDEX,
	FACTORY_END_ADDR
} FACTORY_REGISTER_INDEX;

typedef enum engineer_index
{
	CMMCLK_CURRENT_INDEX=ENGINEER_START_ADDR,
	ENGINEER_END
} FACTORY_ENGINEER_INDEX;

typedef struct
{
	SENSOR_REG_STRUCT	Reg[ENGINEER_END];
   	SENSOR_REG_STRUCT	CCT[FACTORY_END_ADDR];
} SENSOR_DATA_STRUCT, *PSENSOR_DATA_STRUCT;


typedef enum __VIDEO_MODE__
{
	  HM3451_VIDEO_NORMAL = 0,
	  HM3451_VIDEO_MPEG4,	  
	  HM3451_VIDEO_MAX
} HM3451_VIDEO_MODE;


	struct HM3451_sensor_STRUCT
	{	 
		  kal_uint16 i2c_write_id;
		  kal_uint16 i2c_read_id;
		  kal_bool fix_video_fps;
		  kal_bool pv_mode; 				//True: Preview Mode; False: Capture Mode
		  kal_bool night_mode;				//True: Night Mode; False: Auto Mode
		  kal_bool MPEG4_Video_mode;	  //Video Mode: MJPEG or MPEG4
		  kal_uint8 mirror_flip;
		  kal_uint32 pv_pclk;				//Preview Pclk
		  kal_uint32 cp_pclk;				//Capture Pclk
		  kal_uint32 pv_shutter;		   
		  kal_uint32 cp_shutter;
		  kal_uint32 pv_gain;
		  kal_uint32 cp_gain;
		  kal_uint32 pv_line_length;
		  kal_uint32 pv_frame_length;
		  kal_uint32 cp_line_length;
		  kal_uint32 cp_frame_length;
		  kal_uint16 pv_dummy_pixels;		   //Dummy Pixels:must be 12s
		  kal_uint16 pv_dummy_lines;		   //Dummy Lines
		  kal_uint16 cp_dummy_pixels;		   //Dummy Pixels:must be 12s
		  kal_uint16 cp_dummy_lines;		   //Dummy Lines			
		  kal_uint16 video_current_frame_rate;
	};



	// SENSOR EXPOSURE LINE LIMITATION
	#define HM3451_FULL_EXPOSURE_LIMITATION 					1572 
	#define HM3451_PV_EXPOSURE_LIMITATION						628

	//IMAGE SIZE
	#define HM3451_IMAGE_SENSOR_PV_WIDTH						(1024 - 16)//(1024 - 8)
	#define HM3451_IMAGE_SENSOR_PV_HEIGHT						(768 - 12)//(768 - 6)
	#define HM3451_IMAGE_SENSOR_FULL_WIDTH						(2048 - 16-16)
	#define HM3451_IMAGE_SENSOR_FULL_HEIGHT						(1536 - 12-12)

    //Capture Size:
    #define HM3451_IMAGE_SENSOR_3M_WIDTH                        (2048)
    #define HM3451_IMAGE_SENSOR_3M_HEIGHT                       (1536)
    #define HM3451_IMAGE_SENSOR_5M_WIDTH                        (2592)
    #define HM3451_IMAGE_SENSOR_5M_HEIGHT                       (1944)
    
	//USED BY AE TABLE
 	#define IMAGE_SENSOR_FULL_WIDTH								HM3451_IMAGE_SENSOR_FULL_WIDTH

	//SENSOR HORIZONTAL/VERTICAL BLANKING	
	#define HM3451_IMAGE_SENSOR_PV_DEFAULT_HBLANKING			690//(762 + 46) 	//Default +46
	#define HM3451_IMAGE_SENSOR_PV_DEFAULT_VBLANKING			16//(16 + 2) //(16+6)
	
	#define HM3451_IMAGE_SENSOR_FULL_DEFAULT_HBLANKING 			690//(762 + 46) 	//Default +46
	#define HM3451_IMAGE_SENSOR_FULL_DEFAULT_VBLANKING 			16//(16 + 2)	//(16+6)	
	
	//SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD
	#define HM3451_PV_ACTIVE_PIXEL_NUMS 						1092
	#define HM3451_PV_ACTIVE_LINE_NUMS							780//792  need to check

	#define HM3451_FULL_ACTIVE_PIXEL_NUMS						2184
	#define HM3451_FULL_ACTIVE_LINE_NUMS						1560

	#define HM3451_PV_PERIOD_PIXEL_NUMS 						(HM3451_PV_ACTIVE_PIXEL_NUMS+HM3451_IMAGE_SENSOR_PV_DEFAULT_HBLANKING)
	#define HM3451_PV_PERIOD_LINE_NUMS							(HM3451_PV_ACTIVE_LINE_NUMS+HM3451_IMAGE_SENSOR_PV_DEFAULT_VBLANKING)

	#define HM3451_FULL_PERIOD_PIXEL_NUMS						(HM3451_FULL_ACTIVE_PIXEL_NUMS+HM3451_IMAGE_SENSOR_FULL_DEFAULT_HBLANKING)
	#define HM3451_FULL_PERIOD_LINE_NUMS						(HM3451_FULL_ACTIVE_LINE_NUMS+HM3451_IMAGE_SENSOR_FULL_DEFAULT_VBLANKING)


#endif /* __SENSOR_H */

