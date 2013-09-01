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
 * 04 26 2012 guoqing.liu
 * [ALPS00271165] [FPB&ICS Done]modify sensor driver for MT6577
 * modify for mt6577 sensor driver
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
#include <asm/system.h>
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"



#include "mt9v114yuv_Sensor.h"
#include "mt9v114yuv_Camera_Sensor_para.h"
#include "mt9v114yuv_CameraCustomized.h"

#define MT9V114YUV_DEBUG
#ifdef MT9V114YUV_DEBUG
#define SENSORDB(fmt, arg...) printk( "[MT9V114YUV] "  fmt, ##arg)

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
  //0    :No-Fix FrameRate 
  //other:Fixed FrameRate
} MT9V114Status;
MT9V114Status MT9V114CurrentStatus;


static kal_bool MT9V114_AWB_ENABLE = KAL_TRUE; 
static kal_bool MT9V114_AE_ENABLE = KAL_TRUE; 

static DEFINE_SPINLOCK(mt9v114yuv_drv_lock);


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
kal_uint16 MT9V114_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 4,MT9V114_WRITE_ID);
	return 0;
}

kal_uint16 MT9V114_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,MT9V114_WRITE_ID);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}


/*******************************************************************************
* // Adapter for Winmo typedef 
********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
MSDK_SENSOR_CONFIG_STRUCT MT9V114SensorConfigData;

static struct
{
  kal_uint16 sensor_id;
  
  kal_uint32 Digital_Zoom_Factor;
} MT9V114Sensor;


typedef struct _SENSOR_INIT_INFO
{
	kal_uint16 address;
	kal_uint16 data;
}SOCSensorInitInfo;

const SOCSensorInitInfo MT9V114SensorInitialSetting[]=
{

	/*BEGIN*/
			{0x098A, 0x0000},
			{0x8082, 0x0194},
			{0x8084, 0x0163},
			{0x8086, 0x0107},
			{0x8088, 0x01C7},
			{0x808A, 0x01A1},
			{0x808C, 0x022A},
			{0x098E, 0x0000},
			{0x0982, 0x0000},
			{0x098A, 0x0000},
			{0x8098, 0x3C3C},
			{0x809A, 0x1300},
			{0x809C, 0x0147},
			{0x809E, 0xCC31},
			{0x80A0, 0x8230},
			{0x80A2, 0xED02},
			{0x80A4, 0xCC00},
			{0x80A6, 0x90ED},
			{0x80A8, 0x00C6}, ///// 
			{0x80AA, 0x04BD},
			{0x80AC, 0xDDBD},
			{0x80AE, 0x5FD7},
			{0x80B0, 0x8E86},
			{0x80B2, 0x90BD},
			{0x80B4, 0x0330},
			{0x80B6, 0xDD90},
			{0x80B8, 0xCC92},
			{0x80BA, 0x02BD},
			{0x80BC, 0x0330},
			{0x80BE, 0xDD92},
			{0x80C0, 0xCC94},
			{0x80C2, 0x04BD},
			{0x80C4, 0x0330},
			{0x80C6, 0xDD94},
			{0x80C8, 0xCC96},
			{0x80CA, 0x00BD},
			{0x80CC, 0x0330},
			{0x80CE, 0xDD96},
			{0x80D0, 0xCC07},
			{0x80D2, 0xFFDD},
			{0x80D4, 0x8ECC},
			{0x80D6, 0x3180},
			{0x80D8, 0x30ED},
			{0x80DA, 0x02CC},
			{0x80DC, 0x008E},
			{0x80DE, 0xED00},
			{0x80E0, 0xC605},
			{0x80E2, 0xBDDE},
			{0x80E4, 0x13BD},
			{0x80E6, 0x03FA},
			{0x80E8, 0x3838},
			{0x80EA, 0x3913},
			{0x80EC, 0x0001},
			{0x80EE, 0x0109},
			{0x80F0, 0xBC01},
			{0x80F2, 0x9D26},
			{0x80F4, 0x0813},
			{0x80F6, 0x0004},
			{0x80F8, 0x0108},
			{0x80FA, 0xFF02},
			{0x80FC, 0xEF7E},
			{0x80FE, 0xC278},
			{0x8330, 0x364F},
			{0x8332, 0x36CE},
			{0x8334, 0x02F3},
			{0x8336, 0x3AEC},
			{0x8338, 0x00CE},
			{0x833A, 0x0018},
			{0x833C, 0xBDE4},
			{0x833E, 0x5197},
			{0x8340, 0x8F38},
			{0x8342, 0xEC00},
			{0x8344, 0x938E},
			{0x8346, 0x2C02},
			{0x8348, 0x4F5F},
			{0x834A, 0x3900},
			{0x83E4, 0x3C13},
			{0x83E6, 0x0001},
			{0x83E8, 0x0CCC},
			{0x83EA, 0x3180},
			{0x83EC, 0x30ED},
			{0x83EE, 0x00CC},
			{0x83F0, 0x87FF},
			{0x83F2, 0xBDDD},
			{0x83F4, 0xF5BD},
			{0x83F6, 0xC2A9},
			{0x83F8, 0x3839},
			{0x83FA, 0xFE02},
			{0x83FC, 0xEF7E},
			{0x83FE, 0x00EB},
			{0x098E, 0x0000},
			{0x098A, 0x0000},
			{0x83E0, 0x0098},
			{0x83E2, 0x03E4},
			{0x098E, 0x0000},
			{0x098E, 0x1000},
			{0x300A, 0x01F9},
			{0x300C, 0x02D6},
			{0x3010, 0x0012},
			{0x098e, 0x9803},
			{0x9803, 0x0730}, /// 9803 = 0x07
			{0xA06E, 0x0098},
			{0xA070, 0x007E},
			{0xA072, 0x1113},
			//0xA073, 0x13	,
			{0xA074, 0x1416},
			//0xA075, 0x16	,
			{0xA076, 0x0003},
			{0xA078, 0x0004},
			{0xA01A, 0x0003},
			{0x8C00, 0x0201},	// ?
			{0x3E22, 0x3307},
			{0x3ECE, 0x4311},
			{0x3ED0, 0x16AF},
			{0x3640, 0x00F0},
			{0x3642, 0x2667},
			{0x3644, 0x0510},
			{0x3646, 0x3CEC},
			{0x3648, 0x830F},
			{0x364A, 0x00F0},
			{0x364C, 0x9625},
			{0x364E, 0x696F},
			{0x3650, 0x04CB},
			{0x3652, 0xB36E},
			{0x3654, 0x00B0},
			{0x3656, 0x5B26},
			{0x3658, 0x7BAF},
			{0x365A, 0x1888},
			{0x365C, 0x9D4F},
			{0x365E, 0x0210},
			{0x3660, 0x9729},
			{0x3662, 0x0630},
			{0x3664, 0x48CC},
			{0x3666, 0x88CF},
			{0x3680, 0x1F6A},
			{0x3682, 0xCDE9},
			{0x3684, 0x14EE},
			{0x3686, 0x4CA8},
			{0x3688, 0x95CE},
			{0x368A, 0x168C},
			{0x368C, 0x8ECC},
			{0x368E, 0x5DEC},
			{0x3690, 0x076D},
			{0x3692, 0xCB29},
			{0x3694, 0x62AA},
			{0x3696, 0x83AC},
			{0x3698, 0x2A8D},
			{0x369A, 0x256D},
			{0x369C, 0x754C},
			{0x369E, 0x3808},
			{0x36A0, 0x8B6B},
			{0x36A2, 0x348E},
			{0x36A4, 0x66EC},
			{0x36A6, 0xB5EE},
			{0x36C0, 0x5D0F},
			{0x36C2, 0xB3CD},
			{0x36C4, 0xE850},
			{0x36C6, 0x304E},
			{0x36C8, 0x2DF0},
			{0x36CA, 0x776F},
			{0x36CC, 0x0C6F},
			{0x36CE, 0x9030},
			{0x36D0, 0xA131},
			{0x36D2, 0xAF30},
			{0x36D4, 0x7EAF},
			{0x36D6, 0x0D2F},
			{0x36D8, 0x87F1},
			{0x36DA, 0x9071},
			{0x36DC, 0x6DAF},
			{0x36DE, 0x5A2F},
			{0x36E0, 0xC5EC},
			{0x36E2, 0xE4F0},
			{0x36E4, 0xEB6C},
			{0x36E6, 0x582F},
			{0x3700, 0x110E},
			{0x3702, 0xA10F},
			{0x3704, 0xDA70},
			{0x3706, 0x2A91},
			{0x3708, 0x3F92},
			{0x370A, 0xB9EC},
			{0x370C, 0x028E},
			{0x370E, 0x058D},
			{0x3710, 0xBBCC},
			{0x3712, 0x0A30},
			{0x3714, 0x044B},
			{0x3716, 0x294E},
			{0x3718, 0x31CE},
			{0x371A, 0xBEEF},
			{0x371C, 0x9E11},
			{0x371E, 0x0EEE},
			{0x3720, 0xEECE},
			{0x3722, 0x8271},
			{0x3724, 0x01F1},
			{0x3726, 0x4952},
			{0x3740, 0xEF4E},
			{0x3742, 0x5DAF},
			{0x3744, 0x69D1},
			{0x3746, 0xC7D2},
			{0x3748, 0xAA33},
			{0x374A, 0x9CF0},
			{0x374C, 0x96B1},
			{0x374E, 0x534F},
			{0x3750, 0x16B3},
			{0x3752, 0x6172},
			{0x3754, 0xC7B0},
			{0x3756, 0xA151},
			{0x3758, 0x3951},
			{0x375A, 0x1F73},
			{0x375C, 0x1FD2},
			{0x375E, 0xC76E},
			{0x3760, 0x534E},
			{0x3762, 0x3451},
			{0x3764, 0xCD51},
			{0x3766, 0xBDB2},
			{0x3782, 0x00CC},
			{0x3784, 0x0164},
			{0x3210, 0x00B8},
			{0x098E, 0x202F},
			{0xA02F, 0x01D5},
			{0xA031, 0xFFCB},
			{0xA033, 0xFF60},
			{0xA035, 0xFFA0},
			{0xA037, 0x01F9},
			{0xA039, 0xFF66},
			{0xA03B, 0xFF2C},
			{0xA03D, 0xFEF7},
			{0xA03F, 0x02DD},
			{0xA041, 0x0021},
			{0xA043, 0x004A},
			{0xA045, 0xFFF0},
			{0xA047, 0xFF8B},
			{0xA049, 0x0085},
			{0xA04B, 0xFFFD},
			{0xA04D, 0x0027},
			{0xA04F, 0xFFDC},
			{0xA051, 0x009D},
			{0xA053, 0x0067},
			{0xA055, 0xFEFC},
			{0xA057, 0x0010},
			{0xA059, 0xFFDD},
			{0x940A, 0x0000},
			{0x940C, 0x0000},
			{0x940E, 0x027F},
			{0x9410, 0x01DF},
			{0x098E, 0x2061},
			{0xA061, 0x002A},
			{0xA063, 0x0038},
			{0xA065, 0x0402},
			//{0xA066, 0x02}	,
			{0x9408, 0x10F0},
			{0x9416, 0x2D8c},
			//{0x9417, 0x8C}	,
			{0x9418, 0x1678},
			//{0x9419, 0x78}	,
			{0x2112, 0x0000},
			{0x2114, 0x0000},
			{0x2116, 0x0000},
			{0x2118, 0x0F80},
			{0x211A, 0x2A80},
			{0x211C, 0x0B40},
			{0x211E, 0x01AC},
			{0x2120, 0x0038},
			{0x326E, 0x0006},
			{0x33F4, 0x000B},
			{0x098E, 0xA087},
#if 0
			///gamma
			0xA087, 0x0002	,
			//0xA088, 0x02	,
			0xA089, 0x081a	,
			//0xA08A, 0x1A	,
			0xA08B, 0x4362	,
			//0xA08C, 0x62	,
			0xA08D, 0x7D94	,
			//0xA08E, 0x94	,
			0xA08F, 0xA6b5	,
			//0xA090, 0xB5	,
			0xA091, 0xC2cd	,
			//0xA092, 0xCD	,
			0xA093, 0xD6ef	,
			//0xA094, 0xEF	,
			0xA095, 0xE7ed	,
			//0xA096, 0xED	,
			0xA097, 0xF4fa	,
			//0xA098, 0xFA	,
			//0xA099, 0xFF00	,
#else
			{0xA087, 0x0007},
			//{0xA088, 0x02}	,
			{0xA089, 0x1630},
			//{0xA08A, 0x1A}	,
			{0xA08B, 0x526d},
			//{0xA08C, 0x62}	,
			{0xA08D, 0x869b},
			//{0xA08E, 0x94}	,
			{0xA08F, 0xAbb9},
			//{0xA090, 0xB5}	,
			{0xA091, 0xC5cf},
			//{0xA092, 0xCD}	,
			{0xA093, 0xD8e0},
			//{0xA094, 0xEF}	,
			{0xA095, 0xE7ee},
			//{0xA096, 0xED}	,
			{0xA097, 0xF4fa},
			//{0xA098, 0xFA}	,
			//{0xA099, 0xFF00}	,
		
#endif
			//{0xA09A, 0x00	,
			{0xA0AD, 0x0001},
			{0xA0AF, 0x0338},
			{0x098E, 0xA0B1},
			{0xA0B1, 0x102d},
			//{0xA0B2, 0x2D}	,
			{0xA0B3, 0x102d},
			//{0xA0B4, 0x2D}	,
			{0xA0B5, 0x102d},
			//{0xA0B6, 0x2D}	,
			{0xA0B7, 0x102d},	//1e
			//{0xA0B8, 0x2D}	,
			{0xA0B9, 0x0040},
			{0xA0BB, 0x00C8},
			{0xA07A, 0x040f},
			//{0xA07B, 0x0F}	,
			{0xA07C, 0x0300},
			//{0xA07D, 0x00}	,
			{0xA07E, 0x0078},
			{0xA080, 0x0528},
			//{0xA081, 0x28}	,
			{0xA081, 0x2832},
			{0xA083, 0x000F},
			{0xA085, 0x0000},
			{0xA01f, 0x8040},
			{0xA027, 0x0050},
			{0xA029, 0x0140},
			{0xA025, 0x0080},
			{0xA01C, 0x00C8},
			{0xA01E, 0x0080},
			{0xA01A, 0x0005},// FAE : preview  20~15
			//{0xA01A, 0x000B},
			//{0xA05F, 0xD428}	,
			//{0xA060, 0x28}	,
			{0xA05B, 0x0005},
			{0xA05D, 0x0023},
			{0x9801, 0x000F},
			{0x9C01, 0x3201}, /// modify 0x3202 better 0x3201
			{0x9001, 0x0564},
			{0x9007, 0x0509},
			{0x9003, 0x2d02}, //2d
			//{0x9004, 0x02}	,
			{0x9005, 0x2D09},
			{0x8C03, 0x0103},
			{0x8C04, 0x0305},
			//{0x8C05, 0x05}	,
			{0x098E, 0xA05F},
			{0xA05F, 0x501f},  ///601f	0x6428//saturation 451f
			//{0xA060, 0x28}	,
			{0xA029, 0x00f8},
			{0x0018, 0x0002},
		
			{0xFFFF, 100},
			{0xFFFF, 0xFFFF}	// end

	/*END
	[END]*/
};

void print_ae_awb_regs(void)
{
    SENSORDB("awb regs:\n");
    MT9V114_write_cmos_sensor(0x098E, 0x9401);  // MCU_ADDRESS [AWB_MODE]
    SENSORDB("0x9401=0x%x\n",MT9V114_read_cmos_sensor(0x9401));   
    SENSORDB("0x9406=0x%x\n",MT9V114_read_cmos_sensor(0x9406));   
    SENSORDB("0x9436=0x%x\n",MT9V114_read_cmos_sensor(0x9436));   

    SENSORDB("ae regs:\n");
    MT9V114_write_cmos_sensor(0x098E, 0x9001);  // MCU_ADDRESS [AWB_MODE]
    SENSORDB("0x9001=0x%x\n",MT9V114_read_cmos_sensor(0x9001));   


}

void MT9V114InitialSetting(void)
{	
	kal_uint32 iEcount;
	
	MT9V114_write_cmos_sensor(0x001A, 0x0106);
	Sleep(10);
	MT9V114_write_cmos_sensor(0x001A, 0x0124);
	MT9V114_write_cmos_sensor(0x0010, 0x0116);
	MT9V114_write_cmos_sensor(0x0012, 0x0500);
	MT9V114_write_cmos_sensor(0x001E, 0x0700);
	MT9V114_write_cmos_sensor(0x0018, 0x0006);
	Sleep(10);

	for(iEcount=0;(!((0xffff==(MT9V114SensorInitialSetting[iEcount].address))&&(0xffff==(MT9V114SensorInitialSetting[iEcount].data))));iEcount++)
	{
		MT9V114_write_cmos_sensor(MT9V114SensorInitialSetting[iEcount].address, MT9V114SensorInitialSetting[iEcount].data);
	}

}

void MT9V114ReadRegs(void)
{
	UINT32	curReg = 0;
	UINT16	regVal[400];
	
	/* The list is a register number followed by the value */
	for(curReg=0;(!((0xffff==(MT9V114SensorInitialSetting[curReg].address))&&(0xffff==(MT9V114SensorInitialSetting[curReg].data))));curReg++)
	{
		regVal[curReg] = MT9V114_read_cmos_sensor(MT9V114SensorInitialSetting[curReg].address);
	}

}

static kal_uint16 MT9V114_power_on(void)
{
	kal_uint16 id=0;
   spin_lock(&mt9v114yuv_drv_lock);
	MT9V114Sensor.sensor_id = 0;
   spin_unlock(&mt9v114yuv_drv_lock);
	id = MT9V114_read_cmos_sensor(MT9V114_ID_REG);
	spin_lock(&mt9v114yuv_drv_lock);
    MT9V114Sensor.sensor_id=id;
   	spin_unlock(&mt9v114yuv_drv_lock);
	SENSORDB("[MT9V114]MT9V114Sensor.sensor_id =%x\n",MT9V114Sensor.sensor_id);
	return MT9V114Sensor.sensor_id;
}

/*************************************************************************
* FUNCTION
*	MT9V114InitialPara
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
static void MT9V114InitialPara(void)
{
  /*Initial status setting 
  Can be better by sync with MT9V114InitialSetting*/
  spin_lock(&mt9v114yuv_drv_lock);
  MT9V114CurrentStatus.iNightMode = 0xFFFF;
  MT9V114CurrentStatus.iWB = AWB_MODE_AUTO;
  MT9V114CurrentStatus.iEffect = MEFFECT_OFF;
  MT9V114CurrentStatus.iBanding = AE_FLICKER_MODE_50HZ;
  MT9V114CurrentStatus.iEV = AE_EV_COMP_n03;
  MT9V114CurrentStatus.iMirror = IMAGE_NORMAL;
  MT9V114CurrentStatus.iFrameRate = 0;//No Fix FrameRate
  spin_unlock(&mt9v114yuv_drv_lock);
}

/*************************************************************************
* FUNCTION
*	MT9V114Open
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
UINT32 MT9V114Open(void)
{
     SENSORDB("[Enter]:MT9V114 Open func");

	 if (MT9V114_power_on() != MT9V114_SENSOR_ID) 
	 	{
	 	   SENSORDB("[MT9V114]Error:read sensor ID fail\n");
		   return ERROR_SENSOR_CONNECT_FAIL;
	 	}
    
     MT9V114InitialPara();
      
    /* Apply sensor initail setting*/
	MT9V114InitialSetting();
	 SENSORDB("[Exit]:MT9V114 Open func\n");
     
	return ERROR_NONE;
}	/* MT9V114Open() */


UINT32 MT9V114GetSensorID(UINT32 *sensorID)
{
     SENSORDB("[Enter]:MT9V114 MT9V114GetSensorID func zhijie:111");

	 *sensorID = MT9V114_power_on();

	 if (*sensorID != MT9V114_SENSOR_ID) 
	 	{
	 	   SENSORDB("[MT9V114]Error:read sensor ID fail\n");
		   *sensorID = 0xFFFFFFFF;
		   return ERROR_SENSOR_CONNECT_FAIL;
	 	}
    
      
    /* Apply sensor initail setting*/
	//MT9V114InitialSetting();
	 SENSORDB("[Exit]:MT9V114 MT9V114GetSensorID func\n");
     
	return ERROR_NONE;
}	/* MT9V114Open() */


/*************************************************************************
* FUNCTION
*	MT9V114Close
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
UINT32 MT9V114Close(void)
{

	return ERROR_NONE;
}	/* MT9V114Close() */


static void MT9V114_HVMirror(kal_uint8 image_mirror)
{
	if(MT9V114CurrentStatus.iMirror == image_mirror)
      return;
	switch (image_mirror)
	{
	case IMAGE_NORMAL:
		MT9V114_write_cmos_sensor(0x3040, 0x0041);
        MT9V114_write_cmos_sensor(0x098E, 0x8400);
        MT9V114_write_cmos_sensor(0x8400, 0x0200);
        MT9V114_write_cmos_sensor(0x3254, 0x0000);
		break;
	case IMAGE_H_MIRROR:
		break;
	case IMAGE_V_MIRROR:
		MT9V114_write_cmos_sensor(0x3040, 0x4041);
        MT9V114_write_cmos_sensor(0x098E, 0x8400);
        MT9V114_write_cmos_sensor(0x8400, 0x0200);
        MT9V114_write_cmos_sensor(0x3254, 0x0001);
		break;
	case IMAGE_HV_MIRROR:
		break;
	default:
		MT9V114_write_cmos_sensor(0x3040, 0x0041);
        MT9V114_write_cmos_sensor(0x098E, 0x8400);
        MT9V114_write_cmos_sensor(0x8400, 0x0200);
        MT9V114_write_cmos_sensor(0x3254, 0x0000);
		break;
	}
	spin_lock(&mt9v114yuv_drv_lock);
	MT9V114CurrentStatus.iMirror= image_mirror;
	spin_unlock(&mt9v114yuv_drv_lock);
}

void MT9V114_night_mode(kal_bool enable)
{
    if(MT9V114CurrentStatus.iNightMode == enable)
        return;
	  SENSORDB("[Enter]MT9V114 night mode func:enable = %d\n",enable);

	if (0 == MT9V114CurrentStatus.iFrameRate)
	//only for preview mode 
	//video mode will only set 30 or 15 fps
	{
	    if(enable)
	    {
	        MT9V114_write_cmos_sensor(0x098E, 0x2076);// MCU_ADDRESS [AE_MAX_INDEX]
          MT9V114_write_cmos_sensor(0xa076, 0x0014);// MCU_DATA_0 //FAE: night mode min:5fps
    	    MT9V114_write_cmos_sensor(0xa078, 0x0018);// MCU_ADDRESS [AE_INDEX_TH23]
	    }
	else
      {
		    MT9V114_write_cmos_sensor(0x098E, 0x2076);  
        //MT9V114_write_cmos_sensor(0xa076, 0x000A);  
        //MT9V114_write_cmos_sensor(0xa078, 0x000C); 
        MT9V114_write_cmos_sensor(0xa076, 0x0006);  //FAE:normal min: 16.7fps
        MT9V114_write_cmos_sensor(0xa078, 0x0008); //60hz
	    }
  }
	spin_lock(&mt9v114yuv_drv_lock);
	MT9V114CurrentStatus.iNightMode = enable;
	spin_unlock(&mt9v114yuv_drv_lock);
}

/*************************************************************************
* FUNCTION
*	MT9V114Preview
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
static UINT32 MT9V114Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	
	UINT8 StartX = 0, StartY = 1;  

	
	SENSORDB("[Enter]:MT9V114 preview func:\n");		

	MT9V114_HVMirror(sensor_config_data->SensorImageMirror);


	image_window->GrabStartX = StartX;
	image_window->GrabStartY = StartY;
	image_window->ExposureWindowWidth = MT9V114_IMAGE_SENSOR_VGA_WIDTH - 2 * StartX ;
	image_window->ExposureWindowHeight = MT9V114_IMAGE_SENSOR_VGA_HEIGHT - 2 * StartY;

  spin_lock(&mt9v114yuv_drv_lock);
  MT9V114CurrentStatus.iFrameRate = 0;
  spin_unlock(&mt9v114yuv_drv_lock);
	SENSORDB("[Exit]:MT9V114 preview func\n");
    return ERROR_NONE; 
}	/* MT9V114_Preview */

UINT32 MT9V114Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint8 StartX = 0, StartY = 1;

    SENSORDB("[Enter]:MT9V114 capture func:\n");		

	image_window->GrabStartX = StartX;
	image_window->GrabStartY = StartY;
	image_window->ExposureWindowWidth = MT9V114_IMAGE_SENSOR_VGA_WIDTH - 2 * StartX;
	image_window->ExposureWindowHeight = MT9V114_IMAGE_SENSOR_VGA_HEIGHT - 2 * StartY;
  spin_lock(&mt9v114yuv_drv_lock);
  MT9V114CurrentStatus.iFrameRate = 0;
  spin_unlock(&mt9v114yuv_drv_lock);

    SENSORDB("[Exit]:MT9V114 capture func:\n");	
    return ERROR_NONE; 
}


UINT32 MT9V114GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[Enter]:MT9V114 get Resolution func\n");
	
	pSensorResolution->SensorFullWidth=MT9V114_IMAGE_SENSOR_VGA_WIDTH -8;  
	pSensorResolution->SensorFullHeight=MT9V114_IMAGE_SENSOR_VGA_HEIGHT - 6;
	pSensorResolution->SensorPreviewWidth=MT9V114_IMAGE_SENSOR_VGA_WIDTH -8;
	pSensorResolution->SensorPreviewHeight=MT9V114_IMAGE_SENSOR_VGA_HEIGHT - 6;

    SENSORDB("[Exit]:MT9V114 get Resolution func\n");
	
	return ERROR_NONE;
}	/* MT9V114GetResolution() */

UINT32 MT9V114GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("[Enter]:MT9V114 getInfo func:ScenarioId = %d\n",ScenarioId);
   
	pSensorInfo->SensorPreviewResolutionX=MT9V114_IMAGE_SENSOR_VGA_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=MT9V114_IMAGE_SENSOR_VGA_HEIGHT;
	pSensorInfo->SensorFullResolutionX=MT9V114_IMAGE_SENSOR_VGA_WIDTH;
	pSensorInfo->SensorFullResolutionY=MT9V114_IMAGE_SENSOR_VGA_HEIGHT;

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

	pSensorInfo->CaptureDelayFrame = 4; 
	pSensorInfo->PreviewDelayFrame = 4; 
	pSensorInfo->VideoDelayFrame = 4; 
	pSensorInfo->SensorMasterClockSwitch = 0; 
       pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;   		
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
	        pSensorInfo->SensorGrabStartX = 4; 
	        pSensorInfo->SensorGrabStartY =2;  			
			
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 4; 
            pSensorInfo->SensorGrabStartY = 2;			
		break;
		default:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
            pSensorInfo->SensorGrabStartX = 4; 
            pSensorInfo->SensorGrabStartY = 2;  			
		break;
	}
//	MT9V114_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &MT9V114SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	SENSORDB("[Exit]:MT9V114 getInfo func\n");
	
	return ERROR_NONE;
}	/* MT9V114GetInfo() */


UINT32 MT9V114Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("[Enter]:MT9V114 Control func:ScenarioId = %d\n",ScenarioId);
    
    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
    //MT9V114Preview(pImageWindow, pSensorConfigData);
    //break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
    //MT9V114Capture(pImageWindow, pSensorConfigData);
        MT9V114Preview(pImageWindow, pSensorConfigData);
        break;
    default:
        break; 
    }
    
    SENSORDB("[Exit]:MT9V114 Control func\n");
    print_ae_awb_regs();
    return ERROR_NONE;
}	/* MT9V114Control() */


void MT9V114_set_AWB_mode(kal_bool AWB_enable)
{
    UINT16 temp_AWB_reg = 0;
    SENSORDB(" MT9V114_set_AWB_mode=%d\n", AWB_enable);

    if (AWB_enable == KAL_TRUE)
    {
        //enable Auto WB
        MT9V114_write_cmos_sensor(0x098E, 0x9401);	// MCU_ADDRESS [AWB_MODE]
        SENSORDB("1 0x9401=0x%x\n",MT9V114_read_cmos_sensor(0x9401));	
        temp_AWB_reg = MT9V114_read_cmos_sensor(0x9401);
        MT9V114_write_cmos_sensor(0x9401, temp_AWB_reg | 0x0100);	
        SENSORDB("2 0x9401=0x%x\n",MT9V114_read_cmos_sensor(0x9401));	
    }
    else
    {
        //turn off AWB
        MT9V114_write_cmos_sensor(0x098E, 0x9401);	// MCU_ADDRESS [AWB_MODE]
        SENSORDB("1 0x9401=0x%x\n",MT9V114_read_cmos_sensor(0x9401));	
        temp_AWB_reg = MT9V114_read_cmos_sensor(0x9401);
        MT9V114_write_cmos_sensor(0x9401, temp_AWB_reg & ~0x0100);	
        SENSORDB("2 0x9401=0x%x\n",MT9V114_read_cmos_sensor(0x9401));
    }
}

/*************************************************************************
* FUNCTION
*	MT9V114_set_param_wb
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
BOOL MT9V114_set_param_wb(UINT16 para)
{
    if(MT9V114CurrentStatus.iWB == para)
        return TRUE;
    //This sensor need more time to balance AWB, 
    //we suggest higher fps or drop some frame to avoid garbage color when preview initial
    SENSORDB("[Enter]MT9V114 set_param_wb func:para = %d\n",para);
    switch (para)
    {   
    case AWB_MODE_OFF:
		spin_lock(&mt9v114yuv_drv_lock);
        MT9V114_AWB_ENABLE = KAL_FALSE; 
		spin_unlock(&mt9v114yuv_drv_lock);
        MT9V114_set_AWB_mode(MT9V114_AWB_ENABLE);
        SENSORDB(" AWB_MODE_OFF\n");
        break;             
    case AWB_MODE_AUTO:
		spin_lock(&mt9v114yuv_drv_lock);
        MT9V114_AWB_ENABLE = KAL_TRUE; 
		spin_unlock(&mt9v114yuv_drv_lock);
        MT9V114_set_AWB_mode(MT9V114_AWB_ENABLE);         
        SENSORDB(" AWB_MODE_AUTO\n");

        MT9V114_write_cmos_sensor(0x098E, 0x9401);	// MCU_ADDRESS [AWB_MODE]
        MT9V114_write_cmos_sensor(0x9401, 0x0d00);	// MCU_DATA_0
        MT9V114_write_cmos_sensor(0x9406, 0x007F);	
        break;
    case AWB_MODE_CLOUDY_DAYLIGHT:
        MT9V114_write_cmos_sensor(0x098E, 0x9401); // MCU_ADDRESS [AWB_MODE]
        MT9V114_write_cmos_sensor(0x9401, 0x0C00); // MCU_DATA_0
        MT9V114_write_cmos_sensor(0x9406, 0x7F7F);
        //MT9V114_write_cmos_sensor(0x8400, 0x0500); //kevinwang@20100713
        MT9V114_write_cmos_sensor(0x9436, 0x3D47);

        break;
    case AWB_MODE_DAYLIGHT:
        MT9V114_write_cmos_sensor(0x098E, 0x9401); // MCU_ADDRESS [AWB_MODE]
        MT9V114_write_cmos_sensor(0x9401, 0x0C00); // MCU_DATA_0
        MT9V114_write_cmos_sensor(0x9406, 0x7474);
        //MT9V114_write_cmos_sensor(0x8400, 0x0500); //kevinwang@20100713
        MT9V114_write_cmos_sensor(0x9436, 0x4144);
        break;
    case AWB_MODE_INCANDESCENT:	
        MT9V114_write_cmos_sensor(0x098E, 0x9401); // MCU_ADDRESS [AWB_MODE]
        MT9V114_write_cmos_sensor(0x9401, 0x0C00); // MCU_DATA_0
        MT9V114_write_cmos_sensor(0x9406, 0x1010);
        //MT9V114_write_cmos_sensor(0x8400, 0x0500); // kevinwang@20100713
        MT9V114_write_cmos_sensor(0x9436, 0x552A);		
        break;  
    case AWB_MODE_FLUORESCENT:
        MT9V114_write_cmos_sensor(0x098E, 0x9401); // MCU_ADDRESS [AWB_MODE]
        MT9V114_write_cmos_sensor(0x9401, 0x0C00); // MCU_DATA_0
        MT9V114_write_cmos_sensor(0x9406, 0x3a3a);
        //MT9V114_write_cmos_sensor(0x8400, 0x0500); //kevinwang@20100713
        MT9V114_write_cmos_sensor(0x9436, 0x4c31);	
        break;  
    case AWB_MODE_TUNGSTEN:
        MT9V114_write_cmos_sensor(0x098E, 0x9401); // MCU_ADDRESS [AWB_MODE]
        MT9V114_write_cmos_sensor(0x9401, 0x0C00); // MCU_DATA_0
        MT9V114_write_cmos_sensor(0x9406, 0x4444);
        //MT9V114_write_cmos_sensor(0x8400, 0x0500); //kevinwang@20100713
        MT9V114_write_cmos_sensor(0x9436, 0x4F33);
        break;
    default:
        return FALSE;
    }
	spin_lock(&mt9v114yuv_drv_lock);
    MT9V114CurrentStatus.iWB = para;
	spin_unlock(&mt9v114yuv_drv_lock);

    return TRUE;	
} /* MT9V114_set_param_wb */

/*************************************************************************
* FUNCTION
*	MT9V114_set_param_effect
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
BOOL MT9V114_set_param_effect(UINT16 para)
{
   if(MT9V114CurrentStatus.iEffect == para)
      return TRUE;
   SENSORDB("[Enter]MT9V114 set_param_effect func:para = %d\n",para);
   switch (para)
	{
		case MEFFECT_OFF:
			MT9V114_write_cmos_sensor(0x098E, 0xA010);
			//  MT9V114_write_cmos_sensor(0x8400, 0x0101);
			MT9V114_write_cmos_sensor(0xA010, 0x0041);
			//	MT9V114_write_cmos_sensor(0x8400, 0x0202);
			Sleep(10);
	        break;
		case MEFFECT_NEGATIVE:
			MT9V114_write_cmos_sensor(0x098E, 0xA010);
			//   MT9V114_write_cmos_sensor(0x8400, 0x0101);
			MT9V114_write_cmos_sensor(0xA010, 0x0341);
			MT9V114_write_cmos_sensor(0xA012, 0x3F00);
			//	   MT9V114_write_cmos_sensor(0x8400, 0x0202);
			Sleep(10);
			break;
		case MEFFECT_SEPIA:
			MT9V114_write_cmos_sensor(0x098E, 0xA010);
			//	 MT9V114_write_cmos_sensor(0x8400, 0x0101);
			MT9V114_write_cmos_sensor(0xA010, 0x0241);
			MT9V114_write_cmos_sensor(0xA012, 0x1ED8);
			// MT9V114_write_cmos_sensor(0x8400, 0x0202);
			Sleep(10);	
			break;  
		case MEFFECT_SEPIAGREEN:		
			MT9V114_write_cmos_sensor(0x098E, 0xA010);
			//	 MT9V114_write_cmos_sensor(0x8400, 0x0101);
			MT9V114_write_cmos_sensor(0xA010, 0x0241);
			MT9V114_write_cmos_sensor(0xA012, 0xDFC0);
			// MT9V114_write_cmos_sensor(0x8400, 0x0202);
			Sleep(10);	
			break;
		case MEFFECT_SEPIABLUE:
			MT9V114_write_cmos_sensor(0x098E, 0xA010);
			//  MT9V114_write_cmos_sensor(0x8400, 0x0101);
			MT9V114_write_cmos_sensor(0xA010, 0x0241);
			MT9V114_write_cmos_sensor(0xA012, 0x003F);
			//	 MT9V114_write_cmos_sensor(0x8400, 0x0202);
			Sleep(10);     
			break;        
		case MEFFECT_MONO:			
			MT9V114_write_cmos_sensor(0x098E, 0xA010);
			//   MT9V114_write_cmos_sensor(0x8400, 0x0101);
			MT9V114_write_cmos_sensor(0xA010, 0x0141);
			//MT9V114_write_cmos_sensor(0x8400, 0x0202);
			Sleep(10);
			break;

		default:
			return FALSE;
	}
   spin_lock(&mt9v114yuv_drv_lock);
  MT9V114CurrentStatus.iEffect = para;
  spin_unlock(&mt9v114yuv_drv_lock);
	return TRUE;

} /* MT9V114_set_param_effect */

/*************************************************************************
* FUNCTION
*	MT9V114_set_param_banding
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
BOOL MT9V114_set_param_banding(UINT16 para)
{
	if(MT9V114CurrentStatus.iBanding == para)
      return TRUE;
	SENSORDB("[Enter]MT9V114 set_param_banding func:para = %d\n",para);
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			MT9V114_write_cmos_sensor(0x098e, 0x8c00);
			MT9V114_write_cmos_sensor(0x8c00, 0x0201);
			Sleep(10);
		break;

		case AE_FLICKER_MODE_60HZ:
	    	MT9V114_write_cmos_sensor(0x098e, 0x8c00);
			MT9V114_write_cmos_sensor(0x8c00, 0x0200);
			Sleep(10);
		break;

	    default:
	        return FALSE;
	}
	spin_lock(&mt9v114yuv_drv_lock);
  MT9V114CurrentStatus.iBanding = para;
  spin_unlock(&mt9v114yuv_drv_lock);
	return TRUE;
} /* MT9V114_set_param_banding */




/*************************************************************************
* FUNCTION
*	MT9V114_set_param_exposure
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
BOOL MT9V114_set_param_exposure(UINT16 para)
{

	//kal_uint16 base_target = 0;
  if(MT9V114CurrentStatus.iEV == para)
      return TRUE;
	SENSORDB("[Enter]MT9V114 set_param_exposure func:para = %d\n",para);
	MT9V114_write_cmos_sensor(0x098E, 0xA020);

	switch (para)
	{
		case AE_EV_COMP_13:  //+4 EV
			MT9V114_write_cmos_sensor(0xA020,0x9000);
			break;  
		case AE_EV_COMP_10:  //+3 EV
			MT9V114_write_cmos_sensor(0xA020,0x8000);
			break;    
		case AE_EV_COMP_07:  //+2 EV
			MT9V114_write_cmos_sensor(0xA020,0x7000);
			break;    
		case AE_EV_COMP_03:	 //	+1 EV	
			MT9V114_write_cmos_sensor(0xA020,0x6000);
			break;    
		case AE_EV_COMP_00:  // +0 EV
			MT9V114_write_cmos_sensor(0xA020,0x5000);
			break;    
		case AE_EV_COMP_n03:  // -1 EV
			MT9V114_write_cmos_sensor(0xA020,0x4000);
			break;    
		case AE_EV_COMP_n07:	// -2 EV		
			MT9V114_write_cmos_sensor(0xA020,0x3000);
			break;    
		case AE_EV_COMP_n10:   //-3 EV
			MT9V114_write_cmos_sensor(0xA020,0x2000);
			break;
		case AE_EV_COMP_n13:  // -4 EV
			MT9V114_write_cmos_sensor(0x098E, 0xA020);
			break;
		default:
			return FALSE;
	}
	spin_lock(&mt9v114yuv_drv_lock);
  MT9V114CurrentStatus.iEV = para;
  spin_unlock(&mt9v114yuv_drv_lock);
	return TRUE;
	
} /* MT9V114_set_param_exposure */

void MT9V114_set_AE_mode(kal_bool AE_enable)
{
    UINT16 temp_AE_reg = 0;
    SENSORDB("MT9V114_set_AE_mode = %d E \n",AE_enable);
    MT9V114_write_cmos_sensor(0x098E, 0x9001);
    SENSORDB("1  0x9001=0x%x\n",MT9V114_read_cmos_sensor(0x9001));	// MCU_DATA_0
    temp_AE_reg = MT9V114_read_cmos_sensor(0x9001);

    if (AE_enable == KAL_TRUE)
    {
        MT9V114_write_cmos_sensor(0x9001, temp_AE_reg | 0x0100);  
    }
    else
    {
    // turn off AEC/AGC
        MT9V114_write_cmos_sensor(0x9001, temp_AE_reg &~ 0x0100);
    }	

    //MT9V114_write_cmos_sensor(0x098E, 0x9001);
    SENSORDB("2  0x9001=0x%x\n",MT9V114_read_cmos_sensor(0x9001)); // MCU_DATA_0
}

UINT32 MT9V114YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
    SENSORDB("[Enter]MT9V114YUVSensorSetting func:cmd = %d\n",iCmd);

    switch (iCmd) {
    case FID_SCENE_MODE:	    //auto mode or night mode
        if (iPara == SCENE_MODE_OFF)//auto mode
        {
            MT9V114_night_mode(FALSE); 
        }
        else if (iPara == SCENE_MODE_NIGHTSCENE)//night mode
        {
            MT9V114_night_mode(TRUE); 
        }	
        break; 	    
    case FID_AWB_MODE:
        MT9V114_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        MT9V114_set_param_effect(iPara);
        break;
    case FID_AE_EV:	    	    
        MT9V114_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:	    	    	    
        MT9V114_set_param_banding(iPara);
        break;
    case FID_AE_SCENE_MODE: 
        if (iPara == AE_MODE_OFF) {
			spin_lock(&mt9v114yuv_drv_lock);
            MT9V114_AE_ENABLE = KAL_FALSE; 
			spin_unlock(&mt9v114yuv_drv_lock);
        }
        else {
			spin_lock(&mt9v114yuv_drv_lock);
            MT9V114_AE_ENABLE = KAL_TRUE; 
			spin_unlock(&mt9v114yuv_drv_lock);
        }
        MT9V114_set_AE_mode(MT9V114_AE_ENABLE);
        break; 
    case FID_ZOOM_FACTOR:
		spin_lock(&mt9v114yuv_drv_lock);
        MT9V114Sensor.Digital_Zoom_Factor= iPara; 		
		spin_unlock(&mt9v114yuv_drv_lock);
        break; 
    default:
        break;
    }
    return TRUE;
}   /* MT9V114YUVSensorSetting */

UINT32 MT9V114YUVSetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("[Enter]MT9V114 Set Video Mode:FrameRate= %d Current FrameRate = %d\n",u2FrameRate,MT9V114CurrentStatus.iFrameRate);

    if(MT9V114CurrentStatus.iFrameRate == u2FrameRate)
      return TRUE;
	if (u2FrameRate == 30)
    {
    	SENSORDB("Set FrameRate: 30fps\n");
    	MT9V114_write_cmos_sensor(0x098E, 0xA01A);
    	MT9V114_write_cmos_sensor(0xA01A, 0x0003);
    	MT9V114_write_cmos_sensor(0xA076, 0x0003);
		MT9V114_write_cmos_sensor(0xA078, 0x0004);
    	MT9V114_write_cmos_sensor(0x300A, 0x01F9);
		Sleep(12);
        MT9V114_write_cmos_sensor(0xa020, 0x0000);	
		Sleep(120);
		MT9V114_write_cmos_sensor(0xa020, 0x5000);	
    }
    else if (u2FrameRate == 15)       
    {
		SENSORDB("Set FrameRate: 15fps\n");
		MT9V114_write_cmos_sensor(0x098E, 0xA01A);
		MT9V114_write_cmos_sensor(0xA01A, 0x0003);
		MT9V114_write_cmos_sensor(0xA076, 0x0006);
		MT9V114_write_cmos_sensor(0xA078, 0x0008);
		MT9V114_write_cmos_sensor(0x300A, 0x03F2);
		Sleep(12);
		MT9V114_write_cmos_sensor(0xa020, 0x0000);	
		Sleep(120);
		MT9V114_write_cmos_sensor(0xa020, 0x5000);	
    }
    else 
    {
        printk("Wrong frame rate setting \n");
        return FALSE;
    }   
    spin_lock(&mt9v114yuv_drv_lock);
    MT9V114CurrentStatus.iFrameRate = u2FrameRate;
		spin_unlock(&mt9v114yuv_drv_lock);
	
    return TRUE;
}

void MT9V114GetAFMaxNumFocusAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB(" MT9V114GetAFMaxNumFocusAreas, *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);

}

void MT9V114GetAFMaxNumMeteringAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB(" MT9V114GetAFMaxNumMeteringAreas,*pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);

}

void MT9V114GetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = AE_ISO_100;
    pExifInfo->AWBMode = MT9V114CurrentStatus.iWB;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = AE_ISO_100;
}

UINT32 MT9V114FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
        *pFeatureReturnPara16++=MT9V114_IMAGE_SENSOR_VGA_WIDTH;
        *pFeatureReturnPara16=MT9V114_IMAGE_SENSOR_VGA_WIDTH;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=MT9V114_IMAGE_SENSOR_VGA_WIDTH;
        *pFeatureReturnPara16=MT9V114_IMAGE_SENSOR_VGA_WIDTH;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        //*pFeatureReturnPara32 = MT9V114_sensor_pclk/10;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        MT9V114_night_mode((BOOL) *pFeatureData16);
        break;
    case SENSOR_FEATURE_SET_GAIN:
        break; 
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        MT9V114_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = MT9V114_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &MT9V114SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
        MT9V114YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;	
    case SENSOR_FEATURE_SET_VIDEO_MODE:
        MT9V114YUVSetVideoMode(*pFeatureData16);
        break; 
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
        MT9V114GetSensorID(pFeatureReturnPara32); 
        break;
    case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
        MT9V114GetAFMaxNumFocusAreas(pFeatureReturnPara32);            
        *pFeatureParaLen=4;
        break;     
    case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
        MT9V114GetAFMaxNumMeteringAreas(pFeatureReturnPara32);            
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_EXIF_INFO:
        SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
        SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32);          
        MT9V114GetExifInfo(*pFeatureData32);
        break;


    default:
        break;			
    }
	return ERROR_NONE;
}	/* MT9V114FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncMT9V114=
{
	MT9V114Open,
	MT9V114GetInfo,
	MT9V114GetResolution,
	MT9V114FeatureControl,
	MT9V114Control,
	MT9V114Close
};

UINT32 MT9V114_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncMT9V114;

	return ERROR_NONE;
}	/* SensorInit() */


