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
 * 11 16 2012 jianrong.zhang
 * [ALPS00361874] [Must Resolve][MT6517TD_AST3001][camcorder]preview play the video when set effects as choose your video
 * Copy from ALPS.ICS2.TDD.FPB.
 *
 * 03 31 2010 jianhua.tang
 * [DUMA00158728]S5K5CAGX YUV sensor driver 
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


//#define MIPI_INTERFACE

typedef enum _S5K5CAGX_OP_TYPE_ {
        S5K5CAGX_MODE_NONE,
        S5K5CAGX_MODE_PREVIEW,
        S5K5CAGX_MODE_CAPTURE,
        S5K5CAGX_MODE_QCIF_VIDEO,
        S5K5CAGX_MODE_CIF_VIDEO,
        S5K5CAGX_MODE_QVGA_VIDEO
    } S5K5CAGX_OP_TYPE;

extern S5K5CAGX_OP_TYPE S5K5CAGX_g_iS5K5CAGX_Mode;

//#define S5K5CAGX_QVGA_SIZE_PREVIEW

#define S5K5CAGX_PV_BEST_FRAME_RATE_BINNING
//#define S5K5CAGX_ADJ_H_V_BLANKING

// Config the IIC_ID & TST_PIN by specific module
//#define S5K5CAGX_IIC_ID_HIGH		// It is detemined by IIC_ID pin, 0: 78h, 1: 5Ah

// The active sate of STBYN is determined by the TST pin, TST pin state is sampled for STANDBY polarity when RSTN goes high.
// TST:0, STBYN is active low
// TST:1, STBYN is active high.
//#define S5K5CAGX_TST_PIN_HIGH 

// Reminder: Just can un-mask one macro of these 3 macros. if mask these 3 macros, then use mclk 24Mhz, pclk 48Mhz 
// for preview & capture
//#define S5K5CAGX_JPEG_MCLK12M_PCLK24M
//#define S5K5CAGX_JPEG_MCLK24M_PCLK36M 	
//#define S5K5CAGX_JPEG_MCLK24M_PCLK40M		

// If customer want to config the frame rate, please mask this macro.
#define S5K5CAGX_MTK_INTERNAL_USE

// Div 10 is the actual frame rate, 300 means 30fps, 75 means 7.5fps
#if (defined(S5K5CAGX_MTK_INTERNAL_USE))
	// Plz keep these setting un-changed
	#define S5K5CAGX_CAM_NOM_MAX_FPS				300		// 30fps
	#define S5K5CAGX_CAM_NOM_MIN_FPS				100		// 10fps
	#define S5K5CAGX_CAM_NIT_MAX_FPS				300		// 30fps
	#define S5K5CAGX_CAM_NIT_MIN_FPS				50		// 5.0fps

	/* It need double the sensor output frame if enable advance resizer because one for display & another for encode. */
		#define S5K5CAGX_VID_NOM_FIX_FPS			300		// 30fps
		#define S5K5CAGX_VID_NIT_FIX_FPS			150		// 15fps

#else
	// Customizable part.
	#define S5K5CAGX_CAM_NOM_MAX_FPS				160		// 16fps
	#define S5K5CAGX_CAM_NOM_MIN_FPS				75		// 7.5fps
	#define S5K5CAGX_CAM_NIT_MAX_FPS				160		// 16 fps
	#define S5K5CAGX_CAM_NIT_MIN_FPS				50		// 5.0fps

	/* It need double the sensor output frame if enable advance resizer because one for display & another for encode. */
		//#define S5K5CAGX_VID_NOM_FIX_FPS			300		// 30fps
		#define S5K5CAGX_VID_NOM_FIX_FPS			298		// 29.8fps Disable bayer downscale, just can reach 29.8fps.
		#define S5K5CAGX_VID_NIT_FIX_FPS			150		// 15ps
#endif

///////////////////////////////////////////////////////////////////////////////
//					Configuable macro End.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//					Notices: Please don't modify the macro below this line
///////////////////////////////////////////////////////////////////////////////


// Msec / 10 is the actual frame time, 1000 means 100ms.
#define S5K5CAGX_CAM_NOM_MIN_FR_TIME			((1000 * 10 * 10) / S5K5CAGX_CAM_NOM_MAX_FPS)
#define S5K5CAGX_CAM_NOM_MAX_FR_TIME			((1000 * 10 * 10) / S5K5CAGX_CAM_NOM_MIN_FPS)
#define S5K5CAGX_CAM_NIT_MIN_FR_TIME			((1000 * 10 * 10) / S5K5CAGX_CAM_NIT_MAX_FPS)
#define S5K5CAGX_CAM_NIT_MAX_FR_TIME			((1000 * 10 * 10) / S5K5CAGX_CAM_NIT_MIN_FPS)

#define S5K5CAGX_VID_NOM_FIX_FR_TIME			((1000 * 10 * 10) / S5K5CAGX_VID_NOM_FIX_FPS)
#define S5K5CAGX_VID_NIT_FIX_FR_TIME			((1000 * 10 * 10) / S5K5CAGX_VID_NIT_FIX_FPS)



	// It's for test only, must mark it when check in.
	//#define __SW_VIDEO_ADV_RESIZER_SUPPORT__

	/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	#define S5K5CAGX_SXGA_PERIOD_PIXEL_NUMS               2175         /* 2175 */
	#define S5K5CAGX_SXGA_PERIOD_LINE_NUMS                1607         /* 1607 */

	/* 3M RESOLUTION SIZE */
	#define S5K5CAGX_IMAGE_SENSOR_3M_WIDTH				2048
	#define S5K5CAGX_IMAGE_SENSOR_3M_HEIGHT				1536
	/* 2M RESOLUTION SIZE */
	#define S5K5CAGX_IMAGE_SENSOR_2M_WIDTH				1600
	#define S5K5CAGX_IMAGE_SENSOR_2M_HEIGHT				1200
	/* 1.3M RESOLUTION SIZE */
	#define S5K5CAGX_IMAGE_SENSOR_1_3M_WIDTH			1280
	#define S5K5CAGX_IMAGE_SENSOR_1_3M_HEIGHT			1024
	/* 1M RESOLUTION SIZE */
	#define S5K5CAGX_IMAGE_SENSOR_1M_WIDTH				1024
	#define S5K5CAGX_IMAGE_SENSOR_1M_HEIGHT				768
	/* SENSOR VGA SIZE */
	#define S5K5CAGX_IMAGE_SENSOR_VGA_WIDTH				640
	#define S5K5CAGX_IMAGE_SENSOR_VGA_HEIGHT			480
	/* SENSOR QVGA SIZE */
	#define S5K5CAGX_IMAGE_SENSOR_QVGA_WIDTH			320
	#define S5K5CAGX_IMAGE_SENSOR_QVGA_HEIGHT			240


	// Grab Window Setting for preview mode.
	#define S5K5CAGX_IMAGE_SENSOR_PV_INSERTED_PIXELS    	(2)
	#define S5K5CAGX_IMAGE_SENSOR_PV_INSERTED_LINES     	(2)
//#define S5K5CAGX_QVGA_SIZE_PREVIEW	
//Ken marked
#ifdef S5K5CAGX_QVGA_SIZE_PREVIEW
	#define S5K5CAGX_IMAGE_SENSOR_PV_WIDTH					(320)
	#define S5K5CAGX_IMAGE_SENSOR_PV_HEIGHT					(240)
#else
	#define S5K5CAGX_IMAGE_SENSOR_PV_WIDTH					(640-4)
	#define S5K5CAGX_IMAGE_SENSOR_PV_HEIGHT					(480-3)
#endif
	
	// Grab Window Setting for capture mode.
	#define S5K5CAGX_IMAGE_SENSOR_FULL_INSERTED_PIXELS		(2)
	#define S5K5CAGX_IMAGE_SENSOR_FULL_INSERTED_LINES		(2)
	
	//Ken marked
	#define S5K5CAGX_IMAGE_SENSOR_FULL_WIDTH			(2048-8)
	#define S5K5CAGX_IMAGE_SENSOR_FULL_HEIGHT			(1536-6)



/* DUMMY NEEDS TO BE INSERTED */
/* SETUP TIME NEED TO BE INSERTED */


/* SENSOR READ/WRITE ID */
	#define S5K5CAGX_IIC_ID_HIGH
#ifdef S5K5CAGX_IIC_ID_HIGH
	#define S5K5CAGX_WRITE_ID		0x5A		
	#define S5K5CAGX_READ_ID		0x5B
#else
	#define S5K5CAGX_WRITE_ID		0x78
	#define S5K5CAGX_READ_ID		0x79
#endif

#define S5K5CAGX_CAP_3_7FPS


	/* SENSOR CHIP VERSION */
	//#define S5K5CAGX_SENSOR_ID	  (0x05CA)	  // rev.2C

//e_porting add
//e_porting add
//e_porting add

//format index
#define S5K5CAGX_PV_FORMAT_INDEX   1 //1280x960 mode
#define S5K5CAGX_FULL_FORMAT_INDEX 2 //Full resolution mode

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
//#define CAM_SIZE_5M_WIDTH 		2592
//#define CAM_SIZE_5M_HEIGHT 		1944

//
//initial config function
//
int pfInitCfg_VGA(void);
int pfInitCfg_1280X960(void);
int pfInitCfg_FULL(void);

//export functions
UINT32 S5K5CAGXOpen(void);
UINT32 S5K5CAGXControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 S5K5CAGXFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 S5K5CAGXGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 S5K5CAGXGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 S5K5CAGXClose(void);

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT
//e_porting add
//e_porting add
//e_porting add

#endif /* __SENSOR_H */
