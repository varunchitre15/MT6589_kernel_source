/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.c
 *
 * Project:
 * --------

 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author:
 * -------

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
#include <linux/xlog.h>

#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/system.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"
   
#include "ov9740_mipi_yuv_Sensor.h"
#include "ov9740_mipi_yuv_Camera_Sensor_para.h"
#include "ov9740_mipi_yuv_CameraCustomized.h"

#define OV9740MIPI_DEBUG
#ifdef OV9740MIPI_DEBUG
#define SENSORDB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[OV9740MIPI]", fmt, ##arg)

#else
#define SENSORDB(x,...)
#endif

typedef struct
{
  UINT16  iSensorVersion;
  UINT16  iNightMode;
  UINT16  iWB;
  UINT16  iEffect;
  UINT16  iEV;
  UINT16  iBanding;
  UINT16  iMirror;
  UINT16  iFrameRate;
} OV9740MIPIStatus;

OV9740MIPIStatus OV9740MIPICurrentStatus;
static DEFINE_SPINLOCK(ov9740mipi_yuv_drv_lock);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
//static int sensor_id_fail = 0; 
static kal_uint32 zoom_factor = 0; 


kal_uint8 OV9740MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint8 get_byte=0;
	char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,OV9740MIPI_WRITE_ID);

	 SENSORDB("OV9740_read_cmos_sensor, addr:%x;get_byte:%x \n",addr,get_byte);
	
	return get_byte;
}

 inline void OV9740MIPI_write_cmos_sensor(u16 addr, u32 para)
{
   char puSendCmd[3] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
   iWriteRegI2C(puSendCmd , 3,OV9740MIPI_WRITE_ID);
    //SENSORDB("OV9740_write_cmos_sensor, addr:%x; para:%x \n",addr,para);
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
/* Global Valuable */

kal_bool OV9740MIPI_MPEG4_encode_mode = KAL_FALSE, OV9740MIPI_MJPEG_encode_mode = KAL_FALSE;
static kal_bool   OV9740MIPI_VEDIO_encode_mode = KAL_FALSE; 
static kal_bool   OV9740MIPI_sensor_cap_state = KAL_FALSE; 
static kal_uint32 OV9740MIPI_sensor_pclk=720;
static kal_bool   OV9740MIPI_AE_ENABLE = KAL_TRUE; 
MSDK_SENSOR_CONFIG_STRUCT OV9740MIPISensorConfigData;


/*************************************************************************
* FUNCTION
*	OV9740MIPIInitialPara
*
* DESCRIPTION
*	This function initialize the global status of  MT9V114
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
static void OV9740MIPIInitialPara(void)
{
  spin_lock(&ov9740mipi_yuv_drv_lock);
  OV9740MIPICurrentStatus.iNightMode = 0;
  OV9740MIPICurrentStatus.iWB = AWB_MODE_AUTO;
  OV9740MIPICurrentStatus.iEffect = MEFFECT_OFF;
  OV9740MIPICurrentStatus.iBanding = AE_FLICKER_MODE_50HZ;
  OV9740MIPICurrentStatus.iEV = AE_EV_COMP_n03;
  OV9740MIPICurrentStatus.iMirror = IMAGE_NORMAL;
  OV9740MIPICurrentStatus.iFrameRate = 0;
  spin_unlock(&ov9740mipi_yuv_drv_lock);
}


void OV9740MIPI_set_mirror(kal_uint8 image_mirror)
{

		if(OV9740MIPICurrentStatus.iMirror == image_mirror)
		  return;
		//rotation 180 case
		switch (image_mirror)  
		{
			case IMAGE_NORMAL:
				OV9740MIPI_write_cmos_sensor(0x0101, 0x03); 
				break;
			case IMAGE_H_MIRROR:
				OV9740MIPI_write_cmos_sensor(0x0101, 0x02);  //mirror : bit1	
				break;
			case IMAGE_V_MIRROR:
				OV9740MIPI_write_cmos_sensor(0x0101, 0x01);  //mirror : bit 0 
				break;
			case IMAGE_HV_MIRROR:
				OV9740MIPI_write_cmos_sensor(0x0101, 0x00);
				break;
			default:
				ASSERT(0);
				break;
		} 

		/* //general case.
		switch (image_mirror)  
		{
			case IMAGE_NORMAL:
				OV9740MIPI_write_cmos_sensor(0x0101, 0x00);
				break;
			case IMAGE_H_MIRROR:
				OV9740MIPI_write_cmos_sensor(0x0101, 0x01);  //mirror : bit 0 
				break;
			case IMAGE_V_MIRROR:
				OV9740MIPI_write_cmos_sensor(0x0101, 0x02);  //mirror : bit1	
				break;
			case IMAGE_HV_MIRROR:
				OV9740MIPI_write_cmos_sensor(0x0101, 0x03); 	
				break;
			default:
				ASSERT(0);
				break;
		} */
		spin_lock(&ov9740mipi_yuv_drv_lock);
		OV9740MIPICurrentStatus.iMirror = image_mirror;
		spin_unlock(&ov9740mipi_yuv_drv_lock);

}





/*****************************************************************************
 * FUNCTION
 *  OV9740MIPI_set_dummy
 * DESCRIPTION
 *
 * PARAMETERS
 *  pixels      [IN]
 *  lines       [IN]
 * RETURNS
 *  void
 *****************************************************************************/
void OV9740MIPI_set_dummy(kal_uint16 dummy_pixels, kal_uint16 dummy_lines)
{
		/****************************************************
		  * Adjust the extra H-Blanking & V-Blanking.
		  *****************************************************/
		OV9740MIPI_write_cmos_sensor(0x0340, (dummy_lines>>8)  ); 	// Extra V-Blanking
		OV9740MIPI_write_cmos_sensor(0x0341, (dummy_lines&0xFF)); 	// Extra V-Blanking
		OV9740MIPI_write_cmos_sensor(0x0342, (dummy_pixels>>8)	);	// Extra HBlanking
		OV9740MIPI_write_cmos_sensor(0x0343, (dummy_pixels&0xFF));	// Extra H-Blanking


}   /* OV9740MIPI_set_dummy */

/*****************************************************************************
 * FUNCTION
 *  OV9740MIPI_Initialize_Setting
 * DESCRIPTION
 *    OV9740 DVP 1280x720 Full at 30FPS in YUV format
 *    24Mhz input, system clock 76Mhz
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/
void OV9740MIPI_Initialize_Setting(void)
{
	OV9740MIPI_write_cmos_sensor(0x0103,0x01);              // Software RESET
	  // Orientation
	OV9740MIPI_write_cmos_sensor(0x0101,0x02);	// Orientation
	OV9740MIPI_write_cmos_sensor(0x5004,0x11);	// Orientation

	 //OV9740MIPI_write_cmos_sensor(0x0101,0x01);  // Orientation
	// OV9740MIPI_write_cmos_sensor(0x5004,0x10);  // Orientation
	
	OV9740MIPI_write_cmos_sensor(0x3104,0x20);              // PLL mode control
	OV9740MIPI_write_cmos_sensor(0x0305,0x03);                             // PLL control
	OV9740MIPI_write_cmos_sensor(0x0307,0x4c);                             // PLL control
	OV9740MIPI_write_cmos_sensor(0x0303,0x01);                             // PLL control
	OV9740MIPI_write_cmos_sensor(0x0301,0x08);                             // PLL control
	OV9740MIPI_write_cmos_sensor(0x3010,0x01);                             // PLL control
	OV9740MIPI_write_cmos_sensor(0x0340,0x03);                             // VTS
	OV9740MIPI_write_cmos_sensor(0x0341,0x07);                             // VTS
	OV9740MIPI_write_cmos_sensor(0x0342,0x06);                             // HTS
	OV9740MIPI_write_cmos_sensor(0x0343,0x62);                             // HTS
	OV9740MIPI_write_cmos_sensor(0x0344,0x00);                             // X start
	OV9740MIPI_write_cmos_sensor(0x0345,0x08);                             // X start
	OV9740MIPI_write_cmos_sensor(0x0346,0x00);                             // Y start
	OV9740MIPI_write_cmos_sensor(0x0347,0x05);                             // Y start
	OV9740MIPI_write_cmos_sensor(0x0348,0x05);                             // X end
	OV9740MIPI_write_cmos_sensor(0x0349,0x0c);                             // X end
	OV9740MIPI_write_cmos_sensor(0x034a,0x02);                             // Y end
	OV9740MIPI_write_cmos_sensor(0x034b,0xd8);                              // Y end
	OV9740MIPI_write_cmos_sensor(0x034c,0x05);                             // H output size
	OV9740MIPI_write_cmos_sensor(0x034d,0x00);                             // H output size
	OV9740MIPI_write_cmos_sensor(0x034e,0x02);                             // V output size
	OV9740MIPI_write_cmos_sensor(0x034f,0xd0);                             // V output size
	OV9740MIPI_write_cmos_sensor(0x3002,0x00);                             // IO control
	OV9740MIPI_write_cmos_sensor(0x3004,0x00);                             // IO control
	OV9740MIPI_write_cmos_sensor(0x3005,0x00);                             // IO control
	OV9740MIPI_write_cmos_sensor(0x3012,0x70);                             // MIPI control
	OV9740MIPI_write_cmos_sensor(0x3013,0x60);                             // MIPI control
	OV9740MIPI_write_cmos_sensor(0x3014,0x01);                             // MIPI control
	OV9740MIPI_write_cmos_sensor(0x301f,0x43);                             // MIPI control
	OV9740MIPI_write_cmos_sensor(0x3026,0x00);                             // Output select
	OV9740MIPI_write_cmos_sensor(0x3027,0x00);                             // Output select
	OV9740MIPI_write_cmos_sensor(0x3601,0x40);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3602,0x16);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3603,0xaa);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3604,0x0c);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3610,0xa1);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3612,0x24);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3620,0x66);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3621,0xc0);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3622,0x9f);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3630,0xd2);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3631,0x5e);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3632,0x27);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3633,0x50);                             // Analog control
	OV9740MIPI_write_cmos_sensor(0x3703,0x42);                             // Sensor control 
	OV9740MIPI_write_cmos_sensor(0x3704,0x10);                             // Sensor control 
	OV9740MIPI_write_cmos_sensor(0x3705,0x45);
	OV9740MIPI_write_cmos_sensor(0x3707,0x11);   
	OV9740MIPI_write_cmos_sensor(0x3817,0x94);                             // Internal timing control
	OV9740MIPI_write_cmos_sensor(0x3819,0x6e);                             // Internal timing control
	OV9740MIPI_write_cmos_sensor(0x3831,0x40);   
	OV9740MIPI_write_cmos_sensor(0x3833,0x04);  
	OV9740MIPI_write_cmos_sensor(0x3835,0x04);    
	OV9740MIPI_write_cmos_sensor(0x3837,0x01);
	OV9740MIPI_write_cmos_sensor(0x3503,0x10);                             // AEC/AGC control
	OV9740MIPI_write_cmos_sensor(0x3a18,0x01);                             // Gain ceiling
	OV9740MIPI_write_cmos_sensor(0x3a19,0xB5);                             // Gain ceiling
	OV9740MIPI_write_cmos_sensor(0x3a1a,0x05);                             // Max diff
	OV9740MIPI_write_cmos_sensor(0x3a11,0x90);                              // High threshold
	OV9740MIPI_write_cmos_sensor(0x3a1b,0x4a);                              // WPT 2 
	OV9740MIPI_write_cmos_sensor(0x3a0f,0x48);                              // WPT  
	OV9740MIPI_write_cmos_sensor(0x3a10,0x44);                              // BPT  
	OV9740MIPI_write_cmos_sensor(0x3a1e,0x42);                              // BPT 2 
	OV9740MIPI_write_cmos_sensor(0x3a1f,0x22);                          // Banding filter
	OV9740MIPI_write_cmos_sensor(0x3a08,0x00);                             // 50Hz banding step
	OV9740MIPI_write_cmos_sensor(0x3a09,0xe8);                             // 50Hz banding step	
	OV9740MIPI_write_cmos_sensor(0x3a0e,0x03);                             // 50Hz banding Max
	OV9740MIPI_write_cmos_sensor(0x3a14,0x15);                             // 50Hz Max exposure
	OV9740MIPI_write_cmos_sensor(0x3a15,0xc6);                             // 50Hz Max exposure
	OV9740MIPI_write_cmos_sensor(0x3a0a,0x00);                             // 60Hz banding step
	OV9740MIPI_write_cmos_sensor(0x3a0b,0xc0);                             // 60Hz banding step
	OV9740MIPI_write_cmos_sensor(0x3a0d,0x04);                             // 60Hz banding Max
	OV9740MIPI_write_cmos_sensor(0x3a02,0x18);                             // 60Hz Max exposure
	OV9740MIPI_write_cmos_sensor(0x3a03,0x20);                          //50/60 detection
	OV9740MIPI_write_cmos_sensor(0x3c0a,0x9c);                             // Number of samples
	OV9740MIPI_write_cmos_sensor(0x3c0b,0x3f);    
	OV9740MIPI_write_cmos_sensor(0x4002,0x45);                             // BLC auto enable
	OV9740MIPI_write_cmos_sensor(0x4005,0x18);                           // VFIFO control
	OV9740MIPI_write_cmos_sensor(0x4601,0x16);    
	OV9740MIPI_write_cmos_sensor(0x460e,0x82);	
	OV9740MIPI_write_cmos_sensor(0x4702,0x04);     
	OV9740MIPI_write_cmos_sensor(0x4704,0x00);    
	OV9740MIPI_write_cmos_sensor(0x4706,0x08);                           // MIPI control
	OV9740MIPI_write_cmos_sensor(0x4800,0x44);                             // MIPI control
	OV9740MIPI_write_cmos_sensor(0x4801,0x0f);
	OV9740MIPI_write_cmos_sensor(0x4803,0x05);
	OV9740MIPI_write_cmos_sensor(0x4805,0x10);                  // MIPI control
	OV9740MIPI_write_cmos_sensor(0x4837,0x20);                 // ISP control
	OV9740MIPI_write_cmos_sensor(0x5000,0xff);                             // [7] LC [6] Gamma [3] DNS [2] BPC [1] WPC [0] CIP
	OV9740MIPI_write_cmos_sensor(0x5001,0xff);                             // [7] SDE [6] UV adjust [4] scale [3] contrast [2] UV average [1] CMX [0] AWB
	OV9740MIPI_write_cmos_sensor(0x5003,0xff);                    //AWB
	OV9740MIPI_write_cmos_sensor(0x5180,0xf0);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5181,0x00);                               //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5182,0x41);                              //AWB setting 
	OV9740MIPI_write_cmos_sensor(0x5183,0x42);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5184,0x80);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5185,0x68);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5186,0x93);                              //AWB setting 
	OV9740MIPI_write_cmos_sensor(0x5187,0xa8);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5188,0x17);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5189,0x45);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x518a,0x27);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x518b,0x41);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x518c,0x2d);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x518d,0xf0);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x518e,0x10);                               //AWB setting
	OV9740MIPI_write_cmos_sensor(0x518f,0xff);                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5190,0x0 );                              //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5191,0xff);                              //AWB setting 
	OV9740MIPI_write_cmos_sensor(0x5192,0x00);                               //AWB setting
	OV9740MIPI_write_cmos_sensor(0x5193,0xff);                              //AWB setting 
	OV9740MIPI_write_cmos_sensor(0x5194,0x00);                   // DNS
	OV9740MIPI_write_cmos_sensor(0x529a,0x02);                             //DNS setting
	OV9740MIPI_write_cmos_sensor(0x529b,0x08);                             //DNS setting
	OV9740MIPI_write_cmos_sensor(0x529c,0x0a);                             //DNS setting
	OV9740MIPI_write_cmos_sensor(0x529d,0x10);                             //DNS setting
	OV9740MIPI_write_cmos_sensor(0x529e,0x10);                              //DNS setting
	OV9740MIPI_write_cmos_sensor(0x529f,0x28);                                //DNS setting
	OV9740MIPI_write_cmos_sensor(0x52a0,0x32);                              //DNS setting
	OV9740MIPI_write_cmos_sensor(0x52a2,0x00);                             //DNS setting 
	OV9740MIPI_write_cmos_sensor(0x52a3,0x02);                             //DNS setting 
	OV9740MIPI_write_cmos_sensor(0x52a4,0x00);                             //DNS setting 
	OV9740MIPI_write_cmos_sensor(0x52a5,0x04);                             //DNS setting  
	OV9740MIPI_write_cmos_sensor(0x52a6,0x00);                             //DNS setting  
	OV9740MIPI_write_cmos_sensor(0x52a7,0x08);                             //DNS setting  
	OV9740MIPI_write_cmos_sensor(0x52a8,0x00);                             //DNS setting  
	OV9740MIPI_write_cmos_sensor(0x52a9,0x10);                               //DNS setting
	OV9740MIPI_write_cmos_sensor(0x52aa,0x00);                             //DNS setting  
	OV9740MIPI_write_cmos_sensor(0x52ab,0x38);                               //DNS setting
	OV9740MIPI_write_cmos_sensor(0x52ac,0x00);                             //DNS setting  
	OV9740MIPI_write_cmos_sensor(0x52ad,0x3c);                              //DNS setting 
	OV9740MIPI_write_cmos_sensor(0x52ae,0x00);                             //DNS setting   
	OV9740MIPI_write_cmos_sensor(0x52af,0x4c);                   //CIP
	OV9740MIPI_write_cmos_sensor(0x530d,0x06);                   //CMX
	OV9740MIPI_write_cmos_sensor(0x5380,0x01);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x5381,0x00);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x5382,0x00);                              //CMX setting   
	OV9740MIPI_write_cmos_sensor(0x5383,0x0d);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x5384,0x00);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x5385,0x2f);                              //CMX setting   
	OV9740MIPI_write_cmos_sensor(0x5386,0x00);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x5387,0x00);                              //CMX setting   
	OV9740MIPI_write_cmos_sensor(0x5388,0x00);                              //CMX setting   
	OV9740MIPI_write_cmos_sensor(0x5389,0xd3);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x538a,0x00);                              //CMX setting   
	OV9740MIPI_write_cmos_sensor(0x538b,0x0f);                              //CMX setting   
	OV9740MIPI_write_cmos_sensor(0x538c,0x00);                              //CMX setting   
	OV9740MIPI_write_cmos_sensor(0x538d,0x00);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x538e,0x00);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x538f,0x32);                              //CMX setting   
	OV9740MIPI_write_cmos_sensor(0x5390,0x00);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x5391,0x94);                              //CMX setting    
	OV9740MIPI_write_cmos_sensor(0x5392,0x00);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x5393,0xa4);                              //CMX setting  
	OV9740MIPI_write_cmos_sensor(0x5394,0x18);                   // Contrast
	OV9740MIPI_write_cmos_sensor(0x5401,0x2c);                             // Contrast setting
	OV9740MIPI_write_cmos_sensor(0x5403,0x28);                             // Contrast setting
	OV9740MIPI_write_cmos_sensor(0x5404,0x06);                             // Contrast setting	
	OV9740MIPI_write_cmos_sensor(0x5405,0xe0);                   //Y Gamma
	OV9740MIPI_write_cmos_sensor(0x5480,0x04);                              //Y Gamma setting  
	OV9740MIPI_write_cmos_sensor(0x5481,0x12);                              //Y Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5482,0x27);                              //Y Gamma setting  
	OV9740MIPI_write_cmos_sensor(0x5483,0x49);                              //Y Gamma setting  
	OV9740MIPI_write_cmos_sensor(0x5484,0x57);                              //Y Gamma setting  
	OV9740MIPI_write_cmos_sensor(0x5485,0x66);                              //Y Gamma setting  
	OV9740MIPI_write_cmos_sensor(0x5486,0x75);                              //Y Gamma setting  
	OV9740MIPI_write_cmos_sensor(0x5487,0x81);                              //Y Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5488,0x8c);                              //Y Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5489,0x95);                              //Y Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x548a,0xa5);                              //Y Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x548b,0xb2);                              //Y Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x548c,0xc8);                              //Y Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x548d,0xd9);                              //Y Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x548e,0xec);                   //UV Gamma
	OV9740MIPI_write_cmos_sensor(0x5490,0x01);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5491,0xc0);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5492,0x03);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5493,0x00);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5494,0x03);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5495,0xe0);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5496,0x03);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5497,0x10);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5498,0x02);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x5499,0xac);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x549a,0x02);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x549b,0x75);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x549c,0x02);                                //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x549d,0x44);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x549e,0x02);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x549f,0x20);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a0,0x02);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a1,0x07);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a2,0x01);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a3,0xec);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a4,0x01);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a5,0xc0);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a6,0x01);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a7,0x9b);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a8,0x01);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54a9,0x63);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54aa,0x01);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54ab,0x2b);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54ac,0x01);                               //UV Gamma setting 
	OV9740MIPI_write_cmos_sensor(0x54ad,0x22);                   // UV adjust
	OV9740MIPI_write_cmos_sensor(0x5501,0x1c);                               //UV adjust setting 
	OV9740MIPI_write_cmos_sensor(0x5502,0x00);                                //UV adjust setting 
	OV9740MIPI_write_cmos_sensor(0x5503,0x40);                               //UV adjust setting 
	OV9740MIPI_write_cmos_sensor(0x5504,0x00);                               //UV adjust setting 
	OV9740MIPI_write_cmos_sensor(0x5505,0x80);                   // Lens correction
	OV9740MIPI_write_cmos_sensor(0x5800,0x1c);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5801,0x16);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5802,0x15);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5803,0x16);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5804,0x18);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5805,0x1a);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5806,0x0c);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5807,0x0a);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5808,0x08);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5809,0x08);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x580a,0x0a);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x580b,0x0b);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x580c,0x05);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x580d,0x02);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x580e,0x00);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x580f,0x00);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5810,0x02);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5811,0x05);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5812,0x04);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5813,0x01);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5814,0x00);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5815,0x00);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5816,0x02);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5817,0x03);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5818,0x0a);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5819,0x07);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x581a,0x05);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x581b,0x05);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x581c,0x08);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x581d,0x0b);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x581e,0x15);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x581f,0x14);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5820,0x14);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5821,0x13);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5822,0x17);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5823,0x16);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5824,0x46);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5825,0x4c);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5826,0x6c);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5827,0x4c);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5828,0x80);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5829,0x2e);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x582a,0x48);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x582b,0x46);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x582c,0x2a);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x582d,0x68);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x582e,0x08);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x582f,0x26);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5830,0x44);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5831,0x46);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5832,0x62);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5833,0x0c);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5834,0x28);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5835,0x46);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5836,0x28);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5837,0x88);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5838,0x0e);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5839,0x0e);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x583a,0x2c);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x583b,0x2e);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x583c,0x46);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x583d,0xca);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x583e,0xf0);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5842,0x02);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5843,0x5e);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5844,0x04);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5845,0x32);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5846,0x03);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5847,0x29);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5848,0x02);                             // Lens correction setting
	OV9740MIPI_write_cmos_sensor(0x5849,0xcc);                   // Start streaming
	OV9740MIPI_write_cmos_sensor(0x0100,0x01);                             // start streaming

}

/*****************************************************************************
 * FUNCTION
 *  OV9740MIPI_PV_Mode
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/
void OV9740MIPI_PV_Mode(void)
{
//
}

/*****************************************************************************
 * FUNCTION
 *  OV9740MIPI_CAP_Mode
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/

void OV9740MIPI_CAP_Mode(void)
{
//
}

static void OV9740MIPI_set_AE_mode(kal_bool AE_enable)
{
     if(AE_enable==KAL_TRUE)
     {
     	OV9740MIPI_write_cmos_sensor(0x3503, 0x00);  
     }
     else
     {
     	OV9740MIPI_write_cmos_sensor(0x3503, 0x03);  
     }
}


/*************************************************************************
* FUNCTION
*	OV9740MIPI_night_mode
*
* DESCRIPTION
*	This function night mode of OV9740MIPI.
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
void OV9740MIPI_night_mode(kal_bool enable)
{
    //if(OV9740MIPICurrentStatus.iNightMode == enable)
    //    return;

	if (OV9740MIPI_sensor_cap_state == KAL_TRUE)
		return ;	

	if (enable)
	{
		if (OV9740MIPI_MPEG4_encode_mode == KAL_TRUE)
		{
			OV9740MIPI_write_cmos_sensor(0x0303,0x2); 
			OV9740MIPI_write_cmos_sensor(0x3A00,0x78); 
			OV9740MIPI_write_cmos_sensor(0x3a14,0x02);//15);	// 50Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a15,0xf0);//c6);	// 50Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a02,0x02);//18);	// 60Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a03,0xf0);//20);	// 60Hz Max exposure

		}
		else
		{	
			OV9740MIPI_write_cmos_sensor(0x0303,0x1); 
			OV9740MIPI_write_cmos_sensor(0x3A00,0x7C); 
			OV9740MIPI_write_cmos_sensor(0x3a14,0x04);  // 50Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a15,0xB0);  // 50Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a02,0x4);  // 60Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a03,0xB0);  // 60Hz Max exposure
		}	
	}
	else
	{
		if (OV9740MIPI_MPEG4_encode_mode == KAL_TRUE)
		{
			OV9740MIPI_write_cmos_sensor(0x0303,0x01); 
			OV9740MIPI_write_cmos_sensor(0x3A00,0x78); 
			OV9740MIPI_write_cmos_sensor(0x3a14,0x02);	// 50Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a15,0xf0);	// 50Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a02,0x02);	// 60Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a03,0xf0);	// 60Hz Max exposure
		}
		else
		{
			OV9740MIPI_write_cmos_sensor(0x0303,0x01); 
			OV9740MIPI_write_cmos_sensor(0x3A00,0x78); 
			OV9740MIPI_write_cmos_sensor(0x3a14,0x02);	// 50Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a15,0xf0);	// 50Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a02,0x02);	// 60Hz Max exposure
			OV9740MIPI_write_cmos_sensor(0x3a03,0xf0);	// 60Hz Max exposure
		}	
	}

	spin_lock(&ov9740mipi_yuv_drv_lock);
    OV9740MIPICurrentStatus.iNightMode = enable;
	spin_unlock(&ov9740mipi_yuv_drv_lock);
}	/* OV9740MIPI_night_mode */

/*************************************************************************
* FUNCTION
*	OV9740MIPI_GetSensorID
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
static kal_uint32 OV9740MIPI_GetSensorID(kal_uint32 *sensorID)
{
    //volatile signed char i;
	kal_uint16 sensor_id=0,sensor_id_2=0;

     // check if sensor ID correct

	 sensor_id  =OV9740MIPI_read_cmos_sensor(0x0000);
	 sensor_id_2=OV9740MIPI_read_cmos_sensor(0x0001);
	 sensor_id = (sensor_id<<8)+sensor_id_2;
	 SENSORDB("Sensor Read OV9740MIPI ID 0x%x  \r\n", (unsigned int)sensor_id);
	 
	 *sensorID=sensor_id;
	if (sensor_id != OV9740MIPI_SENSOR_ID)
	{
	    *sensorID=0xFFFFFFFF;
	    SENSORDB("Sensor Read ByeBye \r\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}
    return ERROR_NONE;    
}  


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*	OV9740MIPIOpen
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
UINT32 OV9740MIPIOpen(void)
{
	//volatile signed char i;
	kal_uint16 sensor_id=0,sensor_id_2=0;
	
	sensor_id  =OV9740MIPI_read_cmos_sensor(0x0000);
	sensor_id_2=OV9740MIPI_read_cmos_sensor(0x0001);
	sensor_id = (sensor_id<<8)+sensor_id_2;
	SENSORDB("Sensor Read OV9740MIPI ID %x \r\n",(unsigned int)sensor_id);

	if (sensor_id != OV9740MIPI_SENSOR_ID)
	{
	    SENSORDB("Sensor Read ByeBye \r\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}

    OV9740MIPIInitialPara(); 
	
	OV9740MIPI_Initialize_Setting();
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	OV9740MIPIClose
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
UINT32 OV9740MIPIClose(void)
{
	return ERROR_NONE;
}	/* OV9740MIPIClose() */

/*************************************************************************
* FUNCTION
*	OV9740MIPIPreview
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
UINT32 OV9740MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	spin_lock(&ov9740mipi_yuv_drv_lock);
	OV9740MIPI_sensor_cap_state = KAL_FALSE;
	
	if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
	{
		OV9740MIPI_MPEG4_encode_mode = KAL_TRUE;		
		OV9740MIPI_MJPEG_encode_mode = KAL_FALSE;
	}
	else
	{
		OV9740MIPI_MPEG4_encode_mode = KAL_FALSE;		
		OV9740MIPI_MJPEG_encode_mode = KAL_FALSE;
		
	}
	spin_unlock(&ov9740mipi_yuv_drv_lock);
	
	OV9740MIPI_PV_Mode();
	OV9740MIPI_night_mode(OV9740MIPICurrentStatus.iNightMode);
	OV9740MIPI_set_mirror(sensor_config_data->SensorImageMirror);
	
	//just add for porting,please delete this when release
	//OV9740MIPI_set_mirror(IMAGE_HV_MIRROR);

    image_window->ExposureWindowWidth = OV9740MIPI_IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight = OV9740MIPI_IMAGE_SENSOR_PV_HEIGHT;
	
	// copy sensor_config_data
	memcpy(&OV9740MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
  	return ERROR_NONE;
}	/* OV9740MIPIPreview() */

UINT32 OV9740MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
     //kal_uint32 pv_integration_time = 0;       // Uinit - us
     //kal_uint32 cap_integration_time = 0;
     //kal_uint16 PV_line_len = 0;
     //kal_uint16 CAP_line_len = 0;

	spin_lock(&ov9740mipi_yuv_drv_lock);
	OV9740MIPI_sensor_cap_state = KAL_TRUE;
	spin_unlock(&ov9740mipi_yuv_drv_lock);
               
    OV9740MIPI_CAP_Mode();         

    image_window->GrabStartX = OV9740MIPI_IMAGE_SENSOR_FULL_INSERTED_PIXELS;
    image_window->GrabStartY = OV9740MIPI_IMAGE_SENSOR_FULL_INSERTED_LINES;
    image_window->ExposureWindowWidth= OV9740MIPI_IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = OV9740MIPI_IMAGE_SENSOR_FULL_HEIGHT;

     // copy sensor_config_data
     memcpy(&OV9740MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
     return ERROR_NONE;
}        /* OV9740MIPICapture() */

UINT32 OV9740MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=OV9740MIPI_IMAGE_SENSOR_FULL_WIDTH;  //modify by yanxu
	pSensorResolution->SensorFullHeight=OV9740MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	pSensorResolution->SensorPreviewWidth=OV9740MIPI_IMAGE_SENSOR_PV_WIDTH; 
	pSensorResolution->SensorPreviewHeight=OV9740MIPI_IMAGE_SENSOR_PV_HEIGHT;
	pSensorResolution->SensorVideoWidth=OV9740MIPI_IMAGE_SENSOR_PV_WIDTH; 
	pSensorResolution->SensorVideoHeight=OV9740MIPI_IMAGE_SENSOR_PV_HEIGHT;
	return ERROR_NONE;
}	/* OV9740MIPIGetResolution() */

UINT32 OV9740MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	#if defined(MT6575)
    switch(ScenarioId)
    {
        
    	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 pSensorInfo->SensorPreviewResolutionX=OV9740MIPI_IMAGE_SENSOR_FULL_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=OV9740MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			 pSensorInfo->SensorCameraPreviewFrameRate=15;
			 break;
		
		default:
	#else
	{
	#endif
	
			 pSensorInfo->SensorPreviewResolutionX=OV9740MIPI_IMAGE_SENSOR_PV_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=OV9740MIPI_IMAGE_SENSOR_PV_HEIGHT;
			 pSensorInfo->SensorCameraPreviewFrameRate=30;
    }
	pSensorInfo->SensorFullResolutionX=OV9740MIPI_IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=OV9740MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
	
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;

	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 1;
	
	#ifdef MIPI_INTERFACE
   		pSensorInfo->SensroInterfaceType        = SENSOR_INTERFACE_TYPE_MIPI;
   	#else
   		pSensorInfo->SensroInterfaceType		= SENSOR_INTERFACE_TYPE_PARALLEL;
   	#endif
	
	pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;//ISP_DRIVING_4MA;   	
	pSensorInfo->CaptureDelayFrame = 3; 
	pSensorInfo->PreviewDelayFrame = 3; 
	pSensorInfo->VideoDelayFrame = 4; 
	
	switch (ScenarioId) 
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pSensorInfo->SensorClockFreq=24;
			
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			
			pSensorInfo->SensorDataLatchCount= 2;
			
			pSensorInfo->SensorGrabStartX = 2; 
			pSensorInfo->SensorGrabStartY = 2; 
#ifdef MIPI_INTERFACE
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
#endif
		break;

		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX = 2; 
			pSensorInfo->SensorGrabStartY = 2; 					
			#ifdef MIPI_INTERFACE
	            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
	            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
		        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
		        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
	            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
	            pSensorInfo->SensorPacketECCOrder = 1;
	        	#endif			
		break;
				
		default:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = 2; 
			pSensorInfo->SensorGrabStartY = 2; 					
		break;
	}
	memcpy(pSensorConfigData, &OV9740MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* OV9740MIPIGetInfo() */


UINT32 OV9740MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			OV9740MIPIPreview(pImageWindow, pSensorConfigData);
		break;
		
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			OV9740MIPICapture(pImageWindow, pSensorConfigData);
		break;
		
		#if defined(MT6575)
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			   OV9740MIPICapture(pImageWindow, pSensorConfigData);
			break;
		#endif
		
        default:
            return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* OV9740MIPIControl() */

/* [TC] YUV sensor */	
#if WINMO_USE
void OV9740MIPIQuery(PMSDK_FEATURE_INFO_STRUCT pSensorFeatureInfo)
{
	MSDK_FEATURE_TYPE_RANGE_STRUCT *pFeatureRange;
	MSDK_FEATURE_TYPE_MULTI_SELECTION_STRUCT *pFeatureMultiSelection;
	switch (pSensorFeatureInfo->FeatureId)
	{
		case ISP_FEATURE_DSC_MODE:
			pSensorFeatureInfo->FeatureType = MSDK_FEATURE_TYPE_MULTI_SELECTION;
			pSensorFeatureInfo->FeatureSupported = (UINT8)(MSDK_SET_GET_FEATURE_SUPPORTED|MSDK_QUERY_CAMERA_SUPPORTED);
			pFeatureMultiSelection = (PMSDK_FEATURE_TYPE_MULTI_SELECTION_STRUCT)(&pSensorFeatureInfo->FeatureInformation.FeatureMultiSelection);
			pFeatureMultiSelection->TotalSelection = CAM_NO_OF_SCENE_MODE_MAX-2;
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
				CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_GRAYINV)|
				CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_SEPIABLUE)|
				CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_SKETCH)|
				CAMERA_FEATURE_SUPPORT(CAM_EFFECT_ENC_EMBOSSMENT)|
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
			pFeatureMultiSelection->TotalSelection = 2;
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

BOOL OV9740MIPI_set_param_wb(UINT16 para)
{

	if(OV9740MIPICurrentStatus.iWB == para)
		return TRUE;
	SENSORDB("[Enter]OV9740MIPI set_param_wb func:para = %d\n",para);

	switch (para)
	{
		case AWB_MODE_AUTO:
			OV9740MIPI_write_cmos_sensor(0x3406, 0x00); //bit[0]:AWB Auto:0 menual:1
			break;

		case AWB_MODE_CLOUDY_DAYLIGHT:	
			//======================================================================
			//	MWB : Cloudy_D65										 
			//======================================================================
			OV9740MIPI_write_cmos_sensor(0x3406, 0x01); //bit[0]:AWB Auto:0 menual:1
			OV9740MIPI_write_cmos_sensor(0x3400, 0x05);
			OV9740MIPI_write_cmos_sensor(0x3401, 0xC0);
			OV9740MIPI_write_cmos_sensor(0x3402, 0x04);
			OV9740MIPI_write_cmos_sensor(0x3403, 0x00);	
			OV9740MIPI_write_cmos_sensor(0x3404, 0x05);			
			OV9740MIPI_write_cmos_sensor(0x3405, 0x60);
			break;

		case AWB_MODE_DAYLIGHT:
			//==============================================
			//	MWB : sun&daylight_D50						
			//==============================================
			OV9740MIPI_write_cmos_sensor(0x3406, 0x01); //bit[0]:AWB Auto:0 menual:1
			OV9740MIPI_write_cmos_sensor(0x3400, 0x06);
			OV9740MIPI_write_cmos_sensor(0x3401, 0x00);
			OV9740MIPI_write_cmos_sensor(0x3402, 0x04);
			OV9740MIPI_write_cmos_sensor(0x3403, 0x00);	
			OV9740MIPI_write_cmos_sensor(0x3404, 0x05);			
			OV9740MIPI_write_cmos_sensor(0x3405, 0x80);

			break;

		case AWB_MODE_INCANDESCENT:
			//==============================================									   
			//	MWB : Incand_Tungsten						
			//==============================================
			OV9740MIPI_write_cmos_sensor(0x3406, 0x01); //bit[0]:AWB Auto:0 menual:1
			OV9740MIPI_write_cmos_sensor(0x3400, 0x04);
			OV9740MIPI_write_cmos_sensor(0x3401, 0x00);
			OV9740MIPI_write_cmos_sensor(0x3402, 0x04);
			OV9740MIPI_write_cmos_sensor(0x3403, 0x00);	
			OV9740MIPI_write_cmos_sensor(0x3404, 0x09);			
			OV9740MIPI_write_cmos_sensor(0x3405, 0xa0);	
			break;

		case AWB_MODE_FLUORESCENT:
			//==================================================================
			//	MWB : Florescent_TL84							  
			//==================================================================
			OV9740MIPI_write_cmos_sensor(0x3406, 0x01); //bit[0]:AWB Auto:0 menual:1
			OV9740MIPI_write_cmos_sensor(0x3400, 0x04);
			OV9740MIPI_write_cmos_sensor(0x3401, 0x20);
			OV9740MIPI_write_cmos_sensor(0x3402, 0x04);
			OV9740MIPI_write_cmos_sensor(0x3403, 0x00);	
			OV9740MIPI_write_cmos_sensor(0x3404, 0x08);			
			OV9740MIPI_write_cmos_sensor(0x3405, 0xB0);

			break;

		case AWB_MODE_TUNGSTEN:	
			OV9740MIPI_write_cmos_sensor(0x3406, 0x01); //bit[0]:AWB Auto:0 menual:1
			OV9740MIPI_write_cmos_sensor(0x3400, 0x04);
			OV9740MIPI_write_cmos_sensor(0x3401, 0x00);
			OV9740MIPI_write_cmos_sensor(0x3402, 0x04);
			OV9740MIPI_write_cmos_sensor(0x3403, 0x00);	
			OV9740MIPI_write_cmos_sensor(0x3404, 0x0B);			
			OV9740MIPI_write_cmos_sensor(0x3405, 0x80);

			break;
		default:
			return KAL_FALSE;	

		}
		spin_lock(&ov9740mipi_yuv_drv_lock);
	    OV9740MIPICurrentStatus.iWB = para;
		spin_unlock(&ov9740mipi_yuv_drv_lock);
return TRUE;
}

BOOL OV9740MIPI_set_param_effect(UINT16 para)
{
	/*----------------------------------------------------------------*/
	   /* Local Variables												 */
	   /*----------------------------------------------------------------*/
	   kal_uint32 ret = KAL_TRUE;
	   kal_uint8  SDE_1, SDE_2;
	   /*----------------------------------------------------------------*/
	   /* Code Body 													 */
	   /*----------------------------------------------------------------*/
	   if(OV9740MIPICurrentStatus.iEffect == para)
		  return TRUE;
	   SENSORDB("[Enter]ov9740mipi_yuv set_param_effect func:para = %d\n",para);

	   SDE_1=OV9740MIPI_read_cmos_sensor(0x5001);
	   SDE_2=OV9740MIPI_read_cmos_sensor(0x5580);
	   SDE_1&=0x7F;
	   SDE_2&=0x87;
	   OV9740MIPI_write_cmos_sensor(0x5001,SDE_1);  // Normal,	 SDE off
	   OV9740MIPI_write_cmos_sensor(0x5580,SDE_2);  
	   OV9740MIPI_write_cmos_sensor(0x5583,0x40); // SEPIA			   
	   OV9740MIPI_write_cmos_sensor(0x5584,0x40); // SEPIA

	   switch (para)
	   {
		   case MEFFECT_OFF:
			   break;
		   case MEFFECT_MONO:
			   OV9740MIPI_write_cmos_sensor(0x5001,(SDE_1|0x80));  
			   OV9740MIPI_write_cmos_sensor(0x5580,(SDE_2|0x20));  // Monochrome (Black & White)
			   break;
		   case MEFFECT_SEPIA:
			   OV9740MIPI_write_cmos_sensor(0x5001,(SDE_1|0x80)); // SDE, UV adj en,
			   OV9740MIPI_write_cmos_sensor(0x5580,(SDE_2|0x18)); // SDE manual UV en
			   OV9740MIPI_write_cmos_sensor(0x5583,0xA0); // SEPIA			   
			   OV9740MIPI_write_cmos_sensor(0x5584,0x40); // SEPIA
			   break;
		   case MEFFECT_SEPIABLUE:
			   OV9740MIPI_write_cmos_sensor(0x5001,(SDE_1|0x80)); // SDE, UV adj en,
			   OV9740MIPI_write_cmos_sensor(0x5580,(SDE_2|0x18)); // SDE manual UV en
			   OV9740MIPI_write_cmos_sensor(0x5583,0x40); // SEPIA			   
			   OV9740MIPI_write_cmos_sensor(0x5584,0xA0); // SEPIA
			   break;
		   case MEFFECT_NEGATIVE:
			   OV9740MIPI_write_cmos_sensor(0x5001,(SDE_1|0x80));  
			   OV9740MIPI_write_cmos_sensor(0x5580,(SDE_2|0x40)); // Negative Mono
			   break;
		   default:
			   ret = KAL_FALSE;
			   break;
	   }
	   
	   spin_lock(&ov9740mipi_yuv_drv_lock);
	   OV9740MIPICurrentStatus.iEffect = para;
	   spin_unlock(&ov9740mipi_yuv_drv_lock);
	   return ret;
} 


BOOL OV9740MIPI_set_param_banding(UINT16 para)
{
	kal_uint8 m_banding_auto;
	kal_uint8 m_banding_sel;  

	if(OV9740MIPICurrentStatus.iBanding == para)
		return TRUE;

	m_banding_auto  =OV9740MIPI_read_cmos_sensor(0x3C01);
	m_banding_sel   =OV9740MIPI_read_cmos_sensor(0x3C0C);

	m_banding_auto = m_banding_auto & 0x7F;
	m_banding_sel = m_banding_sel   & 0xFA;

    switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			OV9740MIPI_write_cmos_sensor(0x3C01, (m_banding_auto|0x80) );
			OV9740MIPI_write_cmos_sensor(0x3C0C, (m_banding_sel |   1) );
			break;
		case AE_FLICKER_MODE_60HZ:
			OV9740MIPI_write_cmos_sensor(0x3C01, (m_banding_auto|0x80) );
			OV9740MIPI_write_cmos_sensor(0x3C0C,  m_banding_sel        );
			break;
		default:

			OV9740MIPI_write_cmos_sensor(0x3C01,  m_banding_auto     );
			OV9740MIPI_write_cmos_sensor(0x3C0C, (m_banding_sel | 4) );
			return FALSE;
	}
	spin_lock(&ov9740mipi_yuv_drv_lock);
    OV9740MIPICurrentStatus.iBanding = para;
	spin_unlock(&ov9740mipi_yuv_drv_lock);
	return TRUE;
} /* OV9740MIPI_set_param_banding */

BOOL OV9740MIPI_set_param_exposure(UINT16 para)
{

	if(OV9740MIPICurrentStatus.iEV == para)
		return TRUE;
	
	SENSORDB("[Enter]ov9740mipi_yuv set_param_exposure func:para = %d\n",para);
	  
	OV9740MIPI_write_cmos_sensor(0x0028, 0x7000);	
    switch (para)
	{	
		case AE_EV_COMP_n10://EV -1
			OV9740MIPI_write_cmos_sensor(0x3a11, 0x90-0x12); // High threshold
			OV9740MIPI_write_cmos_sensor(0x3a1b, 0x4a-0x12); // WPT 2 
			OV9740MIPI_write_cmos_sensor(0x3a0f, 0x48-0x12); // WPT  
			OV9740MIPI_write_cmos_sensor(0x3a10, 0x44-0x12); // BPT  
			OV9740MIPI_write_cmos_sensor(0x3a1e, 0x42-0x12); // BPT 2 
			OV9740MIPI_write_cmos_sensor(0x3a1f, 0x22-0x12); // Low threshold 
			break;
		case AE_EV_COMP_00:		  //EV 0		   
			OV9740MIPI_write_cmos_sensor(0x3a11, 0x90);	// High threshold
			OV9740MIPI_write_cmos_sensor(0x3a1b, 0x4a);	// WPT 2 
			OV9740MIPI_write_cmos_sensor(0x3a0f, 0x48);	// WPT	
			OV9740MIPI_write_cmos_sensor(0x3a10, 0x44);	// BPT	
			OV9740MIPI_write_cmos_sensor(0x3a1e, 0x42);	// BPT 2 
			OV9740MIPI_write_cmos_sensor(0x3a1f, 0x22);	// Low threshold 
			break;
		case AE_EV_COMP_10:			 //EV +1		
			OV9740MIPI_write_cmos_sensor(0x3a11, 0x90+0x12); // High threshold
			OV9740MIPI_write_cmos_sensor(0x3a1b, 0x4a+0x12); // WPT 2 
			OV9740MIPI_write_cmos_sensor(0x3a0f, 0x48+0x12); // WPT  
			OV9740MIPI_write_cmos_sensor(0x3a10, 0x44+0x12); // BPT  
			OV9740MIPI_write_cmos_sensor(0x3a1e, 0x42+0x12); // BPT 2 
			OV9740MIPI_write_cmos_sensor(0x3a1f, 0x22+0x12); // Low threshold 
			break;
			
		case AE_EV_COMP_n13:					
		case AE_EV_COMP_n07:				   
		case AE_EV_COMP_n03:				   
		case AE_EV_COMP_03: 			
		case AE_EV_COMP_07: 
		case AE_EV_COMP_13:
			break;
			
		default:			
			return FALSE;
	}
	spin_lock(&ov9740mipi_yuv_drv_lock);
	OV9740MIPICurrentStatus.iEV = para;
	spin_unlock(&ov9740mipi_yuv_drv_lock);
	return TRUE;

}/* OV9740MIPI_set_param_exposure */


UINT32 OV9740MIPIYUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
	switch (iCmd) {
	case FID_SCENE_MODE:
	    if (iPara == SCENE_MODE_OFF)
	    {
	        OV9740MIPI_night_mode(0); 
	    }

         else if (iPara == SCENE_MODE_NIGHTSCENE)			
	    {
            OV9740MIPI_night_mode(1); 
	    }	    
	    break; 	    
	case FID_AWB_MODE:
         OV9740MIPI_set_param_wb(iPara);
	break;
	case FID_COLOR_EFFECT:
         OV9740MIPI_set_param_effect(iPara);
	break;
	case FID_AE_EV:  	    
         OV9740MIPI_set_param_exposure(iPara);
	break;
	case FID_AE_FLICKER:
         OV9740MIPI_set_param_banding(iPara);
	break;
    case FID_AE_SCENE_MODE:  
		spin_lock(&ov9740mipi_yuv_drv_lock);
      if (iPara == AE_MODE_OFF) {
          OV9740MIPI_AE_ENABLE = KAL_FALSE; 
      }
      else {
          OV9740MIPI_AE_ENABLE = KAL_TRUE; 
      }
	  spin_unlock(&ov9740mipi_yuv_drv_lock);
      OV9740MIPI_set_AE_mode(OV9740MIPI_AE_ENABLE);
    break; 
	case FID_ZOOM_FACTOR:
		spin_lock(&ov9740mipi_yuv_drv_lock);
	    zoom_factor = iPara; 
		spin_unlock(&ov9740mipi_yuv_drv_lock);
	break; 
	default:
	break;
	}
	return ERROR_NONE;
}   /* OV9740MIPIYUVSensorSetting */


UINT32 OV9740MIPIYUVSetVideoMode(UINT16 u2FrameRate)
{

    if(OV9740MIPICurrentStatus.iFrameRate == u2FrameRate)
      return ERROR_NONE;

	spin_lock(&ov9740mipi_yuv_drv_lock);
    OV9740MIPI_VEDIO_encode_mode = KAL_TRUE; 
	OV9740MIPI_MPEG4_encode_mode = KAL_TRUE;
	spin_unlock(&ov9740mipi_yuv_drv_lock);
	

	OV9740MIPI_write_cmos_sensor(0x3A00,0x78); 
	OV9740MIPI_write_cmos_sensor(0x3a14,0x02);//15);	// 50Hz Max exposure
	OV9740MIPI_write_cmos_sensor(0x3a15,0xf0);//c6);	// 50Hz Max exposure
	OV9740MIPI_write_cmos_sensor(0x3a02,0x02);//18);	// 60Hz Max exposure
	OV9740MIPI_write_cmos_sensor(0x3a03,0xf0);//20);	// 60Hz Max exposure

    if(20<=u2FrameRate && u2FrameRate<=30) //fix 30
    {		
		OV9740MIPI_write_cmos_sensor(0x0303,0x01);	// PLL control
    }
    else if(5<=u2FrameRate && u2FrameRate<20 )// fix 15
    {
		OV9740MIPI_write_cmos_sensor(0x0303,0x02);	// PLL control
    }
    else 
    {
        printk("Wrong Frame Rate \n"); 
    }
    return ERROR_NONE;
}


kal_uint16 OV9740MIPIReadShutter(void)
{
   kal_uint16 temp_msb=0x0000,temp_lsb=0x0000;

   temp_msb=OV9740MIPI_read_cmos_sensor(0x0202);
   temp_msb = (temp_msb<<8)&0xff00;
   temp_lsb=OV9740MIPI_read_cmos_sensor(0x0203);
   temp_lsb = temp_lsb&0x00ff;
   
   return (temp_msb|temp_lsb);
}
kal_uint16 OV9740MIPIReadGain(void)
{
	kal_uint16 temp_msb=0x0000,temp_lsb=0x0000;
	
	temp_msb=OV9740MIPI_read_cmos_sensor(0x0204);
	temp_msb = (temp_msb<<8)&0xff00;
	temp_lsb=OV9740MIPI_read_cmos_sensor(0x0205);
	temp_lsb = temp_lsb&0x00ff;
	
	return (temp_msb|temp_lsb);

}
kal_uint16 OV9740MIPIReadAwbRGain(void)
{
	kal_uint16 temp_msb=0x0000,temp_lsb=0x0000;
	temp_msb = OV9740MIPI_read_cmos_sensor(0x3400);
	temp_msb = (temp_msb<<8)&0xff00;
	temp_lsb = OV9740MIPI_read_cmos_sensor(0x3401);
	temp_lsb = temp_lsb&0x00ff;

	return (temp_msb|temp_lsb);

}
kal_uint16 OV9740MIPIReadAwbBGain(void)
{
	kal_uint16 temp_msb=0x0000,temp_lsb=0x0000;
	temp_msb = OV9740MIPI_read_cmos_sensor(0x3404);
	temp_msb = (temp_msb<<8)&0xff00;
	temp_lsb = OV9740MIPI_read_cmos_sensor(0x3405);
	temp_lsb = temp_lsb&0x00ff;
	return (temp_msb|temp_lsb);

}
#if defined(MT6575)

/*************************************************************************
* FUNCTION
*    OV9740MIPIGetEvAwbRef
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV9740MIPIGetEvAwbRef(UINT32 pSensorAEAWBRefStruct/*PSENSOR_AE_AWB_REF_STRUCT Ref*/)
{
    PSENSOR_AE_AWB_REF_STRUCT Ref = (PSENSOR_AE_AWB_REF_STRUCT)pSensorAEAWBRefStruct;
    SENSORDB("OV9740MIPIGetEvAwbRef ref = 0x%x \n", Ref);
    	
	Ref->SensorAERef.AeRefLV05Shutter = 5422;
    Ref->SensorAERef.AeRefLV05Gain = 478; /* 128 base */
    Ref->SensorAERef.AeRefLV13Shutter = 80;
    Ref->SensorAERef.AeRefLV13Gain = 128; /*  128 base */
    Ref->SensorAwbGainRef.AwbRefD65Rgain = 186; /* 128 base */
    Ref->SensorAwbGainRef.AwbRefD65Bgain = 158; /* 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFRgain = 196; /* 1.25x, 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFBgain = 278; /* 1.28125x, 128 base */
}
/*************************************************************************
* FUNCTION
*    OV9740MIPIGetCurAeAwbInfo
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV9740MIPIGetCurAeAwbInfo(UINT32 pSensorAEAWBCurStruct/*PSENSOR_AE_AWB_CUR_STRUCT Info*/)
{
    PSENSOR_AE_AWB_CUR_STRUCT Info = (PSENSOR_AE_AWB_CUR_STRUCT)pSensorAEAWBCurStruct;
    SENSORDB("OV9740MIPIGetCurAeAwbInfo Info = 0x%x \n", Info);

    Info->SensorAECur.AeCurShutter = OV9740MIPIReadShutter();
    Info->SensorAECur.AeCurGain = OV9740MIPIReadGain() * 2; /* 128 base */
    
    Info->SensorAwbGainCur.AwbCurRgain = OV9740MIPIReadAwbRGain()<< 1; /* 128 base */
    
    Info->SensorAwbGainCur.AwbCurBgain = OV9740MIPIReadAwbBGain()<< 1; /* 128 base */
}

#endif

void OV9740MIPIGetAFMaxNumFocusAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("OV9740MIPIGetAFMaxNumFocusAreas, *pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);

}

void OV9740MIPIGetAFMaxNumMeteringAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("OV9740MIPIGetAFMaxNumMeteringAreas,*pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);

}

void OV9740MIPIGetAEAWBLock(UINT32 *pAElockRet32,UINT32 *pAWBlockRet32)
{
    *pAElockRet32 = 1;
	*pAWBlockRet32 = 1;
    SENSORDB("OV9740MIPIGetAEAWBLock,AE=%d ,AWB=%d\n,",*pAElockRet32,*pAWBlockRet32);
}


void OV9740MIPIGetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = AE_ISO_100;
    pExifInfo->AWBMode = OV9740MIPICurrentStatus.iWB;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = AE_ISO_100;
}

UINT32 OV9740MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	SENSORDB("OV9740MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = OV9740MIPI_sensor_pclk/10;
			lineLength = OV9740MIPI_SXGA_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV9740MIPI_SXGA_PERIOD_LINE_NUMS;
			OV9740MIPI_set_dummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = OV9740MIPI_sensor_pclk/10;
			lineLength = OV9740MIPI_SXGA_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV9740MIPI_SXGA_PERIOD_LINE_NUMS;
			OV9740MIPI_set_dummy(0, dummyLine);			
			break;			
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = OV9740MIPI_sensor_pclk/10;
			lineLength = OV9740MIPI_SXGA_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV9740MIPI_SXGA_PERIOD_LINE_NUMS;
			OV9740MIPI_set_dummy(0, dummyLine);			
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			break;		
		default:
			break;
	}	
	return ERROR_NONE;
}


UINT32 OV9740MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 300;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = 300;
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = 300;
			break;		
		default:
			break;
	}

	return ERROR_NONE;
}

UINT32 OV9740MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	//PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;
#if WINMO_USE	
	PMSDK_FEATURE_INFO_STRUCT pSensorFeatureInfo=(PMSDK_FEATURE_INFO_STRUCT) pFeaturePara;
#endif 


	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=OV9740MIPI_IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=OV9740MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = OV9740MIPI_sensor_pclk/10;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			OV9740MIPI_night_mode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
		case SENSOR_FEATURE_SET_REGISTER:
			OV9740MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = OV9740MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &OV9740MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
		case SENSOR_FEATURE_SET_YUV_CMD:
			//OV9740MIPIYUVSensorSetting((MSDK_ISP_FEATURE_ENUM)*pFeatureData16, *(pFeatureData16+1));
			OV9740MIPIYUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		break;
		break;
#if WINMO_USE		
		case SENSOR_FEATURE_QUERY:
			OV9740MIPIQuery(pSensorFeatureInfo);
			*pFeatureParaLen = sizeof(MSDK_FEATURE_INFO_STRUCT);
		break;		
		case SENSOR_FEATURE_SET_YUV_CAPTURE_RAW_SUPPORT:
			/* update yuv capture raw support flag by *pFeatureData16 */
		break;
#endif 				
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		    OV9740MIPIYUVSetVideoMode(*pFeatureData16);
		    break; 
		#if defined(MT6575)
		case SENSOR_FEATURE_GET_EV_AWB_REF:
			 OV9740MIPIGetEvAwbRef(*pFeatureData32);
				break;
  		case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
			   OV9740MIPIGetCurAeAwbInfo(*pFeatureData32);	
			break;
		#endif
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV9740MIPI_GetSensorID(pFeatureData32); 
            break; 	

		case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
			OV9740MIPIGetAFMaxNumFocusAreas(pFeatureReturnPara32); 		   
			*pFeatureParaLen=4;
			break;	   
		case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
			OV9740MIPIGetAFMaxNumMeteringAreas(pFeatureReturnPara32);			  
			*pFeatureParaLen=4;
			break;
		case SENSOR_FEATURE_GET_EXIF_INFO:
			SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
			SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32); 		 
			OV9740MIPIGetExifInfo(*pFeatureData32);
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			OV9740MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			OV9740MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
		case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
			OV9740MIPIGetAEAWBLock((*pFeatureData32),*(pFeatureData32+1));
			break;			
		default:
			break;			
	}
	return ERROR_NONE;
}

SENSOR_FUNCTION_STRUCT	SensorFuncOV9740MIPI=
{
	OV9740MIPIOpen,
	OV9740MIPIGetInfo,
	OV9740MIPIGetResolution,
	OV9740MIPIFeatureControl,
	OV9740MIPIControl,
	OV9740MIPIClose
};

UINT32 OV9740_MIPI_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncOV9740MIPI;
	return ERROR_NONE;
}	/* SensorInit() */
