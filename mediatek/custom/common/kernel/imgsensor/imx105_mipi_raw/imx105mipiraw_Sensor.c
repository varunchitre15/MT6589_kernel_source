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
 *   Jun Pei (MTK70837)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 06 13 2011 koli.lin
 * [ALPS00053429] [Need Patch] [Volunteer Patch]
 * [Camera] Modify the sensor color order for the CCT tool.
 *
 * 06 07 2011 koli.lin
 * [ALPS00050047] [Camera]AE flash when set EV as -2
 * [Camera] Rollback the preview resolution to 800x600.
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

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx105mipiraw_Sensor.h"
#include "imx105mipiraw_Camera_Sensor_para.h"
#include "imx105mipiraw_CameraCustomized.h"


//#define PRE_PCLK 135200000
static DEFINE_SPINLOCK(imx105_drv_lock);

#define IMX105MIPI_DEBUG
#ifdef IMX105MIPI_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

static struct IMX105MIPI_sensor_STRUCT IMX105MIPI_sensor={IMX105MIPI_WRITE_ID_1,IMX105MIPI_READ_ID_1,KAL_TRUE,KAL_FALSE,KAL_TRUE,KAL_FALSE,
KAL_FALSE,135200000,135200000/*114400000*/,800,0,64,64,3536,1270,3536,2534,0,0,0,0,30};

MSDK_SCENARIO_ID_ENUM CurrentScenarioId = ACDK_SCENARIO_ID_CAMERA_PREVIEW;

kal_uint8 SCCB_Slave_addr[3][2] = {{IMX105MIPI_WRITE_ID_1,IMX105MIPI_READ_ID_1},{IMX105MIPI_WRITE_ID_2,IMX105MIPI_READ_ID_2},{IMX105MIPI_WRITE_ID_3,IMX105MIPI_READ_ID_3}};


/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT IMX105MIPISensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT IMX105MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/

MSDK_SENSOR_CONFIG_STRUCT IMX105MIPISensorConfigData;
/* Parameter For Engineer mode function */
kal_uint32 IMX105MIPI_FAC_SENSOR_REG;
kal_bool IMX105MIPI_Auto_Flicker_mode = KAL_FALSE;
kal_bool IMX105MIPI_MPEG4_encode_mode = KAL_FALSE;



kal_uint16 IMX105MIPI_MAX_EXPOSURE_LINES = IMX105MIPI_PV_FRAME_LENGTH_LINES -5;
kal_uint8  IMX105MIPI_MIN_EXPOSURE_LINES = 1;

kal_uint32 IMX105MIPI_isp_master_clock;
UINT8 IMX105MIPIPixelClockDivider=0;

#define IMX105MIPI_MaxGainIndex 50																				 // Gain Index
kal_uint16 IMX105MIPI_sensorGainMapping[IMX105MIPI_MaxGainIndex][2] = {
    { 64,  0}, { 68, 15}, { 71, 28}, { 75, 40}, { 79, 51}, {84, 61}, { 88, 70}, { 92, 78}, { 95, 85}, { 99, 92},
    {103, 98}, { 107, 104}, {112,110}, {116,115}, {119,119}, {124,124}, {128,128}, {136,136}, {143,142}, {151,148},
    {160,154}, {167,158}, {176,163}, {184,167}, {192,171}, {199,174}, {207,177}, {215,180}, {224,183}, {230,185},
    {240,188}, {248,190}, {256,192}, {273,196}, {287,199}, {303,202}, {321,205}, {334,207}, {348,209}, {364,211}, 
    {381,213}, {399,215}, {420,217}, {431,218}, {442,219}, {455,220}, {468,221}, {481,222}, {496,223}, {512,224}    
};



extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint8 IMX105MIPI_read_cmos_sensor(kal_uint16 addr)
{
	kal_uint8 get_byte=0;
    char puSendCmd[2] = {(char)((addr&0xFF00) >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,IMX105MIPI_sensor.i2c_write_id);
    return get_byte;
}

void IMX105MIPI_write_cmos_sensor(kal_uint16 addr, kal_uint8 para)
{

	char puSendCmd[3] = {(char)((addr&0xFF00) >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 3,IMX105MIPI_sensor.i2c_write_id);
}


void IMX105MIPI_write_shutter(kal_uint16 shutter)
{
  #if 1
	kal_uint32 frame_length = 0,line_length=0;
    kal_uint32 extra_lines = 0;
	kal_uint32 max_exp_shutter = 0;
	unsigned long flags;
	
	SENSORDB("[IMX105MIPI]%s()\n",__FUNCTION__);


     if (IMX105MIPI_sensor.pv_mode == KAL_TRUE) 
	 {
	   max_exp_shutter = IMX105MIPI_PV_FRAME_LENGTH_LINES + IMX105MIPI_sensor.pv_dummy_lines-5;
     }
     else
     {
       max_exp_shutter = IMX105MIPI_FULL_FRAME_LENGTH_LINES + IMX105MIPI_sensor.cp_dummy_lines-5;
	 }


	 if(shutter > max_exp_shutter)
	   extra_lines = shutter - max_exp_shutter;
	 else 
	   extra_lines = 0;

	 if (IMX105MIPI_sensor.pv_mode == KAL_TRUE) 
	 {
       frame_length = IMX105MIPI_PV_FRAME_LENGTH_LINES + IMX105MIPI_sensor.pv_dummy_lines + extra_lines;
	   line_length = IMX105MIPI_PV_LINE_LENGTH_PIXELS + IMX105MIPI_sensor.pv_dummy_pixels;
	   if(IMX105MIPI_Auto_Flicker_mode = TRUE)
	   	 frame_length = frame_length + (frame_length >> 7);
	   
	   spin_lock_irqsave(&imx105_drv_lock,flags);
	   IMX105MIPI_sensor.pv_line_length = line_length;
	   IMX105MIPI_sensor.pv_frame_length = frame_length;
	   spin_unlock_irqrestore(&imx105_drv_lock,flags);
	 }
	 else
     {
	    frame_length = IMX105MIPI_FULL_FRAME_LENGTH_LINES + IMX105MIPI_sensor.cp_dummy_lines + extra_lines;
		line_length = IMX105MIPI_FULL_LINE_LENGTH_PIXELS + IMX105MIPI_sensor.cp_dummy_pixels;
		if(IMX105MIPI_Auto_Flicker_mode = TRUE)
	   	  frame_length = frame_length + (frame_length >> 7);
		 
		spin_lock_irqsave(&imx105_drv_lock,flags);
		IMX105MIPI_sensor.cp_line_length = line_length;
	    IMX105MIPI_sensor.cp_frame_length = frame_length;
		spin_unlock_irqrestore(&imx105_drv_lock,flags);
	 }

   SENSORDB("[IMX105MIPI]Write_shutter:pv_mode =%d,shutter=%d,frame_length=%d\n",IMX105MIPI_sensor.pv_mode,shutter,frame_length);

  //  IMX105MIPI_write_cmos_sensor(0x0104, 1);        

    IMX105MIPI_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
    IMX105MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);	

    IMX105MIPI_write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
    IMX105MIPI_write_cmos_sensor(0x0203, shutter  & 0xFF);
 //   IMX105MIPI_write_cmos_sensor(0x0104, 0);    
 
 

#else
    kal_uint32 frame_length = 0,dummy_lines = 0;
	kal_bool change_length = KAL_FALSE;
	kal_uint32 max_exp_shutter = 0;
	//return;
	SENSORDB("[IMX105MIPI]%s()\n",__FUNCTION__);
		
    if (IMX105MIPI_sensor.pv_mode == KAL_TRUE) 
	{
	   max_exp_shutter = IMX105MIPI_sensor.pv_frame_length - 5;
	   
       if(shutter > max_exp_shutter)
       	{ 
       	  change_length = KAL_TRUE;
       	  frame_length = shutter + 5;
          IMX105MIPI_sensor.pv_frame_length = frame_length;
		  dummy_lines = IMX105MIPI_sensor.pv_frame_length - IMX105MIPI_PV_FRAME_LENGTH_LINES;
		  IMX105MIPI_sensor.pv_dummy_lines = dummy_lines;
       	}  

	   SENSORDB("[IMX105MIPI]Write_shutter:pv_mode =%d,shutter=%d,frame_length=%d,dummy_lines=%d\n",IMX105MIPI_sensor.pv_mode,shutter, IMX105MIPI_sensor.pv_frame_length,dummy_lines);
    }
	else 
	{
	    max_exp_shutter = IMX105MIPI_sensor.cp_frame_length - 5;
        if(shutter > max_exp_shutter)
       	{
       	  change_length = KAL_TRUE;
       	  frame_length = shutter+5;
          IMX105MIPI_sensor.cp_frame_length = frame_length;
		  dummy_lines = IMX105MIPI_sensor.cp_frame_length - IMX105MIPI_FULL_FRAME_LENGTH_LINES;
		  IMX105MIPI_sensor.cp_dummy_lines = dummy_lines;
       	} 

		SENSORDB("[IMX105MIPI]Write_shutter:pv_mode =%d,shutter=%d,frame_length=%d,dummy_lines=%d\n",IMX105MIPI_sensor.pv_mode,shutter, IMX105MIPI_sensor.cp_frame_length,dummy_lines);
    }
	
   // IMX105MIPI_write_cmos_sensor(0x0104, 1);        

    if(change_length == KAL_TRUE)
    {
      IMX105MIPI_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
      IMX105MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);	
    }

    IMX105MIPI_write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
    IMX105MIPI_write_cmos_sensor(0x0203, shutter  & 0xFF);
   // IMX105MIPI_write_cmos_sensor(0x0104, 0);     
 #endif
   
}   /* write_IMX105MIPI_shutter */


static kal_uint16 IMX105MIPIReg2Gain(const kal_uint8 iReg)
{
   #if 1
       kal_uint8 iI;

    // Range: 1x to 8x
    for (iI = 0; iI < IMX105MIPI_MaxGainIndex; iI++) {
        if(iReg <= IMX105MIPI_sensorGainMapping[iI][1]){
            break;
        }
    }

    return IMX105MIPI_sensorGainMapping[iI][0];
	
   #else
    kal_uint16 gain = 0;

	 if(iReg < 256)
	 gain = 256/(256 - iReg);

	 gain = gain*BASEGAIN;

	return gain;  
   #endif
}


static kal_uint8 IMX105MIPIGain2Reg(const kal_uint16 iGain)
{
   #if 1
   kal_uint8 iI;

    for (iI = 0; iI < (IMX105MIPI_MaxGainIndex-1); iI++) {
        if(iGain <= IMX105MIPI_sensorGainMapping[iI][0]){
            break;
        }
    }
   
    if(iGain != IMX105MIPI_sensorGainMapping[iI][0])
    {
         printk("[IMX105MIPIGain2Reg] Gain mapping don't correctly:%d %d \n", iGain, IMX105MIPI_sensorGainMapping[iI][0]);
    }
	
    return IMX105MIPI_sensorGainMapping[iI][1];
   #else
    kal_uint8 iReg = 0x00;
	kal_uint16 reg_gain = 0;

	reg_gain = iGain/BASEGAIN;
    		
	iReg = 256 - 256/reg_gain;

	 if(iReg>240)
	 	{
          iReg = 240;
		  SENSORDB("[IMX105MIPI]%s():Error gain_reg>240\n",__FUNCTION__);
	 	}
	 else if(iReg<0)
	 	{
          iReg = 0;
		  SENSORDB("[IMX105MIPI]%s():Error gain_reg<240\n",__FUNCTION__);
	 	}	

    return iReg; 
	#endif
}


/*************************************************************************
* FUNCTION
*    IMX105MIPI_SetGain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    gain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX105MIPI_SetGain(UINT16 iGain)
{
    kal_uint8 iReg;

    iReg = IMX105MIPIGain2Reg(iGain);

  //  IMX105MIPI_write_cmos_sensor(0x0104, 1);  
    IMX105MIPI_write_cmos_sensor(0x0205, iReg);
//	IMX105MIPI_write_cmos_sensor(0x0104, 0);  
	
   SENSORDB("[IMX105MIPI]%s(),gain=%d,0x0205=%x\n",__FUNCTION__,iGain,IMX105MIPI_read_cmos_sensor(0x0205));

}   /*  IMX105MIPI_SetGain_SetGain  */



/*************************************************************************
* FUNCTION
*    IMX105MIPI_Read_Gain
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
/*
kal_uint8 IMX105MIPI_Read_Gain(void)
{
   kal_uint8 gain = 0;
   kal_uint8 reg_value = IMX105MIPI_read_cmos_sensor(0x0205);
   
	gain = IMX105MIPIReg2Gain(reg_value);
	
	SENSORDB("[IMX105MIPI]%s():0x0205=%x, gain=%d\n",__FUNCTION__,reg_value,gain);

	return gain;
}  

*/

void IMX105MIPI_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=IMX105MIPISensorReg[i].Addr; i++)
    {
        IMX105MIPI_write_cmos_sensor(IMX105MIPISensorReg[i].Addr, IMX105MIPISensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX105MIPISensorReg[i].Addr; i++)
    {
        IMX105MIPI_write_cmos_sensor(IMX105MIPISensorReg[i].Addr, IMX105MIPISensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR+1; i<5; i++)
    {
        IMX105MIPI_write_cmos_sensor(IMX105MIPISensorCCT[i].Addr, IMX105MIPISensorCCT[i].Para);
    }
}



/*************************************************************************
* FUNCTION
*    IMX105MIPI_sensor_to_camera_para
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
void IMX105MIPI_sensor_to_camera_para(void)
{
    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=IMX105MIPISensorReg[i].Addr; i++)
    {
        temp_data = IMX105MIPI_read_cmos_sensor(IMX105MIPISensorReg[i].Addr);
		spin_lock(&imx105_drv_lock);
        IMX105MIPISensorReg[i].Para = temp_data;
		spin_unlock(&imx105_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX105MIPISensorReg[i].Addr; i++)
    {
        temp_data = IMX105MIPI_read_cmos_sensor(IMX105MIPISensorReg[i].Addr);
		spin_lock(&imx105_drv_lock);
        IMX105MIPISensorReg[i].Para = temp_data;
		spin_unlock(&imx105_drv_lock);
    }
}


/*************************************************************************
* FUNCTION
*    IMX105MIPI_get_sensor_group_count
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
kal_int32  IMX105MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void IMX105MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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


void IMX105MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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
                 SENSORDB("[IMX105MIPI][Error]get_sensor_item_info error!!!\n");
          }

		   temp_para = IMX105MIPISensorCCT[temp_addr].Para;
           //Transfer Reg value to Gain value
			if (item_idx == 4){ //base_gain
				temp_gain = IMX105MIPIReg2Gain(temp_para);		
				info_ptr->Min=1*1000;
                info_ptr->Max=8*1000;
				info_ptr->ItemValue=1000*temp_gain/BASEGAIN;
			}else{	//R, G, B gain
	            info_ptr->ItemValue=1000+1000*temp_para/256;
				info_ptr->Min=1000;
                info_ptr->Max=2000; ///2x        
			}          
                info_ptr->IsTrueFalse=KAL_FALSE;
                info_ptr->IsReadOnly=KAL_FALSE;
                info_ptr->IsNeedRestart=KAL_FALSE;		
            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");
                
                    //temp_reg=IMX105MIPISensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=IMX105MIPI_MAX_EXPOSURE_LINES;
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



kal_bool IMX105MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
   kal_uint16 temp_addr=0, temp_para=0;
   kal_uint16 temp_gain =0;

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
                 SENSORDB("[IMX105MIPI][Error]set_sensor_item_info error!!!\n");
          }

		    temp_gain=((ItemValue*BASEGAIN+500)/1000);          //+500:get closed integer value

			//Gain value trnsfer to the Register value
			if (temp_addr == SENSOR_BASEGAIN){		
				temp_para = IMX105MIPIGain2Reg(temp_gain); 	

			}else{
                   // temp_para = (temp_gain/BASEGAIN - 1)*256;
                    temp_para = (temp_gain-BASEGAIN)*256/BASEGAIN;
			     }

            spin_lock(&imx105_drv_lock);
			IMX105MIPISensorCCT[temp_addr].Para = temp_para;
			spin_unlock(&imx105_drv_lock);
			IMX105MIPI_write_cmos_sensor(IMX105MIPISensorCCT[temp_addr].Addr, temp_para);

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
					spin_lock(&imx105_drv_lock);
                    if(ItemValue==2)
                    {
                        IMX105MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
                        //IMX105MIPI_set_isp_driving_current(ISP_DRIVING_2MA);
                    }
                    else if(ItemValue==3 || ItemValue==4)
                    {
                        IMX105MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
                        //IMX105MIPI_set_isp_driving_current(ISP_DRIVING_4MA);
                    }
                    else if(ItemValue==5 || ItemValue==6)
                    {
                        IMX105MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
                        //IMX105MIPI_set_isp_driving_current(ISP_DRIVING_6MA);
                    }
                    else
                    {
                        IMX105MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
                        //IMX105MIPI_set_isp_driving_current(ISP_DRIVING_8MA);
                    }
					spin_unlock(&imx105_drv_lock);
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
					spin_lock(&imx105_drv_lock);
                    IMX105MIPI_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&imx105_drv_lock);
                    break;
                case 1:
                    IMX105MIPI_write_cmos_sensor(IMX105MIPI_FAC_SENSOR_REG,ItemValue);
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

static void IMX105MIPI_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
  kal_uint32 frame_length = 0, line_length = 0;


   if(IMX105MIPI_sensor.pv_mode == KAL_TRUE)
   	{
   	 spin_lock(&imx105_drv_lock);
   	 IMX105MIPI_sensor.pv_dummy_pixels = iPixels;
	 IMX105MIPI_sensor.pv_dummy_lines = iLines;
   	 IMX105MIPI_sensor.pv_line_length = IMX105MIPI_PV_LINE_LENGTH_PIXELS + iPixels;
	 IMX105MIPI_sensor.pv_frame_length = IMX105MIPI_PV_FRAME_LENGTH_LINES + iLines;
	 spin_unlock(&imx105_drv_lock);
	 line_length = IMX105MIPI_sensor.pv_line_length;
	 frame_length = IMX105MIPI_sensor.pv_frame_length;
	 	
   	}
   else
   	{
      spin_lock(&imx105_drv_lock);
   	  IMX105MIPI_sensor.cp_dummy_pixels = iPixels;
	  IMX105MIPI_sensor.cp_dummy_lines = iLines;
	  IMX105MIPI_sensor.cp_line_length = IMX105MIPI_FULL_LINE_LENGTH_PIXELS + iPixels;
	  IMX105MIPI_sensor.cp_frame_length = IMX105MIPI_FULL_FRAME_LENGTH_LINES + iLines;
	  spin_unlock(&imx105_drv_lock);
	  line_length = IMX105MIPI_sensor.cp_line_length;
	  frame_length = IMX105MIPI_sensor.cp_frame_length;
    }

      IMX105MIPI_write_cmos_sensor(0x0104, 1);        
	  
      IMX105MIPI_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
      IMX105MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);	
      IMX105MIPI_write_cmos_sensor(0x0342, (line_length >>8) & 0xFF);
      IMX105MIPI_write_cmos_sensor(0x0343, line_length & 0xFF);

      IMX105MIPI_write_cmos_sensor(0x0104, 0);

	  SENSORDB("[IMX105MIPI]%s(),dumy_pixel=%d,dumy_line=%d,\n",__FUNCTION__,iPixels,iLines);
	  SENSORDB("[IMX105MIPI]pv_mode=%d,line_length=%d,frame_length=%d,\n",IMX105MIPI_sensor.pv_mode,line_length,frame_length);
	  SENSORDB("[IMX105MIPI]0x340=%x,0x341=%x\n",IMX105MIPI_read_cmos_sensor(0x0340),IMX105MIPI_read_cmos_sensor(0x0341));
	  SENSORDB("[IMX105MIPI]0x342=%x,0x343=%x\n",IMX105MIPI_read_cmos_sensor(0x0342),IMX105MIPI_read_cmos_sensor(0x0343));
  
}   /*  IMX105MIPI_SetDummy */

static void IMX105MIPI_Sensor_Init(void)
{    
    //Global Setting																																	
	IMX105MIPI_write_cmos_sensor(0x3031, 0x10); 
	//0x3064 gain mode select: mode 1,set gain code value calc from gain=256/(256-ANA_GAIN_GLOBAL)
	IMX105MIPI_write_cmos_sensor(0x3064, 0x12); 																		
	IMX105MIPI_write_cmos_sensor(0x3087, 0x57); 																		
	IMX105MIPI_write_cmos_sensor(0x308A, 0x35); 																		
	IMX105MIPI_write_cmos_sensor(0x3091, 0x41); 																		
	IMX105MIPI_write_cmos_sensor(0x3098, 0x03); 																		
	IMX105MIPI_write_cmos_sensor(0x3099, 0xC0); 																		
	IMX105MIPI_write_cmos_sensor(0x309A, 0xA3); 																		
	IMX105MIPI_write_cmos_sensor(0x309D, 0x94); 																		
	IMX105MIPI_write_cmos_sensor(0x30AB, 0x01); 																		
	IMX105MIPI_write_cmos_sensor(0x30AD, 0x08); 																		
	IMX105MIPI_write_cmos_sensor(0x30B1, 0x03); 																		
	IMX105MIPI_write_cmos_sensor(0x30F3, 0x03); 																		
	IMX105MIPI_write_cmos_sensor(0x3116, 0x31); 																		
	IMX105MIPI_write_cmos_sensor(0x3117, 0x38); 																		
	IMX105MIPI_write_cmos_sensor(0x3138, 0x28); 																		
	IMX105MIPI_write_cmos_sensor(0x3137, 0x14); 																		
	IMX105MIPI_write_cmos_sensor(0x3139, 0x2E); 																		
	IMX105MIPI_write_cmos_sensor(0x314D, 0x2A); 																		
	IMX105MIPI_write_cmos_sensor(0x3343, 0x04); 																		
																																										  
    //Black level Setting 																														
	IMX105MIPI_write_cmos_sensor(0x3032, 0x40); 
	  																				
	IMX105MIPI_write_cmos_sensor(0x0305, 0x01); 									  
   //PLL Setting "EXTCLK=26MHz, PLL=676MHz"	
	IMX105MIPI_write_cmos_sensor(0x0307, 0x1A); 
	IMX105MIPI_write_cmos_sensor(0x303C, 0x54); 

    //Preview Mode Setting		
	IMX105MIPI_write_cmos_sensor(0x0340, 0x04); //fame_length_line:0x4F6=1270											  
	IMX105MIPI_write_cmos_sensor(0x0341, 0xF6); 										  
	IMX105MIPI_write_cmos_sensor(0x0342, 0x0D); //Line_length_pck:0xDD0=3536											  
	IMX105MIPI_write_cmos_sensor(0x0343, 0xD0); 										  
	IMX105MIPI_write_cmos_sensor(0x0344, 0x00); //x_addr_start: 0x04=4										  
	IMX105MIPI_write_cmos_sensor(0x0345, 0x04); 										  
	IMX105MIPI_write_cmos_sensor(0x0346, 0x00); //y_addr_start:0x24=36										  
	IMX105MIPI_write_cmos_sensor(0x0347, 0x24); 										  
	IMX105MIPI_write_cmos_sensor(0x0348, 0x0C); //x_addr_end:0xCD3=3283										  
	IMX105MIPI_write_cmos_sensor(0x0349, 0xD3); 										  
	IMX105MIPI_write_cmos_sensor(0x034A, 0x09); //y_addr_end:0x9c3=2499											  
	IMX105MIPI_write_cmos_sensor(0x034B, 0xC3); 										  
	IMX105MIPI_write_cmos_sensor(0x034C, 0x06); //x_output_size:0x668=1640											  
	IMX105MIPI_write_cmos_sensor(0x034D, 0x68); 										  
	IMX105MIPI_write_cmos_sensor(0x034E, 0x04); //y_output_size:0x4D0=1232										  
	IMX105MIPI_write_cmos_sensor(0x034F, 0xD0); 										  
	IMX105MIPI_write_cmos_sensor(0x0381, 0x01); //x_even_inc:0x01=1											  
	IMX105MIPI_write_cmos_sensor(0x0383, 0x03); //x_odd_inc:0x03=3											  
	IMX105MIPI_write_cmos_sensor(0x0385, 0x01); //y_even_inc:0x01=1											  
	IMX105MIPI_write_cmos_sensor(0x0387, 0x03); //y_odd_inc:0x03=3										  
	IMX105MIPI_write_cmos_sensor(0x3033, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x303D, 0x70); 										  
	IMX105MIPI_write_cmos_sensor(0x303E, 0x40); 										  
	IMX105MIPI_write_cmos_sensor(0x3040, 0x08); 										  
	IMX105MIPI_write_cmos_sensor(0x3041, 0x97); 										  
	IMX105MIPI_write_cmos_sensor(0x3048, 0x01); 										  
	IMX105MIPI_write_cmos_sensor(0x304C, 0x6F); 										  
	IMX105MIPI_write_cmos_sensor(0x304D, 0x03); 										  
	IMX105MIPI_write_cmos_sensor(0x306A, 0xD2); 										  
	IMX105MIPI_write_cmos_sensor(0x309B, 0x28); 										  
	IMX105MIPI_write_cmos_sensor(0x309C, 0x34); 										  
	IMX105MIPI_write_cmos_sensor(0x309E, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x30AA, 0x02); 										  
	IMX105MIPI_write_cmos_sensor(0x30D5, 0x09); 										  
	IMX105MIPI_write_cmos_sensor(0x30D6, 0x01); 										  
	IMX105MIPI_write_cmos_sensor(0x30D7, 0x01); 										  
	IMX105MIPI_write_cmos_sensor(0x30D8, 0x64); 										  
	IMX105MIPI_write_cmos_sensor(0x30D9, 0x89); 										  
	IMX105MIPI_write_cmos_sensor(0x30DE, 0x02); 										  
	IMX105MIPI_write_cmos_sensor(0x3102, 0x08); 										  
	IMX105MIPI_write_cmos_sensor(0x3103, 0x22); 										  
	IMX105MIPI_write_cmos_sensor(0x3104, 0x20); 										  
	IMX105MIPI_write_cmos_sensor(0x3105, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x3106, 0x87); 										  
	IMX105MIPI_write_cmos_sensor(0x3107, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x315C, 0xA5); 										  
	IMX105MIPI_write_cmos_sensor(0x315D, 0xA4); 										  
	IMX105MIPI_write_cmos_sensor(0x316E, 0xA6); 										  
	IMX105MIPI_write_cmos_sensor(0x316F, 0xA5); 										  
	IMX105MIPI_write_cmos_sensor(0x3301, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x3318, 0x72); 
	
    //Shutter Gain setting																		
	 IMX105MIPI_write_cmos_sensor(0x0202, 0x02); //shutter	 
	 IMX105MIPI_write_cmos_sensor(0x0203, 0x00);	 
	 IMX105MIPI_write_cmos_sensor(0x0205, 0x00);//analoge_gain:  1x gain 
	 IMX105MIPI_write_cmos_sensor(0x020E, 0x01);//digital_gain_Gr: 1x gain		 
	 IMX105MIPI_write_cmos_sensor(0x020F, 0x00);	 
	 IMX105MIPI_write_cmos_sensor(0x0210, 0x01);//digital_gain_R:1x gain	 
	 IMX105MIPI_write_cmos_sensor(0x0211, 0x00);	 
	 IMX105MIPI_write_cmos_sensor(0x0212, 0x01);//digital_gain_B:1X gain	 
	 IMX105MIPI_write_cmos_sensor(0x0213, 0x00);	 
	 IMX105MIPI_write_cmos_sensor(0x0214, 0x01); //digital_gain_Gb:1X gain	 
	 IMX105MIPI_write_cmos_sensor(0x0215, 0x00);	 
	
	//Streaming																		  
	IMX105MIPI_write_cmos_sensor(0x0100, 0x01); 											
	//Streaming is started from here. =>	PLL Lock Up Time + Wake-Up Time ¡ú Init. Time
	 spin_lock(&imx105_drv_lock);
     IMX105MIPI_Auto_Flicker_mode = KAL_FALSE;
	 spin_unlock(&imx105_drv_lock);
}   /*  IMX105MIPI_Sensor_Init  */

void IMX105MIPI_Set_720P_PVsize(void)
{	
    SENSORDB("[IMX105MIPI]%s()\n",__FUNCTION__);
	
	IMX105MIPI_write_cmos_sensor(0x0100, 0x00); 
    //Preview Mode Setting																	
	IMX105MIPI_write_cmos_sensor(0x0340, 0x04); 										  
	IMX105MIPI_write_cmos_sensor(0x0341, 0xF6); 										  
	IMX105MIPI_write_cmos_sensor(0x0342, 0x0D); 										  
	IMX105MIPI_write_cmos_sensor(0x0343, 0xD0); 										  
	IMX105MIPI_write_cmos_sensor(0x0344, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x0345, 0x04); 										  
	IMX105MIPI_write_cmos_sensor(0x0346, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x0347, 0x24); 										  
	IMX105MIPI_write_cmos_sensor(0x0348, 0x0C); 										  
	IMX105MIPI_write_cmos_sensor(0x0349, 0xD3); 										  
	IMX105MIPI_write_cmos_sensor(0x034A, 0x09); 										  
	IMX105MIPI_write_cmos_sensor(0x034B, 0xC3); 										  
	IMX105MIPI_write_cmos_sensor(0x034C, 0x06); 										  
	IMX105MIPI_write_cmos_sensor(0x034D, 0x68); 										  
	IMX105MIPI_write_cmos_sensor(0x034E, 0x04); 										  
	IMX105MIPI_write_cmos_sensor(0x034F, 0xD0); 										  
	IMX105MIPI_write_cmos_sensor(0x0381, 0x01); 										  
	IMX105MIPI_write_cmos_sensor(0x0383, 0x03); 										  
	IMX105MIPI_write_cmos_sensor(0x0385, 0x01); 										  
	IMX105MIPI_write_cmos_sensor(0x0387, 0x03); 										  
	IMX105MIPI_write_cmos_sensor(0x3033, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x303D, 0x70); 										  
	IMX105MIPI_write_cmos_sensor(0x303E, 0x40); 										  
	IMX105MIPI_write_cmos_sensor(0x3040, 0x08); 										  
	IMX105MIPI_write_cmos_sensor(0x3041, 0x97); 										  
	IMX105MIPI_write_cmos_sensor(0x3048, 0x01); 										  
	IMX105MIPI_write_cmos_sensor(0x304C, 0x6F); 										  
	IMX105MIPI_write_cmos_sensor(0x304D, 0x03); 										  
	IMX105MIPI_write_cmos_sensor(0x306A, 0xD2); 										  
	IMX105MIPI_write_cmos_sensor(0x309B, 0x28); 										  
	IMX105MIPI_write_cmos_sensor(0x309C, 0x34); 										  
	IMX105MIPI_write_cmos_sensor(0x309E, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x30AA, 0x02); 										  
	IMX105MIPI_write_cmos_sensor(0x30D5, 0x09); 										  
	IMX105MIPI_write_cmos_sensor(0x30D6, 0x01); 										  
	IMX105MIPI_write_cmos_sensor(0x30D7, 0x01); 										  
	IMX105MIPI_write_cmos_sensor(0x30D8, 0x64); 										  
	IMX105MIPI_write_cmos_sensor(0x30D9, 0x89); 										  
	IMX105MIPI_write_cmos_sensor(0x30DE, 0x02); 										  
	IMX105MIPI_write_cmos_sensor(0x3102, 0x08); 										  
	IMX105MIPI_write_cmos_sensor(0x3103, 0x22); 										  
	IMX105MIPI_write_cmos_sensor(0x3104, 0x20); 										  
	IMX105MIPI_write_cmos_sensor(0x3105, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x3106, 0x87); 										  
	IMX105MIPI_write_cmos_sensor(0x3107, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x315C, 0xA5); 										  
	IMX105MIPI_write_cmos_sensor(0x315D, 0xA4); 										  
	IMX105MIPI_write_cmos_sensor(0x316E, 0xA6); 										  
	IMX105MIPI_write_cmos_sensor(0x316F, 0xA5); 										  
	IMX105MIPI_write_cmos_sensor(0x3301, 0x00); 										  
	IMX105MIPI_write_cmos_sensor(0x3318, 0x72); 	
	//Shutter Gain setting	
	 #if 0
	 IMX105MIPI_write_cmos_sensor(0x0202, 0x02); //shutter	 
	 IMX105MIPI_write_cmos_sensor(0x0203, 0x00);
	
	 IMX105MIPI_write_cmos_sensor(0x0205, 0x00);//analoge_gain:  1x gain 
	 IMX105MIPI_write_cmos_sensor(0x020E, 0x01);//digital_gain_Gr: 1x gain		 
	 IMX105MIPI_write_cmos_sensor(0x020F, 0x00);	 
	 IMX105MIPI_write_cmos_sensor(0x0210, 0x01);//digital_gain_R:1x gain	 
	 IMX105MIPI_write_cmos_sensor(0x0211, 0x00);	 
	 IMX105MIPI_write_cmos_sensor(0x0212, 0x01);//digital_gain_B:1X gain	 
	 IMX105MIPI_write_cmos_sensor(0x0213, 0x00);	 
	 IMX105MIPI_write_cmos_sensor(0x0214, 0x01); //digital_gain_Gb:1X gain	 
	 IMX105MIPI_write_cmos_sensor(0x0215, 0x00);
	 #endif
	//Streaming 																	  
	IMX105MIPI_write_cmos_sensor(0x0100, 0x01); 											
	//Streaming is started from here. =>	PLL Lock Up Time + Wake-Up Time ¡ú Init. Time	

}

void IMX105MIPI_set_8M_FullSize(void)
{	
       SENSORDB("[IMX105MIPI]%s()\n",__FUNCTION__);

       //must add this line to change from stream mode to standby mode, else modify PLL will not take effect
       IMX105MIPI_write_cmos_sensor(0x0100, 0x00); 
	
       //Capture Mode Setting		
	   IMX105MIPI_write_cmos_sensor(0x0340, 0x09);			
	   IMX105MIPI_write_cmos_sensor(0x0341, 0xE6);			
	   IMX105MIPI_write_cmos_sensor(0x0342, 0x0D);			
	   IMX105MIPI_write_cmos_sensor(0x0343, 0xD0);			
	   IMX105MIPI_write_cmos_sensor(0x0344, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x0345, 0x04);			
	   IMX105MIPI_write_cmos_sensor(0x0346, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x0347, 0x24);			
	   IMX105MIPI_write_cmos_sensor(0x0348, 0x0C);			
	   IMX105MIPI_write_cmos_sensor(0x0349, 0xD3);			
	   IMX105MIPI_write_cmos_sensor(0x034A, 0x09);			
	   IMX105MIPI_write_cmos_sensor(0x034B, 0xC3);			
	   IMX105MIPI_write_cmos_sensor(0x034C, 0x0C);			
	   IMX105MIPI_write_cmos_sensor(0x034D, 0xD0);			
	   IMX105MIPI_write_cmos_sensor(0x034E, 0x09);			
	   IMX105MIPI_write_cmos_sensor(0x034F, 0xA0);			
	   IMX105MIPI_write_cmos_sensor(0x0381, 0x01);			
	   IMX105MIPI_write_cmos_sensor(0x0383, 0x01);			
	   IMX105MIPI_write_cmos_sensor(0x0385, 0x01);			
	   IMX105MIPI_write_cmos_sensor(0x0387, 0x01);			
	   IMX105MIPI_write_cmos_sensor(0x3033, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x303D, 0x60);			
	   IMX105MIPI_write_cmos_sensor(0x303E, 0x40);			
	   IMX105MIPI_write_cmos_sensor(0x3040, 0x08);			
	   IMX105MIPI_write_cmos_sensor(0x3041, 0x97);			
	   IMX105MIPI_write_cmos_sensor(0x3048, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x304C, 0x6F);			
	   IMX105MIPI_write_cmos_sensor(0x304D, 0x03);			
	   IMX105MIPI_write_cmos_sensor(0x306A, 0xD2);			
	   IMX105MIPI_write_cmos_sensor(0x309B, 0x20);			
	   IMX105MIPI_write_cmos_sensor(0x309C, 0x34);			
	   IMX105MIPI_write_cmos_sensor(0x309E, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x30AA, 0x02);			
	   IMX105MIPI_write_cmos_sensor(0x30D5, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x30D6, 0x85);			
	   IMX105MIPI_write_cmos_sensor(0x30D7, 0x2A);			
	   IMX105MIPI_write_cmos_sensor(0x30D8, 0x64);			
	   IMX105MIPI_write_cmos_sensor(0x30D9, 0x89);			
	   IMX105MIPI_write_cmos_sensor(0x30DE, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x3102, 0x08);			
	   IMX105MIPI_write_cmos_sensor(0x3103, 0x22);			
	   IMX105MIPI_write_cmos_sensor(0x3104, 0x20);			
	   IMX105MIPI_write_cmos_sensor(0x3105, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x3106, 0x87);			
	   IMX105MIPI_write_cmos_sensor(0x3107, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x315C, 0xA5);			
	   IMX105MIPI_write_cmos_sensor(0x315D, 0xA4);			
	   IMX105MIPI_write_cmos_sensor(0x316E, 0xA6);			
	   IMX105MIPI_write_cmos_sensor(0x316F, 0xA5);			
	   IMX105MIPI_write_cmos_sensor(0x3301, 0x00);			
	   IMX105MIPI_write_cmos_sensor(0x3318, 0x62);									
		//Shutter Gain setting	
		#if 0
	   IMX105MIPI_write_cmos_sensor(0x0202, 0x02); //shutter	 
	   IMX105MIPI_write_cmos_sensor(0x0203, 0x00);	
	   #endif
	   //IMX105MIPI_write_cmos_sensor(0x0205, 0x00);//analoge_gain:  1x gain 
	  // IMX105MIPI_write_cmos_sensor(0x020E, 0x01);//digital_gain_Gr: 1x gain		 
	  // IMX105MIPI_write_cmos_sensor(0x020F, 0x00);	 
	  // IMX105MIPI_write_cmos_sensor(0x0210, 0x01);//digital_gain_R:1x gain	 
	  // IMX105MIPI_write_cmos_sensor(0x0211, 0x00);	 
	  // IMX105MIPI_write_cmos_sensor(0x0212, 0x01);//digital_gain_B:1X gain	 
	  // IMX105MIPI_write_cmos_sensor(0x0213, 0x00);	 
	  // IMX105MIPI_write_cmos_sensor(0x0214, 0x01); //digital_gain_Gb:1X gain	 
	 //  IMX105MIPI_write_cmos_sensor(0x0215, 0x00);	
	   //Streaming				
	   IMX105MIPI_write_cmos_sensor(0x0100, 0x01); 	
   
}



static kal_uint16 IMX105MIPI_power_on(void)
{
    kal_uint8 i;
	kal_uint16 IMX105MIPI_sensor_id = 0;
	
     SENSORDB("[IMX105MIPI]%s()\n",__FUNCTION__);
	
    for(i=0;i<3;i++)
    {
    spin_lock(&imx105_drv_lock);
	IMX105MIPI_sensor.i2c_write_id = SCCB_Slave_addr[i][0];
	IMX105MIPI_sensor.i2c_read_id = SCCB_Slave_addr[i][1];
	spin_unlock(&imx105_drv_lock);

	//Soft Reset
	IMX105MIPI_write_cmos_sensor(0x0103, 0x01);
	mdelay(10);	

	IMX105MIPI_sensor_id = ((IMX105MIPI_read_cmos_sensor(0x0000) << 8) | IMX105MIPI_read_cmos_sensor(0x0001));
    SENSORDB("[IMX105MIPI]read SENSOR ID(i=%d):I2C_Write_Id = %x,Sensor_ID = %x\n", i,IMX105MIPI_sensor.i2c_write_id,IMX105MIPI_sensor_id);
	
	if (IMX105MIPI_sensor_id == IMX105MIPI_SENSOR_ID)
	 break;
	}

	if(i>=3)
		SENSORDB("[IMX105MIPI]IMX105MIPI_I2C_Read_Error:i=%d\n",i);
	
	return IMX105MIPI_sensor_id;
}
static void IMX105MIPI_Init_Parameter(void)
{
    spin_lock(&imx105_drv_lock);
    IMX105MIPI_sensor.pv_mode = KAL_FALSE;
	IMX105MIPI_sensor.first_init = KAL_TRUE;
    IMX105MIPI_sensor.night_mode = KAL_FALSE;
    IMX105MIPI_sensor.cp_pclk = 135200000;//114400000;  
    IMX105MIPI_sensor.pv_pclk = 135200000;  
    IMX105MIPI_sensor.pv_dummy_pixels = 0;
    IMX105MIPI_sensor.pv_dummy_lines = 0;
	IMX105MIPI_sensor.cp_dummy_pixels = 0;
	IMX105MIPI_sensor.cp_dummy_lines = 0;
	IMX105MIPI_sensor.pv_line_length = IMX105MIPI_PV_LINE_LENGTH_PIXELS + IMX105MIPI_sensor.pv_dummy_pixels;
	IMX105MIPI_sensor.pv_frame_length = IMX105MIPI_PV_FRAME_LENGTH_LINES + IMX105MIPI_sensor.pv_dummy_lines;
	IMX105MIPI_sensor.cp_line_length = IMX105MIPI_FULL_LINE_LENGTH_PIXELS + IMX105MIPI_sensor.cp_dummy_pixels;
	IMX105MIPI_sensor.cp_frame_length = IMX105MIPI_FULL_FRAME_LENGTH_LINES+ IMX105MIPI_sensor.cp_dummy_lines;
    IMX105MIPI_sensor.mirror_flip = 0;  //normal:0, mirror:1, flip: 2, mirror_flip:3
    spin_unlock(&imx105_drv_lock);
   // IMX105MIPI_sensor.pv_shutter = 0x200;
    
}



/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   IMX105MIPIOpen
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

UINT32 IMX105MIPIOpen(void)
{
    SENSORDB("[IMX105MIPI]%s()\n",__FUNCTION__);
  
   if (IMX105MIPI_power_on() != IMX105MIPI_SENSOR_ID) 
   {
	 SENSORDB("[IMX105MIPI]Error:read sensor ID fail\n");
	 return ERROR_SENSOR_CONNECT_FAIL;
   }
 
   IMX105MIPI_Sensor_Init();
   IMX105MIPI_Init_Parameter();
   IMX105MIPI_camera_para_to_sensor();

  return ERROR_NONE;

}

/*************************************************************************
* FUNCTION
*   IMX105MIPIGetSensorID
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
UINT32 IMX105MIPIGetSensorID(UINT32 *sensorID) 
{
    int  retry = 3; 

    // check if sensor ID correct
    do {
        *sensorID = IMX105MIPI_power_on();         
        if (*sensorID == IMX105MIPI_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != IMX105MIPI_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;

}


/*************************************************************************
* FUNCTION
*   IMX105MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of IMX105MIPI to change exposure time.
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
void IMX105MIPI_SetShutter(kal_uint16 iShutter)
{

	 SENSORDB("[IMX105MIPI]%s():shutter=%d\n",__FUNCTION__,iShutter);
   
    if (iShutter < 1)
        iShutter = 1; 
	else if(iShutter > 0xffff)
		iShutter = 0xffff;
	
	spin_lock_irqsave(&imx105_drv_lock,flags);
    IMX105MIPI_sensor.pv_shutter = iShutter;
	spin_unlock_irqrestore(&imx105_drv_lock,flags);
	
    IMX105MIPI_write_shutter(iShutter);
}   /*  IMX105MIPI_SetShutter   */



/*************************************************************************
* FUNCTION
*   IMX105MIPI_read_shutter
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
UINT16 IMX105MIPI_read_shutter(void)
{
    return (UINT16)( (IMX105MIPI_read_cmos_sensor(0x0202)<<8) | IMX105MIPI_read_cmos_sensor(0x0203) );
}

/*************************************************************************
* FUNCTION
*   IMX105MIPI_night_mode
*
* DESCRIPTION
*   This function night mode of IMX105MIPI.
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
void IMX105MIPI_NightMode(kal_bool bEnable)
{
	SENSORDB("[IMX105MIPI]%s():enable=%d\n",__FUNCTION__,bEnable);

}/*	IMX105MIPI_NightMode */


UINT32 IMX105MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
 kal_uint32 pv_max_frame_rate_lines = IMX105MIPI_MAX_EXPOSURE_LINES;

 SENSORDB("[IMX105MIPI]%s():frame rate(10base)=%d %d\n",__FUNCTION__,bEnable,u2FrameRate);

 if(bEnable)
  {
    spin_lock(&imx105_drv_lock);
    IMX105MIPI_Auto_Flicker_mode = KAL_TRUE;
	spin_unlock(&imx105_drv_lock);
	if(IMX105MIPI_MPEG4_encode_mode == KAL_TRUE)
		{
		 pv_max_frame_rate_lines = IMX105MIPI_MAX_EXPOSURE_LINES +(IMX105MIPI_MAX_EXPOSURE_LINES>>7);
		 spin_lock(&imx105_drv_lock);
		 IMX105MIPI_sensor.pv_dummy_lines = pv_max_frame_rate_lines - IMX105MIPI_PV_FRAME_LENGTH_LINES
		 spin_unlock(&imx105_drv_lock);
		 IMX105MIPI_SetDummy(IMX105MIPI_sensor.pv_dummy_pixels,IMX105MIPI_sensor.pv_dummy_lines);
		}
  }
 else
  {
     spin_lock(&imx105_drv_lock);
	 IMX105MIPI_Auto_Flicker_mode = KAL_FALSE;
	 spin_unlock(&imx105_drv_lock);
	 if(IMX105MIPI_MPEG4_encode_mode == KAL_TRUE)
		{
		 pv_max_frame_rate_lines = IMX105MIPI_MAX_EXPOSURE_LINES;
		 spin_lock(&imx105_drv_lock);
		 IMX105MIPI_sensor.pv_dummy_lines = pv_max_frame_rate_lines - IMX105MIPI_PV_FRAME_LENGTH_LINES
		 spin_unlock(&imx105_drv_lock);
		 IMX105MIPI_SetDummy(IMX105MIPI_sensor.pv_dummy_pixels,IMX105MIPI_sensor.pv_dummy_lines);
		}
  }

 return TRUE;
}
	

/*************************************************************************
* FUNCTION
*   IMX105MIPIClose
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
UINT32 IMX105MIPIClose(void)
{
    
    return ERROR_NONE;
}	/* IMX105MIPIClose() */

void IMX105MIPISetFlipMirror(kal_int32 imgMirror)
{
    kal_uint8  iTemp; 

     SENSORDB("[IMX105MIPI]%s():mirror_flip=%d\n",__FUNCTION__,imgMirror);

	spin_lock(&imx105_drv_lock);
	IMX105MIPI_sensor.mirror_flip = imgMirror;
	spin_unlock(&imx105_drv_lock);
	  
    iTemp = IMX105MIPI_read_cmos_sensor(0x0101) & 0x03;	//Clear the mirror and flip bits.
    switch (imgMirror)
    {
        case IMAGE_NORMAL:
            IMX105MIPI_write_cmos_sensor(0x0101, 0x00);	//Set normal
            break;
        case IMAGE_V_MIRROR:
            IMX105MIPI_write_cmos_sensor(0x0101, iTemp | 0x02);	//Set flip
            break;
        case IMAGE_H_MIRROR:
            IMX105MIPI_write_cmos_sensor(0x0101, iTemp | 0x01);	//Set mirror
            break;
        case IMAGE_HV_MIRROR:
            IMX105MIPI_write_cmos_sensor(0x0101, 0x03);	//Set mirror and flip
            break;
    }
}


/*************************************************************************
* FUNCTION
*   IMX105MIPIPreview
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
UINT32 IMX105MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
   SENSORDB("[IMX105MIPI]%s()\n",__FUNCTION__);

     if(IMX105MIPI_sensor.pv_mode == KAL_FALSE)
     {
           spin_lock(&imx105_drv_lock);
		   IMX105MIPI_sensor.pv_mode =	KAL_TRUE;
		   spin_unlock(&imx105_drv_lock);
		   
		   IMX105MIPI_Set_720P_PVsize();   
	 }
	 
	 spin_lock(&imx105_drv_lock);
	 if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		 // MPEG4 Encode Mode
	 {
		 IMX105MIPI_MPEG4_encode_mode = KAL_TRUE;
	 }
	 else
	 {
		 IMX105MIPI_MPEG4_encode_mode = KAL_FALSE;
	 }
	 spin_unlock(&imx105_drv_lock);

	 

     spin_lock(&imx105_drv_lock);
     if(IMX105MIPI_sensor.fix_video_fps == KAL_TRUE)
     {
	  IMX105MIPI_sensor.pv_dummy_pixels = 0;
	  IMX105MIPI_sensor.pv_dummy_lines = 0;
	  IMX105MIPI_sensor.fix_video_fps = KAL_FALSE;
     }
	  IMX105MIPI_sensor.pv_line_length = IMX105MIPI_PV_LINE_LENGTH_PIXELS+IMX105MIPI_sensor.pv_dummy_pixels; 
	  IMX105MIPI_sensor.pv_frame_length = IMX105MIPI_PV_FRAME_LENGTH_LINES+IMX105MIPI_sensor.pv_dummy_lines;
	 spin_unlock(&imx105_drv_lock);
	  //IMX105MIPI_sensor.fix_video_fps = KAL_FALSE;

    // if(IMX105MIPI_sensor.first_init == KAL_TRUE)
	 IMX105MIPI_SetDummy(IMX105MIPI_sensor.pv_dummy_pixels,IMX105MIPI_sensor.pv_dummy_lines);

	// IMX105MIPI_sensor.first_init = KAL_FALSE;

	 IMX105MIPI_SetShutter(IMX105MIPI_sensor.pv_shutter);
	  
     memcpy(&IMX105MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
   
    return ERROR_NONE;
}	/* IMX105MIPIPreview() */

UINT32 IMX105MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
       kal_uint32 shutter = IMX105MIPI_sensor.pv_shutter;
	   
	 //  IMX105MIPI_sensor.first_init = KAL_TRUE;
 
        SENSORDB("[IMX105MIPI]%s(),pv_shtter=%d,pv_line_length=%d\n",__FUNCTION__,shutter,IMX105MIPI_sensor.pv_line_length);
	   
        IMX105MIPI_set_8M_FullSize();

		spin_lock(&imx105_drv_lock);
	    IMX105MIPI_sensor.pv_mode = KAL_FALSE;
		IMX105MIPI_sensor.cp_dummy_pixels = 0;
		IMX105MIPI_sensor.cp_dummy_lines = 0;
		IMX105MIPI_sensor.cp_line_length = IMX105MIPI_FULL_LINE_LENGTH_PIXELS + IMX105MIPI_sensor.cp_dummy_pixels;
	    IMX105MIPI_sensor.cp_frame_length = IMX105MIPI_FULL_FRAME_LENGTH_LINES + IMX105MIPI_sensor.cp_dummy_lines;
	    IMX105MIPI_MPEG4_encode_mode = KAL_FALSE; 
        IMX105MIPI_Auto_Flicker_mode = KAL_FALSE;
        spin_unlock(&imx105_drv_lock);

       #if 1//#ifdef DEBUG_CAPPRV_LUX  //pv_pclk=cp_pclk
       shutter = (shutter * IMX105MIPI_sensor.pv_line_length)/IMX105MIPI_sensor.cp_line_length;
	   SENSORDB("[IMX105MIPI]cp_shutter=%d,cp_length=%d\n",shutter,IMX105MIPI_sensor.cp_line_length); 
	   #else
	   //calculate cp shutter 
	   shutter =  (IMX105MIPI_sensor.cp_pclk/100000) * shutter/(IMX105MIPI_sensor.pv_pclk/100000);
	   shutter = (shutter * IMX105MIPI_sensor.pv_line_length)/IMX105MIPI_sensor.cp_line_length;
	   SENSORDB("[IMX105MIPI]cp_shutter=%d,cp_length=%d\n",shutter,IMX105MIPI_sensor.cp_line_length);  

	  // shutter = shutter *9858/10000;
	   #endif
		
		IMX105MIPI_SetDummy(IMX105MIPI_sensor.cp_dummy_pixels,IMX105MIPI_sensor.cp_dummy_lines);

		IMX105MIPI_write_shutter(shutter);
           
        memcpy(&IMX105MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}	/* IMX105MIPICapture() */

UINT32 IMX105MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
   #if 1
    pSensorResolution->SensorPreviewWidth = IMX105MIPI_PV_ACTIVE_PIXEL_NUMS;
    pSensorResolution->SensorPreviewHeight = IMX105MIPI_PV_ACTIVE_LINE_NUMS;
    pSensorResolution->SensorFullWidth = IMX105MIPI_FULL_ACTIVE_PIXEL_NUMS;
    pSensorResolution->SensorFullHeight= IMX105MIPI_FULL_ACTIVE_LINE_NUMS;
   #else
    pSensorResolution->SensorPreviewWidth = IMX105MIPI_PV_ACTIVE_PIXEL_NUMS - 12;//12;//12;//16;
    pSensorResolution->SensorPreviewHeight = IMX105MIPI_PV_ACTIVE_LINE_NUMS - 20;//20;//8;//12;
    pSensorResolution->SensorFullWidth = IMX105MIPI_FULL_ACTIVE_PIXEL_NUMS - 32;//40;//24;//32;
    pSensorResolution->SensorFullHeight= IMX105MIPI_FULL_ACTIVE_LINE_NUMS - 6-12-38;//16;//16;//24;
   #endif

    return ERROR_NONE;
}   /* IMX105MIPIGetResolution() */

UINT32 IMX105MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
   #if 1
   // pSensorInfo->SensorPreviewResolutionX=IMX105MIPI_PV_ACTIVE_PIXEL_NUMS;
   // pSensorInfo->SensorPreviewResolutionY=IMX105MIPI_PV_ACTIVE_LINE_NUMS;
    pSensorInfo->SensorFullResolutionX=IMX105MIPI_FULL_ACTIVE_PIXEL_NUMS;
    pSensorInfo->SensorFullResolutionY=IMX105MIPI_FULL_ACTIVE_LINE_NUMS;
   #else
    pSensorInfo->SensorPreviewResolutionX=IMX105MIPI_PV_ACTIVE_PIXEL_NUMS - 12;//12;//40;//12;//16;
    pSensorInfo->SensorPreviewResolutionY=IMX105MIPI_PV_ACTIVE_LINE_NUMS - 20;//20;//8;//12;
    pSensorInfo->SensorFullResolutionX=IMX105MIPI_FULL_ACTIVE_PIXEL_NUMS - 32;//40//80;//24;//32;
    pSensorInfo->SensorFullResolutionY=IMX105MIPI_FULL_ACTIVE_LINE_NUMS - 6-12-38;//32;//64;//16;//
   #endif

  //  pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=13;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_Gb;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;
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

    pSensorInfo->CaptureDelayFrame = 5;//2; 
    pSensorInfo->PreviewDelayFrame = 5;//2; 
    pSensorInfo->VideoDelayFrame = 5; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;      
    pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 1;	

	switch (ScenarioId)
	{
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 pSensorInfo->SensorPreviewResolutionX=IMX105MIPI_FULL_ACTIVE_PIXEL_NUMS;
             pSensorInfo->SensorPreviewResolutionY=IMX105MIPI_FULL_ACTIVE_LINE_NUMS;
			 pSensorInfo->SensorCameraPreviewFrameRate=15;
			 break;
			 
		default:
			 pSensorInfo->SensorPreviewResolutionX=IMX105MIPI_PV_ACTIVE_PIXEL_NUMS;
             pSensorInfo->SensorPreviewResolutionY=IMX105MIPI_PV_ACTIVE_LINE_NUMS;
			 pSensorInfo->SensorCameraPreviewFrameRate=30;
			 break;	
	}

	   
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX105MIPI_PV_GRAB_X; 
            pSensorInfo->SensorGrabStartY = IMX105MIPI_PV_GRAB_Y;           		
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	     pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	     pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
//            pSensorInfo->SensorGrabStartX = IMX105MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartX = IMX105MIPI_FULL_GRAB_X; 
            pSensorInfo->SensorGrabStartY = IMX105MIPI_FULL_GRAB_Y;          			
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX105MIPI_PV_GRAB_X; 
            pSensorInfo->SensorGrabStartY = IMX105MIPI_PV_GRAB_Y;             
            break;
    }

	spin_lock(&imx105_drv_lock);
    IMX105MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
	spin_unlock(&imx105_drv_lock);
	
    memcpy(pSensorConfigData, &IMX105MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* IMX105MIPIGetInfo() */


UINT32 IMX105MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    spin_lock(&imx105_drv_lock);
    CurrentScenarioId = ScenarioId;
	spin_unlock(&imx105_drv_lock);
	
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            IMX105MIPIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            IMX105MIPICapture(pImageWindow, pSensorConfigData);
            break;
        default:
            return ERROR_INVALID_SCENARIO_ID;
    }
    return TRUE;
} /* IMX105MIPIControl() */

//TO DO :need to write fix frame rate func
static void IMX105MIPI_Fix_Video_Frame_Rate(kal_uint16 framerate)
{    
	kal_uint16 IMX105MIPI_Video_Max_Line_Length = 0;
	kal_uint16 IMX105MIPI_Video_Max_Expourse_Time = 0;
  
	
    SENSORDB("[IMX105MIPI]%s():fix_frame_rate=%d\n",__FUNCTION__,framerate);
	
    framerate = framerate * 10;
	
	spin_lock(&imx105_drv_lock);
	IMX105MIPI_sensor.fix_video_fps = KAL_TRUE;
	spin_unlock(&imx105_drv_lock);
	
	SENSORDB("[IMX105MIPI][Enter Fix_fps func] IMX105MIPI_Fix_Video_Frame_Rate = %d\n", framerate/10);


	IMX105MIPI_Video_Max_Expourse_Time = (kal_uint16)((IMX105MIPI_sensor.pv_pclk*10/framerate)/IMX105MIPI_sensor.pv_line_length);
	
    if (IMX105MIPI_Video_Max_Expourse_Time > IMX105MIPI_PV_FRAME_LENGTH_LINES/*IMX105MIPI_sensor.pv_frame_length*/)	
    	{
    	    spin_lock(&imx105_drv_lock);
	    	IMX105MIPI_sensor.pv_frame_length = IMX105MIPI_Video_Max_Expourse_Time;
			IMX105MIPI_sensor.pv_dummy_lines = IMX105MIPI_sensor.pv_frame_length-IMX105MIPI_PV_FRAME_LENGTH_LINES;
            spin_unlock(&imx105_drv_lock);
			
			SENSORDB("[IMX105MIPI]%s():frame_length=%d,dummy_lines=%d\n",__FUNCTION__,IMX105MIPI_sensor.pv_frame_length,IMX105MIPI_sensor.pv_dummy_lines);
			IMX105MIPI_SetDummy(IMX105MIPI_sensor.pv_dummy_pixels,IMX105MIPI_sensor.pv_dummy_lines);
    	}
    
}

UINT32 IMX105MIPISetVideoMode(UINT16 u2FrameRate)
{
	SENSORDB("[IMX105MIPI]%s():fix_frame_rate=%d\n",__FUNCTION__,u2FrameRate);

    spin_lock(&imx105_drv_lock);
	IMX105MIPI_MPEG4_encode_mode = KAL_TRUE;
	IMX105MIPI_sensor.video_current_frame_rate = u2FrameRate;
	spin_unlock(&imx105_drv_lock);
	
    if(u2FrameRate<=30)

	  IMX105MIPI_Fix_Video_Frame_Rate(u2FrameRate);

    else 
      SENSORDB("[IMX105MIPI][Error]Wrong frame rate setting %d\n", u2FrameRate);
	

    return TRUE;
}

UINT32 IMX105MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++=IMX105MIPI_FULL_ACTIVE_PIXEL_NUMS - IMX105MIPI_FULL_GRAB_X;
            *pFeatureReturnPara16=IMX105MIPI_FULL_ACTIVE_LINE_NUMS - IMX105MIPI_FULL_GRAB_Y;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
			switch(CurrentScenarioId)
			{
              case MSDK_SCENARIO_ID_CAMERA_ZSD:
			  	*pFeatureReturnPara16++=IMX105MIPI_sensor.cp_line_length;
                *pFeatureReturnPara16=IMX105MIPI_sensor.cp_frame_length;
                //SENSORDB("Sensor period:%d %d\n", IMX105MIPI_FULL_LINE_LENGTH_PIXELS+IMX105MIPI_dummy_pixels, IMX105MIPI_PV_PERIOD_LINE_NUMS+IMX105MIPI_dummy_lines); 
                *pFeatureParaLen=4;
				break;
				
			  default:
			  	*pFeatureReturnPara16++=IMX105MIPI_sensor.pv_line_length;
                *pFeatureReturnPara16=IMX105MIPI_sensor.pv_frame_length;
               // SENSORDB("Sensor period:%d %d\n", IMX105MIPI_PV_PERIOD_PIXEL_NUMS+IMX105MIPI_dummy_pixels, IMX105MIPI_PV_PERIOD_LINE_NUMS+IMX105MIPI_dummy_lines); 
                *pFeatureParaLen=4;
				break;
			}
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			switch(CurrentScenarioId)
			{
              case MSDK_SCENARIO_ID_CAMERA_ZSD:
			  	 *pFeatureReturnPara32 = IMX105MIPI_sensor.cp_pclk;
                 *pFeatureParaLen=4;
			  	 break;
				
			  default:
			  	 *pFeatureReturnPara32 = IMX105MIPI_sensor.pv_pclk;
                 *pFeatureParaLen=4;
				 break;
			}
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            IMX105MIPI_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            IMX105MIPI_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            IMX105MIPI_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			spin_lock(&imx105_drv_lock);
            IMX105MIPI_isp_master_clock=*pFeatureData32;
			spin_unlock(&imx105_drv_lock);
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            IMX105MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = IMX105MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
			spin_lock(&imx105_drv_lock);
            for (i=0;i<SensorRegNumber;i++)
            {
                IMX105MIPISensorCCT[i].Addr=*pFeatureData32++;
                IMX105MIPISensorCCT[i].Para=*pFeatureData32++;
            }
			spin_unlock(&imx105_drv_lock);
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX105MIPISensorCCT[i].Addr;
                *pFeatureData32++=IMX105MIPISensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
			spin_lock(&imx105_drv_lock);
            for (i=0;i<SensorRegNumber;i++)
            {
                IMX105MIPISensorReg[i].Addr=*pFeatureData32++;
                IMX105MIPISensorReg[i].Para=*pFeatureData32++;
            }
			spin_unlock(&imx105_drv_lock);
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX105MIPISensorReg[i].Addr;
                *pFeatureData32++=IMX105MIPISensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=IMX105MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, IMX105MIPISensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, IMX105MIPISensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &IMX105MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            IMX105MIPI_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            IMX105MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=IMX105MIPI_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            IMX105MIPI_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            IMX105MIPI_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            IMX105MIPI_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 283;
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

        case SENSOR_FEATURE_INITIALIZE_AF:
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            IMX105MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            IMX105MIPIGetSensorID(pFeatureReturnPara32); 
            break;            
		case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
			IMX105MIPISetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));
			break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* IMX105MIPIFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncIMX105MIPI=
{
    IMX105MIPIOpen,
    IMX105MIPIGetInfo,
    IMX105MIPIGetResolution,
    IMX105MIPIFeatureControl,
    IMX105MIPIControl,
    IMX105MIPIClose
};

UINT32 IMX105_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncIMX105MIPI;

    return ERROR_NONE;
}   /* SensorInit() */

