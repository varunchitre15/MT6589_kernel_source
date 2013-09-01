/*******************************************************************************************/


/*******************************************************************************************/

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

typedef enum {
    SENSOR_MODE_INIT = 0,
    SENSOR_MODE_PREVIEW,
    SENSOR_MODE_VIDEO,
    SENSOR_MODE_CAPTURE
} OV8825_SENSOR_MODE;


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

	OV8825_SENSOR_MODE sensorMode;

	kal_bool OV8825AutoFlickerMode;
	kal_bool OV8825VideoMode;
	
}OV8825_PARA_STRUCT,*POV8825_PARA_STRUCT;


	#define OV8825_IMAGE_SENSOR_FULL_WIDTH					(3264-64)	
	#define OV8825_IMAGE_SENSOR_FULL_HEIGHT					(2448-48)

	/* SENSOR PV SIZE */
	#define OV8825_IMAGE_SENSOR_PV_WIDTH					(1632-32)
	#define OV8825_IMAGE_SENSOR_PV_HEIGHT					(1224-24)

	#define OV8825_IMAGE_SENSOR_VIDEO_WIDTH					(3264-64)
	#define OV8825_IMAGE_SENSOR_VIDEO_HEIGHT				(1836-48)
	

	/* SENSOR SCALER FACTOR */
	#define OV8825_PV_SCALER_FACTOR					    	3
	#define OV8825_FULL_SCALER_FACTOR					    1
	                                        	
	/* SENSOR START/EDE POSITION */         	
	#define OV8825_FULL_X_START						    		(2)
	#define OV8825_FULL_Y_START						    		(8)
	#define OV8825_FULL_X_END						        	(3264) 
	#define OV8825_FULL_Y_END						        	(2448) 
	#define OV8825_PV_X_START						    		(2)
	#define OV8825_PV_Y_START						    		(2)
	#define OV8825_PV_X_END						    			(1632) 
	#define OV8825_PV_Y_END						    			(1224) 
	
	#define OV8825_VIDEO_X_START								(2)
	#define OV8825_VIDEO_Y_START								(2)
	#define OV8825_VIDEO_X_END 									(3264) 
	#define OV8825_VIDEO_Y_END 									(1836) 

	#define OV8825_MAX_ANALOG_GAIN					(16)
	#define OV8825_MIN_ANALOG_GAIN					(1)
	#define OV8825_ANALOG_GAIN_1X						(0x0020)

	//#define OV8825_MAX_DIGITAL_GAIN					(8)
	//#define OV8825_MIN_DIGITAL_GAIN					(1)
	//#define OV8825_DIGITAL_GAIN_1X					(0x0100)

	/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	#define OV8825_FULL_PERIOD_PIXEL_NUMS					0x0E30  //3632
	#define OV8825_FULL_PERIOD_LINE_NUMS					0x9F0	//2544
	
	#define OV8825_PV_PERIOD_PIXEL_NUMS					0x0DBC  //3516
	#define OV8825_PV_PERIOD_LINE_NUMS					0x51E	//1310

	#define OV8825_VIDEO_PERIOD_PIXEL_NUMS 				0x0E30	//3632
	#define OV8825_VIDEO_PERIOD_LINE_NUMS				0x07C0	//1984
	

	#define OV8825_MIN_LINE_LENGTH						0x0AA4  //2724
	#define OV8825_MIN_FRAME_LENGTH						0x0214  //532
	
	#define OV8825_MAX_LINE_LENGTH						0xCCCC
	#define OV8825_MAX_FRAME_LENGTH						0xFFFF

	/* DUMMY NEEDS TO BE INSERTED */
	/* SETUP TIME NEED TO BE INSERTED */
	#define OV8825_IMAGE_SENSOR_PV_INSERTED_PIXELS			2
	#define OV8825_IMAGE_SENSOR_PV_INSERTED_LINES			2

	#define OV8825_IMAGE_SENSOR_FULL_INSERTED_PIXELS		4
	#define OV8825_IMAGE_SENSOR_FULL_INSERTED_LINES		4

#define OV8825MIPI_WRITE_ID 	(0x6C)
#define OV8825MIPI_READ_ID	(0x6D)

// SENSOR CHIP VERSION

#define OV8825MIPI_SENSOR_ID            OV8825_SENSOR_ID

#define OV8825MIPI_PAGE_SETTING_REG    (0xFF)

//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 OV8825MIPIOpen(void);
UINT32 OV8825MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV8825MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV8825MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV8825MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 OV8825MIPIClose(void);

//#define Sleep(ms) mdelay(ms)
//#define RETAILMSG(x,...)
//#define TEXT

//e_add for porting
//e_add for porting
//e_add for porting

#endif /* __SENSOR_H */

