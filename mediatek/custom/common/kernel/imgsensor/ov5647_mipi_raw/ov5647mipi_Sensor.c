/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.c
 *
 * Project:
 * --------
 *   RAW
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author:
 * -------
 *   HengJun (MTK70677)
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

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "ov5647mipi_Sensor.h"
#include "ov5647mipi_Camera_Sensor_para.h"
#include "ov5647mipi_CameraCustomized.h"

MSDK_SCENARIO_ID_ENUM CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
static kal_bool OV5647MIPIAutoFlicKerMode = KAL_FALSE;
//static kal_bool OV5647MIPIZsdCameraPreview = KAL_FALSE;


//#define OV5647MIPI_DRIVER_TRACE
#define OV5647MIPI_DEBUG

#ifdef OV5647MIPI_DEBUG
#define SENSORDB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[OV5647MIPI]", fmt, ##arg)
#else
#define SENSORDB(fmt, arg...)
#endif
//#define ACDK
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);


UINT32 OV5647MIPISetMaxFrameRate(UINT16 u2FrameRate);
static DEFINE_SPINLOCK(ov5647mipi_drv_lock);


static OV5647MIPI_sensor_struct OV5647MIPI_sensor =
{
  .eng =
  {
    .reg = OV5647MIPI_CAMERA_SENSOR_REG_DEFAULT_VALUE,
    .cct = OV5647MIPI_CAMERA_SENSOR_CCT_DEFAULT_VALUE,
  },
  .eng_info =
  {
    .SensorId = 128,
    .SensorType = CMOS_SENSOR,
    .SensorOutputDataFormat = OV5647MIPI_COLOR_FORMAT,
  },
  .shutter = 0x20,  
  .gain = 0x20,
  .pclk = OV5647MIPI_PREVIEW_CLK,
  .frame_height = OV5647MIPI_PV_PERIOD_LINE_NUMS,
  .line_length = OV5647MIPI_PV_PERIOD_PIXEL_NUMS,
};


kal_uint16 OV5647MIPI_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char puSendCmd[3] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 3,OV5647MIPI_WRITE_ID);
	return TRUE;

}
kal_uint16 OV5647MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,OV5647MIPI_WRITE_ID);
    return get_byte;
}

#if defined(OV5647MIPI_USE_OTP)

#if defined(OV5647MIPI_USE_AWB_OTP)
//index:index of otp group.(0,1,2)
//return:	0:group index is empty.
//		1.group index has invalid data
	//		2.group index has valid data
kal_uint16 OV5647MIPI_check_otp_wb(kal_uint16 index)
{
	kal_uint16 temp,flag;
	kal_uint32 address;

       OV5647MIPI_write_cmos_sensor(0x0100, 0x01);	
	 mdelay(1);  
	//read otp into buffer
	OV5647MIPI_write_cmos_sensor(0x3d21,0x01);

	//read flag
	address = 0x3d05+index*9;
	flag = OV5647MIPI_read_cmos_sensor(address);

	//SENSORDB("OV5647MIPI_check_otp_wb=%d\r\n",flag);

	//do not clear OTP buffer
	//clear otp buffer
	//for(temp=0;temp<32;temp++){
	//	OV5647MIPI_write_cmos_sensor(0x3d00+temp,0x00);
	//}
	
	
	//disable otp read
	OV5647MIPI_write_cmos_sensor(0x3d21,0x0);
	OV5647MIPI_write_cmos_sensor(0x0100, 0x00);	

	if(NULL == flag)
		{
			
			SENSORDB("[OV5647MIPI_check_otp_awb]index[%x]read flag[%x][0]\n",index,flag);
			return 0;
			
		}
	else if(!(flag&0x80) && (flag&0x7f))
		{
			SENSORDB("[OV5647MIPI_check_otp_awb]index[%x]read flag[%x][2]\n",index,flag);
			return 2;
		}
	else
		{
			SENSORDB("[OV5647MIPI_check_otp_awb]index[%x]read flag[%x][1]\n",index,flag);
		    return 1;
		}
	
}

//index:index of otp group.(0,1,2)
//return: 0
kal_uint16 OV5647MIPI_read_otp_wb(kal_uint16 index, struct OV5647MIPI_otp_struct *otp)
{
	kal_uint16 temp;
	kal_uint32 address;
       OV5647MIPI_write_cmos_sensor(0x0100, 0x01);	
	 mdelay(1);  
	//read otp into buffer
	OV5647MIPI_write_cmos_sensor(0x3d21,0x01);

	address = 0x3d05 +index*9;

	////4 modified the start address
	//address = 0x05 + index*9 +1;

	otp->customer_id = (OV5647MIPI_read_cmos_sensor(address)&0x7f);
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]module_integrator_id[%x]\n",address,otp->customer_id);
	
	otp->module_integrator_id = OV5647MIPI_read_cmos_sensor(address);
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]module_integrator_id[%x]\n",address,otp->module_integrator_id);
	
	otp->lens_id = OV5647MIPI_read_cmos_sensor(address+1);
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]lens_id[%x]\n",address,otp->lens_id);
	
	otp->rg_ratio = OV5647MIPI_read_cmos_sensor(address+2);
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]rg_ratio[%x]\n",address,otp->rg_ratio);

	otp->bg_ratio = OV5647MIPI_read_cmos_sensor(address+3);
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]bg_ratio[%x]\n",address,otp->bg_ratio);

	otp->user_data[0] = OV5647MIPI_read_cmos_sensor(address+4);
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[0][%x]\n",address,otp->user_data[0]);

	otp->user_data[1] = OV5647MIPI_read_cmos_sensor(address+5);
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[1][%x]\n",address,otp->user_data[1]);

	otp->user_data[2] = OV5647MIPI_read_cmos_sensor(address+6);	
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[2][%x]\n",address,otp->user_data[2]);

	otp->user_data[3] = OV5647MIPI_read_cmos_sensor(address+7);	
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[3][%x]\n",address,otp->user_data[3]);

	otp->user_data[4] = OV5647MIPI_read_cmos_sensor(address+8);	
	SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[3][%x]\n",address,otp->user_data[3]);

	//disable otp read
	OV5647MIPI_write_cmos_sensor(0x3d21,00);
	
       OV5647MIPI_write_cmos_sensor(0x0100, 0x00);	
	
	//do not clear OTP buffer
		//clear otp buffer
	//for(temp=0;temp<32;temp++){
	//	OV5647MIPI_write_cmos_sensor(0x3d00+temp,0x00);
	//}

	return 0;
	
	
}

//R_gain: red gain of sensor AWB, 0x400 = 1
//G_gain: green gain of sensor AWB, 0x400 = 1
//B_gain: blue gain of sensor AWB, 0x400 = 1
//reutrn 0
kal_uint16 OV5647MIPI_update_wb_gain(kal_uint32 R_gain, kal_uint32 G_gain, kal_uint32 B_gain)
{

    SENSORDB("[OV5647MIPI_update_wb_gain]R_gain[%x]G_gain[%x]B_gain[%x]\n",R_gain,G_gain,B_gain);

	if(R_gain > 0x400)
		{
			OV5647MIPI_write_cmos_sensor(0x5186,R_gain >> 8);
			OV5647MIPI_write_cmos_sensor(0x5187,(R_gain&0x00ff));
		}
	if(G_gain > 0x400)
		{
			OV5647MIPI_write_cmos_sensor(0x5188,G_gain >> 8);
			OV5647MIPI_write_cmos_sensor(0x5189,(G_gain&0x00ff));
		}
	if(B_gain >0x400)
		{
			OV5647MIPI_write_cmos_sensor(0x518a,B_gain >> 8);
			OV5647MIPI_write_cmos_sensor(0x518b,(B_gain&0x00ff));
		}
	return 0;
}

//R/G and B/G ratio of typical camera module is defined here

kal_uint32 tRG_Ratio_typical = RG_TYPICAL;
kal_uint32 tBG_Ratio_typical = BG_TYPICAL;

//call this function after OV5647MIPI initialization
//return value:	0 update success
//				1 no 	OTP

kal_uint16 OV5647MIPI_update_wb_register_from_otp(void)
{
	kal_uint16 temp, i, otp_index;
	struct OV5647MIPI_otp_struct current_otp;
	kal_uint32 R_gain, B_gain, G_gain, G_gain_R,G_gain_B;

	SENSORDB("OV5647MIPI_update_wb_register_from_otp\n");

	//update white balance setting from OTP
	//check first wb OTP with valid OTP
	for(i = 0; i < 3; i++)
		{
			temp = OV5647MIPI_check_otp_wb(i);
			if(temp == 2)
				{
					otp_index = i;
					break;
				}
		}
	if( 3 == i)
		{
		 	SENSORDB("[OV5647MIPI_update_wb_register_from_otp]no valid wb OTP data!\r\n");
			return 1;
		}
	OV5647MIPI_read_otp_wb(otp_index,&current_otp);

	//calculate gain
	//0x400 = 1x gain
	if(current_otp.bg_ratio < tBG_Ratio_typical)
		{
			if(current_otp.rg_ratio < tRG_Ratio_typical)
				{
					//current_opt.bg_ratio < tBG_Ratio_typical &&
					//cuttent_otp.rg < tRG_Ratio_typical

					G_gain = 0x400;
					B_gain = 0x400 * tBG_Ratio_typical / current_otp.bg_ratio;
					R_gain = 0x400 * tRG_Ratio_typical / current_otp.rg_ratio;
				}
			else
				{
					//current_otp.bg_ratio < tBG_Ratio_typical &&
			        //current_otp.rg_ratio >= tRG_Ratio_typical
			        R_gain = 0x400;
					G_gain = 0x400 * current_otp.rg_ratio / tRG_Ratio_typical;
					B_gain = G_gain * tBG_Ratio_typical / current_otp.bg_ratio;
					
			        
				}
		}
	else
		{
			if(current_otp.rg_ratio < tRG_Ratio_typical)
				{
					//current_otp.bg_ratio >= tBG_Ratio_typical &&
			        //current_otp.rg_ratio < tRG_Ratio_typical
			        B_gain = 0x400;
					G_gain = 0x400 * current_otp.bg_ratio / tBG_Ratio_typical;
					R_gain = G_gain * tRG_Ratio_typical / current_otp.rg_ratio;
					
				}
			else
				{
					//current_otp.bg_ratio >= tBG_Ratio_typical &&
			        //current_otp.rg_ratio >= tRG_Ratio_typical
			        G_gain_B = 0x400*current_otp.bg_ratio / tBG_Ratio_typical;
				    G_gain_R = 0x400*current_otp.rg_ratio / tRG_Ratio_typical;
					
					if(G_gain_B > G_gain_R)
						{
							B_gain = 0x400;
							G_gain = G_gain_B;
							R_gain = G_gain * tRG_Ratio_typical / current_otp.rg_ratio;
						}
					else

						{
							R_gain = 0x400;
							G_gain = G_gain_R;
							B_gain = G_gain * tBG_Ratio_typical / current_otp.bg_ratio;
						}
			        
				}
			
		}
	//write sensor wb gain to register
	OV5647MIPI_update_wb_gain(R_gain,G_gain,B_gain);

	//success
	return 0;
}

#endif

#if defined(OV5647MIPI_USE_LENC_OTP)    //copy form OV5650 LENC OTP

//index:index of otp group.(0,1,2)
//return:	0.group index is empty.
//		1.group index has invalid data
//		2.group index has valid data

kal_uint16 OV5647MIPI_check_otp_lenc(kal_uint16 index)
{
   kal_uint16 temp,flag;
   kal_uint32 address;

   address = 0x20 + index*71;
   OV5647MIPI_write_cmos_sensor(0x3d00,address);
   
   flag = OV5647MIPI_read_cmos_sensor(0x3d04);
   flag = flag & 0xc0;

   OV5647MIPI_write_cmos_sensor(0x3d00,0);

   if(NULL == flag)
   	{
   		SENSORDB("[OV5647MIPI_check_otp_lenc]index[%x]read flag[%x][0]\n",index,flag);
   	    return 0;
   	}
   else if(0x40 == flag)
   	{
   		SENSORDB("[OV5647MIPI_check_otp_lenc]index[%x]read flag[%x][2]\n",index,flag);
   	    return 2;
   	}
   else
   	{
   		SENSORDB("[OV5647MIPI_check_otp_lenc]index[%x]read flag[%x][1]\n",index,flag);
		return 1;
   	}
}


kal_uint16 OV5647MIPI_read_otp_lenc(kal_uint16 index,struct OV5647MIPI_otp_struct *otp)
{
	kal_uint16 bank,temp1,temp2,i;
	kal_uint32 address;

	address = 0x20 + index*71 +1;
	
	

	//read lenc_g
	for(i = 0; i < 36; i++)
		{
			OV5647MIPI_write_cmos_sensor(0x3d00,address);
			otp->lenc_g[i] = OV5647MIPI_read_cmos_sensor(0x3d04);
			
			SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]otp->lenc_g[%d][%x]\n",address,i,otp->lenc_g[i]);
			address++;
		}
	//read lenc_b
	for(i = 0; i <8; i++)
		{
			OV5647MIPI_write_cmos_sensor(0x3d00,address);
			temp1 = OV5647MIPI_read_cmos_sensor(0x3d04);

			SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);

			address++;
			OV5647MIPI_write_cmos_sensor(0x3d00,address);
			temp2 = OV5647MIPI_read_cmos_sensor(0x3d04);

			SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp2[%x]\n",address,temp2);

			address++;

			otp->lenc_b[i*3] = temp1&0x1f;
			otp->lenc_b[i*3+1] = temp2&0x1f;
			otp->lenc_b[i*3+2] = (((temp1 >> 2)&0x18) | (temp2 >> 5));
		}
	OV5647MIPI_write_cmos_sensor(0x3d00,address);
	temp1 = OV5647MIPI_read_cmos_sensor(0x3d04);
	SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);
	otp->lenc_b[24] = temp1&0x1f;
	address++;

	//read lenc_r
	for(i = 0; i <8; i++)
		{
		   OV5647MIPI_write_cmos_sensor(0x3d00,address);
		   temp1 = OV5647MIPI_read_cmos_sensor(0x3d04);

		   SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);
		   
		   address++;

		   OV5647MIPI_write_cmos_sensor(0x3d00,address);
		   temp2 = OV5647MIPI_read_cmos_sensor(0x3d04);
		   
			
		   SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp2[%x]\n",address,temp2);
		   address++;

		   otp->lenc_r[i*3] = temp1&0x1f;
		   otp->lenc_r[i*3+1] = temp2&0x1f;
		   otp->lenc_r[i*3+2] = (((temp1 >> 2)&0x18) | (temp2 >>5));
		}
	OV5647MIPI_write_cmos_sensor(0x3d00,address);
	temp1 = OV5647MIPI_read_cmos_sensor(0x3d04);
	SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);
	otp->lenc_r[24] = temp1 & 0x1f;

	OV5647MIPI_write_cmos_sensor(0x3d00,0);

	return 0;
}


//return 0
kal_uint16 OV5647MIPI_update_lenc(struct OV5647MIPI_otp_struct *otp)
{
	kal_uint16 i, temp;
	//lenc g
	for(i = 0; i < 36; i++)
		{
		 	OV5647MIPI_write_cmos_sensor(0x5800+i,otp->lenc_g[i]);
			
			SENSORDB("[OV5647MIPI_update_lenc]otp->lenc_g[%d][%x]\n",i,otp->lenc_g[i]);
		}
	//lenc b
	for(i = 0; i < 25; i++)
		{
			OV5647MIPI_write_cmos_sensor(0x5824+i,otp->lenc_b[i]);
			SENSORDB("[OV5647MIPI_update_lenc]otp->lenc_b[%d][%x]\n",i,otp->lenc_b[i]);
		}
	//lenc r
	for(i = 0; i < 25; i++)
		{
			OV5647MIPI_write_cmos_sensor(0x583d+i,otp->lenc_r[i]);
			SENSORDB("[OV5647MIPI_update_lenc]otp->lenc_r[%d][%x]\n",i,otp->lenc_r[i]);
		}
	return 0;
}

//call this function after OV5647MIPI initialization
//return value:	0 update success
//				1 no otp

kal_uint16 OV5647MIPI_update_lenc_register_from_otp(void)
{
	kal_uint16 temp,i,otp_index;
    struct OV5647MIPI_otp_struct current_otp;

	for(i = 0; i < 3; i++)
		{
			temp = OV5647MIPI_check_otp_lenc(i);
			if(2 == temp)
				{
					otp_index = i;
					break;
				}
		}
	if(3 == i)
		{
		 	SENSORDB("[OV5647MIPI_update_lenc_register_from_otp]no valid wb OTP data!\r\n");
			return 1;
		}
	OV5647MIPI_read_otp_lenc(otp_index,&current_otp);

	OV5647MIPI_update_lenc(&current_otp);

	//at last should enable the shading enable register
	OV5647MIPI_read_cmos_sensor(0x5000);
	temp |= 0x80;
	OV5647MIPI_write_cmos_sensor(0x5000,temp);
	

	//success
	return 0;
}

#endif
#endif
static void OV5647MIPI_Write_Shutter(kal_uint16 iShutter)
{
	kal_uint16 extra_line = 0;

	
    /* 0x3500,0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
    /* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */
	if (!iShutter) iShutter = 1; /* avoid 0 */

	SENSORDB("OV5647MIPI_Write_Shutter :0x%x\n",iShutter);

	if(OV5647MIPIAutoFlicKerMode){
		if(OV5647MIPI_sensor.video_mode == KAL_FALSE){
		   if(CurrentScenarioId== MSDK_SCENARIO_ID_CAMERA_ZSD)
		   {
		      //Change frame 14.7fps ~ 14.9fps to do auto flick
		      OV5647MIPISetMaxFrameRate(148);
		   }
		   else
		   {
		      //Change frame 29.5fps ~ 29.8fps to do auto flick
					OV5647MIPISetMaxFrameRate(296);
			 }
		}
	}
	
	if(iShutter > OV5647MIPI_sensor.frame_height - 4)
		extra_line = iShutter - (OV5647MIPI_sensor.frame_height - 4);

	// Update Extra shutter
	OV5647MIPI_write_cmos_sensor(0x350c, (extra_line >> 8) & 0xFF);	
	OV5647MIPI_write_cmos_sensor(0x350d, (extra_line) & 0xFF);

	//Update Shutter
	OV5647MIPI_write_cmos_sensor(0x3500, (iShutter >> 12) & 0xF);
	OV5647MIPI_write_cmos_sensor(0x3501, (iShutter >> 4) & 0xFF);	
	OV5647MIPI_write_cmos_sensor(0x3502, (iShutter << 4) & 0xFF);

	#if 0

    printk("line_length[0x380c]0x:\n",OV5647MIPI_read_cmos_sensor(0x380c));
	printk("line_length[0x380d][%x]\n",OV5647MIPI_read_cmos_sensor(0x380d));
	printk("frame_height[0x380e][%x]\n",OV5647MIPI_read_cmos_sensor(0x380e));
	printk("frame_height[0x380d][%x]\n",OV5647MIPI_read_cmos_sensor(0x380d));


	printk("[OV5647_Write_Shutter]OV5647_sensor.pclk=%d\n",OV5647MIPI_sensor.pclk);
	printk("[OV5647_Write_Shutter]OV5647_sensor.line_length=%d\n",OV5647MIPI_sensor.line_length);
	printk("[OV5647_Write_Shutter]OV5647_sensor.frame_height=%d\n",OV5647MIPI_sensor.frame_height);
	printk("[OV5647_Write_Shutter]iShutter=%d\n",iShutter);

		{

	kal_uint16 fps;

	if(extra_line >0)
		{
		 	fps= (10 * OV5647MIPI_sensor.pclk) / iShutter / OV5647MIPI_sensor.line_length;		 
		}
	else
		{
			fps= (10 * OV5647MIPI_sensor.pclk) / OV5647MIPI_sensor.frame_height / OV5647MIPI_sensor.line_length;	
		}
	printk("[OV5647_Write_Shutter]fps=%d",fps);
			}



	#endif

}   /*  OV5647MIPI_Write_Shutter    */

static void OV5647MIPI_Set_Dummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
	kal_uint16 line_length, frame_height;
	SENSORDB("[soso][OV5647MIPI_Set_Dummy]iLines=%d, OV5647MIPI_sensor.pv_mode = %d \n",iLines, OV5647MIPI_sensor.pv_mode);

	if (OV5647MIPI_sensor.pv_mode){
	  line_length = OV5647MIPI_PV_PERIOD_PIXEL_NUMS + iPixels;
	  frame_height = OV5647MIPI_PV_PERIOD_LINE_NUMS + iLines;
	}
	else if (OV5647MIPI_sensor.video_mode) {
	  line_length = OV5647MIPI_VIDEO_PERIOD_PIXEL_NUMS + iPixels;
	  frame_height = OV5647MIPI_VIDEO_PERIOD_LINE_NUMS + iLines;
	}
	else{
	  line_length = OV5647MIPI_FULL_PERIOD_PIXEL_NUMS + iPixels;
	  frame_height = OV5647MIPI_FULL_PERIOD_LINE_NUMS + iLines;
	}
	
	if ((line_length >= 0x1FFF)||(frame_height >= 0xFFF)) {
		SENSORDB("[soso][OV5647MIPI_Set_Dummy] Error: line_length=%d, frame_height = %d \n",line_length, frame_height);
		}
	
    /*  Add dummy pixels: */
    /* 0x380c [0:4], 0x380d defines the PCLKs in one line of OV5647MIPI  */  
    /* Add dummy lines:*/
    /* 0x380e [0:1], 0x380f defines total lines in one frame of OV5647MIPI */
	SENSORDB("[soso][OV5647MIPI_Set_Dummy]line_length=%d, frame_height = %d \n",line_length, frame_height);
    OV5647MIPI_write_cmos_sensor(0x380c, line_length >> 8);
    OV5647MIPI_write_cmos_sensor(0x380d, line_length & 0xFF);
    OV5647MIPI_write_cmos_sensor(0x380e, frame_height >> 8);
    OV5647MIPI_write_cmos_sensor(0x380f, frame_height & 0xFF);
	
}   /*  OV5647MIPI_Set_Dummy    */

/*Avoid Folat, frame rate =10 * u2FrameRate */
UINT32 OV5647MIPISetMaxFrameRate(UINT16 u2FrameRate)
{
	kal_int16 dummy_line=0;
	kal_uint16 FrameHeight = OV5647MIPI_sensor.frame_height;
	  unsigned long flags;
		
	SENSORDB("[soso][OV5647MIPISetMaxFrameRate]u2FrameRate=%d \n",u2FrameRate);

	//dummy_line = OV5647MIPI_sensor.pclk / u2FrameRate / OV5647MIPI_PV_PERIOD_PIXEL_NUMS - OV5647MIPI_PV_PERIOD_LINE_NUMS;
	FrameHeight= (10 * OV5647MIPI_sensor.pclk) / u2FrameRate / OV5647MIPI_sensor.line_length;
	if(KAL_FALSE == OV5647MIPI_sensor.pv_mode){
		if(KAL_FALSE == OV5647MIPI_sensor.video_mode){
		if(FrameHeight < OV5647MIPI_FULL_PERIOD_LINE_NUMS)
			FrameHeight = OV5647MIPI_FULL_PERIOD_LINE_NUMS;
			}
		else{
			if(FrameHeight < OV5647MIPI_VIDEO_PERIOD_LINE_NUMS)
				FrameHeight = OV5647MIPI_VIDEO_PERIOD_LINE_NUMS;
			}
	}
	    spin_lock_irqsave(&ov5647mipi_drv_lock,flags);
		OV5647MIPI_sensor.frame_height = FrameHeight;
	spin_unlock_irqrestore(&ov5647mipi_drv_lock,flags);
		if((CurrentScenarioId == MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG)||(CurrentScenarioId == MSDK_SCENARIO_ID_CAMERA_ZSD)){
		  dummy_line = FrameHeight - OV5647MIPI_FULL_PERIOD_LINE_NUMS;
		}
		else if(CurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_PREVIEW){
			dummy_line = FrameHeight - OV5647MIPI_PV_PERIOD_LINE_NUMS;
		}
		else if(CurrentScenarioId==MSDK_SCENARIO_ID_VIDEO_PREVIEW) {
			dummy_line = FrameHeight - OV5647MIPI_VIDEO_PERIOD_LINE_NUMS;
		}
		SENSORDB("[soso][OV5647MIPISetMaxFrameRate]frameheight = %d, dummy_line=%d \n",OV5647MIPI_sensor.frame_height,dummy_line);
		if(dummy_line<0) {
			dummy_line = 0;
		}
	    /* to fix VSYNC, to fix frame rate */
		OV5647MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
	//}
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	OV5647MIPI_SetShutter
*
* DESCRIPTION
*	This function set e-shutter of OV5647MIPI to change exposure time.
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
void set_OV5647MIPI_shutter(kal_uint16 iShutter)
{
	unsigned long flags;
	SENSORDB("set_OV5647MIPI_shutter :0x%x\n",iShutter);
    spin_lock_irqsave(&ov5647mipi_drv_lock,flags);
	OV5647MIPI_sensor.shutter = iShutter;
	spin_unlock_irqrestore(&ov5647mipi_drv_lock,flags);
    OV5647MIPI_Write_Shutter(iShutter);
}   /*  Set_OV5647MIPI_Shutter */

/*
static kal_uint16 OV5647MIPIReg2Gain(const kal_uint8 iReg)
{
    kal_uint16 iGain ;   // Range: 1x to 32x 
    
	iGain = (iReg >> 4) * BASEGAIN + (iReg & 0xF) * BASEGAIN / 16; 
    return iGain ;
}
 */

 kal_uint8 OV5647MIPIGain2Reg(const kal_uint16 iGain)
{
    kal_uint16 iReg = 0x00;
	iReg = ((iGain / BASEGAIN) << 4) + ((iGain % BASEGAIN) * 16 / BASEGAIN);
	iReg = iReg & 0xFF;
    return (kal_uint8)iReg;
}

/*************************************************************************
* FUNCTION
*	OV5647MIPI_SetGain
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
kal_uint16 OV5647MIPI_SetGain(kal_uint16 iGain)
{
	kal_uint8 iReg;
	//V5647_sensor.gain = iGain;
	/* 0x350a[0:1], 0x350b AGC real gain */
	/* [0:3] = N meams N /16 X  */
	/* [4:9] = M meams M X  */
	/* Total gain = M + N /16 X */
	SENSORDB("OV5747MIPI_SetGain::%x \n",iGain);
	iReg = OV5647MIPIGain2Reg(iGain);
	SENSORDB("OV5647MIPI_SetGain,iReg:%x",iReg);
	if (iReg < 0x10) iReg = 0x10;
	//OV5647MIPI_write_cmos_sensor(0x350a, iReg);
	OV5647MIPI_write_cmos_sensor(0x350b, iReg);
	return iGain;
}
/*************************************************************************
* FUNCTION
*	OV5647MIPI_NightMode
*
* DESCRIPTION
*	This function night mode of OV5647MIPI.
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
void OV5647MIPI_night_mode(kal_bool enable)
{
/*No Need to implement this function*/
#if 0 
	const kal_uint16 dummy_pixel = OV5647MIPI_sensor.line_length - OV5647MIPI_PV_PERIOD_PIXEL_NUMS;
	const kal_uint16 pv_min_fps =  enable ? OV5647MIPI_sensor.night_fps : OV5647MIPI_sensor.normal_fps;
	kal_uint16 dummy_line = OV5647MIPI_sensor.frame_height - OV5647MIPI_PV_PERIOD_LINE_NUMS;
	kal_uint16 max_exposure_lines;
	
	printk("[soso][OV5647MIPI_night_mode]enable=%d",enable);
	if (!OV5647MIPI_sensor.video_mode) return;
	max_exposure_lines = OV5647MIPI_sensor.pclk * OV5647MIPI_FPS(1) / (pv_min_fps * OV5647MIPI_sensor.line_length);
	if (max_exposure_lines > OV5647MIPI_sensor.frame_height) /* fix max frame rate, AE table will fix min frame rate */
//	{
//	  dummy_line = max_exposure_lines - OV5647MIPI_PV_PERIOD_LINE_NUMS;
//	  OV5647MIPI_Set_Dummy(dummy_pixel, dummy_line);
//	}
#endif
}   /*  OV5647MIPI_NightMode    */


/* write camera_para to sensor register */
static void OV5647MIPI_camera_para_to_sensor(void)
{
  kal_uint32 i;
#ifdef OV5647MIPI_DRIVER_TRACE
	 SENSORDB("OV5647MIPI_camera_para_to_sensor\n");
#endif
  for (i = 0; 0xFFFFFFFF != OV5647MIPI_sensor.eng.reg[i].Addr; i++)
  {
    OV5647MIPI_write_cmos_sensor(OV5647MIPI_sensor.eng.reg[i].Addr, OV5647MIPI_sensor.eng.reg[i].Para);
  }
  for (i = OV5647MIPI_FACTORY_START_ADDR; 0xFFFFFFFF != OV5647MIPI_sensor.eng.reg[i].Addr; i++)
  {
    OV5647MIPI_write_cmos_sensor(OV5647MIPI_sensor.eng.reg[i].Addr, OV5647MIPI_sensor.eng.reg[i].Para);
  }
  OV5647MIPI_SetGain(OV5647MIPI_sensor.gain); /* update gain */
}

/* update camera_para from sensor register */
static void OV5647MIPI_sensor_to_camera_para(void)
{
  kal_uint32 i;
#ifdef OV5647MIPI_DRIVER_TRACE
   SENSORDB("OV5647MIPI_sensor_to_camera_para\n");
#endif
  for (i = 0; 0xFFFFFFFF != OV5647MIPI_sensor.eng.reg[i].Addr; i++)
  {
    OV5647MIPI_sensor.eng.reg[i].Para = OV5647MIPI_read_cmos_sensor(OV5647MIPI_sensor.eng.reg[i].Addr);
  }
  for (i = OV5647MIPI_FACTORY_START_ADDR; 0xFFFFFFFF != OV5647MIPI_sensor.eng.reg[i].Addr; i++)
  {
    OV5647MIPI_sensor.eng.reg[i].Para = OV5647MIPI_read_cmos_sensor(OV5647MIPI_sensor.eng.reg[i].Addr);
  }
}

/* ------------------------ Engineer mode ------------------------ */
inline static void OV5647MIPI_get_sensor_group_count(kal_int32 *sensor_count_ptr)
{
#ifdef OV5647MIPI_DRIVER_TRACE
   SENSORDB("OV5647MIPI_get_sensor_group_count\n");
#endif
  *sensor_count_ptr = OV5647MIPI_GROUP_TOTAL_NUMS;
}

inline static void OV5647MIPI_get_sensor_group_info(MSDK_SENSOR_GROUP_INFO_STRUCT *para)
{
#ifdef OV5647MIPI_DRIVER_TRACE
   SENSORDB("OV5647MIPI_get_sensor_group_info\n");
#endif
  switch (para->GroupIdx)
  {
  case OV5647MIPI_PRE_GAIN:
    sprintf(para->GroupNamePtr, "CCT");
    para->ItemCount = 5;
    break;
  case OV5647MIPI_CMMCLK_CURRENT:
    sprintf(para->GroupNamePtr, "CMMCLK Current");
    para->ItemCount = 1;
    break;
  case OV5647MIPI_FRAME_RATE_LIMITATION:
    sprintf(para->GroupNamePtr, "Frame Rate Limitation");
    para->ItemCount = 2;
    break;
  case OV5647MIPI_REGISTER_EDITOR:
    sprintf(para->GroupNamePtr, "Register Editor");
    para->ItemCount = 2;
    break;
  default:
    ASSERT(0);
  }
}

inline static void OV5647MIPI_get_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{

  const static kal_char *cct_item_name[] = {"SENSOR_BASEGAIN", "Pregain-R", "Pregain-Gr", "Pregain-Gb", "Pregain-B"};
  const static kal_char *editer_item_name[] = {"REG addr", "REG value"};
  
#ifdef OV5647MIPI_DRIVER_TRACE
	 SENSORDB("OV5647MIPI_get_sensor_item_info\n");
#endif
  switch (para->GroupIdx)
  {
  case OV5647MIPI_PRE_GAIN:
    switch (para->ItemIdx)
    {
    case OV5647MIPI_SENSOR_BASEGAIN:
    case OV5647MIPI_PRE_GAIN_R_INDEX:
    case OV5647MIPI_PRE_GAIN_Gr_INDEX:
    case OV5647MIPI_PRE_GAIN_Gb_INDEX:
    case OV5647MIPI_PRE_GAIN_B_INDEX:
      break;
    default:
      ASSERT(0);
    }
    sprintf(para->ItemNamePtr, cct_item_name[para->ItemIdx - OV5647MIPI_SENSOR_BASEGAIN]);
    para->ItemValue = OV5647MIPI_sensor.eng.cct[para->ItemIdx].Para * 1000 / BASEGAIN;
    para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
    para->Min = OV5647MIPI_MIN_ANALOG_GAIN * 1000;
    para->Max = OV5647MIPI_MAX_ANALOG_GAIN * 1000;
    break;
  case OV5647MIPI_CMMCLK_CURRENT:
    switch (para->ItemIdx)
    {
    case 0:
      sprintf(para->ItemNamePtr, "Drv Cur[2,4,6,8]mA");
      switch (OV5647MIPI_sensor.eng.reg[OV5647MIPI_CMMCLK_CURRENT_INDEX].Para)
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
  case OV5647MIPI_FRAME_RATE_LIMITATION:
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
  case OV5647MIPI_REGISTER_EDITOR:
    switch (para->ItemIdx)
    {
    case 0:
    case 1:
      sprintf(para->ItemNamePtr, editer_item_name[para->ItemIdx]);
      para->ItemValue = 0;
      para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
      para->Min = 0;
      para->Max = (para->ItemIdx == 0 ? 0xFFFF : 0xFF);
      break;
    default:
      ASSERT(0);
    }
    break;
  default:
    ASSERT(0);
  }
}

inline static kal_bool OV5647MIPI_set_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{
  kal_uint16 temp_para;
#ifdef OV5647MIPI_DRIVER_TRACE
   SENSORDB("OV5647MIPI_set_sensor_item_info\n");
#endif
  switch (para->GroupIdx)
  {
  case OV5647MIPI_PRE_GAIN:
    switch (para->ItemIdx)
    {
    case OV5647MIPI_SENSOR_BASEGAIN:
    case OV5647MIPI_PRE_GAIN_R_INDEX:
    case OV5647MIPI_PRE_GAIN_Gr_INDEX:
    case OV5647MIPI_PRE_GAIN_Gb_INDEX:
    case OV5647MIPI_PRE_GAIN_B_INDEX:
	  spin_lock(&ov5647mipi_drv_lock);
      OV5647MIPI_sensor.eng.cct[para->ItemIdx].Para = para->ItemValue * BASEGAIN / 1000;
      spin_unlock(&ov5647mipi_drv_lock);
      OV5647MIPI_SetGain(OV5647MIPI_sensor.gain); /* update gain */
      break;
    default:
      ASSERT(0);
    }
    break;
  case OV5647MIPI_CMMCLK_CURRENT:
    switch (para->ItemIdx)
    {
    case 0:
      switch (para->ItemValue)
      {
      case 2:
        temp_para = ISP_DRIVING_2MA;
        break;
      case 3:
      case 4:
        temp_para = ISP_DRIVING_4MA;
        break;
      case 5:
      case 6:
        temp_para = ISP_DRIVING_6MA;
        break;
      default:
        temp_para = ISP_DRIVING_8MA;
        break;
      }
	  spin_lock(&ov5647mipi_drv_lock);
      //OV5647MIPI_set_isp_driving_current(temp_para);
      OV5647MIPI_sensor.eng.reg[OV5647MIPI_CMMCLK_CURRENT_INDEX].Para = temp_para;
	  spin_unlock(&ov5647mipi_drv_lock);
      break;
    default:
      ASSERT(0);
    }
    break;
  case OV5647MIPI_FRAME_RATE_LIMITATION:
    ASSERT(0);
    break;
  case OV5647MIPI_REGISTER_EDITOR:
    switch (para->ItemIdx)
    {
      static kal_uint32 fac_sensor_reg;
    case 0:
      if (para->ItemValue < 0 || para->ItemValue > 0xFFFF) return KAL_FALSE;
      fac_sensor_reg = para->ItemValue;
      break;
    case 1:
      if (para->ItemValue < 0 || para->ItemValue > 0xFF) return KAL_FALSE;
      OV5647MIPI_write_cmos_sensor(fac_sensor_reg, para->ItemValue);
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

static void OV5647MIPI_Sensor_Init(void)
{

  //@@ global setting																  
OV5647MIPI_write_cmos_sensor(0x0100,0x00);
OV5647MIPI_write_cmos_sensor(0x0103,0x01);
	//(5ms)		
//	kal_sleep_task(2);
    mdelay(6);


	OV5647MIPI_write_cmos_sensor(0x3035, 0x11); //  system clk div
	OV5647MIPI_write_cmos_sensor(0x303c, 0x11); //PLL ctrol
	OV5647MIPI_write_cmos_sensor(0x370c, 0x03); // no
	
	OV5647MIPI_write_cmos_sensor(0x5000, 0x06); //isp control
OV5647MIPI_write_cmos_sensor(0x5003,0x08);
OV5647MIPI_write_cmos_sensor(0x5a00,0x08);
	
	OV5647MIPI_write_cmos_sensor(0x3000, 0xff); //system control
OV5647MIPI_write_cmos_sensor(0x3001,0xff);
OV5647MIPI_write_cmos_sensor(0x3002,0xff);
OV5647MIPI_write_cmos_sensor(0x301d,0xf0);
OV5647MIPI_write_cmos_sensor(0x3a18,0x00);
OV5647MIPI_write_cmos_sensor(0x3a19,0xf8);
	//OV5647MIPI_write_cmos_sensor(0x3a18, 0x01);
	//OV5647MIPI_write_cmos_sensor(0x3a19, 0xe0);
	
	
OV5647MIPI_write_cmos_sensor(0x3c01,0x80);
OV5647MIPI_write_cmos_sensor(0x3b07,0x0c);
OV5647MIPI_write_cmos_sensor(0x3708,0x64);
OV5647MIPI_write_cmos_sensor(0x3630,0x2e);
OV5647MIPI_write_cmos_sensor(0x3632,0xe2);
OV5647MIPI_write_cmos_sensor(0x3633,0x23);
OV5647MIPI_write_cmos_sensor(0x3634,0x44);
OV5647MIPI_write_cmos_sensor(0x3620,0x64);
OV5647MIPI_write_cmos_sensor(0x3621,0xe0);
OV5647MIPI_write_cmos_sensor(0x3600,0x37);
OV5647MIPI_write_cmos_sensor(0x3704,0xa0);
OV5647MIPI_write_cmos_sensor(0x3703,0x5a);
OV5647MIPI_write_cmos_sensor(0x3715,0x78);
OV5647MIPI_write_cmos_sensor(0x3717,0x01);
OV5647MIPI_write_cmos_sensor(0x3731,0x02);
OV5647MIPI_write_cmos_sensor(0x370b,0x60);
OV5647MIPI_write_cmos_sensor(0x3705,0x1a);
OV5647MIPI_write_cmos_sensor(0x3f05,0x02);
OV5647MIPI_write_cmos_sensor(0x3f06,0x10);
OV5647MIPI_write_cmos_sensor(0x3f01,0x0a);
	//OV5647MIPI_write_cmos_sensor(0x3a08, 0x01);
	OV5647MIPI_write_cmos_sensor(0x3a08, 0x00);
OV5647MIPI_write_cmos_sensor(0x3a0f,0x58);
OV5647MIPI_write_cmos_sensor(0x3a10,0x50);
OV5647MIPI_write_cmos_sensor(0x3a1b,0x58);
OV5647MIPI_write_cmos_sensor(0x3a1e,0x50);
OV5647MIPI_write_cmos_sensor(0x3a11,0x60);
OV5647MIPI_write_cmos_sensor(0x3a1f,0x28);
	
	OV5647MIPI_write_cmos_sensor(0x4001, 0x02);  //blc
OV5647MIPI_write_cmos_sensor(0x4000,0x09);

OV5647MIPI_write_cmos_sensor(0x3000,0x00);
OV5647MIPI_write_cmos_sensor(0x3001,0x00);
	OV5647MIPI_write_cmos_sensor(0x3002, 0x00); //modify
OV5647MIPI_write_cmos_sensor(0x3017,0xe0);
OV5647MIPI_write_cmos_sensor(0x301c,0xfc);
OV5647MIPI_write_cmos_sensor(0x3636,0x06);
OV5647MIPI_write_cmos_sensor(0x3016,0x08);
OV5647MIPI_write_cmos_sensor(0x3827,0xec);
OV5647MIPI_write_cmos_sensor(0x3018,0x44);
    OV5647MIPI_write_cmos_sensor(0x3035, 0x21);
	OV5647MIPI_write_cmos_sensor(0x3106, 0xf5);
    OV5647MIPI_write_cmos_sensor(0x3034, 0x1a);
OV5647MIPI_write_cmos_sensor(0x301c,0xf8);
	
 
	
	OV5647MIPI_write_cmos_sensor(0x3503, 0x07);
	//OV5647MIPI_write_cmos_sensor(0x3501, 0x3c);
	//OV5647MIPI_write_cmos_sensor(0x3502, 0x00);

	OV5647MIPI_write_cmos_sensor(0x3501, 0x10);
	OV5647MIPI_write_cmos_sensor(0x3502, 0x80);
	
	OV5647MIPI_write_cmos_sensor(0x350a, 0x00);
	OV5647MIPI_write_cmos_sensor(0x350b, 0x7f);
	OV5647MIPI_write_cmos_sensor(0x5001, 0x01);
	OV5647MIPI_write_cmos_sensor(0x5180, 0x08);
	OV5647MIPI_write_cmos_sensor(0x5186, 0x04);
	OV5647MIPI_write_cmos_sensor(0x5187, 0x00);
	OV5647MIPI_write_cmos_sensor(0x5188, 0x04);
	OV5647MIPI_write_cmos_sensor(0x5189, 0x00);
	OV5647MIPI_write_cmos_sensor(0x518a, 0x04);
	OV5647MIPI_write_cmos_sensor(0x518b, 0x00);
	OV5647MIPI_write_cmos_sensor(0x5000, 0x06);
	OV5647MIPI_write_cmos_sensor(0x4800, 0x14);//JH short packet
	OV5647MIPI_write_cmos_sensor(0x4001, 0x02);		 //add							  
	OV5647MIPI_write_cmos_sensor(0x4000, 0x09);		//add							  
	//OV5647MIPI_write_cmos_sensor(0x0010, 0x01);	    //add

	OV5647MIPI_write_cmos_sensor(0x3013,0x00);//liurui modfiy
	OV5647MIPI_write_cmos_sensor(0x4005,0x18);//gain triger
	OV5647MIPI_write_cmos_sensor(0x4050,0x37);//blc max
	OV5647MIPI_write_cmos_sensor(0x4051,0x8f);//blc level trigger
	OV5647MIPI_write_cmos_sensor(0x0100,0x01);   //modify
	

}   /*  OV5647MIPI_Sensor_Init  */   /*  OV5647MIPI_Sensor_Init  */
static void OV5647MIPI_Sensor_1080P(void) 
{
	SENSORDB("OV5647MIPIVideo Setting \n");

OV5647MIPI_write_cmos_sensor(0x0100,0x00);      
OV5647MIPI_write_cmos_sensor(0x3036, 0x64);
OV5647MIPI_write_cmos_sensor(0x3612, 0x5b);
OV5647MIPI_write_cmos_sensor(0x3618, 0x04);
OV5647MIPI_write_cmos_sensor(0x3709, 0x12);
OV5647MIPI_write_cmos_sensor(0x3800, 0x01);
OV5647MIPI_write_cmos_sensor(0x3801, 0x5c);
OV5647MIPI_write_cmos_sensor(0x3802, 0x01);
OV5647MIPI_write_cmos_sensor(0x3803, 0xb2);
OV5647MIPI_write_cmos_sensor(0x3804, 0x08);
OV5647MIPI_write_cmos_sensor(0x3805, 0xe3);
OV5647MIPI_write_cmos_sensor(0x3806, 0x05);
OV5647MIPI_write_cmos_sensor(0x3807, 0xf1);
OV5647MIPI_write_cmos_sensor(0x3808,0x07);
OV5647MIPI_write_cmos_sensor(0x3809,0x80);
OV5647MIPI_write_cmos_sensor(0x380a,0x04);
OV5647MIPI_write_cmos_sensor(0x380b,0x38);       
OV5647MIPI_write_cmos_sensor(0x380c,0x09);       
OV5647MIPI_write_cmos_sensor(0x380d,0x70);
OV5647MIPI_write_cmos_sensor(0x380e,0x04);      
OV5647MIPI_write_cmos_sensor(0x380f,0x50);       
OV5647MIPI_write_cmos_sensor(0x3814,0x11);
OV5647MIPI_write_cmos_sensor(0x3815,0x11);              
OV5647MIPI_write_cmos_sensor(0x3821,0x00);//0x01
OV5647MIPI_write_cmos_sensor(0x3820,0x06);//0x47
OV5647MIPI_write_cmos_sensor(0x3a09,0x4b);
OV5647MIPI_write_cmos_sensor(0x3a0a,0x01);
OV5647MIPI_write_cmos_sensor(0x3a0b,0x13);       
OV5647MIPI_write_cmos_sensor(0x3a0d,0x04);
OV5647MIPI_write_cmos_sensor(0x3a0e,0x03);       
OV5647MIPI_write_cmos_sensor(0x4004, 0x04);
OV5647MIPI_write_cmos_sensor(0x4005,0x18);//gain triger
OV5647MIPI_write_cmos_sensor(0x4837,0x19);             
OV5647MIPI_write_cmos_sensor(0x0100,0x01);


}

static void OV5647MIPI_Sensor_1M(void)
{
	//-------------------------------------------------------------------------------
	// PLL MY_OUTPUT clock(fclk)
	// fclk = (0x40 - 0x300E[5:0]) x N x Bit8Div x MCLK / M, where
	//		N = 1, 1.5, 2, 3 for 0x300F[7:6] = 0~3, respectively
	//		M = 1, 1.5, 2, 3 for 0x300F[1:0] = 0~3, respectively
	//		Bit8Div = 1, 1, 4, 5 for 0x300F[5:4] = 0~3, respectively
	// Sys Clk = fclk / Bit8Div / SenDiv
	// Sensor MY_OUTPUT clock(DVP PCLK)
	// DVP PCLK = ISP CLK / DVPDiv, where
	//		ISP CLK =  fclk / Bit8Div / SenDiv / CLKDiv / 2, where
	//			Bit8Div = 1, 1, 4, 5 for 0x300F[5:4] = 0~3, respectively
	//			SenDiv = 1, 2 for 0x3010[4] = 0 or 1 repectively
	//			CLKDiv = (0x3011[5:0] + 1)
	//		DVPDiv = 0x304C[3:0] * (2 ^ 0x304C[4]), if 0x304C[3:0] = 0, use 16 instead
	//
	// Base shutter calculation
	//		60Hz: (1/120) * ISP Clk / QXGA_MODE_WITHOUT_DUMMY_PIXELS
	//		50Hz: (1/100) * ISP Clk / QXGA_MODE_WITHOUT_DUMMY_PIXELS
	//-------------------------------------------------------------------------------
	
	
	SENSORDB("OV5647MIPIPreview Setting \n");
	OV5647MIPI_write_cmos_sensor(0x0100, 0x00);	
    OV5647MIPI_write_cmos_sensor(0x3036, 0x46);	
OV5647MIPI_write_cmos_sensor(0x3612,0x59);
	OV5647MIPI_write_cmos_sensor(0x3618, 0x00);	
OV5647MIPI_write_cmos_sensor(0x3709,0x52);
OV5647MIPI_write_cmos_sensor(0x3800,0x00);
OV5647MIPI_write_cmos_sensor(0x3801,0x08);
OV5647MIPI_write_cmos_sensor(0x3802,0x00);
OV5647MIPI_write_cmos_sensor(0x3803,0x02);
OV5647MIPI_write_cmos_sensor(0x3804,0x0a);
OV5647MIPI_write_cmos_sensor(0x3805,0x37);
OV5647MIPI_write_cmos_sensor(0x3806,0x07);
OV5647MIPI_write_cmos_sensor(0x3807,0xa1);
OV5647MIPI_write_cmos_sensor(0x3808,0x05);
OV5647MIPI_write_cmos_sensor(0x3809,0x10);
OV5647MIPI_write_cmos_sensor(0x380a,0x03);
OV5647MIPI_write_cmos_sensor(0x380b,0xcc);
OV5647MIPI_write_cmos_sensor(0x380c,0x07);
OV5647MIPI_write_cmos_sensor(0x380d,0x68);
OV5647MIPI_write_cmos_sensor(0x380e,0x03);
OV5647MIPI_write_cmos_sensor(0x380f,0xd8);
	OV5647MIPI_write_cmos_sensor(0x3814, 0x31);	
	OV5647MIPI_write_cmos_sensor(0x3815, 0x31);	
OV5647MIPI_write_cmos_sensor(0x3821,0x01);
OV5647MIPI_write_cmos_sensor(0x3820,0x47);
OV5647MIPI_write_cmos_sensor(0x3a09,0x27);
OV5647MIPI_write_cmos_sensor(0x3a0a,0x00);
OV5647MIPI_write_cmos_sensor(0x3a0b,0xf6);
OV5647MIPI_write_cmos_sensor(0x3a0d,0x04);
OV5647MIPI_write_cmos_sensor(0x3a0e,0x03);
	OV5647MIPI_write_cmos_sensor(0x4004, 0x02);	
	OV5647MIPI_write_cmos_sensor(0x4005,0x18);//gain triger
	OV5647MIPI_write_cmos_sensor(0x4837, 0x23);
	OV5647MIPI_write_cmos_sensor(0x0100, 0x01);	


}


static void OV5647MIPI_Sensor_5M(void)
{
    SENSORDB("OV5647MIPICapture Setting\n");

  
/*
;----------------------------------------------
; 2592x1944_2lane setting
; Mipi: 2 lane
: Mipi data rate: 280Mbps/lane 
; SystemCLK     :56Mhz
; FPS	        :7.26
; HTS		:3468 (R380c:R380d) 
; VTS		:2224 (R380e:R380f)
; Tline 	:61.93us
;---------------------------------------------
*/
		
	OV5647MIPI_write_cmos_sensor(0x0100,0x00); 	
	OV5647MIPI_write_cmos_sensor(0x3036,0x64); 	
OV5647MIPI_write_cmos_sensor(0x3612,0x5b);
	OV5647MIPI_write_cmos_sensor(0x3618,0x04); 
OV5647MIPI_write_cmos_sensor(0x3709,0x12);      
OV5647MIPI_write_cmos_sensor(0x3800,0x00);
OV5647MIPI_write_cmos_sensor(0x3801,0x0c);
OV5647MIPI_write_cmos_sensor(0x3802,0x00);
OV5647MIPI_write_cmos_sensor(0x3803,0x04);
OV5647MIPI_write_cmos_sensor(0x3804,0x0a);
OV5647MIPI_write_cmos_sensor(0x3805,0x33);
OV5647MIPI_write_cmos_sensor(0x3806,0x07);
OV5647MIPI_write_cmos_sensor(0x3807,0xa3);
OV5647MIPI_write_cmos_sensor(0x3808,0x0a);
OV5647MIPI_write_cmos_sensor(0x3809,0x20);
OV5647MIPI_write_cmos_sensor(0x380a,0x07);
OV5647MIPI_write_cmos_sensor(0x380b,0x98);      
	OV5647MIPI_write_cmos_sensor(0x380c,0x0a); 	
OV5647MIPI_write_cmos_sensor(0x380d,0xc0);
	OV5647MIPI_write_cmos_sensor(0x380e,0x07); 	
	OV5647MIPI_write_cmos_sensor(0x380f,0xb6); 	
	OV5647MIPI_write_cmos_sensor(0x3814,0x11); 	
	OV5647MIPI_write_cmos_sensor(0x3815,0x11); 	
OV5647MIPI_write_cmos_sensor(0x3821,0x00);//0x01
OV5647MIPI_write_cmos_sensor(0x3820,0x06); //0x47     
OV5647MIPI_write_cmos_sensor(0x3a09,0x28);       
OV5647MIPI_write_cmos_sensor(0x3a0a,0x00);
OV5647MIPI_write_cmos_sensor(0x3a0b,0xf6);       
OV5647MIPI_write_cmos_sensor(0x3a0d,0x08);
OV5647MIPI_write_cmos_sensor(0x3a0e,0x06);       
	OV5647MIPI_write_cmos_sensor(0x4004,0x04); 
	OV5647MIPI_write_cmos_sensor(0x4005,0x1a);//always triger
	OV5647MIPI_write_cmos_sensor(0x4837,0x19);
	OV5647MIPI_write_cmos_sensor(0x0100,0x01); 	
	
}

/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*	OV5647MIPIOpen
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

UINT32 OV5647MIPIOpen(void)
{
	kal_uint16 sensor_id=0; 

#ifdef OV5647MIPI_USE_OTP	
	kal_uint16 otp_ret = 0;
	kal_uint16 ret = 0;
#endif

	// check if sensor ID correct
	sensor_id=((OV5647MIPI_read_cmos_sensor(0x300A) << 8) | OV5647MIPI_read_cmos_sensor(0x300B));   
#ifdef OV5647MIPI_DRIVER_TRACE
	SENSORDB("OV5647MIPIOpen, sensor_id:%x \n",sensor_id);
#endif		
	if (sensor_id != OV5647MIPI_SENSOR_ID)
		return ERROR_SENSOR_CONNECT_FAIL;
	
	/* initail sequence write in  */
	OV5647MIPI_Sensor_Init();
	#ifdef OV5647MIPI_USE_OTP

	#ifdef OV5647MIPI_USE_AWB_OTP
	
		 otp_ret = OV5647MIPI_update_wb_register_from_otp();
		if(1 == otp_ret)
			{
				SENSORDB("OV5647MIPI_update_wb_register_from_otp invalid\n");
			}
		else if(0 == otp_ret)
			{
				SENSORDB("OV5647MIPI_update_wb_register_from_otp success\n");
			}
	#endif

	#ifdef OV5647MIPI_USE_LENC_OTP   //copy form OV5650 LENC OTP
	

		ret = OV5647MIPI_update_lenc_register_from_otp();
		if(1 == ret)
			{
				SENSORDB("OV5647MIPI_update_lenc_register_from_otp invalid\n");
			}
		else if(0 == ret)
			{
				SENSORDB("OV5647MIPI_update_lenc_register_from_otp success\n");
			}
	#endif
	
#endif
    spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPIAutoFlicKerMode = KAL_FALSE;
	spin_unlock(&ov5647mipi_drv_lock);
	//OV5647MIPIZsdCameraPreview= KAL_FALSE;
	return ERROR_NONE;
}   /* OV5647MIPIOpen  */

/*************************************************************************
* FUNCTION
*   OV5642GetSensorID
*
* DESCRIPTION
*   This function get the sensor ID 
*
* PARAMETERS
*   *sensorID : return the sensor ID 
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5647MIPIGetSensorID(UINT32 *sensorID) 
{
		// check if sensor ID correct
	*sensorID=((OV5647MIPI_read_cmos_sensor(0x300A) << 8) | OV5647MIPI_read_cmos_sensor(0x300B));	
#ifdef OV5647MIPI_DRIVER_TRACE
	SENSORDB("OV5647MIPIOpen, sensor_id:%x \n",*sensorID);
#endif		
	if (*sensorID != OV5647MIPI_SENSOR_ID) {		
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	
   return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	OV5647MIPIClose
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
UINT32 OV5647MIPIClose(void)
{
#ifdef OV5647MIPI_DRIVER_TRACE
   SENSORDB("OV5647MIPIClose\n");
#endif
  //CISModulePowerOn(FALSE);
//	DRV_I2CClose(OV5647MIPIhDrvI2C);
	return ERROR_NONE;
}   /* OV5647MIPIClose */

/*************************************************************************
* FUNCTION
* OV5647MIPIPreview
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
UINT32 OV5647MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 dummy_line=0;
	
	OV5647MIPI_Sensor_1M();
	spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPI_sensor.pv_mode = KAL_TRUE;
	OV5647MIPI_sensor.video_mode = KAL_FALSE;
	spin_unlock(&ov5647mipi_drv_lock);
	//OV5647MIPIZsdCameraPreview=KAL_FALSE;
	
	//OV5647MIPI_set_mirror(sensor_config_data->SensorImageMirror);
     spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPI_sensor.line_length = OV5647MIPI_PV_PERIOD_PIXEL_NUMS;
	OV5647MIPI_sensor.frame_height = OV5647MIPI_PV_PERIOD_LINE_NUMS+dummy_line;

	OV5647MIPI_sensor.pclk = OV5647MIPI_PREVIEW_CLK;
	spin_unlock(&ov5647mipi_drv_lock);

	OV5647MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
	//OV5647MIPI_Write_Shutter(OV5647MIPI_sensor.shutter);
	msleep(40);
	//printk("[soso][OV5647MIPIPreview]shutter=%x,shutter=%d\n",OV5647MIPI_sensor.shutter,OV5647MIPI_sensor.shutter);
	return ERROR_NONE;
}   /*  OV5647MIPIPreview   */

UINT32 OV5647MIPIZsdPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

   kal_uint16 dummy_line=0;
	
#ifdef OV5647MIPI_DRIVER_TRACE
	SENSORDB("OV5647MIPIZsdPreview \n");
#endif

   //OV5647MIPIZsdCameraPreview = KAL_TRUE;
	OV5647MIPI_Sensor_5M();
	spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPI_sensor.pv_mode = KAL_FALSE;
   OV5647MIPI_sensor.video_mode = KAL_FALSE;
spin_unlock(&ov5647mipi_drv_lock);
	//OV5647MIPI_set_mirror(sensor_config_data->SensorImageMirror);
spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPI_sensor.pclk = OV5647MIPI_CAPTURE_CLK;
	OV5647MIPI_sensor.line_length = OV5647MIPI_FULL_PERIOD_PIXEL_NUMS;
	OV5647MIPI_sensor.frame_height = OV5647MIPI_FULL_PERIOD_LINE_NUMS+dummy_line;
spin_unlock(&ov5647mipi_drv_lock);

	msleep(40);
	//OV5647MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
	//OV5647MIPI_Write_Shutter(OV5647MIPI_sensor.shutter);	  
	return ERROR_NONE;
   
}

UINT32 OV5647MIPIVIDEO(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 dummy_line=0;
	
	OV5647MIPI_Sensor_1080P();
	OV5647MIPI_sensor.video_mode = KAL_TRUE;
	OV5647MIPI_sensor.pv_mode = KAL_FALSE;

	
	
	OV5647MIPI_sensor.line_length = OV5647MIPI_VIDEO_PERIOD_PIXEL_NUMS;
	OV5647MIPI_sensor.frame_height = OV5647MIPI_VIDEO_PERIOD_LINE_NUMS+dummy_line;

	OV5647MIPI_sensor.pclk = OV5647MIPI_VIDEO_CLK;

	OV5647MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
	//OV5647MIPI_Write_Shutter(OV5647MIPI_sensor.shutter);
	msleep(40);
	//printk("[soso][OV5647MIPIPreview]shutter=%x,shutter=%d\n",OV5647MIPI_sensor.shutter,OV5647MIPI_sensor.shutter);
	return ERROR_NONE;
}   /*  OV5647MIPIPreview   */

/*************************************************************************
* FUNCTION
*	OV5647MIPICapture
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
UINT32 OV5647MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	
	kal_uint16 cap_fps;

	spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPI_sensor.video_mode = KAL_FALSE;
	OV5647MIPIAutoFlicKerMode = KAL_FALSE;
	spin_unlock(&ov5647mipi_drv_lock);


		OV5647MIPI_Sensor_5M();
		spin_lock(&ov5647mipi_drv_lock);
		OV5647MIPI_sensor.pv_mode = KAL_FALSE;
	
		OV5647MIPI_sensor.pclk = OV5647MIPI_CAPTURE_CLK;
		spin_unlock(&ov5647mipi_drv_lock);
		cap_fps = OV5647MIPI_FPS(15);

		OV5647MIPI_Set_Dummy(0, 0);
		spin_lock(&ov5647mipi_drv_lock);
		OV5647MIPI_sensor.line_length = OV5647MIPI_FULL_PERIOD_PIXEL_NUMS;
		OV5647MIPI_sensor.frame_height = OV5647MIPI_FULL_PERIOD_LINE_NUMS;
		spin_unlock(&ov5647mipi_drv_lock);
        mdelay(40);

	return ERROR_NONE;
}   /* OV5647MIPI_Capture() */

UINT32 OV5647MIPI3DPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 dummy_line;
	
	OV5647MIPI_Sensor_1080P();
		spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPI_sensor.pv_mode = KAL_TRUE;
		spin_unlock(&ov5647mipi_drv_lock);
	
	//OV5647MIPIZsdCameraPreview=KAL_FALSE;
	
	//OV5647MIPI_set_mirror(sensor_config_data->SensorImageMirror);
	switch (sensor_config_data->SensorOperationMode)
	{
	  case MSDK_SENSOR_OPERATION_MODE_VIDEO: 	  	
		spin_lock(&ov5647mipi_drv_lock);
		OV5647MIPI_sensor.video_mode = KAL_TRUE;
		spin_unlock(&ov5647mipi_drv_lock);
		dummy_line = 0;
		break;
	  default: /* ISP_PREVIEW_MODE */
		spin_lock(&ov5647mipi_drv_lock);
		OV5647MIPI_sensor.video_mode = KAL_FALSE;
		spin_unlock(&ov5647mipi_drv_lock);
		dummy_line = 0;
		break;
	}
		spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPI_sensor.line_length = OV5647MIPI_PV_PERIOD_PIXEL_NUMS;
	OV5647MIPI_sensor.frame_height = OV5647MIPI_PV_PERIOD_LINE_NUMS+dummy_line;

	OV5647MIPI_sensor.pclk = OV5647MIPI_PREVIEW_CLK;
		spin_unlock(&ov5647mipi_drv_lock);

	OV5647MIPI_Set_Dummy(0, dummy_line); /* modify dummy_pixel must gen AE table again */
	//OV5647MIPI_Write_Shutter(OV5647MIPI_sensor.shutter);
	mdelay(40);
	//printk("[soso][OV5647MIPIPreview]shutter=%x,shutter=%d\n",OV5647MIPI_sensor.shutter,OV5647MIPI_sensor.shutter);
	return ERROR_NONE;
}   /*  OV5647MIPI3DPreview   */

UINT32 OV5647MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=OV5647MIPI_IMAGE_SENSOR_FULL_WIDTH;
	pSensorResolution->SensorFullHeight=OV5647MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	pSensorResolution->SensorPreviewWidth=OV5647MIPI_IMAGE_SENSOR_PV_WIDTH;
	pSensorResolution->SensorPreviewHeight=OV5647MIPI_IMAGE_SENSOR_PV_HEIGHT;	
	pSensorResolution->SensorVideoWidth=OV5647MIPI_IMAGE_SENSOR_VIDEO_WIDTH;
	pSensorResolution->SensorVideoHeight=OV5647MIPI_IMAGE_SENSOR_VIDEO_HEIGHT;
	pSensorResolution->Sensor3DFullWidth=OV5647MIPI_IMAGE_SENSOR_3D_FULL_WIDTH;
	pSensorResolution->Sensor3DFullHeight=OV5647MIPI_IMAGE_SENSOR_3D_FULL_HEIGHT;
	pSensorResolution->Sensor3DPreviewWidth=OV5647MIPI_IMAGE_SENSOR_3D_PV_WIDTH;
	pSensorResolution->Sensor3DPreviewHeight=OV5647MIPI_IMAGE_SENSOR_3D_PV_HEIGHT;	
	pSensorResolution->Sensor3DVideoWidth=OV5647MIPI_IMAGE_SENSOR_3D_VIDEO_WIDTH;
	pSensorResolution->Sensor3DVideoHeight=OV5647MIPI_IMAGE_SENSOR_3D_VIDEO_HEIGHT;
	
	return ERROR_NONE;
}	/* OV5647MIPIGetResolution() */

UINT32 OV5647MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
#ifdef OV5647MIPI_DRIVER_TRACE
	//SENSORDB("OV5647MIPIGetInfo£¬FeatureId:%d\n",ScenarioId);
#endif


  switch(ScenarioId)
    {
     
    	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 pSensorInfo->SensorPreviewResolutionX=OV5647MIPI_IMAGE_SENSOR_FULL_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=OV5647MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			 pSensorInfo->SensorFullResolutionX=OV5647MIPI_IMAGE_SENSOR_FULL_WIDTH;
			 pSensorInfo->SensorFullResolutionY=OV5647MIPI_IMAGE_SENSOR_FULL_HEIGHT;			 
			 pSensorInfo->SensorCameraPreviewFrameRate=15;
			 break;

		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			 pSensorInfo->SensorPreviewResolutionX=OV5647MIPI_IMAGE_SENSOR_PV_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=OV5647MIPI_IMAGE_SENSOR_PV_HEIGHT;
			 pSensorInfo->SensorFullResolutionX=OV5647MIPI_IMAGE_SENSOR_FULL_WIDTH;
			 pSensorInfo->SensorFullResolutionY=OV5647MIPI_IMAGE_SENSOR_FULL_HEIGHT;				 
			 pSensorInfo->SensorCameraPreviewFrameRate=30;
			 break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 pSensorInfo->SensorPreviewResolutionX=OV5647MIPI_IMAGE_SENSOR_VIDEO_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=OV5647MIPI_IMAGE_SENSOR_VIDEO_HEIGHT;
			 pSensorInfo->SensorFullResolutionX=OV5647MIPI_IMAGE_SENSOR_FULL_WIDTH;
			 pSensorInfo->SensorFullResolutionY=OV5647MIPI_IMAGE_SENSOR_FULL_HEIGHT;				 
			 pSensorInfo->SensorCameraPreviewFrameRate=30;			
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 pSensorInfo->SensorPreviewResolutionX=OV5647MIPI_IMAGE_SENSOR_3D_VIDEO_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=OV5647MIPI_IMAGE_SENSOR_3D_VIDEO_HEIGHT;
			 pSensorInfo->SensorFullResolutionX=OV5647MIPI_IMAGE_SENSOR_3D_FULL_WIDTH;
			 pSensorInfo->SensorFullResolutionY=OV5647MIPI_IMAGE_SENSOR_3D_FULL_HEIGHT;				 
			 pSensorInfo->SensorCameraPreviewFrameRate=30;	        
        	break;
		default:

			 pSensorInfo->SensorPreviewResolutionX=OV5647MIPI_IMAGE_SENSOR_PV_WIDTH;
	         pSensorInfo->SensorPreviewResolutionY=OV5647MIPI_IMAGE_SENSOR_PV_HEIGHT;
			 pSensorInfo->SensorFullResolutionX=OV5647MIPI_IMAGE_SENSOR_FULL_WIDTH;
			 pSensorInfo->SensorFullResolutionY=OV5647MIPI_IMAGE_SENSOR_FULL_HEIGHT;				 
			 pSensorInfo->SensorCameraPreviewFrameRate=30;
			 break;
			 
    	}


	//pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE; //low active
	pSensorInfo->SensorResetDelayCount=5; 

	pSensorInfo->SensorOutputDataFormat=OV5647MIPI_COLOR_FORMAT;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

	pSensorInfo->SensorInterruptDelayLines = 4;
	

	pSensorInfo->SensroInterfaceType        = SENSOR_INTERFACE_TYPE_MIPI;

	pSensorInfo->CaptureDelayFrame = 2;//liurui modfiy 
	pSensorInfo->PreviewDelayFrame = 2; 
	pSensorInfo->VideoDelayFrame = 2; 	

	pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;
    pSensorInfo->AEShutDelayFrame = 0;		   /* The frame of setting shutter default 0 for TG int */
	pSensorInfo->AESensorGainDelayFrame = 0;	   /* The frame of setting sensor gain */
	pSensorInfo->AEISPGainDelayFrame = 2;    
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX=  OV5647MIPI_PV_X_START;
			pSensorInfo->SensorGrabStartY = OV5647MIPI_PV_Y_START; 
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;

		break;

 		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX=  OV5647MIPI_VIDEO_X_START;
			pSensorInfo->SensorGrabStartY = OV5647MIPI_VIDEO_Y_START; 
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;

		break;
		
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = OV5647MIPI_FULL_X_START; 
			pSensorInfo->SensorGrabStartY = OV5647MIPI_FULL_Y_START; 
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
		break;

        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX=  OV5647MIPI_3D_PV_X_START;
			pSensorInfo->SensorGrabStartY = OV5647MIPI_3D_PV_Y_START; 
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;

		break;

		default:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;		
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = OV5647MIPI_PV_X_START; 
			pSensorInfo->SensorGrabStartY = OV5647MIPI_PV_Y_START; 
		break;
	}
#if 0
	//OV5647MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &OV5647MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
#endif		
  return ERROR_NONE;
}	/* OV5647MIPIGetInfo() */


UINT32 OV5647MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("OV5647MIPIControl scenario:%d\n",ScenarioId);
    CurrentScenarioId =ScenarioId;
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		//case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			  OV5647MIPIPreview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			OV5647MIPIVIDEO(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		//case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
	    	OV5647MIPICapture(pImageWindow, pSensorConfigData);
	    break;
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
     		OV5647MIPIZsdPreview(pImageWindow, pSensorConfigData);
		break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
        	OV5647MIPI3DPreview(pImageWindow, pSensorConfigData);
        break;		
        default:
            return ERROR_INVALID_SCENARIO_ID;
		break;	
	}
	
	return ERROR_NONE;
}	/* OV5647MIPIControl() */



UINT32 OV5647MIPISetVideoMode(UINT16 u2FrameRate)
{
	SENSORDB("[soso][OV5647MIPISetMaxFrameRate]u2FrameRate=%d",u2FrameRate);
	spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPI_sensor.video_mode = KAL_TRUE;
	spin_unlock(&ov5647mipi_drv_lock);

	if(u2FrameRate == 30){
		spin_lock(&ov5647mipi_drv_lock);
		OV5647MIPI_sensor.NightMode = KAL_FALSE;
		spin_unlock(&ov5647mipi_drv_lock);
	}else if(u2FrameRate == 15){
	    spin_lock(&ov5647mipi_drv_lock);
		OV5647MIPI_sensor.NightMode = KAL_TRUE;
		spin_unlock(&ov5647mipi_drv_lock);
	}else{
		SENSORDB("[soso][OV5647MIPISetMaxFrameRate],Error Framerate, u2FrameRate=%d",u2FrameRate);
		return ERROR_NONE;
		// TODO: Wrong configuratioin
	}
	
	spin_lock(&ov5647mipi_drv_lock);
	OV5647MIPI_sensor.FixedFps = u2FrameRate;
	spin_unlock(&ov5647mipi_drv_lock);

	if((u2FrameRate == 30)&&(OV5647MIPIAutoFlicKerMode==KAL_TRUE))
		u2FrameRate = 296;
	else
		u2FrameRate = 10 * u2FrameRate;
	
	OV5647MIPISetMaxFrameRate(u2FrameRate);
	OV5647MIPI_Write_Shutter(OV5647MIPI_sensor.shutter);//From Meimei Video issue
    return TRUE;
}

UINT32 OV5647MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
    SENSORDB("OV5647MIPISetAutoFlickerMode:%d",bEnable);
    
	if(bEnable){
		spin_lock(&ov5647mipi_drv_lock);
		OV5647MIPIAutoFlicKerMode = KAL_TRUE;

		spin_unlock(&ov5647mipi_drv_lock);
		/*Change frame rate 29.5fps to 29.8fps to do Auto flick*/
		if((OV5647MIPI_sensor.FixedFps == 30)&&(OV5647MIPI_sensor.video_mode==KAL_TRUE))
			OV5647MIPISetMaxFrameRate(296);
	}else{//Cancel Auto flick
	    spin_lock(&ov5647mipi_drv_lock);
		OV5647MIPIAutoFlicKerMode = KAL_FALSE;
		spin_unlock(&ov5647mipi_drv_lock);
		if((OV5647MIPI_sensor.FixedFps == 30)&&(OV5647MIPI_sensor.video_mode==KAL_TRUE))
			OV5647MIPISetMaxFrameRate(300);
	}
	return TRUE;
}

UINT32 OV5647MIPISetCalData(PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData){
	UINT32 i;
	SENSORDB("OV5647MIPI Sensor write calibration data num = %d \r\n", pSetSensorCalData->DataSize);
	SENSORDB("OV5647MIPI Sensor write calibration data format = %x \r\n", pSetSensorCalData->DataFormat);
	if(pSetSensorCalData->DataSize <= MAX_SHADING_DATA_TBL)	{
		for (i = 0; i < pSetSensorCalData->DataSize; i++)		{
			if (((pSetSensorCalData->DataFormat & 0xFFFF) == 1) && ((pSetSensorCalData->DataFormat >> 16) == 1))			{
				SENSORDB("OV5647MIPI Sensor write calibration data: address = %x, value = %x \r\n",(pSetSensorCalData->ShadingData[i])>>16,(pSetSensorCalData->ShadingData[i])&0xFFFF);
				OV5647MIPI_write_cmos_sensor((pSetSensorCalData->ShadingData[i])>>16, (pSetSensorCalData->ShadingData[i])&0xFFFF);
			}
		}
	}
	return ERROR_NONE;
}

UINT32 OV5647MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	SENSORDB("OV5647MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	    spin_lock(&ov5647mipi_drv_lock);
			pclk = OV5647MIPI_PREVIEW_CLK;
			lineLength = OV5647MIPI_PV_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV5647MIPI_PV_PERIOD_LINE_NUMS;
		spin_unlock(&ov5647mipi_drv_lock);
			if(dummyLine<0) {
				dummyLine=0;
			}
			OV5647MIPI_Set_Dummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	    spin_lock(&ov5647mipi_drv_lock);
			pclk = OV5647MIPI_VIDEO_CLK;
			lineLength = OV5647MIPI_VIDEO_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - OV5647MIPI_VIDEO_PERIOD_LINE_NUMS;
		spin_unlock(&ov5647mipi_drv_lock);
			if(dummyLine<0) {
				dummyLine=0;
			}
		
			OV5647MIPI_Set_Dummy(0, dummyLine);			
			break;			

		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
	    spin_lock(&ov5647mipi_drv_lock);
			pclk = OV5647MIPI_CAPTURE_CLK;
			lineLength = OV5647MIPI_FULL_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			if(frameHeight < OV5647MIPI_FULL_PERIOD_LINE_NUMS)
				frameHeight = OV5647MIPI_FULL_PERIOD_LINE_NUMS;
			dummyLine = frameHeight - OV5647MIPI_FULL_PERIOD_LINE_NUMS;
		spin_unlock(&ov5647mipi_drv_lock);
			if(dummyLine<0) {
				dummyLine=0;
			}
		
			SENSORDB("OV5647MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate calculate = %d\n",((10 * pclk)/frameHeight/lineLength));
			OV5647MIPI_Set_Dummy(0, dummyLine);			
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


UINT32 OV5647MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) {


	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 300;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = 150;
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


UINT32 OV5647MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	//UINT32 OV5647MIPISensorRegNumber;
	//UINT32 i;
	//PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
	//MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
	//MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;
	PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData=(PSET_SENSOR_CALIBRATION_DATA_STRUCT)pFeaturePara;

	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=OV5647MIPI_IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=OV5647MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:	/* 3 */
			*pFeatureReturnPara16++=OV5647MIPI_sensor.line_length;
					*pFeatureReturnPara16=OV5647MIPI_sensor.frame_height;
					*pFeatureParaLen=4;
					break;
			
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:  /* 3 */
			*pFeatureReturnPara32 = OV5647MIPI_sensor.pclk;
			*pFeatureParaLen=4;
					break;

		case SENSOR_FEATURE_SET_ESHUTTER:	/* 4 */
			set_OV5647MIPI_shutter(*pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			OV5647MIPI_night_mode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:	/* 6 */			
			OV5647MIPI_SetGain((UINT16) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
		case SENSOR_FEATURE_SET_REGISTER:
		OV5647MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = OV5647MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
			memcpy(&OV5647MIPI_sensor.eng.cct, pFeaturePara, sizeof(OV5647MIPI_sensor.eng.cct));
			break;

		case SENSOR_FEATURE_GET_CCT_REGISTER:	/* 12 */
			if (*pFeatureParaLen >= sizeof(OV5647MIPI_sensor.eng.cct) + sizeof(kal_uint32))
			{
			  *((kal_uint32 *)pFeaturePara++) = sizeof(OV5647MIPI_sensor.eng.cct);
			  memcpy(pFeaturePara, &OV5647MIPI_sensor.eng.cct, sizeof(OV5647MIPI_sensor.eng.cct));
			}
			break;
		case SENSOR_FEATURE_SET_ENG_REGISTER:
			memcpy(&OV5647MIPI_sensor.eng.reg, pFeaturePara, sizeof(OV5647MIPI_sensor.eng.reg));
			break;
		case SENSOR_FEATURE_GET_ENG_REGISTER:	/* 14 */
			if (*pFeatureParaLen >= sizeof(OV5647MIPI_sensor.eng.reg) + sizeof(kal_uint32))
			{
			  *((kal_uint32 *)pFeaturePara++) = sizeof(OV5647MIPI_sensor.eng.reg);
			  memcpy(pFeaturePara, &OV5647MIPI_sensor.eng.reg, sizeof(OV5647MIPI_sensor.eng.reg));
			}
			break;
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
			((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->Version = NVRAM_CAMERA_SENSOR_FILE_VERSION;
			((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorId = OV5647MIPI_SENSOR_ID;
			memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorEngReg, &OV5647MIPI_sensor.eng.reg, sizeof(OV5647MIPI_sensor.eng.reg));
			memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorCCTReg, &OV5647MIPI_sensor.eng.cct, sizeof(OV5647MIPI_sensor.eng.cct));
			*pFeatureParaLen = sizeof(NVRAM_SENSOR_DATA_STRUCT);
			break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pFeaturePara, &OV5647MIPI_sensor.cfg_data, sizeof(OV5647MIPI_sensor.cfg_data));
			*pFeatureParaLen = sizeof(OV5647MIPI_sensor.cfg_data);
			break;
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		     OV5647MIPI_camera_para_to_sensor();
		break;
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
			OV5647MIPI_sensor_to_camera_para();
		break;							
		case SENSOR_FEATURE_GET_GROUP_COUNT:
			OV5647MIPI_get_sensor_group_count((kal_uint32 *)pFeaturePara);
			*pFeatureParaLen = 4;
		break;		
		case SENSOR_FEATURE_GET_GROUP_INFO:
		  OV5647MIPI_get_sensor_group_info((MSDK_SENSOR_GROUP_INFO_STRUCT *)pFeaturePara);
		  *pFeatureParaLen = sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
		  break;
		case SENSOR_FEATURE_GET_ITEM_INFO:
		  OV5647MIPI_get_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
		  *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		  break;
		case SENSOR_FEATURE_SET_ITEM_INFO:
		  OV5647MIPI_set_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
		  *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		  break;
		case SENSOR_FEATURE_GET_ENG_INFO:
     		memcpy(pFeaturePara, &OV5647MIPI_sensor.eng_info, sizeof(OV5647MIPI_sensor.eng_info));
     		*pFeatureParaLen = sizeof(OV5647MIPI_sensor.eng_info);
     		break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
	       OV5647MIPISetVideoMode(*pFeatureData16);
        break; 
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV5647MIPIGetSensorID(pFeatureReturnPara32); 
            break; 
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			 OV5647MIPISetAutoFlickerMode((BOOL)*pFeatureData16,*(pFeatureData16+1));
			break;
		case SENSOR_FEATURE_SET_CALIBRATION_DATA:			
			OV5647MIPISetCalData(pSetSensorCalData);			
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			OV5647MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			OV5647MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
		default:
			break;
	}
	return ERROR_NONE;
}	/* OV5647MIPIFeatureControl() */
SENSOR_FUNCTION_STRUCT	SensorFuncOV5647MIPI=
{
	OV5647MIPIOpen,
	OV5647MIPIGetInfo,
	OV5647MIPIGetResolution,
	OV5647MIPIFeatureControl,
	OV5647MIPIControl,
	OV5647MIPIClose
};

UINT32 OV5647MIPISensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncOV5647MIPI;

	return ERROR_NONE;
}	/* SensorInit() */
