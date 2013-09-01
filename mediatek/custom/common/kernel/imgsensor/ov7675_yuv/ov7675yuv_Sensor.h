
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
 *   CMOS sensor header file
 *
 ****************************************************************************/
#ifndef __SENSOR_H
#define __SENSOR_H

#include "image_sensor.h"//get IMAGE_SENSOR_DRVNAME
#define IMAGE_SENSOR_DRVNAME SENSOR_DRVNAME_OV7675_YUV

    //------------------------Engineer mode---------------------------------
#define FACTORY_START_ADDR 	0
#define ENGINEER_START_ADDR	10

    typedef enum group_enum {
       PRE_GAIN=0,
	   CMMCLK_CURRENT,
	   FRAME_RATE_LIMITATION,
	   REGISTER_EDITOR,
	   GROUP_TOTAL_NUMS
    } FACTORY_REGISTER_INDEX;

    typedef enum register_index {
        SENSOR_BASEGAIN=FACTORY_START_ADDR,
	      PRE_GAIN_R_INDEX,
	      PRE_GAIN_Gr_INDEX,
	      PRE_GAIN_Gb_INDEX,
	      PRE_GAIN_B_INDEX,
	      FACTORY_END_ADDR
    } CCT_REGISTER_INDEX;
    
 typedef enum engineer_index
{   
	CMMCLK_CURRENT_INDEX=ENGINEER_START_ADDR,
	ENGINEER_END
} FACTORY_ENGINEER_INDEX; 

    //------------------------Engineer mode---------------------------------
    typedef struct {
        SENSOR_REG_STRUCT Reg[ENGINEER_END];
        SENSOR_REG_STRUCT CCT[FACTORY_END_ADDR];
    } SENSOR_DATA_STRUCT,*PSENSOR_DATA_STRUCT;




/*
 typedef struct _sensor_data_struct
 {
   SENSOR_REG_STRUCT reg[MT9P015_ENGINEER_END];
   SENSOR_REG_STRUCT cct[MT9P015_FACTORY_END_ADDR];
 } sensor_data_struct;
*/
	
 #define CAM_PREVIEW_30FPS
 #define SYSTEM_CLK                           (52*1000*1000)
		/* PIXEL CLOCK USED BY BANDING FILTER CACULATION*/
#if defined(CAM_PREVIEW_15FPS)
  #define PIXEL_CLK							    (SYSTEM_CLK/8)		// 52/8 MHz
#elif defined(CAM_PREVIEW_22FPS)
   #define PIXEL_CLK							    (SYSTEM_CLK/6)		// 52/6 MHz
#elif defined(CAM_PREVIEW_30FPS)
   #define PIXEL_CLK 						      	(SYSTEM_CLK/4)		// 52/4 MHz
#endif

   #define OV7675_VIDEO_NORMALMODE_FRAME_RATE							30		// Limitation for MPEG4 Encode Only
   #define OV7675_VIDEO_NIGHTMODE_FRAME_RATE							15
	/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
	#define VGA_PERIOD_PIXEL_NUMS					784
	#define VGA_PERIOD_LINE_NUMS					510

	/* SENSOR EXPOSURE LINE LIMITATION */
	#define VGA_EXPOSURE_LIMITATION					510

	/* SENSOR GLOBAL GAIN AT NIGHT MODE */
	#define OV7675_SENSOR_NIGHT_MODE_GAIN					0x08	// Please refer to OV7670 Implementation Guide
	
	/* SENSOR VGA SIZE */
	#define IMAGE_SENSOR_VGA_WIDTH					(640-8)
	#define IMAGE_SENSOR_VGA_HEIGHT					(480-6)


    #define IMAGE_SENSOR_FULL_WIDTH          (640-8) 
    #define IMAGE_SENSOR_FULL_HEIGHT         (480-6) 

    #define IMAGE_SENSOR_PV_WIDTH   IMAGE_SENSOR_VGA_WIDTH
    #define IMAGE_SENSOR_PV_HEIGHT  IMAGE_SENSOR_VGA_HEIGHT
    
	/* SETUP TIME NEED TO BE INSERTED */
	#define IMAGE_SENSOR_VGA_INSERTED_PIXELS		128
	#define IMAGE_SENSOR_VGA_INSERTED_LINES		    17
	
	#define OV7675_WRITE_ID								0x42
	#define OV7675_READ_ID								0x43
	
//  #define OV7675_SENSOR_ID    (0x7673)  

  typedef struct
  {
	  kal_uint32 (*Open)(struct i2c_client *i2c_clit);
	  kal_uint32 (*Close)(void);
  
	  kal_uint32 (*GetResolution)(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
	  kal_uint32 (*GetInfo)(MSDK_SCENARIO_ID_ENUM ScenarioId,MSDK_SENSOR_INFO_STRUCT *pSensorInfo,MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
  
	  kal_uint32 (*Control)(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);	  
	  kal_uint32 (* FeatureControl)(MSDK_SENSOR_FEATURE_ENUM ScenarioId, kal_uint8 *para, kal_uint32 *len);
  
  }image_sensor_func_struct;
  
  

  void image_sensor_func_config(void);








struct OV7675_Sensor_Struct
{
	struct i2c_client *i2c_clit;
	MSDK_SENSOR_CONFIG_STRUCT cfg_data;
	SENSOR_DATA_STRUCT eng; /* engineer mode */
	MSDK_SENSOR_ENG_INFO_STRUCT eng_info;

	
	kal_bool sensor_night_mode;
	kal_bool MPEG4_encode_mode;

	kal_uint16 dummy_pixels;
	kal_uint16 dummy_lines;
	kal_uint16 extra_exposure_lines;
	kal_uint16 exposure_lines;

	kal_bool MODE_CAPTURE;
	kal_uint16 iBackupExtraExp;


	
	kal_uint32 fPV_PCLK; //26000000;
	kal_uint16 iPV_Pixels_Per_Line;

	kal_bool  bNight_mode; // to distinguish night mode or auto mode, default: auto mode setting
	kal_bool  bBanding_value; // to distinguish between 50HZ and 60HZ.
	kal_uint8 u8Wb_value;
	kal_uint8 u8Effect_value;
	kal_uint8 u8Ev_value;
};


/*

typedef enum
{
    FID_AE_FLICKER = 0,
    FID_AE_METERING,
    FID_AE_ISO,
    FID_AE_EV,
    FID_AE_STROBE,
    FID_AWB_MODE,
    FID_AF_MODE,
    FID_AF_METERING,
    FID_ISP_EDGE,
    FID_ISP_HUE,
    FID_ISP_SAT,
    FID_ISP_BRIGHT,
    FID_ISP_CONTRAST,
    FID_AE_SCENE_MODE,
    FID_SCENE_MODE,
    FID_COLOR_EFFECT,
    FID_FD_ON_OFF,
    FID_CAP_SIZE,
    FID_PREVIEW_SIZE,
    FID_FRAME_RATE,
    FID_TOTAL_NUM
} FEATURE_ID;

// Scene mode definition
typedef enum
{
    SCENE_MODE_UNSUPPORT = -1,
    SCENE_MODE_OFF = 0,         // Disable scene mode    equal Auto mode //    AAA_SCENE_MODE_AUTO,           // Auto mode
    SCENE_MODE_ACTION,          // Action mode
    SCENE_MODE_PORTRAIT,        // Portrait mode
    SCENE_MODE_LANDSCAPE,       // Landscape
    SCENE_MODE_NIGHTSCENE,      // Night Scene
    SCENE_MODE_NIGHTPORTRAIT,   // Night Portrait 
    SCENE_MODE_THEATRE,         // Theatre mode    
    SCENE_MODE_BEACH,           // Beach mode
    SCENE_MODE_SNOW,            // Snow mode    
    SCENE_MODE_SUNSET,          // Sunset mode
    SCENE_MODE_STEADYPHOTO,     // Steady photo mode
    SCENE_MODE_FIREWORKS,       // Fireworks mode
    SCENE_MODE_SPORTS,          // Sports mode    
    SCENE_MODE_PARTY,           // Party mode        
    SCENE_MODE_CANDLELIGHT,     // Candle light mode   
    SCENE_MODE_ISO_ANTI_SHAKE,  // ISO Anti Shake mode
    SCENE_MODE_BRACKET_AE,      // Bracket AE
    SCENE_MODE_NUM,
    SCENE_MODE_MIN = SCENE_MODE_OFF,
    SCENE_MODE_MAX = SCENE_MODE_BRACKET_AE
} SCENE_MODE_T;

// AE set flicker mode
typedef enum
{
    AE_FLICKER_MODE_UNSUPPORTED = -1,
    AE_FLICKER_MODE_60HZ,
    AE_FLICKER_MODE_50HZ,    
    AE_FLICKER_MODE_AUTO,    // No support in MT6516
    AE_FLICKER_MODE_OFF,     // No support in MT6516
    AE_FLICKER_MODE_TOTAL_NUM,
    AE_FLICKER_MODE_MIN = AE_FLICKER_MODE_60HZ,
    AE_FLICKER_MODE_MAX = AE_FLICKER_MODE_OFF
}AE_FLICKER_MODE_T;

// AWB mode definition
typedef enum
{
    AWB_MODE_AUTO ,                // Auto white balance
    AWB_MODE_DAYLIGHT,             // Daylight
    AWB_MODE_CLOUDY_DAYLIGHT,      // Cloudy daylight
    AWB_MODE_SHADE,                // Shade
    AWB_MODE_TWILIGHT,             // Twilight
    AWB_MODE_FLUORESCENT,          // Fluorescent
    AWB_MODE_WARM_FLUORESCENT,     // Warm fluorescent
    AWB_MODE_INCANDESCENT,         // Incandescent
    AWB_MODE_TUNGSTEN,             // Tungsten
    
    AWB_MODE_NUM,                  // AWB mode number
    AWB_MODE_MIN = AWB_MODE_AUTO,
    AWB_MODE_MAX = AWB_MODE_INCANDESCENT
} AWB_MODE_T;

typedef enum
{
    MEFFECT_OFF = 0,                
    MEFFECT_MONO,
    MEFFECT_SEPIA,
    MEFFECT_NEGATIVE,
    MEFFECT_SOLARIZE,
    MEFFECT_AQUA,
    MEFFFECT_BLACKBOARD,
    MEFFECT_POSTERIZE,
    MEFFECT_WHITEBOARD,
    MEFFECT_SEPIAGREEN, 
    MEFFECT_SEPIABLUE,
    MEFFECT_NUM
} MCOLOR_EFFECT;

// AE EV compensation
typedef enum                            // enum  for evcompensate
{
    AE_EV_COMP_UNSUPPORTED = -1,
    AE_EV_COMP_00          =  0,           // Disable EV compenate
    AE_EV_COMP_03          =  1,           // EV compensate 0.3
    AE_EV_COMP_05          =  2,           // EV compensate 0.5
    AE_EV_COMP_07          =  3,           // EV compensate 0.7
    AE_EV_COMP_10          =  4,           // EV compensate 1.0
    AE_EV_COMP_13          =  5,           // EV compensate 1.3
    AE_EV_COMP_15          =  6,           // EV compensate 1.5
    AE_EV_COMP_17          =  7,           // EV compensate 1.7
    AE_EV_COMP_20          =  8,           // EV compensate 2.0
    AE_EV_COMP_n03         =  9,           // EV compensate -0.3    
    AE_EV_COMP_n05         = 10,           // EV compensate -0.5
    AE_EV_COMP_n07         = 11,           // EV compensate -0.7
    AE_EV_COMP_n10         = 12,           // EV compensate -1.0
    AE_EV_COMP_n13         = 13,           // EV compensate -1.3    
    AE_EV_COMP_n15         = 14,           // EV compensate -1.5
    AE_EV_COMP_n17         = 15,           // EV compensate -1.7
    AE_EV_COMP_n20         = 16,           // EV compensate -2.0
    AE_EV_COMP_TOTAL_NUM,
    AE_EV_COMP_MIN = AE_EV_COMP_00,
    AE_EV_COMP_MAX = AE_EV_COMP_n20 
}AE_EVCOMP_T;



enum
{
    CAM_EFFECT_ENC_NORMAL = 0,
    CAM_EFFECT_ENC_GRAYSCALE,
    CAM_EFFECT_ENC_SEPIA,
    CAM_EFFECT_ENC_SEPIAGREEN,
    CAM_EFFECT_ENC_SEPIABLUE,
    CAM_EFFECT_ENC_COLORINV,
    CAM_EFFECT_ENC_GRAYINV,
    CAM_EFFECT_ENC_BLACKBOARD,
    CAM_EFFECT_ENC_WHITEBOARD,
    CAM_EFFECT_ENC_COPPERCARVING,
    CAM_EFFECT_ENC_EMBOSSMENT,
    CAM_EFFECT_ENC_BLUECARVING,
    CAM_EFFECT_ENC_CONTRAST,
    CAM_EFFECT_ENC_JEAN,
    CAM_EFFECT_ENC_SKETCH,
    CAM_EFFECT_ENC_OIL,
    CAM_NO_OF_EFFECT_ENC
};

enum
{
    CAM_EV_NEG_4_3 = 0,
    CAM_EV_NEG_3_3,
    CAM_EV_NEG_2_3,
    CAM_EV_NEG_1_3,
    CAM_EV_ZERO,
    CAM_EV_POS_1_3,
    CAM_EV_POS_2_3,
    CAM_EV_POS_3_3,
    CAM_EV_POS_4_3,
    CAM_EV_NIGHT_SHOT,
    CAM_NO_OF_EV
};

enum
{
    CAM_EFFECT_NOMRAL = 0,
    CAM_EFFECT_SEPIA,
    CAM_EFFECT_WHITELINE,
    CAM_EFFECT_BLACKLINE,
    CAM_EFFECT_BW,
    CAM_EFFECT_GRAYEDGE,
    CAM_EFFECT_FILM,
    CAM_NO_OF_EFFECT
};
enum
{
    CAM_WB_AUTO = 0,
    CAM_WB_CLOUD,
    CAM_WB_DAYLIGHT,
    CAM_WB_INCANDESCENCE,
    CAM_WB_FLUORESCENT,
    CAM_WB_TUNGSTEN,
    CAM_WB_MANUAL,
    CAM_NO_OF_WB
};
enum
{
    CAM_BANDING_50HZ = 0,
    CAM_BANDING_60HZ,
    CAM_NO_OF_BANDING
};

*/


#endif /* __SENSOR_H */ 
