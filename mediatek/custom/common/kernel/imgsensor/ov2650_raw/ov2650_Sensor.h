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
 * 09 10 2010 jackie.su
 * [ALPS00002279] [Need Patch] [Volunteer Patch] ALPS.Wxx.xx Volunteer patch for
 * .alps dual sensor
 *
 * 09 02 2010 jackie.su
 * [ALPS00002279] [Need Patch] [Volunteer Patch] ALPS.Wxx.xx Volunteer patch for
 * .roll back dual sensor
 *
 * Dec 31 2009 mtk70508
 * [DUMA00149823] check in sensor driver for new
 *
 *
 * Dec 21 2009 mtk70508
 * [DUMA00147177] Winmo sensor  and lens driver  modification
 *
 *
 * Nov 24 2009 mtk02204
 * [DUMA00015869] [Camera Driver] Modifiy camera related drivers for dual/backup sensor/lens drivers.
 *
 *
 *    mtk70508
 * [DUMA00126076] OV2650 sensor driver check in
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

typedef enum group_enum {
	   PRE_GAIN=0,
	   CMMCLK_CURRENT,
	   FRAME_RATE_LIMITATION,
	   REGISTER_EDITOR,
	   GROUP_TOTAL_NUMS
} FACTORY_GROUP_ENUM;


#define ENGINEER_START_ADDR 10
#define FACTORY_START_ADDR 0

typedef enum engineer_index
{
	CMMCLK_CURRENT_INDEX=ENGINEER_START_ADDR,
	ENGINEER_END
} FACTORY_ENGINEER_INDEX;



typedef enum register_index
{
	PRE_GAIN_INDEX=FACTORY_START_ADDR,
	GLOBAL_GAIN_INDEX,
	FACTORY_END_ADDR
} FACTORY_REGISTER_INDEX;

typedef struct
{
	SENSOR_REG_STRUCT	Reg[ENGINEER_END];
   	SENSOR_REG_STRUCT	CCT[FACTORY_END_ADDR];
} SENSOR_DATA_STRUCT, *PSENSOR_DATA_STRUCT;



#define CURRENT_MAIN_SENSOR				OV2650_OMNIVISION


//Macro for Resolution
#define OV2650_IMAGE_SENSOR_CCT_WIDTH				(1600)
#define OV2650_IMAGE_SENSOR_CCT_HEIGHT				(1200)
/* SENSOR VGA SIZE */

//s_porting add
#if 1
#define OV2650_IMAGE_SENSOR_PV_WIDTH					(800)
#define OV2650_IMAGE_SENSOR_PV_HEIGHT					(600-8)
#else
#define OV2650_IMAGE_SENSOR_PV_WIDTH					(800)
#define OV2650_IMAGE_SENSOR_PV_HEIGHT					(600)
#endif
//e_porting add

/* SENSOR SXGA SIZE */
#define OV2650_IMAGE_SENSOR_FULL_WIDTH				(1600)
#define OV2650_IMAGE_SENSOR_FULL_HEIGHT				(1200)

/* START GRAB PIXEL OFFSET */
#define IMAGE_SENSOR_START_GRAB_X		        2      //Sean Change to 2 //0	// 0 or 1 recommended
#define IMAGE_SENSOR_START_GRAB_Y		        2 	  //Sean Change to 2 //0 // 0 or 1 recommended

//s_porting add
#if 1
#define OV2650_IMAGE_SENSOR_PV_STARTX				0
#define OV2650_IMAGE_SENSOR_PV_STARTY				0
#else
#define OV2650_IMAGE_SENSOR_PV_STARTX				8
#define OV2650_IMAGE_SENSOR_PV_STARTY				6
#endif
//e_porting add

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */

//s_porting add
#if 1
#define OV2650_FULL_PERIOD_PIXEL_NUMS			        1940
#define OV2650_FULL_PERIOD_LINE_NUMS					1237
#define OV2650_PV_PERIOD_PIXEL_NUMS						970//
#define OV2650_PV_PERIOD_LINE_NUMS						670//618//
#else
#define OV2650_FULL_PERIOD_PIXEL_NUMS			        1940
#define OV2650_FULL_PERIOD_LINE_NUMS					1236
#define OV2650_PV_PERIOD_PIXEL_NUMS						1000 //970//
#define OV2650_PV_PERIOD_LINE_NUMS						650//618//
#endif
//e_porting add

#define OV2650_IMAGE_SENSOR_UXGA_PIXELS_LINE           1940//1940
#define OV2650_IMAGE_SENSOR_SXGA_PIXELS_LINE           1000//970


#define MAX_FRAME_RATE	(15)	// Limitation for MPEG4 Encode Only
#define MIN_FRAME_RATE  (12)	  // Limitation for Camera Preiview


/* SENSOR EXPOSURE LINE LIMITATION */
#define OV2650_FULL_EXPOSURE_LIMITATION	 (1248)
#define OV2650_PV_EXPOSURE_LIMITATION 	 (672)

  // SENSOR VGA SIZE

//For 2x Platform camera_para.c used
#define IMAGE_SENSOR_PV_WIDTH    OV2650_IMAGE_SENSOR_PV_WIDTH
#define IMAGE_SENSOR_PV_HEIGHT   OV2650_IMAGE_SENSOR_PV_HEIGHT



#define IMAGE_SENSOR_FULL_WIDTH	  OV2650_IMAGE_SENSOR_FULL_WIDTH
#define IMAGE_SENSOR_FULL_HEIGHT	  OV2650_IMAGE_SENSOR_FULL_HEIGHT

#define OV2650_SHUTTER_LINES_GAP	  0


#define OV2650_WRITE_ID (0x60)
#define OV2650_READ_ID	(0x61)

// SENSOR CHIP VERSION

//#define OV2655_SENSOR_ID                          0x2656
//#define OV2650_SENSOR_ID_1                          0x2651
//#define OV2650_SENSOR_ID_2                          0x2652
//#define OV2650_SENSOR_ID_3			0x2655

#define OV2650_PAGE_SETTING_REG    (0xFF)



//s_add for porting
//s_add for porting
//s_add for porting

//#define OV265X_SENSOR_ID    OV2655_SENSOR_ID
//#define OV265X_SENSOR_ID1  OV2650_SENSOR_ID_1
//#define OV265X_SENSOR_ID2  OV2650_SENSOR_ID_2
//#define OV265X_WRITE_ID     OV2650_WRITE_ID

//export functions
UINT32 OV2650Open(void);
UINT32 OV2650GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV2650GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV2650Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV2650FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 OV2650Close(void);

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

//e_add for porting
//e_add for porting
//e_add for porting

#endif /* __SENSOR_H */


