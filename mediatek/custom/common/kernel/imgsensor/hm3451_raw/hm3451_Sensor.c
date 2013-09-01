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
 * add new sensor driver for mt6577.
 *
 * 10 12 2011 jun.pei
 * [ALPS00065936] hm3451 check in patch
 * .
 *
 * 08 09 2011 jun.pei
 * [ALPS00065936] hm3451 check in patch
 * .
 *
 * 10 12 2010 koli.lin
 * [ALPS00127101] [Camera] AE will flash
 * [Camera]Create Vsync interrupt to handle the exposure time, sensor gain and raw gain control.
 *
 * 09 10 2010 jackie.su
 * [ALPS00002279] [Need Patch] [Volunteer Patch] ALPS.Wxx.xx Volunteer patch for
 * .10y dual sensor
 *
 * 09 02 2010 jackie.su
 * [ALPS00002279] [Need Patch] [Volunteer Patch] ALPS.Wxx.xx Volunteer patch for
 * .roll back dual sensor
 *
 * 07 27 2010 sean.cheng
 * [ALPS00003112] [Need Patch] [Volunteer Patch] ISP/Sensor driver modification for Customer support
 * .1. add master clock switcher 
 *  2. add master enable/disable 
 *  3. add dummy line/pixel for sensor 
 *  4. add sensor driving current setting
 *
 * 07 02 2010 sean.cheng
 * [ALPS00121215][Camera] Change color when switch low and high 
 * .Change Video Delay = 5, preview delay = 2
 *
 * 07 01 2010 sean.cheng
 * [ALPS00121215][Camera] Change color when switch low and high 
 * .Add video delay frame.
 *
 * 06 15 2010 koli.lin
 * [ALPS00002569][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for 
 * Fix the video recording frame rate don't fix when the setting is 15fps.
 *
 * 06 13 2010 sean.cheng
 * [ALPS00002514][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for E1k Camera 
 * .
 * 1. Add set zoom factor and capdelay frame for YUV sensor 
 * 2. Modify e1k sensor setting
 *
 * 05 07 2010 sean.cheng
 * [ALPS00005476][Performance][Camera] Camera startup time is slow 
 * .
 * Increase the delay for sensor reset, due to short delay will cause unstable
 *
 * 04 30 2010 sean.cheng
 * [ALPS00001357][Meta]CameraTool 
 * .
 * Change DS269 sensor type to YUV sensor.
 *
 * Feb 24 2010 mtk70508
 * [DUMA00154792] Sensor driver
 *
 *
 * Dec 23 2009 mtk01592
 * [DUMA00021387] [CCT Issue] Sensor Curve value沒有存入NVRAM裡
 *
 *
 * Dec 21 2009 mtk70508
 * [DUMA00147177] Winmo sensor  and lens driver  modification
 *
 *
 * Nov 24 2009 mtk02204
 * [DUMA00015869] [Camera Driver] Modifiy camera related drivers for dual/backup sensor/lens drivers.
 *
 *
 * Oct 29 2009 mtk02204
 * [DUMA00015869] [Camera Driver] Modifiy camera related drivers for dual/backup sensor/lens drivers.
 *
 *
 * Oct 27 2009 mtk02204
 * [DUMA00015869] [Camera Driver] Modifiy camera related drivers for dual/backup sensor/lens drivers.
 *
 *
 * Aug 13 2009 mtk01051
 * [DUMA00009217] [Camera Driver] CCAP First Check In
 *
 *
 * Aug 5 2009 mtk01051
 * [DUMA00009217] [Camera Driver] CCAP First Check In
 *
 *
 * Jul 17 2009 mtk01051
 * [DUMA00009217] [Camera Driver] CCAP First Check In
 *
 *
 * Jul 7 2009 mtk01051
 * [DUMA00008051] [Camera Driver] Add drivers for camera high ISO binning mode.
 * Add ISO query info for Sensor
 *
 * May 18 2009 mtk01051
 * [DUMA00005921] [Camera] LED Flashlight first check in
 *
 *
 * May 16 2009 mtk01051
 * [DUMA00005921] [Camera] LED Flashlight first check in
 *
 *
 * May 16 2009 mtk01051
 * [DUMA00005921] [Camera] LED Flashlight first check in
 *
 *
 * Apr 7 2009 mtk02204
 * [DUMA00004012] [Camera] Restructure and rename camera related custom folders and folder name of came
 *
 *
 * Mar 27 2009 mtk02204
 * [DUMA00002977] [CCT] First check in of MT6516 CCT drivers
 *
 *
 * Mar 25 2009 mtk02204
 * [DUMA00111570] [Camera] The system crash after some operations
 *
 *
 * Mar 20 2009 mtk02204
 * [DUMA00002977] [CCT] First check in of MT6516 CCT drivers
 *
 *
 * Mar 2 2009 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 *
 *
 * Feb 24 2009 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 *
 *
 * Dec 27 2008 MTK01813
 * DUMA_MBJ CheckIn Files
 * created by clearfsimport
 *
 * Dec 10 2008 mtk02204
 * [DUMA00001084] First Check in of MT6516 multimedia drivers
 *
 *
 * Oct 27 2008 mtk01051
 * [DUMA00000851] Camera related drivers check in
 * Modify Copyright Header
 *
 * Oct 24 2008 mtk02204
 * [DUMA00000851] Camera related drivers check in
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


#include "hm3451_Sensor.h"
#include "hm3451_Camera_Sensor_para.h"
#include "hm3451_CameraCustomized.h"


//#define HM3451_DEBUG



#ifdef HM3451_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

#define HM3451_NORMAL_VIEW  
//#define HM3451_MIRROR_VIEW
//#define HM3451_FLIP_VIEW
//#define HM3451_MIRROR_FLIP_VIEW

#if defined(HM3451_NORMAL_VIEW)
	#define HM3451_PV_START_X  2
    #define HM3451_PV_START_Y  2
    #define HM3451_CP_START_X  2
    #define HM3451_CP_START_Y  2
#elif defined(HM3451_MIRROR_VIEW)
    #define HM3451_PV_START_X  3
    #define HM3451_PV_START_Y  2
    #define HM3451_CP_START_X  3
    #define HM3451_CP_START_Y  2
#elif defined(HM3451_FLIP_VIEW)
    #define HM3451_PV_START_X  2
    #define HM3451_PV_START_Y  3
    #define HM3451_CP_START_X  2
    #define HM3451_CP_START_Y  3
#elif defined(HM3451_MIRROR_FLIP_VIEW)
    #define HM3451_PV_START_X  3
    #define HM3451_PV_START_Y  3
    #define HM3451_CP_START_X  3
    #define HM3451_CP_START_Y  3
#else				//modify for ywj for macro control in make file 110421
	#define HM3451_PV_START_X  2
    #define HM3451_PV_START_Y  2
    #define HM3451_CP_START_X  2
    #define HM3451_CP_START_Y  2
#endif


static struct HM3451_sensor_STRUCT HM3451_sensor={HM3451_WRITE_ID_1,HM3451_READ_ID_1,KAL_FALSE,KAL_TRUE,KAL_FALSE,
KAL_FALSE,0,48750000,48750000,0,0,64,64,2032,800,2992,1578,132,2,0,0,30};


SENSOR_REG_STRUCT HM3451SensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT HM3451SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
//	camera_para.SENSOR.cct	SensorCCT	=> SensorCCT
//	camera_para.SENSOR.reg	SensorReg
MSDK_SENSOR_CONFIG_STRUCT HM3451SensorConfigData;
kal_uint32 HM3451_isp_master_clock;
kal_uint16 HM3451_max_exposure_lines = HM3451_PV_EXPOSURE_LIMITATION;
kal_uint16 HM3451_Fix_fps_dummy_lines = 0;
/* Parameter For Engineer mode function */
kal_uint32 HM3451_FAC_SENSOR_REG;


static DEFINE_SPINLOCK(hm3451raw_drv_lock);



extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
void HM3451_write_cmos_sensor(kal_uint16 addr, kal_uint8 para)
{
    char puSendCmd[3] = {(char)((addr & 0xFF00)>>8), (char)(addr & 0xFF) , (char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 3,HM3451_sensor.i2c_write_id);

}
kal_uint8 HM3451_read_cmos_sensor(kal_uint16 addr)
{
	kal_uint8 get_byte=0;
    char puSendCmd[2] = {(char)((addr & 0xFF00)>>8), (char)(addr & 0xFF)};
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,HM3451_sensor.i2c_write_id);
	
    return get_byte;
}
/*************************************************************************
* FUNCTION
*	HM3451_write_reg
*
* DESCRIPTION
*	This function set the register of HM3451.
*
* PARAMETERS
*	addr : the register index of HM3451
*  para : setting parameter of the specified register of HM3451
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HM3451_write_reg(kal_uint32 addr, kal_uint32 para)
{
	HM3451_write_cmos_sensor(addr,para);
	
	HM3451_write_cmos_sensor(0x0000,0xFF);//update Cmd
}	/* HM3451_write_reg() */

static void HM3451_set_dummy(kal_uint16 dummy_pixels, kal_uint16 dummy_lines)
{
/*********************************************************
Dummy pixels: BLNKC Reg0x0013 bit[7:0] * 12
Dummy lines:  BLNKRH:BLNKRL Reg0x0010:Reg0x0011
Dummy lines have Default value 2 when Reg0x000F bit [4]= 0 
**********************************************************/

   SENSORDB("[HM3451][Enter]set_dummy func: dummy_pixels=%d,dummy_lines=%d",dummy_pixels,dummy_lines);

   if(dummy_lines < 6)
		dummy_lines = 6;

		dummy_pixels = dummy_pixels/12;
	
	//Dummy lines
	HM3451_write_cmos_sensor(0x0010, (dummy_lines & 0xFF00)>>8);
	HM3451_write_cmos_sensor(0x0011, dummy_lines & 0x00FF);

	//Dummy pixels
	HM3451_write_cmos_sensor(0x0013, dummy_pixels);

	HM3451_write_cmos_sensor(0x0000,0xFF);


	SENSORDB("HM3451_set_dummy Pixels = %d,",dummy_pixels*12);
    SENSORDB("HM3451_set_dummy Lines = %d\n",dummy_lines);

}   
static void HM3451_write_shutter(kal_uint16 shutter)
{
/**************************************************************
	when Shutter > Frame High , It will auto decream the frame rate
	Shutter Timming:
		____		____		____		____		____		____
		|	|	|	|	|	|	|	|	|	|	|	|	|	
	____|	|___|	|___|	|___|	|___|	|___|	|___|
		 {0}		   {1}	  {2}	  {3}	   {0}	   {1}
shutter/sensor Gain		 ISP Gain
**************************************************************/
  kal_uint16 dummy_pixels=0,dummy_lines=0;
  kal_uint16 temp = 0;
  unsigned long flags;

	SENSORDB("[HM3451][Enter]Write_shutter_func:shutter=%d,\n",shutter);

	/*it must allocate the dummy lines dynamically ,then the frame rate can be fixed in the desired value*/
	if(HM3451_sensor.fix_video_fps == KAL_TRUE)
   {
    	if (shutter > (HM3451_PV_PERIOD_LINE_NUMS+HM3451_Fix_fps_dummy_lines))
   	{
			shutter = (HM3451_PV_PERIOD_LINE_NUMS+HM3451_Fix_fps_dummy_lines);
    }
   if(shutter > HM3451_PV_PERIOD_LINE_NUMS)
	{
      SENSORDB("[HM3451][Warning]set shutter exceed active line(798):%d!!!,so reduce dummy_line\n",HM3451_PV_PERIOD_LINE_NUMS);
	  temp = shutter - HM3451_PV_PERIOD_LINE_NUMS;
	  spin_lock_irqsave(&hm3451raw_drv_lock,flags);
	  HM3451_sensor.pv_dummy_lines = HM3451_Fix_fps_dummy_lines - temp;  
	  spin_unlock_irqrestore(&hm3451raw_drv_lock,flags);
    }
	else
	{
	  SENSORDB("[HM3451]set shutter small than active line(798):%d!!!,so fixed dummy_line \n",HM3451_PV_PERIOD_LINE_NUMS);
	  temp = shutter - HM3451_PV_PERIOD_LINE_NUMS;
	  spin_lock_irqsave(&hm3451raw_drv_lock,flags);
	  HM3451_sensor.pv_dummy_lines = HM3451_Fix_fps_dummy_lines; 
	  spin_unlock_irqrestore(&hm3451raw_drv_lock,flags);

	}
   }
	
	HM3451_write_cmos_sensor(0x0015, (shutter >> 8) & 0xff );
	HM3451_write_cmos_sensor(0x0016, shutter & 0x00ff);

	
/*it must allocate the dummy lines dynamically ,then the frame rate can be fixed in the desired value*/
	if(HM3451_sensor.fix_video_fps == KAL_TRUE)
  {
   SENSORDB("[HM3451][write_shutter]fix_video_fps\n");

	  //Dummy lines
	  HM3451_write_cmos_sensor(0x0010, (HM3451_sensor.pv_dummy_lines & 0xFF00)>>8);
	  HM3451_write_cmos_sensor(0x0011, HM3451_sensor.pv_dummy_lines & 0x00FF);
  
	  //Dummy pixels
	  HM3451_write_cmos_sensor(0x0013, HM3451_sensor.pv_dummy_pixels/12); 
	
  }
}	/* HM3451_write_shutter */

 kal_uint16 HM3451_Reg2Gain(kal_uint8 iReg)
{
	kal_uint8 iRegh = 0x00;
	kal_uint16 iGain = BASEGAIN;    // 1x-gain base

    // Range: 1x to 16x
	iRegh= iReg & 0x70;

	if (iRegh == 0x00){
		iGain = 1 * BASEGAIN;
	}else if (iRegh == 0x10){
		iGain = 2 * BASEGAIN;
	}else if (iRegh == 0x20){
		iGain = 4 * BASEGAIN;
	}else if (iRegh == 0x30){
		iGain = 8 * BASEGAIN;
	}/*else{
		iGain = 16 * BASEGAIN;
	}*/
	
	return iGain = iGain + iGain * (iReg & 0x0F) / 16;
}

kal_uint8 HM3451_Gain2Reg(const kal_uint16 iGain)
{
    kal_uint8 iReg = 0x00;

    if (iGain < 2 * BASEGAIN) {
        iReg = 16 * (iGain - BASEGAIN) / BASEGAIN;
    }else if (iGain < 4 * BASEGAIN) {
        iReg |= 0x10;
        iReg |= 8 * (iGain - 2 * BASEGAIN) / BASEGAIN;
    }else if (iGain < 8 * BASEGAIN) {
        iReg |= 0x20;
        iReg |= 4 * (iGain - 4 * BASEGAIN) / BASEGAIN;
    }else if (iGain < 16 * BASEGAIN) {
        iReg |= 0x30;
        iReg |= 2 * (iGain - 8 * BASEGAIN) / BASEGAIN;
    }/*else if (iGain < 32 * BASEGAIN) {
        iReg |= 0x40;
        iReg |= (iGain - 16 * BASEGAIN) / BASEGAIN;
    }*/else {
       SENSORDB("[HM3451][Error]Wrong sensor gain value!\n");
    }

    return iReg;
}


/*************************************************************************
* FUNCTION
*	HM3451_set_shutter
*
* DESCRIPTION
*	This function set e-shutter of HM3451 to change exposure time.
*
* PARAMETERS
*	shutter : exposured lines
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HM3451_set_shutter(kal_uint16 shutter)
{

   unsigned long flags;

	if (shutter <1 )
		shutter =1;
    else if(shutter > 65536)
	{

      SENSORDB("[HM3451][Error]set shutter value too big!!!\n");
	  shutter = 65536;
    }

	SENSORDB("[HM3451][Enter]set_shutter_func:shutter=%d,\n",shutter);

    spin_lock_irqsave(&hm3451raw_drv_lock,flags);
	HM3451_sensor.pv_shutter = shutter;
	spin_unlock_irqrestore(&hm3451raw_drv_lock,flags);

	HM3451_write_shutter(shutter);

	//For Highlight Purple---20100118
	if(shutter < 30)
	{
		HM3451_write_cmos_sensor(0x0057,0xB9);				//0xB8 -> 0xB9 Disable
		HM3451_write_cmos_sensor(0x006A,0x02); 
	}
	else if(shutter > 35)
	{
		HM3451_write_cmos_sensor(0x0057,0xB8);				//0xB9 -> 0xB8 Disable
		HM3451_write_cmos_sensor(0x006A,0x04); 
	}
    else
	{
		HM3451_write_cmos_sensor(0x0057,(HM3451_read_cmos_sensor(0x0057)));	
		HM3451_write_cmos_sensor(0x006A,(HM3451_read_cmos_sensor(0x006A))); 
	}
}	/* HM3451_set_shutter */

/*************************************************************************
* FUNCTION
*	HM3451_set_gain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*	gain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HM3451_set_gain(kal_uint16 iGain)
{

	/**************************************************************
		Gain Timming:
			____	____	____	____	____	____
			|	|	|	|	|	|	|	|	|	|	|	|	|	
		____|	|___|	|___|	|___|	|___|	|___|	|___|
			 {0}		   {1}	  {2}	  {3}	   {0}	   {1}
	shutter/sensor Gain 	 ISP Gain
	**************************************************************/
	kal_uint8 iReg = HM3451_Gain2Reg(iGain);

	//kal_uint8 iRegL=iReg & 0x0F;

	HM3451_write_cmos_sensor(0x0018, iReg);

	HM3451_write_cmos_sensor(0x0000,0xFF);
	
	SENSORDB("[HM3451]Set Sensor Gain = %d\n",iGain);
}

kal_uint16 HM3451_read_gain()
{
    kal_uint16 ireg_gain = HM3451_read_cmos_sensor(0x0018);
	kal_uint16 gain = 0;

	gain = HM3451_Reg2Gain(ireg_gain);

	return gain;

}

// write camera_para to sensor register
void HM3451_camera_para_to_sensor(void)
{
  kal_uint32  i;
  for(i=0; 0xFFFFFFFF!=HM3451SensorReg[i].Addr; i++)
  {
      HM3451_write_cmos_sensor(HM3451SensorReg[i].Addr, HM3451SensorReg[i].Para);
  }
  for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=HM3451SensorReg[i].Addr; i++)
  {
      HM3451_write_cmos_sensor(HM3451SensorReg[i].Addr, HM3451SensorReg[i].Para);
  }
  for(i=FACTORY_START_ADDR+1; i<5; i++)
  {
      HM3451_write_cmos_sensor(HM3451SensorCCT[i].Addr, HM3451SensorCCT[i].Para);
  }
}

// update camera_para from sensor register
void HM3451_sensor_to_camera_para(void)
{
  kal_uint32  i,temp_data=0;
  for(i=0; 0xFFFFFFFF!=HM3451SensorReg[i].Addr; i++)
  {
      temp_data= HM3451_read_cmos_sensor(HM3451SensorReg[i].Addr);
	  spin_lock(&hm3451raw_drv_lock);
	  HM3451SensorReg[i].Para=temp_data;
	  spin_unlock(&hm3451raw_drv_lock);
  }
  for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=HM3451SensorReg[i].Addr; i++)
  {
      temp_data= HM3451_read_cmos_sensor(HM3451SensorReg[i].Addr);
	  spin_lock(&hm3451raw_drv_lock);
	  HM3451SensorReg[i].Para=temp_data;
	  spin_unlock(&hm3451raw_drv_lock);
  }
}

//------------------------Engineer mode---------------------------------
kal_int32  HM3451_get_sensor_group_count(void)
{
  return GROUP_TOTAL_NUMS;
}

void HM3451_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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
      case FRAME_RATE_LIMITATION:
          sprintf((char *)group_name_ptr, "Frame Rate Limitation");
          *item_count_ptr = 2;
      break;
      case REGISTER_EDITOR:
          sprintf((char *)group_name_ptr, "Register Editor");
          *item_count_ptr = 2;
      break;
      default:
         SENSORDB("[HM3451][ERROR]get_sensor_group_info error!!!\n");
  }
}

void HM3451_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
  kal_int16 temp_reg=0;
  kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;
  kal_uint8 iRegbit = 0;

  switch (group_idx)
  {
      case PRE_GAIN:
          switch (item_idx)
          {
              case 0:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-R");
                  temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gr");
                  temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gb");
                  temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-B");
                  temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                 sprintf((char *)info_ptr->ItemNamePtr,"SENSOR_BASEGAIN");
                 temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 SENSORDB("[HM3451][Error]get_sensor_item_info error!!!\n");
          }

          temp_para=HM3451SensorCCT[temp_addr].Para;

           //Transfer Reg value to Gain value
			if (item_idx == 4){ //base_gain
				temp_gain = HM3451_Reg2Gain(temp_para);
				
				info_ptr->Min=1*1000;
                info_ptr->Max=8*1000;
			}else{	//R, G, B gain
				iRegbit= temp_para & 0xE0;               
				
				temp_gain = BASEGAIN + BASEGAIN * iRegbit / 256;
				//iReg2Gain = BASEGAIN + BASEGAIN * iRegbit / 16;

				info_ptr->Min=1000;
                info_ptr->Max=2500; //2x
			}
 
                info_ptr->ItemValue=1000*temp_gain/BASEGAIN;
                info_ptr->IsTrueFalse=KAL_FALSE;
                info_ptr->IsReadOnly=KAL_FALSE;
                info_ptr->IsNeedRestart=KAL_FALSE;

              break;
      case CMMCLK_CURRENT:
          switch (item_idx)
          {
              case 0:
                sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");

                temp_reg=HM3451SensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                 SENSORDB("[HM3451][ERROR]get_sensor_item_info:CMMCLK CURRENT error!!!\n");
          }
      break;
      case FRAME_RATE_LIMITATION:
          switch (item_idx)
          {
              case 0:
                sprintf((char *)info_ptr->ItemNamePtr,"Max Exposure Lines");
                info_ptr->ItemValue=HM3451_max_exposure_lines;
                info_ptr->IsTrueFalse=KAL_FALSE;
                info_ptr->IsReadOnly=KAL_TRUE;
                info_ptr->IsNeedRestart=KAL_FALSE;
                info_ptr->Min=0;
                info_ptr->Max=0;
              break;
              case 1:
                sprintf((char *)info_ptr->ItemNamePtr,"Min Frame Rate");
                info_ptr->ItemValue=12;
                info_ptr->IsTrueFalse=KAL_FALSE;
                info_ptr->IsReadOnly=KAL_TRUE;
                info_ptr->IsNeedRestart=KAL_FALSE;
                info_ptr->Min=0;
                info_ptr->Max=0;
              break;
              default:
                 SENSORDB("[HM3451][ERROR]get_sensor_item_info:frame rate error!!!\n");
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
                 SENSORDB("[HM3451][ERROR]get_sensor_item_info error!!!\n");
          }
      break;
      default:
          SENSORDB("[HM3451][ERROR]get_sensor_item_info error!!!\n");
  }
}

void HM3451_set_isp_driving_current(kal_uint8 current)
{
}

kal_bool HM3451_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
   kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;

   switch (group_idx)
  {
      case PRE_GAIN:
          switch (item_idx)
          {
              case 0:
                temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 SENSORDB("[HM3451][Error]set_sensor_item_info error!!!\n");
          }
   
		    temp_gain=((ItemValue*BASEGAIN+500)/1000);          //+500:get closed integer value

			//Gain value trnsfer to the Register value
			if (temp_addr == SENSOR_BASEGAIN){		
				temp_para = HM3451_Gain2Reg(temp_gain); 	

			}else{
                    temp_para = 256*(temp_gain - BASEGAIN)/BASEGAIN;
			     }
            spin_lock(&hm3451raw_drv_lock);
			HM3451SensorCCT[temp_addr].Para = temp_para;
			spin_unlock(&hm3451raw_drv_lock);
			HM3451_write_reg(HM3451SensorCCT[temp_addr].Addr, temp_para);

      break;
      case CMMCLK_CURRENT:
          switch (item_idx)
          {
              case 0:
			  	spin_lock(&hm3451raw_drv_lock);
                if(ItemValue==2)
                {
                    HM3451SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
                  //  HM3451_set_isp_driving_current(ISP_DRIVING_2MA);
                }
                else if(ItemValue==3 || ItemValue==4)
                {
                    HM3451SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
                   // HM3451_set_isp_driving_current(ISP_DRIVING_4MA);
                }
                else if(ItemValue==5 || ItemValue==6)
                {
                    HM3451SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
                   // HM3451_set_isp_driving_current(ISP_DRIVING_6MA);
                }
                else
                {
                    HM3451SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
                   // HM3451_set_isp_driving_current(ISP_DRIVING_8MA);
                }
				spin_unlock(&hm3451raw_drv_lock);
              break;
              default:
                 SENSORDB("[HM3451][ERROR]set_sensor_item_info error!!!\n");
          }
      break;
      case FRAME_RATE_LIMITATION:
         SENSORDB("[HM3451][ERROR]set_sensor_item_info error!!!\n");
      break;
      case REGISTER_EDITOR:
          switch (item_idx)
          {
              case 0:
			  	spin_lock(&hm3451raw_drv_lock);
                HM3451_FAC_SENSOR_REG=ItemValue;
				spin_unlock(&hm3451raw_drv_lock);
              break;
              case 1:
                HM3451_write_cmos_sensor(HM3451_FAC_SENSOR_REG,ItemValue);
              break;
              default:
                 SENSORDB("[HM3451][ERROR]set_sensor_item_info error!!!\n");
          }
      break;
      default:
         SENSORDB("[HM3451][ERROR]set_sensor_item_info error!!!\n");
  }

  return KAL_TRUE;
}


/*************************************************************************
* FUNCTION
* HM3451_night_mode
*
* DESCRIPTION
* This function night mode of HM3451.
*
* PARAMETERS
* none
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HM3451_night_mode(kal_bool enable)
{ 
  spin_lock(&hm3451raw_drv_lock);
  	HM3451_sensor.night_mode = enable;
	spin_unlock(&hm3451raw_drv_lock);
  
} /* HM3451_night_mode */


static kal_uint16 HM3451_power_on(void)
{
    kal_uint8 i;
	kal_uint16 HM3451_sensor_id = 0;
    SENSORDB("[HM3451][Enter]HM3451_power_on_func\n");
	
    SENSORDB("[HM3451]I2C_Write_ID=%x,I2C_Read_ID=%x\n",HM3451_sensor.i2c_write_id,HM3451_sensor.i2c_read_id);

    for(i=0;i<3;i++)
    {
    spin_lock(&hm3451raw_drv_lock);
	HM3451_sensor.i2c_write_id = HM3451_WRITE_ID_1;
	HM3451_sensor.i2c_read_id = HM3451_READ_ID_1;
	spin_unlock(&hm3451raw_drv_lock);
		//Soft Reset
	HM3451_write_cmos_sensor(0x0038, 0x00);
	mdelay(10);	

	HM3451_sensor_id = ((HM3451_read_cmos_sensor(0x0001) << 8) | HM3451_read_cmos_sensor(0x0002));
    SENSORDB("[HM3451]read SENSOR ID(1):I2C_Write_Id = %x,Sensor_ID = %x\n", HM3451_sensor.i2c_write_id,HM3451_sensor_id);
	
	if (HM3451_sensor_id != HM3451_SENSOR_ID)
	{	
	  spin_lock(&hm3451raw_drv_lock);
		HM3451_sensor.i2c_write_id = HM3451_WRITE_ID_2;
		HM3451_sensor.i2c_read_id = HM3451_READ_ID_2;
	spin_unlock(&hm3451raw_drv_lock);

		//Soft Reset
		HM3451_write_cmos_sensor(0x0038, 0x00);
		mdelay(10);	

		HM3451_sensor_id = ((HM3451_read_cmos_sensor(0x0001) << 8) | HM3451_read_cmos_sensor(0x0002));
		SENSORDB("[HM3451]read SENSOR ID(2):I2C_Write_Id = %x,Sensor_ID = %x\n", HM3451_sensor.i2c_write_id,HM3451_sensor_id);
	}
	if(HM3451_sensor_id == HM3451_SENSOR_ID)
		break;
    	}
	
	return HM3451_sensor_id;
}
static void HM3451_Init_Parameter(void)
{
   spin_lock(&hm3451raw_drv_lock);
    HM3451_sensor.pv_mode = KAL_FALSE;
    HM3451_sensor.night_mode = KAL_FALSE;
    HM3451_sensor.MPEG4_Video_mode = KAL_FALSE;
    HM3451_sensor.pv_pclk = 52000000;//47800000;//48750000;  
    HM3451_sensor.pv_dummy_pixels = 240;//168;//132;
    HM3451_sensor.pv_dummy_lines = 61;//92;//36;
	HM3451_sensor.cp_dummy_pixels = 0;
	HM3451_sensor.cp_dummy_lines = 6;//0;
    HM3451_sensor.mirror_flip = 0;  //normal:0, mirror:1, flip: 2, mirror_flip:3
   spin_unlock(&hm3451raw_drv_lock);
    
}

void HM3451_init_Sensor()
{   kal_uint8 SensorVerson = 0;
	//HM3451_write_cmos_sensor(0x0038,0xFF);
 
	SensorVerson = HM3451_read_cmos_sensor(0x0003);
	SENSORDB("[HM3451 IC Verson]0x0003-->%x\n",SensorVerson);
	
	HM3451_write_cmos_sensor(0x0004,0x18);
	HM3451_write_cmos_sensor(0x0006,0x00); 				//0x03 -> 0x00
	HM3451_write_cmos_sensor(0x000D,0x51); 				//0x00 -> 0x51 -> 0x11 Banding
	HM3451_write_cmos_sensor(0x000E,0x51); 				//0x00 -> 0x51 -> 0x11 Banding
	HM3451_write_cmos_sensor(0x000F,0x10);				//0x1A -> 0x18 -> 0x10
	
	HM3451_write_cmos_sensor(0x0010,0x00);
	HM3451_write_cmos_sensor(0x0011,0x06); 
	HM3451_write_cmos_sensor(0x0012,0xFF); 
	HM3451_write_cmos_sensor(0x0013,0x00);
	
	HM3451_write_cmos_sensor(0x0015,0x00);
	HM3451_write_cmos_sensor(0x0016,0xA3);
	
	HM3451_write_cmos_sensor(0x0017,0x00);				//Gain Mode 1 -> 3 ([1:0]00 -> 10) For Pregain
	HM3451_write_cmos_sensor(0x0018,0x00);
	HM3451_write_cmos_sensor(0x0019,0x00); 
	HM3451_write_cmos_sensor(0x001A,0x00); 
	HM3451_write_cmos_sensor(0x001B,0x00); 
	HM3451_write_cmos_sensor(0x001C,0x00); 
	HM3451_write_cmos_sensor(0x001D,0x40);       //DGain = 2x
	HM3451_write_cmos_sensor(0x001E,0x00); 
	HM3451_write_cmos_sensor(0x001F,0x10); 
	HM3451_write_cmos_sensor(0x0020,0x10); 
	HM3451_write_cmos_sensor(0x0021,0x10);
	
	HM3451_write_cmos_sensor(0x0022,0x33);				//BLC: 0x37 -> 0x33	
	HM3451_write_cmos_sensor(0x0023,0x15);
	//HM3451_write_cmos_sensor(0x0024,0x14); 
	//HM3451_write_cmos_sensor(0x0025,0x00); 
	//HM3451_write_cmos_sensor(0x0026,0x27);
	HM3451_write_cmos_sensor(0x0027,0x15);	
	HM3451_write_cmos_sensor(0x0028,0x14); 
	//HM3451_write_cmos_sensor(0x0029,0x00); 
	//HM3451_write_cmos_sensor(0x002A,0x00); 
	//HM3451_write_cmos_sensor(0x002B,0x00); 
	//HM3451_write_cmos_sensor(0x002C,0x00);	

	/*pclk setting:*/
    #if 1
	HM3451_write_cmos_sensor(0x0030,0x01);
	HM3451_write_cmos_sensor(0x0031,0x11); 				//0x12 -> 0x11  Divide by 1
	HM3451_write_cmos_sensor(0x0032,0x01); 
	HM3451_write_cmos_sensor(0x0033,0x20);
    #else
	HM3451_write_cmos_sensor(0x0030,0x11);
	HM3451_write_cmos_sensor(0x0031,0x11); 				//0x12 -> 0x11  Divide by 1
	HM3451_write_cmos_sensor(0x0032,0x01); 
	HM3451_write_cmos_sensor(0x0033,0x1E);
	HM3451_write_cmos_sensor(0x0030,0x04);
	mdelay(1);
    #endif
	
	HM3451_write_cmos_sensor(0x0034,0x00);
	
	HM3451_write_cmos_sensor(0x0035,0x80);
	HM3451_write_cmos_sensor(0x0036,0xee);   //0xfc           //0xcc -> 0xfc: For Driving Current
	HM3451_write_cmos_sensor(0x0037,0x00);
	
	HM3451_write_cmos_sensor(0x0039,0x14); 
	HM3451_write_cmos_sensor(0x003A,0x00); 
	HM3451_write_cmos_sensor(0x003B,0x2F);
	
	HM3451_write_cmos_sensor(0x0040,0x09);				//0x49->0x09 for BLC issue//Analog Block: 
	HM3451_write_cmos_sensor(0x0041,0x11); 
	HM3451_write_cmos_sensor(0x0042,0x07);				//0x1B -> 0x24 -> 0x17 -> 0x07  Black Point in Sun
	HM3451_write_cmos_sensor(0x0043,0x01);        //from Himax RD sugester, 0x06 -> 0x01,to modify sensor analog performance, not affect any sensor IQ
	HM3451_write_cmos_sensor(0x0044,0x00); 
	HM3451_write_cmos_sensor(0x0045,0x10);				//0x00 -> 0x10  
	HM3451_write_cmos_sensor(0x0046,0x01);
	
	HM3451_write_cmos_sensor(0x0048,0x02); 
	HM3451_write_cmos_sensor(0x0049,0x03); 
	HM3451_write_cmos_sensor(0x004A,0x04); 
	HM3451_write_cmos_sensor(0x004B,0x14);  //BLC 0x24->0x14
	HM3451_write_cmos_sensor(0x004C,0x34);  //BLC 0x44->0x34
	HM3451_write_cmos_sensor(0x004D,0x09); 
	HM3451_write_cmos_sensor(0x004E,0x0A); 
	HM3451_write_cmos_sensor(0x004F,0x0B); 
	HM3451_write_cmos_sensor(0x0050,0x03);//0x0C
	
	HM3451_write_cmos_sensor(0x0051,0x04);				//0x2C -> 0x04  
	HM3451_write_cmos_sensor(0x0052,0x14);				//BLC 0x24->0x14//0x4C -> 0x24   
	HM3451_write_cmos_sensor(0x0053,0x34);				//BLC 0x44->0x34//0xCC -> 0x44   
	HM3451_write_cmos_sensor(0x0054,0xC5);				//0xC4->0xC5 for BLC issue//0xC4 -> 0xCC -> 0xC8 ->  0xC4: For Low Light Capture Blue
	HM3451_write_cmos_sensor(0x0055,0xD2); 
	if((SensorVerson == 0x00)||(SensorVerson == 0x01))
	HM3451_write_cmos_sensor(0x0056,0x21); 
	else if(SensorVerson == 0x04)
	HM3451_write_cmos_sensor(0x0056,0x62);	
	
	HM3451_write_cmos_sensor(0x0057,0xB8);				//0xA0 -> 0xB8   Native pump[0]:0---Enable;1---Disable

	HM3451_write_cmos_sensor(0x0058,0x0F); 
	HM3451_write_cmos_sensor(0x0059,0x1F); 
	HM3451_write_cmos_sensor(0x005A,0x1F); 
	HM3451_write_cmos_sensor(0x005B,0x1F); 
	HM3451_write_cmos_sensor(0x005C,0x1F); 
	HM3451_write_cmos_sensor(0x005D,0x0F); 
	HM3451_write_cmos_sensor(0x005E,0x1F); 
	HM3451_write_cmos_sensor(0x005F,0x1F); 
	HM3451_write_cmos_sensor(0x0060,0xFF); 
	HM3451_write_cmos_sensor(0x0061,0x5F); 
	HM3451_write_cmos_sensor(0x0062,0x33); 
	HM3451_write_cmos_sensor(0x0063,0x0F); 
	HM3451_write_cmos_sensor(0x0064,0x20); 
	HM3451_write_cmos_sensor(0x0065,0xA4); 
	HM3451_write_cmos_sensor(0x0066,0x0d);  //0x0f->0x05 for BLC issue
	HM3451_write_cmos_sensor(0x0067,0x2d); 
	HM3451_write_cmos_sensor(0x0068,0x80); 
	HM3451_write_cmos_sensor(0x0069,0x04); 
	HM3451_write_cmos_sensor(0x006A,0x04); 
	HM3451_write_cmos_sensor(0x006B,0x09); 
	HM3451_write_cmos_sensor(0x006C,0x05); 
	HM3451_write_cmos_sensor(0x006D,0x0E);  //need to be 0x09
	HM3451_write_cmos_sensor(0x006E,0x05); 
	HM3451_write_cmos_sensor(0x006F,0x31); 
	HM3451_write_cmos_sensor(0x0070,0x00); 
	HM3451_write_cmos_sensor(0x0071,0x10); 
	HM3451_write_cmos_sensor(0x0072,0x08); 
	HM3451_write_cmos_sensor(0x0073,0x17); 
	HM3451_write_cmos_sensor(0x0074,0x0B); 
	HM3451_write_cmos_sensor(0x0075,0x18); 
	HM3451_write_cmos_sensor(0x0076,0x08); 
	HM3451_write_cmos_sensor(0x0077,0x14); 
	HM3451_write_cmos_sensor(0x0078,0x01); 
	HM3451_write_cmos_sensor(0x0079,0x0C); 
	HM3451_write_cmos_sensor(0x007A,0x17); 
	HM3451_write_cmos_sensor(0x007B,0x22); 
	HM3451_write_cmos_sensor(0x007C,0x07); 
	HM3451_write_cmos_sensor(0x007D,0x10); 
	HM3451_write_cmos_sensor(0x007E,0x1A); 
	HM3451_write_cmos_sensor(0x007F,0x23); 
	HM3451_write_cmos_sensor(0x0080,0x01); 
	HM3451_write_cmos_sensor(0x0081,0x00); 
	HM3451_write_cmos_sensor(0x0082,0x01); 
	HM3451_write_cmos_sensor(0x0083,0x00); 
	HM3451_write_cmos_sensor(0x0084,0x01); 
	HM3451_write_cmos_sensor(0x0085,0x00); 
	HM3451_write_cmos_sensor(0x0086,0x01); 
	HM3451_write_cmos_sensor(0x0087,0x00); 
	HM3451_write_cmos_sensor(0x0090,0x00); 
	HM3451_write_cmos_sensor(0x0091,0x11); 
	HM3451_write_cmos_sensor(0x0092,0x22); 
	HM3451_write_cmos_sensor(0x0093,0x33); 
	HM3451_write_cmos_sensor(0x0094,0x44); 
	HM3451_write_cmos_sensor(0x0095,0x44); 
	HM3451_write_cmos_sensor(0x0096,0x55); 
	HM3451_write_cmos_sensor(0x0097,0x55); 
	HM3451_write_cmos_sensor(0x0100,0xFF); 
	HM3451_write_cmos_sensor(0x0101,0x02);
	
	//HM3451_write_cmos_sensor(0x0102,0x39);  //bit[1]: Lens shade enable(0:disable, 1:enable)
#if 1
	HM3451_write_cmos_sensor(0x0102,0x05);  //bit[1]: Lens shade enable(0:disable, 1:enable)
#else
	HM3451_write_cmos_sensor(0x0102,0x08);  //bit[1]: Lens shade enable(0:disable, 1:enable)
#endif
	HM3451_write_cmos_sensor(0x0103,0x00); 
	HM3451_write_cmos_sensor(0x0104,0x00);
	
	HM3451_write_cmos_sensor(0x0105,0x00); 
	HM3451_write_cmos_sensor(0x0106,0x00); 
	HM3451_write_cmos_sensor(0x0108,0x00); 
	HM3451_write_cmos_sensor(0x0109,0x80);
	
	HM3451_write_cmos_sensor(0x010A,0x02);
	HM3451_write_cmos_sensor(0x010B,0x08);
	
	HM3451_write_cmos_sensor(0x010C,0x54); 
	HM3451_write_cmos_sensor(0x010D,0x00);
	
	HM3451_write_cmos_sensor(0x0110,0x01);
	HM3451_write_cmos_sensor(0x0111,0x33); 
	HM3451_write_cmos_sensor(0x0112,0x14); 
	HM3451_write_cmos_sensor(0x0113,0x14); 
	HM3451_write_cmos_sensor(0x0114,0x14); 
	HM3451_write_cmos_sensor(0x0115,0x14); 
	HM3451_write_cmos_sensor(0x0116,0x14); 
	HM3451_write_cmos_sensor(0x0117,0x14); 
	HM3451_write_cmos_sensor(0x0118,0x14); 
	HM3451_write_cmos_sensor(0x0119,0x14); 
	HM3451_write_cmos_sensor(0x011A,0x14); 
	HM3451_write_cmos_sensor(0x011B,0x14); 
	HM3451_write_cmos_sensor(0x011C,0x14); 
	HM3451_write_cmos_sensor(0x011D,0x14);
	
	HM3451_write_cmos_sensor(0x011E,0x14); 
	HM3451_write_cmos_sensor(0x0121,0xF0); 
	HM3451_write_cmos_sensor(0x0122,0xF0); 
	HM3451_write_cmos_sensor(0x0123,0xF0); 
	HM3451_write_cmos_sensor(0x0130,0x02); 
	HM3451_write_cmos_sensor(0x0200,0xFF); 
	HM3451_write_cmos_sensor(0x0201,0x02); 
	HM3451_write_cmos_sensor(0x0202,0x60); 
	HM3451_write_cmos_sensor(0x0203,0x02); 
	HM3451_write_cmos_sensor(0x0204,0x83); 
	HM3451_write_cmos_sensor(0x0205,0x03); 
	HM3451_write_cmos_sensor(0x0206,0xDF); 
	HM3451_write_cmos_sensor(0x0207,0x03); 
	HM3451_write_cmos_sensor(0x0208,0xA3); 
	HM3451_write_cmos_sensor(0x0209,0x02); 
	HM3451_write_cmos_sensor(0x020A,0x8B); 
	HM3451_write_cmos_sensor(0x020B,0x03); 
	HM3451_write_cmos_sensor(0x020C,0xE3); 
	HM3451_write_cmos_sensor(0x020D,0x03); 
	HM3451_write_cmos_sensor(0x020E,0x18); 
	HM3451_write_cmos_sensor(0x020F,0x02); 
	HM3451_write_cmos_sensor(0x0210,0xA3); 
	HM3451_write_cmos_sensor(0x0211,0x03); 
	HM3451_write_cmos_sensor(0x0212,0xDF); 
	HM3451_write_cmos_sensor(0x0213,0x02); 
	HM3451_write_cmos_sensor(0x0214,0xD7); 
	HM3451_write_cmos_sensor(0x0215,0x02); 
	HM3451_write_cmos_sensor(0x0216,0xB5); 
	HM3451_write_cmos_sensor(0x0217,0x03); 
	HM3451_write_cmos_sensor(0x0218,0xE5); 
	HM3451_write_cmos_sensor(0x0219,0x16); 
	HM3451_write_cmos_sensor(0x021A,0x0B); 
	HM3451_write_cmos_sensor(0x021B,0x16); 
	HM3451_write_cmos_sensor(0x021C,0x0B); 
	HM3451_write_cmos_sensor(0x021D,0x16); 
	HM3451_write_cmos_sensor(0x021E,0x0B); 
	HM3451_write_cmos_sensor(0x021F,0x16); 
	HM3451_write_cmos_sensor(0x0220,0x0B); 
	HM3451_write_cmos_sensor(0x0221,0x04); 
	HM3451_write_cmos_sensor(0x0222,0x28); 
	HM3451_write_cmos_sensor(0x0223,0x04); 
	HM3451_write_cmos_sensor(0x0224,0x65); 
	HM3451_write_cmos_sensor(0x0225,0x04); 
	HM3451_write_cmos_sensor(0x0226,0x4B); 
	HM3451_write_cmos_sensor(0x0227,0x04); 
	HM3451_write_cmos_sensor(0x0228,0x62); 
	HM3451_write_cmos_sensor(0x0229,0x03); 
	HM3451_write_cmos_sensor(0x022A,0x22); 
	HM3451_write_cmos_sensor(0x022B,0x03); 
	HM3451_write_cmos_sensor(0x022C,0x23); 
	HM3451_write_cmos_sensor(0x022D,0x03); 
	HM3451_write_cmos_sensor(0x022E,0x1C); 
	HM3451_write_cmos_sensor(0x022F,0x03); 
	HM3451_write_cmos_sensor(0x0230,0x1F); 
	HM3451_write_cmos_sensor(0x0231,0x0F); 
	HM3451_write_cmos_sensor(0x0232,0x04);
	
	HM3451_write_cmos_sensor(0x0300,0x00);
	
	HM3451_write_cmos_sensor(0x0301,0x04);				//StartX 0x0000 -> 0x0022
	HM3451_write_cmos_sensor(0x0302,0x22);
	HM3451_write_cmos_sensor(0x0303,0x04);				//EndX 0x0887 -> 0x0421
	HM3451_write_cmos_sensor(0x0304,0x00);          //modified by 2011-5-24  0x21 ->0x00
	HM3451_write_cmos_sensor(0x0305,0x0d);				//modified by 2011-5-24  0x00 ->0x0d
	HM3451_write_cmos_sensor(0x0306,0x00);
	HM3451_write_cmos_sensor(0x0307,0xc0);				//EndY 0x0617 -> 0x0305
	HM3451_write_cmos_sensor(0x0308,0x00);

	HM3451_write_cmos_sensor(0x0309,0x08); 
	HM3451_write_cmos_sensor(0x030A,0x00); 
	HM3451_write_cmos_sensor(0x030B,0x02); 
	HM3451_write_cmos_sensor(0x030C,0x88); 
	HM3451_write_cmos_sensor(0x030D,0x00); 
	HM3451_write_cmos_sensor(0x030E,0x00); 
	HM3451_write_cmos_sensor(0x030F,0x06); 
	HM3451_write_cmos_sensor(0x0310,0x18);
	
	HM3451_write_cmos_sensor(0x4081,0x00); 
	HM3451_write_cmos_sensor(0x4082,0x00); 
	HM3451_write_cmos_sensor(0x4083,0x00); 
	HM3451_write_cmos_sensor(0x4084,0x00); 
	HM3451_write_cmos_sensor(0x4085,0x00); 
	HM3451_write_cmos_sensor(0x4090,0x07);
	
	HM3451_write_cmos_sensor(0x4091,0x00);
	
	HM3451_write_cmos_sensor(0x4092,0x00); 
	HM3451_write_cmos_sensor(0x4093,0xC8); 
	HM3451_write_cmos_sensor(0x4094,0x8F); 
	HM3451_write_cmos_sensor(0x4095,0x00); 
	HM3451_write_cmos_sensor(0x4096,0x03); 
	HM3451_write_cmos_sensor(0x4097,0x07); 
	HM3451_write_cmos_sensor(0x4098,0x00); 
	HM3451_write_cmos_sensor(0x4099,0x31); 
	HM3451_write_cmos_sensor(0x409A,0x04); 
	HM3451_write_cmos_sensor(0x409B,0x04); 
	HM3451_write_cmos_sensor(0x409C,0xFD); 
	HM3451_write_cmos_sensor(0x409D,0xFD); 
	HM3451_write_cmos_sensor(0x409E,0x03); 
	HM3451_write_cmos_sensor(0x409F,0x0C); 
	HM3451_write_cmos_sensor(0x0005,0x01);
	
	HM3451_write_cmos_sensor(0x0000,0x01);

}

/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
* HM3451Open
*
* DESCRIPTION
* This function initialize the registers of CMOS sensor
*
* PARAMETERS
* None
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HM3451Open(void)
{

     SENSORDB("[HM3451][Enter]HM3451_Open func\n");
  
   if (HM3451_power_on() != HM3451_SENSOR_ID) 
   {
	 SENSORDB("[HM3451]Error:read sensor ID fail\n");
	 return ERROR_SENSOR_CONNECT_FAIL;
   }
 
  //Init Setting(Preview)
  HM3451_init_Sensor();
  HM3451_Init_Parameter();
  HM3451_camera_para_to_sensor();

  return ERROR_NONE;
} /* HM3451Open() */

UINT32 HM3451Getsensorid(UINT32 *sensorID)
{
	kal_uint32 sensorid=0;
	
	sensorid=HM3451_power_on();
	if (sensorid!= HM3451_SENSOR_ID) 
   {
   	 *sensorID=0xffffffff;
	 SENSORDB("[HM3451]Error:read sensor ID fail\n");
	 return ERROR_SENSOR_CONNECT_FAIL;
   }
	SENSORDB("[HM3451][Enter]HM3451_Getsensoid:%d\n",sensorid);
   return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
* HM3451Close
*
* DESCRIPTION
* This function is to turn off sensor module power.
*
* PARAMETERS
* None
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HM3451Close(void)
{
//    CISModulePowerOn(FALSE);

	
  return ERROR_NONE;
} /* HM3451Close() */

static void HM3451_PV_Mode_Setting(void)
{
	//Preview Size: 1024*768
	//  HM3451_write_cmos_sensor(0x0006,0x00);			  //0x03 -> 0x00
	  #if defined(HM3451_NORMAL_VIEW)
	{
	spin_lock(&hm3451raw_drv_lock);
	  HM3451_sensor.mirror_flip = 0;  //normal 
	 spin_unlock(&hm3451raw_drv_lock);
	  HM3451_write_cmos_sensor(0x0006,0x03);
	}
    #elif defined(HM3451_MIRROR_VIEW)
    {
    spin_lock(&hm3451raw_drv_lock);
	  HM3451_sensor.mirror_flip = 1;  //mirror
	 spin_unlock(&hm3451raw_drv_lock);
	  HM3451_write_cmos_sensor(0x0006,0x01);
    }
    #elif defined(HM3451_FLIP_VIEW)
    {
    spin_lock(&hm3451raw_drv_lock);
	  HM3451_sensor.mirror_flip = 2;  //flip
	spin_unlock(&hm3451raw_drv_lock);
	  HM3451_write_cmos_sensor(0x0006,0x02);
    }
    #elif defined(HM3451_MIRROR_FLIP_VIEW)
    {
    spin_lock(&hm3451raw_drv_lock);
	  HM3451_sensor.mirror_flip = 3;  //mirror_flip
	spin_unlock(&hm3451raw_drv_lock);
	  HM3451_write_cmos_sensor(0x0006,0x00); 
    }
    #else  //modify for ywj for macro control in make fil  110421
    {
    spin_lock(&hm3451raw_drv_lock);
	HM3451_sensor.mirror_flip = 0;  //normal 
	spin_unlock(&hm3451raw_drv_lock);
	HM3451_write_cmos_sensor(0x0006,0x03);
    }
    #endif
	  HM3451_write_cmos_sensor(0x000D,0x51);			  //0x00 -> 0x51
	  HM3451_write_cmos_sensor(0x000E,0x51);			  //0x00 -> 0x51
	  HM3451_write_cmos_sensor(0x000F,0x10);			  //0x1A -> 0x18 -> 0x10
	  
	  HM3451_write_cmos_sensor(0x0301,0x00);			  //StartX 0x0000 -> 0x0022
	  HM3451_write_cmos_sensor(0x0302,0x22);
	  HM3451_write_cmos_sensor(0x0303,0x04);			  //EndX 0x0887 -> 0x0421
	  HM3451_write_cmos_sensor(0x0304,0x21);
	  HM3451_write_cmos_sensor(0x0305,0x00);			  //StartY 0x0000 -> 0x0006
	  HM3451_write_cmos_sensor(0x0306,0x06);
	  HM3451_write_cmos_sensor(0x0307,0x03);			  //EndY 0x0617 -> 0x0305
	  HM3451_write_cmos_sensor(0x0308,0x05);

}
static void HM3451_CP_Mode_Setting(void)
{ // full size: 2048*1536
	//HM3451_write_cmos_sensor(0x0006,0x03);			//0x03 -> 0x00
	#if 1
	HM3451_write_cmos_sensor(0x000D,0x00);	
	HM3451_write_cmos_sensor(0x000E,0x00);	
	HM3451_write_cmos_sensor(0x000F,0x16);			//0x1A -> 0x00	
	HM3451_write_cmos_sensor(0x0301,0x00); 	
	HM3451_write_cmos_sensor(0x0302,0x44); 	
	HM3451_write_cmos_sensor(0x0303,0x08); 	
	HM3451_write_cmos_sensor(0x0304,0x43); 	
	HM3451_write_cmos_sensor(0x0305,0x00); 	
	HM3451_write_cmos_sensor(0x0306,0x0C); 	
	HM3451_write_cmos_sensor(0x0307,0x06); 	
	HM3451_write_cmos_sensor(0x0308,0x0b); 	
	#else
	HM3451_write_cmos_sensor(0x000D,0x00);
	HM3451_write_cmos_sensor(0x000E,0x00);
	HM3451_write_cmos_sensor(0x000F,0x1A);			//0x1A -> 0x00
	HM3451_write_cmos_sensor(0x0301,0x00); 
	HM3451_write_cmos_sensor(0x0302,0x00); 
	HM3451_write_cmos_sensor(0x0303,0x08); 
	HM3451_write_cmos_sensor(0x0304,0x87); 
	HM3451_write_cmos_sensor(0x0305,0x00); 
	HM3451_write_cmos_sensor(0x0306,0x00); 
	HM3451_write_cmos_sensor(0x0307,0x06); 
	HM3451_write_cmos_sensor(0x0308,0x17); 
	#endif
}

/*************************************************************************
* FUNCTION
* HM3451Preview
*
* DESCRIPTION
* This function start the sensor preview.
*
* PARAMETERS
* *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HM3451Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

  kal_uint8  start_grab_x_offset=0, start_grab_y_offset=0;
	  kal_uint16 iGrabWidth,iGrabheight; 

	SENSORDB("[HM3451][Enter]HM3451_preview_func:\n");
 

       if(HM3451_sensor.pv_mode == KAL_FALSE)
       	{
       	 spin_lock(&hm3451raw_drv_lock);
	      HM3451_sensor.pv_mode =  KAL_TRUE;
		 spin_unlock(&hm3451raw_drv_lock);
          HM3451_PV_Mode_Setting();	  
       	}
  
  /********************************************************
  * Reg0x0006 bit[1:0] :Mirror,Filp
  *********************************************************/
  spin_lock(&hm3451raw_drv_lock);
	  HM3451_sensor.pv_pclk = 52000000;//47800000;//48750000;
	  HM3451_sensor.pv_dummy_pixels = 240;//132;//dummy_pixels must be 12s
	  HM3451_sensor.pv_dummy_lines = 61;//36;//10;
	  HM3451_sensor.pv_line_length = HM3451_PV_PERIOD_PIXEL_NUMS +  HM3451_sensor.pv_dummy_pixels; 
	  HM3451_sensor.pv_frame_length = HM3451_PV_PERIOD_LINE_NUMS +  HM3451_sensor.pv_dummy_lines;
	  HM3451_sensor.fix_video_fps = KAL_FALSE;
   spin_unlock(&hm3451raw_drv_lock);
	//Dummy lines
	HM3451_write_cmos_sensor(0x0010, (HM3451_sensor.pv_dummy_lines & 0xFF00)>>8);
	HM3451_write_cmos_sensor(0x0011, HM3451_sensor.pv_dummy_lines & 0x00FF);

	//Dummy pixels
	HM3451_write_cmos_sensor(0x0013, HM3451_sensor.pv_dummy_pixels/12);

      memcpy(&HM3451SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

      return ERROR_NONE;
}	/* HM3451Preview() */

UINT32 HM3451Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	    kal_uint8  start_grab_x_offset=0, start_grab_y_offset=0;

		kal_uint16 shutter = HM3451_sensor.pv_shutter;

		SENSORDB("[HM3451][Enter]HM3451_capture_func\n");

		HM3451_CP_Mode_Setting();
		
	spin_lock(&hm3451raw_drv_lock);
		HM3451_sensor.pv_mode = KAL_FALSE;
		HM3451_sensor.cp_pclk = 52000000;//47800000;
		HM3451_sensor.cp_dummy_pixels = 0; //dummy_pixels must be 12s
		HM3451_sensor.cp_dummy_lines = 6;
		HM3451_sensor.cp_line_length = HM3451_FULL_PERIOD_PIXEL_NUMS +  HM3451_sensor.cp_dummy_pixels; 
	    HM3451_sensor.cp_frame_length = HM3451_FULL_PERIOD_LINE_NUMS +  HM3451_sensor.cp_dummy_lines;
	spin_unlock(&hm3451raw_drv_lock);	
		HM3451_write_cmos_sensor(0x0010, (HM3451_sensor.cp_dummy_lines & 0xFF00)>>8);

		HM3451_write_cmos_sensor(0x0011, HM3451_sensor.cp_dummy_lines & 0x00FF);
	
	    //Dummy pixels
    	HM3451_write_cmos_sensor(0x0013, HM3451_sensor.cp_dummy_pixels/12);


		//calculate cp shutter 
		shutter = shutter *( HM3451_sensor.cp_pclk/HM3451_sensor.pv_pclk);
		shutter = (shutter * HM3451_sensor.pv_line_length)/HM3451_sensor.cp_line_length;

		HM3451_write_shutter(shutter);
		 
	    mdelay(210);//min fps:5fps,so frame time=200  //must delay 1 frame else capture fail,because in one frame only one time 0x0000 register
		HM3451_write_cmos_sensor(0x0000,0x01);//add to debug	 

	return ERROR_NONE;
}	/* HM3451Capture() */

UINT32 HM3451GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth = HM3451_IMAGE_SENSOR_FULL_WIDTH;
	pSensorResolution->SensorFullHeight = HM3451_IMAGE_SENSOR_FULL_HEIGHT;
	pSensorResolution->SensorPreviewWidth = HM3451_IMAGE_SENSOR_PV_WIDTH;
	pSensorResolution->SensorPreviewHeight = HM3451_IMAGE_SENSOR_PV_HEIGHT;

	return ERROR_NONE;
}	/* HM3451GetResolution() */

UINT32 HM3451GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX=HM3451_IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=HM3451_IMAGE_SENSOR_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX=HM3451_IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=HM3451_IMAGE_SENSOR_FULL_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_Gb;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;/*must be low */
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;/*must be low */
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;/*must be high */
	pSensorInfo->SensorInterruptDelayLines = 1;
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_3M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_3M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_3M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_3M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_3M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_3M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_3M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_3M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_3M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_3M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;

	pSensorInfo->CaptureDelayFrame = 2; 
	pSensorInfo->PreviewDelayFrame = 2; 
	pSensorInfo->VideoDelayFrame = 2; 
	pSensorInfo->SensorMasterClockSwitch = 0;//1;//0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;//ISP_DRIVING_8MA;   	
	pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;    /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 1;	/*ISP gain delay frame must be 1*/

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			pSensorInfo->SensorClockFreq=52;//48;
			pSensorInfo->SensorClockDividCount=	4;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 4;
			pSensorInfo->SensorDataLatchCount= 2;
			pSensorInfo->SensorGrabStartX = HM3451_PV_START_X;
			pSensorInfo->SensorGrabStartY = HM3451_PV_START_Y; 		
		    break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			pSensorInfo->SensorClockFreq=52;//48;
			pSensorInfo->SensorClockDividCount= 4;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=4;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = HM3451_CP_START_X; 
			pSensorInfo->SensorGrabStartY = HM3451_CP_START_Y;			
		break;
		default:
			pSensorInfo->SensorClockFreq=52;//48;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorClockDividCount=4;
			pSensorInfo->SensorPixelClockCount=4;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = HM3451_PV_START_X; 
			pSensorInfo->SensorGrabStartY = HM3451_PV_START_Y;			
		break;
	}
//	HM3451_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &HM3451SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* HM3451GetInfo() */


UINT32 HM3451Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			HM3451Preview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			HM3451Capture(pImageWindow, pSensorConfigData);
		break;

        default:
            return ERROR_INVALID_SCENARIO_ID;
 	}
	return TRUE;
}	/* HM3451Control() */
/*************************************************************************
* FUNCTION
*	HM3451_Fix_Video_Frame_Rate
*

* DESCRIPTION
*	This function is to set dummy line to sensor for video fixed frame rate.
*
* PARAMETERS
*	framerate : Video Fix frame rate 
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
//TO DO :need to write fix frame rate func
static void HM3451_Fix_Video_Frame_Rate(kal_uint16 framerate)
{    
	kal_uint16 HM3451_Video_Max_Line_Length = 0;
	kal_uint16 HM3451_Video_Max_Expourse_Time = 0;

    framerate = framerate * 10;
	spin_lock(&hm3451raw_drv_lock);
	HM3451_sensor.fix_video_fps = KAL_TRUE;
	spin_unlock(&hm3451raw_drv_lock);
	
	SENSORDB("[HM3451][Enter Fix_fps func] HM3451_Fix_Video_Frame_Rate = %d\n", framerate/10);


	HM3451_Video_Max_Expourse_Time = (kal_uint16)((HM3451_sensor.pv_pclk*10/framerate)/HM3451_sensor.pv_line_length);
	
    if (HM3451_Video_Max_Expourse_Time < (HM3451_PV_PERIOD_LINE_NUMS))			
	    HM3451_Video_Max_Expourse_Time = HM3451_PV_PERIOD_LINE_NUMS ;	

    //Dummy pixels
    spin_lock(&hm3451raw_drv_lock);
    HM3451_sensor.pv_dummy_lines = HM3451_Video_Max_Expourse_Time - HM3451_PV_PERIOD_LINE_NUMS;
	HM3451_Fix_fps_dummy_lines = HM3451_sensor.pv_dummy_lines;
    spin_unlock(&hm3451raw_drv_lock);
	//Dummy lines
	HM3451_write_cmos_sensor(0x0010, (HM3451_sensor.pv_dummy_lines & 0xFF00)>>8);
	HM3451_write_cmos_sensor(0x0011, HM3451_sensor.pv_dummy_lines & 0x00FF);

	//Dummy pixels
	HM3451_write_cmos_sensor(0x0013, HM3451_sensor.pv_dummy_pixels/12);

}


UINT32 HM3451SetVideoMode(UINT16 u2FrameRate)
{
    kal_uint16 iTemp;

    SENSORDB("[HM3451][Enter]Set_Video_mode_func:fps = %d\n",u2FrameRate);
	spin_lock(&hm3451raw_drv_lock); 
	HM3451_sensor.video_current_frame_rate = u2FrameRate;
    spin_unlock(&hm3451raw_drv_lock);
    if(u2FrameRate<=30)

	  HM3451_Fix_Video_Frame_Rate(u2FrameRate);

    else 
      SENSORDB("[HM3451][Error]Wrong frame rate setting %d\n", u2FrameRate);

    return TRUE;
}
/*
UINT32 HM3451SetCalData(PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData)
{
	UINT32 i;

	SENSORDB("HM3451 Sensor write calibration data num = %d \r\n", pSetSensorCalData->DataSize);
	SENSORDB("HM3451 Sensor write calibration data format = %x \r\n", pSetSensorCalData->DataFormat);	
	if(pSetSensorCalData->DataSize <= MAX_SHADING_DATA_TBL)
	{
		for (i = 0; i < pSetSensorCalData->DataSize; i++)
		{
			if (((pSetSensorCalData->DataFormat & 0xFFFF) == 1) && ((pSetSensorCalData->DataFormat >> 16) == 1))
			{
				SENSORDB("HM3451 Sensor write calibration data: address = %x, value = %x \r\n",(pSetSensorCalData->ShadingData[i])>>16,(pSetSensorCalData->ShadingData[i])&0xFFFF);
				//HM3451_write_cmos_sensor_16Bit((pSetSensorCalData->ShadingData[i])>>16, (pSetSensorCalData->ShadingData[i])&0xFFFF);
			}
		}
	}
	return ERROR_NONE;
}
*/
UINT32 HM3451FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
	PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData=(PSET_SENSOR_CALIBRATION_DATA_STRUCT)pFeaturePara;
	switch (FeatureId)
	{
      case SENSOR_FEATURE_GET_RESOLUTION:
          *pFeatureReturnPara16++=HM3451_IMAGE_SENSOR_FULL_WIDTH;
          *pFeatureReturnPara16=HM3451_IMAGE_SENSOR_FULL_HEIGHT;
          *pFeatureParaLen=4;
      break;
      case SENSOR_FEATURE_GET_PERIOD:
	  	  SENSORDB("[HM3451]feature control:get_period!!!\n");
          *pFeatureReturnPara16++=HM3451_PV_PERIOD_PIXEL_NUMS + HM3451_sensor.pv_dummy_pixels;
          *pFeatureReturnPara16=HM3451_PV_PERIOD_LINE_NUMS + HM3451_sensor.pv_dummy_lines;
          *pFeatureParaLen=4;
      break;
      case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
	  	  SENSORDB("[HM3451]feature control:get_pixel_clock_freq!!\n");
          *pFeatureReturnPara32 = HM3451_sensor.pv_pclk;//HM3451_sensor_pclk;
          *pFeatureParaLen=4;
      break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			HM3451_set_shutter(*pFeatureData16);
		break;
      case SENSOR_FEATURE_SET_NIGHTMODE:
	  	    SENSORDB("[HM3451]feature control:set_NightMode!!\n");//jun debug
            HM3451_night_mode((BOOL) *pFeatureData16);
      break;
		case SENSOR_FEATURE_SET_GAIN:
			HM3451_set_gain((UINT16) *pFeatureData16);
		break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			SENSORDB("[HM3451]feature control:set_isp_master_clock_freq!!\n");
			spin_lock(&hm3451raw_drv_lock);
            HM3451_isp_master_clock=*pFeatureData32;
			spin_unlock(&hm3451raw_drv_lock);
		break;
		case SENSOR_FEATURE_SET_REGISTER:
            HM3451_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = HM3451_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
	/*		SensorRegNumber=*pFeatureData32++;
			RETAILMSG(1, (TEXT("HM3451 Sensor Reg No %x \r\n"),SensorRegNumber));
			if (SensorRegNumber>FACTORY_END_ADDR)
				return FALSE;*/
			SensorRegNumber=FACTORY_END_ADDR;
			for (i=0;i<SensorRegNumber;i++)
			{
			  spin_lock(&hm3451raw_drv_lock);
				HM3451SensorCCT[i].Addr=*pFeatureData32++;
				HM3451SensorCCT[i].Para=*pFeatureData32++;
		     spin_unlock(&hm3451raw_drv_lock);
			}
		break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=HM3451SensorCCT[i].Addr;
                *pFeatureData32++=HM3451SensorCCT[i].Para;
            }
        break;
		case SENSOR_FEATURE_SET_ENG_REGISTER:
/*SensorRegNumber=*pFeatureData32++;
			RETAILMSG(1, (TEXT("HM3451 Sensor Reg No %x \r\n"),SensorRegNumber));
			if (SensorRegNumber>ENGINEER_END)
				return FALSE;*/
			SensorRegNumber=ENGINEER_END;
			for (i=0;i<SensorRegNumber;i++)
			{
			   spin_lock(&hm3451raw_drv_lock);
				HM3451SensorReg[i].Addr=*pFeatureData32++;
				HM3451SensorReg[i].Para=*pFeatureData32++;
			   spin_unlock(&hm3451raw_drv_lock);
			}
		break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=HM3451SensorReg[i].Addr;
                *pFeatureData32++=HM3451SensorReg[i].Para;
            }
        break;
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
			if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
			{
				pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
				pSensorDefaultData->SensorId=HM3451_SENSOR_ID;
				memcpy(pSensorDefaultData->SensorEngReg, HM3451SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
				memcpy(pSensorDefaultData->SensorCCTReg, HM3451SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
			}
			else
				return FALSE;
			*pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &HM3451SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
			break;
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            HM3451_camera_para_to_sensor();
		break;
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            HM3451_sensor_to_camera_para();
		break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=HM3451_get_sensor_group_count();
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_GROUP_INFO:
            HM3451_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
		break;
		case SENSOR_FEATURE_GET_ITEM_INFO:
            HM3451_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		break;
		case SENSOR_FEATURE_SET_ITEM_INFO:
            HM3451_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		break;
		case SENSOR_FEATURE_GET_ENG_INFO:
			pSensorEngInfo->SensorId = 280;
			pSensorEngInfo->SensorType = CMOS_SENSOR;
			pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_Gb;
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
		break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		       HM3451SetVideoMode(*pFeatureData16);
		       break; 
		case SENSOR_FEATURE_SET_CALIBRATION_DATA:
			SENSORDB("[HM3451]feature control:set_calibration_data!\n");
			//HM3451SetCalData(pSetSensorCalData);
			break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			HM3451Getsensorid(pFeatureReturnPara32);
			break;
		default:
			break;
	}
	return ERROR_NONE;
}	/* HM3451FeatureControl() */

SENSOR_FUNCTION_STRUCT    SensorFuncHM3451=
{
  HM3451Open,
  HM3451GetInfo,
  HM3451GetResolution,
  HM3451FeatureControl,
  HM3451Control,
  HM3451Close
};

UINT32 HM3451SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{

  /* To Do : Check Sensor status here */
  if (pfFunc!=NULL)
      *pfFunc=&SensorFuncHM3451;

  return ERROR_NONE;
} /* SensorInit() */


