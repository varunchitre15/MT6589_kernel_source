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


    typedef enum _OV5650MIPI_OP_TYPE {
        OV5650MIPI_MODE_NONE,
        OV5650MIPI_MODE_PREVIEW,
        OV5650MIPI_MODE_CAPTURE,
        OV5650MIPI_MODE_QCIF_VIDEO,
        OV5650MIPI_MODE_CIF_VIDEO
    } OV5650MIPI_OP_TYPE;


    extern OV5650MIPI_OP_TYPE OV5650MIPI_g_iOV5650MIPI_Mode;

    /* MAXIMUM EXPLOSURE LINES USED BY AE */
    extern kal_uint16 OV5650MIPI_MAX_EXPOSURE_LINES;
    extern kal_uint8  OV5650MIPI_MIN_EXPOSURE_LINES;

    /* DEFINITION USED BY CCT */
//    extern SensorInfo OV5650MIPI_g_CCT_MainSensor;
  //  extern kal_uint8 OV5650MIPI_g_CCT_FirstGrabColor;

	#define OV5650MIPI_VIDEO_NORMALMODE_FRAME_RATE		(25)
	#define OV5650MIPI_VIDEO_NIGHTMODE_FRAME_RATE		(13)

    // 0x3028, 0x3029 defines the PCLKs in one line of OV5650MIPI
    // If [0x3028:0x3029] = N, the total PCLKs in one line of QXGA(3M full mode) is (N+1), and
    // total PCLKs in one line of XGA subsampling mode is (N+1) / 2
    // If need to add dummy pixels, just increase 0x3028 and 0x3029 directly
    #define OV5650MIPI_QXGA_MODE_WITHOUT_DUMMY_PIXELS  (3252) //(3112)  // QXGA mode's pixel # in one HSYNC w/o dummy pixels
    #define OV5650MIPI_XGA_MODE_WITHOUT_DUMMY_PIXELS   (2176)  //(OV5650MIPI_QXGA_MODE_WITHOUT_DUMMY_PIXELS / 2)    // XGA mode's pixel # in one HSYNC w/o dummy pixels
    #define OV5650MIPI_PV_PERIOD_PIXEL_NUMS    (OV5650MIPI_XGA_MODE_WITHOUT_DUMMY_PIXELS) // dummy pixels is down sampled by half in XGA mode

    // 0x302A, 0x302B defines total lines in one frame of OV5650MIPI
    // If [0x302A:0x302B] = N, the total lines in one is N in dependent of resolution setting
    // Even in XGA subsampling mode, total lines defined by 0x302A, 0x302B is not subsampled.
    // If need dummy lines, just increase 0x302A and 0x302B directly
    #define OV5650MIPI_QXGA_MODE_WITHOUT_DUMMY_LINES   (1968)//(1568)  // QXGA mode's HSYNC # in one HSYNC w/o dummy lines
    #define OV5650MIPI_XGA_MODE_WITHOUT_DUMMY_LINES    (1019)  //(984)//(784)   // XGA mode's HSYNC # in one HSYNC w/o dummy lines
    #define OV5650MIPI_PV_PERIOD_LINE_NUMS     (OV5650MIPI_XGA_MODE_WITHOUT_DUMMY_LINES)  // XGA mode's HSYNC # in one VSYNC period

    // SENSOR EXPOSURE LINE LIMITATION
    #define OV5650MIPI_FULL_MAX_LINES_PER_FRAME    (1944)  // QXGA mode 1568
    #define OV5650MIPI_FULL_EXPOSURE_LIMITATION    (OV5650MIPI_FULL_MAX_LINES_PER_FRAME)
    #define OV5650MIPI_PV_MAX_LINES_PER_FRAME  (OV5650MIPI_XGA_MODE_WITHOUT_DUMMY_LINES)  // # of lines in one XGA frame
    #define OV5650MIPI_PV_EXPOSURE_LIMITATION  (OV5650MIPI_PV_MAX_LINES_PER_FRAME)

    // sensor's full resolution
    #define OV5650MIPI_IMAGE_SENSOR_FULL_WIDTH     (2592) // (2584)
    #define OV5650MIPI_IMAGE_SENSOR_FULL_HEIGHT    (1944) //(1944)

    // sensor's full resolution
    #define OV5650MIPI_IMAGE_SENSOR_FULL_WIDTH_DRV     (2592 - 2 - (OV5650MIPI_ISP_INTERPOLATIO_FILTER_WIDTH - 1)) 
    #define OV5650MIPI_IMAGE_SENSOR_FULL_HEIGHT_DRV    (1944 - 2 - (OV5650MIPI_ISP_INTERPOLATIO_FILTER_HEIGHT - 1)) 

    // resolution for preview
    #define OV5650MIPI_IMAGE_SENSOR_PV_WIDTH   (1280)//(1024)//640
    #define OV5650MIPI_IMAGE_SENSOR_PV_HEIGHT  (960)//(768)//480
    
//   #define ISP_INTERPOLATIO_FILTER_WIDTH   (7)
//   #define ISP_INTERPOLATIO_FILTER_HEIGHT  (7)
        #define OV5650MIPI_ISP_INTERPOLATIO_FILTER_WIDTH   (15)//5//11
        #define OV5650MIPI_ISP_INTERPOLATIO_FILTER_HEIGHT  (11)//7//14

    /* SENSOR READ/WRITE ID */
    #define OV5650MIPI_WRITE_ID (0x6C)
    #define OV5650MIPI_READ_ID  (0x6D)

    /* SENSOR CHIP VERSION */
//    #define OV5650MIPI_SENSOR_ID    (0x3641)    // rev.2A
    //#define OV5650MIPI_SENSOR_ID    0x364C    // rev.2C


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
#define OV5650MIPI_PV_FORMAT_INDEX   1 //1280x960 mode
#define OV5650MIPI_FULL_FORMAT_INDEX 2 //Full resolution mode

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
#define CAM_SIZE_5M_WIDTH 		2592
#define CAM_SIZE_5M_HEIGHT 		1944

//
//initial config function
//
int pfInitCfg_VGA(void);
int pfInitCfg_1280X960(void);
int pfInitCfg_FULL(void);

//export functions
UINT32 OV5650MIPIOpen(void);
UINT32 OV5650MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV5650MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 OV5650MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV5650MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV5650MIPIClose(void);

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT
//e_porting add
//e_porting add
//e_porting add



#endif /* __SENSOR_H */ 
