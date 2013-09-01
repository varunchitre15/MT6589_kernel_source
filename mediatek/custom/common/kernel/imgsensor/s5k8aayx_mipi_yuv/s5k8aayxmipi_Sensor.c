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


#include "s5k8aayxmipi_Sensor.h"
#include "s5k8aayxmipi_Camera_Sensor_para.h"
#include "s5k8aayxmipi_CameraCustomized.h"

#define S5K8AAYX_MIPI_DEBUG
#ifdef S5K8AAYX_MIPI_DEBUG
#define SENSORDB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[S5K8AAYXMIPI]", fmt, ##arg)
  
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
} S5K8AAYX_MIPIStatus;
S5K8AAYX_MIPIStatus S5K8AAYX_MIPICurrentStatus;

static DEFINE_SPINLOCK(s5k8aayxmipi_drv_lock);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

//static int sensor_id_fail = 0; 
static kal_uint32 zoom_factor = 0; 


kal_uint16 S5K8AAYX_MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,S5K8AAYX_MIPI_WRITE_ID);
	return (kal_uint16)(((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff));

}

inline void S5K8AAYX_MIPI_write_cmos_sensor(u16 addr, u32 para)
{
   char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};

   iWriteRegI2C(puSendCmd , 4,S5K8AAYX_MIPI_WRITE_ID);
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
kal_bool S5K8AAYX_MIPI_MPEG4_encode_mode = KAL_FALSE, S5K8AAYX_MIPI_MJPEG_encode_mode = KAL_FALSE;
static kal_bool S5K8AAYX_MIPI_VEDIO_encode_mode = KAL_FALSE; 
static kal_bool S5K8AAYX_MIPI_sensor_cap_state = KAL_FALSE; 
kal_uint32 S5K8AAYX_MIPI_PV_dummy_pixels=0,S5K8AAYX_MIPI_PV_dummy_lines=0,S5K8AAYX_MIPI_isp_master_clock=0;
static kal_uint32  S5K8AAYX_MIPI_sensor_pclk=920;
static kal_bool S5K8AAYX_MIPI_AE_ENABLE = KAL_TRUE; 

MSDK_SENSOR_CONFIG_STRUCT S5K8AAYX_MIPISensorConfigData;


/*************************************************************************
* FUNCTION
*	S5K8AAYX_MIPIInitialPara
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
static void S5K8AAYX_MIPIInitialPara(void)
{
  spin_lock(&s5k8aayxmipi_drv_lock);
  S5K8AAYX_MIPICurrentStatus.iNightMode = 0;
  S5K8AAYX_MIPICurrentStatus.iWB = AWB_MODE_AUTO;
  S5K8AAYX_MIPICurrentStatus.iEffect = MEFFECT_OFF;
  S5K8AAYX_MIPICurrentStatus.iBanding = AE_FLICKER_MODE_50HZ;
  S5K8AAYX_MIPICurrentStatus.iEV = AE_EV_COMP_00;
  S5K8AAYX_MIPICurrentStatus.iMirror = IMAGE_NORMAL;
  S5K8AAYX_MIPICurrentStatus.iFrameRate = 0;
  spin_unlock(&s5k8aayxmipi_drv_lock);
}


void S5K8AAYX_MIPI_set_mirror(kal_uint8 image_mirror)
{

		if(S5K8AAYX_MIPICurrentStatus.iMirror == image_mirror)
		  return;

		S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);
		S5K8AAYX_MIPI_write_cmos_sensor(0x002a, 0x01E8); 
	
		switch (image_mirror)  
		{
			case IMAGE_NORMAL:
				S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); 	// REG_0TC_PCFG_uPrevMirror
				S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); 	// REG_0TC_PCFG_uCaptureMirror
				break;
			case IMAGE_H_MIRROR:
				S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); 	// REG_0TC_PCFG_uPrevMirror
				S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); 	// REG_0TC_PCFG_uCaptureMirror
				break;
			case IMAGE_V_MIRROR:
				S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); 	// REG_0TC_PCFG_uPrevMirror
				S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); 	// REG_0TC_PCFG_uCaptureMirror
				break;
			case IMAGE_HV_MIRROR:
				S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003); 	// REG_0TC_PCFG_uPrevMirror
				S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003); 	// REG_0TC_PCFG_uCaptureMirror
				break;
				
			default:
				ASSERT(0);
				break;
		}
	
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01A8); 
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // #REG_TC_GP_ActivePrevConfig // Select preview configuration_0
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01AC);
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001); // #REG_TC_GP_PrevOpenAfterChange
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01A6); 
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);  // #REG_TC_GP_NewConfigSync // Update preview configuration
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01AA); 
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);  // #REG_TC_GP_PrevConfigChanged
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x019E); 
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);  // #REG_TC_GP_EnablePreview // Start preview
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);  // #REG_TC_GP_EnablePreviewChanged

		spin_lock(&s5k8aayxmipi_drv_lock);
		S5K8AAYX_MIPICurrentStatus.iMirror = image_mirror;
		spin_unlock(&s5k8aayxmipi_drv_lock);

}





/*****************************************************************************
 * FUNCTION
 *  S5K8AAYX_MIPI_set_dummy
 * DESCRIPTION
 *
 * PARAMETERS
 *  pixels      [IN]
 *  lines       [IN]
 * RETURNS
 *  void
 *****************************************************************************/
void S5K8AAYX_MIPI_set_dummy(kal_uint16 dummy_pixels, kal_uint16 dummy_lines)
{
		/****************************************************
		  * Adjust the extra H-Blanking & V-Blanking.
		  *****************************************************/
		S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000); 
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x044C); 
		
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, dummy_pixels); 
		//S5K8AAYX_MIPI_write_cmos_sensor(0x0F1C, dummy_pixels); 	// Extra H-Blanking
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, dummy_lines); 	// Extra V-Blanking
}   /* S5K8AAYX_MIPI_set_dummy */

/*****************************************************************************
 * FUNCTION
 *  S5K8AAYX_MIPI_Initialize_Setting
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/
void S5K8AAYX_MIPI_Initialize_Setting(void)
{
/******
	Below two groups of setting is supply by Coasia and AVP, for MIPI EVT1 Version,
	they both can work on my module on MT6577+JB sw.

	we could try those two groups of setting on MT6588 .
*******/

#if 0  //Supply by Coasia ,verification ok on mt6577
	S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000);//ADD TEST


	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0010);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	// Reset
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);	// Simmian bug workaround
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1030);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);	// Clear host interrupt so main will wait
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0014);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	// ARM go

#ifdef MIPI_INTERFACE
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x2470);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xB510);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF9ED);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF9E9);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6341);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF9E2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF9DE);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF9DA);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6448);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBC10);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBC08);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4718);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x27CC);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8EDD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2744);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8725);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x26E4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2638);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA6EF);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2604);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA0F1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x25D0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x058F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x24E4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x403E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00DD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1002);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F86);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00DC);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x200A);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE28D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0E3F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00DB);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1002);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F86);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5DD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C3);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0027);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5DD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0024);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02E0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x12D4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10B8);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5C0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1002);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE28D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3403);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE182);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC2A8);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2080);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE08C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE7B4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x039E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE80F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3E0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4624);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE00E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x47B4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE280);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC084);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE08C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x47B4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1DC);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0493);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4624);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE00E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x47B4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1CC);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC8B4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x039C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE003);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3623);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE00E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x38B4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE280);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1002);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE281);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFE7);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBAFF);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x403E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AB);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0248);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE310);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1234);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0DB2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0214);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE594);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE584);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4070);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0800);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0820);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4041);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE280);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01E0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x11B8);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x51B6);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE041);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0094);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1D11);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x008D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x11C0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x21A8);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3FB0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE353);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x31A4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x5BB2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C3);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE085);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xCBB4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C3);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1DBC);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3EB4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2EB2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0193);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0092);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2811);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0194);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0092);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x11A1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0072);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1160);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02B4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4070);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2148);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x14B0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE311);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x013C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3110);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5C3);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D3);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3C1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x110C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x04B0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x41F0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC801);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC82C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1801);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1821);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4008);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x500C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3005);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x60A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D6);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B8);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x05B4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x70AC);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10F4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D6);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x26B0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D7);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0044);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x26B0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D7);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10F6);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D6);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D5);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C5);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x41F0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE594);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0040);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2068);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0054);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1005);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0032);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE584);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE594);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0030);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE584);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFF9);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEAFF);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x28E8);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3370);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1272);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1728);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x112C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x28EC);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x122C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF200);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2340);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0E2C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF400);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0CDC);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x20D4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x06D4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4778);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x46C0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC091);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0467);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2FA7);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xCB1F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x058F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA0F1);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF004);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE51F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD14C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2B43);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8725);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6777);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8E49);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8EDD);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x96FF);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);                             
	                                                                        
	//============================================================          
	// Set IO driving current                                               
	//============================================================          
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x04B4);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0155); // d0~d4                    
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0155); // d5~d9                    
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1555); // gpio1~gpio3              
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0555); // HSYNC,VSYNC,PCLK,SCL,SDA 
	                                                                        
	//============================================================          
	// Analog Settings                                                      
	//============================================================          
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E38);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0476);	//senHal_RegCompBiasNormSf //CDS bias
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0476);	//senHal_RegCompBiasYAv //CDS bias
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0AA0);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	//setot_bUseDigitalHbin //1-Digital, 0-Analog
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E2C);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	//senHal_bUseAnalogVerAv //2-Adding/averaging, 1-Y-Avg, 0-PLA
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E66);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	//senHal_RegBlstEnNorm      
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1250);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFF); 	//senHal_Bls_nSpExpLines  
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1202);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); 	//senHal_Dblr_VcoFreqMHZ  
	                                                                        
	//ADLC Filter                                                           
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1288);                             
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F);	//gisp_dadlc_ResetFilterValue
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1C02);	//gisp_dadlc_SteadyFilterValue
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0006);	//gisp_dadlc_NResetIIrFrames

#else
	// T&P part
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x2460);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xB510);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFB71);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFB6D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFB69);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFB65);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6241);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x490C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6341);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBC10);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBC08);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4718);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2998);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x27B7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2690);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x25DD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2650);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9DC7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x24C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x49CD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2A65);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2A8F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4070);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10B6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x200D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x20B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0B01);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x44F0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10FB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC4EC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1DC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0C02);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE311);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x200C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x20B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x24CC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE151);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10FA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x14B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE4B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x200C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x11B6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x35B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1DE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x5001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE082);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE155);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x50B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE083);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3C05);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE283);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2005);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE082);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x50B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2005);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE082);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE152);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x200D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x35B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1DE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x5001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE082);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE155);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE0B6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1D0F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE281);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x200E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE082);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE0B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x200E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE082);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE152);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10FC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0022);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10F9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE150);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x20F9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1004);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0018);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE28C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5D4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x13FC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x08B8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x08BA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x13F0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0150);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4070);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0150);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4070);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4070);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x40FF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE200);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2004);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0CC1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0147);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE354);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1390);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0C01);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0388);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE5C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4010);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4FF8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE92D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x013E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x5000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xB36C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x80B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1CB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7340);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA0B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE355);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0137);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE355);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1344);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0344);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x18BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1340);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x19B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x133C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x933C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE155);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE045);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4C02);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE289);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6C01);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE289);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0019);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xCA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE355);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0016);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE355);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE355);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0034);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0C02);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE240);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE250);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xB2EC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE355);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02DC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x82B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x22D8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x20BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE380);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x08B6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x08BA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0040);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE380);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x08BA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x87B6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x87B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x83B6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1CB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0016);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE355);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE155);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0016);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE355);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0DBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0DBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA9BC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA9BA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x89BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x024C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA4B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4FF8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE8BD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE58D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00EE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FFA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00EB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEB00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xB100);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xB0B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x082B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA2B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x08B6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x09B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x87B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x87B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x85BC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x83B6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01E4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA4B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x11E0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE155);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE281);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE155);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA6B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA6B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2D16);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x25BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x202A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE242);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x26B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2AB4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x21AC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2ABC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2F96);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE242);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2ABE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2DBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3C2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2DBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA9BC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA9BA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x89BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE155);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE155);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0174);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE0B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE355);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F47);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3E0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE090);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0026);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FBE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE350);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1154);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x05B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0148);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1CBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2CBE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3DB2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE042);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE08C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4C05);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE28C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4055);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE284);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4DB6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1EBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2EBE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3FB2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1C02);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE28C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10AA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE281);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1FB6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1110);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0110);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1BBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x10C1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1BBC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0104);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x100C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x19BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1008);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1AB2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0C01);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE280);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xABBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0C03);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE240);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x81BC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x82BC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF9A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEAFF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF97);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEAFF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE1C7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF94);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEAFF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE590);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xCA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0016);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEA00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2F47);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3E0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE091);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0C02);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE351);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE580);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE580);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE3A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xEAFF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1760);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1718);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x057C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A86);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0E2C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x20C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x187C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC100);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x232C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3600);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3333);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1200);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2107);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0116);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x012E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC350);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0200);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F88);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x042C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x031D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0834);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xB000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0203);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F2C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0555);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF500);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x102C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0101);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xB510);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF8AF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x482C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7F00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2801);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD00A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x482B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8940);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x07C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4828);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2180);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3820);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8B02);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4828);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF8A8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBC10);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBC08);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4718);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xB5F8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF8AA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4821);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7F00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2801);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD03C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4820);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8940);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x07C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD038);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x481D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2218);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3020);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2512);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x5E82);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x5F45);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1B51);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x17CB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F9B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1859);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x108B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x261E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x5F86);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1AB1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x17CC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FA4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1861);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x108C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4917);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8849);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1949);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x18CD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4916);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x810D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4D16);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x886F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8AC5);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x197F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x814F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4F15);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x887F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x197D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x18EB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x818B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4B13);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x885B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x189B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x81CB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4B12);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x885B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x189A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1912);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x820A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4A11);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8B80);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8852);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1812);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x824A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4A0F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8852);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1810);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1900);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8288);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x480E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8840);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1980);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x82C8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBCF8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBC08);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4718);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2350);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x112C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF402);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F94);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xF4A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F98);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F9C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FA0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FA4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FA8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FAC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FB0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4778);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x46C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBF01);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2E89);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1DEF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x5137);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E0F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2EA7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x25AD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x055F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x256B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2D27);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4778);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x46C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xA5ED);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4778);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x46C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2EA7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x4778);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x46C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xC000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE59F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xE12F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2AF5);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8E66);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x04B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0155); // d0~d4
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0155); // d5~d9
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1555); // gpio1~gpio3
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0555); // HSYNC,VSYNC,PCLK,SCL,SDA
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E8E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E92);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0693);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E96);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E9A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0693);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E9E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EA2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0693);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EA6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EAA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0690);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EAE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EB2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0690);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EB6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x025C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EBE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EC2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x025C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EC6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0ECA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0698);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0ECE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0ED2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0698);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0ED6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EDA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EDE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EE2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EE6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EEA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0052);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EEE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EF2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EF6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EFA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0052);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EFE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F02);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F06);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F0A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F0E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F12);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F16);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F1A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F1E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F22);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F26);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F2A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0205);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F2E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0268);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F32);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x068E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F36);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F3A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F3E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F42);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F46);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F4A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0052);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F4E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F52);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F56);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F5A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0055);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F5E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F62);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F66);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F6A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0208);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F6E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F72);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F76);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F7A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0205);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F7E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02C8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F82);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x068E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F86);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F8A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F8E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F92);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F96);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F9A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0142);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F9E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01A2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FA2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0208);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FA6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03BB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FAA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x04AB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FAE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x059B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FB2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0691);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FB6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FBA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0262);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FBE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FC2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FC6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FCA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FCE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FD2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FD6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0062);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FDA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0691);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FDE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FE2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FE6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x029E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FEA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0691);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FEE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FF2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FF6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FFA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0202);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FFE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02CA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x068B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x100A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x100E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1012);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1016);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x101A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x101E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02CA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1022);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x065C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1026);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x102A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x102E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1032);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1036);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x103A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0205);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x103E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1042);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1046);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02C8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x104A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x068E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x104E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1052);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1056);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x105A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x105E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0205);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1062);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02B8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1066);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x068E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x106A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x106E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1072);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0208);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1076);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0210);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x107A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x107E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1082);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1086);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x108A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x108E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0214);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1092);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1096);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x109A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x109E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10A2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10A6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10AA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10AE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0208);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x065F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10B6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0691);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10BA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10C2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10C6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10CA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10CE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10D2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10D6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10DA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10DE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10E2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10E6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10EA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10EE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10F2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10F6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10FA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10FE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1102);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1106);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x110A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x110E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1112);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x069E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1116);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0708);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x111A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x111E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1122);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1126);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x027D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x112A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02C8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x112E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0708);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1132);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E90);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E94);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0663);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E98);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E9C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0332);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EA0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0335);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EA4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0663);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EA8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EAC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EB0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x033D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EB4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0660);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EB8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0102);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EBC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0153);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EC0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0436);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EC4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0487);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EC8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0ECC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0334);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0ED0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x033B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0ED4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0668);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0ED8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0368);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EDC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0337);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EE0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0034);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EE4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EE8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03B5);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EEC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x038D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EF0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0365);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EF4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x033A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EF8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0EFC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0059);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F00);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0031);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F04);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0006);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F08);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F0C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0390);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F10);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0084);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F14);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0056);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F18);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0369);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F1C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0336);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F20);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0035);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F24);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F28);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F2C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F30);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F34);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0430);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F38);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0493);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F3C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x065E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F40);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F44);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F48);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F4C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0052);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F50);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0340);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F54);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0386);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F58);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F5C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0055);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F60);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0340);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F64);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0389);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F68);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F6C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F70);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F74);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0433);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F78);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F7C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F80);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x018D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F84);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F88);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03B3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F8C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0430);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F90);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x04C1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F94);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x065E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F98);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00BD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0F9C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FA0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x025B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FA4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FA8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03F1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FAC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0433);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FB0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x058F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FB4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0661);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FB8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0104);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FBC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0159);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FC0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0438);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FC4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x043E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FC8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FCC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FD0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FD4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FD8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0062);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FDC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FE0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0396);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FE4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0661);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FE8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0139);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FEC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FF0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x046D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FF4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0661);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FF8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0FFC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F9);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x018F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1004);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0327);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1008);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03B5);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x100C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x042D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1010);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x04C3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1014);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x065B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1018);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0081);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x101C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1020);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x018F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1024);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02F8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1028);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03B5);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x102C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1030);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x04C3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1034);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x062C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1038);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x103C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1040);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1044);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1048);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x018D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x104C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1050);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03B3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1054);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0430);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1058);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x04C1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x105C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x065E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1060);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1064);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x016D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1068);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0430);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x106C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x04A1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1070);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x065E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1074);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1078);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x107C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1080);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x033D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1084);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0433);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1088);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0443);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x108C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0107);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1090);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0117);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1094);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0335);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1098);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0345);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x109C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x043B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10A0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x044B);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10A4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10A8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10AC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CD);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10B0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10B4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02FB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10B8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10BC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0401);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10C0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0433);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10C4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x062F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10C8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0661);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10CC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10D4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10D8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10DC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10E0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10E4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10E8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10EC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10F0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10F4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10F8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x10FC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1100);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1104);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1108);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x110C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1110);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1114);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x066C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1118);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x06DF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x111C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1120);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1124);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1128);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x04AF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x112C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x04C1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1130);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x06DF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1134);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1186);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0404);	// Fix ptr type for 74+75
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0C0C);	// Fix ptr type for 76+77
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);	// Fix ptr type for 78+79
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);	// Fix ptr type for 80+81
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x11F0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x026C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1136);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02FF);	// Single to double conversion threshold - need LSB of "1" to apply 4-sampling in new T&P
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x05FF);	// Single to double conversion threshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1224);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008);	// Single offset
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1230);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1234);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0090);	// Double offset factor
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x122A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1236);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x123A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0090);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E4C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1110);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E5C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F4);	// Clamp Level - temporary pending check of black sun
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E6C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000F);	// All caps. Turn off limiter
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000F);	// All caps. Turn off limiter
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E34);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0013);	// Reduce Pixel boost current
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0013);	// Reduce Pixel boost current
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0474);	// CDS bias
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0474);	// CDS bias
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E5E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	// Enable CMP_PD during Vblank
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1208);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0276);	// 630mV ADC_SAT
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E52);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x8999);	// VPIX ~ 3.15V
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0779);	// 3 Charge pump capacitors
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1202);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008);	// Charge Pump frequency
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x11FE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);	// External calibration of DBLR clock
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1204);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);	// No DBLR clock offset, for low frequency
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0AA0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	// 1 - Digital, 0 - Analog
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E2C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	// 1 - Y-Avg, 0- PLA
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1358);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006C);	// Analog - Min DCLK 108MHz for optimal performance at 54MHz clk
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0E66);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	// Enable anti-blooming shutter
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	// Enable anti-blooming shutter
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	// Enable anti-blooming shutter
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1250);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF);
#endif

	//ae
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0D46);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0440);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3CF0);	//lt_uMaxExp_0_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0444);                          
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6590);	//lt_uMaxExp_1_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0448);                          
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBB80);	//lt_uMaxExp_2_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x044C);                          
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3880);	//lt_uMaxExp_3_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0450);                          
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3CF0);	//lt_uCapMaxExp_0_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0454);                          
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6590);	//lt_uCapMaxExp_1_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0458);                          
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xBB80);	//lt_uCapMaxExp_2_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x045C);                          
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3880);	//lt_uCapMaxExp_3_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);	
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0460);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0190);	//lt_uMaxAnGain_0_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0280);	//lt_uMaxAnGain_1_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0540);	//lt_uMaxAnGain_2_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0B00); // max gain 0x0C00=> 12x  ;0x0BO0=>  11x     lt_uMaxAnGain_3_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100); //lt_uMaxDigGain
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3000); //lt_uMaxTotGain
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x042E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010E); //lt_uLimitHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F5); //lt_uLimitLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0DE0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);	//ae_Fade2BlackEnable  F2B off, F2W on
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0D40);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003E); //TVAR_ae_BrAve
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0D4E); //AE_Weight
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0101);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0101);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0101);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0101);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0101);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0101);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0201);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0303);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0303);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0102);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0201);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0403);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0304);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0102);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0201);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0403);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0304);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0102);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0201);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0403);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0304);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0102);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0201);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0303);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0303);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0102);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0201);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0202);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0202);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0102);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1326);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  //gisp_gos_Enable
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x063A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100);  // #TVAR_ash_GASAlpha[0][0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6);  // #TVAR_ash_GASAlpha[0][1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6);  // #TVAR_ash_GASAlpha[0][2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B4);  // #TVAR_ash_GASAlpha[0][3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B4);  // #TVAR_ash_GASAlpha[1][0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6);  // #TVAR_ash_GASAlpha[1][1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6);  // #TVAR_ash_GASAlpha[1][2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D2);  // #TVAR_ash_GASAlpha[1][3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B4);  // #TVAR_ash_GASAlpha[2][0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6);  // #TVAR_ash_GASAlpha[2][1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6);  // #TVAR_ash_GASAlpha[2][2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D2);  // #TVAR_ash_GASAlpha[2][3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B4);  // #TVAR_ash_GASAlpha[3][0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6);  // #TVAR_ash_GASAlpha[3][1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6);  // #TVAR_ash_GASAlpha[3][2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00DC);  // #TVAR_ash_GASAlpha[3][3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F5);  // #TVAR_ash_GASAlpha[4][0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F8);  // #TVAR_ash_GASAlpha[4][1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F8);  // #TVAR_ash_GASAlpha[4][2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100);  // #TVAR_ash_GASAlpha[4][3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0109);  // #TVAR_ash_GASAlpha[5][0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100);  // #TVAR_ash_GASAlpha[5][1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100);  // #TVAR_ash_GASAlpha[5][2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100);  // #TVAR_ash_GASAlpha[5][3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F0);  // #TVAR_ash_GASAlpha[6][0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100);  // #TVAR_ash_GASAlpha[6][1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100);  // #TVAR_ash_GASAlpha[6][2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100);  // #TVAR_ash_GASAlpha[6][3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x067A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064);  // #TVAR_ash_GASBeta[0][0]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014);  // #TVAR_ash_GASBeta[0][1]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014);  // #TVAR_ash_GASBeta[0][2]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[0][3]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E);  // #TVAR_ash_GASBeta[1][0]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);  // #TVAR_ash_GASBeta[1][1]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);  // #TVAR_ash_GASBeta[1][2]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[1][3]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E);  // #TVAR_ash_GASBeta[2][0]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);  // #TVAR_ash_GASBeta[2][1]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);  // #TVAR_ash_GASBeta[2][2]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[2][3]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E);  // #TVAR_ash_GASBeta[3][0]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[3][1]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[3][2]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[3][3]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0046);  // #TVAR_ash_GASBeta[4][0]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[4][1]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[4][2]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[4][3]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0032);  // #TVAR_ash_GASBeta[5][0]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[5][1]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[5][2]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[5][3]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0055);  // #TVAR_ash_GASBeta[6][0]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[6][1]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[6][2]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #TVAR_ash_GASBeta[6][3]
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x06BA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);  //ash_bLumaMode
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0632);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F5); //TVAR_ash_CGrasAlphas_0_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F8); //TVAR_ash_CGrasAlphas_1_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F8); //TVAR_ash_CGrasAlphas_2_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100); //TVAR_ash_CGrasAlphas_3_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0672);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100); //TVAR_ash_GASOutdoorAlpha_0_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100); //TVAR_ash_GASOutdoorAlpha_1_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100); //TVAR_ash_GASOutdoorAlpha_2_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100); //TVAR_ash_GASOutdoorAlpha_3_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x06B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //ash_GASOutdoorBeta_0_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //ash_GASOutdoorBeta_1_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //ash_GASOutdoorBeta_2_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //ash_GASOutdoorBeta_3_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0624);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009a);  //TVAR_ash_AwbAshCord_0_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00d3);  //TVAR_ash_AwbAshCord_1_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00d4);  //TVAR_ash_AwbAshCord_2_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x012c);  //TVAR_ash_AwbAshCord_3_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0162);  //TVAR_ash_AwbAshCord_4_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0190);  //TVAR_ash_AwbAshCord_5_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01a0);  //TVAR_ash_AwbAshCord_6_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x06CC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0280);  //ash_uParabolicCenterX	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01E0);  //ash_uParabolicCenterY	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x06D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000D);  //ash_uParabolicScalingA	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000F);  //ash_uParabolicScalingB	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x06C6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);  //ash_bParabolicEstimation
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x347C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0254); //Tune_wbt_GAS_0_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01B3); //Tune_wbt_GAS_1_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0163); //Tune_wbt_GAS_2_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0120); //Tune_wbt_GAS_3_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00EE); //Tune_wbt_GAS_4_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CC); //Tune_wbt_GAS_5_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00BE); //Tune_wbt_GAS_6_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C5); //Tune_wbt_GAS_7_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E1); //Tune_wbt_GAS_8_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0113); //Tune_wbt_GAS_9_		
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0151); //Tune_wbt_GAS_10_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01A0); //Tune_wbt_GAS_11_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0203); //Tune_wbt_GAS_12_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01F6); //Tune_wbt_GAS_13_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x018A); //Tune_wbt_GAS_14_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x013C); //Tune_wbt_GAS_15_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F2); //Tune_wbt_GAS_16_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00BD); //Tune_wbt_GAS_17_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009D); //Tune_wbt_GAS_18_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x008D); //Tune_wbt_GAS_19_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0095); //Tune_wbt_GAS_20_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B3); //Tune_wbt_GAS_21_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E7); //Tune_wbt_GAS_22_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0136); //Tune_wbt_GAS_23_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x018E); //Tune_wbt_GAS_24_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01EA); //Tune_wbt_GAS_25_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01B1); //Tune_wbt_GAS_26_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014C); //Tune_wbt_GAS_27_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FA); //Tune_wbt_GAS_28_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AC); //Tune_wbt_GAS_29_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0078); //Tune_wbt_GAS_30_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0056); //Tune_wbt_GAS_31_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004B); //Tune_wbt_GAS_32_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0053); //Tune_wbt_GAS_33_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006C); //Tune_wbt_GAS_34_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A1); //Tune_wbt_GAS_35_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E7); //Tune_wbt_GAS_36_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014E); //Tune_wbt_GAS_37_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01A9); //Tune_wbt_GAS_38_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x017F); //Tune_wbt_GAS_39_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0121); //Tune_wbt_GAS_40_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C7); //Tune_wbt_GAS_41_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007D); //Tune_wbt_GAS_42_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0049); //Tune_wbt_GAS_43_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A); //Tune_wbt_GAS_44_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0021); //Tune_wbt_GAS_45_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0027); //Tune_wbt_GAS_46_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003E); //Tune_wbt_GAS_47_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006E); //Tune_wbt_GAS_48_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B4); //Tune_wbt_GAS_49_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0116); //Tune_wbt_GAS_50_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0178); //Tune_wbt_GAS_51_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0168); //Tune_wbt_GAS_52_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0105); //Tune_wbt_GAS_53_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A9); //Tune_wbt_GAS_54_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005F); //Tune_wbt_GAS_55_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002D); //Tune_wbt_GAS_56_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0012); //Tune_wbt_GAS_57_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); //Tune_wbt_GAS_58_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000E); //Tune_wbt_GAS_59_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); //Tune_wbt_GAS_60_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004F); //Tune_wbt_GAS_61_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0095); //Tune_wbt_GAS_62_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F6); //Tune_wbt_GAS_63_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015A); //Tune_wbt_GAS_64_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015C); //Tune_wbt_GAS_65_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F6); //Tune_wbt_GAS_66_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0099); //Tune_wbt_GAS_67_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0050); //Tune_wbt_GAS_68_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0022); //Tune_wbt_GAS_69_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009); //Tune_wbt_GAS_70_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //Tune_wbt_GAS_71_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003); //Tune_wbt_GAS_72_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0017); //Tune_wbt_GAS_73_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0043); //Tune_wbt_GAS_74_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0084); //Tune_wbt_GAS_75_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6); //Tune_wbt_GAS_76_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014F); //Tune_wbt_GAS_77_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015A); //Tune_wbt_GAS_78_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F7); //Tune_wbt_GAS_79_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009C); //Tune_wbt_GAS_80_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0054); //Tune_wbt_GAS_81_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0024); //Tune_wbt_GAS_82_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009); //Tune_wbt_GAS_83_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //Tune_wbt_GAS_84_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004); //Tune_wbt_GAS_85_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0018); //Tune_wbt_GAS_86_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0044); //Tune_wbt_GAS_87_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0086); //Tune_wbt_GAS_88_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6); //Tune_wbt_GAS_89_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014E); //Tune_wbt_GAS_90_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0162); //Tune_wbt_GAS_91_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0106); //Tune_wbt_GAS_92_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AA); //Tune_wbt_GAS_93_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0062); //Tune_wbt_GAS_94_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0030); //Tune_wbt_GAS_95_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); //Tune_wbt_GAS_96_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000B); //Tune_wbt_GAS_97_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); //Tune_wbt_GAS_98_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0025); //Tune_wbt_GAS_99_	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0053); //Tune_wbt_GAS_100_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0095); //Tune_wbt_GAS_101_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F7); //Tune_wbt_GAS_102_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015C); //Tune_wbt_GAS_103_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x017C); //Tune_wbt_GAS_104_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0122); //Tune_wbt_GAS_105_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CB); //Tune_wbt_GAS_106_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); //Tune_wbt_GAS_107_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004C); //Tune_wbt_GAS_108_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0030); //Tune_wbt_GAS_109_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); //Tune_wbt_GAS_110_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002C); //Tune_wbt_GAS_111_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0043); //Tune_wbt_GAS_112_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0074); //Tune_wbt_GAS_113_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B7); //Tune_wbt_GAS_114_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x011B); //Tune_wbt_GAS_115_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x017A); //Tune_wbt_GAS_116_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01A8); //Tune_wbt_GAS_117_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x013C); //Tune_wbt_GAS_118_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F2); //Tune_wbt_GAS_119_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AA); //Tune_wbt_GAS_120_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0077); //Tune_wbt_GAS_121_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0058); //Tune_wbt_GAS_122_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004E); //Tune_wbt_GAS_123_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0056); //Tune_wbt_GAS_124_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0070); //Tune_wbt_GAS_125_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A2); //Tune_wbt_GAS_126_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00EB); //Tune_wbt_GAS_127_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0146); //Tune_wbt_GAS_128_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x019D); //Tune_wbt_GAS_129_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D9); //Tune_wbt_GAS_130_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x016A); //Tune_wbt_GAS_131_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0121); //Tune_wbt_GAS_132_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E5); //Tune_wbt_GAS_133_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B5); //Tune_wbt_GAS_134_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0098); //Tune_wbt_GAS_135_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x008C); //Tune_wbt_GAS_136_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0094); //Tune_wbt_GAS_137_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B1); //Tune_wbt_GAS_138_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E4); //Tune_wbt_GAS_139_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0126); //Tune_wbt_GAS_140_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x017D); //Tune_wbt_GAS_141_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01DE); //Tune_wbt_GAS_142_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01A0); //Tune_wbt_GAS_143_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FA); //Tune_wbt_GAS_144_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00BE); //Tune_wbt_GAS_145_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0095); //Tune_wbt_GAS_146_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007E); //Tune_wbt_GAS_147_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006F); //Tune_wbt_GAS_148_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006A); //Tune_wbt_GAS_149_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006C); //Tune_wbt_GAS_150_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007C); //Tune_wbt_GAS_151_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0094); //Tune_wbt_GAS_152_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B4); //Tune_wbt_GAS_153_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E7); //Tune_wbt_GAS_154_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014D); //Tune_wbt_GAS_155_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x013A); //Tune_wbt_GAS_156_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D4); //Tune_wbt_GAS_157_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A7); //Tune_wbt_GAS_158_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0081); //Tune_wbt_GAS_159_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0068); //Tune_wbt_GAS_160_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005A); //Tune_wbt_GAS_161_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0053); //Tune_wbt_GAS_162_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0059); //Tune_wbt_GAS_163_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0068); //Tune_wbt_GAS_164_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0082); //Tune_wbt_GAS_165_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A7); //Tune_wbt_GAS_166_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D5); //Tune_wbt_GAS_167_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x011F); //Tune_wbt_GAS_168_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); //Tune_wbt_GAS_169_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AB); //Tune_wbt_GAS_170_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0084); //Tune_wbt_GAS_171_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005D); //Tune_wbt_GAS_172_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0044); //Tune_wbt_GAS_173_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0034); //Tune_wbt_GAS_174_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0030); //Tune_wbt_GAS_175_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0035); //Tune_wbt_GAS_176_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0044); //Tune_wbt_GAS_177_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005F); //Tune_wbt_GAS_178_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0081); //Tune_wbt_GAS_179_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B3); //Tune_wbt_GAS_180_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F3); //Tune_wbt_GAS_181_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00DC); //Tune_wbt_GAS_182_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0094); //Tune_wbt_GAS_183_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006B); //Tune_wbt_GAS_184_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0044); //Tune_wbt_GAS_185_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0029); //Tune_wbt_GAS_186_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A); //Tune_wbt_GAS_187_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015); //Tune_wbt_GAS_188_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A); //Tune_wbt_GAS_189_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); //Tune_wbt_GAS_190_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0042); //Tune_wbt_GAS_191_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); //Tune_wbt_GAS_192_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0094); //Tune_wbt_GAS_193_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D0); //Tune_wbt_GAS_194_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CD); //Tune_wbt_GAS_195_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0087); //Tune_wbt_GAS_196_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005B); //Tune_wbt_GAS_197_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0034); //Tune_wbt_GAS_198_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A); //Tune_wbt_GAS_199_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C); //Tune_wbt_GAS_200_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008); //Tune_wbt_GAS_201_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000B); //Tune_wbt_GAS_202_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0018); //Tune_wbt_GAS_203_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0031); //Tune_wbt_GAS_204_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0055); //Tune_wbt_GAS_205_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0085); //Tune_wbt_GAS_206_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00BD); //Tune_wbt_GAS_207_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C6); //Tune_wbt_GAS_208_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007E); //Tune_wbt_GAS_209_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0053); //Tune_wbt_GAS_210_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002C); //Tune_wbt_GAS_211_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0013); //Tune_wbt_GAS_212_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0006); //Tune_wbt_GAS_213_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); //Tune_wbt_GAS_214_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004); //Tune_wbt_GAS_215_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0011); //Tune_wbt_GAS_216_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A); //Tune_wbt_GAS_217_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004D); //Tune_wbt_GAS_218_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007C); //Tune_wbt_GAS_219_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B7); //Tune_wbt_GAS_220_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C6); //Tune_wbt_GAS_221_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007D); //Tune_wbt_GAS_222_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0054); //Tune_wbt_GAS_223_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002D); //Tune_wbt_GAS_224_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0013); //Tune_wbt_GAS_225_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005); //Tune_wbt_GAS_226_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); //Tune_wbt_GAS_227_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004); //Tune_wbt_GAS_228_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); //Tune_wbt_GAS_229_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A); //Tune_wbt_GAS_230_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004C); //Tune_wbt_GAS_231_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007C); //Tune_wbt_GAS_232_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B7); //Tune_wbt_GAS_233_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CB); //Tune_wbt_GAS_234_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0083); //Tune_wbt_GAS_235_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005A); //Tune_wbt_GAS_236_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0034); //Tune_wbt_GAS_237_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A); //Tune_wbt_GAS_238_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000B); //Tune_wbt_GAS_239_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0007); //Tune_wbt_GAS_240_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); //Tune_wbt_GAS_241_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0017); //Tune_wbt_GAS_242_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0031); //Tune_wbt_GAS_243_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0052); //Tune_wbt_GAS_244_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0082); //Tune_wbt_GAS_245_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00BB); //Tune_wbt_GAS_246_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00DF); //Tune_wbt_GAS_247_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0092); //Tune_wbt_GAS_248_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0069); //Tune_wbt_GAS_249_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0043); //Tune_wbt_GAS_250_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A); //Tune_wbt_GAS_251_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001B); //Tune_wbt_GAS_252_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015); //Tune_wbt_GAS_253_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A); //Tune_wbt_GAS_254_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); //Tune_wbt_GAS_255_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0042); //Tune_wbt_GAS_256_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0063); //Tune_wbt_GAS_257_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0092); //Tune_wbt_GAS_258_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CE); //Tune_wbt_GAS_259_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100); //Tune_wbt_GAS_260_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A3); //Tune_wbt_GAS_261_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007B); //Tune_wbt_GAS_262_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0056); //Tune_wbt_GAS_263_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003D); //Tune_wbt_GAS_264_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0030); //Tune_wbt_GAS_265_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002C); //Tune_wbt_GAS_266_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0030); //Tune_wbt_GAS_267_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003F); //Tune_wbt_GAS_268_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0058); //Tune_wbt_GAS_269_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007A); //Tune_wbt_GAS_270_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A8); //Tune_wbt_GAS_271_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E5); //Tune_wbt_GAS_272_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0139); //Tune_wbt_GAS_273_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C6); //Tune_wbt_GAS_274_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0097); //Tune_wbt_GAS_275_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0077); //Tune_wbt_GAS_276_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0060); //Tune_wbt_GAS_277_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0053); //Tune_wbt_GAS_278_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0050); //Tune_wbt_GAS_279_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0053); //Tune_wbt_GAS_280_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); //Tune_wbt_GAS_281_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007D); //Tune_wbt_GAS_282_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009D); //Tune_wbt_GAS_283_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CD); //Tune_wbt_GAS_284_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x011F); //Tune_wbt_GAS_285_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0192); //Tune_wbt_GAS_286_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00F2); //Tune_wbt_GAS_287_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B6); //Tune_wbt_GAS_288_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x008D); //Tune_wbt_GAS_289_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0075); //Tune_wbt_GAS_290_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0066); //Tune_wbt_GAS_291_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0060); //Tune_wbt_GAS_292_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); //Tune_wbt_GAS_293_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0072); //Tune_wbt_GAS_294_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x008A); //Tune_wbt_GAS_295_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AB); //Tune_wbt_GAS_296_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00DD); //Tune_wbt_GAS_297_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0145); //Tune_wbt_GAS_298_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0136); //Tune_wbt_GAS_299_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D0); //Tune_wbt_GAS_300_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A5); //Tune_wbt_GAS_301_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007E); //Tune_wbt_GAS_302_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); //Tune_wbt_GAS_303_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0055); //Tune_wbt_GAS_304_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004E); //Tune_wbt_GAS_305_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0053); //Tune_wbt_GAS_306_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0061); //Tune_wbt_GAS_307_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007C); //Tune_wbt_GAS_308_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A1); //Tune_wbt_GAS_309_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CE); //Tune_wbt_GAS_310_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x011C); //Tune_wbt_GAS_311_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FC); //Tune_wbt_GAS_312_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AA); //Tune_wbt_GAS_313_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0082); //Tune_wbt_GAS_314_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005A); //Tune_wbt_GAS_315_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0041); //Tune_wbt_GAS_316_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0030); //Tune_wbt_GAS_317_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A); //Tune_wbt_GAS_318_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0030); //Tune_wbt_GAS_319_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003E); //Tune_wbt_GAS_320_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0059); //Tune_wbt_GAS_321_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007A); //Tune_wbt_GAS_322_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AC); //Tune_wbt_GAS_323_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00EE); //Tune_wbt_GAS_324_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D8); //Tune_wbt_GAS_325_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0094); //Tune_wbt_GAS_326_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006A); //Tune_wbt_GAS_327_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0043); //Tune_wbt_GAS_328_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); //Tune_wbt_GAS_329_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0016); //Tune_wbt_GAS_330_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0012); //Tune_wbt_GAS_331_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0016); //Tune_wbt_GAS_332_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); //Tune_wbt_GAS_333_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003E); //Tune_wbt_GAS_334_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0061); //Tune_wbt_GAS_335_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0090); //Tune_wbt_GAS_336_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CE); //Tune_wbt_GAS_337_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CC); //Tune_wbt_GAS_338_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0087); //Tune_wbt_GAS_339_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005C); //Tune_wbt_GAS_340_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0034); //Tune_wbt_GAS_341_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0019); //Tune_wbt_GAS_342_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); //Tune_wbt_GAS_343_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005); //Tune_wbt_GAS_344_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009); //Tune_wbt_GAS_345_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015); //Tune_wbt_GAS_346_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002E); //Tune_wbt_GAS_347_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0052); //Tune_wbt_GAS_348_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0082); //Tune_wbt_GAS_349_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00BE); //Tune_wbt_GAS_350_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C5); //Tune_wbt_GAS_351_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007F); //Tune_wbt_GAS_352_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0054); //Tune_wbt_GAS_353_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002D); //Tune_wbt_GAS_354_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0013); //Tune_wbt_GAS_355_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004); //Tune_wbt_GAS_356_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //Tune_wbt_GAS_357_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); //Tune_wbt_GAS_358_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000E); //Tune_wbt_GAS_359_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0027); //Tune_wbt_GAS_360_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004A); //Tune_wbt_GAS_361_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007A); //Tune_wbt_GAS_362_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B5); //Tune_wbt_GAS_363_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C4); //Tune_wbt_GAS_364_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); //Tune_wbt_GAS_365_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0056); //Tune_wbt_GAS_366_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002E); //Tune_wbt_GAS_367_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); //Tune_wbt_GAS_368_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004); //Tune_wbt_GAS_369_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //Tune_wbt_GAS_370_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); //Tune_wbt_GAS_371_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000E); //Tune_wbt_GAS_372_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0027); //Tune_wbt_GAS_373_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004A); //Tune_wbt_GAS_374_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0079); //Tune_wbt_GAS_375_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B6); //Tune_wbt_GAS_376_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CA); //Tune_wbt_GAS_377_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0086); //Tune_wbt_GAS_378_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005C); //Tune_wbt_GAS_379_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0035); //Tune_wbt_GAS_380_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001B); //Tune_wbt_GAS_381_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); //Tune_wbt_GAS_382_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005); //Tune_wbt_GAS_383_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008); //Tune_wbt_GAS_384_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015); //Tune_wbt_GAS_385_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002F); //Tune_wbt_GAS_386_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0050); //Tune_wbt_GAS_387_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007F); //Tune_wbt_GAS_388_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00BA); //Tune_wbt_GAS_389_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00DE); //Tune_wbt_GAS_390_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0094); //Tune_wbt_GAS_391_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006B); //Tune_wbt_GAS_392_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0045); //Tune_wbt_GAS_393_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A); //Tune_wbt_GAS_394_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A); //Tune_wbt_GAS_395_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0013); //Tune_wbt_GAS_396_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0018); //Tune_wbt_GAS_397_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0025); //Tune_wbt_GAS_398_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003F); //Tune_wbt_GAS_399_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005F); //Tune_wbt_GAS_400_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0090); //Tune_wbt_GAS_401_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CD); //Tune_wbt_GAS_402_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0101); //Tune_wbt_GAS_403_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A6); //Tune_wbt_GAS_404_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007D); //Tune_wbt_GAS_405_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0057); //Tune_wbt_GAS_406_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003E); //Tune_wbt_GAS_407_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002F); //Tune_wbt_GAS_408_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A); //Tune_wbt_GAS_409_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002E); //Tune_wbt_GAS_410_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003B); //Tune_wbt_GAS_411_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0055); //Tune_wbt_GAS_412_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0077); //Tune_wbt_GAS_413_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A5); //Tune_wbt_GAS_414_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E3); //Tune_wbt_GAS_415_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0139); //Tune_wbt_GAS_416_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C6); //Tune_wbt_GAS_417_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0099); //Tune_wbt_GAS_418_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0078); //Tune_wbt_GAS_419_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0060); //Tune_wbt_GAS_420_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0052); //Tune_wbt_GAS_421_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004D); //Tune_wbt_GAS_422_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0051); //Tune_wbt_GAS_423_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0060); //Tune_wbt_GAS_424_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0079); //Tune_wbt_GAS_425_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009A); //Tune_wbt_GAS_426_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00CA); //Tune_wbt_GAS_427_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0120); //Tune_wbt_GAS_428_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x016F); //Tune_wbt_GAS_429_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D5); //Tune_wbt_GAS_430_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009D); //Tune_wbt_GAS_431_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007A); //Tune_wbt_GAS_432_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0065); //Tune_wbt_GAS_433_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0058); //Tune_wbt_GAS_434_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0054); //Tune_wbt_GAS_435_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0056); //Tune_wbt_GAS_436_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0063); //Tune_wbt_GAS_437_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007A); //Tune_wbt_GAS_438_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0095); //Tune_wbt_GAS_439_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C5); //Tune_wbt_GAS_440_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x011E); //Tune_wbt_GAS_441_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010E); //Tune_wbt_GAS_442_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B5); //Tune_wbt_GAS_443_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x008A); //Tune_wbt_GAS_444_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006B); //Tune_wbt_GAS_445_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0057); //Tune_wbt_GAS_446_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004B); //Tune_wbt_GAS_447_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0046); //Tune_wbt_GAS_448_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004A); //Tune_wbt_GAS_449_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0058); //Tune_wbt_GAS_450_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006E); //Tune_wbt_GAS_451_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0090); //Tune_wbt_GAS_452_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B7); //Tune_wbt_GAS_453_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00EE); //Tune_wbt_GAS_454_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D7); //Tune_wbt_GAS_455_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0091); //Tune_wbt_GAS_456_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006C); //Tune_wbt_GAS_457_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004C); //Tune_wbt_GAS_458_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0038); //Tune_wbt_GAS_459_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A); //Tune_wbt_GAS_460_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0026); //Tune_wbt_GAS_461_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002A); //Tune_wbt_GAS_462_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0038); //Tune_wbt_GAS_463_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0050); //Tune_wbt_GAS_464_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006D); //Tune_wbt_GAS_465_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009C); //Tune_wbt_GAS_466_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C7); //Tune_wbt_GAS_467_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B5); //Tune_wbt_GAS_468_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007E); //Tune_wbt_GAS_469_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0058); //Tune_wbt_GAS_470_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0038); //Tune_wbt_GAS_471_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0021); //Tune_wbt_GAS_472_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0015); //Tune_wbt_GAS_473_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0011); //Tune_wbt_GAS_474_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); //Tune_wbt_GAS_475_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0022); //Tune_wbt_GAS_476_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003A); //Tune_wbt_GAS_477_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0057); //Tune_wbt_GAS_478_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0083); //Tune_wbt_GAS_479_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AF); //Tune_wbt_GAS_480_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A8); //Tune_wbt_GAS_481_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0071); //Tune_wbt_GAS_482_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004B); //Tune_wbt_GAS_483_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002B); //Tune_wbt_GAS_484_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); //Tune_wbt_GAS_485_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009); //Tune_wbt_GAS_486_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005); //Tune_wbt_GAS_487_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008); //Tune_wbt_GAS_488_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); //Tune_wbt_GAS_489_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002B); //Tune_wbt_GAS_490_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004A); //Tune_wbt_GAS_491_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0076); //Tune_wbt_GAS_492_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009F); //Tune_wbt_GAS_493_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A1); //Tune_wbt_GAS_494_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006B); //Tune_wbt_GAS_495_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0045); //Tune_wbt_GAS_496_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0024); //Tune_wbt_GAS_497_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000F); //Tune_wbt_GAS_498_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003); //Tune_wbt_GAS_499_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //Tune_wbt_GAS_500_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); //Tune_wbt_GAS_501_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000E); //Tune_wbt_GAS_502_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0024); //Tune_wbt_GAS_503_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0042); //Tune_wbt_GAS_504_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006E); //Tune_wbt_GAS_505_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0098); //Tune_wbt_GAS_506_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A1); //Tune_wbt_GAS_507_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006C); //Tune_wbt_GAS_508_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0046); //Tune_wbt_GAS_509_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0027); //Tune_wbt_GAS_510_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); //Tune_wbt_GAS_511_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004); //Tune_wbt_GAS_512_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //Tune_wbt_GAS_513_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); //Tune_wbt_GAS_514_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000E); //Tune_wbt_GAS_515_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0024); //Tune_wbt_GAS_516_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0043); //Tune_wbt_GAS_517_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006E); //Tune_wbt_GAS_518_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0099); //Tune_wbt_GAS_519_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AA); //Tune_wbt_GAS_520_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0073); //Tune_wbt_GAS_521_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004D); //Tune_wbt_GAS_522_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002D); //Tune_wbt_GAS_523_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0016); //Tune_wbt_GAS_524_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0009); //Tune_wbt_GAS_525_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0005); //Tune_wbt_GAS_526_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0008); //Tune_wbt_GAS_527_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); //Tune_wbt_GAS_528_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002C); //Tune_wbt_GAS_529_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0049); //Tune_wbt_GAS_530_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0076); //Tune_wbt_GAS_531_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009C); //Tune_wbt_GAS_532_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00BD); //Tune_wbt_GAS_533_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007F); //Tune_wbt_GAS_534_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0058); //Tune_wbt_GAS_535_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003A); //Tune_wbt_GAS_536_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0024); //Tune_wbt_GAS_537_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0018); //Tune_wbt_GAS_538_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0012); //Tune_wbt_GAS_539_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0016); //Tune_wbt_GAS_540_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); //Tune_wbt_GAS_541_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003B); //Tune_wbt_GAS_542_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0058); //Tune_wbt_GAS_543_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0082); //Tune_wbt_GAS_544_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AB); //Tune_wbt_GAS_545_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00DC); //Tune_wbt_GAS_546_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x008F); //Tune_wbt_GAS_547_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006A); //Tune_wbt_GAS_548_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004C); //Tune_wbt_GAS_549_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0038); //Tune_wbt_GAS_550_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002C); //Tune_wbt_GAS_551_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); //Tune_wbt_GAS_552_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x002C); //Tune_wbt_GAS_553_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0038); //Tune_wbt_GAS_554_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0050); //Tune_wbt_GAS_555_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006C); //Tune_wbt_GAS_556_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0096); //Tune_wbt_GAS_557_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00C2); //Tune_wbt_GAS_558_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0117); //Tune_wbt_GAS_559_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00AF); //Tune_wbt_GAS_560_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0083); //Tune_wbt_GAS_561_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0068); //Tune_wbt_GAS_562_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0054); //Tune_wbt_GAS_563_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004A); //Tune_wbt_GAS_564_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0046); //Tune_wbt_GAS_565_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004A); //Tune_wbt_GAS_566_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0058); //Tune_wbt_GAS_567_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006D); //Tune_wbt_GAS_568_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x008A); //Tune_wbt_GAS_569_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B4); //Tune_wbt_GAS_570_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FB); //Tune_wbt_GAS_571_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1348);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B36);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);// awbb_IndoorGrZones_ZInfo_m_GridStep */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B3A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F3);// awbb_IndoorGrZones_ZInfo_m_BMin */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02CB);// awbb_IndoorGrZones_ZInfo_m_BMax */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B38);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010);// awbb_IndoorGrZones_ZInfo_m_GridSz */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0AE6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0385);// 0352 03E1 awbb_IndoorGrZones_m_BGrid_0__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03D8);// 038C 0413 awbb_IndoorGrZones_m_BGrid_0__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x032A);// 0321 039E awbb_IndoorGrZones_m_BGrid_1__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03C5);// 03A6 0416 awbb_IndoorGrZones_m_BGrid_1__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02F5);// 02EC 0367 awbb_IndoorGrZones_m_BGrid_2__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x039D);// 03A0 03F3 awbb_IndoorGrZones_m_BGrid_2__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02D3);// 02CA 032D awbb_IndoorGrZones_m_BGrid_3__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0372);// 038D 03C5 awbb_IndoorGrZones_m_BGrid_3__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02B1);// 02A8 02FD awbb_IndoorGrZones_m_BGrid_4__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x033E);// 036E 038F awbb_IndoorGrZones_m_BGrid_4__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x028A);// 0281 02D3 awbb_IndoorGrZones_m_BGrid_5__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0322);// 0344 0365 awbb_IndoorGrZones_m_BGrid_5__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0268);// 025F 02AA awbb_IndoorGrZones_m_BGrid_6__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02FD);// 0327 033E awbb_IndoorGrZones_m_BGrid_6__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0248);// 023F 028D awbb_IndoorGrZones_m_BGrid_7__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02EF);// 0302 0310 awbb_IndoorGrZones_m_BGrid_7__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x022F);// 0226 0271 awbb_IndoorGrZones_m_BGrid_8__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02D5);// 02DC 02F1 awbb_IndoorGrZones_m_BGrid_8__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0219);// 0210 025A awbb_IndoorGrZones_m_BGrid_9__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02C2);// 02B9 02D2 awbb_IndoorGrZones_m_BGrid_9__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0206);// 01FD 0249 awbb_IndoorGrZones_m_BGrid_10__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A3);// 029A 02B9 awbb_IndoorGrZones_m_BGrid_10__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01F0);// 01E7 0238 awbb_IndoorGrZones_m_BGrid_11__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0286);// 027D 02A2 awbb_IndoorGrZones_m_BGrid_11__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01E3);// 01DA 021B awbb_IndoorGrZones_m_BGrid_12__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0268);// 025F 0289 awbb_IndoorGrZones_m_BGrid_12__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01D6);// 01CD 0200 awbb_IndoorGrZones_m_BGrid_13__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x024E);// 0245 026C awbb_IndoorGrZones_m_BGrid_13__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01DD);// 01D4 01FC awbb_IndoorGrZones_m_BGrid_14__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x022A);// 0221 024F awbb_IndoorGrZones_m_BGrid_14__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0210);// 0207 021E awbb_IndoorGrZones_m_BGrid_15__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01F2);// 01E9 022C awbb_IndoorGrZones_m_BGrid_15__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_IndoorGrZones_m_BGrid_16__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_IndoorGrZones_m_BGrid_16__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_IndoorGrZones_m_BGrid_17__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_IndoorGrZones_m_BGrid_17__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_IndoorGrZones_m_BGrid_18__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_IndoorGrZones_m_BGrid_18__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_IndoorGrZones_m_BGrid_19__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_IndoorGrZones_m_BGrid_19__m_right */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BAA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0006);// awbb_LowBrGrZones_ZInfo_m_GridStep */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BAE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00CC);// 010E awbb_LowBrGrZones_ZInfo_m_BMin */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02F3);// 02E9 awbb_LowBrGrZones_ZInfo_m_BMax */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BAC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A);// 0009 awbb_LowBrGrZones_ZInfo_m_GridSz */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B7A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x036C);// 0374 038C awbb_LowBrGrZones_m_BGrid_0__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03C6);// 03CE 03DA awbb_LowBrGrZones_m_BGrid_0__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02EE);// 02F6 030E awbb_LowBrGrZones_m_BGrid_1__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03F9);// 0401 03E9 awbb_LowBrGrZones_m_BGrid_1__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02BE);// 02C6 02A2 awbb_LowBrGrZones_m_BGrid_2__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03DF);// 03E7 03C2 awbb_LowBrGrZones_m_BGrid_2__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x027A);// 0282 0259 awbb_LowBrGrZones_m_BGrid_3__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03AE);// 03B6 038A awbb_LowBrGrZones_m_BGrid_3__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0234);// 023C 0218 awbb_LowBrGrZones_m_BGrid_4__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0376);// 037E 0352 awbb_LowBrGrZones_m_BGrid_4__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0204);// 020C 01F4 awbb_LowBrGrZones_m_BGrid_5__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x033E);// 0346 02E1 awbb_LowBrGrZones_m_BGrid_5__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01E0);// 01E8 01D7 awbb_LowBrGrZones_m_BGrid_6__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02CD);// 02D5 028E awbb_LowBrGrZones_m_BGrid_6__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01C3);// 01CB 01CB awbb_LowBrGrZones_m_BGrid_7__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x027A);// 0282 0258 awbb_LowBrGrZones_m_BGrid_7__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01B7);// 01BF 022B awbb_LowBrGrZones_m_BGrid_8__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0244);// 024C 01CC awbb_LowBrGrZones_m_BGrid_8__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01FE);// 01F8 0000 awbb_LowBrGrZones_m_BGrid_9__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01DD);// 0201 0000 awbb_LowBrGrZones_m_BGrid_9__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_LowBrGrZones_m_BGrid_10__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_LowBrGrZones_m_BGrid_10__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_LowBrGrZones_m_BGrid_11__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 0000 awbb_LowBrGrZones_m_BGrid_11__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B70);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);// awbb_OutdoorGrZones_ZInfo_m_GridStep */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B74);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01E3);// awbb_OutdoorGrZones_ZInfo_m_BMin */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0270);// awbb_OutdoorGrZones_ZInfo_m_BMax */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B72);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0006);// awbb_OutdoorGrZones_ZInfo_m_GridSz */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B40);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x028A);// 029E awbb_OutdoorGrZones_m_BGrid_0__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A1);// 02C8 awbb_OutdoorGrZones_m_BGrid_0__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0263);// 0281 awbb_OutdoorGrZones_m_BGrid_1__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02C0);// 02C8 awbb_OutdoorGrZones_m_BGrid_1__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x024C);// 0266 awbb_OutdoorGrZones_m_BGrid_2__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02BE);// 02AC awbb_OutdoorGrZones_m_BGrid_2__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x023D);// 0251 awbb_OutdoorGrZones_m_BGrid_3__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A6);// 028E awbb_OutdoorGrZones_m_BGrid_3__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0243);// 023D awbb_OutdoorGrZones_m_BGrid_4__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0289);// 0275 awbb_OutdoorGrZones_m_BGrid_4__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x026F);// 0228 awbb_OutdoorGrZones_m_BGrid_5__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x025D);// 025D awbb_OutdoorGrZones_m_BGrid_5__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0228 awbb_OutdoorGrZones_m_BGrid_6__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0243 awbb_OutdoorGrZones_m_BGrid_6__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_7__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_7__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_8__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_8__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_9__m_left   */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_9__m_right  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_10__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_10__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_11__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_OutdoorGrZones_m_BGrid_11__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BC8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);// awbb_CWSkinZone_ZInfo_m_GridStep */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BCC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010F);// awbb_CWSkinZone_ZInfo_m_BMin */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x018F);// awbb_CWSkinZone_ZInfo_m_BMax */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BCA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);// awbb_CWSkinZone_ZInfo_m_GridSz */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BB4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03E7);// awbb_CWSkinZone_m_BGrid_0__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03F8);// awbb_CWSkinZone_m_BGrid_0__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03A7);// awbb_CWSkinZone_m_BGrid_1__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FC);// awbb_CWSkinZone_m_BGrid_1__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0352);// awbb_CWSkinZone_m_BGrid_2__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03D0);// awbb_CWSkinZone_m_BGrid_2__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0322);// awbb_CWSkinZone_m_BGrid_3__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x039E);// awbb_CWSkinZone_m_BGrid_3__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x032B);// awbb_CWSkinZone_m_BGrid_4__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x034D);// awbb_CWSkinZone_m_BGrid_4__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BE6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0006);// awbb_DLSkinZone_ZInfo_m_GridStep */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BEA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x019E);// awbb_DLSkinZone_ZInfo_m_BMin */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0257);// awbb_DLSkinZone_ZInfo_m_BMax */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BE8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);// awbb_DLSkinZone_ZInfo_m_GridSz */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BD2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030B);// awbb_DLSkinZone_m_BGrid_0__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0323);// awbb_DLSkinZone_m_BGrid_0__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02C3);// awbb_DLSkinZone_m_BGrid_1__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F);// awbb_DLSkinZone_m_BGrid_1__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0288);// awbb_DLSkinZone_m_BGrid_2__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02E5);// awbb_DLSkinZone_m_BGrid_2__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x026A);// awbb_DLSkinZone_m_BGrid_3__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A2);// awbb_DLSkinZone_m_BGrid_3__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// awbb_DLSkinZone_m_BGrid_4__m_left  */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// awbb_DLSkinZone_m_BGrid_4__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C2C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0139);// awbb_IntcR */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0122);// awbb_IntcB */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BFC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0378);// 03AD awbb_IndoorWP_0__r */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x011E);// 013F awbb_IndoorWP_0__b */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02F0);// 0341 awbb_IndoorWP_1__r */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0184);// 017B awbb_IndoorWP_1__b */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0313);// 038D awbb_IndoorWP_2__r */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0158);// 014B awbb_IndoorWP_2__b */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02BA);// 02C3 awbb_IndoorWP_3__r */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01BA);// 01CC awbb_IndoorWP_3__b */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0231);// 0241 awbb_IndoorWP_4__r */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0252);// 027F awbb_IndoorWP_4__b */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0237);// 0241 awbb_IndoorWP_5__r */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x024C);// 027F awbb_IndoorWP_5__b */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F);// 0214 awbb_IndoorWP_6__r */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0279);// 02A8 awbb_IndoorWP_6__b */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0268);// 0270 255 awbb_OutdoorWP_r */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x021A);// 0210 25B awbb_OutdoorWP_b */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C4C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0450);// awbb_MvEq_RBthresh */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C58);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x059C);// awbb_MvEq_RBthresh */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BF8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01AE);// awbb_LowTSep_m_RminusB */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C28);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// awbb_SkinPreference */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CAC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0050);// awbb_OutDMaxIncr */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C28);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// awbb_SkinPreference */ 
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x20BA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0006);// Lowtemp bypass */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0D0E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B8);// awbb_GridCoeff_R_2 */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B2);// awbb_GridCoeff_B_2 */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CFE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0FAB);// 0FAB awbb_GridConst_2_0_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0FF5);// 0FF5 0FF5 awbb_GridConst_2_1_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x10BB);// 10BB 10BB awbb_GridConst_2_2_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1117);// 1117 1123 1153 awbb_GridConst_2_3_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x116D);// 116D 11C5 awbb_GridConst_2_4_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x11D5);// 122A awbb_GridConst_2_5_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A9);// awbb_GridCoeff_R_1 */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00C0);// awbb_GridCoeff_B_1 */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CF8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02CC);// awbb_GridConst_1_0_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x031E);// awbb_GridConst_1_1_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0359);// awbb_GridConst_1_2_ */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CB0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030);// 0000 awbb_GridCorr_R_0__0_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);// 0000 awbb_GridCorr_R_0__1_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0060);// 0078 awbb_GridCorr_R_0__2_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);// 00AA awbb_GridCorr_R_0__3_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);// 0000 awbb_GridCorr_R_0__4_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);// 0000 awbb_GridCorr_R_0__5_ */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030);// 0000 awbb_GridCorr_R_1__0_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);// 0096 awbb_GridCorr_R_1__1_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0060);// 0000 awbb_GridCorr_R_1__2_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);// 0000 awbb_GridCorr_R_1__3_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);// 0000 awbb_GridCorr_R_1__4_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);// 0000 awbb_GridCorr_R_1__5_ */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030);// 00E6 awbb_GridCorr_R_2__0_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);// 0000 awbb_GridCorr_R_2__1_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0060);// 0000 awbb_GridCorr_R_2__2_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);// 0000 awbb_GridCorr_R_2__3_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);// 0000 awbb_GridCorr_R_2__4_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);// 0000 awbb_GridCorr_R_2__5_ */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0018);// 0000 awbb_GridCorr_B_0__0_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_GridCorr_B_0__1_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0064 awbb_GridCorr_B_0__2_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_GridCorr_B_0__3_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF80);// 0000 awbb_GridCorr_B_0__4_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFEC0);// 0000 awbb_GridCorr_B_0__5_ */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0018);// 0000 awbb_GridCorr_B_1__0_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0032 awbb_GridCorr_B_1__1_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_GridCorr_B_1__2_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_GridCorr_B_1__3_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF80);// FF38 awbb_GridCorr_B_1__4_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFEC0);// 0000 awbb_GridCorr_B_1__5_ */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0018);// 0000 awbb_GridCorr_B_2__0_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0032 awbb_GridCorr_B_2__1_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_GridCorr_B_2__2_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// 0000 awbb_GridCorr_B_2__3_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF80);// 0000 awbb_GridCorr_B_2__4_ */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFEC0);// 0000 awbb_GridCorr_B_2__5_ */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0D30);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);// awbb_GridEnable */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x3372);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);// awbb_bUseOutdoorGrid */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// awbb_OutdoorGridCorr_R */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// awbb_OutdoorGridCorr_B */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C86);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);// awbb_OutdoorDetectionZone_ZInfo_m_GridSz */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C70);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF7B);// awbb_OutdoorDetectionZone_m_BGrid_0__m_left */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00CE);// awbb_OutdoorDetectionZone_m_BGrid_0__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF23);// awbb_OutdoorDetectionZone_m_BGrid_1__m_left */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010D);// awbb_OutdoorDetectionZone_m_BGrid_1__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFEF3);// awbb_OutdoorDetectionZone_m_BGrid_2__m_left */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012C);// awbb_OutdoorDetectionZone_m_BGrid_2__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFED7);// awbb_OutdoorDetectionZone_m_BGrid_3__m_left */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x014E);// awbb_OutdoorDetectionZone_m_BGrid_3__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFEBB);// awbb_OutdoorDetectionZone_m_BGrid_4__m_left */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0162);// awbb_OutdoorDetectionZone_m_BGrid_4__m_right */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1388);// awbb_OutdoorDetectionZone_ZInfo_m_AbsGridStep */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C8A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4ACB);// awbb_OutdoorDetectionZone_ZInfo_m_MaxNB */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C88);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A7C);// awbb_OutdoorDetectionZone_ZInfo_m_NBoffs */
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CA0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030); //awbb_GnCurPntImmunity
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CA4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030); //awbb_GnCurPntLongJump
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0180); //awbb_GainsMaxMove
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002); //awbb_GnMinMatchToJump


	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0538); //LutPreDemNoBin
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0035);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0095);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0121);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0139);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0150);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0177);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x019A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01BB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01DC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0219);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0251);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02B3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x035F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03B1);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //LutPostDemNoBin
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0004);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0012);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0016);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0024);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0031);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x003E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x004E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0075);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00A8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0126);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0272);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0334);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x33A4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D0); //#TVAR_wbt_pBaseCcms[0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFA1); //#TVAR_wbt_pBaseCcms[1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFA); //#TVAR_wbt_pBaseCcms[2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF6F); //#TVAR_wbt_pBaseCcms[3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0140); //#TVAR_wbt_pBaseCcms[4] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF49); //#TVAR_wbt_pBaseCcms[5] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFC1); //#TVAR_wbt_pBaseCcms[6] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001F); //#TVAR_wbt_pBaseCcms[7] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01BD); //#TVAR_wbt_pBaseCcms[8] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x013F); //#TVAR_wbt_pBaseCcms[9] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E1); //#TVAR_wbt_pBaseCcms[10]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF43); //#TVAR_wbt_pBaseCcms[11]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0191); //#TVAR_wbt_pBaseCcms[12]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFC0); //#TVAR_wbt_pBaseCcms[13]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01B7); //#TVAR_wbt_pBaseCcms[14]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF30); //#TVAR_wbt_pBaseCcms[15]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015F); //#TVAR_wbt_pBaseCcms[16]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0106); //#TVAR_wbt_pBaseCcms[17]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D0); //#TVAR_wbt_pBaseCcms[18]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFA1); //#TVAR_wbt_pBaseCcms[19]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFA); //#TVAR_wbt_pBaseCcms[20]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF6F); //#TVAR_wbt_pBaseCcms[21]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0140); //#TVAR_wbt_pBaseCcms[22]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF49); //#TVAR_wbt_pBaseCcms[23]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFC1); //#TVAR_wbt_pBaseCcms[24]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001F); //#TVAR_wbt_pBaseCcms[25]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01BD); //#TVAR_wbt_pBaseCcms[26]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x013F); //#TVAR_wbt_pBaseCcms[27]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E1); //#TVAR_wbt_pBaseCcms[28]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF43); //#TVAR_wbt_pBaseCcms[29]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0191); //#TVAR_wbt_pBaseCcms[30]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFC0); //#TVAR_wbt_pBaseCcms[31]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01B7); //#TVAR_wbt_pBaseCcms[32]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF30); //#TVAR_wbt_pBaseCcms[33]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015F); //#TVAR_wbt_pBaseCcms[34]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0106); //#TVAR_wbt_pBaseCcms[35]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D0); //#TVAR_wbt_pBaseCcms[36]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFA1); //#TVAR_wbt_pBaseCcms[37]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFA); //#TVAR_wbt_pBaseCcms[38]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF6F); //#TVAR_wbt_pBaseCcms[39]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0140); //#TVAR_wbt_pBaseCcms[40]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF49); //#TVAR_wbt_pBaseCcms[41]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFC1); //#TVAR_wbt_pBaseCcms[42]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001F); //#TVAR_wbt_pBaseCcms[43]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01BD); //#TVAR_wbt_pBaseCcms[44]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x013F); //#TVAR_wbt_pBaseCcms[45]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E1); //#TVAR_wbt_pBaseCcms[46]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF43); //#TVAR_wbt_pBaseCcms[47]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0191); //#TVAR_wbt_pBaseCcms[48]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFC0); //#TVAR_wbt_pBaseCcms[49]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01B7); //#TVAR_wbt_pBaseCcms[50]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF30); //#TVAR_wbt_pBaseCcms[51]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015F); //#TVAR_wbt_pBaseCcms[52]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0106); //#TVAR_wbt_pBaseCcms[53]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D0); //#TVAR_wbt_pBaseCcms[54]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFA1); //#TVAR_wbt_pBaseCcms[55]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFA); //#TVAR_wbt_pBaseCcms[56]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF6F); //#TVAR_wbt_pBaseCcms[57]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0140); //#TVAR_wbt_pBaseCcms[58]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF49); //#TVAR_wbt_pBaseCcms[59]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFC1); //#TVAR_wbt_pBaseCcms[60]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001F); //#TVAR_wbt_pBaseCcms[61]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01BD); //#TVAR_wbt_pBaseCcms[62]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x013F); //#TVAR_wbt_pBaseCcms[63]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00E1); //#TVAR_wbt_pBaseCcms[64]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF43); //#TVAR_wbt_pBaseCcms[65]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0191); //#TVAR_wbt_pBaseCcms[66]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFC0); //#TVAR_wbt_pBaseCcms[67]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01B7); //#TVAR_wbt_pBaseCcms[68]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF30); //#TVAR_wbt_pBaseCcms[69]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015F); //#TVAR_wbt_pBaseCcms[70]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0106); //#TVAR_wbt_pBaseCcms[71]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01BF); //#TVAR_wbt_pBaseCcms[72]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFBF); //#TVAR_wbt_pBaseCcms[73]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFE); //#TVAR_wbt_pBaseCcms[74]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF6D); //#TVAR_wbt_pBaseCcms[75]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01B4); //#TVAR_wbt_pBaseCcms[76]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF66); //#TVAR_wbt_pBaseCcms[77]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFCA); //#TVAR_wbt_pBaseCcms[78]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFCE); //#TVAR_wbt_pBaseCcms[79]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x017B); //#TVAR_wbt_pBaseCcms[80]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0136); //#TVAR_wbt_pBaseCcms[81]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0132); //#TVAR_wbt_pBaseCcms[82]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF85); //#TVAR_wbt_pBaseCcms[83]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x018B); //#TVAR_wbt_pBaseCcms[84]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF73); //#TVAR_wbt_pBaseCcms[85]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0191); //#TVAR_wbt_pBaseCcms[86]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF3F); //#TVAR_wbt_pBaseCcms[87]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015B); //#TVAR_wbt_pBaseCcms[88]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D0); //#TVAR_wbt_pBaseCcms[89]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01BF); //#TVAR_wbt_pBaseCcms[90] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFBF); //#TVAR_wbt_pBaseCcms[91] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFE); //#TVAR_wbt_pBaseCcms[92] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF6D); //#TVAR_wbt_pBaseCcms[93] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01B4); //#TVAR_wbt_pBaseCcms[94] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF66); //#TVAR_wbt_pBaseCcms[95] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFCA); //#TVAR_wbt_pBaseCcms[96] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFCE); //#TVAR_wbt_pBaseCcms[97] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x017B); //#TVAR_wbt_pBaseCcms[98] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0136); //#TVAR_wbt_pBaseCcms[99] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0132); //#TVAR_wbt_pBaseCcms[100]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF85); //#TVAR_wbt_pBaseCcms[101]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x018B); //#TVAR_wbt_pBaseCcms[102]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF73); //#TVAR_wbt_pBaseCcms[103]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0191); //#TVAR_wbt_pBaseCcms[104]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF3F); //#TVAR_wbt_pBaseCcms[105]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x015B); //#TVAR_wbt_pBaseCcms[106]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D0); //#TVAR_wbt_pBaseCcms[107]
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x3380);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01C8); //#TVAR_wbt_pOutdoorCcm[0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFBF); //#TVAR_wbt_pOutdoorCcm[1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFF); //#TVAR_wbt_pOutdoorCcm[2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF0C); //#TVAR_wbt_pOutdoorCcm[3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0230); //#TVAR_wbt_pOutdoorCcm[4] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFEFA); //#TVAR_wbt_pOutdoorCcm[5] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0006); //#TVAR_wbt_pOutdoorCcm[6] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFDE); //#TVAR_wbt_pOutdoorCcm[7] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0225); //#TVAR_wbt_pOutdoorCcm[8] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0124); //#TVAR_wbt_pOutdoorCcm[9] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010D); //#TVAR_wbt_pOutdoorCcm[10]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF21); //#TVAR_wbt_pOutdoorCcm[11]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D4); //#TVAR_wbt_pOutdoorCcm[12]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF40); //#TVAR_wbt_pOutdoorCcm[13]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0187); //#TVAR_wbt_pOutdoorCcm[14]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFEB3); //#TVAR_wbt_pOutdoorCcm[15]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014B); //#TVAR_wbt_pOutdoorCcm[16]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007B); //#TVAR_wbt_pOutdoorCcm[17]
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0612);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D5);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0128);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0166);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0193);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0498); //Indoor
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0021);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0060);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0127);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x016E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01A5);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01FB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x021F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0260);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x029A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02F7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x034D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0395);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03CE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF);
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //Outdoor
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0021);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0060);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00D3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0127);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x014C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x016E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01A5);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01D3);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01FB);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x021F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0260);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x029A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02F7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x034D);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0395);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03CE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF);

	// AFIT
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x06D4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0013);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x005C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B7);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x016E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x02DD);
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0734);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0040); // AFIT16_BRIGHTNESS
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFD0); // AFIT16_CONTRAST
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFE0); // AFIT16_SATURATION
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFE0); // AFIT16_SHARP_BLUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_GLAMOUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0078); // AFIT16_sddd8a_edge_high
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x012C); // AFIT16_demsharpmix1_iLowBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF); // AFIT16_demsharpmix1_iHighBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_demsharpmix1_iLowSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_demsharpmix1_iHighSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C); // AFIT16_demsharpmix1_iLowThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); // AFIT16_demsharpmix1_iHighThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01E6); // AFIT16_demsharpmix1_iRGBOffset
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_demsharpmix1_iDemClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0070); // AFIT16_demsharpmix1_iTune
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01FF); // AFIT16_YUV422_DENOISE_iUVLowThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0144); // AFIT16_YUV422_DENOISE_iUVHighThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000F); // AFIT16_sddd8a_iClustThresh_H
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); // AFIT16_sddd8a_iClustThresh_C
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0073); // AFIT16_Sharpening_iLowSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0087); // AFIT16_Sharpening_iHighSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_sddd8a_iClustThresh_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); // AFIT16_sddd8a_iClustThresh_C_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E); // AFIT16_Sharpening_iHighSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_sddd8a_iClustThresh_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); // AFIT16_sddd8a_iClustThresh_C_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0046); // AFIT16_Sharpening_iHighSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2B32); // AFIT8_sddd8a_edge_low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0601); // AFIT8_sddd8a_repl_force
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iHotThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iColdThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_AddNoisePower1
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF); // AFIT8_sddd8a_iSatSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x07FF); // AFIT8_sddd8a_iRadialLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFF); // AFIT8_sddd8a_iLowMaxSlopeAllowed
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iLowSlopeThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x050D); // AFIT8_Demosaicing_iDFD_ReduceCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E80); // AFIT8_Demosaicing_iCentGrad
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iGRDenoiseVal
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1408); // AFIT8_Demosaicing_iNearGrayDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0214); // AFIT8_Sharpening_iWShThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF01); // AFIT8_Sharpening_iReduceNegative
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x180F); // AFIT8_demsharpmix1_iBCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); // AFIT8_demsharpmix1_iFilterPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_demsharpmix1_iNarrMult
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3A03); // AFIT8_YUV422_DENOISE_iUVSupport //SHADINGPOWER
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0094); // AFIT8_RGBGamma2_1LUT_sim_iLinearity
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0580); // AFIT8_ccm_oscar_sim_iSaturation
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0180); // AFIT8_YUV422_CONTROL_Y_mul
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0308); // AFIT8_sddd8a_iClustMulT_H 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3186); // AFIT8_sddd8a_DispTH_Low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x52FF); // AFIT8_sddd8a_iDenThreshLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A02); // AFIT8_Demosaicing_iDemSharpenLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080A); // AFIT8_Demosaicing_iDemSharpThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0500); // AFIT8_Demosaicing_iEdgeDesatThrLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032D); // AFIT8_Demosaicing_iEdgeDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x324E); // AFIT8_Demosaicing_iDemShLowLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); // AFIT8_Sharpening_iHighSharpPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x022E); // AFIT8_Sharpening_iHighShDenoise
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9696); // AFIT8_sddd8a_DispTH_Low_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x46FF); // AFIT8_sddd8a_iDenThreshLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0802); // AFIT8_Demosaicing_iDemSharpenLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0802); // AFIT8_Demosaicing_iDemSharpThresh_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3202); // AFIT8_Demosaicing_iDemShLowLimit_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9696); // AFIT8_sddd8a_DispTH_Low_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x46FF); // AFIT8_sddd8a_iDenThreshLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0802); // AFIT8_Demosaicing_iDemSharpenLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0802); // AFIT8_Demosaicing_iDemSharpThresh_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3202); // AFIT8_Demosaicing_iDemShLowLimit_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_BRIGHTNESS
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); // AFIT16_CONTRAST
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000D); // AFIT16_SATURATION
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFF0); // AFIT16_SHARP_BLUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_GLAMOUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x006A); // AFIT16_sddd8a_edge_high
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x012C); // AFIT16_demsharpmix1_iLowBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF); // AFIT16_demsharpmix1_iHighBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_demsharpmix1_iLowSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_demsharpmix1_iHighSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C); // AFIT16_demsharpmix1_iLowThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); // AFIT16_demsharpmix1_iHighThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01E6); // AFIT16_demsharpmix1_iRGBOffset
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF); // AFIT16_demsharpmix1_iDemClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0070); // AFIT16_demsharpmix1_iTune
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007D); // AFIT16_YUV422_DENOISE_iUVLowThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_YUV422_DENOISE_iUVHighThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_sddd8a_iClustThresh_H
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); // AFIT16_sddd8a_iClustThresh_C
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0073); // AFIT16_Sharpening_iLowSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0087); // AFIT16_Sharpening_iHighSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_sddd8a_iClustThresh_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); // AFIT16_sddd8a_iClustThresh_C_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E); // AFIT16_Sharpening_iHighSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_sddd8a_iClustThresh_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000A); // AFIT16_sddd8a_iClustThresh_C_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E); // AFIT16_Sharpening_iHighSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2B32); // AFIT8_sddd8a_edge_low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0601); // AFIT8_sddd8a_repl_force
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iHotThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iColdThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_AddNoisePower1
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF); // AFIT8_sddd8a_iSatSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x07FF); // AFIT8_sddd8a_iRadialLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFF); // AFIT8_sddd8a_iLowMaxSlopeAllowed
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iLowSlopeThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x050D); // AFIT8_Demosaicing_iDFD_ReduceCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E80); // AFIT8_Demosaicing_iCentGrad
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iGRDenoiseVal
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1408); // AFIT8_Demosaicing_iNearGrayDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0214); // AFIT8_Sharpening_iWShThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF01); // AFIT8_Sharpening_iReduceNegative
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x180F); // AFIT8_demsharpmix1_iBCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); // AFIT8_demsharpmix1_iFilterPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_demsharpmix1_iNarrMult
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3A03); // AFIT8_YUV422_DENOISE_iUVSupport //SHADINGPOWER
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); // AFIT8_RGBGamma2_1LUT_sim_iLinearity
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); // AFIT8_ccm_oscar_sim_iSaturation
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0180); // AFIT8_YUV422_CONTROL_Y_mul
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0308); // AFIT8_sddd8a_iClustMulT_H
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E65); // AFIT8_sddd8a_DispTH_Low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1A1B); // AFIT8_sddd8a_iDenThreshLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A03); // AFIT8_Demosaicing_iDemSharpenLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080A); // AFIT8_Demosaicing_iDemSharpThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0500); // AFIT8_Demosaicing_iEdgeDesatThrLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032D); // AFIT8_Demosaicing_iEdgeDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x144D); // AFIT8_Demosaicing_iDemShLowLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1805); // AFIT8_Sharpening_iHighSharpPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x021F); // AFIT8_Sharpening_iHighShDenoise
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9696); // AFIT8_sddd8a_DispTH_Low_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2FFF); // AFIT8_sddd8a_iDenThreshLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0504); // AFIT8_Demosaicing_iDemSharpenLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080F); // AFIT8_Demosaicing_iDemSharpThresh_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3208); // AFIT8_Demosaicing_iDemShLowLimit_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9696); // AFIT8_sddd8a_DispTH_Low_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x14FF); // AFIT8_sddd8a_iDenThreshLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0504); // AFIT8_Demosaicing_iDemSharpenLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080F); // AFIT8_Demosaicing_iDemSharpThresh_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3208); // AFIT8_Demosaicing_iDemShLowLimit_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_BRIGHTNESS
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A); // AFIT16_CONTRAST
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0018); // AFIT16_SATURATION
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFF8); // AFIT16_SHARP_BLUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_GLAMOUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_sddd8a_edge_high
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x012C); // AFIT16_demsharpmix1_iLowBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF); // AFIT16_demsharpmix1_iHighBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_demsharpmix1_iLowSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_demsharpmix1_iHighSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C); // AFIT16_demsharpmix1_iLowThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); // AFIT16_demsharpmix1_iHighThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01E6); // AFIT16_demsharpmix1_iRGBOffset
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF); // AFIT16_demsharpmix1_iDemClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0070); // AFIT16_demsharpmix1_iTune
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007D); // AFIT16_YUV422_DENOISE_iUVLowThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_YUV422_DENOISE_iUVHighThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_sddd8a_iClustThresh_H
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_sddd8a_iClustThresh_C
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0073); // AFIT16_Sharpening_iLowSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0087); // AFIT16_Sharpening_iHighSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_sddd8a_iClustThresh_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0019); // AFIT16_sddd8a_iClustThresh_C_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E); // AFIT16_Sharpening_iHighSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_sddd8a_iClustThresh_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0019); // AFIT16_sddd8a_iClustThresh_C_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E); // AFIT16_Sharpening_iHighSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2B32); // AFIT8_sddd8a_edge_low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0601); // AFIT8_sddd8a_repl_force
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iHotThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iColdThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_AddNoisePower1
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF); // AFIT8_sddd8a_iSatSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x07FF); // AFIT8_sddd8a_iRadialLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFF); // AFIT8_sddd8a_iLowMaxSlopeAllowed
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iLowSlopeThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x050D); // AFIT8_Demosaicing_iDFD_ReduceCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E80); // AFIT8_Demosaicing_iCentGrad
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iGRDenoiseVal
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2008); // AFIT8_Demosaicing_iNearGrayDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0200); // AFIT8_Sharpening_iWShThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF01); // AFIT8_Sharpening_iReduceNegative
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x180F); // AFIT8_demsharpmix1_iBCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); // AFIT8_demsharpmix1_iFilterPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_demsharpmix1_iNarrMult
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3A03); // AFIT8_YUV422_DENOISE_iUVSupport
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); // AFIT8_RGBGamma2_1LUT_sim_iLinearity
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); // AFIT8_ccm_oscar_sim_iSaturation
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0180); // AFIT8_YUV422_CONTROL_Y_mul
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0208); // AFIT8_sddd8a_iClustMulT_H
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E4B); // AFIT8_sddd8a_DispTH_Low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F0F); // AFIT8_sddd8a_iDenThreshLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A05); // AFIT8_Demosaicing_iDemSharpenLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080A); // AFIT8_Demosaicing_iDemSharpThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0500); // AFIT8_Demosaicing_iEdgeDesatThrLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x032D); // AFIT8_Demosaicing_iEdgeDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x324D); // AFIT8_Demosaicing_iDemShLowLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E); // AFIT8_Sharpening_iHighSharpPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0200); // AFIT8_Sharpening_iHighShDenoise
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9696); // AFIT8_sddd8a_DispTH_Low_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1EFF); // AFIT8_sddd8a_iDenThreshLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0505); // AFIT8_Demosaicing_iDemSharpenLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080F); // AFIT8_Demosaicing_iDemSharpThresh_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3208); // AFIT8_Demosaicing_iDemShLowLimit_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9696); // AFIT8_sddd8a_DispTH_Low_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1EFF); // AFIT8_sddd8a_iDenThreshLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0505); // AFIT8_Demosaicing_iDemSharpenLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080F); // AFIT8_Demosaicing_iDemSharpThresh_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3208); // AFIT8_Demosaicing_iDemShLowLimit_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_BRIGHTNESS
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A); // AFIT16_CONTRAST
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0018); // AFIT16_SATURATION
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFF8); // AFIT16_SHARP_BLUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_GLAMOUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_sddd8a_edge_high
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x012C); // AFIT16_demsharpmix1_iLowBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF); // AFIT16_demsharpmix1_iHighBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_demsharpmix1_iLowSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_demsharpmix1_iHighSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C); // AFIT16_demsharpmix1_iLowThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); // AFIT16_demsharpmix1_iHighThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01E6); // AFIT16_demsharpmix1_iRGBOffset
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_demsharpmix1_iDemClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0070); // AFIT16_demsharpmix1_iTune
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x007D); // AFIT16_YUV422_DENOISE_iUVLowThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_YUV422_DENOISE_iUVHighThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_H
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_sddd8a_iClustThresh_C
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0073); // AFIT16_Sharpening_iLowSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x009F); // AFIT16_Sharpening_iHighSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_C_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0037); // AFIT16_Sharpening_iHighSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_C_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0037); // AFIT16_Sharpening_iHighSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2B32); // AFIT8_sddd8a_edge_low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0601); // AFIT8_sddd8a_repl_force
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iHotThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iColdThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_AddNoisePower1
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF); // AFIT8_sddd8a_iSatSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x07A0); // AFIT8_sddd8a_iRadialLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFF); // AFIT8_sddd8a_iLowMaxSlopeAllowed
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iLowSlopeThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x050D); // AFIT8_Demosaicing_iDFD_ReduceCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E80); // AFIT8_Demosaicing_iCentGrad
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iGRDenoiseVal
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2008); // AFIT8_Demosaicing_iNearGrayDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0200); // AFIT8_Sharpening_iWShThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF01); // AFIT8_Sharpening_iReduceNegative
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x180F); // AFIT8_demsharpmix1_iBCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); // AFIT8_demsharpmix1_iFilterPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_demsharpmix1_iNarrMult
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3A03); // AFIT8_YUV422_DENOISE_iUVSupport
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); // AFIT8_RGBGamma2_1LUT_sim_iLinearity
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); // AFIT8_ccm_oscar_sim_iSaturation
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0180); // AFIT8_YUV422_CONTROL_Y_mul
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0108); // AFIT8_sddd8a_iClustMulT_H
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E32); // AFIT8_sddd8a_DispTH_Low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x090A); // AFIT8_sddd8a_iDenThreshLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A05); // AFIT8_Demosaicing_iDemSharpenLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080A); // AFIT8_Demosaicing_iDemSharpThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0328); // AFIT8_Demosaicing_iEdgeDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x324C); // AFIT8_Demosaicing_iDemShLowLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E); // AFIT8_Sharpening_iHighSharpPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0200); // AFIT8_Sharpening_iHighShDenoise
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9696); // AFIT8_sddd8a_DispTH_Low_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FFF); // AFIT8_sddd8a_iDenThreshLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0307); // AFIT8_Demosaicing_iDemSharpenLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080F); // AFIT8_Demosaicing_iDemSharpThresh_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3208); // AFIT8_Demosaicing_iDemShLowLimit_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x9696); // AFIT8_sddd8a_DispTH_Low_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FFF); // AFIT8_sddd8a_iDenThreshLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0307); // AFIT8_Demosaicing_iDemSharpenLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080F); // AFIT8_Demosaicing_iDemSharpThresh_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3208); // AFIT8_Demosaicing_iDemShLowLimit_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_BRIGHTNESS
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001A); // AFIT16_CONTRAST
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0018); // AFIT16_SATURATION
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFF8); // AFIT16_SHARP_BLUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_GLAMOUR
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_edge_high
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x012C); // AFIT16_demsharpmix1_iLowBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x03FF); // AFIT16_demsharpmix1_iHighBright
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0014); // AFIT16_demsharpmix1_iLowSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0064); // AFIT16_demsharpmix1_iHighSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x000C); // AFIT16_demsharpmix1_iLowThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); // AFIT16_demsharpmix1_iHighThreshold
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x01E6); // AFIT16_demsharpmix1_iRGBOffset
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT16_demsharpmix1_iDemClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0070); // AFIT16_demsharpmix1_iTune
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0087); // AFIT16_YUV422_DENOISE_iUVLowThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0073); // AFIT16_YUV422_DENOISE_iUVHighThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_H
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_sddd8a_iClustThresh_C
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0073); // AFIT16_Sharpening_iLowSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00B4); // AFIT16_Sharpening_iHighSharpClamp
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_C_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0046); // AFIT16_Sharpening_iHighSharpClamp_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0028); // AFIT16_sddd8a_iClustThresh_C_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0023); // AFIT16_Sharpening_iLowSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0046); // AFIT16_Sharpening_iHighSharpClamp_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2B23); // AFIT8_sddd8a_edge_low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0601); // AFIT8_sddd8a_repl_force
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iHotThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iColdThreshHigh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_AddNoisePower1
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x00FF); // AFIT8_sddd8a_iSatSat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0B84); // AFIT8_sddd8a_iRadialLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFFFF); // AFIT8_sddd8a_iLowMaxSlopeAllowed
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_sddd8a_iLowSlopeThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x050D); // AFIT8_Demosaicing_iDFD_ReduceCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E80); // AFIT8_Demosaicing_iCentGrad
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iGRDenoiseVal
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x2008); // AFIT8_Demosaicing_iNearGrayDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0200); // AFIT8_Sharpening_iWShThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFF01); // AFIT8_Sharpening_iReduceNegative
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x180F); // AFIT8_demsharpmix1_iBCoeff
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); // AFIT8_demsharpmix1_iFilterPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_demsharpmix1_iNarrMult
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3A03); // AFIT8_YUV422_DENOISE_iUVSupport
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); // AFIT8_RGBGamma2_1LUT_sim_iLinearity
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); // AFIT8_ccm_oscar_sim_iSaturation
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0180); // AFIT8_YUV422_CONTROL_Y_mul
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0108); // AFIT8_sddd8a_iClustMulT_H
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x1E1E); // AFIT8_sddd8a_DispTH_Low
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0002); // AFIT8_sddd8a_iDenThreshLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0A0A); // AFIT8_Demosaicing_iDemSharpenLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0800); // AFIT8_Demosaicing_iDemSharpThresh
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0328); // AFIT8_Demosaicing_iEdgeDesat
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x324C); // AFIT8_Demosaicing_iDemShLowLimit
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x001E); // AFIT8_Sharpening_iHighSharpPower
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0200); // AFIT8_Sharpening_iHighShDenoise
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6464); // AFIT8_sddd8a_DispTH_Low_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FFF); // AFIT8_sddd8a_iDenThreshLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0307); // AFIT8_Demosaicing_iDemSharpenLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080F); // AFIT8_Demosaicing_iDemSharpThresh_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3208); // AFIT8_Demosaicing_iDemShLowLimit_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0103); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_1_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x010C); // AFIT8_sddd8a_iClustMulT_H_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x6464); // AFIT8_sddd8a_DispTH_Low_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0FFF); // AFIT8_sddd8a_iDenThreshLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0307); // AFIT8_Demosaicing_iDemSharpenLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x080F); // AFIT8_Demosaicing_iDemSharpThresh_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); // AFIT8_Demosaicing_iEdgeDesatThrLow_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x030F); // AFIT8_Demosaicing_iEdgeDesat_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x3208); // AFIT8_Demosaicing_iDemShLowLimit_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0F1E); // AFIT8_Sharpening_iHighSharpPower_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x020F); // AFIT8_Sharpening_iHighShDenoise_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0003); // AFIT8_demsharpmix1_iNarrFiltReduce_Bin_2_mode
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x7F5E);  //ConstAfitBaseVals_0_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xFEEE);  //ConstAfitBaseVals_1_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xD9B7);  //ConstAfitBaseVals_2_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0472);  //ConstAfitBaseVals_3_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);  //ConstAfitBaseVals_4_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x1278);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0xAAF0);	//gisp_dadlc  Ladlc mode average
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0408);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x067F);	//REG_TC_DBG_AutoAlgEnBits all AA are on

	// User Control
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x018E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //Brightness
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); //contrast
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0010); //Saturation
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000); //sharpness

	// Flicker
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0408);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x065F);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x03F4); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);

//pll
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x012E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x5DC0);	// input clock=24MHz
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0146); //
	
#ifdef MIPI_INTERFACE   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //REG_TC_IPRM_UseNPviClocks
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001); //REG_TC_IPRM_UseNMipiClocks

	//92M for 30 fps
	 S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014C);
	 S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2CEC); 
	 S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0152);
	 S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x59D8);
	 S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014E);
	 S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x59D8); 
#else
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001); //REG_TC_IPRM_UseNPviClocks
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //REG_TC_IPRM_UseNMipiClocks

	//72M Pclk
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2328);	//0x2EE0 //REG_TC_IPRM_sysClocks_0 54MHz 34BC
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0152);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4650);	//72M //REG_TC_IPRM_MinOutRate4KHz_0 6977
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4650); // 72M//REG_TC_IPRM_MaxOutRate4KHz_0 6978
#endif

    //72M Pclk
	//S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014C);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2328);	//0x2EE0 //REG_TC_IPRM_sysClocks_0 54MHz 34BC
	//S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0152);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4650);	//72M //REG_TC_IPRM_MinOutRate4KHz_0 6977
	//S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014E);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4650); // 72M//REG_TC_IPRM_MaxOutRate4KHz_0 6978

    //58M
	//S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014C);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x38A4);	
	//S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0152);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x38A4);	
	//S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014E);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x38A4); 

    //88M 29 fps
	//S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014C);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2AF8); 
	//S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0152);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x55F0); 
	//S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014E);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x55F0);
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00FA); 


	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0164); //update PLL
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001); //REG_TC_IPRM_InitParamsUpdated	 
	
	//============================================================
	// Preview configuration 0
	//============================================================
					
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0500);	// 1280
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03C0);	// 960
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// YUV422 7:raw10
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01C4); 
	//S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0042); //REG_0TC_PCFG_PVIMask 0052
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040); //negative pclk
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01C8); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //REG_0TC_PCFG_uClockInd
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D2); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//REG_0TC_PCFG_usFrTimeType
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	//REG_0TC_PCFG_FrRateQualityType
	
     S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //REG_0TC_PCFG_usMinFrTimeMsecMult10
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0D05);	//REG_0TC_PCFG_usMaxFrTimeMsecMult10
     S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01E8);
     S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //REG_0TC_PCFG_uPrevMirror

	//============================================================
	// active preview configure
	//============================================================
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x01A8); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);  // #REG_TC_GP_ActivePrevConfig
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x01AC); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);  // #REG_TC_GP_PrevOpenAfterChange
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x01A6); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);  // #REG_TC_GP_NewConfigSync
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x01AA); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);  // #REG_TC_GP_PrevConfigChanged
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x019E); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);  // #REG_TC_GP_EnablePreview
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);  // #REG_TC_GP_EnablePreviewChanged
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x1000, 0x0001);

	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// Set host interrupt
#endif

#if 1////Supply by AVP,for Trule module 

	// 00.History																	 
	//*********************************************************************************
	// 2011 : EVT1																	  
	// 20111109 : LSI CSE Standard													 
	// 20111110 : Shading, AWB, Contrast Tuning 									   
	// 20111114 : AE Weight Tuning		 
	// 20111220 : Sequence setting , AFIT , afit_bUseNormBrForAfit ,awbb_bUseOutdoorGrid 									  
	//*********************************************************************************
	
	//$MIPI[Width:1280,Height:960,Format:YUV422,Lane:1,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:2]

	SENSORDB("init set st\n");

	//*********************************************************************************
	// 01.Start Setting 																
	//*********************************************************************************
	S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC,0xD000);	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0010,0x0001);	// S/W Reset 
	S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC,0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0000,0x0000);	// Simmian bug workaround 

	S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC,0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x1030,0x0000);	// contint_host_int 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0014,0x0001);	// sw_load_complete - Release CORE (Arm) from reset state 

	mdelay(5);		// Delay 1ms ,add to 5ms

	//*********************************************************************************
	//02.ETC Setting                                                                  
	//*********************************************************************************
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x1278);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xAAF0);	// gisp_dadlc_config Ladlc mode average

	//=================================================================================
	// 03.Analog Setting1 & ASP Control
	//=================================================================================

	//*********************************************************************************
	// 04.Trap and Patch                                                              
	//*********************************************************************************
	// Start of Patch data 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028,0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x2470);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xB510);	// 70002470 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x490D);	// 70002472 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x480D);	// 70002474 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF000);	// 70002476 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF96D);	// 70002478 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x490D);	// 7000247A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x480D);	// 7000247C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF000);	// 7000247E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF969);	// 70002480 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x490D);	// 70002482 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x480D);	// 70002484 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x6341);	// 70002486 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x490D);	// 70002488 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x480E);	// 7000248A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF000);	// 7000248C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF962);	// 7000248E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x490D);	// 70002490 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x480E);	// 70002492 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF000);	// 70002494 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF95E);	// 70002496 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x490D);	// 70002498 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x480E);	// 7000249A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF000);	// 7000249C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF95A);	// 7000249E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xBC10);	// 700024A0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xBC08);	// 700024A2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4718);	// 700024A4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700024A6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x26D4);	// 700024A8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 700024AA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8EDD);	// 700024AC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700024AE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x264C);	// 700024B0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 700024B2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8725);	// 700024B4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700024B6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x25EC);	// 700024B8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 700024BA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080);	// 700024BC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 700024BE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2540);	// 700024C0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 700024C2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xA6EF);	// 700024C4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700024C6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x250C);	// 700024C8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 700024CA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xA0F1);	// 700024CC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700024CE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x24D8);	// 700024D0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 700024D2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x058F);	// 700024D4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700024D6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4010);	// 700024D8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE92D);	// 700024DA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A0);	// 700024DC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 700024DE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x023C);	// 700024E0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700024E2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B2);	// 700024E4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D0);	// 700024E6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700024E8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE350);	// 700024EA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 700024EC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A00);	// 700024EE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080);	// 700024F0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE310);	// 700024F2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	// 700024F4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1A00);	// 700024F6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1228);	// 700024F8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700024FA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// 700024FC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE3A0);	// 700024FE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0DB2);	// 70002500 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1C1);	// 70002502 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4010);	// 70002504 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE8BD);	// 70002506 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1E);	// 70002508 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 7000250A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4010);	// 7000250C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE92D);	// 7000250E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4000);	// 70002510 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE590);	// 70002512 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 70002514 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 70002516 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0094);	// 70002518 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 7000251A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0208);	// 7000251C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 7000251E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002520 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE5D0);	// 70002522 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002524 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE350);	// 70002526 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	// 70002528 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A00);	// 7000252A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 7000252C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE594);	// 7000252E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A0);	// 70002530 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 70002532 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 70002534 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE584);	// 70002536 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4010);	// 70002538 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE8BD);	// 7000253A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1E);	// 7000253C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 7000253E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4070);	// 70002540 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE92D);	// 70002542 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002544 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE590);	// 70002546 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0800);	// 70002548 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 7000254A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0820);	// 7000254C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 7000254E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4041);	// 70002550 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE280);	// 70002552 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01D4);	// 70002554 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002556 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x11B8);	// 70002558 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D0);	// 7000255A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x51B6);	// 7000255C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D0);	// 7000255E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// 70002560 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE041);	// 70002562 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0094);	// 70002564 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE000);	// 70002566 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1D11);	// 70002568 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE3A0);	// 7000256A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0082);	// 7000256C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 7000256E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x11B4);	// 70002570 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002572 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1000);	// 70002574 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE5D1);	// 70002576 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002578 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE351);	// 7000257A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 7000257C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A00);	// 7000257E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A0);	// 70002580 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 70002582 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x219C);	// 70002584 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002586 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3FB0);	// 70002588 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D2);	// 7000258A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 7000258C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE353);	// 7000258E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003);	// 70002590 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A00);	// 70002592 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3198);	// 70002594 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002596 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x5BB2);	// 70002598 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1C3);	// 7000259A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC000);	// 7000259C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE085);	// 7000259E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xCBB4);	// 700025A0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1C3);	// 700025A2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700025A4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE351);	// 700025A6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700025A8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A00);	// 700025AA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080);	// 700025AC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 700025AE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1DBC);	// 700025B0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D2);	// 700025B2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3EB4);	// 700025B4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D2);	// 700025B6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2EB2);	// 700025B8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D2);	// 700025BA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0193);	// 700025BC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE001);	// 700025BE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0092);	// 700025C0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE000);	// 700025C2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2811);	// 700025C4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE3A0);	// 700025C6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0194);	// 700025C8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE001);	// 700025CA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0092);	// 700025CC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE000);	// 700025CE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x11A1);	// 700025D0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 700025D2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01A0);	// 700025D4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 700025D6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0067);	// 700025D8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 700025DA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1154);	// 700025DC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700025DE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02B4);	// 700025E0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1C1);	// 700025E2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4070);	// 700025E4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE8BD);	// 700025E6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1E);	// 700025E8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 700025EA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4010);	// 700025EC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE92D);	// 700025EE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0063);	// 700025F0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 700025F2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x213C);	// 700025F4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700025F6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x14B0);	// 700025F8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D2);	// 700025FA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080);	// 700025FC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE311);	// 700025FE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// 70002600 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A00);	// 70002602 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0130);	// 70002604 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002606 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B0);	// 70002608 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D0);	// 7000260A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// 7000260C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE350);	// 7000260E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// 70002610 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x9A00);	// 70002612 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// 70002614 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE3A0);	// 70002616 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002618 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEA00);	// 7000261A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 7000261C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE3A0);	// 7000261E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3104);	// 70002620 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002622 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002624 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE5C3);	// 70002626 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002628 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE5D3);	// 7000262A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 7000262C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE350);	// 7000262E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003);	// 70002630 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A00);	// 70002632 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080);	// 70002634 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE3C1);	// 70002636 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1100);	// 70002638 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 7000263A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x04B0);	// 7000263C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1C2);	// 7000263E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B2);	// 70002640 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1C1);	// 70002642 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4010);	// 70002644 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE8BD);	// 70002646 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1E);	// 70002648 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 7000264A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x41F0);	// 7000264C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE92D);	// 7000264E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1000);	// 70002650 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE590);	// 70002652 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC801);	// 70002654 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 70002656 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC82C);	// 70002658 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 7000265A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1004);	// 7000265C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE590);	// 7000265E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1801);	// 70002660 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 70002662 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1821);	// 70002664 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 70002666 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4008);	// 70002668 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE590);	// 7000266A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x500C);	// 7000266C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE590);	// 7000266E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2004);	// 70002670 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 70002672 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3005);	// 70002674 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 70002676 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000C);	// 70002678 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 7000267A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0043);	// 7000267C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 7000267E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x60BC);	// 70002680 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002682 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B2);	// 70002684 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D6);	// 70002686 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002688 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE350);	// 7000268A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000E);	// 7000268C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A00);	// 7000268E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B0);	// 70002690 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002692 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x05B4);	// 70002694 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D0);	// 70002696 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	// 70002698 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE350);	// 7000269A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A);	// 7000269C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1A00);	// 7000269E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x70A4);	// 700026A0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700026A2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x10F4);	// 700026A4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D6);	// 700026A6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x26B0);	// 700026A8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D7);	// 700026AA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F0);	// 700026AC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D4);	// 700026AE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0039);	// 700026B0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 700026B2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B0);	// 700026B4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1C4);	// 700026B6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x26B0);	// 700026B8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D7);	// 700026BA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x10F6);	// 700026BC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D6);	// 700026BE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F0);	// 700026C0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D5);	// 700026C2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0034);	// 700026C4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 700026C6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B0);	// 700026C8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1C5);	// 700026CA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x41F0);	// 700026CC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE8BD);	// 700026CE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1E);	// 700026D0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 700026D2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4010);	// 700026D4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE92D);	// 700026D6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4000);	// 700026D8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 700026DA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1004);	// 700026DC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE594);	// 700026DE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005C);	// 700026E0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700026E2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B0);	// 700026E4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1D0);	// 700026E6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700026E8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE350);	// 700026EA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);	// 700026EC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A00);	// 700026EE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0054);	// 700026F0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700026F2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3001);	// 700026F4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE1A0);	// 700026F6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2068);	// 700026F8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE590);	// 700026FA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004C);	// 700026FC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700026FE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1005);	// 70002700 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE3A0);	// 70002702 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0027);	// 70002704 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 70002706 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002708 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE584);	// 7000270A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4010);	// 7000270C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE8BD);	// 7000270E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1E);	// 70002710 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 70002712 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002714 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE594);	// 70002716 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0025);	// 70002718 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEB00);	// 7000271A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 7000271C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE584);	// 7000271E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFF9);	// 70002720 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xEAFF);	// 70002722 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1728);	// 70002724 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 70002726 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x112C);	// 70002728 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 7000272A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x27C4);	// 7000272C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 7000272E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x122C);	// 70002730 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 70002732 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF200);	// 70002734 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xD000);	// 70002736 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2340);	// 70002738 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 7000273A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0E2C);	// 7000273C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 7000273E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF400);	// 70002740 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xD000);	// 70002742 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3370);	// 70002744 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 70002746 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0CDC);	// 70002748 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 7000274A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x20D4);	// 7000274C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 7000274E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x06D4);	// 70002750 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7000);	// 70002752 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4778);	// 70002754 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x46C0);	// 70002756 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC000);	// 70002758 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 7000275A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1C);	// 7000275C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 7000275E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC091);	// 70002760 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002762 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC000);	// 70002764 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002766 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1C);	// 70002768 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 7000276A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x058F);	// 7000276C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 7000276E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC000);	// 70002770 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002772 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1C);	// 70002774 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 70002776 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xA0F1);	// 70002778 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 7000277A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xF004);	// 7000277C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE51F);	// 7000277E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xD14C);	// 70002780 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 70002782 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC000);	// 70002784 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002786 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1C);	// 70002788 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 7000278A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2B43);	// 7000278C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 7000278E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC000);	// 70002790 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 70002792 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1C);	// 70002794 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 70002796 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8725);	// 70002798 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 7000279A 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC000);	// 7000279C 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 7000279E 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1C);	// 700027A0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 700027A2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x6777);	// 700027A4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700027A6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC000);	// 700027A8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700027AA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1C);	// 700027AC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 700027AE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8E49);	// 700027B0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700027B2 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xC000);	// 700027B4 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE59F);	// 700027B6 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF1C);	// 700027B8 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xE12F);	// 700027BA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8EDD);	// 700027BC 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700027BE 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x90C8);	// 700027C0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 700027C2 
	// End of Patch Data(Last : 700027C2h) 
	// Total Size 852 (0x0354)             
	// Addr : 2470 , Size : 850(352h)      


	//*********************************************************************************
	// 05.OTP Control                                                                   
	//*********************************************************************************

	//*********************************************************************************
	// 06.GAS (Grid Anti-Shading)                                                     
	//*********************************************************************************
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x1326);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// gisp_gos_Enable 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x063A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_0__0_ Horizon 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_0__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_0__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_0__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_1__0_ IncandA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_1__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_1__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_1__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_2__0_ WW      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_2__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_2__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_2__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00E8);	// TVAR_ash_GASAlpha_3__0_ CW      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_3__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_3__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_3__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00C8);	// TVAR_ash_GASAlpha_4__0_ D50     
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F8);	// TVAR_ash_GASAlpha_4__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F8);	// TVAR_ash_GASAlpha_4__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_4__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F0);	// TVAR_ash_GASAlpha_5__0_ D65     
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_5__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_5__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_5__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F0);	// TVAR_ash_GASAlpha_6__0_ D75     
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_6__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_6__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASAlpha_6__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x067A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_0__0_ Horizon 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_0__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_0__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_0__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_1__0_ IncandA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_1__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_1__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_1__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_2__0_ WW      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_2__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_2__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_2__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_3__0_ CW      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_3__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_3__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_3__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_4__0_ D50     
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_4__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_4__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_4__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_5__0_ D65     
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_5__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_5__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_5__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_6__0_ D75     
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_6__1_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_6__2_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASBeta_6__3_         
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x06BA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	//ash_bLumaMode 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0632);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// ash_CGrasAlphas_0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// ash_CGrasAlphas_1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// ash_CGrasAlphas_2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// ash_CGrasAlphas_3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0672);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASOutdoorAlpha_0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASOutdoorAlpha_1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASOutdoorAlpha_2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// TVAR_ash_GASOutdoorAlpha_3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x06B2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASOutdoorBeta_0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASOutdoorBeta_1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASOutdoorBeta_2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// ash_GASOutdoorBeta_3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x06D0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000D);	// ash_uParabolicScalingA
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000F);	// ash_uParabolicScalingB
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x06CC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0280);	// ash_uParabolicCenterX 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01E0);	// ash_uParabolicCenterY 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x06C6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// ash_bParabolicEstimation 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0624);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x009D);	// TVAR_ash_AwbAshCord_0_ Horizon 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D5);	// TVAR_ash_AwbAshCord_1_ IncandA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103);	// TVAR_ash_AwbAshCord_2_ WW      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0128);	// TVAR_ash_AwbAshCord_3_ CW      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0166);	// TVAR_ash_AwbAshCord_4_ D50     
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0193);	// TVAR_ash_AwbAshCord_5_ D65     
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01A0);	// TVAR_ash_AwbAshCord_6_ D75     
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x347C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x013B);	// 011A 012F Tune_wbt_GAS_0_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0116);	// 011A 0111 Tune_wbt_GAS_1_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D9);	// 00EE 00D6 Tune_wbt_GAS_2_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A6);	// 00C1 009E Tune_wbt_GAS_3_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0082);	// 009E 007A Tune_wbt_GAS_4_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006C);	// 008A 0064 Tune_wbt_GAS_5_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0065);	// 0083 005D Tune_wbt_GAS_6_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006C);	// 008A 0063 Tune_wbt_GAS_7_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080);	// 009E 007B Tune_wbt_GAS_8_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A3);	// 00BF 00A3 Tune_wbt_GAS_9_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D4);	// 00E5 00E3 Tune_wbt_GAS_10_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010D);	// 00F9 013B Tune_wbt_GAS_11_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012E);	// 0124 018B Tune_wbt_GAS_12_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0138);	// 0126 012B Tune_wbt_GAS_13_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0104);	// 010E 00F6 Tune_wbt_GAS_14_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00BE);	// 00D3 00B2 Tune_wbt_GAS_15_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0088);	// 009F 007B Tune_wbt_GAS_16_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0062);	// 007C 0057 Tune_wbt_GAS_17_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004D);	// 0068 0042 Tune_wbt_GAS_18_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046);	// 0061 0039 Tune_wbt_GAS_19_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004C);	// 0068 003F Tune_wbt_GAS_20_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0060);	// 007E 0053 Tune_wbt_GAS_21_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0084);	// 00A3 007A Tune_wbt_GAS_22_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B8);	// 00C9 00B6 Tune_wbt_GAS_23_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F9);	// 00F0 0110 Tune_wbt_GAS_24_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012C);	// 0131 016A Tune_wbt_GAS_25_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x011A);	// 011C 0114 Tune_wbt_GAS_26_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00DB);	// 00EB 00D4 Tune_wbt_GAS_27_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0093);	// 00AA 008F Tune_wbt_GAS_28_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005F);	// 0075 005A Tune_wbt_GAS_29_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003C);	// 0053 0035 Tune_wbt_GAS_30_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0027);	// 003F 0020 Tune_wbt_GAS_31_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0020);	// 0038 0019 Tune_wbt_GAS_32_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0026);	// 0040 001F Tune_wbt_GAS_33_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003A);	// 0055 0032 Tune_wbt_GAS_34_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005C);	// 007A 0056 Tune_wbt_GAS_35_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008E);	// 00A6 008E Tune_wbt_GAS_36_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D2);	// 00D5 00E6 Tune_wbt_GAS_37_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010E);	// 0126 0142 Tune_wbt_GAS_38_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0101);	// 00F6 0102 Tune_wbt_GAS_39_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00BF);	// 00C0 00BE Tune_wbt_GAS_40_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 007D 0077 Tune_wbt_GAS_41_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0044);	// 004D 0045 Tune_wbt_GAS_42_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023);	// 002C 0022 Tune_wbt_GAS_43_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0011);	// 001B 000D Tune_wbt_GAS_44_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000C);	// 0017 0006 Tune_wbt_GAS_45_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010);	// 001B 000A Tune_wbt_GAS_46_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0022);	// 002D 001D Tune_wbt_GAS_47_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0043);	// 004E 003F Tune_wbt_GAS_48_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0074);	// 0080 0075 Tune_wbt_GAS_49_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B7);	// 00C1 00CC Tune_wbt_GAS_50_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F7);	// 00FF 0126 Tune_wbt_GAS_51_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00FC);	// 00EA 00FB Tune_wbt_GAS_52_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B7);	// 00B0 00B7 Tune_wbt_GAS_53_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006F);	// 006D 0070 Tune_wbt_GAS_54_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003C);	// 003D 003D Tune_wbt_GAS_55_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001C);	// 001E 001B Tune_wbt_GAS_56_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A);	// 000F 0007 Tune_wbt_GAS_57_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 000A 0000 Tune_wbt_GAS_58_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A);	// 0010 0004 Tune_wbt_GAS_59_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001B);	// 001E 0015 Tune_wbt_GAS_60_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003B);	// 003E 0034 Tune_wbt_GAS_61_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006C);	// 006F 006A Tune_wbt_GAS_62_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B0);	// 00B2 00C0 Tune_wbt_GAS_63_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F2);	// 00F1 011B Tune_wbt_GAS_64_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00EF);	// 00E0 0102 Tune_wbt_GAS_65_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00AB);	// 00A6 00BA Tune_wbt_GAS_66_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0065);	// 0063 0073 Tune_wbt_GAS_67_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0034);	// 0033 0040 Tune_wbt_GAS_68_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0015);	// 0016 001D Tune_wbt_GAS_69_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 0008 0009 Tune_wbt_GAS_70_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 0003 0001 Tune_wbt_GAS_71_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 0009 0006 Tune_wbt_GAS_72_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0013);	// 0016 0017 Tune_wbt_GAS_73_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0033);	// 0035 0039 Tune_wbt_GAS_74_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0063);	// 0066 006F Tune_wbt_GAS_75_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A5);	// 00A7 00C8 Tune_wbt_GAS_76_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00E5);	// 00E9 011E Tune_wbt_GAS_77_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F7);	// 00D5 0111 Tune_wbt_GAS_78_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B4);	// 009D 00C8 Tune_wbt_GAS_79_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006D);	// 005D 0081 Tune_wbt_GAS_80_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003C);	// 002F 004D Tune_wbt_GAS_81_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001C);	// 0010 0028 Tune_wbt_GAS_82_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000B);	// 0004 0014 Tune_wbt_GAS_83_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// 0001 000B Tune_wbt_GAS_84_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A);	// 0005 0010 Tune_wbt_GAS_85_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001B);	// 0013 0022 Tune_wbt_GAS_86_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003B);	// 0031 0047 Tune_wbt_GAS_87_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006B);	// 005F 007E Tune_wbt_GAS_88_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00AD);	// 00A1 00D8 Tune_wbt_GAS_89_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00ED);	// 00DF 0131 Tune_wbt_GAS_90_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010B);	// 00E9 0129 Tune_wbt_GAS_91_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00CB);	// 00B2 00E4 Tune_wbt_GAS_92_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0085);	// 006F 009E Tune_wbt_GAS_93_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0051);	// 003F 0068 Tune_wbt_GAS_94_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 0020 0041 Tune_wbt_GAS_95_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001C);	// 000F 002B Tune_wbt_GAS_96_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0016);	// 000B 0023 Tune_wbt_GAS_97_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001C);	// 0010 0028 Tune_wbt_GAS_98_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002E);	// 0021 003B Tune_wbt_GAS_99_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004F);	// 0041 0063 Tune_wbt_GAS_100_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0081);	// 0071 00A0 Tune_wbt_GAS_101_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00C4);	// 00B4 00FD Tune_wbt_GAS_102_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0102);	// 00F6 015A Tune_wbt_GAS_103_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0119);	// 00F9 014A Tune_wbt_GAS_104_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00DF);	// 00C2 010D Tune_wbt_GAS_105_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x009B);	// 0082 00CA Tune_wbt_GAS_106_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0067);	// 0053 008F Tune_wbt_GAS_107_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0045);	// 0033 0066 Tune_wbt_GAS_108_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030);	// 0021 004F Tune_wbt_GAS_109_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0029);	// 001C 0048 Tune_wbt_GAS_110_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 0022 004E Tune_wbt_GAS_111_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0043);	// 0035 0067 Tune_wbt_GAS_112_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0066);	// 0056 008F Tune_wbt_GAS_113_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0098);	// 0087 00D1 Tune_wbt_GAS_114_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D9);	// 00CA 0132 Tune_wbt_GAS_115_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010F);	// 0107 018D Tune_wbt_GAS_116_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0138);	// 0108 0159 Tune_wbt_GAS_117_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C);	// 00E0 0141 Tune_wbt_GAS_118_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00CB);	// 00A2 0103 Tune_wbt_GAS_119_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0097);	// 0072 00C9 Tune_wbt_GAS_120_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0073);	// 0052 009E Tune_wbt_GAS_121_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005C);	// 0040 0087 Tune_wbt_GAS_122_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0054);	// 003A 007D Tune_wbt_GAS_123_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005B);	// 0041 0087 Tune_wbt_GAS_124_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0070);	// 0055 00A2 Tune_wbt_GAS_125_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0096);	// 0077 00D4 Tune_wbt_GAS_126_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00C9);	// 00A8 011A Tune_wbt_GAS_127_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0106);	// 00E7 017D Tune_wbt_GAS_128_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012D);	// 011E 01CF Tune_wbt_GAS_129_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0147);	// 0123 0181 Tune_wbt_GAS_130_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012F);	// 0100 0169 Tune_wbt_GAS_131_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F8);	// 00CB 0140 Tune_wbt_GAS_132_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00C5);	// 009A 0106 Tune_wbt_GAS_133_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A1);	// 0079 00DD Tune_wbt_GAS_134_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008B);	// 0067 00C5 Tune_wbt_GAS_135_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0083);	// 0060 00BE Tune_wbt_GAS_136_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008B);	// 0068 00C8 Tune_wbt_GAS_137_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A0);	// 007B 00E6 Tune_wbt_GAS_138_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00C2);	// 009D 011B Tune_wbt_GAS_139_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F3);	// 00CD 015F Tune_wbt_GAS_140_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0124);	// 0108 01BC Tune_wbt_GAS_141_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0139);	// 0131 0206 Tune_wbt_GAS_142_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0093);	// 006C 00A8 Tune_wbt_GAS_143_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007E);	// 006E 008F Tune_wbt_GAS_144_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0062);	// 005D 006F Tune_wbt_GAS_145_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004D);	// 004C 0054 Tune_wbt_GAS_146_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003E);	// 0040 0041 Tune_wbt_GAS_147_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0034);	// 0037 0036 Tune_wbt_GAS_148_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030);	// 0035 0033 Tune_wbt_GAS_149_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0032);	// 0036 0037 Tune_wbt_GAS_150_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003B);	// 003F 0045 Tune_wbt_GAS_151_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0049);	// 004B 005A Tune_wbt_GAS_152_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005C);	// 0053 007A Tune_wbt_GAS_153_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 0053 00AA Tune_wbt_GAS_154_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008A);	// 0070 00E2 Tune_wbt_GAS_155_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0093);	// 0075 009E Tune_wbt_GAS_156_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 006D 007D Tune_wbt_GAS_157_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0059);	// 0056 005A Tune_wbt_GAS_158_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0042);	// 0043 0041 Tune_wbt_GAS_159_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0032);	// 0037 002E Tune_wbt_GAS_160_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0027);	// 002F 0022 Tune_wbt_GAS_161_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0024);	// 002C 001F Tune_wbt_GAS_162_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0026);	// 002F 0022 Tune_wbt_GAS_163_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 0038 0030 Tune_wbt_GAS_164_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003D);	// 0045 0044 Tune_wbt_GAS_165_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0052);	// 004E 0063 Tune_wbt_GAS_166_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006E);	// 0055 0091 Tune_wbt_GAS_167_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008B);	// 007F 00CB Tune_wbt_GAS_168_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0083);	// 0077 0093 Tune_wbt_GAS_169_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064);	// 0062 006D Tune_wbt_GAS_170_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046);	// 004A 004A Tune_wbt_GAS_171_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030);	// 0037 0031 Tune_wbt_GAS_172_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0020);	// 002A 001E Tune_wbt_GAS_173_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0016);	// 0021 0013 Tune_wbt_GAS_174_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0011);	// 001E 000F Tune_wbt_GAS_175_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014);	// 0021 0013 Tune_wbt_GAS_176_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E);	// 002B 001F Tune_wbt_GAS_177_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002D);	// 003B 0034 Tune_wbt_GAS_178_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0041);	// 0046 0051 Tune_wbt_GAS_179_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005D);	// 0051 007C Tune_wbt_GAS_180_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007C);	// 007F 00B7 Tune_wbt_GAS_181_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 0062 008A Tune_wbt_GAS_182_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0057);	// 004B 0061 Tune_wbt_GAS_183_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0039);	// 0034 003E Tune_wbt_GAS_184_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0024);	// 0022 0026 Tune_wbt_GAS_185_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014);	// 0014 0013 Tune_wbt_GAS_186_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A);	// 000E 0008 Tune_wbt_GAS_187_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0007);	// 000C 0004 Tune_wbt_GAS_188_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0009);	// 000D 0007 Tune_wbt_GAS_189_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0012);	// 0016 0013 Tune_wbt_GAS_190_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0021);	// 0024 0027 Tune_wbt_GAS_191_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0036);	// 0036 0045 Tune_wbt_GAS_192_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0051);	// 004E 0070 Tune_wbt_GAS_193_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0070);	// 0069 00A7 Tune_wbt_GAS_194_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 005F 0085 Tune_wbt_GAS_195_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0056);	// 0048 005D Tune_wbt_GAS_196_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0038);	// 002F 003A Tune_wbt_GAS_197_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0022);	// 001C 0021 Tune_wbt_GAS_198_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0013);	// 0010 0010 Tune_wbt_GAS_199_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0009);	// 000B 0004 Tune_wbt_GAS_200_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// 0008 0000 Tune_wbt_GAS_201_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);	// 000B 0003 Tune_wbt_GAS_202_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0011);	// 0010 000E Tune_wbt_GAS_203_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0020);	// 001E 0021 Tune_wbt_GAS_204_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0035);	// 0031 003F Tune_wbt_GAS_205_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0051);	// 0049 006B Tune_wbt_GAS_206_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0071);	// 0066 00A1 Tune_wbt_GAS_207_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006E);	// 005B 0089 Tune_wbt_GAS_208_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004E);	// 0043 0060 Tune_wbt_GAS_209_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0032);	// 002B 003D Tune_wbt_GAS_210_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001C);	// 0019 0023 Tune_wbt_GAS_211_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000D);	// 000C 0012 Tune_wbt_GAS_212_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 0007 0006 Tune_wbt_GAS_213_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 0004 0002 Tune_wbt_GAS_214_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003);	// 0007 0005 Tune_wbt_GAS_215_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000B);	// 000D 0011 Tune_wbt_GAS_216_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001A);	// 001B 0025 Tune_wbt_GAS_217_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 002E 0043 Tune_wbt_GAS_218_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0049);	// 0046 0070 Tune_wbt_GAS_219_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0068);	// 0062 00A4 Tune_wbt_GAS_220_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0072);	// 0052 0091 Tune_wbt_GAS_221_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0053);	// 003D 0067 Tune_wbt_GAS_222_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0037);	// 0026 0044 Tune_wbt_GAS_223_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0021);	// 0014 002B Tune_wbt_GAS_224_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0012);	// 0007 0019 Tune_wbt_GAS_225_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0009);	// 0002 000D Tune_wbt_GAS_226_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// 0001 0009 Tune_wbt_GAS_227_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);	// 0002 000C Tune_wbt_GAS_228_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010);	// 0007 0018 Tune_wbt_GAS_229_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001F);	// 0015 002D Tune_wbt_GAS_230_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0034);	// 0028 004B Tune_wbt_GAS_231_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004E);	// 0040 007B Tune_wbt_GAS_232_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006C);	// 005C 00B0 Tune_wbt_GAS_233_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007F);	// 005E 00A1 Tune_wbt_GAS_234_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0060);	// 0049 0077 Tune_wbt_GAS_235_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0043);	// 0030 0054 Tune_wbt_GAS_236_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002D);	// 001E 003B Tune_wbt_GAS_237_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001D);	// 0010 0029 Tune_wbt_GAS_238_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0013);	// 0009 001C Tune_wbt_GAS_239_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010);	// 0007 0018 Tune_wbt_GAS_240_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0013);	// 0009 001B Tune_wbt_GAS_241_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001C);	// 0012 0029 Tune_wbt_GAS_242_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002B);	// 0020 003F Tune_wbt_GAS_243_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);	// 0032 005F Tune_wbt_GAS_244_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005A);	// 004B 008F Tune_wbt_GAS_245_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0079);	// 0069 00C6 Tune_wbt_GAS_246_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0082);	// 0066 00B1 Tune_wbt_GAS_247_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0066);	// 004E 008E Tune_wbt_GAS_248_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0049);	// 0037 006D Tune_wbt_GAS_249_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0035);	// 0026 0050 Tune_wbt_GAS_250_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0025);	// 0019 003D Tune_wbt_GAS_251_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001B);	// 0011 0031 Tune_wbt_GAS_252_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0017);	// 000F 002F Tune_wbt_GAS_253_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0019);	// 0012 0032 Tune_wbt_GAS_254_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023);	// 001B 0042 Tune_wbt_GAS_255_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0033);	// 0028 0058 Tune_wbt_GAS_256_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046);	// 003B 007A Tune_wbt_GAS_257_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0060);	// 0054 00AC Tune_wbt_GAS_258_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007B);	// 0072 00E5 Tune_wbt_GAS_259_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0092);	// 006A 00BD Tune_wbt_GAS_260_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007C);	// 0058 00AA Tune_wbt_GAS_261_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0060);	// 0041 008B Tune_wbt_GAS_262_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004B);	// 0030 006E Tune_wbt_GAS_263_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003C);	// 0025 005A Tune_wbt_GAS_264_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0032);	// 001E 004F Tune_wbt_GAS_265_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002D);	// 001B 004B Tune_wbt_GAS_266_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030);	// 001F 0052 Tune_wbt_GAS_267_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0039);	// 0027 0062 Tune_wbt_GAS_268_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0049);	// 0034 007D Tune_wbt_GAS_269_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005D);	// 0046 00A2 Tune_wbt_GAS_270_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0076);	// 005D 00D6 Tune_wbt_GAS_271_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008C);	// 0078 010C Tune_wbt_GAS_272_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x009F);	// 007C 00E5 Tune_wbt_GAS_273_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008F);	// 006A 00C7 Tune_wbt_GAS_274_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 0055 00B1 Tune_wbt_GAS_275_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0061);	// 0043 0093 Tune_wbt_GAS_276_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0052);	// 0037 007F Tune_wbt_GAS_277_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0048);	// 0030 0074 Tune_wbt_GAS_278_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0043);	// 002E 0071 Tune_wbt_GAS_279_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0047);	// 0030 0077 Tune_wbt_GAS_280_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0050);	// 0039 0089 Tune_wbt_GAS_281_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005E);	// 0045 00A7 Tune_wbt_GAS_282_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0071);	// 0056 00CC Tune_wbt_GAS_283_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0086);	// 006C 00FE Tune_wbt_GAS_284_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0097);	// 0084 0132 Tune_wbt_GAS_285_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0093);	// 006E 00A8 Tune_wbt_GAS_286_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007C);	// 006D 008D Tune_wbt_GAS_287_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005F);	// 005B 006C Tune_wbt_GAS_288_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0049);	// 0046 004E Tune_wbt_GAS_289_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003A);	// 003A 003C Tune_wbt_GAS_290_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0030);	// 0033 0032 Tune_wbt_GAS_291_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002C);	// 002D 002D Tune_wbt_GAS_292_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 0032 0032 Tune_wbt_GAS_293_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0037);	// 0039 0040 Tune_wbt_GAS_294_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0045);	// 0047 0056 Tune_wbt_GAS_295_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005A);	// 004F 0076 Tune_wbt_GAS_296_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0075);	// 0050 00A8 Tune_wbt_GAS_297_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008A);	// 006E 00E6 Tune_wbt_GAS_298_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0094);	// 0077 00A2 Tune_wbt_GAS_299_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 006C 007C Tune_wbt_GAS_300_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0057);	// 0054 0059 Tune_wbt_GAS_301_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);	// 0040 003E Tune_wbt_GAS_302_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 0033 002A Tune_wbt_GAS_303_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0024);	// 002B 0020 Tune_wbt_GAS_304_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0020);	// 0027 001B Tune_wbt_GAS_305_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023);	// 002A 0020 Tune_wbt_GAS_306_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002D);	// 0034 002D Tune_wbt_GAS_307_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003B);	// 0041 0042 Tune_wbt_GAS_308_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0051);	// 004C 0061 Tune_wbt_GAS_309_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006E);	// 0052 0092 Tune_wbt_GAS_310_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008C);	// 007E 00CE Tune_wbt_GAS_311_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0085);	// 0078 0094 Tune_wbt_GAS_312_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0066);	// 0063 006F Tune_wbt_GAS_313_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046);	// 0049 004B Tune_wbt_GAS_314_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 0035 002F Tune_wbt_GAS_315_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001F);	// 0028 001D Tune_wbt_GAS_316_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014);	// 001E 0011 Tune_wbt_GAS_317_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000F);	// 001B 000D Tune_wbt_GAS_318_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0012);	// 001F 0012 Tune_wbt_GAS_319_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001C);	// 0028 001D Tune_wbt_GAS_320_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002B);	// 0037 0033 Tune_wbt_GAS_321_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);	// 0044 0051 Tune_wbt_GAS_322_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005C);	// 0050 007F Tune_wbt_GAS_323_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007D);	// 0080 00BA Tune_wbt_GAS_324_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007A);	// 0064 008B Tune_wbt_GAS_325_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005A);	// 004E 0064 Tune_wbt_GAS_326_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003A);	// 0035 0040 Tune_wbt_GAS_327_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0024);	// 0021 0025 Tune_wbt_GAS_328_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014);	// 0013 0013 Tune_wbt_GAS_329_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0009);	// 000D 0007 Tune_wbt_GAS_330_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0006);	// 000B 0003 Tune_wbt_GAS_331_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);	// 000C 0006 Tune_wbt_GAS_332_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0011);	// 0014 0012 Tune_wbt_GAS_333_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0020);	// 0022 0027 Tune_wbt_GAS_334_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0036);	// 0036 0046 Tune_wbt_GAS_335_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0051);	// 004D 0073 Tune_wbt_GAS_336_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0072);	// 006B 00AB Tune_wbt_GAS_337_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007B);	// 0066 008B Tune_wbt_GAS_338_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0059);	// 004C 0062 Tune_wbt_GAS_339_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003A);	// 0032 003F Tune_wbt_GAS_340_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023);	// 001E 0023 Tune_wbt_GAS_341_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0012);	// 0010 0010 Tune_wbt_GAS_342_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);	// 000A 0004 Tune_wbt_GAS_343_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 0007 0000 Tune_wbt_GAS_344_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0007);	// 000B 0003 Tune_wbt_GAS_345_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000F);	// 0010 000E Tune_wbt_GAS_346_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001F);	// 001E 0022 Tune_wbt_GAS_347_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0035);	// 0032 0041 Tune_wbt_GAS_348_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0051);	// 004B 006E Tune_wbt_GAS_349_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0072);	// 006B 00A4 Tune_wbt_GAS_350_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0073);	// 0061 008E Tune_wbt_GAS_351_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0053);	// 0049 0065 Tune_wbt_GAS_352_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0034);	// 002F 0040 Tune_wbt_GAS_353_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001D);	// 001B 0025 Tune_wbt_GAS_354_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000E);	// 000E 0013 Tune_wbt_GAS_355_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 0008 0006 Tune_wbt_GAS_356_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 0004 0001 Tune_wbt_GAS_357_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	// 0008 0005 Tune_wbt_GAS_358_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A);	// 000D 0010 Tune_wbt_GAS_359_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001A);	// 001C 0025 Tune_wbt_GAS_360_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 002F 0044 Tune_wbt_GAS_361_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004A);	// 0047 0074 Tune_wbt_GAS_362_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006A);	// 0067 00AA Tune_wbt_GAS_363_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 005A 0097 Tune_wbt_GAS_364_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0058);	// 0043 006D Tune_wbt_GAS_365_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0039);	// 002B 0048 Tune_wbt_GAS_366_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0022);	// 0017 002D Tune_wbt_GAS_367_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0012);	// 0009 0019 Tune_wbt_GAS_368_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);	// 0004 000E Tune_wbt_GAS_369_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 0002 0009 Tune_wbt_GAS_370_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0007);	// 0004 000C Tune_wbt_GAS_371_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000F);	// 0008 0018 Tune_wbt_GAS_372_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E);	// 0016 002E Tune_wbt_GAS_373_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0034);	// 002A 004E Tune_wbt_GAS_374_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004F);	// 0042 007D Tune_wbt_GAS_375_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006F);	// 005F 00B5 Tune_wbt_GAS_376_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0083);	// 0066 00A6 Tune_wbt_GAS_377_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064);	// 0050 007C Tune_wbt_GAS_378_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0045);	// 0035 0058 Tune_wbt_GAS_379_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002E);	// 0022 003E Tune_wbt_GAS_380_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001D);	// 0013 0029 Tune_wbt_GAS_381_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0012);	// 000A 001D Tune_wbt_GAS_382_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000F);	// 0008 0018 Tune_wbt_GAS_383_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0011);	// 000B 001B Tune_wbt_GAS_384_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001A);	// 0014 0028 Tune_wbt_GAS_385_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002A);	// 0021 003F Tune_wbt_GAS_386_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003F);	// 0035 0062 Tune_wbt_GAS_387_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005B);	// 004D 0093 Tune_wbt_GAS_388_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007B);	// 006E 00CC Tune_wbt_GAS_389_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0087);	// 006E 00B9 Tune_wbt_GAS_390_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006A);	// 0057 0094 Tune_wbt_GAS_391_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004B);	// 003E 0071 Tune_wbt_GAS_392_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0036);	// 002B 0052 Tune_wbt_GAS_393_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0025);	// 001C 003D Tune_wbt_GAS_394_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0019);	// 0013 0031 Tune_wbt_GAS_395_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0015);	// 0011 002D Tune_wbt_GAS_396_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0017);	// 0013 0031 Tune_wbt_GAS_397_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0022);	// 001D 0040 Tune_wbt_GAS_398_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0031);	// 002B 0058 Tune_wbt_GAS_399_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0045);	// 003D 007B Tune_wbt_GAS_400_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0060);	// 0057 00AE Tune_wbt_GAS_401_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007D);	// 0077 00EA Tune_wbt_GAS_402_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0096);	// 0072 00C2 Tune_wbt_GAS_403_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007F);	// 005F 00AE Tune_wbt_GAS_404_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0061);	// 0047 008E Tune_wbt_GAS_405_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004B);	// 0035 006F Tune_wbt_GAS_406_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003B);	// 0028 0059 Tune_wbt_GAS_407_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 0020 004E Tune_wbt_GAS_408_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002A);	// 001D 0049 Tune_wbt_GAS_409_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002D);	// 0020 004F Tune_wbt_GAS_410_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0036);	// 0029 005E Tune_wbt_GAS_411_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046);	// 0035 007A Tune_wbt_GAS_412_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005B);	// 0047 009F Tune_wbt_GAS_413_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0075);	// 0060 00D5 Tune_wbt_GAS_414_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x008D);	// 007D 010C Tune_wbt_GAS_415_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A1);	// 0084 00E5 Tune_wbt_GAS_416_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0091);	// 0072 00C8 Tune_wbt_GAS_417_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 005B 00B0 Tune_wbt_GAS_418_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0060);	// 0046 0091 Tune_wbt_GAS_419_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0050);	// 003A 007D Tune_wbt_GAS_420_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0044);	// 0031 0070 Tune_wbt_GAS_421_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);	// 002E 006D Tune_wbt_GAS_422_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0043);	// 0032 0074 Tune_wbt_GAS_423_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004C);	// 0039 0086 Tune_wbt_GAS_424_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005A);	// 0046 00A4 Tune_wbt_GAS_425_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006D);	// 0056 00CA Tune_wbt_GAS_426_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0084);	// 006E 00FE Tune_wbt_GAS_427_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0094);	// 0087 0134 Tune_wbt_GAS_428_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0072);	// 004C 009F Tune_wbt_GAS_429_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0063);	// 004C 0089 Tune_wbt_GAS_430_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004C);	// 0041 0067 Tune_wbt_GAS_431_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003A);	// 002F 004E Tune_wbt_GAS_432_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002D);	// 0024 003E Tune_wbt_GAS_433_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0025);	// 001D 0033 Tune_wbt_GAS_434_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023);	// 001B 0031 Tune_wbt_GAS_435_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0025);	// 001E 0036 Tune_wbt_GAS_436_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002C);	// 0024 0045 Tune_wbt_GAS_437_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0038);	// 0032 0058 Tune_wbt_GAS_438_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004A);	// 0037 0074 Tune_wbt_GAS_439_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005F);	// 0038 00A0 Tune_wbt_GAS_440_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006B);	// 004C 00C9 Tune_wbt_GAS_441_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0079);	// 005B 0098 Tune_wbt_GAS_442_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0065);	// 0056 0077 Tune_wbt_GAS_443_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004A);	// 0041 0055 Tune_wbt_GAS_444_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0037);	// 0030 003C Tune_wbt_GAS_445_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0029);	// 0026 002A Tune_wbt_GAS_446_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0021);	// 001F 0022 Tune_wbt_GAS_447_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001D);	// 001C 001F Tune_wbt_GAS_448_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001F);	// 001F 0024 Tune_wbt_GAS_449_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0027);	// 0027 0030 Tune_wbt_GAS_450_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0033);	// 0033 0044 Tune_wbt_GAS_451_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0044);	// 003C 0060 Tune_wbt_GAS_452_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005E);	// 0041 008B Tune_wbt_GAS_453_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006E);	// 0060 00B2 Tune_wbt_GAS_454_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006A);	// 005E 0088 Tune_wbt_GAS_455_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0055);	// 004F 0065 Tune_wbt_GAS_456_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003A);	// 003A 0044 Tune_wbt_GAS_457_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028);	// 002A 002C Tune_wbt_GAS_458_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001A);	// 001D 001B Tune_wbt_GAS_459_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0011);	// 0016 0012 Tune_wbt_GAS_460_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000D);	// 0014 000F Tune_wbt_GAS_461_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000F);	// 0016 0013 Tune_wbt_GAS_462_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0017);	// 0021 001E Tune_wbt_GAS_463_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0024);	// 002D 0032 Tune_wbt_GAS_464_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0035);	// 0038 004E Tune_wbt_GAS_465_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004E);	// 0040 0078 Tune_wbt_GAS_466_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0061);	// 0066 00A2 Tune_wbt_GAS_467_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0061);	// 004A 007F Tune_wbt_GAS_468_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004A);	// 003C 005B Tune_wbt_GAS_469_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0031);	// 0028 0039 Tune_wbt_GAS_470_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E);	// 0019 0021 Tune_wbt_GAS_471_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0011);	// 000D 0012 Tune_wbt_GAS_472_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);	// 0008 0007 Tune_wbt_GAS_473_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// 0007 0004 Tune_wbt_GAS_474_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0007);	// 0007 0006 Tune_wbt_GAS_475_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000E);	// 000F 0012 Tune_wbt_GAS_476_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001B);	// 001C 0024 Tune_wbt_GAS_477_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002D);	// 002C 0041 Tune_wbt_GAS_478_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0045);	// 0042 006A Tune_wbt_GAS_479_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0059);	// 0054 0092 Tune_wbt_GAS_480_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0062);	// 004B 007B Tune_wbt_GAS_481_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004B);	// 003C 0057 Tune_wbt_GAS_482_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0031);	// 0027 0036 Tune_wbt_GAS_483_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E);	// 0017 001E Tune_wbt_GAS_484_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010);	// 000C 000F Tune_wbt_GAS_485_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);	// 0007 0003 Tune_wbt_GAS_486_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 0006 0000 Tune_wbt_GAS_487_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0006);	// 0008 0002 Tune_wbt_GAS_488_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000E);	// 000D 000D Tune_wbt_GAS_489_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001B);	// 001A 0020 Tune_wbt_GAS_490_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002E);	// 002B 003C Tune_wbt_GAS_491_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046);	// 0041 0067 Tune_wbt_GAS_492_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005A);	// 0054 008D Tune_wbt_GAS_493_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005B);	// 0049 007E Tune_wbt_GAS_494_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0045);	// 003A 005A Tune_wbt_GAS_495_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002C);	// 0025 0038 Tune_wbt_GAS_496_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001A);	// 0015 0022 Tune_wbt_GAS_497_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000C);	// 000A 0013 Tune_wbt_GAS_498_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003);	// 0005 0006 Tune_wbt_GAS_499_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// 0003 0001 Tune_wbt_GAS_500_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	// 0006 0004 Tune_wbt_GAS_501_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0009);	// 000B 000F Tune_wbt_GAS_502_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0016);	// 0018 0023 Tune_wbt_GAS_503_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0029);	// 0029 003E Tune_wbt_GAS_504_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);	// 003E 006A Tune_wbt_GAS_505_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0054);	// 0050 0091 Tune_wbt_GAS_506_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005F);	// 0044 0085 Tune_wbt_GAS_507_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004A);	// 0033 0060 Tune_wbt_GAS_508_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0031);	// 0021 0041 Tune_wbt_GAS_509_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001F);	// 0011 002A Tune_wbt_GAS_510_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010);	// 0005 0019 Tune_wbt_GAS_511_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0008);	// 0002 000D Tune_wbt_GAS_512_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// 0001 0008 Tune_wbt_GAS_513_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0007);	// 0003 000A Tune_wbt_GAS_514_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000E);	// 0008 0016 Tune_wbt_GAS_515_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001B);	// 0014 002A Tune_wbt_GAS_516_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002E);	// 0025 0047 Tune_wbt_GAS_517_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0045);	// 0039 0074 Tune_wbt_GAS_518_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0059);	// 004D 009A Tune_wbt_GAS_519_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006C);	// 0050 0097 Tune_wbt_GAS_520_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0057);	// 0041 0070 Tune_wbt_GAS_521_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003E);	// 002C 0052 Tune_wbt_GAS_522_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002A);	// 001C 003C Tune_wbt_GAS_523_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001B);	// 0011 0028 Tune_wbt_GAS_524_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0012);	// 0009 001D Tune_wbt_GAS_525_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000F);	// 0008 0019 Tune_wbt_GAS_526_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0011);	// 000A 001A Tune_wbt_GAS_527_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0019);	// 0012 0026 Tune_wbt_GAS_528_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0027);	// 001F 003B Tune_wbt_GAS_529_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0039);	// 002F 005A Tune_wbt_GAS_530_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0050);	// 0045 0089 Tune_wbt_GAS_531_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0063);	// 005A 00AF Tune_wbt_GAS_532_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006F);	// 0056 00A7 Tune_wbt_GAS_533_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005C);	// 0048 0088 Tune_wbt_GAS_534_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0044);	// 0035 006B Tune_wbt_GAS_535_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0031);	// 0026 0050 Tune_wbt_GAS_536_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023);	// 0019 003D Tune_wbt_GAS_537_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0019);	// 0012 0032 Tune_wbt_GAS_538_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0016);	// 0011 002E Tune_wbt_GAS_539_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0017);	// 0012 0030 Tune_wbt_GAS_540_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0020);	// 001B 003E Tune_wbt_GAS_541_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002E);	// 0027 0052 Tune_wbt_GAS_542_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0040);	// 0037 0073 Tune_wbt_GAS_543_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0055);	// 004C 00A1 Tune_wbt_GAS_544_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064);	// 0060 00C7 Tune_wbt_GAS_545_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007E);	// 0058 00AE Tune_wbt_GAS_546_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0071);	// 0050 00A4 Tune_wbt_GAS_547_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0059);	// 003D 0088 Tune_wbt_GAS_548_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046);	// 002E 006D Tune_wbt_GAS_549_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0039);	// 0023 0059 Tune_wbt_GAS_550_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002F);	// 001C 004D Tune_wbt_GAS_551_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002A);	// 001B 0049 Tune_wbt_GAS_552_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002D);	// 001D 004E Tune_wbt_GAS_553_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0035);	// 0024 005B Tune_wbt_GAS_554_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0043);	// 002F 0073 Tune_wbt_GAS_555_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0054);	// 003E 0095 Tune_wbt_GAS_556_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0069);	// 0052 00C4 Tune_wbt_GAS_557_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0074);	// 0062 00E9 Tune_wbt_GAS_558_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0083);	// 0060 00D5 Tune_wbt_GAS_559_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007D);	// 0057 00C1 Tune_wbt_GAS_560_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0068);	// 0048 00AB Tune_wbt_GAS_561_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0055);	// 0039 008D Tune_wbt_GAS_562_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0048);	// 002E 0079 Tune_wbt_GAS_563_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003E);	// 0028 0070 Tune_wbt_GAS_564_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003A);	// 0027 006A Tune_wbt_GAS_565_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003D);	// 0029 006F Tune_wbt_GAS_566_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0045);	// 002F 0080 Tune_wbt_GAS_567_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0051);	// 0039 0096 Tune_wbt_GAS_568_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0061);	// 0047 00B9 Tune_wbt_GAS_569_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0072);	// 0059 00E2 Tune_wbt_GAS_570_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0077);	// 0067 010F Tune_wbt_GAS_571_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x1348);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// gisp_gras_Enable 

	//*********************************************************************************
	// 07. Analog Setting 2                                                         
	//*********************************************************************************
	// This register is for FACTORY ONLY.                                               
	// If you change it without prior notification                                   
	// YOU are RESPONSIBLE for the FAILURE that will happen in the future             
	// For CDS Timing                                                                  
	//*********************************************************************************
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0EC6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000B);	// 11d	aig_fdba_ptr0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0ECE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000B);	// 11d	aig_fdbb_ptr0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0F16);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002B);	// 43d	aig_sfdba_reg0
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0F1E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x002B);	// 43d	aig_sfdbb_reg0
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0EC8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000B);	// 11d	aig_fdba_ptr0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0ED0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x033F);	// 831d	aig_fdbb_ptr0 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0F18);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0366);	// 870d	aig_sfdba_reg0
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0F20);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0032);	// 50d	aig_sfdbb_reg0

	// For Other Analog Settings 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0E38);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0476);	// senHal_RegCompBiasNormSf CDS bias 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0476);	// senHal_RegCompBiasYAv CDS bias 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0AA0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// setot_bUseDigitalHbin 1-Digital, 0-Analog 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0E2C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// senHal_bUseAnalogVerAv 2-Adding/averaging, 1-Y-Avg, 0-PLA 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x1250);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFF); 	// senHal_Bls_nSpExpLines 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x1202);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010); 	// senHal_Dblr_VcoFreqMHZ 
	//*********************************************************************************
	// 08.AF Setting                                                                    
	//*********************************************************************************

	//*********************************************************************************
	// 09.AWB-BASIC setting                                                          
	//*********************************************************************************
	// For WB Calibration 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B36);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// awbb_IndoorGrZones_ZInfo_m_GridStep 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B3A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00EC);	// awbb_IndoorGrZones_ZInfo_m_BMin 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02C1);	// awbb_IndoorGrZones_ZInfo_m_BMax 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B38);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010);	// awbb_IndoorGrZones_ZInfo_m_GridSz 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0AE6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03E1);	// awbb_IndoorGrZones_m_BGrid_0__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0413);	// awbb_IndoorGrZones_m_BGrid_0__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x039E);	// awbb_IndoorGrZones_m_BGrid_1__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0416);	// awbb_IndoorGrZones_m_BGrid_1__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0367);	// awbb_IndoorGrZones_m_BGrid_2__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03F3);	// awbb_IndoorGrZones_m_BGrid_2__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x032D);	// awbb_IndoorGrZones_m_BGrid_3__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03C5);	// awbb_IndoorGrZones_m_BGrid_3__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02FD);	// awbb_IndoorGrZones_m_BGrid_4__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x038F);	// awbb_IndoorGrZones_m_BGrid_4__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02D3);	// awbb_IndoorGrZones_m_BGrid_5__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0365);	// awbb_IndoorGrZones_m_BGrid_5__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02AA);	// awbb_IndoorGrZones_m_BGrid_6__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x033E);	// awbb_IndoorGrZones_m_BGrid_6__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x028D);	// awbb_IndoorGrZones_m_BGrid_7__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0310);	// awbb_IndoorGrZones_m_BGrid_7__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0271);	// awbb_IndoorGrZones_m_BGrid_8__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02F1);	// awbb_IndoorGrZones_m_BGrid_8__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x025A);	// awbb_IndoorGrZones_m_BGrid_9__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02D2);	// awbb_IndoorGrZones_m_BGrid_9__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0249);	// awbb_IndoorGrZones_m_BGrid_10__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02B9);	// awbb_IndoorGrZones_m_BGrid_10__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0238);	// awbb_IndoorGrZones_m_BGrid_11__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A2);	// awbb_IndoorGrZones_m_BGrid_11__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x021B);	// awbb_IndoorGrZones_m_BGrid_12__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0289);	// awbb_IndoorGrZones_m_BGrid_12__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0200);	// awbb_IndoorGrZones_m_BGrid_13__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x026C);	// awbb_IndoorGrZones_m_BGrid_13__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01FC);	// awbb_IndoorGrZones_m_BGrid_14__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x024F);	// awbb_IndoorGrZones_m_BGrid_14__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x021E);	// awbb_IndoorGrZones_m_BGrid_15__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x022C);	// awbb_IndoorGrZones_m_BGrid_15__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_IndoorGrZones_m_BGrid_16__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_IndoorGrZones_m_BGrid_16__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_IndoorGrZones_m_BGrid_17__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_IndoorGrZones_m_BGrid_17__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_IndoorGrZones_m_BGrid_18__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_IndoorGrZones_m_BGrid_18__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_IndoorGrZones_m_BGrid_19__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_IndoorGrZones_m_BGrid_19__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BAA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0006);	// awbb_LowBrGrZones_ZInfo_m_GridStep 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BAE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010E);	// awbb_LowBrGrZones_ZInfo_m_BMin 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02E9);	// awbb_LowBrGrZones_ZInfo_m_BMax 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BAC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0009);	// awbb_LowBrGrZones_ZInfo_m_GridSz 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B7A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x038C);	// awbb_LowBrGrZones_m_BGrid_0__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03DA);	// awbb_LowBrGrZones_m_BGrid_0__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030E);	// awbb_LowBrGrZones_m_BGrid_1__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03E9);	// awbb_LowBrGrZones_m_BGrid_1__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A2);	// awbb_LowBrGrZones_m_BGrid_2__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03C2);	// awbb_LowBrGrZones_m_BGrid_2__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0259);	// awbb_LowBrGrZones_m_BGrid_3__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x038A);	// awbb_LowBrGrZones_m_BGrid_3__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0218);	// awbb_LowBrGrZones_m_BGrid_4__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0352);	// awbb_LowBrGrZones_m_BGrid_4__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01F4);	// awbb_LowBrGrZones_m_BGrid_5__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02E1);	// awbb_LowBrGrZones_m_BGrid_5__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01D7);	// awbb_LowBrGrZones_m_BGrid_6__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x028E);	// awbb_LowBrGrZones_m_BGrid_6__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01CB);	// awbb_LowBrGrZones_m_BGrid_7__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0258);	// awbb_LowBrGrZones_m_BGrid_7__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x022B);	// awbb_LowBrGrZones_m_BGrid_8__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01CC);	// awbb_LowBrGrZones_m_BGrid_8__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_LowBrGrZones_m_BGrid_9__m_left   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_LowBrGrZones_m_BGrid_9__m_right  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_LowBrGrZones_m_BGrid_10__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_LowBrGrZones_m_BGrid_10__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_LowBrGrZones_m_BGrid_11__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_LowBrGrZones_m_BGrid_11__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B70);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// awbb_OutdoorGrZones_ZInfo_m_GridStep
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B74);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01F8);	// awbb_OutdoorGrZones_ZInfo_m_BMin 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A8);	// awbb_OutdoorGrZones_ZInfo_m_BMax 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B72);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0007);	// awbb_OutdoorGrZones_ZInfo_m_GridSz 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0B40);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x029E);	// awbb_OutdoorGrZones_m_BGrid_0__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02C8);	// awbb_OutdoorGrZones_m_BGrid_0__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0281);	// awbb_OutdoorGrZones_m_BGrid_1__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02C8);	// awbb_OutdoorGrZones_m_BGrid_1__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0266);	// awbb_OutdoorGrZones_m_BGrid_2__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02AC);	// awbb_OutdoorGrZones_m_BGrid_2__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0251);	// awbb_OutdoorGrZones_m_BGrid_3__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x028E);	// awbb_OutdoorGrZones_m_BGrid_3__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x023D);	// awbb_OutdoorGrZones_m_BGrid_4__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0275);	// awbb_OutdoorGrZones_m_BGrid_4__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0228);	// awbb_OutdoorGrZones_m_BGrid_5__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x025D);	// awbb_OutdoorGrZones_m_BGrid_5__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0228);	// awbb_OutdoorGrZones_m_BGrid_6__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0243);	// awbb_OutdoorGrZones_m_BGrid_6__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_7__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_7__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_8__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_8__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_9__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_9__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_10__m_left 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_10__m_right
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_11__m_left 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGrZones_m_BGrid_11__m_right
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BC8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// awbb_CWSkinZone_ZInfo_m_GridStep 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BCC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010F);	// awbb_CWSkinZone_ZInfo_m_BMin 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x018F);	// awbb_CWSkinZone_ZInfo_m_BMax 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BCA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// awbb_CWSkinZone_ZInfo_m_GridSz 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BB4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03E7);	// awbb_CWSkinZone_m_BGrid_0__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03F8);	// awbb_CWSkinZone_m_BGrid_0__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03A7);	// awbb_CWSkinZone_m_BGrid_1__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FC);	// awbb_CWSkinZone_m_BGrid_1__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0352);	// awbb_CWSkinZone_m_BGrid_2__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03D0);	// awbb_CWSkinZone_m_BGrid_2__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0322);	// awbb_CWSkinZone_m_BGrid_3__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x039E);	// awbb_CWSkinZone_m_BGrid_3__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x032B);	// awbb_CWSkinZone_m_BGrid_4__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x034D);	// awbb_CWSkinZone_m_BGrid_4__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BE6);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0006);	// awbb_DLSkinZone_ZInfo_m_GridStep 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BEA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x019E);	// awbb_DLSkinZone_ZInfo_m_BMin
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0257);	// awbb_DLSkinZone_ZInfo_m_BMax
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BE8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// awbb_DLSkinZone_ZInfo_m_GridSz 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BD2);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030B);	// awbb_DLSkinZone_m_BGrid_0__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0323);	// awbb_DLSkinZone_m_BGrid_0__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02C3);	// awbb_DLSkinZone_m_BGrid_1__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F);	// awbb_DLSkinZone_m_BGrid_1__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0288);	// awbb_DLSkinZone_m_BGrid_2__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02E5);	// awbb_DLSkinZone_m_BGrid_2__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x026A);	// awbb_DLSkinZone_m_BGrid_3__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A2);	// awbb_DLSkinZone_m_BGrid_3__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_DLSkinZone_m_BGrid_4__m_left  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_DLSkinZone_m_BGrid_4__m_right 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C2C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0139);	// awbb_IntcR 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0122);	// awbb_IntcB 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BFC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03AD);	// awbb_IndoorWP_0__r 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x013F);	// awbb_IndoorWP_0__b 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0341);	// awbb_IndoorWP_1__r 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x017B);	// awbb_IndoorWP_1__b 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x038D);	// awbb_IndoorWP_2__r 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x014B);	// awbb_IndoorWP_2__b 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02C3);	// awbb_IndoorWP_3__r 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01CC);	// awbb_IndoorWP_3__b 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0241);	// awbb_IndoorWP_4__r 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x027F);	// awbb_IndoorWP_4__b 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0241);	// awbb_IndoorWP_5__r 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x027F);	// awbb_IndoorWP_5__b 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0214);	// awbb_IndoorWP_6__r 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A8);	// awbb_IndoorWP_6__b 

	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0270);	// 255 awbb_OutdoorWP_r 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0210);	// 25B awbb_OutdoorWP_b   

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C4C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0452);	// awbb_MvEq_RBthresh 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C58);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x059C);	// awbb_LowTempRB 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0BF8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01AE);	// awbb_LowTSep_m_RminusB 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C28);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_SkinPreference 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CAC);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0050);	// awbb_OutDMaxIncr 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C28);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_SkinPreference 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0D0E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B8);	// awbb_GridCoeff_R_2
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B2);	// awbb_GridCoeff_B_2
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CFE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0FAB);	// awbb_GridConst_2_0
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0FF5);	// awbb_GridConst_2_1
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x10BB);	// awbb_GridConst_2_2
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1123);	// awbb_GridConst_2_3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1165);	// awbb_GridConst_2_4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x122A);	// awbb_GridConst_2_5
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A9);	// awbb_GridCoeff_R_1
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00C0);	// awbb_GridCoeff_B_1
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CF8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030E);	// awbb_GridConst_1_0_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x034C);	// awbb_GridConst_1_1_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0388);	// awbb_GridConst_1_2_

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0CB0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_R_0__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_R_0__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064);	//awbb_GridCorr_R_0__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064);	//awbb_GridCorr_R_0__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0078);	//awbb_GridCorr_R_0__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0078);	//awbb_GridCorr_R_0__5_ 

	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_R_1__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_R_1__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064);	//awbb_GridCorr_R_1__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064);	//awbb_GridCorr_R_1__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0078);	//awbb_GridCorr_R_1__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0078);	//awbb_GridCorr_R_1__5_ 

	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_R_2__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_R_2__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064);	//awbb_GridCorr_R_2__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064);	//awbb_GridCorr_R_2__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0078);	//awbb_GridCorr_R_2__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0078);	//awbb_GridCorr_R_2__5_ 

	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_B_0__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_B_0__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_B_0__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFC0);	//awbb_GridCorr_B_0__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF60);	//awbb_GridCorr_B_0__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF40);	//awbb_GridCorr_B_0__5_ 

	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_B_1__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_B_1__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_B_1__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFC0);	//awbb_GridCorr_B_1__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF60);	//awbb_GridCorr_B_1__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF40);	//awbb_GridCorr_B_1__5_ 

	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_B_2__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_B_2__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//awbb_GridCorr_B_2__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFC0);	//awbb_GridCorr_B_2__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF60);	//awbb_GridCorr_B_2__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF40);	//awbb_GridCorr_B_2__5_ 

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0D30);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	// awbb_GridEnable 

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x3372);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// awbb_bUseOutdoorGrid 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGridCorr_R 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// awbb_OutdoorGridCorr_B          
	     
	     
	// For Outdoor Detector 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C86);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005);	// awbb_OutdoorDetectionZone_ZInfo_m_GridSz 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C70);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF7B);	// awbb_OutdoorDetectionZone_m_BGrid_0__m_left 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00CE);	// awbb_OutdoorDetectionZone_m_BGrid_0__m_right
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF23);	// awbb_OutdoorDetectionZone_m_BGrid_1__m_left 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010D);	// awbb_OutdoorDetectionZone_m_BGrid_1__m_right
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFEF3);	// awbb_OutdoorDetectionZone_m_BGrid_2__m_left 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012C);	// awbb_OutdoorDetectionZone_m_BGrid_2__m_right
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFED7);	// awbb_OutdoorDetectionZone_m_BGrid_3__m_left 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x014E);	// awbb_OutdoorDetectionZone_m_BGrid_3__m_right
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFEBB);	// awbb_OutdoorDetectionZone_m_BGrid_4__m_left 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0162);	// awbb_OutdoorDetectionZone_m_BGrid_4__m_right
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1388);	// awbb_OutdoorDetectionZone_ZInfo_m_AbsGridStep
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C8A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4ACB);	// awbb_OutdoorDetectionZone_ZInfo_m_MaxNB 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0C88);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A7C);	// awbb_OutdoorDetectionZone_ZInfo_m_NBoffs

	//==================================================================================
	// 10.Clock Setting
	//==================================================================================
	// Input Clock (Mclk) 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x012E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x5DC0);	//input clock
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0146);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //REG_TC_IPRM_UseNPviClocks
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002); //REG_TC_IPRM_UseNMipiClocks

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2CEC);//2328 //REG_TC_IPRM_sysClocks_0
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0152);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x59D8);//4650	//REG_TC_IPRM_MinOutRate4KHz_0
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x014E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x59D8);//4650 //REG_TC_IPRM_MaxOutRate4KHz_0
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0154);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2981); //29FE //REG_TC_IPRM_sysClocks_1
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x015A);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x5208); //5302	//REG_TC_IPRM_MinOutRate4KHz_1
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0156);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x53FC); //54F6 //REG_TC_IPRM_MaxOutRate4KHz_1

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0164); //update PLL
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);
	//*********************************************************************************
	// 11.Auto Flicker Detection                                                        
	//*********************************************************************************
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028,0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x03F4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001); //0002	//REG_SF_USER_FlickerQuant 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	//REG_SF_USER_FlickerQuantChanged 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0408);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x067F);	//REG_TC_DBG_AutoAlgEnBits all AA are on 

	//*********************************************************************************
	// 12.AE Setting                                                                   
	//*********************************************************************************

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0D40);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003E); // 3E TVAR_ae_BrAve 
	// For LT Calibration 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0D46);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000F);	// ae_StatMode 

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0440);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3410);	// lt_uMaxExp_0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0444);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x6590);	// lt_uMaxExp_1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0448);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xBB80);	// lt_uMaxExp_2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x044C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3880);	// lt_uMaxExp_3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0450);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3410);	// lt_uCapMaxExp_0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0454);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x6590);	// lt_uCapMaxExp_1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0458);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xBB80);	// lt_uCapMaxExp_2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x045C);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3880);	// lt_uCapMaxExp_3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01B0);	// lt_uMaxAnGain_0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01B0);	// lt_uMaxAnGain_1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0280);	// lt_uMaxAnGain_2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0800);	// lt_uMaxAnGain_3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	// lt_uMaxDigGain 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3000);	// lt_uMaxTotGain 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x042E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010E);	// lt_uMaxTotGain 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00F5);	// lt_uLimitLow 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0DE0);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	// ae_Fade2BlackEnable F2B off, F2W on 

	// For Illum Type Calibration
	// WRITE SARR_IllumType_0_  0078 
	// WRITE SARR_IllumType_1_  00C3 
	// WRITE SARR_IllumType_2_  00E9 
	// WRITE SARR_IllumType_3_  0128 
	// WRITE SARR_IllumType_4_  016F 
	// WRITE SARR_IllumType_5_  0195 
	// WRITE SARR_IllumType_6_  01A4 
	// WRITE SARR_IllumTypeF_0_  0100
	// WRITE SARR_IllumTypeF_1_  0100
	// WRITE SARR_IllumTypeF_2_  0110
	// WRITE SARR_IllumTypeF_3_  00E5
	// WRITE SARR_IllumTypeF_4_  0100
	// WRITE SARR_IllumTypeF_5_  00ED
	// WRITE SARR_IllumTypeF_6_  00ED

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0D38); // bp_uMaxBrightnessFactor 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0198);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0D3E); // bp_uMinBrightnessFactor 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A0);

	//*********************************************************************************
	// 13.AE Weight (Normal)                                                           
	//*********************************************************************************
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0D4E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //ae_WeightTbl_16_0_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0101); //ae_WeightTbl_16_1_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0101); //ae_WeightTbl_16_2_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //ae_WeightTbl_16_3_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0201); //ae_WeightTbl_16_4_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0202); //ae_WeightTbl_16_5_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0202); //ae_WeightTbl_16_6_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0102); //ae_WeightTbl_16_7_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0201); //ae_WeightTbl_16_8_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0303); //ae_WeightTbl_16_9_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0303); //ae_WeightTbl_16_10_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0102); //ae_WeightTbl_16_11_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0201); //ae_WeightTbl_16_12_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0403); //ae_WeightTbl_16_13_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0304); //ae_WeightTbl_16_14_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0102); //ae_WeightTbl_16_15_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0201); //ae_WeightTbl_16_16_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0403); //ae_WeightTbl_16_17_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0304); //ae_WeightTbl_16_18_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0102); //ae_WeightTbl_16_19_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0201); //ae_WeightTbl_16_20_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0303); //ae_WeightTbl_16_21_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0303); //ae_WeightTbl_16_22_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0102); //ae_WeightTbl_16_23_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0201); //ae_WeightTbl_16_24_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0202); //ae_WeightTbl_16_25_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0202); //ae_WeightTbl_16_26_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0102); //ae_WeightTbl_16_27_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //ae_WeightTbl_16_28_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0101); //ae_WeightTbl_16_29_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0101); //ae_WeightTbl_16_30_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //ae_WeightTbl_16_31_ 

	//*********************************************************************************
	// 14.Flash Setting                                                                
	//*********************************************************************************

	//*********************************************************************************
	// 15.CCM Setting                                                                  
	//*********************************************************************************

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x33A4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01D0);	// Tune_wbt_BaseCcms_0__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFA1);	// Tune_wbt_BaseCcms_0__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFA);	// Tune_wbt_BaseCcms_0__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF6F);	// Tune_wbt_BaseCcms_0__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0140);	// Tune_wbt_BaseCcms_0__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF49);	// Tune_wbt_BaseCcms_0__5_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFC1);	// Tune_wbt_BaseCcms_0__6_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001F);	// Tune_wbt_BaseCcms_0__7_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01BD);	// Tune_wbt_BaseCcms_0__8_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x013F);	// Tune_wbt_BaseCcms_0__9_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00E1);	// Tune_wbt_BaseCcms_0__10_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF43);	// Tune_wbt_BaseCcms_0__11_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0191);	// Tune_wbt_BaseCcms_0__12_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFC0);	// Tune_wbt_BaseCcms_0__13_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01B7);	// Tune_wbt_BaseCcms_0__14_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF30);	// Tune_wbt_BaseCcms_0__15_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x015F);	// Tune_wbt_BaseCcms_0__16_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0106);	// Tune_wbt_BaseCcms_0__17_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01D0);	// Tune_wbt_BaseCcms_1__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFA1);	// Tune_wbt_BaseCcms_1__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFA);	// Tune_wbt_BaseCcms_1__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF6F);	// Tune_wbt_BaseCcms_1__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0140);	// Tune_wbt_BaseCcms_1__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF49);	// Tune_wbt_BaseCcms_1__5_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFC1);	// Tune_wbt_BaseCcms_1__6_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001F);	// Tune_wbt_BaseCcms_1__7_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01BD);	// Tune_wbt_BaseCcms_1__8_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x013F);	// Tune_wbt_BaseCcms_1__9_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00E1);	// Tune_wbt_BaseCcms_1__10_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF43);	// Tune_wbt_BaseCcms_1__11_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0191);	// Tune_wbt_BaseCcms_1__12_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFC0);	// Tune_wbt_BaseCcms_1__13_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01B7);	// Tune_wbt_BaseCcms_1__14_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF30);	// Tune_wbt_BaseCcms_1__15_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x015F);	// Tune_wbt_BaseCcms_1__16_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0106);	// Tune_wbt_BaseCcms_1__17_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01D0);	// Tune_wbt_BaseCcms_2__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFA1);	// Tune_wbt_BaseCcms_2__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFA);	// Tune_wbt_BaseCcms_2__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF6F);	// Tune_wbt_BaseCcms_2__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0140);	// Tune_wbt_BaseCcms_2__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF49);	// Tune_wbt_BaseCcms_2__5_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFE3);	// FFC1 Tune_wbt_BaseCcms_2__6_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFF9);	// 001F Tune_wbt_BaseCcms_2__7_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01C1);	// 01BD Tune_wbt_BaseCcms_2__8_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x013F);	// Tune_wbt_BaseCcms_2__9_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00E1);	// Tune_wbt_BaseCcms_2__10_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF43);	// Tune_wbt_BaseCcms_2__11_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0191);	// Tune_wbt_BaseCcms_2__12_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFC0);	// Tune_wbt_BaseCcms_2__13_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01B7);	// Tune_wbt_BaseCcms_2__14_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF30);	// Tune_wbt_BaseCcms_2__15_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x015F);	// Tune_wbt_BaseCcms_2__16_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0106);	// Tune_wbt_BaseCcms_2__17_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01D0);	// Tune_wbt_BaseCcms_3__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFA1);	// Tune_wbt_BaseCcms_3__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFA);	// Tune_wbt_BaseCcms_3__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF6F);	// Tune_wbt_BaseCcms_3__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0140);	// Tune_wbt_BaseCcms_3__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF49);	// Tune_wbt_BaseCcms_3__5_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFE3);	// FFC1 Tune_wbt_BaseCcms_3__6_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFF9);	// 001F Tune_wbt_BaseCcms_3__7_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01C1);	// 01BD Tune_wbt_BaseCcms_3__8_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x013F);	// Tune_wbt_BaseCcms_3__9_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00E1);	// Tune_wbt_BaseCcms_3__10_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF43);	// Tune_wbt_BaseCcms_3__11_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0191);	// Tune_wbt_BaseCcms_3__12_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFC0);	// Tune_wbt_BaseCcms_3__13_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01B7);	// Tune_wbt_BaseCcms_3__14_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF30);	// Tune_wbt_BaseCcms_3__15_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x015F);	// Tune_wbt_BaseCcms_3__16_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0106);	// Tune_wbt_BaseCcms_3__17_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01BF);	// Tune_wbt_BaseCcms_4__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFBF);	// Tune_wbt_BaseCcms_4__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFE);	// Tune_wbt_BaseCcms_4__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF6D);	// Tune_wbt_BaseCcms_4__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01B4);	// Tune_wbt_BaseCcms_4__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF66);	// Tune_wbt_BaseCcms_4__5_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFCA);	// Tune_wbt_BaseCcms_4__6_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFCE);	// Tune_wbt_BaseCcms_4__7_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x017B);	// Tune_wbt_BaseCcms_4__8_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0136);	// Tune_wbt_BaseCcms_4__9_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0132);	// Tune_wbt_BaseCcms_4__10_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF85);	// Tune_wbt_BaseCcms_4__11_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x018B);	// Tune_wbt_BaseCcms_4__12_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF73);	// Tune_wbt_BaseCcms_4__13_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0191);	// Tune_wbt_BaseCcms_4__14_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF3F);	// Tune_wbt_BaseCcms_4__15_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x015B);	// Tune_wbt_BaseCcms_4__16_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D0);	// Tune_wbt_BaseCcms_4__17_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01BF);	// Tune_wbt_BaseCcms_5__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFBF);	// Tune_wbt_BaseCcms_5__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFE);	// Tune_wbt_BaseCcms_5__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF6D);	// Tune_wbt_BaseCcms_5__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01B4);	// Tune_wbt_BaseCcms_5__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF66);	// Tune_wbt_BaseCcms_5__5_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFCA);	// Tune_wbt_BaseCcms_5__6_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFCE);	// Tune_wbt_BaseCcms_5__7_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x017B);	// Tune_wbt_BaseCcms_5__8_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0136);	// Tune_wbt_BaseCcms_5__9_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0132);	// Tune_wbt_BaseCcms_5__10_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF85);	// Tune_wbt_BaseCcms_5__11_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x018B);	// Tune_wbt_BaseCcms_5__12_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF73);	// Tune_wbt_BaseCcms_5__13_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0191);	// Tune_wbt_BaseCcms_5__14_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF3F);	// Tune_wbt_BaseCcms_5__15_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x015B);	// Tune_wbt_BaseCcms_5__16_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D0);	// Tune_wbt_BaseCcms_5__17_
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x3380);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01AC);	// Tune_wbt_OutdoorCcm_0_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFD7);	// Tune_wbt_OutdoorCcm_1_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0019);	// Tune_wbt_OutdoorCcm_2_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF49);	// Tune_wbt_OutdoorCcm_3_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01D9);	// Tune_wbt_OutdoorCcm_4_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF63);	// Tune_wbt_OutdoorCcm_5_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFCA);	// Tune_wbt_OutdoorCcm_6_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFCE);	// Tune_wbt_OutdoorCcm_7_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x017B);	// Tune_wbt_OutdoorCcm_8_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0132);	// Tune_wbt_OutdoorCcm_9_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012E);	// Tune_wbt_OutdoorCcm_10_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF8D);	// Tune_wbt_OutdoorCcm_11_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x018B);	// Tune_wbt_OutdoorCcm_12_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF73);	// Tune_wbt_OutdoorCcm_13_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0191);	// Tune_wbt_OutdoorCcm_14_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF3F);	// Tune_wbt_OutdoorCcm_15_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x015B);	// Tune_wbt_OutdoorCcm_16_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D0);	// Tune_wbt_OutdoorCcm_17_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0612);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x009D);	// SARR_AwbCcmCord_0_      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D5);	// SARR_AwbCcmCord_1_      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103);	// SARR_AwbCcmCord_2_      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0128);	// SARR_AwbCcmCord_3_      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0166);	// SARR_AwbCcmCord_4_      
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0193);	// SARR_AwbCcmCord_5_      

	//*********************************************************************************
	// 16.GAMMA                                                                         
	//*********************************************************************************
	// For Pre Post Gamma Calibration
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0538);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// seti_uGammaLutPreDemNoBin_0_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001F);	// seti_uGammaLutPreDemNoBin_1_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0035);	// seti_uGammaLutPreDemNoBin_2_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x005A);	// seti_uGammaLutPreDemNoBin_3_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0095);	// seti_uGammaLutPreDemNoBin_4_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00E6);	// seti_uGammaLutPreDemNoBin_5_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0121);	// seti_uGammaLutPreDemNoBin_6_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0139);	// seti_uGammaLutPreDemNoBin_7_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0150);	// seti_uGammaLutPreDemNoBin_8_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0177);	// seti_uGammaLutPreDemNoBin_9_   
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x019A);	// seti_uGammaLutPreDemNoBin_10_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01BB);	// seti_uGammaLutPreDemNoBin_11_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01DC);	// seti_uGammaLutPreDemNoBin_12_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0219);	// seti_uGammaLutPreDemNoBin_13_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0251);	// seti_uGammaLutPreDemNoBin_14_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02B3);	// seti_uGammaLutPreDemNoBin_15_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030A);	// seti_uGammaLutPreDemNoBin_16_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x035F);	// seti_uGammaLutPreDemNoBin_17_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03B1);	// seti_uGammaLutPreDemNoBin_18_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF);	// seti_uGammaLutPreDemNoBin_19_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// seti_uGammaLutPostDemNoBin_0_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// seti_uGammaLutPostDemNoBin_1_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// seti_uGammaLutPostDemNoBin_2_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	// seti_uGammaLutPostDemNoBin_3_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// seti_uGammaLutPostDemNoBin_4_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A);	// seti_uGammaLutPostDemNoBin_5_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0012);	// seti_uGammaLutPostDemNoBin_6_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0016);	// seti_uGammaLutPostDemNoBin_7_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001A);	// seti_uGammaLutPostDemNoBin_8_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0024);	// seti_uGammaLutPostDemNoBin_9_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0031);	// seti_uGammaLutPostDemNoBin_10_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x003E);	// seti_uGammaLutPostDemNoBin_11_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x004E);	// seti_uGammaLutPostDemNoBin_12_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0075);	// seti_uGammaLutPostDemNoBin_13_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00A8);	// seti_uGammaLutPostDemNoBin_14_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0126);	// seti_uGammaLutPostDemNoBin_15_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01BE);	// seti_uGammaLutPostDemNoBin_16_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0272);	// seti_uGammaLutPostDemNoBin_17_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0334);	// seti_uGammaLutPostDemNoBin_18_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF);	// seti_uGammaLutPostDemNoBin_19_ 

	// For Gamma Calibration 

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0498);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// SARR_usDualGammaLutRGBIndoor_0__0_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	// SARR_usDualGammaLutRGBIndoor_0__1_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0007);	// SARR_usDualGammaLutRGBIndoor_0__2_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001D);	// SARR_usDualGammaLutRGBIndoor_0__3_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006E);	// SARR_usDualGammaLutRGBIndoor_0__4_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D3);	// SARR_usDualGammaLutRGBIndoor_0__5_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0127);	// SARR_usDualGammaLutRGBIndoor_0__6_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x014C);	// SARR_usDualGammaLutRGBIndoor_0__7_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x016E);	// SARR_usDualGammaLutRGBIndoor_0__8_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01A5);	// SARR_usDualGammaLutRGBIndoor_0__9_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01D3);	// SARR_usDualGammaLutRGBIndoor_0__10_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01FB);	// SARR_usDualGammaLutRGBIndoor_0__11_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x021F);	// SARR_usDualGammaLutRGBIndoor_0__12_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0260);	// SARR_usDualGammaLutRGBIndoor_0__13_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x029A);	// SARR_usDualGammaLutRGBIndoor_0__14_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02F7);	// SARR_usDualGammaLutRGBIndoor_0__15_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x034D);	// SARR_usDualGammaLutRGBIndoor_0__16_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0395);	// SARR_usDualGammaLutRGBIndoor_0__17_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03CE);	// SARR_usDualGammaLutRGBIndoor_0__18_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF);	// SARR_usDualGammaLutRGBIndoor_0__19_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// SARR_usDualGammaLutRGBOutdoor_0__0_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004);	// SARR_usDualGammaLutRGBOutdoor_0__1_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000C);	// SARR_usDualGammaLutRGBOutdoor_0__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0024);	// SARR_usDualGammaLutRGBOutdoor_0__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006E);	// SARR_usDualGammaLutRGBOutdoor_0__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00D1);	// SARR_usDualGammaLutRGBOutdoor_0__5_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0119);	// SARR_usDualGammaLutRGBOutdoor_0__6_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0139);	// SARR_usDualGammaLutRGBOutdoor_0__7_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0157);	// SARR_usDualGammaLutRGBOutdoor_0__8_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x018E);	// SARR_usDualGammaLutRGBOutdoor_0__9_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01C3);	// SARR_usDualGammaLutRGBOutdoor_0__10_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01F3);	// SARR_usDualGammaLutRGBOutdoor_0__11_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x021F);	// SARR_usDualGammaLutRGBOutdoor_0__12_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0269);	// SARR_usDualGammaLutRGBOutdoor_0__13_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02A6);	// SARR_usDualGammaLutRGBOutdoor_0__14_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x02FF);	// SARR_usDualGammaLutRGBOutdoor_0__15_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0351);	// SARR_usDualGammaLutRGBOutdoor_0__16_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0395);	// SARR_usDualGammaLutRGBOutdoor_0__17_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03CE);	// SARR_usDualGammaLutRGBOutdoor_0__18_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF);	// SARR_usDualGammaLutRGBOutdoor_0__19_

	//*********************************************************************************
	// 17.AFIT                                                                          
	//*********************************************************************************
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x3370);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// afit_bUseNormBrForAfit

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x06D4);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0032);	// afit_uNoiseIndInDoor_0
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0078);	// afit_uNoiseIndInDoor_1
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00C8);	// afit_uNoiseIndInDoor_2
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0190);	// afit_uNoiseIndInDoor_3
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x028C);	// afit_uNoiseIndInDoor_4

	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0734);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__0_  Brightness[0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__1_  Contrast[0] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__2_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__3_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__4_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0078); // AfitBaseVals_0__5_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012C); // AfitBaseVals_0__6_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF); // AfitBaseVals_0__7_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_0__8_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_0__9_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000C); // AfitBaseVals_0__10_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010); // AfitBaseVals_0__11_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01E6); // AfitBaseVals_0__12_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__13_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0070); // AfitBaseVals_0__14_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01FF); // AfitBaseVals_0__15_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0144); // AfitBaseVals_0__16_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000F); // AfitBaseVals_0__17_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A); // AfitBaseVals_0__18_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0073); // AfitBaseVals_0__19_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0087); // AfitBaseVals_0__20_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_0__21_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A); // AfitBaseVals_0__22_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_0__23_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_0__24_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_0__25_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A); // AfitBaseVals_0__26_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_0__27_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046); // AfitBaseVals_0__28_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2B32); // AfitBaseVals_0__29_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0601); // AfitBaseVals_0__30_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__31_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__32_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__33_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00FF); // AfitBaseVals_0__34_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x07FF); // AfitBaseVals_0__35_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFF); // AfitBaseVals_0__36_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__37_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x050D); // AfitBaseVals_0__38_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E80); // AfitBaseVals_0__39_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__40_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1408); // AfitBaseVals_0__41_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0214); // AfitBaseVals_0__42_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF01); // AfitBaseVals_0__43_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x180F); // AfitBaseVals_0__44_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001); // AfitBaseVals_0__45_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__46_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8003); // AfitBaseVals_0__47_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0094); // AfitBaseVals_0__48_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0580); // AfitBaseVals_0__49_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0280); // AfitBaseVals_0__50_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0308); // AfitBaseVals_0__51_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3186); // AfitBaseVals_0__52_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x5260); // AfitBaseVals_0__53_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A02); // AfitBaseVals_0__54_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080A); // AfitBaseVals_0__55_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0500); // AfitBaseVals_0__56_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x032D); // AfitBaseVals_0__57_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x324E); // AfitBaseVals_0__58_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_0__59_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0200); // AfitBaseVals_0__60_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_0__61_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_0__62_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x9696); // AfitBaseVals_0__63_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4646); // AfitBaseVals_0__64_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0802); // AfitBaseVals_0__65_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0802); // AfitBaseVals_0__66_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__67_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_0__68_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3202); // AfitBaseVals_0__69_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_0__70_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_0__71_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_0__72_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_0__73_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x9696); // AfitBaseVals_0__74_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x4646); // AfitBaseVals_0__75_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0802); // AfitBaseVals_0__76_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0802); // AfitBaseVals_0__77_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_0__78_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_0__79_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3202); // AfitBaseVals_0__80_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_0__81_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_0__82_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003); // AfitBaseVals_0__83_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__0_  Brightness[1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__1_  Contrast[1] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__2_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__3_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__4_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x006A); // AfitBaseVals_1__5_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012C); // AfitBaseVals_1__6_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF); // AfitBaseVals_1__7_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_1__8_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_1__9_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000C); // AfitBaseVals_1__10_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010); // AfitBaseVals_1__11_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01E6); // AfitBaseVals_1__12_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF); // AfitBaseVals_1__13_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0070); // AfitBaseVals_1__14_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007D); // AfitBaseVals_1__15_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_1__16_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_1__17_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A); // AfitBaseVals_1__18_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0073); // AfitBaseVals_1__19_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0087); // AfitBaseVals_1__20_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_1__21_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A); // AfitBaseVals_1__22_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_1__23_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_1__24_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_1__25_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000A); // AfitBaseVals_1__26_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_1__27_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_1__28_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2B32); // AfitBaseVals_1__29_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0601); // AfitBaseVals_1__30_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__31_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__32_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__33_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00FF); // AfitBaseVals_1__34_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x07FF); // AfitBaseVals_1__35_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFF); // AfitBaseVals_1__36_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__37_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x050D); // AfitBaseVals_1__38_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E80); // AfitBaseVals_1__39_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__40_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1408); // AfitBaseVals_1__41_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0214); // AfitBaseVals_1__42_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF01); // AfitBaseVals_1__43_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x180F); // AfitBaseVals_1__44_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002); // AfitBaseVals_1__45_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__46_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8003); // AfitBaseVals_1__47_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080); // AfitBaseVals_1__48_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080); // AfitBaseVals_1__49_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0280); // AfitBaseVals_1__50_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0308); // AfitBaseVals_1__51_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E65); // AfitBaseVals_1__52_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1A24); // AfitBaseVals_1__53_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A03); // AfitBaseVals_1__54_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080A); // AfitBaseVals_1__55_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0500); // AfitBaseVals_1__56_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x032D); // AfitBaseVals_1__57_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x324D); // AfitBaseVals_1__58_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_1__59_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0200); // AfitBaseVals_1__60_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_1__61_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_1__62_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x9696); // AfitBaseVals_1__63_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2F34); // AfitBaseVals_1__64_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0504); // AfitBaseVals_1__65_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080F); // AfitBaseVals_1__66_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__67_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_1__68_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3208); // AfitBaseVals_1__69_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_1__70_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_1__71_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_1__72_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_1__73_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x9696); // AfitBaseVals_1__74_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1414); // AfitBaseVals_1__75_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0504); // AfitBaseVals_1__76_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080F); // AfitBaseVals_1__77_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_1__78_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_1__79_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3208); // AfitBaseVals_1__80_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_1__81_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_1__82_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003); // AfitBaseVals_1__83_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__0_  Brightness[2]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__1_  Contrast[2] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__2_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__3_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__4_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_2__5_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012C); // AfitBaseVals_2__6_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF); // AfitBaseVals_2__7_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_2__8_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_2__9_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000C); // AfitBaseVals_2__10_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010); // AfitBaseVals_2__11_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01E6); // AfitBaseVals_2__12_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF); // AfitBaseVals_2__13_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0070); // AfitBaseVals_2__14_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007D); // AfitBaseVals_2__15_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_2__16_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0096); // AfitBaseVals_2__17_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0096); // AfitBaseVals_2__18_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0073); // AfitBaseVals_2__19_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0087); // AfitBaseVals_2__20_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_2__21_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0019); // AfitBaseVals_2__22_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_2__23_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_2__24_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_2__25_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0019); // AfitBaseVals_2__26_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_2__27_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_2__28_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2B32); // AfitBaseVals_2__29_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0601); // AfitBaseVals_2__30_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__31_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__32_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__33_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00FF); // AfitBaseVals_2__34_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x07FF); // AfitBaseVals_2__35_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFF); // AfitBaseVals_2__36_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__37_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x050D); // AfitBaseVals_2__38_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E80); // AfitBaseVals_2__39_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__40_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A08); // AfitBaseVals_2__41_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0200); // AfitBaseVals_2__42_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF01); // AfitBaseVals_2__43_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x180F); // AfitBaseVals_2__44_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002); // AfitBaseVals_2__45_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__46_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8003); // AfitBaseVals_2__47_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080); // AfitBaseVals_2__48_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080); // AfitBaseVals_2__49_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0280); // AfitBaseVals_2__50_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0208); // AfitBaseVals_2__51_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E4B); // AfitBaseVals_2__52_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1A24); // AfitBaseVals_2__53_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A05); // AfitBaseVals_2__54_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080A); // AfitBaseVals_2__55_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0500); // AfitBaseVals_2__56_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x032D); // AfitBaseVals_2__57_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x324D); // AfitBaseVals_2__58_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_2__59_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0200); // AfitBaseVals_2__60_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_2__61_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_2__62_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x9696); // AfitBaseVals_2__63_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E23); // AfitBaseVals_2__64_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0505); // AfitBaseVals_2__65_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080F); // AfitBaseVals_2__66_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__67_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_2__68_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3208); // AfitBaseVals_2__69_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_2__70_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_2__71_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_2__72_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_2__73_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x9696); // AfitBaseVals_2__74_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E23); // AfitBaseVals_2__75_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0505); // AfitBaseVals_2__76_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080F); // AfitBaseVals_2__77_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_2__78_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_2__79_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3208); // AfitBaseVals_2__80_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_2__81_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_2__82_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003); // AfitBaseVals_2__83_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__0_  Brightness[3] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0018); // 0000 AfitBaseVals_3__1_  Contrast[3]
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__2_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__3_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__4_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_3__5_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012C); // AfitBaseVals_3__6_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF); // AfitBaseVals_3__7_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_3__8_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_3__9_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000C); // AfitBaseVals_3__10_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010); // AfitBaseVals_3__11_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01E6); // AfitBaseVals_3__12_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__13_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0070); // AfitBaseVals_3__14_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x007D); // AfitBaseVals_3__15_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_3__16_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0096); // AfitBaseVals_3__17_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0096); // AfitBaseVals_3__18_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0073); // AfitBaseVals_3__19_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x009F); // AfitBaseVals_3__20_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028); // AfitBaseVals_3__21_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028); // AfitBaseVals_3__22_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_3__23_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0037); // AfitBaseVals_3__24_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028); // AfitBaseVals_3__25_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028); // AfitBaseVals_3__26_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_3__27_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0037); // AfitBaseVals_3__28_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2B32); // AfitBaseVals_3__29_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0601); // AfitBaseVals_3__30_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__31_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__32_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__33_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00FF); // AfitBaseVals_3__34_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x07A0); // AfitBaseVals_3__35_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFF); // AfitBaseVals_3__36_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__37_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x050D); // AfitBaseVals_3__38_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E80); // AfitBaseVals_3__39_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__40_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A08); // AfitBaseVals_3__41_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0200); // AfitBaseVals_3__42_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF01); // AfitBaseVals_3__43_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x180F); // AfitBaseVals_3__44_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001); // AfitBaseVals_3__45_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__46_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8003); // AfitBaseVals_3__47_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080); // AfitBaseVals_3__48_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080); // AfitBaseVals_3__49_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0280); // AfitBaseVals_3__50_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0108); // AfitBaseVals_3__51_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E32); // AfitBaseVals_3__52_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1A24); // AfitBaseVals_3__53_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A05); // AfitBaseVals_3__54_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080A); // AfitBaseVals_3__55_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__56_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0328); // AfitBaseVals_3__57_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x324C); // AfitBaseVals_3__58_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_3__59_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0200); // AfitBaseVals_3__60_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_3__61_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_3__62_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x9696); // AfitBaseVals_3__63_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F0F); // AfitBaseVals_3__64_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0307); // AfitBaseVals_3__65_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080F); // AfitBaseVals_3__66_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__67_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_3__68_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3208); // AfitBaseVals_3__69_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_3__70_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_3__71_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_3__72_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_3__73_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x9696); // AfitBaseVals_3__74_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F0F); // AfitBaseVals_3__75_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0307); // AfitBaseVals_3__76_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080F); // AfitBaseVals_3__77_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_3__78_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_3__79_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3208); // AfitBaseVals_3__80_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_3__81_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_3__82_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003); // AfitBaseVals_3__83_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__0_  Brightness[4] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // 0000 AfitBaseVals_4__1_  Contrast[4] 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__2_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__3_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__4_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028); // AfitBaseVals_4__5_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x012C); // AfitBaseVals_4__6_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03FF); // AfitBaseVals_4__7_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0014); // AfitBaseVals_4__8_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0064); // AfitBaseVals_4__9_  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x000C); // AfitBaseVals_4__10_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0010); // AfitBaseVals_4__11_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x01E6); // AfitBaseVals_4__12_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__13_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0070); // AfitBaseVals_4__14_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0087); // AfitBaseVals_4__15_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0073); // AfitBaseVals_4__16_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0096); // AfitBaseVals_4__17_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0096); // AfitBaseVals_4__18_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0073); // AfitBaseVals_4__19_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00B4); // AfitBaseVals_4__20_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028); // AfitBaseVals_4__21_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028); // AfitBaseVals_4__22_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_4__23_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046); // AfitBaseVals_4__24_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028); // AfitBaseVals_4__25_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0028); // AfitBaseVals_4__26_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0023); // AfitBaseVals_4__27_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0046); // AfitBaseVals_4__28_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x2B23); // AfitBaseVals_4__29_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0601); // AfitBaseVals_4__30_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__31_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__32_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__33_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x00FF); // AfitBaseVals_4__34_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0B84); // AfitBaseVals_4__35_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFFFF); // AfitBaseVals_4__36_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__37_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x050D); // AfitBaseVals_4__38_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E80); // AfitBaseVals_4__39_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__40_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A08); // AfitBaseVals_4__41_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0200); // AfitBaseVals_4__42_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFF01); // AfitBaseVals_4__43_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x180F); // AfitBaseVals_4__44_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001); // AfitBaseVals_4__45_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__46_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x8003); // AfitBaseVals_4__47_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080); // AfitBaseVals_4__48_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0080); // AfitBaseVals_4__49_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0280); // AfitBaseVals_4__50_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0108); // AfitBaseVals_4__51_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1E1E); // AfitBaseVals_4__52_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x1419); // AfitBaseVals_4__53_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0A0A); // AfitBaseVals_4__54_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0800); // AfitBaseVals_4__55_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__56_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0328); // AfitBaseVals_4__57_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x324C); // AfitBaseVals_4__58_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x001E); // AfitBaseVals_4__59_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0200); // AfitBaseVals_4__60_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_4__61_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_4__62_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x6464); // AfitBaseVals_4__63_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F0F); // AfitBaseVals_4__64_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0307); // AfitBaseVals_4__65_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080F); // AfitBaseVals_4__66_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__67_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_4__68_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3208); // AfitBaseVals_4__69_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_4__70_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_4__71_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0103); // AfitBaseVals_4__72_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x010C); // AfitBaseVals_4__73_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x6464); // AfitBaseVals_4__74_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F0F); // AfitBaseVals_4__75_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0307); // AfitBaseVals_4__76_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x080F); // AfitBaseVals_4__77_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // AfitBaseVals_4__78_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x030F); // AfitBaseVals_4__79_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x3208); // AfitBaseVals_4__80_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0F1E); // AfitBaseVals_4__81_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x020F); // AfitBaseVals_4__82_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003); // AfitBaseVals_4__83_ 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x7F5E);	// ConstAfitBaseVals_0_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xFEEE);	// ConstAfitBaseVals_1_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0xD9B7);	// ConstAfitBaseVals_2_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0472);	// ConstAfitBaseVals_3_
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// ConstAfitBaseVals_4_

	//*********************************************************************************
	// 18.JPEG Thumnail Setting                                                         
	//*********************************************************************************

	//*********************************************************************************
	// 19.Input Size Setting                                                          
	//*********************************************************************************

	//*********************************************************************************
	// 20.Preview & Capture Configration Setting                                       
	//*********************************************************************************


	SENSORDB("init set ed\n");

#endif

}

/*****************************************************************************
 * FUNCTION
 *  S5K8AAYX_MIPI_PV_Mode
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/
void S5K8AAYX_MIPI_PV_Mode(void)
{	SENSORDB("PV set st\n");
	// Preview config[0] 1280X960  3~30fps //
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01BE);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0500); //REG_0TC_PCFG_usWidth
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x03C0); //REG_0TC_PCFG_usHeight
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005); //REG_0TC_PCFG_Format 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0062); //REG_0TC_PCFG_PVIMask
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100); //REG_0TC_PCFG_OIFMask 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //REG_0TC_PCFG_uClockInd
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D2); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//REG_0TC_PCFG_usFrTimeType 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	//REG_0TC_PCFG_FrRateQualityType
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);//014D//0000 //REG_0TC_PCFG_usMinFrTimeMsecMult10
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x029A);//03e8 MTK revise to 0D05	//REG_0TC_PCFG_usMaxFrTimeMsecMult10
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01E8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //REG_0TC_PCFG_uPrevMirror
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); //REG_0TC_PCFG_uPCaptureMirror

	//==================================================================================
	// 21.Select Cofigration Display
	//==================================================================================
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01A8);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);// REG_TC_GP_ActivePreviewConfig */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01AA);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);// REG_TC_GP_PreviewConfigChanged */
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x019E);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);// REG_TC_GP_EnablePreview */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);// REG_TC_GP_EnablePreviewChanged */
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028,0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x1000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// Set host interrupt


	S5K8AAYX_MIPI_write_cmos_sensor(0x0028,0xD000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x1000,0x0001);
	mdelay(200);

	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002a, 0x019A);	
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100); //EV 0
	SENSORDB("PV set ed\n");
}



/*****************************************************************************
 * FUNCTION
 *  S5K8AAYX_MIPI_CAP_Mode
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/

void S5K8AAYX_MIPI_CAP_Mode(void)
{
//
}

static void S5K8AAYX_MIPI_set_AE_mode(kal_bool AE_enable)
{
     if(AE_enable==KAL_TRUE)
     {
               S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000); // Set page 
               S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000); // Set address  

               S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x214A); 
               S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
     }
     else
     {
               S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000); // Set page 
               S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000); // Set address  

               S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x214A); 
               S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0000);
     }

}


/*************************************************************************
* FUNCTION
*	S5K8AAYX_MIPI_night_mode
*
* DESCRIPTION
*	This function night mode of S5K8AAYX_MIPI.
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
void S5K8AAYX_MIPI_night_mode(kal_bool enable)
{
    //if(S5K8AAYX_MIPICurrentStatus.iNightMode == enable)
    //    return;

	if (S5K8AAYX_MIPI_sensor_cap_state == KAL_TRUE)
		return ;	

	//S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000); // Set page 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000); // Set address	
	if (enable)
	{
		if (S5K8AAYX_MIPI_MPEG4_encode_mode == KAL_TRUE)
		{
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D2); 
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	//REG_0TC_PCFG_usFrTimeType 
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	//REG_0TC_PCFG_FrRateQualityType
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D6);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_VID_NIT_FIX_FR_TIME); //REG_0TC_PCFG_usMinFrTimeMsecMult10
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_VID_NIT_FIX_FR_TIME); //REG_0TC_PCFG_usMaxFrTimeMsecMult10 fps
		}
		else
		{
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D2); 
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//REG_0TC_PCFG_usFrTimeType
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	//REG_0TC_PCFG_FrRateQualityType
			
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D6);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_CAM_NIT_MIN_FR_TIME); //REG_0TC_PCFG_usMinFrTimeMsecMult10
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_CAM_NIT_MAX_FR_TIME);	//REG_0TC_PCFG_usMaxFrTimeMsecMult10
		}	
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0468); 
        S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0140);  //lt_uMaxDigGain
	}
	else
	{
		if (S5K8AAYX_MIPI_MPEG4_encode_mode == KAL_TRUE)
		{
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D2); 
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	//REG_0TC_PCFG_usFrTimeType 
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	//REG_0TC_PCFG_FrRateQualityType
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D6);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, S5K8AAYX_MIPI_VID_NOM_FIX_FR_TIME); //#REG_0TC_PCFG_usMaxFrTimeMsecMult10
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, S5K8AAYX_MIPI_VID_NOM_FIX_FR_TIME); //#REG_0TC_PCFG_usMinFrTimeMsecMult10
		}
		else
		{
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D2); 
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	//REG_0TC_PCFG_usFrTimeType
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	//REG_0TC_PCFG_FrRateQualityType
			
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D6);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_CAM_NOM_MIN_FR_TIME); //REG_0TC_PCFG_usMinFrTimeMsecMult10
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_CAM_NOM_MAX_FR_TIME);	//REG_0TC_PCFG_usMaxFrTimeMsecMult10
		}	

		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x0468); 
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0100);	//lt_uMaxDigGain
	}
	
	// active preview configure
	//============================================================
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01A8); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// #REG_TC_GP_ActivePrevConfig // Select preview configuration_0
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01AC); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_PrevOpenAfterChange
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01A6); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_NewConfigSync // Update preview configuration
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01AA); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_PrevConfigChanged
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x019E); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_EnablePreview // Start preview
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_EnablePreviewChanged

	spin_lock(&s5k8aayxmipi_drv_lock);
    S5K8AAYX_MIPICurrentStatus.iNightMode = enable;
	spin_unlock(&s5k8aayxmipi_drv_lock);
}	/* S5K8AAYX_MIPI_night_mode */

/*************************************************************************
* FUNCTION
*	S5K8AAYX_MIPI_GetSensorID
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
static kal_uint32 S5K8AAYX_MIPI_GetSensorID(kal_uint32 *sensorID)
{
   // volatile signed char i;
	kal_uint16 sensor_id=0;//,sensor_id_2=0;

     // check if sensor ID correct
	 S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0x0000);
	 sensor_id=S5K8AAYX_MIPI_read_cmos_sensor(0x0040);

	 SENSORDB("Sensor Read S5K8AAYX_MIPI ID_use_FCFC %x \r\n",sensor_id);
	 
	 *sensorID=sensor_id;
	if (sensor_id != S5K8AAYX_MIPI_SENSOR_ID)
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
*	S5K8AAYX_MIPIOpen
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
UINT32 S5K8AAYX_MIPIOpen(void)
{
	//volatile signed char i;
	kal_uint16 sensor_id=0;

	 S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0x0000);
	 sensor_id=S5K8AAYX_MIPI_read_cmos_sensor(0x0040);
	 SENSORDB("Sensor Read S5K8AAYX_MIPI ID %x \r\n",sensor_id);

	if (sensor_id != S5K8AAYX_MIPI_SENSOR_ID)
	{
	    SENSORDB("Sensor Read ByeBye \r\n");
		return ERROR_SENSOR_CONNECT_FAIL;
	}

    S5K8AAYX_MIPIInitialPara(); 
	S5K8AAYX_MIPI_Initialize_Setting();
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	S5K8AAYX_MIPIClose
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
UINT32 S5K8AAYX_MIPIClose(void)
{
	return ERROR_NONE;
}	/* S5K8AAYX_MIPIClose() */

/*************************************************************************
* FUNCTION
*	S5K8AAYX_MIPIPreview
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
UINT32 S5K8AAYX_MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	spin_lock(&s5k8aayxmipi_drv_lock);
	S5K8AAYX_MIPI_sensor_cap_state = KAL_FALSE;
	S5K8AAYX_MIPI_PV_dummy_lines = 0;    
	
	if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
	{
		S5K8AAYX_MIPI_MPEG4_encode_mode = KAL_TRUE;		
		S5K8AAYX_MIPI_MJPEG_encode_mode = KAL_FALSE;
	}
	else
	{
		S5K8AAYX_MIPI_MPEG4_encode_mode = KAL_FALSE;		
		S5K8AAYX_MIPI_MJPEG_encode_mode = KAL_FALSE;
		
	}
	spin_unlock(&s5k8aayxmipi_drv_lock);
	
	S5K8AAYX_MIPI_PV_Mode();
	S5K8AAYX_MIPI_night_mode(S5K8AAYX_MIPICurrentStatus.iNightMode);
	S5K8AAYX_MIPI_set_mirror(sensor_config_data->SensorImageMirror);
 
    image_window->ExposureWindowWidth = S5K8AAYX_MIPI_IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight = S5K8AAYX_MIPI_IMAGE_SENSOR_PV_HEIGHT;
	
	// copy sensor_config_data
	memcpy(&S5K8AAYX_MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
  	return ERROR_NONE;
}	/* S5K8AAYX_MIPIPreview() */

UINT32 S5K8AAYX_MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                 MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
     //kal_uint32 pv_integration_time = 0;       // Uinit - us
     //kal_uint32 cap_integration_time = 0;
     //kal_uint16 PV_line_len = 0;
     //kal_uint16 CAP_line_len = 0;

	spin_lock(&s5k8aayxmipi_drv_lock);
	S5K8AAYX_MIPI_sensor_cap_state = KAL_TRUE;
	spin_unlock(&s5k8aayxmipi_drv_lock);
               
    S5K8AAYX_MIPI_CAP_Mode();         

    image_window->GrabStartX = S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_INSERTED_PIXELS;
    image_window->GrabStartY = S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_INSERTED_LINES;
    image_window->ExposureWindowWidth= S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_HEIGHT;

     // copy sensor_config_data
     memcpy(&S5K8AAYX_MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
     return ERROR_NONE;
}        /* S5K8AAYX_MIPICapture() */

UINT32 S5K8AAYX_MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_WIDTH;  //modify by yanxu
	pSensorResolution->SensorFullHeight=S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	pSensorResolution->SensorPreviewWidth=S5K8AAYX_MIPI_IMAGE_SENSOR_PV_WIDTH; 
	pSensorResolution->SensorPreviewHeight=S5K8AAYX_MIPI_IMAGE_SENSOR_PV_HEIGHT;
	
	pSensorResolution->SensorVideoWidth=S5K8AAYX_MIPI_IMAGE_SENSOR_PV_WIDTH; 
	pSensorResolution->SensorVideoHeight=S5K8AAYX_MIPI_IMAGE_SENSOR_PV_HEIGHT;

	return ERROR_NONE;
}	/* S5K8AAYX_MIPIGetResolution() */

UINT32 S5K8AAYX_MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	
    switch(ScenarioId)
    {
    	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 pSensorInfo->SensorPreviewResolutionX=S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			 pSensorInfo->SensorCameraPreviewFrameRate=15;
			 break;
		
		default:
			 pSensorInfo->SensorPreviewResolutionX=S5K8AAYX_MIPI_IMAGE_SENSOR_PV_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=S5K8AAYX_MIPI_IMAGE_SENSOR_PV_HEIGHT;
			 pSensorInfo->SensorCameraPreviewFrameRate=30;
    }
	pSensorInfo->SensorFullResolutionX=S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
	
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;

	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
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

	pSensorInfo->YUVAwbDelayFrame = 3; 
	pSensorInfo->YUVEffectDelayFrame = 2; 
	
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
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 4; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
#endif
		break;

		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX = 2; 
			pSensorInfo->SensorGrabStartY = 2; 					
			#ifdef MIPI_INTERFACE
	            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
	            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
		        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 4; 
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
	memcpy(pSensorConfigData, &S5K8AAYX_MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* S5K8AAYX_MIPIGetInfo() */


UINT32 S5K8AAYX_MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			S5K8AAYX_MIPIPreview(pImageWindow, pSensorConfigData);
		break;
		
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			S5K8AAYX_MIPICapture(pImageWindow, pSensorConfigData);
		break;
		
		//#if defined(MT6575)
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			   S5K8AAYX_MIPICapture(pImageWindow, pSensorConfigData);
			break;
		//#endif
		
        default:
            return ERROR_INVALID_SCENARIO_ID;
	}
	return ERROR_NONE;
}	/* S5K8AAYX_MIPIControl() */

/* [TC] YUV sensor */	
#if WINMO_USE
void S5K8AAYX_MIPIQuery(PMSDK_FEATURE_INFO_STRUCT pSensorFeatureInfo)
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

BOOL S5K8AAYX_MIPI_set_param_wb(UINT16 para)
{

	if(S5K8AAYX_MIPICurrentStatus.iWB == para)
		return TRUE;
	SENSORDB("[Enter]S5K8AAYX_MIPI set_param_wb func:para = %d\n",para);

	S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000); // Set page 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000); // Set address	
	
	switch (para)
	{
		case AWB_MODE_AUTO:
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0408); //bit[3]:AWB Auto:1 menual:0
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x067F);
			break;

		case AWB_MODE_CLOUDY_DAYLIGHT:	
			//======================================================================
			//	MWB : Cloudy_D65										 
			//======================================================================
			S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0408);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0677);							 
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x03DA);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0740);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0400);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0460);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); 
			break;

		case AWB_MODE_DAYLIGHT:
			//==============================================
			//	MWB : sun&daylight_D50						
			//==============================================
			S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0408);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0677);							 
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x03DA);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x05E0);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0400);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0530);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);  
			break;

		case AWB_MODE_INCANDESCENT:
			//==============================================									   
			//	MWB : Incand_Tungsten						
			//==============================================
			S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0408); 
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0677);							
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x03DA); 
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x05C0); //Reg_sf_user_Rgain
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged update
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0400); //Reg_sf_user_Ggain
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x08B0); //Reg_sf_user_Bgain
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged		
			break;

		case AWB_MODE_FLUORESCENT:
			//==================================================================
			//	MWB : Florescent_TL84							  
			//==================================================================
			S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0408);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0677);
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x03DA);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0575);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0400);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0800);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); 
			break;

		case AWB_MODE_TUNGSTEN:	
			S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x0408);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0677);
								 
			S5K8AAYX_MIPI_write_cmos_sensor(0x002A, 0x03DA);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0400);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0400);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0940);
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0001); 
			break;
		default:
			return KAL_FALSE;	

		}
		spin_lock(&s5k8aayxmipi_drv_lock);
	    S5K8AAYX_MIPICurrentStatus.iWB = para;
		spin_unlock(&s5k8aayxmipi_drv_lock);
return TRUE;
}

BOOL S5K8AAYX_MIPI_set_param_effect(UINT16 para)
{
	/*----------------------------------------------------------------*/
	   /* Local Variables												 */
	   /*----------------------------------------------------------------*/
	   kal_uint32 ret = KAL_TRUE;
	
	   /*----------------------------------------------------------------*/
	   /* Code Body 													 */
	   /*----------------------------------------------------------------*/

	   if(S5K8AAYX_MIPICurrentStatus.iEffect == para)
		  return TRUE;
	   SENSORDB("[Enter]s5k8aayxmipi set_param_effect func:para = %d\n",para);

	   S5K8AAYX_MIPI_write_cmos_sensor(0x0028,0x7000);
	   S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x019C);
	   switch (para)
	   {
		   case MEFFECT_OFF:
			   S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000); // Normal, 
			   break;
		   case MEFFECT_MONO:
			   S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001); // Monochrome (Black & White)
			   break;
		   case MEFFECT_SEPIA:
			   S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0004); // Sepia
			   break;
		   case MEFFECT_SEPIABLUE:
			   S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0005); // Aqua (Blue)
			   break;
		   case MEFFECT_NEGATIVE:
			   S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0003); // Negative color
		   default:
			   ret = KAL_FALSE;
	   }
	   
	   spin_lock(&s5k8aayxmipi_drv_lock);
	   S5K8AAYX_MIPICurrentStatus.iEffect = para;
	   spin_unlock(&s5k8aayxmipi_drv_lock);
	   return ret;
} 

void S5K8AAYX_MIPIGetAEAWBLock(UINT32 *pAElockRet32,UINT32 *pAWBlockRet32)
{
    *pAElockRet32 = 1;
	*pAWBlockRet32 = 1;
    SENSORDB("S5K8AAYX_MIPIGetAEAWBLock,AE=%d ,AWB=%d\n,",*pAElockRet32,*pAWBlockRet32);
}


BOOL S5K8AAYX_MIPI_set_param_banding(UINT16 para)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/

#if (defined(S5K8AAYX_MIPI_MANUAL_ANTI_FLICKER))

	if(S5K8AAYX_MIPICurrentStatus.iBanding == para)
      return TRUE;

    switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
				S5K8AAYX_MIPI_write_cmos_sensor(0x0028,0x7000);
				S5K8AAYX_MIPI_write_cmos_sensor(0x002a,0x0408);
				S5K8AAYX_MIPI_write_cmos_sensor(0x0f12,0x065F);
				S5K8AAYX_MIPI_write_cmos_sensor(0x002a,0x03F4);
				S5K8AAYX_MIPI_write_cmos_sensor(0x0f12,0x0001); //REG_SF_USER_FlickerQuant 1:50hz  2:60hz
				S5K8AAYX_MIPI_write_cmos_sensor(0x0f12,0x0001); //REG_SF_USER_FlickerQuantChanged active flicker
			break;
		case AE_FLICKER_MODE_60HZ:
				S5K8AAYX_MIPI_write_cmos_sensor(0x0028,0x7000);
				S5K8AAYX_MIPI_write_cmos_sensor(0x002a,0x0408);
				S5K8AAYX_MIPI_write_cmos_sensor(0x0f12,0x065F);
				S5K8AAYX_MIPI_write_cmos_sensor(0x002a,0x03F4);
				S5K8AAYX_MIPI_write_cmos_sensor(0x0f12,0x0002); //REG_SF_USER_FlickerQuant 1:50hz  2:60hz
				S5K8AAYX_MIPI_write_cmos_sensor(0x0f12,0x0001); //REG_SF_USER_FlickerQuantChanged active flicker
			break;
		default:
			return KAL_FALSE;
	}
	spin_lock(&s5k8aayxmipi_drv_lock);
    S5K8AAYX_MIPICurrentStatus.iBanding = para;
	spin_unlock(&s5k8aayxmipi_drv_lock);
	return TRUE;
	
#else
	/* Auto anti-flicker method is enabled, then nothing need to do in this function.  */

#endif	/* #if (defined(S5K8AAYX_MIPI_MANUAL_ANTI_FLICKER)) */
	return KAL_TRUE;
} /* S5K8AAYX_MIPI_set_param_banding */

BOOL S5K8AAYX_MIPI_set_param_exposure(UINT16 para)
{

	//if(S5K8AAYX_MIPICurrentStatus.iEV == para)
	//	return TRUE;
	
	SENSORDB("[Enter]s5k8aayxmipi set_param_exposure func:para = %d\n",para);
	  
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000);
	S5K8AAYX_MIPI_write_cmos_sensor(0x002a, 0x019A);		
    switch (para)
	{	
		case AE_EV_COMP_n10:
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0080); //EV -1
			break;
		case AE_EV_COMP_00:				   
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0100); //EV 0
			break;
		case AE_EV_COMP_10:					
			S5K8AAYX_MIPI_write_cmos_sensor(0x0F12, 0x0200);  //EV +1
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
	spin_lock(&s5k8aayxmipi_drv_lock);
	S5K8AAYX_MIPICurrentStatus.iEV = para;
	spin_unlock(&s5k8aayxmipi_drv_lock);
	return TRUE;

}/* S5K8AAYX_MIPI_set_param_exposure */


UINT32 S5K8AAYX_MIPIYUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
	switch (iCmd) {
	case FID_SCENE_MODE:
	    if (iPara == SCENE_MODE_OFF)
	    {
	        S5K8AAYX_MIPI_night_mode(0); 
	    }

         else if (iPara == SCENE_MODE_NIGHTSCENE)			
	    {
            S5K8AAYX_MIPI_night_mode(1); 
	    }	    
	    break; 	    
	case FID_AWB_MODE:
         S5K8AAYX_MIPI_set_param_wb(iPara);
	break;
	case FID_COLOR_EFFECT:
    	    
         S5K8AAYX_MIPI_set_param_effect(iPara);
	break;
	case FID_AE_EV:  	    
         S5K8AAYX_MIPI_set_param_exposure(iPara);
	break;
	case FID_AE_FLICKER:
	    	    	    
         S5K8AAYX_MIPI_set_param_banding(iPara);
	break;
    case FID_AE_SCENE_MODE:  
		spin_lock(&s5k8aayxmipi_drv_lock);
      if (iPara == AE_MODE_OFF) {
          S5K8AAYX_MIPI_AE_ENABLE = KAL_FALSE; 
      }
      else {
          S5K8AAYX_MIPI_AE_ENABLE = KAL_TRUE; 
      }
	  spin_unlock(&s5k8aayxmipi_drv_lock);
      S5K8AAYX_MIPI_set_AE_mode(S5K8AAYX_MIPI_AE_ENABLE);
    break; 
	case FID_ZOOM_FACTOR:
		spin_lock(&s5k8aayxmipi_drv_lock);
	    zoom_factor = iPara; 
		spin_unlock(&s5k8aayxmipi_drv_lock);
	break; 
	default:
	break;
	}
	return ERROR_NONE;
}   /* S5K8AAYX_MIPIYUVSensorSetting */


UINT32 S5K8AAYX_MIPIYUVSetVideoMode(UINT16 u2FrameRate)
{

    if(S5K8AAYX_MIPICurrentStatus.iFrameRate == u2FrameRate)
      return ERROR_NONE;

	spin_lock(&s5k8aayxmipi_drv_lock);
    S5K8AAYX_MIPI_VEDIO_encode_mode = KAL_TRUE; 
	S5K8AAYX_MIPI_MPEG4_encode_mode = KAL_TRUE;
	spin_unlock(&s5k8aayxmipi_drv_lock);
	
	//S5K8AAYX_MIPI_write_cmos_sensor(0xFCFC, 0xD000); // Set page 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0028, 0x7000); // Set address	

    if(20<=u2FrameRate && u2FrameRate<=30) //fix 30fps
    {		
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D2); 
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	//REG_0TC_PCFG_usFrTimeType 
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	//REG_0TC_PCFG_FrRateQualityType
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D6);
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_VID_NOM_FIX_FR_TIME); //REG_0TC_PCFG_usMinFrTimeMsecMult10
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_VID_NOM_FIX_FR_TIME); //REG_0TC_PCFG_usMaxFrTimeMsecMult10 fps
    }
    else if(5<=u2FrameRate && u2FrameRate<20 )// fix 15fps
    {
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D2); 
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	//REG_0TC_PCFG_usFrTimeType 
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0002);	//REG_0TC_PCFG_FrRateQualityType
		S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01D6);
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_VID_NIT_FIX_FR_TIME); //REG_0TC_PCFG_usMinFrTimeMsecMult10
		S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,S5K8AAYX_MIPI_VID_NIT_FIX_FR_TIME); //REG_0TC_PCFG_usMaxFrTimeMsecMult10 fps
    }
    else 
    {
        printk("Wrong Frame Rate \n"); 
    }
	
	// active preview configure
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01A8); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0000);	// #REG_TC_GP_ActivePrevConfig // Select preview configuration_0
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01AC); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_PrevOpenAfterChange
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01A6); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_NewConfigSync // Update preview configuration
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x01AA); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_PrevConfigChanged
	S5K8AAYX_MIPI_write_cmos_sensor(0x002A,0x019E); 
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_EnablePreview // Start preview
	S5K8AAYX_MIPI_write_cmos_sensor(0x0F12,0x0001);	// #REG_TC_GP_EnablePreviewChanged
    return ERROR_NONE;
}


kal_uint16 S5K8AAYX_MIPIReadShutter(void)
{
   kal_uint16 temp_msb=0x0000,temp_lsb=0x0000;
   kal_uint32 temp=0x00000000;
   
   
   S5K8AAYX_MIPI_write_cmos_sensor(0x002c,0x7000);
   S5K8AAYX_MIPI_write_cmos_sensor(0x002e,0x16E2);//16bit

   temp_msb=S5K8AAYX_MIPI_read_cmos_sensor(0x0f12);
   temp_msb = (temp_msb<<16)&0xffff0000;

   S5K8AAYX_MIPI_write_cmos_sensor(0x002c,0x7000);
   S5K8AAYX_MIPI_write_cmos_sensor(0x002e,0x16E0);
   temp_lsb=S5K8AAYX_MIPI_read_cmos_sensor(0x0f12);
   temp_lsb = temp_lsb&0x0000ffff;

   temp = (temp_msb|temp_lsb)/400;

   temp = temp*72000/(S5K8AAYX_MIPI_IMAGE_SENSOR_PV_WIDTH*2);
   return temp;
   
}
kal_uint16 S5K8AAYX_MIPIReadGain(void)
{
    kal_uint16 temp=0x0000,Base_gain=64;
	S5K8AAYX_MIPI_write_cmos_sensor(0x002c,0x7000);
    S5K8AAYX_MIPI_write_cmos_sensor(0x002e,0x20D0);//AGain
    
	temp=S5K8AAYX_MIPI_read_cmos_sensor(0x0f12);
	temp = Base_gain*temp/256;

	return temp;
}
kal_uint16 S5K8AAYX_MIPIReadAwbRGain(void)
{
    kal_uint16 temp=0x0000,Base_gain=64;
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002c,0x7000);
    S5K8AAYX_MIPI_write_cmos_sensor(0x002e,0x20DC);

	temp=S5K8AAYX_MIPI_read_cmos_sensor(0x0f12);
	temp = Base_gain*temp/1024;
	return temp;
}
kal_uint16 S5K8AAYX_MIPIReadAwbBGain(void)
{
    kal_uint16 temp=0x0000,Base_gain=64;
	
	S5K8AAYX_MIPI_write_cmos_sensor(0x002c,0x7000);
    S5K8AAYX_MIPI_write_cmos_sensor(0x002e,0x20E0);

	temp=S5K8AAYX_MIPI_read_cmos_sensor(0x0f12);
	temp = Base_gain*temp/1024;

	return temp;
}

//#if defined(MT6575)

/*************************************************************************
* FUNCTION
*    S5K8AAYX_MIPIGetEvAwbRef
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void S5K8AAYX_MIPIGetEvAwbRef(UINT32 pSensorAEAWBRefStruct/*PSENSOR_AE_AWB_REF_STRUCT Ref*/)
{
    PSENSOR_AE_AWB_REF_STRUCT Ref = (PSENSOR_AE_AWB_REF_STRUCT)pSensorAEAWBRefStruct;
    SENSORDB("S5K8AAYX_MIPIGetEvAwbRef  \n" );
    	
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
*    S5K8AAYX_MIPIGetCurAeAwbInfo
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void S5K8AAYX_MIPIGetCurAeAwbInfo(UINT32 pSensorAEAWBCurStruct/*PSENSOR_AE_AWB_CUR_STRUCT Info*/)
{
    PSENSOR_AE_AWB_CUR_STRUCT Info = (PSENSOR_AE_AWB_CUR_STRUCT)pSensorAEAWBCurStruct;
    SENSORDB("S5K8AAYX_MIPIGetCurAeAwbInfo  \n" );

    Info->SensorAECur.AeCurShutter = S5K8AAYX_MIPIReadShutter();
    Info->SensorAECur.AeCurGain = S5K8AAYX_MIPIReadGain() * 2; /* 128 base */
    
    Info->SensorAwbGainCur.AwbCurRgain = S5K8AAYX_MIPIReadAwbRGain()<< 1; /* 128 base */
    
    Info->SensorAwbGainCur.AwbCurBgain = S5K8AAYX_MIPIReadAwbBGain()<< 1; /* 128 base */
}

//#endif

void S5K8AAYX_MIPIGetAFMaxNumFocusAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("S5K8AAYX_MIPIGetAFMaxNumFocusAreas, *pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);

}

void S5K8AAYX_MIPIGetAFMaxNumMeteringAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB("S5K8AAYX_MIPIGetAFMaxNumMeteringAreas,*pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);

}

void S5K8AAYX_MIPIGetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = AE_ISO_100;
    pExifInfo->AWBMode = S5K8AAYX_MIPICurrentStatus.iWB;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = AE_ISO_100;
}

void S5K8AAYX_MIPIGetDelayInfo(UINT32 delayAddr)
{
    SENSOR_DELAY_INFO_STRUCT* pDelayInfo = (SENSOR_DELAY_INFO_STRUCT*)delayAddr;
    pDelayInfo->InitDelay = 3;
    pDelayInfo->EffectDelay = 0;
    pDelayInfo->AwbDelay = 4;
}


UINT32 S5K8AAYXMIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	SENSORDB("S5K8AAYXMIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = S5K8AAYX_MIPI_sensor_pclk/10;
			lineLength = S5K8AAYX_MIPI_SXGA_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - S5K8AAYX_MIPI_SXGA_PERIOD_LINE_NUMS;
			SENSORDB("S5K8AAYXMIPISetMaxFramerateByScenario MSDK_SCENARIO_ID_CAMERA_PREVIEW: lineLength = %d, dummy=%d\n",lineLength,dummyLine);
			S5K8AAYX_MIPI_set_dummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = S5K8AAYX_MIPI_sensor_pclk/10;
			lineLength = S5K8AAYX_MIPI_SXGA_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - S5K8AAYX_MIPI_SXGA_PERIOD_LINE_NUMS;
			SENSORDB("S5K8AAYXMIPISetMaxFramerateByScenario MSDK_SCENARIO_ID_VIDEO_PREVIEW: lineLength = %d, dummy=%d\n",lineLength,dummyLine);			
			S5K8AAYX_MIPI_set_dummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = S5K8AAYX_MIPI_sensor_pclk/10;
			lineLength = S5K8AAYX_MIPI_SXGA_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - S5K8AAYX_MIPI_SXGA_PERIOD_LINE_NUMS;
			SENSORDB("S5K8AAYXMIPISetMaxFramerateByScenario MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG: lineLength = %d, dummy=%d\n",lineLength,dummyLine);			
			S5K8AAYX_MIPI_set_dummy(0, dummyLine);			
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


UINT32 S5K8AAYXMIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
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

UINT32 S5K8AAYX_MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
			*pFeatureReturnPara16++=S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=S5K8AAYX_MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
			//*pFeatureReturnPara16++=S5K8AAYX_MIPI_PV_PERIOD_PIXEL_NUMS+S5K8AAYX_MIPI_PV_dummy_pixels;
			//*pFeatureReturnPara16=S5K8AAYX_MIPI_PV_PERIOD_LINE_NUMS+S5K8AAYX_MIPI_PV_dummy_lines;
			//*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = S5K8AAYX_MIPI_sensor_pclk/10;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			S5K8AAYX_MIPI_night_mode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			S5K8AAYX_MIPI_isp_master_clock=*pFeatureData32;
		break;
		case SENSOR_FEATURE_SET_REGISTER:
			S5K8AAYX_MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = S5K8AAYX_MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &S5K8AAYX_MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
			//S5K8AAYX_MIPIYUVSensorSetting((MSDK_ISP_FEATURE_ENUM)*pFeatureData16, *(pFeatureData16+1));
			S5K8AAYX_MIPIYUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		break;

#if WINMO_USE		
		case SENSOR_FEATURE_QUERY:
			S5K8AAYX_MIPIQuery(pSensorFeatureInfo);
			*pFeatureParaLen = sizeof(MSDK_FEATURE_INFO_STRUCT);
		break;		
		case SENSOR_FEATURE_SET_YUV_CAPTURE_RAW_SUPPORT:
			/* update yuv capture raw support flag by *pFeatureData16 */
		break;
#endif 				
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		    S5K8AAYX_MIPIYUVSetVideoMode(*pFeatureData16);
		    break; 
		//#if defined(MT6575)
		case SENSOR_FEATURE_GET_EV_AWB_REF:
			 S5K8AAYX_MIPIGetEvAwbRef(*pFeatureData32);
				break;
  		case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
			   S5K8AAYX_MIPIGetCurAeAwbInfo(*pFeatureData32);	
			break;
		//#endif
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
            S5K8AAYX_MIPI_GetSensorID(pFeatureData32); 
            break; 	

		case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
			S5K8AAYX_MIPIGetAFMaxNumFocusAreas(pFeatureReturnPara32); 		   
			*pFeatureParaLen=4;
			break;	   
		case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
			S5K8AAYX_MIPIGetAFMaxNumMeteringAreas(pFeatureReturnPara32);			  
			*pFeatureParaLen=4;
			break;
		case SENSOR_FEATURE_GET_EXIF_INFO:
			SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
			SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32); 		 
			S5K8AAYX_MIPIGetExifInfo(*pFeatureData32);
			break;
		case SENSOR_FEATURE_GET_DELAY_INFO:
			SENSORDB("SENSOR_FEATURE_GET_DELAY_INFO\n");
		    S5K8AAYX_MIPIGetDelayInfo(*pFeatureData32);
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			S5K8AAYXMIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			S5K8AAYXMIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
		case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
			S5K8AAYX_MIPIGetAEAWBLock((*pFeatureData32),*(pFeatureData32+1));
			break;
		default:
			break;			
	}
	return ERROR_NONE;
}

SENSOR_FUNCTION_STRUCT	SensorFuncS5K8AAYX_MIPI=
{
	S5K8AAYX_MIPIOpen,
	S5K8AAYX_MIPIGetInfo,
	S5K8AAYX_MIPIGetResolution,
	S5K8AAYX_MIPIFeatureControl,
	S5K8AAYX_MIPIControl,
	S5K8AAYX_MIPIClose
};

UINT32 S5K8AAYX_MIPI_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncS5K8AAYX_MIPI;
	return ERROR_NONE;
}	/* SensorInit() */
