/*******************************************************************************************/


/*******************************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/xlog.h>
#include <asm/atomic.h>
#include <asm/system.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k3h7ymipiraw_Sensor.h"
#include "s5k3h7ymipiraw_Camera_Sensor_para.h"
#include "s5k3h7ymipiraw_CameraCustomized.h"
static DEFINE_SPINLOCK(s5k3h7ymipiraw_drv_lock);


//#define S5K3H7Y_DEBUG_SOFIA

#define mDELAY(ms)  mdelay(ms)
#define Sleep(ms) mdelay(ms)

#define S5K3H7Y_DEBUG
#ifdef S5K3H7Y_DEBUG
	//#define S5K3H7YDB(fmt, arg...) printk( "[S5K3H7YRaw] "  fmt, ##arg)
	#define S5K3H7YDB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[S5K3H7YRaw]" fmt, #arg)
#else
	#define S5K3H7YDB(x,...)
#endif

#ifdef S5K3H7Y_DEBUG_SOFIA
	#define S5K3H7YDBSOFIA(fmt, arg...) printk( "[S5K3H7YRaw] "  fmt, ##arg)
#else
	#define S5K3H7YDBSOFIA(x,...)
#endif

kal_uint32 S5K3H7Y_FeatureControl_PERIOD_PixelNum=S5K3H7Y_PV_PERIOD_PIXEL_NUMS;
kal_uint32 S5K3H7Y_FeatureControl_PERIOD_LineNum=S5K3H7Y_PV_PERIOD_LINE_NUMS;
MSDK_SENSOR_CONFIG_STRUCT S5K3H7YSensorConfigData;

kal_uint32 S5K3H7Y_FAC_SENSOR_REG;
MSDK_SCENARIO_ID_ENUM S5K3H7YCurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;

/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT S5K3H7YSensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT S5K3H7YSensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/

static S5K3H7Y_PARA_STRUCT s5k3h7y;

extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
inline kal_uint16 S5K3H7Y_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,S5K3H7YMIPI_WRITE_ID);
	return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

inline void S5K3H7Y_wordwrite_cmos_sensor(u16 addr, u32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,  (char)(para >> 8),	(char)(para & 0xFF) };
	iWriteRegI2C(puSendCmd , 4,S5K3H7YMIPI_WRITE_ID);
}

inline void S5K3H7Y_bytewrite_cmos_sensor(u16 addr, u32 para)
{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF)  ,	(char)(para & 0xFF) };
	iWriteRegI2C(puSendCmd , 3,S5K3H7YMIPI_WRITE_ID);
}

  

void S5K3H7Y_write_shutter(kal_uint32 shutter)
{
	kal_uint32 min_framelength = S5K3H7Y_PV_PERIOD_PIXEL_NUMS, max_shutter=0;
	kal_uint32 extra_lines = 0;
	kal_uint32 line_length = 0;
	kal_uint32 frame_length = 0;
	unsigned long flags;

	S5K3H7YDBSOFIA("!!shutter=%d!!!!!\n", shutter);

	if(s5k3h7y.S5K3H7YAutoFlickerMode == KAL_TRUE)
	{
		if ( SENSOR_MODE_PREVIEW == s5k3h7y.sensorMode ) 
		{
			line_length = S5K3H7Y_PV_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels;
			max_shutter = S5K3H7Y_PV_PERIOD_LINE_NUMS + s5k3h7y.DummyLines ;
		}
		else
		{
			line_length = S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels;
			max_shutter = S5K3H7Y_FULL_PERIOD_LINE_NUMS + s5k3h7y.DummyLines ;
		}

		switch(S5K3H7YCurrentScenarioId)
		{
        	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		
				#if defined(ZSD15FPS)
				min_framelength = (s5k3h7y.capPclk*10000) /(S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels)/148*10 ;
				#else
				min_framelength = (s5k3h7y.capPclk*10000) /(S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels)/130*10 ;//13fps
				#endif
				break;
			default:
				min_framelength = (s5k3h7y.pvPclk*10000) /(S5K3H7Y_PV_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels)/296*10 ;
    			break;
		}

		S5K3H7YDBSOFIA("AutoFlickerMode!!! min_framelength for AutoFlickerMode = %d (0x%x)\n",min_framelength,min_framelength);
		S5K3H7YDBSOFIA("max framerate(10 base) autofilker = %d\n",(s5k3h7y.pvPclk*10000)*10 /line_length/min_framelength);

		if (shutter < 3)
			shutter = 3;

		if (shutter > max_shutter)
			extra_lines = shutter - max_shutter + 4;
		else
			extra_lines = 0;

		if ( SENSOR_MODE_PREVIEW == s5k3h7y.sensorMode )	
		{
			frame_length = S5K3H7Y_PV_PERIOD_LINE_NUMS+ s5k3h7y.DummyLines + extra_lines ;
		}
		else				
		{
			frame_length = S5K3H7Y_FULL_PERIOD_LINE_NUMS + s5k3h7y.DummyLines + extra_lines ;
		}
		S5K3H7YDBSOFIA("frame_length 0= %d\n",frame_length);

		if (frame_length < min_framelength)
		{
			//shutter = min_framelength - 4;

			//#if defined(MT6575)||defined(MT6577)
			switch(S5K3H7YCurrentScenarioId)
			{
        	case MSDK_SCENARIO_ID_CAMERA_ZSD:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				extra_lines = min_framelength- (S5K3H7Y_FULL_PERIOD_LINE_NUMS+ s5k3h7y.DummyLines);
				S5K3H7YDB("AutoFlickerMode!!! MSDK_SCENARIO_ID_CAMERA_ZSD  1  extra_lines!!\n");
				break;
			default:
				extra_lines = min_framelength- (S5K3H7Y_PV_PERIOD_LINE_NUMS+ s5k3h7y.DummyLines);
    			break;
			}
			//#else
			//extra_lines = min_framelength- (S5K3H7Y_PV_PERIOD_LINE_NUMS+ s5k3h7y.DummyLines);
			//#endif
			frame_length = min_framelength;
		}

		S5K3H7YDBSOFIA("frame_length 1= %d\n",frame_length);

		ASSERT(line_length < S5K3H7Y_MAX_LINE_LENGTH);		//0xCCCC
		ASSERT(frame_length < S5K3H7Y_MAX_FRAME_LENGTH); 	//0xFFFF


		spin_lock_irqsave(&s5k3h7ymipiraw_drv_lock,flags);
		s5k3h7y.maxExposureLines = frame_length;
		S5K3H7Y_FeatureControl_PERIOD_PixelNum = line_length;
		S5K3H7Y_FeatureControl_PERIOD_LineNum = frame_length;
		spin_unlock_irqrestore(&s5k3h7ymipiraw_drv_lock,flags);

		//Set shutter (Coarse integration time, uint: lines.)
		 S5K3H7Y_wordwrite_cmos_sensor(0x0202,  shutter );
		

		S5K3H7YDB("shutter=%d, extra_lines=%d, line_length=%d, frame_length=%d\n", shutter, extra_lines, line_length, frame_length);

	}
	else
	{
		if ( SENSOR_MODE_PREVIEW == s5k3h7y.sensorMode )  //(g_iS5K3H7Y_Mode == S5K3H7Y_MODE_PREVIEW)	//SXGA size output
		{
			max_shutter = S5K3H7Y_PV_PERIOD_LINE_NUMS + s5k3h7y.DummyLines ;
		}
		else
		{
			max_shutter = S5K3H7Y_FULL_PERIOD_LINE_NUMS + s5k3h7y.DummyLines ;
		}

		if (shutter < 3)
			shutter = 3;

		if (shutter > max_shutter)
			extra_lines = shutter - max_shutter + 4;
		else
			extra_lines = 0;

		if ( SENSOR_MODE_PREVIEW == s5k3h7y.sensorMode )	 
		{
			line_length = S5K3H7Y_PV_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels;
			frame_length = S5K3H7Y_PV_PERIOD_LINE_NUMS+ s5k3h7y.DummyLines + extra_lines ;
		}
		else				 
		{
			line_length = S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels;
			frame_length = S5K3H7Y_FULL_PERIOD_LINE_NUMS + s5k3h7y.DummyLines + extra_lines ;
		}

		ASSERT(line_length < S5K3H7Y_MAX_LINE_LENGTH);		//0xCCCC
		ASSERT(frame_length < S5K3H7Y_MAX_FRAME_LENGTH); 	//0xFFFF

		spin_lock_irqsave(&s5k3h7ymipiraw_drv_lock,flags);
		s5k3h7y.maxExposureLines = frame_length -4;
		S5K3H7Y_FeatureControl_PERIOD_PixelNum = line_length;
		S5K3H7Y_FeatureControl_PERIOD_LineNum = frame_length;
		spin_unlock_irqrestore(&s5k3h7ymipiraw_drv_lock,flags);

		//Set shutter (Coarse integration time, uint: lines.)
		 S5K3H7Y_wordwrite_cmos_sensor(0x0202, shutter);
		S5K3H7YDB("shutter=%d, extra_lines=%d, line_length=%d, frame_length=%d\n", shutter, extra_lines, line_length, frame_length);
	}

}   /* write_S5K3H7Y_shutter */


void write_S5K3H7Y_gain(kal_uint16 gain)
{
	  S5K3H7Y_wordwrite_cmos_sensor(0x0204,gain);
}

/*************************************************************************
* FUNCTION
*    S5K3H7Y_SetGain
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
void S5K3H7Y_SetGain(UINT16 iGain)
{
	unsigned long flags;
	spin_lock_irqsave(&s5k3h7ymipiraw_drv_lock,flags);
	s5k3h7y.sensorGain = iGain;
	spin_unlock_irqrestore(&s5k3h7ymipiraw_drv_lock,flags);

	write_S5K3H7Y_gain(s5k3h7y.sensorGain);

}


/*************************************************************************
* FUNCTION
*    read_S5K3H7Y_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 read_S5K3H7Y_gain(void)
{
	kal_uint16 read_gain=S5K3H7Y_read_cmos_sensor(0x0204);
	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.sensorGain = read_gain;
	spin_unlock(&s5k3h7ymipiraw_drv_lock);
	return s5k3h7y.sensorGain;
}  


void S5K3H7Y_camera_para_to_sensor(void)
{
  /*  kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=S5K3H7YSensorReg[i].Addr; i++)
    {
        S5K3H7Y_wordwrite_cmos_sensor(S5K3H7YSensorReg[i].Addr, S5K3H7YSensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=S5K3H7YSensorReg[i].Addr; i++)
    {
        S5K3H7Y_wordwrite_cmos_sensor(S5K3H7YSensorReg[i].Addr, S5K3H7YSensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        S5K3H7Y_wordwrite_cmos_sensor(S5K3H7YSensorCCT[i].Addr, S5K3H7YSensorCCT[i].Para);
    }*/
}


/*************************************************************************
* FUNCTION
*    S5K3H7Y_sensor_to_camera_para
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
void S5K3H7Y_sensor_to_camera_para(void)
{
/*    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=S5K3H7YSensorReg[i].Addr; i++)
    {
         temp_data = S5K3H7Y_read_cmos_sensor(S5K3H7YSensorReg[i].Addr);
		 spin_lock(&s5k3h7ymipiraw_drv_lock);
		 S5K3H7YSensorReg[i].Para =temp_data;
		 spin_unlock(&s5k3h7ymipiraw_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=S5K3H7YSensorReg[i].Addr; i++)
    {
        temp_data = S5K3H7Y_read_cmos_sensor(S5K3H7YSensorReg[i].Addr);
		spin_lock(&s5k3h7ymipiraw_drv_lock);
		S5K3H7YSensorReg[i].Para = temp_data;
		spin_unlock(&s5k3h7ymipiraw_drv_lock);
    }*/
}

/*************************************************************************
* FUNCTION
*    S5K3H7Y_get_sensor_group_count
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
kal_int32  S5K3H7Y_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void S5K3H7Y_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
{
 /*  switch (group_idx)
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
	}*/
}

void S5K3H7Y_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
/*    kal_int16 temp_reg=0;
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

            temp_para= S5K3H7YSensorCCT[temp_addr].Para;
			temp_gain= (temp_para*1000/s5k3h7y.sensorBaseGain) ;

            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min= S5K3H7Y_MIN_ANALOG_GAIN * 1000;
            info_ptr->Max= S5K3H7Y_MAX_ANALOG_GAIN * 1000;
            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");

                    //temp_reg=MT9P017SensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=    111;  //MT9P017_MAX_EXPOSURE_LINES;
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
    }*/
}



kal_bool S5K3H7Y_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
/*
   kal_uint16  temp_gain=0,temp_addr=0, temp_para=0;
   switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
				case 0:	temp_addr = PRE_GAIN_R_INDEX;		break;	
				case 1:	temp_addr = PRE_GAIN_Gr_INDEX;		break;
				case 2: temp_addr = PRE_GAIN_Gb_INDEX;		break;
				case 3: temp_addr = PRE_GAIN_B_INDEX;		break;
				case 4:	temp_addr = SENSOR_BASEGAIN;		break;
				default: ASSERT(0);
          }

			temp_gain=((ItemValue*s5k3h7y.sensorBaseGain+500)/1000);			//+500:get closed integer value

		  spin_lock(&s5k3h7ymipiraw_drv_lock);
          S5K3H7YSensorCCT[temp_addr].Para = temp_para;
		  spin_unlock(&s5k3h7ymipiraw_drv_lock);
          S5K3H7Y_wordwrite_cmos_sensor(S5K3H7YSensorCCT[temp_addr].Addr,temp_para);
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
					spin_lock(&s5k3h7ymipiraw_drv_lock);
                    S5K3H7Y_FAC_SENSOR_REG=ItemValue;
					spin_unlock(&s5k3h7ymipiraw_drv_lock);
                    break;
                case 1:
                    S5K3H7Y_wordwrite_cmos_sensor(S5K3H7Y_FAC_SENSOR_REG,ItemValue);
                    break;
                default:
                    ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }*/
    return KAL_TRUE; 
}

static void S5K3H7Y_SetDummy( const kal_uint32 iPixels, const kal_uint32 iLines )
{
	kal_uint32 line_length = 0,frame_length = 0;
	if ( SENSOR_MODE_PREVIEW == s5k3h7y.sensorMode )
	{
		line_length = S5K3H7Y_PV_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = S5K3H7Y_PV_PERIOD_LINE_NUMS + iLines;
	}
	else				//QSXGA size output
	{
		line_length = S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + iPixels;
		frame_length = S5K3H7Y_FULL_PERIOD_LINE_NUMS + iLines;
	}

	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.maxExposureLines = frame_length -4;
	S5K3H7Y_FeatureControl_PERIOD_PixelNum = line_length;
	S5K3H7Y_FeatureControl_PERIOD_LineNum = frame_length;
	spin_unlock(&s5k3h7ymipiraw_drv_lock);

	S5K3H7Y_wordwrite_cmos_sensor(0x340,frame_length);
	S5K3H7Y_wordwrite_cmos_sensor(0x342,line_length);

}   /*  S5K3H7Y_SetDummy */

void S5K3H7YCaptureSetting(void)
{
	//Full 8M
	#ifndef FullSize_preview
	S5K3H7Y_bytewrite_cmos_sensor(0x0100,0x00  ); // smiaRegs_rw_general_setup_mode_select
	S5K3H7Y_wordwrite_cmos_sensor(0x034C,0x0CC4);	// smiaRegs_rw_frame_timing_x_output_size
	S5K3H7Y_wordwrite_cmos_sensor(0x034E,0x0992);	// smiaRegs_rw_frame_timing_y_output_size
	S5K3H7Y_wordwrite_cmos_sensor(0x0344,0x0000);	// smiaRegs_rw_frame_timing_x_addr_start
	S5K3H7Y_wordwrite_cmos_sensor(0x0346,0x0000);	// smiaRegs_rw_frame_timing_y_addr_start
	S5K3H7Y_wordwrite_cmos_sensor(0x0348,0x0E60);	// smiaRegs_rw_frame_timing_x_addr_end
	S5K3H7Y_wordwrite_cmos_sensor(0x034A,0x09A8);	// smiaRegs_rw_frame_timing_y_addr_end
	S5K3H7Y_wordwrite_cmos_sensor(0x0342,0x0E60);	// smiaRegs_rw_frame_timing_line_length_pck 
	S5K3H7Y_wordwrite_cmos_sensor(0x0340,0x09A8);	// smiaRegs_rw_frame_timing_frame_length_lines 

	S5K3H7Y_wordwrite_cmos_sensor(0x0380,0x0001);	// #smiaRegs_rw_sub_sample_x_even_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0382,0x0001);	// #smiaRegs_rw_sub_sample_x_odd_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0384,0x0001);	// #smiaRegs_rw_sub_sample_y_even_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0386,0x0001);	// #smiaRegs_rw_sub_sample_y_odd_inc
	S5K3H7Y_bytewrite_cmos_sensor(0x0900,0x0000);	// #smiaRegs_rw_binning_mode
	S5K3H7Y_bytewrite_cmos_sensor(0x0901,0x0000);	// #smiaRegs_rw_binning_type
	S5K3H7Y_bytewrite_cmos_sensor(0x0902,0x0000);	// #smiaRegs_rw_binning_weighting

	S5K3H7Y_wordwrite_cmos_sensor(0x0200,0x0BEF);	// smiaRegs_rw_integration_time_fine_integration_time (fixed value)
	S5K3H7Y_wordwrite_cmos_sensor(0x0202,0x09D9);	// smiaRegs_rw_integration_time_coarse_integration_time (40ms)
	S5K3H7Y_wordwrite_cmos_sensor(0x0204,0x0020);	// X1
	S5K3H7Y_bytewrite_cmos_sensor(0x0B05,0x01  ); // #smiaRegs_rw_isp_mapped_couplet_correct_enable
	S5K3H7Y_bytewrite_cmos_sensor(0x0B00,0x00  ); // #smiaRegs_rw_isp_shading_correction_enable
	S5K3H7Y_wordwrite_cmos_sensor(0x0112,0x0A0A);	  //raw 10 foramt
	S5K3H7Y_bytewrite_cmos_sensor(0x3053,0x01);	      //line start/end short packet
	S5K3H7Y_bytewrite_cmos_sensor(0x300D,0x03);	      //pixel order B Gb Gr R
	S5K3H7Y_bytewrite_cmos_sensor(0x0100,0x01  ); // smiaRegs_rw_general_setup_mode_select
	#endif
}

static void S5K3H7Y_Sensor_Init(void)
{
	S5K3H7YDB("3h7_Sensor_Init enter :\n ");

	//=====================================================================================================================
	// * 3H7 EVT0.2 Setting
	// * This setting is only for EVT0.2
	//=====================================================================================================================
	
	//$MIPI[Width:1632,Height:1224,Format:Raw10,Lane:4,ErrorCheck:0,PolarityData:0,PolarityClock:0,Buffer:2]
		
	//=====================================================================================================================
	// * History
	//	- 120214 :	Initial draft to CML
	//	- 120215 :	Added PLL related registers (7-registers)
	//			Change delay 100ms -> 10ms
	//			Added gain control register (0x0204)
	//=====================================================================================================================


  //1600x1200
	
	S5K3H7Y_wordwrite_cmos_sensor(0x6010,0x0001);	// Reset		
	Sleep(10);//; delay(10ms)
	S5K3H7Y_wordwrite_cmos_sensor(0x6218,0xF1D0);	// open all clocks
	S5K3H7Y_wordwrite_cmos_sensor(0x6214,0xF9F0);	// open all clocks
	S5K3H7Y_wordwrite_cmos_sensor(0xF400,0x0BBC); // workaround for the SW standby current
	S5K3H7Y_wordwrite_cmos_sensor(0x6226,0x0001);	// open APB clock for I2C transaction
	S5K3H7Y_wordwrite_cmos_sensor(0xB0C0,0x000C);
	S5K3H7Y_wordwrite_cmos_sensor(0x6226,0x0000);	// close APB clock for I2C transaction
	S5K3H7Y_wordwrite_cmos_sensor(0x6218,0xF9F0);	// close all clocks
	S5K3H7Y_wordwrite_cmos_sensor(0x38FA,0x0030);  // gisp_offs_gains_bls_offs_0_
	S5K3H7Y_wordwrite_cmos_sensor(0x38FC,0x0030);  // gisp_offs_gains_bls_offs_1_
	S5K3H7Y_wordwrite_cmos_sensor(0x32CE,0x0060);	// senHal_usWidthStOfsInit		   
	S5K3H7Y_wordwrite_cmos_sensor(0x32D0,0x0024);	// senHal_usHeightStOfsInit 
	
	S5K3H7Y_wordwrite_cmos_sensor(0x0086,0x01FF);	// analogue_gain_code_max
	S5K3H7Y_wordwrite_cmos_sensor(0x0136,0x1800);	// #smiaRegs_rw_op_cond_extclk_frequency_mhz
	S5K3H7Y_wordwrite_cmos_sensor(0x0300,0x0002);	// smiaRegs_rw_clocks_vt_pix_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x0302,0x0001);	// smiaRegs_rw_clocks_vt_sys_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x0304,0x0006);	// smiaRegs_rw_clocks_pre_pll_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x0306,0x008C);	// smiaRegs_rw_clocks_pll_multiplier  
	S5K3H7Y_wordwrite_cmos_sensor(0x0308,0x0008);	// smiaRegs_rw_clocks_op_pix_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x030A,0x0001);	// smiaRegs_rw_clocks_op_sys_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x030C,0x0006);	// smiaRegs_rw_clocks_secnd_pre_pll_clk_div
	S5K3H7Y_wordwrite_cmos_sensor(0x030E,0x00A6);	// smiaRegs_rw_clocks_secnd_pll_multiplier
	S5K3H7Y_wordwrite_cmos_sensor(0x034C,0x0660);	// smiaRegs_rw_frame_timing_x_output_size
	S5K3H7Y_wordwrite_cmos_sensor(0x034E,0x04C8);	// smiaRegs_rw_frame_timing_y_output_size
	S5K3H7Y_wordwrite_cmos_sensor(0x0380,0x0001);	// #smiaRegs_rw_sub_sample_x_even_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0382,0x0003);	// #smiaRegs_rw_sub_sample_x_odd_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0384,0x0001);	// #smiaRegs_rw_sub_sample_y_even_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0386,0x0003);	// #smiaRegs_rw_sub_sample_y_odd_inc

	S5K3H7Y_wordwrite_cmos_sensor(0x0112,0x0A0A);	  //raw 10 foramt
	S5K3H7Y_bytewrite_cmos_sensor(0x3053,0x01);	      //line start/end short packet
	S5K3H7Y_bytewrite_cmos_sensor(0x300D,0x02);	      //pixel order B Gb Gr R

	S5K3H7Y_bytewrite_cmos_sensor(0x0900,0x0001);	// #smiaRegs_rw_binning_mode
	S5K3H7Y_bytewrite_cmos_sensor(0x0901,0x0022);	// #smiaRegs_rw_binning_type
	S5K3H7Y_bytewrite_cmos_sensor(0x0902,0x0001);	// #smiaRegs_rw_binning_weighting
	S5K3H7Y_wordwrite_cmos_sensor(0x0342,0x0E68);	// smiaRegs_rw_frame_timing_line_length_pck
	S5K3H7Y_wordwrite_cmos_sensor(0x0340,0x0A0F);	// smiaRegs_rw_frame_timing_frame_length_lines
	S5K3H7Y_wordwrite_cmos_sensor(0x0200,0x0618);	  // smiaRegs_rw_integration_time_fine_integration_time
	S5K3H7Y_wordwrite_cmos_sensor(0x0202,0x04BC);	  // smiaRegs_rw_integration_time_coarse_integration_time

	S5K3H7Y_bytewrite_cmos_sensor(0x37F8,0x0001);	  // Analog Gain Precision, 0/1/2/3 = 32/64/128/256 base 1X, set 1=> 64 =1X
	s5k3h7y.sensorBaseGain=64;

	S5K3H7Y_wordwrite_cmos_sensor(0x0204,0x0020);	// X1
	S5K3H7Y_bytewrite_cmos_sensor(0x0B05,0x0001);	  // #smiaRegs_rw_isp_mapped_couplet_correct_enable
	S5K3H7Y_bytewrite_cmos_sensor(0x0B00,0x0000);	  // #smiaRegs_rw_isp_shading_correction_enable
	S5K3H7Y_bytewrite_cmos_sensor(0x0100,0x0001);	  // smiaRegs_rw_general_setup_mode_select



 
	#ifdef FullSize_preview    //open for Full 8M preview.
	S5K3H7Y_bytewrite_cmos_sensor(0x0100,0x00  ); // smiaRegs_rw_general_setup_mode_select
	S5K3H7Y_wordwrite_cmos_sensor(0x034C,0x0CC4);	// smiaRegs_rw_frame_timing_x_output_size
	S5K3H7Y_wordwrite_cmos_sensor(0x034E,0x0992);	// smiaRegs_rw_frame_timing_y_output_size
	S5K3H7Y_wordwrite_cmos_sensor(0x0344,0x0000);	// smiaRegs_rw_frame_timing_x_addr_start
	S5K3H7Y_wordwrite_cmos_sensor(0x0346,0x0000);	// smiaRegs_rw_frame_timing_y_addr_start
	S5K3H7Y_wordwrite_cmos_sensor(0x0348,0x0E60);	// smiaRegs_rw_frame_timing_x_addr_end
	S5K3H7Y_wordwrite_cmos_sensor(0x034A,0x09A8);	// smiaRegs_rw_frame_timing_y_addr_end
	S5K3H7Y_wordwrite_cmos_sensor(0x0342,0x0E60);	// smiaRegs_rw_frame_timing_line_length_pck 
	S5K3H7Y_wordwrite_cmos_sensor(0x0340,0x09A8);	// smiaRegs_rw_frame_timing_frame_length_lines 

	S5K3H7Y_wordwrite_cmos_sensor(0x0380,0x0001);	// #smiaRegs_rw_sub_sample_x_even_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0382,0x0001);	// #smiaRegs_rw_sub_sample_x_odd_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0384,0x0001);	// #smiaRegs_rw_sub_sample_y_even_inc
	S5K3H7Y_wordwrite_cmos_sensor(0x0386,0x0001);	// #smiaRegs_rw_sub_sample_y_odd_inc
	S5K3H7Y_bytewrite_cmos_sensor(0x0900,0x0000);	// #smiaRegs_rw_binning_mode
	S5K3H7Y_bytewrite_cmos_sensor(0x0901,0x0000);	// #smiaRegs_rw_binning_type
	S5K3H7Y_bytewrite_cmos_sensor(0x0902,0x0000);	// #smiaRegs_rw_binning_weighting

	S5K3H7Y_wordwrite_cmos_sensor(0x0200,0x0BEF);	// smiaRegs_rw_integration_time_fine_integration_time (fixed value)
	S5K3H7Y_wordwrite_cmos_sensor(0x0202,0x09D9);	// smiaRegs_rw_integration_time_coarse_integration_time (40ms)
	S5K3H7Y_wordwrite_cmos_sensor(0x0204,0x0020);	// X1
	S5K3H7Y_bytewrite_cmos_sensor(0x0B05,0x01  ); // #smiaRegs_rw_isp_mapped_couplet_correct_enable
	S5K3H7Y_bytewrite_cmos_sensor(0x0B00,0x00  ); // #smiaRegs_rw_isp_shading_correction_enable
	S5K3H7Y_wordwrite_cmos_sensor(0x0112,0x0A0A);	  //raw 10 foramt
	S5K3H7Y_bytewrite_cmos_sensor(0x3053,0x01);	      //line start/end short packet
	S5K3H7Y_bytewrite_cmos_sensor(0x300D,0x02);	      //pixel order B Gb Gr R
	S5K3H7Y_bytewrite_cmos_sensor(0x0100,0x01  ); // smiaRegs_rw_general_setup_mode_select
	#endif

	#if 0  // test pattern = Color Checker
    S5K3H7Y_wordwrite_cmos_sensor(0x0600,0x0100);	
	#endif


    S5K3H7YDB("S5K3H7Y_Sensor_Init exit :\n ");

}   /*  S5K3H7Y_Sensor_Init  */

/*************************************************************************
* FUNCTION
*   S5K3H7YOpen
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

UINT32 S5K3H7YOpen(void)
{

	volatile signed int i;
	kal_uint16 sensor_id = 0;

	S5K3H7YDB("S5K3H7YOpen enter :\n ");

	//  Read sensor ID to adjust I2C is OK?
	for(i=0;i<3;i++)
	{
		sensor_id = S5K3H7Y_read_cmos_sensor(0x0000);
		S5K3H7YDB("OS5K3H7Y READ ID :%x",sensor_id);
		if(sensor_id != S5K3H7Y_SENSOR_ID)
			return ERROR_SENSOR_CONNECT_FAIL;
		else	break;
	}
	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.sensorMode = SENSOR_MODE_INIT;
	s5k3h7y.S5K3H7YAutoFlickerMode = KAL_FALSE;
	s5k3h7y.S5K3H7YVideoMode = KAL_FALSE;
	spin_unlock(&s5k3h7ymipiraw_drv_lock);

	S5K3H7Y_Sensor_Init();

	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.DummyLines= 0;
	s5k3h7y.DummyPixels= 0;

	s5k3h7y.pvPclk =  (13867);
	spin_unlock(&s5k3h7ymipiraw_drv_lock);

    	switch(S5K3H7YCurrentScenarioId)
		{
			case MSDK_SCENARIO_ID_CAMERA_ZSD:
				#if defined(ZSD15FPS)
				spin_lock(&s5k3h7ymipiraw_drv_lock);
				s5k3h7y.capPclk = (13867);//15fps
				spin_unlock(&s5k3h7ymipiraw_drv_lock);
				#else
				spin_lock(&s5k3h7ymipiraw_drv_lock);
				s5k3h7y.capPclk = (13867);//13fps////////////need check
				spin_unlock(&s5k3h7ymipiraw_drv_lock);
				#endif
				break;
        	default:
				spin_lock(&s5k3h7ymipiraw_drv_lock);
				s5k3h7y.capPclk = (13867);
				spin_unlock(&s5k3h7ymipiraw_drv_lock);
				break;
          }

	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.shutter = 0x4EA;
	s5k3h7y.pvShutter = 0x4EA;
	s5k3h7y.maxExposureLines =S5K3H7Y_PV_PERIOD_LINE_NUMS -4;

	s5k3h7y.sensorGain = 0x1f;//sensor gain read from 0x350a 0x350b; 0x1f as 3.875x
	s5k3h7y.pvGain = 0x1f;

	spin_unlock(&s5k3h7ymipiraw_drv_lock);
	//S5K3H7YDB("S5K3H7YReg2Gain(0x1f)=%x :\n ",S5K3H7YReg2Gain(0x1f));

	S5K3H7YDB("S5K3H7YOpen exit :\n ");
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   S5K3H7YGetSensorID
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
UINT32 S5K3H7YGetSensorID(UINT32 *sensorID)
{
    int  retry = 1;

	S5K3H7YDB("S5K3H7YGetSensorID enter :\n ");
    do {
		S5K3H7Y_wordwrite_cmos_sensor(0x6010,0x0001);	// Reset		
    	mDELAY(10);
        *sensorID = S5K3H7Y_read_cmos_sensor(0x0000);
		S5K3H7YDB("S5K3H7Y_read Sensor ID = 0x%04x\n", *sensorID);
		S5K3H7YDB("S5K3H7Y_read Sensor rev = 0x%04x\n",S5K3H7Y_read_cmos_sensor(0x0002));
		S5K3H7YDB("S5K3H7Y_read Sensor manufaid = 0x%04x\n",S5K3H7Y_read_cmos_sensor(0x0003));

        if (*sensorID == S5K3H7Y_SENSOR_ID)
        	{
        		S5K3H7YDB("Sensor ID = 0x%04x\n", *sensorID);
            	break;
        	}
        S5K3H7YDB("Read Sensor ID Fail = 0x%04x\n", *sensorID);
        retry--;
    } while (retry > 0);

	if (*sensorID != S5K3H7Y_SENSOR_ID)
	{
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   S5K3H7Y_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of S5K3H7Y to change exposure time.
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
void S5K3H7Y_SetShutter(kal_uint32 iShutter)
{

	if(s5k3h7y.shutter == iShutter)
		return;
   spin_lock(&s5k3h7ymipiraw_drv_lock);
   s5k3h7y.shutter= iShutter;
   spin_unlock(&s5k3h7ymipiraw_drv_lock);
   S5K3H7Y_write_shutter(iShutter);

}   /*  S5K3H7Y_SetShutter   */



/*************************************************************************
* FUNCTION
*   S5K3H7Y_read_shutter
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
UINT32 S5K3H7Y_read_shutter(void)
{
	return S5K3H7Y_read_cmos_sensor(0x0202);   // smiaRegs_rw_integration_time_coarse_integration_time 
	
}

/*************************************************************************
* FUNCTION
*   S5K3H7Y_night_mode
*
* DESCRIPTION
*   This function night mode of S5K3H7Y.
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
void S5K3H7Y_NightMode(kal_bool bEnable)
{
}/*	S5K3H7Y_NightMode */



/*************************************************************************
* FUNCTION
*   S5K3H7YClose
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
UINT32 S5K3H7YClose(void)
{
    //  CISModulePowerOn(FALSE);
    //s_porting
    //  DRV_I2CClose(S5K3H7YhDrvI2C);
    //e_porting
    return ERROR_NONE;
}	/* S5K3H7YClose() */

void S5K3H7YSetFlipMirror(kal_int32 imgMirror)
{
    switch (imgMirror)
    {
        case IMAGE_H_MIRROR://IMAGE_NORMAL:  bit0 mirror,   bit1 flip.
			S5K3H7Y_bytewrite_cmos_sensor(0x0101,0x01  ); //morror
            break;
        case IMAGE_NORMAL://IMAGE_V_MIRROR:
			S5K3H7Y_bytewrite_cmos_sensor(0x0101,0x00  ); 
            break;
        case IMAGE_HV_MIRROR://IMAGE_H_MIRROR:
			S5K3H7Y_bytewrite_cmos_sensor(0x0101,0x03  );   //morror +flip
            break;
        case IMAGE_V_MIRROR://IMAGE_HV_MIRROR:
			S5K3H7Y_bytewrite_cmos_sensor(0x0101,0x02  ); //flip
            break;
    }
}


/*************************************************************************
* FUNCTION
*   S5K3H7YPreview
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
UINT32 S5K3H7YPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	S5K3H7YDB("S5K3H7YPreview enter:");

	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.sensorMode = SENSOR_MODE_PREVIEW; // Need set preview setting after capture mode
	S5K3H7Y_FeatureControl_PERIOD_PixelNum=S5K3H7Y_PV_PERIOD_PIXEL_NUMS+ s5k3h7y.DummyPixels;
	S5K3H7Y_FeatureControl_PERIOD_LineNum=S5K3H7Y_PV_PERIOD_LINE_NUMS+s5k3h7y.DummyLines;
	spin_unlock(&s5k3h7ymipiraw_drv_lock);

	S5K3H7Y_write_shutter(s5k3h7y.shutter);
	write_S5K3H7Y_gain(s5k3h7y.pvGain);

	//set mirror & flip
	S5K3H7YDB("[S5K3H7YPreview] mirror&flip: %d \n",sensor_config_data->SensorImageMirror);
	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.imgMirror = sensor_config_data->SensorImageMirror;
	spin_unlock(&s5k3h7ymipiraw_drv_lock);
	S5K3H7YSetFlipMirror(sensor_config_data->SensorImageMirror);

	S5K3H7YDB("S5K3H7YPreview exit:\n");
    return ERROR_NONE;
}	/* S5K3H7YPreview() */

UINT32 S5K3H7YCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
 	kal_uint32 shutter = s5k3h7y.shutter;
	kal_uint32 temp_data ;

	if( SENSOR_MODE_CAPTURE== s5k3h7y.sensorMode)
		S5K3H7YDB("S5K3H7YCapture BusrtShot!!!\n");
	else
	{
	S5K3H7YDB("S5K3H7YCapture enter:\n");

	//Record Preview shutter & gain
	shutter=S5K3H7Y_read_shutter();
	temp_data =  read_S5K3H7Y_gain();
	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.pvShutter =shutter;
		s5k3h7y.sensorGain = temp_data;
		s5k3h7y.pvGain =s5k3h7y.sensorGain;
	spin_unlock(&s5k3h7ymipiraw_drv_lock);

		S5K3H7YDB("[S5K3H7YCapture]s5k3h7y.shutter=%d, read_pv_shutter=%d, read_pv_gain = 0x%x\n",s5k3h7y.shutter, shutter,s5k3h7y.sensorGain);

	// Full size setting
	S5K3H7YCaptureSetting();

	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.sensorMode = SENSOR_MODE_CAPTURE;
	s5k3h7y.imgMirror = sensor_config_data->SensorImageMirror;

	S5K3H7Y_FeatureControl_PERIOD_PixelNum = S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels;
	S5K3H7Y_FeatureControl_PERIOD_LineNum = S5K3H7Y_FULL_PERIOD_LINE_NUMS + s5k3h7y.DummyLines;

	spin_unlock(&s5k3h7ymipiraw_drv_lock);

	//S5K3H7YDB("[S5K3H7YCapture] mirror&flip: %d\n",sensor_config_data->SensorImageMirror);
	S5K3H7YSetFlipMirror(sensor_config_data->SensorImageMirror);

	//#if defined(MT6575)||defined(MT6577)
    if(S5K3H7YCurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_ZSD)
    {
		S5K3H7YDB("S5K3H7YCapture exit ZSD!!\n");
		return ERROR_NONE;
    }
	//#endif

	#if 0 //no need to calculate shutter from mt6589
	//calculate shutter
	pv_line_length = S5K3H7Y_PV_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels;
	cap_line_length = S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels;

	S5K3H7YDB("[S5K3H7YCapture]pv_line_length =%d,cap_line_length =%d\n",pv_line_length,cap_line_length);
	S5K3H7YDB("[S5K3H7YCapture]pv_shutter =%d\n",shutter );

	shutter =  shutter * pv_line_length / cap_line_length;
	shutter = shutter *s5k3h7y.capPclk / s5k3h7y.pvPclk;
	shutter *= 2; //preview bining///////////////////////////////////////

	if(shutter < 3)
	    shutter = 3;

	S5K3H7Y_write_shutter(shutter);

	//gain = read_S5K3H7Y_gain();

	S5K3H7YDB("[S5K3H7YCapture]cap_shutter =%d , cap_read gain = 0x%x\n",shutter,read_S5K3H7Y_gain());
		//write_S5K3H7Y_gain(s5k3h7y.sensorGain);
   #endif

	S5K3H7YDB("S5K3H7YCapture exit:\n");
	}

    return ERROR_NONE;
}	/* S5K3H7YCapture() */

UINT32 S5K3H7YGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    S5K3H7YDB("S5K3H7YGetResolution!!\n");
		switch(S5K3H7YCurrentScenarioId)
		{
        	case MSDK_SCENARIO_ID_CAMERA_ZSD:
				pSensorResolution->SensorPreviewWidth	= S5K3H7Y_IMAGE_SENSOR_FULL_WIDTH;
    			pSensorResolution->SensorPreviewHeight	= S5K3H7Y_IMAGE_SENSOR_FULL_HEIGHT;
				break;
			default:
				pSensorResolution->SensorPreviewWidth	= S5K3H7Y_IMAGE_SENSOR_PV_WIDTH;
    			pSensorResolution->SensorPreviewHeight	= S5K3H7Y_IMAGE_SENSOR_PV_HEIGHT;
			pSensorResolution->SensorVideoWidth	= pSensorResolution->SensorPreviewWidth;
			pSensorResolution->SensorVideoHeight =pSensorResolution->SensorPreviewHeight;
    			break;
		}
    pSensorResolution->SensorFullWidth		= S5K3H7Y_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight		= S5K3H7Y_IMAGE_SENSOR_FULL_HEIGHT;
    return ERROR_NONE;
}   /* S5K3H7YGetResolution() */

UINT32 S5K3H7YGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		switch(S5K3H7YCurrentScenarioId)
		{
        	case MSDK_SCENARIO_ID_CAMERA_ZSD:
				pSensorInfo->SensorPreviewResolutionX= S5K3H7Y_IMAGE_SENSOR_FULL_WIDTH;
    			pSensorInfo->SensorPreviewResolutionY= S5K3H7Y_IMAGE_SENSOR_FULL_HEIGHT;
				break;
			default:
				pSensorInfo->SensorPreviewResolutionX= S5K3H7Y_IMAGE_SENSOR_PV_WIDTH;
    			pSensorInfo->SensorPreviewResolutionY= S5K3H7Y_IMAGE_SENSOR_PV_HEIGHT;
				break;
		}

	pSensorInfo->SensorFullResolutionX= S5K3H7Y_IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY= S5K3H7Y_IMAGE_SENSOR_FULL_HEIGHT;

	spin_lock(&s5k3h7ymipiraw_drv_lock);
	s5k3h7y.imgMirror = pSensorConfigData->SensorImageMirror ;
	spin_unlock(&s5k3h7ymipiraw_drv_lock);
	S5K3H7YDB("[S5K3H7YGetInfo]SensorImageMirror:%d\n", s5k3h7y.imgMirror );


	if(s5k3h7y.imgMirror==IMAGE_NORMAL)
   		pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_Gr;
	if(s5k3h7y.imgMirror==IMAGE_H_MIRROR)
   		pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_R;
	if(s5k3h7y.imgMirror==IMAGE_V_MIRROR)
   		pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_B;
	if(s5k3h7y.imgMirror==IMAGE_HV_MIRROR)
   		pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_Gb;


    pSensorInfo->SensorClockPolarity =SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;

    pSensorInfo->CaptureDelayFrame = 1;
    pSensorInfo->PreviewDelayFrame = 1;
    pSensorInfo->VideoDelayFrame = 2;

    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;
    pSensorInfo->AEShutDelayFrame = 0;//0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0 ;//0;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 1;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorGrabStartX = S5K3H7Y_PV_X_START;
            pSensorInfo->SensorGrabStartY = S5K3H7Y_PV_Y_START;
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = S5K3H7Y_FULL_X_START;	//2*S5K3H7Y_IMAGE_SENSOR_PV_STARTX;
            pSensorInfo->SensorGrabStartY = S5K3H7Y_FULL_Y_START;	//2*S5K3H7Y_IMAGE_SENSOR_PV_STARTY;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
			pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = S5K3H7Y_PV_X_START;
            pSensorInfo->SensorGrabStartY = S5K3H7Y_PV_Y_START;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
    }

    memcpy(pSensorConfigData, &S5K3H7YSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* S5K3H7YGetInfo() */


UINT32 S5K3H7YControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		spin_lock(&s5k3h7ymipiraw_drv_lock);
		S5K3H7YCurrentScenarioId = ScenarioId;
		spin_unlock(&s5k3h7ymipiraw_drv_lock);
		//S5K3H7YDB("ScenarioId=%d\n",ScenarioId);
		S5K3H7YDB("S5K3H7YCurrentScenarioId=%d\n",S5K3H7YCurrentScenarioId);
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            S5K3H7YPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            S5K3H7YCapture(pImageWindow, pSensorConfigData);
            break;

        default:
            return ERROR_INVALID_SCENARIO_ID;

    }
    return ERROR_NONE;
} /* S5K3H7YControl() */


UINT32 S5K3H7YSetVideoMode(UINT16 u2FrameRate)
{

    kal_uint32 MIN_Frame_length =0,frameRate=0,extralines=0;
    S5K3H7YDB("[S5K3H7YSetVideoMode] frame rate = %d\n", u2FrameRate);

	if(u2FrameRate==0)
	{
		S5K3H7YDB("Disable Video Mode or dynimac fps\n");
		return ERROR_NONE;
	}
	if(u2FrameRate >30 || u2FrameRate <5)
	    S5K3H7YDB("error frame rate seting\n");

    if(s5k3h7y.sensorMode == SENSOR_MODE_PREVIEW)
    {
    	if(s5k3h7y.S5K3H7YAutoFlickerMode == KAL_TRUE)
    	{
    		if (u2FrameRate==30||u2FrameRate==24)	frameRate= 296;
			else									frameRate= 148;
			MIN_Frame_length = (s5k3h7y.pvPclk*10000)/(S5K3H7Y_PV_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels)/frameRate*10;
    	}
		else
			MIN_Frame_length = (s5k3h7y.pvPclk*10000) /(S5K3H7Y_PV_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels)/u2FrameRate;

		if((MIN_Frame_length <=S5K3H7Y_PV_PERIOD_LINE_NUMS))
		{
			MIN_Frame_length = S5K3H7Y_PV_PERIOD_LINE_NUMS;
			S5K3H7YDB("[S5K3H7YSetVideoMode]current fps = %d\n", (s5k3h7y.pvPclk*10000)  /(S5K3H7Y_PV_PERIOD_PIXEL_NUMS)/S5K3H7Y_PV_PERIOD_LINE_NUMS);
		}
		S5K3H7YDB("[S5K3H7YSetVideoMode]current fps (10 base)= %d\n", (s5k3h7y.pvPclk*10000)*10/(S5K3H7Y_PV_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels)/MIN_Frame_length);
		extralines = MIN_Frame_length - S5K3H7Y_PV_PERIOD_LINE_NUMS;

		S5K3H7Y_SetDummy(s5k3h7y.DummyPixels,extralines);
    }
	else if(s5k3h7y.sensorMode == SENSOR_MODE_CAPTURE)
	{
		S5K3H7YDB("-------[S5K3H7YSetVideoMode]ZSD???---------\n");
		if(s5k3h7y.S5K3H7YAutoFlickerMode == KAL_TRUE)
    	{
			#if defined(ZSD15FPS)
			frameRate= 148;//For ZSD	 mode//15fps
			#else
			frameRate= 130;//For ZSD	 mode	13fps
			#endif
			MIN_Frame_length = (s5k3h7y.pvPclk*10000) /(S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels)/frameRate*10;
    	}
		else
			MIN_Frame_length = (s5k3h7y.capPclk*10000) /(S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels)/u2FrameRate;

		if((MIN_Frame_length <=S5K3H7Y_FULL_PERIOD_LINE_NUMS))
		{
			MIN_Frame_length = S5K3H7Y_FULL_PERIOD_LINE_NUMS;
			S5K3H7YDB("[S5K3H7YSetVideoMode]current fps = %d\n", (s5k3h7y.capPclk*10000) /(S5K3H7Y_FULL_PERIOD_PIXEL_NUMS)/S5K3H7Y_FULL_PERIOD_LINE_NUMS);
		}
		S5K3H7YDB("[S5K3H7YSetVideoMode]current fps (10 base)= %d\n", (s5k3h7y.pvPclk*10000)*10/(S5K3H7Y_FULL_PERIOD_PIXEL_NUMS + s5k3h7y.DummyPixels)/MIN_Frame_length);

		extralines = MIN_Frame_length - S5K3H7Y_FULL_PERIOD_LINE_NUMS;

		S5K3H7Y_SetDummy(s5k3h7y.DummyPixels,extralines);
	}
	S5K3H7YDB("[S5K3H7YSetVideoMode]MIN_Frame_length=%d,s5k3h7y.DummyLines=%d\n",MIN_Frame_length,s5k3h7y.DummyLines);

    return ERROR_NONE;
}

UINT32 S5K3H7YSetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
    S5K3H7YDB("[S5K3H7YSetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
	if(bEnable) {   // enable auto flicker
		spin_lock(&s5k3h7ymipiraw_drv_lock);
		s5k3h7y.S5K3H7YAutoFlickerMode = KAL_TRUE;
		spin_unlock(&s5k3h7ymipiraw_drv_lock);
    } else {
    	spin_lock(&s5k3h7ymipiraw_drv_lock);
        s5k3h7y.S5K3H7YAutoFlickerMode = KAL_FALSE;
		spin_unlock(&s5k3h7ymipiraw_drv_lock);
    }
    return ERROR_NONE;
}

UINT32 S5K3H7YSetTestPatternMode(kal_bool bEnable)
{
    S5K3H7YDB("[S5K3H7YSetTestPatternMode] Test pattern enable:%d\n", bEnable);
	if(bEnable) S5K3H7Y_wordwrite_cmos_sensor(0x0600,0x0100);	
	else        S5K3H7Y_wordwrite_cmos_sensor(0x0600,0x0000);	
    return ERROR_NONE;
}

UINT32 S5K3H7YMIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;
		
	S5K3H7YDB("S5K3H7YMIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = 138670000;
			lineLength = S5K3H7Y_PV_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - S5K3H7Y_PV_PERIOD_LINE_NUMS;
			s5k3h7y.sensorMode = SENSOR_MODE_PREVIEW; 
			S5K3H7Y_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = 138670000;
			lineLength = S5K3H7Y_PV_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - S5K3H7Y_PV_PERIOD_LINE_NUMS;
			s5k3h7y.sensorMode = SENSOR_MODE_PREVIEW;
			S5K3H7Y_SetDummy(0, dummyLine);			
			break;			
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = 138670000;
			lineLength = S5K3H7Y_FULL_PERIOD_PIXEL_NUMS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - S5K3H7Y_FULL_PERIOD_LINE_NUMS;
			s5k3h7y.sensorMode = SENSOR_MODE_CAPTURE;
			S5K3H7Y_SetDummy(0, dummyLine);			
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


UINT32 S5K3H7YMIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = 300;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = 300;
			break;		
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

UINT32 S5K3H7YFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++= S5K3H7Y_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16= S5K3H7Y_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
				*pFeatureReturnPara16++= S5K3H7Y_FeatureControl_PERIOD_PixelNum;
				*pFeatureReturnPara16= S5K3H7Y_FeatureControl_PERIOD_LineNum;
				*pFeatureParaLen=4;
				break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			//same pclk for preview/capture
    	 	*pFeatureReturnPara32 = 138670000;
    	 	*pFeatureParaLen=4;
 			 break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            S5K3H7Y_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            S5K3H7Y_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            S5K3H7Y_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            //S5K3H7Y_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            S5K3H7Y_wordwrite_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = S5K3H7Y_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&s5k3h7ymipiraw_drv_lock);
                S5K3H7YSensorCCT[i].Addr=*pFeatureData32++;
                S5K3H7YSensorCCT[i].Para=*pFeatureData32++;
				spin_unlock(&s5k3h7ymipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return ERROR_INVALID_PARA;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=S5K3H7YSensorCCT[i].Addr;
                *pFeatureData32++=S5K3H7YSensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
            	spin_lock(&s5k3h7ymipiraw_drv_lock);
                S5K3H7YSensorReg[i].Addr=*pFeatureData32++;
                S5K3H7YSensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&s5k3h7ymipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return ERROR_INVALID_PARA;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=S5K3H7YSensorReg[i].Addr;
                *pFeatureData32++=S5K3H7YSensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=S5K3H7Y_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, S5K3H7YSensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, S5K3H7YSensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return ERROR_INVALID_PARA;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &S5K3H7YSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            S5K3H7Y_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            S5K3H7Y_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=S5K3H7Y_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            S5K3H7Y_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            S5K3H7Y_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            S5K3H7Y_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 129;
            pSensorEngInfo->SensorType = CMOS_SENSOR;
            pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_Gr;
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
            S5K3H7YSetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            S5K3H7YGetSensorID(pFeatureReturnPara32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            S5K3H7YSetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));
	        break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            S5K3H7YSetTestPatternMode((BOOL)*pFeatureData16);
            break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			S5K3H7YMIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			S5K3H7YMIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;

        default:
            break;
    }
    return ERROR_NONE;
}	/* S5K3H7YFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncS5K3H7Y=
{
    S5K3H7YOpen,
    S5K3H7YGetInfo,
    S5K3H7YGetResolution,
    S5K3H7YFeatureControl,
    S5K3H7YControl,
    S5K3H7YClose
};

UINT32 S5K3H7Y_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncS5K3H7Y;

    return ERROR_NONE;
}   /* SensorInit() */


