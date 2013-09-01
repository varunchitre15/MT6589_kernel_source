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
#include <asm/atomic.h>
#include <linux/slab.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "mt9p017mipi_Sensor.h"
#include "mt9p017mipi_Camera_Sensor_para.h"
#include "mt9p017mipi_CameraCustomized.h"
#include <asm/system.h>
static DEFINE_SPINLOCK(MT9P017MIPI_drv_lock);
//#define CAPTURE_15FPS
#define MT9P017MIPI_DEBUG
#ifdef MT9P017MIPI_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

struct MT9P017MIPI_SENSOR_STRUCT MT9P017MIPI_sensor= 
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
#define AUTO_FLICKER_NO 10

kal_bool MT9P017MIPI_Auto_Flicker_mode = KAL_FALSE;
kal_bool MT9P017MIPI_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 MT9P017_Frame_Length_preview = 0;


kal_uint16 MT9P017MIPI_dummy_pixels=0, MT9P017MIPI_dummy_lines=0;
kal_uint16 MT9P017MIPI_PV_dummy_pixels=0,MT9P017MIPI_PV_dummy_lines=0;

kal_uint16 MT9P017MIPI_exposure_lines=0x100;
kal_uint16 MT9P017MIPI_sensor_global_gain=BASEGAIN, MT9P017MIPI_sensor_gain_base=BASEGAIN;
kal_uint16 MT9P017MIPI_sensor_gain_array[2][5]={{0x0204,0x0208, 0x0206, 0x020C, 0x020A},{0x08,0x8, 0x8, 0x8, 0x8}};


MSDK_SENSOR_CONFIG_STRUCT MT9P017MIPISensorConfigData;
kal_uint32 MT9P017MIPI_FAC_SENSOR_REG;

/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT MT9P017MIPISensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT MT9P017MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/
MSDK_SCENARIO_ID_ENUM Mt9P017_CurrentScenarioId = ACDK_SCENARIO_ID_CAMERA_PREVIEW;

typedef enum
{
  MT9P017MIPI_MODE_PREVIEW,  
  MT9P017MIPI_MODE_CAPTURE  
} MT9P017MIPI_MODE;
MT9P017MIPI_MODE g_iMT9P017MIPI_Mode = MT9P017MIPI_MODE_PREVIEW;


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 MT9P017MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,MT9P017MIPI_sensor.i2c_write_id);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

void MT9P017MIPI_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{

	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 4,MT9P017MIPI_sensor.i2c_write_id);
}


kal_uint16 MT9P017MIPI_read_cmos_sensor_8(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,MT9P017MIPI_sensor.i2c_write_id);
    return get_byte;
}

void MT9P017MIPI_write_cmos_sensor_8(kal_uint32 addr, kal_uint32 para)
{

	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 3,MT9P017MIPI_sensor.i2c_write_id);
}




/*************************************************************************
* FUNCTION
*    read_MT9P017MIPI_gain
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
kal_uint16 read_MT9P017MIPI_gain(void)
{
	volatile signed char i;
    kal_uint16 temp_reg=0, sensor_gain=0,temp_reg_base=0;
    
	temp_reg_base=MT9P017MIPISensorCCT[SENSOR_BASEGAIN].Para;

	for(i=0;i<4;i++)
	{
		temp_reg=MT9P017MIPISensorCCT[PRE_GAIN_R_INDEX+i].Para;

		if(temp_reg>=0x08 && temp_reg<=0x78)  // 0x78 is 15 by 8 ,means max gain is 15 multiple
			MT9P017MIPI_sensor_gain_array[1][PRE_GAIN_R_INDEX+i]=((((temp_reg*BASEGAIN)/8)*temp_reg_base)/8); //change to MTK basegain
		else if(temp_reg>0x78)
		    SENSORDB("error gain setting");
	}

	sensor_gain=(temp_reg_base*BASEGAIN)/8;

	return sensor_gain;   //mtk gain unit
}  /* read_MT9P017MIPI_gain */

/*******************************************************************************
* 
********************************************************************************/
void write_MT9P017MIPI_gain(kal_uint16 gain)
{
    kal_uint16 reg_gain;
  
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x01);		//parameter_hold
	if(gain >= BASEGAIN && gain <= 15*BASEGAIN)
	{
		reg_gain = 8 * gain/BASEGAIN;        //change mtk gain base to aptina gain base
	    MT9P017MIPI_write_cmos_sensor(0x0204,reg_gain);
	    
	   // SENSORDB("reg_gain =%d,gain = %d \n",reg_gain, gain);
	}
	else
	    SENSORDB("error gain setting");
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x00);		//parameter_hold

}

/*************************************************************************
* FUNCTION
* set_MT9P017MIPI_gain
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
kal_uint16 MT9P07_Set_gain(kal_uint16 gain)
{
      write_MT9P017MIPI_gain(gain);

}

/*******************************************************************************
* 
********************************************************************************/
void MT9P017MIPI_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=MT9P017MIPISensorReg[i].Addr; i++)
    {
        MT9P017MIPI_write_cmos_sensor(MT9P017MIPISensorReg[i].Addr, MT9P017MIPISensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=MT9P017MIPISensorReg[i].Addr; i++)
    {
        MT9P017MIPI_write_cmos_sensor(MT9P017MIPISensorReg[i].Addr, MT9P017MIPISensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        MT9P017MIPI_write_cmos_sensor(MT9P017MIPISensorCCT[i].Addr, MT9P017MIPISensorCCT[i].Para);
    }
}


/*************************************************************************
* FUNCTION
*    MT9P017MIPI_sensor_to_camera_para
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
void MT9P017MIPI_sensor_to_camera_para(void)
{
    kal_uint32    i,temp_data;
    for(i=0; 0xFFFFFFFF!=MT9P017MIPISensorReg[i].Addr; i++)
    {
    	temp_data =  MT9P017MIPI_read_cmos_sensor(MT9P017MIPISensorReg[i].Addr);
		spin_lock(&MT9P017MIPI_drv_lock);
        MT9P017MIPISensorReg[i].Para = temp_data;	    
		spin_unlock(&MT9P017MIPI_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=MT9P017MIPISensorReg[i].Addr; i++)
    {
    	temp_data = MT9P017MIPI_read_cmos_sensor(MT9P017MIPISensorReg[i].Addr);
		spin_lock(&MT9P017MIPI_drv_lock);
        MT9P017MIPISensorReg[i].Para = temp_data;	    
		spin_unlock(&MT9P017MIPI_drv_lock);
    }
}


/*************************************************************************
* FUNCTION
*    MT9P017MIPI_get_sensor_group_count
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
kal_int32  MT9P017MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void MT9P017MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void MT9P017MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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

            temp_para=MT9P017MIPISensorCCT[temp_addr].Para;

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
                
                    //temp_reg=MT9P017MIPISensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=    111;  //MT9P017MIPI_MAX_EXPOSURE_LINES;
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



kal_bool MT9P017MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
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
		    spin_lock(&MT9P017MIPI_drv_lock);
            MT9P017MIPISensorCCT[temp_addr].Para = temp_para;
			spin_unlock(&MT9P017MIPI_drv_lock);
			
            MT9P017MIPI_write_cmos_sensor(MT9P017MIPISensorCCT[temp_addr].Addr,temp_para);
			spin_lock(&MT9P017MIPI_drv_lock);
           MT9P017MIPI_sensor_gain_base=read_MT9P017MIPI_gain();
		   spin_unlock(&MT9P017MIPI_drv_lock);

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
                    MT9P017MIPI_FAC_SENSOR_REG=ItemValue;
                    break;
                case 1:
                    MT9P017MIPI_write_cmos_sensor(MT9P017MIPI_FAC_SENSOR_REG,ItemValue);
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
static void MT9P017MIPI_Init_setting(void)
{
    kal_uint16 status = 0;
	SENSORDB( "MT9P017MIPI initial setting \n");
	MT9P017MIPI_write_cmos_sensor_8(0x0103, 0x01); 	//SOFTWARE_RESET (clears itself)
	mDELAY(5);		//Initialization Time
	
	//[Demo Initialization 1296 x 972 MCLK=24MHz, PCLK=84MHz]
	//stop_streaming
	MT9P017MIPI_write_cmos_sensor_8(0x0100, 0x00);	// MODE_SELECT
	
	//parallel_interface
	#ifdef MIPI_INTERFACE
		#ifdef RAW10
		MT9P017MIPI_write_cmos_sensor(0x301A, 0x0218);	// enable mipi interface
		MT9P017MIPI_write_cmos_sensor(0x3064, 0xB800);	// SMIA_TEST
		MT9P017MIPI_write_cmos_sensor(0x31AE, 0x0202);	// two lane
		MT9P017MIPI_write_cmos_sensor(0x0112, 0x0A0A);	// 10bit raw output
		#else
		MT9P017MIPI_write_cmos_sensor(0x301A, 0x0218);	// enable mipi interface
		MT9P017MIPI_write_cmos_sensor(0x3064, 0x0805);	// SMIA_TEST
		MT9P017MIPI_write_cmos_sensor(0x31AE, 0x0202);	// two lane
		MT9P017MIPI_write_cmos_sensor(0x0112, 0x0808);	// 8bit raw output
		#endif
	#else
	    do
	    {
			MT9P017MIPI_write_cmos_sensor(0x301A, 0x12C8);	// RESET_REGISTER
			mDELAY(2);	
			status = MT9P017MIPI_read_cmos_sensor(0x301A);  //polling status change 
			SENSORDB("MT9P017MIPI status = %x \n",status);
		}while(status != 0x12C8)   ;
		
		MT9P017MIPI_write_cmos_sensor(0x3064, 0x5840);	// SMIA_TEST
		MT9P017MIPI_write_cmos_sensor(0x31AE, 0x0101);	// SERIAL_FORMAT
	#endif


	#if 1
	//[REV4_recommended_settings]
	MT9P017MIPI_write_cmos_sensor(0x316A, 0x8400); // RESERVED 
	MT9P017MIPI_write_cmos_sensor(0x316C, 0x8400); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x316E, 0x8400); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x3EFA, 0x1A1F); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x3ED2, 0xD965); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x3ED8, 0x7F1B); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x3EDA, 0xAF11); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x3EE2, 0x0060); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x3EF2, 0xD965); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x3EF8, 0x797F); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x3EFC, 0xA8EF); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x30d4, 0x9200); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x30b2, 0xC000); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x30bc, 0x0400); // RESERVED 
	MT9P017MIPI_write_cmos_sensor(0x306E, 0xB480); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x3EFE, 0x1F0F); // RESERVED
	MT9P017MIPI_write_cmos_sensor(0x31E0, 0x1F01); // RESERVED
	//[REV4_pixel_timing]
	// @00 Jump Table
	MT9P017MIPI_write_cmos_sensor(0x3E00, 0x042F);
	MT9P017MIPI_write_cmos_sensor(0x3E02, 0xFFFF);
	MT9P017MIPI_write_cmos_sensor(0x3E04, 0xFFFF);
	MT9P017MIPI_write_cmos_sensor(0x3E06, 0xFFFF);
	// @04 Read
	MT9P017MIPI_write_cmos_sensor(0x3E08, 0x8071);
	MT9P017MIPI_write_cmos_sensor(0x3E0A, 0x7281);
	MT9P017MIPI_write_cmos_sensor(0x3E0C, 0x4011);
	MT9P017MIPI_write_cmos_sensor(0x3E0E, 0x8010);
	MT9P017MIPI_write_cmos_sensor(0x3E10, 0x60A5);
	MT9P017MIPI_write_cmos_sensor(0x3E12, 0x4080);
	MT9P017MIPI_write_cmos_sensor(0x3E14, 0x4180);
	MT9P017MIPI_write_cmos_sensor(0x3E16, 0x0018);
	MT9P017MIPI_write_cmos_sensor(0x3E18, 0x46B7);
	MT9P017MIPI_write_cmos_sensor(0x3E1A, 0x4994);
	MT9P017MIPI_write_cmos_sensor(0x3E1C, 0x4997);
	MT9P017MIPI_write_cmos_sensor(0x3E1E, 0x4682);
	MT9P017MIPI_write_cmos_sensor(0x3E20, 0x0018);
	MT9P017MIPI_write_cmos_sensor(0x3E22, 0x4241);
	MT9P017MIPI_write_cmos_sensor(0x3E24, 0x8000);
	MT9P017MIPI_write_cmos_sensor(0x3E26, 0x1880);
	MT9P017MIPI_write_cmos_sensor(0x3E28, 0x4785);
	MT9P017MIPI_write_cmos_sensor(0x3E2A, 0x4992);
	MT9P017MIPI_write_cmos_sensor(0x3E2C, 0x4997);
	MT9P017MIPI_write_cmos_sensor(0x3E2E, 0x4780);
	MT9P017MIPI_write_cmos_sensor(0x3E30, 0x4D80);
	MT9P017MIPI_write_cmos_sensor(0x3E32, 0x100C);
	MT9P017MIPI_write_cmos_sensor(0x3E34, 0x8000);
	MT9P017MIPI_write_cmos_sensor(0x3E36, 0x184A);
	MT9P017MIPI_write_cmos_sensor(0x3E38, 0x8042);
	MT9P017MIPI_write_cmos_sensor(0x3E3A, 0x001A);
	MT9P017MIPI_write_cmos_sensor(0x3E3C, 0x9610);
	MT9P017MIPI_write_cmos_sensor(0x3E3E, 0x0C80);
	MT9P017MIPI_write_cmos_sensor(0x3E40, 0x4DC6);
	MT9P017MIPI_write_cmos_sensor(0x3E42, 0x4A80);
	MT9P017MIPI_write_cmos_sensor(0x3E44, 0x0018);
	MT9P017MIPI_write_cmos_sensor(0x3E46, 0x8042);
	MT9P017MIPI_write_cmos_sensor(0x3E48, 0x8041);
	MT9P017MIPI_write_cmos_sensor(0x3E4A, 0x0018);
	MT9P017MIPI_write_cmos_sensor(0x3E4C, 0x804B);
	MT9P017MIPI_write_cmos_sensor(0x3E4E, 0xB74B);
	MT9P017MIPI_write_cmos_sensor(0x3E50, 0x8010);
	MT9P017MIPI_write_cmos_sensor(0x3E52, 0x6056);
	MT9P017MIPI_write_cmos_sensor(0x3E54, 0x001C);
	MT9P017MIPI_write_cmos_sensor(0x3E56, 0x8211);
	MT9P017MIPI_write_cmos_sensor(0x3E58, 0x8056);
	MT9P017MIPI_write_cmos_sensor(0x3E5A, 0x827C);
	MT9P017MIPI_write_cmos_sensor(0x3E5C, 0x0970);
	MT9P017MIPI_write_cmos_sensor(0x3E5E, 0x8082);
	MT9P017MIPI_write_cmos_sensor(0x3E60, 0x7281);
	MT9P017MIPI_write_cmos_sensor(0x3E62, 0x4C40);
	MT9P017MIPI_write_cmos_sensor(0x3E64, 0x8E4D);
	MT9P017MIPI_write_cmos_sensor(0x3E66, 0x8110);
	MT9P017MIPI_write_cmos_sensor(0x3E68, 0x0CAF);
	MT9P017MIPI_write_cmos_sensor(0x3E6A, 0x4D80);
	MT9P017MIPI_write_cmos_sensor(0x3E6C, 0x100C);
	MT9P017MIPI_write_cmos_sensor(0x3E6E, 0x8440);
	MT9P017MIPI_write_cmos_sensor(0x3E70, 0x4C81);
	MT9P017MIPI_write_cmos_sensor(0x3E72, 0x7C5F);
	MT9P017MIPI_write_cmos_sensor(0x3E74, 0x7000);
	MT9P017MIPI_write_cmos_sensor(0x3E76, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E78, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E7A, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E7C, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E7E, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E80, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E82, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E84, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E86, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E88, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E8A, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E8C, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E8E, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E90, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E92, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E94, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E96, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E98, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E9A, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E9C, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3E9E, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EA0, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EA2, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EA4, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EA6, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EA8, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EAA, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EAC, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EAE, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EB0, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EB2, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EB4, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EB6, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EB8, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EBA, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EBC, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EBE, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EC0, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EC2, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EC4, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EC6, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3EC8, 0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3ECA, 0x0000);

	MT9P017MIPI_write_cmos_sensor(0x3170,0x2150);
	MT9P017MIPI_write_cmos_sensor(0x317A,0x0150);
	MT9P017MIPI_write_cmos_sensor(0x3ECC,0x2200);
	MT9P017MIPI_write_cmos_sensor(0x3174,0x0000);
	MT9P017MIPI_write_cmos_sensor(0x3176,0X0000);

	MT9P017MIPI_write_cmos_sensor(0x31B0, 0x00C4);
	MT9P017MIPI_write_cmos_sensor(0x31B2, 0x0064);
	MT9P017MIPI_write_cmos_sensor(0x31B4, 0x0E77);
	MT9P017MIPI_write_cmos_sensor(0x31B6, 0x0D24);
	MT9P017MIPI_write_cmos_sensor(0x31B8, 0x020E);
	MT9P017MIPI_write_cmos_sensor(0x31BA, 0x0710);
	MT9P017MIPI_write_cmos_sensor(0x31BC, 0x2A0D);
	MT9P017MIPI_write_cmos_sensor(0x31BE, 0xC007);
	#else
	//REV4_recommended_settings
	MT9P017MIPI_write_cmos_sensor(0x316A, 0x8400);	// DAC_FBIAS
	MT9P017MIPI_write_cmos_sensor(0x316C, 0x8400);	// DAC_TXLO
	MT9P017MIPI_write_cmos_sensor(0x316E, 0x8400);	// DAC_ECL
	MT9P017MIPI_write_cmos_sensor(0x3EFA, 0x1B1F);	// DAC_LD_ECL
	MT9P017MIPI_write_cmos_sensor(0x3ED2, 0xD965);	// DAC_LD_6_7
	MT9P017MIPI_write_cmos_sensor(0x3ED8, 0x7F1B);	// DAC_LD_12_13
	MT9P017MIPI_write_cmos_sensor(0x3EDA, 0xAF11);	// DAC_LD_14_15
	//MT9P017MIPI_write_cmos_sensor(0x3EDE, 0xB000);	// DAC_LD_18_19
	MT9P017MIPI_write_cmos_sensor(0x3EE2, 0x0060);	// DAC_LD_22_23
	MT9P017MIPI_write_cmos_sensor(0x3EF2, 0xD965);	// DAC_LP_6_7
	MT9P017MIPI_write_cmos_sensor(0x3EF8, 0x797F);	// DAC_LD_TXHI
	MT9P017MIPI_write_cmos_sensor(0x3EFC, 0xA4EF);	// DAC_LD_FBIAS
	MT9P017MIPI_write_cmos_sensor(0x3EFE, 0x1F0F);	// DAC_LD_TXLO
	MT9P017MIPI_write_cmos_sensor(0x31E0, 0x1F01);	// PIX_DEF_ID
	// dynamic power disable
	MT9P017MIPI_write_cmos_sensor(0x3170, 0x2150);	// ANALOG_CONTROL
	MT9P017MIPI_write_cmos_sensor(0x317A, 0x0150);	// ANALOG_CONTROL6
	MT9P017MIPI_write_cmos_sensor(0x3ECC, 0x2200);	// DAC_LD_0_1
	MT9P017MIPI_write_cmos_sensor(0x3174, 0x0000);	// ANALOG_CONTROL3
	MT9P017MIPI_write_cmos_sensor(0x3176, 0x0000);	// ANALOG_CONTROL4

	MT9P017MIPI_write_cmos_sensor(0x31BE,0x2007);
	//REV4_pixel_timing
	MT9P017MIPI_write_cmos_sensor(0x3E00, 0x0429);	// DYNAMIC_SEQRAM_00
	MT9P017MIPI_write_cmos_sensor(0x3E02, 0xFFFF);	// DYNAMIC_SEQRAM_02
	MT9P017MIPI_write_cmos_sensor(0x3E04, 0xFFFF);	// DYNAMIC_SEQRAM_04
	MT9P017MIPI_write_cmos_sensor(0x3E06, 0xFFFF);	// DYNAMIC_SEQRAM_06
	MT9P017MIPI_write_cmos_sensor(0x3E08, 0x8071);	// DYNAMIC_SEQRAM_08
	MT9P017MIPI_write_cmos_sensor(0x3E0A, 0x7281);	// DYNAMIC_SEQRAM_0A
	MT9P017MIPI_write_cmos_sensor(0x3E0C, 0x0043);	// DYNAMIC_SEQRAM_0C
	MT9P017MIPI_write_cmos_sensor(0x3E0E, 0x5313);	// DYNAMIC_SEQRAM_0E
	MT9P017MIPI_write_cmos_sensor(0x3E10, 0x0087);	// DYNAMIC_SEQRAM_10
	MT9P017MIPI_write_cmos_sensor(0x3E12, 0x1060);	// DYNAMIC_SEQRAM_12
	MT9P017MIPI_write_cmos_sensor(0x3E14, 0x8540);	// DYNAMIC_SEQRAM_14
	MT9P017MIPI_write_cmos_sensor(0x3E16, 0xA200);	// DYNAMIC_SEQRAM_16
	MT9P017MIPI_write_cmos_sensor(0x3E18, 0x1890);	// DYNAMIC_SEQRAM_18
	MT9P017MIPI_write_cmos_sensor(0x3E1A, 0x57A0);	// DYNAMIC_SEQRAM_1A
	MT9P017MIPI_write_cmos_sensor(0x3E1C, 0x49A6);	// DYNAMIC_SEQRAM_1C
	MT9P017MIPI_write_cmos_sensor(0x3E1E, 0x4988);	// DYNAMIC_SEQRAM_1E
	MT9P017MIPI_write_cmos_sensor(0x3E20, 0x4681);	// DYNAMIC_SEQRAM_20
	MT9P017MIPI_write_cmos_sensor(0x3E22, 0x4200);	// DYNAMIC_SEQRAM_22
	MT9P017MIPI_write_cmos_sensor(0x3E24, 0x828B);	// DYNAMIC_SEQRAM_24
	MT9P017MIPI_write_cmos_sensor(0x3E26, 0x499C);	// DYNAMIC_SEQRAM_26
	MT9P017MIPI_write_cmos_sensor(0x3E28, 0x498E);	// DYNAMIC_SEQRAM_28
	MT9P017MIPI_write_cmos_sensor(0x3E2A, 0x4788);	// DYNAMIC_SEQRAM_2A
	MT9P017MIPI_write_cmos_sensor(0x3E2C, 0x4D80);	// DYNAMIC_SEQRAM_2C
	MT9P017MIPI_write_cmos_sensor(0x3E2E, 0x100C);	// DYNAMIC_SEQRAM_2E
	MT9P017MIPI_write_cmos_sensor(0x3E30, 0x0406);	// DYNAMIC_SEQRAM_30
	MT9P017MIPI_write_cmos_sensor(0x3E32, 0x9110);	// DYNAMIC_SEQRAM_32
	MT9P017MIPI_write_cmos_sensor(0x3E34, 0x0C8C);	// DYNAMIC_SEQRAM_34
	MT9P017MIPI_write_cmos_sensor(0x3E36, 0x4DB9);	// DYNAMIC_SEQRAM_36
	MT9P017MIPI_write_cmos_sensor(0x3E38, 0x4A42);	// DYNAMIC_SEQRAM_38
	MT9P017MIPI_write_cmos_sensor(0x3E3A, 0x8341);	// DYNAMIC_SEQRAM_3A
	MT9P017MIPI_write_cmos_sensor(0x3E3C, 0x814B);	// DYNAMIC_SEQRAM_3C
	MT9P017MIPI_write_cmos_sensor(0x3E3E, 0xB24B);	// DYNAMIC_SEQRAM_3E
	MT9P017MIPI_write_cmos_sensor(0x3E40, 0x8056);	// DYNAMIC_SEQRAM_40
	MT9P017MIPI_write_cmos_sensor(0x3E42, 0x8000);	// DYNAMIC_SEQRAM_42
	MT9P017MIPI_write_cmos_sensor(0x3E44, 0x1C81);	// DYNAMIC_SEQRAM_44
	MT9P017MIPI_write_cmos_sensor(0x3E46, 0x10E0);	// DYNAMIC_SEQRAM_46
	MT9P017MIPI_write_cmos_sensor(0x3E48, 0x8013);	// DYNAMIC_SEQRAM_48
	MT9P017MIPI_write_cmos_sensor(0x3E4A, 0x001C);	// DYNAMIC_SEQRAM_4A
	MT9P017MIPI_write_cmos_sensor(0x3E4C, 0x0082);	// DYNAMIC_SEQRAM_4C
	MT9P017MIPI_write_cmos_sensor(0x3E4E, 0x7C09);	// DYNAMIC_SEQRAM_4E
	MT9P017MIPI_write_cmos_sensor(0x3E50, 0x7000);	// DYNAMIC_SEQRAM_50
	MT9P017MIPI_write_cmos_sensor(0x3E52, 0x8082);	// DYNAMIC_SEQRAM_52
	MT9P017MIPI_write_cmos_sensor(0x3E54, 0x7281);	// DYNAMIC_SEQRAM_54
	MT9P017MIPI_write_cmos_sensor(0x3E56, 0x4C40);	// DYNAMIC_SEQRAM_56
	MT9P017MIPI_write_cmos_sensor(0x3E58, 0x8E4D);	// DYNAMIC_SEQRAM_58
	MT9P017MIPI_write_cmos_sensor(0x3E5A, 0x8110);	// DYNAMIC_SEQRAM_5A
	MT9P017MIPI_write_cmos_sensor(0x3E5C, 0x0CAF);	// DYNAMIC_SEQRAM_5C
	MT9P017MIPI_write_cmos_sensor(0x3E5E, 0x4D80);	// DYNAMIC_SEQRAM_5E
	MT9P017MIPI_write_cmos_sensor(0x3E60, 0x100C);	// DYNAMIC_SEQRAM_60
	MT9P017MIPI_write_cmos_sensor(0x3E62, 0x8440);	// DYNAMIC_SEQRAM_62
	MT9P017MIPI_write_cmos_sensor(0x3E64, 0x4C81);	// DYNAMIC_SEQRAM_64
	MT9P017MIPI_write_cmos_sensor(0x3E66, 0x7C53);	// DYNAMIC_SEQRAM_66
	MT9P017MIPI_write_cmos_sensor(0x3E68, 0x7000);	// DYNAMIC_SEQRAM_68
	MT9P017MIPI_write_cmos_sensor(0x3E6A, 0x0000);	// DYNAMIC_SEQRAM_6A
	MT9P017MIPI_write_cmos_sensor(0x3E6C, 0x0000);	// DYNAMIC_SEQRAM_6C
	MT9P017MIPI_write_cmos_sensor(0x3E6E, 0x0000);	// DYNAMIC_SEQRAM_6E
	MT9P017MIPI_write_cmos_sensor(0x3E70, 0x0000);	// DYNAMIC_SEQRAM_70
	MT9P017MIPI_write_cmos_sensor(0x3E72, 0x0000);	// DYNAMIC_SEQRAM_72
	MT9P017MIPI_write_cmos_sensor(0x3E74, 0x0000);	// DYNAMIC_SEQRAM_74
	MT9P017MIPI_write_cmos_sensor(0x3E76, 0x0000);	// DYNAMIC_SEQRAM_76
	MT9P017MIPI_write_cmos_sensor(0x3E78, 0x0000);	// DYNAMIC_SEQRAM_78
	MT9P017MIPI_write_cmos_sensor(0x3E7A, 0x0000);	// DYNAMIC_SEQRAM_7A
	MT9P017MIPI_write_cmos_sensor(0x3E7C, 0x0000);	// DYNAMIC_SEQRAM_7C
	MT9P017MIPI_write_cmos_sensor(0x3E7E, 0x0000);	// DYNAMIC_SEQRAM_7E
	MT9P017MIPI_write_cmos_sensor(0x3E80, 0x0000);	// DYNAMIC_SEQRAM_80
	MT9P017MIPI_write_cmos_sensor(0x3E82, 0x0000);	// DYNAMIC_SEQRAM_82
	MT9P017MIPI_write_cmos_sensor(0x3E84, 0x0000);	// DYNAMIC_SEQRAM_84
	MT9P017MIPI_write_cmos_sensor(0x3E86, 0x0000);	// DYNAMIC_SEQRAM_86
	MT9P017MIPI_write_cmos_sensor(0x3E88, 0x0000);	// DYNAMIC_SEQRAM_88
	MT9P017MIPI_write_cmos_sensor(0x3E8A, 0x0000);	// DYNAMIC_SEQRAM_8A
	MT9P017MIPI_write_cmos_sensor(0x3E8C, 0x0000);	// DYNAMIC_SEQRAM_8C
	MT9P017MIPI_write_cmos_sensor(0x3E8E, 0x0000);	// DYNAMIC_SEQRAM_8E
	MT9P017MIPI_write_cmos_sensor(0x3E90, 0x0000);	// DYNAMIC_SEQRAM_90
	MT9P017MIPI_write_cmos_sensor(0x3E92, 0x0000);	// DYNAMIC_SEQRAM_92
	MT9P017MIPI_write_cmos_sensor(0x3E94, 0x0000);	// DYNAMIC_SEQRAM_94
	MT9P017MIPI_write_cmos_sensor(0x3E96, 0x0000);	// DYNAMIC_SEQRAM_96
	MT9P017MIPI_write_cmos_sensor(0x3E98, 0x0000);	// DYNAMIC_SEQRAM_98
	MT9P017MIPI_write_cmos_sensor(0x3E9A, 0x0000);	// DYNAMIC_SEQRAM_9A
	MT9P017MIPI_write_cmos_sensor(0x3E9C, 0x0000);	// DYNAMIC_SEQRAM_9C
	MT9P017MIPI_write_cmos_sensor(0x3E9E, 0x0000);	// DYNAMIC_SEQRAM_9E
	MT9P017MIPI_write_cmos_sensor(0x3EA0, 0x0000);	// DYNAMIC_SEQRAM_A0
	MT9P017MIPI_write_cmos_sensor(0x3EA2, 0x0000);	// DYNAMIC_SEQRAM_A2
	MT9P017MIPI_write_cmos_sensor(0x3EA4, 0x0000);	// DYNAMIC_SEQRAM_A4
	MT9P017MIPI_write_cmos_sensor(0x3EA6, 0x0000);	// DYNAMIC_SEQRAM_A6
	MT9P017MIPI_write_cmos_sensor(0x3EA8, 0x0000);	// DYNAMIC_SEQRAM_A8
	MT9P017MIPI_write_cmos_sensor(0x3EAA, 0x0000);	// DYNAMIC_SEQRAM_AA
	MT9P017MIPI_write_cmos_sensor(0x3EAC, 0x0000);	// DYNAMIC_SEQRAM_AC
	MT9P017MIPI_write_cmos_sensor(0x3EAE, 0x0000);	// DYNAMIC_SEQRAM_AE
	MT9P017MIPI_write_cmos_sensor(0x3EB0, 0x0000);	// DYNAMIC_SEQRAM_B0
	MT9P017MIPI_write_cmos_sensor(0x3EB2, 0x0000);	// DYNAMIC_SEQRAM_B2
	MT9P017MIPI_write_cmos_sensor(0x3EB4, 0x0000);	// DYNAMIC_SEQRAM_B4
	MT9P017MIPI_write_cmos_sensor(0x3EB6, 0x0000);	// DYNAMIC_SEQRAM_B6
	MT9P017MIPI_write_cmos_sensor(0x3EB8, 0x0000);	// DYNAMIC_SEQRAM_B8
	MT9P017MIPI_write_cmos_sensor(0x3EBA, 0x0000);	// DYNAMIC_SEQRAM_BA
	MT9P017MIPI_write_cmos_sensor(0x3EBC, 0x0000);	// DYNAMIC_SEQRAM_BC
	MT9P017MIPI_write_cmos_sensor(0x3EBE, 0x0000);	// DYNAMIC_SEQRAM_BE
	MT9P017MIPI_write_cmos_sensor(0x3EC0, 0x0000);	// DYNAMIC_SEQRAM_C0
	MT9P017MIPI_write_cmos_sensor(0x3EC2, 0x0000);	// DYNAMIC_SEQRAM_C2
	MT9P017MIPI_write_cmos_sensor(0x3EC4, 0x0000);	// DYNAMIC_SEQRAM_C4
	MT9P017MIPI_write_cmos_sensor(0x3EC6, 0x0000);	// DYNAMIC_SEQRAM_C6
	MT9P017MIPI_write_cmos_sensor(0x3EC8, 0x0000);	// DYNAMIC_SEQRAM_C8
	MT9P017MIPI_write_cmos_sensor(0x3ECA, 0x0000);	// DYNAMIC_SEQRAM_CA

	#endif
	/*LSC_REV4_DEMO_board
	MT9P017MIPI_write_cmos_sensor(0x3600, 0x00D0);	// P_GR_P0Q0
	MT9P017MIPI_write_cmos_sensor(0x3602, 0x31EC);	// P_GR_P0Q1
	MT9P017MIPI_write_cmos_sensor(0x3604, 0x1311);	// P_GR_P0Q2
	MT9P017MIPI_write_cmos_sensor(0x3606, 0xDB28);	// P_GR_P0Q3
	MT9P017MIPI_write_cmos_sensor(0x3608, 0xDC6F);	// P_GR_P0Q4
	MT9P017MIPI_write_cmos_sensor(0x360A, 0x0270);	// P_RD_P0Q0
	MT9P017MIPI_write_cmos_sensor(0x360C, 0xAE8D);	// P_RD_P0Q1
	MT9P017MIPI_write_cmos_sensor(0x360E, 0x6CD0);	// P_RD_P0Q2
	MT9P017MIPI_write_cmos_sensor(0x3610, 0xC3E8);	// P_RD_P0Q3
	MT9P017MIPI_write_cmos_sensor(0x3612, 0xFCCF);	// P_RD_P0Q4
	MT9P017MIPI_write_cmos_sensor(0x3614, 0x0210);	// P_BL_P0Q0
	MT9P017MIPI_write_cmos_sensor(0x3616, 0x082D);	// P_BL_P0Q1
	MT9P017MIPI_write_cmos_sensor(0x3618, 0x6B90);	// P_BL_P0Q2
	MT9P017MIPI_write_cmos_sensor(0x361A, 0x032A);	// P_BL_P0Q3
	MT9P017MIPI_write_cmos_sensor(0x361C, 0xA12E);	// P_BL_P0Q4
	MT9P017MIPI_write_cmos_sensor(0x361E, 0x0450);	// P_GB_P0Q0
	MT9P017MIPI_write_cmos_sensor(0x3620, 0xF3ED);	// P_GB_P0Q1
	MT9P017MIPI_write_cmos_sensor(0x3622, 0x2231);	// P_GB_P0Q2
	MT9P017MIPI_write_cmos_sensor(0x3624, 0xA5ED);	// P_GB_P0Q3
	MT9P017MIPI_write_cmos_sensor(0x3626, 0x8771);	// P_GB_P0Q4
	MT9P017MIPI_write_cmos_sensor(0x3640, 0x916C);	// P_GR_P1Q0
	MT9P017MIPI_write_cmos_sensor(0x3642, 0x864E);	// P_GR_P1Q1
	MT9P017MIPI_write_cmos_sensor(0x3644, 0xD88F);	// P_GR_P1Q2
	MT9P017MIPI_write_cmos_sensor(0x3646, 0xB406);	// P_GR_P1Q3
	MT9P017MIPI_write_cmos_sensor(0x3648, 0x2890);	// P_GR_P1Q4
	MT9P017MIPI_write_cmos_sensor(0x364A, 0xC6EC);	// P_RD_P1Q0
	MT9P017MIPI_write_cmos_sensor(0x364C, 0x56ED);	// P_RD_P1Q1
	MT9P017MIPI_write_cmos_sensor(0x364E, 0xA1AD);	// P_RD_P1Q2
	MT9P017MIPI_write_cmos_sensor(0x3650, 0xE06E);	// P_RD_P1Q3
	MT9P017MIPI_write_cmos_sensor(0x3652, 0x59AE);	// P_RD_P1Q4
	MT9P017MIPI_write_cmos_sensor(0x3654, 0x944B);	// P_BL_P1Q0
	MT9P017MIPI_write_cmos_sensor(0x3656, 0x690D);	// P_BL_P1Q1
	MT9P017MIPI_write_cmos_sensor(0x3658, 0x736E);	// P_BL_P1Q2
	MT9P017MIPI_write_cmos_sensor(0x365A, 0x440E);	// P_BL_P1Q3
	MT9P017MIPI_write_cmos_sensor(0x365C, 0xA26C);	// P_BL_P1Q4
	MT9P017MIPI_write_cmos_sensor(0x365E, 0x400B);	// P_GB_P1Q0
	MT9P017MIPI_write_cmos_sensor(0x3660, 0xD98E);	// P_GB_P1Q1
	MT9P017MIPI_write_cmos_sensor(0x3662, 0x12CF);	// P_GB_P1Q2
	MT9P017MIPI_write_cmos_sensor(0x3664, 0x07F0);	// P_GB_P1Q3
	MT9P017MIPI_write_cmos_sensor(0x3666, 0xD20F);	// P_GB_P1Q4
	MT9P017MIPI_write_cmos_sensor(0x3680, 0x28D1);	// P_GR_P2Q0
	MT9P017MIPI_write_cmos_sensor(0x3682, 0x33CF);	// P_GR_P2Q1
	MT9P017MIPI_write_cmos_sensor(0x3684, 0xB5B1);	// P_GR_P2Q2
	MT9P017MIPI_write_cmos_sensor(0x3686, 0x86F1);	// P_GR_P2Q3
	MT9P017MIPI_write_cmos_sensor(0x3688, 0x784F);	// P_GR_P2Q4
	MT9P017MIPI_write_cmos_sensor(0x368A, 0x1E31);	// P_RD_P2Q0
	MT9P017MIPI_write_cmos_sensor(0x368C, 0xC66F);	// P_RD_P2Q1
	MT9P017MIPI_write_cmos_sensor(0x368E, 0xAF52);	// P_RD_P2Q2
	MT9P017MIPI_write_cmos_sensor(0x3690, 0x49B0);	// P_RD_P2Q3
	MT9P017MIPI_write_cmos_sensor(0x3692, 0x2993);	// P_RD_P2Q4
	MT9P017MIPI_write_cmos_sensor(0x3694, 0x0511);	// P_BL_P2Q0
	MT9P017MIPI_write_cmos_sensor(0x3696, 0x9CAE);	// P_BL_P2Q1
	MT9P017MIPI_write_cmos_sensor(0x3698, 0xD371);	// P_BL_P2Q2
	MT9P017MIPI_write_cmos_sensor(0x369A, 0x7150);	// P_BL_P2Q3
	MT9P017MIPI_write_cmos_sensor(0x369C, 0x60F2);	// P_BL_P2Q4
	MT9P017MIPI_write_cmos_sensor(0x369E, 0x2FB1);	// P_GB_P2Q0
	MT9P017MIPI_write_cmos_sensor(0x36A0, 0xEC4F);	// P_GB_P2Q1
	MT9P017MIPI_write_cmos_sensor(0x36A2, 0xBDD2);	// P_GB_P2Q2
	MT9P017MIPI_write_cmos_sensor(0x36A4, 0x03B1);	// P_GB_P2Q3
	MT9P017MIPI_write_cmos_sensor(0x36A6, 0x1A53);	// P_GB_P2Q4
	MT9P017MIPI_write_cmos_sensor(0x36C0, 0x97EC);	// P_GR_P3Q0
	MT9P017MIPI_write_cmos_sensor(0x36C2, 0xB72F);	// P_GR_P3Q1
	MT9P017MIPI_write_cmos_sensor(0x36C4, 0x628F);	// P_GR_P3Q2
	MT9P017MIPI_write_cmos_sensor(0x36C6, 0x34D0);	// P_GR_P3Q3
	MT9P017MIPI_write_cmos_sensor(0x36C8, 0x9E30);	// P_GR_P3Q4
	MT9P017MIPI_write_cmos_sensor(0x36CA, 0x036E);	// P_RD_P3Q0
	MT9P017MIPI_write_cmos_sensor(0x36CC, 0xCBAA);	// P_RD_P3Q1
	MT9P017MIPI_write_cmos_sensor(0x36CE, 0x73CF);	// P_RD_P3Q2
	MT9P017MIPI_write_cmos_sensor(0x36D0, 0x00ED);	// P_RD_P3Q3
	MT9P017MIPI_write_cmos_sensor(0x36D2, 0xDC31);	// P_RD_P3Q4
	MT9P017MIPI_write_cmos_sensor(0x36D4, 0xF6AB);	// P_BL_P3Q0
	MT9P017MIPI_write_cmos_sensor(0x36D6, 0x130E);	// P_BL_P3Q1
	MT9P017MIPI_write_cmos_sensor(0x36D8, 0x2790);	// P_BL_P3Q2
	MT9P017MIPI_write_cmos_sensor(0x36DA, 0x8B70);	// P_BL_P3Q3
	MT9P017MIPI_write_cmos_sensor(0x36DC, 0xEE91);	// P_BL_P3Q4
	MT9P017MIPI_write_cmos_sensor(0x36DE, 0xA78E);	// P_GB_P3Q0
	MT9P017MIPI_write_cmos_sensor(0x36E0, 0x67CE);	// P_GB_P3Q1
	MT9P017MIPI_write_cmos_sensor(0x36E2, 0xBB30);	// P_GB_P3Q2
	MT9P017MIPI_write_cmos_sensor(0x36E4, 0xC88F);	// P_GB_P3Q3
	MT9P017MIPI_write_cmos_sensor(0x36E6, 0x5AF1);	// P_GB_P3Q4
	MT9P017MIPI_write_cmos_sensor(0x3700, 0xF82F);	// P_GR_P4Q0
	MT9P017MIPI_write_cmos_sensor(0x3702, 0xDAF0);	// P_GR_P4Q1
	MT9P017MIPI_write_cmos_sensor(0x3704, 0xBAB4);	// P_GR_P4Q2
	MT9P017MIPI_write_cmos_sensor(0x3706, 0x1872);	// P_GR_P4Q3
	MT9P017MIPI_write_cmos_sensor(0x3708, 0x4A56);	// P_GR_P4Q4
	MT9P017MIPI_write_cmos_sensor(0x370A, 0xE110);	// P_RD_P4Q0
	MT9P017MIPI_write_cmos_sensor(0x370C, 0x2D31);	// P_RD_P4Q1
	MT9P017MIPI_write_cmos_sensor(0x370E, 0x9DF3);	// P_RD_P4Q2
	MT9P017MIPI_write_cmos_sensor(0x3710, 0xF492);	// P_RD_P4Q3
	MT9P017MIPI_write_cmos_sensor(0x3712, 0x60B5);	// P_RD_P4Q4
	MT9P017MIPI_write_cmos_sensor(0x3714, 0xC7CD);	// P_BL_P4Q0
	MT9P017MIPI_write_cmos_sensor(0x3716, 0x54D1);	// P_BL_P4Q1
	MT9P017MIPI_write_cmos_sensor(0x3718, 0xEC33);	// P_BL_P4Q2
	MT9P017MIPI_write_cmos_sensor(0x371A, 0xE0B3);	// P_BL_P4Q3
	MT9P017MIPI_write_cmos_sensor(0x371C, 0x68F5);	// P_BL_P4Q4
	MT9P017MIPI_write_cmos_sensor(0x371E, 0xC7B0);	// P_GB_P4Q0
	MT9P017MIPI_write_cmos_sensor(0x3720, 0x5E91);	// P_GB_P4Q1
	MT9P017MIPI_write_cmos_sensor(0x3722, 0x8DD4);	// P_GB_P4Q2
	MT9P017MIPI_write_cmos_sensor(0x3724, 0xDF92);	// P_GB_P4Q3
	MT9P017MIPI_write_cmos_sensor(0x3726, 0x26D6);	// P_GB_P4Q4
	MT9P017MIPI_write_cmos_sensor(0x3782, 0x05D0);	// POLY_ORIGIN_C
	MT9P017MIPI_write_cmos_sensor(0x3784, 0x03DC);	// POLY_ORIGIN_R
	MT9P017MIPI_write_cmos_sensor(0x37C0, 0xEC4A);	// P_GR_Q5
	MT9P017MIPI_write_cmos_sensor(0x37C2, 0xB60A);	// P_RD_Q5
	MT9P017MIPI_write_cmos_sensor(0x37C4, 0xD6EA);	// P_BL_Q5
	MT9P017MIPI_write_cmos_sensor(0x37C6, 0xC9CB);	// P_GB_Q5
	MT9P017MIPI_write_cmos_sensor(0x3780, 0x0000);	// POLY_SC_ENABLE
	*/
	
	MT9P017MIPI_write_cmos_sensor(0x3ECE, 0x000A);	// DAC_LD_2_3
	MT9P017MIPI_write_cmos_sensor(0x0400, 0x0000);	// SCALING_MODE disable scale
	MT9P017MIPI_write_cmos_sensor(0x0404, 0x0010);	// SCALE_M
#ifdef RAW10
	MT9P017MIPI_write_cmos_sensor(0x0300, 0x05);	//vt_pix_clk_div = 5
	MT9P017MIPI_write_cmos_sensor(0x0302, 0x01);	//vt_sys_clk_div = 1
	MT9P017MIPI_write_cmos_sensor(0x0304, 0x02);	//pre_pll_clk_div = 2
	MT9P017MIPI_write_cmos_sensor(0x0306, 0x28);	//pll_multiplier    =  40
	MT9P017MIPI_write_cmos_sensor(0x0308, 0x0A);	//op_pix_clk_div =  10
	MT9P017MIPI_write_cmos_sensor(0x030A, 0x01);	//op_sys_clk_div = 1

#else
	//PLL MCLK=24MHZ, PCLK=72.8MHZ, VT=112MHZ
	#ifdef MIPI_INTERFACE
		MT9P017MIPI_write_cmos_sensor(0x0300, 0x04);	//vt_pix_clk_div = 8
	#else
		MT9P017MIPI_write_cmos_sensor(0x0300, 0x08);	//vt_pix_clk_div = 8
	#endif
	MT9P017MIPI_write_cmos_sensor(0x0302, 0x01);	//vt_sys_clk_div = 1
	MT9P017MIPI_write_cmos_sensor(0x0304, 0x02);	//pre_pll_clk_div = 2
	MT9P017MIPI_write_cmos_sensor(0x0306, 0x20);	//pll_multiplier    =  32
	MT9P017MIPI_write_cmos_sensor(0x0308, 0x08);	//op_pix_clk_div =  8
	MT9P017MIPI_write_cmos_sensor(0x030A, 0x01);	//op_sys_clk_div = 1
#endif

	MT9P017MIPI_write_cmos_sensor(0x306E, 0xbc00);	// slew rate for color issue
	MT9P017MIPI_write_cmos_sensor(0x30F0, 0x8000);  //Enable AF function
	//MT9P017MIPI_write_cmos_sensor(0x3040, 0x04C3); 
	//MT9P017MIPI_write_cmos_sensor(0x3010, 0x0184); 	// FINE_CORRECTION
	//MT9P017MIPI_write_cmos_sensor(0x3012, 0x029C); 	// COARSE_INTEGRATION_TIME_
	//MT9P017MIPI_write_cmos_sensor(0x3014, 0x0908); 	// FINE_INTEGRATION_TIME_
	
	//MT9P017MIPI_write_cmos_sensor_8(0x0100, 0x01); 	// MODE_SELECT
	
    spin_lock(&MT9P017MIPI_drv_lock);
	MT9P017MIPI_Auto_Flicker_mode = KAL_FALSE;
	spin_unlock(&MT9P017MIPI_drv_lock);

	
	mDELAY(5);				// Allow PLL to lock

}   /*  MT9P017MIPI_Sensor_Init  */

kal_uint16 MT9P017MIPI_PowerOn(void)
{
	const kal_uint8 i2c_addr[] = {MT9P017MIPI_WRITE_ID_1, MT9P017MIPI_WRITE_ID_2, 0x6C, 0x6E}; 
	kal_uint16 sensor_id = 0xffff;
	kal_uint16 i;
	
	for(i = 0; i < sizeof(i2c_addr) / sizeof(i2c_addr[0]); i++)
	{
		MT9P017MIPI_sensor.i2c_write_id = i2c_addr[i];
		SENSORDB( "i2c address is %x \n", MT9P017MIPI_sensor.i2c_write_id);
		
		sensor_id = MT9P017MIPI_read_cmos_sensor(0x0000);
		if(sensor_id == MT9P017MIPI_SENSOR_ID)
			break;
			
	}
	return sensor_id;
}
/*************************************************************************
* FUNCTION
*   MT9P017MIPIOpen
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

UINT32 MT9P017MIPIOpen(void)
{
	kal_uint16 sensor_id = 0;
	kal_uint16 temp_data;
	sensor_id = MT9P017MIPI_PowerOn() ;
	
	SENSORDB("MT9P017MIPIOpen sensor_id is %x \n", sensor_id);
    if (sensor_id != MT9P017MIPI_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;

    MT9P017MIPI_Init_setting();
	temp_data= read_MT9P017MIPI_gain();
    spin_lock(&MT9P017MIPI_drv_lock);
    MT9P017MIPI_sensor_gain_base = temp_data;
    spin_unlock(&MT9P017MIPI_drv_lock);
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   MT9P017MIPIGetSensorID
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
UINT32 MT9P017MIPIGetSensorID(UINT32 *sensorID) 
{
		*sensorID  = MT9P017MIPI_PowerOn() ;
		
		SENSORDB("MT9P017MIPIOpen sensor_id is %x\n", *sensorID );
		
 
    if (*sensorID != MT9P017MIPI_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   MT9P017MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of MT9P017MIPI to change exposure time.
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
void MT9P017MIPI_SetShutter(kal_uint16 iShutter)
{
    //SENSORDB("MT9P017MIPI_SetShutter =%d \n ",iShutter);
	unsigned long flags;
	
    if(MT9P017MIPI_exposure_lines == iShutter) {
        return;
    }
	
    spin_lock_irqsave(&MT9P017MIPI_drv_lock,flags);
	MT9P017MIPI_exposure_lines=iShutter;
    spin_unlock_irqrestore(&MT9P017MIPI_drv_lock,flags);
	
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x01); 	// GROUPED_PARAMETER_HOLD
	//if(MT9P017MIPI_Auto_Flicker_mode == KAL_TRUE)
	//    MT9P017MIPI_write_cmos_sensor(0x0340, MT9P017_Frame_Length_preview +AUTO_FLICKER_NO);
	MT9P017MIPI_write_cmos_sensor(0x0202, iShutter); /* course_integration_time */
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x00); 	// GROUPED_PARAMETER_HOLD

}   /*  MT9P017MIPI_SetShutter   */



/*************************************************************************
* FUNCTION
*   MT9P017MIPI_read_shutter
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
UINT16 MT9P017MIPI_read_shutter(void)
{
    kal_uint16 ishutter;
	ishutter = MT9P017MIPI_read_cmos_sensor(0x0202); /* course_integration_time */
	return ishutter;
}

/*************************************************************************
* FUNCTION
*   MT9P017MIPI_night_mode
*
* DESCRIPTION
*   This function night mode of MT9P017MIPI.
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
void MT9P017MIPI_NightMode(kal_bool bEnable)
{
    // frame rate will be control by AE table 
     
}/*	MT9P017MIPI_NightMode */



/*************************************************************************
* FUNCTION
*   MT9P017MIPIClose
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
UINT32 MT9P017MIPIClose(void)
{
    kal_uint16 pos = 0;
	pos = MT9P017MIPI_read_cmos_sensor(0x30F2);
    printk("MT9P017MIPIClose 3333 zhijie\n");

	if(pos > 0x60)
	{
		MT9P017MIPI_write_cmos_sensor(0x30F2, 0x0060);  //AF step hold
		mDELAY(20);
	}
	MT9P017MIPI_write_cmos_sensor(0x30F2, 0x0040);  //AF step hold
	
	mDELAY(20);
	MT9P017MIPI_write_cmos_sensor(0x30F2, 0x0010);  //AF step hold
	
	mDELAY(30);
	MT9P017MIPI_write_cmos_sensor(0x30F2, 0x0004);  //AF step hold
	
	mDELAY(20);
    return ERROR_NONE;
}	/* MT9P017MIPIClose() */

void MT9P017MIPI_Set_Mirror_Flip(kal_uint8 image_mirror)
{
	switch (image_mirror)
	{
		case IMAGE_NORMAL:         //Set first grab  B
			MT9P017MIPI_write_cmos_sensor_8(0x0101,0x00);
		break;
		case IMAGE_H_MIRROR:      //Set first grab  Gb
			MT9P017MIPI_write_cmos_sensor_8(0x0101,0x01);
		break;
		case IMAGE_V_MIRROR:     //Set first grab  Gr
			MT9P017MIPI_write_cmos_sensor_8(0x0101,0x02);
		break;
		case IMAGE_HV_MIRROR:     //Set first grab  R
			MT9P017MIPI_write_cmos_sensor_8(0x0101,0x03);
		break;
	}
}


static void MT9P017MIPI_preview_setting(void)
{
	//MT9P017MIPI_write_cmos_sensor_8(0x0100, 0x0); 	// MODE_SELECT
	kal_uint16 temp;
	//1296 x 972  Timing settings 30fps

	#ifdef MIPI_INTERFACE
		#ifdef RAW10
		//MT9P017MIPI_write_cmos_sensor(0x301A, 0x0018);	// enable mipi interface
		MT9P017MIPI_write_cmos_sensor(0x3064, 0xB800);	// SMIA_TEST
		MT9P017MIPI_write_cmos_sensor(0x31AE, 0x0202);	// two lane 201 tow 202
		MT9P017MIPI_write_cmos_sensor(0x0112, 0x0A0A);	// 10bit raw output
		#else
		//MT9P017MIPI_write_cmos_sensor(0x301A, 0x0018);	// enable mipi interface
		MT9P017MIPI_write_cmos_sensor(0x3064, 0x0805);	// SMIA_TEST
		MT9P017MIPI_write_cmos_sensor(0x31AE, 0x0202);	// two lane
		MT9P017MIPI_write_cmos_sensor(0x0112, 0x0808);	// 8bit raw output
		#endif
	#endif
	//PLL MCLK=26MHZ, PCLK=52MHZ, VT=104MHZ
	#ifdef RAW10
		MT9P017MIPI_write_cmos_sensor(0x0300, 0x05);	//vt_pix_clk_div = 5
		MT9P017MIPI_write_cmos_sensor(0x0302, 0x01);	//vt_sys_clk_div = 1
		MT9P017MIPI_write_cmos_sensor(0x0304, 0x02);	//pre_pll_clk_div = 2
		MT9P017MIPI_write_cmos_sensor(0x0306, 0x28);	//pll_multiplier    =  40
		MT9P017MIPI_write_cmos_sensor(0x0308, 0x0A);	//op_pix_clk_div =  10
		MT9P017MIPI_write_cmos_sensor(0x030A, 0x01);	//op_sys_clk_div = 1

	#else	
		#ifdef MIPI_INTERFACE
			MT9P017MIPI_write_cmos_sensor(0x0300, 0x04);	//vt_pix_clk_div = 4
		#else
			MT9P017MIPI_write_cmos_sensor(0x0300, 0x08);	//vt_pix_clk_div = 8
		#endif
		MT9P017MIPI_write_cmos_sensor(0x0302, 0x01);	//vt_sys_clk_div = 1
		MT9P017MIPI_write_cmos_sensor(0x0304, 0x02);	//pre_pll_clk_div = 2
		MT9P017MIPI_write_cmos_sensor(0x0306, 0x20);	//pll_multiplier    =  32
		MT9P017MIPI_write_cmos_sensor(0x0308, 0x08);	//op_pix_clk_div =  8
		MT9P017MIPI_write_cmos_sensor(0x030A, 0x01);	//op_sys_clk_div = 1
	#endif
	
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x01); 	// GROUPED_PARAMETER_HOLD
	
	MT9P017MIPI_write_cmos_sensor(0x0344, 0x0008); 	// X_ADDR_START   =  8
	MT9P017MIPI_write_cmos_sensor(0x0346, 0x0008); 	// Y_ADDR_START   =  8
	MT9P017MIPI_write_cmos_sensor(0x0348, 0x0A25); 	// X_ADDR_END      = 2597
	MT9P017MIPI_write_cmos_sensor(0x034A, 0x079D); 	// Y_ADDR_END       =  1949
	
	temp = MT9P017MIPI_read_cmos_sensor(0x3040);
	temp = temp & 0xF000;
	temp = temp | 0x04C3 ;
	MT9P017MIPI_write_cmos_sensor(0x3040, temp);	// READ_MODE  10 011 000011 xy binning enable xodd=3, yodd=3
	MT9P017MIPI_write_cmos_sensor(0x034C, 0x0510); 	// X_OUTPUT_SIZE    = 1296
	MT9P017MIPI_write_cmos_sensor(0x034E, 0x03CC); 	// Y_OUTPUT_SIZE    =  972

	
	MT9P017MIPI_write_cmos_sensor(0x300C, 0x0C4C); 	// FRAME_LENGTH_LINES
	MT9P017MIPI_write_cmos_sensor(0x300A, 0x0415); 	// LINE_LENGTH_PCK
	MT9P017MIPI_write_cmos_sensor(0x3012, 0x0414); 	
	MT9P017MIPI_write_cmos_sensor(0x3014, 0x0908); 	
	MT9P017MIPI_write_cmos_sensor(0x3010, 0x0184); 	
	
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x00); 	// GROUPED_PARAMETER_HOLD

	//start_streaming
	MT9P017MIPI_write_cmos_sensor_8(0x0100, 0x01); 	// MODE_SELECT
    spin_lock(&MT9P017MIPI_drv_lock);
#ifdef MIPI_INTERFACE
	MT9P017MIPI_sensor.preview_vt_clk = 1040  ; //48M VT 
#else
	MT9P017MIPI_sensor.preview_vt_clk = 520  ; //48M VT 
#endif
    spin_unlock(&MT9P017MIPI_drv_lock);

	mDELAY(50);

	//start_streaming
		
}

static void MT9P017MIPI_capture_setting(void)
{
	//[2592 X 1944]
	// Timing Settings 13.09fps  
	kal_uint16 temp;
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x01); //Grouped Parameter Hold = 0x1

	#ifdef MIPI_INTERFACE
		#ifdef RAW10
		//MT9P017MIPI_write_cmos_sensor(0x301A, 0x0018);	// enable mipi interface
		MT9P017MIPI_write_cmos_sensor(0x3064, 0xB800);	// SMIA_TEST
		MT9P017MIPI_write_cmos_sensor(0x31AE, 0x0202);	// two lane 201 tow 202
		MT9P017MIPI_write_cmos_sensor(0x0112, 0x0A0A);	// 10bit raw output
		#else
		//MT9P017MIPI_write_cmos_sensor(0x301A, 0x0018);	// enable mipi interface
		MT9P017MIPI_write_cmos_sensor(0x3064, 0x0805);	// SMIA_TEST
		MT9P017MIPI_write_cmos_sensor(0x31AE, 0x0202);	// two lane
		MT9P017MIPI_write_cmos_sensor(0x0112, 0x0808);	// 8bit raw output
		#endif
	#endif

	#ifdef RAW10
		MT9P017MIPI_write_cmos_sensor(0x0300, 0x05);	//vt_pix_clk_div = 5
		MT9P017MIPI_write_cmos_sensor(0x0302, 0x01);	//vt_sys_clk_div = 1
		MT9P017MIPI_write_cmos_sensor(0x0304, 0x02);	//pre_pll_clk_div = 2
		MT9P017MIPI_write_cmos_sensor(0x0306, 0x28);	//pll_multiplier    =  40
		MT9P017MIPI_write_cmos_sensor(0x0308, 0x0A);	//op_pix_clk_div =  10
		MT9P017MIPI_write_cmos_sensor(0x030A, 0x01);	//op_sys_clk_div = 1

	#else		
	//PLL MCLK=26MHZ, PCLK=52MHZ, VT=104MHZ
		#ifdef MIPI_INTERFACE
			MT9P017MIPI_write_cmos_sensor(0x0300, 0x04);	//vt_pix_clk_div = 8
		#else
			MT9P017MIPI_write_cmos_sensor(0x0300, 0x08);	//vt_pix_clk_div = 8
		#endif
		MT9P017MIPI_write_cmos_sensor(0x0302, 0x01);	//vt_sys_clk_div = 1
		MT9P017MIPI_write_cmos_sensor(0x0304, 0x02);	//pre_pll_clk_div = 2
		MT9P017MIPI_write_cmos_sensor(0x0306, 0x20);	//pll_multiplier    =  32
		MT9P017MIPI_write_cmos_sensor(0x0308, 0x08);	//op_pix_clk_div =  8
		MT9P017MIPI_write_cmos_sensor(0x030A, 0x01);	//op_sys_clk_div = 1
	#endif
	
	MT9P017MIPI_write_cmos_sensor(0x0344, 0x0008);	//X_ADDR_START   = 8
	MT9P017MIPI_write_cmos_sensor(0x0346, 0x0008);	//Y_ADDR_START    = 8
	MT9P017MIPI_write_cmos_sensor(0x0348, 0x0A27);	//X_ADDR_END =  2599
	MT9P017MIPI_write_cmos_sensor(0x034A, 0x079F);	//Y_ADDR_END = 1951
	
	temp = MT9P017MIPI_read_cmos_sensor(0x3040);
	temp = temp & 0xF000 ;
	temp = temp | 0x0041;
	MT9P017MIPI_write_cmos_sensor(0x3040, temp);	//Read Mode = 0x41   1 000001 binning disable
	MT9P017MIPI_write_cmos_sensor(0x034C, 0x0A20);	//X_OUTPUT_SIZE= 2592
	MT9P017MIPI_write_cmos_sensor(0x034E, 0x0798);	//Y_OUTPUT_SIZE = 1944

	MT9P017MIPI_write_cmos_sensor(0x300C, 0x0AE8);	// Line_LENGTH_LINES
	MT9P017MIPI_write_cmos_sensor(0x300A, 0x082E);	// Frame_LENGTH_PCK

	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x00); //Grouped Parameter Hold = 0x0
	
	MT9P017MIPI_write_cmos_sensor_8(0x0100, 0x01);		//mode_select (Open streaming)
	
    spin_lock(&MT9P017MIPI_drv_lock);
#ifdef MIPI_INTERFACE
	MT9P017MIPI_sensor.capture_vt_clk = 1040  ; // VT 
#else
	MT9P017MIPI_sensor.capture_vt_clk = 520  ; // VT 
#endif
    spin_unlock(&MT9P017MIPI_drv_lock);

	mDELAY(10);
}

/*
static void MT9P017MIPI_capture_setting_15fps(void)
{
	//[2592 X 1944]
	// Timing Settings 13.728fps  
	kal_uint16 temp;
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x01); //Grouped Parameter Hold = 0x1
	
	#ifdef MIPI_INTERFACE
		MT9P017MIPI_write_cmos_sensor(0x301A, 0x0018);	// enable mipi interface
		MT9P017MIPI_write_cmos_sensor(0x3064, 0x0805);	// SMIA_TEST
		MT9P017MIPI_write_cmos_sensor(0x31AE, 0x0202);	// two lane
		MT9P017MIPI_write_cmos_sensor(0x0112, 0x0808);	// 10bit raw output
	#endif
	//PLL MCLK=26MHZ, PCLK=78MHZ, VT=78MHZ
	MT9P017MIPI_write_cmos_sensor(0x0300, 0x04);	//vt_pix_clk_div = 8
	MT9P017MIPI_write_cmos_sensor(0x0302, 0x01);	//vt_sys_clk_div = 1
	MT9P017MIPI_write_cmos_sensor(0x0304, 0x02);	//pre_pll_clk_div = 2
	//MT9P017MIPI_write_cmos_sensor(0x0306, 0x30);	//pll_multiplier    =  40  78M 
	MT9P017MIPI_write_cmos_sensor(0x0306, 0x38);	//pll_multiplier    =  56  91M
	MT9P017MIPI_write_cmos_sensor(0x0308, 0x08);	//op_pix_clk_div =  8
	MT9P017MIPI_write_cmos_sensor(0x030A, 0x01);	//op_sys_clk_div = 1

	MT9P017MIPI_write_cmos_sensor(0x0382, 0x01); 	// x_odd_inc
	MT9P017MIPI_write_cmos_sensor(0x0386, 0x01); 	// y_odd_inc
	//MT9P017MIPI_write_cmos_sensor(0x0400, 0x00); 	// scaleing
	
	MT9P017MIPI_write_cmos_sensor(0x0344, 0x0008);	//X_ADDR_START   = 8
	MT9P017MIPI_write_cmos_sensor(0x0346, 0x0008);	//Y_ADDR_START    = 8
	MT9P017MIPI_write_cmos_sensor(0x0348, 0x0A27);	//X_ADDR_END =  2599
	MT9P017MIPI_write_cmos_sensor(0x034A, 0x079F);	//Y_ADDR_END = 1951

	temp = MT9P017MIPI_read_cmos_sensor(0x3040);
	temp = temp & 0xF000 ;
	temp = temp | 0x0041;
	MT9P017MIPI_write_cmos_sensor(0x3040, temp);	//Read Mode = 0x41   1 000001 binning disable
	MT9P017MIPI_write_cmos_sensor(0x034C, 0x0A20);	//X_OUTPUT_SIZE= 2592
	MT9P017MIPI_write_cmos_sensor(0x034E, 0x0798);	//Y_OUTPUT_SIZE = 1944

	
	MT9P017MIPI_write_cmos_sensor(0x300C, 0x0AE8);	// FRAME_LENGTH_LINES
	MT9P017MIPI_write_cmos_sensor(0x300A, 0x082E);	// LINE_LENGTH_PCK

	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x00); //Grouped Parameter Hold = 0x0
	
	MT9P017MIPI_write_cmos_sensor_8(0x0100, 0x01);		//mode_select (Open streaming)
	//MT9P017MIPI_sensor.capture_vt_clk = 780  ; //4// VT 
	MT9P017MIPI_sensor.capture_vt_clk = 910  ; // PCLK = 91
	
	mDELAY(10);
}
*/
/*************************************************************************
* FUNCTION
*   MT9P017MIPI_SetDummy
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

static void MT9P017MIPI_SetDummy(kal_bool mode,const kal_uint16 iDummyPixels, const kal_uint16 iDummyLines)
{
    kal_uint16 Line_length_pclk, Frame_length_lines;
    
    SENSORDB("Enter MT9P017MIPI_SetDummy \n");
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x01); 	// GROUPED_PARAMETER_HOLD
	if(mode == KAL_TRUE) //preview
	{
		Line_length_pclk   = MT9P017MIPI_PV_PERIOD_PIXEL_NUMS + iDummyPixels;
		Frame_length_lines = MT9P017MIPI_PV_PERIOD_LINE_NUMS  + iDummyLines;
		
		spin_lock(&MT9P017MIPI_drv_lock);
		MT9P017_Frame_Length_preview = Frame_length_lines;
		spin_unlock(&MT9P017MIPI_drv_lock);
	}
	else   //capture
	{
		Line_length_pclk   = MT9P017MIPI_FULL_PERIOD_PIXEL_NUMS + iDummyPixels;
		Frame_length_lines = MT9P017MIPI_FULL_PERIOD_LINE_NUMS  + iDummyLines;
	}
	
    SENSORDB("Enter MT9P017MIPI_SetDummy Frame_length_lines=%d, Line_length_pclk=%d\n",Frame_length_lines,Line_length_pclk);
	MT9P017MIPI_write_cmos_sensor(0x0340, Frame_length_lines);
	MT9P017MIPI_write_cmos_sensor(0x0342, Line_length_pclk);
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x00); //Grouped Parameter Hold = 0x0
}   /*  MT9P017MIPI_SetDummy */


/*************************************************************************
* FUNCTION
*   MT9P017MIPIPreview
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
UINT32 MT9P017MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("Enter MT9P017MIPIPreview 222222 \n ");
    spin_lock(&MT9P017MIPI_drv_lock);
    MT9P017MIPI_PV_dummy_pixels = 0;
	MT9P017MIPI_PV_dummy_lines  = 0;  
	
    spin_unlock(&MT9P017MIPI_drv_lock);
	sensor_config_data->SensorImageMirror = IMAGE_NORMAL;
	
    spin_lock(&MT9P017MIPI_drv_lock);
	 if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        MT9P017MIPI_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {
        MT9P017MIPI_MPEG4_encode_mode = KAL_FALSE;
    }
    spin_unlock(&MT9P017MIPI_drv_lock);
	MT9P017MIPI_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

	MT9P017MIPI_preview_setting();
	MT9P017MIPI_SetDummy(KAL_TRUE,MT9P017MIPI_PV_dummy_pixels,MT9P017MIPI_PV_dummy_lines);

    image_window->GrabStartX= MT9P017MIPI_PV_START_X;
    image_window->GrabStartY= MT9P017MIPI_PV_START_Y;
    image_window->ExposureWindowWidth= MT9P017MIPI_IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight= MT9P017MIPI_IMAGE_SENSOR_PV_HEIGHT;
    spin_lock(&MT9P017MIPI_drv_lock);

    g_iMT9P017MIPI_Mode = MT9P017MIPI_MODE_PREVIEW;
    spin_unlock(&MT9P017MIPI_drv_lock);
    //MT9P017MIPI_SetShutter(MT9P017MIPI_exposure_lines);
    MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x01); 	// GROUPED_PARAMETER_HOLD
    MT9P017MIPI_write_cmos_sensor(0x0202, MT9P017MIPI_exposure_lines); /* course_integration_time */
    MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x00); 	// GROUPED_PARAMETER_HOLD
	memcpy(&MT9P017MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}	/* MT9P017MIPIPreview() */



/*******************************************************************************
*
********************************************************************************/
UINT32 MT9P017MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint32 shutter=MT9P017MIPI_exposure_lines;
    spin_lock(&MT9P017MIPI_drv_lock);

	MT9P017MIPI_Auto_Flicker_mode = KAL_FALSE;
	MT9P017MIPI_MPEG4_encode_mode = KAL_FALSE;
    spin_unlock(&MT9P017MIPI_drv_lock);

	if ((image_window->ImageTargetWidth<= MT9P017MIPI_IMAGE_SENSOR_PV_WIDTH) &&
		  (image_window->ImageTargetHeight<= MT9P017MIPI_IMAGE_SENSOR_PV_HEIGHT))
	{	
		spin_lock(&MT9P017MIPI_drv_lock);
		MT9P017MIPI_dummy_pixels = 0;
		MT9P017MIPI_dummy_lines  = 0;
		SENSORDB("Enter MT9P017MIPICapture small size  \n ");

		MT9P017MIPI_sensor.capture_vt_clk = MT9P017MIPI_sensor.preview_vt_clk;
		spin_unlock(&MT9P017MIPI_drv_lock);
		MT9P017MIPI_SetDummy(KAL_TRUE,MT9P017MIPI_dummy_pixels,MT9P017MIPI_dummy_lines);
		
		shutter = shutter * (MT9P017MIPI_PV_PERIOD_PIXEL_NUMS + MT9P017MIPI_PV_dummy_pixels)/(MT9P017MIPI_PV_PERIOD_PIXEL_NUMS +MT9P017MIPI_dummy_pixels);
		shutter = shutter * MT9P017MIPI_sensor.capture_vt_clk / MT9P017MIPI_sensor.preview_vt_clk;
		
		image_window->GrabStartX           = MT9P017MIPI_PV_START_X;
        image_window->GrabStartY           = MT9P017MIPI_PV_START_Y;
        image_window->ExposureWindowWidth  = MT9P017MIPI_IMAGE_SENSOR_PV_WIDTH;
        image_window->ExposureWindowHeight = MT9P017MIPI_IMAGE_SENSOR_PV_WIDTH;
	}
	else  //5M
	{
	       if(g_iMT9P017MIPI_Mode == MT9P017MIPI_MODE_CAPTURE) {
	       	return;
	       }
		   spin_lock(&MT9P017MIPI_drv_lock);
	       g_iMT9P017MIPI_Mode = MT9P017MIPI_MODE_CAPTURE;
	      
		MT9P017MIPI_dummy_pixels = 0;
		MT9P017MIPI_dummy_lines  = 0;
		
		spin_unlock(&MT9P017MIPI_drv_lock);
		SENSORDB("Enter MT9P017MIPICapture full size  \n ");
		MT9P017MIPI_capture_setting();
		
		MT9P017MIPI_SetDummy(KAL_FALSE,MT9P017MIPI_dummy_pixels,MT9P017MIPI_dummy_lines);
		SENSORDB("preview shutter =%d \n",shutter);
		shutter = shutter * (MT9P017MIPI_PV_PERIOD_PIXEL_NUMS + MT9P017MIPI_PV_dummy_pixels)/(MT9P017MIPI_FULL_PERIOD_PIXEL_NUMS +MT9P017MIPI_dummy_pixels);
		shutter = shutter * MT9P017MIPI_sensor.capture_vt_clk / MT9P017MIPI_sensor.preview_vt_clk;
		SENSORDB("capture  shutter =%d ,gain = %d\n",shutter,MT9P017MIPI_read_cmos_sensor(0x204));

		image_window->GrabStartX           = MT9P017MIPI_FULL_START_X;
        image_window->GrabStartY           = MT9P017MIPI_FULL_START_Y;
        image_window->ExposureWindowWidth  = MT9P017MIPI_IMAGE_SENSOR_FULL_WIDTH;
        image_window->ExposureWindowHeight = MT9P017MIPI_IMAGE_SENSOR_FULL_HEIGHT;
	}
	if(shutter < 1)
	    shutter = 1;
    MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x01); 	// GROUPED_PARAMETER_HOLD
	MT9P017MIPI_write_cmos_sensor(0x0202, shutter); /* course_integration_time */
	MT9P017MIPI_write_cmos_sensor_8(0x0104, 0x00); 	// GROUPED_PARAMETER_HOLD 

	
	memcpy(&MT9P017MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}	/* MT9P017MIPICapture() */

UINT32 MT9P017MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth     =  MT9P017MIPI_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight    =  MT9P017MIPI_IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth  =  MT9P017MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight =  MT9P017MIPI_IMAGE_SENSOR_PV_HEIGHT;

    return ERROR_NONE;
}   /* MT9P017MIPIGetResolution() */

UINT32 MT9P017MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch(ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorPreviewResolutionX=MT9P017MIPI_IMAGE_SENSOR_FULL_WIDTH;
			pSensorInfo->SensorPreviewResolutionY=MT9P017MIPI_IMAGE_SENSOR_FULL_HEIGHT;
			pSensorInfo->SensorCameraPreviewFrameRate=15;
		break;

		default:
        	pSensorInfo->SensorPreviewResolutionX=MT9P017MIPI_IMAGE_SENSOR_PV_WIDTH;
       		pSensorInfo->SensorPreviewResolutionY=MT9P017MIPI_IMAGE_SENSOR_PV_HEIGHT;
			pSensorInfo->SensorCameraPreviewFrameRate=30;
		break;
	}
    //pSensorInfo->SensorPreviewResolutionX =  MT9P017MIPI_IMAGE_SENSOR_PV_WIDTH;
    //pSensorInfo->SensorPreviewResolutionY =  MT9P017MIPI_IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX    =  MT9P017MIPI_IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY    =  MT9P017MIPI_IMAGE_SENSOR_FULL_HEIGHT;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
    pSensorInfo->SensorOutputDataFormat     = SENSOR_OUTPUT_FORMAT_RAW_B;
    pSensorInfo->SensorClockPolarity        = SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity        = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity        = SENSOR_CLOCK_POLARITY_HIGH;
    pSensorInfo->SensorInterruptDelayLines  = 1;
    
	#ifdef MIPI_INTERFACE
   		pSensorInfo->SensroInterfaceType        = SENSOR_INTERFACE_TYPE_MIPI;
   	#else
   		pSensorInfo->SensroInterfaceType		= SENSOR_INTERFACE_TYPE_PARALLEL;
   	#endif
    pSensorInfo->SensorDriver3D = 0;   // the sensor driver is 2D

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
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = MT9P017MIPI_PV_START_X; 
            pSensorInfo->SensorGrabStartY = MT9P017MIPI_PV_START_Y;    
            
			#ifdef MIPI_INTERFACE
	            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
	            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
		        pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
		        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
	            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
	            pSensorInfo->SensorPacketECCOrder = 1;
	        #endif
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = MT9P017MIPI_FULL_START_X; 
            pSensorInfo->SensorGrabStartY = MT9P017MIPI_FULL_START_X;   
            
			#ifdef MIPI_INTERFACE
	            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
	            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0; 
	            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
	            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
	            pSensorInfo->SensorPacketECCOrder = 1;
	        #endif
            break;
        default:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = MT9P017MIPI_PV_START_X; 
            pSensorInfo->SensorGrabStartY = MT9P017MIPI_PV_START_X;             
            break;
    }

   // MT9P017MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &MT9P017MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* MT9P017MIPIGetInfo() */


UINT32 MT9P017MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	Mt9P017_CurrentScenarioId = ScenarioId;
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            MT9P017MIPIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            MT9P017MIPICapture(pImageWindow, pSensorConfigData);
            break;
        default:
            return ERROR_INVALID_SCENARIO_ID;
           
    }
    return TRUE;
} /* MT9P017MIPIControl() */


UINT32 MT9P017MIPISetVideoMode(UINT16 u2FrameRate)
{
	kal_uint16 MAX_Frame_length =0;
	//SENSORDB("MT9P017MIPISetVideoMode u2FrameRate =%d",u2FrameRate);
	
	spin_lock(&MT9P017MIPI_drv_lock);
	MT9P017MIPI_MPEG4_encode_mode = KAL_TRUE;
	spin_unlock(&MT9P017MIPI_drv_lock);

	if(u2FrameRate >30 || u2FrameRate <5)
	    SENSORDB("Error frame rate seting");

	MAX_Frame_length = MT9P017MIPI_sensor.preview_vt_clk*100000/(MT9P017MIPI_PV_PERIOD_PIXEL_NUMS+MT9P017MIPI_PV_dummy_pixels)/u2FrameRate;
	//if(MT9P017MIPI_PV_dummy_lines <(MAX_Frame_length - MT9P017MIPI_PV_PERIOD_LINE_NUMS))  //original dummy length < current needed dummy length 
	if(MAX_Frame_length < MT9P017MIPI_PV_PERIOD_LINE_NUMS )
		MAX_Frame_length = MT9P017MIPI_PV_PERIOD_LINE_NUMS;
	spin_lock(&MT9P017MIPI_drv_lock);
	    MT9P017MIPI_PV_dummy_lines = MAX_Frame_length - MT9P017MIPI_PV_PERIOD_LINE_NUMS ;  
	
	spin_unlock(&MT9P017MIPI_drv_lock);
	MT9P017MIPI_SetDummy(KAL_TRUE,MT9P017MIPI_PV_dummy_pixels,MT9P017MIPI_PV_dummy_lines);
	
    return KAL_TRUE;
}


UINT32 MT9P017MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{

    SENSORDB("[MT9P017MIPISetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
    if(bEnable) 
	{   // enable auto flicker   
		spin_lock(&MT9P017MIPI_drv_lock);
        MT9P017MIPI_Auto_Flicker_mode = KAL_TRUE; 
		spin_unlock(&MT9P017MIPI_drv_lock);
        //if(MT9P017MIPI_MPEG4_encode_mode == KAL_TRUE) 
		{   
			// in the video mode, reset the frame rate
			MT9P017MIPI_write_cmos_sensor_8(0x0104, 1);        
			MT9P017MIPI_write_cmos_sensor(0x0340, MT9P017_Frame_Length_preview +AUTO_FLICKER_NO);
            MT9P017MIPI_write_cmos_sensor_8(0x0104, 0);        	
        }
    } else 
    {
		spin_lock(&MT9P017MIPI_drv_lock);
        MT9P017MIPI_Auto_Flicker_mode = KAL_FALSE; 
		spin_unlock(&MT9P017MIPI_drv_lock);
        if(MT9P017MIPI_MPEG4_encode_mode == KAL_TRUE) 
		{
			// in the video mode, restore the frame rate
			MT9P017MIPI_write_cmos_sensor_8(0x0104, 1);        
			MT9P017MIPI_write_cmos_sensor(0x0340, MT9P017_Frame_Length_preview);
            MT9P017MIPI_write_cmos_sensor_8(0x0104, 0);         	
        }
        printk("Disable Auto flicker\n");    
    }
    return TRUE;
}


UINT32 MT9P017MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++=MT9P017MIPI_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=MT9P017MIPI_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
			switch(Mt9P017_CurrentScenarioId)
			{
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
        		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	            	*pFeatureReturnPara16++= MT9P017MIPI_FULL_PERIOD_PIXEL_NUMS + MT9P017MIPI_dummy_pixels;//MT9P017MIPI_PV_PERIOD_PIXEL_NUMS+MT9P017MIPI_dummy_pixels;
	            	*pFeatureReturnPara16=MT9P017MIPI_FULL_PERIOD_LINE_NUMS+MT9P017MIPI_dummy_lines;
	           		*pFeatureParaLen=4;
				     break;

				default:
					 *pFeatureReturnPara16++= MT9P017MIPI_PV_PERIOD_PIXEL_NUMS + MT9P017MIPI_PV_dummy_pixels;//MT9P017MIPI_PV_PERIOD_PIXEL_NUMS+MT9P017MIPI_dummy_pixels;
	            	*pFeatureReturnPara16=MT9P017MIPI_PV_PERIOD_LINE_NUMS+MT9P017MIPI_PV_dummy_lines;
	           		*pFeatureParaLen=4;
				     break;
			}
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			switch(Mt9P017_CurrentScenarioId)
			{
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
        		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	            	*pFeatureReturnPara32 = MT9P017MIPI_sensor.preview_vt_clk * 100000; //19500000;
	           		*pFeatureParaLen=4;
					break;
				default:
					*pFeatureReturnPara32 = MT9P017MIPI_sensor.preview_vt_clk * 100000; //19500000;
	           		*pFeatureParaLen=4;
					break;
			}
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            MT9P017MIPI_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            MT9P017MIPI_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            MT9P07_Set_gain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
           // MT9P017MIPI_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            MT9P017MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = MT9P017MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
				spin_lock(&MT9P017MIPI_drv_lock);
                MT9P017MIPISensorCCT[i].Addr=*pFeatureData32++;
                MT9P017MIPISensorCCT[i].Para=*pFeatureData32++;
				spin_unlock(&MT9P017MIPI_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=MT9P017MIPISensorCCT[i].Addr;
                *pFeatureData32++=MT9P017MIPISensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
				spin_lock(&MT9P017MIPI_drv_lock);
                MT9P017MIPISensorReg[i].Addr=*pFeatureData32++;
                MT9P017MIPISensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&MT9P017MIPI_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=MT9P017MIPISensorReg[i].Addr;
                *pFeatureData32++=MT9P017MIPISensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=MT9P017MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, MT9P017MIPISensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, MT9P017MIPISensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &MT9P017MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            MT9P017MIPI_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            MT9P017MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=MT9P017MIPI_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            MT9P017MIPI_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            MT9P017MIPI_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            MT9P017MIPI_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
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
            MT9P017MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            MT9P017MIPIGetSensorID(pFeatureReturnPara32); 
            break; 
   		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            MT9P017MIPISetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));            
	        break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* MT9P017MIPIFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncMT9P017MIPI=
{
    MT9P017MIPIOpen,
    MT9P017MIPIGetInfo,
    MT9P017MIPIGetResolution,
    MT9P017MIPIFeatureControl,
    MT9P017MIPIControl,
    MT9P017MIPIClose
};

UINT32 MT9P017MIPISensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncMT9P017MIPI;

    return ERROR_NONE;
}   /* SensorInit() */

