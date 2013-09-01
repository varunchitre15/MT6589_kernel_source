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
 * 02 19 2012 koli.lin
 * [ALPS00237113] [Performance][Video recording]Recording preview the screen have flash
 * [Camera] 1. Modify the AE converge speed in the video mode.
 *                2. Modify the isp gain delay frame with sensor exposure time and gain synchronization.
 *
 * 10 12 2010 koli.lin
 * [ALPS00127101] [Camera] AE will flash
 * [Camera]Create Vsync interrupt to handle the exposure time, sensor gain and raw gain control.
 *
 * 09 10 2010 jackie.su
 * [ALPS00002279] [Need Patch] [Volunteer Patch] ALPS.Wxx.xx Volunteer patch for
 * .alps dual sensor
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
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "mt9p012_Sensor.h"
#include "mt9p012_Camera_Sensor_para.h"
#include "mt9p012_CameraCustomized.h"


#define MT9P012_DEBUG
#ifdef MT9P012_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define MT9P012_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para ,1,MT9P012_WRITE_ID)
#define MT9P012_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,MT9P012_WRITE_ID)
#define MT9P012_write_cmos_sensor_16Bit(addr, para) iWriteReg((u16) addr , (u32) para ,2,MT9P012_WRITE_ID)
kal_uint16 MT9P012_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,MT9P012_WRITE_ID);
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
//
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

#define   MT9P012_LIMIT_EXPOSURE_LINES                (1253)
#define   MT9P012_VIDEO_NORMALMODE_30FRAME_RATE       (30)
#define   MT9P012_VIDEO_NORMALMODE_FRAME_RATE         (15)
//#define   MT9P012_VIDEO_NIGHTMODE_FRAME_RATE          (7.5)

/* Global Valuable */
kal_uint8  MT9P012_start_grab_x_offset=0, MT9P012_start_grab_y_offset=0;
kal_bool   MT9P012_sensor_night_mode=KAL_FALSE, MT9P012_MPEG4_encode_mode=KAL_FALSE;
kal_uint16 MT9P012_PV_dummy_pixels=0, MT9P012_PV_dummy_lines=0, MT9P012_FULL_dummy_pixels=0, MT9P012_FULL_dummy_lines=0;
kal_uint16 MT9P012_ActiveDummyLines=PV_PERIOD_LINE_NUMS;
kal_uint16 MT9P012_exposure_lines=0;
kal_uint16 MT9P012_sensor_global_gain=BASEGAIN, MT9P012_sensor_gain_base=BASEGAIN;
kal_uint16 MT9P012_sensor_gain_array[4][4]={{0x0208, 0x0206, 0x020C, 0x020A},{0, 0, 0, 0},{0x0209, 0x0207, 0x020D, 0x020B},{0x18, 0x10, 0x10, 0x18}};

kal_uint32 MT9P012_isp_master_clock;
kal_uint32 MT9P012_sensor_pclk=48000000;
kal_uint16 MT9P012_sensor_expo_width=1,MT9P012_current_sensor_expo_width=1;
/* Max/Min Explosure Lines Used By AE Algorithm */
kal_uint16 MT9P012_max_exposure_lines=MT9P012_LIMIT_EXPOSURE_LINES;
kal_uint16 MT9P012_sensor_flip_value;
/* Parameter For Engineer mode function */
kal_uint32 MT9P012_FAC_SENSOR_REG;
//kal_uint16 MT9P012_sensor_flip_value;

kal_uint8 MT9P012_sensor_write_I2C_address = MT9P012_WRITE_ID;
kal_uint8 MT9P012_sensor_read_I2C_address = MT9P012_READ_ID;

//HANDLE MT9P012hDrvI2C;
//I2C_TRANSACTION MT9P012I2CConfig;
UINT8 MT9P012_PixelClockDivider=0;

SENSOR_REG_STRUCT MT9P012SensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT MT9P012SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
//	camera_para.SENSOR.cct	SensorCCT	=> SensorCCT
//	camera_para.SENSOR.reg	SensorReg
MSDK_SENSOR_CONFIG_STRUCT MT9P012SensorConfigData;


//void MT9P012_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
//{
//  MT9P012I2CConfig.operation=I2C_OP_WRITE;
//  MT9P012I2CConfig.slaveAddr=MT9P012_sensor_write_I2C_address>>1;
//  MT9P012I2CConfig.transfer_num=1;    /* TRANSAC_LEN */
//  MT9P012I2CConfig.transfer_len=3;
//  MT9P012I2CConfig.buffer[0]=(UINT8)(addr>>8);
//  MT9P012I2CConfig.buffer[1]=(UINT8)(addr&0xFF);
//  MT9P012I2CConfig.buffer[2]=(UINT8)para;
//  DRV_I2CTransaction(MT9P012hDrvI2C, &MT9P012I2CConfig);
//} /* MT9P012_write_cmos_sensor() */

//kal_uint32 MT9P012_read_cmos_sensor(kal_uint32 addr)
//{
//  kal_uint16 get_byte=0;
//  MT9P012I2CConfig.operation=I2C_OP_WRITE;
//  MT9P012I2CConfig.slaveAddr=MT9P012_sensor_write_I2C_address>>1;
//  MT9P012I2CConfig.transfer_num=1;    /* TRANSAC_LEN */
//  MT9P012I2CConfig.transfer_len=2;
//  MT9P012I2CConfig.buffer[0]=(UINT8)(addr>>8);
//  MT9P012I2CConfig.buffer[1]=(UINT8)(addr&0xFF);
//  DRV_I2CTransaction(MT9P012hDrvI2C, &MT9P012I2CConfig);
//
//  MT9P012I2CConfig.operation=I2C_OP_READ;
//  MT9P012I2CConfig.slaveAddr=MT9P012_sensor_read_I2C_address>>1;
//  MT9P012I2CConfig.transfer_num=1;    /* TRANSAC_LEN */
//  MT9P012I2CConfig.transfer_len=1;
//  DRV_I2CTransaction(MT9P012hDrvI2C, &MT9P012I2CConfig);
//  get_byte=MT9P012I2CConfig.buffer[0];
//  return get_byte;
//} /* MT9P012_read_cmos_sensor() */

void write_MT9P012_gain(kal_uint16 gain)
{
  volatile signed char i;
  kal_uint16 temp_reg=0, temp_gain=0;
  kal_uint16 temp_min_gain = gain;

  MT9P012_write_cmos_sensor(0x0104, 0x01);        //parameter_hold
  for(i=0;i<4;i++)
  {
      temp_gain=(MT9P012_sensor_gain_array[3][i]*gain)/BASEGAIN;
      /* prevent init value error of temp_min_gain > 2 */
      if (i==0)
      {
          temp_min_gain = temp_gain;
      }

      if(temp_gain>=1*BASEGAIN && temp_gain<15*BASEGAIN)
      {
          temp_reg=(8*temp_gain)/BASEGAIN;
          if(temp_gain<=temp_min_gain)
          {
              MT9P012_sensor_global_gain=temp_gain&(~0x1);
              temp_min_gain = temp_gain;
          }
      }
      else
      {
          temp_reg = 0x7F;
          if(temp_gain<=temp_min_gain)
          {
              //MT9P012_sensor_global_gain=15.875*BASEGAIN;
              MT9P012_sensor_global_gain=(16256*BASEGAIN) >> 10;  /* 15.875 * BASEGAIN */
              temp_min_gain = temp_gain;
          }
      }
      MT9P012_write_cmos_sensor(MT9P012_sensor_gain_array[2][i],temp_reg);
  }
  MT9P012_write_cmos_sensor(0x0104, 0x00);        //parameter_hold
}   /* write_MT9P012_gain */

kal_uint16 read_MT9P012_gain(void)
{
   volatile signed char i;
   kal_uint16 temp_reg=0, sensor_gain=0,temp_reg_base=0;

   for(i=0;i<4;i++)
   {
      temp_reg_base=MT9P012SensorCCT[SENSOR_BASEGAIN].Para;
      temp_reg=MT9P012SensorCCT[PRE_GAIN_R_INDEX+i].Para;

      if(temp_reg>=0x08 && temp_reg<=0xFF)
         MT9P012_sensor_gain_array[3][i]=((((temp_reg*BASEGAIN)/8)*temp_reg_base)/8);
      else if(temp_reg>0xFF && temp_reg<=0x1FF)
         MT9P012_sensor_gain_array[3][i]=((((temp_reg*BASEGAIN)/8)*temp_reg_base)/8);
      else
         ASSERT(0);
   }

   sensor_gain=(temp_reg_base*BASEGAIN)/8;

   return sensor_gain;
}  /* read_MT9P012_gain */

// write camera_para to sensor register
void MT9P012_camera_para_to_sensor(void)
{
  kal_uint32  i;
  for(i=0; 0xFFFFFFFF!=MT9P012SensorReg[i].Addr; i++)
  {
      MT9P012_write_cmos_sensor(MT9P012SensorReg[i].Addr, MT9P012SensorReg[i].Para);
  }
  for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=MT9P012SensorReg[i].Addr; i++)
  {
      MT9P012_write_cmos_sensor(MT9P012SensorReg[i].Addr, MT9P012SensorReg[i].Para);
  }
  for(i=FACTORY_START_ADDR+1; i<5; i++)
  {
      MT9P012_write_cmos_sensor(MT9P012SensorCCT[i].Addr, MT9P012SensorCCT[i].Para);
  }
}

// update camera_para from sensor register
void MT9P012_sensor_to_camera_para(void)
{
  kal_uint32  i;
  for(i=0; 0xFFFFFFFF!=MT9P012SensorReg[i].Addr; i++)
  {
      MT9P012SensorReg[i].Para = MT9P012_read_cmos_sensor(MT9P012SensorReg[i].Addr);
  }
  for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=MT9P012SensorReg[i].Addr; i++)
  {
      MT9P012SensorReg[i].Para = MT9P012_read_cmos_sensor(MT9P012SensorReg[i].Addr);
  }
}

//------------------------Engineer mode---------------------------------
kal_int32  MT9P012_get_sensor_group_count(void)
{
  return GROUP_TOTAL_NUMS;
}

void MT9P012_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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
         ASSERT(0);
  }
}

void MT9P012_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
  kal_int16 temp_reg=0;
  kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;

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
                 ASSERT(0);
          }

          temp_para=MT9P012SensorCCT[temp_addr].Para;

          if(temp_para>=0x08 && temp_para<=0xFF)
              temp_gain=(temp_para*BASEGAIN)/8;
          else if(temp_para>0xFF && temp_para<=0x1FF)
              temp_gain=(temp_para*BASEGAIN)/8;
          else
              ASSERT(0);

          temp_gain=(temp_gain*1000)/BASEGAIN;

          info_ptr->ItemValue=temp_gain;
                info_ptr->IsTrueFalse=KAL_FALSE;
                info_ptr->IsReadOnly=KAL_FALSE;
                info_ptr->IsNeedRestart=KAL_FALSE;
                info_ptr->Min=1000;
                info_ptr->Max=15875;
              break;
      case CMMCLK_CURRENT:
          switch (item_idx)
          {
              case 0:
                sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");

                temp_reg=MT9P012SensorReg[CMMCLK_CURRENT_INDEX].Para;
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
      case FRAME_RATE_LIMITATION:
          switch (item_idx)
          {
              case 0:
                sprintf((char *)info_ptr->ItemNamePtr,"Max Exposure Lines");
                info_ptr->ItemValue=MT9P012_max_exposure_lines;
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


#if 0 
void MT9P012_set_isp_driving_current(kal_uint32 current)
{
}
#endif 

kal_bool MT9P012_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
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
                 ASSERT(0);
          }

          temp_gain=((ItemValue*BASEGAIN+500)/1000);          //+500:get closed integer value

          if(temp_gain>=1*BASEGAIN && temp_gain<=15*BASEGAIN)
          {
             temp_para=(temp_gain*8+BASEGAIN/2)/BASEGAIN;
          }
          else if(temp_gain>=16*BASEGAIN)
          {
             temp_para=(kal_uint16)(15.875*BASEGAIN*8+BASEGAIN/2)/BASEGAIN;
          }
          else
              return KAL_FALSE;

          MT9P012SensorCCT[temp_addr].Para = temp_para;
          if(temp_addr != SENSOR_BASEGAIN)
          {
              MT9P012_write_cmos_sensor(MT9P012SensorCCT[temp_addr].Addr,temp_para);
          }

          MT9P012_sensor_gain_base=read_MT9P012_gain();
          if(temp_addr == SENSOR_BASEGAIN)
          {
              write_MT9P012_gain(BASEGAIN);
          }
      break;
      case CMMCLK_CURRENT:
          switch (item_idx)
          {
              case 0:
                if(ItemValue==2)
                {
                    MT9P012SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
                    //MT9P012_set_isp_driving_current((kal_uint32)ISP_DRIVING_2MA);
                }
                else if(ItemValue==3 || ItemValue==4)
                {
                    MT9P012SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
                    //MT9P012_set_isp_driving_current((kal_uint32)ISP_DRIVING_4MA);
                }
                else if(ItemValue==5 || ItemValue==6)
                {
                    MT9P012SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
                   // MT9P012_set_isp_driving_current((kal_uint32)ISP_DRIVING_6MA);
                }
                else
                {
                    MT9P012SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
                    //MT9P012_set_isp_driving_current((kal_uint32)ISP_DRIVING_8MA);
                }
              break;
              default:
                 ASSERT(0);
          }
      break;
      case FRAME_RATE_LIMITATION:
         ASSERT(0);
      break;
      case REGISTER_EDITOR:
          switch (item_idx)
          {
              case 0:
                MT9P012_FAC_SENSOR_REG=ItemValue;
              break;
              case 1:
                MT9P012_write_cmos_sensor(MT9P012_FAC_SENSOR_REG,ItemValue);
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

/*************************************************************************
* FUNCTION
* set_MT9P012_shutter
*
* DESCRIPTION
* This function set e-shutter of MT9P012 to change exposure time.
*
* PARAMETERS
* shutter : exposured lines
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void set_MT9P012_shutter(kal_uint16 shutter)
{
  MT9P012_exposure_lines=shutter;
  MT9P012_write_cmos_sensor(0x0104, 0x01);        //parameter_hold
  if(MT9P012_MPEG4_encode_mode==KAL_TRUE)
  {  
      MT9P012_write_cmos_sensor(0x0202,shutter>>8);
      MT9P012_write_cmos_sensor(0x0203,shutter&0xFF);
      MT9P012_write_cmos_sensor(0x0104, 0x00);        //parameter_hold
  }
  else
  {
      if (shutter > MT9P012_ActiveDummyLines)
      {
          MT9P012_write_cmos_sensor(0x0340, shutter>>8);            //frame_Length_lines
          MT9P012_write_cmos_sensor(0x0341, shutter&0xFF);
      }
      else
      {
          MT9P012_write_cmos_sensor(0x0340, MT9P012_ActiveDummyLines>>8);            //frame_Length_lines
          MT9P012_write_cmos_sensor(0x0341, MT9P012_ActiveDummyLines&0xFF);
      }

      MT9P012_write_cmos_sensor(0x0202,shutter>>8);
      MT9P012_write_cmos_sensor(0x0203,shutter&0xFF);
      MT9P012_write_cmos_sensor(0x0104, 0x00);        //parameter_hold
  }
} /* set_MT9P012_shutter */

/*************************************************************************
* FUNCTION
* set_MT9P012_gain
*
* DESCRIPTION
* This function is to set global gain to sensor.
*
* PARAMETERS
* gain : sensor global gain(base: 0x40)
*
* RETURNS
* the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 set_MT9P012_gain(kal_uint16 gain)
{
  kal_uint32 reg_gain;

      write_MT9P012_gain(gain);
      reg_gain=(MT9P012_sensor_global_gain*BASEGAIN)/MT9P012_sensor_gain_base;

      return reg_gain;
}

/*************************************************************************
* FUNCTION
* MT9P012_night_mode
*
* DESCRIPTION
* This function night mode of MT9P012.
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
void MT9P012_night_mode(kal_bool enable)
{
  kal_uint16 dummy = IMAGE_SENSOR_PV_VBLANKING+MT9P012_PV_dummy_lines;   
  if(enable)
  {
      if(MT9P012_MPEG4_encode_mode==KAL_TRUE)
      {
          MT9P012_max_exposure_lines = (kal_uint16)((MT9P012_sensor_pclk/MT9P012_VIDEO_NORMALMODE_FRAME_RATE)/(PV_PERIOD_PIXEL_NUMS+MT9P012_PV_dummy_pixels));

          if((MT9P012_max_exposure_lines>PV_ACTIVE_LINE_NUMS)&&(dummy<(MT9P012_max_exposure_lines-PV_ACTIVE_LINE_NUMS)))
          {
              dummy = MT9P012_max_exposure_lines - PV_ACTIVE_LINE_NUMS;
          }
          MT9P012_write_cmos_sensor(0x0104, 0x01);                            //Grouped parameter hold
          MT9P012_write_cmos_sensor(0x0340, (PV_ACTIVE_LINE_NUMS+dummy)>>8);  //frame_Length_lines
          MT9P012_write_cmos_sensor(0x0341, (PV_ACTIVE_LINE_NUMS+dummy)&0xFF);
          MT9P012_write_cmos_sensor(0x0104, 0x00);                            //Grouped parameter hold
      }
      MT9P012_sensor_night_mode=KAL_TRUE;
  }
  else
  {
      if(MT9P012_MPEG4_encode_mode==KAL_TRUE)
      {
          MT9P012_max_exposure_lines = (kal_uint16)((MT9P012_sensor_pclk/MT9P012_VIDEO_NORMALMODE_30FRAME_RATE)/(PV_PERIOD_PIXEL_NUMS+MT9P012_PV_dummy_pixels));

          if((MT9P012_max_exposure_lines>PV_ACTIVE_LINE_NUMS)&&(dummy<(MT9P012_max_exposure_lines-PV_ACTIVE_LINE_NUMS)))
          {
              dummy = MT9P012_max_exposure_lines - PV_ACTIVE_LINE_NUMS;
          }
          MT9P012_write_cmos_sensor(0x0104, 0x01);                            //Grouped parameter hold
          MT9P012_write_cmos_sensor(0x0340, (PV_ACTIVE_LINE_NUMS+dummy)>>8);  //frame_Length_lines
          MT9P012_write_cmos_sensor(0x0341, (PV_ACTIVE_LINE_NUMS+dummy)&0xFF);
          MT9P012_write_cmos_sensor(0x0104, 0x00);                            //Grouped parameter hold
      }
      MT9P012_sensor_night_mode=KAL_FALSE;
  }
} /* MT9P012_night_mode */

void Config_MT9P012_Window(kal_uint16 startX,kal_uint16 endX,kal_uint16 startY,kal_uint16 endY,kal_uint16 X_size,kal_uint16 Y_size,kal_uint16 scaler)
{
  kal_uint16 Frame_Length_lines, Line_Length_pck;
  if (scaler != 1)
  {
      Frame_Length_lines = PV_PERIOD_LINE_NUMS;
      Line_Length_pck = PV_PERIOD_PIXEL_NUMS*2;//the period pixel number is double in scope
      //MT9P012_write_cmos_sensor(0x0382, scaler>>8);           //X_Odd_INC
      //MT9P012_write_cmos_sensor(0x0383, scaler&0xFF);
       MT9P012_write_cmos_sensor_2(0x0382, scaler, 2);   
      //MT9P012_write_cmos_sensor(0x0386, scaler>>8);           //Y_Odd_INC
      //MT9P012_write_cmos_sensor(0x0387, scaler&0xFF);
      MT9P012_write_cmos_sensor_2(0x0386, scaler, 2);         
  }
  else
  {
      Frame_Length_lines = FULL_PERIOD_LINE_NUMS;//(endY-startY)+IMAGE_SENSOR_FULL_VBLANKING;
      Line_Length_pck = FULL_PERIOD_PIXEL_NUMS*2;//(endX-startX)+IMAGE_SENSOR_FULL_HBLANKING;
      //MT9P012_write_cmos_sensor(0x0382, scaler>>8);           //X_Odd_INC
      //MT9P012_write_cmos_sensor(0x0383, scaler&0xFF);
      MT9P012_write_cmos_sensor_2(0x0382, scaler, 2);   
      
      //MT9P012_write_cmos_sensor(0x0386, scaler>>8);           //Y_Odd_INC
      //MT9P012_write_cmos_sensor(0x0387, scaler&0xFF);
      MT9P012_write_cmos_sensor_2(0x0386, scaler, 2);   
  }

  //MT9P012_write_cmos_sensor(0x0340, Frame_Length_lines>>8);   //frame_Length_lines
  //MT9P012_write_cmos_sensor(0x0341, Frame_Length_lines&0xFF);  
  MT9P012_write_cmos_sensor_2(0x0340, Frame_Length_lines, 2);   
  //MT9P012_write_cmos_sensor(0x0342, Line_Length_pck>>8);      //line_Length_pck
  //MT9P012_write_cmos_sensor(0x0343, Line_Length_pck&0xFF);
  MT9P012_write_cmos_sensor_2(0x0342, Line_Length_pck, 2); 
  
  //MT9P012_write_cmos_sensor(0x0344, startX>>8);               //X_start
  //MT9P012_write_cmos_sensor(0x0345, startX&0xFF);
  MT9P012_write_cmos_sensor_2(0x0344, startX, 2); 
  //MT9P012_write_cmos_sensor(0x0346, startY>>8);               //Y_start
  //MT9P012_write_cmos_sensor(0x0347, startY&0xFF);
  MT9P012_write_cmos_sensor_2(0x0346, startY, 2); 

  //MT9P012_write_cmos_sensor(0x0348, endX>>8);                 //X_end
  //MT9P012_write_cmos_sensor(0x0349, endX&0xFF);
  MT9P012_write_cmos_sensor_2(0x0348, endX, 2); 
  //MT9P012_write_cmos_sensor(0x034A, endY>>8);                 //Y_end
  //MT9P012_write_cmos_sensor(0x034B, endY&0xFF);
  MT9P012_write_cmos_sensor_2(0x034A, endY, 2); 

  //MT9P012_write_cmos_sensor(0x034C, X_size>>8);          //X_size
  //MT9P012_write_cmos_sensor(0x034D, X_size&0xFF);
  MT9P012_write_cmos_sensor_2(0x034C, X_size, 2);   
  //MT9P012_write_cmos_sensor(0x034E, Y_size>>8);          //Y_size
  //MT9P012_write_cmos_sensor(0x034F, Y_size&0xFF);
  MT9P012_write_cmos_sensor_2(0x034E, Y_size, 2);   
} /*  Config_MT9P012_Window */


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
* MT9P012Open
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
UINT32 MT9P012Open(void)
{
  volatile signed char i;
  kal_uint16 sensor_id=0;

//  MT9P012hDrvI2C=DRV_I2COpen(0);
//  DRV_I2CSetParam(MT9P012hDrvI2C, I2C_VAL_FS_SAMPLE_DIV, 3);
//  DRV_I2CSetParam(MT9P012hDrvI2C, I2C_VAL_FS_STEP_DIV, 8);
//  DRV_I2CSetParam(MT9P012hDrvI2C, I2C_VAL_DELAY_LEN, 2);
//  DRV_I2CSetParam(MT9P012hDrvI2C, I2C_VAL_CLK_EXT, I2C_CLKEXT_DISABLE);
//  MT9P012I2CConfig.trans_mode=I2C_TRANSFER_FAST_MODE;
//  MT9P012I2CConfig.slaveAddr=MT9P012_sensor_write_I2C_address>>1;
//  MT9P012I2CConfig.RS_ST=I2C_TRANSFER_STOP;   /* for fast mode */

//    CISModulePowerOn(KAL_TRUE);      // Power On CIS Power
  //Sleep(10);                          // To wait for Stable Power
  mdelay(10); 
  MT9P012_write_cmos_sensor(0x0103, 0x01);    // Reset sensor
  mdelay(5); 
  //mdelay(50); 
  //mdelay(33); 
  //Sleep(1000);
  //Sleep(50);

  //  Read sensor ID to adjust I2C is OK?
  for(i=0;i<3;i++)
  {
      sensor_id=(MT9P012_read_cmos_sensor(0x0000)<<8)|MT9P012_read_cmos_sensor(0x0001);
      if(sensor_id == MT9P012_SENSOR_ID || sensor_id == MT9P012_SENSOR_ID_REV7)
      {
          break;
      }
  }
  if(sensor_id != MT9P012_SENSOR_ID && sensor_id != MT9P012_SENSOR_ID_REV7)
  {
      printk("Read Sesnor Fail \n"); 
      MT9P012_sensor_write_I2C_address = MT9P012_WRITE_ID_1;
      MT9P012_sensor_read_I2C_address = MT9P012_READ_ID_1;
      // Reset sensor
      MT9P012_write_cmos_sensor(0x0103, 0x01);
      //Sleep(50);
      Sleep(1000);

      for(i=0;i<3;i++)
      {
          sensor_id=(MT9P012_read_cmos_sensor(0x0000)<<8)|MT9P012_read_cmos_sensor(0x0001);
          if(sensor_id == MT9P012_SENSOR_ID || sensor_id == MT9P012_SENSOR_ID_REV7)
              break;
      }
      if(sensor_id != MT9P012_SENSOR_ID && sensor_id != MT9P012_SENSOR_ID_REV7)
          return ERROR_SENSOR_CONNECT_FAIL;
  }
  RETAILMSG(1, (TEXT("MT9P012 Sensor Read ID OK \r\n")));
  //MT9P012_write_cmos_sensor(0x301A, 0x10);        //RESET_REGISTER, enable parallel interface and disable serialiser
  //MT9P012_write_cmos_sensor(0x301B, 0xCC);
  MT9P012_write_cmos_sensor_2(0x301A, 0x10C8, 2); 

  //sensor PLL settng: 52MHz input-->48MHz output
  MT9P012_write_cmos_sensor(0x0100, 0x00);        //MODE select (Stop streaming)
  //Sleep(50);
  //Sleep(1000);
  //Hold //sean add 
  MT9P012_write_cmos_sensor(0x0104, 0x01);    //parameter_hold  
  //48MHz
  //MT9P012_write_cmos_sensor(0x0300, 0x00);
  //MT9P012_write_cmos_sensor(0x0301, 0x04);
  MT9P012_write_cmos_sensor_2(0x0300, 0x0004, 2); 
  //MT9P012_write_cmos_sensor(0x0302, 0x00);
  //MT9P012_write_cmos_sensor(0x0303, 0x02);
  MT9P012_write_cmos_sensor_2(0x0302, 0x0002, 2); 
  //MT9P012_write_cmos_sensor(0x0304, 0x00);
  //MT9P012_write_cmos_sensor(0x0305, 0x0D);
  MT9P012_write_cmos_sensor_2(0x0304, 0x000D, 2); 
  //MT9P012_write_cmos_sensor(0x0306, 0x00);
  //MT9P012_write_cmos_sensor(0x0307, 0xC0);
  MT9P012_write_cmos_sensor_2(0x0306, 0x00C0, 2); 
  //MT9P012_write_cmos_sensor(0x0308, 0x00);
  //MT9P012_write_cmos_sensor(0x0309, 0x08);
  MT9P012_write_cmos_sensor_2(0x0308, 0x0008, 2); 
  //MT9P012_write_cmos_sensor(0x030A, 0x00);
  //MT9P012_write_cmos_sensor(0x030B, 0x02);
  MT9P012_write_cmos_sensor_2(0x030A, 0x0002, 2); 
  MT9P012_write_cmos_sensor_2(0x301A, 0x10CC, 2);   
  MT9P012_write_cmos_sensor(0x0104, 0x00);    //parameter_hold  
//  MT9P012_write_cmos_sensor(0x0100, 0x01);        //MODE select (Stop streaming)
  //Sleep(50);
  //Sleep(1000);

  // power saving
  MT9P012_write_cmos_sensor(0x0104, 0x01);    //parameter_hold

  //MT9P012_write_cmos_sensor(0x30B0, 0x00);
  //MT9P012_write_cmos_sensor(0x30B1, 0x01);
  MT9P012_write_cmos_sensor_2(0x30B0, 0x0001, 2); 
  //MT9P012_write_cmos_sensor(0x3088, 0x6E);
  //MT9P012_write_cmos_sensor(0x3089, 0x61);
  MT9P012_write_cmos_sensor_2(0x3088, 0x6E61, 2); 
  //MT9P012_write_cmos_sensor(0x308E, 0xE0);
  //MT9P012_write_cmos_sensor(0x308F, 0x60);
  MT9P012_write_cmos_sensor_2(0x308E, 0xE060, 2); 
  //MT9P012_write_cmos_sensor(0x3084, 0x24);
  //MT9P012_write_cmos_sensor(0x3085, 0x24);
  MT9P012_write_cmos_sensor_2(0x3084, 0x2424, 2); 
  //MT9P012_write_cmos_sensor(0x3092, 0x0A);
  //MT9P012_write_cmos_sensor(0x3093, 0x52);
  MT9P012_write_cmos_sensor_2(0x3092, 0x0A52, 2); 
  //MT9P012_write_cmos_sensor(0x3094, 0x46);
  //MT9P012_write_cmos_sensor(0x3095, 0x56);
  MT9P012_write_cmos_sensor_2(0x3094, 0x4656, 2); 
  //MT9P012_write_cmos_sensor(0x3096, 0x56);
  //MT9P012_write_cmos_sensor(0x3097, 0x52);
  MT9P012_write_cmos_sensor_2(0x3096, 0x5652, 2); 
  //MT9P012_write_cmos_sensor(0x30CA, 0x80);
  //MT9P012_write_cmos_sensor(0x30CB, 0x06);
  MT9P012_write_cmos_sensor_2(0x30CA, 0x8006, 2); 
  //MT9P012_write_cmos_sensor(0x312A, 0xDD);
  //MT9P012_write_cmos_sensor(0x312B, 0x02);
  MT9P012_write_cmos_sensor_2(0x312A, 0xDD02, 2); 
  //MT9P012_write_cmos_sensor(0x312C, 0x00);
  //MT9P012_write_cmos_sensor(0x312D, 0xE4);
  MT9P012_write_cmos_sensor_2(0x312C, 0x00E4, 2); 
  //MT9P012_write_cmos_sensor(0x3170, 0x29);
  //MT9P012_write_cmos_sensor(0x3171, 0x9A);
  MT9P012_write_cmos_sensor_2(0x3170, 0x299A, 2); 

  MT9P012_write_cmos_sensor(0x0104, 0x00);    //parameter_hold

  MT9P012_sensor_gain_base=read_MT9P012_gain();
  return ERROR_NONE;
} /* MT9P012Open() */

/*************************************************************************
* FUNCTION
* MT9P012Close
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
UINT32 MT9P012Close(void)
{
//    CISModulePowerOn(FALSE);

//  DRV_I2CClose(MT9P012hDrvI2C);
  MT9P012_write_cmos_sensor(0x0100, 0x00);        //MODE select (Stop streaming)
  MT9P012_write_cmos_sensor(0x0103, 0x01);    // Reset sensor
  return ERROR_NONE;
} /* MT9P012Close() */

/*************************************************************************
* FUNCTION
* MT9P012Preview
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
UINT32 MT9P012Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
  kal_uint32 backup_PV_dummy_lines;
  MT9P012_start_grab_x_offset=0;
  MT9P012_start_grab_y_offset=0;

  MT9P012_current_sensor_expo_width=MT9P012_sensor_expo_width;    // No TV out
  /* config TG of ISP to match the setting of image sensor*/
  if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)       // MPEG4 Encode Mode
  {
      MT9P012_MPEG4_encode_mode = KAL_TRUE;
  }
  else
  {
      MT9P012_MPEG4_encode_mode = KAL_FALSE;
  }

  MT9P012_PV_dummy_pixels = 0;//220;
  MT9P012_PV_dummy_lines = 0;

  /* cal MAX Exposure line for limit min frame rate */
  //MT9P012_sensor_pclk =104000000/(MT9P012_PixelClockDivider+1)*2;
  MT9P012_sensor_pclk =48000000;
  MT9P012_write_cmos_sensor(0x0104, 0x01);        //parameter_hold

  switch (sensor_config_data->SensorImageMirror)
  {
      case IMAGE_NORMAL:
          MT9P012_write_cmos_sensor(0x301D,0x00);
          MT9P012_sensor_flip_value = 0;
          MT9P012_start_grab_x_offset = 0;
          MT9P012_start_grab_y_offset = 0;
      break;
      case IMAGE_H_MIRROR:
          MT9P012_write_cmos_sensor(0x301D,0x01);
          MT9P012_sensor_flip_value = 2;
          MT9P012_start_grab_x_offset = 1;
          MT9P012_start_grab_y_offset = 0;
      break;
      case IMAGE_V_MIRROR:
          MT9P012_write_cmos_sensor(0x301D,0x02);
          MT9P012_sensor_flip_value = 1;
          MT9P012_start_grab_x_offset = 0;
          MT9P012_start_grab_y_offset = 1;
      break;
      case IMAGE_HV_MIRROR:
          MT9P012_write_cmos_sensor(0x301D,0x03);
          MT9P012_sensor_flip_value = 3;
          MT9P012_start_grab_x_offset = 1;
          MT9P012_start_grab_y_offset = 1;
      break;
  }

  //MT9P012_write_cmos_sensor(0x301A, 0x10);        //Enable SMIA clock
  //MT9P012_write_cmos_sensor(0x301B, 0xCC);
  //MT9P012_write_cmos_sensor_2(0x301A, 0x10CC, 2); 

  //MT9P012_write_cmos_sensor(0x3088, 0x6F);//dac_ld_8_9:[0:3]ana_sreg_vdac_vtx_lo,[4:7]ana_sreg_vdac_vtx_hi
  //MT9P012_write_cmos_sensor(0x3089, 0xF6);//[8:11]ana_sreg_vdac_vrst_lo,[12:15]ana_sreg_vdac_vrst_hi
  MT9P012_write_cmos_sensor_2(0x3088, 0x6FF6, 2); 
  //MT9P012_write_cmos_sensor(0x3154, 0x02);//global boost:[0:7]global_boost_lower_edge,[8:15]global_boost_upper_edge
  //MT9P012_write_cmos_sensor(0x3155, 0x82);
  MT9P012_write_cmos_sensor_2(0x3154, 0x0282, 2); 
  //MT9P012_write_cmos_sensor(0x3156, 0x03);//global done:[0:7]global_done_lower_edge,[8:15]global_done_upper_edge
  //MT9P012_write_cmos_sensor(0x3157, 0x81);
  MT9P012_write_cmos_sensor_2(0x3156, 0x0381, 2); 
  //MT9P012_write_cmos_sensor(0x3162, 0x04);//global rst end
  //MT9P012_write_cmos_sensor(0x3163, 0xCE);
  MT9P012_write_cmos_sensor_2(0x3162, 0x04CE, 2); 

  //MT9P012_write_cmos_sensor(0x0104, 0x01);        //parameter_hold

//s_porting add
//  Config_MT9P012_Window(PV_X_START,PV_X_END,PV_Y_START,PV_Y_END,1296,972,PV_SCALER_FACTOR);
  Config_MT9P012_Window(PV_X_START,PV_X_END,PV_Y_START,PV_Y_END,1288,970,PV_SCALER_FACTOR);
//e_porting add

  //MT9P012_write_cmos_sensor(0x0400, 0x00);        //scaling mode
  //MT9P012_write_cmos_sensor(0x0401, 0x00);
  //MT9P012_write_cmos_sensor_2(0x0400, 0x0000, 2); 
  //MT9P012_write_cmos_sensor(0x0404, 0x00);        //scale_M
  //MT9P012_write_cmos_sensor(0x0405, 0x10);
  //MT9P012_write_cmos_sensor_2(0x0404, 0x0010, 2); 

  //MT9P012_write_cmos_sensor(0x0104, 0x00);        //parameter_hold check 20080408
  //MT9P012_write_cmos_sensor(0x0100, 0x01);        //mode_select (Open streaming)

  /* Config Image Window */
  image_window->GrabStartX=IMAGE_SENSOR_PV_INSERTED_PIXELS+MT9P012_start_grab_x_offset;
  image_window->GrabStartY=IMAGE_SENSOR_PV_INSERTED_LINES+MT9P012_start_grab_y_offset;
  image_window->ExposureWindowWidth=IMAGE_SENSOR_PV_WIDTH;
  image_window->ExposureWindowHeight=IMAGE_SENSOR_PV_HEIGHT;

  image_window->CurrentExposurePixel=MT9P012_current_sensor_expo_width;
  image_window->ExposurePixel=MT9P012_sensor_expo_width;

  /*avoid last exit in dark and make AE exposure not enough*/
  backup_PV_dummy_lines = MT9P012_PV_dummy_lines;
  if ( MT9P012_exposure_lines > (PV_PERIOD_LINE_NUMS+MT9P012_PV_dummy_lines))
  {
      MT9P012_PV_dummy_lines = MT9P012_exposure_lines - PV_PERIOD_LINE_NUMS;
  }
  //MT9P012_write_cmos_sensor(0x0104, 0x01);                                        //Grouped parameter hold
  //MT9P012_write_cmos_sensor(0x0340, (PV_PERIOD_LINE_NUMS+MT9P012_PV_dummy_lines)>>8);     //frame_Length_lines
  //MT9P012_write_cmos_sensor(0x0341, (PV_PERIOD_LINE_NUMS+MT9P012_PV_dummy_lines)&0xFF);  
  MT9P012_write_cmos_sensor_2(0x0340, (PV_PERIOD_LINE_NUMS+MT9P012_PV_dummy_lines), 2); 
  //MT9P012_write_cmos_sensor(0x0342, ((PV_PERIOD_PIXEL_NUMS+MT9P012_PV_dummy_pixels)*2)>>8);   //line_Length_pck
  //MT9P012_write_cmos_sensor(0x0343, ((PV_PERIOD_PIXEL_NUMS+MT9P012_PV_dummy_pixels)*2)&0xFF);
  MT9P012_write_cmos_sensor_2(0x0342, ((PV_PERIOD_PIXEL_NUMS+MT9P012_PV_dummy_pixels)*2), 2); 

  MT9P012_PV_dummy_lines = backup_PV_dummy_lines;

  //MT9P012_write_cmos_sensor(0x0202,MT9P012_exposure_lines>>8);
  //MT9P012_write_cmos_sensor(0x0203,MT9P012_exposure_lines&0xFF);
  MT9P012_write_cmos_sensor_2(0x0202, MT9P012_exposure_lines, 2); 
  MT9P012_write_cmos_sensor(0x0104, 0x00);                                        //Grouped parameter hold
  MT9P012_write_cmos_sensor(0x0100, 0x01);        //mode_select (Open streaming)
  // copy sensor_config_data
  memcpy(&MT9P012SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

  MT9P012_ActiveDummyLines = PV_PERIOD_LINE_NUMS+MT9P012_PV_dummy_lines;
  return ERROR_NONE;
}	/* MT9P012Preview() */

UINT32 MT9P012Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
  volatile kal_uint16 shutter=MT9P012_exposure_lines;

  if(MT9P012_sensor_flip_value == 3)
    {
        MT9P012_start_grab_x_offset = 1;
        MT9P012_start_grab_y_offset = 3;
    }
    else if(MT9P012_sensor_flip_value == 2)
    {
        MT9P012_start_grab_x_offset = 1;
        MT9P012_start_grab_y_offset = 0;
    }
    else if(MT9P012_sensor_flip_value == 1)
    {
        MT9P012_start_grab_x_offset = 0;
        MT9P012_start_grab_y_offset = 3;
    }
    else if(MT9P012_sensor_flip_value == 0)
    {
        MT9P012_start_grab_x_offset = 0;
        MT9P012_start_grab_y_offset = 0;
    }

  MT9P012_write_cmos_sensor(0x0104, 0x01);        //parameter_hold
  Config_MT9P012_Window(FULL_X_START,FULL_X_END,FULL_Y_START,FULL_Y_END,2592,1944,FULL_SCALER_FACTOR);
  //MT9P012_write_cmos_sensor(0x0400, 0x00);        //scaling mode
  //MT9P012_write_cmos_sensor(0x0401, 0x00);
  //MT9P012_write_cmos_sensor(0x0404, 0x00);        //scale_M
  //MT9P012_write_cmos_sensor(0x0405, 0x10);
  MT9P012_write_cmos_sensor(0x0104, 0x00);        //parameter_hold
  MT9P012_write_cmos_sensor(0x0100, 0x01);        //mode_select (Open streaming)

  MT9P012_FULL_dummy_pixels = 0;
  MT9P012_FULL_dummy_lines = 0;

  shutter=(kal_uint16) ((kal_uint32)((kal_uint32)shutter*(PV_PERIOD_PIXEL_NUMS+MT9P012_PV_dummy_pixels)+((FULL_PERIOD_PIXEL_NUMS+MT9P012_FULL_dummy_pixels)>>1))/(FULL_PERIOD_PIXEL_NUMS+MT9P012_FULL_dummy_pixels));
  if (shutter > FULL_PERIOD_LINE_NUMS)
  {
      MT9P012_FULL_dummy_lines = shutter - FULL_PERIOD_LINE_NUMS;
  }
  else
  {
      MT9P012_FULL_dummy_lines = 0;
  }
  MT9P012_write_cmos_sensor(0x0104, 0x01);                                                //Grouped parameter hold
  MT9P012_write_cmos_sensor(0x0340, (FULL_PERIOD_LINE_NUMS+MT9P012_FULL_dummy_lines)>>8);         //frame_Length_lines
  MT9P012_write_cmos_sensor(0x0341, (FULL_PERIOD_LINE_NUMS+MT9P012_FULL_dummy_lines)&0xFF);
  MT9P012_write_cmos_sensor(0x0342, ((FULL_PERIOD_PIXEL_NUMS+MT9P012_FULL_dummy_pixels)*2)>>8);   //line_Length_pck
  MT9P012_write_cmos_sensor(0x0343, ((FULL_PERIOD_PIXEL_NUMS+MT9P012_FULL_dummy_pixels)*2)&0xFF);
  MT9P012_write_cmos_sensor(0x0104, 0x00);                                                //Grouped parameter hold

  image_window->GrabStartX=IMAGE_SENSOR_FULL_INSERTED_PIXELS+MT9P012_start_grab_x_offset;
  image_window->GrabStartY=IMAGE_SENSOR_FULL_INSERTED_LINES+MT9P012_start_grab_y_offset;
  image_window->ExposureWindowWidth=IMAGE_SENSOR_FULL_WIDTH;
  image_window->ExposureWindowHeight=IMAGE_SENSOR_FULL_HEIGHT;

  // config flashlight preview setting
  sensor_config_data->DefaultPclk = 52000000;
  sensor_config_data->Pixels = FULL_PERIOD_PIXEL_NUMS+MT9P012_FULL_dummy_pixels;
  sensor_config_data->FrameLines = FULL_PERIOD_LINE_NUMS+MT9P012_FULL_dummy_lines;
  sensor_config_data->Lines = image_window->ExposureWindowHeight;
  sensor_config_data->Shutter = shutter;

    if(shutter<1)
        shutter=1;

    if ( shutter >= FULL_PERIOD_LINE_NUMS+MT9P012_FULL_dummy_lines)
    { //for flashlight exposure
      MT9P012_write_cmos_sensor(0x0340, (shutter>>8));
      MT9P012_write_cmos_sensor(0x0341, (shutter&0xFF));
    }
    MT9P012_write_cmos_sensor(0x0104, 0x01);                                      //Grouped parameter hold
    MT9P012_write_cmos_sensor(0x0202,shutter>>8);
    MT9P012_write_cmos_sensor(0x0203,shutter&0xFF);
    MT9P012_write_cmos_sensor(0x0104, 0x00);                                        //Grouped parameter hold

  // copy sensor_config_data
  memcpy(&MT9P012SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

  MT9P012_ActiveDummyLines = FULL_PERIOD_LINE_NUMS+MT9P012_FULL_dummy_lines;

	return ERROR_NONE;
}	/* MT9P012Capture() */

UINT32 MT9P012GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH  - 2 * IMAGE_SENSOR_START_GRAB_X ;
	pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y;
	pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH - 2 * IMAGE_SENSOR_START_GRAB_X;
	pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y ;

	return ERROR_NONE;
}	/* MT9P012GetResolution() */

UINT32 MT9P012GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT;
	pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;
	pSensorInfo->SensorResetDelayCount=1;
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_R;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	/*??? */
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
	pSensorInfo->SensorInterruptDelayLines = 1;
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

	pSensorInfo->CaptureDelayFrame = 1; 
	pSensorInfo->PreviewDelayFrame = 2; 
	pSensorInfo->VideoDelayFrame = 5; 
	pSensorInfo->SensorMasterClockSwitch = 0; 
       pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;   	
	pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
       pSensorInfo->AESensorGainDelayFrame = 1;     /* The frame of setting sensor gain */
       pSensorInfo->AEISPGainDelayFrame = 2;	

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
#if 1
			pSensorInfo->SensorClockFreq=52;
			pSensorInfo->SensorClockDividCount=	1;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 1;
			pSensorInfo->SensorPixelClockCount= 1;
			pSensorInfo->SensorDataLatchCount= 1;
			pSensorInfo->SensorGrabStartX = 1; 
			pSensorInfo->SensorGrabStartY = 1; 
#else
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
#endif
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			pSensorInfo->SensorClockFreq=52;
			pSensorInfo->SensorClockDividCount= 1;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=1;
			pSensorInfo->SensorPixelClockCount=1;
			pSensorInfo->SensorDataLatchCount=1;
			pSensorInfo->SensorGrabStartX = 1; 
			pSensorInfo->SensorGrabStartY = 1; 			
		break;
		default:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorClockDividCount=4;
			pSensorInfo->SensorPixelClockCount=4;
			pSensorInfo->SensorDataLatchCount=2;
			pSensorInfo->SensorGrabStartX = 1; 
			pSensorInfo->SensorGrabStartY = 1; 			
		break;
	}
	MT9P012_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &MT9P012SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* MT9P012GetInfo() */


UINT32 MT9P012Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			MT9P012Preview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			MT9P012Capture(pImageWindow, pSensorConfigData);
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

UINT32 MT9P012SetVideoMode(UINT16 u2FrameRate)
{
    kal_uint16 iTemp;
    /* to fix VSYNC, to fix frame rate */
    //printk("Set Video Mode \n");     
    MT9P012_write_cmos_sensor(0x0104, 0x01);		//parameter_hold    

    if (u2FrameRate == 30)
    {
        MT9P012_write_cmos_sensor(0x0340, MT9P012_ActiveDummyLines>>8); 		   //frame_Length_lines
        MT9P012_write_cmos_sensor(0x0341, MT9P012_ActiveDummyLines&0xFF);

	 if(MT9P012_exposure_lines > MT9P012_ActiveDummyLines)
	 {
		 MT9P012_write_cmos_sensor(0x0202,MT9P012_ActiveDummyLines>>8);
		 MT9P012_write_cmos_sensor(0x0203,MT9P012_ActiveDummyLines&0xFF);
	 }
	 else
	 {
		 MT9P012_write_cmos_sensor(0x0202,MT9P012_exposure_lines>>8);
		 MT9P012_write_cmos_sensor(0x0203,MT9P012_exposure_lines&0xFF);
	 }	 
    }
    else if (u2FrameRate == 15)       
    {
        iTemp = 2*MT9P012_ActiveDummyLines;
        MT9P012_write_cmos_sensor(0x0340, iTemp>>8); 		   //frame_Length_lines
        MT9P012_write_cmos_sensor(0x0341, iTemp&0xFF);
		
	if(MT9P012_exposure_lines > iTemp)
	{
		MT9P012_write_cmos_sensor(0x0202,iTemp>>8);
		MT9P012_write_cmos_sensor(0x0203,iTemp&0xFF);
	}
	else
	{
		MT9P012_write_cmos_sensor(0x0202,MT9P012_exposure_lines>>8);
		MT9P012_write_cmos_sensor(0x0203,MT9P012_exposure_lines&0xFF);
	}	
    }
    else 
    {
        printk("Wrong frame rate setting %d\n", u2FrameRate);
    }
    MT9P012_MPEG4_encode_mode = KAL_TRUE; 

    MT9P012_write_cmos_sensor(0x0104, 0x00);		//parameter_hold
    return TRUE;
}

UINT32 MT9P012SetCalData(PSET_SENSOR_CALIBRATION_DATA_STRUCT pSetSensorCalData)
{
	UINT32 i;

	printk("MT9P012 Sensor write calibration data num = %d \r\n", pSetSensorCalData->DataSize);
	printk("MT9P012 Sensor write calibration data format = %x \r\n", pSetSensorCalData->DataFormat);	
	if(pSetSensorCalData->DataSize <= MAX_SHADING_DATA_TBL)
	{
		for (i = 0; i < pSetSensorCalData->DataSize; i++)
		{
			if (((pSetSensorCalData->DataFormat & 0xFFFF) == 1) && ((pSetSensorCalData->DataFormat >> 16) == 1))
			{
				printk("MT9P012 Sensor write calibration data: address = %x, value = %x \r\n",(pSetSensorCalData->ShadingData[i])>>16,(pSetSensorCalData->ShadingData[i])&0xFFFF);
				MT9P012_write_cmos_sensor_16Bit((pSetSensorCalData->ShadingData[i])>>16, (pSetSensorCalData->ShadingData[i])&0xFFFF);
			}
		}
	}
	return ERROR_NONE;
}

UINT32 MT9P012FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
          *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
          *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
          *pFeatureParaLen=4;
      break;
      case SENSOR_FEATURE_GET_PERIOD:
          *pFeatureReturnPara16++=PV_PERIOD_PIXEL_NUMS+MT9P012_PV_dummy_pixels;
          *pFeatureReturnPara16=PV_PERIOD_LINE_NUMS+MT9P012_PV_dummy_lines;
          *pFeatureParaLen=4;
      break;
      case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
          *pFeatureReturnPara32 = MT9P012_sensor_pclk;
          *pFeatureParaLen=4;
      break;
		case SENSOR_FEATURE_SET_ESHUTTER:
            set_MT9P012_shutter(*pFeatureData16);
		break;
      case SENSOR_FEATURE_SET_NIGHTMODE:
          MT9P012_night_mode((BOOL) *pFeatureData16);
      break;
		case SENSOR_FEATURE_SET_GAIN:
            write_MT9P012_gain((UINT16) *pFeatureData16);
		break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            MT9P012_isp_master_clock=*pFeatureData32;
		break;
		case SENSOR_FEATURE_SET_REGISTER:
            MT9P012_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = MT9P012_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
	/*		SensorRegNumber=*pFeatureData32++;
			RETAILMSG(1, (TEXT("MT9P012 Sensor Reg No %x \r\n"),SensorRegNumber));
			if (SensorRegNumber>FACTORY_END_ADDR)
				return FALSE;*/
			SensorRegNumber=FACTORY_END_ADDR;
			for (i=0;i<SensorRegNumber;i++)
			{
				MT9P012SensorCCT[i].Addr=*pFeatureData32++;
				MT9P012SensorCCT[i].Para=*pFeatureData32++;
			}
		break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=MT9P012SensorCCT[i].Addr;
                *pFeatureData32++=MT9P012SensorCCT[i].Para;
            }
        break;
		case SENSOR_FEATURE_SET_ENG_REGISTER:
/*SensorRegNumber=*pFeatureData32++;
			RETAILMSG(1, (TEXT("MT9P012 Sensor Reg No %x \r\n"),SensorRegNumber));
			if (SensorRegNumber>ENGINEER_END)
				return FALSE;*/
			SensorRegNumber=ENGINEER_END;
			for (i=0;i<SensorRegNumber;i++)
			{
				MT9P012SensorReg[i].Addr=*pFeatureData32++;
				MT9P012SensorReg[i].Para=*pFeatureData32++;
			}
		break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=MT9P012SensorReg[i].Addr;
                *pFeatureData32++=MT9P012SensorReg[i].Para;
            }
        break;
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
			if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
			{
				pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
				pSensorDefaultData->SensorId=MT9P012_SENSOR_ID;
				memcpy(pSensorDefaultData->SensorEngReg, MT9P012SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
				memcpy(pSensorDefaultData->SensorCCTReg, MT9P012SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
			}
			else
				return FALSE;
			*pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &MT9P012SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
			break;
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            MT9P012_camera_para_to_sensor();
		break;
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            MT9P012_sensor_to_camera_para();
		break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=MT9P012_get_sensor_group_count();
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_GROUP_INFO:
            MT9P012_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
		break;
		case SENSOR_FEATURE_GET_ITEM_INFO:
            MT9P012_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		break;
		case SENSOR_FEATURE_SET_ITEM_INFO:
            MT9P012_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		break;
		case SENSOR_FEATURE_GET_ENG_INFO:
			pSensorEngInfo->SensorId = 128;
			pSensorEngInfo->SensorType = CMOS_SENSOR;
			pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_R;
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
		break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		       MT9P012SetVideoMode(*pFeatureData16);
		       break; 
		case SENSOR_FEATURE_SET_CALIBRATION_DATA:
			MT9P012SetCalData(pSetSensorCalData);
			break;
		default:
			break;
	}
	return ERROR_NONE;
}	/* MT9P012FeatureControl() */

SENSOR_FUNCTION_STRUCT    SensorFuncMT9P012=
{
  MT9P012Open,
  MT9P012GetInfo,
  MT9P012GetResolution,
  MT9P012FeatureControl,
  MT9P012Control,
  MT9P012Close
};

UINT32 MT9P012SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{

  /* To Do : Check Sensor status here */
  if (pfFunc!=NULL)
      *pfFunc=&SensorFuncMT9P012;

  return ERROR_NONE;
} /* SensorInit() */


