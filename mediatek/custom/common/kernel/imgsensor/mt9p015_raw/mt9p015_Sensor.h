/*****************************************************************************
 *
 * Filename:
 * ---------
 *   mt9p015_Sensor.h
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
 *   Guangye Yang (mtk70662)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef _MT9P015_SENSOR_H
#define _MT9P015_SENSOR_H

#define MT9P015_FACTORY_START_ADDR 0
#define MT9P015_ENGINEER_START_ADDR 39

typedef enum MT9P015_group_enum
{
  MT9P015_PRE_GAIN = 0,
  MT9P015_CMMCLK_CURRENT,
  MT9P015_FRAME_RATE_LIMITATION,
  MT9P015_REGISTER_EDITOR,
  MT9P015_GROUP_TOTAL_NUMS
} MT9P015_FACTORY_GROUP_ENUM;

typedef enum MT9P015_register_index
{
  MT9P015_SENSOR_BASEGAIN = MT9P015_FACTORY_START_ADDR,
  MT9P015_PRE_GAIN_R_INDEX,
  MT9P015_PRE_GAIN_Gr_INDEX,
  MT9P015_PRE_GAIN_Gb_INDEX,
  MT9P015_PRE_GAIN_B_INDEX,
  MT9P015_FACTORY_END_ADDR
} MT9P015_FACTORY_REGISTER_INDEX;

typedef enum MT9P015_engineer_index
{
  MT9P015_CMMCLK_CURRENT_INDEX = MT9P015_ENGINEER_START_ADDR,
  MT9P015_ENGINEER_END
} MT9P015_FACTORY_ENGINEER_INDEX;

typedef struct _sensor_data_struct
{
  SENSOR_REG_STRUCT reg[MT9P015_ENGINEER_END];
  SENSOR_REG_STRUCT cct[MT9P015_FACTORY_END_ADDR];
} sensor_data_struct;

/* IMPORT FUNCTION */
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);
extern int iReadReg(u16 a_u2Addr, u8 *a_puBuff, u16 i2cId);

#define MT9P015_COLOR_FORMAT                    SENSOR_OUTPUT_FORMAT_RAW_B
#define MT9P015_ENG_INFO                        {128,CMOS_SENSOR,MT9P015_COLOR_FORMAT,}

/* SENSOR PREVIEW/CAPTURE VT CLOCK */
#define MT9P015_PREVIEW_CLK                     96000000
#define MT9P015_CAPTURE_CLK                     96000000

/* SENSOR FULL/PV SIZE */
#define MT9P015_IMAGE_SENSOR_FULL_WIDTH_DRV     2592
#define MT9P015_IMAGE_SENSOR_FULL_HEIGHT_DRV    1944
#define MT9P015_IMAGE_SENSOR_PV_WIDTH_DRV       (MT9P015_IMAGE_SENSOR_FULL_WIDTH_DRV / 2)
#define MT9P015_IMAGE_SENSOR_PV_HEIGHT_DRV      (MT9P015_IMAGE_SENSOR_FULL_HEIGHT_DRV / 2)

/* SENSOR HORIZONTAL/VERTICAL ACTIVE REGION */
#define MT9P015_IMAGE_SENSOR_FULL_HACTIVE       MT9P015_IMAGE_SENSOR_FULL_WIDTH_DRV /* 2592 */
#define MT9P015_IMAGE_SENSOR_FULL_VACTIVE       MT9P015_IMAGE_SENSOR_FULL_HEIGHT_DRV /* 1944 */
#define MT9P015_IMAGE_SENSOR_PV_HACTIVE         MT9P015_IMAGE_SENSOR_PV_WIDTH_DRV /* 1296 */
#define MT9P015_IMAGE_SENSOR_PV_VACTIVE         MT9P015_IMAGE_SENSOR_PV_HEIGHT_DRV /* 972 */

/* SENSOR HORIZONTAL/VERTICAL BLANKING IN ONE PERIOD */
#define MT9P015_IMAGE_SENSOR_FULL_HBLANKING     3307
#define MT9P015_IMAGE_SENSOR_FULL_VBLANKING     90
#define MT9P015_IMAGE_SENSOR_PV_HBLANKING       1717
#define MT9P015_IMAGE_SENSOR_PV_VBLANKING       90

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define MT9P015_FULL_PERIOD_PIXEL_NUMS          (MT9P015_IMAGE_SENSOR_FULL_HACTIVE + MT9P015_IMAGE_SENSOR_FULL_HBLANKING) /* 5899 */
#define MT9P015_FULL_PERIOD_LINE_NUMS           (MT9P015_IMAGE_SENSOR_FULL_VACTIVE + MT9P015_IMAGE_SENSOR_FULL_VBLANKING) /* 2034 */
#define MT9P015_PV_PERIOD_PIXEL_NUMS            (MT9P015_IMAGE_SENSOR_PV_HACTIVE + MT9P015_IMAGE_SENSOR_PV_HBLANKING) /* 3013 */
#define MT9P015_PV_PERIOD_LINE_NUMS             (MT9P015_IMAGE_SENSOR_PV_VACTIVE + MT9P015_IMAGE_SENSOR_PV_VBLANKING) /* 1062 */

/* SENSOR START/END POSITION */
#define MT9P015_FULL_X_START                    4
#define MT9P015_FULL_Y_START                    4
#define MT9P015_IMAGE_SENSOR_FULL_WIDTH         (MT9P015_IMAGE_SENSOR_FULL_HACTIVE - 32) /* 2560 */
#define MT9P015_IMAGE_SENSOR_FULL_HEIGHT        (MT9P015_IMAGE_SENSOR_FULL_VACTIVE - 24) /* 1924 */
#define MT9P015_PV_X_START                      4
#define MT9P015_PV_Y_START                      4
#define MT9P015_IMAGE_SENSOR_PV_WIDTH           (MT9P015_IMAGE_SENSOR_PV_HACTIVE - 16) /* 1280 */
#define MT9P015_IMAGE_SENSOR_PV_HEIGHT          (MT9P015_IMAGE_SENSOR_PV_VACTIVE - 12) /* 960 */

/* SENSOR GAIN LIMITATION */
#define MT9P015_MIN_ANALOG_GAIN                 1 /* 1x analog gain */
#define MT9P015_MAX_ANALOG_GAIN                 12.7

/* SENSOR READ/WRITE ID */
#define MT9P015_SLV1_WRITE_ID                   0x20 /* SMIA */
#define MT9P015_SLV1_READ_ID                    0x21
#define MT9P015_SLV2_WRITE_ID                   0x30
#define MT9P015_SLV2_READ_ID                    0x31
#define MT9P015_SLV3_WRITE_ID                   0x6C /* MIPI */
#define MT9P015_SLV3_READ_ID                    0x6D
#define MT9P015_SLV4_WRITE_ID                   0x6E
#define MT9P015_SLV4_READ_ID                    0x6F

/* SENSOR PRIVATE STRUCT */
typedef struct MT9P015_sensor_STRUCT
{
  MSDK_SENSOR_CONFIG_STRUCT cfg_data;
  sensor_data_struct eng; /* engineer mode */
  MSDK_SENSOR_ENG_INFO_STRUCT eng_info;
  kal_uint8 mirror;
  kal_bool pv_size; /* false means full size */
  kal_bool video_mode;
  kal_uint16 normal_fps; /* video normal mode max fps */
  kal_uint16 night_fps; /* video night mode max fps */
  kal_uint8 write_id;
  kal_uint8 read_id;
  kal_uint32 vt_clk; /* internal use */
  kal_uint16 shutter;
  kal_uint16 gain;
  kal_uint16 frame_height;
  kal_uint16 line_length;
} MT9P015_sensor_struct;

/* FRAME RATE */
#define MT9P015_FPS(x)                          ((kal_uint32)(10 * (x)))

#endif /* _MT9P015_SENSOR_H */

