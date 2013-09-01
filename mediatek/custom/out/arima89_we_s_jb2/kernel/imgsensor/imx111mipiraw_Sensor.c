/*****************************************************************************

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

#include "imx111mipiraw_Sensor.h"
#include "imx111mipiraw_Camera_Sensor_para.h"
#include "imx111mipiraw_CameraCustomized.h"

kal_bool  IMX111MIPI_MPEG4_encode_mode = KAL_FALSE;
kal_bool IMX111MIPI_Auto_Flicker_mode = KAL_FALSE;


kal_uint8 IMX111MIPI_sensor_write_I2C_address = IMX111MIPI_WRITE_ID;
kal_uint8 IMX111MIPI_sensor_read_I2C_address = IMX111MIPI_READ_ID;


	

static struct IMX111MIPI_sensor_STRUCT IMX111MIPI_sensor={IMX111MIPI_WRITE_ID,IMX111MIPI_READ_ID,KAL_TRUE,KAL_FALSE,KAL_TRUE,KAL_FALSE,
KAL_FALSE,KAL_FALSE,KAL_FALSE,134400000,200000000,134400000,800,0,0,64,64,64,3536,1254,3500,1884,3536,2502,0,0,0,0,0,0,30};
MSDK_SCENARIO_ID_ENUM CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;


kal_uint16  IMX111MIPI_sensor_gain_base=0x0;
/* MAX/MIN Explosure Lines Used By AE Algorithm */
kal_uint16 IMX111MIPI_MAX_EXPOSURE_LINES = IMX111MIPI_PV_FRAME_LENGTH_LINES-5;//;
kal_uint32 IMX111MIPI_isp_master_clock;
kal_uint16 IMX111MIPI_CURRENT_FRAME_LINES = IMX111MIPI_PV_PERIOD_LINE_NUMS;//;
static DEFINE_SPINLOCK(imx111_drv_lock);

#define SENSORDB(fmt, arg...) printk( "[IMX111MIPIRaw] "  fmt, ##arg)
#define RETAILMSG(x,...)
#define TEXT
UINT8 IMX111MIPIPixelClockDivider=0;
kal_uint16 IMX111MIPI_sensor_id=0;
MSDK_SENSOR_CONFIG_STRUCT IMX111MIPISensorConfigData;
kal_uint32 IMX111MIPI_FAC_SENSOR_REG;
kal_uint16 IMX111MIPI_sensor_flip_value; 
#define IMX111MIPI_MaxGainIndex 72		
// Gain Indexs
kal_uint16 IMX111MIPI_sensorGainMapping[IMX111MIPI_MaxGainIndex][2] = {
	{64 ,1	},
	{67 ,13 },
	{70 ,24 },
	{74 ,34 },
	{77 ,43 },
	{80 ,52 },
	{83 ,59 },
	{87 ,67 },
	{90 ,73 },
	{93 ,80 },
	{96 ,85 },
	{99 ,91 },
	{102,96 },
	{106,101},
	{109,105},
	{112,110},
	{115,114},
	{119,118},
	{122,121},
	{125,125},
	{128,128},
	{131,131},
	{134,134},
	{138,137},
	{141,140},
	{143,141},
	{144,142},
	{148,145},
	{150,147},
	{153,149},
	{157,152},
	{159,153},
	{164,156},
	{167,158},
	{171,160},
	{174,162},
	{178,164},
	{182,166},
	{186,168},
	{191,170},
	{195,172},
	{200,174},
	{205,176},
	{210,178}, 
	{216,180}, 
	{221,182}, 
	{228,184}, 
	{234,186}, 
	{241,188}, 
	{248,190}, 
	{256,192}, 
	{264,194},
	{273,196},
	{282,198},
	{292,200},
	{303,202},
	{328,206},
	{341,208},
	{356,210},
	{372,212},
	{381,213},
	{390,214},
	{399,215},
	{410,216},
	{420,217},
	{431,218},
	{443,219},
	{455,220},
	{468,221},
	{482,222},
	{497,223},
	{512,224},
	{512,224},
	{546,226},
	{585,228},
	{630,230},
	{683,232},
	{712,233},
	{745,234},
	{780,235},
	{819,236},
	{862,237},
	{910,238},
	{964,239},
	{1024,240}
		
};
/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT IMX111MIPISensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT IMX111MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define IMX111MIPI_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, IMX111MIPI_WRITE_ID)

kal_uint16 IMX111MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,IMX111MIPI_WRITE_ID);
    return get_byte;
}

void IMX111MIPI_write_shutter(kal_uint16 shutter)
{
    
	kal_uint32 frame_length = 0,line_length=0,shutter1=0;
    kal_uint32 extra_lines = 0;
	kal_uint32 max_exp_shutter = 0;
	unsigned long flags;
	
	SENSORDB("shutter&gain test by hhl: write shutter IMX111MIPI_write_shutter\n"); 
	SENSORDB("[IMX111MIPI]%s()\n",__FUNCTION__);
    if (IMX111MIPI_sensor.pv_mode == KAL_TRUE) 
	 {
	   max_exp_shutter = IMX111MIPI_PV_FRAME_LENGTH_LINES + IMX111MIPI_sensor.pv_dummy_lines-5;
     }
     else if (IMX111MIPI_sensor.video_mode== KAL_TRUE) 
     {
       max_exp_shutter = IMX111MIPI_VIDEO_FRAME_LENGTH_LINES + IMX111MIPI_sensor.video_dummy_lines-5;
	 }	 
     else if (IMX111MIPI_sensor.capture_mode== KAL_TRUE) 
     {
       max_exp_shutter = IMX111MIPI_FULL_FRAME_LENGTH_LINES + IMX111MIPI_sensor.cp_dummy_lines-5;
	 }	 
	 else
	 	{
	 	
		SENSORDB("sensor mode error\n");
	 	}
	 
	 if(shutter > max_exp_shutter)
	   extra_lines = shutter - max_exp_shutter;
	 else 
	   extra_lines = 0;
	 if (IMX111MIPI_sensor.pv_mode == KAL_TRUE) 
	 {
       frame_length =IMX111MIPI_PV_FRAME_LENGTH_LINES+ IMX111MIPI_sensor.pv_dummy_lines + extra_lines;
	   line_length = IMX111MIPI_PV_LINE_LENGTH_PIXELS+ IMX111MIPI_sensor.pv_dummy_pixels;
	   spin_lock_irqsave(&imx111_drv_lock,flags);
	   IMX111MIPI_sensor.pv_line_length = line_length;
	   IMX111MIPI_sensor.pv_frame_length = frame_length;
	   spin_unlock_irqrestore(&imx111_drv_lock,flags);
	 }
	 else if (IMX111MIPI_sensor.video_mode== KAL_TRUE) 
     {
	    frame_length = IMX111MIPI_VIDEO_FRAME_LENGTH_LINES+ IMX111MIPI_sensor.video_dummy_lines + extra_lines;
		line_length =IMX111MIPI_VIDEO_LINE_LENGTH_PIXELS + IMX111MIPI_sensor.video_dummy_pixels;
		spin_lock_irqsave(&imx111_drv_lock,flags);
		IMX111MIPI_sensor.video_line_length = line_length;
	    IMX111MIPI_sensor.video_frame_length = frame_length;
		spin_unlock_irqrestore(&imx111_drv_lock,flags);
	 } 
	 else if(IMX111MIPI_sensor.capture_mode== KAL_TRUE)
	 	{
	    frame_length = IMX111MIPI_FULL_FRAME_LENGTH_LINES+ IMX111MIPI_sensor.cp_dummy_lines + extra_lines;
		line_length =IMX111MIPI_FULL_LINE_LENGTH_PIXELS + IMX111MIPI_sensor.cp_dummy_pixels;
		spin_lock_irqsave(&imx111_drv_lock,flags);
		IMX111MIPI_sensor.cp_line_length = line_length;
	    IMX111MIPI_sensor.cp_frame_length = frame_length;
		spin_unlock_irqrestore(&imx111_drv_lock,flags);
	 }
	 else
	 	{
	 	
		SENSORDB("sensor mode error\n");
	 	}
   // SENSORDB("[IMX111MIPI]Write_shutter:pv_mode =%d,video_mode=%d,capture mode= %d\n",IMX111MIPI_sensor.pv_mode,IMX111MIPI_sensor.video_mode,IMX111MIPI_sensor.capture_mode);
    SENSORDB("[IMX111MIPI]Write_shutter:shutter =%d,frame_length=%d,linelength=%d\n",shutter,frame_length,line_length);
	
	//IMX111MIPI_write_cmos_sensor(0x0100,0x00);// STREAM STop
	IMX111MIPI_write_cmos_sensor(0x0104, 1);        
	IMX111MIPI_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
    IMX111MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);	
    
    IMX111MIPI_write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
    IMX111MIPI_write_cmos_sensor(0x0203, shutter  & 0xFF);
    IMX111MIPI_write_cmos_sensor(0x0104, 0);    
	
	//IMX111MIPI_write_cmos_sensor(0x0100,0x01);// STREAM Start
   // frame_length=IMX111MIPI_read_cmos_sensor(0x0340)<<8 | IMX111MIPI_read_cmos_sensor(0x0341);
	//shutter1=IMX111MIPI_read_cmos_sensor(0x0202)<<8 | IMX111MIPI_read_cmos_sensor(0x0203);
	//line_length=IMX111MIPI_read_cmos_sensor(0x0342)<<8 | IMX111MIPI_read_cmos_sensor(0x0343);
  //  SENSORDB("[IMX111MIPI]:register shutter =%d,register frame_length=%d,register linelength=%d\n",shutter1,frame_length,line_length);


}   /* write_IMX111MIPI_shutter */

static kal_uint16 IMX111MIPIReg2Gain(const kal_uint8 iReg)
{

    kal_uint8 iI;
    // Range: 1x to 16x
    for (iI = 0; iI < IMX111MIPI_MaxGainIndex; iI++) {
        if(iReg <= IMX111MIPI_sensorGainMapping[iI][1]){
            break;
        }
    }
    return IMX111MIPI_sensorGainMapping[iI][0];

}
static kal_uint8 IMX111MIPIGain2Reg(const kal_uint16 iGain)
{

	kal_uint8 iI;
    
    for (iI = 0; iI < (IMX111MIPI_MaxGainIndex-1); iI++) {
        if(iGain <= IMX111MIPI_sensorGainMapping[iI][0]){    
            break;
        }
    }
    if(iGain != IMX111MIPI_sensorGainMapping[iI][0])
    {
         printk("[IMX111MIPIGain2Reg] Gain mapping don't correctly:%d %d \n", iGain, IMX111MIPI_sensorGainMapping[iI][0]);
    }
    return IMX111MIPI_sensorGainMapping[iI][1];
	/*return NONE;

kal_uint16 Gain;
	Gain=(256*iGain-(256*64+iGain*1/2))/iGain;
    return Gain;
	*/	
}

/*************************************************************************
* FUNCTION
*    IMX111MIPI_SetGain
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
void IMX111MIPI_SetGain(UINT16 iGain)
{
    kal_uint8 iReg;
	printk("[IMX111MIPI_SetGain] SetGain:%d\n",  iGain);
    iReg = IMX111MIPIGain2Reg(iGain);
	printk("[IMX111MIPI_SetGain ] RegisterGain:%d\n", iReg);
	
	SENSORDB("shutter&gain test by hhl: set gain IMX111MIPI_SetGain\n"); 
	IMX111MIPI_write_cmos_sensor(0x0104, 1);
    IMX111MIPI_write_cmos_sensor(0x0205, (kal_uint8)iReg);
    IMX111MIPI_write_cmos_sensor(0x0104, 0);
    iReg=IMX111MIPI_read_cmos_sensor(0x205);
	printk("[IMX111MIPI_SetGain ] RegisterGain:%d\n", iReg);
}   /*  IMX111MIPI_SetGain_SetGain  */


/*************************************************************************
* FUNCTION
*    read_IMX111MIPI_gain
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
kal_uint16 read_IMX111MIPI_gain(void)
{
    return (kal_uint16)((IMX111MIPI_read_cmos_sensor(0x0204)<<8) | IMX111MIPI_read_cmos_sensor(0x0205)) ;
}  /* read_IMX111MIPI_gain */

void IMX111MIPI_camera_para_to_sensor(void)
{

	kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=IMX111MIPISensorReg[i].Addr; i++)
    {
        IMX111MIPI_write_cmos_sensor(IMX111MIPISensorReg[i].Addr, IMX111MIPISensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX111MIPISensorReg[i].Addr; i++)
    {
        IMX111MIPI_write_cmos_sensor(IMX111MIPISensorReg[i].Addr, IMX111MIPISensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        IMX111MIPI_write_cmos_sensor(IMX111MIPISensorCCT[i].Addr, IMX111MIPISensorCCT[i].Para);
    }

}


/*************************************************************************
* FUNCTION
*    IMX111MIPI_sensor_to_camera_para
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
void IMX111MIPI_sensor_to_camera_para(void)
{

	kal_uint32    i,temp_data;
    for(i=0; 0xFFFFFFFF!=IMX111MIPISensorReg[i].Addr; i++)
    {
		temp_data=IMX111MIPI_read_cmos_sensor(IMX111MIPISensorReg[i].Addr);
		spin_lock(&imx111_drv_lock);
		IMX111MIPISensorReg[i].Para = temp_data;
		spin_unlock(&imx111_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX111MIPISensorReg[i].Addr; i++)
    {
    	temp_data=IMX111MIPI_read_cmos_sensor(IMX111MIPISensorReg[i].Addr);
         spin_lock(&imx111_drv_lock);
        IMX111MIPISensorReg[i].Para = temp_data;
		spin_unlock(&imx111_drv_lock);
    }

}

/*************************************************************************
* FUNCTION
*    IMX111MIPI_get_sensor_group_count
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
kal_int32  IMX111MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void IMX111MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void IMX111MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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
           	spin_lock(&imx111_drv_lock);    
            temp_para=IMX111MIPISensorCCT[temp_addr].Para;	
			spin_unlock(&imx111_drv_lock);
            temp_gain = IMX111MIPIReg2Gain(temp_para);
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
                
                    //temp_reg=IMX111MIPISensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=IMX111MIPI_MAX_EXPOSURE_LINES;
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

//void IMX111MIPI_set_isp_driving_current(kal_uint8 current)
//{

//}

kal_bool IMX111MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
   kal_uint16 temp_addr=0, temp_para=0;

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
            temp_para = IMX111MIPIGain2Reg(ItemValue);
            spin_lock(&imx111_drv_lock);    
            IMX111MIPISensorCCT[temp_addr].Para = temp_para;
			spin_unlock(&imx111_drv_lock);
            IMX111MIPI_write_cmos_sensor(IMX111MIPISensorCCT[temp_addr].Addr,temp_para);
			temp_para=read_IMX111MIPI_gain();	
            spin_lock(&imx111_drv_lock);    
            IMX111MIPI_sensor_gain_base=temp_para;
			spin_unlock(&imx111_drv_lock);

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    if(ItemValue==2)
                    {			
                    spin_lock(&imx111_drv_lock);    
                        IMX111MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
					spin_unlock(&imx111_drv_lock);
                        //IMX111MIPI_set_isp_driving_current(ISP_DRIVING_2MA);
                    }
                    else if(ItemValue==3 || ItemValue==4)
                    {
                    	spin_lock(&imx111_drv_lock);    
                        IMX111MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
						spin_unlock(&imx111_drv_lock);
                        //IMX111MIPI_set_isp_driving_current(ISP_DRIVING_4MA);
                    }
                    else if(ItemValue==5 || ItemValue==6)
                    {
                    	spin_lock(&imx111_drv_lock);    
                        IMX111MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
						spin_unlock(&imx111_drv_lock);
                        //IMX111MIPI_set_isp_driving_current(ISP_DRIVING_6MA);
                    }
                    else
                    {
                    	spin_lock(&imx111_drv_lock);    
                        IMX111MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
						spin_unlock(&imx111_drv_lock);
                        //IMX111MIPI_set_isp_driving_current(ISP_DRIVING_8MA);
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
					spin_lock(&imx111_drv_lock);    
                    IMX111MIPI_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&imx111_drv_lock);
                    break;
                case 1:
                    IMX111MIPI_write_cmos_sensor(IMX111MIPI_FAC_SENSOR_REG,ItemValue);
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

static void IMX111MIPI_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
  kal_uint32 frame_length = 0, line_length = 0;
   if(IMX111MIPI_sensor.pv_mode == KAL_TRUE)
   	{
   	 spin_lock(&imx111_drv_lock);    
   	 IMX111MIPI_sensor.pv_dummy_pixels = iPixels;
	 IMX111MIPI_sensor.pv_dummy_lines = iLines;
   	 IMX111MIPI_sensor.pv_line_length = IMX111MIPI_PV_LINE_LENGTH_PIXELS + iPixels;
	 IMX111MIPI_sensor.pv_frame_length = IMX111MIPI_PV_FRAME_LENGTH_LINES + iLines;
	 spin_unlock(&imx111_drv_lock);
	 line_length = IMX111MIPI_sensor.pv_line_length;
	 frame_length = IMX111MIPI_sensor.pv_frame_length; 	
   	}
   else if(IMX111MIPI_sensor.video_mode== KAL_TRUE)
   	{
   	 spin_lock(&imx111_drv_lock);    
   	 IMX111MIPI_sensor.video_dummy_pixels = iPixels;
	 IMX111MIPI_sensor.video_dummy_lines = iLines;
   	 IMX111MIPI_sensor.video_line_length = IMX111MIPI_VIDEO_LINE_LENGTH_PIXELS + iPixels;
	 IMX111MIPI_sensor.video_frame_length = IMX111MIPI_VIDEO_FRAME_LENGTH_LINES + iLines;
	 spin_unlock(&imx111_drv_lock);
	 line_length = IMX111MIPI_sensor.video_line_length;
	 frame_length = IMX111MIPI_sensor.video_frame_length;
   	}
	else if(IMX111MIPI_sensor.capture_mode== KAL_TRUE) 
		{
	  spin_lock(&imx111_drv_lock);	
   	  IMX111MIPI_sensor.cp_dummy_pixels = iPixels;
	  IMX111MIPI_sensor.cp_dummy_lines = iLines;
	  IMX111MIPI_sensor.cp_line_length = IMX111MIPI_FULL_LINE_LENGTH_PIXELS + iPixels;
	  IMX111MIPI_sensor.cp_frame_length = IMX111MIPI_FULL_FRAME_LENGTH_LINES + iLines;
	   spin_unlock(&imx111_drv_lock);
	  line_length = IMX111MIPI_sensor.cp_line_length;
	  frame_length = IMX111MIPI_sensor.cp_frame_length;
    }
	else
		{
		 SENSORDB("[IMX111MIPI]%s(),sensor mode error",__FUNCTION__);
		}

      IMX111MIPI_write_cmos_sensor(0x0104, 1);        
      IMX111MIPI_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
      IMX111MIPI_write_cmos_sensor(0x0341, frame_length & 0xFF);	
      IMX111MIPI_write_cmos_sensor(0x0342, (line_length >>8) & 0xFF);
      IMX111MIPI_write_cmos_sensor(0x0343, line_length & 0xFF);
      IMX111MIPI_write_cmos_sensor(0x0104, 0);

	  SENSORDB("[IMX111MIPI]%s(),dumy_pixel=%d,dumy_line=%d,\n",__FUNCTION__,iPixels,iLines);
	  SENSORDB("[IMX111MIPI]pv_mode=%d,video_mode=%d,cp_mode=%d,line_length=%d,frame_length=%d,\n",IMX111MIPI_sensor.pv_mode,IMX111MIPI_sensor.video_mode,IMX111MIPI_sensor.capture_mode,line_length,frame_length);
	  SENSORDB("[IMX111MIPI]0x340=%x,0x341=%x\n",IMX111MIPI_read_cmos_sensor(0x0340),IMX111MIPI_read_cmos_sensor(0x0341));
	  SENSORDB("[IMX111MIPI]0x342=%x,0x343=%x\n",IMX111MIPI_read_cmos_sensor(0x0342),IMX111MIPI_read_cmos_sensor(0x0343));
  
}   /*  IMX111MIPI_SetDummy */

static void IMX111MIPI_Sensor_Init(void)
{
    
	IMX111MIPI_write_cmos_sensor(0x0101, 0x00);// Daniel 20130419
	IMX111MIPI_write_cmos_sensor(0x30C6, 0x00);//
	IMX111MIPI_write_cmos_sensor(0x30C7, 0x00);//
	IMX111MIPI_write_cmos_sensor(0x3080, 0x50);//
	IMX111MIPI_write_cmos_sensor(0x3087, 0x53);//
	IMX111MIPI_write_cmos_sensor(0x309D, 0x94);//
	IMX111MIPI_write_cmos_sensor(0x30B1, 0x03);//
	IMX111MIPI_write_cmos_sensor(0x30C6, 0x00);//
	IMX111MIPI_write_cmos_sensor(0x30C7, 0x00);//
	IMX111MIPI_write_cmos_sensor(0x3115, 0x0B);//
	IMX111MIPI_write_cmos_sensor(0x3118, 0x30);//
	IMX111MIPI_write_cmos_sensor(0x311D, 0x25);//
	IMX111MIPI_write_cmos_sensor(0x3121, 0x0A);//
	IMX111MIPI_write_cmos_sensor(0x3212, 0xF2);//
	IMX111MIPI_write_cmos_sensor(0x3213, 0x0F);//
	IMX111MIPI_write_cmos_sensor(0x3215, 0x0F);//
	IMX111MIPI_write_cmos_sensor(0x3217, 0x0B);//
	IMX111MIPI_write_cmos_sensor(0x3219, 0x0B);//
	IMX111MIPI_write_cmos_sensor(0x321B, 0x0D);//
	IMX111MIPI_write_cmos_sensor(0x321D, 0x0D);//
	IMX111MIPI_write_cmos_sensor(0x32AA, 0x11);//
	IMX111MIPI_write_cmos_sensor(0x3032, 0x40);//black level setting  
    // The register only need to enable 1 time.    
    spin_lock(&imx111_drv_lock);  
    IMX111MIPI_Auto_Flicker_mode = KAL_FALSE;     // reset the flicker status    
	spin_unlock(&imx111_drv_lock);
    printk("[IMX111MIPIRaw] Init Success \n");
}   /*  IMX111MIPI_Sensor_Init  */
void VideoFullSizeSetting(void)//16:9   6M

{	
#if 1  //1080p  30fps mipi clock  600Mhz  H 1.67 bining V 1.67 Bining 
		IMX111MIPI_write_cmos_sensor(0x0104,1);  
		IMX111MIPI_write_cmos_sensor(0x0100,0x00);// STREAM STop
		IMX111MIPI_write_cmos_sensor(0x0305,0x02);
		IMX111MIPI_write_cmos_sensor(0x0307,0x32);
		IMX111MIPI_write_cmos_sensor(0x30A4,0x02);
		IMX111MIPI_write_cmos_sensor(0x303C,0x4B);
//<2013/06/21-26217-alberthsiao,fix ANR issue for FIFA test
		mdelay(120);
//>2013/06/21-26217-alberthsiao
		IMX111MIPI_write_cmos_sensor(0x0340,0x07);
		IMX111MIPI_write_cmos_sensor(0x0341,0x5C);
		IMX111MIPI_write_cmos_sensor(0x0342,0x0D);
		IMX111MIPI_write_cmos_sensor(0x0343,0xAC);
		IMX111MIPI_write_cmos_sensor(0x0344,0x00);
		IMX111MIPI_write_cmos_sensor(0x0345,0x16);
		IMX111MIPI_write_cmos_sensor(0x0346,0x01);
		IMX111MIPI_write_cmos_sensor(0x0347,0x6E);
		IMX111MIPI_write_cmos_sensor(0x0348,0x0C);
		IMX111MIPI_write_cmos_sensor(0x0349,0xCB);
		IMX111MIPI_write_cmos_sensor(0x034A,0x08);
		IMX111MIPI_write_cmos_sensor(0x034B,0x93);
		IMX111MIPI_write_cmos_sensor(0x034C,0x07);
		IMX111MIPI_write_cmos_sensor(0x034D,0xA0);
		IMX111MIPI_write_cmos_sensor(0x034E,0x04);
		IMX111MIPI_write_cmos_sensor(0x034F,0x4A);
		IMX111MIPI_write_cmos_sensor(0x0381,0x01);
		IMX111MIPI_write_cmos_sensor(0x0383,0x01);
		IMX111MIPI_write_cmos_sensor(0x0385,0x01);
		IMX111MIPI_write_cmos_sensor(0x0387,0x01);
		IMX111MIPI_write_cmos_sensor(0x3033,0x00);
		IMX111MIPI_write_cmos_sensor(0x303D,0x10);
		IMX111MIPI_write_cmos_sensor(0x303E,0x00);
		IMX111MIPI_write_cmos_sensor(0x3040,0x08);
		IMX111MIPI_write_cmos_sensor(0x3041,0x91);
		IMX111MIPI_write_cmos_sensor(0x3048,0x00);
		IMX111MIPI_write_cmos_sensor(0x304C,0x67);
		IMX111MIPI_write_cmos_sensor(0x304D,0x03);
		IMX111MIPI_write_cmos_sensor(0x3064,0x10);
		IMX111MIPI_write_cmos_sensor(0x3073,0xA0);
		IMX111MIPI_write_cmos_sensor(0x3074,0x12);
		IMX111MIPI_write_cmos_sensor(0x3075,0x12);
		IMX111MIPI_write_cmos_sensor(0x3076,0x12);
		IMX111MIPI_write_cmos_sensor(0x3077,0x11);
		IMX111MIPI_write_cmos_sensor(0x3079,0x0A);
		IMX111MIPI_write_cmos_sensor(0x307A,0x0A);
		IMX111MIPI_write_cmos_sensor(0x309B,0x60);
		IMX111MIPI_write_cmos_sensor(0x309C,0x13);
		IMX111MIPI_write_cmos_sensor(0x309E,0x04);
		IMX111MIPI_write_cmos_sensor(0x30A0,0x15);
		IMX111MIPI_write_cmos_sensor(0x30A1,0x08);
		IMX111MIPI_write_cmos_sensor(0x30AA,0x03);
		IMX111MIPI_write_cmos_sensor(0x30B2,0x05);
		IMX111MIPI_write_cmos_sensor(0x30D5,0x20);
		IMX111MIPI_write_cmos_sensor(0x30D6,0x85);
		IMX111MIPI_write_cmos_sensor(0x30D7,0x2A);
		IMX111MIPI_write_cmos_sensor(0x30D8,0x64);
		IMX111MIPI_write_cmos_sensor(0x30D9,0x89);
		IMX111MIPI_write_cmos_sensor(0x30DE,0x00);
		IMX111MIPI_write_cmos_sensor(0x30DF,0x21);
		IMX111MIPI_write_cmos_sensor(0x3102,0x08);
		IMX111MIPI_write_cmos_sensor(0x3103,0x1D);
		IMX111MIPI_write_cmos_sensor(0x3104,0x1E);
		IMX111MIPI_write_cmos_sensor(0x3105,0x00);
		IMX111MIPI_write_cmos_sensor(0x3106,0x74);
		IMX111MIPI_write_cmos_sensor(0x3107,0x00);
		IMX111MIPI_write_cmos_sensor(0x3108,0x03);
		IMX111MIPI_write_cmos_sensor(0x3109,0x02);
		IMX111MIPI_write_cmos_sensor(0x310A,0x03);
		IMX111MIPI_write_cmos_sensor(0x315C,0x37);
		IMX111MIPI_write_cmos_sensor(0x315D,0x36);
		IMX111MIPI_write_cmos_sensor(0x316E,0x38);
		IMX111MIPI_write_cmos_sensor(0x316F,0x37);
		IMX111MIPI_write_cmos_sensor(0x3318,0x63);
		IMX111MIPI_write_cmos_sensor(0x3348,0xA0);
		IMX111MIPI_write_cmos_sensor(0x0104,0);  
		IMX111MIPI_write_cmos_sensor(0x0100,0x01);// STREAM START

#endif
#if 0   // 2 lane   16:9 6M mipi 996Mhz 
  //PLL setting 
		IMX111MIPI_write_cmos_sensor(0x0104,1);  
		IMX111MIPI_write_cmos_sensor(0x0100,0x00);// STREAM STop
		IMX111MIPI_write_cmos_sensor(0x0305,0x02);// 
		IMX111MIPI_write_cmos_sensor(0x0307,0x53);// 
		IMX111MIPI_write_cmos_sensor(0x30A4,0x02);// 
		IMX111MIPI_write_cmos_sensor(0x303C,0x4B);
     //size setting
      mdelay(20);
      IMX111MIPI_write_cmos_sensor(0x0340,0x07);// 
	  IMX111MIPI_write_cmos_sensor(0x0341,0x56);// 
	  IMX111MIPI_write_cmos_sensor(0x0342,0x0D);// 
	  IMX111MIPI_write_cmos_sensor(0x0343,0xD0);// 
	  IMX111MIPI_write_cmos_sensor(0x0344,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x0345,0x08);// 
	  IMX111MIPI_write_cmos_sensor(0x0346,0x01);// 
	  IMX111MIPI_write_cmos_sensor(0x0347,0x66);// 
	  IMX111MIPI_write_cmos_sensor(0x0348,0x0C);// 
	  IMX111MIPI_write_cmos_sensor(0x0349,0xD7);// 
	  IMX111MIPI_write_cmos_sensor(0x034A,0x08);// 
	  IMX111MIPI_write_cmos_sensor(0x034B,0x99);// 
	  IMX111MIPI_write_cmos_sensor(0x034C,0x0C);// 
	  IMX111MIPI_write_cmos_sensor(0x034D,0xD0);// 
	  IMX111MIPI_write_cmos_sensor(0x034E,0x07);// 
	  IMX111MIPI_write_cmos_sensor(0x034F,0x34);// 
	  IMX111MIPI_write_cmos_sensor(0x0381,0x01);// 
	  IMX111MIPI_write_cmos_sensor(0x0383,0x01);// 
	  IMX111MIPI_write_cmos_sensor(0x0385,0x01);// 
	  IMX111MIPI_write_cmos_sensor(0x0387,0x01);// 
	  IMX111MIPI_write_cmos_sensor(0x3033,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x303D,0x10);// 
	  IMX111MIPI_write_cmos_sensor(0x303E,0x41);// 
	  IMX111MIPI_write_cmos_sensor(0x3040,0x08);// 
	  IMX111MIPI_write_cmos_sensor(0x3041,0x97);// 
	  IMX111MIPI_write_cmos_sensor(0x3048,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x304C,0x6F);// 
	  IMX111MIPI_write_cmos_sensor(0x304D,0x03);// 
	  IMX111MIPI_write_cmos_sensor(0x3064,0x12);// 
	  IMX111MIPI_write_cmos_sensor(0x3073,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x3074,0x11);// 
	  IMX111MIPI_write_cmos_sensor(0x3075,0x11);// 
	  IMX111MIPI_write_cmos_sensor(0x3076,0x11);// 
	  IMX111MIPI_write_cmos_sensor(0x3077,0x11);// 
	  IMX111MIPI_write_cmos_sensor(0x3079,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x307A,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x309B,0x20);// 
	  IMX111MIPI_write_cmos_sensor(0x309C,0x13);// 
	  IMX111MIPI_write_cmos_sensor(0x309E,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x30A0,0x14);// 
	  IMX111MIPI_write_cmos_sensor(0x30A1,0x08);// 
	  IMX111MIPI_write_cmos_sensor(0x30AA,0x03);// 
	  IMX111MIPI_write_cmos_sensor(0x30B2,0x07);// 
	  IMX111MIPI_write_cmos_sensor(0x30D5,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x30D6,0x85);// 
	  IMX111MIPI_write_cmos_sensor(0x30D7,0x2A);// 
	  IMX111MIPI_write_cmos_sensor(0x30D8,0x64);// 
	  IMX111MIPI_write_cmos_sensor(0x30D9,0x89);// 
	  IMX111MIPI_write_cmos_sensor(0x30DE,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x30DF,0x20);// 
	  IMX111MIPI_write_cmos_sensor(0x3102,0x10);// 
	  IMX111MIPI_write_cmos_sensor(0x3103,0x44);// 
	  IMX111MIPI_write_cmos_sensor(0x3104,0x40);// 
	  IMX111MIPI_write_cmos_sensor(0x3105,0x00);// 
	  IMX111MIPI_write_cmos_sensor(0x3106,0x0D);// 
	  IMX111MIPI_write_cmos_sensor(0x3107,0x01);// 
	  IMX111MIPI_write_cmos_sensor(0x3108,0x09);// 
	  IMX111MIPI_write_cmos_sensor(0x3109,0x08);// 
	  IMX111MIPI_write_cmos_sensor(0x310A,0x0F);// 
	  IMX111MIPI_write_cmos_sensor(0x315C,0x5D);// 
	  IMX111MIPI_write_cmos_sensor(0x315D,0x5C);// 
	  IMX111MIPI_write_cmos_sensor(0x316E,0x5E);// 
	  IMX111MIPI_write_cmos_sensor(0x316F,0x5D);// 
	  IMX111MIPI_write_cmos_sensor(0x3318,0x60);// 
	  IMX111MIPI_write_cmos_sensor(0x3348,0xE0);// 
	  /*IMX111MIPI_write_cmos_sensor(0x3304,0x06);// 
	  IMX111MIPI_write_cmos_sensor(0x3305,0x02);// 
	  IMX111MIPI_write_cmos_sensor(0x3306,0x1e);// 
	  IMX111MIPI_write_cmos_sensor(0x3307,0x07);// 
	  IMX111MIPI_write_cmos_sensor(0x3308,0x0f);// 
	  IMX111MIPI_write_cmos_sensor(0x3309,0x08);// 
	  IMX111MIPI_write_cmos_sensor(0x330a,0x0d);// 
	  IMX111MIPI_write_cmos_sensor(0x330b,0x07);// 
	  IMX111MIPI_write_cmos_sensor(0x330c,0x0c);// 
	  IMX111MIPI_write_cmos_sensor(0x330D,0x08);// 
	  IMX111MIPI_write_cmos_sensor(0x3318,0x60);//*/ 
	  IMX111MIPI_write_cmos_sensor(0x0104,0);  
	  IMX111MIPI_write_cmos_sensor(0x0100,0x01);// STREAM START
	  SENSORDB("shutter&gain test by hhl: exit the video setting \n"); 
#endif




}


void PreviewSetting(void)
	{	
	


#if 1
        //PLL Setting  mipiClock 672Mhz/Lane  
		IMX111MIPI_write_cmos_sensor(0x0104,1);  
		IMX111MIPI_write_cmos_sensor(0x0100,0x00);// STREAM STop
		IMX111MIPI_write_cmos_sensor(0x0305,0x02);
		IMX111MIPI_write_cmos_sensor(0x0307,0x38);
		IMX111MIPI_write_cmos_sensor(0x30A4,0x02);
		IMX111MIPI_write_cmos_sensor(0x303C,0x4B);
//<2013/06/21-26217-alberthsiao,fix ANR issue for FIFA test
		mdelay(120);
//>2013/06/21-26217-alberthsiao
		//size setting  s
		IMX111MIPI_write_cmos_sensor(0x0340,0x04);
		IMX111MIPI_write_cmos_sensor(0x0341,0xE6);
		IMX111MIPI_write_cmos_sensor(0x0342,0x0D);
		IMX111MIPI_write_cmos_sensor(0x0343,0xD0);
		IMX111MIPI_write_cmos_sensor(0x0344,0x00);
		IMX111MIPI_write_cmos_sensor(0x0345,0x08);
		IMX111MIPI_write_cmos_sensor(0x0346,0x00);
		IMX111MIPI_write_cmos_sensor(0x0347,0x30);
		IMX111MIPI_write_cmos_sensor(0x0348,0x0C);
		IMX111MIPI_write_cmos_sensor(0x0349,0xD7);
		IMX111MIPI_write_cmos_sensor(0x034A,0x09);
		IMX111MIPI_write_cmos_sensor(0x034B,0xCF);
		IMX111MIPI_write_cmos_sensor(0x034C,0x06);
		IMX111MIPI_write_cmos_sensor(0x034D,0x68);
		IMX111MIPI_write_cmos_sensor(0x034E,0x04);
		IMX111MIPI_write_cmos_sensor(0x034F,0xD0);
		IMX111MIPI_write_cmos_sensor(0x0381,0x01);
		IMX111MIPI_write_cmos_sensor(0x0383,0x03);
		IMX111MIPI_write_cmos_sensor(0x0385,0x01);
		IMX111MIPI_write_cmos_sensor(0x0387,0x03);
		IMX111MIPI_write_cmos_sensor(0x3033,0x00);
		IMX111MIPI_write_cmos_sensor(0x303D,0x10);
		IMX111MIPI_write_cmos_sensor(0x303E,0x40);
		IMX111MIPI_write_cmos_sensor(0x3040,0x08);
		IMX111MIPI_write_cmos_sensor(0x3041,0x97);
		IMX111MIPI_write_cmos_sensor(0x3048,0x01);
		IMX111MIPI_write_cmos_sensor(0x304C,0x6F);
		IMX111MIPI_write_cmos_sensor(0x304D,0x03);
		IMX111MIPI_write_cmos_sensor(0x3064,0x12);
		IMX111MIPI_write_cmos_sensor(0x3073,0x00);
		IMX111MIPI_write_cmos_sensor(0x3074,0x11);
		IMX111MIPI_write_cmos_sensor(0x3075,0x11);
		IMX111MIPI_write_cmos_sensor(0x3076,0x11);
		IMX111MIPI_write_cmos_sensor(0x3077,0x11);
		IMX111MIPI_write_cmos_sensor(0x3079,0x00);
		IMX111MIPI_write_cmos_sensor(0x307A,0x00);
		IMX111MIPI_write_cmos_sensor(0x309B,0x28);
		IMX111MIPI_write_cmos_sensor(0x309C,0x13);
		IMX111MIPI_write_cmos_sensor(0x309E,0x00);
		IMX111MIPI_write_cmos_sensor(0x30A0,0x14);
		IMX111MIPI_write_cmos_sensor(0x30A1,0x09);
		IMX111MIPI_write_cmos_sensor(0x30AA,0x03);
		IMX111MIPI_write_cmos_sensor(0x30B2,0x05);
		IMX111MIPI_write_cmos_sensor(0x30D5,0x09);
		IMX111MIPI_write_cmos_sensor(0x30D6,0x01);
		IMX111MIPI_write_cmos_sensor(0x30D7,0x01);
		IMX111MIPI_write_cmos_sensor(0x30D8,0x64);
		IMX111MIPI_write_cmos_sensor(0x30D9,0x89);
		IMX111MIPI_write_cmos_sensor(0x30DE,0x02);
		IMX111MIPI_write_cmos_sensor(0x30DF,0x20);
		IMX111MIPI_write_cmos_sensor(0x3102,0x08);
		IMX111MIPI_write_cmos_sensor(0x3103,0x22);
		IMX111MIPI_write_cmos_sensor(0x3104,0x20);
		IMX111MIPI_write_cmos_sensor(0x3105,0x00);
		IMX111MIPI_write_cmos_sensor(0x3106,0x87);
		IMX111MIPI_write_cmos_sensor(0x3107,0x00);
		IMX111MIPI_write_cmos_sensor(0x3108,0x03);
		IMX111MIPI_write_cmos_sensor(0x3109,0x02);
		IMX111MIPI_write_cmos_sensor(0x310A,0x03);
		IMX111MIPI_write_cmos_sensor(0x315C,0x9C);
		IMX111MIPI_write_cmos_sensor(0x315D,0x9B);
		IMX111MIPI_write_cmos_sensor(0x316E,0x9D);
		IMX111MIPI_write_cmos_sensor(0x316F,0x9C);
		IMX111MIPI_write_cmos_sensor(0x3301,0x04);
		IMX111MIPI_write_cmos_sensor(0x3318,0x72);
		IMX111MIPI_write_cmos_sensor(0x3348,0xE0);
		IMX111MIPI_write_cmos_sensor(0x0104,0);   
		IMX111MIPI_write_cmos_sensor(0x0100,0x01);//STREAM START
#endif

}

void IMX111MIPI_set_8M(void)
{	
     #if 0
	// capture setting	  mipiclock=
			IMX111MIPI_write_cmos_sensor(0x0100,0x00);// STREAM STop
			//PLL setting
			IMX111MIPI_write_cmos_sensor(0x0305,0x02);// PLL input divider 1/2
			IMX111MIPI_write_cmos_sensor(0x0307,0x38);// PLL multiplier X 56
			IMX111MIPI_write_cmos_sensor(0x30A4,0x02);// 24Mhz / 2 X 56 / 1 = 672Mhz 
			IMX111MIPI_write_cmos_sensor(0x303C,0x4B);
			//size setting 
			IMX111MIPI_write_cmos_sensor(0x0340,0x09);
			IMX111MIPI_write_cmos_sensor(0x0341,0xBA);
			IMX111MIPI_write_cmos_sensor(0x0342,0x0D);// 
			IMX111MIPI_write_cmos_sensor(0x0343,0xD0);// 
			IMX111MIPI_write_cmos_sensor(0x0346,0x00);// 
			IMX111MIPI_write_cmos_sensor(0x0347,0x30);// 
			IMX111MIPI_write_cmos_sensor(0x034A,0x09);// 
			IMX111MIPI_write_cmos_sensor(0x034B,0xCF);// 
			IMX111MIPI_write_cmos_sensor(0x034C,0x0C);
			IMX111MIPI_write_cmos_sensor(0x034D,0xD0); 
			IMX111MIPI_write_cmos_sensor(0x034E,0x09);
			IMX111MIPI_write_cmos_sensor(0x034F,0xA0);
			IMX111MIPI_write_cmos_sensor(0x0381,0x01);
			IMX111MIPI_write_cmos_sensor(0x0383,0x01);
			IMX111MIPI_write_cmos_sensor(0x0385,0x01);
			IMX111MIPI_write_cmos_sensor(0x0387,0x01);
			IMX111MIPI_write_cmos_sensor(0x303E,0x41);// 
			IMX111MIPI_write_cmos_sensor(0x3048,0x00);
			IMX111MIPI_write_cmos_sensor(0x304C,0x57);// 
			IMX111MIPI_write_cmos_sensor(0x309B,0x20);
			IMX111MIPI_write_cmos_sensor(0x30A1,0x08);
			IMX111MIPI_write_cmos_sensor(0x30AA,0x03);// 
			IMX111MIPI_write_cmos_sensor(0x30D5,0x00);
			IMX111MIPI_write_cmos_sensor(0x30D6,0x85);
			IMX111MIPI_write_cmos_sensor(0x30D7,0x2A);
			IMX111MIPI_write_cmos_sensor(0x30D8,0x64);// 
			IMX111MIPI_write_cmos_sensor(0x30D9,0x89);// 
			IMX111MIPI_write_cmos_sensor(0x30DE,0x00);
			IMX111MIPI_write_cmos_sensor(0x3102,0x10);// 
			IMX111MIPI_write_cmos_sensor(0x3103,0x44);// 
			IMX111MIPI_write_cmos_sensor(0x3104,0x40);// 
			IMX111MIPI_write_cmos_sensor(0x3105,0x00);// 
			IMX111MIPI_write_cmos_sensor(0x3106,0x0D);// 
			IMX111MIPI_write_cmos_sensor(0x3107,0x01);// 
			IMX111MIPI_write_cmos_sensor(0x3108,0x09);// 
			IMX111MIPI_write_cmos_sensor(0x3109,0x08);// 
			IMX111MIPI_write_cmos_sensor(0x310A,0x0F);// 
			IMX111MIPI_write_cmos_sensor(0x315C,0x5D);// 
			IMX111MIPI_write_cmos_sensor(0x315D,0x5C);// 
			IMX111MIPI_write_cmos_sensor(0x316E,0x5E);// 
			IMX111MIPI_write_cmos_sensor(0x316F,0x5D);// 
			IMX111MIPI_write_cmos_sensor(0x3318,0x62);
			IMX111MIPI_write_cmos_sensor(0x3301,0x00);//
			IMX111MIPI_write_cmos_sensor(0x0100,0x01);//STREAM START
			printk("[IMX111MIPIRaw] Set 8M End\n"); 
	#endif
#if 1
			IMX111MIPI_write_cmos_sensor(0x0100,0x00);// STREAM STop
			IMX111MIPI_write_cmos_sensor(0x0305,0x02);
			IMX111MIPI_write_cmos_sensor(0x0307,0x38);
			IMX111MIPI_write_cmos_sensor(0x30A4,0x02);
			IMX111MIPI_write_cmos_sensor(0x303C,0x4B);
			IMX111MIPI_write_cmos_sensor(0x0340,0x09);
			IMX111MIPI_write_cmos_sensor(0x0341,0xC6);
			IMX111MIPI_write_cmos_sensor(0x0342,0x0D);
			IMX111MIPI_write_cmos_sensor(0x0343,0xD0);
			IMX111MIPI_write_cmos_sensor(0x0344,0x00);
			IMX111MIPI_write_cmos_sensor(0x0345,0x08);
			IMX111MIPI_write_cmos_sensor(0x0346,0x00);
			IMX111MIPI_write_cmos_sensor(0x0347,0x30);
			IMX111MIPI_write_cmos_sensor(0x0348,0x0C);
			IMX111MIPI_write_cmos_sensor(0x0349,0xD7);
			IMX111MIPI_write_cmos_sensor(0x034A,0x09);
			IMX111MIPI_write_cmos_sensor(0x034B,0xCF);
			IMX111MIPI_write_cmos_sensor(0x034C,0x0C);
			IMX111MIPI_write_cmos_sensor(0x034D,0xD0);
			IMX111MIPI_write_cmos_sensor(0x034E,0x09);
			IMX111MIPI_write_cmos_sensor(0x034F,0xA0);
			IMX111MIPI_write_cmos_sensor(0x0381,0x01);
			IMX111MIPI_write_cmos_sensor(0x0383,0x01);
			IMX111MIPI_write_cmos_sensor(0x0385,0x01);
			IMX111MIPI_write_cmos_sensor(0x0387,0x01);
			IMX111MIPI_write_cmos_sensor(0x3033,0x00);
			IMX111MIPI_write_cmos_sensor(0x303D,0x00);
			IMX111MIPI_write_cmos_sensor(0x303E,0x40);
			IMX111MIPI_write_cmos_sensor(0x3040,0x08);
			IMX111MIPI_write_cmos_sensor(0x3041,0x97);
			IMX111MIPI_write_cmos_sensor(0x3048,0x00);
			IMX111MIPI_write_cmos_sensor(0x304C,0x6F);
			IMX111MIPI_write_cmos_sensor(0x304D,0x03);
			IMX111MIPI_write_cmos_sensor(0x3064,0x12);
			IMX111MIPI_write_cmos_sensor(0x3073,0x00);
			IMX111MIPI_write_cmos_sensor(0x3074,0x11);
			IMX111MIPI_write_cmos_sensor(0x3075,0x11);
			IMX111MIPI_write_cmos_sensor(0x3076,0x11);
			IMX111MIPI_write_cmos_sensor(0x3077,0x11);
			IMX111MIPI_write_cmos_sensor(0x3079,0x00);
			IMX111MIPI_write_cmos_sensor(0x307A,0x00);
			IMX111MIPI_write_cmos_sensor(0x309B,0x20);
			IMX111MIPI_write_cmos_sensor(0x309C,0x13);
			IMX111MIPI_write_cmos_sensor(0x309E,0x00);
			IMX111MIPI_write_cmos_sensor(0x30A0,0x14);
			IMX111MIPI_write_cmos_sensor(0x30A1,0x08);
			IMX111MIPI_write_cmos_sensor(0x30AA,0x03);
			IMX111MIPI_write_cmos_sensor(0x30B2,0x07);
			IMX111MIPI_write_cmos_sensor(0x30D5,0x00);
			IMX111MIPI_write_cmos_sensor(0x30D6,0x85);
			IMX111MIPI_write_cmos_sensor(0x30D7,0x2A);
			IMX111MIPI_write_cmos_sensor(0x30D8,0x64);
			IMX111MIPI_write_cmos_sensor(0x30D9,0x89);
			IMX111MIPI_write_cmos_sensor(0x30DE,0x00);
			IMX111MIPI_write_cmos_sensor(0x30DF,0x20);
			IMX111MIPI_write_cmos_sensor(0x3102,0x08);
			IMX111MIPI_write_cmos_sensor(0x3103,0x22);
			IMX111MIPI_write_cmos_sensor(0x3104,0x20);
			IMX111MIPI_write_cmos_sensor(0x3105,0x00);
			IMX111MIPI_write_cmos_sensor(0x3106,0x87);
			IMX111MIPI_write_cmos_sensor(0x3107,0x00);
			IMX111MIPI_write_cmos_sensor(0x3108,0x03);
			IMX111MIPI_write_cmos_sensor(0x3109,0x02);
			IMX111MIPI_write_cmos_sensor(0x310A,0x03);
			IMX111MIPI_write_cmos_sensor(0x315C,0x9C);
			IMX111MIPI_write_cmos_sensor(0x315D,0x9B);
			IMX111MIPI_write_cmos_sensor(0x316E,0x9D);
			IMX111MIPI_write_cmos_sensor(0x316F,0x9C);
			IMX111MIPI_write_cmos_sensor(0x3301,0x04);
			IMX111MIPI_write_cmos_sensor(0x3318,0x62);
			IMX111MIPI_write_cmos_sensor(0x3348,0xE0);
			IMX111MIPI_write_cmos_sensor(0x0100,0x01);//STREAM START
#endif
}
/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   IMX111MIPIOpen
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

UINT32 IMX111MIPIOpen(void)
{
   int  retry = 0; 
	kal_uint16 sensorid;
    // check if sensor ID correct
    retry = 3; 
    do {
       SENSORDB("Read ID in the Open function"); 
	   sensorid=(kal_uint16)((IMX111MIPI_read_cmos_sensor(0x0000)<<8) | IMX111MIPI_read_cmos_sensor(0x0001));  
	   spin_lock(&imx111_drv_lock);    
	   IMX111MIPI_sensor_id =sensorid;
	   spin_unlock(&imx111_drv_lock);
		if (IMX111MIPI_sensor_id == IMX111MIPI_SENSOR_ID)
		break; 
		SENSORDB("Read Sensor ID Fail = 0x%04x\n", IMX111MIPI_sensor_id); 
		retry--; 
	    }
	while (retry > 0);
    SENSORDB("Read Sensor ID = 0x%04x\n", IMX111MIPI_sensor_id); 
    if (IMX111MIPI_sensor_id != IMX111MIPI_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;
    IMX111MIPI_Sensor_Init();
	sensorid=read_IMX111MIPI_gain();
	spin_lock(&imx111_drv_lock);	
    IMX111MIPI_sensor_gain_base = sensorid;
	spin_unlock(&imx111_drv_lock);
	
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   IMX111MIPIGetSensorID
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
UINT32 IMX111MIPIGetSensorID(UINT32 *sensorID) 
{
   int  retry = 3; 
    // check if sensor ID correct
    do {		
	   *sensorID =(kal_uint16)((IMX111MIPI_read_cmos_sensor(0x0000)<<8) | IMX111MIPI_read_cmos_sensor(0x0001)); 
        if (*sensorID == IMX111MIPI_SENSOR_ID)
            break;
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
		
    	} while (retry > 0);

    if (*sensorID != IMX111MIPI_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   IMX111MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of IMX111MIPI to change exposure time.
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
void IMX111MIPI_SetShutter(kal_uint16 iShutter)
{

	 SENSORDB("[IMX111MIPI]%s():shutter=%d\n",__FUNCTION__,iShutter);
   
   SENSORDB("shutter&gain test by hhl: set shutter IMX111MIPI_SetShutter\n"); 
    if (iShutter < 1)
        iShutter = 1; 
	else if(iShutter > 0xffff)
		iShutter = 0xffff;
    IMX111MIPI_write_shutter(iShutter);
}   /*  IMX111MIPI_SetShutter   */



/*************************************************************************
* FUNCTION
*   IMX111MIPI_read_shutter
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
UINT16 IMX111MIPI_read_shutter(void)
{
    return (UINT16)( (IMX111MIPI_read_cmos_sensor(0x0202)<<8) | IMX111MIPI_read_cmos_sensor(0x0203) );
}

/*************************************************************************
* FUNCTION
*   IMX111MIPI_night_mode
*
* DESCRIPTION
*   This function night mode of IMX111MIPI.
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
void IMX111MIPI_NightMode(kal_bool bEnable)
{
#if 0
    /************************************************************************/
    /*                      Auto Mode: 30fps                                                                                          */
    /*                      Night Mode:15fps                                                                                          */
    /************************************************************************/
    if(bEnable)
    {
        if(OV5642_MPEG4_encode_mode==KAL_TRUE)
        {
            OV5642_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642_sensor_pclk/15)/(OV5642_PV_PERIOD_PIXEL_NUMS+OV5642_PV_dummy_pixels));
            OV5642_write_cmos_sensor(0x350C, (OV5642_MAX_EXPOSURE_LINES >> 8) & 0xFF);
            OV5642_write_cmos_sensor(0x350D, OV5642_MAX_EXPOSURE_LINES & 0xFF);
            OV5642_CURRENT_FRAME_LINES = OV5642_MAX_EXPOSURE_LINES;
            OV5642_MAX_EXPOSURE_LINES = OV5642_CURRENT_FRAME_LINES - OV5642_SHUTTER_LINES_GAP;
        }
    }
    else// Fix video framerate 30 fps
    {
        if(OV5642_MPEG4_encode_mode==KAL_TRUE)
        {
            OV5642_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642_sensor_pclk/30)/(OV5642_PV_PERIOD_PIXEL_NUMS+OV5642_PV_dummy_pixels));
            if(OV5642_pv_exposure_lines < (OV5642_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                OV5642_write_cmos_sensor(0x350C, (OV5642_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                OV5642_write_cmos_sensor(0x350D, OV5642_MAX_EXPOSURE_LINES & 0xFF);
                OV5642_CURRENT_FRAME_LINES = OV5642_MAX_EXPOSURE_LINES;
            }
            OV5642_MAX_EXPOSURE_LINES = OV5642_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP;
        }
    }
#endif	
}/*	IMX111MIPI_NightMode */



/*************************************************************************
* FUNCTION
*   IMX111MIPIClose
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
UINT32 IMX111MIPIClose(void)
{
   // IMX111MIPI_write_cmos_sensor(0x0100,0x00);
    return ERROR_NONE;
}	/* IMX111MIPIClose() */

void IMX111MIPISetFlipMirror(kal_int32 imgMirror)
{
    kal_uint8  iTemp; 
	
    iTemp = IMX111MIPI_read_cmos_sensor(0x0101) & 0x03;	//Clear the mirror and flip bits.
    switch (imgMirror)
    {
        case IMAGE_NORMAL:
            IMX111MIPI_write_cmos_sensor(0x0101, 0x03);	//Set normal
            break;
        case IMAGE_V_MIRROR:
            IMX111MIPI_write_cmos_sensor(0x0101, iTemp | 0x01);	//Set flip
            break;
        case IMAGE_H_MIRROR:
            IMX111MIPI_write_cmos_sensor(0x0101, iTemp | 0x02);	//Set mirror
            break;
        case IMAGE_HV_MIRROR:
            IMX111MIPI_write_cmos_sensor(0x0101, 0x00);	//Set mirror and flip
            break;
    }
}


/*************************************************************************
* FUNCTION
*   IMX111MIPIPreview
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
UINT32 IMX111MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 iStartX = 0, iStartY = 0;
	
	SENSORDB("shutter&gain test by hhl: enter the preview function IMX111MIPIPreview\n"); 
    
		//IMX111MIPI_sensor.pv_dummy_pixels = 0;
		//IMX111MIPI_sensor.pv_dummy_lines = 0;
    	spin_lock(&imx111_drv_lock);    
		IMX111MIPI_sensor.video_mode=KAL_FALSE;
		IMX111MIPI_sensor.pv_mode=KAL_TRUE;
		IMX111MIPI_sensor.capture_mode=KAL_FALSE;
		spin_unlock(&imx111_drv_lock);
        PreviewSetting();
		iStartX += IMX111MIPI_IMAGE_SENSOR_PV_STARTX;
		iStartY += IMX111MIPI_IMAGE_SENSOR_PV_STARTY;
		spin_lock(&imx111_drv_lock);	
		IMX111MIPI_sensor.pv_dummy_pixels = 0;
		IMX111MIPI_sensor.pv_dummy_lines = 0;
		IMX111MIPI_sensor.pv_line_length = IMX111MIPI_PV_LINE_LENGTH_PIXELS+IMX111MIPI_sensor.pv_dummy_pixels; 
		IMX111MIPI_sensor.pv_frame_length = IMX111MIPI_PV_FRAME_LENGTH_LINES+IMX111MIPI_sensor.pv_dummy_lines;
		spin_unlock(&imx111_drv_lock);
		IMX111MIPI_SetDummy(IMX111MIPI_sensor.pv_dummy_pixels,IMX111MIPI_sensor.pv_dummy_lines);
	//	IMX111MIPI_SetShutter(IMX111MIPI_sensor.pv_shutter);
		spin_lock(&imx111_drv_lock);	
		memcpy(&IMX111MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
		spin_unlock(&imx111_drv_lock);
		image_window->GrabStartX= iStartX;
		image_window->GrabStartY= iStartY;
		image_window->ExposureWindowWidth= IMX111MIPI_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
		image_window->ExposureWindowHeight= IMX111MIPI_IMAGE_SENSOR_PV_HEIGHT - 2*iStartY;
		SENSORDB("Preview resolution:%d %d %d %d\n", image_window->GrabStartX, image_window->GrabStartY, image_window->ExposureWindowWidth, image_window->ExposureWindowHeight); 
		SENSORDB("shutter&gain test by hhl: exit the preview function IMX111MIPIPreview\n"); 
	return ERROR_NONE;
}	/* IMX111MIPIPreview() */



/*************************************************************************
* FUNCTION
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX111MIPIVideo(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 iStartX = 0, iStartY = 0;
	SENSORDB("shutter&gain test by hhl: enter the video function IMX111MIPIPreview\n"); 
    	spin_lock(&imx111_drv_lock);    
		IMX111MIPI_sensor.video_mode=KAL_TRUE;
		IMX111MIPI_sensor.pv_mode=KAL_FALSE;
		IMX111MIPI_sensor.capture_mode=KAL_FALSE;
		spin_unlock(&imx111_drv_lock);
		VideoFullSizeSetting();
		iStartX += IMX111MIPI_IMAGE_SENSOR_VIDEO_STARTX;
		iStartY += IMX111MIPI_IMAGE_SENSOR_VIDEO_STARTY;
		spin_lock(&imx111_drv_lock);	
		IMX111MIPI_sensor.video_dummy_pixels = 0;
		IMX111MIPI_sensor.video_dummy_lines = 0;
		IMX111MIPI_sensor.video_line_length = IMX111MIPI_VIDEO_LINE_LENGTH_PIXELS+IMX111MIPI_sensor.video_dummy_pixels; 
		IMX111MIPI_sensor.video_frame_length = IMX111MIPI_VIDEO_FRAME_LENGTH_LINES+IMX111MIPI_sensor.video_dummy_lines;
		spin_unlock(&imx111_drv_lock);
		spin_lock(&imx111_drv_lock);	
		memcpy(&IMX111MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
		spin_unlock(&imx111_drv_lock);
		image_window->GrabStartX= iStartX;
		image_window->GrabStartY= iStartY;
	//	image_window->ExposureWindowWidth= IMX111MIPI_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
	//	image_window->ExposureWindowHeight= IMX111MIPI_IMAGE_SENSOR_PV_HEIGHT - 2*iStartY;
		SENSORDB("VideoPreview resolution:%d %d %d %d\n", image_window->GrabStartX, image_window->GrabStartY, image_window->ExposureWindowWidth, image_window->ExposureWindowHeight); 
		SENSORDB("shutter&gain test by hhl: exit the preview function IMX111MIPIPreview\n");    
		IMX111MIPI_SetDummy(IMX111MIPI_sensor.video_dummy_pixels,IMX111MIPI_sensor.video_dummy_lines);
	return ERROR_NONE;
}	/* IMX111MIPIPreview() */



UINT32 IMX111MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

  //  kal_uint32 shutter=IMX111MIPI_sensor.pv_shutter;
    kal_uint16 iStartX = 0, iStartY = 0;
	
	SENSORDB("shutter&gain test by hhl: enter the capture function IMX111MIPICapture\n"); 
	spin_lock(&imx111_drv_lock);	
	IMX111MIPI_sensor.video_mode=KAL_FALSE;
	IMX111MIPI_sensor.pv_mode=KAL_FALSE;
	IMX111MIPI_sensor.capture_mode=KAL_TRUE;
	IMX111MIPI_sensor.cp_dummy_pixels = 0;
	IMX111MIPI_sensor.cp_dummy_lines = 0;
	spin_unlock(&imx111_drv_lock);
     IMX111MIPI_set_8M();
	 spin_lock(&imx111_drv_lock);    
     IMX111MIPI_sensor.cp_line_length=IMX111MIPI_FULL_LINE_LENGTH_PIXELS+IMX111MIPI_sensor.cp_dummy_pixels;
     IMX111MIPI_sensor.cp_frame_length=IMX111MIPI_FULL_FRAME_LENGTH_LINES+IMX111MIPI_sensor.cp_dummy_lines;
	 spin_unlock(&imx111_drv_lock);
	iStartX = IMX111MIPI_IMAGE_SENSOR_CAP_STARTX;
	iStartY = IMX111MIPI_IMAGE_SENSOR_CAP_STARTY;
	image_window->GrabStartX=iStartX;
	image_window->GrabStartY=iStartY;
	image_window->ExposureWindowWidth=IMX111MIPI_IMAGE_SENSOR_FULL_WIDTH -2*iStartX;
	image_window->ExposureWindowHeight=IMX111MIPI_IMAGE_SENSOR_FULL_HEIGHT-2*iStartY;
    IMX111MIPI_SetDummy(IMX111MIPI_sensor.cp_dummy_pixels, IMX111MIPI_sensor.cp_dummy_lines);
	spin_lock(&imx111_drv_lock);	
    memcpy(&IMX111MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx111_drv_lock);
	SENSORDB("shutter&gain test by hhl: exit the capture function IMX111MIPICapture\n"); 

    return ERROR_NONE;
	
}	/* IMX111MIPICapture() */

UINT32 IMX111MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    pSensorResolution->SensorPreviewWidth	= IMX111MIPI_REAL_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= IMX111MIPI_REAL_PV_HEIGHT;
    pSensorResolution->SensorFullWidth		= IMX111MIPI_REAL_CAP_WIDTH;
    pSensorResolution->SensorFullHeight		= IMX111MIPI_REAL_CAP_HEIGHT;
    pSensorResolution->SensorVideoWidth		= IMX111MIPI_REAL_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = IMX111MIPI_REAL_VIDEO_HEIGHT;
    SENSORDB("IMX111MIPIGetResolution :8-14");    

    return ERROR_NONE;
}   /* IMX111MIPIGetResolution() */

UINT32 IMX111MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch(ScenarioId){
			case MSDK_SCENARIO_ID_CAMERA_ZSD:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG://hhl 2-28
				pSensorInfo->SensorFullResolutionX=IMX111MIPI_REAL_CAP_WIDTH;
				pSensorInfo->SensorFullResolutionY=IMX111MIPI_REAL_CAP_HEIGHT;
				pSensorInfo->SensorStillCaptureFrameRate=22;
			break;//hhl 2-28
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				pSensorInfo->SensorPreviewResolutionX=IMX111MIPI_REAL_VIDEO_WIDTH;
				pSensorInfo->SensorPreviewResolutionY=IMX111MIPI_REAL_VIDEO_HEIGHT;
				pSensorInfo->SensorCameraPreviewFrameRate=30;
			break;
		default:
        pSensorInfo->SensorPreviewResolutionX=IMX111MIPI_REAL_PV_WIDTH;
        pSensorInfo->SensorPreviewResolutionY=IMX111MIPI_REAL_PV_HEIGHT;
		pSensorInfo->SensorCameraPreviewFrameRate=30;
			break;
	}
//	pSensorInfo->SensorPreviewResolutionX=IMX111MIPI_REAL_CAP_WIDTH;
   //     pSensorInfo->SensorPreviewResolutionY=IMX111MIPI_REAL_CAP_HEIGHT;

    pSensorInfo->SensorVideoFrameRate=30;
//    pSensorInfo->SensorStillCaptureFrameRate=22;
//    pSensorInfo->SensorWebCamCaptureFrameRate=22;//hhl 2-28
	
    pSensorInfo->SensorStillCaptureFrameRate=22;
    pSensorInfo->SensorWebCamCaptureFrameRate=22;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_R;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;
    pSensorInfo->CaptureDelayFrame = 3; 
    pSensorInfo->PreviewDelayFrame = 3; 
    pSensorInfo->VideoDelayFrame = 3; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_4MA;      
    pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;	
	   
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
     //   case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX111MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX111MIPI_IMAGE_SENSOR_PV_STARTY;           		
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
			   pSensorInfo->SensorClockDividCount= 5;
			   pSensorInfo->SensorClockRisingCount= 0;
			   pSensorInfo->SensorClockFallingCount= 2;
			   pSensorInfo->SensorPixelClockCount= 3;
			   pSensorInfo->SensorDataLatchCount= 2;
			   pSensorInfo->SensorGrabStartX = IMX111MIPI_IMAGE_SENSOR_VIDEO_STARTX; 
			   pSensorInfo->SensorGrabStartY = IMX111MIPI_IMAGE_SENSOR_VIDEO_STARTY;				   
			   pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;		   
			   pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
			pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
			pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			   pSensorInfo->SensorWidthSampling = 1;  // 0 is default 1x
			   pSensorInfo->SensorHightSampling = 1;   // 0 is default 1x 
			   pSensorInfo->SensorPacketECCOrder = 1;

			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
       // case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX111MIPI_IMAGE_SENSOR_CAP_STARTX;	//2*IMX111MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX111MIPI_IMAGE_SENSOR_CAP_STARTY;	//2*IMX111MIPI_IMAGE_SENSOR_PV_STARTY;          			
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
			 pSensorInfo->SensorClockDividCount= 5;
			 pSensorInfo->SensorClockRisingCount= 0;
			 pSensorInfo->SensorClockFallingCount= 2;
			 pSensorInfo->SensorPixelClockCount= 3;
			 pSensorInfo->SensorDataLatchCount= 2;
			 pSensorInfo->SensorGrabStartX = IMX111MIPI_IMAGE_SENSOR_PV_STARTX; 
			 pSensorInfo->SensorGrabStartY = IMX111MIPI_IMAGE_SENSOR_PV_STARTY; 				 
			 pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;		 
			 pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
		  pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
		  pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			 pSensorInfo->SensorWidthSampling = 1;	// 0 is default 1x
			 pSensorInfo->SensorHightSampling = 1;	 // 0 is default 1x 
			 pSensorInfo->SensorPacketECCOrder = 2;

            break;
    }
	spin_lock(&imx111_drv_lock);	

    IMX111MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &IMX111MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&imx111_drv_lock);

    return ERROR_NONE;
}   /* IMX111MIPIGetInfo() */


UINT32 IMX111MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		spin_lock(&imx111_drv_lock);	
		CurrentScenarioId = ScenarioId;
		spin_unlock(&imx111_drv_lock);
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            IMX111MIPIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			IMX111MIPIVideo(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	    case MSDK_SCENARIO_ID_CAMERA_ZSD:
            IMX111MIPICapture(pImageWindow, pSensorConfigData);//hhl 2-28
            break;
        default:
            return ERROR_INVALID_SCENARIO_ID;
    }
    return ERROR_NONE;
} /* IMX111MIPIControl() */

UINT32 IMX111MIPISetVideoMode(UINT16 u2FrameRate)
{
         SENSORDB("[IMX111MIPISetVideoMode] frame rate = %d\n", u2FrameRate);
		kal_uint16 IMX111MIPI_Video_Max_Expourse_Time = 0;
		SENSORDB("[IMX111MIPI]%s():fix_frame_rate=%d\n",__FUNCTION__,u2FrameRate);
	//	VideoFullSizeSetting()
		if(u2FrameRate>=24)
			u2FrameRate=30;
		else
			u2FrameRate=15; 
		spin_lock(&imx111_drv_lock);
		IMX111MIPI_sensor.fix_video_fps = KAL_TRUE;
		spin_unlock(&imx111_drv_lock);
		u2FrameRate=u2FrameRate*10;//10*FPS
		SENSORDB("[IMX111MIPI][Enter Fix_fps func] IMX111MIPI_Fix_Video_Frame_Rate = %d\n", u2FrameRate/10);
	
		IMX111MIPI_Video_Max_Expourse_Time = (kal_uint16)((IMX111MIPI_sensor.video_pclk*10/u2FrameRate)/IMX111MIPI_sensor.video_line_length);
		
		if (IMX111MIPI_Video_Max_Expourse_Time > IMX111MIPI_VIDEO_FRAME_LENGTH_LINES/*IMX111MIPI_sensor.pv_frame_length*/) 
			{
				spin_lock(&imx111_drv_lock);    
				IMX111MIPI_sensor.video_frame_length = IMX111MIPI_Video_Max_Expourse_Time;
				IMX111MIPI_sensor.video_dummy_lines = IMX111MIPI_sensor.video_frame_length-IMX111MIPI_VIDEO_FRAME_LENGTH_LINES;
				spin_unlock(&imx111_drv_lock);
				SENSORDB("[IMX111MIPI]%s():frame_length=%d,dummy_lines=%d\n",__FUNCTION__,IMX111MIPI_sensor.video_frame_length,IMX111MIPI_sensor.video_dummy_lines);
				IMX111MIPI_SetDummy(IMX111MIPI_sensor.video_dummy_pixels,IMX111MIPI_sensor.video_dummy_lines);
			}
	spin_lock(&imx111_drv_lock);    
    IMX111MIPI_MPEG4_encode_mode = KAL_TRUE; 
	spin_unlock(&imx111_drv_lock);
	
    return TRUE;
}

UINT32 IMX111MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
/*	kal_uint32 pv_max_frame_rate_lines=0;

if(IMX111MIPI_sensor.pv_mode==TRUE)
	pv_max_frame_rate_lines=IMX111MIPI_PV_FRAME_LENGTH_LINES;
else
    pv_max_frame_rate_lines=IMX111MIPI_VIDEO_FRAME_LENGTH_LINES	;
//kal_uint32 pv_max_frame_rate_lines = IMX111MIPI_MAX_EXPOSURE_LINES;

    SENSORDB("[IMX111MIPISetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
    if(bEnable) {   // enable auto flicker   
    	spin_lock(&imx111_drv_lock);    
        IMX111MIPI_Auto_Flicker_mode = KAL_TRUE; 
		spin_unlock(&imx111_drv_lock);
        if(IMX111MIPI_MPEG4_encode_mode == KAL_TRUE) { // in the video mode, reset the frame rate
        IMX111MIPI_MAX_EXPOSURE_LINES=pv_max_frame_rate_lines-5;
            pv_max_frame_rate_lines = IMX111MIPI_MAX_EXPOSURE_LINES + (IMX111MIPI_MAX_EXPOSURE_LINES>>7);            
            IMX111MIPI_write_cmos_sensor(0x0104, 1);        
            IMX111MIPI_write_cmos_sensor(0x0340, (pv_max_frame_rate_lines >>8) & 0xFF);
            IMX111MIPI_write_cmos_sensor(0x0341, pv_max_frame_rate_lines & 0xFF);	
            IMX111MIPI_write_cmos_sensor(0x0104, 0);        	
        }
    } else {
    	/*spin_lock(&imx111_drv_lock);    
        IMX111MIPI_Auto_Flicker_mode = KAL_FALSE; 
		spin_unlock(&imx111_drv_lock);
        if(IMX111MIPI_MPEG4_encode_mode == KAL_TRUE) {    // in the video mode, restore the frame rate
            IMX111MIPI_write_cmos_sensor(0x0104, 1);        
            IMX111MIPI_write_cmos_sensor(0x0340, (IMX111MIPI_MAX_EXPOSURE_LINES >>8) & 0xFF);
            IMX111MIPI_write_cmos_sensor(0x0341, IMX111MIPI_MAX_EXPOSURE_LINES & 0xFF);	
            IMX111MIPI_write_cmos_sensor(0x0104, 0);        	
        }
        printk("Disable Auto flicker\n");    
    }*/
	return ERROR_NONE;
}




UINT32 IMX111MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	SENSORDB("IMX111MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = 134200000;
			lineLength = IMX111MIPI_PV_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX111MIPI_PV_FRAME_LENGTH_LINES;

			
			if(dummyLine<0)
				dummyLine = 0;
			//spin_lock(&imx111_drv_lock);	
			//IMX111MIPI_sensor.pv_mode=TRUE;
			//spin_unlock(&imx111_drv_lock);
			IMX111MIPI_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = 120000000;
			lineLength = IMX111MIPI_VIDEO_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX111MIPI_VIDEO_FRAME_LENGTH_LINES;
			if(dummyLine<0)
				dummyLine = 0;
		//	spin_lock(&imx111_drv_lock);	
		//	IMX111MIPI_sensor.pv_mode=TRUE;
		//	spin_unlock(&imx111_drv_lock);
			IMX111MIPI_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = 134200000;
			lineLength = IMX111MIPI_FULL_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX111MIPI_FULL_FRAME_LENGTH_LINES;
			if(dummyLine<0)
				dummyLine = 0;
			
			//spin_lock(&imx111_drv_lock);	
			//IMX111MIPI_sensor.pv_mode=FALSE;
			//spin_unlock(&imx111_drv_lock);
			IMX111MIPI_SetDummy(0, dummyLine);			
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



UINT32 IMX111MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 300;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = 150;
			break;		//hhl 2-28
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




UINT32 IMX111MIPISetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[IMX111MIPISetTestPatternMode] Test pattern enable:%d\n", bEnable);
    
    if(bEnable) {   // enable color bar   
        IMX111MIPI_write_cmos_sensor(0x30D8, 0x10);  // color bar test pattern
        IMX111MIPI_write_cmos_sensor(0x0600, 0x00);  // color bar test pattern
        IMX111MIPI_write_cmos_sensor(0x0601, 0x02);  // color bar test pattern 
    } else {
        IMX111MIPI_write_cmos_sensor(0x30D8, 0x00);  // disable color bar test pattern
    }
    return TRUE;
}

UINT32 IMX111MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
        		switch(CurrentScenarioId)
        		{
        			case MSDK_SCENARIO_ID_CAMERA_ZSD:
        		    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
 		            *pFeatureReturnPara16++=IMX111MIPI_sensor.cp_line_length;  
 		            *pFeatureReturnPara16=IMX111MIPI_sensor.cp_frame_length;
		            SENSORDB("Sensor period:%d %d\n",IMX111MIPI_sensor.cp_line_length, IMX111MIPI_sensor.cp_frame_length); 
		            *pFeatureParaLen=4;        				
        				break;
        			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara16++=IMX111MIPI_sensor.video_line_length;  
					*pFeatureReturnPara16=IMX111MIPI_sensor.video_frame_length;
					 SENSORDB("Sensor period:%d %d\n", IMX111MIPI_sensor.video_line_length, IMX111MIPI_sensor.video_frame_length); 
					 *pFeatureParaLen=4;
						break;
        			default:	
					*pFeatureReturnPara16++=IMX111MIPI_sensor.pv_line_length;  
					*pFeatureReturnPara16=IMX111MIPI_sensor.pv_frame_length;
		            SENSORDB("Sensor period:%d %d\n", IMX111MIPI_sensor.pv_line_length, IMX111MIPI_sensor.pv_frame_length); 
		            *pFeatureParaLen=4;
	            break;
          	}
          	break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        		switch(CurrentScenarioId)
        		{
        			case MSDK_SCENARIO_ID_CAMERA_ZSD:
        			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		            *pFeatureReturnPara32 = IMX111MIPI_sensor.cp_pclk; //199200000
		            *pFeatureParaLen=4;		         	
					
		            SENSORDB("Sensor CPCLK:%dn",IMX111MIPI_sensor.cp_pclk); 
		         		break; //hhl 2-28
					case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
						*pFeatureReturnPara32 = IMX111MIPI_sensor.video_pclk;//199200000; 
						*pFeatureParaLen=4;
						SENSORDB("Sensor videoCLK:%d\n",IMX111MIPI_sensor.video_pclk); 
						break;
		         		default:
		            *pFeatureReturnPara32 = IMX111MIPI_sensor.pv_pclk;//134400000; //;
		            *pFeatureParaLen=4;
					SENSORDB("Sensor pvclk:%d\n",IMX111MIPI_sensor.pv_pclk); 
		            break;
		         }
		         break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            IMX111MIPI_SetShutter(*pFeatureData16);
		//	SENSORDB("shutter&gain test by hhl:IMX111MIPI_SetShutter in feature ctrl\n"); 
            break;
		case SENSOR_FEATURE_SET_SENSOR_SYNC:
		//	SENSORDB("hhl'test the function of the sync cased\n"); 
			break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            IMX111MIPI_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
           IMX111MIPI_SetGain((UINT16) *pFeatureData16);
            
		//	SENSORDB("shutter&gain test by hhl:IMX111MIPI_SetGain in feature ctrl\n"); 
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			spin_lock(&imx111_drv_lock);    
            IMX111MIPI_isp_master_clock=*pFeatureData32;
			spin_unlock(&imx111_drv_lock);
            break;
        case SENSOR_FEATURE_SET_REGISTER:
			//iWriteReg((u16) pSensorRegData->RegAddr , (u32) pSensorRegData->RegData , 1, 0x66);//to test the AF
			IMX111MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = IMX111MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&imx111_drv_lock);    
                IMX111MIPISensorCCT[i].Addr=*pFeatureData32++;
                IMX111MIPISensorCCT[i].Para=*pFeatureData32++; 
				spin_unlock(&imx111_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX111MIPISensorCCT[i].Addr;
                *pFeatureData32++=IMX111MIPISensorCCT[i].Para; 
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {	spin_lock(&imx111_drv_lock);    
                IMX111MIPISensorReg[i].Addr=*pFeatureData32++;
                IMX111MIPISensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&imx111_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX111MIPISensorReg[i].Addr;
                *pFeatureData32++=IMX111MIPISensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=IMX111MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, IMX111MIPISensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, IMX111MIPISensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &IMX111MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            IMX111MIPI_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            IMX111MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=IMX111MIPI_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            IMX111MIPI_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            IMX111MIPI_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            IMX111MIPI_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 129;
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

        case SENSOR_FEATURE_INITIALIZE_AF:
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            IMX111MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            IMX111MIPIGetSensorID(pFeatureReturnPara32); 
            break;             
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            IMX111MIPISetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));            
	        break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            IMX111MIPISetTestPatternMode((BOOL)*pFeatureData16);        	
            break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			IMX111MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			IMX111MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* IMX111MIPIFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncIMX111MIPI=
{
    IMX111MIPIOpen,
    IMX111MIPIGetInfo,
    IMX111MIPIGetResolution,
    IMX111MIPIFeatureControl,
    IMX111MIPIControl,
    IMX111MIPIClose
};

UINT32 IMX111_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncIMX111MIPI;

    return ERROR_NONE;
}   /* SensorInit() */

