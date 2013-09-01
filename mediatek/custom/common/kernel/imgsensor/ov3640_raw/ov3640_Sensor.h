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

    typedef enum register_index {
        PRE_GAIN=0,
        CMMCLK_CURRENT,
        REGISTER_EDITOR,
        GROUP_TOTAL_NUMS
    } FACTORY_REGISTER_INDEX;

    typedef enum cct_register_index {
        INDEX_BASE_GAIN =0,
        INDEX_PRE_GAIN_R,
        INDEX_PRE_GAIN_Gr,
        INDEX_PRE_GAIN_Gb,
        INDEX_PRE_GAIN_B,
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

  
    typedef enum _OV3640_OP_TYPE {
        OV3640_MODE_NONE,
        OV3640_MODE_PREVIEW,
        OV3640_MODE_CAPTURE,
   //     OV3640_MODE_QCIF_VIDEO,
    //    OV3640_MODE_CIF_VIDEO
    } OV3640_OP_TYPE;

    extern OV3640_OP_TYPE g_iOV3640_Mode;

	#define OV3640_VIDEO_NORMALMODE_FRAME_RATE		(30)
	#define OV3640_VIDEO_NIGHTMODE_FRAME_RATE		(15)

//	#define OV3640_VIDEO_QCIF_NORMAL_FRAME_RATE		(30)
//	#define OV3640_VIDEO_QCIF_NIGHT_FRAME_RATE 		(15)

//	#define OV3640_VIDEO_CIF_NORMAL_FRAME_RATE		(15)
//	#define OV3640_VIDEO_CIF_NIGHT_FRAME_RATE		(7.5)


    /* MAXIMUM EXPLOSURE LINES USED BY AE */
    extern kal_uint16 MAX_EXPOSURE_LINES;
    extern kal_uint8  MIN_EXPOSURE_LINES;

 
    // 0x3028, 0x3029 defines the PCLKs in one line of OV3640
    // If [0x3028:0x3029] = N, the total PCLKs in one line of QXGA(3M full mode) is (N+1), and
    // total PCLKs in one line of XGA subsampling mode is (N+1) / 2
    // If need to add dummy pixels, just increase 0x3028 and 0x3029 directly
    #define QXGA_MODE_WITHOUT_DUMMY_PIXELS  (2376)  // QXGA mode's pixel # in one HSYNC w/o dummy pixels
    #define XGA_MODE_WITHOUT_DUMMY_PIXELS   (QXGA_MODE_WITHOUT_DUMMY_PIXELS / 2)    // XGA mode's pixel # in one HSYNC w/o dummy pixels
    #define PV_PERIOD_PIXEL_NUMS    (XGA_MODE_WITHOUT_DUMMY_PIXELS) // dummy pixels is down sampled by half in XGA mode

    // 0x302A, 0x302B defines total lines in one frame of OV3640
    // If [0x302A:0x302B] = N, the total lines in one is N in dependent of resolution setting
    // Even in XGA subsampling mode, total lines defined by 0x302A, 0x302B is not subsampled.
    // If need dummy lines, just increase 0x302A and 0x302B directly
    #define QXGA_MODE_WITHOUT_DUMMY_LINES   (1568)  // QXGA mode's HSYNC # in one HSYNC w/o dummy lines
    #define XGA_MODE_WITHOUT_DUMMY_LINES    (784)   // XGA mode's HSYNC # in one HSYNC w/o dummy lines
    #define PV_PERIOD_LINE_NUMS     (XGA_MODE_WITHOUT_DUMMY_LINES)  // XGA mode's HSYNC # in one VSYNC period

    // SENSOR EXPOSURE LINE LIMITATION
    #define FULL_MAX_LINES_PER_FRAME    (1568)  // QXGA mode
    #define FULL_EXPOSURE_LIMITATION    (FULL_MAX_LINES_PER_FRAME)
    #define PV_MAX_LINES_PER_FRAME  (XGA_MODE_WITHOUT_DUMMY_LINES)  // # of lines in one XGA frame
    #define PV_EXPOSURE_LIMITATION  (PV_MAX_LINES_PER_FRAME)

    // sensor's full resolution
  // #define IMAGE_SENSOR_FULL_WIDTH     (2044) // (2048)
  //  #define IMAGE_SENSOR_FULL_HEIGHT    (1528) //(1536)
  
	#define IMAGE_SENSOR_FULL_WIDTH 	(2048-16)//16
    #define IMAGE_SENSOR_FULL_HEIGHT	 (1536-12) //12

    // sensor's full resolution
    #define IMAGE_SENSOR_FULL_WIDTH_DRV     (2048) 
    #define IMAGE_SENSOR_FULL_HEIGHT_DRV    (1536) 

    // resolution for preview
    #define IMAGE_SENSOR_PV_WIDTH   (1024)
    #define IMAGE_SENSOR_PV_HEIGHT  (768)
    
//   #define ISP_INTERPOLATIO_FILTER_WIDTH   (7)
//   #define ISP_INTERPOLATIO_FILTER_HEIGHT  (7)

    /* SENSOR READ/WRITE ID */
    #define OV3640_WRITE_ID (0x78)
    #define OV3640_READ_ID  (0x79)

    /* SENSOR CHIP VERSION */
//    #define OV3640_SENSOR_ID    (0x3641)    // rev.2A
//    #define OV3640_SENSOR_ID    (0x364C)    // rev.2C


//s_porting add
//s_porting add
//s_porting add
//#define PV_VGA_ACTUAL_WIDTH     648
//#define PV_VGA_ACTUAL_HEIGHT    490
//#define PV_VGA_TOTAL_PIXEL_PER_LINE (2068 + 648)
//#define PV_VGA_TOTAL_LINE_PER_FRAME (662 + 490)

//#define PV_1280X960_ACTUAL_WIDTH     1288 //(PV_ACTIVE_PIXEL_NUMS+1)
//#define PV_1280X960_ACTUAL_HEIGHT    970 //(PV_ACTIVE_LINE_NUMS+1)
//#define PV_1280X960_TOTAL_PIXEL_PER_LINE (1614 + 1288)
//#define PV_1280X960_TOTAL_LINE_PER_FRAME (106 + 970)

//#define FULL_ACTUAL_WIDTH     2592
//#define FULL_ACTUAL_HEIGHT    1944
//#define FULL_TOTAL_PIXEL_PER_LINE (2838 + 2592)
//#define FULL_TOTAL_LINE_PER_FRAME (112 + 1944)

//#define IMAGE_SENSOR_PV_WIDTH    IMAGE_SENSOR_PV_WIDTH
//#define IMAGE_SENSOR_PV_HEIGHT   IMAGE_SENSOR_PV_HEIGHT
	
//#define FULL_ACTUAL_WIDTH	  IMAGE_SENSOR_FULL_WIDTH
//#define FULL_ACTUAL_HEIGHT	  IMAGE_SENSOR_FULL_HEIGHT

//format index
#define OV3640_PV_FORMAT_INDEX   1 //1280x960 mode
#define OV3640_FULL_FORMAT_INDEX 2 //Full resolution mode

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
UINT32 OV3640Open(void);
UINT32 OV3640Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV3640FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 OV3640GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV3640GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV3640Close(void);

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT
//e_porting add
//e_porting add
//e_porting add



#endif /* __SENSOR_H */ 
