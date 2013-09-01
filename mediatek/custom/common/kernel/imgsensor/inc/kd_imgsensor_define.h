#ifndef _KD_IMGSENSOR_DATA_H
#define _KD_IMGSENSOR_DATA_H

//#include "../camera/kd_camera_hw.h"
#include "kd_camera_feature.h"

#define SENSOR_CLOCK_POLARITY_HIGH     0
#define SENSOR_CLOCK_POLARITY_LOW      1

/*************************************************
*
**************************************************/
//In KERNEL mode,SHOULD be sync with mediatype.h
//CHECK before remove or modify
//#undef BOOL
//#define BOOL signed int
#ifndef _MEDIA_TYPES_H
typedef unsigned char   MUINT8;
typedef unsigned short  MUINT16;
typedef unsigned int    MUINT32;
typedef signed char     MINT8;
typedef signed short    MINT16;
typedef signed int      MINT32;
#endif

/*******************************************************************************
*
********************************************************************************/
//
//msdk_isp_exp.h
//
#define BASEGAIN 0x40
#define BASEGAIN_SHIFT 6

typedef enum
{
    ISP_DRIVING_2MA=0,
    ISP_DRIVING_4MA,
    ISP_DRIVING_6MA,
    ISP_DRIVING_8MA
} ISP_DRIVING_CURRENT_ENUM;



enum
{
    IMAGE_NORMAL=0,
    IMAGE_H_MIRROR,
    IMAGE_V_MIRROR,
    IMAGE_HV_MIRROR 
};


#if 0
// defined the enum for enumerating the ISO/Binning information about each ISO mode.
typedef enum
{
  ISO_100_MODE =0,
  ISO_200_MODE,
  ISO_400_MODE,
  ISO_800_MODE,
  ISO_1600_MODE,
  ISO_MAX_MODE
} ACDK_ISP_ISO_ENUM;

typedef struct
{
  MUINT32              MaxWidth;
  MUINT32              MaxHeight;
  MINT32            ISOSupported;
  MINT32                BinningEnable;
} ACDK_ISP_BINNING_INFO_STRUCT, *PACDK_ISP_BINNING_INFO_STRUCT;
//#endif /* __MSDK_ISP_Feature_H */


typedef struct
{
    ACDK_ISP_BINNING_INFO_STRUCT    ISOBinningInfo[ISO_MAX_MODE];
} CAMERA_ISO_BINNING_INFO_STRUCT, *PCAMERA_ISO_BINNING_INFO_STRUCT;

#endif

/*
typedef enum
{
    ACDK_SCENARIO_ID_CAMERA_PREVIEW=0,
    ACDK_SCENARIO_ID_VIDEO_PREVIEW,
    ACDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4,
    ACDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG,
    ACDK_SCENARIO_ID_CAMERA_CAPTURE_MEM,
    ACDK_SCENARIO_ID_CAMERA_BURST_CAPTURE_JPEG,
    ACDK_SCENARIO_ID_VIDEO_DECODE_MPEG4,
    ACDK_SCENARIO_ID_VIDEO_DECODE_H263,
    ACDK_SCENARIO_ID_VIDEO_DECODE_H264,
    ACDK_SCENARIO_ID_VIDEO_DECODE_WMV78,
    ACDK_SCENARIO_ID_VIDEO_DECODE_WMV9,
    ACDK_SCENARIO_ID_VIDEO_DECODE_MPEG2,
    ACDK_SCENARIO_ID_IMAGE_YUV2RGB,
    ACDK_SCENARIO_ID_IMAGE_RESIZE,
    ACDK_SCENARIO_ID_IMAGE_ROTATE,
    ACDK_SCENARIO_ID_IMAGE_POST_PROCESS,
    ACDK_SCENARIO_ID_JPEG_RESIZE,
    ACDK_SCENARIO_ID_JPEG_DECODE,
    ACDK_SCENARIO_ID_JPEG_PARSE,
    ACDK_SCENARIO_ID_JPEG_ENCODE,
    ACDK_SCENARIO_ID_JPEG_ENCODE_THUMBNAIL,
    ACDK_SCENARIO_ID_DRIVER_IO_CONTROL,
    ACDK_SCENARIO_ID_DO_NOT_CARE,
    ACDK_SCENARIO_ID_IMAGE_DSPL_BUFFER_ALLOC,
    ACDK_SCENARIO_ID_TV_OUT,
    ACDK_SCENARIO_ID_VIDOE_ENCODE_WITHOUT_PREVIEW,      // for LTK test case
    ACDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG_BACK_PREVIEW,  // for LTK test case
    ACDK_SCENARIO_ID_VIDEO_DECODE_RV8,
    ACDK_SCENARIO_ID_VIDEO_DECODE_RV9,
    ACDK_SCENARIO_ID_CAMERA_ZSD,
    ACDK_SCENARIO_ID_CAMERA_3D_PREVIEW,
    ACDK_SCENARIO_ID_CAMERA_3D_CAPTURE,
    ACDK_SCENARIO_ID_MAX,    
}   ACDK_SCENARIO_ID_ENUM;
*/


typedef enum
{
    MSDK_SCENARIO_ID_CAMERA_PREVIEW=0,
    MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG,
    MSDK_SCENARIO_ID_VIDEO_PREVIEW,
    MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO,
    MSDK_SCENARIO_ID_CAMERA_ZSD,
    MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW,
    MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE,
    MSDK_SCENARIO_ID_CAMERA_3D_VIDEO,
    MSDK_SCENARIO_ID_TV_OUT,
    MSDK_SCENARIO_ID_MAX,    
}   MSDK_SCENARIO_ID_ENUM;


typedef enum
{
    MSDK_CAMERA_OPERATION_NORMAL_MODE=0,
    MSDK_CAMERA_OPERATION_META_MODE
} ACDK_CAMERA_OPERATION_MODE_ENUM;


/*******************************************************************************
*
********************************************************************************/

//
#define MAX_NUM_OF_SUPPORT_SENSOR 16
//
#define SENSOR_CLOCK_POLARITY_HIGH    0
#define SENSOR_CLOCK_POLARITY_LOW 1
//
#define LENS_DRIVER_ID_DO_NOT_CARE    0xFFFFFFFF
#define SENSOR_DOES_NOT_EXIST     0x00FFFFFF
#define SENSOR_DOES_NOT_KNOW      0xFFFFFFFF

#define SENSOR_FEATURE_START                     3000
typedef enum
{
  SENSOR_FEATURE_BEGIN = SENSOR_FEATURE_START,
  SENSOR_FEATURE_GET_RESOLUTION,
  SENSOR_FEATURE_GET_PERIOD,
  SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ,
  SENSOR_FEATURE_SET_ESHUTTER,
  SENSOR_FEATURE_SET_NIGHTMODE,
  SENSOR_FEATURE_SET_GAIN,
  SENSOR_FEATURE_SET_FLASHLIGHT,
  SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ,
  SENSOR_FEATURE_SET_REGISTER,
  SENSOR_FEATURE_GET_REGISTER,
  SENSOR_FEATURE_SET_CCT_REGISTER,
  SENSOR_FEATURE_GET_CCT_REGISTER,
  SENSOR_FEATURE_SET_ENG_REGISTER,
  SENSOR_FEATURE_GET_ENG_REGISTER,
  SENSOR_FEATURE_GET_REGISTER_DEFAULT,
  SENSOR_FEATURE_GET_CONFIG_PARA,
  SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR,
  SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA,
  SENSOR_FEATURE_GET_GROUP_COUNT,
  SENSOR_FEATURE_GET_GROUP_INFO,
  SENSOR_FEATURE_GET_ITEM_INFO,
  SENSOR_FEATURE_SET_ITEM_INFO,
  SENSOR_FEATURE_GET_ENG_INFO,
  SENSOR_FEATURE_GET_LENS_DRIVER_ID,
  SENSOR_FEATURE_SET_YUV_CMD,
  SENSOR_FEATURE_SET_VIDEO_MODE,   
  SENSOR_FEATURE_SET_CALIBRATION_DATA,
  SENSOR_FEATURE_SET_SENSOR_SYNC,     
  SENSOR_FEATURE_INITIALIZE_AF,
  SENSOR_FEATURE_CONSTANT_AF,
  SENSOR_FEATURE_MOVE_FOCUS_LENS,
  SENSOR_FEATURE_GET_AF_STATUS,
  SENSOR_FEATURE_GET_AF_INF,
  SENSOR_FEATURE_GET_AF_MACRO,
  SENSOR_FEATURE_CHECK_SENSOR_ID, 
  SENSOR_FEATURE_SET_AUTO_FLICKER_MODE,
  SENSOR_FEATURE_SET_TEST_PATTERN,
  SENSOR_FEATURE_SET_SOFTWARE_PWDN,  
  SENSOR_FEATURE_SINGLE_FOCUS_MODE,
  SENSOR_FEATURE_CANCEL_AF,
  SENSOR_FEATURE_SET_AF_WINDOW,
  SENSOR_FEATURE_GET_EV_AWB_REF,
  SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN,
  SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS,
  SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS,
  SENSOR_FEATURE_SET_AE_WINDOW,
  SENSOR_FEATURE_GET_EXIF_INFO,
  SENSOR_FEATURE_GET_DELAY_INFO,
  SENSOR_FEATURE_SET_SLAVE_I2C_ID,
  SENSOR_FEATURE_SUSPEND,
  SENSOR_FEATURE_RESUME,
  SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO,
  SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO,
  SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO,
  SENSOR_FEATURE_GET_AE_FLASHLIGHT_INFO,
  SENSOR_FEATURE_AUTOTEST_CMD,
  SENSOR_FEATURE_MAX
} ACDK_SENSOR_FEATURE_ENUM;

typedef enum
{
  SENSOR_AF_IDLE=0,
  SENSOR_AF_FOCUSING,
  SENSOR_AF_FOCUSED,
  SENSOR_AF_ERROR,
  SENSOR_AF_SCENE_DETECTING,
  SENSOR_AF_STATUS_MAX
} ACDK_SENSOR_AF_STATUS_ENUM;

typedef enum
{
  SENSOR_INTERFACE_TYPE_PARALLEL=0,
  SENSOR_INTERFACE_TYPE_MIPI,
  SENSOR_INTERFACE_TYPE_MAX
} ACDK_SENSOR_INTERFACE_TYPE_ENUM;

typedef enum
{
  SENSOR_OUTPUT_FORMAT_RAW_B=0,
  SENSOR_OUTPUT_FORMAT_RAW_Gb,
  SENSOR_OUTPUT_FORMAT_RAW_Gr,
  SENSOR_OUTPUT_FORMAT_RAW_R,
  SENSOR_OUTPUT_FORMAT_UYVY,
  SENSOR_OUTPUT_FORMAT_VYUY,
  SENSOR_OUTPUT_FORMAT_YUYV,
  SENSOR_OUTPUT_FORMAT_YVYU,
  SENSOR_OUTPUT_FORMAT_CbYCrY,
  SENSOR_OUTPUT_FORMAT_CrYCbY,
  SENSOR_OUTPUT_FORMAT_YCbYCr,
  SENSOR_OUTPUT_FORMAT_YCrYCb,
  SENSOR_OUTPUT_FORMAT_RAW8_B,
  SENSOR_OUTPUT_FORMAT_RAW8_Gb,
  SENSOR_OUTPUT_FORMAT_RAW8_Gr,
  SENSOR_OUTPUT_FORMAT_RAW8_R,  
} ACDK_SENSOR_OUTPUT_DATA_FORMAT_ENUM;

typedef enum
{
  SENSOR_MIPI_1_LANE=0,
  SENSOR_MIPI_2_LANE,
  SENSOR_MIPI_3_LANE,
  SENSOR_MIPI_4_LANE
} ACDK_SENSOR_MIPI_LANE_NUMBER_ENUM;

typedef struct
{
  MUINT16 SensorPreviewWidth;
  MUINT16 SensorPreviewHeight;
  MUINT16 SensorFullWidth;
  MUINT16 SensorFullHeight;
  MUINT16 SensorVideoWidth;
  MUINT16 SensorVideoHeight;
  MUINT16 SensorHighSpeedVideoWidth;
  MUINT16 SensorHighSpeedVideoHeight;
  MUINT16 Sensor3DPreviewWidth;
  MUINT16 Sensor3DPreviewHeight;
  MUINT16 Sensor3DFullWidth;
  MUINT16 Sensor3DFullHeight;
  MUINT16 Sensor3DVideoWidth;
  MUINT16 Sensor3DVideoHeight;  
} ACDK_SENSOR_RESOLUTION_INFO_STRUCT, *PACDK_SENSOR_RESOLUTION_INFO_STRUCT;


typedef struct
{
  MUINT16 SensorPreviewResolutionX;
  MUINT16 SensorPreviewResolutionY;
  MUINT16 SensorFullResolutionX;
  MUINT16 SensorFullResolutionY;
  MUINT8 SensorClockFreq;              /* MHz */
  MUINT8 SensorCameraPreviewFrameRate;
  MUINT8 SensorVideoFrameRate;
  MUINT8 SensorStillCaptureFrameRate;
  MUINT8 SensorWebCamCaptureFrameRate;
  MUINT8 SensorClockPolarity;          /* SENSOR_CLOCK_POLARITY_HIGH/SENSOR_CLOCK_POLARITY_Low */
  MUINT8 SensorClockFallingPolarity;
  MUINT8 SensorClockRisingCount;       /* 0..15 */
  MUINT8 SensorClockFallingCount;      /* 0..15 */
  MUINT8 SensorClockDividCount;        /* 0..15 */
  MUINT8 SensorPixelClockCount;        /* 0..15 */
  MUINT8 SensorDataLatchCount;         /* 0..15 */
  MUINT8 SensorHsyncPolarity;
  MUINT8 SensorVsyncPolarity;
  MUINT8 SensorInterruptDelayLines;
  MINT32  SensorResetActiveHigh;
  MUINT32 SensorResetDelayCount;
  ACDK_SENSOR_INTERFACE_TYPE_ENUM SensroInterfaceType;
  ACDK_SENSOR_OUTPUT_DATA_FORMAT_ENUM SensorOutputDataFormat;
  ACDK_SENSOR_MIPI_LANE_NUMBER_ENUM SensorMIPILaneNumber;
  MUINT32 CaptureDelayFrame; 
  MUINT32 PreviewDelayFrame;   
  MUINT32 VideoDelayFrame; 
  MUINT32 YUVAwbDelayFrame;
  MUINT32 YUVEffectDelayFrame;
  MUINT16 SensorGrabStartX;
  MUINT16 SensorGrabStartY; 
  MUINT16 SensorDrivingCurrent;   
  MUINT8   SensorMasterClockSwitch; 
  MUINT8   AEShutDelayFrame;    	     /* The frame of setting shutter default 0 for TG int */
  MUINT8   AESensorGainDelayFrame;   /* The frame of setting sensor gain */
  MUINT8   AEISPGainDelayFrame;
  MUINT8   MIPIDataLowPwr2HighSpeedTermDelayCount; 
  MUINT8   MIPIDataLowPwr2HighSpeedSettleDelayCount; 
  MUINT8   MIPICLKLowPwr2HighSpeedTermDelayCount; 
  MUINT8   SensorWidthSampling;
  MUINT8   SensorHightSampling;  
  MUINT8   SensorPacketECCOrder;
} ACDK_SENSOR_INFO_STRUCT, *PACDK_SENSOR_INFO_STRUCT;


typedef enum {
    ACDK_CCT_REG_ISP = 0,
    ACDK_CCT_REG_CMOS,
    ACDK_CCT_REG_CCD
} ACDK_CCT_REG_TYPE_ENUM;


/* R/W ISP/Sensor Register */
typedef struct
{
    ACDK_CCT_REG_TYPE_ENUM Type;
    MUINT32 RegAddr;
    MUINT32 RegData;
}ACDK_CCT_REG_RW_STRUCT, *PACDK_CCT_REG_RW_STRUCT;

typedef struct
{
    ACDK_CCT_REG_TYPE_ENUM Type;                    // ISP, CMOS_SENSOR, CCD_SENSOR
    MUINT32	DeviceId;
    ACDK_SENSOR_OUTPUT_DATA_FORMAT_ENUM	StartPixelBayerPtn;
    MUINT16   GrabXOffset;
    MUINT16   GrabYOffset;
} ACDK_CCT_SENSOR_INFO_STRUCT, *PACDK_CCT_SENSOR_INFO_STRUCT;



typedef enum
{
  CMOS_SENSOR=0,
  CCD_SENSOR
} SENSOR_TYPE_ENUM;

typedef struct
{
  MUINT16              SensorId;
  SENSOR_TYPE_ENUM    SensorType;
  ACDK_SENSOR_OUTPUT_DATA_FORMAT_ENUM SensorOutputDataFormat;
} ACDK_SENSOR_ENG_INFO_STRUCT;

typedef struct
{
  MUINT32 RegAddr;
  MUINT32 RegData;
} ACDK_SENSOR_REG_INFO_STRUCT;

typedef struct
{
  MUINT32 GroupIdx;
  MUINT32 ItemCount;
  MUINT8 *GroupNamePtr;
} ACDK_SENSOR_GROUP_INFO_STRUCT;

typedef struct
{
  MUINT32  GroupIdx;
  MUINT32  ItemIdx;
  MUINT8   ItemNamePtr[50];        // item name
  MUINT32  ItemValue;              // item value
  MINT32    IsTrueFalse;            // is this item for enable/disable functions
  MINT32    IsReadOnly;             // is this item read only
  MINT32    IsNeedRestart;          // after set this item need restart
  MUINT32  Min;                    // min value of item value
  MUINT32  Max;                    // max value of item value
}ACDK_SENSOR_ITEM_INFO_STRUCT;

typedef enum
{
	ACDK_SENSOR_IMAGE_NORMAL=0,
	ACDK_SENSOR_IMAGE_H_MIRROR,
	ACDK_SENSOR_IMAGE_V_MIRROR,
	ACDK_SENSOR_IMAGE_HV_MIRROR
}ACDK_SENSOR_IMAGE_MIRROR_ENUM;

typedef enum
{
	ACDK_SENSOR_OPERATION_MODE_CAMERA_PREVIEW=0,
	ACDK_SENSOR_OPERATION_MODE_VIDEO,
	ACDK_SENSOR_OPERATION_MODE_STILL_CAPTURE,
	ACDK_SENSOR_OPERATION_MODE_WEB_CAPTURE,
	ACDK_SENSOR_OPERATION_MODE_MAX
} ACDK_SENSOR_OPERATION_MODE_ENUM;

typedef struct
{
  MUINT16 GrabStartX;              /* The first grabed column data of the image sensor in pixel clock count */
  MUINT16 GrabStartY;              /* The first grabed row data of the image sensor in pixel clock count */
  MUINT16 ExposureWindowWidth;     /* Exposure window width of image sensor */
  MUINT16 ExposureWindowHeight;    /* Exposure window height of image sensor */
  MUINT16 ImageTargetWidth;        /* image captured width */
  MUINT16 ImageTargetHeight;       /* image captuerd height */
  MUINT16 ExposurePixel;           /* exposure window width of image sensor + dummy pixel */
  MUINT16 CurrentExposurePixel;    /* exposure window width of image sensor + dummy pixel */
  MUINT16 ExposureLine;            /* exposure window width of image sensor + dummy line */
  MUINT16  ZoomFactor;              /* digital zoom factor */
} ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT;

typedef struct
{
	ACDK_SENSOR_IMAGE_MIRROR_ENUM	SensorImageMirror;
	MINT32	EnableShutterTansfer;			/* capture only */
	MINT32	EnableFlashlightTansfer;		/* flash light capture only */
	ACDK_SENSOR_OPERATION_MODE_ENUM	SensorOperationMode;
	MUINT16  ImageTargetWidth;		/* image captured width */
	MUINT16  ImageTargetHeight;		/* image captuerd height */
	MUINT16	CaptureShutter;			/* capture only */
	MUINT16	FlashlightDuty;			/* flash light capture only */
	MUINT16	FlashlightOffset;		/* flash light capture only */
	MUINT16	FlashlightShutFactor;	/* flash light capture only */
	MUINT16 	FlashlightMinShutter;
	ACDK_CAMERA_OPERATION_MODE_ENUM 	MetaMode; /* capture only */
	MUINT32  DefaultPclk;       // Sensor pixel clock(Ex:24000000)
	MUINT32  Pixels;             // Sensor active pixel number
	MUINT32  Lines;              // Sensor active line number
	MUINT32  Shutter;            // Sensor current shutter
	MUINT32  FrameLines;      //valid+dummy lines for minimum shutter
}	ACDK_SENSOR_CONFIG_STRUCT;


/*******************************************************************************
*
********************************************************************************/

#define MAXIMUM_NVRAM_CAMERA_SENSOR_FILE_SIZE       4096

#define NVRAM_CAMERA_SENSOR_FILE_VERSION        1



// Sensor table
#define MAXIMUM_SENSOR_CCT_REG_NUMBER   100
#define MAXIMUM_SENSOR_ENG_REG_NUMBER   100

typedef struct
{
    MUINT32  Addr;
    MUINT32  Para;
} SENSOR_REG_STRUCT;

typedef struct
{
    MUINT32 Version;
    MUINT32 SensorId;        // ID of sensor module
    SENSOR_REG_STRUCT   SensorEngReg[MAXIMUM_SENSOR_ENG_REG_NUMBER];
    SENSOR_REG_STRUCT   SensorCCTReg[MAXIMUM_SENSOR_CCT_REG_NUMBER];
    MUINT8 CameraData[MAXIMUM_NVRAM_CAMERA_SENSOR_FILE_SIZE/2-8-sizeof(SENSOR_REG_STRUCT)*(MAXIMUM_SENSOR_ENG_REG_NUMBER+MAXIMUM_SENSOR_CCT_REG_NUMBER)];
} NVRAM_SENSOR_DATA_STRUCT, *PNVRAM_SENSOR_DATA_STRUCT;

#define MAX_SENSOR_CAL_SIZE     (1024) //Byte
#define MAX_SHADING_DATA_TBL ((MAX_SENSOR_CAL_SIZE-8)/4)
typedef struct
{
	MUINT32 DataFormat;
	MUINT32 DataSize;
	MUINT32 ShadingData[MAX_SHADING_DATA_TBL];
} SET_SENSOR_CALIBRATION_DATA_STRUCT, *PSET_SENSOR_CALIBRATION_DATA_STRUCT;



typedef struct{
    MSDK_SCENARIO_ID_ENUM ScenarioId[2];
    ACDK_SENSOR_INFO_STRUCT *pInfo[2];
    ACDK_SENSOR_CONFIG_STRUCT *pConfig[2];
}ACDK_SENSOR_GETINFO_STRUCT, *PACDK_SENSOR_GETINFO_STRUCT;



typedef struct{
	CAMERA_DUAL_CAMERA_SENSOR_ENUM InvokeCamera;
    ACDK_SENSOR_FEATURE_ENUM FeatureId;
    MUINT8  *pFeaturePara;
    MUINT32 *pFeatureParaLen;
}ACDK_SENSOR_FEATURECONTROL_STRUCT, *PACDK_SENSOR_FEATURECONTROL_STRUCT;

typedef struct{
    MSDK_SCENARIO_ID_ENUM ScenarioId;
    ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow;
	ACDK_SENSOR_CONFIG_STRUCT *pSensorConfigData;
}ACDK_SENSOR_CONTROL_STRUCT;



typedef struct regval_list {
	MUINT32 reg_addr;
	MUINT32 value;
	MUINT32 bytes;
}REGVAL_LIST_STRUCT;

#define KDIMGSENSOR_REGVAL_LIST_MAX_NUM 256

typedef struct format_struct{
	MUINT8 *desc;
	MUINT32 pixelformat;
//	REGVAL_LIST_STRUCT regs[KDIMGSENSOR_REGVAL_LIST_MAX_NUM];
	int (*pfInitCfg)(void);
}IMGSENSOR_FORMAT_STRUCT;

typedef struct {
    IMGSENSOR_FORMAT_STRUCT format;
    MUINT32 u4InClk; // Common part                                     //hard coded
    MUINT32 u4OutClk; // Common part                                    //
    MUINT32 u4TotalPixelPerLine; //By modes
    MUINT32 u4TotalLinesPerFrame; //By modes and frame rate setting
    MUINT32 u4ActualWidth;// By modes
    MUINT32 u4ActualHeight;// By modes
    MUINT32 u4Width; //By modes
    MUINT32 u4Height; //By modes
    MUINT32 u4FrameTimeInus; //By modes and frame rate setting
    MUINT32 u4MinFrameTimeInus;//By modes
    MUINT32 u4LineTimeInus; // By modes
    MUINT32 u4FinePixCntPerus; //Common part
    MUINT32 u4MinFineTimeInus; //By modes
    MUINT32 u4MaxFineTimeInus; //By modes
    MUINT32 u4XStart;
    MUINT32 u4XEnd;
    MUINT32 u4YStart;
    MUINT32 u4YEnd;
} stImgSensorFormat;

/*******************************************************************************
*
********************************************************************************/
//
//adoption to winmo driver files
//

//typedef
#define kal_uint8 u8

//#define MSDK_SCENARIO_ID_ENUM               ACDK_SCENARIO_ID_ENUM
#define MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT  ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT
#define MSDK_SENSOR_CONFIG_STRUCT           ACDK_SENSOR_CONFIG_STRUCT

#define MSDK_SENSOR_FEATURE_ENUM            ACDK_SENSOR_FEATURE_ENUM
#define MSDK_SENSOR_REG_INFO_STRUCT         ACDK_SENSOR_REG_INFO_STRUCT
#define MSDK_SENSOR_GROUP_INFO_STRUCT       ACDK_SENSOR_GROUP_INFO_STRUCT
#define MSDK_SENSOR_ITEM_INFO_STRUCT        ACDK_SENSOR_ITEM_INFO_STRUCT
#define MSDK_SENSOR_ENG_INFO_STRUCT         ACDK_SENSOR_ENG_INFO_STRUCT
#define MSDK_SENSOR_INFO_STRUCT             ACDK_SENSOR_INFO_STRUCT
#define MSDK_SENSOR_RESOLUTION_INFO_STRUCT  ACDK_SENSOR_RESOLUTION_INFO_STRUCT

//MSDK_SCENARIO_ID_ENUM => ACDK_SCENARIO_ID_ENUM
//#define MSDK_SCENARIO_ID_CAMERA_PREVIEW         ACDK_SCENARIO_ID_CAMERA_PREVIEW
//#define MSDK_SCENARIO_ID_VIDEO_PREVIEW          ACDK_SCENARIO_ID_VIDEO_PREVIEW
//#define MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG    ACDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG
#define MSDK_SENSOR_OPERATION_MODE_VIDEO	    ACDK_SENSOR_OPERATION_MODE_VIDEO
//#define MSDK_SCENARIO_ID_CAMERA_ZSD				ACDK_SCENARIO_ID_CAMERA_ZSD
//#define MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW		ACDK_SCENARIO_ID_CAMERA_3D_PREVIEW
//#define MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE		ACDK_SCENARIO_ID_CAMERA_3D_CAPTURE
//#define MSDK_SCENARIO_ID_CAMERA_3D_VIDEO		ACDK_SCENARIO_ID_CAMERA_3D_VIDEO
/*******************************************************************************
*
********************************************************************************/
   
/*******************************************************************************
*
********************************************************************************/
//for new simplifed sensor driver
typedef struct
{
   MUINT32 (* SensorOpen)(void);
   MUINT32 (* SensorGetInfo) (MUINT32 *pScenarioId[2], MSDK_SENSOR_INFO_STRUCT *pSensorInfo[2],
                               MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData[2]);
   MUINT32 (* SensorGetResolution) (MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution[2]);
   MUINT32 (* SensorFeatureControl) (CAMERA_DUAL_CAMERA_SENSOR_ENUM InvokeCamera,MSDK_SENSOR_FEATURE_ENUM FeatureId, MUINT8 *pFeaturePara,MUINT32 *pFeatureParaLen);
   MUINT32 (* SensorControl) (MSDK_SCENARIO_ID_ENUM ScenarioId,MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
   MUINT32 (* SensorClose)(void);
} MULTI_SENSOR_FUNCTION_STRUCT, *PMULTI_SENSOR_FUNCTION_STRUCT;


typedef struct
{
   MUINT32 (* SensorOpen)(void);
   MUINT32 (* SensorGetInfo) (MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                               MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
   MUINT32 (* SensorGetResolution) (MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
   MUINT32 (* SensorFeatureControl) (MSDK_SENSOR_FEATURE_ENUM FeatureId, MUINT8 *pFeaturePara,MUINT32 *pFeatureParaLen);
   MUINT32 (* SensorControl) (MSDK_SCENARIO_ID_ENUM ScenarioId,MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
   MUINT32 (* SensorClose)(void);
} SENSOR_FUNCTION_STRUCT, *PSENSOR_FUNCTION_STRUCT;

typedef struct
{
	MUINT32 SensorId;
	MUINT8  drvname[32];
    MUINT32 (* SensorInit)(PSENSOR_FUNCTION_STRUCT *pfFunc);
} ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT, *PACDK_KD_SENSOR_INIT_FUNCTION_STRUCT;

#define KDIMGSENSOR_DUAL_SHIFT 16
#define KDIMGSENSOR_DUAL_MASK_MSB 0xFFFF0000
#define KDIMGSENSOR_DUAL_MASK_LSB 0x0000FFFF

#define KDIMGSENSOR_NOSENSOR    "non_sensor"

#define KDIMGSENSOR_MAX_INVOKE_DRIVERS  (2)
#define KDIMGSENSOR_INVOKE_DRIVER_0     (0)
#define KDIMGSENSOR_INVOKE_DRIVER_1     (1)

// For sensor synchronize the exposure time / sensor gain and isp gain.
typedef struct
{
MUINT16 u2ISPNewRGain;
MUINT16 u2ISPNewGrGain;
MUINT16 u2ISPNewGbGain;
MUINT16 u2ISPNewBGain;
MUINT16 u2SensorNewExpTime;
MUINT16 u2SensorNewGain;
MUINT8 uSensorExpDelayFrame;
MUINT8 uSensorGainDelayFrame;
MUINT8 uISPGainDelayFrame;
MUINT8 uDummy;
}ACDK_KD_SENSOR_SYNC_STRUCT, *PACDK_KD_SENSOR_SYNC_STRUCT;


typedef struct
{
    MUINT16 AeRefLV05Shutter; /* Sensor AE Shutter under Lv05 */
    MUINT16 AeRefLV13Shutter; /* Sensor AE Shutter under Lv13 */
    MUINT16 AeRefLV05Gain; /* Sensor AE Gain under Lv05 */
    MUINT16 AeRefLV13Gain; /* Sensor AE Gain under Lv13 */
} SENSOR_AE_REF_STRUCT, *PSENSOR_AE_REF_STRUCT;


typedef struct
{
    MUINT16 AwbRefD65Rgain; /* Sensor AWB R Gain under D65 */
    MUINT16 AwbRefD65Bgain; /* Sensor AWB B Gain under D65 */
    MUINT16 AwbRefCWFRgain; /* Sensor AWB R Gain under CWF */
    MUINT16 AwbRefCWFBgain; /* Sensor AWB B Gain under CWF */
} SENSOR_AWB_GAIN_REF_STRUCT, *PSENSOR_AWB_GAIN_REF_STRUCT;


typedef struct
{
    SENSOR_AE_REF_STRUCT  SensorAERef; /* AE Ref information for ASD usage */
    SENSOR_AWB_GAIN_REF_STRUCT  SensorAwbGainRef;  /* AWB Gain Ref information for ASD usage */
    MUINT32	SensorLV05LV13EVRef; /* EV calculate  for ASD usage */
} SENSOR_AE_AWB_REF_STRUCT, *PSENSOR_AE_AWB_REF_STRUCT;



typedef struct
{
    MUINT16 AeCurShutter; /* Current Sensor AE Shutter */
    MUINT16 AeCurGain; /* Current Sensor AE Gain */
} SENSOR_AE_CUR_STRUCT, *PSENSOR_AE_CUR_STRUCT;


typedef struct
{
    MUINT16 AwbCurRgain; /* Current Sensor AWB R Gain */
    MUINT16 AwbCurBgain; /* Current Sensor AWB R Gain */
} SENSOR_AWB_GAIN_CUR_STRUCT, *PSENSOR_AWB_GAIN_CUR_STRUCT;


typedef struct
{
    SENSOR_AE_CUR_STRUCT  SensorAECur; /* AE Current information for ASD usage */
    SENSOR_AWB_GAIN_CUR_STRUCT  SensorAwbGainCur;  /* AWB Gain Current information for ASD usage */
} SENSOR_AE_AWB_CUR_STRUCT, *PSENSOR_AE_AWB_CUR_STRUCT;


typedef struct
{
    MUINT32 FNumber;
    MUINT32 AEISOSpeed;
    MUINT32 AWBMode;
    MUINT32 CapExposureTime;
    MUINT32 FlashLightTimeus;
    MUINT32 RealISOValue;
}SENSOR_EXIF_INFO_STRUCT, *PSENSOR_EXIF_INFO_STRUCT;

typedef struct
{
    MUINT32 InitDelay;
    MUINT32 EffectDelay;
    MUINT32 AwbDelay;
    MUINT32 AFSwitchDelayFrame;
}SENSOR_DELAY_INFO_STRUCT, *PSENSOR_DELAY_INFO_STRUCT;

typedef struct
{
    MUINT32 u4Fno;
    MUINT32 Exposuretime;
    MUINT32 Gain;
    MUINT32 GAIN_BASE;
}SENSOR_FLASHLIGHT_AE_INFO_STRUCT, *PSENSOR_FLASHLIGHT_AE_INFO_STRUCT;

//multisensor driver

typedef struct
{
    MUINT32 drvIndex[KDIMGSENSOR_MAX_INVOKE_DRIVERS]; /* max 2 driver sumultaneously */
} SENSOR_DRIVER_INDEX_STRUCT, *PSENSOR_DRIVER_INDEX_STRUCT;

//hardcode by GPIO module, should be sync with.(cust_gpio_usage.h)
#define GPIO_CAMERA_INVALID 0xFF
//
typedef enum {
    IMGSENSOR_SOCKET_POS_NONE   = 0xFFFFFFFF,
    IMGSENSOR_SOCKET_POS_RIGHT  = 0x1,
    IMGSENSOR_SOCKET_POS_LEFT   = 0x2,
} IMGSENSOR_SOCKET_POSITION_ENUM;
//
typedef enum {
    IMGSENSOR_SET_I2C_ID_STATE  = 0x00,
    IMGSENSOR_SET_I2C_ID_FORCE
} IMGSENSOR_SET_I2C_ID_ENUM;

#endif //_KD_IMGSENSOR_DATA_H


