/*****************************************************************************
 *
 * Filename:
 * ---------
 *   Sensor.c
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
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

#include "ov3640_Sensor.h"
#include "ov3640_Camera_Sensor_para.h"
#include "ov3640_CameraCustomized.h"

#define OV3640_DEBUG
#ifdef OV3640_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define OV3640_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para ,1,OV3640_WRITE_ID)
#define OV3640_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,OV3640_WRITE_ID)
kal_uint16 OV3640_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV3640_WRITE_ID);
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

//#include "kal_release.h"
//#include "i2c_exp.h"
//#include "gpio_exp.h"
//#include "msdk_exp.h"
//#include "msdk_sensor_exp.h"
//#include "msdk_isp_exp.h"
//#include "base_regs.h"
//#include "ov3640_Sensor.h"
//#include "ov3640_camera_sensor_para.h"
//#include "CameraCustomized.h"

static kal_uint16 OV3640_g_iBackupExtraExp = 0;

//#define __OV_CMX_RAW8__
#define __OV_CIP_RAW8__
#define __DENOISE_MANUAL__

kal_uint32 OV3640_isp_master_clock;


static kal_bool OV3640_g_bXGA_Mode = KAL_TRUE;
static kal_uint16 OV3640_g_iExpLines = 0,OV3640_g_iExtra_ExpLines = 0;
OV3640_OP_TYPE g_iOV3640_Mode = OV3640_MODE_NONE;
kal_uint8 OV3640_MIN_EXPOSURE_LINES = 1;
kal_uint16 OV3640_MAX_EXPOSURE_LINES = PV_EXPOSURE_LIMITATION;
kal_uint16 OV3640_g_iDummyLines = 0,OV3640_iDummyPixels=0;

kal_uint16 OV3640_g_iPV_PCLK_Divider = 0;
kal_uint16 OV3640_g_iPV_Pixels_Per_Line = 0;

 kal_uint32 OV3640_g_fPV_PCLK = 28166666;
//static float OV3640_g_fPV_PCLK =  47666666;


kal_bool OV3640_MPEG4_encode_mode = KAL_FALSE;
kal_uint32 OV3640_FAC_SENSOR_REG;
kal_uint8 OV3640_sensor_write_I2C_address = OV3640_WRITE_ID;
kal_uint8 OV3640_sensor_read_I2C_address = OV3640_READ_ID;

//HANDLE OV3640hDrvI2C;
//I2C_TRANSACTION OV3640I2CConfig;

UINT8 OV3640PixelClockDivider=0;

SENSOR_REG_STRUCT OV3640SensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT OV3640SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;

MSDK_SENSOR_CONFIG_STRUCT OV3640SensorConfigData;

//static void OV3640_write_cmos_sensor(const kal_uint32 addr, const kal_uint32 para)
//{    
//      OV3640I2CConfig.operation=I2C_OP_WRITE;
//	  OV3640I2CConfig.slaveAddr=OV3640_sensor_write_I2C_address>>1;
//	  OV3640I2CConfig.transfer_num=1;	/* TRANSAC_LEN */
//	  OV3640I2CConfig.transfer_len=3;
//	  OV3640I2CConfig.buffer[0]=(UINT8)(addr>>8);
//	  OV3640I2CConfig.buffer[1]=(UINT8)(addr&0xFF);
//	  OV3640I2CConfig.buffer[2]=(UINT8)para;
//	  DRV_I2CTransaction(OV3640hDrvI2C, &OV3640I2CConfig);
//}   /*  OV3640_OV3640_write_cmos_sensor()  */

//static kal_uint32 OV3640_read_cmos_sensor(const kal_uint32 addr)
//{   
//    kal_uint8 iGetByte = 0;
//         OV3640I2CConfig.operation=I2C_OP_WRITE;
//	     OV3640I2CConfig.slaveAddr=OV3640_sensor_write_I2C_address>>1;
//	     OV3640I2CConfig.transfer_num=1;	/* TRANSAC_LEN */
//	     OV3640I2CConfig.transfer_len=2;
//	     OV3640I2CConfig.buffer[0]=(UINT8)(addr>>8);
//	     OV3640I2CConfig.buffer[1]=(UINT8)(addr&0xFF);
//	     DRV_I2CTransaction(OV3640hDrvI2C, &OV3640I2CConfig);

//	     OV3640I2CConfig.operation=I2C_OP_READ;
//	     OV3640I2CConfig.slaveAddr=OV3640_sensor_read_I2C_address>>1;
//	     OV3640I2CConfig.transfer_num=1;	/* TRANSAC_LEN */
//	     OV3640I2CConfig.transfer_len=1;
//	     DRV_I2CTransaction(OV3640hDrvI2C, &OV3640I2CConfig);
//	     iGetByte=OV3640I2CConfig.buffer[0];
//    return iGetByte;
//}   /*  OV3640_read_cmos_sensor()  */

static void OV3640_Write_Shutter(const kal_uint16 iShutter)
{
    kal_uint16 iExp = iShutter;

    if(OV3640_MPEG4_encode_mode == KAL_TRUE)
    {
        OV3640_write_cmos_sensor(0x3002, iExp >> 8);
        OV3640_write_cmos_sensor(0x3003, iExp & 0x00FF); 
    }
    else
    {
        if (OV3640_g_bXGA_Mode) {
            if (iExp <= PV_EXPOSURE_LIMITATION + OV3640_g_iDummyLines) {
                OV3640_g_iExtra_ExpLines = 0;
            }else {
                OV3640_g_iExtra_ExpLines = iExp - PV_EXPOSURE_LIMITATION - OV3640_g_iDummyLines;
			iExp = PV_EXPOSURE_LIMITATION + OV3640_g_iDummyLines;
            }
        }else {
            if (iExp <= FULL_EXPOSURE_LIMITATION + OV3640_g_iDummyLines) {
                OV3640_g_iExtra_ExpLines = 0;
            }else {
                OV3640_g_iExtra_ExpLines = iExp - FULL_EXPOSURE_LIMITATION - OV3640_g_iDummyLines;
			iExp = FULL_EXPOSURE_LIMITATION + OV3640_g_iDummyLines;
            }
        }
        OV3640_write_cmos_sensor(0x3002, iExp >> 8);
        OV3640_write_cmos_sensor(0x3003, iExp & 0x00FF);
	//RETAILMSG(1, (TEXT("Camera reg 0x3002= %x ,0x3003=%x \r\n"),OV3640_read_cmos_sensor(0x3002),OV3640_read_cmos_sensor(0x3003)));


    // 0x302D, 0x302E will increase VBLANK to get exposure larger than frame exposure
    // limitation. 0x302D, 0x302E must update at the same frame as sensor gain update.
    // AE doesn't update sensor gain at capture mode, thus extra exposure lines must be
    // updated here.

  // if ((g_iBackupExtraExp == 0 && OV3640_g_iExtra_ExpLines > 0) ||
   //     g_iOV3640_Mode == OV3640_MODE_CAPTURE ||
   //     aeIsEnable() == KAL_FALSE) {
        if ((OV3640_g_iBackupExtraExp == 0 && OV3640_g_iExtra_ExpLines > 0) ||
          g_iOV3640_Mode == OV3640_MODE_CAPTURE ) {
            OV3640_write_cmos_sensor(0x302D, OV3640_g_iExtra_ExpLines >> 8);
            OV3640_write_cmos_sensor(0x302E, OV3640_g_iExtra_ExpLines & 0x00FF);
	  //  RETAILMSG(1, (TEXT("Camera reg 0x302D= %x ,0x302E=%x \r\n"),OV3640_read_cmos_sensor(0x302D),OV3640_read_cmos_sensor(0x302E)));
        }

        OV3640_g_iBackupExtraExp = OV3640_g_iExtra_ExpLines;
    }
}   /*  OV3640_Write_Shutter    */

static void OV3640_Set_Dummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
    kal_uint16 iPCLKs_inOneLine = QXGA_MODE_WITHOUT_DUMMY_PIXELS, iTotalLines;

    // Add dummy pixels:
    // 0x3028, 0x3029 defines the PCLKs in one line of OV3640
    // If [0x3028:0x3029] = N, the total PCLKs in one line of QXGA(3M full mode) is (N+1), and
    // total PCLKs in one line of XGA subsampling mode is (N+1) / 2
    // If need to add dummy pixels, just increase 0x3028 and 0x3029 directly
    iPCLKs_inOneLine += (iPixels * (OV3640_g_bXGA_Mode == KAL_TRUE ? 2 : 1));
    OV3640_write_cmos_sensor(0x3028, (iPCLKs_inOneLine - 1) >> 8);
    OV3640_write_cmos_sensor(0x3029, (iPCLKs_inOneLine - 1) & 0x00FF);

    // Add dummy lines:
    // 0x302A, 0x302B defines total lines in one frame of OV3640
    // If [0x302A:0x302B] = N, the total lines in one is N in dependent of resolution setting
    // Even in XGA subsampling mode, total lines defined by 0x302A, 0x302B is not subsampled.
    // If need dummy lines, just increase 0x302A and 0x302B directly
    //iTotalLines = (OV3640_read_cmos_sensor(0x302A) << 8) + OV3640_read_cmos_sensor(0x302B) + iLines;
 if(OV3640_g_bXGA_Mode == KAL_TRUE)
		iTotalLines=XGA_MODE_WITHOUT_DUMMY_LINES + iLines;
 else
		iTotalLines=QXGA_MODE_WITHOUT_DUMMY_LINES + iLines;
    OV3640_write_cmos_sensor(0x302A, iTotalLines >> 8);
    OV3640_write_cmos_sensor(0x302B, iTotalLines & 0x00FF);
}   /*  OV3640_Set_Dummy    */

/*************************************************************************
* FUNCTION
*	OV3640_SetShutter
*
* DESCRIPTION
*	This function set e-shutter of OV3640 to change exposure time.
*
* PARAMETERS
*   iShutter : exposured lines
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void set_OV3640_shutter(kal_uint16 iShutter)
{
    OV3640_g_iExpLines = iShutter;
    OV3640_Write_Shutter(OV3640_g_iExpLines);

}   /*  Set_OV3640_Shutter */

static kal_uint16 OV3640Reg2Gain(const kal_uint8 iReg)
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

 kal_uint8 OV3640Gain2Reg(const kal_uint16 iGain)
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
*	OV3640_SetGain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*   iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 OV3640_SetGain(kal_uint16 iGain)

{
   // const kal_uint16 iBaseGain = Reg2Gain(OV3640SensorCCT[INDEX_BASE_GAIN].Para);
   // const kal_uint16 iGain2Set = iBaseGain * iGain / BASEGAIN;
   // const kal_uint8 iReg = Gain2Reg(iGain2Set);
   const kal_uint8 iReg = OV3640Gain2Reg(iGain);

    OV3640_write_cmos_sensor(0x3001, iReg);

    // 0x302D, 0x302E will increase VBLANK to get exposure larger than frame exposure
    // limitation. 0x302D, 0x302E must update at the same frame as sensor gain update.
    OV3640_write_cmos_sensor(0x302D, OV3640_g_iExtra_ExpLines >> 8);
    OV3640_write_cmos_sensor(0x302E, OV3640_g_iExtra_ExpLines & 0x00FF);

   // return Reg2Gain(iReg) * BASEGAIN / iBaseGain;
   return 0;
}

/*************************************************************************
* FUNCTION
*	OV3640_NightMode
*
* DESCRIPTION
*	This function night mode of OV3640.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void OV3640_night_mode(kal_bool bEnable)
{
	  kal_uint16 iTotalLines = 0;
  
	// Impelement night mode function to fix video frame rate.
    if (bEnable)	//Night mode
    {	
    	if(OV3640_MPEG4_encode_mode ==KAL_TRUE)
    	{
			OV3640_MAX_EXPOSURE_LINES = OV3640_g_fPV_PCLK / OV3640_VIDEO_NIGHTMODE_FRAME_RATE / OV3640_g_iPV_Pixels_Per_Line;
			
		   if (OV3640_MAX_EXPOSURE_LINES >= PV_PERIOD_LINE_NUMS)		  
			OV3640_g_iDummyLines = OV3640_MAX_EXPOSURE_LINES - PV_PERIOD_LINE_NUMS; 
		   else
			OV3640_g_iDummyLines = 0;
	
		  //Set dummy lines.
	     // iTotalLines = (OV3640_read_cmos_sensor(0x302A) << 8) + OV3640_read_cmos_sensor(0x302B) + OV3640_g_iDummyLines;
	      iTotalLines = PV_PERIOD_LINE_NUMS + OV3640_g_iDummyLines;
	      OV3640_write_cmos_sensor(0x302A, iTotalLines >> 8);
	      OV3640_write_cmos_sensor(0x302B, iTotalLines & 0x00FF);
	     // OV3640_write_cmos_sensor(0x302A, 0x62C>> 8);
	    //  OV3640_write_cmos_sensor(0x302B, 0x62C & 0x00FF);
    	}			
	}
	else			//Normal mode
	{
		if(OV3640_MPEG4_encode_mode==KAL_TRUE)	
		{
			OV3640_MAX_EXPOSURE_LINES = OV3640_g_fPV_PCLK / OV3640_VIDEO_NORMALMODE_FRAME_RATE / OV3640_g_iPV_Pixels_Per_Line;
			
		   if (OV3640_MAX_EXPOSURE_LINES >= PV_PERIOD_LINE_NUMS)		  
		     OV3640_g_iDummyLines = OV3640_MAX_EXPOSURE_LINES - PV_PERIOD_LINE_NUMS; 
		    else
			OV3640_g_iDummyLines = 0;
	
		   //Set dummy lines.
	      //  iTotalLines = (OV3640_read_cmos_sensor(0x302A) << 8) + OV3640_read_cmos_sensor(0x302B) + OV3640_g_iDummyLines;
	        iTotalLines = PV_PERIOD_LINE_NUMS + OV3640_g_iDummyLines;
	       OV3640_write_cmos_sensor(0x302A, iTotalLines >> 8);
	       OV3640_write_cmos_sensor(0x302B, iTotalLines & 0x00FF);	      
		}		
	}
	
}   /*  OV3640_NightMode    */


void OV3640_camera_para_to_sensor(void)
{
    kal_uint16 iI;

    for (iI = 0; 0xFFFFFFFF != OV3640SensorReg[iI].Addr; iI++) {
        OV3640_write_cmos_sensor(OV3640SensorReg[iI].Addr, OV3640SensorReg[iI].Para);
    }

    for (iI = ENGINEER_START_ADDR; 0xFFFFFFFF != OV3640SensorReg[iI].Addr; iI++) {
       OV3640_write_cmos_sensor(OV3640SensorReg[iI].Addr, OV3640SensorReg[iI].Para);
    }

    for (iI = FACTORY_START_ADDR+1; iI < 5; iI++) {
        OV3640_write_cmos_sensor(OV3640SensorCCT[iI].Addr, OV3640SensorCCT[iI].Para);
    }
}

// update camera_para from sensor register
void OV3640_sensor_to_camera_para(void)
{
    kal_uint16 iI;

    for (iI = 0; 0xFFFFFFFF != OV3640SensorReg[iI].Addr; iI++) {
       OV3640SensorReg[iI].Para = OV3640_read_cmos_sensor(OV3640SensorReg[iI].Addr);
    }

	for (iI = ENGINEER_START_ADDR; 0xFFFFFFFF != OV3640SensorReg[iI].Addr; iI++) {
       OV3640SensorReg[iI].Para = OV3640_read_cmos_sensor(OV3640SensorReg[iI].Addr);
    }

/*
    // CCT record should be not overwritten except by engineering mode
    for (iI = 0; iI < CCT_END_ADDR; iI++) {
        camera_para.SENSOR.cct[iI].para = read_OV3640_reg(camera_para.SENSOR.cct[iI].addr);
    }
*/
}
//------------------------Engineer mode---------------------------------
kal_int32  OV3640_get_sensor_group_count(void)
{   
	return GROUP_TOTAL_NUMS;
}

void OV3640_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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
		case REGISTER_EDITOR:
			sprintf((char *)group_name_ptr, "Register Editor");
			*item_count_ptr = 2;
		break;		
		default:
		   ASSERT(0);
	}
}

void OV3640_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT *info_ptr)
{
	kal_int16 temp_reg=0;
	kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;

	 temp_para=OV3640SensorCCT[temp_addr].Para;
	
	switch (group_idx)
	{
		case PRE_GAIN:
			switch (item_idx)
			{
				case 0:
				 sprintf((char *)info_ptr->ItemNamePtr,"Pregain-R");			  				  
				  temp_addr = INDEX_PRE_GAIN_R;					  			
				break; 
				case 1:
				  sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gr");
					temp_addr = INDEX_PRE_GAIN_Gr;								
				break;
				case 2:
				     sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gb");
					 temp_addr = INDEX_PRE_GAIN_Gb;					 			
				break;
				case 3:
				      sprintf((char *)info_ptr->ItemNamePtr,"Pregain-B");
					  temp_addr = INDEX_PRE_GAIN_B;					  					
				break;
				case 4:
				   sprintf((char *)info_ptr->ItemNamePtr,"SENSOR_BASEGAIN");
				   temp_addr = INDEX_BASE_GAIN;									    
				break;
				default:
				   ASSERT(0);		
			}

			if(temp_addr!=INDEX_BASE_GAIN)		
			 {  
			 	   temp_gain=1000*(OV3640SensorCCT[temp_addr].Para)/BASEGAIN;
				   info_ptr->ItemValue=temp_gain;
		           info_ptr->IsTrueFalse=KAL_FALSE;
			       info_ptr->IsReadOnly=KAL_FALSE;
			       info_ptr->IsNeedRestart=KAL_FALSE;

			       info_ptr->Min=1*1000;
		           info_ptr->Max=255*1000/0x40;
			 	}
		    else if(temp_addr==INDEX_BASE_GAIN)
			 	{ 
			 	   temp_gain=1000*OV3640Reg2Gain(OV3640SensorCCT[temp_addr].Para)/BASEGAIN;
			
				  info_ptr->ItemValue=temp_gain;
		          info_ptr->IsTrueFalse=KAL_FALSE;
			      info_ptr->IsReadOnly=KAL_FALSE;
			      info_ptr->IsNeedRestart=KAL_FALSE;

			      info_ptr->Min=1*1000;
		          info_ptr->Max=32*1000;			 	
			 }							
		
			break;
		case CMMCLK_CURRENT:
			switch (item_idx)
			{
				case 0:
				  sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");
				  
				  temp_reg=OV3640SensorReg[CMMCLK_CURRENT_INDEX].Para;
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

void OV3640_set_isp_driving_current(kal_uint8 current)
{
}

kal_bool OV3640_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
   kal_uint16 temp_gain=0, temp_addr=0;//, temp_para=0;
   
   switch (group_idx)
	{
		case PRE_GAIN:
			switch (item_idx)
			{
			case 0:
				  temp_addr = INDEX_PRE_GAIN_R;
				  if (ItemValue < 1 * 1000 || ItemValue >= 1000*255/0x40) {
                      return KAL_FALSE;
                  }
				 
				   temp_gain = ItemValue * 0x40 / 1000;  // R, G, B gain = reg. / 0x40
                   OV3640SensorCCT[temp_addr].Para= temp_gain;
                   OV3640_write_cmos_sensor(OV3640SensorCCT[temp_addr].Addr, temp_gain);
                  break;
				
			 case 1:
				  temp_addr = INDEX_PRE_GAIN_Gr;
				   if (ItemValue < 1 * 1000 || ItemValue >= 1000*255/0x40) {
                      return KAL_FALSE;
                  }
				 
				   temp_gain = ItemValue * 0x40 / 1000;  // R, G, B gain = reg. / 0x40
                   OV3640SensorCCT[temp_addr].Para= temp_gain;
                   OV3640_write_cmos_sensor(OV3640SensorCCT[temp_addr].Addr, temp_gain);
				break;
			 case 2:
				  temp_addr = INDEX_PRE_GAIN_Gb;
				   if (ItemValue < 1 * 1000 || ItemValue >= 1000*255/0x40) {
                      return KAL_FALSE;
                  }
				 
				   temp_gain = ItemValue * 0x40 / 1000;  // R, G, B gain = reg. / 0x40
                   OV3640SensorCCT[temp_addr].Para= temp_gain;
                   OV3640_write_cmos_sensor(OV3640SensorCCT[temp_addr].Addr, temp_gain);
				break;
			 case 3:
				  temp_addr = INDEX_PRE_GAIN_B;
				   if (ItemValue < 1 * 1000 || ItemValue >= 1000*255/0x40) {
                      return KAL_FALSE;
                  }
				 
				   temp_gain = ItemValue * 0x40 / 1000;  // R, G, B gain = reg. / 0x40
                   OV3640SensorCCT[temp_addr].Para= temp_gain;
                   OV3640_write_cmos_sensor(OV3640SensorCCT[temp_addr].Addr, temp_gain);
				break;
			  case 4:
				  temp_addr = INDEX_BASE_GAIN;
				  				 
				  if (ItemValue < 1 * 1000 || ItemValue >= 32 * 1000) {
                      return KAL_FALSE;
                  }

                  temp_gain = OV3640Gain2Reg(ItemValue * BASEGAIN / 1000);
                  OV3640SensorCCT[temp_addr].Para= temp_gain;
                  OV3640_write_cmos_sensor(OV3640SensorCCT[temp_addr].Addr, temp_gain);
				  break;
				default:
				   ASSERT(0);		
			}
						
		break;
		case CMMCLK_CURRENT:
			switch (item_idx)
			{
				case 0:
				  if(ItemValue==2)
				  {
				      OV3640SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
//				      OV3640_set_isp_driving_current(ISP_DRIVING_2MA);
				  }
				  else if(ItemValue==3 || ItemValue==4)
				  {
				      OV3640SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
//				      OV3640_set_isp_driving_current(ISP_DRIVING_4MA);
				  }
				  else if(ItemValue==5 || ItemValue==6)
				  {
				      OV3640SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
//				      OV3640_set_isp_driving_current(ISP_DRIVING_6MA);
				  }
				  else
				  {
				      OV3640SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
//				      OV3640_set_isp_driving_current(ISP_DRIVING_8MA);
				  }
				break;
				default:
				   ASSERT(0);
			}
		break;
		case REGISTER_EDITOR:
			switch (item_idx)
			{
				case 0:
					if (ItemValue < 0 || ItemValue > 0xFF) {
                        return KAL_FALSE;
                    }
				  OV3640_FAC_SENSOR_REG=ItemValue;
				break;
				case 1:
					if (ItemValue < 0 || ItemValue > 0xFF) {
                        return KAL_FALSE;
                    }
				  OV3640_write_cmos_sensor(OV3640_FAC_SENSOR_REG,ItemValue);
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

UINT32 OV3640SetVideoMode(UINT16 u2FrameRate)
{
	kal_uint16 iTotalLines = 0;
	printk("[OV3640YUV] Set Video Mode:%d \n", u2FrameRate); 

	//iTemp = OV3640_read_cmos_sensor(0x3014);
	//OV3640_write_cmos_sensor(0x3014, iTemp & 0xf7); //Disable night mode
	
	if (u2FrameRate == 30)
	{
		/* MJPEG or MPEG4 Apps */
		// set max gain to 4x
		// OV3640_write_cmos_sensor(0x3014,0x84);
		/* to fix VSYNC, to fix frame rate */
	
//		OV3640_write_cmos_sensor(0x3015,0x01);
//		OV3640_write_cmos_sensor(0x3011,0x00);
		OV3640_g_iExtra_ExpLines = 0;
		OV3640_write_cmos_sensor(0x302d,0x00);
		OV3640_write_cmos_sensor(0x302e,0x00);

		iTotalLines = XGA_MODE_WITHOUT_DUMMY_LINES;		
		OV3640_write_cmos_sensor(0x302A, iTotalLines >> 8);   // Vertical Total Size 8 MSBs
		OV3640_write_cmos_sensor(0x302B, iTotalLines & 0x00FF);   // Vertical Total Size 8 LSBs
	}
	else if (u2FrameRate == 15)
	{
		// set Max gain to 16X
//		OV3640_write_cmos_sensor(0x3015, 0x01); //jerry
//		OV3640_write_cmos_sensor(0x3011,0x01);
		// clear extra exposure line
		OV3640_g_iExtra_ExpLines = 0;
              OV3640_write_cmos_sensor(0x302d, 0x00);
       	OV3640_write_cmos_sensor(0x302e, 0x00);		
		iTotalLines = 2*XGA_MODE_WITHOUT_DUMMY_LINES;
		OV3640_write_cmos_sensor(0x302A, iTotalLines >> 8);   // Vertical Total Size 8 MSBs
		OV3640_write_cmos_sensor(0x302B, iTotalLines & 0x00FF);   // Vertical Total Size 8 LSBs
	}
	else 
	{
		printk("Wrong Frame Rate \n"); 
	}
	OV3640_MPEG4_encode_mode = KAL_TRUE;	
	return TRUE;
}


static void OV3640_Sensor_Init(void)
{
#if 0 //yh modify
    OV3640_write_cmos_sensor(0x304D, 0x45);    // reserved
    OV3640_write_cmos_sensor(0x3087, 0x16);    // Timing Control 11
    OV3640_write_cmos_sensor(0x30AA, 0x45);    // reserved
    OV3640_write_cmos_sensor(0x30B0, 0xFF);    // IO control 0
    OV3640_write_cmos_sensor(0x30B1, 0xFF);    // IO control 1

    //gaiyang modify
    #if 0
/* under construction !*/
	#endif
    OV3640_write_cmos_sensor(0x30B2, 0x11);    // IO control 2
#else
    OV3640_write_cmos_sensor(0x304D, 0x45);    
    OV3640_write_cmos_sensor(0x30A7, 0x5e);  
    OV3640_write_cmos_sensor(0x3087, 0x16);  
    OV3640_write_cmos_sensor(0x309C, 0x1a);  
    OV3640_write_cmos_sensor(0x30A2, 0xe4);     
    OV3640_write_cmos_sensor(0x30AA, 0x42); 
    OV3640_write_cmos_sensor(0x30B0, 0xFF); 
    OV3640_write_cmos_sensor(0x30B1, 0xFF); 
    OV3640_write_cmos_sensor(0x30B2, 0x10); 
#endif   
     
    OV3640_write_cmos_sensor(0x300E, 0x32);    // PLL control 1
    OV3640_write_cmos_sensor(0x300F, 0x21);    // PLL control 2
    OV3640_write_cmos_sensor(0x3010, 0x20);    // PLL control 3
    OV3640_write_cmos_sensor(0x3011, 0x01);    // Clock rate control
    OV3640_write_cmos_sensor(0x304C, 0x82);    // reserved
    OV3640_write_cmos_sensor(0x30D7, 0x10);    // reserved
    OV3640_write_cmos_sensor(0x3018, 0x38);    // Luminance control
    OV3640_write_cmos_sensor(0x3019, 0x30);    // Luminance control
    OV3640_write_cmos_sensor(0x301A, 0x61);    // Fast mode AEC
    OV3640_write_cmos_sensor(0x307D, 0x00);    // Timing control 7
    OV3640_write_cmos_sensor(0x3087, 0x02);    // Timing control 11
    OV3640_write_cmos_sensor(0x3082, 0x20);    // reserved
    OV3640_write_cmos_sensor(0x3015, 0x12);    // Auto control 3
    OV3640_write_cmos_sensor(0x3014, 0x04/*0x0C*/);    // Auto control 2, b3(night mode) must be off
                                                // otherwise, OV3640 AE is always on
    OV3640_write_cmos_sensor(0x3013, 0xF2/*0xF7*/);    // disable AEC/AGC
    OV3640_write_cmos_sensor(0x303C, 0x08);    // Average window width
    OV3640_write_cmos_sensor(0x303D, 0x18);    // Average window width
    OV3640_write_cmos_sensor(0x303E, 0x06);    // Average window height
    OV3640_write_cmos_sensor(0x303F, 0x0C);    // Average window height
    OV3640_write_cmos_sensor(0x3030, 0x62);    // 16-zone average weight
    OV3640_write_cmos_sensor(0x3031, 0x26);    // 16-zone average weight
    OV3640_write_cmos_sensor(0x3032, 0xE6);    // 16-zone average weight
    OV3640_write_cmos_sensor(0x3033, 0x6E);    // 16-zone average weight
    OV3640_write_cmos_sensor(0x3034, 0xEA);    // 16-zone average weight
    OV3640_write_cmos_sensor(0x3035, 0xAE);    // 16-zone average weight
    OV3640_write_cmos_sensor(0x3036, 0xA6);    // 16-zone average weight
    OV3640_write_cmos_sensor(0x3037, 0x6A);    // 16-zone average weight
    OV3640_write_cmos_sensor(0x3104, 0x02);    // Gated reset0
    OV3640_write_cmos_sensor(0x3105, 0xFD);    // Gated clock0
    OV3640_write_cmos_sensor(0x3106, 0x00);    // Gated reset1
    OV3640_write_cmos_sensor(0x3107, 0xFF);    // Gated clock1
    OV3640_write_cmos_sensor(0x3300, 0x12);    // DSP_CTRL_0
    OV3640_write_cmos_sensor(0x3301, 0xCE/*0xDE*/);    // DSP_CTRL_1
    OV3640_write_cmos_sensor(0x3302, 0xCB/*0xCF*/);    // DSP_CTRL_2
    OV3640_write_cmos_sensor(0x3312, 0x26);    // reserved
    OV3640_write_cmos_sensor(0x3314, 0x42);    // reserved
    OV3640_write_cmos_sensor(0x3313, 0x2B);    // reserved
    OV3640_write_cmos_sensor(0x3315, 0x42);    // reserved
    OV3640_write_cmos_sensor(0x3310, 0xD0);    // reserved
    OV3640_write_cmos_sensor(0x3311, 0xBD);    // reserved
    OV3640_write_cmos_sensor(0x330C, 0x18);    // reserved
    OV3640_write_cmos_sensor(0x330D, 0x18);    // reserved
    OV3640_write_cmos_sensor(0x330E, 0x56);    // reserved
    OV3640_write_cmos_sensor(0x330F, 0x5C);    // reserved
    OV3640_write_cmos_sensor(0x330B, 0x1C);    // reserved
    OV3640_write_cmos_sensor(0x3306, 0x5C);    // reserved
    OV3640_write_cmos_sensor(0x3307, 0x11);    // reserved
    OV3640_write_cmos_sensor(0x336A, 0x52);    // R_A1
    OV3640_write_cmos_sensor(0x3370, 0x46);    // G_A1
    OV3640_write_cmos_sensor(0x3376, 0x38);    // B_A1
//    OV3640_write_cmos_sensor(0x3300, 0x13);    // DSP control 0
    OV3640_write_cmos_sensor(0x3300, 0x12);    // DSP control 0
//    OV3640_write_cmos_sensor(0x3300, 0x13);    // DSP control 0    // Sean

    OV3640_write_cmos_sensor(0x30B8, 0x20);    // UV adjust
    OV3640_write_cmos_sensor(0x30B9, 0x17);    // UV adjust
    OV3640_write_cmos_sensor(0x30BA, 0x04);    // UV adjust
    OV3640_write_cmos_sensor(0x30BB, 0x08);    // UV adjust
    OV3640_write_cmos_sensor(0x3507, 0x06);    // reserved
    OV3640_write_cmos_sensor(0x350A, 0x4F);    // reserved
    OV3640_write_cmos_sensor(0x3100, 0x32);    // Compression control
    OV3640_write_cmos_sensor(0x3304, 0x00);    // DSP_CTRL_4
    OV3640_write_cmos_sensor(0x3400, 0x02);    // ISP YUV422, MY_OUTPUT format
    OV3640_write_cmos_sensor(0x3404, 0x22);    // FMT_CTROL0
    OV3640_write_cmos_sensor(0x3500, 0x00);    // reserved
    OV3640_write_cmos_sensor(0x3600, 0xC4);    // Polarity control
    OV3640_write_cmos_sensor(0x3610, 0x60);    // WIDTH_CTRL
    OV3640_write_cmos_sensor(0x350A, 0x4F);    // reserved
    OV3640_write_cmos_sensor(0x3088, 0x08);    // ISP_XOUT
    OV3640_write_cmos_sensor(0x3089, 0x00);    // ISP_XOUT
    OV3640_write_cmos_sensor(0x308A, 0x06);    // ISP_YOUT
    OV3640_write_cmos_sensor(0x308B, 0x00);    // ISP_YOUT
    OV3640_write_cmos_sensor(0x308D, 0x04);    // Timing control 13
    OV3640_write_cmos_sensor(0x3086, 0x03);    // Timing control 10
    OV3640_write_cmos_sensor(0x3086, 0x00);    // Timing control 10
    // CIP Raw10
    OV3640_write_cmos_sensor(0x3100, 0x22);    // SC_CTRL0
    OV3640_write_cmos_sensor(0x3304, 0x01);    // DSP_CTRL_4
    OV3640_write_cmos_sensor(0x3400, 0x03);    // ISP RAW, MY_OUTPUT format
    OV3640_write_cmos_sensor(0x3600, 0xC4);    // Polarity control

    OV3640_write_cmos_sensor(0x332B, 0x08);    // enable manual R/G/B gain usage
{
    kal_uint8 iReg = OV3640_read_cmos_sensor(0x30A9);
    OV3640_write_cmos_sensor(0x30a9, iReg | 0x08); // close internal LDO
} 

    //-------------------------------------
    OV3640_write_cmos_sensor(0x30D9, 0x0D/*0x05*/);    // for 3M reddish issue, 0x04 is better but only valid for rev.2C 
    OV3640_write_cmos_sensor(0x3016, 0x82/*0x81*/);
    OV3640_write_cmos_sensor(0x30DB, 0x08);    // for vertical line
    //-------------------------------------

{
#ifdef __OV_CMX_RAW8__    // CMX8 (w/ de-noise, w/ edge)
    kal_uint8 iReg;

    // CMX RAW8
    OV3640_write_cmos_sensor(0x3100, 0x22);
    OV3640_write_cmos_sensor(0x3301, 0xEE);
    OV3640_write_cmos_sensor(0x3304, 0x03);
    OV3640_write_cmos_sensor(0x3400, 0x03);
    OV3640_write_cmos_sensor(0x3600, 0xC4);

    #ifdef __DENOISE_MANUAL__
    iReg = OV3640_read_cmos_sensor(0x332D);
    OV3640_write_cmos_sensor(0x332D, iReg & ~0x40);    // manual denoise mode
    OV3640_write_cmos_sensor(0x332C, 0xFF);    // denoise strength, bigger value has bigger strength
    #else
    iReg = OV3640_read_cmos_sensor(0x332D);
    OV3640_write_cmos_sensor(0x332D, iReg | 0x40);    // auto denoise mode
    OV3640_write_cmos_sensor(0x3331, 0x02);    // denoise strength, bigger value has bigger threshold
    #endif
#elif defined(__OV_CIP_RAW8__) // (w/ de-noise in low light, w/o de-noise in normal light, no other ISP function)
    // CIP RAW8
    OV3640_write_cmos_sensor(0x3100, 0x22);
    OV3640_write_cmos_sensor(0x3304, 0x02);
    OV3640_write_cmos_sensor(0x3400, 0x03);
    OV3640_write_cmos_sensor(0x3600, 0xC4);

    // de-noise
    #ifdef __DENOISE_MANUAL__
 #if 0
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
 #else
    OV3640_write_cmos_sensor(0x332D, 0x01);    // manual denoise mode
    OV3640_write_cmos_sensor(0x332C, 0x01);    // denoise strength, bigger value has bigger strength
 #endif
  #else   // __DENOISE_MANUAL__
{
    kal_uint8 iReg = OV3640_read_cmos_sensor(0x332D);
    OV3640_write_cmos_sensor(0x332D, iReg | 0x40);    // auto denoise mode
    OV3640_write_cmos_sensor(0x3331, 0x02);    // denoise strength, bigger value has bigger threshold
}
  #endif  // __DENOISE_MANUAL__

#else   // CIP10 (pure RAW, no ISP function)
#endif
}
}   /*  OV3640_Sensor_Init  */


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*	OV3640Open
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

UINT32 OV3640Open(void)
{
//    	volatile signed char i;
	    kal_uint16 sensor_id=0; 
	    
//	OV3640hDrvI2C=DRV_I2COpen(0);
//	DRV_I2CSetParam(OV3640hDrvI2C, I2C_VAL_FS_SAMPLE_DIV, 3);
//	DRV_I2CSetParam(OV3640hDrvI2C, I2C_VAL_FS_STEP_DIV, 8);
//	DRV_I2CSetParam(OV3640hDrvI2C, I2C_VAL_DELAY_LEN, 2);
//	DRV_I2CSetParam(OV3640hDrvI2C, I2C_VAL_CLK_EXT, I2C_CLKEXT_DISABLE);
//	OV3640I2CConfig.trans_mode=I2C_TRANSFER_FAST_MODE;
//	OV3640I2CConfig.slaveAddr=OV3640_sensor_write_I2C_address>>1;
//	OV3640I2CConfig.RS_ST=I2C_TRANSFER_STOP;	/* for fast mode */

    // SW reset must be the 1st command sent to OV3640
   OV3640_write_cmos_sensor(0x3012, 0x80);
   Sleep(150); // at least one frame delay time needed after SW reset
   //Sleep(50);

    // check if sensor ID correct
    sensor_id=((OV3640_read_cmos_sensor(0x300A) << 8) | OV3640_read_cmos_sensor(0x300B));   
	RETAILMSG(1, (TEXT("Sensor Read  OV3640 ID %x \r\n"),sensor_id));
    if (((OV3640_read_cmos_sensor(0x300A) << 8) | OV3640_read_cmos_sensor(0x300B)) != OV3640_SENSOR_ID) {        
        return ERROR_SENSOR_CONNECT_FAIL;
    }
	 RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
    // initail sequence write in
    OV3640_Sensor_Init();

   return ERROR_NONE;
}   /* OV3640Open  */

UINT32 OV3640GetSensorID(UINT32 *sensorID) 
{

    OV3640_write_cmos_sensor(0x3012, 0x80);
   Sleep(150); // at least one frame delay time needed after SW reset
   //Sleep(50);

    // check if sensor ID correct
   *sensorID=((OV3640_read_cmos_sensor(0x300A) << 8) | OV3640_read_cmos_sensor(0x300B));  

	RETAILMSG(1, (TEXT("Sensor Read  OV3640 ID %x \r\n"),*sensorID));
    if (((OV3640_read_cmos_sensor(0x300A) << 8) | OV3640_read_cmos_sensor(0x300B)) != OV3640_SENSOR_ID) { 
		*sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
	 RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));

    return ERROR_NONE;
}
/*************************************************************************
* FUNCTION
*	OV3640Close
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
UINT32 OV3640Close(void)
{
  //CISModulePowerOn(FALSE);
//	DRV_I2CClose(OV3640hDrvI2C);
	return ERROR_NONE;
}   /* OV3640Close */

/*************************************************************************
* FUNCTION
* OV3640Preview
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
UINT32 OV3640Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint32 iTemp,iTemp2;
    kal_uint16 iStartX = 0, iStartY = 1;

    OV3640_write_cmos_sensor(0x3012, 0x10);    // change to XGA mode
    OV3640_write_cmos_sensor(0x3023, 0x06);    // Vertical Window Start 8 LSBs
    OV3640_write_cmos_sensor(0x3026, 0x03);    // Vertical Height 8 MSBs
    OV3640_write_cmos_sensor(0x3027, 0x04);    // Vertical Height 8 LSBs
    OV3640_write_cmos_sensor(0x302A, XGA_MODE_WITHOUT_DUMMY_LINES >> 8/*0x03*/);   // Vertical Total Size 8 MSBs
    OV3640_write_cmos_sensor(0x302B, XGA_MODE_WITHOUT_DUMMY_LINES & 0x00FF/*0x10*/);   // Vertical Total Size 8 LSBs
    OV3640_write_cmos_sensor(0x3075, 0x24);    // VSYNC pulse options
    OV3640_write_cmos_sensor(0x300D, 0x01);    // PCLK MY_OUTPUT control, divided by 2 
    OV3640_write_cmos_sensor(0x30D7, 0x90);    // reserved gaiyang add
    OV3640_write_cmos_sensor(0x3069, 0x04);    // reserved  gaiyang add
    OV3640_write_cmos_sensor(0x303E, 0x00);    // Average Window Vertical Height 8 MSBs
    OV3640_write_cmos_sensor(0x303F, 0xC0);    // Average Window Vertical Height 8 LSBs
    OV3640_write_cmos_sensor(0x3302, 0xEA);  /*0xEE*//*0xEF*/  // DSP_CTRL_2, disable AWB(b0 = 0) //gaiyang add
//    OV3640_write_cmos_sensor(0x3302, 0xEE/*0xEF*/);    // DSP_CTRL_2, disable AWB(b0 = 0) Sean

    OV3640_write_cmos_sensor(0x335F, 0x34);    // SIZE_IN_MISC
    OV3640_write_cmos_sensor(0x3360, 0x0C);    // HSIZE_IN_L
    OV3640_write_cmos_sensor(0x3361, 0x04);    // VSIZE_IN_L
    OV3640_write_cmos_sensor(0x3362, 0x34);    // SIZE_OUT_MISC, Zoom_out MY_OUTPUT size
    OV3640_write_cmos_sensor(0x3363, 0x08);    // HSIZE_OUT_L
    OV3640_write_cmos_sensor(0x3364, 0x04);    // VSIZE_OUT_L
    OV3640_write_cmos_sensor(0x3403, 0x42);    // ISP_PAD_CTR2
    OV3640_write_cmos_sensor(0x3088, 0x04);    // ISP_X_OUT[15:8], x_MY_OUTPUT_size
    OV3640_write_cmos_sensor(0x3089, 0x00);    // ISP_X_OUT[7:0]
    OV3640_write_cmos_sensor(0x308A, 0x03);    // ISP_Y_OUT[15:8], y_MY_OUTPUT_size
    OV3640_write_cmos_sensor(0x308B, 0x00);    // ISP_Y_OUT[7:0]

    //-------------------------------------------------------------------------------
    // PLL MY_OUTPUT clock(fclk)
    // fclk = (0x40 - 0x300E[5:0]) x N x Bit8Div x MCLK / M, where
    //      N = 1, 1.5, 2, 3 for 0x300F[7:6] = 0~3, respectively
    //      M = 1, 1.5, 2, 3 for 0x300F[1:0] = 0~3, respectively
    //      Bit8Div = 1, 1, 4, 5 for 0x300F[5:4] = 0~3, respectively
    // Sys Clk = fclk / Bit8Div / SenDiv
    // Sensor MY_OUTPUT clock(DVP PCLK)
    // DVP PCLK = ISP CLK / DVPDiv, where
    //      ISP CLK =  fclk / Bit8Div / SenDiv / CLKDiv / 2, where
    //          Bit8Div = 1, 1, 4, 5 for 0x300F[5:4] = 0~3, respectively
    //          SenDiv = 1, 2 for 0x3010[4] = 0 or 1 repectively
    //          CLKDiv = (0x3011[5:0] + 1)
    //      DVPDiv = 0x304C[3:0] * (2 ^ 0x304C[4]), if 0x304C[3:0] = 0, use 16 instead
    //
    // Base shutter calculation
    //      60Hz: (1/120) * ISP Clk / QXGA_MODE_WITHOUT_DUMMY_PIXELS
    //      50Hz: (1/100) * ISP Clk / QXGA_MODE_WITHOUT_DUMMY_PIXELS
    //-------------------------------------------------------------------------------
    #if 0
    OV3640_write_cmos_sensor(0x300F, 0x21); //pclk=26M
    OV3640_write_cmos_sensor(0x3010, 0x20);
    OV3640_write_cmos_sensor(0x3011, 0x00);
    OV3640_write_cmos_sensor(0x300E, 0x34);//0x33
    OV3640_write_cmos_sensor(0x304C, 0x84);
	#endif
	
    #if  1
    OV3640_write_cmos_sensor(0x300F, 0x21); //pclk=28.1666M
    OV3640_write_cmos_sensor(0x3010, 0x20);
    OV3640_write_cmos_sensor(0x3011, 0x00);
    OV3640_write_cmos_sensor(0x300E, 0x33);
    OV3640_write_cmos_sensor(0x304C, 0x84);
	#endif
	
	if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
	{
	    RETAILMSG(1,(TEXT("Camera Video preview\r\n")));
		OV3640_MPEG4_encode_mode = KAL_TRUE;	
	}
	else
	{
	    RETAILMSG(1,(TEXT("Camera preview\r\n")));
		OV3640_MPEG4_encode_mode = KAL_FALSE;	
		g_iOV3640_Mode = OV3640_MODE_PREVIEW;		
	}
   
   OV3640_g_fPV_PCLK  =28166666;
 
   OV3640_iDummyPixels = 0;
   OV3640_g_iDummyLines = 0;//26000000/28166666
         
   OV3640_g_bXGA_Mode = KAL_TRUE; // 1024x768 MY_OUTPUT

    iTemp = OV3640_read_cmos_sensor(0x307C) & (~0x03);
 iTemp2 = OV3640_read_cmos_sensor(0x3090) & (~0x08);
    switch (sensor_config_data->SensorImageMirror) {
    case IMAGE_NORMAL:
         OV3640_write_cmos_sensor(0x307C, iTemp | 0x03);
	 OV3640_write_cmos_sensor(0x3090, iTemp2|0x08);
        if (g_iOV3640_Mode == OV3640_MODE_PREVIEW) {
            iStartX = 0;
            iStartY = 2;
        }else {
            iStartX = 0;
            iStartY = 2;
        }
        break;
    case IMAGE_HV_MIRROR:
        OV3640_write_cmos_sensor(0x307C, iTemp );
        OV3640_write_cmos_sensor(0x3090, iTemp2);
        if (g_iOV3640_Mode == OV3640_MODE_PREVIEW) {
            iStartX = 0;
            iStartY = 1;
        }else {
            iStartX = 0;
            iStartY = 1;
        }
        break;
    default:
	 break;
    }

    OV3640_g_iPV_Pixels_Per_Line = XGA_MODE_WITHOUT_DUMMY_PIXELS + OV3640_iDummyPixels;
    OV3640_Set_Dummy(OV3640_iDummyPixels, OV3640_g_iDummyLines);

   image_window->GrabStartX= iStartX;
   image_window->GrabStartY= iStartY;
 image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH - 16;//16
 image_window->ExposureWindowHeight = IMAGE_SENSOR_PV_HEIGHT - 12;//12

    OV3640_Write_Shutter(OV3640_g_iExpLines); 
	// copy sensor_config_data
	memcpy(&OV3640SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;

}   /*  OV3640Preview   */

/*************************************************************************
* FUNCTION
*	OV3640Capture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV3640Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint32 iShutter = OV3640_g_iExpLines;    
    kal_uint16 iStartX = 0, iStartY = 0, iGrabWidth = 0, iGrabHeight = 0;
    kal_uint16 iCP_Pixels_Per_Line = 0;
//    static float fCP_PCLK = 0;
    static kal_uint32 fCP_PCLK = 0;

	g_iOV3640_Mode = OV3640_MODE_CAPTURE;
	
    switch (sensor_config_data->SensorImageMirror) {
        case IMAGE_NORMAL:
            iStartX = 0;
            iStartY = 2;
            break;

        case IMAGE_HV_MIRROR:
            iStartX = 0;
            iStartY = 1;
            break;

        default:
            ASSERT(0);
        }   
	 if ((image_window->ImageTargetWidth<= IMAGE_SENSOR_PV_WIDTH) &&
        (image_window->ImageTargetHeight<= IMAGE_SENSOR_PV_HEIGHT)) 
	  	{  	
		  RETAILMSG(1,(TEXT("Camera capture small size\r\n")));

		  fCP_PCLK = OV3640_g_fPV_PCLK;
		  
		  OV3640_iDummyPixels=0;
          OV3640_g_iDummyLines = 0;
		
          iGrabWidth = IMAGE_SENSOR_PV_WIDTH - 16;
          iGrabHeight = IMAGE_SENSOR_PV_HEIGHT - 12;
		  iCP_Pixels_Per_Line = XGA_MODE_WITHOUT_DUMMY_PIXELS + OV3640_iDummyPixels;
		 
		}
	else
		{
     // ordinary capture mode for > XGA resolution
        RETAILMSG(1,(TEXT("Camera capture big size\r\n")));
        OV3640_g_bXGA_Mode = KAL_FALSE;

        OV3640_write_cmos_sensor(0x3012, 0x00);    // change to QXGA mode(2048x1536)
        OV3640_write_cmos_sensor(0x3020, 0x01);    // H window start 8 MSBs
        OV3640_write_cmos_sensor(0x3021, 0x1D);    // H window start 8 LSBs
        OV3640_write_cmos_sensor(0x3022, 0x00);    // V window start 8 MSBs
        OV3640_write_cmos_sensor(0x3023, 0x0A);    // V window start 8 LSBs
//        OV3640_write_cmos_sensor(0x3023, 0x09);    // Koli change
        OV3640_write_cmos_sensor(0x3024, 0x08);    // H width 8 MSBs
        OV3640_write_cmos_sensor(0x3025, 0x18);    // H width 8 LSBs
        OV3640_write_cmos_sensor(0x3026, 0x06);    // V height 8 MSBs
        OV3640_write_cmos_sensor(0x3027, 0x0C);    // Vertical Height 8 LSBs
        OV3640_write_cmos_sensor(0x302A, QXGA_MODE_WITHOUT_DUMMY_LINES >> 8);      // Vertical Total Size 8 MSBs
        OV3640_write_cmos_sensor(0x302B, QXGA_MODE_WITHOUT_DUMMY_LINES & 0x00FF);  // Vertical Total Size 8 LSBs
        OV3640_write_cmos_sensor(0x3075, 0x44);    // Vsync pulse options
        OV3640_write_cmos_sensor(0x300D, 0x00);    // PCLK MY_OUTPUT control
        OV3640_write_cmos_sensor(0x30D7, 0x10);    // reserved
        OV3640_write_cmos_sensor(0x3069, 0x44);    // BLC control
        OV3640_write_cmos_sensor(0x303E, 0x01);    // Average Window Vertical Height 8 MSBs
        OV3640_write_cmos_sensor(0x303F, 0x80);    // Average Window Vertical Height 8 LSBs
        OV3640_write_cmos_sensor(0x3302, 0xEA/*0xEE*//*0xEF*/);    // DSP_CTRL_2, disable AWB(b0 = 0)
//        OV3640_write_cmos_sensor(0x3302, 0xEE/*0xEF*/);    // DSP_CTRL_2, disable AWB(b0 = 0) Sean
        OV3640_write_cmos_sensor(0x335F, 0x68);    // SIZE_IN_MISC
        OV3640_write_cmos_sensor(0x3360, 0x18);    // HSIZE_IN_L
        OV3640_write_cmos_sensor(0x3361, 0x0C);    // VSIZE_IN_L
        OV3640_write_cmos_sensor(0x3362, 0x68);    // SIZE_OUT_MISC, Zoom_out MY_OUTPUT size
        OV3640_write_cmos_sensor(0x3363, 0x08);    // HSIZE_OUT_L
        OV3640_write_cmos_sensor(0x3364, 0x04);    // VSIZE_OUT_L
        OV3640_write_cmos_sensor(0x3403, 0x42);    // ISP_PAD_CTR2
        OV3640_write_cmos_sensor(0x3088, 0x08);    // ISP_X_OUT[15:8], x_MY_OUTPUT_size
        OV3640_write_cmos_sensor(0x3089, 0x00);    // ISP_X_OUT[7:0]
        OV3640_write_cmos_sensor(0x308A, 0x06);    // ISP_YOUT[15:8], y_MY_OUTPUT_size
        OV3640_write_cmos_sensor(0x308B, 0x00);    // ISP_YOUT[7:0]
//        OV3640_write_cmos_sensor(0x3011, 0x01);  // for QXGA mode, must satisfy " >= 0x01"
       #if 1
        OV3640_write_cmos_sensor(0x304C, 0x82);    // reserved, must be set for QXGA mode      
        OV3640_write_cmos_sensor(0x300E, 0x35);    // PCLK = 47.666666MHz
	   #endif
	   #if 0
        OV3640_write_cmos_sensor(0x304C, 0x82);    // reserved, must be set for QXGA mode      
        OV3640_write_cmos_sensor(0x300E, 0x36);    // PCLK = 43.333333MHz
	   #endif
       fCP_PCLK = 47666666;     
      // fCP_PCLK = 43333333;
        OV3640_iDummyPixels=0;
        OV3640_g_iDummyLines = 0;                
	   iGrabWidth = IMAGE_SENSOR_FULL_WIDTH;
	   iGrabHeight = IMAGE_SENSOR_FULL_HEIGHT;

        iCP_Pixels_Per_Line = QXGA_MODE_WITHOUT_DUMMY_PIXELS + OV3640_iDummyPixels;
		}
   

    image_window->GrabStartX = iStartX;
    image_window->GrabStartY = iStartY;
    image_window->ExposureWindowWidth= iGrabWidth;
    image_window->ExposureWindowHeight = iGrabHeight;   

//    iShutter = iShutter * (fCP_PCLK / OV3640_g_fPV_PCLK) * OV3640_g_iPV_Pixels_Per_Line / iCP_Pixels_Per_Line;
	  iShutter = (iShutter * (128*(fCP_PCLK>>10) / (OV3640_g_fPV_PCLK>>10)) * OV3640_g_iPV_Pixels_Per_Line / iCP_Pixels_Per_Line) >>7;

	  RETAILMSG(1, (TEXT("Camera compute iShutter= %x \r\n"),iShutter));

    if (OV3640_g_bXGA_Mode == KAL_FALSE) {
        iShutter <<= 1; // XGA mode's brightness level = 2x QXGA mode's brightness level
      }
	
  
    if (iShutter < 1) {
        iShutter = 1;
    }
	// config flashlight preview setting
		sensor_config_data->DefaultPclk = fCP_PCLK; 
		sensor_config_data->Pixels = iCP_Pixels_Per_Line;
		
		if(OV3640_g_bXGA_Mode == KAL_TRUE)
		sensor_config_data->FrameLines =QXGA_MODE_WITHOUT_DUMMY_LINES+OV3640_g_iDummyLines ;
		else
		sensor_config_data->FrameLines =PV_PERIOD_LINE_NUMS ;
		
		sensor_config_data->Lines = image_window->ExposureWindowHeight;    
		sensor_config_data->Shutter =iShutter;	

{
    kal_uint8 iTemp = OV3640_read_cmos_sensor(0x3001);//trigger BLC,gain change
    OV3640_write_cmos_sensor(0x3001, ++iTemp); //gaiyang modify
    OV3640_write_cmos_sensor(0x3001, --iTemp);
}
    OV3640_Set_Dummy(OV3640_iDummyPixels, OV3640_g_iDummyLines);
    OV3640_Write_Shutter(iShutter);
	// copy sensor_config_data
	memcpy(&OV3640SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}   /* OV3640_Capture() */

UINT32 OV3640GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
	pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
	pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH-16;
	pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT-12;
	return ERROR_NONE;
}	/* OV3640GetResolution() */

UINT32 OV3640GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH-16;
	pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT-12;
	pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
	pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=10;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE; //low active
	pSensorInfo->SensorResetDelayCount=5; 
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_R;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	//SENSOR_CLOCK_POLARITY_LOW
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;//
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
	pSensorInfo->SensorInterruptDelayLines = 4;
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
	
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;
	
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;
	pSensorInfo->CaptureDelayFrame = 3; 
	pSensorInfo->PreviewDelayFrame = 0; 
	pSensorInfo->VideoDelayFrame = 0; 	
	pSensorInfo->SensorMasterClockSwitch = 0; 
       pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;   	
       pSensorInfo->AEShutDelayFrame = 0;		   /* The frame of setting shutter default 0 for TG int */
	pSensorInfo->AESensorGainDelayFrame = 1;	   /* The frame of setting sensor gain */
	pSensorInfo->AEISPGainDelayFrame = 2;    

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			#if 1
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;			
                     pSensorInfo->SensorGrabStartX = 1; 
                     pSensorInfo->SensorGrabStartY = 1;     			
		   #else
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	4;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 4;
			pSensorInfo->SensorDataLatchCount= 2;
		#endif

		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			#if 1
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
                     pSensorInfo->SensorGrabStartX = 1; 
                     pSensorInfo->SensorGrabStartY = 1;     			
			#else
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	4;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 4;
			pSensorInfo->SensorDataLatchCount= 2;
			#endif
		break;
		default:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;		
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
                     pSensorInfo->SensorGrabStartX = 1; 
                     pSensorInfo->SensorGrabStartY = 1;     			
		break;
	}
	OV3640PixelClockDivider=pSensorInfo->SensorPixelClockCount;
		memcpy(pSensorConfigData, &OV3640SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
return ERROR_NONE;
}	/* OV3640GetInfo() */


UINT32 OV3640Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			OV3640Preview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			OV3640Capture(pImageWindow, pSensorConfigData);
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
}	/* OV3640Control() */

UINT32 OV3640FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	UINT32 OV3640SensorRegNumber;
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
			*pFeatureReturnPara16++=PV_PERIOD_PIXEL_NUMS+OV3640_iDummyPixels;
			*pFeatureReturnPara16=PV_PERIOD_LINE_NUMS+OV3640_g_iDummyLines;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = OV3640_g_fPV_PCLK;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			set_OV3640_shutter(*pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			OV3640_night_mode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:
			OV3640_SetGain((UINT16) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			OV3640_isp_master_clock=*pFeatureData32;
		break;
		case SENSOR_FEATURE_SET_REGISTER:
OV3640_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
	pSensorRegData->RegData = OV3640_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
			OV3640SensorRegNumber=*pFeatureData32++;
			if (OV3640SensorRegNumber> FACTORY_END_ADDR)
				return FALSE;
			for (i=0;i<OV3640SensorRegNumber;i++)
			{
				OV3640SensorCCT[i].Addr=*pFeatureData32++;
				OV3640SensorCCT[i].Para=*pFeatureData32++;
			}
		break;
		case SENSOR_FEATURE_GET_CCT_REGISTER:
			OV3640SensorRegNumber= FACTORY_END_ADDR;
			if (*pFeatureParaLen<(OV3640SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
				return FALSE;
			*pFeatureData32++=OV3640SensorRegNumber;
			for (i=0;i<OV3640SensorRegNumber;i++)
			{
				*pFeatureData32++=OV3640SensorCCT[i].Addr;
				*pFeatureData32++=OV3640SensorCCT[i].Para;
			}
		break;
		case SENSOR_FEATURE_SET_ENG_REGISTER:
			OV3640SensorRegNumber=*pFeatureData32++;
			if (OV3640SensorRegNumber>FACTORY_END_ADDR)
				return FALSE;
			for (i=0;i<OV3640SensorRegNumber;i++)
			{
				OV3640SensorReg[i].Addr=*pFeatureData32++;
				OV3640SensorReg[i].Para=*pFeatureData32++;
			}
		break;
		case SENSOR_FEATURE_GET_ENG_REGISTER:
			OV3640SensorRegNumber=FACTORY_END_ADDR;
			if (*pFeatureParaLen<(OV3640SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
				return FALSE;
			*pFeatureData32++=OV3640SensorRegNumber;
			for (i=0;i<OV3640SensorRegNumber;i++)
			{
				*pFeatureData32++=OV3640SensorReg[i].Addr;
				*pFeatureData32++=OV3640SensorReg[i].Para;
			}
		break;
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
			if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
			{
				pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
				pSensorDefaultData->SensorId=OV3640_SENSOR_ID;
				memcpy(pSensorDefaultData->SensorEngReg, OV3640SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
				memcpy(pSensorDefaultData->SensorCCTReg, OV3640SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
			}
			else
				return FALSE;
			*pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
		break;
case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &OV3640SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
			break;
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		     OV3640_camera_para_to_sensor();
		break;
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
			OV3640_sensor_to_camera_para();
		break;							
		case SENSOR_FEATURE_GET_GROUP_COUNT:
			*pFeatureReturnPara32++=OV3640_get_sensor_group_count();
			*pFeatureParaLen=4;
		break;										
		case SENSOR_FEATURE_GET_GROUP_INFO:
			OV3640_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
		break;	
		case SENSOR_FEATURE_GET_ITEM_INFO:
			OV3640_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		break;			
		case SENSOR_FEATURE_SET_ITEM_INFO:
			OV3640_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
		break;			
		case SENSOR_FEATURE_GET_ENG_INFO:
			pSensorEngInfo->SensorId = 130;	
			pSensorEngInfo->SensorType = CMOS_SENSOR;
			pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_R;
			*pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
		break;
		 case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV3640GetSensorID(pFeatureReturnPara32); 
            break; 
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;		
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		       OV3640SetVideoMode(*pFeatureData16);
		       break; 
		default:
			break;
	}
	return ERROR_NONE;
}	/* OV3640FeatureControl() */

SENSOR_FUNCTION_STRUCT	SensorFuncOV3640=
{
	OV3640Open,
	OV3640GetInfo,
	OV3640GetResolution,
	OV3640FeatureControl,
	OV3640Control,
	OV3640Close
};

UINT32 OV3640SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncOV3640;

	return ERROR_NONE;
}	/* SensorInit() */

