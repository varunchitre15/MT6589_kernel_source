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
 * 03 31 2010 jianhua.tang
 * [DUMA00158728]BF3905 YUV sensor driver 
 * .
 *
 * Feb 24 2010 mtk01118
 * [DUMA00025869] [Camera][YUV I/F & Query feature] check in camera code
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

#define MIPI_INTERFACE  

typedef enum _BF3905_MIPI_OP_TYPE_ {
        BF3905_MIPI_MODE_NONE,
        BF3905_MIPI_MODE_PREVIEW,
        BF3905_MIPI_MODE_CAPTURE,
        BF3905_MIPI_MODE_QCIF_VIDEO,
        BF3905_MIPI_MODE_CIF_VIDEO,
        BF3905_MIPI_MODE_QVGA_VIDEO
    } BF3905_MIPI_OP_TYPE;

extern BF3905_MIPI_OP_TYPE BF3905_MIPI_g_iBF3905_MIPI_Mode;

//#define BF3905_MIPI_QVGA_SIZE_PREVIEW

#define BF3905_MIPI_PV_BEST_FRAME_RATE_BINNING
//#define BF3905_MIPI_ADJ_H_V_BLANKING

// The active sate of STBYN is determined by the TST pin, TST pin state is sampled for STANDBY polarity when RSTN goes high.
// TST:0, STBYN is active low
// TST:1, STBYN is active high.
//#define BF3905_MIPI_TST_PIN_HIGH 

// Reminder: Just can un-mask one macro of these 3 macros. if mask these 3 macros, then use mclk 24Mhz, pclk 48Mhz 
// for preview & capture
//#define BF3905_MIPI_JPEG_MCLK12M_PCLK24M
//#define BF3905_MIPI_JPEG_MCLK24M_PCLK36M 	
//#define BF3905_MIPI_JPEG_MCLK24M_PCLK40M		

// Div 10 is the actual frame rate, 300 means 30fps, 75 means 7.5fps
	// Plz keep these setting un-changed
	#define BF3905_MIPI_CAM_NOM_MAX_FPS				30		// 30fps
	#define BF3905_MIPI_CAM_NOM_MIN_FPS				10		// 10.0fps
	#define BF3905_MIPI_CAM_NIT_MAX_FPS				30		// 30fps
	#define BF3905_MIPI_CAM_NIT_MIN_FPS				5		// 5.0fps


	#define BF3905_MIPI_VID_NOM_FIX_FPS			30		// 30fps

	#define BF3905_MIPI_VID_NIT_FIX_FPS			15		// 15fps

///////////////////////////////////////////////////////////////////////////////
//					Configuable macro End.
//					Notices: Please don't modify the macro below this line
///////////////////////////////////////////////////////////////////////////////


// Msec / 10 is the actual frame time, 1000 means 100ms.
#define BF3905_MIPI_CAM_NOM_MIN_FR_TIME			((100) / BF3905_MIPI_CAM_NOM_MAX_FPS)
#define BF3905_MIPI_CAM_NOM_MAX_FR_TIME			((100) / BF3905_MIPI_CAM_NOM_MIN_FPS)
#define BF3905_MIPI_CAM_NIT_MIN_FR_TIME			((100) / BF3905_MIPI_CAM_NIT_MAX_FPS)
#define BF3905_MIPI_CAM_NIT_MAX_FR_TIME			((100) / BF3905_MIPI_CAM_NIT_MIN_FPS)

#define BF3905_MIPI_VID_NOM_FIX_FR_TIME			((100) / BF3905_MIPI_VID_NOM_FIX_FPS)
#define BF3905_MIPI_VID_NIT_FIX_FR_TIME			((100) / BF3905_MIPI_VID_NIT_FIX_FPS)



// It's for test only, must mark it when check in.
//#define __SW_VIDEO_ADV_RESIZER_SUPPORT__

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS               (784+16)         /* 800 */
#define BF3905_MIPI_VGA_PERIOD_LINE_NUMS                (510)            /*510 */

/* 3M RESOLUTION SIZE */
#define BF3905_MIPI_IMAGE_SENSOR_3M_WIDTH				2048
#define BF3905_MIPI_IMAGE_SENSOR_3M_HEIGHT				1536
/* 2M RESOLUTION SIZE */
#define BF3905_MIPI_IMAGE_SENSOR_2M_WIDTH				1600
#define BF3905_MIPI_IMAGE_SENSOR_2M_HEIGHT				1200
/* 1.3M RESOLUTION SIZE */
#define BF3905_MIPI_IMAGE_SENSOR_1_3M_WIDTH			1280
#define BF3905_MIPI_IMAGE_SENSOR_1_3M_HEIGHT			1024
/* 1.2M RESOLUTION SIZE */
#define BF3905_MIPI_IMAGE_SENSOR_1_2M_WIDTH			1280
#define BF3905_MIPI_IMAGE_SENSOR_1_2M_HEIGHT			960
/* 1M RESOLUTION SIZE */
#define BF3905_MIPI_IMAGE_SENSOR_1M_WIDTH				1024
#define BF3905_MIPI_IMAGE_SENSOR_1M_HEIGHT				768
/* SENSOR VGA SIZE */
#define BF3905_MIPI_IMAGE_SENSOR_VGA_WIDTH				640
#define BF3905_MIPI_IMAGE_SENSOR_VGA_HEIGHT			480
/* SENSOR QVGA SIZE */
#define BF3905_MIPI_IMAGE_SENSOR_QVGA_WIDTH			320
#define BF3905_MIPI_IMAGE_SENSOR_QVGA_HEIGHT			240


// Grab Window Setting for preview mode.
#define BF3905_MIPI_IMAGE_SENSOR_PV_INSERTED_PIXELS    	(2)
#define BF3905_MIPI_IMAGE_SENSOR_PV_INSERTED_LINES     	(2)

#define BF3905_MIPI_IMAGE_SENSOR_PV_WIDTH	 				(640)
#define BF3905_MIPI_IMAGE_SENSOR_PV_HEIGHT					(480)
	
// Grab Window Setting for capture mode.
#define BF3905_MIPI_IMAGE_SENSOR_FULL_INSERTED_PIXELS		(2)
#define BF3905_MIPI_IMAGE_SENSOR_FULL_INSERTED_LINES		(2)
	
#define BF3905_MIPI_IMAGE_SENSOR_FULL_WIDTH			(640)
#define BF3905_MIPI_IMAGE_SENSOR_FULL_HEIGHT			(480)



/* DUMMY NEEDS TO BE INSERTED */
/* SETUP TIME NEED TO BE INSERTED */


/* SENSOR READ/WRITE ID */

#define BF3905_MIPI_WRITE_ID		0xDC		
#define BF3905_MIPI_READ_ID		0xDD


//format index
#define BF3905_MIPI_PV_FORMAT_INDEX   1 
#define BF3905_MIPI_FULL_FORMAT_INDEX 2 

//customize
#define CAM_SIZE_QVGA_WIDTH 	320
#define CAM_SIZE_QVGA_HEIGHT 	240
#define CAM_SIZE_VGA_WIDTH 		640
#define CAM_SIZE_VGA_HEIGHT 	480
#define CAM_SIZE_05M_WIDTH 		800
#define CAM_SIZE_05M_HEIGHT 	600
#define CAM_SIZE_1M_WIDTH 		1280
#define CAM_SIZE_1M_HEIGHT 		960
#define CAM_SIZE_2M_WIDTH 		1600
#define CAM_SIZE_2M_HEIGHT 		1200
#define CAM_SIZE_3M_WIDTH 		2048
#define CAM_SIZE_3M_HEIGHT 		1536
#define CAM_SIZE_5M_WIDTH 		2592
#define CAM_SIZE_5M_HEIGHT 		1944

//
//initial config function
//
int pfInitCfg_VGA(void);
int pfInitCfg_1280X960(void);
int pfInitCfg_FULL(void);

//export functions
UINT32 BF3905_MIPIOpen(void);
UINT32 BF3905_MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 BF3905_MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 BF3905_MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 BF3905_MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 BF3905_MIPIClose(void);

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

#endif /* __SENSOR_H */
