/*******************************************************************************************/


/*******************************************************************************************/

/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H

#define MIPI_2_LANE

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
    SENSOR_MODE_CAPTURE
} S5K4E1GA_SENSOR_MODE;

typedef struct
{
	kal_uint16 DummyPixels;
	kal_uint16 DummyLines;
	
	kal_uint16 pvPclk;  // x10 480 for 48MHZ
	kal_uint16 capPclk; // x10
	
	kal_uint16 shutter;
	kal_uint16 maxExposureLines;

	kal_uint16 sensorGlobalGain;
	kal_uint16 pvSensorGlobalGain;
	kal_uint16 sensorBaseGain;
	kal_uint16 ispBaseGain;

	kal_int16 imgMirror;
	kal_bool  S5K4E1GAAutoFlickerMode;

	S5K4E1GA_SENSOR_MODE sensorMode;
	
}S5K4E1GA_PARA_STRUCT,*PS5K4E1GA_PARA_STRUCT;


	#define S5K4E1GA_IMAGE_SENSOR_FULL_WIDTH					(2592-24)	
	#define S5K4E1GA_IMAGE_SENSOR_FULL_HEIGHT					(1944-18)

	/* SENSOR PV SIZE */
	#define S5K4E1GA_IMAGE_SENSOR_PV_WIDTH						(1280)//(1280-4)
	#define S5K4E1GA_IMAGE_SENSOR_PV_HEIGHT						(960)//(960-4)

	/* SENSOR SCALER FACTOR */
	#define S5K4E1GA_PV_SCALER_FACTOR					    	3
	#define S5K4E1GA_FULL_SCALER_FACTOR					    	1
	                                        	
	/* SENSOR START/EDE POSITION */         	
	#define S5K4E1GA_FULL_X_START						    		(2)
	#define S5K4E1GA_FULL_Y_START						    		(2)
	#define S5K4E1GA_FULL_X_END						        		2607 //2601
	#define S5K4E1GA_FULL_Y_END						        		1959 //1953
	#define S5K4E1GA_PV_X_START						    			(2)
	#define S5K4E1GA_PV_Y_START						    			(2)
	#define S5K4E1GA_PV_X_END						    			2607 //2597//2597
	#define S5K4E1GA_PV_Y_END						    			1959 //1953//1949
	
	/* SENSOR HORIZONTAL/VERTICAL BLANKING */	
	//#define S5K4E1GA_IMAGE_SENSOR_FULL_HBLANKING				(30)
	//#define S5K4E1GA_IMAGE_SENSOR_FULL_VBLANKING				(126)			
	//#define S5K4E1GA_IMAGE_SENSOR_PV_HBLANKING					(160)	
	//#define S5K4E1GA_IMAGE_SENSOR_PV_VBLANKING					(130)			

	/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	#define S5K4E1GA_PV_ACTIVE_PIXEL_NUMS						(S5K4E1GA_FULL_X_END-S5K4E1GA_FULL_X_START+1-2)//2598
	#define S5K4E1GA_PV_ACTIVE_LINE_NUMS						(S5K4E1GA_FULL_Y_END-S5K4E1GA_FULL_Y_START+1-2)//1950
	#define S5K4E1GA_FULL_ACTIVE_PIXEL_NUMS						(((S5K4E1GA_PV_X_END-S5K4E1GA_PV_X_START+1)/((S5K4E1GA_PV_SCALER_FACTOR+1)/2)))//1300
	#define S5K4E1GA_FULL_ACTIVE_LINE_NUMS						(((S5K4E1GA_PV_Y_END-S5K4E1GA_PV_Y_START+1)/((S5K4E1GA_PV_SCALER_FACTOR+1)/2)))//1952

	#define S5K4E1GA_MAX_ANALOG_GAIN					(16)
	#define S5K4E1GA_MIN_ANALOG_GAIN					(1)
	#define S5K4E1GA_ANALOG_GAIN_1X						(0x0020)

	//#define S5K4E1GA_MAX_DIGITAL_GAIN					(8)
	//#define S5K4E1GA_MIN_DIGITAL_GAIN					(1)
	//#define S5K4E1GA_DIGITAL_GAIN_1X					(0x0100)


	#define S5K4E1GA_FULL_PERIOD_PIXEL_NUMS					0x0AB2  //2738
	#define S5K4E1GA_FULL_PERIOD_LINE_NUMS					0x07B4  //1972
	
	#define S5K4E1GA_PV_PERIOD_PIXEL_NUMS					0x0AB2	//2738
	#define S5K4E1GA_PV_PERIOD_LINE_NUMS					0x03E0	//992

	#define S5K4E1GA_MIN_LINE_LENGTH						0x0AA4  //2724
	#define S5K4E1GA_MIN_FRAME_LENGTH						0x0214  //532
	
	#define S5K4E1GA_MAX_LINE_LENGTH						0xCCCC
	#define S5K4E1GA_MAX_FRAME_LENGTH						0xFFFF

	/* DUMMY NEEDS TO BE INSERTED */
	/* SETUP TIME NEED TO BE INSERTED */
	#define S5K4E1GA_IMAGE_SENSOR_PV_INSERTED_PIXELS			2	// Sync, Nosync=2
	#define S5K4E1GA_IMAGE_SENSOR_PV_INSERTED_LINES			2

	#define S5K4E1GA_IMAGE_SENSOR_FULL_INSERTED_PIXELS		4
	#define S5K4E1GA_IMAGE_SENSOR_FULL_INSERTED_LINES		4

#define S5K4E1GAMIPI_WRITE_ID 	(0x6E)	//(0x20)   //(0x6E)
#define S5K4E1GAMIPI_READ_ID	(0x6F)

// SENSOR CHIP VERSION

#define S5K4E1GAMIPI_SENSOR_ID            S5K4E1GA_SENSOR_ID

#define S5K4E1GAMIPI_PAGE_SETTING_REG    (0xFF)

//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 S5K4E1GAMIPIOpen(void);
UINT32 S5K4E1GAMIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 S5K4E1GAMIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 S5K4E1GAMIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 S5K4E1GAMIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 S5K4E1GAMIPIClose(void);

//#define Sleep(ms) mdelay(ms)
//#define RETAILMSG(x,...)
//#define TEXT

//e_add for porting
//e_add for porting
//e_add for porting

#endif /* __SENSOR_H */

