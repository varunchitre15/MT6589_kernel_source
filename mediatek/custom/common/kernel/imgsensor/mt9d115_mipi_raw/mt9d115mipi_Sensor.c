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
 *   Jackie Su (MTK02380)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 06 04 2012 yan.xu
 * [ALPS00294930] A5140's setting update by FAE's request
 * .
 *
 * 04 20 2012 zhijie.yuan
 * [ALPS00271165] modify sensor driver for MT6577
 * .
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
#include <linux/slab.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "mt9d115mipi_Sensor.h"
#include "mt9d115mipi_Camera_Sensor_para.h"
#include "mt9d115mipi_CameraCustomized.h"
#include <asm/system.h>

#define MIPI_RAW8

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);

static DEFINE_SPINLOCK(MT9D115MIPI_drv_lock);

#define MT9D115MIPI_DEBUG
#ifdef MT9D115MIPI_DEBUG
#define SENSORDB(fmt, arg...) xlog_printk(ANDROID_LOG_INFO,  "[MT9D115MIPI] ", fmt,##arg)
#else
#define SENSORDB(fmt, arg...)
#endif


struct MT9D115MIPI_SENSOR_STRUCT MT9D115MIPI_sensor= 
{	
	.i2c_write_id = 0x78,
	.i2c_read_id  = 0x79,
	.preview_vt_clk = 420,  //52M
	.capture_vt_clk = 420,
	.video_vt_clk = 420,
	.preview_3d_vt_clk = 420,
	.capture_3d_vt_clk = 420,

};
#define AUTO_FLICKER_NO 10

kal_bool MT9D115MIPI_Auto_Flicker_mode = KAL_FALSE;
kal_bool MT9D115MIPI_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 MT9D115_Frame_Length_preview = 0;


kal_uint16 MT9D115MIPI_dummy_pixels=0, MT9D115MIPI_dummy_lines=0;
kal_uint16 MT9D115MIPI_PV_dummy_pixels=0,MT9D115MIPI_PV_dummy_lines=0;
kal_uint16 MT9D115MIPI_VIDEO_dummy_pixels=0,MT9D115MIPI_VIDEO_dummy_lines=0;


kal_uint16 MT9D115MIPI_exposure_lines=0x100;
kal_uint16 MT9D115MIPI_sensor_global_gain=BASEGAIN, MT9D115MIPI_sensor_gain_base=BASEGAIN;
kal_uint16 MT9D115MIPI_sensor_gain_array[2][5]={{0x3028,0x302C, 0x302A, 0x3030, 0x302E},{0x08,0x8, 0x8, 0x8, 0x8}};

kal_uint16 MT9D115MIPI_line_pixel,MT9D115MIPI_frame_line;
kal_uint32 MT9D115MIPI_pixel_clock;

MSDK_SENSOR_CONFIG_STRUCT MT9D115MIPISensorConfigData;
kal_uint32 MT9D115MIPI_FAC_SENSOR_REG;

/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT MT9D115MIPISensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT MT9D115MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/
MSDK_SCENARIO_ID_ENUM Mt9d115_CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;

typedef enum
{
  MT9D115MIPI_MODE_PREVIEW,  
  MT9D115MIPI_MODE_CAPTURE,
  MT9D115MIPI_MODE_3D_PREVIEW,  
  MT9D115MIPI_MODE_3D_CAPTURE,    
} MT9D115MIPI_MODE;
MT9D115MIPI_MODE g_iMT9D115MIPI_Mode = MT9D115MIPI_MODE_PREVIEW;


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
/*
kal_uint16 MT9D115MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,MT9D115MIPI_sensor.i2c_write_id);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}
*/
kal_uint16 MT9D115MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,MT9D115MIPI_sensor.i2c_write_id);
	SENSORDB("MT9D115MIPI_read_cmos_sensor, addr:%x;get_byte:%x \n",addr,get_byte);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

/*
kal_uint16 MT9D115MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,MT9D115MIPI_sensor.i2c_write_id);
    return get_byte;
}
*/
void MT9D115MIPI_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{

	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 4,MT9D115MIPI_sensor.i2c_write_id);
}




/*************************************************************************
* FUNCTION
*    read_MT9D115MIPI_gain
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
kal_uint16 read_MT9D115MIPI_gain(void)
{
	volatile signed char i;
    kal_uint16 temp_reg=0, sensor_gain=0,temp_reg_base=0;
    
	temp_reg_base=MT9D115MIPISensorCCT[SENSOR_BASEGAIN].Para;

	for(i=0;i<4;i++)
	{
		temp_reg=MT9D115MIPISensorCCT[PRE_GAIN_R_INDEX+i].Para;

		if(temp_reg>=0x08 && temp_reg<=0x78)  // 0x78 is 15 by 8 ,means max gain is 15 multiple
			MT9D115MIPI_sensor_gain_array[1][PRE_GAIN_R_INDEX+i]=((((temp_reg*BASEGAIN)/8)*temp_reg_base)/8); //change to MTK basegain
		else if(temp_reg>0x78)
		    SENSORDB("error gain setting");
	}

	sensor_gain=(temp_reg_base*BASEGAIN)/8;

	return sensor_gain;   //mtk gain unit
}  /* read_MT9D115MIPI_gain */

/*******************************************************************************
* 
********************************************************************************/
void write_MT9D115MIPI_gain(kal_uint16 gain)
{
    kal_uint16 reg_gain;
  
	MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100);		//parameter_hold
	if(gain >= BASEGAIN && gain <= 15*BASEGAIN)
	{
		reg_gain = 8 * gain/BASEGAIN;        //change mtk gain base to aptina gain base
	    MT9D115MIPI_write_cmos_sensor(0x3028,reg_gain);
	    
	   // SENSORDB("reg_gain =%d,gain = %d \n",reg_gain, gain);
	}
	else
	    SENSORDB("error gain setting");
	MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000);		//parameter_hold

}

/*************************************************************************
* FUNCTION
* set_MT9D115MIPI_gain
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
kal_uint16 MT9D115MIPI_Set_gain(kal_uint16 gain)
{
      write_MT9D115MIPI_gain(gain);
    return ERROR_NONE;
}

/*******************************************************************************
* 
********************************************************************************/
void MT9D115MIPI_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=MT9D115MIPISensorReg[i].Addr; i++)
    {
        MT9D115MIPI_write_cmos_sensor(MT9D115MIPISensorReg[i].Addr, MT9D115MIPISensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=MT9D115MIPISensorReg[i].Addr; i++)
    {
        MT9D115MIPI_write_cmos_sensor(MT9D115MIPISensorReg[i].Addr, MT9D115MIPISensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        MT9D115MIPI_write_cmos_sensor(MT9D115MIPISensorCCT[i].Addr, MT9D115MIPISensorCCT[i].Para);
    }
}


/*************************************************************************
* FUNCTION
*    MT9D115MIPI_sensor_to_camera_para
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
void MT9D115MIPI_sensor_to_camera_para(void)
{
    kal_uint32    i,temp_data;
    for(i=0; 0xFFFFFFFF!=MT9D115MIPISensorReg[i].Addr; i++)
    {
    	temp_data =  MT9D115MIPI_read_cmos_sensor(MT9D115MIPISensorReg[i].Addr);
		spin_lock(&MT9D115MIPI_drv_lock);
        MT9D115MIPISensorReg[i].Para = temp_data;	    
		spin_unlock(&MT9D115MIPI_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=MT9D115MIPISensorReg[i].Addr; i++)
    {
    	temp_data = MT9D115MIPI_read_cmos_sensor(MT9D115MIPISensorReg[i].Addr);
		spin_lock(&MT9D115MIPI_drv_lock);
        MT9D115MIPISensorReg[i].Para = temp_data;	    
		spin_unlock(&MT9D115MIPI_drv_lock);
    }
}


/*************************************************************************
* FUNCTION
*    MT9D115MIPI_get_sensor_group_count
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
kal_int32  MT9D115MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void MT9D115MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void MT9D115MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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

            temp_para=MT9D115MIPISensorCCT[temp_addr].Para;

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
                
                    //temp_reg=MT9D115MIPISensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=    111;  //MT9D115MIPI_MAX_EXPOSURE_LINES;
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



kal_bool MT9D115MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
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

			temp_gain=((ItemValue*BASEGAIN+500)/1000);			//+500:get closed integer value

		  if(temp_gain>=1*BASEGAIN && temp_gain<=15*BASEGAIN)
          {
             temp_para=(temp_gain*8+BASEGAIN/2)/BASEGAIN;
          }          
          else
			  ASSERT(0);
		    spin_lock(&MT9D115MIPI_drv_lock);
            MT9D115MIPISensorCCT[temp_addr].Para = temp_para;
			spin_unlock(&MT9D115MIPI_drv_lock);
			
            MT9D115MIPI_write_cmos_sensor(MT9D115MIPISensorCCT[temp_addr].Addr,temp_para);
			spin_lock(&MT9D115MIPI_drv_lock);
           MT9D115MIPI_sensor_gain_base=read_MT9D115MIPI_gain();
		   spin_unlock(&MT9D115MIPI_drv_lock);

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
                    MT9D115MIPI_FAC_SENSOR_REG=ItemValue;
                    break;
                case 1:
                    MT9D115MIPI_write_cmos_sensor(MT9D115MIPI_FAC_SENSOR_REG,ItemValue);
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
static void MT9D115MIPI_Init_setting(void)
{
    
	SENSORDB( "MT9D115MIPI initial setting \n");

	#ifdef MIPI_RAW8
	
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
    MT9D115MIPI_write_cmos_sensor( 0x001A, 0x0050); 	// RESET_AND_MISC_CONTROL
    MT9D115MIPI_write_cmos_sensor( 0x001A, 0x0058);	// RESET_AND_MISC_CONTROL
    
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x21F9);	//PLL Control: BYPASS PLL = 8697
        
    // program the on-chip PLL
    //MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2545);	
    MT9D115MIPI_write_cmos_sensor( 0x0010, 0x0515); 
    MT9D115MIPI_write_cmos_sensor( 0x0012, 0x00F5);        //wcd = 10
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2545);	//PLL Control: TEST_BYPASS on = 9541
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2547);	//PLL Control: PLL_ENABLE on = 9543
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2447);	//PLL Control: SEL_LOCK_DET on
    mDELAY(1);               //Delay 1ms to allow PLL to lock
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2047);	//PLL Control: PLL_BYPASS off
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2046);	//PLL Control: TEST_BYPASS off
    
    // enable powerup stop
    MT9D115MIPI_write_cmos_sensor( 0x0018, 0x402D); 	// STANDBY_CONTROL
    MT9D115MIPI_write_cmos_sensor( 0x0018, 0x402C); 	// STANDBY_CONTROL
    
    mDELAY(10);
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD

	#else
	
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
    MT9D115MIPI_write_cmos_sensor( 0x001A, 0x0050); 	// RESET_AND_MISC_CONTROL
    MT9D115MIPI_write_cmos_sensor( 0x001A, 0x0058);	// RESET_AND_MISC_CONTROL
    
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x21F9);	//PLL Control: BYPASS PLL = 8697
        
    // program the on-chip PLL
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2545);	
    MT9D115MIPI_write_cmos_sensor( 0x0010, 0x0C7E); 
    MT9D115MIPI_write_cmos_sensor( 0x0012, 0x10F5);        //wcd = 10
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2545);	//PLL Control: TEST_BYPASS on = 9541
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2547);	//PLL Control: PLL_ENABLE on = 9543
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2447);	//PLL Control: SEL_LOCK_DET on
    mDELAY(1);               //Delay 1ms to allow PLL to lock
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2047);	//PLL Control: PLL_BYPASS off
    MT9D115MIPI_write_cmos_sensor( 0x0014, 0x2046);	//PLL Control: TEST_BYPASS off
    
    // enable powerup stop
    MT9D115MIPI_write_cmos_sensor( 0x0018, 0x402D); 	// STANDBY_CONTROL
    MT9D115MIPI_write_cmos_sensor( 0x0018, 0x402C); 	// STANDBY_CONTROL
    
    mDELAY(10);
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD

	#endif

    
    spin_lock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_Auto_Flicker_mode = KAL_FALSE;
	spin_unlock(&MT9D115MIPI_drv_lock);
	

}   /*  MT9D115MIPI_Sensor_Init  */

kal_uint16 MT9D115MIPI_PowerOn(void)
{
	kal_uint16 sensor_id = 0xffff;
	kal_uint16 i;
	
	for(i = 0; i < 2; i++)
	{
		
		sensor_id = MT9D115MIPI_read_cmos_sensor(0x0000);
		SENSORDB( "sensor id = 0%x \n", sensor_id);
		if(sensor_id != MT9D115MIPI_SENSOR_ID){//MT9D115MIPI_SENSOR_ID = 0x2580
			SENSORDB( "sensor id = 0%x \n", sensor_id);
		}
	}
	return sensor_id;
}
/*************************************************************************
* FUNCTION
*   MT9D115MIPIOpen
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

UINT32 MT9D115MIPIOpen(void)
{
	kal_uint16 sensor_id = 0;
	kal_uint16 temp_data;
	sensor_id = MT9D115MIPI_PowerOn() ;
	
	SENSORDB("MT9D115MIPIOpen sensor_id is %x \n", sensor_id);
    if (sensor_id != MT9D115MIPI_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;

    MT9D115MIPI_Init_setting();
	temp_data= read_MT9D115MIPI_gain();
    spin_lock(&MT9D115MIPI_drv_lock);
    MT9D115MIPI_sensor_gain_base = temp_data;
    spin_unlock(&MT9D115MIPI_drv_lock);
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   MT9D115MIPIGetSensorID
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
UINT32 MT9D115MIPIGetSensorID(UINT32 *sensorID) 
{
		*sensorID  = MT9D115MIPI_PowerOn() ;
		
		SENSORDB("MT9D115MIPIOpen sensor_id is %x\n", *sensorID );
		
 
    if (*sensorID != MT9D115MIPI_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   MT9D115MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of MT9D115MIPI to change exposure time.
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
void MT9D115MIPI_SetShutter(kal_uint16 iShutter)
{
    //SENSORDB("MT9D115MIPI_SetShutter =%d \n ",iShutter);
	unsigned long flags;
	
    if(MT9D115MIPI_exposure_lines == iShutter) {
        return;
    }
	
    spin_lock_irqsave(&MT9D115MIPI_drv_lock,flags);
	MT9D115MIPI_exposure_lines=iShutter;
    spin_unlock_irqrestore(&MT9D115MIPI_drv_lock,flags);
	
	MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
	//if(MT9D115MIPI_Auto_Flicker_mode == KAL_TRUE)
	//    MT9D115MIPI_write_cmos_sensor(0x300A, MT9D115_Frame_Length_preview +AUTO_FLICKER_NO);
	MT9D115MIPI_write_cmos_sensor(0x3012, iShutter); /* course_integration_time */
	MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD

}   /*  MT9D115MIPI_SetShutter   */



/*************************************************************************
* FUNCTION
*   MT9D115MIPI_read_shutter
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
UINT16 MT9D115MIPI_read_shutter(void)
{
    kal_uint16 ishutter;
	ishutter = MT9D115MIPI_read_cmos_sensor(0x3012); /* course_integration_time */
	return ishutter;
}

/*************************************************************************
* FUNCTION
*   MT9D115MIPI_night_mode
*
* DESCRIPTION
*   This function night mode of MT9D115MIPI.
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
void MT9D115MIPI_NightMode(kal_bool bEnable)
{
    // frame rate will be control by AE table 
     
}/*	MT9D115MIPI_NightMode */



/*************************************************************************
* FUNCTION
*   MT9D115MIPIClose
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
UINT32 MT9D115MIPIClose(void)
{

    return ERROR_NONE;
}	/* MT9D115MIPIClose() */

void MT9D115MIPI_Set_Mirror_Flip(kal_uint8 image_mirror)
{
    kal_uint16 temp_reg;
    temp_reg = MT9D115MIPI_read_cmos_sensor(0x3040)& 0xFFFC;
    
	switch (image_mirror)
	{
		case IMAGE_NORMAL:         //Set first grab  B
		    temp_reg = temp_reg | 0x0;
			MT9D115MIPI_write_cmos_sensor(0x3040,temp_reg);
		break;
		case IMAGE_H_MIRROR:      //Set first grab  Gb
		    temp_reg = temp_reg | 0x1;
			MT9D115MIPI_write_cmos_sensor(0x3040,temp_reg);
		break;
		case IMAGE_V_MIRROR:     //Set first grab  Gr
		    temp_reg = temp_reg | 0x2;
			MT9D115MIPI_write_cmos_sensor(0x3040,temp_reg);
		break;
		case IMAGE_HV_MIRROR:     //Set first grab  R
		    temp_reg = temp_reg | 0x3;
			MT9D115MIPI_write_cmos_sensor(0x3040,temp_reg);
		break;
	}
}


static void MT9D115MIPI_800x600_preview_setting(void)
{
	#ifdef MIPI_RAW8


	//808 x 608  Timing settings 30fps  -> 806 x 606
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
    // program the on-chip PLL 
	MT9D115MIPI_write_cmos_sensor( 0x3002, 0x0000); 	// MCU_BOOT_MODE
	MT9D115MIPI_write_cmos_sensor( 0x3004, 0x0000); 	// STANDBY_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x3006, 0x04BD); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x3008, 0x064D); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x300A, 0x02BB); 	// MCU_BOOT_MODE	
    MT9D115MIPI_write_cmos_sensor( 0x300C, 0x0888); 	// LINE_LENGTH_PCK 2002
	//MT9D115MIPI_write_cmos_sensor( 0x300C, 0x0888); 	// STANDBY_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x3040, 0x046C); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x321C, 0x0083); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x3400, 0xA82C); 	// MCU_BOOT_MODE
	MT9D115MIPI_write_cmos_sensor( 0x321E, 0x00D0); 	// STANDBY_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x3408, 0x0328); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x301A, 0x12DC); 	// OFIFO_CONTROL_STATUS

	#else//RAW10
	//808 x 608  Timing settings 30fps  -> 806 x 606
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
    // program the on-chip PLL 
	MT9D115MIPI_write_cmos_sensor( 0x3002, 0x0000); 	// MCU_BOOT_MODE
	MT9D115MIPI_write_cmos_sensor( 0x3004, 0x0000); 	// STANDBY_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x3006, 0x04BD); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x3008, 0x064D); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x300A, 0x02BB); 	// MCU_BOOT_MODE	
    MT9D115MIPI_write_cmos_sensor( 0x300C, 0x07D2); 	// LINE_LENGTH_PCK 2002
	//MT9D115MIPI_write_cmos_sensor( 0x300C, 0x0888); 	// STANDBY_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x3040, 0x046C); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x321C, 0x0083); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x3400, 0xAC2C); 	// MCU_BOOT_MODE
	MT9D115MIPI_write_cmos_sensor( 0x321E, 0x00D0); 	// STANDBY_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x3408, 0x03F2); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x301A, 0x12DC); 	// OFIFO_CONTROL_STATUS


	#endif
	
    spin_lock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_sensor.preview_vt_clk = 420  ; //48M VT 
    spin_unlock(&MT9D115MIPI_drv_lock);

	mDELAY(5);
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD
	//start_streaming
		
}



static void MT9D115MIPI_1280x720_preview_setting(void)
{

	//1288 x 728  Timing settings 30fps  
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
    MT9D115MIPI_write_cmos_sensor( 0x3002, 0x00F2); 	// Y_ADDR_START
    MT9D115MIPI_write_cmos_sensor( 0x3004, 0x00A2); 	// X_ADDR_START
    MT9D115MIPI_write_cmos_sensor( 0x3006, 0x03C9); 	// Y_ADDR_END
    MT9D115MIPI_write_cmos_sensor( 0x3008, 0x05A9); 	// X_ADDR_END
    MT9D115MIPI_write_cmos_sensor( 0x300A, 0x032D); 	// FRAME_LENGTH_LINES    
    MT9D115MIPI_write_cmos_sensor( 0x300C, 0x06BA); 	// LINE_LENGTH_PCK 1722
    //MT9D115MIPI_write_cmos_sensor( 0x300C, 0x0714); 	// LINE_LENGTH_PCK
    MT9D115MIPI_write_cmos_sensor( 0x3040, 0x0024); 	// READ_MODE
    MT9D115MIPI_write_cmos_sensor( 0x321C, 0x0083); 	// OFIFO_CONTROL_STATUS
    MT9D115MIPI_write_cmos_sensor( 0x3400, 0xAC2C); 	// MIPI_CONTROL
    MT9D115MIPI_write_cmos_sensor( 0x321E, 0x00D0); 	// OFIFO_CONTROL_STATUS_2
    MT9D115MIPI_write_cmos_sensor( 0x3408, 0x064A); 	// LINE_BYTE_CNT
    MT9D115MIPI_write_cmos_sensor( 0x301A, 0x12DC); 	// RESET_REGISTER
	
    spin_lock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_sensor.preview_vt_clk = 420  ; //48M VT 
    spin_unlock(&MT9D115MIPI_drv_lock);

	mDELAY(5);
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD
	//start_streaming
		
}


static void MT9D115MIPI_capture_setting(void)
{
	//[1600 X 1200]
	// Timing Settings 15fps  
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
    // program the on-chip PLL
#ifdef MIPI_RAW8    
	MT9D115MIPI_write_cmos_sensor( 0x3002, 0x0004); 	// MCU_BOOT_MODE
	MT9D115MIPI_write_cmos_sensor( 0x3004, 0x0004); 	// STANDBY_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x3006, 0x04BB); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x3008, 0x064B); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x300A, 0x0512); 	// MCU_BOOT_MODE	
	MT9D115MIPI_write_cmos_sensor( 0x300C, 0x0886); 	// STANDBY_CONTROL
	//MT9D115MIPI_write_cmos_sensor( 0x300C, 0x086D); 	// LINE_LENGTH_PCK 2157
	MT9D115MIPI_write_cmos_sensor( 0x3040, 0x0024); 	// OFIFO_CONTROL_STATUS
    MT9D115MIPI_write_cmos_sensor( 0x321C, 0x0083); 	// OFIFO_CONTROL_STATUS
    MT9D115MIPI_write_cmos_sensor( 0x3400, 0xA82C); 	// MIPI_CONTROL
    MT9D115MIPI_write_cmos_sensor( 0x321E, 0x0190); 	// OFIFO_CONTROL_STATUS_2
    MT9D115MIPI_write_cmos_sensor( 0x3408, 0x0648); 	// LINE_BYTE_CNT
    MT9D115MIPI_write_cmos_sensor( 0x301A, 0x12DC); 	// RESET_REGISTER
#else
	MT9D115MIPI_write_cmos_sensor( 0x3002, 0x0004); 	// MCU_BOOT_MODE
	MT9D115MIPI_write_cmos_sensor( 0x3004, 0x0004); 	// STANDBY_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x3006, 0x04BB); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x3008, 0x064B); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x300A, 0x0512); 	// MCU_BOOT_MODE	
	//MT9D115MIPI_write_cmos_sensor( 0x300C, 0x0886);	// STANDBY_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x300C, 0x086D); 	// LINE_LENGTH_PCK 2157
	MT9D115MIPI_write_cmos_sensor( 0x3040, 0x0024); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x321C, 0x0083); 	// OFIFO_CONTROL_STATUS
	MT9D115MIPI_write_cmos_sensor( 0x3400, 0xAC2C); 	// MIPI_CONTROL
	MT9D115MIPI_write_cmos_sensor( 0x321E, 0x00D0); 	// OFIFO_CONTROL_STATUS_2
	MT9D115MIPI_write_cmos_sensor( 0x3408, 0x07DA); 	// LINE_BYTE_CNT
	MT9D115MIPI_write_cmos_sensor( 0x301A, 0x12DC); 	// RESET_REGISTER

#endif
	
    spin_lock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_sensor.preview_vt_clk = 420  ; //48M VT 
    spin_unlock(&MT9D115MIPI_drv_lock);

	mDELAY(5);
	
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD
    spin_lock(&MT9D115MIPI_drv_lock);

	MT9D115MIPI_sensor.capture_vt_clk = 420  ; // VT 

    spin_unlock(&MT9D115MIPI_drv_lock);

	//mDELAY(10);
}

/*************************************************************************
* FUNCTION
*   MT9D115MIPI_SetDummy
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

static void MT9D115MIPI_SetDummy(kal_bool mode,const kal_uint16 iDummyPixels, const kal_uint16 iDummyLines)
{
    kal_uint16 Line_length_pclk=0, Frame_length_lines=0;
    
    SENSORDB("Enter MT9D115MIPI_SetDummy \n");
    
#if 1//JH mark    
	MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
	if(mode == KAL_TRUE) //preview
	{
	    if (MT9D115MIPI_MODE_PREVIEW == g_iMT9D115MIPI_Mode) {	    
    		Line_length_pclk   = MT9D115MIPI_PV_PERIOD_PIXEL_NUMS + iDummyPixels;
    		Frame_length_lines = MT9D115MIPI_PV_PERIOD_LINE_NUMS  + iDummyLines;
    	}
	    else if (MT9D115MIPI_MODE_3D_PREVIEW == g_iMT9D115MIPI_Mode) {	    
    		Line_length_pclk   = MT9D115MIPI_3D_PV_PERIOD_PIXEL_NUMS + iDummyPixels;
    		Frame_length_lines = MT9D115MIPI_3D_PV_PERIOD_LINE_NUMS  + iDummyLines;
    	}
    	else {
    	    SENSORDB("Error g_iMT9D115MIPI_Mode !\n");
    	}
  			
		spin_lock(&MT9D115MIPI_drv_lock);
		MT9D115_Frame_Length_preview = Frame_length_lines;
		spin_unlock(&MT9D115MIPI_drv_lock);
	}
	else   //capture
	{
	    if (MT9D115MIPI_MODE_CAPTURE == g_iMT9D115MIPI_Mode) {
    		Line_length_pclk   = MT9D115MIPI_FULL_PERIOD_PIXEL_NUMS + iDummyPixels;
    		Frame_length_lines = MT9D115MIPI_FULL_PERIOD_LINE_NUMS  + iDummyLines;
    	}
    	else if (MT9D115MIPI_MODE_3D_CAPTURE == g_iMT9D115MIPI_Mode) {
    		Line_length_pclk   = MT9D115MIPI_3D_FULL_PERIOD_PIXEL_NUMS + iDummyPixels;
    		Frame_length_lines = MT9D115MIPI_3D_FULL_PERIOD_LINE_NUMS  + iDummyLines;    	    
    	}
    	else {
    	    SENSORDB("Error g_iMT9D115MIPI_Mode !\n");
    	}
	}
	
	
    SENSORDB("Enter MT9D115MIPI_SetDummy Frame_length_lines=%d, Line_length_pclk=%d\n",Frame_length_lines,Line_length_pclk);
	MT9D115MIPI_write_cmos_sensor(0x300A, Frame_length_lines);
	MT9D115MIPI_write_cmos_sensor(0x300C, Line_length_pclk);
	MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); //Grouped Parameter Hold = 0x0
#endif//	
}   /*  MT9D115MIPI_SetDummy */


/*************************************************************************
* FUNCTION
*   MT9D115MIPIPreview
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
UINT32 MT9D115MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("Enter MT9D115MIPIPreview  \n ");
	sensor_config_data->SensorImageMirror = IMAGE_NORMAL;
    spin_lock(&MT9D115MIPI_drv_lock);
    MT9D115MIPI_PV_dummy_pixels = 0;
	MT9D115MIPI_PV_dummy_lines  = 0;  

	 if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        MT9D115MIPI_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {
        MT9D115MIPI_MPEG4_encode_mode = KAL_FALSE;
    }
    g_iMT9D115MIPI_Mode = MT9D115MIPI_MODE_PREVIEW;    
    spin_unlock(&MT9D115MIPI_drv_lock);

	MT9D115MIPI_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

	MT9D115MIPI_800x600_preview_setting();
	//MT9D115MIPI_1280x720_preview_setting();
	
	SENSORDB("MT9D115MIPIPreview DummyPixels=%d, DummyLines=%d\n",MT9D115MIPI_PV_dummy_pixels,MT9D115MIPI_PV_dummy_lines);
	MT9D115MIPI_SetDummy(KAL_TRUE,MT9D115MIPI_PV_dummy_pixels,MT9D115MIPI_PV_dummy_lines);

    image_window->GrabStartX= MT9D115MIPI_PV_START_X;
    image_window->GrabStartY= MT9D115MIPI_PV_START_Y;
    image_window->ExposureWindowWidth= MT9D115MIPI_IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight= MT9D115MIPI_IMAGE_SENSOR_PV_HEIGHT;

    //MT9D115MIPI_SetShutter(MT9D115MIPI_exposure_lines);

    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
    MT9D115MIPI_write_cmos_sensor(0x3012, MT9D115MIPI_exposure_lines); /* course_integration_time */
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD

	spin_lock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_line_pixel = MT9D115MIPI_PV_PERIOD_PIXEL_NUMS + MT9D115MIPI_PV_dummy_pixels;
	MT9D115MIPI_frame_line = MT9D115MIPI_PV_PERIOD_LINE_NUMS+MT9D115MIPI_PV_dummy_lines;
	MT9D115MIPI_pixel_clock = MT9D115MIPI_sensor.preview_vt_clk * 100000;
	spin_unlock(&MT9D115MIPI_drv_lock);
   
	memcpy(&MT9D115MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}	/* MT9D115MIPIPreview() */



/*******************************************************************************
*
********************************************************************************/
UINT32 MT9D115MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    
    spin_lock(&MT9D115MIPI_drv_lock);

	MT9D115MIPI_Auto_Flicker_mode = KAL_FALSE;
	MT9D115MIPI_MPEG4_encode_mode = KAL_FALSE;
    spin_unlock(&MT9D115MIPI_drv_lock);

	if(g_iMT9D115MIPI_Mode == MT9D115MIPI_MODE_CAPTURE) {
		return ERROR_NONE;
	}
	spin_lock(&MT9D115MIPI_drv_lock);
	g_iMT9D115MIPI_Mode = MT9D115MIPI_MODE_CAPTURE;
      
	MT9D115MIPI_dummy_pixels = 0;
	MT9D115MIPI_dummy_lines  = 0;
	
	spin_unlock(&MT9D115MIPI_drv_lock);
	SENSORDB("Enter MT9D115MIPICapture full size  \n ");
	MT9D115MIPI_capture_setting();
	SENSORDB("MT9D115MIPICapture DummyPixels=%d, DummyLines=%d\n",MT9D115MIPI_PV_dummy_pixels,MT9D115MIPI_PV_dummy_lines);
	MT9D115MIPI_SetDummy(KAL_FALSE,MT9D115MIPI_dummy_pixels,MT9D115MIPI_dummy_lines);

	image_window->GrabStartX           = MT9D115MIPI_FULL_START_X;
    image_window->GrabStartY           = MT9D115MIPI_FULL_START_Y;
    image_window->ExposureWindowWidth  = MT9D115MIPI_IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = MT9D115MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	

	spin_lock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_line_pixel = MT9D115MIPI_FULL_PERIOD_PIXEL_NUMS + MT9D115MIPI_dummy_pixels;
	MT9D115MIPI_frame_line = MT9D115MIPI_FULL_PERIOD_LINE_NUMS + MT9D115MIPI_dummy_lines;
	MT9D115MIPI_pixel_clock = MT9D115MIPI_sensor.capture_vt_clk * 100000;
	spin_unlock(&MT9D115MIPI_drv_lock);

	
	memcpy(&MT9D115MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}	/* MT9D115MIPICapture() */

/*******************************************************************************
*
********************************************************************************/

UINT32 MT9D115MIPIVIDEO(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("Enter MT9D115MIPIPreview  \n ");
    spin_lock(&MT9D115MIPI_drv_lock);
    MT9D115MIPI_VIDEO_dummy_pixels = 0;
	MT9D115MIPI_VIDEO_dummy_lines  = 0;  
	
    spin_unlock(&MT9D115MIPI_drv_lock);
	sensor_config_data->SensorImageMirror = IMAGE_NORMAL;
	
    spin_lock(&MT9D115MIPI_drv_lock);
	 if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        MT9D115MIPI_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {
        MT9D115MIPI_MPEG4_encode_mode = KAL_FALSE;
    }
    g_iMT9D115MIPI_Mode = MT9D115MIPI_MODE_PREVIEW;    
    spin_unlock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

	MT9D115MIPI_1280x720_preview_setting();
	
	SENSORDB("MT9D115MIPI Video DummyPixels=%d, DummyLines=%d\n",MT9D115MIPI_VIDEO_dummy_pixels,MT9D115MIPI_VIDEO_dummy_lines);
	MT9D115MIPI_SetDummy(KAL_TRUE,MT9D115MIPI_VIDEO_dummy_pixels,MT9D115MIPI_VIDEO_dummy_lines);

    image_window->GrabStartX= MT9D115MIPI_PV_START_X;
    image_window->GrabStartY= MT9D115MIPI_PV_START_Y;
    image_window->ExposureWindowWidth= MT9D115MIPI_IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight= MT9D115MIPI_IMAGE_SENSOR_PV_HEIGHT;

    //MT9D115MIPI_SetShutter(MT9D115MIPI_exposure_lines);

    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
    MT9D115MIPI_write_cmos_sensor(0x3012, MT9D115MIPI_exposure_lines); /* course_integration_time */
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD

	spin_lock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_line_pixel = MT9D115MIPI_VIDEO_PERIOD_PIXEL_NUMS + MT9D115MIPI_VIDEO_dummy_pixels;
	MT9D115MIPI_frame_line = MT9D115MIPI_VIDEO_PERIOD_LINE_NUMS+MT9D115MIPI_VIDEO_dummy_lines;
	MT9D115MIPI_pixel_clock = MT9D115MIPI_sensor.video_vt_clk * 100000;
	spin_unlock(&MT9D115MIPI_drv_lock);

   
	memcpy(&MT9D115MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}	/* MT9D115MIPIPreview() */


/*************************************************************************
* FUNCTION
*   MT9D115MIPI3DPreview
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
UINT32 MT9D115MIPI3DPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("Enter MT9D115MIPI3DPreview  \n ");
    spin_lock(&MT9D115MIPI_drv_lock);
    MT9D115MIPI_dummy_pixels = 0;
	MT9D115MIPI_dummy_lines  = 0;  
	
    spin_unlock(&MT9D115MIPI_drv_lock);
	sensor_config_data->SensorImageMirror = IMAGE_NORMAL;
	
    spin_lock(&MT9D115MIPI_drv_lock);
	 if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        MT9D115MIPI_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {
        MT9D115MIPI_MPEG4_encode_mode = KAL_FALSE;
    }
    g_iMT9D115MIPI_Mode = MT9D115MIPI_MODE_3D_PREVIEW;
    spin_unlock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

	MT9D115MIPI_1280x720_preview_setting();
	MT9D115MIPI_SetDummy(KAL_TRUE,MT9D115MIPI_PV_dummy_pixels,MT9D115MIPI_PV_dummy_lines);

    image_window->GrabStartX= MT9D115MIPI_3D_PV_START_X;
    image_window->GrabStartY= MT9D115MIPI_3D_PV_START_Y;
    image_window->ExposureWindowWidth= MT9D115MIPI_IMAGE_SENSOR_3D_PV_WIDTH;
    image_window->ExposureWindowHeight= MT9D115MIPI_IMAGE_SENSOR_3D_PV_HEIGHT;

    //MT9D115MIPI_SetShutter(MT9D115MIPI_exposure_lines);

    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
    MT9D115MIPI_write_cmos_sensor(0x3012, MT9D115MIPI_exposure_lines); /* course_integration_time */
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD

	memcpy(&MT9D115MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}	/* MT9D115MIPI3DPreview() */

/*******************************************************************************
*
********************************************************************************/
UINT32 MT9D115MIPI3DCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint32 shutter=MT9D115MIPI_exposure_lines;
    spin_lock(&MT9D115MIPI_drv_lock);

	MT9D115MIPI_Auto_Flicker_mode = KAL_FALSE;
	MT9D115MIPI_MPEG4_encode_mode = KAL_FALSE;

    g_iMT9D115MIPI_Mode = MT9D115MIPI_MODE_3D_CAPTURE;  
	MT9D115MIPI_dummy_pixels = 0;
	MT9D115MIPI_dummy_lines  = 0;
	
	spin_unlock(&MT9D115MIPI_drv_lock);
	SENSORDB("Enter MT9D115MIPICapture full size  \n ");
	MT9D115MIPI_capture_setting();
	
	MT9D115MIPI_SetDummy(KAL_FALSE,MT9D115MIPI_dummy_pixels,MT9D115MIPI_dummy_lines);
	SENSORDB("preview shutter =%d \n",shutter);
	shutter = shutter * (MT9D115MIPI_3D_PV_PERIOD_PIXEL_NUMS + MT9D115MIPI_PV_dummy_pixels)/(MT9D115MIPI_3D_FULL_PERIOD_PIXEL_NUMS +MT9D115MIPI_dummy_pixels);
	shutter = shutter * MT9D115MIPI_sensor.capture_3d_vt_clk / MT9D115MIPI_sensor.preview_3d_vt_clk;
	SENSORDB("capture  shutter =%d ,gain = %d\n",shutter,MT9D115MIPI_read_cmos_sensor(0x204));

	image_window->GrabStartX           = MT9D115MIPI_3D_FULL_START_X;
    image_window->GrabStartY           = MT9D115MIPI_3D_FULL_START_Y;
    image_window->ExposureWindowWidth  = MT9D115MIPI_IMAGE_SENSOR_3D_FULL_WIDTH;
    image_window->ExposureWindowHeight = MT9D115MIPI_IMAGE_SENSOR_3D_FULL_HEIGHT;


	if(shutter < 1)
	    shutter = 1;
    MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100); 	// GROUPED_PARAMETER_HOLD
	MT9D115MIPI_write_cmos_sensor(0x3012, shutter); /* course_integration_time */
	MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000); 	// GROUPED_PARAMETER_HOLD 

	
	memcpy(&MT9D115MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}	/* MT9D115MIPI3DCapture() */

UINT32 MT9D115MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth     =  MT9D115MIPI_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight    =  MT9D115MIPI_IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth  =  MT9D115MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight =  MT9D115MIPI_IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->SensorVideoWidth  =  MT9D115MIPI_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight =  MT9D115MIPI_IMAGE_SENSOR_VIDEO_HEIGHT;
    pSensorResolution->Sensor3DFullWidth     =  MT9D115MIPI_IMAGE_SENSOR_3D_FULL_WIDTH;
    pSensorResolution->Sensor3DFullHeight    =  MT9D115MIPI_IMAGE_SENSOR_3D_FULL_HEIGHT;
    pSensorResolution->Sensor3DPreviewWidth  =  MT9D115MIPI_IMAGE_SENSOR_3D_PV_WIDTH;
    pSensorResolution->Sensor3DPreviewHeight =  MT9D115MIPI_IMAGE_SENSOR_3D_PV_HEIGHT;
    pSensorResolution->Sensor3DVideoWidth  =  MT9D115MIPI_IMAGE_SENSOR_3D_PV_WIDTH;
    pSensorResolution->Sensor3DVideoHeight =  MT9D115MIPI_IMAGE_SENSOR_3D_PV_HEIGHT;

    return ERROR_NONE;
}   /* MT9D115MIPIGetResolution() */

UINT32 MT9D115MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch(ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorPreviewResolutionX=MT9D115MIPI_IMAGE_SENSOR_FULL_WIDTH;
			pSensorInfo->SensorPreviewResolutionY=MT9D115MIPI_IMAGE_SENSOR_FULL_HEIGHT;
            pSensorInfo->SensorFullResolutionX    =  MT9D115MIPI_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY    =  MT9D115MIPI_IMAGE_SENSOR_FULL_HEIGHT; 			
			pSensorInfo->SensorCameraPreviewFrameRate=15;
		break;

        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        	pSensorInfo->SensorPreviewResolutionX=MT9D115MIPI_IMAGE_SENSOR_PV_WIDTH;
       		pSensorInfo->SensorPreviewResolutionY=MT9D115MIPI_IMAGE_SENSOR_PV_HEIGHT;
            pSensorInfo->SensorFullResolutionX    =  MT9D115MIPI_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY    =  MT9D115MIPI_IMAGE_SENSOR_FULL_HEIGHT;       		
			pSensorInfo->SensorCameraPreviewFrameRate=30;            
            break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        	pSensorInfo->SensorPreviewResolutionX=MT9D115MIPI_IMAGE_SENSOR_VIDEO_WIDTH;
       		pSensorInfo->SensorPreviewResolutionY=MT9D115MIPI_IMAGE_SENSOR_VIDEO_HEIGHT;
            pSensorInfo->SensorFullResolutionX    =  MT9D115MIPI_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY    =  MT9D115MIPI_IMAGE_SENSOR_FULL_HEIGHT;       		
			pSensorInfo->SensorCameraPreviewFrameRate=30;  
			break;

        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added            
        	pSensorInfo->SensorPreviewResolutionX=MT9D115MIPI_IMAGE_SENSOR_3D_PV_WIDTH;
       		pSensorInfo->SensorPreviewResolutionY=MT9D115MIPI_IMAGE_SENSOR_3D_PV_HEIGHT;
            pSensorInfo->SensorFullResolutionX    =  MT9D115MIPI_IMAGE_SENSOR_3D_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY    =  MT9D115MIPI_IMAGE_SENSOR_3D_FULL_HEIGHT;       		
			pSensorInfo->SensorCameraPreviewFrameRate=30;            
            break;

		default:
        	pSensorInfo->SensorPreviewResolutionX=MT9D115MIPI_IMAGE_SENSOR_PV_WIDTH;
       		pSensorInfo->SensorPreviewResolutionY=MT9D115MIPI_IMAGE_SENSOR_PV_HEIGHT;
            pSensorInfo->SensorFullResolutionX    =  MT9D115MIPI_IMAGE_SENSOR_FULL_WIDTH;
            pSensorInfo->SensorFullResolutionY    =  MT9D115MIPI_IMAGE_SENSOR_FULL_HEIGHT;       		
			pSensorInfo->SensorCameraPreviewFrameRate=30;
		break;
	}
    pSensorInfo->SensorPreviewResolutionX =  MT9D115MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY =  MT9D115MIPI_IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX    =  MT9D115MIPI_IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY    =  MT9D115MIPI_IMAGE_SENSOR_FULL_HEIGHT;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
#ifdef MIPI_RAW8		
    pSensorInfo->SensorOutputDataFormat     = SENSOR_OUTPUT_FORMAT_RAW8_Gb;
#else
	pSensorInfo->SensorOutputDataFormat 	= SENSOR_OUTPUT_FORMAT_RAW_Gb;
#endif
    pSensorInfo->SensorClockPolarity        = SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity        = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity        = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines  = 1;
    
 	pSensorInfo->SensroInterfaceType        = SENSOR_INTERFACE_TYPE_MIPI;


    pSensorInfo->CaptureDelayFrame = 1; 
    pSensorInfo->PreviewDelayFrame = 2; 
    pSensorInfo->VideoDelayFrame = 5; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;      
    pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 1;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;	
	   
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        //case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4://removed
#ifdef MIPI_RAW8        
            pSensorInfo->SensorClockFreq=24;
#else
			pSensorInfo->SensorClockFreq=26;
#endif
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = MT9D115MIPI_PV_START_X; 
            pSensorInfo->SensorGrabStartY = MT9D115MIPI_PV_START_Y;    

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
#ifdef MIPI_RAW8        
			pSensorInfo->SensorClockFreq=24;
#else
			pSensorInfo->SensorClockFreq=26;
#endif

            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = MT9D115MIPI_VIDEO_START_X; 
            pSensorInfo->SensorGrabStartY = MT9D115MIPI_VIDEO_START_Y;    

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
			
			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        //case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM://removed
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
#ifdef MIPI_RAW8        
			pSensorInfo->SensorClockFreq=24;
#else
			pSensorInfo->SensorClockFreq=26;
#endif

            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = MT9D115MIPI_FULL_START_X; 
            pSensorInfo->SensorGrabStartY = MT9D115MIPI_FULL_START_X;   
            
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;

            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
#ifdef MIPI_RAW8        
			pSensorInfo->SensorClockFreq=24;
#else
			pSensorInfo->SensorClockFreq=26;
#endif

            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = MT9D115MIPI_3D_PV_START_X; 
            pSensorInfo->SensorGrabStartY = MT9D115MIPI_3D_PV_START_Y;    

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;        
        	break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
#ifdef MIPI_RAW8        
			pSensorInfo->SensorClockFreq=24;
#else
			pSensorInfo->SensorClockFreq=26;
#endif

            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = MT9D115MIPI_3D_FULL_START_X; 
            pSensorInfo->SensorGrabStartY = MT9D115MIPI_3D_FULL_START_Y;    

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;   
        	break;
        default:
#ifdef MIPI_RAW8        
			pSensorInfo->SensorClockFreq=24;
#else
			pSensorInfo->SensorClockFreq=26;
#endif

            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = MT9D115MIPI_PV_START_X; 
            pSensorInfo->SensorGrabStartY = MT9D115MIPI_PV_START_X;             
            break;
    }

   // MT9D115MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &MT9D115MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* MT9D115MIPIGetInfo() */


UINT32 MT9D115MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("Enter MT9D115MIPIControl ScenarioId=%d\n",ScenarioId);
	Mt9d115_CurrentScenarioId = ScenarioId;
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        //case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4://removed
            MT9D115MIPIPreview(pImageWindow, pSensorConfigData);
            break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			MT9D115MIPIVIDEO(pImageWindow, pSensorConfigData);
			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        //case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM://removed
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            MT9D115MIPICapture(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
            MT9D115MIPI3DPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
            MT9D115MIPI3DCapture(pImageWindow, pSensorConfigData);
            break;
        default:
            return ERROR_INVALID_SCENARIO_ID;
           
    }
    return ERROR_NONE;
} /* MT9D115MIPIControl() */


UINT32 MT9D115MIPISetVideoMode(UINT16 u2FrameRate)
{
	kal_uint16 MAX_Frame_length =0;
	//SENSORDB("MT9D115MIPISetVideoMode u2FrameRate =%d",u2FrameRate);
	
	spin_lock(&MT9D115MIPI_drv_lock);
	MT9D115MIPI_MPEG4_encode_mode = KAL_TRUE;
	spin_unlock(&MT9D115MIPI_drv_lock);

	if(u2FrameRate >30 || u2FrameRate <5)
	    SENSORDB("Error frame rate seting");

    if (MT9D115MIPI_MODE_PREVIEW == g_iMT9D115MIPI_Mode) {
    	MAX_Frame_length = MT9D115MIPI_sensor.preview_vt_clk*100000/(MT9D115MIPI_PV_PERIOD_PIXEL_NUMS+MT9D115MIPI_PV_dummy_pixels)/u2FrameRate;
    	//if(MT9D115MIPI_PV_dummy_lines <(MAX_Frame_length - MT9D115MIPI_PV_PERIOD_LINE_NUMS))  //original dummy length < current needed dummy length 
    	if(MAX_Frame_length < MT9D115MIPI_PV_PERIOD_LINE_NUMS )
    		MAX_Frame_length = MT9D115MIPI_PV_PERIOD_LINE_NUMS;
    	spin_lock(&MT9D115MIPI_drv_lock);
    	    MT9D115MIPI_PV_dummy_lines = MAX_Frame_length - MT9D115MIPI_PV_PERIOD_LINE_NUMS ;     	
    	spin_unlock(&MT9D115MIPI_drv_lock);
    }
    else if (MT9D115MIPI_MODE_3D_PREVIEW == g_iMT9D115MIPI_Mode) {
    	MAX_Frame_length = MT9D115MIPI_sensor.preview_3d_vt_clk*100000/(MT9D115MIPI_3D_PV_PERIOD_PIXEL_NUMS+MT9D115MIPI_PV_dummy_pixels)/u2FrameRate;
    	
    	if(MAX_Frame_length < MT9D115MIPI_3D_PV_PERIOD_LINE_NUMS )
    		MAX_Frame_length = MT9D115MIPI_3D_PV_PERIOD_LINE_NUMS;
    	spin_lock(&MT9D115MIPI_drv_lock);
    	    MT9D115MIPI_PV_dummy_lines = MAX_Frame_length - MT9D115MIPI_3D_PV_PERIOD_LINE_NUMS ;     	
    	spin_unlock(&MT9D115MIPI_drv_lock);        
    }
	
	MT9D115MIPI_SetDummy(KAL_TRUE,MT9D115MIPI_PV_dummy_pixels,MT9D115MIPI_PV_dummy_lines);
	
    return KAL_TRUE;
}


UINT32 MT9D115MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{

    SENSORDB("[MT9D115MIPISetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
    if(bEnable) 
	{   // enable auto flicker   
		spin_lock(&MT9D115MIPI_drv_lock);
        MT9D115MIPI_Auto_Flicker_mode = KAL_TRUE; 
		spin_unlock(&MT9D115MIPI_drv_lock);
        //if(MT9D115MIPI_MPEG4_encode_mode == KAL_TRUE) 
		{   
			// in the video mode, reset the frame rate
			MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100);        
			MT9D115MIPI_write_cmos_sensor(0x300A, MT9D115_Frame_Length_preview +AUTO_FLICKER_NO);
            MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000);        	
        }
    } else 
    {
		spin_lock(&MT9D115MIPI_drv_lock);
        MT9D115MIPI_Auto_Flicker_mode = KAL_FALSE; 
		spin_unlock(&MT9D115MIPI_drv_lock);
        if(MT9D115MIPI_MPEG4_encode_mode == KAL_TRUE) 
		{
			// in the video mode, restore the frame rate
			MT9D115MIPI_write_cmos_sensor(0x3022, 0x0100);        
			MT9D115MIPI_write_cmos_sensor(0x300A, MT9D115_Frame_Length_preview);
            MT9D115MIPI_write_cmos_sensor(0x3022, 0x0000);         	
        }
        SENSORDB("Disable Auto flicker\n");    
    }
    return TRUE;
}


UINT32 MT9D115MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=MT9D115MIPI_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=MT9D115MIPI_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
			*pFeatureReturnPara16++=MT9D115MIPI_line_pixel ;
			*pFeatureReturnPara16=MT9D115MIPI_frame_line ;
			*pFeatureParaLen=4;		

        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = MT9D115MIPI_pixel_clock;
			*pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            MT9D115MIPI_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            MT9D115MIPI_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            MT9D115MIPI_Set_gain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
           // MT9D115MIPI_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            MT9D115MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = MT9D115MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
				spin_lock(&MT9D115MIPI_drv_lock);
                MT9D115MIPISensorCCT[i].Addr=*pFeatureData32++;
                MT9D115MIPISensorCCT[i].Para=*pFeatureData32++;
				spin_unlock(&MT9D115MIPI_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=MT9D115MIPISensorCCT[i].Addr;
                *pFeatureData32++=MT9D115MIPISensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
				spin_lock(&MT9D115MIPI_drv_lock);
                MT9D115MIPISensorReg[i].Addr=*pFeatureData32++;
                MT9D115MIPISensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&MT9D115MIPI_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=MT9D115MIPISensorReg[i].Addr;
                *pFeatureData32++=MT9D115MIPISensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=MT9D115MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, MT9D115MIPISensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, MT9D115MIPISensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &MT9D115MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            MT9D115MIPI_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            MT9D115MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=MT9D115MIPI_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            MT9D115MIPI_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            MT9D115MIPI_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            MT9D115MIPI_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 221;
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
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            MT9D115MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            MT9D115MIPIGetSensorID(pFeatureReturnPara32); 
            break; 
   		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            MT9D115MIPISetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));            
	        break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* MT9D115MIPIFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncMT9D115MIPI=
{
    MT9D115MIPIOpen,
    MT9D115MIPIGetInfo,
    MT9D115MIPIGetResolution,
    MT9D115MIPIFeatureControl,
    MT9D115MIPIControl,
    MT9D115MIPIClose
};

UINT32 MT9D115MIPISensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncMT9D115MIPI;

    return ERROR_NONE;
}   /* SensorInit() */

