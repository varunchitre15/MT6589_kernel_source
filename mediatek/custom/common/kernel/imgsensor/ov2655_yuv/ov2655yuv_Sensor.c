/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.c
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author:
 * -------
 *   PC Huang (MTK02204)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 04 23 2012 yan.xu
 * [ALPS00271165] [FPB&ICS Done]modify sensor driver for MT6577
 * .
 *
 * 10 12 2010 sean.cheng
 * [ALPS00021722] [Need Patch] [Volunteer Patch][Camera]MT6573 Camera related function
 * .rollback the lib3a for mt6573 camera related files
 *
 * 09 10 2010 jackie.su
 * [ALPS00002279] [Need Patch] [Volunteer Patch] ALPS.Wxx.xx Volunteer patch for
 * .alps dual sensor
 *
 * 09 02 2010 jackie.su
 * [ALPS00002279] [Need Patch] [Volunteer Patch] ALPS.Wxx.xx Volunteer patch for
 * .roll back dual sensor
 *
 * 07 27 2010 sean.cheng
 * [ALPS00003112] [Need Patch] [Volunteer Patch] ISP/Sensor driver modification for Customer support
 * .1. add master clock switcher 
 *  2. add master enable/disable 
 *  3. add dummy line/pixel for sensor 
 *  4. add sensor driving current setting
 *
 * 07 01 2010 sean.cheng
 * [ALPS00121215][Camera] Change color when switch low and high 
 * .Add video delay frame.
 *
 * 07 01 2010 sean.cheng
 * [ALPS00002805][Need Patch] [Volunteer Patch]10X Patch for DS269 Video Frame Rate 
 * .Change the sensor clock to let frame rate to be 30fps in vidoe mode
 *
 * 06 13 2010 sean.cheng
 * [ALPS00002514][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for E1k Camera 
 * .
 * 1. Add set zoom factor and capdelay frame for YUV sensor 
 * 2. Modify e1k sensor setting
 *
 * 05 25 2010 sean.cheng
 * [ALPS00002250][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for YUV video frame rate 
 * .
 * Add 15fps option for video mode
 *
 * 05 03 2010 sean.cheng
 * [ALPS00001357][Meta]CameraTool 
 * .
 * Fix OV2655 YUV sensor frame rate to 30fps in vidoe mode
 *
 * Mar 4 2010 mtk70508
 * [DUMA00154792] Sensor driver
 * 
 *
 * Mar 4 2010 mtk70508
 * [DUMA00154792] Sensor driver
 * 
 *
 * Mar 1 2010 mtk01118
 * [DUMA00025869] [Camera][YUV I/F & Query feature] check in camera code
 * 
 *
 * Feb 24 2010 mtk01118
 * [DUMA00025869] [Camera][YUV I/F & Query feature] check in camera code
 * 
 *
 * Nov 24 2009 mtk02204
 * [DUMA00015869] [Camera Driver] Modifiy camera related drivers for dual/backup sensor/lens drivers.
 * 
 *
 * Oct 29 2009 mtk02204
 * [DUMA00015869] [Camera Driver] Modifiy camera related drivers for dual/backup sensor/lens drivers.
 * 
 *
 * Oct 27 2009 mtk02204
 * [DUMA00015869] [Camera Driver] Modifiy camera related drivers for dual/backup sensor/lens drivers.
 * 
 *
 * Aug 13 2009 mtk01051
 * [DUMA00009217] [Camera Driver] CCAP First Check In
 * 
 *
 * Aug 5 2009 mtk01051
 * [DUMA00009217] [Camera Driver] CCAP First Check In
 * 
 *
 * Jul 17 2009 mtk01051
 * [DUMA00009217] [Camera Driver] CCAP First Check In
 * 
 *
 * Jul 7 2009 mtk01051
 * [DUMA00008051] [Camera Driver] Add drivers for camera high ISO binning mode.
 * Add ISO query info for Sensor
 *
 * May 18 2009 mtk01051
 * [DUMA00005921] [Camera] LED Flashlight first check in
 * 
 *
 * May 16 2009 mtk01051
 * [DUMA00005921] [Camera] LED Flashlight first check in
 * 
 *
 * May 16 2009 mtk01051
 * [DUMA00005921] [Camera] LED Flashlight first check in
 * 
 *
 * Apr 7 2009 mtk02204
 * [DUMA00004012] [Camera] Restructure and rename camera related custom folders and folder name of came
 * 
 *
 * Mar 27 2009 mtk02204
 * [DUMA00002977] [CCT] First check in of MT6516 CCT drivers
 *
 *
 * Mar 25 2009 mtk02204
 * [DUMA00111570] [Camera] The system crash after some operations
 *
 *
 * Mar 20 2009 mtk02204
 * [DUMA00002977] [CCT] First check in of MT6516 CCT drivers
 *
 *
 * Mar 2 2009 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 *
 *
 * Feb 24 2009 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 *
 *
 * Dec 27 2008 MTK01813
 * DUMA_MBJ CheckIn Files
 * created by clearfsimport
 *
 * Dec 10 2008 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 *
 *
 * Oct 27 2008 mtk01051
 * [DUMA00000851] Camera related drivers check in
 * Modify Copyright Header
 *
 * Oct 24 2008 mtk02204
 * [DUMA00000851] Camera related drivers check in
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
//#include <windows.h>
//#include <memory.h>
//#include <nkintr.h>
//#include <ceddk.h>
//#include <ceddk_exp.h>

//#include "kal_release.h"
//#include "i2c_exp.h"
//#include "gpio_exp.h"
//#include "msdk_exp.h"
//#include "msdk_sensor_exp.h"
//#include "msdk_isp_exp.h"
//#include "base_regs.h"
//#include "Sensor.h"
//#include "camera_sensor_para.h"
//#include "CameraCustomized.h"

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/system.h>

//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "ov2655yuv_Sensor.h"
#include "ov2655yuv_Camera_Sensor_para.h"
#include "ov2655yuv_CameraCustomized.h"

#define OV2655YUV_DEBUG
#ifdef OV2655YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

static DEFINE_SPINLOCK(ov2655_yuv_drv_lock);

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define OV2655_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para ,1,OV2655_WRITE_ID)
#define OV2655_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,OV2655_WRITE_ID)
kal_uint16 OV2655_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV2655_WRITE_ID);
    return get_byte;
}


/*******************************************************************************
* // Adapter for Winmo typedef 
********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT


/*******************************************************************************
* // End Adapter for Winmo typedef 
********************************************************************************/


#define	OV2655_LIMIT_EXPOSURE_LINES				(1253)
#define	OV2655_VIDEO_NORMALMODE_30FRAME_RATE       (30)
#define	OV2655_VIDEO_NORMALMODE_FRAME_RATE         (15)
#define	OV2655_VIDEO_NIGHTMODE_FRAME_RATE          (7.5)
#define BANDING50_30HZ
/* Global Valuable */

static kal_uint32 zoom_factor = 0; 
static kal_uint8 OV2655_exposure_line_h = 0, OV2655_exposure_line_l = 0,OV2655_extra_exposure_line_h = 0, OV2655_extra_exposure_line_l = 0;

static kal_bool OV2655_gPVmode = KAL_TRUE; //PV size or Full size
static kal_bool OV2655_VEDIO_encode_mode = KAL_FALSE; //Picture(Jpeg) or Video(Mpeg4)
static kal_bool OV2655_sensor_cap_state = KAL_FALSE; //Preview or Capture

static kal_uint16 OV2655_dummy_pixels=0, OV2655_dummy_lines=0;

static kal_uint16 OV2655_exposure_lines=0, OV2655_extra_exposure_lines = 0;


static kal_int8 OV2655_DELAY_AFTER_PREVIEW = -1;

static kal_uint8 OV2655_Banding_setting = AE_FLICKER_MODE_50HZ;  //Wonder add

/****** OVT 6-18******/
static kal_uint16 OV2655_Capture_Max_Gain16= 6*16;
static kal_uint16 OV2655_Capture_Gain16=0 ;    
static kal_uint16 OV2655_Capture_Shutter=0;
static kal_uint16 OV2655_Capture_Extra_Lines=0;

static kal_uint16  OV2655_PV_Dummy_Pixels =0, OV2655_Capture_Dummy_Pixels =0, OV2655_Capture_Dummy_Lines =0;
static kal_uint16  OV2655_PV_Gain16 = 0;
static kal_uint16  OV2655_PV_Shutter = 0;
static kal_uint16  OV2655_PV_Extra_Lines = 0;

kal_uint16 OV2655_sensor_gain_base=0,OV2655_FAC_SENSOR_REG=0,OV2655_iOV2655_Mode=0,OV2655_max_exposure_lines=0;
kal_uint32 OV2655_capture_pclk_in_M=520,OV2655_preview_pclk_in_M=390,OV2655_PV_dummy_pixels=0,OV2655_PV_dummy_lines=0,OV2655_isp_master_clock=0;


static kal_uint32  OV2655_sensor_pclk=390;
static kal_bool OV2655_AWB_ENABLE = KAL_TRUE; 
static kal_bool OV2655_AE_ENABLE = KAL_TRUE; 

static kal_uint32 Capture_Shutter = 0; 
static kal_uint32 Capture_Gain = 0; 

#if WINMO_USE
kal_uint8 OV2655_sensor_write_I2C_address = OV2655_WRITE_ID;
kal_uint8 OV2655_sensor_read_I2C_address = OV2655_READ_ID;

UINT32 OV2655GPIOBaseAddr;
HANDLE OV2655hGPIO;
HANDLE OV2655hDrvI2C;
I2C_TRANSACTION OV2655I2CConfig;
#endif 
UINT8 OV2655_PixelClockDivider=0;

//SENSOR_REG_STRUCT OV2655SensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
//SENSOR_REG_STRUCT OV2655SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
//	camera_para.SENSOR.cct	SensorCCT	=> SensorCCT
//	camera_para.SENSOR.reg	SensorReg
MSDK_SENSOR_CONFIG_STRUCT OV2655SensorConfigData;

#if WINMO_USE
void OV2655_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	OV2655I2CConfig.operation=I2C_OP_WRITE;
	OV2655I2CConfig.slaveAddr=OV2655_sensor_write_I2C_address>>1;
	OV2655I2CConfig.transfer_num=1;	/* TRANSAC_LEN */
	OV2655I2CConfig.transfer_len=3;
	OV2655I2CConfig.buffer[0]=(UINT8)(addr>>8);
	OV2655I2CConfig.buffer[1]=(UINT8)(addr&0xFF);
	OV2655I2CConfig.buffer[2]=(UINT8)para;
	DRV_I2CTransaction(OV2655hDrvI2C, &OV2655I2CConfig);

}	/* OV2655_write_cmos_sensor() */

kal_uint32 OV2655_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint8 get_byte=0xFF;

	OV2655I2CConfig.operation=I2C_OP_WRITE;
	OV2655I2CConfig.slaveAddr=OV2655_sensor_write_I2C_address>>1;
	OV2655I2CConfig.transfer_num=1;	/* TRANSAC_LEN */
	OV2655I2CConfig.transfer_len=2;
	OV2655I2CConfig.buffer[0]=(UINT8)(addr>>8);
	OV2655I2CConfig.buffer[1]=(UINT8)(addr&0xFF);
	DRV_I2CTransaction(OV2655hDrvI2C, &OV2655I2CConfig);

	OV2655I2CConfig.operation=I2C_OP_READ;
	OV2655I2CConfig.slaveAddr=OV2655_sensor_read_I2C_address>>1;
	OV2655I2CConfig.transfer_num=1;	/* TRANSAC_LEN */
	OV2655I2CConfig.transfer_len=1;
	DRV_I2CTransaction(OV2655hDrvI2C, &OV2655I2CConfig);
	get_byte=OV2655I2CConfig.buffer[0];

	return get_byte;
}	/* OV2655_read_cmos_sensor() */
#endif 
void OV2655_set_dummy(kal_uint16 pixels, kal_uint16 lines)
{
    kal_uint8 temp_reg1, temp_reg2;
    kal_uint16 temp_reg;

    /*Very Important: The line_length must < 0x1000, it is to say 0x3028 must < 0x10, or else the sensor will crash*/
    /*The dummy_pixel must < 2156*/
    if (pixels >= 2156) 
        pixels = 2155;
    if (pixels < 0x100)
    {
        OV2655_write_cmos_sensor(0x302c,(pixels&0xFF)); //EXHTS[7:0]
        temp_reg = OV2655_FULL_PERIOD_PIXEL_NUMS;
        OV2655_write_cmos_sensor(0x3029,(temp_reg&0xFF));         //H_length[7:0]
        OV2655_write_cmos_sensor(0x3028,((temp_reg&0xFF00)>>8));  //H_length[15:8]
    }
    else
    {
        OV2655_write_cmos_sensor(0x302c,0);
        temp_reg = pixels + OV2655_FULL_PERIOD_PIXEL_NUMS;
        OV2655_write_cmos_sensor(0x3029,(temp_reg&0xFF));         //H_length[7:0]
        OV2655_write_cmos_sensor(0x3028,((temp_reg&0xFF00)>>8));  //H_length[15:8]
    }

    // read out and + line
    temp_reg1 = OV2655_read_cmos_sensor(0x302B);    // VTS[b7~b0]
    temp_reg2 = OV2655_read_cmos_sensor(0x302A);    // VTS[b15~b8]
    temp_reg = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

    temp_reg += lines;

    OV2655_write_cmos_sensor(0x302B,(temp_reg&0xFF));         //VTS[7:0]
    OV2655_write_cmos_sensor(0x302A,((temp_reg&0xFF00)>>8));  //VTS[15:8]
}    /* OV2655_set_dummy */

kal_uint16 OV2655_read_OV2655_gain(void)
{
    kal_uint8  temp_reg;
    kal_uint16 sensor_gain;

    temp_reg=OV2655_read_cmos_sensor(0x3000);  


    sensor_gain=(16+(temp_reg&0x0F));
    if(temp_reg&0x10)
        sensor_gain<<=1;
    if(temp_reg&0x20)
        sensor_gain<<=1;
      
    if(temp_reg&0x40)
        sensor_gain<<=1;
      
    if(temp_reg&0x80)
        sensor_gain<<=1;
      
    return sensor_gain;
}  /* OV2655_read_OV2655_gain */
kal_uint16 OV2655_read_shutter(void)
{
    kal_uint8 temp_reg1, temp_reg2;
    kal_uint16 temp_reg, extra_exp_lines;

    temp_reg1 = OV2655_read_cmos_sensor(0x3003);    // AEC[b7~b0]
    temp_reg2 = OV2655_read_cmos_sensor(0x3002);    // AEC[b15~b8]
    temp_reg = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

    temp_reg1 = OV2655_read_cmos_sensor(0x302E);    // EXVTS[b7~b0]
    temp_reg2 = OV2655_read_cmos_sensor(0x302D);    // EXVTS[b15~b8]
    extra_exp_lines = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

    OV2655_PV_Shutter = temp_reg ;
    OV2655_PV_Extra_Lines = extra_exp_lines;
    return temp_reg + extra_exp_lines;
}    /* OV2655_read_shutter */

void OV2655_write_OV2655_gain(kal_uint16 gain)
{    
    kal_uint16 temp_reg;
   
	RETAILMSG(1, (TEXT("OV2655 write gain: %d\r\n"), gain));
   
   if(gain > 248)  return ;//ASSERT(0);
   
    temp_reg = 0;
    if (gain > 31)
    {
        temp_reg |= 0x10;
        gain = gain >> 1;
    }
    if (gain > 31)
    {
        temp_reg |= 0x20;
        gain = gain >> 1;
    }

    if (gain > 31)
    {
        temp_reg |= 0x40;
        gain = gain >> 1;
    }
    if (gain > 31)
    {
        temp_reg |= 0x80;
        gain = gain >> 1;
    }
    
    if (gain > 16)
    {
        temp_reg |= ((gain -16) & 0x0f);
    }   
  
   OV2655_write_cmos_sensor(0x3000,temp_reg);
}  /* OV2655_write_OV2655_gain */

static void OV2655_write_shutter(kal_uint16 shutter)
{
    if (OV2655_gPVmode) 
    {
        spin_lock(&ov2655_yuv_drv_lock);
        if (shutter <= OV2655_PV_EXPOSURE_LIMITATION) 
        {
            OV2655_extra_exposure_lines = 0;
        }
        else 
        {
            OV2655_extra_exposure_lines=shutter - OV2655_PV_EXPOSURE_LIMITATION;
        }
		spin_unlock(&ov2655_yuv_drv_lock); 

        if (shutter > OV2655_PV_EXPOSURE_LIMITATION) 
        {
            shutter = OV2655_PV_EXPOSURE_LIMITATION;
        }
    }
    else 
    {
        spin_lock(&ov2655_yuv_drv_lock);
        if (shutter <= OV2655_FULL_EXPOSURE_LIMITATION) 
        {
            OV2655_extra_exposure_lines = 0;
    }
        else 
        {
            OV2655_extra_exposure_lines = shutter - OV2655_FULL_EXPOSURE_LIMITATION;
        }
        spin_unlock(&ov2655_yuv_drv_lock); 
        if (shutter > OV2655_FULL_EXPOSURE_LIMITATION) {
            shutter = OV2655_FULL_EXPOSURE_LIMITATION;
        }
    }

    // set extra exposure line
    OV2655_write_cmos_sensor(0x302E, OV2655_extra_exposure_lines & 0xFF);          // EXVTS[b7~b0]
    OV2655_write_cmos_sensor(0x302D, (OV2655_extra_exposure_lines & 0xFF00) >> 8); // EXVTS[b15~b8]

    /* Max exporsure time is 1 frmae period event if Tex is set longer than 1 frame period */
    OV2655_write_cmos_sensor(0x3003, shutter & 0xFF);           //AEC[7:0]
    OV2655_write_cmos_sensor(0x3002, (shutter & 0xFF00) >> 8);  //AEC[8:15]

}    /* OV2655_write_shutter */
/*
void OV2655_Computer_AEC(kal_uint16 preview_clk_in_M, kal_uint16 capture_clk_in_M)
{
    kal_uint16 PV_Line_Width;
    kal_uint16 Capture_Line_Width;
    kal_uint16 Capture_Maximum_Shutter;
    kal_uint16 Capture_Exposure;
    kal_uint16 Capture_Gain16;
    kal_uint16 Capture_Banding_Filter;
    kal_uint32 Gain_Exposure=0;

    PV_Line_Width = OV2655_PV_PERIOD_PIXEL_NUMS + OV2655_PV_Dummy_Pixels;   

    Capture_Line_Width = OV2655_FULL_PERIOD_PIXEL_NUMS + OV2655_Capture_Dummy_Pixels;
    Capture_Maximum_Shutter = OV2655_FULL_EXPOSURE_LIMITATION + OV2655_Capture_Dummy_Lines;
    Gain_Exposure = 1;
    ///////////////////////
    Gain_Exposure *=(OV2655_PV_Shutter+OV2655_PV_Extra_Lines);
    Gain_Exposure *=PV_Line_Width;  //970
    //   Gain_Exposure /=g_Preview_PCLK_Frequency;
    Gain_Exposure /=Capture_Line_Width;//1940
    Gain_Exposure = Gain_Exposure*capture_clk_in_M/preview_clk_in_M;// for clock   

    //OV2655_Capture_Gain16 = Capture_Gain16;
    OV2655_Capture_Extra_Lines = (Gain_Exposure > Capture_Maximum_Shutter)?
            (Gain_Exposure - Capture_Maximum_Shutter):0;     
    
    OV2655_Capture_Shutter = Gain_Exposure - OV2655_Capture_Extra_Lines;
}
*/

void OV2655_Computer_AECAGC(kal_uint16 preview_clk_in_M, kal_uint16 capture_clk_in_M)
{
    kal_uint16 PV_Line_Width;
    kal_uint16 Capture_Line_Width;
    kal_uint16 Capture_Maximum_Shutter;
    kal_uint16 Capture_Exposure;
    kal_uint16 Capture_Gain16;
	kal_uint16 temp_gain;
    kal_uint32 Capture_Banding_Filter;
    kal_uint32 Gain_Exposure=0;

    PV_Line_Width = OV2655_PV_PERIOD_PIXEL_NUMS + OV2655_PV_Dummy_Pixels;   

    Capture_Line_Width = OV2655_FULL_PERIOD_PIXEL_NUMS + OV2655_Capture_Dummy_Pixels;
    Capture_Maximum_Shutter = OV2655_FULL_EXPOSURE_LIMITATION + OV2655_Capture_Dummy_Lines;

    if (OV2655_Banding_setting == AE_FLICKER_MODE_50HZ)
#if WINMO_USE        
        Capture_Banding_Filter = (kal_uint32)(capture_clk_in_M*1000000/100/(2*Capture_Line_Width)+0.5);
#else 
        Capture_Banding_Filter = (kal_uint32)(capture_clk_in_M*100000/100/(2*Capture_Line_Width));
#endif 
    else
#if WINMO_USE
        Capture_Banding_Filter = (kal_uint16)(capture_clk_in_M*1000000/120/(2*Capture_Line_Width)+0.5);
#else 
        Capture_Banding_Filter = (kal_uint32)(capture_clk_in_M*100000/120/(2*Capture_Line_Width) );
#endif 

    /*   Gain_Exposure = OV2655_PV_Gain16*(OV2655_PV_Shutter+OV2655_PV_Extra_Lines)*PV_Line_Width/g_Preview_PCLK_Frequency/Capture_Line_Width*g_Capture_PCLK_Frequency
    ;*/
    temp_gain = OV2655_read_OV2655_gain();
    spin_lock(&ov2655_yuv_drv_lock);
	OV2655_PV_Gain16 = temp_gain;
	spin_unlock(&ov2655_yuv_drv_lock); 
    Gain_Exposure = 1 * OV2655_PV_Gain16;  //For OV2655
    ///////////////////////
    Gain_Exposure *=(OV2655_PV_Shutter+OV2655_PV_Extra_Lines);
    Gain_Exposure *=PV_Line_Width;  //970
    //   Gain_Exposure /=g_Preview_PCLK_Frequency;
    Gain_Exposure /=Capture_Line_Width;//1940
    Gain_Exposure = Gain_Exposure*capture_clk_in_M/preview_clk_in_M;// for clock   

    //redistribute gain and exposure
    if (Gain_Exposure < (kal_uint32)(Capture_Banding_Filter * 16))     // Exposure < 1/100/120
    {
       if(Gain_Exposure<16){//exposure line smaller than 2 lines and gain smaller than 0x08 
            Gain_Exposure = Gain_Exposure*4;     
            Capture_Exposure = 1;
            Capture_Gain16 = (Gain_Exposure*2 + 1)/Capture_Exposure/2/4;
        }
        else
        {
            Capture_Exposure = Gain_Exposure /16;
            Capture_Gain16 = (Gain_Exposure*2 + 1)/Capture_Exposure/2;
        }
    }
    else 
    {
        if (Gain_Exposure >(kal_uint32)( Capture_Maximum_Shutter * 16)) // Exposure > Capture_Maximum_Shutter
        {
           
            Capture_Exposure = Capture_Maximum_Shutter/Capture_Banding_Filter*Capture_Banding_Filter;
            Capture_Gain16 = (Gain_Exposure*2 + 1)/Capture_Exposure/2;
            if (Capture_Gain16 > OV2655_Capture_Max_Gain16) 
            {
                // gain reach maximum, insert extra line
                Capture_Exposure = (kal_uint16)(Gain_Exposure*11 /10 /OV2655_Capture_Max_Gain16);
                
                // Exposure = n/100/120
                Capture_Exposure = Capture_Exposure/Capture_Banding_Filter * Capture_Banding_Filter;
                Capture_Gain16 = ((Gain_Exposure *4)/ Capture_Exposure+3)/4;
            }
        }
        else  // 1/100 < Exposure < Capture_Maximum_Shutter, Exposure = n/100/120
        {
            Capture_Exposure = Gain_Exposure/16/Capture_Banding_Filter;
            Capture_Exposure = Capture_Exposure * Capture_Banding_Filter;
            Capture_Gain16 = (Gain_Exposure*2 +1) / Capture_Exposure/2;
        }
    }
    spin_lock(&ov2655_yuv_drv_lock);
    OV2655_Capture_Gain16 = Capture_Gain16;
    OV2655_Capture_Extra_Lines = (Capture_Exposure > Capture_Maximum_Shutter)?
            (Capture_Exposure - Capture_Maximum_Shutter/Capture_Banding_Filter*Capture_Banding_Filter):0;     
    
    OV2655_Capture_Shutter = Capture_Exposure - OV2655_Capture_Extra_Lines;
	spin_unlock(&ov2655_yuv_drv_lock); 
}

void OV2655_set_isp_driving_current(kal_uint8 current)
{
}


static void OV2655_set_AE_mode(kal_bool AE_enable)
{
    kal_uint8 temp_AE_reg = 0;

    if (AE_enable == KAL_TRUE)
    {
        // turn on AEC/AGC
        temp_AE_reg = OV2655_read_cmos_sensor(0x3013);
        OV2655_write_cmos_sensor(0x3013, temp_AE_reg| 0x05);
    }
    else
    {
        // turn off AEC/AGC
        temp_AE_reg = OV2655_read_cmos_sensor(0x3013);
        OV2655_write_cmos_sensor(0x3013, temp_AE_reg&~0x05);
    }
}


static void OV2655_set_AWB_mode(kal_bool AWB_enable)
{
    kal_uint8 temp_AWB_reg = 0;

    //return ;

    if (AWB_enable == KAL_TRUE)
    {
        //enable Auto WB
        temp_AWB_reg = OV2655_read_cmos_sensor(0x3324);
        OV2655_write_cmos_sensor(0x3324, temp_AWB_reg & ~0x40);        
    }
    else
    {
        //turn off AWB
        temp_AWB_reg = OV2655_read_cmos_sensor(0x3324);
        OV2655_write_cmos_sensor(0x3324, temp_AWB_reg | 0x40);        
    }
}


/*************************************************************************
* FUNCTION
*	OV2655_night_mode
*
* DESCRIPTION
*	This function night mode of OV2655.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL OV2655_set_param_banding(UINT16 para); 
void OV2655_night_mode(kal_bool enable)
{
	
	kal_uint8 night = OV2655_read_cmos_sensor(0x3014); //bit[3], 0: disable, 1:enable
	//	kal_uint8 temp_AE_reg = OV2655_read_cmos_sensor(0x3013);
		 		 
		 
		// OV2655_write_cmos_sensor(0x3013, (temp_AE_reg&(~0x05)));		   
		/* ==Video Preview, Auto Mode, use 39MHz PCLK, 30fps; Night Mode use 39M, 15fps */
		if (OV2655_sensor_cap_state == KAL_FALSE) 
		{
			if (enable) 
			{
			  //  OV2655_write_cmos_sensor(0x302D, OV2655_extra_exposure_line_h);
			  //  OV2655_write_cmos_sensor(0x302E, OV2655_extra_exposure_line_l);	
				if (OV2655_VEDIO_encode_mode == KAL_TRUE) 
				{
					// set Max gain to 16X
					OV2655_write_cmos_sensor(0x3015, 0x03); //jerry
					OV2655_write_cmos_sensor(0x300e, 0x34);
					OV2655_write_cmos_sensor(0x302A, OV2655_VIDEO_15FPS_FRAME_LENGTH>>8);  /*  15fps*/
					OV2655_write_cmos_sensor(0x302B, OV2655_VIDEO_15FPS_FRAME_LENGTH&0xFF);
                
					OV2655_write_cmos_sensor(0x3014, night & 0xf7); //Disable night for fix framerate
					 // clear extra exposure line
					OV2655_write_cmos_sensor(0x302d, 0x00);
					OV2655_write_cmos_sensor(0x302e, 0x00);
				}
				else 
				{
					/* Camera mode only */				  
					OV2655_write_cmos_sensor(0x3015, 0x42);  //jerry 0624 for night mode
					OV2655_write_cmos_sensor(0x3014, night & 0xf7); //Disable night mode
					OV2655_write_cmos_sensor(0x3014, night | 0x08);
				}	   
			}
			else 
			{
				/* when enter normal mode (disable night mode) without light, the AE vibrate */
				if (OV2655_VEDIO_encode_mode == KAL_TRUE) 
				{
					/* MJPEG or MPEG4 Apps */
					OV2655_write_cmos_sensor(0x3015, 0x02); //0x51
					OV2655_write_cmos_sensor(0x300e, 0x34);
					OV2655_write_cmos_sensor(0x302A, OV2655_VIDEO_30FPS_FRAME_LENGTH>>8);	/*	30fps-->15fps*/
					OV2655_write_cmos_sensor(0x302B, OV2655_VIDEO_30FPS_FRAME_LENGTH&0xFF);
                	OV2655_write_cmos_sensor(0x3014, night & 0xf7); //Disable night mode  for fix framerate
				}
				else 
				{
					/* Camera mode only */
					// set Max gain to 4X
					OV2655_write_cmos_sensor(0x3015, 0x22);   
     				       OV2655_write_cmos_sensor(0x3014, night & 0xf7); //Disable night mode
					OV2655_write_cmos_sensor(0x3014, night | 0x08); 
				}	
				// clear extra exposure line
				OV2655_write_cmos_sensor(0x302d, 0x00);
				OV2655_write_cmos_sensor(0x302e, 0x00);
		}
	}
        OV2655_set_param_banding(OV2655_Banding_setting); 
}	/* OV2655_night_mode */



/*************************************************************************
* FUNCTION
*	OV2655_GetSensorID
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
static kal_uint32 OV2655_GetSensorID(kal_uint32 *sensorID)

{
	volatile signed char i;
		kal_uint32 sensor_id=0;
		kal_uint8 temp_sccb_addr = 0;
		//s_move to here from CISModulePowerOn()

		OV2655_write_cmos_sensor(0x3012,0x80);// Reset sensor
			mDELAY(10);
		
		
			//	Read sensor ID to adjust I2C is OK?
			for(i=0;i<3;i++)
			{
				sensor_id = (OV2655_read_cmos_sensor(0x300A) << 8) | OV2655_read_cmos_sensor(0x300B);
				if(sensor_id != OV2655_SENSOR_ID)
				{
					*sensorID =0xFFFFFFFF;
					return ERROR_SENSOR_CONNECT_FAIL;
				}
			}
			
			RETAILMSG(1, (TEXT("OV2655 Sensor Read ID OK \r\n")));
		
    return ERROR_NONE;    
}   
/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*	OV2655Open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
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
UINT32 OV2655Open(void)
{
	volatile signed char i;
	kal_uint16 sensor_id=0;

#if WINMO_USE	
	OV2655hDrvI2C=DRV_I2COpen(0);
	DRV_I2CSetParam(OV2655hDrvI2C, I2C_VAL_FS_SAMPLE_DIV, 3);
	DRV_I2CSetParam(OV2655hDrvI2C, I2C_VAL_FS_STEP_DIV, 8);
	DRV_I2CSetParam(OV2655hDrvI2C, I2C_VAL_DELAY_LEN, 2);
	DRV_I2CSetParam(OV2655hDrvI2C, I2C_VAL_CLK_EXT, I2C_CLKEXT_DISABLE);
	OV2655I2CConfig.trans_mode=I2C_TRANSFER_FAST_MODE;
	OV2655I2CConfig.slaveAddr=OV2655_sensor_write_I2C_address>>1;
	OV2655I2CConfig.RS_ST=I2C_TRANSFER_STOP;	/* for fast mode */

	CISModulePowerOn(KAL_TRUE);      // Power On CIS Power
	Sleep(10);							// To wait for Stable Power
#endif 
	spin_lock(&ov2655_yuv_drv_lock);
	zoom_factor = 0; 
	spin_unlock(&ov2655_yuv_drv_lock); 

	OV2655_write_cmos_sensor(0x3012,0x80);// Reset sensor
    Sleep(10);


	//  Read sensor ID to adjust I2C is OK?
	for(i=0;i<3;i++)
	{
		sensor_id = (OV2655_read_cmos_sensor(0x300A) << 8) | OV2655_read_cmos_sensor(0x300B);
		if(sensor_id != OV2655_SENSOR_ID)
		{
			return ERROR_SENSOR_CONNECT_FAIL;
		}
	}
	
	RETAILMSG(1, (TEXT("OV2655 Sensor Read ID OK \r\n")));
	OV2655_write_cmos_sensor(0x308c,0x80); //TMC12: DIS_MIPI_RW
    OV2655_write_cmos_sensor(0x308d,0x0e); //TMC13: MIPI disable
    OV2655_write_cmos_sensor(0x360b,0x00);
    OV2655_write_cmos_sensor(0x30b0,0xff); //IO_CTRL0: Cy[7:0]
    OV2655_write_cmos_sensor(0x30b1,0xff); //IO_CTRL1: C_VSYNC,C_STROBE,C_PCLK,C_HREF,Cy[9:8]
    OV2655_write_cmos_sensor(0x30b2,0x2c); //IO_CTRL2: R_PAD[3:0]

	
	//Internal clock = (64-0x300e[5:0])*0x300f[7:6]*MCLK/0x300f[2:0]/0x3010[4]/(0x3011[5:0]+1)/4
    OV2655_write_cmos_sensor(0x300f,0xa6);
    OV2655_write_cmos_sensor(0x3010,0x81);
	
    OV2655_write_cmos_sensor(0x3082,0x01);
    OV2655_write_cmos_sensor(0x30f4,0x01);
    OV2655_write_cmos_sensor(0x3091,0xc0);
    OV2655_write_cmos_sensor(0x30ac,0x42);

    OV2655_write_cmos_sensor(0x30d1,0x08);
    OV2655_write_cmos_sensor(0x3015,0x02); //AUTO_3: AGC ceiling = 4x, 5dummy frame
    OV2655_write_cmos_sensor(0x3093,0x00);
    OV2655_write_cmos_sensor(0x307e,0xe5); //TMC8: AGC[7:6]=b'11
    OV2655_write_cmos_sensor(0x3079,0x00);
    OV2655_write_cmos_sensor(0x3017,0x40); //AUTO_5: disable data drop, manual banding counter=0
    OV2655_write_cmos_sensor(0x30f3,0x82);
    OV2655_write_cmos_sensor(0x306a,0x0c); //0x0c->0x0f Joe 0814 : BLC
    OV2655_write_cmos_sensor(0x306d,0x00);
    OV2655_write_cmos_sensor(0x336a,0x3c);
    OV2655_write_cmos_sensor(0x3076,0x6a); //TMC0: VSYNC drop option: drop
    OV2655_write_cmos_sensor(0x30d9,0x95);
    OV2655_write_cmos_sensor(0x3601,0x30);
    OV2655_write_cmos_sensor(0x304e,0x88);
    OV2655_write_cmos_sensor(0x30f1,0x82);
    OV2655_write_cmos_sensor(0x306f,0x14);
    OV2655_write_cmos_sensor(0x302a, (OV2655_PV_PERIOD_LINE_NUMS>>8)&0x00ff);
    OV2655_write_cmos_sensor(0x302b, OV2655_PV_PERIOD_LINE_NUMS&0x00ff); 
    

    OV2655_write_cmos_sensor(0x3012,0x10);

    //AEC/AGC
    OV2655_write_cmos_sensor(0x3018,0x80); // jerry, 0624
    OV2655_write_cmos_sensor(0x3019,0x70); 
    OV2655_write_cmos_sensor(0x301a,0xd4);   
    OV2655_write_cmos_sensor(0x3013,0xf7);

    //D5060
    OV2655_write_cmos_sensor(0x30af,0x10);
    OV2655_write_cmos_sensor(0x304a,0x00);
    OV2655_write_cmos_sensor(0x304f,0x00);
    OV2655_write_cmos_sensor(0x30a3,0x80); 
    //OV2655_write_cmos_sensor(0x3013,0xf7); 
    OV2655_write_cmos_sensor(0x3014,0xa4); //R1D bit6 always = 0 , bit[5]=1, bit[0]=1   
    OV2655_write_cmos_sensor(0x3071,0x00);
    OV2655_write_cmos_sensor(0x3073,0x00);
    OV2655_write_cmos_sensor(0x304d,0x42);
    OV2655_write_cmos_sensor(0x304a,0x00); //Disable 50/60 auto detection function, due to ov2650 no this function
    OV2655_write_cmos_sensor(0x304f,0x00); //Disable 50/60 auto detection function, due to ov2650 no this function
    OV2655_write_cmos_sensor(0x3095,0x07);
    OV2655_write_cmos_sensor(0x3096,0x16);
    OV2655_write_cmos_sensor(0x3097,0x1d);

    //Window Setup
    OV2655_write_cmos_sensor(0x3020,0x01); 
    OV2655_write_cmos_sensor(0x3021,0x18); 
    OV2655_write_cmos_sensor(0x3022,0x00);
    OV2655_write_cmos_sensor(0x3023,0x06);
    OV2655_write_cmos_sensor(0x3024,0x06);
    OV2655_write_cmos_sensor(0x3025,0x58);
    OV2655_write_cmos_sensor(0x3026,0x02);
    OV2655_write_cmos_sensor(0x3027,0x5e);
    OV2655_write_cmos_sensor(0x3088,0x03);
    OV2655_write_cmos_sensor(0x3089,0x20);
    OV2655_write_cmos_sensor(0x308a,0x02);
    OV2655_write_cmos_sensor(0x308b,0x58);
    OV2655_write_cmos_sensor(0x3316,0x64);
    OV2655_write_cmos_sensor(0x3317,0x25);
    OV2655_write_cmos_sensor(0x3318,0x80);
    OV2655_write_cmos_sensor(0x3319,0x08);
    OV2655_write_cmos_sensor(0x331a,0x64);
    OV2655_write_cmos_sensor(0x331b,0x4b);
    OV2655_write_cmos_sensor(0x331c,0x00);
    OV2655_write_cmos_sensor(0x331d,0x38);
    OV2655_write_cmos_sensor(0x3100,0x00);

    //AWB
    OV2655_write_cmos_sensor(0x3320,0xfa);   //Jerry 0x9a
    OV2655_write_cmos_sensor(0x3321,0x11);   
    OV2655_write_cmos_sensor(0x3322,0x92);   
    OV2655_write_cmos_sensor(0x3323,0x01);   
    OV2655_write_cmos_sensor(0x3324,0x97);   //Jerry 0x92   
    OV2655_write_cmos_sensor(0x3325,0x02);   
    OV2655_write_cmos_sensor(0x3326,0xff);   
    OV2655_write_cmos_sensor(0x3327,0x10);   //0x0c
    OV2655_write_cmos_sensor(0x3328,0x11);   //Jerry 0x0f
    OV2655_write_cmos_sensor(0x3329,0x16);   //Jerry 0x14
    OV2655_write_cmos_sensor(0x332a,0x59);   //Jerry 0x66
    OV2655_write_cmos_sensor(0x332b,0x60);   //5f -> 5c  //Jerry 0x5c
    OV2655_write_cmos_sensor(0x332c,0xbe);   //a5 -> 89    //Jerry 0x89
    OV2655_write_cmos_sensor(0x332d,0x9b);   //ac -> 96    //Jerry 0x96
    OV2655_write_cmos_sensor(0x332e,0x34);   //35 -> 3d    //Jerry 0x3d
    OV2655_write_cmos_sensor(0x332f,0x36);   //Jerry 0x2f
    OV2655_write_cmos_sensor(0x3330,0x49);   //Jerry 0x57
    OV2655_write_cmos_sensor(0x3331,0x44);   //Jerry 0x3d
    OV2655_write_cmos_sensor(0x3332,0xf0);   
    OV2655_write_cmos_sensor(0x3333,0x00);  //0x10 
    OV2655_write_cmos_sensor(0x3334,0xf0);   
    OV2655_write_cmos_sensor(0x3335,0xf0);   
    OV2655_write_cmos_sensor(0x3336,0xf0);   
    OV2655_write_cmos_sensor(0x3337,0x40);   
    OV2655_write_cmos_sensor(0x3338,0x40);   
    OV2655_write_cmos_sensor(0x3339,0x40);   
    OV2655_write_cmos_sensor(0x333a,0x00);   
    OV2655_write_cmos_sensor(0x333b,0x00);   

    //Color Matrix
    OV2655_write_cmos_sensor(0x3380,0x27); 
    OV2655_write_cmos_sensor(0x3381,0x5c); 
    OV2655_write_cmos_sensor(0x3382,0x0a); 
    OV2655_write_cmos_sensor(0x3383,0x29);  //0x2a
    OV2655_write_cmos_sensor(0x3384,0xab);  //0xad
    OV2655_write_cmos_sensor(0x3385,0xd3); //d5
    OV2655_write_cmos_sensor(0x3386,0xbf);  
    OV2655_write_cmos_sensor(0x3387,0xbc); 
    OV2655_write_cmos_sensor(0x3388,0x03); 
    OV2655_write_cmos_sensor(0x3389,0x98); 
    OV2655_write_cmos_sensor(0x338a,0x01); 

    //Gamma
    OV2655_write_cmos_sensor(0x3340,0x0c);  
    OV2655_write_cmos_sensor(0x3341,0x18);  
    OV2655_write_cmos_sensor(0x3342,0x30); 
    OV2655_write_cmos_sensor(0x3343,0x3d); 
    OV2655_write_cmos_sensor(0x3344,0x4b); 
    OV2655_write_cmos_sensor(0x3345,0x59);  //0x60 
    OV2655_write_cmos_sensor(0x3346,0x67);  //0x6a
    OV2655_write_cmos_sensor(0x3347,0x71);  
    OV2655_write_cmos_sensor(0x3348,0x7d);  //0x84
    OV2655_write_cmos_sensor(0x3349,0x8e);  //0x96
    OV2655_write_cmos_sensor(0x334a,0x9b);  //0xa2
    OV2655_write_cmos_sensor(0x334b,0xa6);  //0xac
    OV2655_write_cmos_sensor(0x334c,0xb9);
    OV2655_write_cmos_sensor(0x334d,0xc6);
    OV2655_write_cmos_sensor(0x334e,0xd9);
    OV2655_write_cmos_sensor(0x334f,0x34);    

    //Lens correction
    //R
#if 0    
    OV2655_write_cmos_sensor(0x3350,0x35); 
    OV2655_write_cmos_sensor(0x3351,0x25);
    OV2655_write_cmos_sensor(0x3352,0x08);
    OV2655_write_cmos_sensor(0x3353,0x23);
    OV2655_write_cmos_sensor(0x3354,0x00);
    OV2655_write_cmos_sensor(0x3355,0x85);
    //G
    OV2655_write_cmos_sensor(0x3356,0x34);
    OV2655_write_cmos_sensor(0x3357,0x25);
    OV2655_write_cmos_sensor(0x3358,0x08);
    OV2655_write_cmos_sensor(0x3359,0x1e);
    OV2655_write_cmos_sensor(0x335a,0x00);
    OV2655_write_cmos_sensor(0x335b,0x85);
    //B
    OV2655_write_cmos_sensor(0x335c,0x35);
    OV2655_write_cmos_sensor(0x335d,0x25);
    OV2655_write_cmos_sensor(0x335e,0x08);
    OV2655_write_cmos_sensor(0x335f,0x18);
    OV2655_write_cmos_sensor(0x3360,0x00);
    OV2655_write_cmos_sensor(0x3361,0x85);

    OV2655_write_cmos_sensor(0x3363,0x01);
    OV2655_write_cmos_sensor(0x3364,0x03);
    OV2655_write_cmos_sensor(0x3365,0x02);
    OV2655_write_cmos_sensor(0x3366,0x00);
#else 
    OV2655_write_cmos_sensor(0x3350,0x33); 
    OV2655_write_cmos_sensor(0x3351,0x27);
    OV2655_write_cmos_sensor(0x3352,0x08);
    OV2655_write_cmos_sensor(0x3353,0x23);
    OV2655_write_cmos_sensor(0x3354,0x00);
    OV2655_write_cmos_sensor(0x3355,0x85);
    //G
    OV2655_write_cmos_sensor(0x3356,0x32);
    OV2655_write_cmos_sensor(0x3357,0x27);
    OV2655_write_cmos_sensor(0x3358,0x08);
    OV2655_write_cmos_sensor(0x3359,0x1e);
    OV2655_write_cmos_sensor(0x335a,0x00);
    OV2655_write_cmos_sensor(0x335b,0x85);
    //B
    OV2655_write_cmos_sensor(0x335c,0x33);
    OV2655_write_cmos_sensor(0x335d,0x27);
    OV2655_write_cmos_sensor(0x335e,0x08);
    OV2655_write_cmos_sensor(0x335f,0x1b); 
    OV2655_write_cmos_sensor(0x3360,0x00);
    OV2655_write_cmos_sensor(0x3361,0x85);
#endif     

    //UVadjust
    OV2655_write_cmos_sensor(0x338b,0x0C); //auto uv  //add saturation
    OV2655_write_cmos_sensor(0x338c,0x10);
    OV2655_write_cmos_sensor(0x338d,0x80);  //40  indoor saturation

    //Sharpness/De-noise
    OV2655_write_cmos_sensor(0x3370,0xd0);
    OV2655_write_cmos_sensor(0x3371,0x00);
    OV2655_write_cmos_sensor(0x3372,0x00);
    OV2655_write_cmos_sensor(0x3374,0x10);
    OV2655_write_cmos_sensor(0x3375,0x10);
    OV2655_write_cmos_sensor(0x3376,0x04); ///0624,jerry, for preview sharp
    OV2655_write_cmos_sensor(0x3377,0x00);
    OV2655_write_cmos_sensor(0x3378,0x04);
    OV2655_write_cmos_sensor(0x3379,0x40);

    //BLC
    OV2655_write_cmos_sensor(0x3069,0x80); //Jerry
    OV2655_write_cmos_sensor(0x3087,0x02);

    //Other functions
    OV2655_write_cmos_sensor(0x3300,0xfc);
    OV2655_write_cmos_sensor(0x3302,0x11);
    OV2655_write_cmos_sensor(0x3400,0x00);
    OV2655_write_cmos_sensor(0x3606,0x20);
    OV2655_write_cmos_sensor(0x3601,0x30);
    OV2655_write_cmos_sensor(0x30f3,0x83);
    OV2655_write_cmos_sensor(0x304e,0x88);
    OV2655_write_cmos_sensor(0x3086,0x0f); 
    OV2655_write_cmos_sensor(0x3086,0x00); //

	OV2655_write_cmos_sensor(0x3390, 0x41);// jeff add for effect init
    
    OV2655_write_cmos_sensor(0x30a8, 0x54); //0x56    for Sun black
    OV2655_write_cmos_sensor(0x30aa, 0x52);  //0x42   for Sun black
    OV2655_write_cmos_sensor(0x30af, 0x10); 
    OV2655_write_cmos_sensor(0x30b2, 0x2c); 
    OV2655_write_cmos_sensor(0x30d9, 0x8c);

    //for close Mipi Interface
    OV2655_write_cmos_sensor(0x363B,0x01);
    OV2655_write_cmos_sensor(0x363C,0xF2);

	return ERROR_NONE;
}	/* OV2655Open() */

/*************************************************************************
* FUNCTION
*	OV2655Close
*
* DESCRIPTION
*	This function is to turn off sensor module power.
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
UINT32 OV2655Close(void)
{
//	CISModulePowerOn(FALSE);

#if WINMO_USE
	#ifndef SOFTWARE_I2C_INTERFACE	/* software I2c MODE */
	DRV_I2CClose(OV2655hDrvI2C);
	#endif
#endif 	
	return ERROR_NONE;
}	/* OV2655Close() */

/*************************************************************************
* FUNCTION
*	OV2655Preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV2655Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint8 iTemp, temp_AE_reg, temp_AWB_reg;
    kal_uint16 iDummyPixels = 0, iDummyLines = 0, iStartX = 0, iStartY = 0;
    spin_lock(&ov2655_yuv_drv_lock);
	OV2655_sensor_cap_state = KAL_FALSE;
	spin_unlock(&ov2655_yuv_drv_lock);  
    
    
    //4  <1> preview config sequence
    OV2655_write_cmos_sensor(0x3002, OV2655_exposure_line_h * 1); //090623
    OV2655_write_cmos_sensor(0x3003, OV2655_exposure_line_l * 1);  //090623
    OV2655_write_cmos_sensor(0x302D, OV2655_extra_exposure_line_h * 1); //090623
    OV2655_write_cmos_sensor(0x302E, OV2655_extra_exposure_line_l * 1);  //090623

    OV2655_write_OV2655_gain(OV2655_PV_Gain16);
	spin_lock(&ov2655_yuv_drv_lock);
	OV2655_sensor_pclk=390;
	spin_unlock(&ov2655_yuv_drv_lock); 

    //0x3011 = 0;
    //0x300e=0x34
    //YUV    //SVGA (800x600)
    // setup sensor output ROI
    OV2655_write_cmos_sensor(0x3010, 0x81); 
    OV2655_write_cmos_sensor(0x3012, 0x10); 
    #ifdef BANDING50_30HZ
	OV2655_write_cmos_sensor(0x300e, 0x34); 
	#else
    if (OV2655_Banding_setting == AE_FLICKER_MODE_50HZ)
       OV2655_write_cmos_sensor(0x300e, 0x36); 
    else
       OV2655_write_cmos_sensor(0x300e, 0x34); 
	#endif
    OV2655_write_cmos_sensor(0x3011, 0x00); 
    OV2655_write_cmos_sensor(0x3015, 0x02);
    OV2655_write_cmos_sensor(0x3016, 0x52);    
    OV2655_write_cmos_sensor(0x3023, 0x6 );
    OV2655_write_cmos_sensor(0x3026, 0x2 );
    OV2655_write_cmos_sensor(0x3027, 0x5e); 
    OV2655_write_cmos_sensor(0x302a, (OV2655_PV_PERIOD_LINE_NUMS>>8)&0x00ff);
    OV2655_write_cmos_sensor(0x302b, OV2655_PV_PERIOD_LINE_NUMS&0x00ff); 
    
    OV2655_write_cmos_sensor(0x330c, 0x00);
    OV2655_write_cmos_sensor(0x3301, 0xff);
    //OV2655_write_cmos_sensor(0x3391, 0x00);  // move to effect function

    OV2655_write_cmos_sensor(0x3069, 0x80); //84   //Jerry
    OV2655_write_cmos_sensor(0x306f, 0x14); 
    OV2655_write_cmos_sensor(0x3088, 0x3 );
    OV2655_write_cmos_sensor(0x3089, 0x20); 
    OV2655_write_cmos_sensor(0x308a, 0x2 );
    OV2655_write_cmos_sensor(0x308b, 0x58); 
    OV2655_write_cmos_sensor(0x308e, 0x0 );

    OV2655_write_cmos_sensor(0x30a1, 0x41);  //Jerry 0x01
    OV2655_write_cmos_sensor(0x30a3, 0x80);  //0x0
    OV2655_write_cmos_sensor(0x30d9, 0x95);  //0x8c
    OV2655_write_cmos_sensor(0x3302, 0x11); 
    OV2655_write_cmos_sensor(0x3317, 0x25); 
    OV2655_write_cmos_sensor(0x3318, 0x80); 
    OV2655_write_cmos_sensor(0x3319, 0x8 );
    OV2655_write_cmos_sensor(0x331d, 0x38); 

    OV2655_write_cmos_sensor(0x3373, 0x40);
    OV2655_write_cmos_sensor(0x3376, 0x04); ///0624,jerry, for preview sharp
    OV2655_write_cmos_sensor(0x3362, 0x90);///////////////////
    
    //0629, help with saturation, from OV
    //OV2655_write_cmos_sensor(0x3391,0x02);
    //OV2655_write_cmos_sensor(0x3394,0x44);
    //OV2655_write_cmos_sensor(0x3395,0x44);
    
     OV2655_write_cmos_sensor(0x300f, 0xa6); //must set, from FAE

    //===preview setting end===
    

    /* ==Camera Preview, MT6235 use 36MHz PCLK, 30fps 60Hz, 25fps in 50Hz== */

    /* after set exposure line, there should be delay for 2~4 frame time, then enable AEC */
    //Use preview_ae_stable_frame to drop frame
    //kal_sleep_task(10);

    // turn on AEC/AGC
    OV2655_set_AE_mode(OV2655_AE_ENABLE); 
//    temp_AE_reg = OV2655_read_cmos_sensor(0x3013);
//    OV2655_write_cmos_sensor(0x3013, temp_AE_reg| 0x05);

    //enable Auto WB
    OV2655_set_AWB_mode(OV2655_AWB_ENABLE); 
    //temp_AWB_reg = OV2655_read_cmos_sensor(0x3324);
    //OV2655_write_cmos_sensor(0x3324, temp_AWB_reg& ~0x40);
    spin_lock(&ov2655_yuv_drv_lock);
	OV2655_gPVmode = KAL_TRUE;
	spin_unlock(&ov2655_yuv_drv_lock); 
    

    if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        RETAILMSG(1,(TEXT("Camera Video preview\r\n")));
		spin_lock(&ov2655_yuv_drv_lock);
		OV2655_VEDIO_encode_mode = KAL_TRUE;
		spin_unlock(&ov2655_yuv_drv_lock); 
        

        iDummyPixels = 0;
        iDummyLines = 0;

        /* to fix VSYNC, to fix frame rate */
        iTemp = OV2655_read_cmos_sensor(0x3014);
        OV2655_write_cmos_sensor(0x3014, iTemp & 0xf7); //Disable night mode
        OV2655_write_cmos_sensor(0x302d, 0x00);
        OV2655_write_cmos_sensor(0x302e, 0x00);
        if (image_window->ImageTargetWidth <= OV2655_VIDEO_QCIF_WIDTH)
        {
			spin_lock(&ov2655_yuv_drv_lock);
			OV2655_iOV2655_Mode = OV2655_MODE_QCIF_VIDEO;
			spin_unlock(&ov2655_yuv_drv_lock); 
           
        }
        else
        {
            spin_lock(&ov2655_yuv_drv_lock);
			OV2655_iOV2655_Mode = OV2655_MODE_QVGA_VIDEO;
			spin_unlock(&ov2655_yuv_drv_lock); 
           
        }
        //image_window->wait_stable_frame = 3;	
    }
    else
    {
        RETAILMSG(1,(TEXT("Camera preview\r\n")));
        //sensor_config_data->frame_rate == 30
        //ISP_PREVIEW_MODE
        //4  <2> if preview of capture PICTURE

        /* preview: 30 fps with 36M PCLK */
		spin_lock(&ov2655_yuv_drv_lock);
		OV2655_VEDIO_encode_mode = KAL_FALSE;
	    spin_unlock(&ov2655_yuv_drv_lock);
       
        iDummyPixels = 0; 
        iDummyLines = 0; 
		spin_lock(&ov2655_yuv_drv_lock);
		OV2655_iOV2655_Mode = OV2655_MODE_PREVIEW;
		spin_unlock(&ov2655_yuv_drv_lock);

      
        // Set for dynamic sensor delay. 2009-09-09
        //image_window->wait_stable_frame = 3;	
    }
    
    //4 <3> set mirror and flip
    iTemp = OV2655_read_cmos_sensor(0x307c) & 0xfc;

    switch (sensor_config_data->SensorImageMirror) {
    case IMAGE_NORMAL:
        iStartX = 13;
        iStartY = 1;
        //normal 0x3090=0x33 0x307c=10
        OV2655_write_cmos_sensor(0x3090,0x33);     
        OV2655_write_cmos_sensor(0x307c, 0x10);  
        break;
    case IMAGE_H_MIRROR:
        iStartX = 1;
        iStartY = 1;
        OV2655_write_cmos_sensor(0x3090,0x33);     
        OV2655_write_cmos_sensor(0x307c, 0x13);  
        break;
    case IMAGE_V_MIRROR:
        iStartX = 1;
        iStartY = 1;
        OV2655_write_cmos_sensor(0x3090,0x3b);     
        OV2655_write_cmos_sensor(0x307c, 0x12);  
        break;

    case IMAGE_HV_MIRROR:
        iStartX = 1;
        iStartY = 1;
        iTemp |= 0x03;
        //mirror and flip  0x3090=0x3b 0x307c=13
        OV2655_write_cmos_sensor(0x3090,0x3b);        
        OV2655_write_cmos_sensor(0x307c, 0x13); 
        break;

    default:
        ASSERT(0);
    }

    //4 <6> set dummy
    spin_lock(&ov2655_yuv_drv_lock);
	OV2655_PV_Dummy_Pixels = iDummyPixels;
	spin_unlock(&ov2655_yuv_drv_lock);
    
    OV2655_set_dummy(iDummyPixels, iDummyLines);


    //4 <7> set shutter
    image_window->GrabStartX = iStartX;
    image_window->GrabStartY = iStartY;
    image_window->ExposureWindowWidth = OV2655_IMAGE_SENSOR_PV_WIDTH - iStartX -2;
    image_window->ExposureWindowHeight = OV2655_IMAGE_SENSOR_PV_HEIGHT- iStartY -2;
    spin_lock(&ov2655_yuv_drv_lock);
	OV2655_DELAY_AFTER_PREVIEW = 1;
	spin_unlock(&ov2655_yuv_drv_lock);
    

	// copy sensor_config_data
	memcpy(&OV2655SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
  	return ERROR_NONE;
}	/* OV2655Preview() */

UINT32 OV2655Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    volatile kal_uint32 shutter = OV2655_exposure_lines, temp_reg;
	kal_uint16 temp_exposure_line_h,temp_exposure_line_l,temp_extra_exposure_line_h,temp_extra_exposure_line_l;
    kal_uint8 temp_AE_reg, temp;
    kal_uint16 AE_setting_delay = 0;
	spin_lock(&ov2655_yuv_drv_lock);temp_extra_exposure_line_l
	OV2655_sensor_cap_state = KAL_TRUE;
	spin_unlock(&ov2655_yuv_drv_lock);
 
    temp_reg = OV2655_read_cmos_sensor(0x3014);
    OV2655_write_cmos_sensor(0x3014, temp_reg & 0xf7); //Disable night mode
        
    // turn off AEC/AGC
    OV2655_set_AE_mode(KAL_FALSE);
    //temp_AE_reg = OV2655_read_cmos_sensor(0x3013);
    //OV2655_write_cmos_sensor(0x3013, (temp_AE_reg&(~0x05)) );        

    OV2655_set_AWB_mode(KAL_FALSE); 
    temp_exposure_line_h= OV2655_read_cmos_sensor(0x3002);
    temp_exposure_line_l= OV2655_read_cmos_sensor(0x3003);
    temp_extra_exposure_line_h= OV2655_read_cmos_sensor(0x302D);
    temp_extra_exposure_line_l= OV2655_read_cmos_sensor(0x302E);
	spin_lock(&ov2655_yuv_drv_lock);
	OV2655_exposure_line_h = temp_exposure_line_h;
	OV2655_exposure_line_l = temp_exposure_line_l;
	OV2655_extra_exposure_line_h = temp_extra_exposure_line_h;
	OV2655_extra_exposure_line_l = temp_extra_exposure_line_l;
	spin_unlock(&ov2655_yuv_drv_lock);

    shutter = OV2655_read_shutter();

    if ((image_window->ImageTargetWidth<=OV2655_IMAGE_SENSOR_PV_WIDTH)&&
        (image_window->ImageTargetHeight<=OV2655_IMAGE_SENSOR_PV_HEIGHT))
    {    /* Less than PV Mode */

		spin_lock(&ov2655_yuv_drv_lock);
		OV2655_gPVmode=KAL_TRUE;

        OV2655_dummy_pixels = 0;
        OV2655_dummy_lines = 0;

        OV2655_capture_pclk_in_M = OV2655_preview_pclk_in_M;   //Don't change the clk
		spin_unlock(&ov2655_yuv_drv_lock);

        shutter = (shutter*OV2655_capture_pclk_in_M)/OV2655_preview_pclk_in_M;
        shutter = (shutter*OV2655_PV_PERIOD_PIXEL_NUMS)/(OV2655_PV_PERIOD_PIXEL_NUMS+OV2655_dummy_pixels/2);
        // set dummy
        OV2655_set_dummy(OV2655_dummy_pixels, OV2655_dummy_lines);
        if (shutter < 1) 
        {
            shutter = 1;
        }
        // set shutter OVT
        OV2655_write_shutter(shutter);
        
        image_window->GrabStartX = 1;
        image_window->GrabStartY = 1;
        image_window->ExposureWindowWidth= OV2655_IMAGE_SENSOR_PV_WIDTH - 3;
        image_window->ExposureWindowHeight = OV2655_IMAGE_SENSOR_PV_HEIGHT - 3;

    }
    else 
    {    
    
    	 /* 2M FULL Mode */
        image_window->GrabStartX=1;
        image_window->GrabStartY=6;
        image_window->ExposureWindowWidth=OV2655_IMAGE_SENSOR_FULL_WIDTH - image_window->GrabStartX - 2;
        image_window->ExposureWindowHeight=OV2655_IMAGE_SENSOR_FULL_HEIGHT -image_window->GrabStartY - 2;    	 
        //calculator auto uv
        OV2655_write_cmos_sensor(0x330c,0x3e);
        temp = OV2655_read_cmos_sensor(0x330f);
        OV2655_write_cmos_sensor(0x3301,0xbf);//turn off auto uv
        // temp = (temp&0x1f + 0x01)<<1;
        temp = temp&0x1f;
        temp = temp + 0x01;
        temp = 2*temp;
        OV2655_write_cmos_sensor(0x3391, (OV2655_read_cmos_sensor(0x3391) | 0x02));  //  enable Saturation
        OV2655_write_cmos_sensor(0x3394,temp);
        OV2655_write_cmos_sensor(0x3395,temp);
        
        // 1600x1200
        OV2655_write_cmos_sensor(0x3010,0x81); 
        OV2655_write_cmos_sensor(0x3012,0x0 );
        //OV2655_write_cmos_sensor(0x3014,0x84); 
        OV2655_write_cmos_sensor(0x3015,0x02);
        OV2655_write_cmos_sensor(0x3016,0xc2); 
        OV2655_write_cmos_sensor(0x3023,0xc );
        OV2655_write_cmos_sensor(0x3026,0x4 );
        OV2655_write_cmos_sensor(0x3027,0xbc); 
        OV2655_write_cmos_sensor(0x302a, (OV2655_FULL_PERIOD_LINE_NUMS>>8)&0x00ff);
        OV2655_write_cmos_sensor(0x302b, OV2655_FULL_PERIOD_LINE_NUMS&0x00ff); 

        OV2655_write_cmos_sensor(0x3069,0x80); 
        OV2655_write_cmos_sensor(0x306f,0x54); 
        OV2655_write_cmos_sensor(0x3088,0x6 );
        OV2655_write_cmos_sensor(0x3089,0x40); 
        OV2655_write_cmos_sensor(0x308a,0x4 );
        OV2655_write_cmos_sensor(0x308b,0xb0); 
        OV2655_write_cmos_sensor(0x308e,0x64); 
        //OV2655_write_cmos_sensor(0x30a1,0x01);    //This REG must equal to Preview
        //OV2655_write_cmos_sensor(0x30a3,0x00); 
        OV2655_write_cmos_sensor(0x30d9,0x95); 
        OV2655_write_cmos_sensor(0x3302,0x1 );
        OV2655_write_cmos_sensor(0x3317,0x4b); 
        OV2655_write_cmos_sensor(0x3318,0x0 );
        OV2655_write_cmos_sensor(0x3319,0x4c); 
        OV2655_write_cmos_sensor(0x331d,0x6c); 
        OV2655_write_cmos_sensor(0x3362,0x80);  //full size shading  
        OV2655_write_cmos_sensor(0x3373,0x50);  
        OV2655_write_cmos_sensor(0x3376,0x03); 
		spin_lock(&ov2655_yuv_drv_lock);
		OV2655_gPVmode = KAL_FALSE;		
		spin_unlock(&ov2655_yuv_drv_lock);
              
        if ((image_window->ImageTargetWidth<=OV2655_IMAGE_SENSOR_FULL_WIDTH)&&
        (image_window->ImageTargetHeight<=OV2655_IMAGE_SENSOR_FULL_HEIGHT))
        {     
	        if (zoom_factor  <  3) 
	        {  
		     //48Mhz Full size Capture CLK
		     OV2655_write_cmos_sensor(0x3011, 0x00); 
		     OV2655_write_cmos_sensor(0x300e, 0x38);  
			 spin_lock(&ov2655_yuv_drv_lock);
			 OV2655_sensor_pclk = 520;

       	     OV2655_dummy_pixels=0;  /*If Capture fail, you can add this dummy*/
	         OV2655_dummy_lines=0;

	         OV2655_capture_pclk_in_M = 520;   //Don't change the clk
			 spin_unlock(&ov2655_yuv_drv_lock);
	            //10 fps 1 frame = 100ms = 30
	            AE_setting_delay = 26; 	        
	        }
	        else 
	        {
			     //48Mhz Full size Capture CLK/2
	       	     OV2655_write_cmos_sensor(0x3011, 0x01);
			     OV2655_write_cmos_sensor(0x300e, 0x38);  
				 spin_lock(&ov2655_yuv_drv_lock);

	            OV2655_sensor_pclk = 260;
	            
	            OV2655_dummy_pixels=0;  /*If Capture fail, you can add this dummy*/
	            OV2655_dummy_lines=0;

	            OV2655_capture_pclk_in_M = 260;   //Don't change the clk
				spin_unlock(&ov2655_yuv_drv_lock);

	            //9.3 fps, 1 frame = 200ms
	            AE_setting_delay = 30;

	        }                  
        }
        else//Interpolate to 3M
        {
  	        if (image_window->ZoomFactor >= 340)
	        {  
	        
		     //48Mhz Full size Capture CLK/4
       	     OV2655_write_cmos_sensor(0x3011, 0x03);
		     OV2655_write_cmos_sensor(0x300e, 0x38);  
		     spin_lock(&ov2655_yuv_drv_lock);  
			OV2655_sensor_pclk = 130;

			OV2655_dummy_pixels=0;  /*If Capture fail, you can add this dummy*/
			OV2655_dummy_lines=0;

			OV2655_capture_pclk_in_M = 130;   //Don't change the clk
	         spin_unlock(&ov2655_yuv_drv_lock); 
	            //9.3 fps, 1 frame = 200ms
	            AE_setting_delay = 34;
	        }
	        else if (image_window->ZoomFactor >= 240) 
	        {  
	        
		     //48Mhz Full size Capture CLK/2
       	     OV2655_write_cmos_sensor(0x3011, 0x01); 
		     OV2655_write_cmos_sensor(0x300e, 0x38);  
			 spin_lock(&ov2655_yuv_drv_lock); 

	            OV2655_sensor_pclk = 260;

       	     OV2655_dummy_pixels=0;  /*If Capture fail, you can add this dummy*/
	            OV2655_dummy_lines=0;

	            OV2655_capture_pclk_in_M = 260;   //Don't change the clk
	            spin_unlock(&ov2655_yuv_drv_lock); 
	            //9.3 fps, 1 frame = 200ms
	            AE_setting_delay = 30;
	        }
	        else 
	        {
	        
		     //48Mhz Full size Capture CLK
		     OV2655_write_cmos_sensor(0x3011, 0x00); 
		     OV2655_write_cmos_sensor(0x300e, 0x38);
		     spin_lock(&ov2655_yuv_drv_lock); 
            OV2655_sensor_pclk = 520;
            OV2655_dummy_pixels=0;  /*If Capture fail, you can add this dummy*/
            OV2655_dummy_lines=0;

            OV2655_capture_pclk_in_M = 520;   //Don't change the clk
			spin_unlock(&ov2655_yuv_drv_lock);

	            //10 fps 1 frame = 100ms = 30
	            AE_setting_delay = 26; 
	        }                  
        }
        spin_lock(&ov2655_yuv_drv_lock); 
        OV2655_Capture_Dummy_Pixels = OV2655_dummy_pixels ;
        OV2655_Capture_Dummy_Lines = OV2655_dummy_lines;
		spin_unlock(&ov2655_yuv_drv_lock);
        //Jerry It need to change gain to shutter
        OV2655_Computer_AECAGC(OV2655_preview_pclk_in_M, OV2655_capture_pclk_in_M);
        shutter = OV2655_Capture_Shutter + OV2655_Capture_Extra_Lines;

        // set dummy
        OV2655_set_dummy(OV2655_dummy_pixels, OV2655_dummy_lines);
       
        if (shutter < 1) 
        {
            shutter = 1;
        }
        if (OV2655_AE_ENABLE == KAL_TRUE)
        {
            spin_lock(&ov2655_yuv_drv_lock); 
            Capture_Shutter = shutter; 
            Capture_Gain = OV2655_Capture_Gain16;
			spin_unlock(&ov2655_yuv_drv_lock);
        }
        
        // set shutter OVT
        OV2655_write_shutter(Capture_Shutter);
		/*
        if(OV2655_Capture_Gain16>62)
            OV2655_write_OV2655_gain(16); 
        else
            OV2655_write_OV2655_gain((OV2655_Capture_Gain16+5)); 
        */
        //kal_sleep_task(23);  //delay 1frame
       // Sleep(95);
        OV2655_write_OV2655_gain(Capture_Gain); 
    }

    printk("Capture Shutter = %d\, Capture Gain = %d\n", Capture_Shutter, Capture_Gain); 
    // AEC/AGC/AWB will be enable in preview and param_wb function
    /* total delay 4 frame for AE stable */
	spin_lock(&ov2655_yuv_drv_lock); 

    OV2655_DELAY_AFTER_PREVIEW = 2;
	spin_unlock(&ov2655_yuv_drv_lock);

	// copy sensor_config_data
    memcpy(&OV2655SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* OV2655Capture() */

UINT32 OV2655GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=OV2655_IMAGE_SENSOR_FULL_WIDTH - 2 * IMAGE_SENSOR_START_GRAB_X - 8;  //modify by yanxu
	pSensorResolution->SensorFullHeight=OV2655_IMAGE_SENSOR_FULL_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y -6;
	pSensorResolution->SensorPreviewWidth=OV2655_IMAGE_SENSOR_PV_WIDTH - 2 * IMAGE_SENSOR_START_GRAB_X -8;
	pSensorResolution->SensorPreviewHeight=OV2655_IMAGE_SENSOR_PV_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y -6;

	return ERROR_NONE;
}	/* OV2655GetResolution() */

UINT32 OV2655GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX=OV2655_IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=OV2655_IMAGE_SENSOR_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX=OV2655_IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=OV2655_IMAGE_SENSOR_FULL_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	/*??? */
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 1;
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;
	pSensorInfo->CaptureDelayFrame = 1; 
	pSensorInfo->PreviewDelayFrame = 0; 
	pSensorInfo->VideoDelayFrame = 4; 		
	pSensorInfo->SensorMasterClockSwitch = 0; 
       pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;   		

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
                     pSensorInfo->SensorGrabStartX = 4; 
                     pSensorInfo->SensorGrabStartY = 2;             
			
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
                     pSensorInfo->SensorGrabStartX = 4; 
                     pSensorInfo->SensorGrabStartY = 2;             
			
		break;
		default:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
                     pSensorInfo->SensorGrabStartX = 4; 
                     pSensorInfo->SensorGrabStartY = 2;             
			
		break;
	}
	spin_lock(&ov2655_yuv_drv_lock); 
	OV2655_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	spin_unlock(&ov2655_yuv_drv_lock);
	
	memcpy(pSensorConfigData, &OV2655SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* OV2655GetInfo() */


UINT32 OV2655Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			OV2655Preview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			OV2655Capture(pImageWindow, pSensorConfigData);
		break;
		default:
		    break; 
	}
	return TRUE;
}	/* OV2655Control() */

/* [TC] YUV sensor */	
#if WINMO_USE
void OV2655Query(PMSDK_FEATURE_INFO_STRUCT pSensorFeatureInfo)
{
	MSDK_FEATURE_TYPE_RANGE_STRUCT *pFeatureRange;
	MSDK_FEATURE_TYPE_MULTI_SELECTION_STRUCT *pFeatureMultiSelection;
	switch (pSensorFeatureInfo->FeatureId)
	{
		case ISP_FEATURE_DSC_MODE:
			pSensorFeatureInfo->FeatureType = MSDK_FEATURE_TYPE_MULTI_SELECTION;
			pSensorFeatureInfo->FeatureSupported = (UINT8)(MSDK_SET_GET_FEATURE_SUPPORTED|MSDK_QUERY_CAMERA_SUPPORTED);
			pFeatureMultiSelection = (PMSDK_FEATURE_TYPE_MULTI_SELECTION_STRUCT)(&pSensorFeatureInfo->FeatureInformation.FeatureMultiSelection);
			pFeatureMultiSelection->TotalSelection = CAM_NO_OF_SCENE_MODE_MAX;
			pFeatureMultiSelection->DefaultSelection = CAM_AUTO_DSC_MODE;
			pFeatureMultiSelection->SupportedSelection = 
				(CAMERA_FEATURE_SUPPORT(CAM_AUTO_DSC_MODE)|
				CAMERA_FEATURE_SUPPORT(CAM_NIGHTSCENE_MODE));			
		break;
		case ISP_FEATURE_WHITEBALANCE:
			pSensorFeatureInfo->FeatureType = MSDK_FEATURE_TYPE_MULTI_SELECTION;
			pSensorFeatureInfo->FeatureSupported = (UINT8)(MSDK_SET_GET_FEATURE_SUPPORTED|MSDK_QUERY_CAMERA_VIDEO_SUPPORTED);
			pFeatureMultiSelection = (PMSDK_FEATURE_TYPE_MULTI_SELECTION_STRUCT)(&pSensorFeatureInfo->FeatureInformation.FeatureMultiSelection);
			pFeatureMultiSelection->TotalSelection = CAM_NO_OF_WB;
			pFeatureMultiSelection->DefaultSelection = CAM_WB_AUTO;
			pFeatureMultiSelection->SupportedSelection = 
				(CAMERA_FEATURE_SUPPORT(CAM_WB_AUTO)|
				CAMERA_FEATURE_SUPPORT(CAM_WB_CLOUD)|
				CAMERA_FEATURE_SUPPORT(CAM_WB_DAYLIGHT)|
				CAMERA_FEATURE_SUPPORT(CAM_WB_INCANDESCENCE)|
				CAMERA_FEATURE_SUPPORT(CAM_WB_TUNGSTEN)|
				CAMERA_FEATURE_SUPPORT(CAM_WB_FLUORESCENT));
		break;
		case ISP_FEATURE_IMAGE_EFFECT:
			pSensorFeatureInfo->FeatureType = MSDK_FEATURE_TYPE_MULTI_SELECTION;
			pSensorFeatureInfo->FeatureSupported = (UINT8)(MSDK_SET_GET_FEATURE_SUPPORTED|MSDK_QUERY_CAMERA_VIDEO_SUPPORTED);
			pFeatureMultiSelection = (PMSDK_FEATURE_TYPE_MULTI_SELECTION_STRUCT)(&pSensorFeatureInfo->FeatureInformation.FeatureMultiSelection);
			pFeatureMultiSelection->TotalSelection = CAM_NO_OF_EFFECT_ENC;
			pFeatureMultiSelection->DefaultSelection = CAM_EFFECT_ENC_NORMAL;
			pFeatureMultiSelection->SupportedSelection = 
				(CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_NORMAL)|
				CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_GRAYSCALE)|
				CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_COLORINV)|
				CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_SEPIAGREEN)|
				CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_SEPIABLUE)|
				CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_SEPIA));	
		break;
		case ISP_FEATURE_AE_METERING_MODE:
			pSensorFeatureInfo->FeatureSupported = MSDK_FEATURE_NOT_SUPPORTED;
		break;
		case ISP_FEATURE_BRIGHTNESS:
			pSensorFeatureInfo->FeatureType = MSDK_FEATURE_TYPE_RANGE;
			pSensorFeatureInfo->FeatureSupported = (UINT8)(MSDK_SET_GET_FEATURE_SUPPORTED|MSDK_QUERY_CAMERA_VIDEO_SUPPORTED);
			pFeatureRange = (PMSDK_FEATURE_TYPE_RANGE_STRUCT)(&pSensorFeatureInfo->FeatureInformation.FeatureRange);
			pFeatureRange->MinValue = CAM_EV_NEG_4_3;
			pFeatureRange->MaxValue = CAM_EV_POS_4_3;
			pFeatureRange->StepValue = CAMERA_FEATURE_ID_EV_STEP;
			pFeatureRange->DefaultValue = CAM_EV_ZERO;
		break;
		case ISP_FEATURE_BANDING_FREQ:
			pSensorFeatureInfo->FeatureType = MSDK_FEATURE_TYPE_MULTI_SELECTION;
			pSensorFeatureInfo->FeatureSupported = (UINT8)(MSDK_SET_GET_FEATURE_SUPPORTED|MSDK_QUERY_CAMERA_VIDEO_SUPPORTED);
			pFeatureMultiSelection = (PMSDK_FEATURE_TYPE_MULTI_SELECTION_STRUCT)(&pSensorFeatureInfo->FeatureInformation.FeatureMultiSelection);
			pFeatureMultiSelection->TotalSelection = CAM_NO_OF_BANDING;
			pFeatureMultiSelection->DefaultSelection = CAM_BANDING_50HZ;
			pFeatureMultiSelection->SupportedSelection = 
				(CAMERA_FEATURE_SUPPORT(CAM_BANDING_50HZ)|
				CAMERA_FEATURE_SUPPORT(CAM_BANDING_60HZ));
		break;
		case ISP_FEATURE_AF_OPERATION:
			pSensorFeatureInfo->FeatureSupported = MSDK_FEATURE_NOT_SUPPORTED;
		break;
		case ISP_FEATURE_AF_RANGE_CONTROL:
			pSensorFeatureInfo->FeatureSupported = MSDK_FEATURE_NOT_SUPPORTED;
		break;
		case ISP_FEATURE_FLASH:
			pSensorFeatureInfo->FeatureSupported = MSDK_FEATURE_NOT_SUPPORTED;			
		break;
		case ISP_FEATURE_VIDEO_SCENE_MODE:
			pSensorFeatureInfo->FeatureType = MSDK_FEATURE_TYPE_MULTI_SELECTION;
			pSensorFeatureInfo->FeatureSupported = (UINT8)(MSDK_SET_GET_FEATURE_SUPPORTED|MSDK_QUERY_VIDEO_SUPPORTED);
			pFeatureMultiSelection = (PMSDK_FEATURE_TYPE_MULTI_SELECTION_STRUCT)(&pSensorFeatureInfo->FeatureInformation.FeatureMultiSelection);
			pFeatureMultiSelection->TotalSelection = CAM_NO_OF_SCENE_MODE_MAX;
			pFeatureMultiSelection->DefaultSelection = CAM_VIDEO_AUTO_MODE;
			pFeatureMultiSelection->SupportedSelection = 
				(CAMERA_FEATURE_SUPPORT(CAM_VIDEO_AUTO_MODE)|
				CAMERA_FEATURE_SUPPORT(CAM_VIDEO_NIGHT_MODE));
		break;
		case ISP_FEATURE_ISO:
			pSensorFeatureInfo->FeatureSupported = MSDK_FEATURE_NOT_SUPPORTED;			
		break;
		default:
			pSensorFeatureInfo->FeatureSupported = MSDK_FEATURE_NOT_SUPPORTED;			
		break;
	}
}
#endif 

BOOL OV2655_set_param_wb(UINT16 para)
{
    kal_uint8  temp_reg;

    temp_reg=OV2655_read_cmos_sensor(0x3306);

    switch (para)
    {
        case AWB_MODE_OFF:
			spin_lock(&ov2655_yuv_drv_lock); 
			OV2655_AWB_ENABLE = KAL_FALSE; 
			spin_unlock(&ov2655_yuv_drv_lock); 
            OV2655_set_AWB_mode(OV2655_AWB_ENABLE);
            break;                     
        case AWB_MODE_AUTO:
			spin_lock(&ov2655_yuv_drv_lock); 
            OV2655_AWB_ENABLE = KAL_TRUE;  //
            spin_unlock(&ov2655_yuv_drv_lock);
            OV2655_set_AWB_mode(OV2655_AWB_ENABLE);
            OV2655_write_cmos_sensor(0x3306, temp_reg&~0x2);   // select Auto WB
            break;

        case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
            OV2655_write_cmos_sensor(0x3306, temp_reg|0x2);  // select manual WB
            OV2655_write_cmos_sensor(0x3337, 0x68); //manual R G B
            OV2655_write_cmos_sensor(0x3338, 0x40);
            OV2655_write_cmos_sensor(0x3339, 0x4e);
            break;

        case AWB_MODE_DAYLIGHT: //sunny
            OV2655_write_cmos_sensor(0x3306, temp_reg|0x2);  // Disable AWB
            OV2655_write_cmos_sensor(0x3337, 0x5e);
            OV2655_write_cmos_sensor(0x3338, 0x40);
            OV2655_write_cmos_sensor(0x3339, 0x46);
            break;

        case AWB_MODE_INCANDESCENT: //office
            OV2655_write_cmos_sensor(0x3306, temp_reg|0x2);  // Disable AWB
            OV2655_write_cmos_sensor(0x3337, 0x5e);
            OV2655_write_cmos_sensor(0x3338, 0x40);
            OV2655_write_cmos_sensor(0x3339, 0x58);
            break;

        case AWB_MODE_TUNGSTEN: //home
            //OV2655_write_cmos_sensor(0x13, temp_reg|0x2);  // Disable AWB
            OV2655_write_cmos_sensor(0x3306, temp_reg|0x2);  // Disable AWB
            OV2655_write_cmos_sensor(0x3337, 0x54);
            OV2655_write_cmos_sensor(0x3338, 0x40);
            OV2655_write_cmos_sensor(0x3339, 0x70);
            break;

        case AWB_MODE_FLUORESCENT:
            OV2655_write_cmos_sensor(0x3306, temp_reg|0x2); // Disable AWB
            OV2655_write_cmos_sensor(0x3337, 0x65);
            OV2655_write_cmos_sensor(0x3338, 0x40);
            OV2655_write_cmos_sensor(0x3339, 0x41);
            break;
#if WINMO_USE
        case AWB_MODE_MANUAL:
            // TODO
            break;
#endif 

        default:
            return FALSE;
    }

    return TRUE;
} /* OV2655_set_param_wb */

BOOL OV2655_set_param_effect(UINT16 para)
{
   BOOL  ret = TRUE;
   //UINT8  temp_reg;
   //temp_reg=OV2655_read_cmos_sensor(0x3391);
    switch (para)
    {
        case MEFFECT_OFF:
            OV2655_write_cmos_sensor(0x3391, 0x04);
            //OV2655_write_cmos_sensor(0x3390, 0x41);
            break;

        case MEFFECT_SEPIA:
            OV2655_write_cmos_sensor(0x3391, 0x1c);
            OV2655_write_cmos_sensor(0x3396, 0x40);
            OV2655_write_cmos_sensor(0x3397, 0xa6);
            break;

        case MEFFECT_NEGATIVE:
            OV2655_write_cmos_sensor(0x3391, 0x44);
            break;

        case MEFFECT_SEPIAGREEN:
            OV2655_write_cmos_sensor(0x3391, 0x1c);
            OV2655_write_cmos_sensor(0x3396, 0x60);
            OV2655_write_cmos_sensor(0x3397, 0x60);
            break;

        case MEFFECT_SEPIABLUE:
            OV2655_write_cmos_sensor(0x3391, 0x1c);
            OV2655_write_cmos_sensor(0x3396, 0xf0);
            OV2655_write_cmos_sensor(0x3397, 0x60);
            break;
		case MEFFECT_MONO: //B&W
            OV2655_write_cmos_sensor(0x3391, 0x24);
			break;
#if WINMO_USE
        case CAM_EFFECT_ENC_GRAYINV:
        case CAM_EFFECT_ENC_COPPERCARVING:
              case CAM_EFFECT_ENC_BLUECARVING:


        case CAM_EFFECT_ENC_EMBOSSMENT:
        case CAM_EFFECT_ENC_SKETCH:
        case CAM_EFFECT_ENC_BLACKBOARD:
        case CAM_EFFECT_ENC_WHITEBOARD:
        case CAM_EFFECT_ENC_JEAN:
        case CAM_EFFECT_ENC_OIL:
#endif 
        default:
            ret = FALSE;
    }

    return ret;

} /* OV2655_set_param_effect */

BOOL OV2655_set_param_banding(UINT16 para)
{

    kal_uint8 banding;

    banding = OV2655_read_cmos_sensor(0x3014);
    switch (para)
    {
        case AE_FLICKER_MODE_50HZ:
			spin_lock(&ov2655_yuv_drv_lock); 
           OV2655_Banding_setting = AE_FLICKER_MODE_50HZ;
            spin_unlock(&ov2655_yuv_drv_lock);
           
           #ifdef BANDING50_30HZ
		   spin_lock(&ov2655_yuv_drv_lock); 
           OV2655_preview_pclk_in_M = 390;
		    spin_unlock(&ov2655_yuv_drv_lock);
           OV2655_write_cmos_sensor( 0x3014, banding|0x81 );    /* enable banding and 50 Hz */
           OV2655_write_cmos_sensor(0x301c, 0x03); 
           OV2655_write_cmos_sensor(0x3070, 0xC9); 
           #else
		   spin_lock(&ov2655_yuv_drv_lock); 
           OV2655_preview_pclk_in_M = 335;
		   spin_unlock(&ov2655_yuv_drv_lock);
           if (OV2655_VEDIO_encode_mode)
           {
               OV2655_write_cmos_sensor( 0x3014, banding|0x81 );    /* enable banding and 50 Hz */
               OV2655_write_cmos_sensor(0x301c, 0x02); 
        	 OV2655_write_cmos_sensor(0x3070, 0xB9); 
           }
           else
           {
        	  OV2655_write_cmos_sensor(0x300e, 0x36);
                OV2655_write_cmos_sensor(0x3011, 0x00);
                OV2655_write_cmos_sensor( 0x3014, banding|0x81 );    /* enable banding and 50 Hz */
        	  OV2655_write_cmos_sensor(0x301c, 0x03); 
        	  OV2655_write_cmos_sensor(0x3070, 0x9b); 
           }
           #endif
            break;

        case AE_FLICKER_MODE_60HZ:		
			spin_lock(&ov2655_yuv_drv_lock);
            OV2655_preview_pclk_in_M = 390;
            OV2655_Banding_setting = AE_FLICKER_MODE_60HZ;	
			spin_unlock(&ov2655_yuv_drv_lock);
#ifndef BANDING50_30HZ
            OV2655_write_cmos_sensor(0x3011, 0x00);
            OV2655_write_cmos_sensor(0x300e, 0x34);     
#endif
            OV2655_write_cmos_sensor( 0x3014, (banding & ~0x80)|0x01 );    /* enable banding and 60 Hz */
            OV2655_write_cmos_sensor(0x301d, 0x04); 
            OV2655_write_cmos_sensor(0x3072, 0xa7); 
            break;

          default:
              return FALSE;
    }

    return TRUE;
} /* OV2655_set_param_banding */

BOOL OV2655_set_param_exposure(UINT16 para)
{
    kal_uint8  temp_reg;

    temp_reg=OV2655_read_cmos_sensor(0x3391);

    switch (para)
    {
        case AE_EV_COMP_n13:
            OV2655_write_cmos_sensor(0x3391, temp_reg|0x4);
            OV2655_write_cmos_sensor(0x3390, 0x49);
            OV2655_write_cmos_sensor(0x339a, 0x30);
            break;

        case AE_EV_COMP_n10:    
            OV2655_write_cmos_sensor(0x3391, temp_reg|0x4);
            OV2655_write_cmos_sensor(0x3390, 0x49);
            OV2655_write_cmos_sensor(0x339a, 0x28);/* for difference */
            break;

        case AE_EV_COMP_n07:
            OV2655_write_cmos_sensor(0x3391, temp_reg|0x4);
            OV2655_write_cmos_sensor(0x3390, 0x49);
            OV2655_write_cmos_sensor(0x339a, 0x20);
            break;

        case AE_EV_COMP_n03:
            OV2655_write_cmos_sensor(0x3391, temp_reg|0x4);
            OV2655_write_cmos_sensor(0x3390, 0x49);
            OV2655_write_cmos_sensor(0x339a, 0x10);
            break;

        case AE_EV_COMP_00:
            OV2655_write_cmos_sensor(0x3391, temp_reg|0x4);
            OV2655_write_cmos_sensor(0x3390, 0x41);
            OV2655_write_cmos_sensor(0x339a, 0x00);
            break;

        case AE_EV_COMP_03:
            OV2655_write_cmos_sensor(0x3391, temp_reg|0x4);
            OV2655_write_cmos_sensor(0x3390, 0x41);
            OV2655_write_cmos_sensor(0x339a, 0x10);
            break;

        case AE_EV_COMP_07:
            OV2655_write_cmos_sensor(0x3391, temp_reg|0x4);
            OV2655_write_cmos_sensor(0x3390, 0x41);
            OV2655_write_cmos_sensor(0x339a, 0x20);
            break;

        case AE_EV_COMP_10:
            OV2655_write_cmos_sensor(0x3391, temp_reg|0x4);
            OV2655_write_cmos_sensor(0x3390, 0x41);
            OV2655_write_cmos_sensor(0x339a, 0x28);/* for difference */
            break;

        case AE_EV_COMP_13:
            OV2655_write_cmos_sensor(0x3391, temp_reg|0x4);
            OV2655_write_cmos_sensor(0x3390, 0x41);
            OV2655_write_cmos_sensor(0x339a, 0x30);
            break;

        default:
            return FALSE;
    }

    return TRUE;
} /* OV2655_set_param_exposure */



UINT32 OV2655YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
//   if( OV2655_sensor_cap_state == KAL_TRUE)
//	   return TRUE;

	switch (iCmd) {
	case FID_SCENE_MODE:	    
//	    printk("Set Scene Mode:%d\n", iPara); 
	    if (iPara == SCENE_MODE_OFF)
	    {
	        OV2655_night_mode(0); 
	    }
	    else if (iPara == SCENE_MODE_NIGHTSCENE)
	    {
               OV2655_night_mode(1); 
	    }	    
	    break; 	    
	case FID_AWB_MODE:
//	    printk("Set AWB Mode:%d\n", iPara); 	    
           OV2655_set_param_wb(iPara);
	break;
	case FID_COLOR_EFFECT:
//	    printk("Set Color Effect:%d\n", iPara); 	    	    
           OV2655_set_param_effect(iPara);
	break;
	case FID_AE_EV:
#if WINMO_USE	    
	case ISP_FEATURE_EXPOSURE:
#endif 	    
//           printk("Set EV:%d\n", iPara); 	    	    
           OV2655_set_param_exposure(iPara);
	break;
	case FID_AE_FLICKER:
//           printk("Set Flicker:%d\n", iPara); 	    	    	    
           OV2655_set_param_banding(iPara);
	break;
        case FID_AE_SCENE_MODE: 
            if (iPara == AE_MODE_OFF) {
				spin_lock(&ov2655_yuv_drv_lock);
	            OV2655_AE_ENABLE = KAL_FALSE; 
				spin_unlock(&ov2655_yuv_drv_lock);
                
            }
            else {
				spin_lock(&ov2655_yuv_drv_lock);
                OV2655_AE_ENABLE = KAL_TRUE; 
				spin_unlock(&ov2655_yuv_drv_lock);
	    }
            OV2655_set_AE_mode(OV2655_AE_ENABLE);
            break; 
	case FID_ZOOM_FACTOR:
		spin_lock(&ov2655_yuv_drv_lock);
	    zoom_factor = iPara; 
		spin_unlock(&ov2655_yuv_drv_lock);
        break; 
	default:
	break;
	}
	return TRUE;
}   /* OV2655YUVSensorSetting */

UINT32 OV2655YUVSetVideoMode(UINT16 u2FrameRate)
{
    kal_uint8 iTemp;
    /* to fix VSYNC, to fix frame rate */
    //printk("Set YUV Video Mode \n");  
    iTemp = OV2655_read_cmos_sensor(0x3014);
    OV2655_write_cmos_sensor(0x3014, iTemp & 0xf7); //Disable night mode

    if (u2FrameRate == 30)
    {
        OV2655_write_cmos_sensor(0x302d, 0x00);
        OV2655_write_cmos_sensor(0x302e, 0x00);
    }
    else if (u2FrameRate == 15)       
    {
        OV2655_write_cmos_sensor(0x300e, 0x34);
        OV2655_write_cmos_sensor(0x302A, OV2655_VIDEO_15FPS_FRAME_LENGTH>>8);  /*  15fps*/
        OV2655_write_cmos_sensor(0x302B, OV2655_VIDEO_15FPS_FRAME_LENGTH&0xFF);
                
        // clear extra exposure line
        OV2655_write_cmos_sensor(0x302d, 0x00);
        OV2655_write_cmos_sensor(0x302e, 0x00);   
    }
    else 
    {
        printk("Wrong frame rate setting \n");
    }
	spin_lock(&ov2655_yuv_drv_lock);
	OV2655_VEDIO_encode_mode = KAL_TRUE;
    spin_unlock(&ov2655_yuv_drv_lock);
     
        
    return TRUE;
}

UINT32 OV2655FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

#if WINMO_USE	
	PMSDK_FEATURE_INFO_STRUCT pSensorFeatureInfo=(PMSDK_FEATURE_INFO_STRUCT) pFeaturePara;
#endif 

	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=OV2655_IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=OV2655_IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
			*pFeatureReturnPara16++=OV2655_PV_PERIOD_PIXEL_NUMS+OV2655_PV_dummy_pixels;
			*pFeatureReturnPara16=OV2655_PV_PERIOD_LINE_NUMS+OV2655_PV_dummy_lines;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = OV2655_sensor_pclk/10;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			OV2655_night_mode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			spin_lock(&ov2655_yuv_drv_lock);
			OV2655_isp_master_clock=*pFeatureData32;
    		spin_unlock(&ov2655_yuv_drv_lock);
			
		break;
		case SENSOR_FEATURE_SET_REGISTER:
			OV2655_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = OV2655_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &OV2655SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
		case SENSOR_FEATURE_GET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:

		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
		case SENSOR_FEATURE_GET_GROUP_INFO:
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
		break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
                        *pFeatureReturnPara32++=0;
                        *pFeatureParaLen=4;	    
		    break; 
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			 OV2655_GetSensorID(pFeatureData32);
			 break;
		case SENSOR_FEATURE_SET_YUV_CMD:
//		       printk("OV2655 YUV sensor Setting:%d, %d \n", *pFeatureData32,  *(pFeatureData32+1));
			OV2655YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		break;
#if WINMO_USE		    		
		case SENSOR_FEATURE_QUERY:
			OV2655Query(pSensorFeatureInfo);
			*pFeatureParaLen = sizeof(MSDK_FEATURE_INFO_STRUCT);
		break;		
		case SENSOR_FEATURE_SET_YUV_CAPTURE_RAW_SUPPORT:
			/* update yuv capture raw support flag by *pFeatureData16 */
		break;		
#endif 			
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		       OV2655YUVSetVideoMode(*pFeatureData16);
		       break; 
		default:
			break;			
	}
	return ERROR_NONE;
}	/* OV2655FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncOV2655=
{
	OV2655Open,
	OV2655GetInfo,
	OV2655GetResolution,
	OV2655FeatureControl,
	OV2655Control,
	OV2655Close
};

UINT32 OV2655_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncOV2655;

	return ERROR_NONE;
}	/* SensorInit() */



