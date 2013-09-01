/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ov5648mipi_Sensor.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   CMOS sensor header file
 *
 ****************************************************************************/
#ifndef _OV5648MIPI_SENSOR_H
#define _OV5648MIPI_SENSOR_H

#define OV5648MIPI_FACTORY_START_ADDR 0
#define OV5648MIPI_ENGINEER_START_ADDR 10
 
typedef enum OV5648MIPI_group_enum
{
  OV5648MIPI_PRE_GAIN = 0,
  OV5648MIPI_CMMCLK_CURRENT,
  OV5648MIPI_FRAME_RATE_LIMITATION,
  OV5648MIPI_REGISTER_EDITOR,
  OV5648MIPI_GROUP_TOTAL_NUMS
} OV5648MIPI_FACTORY_GROUP_ENUM;

typedef enum OV5648MIPI_register_index
{
  OV5648MIPI_SENSOR_BASEGAIN = OV5648MIPI_FACTORY_START_ADDR,
  OV5648MIPI_PRE_GAIN_R_INDEX,
  OV5648MIPI_PRE_GAIN_Gr_INDEX,
  OV5648MIPI_PRE_GAIN_Gb_INDEX,
  OV5648MIPI_PRE_GAIN_B_INDEX,
  OV5648MIPI_FACTORY_END_ADDR
} OV5648MIPI_FACTORY_REGISTER_INDEX;

typedef enum OV5648MIPI_engineer_index
{
  OV5648MIPI_CMMCLK_CURRENT_INDEX = OV5648MIPI_ENGINEER_START_ADDR,
  OV5648MIPI_ENGINEER_END
} OV5648MIPI_FACTORY_ENGINEER_INDEX;

typedef struct _sensor_data_struct
{
  SENSOR_REG_STRUCT reg[OV5648MIPI_ENGINEER_END];
  SENSOR_REG_STRUCT cct[OV5648MIPI_FACTORY_END_ADDR];
} sensor_data_struct;


/* SENSOR PREVIEW/CAPTURE VT CLOCK */
#define OV5648MIPI_PREVIEW_CLK                      84000000
#define OV5648MIPI_CAPTURE_CLK                      84000000
#define OV5648MIPI_VIDEO_CLK                        84000000


/* Data Format */
#define OV5648MIPI_COLOR_FORMAT                     SENSOR_OUTPUT_FORMAT_RAW_B


#define OV5648MIPI_MIN_ANALOG_GAIN                  1   /* 1x */
#define OV5648MIPI_MAX_ANALOG_GAIN                  32  /* 32x */


#define OV5648MIPI_FULL_PERIOD_PIXEL_NUMS          (2752) /* 15 fps */
#define OV5648MIPI_FULL_PERIOD_LINE_NUMS           (1974)

#define OV5648MIPI_PV_PERIOD_PIXEL_NUMS            (1896) /* 30 fps */
#define OV5648MIPI_PV_PERIOD_LINE_NUMS             (984)

#define OV5648MIPI_VIDEO_PERIOD_PIXEL_NUMS         (2416) /* 30 fps */
#define OV5648MIPI_VIDEO_PERIOD_LINE_NUMS          (1104)

#define OV5648MIPI_FULL_START_X                    (4)
#define OV5648MIPI_FULL_START_Y                    (8)
#define OV5648MIPI_IMAGE_SENSOR_FULL_WIDTH         (2560)
#define OV5648MIPI_IMAGE_SENSOR_FULL_HEIGHT        (1920)

#define OV5648MIPI_PV_START_X                      (2)
#define OV5648MIPI_PV_START_Y                      (2)
#define OV5648MIPI_IMAGE_SENSOR_PV_WIDTH           (1280)
#define OV5648MIPI_IMAGE_SENSOR_PV_HEIGHT          (960)


/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define OV5648MIPI_FULL_PERIOD_PIXEL_NUMS          (2816) /* 15 fps */
#define OV5648MIPI_FULL_PERIOD_LINE_NUMS           (1984)
#define OV5648MIPI_PV_PERIOD_PIXEL_NUMS            (2816) /* 30 fps */
#define OV5648MIPI_PV_PERIOD_LINE_NUMS             (992)

/* SENSOR READ/WRITE ID */
#define OV5648MIPI_WRITE_ID (0x6c)
#define OV5648MIPI_READ_ID  (0x6d)

/* FRAME RATE UNIT */
#define OV5648MIPI_FPS(x)                          (10 * (x))


/* EXPORT FUNCTIONS */
UINT32 OV5648MIPIOpen(void);
UINT32 OV5648MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV5648MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 OV5648MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV5648MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV5648MIPIClose(void);

#define Sleep(ms) mdelay(ms)

#endif 
