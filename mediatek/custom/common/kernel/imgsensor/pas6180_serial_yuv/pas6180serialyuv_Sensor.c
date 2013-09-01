/*****************************************************************************
 *
 * Filename:
 * ---------
 *   Sensor.c
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
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
#include <asm/io.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "pas6180serialyuv_Sensor.h"
#include "pas6180serialyuv_Camera_Sensor_para.h"
#include "pas6180serialyuv_CameraCustomized.h"


kal_uint16 PAS6180SERIALYUV_sensor_id=0;
kal_bool  PAS6180SERIALYUV_MPEG4_encode_mode = KAL_FALSE;


static kal_uint16 PAS6180SERIALYUV_dummy_pixels=0, PAS6180SERIALYUV_dummy_lines=0;
kal_uint16 PAS6180SERIALYUV_PV_dummy_pixels=0,PAS6180SERIALYUV_PV_dummy_lines=0;


kal_uint16 PAS6180SERIALYUV_pv_exposure_lines=0x100, PAS6180SERIALYUV_g_iBackupExtraExp = 0, PAS6180SERIALYUV_extra_exposure_lines = 0;


UINT8 PAS6180SERIALYUVPixelClockDivider=0;

kal_uint32 PAS6180SERIALYUV_isp_master_clock;

MSDK_SENSOR_CONFIG_STRUCT PAS6180SERIALYUVSensorConfigData;

/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT PAS6180SERIALYUVSensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT PAS6180SERIALYUVSensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/


////////////////////////////////////////////////////////////////


typedef enum
{
  PAS6180_MODE_PREVIEW,  //240*320
  PAS6180_MODE_CAPTURE   //240*320
} PAS6180_MODE;
PAS6180_MODE g_iPAS6180SERIALYUV_Mode=PAS6180_MODE_PREVIEW;


#define LOG_TAG "[pas6180serialyuv]"
#define SENSORDB(fmt, arg...) printk( LOG_TAG  fmt, ##arg)
#define RETAILMSG(x,...)
#define TEXT


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

/*************************************************************************
* FUNCTION
*    PAS6180SERIALYUV_read_cmos_sensor
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
static kal_uint8 PAS6180SERIALYUV_read_cmos_sensor(kal_uint8 addr)
{
  kal_uint8 in_buff[1] = {0xFF};
  kal_uint8 out_buff[1];
  
  out_buff[0] = addr;

    if (0 != iReadRegI2C((u8*)out_buff , (u16) sizeof(out_buff), (u8*)in_buff, (u16) sizeof(in_buff), PAS6180_READ_ID)) {
        SENSORDB("ERROR: PAS6180SERIALYUV_read_cmos_sensor \n");
    }

  return in_buff[0];
}

/*************************************************************************
* FUNCTION
*    PAS6180SERIALYUV_write_cmos_sensor
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
static void PAS6180SERIALYUV_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    kal_uint8 out_buff[2];

    out_buff[0] = addr;
    out_buff[1] = para;

    if (0 != iWriteRegI2C((u8*)out_buff , (u16)sizeof(out_buff), PAS6180_WRITE_ID)) {
        SENSORDB("ERROR: OV7675_write_cmos_sensor \n");
    }

}

#define Sleep(ms) mdelay(ms)


/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_write_Shutter
*
* DESCRIPTION
*	This function used to write the shutter.
*
* PARAMETERS
*	1. kal_uint32 : The shutter want to apply to sensor.
*
* RETURNS
*	None
*
*************************************************************************/
static void PAS6180SERIALYUV_write_Shutter(kal_uint32 shutter)
{  
    
    kal_uint16 lpf_value, ny_value, ne_value;
    kal_uint16 line_size, Hsize, Nov_Size;
    kal_uint32 shutter_value;
    	
    PAS6180SERIALYUV_write_cmos_sensor(0xef,0x00);
    lpf_value = ((PAS6180SERIALYUV_read_cmos_sensor(0x14)&0x07)<<8)|PAS6180SERIALYUV_read_cmos_sensor(0x13);
    PAS6180SERIALYUV_write_cmos_sensor(0xef,0x01);
    Hsize = ((PAS6180SERIALYUV_read_cmos_sensor(0x1C)&0x03)<<8)|PAS6180SERIALYUV_read_cmos_sensor(0x1B);
    Nov_Size = ((PAS6180SERIALYUV_read_cmos_sensor(0x80)&0x0F)<<8)|PAS6180SERIALYUV_read_cmos_sensor(0x03);
    line_size = Hsize + Nov_Size + 1;
    
    if( shutter > line_size ) 
    {
    	shutter_value = shutter / line_size;
    	shutter_value++;
    }
    else
    	shutter_value = 1;
    
    if(shutter_value >= lpf_value)
    	lpf_value = shutter_value + 9;
    	
    ny_value = lpf_value - shutter_value + 1;
    
    ne_value=  (lpf_value+1-ny_value)*line_size- shutter;
    
    if(ny_value %2 != 0)
    {
    	lpf_value++;
    	ny_value++;
    }
    
    PAS6180SERIALYUV_write_cmos_sensor(0x0d,(ne_value&0x00ff));		/* Cmd_OffNe[7:0] */
    PAS6180SERIALYUV_write_cmos_sensor(0x91,(ne_value&0x1f00)>>8);	/* Cmd_OffNe[15:8] */			
    PAS6180SERIALYUV_write_cmos_sensor(0x04,(lpf_value&0x00ff)); 		/* Cmd_Lpf[7:0] */
    PAS6180SERIALYUV_write_cmos_sensor(0x05,(lpf_value&0x3f00)>>8);	/* Cmd_Lpf[13:8] */
    PAS6180SERIALYUV_write_cmos_sensor(0x0e,(ny_value&0x00ff));		/* Cmd_OffNy[7:0] */
    PAS6180SERIALYUV_write_cmos_sensor(0x0f,(ny_value&0x3f00)>>8);	/* Cmd_OffNy[13:8] */
    
    PAS6180SERIALYUV_write_cmos_sensor(0x11,0x01);	/*update mode*/		
    	
}   /*  PAS6180SERIALYUV_write_Shutter    */

static void PAS6180SERIALYUV_SetShutter(kal_uint32 iShutter)
{
    PAS6180SERIALYUV_pv_exposure_lines = iShutter;
    PAS6180SERIALYUV_write_Shutter(iShutter);

}/*  PAS6180SERIALYUV_SetShutter   */

/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_read_Shutter
*
* DESCRIPTION
*	This function read current shutter for calculate the exposure.
*
* PARAMETERS
*	None
*
* RETURNS
*	kal_uint16 : The current shutter value.
*
*************************************************************************/
static kal_uint32 PAS6180SERIALYUV_read_Shutter(void)
{
    kal_uint32 lpf_value, ny_value, ne_value;
    kal_uint16 Hsize, Nov_Size;
    kal_uint32 	pv_shutter, pv_extra_shutter;
    
    PAS6180SERIALYUV_write_cmos_sensor(0xef,0x01);
    Hsize = ((PAS6180SERIALYUV_read_cmos_sensor(0x1C)&0x03)<<8)|PAS6180SERIALYUV_read_cmos_sensor(0x1B);
    Nov_Size = ((PAS6180SERIALYUV_read_cmos_sensor(0x80)&0x0F)<<8)|PAS6180SERIALYUV_read_cmos_sensor(0x03);
    lpf_value = ((PAS6180SERIALYUV_read_cmos_sensor(0x05)&0x3F)<<8)|PAS6180SERIALYUV_read_cmos_sensor(0x04);//Cmd_Lpf[13:0]
    ny_value = ((PAS6180SERIALYUV_read_cmos_sensor(0x0f)&0x3F)<<8)|PAS6180SERIALYUV_read_cmos_sensor(0x0e);//Cmd_OffNy[13:0]
    ne_value = ((PAS6180SERIALYUV_read_cmos_sensor(0x91)&0x1F)<<8)|PAS6180SERIALYUV_read_cmos_sensor(0x0d);//Cmd_OffNe[15:0]
    
    pv_shutter = (lpf_value+1-ny_value)*(Hsize+1+Nov_Size)-(ne_value+1);
    
    pv_extra_shutter = 0;
    
    return pv_shutter + pv_extra_shutter;

}


/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_write_sensor_gain
*
* DESCRIPTION
*	This function used to write the sensor gain.
*
* PARAMETERS
*	1. kal_uint32 : The sensor gain want to apply to sensor.
*
* RETURNS
*	None
*
*************************************************************************/
static void PAS6180SERIALYUV_SERIAL_write_sensor_gain(kal_uint32 gain)
{
	return ;
}  /* PAS6180SERIALYUV_write_sensor_gain */

/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_read_sensor_gain
*
* DESCRIPTION
*	This function read current sensor gain for calculate the exposure.
*
* PARAMETERS
*	None
*
* RETURNS
*	kal_uint16 : The current sensor gain value.
*
*************************************************************************/
static kal_uint32 PAS6180SERIALYUV_read_sensor_gain(void)
{
	return 64;
}  /* PAS6180SERIALYUV_read_sensor_gain */

/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_SetDummy
*
* DESCRIPTION
*	This function set the dummy pixels(Horizontal Blanking) & dummy lines(Vertical Blanking), it can be
*	used to adjust the frame rate or gain more time for back-end process.
*	
*	IMPORTANT NOTICE: the base shutter need re-calculate for some sensor, or else flicker may occur.
*
* PARAMETERS
*	1. kal_uint32 : Dummy Pixels (Horizontal Blanking)
*	2. kal_uint32 : Dummy Lines (Vertical Blanking)
*
* RETURNS
*	None
*
*************************************************************************/
static void PAS6180SERIALYUV_SetDummy(const kal_uint16 dummy_pixels, const kal_uint16 dummy_lines)
{
    //Dummy pxiels => Nov Size
    //Dummy lines => LPF_min
    kal_uint32 base_LPF = 0x0150;//0x0154; //Bank_0_Reg_0x13, Bank_0_reg_0x14
    kal_uint32 base_Ny = 0x02; //Bank_0_reg_0x14
    kal_uint32 base_Nov = 0x44; //Bank_1_Reg_0x03, Bank_1_Reg_0x80
    kal_uint16 i_dummy_pixels, i_dummy_lines;
    i_dummy_pixels = dummy_pixels;
    i_dummy_lines = dummy_lines;	
    /*The dummy_pixel must < 0x0FFF*/
    if (i_dummy_pixels > 0x0FFF) //
    	i_dummy_pixels = 0x0FFF;
    else if (i_dummy_pixels < base_Nov)
    	i_dummy_pixels = base_Nov;
    
    PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x01);//Bank 1
    PAS6180SERIALYUV_write_cmos_sensor(0x03,(i_dummy_pixels&0x00FF));
    PAS6180SERIALYUV_write_cmos_sensor(0x80,(i_dummy_pixels&0xFF00)>>8);
    PAS6180SERIALYUV_write_cmos_sensor(0x11,0x01);//Bank 1 Update
    
    
    if (i_dummy_lines > 0x07FF) //
    	i_dummy_lines = 0x07FF;
    else if (i_dummy_lines < base_LPF)
    	i_dummy_lines = base_LPF;
    
    PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x00);//Bank 0
    PAS6180SERIALYUV_write_cmos_sensor(0x13,(i_dummy_lines&0x00FF));
    PAS6180SERIALYUV_write_cmos_sensor(0x14,(base_Ny<<4)|((i_dummy_lines&0x0700)>>8));
    PAS6180SERIALYUV_write_cmos_sensor(0xed,0x01);//Bank 0 Update
}   /*  PAS6180SERIALYUV_SetDummy    */


/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_Set_Mirror_Flip
*
* DESCRIPTION
*	This function mirror, flip or mirror & flip the sensor output image.
*
*	IMPORTANT NOTICE: For some sensor, it need re-set the output order Y1CbY2Cr after
*	mirror or flip.
*
* PARAMETERS
*	1. kal_uint16 : horizontal mirror or vertical flip direction.
*
* RETURNS
*	None
*
*************************************************************************/
static void PAS6180SERIALYUV_Set_Mirror_Flip(INT32 image_mirror)
{
    kal_uint8 temp_Mirror_reg = 0;
    
    PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x01);	 
    temp_Mirror_reg = PAS6180SERIALYUV_read_cmos_sensor(0x21) & ~0x0c;//Clear the mirror and flip bits.
    
    switch (image_mirror)
    {
        case IMAGE_NORMAL:
        	PAS6180SERIALYUV_write_cmos_sensor(0x21, (temp_Mirror_reg | 0x00));	//Set normal 
        	break;
        case IMAGE_H_MIRROR:
        	PAS6180SERIALYUV_write_cmos_sensor(0x21, (temp_Mirror_reg | 0x04));	 //Set flip
        	break;
        case IMAGE_V_MIRROR:
        	PAS6180SERIALYUV_write_cmos_sensor(0x21, (temp_Mirror_reg | 0x08));	 //Set mirror
        	break;
        case IMAGE_HV_MIRROR:
        	PAS6180SERIALYUV_write_cmos_sensor(0x21, (temp_Mirror_reg | 0x0C));	 //Set mirror and flip
        	break;

    }
    
    PAS6180SERIALYUV_write_cmos_sensor(0x11,0x01);  
	
}


/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_set_AWB_mode
*
* DESCRIPTION
*	This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/
static void PAS6180SERIALYUV_set_AWB_mode(kal_bool AWB_enable)
{
    kal_uint8 temp_AWB_reg = 0;
    PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x00);  
    temp_AWB_reg = PAS6180SERIALYUV_read_cmos_sensor(0x72);	
	
    if (AWB_enable == KAL_TRUE)
    {
    	//enable Auto WB
        PAS6180SERIALYUV_write_cmos_sensor(0x72, (temp_AWB_reg | 0x01));
    }
    else
    {
    	//turn off AWB
        PAS6180SERIALYUV_write_cmos_sensor(0x72, (temp_AWB_reg & ~0x01));
    }	

}

/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_set_AE_mode
*
* DESCRIPTION
*	This function enable or disable the ae (Auto Exposure).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable ae, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/
static void PAS6180SERIALYUV_set_AE_mode(kal_bool AE_enable)
{
    kal_uint8 temp_AE_reg = 0;
    PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x00);	 
    temp_AE_reg = PAS6180SERIALYUV_read_cmos_sensor(0x66);	
    
    if (AE_enable == KAL_TRUE)
    {
    	// turn on AEC/AGC
        PAS6180SERIALYUV_write_cmos_sensor(0x66, (temp_AE_reg | 0x10));

    }
    else
    {
    	// turn off AEC/AGC
        PAS6180SERIALYUV_write_cmos_sensor(0x66, (temp_AE_reg & ~0x10)); 
    }
}

/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_Sensor_Init
*
* DESCRIPTION
*	This function apply all of the initial setting to sensor.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
*************************************************************************/
static void PAS6180SERIALYUV_Sensor_Init(void)
{
    
     PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xe6,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x6b,0x01);//SPI on
    
     PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x04,0x00);//R_AE_stage_indoor_Sel
     PAS6180SERIALYUV_write_cmos_sensor(0x07,0xA0);//0xDC);//R_Indoor_Ythd[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x08,0x23);
     PAS6180SERIALYUV_write_cmos_sensor(0x09,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x0A,0x64);
     PAS6180SERIALYUV_write_cmos_sensor(0x0C,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x0F,0x3A);
     PAS6180SERIALYUV_write_cmos_sensor(0x11,0x4E);
     PAS6180SERIALYUV_write_cmos_sensor(0x13,0x50);//R_lpf_min[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x14,0x21);
     PAS6180SERIALYUV_write_cmos_sensor(0x15,0x3A);
     PAS6180SERIALYUV_write_cmos_sensor(0x17,0x3C);
     PAS6180SERIALYUV_write_cmos_sensor(0x19,0x43);//R_AWB_DGnR_LB_by2[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x1A,0x64);//R_AWB_DGnR_UB_by2[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x1B,0x45);//R_AWB_DGnB_LB_by2[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x1C,0x6A);//R_AWB_DGnB_UB_by2[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x1D,0xFE);
     PAS6180SERIALYUV_write_cmos_sensor(0x1E,0xC8);
     PAS6180SERIALYUV_write_cmos_sensor(0x1F,0x16);//R_Denoise_Model
     PAS6180SERIALYUV_write_cmos_sensor(0x20,0x1A);//R_DeNoise_Str__G[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x21,0x06);
     PAS6180SERIALYUV_write_cmos_sensor(0x22,0x8C);
     PAS6180SERIALYUV_write_cmos_sensor(0x23,0x1A);//R_DeNoise_Str__RB[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x24,0x06);
     PAS6180SERIALYUV_write_cmos_sensor(0x25,0xA0);
     PAS6180SERIALYUV_write_cmos_sensor(0x26,0x04);
     PAS6180SERIALYUV_write_cmos_sensor(0x27,0x04);
     PAS6180SERIALYUV_write_cmos_sensor(0x29,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x2B,0x20);
     PAS6180SERIALYUV_write_cmos_sensor(0x2C,0x3A);
     PAS6180SERIALYUV_write_cmos_sensor(0x2D,0x64);
     PAS6180SERIALYUV_write_cmos_sensor(0x2E,0x75);
     PAS6180SERIALYUV_write_cmos_sensor(0x2F,0x84);
     PAS6180SERIALYUV_write_cmos_sensor(0x30,0x92);
     PAS6180SERIALYUV_write_cmos_sensor(0x31,0x9F);
     PAS6180SERIALYUV_write_cmos_sensor(0x32,0xB5);
     PAS6180SERIALYUV_write_cmos_sensor(0x33,0xC7);
     PAS6180SERIALYUV_write_cmos_sensor(0x34,0xD5);
     PAS6180SERIALYUV_write_cmos_sensor(0x35,0xE0);
     PAS6180SERIALYUV_write_cmos_sensor(0x36,0xEF);
     PAS6180SERIALYUV_write_cmos_sensor(0x37,0xF8);
     PAS6180SERIALYUV_write_cmos_sensor(0x38,0xFD);
     PAS6180SERIALYUV_write_cmos_sensor(0x39,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x3A,0x4B);
     PAS6180SERIALYUV_write_cmos_sensor(0x3B,0x08);
     PAS6180SERIALYUV_write_cmos_sensor(0x3C,0x1E);//R_Ycap_dark_Thd2[7:0]  
     PAS6180SERIALYUV_write_cmos_sensor(0x3D,0x10);
     PAS6180SERIALYUV_write_cmos_sensor(0x44,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x45,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x46,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x47,0x31);
     PAS6180SERIALYUV_write_cmos_sensor(0x48,0x3F);//R_AWB_CThdM[3:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x49,0x7F);//R_AWB_SumRatio_B
     PAS6180SERIALYUV_write_cmos_sensor(0x4A,0x7F);//R_AWB_SumRatio_R
     PAS6180SERIALYUV_write_cmos_sensor(0x4B,0x42);
     PAS6180SERIALYUV_write_cmos_sensor(0x4C,0x06);
     PAS6180SERIALYUV_write_cmos_sensor(0x4D,0x64);
     PAS6180SERIALYUV_write_cmos_sensor(0x4E,0x87);
     PAS6180SERIALYUV_write_cmos_sensor(0x4F,0x7A);//R_AWB_CbCrThdL[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x50,0x75);
     PAS6180SERIALYUV_write_cmos_sensor(0x51,0x96);
     PAS6180SERIALYUV_write_cmos_sensor(0x52,0x85);//R_AWB_CbCrThdH[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x53,0x14);
     PAS6180SERIALYUV_write_cmos_sensor(0x54,0xFA);
     PAS6180SERIALYUV_write_cmos_sensor(0x55,0x42);//R_AWB_CThdDn[3:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x57,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x58,0x02);//R_AWB_LockRange_Out[5:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x5B,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x5F,0x08);
     PAS6180SERIALYUV_write_cmos_sensor(0x60,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x61,0x82);//R_GBalance_Thd_LL[3:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x62,0x02);//R_GBalance_Thd_LB[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x63,0x30);
     PAS6180SERIALYUV_write_cmos_sensor(0x64,0x08);//R_AE_LockRange_Out_UB[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x65,0x40);
     PAS6180SERIALYUV_write_cmos_sensor(0x66,0x02);//R_AE_EnH
     PAS6180SERIALYUV_write_cmos_sensor(0x67,0x12);
     PAS6180SERIALYUV_write_cmos_sensor(0x68,0x7A);
     PAS6180SERIALYUV_write_cmos_sensor(0x69,0x18);
     PAS6180SERIALYUV_write_cmos_sensor(0x6A,0x30);
     PAS6180SERIALYUV_write_cmos_sensor(0x6B,0x04);
     PAS6180SERIALYUV_write_cmos_sensor(0x6C,0x14);//R_AE_maxStage[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x6D,0x2E);//R_AG_stage_UB
     PAS6180SERIALYUV_write_cmos_sensor(0x6E,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x6F,0x87);/*******************0x91 =145, 0x87 = 135***************************************************/
     PAS6180SERIALYUV_write_cmos_sensor(0x70,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x71,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x72,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x73,0xC0);
     PAS6180SERIALYUV_write_cmos_sensor(0x74,0xA2);//R_AWB_BalGain_B[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x75,0x0D);
     PAS6180SERIALYUV_write_cmos_sensor(0x76,0x84);//R_AWB_HCT_WeightThd[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x77,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x78,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x79,0x0E);
     PAS6180SERIALYUV_write_cmos_sensor(0x7B,0x04);
     PAS6180SERIALYUV_write_cmos_sensor(0x7C,0x90);
     PAS6180SERIALYUV_write_cmos_sensor(0x7D,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x7E,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x7F,0xBF);
     PAS6180SERIALYUV_write_cmos_sensor(0x80,0x82);
     PAS6180SERIALYUV_write_cmos_sensor(0x81,0x23);
     PAS6180SERIALYUV_write_cmos_sensor(0x82,0x7E);
     PAS6180SERIALYUV_write_cmos_sensor(0x84,0x18);
     PAS6180SERIALYUV_write_cmos_sensor(0x85,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x86,0x0A);
     PAS6180SERIALYUV_write_cmos_sensor(0x87,0xEA);
     PAS6180SERIALYUV_write_cmos_sensor(0x88,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x89,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x8A,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x8B,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x8C,0x9E);
     PAS6180SERIALYUV_write_cmos_sensor(0x8E,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x8F,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x90,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x91,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x93,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x94,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x95,0x0A);//R_SENCLK_delay[3:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x97,0x10);
     PAS6180SERIALYUV_write_cmos_sensor(0x98,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x99,0x40);//R_OffsetX_R[6:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x9A,0x40);//R_OffsetY_R[6:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x9B,0x40);//R_OffsetX_G[6:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x9C,0x40);//R_OffsetY_G[6:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x9D,0x40);//R_OffsetX_B[6:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x9E,0x40);//R_OffsetY_B[6:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x9F,0x28);
     PAS6180SERIALYUV_write_cmos_sensor(0xA0,0x28);
     PAS6180SERIALYUV_write_cmos_sensor(0xA1,0x28);
     PAS6180SERIALYUV_write_cmos_sensor(0xA2,0x31);//R_LSC_R2[5:0]
     PAS6180SERIALYUV_write_cmos_sensor(0xA3,0x28);
     PAS6180SERIALYUV_write_cmos_sensor(0xA4,0x26);
     PAS6180SERIALYUV_write_cmos_sensor(0xA5,0x07);//R_LSFT_1[2:0]
     PAS6180SERIALYUV_write_cmos_sensor(0xA6,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA7,0x01);//R_LSFT_3[2:0]
     PAS6180SERIALYUV_write_cmos_sensor(0xD3,0x00);//CurrentMidleGain[5:0]
     PAS6180SERIALYUV_write_cmos_sensor(0xDF,0x20);
     PAS6180SERIALYUV_write_cmos_sensor(0xE0,0xF2);
     PAS6180SERIALYUV_write_cmos_sensor(0xE1,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xE2,0x40);
     PAS6180SERIALYUV_write_cmos_sensor(0xE3,0x01);
    // PAS6180SERIALYUV_write_cmos_sensor(0xE6,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xE9,0x87);
     PAS6180SERIALYUV_write_cmos_sensor(0xEA,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xEB,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xEC,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xED,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xEE,0x3F);
     PAS6180SERIALYUV_write_cmos_sensor(0xED,0x01);
    
     PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x02,0x03);
     PAS6180SERIALYUV_write_cmos_sensor(0x03,0x44);//Cmd_Nov_Size[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x04,0x1F);//Cmd_Lpf[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x05,0x02);//Cmd_Lpf[13:8]
     PAS6180SERIALYUV_write_cmos_sensor(0x06,0x0F);
     PAS6180SERIALYUV_write_cmos_sensor(0x07,0x0F);
     PAS6180SERIALYUV_write_cmos_sensor(0x08,0x0F);
     PAS6180SERIALYUV_write_cmos_sensor(0x09,0x0F);
     PAS6180SERIALYUV_write_cmos_sensor(0x0C,0x25);//Cmd_Dac_ct
     PAS6180SERIALYUV_write_cmos_sensor(0x0D,0x96);//Cmd_OffNe[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x0E,0x55);//Cmd_OffNy[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x0F,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x10,0x02);//Cmd_Global[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x11,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x17,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x18,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x19,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x1A,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x1B,0xFB);
     PAS6180SERIALYUV_write_cmos_sensor(0x1C,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x1D,0x47);
     PAS6180SERIALYUV_write_cmos_sensor(0x1E,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x20,0x16);
     PAS6180SERIALYUV_write_cmos_sensor(0x21,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x22,0x00);//Cmd_Pxclk_Inv
     PAS6180SERIALYUV_write_cmos_sensor(0x23,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x24,0x05);
     PAS6180SERIALYUV_write_cmos_sensor(0x28,0x08);
     PAS6180SERIALYUV_write_cmos_sensor(0x2A,0x03);
     PAS6180SERIALYUV_write_cmos_sensor(0x2B,0x31);//
     PAS6180SERIALYUV_write_cmos_sensor(0x2C,0x3b);//
     PAS6180SERIALYUV_write_cmos_sensor(0x2D,0x21);//
     PAS6180SERIALYUV_write_cmos_sensor(0x2E,0x30);//
     PAS6180SERIALYUV_write_cmos_sensor(0x2F,0x31);//
     PAS6180SERIALYUV_write_cmos_sensor(0x30,0x33);//
     PAS6180SERIALYUV_write_cmos_sensor(0x33,0x28);//
     PAS6180SERIALYUV_write_cmos_sensor(0x34,0x42);//
     PAS6180SERIALYUV_write_cmos_sensor(0x35,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x36,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x37,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x38,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x39,0x02);//Cmd_Nepls[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x3B,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x3C,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x3D,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x3E,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x41,0x06);
     PAS6180SERIALYUV_write_cmos_sensor(0x43,0x8A);
     PAS6180SERIALYUV_write_cmos_sensor(0x44,0x0E);
     PAS6180SERIALYUV_write_cmos_sensor(0x45,0x23);
     PAS6180SERIALYUV_write_cmos_sensor(0x48,0x50);
     PAS6180SERIALYUV_write_cmos_sensor(0x49,0x10);
     PAS6180SERIALYUV_write_cmos_sensor(0x4A,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x4B,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x50,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x51,0x20);
     PAS6180SERIALYUV_write_cmos_sensor(0x52,0x29);
     PAS6180SERIALYUV_write_cmos_sensor(0x53,0x62);
     PAS6180SERIALYUV_write_cmos_sensor(0x58,0x00);//Cmd_ggh_EnH[1:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x5D,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x5E,0x06);
     PAS6180SERIALYUV_write_cmos_sensor(0x62,0x35);
     PAS6180SERIALYUV_write_cmos_sensor(0x63,0x7F);
     PAS6180SERIALYUV_write_cmos_sensor(0x64,0x3C);
     PAS6180SERIALYUV_write_cmos_sensor(0x66,0xE4);//T_dig_sink[2:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x67,0x07);
     PAS6180SERIALYUV_write_cmos_sensor(0x68,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x69,0x32);
     PAS6180SERIALYUV_write_cmos_sensor(0x70,0xAB);
     PAS6180SERIALYUV_write_cmos_sensor(0x71,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x76,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x77,0x20);
     PAS6180SERIALYUV_write_cmos_sensor(0x78,0x0B);//IO Driving Current
     PAS6180SERIALYUV_write_cmos_sensor(0x79,0x0F);
     PAS6180SERIALYUV_write_cmos_sensor(0x7A,0x0F);
     PAS6180SERIALYUV_write_cmos_sensor(0x7B,0x8A);
     PAS6180SERIALYUV_write_cmos_sensor(0x7D,0x0F);
     PAS6180SERIALYUV_write_cmos_sensor(0x7E,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x80,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x82,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x91,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x9F,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA1,0x04);
     PAS6180SERIALYUV_write_cmos_sensor(0xA2,0xC0);
     PAS6180SERIALYUV_write_cmos_sensor(0xA4,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA5,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA6,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA7,0x08);
     PAS6180SERIALYUV_write_cmos_sensor(0xA8,0x0F);
     PAS6180SERIALYUV_write_cmos_sensor(0xA9,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xAA,0xAA);
     PAS6180SERIALYUV_write_cmos_sensor(0xAB,0x55);
     PAS6180SERIALYUV_write_cmos_sensor(0xAC,0x67);
     PAS6180SERIALYUV_write_cmos_sensor(0xB0,0x05);
     PAS6180SERIALYUV_write_cmos_sensor(0xB1,0x3F);
     PAS6180SERIALYUV_write_cmos_sensor(0xB2,0xCA);
     PAS6180SERIALYUV_write_cmos_sensor(0xB4,0x0C);
     PAS6180SERIALYUV_write_cmos_sensor(0xB6,0x0E);
     PAS6180SERIALYUV_write_cmos_sensor(0xB8,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xB9,0x5E);
     PAS6180SERIALYUV_write_cmos_sensor(0xBA,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0xBB,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xBC,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xBF,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC0,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC1,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x11,0x01);
    
     PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x00,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x0B,0x50);
     PAS6180SERIALYUV_write_cmos_sensor(0x0C,0x50);
     PAS6180SERIALYUV_write_cmos_sensor(0x0D,0x40);
     PAS6180SERIALYUV_write_cmos_sensor(0x18,0x88);
     PAS6180SERIALYUV_write_cmos_sensor(0x19,0xC6);
     PAS6180SERIALYUV_write_cmos_sensor(0x1A,0xB1);
     PAS6180SERIALYUV_write_cmos_sensor(0x1B,0x87);//R_AWB_WeightSH[3:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x1C,0x69);
     PAS6180SERIALYUV_write_cmos_sensor(0x1D,0x7D);
     PAS6180SERIALYUV_write_cmos_sensor(0x1E,0x87);
     PAS6180SERIALYUV_write_cmos_sensor(0x1F,0x9B);
     PAS6180SERIALYUV_write_cmos_sensor(0x20,0xB4);
     PAS6180SERIALYUV_write_cmos_sensor(0x21,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x22,0x08);
     PAS6180SERIALYUV_write_cmos_sensor(0x26,0x18);
     PAS6180SERIALYUV_write_cmos_sensor(0x27,0x05);
     PAS6180SERIALYUV_write_cmos_sensor(0x28,0x1F);
     PAS6180SERIALYUV_write_cmos_sensor(0x2A,0xD2);//R_FlatRatio[3:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x2B,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x2C,0x05);//R_Edge_UB[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x2D,0x05);//R_Edge_LB[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x2E,0x1F);
     PAS6180SERIALYUV_write_cmos_sensor(0x2F,0x14);//R_AE_stage_LL[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x30,0x11);
     PAS6180SERIALYUV_write_cmos_sensor(0x31,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x35,0x08);
     PAS6180SERIALYUV_write_cmos_sensor(0x36,0x08);//R_Gamma_Strength_Delta[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x37,0x06);//R_Gamma_Strength_LL[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x38,0x8F);//R_AE_Middle_Stage[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x39,0x00);//R_AE_Middle_Gain[5:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x3C,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x3E,0x33);
     PAS6180SERIALYUV_write_cmos_sensor(0x3F,0x12);
     PAS6180SERIALYUV_write_cmos_sensor(0x40,0x25);
     PAS6180SERIALYUV_write_cmos_sensor(0x41,0x09);
     PAS6180SERIALYUV_write_cmos_sensor(0x42,0x0A);//R_CCMb1_0[6:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x43,0x3C);//R_CCMb1_1[6:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x44,0x46);//R_CCMb1_2[6:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x45,0x48);
     PAS6180SERIALYUV_write_cmos_sensor(0x46,0x48);
     PAS6180SERIALYUV_write_cmos_sensor(0x47,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x48,0x31);//R_AE_Middle_Gain2[2:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x49,0x34);//R_AE_Middle_Gain3[2:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x4A,0x28);
     PAS6180SERIALYUV_write_cmos_sensor(0x4B,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x4C,0x14);//R_AUTO_Contrast_BlackTarget[4:0]  
     PAS6180SERIALYUV_write_cmos_sensor(0x4D,0x3F);
     PAS6180SERIALYUV_write_cmos_sensor(0x4E,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x4F,0x28);
     PAS6180SERIALYUV_write_cmos_sensor(0x50,0x3C);
     PAS6180SERIALYUV_write_cmos_sensor(0x51,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x52,0x16);//R_CThdH[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x53,0x03);//R_CThdL[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x54,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x55,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x56,0x02);//R_EdgeRatio_Delta[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x57,0x00);//R_EdgeRatio_LL[3:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x58,0x08);
     PAS6180SERIALYUV_write_cmos_sensor(0x5C,0x12);//R_Edge_th_NL[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x5D,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x5E,0x03);
     PAS6180SERIALYUV_write_cmos_sensor(0x5F,0x0C);
     PAS6180SERIALYUV_write_cmos_sensor(0x60,0x13);//0x11);//R_Saturation_NL[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x61,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x62,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x63,0x5F);
     PAS6180SERIALYUV_write_cmos_sensor(0x64,0x03);//R_Contrast_En
     PAS6180SERIALYUV_write_cmos_sensor(0x65,0x46);
     PAS6180SERIALYUV_write_cmos_sensor(0x66,0x96);
     PAS6180SERIALYUV_write_cmos_sensor(0x67,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x68,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x69,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x6A,0x00);
    // PAS6180SERIALYUV_write_cmos_sensor(0x6B,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x6C,0x32);
     PAS6180SERIALYUV_write_cmos_sensor(0x6D,0x12);
     PAS6180SERIALYUV_write_cmos_sensor(0x6E,0x08);
     PAS6180SERIALYUV_write_cmos_sensor(0x6F,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x70,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x71,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x72,0x0E);
     PAS6180SERIALYUV_write_cmos_sensor(0x73,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x8D,0x13);//R_DigitalColorGain_B[4:0] 
     PAS6180SERIALYUV_write_cmos_sensor(0x8E,0x13);//R_DigitalColorGain_Gb[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x8F,0x13);//R_DigitalColorGain_Gr[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x90,0x14);//R_DigitalColorGain_R[4:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x91,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x94,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x95,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x9B,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x9C,0xF0);//R_ISP_WOI_HSize[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0x9D,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x9E,0x40);
     PAS6180SERIALYUV_write_cmos_sensor(0x9F,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA0,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA1,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA2,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA3,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA4,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0xA5,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA6,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0xA8,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xA9,0x20);
     PAS6180SERIALYUV_write_cmos_sensor(0xAA,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xAB,0xF0);//R_FIFO_HSize[7:0]
     PAS6180SERIALYUV_write_cmos_sensor(0xAD,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xAE,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xB0,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xB1,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xB2,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xB4,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xB5,0xF0);//R_ISP_WOIb_HSize[7:0] 
     PAS6180SERIALYUV_write_cmos_sensor(0xB6,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0xB7,0x40);
     PAS6180SERIALYUV_write_cmos_sensor(0xB8,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xB9,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xBA,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xBB,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xBC,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xBD,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0xBE,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xBF,0x04);//R_UV_Swap
     PAS6180SERIALYUV_write_cmos_sensor(0xC0,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC1,0x00);//R_Vsync_INV
     PAS6180SERIALYUV_write_cmos_sensor(0xC2,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC3,0xE0);
     PAS6180SERIALYUV_write_cmos_sensor(0xC4,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC5,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC6,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0xC7,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC8,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC9,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xCA,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xCB,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xCC,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xCD,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xCE,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xCF,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD0,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD1,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD2,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD3,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD4,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD5,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD6,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD7,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD8,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xD9,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xDA,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xDB,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xDC,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xDD,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xDE,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xDF,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xE0,0x04);
     PAS6180SERIALYUV_write_cmos_sensor(0xE3,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xE4,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xE5,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xE6,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xEB,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x00,0x01);
    
     PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x9C,0xF2);
     PAS6180SERIALYUV_write_cmos_sensor(0xAB,0xF2);
     PAS6180SERIALYUV_write_cmos_sensor(0xB5,0xF2);
     PAS6180SERIALYUV_write_cmos_sensor(0xBF,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC1,0x12);
     PAS6180SERIALYUV_write_cmos_sensor(0x00,0x01);
    
     PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x9C,0xF0);
     PAS6180SERIALYUV_write_cmos_sensor(0xAB,0xF0);
     PAS6180SERIALYUV_write_cmos_sensor(0xB5,0xF0);
     PAS6180SERIALYUV_write_cmos_sensor(0xBF,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xC1,0x12);
     PAS6180SERIALYUV_write_cmos_sensor(0x00,0x01);
    
     PAS6180SERIALYUV_write_cmos_sensor(0xef,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x66,0x03);//RegAddr102x66:ARegRdData={{1'b0,1'b0,1'b0,R_AE_EnH},{2'b0,R_TGWr_buf_EnH,R_freq_60}};
    
     PAS6180SERIALYUV_write_cmos_sensor(0xef,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0xbc,0x10);//RegAddr188xbc:ARegRdData={  3'b0, R_Force_ISP_Toggle,2'b0, R_PXD_Toggle_InVsync[1:0] };
     PAS6180SERIALYUV_write_cmos_sensor(0xbf,0x04);//00//RegAddr191xbf:ARegRdData={ { 5'b0,R_YC_Swap, R_UV_Swap, 1'b0 } };
     PAS6180SERIALYUV_write_cmos_sensor(0xc1,0x10);//RegAddr193xc1:ARegRdData={{1'b0,R_SenHsync_En,R_HsyncInVsync,R_SenVsync_En},{R_Pxclk_Gated_InHVSync,R_Pxclk_INV,R_Hsync_INV,R_Vsync_INV}};
     PAS6180SERIALYUV_write_cmos_sensor(0xb0,0x00);//RegAddr176xb0:ARegRdData={R_Scaler_X_En,1'b0,R_ScaleDenr_X[5:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0xb1,0x00);//RegAddr177xb1:ARegRdData={R_Scaler_Y_En,1'b0,R_ScaleDenr_Y[5:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0xb4,0x00);//RegAddr180xb4:ARegRdData={{4'b0},{3'b0,R_ISP_WOIb_HSize[8]}};
     PAS6180SERIALYUV_write_cmos_sensor(0xb5,0xf0);//RegAddr181xb5:ARegRdData={R_ISP_WOIb_HSize[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0xb6,0x01);//RegAddr182xb6:ARegRdData={{4'b0},{3'b0,R_ISP_WOIb_VSize[8]}};
     PAS6180SERIALYUV_write_cmos_sensor(0xb7,0x40);//RegAddr183xb7:ARegRdData={R_ISP_WOIb_VSize[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0xb8,0x00);//RegAddr184xb8:ARegRdData={{4'b0},{3'b0,R_ISP_WOIb_HOffset[8]}};
     PAS6180SERIALYUV_write_cmos_sensor(0xb9,0x00);//RegAddr185xb9:ARegRdData={R_ISP_WOIb_HOffset[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0xba,0x00);//RegAddr186xba:ARegRdData={{4'b0},{3'b0,R_ISP_WOIb_VOffset[8]}};
     PAS6180SERIALYUV_write_cmos_sensor(0xbb,0x00);//RegAddr187xbb:ARegRdData={R_ISP_WOIb_VOffset[7:0]};
    //-------------fifo-----------------
     PAS6180SERIALYUV_write_cmos_sensor(0xc5,0x00);//RegAddr197xc5:ARegRdData={{2'b0,R_SPI_FIFO_Ptr[5:0]}};
     PAS6180SERIALYUV_write_cmos_sensor(0xa3,0x01);//RegAddr163xa3:ARegRdData={7'b0,R_ScalingFIFO_En};
     PAS6180SERIALYUV_write_cmos_sensor(0xa4,0x02);//RegAddr164xa4:ARegRdData={3'b0,R_ScalingFIFO_Out_NP[4:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0xa5,0x00);//RegAddr165xa5:ARegRdData={7'b0,R_Ptr_ScalingFIFO[8]};
     PAS6180SERIALYUV_write_cmos_sensor(0xa6,0x20);//RegAddr166xa6:ARegRdData={R_Ptr_ScalingFIFO[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0xa8,0x00);//RegAddr168xa8:ARegRdData={7'b0,R_FIFO_Hsync[8]};
     PAS6180SERIALYUV_write_cmos_sensor(0xa9,0x20);//RegAddr169xa9:ARegRdData={R_FIFO_Hsync[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0xaa,0x00);//RegAddr170xaa:ARegRdData={7'b0,R_FIFO_HSize[8]};
     PAS6180SERIALYUV_write_cmos_sensor(0xab,0xf0);//RegAddr171xab:ARegRdData={R_FIFO_HSize[7:0]};
    //-------------spi-----------------
     PAS6180SERIALYUV_write_cmos_sensor(0xc6,0x01);//RegAddr198xc6:ARegRdData={{2'b0,R_SPI_SysClk_Div[5:0]}};
     PAS6180SERIALYUV_write_cmos_sensor(0x6c,0xff);//RegAddr108x6c:ARegRdData={R_MTK_SPI_SyncCode_Bit_07_00[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0x6d,0xff);//RegAddr109x6d:ARegRdData={R_MTK_SPI_SyncCode_Bit_15_08[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0x6e,0xff);//RegAddr110x6e:ARegRdData={R_MTK_SPI_SyncCode_Bit_23_16[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0x6f,0x01);//RegAddr111x6f:ARegRdData={7'b0,R_MTK_SPI_VSIZE};
     PAS6180SERIALYUV_write_cmos_sensor(0x70,0x40);//RegAddr112x70:ARegRdData={R_MTK_SPI_VSIZE[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0x71,0x00);//RegAddr113x71:ARegRdData={7'b0,R_MTK_SPI_HSIZE};
     PAS6180SERIALYUV_write_cmos_sensor(0x72,0xf0);//RegAddr114x72:ARegRdData={R_MTK_SPI_HSIZE[7:0]};
     PAS6180SERIALYUV_write_cmos_sensor(0x6b,0x01);//05//RegAddr107x6b:ARegRdData={2'b0,R_MTK_SPI_DataParallelism[1:0],1'b0,R_MTK_SPI_ClkInv,R_MTK_SPI_LineStartPktEn,R_MTK_SPI_En};
    //-------------spi-----------------
     PAS6180SERIALYUV_write_cmos_sensor(0x00,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0xef,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x77,0x01);//00//RegAddr119x77:ARegRdData={6'b0, R_VHSYNC_Out_GateAllMode,R_VHSYNC_Out_Gate};
    //24/8*1000000/256=11718=2DC6
    // PAS6180SERIALYUV_write_cmos_sensor(0x67,0xC6);//RegAddr103x67:ARegRdData={ {R_SysClk_freq[7:0]}};
    // PAS6180SERIALYUV_write_cmos_sensor(0x68,0x2D);//RegAddr104x68:ARegRdData={ {1'b0, R_SysClk_freq[14:8]}};
    //52/8*1000000/256=25390=632E
     PAS6180SERIALYUV_write_cmos_sensor(0x67,0x2E);//RegAddr103x67:ARegRdData={ {R_SysClk_freq[7:0]}};
     PAS6180SERIALYUV_write_cmos_sensor(0x68,0x63);//RegAddr104x68:ARegRdData={ {1'b0, R_SysClk_freq[14:8]}};
    
    //66 13//AE_ON
     PAS6180SERIALYUV_write_cmos_sensor(0xef,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x02,0x08);//10//2:RegRdData=Cmd_Np[7:0];
     PAS6180SERIALYUV_write_cmos_sensor(0x11,0x01);//17:RegRdData={FastUpdate,6'b0,UpdateFlag};
    
    //=================================
     PAS6180SERIALYUV_write_cmos_sensor(0xef,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0xe6,0x00);
    
    //MTK SPI Output Enable
     PAS6180SERIALYUV_write_cmos_sensor(0xef,0x02);
     PAS6180SERIALYUV_write_cmos_sensor(0x67,0x01);
     PAS6180SERIALYUV_write_cmos_sensor(0x00,0x01);
    
    //AE on
     PAS6180SERIALYUV_write_cmos_sensor(0xEF,0x00);
     PAS6180SERIALYUV_write_cmos_sensor(0x66,0x13);
    
     //PAS6180_SERIAL_sensor.preview_pclk = 80;
    


}


/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUVOpen
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
UINT32 PAS6180SERIALYUVOpen(void)
{
    kal_uint16 sensor_id=0; 
    int retry = 3; 

    // Dummy read  for wake-up sensor
    /******NOTICE: FAE RECOMMAND--In order to wale up sensor, It must read at least 4 times************/
    PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
    PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
    PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
    PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
    PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
    PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
    // check if sensor ID correct
    do{
        PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
        PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
        PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
        PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
        PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);
        PAS6180SERIALYUV_sensor_id = (PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01);

        if(PAS6180_SENSOR_ID == PAS6180SERIALYUV_sensor_id)
            break;
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", PAS6180SERIALYUV_sensor_id); 
        retry--; 				
    }while(retry > 0);
	
    SENSORDB("Read Sensor ID = 0x%04x\n", PAS6180SERIALYUV_sensor_id); 

    if (PAS6180_SENSOR_ID != PAS6180SERIALYUV_sensor_id)
        return ERROR_SENSOR_CONNECT_FAIL;

    PAS6180SERIALYUV_Sensor_Init();	

//    IMX073MIPI_sensor_gain_base = read_IMX073MIPI_gain();
    
    PAS6180SERIALYUV_sensor_id = 0;	

    SENSORDB("PAS6180_SENSOR open end \n");
    
    return ERROR_NONE;
}   /* PAS6180SERIALYUVOpen  */


/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUV_GetSensorID
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
static kal_uint32 PAS6180SERIALYUV_GetSensorID(kal_uint32 *sensorID)

{
    int  retry = 3; 
    // check if sensor ID correct
    do {
        *sensorID=((PAS6180SERIALYUV_read_cmos_sensor(0x00) << 8) | PAS6180SERIALYUV_read_cmos_sensor(0x01));
        if (*sensorID == PAS6180_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != PAS6180_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;    
}   /* PAS6180SERIALYUV_GetSensorID  */

/*************************************************************************
* FUNCTION
*	AS6180SERIALYUVClose
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
UINT32 PAS6180SERIALYUVClose(void)
{

	return ERROR_NONE;
}   /* PAS6180SERIALYUVClose */




/*************************************************************************
* FUNCTION
* PAS6180SERIALYUVPreview
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
UINT32 PAS6180SERIALYUVPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 iStartX = 0, iStartY = 0;

    g_iPAS6180SERIALYUV_Mode = PAS6180_MODE_PREVIEW;

//preview setting
//preview resolution is the same as capture

    if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        PAS6180SERIALYUV_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {
        PAS6180SERIALYUV_MPEG4_encode_mode = KAL_FALSE;
    }

    iStartX += PAS6180_IMAGE_SENSOR_START_X;
    iStartY += PAS6180_IMAGE_SENSOR_START_Y;
	
    PAS6180SERIALYUV_dummy_pixels = 0;
    PAS6180SERIALYUV_dummy_lines = 0;
    PAS6180SERIALYUV_PV_dummy_pixels = PAS6180SERIALYUV_dummy_pixels;
    PAS6180SERIALYUV_PV_dummy_lines = PAS6180SERIALYUV_dummy_lines;

    PAS6180SERIALYUV_SetDummy(PAS6180SERIALYUV_dummy_pixels, PAS6180SERIALYUV_dummy_lines);

    // copy sensor_config_data
    memcpy(&PAS6180SERIALYUVSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
 	
    image_window->GrabStartX= iStartX;
    image_window->GrabStartY= iStartY;
    image_window->ExposureWindowWidth = PAS6180_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
    image_window->ExposureWindowHeight = PAS6180_IMAGE_SENSOR_PV_HEIGHT - 2*iStartY;
    SENSORDB("Preview resolution:%d %d %d %d\n", image_window->GrabStartX, image_window->GrabStartY, image_window->ExposureWindowWidth, image_window->ExposureWindowHeight); 
    return ERROR_NONE;

}   /*  PAS6180SERIALYUVPreview   */

/*************************************************************************
* FUNCTION
*	PAS6180SERIALYUVCapture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 PAS6180SERIALYUVCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint32 shutter = 0;
    kal_uint16 iStartX = 0, iStartY = 0;

    g_iPAS6180SERIALYUV_Mode = PAS6180_MODE_CAPTURE;

    if(sensor_config_data->EnableShutterTansfer==KAL_TRUE)
        shutter=sensor_config_data->CaptureShutter;
	
//1. disable night mode 

//2. disable AE,AWB
    //PAS6180SERIALYUV_set_AE_mode(KAL_FALSE);
    //PAS6180SERIALYUV_set_AWB_mode(KAL_FALSE);

//3. read shutter, gain
    //shutter = PAS6180SERIALYUV_read_shutter();

    sensor_config_data->DefaultPclk = 26000000;
    sensor_config_data->Pixels = PAS6180_FULL_PERIOD_PIXEL_NUMS + PAS6180SERIALYUV_PV_dummy_pixels;
    sensor_config_data->FrameLines =PAS6180_PV_PERIOD_LINE_NUMS+PAS6180SERIALYUV_PV_dummy_lines;
    sensor_config_data->Lines = image_window->ExposureWindowHeight;
    sensor_config_data->Shutter =shutter;

//5.set shutter
    //PAS6180SERIALYUV_write_Shutter(shutter);
//6.set gain
    


    memcpy(&PAS6180SERIALYUVSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	
    SENSORDB("capture done\n");  

    return ERROR_NONE;
}   /* OV7576_Capture() */

UINT32 PAS6180SERIALYUVGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=PAS6180_IMAGE_SENSOR_FULL_WIDTH - 2 * PAS6180_IMAGE_SENSOR_START_X;
    pSensorResolution->SensorFullHeight=PAS6180_IMAGE_SENSOR_FULL_HEIGHT - 2 * PAS6180_IMAGE_SENSOR_START_Y;
    pSensorResolution->SensorPreviewWidth=PAS6180_IMAGE_SENSOR_PV_WIDTH - 2 * PAS6180_IMAGE_SENSOR_START_X;
    pSensorResolution->SensorPreviewHeight=PAS6180_IMAGE_SENSOR_PV_HEIGHT - 2 * PAS6180_IMAGE_SENSOR_START_Y;
    return ERROR_NONE;
}	/* PAS6180SERIALYUVGetResolution() */

UINT32 PAS6180SERIALYUVGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    pSensorInfo->SensorPreviewResolutionX=PAS6180_IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY=PAS6180_IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX=PAS6180_IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=PAS6180_IMAGE_SENSOR_FULL_HEIGHT;
	
    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_VYUY;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	/*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;//parallel? serial?

#if 0
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
#endif


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
            pSensorInfo->SensorGrabStartX = PAS6180_IMAGE_SENSOR_START_X; 
            pSensorInfo->SensorGrabStartY = PAS6180_IMAGE_SENSOR_START_Y;
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            //pSensorInfo->SensorPacketECCOrder = 1;			
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:		
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 4*PAS6180_IMAGE_SENSOR_START_X; 
            pSensorInfo->SensorGrabStartY = PAS6180_IMAGE_SENSOR_START_Y;          			
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            //pSensorInfo->SensorPacketECCOrder = 1;			
            break;
        default:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount= 3;
            pSensorInfo->SensorClockRisingCount=0;
            pSensorInfo->SensorClockFallingCount=2;
            pSensorInfo->SensorPixelClockCount=3;
            pSensorInfo->SensorDataLatchCount=2;
            
            pSensorInfo->SensorGrabStartX = 1; 
            pSensorInfo->SensorGrabStartY = 1;			   
            break;
        }
	
    PAS6180SERIALYUVPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &PAS6180SERIALYUVSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}	/* PAS6180SERIALYUVGetInfo() */


UINT32 PAS6180SERIALYUVControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            PAS6180SERIALYUVPreview(pImageWindow, pSensorConfigData);
        break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
            PAS6180SERIALYUVCapture(pImageWindow, pSensorConfigData);
        break;
            //s_porting add
            //s_porting add
            //s_porting add		
        default:
        	return ERROR_INVALID_SCENARIO_ID;
            //s_porting add
            //s_porting add
            //s_porting add				
    }
    return TRUE;
}	/* PAS6180SERIALYUVControl() */




/*************************************************************************
* FUNCTION
*    PAS6180SERIALYUVFeatureControl
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
UINT32 PAS6180SERIALYUVFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                                                                UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 SensorRegNumber;
    UINT32 i;
    PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
            *pFeatureReturnPara16++=PAS6180_FULL_PERIOD_PIXEL_NUMS;  
            *pFeatureReturnPara16=PAS6180_FULL_PERIOD_LINE_NUMS+PAS6180SERIALYUV_dummy_lines;	
            SENSORDB("Sensor period:%d %d\n", PAS6180_FULL_PERIOD_PIXEL_NUMS+PAS6180SERIALYUV_dummy_pixels, PAS6180_FULL_PERIOD_LINE_NUMS+PAS6180SERIALYUV_dummy_lines); 
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            *pFeatureReturnPara32 = 57600000; //19500000;
            *pFeatureParaLen=4;
            break;			
        case SENSOR_FEATURE_SET_ESHUTTER:
            PAS6180SERIALYUV_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            PAS6180SERIALYUV_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            PAS6180SERIALYUV_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;	
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = PAS6180SERIALYUV_read_cmos_sensor(pSensorRegData->RegAddr);
            break;	
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
                PAS6180SERIALYUVSensorCCT[i].Addr=*pFeatureData32++;
                PAS6180SERIALYUVSensorCCT[i].Para=*pFeatureData32++;
            }
            break;		
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=PAS6180SERIALYUVSensorCCT[i].Addr;
                *pFeatureData32++=PAS6180SERIALYUVSensorCCT[i].Para;
            }
            break;	
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
                PAS6180SERIALYUVSensorReg[i].Addr=*pFeatureData32++;
                PAS6180SERIALYUVSensorReg[i].Para=*pFeatureData32++;
            }
            break;		
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=PAS6180SERIALYUVSensorReg[i].Addr;
                *pFeatureData32++=PAS6180SERIALYUVSensorReg[i].Para;
            }
            break;		
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=PAS6180_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, PAS6180SERIALYUVSensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, PAS6180SERIALYUVSensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;	
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &PAS6180SERIALYUVSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;			
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            PAS6180SERIALYUV_GetSensorID(pFeatureReturnPara32); 
            break; 
        default:
            break;			
		
    }
	
    return ERROR_NONE;
}



SENSOR_FUNCTION_STRUCT	SensorFuncPAS6180SERIALYUV=
{
	PAS6180SERIALYUVOpen,
	PAS6180SERIALYUVGetInfo,
	PAS6180SERIALYUVGetResolution,
	PAS6180SERIALYUVFeatureControl,
	PAS6180SERIALYUVControl,
	PAS6180SERIALYUVClose
};

UINT32 PAS6180_SERIAL_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncPAS6180SERIALYUV;

	return ERROR_NONE;
}	/* PAS6180_SERIAL_YUV_SensorInit() */

