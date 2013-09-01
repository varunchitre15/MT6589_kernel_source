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

//s_porting add
//s_porting add
//s_porting add
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


#include "bf3905mipi_Sensor.h"
#include "bf3905mipi_Camera_Sensor_para.h"
#include "bf3905mipi_CameraCustomized.h"

#include <linux/proc_fs.h> 


#include <linux/dma-mapping.h>

#define BF3905_MIPI_DEBUG
#ifdef BF3905_MIPI_DEBUG
#define SENSORDB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[BF3905MIPI]", fmt, ##arg)
  
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
  UINT16  iSAT;
  UINT16  iISO;
  UINT16  iCONTRAST;
  UINT16  iEDGE;
  UINT16  iBanding;
  UINT16  iMirror;
  UINT16  iFrameRate;
  UINT32  iPCLK;
  UINT32  iInterClock;
} BF3905_MIPIStatus;
BF3905_MIPIStatus BF3905_MIPICurrentStatus;

static DEFINE_SPINLOCK(bf3905mipi_drv_lock);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

//static int sensor_id_fail = 0; 
static kal_uint32 zoom_factor = 0; 



kal_uint16 BF3905_MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char puSendCmd[1] = {(char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 1, (u8*)&get_byte,1,BF3905_MIPI_WRITE_ID);
	SENSORDB("BF3905_MIPI_read_cmos_sensor reg 0x%x = 0x%x\n",addr,get_byte);
	return get_byte;

}

inline void BF3905_MIPI_write_cmos_sensor(u16 addr, u32 para)
{
   char puSendCmd[2] = {(char)(addr & 0xFF) ,((char)(para & 0xFF))};

   iWriteRegI2C(puSendCmd , 2,BF3905_MIPI_WRITE_ID);
   
   SENSORDB("BF3905_MIPI_write_cmos_sensor reg 0x%x = 0x%x\n",addr,para);
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
kal_bool BF3905_MIPI_MPEG4_encode_mode = KAL_FALSE, BF3905_MIPI_MJPEG_encode_mode = KAL_FALSE;
static kal_bool BF3905_MIPI_VEDIO_encode_mode = KAL_FALSE; 
static kal_bool BF3905_MIPI_sensor_cap_state = KAL_FALSE; 
kal_uint32 BF3905_MIPI_PV_dummy_pixels=0,BF3905_MIPI_PV_dummy_lines=0,BF3905_MIPI_isp_master_clock=0;
static kal_uint32  BF3905_MIPI_sensor_pclk=260,BF3905_MIPI_sensor_inter_pclk= 130;

static kal_bool BF3905_MIPI_AE_ENABLE = KAL_TRUE; 

MSDK_SENSOR_CONFIG_STRUCT BF3905_MIPISensorConfigData;


/*************************************************************************
* FUNCTION
*	BF3905_MIPIInitialPara
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
static void BF3905_MIPIInitialPara(void)
{
  spin_lock(&bf3905mipi_drv_lock);
  BF3905_MIPICurrentStatus.iNightMode = 0;
  BF3905_MIPICurrentStatus.iWB = AWB_MODE_AUTO;
  BF3905_MIPICurrentStatus.iEffect = MEFFECT_OFF;
  BF3905_MIPICurrentStatus.iBanding = AE_FLICKER_MODE_50HZ;
  BF3905_MIPICurrentStatus.iEV = AE_EV_COMP_00;
  BF3905_MIPICurrentStatus.iISO = AE_ISO_AUTO;
  BF3905_MIPICurrentStatus.iCONTRAST= ISP_CONTRAST_MIDDLE;
  BF3905_MIPICurrentStatus.iEDGE = ISP_EDGE_MIDDLE;  
  BF3905_MIPICurrentStatus.iSAT = ISP_SAT_MIDDLE;
  BF3905_MIPICurrentStatus.iMirror = IMAGE_NORMAL;
  BF3905_MIPICurrentStatus.iFrameRate = 0;
  BF3905_MIPICurrentStatus.iPCLK = 260;
  BF3905_MIPICurrentStatus.iInterClock = BF3905_MIPICurrentStatus.iPCLK/2;
  spin_unlock(&bf3905mipi_drv_lock);
}


void BF3905_MIPI_set_mirror(kal_uint8 image_mirror)
{
        kal_uint8 temp_r = 0;
		temp_r = BF3905_MIPI_read_cmos_sensor(0x1e);
		//if(BF3905_MIPICurrentStatus.iMirror == image_mirror)
		//  return;
#if 1
		switch (image_mirror)  
		{
			case IMAGE_NORMAL:
				BF3905_MIPI_write_cmos_sensor(0x1e, temp_r & 0xCF); // 0x1e[4] vertical flip, 0:normal, 1:vertical
				                                                    //0x1e[5] mirror, 0:normal, 1:mirror
				break;
			case IMAGE_H_MIRROR:
				BF3905_MIPI_write_cmos_sensor(0x1e, temp_r | 0x20); 	// 
				break;
			case IMAGE_V_MIRROR:
				BF3905_MIPI_write_cmos_sensor(0x1e, temp_r | 0x10); 	// 
				break;
			case IMAGE_HV_MIRROR:
				BF3905_MIPI_write_cmos_sensor(0x1e, temp_r | 0x30); 	// 
				break;
				
			default:
				ASSERT(0);
				break;
		}		
		spin_lock(&bf3905mipi_drv_lock);
		BF3905_MIPICurrentStatus.iMirror = image_mirror;
		spin_unlock(&bf3905mipi_drv_lock);
#endif
}





/*****************************************************************************
 * FUNCTION
 *  BF3905_MIPI_set_dummy
 * DESCRIPTION
 *
 * PARAMETERS
 *  pixels      [IN]
 *  lines       [IN]
 * RETURNS
 *  void
 *****************************************************************************/
void BF3905_MIPI_set_dummy(kal_uint16 dummy_pixels, kal_uint16 dummy_lines)
{
		/****************************************************
		  * Adjust the extra H-Blanking & V-Blanking.
		  *****************************************************/
		SENSORDB("BF3905_MIPI_set_dummy,dummy_pixels=%d,dummy_lines=%d\n",dummy_pixels, dummy_lines);
		#if 1
		
		BF3905_MIPI_write_cmos_sensor(0x92, dummy_lines&0xff); //default 510
		BF3905_MIPI_write_cmos_sensor(0x93, (dummy_lines>>8)&0xff); 	// Extra V-Blanking
		BF3905_MIPI_write_cmos_sensor(0x2b, (dummy_pixels+16)&0xff); //default 784
		BF3905_MIPI_write_cmos_sensor(0x2a, (((dummy_pixels+16)>>8)<<4)|((BF3905_MIPI_read_cmos_sensor(0x2a))&0x0f)); 	// Extra H-Blanking
		
		#endif
}   /* BF3905_MIPI_set_dummy */

/*****************************************************************************
 * FUNCTION
 *  BF3905_MIPI_Initialize_Setting
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/
//<2013/06/18-26042-alberthsiao, Update camera parameters for imx111 and bf3905
void BF3905_MIPI_Initialize_Setting(void)
{
/******
	Below two groups of setting is supply by Coasia and AVP, for MIPI EVT1 Version,
	they both can work on my module on MT6577+JB sw.

	we could try those two groups of setting on MT6588 .
*******/
	
	SENSORDB("BF3905_MIPI_Initialize_Setting st\n");
//<2013/05/08-24731-alberthsiao, Update camera parameters for BF3905 sensor
	//BF3905_MIPI_write_cmos_sensor(0x12,0x80);
	//3905_MIPI for shading 20130507 kevinlin
	BF3905_MIPI_write_cmos_sensor(0x12,0x80);
	BF3905_MIPI_write_cmos_sensor(0x15,0x12);
	BF3905_MIPI_write_cmos_sensor(0x09,0x01);
	BF3905_MIPI_write_cmos_sensor(0x12,0x00);
	BF3905_MIPI_write_cmos_sensor(0x3a,0x20);
	BF3905_MIPI_write_cmos_sensor(0x1e,0x40);
	BF3905_MIPI_write_cmos_sensor(0x1b,0x0e);
	BF3905_MIPI_write_cmos_sensor(0x2a,0x00);
	BF3905_MIPI_write_cmos_sensor(0x2b,0x10);
	BF3905_MIPI_write_cmos_sensor(0x92,0x09);
	BF3905_MIPI_write_cmos_sensor(0x93,0x00);
	BF3905_MIPI_write_cmos_sensor(0x8a,0x9c);
	BF3905_MIPI_write_cmos_sensor(0x8b,0x82);
	BF3905_MIPI_write_cmos_sensor(0x13,0x00);
	BF3905_MIPI_write_cmos_sensor(0x01,0x15);
	BF3905_MIPI_write_cmos_sensor(0x02,0x23);
	BF3905_MIPI_write_cmos_sensor(0x9d,0x20);
	BF3905_MIPI_write_cmos_sensor(0x8c,0x02);
	BF3905_MIPI_write_cmos_sensor(0x8d,0xee);
	BF3905_MIPI_write_cmos_sensor(0x13,0x07);
	BF3905_MIPI_write_cmos_sensor(0x5d,0xb3);
	BF3905_MIPI_write_cmos_sensor(0xbf,0x08);
	BF3905_MIPI_write_cmos_sensor(0xc3,0x08);
	BF3905_MIPI_write_cmos_sensor(0xca,0x10);
	BF3905_MIPI_write_cmos_sensor(0x62,0x00);
	BF3905_MIPI_write_cmos_sensor(0x63,0x00);
	BF3905_MIPI_write_cmos_sensor(0xb9,0x00);
	BF3905_MIPI_write_cmos_sensor(0x64,0x00);
	BF3905_MIPI_write_cmos_sensor(0xbb,0x10);
	BF3905_MIPI_write_cmos_sensor(0x08,0x02);
	BF3905_MIPI_write_cmos_sensor(0x0e,0x10);
	BF3905_MIPI_write_cmos_sensor(0x22,0x12);	
	BF3905_MIPI_write_cmos_sensor(0x20,0x09);
	BF3905_MIPI_write_cmos_sensor(0x21,0x4f);
	BF3905_MIPI_write_cmos_sensor(0x3e,0x83);
	BF3905_MIPI_write_cmos_sensor(0x2f,0x84);
	BF3905_MIPI_write_cmos_sensor(0x16,0xa1);
	BF3905_MIPI_write_cmos_sensor(0x6c,0xc2);
	BF3905_MIPI_write_cmos_sensor(0x71,0x0f);
	BF3905_MIPI_write_cmos_sensor(0x7e,0x84);
	BF3905_MIPI_write_cmos_sensor(0x7f,0x3c);
	BF3905_MIPI_write_cmos_sensor(0x60,0xe5);
	BF3905_MIPI_write_cmos_sensor(0x61,0xf2);
	BF3905_MIPI_write_cmos_sensor(0x6d,0xc0);
	BF3905_MIPI_write_cmos_sensor(0x1e,0x40);
	BF3905_MIPI_write_cmos_sensor(0xd9,0x25);
	BF3905_MIPI_write_cmos_sensor(0xdf,0x26);
	BF3905_MIPI_write_cmos_sensor(0x17,0x00);
	BF3905_MIPI_write_cmos_sensor(0x18,0xa0);
	BF3905_MIPI_write_cmos_sensor(0x19,0x00);
	BF3905_MIPI_write_cmos_sensor(0x1a,0x78);
	BF3905_MIPI_write_cmos_sensor(0x03,0xa0);
	BF3905_MIPI_write_cmos_sensor(0x4a,0x0c);
	BF3905_MIPI_write_cmos_sensor(0xda,0x00);
	BF3905_MIPI_write_cmos_sensor(0xdb,0xa2);
	BF3905_MIPI_write_cmos_sensor(0xdc,0x00);
	BF3905_MIPI_write_cmos_sensor(0xdd,0x7a);
	BF3905_MIPI_write_cmos_sensor(0xde,0x00);
	BF3905_MIPI_write_cmos_sensor(0x34,0x1d);
	BF3905_MIPI_write_cmos_sensor(0x36,0x45);
	BF3905_MIPI_write_cmos_sensor(0x6e,0x20);
	BF3905_MIPI_write_cmos_sensor(0xbc,0x0d);
	BF3905_MIPI_write_cmos_sensor(0x35,0x30);
	BF3905_MIPI_write_cmos_sensor(0x65,0x24);
	BF3905_MIPI_write_cmos_sensor(0x66,0x19);
	BF3905_MIPI_write_cmos_sensor(0xbd,0xf4);
	BF3905_MIPI_write_cmos_sensor(0xbe,0x44);
	BF3905_MIPI_write_cmos_sensor(0x9b,0xf4);
	BF3905_MIPI_write_cmos_sensor(0x9c,0x44);
	BF3905_MIPI_write_cmos_sensor(0x37,0xf4);
	BF3905_MIPI_write_cmos_sensor(0x38,0x44);
	BF3905_MIPI_write_cmos_sensor(0xf1,0x00);
	BF3905_MIPI_write_cmos_sensor(0x70,0x0b);
	BF3905_MIPI_write_cmos_sensor(0x73,0x27);
	BF3905_MIPI_write_cmos_sensor(0x79,0x24);
	BF3905_MIPI_write_cmos_sensor(0x7a,0x23);
	BF3905_MIPI_write_cmos_sensor(0x75,0xaa);
	BF3905_MIPI_write_cmos_sensor(0x76,0x98);
	BF3905_MIPI_write_cmos_sensor(0x77,0x2a);
	BF3905_MIPI_write_cmos_sensor(0x7b,0x58);
	BF3905_MIPI_write_cmos_sensor(0x7d,0x00);
	BF3905_MIPI_write_cmos_sensor(0x13,0x07);
	BF3905_MIPI_write_cmos_sensor(0x24,0x4c);
	BF3905_MIPI_write_cmos_sensor(0x25,0xff);
	BF3905_MIPI_write_cmos_sensor(0x97,0x3c);
	BF3905_MIPI_write_cmos_sensor(0x98,0xbf);
	BF3905_MIPI_write_cmos_sensor(0x80,0xd0);
	BF3905_MIPI_write_cmos_sensor(0x81,0x00);
	BF3905_MIPI_write_cmos_sensor(0x82,0x2a);
	BF3905_MIPI_write_cmos_sensor(0x83,0x54);
	BF3905_MIPI_write_cmos_sensor(0x84,0x39);
	BF3905_MIPI_write_cmos_sensor(0x85,0x5d);
	BF3905_MIPI_write_cmos_sensor(0x86,0x88);
	BF3905_MIPI_write_cmos_sensor(0x89,0x63);
	BF3905_MIPI_write_cmos_sensor(0x94,0x22);
	BF3905_MIPI_write_cmos_sensor(0x96,0xb3);
	BF3905_MIPI_write_cmos_sensor(0x9a,0x50);
	BF3905_MIPI_write_cmos_sensor(0x99,0x10);
	BF3905_MIPI_write_cmos_sensor(0x9f,0x64);
	BF3905_MIPI_write_cmos_sensor(0x39,0x98);
	BF3905_MIPI_write_cmos_sensor(0x3f,0x98);
	BF3905_MIPI_write_cmos_sensor(0x90,0x20);
	BF3905_MIPI_write_cmos_sensor(0x91,0xf7);
	//default gamma
	BF3905_MIPI_write_cmos_sensor(0x40,0x3b);
	BF3905_MIPI_write_cmos_sensor(0x41,0x36);
	BF3905_MIPI_write_cmos_sensor(0x42,0x2b);
	BF3905_MIPI_write_cmos_sensor(0x43,0x1d);
	BF3905_MIPI_write_cmos_sensor(0x44,0x1a);
	BF3905_MIPI_write_cmos_sensor(0x45,0x14);
	BF3905_MIPI_write_cmos_sensor(0x46,0x11);
	BF3905_MIPI_write_cmos_sensor(0x47,0x0f);
	BF3905_MIPI_write_cmos_sensor(0x48,0x0e);
	BF3905_MIPI_write_cmos_sensor(0x49,0x0d);
	BF3905_MIPI_write_cmos_sensor(0x4b,0x0c);
	BF3905_MIPI_write_cmos_sensor(0x4c,0x0b);
	BF3905_MIPI_write_cmos_sensor(0x4e,0x0a);
	BF3905_MIPI_write_cmos_sensor(0x4f,0x09);
	BF3905_MIPI_write_cmos_sensor(0x50,0x09);
	//outdoor colormatrix
	BF3905_MIPI_write_cmos_sensor(0x5a,0x56);
	BF3905_MIPI_write_cmos_sensor(0x51,0x12);
	BF3905_MIPI_write_cmos_sensor(0x52,0x0d);
	BF3905_MIPI_write_cmos_sensor(0x53,0x92);
	BF3905_MIPI_write_cmos_sensor(0x54,0x7d);
	BF3905_MIPI_write_cmos_sensor(0x57,0x97);
	BF3905_MIPI_write_cmos_sensor(0x58,0x43);
	//indoor colormatrix
	BF3905_MIPI_write_cmos_sensor(0x5a,0xd6);
	BF3905_MIPI_write_cmos_sensor(0x51,0x39);
	BF3905_MIPI_write_cmos_sensor(0x52,0x0f);
	BF3905_MIPI_write_cmos_sensor(0x53,0x3b);
	BF3905_MIPI_write_cmos_sensor(0x54,0x55);
	BF3905_MIPI_write_cmos_sensor(0x57,0x7e);
	BF3905_MIPI_write_cmos_sensor(0x58,0x05);
	BF3905_MIPI_write_cmos_sensor(0x5c,0x26);
	BF3905_MIPI_write_cmos_sensor(0x6a,0xe1);
	BF3905_MIPI_write_cmos_sensor(0x23,0x55);
	BF3905_MIPI_write_cmos_sensor(0xa1,0x31);
	BF3905_MIPI_write_cmos_sensor(0xa2,0x0d);
	BF3905_MIPI_write_cmos_sensor(0xa3,0x25);
	BF3905_MIPI_write_cmos_sensor(0xa4,0x0b);
	BF3905_MIPI_write_cmos_sensor(0xa5,0x23);
	BF3905_MIPI_write_cmos_sensor(0xa6,0x04);
	BF3905_MIPI_write_cmos_sensor(0xa7,0x17);
	BF3905_MIPI_write_cmos_sensor(0xa8,0x1a);
	BF3905_MIPI_write_cmos_sensor(0xa9,0x20);
	BF3905_MIPI_write_cmos_sensor(0xaa,0x20);
	BF3905_MIPI_write_cmos_sensor(0xab,0x20);
	BF3905_MIPI_write_cmos_sensor(0xac,0x3c);
	BF3905_MIPI_write_cmos_sensor(0xad,0xf0);
	BF3905_MIPI_write_cmos_sensor(0xae,0x7b);
	BF3905_MIPI_write_cmos_sensor(0xd0,0xa4);
	BF3905_MIPI_write_cmos_sensor(0xd1,0x00);
	BF3905_MIPI_write_cmos_sensor(0xd2,0x58);
	BF3905_MIPI_write_cmos_sensor(0xc5,0xaa);
	BF3905_MIPI_write_cmos_sensor(0xc6,0xca);
	BF3905_MIPI_write_cmos_sensor(0xc7,0x30);
	BF3905_MIPI_write_cmos_sensor(0xc8,0x0d);
	BF3905_MIPI_write_cmos_sensor(0xc9,0x10);
	BF3905_MIPI_write_cmos_sensor(0xd3,0x09);
	BF3905_MIPI_write_cmos_sensor(0xd4,0x24);
	BF3905_MIPI_write_cmos_sensor(0xee,0x30);
	BF3905_MIPI_write_cmos_sensor(0xb0,0xe0);
	BF3905_MIPI_write_cmos_sensor(0xb3,0x48);
	BF3905_MIPI_write_cmos_sensor(0xb4,0xe3);
	BF3905_MIPI_write_cmos_sensor(0xb1,0xff);
	BF3905_MIPI_write_cmos_sensor(0xb2,0xa0);
	BF3905_MIPI_write_cmos_sensor(0xb4,0x63);
	BF3905_MIPI_write_cmos_sensor(0xb1,0xb3);
	BF3905_MIPI_write_cmos_sensor(0xb2,0xa0);
	BF3905_MIPI_write_cmos_sensor(0x55,0x00);
	BF3905_MIPI_write_cmos_sensor(0x56,0x40);
    SENSORDB("BF3905_MIPI_Initialize_Setting end\n");


}

/*****************************************************************************
 * FUNCTION
 *  BF3905_MIPI_PV_Mode
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/
void BF3905_MIPI_PV_Mode(void)
{	
    SENSORDB("PV set st\n");	
	SENSORDB("PV set ed\n");
}



/*****************************************************************************
 * FUNCTION
 *  BF3905_MIPI_CAP_Mode
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/

void BF3905_MIPI_CAP_Mode(void)
{
//
}

static void BF3905_MIPI_set_AE_mode(kal_bool AE_enable)
{
     kal_uint8 temp_r=0;	 
	 SENSORDB("BF3905_MIPI_set_AE_mode %d \n",AE_enable);
	 temp_r =  BF3905_MIPI_read_cmos_sensor(0x13);    
	 if(AE_enable==KAL_TRUE)
     {
          BF3905_MIPI_write_cmos_sensor(0x13,temp_r|0x01);//0x13[0]=1, enable
     }
     else
     {
		  BF3905_MIPI_write_cmos_sensor(0x13,temp_r&0xfe);//0x13[0]=0, disable             
     }
}
/*************************************************************************
* FUNCTION
*	BF3905_MIPI_set_min_framerate
*
* DESCRIPTION
*	This function set min framerate of BF3905_MIPI.
*
* PARAMETERS
*	UINT16 u2FrameRate
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/

void BF3905_MIPI_set_min_framerate(UINT16 u2FrameRate)
{
    kal_uint16 fr_time=0,temp_r=0;
	SENSORDB("BF3905_MIPI_set_min_framerate %d\n",u2FrameRate);
	temp_r=BF3905_MIPI_read_cmos_sensor(0x89);
	if(BF3905_MIPICurrentStatus.iBanding==AE_FLICKER_MODE_50HZ)
		{
		fr_time=100/u2FrameRate;
	    BF3905_MIPI_write_cmos_sensor(0x89,((temp_r&0x07)| (fr_time<<3)));//setMinFramerate
		}
	else if(BF3905_MIPICurrentStatus.iBanding ==AE_FLICKER_MODE_60HZ)
		{
		fr_time=120/u2FrameRate;
	    BF3905_MIPI_write_cmos_sensor(0x89,((temp_r&0x07)| (fr_time<<3)));//setMinFramerate
		}
	else
		{
	fr_time=100/u2FrameRate;
	BF3905_MIPI_write_cmos_sensor(0x89,((temp_r&0x07)| (fr_time<<3)));//setMinFramerate
		}

}


/*************************************************************************
* FUNCTION
*	BF3905_MIPI_night_mode
*
* DESCRIPTION
*	This function night mode of BF3905_MIPI.
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
void BF3905_MIPI_night_mode(kal_bool enable)
{
    //if(BF3905_MIPICurrentStatus.iNightMode == enable)
        //return;
    kal_uint8 temp_r=0;
    kal_uint32 dummylines=0, dummypixels=0;
    SENSORDB("BF3905_MIPI_night_mode st %d \n",enable);
	if (BF3905_MIPI_sensor_cap_state  || BF3905_MIPI_VEDIO_encode_mode)
		return;
	
	dummylines = BF3905_MIPICurrentStatus.iInterClock*100000/BF3905_MIPI_CAM_NOM_MAX_FPS/BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS-BF3905_MIPI_VGA_PERIOD_LINE_NUMS;
	temp_r = BF3905_MIPI_read_cmos_sensor(0x89);
    if(enable)
    	{
    	BF3905_MIPI_set_dummy(0,dummylines); //set max framerate
    	//BF3905_MIPI_write_cmos_sensor(0x89,((temp_r&0x07)| (BF3905_MIPI_CAM_NIT_MAX_FR_TIME<<3)));
    	BF3905_MIPI_set_min_framerate(7);
    	}

	else
		{	
		BF3905_MIPI_set_dummy(0,dummylines); //set max framerate
    	//BF3905_MIPI_write_cmos_sensor(0x89,((temp_r&0x07)| (BF3905_MIPI_CAM_NOM_MAX_FR_TIME<<3)));
    	BF3905_MIPI_set_min_framerate(13);
		}

	spin_lock(&bf3905mipi_drv_lock);
    BF3905_MIPICurrentStatus.iNightMode = enable;
	spin_unlock(&bf3905mipi_drv_lock);	
    SENSORDB("BF3905_MIPI_night_mode ed \n");
}	/* BF3905_MIPI_night_mode */

/*************************************************************************
* FUNCTION
*	BF3905_MIPI_GetSensorID
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
static kal_uint32 BF3905_MIPI_GetSensorID(kal_uint32 *sensorID)
{
	kal_uint16 sensor_id=0;

     // check if sensor ID correct
	 sensor_id=((BF3905_MIPI_read_cmos_sensor(0xfc)<<8)|(BF3905_MIPI_read_cmos_sensor(0xfd)));

	 SENSORDB("Read BF3905_MIPI Sensor ID%x \r\n",sensor_id);
	 
	 *sensorID=sensor_id;
	if (sensor_id != BF3905_MIPI_SENSOR_ID)
	{
	    *sensorID=0xFFFFFFFF;
	    SENSORDB("Sensor Read ByeBye \r\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	SENSORDB("Read BF3905_MIPI Sensor ID Sucessfully \r\n");
    return ERROR_NONE;    
}  


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*	BF3905_MIPIOpen
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
UINT32 BF3905_MIPIOpen(void)
{
	//volatile signed char i;
	kal_uint16 sensor_id=0;
	sensor_id=((BF3905_MIPI_read_cmos_sensor(0xfc)<<8)|(BF3905_MIPI_read_cmos_sensor(0xfd)));
	SENSORDB("Sensor Read BF3905_MIPI ID %x \r\n",sensor_id);

	if (sensor_id != BF3905_MIPI_SENSOR_ID)
	{
	    SENSORDB("Sensor Read ByeBye \r\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}

    BF3905_MIPIInitialPara(); 
	BF3905_MIPI_Initialize_Setting();
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	BF3905_MIPIClose
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
UINT32 BF3905_MIPIClose(void)
{
	return ERROR_NONE;
}	/* BF3905_MIPIClose() */

/*************************************************************************
* FUNCTION
*	BF3905_MIPIPreview
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
UINT32 BF3905_MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&bf3905mipi_drv_lock);
	BF3905_MIPI_sensor_cap_state = KAL_FALSE;
	BF3905_MIPI_PV_dummy_lines = 0;    
	
	if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
	{
	    SENSORDB("BF3905_MIPIPreview video \n");
		BF3905_MIPI_VEDIO_encode_mode = KAL_TRUE;				
	}
	else
	{
	    SENSORDB("BF3905_MIPIPreview camera \n");
		BF3905_MIPI_VEDIO_encode_mode = KAL_FALSE;		
	}
	spin_unlock(&bf3905mipi_drv_lock);
	
	BF3905_MIPI_PV_Mode();
	BF3905_MIPI_night_mode(BF3905_MIPICurrentStatus.iNightMode);
	BF3905_MIPI_set_mirror(sensor_config_data->SensorImageMirror);
 
    image_window->ExposureWindowWidth = BF3905_MIPI_IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight = BF3905_MIPI_IMAGE_SENSOR_PV_HEIGHT;

	// copy sensor_config_data
	memcpy(&BF3905_MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
  	return ERROR_NONE;
}	/* BF3905_MIPIPreview() */

UINT32 BF3905_MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
     //kal_uint32 pv_integration_time = 0;       // Uinit - us
     //kal_uint32 cap_integration_time = 0;
     //kal_uint16 PV_line_len = 0;
     //kal_uint16 CAP_line_len = 0;

	spin_lock(&bf3905mipi_drv_lock);
	BF3905_MIPI_sensor_cap_state = KAL_TRUE;
	spin_unlock(&bf3905mipi_drv_lock);
               
    //BF3905_MIPI_CAP_Mode();        
     // copy sensor_config_data
     memcpy(&BF3905_MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
     return ERROR_NONE;
}        /* BF3905_MIPICapture() */

UINT32 BF3905_MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=BF3905_MIPI_IMAGE_SENSOR_FULL_WIDTH;  
	pSensorResolution->SensorFullHeight=BF3905_MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	pSensorResolution->SensorPreviewWidth=BF3905_MIPI_IMAGE_SENSOR_PV_WIDTH; 
	pSensorResolution->SensorPreviewHeight=BF3905_MIPI_IMAGE_SENSOR_PV_HEIGHT;
	
	pSensorResolution->SensorVideoWidth=BF3905_MIPI_IMAGE_SENSOR_PV_WIDTH; 
	pSensorResolution->SensorVideoHeight=BF3905_MIPI_IMAGE_SENSOR_PV_HEIGHT;

	return ERROR_NONE;
}	/* BF3905_MIPIGetResolution() */

UINT32 BF3905_MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	
    switch(ScenarioId)
    {
    	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 pSensorInfo->SensorPreviewResolutionX=BF3905_MIPI_IMAGE_SENSOR_FULL_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=BF3905_MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			 pSensorInfo->SensorCameraPreviewFrameRate=15;
			 break;
		
		default:
			 pSensorInfo->SensorPreviewResolutionX=BF3905_MIPI_IMAGE_SENSOR_PV_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=BF3905_MIPI_IMAGE_SENSOR_PV_HEIGHT;
			 pSensorInfo->SensorCameraPreviewFrameRate=30;
    }
	pSensorInfo->SensorFullResolutionX=BF3905_MIPI_IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=BF3905_MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_UYVY;
	
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;

	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 1;
	

   	pSensorInfo->SensroInterfaceType        = SENSOR_INTERFACE_TYPE_MIPI;
 
	
	pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;//ISP_DRIVING_4MA;   	
	pSensorInfo->CaptureDelayFrame = 3; 
	pSensorInfo->PreviewDelayFrame = 3; 
	pSensorInfo->VideoDelayFrame = 4; 

	pSensorInfo->YUVAwbDelayFrame = 3; 
	pSensorInfo->YUVEffectDelayFrame = 2; 
	
	switch (ScenarioId) 
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pSensorInfo->SensorClockFreq=26;
			
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			
			pSensorInfo->SensorDataLatchCount= 2;
			
			pSensorInfo->SensorGrabStartX = 0; 
			pSensorInfo->SensorGrabStartY = 0; 
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
		break;

		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX = 0; 
			pSensorInfo->SensorGrabStartY = 0; 					
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
		
		break;
				
		default:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = 0; 
			pSensorInfo->SensorGrabStartY = 0; 					
		break;
	}
	memcpy(pSensorConfigData, &BF3905_MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* BF3905_MIPIGetInfo() */


UINT32 BF3905_MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			BF3905_MIPIPreview(pImageWindow, pSensorConfigData);
		break;
		
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			BF3905_MIPICapture(pImageWindow, pSensorConfigData);
		break;
		
		//#if defined(MT6575)
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			   BF3905_MIPICapture(pImageWindow, pSensorConfigData);
			break;
		//#endif
		
        default:
            return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* BF3905_MIPIControl() */

/* [TC] YUV sensor */	

BOOL BF3905_MIPI_set_param_wb(UINT16 para)
{
    kal_uint8 temp_r=0;
	//if(BF3905_MIPICurrentStatus.iWB == para)
		//return TRUE;
	SENSORDB("[Enter]BF3905_MIPI set_param_wb func:para = %d\n",para);
	temp_r=BF3905_MIPI_read_cmos_sensor(0x13);
	
	switch (para)
	{
		case AWB_MODE_AUTO:
			BF3905_MIPI_write_cmos_sensor(0x13, temp_r|0x02); //bit[1]:AWB Auto:1 menual:0
			BF3905_MIPI_write_cmos_sensor(0x01, 0x15);
			BF3905_MIPI_write_cmos_sensor(0x02, 0x23);	
			break;

		case AWB_MODE_CLOUDY_DAYLIGHT:	
			//======================================================================
			//	MWB : Cloudy_D65										 
			//======================================================================	
			BF3905_MIPI_write_cmos_sensor(0x13, temp_r&0xfd); //bit[1]:AWB Auto:1 menual:0
			BF3905_MIPI_write_cmos_sensor(0x01, 0x10);
			BF3905_MIPI_write_cmos_sensor(0x02, 0x28);
			break;

		case AWB_MODE_DAYLIGHT:
			//==============================================
			//	MWB : sun&daylight_D50						
			//==============================================
			BF3905_MIPI_write_cmos_sensor(0x13, temp_r&0xfd); //bit[1]:AWB Auto:1 menual:0
			BF3905_MIPI_write_cmos_sensor(0x01, 0x11);
			BF3905_MIPI_write_cmos_sensor(0x02, 0x26);  
			break;

		case AWB_MODE_INCANDESCENT:
			//==============================================									   
			//	MWB : Incand_Tungsten						
			//==============================================
			BF3905_MIPI_write_cmos_sensor(0x13, temp_r&0xfd); //bit[1]:AWB Auto:1 menual:0
			BF3905_MIPI_write_cmos_sensor(0x01, 0x1f);
			BF3905_MIPI_write_cmos_sensor(0x02, 0x15); 		
			break;

		case AWB_MODE_FLUORESCENT:
			//==================================================================
			//	MWB : Florescent_TL84							  
			//==================================================================
			BF3905_MIPI_write_cmos_sensor(0x13, temp_r&0xfd); //bit[1]:AWB Auto:1 menual:0
			BF3905_MIPI_write_cmos_sensor(0x01, 0x1a);
			BF3905_MIPI_write_cmos_sensor(0x02, 0x1e); 	 
			break;
		//case AWB_MODE_TUNGSTEN:	
			//BF3905_MIPI_write_cmos_sensor(0x13, temp_r&0xfd); //bit[1]:AWB Auto:1 menual:0
			//BF3905_MIPI_write_cmos_sensor(0x01, 0x1a);
			//BF3905_MIPI_write_cmos_sensor(0x02, 0x1e); 	  
			//break;
		default:
			return KAL_FALSE;	
	}
	spin_lock(&bf3905mipi_drv_lock);
    BF3905_MIPICurrentStatus.iWB = para;
	spin_unlock(&bf3905mipi_drv_lock);
    return TRUE;
}

BOOL BF3905_MIPI_set_param_effect(UINT16 para)
{
	/*----------------------------------------------------------------*/
   /* Local Variables												 */
   /*----------------------------------------------------------------*/
   kal_uint32 ret = KAL_TRUE;

   /*----------------------------------------------------------------*/
   /* Code Body 													 */
   /*----------------------------------------------------------------*/

   //if(BF3905_MIPICurrentStatus.iEffect == para)
	  //return TRUE;

   SENSORDB("[Enter]bf3905mipi set_param_effect func:para = %d\n",para);

   switch (para)
   {
	   case MEFFECT_OFF:
			BF3905_MIPI_write_cmos_sensor(0x69,0x00); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x67,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x68,0x80); // Normal, 
			
			BF3905_MIPI_write_cmos_sensor(0x70,0x0b); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x63); // Normal, 
			break;
	   case MEFFECT_MONO:
			BF3905_MIPI_write_cmos_sensor(0xb4,0xe3); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x63); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xba); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xaa); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x56,0x40); // Normal, 
			
			BF3905_MIPI_write_cmos_sensor(0x70,0x0b); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x7a,0x12); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x73,0x27); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x69,0x20); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x67,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x68,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x03); // Normal, 
		   break;
	   case MEFFECT_SEPIA:
			BF3905_MIPI_write_cmos_sensor(0xb4,0xe3); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x63); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xba); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xaa); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x56,0x40); // Normal,
			
			BF3905_MIPI_write_cmos_sensor(0x70,0x0b); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x7a,0x12); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x73,0x27); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x69,0x20); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x67,0x60); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x68,0xa0); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x03); // Normal, 
			break;
	   case MEFFECT_SOLARIZE:
			BF3905_MIPI_write_cmos_sensor(0xb4,0xe3); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x63); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xba); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xaa); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x56,0x40); // Normal, 
			
			BF3905_MIPI_write_cmos_sensor(0x70,0x0b); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x7a,0x12); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x73,0x27); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x69,0x40); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x67,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x68,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x03); // Normal, 
			break;
	   case MEFFECT_NEGATIVE: 
			BF3905_MIPI_write_cmos_sensor(0xb4,0xe3); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x63); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xba); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xaa); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x56,0x40); // Normal, 
			
			BF3905_MIPI_write_cmos_sensor(0x70,0x0b); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x7a,0x12); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x73,0x27); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x69,0x01); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x67,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x68,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x03); // Normal, 
			break;
		case MEFFECT_BLACKBOARD:   
			BF3905_MIPI_write_cmos_sensor(0xb4,0xe3); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x63); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xba); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xaa); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x56,0x40); // Normal, 
			
			BF3905_MIPI_write_cmos_sensor(0x70,0x6b); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x7a,0x12); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x73,0x27); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x69,0x00); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x67,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x68,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x03); // Normal, 
			break;
		case MEFFECT_WHITEBOARD:  
			BF3905_MIPI_write_cmos_sensor(0xb4,0xe3); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x63); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xba); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xaa); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x56,0x40); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x70,0x7b); // Normal, 
			
			BF3905_MIPI_write_cmos_sensor(0x7a,0x12); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x73,0x27); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x69,0x00); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x67,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x68,0x80); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x03); // Normal,    
			break;
		case MEFFECT_AQUA:
			BF3905_MIPI_write_cmos_sensor(0xb4,0xe3); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xef); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x63); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb1,0xba); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb2,0xaa); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x56,0x40); // Normal, 
			
			BF3905_MIPI_write_cmos_sensor(0x70,0x0b); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x7a,0x12); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x73,0x27); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x69,0x20); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x67,0x90); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0x68,0x60); // Normal, 
			BF3905_MIPI_write_cmos_sensor(0xb4,0x03); // Normal,  
			break;
	   default:
		   ret = KAL_FALSE;
   }
   
   spin_lock(&bf3905mipi_drv_lock);
   BF3905_MIPICurrentStatus.iEffect = para;
   spin_unlock(&bf3905mipi_drv_lock);
   return ret;
} 

void BF3905_MIPIGetAEAWBLock(UINT32 *pAElockRet32,UINT32 *pAWBlockRet32)
{
    *pAElockRet32 = 1;
	*pAWBlockRet32 = 1;
    SENSORDB("BF3905_MIPIGetAEAWBLock,AE=%d ,AWB=%d\n,",*pAElockRet32,*pAWBlockRet32);
}


BOOL BF3905_MIPI_set_param_banding(UINT16 para)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/

	kal_uint32 int_step_50=0,int_step_60=0;
    SENSORDB("BF3905_MIPI_set_param_banding st\n");
	//if(BF3905_MIPICurrentStatus.iBanding == para)
     // return TRUE;
    switch (para)
	{
		case AE_FLICKER_MODE_50HZ:		
			SENSORDB("BF3905_MIPI_set_param_banding 50hz \n");
			int_step_50 = BF3905_MIPICurrentStatus.iInterClock*100000/BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS/100;
			BF3905_MIPI_write_cmos_sensor(0x8a,int_step_50);
			BF3905_MIPI_write_cmos_sensor(0x80,BF3905_MIPI_read_cmos_sensor(0x80)|0x02); //0x80[1]=1, select 50hz banding filter	
			spin_lock(&bf3905mipi_drv_lock);
		    BF3905_MIPICurrentStatus.iBanding = para;
			spin_unlock(&bf3905mipi_drv_lock);
			BF3905_MIPI_night_mode(BF3905_MIPICurrentStatus.iNightMode);
			break;
		case AE_FLICKER_MODE_60HZ:		
			SENSORDB("BF3905_MIPI_set_param_banding 60hz \n");
			int_step_60 = BF3905_MIPICurrentStatus.iInterClock*100000/BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS/120;
			BF3905_MIPI_write_cmos_sensor(0x8b,int_step_60);
			BF3905_MIPI_write_cmos_sensor(0x80,BF3905_MIPI_read_cmos_sensor(0x80)&0xfd); //0x80[1]=0, select 60hz banding filter					
			spin_lock(&bf3905mipi_drv_lock);
		    BF3905_MIPICurrentStatus.iBanding = para;
			spin_unlock(&bf3905mipi_drv_lock);
			BF3905_MIPI_night_mode(BF3905_MIPICurrentStatus.iNightMode);
			break;
		default:
			/*
			SENSORDB("BF3905_MIPI_set_param_banding default,we choose 50hz instead\n");
			int_step_50 = BF3905_MIPICurrentStatus.iInterClock*100000/BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS/100;
			BF3905_MIPI_write_cmos_sensor(0x8a,int_step_50);
			BF3905_MIPI_write_cmos_sensor(0x80,BF3905_MIPI_read_cmos_sensor(0x80)|0x02); //0x80[1]=1, select 50hz banding filter	
			spin_lock(&bf3905mipi_drv_lock);
		    BF3905_MIPICurrentStatus.iBanding = AE_FLICKER_MODE_50HZ;
			spin_unlock(&bf3905mipi_drv_lock);
			BF3905_MIPI_night_mode(BF3905_MIPICurrentStatus.iNightMode);
			*/
			SENSORDB("BF3905_MIPI_set_param_banding default 60hz  zzg20130604  \n");
			int_step_60 = BF3905_MIPICurrentStatus.iInterClock*100000/BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS/120;
			BF3905_MIPI_write_cmos_sensor(0x8b,int_step_60);
			BF3905_MIPI_write_cmos_sensor(0x80,BF3905_MIPI_read_cmos_sensor(0x80)&0xfd); //0x80[1]=0, select 60hz banding filter					
			spin_lock(&bf3905mipi_drv_lock);
		    BF3905_MIPICurrentStatus.iBanding = para;
			spin_unlock(&bf3905mipi_drv_lock);
			BF3905_MIPI_night_mode(BF3905_MIPICurrentStatus.iNightMode);
			break;
			return KAL_FALSE;
	}
	
    SENSORDB("BF3905_MIPI_set_param_banding ed\n");
	return TRUE;
} /* BF3905_MIPI_set_param_banding */

BOOL BF3905_MIPI_set_param_exposure(UINT16 para)
{

	//if(BF3905_MIPICurrentStatus.iEV == para)
	//	return TRUE;
	
	SENSORDB("[Enter]bf3905mipi set_param_exposure func:para = %d\n",para);
	
    switch (para)
	{	
		case AE_EV_COMP_n23:	
			BF3905_MIPI_write_cmos_sensor(0x24, 0x28); //EV -2
			break;
		case AE_EV_COMP_n10:
			BF3905_MIPI_write_cmos_sensor(0x24, 0x10); //EV -1
			break;
		case AE_EV_COMP_00:				   
			BF3905_MIPI_write_cmos_sensor(0x24, 0x4c); //EV 0  //48
			break;
		case AE_EV_COMP_10:					
			BF3905_MIPI_write_cmos_sensor(0x24, 0x7f);  //EV +1
			break;			
		case AE_EV_COMP_23:	
			BF3905_MIPI_write_cmos_sensor(0x24, 0x60); //EV +2
			break;	
		case AE_EV_COMP_n07:				   
		case AE_EV_COMP_n03:				   
		case AE_EV_COMP_03: 			
		case AE_EV_COMP_07: 
		case AE_EV_COMP_13:
			break;
			
		default:			
			return FALSE;
	}
	spin_lock(&bf3905mipi_drv_lock);
	BF3905_MIPICurrentStatus.iEV = para;
	spin_unlock(&bf3905mipi_drv_lock);
	return TRUE;

}/* BF3905_MIPI_set_param_exposure */

BOOL BF3905_MIPI_set_param_saturation(UINT16 para)
{
	
	//if(BF3905_MIPICurrentStatus.iSAT == para)
		return TRUE;
	
	SENSORDB("[Enter]bf3905mipi set_param_saturation func:para = %d\n",para);

	switch (para)
	{	
		case ISP_SAT_LOW: 
			BF3905_MIPI_write_cmos_sensor(0xb4, 0xe3); 				
			BF3905_MIPI_write_cmos_sensor(0xb1, 0x00); //ef->00
			BF3905_MIPI_write_cmos_sensor(0xb2, 0x00); //ef->00
			BF3905_MIPI_write_cmos_sensor(0xb4, 0x63);
			BF3905_MIPI_write_cmos_sensor(0xb1, 0x00); //low
			BF3905_MIPI_write_cmos_sensor(0xb2, 0x00); 
			break;
		case ISP_SAT_MIDDLE: 	
			BF3905_MIPI_write_cmos_sensor(0xb4, 0xe3); 				
			BF3905_MIPI_write_cmos_sensor(0xb1, 0xf0); 
			BF3905_MIPI_write_cmos_sensor(0xb2, 0xa0); 
			BF3905_MIPI_write_cmos_sensor(0xb4, 0x63);
			BF3905_MIPI_write_cmos_sensor(0xb1, 0xb0); //middle
			BF3905_MIPI_write_cmos_sensor(0xb2, 0xa0);  
			break;
		case ISP_SAT_HIGH: 	
			BF3905_MIPI_write_cmos_sensor(0xb4, 0xe3); 				
			BF3905_MIPI_write_cmos_sensor(0xb1, 0xef); 
			BF3905_MIPI_write_cmos_sensor(0xb2, 0xef); 
			BF3905_MIPI_write_cmos_sensor(0xb4, 0x63);
			BF3905_MIPI_write_cmos_sensor(0xb1, 0xff); //high
			BF3905_MIPI_write_cmos_sensor(0xb2, 0xff); 
			break;
		default:			
			return FALSE;
	}
	spin_lock(&bf3905mipi_drv_lock);
	BF3905_MIPICurrentStatus.iSAT = para;
	spin_unlock(&bf3905mipi_drv_lock);

	return TRUE;
	
	
}
//>2013/05/08-24731-alberthsiao
BOOL BF3905_MIPI_set_param_contrast(UINT16 para)
{

	//if(BF3905_MIPICurrentStatus.iCONTRAST == para)
	//	return TRUE;
	
	SENSORDB("[Enter]bf3905mipi set_param_contrast func:para = %d\n",para);
  			
	switch (para)
	{	
		case ISP_CONTRAST_LOW: 			   
			BF3905_MIPI_write_cmos_sensor(0x56, 0x18); //low
			break;
		case ISP_CONTRAST_MIDDLE: 				
			BF3905_MIPI_write_cmos_sensor(0x56, 0x40);	//middle
			break;
		case ISP_CONTRAST_HIGH:	
			BF3905_MIPI_write_cmos_sensor(0x56, 0x79); //high
			break;
		default:			
			return FALSE;
	}
	spin_lock(&bf3905mipi_drv_lock);
	BF3905_MIPICurrentStatus.iCONTRAST = para;
	spin_unlock(&bf3905mipi_drv_lock);
	return TRUE;
	
}

	
BOOL BF3905_MIPI_set_param_edge(UINT16 para)
{

	//if(BF3905_MIPICurrentStatus.iEDGE == para)
	//	return TRUE;
	
	SENSORDB("[Enter]bf3905mipi set_param_edge func:para = %d\n",para);
		
	switch (para)
	{	
		case ISP_EDGE_LOW:			   
			BF3905_MIPI_write_cmos_sensor(0x7a, 0x00); //low	
			BF3905_MIPI_write_cmos_sensor(0x73, 0x63); 
			break;
		case ISP_EDGE_MIDDLE:				
			BF3905_MIPI_write_cmos_sensor(0x7a, 0x23);	//middle		
			BF3905_MIPI_write_cmos_sensor(0x73, 0x27); 
			break;
		case ISP_EDGE_HIGH: 
			BF3905_MIPI_write_cmos_sensor(0x7a, 0x66); //high		
			BF3905_MIPI_write_cmos_sensor(0x73, 0x1c); 
			break;
		default:			
			return FALSE;
	}
	spin_lock(&bf3905mipi_drv_lock);
	BF3905_MIPICurrentStatus.iEDGE = para;
	spin_unlock(&bf3905mipi_drv_lock);
	return TRUE;

}
//>2013/06/18-26042-alberthsiao

BOOL BF3905_MIPI_set_param_iso(UINT16 para)
{
	
	//if(BF3905_MIPICurrentStatus.iISO == para)
	//	return TRUE;
	
	SENSORDB("[Enter]bf3905mipi set_param_iso func:para = %d\n",para);	  
	kal_uint8 temp_r;
	temp_r=BF3905_MIPI_read_cmos_sensor(0x13);
	switch (para)
	{	
		case AE_ISO_AUTO:			
			BF3905_MIPI_write_cmos_sensor(0x13,temp_r&0xBF);//0x13[6]=0
			BF3905_MIPI_write_cmos_sensor(0x9f, 0x61); //auto
			break;
		case AE_ISO_100: 		
			BF3905_MIPI_write_cmos_sensor(0x13,temp_r|0x50);//0x13[6]=1
			BF3905_MIPI_write_cmos_sensor(0x9f, 0x62); //iso 100
			break;
		case AE_ISO_200:			
			BF3905_MIPI_write_cmos_sensor(0x13,temp_r|0x50);//0x13[6]=1
			BF3905_MIPI_write_cmos_sensor(0x9f, 0x63);	//iso 200
			break;
		case AE_ISO_400:		
			BF3905_MIPI_write_cmos_sensor(0x13,temp_r|0x50);//0x13[6]=1
			BF3905_MIPI_write_cmos_sensor(0x9f, 0x64); //iso 400
			break;
		case AE_ISO_800: 
			BF3905_MIPI_write_cmos_sensor(0x13,temp_r|0x50);//0x13[6]=1
			BF3905_MIPI_write_cmos_sensor(0x9f, 0x65); //iso 800
			break;	
		case AE_ISO_1600:		
			BF3905_MIPI_write_cmos_sensor(0x13,temp_r|0x50);//0x13[6]=1
			BF3905_MIPI_write_cmos_sensor(0x9f, 0x66); //iso 1600
			break;
		default:			
			return FALSE;
	}
	spin_lock(&bf3905mipi_drv_lock);
	BF3905_MIPICurrentStatus.iISO = para;
	spin_unlock(&bf3905mipi_drv_lock);
	return TRUE;
	
}

UINT32 BF3905_MIPIYUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
	switch (iCmd) {
	case FID_SCENE_MODE:
	    if (iPara == SCENE_MODE_OFF)
	    {
	        BF3905_MIPI_night_mode(0); 
	    }

         else if (iPara == SCENE_MODE_NIGHTSCENE)			
	    {
            BF3905_MIPI_night_mode(1); 
	    }	    
	    break; 	    
	case FID_AWB_MODE:
         BF3905_MIPI_set_param_wb(iPara);
	break;
	case FID_COLOR_EFFECT:
    	    
         BF3905_MIPI_set_param_effect(iPara);
	break;
	case FID_AE_EV:  	    
         BF3905_MIPI_set_param_exposure(iPara);
	break;
	case FID_AE_FLICKER:
	    	    	    
         BF3905_MIPI_set_param_banding(iPara);
	break;
    case FID_AE_SCENE_MODE:  
		spin_lock(&bf3905mipi_drv_lock);
        if (iPara == AE_MODE_OFF) {
          BF3905_MIPI_AE_ENABLE = KAL_FALSE; 
        }
        else {
          BF3905_MIPI_AE_ENABLE = KAL_TRUE; 
        }
	    spin_unlock(&bf3905mipi_drv_lock);
        BF3905_MIPI_set_AE_mode(BF3905_MIPI_AE_ENABLE);
    break; 
	case FID_ISP_SAT:
		BF3905_MIPI_set_param_saturation(iPara);
	break;
	case FID_ISP_CONTRAST:
		BF3905_MIPI_set_param_contrast(iPara);
	break;
	case FID_ISP_EDGE:	
		BF3905_MIPI_set_param_edge(iPara);
	break;
	case FID_AE_ISO:	
		BF3905_MIPI_set_param_iso(iPara);
	break;
	case FID_ZOOM_FACTOR:
		spin_lock(&bf3905mipi_drv_lock);
	    zoom_factor = iPara; 
		spin_unlock(&bf3905mipi_drv_lock);
	break; 
	default:
	break;
	}
	return ERROR_NONE;
}   /* BF3905_MIPIYUVSensorSetting */



UINT32 BF3905_MIPIYUVSetVideoMode(UINT16 u2FrameRate)
{

   // if(BF3905_MIPICurrentStatus.iFrameRate == u2FrameRate)
    //  return ERROR_NONE;
    kal_uint32 dummylines=0, dummypixels=0;
    SENSORDB("BF3905_MIPIYUVSetVideoMode framerate= %d \n",u2FrameRate);
	spin_lock(&bf3905mipi_drv_lock);
    BF3905_MIPI_VEDIO_encode_mode = KAL_TRUE; 
	//BF3905_MIPI_MPEG4_encode_mode = KAL_TRUE;
	spin_unlock(&bf3905mipi_drv_lock);

    if(0==u2FrameRate ) //dynamic framerate
    {		
		return ERROR_NONE;
    }
    if(5<=u2FrameRate && u2FrameRate<=30 )// 
    {
        dummylines=BF3905_MIPICurrentStatus.iInterClock*100000/u2FrameRate/BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS-BF3905_MIPI_VGA_PERIOD_LINE_NUMS;
		BF3905_MIPI_set_dummy(0,dummylines);//set max framerate
		BF3905_MIPI_set_min_framerate(u2FrameRate);
    }
    else 
    {
        printk("Wrong Frame Rate \n"); 
    }
	
    return ERROR_NONE;
}


kal_uint16 BF3905_MIPIReadShutter(void)
{
   kal_uint8 temp_msb=0x00,temp_lsb=0x00;
   kal_uint16 temp=0x0000;
      
   temp_msb=BF3905_MIPI_read_cmos_sensor(0x8c);
   temp_msb = (temp_msb<<8)&0xff00;
   
   temp_lsb=BF3905_MIPI_read_cmos_sensor(0x8d);
   temp_lsb = temp_lsb&0x00ff;

   temp = temp_msb|temp_lsb;

   return temp;   
}
kal_uint16 BF3905_MIPIReadGain(void)
{
    kal_uint16 temp=0x0000;   
	temp = BF3905_MIPI_read_cmos_sensor(0x87); //0x10 means 1X gain.
	temp = temp*4; 

	return temp;
}
kal_uint16 BF3905_MIPIReadAwbRGain(void)
{
    kal_uint16 temp=0x0000,Base_gain=64;
	
	temp=BF3905_MIPI_read_cmos_sensor(0x02);
	temp = temp*4;
	return temp;
}
kal_uint16 BF3905_MIPIReadAwbBGain(void)
{
    kal_uint16 temp=0x0000,Base_gain=64;	
	temp=BF3905_MIPI_read_cmos_sensor(0x01);
	temp = temp*4;
	return temp;
}



/*************************************************************************
* FUNCTION
*    BF3905_MIPIGetEvAwbRef
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void BF3905_MIPIGetEvAwbRef(UINT32 pSensorAEAWBRefStruct/*PSENSOR_AE_AWB_REF_STRUCT Ref*/)
{
    PSENSOR_AE_AWB_REF_STRUCT Ref = (PSENSOR_AE_AWB_REF_STRUCT)pSensorAEAWBRefStruct;
    SENSORDB("BF3905_MIPIGetEvAwbRef  \n" );
    	
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
*    BF3905_MIPIGetCurAeAwbInfo
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void BF3905_MIPIGetCurAeAwbInfo(UINT32 pSensorAEAWBCurStruct/*PSENSOR_AE_AWB_CUR_STRUCT Info*/)
{
    PSENSOR_AE_AWB_CUR_STRUCT Info = (PSENSOR_AE_AWB_CUR_STRUCT)pSensorAEAWBCurStruct;
    SENSORDB("BF3905_MIPIGetCurAeAwbInfo  \n" );

    Info->SensorAECur.AeCurShutter = BF3905_MIPIReadShutter();
    Info->SensorAECur.AeCurGain = BF3905_MIPIReadGain() * 2; /* 128 base */
    
    Info->SensorAwbGainCur.AwbCurRgain = BF3905_MIPIReadAwbRGain()<< 1; /* 128 base */
    
    Info->SensorAwbGainCur.AwbCurBgain = BF3905_MIPIReadAwbBGain()<< 1; /* 128 base */
}



void BF3905_MIPIGetAFMaxNumFocusAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("BF3905_MIPIGetAFMaxNumFocusAreas, *pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);

}

void BF3905_MIPIGetAFMaxNumMeteringAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("BF3905_MIPIGetAFMaxNumMeteringAreas,*pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);

}

void BF3905_MIPIGetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = BF3905_MIPICurrentStatus.iISO;
    pExifInfo->AWBMode = BF3905_MIPICurrentStatus.iWB;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = BF3905_MIPICurrentStatus.iISO;
}

void BF3905_MIPIGetDelayInfo(UINT32 delayAddr)
{
    SENSOR_DELAY_INFO_STRUCT* pDelayInfo = (SENSOR_DELAY_INFO_STRUCT*)delayAddr;
    pDelayInfo->InitDelay = 3;
    pDelayInfo->EffectDelay = 0;
    pDelayInfo->AwbDelay = 0;
}


UINT32 BF3905MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 inter_pclk;
	kal_int32 dummyLine;
	kal_uint32 lineLength,frameHeight;
		
	SENSORDB("BF3905MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			inter_pclk = BF3905_MIPICurrentStatus.iInterClock*100000;
			lineLength = BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS;
			frameHeight = inter_pclk/frameRate/lineLength;
			dummyLine = frameHeight - BF3905_MIPI_VGA_PERIOD_LINE_NUMS;
			SENSORDB("BF3905MIPISetMaxFramerateByScenario MSDK_SCENARIO_ID_CAMERA_PREVIEW: lineLength = %d, dummy=%d\n",lineLength,dummyLine);
			BF3905_MIPI_set_dummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			inter_pclk = BF3905_MIPICurrentStatus.iInterClock*100000;;
			lineLength = BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS;
			frameHeight = inter_pclk/frameRate/lineLength;
			dummyLine = frameHeight - BF3905_MIPI_VGA_PERIOD_LINE_NUMS;
			SENSORDB("BF3905MIPISetMaxFramerateByScenario MSDK_SCENARIO_ID_VIDEO_PREVIEW: lineLength = %d, dummy=%d\n",lineLength,dummyLine);			
			BF3905_MIPI_set_dummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			inter_pclk = BF3905_MIPICurrentStatus.iInterClock*100000;;
			lineLength = BF3905_MIPI_VGA_PERIOD_PIXEL_NUMS;
			frameHeight = inter_pclk/frameRate/lineLength;
			dummyLine = frameHeight - BF3905_MIPI_VGA_PERIOD_LINE_NUMS;
			SENSORDB("BF3905MIPISetMaxFramerateByScenario MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG: lineLength = %d, dummy=%d\n",lineLength,dummyLine);			
			BF3905_MIPI_set_dummy(0, dummyLine);			
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


UINT32 BF3905MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
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

UINT32 BF3905_MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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

	SENSORDB("BF3905_MIPIFeatureControl FeatureId %d \n",FeatureId);
	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=BF3905_MIPI_IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=BF3905_MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
			//*pFeatureReturnPara16++=BF3905_MIPI_PV_PERIOD_PIXEL_NUMS+BF3905_MIPI_PV_dummy_pixels;
			//*pFeatureReturnPara16=BF3905_MIPI_PV_PERIOD_LINE_NUMS+BF3905_MIPI_PV_dummy_lines;
			//*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = BF3905_MIPI_sensor_pclk/10;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			BF3905_MIPI_night_mode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			BF3905_MIPI_isp_master_clock=*pFeatureData32;
		break;
		case SENSOR_FEATURE_SET_REGISTER:
			BF3905_MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = BF3905_MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &BF3905_MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
			//BF3905_MIPIYUVSensorSetting((MSDK_ISP_FEATURE_ENUM)*pFeatureData16, *(pFeatureData16+1));
			BF3905_MIPIYUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		break;

		case SENSOR_FEATURE_SET_VIDEO_MODE:
		    BF3905_MIPIYUVSetVideoMode(*pFeatureData16);
	    break; 
		//#if defined(MT6575)
		case SENSOR_FEATURE_GET_EV_AWB_REF:
			 BF3905_MIPIGetEvAwbRef(*pFeatureData32);
		break;
  		case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
			   BF3905_MIPIGetCurAeAwbInfo(*pFeatureData32);	
		break;
		//#endif
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
            BF3905_MIPI_GetSensorID(pFeatureData32); 
        break; 	

		case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
			BF3905_MIPIGetAFMaxNumFocusAreas(pFeatureReturnPara32); 		   
			*pFeatureParaLen=4;
		break;	   
		case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
			BF3905_MIPIGetAFMaxNumMeteringAreas(pFeatureReturnPara32);			  
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_EXIF_INFO:
			SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
			SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32); 		 
			BF3905_MIPIGetExifInfo(*pFeatureData32);
		break;
		case SENSOR_FEATURE_GET_DELAY_INFO:
			SENSORDB("SENSOR_FEATURE_GET_DELAY_INFO\n");
		    BF3905_MIPIGetDelayInfo(*pFeatureData32);
		break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			BF3905MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
		break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			BF3905MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
		break;
		case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
			BF3905_MIPIGetAEAWBLock((*pFeatureData32),*(pFeatureData32+1));
		break;
		default:
		break;			
	}
	return ERROR_NONE;
}

SENSOR_FUNCTION_STRUCT	SensorFuncBF3905_MIPI=
{
	BF3905_MIPIOpen,
	BF3905_MIPIGetInfo,
	BF3905_MIPIGetResolution,
	BF3905_MIPIFeatureControl,
	BF3905_MIPIControl,
	BF3905_MIPIClose
};

UINT32 BF3905_MIPI_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncBF3905_MIPI;
	return ERROR_NONE;
}	/* SensorInit() */
