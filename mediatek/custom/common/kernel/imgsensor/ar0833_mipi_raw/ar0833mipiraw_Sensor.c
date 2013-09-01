/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.c
 *
 * Project:
 * --------
 *   YUSU
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author:
 * -------
 *   Kenny Chuang (MTK04214)
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
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/xlog.h>
#include <asm/atomic.h>
#include <linux/slab.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ar0833mipiraw_Sensor.h"
#include "ar0833mipiraw_Camera_Sensor_para.h"
#include "ar0833mipiraw_CameraCustomized.h"


///kk test


/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[ar0833mipiraw_sensor]"

static DEFINE_SPINLOCK(ar0833mipiraw_drv_lock);


#define AR0833_DEBUG

#ifdef AR0833_DEBUG
#define AR0833DB(fmt, arg...)           xlog_printk(ANDROID_LOG_DEBUG, "[AR0833MipiRaw] ",  fmt, ##arg)
#else
#define AR0833DB(fmt, arg...)
#endif


#ifdef AR0833_DEBUG_SOFIA
	#define AR0833DBSOFIA(fmt, arg...)  xlog_printk(ANDROID_LOG_DEBUG, "[AR0833MipiRaw] ",  fmt, ##arg)
#else
	#define AR0833DBSOFIA(fmt, arg...)
#endif




//
/// E2 chip need to mark define "AR0833_RUN_RAW8"
#define AR0833_RUN_RAW8


#ifndef AR0833_RUN_RAW8
#define RSE3264x2448_30FPS_MCLK24_4LANE_RAW10                    1
#define RSE3264x1836_30FPS_MCLK24_4LANE_RAW10                    0
#define RSE1616x1212_30FPS_MCLK24_4LANE_RAW10                    1
#define RSE3264x2448_30FPS_MCLK24_4LANE_RAW8                     0
#define RSE1616x1212_30FPS_MCLK24_4LANE_RAW8                     0

#else
#define RSE3264x2448_30FPS_MCLK24_4LANE_RAW10                    0
#define RSE3264x1836_30FPS_MCLK24_4LANE_RAW10                    0
#define RSE1616x1212_30FPS_MCLK24_4LANE_RAW10                    0
#define RSE3264x2448_30FPS_MCLK24_4LANE_RAW8                     1
#define RSE1616x1212_30FPS_MCLK24_4LANE_RAW8                     1
#endif
//





struct AR0833_SENSOR_STRUCT AR0833_sensor=
{
    .i2c_write_id = 0x6c,
    .i2c_read_id  = 0x6d,

#ifdef MIPI_INTERFACE
    .preview_vt_clk = 1040,  //52M
    .capture_vt_clk = 1040,
#else
    .preview_vt_clk = 520,  //52M
    .capture_vt_clk = 520,
#endif
};

#define DELAY_TAG   0xFFFF  /* ar0833 do not use register 0xffff */

#define Sleep(ms) mdelay(ms)


kal_uint32 AR0833_FeatureControl_PERIOD_PixelNum=AR0833_PV_PERIOD_PIXEL_NUMS;
kal_uint32 AR0833_FeatureControl_PERIOD_LineNum=AR0833_PV_PERIOD_LINE_NUMS;


typedef struct _SENSOR_INIT_INFO
{
    kal_uint16 address;
    kal_uint16 data;
}SOCSensorInitInfo;



kal_uint16 AR0833_dummy_pixels=0, AR0833_dummy_lines=0;
kal_uint16 AR0833_PV_dummy_pixels=0,AR0833_PV_dummy_lines=0;

kal_uint16 AR0833_exposure_lines=0x100;
kal_uint16 AR0833_sensor_global_gain=BASEGAIN, AR0833_sensor_gain_base=BASEGAIN;
kal_uint16 AR0833_sensor_gain_array[2][5]={{0x0204,0x0208, 0x0206, 0x020C, 0x020A},{0x08,0x8, 0x8, 0x8, 0x8}};



static AR0833_PARA_STRUCT ar0833;



MSDK_SENSOR_CONFIG_STRUCT AR0833_SensorConfigData;
kal_uint32 AR0833_FAC_SENSOR_REG;

/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT AR0833SensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT AR0833SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/
MSDK_SCENARIO_ID_ENUM AR0833CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);


kal_uint16 AR0833_read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;

    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

    iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,AR0833_sensor.i2c_write_id);

    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}



void AR0833_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{

    char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};

    iWriteRegI2C(puSendCmd , 4, AR0833_sensor.i2c_write_id);
}


kal_uint16 AR0833_read_cmos_sensor_8(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,AR0833_sensor.i2c_write_id);
    return get_byte;
}

void AR0833_write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para)
{

    char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};

    iWriteRegI2C(puSendCmd , 3, AR0833_sensor.i2c_write_id);
}

#define INF_POSITION 0
#define MACRO_POSITION 255

void AR0833_FOCUS_Init(void)
{
    AR0833_write_cmos_sensor(0x30F4, 0x0000);   // vcm_step_time (R/W)
	//Programmable counter to define how many system clocks for each step time.
	//vcm step time = Tsysclk * 16 * (vcm_step_time[15:0] + 1)
	//Legal values: [0, 65535].

    AR0833_write_cmos_sensor(0x30F0, 0x8000);   // vcm_control (R/W)
    //bit15: en,
    //bit3:VCM is not disabled in standby state,
    //bit2~0:vcm_slew=0: mode 0, refresh the code directly to target code
	//       vcm_slew>0: mode 1, increment/decrement 1 code every step transition time to target code step transition time = Tsysclk
}

void AR0833_FOCUS_Move_to(UINT16 a_u2MovePosition)
{
	if (a_u2MovePosition > MACRO_POSITION)   {a_u2MovePosition = MACRO_POSITION;}
	if (a_u2MovePosition < INF_POSITION)     {a_u2MovePosition = INF_POSITION;}

    AR0833_write_cmos_sensor(0x30F2, (kal_uint32)a_u2MovePosition);   // vcm_new_code (R/W)
	// New target code to VCM DAC.
	//Legal values: [0, 255].
}

void AR0833_FOCUS_Get_AF_Status(UINT32 *pFeatureReturnPara32)
{
	*pFeatureReturnPara32 = 1;
}

void AR0833_FOCUS_Get_AF_Inf(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = INF_POSITION;
}

void AR0833_FOCUS_Get_AF_Macro(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = MACRO_POSITION;
}


/*************************************************************************
* FUNCTION
*    read_AR0833_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 read_AR0833_gain(void)
{

    volatile signed char i;
    kal_uint16 temp_reg=0, sensor_gain=0,temp_reg_base=0;

    temp_reg_base=AR0833SensorCCT[SENSOR_BASEGAIN].Para;

    for(i=0;i<4;i++) {
        temp_reg=AR0833SensorCCT[PRE_GAIN_R_INDEX+i].Para;

        if(temp_reg>=0x08 && temp_reg<=0x78)  // 0x78 is 15 by 8 ,means max gain is 15 multiple
            AR0833_sensor_gain_array[1][PRE_GAIN_R_INDEX+i]=((((temp_reg*BASEGAIN)/8)*temp_reg_base)/8); //change to MTK basegain
        else if(temp_reg>0x78)
            AR0833DB("error gain setting");
    }

    sensor_gain=(temp_reg_base*BASEGAIN)/8;

    AR0833DB("read_AR0833_gain sensor_gain=0x%x\n",sensor_gain);

    return sensor_gain;   //mtk gain unit
}  /* read_AR0833_gain */

/*******************************************************************************
*
********************************************************************************/
void write_AR0833_gain(kal_uint16 gain)
{

    kal_uint16 reg_gain;

    AR0833DB("write_AR0833_gain gain=0x%x\n",gain);

    AR0833_write_cmos_sensor_8(0x0104, 0x01);       //parameter_hold
    if(gain >= BASEGAIN && gain <= 15*BASEGAIN) {
        reg_gain = 8 * gain/BASEGAIN;        //change mtk gain base to aptina gain base
        AR0833_write_cmos_sensor(0x0204,reg_gain);

       // AR0833DB("reg_gain =%d,gain = %d \n",reg_gain, gain);
    }
    else
        AR0833DB("error gain setting");
    AR0833_write_cmos_sensor_8(0x0104, 0x00);       //parameter_hold
}

/*************************************************************************
* FUNCTION
* set_AR0833_gain
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
kal_uint16 AR0833_Set_gain(kal_uint16 gain)
{
      write_AR0833_gain(gain);
      return 0;
}

/*******************************************************************************
*
********************************************************************************/
void AR0833_camera_para_to_sensor(void)
{

    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=AR0833SensorReg[i].Addr; i++)
    {
        AR0833_write_cmos_sensor(AR0833SensorReg[i].Addr, AR0833SensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=AR0833SensorReg[i].Addr; i++)
    {
        AR0833_write_cmos_sensor(AR0833SensorReg[i].Addr, AR0833SensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        AR0833_write_cmos_sensor(AR0833SensorCCT[i].Addr, AR0833SensorCCT[i].Para);
    }

}


/*************************************************************************
* FUNCTION
*    AR0833_sensor_to_camera_para
*
* DESCRIPTION
*    // update camera_para from sensor register
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
void AR0833_sensor_to_camera_para(void)
{

    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=AR0833SensorReg[i].Addr; i++)
    {
         temp_data = AR0833_read_cmos_sensor(AR0833SensorReg[i].Addr);
		 spin_lock(&ar0833mipiraw_drv_lock);
		 AR0833SensorReg[i].Para =temp_data;
		 spin_unlock(&ar0833mipiraw_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=AR0833SensorReg[i].Addr; i++)
    {
        temp_data = AR0833_read_cmos_sensor(AR0833SensorReg[i].Addr);
		spin_lock(&ar0833mipiraw_drv_lock);
		AR0833SensorReg[i].Para = temp_data;
		spin_unlock(&ar0833mipiraw_drv_lock);
    }

}


/*************************************************************************
* FUNCTION
*    AR0833_get_sensor_group_count
*
* DESCRIPTION
*    //
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_int32  AR0833_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void AR0833_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void AR0833_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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

            temp_para=AR0833SensorCCT[temp_addr].Para;

           if(temp_para>=0x08 && temp_para<=0x78)
                temp_gain=(temp_para*BASEGAIN)/8;
            else
                ASSERT(0);

            temp_gain=(temp_gain*1000)/BASEGAIN;

            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min=1000;
            info_ptr->Max=15000;
            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");

                    //temp_reg=AR0833SensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=    111;  //AR0833_MAX_EXPOSURE_LINES;
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



kal_bool AR0833_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{

   kal_uint16  temp_gain=0,temp_addr=0, temp_para=0;

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

            temp_gain=((ItemValue*BASEGAIN+500)/1000);            //+500:get closed integer value

          if(temp_gain>=1*BASEGAIN && temp_gain<=15*BASEGAIN)
          {
             temp_para=(temp_gain*8+BASEGAIN/2)/BASEGAIN;
          }
          else
              ASSERT(0);

            AR0833SensorCCT[temp_addr].Para = temp_para;
            AR0833_write_cmos_sensor(AR0833SensorCCT[temp_addr].Addr,temp_para);

           AR0833_sensor_gain_base=read_AR0833_gain();

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    //no need to apply this item for driving current
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
                    AR0833_FAC_SENSOR_REG=ItemValue;
                    break;
                case 1:
                    AR0833_write_cmos_sensor(AR0833_FAC_SENSOR_REG,ItemValue);
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


/*******************************************************************************
*
********************************************************************************/
#if RSE1616x1212_30FPS_MCLK24_4LANE_RAW8
void Func_RSE3264x2448_30FPS_MCLK24_4LANE_RAW8(void)
{

    AR0833DB("Func_RSE3264x2448_30FPS_MCLK24_4LANE_RAW8\n");

    AR0833_write_cmos_sensor(0x301A, 0x0018);   // RESET_REGISTER
    AR0833_write_cmos_sensor(0x0346, 0x0008);   // Y_ADDR_START
    AR0833_write_cmos_sensor(0x034A, 0x0997);   // Y_ADDR_END
    AR0833_write_cmos_sensor(0x0344, 0x0008);   // X_ADDR_START
    AR0833_write_cmos_sensor(0x0348, 0x0CC7);   // X_ADDR_END
    AR0833_write_cmos_sensor(0x034C, 0x0CC0);   // X_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x034E, 0x0990);   // Y_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x3040, 0x4041);   // READ_MODE
    AR0833_write_cmos_sensor(0x0342, 0x0FCC);   // LINE_LENGTH_PCK
    AR0833_write_cmos_sensor(0x0340, 0x0A01);   // FRAME_LENGTH_LINES
    AR0833_write_cmos_sensor(0x0202, 0x0A00);   // COARSE_INTEGRATION_TIME
    AR0833_write_cmos_sensor(0x301A, 0x001C);   // RESET_REGISTER



#if 0   //for test pattern
    AR0833_write_cmos_sensor(0x3070, 0x0003);
#endif


}
#endif

#if RSE1616x1212_30FPS_MCLK24_4LANE_RAW8
void Func_RSE1616x1212_30FPS_MCLK24_4LANE_RAW8(void)
{

    AR0833DB("Func_RSE1616x1212_30FPS_MCLK24_4LANE_RAW8");

    AR0833_write_cmos_sensor(0x301A, 0x0018);   // RESET_REGISTER
    AR0833_write_cmos_sensor(0x0346, 0x0008);   // Y_ADDR_START
    AR0833_write_cmos_sensor(0x034A, 0x0995);   // Y_ADDR_END
    AR0833_write_cmos_sensor(0x0344, 0x0008);   // X_ADDR_START
    AR0833_write_cmos_sensor(0x0348, 0x0CC5);   // X_ADDR_END
    AR0833_write_cmos_sensor(0x034C, 0x0660);   // X_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x034E, 0x04C8);   // Y_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x3040, 0x48C3);   // READ_MODE
    AR0833_write_cmos_sensor(0x0342, 0x0FCC);   // LINE_LENGTH_PCK
    AR0833_write_cmos_sensor(0x0340, 0x0A09);   // FRAME_LENGTH_LINES


    AR0833_write_cmos_sensor(0x0202, 0x0530);   // COARSE_INTEGRATION_TIME
    AR0833_write_cmos_sensor(0x301A, 0x001C);   // RESET_REGISTER

#if 0   //for test pattern
    AR0833_write_cmos_sensor(0x3070, 0x0003);
#endif

}
#endif

#ifdef RSE1616x1212_30FPS_MCLK24_4LANE_RAW10
void Func_RSE1616x1212_30FPS_MCLK24_4LANE_RAW10(void)
{
    AR0833DB("Func_RSE1616x1212_30FPS_MCLK24_4LANE_RAW10");

    AR0833_write_cmos_sensor(0x301A, 0x0019); 	// RESET_REGISTER
    Sleep(100);
    AR0833_write_cmos_sensor(0x301A, 0x0218); 	// RESET_REGISTER
    AR0833_write_cmos_sensor(0x3042, 0x0000); 	// DARK_CONTROL2
    AR0833_write_cmos_sensor(0x30C0, 0x1810); 	// CALIB_CONTROL
    AR0833_write_cmos_sensor(0x30C8, 0x0018); 	// CALIB_DAC
    AR0833_write_cmos_sensor(0x30D2, 0x0000); 	// CRM_CONTROL
    AR0833_write_cmos_sensor(0x30D4, 0xD030); 	// COLUMN_CORRECTION
    AR0833_write_cmos_sensor(0x30D6, 0x2200); 	// COLUMN_CORRECTION2
    AR0833_write_cmos_sensor(0x30DA, 0x0080); 	// COLUMN_CORRECTION_CLIP2
    AR0833_write_cmos_sensor(0x30DC, 0x0080); 	// COLUMN_CORRECTION_CLIP3
    AR0833_write_cmos_sensor(0x30EE, 0x0340); 	// DARK_CONTROL3
    AR0833_write_cmos_sensor(0x316A, 0x8800); 	// DAC_RSTLO
    AR0833_write_cmos_sensor(0x316C, 0x8200); 	// DAC_TXLO
    AR0833_write_cmos_sensor(0x3172, 0x0286); 	// ANALOG_CONTROL2
    AR0833_write_cmos_sensor(0x3174, 0x8000); 	// ANALOG_CONTROL3
    AR0833_write_cmos_sensor(0x317C, 0xE103); 	// ANALOG_CONTROL7
    AR0833_write_cmos_sensor(0x3180, 0xF0FF); 	// FINEDIGCORR_CONTROL
    AR0833_write_cmos_sensor(0x31E0, 0x0741); 	// PIX_DEF_ID
    AR0833_write_cmos_sensor(0x3ECC, 0x0056); 	// DAC_LD_0_1
    AR0833_write_cmos_sensor(0x3ED0, 0xA8AA); 	// DAC_LD_4_5
    AR0833_write_cmos_sensor(0x3ED2, 0xAAA8); 	// DAC_LD_6_7
    AR0833_write_cmos_sensor(0x3ED4, 0x8ACC); 	// DAC_LD_8_9
    AR0833_write_cmos_sensor(0x3ED8, 0x7288); 	// DAC_LD_12_13
    AR0833_write_cmos_sensor(0x3EDA, 0x77CA); 	// DAC_LD_14_15
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5); 	// DAC_LD_20_21
    AR0833_write_cmos_sensor(0x3EE4, 0x1548); 	// DAC_LD_24_25
    AR0833_write_cmos_sensor(0x3EE6, 0xB10C); 	// DAC_LD_26_27
    AR0833_write_cmos_sensor(0x3EE8, 0x6E79); 	// DAC_LD_28_29
    AR0833_write_cmos_sensor(0x3EFE, 0x77CC); 	// DAC_LD_TXLO
    AR0833_write_cmos_sensor(0x31E6, 0x0000); 	// PIX_DEF_ID_2
    AR0833_write_cmos_sensor(0x3F00, 0x0028); 	// BM_T0
    AR0833_write_cmos_sensor(0x3F02, 0x0140); 	// BM_T1
    AR0833_write_cmos_sensor(0x3F04, 0x0002); 	// NOISE_GAIN_THRESHOLD0
    AR0833_write_cmos_sensor(0x3F06, 0x0004); 	// NOISE_GAIN_THRESHOLD1
    AR0833_write_cmos_sensor(0x3F08, 0x0008); 	// NOISE_GAIN_THRESHOLD2
    AR0833_write_cmos_sensor(0x3F0A, 0x0B09); 	// NOISE_FLOOR10
    AR0833_write_cmos_sensor(0x3F0C, 0x0302); 	// NOISE_FLOOR32
    AR0833_write_cmos_sensor(0x3F10, 0x0505); 	// SINGLE_K_FACTOR0
    AR0833_write_cmos_sensor(0x3F12, 0x0303); 	// SINGLE_K_FACTOR1
    AR0833_write_cmos_sensor(0x3F14, 0x0101); 	// SINGLE_K_FACTOR2
    AR0833_write_cmos_sensor(0x3F16, 0x2020); 	// CROSSFACTOR0
    AR0833_write_cmos_sensor(0x3F18, 0x0404); 	// CROSSFACTOR1
    AR0833_write_cmos_sensor(0x3F1A, 0x7070); 	// CROSSFACTOR2
    AR0833_write_cmos_sensor(0x3F1C, 0x003A); 	// SINGLE_MAXFACTOR
    AR0833_write_cmos_sensor(0x3F1E, 0x003C); 	// NOISE_COEF
    AR0833_write_cmos_sensor(0x3F2C, 0x2210); 	// GTH_THRES_RTN
    AR0833_write_cmos_sensor(0x3F40, 0x2020); 	// COUPLE_K_FACTOR0
    AR0833_write_cmos_sensor(0x3F42, 0x0808); 	// COUPLE_K_FACTOR1
    AR0833_write_cmos_sensor(0x3F44, 0x0101); 	// COUPLE_K_FACTOR2
    Sleep(100);

    AR0833_write_cmos_sensor_8(0x3D00, 0x04);
    AR0833_write_cmos_sensor_8(0x3D01, 0x70);
    AR0833_write_cmos_sensor_8(0x3D02, 0xC8);
    AR0833_write_cmos_sensor_8(0x3D03, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D04, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D05, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D06, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D07, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D08, 0x6F);
    AR0833_write_cmos_sensor_8(0x3D09, 0x40);
    AR0833_write_cmos_sensor_8(0x3D0A, 0x14);
    AR0833_write_cmos_sensor_8(0x3D0B, 0x0E);
    AR0833_write_cmos_sensor_8(0x3D0C, 0x23);
    AR0833_write_cmos_sensor_8(0x3D0D, 0x82);
    AR0833_write_cmos_sensor_8(0x3D0E, 0x41);
    AR0833_write_cmos_sensor_8(0x3D0F, 0x5C);
    AR0833_write_cmos_sensor_8(0x3D10, 0x54);
    AR0833_write_cmos_sensor_8(0x3D11, 0x6E);
    AR0833_write_cmos_sensor_8(0x3D12, 0x42);
    AR0833_write_cmos_sensor_8(0x3D13, 0x00);
    AR0833_write_cmos_sensor_8(0x3D14, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D15, 0x5D);
    AR0833_write_cmos_sensor_8(0x3D16, 0x80);
    AR0833_write_cmos_sensor_8(0x3D17, 0x5A);
    AR0833_write_cmos_sensor_8(0x3D18, 0x80);
    AR0833_write_cmos_sensor_8(0x3D19, 0x57);
    AR0833_write_cmos_sensor_8(0x3D1A, 0x84);
    AR0833_write_cmos_sensor_8(0x3D1B, 0x64);
    AR0833_write_cmos_sensor_8(0x3D1C, 0x80);
    AR0833_write_cmos_sensor_8(0x3D1D, 0x55);
    AR0833_write_cmos_sensor_8(0x3D1E, 0x86);
    AR0833_write_cmos_sensor_8(0x3D1F, 0x64);
    AR0833_write_cmos_sensor_8(0x3D20, 0x80);
    AR0833_write_cmos_sensor_8(0x3D21, 0x65);
    AR0833_write_cmos_sensor_8(0x3D22, 0x88);
    AR0833_write_cmos_sensor_8(0x3D23, 0x65);
    AR0833_write_cmos_sensor_8(0x3D24, 0x84);
    AR0833_write_cmos_sensor_8(0x3D25, 0x58);
    AR0833_write_cmos_sensor_8(0x3D26, 0x80);
    AR0833_write_cmos_sensor_8(0x3D27, 0x00);
    AR0833_write_cmos_sensor_8(0x3D28, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D29, 0x80);
    AR0833_write_cmos_sensor_8(0x3D2A, 0x30);
    AR0833_write_cmos_sensor_8(0x3D2B, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D2C, 0x84);
    AR0833_write_cmos_sensor_8(0x3D2D, 0x42);
    AR0833_write_cmos_sensor_8(0x3D2E, 0x82);
    AR0833_write_cmos_sensor_8(0x3D2F, 0x10);
    AR0833_write_cmos_sensor_8(0x3D30, 0x30);
    AR0833_write_cmos_sensor_8(0x3D31, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D32, 0x5B);
    AR0833_write_cmos_sensor_8(0x3D33, 0x80);
    AR0833_write_cmos_sensor_8(0x3D34, 0x63);
    AR0833_write_cmos_sensor_8(0x3D35, 0x8B);
    AR0833_write_cmos_sensor_8(0x3D36, 0x30);
    AR0833_write_cmos_sensor_8(0x3D37, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D38, 0xA5);
    AR0833_write_cmos_sensor_8(0x3D39, 0x59);
    AR0833_write_cmos_sensor_8(0x3D3A, 0x84);
    AR0833_write_cmos_sensor_8(0x3D3B, 0x6C);
    AR0833_write_cmos_sensor_8(0x3D3C, 0x80);
    AR0833_write_cmos_sensor_8(0x3D3D, 0x6D);
    AR0833_write_cmos_sensor_8(0x3D3E, 0x81);
    AR0833_write_cmos_sensor_8(0x3D3F, 0x5F);
    AR0833_write_cmos_sensor_8(0x3D40, 0x60);
    AR0833_write_cmos_sensor_8(0x3D41, 0x61);
    AR0833_write_cmos_sensor_8(0x3D42, 0x10);
    AR0833_write_cmos_sensor_8(0x3D43, 0x30);
    AR0833_write_cmos_sensor_8(0x3D44, 0x88);
    AR0833_write_cmos_sensor_8(0x3D45, 0x66);
    AR0833_write_cmos_sensor_8(0x3D46, 0x83);
    AR0833_write_cmos_sensor_8(0x3D47, 0x6E);
    AR0833_write_cmos_sensor_8(0x3D48, 0x80);
    AR0833_write_cmos_sensor_8(0x3D49, 0x64);
    AR0833_write_cmos_sensor_8(0x3D4A, 0x87);
    AR0833_write_cmos_sensor_8(0x3D4B, 0x64);
    AR0833_write_cmos_sensor_8(0x3D4C, 0x30);
    AR0833_write_cmos_sensor_8(0x3D4D, 0x50);
    AR0833_write_cmos_sensor_8(0x3D4E, 0xDA);
    AR0833_write_cmos_sensor_8(0x3D4F, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D50, 0x83);
    AR0833_write_cmos_sensor_8(0x3D51, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D52, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D53, 0x30);
    AR0833_write_cmos_sensor_8(0x3D54, 0x94);
    AR0833_write_cmos_sensor_8(0x3D55, 0x67);
    AR0833_write_cmos_sensor_8(0x3D56, 0x84);
    AR0833_write_cmos_sensor_8(0x3D57, 0x65);
    AR0833_write_cmos_sensor_8(0x3D58, 0x82);
    AR0833_write_cmos_sensor_8(0x3D59, 0x4D);
    AR0833_write_cmos_sensor_8(0x3D5A, 0x83);
    AR0833_write_cmos_sensor_8(0x3D5B, 0x65);
    AR0833_write_cmos_sensor_8(0x3D5C, 0x30);
    AR0833_write_cmos_sensor_8(0x3D5D, 0x50);
    AR0833_write_cmos_sensor_8(0x3D5E, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D5F, 0x58);
    AR0833_write_cmos_sensor_8(0x3D60, 0x43);
    AR0833_write_cmos_sensor_8(0x3D61, 0x06);
    AR0833_write_cmos_sensor_8(0x3D62, 0x00);
    AR0833_write_cmos_sensor_8(0x3D63, 0x8D);
    AR0833_write_cmos_sensor_8(0x3D64, 0x45);
    AR0833_write_cmos_sensor_8(0x3D65, 0xA0);
    AR0833_write_cmos_sensor_8(0x3D66, 0x45);
    AR0833_write_cmos_sensor_8(0x3D67, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D68, 0x83);
    AR0833_write_cmos_sensor_8(0x3D69, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D6A, 0x06);
    AR0833_write_cmos_sensor_8(0x3D6B, 0x00);
    AR0833_write_cmos_sensor_8(0x3D6C, 0x81);
    AR0833_write_cmos_sensor_8(0x3D6D, 0x43);
    AR0833_write_cmos_sensor_8(0x3D6E, 0x9C);
    AR0833_write_cmos_sensor_8(0x3D6F, 0x58);
    AR0833_write_cmos_sensor_8(0x3D70, 0x84);
    AR0833_write_cmos_sensor_8(0x3D71, 0x30);
    AR0833_write_cmos_sensor_8(0x3D72, 0x90);
    AR0833_write_cmos_sensor_8(0x3D73, 0x67);
    AR0833_write_cmos_sensor_8(0x3D74, 0x64);
    AR0833_write_cmos_sensor_8(0x3D75, 0x88);
    AR0833_write_cmos_sensor_8(0x3D76, 0x64);
    AR0833_write_cmos_sensor_8(0x3D77, 0x80);
    AR0833_write_cmos_sensor_8(0x3D78, 0x65);
    AR0833_write_cmos_sensor_8(0x3D79, 0x88);
    AR0833_write_cmos_sensor_8(0x3D7A, 0x65);
    AR0833_write_cmos_sensor_8(0x3D7B, 0x82);
    AR0833_write_cmos_sensor_8(0x3D7C, 0x10);
    AR0833_write_cmos_sensor_8(0x3D7D, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D7E, 0xEB);
    AR0833_write_cmos_sensor_8(0x3D7F, 0x10);
    AR0833_write_cmos_sensor_8(0x3D80, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D81, 0x66);
    AR0833_write_cmos_sensor_8(0x3D82, 0x85);
    AR0833_write_cmos_sensor_8(0x3D83, 0x64);
    AR0833_write_cmos_sensor_8(0x3D84, 0x81);
    AR0833_write_cmos_sensor_8(0x3D85, 0x17);
    AR0833_write_cmos_sensor_8(0x3D86, 0x00);
    AR0833_write_cmos_sensor_8(0x3D87, 0x80);
    AR0833_write_cmos_sensor_8(0x3D88, 0x20);
    AR0833_write_cmos_sensor_8(0x3D89, 0x0D);
    AR0833_write_cmos_sensor_8(0x3D8A, 0x80);
    AR0833_write_cmos_sensor_8(0x3D8B, 0x18);
    AR0833_write_cmos_sensor_8(0x3D8C, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D8D, 0x80);
    AR0833_write_cmos_sensor_8(0x3D8E, 0x64);
    AR0833_write_cmos_sensor_8(0x3D8F, 0x30);
    AR0833_write_cmos_sensor_8(0x3D90, 0x60);
    AR0833_write_cmos_sensor_8(0x3D91, 0x41);
    AR0833_write_cmos_sensor_8(0x3D92, 0x82);
    AR0833_write_cmos_sensor_8(0x3D93, 0x42);
    AR0833_write_cmos_sensor_8(0x3D94, 0xB2);
    AR0833_write_cmos_sensor_8(0x3D95, 0x42);
    AR0833_write_cmos_sensor_8(0x3D96, 0x80);
    AR0833_write_cmos_sensor_8(0x3D97, 0x40);
    AR0833_write_cmos_sensor_8(0x3D98, 0x81);
    AR0833_write_cmos_sensor_8(0x3D99, 0x40);
    AR0833_write_cmos_sensor_8(0x3D9A, 0x80);
    AR0833_write_cmos_sensor_8(0x3D9B, 0x41);
    AR0833_write_cmos_sensor_8(0x3D9C, 0x80);
    AR0833_write_cmos_sensor_8(0x3D9D, 0x42);
    AR0833_write_cmos_sensor_8(0x3D9E, 0x80);
    AR0833_write_cmos_sensor_8(0x3D9F, 0x43);
    AR0833_write_cmos_sensor_8(0x3DA0, 0x83);
    AR0833_write_cmos_sensor_8(0x3DA1, 0x06);
    AR0833_write_cmos_sensor_8(0x3DA2, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DA3, 0x88);
    AR0833_write_cmos_sensor_8(0x3DA4, 0x44);
    AR0833_write_cmos_sensor_8(0x3DA5, 0x87);
    AR0833_write_cmos_sensor_8(0x3DA6, 0x6A);
    AR0833_write_cmos_sensor_8(0x3DA7, 0x83);
    AR0833_write_cmos_sensor_8(0x3DA8, 0x6B);
    AR0833_write_cmos_sensor_8(0x3DA9, 0x92);
    AR0833_write_cmos_sensor_8(0x3DAA, 0x44);
    AR0833_write_cmos_sensor_8(0x3DAB, 0x88);
    AR0833_write_cmos_sensor_8(0x3DAC, 0x06);
    AR0833_write_cmos_sensor_8(0x3DAD, 0xC8);
    AR0833_write_cmos_sensor_8(0x3DAE, 0x81);
    AR0833_write_cmos_sensor_8(0x3DAF, 0x41);
    AR0833_write_cmos_sensor_8(0x3DB0, 0x85);
    AR0833_write_cmos_sensor_8(0x3DB1, 0x30);
    AR0833_write_cmos_sensor_8(0x3DB2, 0xA4);
    AR0833_write_cmos_sensor_8(0x3DB3, 0x67);
    AR0833_write_cmos_sensor_8(0x3DB4, 0x85);
    AR0833_write_cmos_sensor_8(0x3DB5, 0x65);
    AR0833_write_cmos_sensor_8(0x3DB6, 0x87);
    AR0833_write_cmos_sensor_8(0x3DB7, 0x65);
    AR0833_write_cmos_sensor_8(0x3DB8, 0x30);
    AR0833_write_cmos_sensor_8(0x3DB9, 0x60);
    AR0833_write_cmos_sensor_8(0x3DBA, 0x8D);
    AR0833_write_cmos_sensor_8(0x3DBB, 0x42);
    AR0833_write_cmos_sensor_8(0x3DBC, 0x82);
    AR0833_write_cmos_sensor_8(0x3DBD, 0x40);
    AR0833_write_cmos_sensor_8(0x3DBE, 0x82);
    AR0833_write_cmos_sensor_8(0x3DBF, 0x40);
    AR0833_write_cmos_sensor_8(0x3DC0, 0x80);
    AR0833_write_cmos_sensor_8(0x3DC1, 0x41);
    AR0833_write_cmos_sensor_8(0x3DC2, 0x80);
    AR0833_write_cmos_sensor_8(0x3DC3, 0x42);
    AR0833_write_cmos_sensor_8(0x3DC4, 0x80);
    AR0833_write_cmos_sensor_8(0x3DC5, 0x43);
    AR0833_write_cmos_sensor_8(0x3DC6, 0x83);
    AR0833_write_cmos_sensor_8(0x3DC7, 0x06);
    AR0833_write_cmos_sensor_8(0x3DC8, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DC9, 0x88);
    AR0833_write_cmos_sensor_8(0x3DCA, 0x44);
    AR0833_write_cmos_sensor_8(0x3DCB, 0x9C);
    AR0833_write_cmos_sensor_8(0x3DCC, 0x44);
    AR0833_write_cmos_sensor_8(0x3DCD, 0x88);
    AR0833_write_cmos_sensor_8(0x3DCE, 0x06);
    AR0833_write_cmos_sensor_8(0x3DCF, 0xC8);
    AR0833_write_cmos_sensor_8(0x3DD0, 0x85);
    AR0833_write_cmos_sensor_8(0x3DD1, 0x41);
    AR0833_write_cmos_sensor_8(0x3DD2, 0x6A);
    AR0833_write_cmos_sensor_8(0x3DD3, 0x83);
    AR0833_write_cmos_sensor_8(0x3DD4, 0x6B);
    AR0833_write_cmos_sensor_8(0x3DD5, 0xA0);
    AR0833_write_cmos_sensor_8(0x3DD6, 0x42);
    AR0833_write_cmos_sensor_8(0x3DD7, 0x82);
    AR0833_write_cmos_sensor_8(0x3DD8, 0x40);
    AR0833_write_cmos_sensor_8(0x3DD9, 0x6C);
    AR0833_write_cmos_sensor_8(0x3DDA, 0x3A);
    AR0833_write_cmos_sensor_8(0x3DDB, 0xA8);
    AR0833_write_cmos_sensor_8(0x3DDC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DDD, 0x28);
    AR0833_write_cmos_sensor_8(0x3DDE, 0x30);
    AR0833_write_cmos_sensor_8(0x3DDF, 0x70);
    AR0833_write_cmos_sensor_8(0x3DE0, 0x6F);
    AR0833_write_cmos_sensor_8(0x3DE1, 0x40);
    AR0833_write_cmos_sensor_8(0x3DE2, 0x14);
    AR0833_write_cmos_sensor_8(0x3DE3, 0x0E);
    AR0833_write_cmos_sensor_8(0x3DE4, 0x23);
    AR0833_write_cmos_sensor_8(0x3DE5, 0xC2);
    AR0833_write_cmos_sensor_8(0x3DE6, 0x41);
    AR0833_write_cmos_sensor_8(0x3DE7, 0x82);
    AR0833_write_cmos_sensor_8(0x3DE8, 0x42);
    AR0833_write_cmos_sensor_8(0x3DE9, 0x00);
    AR0833_write_cmos_sensor_8(0x3DEA, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DEB, 0x5D);
    AR0833_write_cmos_sensor_8(0x3DEC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DED, 0x5A);
    AR0833_write_cmos_sensor_8(0x3DEE, 0x80);
    AR0833_write_cmos_sensor_8(0x3DEF, 0x57);
    AR0833_write_cmos_sensor_8(0x3DF0, 0x84);
    AR0833_write_cmos_sensor_8(0x3DF1, 0x64);
    AR0833_write_cmos_sensor_8(0x3DF2, 0x80);
    AR0833_write_cmos_sensor_8(0x3DF3, 0x55);
    AR0833_write_cmos_sensor_8(0x3DF4, 0x86);
    AR0833_write_cmos_sensor_8(0x3DF5, 0x64);
    AR0833_write_cmos_sensor_8(0x3DF6, 0x80);
    AR0833_write_cmos_sensor_8(0x3DF7, 0x65);
    AR0833_write_cmos_sensor_8(0x3DF8, 0x88);
    AR0833_write_cmos_sensor_8(0x3DF9, 0x65);
    AR0833_write_cmos_sensor_8(0x3DFA, 0x82);
    AR0833_write_cmos_sensor_8(0x3DFB, 0x54);
    AR0833_write_cmos_sensor_8(0x3DFC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DFD, 0x58);
    AR0833_write_cmos_sensor_8(0x3DFE, 0x80);
    AR0833_write_cmos_sensor_8(0x3DFF, 0x00);
    AR0833_write_cmos_sensor_8(0x3E00, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E01, 0x86);
    AR0833_write_cmos_sensor_8(0x3E02, 0x42);
    AR0833_write_cmos_sensor_8(0x3E03, 0x82);
    AR0833_write_cmos_sensor_8(0x3E04, 0x10);
    AR0833_write_cmos_sensor_8(0x3E05, 0x30);
    AR0833_write_cmos_sensor_8(0x3E06, 0x9C);
    AR0833_write_cmos_sensor_8(0x3E07, 0x5C);
    AR0833_write_cmos_sensor_8(0x3E08, 0x80);
    AR0833_write_cmos_sensor_8(0x3E09, 0x6E);
    AR0833_write_cmos_sensor_8(0x3E0A, 0x86);
    AR0833_write_cmos_sensor_8(0x3E0B, 0x5B);
    AR0833_write_cmos_sensor_8(0x3E0C, 0x80);
    AR0833_write_cmos_sensor_8(0x3E0D, 0x63);
    AR0833_write_cmos_sensor_8(0x3E0E, 0x9E);
    AR0833_write_cmos_sensor_8(0x3E0F, 0x59);
    AR0833_write_cmos_sensor_8(0x3E10, 0x8C);
    AR0833_write_cmos_sensor_8(0x3E11, 0x5E);
    AR0833_write_cmos_sensor_8(0x3E12, 0x8A);
    AR0833_write_cmos_sensor_8(0x3E13, 0x6C);
    AR0833_write_cmos_sensor_8(0x3E14, 0x80);
    AR0833_write_cmos_sensor_8(0x3E15, 0x6D);
    AR0833_write_cmos_sensor_8(0x3E16, 0x81);
    AR0833_write_cmos_sensor_8(0x3E17, 0x5F);
    AR0833_write_cmos_sensor_8(0x3E18, 0x60);
    AR0833_write_cmos_sensor_8(0x3E19, 0x61);
    AR0833_write_cmos_sensor_8(0x3E1A, 0x88);
    AR0833_write_cmos_sensor_8(0x3E1B, 0x10);
    AR0833_write_cmos_sensor_8(0x3E1C, 0x30);
    AR0833_write_cmos_sensor_8(0x3E1D, 0x66);
    AR0833_write_cmos_sensor_8(0x3E1E, 0x83);
    AR0833_write_cmos_sensor_8(0x3E1F, 0x6E);
    AR0833_write_cmos_sensor_8(0x3E20, 0x80);
    AR0833_write_cmos_sensor_8(0x3E21, 0x64);
    AR0833_write_cmos_sensor_8(0x3E22, 0x87);
    AR0833_write_cmos_sensor_8(0x3E23, 0x64);
    AR0833_write_cmos_sensor_8(0x3E24, 0x30);
    AR0833_write_cmos_sensor_8(0x3E25, 0x50);
    AR0833_write_cmos_sensor_8(0x3E26, 0xD3);
    AR0833_write_cmos_sensor_8(0x3E27, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E28, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E29, 0xAD);
    AR0833_write_cmos_sensor_8(0x3E2A, 0x30);
    AR0833_write_cmos_sensor_8(0x3E2B, 0x94);
    AR0833_write_cmos_sensor_8(0x3E2C, 0x67);
    AR0833_write_cmos_sensor_8(0x3E2D, 0x84);
    AR0833_write_cmos_sensor_8(0x3E2E, 0x65);
    AR0833_write_cmos_sensor_8(0x3E2F, 0x82);
    AR0833_write_cmos_sensor_8(0x3E30, 0x4D);
    AR0833_write_cmos_sensor_8(0x3E31, 0x83);
    AR0833_write_cmos_sensor_8(0x3E32, 0x65);
    AR0833_write_cmos_sensor_8(0x3E33, 0x30);
    AR0833_write_cmos_sensor_8(0x3E34, 0x50);
    AR0833_write_cmos_sensor_8(0x3E35, 0xA7);
    AR0833_write_cmos_sensor_8(0x3E36, 0x43);
    AR0833_write_cmos_sensor_8(0x3E37, 0x06);
    AR0833_write_cmos_sensor_8(0x3E38, 0x00);
    AR0833_write_cmos_sensor_8(0x3E39, 0x8D);
    AR0833_write_cmos_sensor_8(0x3E3A, 0x45);
    AR0833_write_cmos_sensor_8(0x3E3B, 0x9A);
    AR0833_write_cmos_sensor_8(0x3E3C, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E3D, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E3E, 0x45);
    AR0833_write_cmos_sensor_8(0x3E3F, 0x85);
    AR0833_write_cmos_sensor_8(0x3E40, 0x06);
    AR0833_write_cmos_sensor_8(0x3E41, 0x00);
    AR0833_write_cmos_sensor_8(0x3E42, 0x81);
    AR0833_write_cmos_sensor_8(0x3E43, 0x43);
    AR0833_write_cmos_sensor_8(0x3E44, 0x8A);
    AR0833_write_cmos_sensor_8(0x3E45, 0x6F);
    AR0833_write_cmos_sensor_8(0x3E46, 0x96);
    AR0833_write_cmos_sensor_8(0x3E47, 0x30);
    AR0833_write_cmos_sensor_8(0x3E48, 0x90);
    AR0833_write_cmos_sensor_8(0x3E49, 0x67);
    AR0833_write_cmos_sensor_8(0x3E4A, 0x64);
    AR0833_write_cmos_sensor_8(0x3E4B, 0x88);
    AR0833_write_cmos_sensor_8(0x3E4C, 0x64);
    AR0833_write_cmos_sensor_8(0x3E4D, 0x80);
    AR0833_write_cmos_sensor_8(0x3E4E, 0x65);
    AR0833_write_cmos_sensor_8(0x3E4F, 0x82);
    AR0833_write_cmos_sensor_8(0x3E50, 0x10);
    AR0833_write_cmos_sensor_8(0x3E51, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E52, 0x84);
    AR0833_write_cmos_sensor_8(0x3E53, 0x65);
    AR0833_write_cmos_sensor_8(0x3E54, 0xEF);
    AR0833_write_cmos_sensor_8(0x3E55, 0x10);
    AR0833_write_cmos_sensor_8(0x3E56, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E57, 0x66);
    AR0833_write_cmos_sensor_8(0x3E58, 0x85);
    AR0833_write_cmos_sensor_8(0x3E59, 0x64);
    AR0833_write_cmos_sensor_8(0x3E5A, 0x81);
    AR0833_write_cmos_sensor_8(0x3E5B, 0x17);
    AR0833_write_cmos_sensor_8(0x3E5C, 0x00);
    AR0833_write_cmos_sensor_8(0x3E5D, 0x80);
    AR0833_write_cmos_sensor_8(0x3E5E, 0x20);
    AR0833_write_cmos_sensor_8(0x3E5F, 0x0D);
    AR0833_write_cmos_sensor_8(0x3E60, 0x80);
    AR0833_write_cmos_sensor_8(0x3E61, 0x18);
    AR0833_write_cmos_sensor_8(0x3E62, 0x0C);
    AR0833_write_cmos_sensor_8(0x3E63, 0x80);
    AR0833_write_cmos_sensor_8(0x3E64, 0x64);
    AR0833_write_cmos_sensor_8(0x3E65, 0x30);
    AR0833_write_cmos_sensor_8(0x3E66, 0x60);
    AR0833_write_cmos_sensor_8(0x3E67, 0x41);
    AR0833_write_cmos_sensor_8(0x3E68, 0x82);
    AR0833_write_cmos_sensor_8(0x3E69, 0x42);
    AR0833_write_cmos_sensor_8(0x3E6A, 0xB2);
    AR0833_write_cmos_sensor_8(0x3E6B, 0x42);
    AR0833_write_cmos_sensor_8(0x3E6C, 0x80);
    AR0833_write_cmos_sensor_8(0x3E6D, 0x40);
    AR0833_write_cmos_sensor_8(0x3E6E, 0x82);
    AR0833_write_cmos_sensor_8(0x3E6F, 0x40);
    AR0833_write_cmos_sensor_8(0x3E70, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E71, 0x45);
    AR0833_write_cmos_sensor_8(0x3E72, 0x92);
    AR0833_write_cmos_sensor_8(0x3E73, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E74, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E75, 0x9B);
    AR0833_write_cmos_sensor_8(0x3E76, 0x45);
    AR0833_write_cmos_sensor_8(0x3E77, 0x81);
    AR0833_write_cmos_sensor_8(0x3E78, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E79, 0x40);
    AR0833_write_cmos_sensor_8(0x3E7A, 0x8C);
    AR0833_write_cmos_sensor_8(0x3E7B, 0x30);
    AR0833_write_cmos_sensor_8(0x3E7C, 0xA4);
    AR0833_write_cmos_sensor_8(0x3E7D, 0x67);
    AR0833_write_cmos_sensor_8(0x3E7E, 0x85);
    AR0833_write_cmos_sensor_8(0x3E7F, 0x65);
    AR0833_write_cmos_sensor_8(0x3E80, 0x87);
    AR0833_write_cmos_sensor_8(0x3E81, 0x65);
    AR0833_write_cmos_sensor_8(0x3E82, 0x30);
    AR0833_write_cmos_sensor_8(0x3E83, 0x60);
    AR0833_write_cmos_sensor_8(0x3E84, 0xD3);
    AR0833_write_cmos_sensor_8(0x3E85, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E86, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E87, 0xAC);
    AR0833_write_cmos_sensor_8(0x3E88, 0x6C);
    AR0833_write_cmos_sensor_8(0x3E89, 0x32);
    AR0833_write_cmos_sensor_8(0x3E8A, 0xA8);
    AR0833_write_cmos_sensor_8(0x3E8B, 0x80);
    AR0833_write_cmos_sensor_8(0x3E8C, 0x28);
    AR0833_write_cmos_sensor_8(0x3E8D, 0x30);
    AR0833_write_cmos_sensor_8(0x3E8E, 0x70);
    AR0833_write_cmos_sensor_8(0x3E8F, 0x00);
    AR0833_write_cmos_sensor_8(0x3E90, 0x80);
    AR0833_write_cmos_sensor_8(0x3E91, 0x40);
    AR0833_write_cmos_sensor_8(0x3E92, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E93, 0xBD);
    AR0833_write_cmos_sensor_8(0x3E94, 0x00);
    AR0833_write_cmos_sensor_8(0x3E95, 0x0E);
    AR0833_write_cmos_sensor_8(0x3E96, 0xBE);
    AR0833_write_cmos_sensor_8(0x3E97, 0x44);
    AR0833_write_cmos_sensor_8(0x3E98, 0x88);
    AR0833_write_cmos_sensor_8(0x3E99, 0x44);
    AR0833_write_cmos_sensor_8(0x3E9A, 0xBC);
    AR0833_write_cmos_sensor_8(0x3E9B, 0x78);
    AR0833_write_cmos_sensor_8(0x3E9C, 0x09);
    AR0833_write_cmos_sensor_8(0x3E9D, 0x00);
    AR0833_write_cmos_sensor_8(0x3E9E, 0x89);
    AR0833_write_cmos_sensor_8(0x3E9F, 0x04);
    AR0833_write_cmos_sensor_8(0x3EA0, 0x80);
    AR0833_write_cmos_sensor_8(0x3EA1, 0x80);
    AR0833_write_cmos_sensor_8(0x3EA2, 0x02);
    AR0833_write_cmos_sensor_8(0x3EA3, 0x40);
    AR0833_write_cmos_sensor_8(0x3EA4, 0x86);
    AR0833_write_cmos_sensor_8(0x3EA5, 0x09);
    AR0833_write_cmos_sensor_8(0x3EA6, 0x00);
    AR0833_write_cmos_sensor_8(0x3EA7, 0x8E);
    AR0833_write_cmos_sensor_8(0x3EA8, 0x09);
    AR0833_write_cmos_sensor_8(0x3EA9, 0x00);
    AR0833_write_cmos_sensor_8(0x3EAA, 0x80);
    AR0833_write_cmos_sensor_8(0x3EAB, 0x02);
    AR0833_write_cmos_sensor_8(0x3EAC, 0x40);
    AR0833_write_cmos_sensor_8(0x3EAD, 0x80);
    AR0833_write_cmos_sensor_8(0x3EAE, 0x04);
    AR0833_write_cmos_sensor_8(0x3EAF, 0x80);
    AR0833_write_cmos_sensor_8(0x3EB0, 0x88);
    AR0833_write_cmos_sensor_8(0x3EB1, 0x7D);
    AR0833_write_cmos_sensor_8(0x3EB2, 0x9E);
    AR0833_write_cmos_sensor_8(0x3EB3, 0x86);
    AR0833_write_cmos_sensor_8(0x3EB4, 0x09);
    AR0833_write_cmos_sensor_8(0x3EB5, 0x00);
    AR0833_write_cmos_sensor_8(0x3EB6, 0x87);
    AR0833_write_cmos_sensor_8(0x3EB7, 0x7A);
    AR0833_write_cmos_sensor_8(0x3EB8, 0x00);
    AR0833_write_cmos_sensor_8(0x3EB9, 0x0E);
    AR0833_write_cmos_sensor_8(0x3EBA, 0xC3);
    AR0833_write_cmos_sensor_8(0x3EBB, 0x79);
    AR0833_write_cmos_sensor_8(0x3EBC, 0x4C);
    AR0833_write_cmos_sensor_8(0x3EBD, 0x40);
    AR0833_write_cmos_sensor_8(0x3EBE, 0xBF);
    AR0833_write_cmos_sensor_8(0x3EBF, 0x70);
    AR0833_write_cmos_sensor_8(0x3EC0, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC1, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC2, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC3, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC4, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC5, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC6, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC7, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC8, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC9, 0x00);
    AR0833_write_cmos_sensor_8(0x3ECA, 0x00);
    AR0833_write_cmos_sensor_8(0x3ECB, 0x00);


    AR0833_write_cmos_sensor(0x301A, 0x0018); 	// RESET_REGISTER
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5); 	// DAC_LD_20_21
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5); 	// DAC_LD_20_21
    AR0833_write_cmos_sensor(0x301A, 0x001C); 	// RESET_REGISTER
    AR0833_write_cmos_sensor(0x0300, 0x0005); 	// VT_PIX_CLK_DIV
    AR0833_write_cmos_sensor(0x0302, 0x0001); 	// VT_SYS_CLK_DIV
    AR0833_write_cmos_sensor(0x0304, 0x0004); 	// PRE_PLL_CLK_DIV
    AR0833_write_cmos_sensor(0x0306, 0x007A); 	// PLL_MULTIPLIER
    AR0833_write_cmos_sensor(0x0308, 0x000A); 	// OP_PIX_CLK_DIV
    AR0833_write_cmos_sensor(0x030A, 0x0001); 	// OP_SYS_CLK_DIV
    AR0833_write_cmos_sensor(0x3064, 0x7800); 	// SMIA_TEST
    Sleep(1);
    AR0833_write_cmos_sensor(0x31B0, 0x0060); 	// FRAME_PREAMBLE
    AR0833_write_cmos_sensor(0x31B2, 0x0042); 	// LINE_PREAMBLE
    AR0833_write_cmos_sensor(0x31B4, 0x1C36); 	// MIPI_TIMING_0
    AR0833_write_cmos_sensor(0x31B6, 0x5218); 	// MIPI_TIMING_1
    AR0833_write_cmos_sensor(0x31B8, 0x404A); 	// MIPI_TIMING_2
    AR0833_write_cmos_sensor(0x31BA, 0x028A); 	// MIPI_TIMING_3
    AR0833_write_cmos_sensor(0x31BC, 0x0008); 	// MIPI_TIMING_4
    Sleep(1);
    AR0833_write_cmos_sensor(0x0342, 0x0ECE); 	// LINE_LENGTH_PCK
    AR0833_write_cmos_sensor(0x0340, 0x0A0F); 	// FRAME_LENGTH_LINES
    AR0833_write_cmos_sensor(0x0202, 0x0A0F); 	// COARSE_INTEGRATION_TIME
    AR0833_write_cmos_sensor(0x0344, 0x0008); 	// X_ADDR_START
    AR0833_write_cmos_sensor(0x0348, 0x0CC7); 	// X_ADDR_END
    AR0833_write_cmos_sensor(0x0346, 0x0008); 	// Y_ADDR_START
    AR0833_write_cmos_sensor(0x034A, 0x0997); 	// Y_ADDR_END
    AR0833_write_cmos_sensor(0x034C, 0x0650); 	// X_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x034E, 0x04BC); 	// Y_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x3040, 0x4041); 	// READ_MODE
    AR0833_write_cmos_sensor(0x0400, 0x0002); 	// SCALING_MODE
    AR0833_write_cmos_sensor(0x0402, 0x0000); 	// SPATIAL_SAMPLING
    AR0833_write_cmos_sensor(0x0404, 0x0020); 	// SCALE_M
    AR0833_write_cmos_sensor(0x0408, 0x0208); 	// SECOND_RESIDUAL
    AR0833_write_cmos_sensor(0x040A, 0x00C7); 	// SECOND_CROP
    AR0833_write_cmos_sensor(0x306E, 0x9090); 	// DATA_PATH_SELECT
    AR0833_write_cmos_sensor(0x301A, 0x001C); 	// RESET_REGISTER


}
#endif

#if RSE3264x1836_30FPS_MCLK24_4LANE_RAW10
static void Func_RSE3264x1836_30FPS_MCLK24_4LANE_RAW10(void)
{
    AR0833DB("Func_RSE3264x1836_30FPS_MCLK24_4LANE_RAW10");


    AR0833_write_cmos_sensor(0x301A, 0x0019); 	// RESET_REGISTER
    Sleep(100);
    AR0833_write_cmos_sensor(0x301A, 0x0218); 	// RESET_REGISTER
    AR0833_write_cmos_sensor(0x3042, 0x0000); 	// DARK_CONTROL2
    AR0833_write_cmos_sensor(0x30C0, 0x1810); 	// CALIB_CONTROL
    AR0833_write_cmos_sensor(0x30C8, 0x0018); 	// CALIB_DAC
    AR0833_write_cmos_sensor(0x30D2, 0x0000); 	// CRM_CONTROL
    AR0833_write_cmos_sensor(0x30D4, 0xD030); 	// COLUMN_CORRECTION
    AR0833_write_cmos_sensor(0x30D6, 0x2200); 	// COLUMN_CORRECTION2
    AR0833_write_cmos_sensor(0x30DA, 0x0080); 	// COLUMN_CORRECTION_CLIP2
    AR0833_write_cmos_sensor(0x30DC, 0x0080); 	// COLUMN_CORRECTION_CLIP3
    AR0833_write_cmos_sensor(0x30EE, 0x0340); 	// DARK_CONTROL3
    AR0833_write_cmos_sensor(0x316A, 0x8800); 	// DAC_RSTLO
    AR0833_write_cmos_sensor(0x316C, 0x8200); 	// DAC_TXLO
    AR0833_write_cmos_sensor(0x3172, 0x0286); 	// ANALOG_CONTROL2
    AR0833_write_cmos_sensor(0x3174, 0x8000); 	// ANALOG_CONTROL3
    AR0833_write_cmos_sensor(0x317C, 0xE103); 	// ANALOG_CONTROL7
    AR0833_write_cmos_sensor(0x3180, 0xF0FF); 	// FINEDIGCORR_CONTROL
    AR0833_write_cmos_sensor(0x31E0, 0x0741); 	// PIX_DEF_ID
    AR0833_write_cmos_sensor(0x3ECC, 0x0056); 	// DAC_LD_0_1
    AR0833_write_cmos_sensor(0x3ED0, 0xA8AA); 	// DAC_LD_4_5
    AR0833_write_cmos_sensor(0x3ED2, 0xAAA8); 	// DAC_LD_6_7
    AR0833_write_cmos_sensor(0x3ED4, 0x8ACC); 	// DAC_LD_8_9
    AR0833_write_cmos_sensor(0x3ED8, 0x7288); 	// DAC_LD_12_13
    AR0833_write_cmos_sensor(0x3EDA, 0x77CA); 	// DAC_LD_14_15
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5); 	// DAC_LD_20_21
    AR0833_write_cmos_sensor(0x3EE4, 0x1548); 	// DAC_LD_24_25
    AR0833_write_cmos_sensor(0x3EE6, 0xB10C); 	// DAC_LD_26_27
    AR0833_write_cmos_sensor(0x3EE8, 0x6E79); 	// DAC_LD_28_29
    AR0833_write_cmos_sensor(0x3EFE, 0x77CC); 	// DAC_LD_TXLO
    AR0833_write_cmos_sensor(0x31E6, 0x0000); 	// PIX_DEF_ID_2
    AR0833_write_cmos_sensor(0x3F00, 0x0028); 	// BM_T0
    AR0833_write_cmos_sensor(0x3F02, 0x0140); 	// BM_T1
    AR0833_write_cmos_sensor(0x3F04, 0x0002); 	// NOISE_GAIN_THRESHOLD0
    AR0833_write_cmos_sensor(0x3F06, 0x0004); 	// NOISE_GAIN_THRESHOLD1
    AR0833_write_cmos_sensor(0x3F08, 0x0008); 	// NOISE_GAIN_THRESHOLD2
    AR0833_write_cmos_sensor(0x3F0A, 0x0B09); 	// NOISE_FLOOR10
    AR0833_write_cmos_sensor(0x3F0C, 0x0302); 	// NOISE_FLOOR32
    AR0833_write_cmos_sensor(0x3F10, 0x0505); 	// SINGLE_K_FACTOR0
    AR0833_write_cmos_sensor(0x3F12, 0x0303); 	// SINGLE_K_FACTOR1
    AR0833_write_cmos_sensor(0x3F14, 0x0101); 	// SINGLE_K_FACTOR2
    AR0833_write_cmos_sensor(0x3F16, 0x2020); 	// CROSSFACTOR0
    AR0833_write_cmos_sensor(0x3F18, 0x0404); 	// CROSSFACTOR1
    AR0833_write_cmos_sensor(0x3F1A, 0x7070); 	// CROSSFACTOR2
    AR0833_write_cmos_sensor(0x3F1C, 0x003A); 	// SINGLE_MAXFACTOR
    AR0833_write_cmos_sensor(0x3F1E, 0x003C); 	// NOISE_COEF
    AR0833_write_cmos_sensor(0x3F2C, 0x2210); 	// GTH_THRES_RTN
    AR0833_write_cmos_sensor(0x3F40, 0x2020); 	// COUPLE_K_FACTOR0
    AR0833_write_cmos_sensor(0x3F42, 0x0808); 	// COUPLE_K_FACTOR1
    AR0833_write_cmos_sensor(0x3F44, 0x0101); 	// COUPLE_K_FACTOR2
    Sleep(100);

    AR0833_write_cmos_sensor_8(0x3D00, 0x04);
    AR0833_write_cmos_sensor_8(0x3D01, 0x70);
    AR0833_write_cmos_sensor_8(0x3D02, 0xC8);
    AR0833_write_cmos_sensor_8(0x3D03, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D04, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D05, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D06, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D07, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D08, 0x6F);
    AR0833_write_cmos_sensor_8(0x3D09, 0x40);
    AR0833_write_cmos_sensor_8(0x3D0A, 0x14);
    AR0833_write_cmos_sensor_8(0x3D0B, 0x0E);
    AR0833_write_cmos_sensor_8(0x3D0C, 0x23);
    AR0833_write_cmos_sensor_8(0x3D0D, 0x82);
    AR0833_write_cmos_sensor_8(0x3D0E, 0x41);
    AR0833_write_cmos_sensor_8(0x3D0F, 0x5C);
    AR0833_write_cmos_sensor_8(0x3D10, 0x54);
    AR0833_write_cmos_sensor_8(0x3D11, 0x6E);
    AR0833_write_cmos_sensor_8(0x3D12, 0x42);
    AR0833_write_cmos_sensor_8(0x3D13, 0x00);
    AR0833_write_cmos_sensor_8(0x3D14, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D15, 0x5D);
    AR0833_write_cmos_sensor_8(0x3D16, 0x80);
    AR0833_write_cmos_sensor_8(0x3D17, 0x5A);
    AR0833_write_cmos_sensor_8(0x3D18, 0x80);
    AR0833_write_cmos_sensor_8(0x3D19, 0x57);
    AR0833_write_cmos_sensor_8(0x3D1A, 0x84);
    AR0833_write_cmos_sensor_8(0x3D1B, 0x64);
    AR0833_write_cmos_sensor_8(0x3D1C, 0x80);
    AR0833_write_cmos_sensor_8(0x3D1D, 0x55);
    AR0833_write_cmos_sensor_8(0x3D1E, 0x86);
    AR0833_write_cmos_sensor_8(0x3D1F, 0x64);
    AR0833_write_cmos_sensor_8(0x3D20, 0x80);
    AR0833_write_cmos_sensor_8(0x3D21, 0x65);
    AR0833_write_cmos_sensor_8(0x3D22, 0x88);
    AR0833_write_cmos_sensor_8(0x3D23, 0x65);
    AR0833_write_cmos_sensor_8(0x3D24, 0x84);
    AR0833_write_cmos_sensor_8(0x3D25, 0x58);
    AR0833_write_cmos_sensor_8(0x3D26, 0x80);
    AR0833_write_cmos_sensor_8(0x3D27, 0x00);
    AR0833_write_cmos_sensor_8(0x3D28, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D29, 0x80);
    AR0833_write_cmos_sensor_8(0x3D2A, 0x30);
    AR0833_write_cmos_sensor_8(0x3D2B, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D2C, 0x84);
    AR0833_write_cmos_sensor_8(0x3D2D, 0x42);
    AR0833_write_cmos_sensor_8(0x3D2E, 0x82);
    AR0833_write_cmos_sensor_8(0x3D2F, 0x10);
    AR0833_write_cmos_sensor_8(0x3D30, 0x30);
    AR0833_write_cmos_sensor_8(0x3D31, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D32, 0x5B);
    AR0833_write_cmos_sensor_8(0x3D33, 0x80);
    AR0833_write_cmos_sensor_8(0x3D34, 0x63);
    AR0833_write_cmos_sensor_8(0x3D35, 0x8B);
    AR0833_write_cmos_sensor_8(0x3D36, 0x30);
    AR0833_write_cmos_sensor_8(0x3D37, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D38, 0xA5);
    AR0833_write_cmos_sensor_8(0x3D39, 0x59);
    AR0833_write_cmos_sensor_8(0x3D3A, 0x84);
    AR0833_write_cmos_sensor_8(0x3D3B, 0x6C);
    AR0833_write_cmos_sensor_8(0x3D3C, 0x80);
    AR0833_write_cmos_sensor_8(0x3D3D, 0x6D);
    AR0833_write_cmos_sensor_8(0x3D3E, 0x81);
    AR0833_write_cmos_sensor_8(0x3D3F, 0x5F);
    AR0833_write_cmos_sensor_8(0x3D40, 0x60);
    AR0833_write_cmos_sensor_8(0x3D41, 0x61);
    AR0833_write_cmos_sensor_8(0x3D42, 0x10);
    AR0833_write_cmos_sensor_8(0x3D43, 0x30);
    AR0833_write_cmos_sensor_8(0x3D44, 0x88);
    AR0833_write_cmos_sensor_8(0x3D45, 0x66);
    AR0833_write_cmos_sensor_8(0x3D46, 0x83);
    AR0833_write_cmos_sensor_8(0x3D47, 0x6E);
    AR0833_write_cmos_sensor_8(0x3D48, 0x80);
    AR0833_write_cmos_sensor_8(0x3D49, 0x64);
    AR0833_write_cmos_sensor_8(0x3D4A, 0x87);
    AR0833_write_cmos_sensor_8(0x3D4B, 0x64);
    AR0833_write_cmos_sensor_8(0x3D4C, 0x30);
    AR0833_write_cmos_sensor_8(0x3D4D, 0x50);
    AR0833_write_cmos_sensor_8(0x3D4E, 0xDA);
    AR0833_write_cmos_sensor_8(0x3D4F, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D50, 0x83);
    AR0833_write_cmos_sensor_8(0x3D51, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D52, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D53, 0x30);
    AR0833_write_cmos_sensor_8(0x3D54, 0x94);
    AR0833_write_cmos_sensor_8(0x3D55, 0x67);
    AR0833_write_cmos_sensor_8(0x3D56, 0x84);
    AR0833_write_cmos_sensor_8(0x3D57, 0x65);
    AR0833_write_cmos_sensor_8(0x3D58, 0x82);
    AR0833_write_cmos_sensor_8(0x3D59, 0x4D);
    AR0833_write_cmos_sensor_8(0x3D5A, 0x83);
    AR0833_write_cmos_sensor_8(0x3D5B, 0x65);
    AR0833_write_cmos_sensor_8(0x3D5C, 0x30);
    AR0833_write_cmos_sensor_8(0x3D5D, 0x50);
    AR0833_write_cmos_sensor_8(0x3D5E, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D5F, 0x58);
    AR0833_write_cmos_sensor_8(0x3D60, 0x43);
    AR0833_write_cmos_sensor_8(0x3D61, 0x06);
    AR0833_write_cmos_sensor_8(0x3D62, 0x00);
    AR0833_write_cmos_sensor_8(0x3D63, 0x8D);
    AR0833_write_cmos_sensor_8(0x3D64, 0x45);
    AR0833_write_cmos_sensor_8(0x3D65, 0xA0);
    AR0833_write_cmos_sensor_8(0x3D66, 0x45);
    AR0833_write_cmos_sensor_8(0x3D67, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D68, 0x83);
    AR0833_write_cmos_sensor_8(0x3D69, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D6A, 0x06);
    AR0833_write_cmos_sensor_8(0x3D6B, 0x00);
    AR0833_write_cmos_sensor_8(0x3D6C, 0x81);
    AR0833_write_cmos_sensor_8(0x3D6D, 0x43);
    AR0833_write_cmos_sensor_8(0x3D6E, 0x9C);
    AR0833_write_cmos_sensor_8(0x3D6F, 0x58);
    AR0833_write_cmos_sensor_8(0x3D70, 0x84);
    AR0833_write_cmos_sensor_8(0x3D71, 0x30);
    AR0833_write_cmos_sensor_8(0x3D72, 0x90);
    AR0833_write_cmos_sensor_8(0x3D73, 0x67);
    AR0833_write_cmos_sensor_8(0x3D74, 0x64);
    AR0833_write_cmos_sensor_8(0x3D75, 0x88);
    AR0833_write_cmos_sensor_8(0x3D76, 0x64);
    AR0833_write_cmos_sensor_8(0x3D77, 0x80);
    AR0833_write_cmos_sensor_8(0x3D78, 0x65);
    AR0833_write_cmos_sensor_8(0x3D79, 0x88);
    AR0833_write_cmos_sensor_8(0x3D7A, 0x65);
    AR0833_write_cmos_sensor_8(0x3D7B, 0x82);
    AR0833_write_cmos_sensor_8(0x3D7C, 0x10);
    AR0833_write_cmos_sensor_8(0x3D7D, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D7E, 0xEB);
    AR0833_write_cmos_sensor_8(0x3D7F, 0x10);
    AR0833_write_cmos_sensor_8(0x3D80, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D81, 0x66);
    AR0833_write_cmos_sensor_8(0x3D82, 0x85);
    AR0833_write_cmos_sensor_8(0x3D83, 0x64);
    AR0833_write_cmos_sensor_8(0x3D84, 0x81);
    AR0833_write_cmos_sensor_8(0x3D85, 0x17);
    AR0833_write_cmos_sensor_8(0x3D86, 0x00);
    AR0833_write_cmos_sensor_8(0x3D87, 0x80);
    AR0833_write_cmos_sensor_8(0x3D88, 0x20);
    AR0833_write_cmos_sensor_8(0x3D89, 0x0D);
    AR0833_write_cmos_sensor_8(0x3D8A, 0x80);
    AR0833_write_cmos_sensor_8(0x3D8B, 0x18);
    AR0833_write_cmos_sensor_8(0x3D8C, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D8D, 0x80);
    AR0833_write_cmos_sensor_8(0x3D8E, 0x64);
    AR0833_write_cmos_sensor_8(0x3D8F, 0x30);
    AR0833_write_cmos_sensor_8(0x3D90, 0x60);
    AR0833_write_cmos_sensor_8(0x3D91, 0x41);
    AR0833_write_cmos_sensor_8(0x3D92, 0x82);
    AR0833_write_cmos_sensor_8(0x3D93, 0x42);
    AR0833_write_cmos_sensor_8(0x3D94, 0xB2);
    AR0833_write_cmos_sensor_8(0x3D95, 0x42);
    AR0833_write_cmos_sensor_8(0x3D96, 0x80);
    AR0833_write_cmos_sensor_8(0x3D97, 0x40);
    AR0833_write_cmos_sensor_8(0x3D98, 0x81);
    AR0833_write_cmos_sensor_8(0x3D99, 0x40);
    AR0833_write_cmos_sensor_8(0x3D9A, 0x80);
    AR0833_write_cmos_sensor_8(0x3D9B, 0x41);
    AR0833_write_cmos_sensor_8(0x3D9C, 0x80);
    AR0833_write_cmos_sensor_8(0x3D9D, 0x42);
    AR0833_write_cmos_sensor_8(0x3D9E, 0x80);
    AR0833_write_cmos_sensor_8(0x3D9F, 0x43);
    AR0833_write_cmos_sensor_8(0x3DA0, 0x83);
    AR0833_write_cmos_sensor_8(0x3DA1, 0x06);
    AR0833_write_cmos_sensor_8(0x3DA2, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DA3, 0x88);
    AR0833_write_cmos_sensor_8(0x3DA4, 0x44);
    AR0833_write_cmos_sensor_8(0x3DA5, 0x87);
    AR0833_write_cmos_sensor_8(0x3DA6, 0x6A);
    AR0833_write_cmos_sensor_8(0x3DA7, 0x83);
    AR0833_write_cmos_sensor_8(0x3DA8, 0x6B);
    AR0833_write_cmos_sensor_8(0x3DA9, 0x92);
    AR0833_write_cmos_sensor_8(0x3DAA, 0x44);
    AR0833_write_cmos_sensor_8(0x3DAB, 0x88);
    AR0833_write_cmos_sensor_8(0x3DAC, 0x06);
    AR0833_write_cmos_sensor_8(0x3DAD, 0xC8);
    AR0833_write_cmos_sensor_8(0x3DAE, 0x81);
    AR0833_write_cmos_sensor_8(0x3DAF, 0x41);
    AR0833_write_cmos_sensor_8(0x3DB0, 0x85);
    AR0833_write_cmos_sensor_8(0x3DB1, 0x30);
    AR0833_write_cmos_sensor_8(0x3DB2, 0xA4);
    AR0833_write_cmos_sensor_8(0x3DB3, 0x67);
    AR0833_write_cmos_sensor_8(0x3DB4, 0x85);
    AR0833_write_cmos_sensor_8(0x3DB5, 0x65);
    AR0833_write_cmos_sensor_8(0x3DB6, 0x87);
    AR0833_write_cmos_sensor_8(0x3DB7, 0x65);
    AR0833_write_cmos_sensor_8(0x3DB8, 0x30);
    AR0833_write_cmos_sensor_8(0x3DB9, 0x60);
    AR0833_write_cmos_sensor_8(0x3DBA, 0x8D);
    AR0833_write_cmos_sensor_8(0x3DBB, 0x42);
    AR0833_write_cmos_sensor_8(0x3DBC, 0x82);
    AR0833_write_cmos_sensor_8(0x3DBD, 0x40);
    AR0833_write_cmos_sensor_8(0x3DBE, 0x82);
    AR0833_write_cmos_sensor_8(0x3DBF, 0x40);
    AR0833_write_cmos_sensor_8(0x3DC0, 0x80);
    AR0833_write_cmos_sensor_8(0x3DC1, 0x41);
    AR0833_write_cmos_sensor_8(0x3DC2, 0x80);
    AR0833_write_cmos_sensor_8(0x3DC3, 0x42);
    AR0833_write_cmos_sensor_8(0x3DC4, 0x80);
    AR0833_write_cmos_sensor_8(0x3DC5, 0x43);
    AR0833_write_cmos_sensor_8(0x3DC6, 0x83);
    AR0833_write_cmos_sensor_8(0x3DC7, 0x06);
    AR0833_write_cmos_sensor_8(0x3DC8, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DC9, 0x88);
    AR0833_write_cmos_sensor_8(0x3DCA, 0x44);
    AR0833_write_cmos_sensor_8(0x3DCB, 0x9C);
    AR0833_write_cmos_sensor_8(0x3DCC, 0x44);
    AR0833_write_cmos_sensor_8(0x3DCD, 0x88);
    AR0833_write_cmos_sensor_8(0x3DCE, 0x06);
    AR0833_write_cmos_sensor_8(0x3DCF, 0xC8);
    AR0833_write_cmos_sensor_8(0x3DD0, 0x85);
    AR0833_write_cmos_sensor_8(0x3DD1, 0x41);
    AR0833_write_cmos_sensor_8(0x3DD2, 0x6A);
    AR0833_write_cmos_sensor_8(0x3DD3, 0x83);
    AR0833_write_cmos_sensor_8(0x3DD4, 0x6B);
    AR0833_write_cmos_sensor_8(0x3DD5, 0xA0);
    AR0833_write_cmos_sensor_8(0x3DD6, 0x42);
    AR0833_write_cmos_sensor_8(0x3DD7, 0x82);
    AR0833_write_cmos_sensor_8(0x3DD8, 0x40);
    AR0833_write_cmos_sensor_8(0x3DD9, 0x6C);
    AR0833_write_cmos_sensor_8(0x3DDA, 0x3A);
    AR0833_write_cmos_sensor_8(0x3DDB, 0xA8);
    AR0833_write_cmos_sensor_8(0x3DDC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DDD, 0x28);
    AR0833_write_cmos_sensor_8(0x3DDE, 0x30);
    AR0833_write_cmos_sensor_8(0x3DDF, 0x70);
    AR0833_write_cmos_sensor_8(0x3DE0, 0x6F);
    AR0833_write_cmos_sensor_8(0x3DE1, 0x40);
    AR0833_write_cmos_sensor_8(0x3DE2, 0x14);
    AR0833_write_cmos_sensor_8(0x3DE3, 0x0E);
    AR0833_write_cmos_sensor_8(0x3DE4, 0x23);
    AR0833_write_cmos_sensor_8(0x3DE5, 0xC2);
    AR0833_write_cmos_sensor_8(0x3DE6, 0x41);
    AR0833_write_cmos_sensor_8(0x3DE7, 0x82);
    AR0833_write_cmos_sensor_8(0x3DE8, 0x42);
    AR0833_write_cmos_sensor_8(0x3DE9, 0x00);
    AR0833_write_cmos_sensor_8(0x3DEA, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DEB, 0x5D);
    AR0833_write_cmos_sensor_8(0x3DEC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DED, 0x5A);
    AR0833_write_cmos_sensor_8(0x3DEE, 0x80);
    AR0833_write_cmos_sensor_8(0x3DEF, 0x57);
    AR0833_write_cmos_sensor_8(0x3DF0, 0x84);
    AR0833_write_cmos_sensor_8(0x3DF1, 0x64);
    AR0833_write_cmos_sensor_8(0x3DF2, 0x80);
    AR0833_write_cmos_sensor_8(0x3DF3, 0x55);
    AR0833_write_cmos_sensor_8(0x3DF4, 0x86);
    AR0833_write_cmos_sensor_8(0x3DF5, 0x64);
    AR0833_write_cmos_sensor_8(0x3DF6, 0x80);
    AR0833_write_cmos_sensor_8(0x3DF7, 0x65);
    AR0833_write_cmos_sensor_8(0x3DF8, 0x88);
    AR0833_write_cmos_sensor_8(0x3DF9, 0x65);
    AR0833_write_cmos_sensor_8(0x3DFA, 0x82);
    AR0833_write_cmos_sensor_8(0x3DFB, 0x54);
    AR0833_write_cmos_sensor_8(0x3DFC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DFD, 0x58);
    AR0833_write_cmos_sensor_8(0x3DFE, 0x80);
    AR0833_write_cmos_sensor_8(0x3DFF, 0x00);
    AR0833_write_cmos_sensor_8(0x3E00, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E01, 0x86);
    AR0833_write_cmos_sensor_8(0x3E02, 0x42);
    AR0833_write_cmos_sensor_8(0x3E03, 0x82);
    AR0833_write_cmos_sensor_8(0x3E04, 0x10);
    AR0833_write_cmos_sensor_8(0x3E05, 0x30);
    AR0833_write_cmos_sensor_8(0x3E06, 0x9C);
    AR0833_write_cmos_sensor_8(0x3E07, 0x5C);
    AR0833_write_cmos_sensor_8(0x3E08, 0x80);
    AR0833_write_cmos_sensor_8(0x3E09, 0x6E);
    AR0833_write_cmos_sensor_8(0x3E0A, 0x86);
    AR0833_write_cmos_sensor_8(0x3E0B, 0x5B);
    AR0833_write_cmos_sensor_8(0x3E0C, 0x80);
    AR0833_write_cmos_sensor_8(0x3E0D, 0x63);
    AR0833_write_cmos_sensor_8(0x3E0E, 0x9E);
    AR0833_write_cmos_sensor_8(0x3E0F, 0x59);
    AR0833_write_cmos_sensor_8(0x3E10, 0x8C);
    AR0833_write_cmos_sensor_8(0x3E11, 0x5E);
    AR0833_write_cmos_sensor_8(0x3E12, 0x8A);
    AR0833_write_cmos_sensor_8(0x3E13, 0x6C);
    AR0833_write_cmos_sensor_8(0x3E14, 0x80);
    AR0833_write_cmos_sensor_8(0x3E15, 0x6D);
    AR0833_write_cmos_sensor_8(0x3E16, 0x81);
    AR0833_write_cmos_sensor_8(0x3E17, 0x5F);
    AR0833_write_cmos_sensor_8(0x3E18, 0x60);
    AR0833_write_cmos_sensor_8(0x3E19, 0x61);
    AR0833_write_cmos_sensor_8(0x3E1A, 0x88);
    AR0833_write_cmos_sensor_8(0x3E1B, 0x10);
    AR0833_write_cmos_sensor_8(0x3E1C, 0x30);
    AR0833_write_cmos_sensor_8(0x3E1D, 0x66);
    AR0833_write_cmos_sensor_8(0x3E1E, 0x83);
    AR0833_write_cmos_sensor_8(0x3E1F, 0x6E);
    AR0833_write_cmos_sensor_8(0x3E20, 0x80);
    AR0833_write_cmos_sensor_8(0x3E21, 0x64);
    AR0833_write_cmos_sensor_8(0x3E22, 0x87);
    AR0833_write_cmos_sensor_8(0x3E23, 0x64);
    AR0833_write_cmos_sensor_8(0x3E24, 0x30);
    AR0833_write_cmos_sensor_8(0x3E25, 0x50);
    AR0833_write_cmos_sensor_8(0x3E26, 0xD3);
    AR0833_write_cmos_sensor_8(0x3E27, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E28, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E29, 0xAD);
    AR0833_write_cmos_sensor_8(0x3E2A, 0x30);
    AR0833_write_cmos_sensor_8(0x3E2B, 0x94);
    AR0833_write_cmos_sensor_8(0x3E2C, 0x67);
    AR0833_write_cmos_sensor_8(0x3E2D, 0x84);
    AR0833_write_cmos_sensor_8(0x3E2E, 0x65);
    AR0833_write_cmos_sensor_8(0x3E2F, 0x82);
    AR0833_write_cmos_sensor_8(0x3E30, 0x4D);
    AR0833_write_cmos_sensor_8(0x3E31, 0x83);
    AR0833_write_cmos_sensor_8(0x3E32, 0x65);
    AR0833_write_cmos_sensor_8(0x3E33, 0x30);
    AR0833_write_cmos_sensor_8(0x3E34, 0x50);
    AR0833_write_cmos_sensor_8(0x3E35, 0xA7);
    AR0833_write_cmos_sensor_8(0x3E36, 0x43);
    AR0833_write_cmos_sensor_8(0x3E37, 0x06);
    AR0833_write_cmos_sensor_8(0x3E38, 0x00);
    AR0833_write_cmos_sensor_8(0x3E39, 0x8D);
    AR0833_write_cmos_sensor_8(0x3E3A, 0x45);
    AR0833_write_cmos_sensor_8(0x3E3B, 0x9A);
    AR0833_write_cmos_sensor_8(0x3E3C, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E3D, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E3E, 0x45);
    AR0833_write_cmos_sensor_8(0x3E3F, 0x85);
    AR0833_write_cmos_sensor_8(0x3E40, 0x06);
    AR0833_write_cmos_sensor_8(0x3E41, 0x00);
    AR0833_write_cmos_sensor_8(0x3E42, 0x81);
    AR0833_write_cmos_sensor_8(0x3E43, 0x43);
    AR0833_write_cmos_sensor_8(0x3E44, 0x8A);
    AR0833_write_cmos_sensor_8(0x3E45, 0x6F);
    AR0833_write_cmos_sensor_8(0x3E46, 0x96);
    AR0833_write_cmos_sensor_8(0x3E47, 0x30);
    AR0833_write_cmos_sensor_8(0x3E48, 0x90);
    AR0833_write_cmos_sensor_8(0x3E49, 0x67);
    AR0833_write_cmos_sensor_8(0x3E4A, 0x64);
    AR0833_write_cmos_sensor_8(0x3E4B, 0x88);
    AR0833_write_cmos_sensor_8(0x3E4C, 0x64);
    AR0833_write_cmos_sensor_8(0x3E4D, 0x80);
    AR0833_write_cmos_sensor_8(0x3E4E, 0x65);
    AR0833_write_cmos_sensor_8(0x3E4F, 0x82);
    AR0833_write_cmos_sensor_8(0x3E50, 0x10);
    AR0833_write_cmos_sensor_8(0x3E51, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E52, 0x84);
    AR0833_write_cmos_sensor_8(0x3E53, 0x65);
    AR0833_write_cmos_sensor_8(0x3E54, 0xEF);
    AR0833_write_cmos_sensor_8(0x3E55, 0x10);
    AR0833_write_cmos_sensor_8(0x3E56, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E57, 0x66);
    AR0833_write_cmos_sensor_8(0x3E58, 0x85);
    AR0833_write_cmos_sensor_8(0x3E59, 0x64);
    AR0833_write_cmos_sensor_8(0x3E5A, 0x81);
    AR0833_write_cmos_sensor_8(0x3E5B, 0x17);
    AR0833_write_cmos_sensor_8(0x3E5C, 0x00);
    AR0833_write_cmos_sensor_8(0x3E5D, 0x80);
    AR0833_write_cmos_sensor_8(0x3E5E, 0x20);
    AR0833_write_cmos_sensor_8(0x3E5F, 0x0D);
    AR0833_write_cmos_sensor_8(0x3E60, 0x80);
    AR0833_write_cmos_sensor_8(0x3E61, 0x18);
    AR0833_write_cmos_sensor_8(0x3E62, 0x0C);
    AR0833_write_cmos_sensor_8(0x3E63, 0x80);
    AR0833_write_cmos_sensor_8(0x3E64, 0x64);
    AR0833_write_cmos_sensor_8(0x3E65, 0x30);
    AR0833_write_cmos_sensor_8(0x3E66, 0x60);
    AR0833_write_cmos_sensor_8(0x3E67, 0x41);
    AR0833_write_cmos_sensor_8(0x3E68, 0x82);
    AR0833_write_cmos_sensor_8(0x3E69, 0x42);
    AR0833_write_cmos_sensor_8(0x3E6A, 0xB2);
    AR0833_write_cmos_sensor_8(0x3E6B, 0x42);
    AR0833_write_cmos_sensor_8(0x3E6C, 0x80);
    AR0833_write_cmos_sensor_8(0x3E6D, 0x40);
    AR0833_write_cmos_sensor_8(0x3E6E, 0x82);
    AR0833_write_cmos_sensor_8(0x3E6F, 0x40);
    AR0833_write_cmos_sensor_8(0x3E70, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E71, 0x45);
    AR0833_write_cmos_sensor_8(0x3E72, 0x92);
    AR0833_write_cmos_sensor_8(0x3E73, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E74, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E75, 0x9B);
    AR0833_write_cmos_sensor_8(0x3E76, 0x45);
    AR0833_write_cmos_sensor_8(0x3E77, 0x81);
    AR0833_write_cmos_sensor_8(0x3E78, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E79, 0x40);
    AR0833_write_cmos_sensor_8(0x3E7A, 0x8C);
    AR0833_write_cmos_sensor_8(0x3E7B, 0x30);
    AR0833_write_cmos_sensor_8(0x3E7C, 0xA4);
    AR0833_write_cmos_sensor_8(0x3E7D, 0x67);
    AR0833_write_cmos_sensor_8(0x3E7E, 0x85);
    AR0833_write_cmos_sensor_8(0x3E7F, 0x65);
    AR0833_write_cmos_sensor_8(0x3E80, 0x87);
    AR0833_write_cmos_sensor_8(0x3E81, 0x65);
    AR0833_write_cmos_sensor_8(0x3E82, 0x30);
    AR0833_write_cmos_sensor_8(0x3E83, 0x60);
    AR0833_write_cmos_sensor_8(0x3E84, 0xD3);
    AR0833_write_cmos_sensor_8(0x3E85, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E86, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E87, 0xAC);
    AR0833_write_cmos_sensor_8(0x3E88, 0x6C);
    AR0833_write_cmos_sensor_8(0x3E89, 0x32);
    AR0833_write_cmos_sensor_8(0x3E8A, 0xA8);
    AR0833_write_cmos_sensor_8(0x3E8B, 0x80);
    AR0833_write_cmos_sensor_8(0x3E8C, 0x28);
    AR0833_write_cmos_sensor_8(0x3E8D, 0x30);
    AR0833_write_cmos_sensor_8(0x3E8E, 0x70);
    AR0833_write_cmos_sensor_8(0x3E8F, 0x00);
    AR0833_write_cmos_sensor_8(0x3E90, 0x80);
    AR0833_write_cmos_sensor_8(0x3E91, 0x40);
    AR0833_write_cmos_sensor_8(0x3E92, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E93, 0xBD);
    AR0833_write_cmos_sensor_8(0x3E94, 0x00);
    AR0833_write_cmos_sensor_8(0x3E95, 0x0E);
    AR0833_write_cmos_sensor_8(0x3E96, 0xBE);
    AR0833_write_cmos_sensor_8(0x3E97, 0x44);
    AR0833_write_cmos_sensor_8(0x3E98, 0x88);
    AR0833_write_cmos_sensor_8(0x3E99, 0x44);
    AR0833_write_cmos_sensor_8(0x3E9A, 0xBC);
    AR0833_write_cmos_sensor_8(0x3E9B, 0x78);
    AR0833_write_cmos_sensor_8(0x3E9C, 0x09);
    AR0833_write_cmos_sensor_8(0x3E9D, 0x00);
    AR0833_write_cmos_sensor_8(0x3E9E, 0x89);
    AR0833_write_cmos_sensor_8(0x3E9F, 0x04);
    AR0833_write_cmos_sensor_8(0x3EA0, 0x80);
    AR0833_write_cmos_sensor_8(0x3EA1, 0x80);
    AR0833_write_cmos_sensor_8(0x3EA2, 0x02);
    AR0833_write_cmos_sensor_8(0x3EA3, 0x40);
    AR0833_write_cmos_sensor_8(0x3EA4, 0x86);
    AR0833_write_cmos_sensor_8(0x3EA5, 0x09);
    AR0833_write_cmos_sensor_8(0x3EA6, 0x00);
    AR0833_write_cmos_sensor_8(0x3EA7, 0x8E);
    AR0833_write_cmos_sensor_8(0x3EA8, 0x09);
    AR0833_write_cmos_sensor_8(0x3EA9, 0x00);
    AR0833_write_cmos_sensor_8(0x3EAA, 0x80);
    AR0833_write_cmos_sensor_8(0x3EAB, 0x02);
    AR0833_write_cmos_sensor_8(0x3EAC, 0x40);
    AR0833_write_cmos_sensor_8(0x3EAD, 0x80);
    AR0833_write_cmos_sensor_8(0x3EAE, 0x04);
    AR0833_write_cmos_sensor_8(0x3EAF, 0x80);
    AR0833_write_cmos_sensor_8(0x3EB0, 0x88);
    AR0833_write_cmos_sensor_8(0x3EB1, 0x7D);
    AR0833_write_cmos_sensor_8(0x3EB2, 0x9E);
    AR0833_write_cmos_sensor_8(0x3EB3, 0x86);
    AR0833_write_cmos_sensor_8(0x3EB4, 0x09);
    AR0833_write_cmos_sensor_8(0x3EB5, 0x00);
    AR0833_write_cmos_sensor_8(0x3EB6, 0x87);
    AR0833_write_cmos_sensor_8(0x3EB7, 0x7A);
    AR0833_write_cmos_sensor_8(0x3EB8, 0x00);
    AR0833_write_cmos_sensor_8(0x3EB9, 0x0E);
    AR0833_write_cmos_sensor_8(0x3EBA, 0xC3);
    AR0833_write_cmos_sensor_8(0x3EBB, 0x79);
    AR0833_write_cmos_sensor_8(0x3EBC, 0x4C);
    AR0833_write_cmos_sensor_8(0x3EBD, 0x40);
    AR0833_write_cmos_sensor_8(0x3EBE, 0xBF);
    AR0833_write_cmos_sensor_8(0x3EBF, 0x70);
    AR0833_write_cmos_sensor_8(0x3EC0, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC1, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC2, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC3, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC4, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC5, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC6, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC7, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC8, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC9, 0x00);
    AR0833_write_cmos_sensor_8(0x3ECA, 0x00);
    AR0833_write_cmos_sensor_8(0x3ECB, 0x00);

    AR0833_write_cmos_sensor(0x301A, 0x0018);   // RESET_REGISTER
    AR0833_write_cmos_sensor(0x3EDE, 0x6664);   // DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EDE, 0x6664);   // DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EDE, 0x6664);   // DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EDE, 0x6664);   // DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5);   // DAC_LD_20_21
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5);   // DAC_LD_20_21
    AR0833_write_cmos_sensor(0x301A, 0x001C);   // RESET_REGISTER
    AR0833_write_cmos_sensor(0x0300, 0x0005);   // VT_PIX_CLK_DIV
    AR0833_write_cmos_sensor(0x0302, 0x0001);   // VT_SYS_CLK_DIV
    AR0833_write_cmos_sensor(0x0304, 0x0004);   // PRE_PLL_CLK_DIV
    AR0833_write_cmos_sensor(0x0306, 0x007A);   // PLL_MULTIPLIER
    AR0833_write_cmos_sensor(0x0308, 0x000A);   // OP_PIX_CLK_DIV
    AR0833_write_cmos_sensor(0x030A, 0x0001);   // OP_SYS_CLK_DIV
    AR0833_write_cmos_sensor(0x3064, 0x7800);   // SMIA_TEST
    Sleep(1);
    AR0833_write_cmos_sensor(0x31B0, 0x0060);   // FRAME_PREAMBLE
    AR0833_write_cmos_sensor(0x31B2, 0x0042);   // LINE_PREAMBLE
    AR0833_write_cmos_sensor(0x31B4, 0x1C36);   // MIPI_TIMING_0
    AR0833_write_cmos_sensor(0x31B6, 0x5218);   // MIPI_TIMING_1
    AR0833_write_cmos_sensor(0x31B8, 0x404A);   // MIPI_TIMING_2
    AR0833_write_cmos_sensor(0x31BA, 0x028A);   // MIPI_TIMING_3
    AR0833_write_cmos_sensor(0x31BC, 0x0008);   // MIPI_TIMING_4
    Sleep(1);
    AR0833_write_cmos_sensor(0x0342, 0x0ECC);   // LINE_LENGTH_PCK
    AR0833_write_cmos_sensor(0x0340, 0x0A10);   // FRAME_LENGTH_LINES
    AR0833_write_cmos_sensor(0x0202, 0x0A01);   // COARSE_INTEGRATION_TIME
    AR0833_write_cmos_sensor(0x0344, 0x0008);   // X_ADDR_START
    AR0833_write_cmos_sensor(0x0348, 0x0CC7);   // X_ADDR_END
    AR0833_write_cmos_sensor(0x0346, 0x0008);   // Y_ADDR_START
    AR0833_write_cmos_sensor(0x034A, 0x0997);   // Y_ADDR_END
    AR0833_write_cmos_sensor(0x034C, 0x0CC0);   // X_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x034E, 0x0990);   // Y_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x3040, 0x4041);   // READ_MODE
    AR0833_write_cmos_sensor(0x0400, 0x0000);   // SCALING_MODE
    AR0833_write_cmos_sensor(0x0402, 0x0000);   // SPATIAL_SAMPLING
    AR0833_write_cmos_sensor(0x0404, 0x0010);   // SCALE_M
    AR0833_write_cmos_sensor(0x0408, 0x1010);   // SECOND_RESIDUAL
    AR0833_write_cmos_sensor(0x040A, 0x0210);   // SECOND_CROP
    AR0833_write_cmos_sensor(0x306E, 0x9080);   // DATA_PATH_SELECT
    AR0833_write_cmos_sensor(0x301A, 0x001C);   // RESET_REGISTER

}
#endif

#if RSE3264x2448_30FPS_MCLK24_4LANE_RAW10
static void Func_RSE3264x2448_30FPS_MCLK24_4LANE_RAW10(void)
{

    AR0833DB("Func_RSE3264x2448_30FPS_MCLK24_4LANE_RAW10");

    AR0833_write_cmos_sensor(0x301A, 0x0019);   // RESET_REGISTER
    Sleep(100);
    AR0833_write_cmos_sensor(0x301A, 0x0218);   // RESET_REGISTER
    AR0833_write_cmos_sensor(0x3042, 0x0000);   // DARK_CONTROL2
    AR0833_write_cmos_sensor(0x30C0, 0x1810);   // CALIB_CONTROL
    AR0833_write_cmos_sensor(0x30C8, 0x0018);   // CALIB_DAC
    AR0833_write_cmos_sensor(0x30D2, 0x0000);   // CRM_CONTROL
    AR0833_write_cmos_sensor(0x30D4, 0xD030);   // COLUMN_CORRECTION
    AR0833_write_cmos_sensor(0x30D6, 0x2200);   // COLUMN_CORRECTION2
    AR0833_write_cmos_sensor(0x30DA, 0x0080);   // COLUMN_CORRECTION_CLIP2
    AR0833_write_cmos_sensor(0x30DC, 0x0080);   // COLUMN_CORRECTION_CLIP3
    AR0833_write_cmos_sensor(0x30EE, 0x0340);   // DARK_CONTROL3
    AR0833_write_cmos_sensor(0x316A, 0x8800);   // DAC_RSTLO
    AR0833_write_cmos_sensor(0x316C, 0x8200);   // DAC_TXLO
    AR0833_write_cmos_sensor(0x3172, 0x0286);   // ANALOG_CONTROL2
    AR0833_write_cmos_sensor(0x3174, 0x8000);   // ANALOG_CONTROL3
    AR0833_write_cmos_sensor(0x317C, 0xE103);   // ANALOG_CONTROL7
    AR0833_write_cmos_sensor(0x3180, 0xF0FF);   // FINEDIGCORR_CONTROL
    AR0833_write_cmos_sensor(0x31E0, 0x0741);   // PIX_DEF_ID
    AR0833_write_cmos_sensor(0x3ECC, 0x0056);   // DAC_LD_0_1
    AR0833_write_cmos_sensor(0x3ED0, 0xA8AA);   // DAC_LD_4_5
    AR0833_write_cmos_sensor(0x3ED2, 0xAAA8);   // DAC_LD_6_7
    AR0833_write_cmos_sensor(0x3ED4, 0x8ACC);   // DAC_LD_8_9
    AR0833_write_cmos_sensor(0x3ED8, 0x7288);   // DAC_LD_12_13
    AR0833_write_cmos_sensor(0x3EDA, 0x77CA);   // DAC_LD_14_15
    AR0833_write_cmos_sensor(0x3EDE, 0x6664);   // DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5);   // DAC_LD_20_21
    AR0833_write_cmos_sensor(0x3EE4, 0x1548);   // DAC_LD_24_25
    AR0833_write_cmos_sensor(0x3EE6, 0xB10C);   // DAC_LD_26_27
    AR0833_write_cmos_sensor(0x3EE8, 0x6E79);   // DAC_LD_28_29
    AR0833_write_cmos_sensor(0x3EFE, 0x77CC);   // DAC_LD_TXLO
    AR0833_write_cmos_sensor(0x31E6, 0x0000);   // PIX_DEF_ID_2
    AR0833_write_cmos_sensor(0x3F00, 0x0028);   // BM_T0
    AR0833_write_cmos_sensor(0x3F02, 0x0140);   // BM_T1
    AR0833_write_cmos_sensor(0x3F04, 0x0002);   // NOISE_GAIN_THRESHOLD0
    AR0833_write_cmos_sensor(0x3F06, 0x0004);   // NOISE_GAIN_THRESHOLD1
    AR0833_write_cmos_sensor(0x3F08, 0x0008);   // NOISE_GAIN_THRESHOLD2
    AR0833_write_cmos_sensor(0x3F0A, 0x0B09);   // NOISE_FLOOR10
    AR0833_write_cmos_sensor(0x3F0C, 0x0302);   // NOISE_FLOOR32
    AR0833_write_cmos_sensor(0x3F10, 0x0505);   // SINGLE_K_FACTOR0
    AR0833_write_cmos_sensor(0x3F12, 0x0303);   // SINGLE_K_FACTOR1
    AR0833_write_cmos_sensor(0x3F14, 0x0101);   // SINGLE_K_FACTOR2
    AR0833_write_cmos_sensor(0x3F16, 0x2020);   // CROSSFACTOR0
    AR0833_write_cmos_sensor(0x3F18, 0x0404);   // CROSSFACTOR1
    AR0833_write_cmos_sensor(0x3F1A, 0x7070);   // CROSSFACTOR2
    AR0833_write_cmos_sensor(0x3F1C, 0x003A);   // SINGLE_MAXFACTOR
    AR0833_write_cmos_sensor(0x3F1E, 0x003C);   // NOISE_COEF
    AR0833_write_cmos_sensor(0x3F2C, 0x2210);   // GTH_THRES_RTN
    AR0833_write_cmos_sensor(0x3F40, 0x2020);   // COUPLE_K_FACTOR0
    AR0833_write_cmos_sensor(0x3F42, 0x0808);   // COUPLE_K_FACTOR1
    AR0833_write_cmos_sensor(0x3F44, 0x0101);   // COUPLE_K_FACTOR2
    Sleep(100);
    AR0833_write_cmos_sensor_8(0x3D00, 0x04);
    AR0833_write_cmos_sensor_8(0x3D01, 0x70);
    AR0833_write_cmos_sensor_8(0x3D02, 0xC8);
    AR0833_write_cmos_sensor_8(0x3D03, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D04, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D05, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D06, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D07, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D08, 0x6F);
    AR0833_write_cmos_sensor_8(0x3D09, 0x40);
    AR0833_write_cmos_sensor_8(0x3D0A, 0x14);
    AR0833_write_cmos_sensor_8(0x3D0B, 0x0E);
    AR0833_write_cmos_sensor_8(0x3D0C, 0x23);
    AR0833_write_cmos_sensor_8(0x3D0D, 0x82);
    AR0833_write_cmos_sensor_8(0x3D0E, 0x41);
    AR0833_write_cmos_sensor_8(0x3D0F, 0x5C);
    AR0833_write_cmos_sensor_8(0x3D10, 0x54);
    AR0833_write_cmos_sensor_8(0x3D11, 0x6E);
    AR0833_write_cmos_sensor_8(0x3D12, 0x42);
    AR0833_write_cmos_sensor_8(0x3D13, 0x00);
    AR0833_write_cmos_sensor_8(0x3D14, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D15, 0x5D);
    AR0833_write_cmos_sensor_8(0x3D16, 0x80);
    AR0833_write_cmos_sensor_8(0x3D17, 0x5A);
    AR0833_write_cmos_sensor_8(0x3D18, 0x80);
    AR0833_write_cmos_sensor_8(0x3D19, 0x57);
    AR0833_write_cmos_sensor_8(0x3D1A, 0x84);
    AR0833_write_cmos_sensor_8(0x3D1B, 0x64);
    AR0833_write_cmos_sensor_8(0x3D1C, 0x80);
    AR0833_write_cmos_sensor_8(0x3D1D, 0x55);
    AR0833_write_cmos_sensor_8(0x3D1E, 0x86);
    AR0833_write_cmos_sensor_8(0x3D1F, 0x64);
    AR0833_write_cmos_sensor_8(0x3D20, 0x80);
    AR0833_write_cmos_sensor_8(0x3D21, 0x65);
    AR0833_write_cmos_sensor_8(0x3D22, 0x88);
    AR0833_write_cmos_sensor_8(0x3D23, 0x65);
    AR0833_write_cmos_sensor_8(0x3D24, 0x84);
    AR0833_write_cmos_sensor_8(0x3D25, 0x58);
    AR0833_write_cmos_sensor_8(0x3D26, 0x80);
    AR0833_write_cmos_sensor_8(0x3D27, 0x00);
    AR0833_write_cmos_sensor_8(0x3D28, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D29, 0x80);
    AR0833_write_cmos_sensor_8(0x3D2A, 0x30);
    AR0833_write_cmos_sensor_8(0x3D2B, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D2C, 0x84);
    AR0833_write_cmos_sensor_8(0x3D2D, 0x42);
    AR0833_write_cmos_sensor_8(0x3D2E, 0x82);
    AR0833_write_cmos_sensor_8(0x3D2F, 0x10);
    AR0833_write_cmos_sensor_8(0x3D30, 0x30);
    AR0833_write_cmos_sensor_8(0x3D31, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D32, 0x5B);
    AR0833_write_cmos_sensor_8(0x3D33, 0x80);
    AR0833_write_cmos_sensor_8(0x3D34, 0x63);
    AR0833_write_cmos_sensor_8(0x3D35, 0x8B);
    AR0833_write_cmos_sensor_8(0x3D36, 0x30);
    AR0833_write_cmos_sensor_8(0x3D37, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D38, 0xA5);
    AR0833_write_cmos_sensor_8(0x3D39, 0x59);
    AR0833_write_cmos_sensor_8(0x3D3A, 0x84);
    AR0833_write_cmos_sensor_8(0x3D3B, 0x6C);
    AR0833_write_cmos_sensor_8(0x3D3C, 0x80);
    AR0833_write_cmos_sensor_8(0x3D3D, 0x6D);
    AR0833_write_cmos_sensor_8(0x3D3E, 0x81);
    AR0833_write_cmos_sensor_8(0x3D3F, 0x5F);
    AR0833_write_cmos_sensor_8(0x3D40, 0x60);
    AR0833_write_cmos_sensor_8(0x3D41, 0x61);
    AR0833_write_cmos_sensor_8(0x3D42, 0x10);
    AR0833_write_cmos_sensor_8(0x3D43, 0x30);
    AR0833_write_cmos_sensor_8(0x3D44, 0x88);
    AR0833_write_cmos_sensor_8(0x3D45, 0x66);
    AR0833_write_cmos_sensor_8(0x3D46, 0x83);
    AR0833_write_cmos_sensor_8(0x3D47, 0x6E);
    AR0833_write_cmos_sensor_8(0x3D48, 0x80);
    AR0833_write_cmos_sensor_8(0x3D49, 0x64);
    AR0833_write_cmos_sensor_8(0x3D4A, 0x87);
    AR0833_write_cmos_sensor_8(0x3D4B, 0x64);
    AR0833_write_cmos_sensor_8(0x3D4C, 0x30);
    AR0833_write_cmos_sensor_8(0x3D4D, 0x50);
    AR0833_write_cmos_sensor_8(0x3D4E, 0xDA);
    AR0833_write_cmos_sensor_8(0x3D4F, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D50, 0x83);
    AR0833_write_cmos_sensor_8(0x3D51, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D52, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D53, 0x30);
    AR0833_write_cmos_sensor_8(0x3D54, 0x94);
    AR0833_write_cmos_sensor_8(0x3D55, 0x67);
    AR0833_write_cmos_sensor_8(0x3D56, 0x84);
    AR0833_write_cmos_sensor_8(0x3D57, 0x65);
    AR0833_write_cmos_sensor_8(0x3D58, 0x82);
    AR0833_write_cmos_sensor_8(0x3D59, 0x4D);
    AR0833_write_cmos_sensor_8(0x3D5A, 0x83);
    AR0833_write_cmos_sensor_8(0x3D5B, 0x65);
    AR0833_write_cmos_sensor_8(0x3D5C, 0x30);
    AR0833_write_cmos_sensor_8(0x3D5D, 0x50);
    AR0833_write_cmos_sensor_8(0x3D5E, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D5F, 0x58);
    AR0833_write_cmos_sensor_8(0x3D60, 0x43);
    AR0833_write_cmos_sensor_8(0x3D61, 0x06);
    AR0833_write_cmos_sensor_8(0x3D62, 0x00);
    AR0833_write_cmos_sensor_8(0x3D63, 0x8D);
    AR0833_write_cmos_sensor_8(0x3D64, 0x45);
    AR0833_write_cmos_sensor_8(0x3D65, 0xA0);
    AR0833_write_cmos_sensor_8(0x3D66, 0x45);
    AR0833_write_cmos_sensor_8(0x3D67, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D68, 0x83);
    AR0833_write_cmos_sensor_8(0x3D69, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D6A, 0x06);
    AR0833_write_cmos_sensor_8(0x3D6B, 0x00);
    AR0833_write_cmos_sensor_8(0x3D6C, 0x81);
    AR0833_write_cmos_sensor_8(0x3D6D, 0x43);
    AR0833_write_cmos_sensor_8(0x3D6E, 0x9C);
    AR0833_write_cmos_sensor_8(0x3D6F, 0x58);
    AR0833_write_cmos_sensor_8(0x3D70, 0x84);
    AR0833_write_cmos_sensor_8(0x3D71, 0x30);
    AR0833_write_cmos_sensor_8(0x3D72, 0x90);
    AR0833_write_cmos_sensor_8(0x3D73, 0x67);
    AR0833_write_cmos_sensor_8(0x3D74, 0x64);
    AR0833_write_cmos_sensor_8(0x3D75, 0x88);
    AR0833_write_cmos_sensor_8(0x3D76, 0x64);
    AR0833_write_cmos_sensor_8(0x3D77, 0x80);
    AR0833_write_cmos_sensor_8(0x3D78, 0x65);
    AR0833_write_cmos_sensor_8(0x3D79, 0x88);
    AR0833_write_cmos_sensor_8(0x3D7A, 0x65);
    AR0833_write_cmos_sensor_8(0x3D7B, 0x82);
    AR0833_write_cmos_sensor_8(0x3D7C, 0x10);
    AR0833_write_cmos_sensor_8(0x3D7D, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D7E, 0xEB);
    AR0833_write_cmos_sensor_8(0x3D7F, 0x10);
    AR0833_write_cmos_sensor_8(0x3D80, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D81, 0x66);
    AR0833_write_cmos_sensor_8(0x3D82, 0x85);
    AR0833_write_cmos_sensor_8(0x3D83, 0x64);
    AR0833_write_cmos_sensor_8(0x3D84, 0x81);
    AR0833_write_cmos_sensor_8(0x3D85, 0x17);
    AR0833_write_cmos_sensor_8(0x3D86, 0x00);
    AR0833_write_cmos_sensor_8(0x3D87, 0x80);
    AR0833_write_cmos_sensor_8(0x3D88, 0x20);
    AR0833_write_cmos_sensor_8(0x3D89, 0x0D);
    AR0833_write_cmos_sensor_8(0x3D8A, 0x80);
    AR0833_write_cmos_sensor_8(0x3D8B, 0x18);
    AR0833_write_cmos_sensor_8(0x3D8C, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D8D, 0x80);
    AR0833_write_cmos_sensor_8(0x3D8E, 0x64);
    AR0833_write_cmos_sensor_8(0x3D8F, 0x30);
    AR0833_write_cmos_sensor_8(0x3D90, 0x60);
    AR0833_write_cmos_sensor_8(0x3D91, 0x41);
    AR0833_write_cmos_sensor_8(0x3D92, 0x82);
    AR0833_write_cmos_sensor_8(0x3D93, 0x42);
    AR0833_write_cmos_sensor_8(0x3D94, 0xB2);
    AR0833_write_cmos_sensor_8(0x3D95, 0x42);
    AR0833_write_cmos_sensor_8(0x3D96, 0x80);
    AR0833_write_cmos_sensor_8(0x3D97, 0x40);
    AR0833_write_cmos_sensor_8(0x3D98, 0x81);
    AR0833_write_cmos_sensor_8(0x3D99, 0x40);
    AR0833_write_cmos_sensor_8(0x3D9A, 0x80);
    AR0833_write_cmos_sensor_8(0x3D9B, 0x41);
    AR0833_write_cmos_sensor_8(0x3D9C, 0x80);
    AR0833_write_cmos_sensor_8(0x3D9D, 0x42);
    AR0833_write_cmos_sensor_8(0x3D9E, 0x80);
    AR0833_write_cmos_sensor_8(0x3D9F, 0x43);
    AR0833_write_cmos_sensor_8(0x3DA0, 0x83);
    AR0833_write_cmos_sensor_8(0x3DA1, 0x06);
    AR0833_write_cmos_sensor_8(0x3DA2, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DA3, 0x88);
    AR0833_write_cmos_sensor_8(0x3DA4, 0x44);
    AR0833_write_cmos_sensor_8(0x3DA5, 0x87);
    AR0833_write_cmos_sensor_8(0x3DA6, 0x6A);
    AR0833_write_cmos_sensor_8(0x3DA7, 0x83);
    AR0833_write_cmos_sensor_8(0x3DA8, 0x6B);
    AR0833_write_cmos_sensor_8(0x3DA9, 0x92);
    AR0833_write_cmos_sensor_8(0x3DAA, 0x44);
    AR0833_write_cmos_sensor_8(0x3DAB, 0x88);
    AR0833_write_cmos_sensor_8(0x3DAC, 0x06);
    AR0833_write_cmos_sensor_8(0x3DAD, 0xC8);
    AR0833_write_cmos_sensor_8(0x3DAE, 0x81);
    AR0833_write_cmos_sensor_8(0x3DAF, 0x41);
    AR0833_write_cmos_sensor_8(0x3DB0, 0x85);
    AR0833_write_cmos_sensor_8(0x3DB1, 0x30);
    AR0833_write_cmos_sensor_8(0x3DB2, 0xA4);
    AR0833_write_cmos_sensor_8(0x3DB3, 0x67);
    AR0833_write_cmos_sensor_8(0x3DB4, 0x85);
    AR0833_write_cmos_sensor_8(0x3DB5, 0x65);
    AR0833_write_cmos_sensor_8(0x3DB6, 0x87);
    AR0833_write_cmos_sensor_8(0x3DB7, 0x65);
    AR0833_write_cmos_sensor_8(0x3DB8, 0x30);
    AR0833_write_cmos_sensor_8(0x3DB9, 0x60);
    AR0833_write_cmos_sensor_8(0x3DBA, 0x8D);
    AR0833_write_cmos_sensor_8(0x3DBB, 0x42);
    AR0833_write_cmos_sensor_8(0x3DBC, 0x82);
    AR0833_write_cmos_sensor_8(0x3DBD, 0x40);
    AR0833_write_cmos_sensor_8(0x3DBE, 0x82);
    AR0833_write_cmos_sensor_8(0x3DBF, 0x40);
    AR0833_write_cmos_sensor_8(0x3DC0, 0x80);
    AR0833_write_cmos_sensor_8(0x3DC1, 0x41);
    AR0833_write_cmos_sensor_8(0x3DC2, 0x80);
    AR0833_write_cmos_sensor_8(0x3DC3, 0x42);
    AR0833_write_cmos_sensor_8(0x3DC4, 0x80);
    AR0833_write_cmos_sensor_8(0x3DC5, 0x43);
    AR0833_write_cmos_sensor_8(0x3DC6, 0x83);
    AR0833_write_cmos_sensor_8(0x3DC7, 0x06);
    AR0833_write_cmos_sensor_8(0x3DC8, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DC9, 0x88);
    AR0833_write_cmos_sensor_8(0x3DCA, 0x44);
    AR0833_write_cmos_sensor_8(0x3DCB, 0x9C);
    AR0833_write_cmos_sensor_8(0x3DCC, 0x44);
    AR0833_write_cmos_sensor_8(0x3DCD, 0x88);
    AR0833_write_cmos_sensor_8(0x3DCE, 0x06);
    AR0833_write_cmos_sensor_8(0x3DCF, 0xC8);
    AR0833_write_cmos_sensor_8(0x3DD0, 0x85);
    AR0833_write_cmos_sensor_8(0x3DD1, 0x41);
    AR0833_write_cmos_sensor_8(0x3DD2, 0x6A);
    AR0833_write_cmos_sensor_8(0x3DD3, 0x83);
    AR0833_write_cmos_sensor_8(0x3DD4, 0x6B);
    AR0833_write_cmos_sensor_8(0x3DD5, 0xA0);
    AR0833_write_cmos_sensor_8(0x3DD6, 0x42);
    AR0833_write_cmos_sensor_8(0x3DD7, 0x82);
    AR0833_write_cmos_sensor_8(0x3DD8, 0x40);
    AR0833_write_cmos_sensor_8(0x3DD9, 0x6C);
    AR0833_write_cmos_sensor_8(0x3DDA, 0x3A);
    AR0833_write_cmos_sensor_8(0x3DDB, 0xA8);
    AR0833_write_cmos_sensor_8(0x3DDC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DDD, 0x28);
    AR0833_write_cmos_sensor_8(0x3DDE, 0x30);
    AR0833_write_cmos_sensor_8(0x3DDF, 0x70);
    AR0833_write_cmos_sensor_8(0x3DE0, 0x6F);
    AR0833_write_cmos_sensor_8(0x3DE1, 0x40);
    AR0833_write_cmos_sensor_8(0x3DE2, 0x14);
    AR0833_write_cmos_sensor_8(0x3DE3, 0x0E);
    AR0833_write_cmos_sensor_8(0x3DE4, 0x23);
    AR0833_write_cmos_sensor_8(0x3DE5, 0xC2);
    AR0833_write_cmos_sensor_8(0x3DE6, 0x41);
    AR0833_write_cmos_sensor_8(0x3DE7, 0x82);
    AR0833_write_cmos_sensor_8(0x3DE8, 0x42);
    AR0833_write_cmos_sensor_8(0x3DE9, 0x00);
    AR0833_write_cmos_sensor_8(0x3DEA, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DEB, 0x5D);
    AR0833_write_cmos_sensor_8(0x3DEC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DED, 0x5A);
    AR0833_write_cmos_sensor_8(0x3DEE, 0x80);
    AR0833_write_cmos_sensor_8(0x3DEF, 0x57);
    AR0833_write_cmos_sensor_8(0x3DF0, 0x84);
    AR0833_write_cmos_sensor_8(0x3DF1, 0x64);
    AR0833_write_cmos_sensor_8(0x3DF2, 0x80);
    AR0833_write_cmos_sensor_8(0x3DF3, 0x55);
    AR0833_write_cmos_sensor_8(0x3DF4, 0x86);
    AR0833_write_cmos_sensor_8(0x3DF5, 0x64);
    AR0833_write_cmos_sensor_8(0x3DF6, 0x80);
    AR0833_write_cmos_sensor_8(0x3DF7, 0x65);
    AR0833_write_cmos_sensor_8(0x3DF8, 0x88);
    AR0833_write_cmos_sensor_8(0x3DF9, 0x65);
    AR0833_write_cmos_sensor_8(0x3DFA, 0x82);
    AR0833_write_cmos_sensor_8(0x3DFB, 0x54);
    AR0833_write_cmos_sensor_8(0x3DFC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DFD, 0x58);
    AR0833_write_cmos_sensor_8(0x3DFE, 0x80);
    AR0833_write_cmos_sensor_8(0x3DFF, 0x00);
    AR0833_write_cmos_sensor_8(0x3E00, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E01, 0x86);
    AR0833_write_cmos_sensor_8(0x3E02, 0x42);
    AR0833_write_cmos_sensor_8(0x3E03, 0x82);
    AR0833_write_cmos_sensor_8(0x3E04, 0x10);
    AR0833_write_cmos_sensor_8(0x3E05, 0x30);
    AR0833_write_cmos_sensor_8(0x3E06, 0x9C);
    AR0833_write_cmos_sensor_8(0x3E07, 0x5C);
    AR0833_write_cmos_sensor_8(0x3E08, 0x80);
    AR0833_write_cmos_sensor_8(0x3E09, 0x6E);
    AR0833_write_cmos_sensor_8(0x3E0A, 0x86);
    AR0833_write_cmos_sensor_8(0x3E0B, 0x5B);
    AR0833_write_cmos_sensor_8(0x3E0C, 0x80);
    AR0833_write_cmos_sensor_8(0x3E0D, 0x63);
    AR0833_write_cmos_sensor_8(0x3E0E, 0x9E);
    AR0833_write_cmos_sensor_8(0x3E0F, 0x59);
    AR0833_write_cmos_sensor_8(0x3E10, 0x8C);
    AR0833_write_cmos_sensor_8(0x3E11, 0x5E);
    AR0833_write_cmos_sensor_8(0x3E12, 0x8A);
    AR0833_write_cmos_sensor_8(0x3E13, 0x6C);
    AR0833_write_cmos_sensor_8(0x3E14, 0x80);
    AR0833_write_cmos_sensor_8(0x3E15, 0x6D);
    AR0833_write_cmos_sensor_8(0x3E16, 0x81);
    AR0833_write_cmos_sensor_8(0x3E17, 0x5F);
    AR0833_write_cmos_sensor_8(0x3E18, 0x60);
    AR0833_write_cmos_sensor_8(0x3E19, 0x61);
    AR0833_write_cmos_sensor_8(0x3E1A, 0x88);
    AR0833_write_cmos_sensor_8(0x3E1B, 0x10);
    AR0833_write_cmos_sensor_8(0x3E1C, 0x30);
    AR0833_write_cmos_sensor_8(0x3E1D, 0x66);
    AR0833_write_cmos_sensor_8(0x3E1E, 0x83);
    AR0833_write_cmos_sensor_8(0x3E1F, 0x6E);
    AR0833_write_cmos_sensor_8(0x3E20, 0x80);
    AR0833_write_cmos_sensor_8(0x3E21, 0x64);
    AR0833_write_cmos_sensor_8(0x3E22, 0x87);
    AR0833_write_cmos_sensor_8(0x3E23, 0x64);
    AR0833_write_cmos_sensor_8(0x3E24, 0x30);
    AR0833_write_cmos_sensor_8(0x3E25, 0x50);
    AR0833_write_cmos_sensor_8(0x3E26, 0xD3);
    AR0833_write_cmos_sensor_8(0x3E27, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E28, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E29, 0xAD);
    AR0833_write_cmos_sensor_8(0x3E2A, 0x30);
    AR0833_write_cmos_sensor_8(0x3E2B, 0x94);
    AR0833_write_cmos_sensor_8(0x3E2C, 0x67);
    AR0833_write_cmos_sensor_8(0x3E2D, 0x84);
    AR0833_write_cmos_sensor_8(0x3E2E, 0x65);
    AR0833_write_cmos_sensor_8(0x3E2F, 0x82);
    AR0833_write_cmos_sensor_8(0x3E30, 0x4D);
    AR0833_write_cmos_sensor_8(0x3E31, 0x83);
    AR0833_write_cmos_sensor_8(0x3E32, 0x65);
    AR0833_write_cmos_sensor_8(0x3E33, 0x30);
    AR0833_write_cmos_sensor_8(0x3E34, 0x50);
    AR0833_write_cmos_sensor_8(0x3E35, 0xA7);
    AR0833_write_cmos_sensor_8(0x3E36, 0x43);
    AR0833_write_cmos_sensor_8(0x3E37, 0x06);
    AR0833_write_cmos_sensor_8(0x3E38, 0x00);
    AR0833_write_cmos_sensor_8(0x3E39, 0x8D);
    AR0833_write_cmos_sensor_8(0x3E3A, 0x45);
    AR0833_write_cmos_sensor_8(0x3E3B, 0x9A);
    AR0833_write_cmos_sensor_8(0x3E3C, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E3D, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E3E, 0x45);
    AR0833_write_cmos_sensor_8(0x3E3F, 0x85);
    AR0833_write_cmos_sensor_8(0x3E40, 0x06);
    AR0833_write_cmos_sensor_8(0x3E41, 0x00);
    AR0833_write_cmos_sensor_8(0x3E42, 0x81);
    AR0833_write_cmos_sensor_8(0x3E43, 0x43);
    AR0833_write_cmos_sensor_8(0x3E44, 0x8A);
    AR0833_write_cmos_sensor_8(0x3E45, 0x6F);
    AR0833_write_cmos_sensor_8(0x3E46, 0x96);
    AR0833_write_cmos_sensor_8(0x3E47, 0x30);
    AR0833_write_cmos_sensor_8(0x3E48, 0x90);
    AR0833_write_cmos_sensor_8(0x3E49, 0x67);
    AR0833_write_cmos_sensor_8(0x3E4A, 0x64);
    AR0833_write_cmos_sensor_8(0x3E4B, 0x88);
    AR0833_write_cmos_sensor_8(0x3E4C, 0x64);
    AR0833_write_cmos_sensor_8(0x3E4D, 0x80);
    AR0833_write_cmos_sensor_8(0x3E4E, 0x65);
    AR0833_write_cmos_sensor_8(0x3E4F, 0x82);
    AR0833_write_cmos_sensor_8(0x3E50, 0x10);
    AR0833_write_cmos_sensor_8(0x3E51, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E52, 0x84);
    AR0833_write_cmos_sensor_8(0x3E53, 0x65);
    AR0833_write_cmos_sensor_8(0x3E54, 0xEF);
    AR0833_write_cmos_sensor_8(0x3E55, 0x10);
    AR0833_write_cmos_sensor_8(0x3E56, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E57, 0x66);
    AR0833_write_cmos_sensor_8(0x3E58, 0x85);
    AR0833_write_cmos_sensor_8(0x3E59, 0x64);
    AR0833_write_cmos_sensor_8(0x3E5A, 0x81);
    AR0833_write_cmos_sensor_8(0x3E5B, 0x17);
    AR0833_write_cmos_sensor_8(0x3E5C, 0x00);
    AR0833_write_cmos_sensor_8(0x3E5D, 0x80);
    AR0833_write_cmos_sensor_8(0x3E5E, 0x20);
    AR0833_write_cmos_sensor_8(0x3E5F, 0x0D);
    AR0833_write_cmos_sensor_8(0x3E60, 0x80);
    AR0833_write_cmos_sensor_8(0x3E61, 0x18);
    AR0833_write_cmos_sensor_8(0x3E62, 0x0C);
    AR0833_write_cmos_sensor_8(0x3E63, 0x80);
    AR0833_write_cmos_sensor_8(0x3E64, 0x64);
    AR0833_write_cmos_sensor_8(0x3E65, 0x30);
    AR0833_write_cmos_sensor_8(0x3E66, 0x60);
    AR0833_write_cmos_sensor_8(0x3E67, 0x41);
    AR0833_write_cmos_sensor_8(0x3E68, 0x82);
    AR0833_write_cmos_sensor_8(0x3E69, 0x42);
    AR0833_write_cmos_sensor_8(0x3E6A, 0xB2);
    AR0833_write_cmos_sensor_8(0x3E6B, 0x42);
    AR0833_write_cmos_sensor_8(0x3E6C, 0x80);
    AR0833_write_cmos_sensor_8(0x3E6D, 0x40);
    AR0833_write_cmos_sensor_8(0x3E6E, 0x82);
    AR0833_write_cmos_sensor_8(0x3E6F, 0x40);
    AR0833_write_cmos_sensor_8(0x3E70, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E71, 0x45);
    AR0833_write_cmos_sensor_8(0x3E72, 0x92);
    AR0833_write_cmos_sensor_8(0x3E73, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E74, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E75, 0x9B);
    AR0833_write_cmos_sensor_8(0x3E76, 0x45);
    AR0833_write_cmos_sensor_8(0x3E77, 0x81);
    AR0833_write_cmos_sensor_8(0x3E78, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E79, 0x40);
    AR0833_write_cmos_sensor_8(0x3E7A, 0x8C);
    AR0833_write_cmos_sensor_8(0x3E7B, 0x30);
    AR0833_write_cmos_sensor_8(0x3E7C, 0xA4);
    AR0833_write_cmos_sensor_8(0x3E7D, 0x67);
    AR0833_write_cmos_sensor_8(0x3E7E, 0x85);
    AR0833_write_cmos_sensor_8(0x3E7F, 0x65);
    AR0833_write_cmos_sensor_8(0x3E80, 0x87);
    AR0833_write_cmos_sensor_8(0x3E81, 0x65);
    AR0833_write_cmos_sensor_8(0x3E82, 0x30);
    AR0833_write_cmos_sensor_8(0x3E83, 0x60);
    AR0833_write_cmos_sensor_8(0x3E84, 0xD3);
    AR0833_write_cmos_sensor_8(0x3E85, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E86, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E87, 0xAC);
    AR0833_write_cmos_sensor_8(0x3E88, 0x6C);
    AR0833_write_cmos_sensor_8(0x3E89, 0x32);
    AR0833_write_cmos_sensor_8(0x3E8A, 0xA8);
    AR0833_write_cmos_sensor_8(0x3E8B, 0x80);
    AR0833_write_cmos_sensor_8(0x3E8C, 0x28);
    AR0833_write_cmos_sensor_8(0x3E8D, 0x30);
    AR0833_write_cmos_sensor_8(0x3E8E, 0x70);
    AR0833_write_cmos_sensor_8(0x3E8F, 0x00);
    AR0833_write_cmos_sensor_8(0x3E90, 0x80);
    AR0833_write_cmos_sensor_8(0x3E91, 0x40);
    AR0833_write_cmos_sensor_8(0x3E92, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E93, 0xBD);
    AR0833_write_cmos_sensor_8(0x3E94, 0x00);
    AR0833_write_cmos_sensor_8(0x3E95, 0x0E);
    AR0833_write_cmos_sensor_8(0x3E96, 0xBE);
    AR0833_write_cmos_sensor_8(0x3E97, 0x44);
    AR0833_write_cmos_sensor_8(0x3E98, 0x88);
    AR0833_write_cmos_sensor_8(0x3E99, 0x44);
    AR0833_write_cmos_sensor_8(0x3E9A, 0xBC);
    AR0833_write_cmos_sensor_8(0x3E9B, 0x78);
    AR0833_write_cmos_sensor_8(0x3E9C, 0x09);
    AR0833_write_cmos_sensor_8(0x3E9D, 0x00);
    AR0833_write_cmos_sensor_8(0x3E9E, 0x89);
    AR0833_write_cmos_sensor_8(0x3E9F, 0x04);
    AR0833_write_cmos_sensor_8(0x3EA0, 0x80);
    AR0833_write_cmos_sensor_8(0x3EA1, 0x80);
    AR0833_write_cmos_sensor_8(0x3EA2, 0x02);
    AR0833_write_cmos_sensor_8(0x3EA3, 0x40);
    AR0833_write_cmos_sensor_8(0x3EA4, 0x86);
    AR0833_write_cmos_sensor_8(0x3EA5, 0x09);
    AR0833_write_cmos_sensor_8(0x3EA6, 0x00);
    AR0833_write_cmos_sensor_8(0x3EA7, 0x8E);
    AR0833_write_cmos_sensor_8(0x3EA8, 0x09);
    AR0833_write_cmos_sensor_8(0x3EA9, 0x00);
    AR0833_write_cmos_sensor_8(0x3EAA, 0x80);
    AR0833_write_cmos_sensor_8(0x3EAB, 0x02);
    AR0833_write_cmos_sensor_8(0x3EAC, 0x40);
    AR0833_write_cmos_sensor_8(0x3EAD, 0x80);
    AR0833_write_cmos_sensor_8(0x3EAE, 0x04);
    AR0833_write_cmos_sensor_8(0x3EAF, 0x80);
    AR0833_write_cmos_sensor_8(0x3EB0, 0x88);
    AR0833_write_cmos_sensor_8(0x3EB1, 0x7D);
    AR0833_write_cmos_sensor_8(0x3EB2, 0x9E);
    AR0833_write_cmos_sensor_8(0x3EB3, 0x86);
    AR0833_write_cmos_sensor_8(0x3EB4, 0x09);
    AR0833_write_cmos_sensor_8(0x3EB5, 0x00);
    AR0833_write_cmos_sensor_8(0x3EB6, 0x87);
    AR0833_write_cmos_sensor_8(0x3EB7, 0x7A);
    AR0833_write_cmos_sensor_8(0x3EB8, 0x00);
    AR0833_write_cmos_sensor_8(0x3EB9, 0x0E);
    AR0833_write_cmos_sensor_8(0x3EBA, 0xC3);
    AR0833_write_cmos_sensor_8(0x3EBB, 0x79);
    AR0833_write_cmos_sensor_8(0x3EBC, 0x4C);
    AR0833_write_cmos_sensor_8(0x3EBD, 0x40);
    AR0833_write_cmos_sensor_8(0x3EBE, 0xBF);
    AR0833_write_cmos_sensor_8(0x3EBF, 0x70);
    AR0833_write_cmos_sensor_8(0x3EC0, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC1, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC2, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC3, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC4, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC5, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC6, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC7, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC8, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC9, 0x00);
    AR0833_write_cmos_sensor_8(0x3ECA, 0x00);
    AR0833_write_cmos_sensor_8(0x3ECB, 0x00);
    //
    AR0833_write_cmos_sensor(0x301A, 0x0018); 	// RESET_REGISTER
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EDE, 0x6664); 	// DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5); 	// DAC_LD_20_21
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5); 	// DAC_LD_20_21
    AR0833_write_cmos_sensor(0x301A, 0x001C); 	// RESET_REGISTER
    AR0833_write_cmos_sensor(0x0300, 0x0005); 	// VT_PIX_CLK_DIV
    AR0833_write_cmos_sensor(0x0302, 0x0001); 	// VT_SYS_CLK_DIV
    AR0833_write_cmos_sensor(0x0304, 0x0004); 	// PRE_PLL_CLK_DIV
    AR0833_write_cmos_sensor(0x0306, 0x007A); 	// PLL_MULTIPLIER
    AR0833_write_cmos_sensor(0x0308, 0x000A); 	// OP_PIX_CLK_DIV
    AR0833_write_cmos_sensor(0x030A, 0x0001); 	// OP_SYS_CLK_DIV
    AR0833_write_cmos_sensor(0x3064, 0x7800); 	// SMIA_TEST
    Sleep(1);
    AR0833_write_cmos_sensor(0x31B0, 0x0060); 	// FRAME_PREAMBLE
    AR0833_write_cmos_sensor(0x31B2, 0x0042); 	// LINE_PREAMBLE
    AR0833_write_cmos_sensor(0x31B4, 0x1C36); 	// MIPI_TIMING_0
    AR0833_write_cmos_sensor(0x31B6, 0x5218); 	// MIPI_TIMING_1
    AR0833_write_cmos_sensor(0x31B8, 0x404A); 	// MIPI_TIMING_2
    AR0833_write_cmos_sensor(0x31BA, 0x028A); 	// MIPI_TIMING_3
    AR0833_write_cmos_sensor(0x31BC, 0x0008); 	// MIPI_TIMING_4
    Sleep(1);
    AR0833_write_cmos_sensor(0x0342, 0x0ECC); 	// LINE_LENGTH_PCK
    AR0833_write_cmos_sensor(0x0340, 0x0A10); 	// FRAME_LENGTH_LINES
    AR0833_write_cmos_sensor(0x0202, 0x0A01); 	// COARSE_INTEGRATION_TIME
    AR0833_write_cmos_sensor(0x0344, 0x0008); 	// X_ADDR_START
    AR0833_write_cmos_sensor(0x0348, 0x0CC7); 	// X_ADDR_END
    AR0833_write_cmos_sensor(0x0346, 0x0008); 	// Y_ADDR_START
    AR0833_write_cmos_sensor(0x034A, 0x0997); 	// Y_ADDR_END
    AR0833_write_cmos_sensor(0x034C, 0x0CC0); 	// X_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x034E, 0x0990); 	// Y_OUTPUT_SIZE
    AR0833_write_cmos_sensor(0x3040, 0x4041); 	// READ_MODE
    AR0833_write_cmos_sensor(0x0400, 0x0000); 	// SCALING_MODE
    AR0833_write_cmos_sensor(0x0402, 0x0000); 	// SPATIAL_SAMPLING
    AR0833_write_cmos_sensor(0x0404, 0x0010); 	// SCALE_M
    AR0833_write_cmos_sensor(0x0408, 0x1010); 	// SECOND_RESIDUAL
    AR0833_write_cmos_sensor(0x040A, 0x0210); 	// SECOND_CROP
    AR0833_write_cmos_sensor(0x306E, 0x9080); 	// DATA_PATH_SELECT
    AR0833_write_cmos_sensor(0x301A, 0x001C); 	// RESET_REGISTER
}
#endif

#ifdef AR0833_RUN_RAW8

static void Func_Init_Setting_Raw8(void)
{
    AR0833DB("Func_Init_Setting_Raw8\n");

    AR0833_write_cmos_sensor(0x301A, 0x0019);   // RESET_REGISTER
    Sleep(100);
    AR0833_write_cmos_sensor(0x3042, 0x0000);   // DARK_CONTROL2
    AR0833_write_cmos_sensor(0x30C0, 0x1810);   // CALIB_CONTROL
    AR0833_write_cmos_sensor(0x30C8, 0x0018);   // CALIB_DAC
    AR0833_write_cmos_sensor(0x30D2, 0x0000);   // CRM_CONTROL
    AR0833_write_cmos_sensor(0x30D4, 0xD030);   // COLUMN_CORRECTION
    AR0833_write_cmos_sensor(0x30D6, 0x2200);   // COLUMN_CORRECTION2
    AR0833_write_cmos_sensor(0x30DA, 0x0080);   // COLUMN_CORRECTION_CLIP2
    AR0833_write_cmos_sensor(0x30DC, 0x0080);   // COLUMN_CORRECTION_CLIP3
    AR0833_write_cmos_sensor(0x30EE, 0x0340);   // DARK_CONTROL3
    AR0833_write_cmos_sensor(0x316A, 0x8800);   // DAC_RSTLO_BLCLO
    AR0833_write_cmos_sensor(0x316C, 0x8200);   // DAC_TXLO
    AR0833_write_cmos_sensor(0x3172, 0x0286);   // ANALOG_CONTROL2
    AR0833_write_cmos_sensor(0x3174, 0x8000);   // ANALOG_CONTROL3
    AR0833_write_cmos_sensor(0x317C, 0xE103);   // ANALOG_CONTROL7
    AR0833_write_cmos_sensor(0x3180, 0xF0FF);   // FINEDIGCORR_CONTROL
    AR0833_write_cmos_sensor(0x31E0, 0x0741);   // PIX_DEF_ID
    AR0833_write_cmos_sensor(0x3ECC, 0x0056);   // DAC_LD_0_1
    AR0833_write_cmos_sensor(0x3ED0, 0xA8AA);   // DAC_LD_4_5
    AR0833_write_cmos_sensor(0x3ED2, 0xAAA8);   // DAC_LD_6_7
    AR0833_write_cmos_sensor(0x3ED4, 0x8ACC);   // DAC_LD_8_9
    AR0833_write_cmos_sensor(0x3ED8, 0x7288);   // DAC_LD_12_13
    AR0833_write_cmos_sensor(0x3EDA, 0x77CA);   // DAC_LD_14_15
    AR0833_write_cmos_sensor(0x3EDE, 0x6664);   // DAC_LD_18_19
    AR0833_write_cmos_sensor(0x3EE0, 0x26D5);   // DAC_LD_20_21
    AR0833_write_cmos_sensor(0x3EE4, 0x1548);   // DAC_LD_24_25
    AR0833_write_cmos_sensor(0x3EE6, 0xB10C);   // DAC_LD_26_27
    AR0833_write_cmos_sensor(0x3EE8, 0x6E79);   // DAC_LD_28_29
    AR0833_write_cmos_sensor(0x3EFE, 0x77CC);   // DAC_LD_TXLO
    AR0833_write_cmos_sensor(0x31E6, 0x0000);   // PIX_DEF_ID_2
    AR0833_write_cmos_sensor(0x3F00, 0x0028);   // BM_T0
    AR0833_write_cmos_sensor(0x3F02, 0x0140);   // BM_T1
    AR0833_write_cmos_sensor(0x3F04, 0x0002);   // NOISE_GAIN_THRESHOLD0
    AR0833_write_cmos_sensor(0x3F06, 0x0004);   // NOISE_GAIN_THRESHOLD1
    AR0833_write_cmos_sensor(0x3F08, 0x0008);   // NOISE_GAIN_THRESHOLD2
    AR0833_write_cmos_sensor(0x3F0A, 0x0B09);   // NOISE_FLOOR10
    AR0833_write_cmos_sensor(0x3F0C, 0x0302);   // NOISE_FLOOR32
    AR0833_write_cmos_sensor(0x3F10, 0x0505);   // SINGLE_K_FACTOR0
    AR0833_write_cmos_sensor(0x3F12, 0x0303);   // SINGLE_K_FACTOR1
    AR0833_write_cmos_sensor(0x3F14, 0x0101);   // SINGLE_K_FACTOR2
    AR0833_write_cmos_sensor(0x3F16, 0x2020);   // CROSSFACTOR0
    AR0833_write_cmos_sensor(0x3F18, 0x0404);   // CROSSFACTOR1
    AR0833_write_cmos_sensor(0x3F1A, 0x7070);   // CROSSFACTOR2
    AR0833_write_cmos_sensor(0x3F1C, 0x003A);   // SINGLE_MAXFACTOR
    AR0833_write_cmos_sensor(0x3F1E, 0x003C);   // NOISE_COEF
    AR0833_write_cmos_sensor(0x3F2C, 0x2210);   // GTH_THRES_RTN
    AR0833_write_cmos_sensor(0x3F40, 0x2020);   // COUPLE_K_FACTOR0
    AR0833_write_cmos_sensor(0x3F42, 0x0808);   // COUPLE_K_FACTOR1
    AR0833_write_cmos_sensor(0x3F44, 0x0101);   // COUPLE_K_FACTOR2
    AR0833_write_cmos_sensor(0x301A, 0x0018);   // RESET_REGISTER

    Sleep(100);
    AR0833_write_cmos_sensor_8(0x3D00, 0x04);
    AR0833_write_cmos_sensor_8(0x3D01, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D02, 0xC3);
    AR0833_write_cmos_sensor_8(0x3D03, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D04, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D05, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D06, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D07, 0xFF);
    AR0833_write_cmos_sensor_8(0x3D08, 0x6F);
    AR0833_write_cmos_sensor_8(0x3D09, 0x40);
    AR0833_write_cmos_sensor_8(0x3D0A, 0x14);
    AR0833_write_cmos_sensor_8(0x3D0B, 0x0E);
    AR0833_write_cmos_sensor_8(0x3D0C, 0x23);
    AR0833_write_cmos_sensor_8(0x3D0D, 0xC6);
    AR0833_write_cmos_sensor_8(0x3D0E, 0x41);
    AR0833_write_cmos_sensor_8(0x3D0F, 0x20);
    AR0833_write_cmos_sensor_8(0x3D10, 0x30);
    AR0833_write_cmos_sensor_8(0x3D11, 0x19);
    AR0833_write_cmos_sensor_8(0x3D12, 0x00);
    AR0833_write_cmos_sensor_8(0x3D13, 0x80);
    AR0833_write_cmos_sensor_8(0x3D14, 0x42);
    AR0833_write_cmos_sensor_8(0x3D15, 0x00);
    AR0833_write_cmos_sensor_8(0x3D16, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D17, 0x81);
    AR0833_write_cmos_sensor_8(0x3D18, 0x64);
    AR0833_write_cmos_sensor_8(0x3D19, 0x64);
    AR0833_write_cmos_sensor_8(0x3D1A, 0x65);
    AR0833_write_cmos_sensor_8(0x3D1B, 0x65);
    AR0833_write_cmos_sensor_8(0x3D1C, 0x12);
    AR0833_write_cmos_sensor_8(0x3D1D, 0x30);
    AR0833_write_cmos_sensor_8(0x3D1E, 0x86);
    AR0833_write_cmos_sensor_8(0x3D1F, 0x23);
    AR0833_write_cmos_sensor_8(0x3D20, 0x01);
    AR0833_write_cmos_sensor_8(0x3D21, 0x83);
    AR0833_write_cmos_sensor_8(0x3D22, 0x00);
    AR0833_write_cmos_sensor_8(0x3D23, 0xC4);
    AR0833_write_cmos_sensor_8(0x3D24, 0x80);
    AR0833_write_cmos_sensor_8(0x3D25, 0x6E);
    AR0833_write_cmos_sensor_8(0x3D26, 0x81);
    AR0833_write_cmos_sensor_8(0x3D27, 0x59);
    AR0833_write_cmos_sensor_8(0x3D28, 0x89);
    AR0833_write_cmos_sensor_8(0x3D29, 0x6E);
    AR0833_write_cmos_sensor_8(0x3D2A, 0x80);
    AR0833_write_cmos_sensor_8(0x3D2B, 0x5F);
    AR0833_write_cmos_sensor_8(0x3D2C, 0x86);
    AR0833_write_cmos_sensor_8(0x3D2D, 0x59);
    AR0833_write_cmos_sensor_8(0x3D2E, 0x28);
    AR0833_write_cmos_sensor_8(0x3D2F, 0x02);
    AR0833_write_cmos_sensor_8(0x3D30, 0x82);
    AR0833_write_cmos_sensor_8(0x3D31, 0x5B);
    AR0833_write_cmos_sensor_8(0x3D32, 0x85);
    AR0833_write_cmos_sensor_8(0x3D33, 0x5E);
    AR0833_write_cmos_sensor_8(0x3D34, 0xBD);
    AR0833_write_cmos_sensor_8(0x3D35, 0x59);
    AR0833_write_cmos_sensor_8(0x3D36, 0x59);
    AR0833_write_cmos_sensor_8(0x3D37, 0x9D);
    AR0833_write_cmos_sensor_8(0x3D38, 0x6C);
    AR0833_write_cmos_sensor_8(0x3D39, 0x80);
    AR0833_write_cmos_sensor_8(0x3D3A, 0x6D);
    AR0833_write_cmos_sensor_8(0x3D3B, 0xAA);
    AR0833_write_cmos_sensor_8(0x3D3C, 0x10);
    AR0833_write_cmos_sensor_8(0x3D3D, 0x30);
    AR0833_write_cmos_sensor_8(0x3D3E, 0x66);
    AR0833_write_cmos_sensor_8(0x3D3F, 0x81);
    AR0833_write_cmos_sensor_8(0x3D40, 0x58);
    AR0833_write_cmos_sensor_8(0x3D41, 0x82);
    AR0833_write_cmos_sensor_8(0x3D42, 0x64);
    AR0833_write_cmos_sensor_8(0x3D43, 0x64);
    AR0833_write_cmos_sensor_8(0x3D44, 0x80);
    AR0833_write_cmos_sensor_8(0x3D45, 0x30);
    AR0833_write_cmos_sensor_8(0x3D46, 0x50);
    AR0833_write_cmos_sensor_8(0x3D47, 0xE1);
    AR0833_write_cmos_sensor_8(0x3D48, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D49, 0x83);
    AR0833_write_cmos_sensor_8(0x3D4A, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D4B, 0xA6);
    AR0833_write_cmos_sensor_8(0x3D4C, 0x30);
    AR0833_write_cmos_sensor_8(0x3D4D, 0x94);
    AR0833_write_cmos_sensor_8(0x3D4E, 0x67);
    AR0833_write_cmos_sensor_8(0x3D4F, 0x84);
    AR0833_write_cmos_sensor_8(0x3D50, 0x65);
    AR0833_write_cmos_sensor_8(0x3D51, 0x65);
    AR0833_write_cmos_sensor_8(0x3D52, 0x81);
    AR0833_write_cmos_sensor_8(0x3D53, 0x4D);
    AR0833_write_cmos_sensor_8(0x3D54, 0x68);
    AR0833_write_cmos_sensor_8(0x3D55, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D56, 0xAC);
    AR0833_write_cmos_sensor_8(0x3D57, 0x06);
    AR0833_write_cmos_sensor_8(0x3D58, 0x08);
    AR0833_write_cmos_sensor_8(0x3D59, 0x8D);
    AR0833_write_cmos_sensor_8(0x3D5A, 0x45);
    AR0833_write_cmos_sensor_8(0x3D5B, 0xA0);
    AR0833_write_cmos_sensor_8(0x3D5C, 0x45);
    AR0833_write_cmos_sensor_8(0x3D5D, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D5E, 0x83);
    AR0833_write_cmos_sensor_8(0x3D5F, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D60, 0x06);
    AR0833_write_cmos_sensor_8(0x3D61, 0x08);
    AR0833_write_cmos_sensor_8(0x3D62, 0xA5);
    AR0833_write_cmos_sensor_8(0x3D63, 0x30);
    AR0833_write_cmos_sensor_8(0x3D64, 0x90);
    AR0833_write_cmos_sensor_8(0x3D65, 0x67);
    AR0833_write_cmos_sensor_8(0x3D66, 0x64);
    AR0833_write_cmos_sensor_8(0x3D67, 0x64);
    AR0833_write_cmos_sensor_8(0x3D68, 0x89);
    AR0833_write_cmos_sensor_8(0x3D69, 0x65);
    AR0833_write_cmos_sensor_8(0x3D6A, 0x65);
    AR0833_write_cmos_sensor_8(0x3D6B, 0x81);
    AR0833_write_cmos_sensor_8(0x3D6C, 0x58);
    AR0833_write_cmos_sensor_8(0x3D6D, 0x88);
    AR0833_write_cmos_sensor_8(0x3D6E, 0x10);
    AR0833_write_cmos_sensor_8(0x3D6F, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D70, 0xCE);
    AR0833_write_cmos_sensor_8(0x3D71, 0x10);
    AR0833_write_cmos_sensor_8(0x3D72, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D73, 0x66);
    AR0833_write_cmos_sensor_8(0x3D74, 0x85);
    AR0833_write_cmos_sensor_8(0x3D75, 0x64);
    AR0833_write_cmos_sensor_8(0x3D76, 0x64);
    AR0833_write_cmos_sensor_8(0x3D77, 0x80);
    AR0833_write_cmos_sensor_8(0x3D78, 0x17);
    AR0833_write_cmos_sensor_8(0x3D79, 0x00);
    AR0833_write_cmos_sensor_8(0x3D7A, 0x80);
    AR0833_write_cmos_sensor_8(0x3D7B, 0x20);
    AR0833_write_cmos_sensor_8(0x3D7C, 0x0D);
    AR0833_write_cmos_sensor_8(0x3D7D, 0x80);
    AR0833_write_cmos_sensor_8(0x3D7E, 0x18);
    AR0833_write_cmos_sensor_8(0x3D7F, 0x0C);
    AR0833_write_cmos_sensor_8(0x3D80, 0x81);
    AR0833_write_cmos_sensor_8(0x3D81, 0x30);
    AR0833_write_cmos_sensor_8(0x3D82, 0x60);
    AR0833_write_cmos_sensor_8(0x3D83, 0x41);
    AR0833_write_cmos_sensor_8(0x3D84, 0x82);
    AR0833_write_cmos_sensor_8(0x3D85, 0x42);
    AR0833_write_cmos_sensor_8(0x3D86, 0xB2);
    AR0833_write_cmos_sensor_8(0x3D87, 0x42);
    AR0833_write_cmos_sensor_8(0x3D88, 0x80);
    AR0833_write_cmos_sensor_8(0x3D89, 0x40);
    AR0833_write_cmos_sensor_8(0x3D8A, 0x81);
    AR0833_write_cmos_sensor_8(0x3D8B, 0x40);
    AR0833_write_cmos_sensor_8(0x3D8C, 0x89);
    AR0833_write_cmos_sensor_8(0x3D8D, 0x06);
    AR0833_write_cmos_sensor_8(0x3D8E, 0xC0);
    AR0833_write_cmos_sensor_8(0x3D8F, 0x41);
    AR0833_write_cmos_sensor_8(0x3D90, 0x80);
    AR0833_write_cmos_sensor_8(0x3D91, 0x42);
    AR0833_write_cmos_sensor_8(0x3D92, 0x85);
    AR0833_write_cmos_sensor_8(0x3D93, 0x44);
    AR0833_write_cmos_sensor_8(0x3D94, 0x83);
    AR0833_write_cmos_sensor_8(0x3D95, 0x43);
    AR0833_write_cmos_sensor_8(0x3D96, 0x82);
    AR0833_write_cmos_sensor_8(0x3D97, 0x6A);
    AR0833_write_cmos_sensor_8(0x3D98, 0x83);
    AR0833_write_cmos_sensor_8(0x3D99, 0x6B);
    AR0833_write_cmos_sensor_8(0x3D9A, 0x8D);
    AR0833_write_cmos_sensor_8(0x3D9B, 0x43);
    AR0833_write_cmos_sensor_8(0x3D9C, 0x83);
    AR0833_write_cmos_sensor_8(0x3D9D, 0x44);
    AR0833_write_cmos_sensor_8(0x3D9E, 0x81);
    AR0833_write_cmos_sensor_8(0x3D9F, 0x41);
    AR0833_write_cmos_sensor_8(0x3DA0, 0x85);
    AR0833_write_cmos_sensor_8(0x3DA1, 0x06);
    AR0833_write_cmos_sensor_8(0x3DA2, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DA3, 0x88);
    AR0833_write_cmos_sensor_8(0x3DA4, 0x30);
    AR0833_write_cmos_sensor_8(0x3DA5, 0xA4);
    AR0833_write_cmos_sensor_8(0x3DA6, 0x67);
    AR0833_write_cmos_sensor_8(0x3DA7, 0x81);
    AR0833_write_cmos_sensor_8(0x3DA8, 0x42);
    AR0833_write_cmos_sensor_8(0x3DA9, 0x82);
    AR0833_write_cmos_sensor_8(0x3DAA, 0x65);
    AR0833_write_cmos_sensor_8(0x3DAB, 0x65);
    AR0833_write_cmos_sensor_8(0x3DAC, 0x81);
    AR0833_write_cmos_sensor_8(0x3DAD, 0x69);
    AR0833_write_cmos_sensor_8(0x3DAE, 0x6A);
    AR0833_write_cmos_sensor_8(0x3DAF, 0x96);
    AR0833_write_cmos_sensor_8(0x3DB0, 0x40);
    AR0833_write_cmos_sensor_8(0x3DB1, 0x82);
    AR0833_write_cmos_sensor_8(0x3DB2, 0x40);
    AR0833_write_cmos_sensor_8(0x3DB3, 0x89);
    AR0833_write_cmos_sensor_8(0x3DB4, 0x06);
    AR0833_write_cmos_sensor_8(0x3DB5, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DB6, 0x41);
    AR0833_write_cmos_sensor_8(0x3DB7, 0x80);
    AR0833_write_cmos_sensor_8(0x3DB8, 0x42);
    AR0833_write_cmos_sensor_8(0x3DB9, 0x85);
    AR0833_write_cmos_sensor_8(0x3DBA, 0x44);
    AR0833_write_cmos_sensor_8(0x3DBB, 0x83);
    AR0833_write_cmos_sensor_8(0x3DBC, 0x43);
    AR0833_write_cmos_sensor_8(0x3DBD, 0x92);
    AR0833_write_cmos_sensor_8(0x3DBE, 0x43);
    AR0833_write_cmos_sensor_8(0x3DBF, 0x83);
    AR0833_write_cmos_sensor_8(0x3DC0, 0x44);
    AR0833_write_cmos_sensor_8(0x3DC1, 0x85);
    AR0833_write_cmos_sensor_8(0x3DC2, 0x41);
    AR0833_write_cmos_sensor_8(0x3DC3, 0x81);
    AR0833_write_cmos_sensor_8(0x3DC4, 0x06);
    AR0833_write_cmos_sensor_8(0x3DC5, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DC6, 0x86);
    AR0833_write_cmos_sensor_8(0x3DC7, 0x6A);
    AR0833_write_cmos_sensor_8(0x3DC8, 0x83);
    AR0833_write_cmos_sensor_8(0x3DC9, 0x6B);
    AR0833_write_cmos_sensor_8(0x3DCA, 0x82);
    AR0833_write_cmos_sensor_8(0x3DCB, 0x42);
    AR0833_write_cmos_sensor_8(0x3DCC, 0xA0);
    AR0833_write_cmos_sensor_8(0x3DCD, 0x40);
    AR0833_write_cmos_sensor_8(0x3DCE, 0x6C);
    AR0833_write_cmos_sensor_8(0x3DCF, 0x3A);
    AR0833_write_cmos_sensor_8(0x3DD0, 0xA8);
    AR0833_write_cmos_sensor_8(0x3DD1, 0x80);
    AR0833_write_cmos_sensor_8(0x3DD2, 0x28);
    AR0833_write_cmos_sensor_8(0x3DD3, 0x30);
    AR0833_write_cmos_sensor_8(0x3DD4, 0x70);
    AR0833_write_cmos_sensor_8(0x3DD5, 0x00);
    AR0833_write_cmos_sensor_8(0x3DD6, 0x6F);
    AR0833_write_cmos_sensor_8(0x3DD7, 0x40);
    AR0833_write_cmos_sensor_8(0x3DD8, 0x14);
    AR0833_write_cmos_sensor_8(0x3DD9, 0x0E);
    AR0833_write_cmos_sensor_8(0x3DDA, 0x23);
    AR0833_write_cmos_sensor_8(0x3DDB, 0xC2);
    AR0833_write_cmos_sensor_8(0x3DDC, 0x41);
    AR0833_write_cmos_sensor_8(0x3DDD, 0x82);
    AR0833_write_cmos_sensor_8(0x3DDE, 0x42);
    AR0833_write_cmos_sensor_8(0x3DDF, 0x00);
    AR0833_write_cmos_sensor_8(0x3DE0, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DE1, 0x5D);
    AR0833_write_cmos_sensor_8(0x3DE2, 0x80);
    AR0833_write_cmos_sensor_8(0x3DE3, 0x5A);
    AR0833_write_cmos_sensor_8(0x3DE4, 0x80);
    AR0833_write_cmos_sensor_8(0x3DE5, 0x57);
    AR0833_write_cmos_sensor_8(0x3DE6, 0x84);
    AR0833_write_cmos_sensor_8(0x3DE7, 0x64);
    AR0833_write_cmos_sensor_8(0x3DE8, 0x80);
    AR0833_write_cmos_sensor_8(0x3DE9, 0x55);
    AR0833_write_cmos_sensor_8(0x3DEA, 0x86);
    AR0833_write_cmos_sensor_8(0x3DEB, 0x64);
    AR0833_write_cmos_sensor_8(0x3DEC, 0x80);
    AR0833_write_cmos_sensor_8(0x3DED, 0x65);
    AR0833_write_cmos_sensor_8(0x3DEE, 0x88);
    AR0833_write_cmos_sensor_8(0x3DEF, 0x65);
    AR0833_write_cmos_sensor_8(0x3DF0, 0x82);
    AR0833_write_cmos_sensor_8(0x3DF1, 0x54);
    AR0833_write_cmos_sensor_8(0x3DF2, 0x80);
    AR0833_write_cmos_sensor_8(0x3DF3, 0x58);
    AR0833_write_cmos_sensor_8(0x3DF4, 0x80);
    AR0833_write_cmos_sensor_8(0x3DF5, 0x00);
    AR0833_write_cmos_sensor_8(0x3DF6, 0xC0);
    AR0833_write_cmos_sensor_8(0x3DF7, 0x86);
    AR0833_write_cmos_sensor_8(0x3DF8, 0x42);
    AR0833_write_cmos_sensor_8(0x3DF9, 0x82);
    AR0833_write_cmos_sensor_8(0x3DFA, 0x10);
    AR0833_write_cmos_sensor_8(0x3DFB, 0x30);
    AR0833_write_cmos_sensor_8(0x3DFC, 0x9C);
    AR0833_write_cmos_sensor_8(0x3DFD, 0x5C);
    AR0833_write_cmos_sensor_8(0x3DFE, 0x80);
    AR0833_write_cmos_sensor_8(0x3DFF, 0x6E);
    AR0833_write_cmos_sensor_8(0x3E00, 0x86);
    AR0833_write_cmos_sensor_8(0x3E01, 0x5B);
    AR0833_write_cmos_sensor_8(0x3E02, 0x80);
    AR0833_write_cmos_sensor_8(0x3E03, 0x63);
    AR0833_write_cmos_sensor_8(0x3E04, 0x9E);
    AR0833_write_cmos_sensor_8(0x3E05, 0x59);
    AR0833_write_cmos_sensor_8(0x3E06, 0x8C);
    AR0833_write_cmos_sensor_8(0x3E07, 0x5E);
    AR0833_write_cmos_sensor_8(0x3E08, 0x8A);
    AR0833_write_cmos_sensor_8(0x3E09, 0x6C);
    AR0833_write_cmos_sensor_8(0x3E0A, 0x80);
    AR0833_write_cmos_sensor_8(0x3E0B, 0x6D);
    AR0833_write_cmos_sensor_8(0x3E0C, 0x81);
    AR0833_write_cmos_sensor_8(0x3E0D, 0x5F);
    AR0833_write_cmos_sensor_8(0x3E0E, 0x60);
    AR0833_write_cmos_sensor_8(0x3E0F, 0x61);
    AR0833_write_cmos_sensor_8(0x3E10, 0x88);
    AR0833_write_cmos_sensor_8(0x3E11, 0x10);
    AR0833_write_cmos_sensor_8(0x3E12, 0x30);
    AR0833_write_cmos_sensor_8(0x3E13, 0x66);
    AR0833_write_cmos_sensor_8(0x3E14, 0x83);
    AR0833_write_cmos_sensor_8(0x3E15, 0x6E);
    AR0833_write_cmos_sensor_8(0x3E16, 0x80);
    AR0833_write_cmos_sensor_8(0x3E17, 0x64);
    AR0833_write_cmos_sensor_8(0x3E18, 0x87);
    AR0833_write_cmos_sensor_8(0x3E19, 0x64);
    AR0833_write_cmos_sensor_8(0x3E1A, 0x30);
    AR0833_write_cmos_sensor_8(0x3E1B, 0x50);
    AR0833_write_cmos_sensor_8(0x3E1C, 0xD3);
    AR0833_write_cmos_sensor_8(0x3E1D, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E1E, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E1F, 0xAD);
    AR0833_write_cmos_sensor_8(0x3E20, 0x30);
    AR0833_write_cmos_sensor_8(0x3E21, 0x94);
    AR0833_write_cmos_sensor_8(0x3E22, 0x67);
    AR0833_write_cmos_sensor_8(0x3E23, 0x84);
    AR0833_write_cmos_sensor_8(0x3E24, 0x65);
    AR0833_write_cmos_sensor_8(0x3E25, 0x82);
    AR0833_write_cmos_sensor_8(0x3E26, 0x4D);
    AR0833_write_cmos_sensor_8(0x3E27, 0x83);
    AR0833_write_cmos_sensor_8(0x3E28, 0x65);
    AR0833_write_cmos_sensor_8(0x3E29, 0x30);
    AR0833_write_cmos_sensor_8(0x3E2A, 0x50);
    AR0833_write_cmos_sensor_8(0x3E2B, 0xA7);
    AR0833_write_cmos_sensor_8(0x3E2C, 0x43);
    AR0833_write_cmos_sensor_8(0x3E2D, 0x06);
    AR0833_write_cmos_sensor_8(0x3E2E, 0x00);
    AR0833_write_cmos_sensor_8(0x3E2F, 0x8D);
    AR0833_write_cmos_sensor_8(0x3E30, 0x45);
    AR0833_write_cmos_sensor_8(0x3E31, 0x9A);
    AR0833_write_cmos_sensor_8(0x3E32, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E33, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E34, 0x45);
    AR0833_write_cmos_sensor_8(0x3E35, 0x85);
    AR0833_write_cmos_sensor_8(0x3E36, 0x06);
    AR0833_write_cmos_sensor_8(0x3E37, 0x00);
    AR0833_write_cmos_sensor_8(0x3E38, 0x81);
    AR0833_write_cmos_sensor_8(0x3E39, 0x43);
    AR0833_write_cmos_sensor_8(0x3E3A, 0x8A);
    AR0833_write_cmos_sensor_8(0x3E3B, 0x6F);
    AR0833_write_cmos_sensor_8(0x3E3C, 0x96);
    AR0833_write_cmos_sensor_8(0x3E3D, 0x30);
    AR0833_write_cmos_sensor_8(0x3E3E, 0x90);
    AR0833_write_cmos_sensor_8(0x3E3F, 0x67);
    AR0833_write_cmos_sensor_8(0x3E40, 0x64);
    AR0833_write_cmos_sensor_8(0x3E41, 0x88);
    AR0833_write_cmos_sensor_8(0x3E42, 0x64);
    AR0833_write_cmos_sensor_8(0x3E43, 0x80);
    AR0833_write_cmos_sensor_8(0x3E44, 0x65);
    AR0833_write_cmos_sensor_8(0x3E45, 0x82);
    AR0833_write_cmos_sensor_8(0x3E46, 0x10);
    AR0833_write_cmos_sensor_8(0x3E47, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E48, 0x84);
    AR0833_write_cmos_sensor_8(0x3E49, 0x65);
    AR0833_write_cmos_sensor_8(0x3E4A, 0xEF);
    AR0833_write_cmos_sensor_8(0x3E4B, 0x10);
    AR0833_write_cmos_sensor_8(0x3E4C, 0xC0);
    AR0833_write_cmos_sensor_8(0x3E4D, 0x66);
    AR0833_write_cmos_sensor_8(0x3E4E, 0x85);
    AR0833_write_cmos_sensor_8(0x3E4F, 0x64);
    AR0833_write_cmos_sensor_8(0x3E50, 0x81);
    AR0833_write_cmos_sensor_8(0x3E51, 0x17);
    AR0833_write_cmos_sensor_8(0x3E52, 0x00);
    AR0833_write_cmos_sensor_8(0x3E53, 0x80);
    AR0833_write_cmos_sensor_8(0x3E54, 0x20);
    AR0833_write_cmos_sensor_8(0x3E55, 0x0D);
    AR0833_write_cmos_sensor_8(0x3E56, 0x80);
    AR0833_write_cmos_sensor_8(0x3E57, 0x18);
    AR0833_write_cmos_sensor_8(0x3E58, 0x0C);
    AR0833_write_cmos_sensor_8(0x3E59, 0x80);
    AR0833_write_cmos_sensor_8(0x3E5A, 0x64);
    AR0833_write_cmos_sensor_8(0x3E5B, 0x30);
    AR0833_write_cmos_sensor_8(0x3E5C, 0x60);
    AR0833_write_cmos_sensor_8(0x3E5D, 0x41);
    AR0833_write_cmos_sensor_8(0x3E5E, 0x82);
    AR0833_write_cmos_sensor_8(0x3E5F, 0x42);
    AR0833_write_cmos_sensor_8(0x3E60, 0xB2);
    AR0833_write_cmos_sensor_8(0x3E61, 0x42);
    AR0833_write_cmos_sensor_8(0x3E62, 0x80);
    AR0833_write_cmos_sensor_8(0x3E63, 0x40);
    AR0833_write_cmos_sensor_8(0x3E64, 0x82);
    AR0833_write_cmos_sensor_8(0x3E65, 0x40);
    AR0833_write_cmos_sensor_8(0x3E66, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E67, 0x45);
    AR0833_write_cmos_sensor_8(0x3E68, 0x92);
    AR0833_write_cmos_sensor_8(0x3E69, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E6A, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E6B, 0x9B);
    AR0833_write_cmos_sensor_8(0x3E6C, 0x45);
    AR0833_write_cmos_sensor_8(0x3E6D, 0x81);
    AR0833_write_cmos_sensor_8(0x3E6E, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E6F, 0x40);
    AR0833_write_cmos_sensor_8(0x3E70, 0x8C);
    AR0833_write_cmos_sensor_8(0x3E71, 0x30);
    AR0833_write_cmos_sensor_8(0x3E72, 0xA4);
    AR0833_write_cmos_sensor_8(0x3E73, 0x67);
    AR0833_write_cmos_sensor_8(0x3E74, 0x85);
    AR0833_write_cmos_sensor_8(0x3E75, 0x65);
    AR0833_write_cmos_sensor_8(0x3E76, 0x87);
    AR0833_write_cmos_sensor_8(0x3E77, 0x65);
    AR0833_write_cmos_sensor_8(0x3E78, 0x30);
    AR0833_write_cmos_sensor_8(0x3E79, 0x60);
    AR0833_write_cmos_sensor_8(0x3E7A, 0xD3);
    AR0833_write_cmos_sensor_8(0x3E7B, 0x6A);
    AR0833_write_cmos_sensor_8(0x3E7C, 0x6B);
    AR0833_write_cmos_sensor_8(0x3E7D, 0xAC);
    AR0833_write_cmos_sensor_8(0x3E7E, 0x6C);
    AR0833_write_cmos_sensor_8(0x3E7F, 0x32);
    AR0833_write_cmos_sensor_8(0x3E80, 0xA8);
    AR0833_write_cmos_sensor_8(0x3E81, 0x80);
    AR0833_write_cmos_sensor_8(0x3E82, 0x28);
    AR0833_write_cmos_sensor_8(0x3E83, 0x30);
    AR0833_write_cmos_sensor_8(0x3E84, 0x70);
    AR0833_write_cmos_sensor_8(0x3E85, 0x00);
    AR0833_write_cmos_sensor_8(0x3E86, 0x80);
    AR0833_write_cmos_sensor_8(0x3E87, 0x40);
    AR0833_write_cmos_sensor_8(0x3E88, 0x4C);
    AR0833_write_cmos_sensor_8(0x3E89, 0xBD);
    AR0833_write_cmos_sensor_8(0x3E8A, 0x00);
    AR0833_write_cmos_sensor_8(0x3E8B, 0x0E);
    AR0833_write_cmos_sensor_8(0x3E8C, 0xBE);
    AR0833_write_cmos_sensor_8(0x3E8D, 0x44);
    AR0833_write_cmos_sensor_8(0x3E8E, 0x88);
    AR0833_write_cmos_sensor_8(0x3E8F, 0x44);
    AR0833_write_cmos_sensor_8(0x3E90, 0xBC);
    AR0833_write_cmos_sensor_8(0x3E91, 0x78);
    AR0833_write_cmos_sensor_8(0x3E92, 0x09);
    AR0833_write_cmos_sensor_8(0x3E93, 0x00);
    AR0833_write_cmos_sensor_8(0x3E94, 0x89);
    AR0833_write_cmos_sensor_8(0x3E95, 0x04);
    AR0833_write_cmos_sensor_8(0x3E96, 0x80);
    AR0833_write_cmos_sensor_8(0x3E97, 0x80);
    AR0833_write_cmos_sensor_8(0x3E98, 0x02);
    AR0833_write_cmos_sensor_8(0x3E99, 0x40);
    AR0833_write_cmos_sensor_8(0x3E9A, 0x86);
    AR0833_write_cmos_sensor_8(0x3E9B, 0x09);
    AR0833_write_cmos_sensor_8(0x3E9C, 0x00);
    AR0833_write_cmos_sensor_8(0x3E9D, 0x8E);
    AR0833_write_cmos_sensor_8(0x3E9E, 0x09);
    AR0833_write_cmos_sensor_8(0x3E9F, 0x00);
    AR0833_write_cmos_sensor_8(0x3EA0, 0x80);
    AR0833_write_cmos_sensor_8(0x3EA1, 0x02);
    AR0833_write_cmos_sensor_8(0x3EA2, 0x40);
    AR0833_write_cmos_sensor_8(0x3EA3, 0x80);
    AR0833_write_cmos_sensor_8(0x3EA4, 0x04);
    AR0833_write_cmos_sensor_8(0x3EA5, 0x80);
    AR0833_write_cmos_sensor_8(0x3EA6, 0x88);
    AR0833_write_cmos_sensor_8(0x3EA7, 0x7D);
    AR0833_write_cmos_sensor_8(0x3EA8, 0x94);
    AR0833_write_cmos_sensor_8(0x3EA9, 0x86);
    AR0833_write_cmos_sensor_8(0x3EAA, 0x09);
    AR0833_write_cmos_sensor_8(0x3EAB, 0x00);
    AR0833_write_cmos_sensor_8(0x3EAC, 0x87);
    AR0833_write_cmos_sensor_8(0x3EAD, 0x7A);
    AR0833_write_cmos_sensor_8(0x3EAE, 0x00);
    AR0833_write_cmos_sensor_8(0x3EAF, 0x0E);
    AR0833_write_cmos_sensor_8(0x3EB0, 0xC3);
    AR0833_write_cmos_sensor_8(0x3EB1, 0x79);
    AR0833_write_cmos_sensor_8(0x3EB2, 0x4C);
    AR0833_write_cmos_sensor_8(0x3EB3, 0x40);
    AR0833_write_cmos_sensor_8(0x3EB4, 0xBF);
    AR0833_write_cmos_sensor_8(0x3EB5, 0x70);
    AR0833_write_cmos_sensor_8(0x3EB6, 0x00);
    AR0833_write_cmos_sensor_8(0x3EB7, 0x00);
    AR0833_write_cmos_sensor_8(0x3EB8, 0x00);
    AR0833_write_cmos_sensor_8(0x3EB9, 0x00);
    AR0833_write_cmos_sensor_8(0x3EBA, 0x00);
    AR0833_write_cmos_sensor_8(0x3EBB, 0x00);
    AR0833_write_cmos_sensor_8(0x3EBC, 0x00);
    AR0833_write_cmos_sensor_8(0x3EBD, 0x00);
    AR0833_write_cmos_sensor_8(0x3EBE, 0x00);
    AR0833_write_cmos_sensor_8(0x3EBF, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC0, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC1, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC2, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC3, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC4, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC5, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC6, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC7, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC8, 0x00);
    AR0833_write_cmos_sensor_8(0x3EC9, 0x00);
    AR0833_write_cmos_sensor_8(0x3ECA, 0x00);
    AR0833_write_cmos_sensor_8(0x3ECB, 0x00);


    AR0833_write_cmos_sensor(0x0300, 0x0004);   // VT_PIX_CLK_DIV
    AR0833_write_cmos_sensor(0x0304, 0x0004);   // PRE_PLL_CLK_DIV
    AR0833_write_cmos_sensor(0x0306, 0x0062);   // PLL_MULTIPLIER
    AR0833_write_cmos_sensor(0x0308, 0x0008);   // OP_PIX_CLK_DIV
    AR0833_write_cmos_sensor(0x0342, 0x0FCC);   // LINE_LENGTH_PCK
    AR0833_write_cmos_sensor(0x0112, 0x0808);   // CCP_DATA_FORMAT
    AR0833_write_cmos_sensor(0x31B0, 0x0060);   // FRAME_PREAMBLE
    AR0833_write_cmos_sensor(0x31B2, 0x0045);   // LINE_PREAMBLE
    AR0833_write_cmos_sensor(0x31B4, 0x4A25);   // MIPI_TIMING_0
    AR0833_write_cmos_sensor(0x31B6, 0x428F);   // MIPI_TIMING_1
    AR0833_write_cmos_sensor(0x31B8, 0x4089);   // MIPI_TIMING_2
    AR0833_write_cmos_sensor(0x31BA, 0x0209);   // MIPI_TIMING_3
    AR0833_write_cmos_sensor(0x31BC, 0x8007);   // MIPI_TIMING_4


}
#else
static void Func_Init_Setting_Raw10(void)
{


}
#endif



static void AR0833_Init_setting(void)
{
    AR0833DB("AR0833_Init_setting\n");

#ifdef AR0833_RUN_RAW8
    Func_Init_Setting_Raw8();
#else
    Func_Init_Setting_Raw10();
#endif

}   /*  AR0833_Sensor_Init  */



kal_uint16 AR0833_PowerOn(void)
{
    kal_uint16 sensor_id = 0x00;

    sensor_id = AR0833_read_cmos_sensor(0x0000);

    return sensor_id;
}
/*************************************************************************
* FUNCTION
*   AR0833_Open
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


UINT32 AR0833_Open(void)
{
    kal_uint16 sensor_id = 0;

    sensor_id = AR0833_PowerOn() ;

    AR0833DB("AR0833_Open sensor_id is %x \n", sensor_id);

    if (sensor_id != AR0833_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;


    AR0833_Init_setting();


    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   AR0833GetSensorID
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
UINT32 AR0833GetSensorID(UINT32 *sensorID)
{

    *sensorID  = AR0833_PowerOn() ;

    AR0833DB("AR0833GetSensorID sensor_id(%x)\n", *sensorID );

    // for SMT test
#if 0
    AR0833_Init_setting();
    AR0833DB("AR0833_FOCUS_init\n");
    AR0833_FOCUS_Init();
    AR0833DB("AR0833_FOCUS_Move_to\n");
    AR0833_FOCUS_Move_to(200);
    mDELAY(50);
    AR0833_FOCUS_Move_to(0);
#endif

    if (*sensorID != AR0833_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }


    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   AR0833_write_shutter
*
* DESCRIPTION
*   This function set e-shutter of AR0833 to change exposure time.
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
void AR0833_write_shutter(kal_uint16 iShutter)
{
    return;  //kk test


    //AR0833DB("AR0833_write_shutter =%d \n ",iShutter);
    AR0833_exposure_lines=iShutter;
    AR0833_write_cmos_sensor_8(0x0104, 0x01);     // GROUPED_PARAMETER_HOLD
    AR0833_write_cmos_sensor(0x0202, iShutter); /* course_integration_time */
    AR0833_write_cmos_sensor_8(0x0104, 0x00);     // GROUPED_PARAMETER_HOLD
}   /*  AR0833_write_shutter   */



/*************************************************************************
* FUNCTION
*   AR0833_read_shutter
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
UINT16 AR0833_read_shutter(void)
{
    kal_uint16 ishutter;

    ishutter = AR0833_read_cmos_sensor(0x0202); /* course_integration_time */

    AR0833DB("AR0833_read_shutter (0x%x)\n",ishutter);

    return ishutter;
}

/*************************************************************************
* FUNCTION
*   AR0833_night_mode
*
* DESCRIPTION
*   This function night mode of AR0833.
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
void AR0833_NightMode(kal_bool bEnable)
{
    // frame rate will be control by AE table

}/*    AR0833_NightMode */



/*************************************************************************
* FUNCTION
*   AR0833_Close
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
UINT32 AR0833_Close(void)
{

    return ERROR_NONE;
}    /* AR0833_Close() */

void AR0833SetFlipMirror(kal_uint8 image_mirror)
{

    switch (image_mirror)
    {
        case IMAGE_NORMAL:         //Set first grab  B
            AR0833_write_cmos_sensor_8(0x0101,0x01);
        break;
        case IMAGE_H_MIRROR:      //Set first grab  Gb
            AR0833_write_cmos_sensor_8(0x0101,0x00);
        break;
        case IMAGE_V_MIRROR:     //Set first grab  Gr
            AR0833_write_cmos_sensor_8(0x0101,0x03);
        break;
        case IMAGE_HV_MIRROR:     //Set first grab  R
            AR0833_write_cmos_sensor_8(0x0101,0x02);
        break;
    }
}


/*************************************************************************
* FUNCTION
*   AR0833_SetDummy
*
* DESCRIPTION
*   This function initialize the registers of CMOS sensor
*
* PARAMETERS
*   mode  ture : preview mode
*             false : capture mode
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/

static void AR0833_SetDummy(kal_bool mode,const kal_uint16 iDummyPixels, const kal_uint16 iDummyLines)
{

    kal_uint16 Line_length_pclk, Frame_length_lines;

    AR0833DB("Enter AR0833_SetDummy \n");
    AR0833_write_cmos_sensor_8(0x0104, 0x01);     // GROUPED_PARAMETER_HOLD
    if(mode == KAL_TRUE) //preview
    {
        Line_length_pclk   = AR0833_PV_PERIOD_PIXEL_NUMS + iDummyPixels;
        Frame_length_lines = AR0833_PV_PERIOD_LINE_NUMS  + iDummyLines;
    }
    else   //capture
    {
        Line_length_pclk   = AR0833_FULL_PERIOD_PIXEL_NUMS + iDummyPixels;
        Frame_length_lines = AR0833_FULL_PERIOD_LINE_NUMS  + iDummyLines;
    }

    AR0833DB("Enter AR0833_SetDummy Frame_length_lines=%d, Line_length_pclk=%d\n",Frame_length_lines,Line_length_pclk);
    AR0833_write_cmos_sensor(0x0340, Frame_length_lines);
    AR0833_write_cmos_sensor(0x0342, Line_length_pclk);
    AR0833_write_cmos_sensor_8(0x0104, 0x00); //Grouped Parameter Hold = 0x0

}   /*  AR0833_SetDummy */


void AR0833PreviewSetting(void)   // 1616x1212
{
    AR0833DB("AR0833PreviewSetting enter :\n ");

#if RSE1616x1212_30FPS_MCLK24_4LANE_RAW10
    Func_RSE1616x1212_30FPS_MCLK24_4LANE_RAW10();
#endif


#if RSE1616x1212_30FPS_MCLK24_4LANE_RAW8
    Func_RSE1616x1212_30FPS_MCLK24_4LANE_RAW8();
    //Func_RSE3264x2448_30FPS_MCLK24_4LANE_RAW8();


#endif

}

void AR0833CaptureSetting(void)  // 3264x2448
{
    AR0833DB("AR0833CaptureSetting enter :\n ");

#if RSE3264x2448_30FPS_MCLK24_4LANE_RAW10
    Func_RSE3264x2448_30FPS_MCLK24_4LANE_RAW10();
#endif

#if RSE3264x1836_30FPS_MCLK24_4LANE_RAW10
    Func_RSE3264x1836_30FPS_MCLK24_4LANE_RAW10();
#endif

#if RSE3264x2448_30FPS_MCLK24_4LANE_RAW8
    Func_RSE3264x2448_30FPS_MCLK24_4LANE_RAW8();
#endif

}

/*************************************************************************
* FUNCTION
*   AR0833Preview
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
UINT32 AR0833Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	AR0833DB("AR0833Preview enter:");

	AR0833PreviewSetting();

	spin_lock(&ar0833mipiraw_drv_lock);
	ar0833.sensorMode = SENSOR_MODE_PREVIEW; // Need set preview setting after capture mode
	AR0833_FeatureControl_PERIOD_PixelNum=AR0833_PV_PERIOD_PIXEL_NUMS+ ar0833.DummyPixels;
	AR0833_FeatureControl_PERIOD_LineNum=AR0833_PV_PERIOD_LINE_NUMS+ar0833.DummyLines;
	spin_unlock(&ar0833mipiraw_drv_lock);

	AR0833_write_shutter(ar0833.shutter);
	//write_AR0833_gain(ar0833.pvGain);

	//set mirror & flip
	//AR0833DB("[AR0833Preview] mirror&flip: %d \n",sensor_config_data->SensorImageMirror);
	spin_lock(&ar0833mipiraw_drv_lock);
	ar0833.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&ar0833mipiraw_drv_lock);
	AR0833SetFlipMirror(sensor_config_data->SensorImageMirror);

	AR0833DBSOFIA("[AR0833Preview]frame_len=%x\n", ((AR0833_read_cmos_sensor(0x380e)<<8)+AR0833_read_cmos_sensor(0x380f)));

	AR0833DB("AR0833Preview exit:\n");

    return ERROR_NONE;

}    /* AR0833Preview() */



/*******************************************************************************
*
********************************************************************************/
UINT32 AR0833Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
     kal_uint32 shutter = ar0833.shutter;
     kal_uint32 temp_data;
     //kal_uint32 pv_line_length , cap_line_length;

     if( SENSOR_MODE_CAPTURE== ar0833.sensorMode)
     {
         AR0833DB("AR0833Capture BusrtShot!!!\n");
     }else{
     AR0833DB("AR0833Capture enter:\n");

     //Record Preview shutter & gain
     shutter=AR0833_read_shutter();
     temp_data =  read_AR0833_gain();
     spin_lock(&ar0833mipiraw_drv_lock);
     ar0833.pvShutter =shutter;
     ar0833.sensorGlobalGain = temp_data;
     ar0833.pvGain =ar0833.sensorGlobalGain;
     spin_unlock(&ar0833mipiraw_drv_lock);

     AR0833DB("[AR0833Capture]ar0833.shutter=%d, read_pv_shutter=%d, read_pv_gain = 0x%x\n",ar0833.shutter, shutter,ar0833.sensorGlobalGain);

     // Full size setting
     AR0833CaptureSetting();

     spin_lock(&ar0833mipiraw_drv_lock);
     ar0833.sensorMode = SENSOR_MODE_CAPTURE;
     ar0833.imgMirror = sensor_config_data->SensorImageMirror;

     AR0833_FeatureControl_PERIOD_PixelNum = AR0833_FULL_PERIOD_PIXEL_NUMS + ar0833.DummyPixels;
     AR0833_FeatureControl_PERIOD_LineNum = AR0833_FULL_PERIOD_LINE_NUMS + ar0833.DummyLines;

     spin_unlock(&ar0833mipiraw_drv_lock);

     //AR0833DB("[AR0833Capture] mirror&flip: %d\n",sensor_config_data->SensorImageMirror);
     AR0833SetFlipMirror(sensor_config_data->SensorImageMirror);

     //#if defined(MT6575)||defined(MT6577)
     if(AR0833CurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_ZSD)
     {
         AR0833DB("AR0833Capture exit ZSD!!\n");
         return 0;
     }
     //#endif

 #if 0 //no need to calculate shutter from mt6589
     //calculate shutter
     pv_line_length = AR0833_PV_PERIOD_PIXEL_NUMS + ar0833.DummyPixels;
     cap_line_length = AR0833_FULL_PERIOD_PIXEL_NUMS + ar0833.DummyPixels;

     AR0833DB("[AR0833Capture]pv_line_length =%d,cap_line_length =%d\n",pv_line_length,cap_line_length);
     AR0833DB("[AR0833Capture]pv_shutter =%d\n",shutter );

     shutter =  shutter * pv_line_length / cap_line_length;
     shutter = shutter *ar0833.capPclk / ar0833.pvPclk;
     shutter *= 2; //preview bining///////////////////////////////////////

     if(shutter < 3)
         shutter = 3;

     AR0833_write_shutter(shutter);

     //gain = read_AR0833_gain();

     AR0833DB("[AR0833Capture]cap_shutter =%d , cap_read gain = 0x%x\n",shutter,read_AR0833_gain());
     //write_AR0833_gain(ar0833.sensorGlobalGain);
#endif

     AR0833DB("AR0833Capture exit:\n");
     }

     return ERROR_NONE;

}    /* AR0833Capture() */

UINT32 AR0833_GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    AR0833DB("AR0833_GetResolution!!\n");

    switch(AR0833CurrentScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorResolution->SensorPreviewWidth   = AR0833_IMAGE_SENSOR_FULL_HACTIVE;
            pSensorResolution->SensorPreviewHeight  = AR0833_IMAGE_SENSOR_FULL_VACTIVE;
            break;
        default:
            pSensorResolution->SensorPreviewWidth   = AR0833_IMAGE_SENSOR_PV_WIDTH;
            pSensorResolution->SensorPreviewHeight  = AR0833_IMAGE_SENSOR_PV_HEIGHT;
            break;
    }

    pSensorResolution->SensorFullWidth      = AR0833_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight     = AR0833_IMAGE_SENSOR_FULL_HEIGHT;

    return ERROR_NONE;

}   /* AR0833_GetResolution() */


UINT32 AR0833_GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{


    switch(ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorPreviewResolutionX=AR0833_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorPreviewResolutionY=AR0833_IMAGE_SENSOR_FULL_HEIGHT;
            pSensorInfo->SensorCameraPreviewFrameRate=15;
        break;

        default:
            pSensorInfo->SensorPreviewResolutionX=AR0833_IMAGE_SENSOR_PV_WIDTH;
               pSensorInfo->SensorPreviewResolutionY=AR0833_IMAGE_SENSOR_PV_HEIGHT;
            pSensorInfo->SensorCameraPreviewFrameRate=30;
        break;
    }

    AR0833DB("ScenarioId(%d)\n",ScenarioId);


    pSensorInfo->SensorFullResolutionX    =  AR0833_IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY    =  AR0833_IMAGE_SENSOR_FULL_HEIGHT;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=30;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;


#ifdef AR0833_RUN_RAW8
    pSensorInfo->SensorOutputDataFormat     = SENSOR_OUTPUT_FORMAT_RAW8_B;
#else
    pSensorInfo->SensorOutputDataFormat     = SENSOR_OUTPUT_FORMAT_RAW_B;
#endif

    pSensorInfo->SensorClockPolarity        = SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity        = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity        = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines  = 1;

    #ifdef MIPI_INTERFACE
           pSensorInfo->SensroInterfaceType        = SENSOR_INTERFACE_TYPE_MIPI;
       #else
           pSensorInfo->SensroInterfaceType        = SENSOR_INTERFACE_TYPE_PARALLEL;
       #endif

    pSensorInfo->CaptureDelayFrame = 1;
    pSensorInfo->PreviewDelayFrame = 2;
    pSensorInfo->VideoDelayFrame = 5;
    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;
    pSensorInfo->AEShutDelayFrame = 0;            /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 1;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=    3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = AR0833_PV_START_X;
            pSensorInfo->SensorGrabStartY = AR0833_PV_START_Y;

            #ifdef MIPI_INTERFACE
                pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
                pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
                pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 26;
                pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
                pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
                pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
                pSensorInfo->SensorPacketECCOrder = 1;
            #endif
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=    3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = AR0833_FULL_START_X;
            pSensorInfo->SensorGrabStartY = AR0833_FULL_START_X;

            #ifdef MIPI_INTERFACE
                pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
                pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
                pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 1;
                pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
                pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
                pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
                pSensorInfo->SensorPacketECCOrder = 1;
            #endif
            break;
        default:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=    3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = AR0833_PV_START_X;
            pSensorInfo->SensorGrabStartY = AR0833_PV_START_X;
            break;
    }

   // AR0833PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &AR0833_SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* AR0833_GetInfo() */


UINT32 AR0833_Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	spin_lock(&ar0833mipiraw_drv_lock);
	AR0833CurrentScenarioId = ScenarioId;
	spin_unlock(&ar0833mipiraw_drv_lock);

	AR0833DB("AR0833CurrentScenarioId=%d ScenarioId(%d)\n",AR0833CurrentScenarioId,ScenarioId);
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            AR0833Preview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            AR0833Capture(pImageWindow, pSensorConfigData);
            break;

        default:
            return ERROR_INVALID_SCENARIO_ID;

    }

    return ERROR_NONE;
} /* AR0833_Control() */


UINT32 AR0833SetVideoMode(UINT16 u2FrameRate)
{
    kal_uint16 MAX_Frame_length =0;
    //AR0833DB("AR0833SetVideoMode u2FrameRate =%d",u2FrameRate);

    if(u2FrameRate >30 || u2FrameRate <5)
        AR0833DB("Error frame rate seting");

    MAX_Frame_length = AR0833_sensor.preview_vt_clk*100000/(AR0833_PV_PERIOD_PIXEL_NUMS+AR0833_PV_dummy_pixels)/u2FrameRate;
    //if(AR0833_PV_dummy_lines <(MAX_Frame_length - AR0833_PV_PERIOD_LINE_NUMS))  //original dummy length < current needed dummy length
    if(MAX_Frame_length < AR0833_PV_PERIOD_LINE_NUMS )
        MAX_Frame_length = AR0833_PV_PERIOD_LINE_NUMS;
        AR0833_PV_dummy_lines = MAX_Frame_length - AR0833_PV_PERIOD_LINE_NUMS  ;

    AR0833_SetDummy(KAL_TRUE,AR0833_PV_dummy_pixels,AR0833_PV_dummy_lines);

    return KAL_TRUE;
}

UINT32 AR0833_FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
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
    MSDK_SENSOR_ENG_INFO_STRUCT    *pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

    AR0833DB("AR0833_FeatureControl FeatureId(%d)\n",FeatureId);

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=AR0833_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=AR0833_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
            switch(AR0833CurrentScenarioId)
            {
                case MSDK_SCENARIO_ID_CAMERA_ZSD:
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                    *pFeatureReturnPara16++= AR0833_FULL_PERIOD_PIXEL_NUMS + AR0833_dummy_pixels;//AR0833_PV_PERIOD_PIXEL_NUMS+AR0833_dummy_pixels;
                    *pFeatureReturnPara16=AR0833_FULL_PERIOD_LINE_NUMS+AR0833_dummy_lines;
                       *pFeatureParaLen=4;
                     break;

                default:
                     *pFeatureReturnPara16++= AR0833_PV_PERIOD_PIXEL_NUMS + AR0833_PV_dummy_pixels;//AR0833_PV_PERIOD_PIXEL_NUMS+AR0833_dummy_pixels;
                    *pFeatureReturnPara16=AR0833_PV_PERIOD_LINE_NUMS+AR0833_PV_dummy_lines;
                       *pFeatureParaLen=4;
                     break;
            }
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            switch(AR0833CurrentScenarioId)
            {
                case MSDK_SCENARIO_ID_CAMERA_ZSD:
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                    *pFeatureReturnPara32 = AR0833_sensor.preview_vt_clk * 100000; //19500000;
                       *pFeatureParaLen=4;
                    break;
                default:
                    *pFeatureReturnPara32 = AR0833_sensor.preview_vt_clk * 100000; //19500000;
                       *pFeatureParaLen=4;
                    break;
            }
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            AR0833_write_shutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            AR0833_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            AR0833_Set_gain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
           // AR0833_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            AR0833_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = AR0833_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
                AR0833SensorCCT[i].Addr=*pFeatureData32++;
                AR0833SensorCCT[i].Para=*pFeatureData32++;
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=AR0833SensorCCT[i].Addr;
                *pFeatureData32++=AR0833SensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
                AR0833SensorReg[i].Addr=*pFeatureData32++;
                AR0833SensorReg[i].Para=*pFeatureData32++;
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=AR0833SensorReg[i].Addr;
                *pFeatureData32++=AR0833SensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=AR0833_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, AR0833SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, AR0833SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &AR0833_SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            AR0833_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            AR0833_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=AR0833_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            AR0833_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            AR0833_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            AR0833_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 221;
            pSensorEngInfo->SensorType = CMOS_SENSOR;

#ifdef AR0833_RUN_RAW8
            pSensorEngInfo->SensorOutputDataFormat = SENSOR_OUTPUT_FORMAT_RAW8_B;
#else
            pSensorEngInfo->SensorOutputDataFormat = SENSOR_OUTPUT_FORMAT_RAW_B;
#endif

            *pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;

        case SENSOR_FEATURE_INITIALIZE_AF:
            AR0833DB("AR0833_FOCUS_Init\n");
            AR0833_FOCUS_Init();
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            AR0833DB("AR0833_FOCUS_Move_to %d\n", *pFeatureData16);
            AR0833_FOCUS_Move_to(*pFeatureData16);
            break;
        case SENSOR_FEATURE_GET_AF_STATUS:
            AR0833_FOCUS_Get_AF_Status(pFeatureReturnPara32);
            AR0833DB("SENSOR_FEATURE_GET_AF_STATUS %d\n", *pFeatureReturnPara32);
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_AF_INF:
            AR0833_FOCUS_Get_AF_Inf(pFeatureReturnPara32);
            AR0833DB("AR0833_FOCUS_Get_AF_Inf %d\n", *pFeatureReturnPara32);
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_AF_MACRO:
            AR0833_FOCUS_Get_AF_Macro(pFeatureReturnPara32);
            AR0833DB("AR0833_FOCUS_Get_AF_Macro %d\n", *pFeatureReturnPara32);
            *pFeatureParaLen=4;
            break;

        case SENSOR_FEATURE_SET_VIDEO_MODE:
            AR0833SetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            AR0833GetSensorID(pFeatureReturnPara32);
            break;

        default:
            break;
    }

    return ERROR_NONE;
}    /* AR0833_FeatureControl() */


SENSOR_FUNCTION_STRUCT    SensorFuncAR0833=
{
    AR0833_Open,
    AR0833_GetInfo,
    AR0833_GetResolution,
    AR0833_FeatureControl,
    AR0833_Control,
    AR0833_Close
};

UINT32 AR0833_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{

    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncAR0833;

    return ERROR_NONE;
}   /* SensorInit() */


