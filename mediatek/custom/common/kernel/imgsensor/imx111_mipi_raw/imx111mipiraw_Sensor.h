/*****************************************************************************
 *
 * Filename:
 * ---------
 *   imx111mipiraw_sensor.h
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
	SENSOR_BASEGAIN=FACTORY_START_ADDR,
	PRE_GAIN_R_INDEX,
	PRE_GAIN_Gr_INDEX,
	PRE_GAIN_Gb_INDEX,
	PRE_GAIN_B_INDEX,
	FACTORY_END_ADDR
} FACTORY_REGISTER_INDEX;



typedef struct
{
    SENSOR_REG_STRUCT	Reg[ENGINEER_END];
    SENSOR_REG_STRUCT	CCT[FACTORY_END_ADDR];
} SENSOR_DATA_STRUCT, *PSENSOR_DATA_STRUCT;


// Important Note:
//     1. Make sure horizontal PV sensor output is larger than IMX111MIPI_REAL_PV_WIDTH  + 2 * IMX111MIPI_IMAGE_SENSOR_PV_STARTX + 4.
//     2. Make sure vertical   PV sensor output is larger than IMX111MIPI_REAL_PV_HEIGHT + 2 * IMX111MIPI_IMAGE_SENSOR_PV_STARTY + 6.
//     3. Make sure horizontal CAP sensor output is larger than IMX111MIPI_REAL_CAP_WIDTH  + 2 * IMX111MIPI_IMAGE_SENSOR_CAP_STARTX + IMAGE_SENSOR_H_DECIMATION*4.
//     4. Make sure vertical   CAP sensor output is larger than IMX111MIPI_REAL_CAP_HEIGHT + 2 * IMX111MIPI_IMAGE_SENSOR_CAP_STARTY + IMAGE_SENSOR_V_DECIMATION*6.
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



#define IMX111MIPI_REAL_PV_WIDTH				1600//(1600)
#define IMX111MIPI_REAL_PV_HEIGHT				1200//(1200)
/* Real CAP Size, i.e. the size after all ISP processing (so already -4/-6), before MDP. */
#define IMX111MIPI_REAL_CAP_WIDTH				(IMX111MIPI_REAL_PV_WIDTH  * IMAGE_SENSOR_H_DECIMATION)
#define IMX111MIPI_REAL_CAP_HEIGHT				(IMX111MIPI_REAL_PV_HEIGHT * IMAGE_SENSOR_V_DECIMATION)
#define IMX111MIPI_REAL_VIDEO_WIDTH				1920
#define IMX111MIPI_REAL_VIDEO_HEIGHT			1080


/* X/Y Starting point */
#define IMX111MIPI_IMAGE_SENSOR_PV_STARTX       2
#define IMX111MIPI_IMAGE_SENSOR_PV_STARTY       2	// The value must bigger or equal than 1.
#define IMX111MIPI_IMAGE_SENSOR_CAP_STARTX		(IMX111MIPI_IMAGE_SENSOR_PV_STARTX * IMAGE_SENSOR_H_DECIMATION)
#define IMX111MIPI_IMAGE_SENSOR_CAP_STARTY		(IMX111MIPI_IMAGE_SENSOR_PV_STARTY * IMAGE_SENSOR_V_DECIMATION)		// The value must bigger or equal than 1.
#define IMX111MIPI_IMAGE_SENSOR_VIDEO_STARTX       2
#define IMX111MIPI_IMAGE_SENSOR_VIDEO_STARTY       2	// The value must bigger or equal than 1.

/* SENSOR 2M SIZE */
#define IMX111MIPI_IMAGE_SENSOR_PV_WIDTH		(IMX111MIPI_REAL_PV_WIDTH  + 2 * IMX111MIPI_IMAGE_SENSOR_PV_STARTX)	// 2*: Leave PV_STARTX unused space at both left side and right side of REAL_PV_WIDTH.	//(1620) //(820) //(1600)//(3272-8)
#define IMX111MIPI_IMAGE_SENSOR_PV_HEIGHT		(IMX111MIPI_REAL_PV_HEIGHT + 2 * IMX111MIPI_IMAGE_SENSOR_PV_STARTY)	// 2*: Leave PV_STARTY unused space at both top side and bottom side of REAL_PV_HEIGHT.	//(1220) //(612) //(1200)//(612)
/* SENSOR 8M SIZE */
#define IMX111MIPI_IMAGE_SENSOR_FULL_WIDTH		(IMX111MIPI_REAL_CAP_WIDTH  + 2 * IMX111MIPI_IMAGE_SENSOR_CAP_STARTX)	// 2*: Leave CAP_STARTX unused space at both left side and right side of REAL_CAP_WIDTH.	//3284
#define IMX111MIPI_IMAGE_SENSOR_FULL_HEIGHT		(IMX111MIPI_REAL_CAP_HEIGHT + 2 * IMX111MIPI_IMAGE_SENSOR_CAP_STARTY)	// 2*: Leave CAP_STARTY unused space at both top side and bottom side of REAL_CAP_HEIGHT.	//2462


#define IMX111MIPI_IMAGE_SENSOR_VIDEO_WIDTH		(IMX111MIPI_REAL_VIDEO_WIDTH  + 2 * IMX111MIPI_IMAGE_SENSOR_VIDEO_STARTX)	// 2*: Leave CAP_STARTX unused space at both left side and right side of REAL_CAP_WIDTH.	//3284
#define IMX111MIPI_IMAGE_SENSOR_VIDEO_HEIGHT	(IMX111MIPI_REAL_VIDEO_HEIGHT + 2 * IMX111MIPI_IMAGE_SENSOR_VIDEO_STARTY)	// 2*: Leave CAP_STARTY unused space at both top side and bottom side of REAL_CAP_HEIGHT.	//2462

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define IMX111MIPI_PV_PERIOD_PIXEL_NUMS			IMX111MIPI_IMAGE_SENSOR_PV_WIDTH	//(1620)	//(820)//(1600)//(3272-8)	// 720P mode's pixel # in one HSYNC w/o dummy pixels
#define IMX111MIPI_PV_PERIOD_LINE_NUMS			IMX111MIPI_IMAGE_SENSOR_PV_HEIGHT	//(1220)	//(612)//(1200)//(612)		// 720P mode's HSYNC # in one HSYNC w/o dummy lines
#define IMX111MIPI_FULL_PERIOD_PIXEL_NUMS		IMX111MIPI_IMAGE_SENSOR_FULL_WIDTH	//(3284)	// 8M mode's pixel # in one HSYNC w/o dummy pixels
#define IMX111MIPI_FULL_PERIOD_LINE_NUMS		IMX111MIPI_IMAGE_SENSOR_FULL_HEIGHT	//(2462)	// 8M mode's HSYNC # in one HSYNC w/o dummy lines
#define IMX111MIPI_VIDEO_PERIOD_PIXEL_NUMS		IMX111MIPI_REAL_VIDEO_WIDTH	//(1620)	//(820)//(1600)//(3272-8)	// 720P mode's pixel # in one HSYNC w/o dummy pixels
#define IMX111MIPI_VIDEO_PERIOD_LINE_NUMS		IMX111MIPI_REAL_VIDEO_HEIGHT	//(1220)	//(612)//(1200)//(612)		// 720P mode's HSYNC # in one HSYNC w/o dummy lines




#define IMX111MIPI_PV_PERIOD_EXTRA_PIXEL_NUMS	18 // 4 //32
#define IMX111MIPI_PV_PERIOD_EXTRA_LINE_NUMS	34 // 17 //44 //500
#define IMX111MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS	36
#define IMX111MIPI_FULL_PERIOD_EXTRA_LINE_NUMS	36

#define IMX111MIPI_IMAGE_SENSOR_8M_PIXELS_LINE         (3292 + 168)
#define IMX111MIPI_IMAGE_SENSOR_720P_PIXELS_LINE      1690 //1716 // 820 //1600 // 616

#define MAX_FRAME_RATE	(15)
#define MIN_FRAME_RATE  (12)

/* SENSOR EXPOSURE LINE LIMITATION */
#define IMX111MIPI_FULL_EXPOSURE_LIMITATION   (2496) // 8M mode
#define IMX111MIPI_PV_EXPOSURE_LIMITATION (1222) // (612 + 490) // (1254)  // # of lines in one 720P frame
// SENSOR VGA SIZE

#define IMX111MIPI_IMAGE_SENSOR_CCT_WIDTH		(3284)
#define IMX111MIPI_IMAGE_SENSOR_CCT_HEIGHT		(2462)

#define IMX111MIPI_PV_LINE_LENGTH_PIXELS 						(3536)
#define IMX111MIPI_PV_FRAME_LENGTH_LINES						(1254)	
#define IMX111MIPI_FULL_LINE_LENGTH_PIXELS 						(3536)
//<2013/06/21-26217-alberthsiao,fix ANR issue for FIFA test
#define IMX111MIPI_FULL_FRAME_LENGTH_LINES			            (2502)
//>2013/06/21-26217-alberthsiao
#define IMX111MIPI_VIDEO_LINE_LENGTH_PIXELS 					(3500)
#define IMX111MIPI_VIDEO_FRAME_LENGTH_LINES						(1884)	


//For 2x Platform camera_para.c used
#define IMAGE_SENSOR_PV_WIDTH		IMX111MIPI_IMAGE_SENSOR_PV_WIDTH
#define IMAGE_SENSOR_PV_HEIGHT		IMX111MIPI_IMAGE_SENSOR_PV_HEIGHT

#define IMAGE_SENSOR_FULL_WIDTH		IMX111MIPI_IMAGE_SENSOR_FULL_WIDTH
#define IMAGE_SENSOR_FULL_HEIGHT	IMX111MIPI_IMAGE_SENSOR_FULL_HEIGHT

#define IMAGE_SENSOR_FULL_WIDTH		IMX111MIPI_IMAGE_SENSOR_VIDEO_WIDTH
#define IMAGE_SENSOR_FULL_HEIGHT	IMX111MIPI_IMAGE_SENSOR_VIDEO_HEIGHT


#define IMX111MIPI_SHUTTER_LINES_GAP	  3

#define IMX111MIPI_WRITE_ID (0x34)
#define IMX111MIPI_READ_ID	(0x35)



/* SENSOR PRIVATE STRUCT */
struct IMX111_SENSOR_STRUCT
{
	kal_uint8 i2c_write_id;
	kal_uint8 i2c_read_id;

};



struct IMX111MIPI_sensor_STRUCT
	{	 
		  kal_uint16 i2c_write_id;
		  kal_uint16 i2c_read_id;
		  kal_bool first_init;
		  kal_bool fix_video_fps;
		  kal_bool pv_mode; 
		  kal_bool video_mode; 				
		  kal_bool capture_mode; 				//True: Preview Mode; False: Capture Mode
		  kal_bool night_mode;				//True: Night Mode; False: Auto Mode
		  kal_uint8 mirror_flip;
		  kal_uint32 pv_pclk;				//Preview Pclk
		  kal_uint32 video_pclk;				//video Pclk
		  kal_uint32 cp_pclk;				//Capture Pclk
		  kal_uint32 pv_shutter;		   
		  kal_uint32 video_shutter;		   
		  kal_uint32 cp_shutter;
		  kal_uint32 pv_gain;
		  kal_uint32 video_gain;
		  kal_uint32 cp_gain;
		  kal_uint32 pv_line_length;
		  kal_uint32 pv_frame_length;
		  kal_uint32 video_line_length;
		  kal_uint32 video_frame_length;
		  kal_uint32 cp_line_length;
		  kal_uint32 cp_frame_length;
		  kal_uint16 pv_dummy_pixels;		   //Dummy Pixels:must be 12s
		  kal_uint16 pv_dummy_lines;		   //Dummy Lines
		  kal_uint16 video_dummy_pixels;		   //Dummy Pixels:must be 12s
		  kal_uint16 video_dummy_lines;		   //Dummy Lines
		  kal_uint16 cp_dummy_pixels;		   //Dummy Pixels:must be 12s
		  kal_uint16 cp_dummy_lines;		   //Dummy Lines			
		  kal_uint16 video_current_frame_rate;
	};

// SENSOR CHIP VERSION

#define IMX111MIPI_SENSOR_ID            IMX111_SENSOR_ID

#define IMX111MIPI_PAGE_SETTING_REG    (0xFF)

//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 IMX111MIPIOpen(void);
UINT32 IMX111MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 IMX111MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX111MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX111MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 IMX111MIPIClose(void);

//#define Sleep(ms) mdelay(ms)
//#define RETAILMSG(x,...)
//#define TEXT

//e_add for porting
//e_add for porting
//e_add for porting

#endif /* __SENSOR_H */

