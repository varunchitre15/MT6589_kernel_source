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
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>

//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"


#include "mt9v113yuv_Sensor.h"
#include "mt9v113yuv_Camera_Sensor_para.h"
#include "mt9v113yuv_CameraCustomized.h"

#define MT9V113YUV_DEBUG
#ifdef MT9V113YUV_DEBUG
#define SENSORDB(fmt, arg...) xlog_printk(ANDROID_LOG_INFO ,"[MT9V113YUV]", fmt, ##arg)
#else
#define SENSORDB(fmt, arg...)
#endif


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
kal_uint16 MT9V113_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 4,MT9V113_WRITE_ID);
    return 0;
}

kal_uint16 MT9V113_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,MT9V113_WRITE_ID);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}


/*******************************************************************************
* // Adapter for Winmo typedef 
********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)

struct MT9V113_sensor_struct MT9V113_Sensor_Driver;
MSDK_SENSOR_CONFIG_STRUCT MT9V113SensorConfigData;

void sequencer_refresh(void)
{
	/* It is recommended that Refresh and Refresh mode commands always	run 
	together, and Refresh mode should be issued before the Refresh command. */
	MT9V113_write_cmos_sensor( 0x098C, 0xA103 );    // MCU_ADDRESS
	MT9V113_write_cmos_sensor( 0x0990, 0x06 );    // MCU_DATA_0
	Sleep(25);
	MT9V113_write_cmos_sensor( 0x098C, 0xA103 );    // MCU_ADDRESS
	MT9V113_write_cmos_sensor( 0x0990, 0x05 );    // MCU_DATA_0
	Sleep(25);
}


static void MT9V113_LensShading_2(void)
{
	
	//[Lens Correction 08/25/08 10:55:30]
	MT9V113_write_cmos_sensor(0x3658, 0x00F0);   //P_RD_P0Q0
	MT9V113_write_cmos_sensor(0x365A, 0x43AC);   //P_RD_P0Q1
	MT9V113_write_cmos_sensor(0x365C, 0x26B2);   //P_RD_P0Q2
	MT9V113_write_cmos_sensor(0x365E, 0xD62C);   //P_RD_P0Q3
	MT9V113_write_cmos_sensor(0x3660, 0xE0D2);   //P_RD_P0Q4
	MT9V113_write_cmos_sensor(0x3680, 0x496D);   //P_RD_P1Q0
	MT9V113_write_cmos_sensor(0x3682, 0x9148);   //P_RD_P1Q1
	MT9V113_write_cmos_sensor(0x3684, 0x55D2);   //P_RD_P1Q2
	MT9V113_write_cmos_sensor(0x3686, 0xE3B2);   //P_RD_P1Q3
	MT9V113_write_cmos_sensor(0x3688, 0xB812);   //P_RD_P1Q4
	MT9V113_write_cmos_sensor(0x36A8, 0x1953);   //P_RD_P2Q0
	MT9V113_write_cmos_sensor(0x36AA, 0x056F);   //P_RD_P2Q1
	MT9V113_write_cmos_sensor(0x36AC, 0x0BF5);   //P_RD_P2Q2
	MT9V113_write_cmos_sensor(0x36AE, 0xF875);   //P_RD_P2Q3
	MT9V113_write_cmos_sensor(0x36B0, 0x9DD8);   //P_RD_P2Q4
	MT9V113_write_cmos_sensor(0x36D0, 0xEA90);   //P_RD_P3Q0
	MT9V113_write_cmos_sensor(0x36D2, 0x8194);   //P_RD_P3Q1
	MT9V113_write_cmos_sensor(0x36D4, 0x5513);   //P_RD_P3Q2
	MT9V113_write_cmos_sensor(0x36D6, 0x3337);   //P_RD_P3Q3
	MT9V113_write_cmos_sensor(0x36D8, 0xDFF7);   //P_RD_P3Q4
	MT9V113_write_cmos_sensor(0x36F8, 0x1A94);   //P_RD_P4Q0
	MT9V113_write_cmos_sensor(0x36FA, 0xF394);   //P_RD_P4Q1
	MT9V113_write_cmos_sensor(0x36FC, 0xA259);   //P_RD_P4Q2
	MT9V113_write_cmos_sensor(0x36FE, 0x2678);   //P_RD_P4Q3
	MT9V113_write_cmos_sensor(0x3700, 0xD019);   //P_RD_P4Q4
	MT9V113_write_cmos_sensor(0x364E, 0x00F0);   //P_GR_P0Q0
	MT9V113_write_cmos_sensor(0x3650, 0x0C8C);   //P_GR_P0Q1
	MT9V113_write_cmos_sensor(0x3652, 0x1672);   //P_GR_P0Q2
	MT9V113_write_cmos_sensor(0x3654, 0x9DEF);   //P_GR_P0Q3
	MT9V113_write_cmos_sensor(0x3656, 0xB5F2);   //P_GR_P0Q4
	MT9V113_write_cmos_sensor(0x3676, 0x0F8D);   //P_GR_P1Q0
	MT9V113_write_cmos_sensor(0x3678, 0xA12D);   //P_GR_P1Q1
	MT9V113_write_cmos_sensor(0x367A, 0x2252);   //P_GR_P1Q2
	MT9V113_write_cmos_sensor(0x367C, 0x93B3);   //P_GR_P1Q3
	MT9V113_write_cmos_sensor(0x367E, 0x31F2);   //P_GR_P1Q4
	MT9V113_write_cmos_sensor(0x369E, 0x0933);   //P_GR_P2Q0
	MT9V113_write_cmos_sensor(0x36A0, 0x922D);   //P_GR_P2Q1
	MT9V113_write_cmos_sensor(0x36A2, 0x1B35);   //P_GR_P2Q2
	MT9V113_write_cmos_sensor(0x36A4, 0x8DB6);   //P_GR_P2Q3
	MT9V113_write_cmos_sensor(0x36A6, 0xC1B8);   //P_GR_P2Q4
	MT9V113_write_cmos_sensor(0x36C6, 0xD050);   //P_GR_P3Q0
	MT9V113_write_cmos_sensor(0x36C8, 0xB3B4);   //P_GR_P3Q1
	MT9V113_write_cmos_sensor(0x36CA, 0x4514);   //P_GR_P3Q2
	MT9V113_write_cmos_sensor(0x36CC, 0x74F7);   //P_GR_P3Q3
	MT9V113_write_cmos_sensor(0x36CE, 0xAC78);   //P_GR_P3Q4
	MT9V113_write_cmos_sensor(0x36EE, 0x1755);   //P_GR_P4Q0
	MT9V113_write_cmos_sensor(0x36F0, 0xDBD4);   //P_GR_P4Q1
	MT9V113_write_cmos_sensor(0x36F2, 0xBB79);   //P_GR_P4Q2
	MT9V113_write_cmos_sensor(0x36F4, 0x2758);   //P_GR_P4Q3
	MT9V113_write_cmos_sensor(0x36F6, 0x9F78);   //P_GR_P4Q4
	MT9V113_write_cmos_sensor(0x3662, 0x0130);   //P_BL_P0Q0
	MT9V113_write_cmos_sensor(0x3664, 0x6B8D);   //P_BL_P0Q1
	MT9V113_write_cmos_sensor(0x3666, 0x0812);   //P_BL_P0Q2
	MT9V113_write_cmos_sensor(0x3668, 0x138E);   //P_BL_P0Q3
	MT9V113_write_cmos_sensor(0x366A, 0x89F3);   //P_BL_P0Q4
	MT9V113_write_cmos_sensor(0x368A, 0x422C);   //P_BL_P1Q0
	MT9V113_write_cmos_sensor(0x368C, 0x626A);   //P_BL_P1Q1
	MT9V113_write_cmos_sensor(0x368E, 0x29F2);   //P_BL_P1Q2
	MT9V113_write_cmos_sensor(0x3690, 0x99B2);   //P_BL_P1Q3
	MT9V113_write_cmos_sensor(0x3692, 0xBD93);   //P_BL_P1Q4
	MT9V113_write_cmos_sensor(0x36B2, 0x6AB2);   //P_BL_P2Q0
	MT9V113_write_cmos_sensor(0x36B4, 0x3A30);   //P_BL_P2Q1
	MT9V113_write_cmos_sensor(0x36B6, 0x3D94);   //P_BL_P2Q2
	MT9V113_write_cmos_sensor(0x36B8, 0xDB15);   //P_BL_P2Q3
	MT9V113_write_cmos_sensor(0x36BA, 0x9398);   //P_BL_P2Q4
	MT9V113_write_cmos_sensor(0x36DA, 0xC910);   //P_BL_P3Q0
	MT9V113_write_cmos_sensor(0x36DC, 0xDBB3);   //P_BL_P3Q1
	MT9V113_write_cmos_sensor(0x36DE, 0xA214);   //P_BL_P3Q2
	MT9V113_write_cmos_sensor(0x36E0, 0x14B7);   //P_BL_P3Q3
	MT9V113_write_cmos_sensor(0x36E2, 0x6477);   //P_BL_P3Q4
	MT9V113_write_cmos_sensor(0x3702, 0x33D4);   //P_BL_P4Q0
	MT9V113_write_cmos_sensor(0x3704, 0x8614);   //P_BL_P4Q1
	MT9V113_write_cmos_sensor(0x3706, 0x8C99);   //P_BL_P4Q2
	MT9V113_write_cmos_sensor(0x3708, 0xB2B2);   //P_BL_P4Q3
	MT9V113_write_cmos_sensor(0x370A, 0x36D9);   //P_BL_P4Q4
	MT9V113_write_cmos_sensor(0x366C, 0x00D0);   //P_GB_P0Q0
	MT9V113_write_cmos_sensor(0x366E, 0xFE0A);   //P_GB_P0Q1
	MT9V113_write_cmos_sensor(0x3670, 0x1C32);   //P_GB_P0Q2
	MT9V113_write_cmos_sensor(0x3672, 0xF22F);   //P_GB_P0Q3
	MT9V113_write_cmos_sensor(0x3674, 0xAF11);   //P_GB_P0Q4
	MT9V113_write_cmos_sensor(0x3694, 0x46CC);   //P_GB_P1Q0
	MT9V113_write_cmos_sensor(0x3696, 0x8AEE);   //P_GB_P1Q1
	MT9V113_write_cmos_sensor(0x3698, 0x3CB2);   //P_GB_P1Q2
	MT9V113_write_cmos_sensor(0x369A, 0x9C73);   //P_GB_P1Q3
	MT9V113_write_cmos_sensor(0x369C, 0x872C);   //P_GB_P1Q4
	MT9V113_write_cmos_sensor(0x36BC, 0x7CF2);   //P_GB_P2Q0
	MT9V113_write_cmos_sensor(0x36BE, 0xD750);   //P_GB_P2Q1
	MT9V113_write_cmos_sensor(0x36C0, 0x3E15);   //P_GB_P2Q2
	MT9V113_write_cmos_sensor(0x36C2, 0xF235);   //P_GB_P2Q3
	MT9V113_write_cmos_sensor(0x36C4, 0xF3B8);   //P_GB_P2Q4
	MT9V113_write_cmos_sensor(0x36E4, 0xCEB0);   //P_GB_P3Q0
	MT9V113_write_cmos_sensor(0x36E6, 0xAA74);   //P_GB_P3Q1
	MT9V113_write_cmos_sensor(0x36E8, 0x0F54);   //P_GB_P3Q2
	MT9V113_write_cmos_sensor(0x36EA, 0x6C57);   //P_GB_P3Q3
	MT9V113_write_cmos_sensor(0x36EC, 0xF857);   //P_GB_P3Q4
	MT9V113_write_cmos_sensor(0x370C, 0x1775);   //P_GB_P4Q0
	MT9V113_write_cmos_sensor(0x370E, 0x8454);   //P_GB_P4Q1
	MT9V113_write_cmos_sensor(0x3710, 0xCCF9);   //P_GB_P4Q2
	MT9V113_write_cmos_sensor(0x3712, 0x0F78);   //P_GB_P4Q3
	MT9V113_write_cmos_sensor(0x3714, 0x3359);   //P_GB_P4Q4
	MT9V113_write_cmos_sensor(0x3644, 0x0144);   //POLY_ORIGIN_C
	MT9V113_write_cmos_sensor(0x3642, 0x00F4);   //POLY_ORIGIN_R
	//STATE=Lens Correction Falloff, 85
	//STATE=Lens Correction Center X, 320
	//STATE=Lens Correction Center Y, 240
	//BITFIELD=0x3210, 0x0008, 1 //PGA_ENABLE
	MT9V113_write_cmos_sensor(0x3210, 0x09B8 );	//PGA_ENABLE
}

static void MT9V113_CCM2(void)
{
	//Adjust the CMM to increase the saturation. 
	MT9V113_write_cmos_sensor(0x098C, 0x2306);    	// MCU_ADDRESS [AWB_CCM_L_0]
	MT9V113_write_cmos_sensor(0x0990, 0x01B3);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2308);    	// MCU_ADDRESS [AWB_CCM_L_1]
	MT9V113_write_cmos_sensor(0x0990, 0xFF40);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x230A);    	// MCU_ADDRESS [AWB_CCM_L_2]
	MT9V113_write_cmos_sensor(0x0990, 0x001A);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x230C);    	// MCU_ADDRESS [AWB_CCM_L_3]
	MT9V113_write_cmos_sensor(0x0990, 0xFF73);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x230E);    	// MCU_ADDRESS [AWB_CCM_L_4]
	MT9V113_write_cmos_sensor(0x0990, 0x0200);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2310);    	// MCU_ADDRESS [AWB_CCM_L_5]
	MT9V113_write_cmos_sensor(0x0990, 0xFF84);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2312);    	// MCU_ADDRESS [AWB_CCM_L_6]
	MT9V113_write_cmos_sensor(0x0990, 0xFF66);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2314);    	// MCU_ADDRESS [AWB_CCM_L_7]
	MT9V113_write_cmos_sensor(0x0990, 0xFE5A);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2316);    	// MCU_ADDRESS [AWB_CCM_L_8]
	MT9V113_write_cmos_sensor(0x0990, 0x02A6);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x231C);    	// MCU_ADDRESS [AWB_CCM_RL_0]
	MT9V113_write_cmos_sensor(0x0990, 0x0002);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x231E);    	// MCU_ADDRESS [AWB_CCM_RL_1]
	MT9V113_write_cmos_sensor(0x0990, 0xFFD9);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2320);    	// MCU_ADDRESS [AWB_CCM_RL_2]
	MT9V113_write_cmos_sensor(0x0990, 0xFFDC);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2322);    	// MCU_ADDRESS [AWB_CCM_RL_3]
	MT9V113_write_cmos_sensor(0x0990, 0x0028);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2324);    	// MCU_ADDRESS [AWB_CCM_RL_4]
	MT9V113_write_cmos_sensor(0x0990, 0xFFA5);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2326);    	// MCU_ADDRESS [AWB_CCM_RL_5]
	MT9V113_write_cmos_sensor(0x0990, 0x0007);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2328);    	// MCU_ADDRESS [AWB_CCM_RL_6]
	MT9V113_write_cmos_sensor(0x0990, 0x0077);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x232A);    	// MCU_ADDRESS [AWB_CCM_RL_7]
	MT9V113_write_cmos_sensor(0x0990, 0x00BF);    	// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x232C);    	// MCU_ADDRESS [AWB_CCM_RL_8]
	MT9V113_write_cmos_sensor(0x0990, 0xFF4D);    	// MCU_DATA_0
}

void MT9V113_Initial_Setting(void)
{
	/* Power on initialization. */
	MT9V113_write_cmos_sensor( 0x301A, 0x1218 );
	Sleep(10);
	MT9V113_write_cmos_sensor( 0x301A, 0x121C );
	Sleep(10); 
	MT9V113_write_cmos_sensor( 0x0018, 0x4028 );	//Enable STANDBY mode	
	Sleep(10); 
	MT9V113_write_cmos_sensor( 0x001A, 0x0011 );	//Enable parallel port, and output enable.
	Sleep(10);
	MT9V113_write_cmos_sensor( 0x001A, 0x0010 );
	Sleep(10);
	MT9V113_write_cmos_sensor( 0x0018, 0x4028 );
	Sleep(10);
	MT9V113_write_cmos_sensor( 0x001A, 0x0210 );
	MT9V113_write_cmos_sensor( 0x001E, 0x0777 );	//Program to fastest SLEW rate. 
	MT9V113_write_cmos_sensor( 0x0016, 0x42DF );
	
	//[Timing setting, 12MHz input, 24MHz PCLK]
	MT9V113_write_cmos_sensor(0x0014, 0xB04B);    // PLL_CONTROL
	MT9V113_write_cmos_sensor(0x0014, 0xB049);    // PLL_CONTROL
	/* Pixel clock = (M * EXTCLK)/[(N+1)*(P3+1)*8], M[7:0], N[13:8] */
	MT9V113_write_cmos_sensor(0x0010, 0x0010);    // PLL_DIVIDERS, M=16,N=0,P3=0,PCLK=24MHz
	MT9V113_write_cmos_sensor(0x0012, 0x0000);    // PLL_P_DIVIDERS, P3,[11:8]
	MT9V113_write_cmos_sensor(0x0014, 0x244B);    // PLL_CONTROL
	Sleep(5);
	MT9V113_write_cmos_sensor(0x0014, 0x304B);    // PLL_CONTROL
	Sleep(15);
	MT9V113_write_cmos_sensor(0x0014, 0xB04A);    // PLL_CONTROL

	/* Set output width and height.  Preview(640*480) */
	MT9V113_write_cmos_sensor(0x098C, 0x2703);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0280);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2705);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x01E0);    // MCU_DATA_0
	/* Set output width and height.  Capture(640*480) */
	MT9V113_write_cmos_sensor(0x098C, 0x2707);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0280);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2709);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x01E0);    // MCU_DATA_0

	/* [Preview] The first row/column of visible pixels and the 
		last row/column of visible pixels. */
	MT9V113_write_cmos_sensor(0x098C, 0x270D);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0004);    // Row
	MT9V113_write_cmos_sensor(0x098C, 0x270F);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0004);    // Column
	MT9V113_write_cmos_sensor(0x098C, 0x2711);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x01EB);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2713);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x028B);    // MCU_DATA_0
	
	MT9V113_write_cmos_sensor(0x098C, 0x2715);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0001);    // MCU_DATA_0

	/* Preview image orientation */
	MT9V113_write_cmos_sensor(0x098C, 0x2717);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0025);    // MCU_DATA_0
	
	MT9V113_write_cmos_sensor(0x098C, 0x2719);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x001A);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x271B);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x006B);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x271D);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x006B);    // MCU_DATA_0

	/* Visible line and vertical blanking. Preview mode */
	MT9V113_write_cmos_sensor(0x098C, 0x271F);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0206);    // MCU_DATA_0 518
	/* Visible pixel and horizontal blanking. Preview mode */
	MT9V113_write_cmos_sensor(0x098C, 0x2721);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x034A);    // MCU_DATA_0 842

	/* [Capture] The first row/column of visible pixels and the 
			last row/column of visible pixels. */
	MT9V113_write_cmos_sensor(0x098C, 0x2723);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0004);    // Row
	MT9V113_write_cmos_sensor(0x098C, 0x2725);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0004);    // Column
	MT9V113_write_cmos_sensor(0x098C, 0x2727);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x01EB);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2729);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x028B);    // MCU_DATA_0

	MT9V113_write_cmos_sensor(0x098C, 0x272B);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0001);    // MCU_DATA_0

	/* Capture image orientation */
	MT9V113_write_cmos_sensor(0x098C, 0x272D);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0025);    // MCU_DATA_0
	
	MT9V113_write_cmos_sensor(0x098C, 0x272F);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x001A);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2731);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x006B);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2733);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x006B);    // MCU_DATA_0

	/* Visible line and vertical blanking. capture mode */
	MT9V113_write_cmos_sensor(0x098C, 0x2735);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x01FB);    // MCU_DATA_0
	/* Visible pixel and horizontal blanking. capture mode */
	MT9V113_write_cmos_sensor(0x098C, 0x2737);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x06FE);    // MCU_DATA_0

	/* Output crop for preview. */
	MT9V113_write_cmos_sensor(0x098C, 0x2739);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0000);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x273B);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x027F);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x273D);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0000);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x273F);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x01DF);    // MCU_DATA_0

	/* Output crop for capture. */
	MT9V113_write_cmos_sensor(0x098C, 0x2747);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0000);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2749);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x027F);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x274B);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0000);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x274D);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x01DF);    // MCU_DATA_0
	
	MT9V113_write_cmos_sensor(0x098C, 0x222D);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0077);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0xA408);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x001C);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0xA409);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x001F);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0xA40A);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0022);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0xA40B);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0025);    // MCU_DATA_0

	/* Set flicker minimal shutter width step. */
	MT9V113_write_cmos_sensor(0x098C, 0x2411);    // R9_step_0_f60, Preview
	MT9V113_write_cmos_sensor(0x0990, 0x0077);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2413);    // R9_step_0_f50
	MT9V113_write_cmos_sensor(0x0990, 0x008F);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2415);    // R9_step_1_f60, Capture
	MT9V113_write_cmos_sensor(0x0990, 0x0038);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0x2417);    // R9_step_1_f50
	MT9V113_write_cmos_sensor(0x0990, 0x0044);    // MCU_DATA_0
	
	MT9V113_write_cmos_sensor(0x098C, 0xA404);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0010);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0xA40D);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0002);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0xA40E);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x0003);    // MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0xA410);    // MCU_ADDRESS
	MT9V113_write_cmos_sensor(0x0990, 0x000A);    // MCU_DATA_0
	
	/* AE mode and turn of HG */
	MT9V113_write_cmos_sensor(0x098C, 0xA11D);
	MT9V113_write_cmos_sensor(0x0990, 0x0002);
	MT9V113_write_cmos_sensor(0x098C, 0xA120);
	MT9V113_write_cmos_sensor(0x0990, 0x0000);

	/* Set frame rate */
	MT9V113_write_cmos_sensor(0x098C, 0xA20C);
	MT9V113_write_cmos_sensor(0x0990, 0x0014);	// Max zone number, max integration time.

	/* Turn off Auto exposure configuration in CaptureEnter state.  */
	MT9V113_write_cmos_sensor(0x098C, 0xA129);		// [AE_BASETARGET]
	MT9V113_write_cmos_sensor(0x0990, 0x0000);	

	/* To avoid flicker, 50Hz for default. */
	MT9V113_write_cmos_sensor(0x098C, 0xA11E); 		// MCU_ADDRESS [SEQ_PREVIEW_1_FD]
	MT9V113_write_cmos_sensor(0x0990, 0x0002); 		// MCU_DATA_0
	MT9V113_write_cmos_sensor(0x098C, 0xA404); 		// MCU_ADDRESS [FD_MODE]
	MT9V113_write_cmos_sensor(0x0990, 0x00C0); 		// MCU_DATA_0
	
	sequencer_refresh();

	MT9V113_LensShading_2();

	MT9V113_CCM2();

	//To increase the saturation, 
	MT9V113_write_cmos_sensor(0x098C, 0xA354); 		// MCU_ADDRESS [FD_MODE]
	MT9V113_write_cmos_sensor(0x0990, 0x006A); 		// MCU_DATA_0
	sequencer_refresh();

	Sleep(5);
	/* Read the shutter of preview mode. */
	//pre_pv_shutter = read_cmos_sensor(0x3012);	
}

void MT9V113_Init_Para(void)
{
	SENSORDB("[Enter]:MT9V113_Init_Para");		

	MT9V113_Sensor_Driver.Preview_PClk = 24;// 12Mhz
	MT9V113_Sensor_Driver.Dummy_Pixels = 0;
	MT9V113_Sensor_Driver.Dummy_Lines= 0;
	MT9V113_Sensor_Driver.Min_Frame_Rate = 75;
	MT9V113_Sensor_Driver.Max_Frame_Rate = 300;

	MT9V113_Sensor_Driver.Preview_Pixels_In_Line = MT9V113_DEFUALT_PREVIEW_LINE_LENGTH + MT9V113_Sensor_Driver.Dummy_Pixels;
	MT9V113_Sensor_Driver.Preview_Lines_In_Frame = MT9V113_DEFUALT_PREVIEW_FRAME_LENGTH + MT9V113_Sensor_Driver.Dummy_Lines;
}

static kal_uint16 MT9V113_power_on(void)
{
	MT9V113_Sensor_Driver.sensor_id = 0;
	MT9V113_Sensor_Driver.sensor_id = MT9V113_read_cmos_sensor(0x0000);

   	
	SENSORDB("[MT9V113]MT9V113_Sensor_Driver.sensor_id =%x\n",MT9V113_Sensor_Driver.sensor_id);
	return MT9V113_Sensor_Driver.sensor_id;
}


/*************************************************************************
* FUNCTION
*	MT9V113Open
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
UINT32 MT9V113Open(void)
{
     SENSORDB("[Enter]:MT9V113 Open func zhijie:");

	 if (MT9V113_power_on() != MT9V113_SENSOR_ID) 
	 	{
	 	   SENSORDB("[MT9V113]Error:read sensor ID fail\n");
		   return ERROR_SENSOR_CONNECT_FAIL;
	 	}
    
      
    /* Apply sensor initail setting*/
     MT9V113_Initial_Setting();
     MT9V113_Init_Para();
	 SENSORDB("[Exit]:MT9V113 Open func\n");
     
	return ERROR_NONE;
}	/* MT9V113Open() */


UINT32 MT9V113GetSensorID(UINT32 *sensorID)
{
     SENSORDB("[Enter]:MT9V113 MT9V113GetSensorID func zhijie:");

	 *sensorID = MT9V113_power_on();

	 if (*sensorID != MT9V113_SENSOR_ID) 
	 	{
	 	   SENSORDB("[MT9V113]Error:read sensor ID fail\n");
		   *sensorID = 0xFFFFFFFF;
		   return ERROR_SENSOR_CONNECT_FAIL;
	 	}
    
      
    /* Apply sensor initail setting*/
     //MT9V113_Initial_Setting();
     //MT9V113_Init_Para();
	 SENSORDB("[Exit]:MT9V113 MT9V113GetSensorID func\n");
     
	return ERROR_NONE;
}	/* MT9V113Open() */


/*************************************************************************
* FUNCTION
*	MT9V113Close
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
UINT32 MT9V113Close(void)
{

	return ERROR_NONE;
}	/* MT9V113Close() */


static void MT9V113_HVMirror(kal_uint8 image_mirror)
{
	switch (image_mirror)
	{
	case IMAGE_NORMAL:
		/* Preview image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x2717);
		MT9V113_write_cmos_sensor(0x0990, 0x0024);
		/* Capture image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x272D);
		MT9V113_write_cmos_sensor(0x0990, 0x0024);
		break;
	case IMAGE_H_MIRROR:
		/* Preview image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x2717);
		MT9V113_write_cmos_sensor(0x0990, 0x0025);
		/* Capture image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x272D);
		MT9V113_write_cmos_sensor(0x0990, 0x0025);
		break;
	case IMAGE_V_MIRROR:
		/* Preview image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x2717);
		MT9V113_write_cmos_sensor(0x0990, 0x0026);
		/* Capture image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x272D);
		MT9V113_write_cmos_sensor(0x0990, 0x0026);
		break;
	case IMAGE_HV_MIRROR:
		/* Preview image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x2717);
		MT9V113_write_cmos_sensor(0x0990, 0x0027);
		/* Capture image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x272D);
		MT9V113_write_cmos_sensor(0x0990, 0x0027);
		break;
	default:
		/* Preview image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x2717);
		MT9V113_write_cmos_sensor(0x0990, 0x0024);
		/* Capture image orientation */
		MT9V113_write_cmos_sensor(0x098C, 0x272D);
		MT9V113_write_cmos_sensor(0x0990, 0x0024);
		break;
	}
	
	/* Seq command, refresh mode. */
	MT9V113_write_cmos_sensor(0x098C, 0xA103);
   	MT9V113_write_cmos_sensor(0x0990, 0x0006);
}

/*****************************************************************************
* FUNCTION
*  set_preview_dummy
* DESCRIPTION
*  		For preview config the dummy pixel and dummy line
*
* PARAMETERS
*  H_Blank: The value of dummy pixels.
*  V_Blank: The value of dummy lines.
* RETURNS
*  
*****************************************************************************/
void MT9V113_set_preview_dummy(kal_uint32 dummy_pixels, kal_uint32 dummy_lines)
{
	//kal_uint32 line_len_pclk = 0;
	//kal_uint32 frame_len_lines = 0;
	kal_uint32 min_shutter_step = 0;

//	MT9V113_Sensor_Driver.Preview_Pixels_In_Line = MT9V113_DEFUALT_PREVIEW_LINE_LENGTH + dummy_pixels;
//	MT9V113_Sensor_Driver.Preview_Lines_In_Frame = MT9V113_DEFUALT_PREVIEW_FRAME_LENGTH + dummy_lines;

	/* The number of complete lines (rows) in the output frame in preview. */
	MT9V113_write_cmos_sensor(0x098C, 0x271F);
	MT9V113_write_cmos_sensor(0x0990, MT9V113_Sensor_Driver.Preview_Lines_In_Frame);
	/* The number of pixel clock periods in one line (row) time in preview. */
	MT9V113_write_cmos_sensor(0x098C, 0x2721);
	MT9V113_write_cmos_sensor(0x0990, MT9V113_Sensor_Driver.Preview_Pixels_In_Line);

	/* Anti-Flicker vriable config. */
	min_shutter_step = ((FACTOR_60HZ / MT9V113_Sensor_Driver.Preview_Pixels_In_Line) + 1);
	//min_shutter_step = (min_shutter_step * init_pclk_division) / preview_pclk_division;
	MT9V113_write_cmos_sensor(0x098C, 0x2411);	//60Hz
	MT9V113_write_cmos_sensor(0x0990, min_shutter_step);
	
	min_shutter_step = ((FACTOR_50HZ / MT9V113_Sensor_Driver.Preview_Pixels_In_Line) + 1);
	//min_shutter_step = (min_shutter_step * init_pclk_division) / preview_pclk_division;
	MT9V113_write_cmos_sensor(0x098C, 0x2413);	//50Hz
	MT9V113_write_cmos_sensor(0x0990, min_shutter_step);	
	
	/* Refresh */
	sequencer_refresh();
}


void MT9V113FixFrameRate(kal_uint32 Min,kal_uint32 Max)
{
	kal_uint16 Min_Exposure;
	kal_uint32 Max_Exposure;
	SENSORDB("[Enter]:MT9V113FixFrameRate: zhijie");		


	/*For Min frame rate*/
	if(MT9V113_Sensor_Driver.Banding == AE_FLICKER_MODE_50HZ){
		Min_Exposure = (MACRO_50HZ * 10)/Min;
	}else{
		Min_Exposure = (MACRO_60HZ * 10)/Min;
	}
	
	SENSORDB("Min_Exposure = %d: zhijie",Min_Exposure);		
	MT9V113_write_cmos_sensor(0x098C, 0xA20C); 
	MT9V113_write_cmos_sensor(0x0990, Min_Exposure); //100/Fix frame rate  or 120/Fix frame rate

	/*Fix Max Frame Rate By Add dummy line*/
	Max_Exposure = (kal_uint32)(((MT9V113_Sensor_Driver.Preview_PClk* 1000*1000*10)/(2*Max))/MT9V113_Sensor_Driver.Preview_Pixels_In_Line); 			
	SENSORDB("Max_Exposure = %d: zhijie",Max_Exposure);		

	if(Max_Exposure>MT9V113_Sensor_Driver.Preview_Lines_In_Frame)
		MT9V113_Sensor_Driver.Preview_Lines_In_Frame = Max_Exposure;
	
	MT9V113_set_preview_dummy(MT9V113_Sensor_Driver.Preview_Pixels_In_Line,MT9V113_Sensor_Driver.Preview_Lines_In_Frame);

//kal_prompt_trace(MOD_ENG,"Min_frame_rate = %d,Min_frame_rate = %d",Min,Max);
//kal_prompt_trace(MOD_ENG,"pielx = %d,lines = %d",MT9V113_Sensor_Driver.Preview_Pixels_In_Line,MT9V113_Sensor_Driver.Preview_Lines_In_Frame);

MT9V113_write_cmos_sensor(0x098C, 0x2721);
//kal_prompt_trace(MOD_ENG,"read_pielx = %d",MT9V113_read_cmos_sensor(0x0990));

MT9V113_write_cmos_sensor(0x098C, 0x271F);
//kal_prompt_trace(MOD_ENG,"read_lines = %d",MT9V113_read_cmos_sensor(0x0990));

SENSORDB("[Exit]:MT9V113FixFrameRate:");		




}

void MT9V113_night_mode(kal_bool enable)
{

	  SENSORDB("[Enter]MT9V113 night mode func:enable = %d\n",enable);

		if(enable)
	{
        MT9V113_Sensor_Driver.Min_Frame_Rate = 50;
		MT9V113_Sensor_Driver.Max_Frame_Rate = 300;

		
		MT9V113_write_cmos_sensor(0x098C, 0xA20C); 		// MCU_ADDRESS [AE_MAX_INDEX]
		MT9V113_write_cmos_sensor(0x0990, 0x0018);	 	// Max zone number.(Integration time)

		//To adjust the digital gain.
		MT9V113_write_cmos_sensor(0x098C, 0x2212);
		MT9V113_write_cmos_sensor(0x0990, 0x00D0);

		MT9V113_write_cmos_sensor(0x098C, 0xA103); 		// MCU_ADDRESS [SEQ_CMD]
		MT9V113_write_cmos_sensor(0x0990, 0x0006); 		// MCU_DATA_0
		Sleep(15);
		MT9V113_write_cmos_sensor(0x098C, 0xA103); 		// MCU_ADDRESS [SEQ_CMD]
		MT9V113_write_cmos_sensor(0x0990, 0x0005); 		// MCU_DATA_0
		Sleep(10);                           
	}
	else
    {
		MT9V113_Sensor_Driver.Min_Frame_Rate = 75;
		MT9V113_Sensor_Driver.Max_Frame_Rate = 300;
	
		MT9V113_write_cmos_sensor(0x098C, 0xA20C);		// MCU_ADDRESS [AE_MAX_INDEX]
		MT9V113_write_cmos_sensor(0x0990, 0x0014);		// Max zone number.(Integration time)

		//To adjust the digital gain.
		MT9V113_write_cmos_sensor(0x098C, 0x2212);
		MT9V113_write_cmos_sensor(0x0990, 0x00BC);

		MT9V113_write_cmos_sensor(0x098C, 0xA103);		// MCU_ADDRESS [SEQ_CMD]
		MT9V113_write_cmos_sensor(0x0990, 0x0006);		// MCU_DATA_0
		Sleep(15);
		MT9V113_write_cmos_sensor(0x098C, 0xA103);		// MCU_ADDRESS [SEQ_CMD]
		MT9V113_write_cmos_sensor(0x0990, 0x0005);		// MCU_DATA_0
		Sleep(10);
	}
	
	MT9V113FixFrameRate(MT9V113_Sensor_Driver.Min_Frame_Rate,MT9V113_Sensor_Driver.Max_Frame_Rate);
}

/*************************************************************************
* FUNCTION
*	MT9V113Preview
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
static UINT32 MT9V113Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	
//	if(MT9V113_sensor.first_init == KAL_TRUE)
//		MT9V113_sensor.MPEG4_Video_mode = MT9V113_sensor.MPEG4_Video_mode;
//	else
//		MT9V113_sensor.MPEG4_Video_mode = !MT9V113_sensor.MPEG4_Video_mode;

	
	SENSORDB("[Enter]:MT9V113 preview func:");		

    MT9V113_Sensor_Driver.Camco_mode = MT9V113_CAM_PREVIEW;

	MT9V113_Sensor_Driver.Preview_PClk = 24;// 12Mhz
	MT9V113_Sensor_Driver.Dummy_Pixels = 0;
	MT9V113_Sensor_Driver.Dummy_Lines= 0;
	MT9V113_Sensor_Driver.Min_Frame_Rate = 75;
	MT9V113_Sensor_Driver.Max_Frame_Rate = 300;
		

	MT9V113_Sensor_Driver.StartX=1;
	MT9V113_Sensor_Driver.StartY=1;
	MT9V113_Sensor_Driver.iGrabWidth = MT9V113_IMAGE_SENSOR_VGA_WIDTH - 16;//16;
	MT9V113_Sensor_Driver.iGrabheight = MT9V113_IMAGE_SENSOR_VGA_HEIGHT - 12;//16;
   
	MT9V113_HVMirror(sensor_config_data->SensorImageMirror);


	MT9V113_Sensor_Driver.Preview_Pixels_In_Line = MT9V113_DEFUALT_PREVIEW_LINE_LENGTH + MT9V113_Sensor_Driver.Dummy_Pixels;
	MT9V113_Sensor_Driver.Preview_Lines_In_Frame = MT9V113_DEFUALT_PREVIEW_FRAME_LENGTH + MT9V113_Sensor_Driver.Dummy_Lines;
	MT9V113_set_preview_dummy(MT9V113_Sensor_Driver.Preview_Lines_In_Frame, MT9V113_Sensor_Driver.Preview_Pixels_In_Line);

	image_window->GrabStartX = MT9V113_Sensor_Driver.StartX;
	image_window->GrabStartY = MT9V113_Sensor_Driver.StartY;
	image_window->ExposureWindowWidth = MT9V113_Sensor_Driver.iGrabWidth;
	image_window->ExposureWindowHeight = MT9V113_Sensor_Driver.iGrabheight;

	SENSORDB("[Exit]:MT9V113 preview func\n");
    return ERROR_NONE; 
}	/* MT9V113_Preview */

UINT32 MT9V113Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	MT9V113_Sensor_Driver.Camco_mode = MT9V113_CAM_CAPTURE;

	MT9V113_Sensor_Driver.StartX=1;
	MT9V113_Sensor_Driver.StartY=1;
	MT9V113_Sensor_Driver.iGrabWidth=MT9V113_IMAGE_SENSOR_VGA_WIDTH - 16;
	MT9V113_Sensor_Driver.iGrabheight=MT9V113_IMAGE_SENSOR_VGA_HEIGHT - 12;

	image_window->GrabStartX = MT9V113_Sensor_Driver.StartX;
	image_window->GrabStartY = MT9V113_Sensor_Driver.StartY;
	image_window->ExposureWindowWidth = MT9V113_Sensor_Driver.iGrabWidth;
	image_window->ExposureWindowHeight = MT9V113_Sensor_Driver.iGrabheight;
	return ERROR_NONE;
}


UINT32 MT9V113GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[Enter]:MT9V113 get Resolution func\n");
	
	pSensorResolution->SensorFullWidth=MT9V113_IMAGE_SENSOR_VGA_WIDTH - 16;  
	pSensorResolution->SensorFullHeight=MT9V113_IMAGE_SENSOR_VGA_HEIGHT - 12;
	pSensorResolution->SensorPreviewWidth=MT9V113_IMAGE_SENSOR_VGA_WIDTH - 16;
	pSensorResolution->SensorPreviewHeight=MT9V113_IMAGE_SENSOR_VGA_HEIGHT - 12;
	pSensorResolution->SensorVideoWidth=MT9V113_IMAGE_SENSOR_VGA_WIDTH - 16;
	pSensorResolution->SensorVideoHeight=MT9V113_IMAGE_SENSOR_VGA_HEIGHT - 12;

    SENSORDB("[Exit]:MT9V113 get Resolution func\n");
	
	return ERROR_NONE;
}	/* MT9V113GetResolution() */

UINT32 MT9V113GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("[Enter]:MT9V113 getInfo func:ScenarioId = %d\n",ScenarioId);
   
	pSensorInfo->SensorPreviewResolutionX=MT9V113_IMAGE_SENSOR_VGA_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=MT9V113_IMAGE_SENSOR_VGA_HEIGHT;
	pSensorInfo->SensorFullResolutionX=MT9V113_IMAGE_SENSOR_VGA_WIDTH;
	pSensorInfo->SensorFullResolutionY=MT9V113_IMAGE_SENSOR_VGA_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=30;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;//low is to reset 
	pSensorInfo->SensorResetDelayCount=4;  //4ms 
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
	pSensorInfo->SensorInterruptDelayLines = 1; 
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;


	pSensorInfo->CaptureDelayFrame = 1; 
	pSensorInfo->PreviewDelayFrame = 2; 
	pSensorInfo->VideoDelayFrame = 0; 
	pSensorInfo->SensorMasterClockSwitch = 0; 
       pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;   		
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pSensorInfo->SensorClockFreq=12;
			pSensorInfo->SensorClockDividCount=	7;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 4;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
	        pSensorInfo->SensorGrabStartX = 4; 
	        pSensorInfo->SensorGrabStartY = 2;  			
			
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			pSensorInfo->SensorClockFreq=12;
			pSensorInfo->SensorClockDividCount=	7;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 4;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 4; 
            pSensorInfo->SensorGrabStartY = 2;			
		break;
		default:
			pSensorInfo->SensorClockFreq=12;
			pSensorInfo->SensorClockDividCount=7;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=4;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
            pSensorInfo->SensorGrabStartX = 4; 
            pSensorInfo->SensorGrabStartY = 2;  			
		break;
	}
//	MT9V113_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &MT9V113SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	SENSORDB("[Exit]:MT9V113 getInfo func\n");
	
	return ERROR_NONE;
}	/* MT9V113GetInfo() */


UINT32 MT9V113Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
   SENSORDB("[Enter]:MT9V113 Control func:ScenarioId = %d\n",ScenarioId);

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 MT9V113Preview(pImageWindow, pSensorConfigData);
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			 MT9V113Capture(pImageWindow, pSensorConfigData);
		default:
		     break; 
	}

   SENSORDB("[Exit]:MT9V113 Control func\n");
	
	return ERROR_NONE;
}	/* MT9V113Control() */


/*************************************************************************
* FUNCTION
*	MT9V113_set_param_wb
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
BOOL MT9V113_set_param_wb(UINT16 para)
{
	
    //This sensor need more time to balance AWB, 
    //we suggest higher fps or drop some frame to avoid garbage color when preview initial
   SENSORDB("[Enter]MT9V113 set_param_wb func:para = %d\n",para);
	switch (para)
	{            
		case AWB_MODE_AUTO:
			{
		        MT9V113_write_cmos_sensor( 0x098C, 0xA11F); // MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
				MT9V113_write_cmos_sensor( 0x0990, 0x0001); // MCU_DATA_0
				MT9V113_write_cmos_sensor( 0x098C, 0xA103); // MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor( 0x0990, 0x0006); // MCU_DATA_0			
            }                
		    break;
		case AWB_MODE_CLOUDY_DAYLIGHT:
			{
		        MT9V113_write_cmos_sensor(	0x098C, 0xA11F);	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
				MT9V113_write_cmos_sensor(	0x0990, 0x0000);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA103);	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor(	0x0990, 0x0006);	// MCU_DATA_0
				Sleep(25);
				MT9V113_write_cmos_sensor(	0x098C, 0xA353);	// MCU_ADDRESS [AWB_CCM_POSITION]
				MT9V113_write_cmos_sensor(	0x0990, 0x007F);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA34E);	// MCU_ADDRESS [AWB_GAIN_R]
				MT9V113_write_cmos_sensor(	0x0990, 0x00A6);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA350);	// MCU_ADDRESS [AWB_GAIN_B]
				MT9V113_write_cmos_sensor(	0x0990, 0x007F);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA103);	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor(	0x0990, 0x0005);	// MCU_DATA_0
	        }			   
		    break;
		case AWB_MODE_DAYLIGHT:
		    {
	            MT9V113_write_cmos_sensor( 0x098C, 0xA11F); 	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
				MT9V113_write_cmos_sensor( 0x0990, 0x0000); 	// MCU_DATA_0
				MT9V113_write_cmos_sensor( 0x098C, 0xA103); 	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor( 0x0990, 0x0006); 	// MCU_DATA_0
				Sleep(25); 																 
				MT9V113_write_cmos_sensor( 0x098C, 0xA353); 	// MCU_ADDRESS [AWB_CCM_POSITION]
				MT9V113_write_cmos_sensor( 0x0990, 0x007F); 	// MCU_DATA_0
				MT9V113_write_cmos_sensor( 0x098C, 0xA34E); 	// MCU_ADDRESS [AWB_GAIN_R]
				MT9V113_write_cmos_sensor( 0x0990, 0x00A2); 	// MCU_DATA_0
				MT9V113_write_cmos_sensor( 0x098C, 0xA350); 	// MCU_ADDRESS [AWB_GAIN_B]
				MT9V113_write_cmos_sensor( 0x0990, 0x0079); 	// MCU_DATA_0
				MT9V113_write_cmos_sensor( 0x098C, 0xA103); 	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor( 0x0990, 0x0005); 	// MCU_DATA_0
            }      
		    break;
		case AWB_MODE_INCANDESCENT:	
		    {
		        MT9V113_write_cmos_sensor( 0x098C, 0xA11F); 	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
				MT9V113_write_cmos_sensor( 0x0990, 0x0000); 	// MCU_DATA_0
				MT9V113_write_cmos_sensor( 0x098C, 0xA103); 	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor( 0x0990, 0x0006); 	// MCU_DATA_0
				Sleep(25); 				 
				MT9V113_write_cmos_sensor( 0x098C, 0xA353); 	// MCU_ADDRESS [AWB_CCM_POSITION]
				MT9V113_write_cmos_sensor( 0x0990, 0x0037); 	// MCU_DATA_0
				MT9V113_write_cmos_sensor( 0x098C, 0xA34E); 	// MCU_ADDRESS [AWB_GAIN_R]
				MT9V113_write_cmos_sensor( 0x0990, 0x00A6); 	// MCU_DATA_0
				MT9V113_write_cmos_sensor( 0x098C, 0xA350); 	// MCU_ADDRESS [AWB_GAIN_B]
				MT9V113_write_cmos_sensor( 0x0990, 0x0082); 	// MCU_DATA_0
				MT9V113_write_cmos_sensor( 0x098C, 0xA103); 	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor( 0x0990, 0x0005); 	// MCU_DATA_0
            }		
		    break;  
		case AWB_MODE_FLUORESCENT:
		    {
	            MT9V113_write_cmos_sensor(	0x098C, 0xA11F);	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
				MT9V113_write_cmos_sensor(	0x0990, 0x0000);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA103);	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor(	0x0990, 0x0006);	// MCU_DATA_0
				Sleep(25);
				MT9V113_write_cmos_sensor(	0x098C, 0xA353);	// MCU_ADDRESS [AWB_CCM_POSITION]
				MT9V113_write_cmos_sensor(	0x0990, 0x0030);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA34E);	// MCU_ADDRESS [AWB_GAIN_R]
				MT9V113_write_cmos_sensor(	0x0990, 0x0099);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA350);	// MCU_ADDRESS [AWB_GAIN_B]
				MT9V113_write_cmos_sensor(	0x0990, 0x0081);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA103);	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor(	0x0990, 0x0005);	// MCU_DATA_0
            }	
		    break;  
		case AWB_MODE_TUNGSTEN:
		   {
	            MT9V113_write_cmos_sensor(	0x098C, 0xA11F);	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
				MT9V113_write_cmos_sensor(	0x0990, 0x0000);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA103);	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor(	0x0990, 0x0006);	// MCU_DATA_0
				Sleep(25);
				MT9V113_write_cmos_sensor(	0x098C, 0xA353);	// MCU_ADDRESS [AWB_CCM_POSITION]
				MT9V113_write_cmos_sensor(	0x0990, 0x0000);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA34E);	// MCU_ADDRESS [AWB_GAIN_R]
				MT9V113_write_cmos_sensor(	0x0990, 0x0093);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA350);	// MCU_ADDRESS [AWB_GAIN_B]
				MT9V113_write_cmos_sensor(	0x0990, 0x007F);	// MCU_DATA_0
				MT9V113_write_cmos_sensor(	0x098C, 0xA103);	// MCU_ADDRESS [SEQ_CMD]
				MT9V113_write_cmos_sensor(	0x0990, 0x0005);	// MCU_DATA_0
		    break;
			}
		default:
			return FALSE;
	}

	return TRUE;
	
} /* MT9V113_set_param_wb */

/*************************************************************************
* FUNCTION
*	MT9V113_set_param_effect
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
BOOL MT9V113_set_param_effect(UINT16 para)
{

   SENSORDB("[Enter]MT9V113 set_param_effect func:para = %d\n",para);
   switch (para)
	{
		case MEFFECT_OFF:
		{
         	MT9V113_write_cmos_sensor(0x98C, 0x2759); 
			MT9V113_write_cmos_sensor(0x990, 0x6440); 
			MT9V113_write_cmos_sensor(0x98C, 0x275B); 
			MT9V113_write_cmos_sensor(0x990, 0x6440); 
			MT9V113_write_cmos_sensor(0x98C, 0xA103); 
			MT9V113_write_cmos_sensor(0x990, 0x05); 
        }
	        break;
		case MEFFECT_NEGATIVE:
			MT9V113_write_cmos_sensor(0x98C, 0x2759);
			MT9V113_write_cmos_sensor(0x990, 0x6444);
			MT9V113_write_cmos_sensor(0x98C, 0x275B);
			MT9V113_write_cmos_sensor(0x990, 0x6444);
			MT9V113_write_cmos_sensor(0x98C, 0xA103);
			MT9V113_write_cmos_sensor(0x990, 0x05); 
			break;
		case MEFFECT_SEPIA:
		{
            MT9V113_write_cmos_sensor(0x98C, 0x2759);
			MT9V113_write_cmos_sensor(0x990, 0x6442);
			MT9V113_write_cmos_sensor(0x98C, 0x275B);
			MT9V113_write_cmos_sensor(0x990, 0x6442);
			MT9V113_write_cmos_sensor(0x98C, 0xA103);
			MT9V113_write_cmos_sensor(0x990, 0x05); 
        }	
			break;  
		case MEFFECT_SEPIAGREEN:		
		{
            MT9V113_write_cmos_sensor(0x98C, 0x2759); 
			MT9V113_write_cmos_sensor(0x990, 0x6442); 
			MT9V113_write_cmos_sensor(0x98C, 0x275B); 
			MT9V113_write_cmos_sensor(0x990, 0x6442); 
			MT9V113_write_cmos_sensor(0x98C, 0x2763); 
			MT9V113_write_cmos_sensor(0x990, 0x0080); 
			MT9V113_write_cmos_sensor(0x98C, 0xA103); 
			MT9V113_write_cmos_sensor(0x990, 0x05); 
        }	
			break;
		case MEFFECT_SEPIABLUE:
		{
		  	MT9V113_write_cmos_sensor(0x98C, 0x2759);
			MT9V113_write_cmos_sensor(0x990, 0x6442);
			MT9V113_write_cmos_sensor(0x98C, 0x275B);
			MT9V113_write_cmos_sensor(0x990, 0x6442);
			MT9V113_write_cmos_sensor(0x98C, 0x2763);
			MT9V113_write_cmos_sensor(0x990, 0x7F00);
			MT9V113_write_cmos_sensor(0x98C, 0xA103); 
			MT9V113_write_cmos_sensor(0x990, 0x05); 
	    }     
			break;        
		case MEFFECT_MONO:			
		{
			MT9V113_write_cmos_sensor(0x98C, 0x2759); 
			MT9V113_write_cmos_sensor(0x990, 0x6441); 
			MT9V113_write_cmos_sensor(0x98C, 0x275B); 
			MT9V113_write_cmos_sensor(0x990, 0x6441); 
			MT9V113_write_cmos_sensor(0x98C, 0xA103); 
			MT9V113_write_cmos_sensor(0x990, 0x05); 
        }
			break;

		default:
			return KAL_FALSE;
	}

	return KAL_TRUE;

} /* MT9V113_set_param_effect */

/*************************************************************************
* FUNCTION
*	MT9V113_set_param_banding
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
BOOL MT9V113_set_param_banding(UINT16 para)
{


	SENSORDB("[Enter]MT9V113 set_param_banding func:para = %d\n",para);
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
	    {
			MT9V113_Sensor_Driver.Banding = AE_FLICKER_MODE_50HZ;
			MT9V113_write_cmos_sensor(0x098C, 0xA11E);		// MCU_ADDRESS [SEQ_PREVIEW_1_FD]
			MT9V113_write_cmos_sensor(0x0990, 0x0002);		// MCU_DATA_0
			MT9V113_write_cmos_sensor(0x098C, 0xA404);		// MCU_ADDRESS [FD_MODE]
			MT9V113_write_cmos_sensor(0x0990, 0x00C0);		// MCU_DATA_0
			
			MT9V113_write_cmos_sensor(0x098C, 0xA103);		// MCU_ADDRESS [SEQ_CMD]
			MT9V113_write_cmos_sensor(0x0990, 0x0006);		// MCU_DATA_0
	    }
		break;

		case AE_FLICKER_MODE_60HZ:
	    {
			MT9V113_Sensor_Driver.Banding = AE_FLICKER_MODE_60HZ;
			MT9V113_write_cmos_sensor(0x098C, 0xA11E);		// MCU_ADDRESS [SEQ_PREVIEW_1_FD]
			MT9V113_write_cmos_sensor(0x0990, 0x0002);		// MCU_DATA_0
			MT9V113_write_cmos_sensor(0x098C, 0xA404);		// MCU_ADDRESS [FD_MODE]
			MT9V113_write_cmos_sensor(0x0990, 0x00A0);		// MCU_DATA_0
			
			MT9V113_write_cmos_sensor(0x098C, 0xA103);		// MCU_ADDRESS [SEQ_CMD]
			MT9V113_write_cmos_sensor(0x0990, 0x0006);		// MCU_DATA_0
	    }
		break;

	    default:
	        return KAL_FALSE;
	}
	MT9V113FixFrameRate(MT9V113_Sensor_Driver.Min_Frame_Rate,MT9V113_Sensor_Driver.Max_Frame_Rate);


	return KAL_TRUE;
} /* MT9V113_set_param_banding */




/*************************************************************************
* FUNCTION
*	MT9V113_set_param_exposure
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
BOOL MT9V113_set_param_exposure(UINT16 para)
{

	kal_uint16 base_target = 0;

	SENSORDB("[Enter]MT9V113 set_param_exposure func:para = %d\n",para);

	switch (para)
	{
		case AE_EV_COMP_13:  //+4 EV
			base_target = 0x54;
			break;  
		case AE_EV_COMP_10:  //+3 EV
			base_target = 0x48;
			break;    
		case AE_EV_COMP_07:  //+2 EV
			base_target = 0x40;
			break;    
		case AE_EV_COMP_03:	 //	+1 EV	
			base_target = 0x38;
			break;    
		case AE_EV_COMP_00:  // +0 EV
			base_target = 0x30;
			break;    
		case AE_EV_COMP_n03:  // -1 EV
			base_target = 0x2C;
			break;    
		case AE_EV_COMP_n07:	// -2 EV		
			base_target = 0x28;
			break;    
		case AE_EV_COMP_n10:   //-3 EV
			base_target = 0x24;
			break;
		case AE_EV_COMP_n13:  // -4 EV
			base_target = 0x20;
			break;
		default:
			return FALSE;
	}
	MT9V113_write_cmos_sensor(0x098C, 0xA24F);		// [AE_BASETARGET]
	MT9V113_write_cmos_sensor(0x0990, base_target);
	MT9V113_write_cmos_sensor(0x098C, 0xA103);		// [SEQ_CMD]
	MT9V113_write_cmos_sensor(0x0990, 0x0005);

	return TRUE;
	
} /* MT9V113_set_param_exposure */


UINT32 MT9V113YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
    SENSORDB("[Enter]MT9V113YUVSensorSetting func:cmd = %d\n",iCmd);
	
	switch (iCmd) {
	case FID_SCENE_MODE:	    //auto mode or night mode

		    if (iPara == SCENE_MODE_OFF)//auto mode
		    {
		        MT9V113_night_mode(FALSE); 
		    }
		    else if (iPara == SCENE_MODE_NIGHTSCENE)//night mode
		    {
	            MT9V113_night_mode(TRUE); 
		    }	
			
	     break; 	    
	case FID_AWB_MODE:
           MT9V113_set_param_wb(iPara);
	     break;
	case FID_COLOR_EFFECT:
           MT9V113_set_param_effect(iPara);
	     break;
	case FID_AE_EV:	    	    
           MT9V113_set_param_exposure(iPara);
	     break;
	case FID_AE_FLICKER:	    	    	    
           MT9V113_set_param_banding(iPara);
	     break;
	case FID_ZOOM_FACTOR:
	     MT9V113_Sensor_Driver.Digital_Zoom_Factor= iPara; 		
	     break; 
	default:
	     break;
	}
	return TRUE;
}   /* MT9V113YUVSensorSetting */

UINT32 MT9V113YUVSetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("[Enter]MT9V113 Set Video Mode:FrameRate= %d\n",u2FrameRate);

	if (u2FrameRate == 30)
    {
    	
		MT9V113_Sensor_Driver.Min_Frame_Rate = 270;
		MT9V113_Sensor_Driver.Max_Frame_Rate = 270;
    }
    else if (u2FrameRate == 15)       
    {
                
		MT9V113_Sensor_Driver.Min_Frame_Rate = 150;
		MT9V113_Sensor_Driver.Max_Frame_Rate = 150;
    }
    else 
    {
        SENSORDB("Wrong frame rate setting \n");
    }   

	
	MT9V113FixFrameRate(MT9V113_Sensor_Driver.Min_Frame_Rate,MT9V113_Sensor_Driver.Max_Frame_Rate);
    return TRUE;
}

UINT32 MT9V113FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    //UINT16 u2Temp = 0; 
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=MT9V113_IMAGE_SENSOR_VGA_WIDTH;
			*pFeatureReturnPara16=MT9V113_IMAGE_SENSOR_VGA_WIDTH;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_GET_PERIOD:
			*pFeatureReturnPara16++=MT9V113_IMAGE_SENSOR_VGA_WIDTH+MT9V113_Sensor_Driver.Dummy_Pixels;
			*pFeatureReturnPara16=MT9V113_IMAGE_SENSOR_VGA_WIDTH+MT9V113_Sensor_Driver.Dummy_Lines;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = MT9V113_Sensor_Driver.Preview_PClk* 1000*1000;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_SET_ESHUTTER:
	
		     break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			 MT9V113_night_mode((BOOL) *pFeatureData16);
		     break;
		case SENSOR_FEATURE_SET_GAIN:
			 break; 
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		     break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		     break;
		case SENSOR_FEATURE_SET_REGISTER:
			 MT9V113_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		     break;
		case SENSOR_FEATURE_GET_REGISTER:
			 pSensorRegData->RegData = MT9V113_read_cmos_sensor(pSensorRegData->RegAddr);
		     break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			 memcpy(pSensorConfigData, &MT9V113SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
			 MT9V113YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		     break;	
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		     MT9V113YUVSetVideoMode(*pFeatureData16);
		     break; 
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
             MT9V113GetSensorID(pFeatureReturnPara32); 
            break;     
		default:
			 break;			
	}
	return ERROR_NONE;
}	/* MT9V113FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncMT9V113=
{
	MT9V113Open,
	MT9V113GetInfo,
	MT9V113GetResolution,
	MT9V113FeatureControl,
	MT9V113Control,
	MT9V113Close
};

UINT32 MT9V113_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncMT9V113;

	return ERROR_NONE;
}	/* SensorInit() */


