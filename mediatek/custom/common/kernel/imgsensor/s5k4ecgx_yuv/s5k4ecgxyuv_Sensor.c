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
 * 01 04 2012 hao.wang
 * [ALPS00109603] getsensorid func check in
 * .
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#if !defined(MTK_NATIVE_3D_SUPPORT) //2D
	#define S5K4ECGXYUV_MAIN_2_USE_HW_I2C
#else //MTK_NATIVE_3D_SUPPORT
	#define S5K4ECGXYUV_MAIN_2_USE_HW_I2C
 
	#ifdef S5K4ECGXYUV_MAIN_2_USE_HW_I2C
		#define S5K4ECGXYUV_SUPPORT_N3D
	#endif
#endif

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
//Daniel
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h> //copy from user
#include <linux/miscdevice.h>
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"


#include "s5k4ecgxyuv_Sensor.h"
#include "s5k4ecgxyuv_Camera_Sensor_para.h"
#include "s5k4ecgxyuv_CameraCustomized.h"

#define PRE_CLK 80

#define S5K4ECGXYUV_DEBUG
#ifdef S5K4ECGXYUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif
#define Sleep(ms) mdelay(ms)


// global
kal_bool S5K4ECGX_night_mode_enable = KAL_FALSE;
static MSDK_SENSOR_CONFIG_STRUCT S5K4ECGXSensorConfigData;
//
kal_uint8 S5K4ECGXYUV_sensor_write_I2C_address = S5K4ECGX_WRITE_ID;
kal_uint8 S5K4ECGXYUV_sensor_read_I2C_address = S5K4ECGX_READ_ID;
kal_uint8 S5K4ECGXYUV_sensor_socket = DUAL_CAMERA_NONE_SENSOR;


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iMultiWriteReg(u8 *pData, u16 lens, u16 i2cId);

static DEFINE_SPINLOCK(s5k4ecgx_drv_lock);


static kal_uint16 S5K4ECGX_write_cmos_sensor_wID(kal_uint32 addr, kal_uint32 para, kal_uint32 id)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 4,id);
  //      SENSORDB("[Write]:addr=0x%x, para=0x%x, ID=0x%x\r\n", addr, para, id);

}
static kal_uint16 S5K4ECGX_read_cmos_sensor_wID(kal_uint32 addr, kal_uint32 id)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,id);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

static kal_uint16 S5K4ECGX_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,S5K4ECGXYUV_sensor_write_I2C_address);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}
static kal_uint16 S5K4ECGX_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 4,S5K4ECGXYUV_sensor_write_I2C_address);
//        SENSORDB("[Write]:id=0x%x, addr=0x%x, para=0x%x\r\n", S5K4ECGXYUV_sensor_write_I2C_address, addr, para);
}


static void S5K4ECGX_Init_Setting(void)
{
    printk("[4EC] Sensor Init...\n");
                // FOR 4EC EVT1.1
                // ARM Initiation
                
    S5K4ECGX_write_cmos_sensor(0xFCFC, 0xD000);
    S5K4ECGX_write_cmos_sensor(0x0010, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x1030, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0014, 0x0001);
    mdelay(50);
    S5K4ECGX_write_cmos_sensor(0x0028, 0xD000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1082);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0155);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0155);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0055);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05D5);	
    S5K4ECGX_write_cmos_sensor(0x002A, 0x100E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	
    S5K4ECGX_write_cmos_sensor(0x002A, 0x007A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0xE406);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0092);
    S5K4ECGX_write_cmos_sensor(0x002A, 0xE410);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3804);
    S5K4ECGX_write_cmos_sensor(0x002A, 0xE41A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0xE420);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);
    S5K4ECGX_write_cmos_sensor(0x002A, 0xE42E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);
    S5K4ECGX_write_cmos_sensor(0x002A, 0xF400);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5A3C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0023);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8080);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03AF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xAA54);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x464E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0240);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0240);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x55FF);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0202);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0401);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0088);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x009F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1800);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0088);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2428);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03EE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0xF552);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0708);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x080C);
    S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x18BC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05B6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05BA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0007);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05BA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x024E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05B6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05BA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x024F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0075);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00CF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0075);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00F0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x029E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05B2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0228);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0208);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0238);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0218);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0238);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0009);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00DE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05C0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00DF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00E4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01FD);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05B6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05BB);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0077);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x007E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x024F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x025E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09D1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09D5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09D5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0326);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09D1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09D5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0327);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0084);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x008D);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00AA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03AD);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09CD);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02DE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02BE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02EE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02CE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02EE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0009);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0095);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09DB);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0096);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x009B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02B3);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09D1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09D6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0009);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0327);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0336);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1AF8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5A3C);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1896);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x189E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0FB0);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x18AC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05C0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05C0);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1AEA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8080);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1AE0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1A72);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x18A2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1A6A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x009A);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x385E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x024C);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0EE6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1B2A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x008D);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00CF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0084);
    
    //==================================================================================
    //Gas_Anti Shading_Otp.no OTP)
    //==================================================================================
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0722);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);	//skl_OTP_usWaitTime This register should be positioned in fornt of D0001000	
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0726);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	//skl_bUseOTPfunc This is OTP on/off function	
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08D6);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	//ash_bUseOTPData	
    S5K4ECGX_write_cmos_sensor(0x002A, 0x146E);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);	//awbb_otp_disable	
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08DC);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	//ash_bUseGasAlphaOTP	
    
    // TVAR_ash_pGAS_high
    S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0D26);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F0F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F00);
    
    // TVAR_ash_pGAS_low
    S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0DB6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x94C8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE78D);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF985);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2237);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEA86);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x013A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xADE0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0B8F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1276);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD850);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0B55);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x158F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5E3B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF62B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE9A1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2952);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x073E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC818);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xA6F0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFD77);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0DB0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE9D3);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF39F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3BCD);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3CA4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x133B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A3C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF18D);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF760);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF6B3);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF5AE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE8D4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF00F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1834);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x186A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE0AE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xA83C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDFE0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFED4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x20B2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE8B2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0541);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xA78D);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0B62);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1364);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD247);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x17C7);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0B8A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5817);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF488);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEC21);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2E7E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF8AF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD353);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBBCB);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05B2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE7F9);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFE21);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x360B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x247E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0E82);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x14DB);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEF9E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF488);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF513);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x040F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEAA4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE8A7);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1ECC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1084);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE896);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x994F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDFC8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFF6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2643);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD8D2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x11C8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xAB93);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x149D);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x09A7);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD77A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x170D);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0B56);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5E42);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEB77);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF5A5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2301);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x07D8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC746);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB08D);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x06FE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0335);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xED10);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xED93);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x48AB);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2982);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0B93);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1018);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF758);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF8DA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE6DA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03F5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xED1E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xECBF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1528);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x15E8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEBFE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x99D6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE4DE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFD52);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1CBC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF06E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFD9B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xAC4A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0D08);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1285);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD2A1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1426);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1132);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5DDB);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF3CE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE728);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3A5E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xED32);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD64C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB144);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF27);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0B10);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD985);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1209);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2B0C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2A96);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x12A1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x16F9);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEEDF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEDD3);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFC25);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x021C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE89F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE2E5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2854);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0BE7);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE768);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08B4);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);	//wbt_bUseOutdoorASH	
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08BC);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);	//TVAR_ash_AwbAshCord_0_ 2300K	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00DF);	//TVAR_ash_AwbAshCord_1_ 2750K	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);	//TVAR_ash_AwbAshCord_2_ 3300K	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0125);	//TVAR_ash_AwbAshCord_3_ 4150K	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x015F);	//TVAR_ash_AwbAshCord_4_ 5250K	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x017C);	//TVAR_ash_AwbAshCord_5_ 6400K	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0194);	//TVAR_ash_AwbAshCord_6_ 7500K	
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08F6);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3800);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3B00); 	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4300); 	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4300);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    
    //Outdoor Gas Alpha	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4500);  	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08F4);	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);	//ash_bUseGasAlpha
    
    
    
    S5K4ECGX_write_cmos_sensor(0x002A, 0x189E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0FB0);       //#senHal_ExpMinPixels
    S5K4ECGX_write_cmos_sensor(0x002A, 0x18AC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);       //#senHal_uAddColsBin
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);       //#senHal_uAddColsNoBin
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05C0);       //#senHal_uMinColsBin
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05C0);       //#senHal_uMinColsNoBin
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1AEA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8080);       //#senHal_SubF404Tune
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);       //#senHal_FullF404Tune
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1AE0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);       //#senHal_bSenAAC
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1A72);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);       //#senHal_bSRX  //SRX off
    S5K4ECGX_write_cmos_sensor(0x002A, 0x18A2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);       //#senHal_NExpLinesCheckFine     //extend Forbidden area line
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1A6A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x009A);       //#senHal_usForbiddenRightOfs    //extend right Forbidden area line
    S5K4ECGX_write_cmos_sensor(0x002A, 0x385E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x024C);       //#Mon_Sen_uExpPixelsOfs
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0EE6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);       //#setot_bUseDigitalHbin
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1B2A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);  //senHal_TuneStr2_usAngTuneGainTh
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D6);  //senHal_TuneStr2_AngTuneF4CA_0_
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x008D);  //senHal_TuneStr2_AngTuneF4CA_1_
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00CF);  //senHal_TuneStr2_AngTuneF4C2_0_
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0084);  //senHal_TuneStr2_AngTuneF4C2_1_
    
       ///////////////////////////////////////////////////////////////////////////
       
       // OTP setting
    
       ///////////////////////////////////////////////////////////////////////////
    
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0722);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);       //#skl_OTP_usWaitTime  // This register should be positioned in fornt of D0001000. // Check
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0726);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  //skl_bUseOTPfunc OTP shading is used,this reg should be 1  //
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08D6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);       //#ash_bUseOTPData          // If OTP for shading is used, this register should be enable. ( default : disable)
    S5K4ECGX_write_cmos_sensor(0x002A, 0x146E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);       //#awbb_otp_disable         // If OTP for AWB is used, this register should be enable.
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08DC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);       //#ash_bUseGasAlphaOTP          // If OTP alpha for shading is used, this register should be enable.
    
       ///////////////////////////////////////////////////////////////////////////
       
       // TnP setting
    
       ///////////////////////////////////////////////////////////////////////////
    
       // Start of Patch data
    
    S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x3AF8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB570);    // 70003AF8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4B3D);    // 70003AFA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4A3D);    // 70003AFC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x483E);    // 70003AFE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2100);    // 70003B00 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC008);    // 70003B02 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6002);    // 70003B04 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);    // 70003B06 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x493C);    // 70003B08 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x483D);    // 70003B0A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2401);    // 70003B0C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003B0E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFC3B);    // 70003B10 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x493C);    // 70003B12 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x483C);    // 70003B14 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);    // 70003B16 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2502);    // 70003B18 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003B1A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFC35);    // 70003B1C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x483B);    // 70003B1E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0261);    // 70003B20 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8001);    // 70003B22 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2100);    // 70003B24 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8041);    // 70003B26 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4939);    // 70003B28 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x483A);    // 70003B2A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6041);    // 70003B2C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x493A);    // 70003B2E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x483A);    // 70003B30 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2403);    // 70003B32 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);    // 70003B34 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003B36 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFC27);    // 70003B38 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4939);    // 70003B3A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4839);    // 70003B3C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6001);    // 70003B3E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4939);    // 70003B40 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3840);    // 70003B42 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x63C1);    // 70003B44 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4933);    // 70003B46 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4838);    // 70003B48 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3980);    // 70003B4A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6408);    // 70003B4C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4838);    // 70003B4E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4938);    // 70003B50 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6388);    // 70003B52 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4938);    // 70003B54 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4839);    // 70003B56 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);    // 70003B58 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2504);    // 70003B5A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003B5C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFC14);    // 70003B5E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4937);    // 70003B60 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4838);    // 70003B62 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2405);    // 70003B64 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);    // 70003B66 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003B68 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF890);    // 70003B6A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4835);    // 70003B6C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4936);    // 70003B6E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);    // 70003B70 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2506);    // 70003B72 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1D80);    // 70003B74 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003B76 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF889);    // 70003B78 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4832);    // 70003B7A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4933);    // 70003B7C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2407);    // 70003B7E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);    // 70003B80 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x300C);    // 70003B82 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003B84 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF882);    // 70003B86 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482E);    // 70003B88 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4931);    // 70003B8A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);    // 70003B8C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2508);    // 70003B8E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3010);    // 70003B90 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003B92 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF87B);    // 70003B94 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x492F);    // 70003B96 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482F);    // 70003B98 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2409);    // 70003B9A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);    // 70003B9C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003B9E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFBF3);    // 70003BA0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x492E);    // 70003BA2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482E);    // 70003BA4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);    // 70003BA6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x250A);    // 70003BA8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003BAA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFBED);    // 70003BAC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x492D);    // 70003BAE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482D);    // 70003BB0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x240B);    // 70003BB2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);    // 70003BB4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003BB6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFBE7);    // 70003BB8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x492C);    // 70003BBA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482C);    // 70003BBC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x250C);    // 70003BBE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);    // 70003BC0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003BC2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFBE1);    // 70003BC4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x492B);    // 70003BC6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482B);    // 70003BC8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);    // 70003BCA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x240D);    // 70003BCC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003BCE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFBDB);    // 70003BD0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x492A);    // 70003BD2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482A);    // 70003BD4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x250E);    // 70003BD6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);    // 70003BD8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003BDA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFBD5);    // 70003BDC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4929);    // 70003BDE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4829);    // 70003BE0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);    // 70003BE2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003BE4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFBD0);    // 70003BE6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC70);    // 70003BE8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC08);    // 70003BEA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4718);    // 70003BEC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003BEE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0184);    // 70003BF0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4EC2);    // 70003BF2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03FF);    // 70003BF4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);    // 70003BF6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1F90);    // 70003BF8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003BFA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3CA5);    // 70003BFC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003BFE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE38B);    // 70003C00 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C02 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3CDD);    // 70003C04 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C06 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC3B1);    // 70003C08 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C0A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4780);    // 70003C0C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C0E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3D3B);    // 70003C10 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C12 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);    // 70003C14 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C16 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3D77);    // 70003C18 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C1A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB49D);    // 70003C1C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C1E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3F9F);    // 70003C20 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C22 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0180);    // 70003C24 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C26 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3E23);    // 70003C28 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C2A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3DD7);    // 70003C2C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C2E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFFF);    // 70003C30 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00FF);    // 70003C32 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x17E0);    // 70003C34 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C36 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3FBD);    // 70003C38 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C3A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x053D);    // 70003C3C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C3E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C40 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A89);    // 70003C42 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6CD2);    // 70003C44 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C46 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02C9);    // 70003C48 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C4A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C4C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A9A);    // 70003C4E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C50 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02D2);    // 70003C52 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x400B);    // 70003C54 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C56 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9E65);    // 70003C58 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C5A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x40DD);    // 70003C5C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C5E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7C49);    // 70003C60 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C62 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x40F9);    // 70003C64 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C66 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7C63);    // 70003C68 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C6A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4115);    // 70003C6C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C6E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8F01);    // 70003C70 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C72 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x41B7);    // 70003C74 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C76 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7F3F);    // 70003C78 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C7A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4245);    // 70003C7C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C7E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x98C5);    // 70003C80 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C82 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x42FB);    // 70003C84 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70003C86 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xCD59);    // 70003C88 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70003C8A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB570);    // 70003C8C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000C);    // 70003C8E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0015);    // 70003C90 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0029);    // 70003C92 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003C94 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFB80);    // 70003C96 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x49F9);    // 70003C98 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A8);    // 70003C9A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x500C);    // 70003C9C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC70);    // 70003C9E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC08);    // 70003CA0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4718);    // 70003CA2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6808);    // 70003CA4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 70003CA6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70003CA8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6849);    // 70003CAA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0409);    // 70003CAC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C09);    // 70003CAE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4AF4);    // 70003CB0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8992);    // 70003CB2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2A00);    // 70003CB4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD00D);    // 70003CB6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2300);    // 70003CB8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A89);    // 70003CBA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD400);    // 70003CBC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);    // 70003CBE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0419);    // 70003CC0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C09);    // 70003CC2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x23FF);    // 70003CC4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x33C1);    // 70003CC6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1810);    // 70003CC8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4298);    // 70003CCA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD800);    // 70003CCC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);    // 70003CCE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0418);    // 70003CD0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70003CD2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4AEC);    // 70003CD4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8150);    // 70003CD6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8191);    // 70003CD8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4770);    // 70003CDA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB5F3);    // 70003CDC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);    // 70003CDE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB081);    // 70003CE0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9802);    // 70003CE2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6800);    // 70003CE4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0600);    // 70003CE6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0E00);    // 70003CE8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2201);    // 70003CEA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0015);    // 70003CEC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0021);    // 70003CEE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3910);    // 70003CF0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x408A);    // 70003CF2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x40A5);    // 70003CF4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4FE5);    // 70003CF6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0016);    // 70003CF8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2C10);    // 70003CFA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDA03);    // 70003CFC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8839);    // 70003CFE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x43A9);    // 70003D00 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8039);    // 70003D02 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE002);    // 70003D04 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8879);    // 70003D06 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x43B1);    // 70003D08 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8079);    // 70003D0A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003D0C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFB4C);    // 70003D0E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2C10);    // 70003D10 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDA03);    // 70003D12 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8839);    // 70003D14 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4329);    // 70003D16 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8039);    // 70003D18 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE002);    // 70003D1A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8879);    // 70003D1C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4331);    // 70003D1E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8079);    // 70003D20 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x49DB);    // 70003D22 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8809);    // 70003D24 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2900);    // 70003D26 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD102);    // 70003D28 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003D2A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFB45);    // 70003D2C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2000);    // 70003D2E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9902);    // 70003D30 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6008);    // 70003D32 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBCFE);    // 70003D34 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC08);    // 70003D36 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4718);    // 70003D38 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB538);    // 70003D3A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9C04);    // 70003D3C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0015);    // 70003D3E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);    // 70003D40 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9400);    // 70003D42 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003D44 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFB40);    // 70003D46 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4AD2);    // 70003D48 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8811);    // 70003D4A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2900);    // 70003D4C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD00F);    // 70003D4E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8820);    // 70003D50 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4281);    // 70003D52 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD20C);    // 70003D54 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8861);    // 70003D56 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8853);    // 70003D58 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4299);    // 70003D5A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD200);    // 70003D5C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1E40);    // 70003D5E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 70003D60 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70003D62 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8020);    // 70003D64 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8851);    // 70003D66 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8061);    // 70003D68 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4368);    // 70003D6A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1840);    // 70003D6C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6060);    // 70003D6E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC38);    // 70003D70 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC08);    // 70003D72 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4718);    // 70003D74 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB5F8);    // 70003D76 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);    // 70003D78 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6808);    // 70003D7A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 70003D7C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70003D7E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2201);    // 70003D80 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0015);    // 70003D82 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0021);    // 70003D84 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3910);    // 70003D86 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x408A);    // 70003D88 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x40A5);    // 70003D8A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4FBF);    // 70003D8C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0016);    // 70003D8E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2C10);    // 70003D90 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDA03);    // 70003D92 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8839);    // 70003D94 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x43A9);    // 70003D96 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8039);    // 70003D98 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE002);    // 70003D9A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8879);    // 70003D9C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x43B1);    // 70003D9E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8079);    // 70003DA0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003DA2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFB19);    // 70003DA4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2C10);    // 70003DA6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDA03);    // 70003DA8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8838);    // 70003DAA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4328);    // 70003DAC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8038);    // 70003DAE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE002);    // 70003DB0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8878);    // 70003DB2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4330);    // 70003DB4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8078);    // 70003DB6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x48B7);    // 70003DB8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8800);    // 70003DBA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 70003DBC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD507);    // 70003DBE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4BB6);    // 70003DC0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7819);    // 70003DC2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4AB6);    // 70003DC4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7810);    // 70003DC6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7018);    // 70003DC8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7011);    // 70003DCA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x49B5);    // 70003DCC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8188);    // 70003DCE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBCF8);    // 70003DD0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC08);    // 70003DD2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4718);    // 70003DD4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB538);    // 70003DD6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x48B3);    // 70003DD8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4669);    // 70003DDA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003DDC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFB04);    // 70003DDE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x48B2);    // 70003DE0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x49B1);    // 70003DE2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x69C2);    // 70003DE4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2400);    // 70003DE6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x31A8);    // 70003DE8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2A00);    // 70003DEA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD008);    // 70003DEC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x61C4);    // 70003DEE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x684A);    // 70003DF0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6242);    // 70003DF2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6282);    // 70003DF4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x466B);    // 70003DF6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x881A);    // 70003DF8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6302);    // 70003DFA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x885A);    // 70003DFC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6342);    // 70003DFE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6A02);    // 70003E00 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2A00);    // 70003E02 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD00A);    // 70003E04 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6204);    // 70003E06 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6849);    // 70003E08 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6281);    // 70003E0A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x466B);    // 70003E0C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8819);    // 70003E0E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6301);    // 70003E10 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8859);    // 70003E12 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6341);    // 70003E14 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x49A6);    // 70003E16 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88C9);    // 70003E18 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x63C1);    // 70003E1A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003E1C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFAEC);    // 70003E1E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE7A6);    // 70003E20 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB5F0);    // 70003E22 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB08B);    // 70003E24 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x20FF);    // 70003E26 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1C40);    // 70003E28 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x49A2);    // 70003E2A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x89CC);    // 70003E2C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4E9F);    // 70003E2E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6AB1);    // 70003E30 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4284);    // 70003E32 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD101);    // 70003E34 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x48A0);    // 70003E36 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6081);    // 70003E38 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6A70);    // 70003E3A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200);    // 70003E3C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003E3E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFAE3);    // 70003E40 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 70003E42 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70003E44 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4A97);    // 70003E46 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8A11);    // 70003E48 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9109);    // 70003E4A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2101);    // 70003E4C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0349);    // 70003E4E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4288);    // 70003E50 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD200);    // 70003E52 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);    // 70003E54 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4A93);    // 70003E56 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8211);    // 70003E58 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4D98);    // 70003E5A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8829);    // 70003E5C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9108);    // 70003E5E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4A8C);    // 70003E60 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2303);    // 70003E62 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3222);    // 70003E64 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1F91);    // 70003E66 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003E68 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFAD4);    // 70003E6A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8028);    // 70003E6C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x488F);    // 70003E6E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4988);    // 70003E70 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6BC2);    // 70003E72 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6AC0);    // 70003E74 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4282);    // 70003E76 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD201);    // 70003E78 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8CC8);    // 70003E7A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8028);    // 70003E7C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88E8);    // 70003E7E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9007);    // 70003E80 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2240);    // 70003E82 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4310);    // 70003E84 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x80E8);    // 70003E86 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2000);    // 70003E88 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0041);    // 70003E8A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x194B);    // 70003E8C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x001E);    // 70003E8E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3680);    // 70003E90 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8BB2);    // 70003E92 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xAF04);    // 70003E94 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x527A);    // 70003E96 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4A7E);    // 70003E98 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x188A);    // 70003E9A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8897);    // 70003E9C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x83B7);    // 70003E9E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x33A0);    // 70003EA0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x891F);    // 70003EA2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xAE01);    // 70003EA4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5277);    // 70003EA6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8A11);    // 70003EA8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8119);    // 70003EAA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1C40);    // 70003EAC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 70003EAE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70003EB0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2806);    // 70003EB2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD3E9);    // 70003EB4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003EB6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFAB5);    // 70003EB8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003EBA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFABB);    // 70003EBC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4F7A);    // 70003EBE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x37A8);    // 70003EC0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2800);    // 70003EC2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD10A);    // 70003EC4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1FE0);    // 70003EC6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x38FD);    // 70003EC8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD001);    // 70003ECA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1CC0);    // 70003ECC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD105);    // 70003ECE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4875);    // 70003ED0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8829);    // 70003ED2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3818);    // 70003ED4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6840);    // 70003ED6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4348);    // 70003ED8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6078);    // 70003EDA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4973);    // 70003EDC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6878);    // 70003EDE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6B89);    // 70003EE0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4288);    // 70003EE2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD300);    // 70003EE4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);    // 70003EE6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6078);    // 70003EE8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2000);    // 70003EEA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0041);    // 70003EEC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xAA04);    // 70003EEE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5A53);    // 70003EF0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x194A);    // 70003EF2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x269C);    // 70003EF4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x52B3);    // 70003EF6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xAB01);    // 70003EF8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5A59);    // 70003EFA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x32A0);    // 70003EFC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8111);    // 70003EFE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1C40);    // 70003F00 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 70003F02 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70003F04 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2806);    // 70003F06 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD3F0);    // 70003F08 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4966);    // 70003F0A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9809);    // 70003F0C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8208);    // 70003F0E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9808);    // 70003F10 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8028);    // 70003F12 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9807);    // 70003F14 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x80E8);    // 70003F16 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1FE0);    // 70003F18 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x38FD);    // 70003F1A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD13B);    // 70003F1C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4D65);    // 70003F1E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x89E8);    // 70003F20 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1FC1);    // 70003F22 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x39FF);    // 70003F24 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD136);    // 70003F26 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4C60);    // 70003F28 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8AE0);    // 70003F2A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003F2C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA8A);    // 70003F2E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0006);    // 70003F30 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8B20);    // 70003F32 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003F34 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA8E);    // 70003F36 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9000);    // 70003F38 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6AA1);    // 70003F3A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6878);    // 70003F3C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1809);    // 70003F3E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200);    // 70003F40 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003F42 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA61);    // 70003F44 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 70003F46 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70003F48 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);    // 70003F4A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3246);    // 70003F4C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0011);    // 70003F4E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x310A);    // 70003F50 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2305);    // 70003F52 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003F54 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA5E);    // 70003F56 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x66E8);    // 70003F58 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6B23);    // 70003F5A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);    // 70003F5C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0031);    // 70003F5E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0018);    // 70003F60 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003F62 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA7F);    // 70003F64 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x466B);    // 70003F66 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8518);    // 70003F68 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6EEA);    // 70003F6A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6B60);    // 70003F6C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9900);    // 70003F6E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003F70 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA78);    // 70003F72 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x466B);    // 70003F74 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8558);    // 70003F76 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0029);    // 70003F78 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x980A);    // 70003F7A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3170);    // 70003F7C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003F7E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA79);    // 70003F80 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0028);    // 70003F82 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3060);    // 70003F84 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8A02);    // 70003F86 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4947);    // 70003F88 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3128);    // 70003F8A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x808A);    // 70003F8C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8A42);    // 70003F8E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x80CA);    // 70003F90 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8A80);    // 70003F92 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8108);    // 70003F94 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB00B);    // 70003F96 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBCF0);    // 70003F98 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC08);    // 70003F9A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4718);    // 70003F9C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4845);    // 70003F9E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3060);    // 70003FA0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8881);    // 70003FA2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2900);    // 70003FA4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD007);    // 70003FA6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2100);    // 70003FA8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8081);    // 70003FAA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4944);    // 70003FAC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x20FF);    // 70003FAE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1C40);    // 70003FB0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8048);    // 70003FB2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2001);    // 70003FB4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4770);    // 70003FB6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2000);    // 70003FB8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4770);    // 70003FBA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB570);    // 70003FBC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2400);    // 70003FBE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4D40);    // 70003FC0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4841);    // 70003FC2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8881);    // 70003FC4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4841);    // 70003FC6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8041);    // 70003FC8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2101);    // 70003FCA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8001);    // 70003FCC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003FCE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA59);    // 70003FD0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x483D);    // 70003FD2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3820);    // 70003FD4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8BC0);    // 70003FD6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70003FD8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA5C);    // 70003FDA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4B3C);    // 70003FDC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x220D);    // 70003FDE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0712);    // 70003FE0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x18A8);    // 70003FE2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8806);    // 70003FE4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00E1);    // 70003FE6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x18C9);    // 70003FE8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x81CE);    // 70003FEA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8846);    // 70003FEC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x818E);    // 70003FEE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8886);    // 70003FF0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x824E);    // 70003FF2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88C0);    // 70003FF4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8208);    // 70003FF6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3508);    // 70003FF8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x042D);    // 70003FFA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C2D);    // 70003FFC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1C64);    // 70003FFE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0424);    // 70004000 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C24);    // 70004002 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2C07);    // 70004004 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD3EC);    // 70004006 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE649);    // 70004008 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB510);    // 7000400A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482E);    // 7000400C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4C2F);    // 7000400E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88C0);    // 70004010 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8060);    // 70004012 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2001);    // 70004014 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8020);    // 70004016 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482B);    // 70004018 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3820);    // 7000401A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8BC0);    // 7000401C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 7000401E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFA39);    // 70004020 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88E0);    // 70004022 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4A2B);    // 70004024 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2800);    // 70004026 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD003);    // 70004028 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x492B);    // 7000402A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8849);    // 7000402C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2900);    // 7000402E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD009);    // 70004030 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2001);    // 70004032 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03C0);    // 70004034 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8050);    // 70004036 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x80D0);    // 70004038 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2000);    // 7000403A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8090);    // 7000403C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8110);    // 7000403E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC10);    // 70004040 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC08);    // 70004042 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4718);    // 70004044 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8050);    // 70004046 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8920);    // 70004048 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x80D0);    // 7000404A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8960);    // 7000404C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 7000404E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1400);    // 70004050 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8090);    // 70004052 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x89A1);    // 70004054 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0409);    // 70004056 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1409);    // 70004058 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8111);    // 7000405A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x89E3);    // 7000405C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8A24);    // 7000405E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2B00);    // 70004060 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD104);    // 70004062 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x17C3);    // 70004064 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F5B);    // 70004066 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1818);    // 70004068 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x10C0);    // 7000406A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8090);    // 7000406C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2C00);    // 7000406E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD1E6);    // 70004070 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x17C8);    // 70004072 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F40);    // 70004074 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1840);    // 70004076 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x10C0);    // 70004078 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8110);    // 7000407A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE7E0);    // 7000407C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 7000407E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x38D4);    // 70004080 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70004082 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x17D0);    // 70004084 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70004086 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5000);    // 70004088 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD000);    // 7000408A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1100);    // 7000408C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD000);    // 7000408E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x171A);    // 70004090 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70004092 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4780);    // 70004094 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70004096 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2FCA);    // 70004098 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 7000409A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2FC5);    // 7000409C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 7000409E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2FC6);    // 700040A0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040A2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2ED8);    // 700040A4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040A6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2BD0);    // 700040A8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040AA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x17E0);    // 700040AC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040AE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2DE8);    // 700040B0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040B2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x37E0);    // 700040B4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040B6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x210C);    // 700040B8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040BA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1484);    // 700040BC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040BE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC100);    // 700040C0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD000);    // 700040C2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xA006);    // 700040C4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700040C6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0724);    // 700040C8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040CA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xA000);    // 700040CC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD000);    // 700040CE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2270);    // 700040D0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040D2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2558);    // 700040D4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040D6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x146C);    // 700040D8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 700040DA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB510);    // 700040DC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000C);    // 700040DE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x499D);    // 700040E0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2204);    // 700040E2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6820);    // 700040E4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5E8A);    // 700040E6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0140);    // 700040E8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A80);    // 700040EA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0280);    // 700040EC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8849);    // 700040EE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 700040F0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF9D8);    // 700040F2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6020);    // 700040F4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE7A3);    // 700040F6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB510);    // 700040F8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000C);    // 700040FA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4996);    // 700040FC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2208);    // 700040FE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6820);    // 70004100 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5E8A);    // 70004102 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0140);    // 70004104 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A80);    // 70004106 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0280);    // 70004108 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88C9);    // 7000410A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 7000410C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF9CA);    // 7000410E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6020);    // 70004110 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE795);    // 70004112 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB5FE);    // 70004114 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000C);    // 70004116 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6825);    // 70004118 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6866);    // 7000411A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x68A0);    // 7000411C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9001);    // 7000411E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x68E7);    // 70004120 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1BA8);    // 70004122 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x42B5);    // 70004124 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDA00);    // 70004126 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1B70);    // 70004128 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9000);    // 7000412A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x498A);    // 7000412C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x488B);    // 7000412E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x884A);    // 70004130 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8843);    // 70004132 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x435A);    // 70004134 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2304);    // 70004136 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5ECB);    // 70004138 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A92);    // 7000413A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x18D2);    // 7000413C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02D2);    // 7000413E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C12);    // 70004140 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88CB);    // 70004142 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8880);    // 70004144 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4343);    // 70004146 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A98);    // 70004148 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2308);    // 7000414A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5ECB);    // 7000414C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x18C0);    // 7000414E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02C0);    // 70004150 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70004152 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0411);    // 70004154 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 70004156 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1409);    // 70004158 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1400);    // 7000415A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A08);    // 7000415C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x497F);    // 7000415E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x39E0);    // 70004160 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6148);    // 70004162 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9801);    // 70004164 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3040);    // 70004166 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7880);    // 70004168 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2800);    // 7000416A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD103);    // 7000416C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9801);    // 7000416E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0029);    // 70004170 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70004172 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF99D);    // 70004174 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8839);    // 70004176 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9800);    // 70004178 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4281);    // 7000417A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD814);    // 7000417C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8879);    // 7000417E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9800);    // 70004180 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4281);    // 70004182 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD20C);    // 70004184 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9801);    // 70004186 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0029);    // 70004188 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 7000418A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF999);    // 7000418C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9801);    // 7000418E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0029);    // 70004190 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70004192 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF995);    // 70004194 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9801);    // 70004196 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0029);    // 70004198 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 7000419A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF991);    // 7000419C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE003);    // 7000419E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9801);    // 700041A0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0029);    // 700041A2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 700041A4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF98C);    // 700041A6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9801);    // 700041A8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0032);    // 700041AA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0039);    // 700041AC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 700041AE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF98F);    // 700041B0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6020);    // 700041B2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE5BE);    // 700041B4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB57C);    // 700041B6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4869);    // 700041B8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xA901);    // 700041BA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);    // 700041BC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 700041BE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF913);    // 700041C0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x466B);    // 700041C2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88D9);    // 700041C4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8898);    // 700041C6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4B64);    // 700041C8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3346);    // 700041CA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1E9A);    // 700041CC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 700041CE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF987);    // 700041D0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4863);    // 700041D2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4961);    // 700041D4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3812);    // 700041D6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3140);    // 700041D8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8A42);    // 700041DA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x888B);    // 700041DC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x18D2);    // 700041DE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8242);    // 700041E0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8AC2);    // 700041E2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88C9);    // 700041E4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1851);    // 700041E6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x82C1);    // 700041E8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0020);    // 700041EA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4669);    // 700041EC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 700041EE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF8FB);    // 700041F0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x485C);    // 700041F2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x214D);    // 700041F4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8301);    // 700041F6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2196);    // 700041F8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8381);    // 700041FA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x211D);    // 700041FC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3020);    // 700041FE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8001);    // 70004200 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70004202 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF975);    // 70004204 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70004206 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF97B);    // 70004208 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4857);    // 7000420A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4C57);    // 7000420C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6E00);    // 7000420E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x60E0);    // 70004210 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x466B);    // 70004212 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8818);    // 70004214 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8859);    // 70004216 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0025);    // 70004218 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A40);    // 7000421A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3540);    // 7000421C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x61A8);    // 7000421E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x484E);    // 70004220 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9900);    // 70004222 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3060);    // 70004224 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70004226 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF973);    // 70004228 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x466B);    // 7000422A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8819);    // 7000422C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1DE0);    // 7000422E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x30F9);    // 70004230 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8741);    // 70004232 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8859);    // 70004234 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8781);    // 70004236 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2000);    // 70004238 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x71A0);    // 7000423A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x74A8);    // 7000423C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC7C);    // 7000423E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xBC08);    // 70004240 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4718);    // 70004242 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB5F8);    // 70004244 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);    // 70004246 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6808);    // 70004248 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);    // 7000424A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 7000424C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x684A);    // 7000424E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0412);    // 70004250 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C12);    // 70004252 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x688E);    // 70004254 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x68CC);    // 70004256 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x493F);    // 70004258 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x884B);    // 7000425A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4343);    // 7000425C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A98);    // 7000425E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2304);    // 70004260 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5ECB);    // 70004262 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x18C0);    // 70004264 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02C0);    // 70004266 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);    // 70004268 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88CB);    // 7000426A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4353);    // 7000426C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A9A);    // 7000426E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2308);    // 70004270 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5ECB);    // 70004272 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x18D1);    // 70004274 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02C9);    // 70004276 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C09);    // 70004278 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2701);    // 7000427A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x003A);    // 7000427C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x40AA);    // 7000427E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9200);    // 70004280 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);    // 70004282 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3A10);    // 70004284 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4097);    // 70004286 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2D10);    // 70004288 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDA06);    // 7000428A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4A38);    // 7000428C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9B00);    // 7000428E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8812);    // 70004290 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x439A);    // 70004292 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4B36);    // 70004294 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x801A);    // 70004296 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE003);    // 70004298 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4B35);    // 7000429A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x885A);    // 7000429C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x43BA);    // 7000429E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x805A);    // 700042A0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0023);    // 700042A2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0032);    // 700042A4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 700042A6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF91B);    // 700042A8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2D10);    // 700042AA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDA05);    // 700042AC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4930);    // 700042AE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9A00);    // 700042B0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8808);    // 700042B2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4310);    // 700042B4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8008);    // 700042B6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE003);    // 700042B8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x482D);    // 700042BA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8841);    // 700042BC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4339);    // 700042BE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8041);    // 700042C0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4D2A);    // 700042C2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2000);    // 700042C4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3580);    // 700042C6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88AA);    // 700042C8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5E30);    // 700042CA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2100);    // 700042CC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 700042CE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF927);    // 700042D0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8030);    // 700042D2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2000);    // 700042D4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x88AA);    // 700042D6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5E20);    // 700042D8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2100);    // 700042DA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 700042DC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF920);    // 700042DE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8020);    // 700042E0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE575);    // 700042E2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4823);    // 700042E4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2103);    // 700042E6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x81C1);    // 700042E8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4A23);    // 700042EA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2100);    // 700042EC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8011);    // 700042EE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2101);    // 700042F0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8181);    // 700042F2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2102);    // 700042F4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x81C1);    // 700042F6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4770);    // 700042F8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB5F3);    // 700042FA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);    // 700042FC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB081);    // 700042FE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2101);    // 70004300 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000D);    // 70004302 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0020);    // 70004304 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3810);    // 70004306 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4081);    // 70004308 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x40A5);    // 7000430A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4F18);    // 7000430C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000E);    // 7000430E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2C10);    // 70004310 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDA03);    // 70004312 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8838);    // 70004314 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x43A8);    // 70004316 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8038);    // 70004318 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE002);    // 7000431A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8878);    // 7000431C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x43B0);    // 7000431E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8078);    // 70004320 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF000);    // 70004322 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF905);    // 70004324 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4B15);    // 70004326 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7018);    // 70004328 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2C10);    // 7000432A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDA03);    // 7000432C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8838);    // 7000432E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4328);    // 70004330 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8038);    // 70004332 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE002);    // 70004334 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8878);    // 70004336 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4330);    // 70004338 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8078);    // 7000433A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4C10);    // 7000433C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7820);    // 7000433E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2800);    // 70004340 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD005);    // 70004342 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF7FF);    // 70004344 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFCE);    // 70004346 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2000);    // 70004348 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7020);    // 7000434A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x490D);    // 7000434C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7008);    // 7000434E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7818);    // 70004350 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9902);    // 70004352 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7008);    // 70004354 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE4ED);    // 70004356 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2558);    // 70004358 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 7000435A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2AB8);    // 7000435C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 7000435E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x145E);    // 70004360 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70004362 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2698);    // 70004364 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70004366 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2BB8);    // 70004368 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 7000436A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2998);    // 7000436C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 7000436E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1100);    // 70004370 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD000);    // 70004372 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1040);    // 70004374 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD000);    // 70004376 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9100);    // 70004378 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD000);    // 7000437A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3044);    // 7000437C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 7000437E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3898);    // 70004380 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70004382 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3076);    // 70004384 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);    // 70004386 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004388 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 7000438A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 7000438C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000438E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 70004390 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 70004392 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1789);    // 70004394 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);    // 70004396 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004398 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 7000439A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 7000439C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000439E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700043A0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700043A2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x16F1);    // 700043A4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);    // 700043A6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700043A8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700043AA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700043AC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700043AE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700043B0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700043B2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC3B1);    // 700043B4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700043B6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700043B8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700043BA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700043BC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700043BE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700043C0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700043C2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC36D);    // 700043C4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700043C6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700043C8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700043CA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700043CC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700043CE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700043D0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700043D2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF6D7);    // 700043D4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700043D6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700043D8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700043DA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700043DC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700043DE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700043E0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700043E2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xB49D);    // 700043E4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700043E6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700043E8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700043EA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700043EC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700043EE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700043F0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700043F2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7EDF);    // 700043F4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700043F6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700043F8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700043FA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700043FC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700043FE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 70004400 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 70004402 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x448D);    // 70004404 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70004406 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004408 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 7000440A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF004);    // 7000440C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE51F);    // 7000440E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x29EC);    // 70004410 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);    // 70004412 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004414 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004416 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004418 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000441A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 7000441C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000441E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2EF1);    // 70004420 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70004422 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004424 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004426 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004428 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000442A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 7000442C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000442E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEE03);    // 70004430 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70004432 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004434 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004436 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004438 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000443A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 7000443C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000443E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xA58B);    // 70004440 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70004442 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004444 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004446 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004448 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000444A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 7000444C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000444E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7C49);    // 70004450 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70004452 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004454 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004456 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004458 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000445A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 7000445C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000445E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7C63);    // 70004460 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70004462 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004464 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004466 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004468 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000446A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 7000446C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000446E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2DB7);    // 70004470 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70004472 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004474 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004476 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004478 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000447A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 7000447C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000447E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xEB3D);    // 70004480 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70004482 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004484 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004486 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004488 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000448A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 7000448C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000448E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF061);    // 70004490 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 70004492 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004494 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004496 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004498 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 7000449A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 7000449C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000449E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF0EF);    // 700044A0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700044A2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700044A4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700044A6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF004);    // 700044A8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE51F);    // 700044AA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2824);    // 700044AC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);    // 700044AE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700044B0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700044B2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700044B4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700044B6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700044B8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700044BA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8EDD);    // 700044BC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700044BE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700044C0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700044C2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700044C4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700044C6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700044C8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700044CA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8DCB);    // 700044CC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700044CE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700044D0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700044D2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700044D4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700044D6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700044D8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700044DA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8E17);    // 700044DC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700044DE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700044E0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700044E2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700044E4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700044E6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700044E8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700044EA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x98C5);    // 700044EC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700044EE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 700044F0 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 700044F2 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 700044F4 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 700044F6 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 700044F8 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 700044FA 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7C7D);    // 700044FC 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 700044FE 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004500 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004502 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004504 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 70004506 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 70004508 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000450A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7E31);    // 7000450C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 7000450E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004510 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004512 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004514 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 70004516 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 70004518 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000451A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7EAB);    // 7000451C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 7000451E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004520 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004522 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004524 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 70004526 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 70004528 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000452A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7501);    // 7000452C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 7000452E 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4778);    // 70004530 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x46C0);    // 70004532 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC000);    // 70004534 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE59F);    // 70004536 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1C);    // 70004538 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xE12F);    // 7000453A 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xCD59);    // 7000453C 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);    // 7000453E 
           // End of Patch Data(Last : 7000453Eh)
           // Total Size 2632 (0x0A48)
           // Addr : 3AF8 , Size : 2630(A46h) 
    
           //TNP_USER_MBCV_CONTROL
           //TNP_4EC_MBR_TUNE
           //TNP_4EC_FORBIDDEN_TUNE
           //TNP_AF_FINESEARCH_DRIVEBACK
           //TNP_FLASH_ALG
           //TNP_GAS_ALPHA_OTP
           //TNP_AWB_MODUL_COMP
           //TNP_AWB_INIT_QUEUE
           //TNP_AWB_GRID_LOWBR
           //TNP_AWB_GRID_MODULECOMP        
           // TNP_AF_INIT_ON_SEARCH
    
    
    S5K4ECGX_write_cmos_sensor(0x0028, 0xD000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x01FC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x01FE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0204);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0061);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x020C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2F0C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0190);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0294);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00E3);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0238);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0166);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0074);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0132);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x070E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00FF);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x071E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x163C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1648);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9002);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1652);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x15E0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0801);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x164C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x163E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00E5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00CC);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x15D4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x169A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF95);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x166A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0280);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1676);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03A0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0320);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x16BC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0030);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x16E0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x16D4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1656);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x15E6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x003C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0015);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0032);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0038);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x003E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0044);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x004A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0050);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0056);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x005C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0062);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0068);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x006E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0074);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x007A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0086);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x008C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0092);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0098);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x009E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00AA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00B0);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1722);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0006);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3FF0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03E8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0009);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0020);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00E0);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x028C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08B4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08BC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00DF);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0125);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x015F);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x017C);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0194);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08F6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3800);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3B00);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4300);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4300);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4500);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08F4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1492);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0201);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0102);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0202);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0202);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0201);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0302);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0203);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0102);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0201);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0302);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0203);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0102);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0201);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0202);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0202);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0102);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0202);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0202);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1484);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x003C);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x148A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000F);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x058C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3520);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD4c0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3520);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD4c0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x059C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0470);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C00);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1000);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0544);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0111);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00EF);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0F2A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x04E6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x077F);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0F30);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0608);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0800);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A3C);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0D05);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4008);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9C00);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xAD00);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xF1D4);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDC00);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xDC00);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0638);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A3C);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0D05);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3408);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3408);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6810);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8214);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xC350);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD4C0);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xD4c0);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0660);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0650);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x06B8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x452C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x05D0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x145E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0580);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0428);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x07B0);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x11F0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0120);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0121);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x101C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x037C);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x038E);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x033C);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0384);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02FE);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x036C);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02BA);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0352);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x028E);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x026A);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02C8);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0254);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02A8);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0242);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02A0);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x021A);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02A0);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0298);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01D4);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0290);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01CC);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0276);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01D2);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0260);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F6);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x023A);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1070);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000E);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1074);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0126);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1078);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0272);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02A0);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x025A);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02BC);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x024A);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02C0);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x023C);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02BE);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x022E);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02BC);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0224);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02B6);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0218);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02AA);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0210);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02A0);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x020C);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0296);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x020A);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x028C);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0212);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x027E);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0234);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0256);                                       
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x10AC);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000C);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x10B0);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01D8);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x10B4);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0350);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0422);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02C4);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0452);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0278);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x041C);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0230);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03EE);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F0);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0392);              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0340);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0194);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0302);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x016E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02C2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0148);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0286);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x018A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0242);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0006);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x10E8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x10EC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0106);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x10F0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0380);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0168);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2D90);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1464);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0190);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A0);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1228);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x122C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x122A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x120A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x05D5);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x120E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0771);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03A4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0036);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x002A);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1278);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFEF7);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0021);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0AF0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0AF0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x018F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0096);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000E);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1224);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0032);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x001E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x2BA4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0006);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x146C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1434);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02CE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0347);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03C2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x10A0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x10A1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1185);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1186);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x11E5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x11E6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00AB);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00BF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0093);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x13A4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD0);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD0);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD0);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFEC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF66);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFEC);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC4);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC4);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF66);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFEC);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC4);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC4);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF66);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFC0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1208);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0020);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x144E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFE0);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0734);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0016);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0030);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0066);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0138);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0163);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0189);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0222);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0247);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0282);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02B5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x030F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x035F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03A2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03D8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03FF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0016);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0030);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0066);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0138);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0163);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0189);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0222);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0247);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0282);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02B5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x030F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x035F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03A2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03D8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03FF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0016);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0030);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0066);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0138);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0163);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0189);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0222);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0247);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0282);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02B5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x030F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x035F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03A2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03D8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03FF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0019);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0036);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x006F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0135);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x015F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0185);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F3);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0220);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x024A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0291);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02D0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x032A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x036A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x039F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03CC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03F9);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0019);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0036);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x006F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0135);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x015F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0185);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F3);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0220);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x024A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0291);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02D0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x032A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x036A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x039F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03CC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03F9);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0019);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0036);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x006F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0135);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x015F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0185);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C1);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F3);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0220);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x024A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0291);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02D0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x032A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x036A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x039F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03CC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03F9);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08A6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0125);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x015F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x017C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0194);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0898);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4800);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x08A0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x48D8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x4800);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0208);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFB5);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFE8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF20);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01BF);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF53);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFEA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C2);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0095);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFEFD);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0206);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF7F);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0191);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF06);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01BA);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0108);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0208);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFB5); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFE8); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF20); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01BF); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF53); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFEA); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C2); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C6); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0095); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFEFD); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0206); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF7F); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0191); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF06); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01BA); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0108); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0208);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFB5); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFE8); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF20); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01BF); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF53); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0022); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFEA); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01C2); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C6); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0095); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFEFD); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0206); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF7F); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0191); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF06); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01BA); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0108); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0203);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFBD); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFEF1); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x014E); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF18); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFE6); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFDD); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01B2); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C8); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00F0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF49); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0151); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF50); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0147); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF75); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0187); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01BF); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0203);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFBD);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFEF1); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x014E); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF18); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFE6); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFDD); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01B2); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00F0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF49);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0151); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF50); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0147); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF75); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0187); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01BF); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0203); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFBD); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFEF1); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x014E); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF18); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFE6); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFDD); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01B2); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C8); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00F0); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF49); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0151); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF50); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0147); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF75); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0187); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01BF); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01E5); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFA4); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFDC); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFE90); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x013F); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF1B);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFD2);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFDF);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0236);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00EC);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00F8);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF34);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01CE);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF83);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0195);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFEF3);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0126);                              
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0162);                              
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0944);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0050);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00B0);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0196);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0245);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x097A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01CC);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01CC);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01CC);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01CC);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01CC);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0180);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0196);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0976);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0070);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0938);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0014);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D2);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0384);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x07D0);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1388);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x098C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0064);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0384);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x005F);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0070);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1430);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0201);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0204);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3604);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x032A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0403);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1B06);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6015);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0640);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0306);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2003);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x365A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x102A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0600);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5A0F);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0505);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1802);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3028);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0418);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0800);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1804);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0540);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0020);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1800);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1E10);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0405);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0205);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0304);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0409);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0306);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0407);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1C04);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0214);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1002);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0610);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A02);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4A18);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0348);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0180);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A0A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2A36);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6024);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2A36);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFFF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0808);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x010A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3601);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x242A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3660);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF2A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x08FF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0064);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0384);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0051);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0070);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1430);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0201);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0204);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2404);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x031B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0103);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1205);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x400D);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3040);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0630);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0306);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2003);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0404);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x245A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1018);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0B00);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5A0F);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0505);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1802);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3428);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x041C);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0800);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1004);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0540);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0020);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1800);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1E10);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0405);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0205);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0304);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0409);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0306);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0407);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1F04);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0218);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1102);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0611);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A02);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8018);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0380);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0180);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A0A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1B24);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6024);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1D22);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFFF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0808);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x010A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2401);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x241B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1E60);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF18);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x08FF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0064);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0384);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0043);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0070);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1430);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0201);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0204);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1B04);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0312);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C03);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2806);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1580);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2020);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0620);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0306);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2003);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0404);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x145A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1010);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0E00);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5A0F);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0504);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1802);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3828);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0428);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A04);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0540);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0020);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1800);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1E10);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0405);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0207);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0304);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0409);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0306);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0407);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2404);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0221);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1202);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0613);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A02);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8018);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0180);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A0A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x141D);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6024);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C0C);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFFF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0808);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x010A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1B01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2412);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C60);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF0C);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x08FF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0064);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0384);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0032);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0070);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1430);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0201);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0204);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1504);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x030F);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0902);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2004);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0050);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1140);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x201C);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0620);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0306);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2003);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0404);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x145A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1010);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5A0F);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0503);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1802);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3C28);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x042C);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF00);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0904);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0540);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0020);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1800);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1E10);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0405);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0206);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0304);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0409);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0305);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0406);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2804);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0228);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1402);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0618);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A02);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8018);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0180);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A0A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1117);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6024);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A0A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFFF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0808);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x010A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1501);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x240F);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A60);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF0A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x08FF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0064);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0384);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0032);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01F4);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0070);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A0);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1430);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0201);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0204);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F04);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x030C);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0602);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1803);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0E20);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2018);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0620);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0306);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2003);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0404);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x145A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1010);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1200);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x5A0F);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0502);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1802);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4028);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0430);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF00);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0804);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x4008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0540);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8006);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0020);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1800);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1E10);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000B);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0607);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0405);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0205);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0304);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0409);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0306);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0407);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x2C04);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x022C);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1402);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0618);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x1A02);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x8018);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0180);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A0A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0101);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0C0F);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6024);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0808);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFFFF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0808);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x010A);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0F01);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x240C);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0860);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFF08);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x08FF);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0008);   	
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x23CE);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFDC8);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x112E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x93A5);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0xFE67);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    
       //20120521 by Caval
    //	For MCLK=40MHz, PCLK=86MHz
    #if 0
    S5K4ECGX_write_cmos_sensor(0x002A, 0x01F8);   //System Setting
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x9C40);   //REG_TC_IPRM_InClockLSBs MCLK: 40Mhz
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0212);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);   //REG_TC_IPRM_UseNPviClocks
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_TC_IPRM_UseNMipiClocks
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_TC_IPRM_NumberOfMipiLanes
    S5K4ECGX_write_cmos_sensor(0x002A, 0x021A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3A98);   //REG_TC_IPRM_OpClk4KHz_0 SCLK: 60Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x53fc);   //REG_TC_IPRM_MinOutRate4KHz_0	PCLK Min : 88Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x53fc);   //REG_TC_IPRM_MaxOutRate4KHz_0	PCLK Max : 88Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x278D);   //REG_TC_IPRM_OpClk4KHz_1	SCLK 	 : 40.5Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x53fc);   //REG_TC_IPRM_MinOutRate4KHz_1	PCLK Min : 88Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x53fc);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 88Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x53fc);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 88Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x53fc);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 88Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x53fc);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 88Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x53fc);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 88Mhz
    #endif
    //	For MCLK=26MHz, PCLK=91MHz
    #if 1
    S5K4ECGX_write_cmos_sensor(0x002A, 0x01F8);   //System Setting
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x6590);   //REG_TC_IPRM_InClockLSBs MCLK: 26Mhz
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0212);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);   //REG_TC_IPRM_UseNPviClocks
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_TC_IPRM_UseNMipiClocks
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_TC_IPRM_NumberOfMipiLanes
    S5K4ECGX_write_cmos_sensor(0x002A, 0x021A);    
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3A98);   //REG_TC_IPRM_OpClk4KHz_0 SCLK: 60Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_TC_IPRM_MinOutRate4KHz_0	PCLK Min : 91Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_TC_IPRM_MaxOutRate4KHz_0	PCLK Max : 91Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x278D);   //REG_TC_IPRM_OpClk4KHz_1	SCLK 	 : 40.5Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_TC_IPRM_MinOutRate4KHz_1	PCLK Min : 91Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 91Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 91Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 91Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 91Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 91Mhz
    #endif
    //	For MCLK=30MHz, PCLK=90MHz
    #if 0
    S5K4ECGX_write_cmos_sensor(0x002A, 0x01F8);   //System Setting
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x7530);   //REG_TC_IPRM_InClockLSBs MCLK: 30Mhz
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0212);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);   //REG_TC_IPRM_UseNPviClocks
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_TC_IPRM_UseNMipiClocks
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_TC_IPRM_NumberOfMipiLanes
    S5K4ECGX_write_cmos_sensor(0x002A, 0x021A);    
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x3A98);   //REG_TC_IPRM_OpClk4KHz_0 SCLK: 60Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x57E4);   //REG_TC_IPRM_MinOutRate4KHz_0	PCLK Min : 90Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x57E4);   //REG_TC_IPRM_MaxOutRate4KHz_0	PCLK Max : 90Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x278D);   //REG_TC_IPRM_OpClk4KHz_1	SCLK 	 : 40.5Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x57E4);   //REG_TC_IPRM_MinOutRate4KHz_1	PCLK Min : 90Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x57E4);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 90Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x57E4);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 90Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x57E4);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 90Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x57E4);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 90Mhz
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x57E4);   //REG_TC_IPRM_MaxOutRate4KHz_1 PCLK Max : 90Mhz
    #endif

    
    S5K4ECGX_write_cmos_sensor(0x002A, 0x022C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_IPRM_InitParamsUpdated
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0478);   //ETC Setting
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x005F);   //REG_TC_BRC_usPrevQuality
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x005F);   //REG_TC_BRC_usCaptureQuality
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_THUMB_Thumb_bActive
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0280);   //REG_TC_THUMB_Thumb_uWidth
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01E0);   //REG_TC_THUMB_Thumb_uHeight
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   //REG_TC_THUMB_Thumb_Format
    S5K4ECGX_write_cmos_sensor(0x002A, 0x17DC);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0054);   //jpeg_ManualMBCV
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1AE4);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x001C);   //senHal_bExtraAddLine
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0284);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   //REG_TC_GP_bBypassScalerJpg
    S5K4ECGX_write_cmos_sensor(0x002A, 0x028A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_TC_GP_bUse1FrameCaptureMode 0 Continuous mode 1 Single frame mode
       //Preview config[0]
       //91MHz, 1280x960, Dynamic 10~30fps
    S5K4ECGX_write_cmos_sensor(0x002A, 0x02A6);   //Configuration Setting//Normal mode(VGA preview 30~15fps)
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0500);   //REG_0TC_PCFG_usWidth
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03c0);   //REG_0TC_PCFG_usHeight
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   //REG_0TC_PCFG_Format	5 YUV	7 Raw	9 JPG
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_0TC_PCFG_usMaxOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_0TC_PCFG_usMinOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   //REG_0TC_PCFG_OutClkPerPix88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);   //REG_0TC_PCFG_uBpp88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0042);   //REG_0TC_PCFG_PVIMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_OIFMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01E0);   //REG_0TC_PCFG_usJpegPacketSize
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_usJpegTotalPackets
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_uClockInd
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_usFrTimeType
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   //REG_0TC_PCFG_FrRateQualityType
    //S5K4ECGX_write_cmos_sensor(0x0F12, 0x03E8);   //REG_0TC_PCFG_usMaxFrTimeMsecMult10    
    //S5K4ECGX_write_cmos_sensor(0x0F12, 0x014D);   //REG_0TC_PCFG_usMinFrTimeMsecMult10
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01A0);   //REG_0TC_PCFG_usMaxFrTimeMsecMult10    
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01A0);   //REG_0TC_PCFG_usMinFrTimeMsecMult10    
    S5K4ECGX_write_cmos_sensor(0x002A, 0x02D0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_uPrevMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_uCaptureMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_uRotation
       //Preview config[1]
       //91MHz, 1280x960, Dynamic 5~10fps
    S5K4ECGX_write_cmos_sensor(0x002A, 0x02D6);   //Night mode(VGA preview 30~4fps)
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0500);   //REG_1TC_PCFG_usWidth
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03c0);   //REG_1TC_PCFG_usHeight
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   //REG_1TC_PCFG_Format	5 YUV	7 Raw	9 JPG
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_1TC_PCFG_usMaxOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_1TC_PCFG_usMinOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   //REG_1TC_PCFG_OutClkPerPix88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);   //REG_1TC_PCFG_uBpp88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0042);   //REG_1TC_PCFG_PVIMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_PCFG_OIFMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01E0);   //REG_1TC_PCFG_usJpegPacketSize
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_PCFG_usJpegTotalPackets
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_PCFG_uClockInd
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_PCFG_usFrTimeType
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   //REG_1TC_PCFG_FrRateQualityType
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x07D0);   //REG_1TC_PCFG_usMaxFrTimeMsecMult10
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03e8);   //REG_1TC_PCFG_usMinFrTimeMsecMult10
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0300);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_PCFG_uPrevMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_PCFG_uCaptureMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_PCFG_uRotation
       //Preview config[2]
       //91MHz, 1280x960, Fix 30fps
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0306);   //Configuration Setting//Normal mode(VGA preview 30~15fps)
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0500);   //REG_0TC_PCFG_usWidth
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03c0);   //REG_0TC_PCFG_usHeight
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   //REG_0TC_PCFG_Format	5 YUV	7 Raw	9 JPG
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_0TC_PCFG_usMaxOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_0TC_PCFG_usMinOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   //REG_0TC_PCFG_OutClkPerPix88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);   //REG_0TC_PCFG_uBpp88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0042);   //REG_0TC_PCFG_PVIMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_OIFMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01E0);   //REG_0TC_PCFG_usJpegPacketSize
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_usJpegTotalPackets
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_uClockInd
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_usFrTimeType
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   //REG_0TC_PCFG_FrRateQualityType
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x014D);   //REG_0TC_PCFG_usMaxFrTimeMsecMult10
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x014D);   //REG_0TC_PCFG_usMinFrTimeMsecMult10
    S5K4ECGX_write_cmos_sensor(0x002A, 0x02D0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_uPrevMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_uCaptureMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_PCFG_uRotation

       //Preview config[3]
       //91MHz, 1280x720, fixed 30fps
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0336);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0500); //REG_2TC_PCFG_usWidth
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02D0); //REG_2TC_PCFG_usHeight
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005); //REG_2TC_PCFG_Format	             
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE); //REG_2TC_PCFG_usMaxOut4KHzRate      
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE); //REG_2TC_PCFG_usMinOut4KHzRate      
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100); //REG_2TC_PCFG_OutClkPerPix88        
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300); //REG_2TC_PCFG_uBpp88                
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0042); //REG_2TC_PCFG_PVIMask       //YUYV        
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); //REG_2TC_PCFG_OIFMask               
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01E0); //REG_2TC_PCFG_usJpegPacketSize      
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); //REG_2TC_PCFG_usJpegTotalPackets    
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); //REG_2TC_PCFG_uClockInd             
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); //REG_2TC_PCFG_usFrTimeType          
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //REG_2TC_PCFG_FrRateQualityType     
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x014D); //REG_2TC_PCFG_usMaxFrTimeMsecMult10 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x014D); //REG_2TC_PCFG_usMinFrTimeMsecMult10 
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0330);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	//#REG_2TC_PCFG_uPrevMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	//#REG_2TC_PCFG_uCaptureMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	//#REG_2TC_PCFG_uRotation
       //Preview config[4]
       //91MHz, 1280x720, fixed 10fps(for normal video)
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0366);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0500); //REG_3TC_PCFG_usWidth
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x02D0); //REG_3TC_PCFG_usHeight
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005); //REG_3TC_PCFG_Format	             
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE); //REG_3TC_PCFG_usMaxOut4KHzRate      
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE); //REG_3TC_PCFG_usMinOut4KHzRate      
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100); //REG_3TC_PCFG_OutClkPerPix88        
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300); //REG_3TC_PCFG_uBpp88                
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0042); //REG_3TC_PCFG_PVIMask       //YUYV        
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); //REG_3TC_PCFG_OIFMask               
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01E0); //REG_3TC_PCFG_usJpegPacketSize      
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); //REG_3TC_PCFG_usJpegTotalPackets    
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); //REG_3TC_PCFG_uClockInd             
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000); //REG_3TC_PCFG_usFrTimeType          
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //REG_3TC_PCFG_FrRateQualityType     
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03E8); //REG_3TC_PCFG_usMaxFrTimeMsecMult10 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03E8); //REG_3TC_PCFG_usMinFrTimeMsecMult10 
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0360);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	//#REG_3TC_PCFG_uPrevMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	//#REG_3TC_PCFG_uCaptureMirror
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);	//#REG_3TC_PCFG_uRotation
       //Capture config[0]
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0396);  //Normal mode Capture(7.5fps)
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_CCFG_uCaptureMode
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);   //REG_0TC_CCFG_usWidth
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);   //REG_0TC_CCFG_usHeight
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   //REG_0TC_CCFG_Format
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_0TC_CCFG_usMaxOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_0TC_CCFG_usMinOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   //REG_0TC_CCFG_OutClkPerPix88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);   //REG_0TC_CCFG_uBpp88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0042);   //REG_0TC_CCFG_PVIMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_CCFG_OIFMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03C0);   //REG_0TC_CCFG_usJpegPacketSize
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0E80);   //REG_0TC_CCFG_usJpegTotalPackets
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_CCFG_uClockInd
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   //REG_0TC_CCFG_usFrTimeType
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_0TC_CCFG_FrRateQualityType
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0570);   //REG_0TC_CCFG_usMaxFrTimeMsecMult10
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0500);   //REG_0TC_CCFG_usMinFrTimeMsecMult10
       //Capture config[1]
    S5K4ECGX_write_cmos_sensor(0x002A, 0x03C2);   //Night mode Capture(7.5~4fps)
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_CCFG_uCaptureMode
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);   //REG_1TC_CCFG_usWidth
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);   //REG_1TC_CCFG_usHeight
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0005);   //REG_1TC_CCFG_Format
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_1TC_CCFG_usMaxOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x58DE);   //REG_1TC_CCFG_usMinOut4KHzRate
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   //REG_1TC_CCFG_OutClkPerPix88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);   //REG_1TC_CCFG_uBpp88
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0042);   //REG_1TC_CCFG_PVIMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_CCFG_OIFMask
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x03c0);   //REG_1TC_CCFG_usJpegPacketSize
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0E80);   //REG_1TC_CCFG_usJpegTotalPackets
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);   //REG_1TC_CCFG_uClockInd
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   //REG_1TC_CCFG_usFrTimeType
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);   //REG_1TC_CCFG_FrRateQualityType
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0570);   //REG_1TC_CCFG_usMaxFrTimeMsecMult10
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0500);   //REG_1TC_CCFG_usMinFrTimeMsecMult10
    S5K4ECGX_write_cmos_sensor(0xFCFC, 0xd000); 
    S5K4ECGX_write_cmos_sensor(0x0028, 0x7000); 					//AFIT 0
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0250);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000C);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0494);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0262);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    
    
      //unknown
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1CC2);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100); 
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100); 
    S5K4ECGX_write_cmos_sensor(0x002A, 0x01A8);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A0A);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x147C);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0170);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1482);  
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x01E0);
    
    S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0484);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0002);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x183A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x17F6);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x023C);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0248);   
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1840);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0120);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0180);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0800);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0090);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0070);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0045);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0030);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1884);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x1826);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0080);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0030);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0040);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0048);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0050);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0060);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x4784);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00A0);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00D0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0088);   
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00B0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x00C0);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0100);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0300);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x479C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0120);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0150);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x003C);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x003B);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0026);
       //Select preview 0
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0266);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x026A);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x024E);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0268);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0270);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
}

static void S5K4ECGX_enb_preview(){
   
        printk("[4EC] Enable preview...\n");
	S5K4ECGX_write_cmos_sensor(0x002A, 0x023E);
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);  
        printk("[4EC] Enable preview done...\n");
}



/*************************************************************************
* FUNCTION
*    S5K4ECGXReadShutter
*
* DESCRIPTION
*    This function Read Shutter from sensor
*
* PARAMETERS
*    Shutter: integration time
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint16 S5K4ECGXReadShutter(void)
{
	kal_uint32 Shutter=0,Integration_Time=0;
	
	S5K4ECGX_write_cmos_sensor(0x002C,0x7000);
	S5K4ECGX_write_cmos_sensor(0x002E,0x2c28);

	Integration_Time=S5K4ECGX_read_cmos_sensor(0x0F12); 
	Integration_Time=Integration_Time+65536*S5K4ECGX_read_cmos_sensor(0x0F12); 

	//Shutter=PRE_CLK*1000000*Integration_Time/((400*1000)*1920);
	Shutter=PRE_CLK*Integration_Time/768;


	return Shutter;
}

/*************************************************************************
* FUNCTION
*    S5K4ECGXReadGain
*
* DESCRIPTION
*    This function get gain from sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    Gain: base on 0x40
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint32 S5K4ECGXReadGain(void)
{
	kal_uint32 Reg ;

	S5K4ECGX_write_cmos_sensor(0x002C,0x7000);
	S5K4ECGX_write_cmos_sensor(0x002E,0x2bc4);
	Reg=S5K4ECGX_read_cmos_sensor(0x0F12);	

	Reg=Reg/2;
	if(Reg<1)
	{
		Reg=1;
	}

	return Reg; //

	
}
/*************************************************************************
* FUNCTION
*    S5K4ECGXGetEvAwbRef
*
* DESCRIPTION
*    This function get sensor Ev/Awb (EV05/EV13) for auto scene detect
*
* PARAMETERS
*    Ref
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void S5K4ECGXGetEvAwbRef(PSENSOR_AE_AWB_REF_STRUCT Ref)  //???
{
    Ref->SensorAERef.AeRefLV05Shutter = 3816; //0xc6c
    Ref->SensorAERef.AeRefLV05Gain = 896; /* 4.1x, 128 base */
    Ref->SensorAERef.AeRefLV13Shutter = 99;   //0x88
    Ref->SensorAERef.AeRefLV13Gain = 1 * 128; /* 2x, 128 base */
    Ref->SensorAwbGainRef.AwbRefD65Rgain = 210; //0xc4/* 1.58x, 128 base */
    Ref->SensorAwbGainRef.AwbRefD65Bgain = 149; //0xa6/* 1.23x, 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFRgain = 179; //0xb9/* 1.4453125x, 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFBgain = 267; //0xf1/* 1.8828125x, 128 base */
}
/*************************************************************************
* FUNCTION
*    S5K4ECGXGetCurAeAwbInfo
*
* DESCRIPTION
*    This function get sensor cur Ae/Awb for auto scene detect
*
* PARAMETERS
*    Info
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void S5K4ECGXGetCurAeAwbInfo(PSENSOR_AE_AWB_CUR_STRUCT Info)
{
    Info->SensorAECur.AeCurShutter = S5K4ECGXReadShutter();
    Info->SensorAECur.AeCurGain = S5K4ECGXReadGain(); /* 128 base */
    S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
    	
	Info->SensorAwbGainCur.AwbCurRgain=S5K4ECGX_read_cmos_sensor(0x2bd0)/8; //   (sensorGain/1024)*128// 
    Info->SensorAwbGainCur.AwbCurBgain = S5K4ECGX_read_cmos_sensor(0x2bd4)/8; /* 128 base */
}


static void S5K4ECGX_Get_AF_Max_Num_Focus_Areas(UINT32 *pFeatureReturnPara32)
{ 	
    
    *pFeatureReturnPara32 = 0;    
    SENSORDB(" *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);	
}

static void S5K4ECGX_Get_AE_Max_Num_Metering_Areas(UINT32 *pFeatureReturnPara32)
{ 	
    
    *pFeatureReturnPara32 = 0;    
    SENSORDB(" *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);	
}


struct S5K4ECGX_sensor_struct S5K4ECGX_Sensor_Driver;




/*************************************************************************
* FUNCTION
*	S5K4ECOpen
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
 
UINT32 S5K4ECGXOpen(void)
{
	kal_uint16 sensor_id=0;
	kal_uint16 retry =0;	
  
    SENSORDB("[Enter]:S5K4ECGX Open func zhijie:\r\n");
    
#if 1    
    retry = 3; 
    do {
        S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);
	    S5K4ECGX_write_cmos_sensor(0x002C,0x7000);
	    S5K4ECGX_write_cmos_sensor(0x002E,0x01A4);//id register
		sensor_id = S5K4ECGX_read_cmos_sensor(0x0F12);

        if (sensor_id == S5K4ECGX_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", sensor_id); 
        retry--; 
    } while (retry > 0);
	
    SENSORDB("Read Sensor ID = 0x%04x\n", sensor_id); 
	
    if (sensor_id != S5K4ECGX_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;
#endif	

    

    S5K4ECGX_Init_Setting();
    S5K4ECGX_enb_preview();
     
	return ERROR_NONE;
}	/* S5K4ECGXOpen() */


UINT32 S5K4ECGXGetSensorID(UINT32 *sensorID)
{
#if 1
	int  retry = 3; 
    SENSORDB("S5K4ECGXGetSensorID \n");
	// check if sensor ID correct
	do {
	    S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);
	    S5K4ECGX_write_cmos_sensor(0x002C,0x7000);
	    S5K4ECGX_write_cmos_sensor(0x002E,0x01A4);//id register
		*sensorID = S5K4ECGX_read_cmos_sensor(0x0F12);	
		
		if (*sensorID == S5K4ECGX_SENSOR_ID)
			break; 
		SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
		retry--; 
	} while (retry > 0);

	if (*sensorID != S5K4ECGX_SENSOR_ID) {
		*sensorID = 0xFFFFFFFF; 
		return ERROR_SENSOR_CONNECT_FAIL;
	}
#endif	
    
	return ERROR_NONE;

	
}	/* S5K4ECGXGetSensorID() */


/*************************************************************************
* FUNCTION
*	S5K4ECGXClose
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
UINT32 S5K4ECGXClose(void)
{

	return ERROR_NONE;
}	/* S5K4ECGXClose() */

static void S5K4ECGX_Preview_Mode_Setting(kal_uint8 preview_mode )
{
   
        SENSORDB("[Enter]:Preview mode: mode=%d\r\n", preview_mode);
	
	S5K4ECGX_write_cmos_sensor(0xFCFC,0xd000);
	S5K4ECGX_write_cmos_sensor(0x0028,0x7000);

	S5K4ECGX_write_cmos_sensor(0x002A, 0x0250); 
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000C);				
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0010);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x000C);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0494);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0A00);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0780);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
    S5K4ECGX_write_cmos_sensor(0x002A, 0x0262);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
    S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);

    S5K4ECGX_write_cmos_sensor(0x002A, 0x0266);
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000+preview_mode); 	  //#REG_TC_GP_ActivePrevConfig
	S5K4ECGX_write_cmos_sensor(0x002A, 0x026A);
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); 	  //#REG_TC_GP_PrevOpenAfterChange
	S5K4ECGX_write_cmos_sensor(0x002A, 0x024E);
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); 	  //#REG_TC_GP_NewConfigSync
	S5K4ECGX_write_cmos_sensor(0x002A, 0x0268);
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); 	  //#REG_TC_GP_PrevConfigChanged
	S5K4ECGX_write_cmos_sensor(0x002A, 0x0270);
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); 	  //#REG_TC_GP_CapConfigChanged
	S5K4ECGX_write_cmos_sensor(0x002A, 0x023E);
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); 	  //#REG_TC_GP_EnablePreview
	S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);

        SENSORDB("[Exit]:Preview mode:\r\n");

}
static void S5K4ECGX_Capture_Mode_Setting(kal_uint8 capture_mode )
{
   
	kal_uint32 af_error_state=0xff;
	SENSORDB("[Enter]:Enter capture mode\r\n");
	
	while(af_error_state!=0)
	{
		S5K4ECGX_write_cmos_sensor(0xFCFC,0xd000);
		S5K4ECGX_write_cmos_sensor(0x002C,0x7000);
		S5K4ECGX_write_cmos_sensor(0x002E,0x0290);
		af_error_state=S5K4ECGX_read_cmos_sensor(0x0F12); //Index number of active capture configuration //Normal capture// 
		Sleep(10);
	}
	SENSORDB("[Exit]:Leave af state check\r\n");

//test config if active

	kal_uint32 capture_state=0xff;
	
	while(capture_state!=0)
	{
		S5K4ECGX_write_cmos_sensor(0xFCFC,0xd000);
		S5K4ECGX_write_cmos_sensor(0x002C,0x7000);
		S5K4ECGX_write_cmos_sensor(0x002E,0x0272); //capture state
		capture_state=S5K4ECGX_read_cmos_sensor(0x0F12); //Index number of active capture configuration //Normal capture// 
		SENSORDB("[Enter]:1808capture state=%d\r\n", capture_state);
		{
			S5K4ECGX_write_cmos_sensor(0x002a, 0x026e);
			S5K4ECGX_write_cmos_sensor(0x0f12, 0x0000+capture_mode);			
			S5K4ECGX_write_cmos_sensor(0x002A, 0x0242);
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x024E);
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x0244);
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x0272);
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
		}
	
		Sleep(10);
	}
        SENSORDB("[Exit]:Capture mode:\r\n");
	

}

static void S5K4ECGX_HVMirror(kal_uint8 image_mirror)
{
/********************************************************
Preview:Mirror: 0x02d0 bit[0],Flip :    0x02d0 bit[1]
Capture:Mirror: 0x02d2 bit[0],Flip :    0x02d2 bit[1]
*********************************************************/

        SENSORDB("[Enter]:Mirror\r\n");
	S5K4ECGX_write_cmos_sensor(0xFCFC,0xd000);
	S5K4ECGX_write_cmos_sensor(0x0028,0x7000);


	switch (image_mirror) {
	    default:
		case IMAGE_NORMAL:

			S5K4ECGX_write_cmos_sensor(0x002A,	0x02D0);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_0TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_0TC_PCFG_uCaptureMirror


			S5K4ECGX_write_cmos_sensor(0x002A,	0x0300);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_1TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_1TC_PCFG_uCaptureMirror
			
			S5K4ECGX_write_cmos_sensor(0x002A,	0x0330);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_2TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_2TC_PCFG_uCaptureMirror
			
			S5K4ECGX_write_cmos_sensor(0x002A,	0x0360);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_3TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_3TC_PCFG_uCaptureMirror

			S5K4ECGX_write_cmos_sensor(0x002A,	0x0390);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_4TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0000);	//#REG_4TC_PCFG_uCaptureMirror

            break;
		case IMAGE_H_MIRROR:
			S5K4ECGX_write_cmos_sensor(0x002A,	0x02D0);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_0TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_0TC_PCFG_uCaptureMirror


			S5K4ECGX_write_cmos_sensor(0x002A,	0x0300);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_1TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_1TC_PCFG_uCaptureMirror
			
			S5K4ECGX_write_cmos_sensor(0x002A,	0x0330);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_2TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_2TC_PCFG_uCaptureMirror
			
			S5K4ECGX_write_cmos_sensor(0x002A,	0x0360);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_3TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_3TC_PCFG_uCaptureMirror

			S5K4ECGX_write_cmos_sensor(0x002A,	0x0390);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_4TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0001);	//#REG_4TC_PCFG_uCaptureMirror
            break;
		case IMAGE_V_MIRROR:
			S5K4ECGX_write_cmos_sensor(0x002A,	0x02D0);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_0TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_0TC_PCFG_uCaptureMirror


			S5K4ECGX_write_cmos_sensor(0x002A,	0x0300);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_1TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_1TC_PCFG_uCaptureMirror
			
			S5K4ECGX_write_cmos_sensor(0x002A,	0x0330);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_2TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_2TC_PCFG_uCaptureMirror
			
			S5K4ECGX_write_cmos_sensor(0x002A,	0x0360);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_3TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_3TC_PCFG_uCaptureMirror

			S5K4ECGX_write_cmos_sensor(0x002A,	0x0390);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_4TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0002);	//#REG_4TC_PCFG_uCaptureMirror
            break;
		case IMAGE_HV_MIRROR:
			S5K4ECGX_write_cmos_sensor(0x002A,	0x02D0);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_0TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_0TC_PCFG_uCaptureMirror


			S5K4ECGX_write_cmos_sensor(0x002A,	0x0300);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_1TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_1TC_PCFG_uCaptureMirror
			
			S5K4ECGX_write_cmos_sensor(0x002A,	0x0330);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_2TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_2TC_PCFG_uCaptureMirror
			
			S5K4ECGX_write_cmos_sensor(0x002A,	0x0360);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_3TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_3TC_PCFG_uCaptureMirror

			S5K4ECGX_write_cmos_sensor(0x002A,	0x0390);
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_4TC_PCFG_uPrevMirror
			S5K4ECGX_write_cmos_sensor(0x0F12,	0x0003);	//#REG_4TC_PCFG_uCaptureMirror
            break;
	}

}

void S5K4ECGX_night_mode(kal_bool enable)
{
      
	  SENSORDB("[Enter]S5K4ECGX night mode func:enable = %d\n",enable);
	if(enable)
	{
    		S5K4ECGX_Preview_Mode_Setting(1); ////MODE0=5-30FPS
	}
	else
        {
    		S5K4ECGX_Preview_Mode_Setting(0); ////MODE0=10-30FPS
	}
	
}

/*************************************************************************
* FUNCTION
*	S5K4ECGXPreview
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
static UINT32 S5K4ECGXPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{	
     
	SENSORDB("[Enter]:S5K4ECGX preview func:");		

    S5K4ECGX_HVMirror(sensor_config_data->SensorImageMirror);
    S5K4ECGX_Preview_Mode_Setting(0); ////MODE0=10-30FPS
    
	image_window->GrabStartX = S5K4ECGX_PV_X_START;
	image_window->GrabStartY = S5K4ECGX_PV_Y_START;
	image_window->ExposureWindowWidth = S5K4ECGX_IMAGE_SENSOR_PV_WIDTH-S5K4ECGX_PV_X_START;
	image_window->ExposureWindowHeight = S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT-S5K4ECGX_PV_Y_START;
	
	memcpy(&S5K4ECGXSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	SENSORDB("[Exit]:S5K4ECGX preview func\n");
    return ERROR_NONE; 
}	/* S5K4ECGXPreview */

UINT32 S5K4ECGXCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    
	spin_lock(&s5k4ecgx_drv_lock);
	S5K4ECGX_Sensor_Driver.Camco_mode = S5K4ECGX_CAM_CAPTURE;

	S5K4ECGX_Sensor_Driver.StartX=1;
	S5K4ECGX_Sensor_Driver.StartY=1;
	spin_unlock(&s5k4ecgx_drv_lock);;


	//when entry capture mode,will auto close ae,awb . 
	SENSORDB("[Enter]:targetwidth=%d\r\n", image_window->ImageTargetWidth);

    if(image_window->ImageTargetWidth <= S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV)
    {
    	spin_lock(&s5k4ecgx_drv_lock);
    	S5K4ECGX_Sensor_Driver.iGrabWidth=S5K4ECGX_IMAGE_SENSOR_PV_WIDTH-S5K4ECGX_PV_X_START;
    	S5K4ECGX_Sensor_Driver.iGrabheight=S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT-S5K4ECGX_PV_Y_START;
		spin_unlock(&s5k4ecgx_drv_lock);;

    	image_window->GrabStartX = S5K4ECGX_PV_X_START;
    	image_window->GrabStartY = S5K4ECGX_PV_Y_START;
    	image_window->ExposureWindowWidth = S5K4ECGX_IMAGE_SENSOR_PV_WIDTH-S5K4ECGX_PV_X_START;
    	image_window->ExposureWindowHeight = S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT-S5K4ECGX_PV_Y_START;
    }
    else
    {
    	S5K4ECGX_Capture_Mode_Setting(0);

    	Sleep(4);
    	spin_lock(&s5k4ecgx_drv_lock);
    	S5K4ECGX_Sensor_Driver.iGrabWidth=S5K4ECGX_IMAGE_SENSOR_FULL_WIDTH_DRV - 16;
    	S5K4ECGX_Sensor_Driver.iGrabheight=S5K4ECGX_IMAGE_SENSOR_FULL_HEIGHT_DRV - 12;
		spin_unlock(&s5k4ecgx_drv_lock);;
    
    	image_window->GrabStartX = S5K4ECGX_Sensor_Driver.StartX;
    	image_window->GrabStartY = S5K4ECGX_Sensor_Driver.StartY;
    	image_window->ExposureWindowWidth = S5K4ECGX_Sensor_Driver.iGrabWidth;
    	image_window->ExposureWindowHeight = S5K4ECGX_Sensor_Driver.iGrabheight;
    }
	
}


UINT32 S5K4ECGXGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[Enter]:S5K4ECGX get Resolution func\n");
	
	pSensorResolution->SensorFullWidth=S5K4ECGX_IMAGE_SENSOR_FULL_WIDTH_DRV;  
	pSensorResolution->SensorFullHeight=S5K4ECGX_IMAGE_SENSOR_FULL_HEIGHT_DRV;
	pSensorResolution->SensorPreviewWidth=S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV;
	pSensorResolution->SensorPreviewHeight=S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT_DRV;
	pSensorResolution->Sensor3DFullWidth=S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV;  
	pSensorResolution->Sensor3DFullHeight=S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT_DRV;
	pSensorResolution->Sensor3DPreviewWidth=S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV;
	pSensorResolution->Sensor3DPreviewHeight=S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT_DRV;


    SENSORDB("[Exit]:S5K4ECGX get Resolution func\n");
	
	return ERROR_NONE;
}	/* NSXC301HS5K4ECGXGetResolution() */

UINT32 S5K4ECGXGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("[Enter]:S5K4ECGX getInfo func:ScenarioId = %d\n",ScenarioId);
   
	pSensorInfo->SensorPreviewResolutionX=S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV;
	pSensorInfo->SensorPreviewResolutionY=S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT_DRV;
	pSensorInfo->SensorFullResolutionX=S5K4ECGX_IMAGE_SENSOR_FULL_WIDTH_DRV;
	pSensorInfo->SensorFullResolutionY=S5K4ECGX_IMAGE_SENSOR_FULL_HEIGHT_DRV;

//tbd
        pSensorInfo->SensorCameraPreviewFrameRate=12;
        pSensorInfo->SensorVideoFrameRate=30;
        pSensorInfo->SensorStillCaptureFrameRate=5;
        SENSORDB("[Enter]:2005still frame=%d\r\n", pSensorInfo->SensorStillCaptureFrameRate);
        pSensorInfo->SensorWebCamCaptureFrameRate=15;
        pSensorInfo->SensorResetActiveHigh=FALSE;//low is to reset 
        pSensorInfo->SensorResetDelayCount=4;  //4ms 
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_UYVY;
//	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
        pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
        pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
        pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
        pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
        pSensorInfo->SensorInterruptDelayLines = 1; 
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;
	pSensorInfo->SensorDriver3D = SENSOR_3D_NOT_SUPPORT;

        pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH; //???
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

	pSensorInfo->CaptureDelayFrame = 1; 
	pSensorInfo->PreviewDelayFrame = 2; 
        pSensorInfo->VideoDelayFrame = 0; 
        pSensorInfo->SensorMasterClockSwitch = 0; 
        pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;   		
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
                    pSensorInfo->SensorClockFreq=26;
                    pSensorInfo->SensorClockDividCount=	5;
                    pSensorInfo->SensorClockRisingCount= 0;
                    pSensorInfo->SensorClockFallingCount= 2;
                    pSensorInfo->SensorPixelClockCount= 3;
                    pSensorInfo->SensorDataLatchCount= 2;
                    pSensorInfo->SensorGrabStartX = S5K4ECGX_PV_X_START; 
                    pSensorInfo->SensorGrabStartY = S5K4ECGX_PV_Y_START;  			
		    pSensorInfo->SensorPreviewResolutionX=S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV;
		    pSensorInfo->SensorPreviewResolutionY=S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT_DRV;
		    pSensorInfo->SensorFullResolutionX=S5K4ECGX_IMAGE_SENSOR_FULL_WIDTH_DRV;
		    pSensorInfo->SensorFullResolutionY=S5K4ECGX_IMAGE_SENSOR_FULL_HEIGHT_DRV;
			
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
                    pSensorInfo->SensorClockFreq=26;
                    pSensorInfo->SensorClockDividCount=	5;
                    pSensorInfo->SensorClockRisingCount= 0;
                    pSensorInfo->SensorClockFallingCount= 2;
                    pSensorInfo->SensorPixelClockCount= 3;
                    pSensorInfo->SensorDataLatchCount= 2;
                    pSensorInfo->SensorGrabStartX = S5K4ECGX_FULL_X_START; 
                    pSensorInfo->SensorGrabStartY = S5K4ECGX_FULL_Y_START;			
		    pSensorInfo->SensorPreviewResolutionX=S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV;
		    pSensorInfo->SensorPreviewResolutionY=S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT_DRV;
		    pSensorInfo->SensorFullResolutionX=S5K4ECGX_IMAGE_SENSOR_FULL_WIDTH_DRV;
		    pSensorInfo->SensorFullResolutionY=S5K4ECGX_IMAGE_SENSOR_FULL_HEIGHT_DRV;
		break;
		default:
                    pSensorInfo->SensorClockFreq=26;
                    pSensorInfo->SensorClockDividCount=	5;
                    pSensorInfo->SensorClockRisingCount= 0;
                    pSensorInfo->SensorClockFallingCount= 2;
                    pSensorInfo->SensorPixelClockCount=3;
                    pSensorInfo->SensorDataLatchCount=2;
                    pSensorInfo->SensorGrabStartX = S5K4ECGX_PV_X_START; 
                    pSensorInfo->SensorGrabStartY = S5K4ECGX_PV_Y_START;  			
		    pSensorInfo->SensorPreviewResolutionX=S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV;
		    pSensorInfo->SensorPreviewResolutionY=S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT_DRV;
		    pSensorInfo->SensorFullResolutionX=S5K4ECGX_IMAGE_SENSOR_FULL_WIDTH_DRV;
		    pSensorInfo->SensorFullResolutionY=S5K4ECGX_IMAGE_SENSOR_FULL_HEIGHT_DRV;
		break;
	}

	SENSORDB("[Exit]:S5K4ECGX getInfo func\n");
	return ERROR_NONE;
}	/* NSXC301HS5K4ECGXGetInfo() */
static void S5K4ECGX_set_AF_infinite(kal_bool is_AF_OFF)
{
	if(is_AF_OFF){
		S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
		S5K4ECGX_write_cmos_sensor(0x002a, 0x028E);
		S5K4ECGX_write_cmos_sensor(0x0F12, 0x0000);
		Sleep(100);
		S5K4ECGX_write_cmos_sensor(0x002a, 0x028C);
		S5K4ECGX_write_cmos_sensor(0x0F12, 0x0004);
	} else {
		S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
		S5K4ECGX_write_cmos_sensor(0x002a, 0x028C);
		S5K4ECGX_write_cmos_sensor(0x0F12, 0x0003);
	}

}
/*************************************************************************
* FUNCTION
*	S5K4ECGX_set_param_effect
*
* DESCRIPTION
*	effect setting.
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
BOOL S5K4ECGX_set_param_effect(UINT16 para)
{

   SENSORDB("[Enter]S5K4ECGX set_param_effect func:para = %d\n",para);
   switch (para)
	{
		case MEFFECT_OFF:
		{
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023c);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0000);	//REG_TC_GP_SpecialEffects 	
        }
	        break;
		case MEFFECT_NEGATIVE:
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023c);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0002);	//REG_TC_GP_SpecialEffects 	
			break;
		case MEFFECT_SEPIA:
		{
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023c);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0004);	//REG_TC_GP_SpecialEffects 	
        }	
			break;  
		case MEFFECT_SEPIABLUE:
		{
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023c);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0007);	//REG_TC_GP_SpecialEffects 	
	    }     
			break;        
		case MEFFECT_SEPIAGREEN:		
		{
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023c);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0008);	//REG_TC_GP_SpecialEffects 	
	    }     
			break;        
		case MEFFECT_MONO:			
		{
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023c);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0001);	//REG_TC_GP_SpecialEffects 	
        }
			break;

		default:
			return KAL_FALSE;
	}

	return KAL_TRUE;

} /* S5K4ECGX_set_param_effect */

UINT32 S5K4ECGXControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
   SENSORDB("[Enter]:S5K4ECGXControl  func:ScenarioId = %d\n",ScenarioId);

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:

			 S5K4ECGXPreview(pImageWindow, pSensorConfigData);
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			 S5K4ECGXCapture(pImageWindow, pSensorConfigData);
			 break;			 
		default:
		     break; 
	}

   SENSORDB("[Exit]:S5K4ECGXControl  func\n");
	
	return ERROR_NONE;
}	/* S5K4ECGXControl() */


/*************************************************************************
* FUNCTION
*	S5K4ECGX_set_param_wb
*
* DESCRIPTION
*	wb setting.
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

BOOL S5K4ECGX_set_param_wb(UINT16 para)
{
	
    //This sensor need more time to balance AWB, 
    //we suggest higher fps or drop some frame to avoid garbage color when preview initial
   SENSORDB("[Enter]S5K4ECGX set_param_wb func:para = %d\n",para);
	kal_uint16 Status_3A=0;
	while(Status_3A==0)
	{
		S5K4ECGX_write_cmos_sensor(0xFCFC,0xd000);
		S5K4ECGX_write_cmos_sensor(0x002C,0x7000);
		S5K4ECGX_write_cmos_sensor(0x002E,0x04E6);
		Status_3A=S5K4ECGX_read_cmos_sensor(0x0F12); //Index number of active capture configuration //Normal capture// 
		Sleep(10);
	}
	
	switch (para)
	{            
		case AWB_MODE_AUTO:
			{
			Status_3A = (Status_3A | 0x8); // Enable AWB
			S5K4ECGX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
			S5K4ECGX_write_cmos_sensor(0x002a, 0x04e6);//
			S5K4ECGX_write_cmos_sensor(0x0F12, Status_3A);//
			//S5K4ECGX_write_cmos_sensor(0x0F12, 0x077F);//
                        }                
		    break;
		case AWB_MODE_CLOUDY_DAYLIGHT:
			{
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K4ECGX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04E6);
			S5K4ECGX_write_cmos_sensor(0x0F12, Status_3A);
//			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0777);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04BA); 
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0740); //Reg_sf_user_Rgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x03D0); //0400Reg_sf_user_Ggain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x04D0); //0460Reg_sf_user_Bgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
	        }			   
		    break;
		case AWB_MODE_DAYLIGHT:
		    {
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K4ECGX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04E6);
			S5K4ECGX_write_cmos_sensor(0x0F12, Status_3A);
//			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0777);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04BA); 
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x06c5); //05E0Reg_sf_user_Rgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400); //0400Reg_sf_user_Ggain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x04d3); //0530Reg_sf_user_Bgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
            }      
		    break;
		case AWB_MODE_INCANDESCENT:	
		    {
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K4ECGX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04E6);
			S5K4ECGX_write_cmos_sensor(0x0F12, Status_3A);
//			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0777);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04BA); 
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0401); //0575Reg_sf_user_Rgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400); //0400Reg_sf_user_Ggain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0957); //0800Reg_sf_user_Bgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
            }		
		    break;  
		case AWB_MODE_FLUORESCENT:
		    {
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K4ECGX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04E6);
			S5K4ECGX_write_cmos_sensor(0x0F12, Status_3A);
//			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0777);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04BA); 
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x05a1); //0400Reg_sf_user_Rgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0400); //0400Reg_sf_user_Ggain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x08e7); //Reg_sf_user_Bgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
            }	
		    break;  
		case AWB_MODE_TUNGSTEN:
		    {
			Status_3A = (Status_3A & 0xFFF7); // Disable AWB
			S5K4ECGX_write_cmos_sensor(0xFCFC, 0xD000);
			S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04E6);
			S5K4ECGX_write_cmos_sensor(0x0F12, Status_3A);
//			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0777);
			S5K4ECGX_write_cmos_sensor(0x002A, 0x04BA); 
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200); //0400Reg_sf_user_Rgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_RgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0200); //0400Reg_sf_user_Ggain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_GgainChanged
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x04A0); //Reg_sf_user_Bgain
			S5K4ECGX_write_cmos_sensor(0x0F12, 0x0001); //Reg_sf_user_BgainChanged
            }	
		    break;  			    
		default:
			return FALSE;
	}
        SENSORDB("Status_3A = 0x%x\n",Status_3A);

	return TRUE;
	
} /* S5K4ECGX_set_param_wb */


/*************************************************************************
* FUNCTION
*	S5K4ECGX_set_param_banding
*
* DESCRIPTION
*	banding setting.
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
BOOL S5K4ECGX_set_param_banding(UINT16 para)
{
	SENSORDB("[Enter]S5K4ECGX set_param_banding func:para = %d\n",para);
	kal_uint16 Status_3A=0;
	while(Status_3A==0)
	{
		S5K4ECGX_write_cmos_sensor(0xFCFC,0xd000);
		S5K4ECGX_write_cmos_sensor(0x002C,0x7000);
		S5K4ECGX_write_cmos_sensor(0x002E,0x04E6);
		Status_3A=S5K4ECGX_read_cmos_sensor(0x0F12); //Index number of active capture configuration //Normal capture// 
		Sleep(10);
	}
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
	    {
			Status_3A = (Status_3A & 0xFFDF); // disable auto-flicker
			spin_lock(&s5k4ecgx_drv_lock);
			S5K4ECGX_Sensor_Driver.Banding = AE_FLICKER_MODE_50HZ;
			spin_unlock(&s5k4ecgx_drv_lock);;
			S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);   
			S5K4ECGX_write_cmos_sensor(0x002a, 0x04e6);   
			S5K4ECGX_write_cmos_sensor(0x0f12, Status_3A);   
//			S5K4ECGX_write_cmos_sensor(0x0f12, 0x075f);   
			S5K4ECGX_write_cmos_sensor(0x002a, 0x04d6);   
			S5K4ECGX_write_cmos_sensor(0x0f12, 0x0001);   
			S5K4ECGX_write_cmos_sensor(0x0f12, 0x0001);
	    }
		break;

		case AE_FLICKER_MODE_60HZ:
	    {
			Status_3A = (Status_3A & 0xFFDF); // disable auto-flicker
			spin_lock(&s5k4ecgx_drv_lock);
			S5K4ECGX_Sensor_Driver.Banding = AE_FLICKER_MODE_60HZ;
			spin_unlock(&s5k4ecgx_drv_lock);;
			S5K4ECGX_write_cmos_sensor(0x0028, 0x7000);   
			S5K4ECGX_write_cmos_sensor(0x002a, 0x04e6);   
			S5K4ECGX_write_cmos_sensor(0x0f12, Status_3A);   
			S5K4ECGX_write_cmos_sensor(0x002a, 0x04d6);   
			S5K4ECGX_write_cmos_sensor(0x0f12, 0x0002);   
			S5K4ECGX_write_cmos_sensor(0x0f12, 0x0001); 
	    }
		break;

	    default:
	        return KAL_FALSE;
	}
        SENSORDB("Status_3A = 0x%x\n",Status_3A);
	return KAL_TRUE;
} /* S5K4ECGX_set_param_banding */



/*************************************************************************
* FUNCTION
*	S5K4ECGX_set_param_exposure
*
* DESCRIPTION
*	exposure setting.
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
BOOL S5K4ECGX_set_param_exposure(UINT16 para)
{

	kal_uint16 base_target = 0;

	SENSORDB("[Enter]S5K4ECGX set_param_exposure func:para = %d\n",para);
	switch (para)
	{
		case AE_EV_COMP_13:  //+4 EV
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023A);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0200);	//TVAR_ae_BrAve  
			break;  
		case AE_EV_COMP_10:  //+3 EV
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023A);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0190);	//TVAR_ae_BrAve  
			break;    
		case AE_EV_COMP_07:  //+2 EV
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023A);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0170);	//TVAR_ae_BrAve  
			break;    
		case AE_EV_COMP_03:  //	+1 EV	
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023A);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0145);	//TVAR_ae_BrAve  
			break;    
		case AE_EV_COMP_00:  // +0 EV
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023A);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0100);	//TVAR_ae_BrAve  
			break;    
		case AE_EV_COMP_n03:  // -1 EV
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023A);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x00C0);	//TVAR_ae_BrAve  
			break;    
		case AE_EV_COMP_n07:	// -2 EV		
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023A);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0080);	//TVAR_ae_BrAve  
			break;    
		case AE_EV_COMP_n10:   //-3 EV
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023A);					 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0060);	//TVAR_ae_BrAve  
			break;
		case AE_EV_COMP_n13:  // -4 EV
			S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);				 
			S5K4ECGX_write_cmos_sensor(0x0028,0x7000);				 
			S5K4ECGX_write_cmos_sensor(0x002A,0x023A);				 
			S5K4ECGX_write_cmos_sensor(0x0F12,0x0040);	//TVAR_ae_BrAve
			break;
		default:
			return FALSE;
	}
	return TRUE;
	
} /* S5K4ECGX_set_param_exposure */

UINT32 S5K4ECGXYUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
        
    SENSORDB("[Enter]S5K4ECGXYUVSensorSetting func:cmd = %d\n",iCmd);
	switch (iCmd) {
	case FID_SCENE_MODE:	    //auto mode or night mode
	    if (iPara == SCENE_MODE_OFF)//auto mode
	    {
	        S5K4ECGX_night_mode(FALSE); 
	    }
	    else if (iPara == SCENE_MODE_NIGHTSCENE)//night mode
	    {
                S5K4ECGX_night_mode(TRUE); 
	    }	
	     break; 	    
	case FID_AWB_MODE:
           S5K4ECGX_set_param_wb(iPara);
	     break;
	case FID_COLOR_EFFECT:
           S5K4ECGX_set_param_effect(iPara);
	     break;
	case FID_AE_EV:	    	    
           S5K4ECGX_set_param_exposure(iPara);
	     break;
	case FID_AE_FLICKER:	    	    	    
           S5K4ECGX_set_param_banding(iPara);
	     break;
	case FID_ZOOM_FACTOR:
	     break; 
	default:
	     break;
	}
	return TRUE;
}   /* S5K4ECGXYUVSensorSetting */

UINT32 S5K4ECGXYUVSetVideoMode(UINT16 u2FrameRate)
{
    
    SENSORDB("[Enter]S5K4ECGX Set Video Mode:FrameRate= %d\n",u2FrameRate);
		kal_uint16 frameTime=0;

		frameTime=10000/u2FrameRate;  //fps=10x
		//Configuration Setting 	  preview2	   1280*960 	for video fix fps  
		
		S5K4ECGX_write_cmos_sensor(0xFCFC,0xD000);					 
		S5K4ECGX_write_cmos_sensor(0x0028,0x7000);					 
		S5K4ECGX_write_cmos_sensor(0x002A,0x0306);
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0500);//0320//0280	//#REG_2TC_PCFG_usWidth
		S5K4ECGX_write_cmos_sensor(0x0F12,0x03C0);//0258//01E0	//#REG_2TC_PCFG_usHeight
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0005);	//#REG_2TC_PCFG_Format
		S5K4ECGX_write_cmos_sensor(0x0F12,0x58DE);//4F1E	//#REG_2TC_PCFG_usMaxOut4KHzRate
		S5K4ECGX_write_cmos_sensor(0x0F12,0x58DE);//4F16	//#REG_2TC_PCFG_usMinOut4KHzRate
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0100);	//#REG_2TC_PCFG_OutClkPerPix88
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0300);	//#REG_2TC_PCFG_uBpp88
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0042);	//#REG_2TC_PCFG_PVIMask
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0000);//0000//0000	//#REG_2TC_PCFG_OIFMask
		S5K4ECGX_write_cmos_sensor(0x0F12,0x01E0);	//#REG_2TC_PCFG_usJpegPacketSize
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0000);//0000	//#REG_2TC_PCFG_usJpegTotalPackets
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0000);	//#REG_2TC_PCFG_uClockInd
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0000);	//#REG_2TC_PCFG_usFrTimeType
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0001);//0001	//#REG_2TC_PCFG_FrRateQualityType
		S5K4ECGX_write_cmos_sensor(0x0F12,frameTime);//029A//014D//029A	//#REG_2TC_PCFG_usMaxFrTimeMsecMult10
		S5K4ECGX_write_cmos_sensor(0x0F12,frameTime);//0000	//#REG_2TC_PCFG_usMinFrTimeMsecMult10
		S5K4ECGX_write_cmos_sensor(0x002A,0x0330);
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0000);	//#REG_2TC_PCFG_uPrevMirror
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0000);	//#REG_2TC_PCFG_uCaptureMirror
		S5K4ECGX_write_cmos_sensor(0x0F12,0x0000);	//#REG_2TC_PCFG_uRotation
		S5K4ECGX_Preview_Mode_Setting(2);
    return TRUE;
}




UINT32 S5K4ECGXFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 u2Temp = 0; 
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	ACDK_SENSOR_REG_W_ID_INFO_STRUCT *pSensorRegData_wID=(ACDK_SENSOR_REG_W_ID_INFO_STRUCT *) pFeaturePara;

	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=S5K4ECGX_IMAGE_SENSOR_FULL_WIDTH_DRV;
			*pFeatureReturnPara16=S5K4ECGX_IMAGE_SENSOR_FULL_HEIGHT_DRV;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_GET_PERIOD:
		    /*
			*pFeatureReturnPara16++=S5K4ECGX_IMAGE_SENSOR_VGA_WIDTH+S5K4ECGX_Sensor_Driver.Dummy_Pixels;
			*pFeatureReturnPara16=S5K4ECGX_IMAGE_SENSOR_VGA_WIDTH+S5K4ECGX_Sensor_Driver.Dummy_Lines;
			*pFeatureParaLen=4;
			*/
		     break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			//*pFeatureReturnPara32 = S5K4ECGX_sensor_pclk/10;
			//*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_SET_ESHUTTER:
	
		     break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			 S5K4ECGX_night_mode((BOOL) *pFeatureData16);
		     break;
		case SENSOR_FEATURE_SET_GAIN:
			 break; 
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		     break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		     break;
		case SENSOR_FEATURE_SET_REGISTER:
			 S5K4ECGX_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		     break;
		case SENSOR_FEATURE_GET_REGISTER:
			 pSensorRegData->RegData = S5K4ECGX_read_cmos_sensor(pSensorRegData->RegAddr);
		     break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			 memcpy(pSensorConfigData, &S5K4ECGXSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
	               // *pFeatureReturnPara32++=0;
			//*pFeatureParaLen=4;
		     break; 
		
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_SET_YUV_CMD:
			 S5K4ECGXYUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		     break;	
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		     S5K4ECGXYUVSetVideoMode(*pFeatureData16);
		     break; 
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
             S5K4ECGXGetSensorID(pFeatureReturnPara32); 
            break;     
/*            
		case SENSOR_FEATURE_SET_REGISTER_W_ID:
		     S5K4ECGX_write_cmos_sensor_wID(pSensorRegData_wID->RegAddr, pSensorRegData_wID->RegData, pSensorRegData_wID->ID);
		     break;
		case SENSOR_FEATURE_GET_REGISTER_W_ID:
		     pSensorRegData_wID->RegData = S5K4ECGX_read_cmos_sensor_wID(pSensorRegData_wID->RegAddr, pSensorRegData_wID->ID);
		     break;
*/		     
		case SENSOR_FEATURE_GET_EV_AWB_REF:
		    S5K4ECGXGetEvAwbRef(*pFeatureData32);
		    break;
		
		case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
		    S5K4ECGXGetCurAeAwbInfo(*pFeatureData32);
		    break;
        case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
            S5K4ECGX_Get_AF_Max_Num_Focus_Areas(pFeatureReturnPara32);            
            *pFeatureParaLen=4;
            break;        
        case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
            S5K4ECGX_Get_AE_Max_Num_Metering_Areas(pFeatureReturnPara32);            
            *pFeatureParaLen=4;
            break;

#ifdef S5K4ECGXYUV_SUPPORT_N3D
        case SENSOR_FEATURE_SET_SLAVE_I2C_ID:
            //SENSORDB("SENSOR_FEATURE_SET_SLAVE_I2C_ID:[%d][%d][%d]\n",*pFeatureData32,\
            //                    *(pFeatureData32+1),S5K4ECGXYUV_sensor_slave_I2C_ID_ready); 

            S5K4ECGXYUV_sensor_socket = *pFeatureData32;
            if ( DUAL_CAMERA_MAIN_SENSOR == S5K4ECGXYUV_sensor_socket ) {
                S5K4ECGXYUV_sensor_write_I2C_address = S5K4ECGX_WRITE_ID;
                S5K4ECGXYUV_sensor_read_I2C_address = S5K4ECGX_READ_ID;
            }
            else if ( DUAL_CAMERA_MAIN_2_SENSOR == S5K4ECGXYUV_sensor_socket ) {
                S5K4ECGXYUV_sensor_write_I2C_address = 0x7A;
                S5K4ECGXYUV_sensor_read_I2C_address = 0x7B;
            }
            break;
/*
        case SENSOR_FEATURE_SUSPEND:
            //js_test
            //S5K4ECGXYUV_write_cmos_sensor(0x3008, 0x42);
            SENSORDB("[js_test]:E SENSOR_FEATURE_SUSPEND \n");
            iWriteReg((u16) 0x3008 , (u32) 0x42 , 1, 0x78);
            iWriteReg((u16) 0x3008 , (u32) 0x42 , 1, 0x70);
            SENSORDB("[js_test]:X SENSOR_FEATURE_SUSPEND \n");
            break;
        case SENSOR_FEATURE_RESUME:
            //js_test
            //S5K4ECGXYUV_write_cmos_sensor(0x3008, 0x02);
            SENSORDB("[js_test]:E SENSOR_FEATURE_RESUME \n");
            iWriteReg((u16) 0x3008 , (u32) 0x02 , 1, 0x78);
            iWriteReg((u16) 0x3008 , (u32) 0x02 , 1, 0x70);
            SENSORDB("[js_test]:X SENSOR_FEATURE_RESUME \n");
            break;
*/
#else
        case SENSOR_FEATURE_SET_SLAVE_I2C_ID:
            //SENSORDB("SENSOR_FEATURE_SET_SLAVE_I2C_ID:[%d]\n",*pFeatureData32); 
            S5K4ECGXYUV_sensor_socket = *pFeatureData32;
            break;

#endif

		default:
			 break;			
	}

	return ERROR_NONE;
}	/* S5K4ECGXFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncS5K4ECGX=
{
	S5K4ECGXOpen,             // get sensor id, set initial setting to sesnor
	S5K4ECGXGetInfo,          // get sensor capbility, 
	S5K4ECGXGetResolution,    // get sensor capure/preview resolution
	S5K4ECGXFeatureControl,   // set shutter/gain, set/read register
	S5K4ECGXControl,          // change mode to preview/capture/video
	S5K4ECGXClose             // close, do nothing currently
};

UINT32 S5K4ECGX_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncS5K4ECGX;

	return ERROR_NONE;
}	/* SensorInit() */





