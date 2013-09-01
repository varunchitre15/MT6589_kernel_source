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
 * 05 17 2011 koli.lin
 * [ALPS00048194] [Need Patch] [Volunteer Patch]
 * [Camera]. Chagne the preview size to 1600x1200 for IMX105 sensor.
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
 * Modify the IMX105 sensor driver for preview mode.
 *
 * 02 11 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Create IMX105 sensor driver to database.
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

//************************Jun add*************************//
//************************Jun add*************************//
#define Sleep(us) udelay(us)

#define RETAILMSG(x,...)
#define TEXT

#define IMX105MIPI_SENSOR_ID            IMX105_SENSOR_ID

#define IMX105MIPI_WRITE_ID_1 (0x34)
#define IMX105MIPI_READ_ID_1	(0x35)

#define IMX105MIPI_WRITE_ID_2 (0x20)
#define IMX105MIPI_READ_ID_2	(0x21)

#define IMX105MIPI_WRITE_ID_3 (0x6C)
#define IMX105MIPI_READ_ID_3	(0x6D)


#if 1
#define IMX105MIPI_PV_GRAB_X 						(2)
#define IMX105MIPI_PV_GRAB_Y					    (2)
#define IMX105MIPI_FULL_GRAB_X						(4)
#define IMX105MIPI_FULL_GRAB_Y						(4)

#define IMX105MIPI_PV_ACTIVE_PIXEL_NUMS 						(1640-40)
#define IMX105MIPI_PV_ACTIVE_LINE_NUMS							(1232-32)
#define IMX105MIPI_FULL_ACTIVE_PIXEL_NUMS						(3280-80)
#define IMX105MIPI_FULL_ACTIVE_LINE_NUMS						(2464-64)

#else
#define IMX105MIPI_PV_GRAB_X 						(8)
#define IMX105MIPI_PV_GRAB_Y					    (2)
#define IMX105MIPI_FULL_GRAB_X						(20)//(16)
#define IMX105MIPI_FULL_GRAB_Y						(18)//(4)

#define IMX105MIPI_PV_ACTIVE_PIXEL_NUMS 						(1640)
#define IMX105MIPI_PV_ACTIVE_LINE_NUMS							(1232)
#define IMX105MIPI_FULL_ACTIVE_PIXEL_NUMS						(3280)
#define IMX105MIPI_FULL_ACTIVE_LINE_NUMS						(2464)
#endif

#define IMX105MIPI_PV_LINE_LENGTH_PIXELS 						(3536)
#define IMX105MIPI_PV_FRAME_LENGTH_LINES						(1270)	
#define IMX105MIPI_FULL_LINE_LENGTH_PIXELS 						(3536)
#define IMX105MIPI_FULL_FRAME_LENGTH_LINES			            (2534)

#define FACTORY_START_ADDR 	0
#define ENGINEER_START_ADDR	10

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


struct IMX105MIPI_sensor_STRUCT
	{	 
		  kal_uint16 i2c_write_id;
		  kal_uint16 i2c_read_id;
		  kal_bool first_init;
		  kal_bool fix_video_fps;
		  kal_bool pv_mode; 				//True: Preview Mode; False: Capture Mode
		  kal_bool night_mode;				//True: Night Mode; False: Auto Mode
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

//************************Jun add*************************//
//************************Jun add*************************//

//export functions
UINT32 IMX105MIPIOpen(void);
UINT32 IMX105MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 IMX105MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX105MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 IMX105MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 IMX105MIPIClose(void);



#endif /* __SENSOR_H */

