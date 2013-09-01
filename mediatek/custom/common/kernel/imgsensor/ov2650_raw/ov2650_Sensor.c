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
 * 01 04 2012 hao.wang
 * [ALPS00109603] getsensorid func check in
 * .
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
 * 07 01 2010 sean.cheng
 * [ALPS00121215][Camera] Change color when switch low and high 
 * .Add video delay frame.
 *
 * 06 13 2010 sean.cheng
 * [ALPS00002514][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for E1k Camera 
 * .
 * 1. Add set zoom factor and capdelay frame for YUV sensor 
 * 2. Modify e1k sensor setting
 *
 * Feb 24 2010 mtk70508
 * [DUMA00154792] Sensor driver
 *
 *
 * Dec 31 2009 mtk70508
 * [DUMA00149823] check in sensor driver for new
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
 *    mtk70508
 * [DUMA00136264] Fix Video night mode framerate
 * Fix video night mode framerate
 *
 *    mtk70508
 * [DUMA00133424] [MTK Camera] There will be a darkness preview and it come out again after delete pict
 *
 *
 *    mtk70508
 * [DUMA00133424] [MTK Camera] There will be a darkness preview and it come out again after delete pict
 *
 *
 *    mtk70508
 * [DUMA00133424] [MTK Camera] There will be a darkness preview and it come out again after delete pict
 *
 *
 *    mtk70508
 * [DUMA00134136] DS629 OV2655 sensor driver disable denoise and sharpness function
 * make full size == exposure size
 *
 *    mtk70508
 * [DUMA00134136] DS629 OV2655 sensor driver disable denoise and sharpness function
 *
 *
 *    mtk70508
 * [DUMA00134136] DS629 OV2655 sensor driver disable denoise and sharpness function
 *
 *
 *    mtk70508
 * [DUMA00126076] OV2650 sensor driver check in
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

#include "ov2650_Sensor.h"
#include "ov2650_Camera_Sensor_para.h"
#include "ov2650_CameraCustomized.h"


#define OV2650_DEBUG
#ifdef OV2650_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define OV2650_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para ,1,OV2650_WRITE_ID)
#define OV2650_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,OV2650_WRITE_ID)
kal_uint16 OV2650_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV2650_WRITE_ID);
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
//#include "CameraCustomized.h"
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
/*
DBGPARAM dpCurSettings = {
    TEXT("Sensor"), {
        TEXT("Preview"),TEXT("Capture"),TEXT("Init"),TEXT("Error"),
        TEXT("Gain"),TEXT("Shutter"),TEXT("Undef"),TEXT("Undef"),
        TEXT("Undef"),TEXT("Undef"),TEXT("Undef"),TEXT("Undef"),
        TEXT("Undef"),TEXT("Undef"),TEXT("Undef"),TEXT("Undef")},
    0x00FF  // ZONE_INIT | ZONE_WARNING | ZONE_ERROR
};

*/
kal_bool  OV2650_MPEG4_encode_mode = KAL_FALSE;
kal_uint16  OV2650_sensor_gain_base=0x0;
///* MAX/MIN Explosure Lines Used By AE Algorithm */
kal_uint16 OV2650_MAX_EXPOSURE_LINES = 650;
//kal_uint8  OV2650_MIN_EXPOSURE_LINES = 2;
kal_uint32 OV2650_isp_master_clock;
kal_uint16 OV2650_CURRENT_FRAME_LINES = 650;

static kal_uint16 OV2650_dummy_pixels=0, OV2650_dummy_lines=0;
kal_uint16 OV2650_PV_dummy_pixels=0,OV2650_PV_dummy_lines=0;

//kal_uint8 OV2650_sensor_write_I2C_address = OV2650_WRITE_ID;
//kal_uint8 OV2650_sensor_read_I2C_address = OV2650_READ_ID;

//HANDLE OV2650hDrvI2C;
//I2C_TRANSACTION OV2650I2CConfig;
UINT8 OV2650PixelClockDivider=0;
kal_uint32 OV2650_sensor_pclk=19500000;

kal_uint16 OV2650_pv_exposure_lines=0,OV2650_g_iBackupExtraExp = 0,OV2650_extra_exposure_lines = 0;

kal_uint16 OV2650_sensor_id=0;

MSDK_SENSOR_CONFIG_STRUCT OV2650SensorConfigData;

kal_uint32 OV2650_FAC_SENSOR_REG;
//kal_uint16 OV2650_sensor_flip_value;

SENSOR_REG_STRUCT OV2650SensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT OV2650SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;

typedef enum
{
  OV2650_SVGA,  //1M    800*600
  OV2650_UXGA   //2M    1600*1200
} OV2650_RES_TYPE;
OV2650_RES_TYPE OV2650_g_RES=OV2650_SVGA;


typedef enum
{
  OV2650_MODE_PREVIEW,  //1M    800*600
  OV2650_MODE_CAPTURE   //2M    1600*1200
} OV2650_MODE;
OV2650_MODE g_iOV2650_Mode=OV2650_MODE_PREVIEW;





//void OV2650_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
//{
//  OV2650I2CConfig.operation=I2C_OP_WRITE;
//  OV2650I2CConfig.slaveAddr=OV2650_sensor_write_I2C_address>>1;
//  OV2650I2CConfig.transfer_num=1; /* TRANSAC_LEN */
//  OV2650I2CConfig.transfer_len=3;
//  OV2650I2CConfig.buffer[0]=(UINT8)(addr>>8);
//  OV2650I2CConfig.buffer[1]=(UINT8)(addr&0xFF);
//  OV2650I2CConfig.buffer[2]=(UINT8)para;
//  DRV_I2CTransaction(OV2650hDrvI2C, &OV2650I2CConfig);
//} /* OV2650_write_cmos_sensor() */
//
//kal_uint32 OV2650_read_cmos_sensor(kal_uint32 addr)
//{
//  kal_uint16 get_byte=0;
//  OV2650I2CConfig.operation=I2C_OP_WRITE;
//  OV2650I2CConfig.slaveAddr=OV2650_sensor_write_I2C_address>>1;
//  OV2650I2CConfig.transfer_num=1; /* TRANSAC_LEN */
//  OV2650I2CConfig.transfer_len=2;
//  OV2650I2CConfig.buffer[0]=(UINT8)(addr>>8);
//  OV2650I2CConfig.buffer[1]=(UINT8)(addr&0xFF);
//  DRV_I2CTransaction(OV2650hDrvI2C, &OV2650I2CConfig);
//
//  OV2650I2CConfig.operation=I2C_OP_READ;
//  OV2650I2CConfig.slaveAddr=OV2650_sensor_read_I2C_address>>1;
//  OV2650I2CConfig.transfer_num=1; /* TRANSAC_LEN */
//  OV2650I2CConfig.transfer_len=1;
//  DRV_I2CTransaction(OV2650hDrvI2C, &OV2650I2CConfig);
//  get_byte=OV2650I2CConfig.buffer[0];
//  return get_byte;
//} /* OV2650_read_cmos_sensor() */




void OV2650_write_shutter(kal_uint16 shutter)
{
    if (shutter > OV2650_MAX_EXPOSURE_LINES)
    {
        OV2650_extra_exposure_lines = shutter - OV2650_MAX_EXPOSURE_LINES;
        shutter = OV2650_MAX_EXPOSURE_LINES;
    }
    else
    {
        if((OV2650_CURRENT_FRAME_LINES != (OV2650_MAX_EXPOSURE_LINES+OV2650_SHUTTER_LINES_GAP))&&(OV2650_MPEG4_encode_mode==KAL_TRUE))
         {// Avoid shutter > frame lines case, set the frame lines  for video
          OV2650_CURRENT_FRAME_LINES = OV2650_MAX_EXPOSURE_LINES+OV2650_SHUTTER_LINES_GAP;
          OV2650_write_cmos_sensor(0x302A, OV2650_CURRENT_FRAME_LINES >> 8);
          OV2650_write_cmos_sensor(0x302B, OV2650_CURRENT_FRAME_LINES & 0x00FF);
         }
        OV2650_extra_exposure_lines = 0;
    }

    OV2650_write_cmos_sensor(0x3002, (shutter >> 8) & 0xff );
    OV2650_write_cmos_sensor(0x3003, shutter & 0x00ff );

    if (((OV2650_g_iBackupExtraExp == 0) && (OV2650_extra_exposure_lines > 0))|| // set extra shutter  when extra_shutter is zero before
        (g_iOV2650_Mode == OV2650_MODE_CAPTURE ))// Set extra shutter when capture
    {
        OV2650_write_cmos_sensor(0x302d, (OV2650_extra_exposure_lines >> 8) & 0xff);
        OV2650_write_cmos_sensor(0x302e, OV2650_extra_exposure_lines & 0xff);
        OV2650_g_iBackupExtraExp = OV2650_extra_exposure_lines;
    }
    //OV2650_g_iBackupExtraExp = OV2650_extra_exposure_lines;
    //for not use extra shutter, don't enable it
/*
    if (shutter > OV2650_MAX_EXPOSURE_LINES)
    {
     OV2650_write_cmos_sensor(0x302A, (shutter) >> 8);
     OV2650_write_cmos_sensor(0x302B, (shutter) & 0x00FF);
     OV2650_CURRENT_FRAME_LINES = shutter;
    }
    else
    {
        if(OV2650_CURRENT_FRAME_LINES > OV2650_MAX_EXPOSURE_LINES)
            {
            OV2650_write_cmos_sensor(0x302A, OV2650_MAX_EXPOSURE_LINES >> 8);
            OV2650_write_cmos_sensor(0x302B, OV2650_MAX_EXPOSURE_LINES & 0x00FF);
            OV2650_CURRENT_FRAME_LINES = OV2650_MAX_EXPOSURE_LINES;
            }
    }
    OV2650_write_cmos_sensor(0x3002, (shutter >> 8) & 0xff );
    OV2650_write_cmos_sensor(0x3003, shutter & 0x00ff );*/
}   /* write_OV2650_shutter */



static kal_uint16 OV2650Reg2Gain(const kal_uint8 iReg)
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

static kal_uint8 OV2650Gain2Reg(const kal_uint16 iGain)
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

    return iReg;

}

/*************************************************************************
* FUNCTION
*   OV2650_SetGain
*
* DESCRIPTION
*   This function is to set global gain to sensor.
*
* PARAMETERS
*   gain : sensor global gain(base: 0x40)
*
* RETURNS
*   the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/

void OV2650_SetGain(UINT16 iGain)
{
//  static kal_uint16 Extra_Shutter = 0;
    kal_uint8 iReg;
     iReg = OV2650Gain2Reg(iGain);
     //=======================================================
     /* For sensor Gain curve correction, all shift number is test result. mtk70508*/
     if((iReg >=0x10)&&(iReg <0x30))// 2*Gain -4*Gain ,shift  1/8* Gain
      iReg +=1;

     if((iReg > 0x1f)&&(iReg <0x30))// Above 4 *Gain  after shift 1/8* Gain, Set 4*Gain register value ,Then shift  again
      {
      iReg |= 0x30;
      iReg += 0x2;
      }
     else if((iReg >=0x30)&&(iReg <0x70))//  4*Gain - 8*Gain, Shift  3/4* Gain
      iReg +=3;
     if(OV2650_sensor_id == OV2655_SENSOR_ID)
     {
          if((iReg > 0x3f)||(iReg >=0x70))//ov2655 max gain is 8X
             iReg = 0x3f;
     }
     else
     {
         if((iReg > 0x3f)&&(iReg <0x70))// above 8*Gain after shift   1* Gain, Set 8*Gain register value
          {
          iReg |= 0x70;
          //iReg +=1;
          }
          else if((iReg >=0x70)&&(iReg <0xf0))//  8*Gain - 16*Gain, Shift  3/2* Gain
          iReg +=3;

          if(iReg >0x7f)// above 16*Gain, Set  as 16*Gain
          iReg =0xf0;
     }
      //attention: OV2650 Max Gain is 16 * Basegain,OV2655 max gain is 8*Basegain
      //========================================================
      OV2650_write_cmos_sensor(0x3000, (kal_uint32)iReg);

//===============================
  /* Set extra shutter here because extra shutter active time same as gain  */
        if (OV2650_g_iBackupExtraExp !=OV2650_extra_exposure_lines)
        {
         OV2650_write_cmos_sensor(0x302d, (OV2650_extra_exposure_lines >> 8) & 0xff);
         OV2650_write_cmos_sensor(0x302e, OV2650_extra_exposure_lines & 0xff);
         OV2650_g_iBackupExtraExp = OV2650_extra_exposure_lines;
        }
//==================================
}   /*  OV2650_SetGain  */


/*************************************************************************
* FUNCTION
*   read_OV2650_gain
*
* DESCRIPTION
*   This function is to set global gain to sensor.
*
* PARAMETERS
*   None
*
* RETURNS
*   gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/

kal_uint16 read_OV2650_gain(void)
{
   return (kal_uint16)OV2650_read_cmos_sensor(0x3000);
}  /* read_OV2650_gain */

void write_OV2650_gain(kal_uint16 gain)
{
    OV2650_SetGain(gain);
}
void OV2650_camera_para_to_sensor(void)
{
    kal_uint32  i;
    for(i=0; 0xFFFFFFFF!=OV2650SensorReg[i].Addr; i++)
    {
        OV2650_write_cmos_sensor(OV2650SensorReg[i].Addr, OV2650SensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV2650SensorReg[i].Addr; i++)
    {
        OV2650_write_cmos_sensor(OV2650SensorReg[i].Addr, OV2650SensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        OV2650_write_cmos_sensor(OV2650SensorCCT[i].Addr, OV2650SensorCCT[i].Para);
    }
}

// update camera_para from sensor register
void OV2650_sensor_to_camera_para(void)
{
    kal_uint32  i;
    for(i=0; 0xFFFFFFFF!=OV2650SensorReg[i].Addr; i++)
    {
        OV2650SensorReg[i].Para = OV2650_read_cmos_sensor(OV2650SensorReg[i].Addr);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV2650SensorReg[i].Addr; i++)
    {
        OV2650SensorReg[i].Para = OV2650_read_cmos_sensor(OV2650SensorReg[i].Addr);
    }
}

kal_int32  OV2650_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void OV2650_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
{
   switch (group_idx)
   {
        case PRE_GAIN:
            sprintf((char *)group_name_ptr, "CCT");
            *item_count_ptr = 2;
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

void OV2650_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
    kal_int16 temp_reg=0;
    kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;

    switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
                case 0:
                  sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Global");
                    temp_addr = PRE_GAIN_INDEX;
                break;
                case 1:
                  sprintf((char *)info_ptr->ItemNamePtr,"GLOBAL_GAIN");
                    temp_addr = GLOBAL_GAIN_INDEX;
                break;
                default:
                   ASSERT(0);
            }

            temp_para=OV2650SensorCCT[temp_addr].Para;

     temp_gain = OV2650Reg2Gain(temp_para);

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

                  //temp_reg=OV2650SensorReg[CMMCLK_CURRENT_INDEX].Para;
                  temp_reg = ISP_DRIVING_2MA;
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
                  info_ptr->ItemValue=OV2650_MAX_EXPOSURE_LINES;
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

void OV2650_set_isp_driving_current(kal_uint8 current)
{
}

kal_bool OV2650_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
   kal_uint16 /*temp_gain=0,*/ temp_addr=0, temp_para=0;

   switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
                case 0:
                  temp_addr = PRE_GAIN_INDEX;
                break;
                case 1:
                  temp_addr = GLOBAL_GAIN_INDEX;
                break;
                default:
                   ASSERT(0);
            }

            temp_para = OV2650Gain2Reg(ItemValue);


            OV2650SensorCCT[temp_addr].Para = temp_para;
                OV2650_write_cmos_sensor(OV2650SensorCCT[temp_addr].Addr,temp_para);

            OV2650_sensor_gain_base=read_OV2650_gain();

        break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                  if(ItemValue==2)
                  {
                      OV2650SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
//                      OV2650_set_isp_driving_current(ISP_DRIVING_2MA);
                  }
                  else if(ItemValue==3 || ItemValue==4)
                  {
                      OV2650SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
//                      OV2650_set_isp_driving_current(ISP_DRIVING_4MA);
                  }
                  else if(ItemValue==5 || ItemValue==6)
                  {
                      OV2650SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
//                      OV2650_set_isp_driving_current(ISP_DRIVING_6MA);
                  }
                  else
                  {
                      OV2650SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
//                      OV2650_set_isp_driving_current(ISP_DRIVING_8MA);
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
                  OV2650_FAC_SENSOR_REG=ItemValue;
                break;
                case 1:
                  OV2650_write_cmos_sensor(OV2650_FAC_SENSOR_REG,ItemValue);
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

static void OV2650_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
    kal_uint16 PixelsOneline = OV2650_FULL_PERIOD_PIXEL_NUMS;


    if(OV2650_SVGA == OV2650_g_RES)
    {
        PixelsOneline = (OV2650_PV_PERIOD_PIXEL_NUMS + iPixels)* 2; // time 2 for 800 * 600, not required for UXGA
        if(OV2650_MPEG4_encode_mode == KAL_FALSE)//for Fix video framerate
            OV2650_CURRENT_FRAME_LINES = iLines + OV2650_PV_PERIOD_LINE_NUMS ;
    }
    else if (OV2650_UXGA== OV2650_g_RES)
    {

        PixelsOneline = OV2650_FULL_PERIOD_PIXEL_NUMS + iPixels;
        OV2650_CURRENT_FRAME_LINES = iLines + OV2650_FULL_PERIOD_LINE_NUMS;
    }
    OV2650_write_cmos_sensor(0x3028, (PixelsOneline - 1) >> 8);
    OV2650_write_cmos_sensor(0x3029, (PixelsOneline - 1) & 0x00FF);
    if(OV2650_MPEG4_encode_mode == KAL_FALSE)//for Fix video framerate,set the frame lines in night mode function
    {
        OV2650_write_cmos_sensor(0x302A, OV2650_CURRENT_FRAME_LINES >> 8);
        OV2650_write_cmos_sensor(0x302B, OV2650_CURRENT_FRAME_LINES & 0x00FF);
        OV2650_MAX_EXPOSURE_LINES = OV2650_CURRENT_FRAME_LINES - OV2650_SHUTTER_LINES_GAP;
    }
}   /*  OV2650_SetDummy */


void OV2650_set_SVGA()
{
        OV2650_g_RES = OV2650_SVGA;

        //OV2650_write_cmos_sensor(0x3012,0x80);
//Timing Control
        OV2650_write_cmos_sensor(0x308c,0x80);
        OV2650_write_cmos_sensor(0x308d,0x0e);
//DVP Control
        OV2650_write_cmos_sensor(0x360b,0x00);
//IO Control 1-3
        OV2650_write_cmos_sensor(0x30b0,0xff);
        OV2650_write_cmos_sensor(0x30b1,0xff);
        OV2650_write_cmos_sensor(0x30b2,0x2c);

//CLK Setting
//Internal clock = (64-0x300e[5:0])*0x300f[7:6]*MCLK/0x300f[2:0]/0x3010[4]/(0x3011[5:0]+1)/4
        OV2650_write_cmos_sensor(0x300e,0x28);
        OV2650_write_cmos_sensor(0x300f,0xa6);
        OV2650_write_cmos_sensor(0x3010,0x81);
        OV2650_write_cmos_sensor(0x3011,0x01);

//Pixel clock = 19.5M Hz

// Init extra shutter
    //  OV2650_write_cmos_sensor(0x302d, 0x00);
    //  OV2650_write_cmos_sensor(0x302e, 0x00);


//Reserved
        OV2650_write_cmos_sensor(0x3082,0x01);
        OV2650_write_cmos_sensor(0x30f4,0x01);
// Mirror
        OV2650_write_cmos_sensor(0x3090,0x33);
        OV2650_write_cmos_sensor(0x3091,0xc0);
//Reserved
        OV2650_write_cmos_sensor(0x30ac,0x42);
        OV2650_write_cmos_sensor(0x30d1,0x08);
        OV2650_write_cmos_sensor(0x30a8,0x56);
//Max shutter and AGC
        OV2650_write_cmos_sensor(0x3015,0x04);
//Reserved
        OV2650_write_cmos_sensor(0x3093,0x00);
//Digital gain source
        OV2650_write_cmos_sensor(0x307e,0xe5);
//Reserved
        OV2650_write_cmos_sensor(0x3079,0x00);
        OV2650_write_cmos_sensor(0x30aa,0x42);
//Auto control --Drop frame data when AEC
        OV2650_write_cmos_sensor(0x3017,0x40);
//Reserved
        OV2650_write_cmos_sensor(0x30f3,0x82);
        OV2650_write_cmos_sensor(0x306a,0x0c);
//BLC
        OV2650_write_cmos_sensor(0x306d,0x00);
//ISP Control reserved
        OV2650_write_cmos_sensor(0x336a,0x3c);
//Vsync drop option
        OV2650_write_cmos_sensor(0x3076,0x6a);
//Reserved
        OV2650_write_cmos_sensor(0x30d9,0x8c);
//Auto control--AGC and AEC
        OV2650_write_cmos_sensor(0x3016,0x82);
//DVP Control ---Sync sel and data output type
        OV2650_write_cmos_sensor(0x3601,0x30);
//Reserved
        OV2650_write_cmos_sensor(0x304e,0x88);
        OV2650_write_cmos_sensor(0x30f1,0x82);
//lens correction
        OV2650_write_cmos_sensor(0x3350,0x28);
        OV2650_write_cmos_sensor(0x3351,0x21);
        OV2650_write_cmos_sensor(0x3352,0x80);
        OV2650_write_cmos_sensor(0x3353,0x18);
        OV2650_write_cmos_sensor(0x3354,0x00);
        OV2650_write_cmos_sensor(0x3355,0x84);
        OV2650_write_cmos_sensor(0x3356,0x28);
        OV2650_write_cmos_sensor(0x3357,0x21);
        OV2650_write_cmos_sensor(0x3358,0x80);
        OV2650_write_cmos_sensor(0x3359,0x18);
        OV2650_write_cmos_sensor(0x335a,0x00);
        OV2650_write_cmos_sensor(0x335b,0x84);
        OV2650_write_cmos_sensor(0x335c,0x28);
        OV2650_write_cmos_sensor(0x335d,0x21);
        OV2650_write_cmos_sensor(0x335e,0x80);
        OV2650_write_cmos_sensor(0x335f,0x18);
        OV2650_write_cmos_sensor(0x3360,0x00);
        OV2650_write_cmos_sensor(0x3361,0x84);
        OV2650_write_cmos_sensor(0x3363,0x01);
        OV2650_write_cmos_sensor(0x3364,0x03);
        OV2650_write_cmos_sensor(0x3365,0x02);
//LENC BLC offset
        OV2650_write_cmos_sensor(0x3366,0x00);
//CMX
/*
        OV2650_write_cmos_sensor(0x3380,0x20);
        OV2650_write_cmos_sensor(0x3381,0x64);
        OV2650_write_cmos_sensor(0x3382,0x08);
        OV2650_write_cmos_sensor(0x3383,0x30);
        OV2650_write_cmos_sensor(0x3384,0x90);
        OV2650_write_cmos_sensor(0x3385,0xc0);
        OV2650_write_cmos_sensor(0x3386,0xa0);
        OV2650_write_cmos_sensor(0x3387,0x98);
        OV2650_write_cmos_sensor(0x3388,0x08);
        OV2650_write_cmos_sensor(0x3389,0x98);
        OV2650_write_cmos_sensor(0x338a,0x01);
*/
//Gamma
        OV2650_write_cmos_sensor(0x3340,0x0e);
        OV2650_write_cmos_sensor(0x3341,0x1a);
        OV2650_write_cmos_sensor(0x3342,0x31);
        OV2650_write_cmos_sensor(0x3343,0x46);
        OV2650_write_cmos_sensor(0x3344,0x5a);
        OV2650_write_cmos_sensor(0x3345,0x69);
        OV2650_write_cmos_sensor(0x3346,0x75);
        OV2650_write_cmos_sensor(0x3347,0x7e);
        OV2650_write_cmos_sensor(0x3348,0x88);
        OV2650_write_cmos_sensor(0x3349,0x96);
        OV2650_write_cmos_sensor(0x334a,0xa3);
        OV2650_write_cmos_sensor(0x334b,0xaf);
        OV2650_write_cmos_sensor(0x334c,0xc4);
        OV2650_write_cmos_sensor(0x334d,0xd7);
        OV2650_write_cmos_sensor(0x334e,0xe8);
        OV2650_write_cmos_sensor(0x334f,0x20);

//ISP Control Hw component  Enbale  register
/*  Bit7:SDE_En
    Bit6:UV_ADJ_En
    Bit5:CMX_En
    Bit4:SharpEn_En
    Bit3:DNC_En
    Bit2:CIP_En
    Bit1:BC_En
    Bit0:WC_En*/
        OV2650_write_cmos_sensor(0x3301,0xbf);

//BLC isuue
        OV2650_write_cmos_sensor(0x3069,0x84);//04->84
        OV2650_write_cmos_sensor(0x306c,0x10);//00->10
        OV2650_write_cmos_sensor(0x309c,0x00);//02->00
        OV2650_write_cmos_sensor(0x30a2,0x41);//43->41
//Digital Gain enable and filp mirror control
        OV2650_write_cmos_sensor(0x307c,0x10);//00->10
//reserved
        OV2650_write_cmos_sensor(0x304a,0x00);
        OV2650_write_cmos_sensor(0x304f,0x00);
        OV2650_write_cmos_sensor(0x304d,0x22);
        OV2650_write_cmos_sensor(0x304f,0xa0);
        OV2650_write_cmos_sensor(0x3095,0x00);
        OV2650_write_cmos_sensor(0x3096,0xff);
        OV2650_write_cmos_sensor(0x3097,0x00);
//Select average mode and SVGA
        OV2650_write_cmos_sensor(0x3012,0x10);
//Target black level value
        OV2650_write_cmos_sensor(0x306f,0x14);
//Scale offset
        OV2650_write_cmos_sensor(0x3319,0x28);
        OV2650_write_cmos_sensor(0x331d,0x48);
//VARIO Enbale
        OV2650_write_cmos_sensor(0x3303,0x04);
//image window setting
        OV2650_write_cmos_sensor(0x3023,0x04);//vertical window start
        OV2650_write_cmos_sensor(0x3024,0x06); // Array Horizantal
        OV2650_write_cmos_sensor(0x3025,0x58);
        OV2650_write_cmos_sensor(0x3026,0x02); // Aarry Vertical
        OV2650_write_cmos_sensor(0x3027,0x5e);//0x5e
// Horizantal Total size
    //  OV2650_write_cmos_sensor(0x3028,...); // Vertical total 768
    //  OV2650_write_cmos_sensor(0x3029,...);

//Vertical total size
        OV2650_write_cmos_sensor(0x302A, OV2650_PV_PERIOD_LINE_NUMS >> 8);
        OV2650_write_cmos_sensor(0x302B, OV2650_PV_PERIOD_LINE_NUMS & 0x00FF);

//Output size

//s_porting add
#if 1
        OV2650_write_cmos_sensor(0x3088,0x03); // Output 808 pixels,
        OV2650_write_cmos_sensor(0x3089,0x28);
        OV2650_write_cmos_sensor(0x308a,0x02); // Output 602 lines
        OV2650_write_cmos_sensor(0x308b,0x5a);
#else
        OV2650_write_cmos_sensor(0x3088,0x03); // Output 800 pixels,
        OV2650_write_cmos_sensor(0x3089,0x20);
        OV2650_write_cmos_sensor(0x308a,0x02); // Output 600 lines
        OV2650_write_cmos_sensor(0x308b,0x58);
#endif
//e_porting add

    //Noise Issue enable ISP
        OV2650_write_cmos_sensor(0x3300,0x83); //8bits. 2008-08-26

//Denoise function, only aviable in 8bits. 2008-08-26
/*
//Auto Denoise & Sharpness
        OV2650_write_cmos_sensor(0x3306,0x00);
        OV2650_write_cmos_sensor(0x3370,0xd0);//a0->d0
        OV2650_write_cmos_sensor(0x3373,0x40);//00->10
        OV2650_write_cmos_sensor(0x3374,0x10);
        OV2650_write_cmos_sensor(0x3375,0x10);

        //Sharpness function
        OV2650_write_cmos_sensor(0x3376,0x04);
        OV2650_write_cmos_sensor(0x3377,0x00);
        OV2650_write_cmos_sensor(0x3378,0x04);
            OV2650_write_cmos_sensor(0x3379,0x80);
    */
    //Manual Denoise & Sharpness
        OV2650_write_cmos_sensor(0x3306,0x0C);
        OV2650_write_cmos_sensor(0x3370,0x00);// Disable Denoise
        OV2650_write_cmos_sensor(0x3373,0x40);//00->10
        OV2650_write_cmos_sensor(0x3374,0x10);
        OV2650_write_cmos_sensor(0x3375,0x10);

        //Sharpness function
        OV2650_write_cmos_sensor(0x3371,0x00);//disable Sharpness
        OV2650_write_cmos_sensor(0x3376,0x04);
        OV2650_write_cmos_sensor(0x3377,0x00);
        OV2650_write_cmos_sensor(0x3378,0x04);
        OV2650_write_cmos_sensor(0x3379,0x80);
//Bypass format
        OV2650_write_cmos_sensor(0x3100,0x04);
//format control select RAW output
        OV2650_write_cmos_sensor(0x3400,0x90);

        OV2650_write_cmos_sensor(0x3606,0x20); // Hsync Mode 0x30
//CMX
        OV2650_write_cmos_sensor(0x3380,0x80);
        OV2650_write_cmos_sensor(0x3381,0x00);
        OV2650_write_cmos_sensor(0x3382,0x00);
        OV2650_write_cmos_sensor(0x3383,0x00);
        OV2650_write_cmos_sensor(0x3384,0x80);
        OV2650_write_cmos_sensor(0x3385,0x00);
        OV2650_write_cmos_sensor(0x3386,0x00);
        OV2650_write_cmos_sensor(0x3387,0x00);
        OV2650_write_cmos_sensor(0x3388,0x80);
        OV2650_write_cmos_sensor(0x3389,0x00);
        OV2650_write_cmos_sensor(0x338a,0x04);
/*  Bit7:SDE_En
    Bit6:UV_ADJ_En
    Bit5:CMX_En
    Bit4:SharpEn_En
    Bit3:DNC_En
    Bit2:CIP_En
    Bit1:BC_En
    Bit0:WC_En*/
        OV2650_write_cmos_sensor(0x3301,0xbf);
//reserved
        OV2650_write_cmos_sensor(0x30f3,0x83);
        OV2650_write_cmos_sensor(0x304e,0x4b);

//Sleep off-->On
        OV2650_write_cmos_sensor(0x3086,0x0f);
        OV2650_write_cmos_sensor(0x3086,0x00);
//DVP Control ---Sync sel and data output type
        OV2650_write_cmos_sensor(0x3601,0x30);
//AEC and Banding enbale
        OV2650_write_cmos_sensor(0x3013,0xE0);
//AGC control
        OV2650_write_cmos_sensor(0x3000,0x1B);
        OV2650_write_cmos_sensor(0x3001,0x0B);
//AEC Control
        OV2650_write_cmos_sensor(0x3002,0x02);
        OV2650_write_cmos_sensor(0x3003,0x65);
//Reserved
        OV2650_write_cmos_sensor(0x30a3,0x80);
}

void OV2650_set_UXGA()
{

        OV2650_g_RES = OV2650_UXGA;

//============CLK Setting================
//Pixel clock = 36M Hz
        OV2650_write_cmos_sensor(0x300e,0x34);
        //OV2650_write_cmos_sensor(0x300f,0xa6);
        OV2650_write_cmos_sensor(0x3010,0x82);
        OV2650_write_cmos_sensor(0x3011,0x00);
//=============================
//AEC and Banding enbale
        OV2650_write_cmos_sensor(0x3013,0xf0);
        OV2650_write_cmos_sensor(0x3014,0x44);
//Average luminance
        OV2650_write_cmos_sensor(0x301b,0x73);
//Banding setting
        OV2650_write_cmos_sensor(0x3070,0xba);//50Hz
        OV2650_write_cmos_sensor(0x3072,0x9a);//60Hz
        //OV2650_write_cmos_sensor(0x307c,0x10);
//Reserved
        OV2650_write_cmos_sensor(0x308e,0x4c);
//Reserved
        OV2650_write_cmos_sensor(0x3095,0x07);
        OV2650_write_cmos_sensor(0x3096,0x16);
        OV2650_write_cmos_sensor(0x3097,0x1a);
        OV2650_write_cmos_sensor(0x309c,0x00 );
        OV2650_write_cmos_sensor(0x30a2,0x41);//45->41
        OV2650_write_cmos_sensor(0x30a3,0x11);
        OV2650_write_cmos_sensor(0x30af,0x00);
        OV2650_write_cmos_sensor(0x30c1,0xb9);
        OV2650_write_cmos_sensor(0x30de,0x81);
        OV2650_write_cmos_sensor(0x30df,0x50);
        OV2650_write_cmos_sensor(0x30f3,0x82);

//image window setting

//s_porting add
#if 1
        OV2650_write_cmos_sensor(0x3022,0x00);//vertical window start
        OV2650_write_cmos_sensor(0x3023,0x04);//vertical window start
#else
        OV2650_write_cmos_sensor(0x3023,0x08);//vertical window start
#endif
//e_porting add

//Vertical height

//s_porting add
#if 1
        OV2650_write_cmos_sensor(0x3026,0x04);
        OV2650_write_cmos_sensor(0x3027,0xc2);
#else
        OV2650_write_cmos_sensor(0x3026,0x04);
        OV2650_write_cmos_sensor(0x3027,0xbc);
#endif
//e_porting add

//Vertical total size
        OV2650_write_cmos_sensor(0x302A, OV2650_FULL_PERIOD_LINE_NUMS >> 8);
        OV2650_write_cmos_sensor(0x302B, OV2650_FULL_PERIOD_LINE_NUMS & 0x00FF);
//output window

//s_porting add
#if 1
        OV2650_write_cmos_sensor(0x3088,0x06);  //Output 1600 pixels
        OV2650_write_cmos_sensor(0x3089,0x48);
        OV2650_write_cmos_sensor(0x308a,0x04);      //Output 1200 lines
        OV2650_write_cmos_sensor(0x308b,0xc2);
#else
        OV2650_write_cmos_sensor(0x3088,0x06);  //Output 1600 pixels
        OV2650_write_cmos_sensor(0x3089,0x40);
        OV2650_write_cmos_sensor(0x308a,0x04);      //Output 1200 lines
        OV2650_write_cmos_sensor(0x308b,0xb0);
#endif
//e_porting add

//recover to UXGA
        OV2650_write_cmos_sensor(0x3012,0x00);

//BLC
        OV2650_write_cmos_sensor(0x306f,0x54);

        OV2650_write_cmos_sensor(0x30dc,0x00);
//Scale offset
        OV2650_write_cmos_sensor(0x3319,0x6c);
        OV2650_write_cmos_sensor(0x331d,0x6c);
//VARIO Disable
        OV2650_write_cmos_sensor(0x3303,0x00);

//BLC isuue
        OV2650_write_cmos_sensor(0x3069,0x84);
        OV2650_write_cmos_sensor(0x306c,0x10);
        OV2650_write_cmos_sensor(0x306f,0x54);
}



/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   OV2650Open
*
* DESCRIPTION
*   This function initialize the registers of CMOS sensor
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/

UINT32 OV2650Open(void)
{
//  OV2650hDrvI2C=DRV_I2COpen(0);
//  DRV_I2CSetParam(OV2650hDrvI2C, I2C_VAL_FS_SAMPLE_DIV, 3);
//  DRV_I2CSetParam(OV2650hDrvI2C, I2C_VAL_FS_STEP_DIV, 8);
//  DRV_I2CSetParam(OV2650hDrvI2C, I2C_VAL_DELAY_LEN, 2);
//  DRV_I2CSetParam(OV2650hDrvI2C, I2C_VAL_CLK_EXT, I2C_CLKEXT_DISABLE);
//  OV2650I2CConfig.trans_mode=I2C_TRANSFER_FAST_MODE;
//  OV2650I2CConfig.slaveAddr=OV2650_sensor_write_I2C_address>>1;
//  OV2650I2CConfig.RS_ST=I2C_TRANSFER_STOP;    /* for fast mode */

    OV2650_write_cmos_sensor(0x3012, 0x80);
    Sleep(30);                  // at least one frame delay time needed after SW reset
    OV2650_sensor_id = ((OV2650_read_cmos_sensor(0x300A) << 8) | OV2650_read_cmos_sensor(0x300B));
    // check if sensor ID correct
    if ((OV2650_sensor_id != OV2655_SENSOR_ID) && (OV2650_sensor_id != OV2650_SENSOR_ID_1)&&(OV2650_sensor_id != OV2650_SENSOR_ID_2)) {
    return ERROR_SENSOR_CONNECT_FAIL;
        }

    RETAILMSG(1, (TEXT("OV2655 Sensor Read ID OK \r\n")));
    OV2650_set_SVGA();
    OV2650_sensor_gain_base = read_OV2650_gain();

    OV2650_g_iBackupExtraExp = 0;
    OV2650_write_cmos_sensor(0x302d, 0x00);
    OV2650_write_cmos_sensor(0x302e, 0x00);

    return ERROR_NONE;
}


UINT32 OV2650GetSensorID(UINT32 *sensorID)
{


    OV2650_write_cmos_sensor(0x3012, 0x80);
    Sleep(30);                  // at least one frame delay time needed after SW reset
    *sensorID = ((OV2650_read_cmos_sensor(0x300A) << 8) | OV2650_read_cmos_sensor(0x300B));
    // check if sensor ID correct
    if ((*sensorID != OV2655_SENSOR_ID) && (*sensorID != OV2650_SENSOR_ID_1)&&(*sensorID != OV2650_SENSOR_ID_2)) {
		 *sensorID = 0xFFFFFFFF;
    return ERROR_SENSOR_CONNECT_FAIL;
        }

    RETAILMSG(1, (TEXT("OV2655 Sensor Read ID OK \r\n")));
    //OV2650_set_SVGA();
    //OV2650_sensor_gain_base = read_OV2650_gain();

    //OV2650_g_iBackupExtraExp = 0;
    //OV2650_write_cmos_sensor(0x302d, 0x00);
    //OV2650_write_cmos_sensor(0x302e, 0x00);

    return ERROR_NONE;
}




/*************************************************************************
* FUNCTION
*   OV2650_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of OV2650 to change exposure time.
*
* PARAMETERS
*   shutter : exposured lines
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/

void OV2650_SetShutter(kal_uint16 iShutter)
{

    if (iShutter <4 )
        iShutter =4;
    OV2650_pv_exposure_lines = iShutter;
    OV2650_write_shutter(iShutter);
}   /*  OV2650_SetShutter   */



/*************************************************************************
* FUNCTION
*   OV2650_read_shutter
*
* DESCRIPTION
*   This function to  Get exposure time.
*
* PARAMETERS
*   None
*
* RETURNS
*   shutter : exposured lines
*
* GLOBALS AFFECTED
*
*************************************************************************/
/*
UINT16 OV2650_read_shutter(void)
{
    return (UINT16)( (OV2650_read_cmos_sensor(0x3002)<<8) | OV2650_read_cmos_sensor(0x3003) );
}
*/
/*************************************************************************
* FUNCTION
*   OV2650_night_mode
*
* DESCRIPTION
*   This function night mode of OV2650.
*
* PARAMETERS
*   none
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void OV2650_NightMode(kal_bool bEnable)
{
    /************************************************************************/
    /*          Auto Mode: 30fps                                    */
    /*          Night Mode:15fps                            */
    /************************************************************************/
    if(bEnable)
    {
        if(OV2650_MPEG4_encode_mode==KAL_TRUE)
        {
            OV2650_MAX_EXPOSURE_LINES = (kal_uint16)((OV2650_sensor_pclk/15)/(OV2650_PV_PERIOD_PIXEL_NUMS+OV2650_PV_dummy_pixels));
            OV2650_write_cmos_sensor(0x302A, OV2650_MAX_EXPOSURE_LINES >> 8);
            OV2650_write_cmos_sensor(0x302B, OV2650_MAX_EXPOSURE_LINES & 0x00FF);
            OV2650_CURRENT_FRAME_LINES = OV2650_MAX_EXPOSURE_LINES;
            OV2650_MAX_EXPOSURE_LINES = OV2650_CURRENT_FRAME_LINES - OV2650_SHUTTER_LINES_GAP;
        }
    }
    else// Fix video framerate 30 fps
    {
        if(OV2650_MPEG4_encode_mode==KAL_TRUE)
        {
          OV2650_MAX_EXPOSURE_LINES = (kal_uint16)((OV2650_sensor_pclk/30)/(OV2650_PV_PERIOD_PIXEL_NUMS+OV2650_PV_dummy_pixels));
         if(OV2650_pv_exposure_lines < (OV2650_MAX_EXPOSURE_LINES - OV2650_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
         {
          OV2650_write_cmos_sensor(0x302A, OV2650_MAX_EXPOSURE_LINES >> 8);
          OV2650_write_cmos_sensor(0x302B, OV2650_MAX_EXPOSURE_LINES & 0x00FF);
          OV2650_CURRENT_FRAME_LINES = OV2650_MAX_EXPOSURE_LINES;
         }
         OV2650_MAX_EXPOSURE_LINES = OV2650_MAX_EXPOSURE_LINES - OV2650_SHUTTER_LINES_GAP;
        }
    }
}   /*  OV2650_NightMode    */



/*************************************************************************
* FUNCTION
*   OV2650Close
*
* DESCRIPTION
*   This function is to turn off sensor module power.
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV2650Close(void)
{
//  CISModulePowerOn(FALSE);

//  DRV_I2CClose(OV2650hDrvI2C);
    return ERROR_NONE;
}   /* OV2650Close() */

/*************************************************************************
* FUNCTION
*   OV2650Preview
*
* DESCRIPTION
*   This function start the sensor preview.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV2650Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 /*iDummyPixels = 0,*/ iDummyLines = 0, iStartX = 0, iStartY = 0;

    g_iOV2650_Mode = OV2650_MODE_PREVIEW;

    if(OV2650_UXGA == OV2650_g_RES)
      {
         OV2650_set_SVGA();
      }


    if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)       // MPEG4 Encode Mode
    {
        OV2650_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {

        OV2650_MPEG4_encode_mode = KAL_FALSE;
    }

    iStartX += OV2650_IMAGE_SENSOR_PV_STARTX;
    iStartY += OV2650_IMAGE_SENSOR_PV_STARTY;

    switch (sensor_config_data->SensorImageMirror)
    {
    case IMAGE_NORMAL:
        OV2650_write_cmos_sensor(0x307C,0x10);
        OV2650_write_cmos_sensor(0x3090,0x33);
     break;
    case IMAGE_H_MIRROR:
        OV2650_write_cmos_sensor(0x307C,0x12);
        OV2650_write_cmos_sensor(0x3090,0x3b);
      break;
    case IMAGE_V_MIRROR:
        OV2650_write_cmos_sensor(0x307C,0x11);
        OV2650_write_cmos_sensor(0x3090,0x33);

         break;
    case IMAGE_HV_MIRROR:
        OV2650_write_cmos_sensor(0x307C,0x13);
        OV2650_write_cmos_sensor(0x3090,0x3b);
      break;
    default:
    ASSERT(0);
    }


    OV2650_dummy_pixels = 0;
     OV2650_dummy_lines = 0;
     OV2650_PV_dummy_pixels = OV2650_dummy_pixels;
     OV2650_PV_dummy_lines = OV2650_dummy_lines;

    OV2650_SetDummy(OV2650_dummy_pixels, OV2650_dummy_lines);
    OV2650_SetShutter(OV2650_pv_exposure_lines);

    memcpy(&OV2650SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    image_window->GrabStartX= iStartX;
    image_window->GrabStartY= iStartY;
    image_window->ExposureWindowWidth= OV2650_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
    image_window->ExposureWindowHeight= OV2650_IMAGE_SENSOR_PV_HEIGHT - 2*iStartY;
    return ERROR_NONE;
}   /* OV2650Preview() */

UINT32 OV2650Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint32 shutter=OV2650_pv_exposure_lines;
    kal_uint16 iStartX = 0, iStartY = 0;

    g_iOV2650_Mode = OV2650_MODE_CAPTURE;


    if(sensor_config_data->EnableShutterTansfer==KAL_TRUE)
        shutter=sensor_config_data->CaptureShutter;


      if ((image_window->ImageTargetWidth<= OV2650_IMAGE_SENSOR_PV_WIDTH) &&
        (image_window->ImageTargetHeight<= OV2650_IMAGE_SENSOR_PV_HEIGHT)) {

            OV2650_dummy_pixels= 0;
            OV2650_dummy_lines = 0;

        shutter = (shutter*(OV2650_IMAGE_SENSOR_SXGA_PIXELS_LINE + OV2650_PV_dummy_pixels))/(OV2650_IMAGE_SENSOR_SXGA_PIXELS_LINE+OV2650_dummy_pixels);
        iStartX = OV2650_IMAGE_SENSOR_PV_STARTX;
        iStartY = OV2650_IMAGE_SENSOR_PV_STARTY;
        image_window->GrabStartX=iStartX;
        image_window->GrabStartY=iStartY;
        image_window->ExposureWindowWidth=OV2650_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
        image_window->ExposureWindowHeight=OV2650_IMAGE_SENSOR_PV_HEIGHT- 2*iStartY;


    }else { // 2M UXGA Mode
        OV2650_dummy_pixels= 0;
        OV2650_dummy_lines = 0;
        OV2650_set_UXGA();
        //SVGA Internal CLK = 1/4 UXGA Internal CLK
        if(OV2650_sensor_id == OV2655_SENSOR_ID)
        shutter = 2* shutter;
        else
        shutter = 4* shutter;
        shutter = ((UINT32)(shutter*(OV2650_IMAGE_SENSOR_SXGA_PIXELS_LINE + OV2650_PV_dummy_pixels)))/(OV2650_IMAGE_SENSOR_UXGA_PIXELS_LINE+OV2650_dummy_pixels);
        iStartX = 2* OV2650_IMAGE_SENSOR_PV_STARTX;
        iStartY = 2* OV2650_IMAGE_SENSOR_PV_STARTY;

        image_window->GrabStartX=iStartX;
        image_window->GrabStartY=iStartY;
        image_window->ExposureWindowWidth=OV2650_IMAGE_SENSOR_FULL_WIDTH -2*iStartX;
        image_window->ExposureWindowHeight=OV2650_IMAGE_SENSOR_FULL_HEIGHT-2*iStartY;
    }//2M Capture

     OV2650_SetDummy(OV2650_dummy_pixels, OV2650_dummy_lines);

     OV2650_SetShutter(shutter);

        memcpy(&OV2650SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
     return ERROR_NONE;
}   /* OV2650Capture() */

UINT32 OV2650GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
/*
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH - 4*OV2650_IMAGE_SENSOR_PV_STARTX;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT - 4*OV2650_IMAGE_SENSOR_PV_STARTY;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH - 2*OV2650_IMAGE_SENSOR_PV_STARTX;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT - 2*OV2650_IMAGE_SENSOR_PV_STARTY;
*/    

    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH  - 2 * IMAGE_SENSOR_START_GRAB_X ;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH - 2 * IMAGE_SENSOR_START_GRAB_X;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT - 2 * IMAGE_SENSOR_START_GRAB_Y ;

    return ERROR_NONE;
}   /* OV2650GetResolution() */

UINT32 OV2650GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                      MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                      MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH - 2*OV2650_IMAGE_SENSOR_PV_STARTX;
    pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT - 2*OV2650_IMAGE_SENSOR_PV_STARTY;
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH - 4*OV2650_IMAGE_SENSOR_PV_STARTX;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_HEIGHT - 4*OV2650_IMAGE_SENSOR_PV_STARTY;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_Gb;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_2M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_2M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_2M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_2M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_2M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_2M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=FALSE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_05M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=FALSE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_05M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_05M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=FALSE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;


    pSensorInfo->CaptureDelayFrame = 1; 
    pSensorInfo->PreviewDelayFrame = 0; 
    pSensorInfo->VideoDelayFrame = 0; 	    
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
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount= 3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 1; 
            pSensorInfo->SensorGrabStartY = 1;             
        break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount= 3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 1; 
            pSensorInfo->SensorGrabStartY = 1;                         
        break;
        default:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount= 3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 1; 
            pSensorInfo->SensorGrabStartY = 1;                         
        break;
    }
    OV2650PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &OV2650SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* OV2650GetInfo() */


UINT32 OV2650Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                      MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            OV2650Preview(pImageWindow, pSensorConfigData);
        break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
            OV2650Capture(pImageWindow, pSensorConfigData);
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
}   /* OV2650Control() */

UINT32 OV2650FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
    MSDK_SENSOR_ENG_INFO_STRUCT *pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
        break;
        case SENSOR_FEATURE_GET_PERIOD:
            *pFeatureReturnPara16++=OV2650_PV_PERIOD_PIXEL_NUMS+OV2650_dummy_pixels;
            *pFeatureReturnPara16=OV2650_PV_PERIOD_LINE_NUMS+OV2650_dummy_lines;
            *pFeatureParaLen=4;
        break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            *pFeatureReturnPara32 = 19500000;
            *pFeatureParaLen=4;
        break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            OV2650_SetShutter(*pFeatureData16);
        break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            OV2650_NightMode((BOOL) *pFeatureData16);
        break;
        case SENSOR_FEATURE_SET_GAIN:
            OV2650_SetGain((UINT16) *pFeatureData16);
        break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            OV2650_isp_master_clock=*pFeatureData32;
        break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV2650_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
        case SENSOR_FEATURE_GET_REGISTER:
          pSensorRegData->RegData = OV2650_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
                OV2650SensorCCT[i].Addr=*pFeatureData32++;
                OV2650SensorCCT[i].Para=*pFeatureData32++;
            }
        break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV2650SensorCCT[i].Addr;
                *pFeatureData32++=OV2650SensorCCT[i].Para;
            }
        break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
                OV2650SensorReg[i].Addr=*pFeatureData32++;
                OV2650SensorReg[i].Para=*pFeatureData32++;
            }
        break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV2650SensorReg[i].Addr;
                *pFeatureData32++=OV2650SensorReg[i].Para;
            }
        break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=OV2650_SENSOR_ID_3;
                memcpy(pSensorDefaultData->SensorEngReg, OV2650SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, OV2650SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
        break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &OV2650SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            OV2650_camera_para_to_sensor();
        break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV2650_sensor_to_camera_para();
        break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=OV2650_get_sensor_group_count();
            *pFeatureParaLen=4;
        break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV2650_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
        break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV2650_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
        break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV2650_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
        break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 129;
            pSensorEngInfo->SensorType = CMOS_SENSOR;
            pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B;
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
        break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
             OV2650GetSensorID(pFeatureReturnPara32); 
            break;   
        default:
            break;
    }
    return ERROR_NONE;
}   /* OV2650FeatureControl() */

SENSOR_FUNCTION_STRUCT    SensorFuncOV2650=
{
  OV2650Open,
  OV2650GetInfo,
  OV2650GetResolution,
  OV2650FeatureControl,
  OV2650Control,
  OV2650Close
};

UINT32 OV2650SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{

  /* To Do : Check Sensor status here */
  if (pfFunc!=NULL)
      *pfFunc=&SensorFuncOV2650;

  return ERROR_NONE;
} /* SensorInit() */

