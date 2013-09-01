/*******************************************************************************************/


/*******************************************************************************************/

/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H

#define ZSD15FPS

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

typedef enum {
    SENSOR_MODE_INIT = 0,
    SENSOR_MODE_PREVIEW,
    SENSOR_MODE_VIDEO,
    SENSOR_MODE_CAPTURE
} OV12830_SENSOR_MODE;


typedef struct
{
	kal_uint32 DummyPixels;
	kal_uint32 DummyLines;
	
	kal_uint32 pvShutter;
	kal_uint32 pvGain;
	
	kal_uint32 pvPclk;  // x10 480 for 48MHZ
	kal_uint32 videoPclk;
	kal_uint32 capPclk; // x10
	
	kal_uint32 shutter;
	kal_uint32 maxExposureLines;

	kal_uint16 sensorGlobalGain;//sensor gain read from 0x350a 0x350b;
	kal_uint16 ispBaseGain;//64
	kal_uint16 realGain;//ispBaseGain as 1x

	kal_int16 imgMirror;

	OV12830_SENSOR_MODE sensorMode;

	kal_bool OV12830AutoFlickerMode;
	kal_bool OV12830VideoMode;
	
}OV12830_PARA_STRUCT,*POV12830_PARA_STRUCT;


	#define OV12830_IMAGE_SENSOR_FULL_WIDTH					(4000-8)	
	#define OV12830_IMAGE_SENSOR_FULL_HEIGHT					(3000-6)

	/* SENSOR PV SIZE */
	#define OV12830_IMAGE_SENSOR_PV_WIDTH					(2000-4)
	#define OV12830_IMAGE_SENSOR_PV_HEIGHT					(1500-4)

	#define OV12830_IMAGE_SENSOR_VIDEO_WIDTH					(2000-4)
	#define OV12830_IMAGE_SENSOR_VIDEO_HEIGHT				(1500-4)
	

	/* SENSOR SCALER FACTOR */
	#define OV12830_PV_SCALER_FACTOR					    	3
	#define OV12830_FULL_SCALER_FACTOR					    1
	                                        	
	/* SENSOR START/EDE POSITION */         	
	#define OV12830_FULL_X_START						    		(4)
	#define OV12830_FULL_Y_START						    		(4)

	#define OV12830_FULL_X_END						        	(4432)
	#define OV12830_FULL_Y_END						        	(3040)
	#define OV12830_PV_X_START						    		(4)
	#define OV12830_PV_Y_START						    		(4)
	#define OV12830_PV_X_END						    			(2000) 
	#define OV12830_PV_Y_END						    			(1500) 
	
	#define OV12830_VIDEO_X_START								(2)
	#define OV12830_VIDEO_Y_START								(2)
	#define OV12830_VIDEO_X_END 									(2000)
	#define OV12830_VIDEO_Y_END 									(1500)

	#define OV12830_MAX_ANALOG_GAIN					(16)
	#define OV12830_MIN_ANALOG_GAIN					(1)
	#define OV12830_ANALOG_GAIN_1X						(0x0020)

	#define OV12830MIPI_CAPTURE_CLK 	(204000000)
	#define OV12830MIPI_PREVIEW_CLK 	(168000000)
	#define OV12830MIPI_VIDEO_CLK		(168000000)


	/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	#define OV12830_FULL_PERIOD_PIXEL_NUMS					(0x1100)  
	#define OV12830_FULL_PERIOD_LINE_NUMS					(0xc00)


	#define OV12830_PV_PERIOD_PIXEL_NUMS					(2816)
	#define OV12830_PV_PERIOD_LINE_NUMS						(1949)	 

	#define OV12830_VIDEO_PERIOD_PIXEL_NUMS 				(2816)  
	#define OV12830_VIDEO_PERIOD_LINE_NUMS					(1949)	 
	

	#define OV12830_MIN_LINE_LENGTH						0x1100     
	#define OV12830_MIN_FRAME_LENGTH						0xc00	  
	
	#define OV12830_MAX_LINE_LENGTH						0xCCCC
	#define OV12830_MAX_FRAME_LENGTH						0xFFFF

	/* DUMMY NEEDS TO BE INSERTED */
	/* SETUP TIME NEED TO BE INSERTED */
	#define OV12830_IMAGE_SENSOR_PV_INSERTED_PIXELS			2
	#define OV12830_IMAGE_SENSOR_PV_INSERTED_LINES			2

	#define OV12830_IMAGE_SENSOR_FULL_INSERTED_PIXELS		4
	#define OV12830_IMAGE_SENSOR_FULL_INSERTED_LINES		4

 //H/W SID is low.
	#define OV12830MIPI_WRITE_ID 	(0x20)
	#define OV12830MIPI_READ_ID	  (0x21)
	
 //H/W SID is high.
	//#define OV12830MIPI_WRITE_ID 	(0x6C)
	//#define OV12830MIPI_READ_ID	(0x6D)

// SENSOR CHIP VERSION

#define OV12830MIPI_SENSOR_ID            OV12830_SENSOR_ID

#define OV12830MIPI_PAGE_SETTING_REG    (0xFF)

//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 OV12830MIPIOpen(void);
UINT32 OV12830MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV12830MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV12830MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV12830MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 OV12830MIPIClose(void);

//#define Sleep(ms) mdelay(ms)
//#define RETAILMSG(x,...)
//#define TEXT

//e_add for porting
//e_add for porting
//e_add for porting

#endif /* __SENSOR_H */

