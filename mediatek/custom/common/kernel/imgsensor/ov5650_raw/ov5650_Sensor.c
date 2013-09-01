
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
#include <asm/atomic.h>
#include <linux/slab.h>
#include <asm/system.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov5650_Sensor.h"
#include "ov5650_Camera_Sensor_para.h"
#include "ov5650_CameraCustomized.h"

#define OV5650_DEBUG
#ifdef OV5650_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif
#define OV5650_SENSOR_ID                        0x5651

//extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);



kal_uint16 OV5650_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char puSendCmd[3] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 3,OV5650_WRITE_ID);

}
kal_uint16 OV5650_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,OV5650_WRITE_ID);
	
    return get_byte;
}

//e_porting add
//e_porting add
//e_porting add

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
//#include "ov5650_Sensor.h"
//#include "ov5650_camera_sensor_para.h"
//#include "CameraCustomized.h"
static kal_uint16 OV5650_g_iBackupExtraExp = 0;
static kal_bool OV5650_IsNightMode = KAL_FALSE;
//#define __OV_CMX_RAW8__
#define OV5650__OV_CIP_RAW8__
#define OV5650__DENOISE_MANUAL__

//extern camcorder_info_struct OV5650_dsc_support_info;
extern kal_bool	aaa_ae_bypass_flag;
extern kal_bool	aaa_awb_bypass_flag;


//static float OV5650_g_fPV_PCLK = 52 * 1000000;//24
static kal_uint32  OV5650_g_fPV_PCLK = 52 * 1000000;//24
//static float OV5650_g_fCP_PCLK = 52 * 1000000;
static kal_uint32 OV5650_g_fCP_PCLK = 52 * 1000000;




//SensorInfo OV5650_g_CCT_MainSensor = OV5650_CURRENT_MAIN_SENSOR;
//kal_uint8 OV5650_g_CCT_FirstGrabColor = OV5650_FIRST_GRAB_COLOR;
static kal_bool OV5650_g_bXGA_Mode = KAL_TRUE;
static kal_uint16 OV5650_g_iExpLines = 0, OV5650_g_iExtra_ExpLines = 0;
OV5650_OP_TYPE OV5650_g_iOV5650_Mode = OV5650_MODE_NONE;
kal_uint8 OV5650_MIN_EXPOSURE_LINES = 1;
kal_uint16 OV5650_MAX_EXPOSURE_LINES = OV5650_PV_EXPOSURE_LIMITATION;
kal_uint16 OV5650_g_iDummyLines = 0;
//added by mandrave
kal_uint16 OV5650_iDummypixels = 555;
//added end
kal_uint16 OV5650_g_iPV_Pixels_Per_Line = 0;

//static kal_uint16 OV5650_g_iBackupExtraExp = 0;

//#define __OV_CMX_RAW8__
//#define __OV_CIP_RAW8__
//#define __DENOISE_MANUAL__

kal_uint32 OV5650_isp_master_clock;

static kal_bool OV5650_g_Fliker_Mode = KAL_FALSE;
static kal_bool OV5650_g_ZSD_Mode = KAL_FALSE;
MSDK_SCENARIO_ID_ENUM CurrentScenarioId = ACDK_SCENARIO_ID_CAMERA_PREVIEW;



//static kal_bool OV5650_g_bXGA_Mode = KAL_TRUE;
//static kal_uint16 OV5650_g_iExpLines = 0,OV5650_g_iExtra_ExpLines = 0;
//OV5650_OP_TYPE g_iOV5650_Mode = OV5650_MODE_NONE;
//kal_uint8 OV5650_MIN_EXPOSURE_LINES = 1;
//kal_uint16 OV5650_MAX_EXPOSURE_LINES = PV_EXPOSURE_LIMITATION;
//kal_uint16 OV5650_g_iDummyLines = 0,OV5650_iDummyPixels=0;

//kal_uint16 OV5650_g_iPV_PCLK_Divider = 0;
//kal_uint16 OV5650_g_iPV_Pixels_Per_Line = 0;

// kal_uint32 OV5650_g_fPV_PCLK = 28166666;
//static float OV5650_g_fPV_PCLK =  47666666;


//kal_bool OV5650_MPEG4_encode_mode = KAL_FALSE;
kal_uint32 OV5650_FAC_SENSOR_REG;
//kal_uint8 OV5650_sensor_write_I2C_address = OV5650_WRITE_ID;
//kal_uint8 OV5650_sensor_read_I2C_address = OV5650_READ_ID;

//HANDLE OV5650hDrvI2C;
//I2C_TRANSACTION OV5650I2CConfig;

UINT8 OV5650PixelClockDivider=0;

SENSOR_REG_STRUCT OV5650SensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT OV5650SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;

MSDK_SENSOR_CONFIG_STRUCT OV5650SensorConfigData;


static DEFINE_SPINLOCK(ov5650_drv_lock);



#if defined(OV5650_USE_OTP)
//index:index of otp group.(0,1,2)
//return:	0:group index is empty.
//		1.group index has invalid data
	//		2.group index has valid data

kal_uint16 ov5650_check_otp_wb(kal_uint16 index)
{
	kal_uint16 temp,flag;
	kal_uint32 address;

	//read flag
	address = 0x05+index*9;
	OV5650_write_cmos_sensor(0x3d00,address);
	flag = OV5650_read_cmos_sensor(0x3d04);

	//SENSORDB("ov5650_check_otp_wb=%d\r\n",flag);

	OV5650_write_cmos_sensor(0x3d00,0x0);

	if(NULL == flag)
		{
			
			SENSORDB("[ov5650_check_otp_awb]index[%x]read flag[%x][0]\n",index,flag);
			return 0;
			
		}
	else if(!(flag&0x80) && (flag&0x7f))
		{
			SENSORDB("[ov5650_check_otp_awb]index[%x]read flag[%x][2]\n",index,flag);
			return 2;
		}
	else
		{
			SENSORDB("[ov5650_check_otp_awb]index[%x]read flag[%x][1]\n",index,flag);
		    return 1;
		}
	
}


//index:index of otp group.(0,1,2)
//return:	0.group index is empty.
//		1.group index has invalid data
//		2.group index has valid data

kal_uint16 ov5650_check_otp_lenc(kal_uint16 index)
{
   kal_uint16 temp,flag;
   kal_uint32 address;

   address = 0x20 + index*71;
   OV5650_write_cmos_sensor(0x3d00,address);
   
   flag = OV5650_read_cmos_sensor(0x3d04);
   flag = flag & 0xc0;

   OV5650_write_cmos_sensor(0x3d00,0);

   if(NULL == flag)
   	{
   		SENSORDB("[ov5650_check_otp_lenc]index[%x]read flag[%x][0]\n",index,flag);
   	    return 0;
   	}
   else if(0x40 == flag)
   	{
   		SENSORDB("[ov5650_check_otp_lenc]index[%x]read flag[%x][2]\n",index,flag);
   	    return 2;
   	}
   else
   	{
   		SENSORDB("[ov5650_check_otp_lenc]index[%x]read flag[%x][1]\n",index,flag);
		return 1;
   	}
}

//index:index of otp group.(0,1,2)
//return: 0
kal_uint16 ov5650_read_otp_wb(kal_uint16 index, struct ov5650_otp_struct *otp)
{
	kal_uint16 temp;
	kal_uint32 address;

	address = 0x05 +index*9;

	////4 modified the start address
	//address = 0x05 + index*9 +1;

	OV5650_write_cmos_sensor(0x3d00,address);
	otp->module_integrator_id = (OV5650_read_cmos_sensor(0x3d04)&0x7f);
	SENSORDB("[ov5650_read_otp_wb]address[%x]module_integrator_id[%x]\n",address,otp->module_integrator_id);
	
	address++;
	OV5650_write_cmos_sensor(0x3d00,address);
	otp->lens_id = OV5650_read_cmos_sensor(0x3d04);
	SENSORDB("[ov5650_read_otp_wb]address[%x]lens_id[%x]\n",address,otp->lens_id);
	
	address++;
	OV5650_write_cmos_sensor(0x3d00,address);
	otp->rg_ratio = OV5650_read_cmos_sensor(0x3d04);
	SENSORDB("[ov5650_read_otp_wb]address[%x]rg_ratio[%x]\n",address,otp->rg_ratio);

	address++;
	OV5650_write_cmos_sensor(0x3d00,address);
	otp->bg_ratio = OV5650_read_cmos_sensor(0x3d04);
	SENSORDB("[ov5650_read_otp_wb]address[%x]bg_ratio[%x]\n",address,otp->bg_ratio);

	address++;
	OV5650_write_cmos_sensor(0x3d00,address);
	otp->user_data[0] = OV5650_read_cmos_sensor(0x3d04);
	SENSORDB("[ov5650_read_otp_wb]address[%x]user_data[0][%x]\n",address,otp->user_data[0]);

	address++;
	OV5650_write_cmos_sensor(0x3d00,address);
	otp->user_data[1] = OV5650_read_cmos_sensor(0x3d04);
	SENSORDB("[ov5650_read_otp_wb]address[%x]user_data[1][%x]\n",address,otp->user_data[1]);

	address++;
	OV5650_write_cmos_sensor(0x3d00,address);
	otp->user_data[2] = OV5650_read_cmos_sensor(0x3d04);	
	SENSORDB("[ov5650_read_otp_wb]address[%x]user_data[2][%x]\n",address,otp->user_data[2]);

	address++;
	OV5650_write_cmos_sensor(0x3d00,address);
	otp->user_data[3] = OV5650_read_cmos_sensor(0x3d04);	
	SENSORDB("[ov5650_read_otp_wb]address[%x]user_data[3][%x]\n",address,otp->user_data[3]);

	////4 modified the start address
    
	address++;
	OV5650_write_cmos_sensor(0x3d00,address);
	otp->user_data[4] = OV5650_read_cmos_sensor(0x3d04);
	SENSORDB("[ov5650_read_otp_wb]address[%x]user_data[4][%x]\n",address,otp->user_data[4]);

	OV5650_write_cmos_sensor(0x3d00,00);

	return 0;
	
	
}

kal_uint16 ov5650_read_otp_lenc(kal_uint16 index,struct ov5650_otp_struct *otp)
{
	kal_uint16 bank,temp1,temp2,i;
	kal_uint32 address;

	address = 0x20 + index*71 +1;
	
	

	//read lenc_g
	for(i = 0; i < 36; i++)
		{
			OV5650_write_cmos_sensor(0x3d00,address);
			otp->lenc_g[i] = OV5650_read_cmos_sensor(0x3d04);
			
			SENSORDB("[ov5650_read_otp_lenc]address[%x]otp->lenc_g[%d][%x]\n",address,i,otp->lenc_g[i]);
			address++;
		}
	//read lenc_b
	for(i = 0; i <8; i++)
		{
			OV5650_write_cmos_sensor(0x3d00,address);
			temp1 = OV5650_read_cmos_sensor(0x3d04);

			SENSORDB("[ov5650_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);

			address++;
			OV5650_write_cmos_sensor(0x3d00,address);
			temp2 = OV5650_read_cmos_sensor(0x3d04);

			SENSORDB("[ov5650_read_otp_lenc]address[%x]temp2[%x]\n",address,temp2);

			address++;

			otp->lenc_b[i*3] = temp1&0x1f;
			otp->lenc_b[i*3+1] = temp2&0x1f;
			otp->lenc_b[i*3+2] = (((temp1 >> 2)&0x18) | (temp2 >> 5));
		}
	OV5650_write_cmos_sensor(0x3d00,address);
	temp1 = OV5650_read_cmos_sensor(0x3d04);
	SENSORDB("[ov5650_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);
	otp->lenc_b[24] = temp1&0x1f;
	address++;

	//read lenc_r
	for(i = 0; i <8; i++)
		{
		   OV5650_write_cmos_sensor(0x3d00,address);
		   temp1 = OV5650_read_cmos_sensor(0x3d04);

		   SENSORDB("[ov5650_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);
		   
		   address++;

		   OV5650_write_cmos_sensor(0x3d00,address);
		   temp2 = OV5650_read_cmos_sensor(0x3d04);
		   
			
		   SENSORDB("[ov5650_read_otp_lenc]address[%x]temp2[%x]\n",address,temp2);
		   address++;

		   otp->lenc_r[i*3] = temp1&0x1f;
		   otp->lenc_r[i*3+1] = temp2&0x1f;
		   otp->lenc_r[i*3+2] = (((temp1 >> 2)&0x18) | (temp2 >>5));
		}
	OV5650_write_cmos_sensor(0x3d00,address);
	temp1 = OV5650_read_cmos_sensor(0x3d04);
	SENSORDB("[ov5650_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);
	otp->lenc_r[24] = temp1 & 0x1f;

	OV5650_write_cmos_sensor(0x3d00,0);

	return 0;
}

//R_gain: red gain of sensor AWB, 0x400 = 1
//G_gain: green gain of sensor AWB, 0x400 = 1
//B_gain: blue gain of sensor AWB, 0x400 = 1
//reutrn 0
kal_uint16 ov5650_update_wb_gain(kal_uint32 R_gain, kal_uint32 G_gain, kal_uint32 B_gain)
{

    SENSORDB("[ov5650_update_wb_gain]R_gain[%x]G_gain[%x]B_gain[%x]\n",R_gain,G_gain,B_gain);

	if(R_gain > 0x400)
		{
			OV5650_write_cmos_sensor(0x3400,R_gain >> 8);
			OV5650_write_cmos_sensor(0x3401,(R_gain&0x00ff));
		}
	if(G_gain > 0x400)
		{
			OV5650_write_cmos_sensor(0x3402,G_gain >> 8);
			OV5650_write_cmos_sensor(0x3403,(G_gain&0x00ff));
		}
	if(B_gain >0x400)
		{
			OV5650_write_cmos_sensor(0x3404,B_gain >> 8);
			OV5650_write_cmos_sensor(0x3405,(B_gain&0x00ff));
		}
	return 0;
}


//return 0
kal_uint16 ov5650_update_lenc(struct ov5650_otp_struct *otp)
{
	kal_uint16 i, temp;
	//lenc g
	for(i = 0; i < 36; i++)
		{
		 	OV5650_write_cmos_sensor(0x5800+i,otp->lenc_g[i]);
			
			SENSORDB("[ov5650_update_lenc]otp->lenc_g[%d][%x]\n",i,otp->lenc_g[i]);
		}
	//lenc b
	for(i = 0; i < 25; i++)
		{
			OV5650_write_cmos_sensor(0x5824+i,otp->lenc_b[i]);
			SENSORDB("[ov5650_update_lenc]otp->lenc_b[%d][%x]\n",i,otp->lenc_b[i]);
		}
	//lenc r
	for(i = 0; i < 25; i++)
		{
			OV5650_write_cmos_sensor(0x583d+i,otp->lenc_r[i]);
			SENSORDB("[ov5650_update_lenc]otp->lenc_r[%d][%x]\n",i,otp->lenc_r[i]);
		}
	return 0;
}

//R/G and B/G ratio of typical camera module is defined here

kal_uint32 RG_Ratio_typical = RG_TYPICAL;
kal_uint32 BG_Ratio_typical = BG_TYPICAL;

//call this function after OV5650 initialization
//return value:	0 update success
//				1 no 	OTP

kal_uint16 ov5650_update_wb_register_from_otp(void)
{
	kal_uint16 temp, i, otp_index;
	struct ov5650_otp_struct current_otp;
	kal_uint32 R_gain, B_gain, G_gain, G_gain_R,G_gain_B;

	SENSORDB("ov5650_update_wb_register_from_otp\n");

	//update white balance setting from OTP
	//check first wb OTP with valid OTP
	for(i = 0; i < 3; i++)
		{
			temp = ov5650_check_otp_wb(i);
			if(temp == 2)
				{
					otp_index = i;
					break;
				}
		}
	if( 3 == i)
		{
		 	SENSORDB("[ov5650_update_wb_register_from_otp]no valid wb OTP data!\r\n");
			return 1;
		}
	ov5650_read_otp_wb(otp_index,&current_otp);

	//calculate gain
	//0x400 = 1x gain
	if(current_otp.bg_ratio < BG_Ratio_typical)
		{
			if(current_otp.rg_ratio < RG_Ratio_typical)
				{
					//current_opt.bg_ratio < BG_Ratio_typical &&
					//cuttent_otp.rg < RG_Ratio_typical

					G_gain = 0x400;
					B_gain = 0x400 * BG_Ratio_typical / current_otp.bg_ratio;
					R_gain = 0x400 * RG_Ratio_typical / current_otp.rg_ratio;
				}
			else
				{
					//current_otp.bg_ratio < BG_Ratio_typical &&
			        //current_otp.rg_ratio >= RG_Ratio_typical
			        R_gain = 0x400;
					G_gain = 0x400 * current_otp.rg_ratio / RG_Ratio_typical;
					B_gain = G_gain * BG_Ratio_typical / current_otp.bg_ratio;
					
			        
				}
		}
	else
		{
			if(current_otp.rg_ratio < RG_Ratio_typical)
				{
					//current_otp.bg_ratio >= BG_Ratio_typical &&
			        //current_otp.rg_ratio < RG_Ratio_typical
			        B_gain = 0x400;
					G_gain = 0x400 * current_otp.bg_ratio / BG_Ratio_typical;
					R_gain = G_gain * RG_Ratio_typical / current_otp.rg_ratio;
					
				}
			else
				{
					//current_otp.bg_ratio >= BG_Ratio_typical &&
			        //current_otp.rg_ratio >= RG_Ratio_typical
			        G_gain_B = 0x400*current_otp.bg_ratio / BG_Ratio_typical;
				    G_gain_R = 0x400*current_otp.rg_ratio / RG_Ratio_typical;
					
					if(G_gain_B > G_gain_R)
						{
							B_gain = 0x400;
							G_gain = G_gain_B;
							R_gain = G_gain * RG_Ratio_typical / current_otp.rg_ratio;
						}
					else

						{
							R_gain = 0x400;
							G_gain = G_gain_R;
							B_gain = G_gain * BG_Ratio_typical / current_otp.bg_ratio;
						}
			        
				}
			
		}
	//write sensor wb gain to register
	ov5650_update_wb_gain(R_gain,G_gain,B_gain);

	//success
	return 0;
}

//call this function after ov5650 initialization
//return value:	0 update success
//				1 no otp

kal_uint16 ov5650_update_lenc_register_from_otp(void)
{
	kal_uint16 temp,i,otp_index;
    struct ov5650_otp_struct current_otp;

	for(i = 0; i < 3; i++)
		{
			temp = ov5650_check_otp_lenc(i);
			if(2 == temp)
				{
					otp_index = i;
					break;
				}
		}
	if(3 == i)
		{
		 	SENSORDB("[ov5650_update_lenc_register_from_otp]no valid wb OTP data!\r\n");
			return 1;
		}
	ov5650_read_otp_lenc(otp_index,&current_otp);

	ov5650_update_lenc(&current_otp);

	//at last should enable the shading enable register
	OV5650_read_cmos_sensor(0x5000);
	temp |= 0x80;
	OV5650_write_cmos_sensor(0x5000,temp);
	

	//success
	return 0;
}



#endif



static void OV5650_Write_Shutter(const kal_uint16 iShutter)
{

    kal_uint16 iExp = iShutter;
	kal_uint16 Total_line;
	kal_uint16 Extra_ExpLines = 0;
	kal_uint16 realtime_fp = 0;
	kal_uint32 pclk = 0;
	kal_uint16 line_length = 0;
	//kal_uint16 frame_length = 0;
	unsigned long flags;
//OV5650_g_iExtra_ExpLines  = total lines
	Total_line = (OV5650_read_cmos_sensor(0x380e) & 0x0f)<<8;
	Total_line |= OV5650_read_cmos_sensor(0x380f);
	//OV5650_g_iDummyLines = Total_line - OV5650_PV_EXPOSURE_LIMITATION;
	SENSORDB("ov5650_write_shutter ishutter=%d\r\n",iShutter);
	SENSORDB("ov5650_write_shutter total_line=%d\r\n",Total_line);

	if (OV5650_g_bXGA_Mode) {
        if (iExp <= OV5650_PV_EXPOSURE_LIMITATION + OV5650_g_iDummyLines) {
            Extra_ExpLines = OV5650_PV_EXPOSURE_LIMITATION + OV5650_g_iDummyLines;

			SENSORDB("bxga little iExpr\n");
			SENSORDB("OV5650_g_iDummyLines %d\r\n",OV5650_g_iDummyLines);
        }else {
            Extra_ExpLines = iExp+28;//; - OV5650_PV_EXPOSURE_LIMITATION - OV5650_g_iDummyLines;
			//iExp = OV5650_PV_EXPOSURE_LIMITATION + OV5650_g_iDummyLines;
			SENSORDB("bxga large iExp\n");
			SENSORDB("OV5650_g_iExtra_ExpLines %d\r\n",Extra_ExpLines);
        }

    }else {
        if (iExp <= OV5650_FULL_EXPOSURE_LIMITATION + OV5650_g_iDummyLines) {
            Extra_ExpLines = OV5650_FULL_EXPOSURE_LIMITATION+ OV5650_g_iDummyLines;
			SENSORDB("!bxga little iExp\n");
			SENSORDB("OV5650_g_iDummyLines %d\r\n",OV5650_g_iDummyLines);

        }else {
            Extra_ExpLines = iExp+28;// - OV5650_FULL_EXPOSURE_LIMITATION - OV5650_g_iDummyLines;
			//iExp = OV5650_FULL_EXPOSURE_LIMITATION + OV5650_g_iDummyLines;
			SENSORDB("!bxga large iExp\n");
			SENSORDB("OV5650_g_iExtra_ExpLines %d\r\n",Extra_ExpLines);

        }
    }

	if(KAL_TRUE == OV5650_g_Fliker_Mode)
		{
			if(KAL_TRUE == OV5650_g_ZSD_Mode)
				{
					pclk = OV5650_g_fCP_PCLK;
					line_length = OV5650_QXGA_MODE_WITHOUT_DUMMY_PIXELS+ OV5650_iDummypixels; 
					//frame_length = Extra_ExpLines; //OV5650_QXGA_MODE_WITHOUT_DUMMY_LINES + OV5650_g_iDummyLines;
				}
			else
				{
					pclk = OV5650_g_fPV_PCLK;
					line_length = OV5650_XGA_MODE_WITHOUT_DUMMY_PIXELS + OV5650_iDummypixels;  
					//frame_length = Extra_ExpLines; //OV5650_XGA_MODE_WITHOUT_DUMMY_LINES + OV5650_g_iDummyLines;
				}
			realtime_fp = pclk*10 / (line_length *Extra_ExpLines);

			SENSORDB("[OV5650_Write_Shutter]pv_clk:%d\n",pclk);
			SENSORDB("[OV5650_Write_Shutter]line_length:%d\n",line_length);
			//SENSORDB("[OV5650_Write_Shutter]frame_height:%d\n",frame_length);
			SENSORDB("[OV5650_Write_Shutter]framerate(10base):%d\n",realtime_fp);

			if((realtime_fp >= 297)&&(realtime_fp <=303))
				{
					realtime_fp = 296;
					Extra_ExpLines = pclk*10 /(line_length *realtime_fp);
					SENSORDB("[autofliker realtime_fp=30,extern heights slowdown to 29.6fps][height:%d]",Extra_ExpLines);
				}
			else if((realtime_fp >=147)&&(realtime_fp <=153))
				{
					realtime_fp = 146;

					Extra_ExpLines = pclk*10 /(line_length *realtime_fp);

					SENSORDB("[autofliker realtime_fp=15,extern heights slowdown to 14.6fps][height:%d]",Extra_ExpLines);
				}

			
		}

	OV5650_write_cmos_sensor(0x380e, Extra_ExpLines >> 8);
	OV5650_write_cmos_sensor(0x380f, Extra_ExpLines & 0x00FF);
	
	OV5650_write_cmos_sensor(0x3500, (iExp>> 12) & 0xFF);
	OV5650_write_cmos_sensor(0x3501, (iExp >> 4) & 0xFF);
	
	OV5650_write_cmos_sensor(0x3502, (iExp<<4) & 0xFF);

    spin_lock_irqsave(&ov5650_drv_lock,flags);
	OV5650_g_iExtra_ExpLines = Extra_ExpLines;
	OV5650_g_iExpLines = iShutter;
	spin_unlock_irqrestore(&ov5650_drv_lock,flags);


}   /*  OV5650_Write_Shutter    */

static void OV5650_Set_Dummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{

//kal_uint16 iPCLKs_inOneLine = OV5650_QXGA_MODE_WITHOUT_DUMMY_PIXELS, iTotalLines;
kal_uint16 iTotalLines,iTotapixels;
// Add dummy pixels:
// 0x3028, 0x3029 defines the PCLKs in one line of OV5650
// If [0x3028:0x3029] = N, the total PCLKs in one line of QXGA(3M full mode) is (N+1), and
// total PCLKs in one line of XGA subsampling mode is (N+1) / 2
// If need to add dummy pixels, just increase 0x3028 and 0x3029 directly

//added by mandrave 
spin_lock(&ov5650_drv_lock);
  OV5650_iDummypixels = iPixels;
  OV5650_g_iDummyLines = iLines;
  spin_unlock(&ov5650_drv_lock);
//added end

if(OV5650_g_bXGA_Mode == KAL_TRUE)
	{
	iTotalLines=OV5650_XGA_MODE_WITHOUT_DUMMY_LINES + iLines;
	iTotapixels=OV5650_XGA_MODE_WITHOUT_DUMMY_PIXELS+ iPixels;
	}
else
	{
	iTotalLines=OV5650_QXGA_MODE_WITHOUT_DUMMY_LINES + iLines;
	iTotapixels=OV5650_QXGA_MODE_WITHOUT_DUMMY_PIXELS+ iPixels;
	}
OV5650_write_cmos_sensor(0x380c, (iTotapixels >> 8) & 0xFF);
OV5650_write_cmos_sensor(0x380d, iTotapixels & 0xFF);

//Set dummy lines.
//The maximum shutter value = Line_Without_Dummy + Dummy_lines

SENSORDB("\r\nmandrave preview set iTotalLines %d\r\n",iTotalLines);
SENSORDB("\r\nmandrave preview set OV5650_g_iExtra_ExpLines %d\r\n",OV5650_g_iExtra_ExpLines);

if(OV5650_g_iExtra_ExpLines > iTotalLines)
{
    iTotalLines = OV5650_g_iExtra_ExpLines;
}
OV5650_write_cmos_sensor(0x380e, (iTotalLines >> 8) & 0xFF);
OV5650_write_cmos_sensor(0x380f, iTotalLines & 0xFF);


}   /*  OV5650_Set_Dummy    */

/*************************************************************************
* FUNCTION
*	OV5650_SetShutter
*
* DESCRIPTION
*	This function set e-shutter of OV5650 to change exposure time.
*
* PARAMETERS
*   iShutter : exposured lines
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void set_OV5650_shutter(kal_uint16 iShutter)
{
   
    OV5650_Write_Shutter(iShutter);

}   /*  Set_OV5650_Shutter */

static kal_uint16 OV5650_Reg2Gain(const kal_uint8 iReg)
{
    kal_uint8 iI;
    kal_uint16 iGain = BASEGAIN;    // 1x-gain base

    // Range: 1x to 32x
    // Gain = (GAIN[7] + 1) * (GAIN[6] + 1) * (GAIN[5] + 1) * (GAIN[4] + 1) * (1 + GAIN[3:0] / 16)
    for (iI = 7; iI >= 4; iI--) {
        iGain *= (((iReg >> iI) & 0x01) + 1);
    }

    return iGain +  iGain * (iReg & 0x0F) / 16;
}


		static kal_uint8 OV5650_Gain2Reg(const kal_uint16 iGain)
		{
			kal_uint8 iReg = 0x00;
		
			if (iGain < 2 * BASEGAIN) {
				// Gain = 1 + GAIN[3:0](0x00) / 16
				iReg = 16 * (iGain - BASEGAIN) / BASEGAIN;
			}else if (iGain < 4 * BASEGAIN) {
				// Gain = 2 * (1 + GAIN[3:0](0x00) / 16)
				iReg |= 0x10;
				iReg |= 8 * (iGain - 2 * BASEGAIN) / BASEGAIN;
			}else if (iGain < 8 * BASEGAIN) {
				// Gain = 4 * (1 + GAIN[3:0](0x00) / 16)
				iReg |= 0x30;
				iReg |= 4 * (iGain - 4 * BASEGAIN) / BASEGAIN;
			}else if (iGain < 16 * BASEGAIN) {
				// Gain = 8 * (1 + GAIN[3:0](0x00) / 16)
				iReg |= 0x70;
				iReg |= 2 * (iGain - 8 * BASEGAIN) / BASEGAIN;
			}else if (iGain < 32 * BASEGAIN) {
				// Gain = 16 * (1 + GAIN[3:0](0x00) / 16)
				iReg |= 0xF0;
				iReg |= (iGain - 16 * BASEGAIN) / BASEGAIN;
			}else {
				ASSERT(0);
			}
		  SENSORDB("Sensor OV5650 Gain2Reg ireg=%x\n",iReg);
			return iReg;
		}


/*************************************************************************
* FUNCTION
*	OV5650_SetGain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*   iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 OV5650_SetGain(kal_uint16 iGain)

{
	SENSORDB("Sensor OV5650 SetGain! %d\n",iGain);

const kal_uint16 iBaseGain = 64;//OV5650_Reg2Gain(camera_para.SENSOR.cct[OV5650_INDEX_BASE_GAIN].para);
    const kal_uint16 iGain2Set = iBaseGain * iGain / BASEGAIN;
    const kal_uint8 iReg = OV5650_Gain2Reg(iGain2Set);

    OV5650_write_cmos_sensor(0x350b, iReg);//OV5650_write_cmos_sensor(0x3001, iReg);

    // 0x302D, 0x302E will increase VBLANK to get exposure larger than frame exposure
    // limitation. 0x302D, 0x302E must update at the same frame as sensor gain update.
   // OV5650_write_cmos_sensor(0x302D, OV5650_g_iExtra_ExpLines >> 8);
   // OV5650_write_cmos_sensor(0x302E, OV5650_g_iExtra_ExpLines & 0x00FF);

    return OV5650_Reg2Gain(iReg) * BASEGAIN / iBaseGain;

}

/*************************************************************************
* FUNCTION
*	OV5650_NightMode
*
* DESCRIPTION
*	This function night mode of OV5650.
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
#if 0
void OV5650_night_mode(kal_bool bEnable)
{
#if 1
 SENSORDB("ov5650_nightmode begin\r\n");

 kal_uint16 dummy_lines = 0;
 kal_uint16 iTotalLines = 0;

 // Impelement night mode function to fix video frame rate.
OV5650_IsNightMode = bEnable;
if (bEnable) //Night mode
 {
	 if (OV5650_g_iOV5650_Mode == OV5650_MODE_CIF_VIDEO || OV5650_g_iOV5650_Mode == OV5650_MODE_QCIF_VIDEO)
	 {
		 OV5650_MAX_EXPOSURE_LINES = OV5650_g_fPV_PCLK / OV5650_VIDEO_NIGHTMODE_FRAME_RATE / OV5650_g_iPV_Pixels_Per_Line;

	 }
 }
 else			 //Normal mode
 {
	 if (OV5650_g_iOV5650_Mode == OV5650_MODE_CIF_VIDEO || OV5650_g_iOV5650_Mode == OV5650_MODE_QCIF_VIDEO)
	 {
		 OV5650_MAX_EXPOSURE_LINES = OV5650_g_fPV_PCLK / OV5650_VIDEO_NORMALMODE_FRAME_RATE / OV5650_g_iPV_Pixels_Per_Line;
	
	 }
 }

 // Just video need.
 if (OV5650_g_iOV5650_Mode == OV5650_MODE_CIF_VIDEO || OV5650_g_iOV5650_Mode == OV5650_MODE_QCIF_VIDEO)
 {
	 if (OV5650_MAX_EXPOSURE_LINES >= OV5650_PV_PERIOD_LINE_NUMS)
	 {
		 OV5650_g_iDummyLines = OV5650_MAX_EXPOSURE_LINES - OV5650_PV_PERIOD_LINE_NUMS;
	 }
	 else
		 OV5650_g_iDummyLines = 0;

	 //Set dummy lines.
	 //iTotalLines = (OV5650_read_cmos_sensor(0x302A) << 8) + OV5650_read_cmos_sensor(0x302B) + OV5650_g_iDummyLines;
	 //OV5650_write_cmos_sensor(0x302A, iTotalLines >> 8);
	 //OV5650_write_cmos_sensor(0x302B, iTotalLines & 0x00FF);
	 //iTotalLines = ((OV5650_read_cmos_sensor(0x380e)&0x0f) << 8) + OV5650_read_cmos_sensor(0x380f); //+ OV5650_g_iDummyLines;
	 iTotalLines= (OV5650_read_cmos_sensor(0x380e) & 0x0f)<<8;
	 iTotalLines |= OV5650_read_cmos_sensor(0x380f);
	 iTotalLines += OV5650_g_iDummyLines;
	 
	 OV5650_write_cmos_sensor(0x380e, iTotalLines >> 8);
	 OV5650_write_cmos_sensor(0x380f, iTotalLines & 0x00FF);
	 
 }
 SENSORDB("ov5650_nightmode end\r\n");


#else
	  kal_uint16 iTotalLines = 0;
  
	// Impelement night mode function to fix video frame rate.
    if (bEnable)	//Night mode
    {	
    	if(OV5650_MPEG4_encode_mode ==KAL_TRUE)
    	{
			OV5650_MAX_EXPOSURE_LINES = OV5650_g_fPV_PCLK / OV5650_VIDEO_NIGHTMODE_FRAME_RATE / OV5650_g_iPV_Pixels_Per_Line;
			
		   if (OV5650_MAX_EXPOSURE_LINES >= PV_PERIOD_LINE_NUMS)		  
			OV5650_g_iDummyLines = OV5650_MAX_EXPOSURE_LINES - PV_PERIOD_LINE_NUMS; 
		   else
			OV5650_g_iDummyLines = 0;
	
		  //Set dummy lines.
	     // iTotalLines = (OV5650_read_cmos_sensor(0x302A) << 8) + OV5650_read_cmos_sensor(0x302B) + OV5650_g_iDummyLines;
	      iTotalLines = PV_PERIOD_LINE_NUMS + OV5650_g_iDummyLines;
	      OV5650_write_cmos_sensor(0x302A, iTotalLines >> 8);
	      OV5650_write_cmos_sensor(0x302B, iTotalLines & 0x00FF);
	     // OV5650_write_cmos_sensor(0x302A, 0x62C>> 8);
	    //  OV5650_write_cmos_sensor(0x302B, 0x62C & 0x00FF);
    	}			
	}
	else			//Normal mode
	{
		if(OV5650_MPEG4_encode_mode==KAL_TRUE)	
		{
			OV5650_MAX_EXPOSURE_LINES = OV5650_g_fPV_PCLK / OV5650_VIDEO_NORMALMODE_FRAME_RATE / OV5650_g_iPV_Pixels_Per_Line;
			
		   if (OV5650_MAX_EXPOSURE_LINES >= PV_PERIOD_LINE_NUMS)		  
		     OV5650_g_iDummyLines = OV5650_MAX_EXPOSURE_LINES - PV_PERIOD_LINE_NUMS; 
		    else
			OV5650_g_iDummyLines = 0;
	
		   //Set dummy lines.
	      //  iTotalLines = (OV5650_read_cmos_sensor(0x302A) << 8) + OV5650_read_cmos_sensor(0x302B) + OV5650_g_iDummyLines;
	        iTotalLines = PV_PERIOD_LINE_NUMS + OV5650_g_iDummyLines;
	       OV5650_write_cmos_sensor(0x302A, iTotalLines >> 8);
	       OV5650_write_cmos_sensor(0x302B, iTotalLines & 0x00FF);	      
		}		
	}
	#endif
	
}   /*  OV5650_NightMode    */
#endif

void OV5650_camera_para_to_sensor(void)
{
    kal_uint16 iI;

    for (iI = 0; 0xFFFFFFFF != OV5650SensorReg[iI].Addr; iI++) {
        OV5650_write_cmos_sensor(OV5650SensorReg[iI].Addr, OV5650SensorReg[iI].Para);
    }

    for (iI = ENGINEER_START_ADDR; 0xFFFFFFFF != OV5650SensorReg[iI].Addr; iI++) {
       OV5650_write_cmos_sensor(OV5650SensorReg[iI].Addr, OV5650SensorReg[iI].Para);
    }

    for (iI = FACTORY_START_ADDR+1; iI < 5; iI++) {
        OV5650_write_cmos_sensor(OV5650SensorCCT[iI].Addr, OV5650SensorCCT[iI].Para);
    }
}

// update camera_para from sensor register
void OV5650_sensor_to_camera_para(void)
{
    kal_uint16 iI;
	kal_uint16 temp_data;

    for (iI = 0; 0xFFFFFFFF != OV5650SensorReg[iI].Addr; iI++) {
	   	temp_data = OV5650_read_cmos_sensor(OV5650SensorReg[iI].Addr);

		spin_lock(&ov5650_drv_lock);
		OV5650SensorReg[iI].Para = temp_data;
		spin_unlock(&ov5650_drv_lock);
    }

	for (iI = ENGINEER_START_ADDR; 0xFFFFFFFF != OV5650SensorReg[iI].Addr; iI++) {
        temp_data = OV5650_read_cmos_sensor(OV5650SensorReg[iI].Addr);

	    spin_lock(&ov5650_drv_lock);
		OV5650SensorReg[iI].Para = temp_data;
		spin_unlock(&ov5650_drv_lock);
    }

/*
    // CCT record should be not overwritten except by engineering mode
    for (iI = 0; iI < CCT_END_ADDR; iI++) {
        camera_para.SENSOR.cct[iI].para = read_OV5650_reg(camera_para.SENSOR.cct[iI].addr);
    }
*/
}
//------------------------Engineer mode---------------------------------
kal_int32  OV5650_get_sensor_group_count(void)
{   
	return GROUP_TOTAL_NUMS;
}

void OV5650_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
{
   switch (group_idx)
   {
		case PRE_GAIN:
			sprintf((char *)group_name_ptr, "CCT");
			*item_count_ptr = 5;
		break;
		case CMMCLK_CURRENT:
			sprintf((char *)group_name_ptr, "CMMCLK Current");
			*item_count_ptr = 1;
		break;			
		case REGISTER_EDITOR:
			sprintf((char *)group_name_ptr, "Register Editor");
			*item_count_ptr = 2;
		break;		
		default:
		   ASSERT(0);
	}
}

void OV5650_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT *info_ptr)
{
	kal_int16 temp_reg=0;
	kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;

	 temp_para=OV5650SensorCCT[temp_addr].Para;
	
	switch (group_idx)
	{
		case PRE_GAIN:
			switch (item_idx)
			{
				case 0:
				 sprintf((char *)info_ptr->ItemNamePtr,"Pregain-R");			  				  
				  temp_addr = INDEX_PRE_GAIN_R;					  			
				break; 
				case 1:
				  sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gr");
					temp_addr = INDEX_PRE_GAIN_Gr;								
				break;
				case 2:
				     sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gb");
					 temp_addr = INDEX_PRE_GAIN_Gb;					 			
				break;
				case 3:
				      sprintf((char *)info_ptr->ItemNamePtr,"Pregain-B");
					  temp_addr = INDEX_PRE_GAIN_B;					  					
				break;
				case 4:
				   sprintf((char *)info_ptr->ItemNamePtr,"SENSOR_BASEGAIN");
				   temp_addr = INDEX_BASE_GAIN;									    
				break;
				default:
				   ASSERT(0);		
			}

			if(temp_addr!=INDEX_BASE_GAIN)		
			 {  
			 	   temp_gain=1000*(OV5650SensorCCT[temp_addr].Para)/BASEGAIN;
				   info_ptr->ItemValue=temp_gain;
		           info_ptr->IsTrueFalse=KAL_FALSE;
			       info_ptr->IsReadOnly=KAL_FALSE;
			       info_ptr->IsNeedRestart=KAL_FALSE;

			       info_ptr->Min=1*1000;
		           info_ptr->Max=255*1000/0x40;
			 	}
		    else if(temp_addr==INDEX_BASE_GAIN)
			 	{ 
			 	   temp_gain=1000*OV5650_Reg2Gain(OV5650SensorCCT[temp_addr].Para)/BASEGAIN;
			
				  info_ptr->ItemValue=temp_gain;
		          info_ptr->IsTrueFalse=KAL_FALSE;
			      info_ptr->IsReadOnly=KAL_FALSE;
			      info_ptr->IsNeedRestart=KAL_FALSE;

			      info_ptr->Min=1*1000;
		          info_ptr->Max=32*1000;			 	
			 }							
		
			break;
		case CMMCLK_CURRENT:
			switch (item_idx)
			{
				case 0:
				  sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");
				  
				  temp_reg=OV5650SensorReg[CMMCLK_CURRENT_INDEX].Para;
				  if(temp_reg==ISP_DRIVING_2MA)
				  {
				      info_ptr->ItemValue=2;
				  }
				  else if(temp_reg==ISP_DRIVING_4MA)
				  {
				      info_ptr->ItemValue=4;
				  }
				  else if(temp_reg==ISP_DRIVING_6MA)
				  {
				      info_ptr->ItemValue=6;
				  }
				  else if(temp_reg==ISP_DRIVING_8MA)
				  {
				      info_ptr->ItemValue=8;
				  }
				  
				  info_ptr->IsTrueFalse=KAL_FALSE;
				  info_ptr->IsReadOnly=KAL_FALSE;
				  info_ptr->IsNeedRestart=KAL_TRUE;
				  info_ptr->Min=2;
				  info_ptr->Max=8;
				break;
				default:
				   ASSERT(0);
			}
		break;		
		case REGISTER_EDITOR:
			switch (item_idx)
			{
				case 0:
				  sprintf((char *)info_ptr->ItemNamePtr,"REG Addr.");
				  info_ptr->ItemValue=0;
				  info_ptr->IsTrueFalse=KAL_FALSE;
				  info_ptr->IsReadOnly=KAL_FALSE;
				  info_ptr->IsNeedRestart=KAL_FALSE;
				  info_ptr->Min=0;
				  info_ptr->Max=0xFFFF;
				break;
				case 1:
				  sprintf((char *)info_ptr->ItemNamePtr,"REG Value");
				  info_ptr->ItemValue=0;
				  info_ptr->IsTrueFalse=KAL_FALSE;
				  info_ptr->IsReadOnly=KAL_FALSE;
				  info_ptr->IsNeedRestart=KAL_FALSE;
				  info_ptr->Min=0;
				  info_ptr->Max=0xFFFF;
				break;
				default:
				   ASSERT(0);		
			}
		break;
		default:
			ASSERT(0); 
	}
}

void OV5650_set_isp_driving_current(kal_uint8 current)
{
}

kal_bool OV5650_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
   kal_uint16 temp_gain=0, temp_addr=0;//, temp_para=0;
   kal_uint16 temp_data;
   
   switch (group_idx)
	{
		case PRE_GAIN:
			switch (item_idx)
			{
			case 0:
				  temp_addr = INDEX_PRE_GAIN_R;
				  if (ItemValue < 1 * 1000 || ItemValue >= 1000*255/0x40) {
                      return KAL_FALSE;
                  }
				 
				   temp_gain = ItemValue * 0x40 / 1000;  // R, G, B gain = reg. / 0x40
				   spin_lock(&ov5650_drv_lock);
                   OV5650SensorCCT[temp_addr].Para= temp_gain;
				   spin_unlock(&ov5650_drv_lock);
                   OV5650_write_cmos_sensor(OV5650SensorCCT[temp_addr].Addr, temp_gain);
                  break;
				
			 case 1:
				  temp_addr = INDEX_PRE_GAIN_Gr;
				   if (ItemValue < 1 * 1000 || ItemValue >= 1000*255/0x40) {
                      return KAL_FALSE;
                  }
				 
				   temp_gain = ItemValue * 0x40 / 1000;  // R, G, B gain = reg. / 0x40
				   spin_lock(&ov5650_drv_lock);
                   OV5650SensorCCT[temp_addr].Para= temp_gain;
				   spin_unlock(&ov5650_drv_lock);
                   OV5650_write_cmos_sensor(OV5650SensorCCT[temp_addr].Addr, temp_gain);
				break;
			 case 2:
				  temp_addr = INDEX_PRE_GAIN_Gb;
				   if (ItemValue < 1 * 1000 || ItemValue >= 1000*255/0x40) {
                      return KAL_FALSE;
                  }
				 
				   temp_gain = ItemValue * 0x40 / 1000;  // R, G, B gain = reg. / 0x40
				   spin_lock(&ov5650_drv_lock);
                   OV5650SensorCCT[temp_addr].Para= temp_gain;
				   spin_unlock(&ov5650_drv_lock);
                   OV5650_write_cmos_sensor(OV5650SensorCCT[temp_addr].Addr, temp_gain);
				break;
			 case 3:
				  temp_addr = INDEX_PRE_GAIN_B;
				   if (ItemValue < 1 * 1000 || ItemValue >= 1000*255/0x40) {
                      return KAL_FALSE;
                  }
				 
				   temp_gain = ItemValue * 0x40 / 1000;  // R, G, B gain = reg. / 0x40
				   spin_lock(&ov5650_drv_lock);
                   OV5650SensorCCT[temp_addr].Para= temp_gain;
				   spin_unlock(&ov5650_drv_lock);
                   OV5650_write_cmos_sensor(OV5650SensorCCT[temp_addr].Addr, temp_gain);
				break;
			  case 4:
				  temp_addr = INDEX_BASE_GAIN;
				  				 
				  if (ItemValue < 1 * 1000 || ItemValue >= 32 * 1000) {
                      return KAL_FALSE;
                  }

                  temp_gain = OV5650_Gain2Reg(ItemValue * BASEGAIN / 1000);
				  spin_lock(&ov5650_drv_lock);
                  OV5650SensorCCT[temp_addr].Para= temp_gain;
				  spin_unlock(&ov5650_drv_lock);
                  OV5650_write_cmos_sensor(OV5650SensorCCT[temp_addr].Addr, temp_gain);
				  break;
				default:
				   ASSERT(0);		
			}
						
		break;
		case CMMCLK_CURRENT:
			switch (item_idx)
			{
				case 0:
					spin_lock(&ov5650_drv_lock);
				  if(ItemValue==2)
				  {
				      OV5650SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
//				      OV5650_set_isp_driving_current(ISP_DRIVING_2MA);
				  }
				  else if(ItemValue==3 || ItemValue==4)
				  {
				      OV5650SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
//				      OV5650_set_isp_driving_current(ISP_DRIVING_4MA);
				  }
				  else if(ItemValue==5 || ItemValue==6)
				  {
				      OV5650SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
//				      OV5650_set_isp_driving_current(ISP_DRIVING_6MA);
				  }
				  else
				  {
				      OV5650SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
//				      OV5650_set_isp_driving_current(ISP_DRIVING_8MA);
				  }
				  spin_unlock(&ov5650_drv_lock);
				break;
				default:
				   ASSERT(0);
			}
		break;
		case REGISTER_EDITOR:
			switch (item_idx)
			{
				case 0:
					if (ItemValue < 0 || ItemValue > 0xFF) {
                        return KAL_FALSE;
                    }
					spin_lock(&ov5650_drv_lock);
				  OV5650_FAC_SENSOR_REG=ItemValue;
				  spin_unlock(&ov5650_drv_lock);
				break;
				case 1:
					if (ItemValue < 0 || ItemValue > 0xFF) {
                        return KAL_FALSE;
                    }
				  OV5650_write_cmos_sensor(OV5650_FAC_SENSOR_REG,ItemValue);
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
static void OV5650_Sensor_Init(void)
{
	   SENSORDB("ov5650sensor_init begin\r\n");

	    OV5650_write_cmos_sensor(0x3008,0x82);//reset register
		mdelay(3);
		OV5650_write_cmos_sensor(0x3008,0x42);//registers power down
		OV5650_write_cmos_sensor(0x3103,0x93);
		OV5650_write_cmos_sensor(0x3b07,0x0d);
		OV5650_write_cmos_sensor(0x3017,0x7f);
		OV5650_write_cmos_sensor(0x3018,0xfc);
		OV5650_write_cmos_sensor(0x3706,0x41);
		OV5650_write_cmos_sensor(0x3703,0x9a);
		OV5650_write_cmos_sensor(0x3613,0x44);
		OV5650_write_cmos_sensor(0x3630,0x22);
		OV5650_write_cmos_sensor(0x3605,0x04);
		OV5650_write_cmos_sensor(0x3606,0x3f);
		OV5650_write_cmos_sensor(0x3712,0x13);
		OV5650_write_cmos_sensor(0x370e,0x00);
		OV5650_write_cmos_sensor(0x370b,0x40);
		OV5650_write_cmos_sensor(0x3600,0x54);
		OV5650_write_cmos_sensor(0x3601,0x05);
		OV5650_write_cmos_sensor(0x3713,0x92);
		OV5650_write_cmos_sensor(0x3714,0x17);
		OV5650_write_cmos_sensor(0x3631,0x22);
		OV5650_write_cmos_sensor(0x3612,0x1a);
		OV5650_write_cmos_sensor(0x3604,0x40);
		OV5650_write_cmos_sensor(0x3705,0xda);
		OV5650_write_cmos_sensor(0x370a,0x80);
		OV5650_write_cmos_sensor(0x370c,0xa0);
		OV5650_write_cmos_sensor(0x3710,0x28);
		OV5650_write_cmos_sensor(0x3702,0x3a);
		OV5650_write_cmos_sensor(0x3704,0x18);
		OV5650_write_cmos_sensor(0x3a18,0x00);
		OV5650_write_cmos_sensor(0x3a19,0xf8);
		OV5650_write_cmos_sensor(0x3a00,0x38);
		OV5650_write_cmos_sensor(0x3800,0x03);
		OV5650_write_cmos_sensor(0x3801,0x2c);
		OV5650_write_cmos_sensor(0x3803,0x0c);
		OV5650_write_cmos_sensor(0x3804,0x05);
		OV5650_write_cmos_sensor(0x3805,0x00);
		OV5650_write_cmos_sensor(0x3806,0x03);
		OV5650_write_cmos_sensor(0x3807,0xc0);
		OV5650_write_cmos_sensor(0x3808,0x05);
		OV5650_write_cmos_sensor(0x3809,0x00);
		OV5650_write_cmos_sensor(0x380a,0x03);
		OV5650_write_cmos_sensor(0x380b,0xc0);
		OV5650_write_cmos_sensor(0x380c,0x0a);
		OV5650_write_cmos_sensor(0x380d,0x3C);
		OV5650_write_cmos_sensor(0x380e,0x04);
		OV5650_write_cmos_sensor(0x380f,0xC4);
		OV5650_write_cmos_sensor(0x3830,0x50);
		OV5650_write_cmos_sensor(0x3a08,0x12);
		OV5650_write_cmos_sensor(0x3a09,0x70);
		OV5650_write_cmos_sensor(0x3a0a,0x0f);
		OV5650_write_cmos_sensor(0x3a0b,0x60);
		OV5650_write_cmos_sensor(0x3a0d,0x06);
		OV5650_write_cmos_sensor(0x3a0e,0x06);
		OV5650_write_cmos_sensor(0x3a13,0x54);
		OV5650_write_cmos_sensor(0x3815,0x82);
		OV5650_write_cmos_sensor(0x5059,0x80);
		OV5650_write_cmos_sensor(0x505a,0x0a);
		OV5650_write_cmos_sensor(0x505b,0x2e);
		OV5650_write_cmos_sensor(0x3a1a,0x06);
		OV5650_write_cmos_sensor(0x3503,0x00);
		OV5650_write_cmos_sensor(0x3623,0x01);
		OV5650_write_cmos_sensor(0x3633,0x24);
		OV5650_write_cmos_sensor(0x3c01,0x34);
		OV5650_write_cmos_sensor(0x3c04,0x28);
		OV5650_write_cmos_sensor(0x3c05,0x98);
		OV5650_write_cmos_sensor(0x3c07,0x07);
		OV5650_write_cmos_sensor(0x3c09,0xc2);
		OV5650_write_cmos_sensor(0x401d,0x28);
		OV5650_write_cmos_sensor(0x5046,0x09);
		OV5650_write_cmos_sensor(0x3810,0x40);
		OV5650_write_cmos_sensor(0x3836,0x41);
		OV5650_write_cmos_sensor(0x5000,0xfe);
		OV5650_write_cmos_sensor(0x5001,0x01);
		OV5650_write_cmos_sensor(0x5002,0x00);
		OV5650_write_cmos_sensor(0x503d,0x00);
		OV5650_write_cmos_sensor(0x5901,0x00);
		OV5650_write_cmos_sensor(0x585a,0x01);
		OV5650_write_cmos_sensor(0x585b,0x2c);
		OV5650_write_cmos_sensor(0x585c,0x01);
		OV5650_write_cmos_sensor(0x585d,0x93);
		OV5650_write_cmos_sensor(0x585e,0x01);
		OV5650_write_cmos_sensor(0x585f,0x90);
		OV5650_write_cmos_sensor(0x5860,0x01);
		OV5650_write_cmos_sensor(0x5861,0x0d);
		OV5650_write_cmos_sensor(0x5180,0xc0);
		OV5650_write_cmos_sensor(0x5184,0x00);
		OV5650_write_cmos_sensor(0x470a,0x00);
		OV5650_write_cmos_sensor(0x470b,0x00);
		OV5650_write_cmos_sensor(0x470c,0x00);
		OV5650_write_cmos_sensor(0x300f,0x8e);
		OV5650_write_cmos_sensor(0x3603,0xa7);
		OV5650_write_cmos_sensor(0x3615,0x50);
		OV5650_write_cmos_sensor(0x3632,0x55);
		OV5650_write_cmos_sensor(0x3620,0x56);
		OV5650_write_cmos_sensor(0x3621,0xAf);
		OV5650_write_cmos_sensor(0x381a,0x3c);
		OV5650_write_cmos_sensor(0x3818,0xc0);
		OV5650_write_cmos_sensor(0x3631,0x36);
		OV5650_write_cmos_sensor(0x3632,0x5f);
		OV5650_write_cmos_sensor(0x3711,0x24);
		OV5650_write_cmos_sensor(0x370D,0x42);
		OV5650_write_cmos_sensor(0x3818,0xC1);
		OV5650_write_cmos_sensor(0x3010,0x30);
		OV5650_write_cmos_sensor(0x3A08,0x16);
		OV5650_write_cmos_sensor(0x3A09,0xE0);
		OV5650_write_cmos_sensor(0x3A0A,0x13);
		OV5650_write_cmos_sensor(0x3A0B,0x10);
		OV5650_write_cmos_sensor(0x3A0D,0x03);
		OV5650_write_cmos_sensor(0x3A0E,0x03);
		OV5650_write_cmos_sensor(0x401f,0x03);
		OV5650_write_cmos_sensor(0x4000,0x05);
		OV5650_write_cmos_sensor(0x4001,0x02);
		OV5650_write_cmos_sensor(0x401c,0x42);
		OV5650_write_cmos_sensor(0x3008,0x02);
											  
		OV5650_write_cmos_sensor(0x380c,0x08);
		OV5650_write_cmos_sensor(0x380d,0x3C);
		OV5650_write_cmos_sensor(0x380e,0x03);
		OV5650_write_cmos_sensor(0x380f,0xD0);
											  
		OV5650_write_cmos_sensor(0x3503,0x13);//822 modify by meimei 13-> 03
		OV5650_write_cmos_sensor(0x3501,0x44);
		OV5650_write_cmos_sensor(0x3502,0xa0);
		OV5650_write_cmos_sensor(0x350b,0x3d);
		OV5650_write_cmos_sensor(0x5001,0x01);
		OV5650_write_cmos_sensor(0x5046,0x09);
		OV5650_write_cmos_sensor(0x3406,0x01);
		OV5650_write_cmos_sensor(0x3400,0x04);
		OV5650_write_cmos_sensor(0x3401,0x00);
		OV5650_write_cmos_sensor(0x3402,0x04);
		OV5650_write_cmos_sensor(0x3403,0x00);
		OV5650_write_cmos_sensor(0x3404,0x04);
		OV5650_write_cmos_sensor(0x3405,0x00);
		OV5650_write_cmos_sensor(0x5000,0x06);
		OV5650_write_cmos_sensor(0x3621,0xbf);
		OV5650_write_cmos_sensor(0x3818,0x81);
		SENSORDB("ov5650sensor_init end\r\n");

}   /*  OV5650_Sensor_Init  */


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*	OV5650Open
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

UINT32 OV5650Open(void)
{
   kal_uint16 sensor_id=0; 
   kal_uint8 ret;
   SENSORDB("OV5650Open");

   // check if sensor ID correct
   sensor_id=((OV5650_read_cmos_sensor(0x300A) << 8) | OV5650_read_cmos_sensor(0x300B));   
   SENSORDB("ov5650 sensor_id =%d\n",sensor_id);
   if (((OV5650_read_cmos_sensor(0x300A) << 8) | OV5650_read_cmos_sensor(0x300B)) != OV5650_SENSOR_ID) 		      
     return ERROR_SENSOR_CONNECT_FAIL;
 
   SENSORDB("Sensor Read ID OK!\n");

   spin_lock(&ov5650_drv_lock);
   OV5650_g_Fliker_Mode = KAL_FALSE;
   OV5650_g_ZSD_Mode = KAL_FALSE;
   spin_unlock(&ov5650_drv_lock);
	 

   // initail sequence write in
   OV5650_Sensor_Init();

#ifdef OV5650_USE_OTP

     ret = ov5650_update_wb_register_from_otp();
	if(1 == ret)
		{
			SENSORDB("ov5650_update_wb_register_from_otp invalid\n");
		}
	else if(0 == ret)
		{
			SENSORDB("ov5650_update_wb_register_from_otp success\n");
		}

	ret = ov5650_update_lenc_register_from_otp();
	if(1 == ret)
		{
			SENSORDB("ov5650_update_lenc_register_from_otp invalid\n");
		}
	else if(0 == ret)
		{
			SENSORDB("ov5650_update_lenc_register_from_otp success\n");
		}
#endif

   return ERROR_NONE;
}   /* OV5650Open  */

/*************************************************************************
* FUNCTION
*	OV5650Close
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
UINT32 OV5650Close(void)
{
  //CISModulePowerOn(FALSE);
//	DRV_I2CClose(OV5650hDrvI2C);
	return ERROR_NONE;
}   /* OV5650Close */

/*************************************************************************
* FUNCTION
* OV5650Preview
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
UINT32 OV5650Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    //SENSORDB("ov5650preview enter\r\n");
    kal_uint8 iTemp;
    kal_uint16 iStartX = 2, iStartY = 2;//0,1
    kal_uint16 iDummyPixels = 0;    // in XGA mode, dummy pixels must be even number
    kal_uint32 iTempArray = 0;
	kal_uint16 dummy_pixel_temp;
	kal_uint16 Total_line;
	SENSORDB("ov5650 preview begin\r\n");
//Daniel
	OV5650_write_cmos_sensor(0x3008,0x42);//registers power down

	OV5650_write_cmos_sensor(0x3b07,0x0d);
	OV5650_write_cmos_sensor(0x3017,0x7f);//FREX input
	OV5650_write_cmos_sensor(0x3703,0x9a);

	OV5650_write_cmos_sensor(0x3713,0x92);
	OV5650_write_cmos_sensor(0x3714,0x17);
	OV5650_write_cmos_sensor(0x370c,0xa0);

	OV5650_write_cmos_sensor(0x3800,0x03);//HS:812
	OV5650_write_cmos_sensor(0x3801,0x2c);
	OV5650_write_cmos_sensor(0x3803,0x0c);//VS:12
	OV5650_write_cmos_sensor(0x3804,0x05);//HW:1280
	OV5650_write_cmos_sensor(0x3805,0x00);
	OV5650_write_cmos_sensor(0x3806,0x03);//VH:960
	OV5650_write_cmos_sensor(0x3807,0xc0);
	OV5650_write_cmos_sensor(0x3808,0x05);
	OV5650_write_cmos_sensor(0x3809,0x00);
	OV5650_write_cmos_sensor(0x380a,0x03);
	OV5650_write_cmos_sensor(0x380b,0xc0);
	OV5650_write_cmos_sensor(0x380c,0x08);
	OV5650_write_cmos_sensor(0x380d,0x3C);//total H:2108
	OV5650_write_cmos_sensor(0x380e,0x03);
	OV5650_write_cmos_sensor(0x380f,0xD0);//total V:976

	OV5650_write_cmos_sensor(0x401c,0x42);
	OV5650_write_cmos_sensor(0x505f,0x05);

	OV5650_write_cmos_sensor(0x370D,0x42);
	OV5650_write_cmos_sensor(0x3010,0x30);
	OV5650_write_cmos_sensor(0x3A08,0x16);//b50 step
	OV5650_write_cmos_sensor(0x3A09,0xE0);
	OV5650_write_cmos_sensor(0x3A0A,0x13);//b60 step
	OV5650_write_cmos_sensor(0x3A0B,0x10);
	OV5650_write_cmos_sensor(0x3A0D,0x03); //b60 max
	OV5650_write_cmos_sensor(0x3A0E,0x03); //b50 max
	OV5650_write_cmos_sensor(0x3621,0xbf); 
	OV5650_write_cmos_sensor(0x3818,0x81);//bit[6] mirror bit[5]flip
	  //////////////////////////////////////////////hanks add
	  if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)	
	  {
	    
	  	iDummyPixels = 555;
		spin_lock(&ov5650_drv_lock);
		OV5650_g_iDummyLines = 0;
		OV5650_g_iOV5650_Mode = OV5650_MODE_CIF_VIDEO;
		spin_unlock(&ov5650_drv_lock);
	  }

	  else 
	  { // camear preview mode			  
		iDummyPixels = 555;
		spin_lock(&ov5650_drv_lock);
		OV5650_g_iDummyLines = 0;
	    OV5650_g_iOV5650_Mode = OV5650_MODE_PREVIEW;
		spin_unlock(&ov5650_drv_lock);
	  }
	  
		   
	  
		  // Mirror/Flip function
		  //for CCT TEST
		  
		  switch (sensor_config_data->SensorImageMirror)
		  {
			  case IMAGE_NORMAL://h_mirror 
				  iStartX = 2;
				  iStartY = 2;
				  OV5650_write_cmos_sensor(0x3621, 0xaf);
				  OV5650_write_cmos_sensor(0x505a, 0x0a);//add hmirror
				  OV5650_write_cmos_sensor(0x505b, 0x2e);//
				  OV5650_write_cmos_sensor(0x3827, 0x0c);
				  OV5650_write_cmos_sensor(0x3818, 0xc1);
				  SENSORDB("IMAGE_NORMAL\n");
				  break;
			  case IMAGE_H_MIRROR:

				  iStartX = 2;
				  iStartY = 2;
				  OV5650_write_cmos_sensor(0x3621, 0xaf);
				  OV5650_write_cmos_sensor(0x505a, 0x0a);//add hmirror
				  OV5650_write_cmos_sensor(0x505b, 0x2e);//
				  OV5650_write_cmos_sensor(0x3827, 0x0c);
				  OV5650_write_cmos_sensor(0x3818, 0xc1);
				  SENSORDB("IMAGE_H_MIRROR\n");
				  break;
			  case IMAGE_V_MIRROR:

				  iStartX = 2;
				  iStartY = 2;
				  OV5650_write_cmos_sensor(0x3621, 0xbf);
				  OV5650_write_cmos_sensor(0x505a, 0x00);//add hmirror
				  OV5650_write_cmos_sensor(0x505b, 0x12);//
				  OV5650_write_cmos_sensor(0x3827, 0x0b);
				  OV5650_write_cmos_sensor(0x3818, 0xa1);
				  SENSORDB("IMAGE_V_MIRROR\n");
				  break;
			  case IMAGE_HV_MIRROR:

				  iStartX = 2;
				  iStartY = 2;
				  OV5650_write_cmos_sensor(0x3621, 0xaf);
				  OV5650_write_cmos_sensor(0x505a, 0x0a);//add hmirror
				  OV5650_write_cmos_sensor(0x505b, 0x2e);//
				  OV5650_write_cmos_sensor(0x3827, 0x0b);
				  OV5650_write_cmos_sensor(0x3818, 0xe1);
				  SENSORDB("IMAGE_HV_MIRROR\n");
				  break;
			  default:
				  ASSERT(0);
		  }
		  spin_lock(&ov5650_drv_lock);
		  OV5650_g_bXGA_Mode = KAL_TRUE;
		 OV5650_g_iPV_Pixels_Per_Line = OV5650_XGA_MODE_WITHOUT_DUMMY_PIXELS + iDummyPixels;// = dummy_pixel_temp;
		 spin_unlock(&ov5650_drv_lock);
		 OV5650_Set_Dummy(iDummyPixels, OV5650_g_iDummyLines);
//Daniel
	OV5650_write_cmos_sensor(0x3008,0x02);//registers power down
	SENSORDB("OV5650 group hold done(pre)\n");

	  ////////////////////////////////////////////////////////
	image_window->GrabStartX= iStartX;
	image_window->GrabStartY= iStartY;
	image_window->ExposureWindowWidth = OV5650_IMAGE_SENSOR_PV_WIDTH - iStartX  - (OV5650_ISP_INTERPOLATIO_FILTER_WIDTH - 1)-4;
	image_window->ExposureWindowHeight = OV5650_IMAGE_SENSOR_PV_HEIGHT - iStartY - (OV5650_ISP_INTERPOLATIO_FILTER_HEIGHT - 1)-4;
	// copy sensor_config_data
	SENSORDB("OV5650 preview end\n");
	memcpy(&OV5650SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
	



}   /*  OV5650Preview   */

/*************************************************************************
* FUNCTION
*	OV5650Capture
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
UINT32 OV5650Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    kal_uint32 iShutter = OV5650_g_iExpLines;
    //const kal_bool bMetaMode = pSensor_Config_Data->meta_mode != CAPTURE_MODE_NORMAL;
    kal_uint16 iStartX = 2, iStartY = 2, iGrabWidth = 0, iGrabHeight = 0;
    kal_uint16 iDummyPixels = 0;
    kal_uint16 iCP_Pixels_Per_Line = 0;
	kal_uint16 Pv_Shutter,Pc_Shutter;
	kal_uint16 Pv_Dummy,Pc_Dummy;
	kal_uint16 Total_line;
	
    //static float fCP_PCLK = 0;
	static kal_uint32 fCP_PCLK = 0;

	SENSORDB("ov5650capture start\r\n");

if(OV5650_MODE_PREVIEW == OV5650_g_iOV5650_Mode)
{
    spin_lock(&ov5650_drv_lock);
    OV5650_g_iOV5650_Mode = OV5650_MODE_CAPTURE;
    OV5650_g_Fliker_Mode = KAL_FALSE;
	spin_unlock(&ov5650_drv_lock);

    if (sensor_config_data->EnableShutterTansfer == KAL_TRUE) {
        iShutter =sensor_config_data->CaptureShutter;
    }

    if (image_window->ImageTargetWidth <= OV5650_IMAGE_SENSOR_PV_WIDTH && image_window->ImageTargetHeight <= OV5650_IMAGE_SENSOR_PV_HEIGHT) 
		{
		SENSORDB("ov5650capture_pv_size\n");
        // ordinary capture mode for <= XGA resolution

			//#if (defined(CAM_OTF_CAPTURE))
			//	{
			//		fCP_PCLK = OV5650_g_fPV_PCLK;
	       //         iGrabWidth = OV5650_IMAGE_SENSOR_PV_WIDTH - iStartX - (OV5650_ISP_INTERPOLATIO_FILTER_WIDTH - 1);
	        //        iGrabHeight = OV5650_IMAGE_SENSOR_PV_HEIGHT - iStartY - (OV5650_ISP_INTERPOLATIO_FILTER_HEIGHT - 1);
			//		iDummyPixels = 1000;
			//		OV5650_g_iDummyLines = 0;
			//	}
			//#else
				fCP_PCLK = OV5650_g_fPV_PCLK;
                iGrabWidth = OV5650_IMAGE_SENSOR_PV_WIDTH - iStartX - (OV5650_ISP_INTERPOLATIO_FILTER_WIDTH - 1);
                iGrabHeight = OV5650_IMAGE_SENSOR_PV_HEIGHT - iStartY - (OV5650_ISP_INTERPOLATIO_FILTER_HEIGHT - 1);
				iDummyPixels = 555;
				spin_lock(&ov5650_drv_lock);
				OV5650_g_iDummyLines = 0;
				spin_unlock(&ov5650_drv_lock);
			//#endif
        

        iCP_Pixels_Per_Line = OV5650_XGA_MODE_WITHOUT_DUMMY_PIXELS + iDummyPixels;
    }
	else 
    { // ordinary capture mode for > XGA resolution
    
        
	SENSORDB("ov5650full size capture\n");	
//Daniel
	OV5650_write_cmos_sensor(0x3008,0x42);//registers power down
//Daniel
	OV5650_write_cmos_sensor(0x3b07,0x0c);
	OV5650_write_cmos_sensor(0x3017,0xff);
	OV5650_write_cmos_sensor(0x3703,0xe6);
	OV5650_write_cmos_sensor(0x3713,0x22);
	OV5650_write_cmos_sensor(0x3714,0x27);
	OV5650_write_cmos_sensor(0x370c,0x00);
	OV5650_write_cmos_sensor(0x3800,0x02);//HS:596
	OV5650_write_cmos_sensor(0x3801,0x54);

	OV5650_write_cmos_sensor(0x3804,0x0a);//HW:2592
	OV5650_write_cmos_sensor(0x3805,0x20);
	OV5650_write_cmos_sensor(0x3806,0x07);//VH:1944
	OV5650_write_cmos_sensor(0x3807,0x98);
	OV5650_write_cmos_sensor(0x3808,0x0a);
	OV5650_write_cmos_sensor(0x3809,0x20);
	OV5650_write_cmos_sensor(0x380a,0x07);
	OV5650_write_cmos_sensor(0x380b,0x98);
	OV5650_write_cmos_sensor(0x380c,0x0c);
	OV5650_write_cmos_sensor(0x380d,0xb4);
	OV5650_write_cmos_sensor(0x380e,0x07);
	OV5650_write_cmos_sensor(0x380f,0xb0);


	OV5650_write_cmos_sensor(0x401c,0x46);
	OV5650_write_cmos_sensor(0x505f,0x04);

	OV5650_write_cmos_sensor(0x370D,0x04);
	OV5650_write_cmos_sensor(0x3A08,0x12);
	OV5650_write_cmos_sensor(0x3A09,0x70);
	OV5650_write_cmos_sensor(0x3A0A,0x0f);
	OV5650_write_cmos_sensor(0x3A0B,0x60);
	OV5650_write_cmos_sensor(0x3A0D,0x06);
	OV5650_write_cmos_sensor(0x3A0E,0x06);

	OV5650_write_cmos_sensor(0x3010,0x30); 

//add for CCT test
	switch (sensor_config_data->SensorImageMirror)
		  {
			  case IMAGE_NORMAL:
				  iStartX = 2;
				  iStartY = 2;

				  OV5650_write_cmos_sensor(0x3621, 0x2f);
				  OV5650_write_cmos_sensor(0x505a, 0x0a);
				  OV5650_write_cmos_sensor(0x505b, 0x2e);
				  OV5650_write_cmos_sensor(0x3827, 0x0c);
				  OV5650_write_cmos_sensor(0x3818, 0xc0);
				  SENSORDB("IMAGE_NORMAL\r\n");

				  break;
			  case IMAGE_H_MIRROR:

				  iStartX = 2;
				  iStartY = 2;
				  OV5650_write_cmos_sensor(0x3621, 0x2f);
				  OV5650_write_cmos_sensor(0x505a, 0x0a);
				  OV5650_write_cmos_sensor(0x505b, 0x2e);
				  OV5650_write_cmos_sensor(0x3827, 0x0c);
				  OV5650_write_cmos_sensor(0x3818, 0xc0);
				  SENSORDB("IMAGE_H_MIRROR\r\n");

				  break;
			  case IMAGE_V_MIRROR:

				  iStartX = 2;
				  iStartY = 2;
				  OV5650_write_cmos_sensor(0x3621, 0x3f);
				  OV5650_write_cmos_sensor(0x505a, 0x00);
				  OV5650_write_cmos_sensor(0x505b, 0x12);
				  OV5650_write_cmos_sensor(0x3827, 0x0b);
				  OV5650_write_cmos_sensor(0x3818, 0xa0);
				  SENSORDB("IMAGE_V_MIRROR\r\n");

				  break;
			  case IMAGE_HV_MIRROR:

				  iStartX = 2;
				  iStartY = 2;
				  OV5650_write_cmos_sensor(0x3621, 0x2f);
				  OV5650_write_cmos_sensor(0x505a, 0x0a);
				  OV5650_write_cmos_sensor(0x505b, 0x2e);
				  OV5650_write_cmos_sensor(0x3827, 0x0b);
				  OV5650_write_cmos_sensor(0x3818, 0xe0);
				  SENSORDB("IMAGE_HV_MIRROR\r\n");
	
				  break;
			  default:
				  ASSERT(0);
		  }

		iDummyPixels = 140;
		
		spin_lock(&ov5650_drv_lock);
		OV5650_g_iDummyLines = 24;
		OV5650_g_bXGA_Mode = KAL_FALSE;
		spin_unlock(&ov5650_drv_lock);
		
		fCP_PCLK = OV5650_g_fCP_PCLK;
        iGrabWidth = OV5650_IMAGE_SENSOR_FULL_WIDTH_DRV; //- iStartX - (OV5650_ISP_INTERPOLATIO_FILTER_WIDTH - 1);//2578
        iGrabHeight = OV5650_IMAGE_SENSOR_FULL_HEIGHT_DRV;// - iStartY - (OV5650_ISP_INTERPOLATIO_FILTER_HEIGHT - 1);//1936
        SENSORDB("iGrabWidth = %d\r\n",iGrabWidth);
		SENSORDB("iGrabHeight = %d\r\n",iGrabHeight);
        iCP_Pixels_Per_Line = OV5650_QXGA_MODE_WITHOUT_DUMMY_PIXELS + iDummyPixels;
    }
	image_window->GrabStartX = iStartX;
    image_window->GrabStartY = iStartY;
    image_window->ExposureWindowWidth= iGrabWidth;
    image_window->ExposureWindowHeight = iGrabHeight;   

    SENSORDB("pre iShutter = %d\r\n",iShutter);
	SENSORDB("pre fCP_PCLK = %d\r\n",fCP_PCLK);
	SENSORDB("pre OV5650_g_fPV_PCLK = %d\r\n",OV5650_g_fPV_PCLK);
	SENSORDB("pre OV5650_g_iPV_Pixels_Per_Line = %d\r\n",OV5650_g_iPV_Pixels_Per_Line);
	SENSORDB("pre iCP_Pixels_Per_Line = %d\r\n",iCP_Pixels_Per_Line);
//	xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//	iShutter = (iShutter * (1024*(fCP_PCLK>>10) / (OV5650_g_fPV_PCLK>>10)) * OV5650_g_iPV_Pixels_Per_Line / iCP_Pixels_Per_Line) >>10;


    iShutter = iShutter * (fCP_PCLK / OV5650_g_fPV_PCLK );
    iShutter = (iShutter * OV5650_g_iPV_Pixels_Per_Line) / iCP_Pixels_Per_Line;
	
    SENSORDB("after iShutter = %d\r\n",iShutter);
	
    //iShutter = iShutter * (fCP_PCLK / OV5650_g_fPV_PCLK) * OV5650_g_iPV_Pixels_Per_Line / iCP_Pixels_Per_Line;
	//kal_prompt_trace(MOD_ENG,"%d",iShutter);
	//if(OV5650_g_bXGA_Mode == KAL_FALSE)
	//	iShutter *= 2;
	//kal_prompt_trace(MOD_ENG,"%d",iShutter);
	OV5650_Set_Dummy(iDummyPixels,OV5650_g_iDummyLines);
	OV5650_Write_Shutter(iShutter);
	//Daniel
	OV5650_write_cmos_sensor(0x3008,0x02);//registers power down
	SENSORDB("Group hold done!\r\n");
//Daniel
	SENSORDB("capture set dummy and shutter done!\r\n");
}
	SENSORDB("ov5650capture end\r\n");

    // copy sensor_config_data
	memcpy(&OV5650SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;


}   /* OV5650_Capture() */

UINT32 OV5650GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=OV5650_IMAGE_SENSOR_FULL_WIDTH_DRV - 40;
	pSensorResolution->SensorFullHeight=OV5650_IMAGE_SENSOR_FULL_HEIGHT_DRV - 32;
	pSensorResolution->SensorPreviewWidth=OV5650_IMAGE_SENSOR_PV_WIDTH - 16;
	pSensorResolution->SensorPreviewHeight=OV5650_IMAGE_SENSOR_PV_HEIGHT - 12;
	return ERROR_NONE;
}	/* OV5650GetResolution() */

UINT32 OV5650GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
  switch(ScenarioId)
  	{
  		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorPreviewResolutionX=OV5650_IMAGE_SENSOR_FULL_WIDTH_DRV;
			pSensorInfo->SensorPreviewResolutionY=OV5650_IMAGE_SENSOR_FULL_HEIGHT_DRV;
			pSensorInfo->SensorCameraPreviewFrameRate = 15;
			//pSensorInfo->SensorFullResolutionX=OV5650_IMAGE_SENSOR_FULL_WIDTH_DRV;
			//pSensorInfo->SensorFullResolutionY=OV5650_IMAGE_SENSOR_FULL_HEIGHT_DRV;
			break;
		default:
  	        pSensorInfo->SensorPreviewResolutionX=OV5650_IMAGE_SENSOR_PV_WIDTH - 16;
			pSensorInfo->SensorPreviewResolutionY=OV5650_IMAGE_SENSOR_PV_HEIGHT - 12;
			pSensorInfo->SensorCameraPreviewFrameRate = 30;
			break;
	}

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE; //low active
	pSensorInfo->SensorResetDelayCount=5; 
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_Gr;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	//SENSOR_CLOCK_POLARITY_LOW
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_HIGH;//
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 4;
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
	pSensorInfo->CaptureDelayFrame = 2; 
	pSensorInfo->PreviewDelayFrame = 2; 
	pSensorInfo->VideoDelayFrame = 2;  
	pSensorInfo->SensorMasterClockSwitch = 0; 
  pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;   	
  
  pSensorInfo->AEShutDelayFrame = 0;		  /* The frame of setting shutter default 0 for TG int */
	 pSensorInfo->AESensorGainDelayFrame = 1;	  /* The frame of setting sensor gain */
	 pSensorInfo->AEISPGainDelayFrame = 2;	  
	

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			#if 1
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;	
			pSensorInfo->SensorGrabStartX = 2; 		
			pSensorInfo->SensorGrabStartY = 2; 
		   #else
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	4;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 4;
			pSensorInfo->SensorDataLatchCount= 2;
		#endif

		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			#if 1
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = 12; 		
			pSensorInfo->SensorGrabStartY = 16; 
			#else
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	4;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 4;
			pSensorInfo->SensorDataLatchCount= 2;
			#endif
		break;
		default:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;		
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = 2; 		
			pSensorInfo->SensorGrabStartY = 2; 
		break;
	}
	spin_lock(&ov5650_drv_lock);
	OV5650PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	spin_unlock(&ov5650_drv_lock);
		memcpy(pSensorConfigData, &OV5650SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
return ERROR_NONE;
}	/* OV5650GetInfo() */


UINT32 OV5650Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	CurrentScenarioId = ScenarioId;
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			OV5650Preview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			OV5650Capture(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
		    OV5650_g_ZSD_Mode = KAL_TRUE;
			OV5650Capture(pImageWindow, pSensorConfigData);
		break;		
        //s_porting add
        //s_porting add
        //s_porting add
              default:
                  return ERROR_INVALID_SCENARIO_ID;
        //e_porting add
        //e_porting add
        //e_porting add
	}
	return TRUE;
}	/* MT9P012Control() */
UINT32 OV5650SetCalData(PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData)
{
	UINT32 i;

	printk("OV5650 Sensor write calibration data num = %d \r\n", pSetSensorCalData->DataSize);
	printk("OV5650 Sensor write calibration data format = %x \r\n", pSetSensorCalData->DataFormat);	
	if(pSetSensorCalData->DataSize <= MAX_SHADING_DATA_TBL)
	{
		for (i = 0; i < pSetSensorCalData->DataSize; i++)
		{
			if (((pSetSensorCalData->DataFormat & 0xFFFF) == 1) && ((pSetSensorCalData->DataFormat >> 16) == 1))
			{
				printk("OV5650 Sensor write calibration data: address = %x, value = %x \r\n",(pSetSensorCalData->ShadingData[i])>>16,(pSetSensorCalData->ShadingData[i])&0xFFFF);
				//OV5650_write_cmos_sensor((pSetSensorCalData->ShadingData[i])>>16, (pSetSensorCalData->ShadingData[i])&0xFFFF);
			}
		}
	}
	return ERROR_NONE;
}

UINT32 OV5650SetVideoMode(UINT16 u2FrameRate)
{

  kal_uint16 dummy_lines;

  SENSORDB("OV5650SetVideoMode£¬u2FrameRate:%d\n",u2FrameRate);

  if((30 == u2FrameRate)||(15 == u2FrameRate))
  	{
  	    
	    dummy_lines = (kal_uint16)((OV5650_g_fPV_PCLK/u2FrameRate)/(OV5650_PV_PERIOD_PIXEL_NUMS+OV5650_iDummypixels));      
		dummy_lines = dummy_lines - OV5650_PV_PERIOD_LINE_NUMS;
	    OV5650_Set_Dummy(0,dummy_lines);
  	}
  else if(0 == u2FrameRate)
  	{
  	   SENSORDB("disable video mode\n");
  	}
  return TRUE;
  
}


UINT32 OV5650SetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
	
	
	
	SENSORDB("[OV5650SetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);

	if(bEnable)
		{
		    
			
			OV5650_g_Fliker_Mode = KAL_TRUE;

			
		}
	else
		{
			
			OV5650_g_Fliker_Mode = KAL_FALSE;
			
		}
	SENSORDB("[OV5650SetAutoFlickerMode]bEnable:%x \n",bEnable);
}




UINT32 OV5650FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	UINT32 OV5650SensorRegNumber;
	UINT32 i;
	PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
	MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
	MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;
	PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData=(PSET_SENSOR_CALIBRATION_DATA_STRUCT)pFeaturePara;

	SENSORDB("OV5650FeatureControl FeatureId =%d\n",FeatureId); 

	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=OV5650_IMAGE_SENSOR_FULL_WIDTH_DRV;
			*pFeatureReturnPara16=OV5650_IMAGE_SENSOR_FULL_HEIGHT_DRV;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
			switch(CurrentScenarioId)
				{
					case MSDK_SCENARIO_ID_CAMERA_ZSD:
						*pFeatureReturnPara16++=OV5650_QXGA_MODE_WITHOUT_DUMMY_PIXELS+ OV5650_iDummypixels;  
						*pFeatureReturnPara16=OV5650_QXGA_MODE_WITHOUT_DUMMY_LINES + OV5650_g_iDummyLines;   
						*pFeatureParaLen=4;
						 SENSORDB("OV5650FeatureControl CAP OV5650_iDummypixels =%d\n",OV5650_iDummypixels);
						 //SENSORDB("OV5650FeatureControl OV5650_g_iDummyLines =%d\n",OV5650_g_iDummyLines);
						break;
					default:
						*pFeatureReturnPara16++=OV5650_XGA_MODE_WITHOUT_DUMMY_PIXELS + OV5650_iDummypixels;  
						*pFeatureReturnPara16=OV5650_XGA_MODE_WITHOUT_DUMMY_LINES + OV5650_g_iDummyLines;  
						*pFeatureParaLen=4;
						 SENSORDB("OV5650FeatureControl PV OV5650_iDummypixels =%d\n",OV5650_iDummypixels);
						 //SENSORDB("OV5650FeatureControl OV5650_g_iDummyLines =%d\n",OV5650_g_iDummyLines);
						 break;
				}
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			switch(CurrentScenarioId)
				{
					case MSDK_SCENARIO_ID_CAMERA_ZSD:
						*pFeatureReturnPara32 = OV5650_g_fCP_PCLK;
				        *pFeatureParaLen=4;
						SENSORDB("OV5650FeatureControl OV5650_g_fCP_PCLK =%d\n",OV5650_g_fCP_PCLK);
						break;
					default:
					   *pFeatureReturnPara32 = OV5650_g_fPV_PCLK;
				       *pFeatureParaLen=4;
					   SENSORDB("OV5650FeatureControl OV5650_g_fPV_PCLK =%d\n",OV5650_g_fPV_PCLK);
					   break;
				}
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			set_OV5650_shutter(*pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			//OV5650_night_mode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:
			OV5650_SetGain((UINT16) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			spin_lock(&ov5650_drv_lock);
			OV5650_isp_master_clock=*pFeatureData32;
			spin_unlock(&ov5650_drv_lock);
		break;
		case SENSOR_FEATURE_SET_REGISTER:
OV5650_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
	pSensorRegData->RegData = OV5650_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
			OV5650SensorRegNumber=*pFeatureData32++;
			if (OV5650SensorRegNumber> FACTORY_END_ADDR)
				return FALSE;
			for (i=0;i<OV5650SensorRegNumber;i++)
			{
				spin_lock(&ov5650_drv_lock);
				OV5650SensorCCT[i].Addr=*pFeatureData32++;
				OV5650SensorCCT[i].Para=*pFeatureData32++;
				spin_unlock(&ov5650_drv_lock);
			}
		break;
		case SENSOR_FEATURE_GET_CCT_REGISTER:
			OV5650SensorRegNumber= FACTORY_END_ADDR;
			if (*pFeatureParaLen<(OV5650SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
				return FALSE;
			*pFeatureData32++=OV5650SensorRegNumber;
			for (i=0;i<OV5650SensorRegNumber;i++)
			{
				*pFeatureData32++=OV5650SensorCCT[i].Addr;
				*pFeatureData32++=OV5650SensorCCT[i].Para;
			}
		break;
		case SENSOR_FEATURE_SET_ENG_REGISTER:
			OV5650SensorRegNumber=*pFeatureData32++;
			if (OV5650SensorRegNumber>FACTORY_END_ADDR)
				return FALSE;
			for (i=0;i<OV5650SensorRegNumber;i++)
			{
				spin_lock(&ov5650_drv_lock);
				OV5650SensorReg[i].Addr=*pFeatureData32++;
				OV5650SensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&ov5650_drv_lock);
			}
		break;
		case SENSOR_FEATURE_GET_ENG_REGISTER:
			OV5650SensorRegNumber=FACTORY_END_ADDR;
			if (*pFeatureParaLen<(OV5650SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
				return FALSE;
			*pFeatureData32++=OV5650SensorRegNumber;
			for (i=0;i<OV5650SensorRegNumber;i++)
			{
				*pFeatureData32++=OV5650SensorReg[i].Addr;
				*pFeatureData32++=OV5650SensorReg[i].Para;
			}
		break;
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
			if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
			{
				pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
				pSensorDefaultData->SensorId=OV5650_SENSOR_ID;
				memcpy(pSensorDefaultData->SensorEngReg, OV5650SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
				memcpy(pSensorDefaultData->SensorCCTReg, OV5650SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
			}
			else
				return FALSE;
			*pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
		break;
case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &OV5650SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
			break;
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		     OV5650_camera_para_to_sensor();
		break;
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
			OV5650_sensor_to_camera_para();
		break;							
		case SENSOR_FEATURE_GET_GROUP_COUNT:
			*pFeatureReturnPara32++=OV5650_get_sensor_group_count();
			*pFeatureParaLen=4;
		break;										
		case SENSOR_FEATURE_GET_GROUP_INFO:
			OV5650_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
		break;	
		case SENSOR_FEATURE_GET_ITEM_INFO:
			OV5650_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		break;			
		case SENSOR_FEATURE_SET_ITEM_INFO:
			OV5650_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		break;			
		case SENSOR_FEATURE_GET_ENG_INFO:
			pSensorEngInfo->SensorId = 130;	
			pSensorEngInfo->SensorType = CMOS_SENSOR;
			pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_Gr;
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
		break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		       OV5650SetVideoMode(*pFeatureData16);
        break; 
		case SENSOR_FEATURE_SET_CALIBRATION_DATA:
			OV5650SetCalData(pSetSensorCalData);
			break;
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			OV5650SetAutoFlickerMode((BOOL)*pFeatureData16,*(pFeatureData16+1));
			break;
		default:
			break;
	}
	return ERROR_NONE;
}	/* MT9P012FeatureControl() */
SENSOR_FUNCTION_STRUCT    SensorFuncOV5650=
{
  OV5650Open,
  OV5650GetInfo,
  OV5650GetResolution,
  OV5650FeatureControl,
  OV5650Control,
  OV5650Close
};

UINT32 OV5650SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{

  /* To Do : Check Sensor status here */
  if (pfFunc!=NULL)
      *pfFunc=&SensorFuncOV5650;

  return ERROR_NONE;
} /* SensorInit() */


