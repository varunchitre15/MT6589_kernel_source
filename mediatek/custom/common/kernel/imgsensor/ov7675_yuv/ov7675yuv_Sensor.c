
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
#include <asm/system.h>

#include <asm/io.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "ov7675yuv_Sensor.h"
#include "ov7675yuv_Camera_Sensor_para.h"
#include "ov7675yuv_CameraCustomized.h"

static MSDK_SENSOR_CONFIG_STRUCT OV7675SensorConfigData;
static struct OV7675_Sensor_Struct OV7675_Sensor_Driver;




#define OV7675YUV_DEBUG
#ifdef OV7675YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif


#define __SENSOR_CONTROL__
#ifdef __SENSOR_CONTROL__
#define CAMERA_CONTROL_FLOW(para1,para2) printk("[%s:%d]::para1=0x%x,para1=0x%x\n\n",__FUNCTION__,__LINE__,para1,para2)
#else
#define CAMERA_CONTROL_FLOW(para1, para2)
#endif

kal_uint8 res=0,closed=0,info=0;



#if 0
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
static int sensor_id_fail = 0; 
#define OV7675_write_cmos_sensor(addr,para) iWriteReg((u16) addr , (u32) para ,1,OV7675_WRITE_ID)
//#define OV7675_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,OV7675_WRITE_ID)
kal_uint16 OV7675_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV7675_WRITE_ID);
    return get_byte;
}
#endif
static DEFINE_SPINLOCK(ov7675_yuv_drv_lock);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
/*************************************************************************
* FUNCTION
*    OV7675_write_cmos_sensor
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
static void OV7675_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
kal_uint8 out_buff[2];

    out_buff[0] = addr;
    out_buff[1] = para;

    iWriteRegI2C((u8*)out_buff , (u16)sizeof(out_buff), OV7675_WRITE_ID); 

#if (defined(__OV7675_DEBUG_TRACE__))
  if (sizeof(out_buff) != rt) printk("I2C write %x, %x error\n", addr, para);
#endif
}

/*************************************************************************
* FUNCTION
*    OV7675_read_cmos_sensor
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
static kal_uint8 OV7675_read_cmos_sensor(kal_uint8 addr)
{
  kal_uint8 in_buff[1] = {0xFF};
  kal_uint8 out_buff[1];
  
  out_buff[0] = addr;

    if (0 != iReadRegI2C((u8*)out_buff , (u16) sizeof(out_buff), (u8*)in_buff, (u16) sizeof(in_buff), OV7675_WRITE_ID)) {
        SENSORDB("ERROR: OV7675_read_cmos_sensor \n");
    }

#if (defined(__OV7675_DEBUG_TRACE__))
  if (size != rt) printk("I2C read %x error\n", addr);
#endif

  return in_buff[0];
}


#if 0
static void OV7675_Write_Shutter(kal_uint16 shutter)
{  
  kal_uint8 temp_reg;
   
	if(shutter<=VGA_EXPOSURE_LIMITATION)
	{		
		 OV7675_Sensor_Driver.extra_exposure_lines=0;
		}
	else
	{			
	OV7675_Sensor_Driver.extra_exposure_lines=shutter-VGA_EXPOSURE_LIMITATION;
	}
		
	if(shutter>VGA_EXPOSURE_LIMITATION)
	shutter=VGA_EXPOSURE_LIMITATION;
		 
	temp_reg=OV7675_read_cmos_sensor(0x04);
	OV7675_write_cmos_sensor(0x04,( (temp_reg&0xFC) | (shutter&0x3) ));			// AEC[b1~b0]
	OV7675_write_cmos_sensor(0x10,((shutter&0x3FC)>>2));								// AEC[b9~b2]
	OV7675_write_cmos_sensor(0x07,((shutter&0x400)>>10));

   OV7675_write_cmos_sensor(0x2D,OV7675_Sensor_Driver.extra_exposure_lines&0xFF);                   // ADVFL(LSB of extra exposure lines)
   OV7675_write_cmos_sensor(0x2E,(OV7675_Sensor_Driver.extra_exposure_lines&0xFF00)>>8);            // ADVFH(MSB of extra exposure lines)  
	
}   /*  OV7675_Write_Shutter    */
#endif

 static void OV7675_Set_Dummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
    OV7675_write_cmos_sensor(0x2A,((iPixels&0x700)>>4));
	OV7675_write_cmos_sensor(0x2B,(iPixels&0xFF));
	OV7675_write_cmos_sensor(0x92,(iLines&0xFF));
	OV7675_write_cmos_sensor(0x93,((iLines&0xFF00)>>8));
}   /*  OV7675_Set_Dummy    */


/*************************************************************************
* FUNCTION
*	OV7675_write_reg
*
* DESCRIPTION
*	This function set the register of OV7675.
*
* PARAMETERS
*	addr : the register index of OV76X0
*  para : setting parameter of the specified register of OV76X0
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/

//static void OV7675_write_reg(kal_uint32 addr, kal_uint32 para)
//{
//	OV7675_write_cmos_sensor(addr,para);
//}	/* OV7675_write_reg() */

/*************************************************************************
* FUNCTION
*	ov7670_read_cmos_sensor
*
* DESCRIPTION
*	This function read parameter of specified register from OV76X0.
*
* PARAMETERS
*	addr : the register index of OV76X0
*
* RETURNS
*	the data that read from OV76X0
*
* GLOBALS AFFECTED
*
*************************************************************************/
//static kal_uint32 OV7675_read_reg(kal_uint32 addr)
//{
//	return (OV7675_read_cmos_sensor(addr));
//}	/* OV7670_read_reg() */


/*************************************************************************
* FUNCTION
*	OV7675_NightMode
*
* DESCRIPTION
*	This function night mode of OV7675.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void OV7675_night_mode(kal_bool bEnable)
{
 kal_uint8 temp = OV7675_read_cmos_sensor(0x3B);


  if (!OV7675_Sensor_Driver.MODE_CAPTURE) { 
	if(bEnable)
	{      spin_lock(&ov7675_yuv_drv_lock);
		   OV7675_Sensor_Driver.bNight_mode = KAL_TRUE;
		   spin_unlock(&ov7675_yuv_drv_lock);
	   if(OV7675_Sensor_Driver.MPEG4_encode_mode == KAL_TRUE)
		{		

			 
		   OV7675_Set_Dummy(0,595);	
		   OV7675_write_cmos_sensor(0x3B,(temp&0x1F)); // fixed frame rate
		   OV7675_write_cmos_sensor(0x2d, 0); // if not set to zero, maybe AE osillation
		   OV7675_write_cmos_sensor(0x2e, 0);
		   OV7675_write_cmos_sensor(0xc9, 0x30);  			  
		}	
	    else 
	   {
	          printk("OV7675 night mode ture camera preview\r\n");
			  OV7675_Set_Dummy(0,42); 
			//  OV7675_write_cmos_sensor(0x3B,(temp&0x1F)|0xE0); //1/8 normal framerate   
			  OV7675_write_cmos_sensor(0x3B,(temp&0x1F)|0xE0); //1/4 normal framerate   
			  OV7675_write_cmos_sensor(0xCF,0x08);//gaiyang add for 1/8 normal framerate
		      OV7675_write_cmos_sensor(0x14, 0x38); //AGC ceining 16X
			  OV7675_write_cmos_sensor(0xc9, 0x10);	
		  } 		
	}
	else
	{
	       spin_lock(&ov7675_yuv_drv_lock);
		   OV7675_Sensor_Driver.bNight_mode = KAL_FALSE;
		   spin_unlock(&ov7675_yuv_drv_lock);
		
	    if(OV7675_Sensor_Driver.MPEG4_encode_mode == KAL_TRUE)
	    {
	   
		     
		     OV7675_Set_Dummy(0,42); 	
		    OV7675_write_cmos_sensor(0x3B,(temp&0x1F)); // fixed frame rate
			OV7675_write_cmos_sensor(0x2d, 0); // if not set to zero, maybe AE osillation
			OV7675_write_cmos_sensor(0x2e, 0);
			OV7675_write_cmos_sensor(0xc9, 0x30);//satuation control 					
	   }
		else 
			{	
			   
			   OV7675_Set_Dummy(0,42); 
			   OV7675_write_cmos_sensor(0x3B,(temp&0x1F)|0xA0); //1/2 normal frame rate	
			   OV7675_write_cmos_sensor(0xCF,0x00); //gaiyang add for 1/2 normal framerate
		       OV7675_write_cmos_sensor(0x14,0x28);//X8 gain
			  OV7675_write_cmos_sensor(0xc9, 0x60); //satuation control
			}
	   
	}  
	}
}	/*	OV7675_NightMode	*/

/*
static void OV7675_set_isp_driving_current(kal_uint8 current)
{
    //#define CONFIG_BASE      	(0xF0001000)     
//  iowrite32((0xE << 12)|(0 << 28)|0x8880888, 0xF0001500);
}
*/

static void OV7675_Sensor_Driver_Init(void)
{                                          
	   OV7675_write_cmos_sensor(0x11,0x80);//PLL
	   mDELAY(2); 
	   OV7675_write_cmos_sensor(0x3a,0x0C);
	   OV7675_write_cmos_sensor(0x3D,0xC0);
	   OV7675_write_cmos_sensor(0x12,0x00);//YUV format
	   OV7675_write_cmos_sensor(0x15,0x42);
	  
	   OV7675_write_cmos_sensor(0xc1,0x7f);//BLC
	   
	   OV7675_write_cmos_sensor(0x17,0x13);//window
	   OV7675_write_cmos_sensor(0x18,0x01);
	   OV7675_write_cmos_sensor(0x32,0xbF);
	   OV7675_write_cmos_sensor(0x19,0x03);
	   OV7675_write_cmos_sensor(0x1a,0x7b);
	   OV7675_write_cmos_sensor(0x03,0x0a);
	   
	   OV7675_write_cmos_sensor(0x0c,0x00);
	   OV7675_write_cmos_sensor(0x3e,0x00);//scale 
	   OV7675_write_cmos_sensor(0x70,0x3a);
	   OV7675_write_cmos_sensor(0x71,0x35);
	   OV7675_write_cmos_sensor(0x72,0x11);
	   OV7675_write_cmos_sensor(0x73,0xf0);
	   OV7675_write_cmos_sensor(0xa2,0x02);//Pclk delay

	    OV7675_write_cmos_sensor(0x7a,0x24);
	   OV7675_write_cmos_sensor(0x7b,0x04);
	   OV7675_write_cmos_sensor(0x7c,0x07);
	   OV7675_write_cmos_sensor(0x7d,0x10);
	   OV7675_write_cmos_sensor(0x7e,0x28);
	   OV7675_write_cmos_sensor(0x7f,0x36);
	   OV7675_write_cmos_sensor(0x80,0x44);
	   OV7675_write_cmos_sensor(0x81,0x52);
	   OV7675_write_cmos_sensor(0x82,0x60);
	   OV7675_write_cmos_sensor(0x83,0x6c);
	   OV7675_write_cmos_sensor(0x84,0x78);
	   OV7675_write_cmos_sensor(0x85,0x8c);
	   OV7675_write_cmos_sensor(0x86,0x9e);
	   OV7675_write_cmos_sensor(0x87,0xbb);
	   OV7675_write_cmos_sensor(0x88,0xd2);
	   OV7675_write_cmos_sensor(0x89,0xe5);
	  
	   OV7675_write_cmos_sensor(0x13,0xe0);//3a enable
	   OV7675_write_cmos_sensor(0x00,0x00);//gain
	   OV7675_write_cmos_sensor(0x10,0x00);//exposue  value
	   OV7675_write_cmos_sensor(0x0d,0x40); //full window
	   OV7675_write_cmos_sensor(0x42,0x00);//AEC window
	   OV7675_write_cmos_sensor(0xb7,0x56); //reserved
	   OV7675_write_cmos_sensor(0x14,0x38); // auto gain ceiling
	   OV7675_write_cmos_sensor(0xa5,0x02);  // 50 banding step
	   OV7675_write_cmos_sensor(0xab,0x03);  // 60HZ banding step 
	   OV7675_write_cmos_sensor(0x24,0x54); //AEC and AGC operation region
	   OV7675_write_cmos_sensor(0x25,0x4c); 
	   OV7675_write_cmos_sensor(0x26,0x82);   
	   OV7675_write_cmos_sensor(0x9f,0x78);
	   OV7675_write_cmos_sensor(0xa0,0x68);
	   OV7675_write_cmos_sensor(0xa1,0x03);
	   OV7675_write_cmos_sensor(0xa6,0xD8);
	   OV7675_write_cmos_sensor(0xa7,0xD8);
	   OV7675_write_cmos_sensor(0xa8,0xf0);
	   OV7675_write_cmos_sensor(0xa9,0x90);
	   OV7675_write_cmos_sensor(0xaa,0x14);
	   OV7675_write_cmos_sensor(0x13,0xe5);//
	   
	   OV7675_write_cmos_sensor(0x0e,0x61);//reserved
	   OV7675_write_cmos_sensor(0x0f,0x4b);
	   OV7675_write_cmos_sensor(0x16,0x02);//reserved
	   OV7675_write_cmos_sensor(0x1e,0x07);//mirror and flip
	   OV7675_write_cmos_sensor(0x21,0x02);//reserved
	   OV7675_write_cmos_sensor(0x22,0x91);//reserved
	   OV7675_write_cmos_sensor(0x29,0x07);//reserved
	   OV7675_write_cmos_sensor(0x33,0x0b);
	   OV7675_write_cmos_sensor(0x35,0x0b);
	   OV7675_write_cmos_sensor(0x37,0x1d);//ADC control
	   OV7675_write_cmos_sensor(0x38,0x71);
	   OV7675_write_cmos_sensor(0x39,0x2a);
	   OV7675_write_cmos_sensor(0x3c,0x78);//HREF option
	   OV7675_write_cmos_sensor(0x4e,0x20);
	   OV7675_write_cmos_sensor(0x69,0x00);//pre gain control
	
	   OV7675_write_cmos_sensor(0x6b,0x0a);//PLL control
	   OV7675_write_cmos_sensor(0x74,0x10);//Digital gain control
	   OV7675_write_cmos_sensor(0x8d,0x4f);//reserved
	   OV7675_write_cmos_sensor(0x8e,0x00);
	   OV7675_write_cmos_sensor(0x8f,0x00);
	   OV7675_write_cmos_sensor(0x90,0x00);
	   OV7675_write_cmos_sensor(0x91,0x00);
	   OV7675_write_cmos_sensor(0x96,0x00);
	   OV7675_write_cmos_sensor(0x9a,0x80);
	   OV7675_write_cmos_sensor(0xb0,0x84);//reserved
	   OV7675_write_cmos_sensor(0xb1,0x0c);//ABLC
	   OV7675_write_cmos_sensor(0xb2,0x0e);
	   OV7675_write_cmos_sensor(0xb3,0x82);
	   OV7675_write_cmos_sensor(0xb8,0x0a);

       OV7675_write_cmos_sensor(0x43,0x0a);//reserved
	   OV7675_write_cmos_sensor(0x44,0xf2);
	   OV7675_write_cmos_sensor(0x45,0x40);
	   OV7675_write_cmos_sensor(0x46,0x5a);
	   OV7675_write_cmos_sensor(0x47,0x33);
	   OV7675_write_cmos_sensor(0x48,0x42);
	   OV7675_write_cmos_sensor(0x59,0xab);//AWB
	   OV7675_write_cmos_sensor(0x5a,0xad);
	   OV7675_write_cmos_sensor(0x5b,0xba);
	   OV7675_write_cmos_sensor(0x5c,0x6d);
	   OV7675_write_cmos_sensor(0x5d,0x54);
	   OV7675_write_cmos_sensor(0x5e,0x0e);
	   OV7675_write_cmos_sensor(0x6c,0x0a);
	   OV7675_write_cmos_sensor(0x6d,0x65);
	   OV7675_write_cmos_sensor(0x6e,0x11);
	   OV7675_write_cmos_sensor(0x6f,0x94);

	   OV7675_write_cmos_sensor(0x6a,0x40);//AWB G gain
	   OV7675_write_cmos_sensor(0x01,0x56);//AWB B gain
	   OV7675_write_cmos_sensor(0x02,0x44);//AWB R Gain
	   OV7675_write_cmos_sensor(0x13,0xe7);	
	 
	   OV7675_write_cmos_sensor(0x4f,0x73);//CCM
	   OV7675_write_cmos_sensor(0x50,0x73);
	   OV7675_write_cmos_sensor(0x51,0x00);
	   OV7675_write_cmos_sensor(0x52,0x1F);
	   OV7675_write_cmos_sensor(0x53,0x55);   
	   OV7675_write_cmos_sensor(0x54,0x73);   
	   OV7675_write_cmos_sensor(0x55,0x00);
	   OV7675_write_cmos_sensor(0x56,0x40);
	   OV7675_write_cmos_sensor(0x57,0x80);
	   OV7675_write_cmos_sensor(0x58,0x9e);
	   
	   OV7675_write_cmos_sensor(0x3f,0x02);//Eege
	   OV7675_write_cmos_sensor(0x75,0x23);
	   OV7675_write_cmos_sensor(0x76,0xe1);
	   OV7675_write_cmos_sensor(0x4c,0x00);//De noie strength
	   OV7675_write_cmos_sensor(0x77,0x01);//De noise offet
	   OV7675_write_cmos_sensor(0x3D,0xC2);//Satuation
	   OV7675_write_cmos_sensor(0x4b,0x09);
	   OV7675_write_cmos_sensor(0xc9,0x60);
	   OV7675_write_cmos_sensor(0x41,0x38);	
	   OV7675_write_cmos_sensor(0x56,0x40);//contrast control
	
	   OV7675_write_cmos_sensor(0x34,0x11);
	   OV7675_write_cmos_sensor(0x3b,0xaa);   
	   OV7675_write_cmos_sensor(0xa4,0x89);//insert dummy from 4x
	   OV7675_write_cmos_sensor(0x96,0x00);
	   OV7675_write_cmos_sensor(0x97,0x30);
	   OV7675_write_cmos_sensor(0x98,0x20);
	   OV7675_write_cmos_sensor(0x99,0x30);
	   OV7675_write_cmos_sensor(0x9a,0x84);
	   OV7675_write_cmos_sensor(0x9b,0x29);
	   OV7675_write_cmos_sensor(0x9c,0x03);
	   OV7675_write_cmos_sensor(0x9d,0x99);
	   OV7675_write_cmos_sensor(0x9e,0x7f);
	   OV7675_write_cmos_sensor(0x78,0x04);

	   OV7675_write_cmos_sensor(0x79,0x01);
	   OV7675_write_cmos_sensor(0xc8,0xf0);
	   OV7675_write_cmos_sensor(0x79,0x0f);
	   OV7675_write_cmos_sensor(0xc8,0x00);
	   OV7675_write_cmos_sensor(0x79,0x10);
	   OV7675_write_cmos_sensor(0xc8,0x7e);
	   OV7675_write_cmos_sensor(0x79,0x0a);
	   OV7675_write_cmos_sensor(0xc8,0x80);
	   OV7675_write_cmos_sensor(0x79,0x0b);
	   OV7675_write_cmos_sensor(0xc8,0x01);
	   OV7675_write_cmos_sensor(0x79,0x0c);
	   OV7675_write_cmos_sensor(0xc8,0x0f);
	   OV7675_write_cmos_sensor(0x79,0x0d);
	   OV7675_write_cmos_sensor(0xc8,0x20);
	   OV7675_write_cmos_sensor(0x79,0x09);
	   OV7675_write_cmos_sensor(0xc8,0x80);
	   OV7675_write_cmos_sensor(0x79,0x02);
	   OV7675_write_cmos_sensor(0xc8,0xc0);
	   OV7675_write_cmos_sensor(0x79,0x03);
	   OV7675_write_cmos_sensor(0xc8,0x40);
	   OV7675_write_cmos_sensor(0x79,0x05);
	   OV7675_write_cmos_sensor(0xc8,0x30);
	   OV7675_write_cmos_sensor(0x79,0x26);	
	   
	   OV7675_write_cmos_sensor(0x62,0x00);
	   OV7675_write_cmos_sensor(0x63,0x00);
	   OV7675_write_cmos_sensor(0x64,0x10);
	   OV7675_write_cmos_sensor(0x65,0x00);
	   OV7675_write_cmos_sensor(0x66,0x05);
	   OV7675_write_cmos_sensor(0x94,0x10);   
	   OV7675_write_cmos_sensor(0x95,0x13);  
	   OV7675_write_cmos_sensor(0xbb, 0xa1); //blc target	
	   
	   //gamma
	   OV7675_write_cmos_sensor(0x7a, 0x28); 
	   OV7675_write_cmos_sensor(0x7b, 0x04); 
	   OV7675_write_cmos_sensor(0x7c, 0x09); 
	   OV7675_write_cmos_sensor(0x7d, 0x16); 
	   OV7675_write_cmos_sensor(0x7e, 0x30); 
	   OV7675_write_cmos_sensor(0x7f, 0x3E); 
	   OV7675_write_cmos_sensor(0x80, 0x4B); 
	   OV7675_write_cmos_sensor(0x81, 0x59); 
	   OV7675_write_cmos_sensor(0x82, 0x67); 
	   OV7675_write_cmos_sensor(0x83, 0x72); 
	   OV7675_write_cmos_sensor(0x84, 0x7c); 
	   OV7675_write_cmos_sensor(0x85, 0x8e); 
	   OV7675_write_cmos_sensor(0x86, 0x9e); 
	   OV7675_write_cmos_sensor(0x87, 0xB6); 
	   OV7675_write_cmos_sensor(0x88, 0xcc); 
	   OV7675_write_cmos_sensor(0x89, 0xE2); 
	
	   //CCM
	   OV7675_write_cmos_sensor(0x4f, 0x7d);
	   OV7675_write_cmos_sensor(0x50, 0x81);
	   OV7675_write_cmos_sensor(0x51, 0x04);
	   OV7675_write_cmos_sensor(0x52, 0x23);
	   OV7675_write_cmos_sensor(0x53, 0x5a);
	   OV7675_write_cmos_sensor(0x54, 0x7d);
	   OV7675_write_cmos_sensor(0x58, 0x1a);
	   //awb 
	   OV7675_write_cmos_sensor(0x43, 0x0a);
	   OV7675_write_cmos_sensor(0x44, 0xf2);
	   OV7675_write_cmos_sensor(0x45, 0x31);
	   OV7675_write_cmos_sensor(0x46, 0x57);
	   OV7675_write_cmos_sensor(0x47, 0x37);
	   OV7675_write_cmos_sensor(0x48, 0x4c);
	   OV7675_write_cmos_sensor(0x59, 0x80);
	   OV7675_write_cmos_sensor(0x5a, 0xbb);
	   OV7675_write_cmos_sensor(0x5b, 0xbb);
	   OV7675_write_cmos_sensor(0x5c, 0x60);
	   OV7675_write_cmos_sensor(0x5d, 0x75);
	   OV7675_write_cmos_sensor(0x5e, 0x16);
	   OV7675_write_cmos_sensor(0x6a, 0x40);
	   OV7675_write_cmos_sensor(0x6c, 0x0a);
	   OV7675_write_cmos_sensor(0x6d, 0x65);
	   OV7675_write_cmos_sensor(0x6e, 0x11);
	   OV7675_write_cmos_sensor(0x6f, 0x96);
	   //lenc 
	   OV7675_write_cmos_sensor(0x62,0x18);
	   OV7675_write_cmos_sensor(0x63,0x10);
	   OV7675_write_cmos_sensor(0x64,0x0f);
	   OV7675_write_cmos_sensor(0x65,0x00);
	   OV7675_write_cmos_sensor(0x66,0x05);
	   OV7675_write_cmos_sensor(0x94,0x0d);
	   OV7675_write_cmos_sensor(0x95,0x11); //0x10   
	   //aec
	   OV7675_write_cmos_sensor(0x24, 0x5e);//0x68,0x5e(0x62)
	   OV7675_write_cmos_sensor(0x25, 0x54); //0x54(0x58)
	   OV7675_write_cmos_sensor(0x26, 0x93); //0x84	   
	   //Edge + Denoise
	    OV7675_write_cmos_sensor(0x41, 0x38);
        OV7675_write_cmos_sensor(0x75, 0x63);  //0x64
        OV7675_write_cmos_sensor(0x76, 0xe0); 
        OV7675_write_cmos_sensor(0x77, 0x07); 
	   
	   //UVAdjust
	   OV7675_write_cmos_sensor(0xc9, 0x30);  // to make more saturation 0x60
	   OV7675_write_cmos_sensor(0x61, 0x43);	

	   OV7675_write_cmos_sensor(0x4d,0xc0); // this will make add dummy line after active line, it is a little difference with OV7670_AE_LUTs
	   OV7675_write_cmos_sensor(0x13, 0xF7); // for mt6268 and mt6253, capture no need to change clock,so preview no need to enable ae
   
	  OV7675_write_cmos_sensor( 0x09 ,0x03);//output drive capability 
  
}
/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*	OV7675Open
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
static kal_uint32 OV7675Open(void)

{
	kal_uint16 sensor_id=0; 
	int retry = 10; 

    SENSORDB("OV7675Open_start \n");

//	OV7675_Sensor_Driver.i2c_clit.addr=OV7675_WRITE_ID;
//	OV7675_Sensor_Driver.i2c_clit = i2c_clit;
//    OV7675_Sensor_Driver.i2c_clit->addr = OV7675_WRITE_ID;

#if 0 
	OV7675_write_cmos_sensor(0x12, 0x80);
	mDELAY(10);
#endif 

	// check if sensor ID correct
	do {
    	    sensor_id=((OV7675_read_cmos_sensor(0x0A)<< 8)|OV7675_read_cmos_sensor(0x0B));
    	    if (sensor_id == OV7675_SENSOR_ID) {
                 break; 
    	    }
            SENSORDB("Read Sensor ID Fail = 0x%04x\n", sensor_id); 
    	    
    	    retry--; 
	}while (retry > 0); 
	
	if (sensor_id != OV7675_SENSOR_ID) {
	    return ERROR_SENSOR_CONNECT_FAIL;
	}
	 
    memset(&OV7675_Sensor_Driver, 0, sizeof(struct OV7675_Sensor_Struct)); 
	spin_lock(&ov7675_yuv_drv_lock);   
	OV7675_Sensor_Driver.MPEG4_encode_mode=KAL_FALSE;
	OV7675_Sensor_Driver.dummy_pixels=0;
	OV7675_Sensor_Driver.dummy_lines=0;
	OV7675_Sensor_Driver.extra_exposure_lines=0;
	OV7675_Sensor_Driver.exposure_lines=0;
	OV7675_Sensor_Driver.MODE_CAPTURE=KAL_FALSE;
		
	OV7675_Sensor_Driver.bNight_mode =KAL_FALSE; // to distinguish night mode or auto mode, default: auto mode setting
	OV7675_Sensor_Driver.bBanding_value = AE_FLICKER_MODE_50HZ; // to distinguish between 50HZ and 60HZ.
		
	OV7675_Sensor_Driver.fPV_PCLK = 26; //26000000;
	OV7675_Sensor_Driver.iPV_Pixels_Per_Line = 0;
	spin_unlock(&ov7675_yuv_drv_lock);

//	OV7675_set_isp_driving_current(1);
	// initail sequence write in
    OV7675_write_cmos_sensor(0x12, 0x80);
    mDELAY(10);
    OV7675_Sensor_Driver_Init();		
    SENSORDB("OV7675Open_end \n");
    
	return ERROR_NONE;
}   /* OV7675Open  */


/*************************************************************************
* FUNCTION
*	OV7675_GetSensorID
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
static kal_uint32 OV7675_GetSensorID(kal_uint32 *sensorID)

{
    int  retry = 3; 
    // check if sensor ID correct
    do {
        *sensorID=((OV7675_read_cmos_sensor(0x0A)<< 8)|OV7675_read_cmos_sensor(0x0B));
        if (*sensorID == OV7675_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != OV7675_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;    
}   /* OV7675Open  */

/*************************************************************************
* FUNCTION
*	OV7675Close
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
static kal_uint32 OV7675Close(void)
{
kal_uint8 tmp1;
    tmp1 = closed;
	CAMERA_CONTROL_FLOW(tmp1,closed++);

	return ERROR_NONE;
}   /* OV7675Close */


static void OV7675_HVMirror(ACDK_SENSOR_IMAGE_MIRROR_ENUM SensorImageMirror)
{
	volatile kal_uint32 temp_reg2=OV7675_read_cmos_sensor(0x1E), temp_reg1=(temp_reg2&0x0F);
	kal_uint32 iTemp;
        int retry = 3; 
	iTemp = OV7675_read_cmos_sensor(0x78) & 0xBF;	// clear 0x78[6] = 0
	switch (SensorImageMirror)
	{
		case IMAGE_NORMAL:
			while(temp_reg2 != (0x00|temp_reg1)){
				OV7675_write_cmos_sensor(0x1E,(0x00|temp_reg1));
				temp_reg2=OV7675_read_cmos_sensor(0x1E);
				OV7675_write_cmos_sensor(0x78, iTemp | 0x00); // 0x78[6] must be equal to 0x1E[5]
				retry --; 
                                if (retry < 0) {
                                   break;  
                                }
			};
			break;
		case IMAGE_H_MIRROR:			 
			while(temp_reg2 != (0x20|temp_reg1)){
				OV7675_write_cmos_sensor(0x1E,(0x20|temp_reg1));
				temp_reg2=OV7675_read_cmos_sensor(0x1E);
				retry --; 
                                if (retry < 0) {
                                   break;  
                                }
			};
			break;
		case IMAGE_V_MIRROR:			 
			while(temp_reg2 != (0x10|temp_reg1)){
				OV7675_write_cmos_sensor(0x1E,(0x10|temp_reg1));
				temp_reg2=OV7675_read_cmos_sensor(0x1E);
				retry --; 
                                if (retry < 0) {
                                   break;  
                                }
			};
			break;
		case IMAGE_HV_MIRROR:						 
			while(temp_reg2 != (0x30|temp_reg1)){
				OV7675_write_cmos_sensor(0x1E,(0x30|temp_reg1));
				temp_reg2=OV7675_read_cmos_sensor(0x1E);
				OV7675_write_cmos_sensor(0x78, iTemp | 0x40);	// 0x78[6] must be equal to 0x1E[5]
				retry --; 
                                if (retry < 0) {
                                   break;  
                                }
			};
			break;
	}


}
/*************************************************************************
* FUNCTION
* OV7675_Preview
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
static kal_uint32 OV7675_Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
	spin_lock(&ov7675_yuv_drv_lock);

	OV7675_Sensor_Driver.fPV_PCLK=26000000;
	OV7675_Sensor_Driver.MODE_CAPTURE=KAL_FALSE;
	spin_unlock(&ov7675_yuv_drv_lock);

	if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO){
		spin_lock(&ov7675_yuv_drv_lock);
		OV7675_Sensor_Driver.MPEG4_encode_mode = KAL_TRUE;  // MPEG4 Encode Mode
		spin_unlock(&ov7675_yuv_drv_lock);
	}else{
	    spin_lock(&ov7675_yuv_drv_lock);
		OV7675_Sensor_Driver.MPEG4_encode_mode = KAL_FALSE;
		spin_unlock(&ov7675_yuv_drv_lock);
	}


	OV7675_HVMirror(sensor_config_data->SensorImageMirror);
	spin_lock(&ov7675_yuv_drv_lock);

	OV7675_Sensor_Driver.dummy_pixels = 0;
	OV7675_Sensor_Driver.dummy_lines = 42;
	OV7675_Sensor_Driver.iPV_Pixels_Per_Line =VGA_PERIOD_PIXEL_NUMS+OV7675_Sensor_Driver.dummy_pixels;  
	spin_unlock(&ov7675_yuv_drv_lock);
	OV7675_Set_Dummy(OV7675_Sensor_Driver.dummy_pixels, OV7675_Sensor_Driver.dummy_lines);

	
	image_window->GrabStartX= IMAGE_SENSOR_VGA_INSERTED_PIXELS;
	image_window->GrabStartY= IMAGE_SENSOR_VGA_INSERTED_LINES;
	image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
	image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

	if(KAL_TRUE == OV7675_Sensor_Driver.bNight_mode) // for nd 128 noise,decrease color matrix
	{
		OV7675_write_cmos_sensor(0x4f, 0x7d);
		OV7675_write_cmos_sensor(0x50, 0x81);
		OV7675_write_cmos_sensor(0x51, 0x04);
		OV7675_write_cmos_sensor(0x52, 0x23);
		OV7675_write_cmos_sensor(0x53, 0x5a);
		OV7675_write_cmos_sensor(0x54, 0x7d);
		OV7675_write_cmos_sensor(0x58, 0x1a);
	}

	// copy sensor_config_data
	memcpy(&OV7675SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;

}   /*  OV7675_Preview   */

/*************************************************************************
* FUNCTION
*	OV7675_Capture
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
#if 0
static kal_uint32 OV7675_Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
	volatile kal_uint32 shutter=OV7675_Sensor_Driver.exposure_lines;
	//tatic float fCP_PCLK = 0;
	kal_uint32 fCP_PCLK = 0;
	kal_uint16 iCP_Pixels_Per_Line = 0;

	OV7675_Sensor_Driver.MODE_CAPTURE=KAL_TRUE;




	OV7675_Sensor_Driver.dummy_pixels=0;
	OV7675_Sensor_Driver.dummy_lines=0;  
	fCP_PCLK= OV7675_Sensor_Driver.fPV_PCLK;
	iCP_Pixels_Per_Line =VGA_PERIOD_PIXEL_NUMS+OV7675_Sensor_Driver.dummy_pixels;
	//shutter = shutter * (fCP_PCLK / OV7675_Sensor_Driver.fPV_PCLK) * OV7675_Sensor_Driver.iPV_Pixels_Per_Line / iCP_Pixels_Per_Line;
	shutter = shutter * (fCP_PCLK / OV7675_Sensor_Driver.fPV_PCLK) * OV7675_Sensor_Driver.iPV_Pixels_Per_Line / iCP_Pixels_Per_Line;
	if (shutter < 1) {
	  shutter = 1;
	}	  
	OV7675_Set_Dummy(OV7675_Sensor_Driver.dummy_pixels, OV7675_Sensor_Driver.dummy_lines);
	OV7675_Write_Shutter(shutter);	

/*	
	sensor_config_data->DefaultPclk = fCP_PCLK; 
	sensor_config_data->Pixels = iCP_Pixels_Per_Line;
	sensor_config_data->FrameLines =VGA_PERIOD_PIXEL_NUMS ;
	sensor_config_data->Lines = image_window->ExposureWindowHeight;    
	sensor_config_data->Shutter =shutter;		
*/	

	
	if(KAL_TRUE == OV7675_Sensor_Driver.bNight_mode)  // for nd128 noise
	{
		kal_uint8 gain = OV7675_read_cmos_sensor(0x00);
		if(gain == 0x7f){
			OV7675_write_cmos_sensor(0x4f,0x63);//matrix
			OV7675_write_cmos_sensor(0x50,0x68);
			OV7675_write_cmos_sensor(0x51,0x05);
			OV7675_write_cmos_sensor(0x52,0x16);
			OV7675_write_cmos_sensor(0x53,0x42);
			OV7675_write_cmos_sensor(0x54,0x57);
			OV7675_write_cmos_sensor(0x58,0x1a);
		}
	}
	
	// copy sensor_config_data
	memcpy(&OV7675SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	image_window->GrabStartX = IMAGE_SENSOR_VGA_INSERTED_PIXELS;
	image_window->GrabStartY = IMAGE_SENSOR_VGA_INSERTED_LINES;
	image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
	image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;  


	return ERROR_NONE;
}   /* OV7576_Capture() */
#endif

static kal_uint32 OV7675GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
kal_uint8 tmp1;
    tmp1 = res;
	CAMERA_CONTROL_FLOW(tmp1,res++);

	pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
	pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH;
	pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;
	return ERROR_NONE;
}	/* OV7675GetResolution() */

static kal_uint32 OV7675GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

#if 0
	pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE; //low active
	pSensorInfo->SensorResetDelayCount=5; 
#endif

	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
 	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
       pSensorInfo->SensorDriver3D = 0;   // the sensor driver is 2D

	
	pSensorInfo->SensorMasterClockSwitch = 0; 
      pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;   		

#if 0
	pSensorInfo->SensorInterruptDelayLines = 1;
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;
	
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;
#endif

	//CAMERA_CONTROL_FLOW(ScenarioId,ScenarioId);
      pSensorInfo->PreviewDelayFrame = 10; 
      pSensorInfo->VideoDelayFrame = 5; 	

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
			
			pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_INSERTED_PIXELS; 
			pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_INSERTED_LINES;			   
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:		
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;	
			
			pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_INSERTED_PIXELS; 
			pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_INSERTED_LINES;			   
		break;
		default:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			
			pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_INSERTED_PIXELS; 
			pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_INSERTED_LINES;			   
		break;
	}

	memcpy(pSensorConfigData, &OV7675SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	
	return ERROR_NONE;
}	/* OV7675GetInfo() */


static kal_uint32 OV7675Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	CAMERA_CONTROL_FLOW(ScenarioId,ScenarioId);

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			OV7675_Preview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			//OV7675_Capture(pImageWindow, pSensorConfigData);
			OV7675_Preview(pImageWindow, pSensorConfigData);
		break;
		default:
			return ERROR_INVALID_SCENARIO_ID;
	}
	return TRUE;
}	/* MT9P012Control() */



static BOOL OV7675_set_param_wb(UINT16 para)
{
	kal_uint8  temp_reg;

	if(OV7675_Sensor_Driver.u8Wb_value==para)
		return FALSE;

	spin_lock(&ov7675_yuv_drv_lock);
	OV7675_Sensor_Driver.u8Wb_value = para;
	spin_unlock(&ov7675_yuv_drv_lock);

	temp_reg=OV7675_read_cmos_sensor(0x13);
	switch (para)
	{
		case AWB_MODE_AUTO:
			OV7675_write_cmos_sensor(0x01,0x56);
			OV7675_write_cmos_sensor(0x02,0x44);
			OV7675_write_cmos_sensor(0x13,temp_reg|0x2);   // Enable AWB	
			break;
		case AWB_MODE_DAYLIGHT: //sunny
			OV7675_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB 
			OV7675_write_cmos_sensor(0x01, 0x56); 
			OV7675_write_cmos_sensor(0x02, 0x5C); 
			OV7675_write_cmos_sensor(0x6A, 0x42);
			break;
		case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
			OV7675_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB				
			OV7675_write_cmos_sensor(0x01, 0x58);//B gain
			OV7675_write_cmos_sensor(0x02, 0x60);//R gain
			OV7675_write_cmos_sensor(0x6A, 0x40);//G gain
			break;
		case AWB_MODE_INCANDESCENT: //office 
			OV7675_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB	
			OV7675_write_cmos_sensor(0x01, 0x9A); 
			OV7675_write_cmos_sensor(0x02, 0x40); 
			OV7675_write_cmos_sensor(0x6A, 0x48); 
			break;
		case AWB_MODE_TUNGSTEN: //home 
			OV7675_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB				
			OV7675_write_cmos_sensor(0x01, 0xB8);
			OV7675_write_cmos_sensor(0x02, 0x40);
			OV7675_write_cmos_sensor(0x6A, 0x4F);
			break;
		case AWB_MODE_FLUORESCENT: 
			OV7675_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB				
			OV7675_write_cmos_sensor(0x01, 0x8B);
			OV7675_write_cmos_sensor(0x02, 0x42);
			OV7675_write_cmos_sensor(0x6A, 0x40);
			break; 
		default:
			return FALSE;
	}

	return TRUE;
} /* OV7675_set_param_wb */


static BOOL OV7675_set_param_effect(UINT16 para)
{
	kal_uint32 ret = KAL_TRUE;

	if(para==OV7675_Sensor_Driver.u8Effect_value)
		return FALSE;

	spin_lock(&ov7675_yuv_drv_lock);
	OV7675_Sensor_Driver.u8Effect_value = para;
	spin_unlock(&ov7675_yuv_drv_lock);
	switch (para)
	{
		case MEFFECT_OFF:
			OV7675_write_cmos_sensor(0x3A,0x0C);		
			OV7675_write_cmos_sensor(0x67,0x80);		
			OV7675_write_cmos_sensor(0x68,0x80);
			OV7675_write_cmos_sensor(0x56,0x40);		
			break;
		case MEFFECT_SEPIA:
			OV7675_write_cmos_sensor(0x3A,0x1C);		
			OV7675_write_cmos_sensor(0x67,0x40);		
			OV7675_write_cmos_sensor(0x68,0xA0);
			OV7675_write_cmos_sensor(0x56,0x40);	
			break;
		case MEFFECT_NEGATIVE:
			OV7675_write_cmos_sensor(0x3A,0x2C);		
			OV7675_write_cmos_sensor(0x67,0x80);		
			OV7675_write_cmos_sensor(0x68,0x80);
			OV7675_write_cmos_sensor(0x56,0x40);
			break;
		case MEFFECT_SEPIAGREEN:
			OV7675_write_cmos_sensor(0x3A,0x1C);		
			OV7675_write_cmos_sensor(0x67,0x40);		
			OV7675_write_cmos_sensor(0x68,0x40);
			OV7675_write_cmos_sensor(0x56,0x40);	
			break;
		case MEFFECT_SEPIABLUE:
			OV7675_write_cmos_sensor(0x3A,0x1C);		
			OV7675_write_cmos_sensor(0x67,0xA0);		
			OV7675_write_cmos_sensor(0x68,0x40);
			OV7675_write_cmos_sensor(0x56,0x40);	
			break;
		case MEFFECT_MONO://CAM_EFFECT_ENC_GRAYSCALE: 
			OV7675_write_cmos_sensor(0x3A,0x1C);		
			OV7675_write_cmos_sensor(0x67,0x80);		
			OV7675_write_cmos_sensor(0x68,0x80);
			OV7675_write_cmos_sensor(0x56,0x40);		
			break;
		case MEFFECT_SOLARIZE://CAM_EFFECT_ENC_GRAYINV:
			OV7675_write_cmos_sensor(0x3A,0x2C);		
			OV7675_write_cmos_sensor(0x67,0x80);		
			OV7675_write_cmos_sensor(0x68,0x80);
			OV7675_write_cmos_sensor(0x56,0x40);
			break;
		case MEFFECT_AQUA://CAM_EFFECT_ENC_COPPERCARVING:
			OV7675_write_cmos_sensor(0x3A,0x1C);		
			OV7675_write_cmos_sensor(0x67,0x80);		
			OV7675_write_cmos_sensor(0x68,0xF0);
			OV7675_write_cmos_sensor(0x56,0x40);	
			break;
		case MEFFECT_POSTERIZE://CAM_EFFECT_ENC_BLUECARVING:
			OV7675_write_cmos_sensor(0x3A,0x1C);		
			OV7675_write_cmos_sensor(0x67,0xB0);		
			OV7675_write_cmos_sensor(0x68,0x60);	
			OV7675_write_cmos_sensor(0x56,0x40);		
			break;					

		default:
			ret = FALSE;
	}

	return ret;

} /* OV7675_set_param_effect */

static void OV7675_set_banding_for_50Hz(void)
{
    kal_uint8 banding;
	banding=OV7675_read_cmos_sensor(0x3b);
	OV7675_write_cmos_sensor(0x3b,banding|0x08); //select 9D as  banding filter value;	
	
	if (OV7675_Sensor_Driver.MPEG4_encode_mode == KAL_TRUE)
   {
	   OV7675_write_cmos_sensor(0x9D,0xa5); //PCLK:26M, Line_lenght:784
	  if(KAL_TRUE == OV7675_Sensor_Driver.bNight_mode)
	   {  
		     OV7675_write_cmos_sensor(0xA5, 0x05); //banding step, frame lenth: 1105
		}
	   else
	    {     
		   OV7675_write_cmos_sensor(0xA5, 0x02); //banding step, frame lengh:(510+42)
	    }
	}
   else 
	{
	    OV7675_write_cmos_sensor(0x9D,0xa5); //PCLK:26M, Line_lenght:784
		if(KAL_TRUE == OV7675_Sensor_Driver.bNight_mode)
		 {
		   OV7675_write_cmos_sensor(0xA5, 0x02); //banding step, frame lengh(510+42)
		  }
		else
		{
		   OV7675_write_cmos_sensor(0xA5, 0x02); //banding step, frame lengh(510+42)
		}
	}		
}


static void OV7675_set_banding_for_60Hz(void)
{   
    kal_uint8 banding;
	banding=OV7675_read_cmos_sensor(0x3b);
	OV7675_write_cmos_sensor(0x3b,banding&0xF7);	//seltct 0x9E as banding filter value	
			
	if (OV7675_Sensor_Driver.MPEG4_encode_mode == KAL_TRUE)
	{
	 OV7675_write_cmos_sensor(0x9E,0x8a); //PCLK:26M, Line_lenght:784
	  if(KAL_TRUE == OV7675_Sensor_Driver.bNight_mode)
	   {   		 
	        OV7675_write_cmos_sensor(0xAb, 0x07); //banding step, frame lenth: 1105
	  	}
	   else
	   {	   		 
		 OV7675_write_cmos_sensor(0xAb, 0x03); //banding step, frame lengh:510+42
	   }
	}
	else 
	{
	    OV7675_write_cmos_sensor(0x9E, 0x8a); //PCLK:24M, Line_lenght:784
		if(KAL_TRUE == OV7675_Sensor_Driver.bNight_mode)
		{
		  OV7675_write_cmos_sensor(0xAb, 0x03); //banding step, frame lengh:(510+42)
		}
		else
		{
		   OV7675_write_cmos_sensor(0xab, 0x03); //banding step, frame length:(510+42)
		}
	}
}

static BOOL OV7675_set_param_banding(UINT16 para)
{
	if(OV7675_Sensor_Driver.bBanding_value == para)
		return TRUE;
	spin_lock(&ov7675_yuv_drv_lock);
	OV7675_Sensor_Driver.bBanding_value = para;
	spin_unlock(&ov7675_yuv_drv_lock);
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			OV7675_set_banding_for_50Hz();
			break;
		case AE_FLICKER_MODE_60HZ:
			OV7675_set_banding_for_60Hz();
			break;
		default:
			return FALSE;
	}

	return TRUE;
} /* OV7675_set_param_banding */

static BOOL OV7675_set_param_exposure(UINT16 para)
{
	if(para == OV7675_Sensor_Driver.u8Ev_value)
		return FALSE;
	spin_lock(&ov7675_yuv_drv_lock);

	OV7675_Sensor_Driver.u8Ev_value = para;
	spin_unlock(&ov7675_yuv_drv_lock);

	switch (para)
	{
		case AE_EV_COMP_00:	// Disable EV compenate
			OV7675_write_cmos_sensor(0x55, 0x00);	
			break;
		case AE_EV_COMP_03:// EV compensate 0.3
			OV7675_write_cmos_sensor(0x55, 0x08);	
			break;
		case AE_EV_COMP_05:// EV compensate 0.5
			OV7675_write_cmos_sensor(0x55, 0x18);	
			break;
		case AE_EV_COMP_07:// EV compensate 0.7
			OV7675_write_cmos_sensor(0x55, 0x28);	
			break;
		case AE_EV_COMP_10:// EV compensate 1.0
			OV7675_write_cmos_sensor(0x55, 0x38);	
			break;
		case AE_EV_COMP_13:// EV compensate 1.3
			OV7675_write_cmos_sensor(0x55, 0x48);	
			break;
		case AE_EV_COMP_15:// EV compensate 1.5
			OV7675_write_cmos_sensor(0x55, 0x58);	
			break;
		case AE_EV_COMP_17:// EV compensate 1.7
			OV7675_write_cmos_sensor(0x55, 0x68);	
			break;
		case AE_EV_COMP_20:// EV compensate 2.0
			OV7675_write_cmos_sensor(0x55, 0x78);	
			break;
		case AE_EV_COMP_n03:// EV compensate -0.3	
			OV7675_write_cmos_sensor(0x55, 0x88);	
			break;
		case AE_EV_COMP_n05:// EV compensate -0.5
			OV7675_write_cmos_sensor(0x55, 0x98);	
			break;
		case AE_EV_COMP_n07:// EV compensate -0.7
			OV7675_write_cmos_sensor(0x55, 0xA8);	
			break;
		case AE_EV_COMP_n10:// EV compensate -1.0
			OV7675_write_cmos_sensor(0x55, 0xB8);	
			break;
		case AE_EV_COMP_n13: // EV compensate -1.3	
			OV7675_write_cmos_sensor(0x55, 0xC8);	
			break;
		case AE_EV_COMP_n15:// EV compensate -1.5
			OV7675_write_cmos_sensor(0x55, 0xD8);	
			break;
		case AE_EV_COMP_n17:// EV compensate -1.7
			OV7675_write_cmos_sensor(0x55, 0xE8);	
			break;
		case AE_EV_COMP_n20:// EV compensate -2.0
			OV7675_write_cmos_sensor(0x55, 0xF8);	
			break;
		default:
			return FALSE;
		}

	return TRUE;
} /* OV7675_set_param_exposure */

static kal_uint32 OV7675_YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
/*
CMD: AE Strob(4)->AE Flick(0) -> AF Mode(6) -> AF Metring(7) -> AE metring(1)
-> EV(3) -> AWB(5) -> ISO(2) -> AE Scene Mode(13) ->Brightness(1) -> Hue(9)
-> Saturation(10) -> Edge(8) -> Contrast(12) -> Scene Mode(14) -> Effect(15)

For Current: Banding->EV->WB->Effect
*/
    //printk("[OV7675_YUVSensorSetting], Cmd = 0x%x, Para = 0x%x\n", iCmd, iPara); 
	//CAMERA_CONTROL_FLOW(iCmd,iPara);

	switch (iCmd) {
		case FID_SCENE_MODE:
                //printk("\n\nOV7675YUVSensorSetting:para=%d\n\n",iPara);
			/*
		    if (iPara == SCENE_MODE_OFF){
		        OV7675_night_mode(FALSE); 
		    }else if (iPara == SCENE_MODE_NIGHTSCENE){
               OV7675_night_mode(TRUE); 
		    }	    
		    */
		    break; 
		case FID_AWB_MODE:
			OV7675_set_param_wb(iPara);
		break;
		case FID_COLOR_EFFECT:
			OV7675_set_param_effect(iPara);
		break;
		case FID_AE_EV:	
			OV7675_set_param_exposure(iPara);
		break;
		case FID_AE_FLICKER:
			OV7675_set_param_banding(iPara);
		break;
		default:
		break;
	}
	
	return TRUE;
}   /* OV7675_YUVSensorSetting */

static kal_uint32 OV7675_YUVSetVideoMode(UINT16 u2FrameRate)
{
    kal_uint8 temp = OV7675_read_cmos_sensor(0x3B);
	spin_lock(&ov7675_yuv_drv_lock);
    OV7675_Sensor_Driver.MPEG4_encode_mode = KAL_TRUE; 
	spin_unlock(&ov7675_yuv_drv_lock);

    if (u2FrameRate == 30)
    {
        OV7675_Set_Dummy(0,42); 	
        OV7675_write_cmos_sensor(0x3B,(temp&0x1F)); // fixed frame rate
        OV7675_write_cmos_sensor(0x2d, 0); // if not set to zero, maybe AE osillation
        OV7675_write_cmos_sensor(0x2e, 0);
        OV7675_write_cmos_sensor(0xc9, 0x30);//satuation control 					    
    }
    else if (u2FrameRate == 15)       
    {
        OV7675_Set_Dummy(0,595);	
        OV7675_write_cmos_sensor(0x3B,(temp&0x1F)); // fixed frame rate
        OV7675_write_cmos_sensor(0x2d, 0); // if not set to zero, maybe AE osillation
        OV7675_write_cmos_sensor(0x2e, 0);
        OV7675_write_cmos_sensor(0xc9, 0x30);  			                  
    }
    else 
    {
        printk("Wrong frame rate setting \n");
    }   
    
	printk("\n OV7675_YUVSetVideoMode:u2FrameRate=%d\n\n",u2FrameRate);
    return TRUE;
}

UINT32 OV7675SetSoftwarePWDNMode(kal_bool bEnable)
{
    SENSORDB("[OV7675SetSoftwarePWDNMode] Software Power down enable:%d\n", bEnable);
    
    if(bEnable) {   // enable software sleep mode   
	 OV7675_write_cmos_sensor(0x09, 0x10);
    } else {
        OV7675_write_cmos_sensor(0x09, 0x03);  
    }
    return TRUE;
}

/*************************************************************************
* FUNCTION
*    OV7675_get_size
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
static void OV7675_get_size(kal_uint16 *sensor_width, kal_uint16 *sensor_height)
{
  *sensor_width = IMAGE_SENSOR_FULL_WIDTH; /* must be 4:3 */
  *sensor_height = IMAGE_SENSOR_FULL_HEIGHT;
}

/*************************************************************************
* FUNCTION
*    OV7675_get_period
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
static void OV7675_get_period(kal_uint16 *pixel_number, kal_uint16 *line_number)
{
  *pixel_number = VGA_PERIOD_PIXEL_NUMS+OV7675_Sensor_Driver.dummy_pixels;
  *line_number = VGA_PERIOD_LINE_NUMS+OV7675_Sensor_Driver.dummy_lines;
}

/*************************************************************************
* FUNCTION
*    OV7675_feature_control
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
static kal_uint32 OV7675FeatureControl(MSDK_SENSOR_FEATURE_ENUM id, kal_uint8 *para, kal_uint32 *len)
{
	UINT32 *pFeatureData32=(UINT32 *) para;
	if((id!=3000)&&(id!=3004)&&(id!=3006)){
	    //CAMERA_CONTROL_FLOW(id,id);
	}

	switch (id)
	{
		case SENSOR_FEATURE_GET_RESOLUTION: /* no use */
			OV7675_get_size((kal_uint16 *)para, (kal_uint16 *)(para + sizeof(kal_uint16)));
			*len = sizeof(kal_uint32);
			break;
		case SENSOR_FEATURE_GET_PERIOD:
			OV7675_get_period((kal_uint16 *)para, (kal_uint16 *)(para + sizeof(kal_uint16)));
			*len = sizeof(kal_uint32);
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*(kal_uint32 *)para = OV7675_Sensor_Driver.fPV_PCLK;
			*len = sizeof(kal_uint32);
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE: 
			OV7675_night_mode((kal_bool)*(kal_uint16 *)para);
			break;
		case SENSOR_FEATURE_SET_GAIN:
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			OV7675_write_cmos_sensor(((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegAddr, ((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER: /* 10 */
			((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegData = OV7675_read_cmos_sensor(((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegAddr);
			break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
			memcpy(&OV7675_Sensor_Driver.eng.CCT, para, sizeof(OV7675_Sensor_Driver.eng.CCT));
			break;
		case SENSOR_FEATURE_GET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
		case SENSOR_FEATURE_GET_CONFIG_PARA: /* no use */
			break;
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
			break;
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
			break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
		case SENSOR_FEATURE_GET_GROUP_INFO: /* 20 */
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/*
		* get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
		* if EEPROM does not exist in camera module.
		*/
			*(kal_uint32 *)para = LENS_DRIVER_ID_DO_NOT_CARE;
			*len = sizeof(kal_uint32);
			break;
		case SENSOR_FEATURE_SET_YUV_CMD:
	//		OV7675_YUVSensorSetting((FEATURE_ID)(UINT32 *)para, (UINT32 *)(para+1));
			
			OV7675_YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
			break;
#if 0		    		
		case SENSOR_FEATURE_QUERY:
			OV7675_Query(pSensorFeatureInfo);
			*pFeatureParaLen = sizeof(MSDK_FEATURE_INFO_STRUCT);
			break;		
		case SENSOR_FEATURE_SET_YUV_CAPTURE_RAW_SUPPORT:
			/* update yuv capture raw support flag by *pFeatureData16 */
			break;		
#endif 			
		case SENSOR_FEATURE_SET_VIDEO_MODE:
			OV7675_YUVSetVideoMode(*para);
			break;
              case SENSOR_FEATURE_CHECK_SENSOR_ID:
                     OV7675_GetSensorID(pFeatureData32); 
                     break; 	
              case SENSOR_FEATURE_SET_SOFTWARE_PWDN:
                     OV7675SetSoftwarePWDNMode((BOOL)*pFeatureData32);        	        	
                     break;
		default:
			break;
	}
	return ERROR_NONE;
}




#if 0
image_sensor_func_struct image_sensor_driver_OV7675=
{
	OV7675Open,
	OV7675Close,
	OV7675GetResolution,
	OV7675GetInfo,
	OV7675Control,
	OV7675FeatureControl
};
void image_sensor_func_config(void)
{
	extern image_sensor_func_struct *image_sensor_driver;

	image_sensor_driver = &image_sensor_driver_OV7675;
}

#endif

SENSOR_FUNCTION_STRUCT	SensorFuncOV7675=
{
	OV7675Open,
	OV7675GetInfo,
	OV7675GetResolution,
	OV7675FeatureControl,
	OV7675Control,
	OV7675Close
};

UINT32 OV7675_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncOV7675;

	return ERROR_NONE;
}	/* SensorInit() */




