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
	



/* START GRAB PIXEL OFFSET */
#define PAS6180_IMAGE_SENSOR_START_X		        0	// 0 or 1 recommended
#define PAS6180_IMAGE_SENSOR_START_Y		        1	// 0 or 1 recommended

/* MAX/MIN FRAME RATE (FRAMES PER SEC.) */
#define MAX_FRAME_RATE							15		// Limitation for MPEG4 Encode Only
#define MIN_FRAME_RATE							12

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
    #define PAS6180_FULL_PERIOD_PIXEL_NUMS  (252)  //  Default full size line length 
    #define PAS6180_FULL_PERIOD_LINE_NUMS   (328)  // Default full size frame length
    #define PAS6180_PV_PERIOD_PIXEL_NUMS    (252)  // Default preview line length
    #define PAS6180_PV_PERIOD_LINE_NUMS     (328)   // Default preview frame length

    /* SENSOR EXPOSURE LINE LIMITATION */
    #define PAS6180_FULL_EXPOSURE_LIMITATION    (1236)
    #define PAS6180_PV_EXPOSURE_LIMITATION      (618)

/* SENSOR FULL SIZE */
   #define PAS6180_IMAGE_SENSOR_FULL_WIDTH	   (240)  
   #define PAS6180_IMAGE_SENSOR_FULL_HEIGHT	 (320)    



/* SENSOR PV SIZE */
#define PAS6180_IMAGE_SENSOR_PV_WIDTH   (240)   
#define PAS6180_IMAGE_SENSOR_PV_HEIGHT (320)

#define PAS6180_VIDEO_QCIF_WIDTH   (176)
#define PAS6180_VIDEO_QCIF_HEIGHT  (144)

#define PAS6180_VIDEO_30FPS_FRAME_LENGTH   (0x29E)
#define PAS6180_VIDEO_20FPS_FRAME_LENGTH   (0x3ED)
#define PAS6180_VIDEO_15FPS_FRAME_LENGTH   (0x53C)
#define PAS6180_VIDEO_10FPS_FRAME_LENGTH   (0x7DA)

// SETUP TIME NEED TO BE INSERTED
#define PAS6180_IMAGE_SENSOR_PV_INSERTED_PIXELS (390)
#define PAS6180_IMAGE_SENSOR_PV_INSERTED_LINES  (9 - 6)

#define PAS6180_IMAGE_SENSOR_FULL_INSERTED_PIXELS   (248)
#define PAS6180_IMAGE_SENSOR_FULL_INSERTED_LINES    (11 - 2)

#define PAS6180_PV_DUMMY_PIXELS			(0)
#define PAS6180_VIDEO__CIF_DUMMY_PIXELS  (0)
#define PAS6180_VIDEO__QCIF_DUMMY_PIXELS (0)

/* SENSOR SCALER FACTOR */
#define PV_SCALER_FACTOR					    3
#define FULL_SCALER_FACTOR					    1


/* DUMMY NEEDS TO BE INSERTED */
/* SETUP TIME NEED TO BE INSERTED */

// SENSOR VGA SIZE
//For 2x Platform camera_para.c used
#define IMAGE_SENSOR_PV_WIDTH    PAS6180_IMAGE_SENSOR_PV_WIDTH
#define IMAGE_SENSOR_PV_HEIGHT   PAS6180_IMAGE_SENSOR_PV_HEIGHT

#define IMAGE_SENSOR_FULL_WIDTH	  PAS6180_IMAGE_SENSOR_FULL_WIDTH
#define IMAGE_SENSOR_FULL_HEIGHT	  PAS6180_IMAGE_SENSOR_FULL_HEIGHT


/* SENSOR READ/WRITE ID */
	#define PAS6180_WRITE_ID				0x70
	#define PAS6180_READ_ID				0x71



	/* SENSOR CHIP VERSION */
//	#define PAS6180_SENSOR_ID    (0x6179)    // 




//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 PAS6180SERIALYUVOpen(void);
UINT32 PAS6180SERIALYUVGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 PAS6180SERIALYUVGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 PAS6180SERIALYUVControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 PAS6180SERIALYUVFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 PAS6180SERIALYUVClose(void);


//e_add for porting
//e_add for porting
//e_add for porting

#endif /* __SENSOR_H */ 
