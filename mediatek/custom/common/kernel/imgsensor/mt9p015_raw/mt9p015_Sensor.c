/*****************************************************************************
 *
 * Filename:
 * ---------
 *   mt9p015_Sensor.c
 *
 * Project:
 * --------
 *   YUSU
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
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
 * 02 19 2012 koli.lin
 * [ALPS00237113] [Performance][Video recording]Recording preview the screen have flash
 * [Camera] 1. Modify the AE converge speed in the video mode.
 *                2. Modify the isp gain delay frame with sensor exposure time and gain synchronization.
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"


#include "mt9p015_Sensor.h"
#include "mt9p015_Camera_Sensor_para.h"
#include "mt9p015_CameraCustomized.h"


static MT9P015_sensor_struct MT9P015_sensor =
{
  .eng =
  {
    .reg = MT9P015_CAMERA_SENSOR_REG_DEFAULT_VALUE,
    .cct = MT9P015_CAMERA_SENSOR_CCT_DEFAULT_VALUE,
  },
  .eng_info = MT9P015_ENG_INFO,
  .vt_clk = MT9P015_PREVIEW_CLK,
  .frame_height = MT9P015_PV_PERIOD_LINE_NUMS,
  .line_length = MT9P015_PV_PERIOD_PIXEL_NUMS,
};

// #define __MT9P015_DEBUG_TRACE__

static void MT9P015_camera_para_to_sensor(void);

/*************************************************************************
* FUNCTION
*    MT9P015_write_cmos_sensor
*
* DESCRIPTION
*    This function wirte data to CMOS sensor through I2C
*
* PARAMETERS
*    addr: the 16bit address of register
*    para: the 8bit value of register
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_write_cmos_sensor(kal_uint16 addr, kal_uint8 para)
{
  kal_int32 rt;
  
  rt = iWriteReg(addr, para, sizeof(para), MT9P015_sensor.write_id);
#ifdef __MT9P015_DEBUG_TRACE__
  if (rt < 0) printk("[MT9P015] I2C write %x, %x error\n", addr, para);
#endif
}

static void MT9P015_write_cmos_sensor_16Bit(kal_uint16 addr, kal_uint16 para)
{
  kal_int32 rt;
  rt = iWriteReg(addr, para, sizeof(para), MT9P015_sensor.write_id);
#ifdef __MT9P015_DEBUG_TRACE__
  if (rt < 0) printk("[MT9P015] I2C 16 bit write %x, %x error\n", addr, para);
#endif
}
/*************************************************************************
* FUNCTION
*    MT9P015_read_cmos_sensor
*
* DESCRIPTION
*    This function read data from CMOS sensor through I2C.
*
* PARAMETERS
*    addr: the 16bit address of register
*
* RETURNS
*    8bit data read through I2C
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint8 MT9P015_read_cmos_sensor(kal_uint16 addr)
{
  kal_int32 rt;
  kal_uint8 data;
  
  rt = iReadReg(addr, &data, MT9P015_sensor.read_id);
#ifdef __MT9P015_DEBUG_TRACE__
  if (rt < 0) printk("[MT9P015] I2C read %x error\n", addr);
#endif
  
  return data;
}

/*************************************************************************
* FUNCTION
*    MT9P015_set_mirror
*
* DESCRIPTION
*    This function set the mirror to the CMOS sensor
*
* PARAMETERS
*    mirror
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
inline static void MT9P015_set_mirror(kal_uint8 mirror)
{
  kal_uint8 sensor_mirror = 0;
  
  if (MT9P015_sensor.mirror == mirror) return;
  MT9P015_sensor.mirror = mirror;
  switch (MT9P015_sensor.mirror)
  {
  case IMAGE_H_MIRROR:
    sensor_mirror = 1;
    break;
  case IMAGE_V_MIRROR:
    sensor_mirror = 2;
    break;
  case IMAGE_HV_MIRROR:
    sensor_mirror = 3;
    break;
  default:
    break;
  }
  MT9P015_write_cmos_sensor(0x0101, sensor_mirror); /* image_orientation */
}

/*************************************************************************
* FUNCTION
*    MT9P015_gain2reg
*
* DESCRIPTION
*    This function translate gain to sensor register value
*
* PARAMETERS
*    gain: sensor gain, max gain is 12.7x, base(0x40)
*
* RETURNS
*    reg value
*
* LOCAL AFFECTED
*
*************************************************************************/
inline static kal_uint16 MT9P015_gain2reg(kal_uint16 gain)
{
  kal_uint16 reg;
  
  reg = gain * 10 / BASEGAIN;
  
  return reg > 0x7F ? 0x7F : reg;
}

/*************************************************************************
* FUNCTION
*    MT9P015_set_clock
*
* DESCRIPTION
*    This function set sensor vt clock and op clock
*
* PARAMETERS
*    clk: vt clock
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_set_clock(kal_uint32 clk)
{
  static const kal_uint8 clk_setting[][6] =
  {
    {0x08,0x01,0x02,0x20,0x08,0x02}, /* MCLK: 48M, VT CLK: 96M, PCLK: 48M */
  //{0x08,0x01,0x02,0x20,0x08,0x02}, /* MCLK: 48M, VT CLK: 96M, PCLK: 48M */
  //{0x06,0x01,0x03,0x24,0x08,0x01}, /* MCLK: 48M, VT CLK: 96M, PCLK: 72M */
  //{0x04,0x01,0x0d,0x60,0x08,0x01}, /* MCLK: 52M, VT CLK: 96M, PCLK: 48M */
    {0x05,0x01,0x03,0x28,0x08,0x01}, /* MCLK: 48M, VT CLK: 128M, PCLK: 80M */
    //{0x06,0x01,0x04,0x3C,0x08,0x01}, /* MCLK: 48M, VT CLK: 120M, PCLK: 90M */
  //{0x06,0x01,0x04,0x3C,0x0A,0x01}, /* MCLK: 48M, VT CLK: 120M, PCLK: 72M */
  //{0x05,0x01,0x0d,0xa0,0x08,0x01}, /* MCLK: 52M, VT CLK: 128M, PCLK: 80M */
  };
  kal_uint8 i;
  
  if (MT9P015_sensor.vt_clk == clk) return;
  MT9P015_sensor.vt_clk = clk;
  switch (MT9P015_sensor.vt_clk)
  {
  case 96000000: i = 0; break;
  case 128000000: i = 1; break;
  default:
    ASSERT(0);
  }
  /* pclk = mclk * pll_multipler / (pre_pll_div * op_sys_clk_div * op_pix_clk_div * row_speed[10:8]) */
  /* vtclk =mclk * pll_multipler / (pre_pll_div * vt_sys_div * vt_pix_clk_div * row_speed[2:0]) */
  MT9P015_write_cmos_sensor(0x0100, 0x00); /* standby */
  
  MT9P015_write_cmos_sensor(0x0301, clk_setting[i][0]); /* vt_pix_clk_div, (Internal used to control the timing of the pixel array) */
  MT9P015_write_cmos_sensor(0x0303, clk_setting[i][1]); /* vt_sys_div (For SIMA) */
  MT9P015_write_cmos_sensor(0x0305, clk_setting[i][2]); /* pre_pll_div */
  MT9P015_write_cmos_sensor(0x0307, clk_setting[i][3]); /* pll_multipler */
  MT9P015_write_cmos_sensor(0x0309, clk_setting[i][4]); /* op_pix_clk_div (Pixel clock) */
  MT9P015_write_cmos_sensor(0x030B, clk_setting[i][5]); /* op_sys_clk_div */
  mdelay(1); /* allow PLL to lock */
}

/*************************************************************************
* FUNCTION
*    MT9P015_write_shutter
*
* DESCRIPTION
*    This function apply shutter to sensor
*
* PARAMETERS
*    course: course integration time
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_write_shutter(kal_uint16 course)
{
  if (!course) course = 1; /* avoid 0 */
  MT9P015_write_cmos_sensor(0x0104, 0x01); /* grouped_parameter_hold */
  MT9P015_write_cmos_sensor(0x0202, course >> 8); /* course_integration_time */
  MT9P015_write_cmos_sensor(0x0203, course);
  MT9P015_write_cmos_sensor(0x0104, 0x00);
}

/*************************************************************************
* FUNCTION
*    MT9P015_array_window
*
* DESCRIPTION
*    This function config sensor array window
*
* PARAMETERS
*    startx, starty, width, height
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_array_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
  const kal_uint16 endx = startx + width - 1;
  const kal_uint16 endy = starty + height - 1;
  
  MT9P015_write_cmos_sensor(0x0344, startx >> 8); /* x_addr_start */
  MT9P015_write_cmos_sensor(0x0345, startx);
  MT9P015_write_cmos_sensor(0x0346, starty >> 8); /* y_addr_start */
  MT9P015_write_cmos_sensor(0x0347, starty);
  MT9P015_write_cmos_sensor(0x0348, endx >> 8); /* x_addr_end */
  MT9P015_write_cmos_sensor(0x0349, endx);
  MT9P015_write_cmos_sensor(0x034A, endy >> 8); /* y_addr_end */
  MT9P015_write_cmos_sensor(0x034B, endy);
}

/*************************************************************************
* FUNCTION
*    MT9P015_output_window
*
* DESCRIPTION
*    This function config output window
*
* PARAMETERS
*    pv_size: KAL_TRUE means switch to 1/4 size to preview, KAL_FALSE means switch to full size to caputre
*    dummy_pixel, dummy_line
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_output_window(kal_bool pv_size, kal_uint16 dummy_pixel, kal_uint16 dummy_line)
{
  kal_uint16 hactive, vactive, line_length, frame_height;
  kal_uint8 inc;
  
  if (pv_size)
  {
    inc = 3;
    hactive = MT9P015_IMAGE_SENSOR_PV_HACTIVE;
    vactive = MT9P015_IMAGE_SENSOR_PV_VACTIVE;
    line_length = MT9P015_PV_PERIOD_PIXEL_NUMS + dummy_pixel;
    frame_height = MT9P015_PV_PERIOD_LINE_NUMS + dummy_line;
  }
  else
  {
    inc = 1;
    hactive = MT9P015_IMAGE_SENSOR_FULL_HACTIVE;
    vactive = MT9P015_IMAGE_SENSOR_FULL_VACTIVE;
    line_length = MT9P015_FULL_PERIOD_PIXEL_NUMS + dummy_pixel;
    frame_height = MT9P015_FULL_PERIOD_LINE_NUMS + dummy_line;
  }
  MT9P015_write_cmos_sensor(0x0104, 0x01);
  /* config line length */
  if (line_length != MT9P015_sensor.line_length)
  {
    MT9P015_sensor.line_length = line_length;
    MT9P015_write_cmos_sensor(0x0342, line_length >> 8); /* line_length_pck */
    MT9P015_write_cmos_sensor(0x0343, line_length);
  }
  /* config frame height */
  if (frame_height != MT9P015_sensor.frame_height)
  {
    MT9P015_sensor.frame_height = frame_height;
    MT9P015_write_cmos_sensor(0x0340, MT9P015_sensor.frame_height >> 8); /* frame_length_lines */
    MT9P015_write_cmos_sensor(0x0341, MT9P015_sensor.frame_height);
  }
  /* config output window */
  if (MT9P015_sensor.pv_size != pv_size)
  {
    MT9P015_sensor.pv_size = pv_size;
    MT9P015_array_window(8, 8, MT9P015_IMAGE_SENSOR_FULL_HACTIVE + (MT9P015_sensor.pv_size ? -2 : 2), MT9P015_IMAGE_SENSOR_FULL_VACTIVE + 2);
    MT9P015_write_cmos_sensor(0x0383, inc); /* x_odd_inc */
    MT9P015_write_cmos_sensor(0x0387, inc); /* y_odd_inc */
    MT9P015_write_cmos_sensor(0x034C, hactive >> 8); /* x_output_size */
    MT9P015_write_cmos_sensor(0x034D, hactive);
    MT9P015_write_cmos_sensor(0x034E, vactive >> 8); /* y_output_size */
    MT9P015_write_cmos_sensor(0x034F, vactive);
  }
  MT9P015_write_cmos_sensor(0x0104, 0x00);
}

/*************************************************************************
* FUNCTION
*    MT9P015_initial_setting
*
* DESCRIPTION
*    This function initialize the registers of CMOS sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_initial_setting(void)
{
  MT9P015_write_cmos_sensor(0x0103, 0x01); /* software reset */
  mdelay(20); /* wait for reset done */
  MT9P015_write_cmos_sensor(0x301A, 0x10); /* RESET_REGISTER, enable parallel interface and disable serialiser */
  MT9P015_write_cmos_sensor(0x301B, 0xC8); /* software standby */
  MT9P015_write_cmos_sensor(0x3064, 0x08); /* smia test:[0:3]smia_test_pfd,[4]smia_test_dp_bypass,[5]smia_test_scaler_pattern_test,[6]smia_test_scaler_bist */
  MT9P015_write_cmos_sensor(0x3065, 0x05); /* [7]smia_test_fifo_sam_bist,[8]smia_test_embedded_data,[9]smia_test_pll_bypass,[10:11]smia_test_pll_lock_mode */
  MT9P015_write_cmos_sensor(0x0104, 0x01);
  
  /* parallel recommended setting */
  MT9P015_write_cmos_sensor(0x3094, 0x46);
  MT9P015_write_cmos_sensor(0x3095, 0x56);
  MT9P015_write_cmos_sensor(0x309E, 0x5D);
  MT9P015_write_cmos_sensor(0x309F, 0x00);
  
  MT9P015_write_cmos_sensor(0x3088, 0x6F); /* dac_ld_8_9:[0:3]ana_sreg_vdac_vtx_lo,[4:7]ana_sreg_vdac_vtx_hi */
  MT9P015_write_cmos_sensor(0x3089, 0xFF); /* [8:11]ana_sreg_vdac_vrst_lo,[12:15]ana_sreg_vdac_vrst_hi */
  MT9P015_write_cmos_sensor(0x3086, 0x24); 
  MT9P015_write_cmos_sensor(0x3087, 0x68);
  
  MT9P015_sensor.vt_clk = 0; /* force config clock */
  MT9P015_set_clock(MT9P015_PREVIEW_CLK);
  
  MT9P015_write_cmos_sensor(0x301A, 0x10);
  MT9P015_write_cmos_sensor(0x301B, 0xCC); /* Streaming start  */
  MT9P015_write_cmos_sensor(0x316C, 0xA4);  /* relate to ADC, with1.4X as min gain */
  MT9P015_write_cmos_sensor(0x316D, 0xF0);  /* default value 0x1000 */
  
  MT9P015_write_cmos_sensor(0x306E, 0x04);  /* slew rate */
  MT9P015_write_cmos_sensor(0x306F, 0x80);  
  
  /* default setting */
  MT9P015_sensor.gain = 0x100; /* 4x */
  MT9P015_sensor.shutter = 800;
  MT9P015_sensor.video_mode = KAL_FALSE;
  
  MT9P015_sensor.mirror = !IMAGE_NORMAL; /* force config sensor mirror */
  MT9P015_set_mirror(IMAGE_NORMAL);
  MT9P015_write_cmos_sensor(0x0104, 0x00);
  MT9P015_sensor.pv_size = KAL_FALSE; /* force config pv mode */
  MT9P015_sensor.line_length = MT9P015_sensor.frame_height = 0; /* force config line length & frame height */
  MT9P015_output_window(KAL_TRUE, 0, 0);
  MT9P015_camera_para_to_sensor();
  MT9P015_write_cmos_sensor(0x0100, 0x01); /* streaming */
}

/*************************************************************************
* FUNCTION
*    MT9P015_power_on
*
* DESCRIPTION
*    This function power on CMOS sensor and check sensor id
*
* PARAMETERS
*    None
*
* RETURNS
*    sensor ID
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint16 MT9P015_power_on(void)
{
  const kal_uint8 i2c_addr[] = {MT9P015_SLV1_WRITE_ID, MT9P015_SLV2_WRITE_ID, MT9P015_SLV3_WRITE_ID, MT9P015_SLV4_WRITE_ID};
  kal_uint8 i;
  kal_uint16 sensor_id;
  
  for (i = 0; i < sizeof(i2c_addr) / sizeof(i2c_addr[0]); i++)
  {
    MT9P015_sensor.write_id = i2c_addr[i];
    MT9P015_sensor.read_id = i2c_addr[i]|1;
    sensor_id = (MT9P015_read_cmos_sensor(0x0000) << 8)|MT9P015_read_cmos_sensor(0x0001);
    if (MT9P015_SENSOR_ID == sensor_id)
    {
#if (defined(__MT9P015_DEBUG_TRACE__))
      printk("[MT9P015] I2C address: %x\n", i2c_addr[i]);
#endif
      return MT9P015_SENSOR_ID;
    }
  }
#if (defined(__MT9P015_DEBUG_TRACE__))
  printk("[MT9P015] SENSOR ID: %x\n", sensor_id);
#endif
  return sensor_id;
}

/*************************************************************************
* FUNCTION
*    MT9P015_get_size
*
* DESCRIPTION
*    This function return the image width and height of image sensor.
*
* PARAMETERS
*    *sensor_width: address pointer of horizontal effect pixels of image sensor
*    *sensor_height: address pointer of vertical effect pixels of image sensor
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_get_size(kal_uint16 *sensor_width, kal_uint16 *sensor_height)
{
  *sensor_width = MT9P015_IMAGE_SENSOR_FULL_WIDTH; /* must be 4:3 */
  *sensor_height = MT9P015_IMAGE_SENSOR_FULL_HEIGHT;
}

/*************************************************************************
* FUNCTION
*    MT9P015_get_period
*
* DESCRIPTION
*    This function return the image width and height of image sensor.
*
* PARAMETERS
*    *pixel_number: address pointer of pixel numbers in one period of HSYNC
*    *line_number: address pointer of line numbers in one period of VSYNC
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_get_period(kal_uint16 *pixel_number, kal_uint16 *line_number)
{
  *pixel_number = MT9P015_sensor.line_length;
  *line_number = MT9P015_sensor.frame_height;
}

/*************************************************************************
* FUNCTION
*    MT9P015_preview
*
* DESCRIPTION
*    This function config CMOS sensor to preview state
*
* PARAMETERS
*    image_window: grab window
*    sensor_config_data
*
* RETURNS
*    error code
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 MT9P015_preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
  kal_uint16 dummy_line;
  
  MT9P015_set_clock(MT9P015_PREVIEW_CLK);
  MT9P015_set_mirror(sensor_config_data->SensorImageMirror);
  switch (sensor_config_data->SensorOperationMode)
  {
  case MSDK_SENSOR_OPERATION_MODE_VIDEO:
    MT9P015_sensor.video_mode = KAL_TRUE;
    MT9P015_sensor.normal_fps = MT9P015_FPS(30);
    MT9P015_sensor.night_fps = MT9P015_FPS(15);
    dummy_line = 0;
    break;
  default: /* ISP_PREVIEW_MODE */
    MT9P015_sensor.video_mode = KAL_FALSE;
    dummy_line = 0;
  }
  MT9P015_output_window(KAL_TRUE, 0, dummy_line); /* modify dummy_pixel must gen AE table again */
  
  image_window->GrabStartX = MT9P015_PV_X_START;
  image_window->GrabStartY = MT9P015_PV_Y_START;
  image_window->ExposureWindowWidth = MT9P015_IMAGE_SENSOR_PV_WIDTH;
  image_window->ExposureWindowHeight = MT9P015_IMAGE_SENSOR_PV_HEIGHT;
  image_window->CurrentExposurePixel = MT9P015_sensor.line_length;
  image_window->ExposurePixel = MT9P015_sensor.line_length;
  
  MT9P015_write_cmos_sensor(0x0100, 0x01); /* streaming */
  memcpy(&MT9P015_sensor.cfg_data, sensor_config_data, sizeof(sensor_config_data));
  
  return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    MT9P015_capture
*
* DESCRIPTION
*    This function config CMOS sensor to capture state
*
* PARAMETERS
*    image_window: grab window and size
*    sensor_config_data
*
* RETURNS
*    error code
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 MT9P015_capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
  const kal_uint16 pv_line_length = MT9P015_PV_PERIOD_PIXEL_NUMS;
  const kal_uint32 pv_vt_clk = MT9P015_PREVIEW_CLK;
  kal_uint16 shutter = MT9P015_sensor.shutter;
  kal_uint16 cap_fps, dummy_pixel;
  
  if (sensor_config_data->EnableShutterTansfer)
  {
    shutter = sensor_config_data->CaptureShutter;
  }
  if (image_window->ImageTargetWidth <= MT9P015_IMAGE_SENSOR_PV_WIDTH_DRV &&
      image_window->ImageTargetHeight <= MT9P015_IMAGE_SENSOR_PV_HEIGHT_DRV)
  {
    if (ACDK_SENSOR_OPERATION_MODE_WEB_CAPTURE == sensor_config_data->SensorOperationMode) /* webcam mode */
    {
      dummy_pixel = 0;
    }
    else
    {
      cap_fps = MT9P015_FPS(8);
      dummy_pixel = MT9P015_sensor.vt_clk * MT9P015_FPS(1) / (MT9P015_PV_PERIOD_LINE_NUMS * cap_fps);
      dummy_pixel = dummy_pixel < MT9P015_PV_PERIOD_PIXEL_NUMS ? 0 : dummy_pixel - MT9P015_PV_PERIOD_PIXEL_NUMS;
    }
    MT9P015_output_window(KAL_TRUE, dummy_pixel, 0);
    image_window->GrabStartX = MT9P015_PV_X_START;
    image_window->GrabStartY = MT9P015_PV_Y_START;
    image_window->ExposureWindowWidth = MT9P015_IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight = MT9P015_IMAGE_SENSOR_PV_HEIGHT;
  }
  else
  {
    MT9P015_set_clock(MT9P015_CAPTURE_CLK);
    cap_fps = MT9P015_FPS(8);
    dummy_pixel = MT9P015_sensor.vt_clk * MT9P015_FPS(1) / (MT9P015_FULL_PERIOD_LINE_NUMS * cap_fps);
    dummy_pixel = dummy_pixel < MT9P015_FULL_PERIOD_PIXEL_NUMS ? 0 : dummy_pixel - MT9P015_FULL_PERIOD_PIXEL_NUMS;
    
    MT9P015_output_window(KAL_FALSE, dummy_pixel, 0);
    image_window->GrabStartX = MT9P015_FULL_X_START;
    image_window->GrabStartY = MT9P015_FULL_Y_START;
    image_window->ExposureWindowWidth = MT9P015_IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = MT9P015_IMAGE_SENSOR_FULL_HEIGHT;
  }
  /* shutter translation */
  shutter = MT9P015_sensor.vt_clk / (pv_vt_clk >> 7) * shutter * pv_line_length / (MT9P015_sensor.line_length << 7);
  
  /* config flashlight capture setting */
  sensor_config_data->DefaultPclk = MT9P015_sensor.vt_clk;
  sensor_config_data->Pixels = MT9P015_sensor.line_length;
  sensor_config_data->FrameLines = MT9P015_sensor.frame_height;
  sensor_config_data->Lines = image_window->ExposureWindowHeight;
  sensor_config_data->Shutter = shutter;
  
  if (shutter != MT9P015_sensor.shutter) MT9P015_write_shutter(shutter);
  MT9P015_write_cmos_sensor(0x0100, 0x01); /* streaming */
  memcpy(&MT9P015_sensor.cfg_data, sensor_config_data, sizeof(sensor_config_data));
  
  return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    MT9P015_set_shutter
*
* DESCRIPTION
*    This function set e-shutter to change exposure time.
*
* PARAMETERS
*    shutter: exposured lines
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_set_shutter(kal_uint16 shutter)
{
  MT9P015_sensor.shutter = shutter;
  MT9P015_write_shutter(MT9P015_sensor.shutter);
}

/*************************************************************************
* FUNCTION
*    MT9P015_night_mode
*
* DESCRIPTION
*    This function night mode
*
* PARAMETERS
*    enable
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_night_mode(kal_bool enable)
{
  const kal_uint16 dummy_pixel = MT9P015_sensor.line_length - MT9P015_PV_PERIOD_PIXEL_NUMS;
  const kal_uint16 pv_min_fps =  enable ? MT9P015_sensor.night_fps : MT9P015_sensor.normal_fps;
  kal_uint16 dummy_line = MT9P015_sensor.frame_height - MT9P015_PV_PERIOD_LINE_NUMS;
  kal_uint16 max_exposure_lines;
  
  if (!MT9P015_sensor.video_mode) return;
  max_exposure_lines = MT9P015_sensor.vt_clk * MT9P015_FPS(1) / (pv_min_fps * MT9P015_sensor.line_length);
  if (max_exposure_lines > MT9P015_sensor.frame_height) /* fix max frame rate, AE table will fix min frame rate */
  {
    dummy_line = max_exposure_lines - MT9P015_PV_PERIOD_LINE_NUMS;
  }
  MT9P015_output_window(KAL_TRUE, dummy_pixel, dummy_line);
}

/*************************************************************************
* FUNCTION
*    MT9P015_set_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    gain: sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint16 MT9P015_set_gain(kal_uint16 gain)
{
  kal_uint16 reg, new_gain;
  
  MT9P015_sensor.gain = gain * MT9P015_sensor.eng.cct[MT9P015_SENSOR_BASEGAIN].Para / BASEGAIN;
  
  MT9P015_write_cmos_sensor(0x0104, 0x01);
  new_gain = MT9P015_sensor.gain * MT9P015_sensor.eng.cct[MT9P015_PRE_GAIN_R_INDEX].Para / BASEGAIN;
  reg = MT9P015_gain2reg(new_gain);
  MT9P015_write_cmos_sensor(0x0209, reg&0xFF);
  
  new_gain = MT9P015_sensor.gain * MT9P015_sensor.eng.cct[MT9P015_PRE_GAIN_Gr_INDEX].Para / BASEGAIN;
  reg = MT9P015_gain2reg(new_gain);
  MT9P015_write_cmos_sensor(0x0207, reg&0xFF);
  
  new_gain = MT9P015_sensor.gain * MT9P015_sensor.eng.cct[MT9P015_PRE_GAIN_Gb_INDEX].Para / BASEGAIN;
  reg = MT9P015_gain2reg(new_gain);
  MT9P015_write_cmos_sensor(0x020D, reg&0xFF);
  
  new_gain = MT9P015_sensor.gain * MT9P015_sensor.eng.cct[MT9P015_PRE_GAIN_B_INDEX].Para / BASEGAIN;
  reg = MT9P015_gain2reg(new_gain);
  MT9P015_write_cmos_sensor(0x020B, reg&0xFF);
  MT9P015_write_cmos_sensor(0x0104, 0x00);
  
  return gain;
}

/*************************************************************************
* FUNCTION
*    MT9P015_set_flashlight
*
* DESCRIPTION
*    turn on/off flashlight.
*
* PARAMETERS
*    enable
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_set_flashlight(kal_bool enable)
{
  /* not ready */
}

/* write camera_para to sensor register */
static void MT9P015_camera_para_to_sensor(void)
{
  kal_uint32 i;
  
  for (i = 0; 0xFFFFFFFF != MT9P015_sensor.eng.reg[i].Addr; i++)
  {
    MT9P015_write_cmos_sensor(MT9P015_sensor.eng.reg[i].Addr, MT9P015_sensor.eng.reg[i].Para);
  }
  for (i = MT9P015_ENGINEER_START_ADDR; 0xFFFFFFFF != MT9P015_sensor.eng.reg[i].Addr; i++)
  {
    MT9P015_write_cmos_sensor(MT9P015_sensor.eng.reg[i].Addr, MT9P015_sensor.eng.reg[i].Para);
  }
  for (i = MT9P015_FACTORY_START_ADDR; 0xFFFFFFFF != MT9P015_sensor.eng.reg[i].Addr; i++)
  {
    MT9P015_write_cmos_sensor(MT9P015_sensor.eng.cct[i].Addr, MT9P015_sensor.eng.cct[i].Para);
  }
  MT9P015_set_gain(MT9P015_sensor.gain); /* update gain */
}

/* update camera_para from sensor register */
static void MT9P015_sensor_to_camera_para(void)
{
  kal_uint32 i;
  
  for (i = 0; 0xFFFFFFFF != MT9P015_sensor.eng.reg[i].Addr; i++)
  {
    MT9P015_sensor.eng.reg[i].Para = MT9P015_read_cmos_sensor(MT9P015_sensor.eng.reg[i].Addr);
  }
  for (i = MT9P015_ENGINEER_START_ADDR; 0xFFFFFFFF != MT9P015_sensor.eng.reg[i].Addr; i++)
  {
    MT9P015_sensor.eng.reg[i].Para = MT9P015_read_cmos_sensor(MT9P015_sensor.eng.reg[i].Addr);
  }
  for (i = MT9P015_FACTORY_START_ADDR; 0xFFFFFFFF != MT9P015_sensor.eng.reg[i].Addr; i++)
  {
    MT9P015_sensor.eng.cct[i].Para = MT9P015_read_cmos_sensor(MT9P015_sensor.eng.cct[i].Addr);
  }
}

/* -----------------------------Engineer mode----------------------------- */
inline static void MT9P015_get_sensor_group_count(kal_int32 *sensor_count_ptr)
{
  *sensor_count_ptr = MT9P015_GROUP_TOTAL_NUMS;
}

inline static void MT9P015_get_sensor_group_info(MSDK_SENSOR_GROUP_INFO_STRUCT *para)
{
  switch (para->GroupIdx)
  {
  case MT9P015_PRE_GAIN:
    sprintf(para->GroupNamePtr, "CCT");
    para->ItemCount = 5;
    break;
  case MT9P015_CMMCLK_CURRENT:
    sprintf(para->GroupNamePtr, "CMMCLK Current");
    para->ItemCount = 1;
    break;
  case MT9P015_FRAME_RATE_LIMITATION:
    sprintf(para->GroupNamePtr, "Frame Rate Limitation");
    para->ItemCount = 2;
    break;
  case MT9P015_REGISTER_EDITOR:
    sprintf(para->GroupNamePtr, "Register Editor");
    para->ItemCount = 2;
    break;
  default:
    ASSERT(0);
  }
}

inline static void MT9P015_get_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{
  const static kal_char *cct_item_name[] = {"SENSOR_BASEGAIN", "Pregain-R", "Pregain-Gr", "Pregain-Gb", "Pregain-B"};
  const static kal_char *editer_item_name[] = {"REG addr", "REG value"};
  
  switch (para->GroupIdx)
  {
  case MT9P015_PRE_GAIN:
    switch (para->ItemIdx)
    {
    case MT9P015_SENSOR_BASEGAIN:
    case MT9P015_PRE_GAIN_R_INDEX:
    case MT9P015_PRE_GAIN_Gr_INDEX:
    case MT9P015_PRE_GAIN_Gb_INDEX:
    case MT9P015_PRE_GAIN_B_INDEX:
      break;
    default:
      ASSERT(0);
    }
    sprintf(para->ItemNamePtr, cct_item_name[para->ItemIdx - MT9P015_SENSOR_BASEGAIN]);
    para->ItemValue = MT9P015_sensor.eng.cct[para->ItemIdx].Para * 1000 / BASEGAIN;
    para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
    para->Min = MT9P015_MIN_ANALOG_GAIN * 1000;
    para->Max = MT9P015_MAX_ANALOG_GAIN * 1000;
    break;
  case MT9P015_CMMCLK_CURRENT:
    switch (para->ItemIdx)
    {
    case 0:
      sprintf(para->ItemNamePtr, "Drv Cur[2,4,6,8]mA");
      switch (MT9P015_sensor.eng.reg[MT9P015_CMMCLK_CURRENT_INDEX].Para)
      {
      case ISP_DRIVING_2MA:
        para->ItemValue = 2;
        break;
      case ISP_DRIVING_4MA:
        para->ItemValue = 4;
        break;
      case ISP_DRIVING_6MA:
        para->ItemValue = 6;
        break;
      case ISP_DRIVING_8MA:
        para->ItemValue = 8;
        break;
      default:
        ASSERT(0);
      }
      para->IsTrueFalse = para->IsReadOnly = KAL_FALSE;
      para->IsNeedRestart = KAL_TRUE;
      para->Min = 2;
      para->Max = 8;
      break;
    default:
      ASSERT(0);
    }
    break;
  case MT9P015_FRAME_RATE_LIMITATION:
    switch (para->ItemIdx)
    {
    case 0:
      sprintf(para->ItemNamePtr, "Max Exposure Lines");
      para->ItemValue = 5998;
      break;
    case 1:
      sprintf(para->ItemNamePtr, "Min Frame Rate");
      para->ItemValue = 5;
      break;
    default:
      ASSERT(0);
    }
    para->IsTrueFalse = para->IsNeedRestart = KAL_FALSE;
    para->IsReadOnly = KAL_TRUE;
    para->Min = para->Max = 0;
    break;
  case MT9P015_REGISTER_EDITOR:
    switch (para->ItemIdx)
    {
    case 0:
    case 1:
      sprintf(para->ItemNamePtr, editer_item_name[para->ItemIdx]);
      para->ItemValue = 0;
      para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
      para->Min = 0;
      para->Max = (para->ItemIdx ? 0xFF : 0xFFFF);
      break;
    default:
      ASSERT(0);
    }
    break;
  default:
    ASSERT(0);
  }
}

inline static kal_bool MT9P015_set_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{
  kal_uint16 temp;
  
  switch (para->GroupIdx)
  {
  case MT9P015_PRE_GAIN:
    switch (para->ItemIdx)
    {
    case MT9P015_SENSOR_BASEGAIN:
    case MT9P015_PRE_GAIN_R_INDEX:
    case MT9P015_PRE_GAIN_Gr_INDEX:
    case MT9P015_PRE_GAIN_Gb_INDEX:
    case MT9P015_PRE_GAIN_B_INDEX:
      MT9P015_sensor.eng.cct[para->ItemIdx].Para = para->ItemValue * BASEGAIN / 1000;
      MT9P015_set_gain(MT9P015_sensor.gain); /* update gain */
      break;
    default:
      ASSERT(0);
    }
    break;
  case MT9P015_CMMCLK_CURRENT:
    switch (para->ItemIdx)
    {
    case 0:
      switch (para->ItemValue)
      {
      case 2: temp = ISP_DRIVING_2MA; break;
      case 3:
      case 4: temp = ISP_DRIVING_4MA; break;
      case 5:
      case 6: temp = ISP_DRIVING_6MA; break;
      default: temp = ISP_DRIVING_8MA; break;
      }
      MT9P015_sensor.eng.reg[MT9P015_CMMCLK_CURRENT_INDEX].Para = temp;
      break;
    default:
      ASSERT(0);
    }
    break;
  case MT9P015_FRAME_RATE_LIMITATION:
    ASSERT(0);
    break;
  case MT9P015_REGISTER_EDITOR:
    switch (para->ItemIdx)
    {
      static kal_uint32 fac_sensor_reg;
    case 0:
      if (para->ItemValue < 0 || para->ItemValue > 0xFFFF) return KAL_FALSE;
      fac_sensor_reg = para->ItemValue;
      break;
    case 1:
      if (para->ItemValue < 0 || para->ItemValue > 0xFF) return KAL_FALSE;
      MT9P015_write_cmos_sensor(fac_sensor_reg, para->ItemValue);
      break;
    default:
      ASSERT(0);
    }
    break;
  default:
    ASSERT(0);
  }
  return KAL_TRUE;
}

/*************************************************************************
* FUNCTION
*    MT9P015_set_video_mode
*
* DESCRIPTION
*    This function set video frame rate
*
* PARAMETERS
*    fps: frame rate per second
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void MT9P015_set_video_mode(kal_uint16 fps)
{
  kal_uint16 frame_height;
  
  frame_height = MT9P015_sensor.vt_clk / (fps * MT9P015_sensor.line_length);
  if (frame_height < MT9P015_sensor.frame_height)
  {
    frame_height = MT9P015_sensor.frame_height;
  }
  MT9P015_output_window(KAL_TRUE, MT9P015_sensor.line_length - MT9P015_PV_PERIOD_PIXEL_NUMS, frame_height - MT9P015_PV_PERIOD_LINE_NUMS);
}

/*************************************************************************
* FUNCTION
*    MT9P015_init
*
* DESCRIPTION
*    This function initialize the registers of CMOS sensor
*
* PARAMETERS
*    None
*
* RETURNS
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 MT9P015_init(void)
{
  if (MT9P015_SENSOR_ID != MT9P015_power_on()) return ERROR_SENSOR_CONNECT_FAIL;
  MT9P015_initial_setting();
  
  return ERROR_NONE;
}
/*************************************************************************
* FUNCTION
*	MT9P015_GetSensorID
*
* DESCRIPTION
*	This function get the sensor ID
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 MT9P015_GetSensorID(kal_uint32 *sensorID)

{
	//SENSORDB("[Enter]:MT9P015 Open func ");

	*sensorID = MT9P015_power_on();

	 if (MT9P015_SENSOR_ID != *sensorID  ) 
 	{
 		*sensorID = 0xFFFFFFFF; 
 	   //SENSORDB("[MT9P015]Error:read sensor ID fail\n");
	   return ERROR_SENSOR_CONNECT_FAIL;
 	}

    return ERROR_NONE;    
}   /* MT9P015Open  */


/*************************************************************************
* FUNCTION
*    MT9P015_get_info
*
* DESCRIPTION
*    This function provide information to upper layer
*
* PARAMETERS
*    id: scenario id
*    info: info struct
*    cfg_data: config data
*
* RETURNS
*    error code
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 MT9P015_get_info(MSDK_SCENARIO_ID_ENUM id, MSDK_SENSOR_INFO_STRUCT *info, MSDK_SENSOR_CONFIG_STRUCT *cfg_data)
{
  info->SensorPreviewResolutionX = MT9P015_IMAGE_SENSOR_PV_WIDTH_DRV;
  info->SensorPreviewResolutionY = MT9P015_IMAGE_SENSOR_PV_HEIGHT_DRV;
  info->SensorFullResolutionX = MT9P015_IMAGE_SENSOR_FULL_WIDTH_DRV;
  info->SensorFullResolutionY = MT9P015_IMAGE_SENSOR_FULL_HEIGHT_DRV;
  
  info->SensorCameraPreviewFrameRate = 30;
  info->SensorVideoFrameRate = 30;
  info->SensorStillCaptureFrameRate = 10;
  info->SensorWebCamCaptureFrameRate = 15;
  info->SensorResetActiveHigh = FALSE;
  info->SensorResetDelayCount = 1;
  
  info->SensorOutputDataFormat = MT9P015_COLOR_FORMAT;
  info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
  info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
  info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
  info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
  
  info->SensorInterruptDelayLines = 1;
  info->SensroInterfaceType = SENSOR_INTERFACE_TYPE_PARALLEL;

  info->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth = CAM_SIZE_5M_WIDTH;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight = CAM_SIZE_5M_HEIGHT;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported = TRUE;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable = FALSE;

  info->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth = CAM_SIZE_5M_WIDTH;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight = CAM_SIZE_5M_HEIGHT;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported = TRUE;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable = FALSE;

  info->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth = CAM_SIZE_5M_WIDTH;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight = CAM_SIZE_5M_HEIGHT;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported = TRUE;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable = FALSE;

  info->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth = CAM_SIZE_1M_WIDTH;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight = CAM_SIZE_1M_HEIGHT;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported = TRUE;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable = TRUE;

  info->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth = CAM_SIZE_1M_WIDTH;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight = CAM_SIZE_1M_HEIGHT;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported = TRUE;
  info->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable = TRUE;
  
  info->CaptureDelayFrame = 2;
  info->PreviewDelayFrame = 2;
  
  info->AEShutDelayFrame = 0; /* The frame of setting shutter default 0 for TG int */
  info->AESensorGainDelayFrame = 1; /* The frame of setting sensor gain */
  info->AEISPGainDelayFrame = 2;
  
  info->SensorMasterClockSwitch = 0; 
  info->SensorDrivingCurrent = ISP_DRIVING_8MA;
  
  switch (id)
  {
  case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
  case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
  case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
    info->SensorClockFreq = 48;
    info->SensorClockDividCount = 1;
    info->SensorClockRisingCount = 0;
    info->SensorClockFallingCount = 1;
    info->SensorPixelClockCount = 1;
    info->SensorDataLatchCount = 1;
    info->SensorGrabStartX = MT9P015_PV_X_START;
    info->SensorGrabStartY = MT9P015_PV_Y_START;
    break;
  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
    info->SensorClockFreq = 48;
    info->SensorClockDividCount = 1;
    info->SensorClockRisingCount =0;
    info->SensorClockFallingCount = 1;
    info->SensorPixelClockCount = 1;
    info->SensorDataLatchCount = 1;
    info->SensorGrabStartX = MT9P015_FULL_X_START;
    info->SensorGrabStartY = MT9P015_FULL_Y_START;
    break;
  default:
    info->SensorClockFreq = 24;
    info->SensorClockRisingCount = 0;
    info->SensorClockFallingCount = 2;
    info->SensorClockDividCount = 4;
    info->SensorPixelClockCount = 4;
    info->SensorDataLatchCount = 2;
    info->SensorGrabStartX = MT9P015_PV_X_START;
    info->SensorGrabStartY = MT9P015_PV_Y_START;
    break;
  }
  memcpy(cfg_data, &MT9P015_sensor.cfg_data, sizeof(MT9P015_sensor.cfg_data));
  
  return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    MT9P015_get_resolution
*
* DESCRIPTION
*    This function provide resolution to upper layer
*
* PARAMETERS
*    res
*
* RETURNS
*    error code
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 MT9P015_get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *res)
{
  res->SensorFullWidth = MT9P015_IMAGE_SENSOR_FULL_WIDTH;
  res->SensorFullHeight = MT9P015_IMAGE_SENSOR_FULL_HEIGHT;
  res->SensorPreviewWidth = MT9P015_IMAGE_SENSOR_PV_WIDTH;
  res->SensorPreviewHeight = MT9P015_IMAGE_SENSOR_PV_HEIGHT;
  
  return ERROR_NONE;
}

UINT32 MT9P015SetCalData(PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData)
{
	UINT32 i;
#ifdef __MT9P015_DEBUG_TRACE__
	printk("MT9P015 Sensor write calibration data num = %d \r\n", pSetSensorCalData->DataSize);
	printk("MT9P015 Sensor write calibration data format = %x \r\n", pSetSensorCalData->DataFormat);
#endif	
	if((pSetSensorCalData->DataSize > 0) && (pSetSensorCalData->DataSize <= MAX_SHADING_DATA_TBL))
	{
		MT9P015_write_cmos_sensor(0x0104, 0x01);
		for (i = 0; i < pSetSensorCalData->DataSize; i++)
		{
			if (((pSetSensorCalData->DataFormat & 0xFFFF) == 1) && ((pSetSensorCalData->DataFormat >> 16) == 1))
			{
#ifdef __MT9P015_DEBUG_TRACE__
				printk("MT9P015 Sensor write calibration data: address = %x, value = %x \r\n",(pSetSensorCalData->ShadingData[i])&0xFFFF,(pSetSensorCalData->ShadingData[i])>>16);
#endif
				MT9P015_write_cmos_sensor_16Bit((pSetSensorCalData->ShadingData[i])&0xFFFF,(pSetSensorCalData->ShadingData[i])>>16);
			}
		}
		MT9P015_write_cmos_sensor(0x3780, 0x80);
		MT9P015_write_cmos_sensor(0x3781, 0x00);
		MT9P015_write_cmos_sensor(0x0104, 0x00);
	}
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    MT9P015_feature_control
*
* DESCRIPTION
*    This function control sensor mode
*
* PARAMETERS
*    id: scenario id
*    image_window: image grab window
*    cfg_data: config data
*
* RETURNS
*    error code
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 MT9P015_feature_control(MSDK_SENSOR_FEATURE_ENUM id, kal_uint8 *para, kal_uint32 *len)
{
	UINT32 *pFeatureData32=(UINT32 *) para;
  switch (id)
  {
  case SENSOR_FEATURE_GET_RESOLUTION: /* no use */
    MT9P015_get_size((kal_uint16 *)para, (kal_uint16 *)(para + sizeof(kal_uint16)));
    *len = sizeof(kal_uint32);
    break;
  case SENSOR_FEATURE_GET_PERIOD:
    MT9P015_get_period((kal_uint16 *)para, (kal_uint16 *)(para + sizeof(kal_uint16)));
    *len = sizeof(kal_uint32);
    break;
  case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
    *(kal_uint32 *)para = MT9P015_sensor.vt_clk;
    *len = sizeof(kal_uint32);
    break;
  case SENSOR_FEATURE_SET_ESHUTTER:
    MT9P015_set_shutter(*(kal_uint16 *)para);
    break;
  case SENSOR_FEATURE_SET_NIGHTMODE:
    MT9P015_night_mode((kal_bool)*(kal_uint16 *)para);
    break;
  case SENSOR_FEATURE_SET_GAIN:
    MT9P015_set_gain(*(kal_uint16 *)para);
    break;
  case SENSOR_FEATURE_SET_FLASHLIGHT:
    MT9P015_set_flashlight((kal_bool)*(kal_uint16 *)para);
   break;
  case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
    *(kal_uint32 *)para = 13000000; /* not ready */
    break;
  case SENSOR_FEATURE_SET_REGISTER:
    MT9P015_write_cmos_sensor(((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegAddr, ((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegData);
    break;
  case SENSOR_FEATURE_GET_REGISTER:
    ((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegData = MT9P015_read_cmos_sensor(((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegAddr);
    break;
  case SENSOR_FEATURE_SET_CCT_REGISTER:
    memcpy(&MT9P015_sensor.eng.cct, para, sizeof(MT9P015_sensor.eng.cct));
    break;
  case SENSOR_FEATURE_GET_CCT_REGISTER:
    if (*len >= sizeof(MT9P015_sensor.eng.cct) + sizeof(kal_uint32))
    {
      *((kal_uint32 *)para++) = sizeof(MT9P015_sensor.eng.cct);
      memcpy(para, &MT9P015_sensor.eng.cct, sizeof(MT9P015_sensor.eng.cct));
    }
    break;
  case SENSOR_FEATURE_SET_ENG_REGISTER:
    memcpy(&MT9P015_sensor.eng.reg, para, sizeof(MT9P015_sensor.eng.reg));
    break;
  case SENSOR_FEATURE_GET_ENG_REGISTER:
    if (*len >= sizeof(MT9P015_sensor.eng.reg) + sizeof(kal_uint32))
    {
      *((kal_uint32 *)para++) = sizeof(MT9P015_sensor.eng.reg);
      memcpy(para, &MT9P015_sensor.eng.reg, sizeof(MT9P015_sensor.eng.reg));
    }
    break;
  case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    ((PNVRAM_SENSOR_DATA_STRUCT)para)->Version = NVRAM_CAMERA_SENSOR_FILE_VERSION;
    ((PNVRAM_SENSOR_DATA_STRUCT)para)->SensorId = MT9P015_SENSOR_ID;
    memcpy(((PNVRAM_SENSOR_DATA_STRUCT)para)->SensorEngReg, &MT9P015_sensor.eng.reg, sizeof(MT9P015_sensor.eng.reg));
    memcpy(((PNVRAM_SENSOR_DATA_STRUCT)para)->SensorCCTReg, &MT9P015_sensor.eng.cct, sizeof(MT9P015_sensor.eng.cct));
    *len = sizeof(NVRAM_SENSOR_DATA_STRUCT);
    break;
  case SENSOR_FEATURE_GET_CONFIG_PARA: /* no use */
    memcpy(para, &MT9P015_sensor.cfg_data, sizeof(MT9P015_sensor.cfg_data));
    *len = sizeof(MT9P015_sensor.cfg_data);
    break;
  case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    MT9P015_camera_para_to_sensor();
    break;
  case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    MT9P015_sensor_to_camera_para();
    break;
  case SENSOR_FEATURE_GET_GROUP_COUNT:
    MT9P015_get_sensor_group_count((kal_uint32 *)para);
    *len = 4;
    break;
  case SENSOR_FEATURE_GET_GROUP_INFO:
    MT9P015_get_sensor_group_info((MSDK_SENSOR_GROUP_INFO_STRUCT *)para);
    *len = sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
    break;
  case SENSOR_FEATURE_GET_ITEM_INFO:
    MT9P015_get_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)para);
    *len = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
    break;
  case SENSOR_FEATURE_SET_ITEM_INFO:
    MT9P015_set_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)para);
    *len = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
    break;
  case SENSOR_FEATURE_GET_ENG_INFO:
    memcpy(para, &MT9P015_sensor.eng_info, sizeof(MT9P015_sensor.eng_info));
    *len = sizeof(MT9P015_sensor.eng_info);
    break;
  case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
    /*
     * get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
     * if EEPROM does not exist in camera module.
     */
    *(kal_uint32 *)para = LENS_DRIVER_ID_DO_NOT_CARE;
    *len = sizeof(kal_uint32);
    break;
  case SENSOR_FEATURE_SET_VIDEO_MODE:
    MT9P015_set_video_mode(*(kal_uint16 *)para);
    break;
  case SENSOR_FEATURE_SET_CALIBRATION_DATA:
    MT9P015SetCalData((PSET_SENSOR_CALIBRATION_DATA_STRUCT)para);
    break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		MT9P015_GetSensorID(pFeatureData32); 
		break; 
  default:
    break;
  }
  return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    MT9P015_control
*
* DESCRIPTION
*    This function set sensor mode
*
* PARAMETERS
*    id: scenario id
*    image_window: image grab window
*    cfg_data: config data
*
* RETURNS
*    error code
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 MT9P015_control(MSDK_SCENARIO_ID_ENUM id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window, MSDK_SENSOR_CONFIG_STRUCT *cfg_data)
{
  switch (id)
  {
  case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
  case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
  case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
    return MT9P015_preview(image_window, cfg_data);
  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
    return MT9P015_capture(image_window, cfg_data);
  default:
    return ERROR_INVALID_SCENARIO_ID;
  }
}

/*************************************************************************
* FUNCTION
*    MT9P015_power_off
*
* DESCRIPTION
*    This function is to turn off sensor module power.
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 MT9P015_power_off(void)
{
  MT9P015_write_cmos_sensor(0x0100, 0x00);
  MT9P015_write_cmos_sensor(0x0103, 0x01);
  
  return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*    MT9P015SensorInit
*
* DESCRIPTION
*    This function maps the external camera module function API structure.
*
* PARAMETERS
*    pfunc: function pointer
*
* RETURNS
*    error code
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 MT9P015SensorInit(PSENSOR_FUNCTION_STRUCT *pfunc)
{
  static SENSOR_FUNCTION_STRUCT MT9P015_sensor_func =
  {
    MT9P015_init,
    MT9P015_get_info,
    MT9P015_get_resolution,
    MT9P015_feature_control,
    MT9P015_control,
    MT9P015_power_off,
  };
  
  if (pfunc)
  {
    *pfunc = &MT9P015_sensor_func;
  }
  
  return ERROR_NONE;
}

