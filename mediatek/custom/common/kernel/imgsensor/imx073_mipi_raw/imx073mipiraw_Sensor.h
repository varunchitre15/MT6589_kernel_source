/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ov5642_Sensor.h
 *
 * Project:
 * --------
 *   YUSU
 *
 * Description:
 * ------------
 *   Header file of Sensor driver
 *
 *
 * Author:
 * -------
 *   Jackie Su (MTK02380)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 12 07 2011 koli.lin
 * [ALPS00030473] [Camera]
 * [Camera] Modify the custom tuning files for AE.
 *
 * 05 17 2011 koli.lin
 * [ALPS00048194] [Need Patch] [Volunteer Patch]
 * [Camera]. Chagne the preview size to 1600x1200 for IMX073 sensor.
 *
 * 04 01 2011 koli.lin
 * [ALPS00037670] [MPEG4 recording]the frame rate of fine quality video can not reach 30fps
 * [Camera]Modify the sensor preview output resolution and line time to fix frame rate at 30fps for video mode.
 *
 * 02 11 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Change sensor driver preview size ratio to 4:3.
 *
 * 02 11 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Modify the IMX073 sensor driver for preview mode.
 *
 * 02 11 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Create IMX073 sensor driver to database.
 *
 * 08 19 2010 ronnie.lai
 * [DUMA00032601] [Camera][ISP]
 * Merge dual camera relative settings. Main OV5642, SUB O7675 ready.
 *
 * 08 18 2010 ronnie.lai
 * [DUMA00032601] [Camera][ISP]
 * Mmodify ISP setting and add OV5642 sensor driver.
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


// Important Note:
//     1. Make sure horizontal PV sensor output is larger than IMX073MIPI_REAL_PV_WIDTH  + 2 * IMX073MIPI_IMAGE_SENSOR_PV_STARTX + 4.
//     2. Make sure vertical   PV sensor output is larger than IMX073MIPI_REAL_PV_HEIGHT + 2 * IMX073MIPI_IMAGE_SENSOR_PV_STARTY + 6.
//     3. Make sure horizontal CAP sensor output is larger than IMX073MIPI_REAL_CAP_WIDTH  + 2 * IMX073MIPI_IMAGE_SENSOR_CAP_STARTX + IMAGE_SENSOR_H_DECIMATION*4.
//     4. Make sure vertical   CAP sensor output is larger than IMX073MIPI_REAL_CAP_HEIGHT + 2 * IMX073MIPI_IMAGE_SENSOR_CAP_STARTY + IMAGE_SENSOR_V_DECIMATION*6.
// Note:
//     1. The reason why we choose REAL_PV_WIDTH/HEIGHT as tuning starting point is
//        that if we choose REAL_CAP_WIDTH/HEIGHT as starting point, then:
//            REAL_PV_WIDTH  = REAL_CAP_WIDTH  / IMAGE_SENSOR_H_DECIMATION
//            REAL_PV_HEIGHT = REAL_CAP_HEIGHT / IMAGE_SENSOR_V_DECIMATION
//        There might be some truncation error when dividing, which may cause a little view angle difference.
//Macro for Resolution
#define IMAGE_SENSOR_H_DECIMATION				2	// For current PV mode, take 1 line for every 2 lines in horizontal direction.
#define IMAGE_SENSOR_V_DECIMATION				2	// For current PV mode, take 1 line for every 2 lines in vertical direction.

/* Real PV Size, i.e. the size after all ISP processing (so already -4/-6), before MDP. */
#define IMX073MIPI_REAL_PV_WIDTH				(1600)
#define IMX073MIPI_REAL_PV_HEIGHT				(1200)

/* Real CAP Size, i.e. the size after all ISP processing (so already -4/-6), before MDP. */
#define IMX073MIPI_REAL_CAP_WIDTH				(IMX073MIPI_REAL_PV_WIDTH  * IMAGE_SENSOR_H_DECIMATION)
#define IMX073MIPI_REAL_CAP_HEIGHT				(IMX073MIPI_REAL_PV_HEIGHT * IMAGE_SENSOR_V_DECIMATION)

/* X/Y Starting point */
#define IMX073MIPI_IMAGE_SENSOR_PV_STARTX       8
#define IMX073MIPI_IMAGE_SENSOR_PV_STARTY       2	// The value must bigger or equal than 1.
#define IMX073MIPI_IMAGE_SENSOR_CAP_STARTX		(IMX073MIPI_IMAGE_SENSOR_PV_STARTX * IMAGE_SENSOR_H_DECIMATION)
#define IMX073MIPI_IMAGE_SENSOR_CAP_STARTY		(IMX073MIPI_IMAGE_SENSOR_PV_STARTY * IMAGE_SENSOR_V_DECIMATION)		// The value must bigger or equal than 1.

/* SENSOR 720P SIZE */
#define IMX073MIPI_IMAGE_SENSOR_PV_WIDTH		(IMX073MIPI_REAL_PV_WIDTH  + 2 * IMX073MIPI_IMAGE_SENSOR_PV_STARTX)	// 2*: Leave PV_STARTX unused space at both left side and right side of REAL_PV_WIDTH.	//(1620) //(820) //(1600)//(3272-8)
#define IMX073MIPI_IMAGE_SENSOR_PV_HEIGHT		(IMX073MIPI_REAL_PV_HEIGHT + 2 * IMX073MIPI_IMAGE_SENSOR_PV_STARTY)	// 2*: Leave PV_STARTY unused space at both top side and bottom side of REAL_PV_HEIGHT.	//(1220) //(612) //(1200)//(612)

/* SENSOR 8M SIZE */
#define IMX073MIPI_IMAGE_SENSOR_FULL_WIDTH		(IMX073MIPI_REAL_CAP_WIDTH  + 2 * IMX073MIPI_IMAGE_SENSOR_CAP_STARTX)	// 2*: Leave CAP_STARTX unused space at both left side and right side of REAL_CAP_WIDTH.	//3284
#define IMX073MIPI_IMAGE_SENSOR_FULL_HEIGHT		(IMX073MIPI_REAL_CAP_HEIGHT + 2 * IMX073MIPI_IMAGE_SENSOR_CAP_STARTY)	// 2*: Leave CAP_STARTY unused space at both top side and bottom side of REAL_CAP_HEIGHT.	//2462

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define IMX073MIPI_PV_PERIOD_PIXEL_NUMS			IMX073MIPI_IMAGE_SENSOR_PV_WIDTH	//(1620)	//(820)//(1600)//(3272-8)	// 720P mode's pixel # in one HSYNC w/o dummy pixels
#define IMX073MIPI_PV_PERIOD_LINE_NUMS			IMX073MIPI_IMAGE_SENSOR_PV_HEIGHT	//(1220)	//(612)//(1200)//(612)		// 720P mode's HSYNC # in one HSYNC w/o dummy lines
#define IMX073MIPI_FULL_PERIOD_PIXEL_NUMS		IMX073MIPI_IMAGE_SENSOR_FULL_WIDTH	//(3284)	// 8M mode's pixel # in one HSYNC w/o dummy pixels
#define IMX073MIPI_FULL_PERIOD_LINE_NUMS		IMX073MIPI_IMAGE_SENSOR_FULL_HEIGHT	//(2462)	// 8M mode's HSYNC # in one HSYNC w/o dummy lines

#define IMX073MIPI_PV_PERIOD_EXTRA_PIXEL_NUMS	18 // 4 //32
#define IMX073MIPI_PV_PERIOD_EXTRA_LINE_NUMS 34 // 17 //44 //500
#define IMX073MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS	36
#define IMX073MIPI_FULL_PERIOD_EXTRA_LINE_NUMS	36

#define IMX073MIPI_IMAGE_SENSOR_8M_PIXELS_LINE         3380 //(3292 + 168) //3292
#define IMX073MIPI_IMAGE_SENSOR_720P_PIXELS_LINE      1690 //1716 // 820 //1600 // 616

#define MAX_FRAME_RATE	(15)
#define MIN_FRAME_RATE  (12)

/* SENSOR EXPOSURE LINE LIMITATION */
#define IMX073MIPI_FULL_EXPOSURE_LIMITATION   (2496) // 8M mode
#define IMX073MIPI_PV_EXPOSURE_LIMITATION (1222) // (612 + 490) // (1254)  // # of lines in one 720P frame
// SENSOR VGA SIZE

#define IMX073MIPI_IMAGE_SENSOR_CCT_WIDTH		(3284)
#define IMX073MIPI_IMAGE_SENSOR_CCT_HEIGHT		(2462)

//For 2x Platform camera_para.c used
#define IMAGE_SENSOR_PV_WIDTH    IMX073MIPI_IMAGE_SENSOR_PV_WIDTH
#define IMAGE_SENSOR_PV_HEIGHT   IMX073MIPI_IMAGE_SENSOR_PV_HEIGHT

#define IMAGE_SENSOR_FULL_WIDTH	  IMX073MIPI_IMAGE_SENSOR_FULL_WIDTH
#define IMAGE_SENSOR_FULL_HEIGHT	  IMX073MIPI_IMAGE_SENSOR_FULL_HEIGHT

#define IMX073MIPI_SHUTTER_LINES_GAP	  3


#define IMX073MIPI_WRITE_ID (0x34)
#define IMX073MIPI_READ_ID	(0x35)

// SENSOR CHIP VERSION

#define IMX073MIPI_SENSOR_ID            IMX073_SENSOR_ID

#define IMX073MIPI_PAGE_SETTING_REG    (0xFF)

//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 IMX073MIPIOpen(void);
UINT32 IMX073MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 IMX073MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX073MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX073MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 IMX073MIPIClose(void);

//#define Sleep(ms) mdelay(ms)
//#define RETAILMSG(x,...)
//#define TEXT

//e_add for porting
//e_add for porting
//e_add for porting

#endif /* __SENSOR_H */

