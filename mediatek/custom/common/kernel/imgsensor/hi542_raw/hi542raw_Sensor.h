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
 * 04 20 2012 chengxue.shen
 * [ALPS00272900] HI542 Sensor Driver Check In
 * HI542 Sensor driver Check in and modify For MT6577 dual core processor
 *
 * 08 19 2010 ronnie.lai
 * [DUMA00032601] [Camera][ISP]
 * Merge dual camera relative settings. Main HI542, SUB O7675 ready.
 *
 * 08 18 2010 ronnie.lai
 * [DUMA00032601] [Camera][ISP]
 * Mmodify ISP setting and add HI542 sensor driver.
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



#define CURRENT_MAIN_SENSOR                HI542_OMNIVISION


//Macro for Resolution
#define HI542_IMAGE_SENSOR_CCT_WIDTH     (2592)
#define HI542_IMAGE_SENSOR_CCT_HEIGHT    (1944)
//#define HI542_IMAGE_SENSOR_CCT_WIDTH     (2592-32) // (2048)
//#define HI542_IMAGE_SENSOR_CCT_HEIGHT    (1944-24) //(1536)

/* SENSOR PREVIEW SIZE */
#define HI542_IMAGE_SENSOR_PV_WIDTH   (1280)
#define HI542_IMAGE_SENSOR_PV_HEIGHT  (960)
//#define HI542_IMAGE_SENSOR_PV_WIDTH   (1296-16)
//#define HI542_IMAGE_SENSOR_PV_HEIGHT  (972-12)

/* SENSOR 5M SIZE */
#define HI542_IMAGE_SENSOR_FULL_WIDTH                                       (2592)
#define HI542_IMAGE_SENSOR_FULL_HEIGHT                                      (1944)
//#define HI542_IMAGE_SENSOR_FULL_WIDTH     (2592-32) // (2048)
//#define HI542_IMAGE_SENSOR_FULL_HEIGHT    (1944-24) //(1536)

#define HI542_IMAGE_SENSOR_PV_STARTX                         1
#define HI542_IMAGE_SENSOR_PV_STARTY                         1
//#define HI542_IMAGE_SENSOR_PV_STARTX				2
//#define HI542_IMAGE_SENSOR_PV_STARTY				2

/*   notes  this 
preview active window : 1288*968
full active window : 2608*1960
default blanking :52*12
*/

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define HI542_FULL_PERIOD_PIXEL_NUMS  (2608)  // 5M mode's pixel # in one HSYNC w/o dummy pixels
#define HI542_FULL_PERIOD_LINE_NUMS   (1960)  // 5M mode's HSYNC # in one HSYNC w/o dummy lines
#define HI542_PV_PERIOD_PIXEL_NUMS   (1288)    // P mode's pixel # in one HSYNC w/o dummy pixels
#define HI542_PV_PERIOD_LINE_NUMS    (968)   // P mode's HSYNC # in one HSYNC w/o dummy lines

#define HI542_FULL_PERIOD_EXTRA_PIXEL_NUMS  52 //608 //450
#define HI542_FULL_PERIOD_EXTRA_LINE_NUMS 12 //56 //28
#define HI542_PV_PERIOD_EXTRA_PIXEL_NUMS 52 //450
#define HI542_PV_PERIOD_EXTRA_LINE_NUMS 12

#define HI542_IMAGE_SENSOR_FULL_PIXELS_LINE           2608//1940
#define HI542_IMAGE_SENSOR_PV_PIXELS_LINE           (1288)//970
    
#define MAX_FRAME_RATE	(15)
#define MIN_FRAME_RATE  (12)

/* SENSOR EXPOSURE LINE LIMITATION */
#define HI542_FULL_EXPOSURE_LIMITATION    (1944)  // 5M mode
#define HI542_PV_EXPOSURE_LIMITATION  (HI542_PV_PERIOD_LINE_NUMS)  // # of lines in one 720P frame
// SENSOR VGA SIZE
//For 2x Platform camera_para.c used
#define IMAGE_SENSOR_PV_WIDTH    HI542_IMAGE_SENSOR_PV_WIDTH
#define IMAGE_SENSOR_PV_HEIGHT   HI542_IMAGE_SENSOR_PV_HEIGHT

#define IMAGE_SENSOR_FULL_WIDTH	  HI542_IMAGE_SENSOR_FULL_WIDTH
#define IMAGE_SENSOR_FULL_HEIGHT	  HI542_IMAGE_SENSOR_FULL_HEIGHT

#define HI542_SHUTTER_LINES_GAP	  0


#define HI542_WRITE_ID (0x40)
#define HI542_READ_ID	(0x41)

// SENSOR CHIP VERSION

//#define HI542_SENSOR_ID            0x5642

#define HI542_PAGE_SETTING_REG    (0xFF)

//s_add for porting
//s_add for porting
//s_add for porting

//#define HI542_SENSOR_ID    HI542_SENSOR_ID
//#define HI542_SENSOR_ID1   HI542_SENSOR_ID_1
//#define HI542_SENSOR_ID2   HI542_SENSOR_ID_2
//#define HI542_WRITE_ID     HI542_WRITE_ID

//export functions
UINT32 HI542Open(void);
UINT32 HI542GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 HI542GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HI542Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 HI542FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 HI542Close(void);

//#define Sleep(ms) mdelay(ms)
//#define RETAILMSG(x,...)
//#define TEXT

//e_add for porting
//e_add for porting
//e_add for porting

#endif /* __SENSOR_H */

