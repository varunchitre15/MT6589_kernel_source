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
 * 02 19 2012 koli.lin
 * [ALPS00237113] [Performance][Video recording]Recording preview the screen have flash
 * [Camera] 1. Modify the AE converge speed in the video mode.
 *                2. Modify the isp gain delay frame with sensor exposure time and gain synchronization.
 *
 * 10 31 2011 koli.lin
 * [ALPS00081266] [Li Zhen]

P49?是存在此?象


[Wenjing]

Hi,Koli:

tester?有抓到??出??的?片，但是?描述看，??跟之前6573上何?提在video mode的一?issue相同，麻??忙check

thanks


李珍  61204

 * [Camera] 1. Modify the preview output speed 648Mbps/lane.
 *                2. Fix the flicker min limitation value. (max 1 fps)
 *
 * 05 17 2011 koli.lin
 * [ALPS00048684] [Need Patch] [Volunteer Patch]
 * [Camera] Add the sensor software power down mode.
 *
 * 04 12 2011 koli.lin
 * [ALPS00039760] [Need Patch] [Volunteer Patch]
 * [Camera] Add the sensor 2D or 3D information to sensor driver.
 *
 * 04 11 2011 koli.lin
 * [ALPS00039429] [Need Patch] [Volunteer Patch]
 * [Camera] Add flicker frame rate and sensor test pattern feature id.
 *
 * 03 15 2011 koli.lin
 * [ALPS00034474] [Need Patch] [Volunteer Patch]
 * Move sensor driver current setting to isp of middleware.
 *
 * 10 12 2010 koli.lin
 * [ALPS00127101] [Camera] AE will flash
 * [Camera]Create Vsync interrupt to handle the exposure time, sensor gain and raw gain control.
 *
 * 08 27 2010 ronnie.lai
 * [DUMA00032601] [Camera][ISP]
 * Check in AD5820 Constant AF function.
 *
 * 08 26 2010 ronnie.lai
 * [DUMA00032601] [Camera][ISP]
 * Add AD5820 Lens driver function.
 * must disable SWIC and bus log, otherwise the lens initial time take about 30 second.(without log about 3 sec)
 *
 * 08 19 2010 ronnie.lai
 * [DUMA00032601] [Camera][ISP]
 * Merge dual camera relative settings. Main OV5642, SUB O7675 ready.
 *
 * 08 18 2010 ronnie.lai
 * [DUMA00032601] [Camera][ISP]
 * Mmodify ISP setting and add OV5642 sensor driver.
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

#include "ov5642raw_Sensor.h"
#include "ov5642raw_Camera_Sensor_para.h"
#include "ov5642raw_CameraCustomized.h"

kal_bool  OV5642_MPEG4_encode_mode = KAL_FALSE;
kal_bool  OV5642_Auto_Flicker_mode = KAL_FALSE;

kal_uint16  OV5642_sensor_gain_base=0x0;
/* MAX/MIN Explosure Lines Used By AE Algorithm */
kal_uint16 OV5642_MAX_EXPOSURE_LINES = OV5642_PV_PERIOD_LINE_NUMS;//650;
kal_uint8  OV5642_MIN_EXPOSURE_LINES = 2;
kal_uint32 OV5642_isp_master_clock;
kal_uint16 OV5642_CURRENT_FRAME_LINES = OV5642_PV_PERIOD_LINE_NUMS;//650;

static kal_uint16 OV5642_dummy_pixels=0, OV5642_dummy_lines=0;
kal_uint16 OV5642_PV_dummy_pixels=0,OV5642_PV_dummy_lines=0;

kal_uint8 OV5642_sensor_write_I2C_address = OV5642_WRITE_ID;
kal_uint8 OV5642_sensor_read_I2C_address = OV5642_READ_ID;

#define LOG_TAG "[OV5642Raw]"
#define SENSORDB(fmt, arg...) printk( LOG_TAG  fmt, ##arg)
#define RETAILMSG(x,...)
#define TEXT

kal_uint16 OV5642_g_iDummyLines = 28; 

UINT8 OV5642PixelClockDivider=0;
kal_uint32 OV5642_sensor_pclk=52000000;//19500000;
kal_uint32 OV5642_PV_pclk = 5525; 
kal_uint32 OV5642_CAP_pclk = 6175;

kal_uint16 OV5642_pv_exposure_lines=0x100,OV5642_g_iBackupExtraExp = 0,OV5642_extra_exposure_lines = 0;

kal_uint16 OV5642_sensor_id=0;

MSDK_SENSOR_CONFIG_STRUCT OV5642SensorConfigData;

kal_uint32 OV5642_FAC_SENSOR_REG;
kal_uint16 OV5642_sensor_flip_value;


/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT OV5642SensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT OV5642SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/

typedef enum
{
  OV5642_720P,       //1M 1280x960
  OV5642_5M,     //5M 2592x1944
} OV5642_RES_TYPE;
OV5642_RES_TYPE OV5642_g_RES=OV5642_720P;

typedef enum
{
  OV5642_MODE_PREVIEW,  //1M  	1280x960
  OV5642_MODE_CAPTURE   //5M    2592x1944
} OV5642_MODE;
OV5642_MODE g_iOV5642_Mode=OV5642_MODE_PREVIEW;


extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
extern int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId); 
#define OV5642_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, OV5642_WRITE_ID)
#define OV5642_burst_write_cmos_sensor(pData, bytes)  iBurstWriteReg(pData, bytes, OV5642_WRITE_ID)

/*******************************************************************************
* 
********************************************************************************/
kal_uint16 OV5642_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV5642_WRITE_ID);
    return get_byte;
}

/*******************************************************************************
* 
********************************************************************************/
void OV5642_write_shutter(kal_uint16 shutter)
{
    kal_uint16 iExp = shutter;
    kal_uint16 OV5642_g_iExtra_ExpLines = 0 ;
    
    if (OV5642_g_RES == OV5642_720P) {
        if (iExp <= OV5642_PV_EXPOSURE_LIMITATION + OV5642_g_iDummyLines) {
            OV5642_g_iExtra_ExpLines = OV5642_PV_EXPOSURE_LIMITATION + OV5642_g_iDummyLines;
        }else {
            OV5642_g_iExtra_ExpLines = iExp+28;//; - OV5642_PV_EXPOSURE_LIMITATION - OV5642_g_iDummyLines;
           //iExp = OV5642_PV_EXPOSURE_LIMITATION + OV5642_g_iDummyLines;
        }

    }else {
        if (iExp <= OV5642_FULL_EXPOSURE_LIMITATION + OV5642_g_iDummyLines) {
            OV5642_g_iExtra_ExpLines = OV5642_FULL_EXPOSURE_LIMITATION+ OV5642_g_iDummyLines;
        }else {
            OV5642_g_iExtra_ExpLines = iExp+28;// - OV5642_FULL_EXPOSURE_LIMITATION - OV5642_g_iDummyLines;
           //iExp = OV5642_FULL_EXPOSURE_LIMITATION + OV5642_g_iDummyLines;
        }
    }

    //! The exposure limit can not be the same as exp 
    if (OV5642_g_iExtra_ExpLines == iExp) {
          OV5642_g_iExtra_ExpLines += 28; 
    }
    //SENSORDB("Set Extra-line = %d, iExp = %d \n", OV5642_g_iExtra_ExpLines, iExp);     

//    OV5642_write_cmos_sensor(0x3212, 0x01); 
//    if (OV5642_MPEG4_encode_mode != TRUE) {
    if ((OV5642_MPEG4_encode_mode != TRUE) && (OV5642_Auto_Flicker_mode != TRUE)) {
        OV5642_write_cmos_sensor(0x350c, OV5642_g_iExtra_ExpLines >> 8);
        OV5642_write_cmos_sensor(0x350d, OV5642_g_iExtra_ExpLines & 0x00FF);
    }
    
    OV5642_write_cmos_sensor(0x3500, (iExp >> 12) & 0xFF);
    OV5642_write_cmos_sensor(0x3501, (iExp >> 4 ) & 0xFF);
    OV5642_write_cmos_sensor(0x3502, (iExp <<4 ) & 0xFF);
//    OV5642_write_cmos_sensor(0x3212, 0x11); 
//    OV5642_write_cmos_sensor(0x3212, 0xa1);     

    OV5642_g_iBackupExtraExp = OV5642_g_iExtra_ExpLines;    
}   /* write_OV5642_shutter */

/*******************************************************************************
* 
********************************************************************************/
static kal_uint16 OV5642Reg2Gain(const kal_uint8 iReg)
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

/*******************************************************************************
* 
********************************************************************************/
static kal_uint8 OV5642Gain2Reg(const kal_uint16 iGain)
{
    kal_uint8 iReg = 0x00;

    if (iGain < 2 * BASEGAIN) {
        // Gain = 1 + GAIN[3:0](0x00) / 16
        //iReg = 16 * (iGain - BASEGAIN) / BASEGAIN;
        iReg = 16 * iGain / BASEGAIN - 16; 
    }else if (iGain < 4 * BASEGAIN) {
        // Gain = 2 * (1 + GAIN[3:0](0x00) / 16)
        iReg |= 0x10;
        //iReg |= 8 * (iGain - 2 * BASEGAIN) / BASEGAIN;
        iReg |= (8 *iGain / BASEGAIN - 16); 
    }else if (iGain < 8 * BASEGAIN) {
        // Gain = 4 * (1 + GAIN[3:0](0x00) / 16)
        iReg |= 0x30;
        //iReg |= 4 * (iGain - 4 * BASEGAIN) / BASEGAIN;
        iReg |= (4 * iGain / BASEGAIN - 16); 
    }else if (iGain < 16 * BASEGAIN) {
        // Gain = 8 * (1 + GAIN[3:0](0x00) / 16)
        iReg |= 0x70;
        //iReg |= 2 * (iGain - 8 * BASEGAIN) / BASEGAIN;
        iReg |= (2 * iGain / BASEGAIN - 16); 
    }else if (iGain < 32 * BASEGAIN) {
        // Gain = 16 * (1 + GAIN[3:0](0x00) / 16)
        iReg |= 0xF0;
        //iReg |= (iGain - 16 * BASEGAIN) / BASEGAIN;
        iReg |= (iGain / BASEGAIN - 16); 
    }else {
        ASSERT(0);
    }

    return iReg;

}

/*************************************************************************
* FUNCTION
*    OV5642_SetGain
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
 void OV5642_SetGain(UINT16 iGain)
{
    kal_uint8 iReg;

    //KD_IMGSENSOR_PROFILE_INIT();
    //SENSORDB("[OV5642_SetGain] E Gain = %d \n", iGain); 
    iReg = OV5642Gain2Reg(iGain);

    //! For OV642 sensor, the set gain don't have double buffer,  
    //! it needs use group write to write sensor gain 
    OV5642_write_cmos_sensor(0x3212, 0x00); 
    OV5642_write_cmos_sensor(0x350B, (kal_uint32)iReg);
    OV5642_write_cmos_sensor(0x3212, 0x10); 
    OV5642_write_cmos_sensor(0x3212, 0xA0); 
    
    //KD_IMGSENSOR_PROFILE("OV5642_SetGain"); 
    //SENSORDB("Gain = %x\n", iReg);

    /* Set extra shutter here because extra shutter active time same as gain  */
    /*if (OV5642_g_iBackupExtraExp !=OV5642_extra_exposure_lines)
    {
        OV5642_write_cmos_sensor(0x350c, OV5642_extra_exposure_lines >> 8);
        OV5642_write_cmos_sensor(0x350d, OV5642_extra_exposure_lines & 0x00FF);
        OV5642_g_iBackupExtraExp = OV5642_extra_exposure_lines;
    }
    */
    //SENSORDB("[OV5642_SetGain] X Gain = %d \n", iGain); 
}   /*  OV5642_SetGain  */

/*************************************************************************
* FUNCTION
*    read_OV5642_gain
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
kal_uint16 read_OV5642_gain(void)
{
    return (kal_uint16)OV5642_read_cmos_sensor(0x350B);
}  /* read_OV5642_gain */

/*******************************************************************************
* 
********************************************************************************/
void write_OV5642_gain(kal_uint16 gain)
{
    OV5642_SetGain(gain);
}

/*******************************************************************************
* 
********************************************************************************/
void OV5642_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=OV5642SensorReg[i].Addr; i++)
    {
        OV5642_write_cmos_sensor(OV5642SensorReg[i].Addr, OV5642SensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV5642SensorReg[i].Addr; i++)
    {
        OV5642_write_cmos_sensor(OV5642SensorReg[i].Addr, OV5642SensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        OV5642_write_cmos_sensor(OV5642SensorCCT[i].Addr, OV5642SensorCCT[i].Para);
    }
}


/*************************************************************************
* FUNCTION
*    OV5642_sensor_to_camera_para
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
void OV5642_sensor_to_camera_para(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=OV5642SensorReg[i].Addr; i++)
    {
        OV5642SensorReg[i].Para = OV5642_read_cmos_sensor(OV5642SensorReg[i].Addr);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV5642SensorReg[i].Addr; i++)
    {
        OV5642SensorReg[i].Para = OV5642_read_cmos_sensor(OV5642SensorReg[i].Addr);
    }
}


/*************************************************************************
* FUNCTION
*    OV5642_get_sensor_group_count
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
kal_int32  OV5642_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void OV5642_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void OV5642_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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

            temp_para=OV5642SensorCCT[temp_addr].Para;

            temp_gain = OV5642Reg2Gain(temp_para);

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
                
                    //temp_reg=OV5642SensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=OV5642_MAX_EXPOSURE_LINES;
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

//void OV5642_set_isp_driving_current(kal_uint8 current)
//{
//}

kal_bool OV5642_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
   kal_uint16 temp_addr=0, temp_para=0;

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

            temp_para = OV5642Gain2Reg(ItemValue);


            OV5642SensorCCT[temp_addr].Para = temp_para;
            OV5642_write_cmos_sensor(OV5642SensorCCT[temp_addr].Addr,temp_para);

            OV5642_sensor_gain_base=read_OV5642_gain();

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    if(ItemValue==2)
                    {
                        OV5642SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
                        //OV5642_set_isp_driving_current(ISP_DRIVING_2MA);
                    }
                    else if(ItemValue==3 || ItemValue==4)
                    {
                        OV5642SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
                        //OV5642_set_isp_driving_current(ISP_DRIVING_4MA);
                    }
                    else if(ItemValue==5 || ItemValue==6)
                    {
                        OV5642SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
                        //OV5642_set_isp_driving_current(ISP_DRIVING_6MA);
                    }
                    else
                    {
                        OV5642SensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
                        //OV5642_set_isp_driving_current(ISP_DRIVING_8MA);
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
                    OV5642_FAC_SENSOR_REG=ItemValue;
                    break;
                case 1:
                    OV5642_write_cmos_sensor(OV5642_FAC_SENSOR_REG,ItemValue);
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

static void OV5642_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
    kal_uint16 PixelsOneline = OV5642_FULL_PERIOD_PIXEL_NUMS;
    if(OV5642_720P == OV5642_g_RES)
    {
        PixelsOneline = (OV5642_PV_PERIOD_PIXEL_NUMS + iPixels + OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS);
        if(OV5642_MPEG4_encode_mode == KAL_FALSE)//for Fix video framerate
            OV5642_CURRENT_FRAME_LINES = iLines + OV5642_PV_PERIOD_LINE_NUMS + OV5642_PV_PERIOD_EXTRA_LINE_NUMS ;
    }
    else if (OV5642_5M == OV5642_g_RES)
    {
        PixelsOneline = OV5642_FULL_PERIOD_PIXEL_NUMS + iPixels + OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS;
        OV5642_CURRENT_FRAME_LINES = iLines + OV5642_FULL_PERIOD_LINE_NUMS + OV5642_FULL_PERIOD_EXTRA_LINE_NUMS;
    }
    OV5642_write_cmos_sensor(0x380c, (PixelsOneline >> 8) & 0xFF);
    OV5642_write_cmos_sensor(0x380d, PixelsOneline & 0xFF);

#if 0 //! Don't set shutter related register 
    if(OV5642_MPEG4_encode_mode == KAL_FALSE)//for Fix video framerate,set the frame lines in night mode function
    {
        //Set dummy lines.
        //The maximum shutter value = Line_Without_Dummy + Dummy_lines
        OV5642_write_cmos_sensor(0x350C, (OV5642_CURRENT_FRAME_LINES >> 8) & 0xFF);
        OV5642_write_cmos_sensor(0x350D, OV5642_CURRENT_FRAME_LINES & 0xFF);
        OV5642_MAX_EXPOSURE_LINES = OV5642_CURRENT_FRAME_LINES - OV5642_SHUTTER_LINES_GAP;
    }
#endif     
}   /*  OV5642_SetDummy */


/*******************************************************************************
*
********************************************************************************/
static void OV5642_Sensor_Init(void)
{
    OV5642_write_cmos_sensor(0x3103, 0x93);//PCLK SELECT
    OV5642_write_cmos_sensor(0x3008, 0x82);//SYSTEM CONTROL SOFT RESET
    mDELAY(2);
    
    OV5642_write_cmos_sensor(0x3017, 0x7f);//PAD OUTPUT ENABLE
    OV5642_write_cmos_sensor(0x3018, 0xfc);//PAD OUTPUT ENABLE
    OV5642_write_cmos_sensor(0x302c, 0xC0); //IO Driving, 4x output Control ,
    //OV5642_write_cmos_sensor(0x3810, 0xc2);//H and V offset setting
    OV5642_write_cmos_sensor(0x3615, 0xf0);//
    OV5642_write_cmos_sensor(0x3000, 0x00);
    OV5642_write_cmos_sensor(0x3001, 0x00);
    OV5642_write_cmos_sensor(0x3002, 0x5c);
    OV5642_write_cmos_sensor(0x3003, 0x00);
    OV5642_write_cmos_sensor(0x3004, 0xff);
    OV5642_write_cmos_sensor(0x3005, 0xff);
    OV5642_write_cmos_sensor(0x3006, 0x43);
    OV5642_write_cmos_sensor(0x3007, 0x37);
    OV5642_write_cmos_sensor(0x3011, 0x0a);

    OV5642_write_cmos_sensor(0x3012, 0x02);
    
    OV5642_write_cmos_sensor(0x3010, 0x00);

    OV5642_write_cmos_sensor(0x360c, 0x20);

    OV5642_write_cmos_sensor(0x3815, 0x04);

    OV5642_write_cmos_sensor(0x370c, 0xa0);
    OV5642_write_cmos_sensor(0x3602, 0xfc);
    OV5642_write_cmos_sensor(0x3612, 0xff);

    OV5642_write_cmos_sensor(0x3634, 0xc0);
    OV5642_write_cmos_sensor(0x3613, 0x00);

    OV5642_write_cmos_sensor(0x3605, 0x7c);
    OV5642_write_cmos_sensor(0x3621, 0x09);
    OV5642_write_cmos_sensor(0x3622, 0x60);

    OV5642_write_cmos_sensor(0x3604, 0x40);
    OV5642_write_cmos_sensor(0x3603, 0xa7);
    OV5642_write_cmos_sensor(0x3603, 0x27);
    OV5642_write_cmos_sensor(0x4000, 0x21);

    OV5642_write_cmos_sensor(0x401d, 0x22);
    OV5642_write_cmos_sensor(0x3600, 0x54);
    OV5642_write_cmos_sensor(0x3605, 0x04);
    OV5642_write_cmos_sensor(0x3606, 0x3f);
    OV5642_write_cmos_sensor(0x3c01, 0x80);
    OV5642_write_cmos_sensor(0x5000, 0x4f);
    OV5642_write_cmos_sensor(0x5020, 0x04);
    OV5642_write_cmos_sensor(0x5181, 0x79);

    OV5642_write_cmos_sensor(0x5182, 0x00);
    OV5642_write_cmos_sensor(0x5185, 0x22);
    OV5642_write_cmos_sensor(0x5179, 0x01);
    OV5642_write_cmos_sensor(0x5001, 0xff);
    OV5642_write_cmos_sensor(0x5500, 0x0a);
    OV5642_write_cmos_sensor(0x5504, 0x00);
    OV5642_write_cmos_sensor(0x5505, 0x7f);
    OV5642_write_cmos_sensor(0x5080, 0x08);
    OV5642_write_cmos_sensor(0x300e, 0x18);
    OV5642_write_cmos_sensor(0x4610, 0x00);
    OV5642_write_cmos_sensor(0x471d, 0x05);

    OV5642_write_cmos_sensor(0x4708, 0x06);
    OV5642_write_cmos_sensor(0x3808, 0x05);
    OV5642_write_cmos_sensor(0x3809, 0x10);
    OV5642_write_cmos_sensor(0x380a, 0x03);
    OV5642_write_cmos_sensor(0x380b, 0xcc);
    OV5642_write_cmos_sensor(0x380e, 0x07);

    OV5642_write_cmos_sensor(0x380f, 0xd0);
    OV5642_write_cmos_sensor(0x501f, 0x00);
    OV5642_write_cmos_sensor(0x5000, 0x4f);
    OV5642_write_cmos_sensor(0x4300, 0x30);
    OV5642_write_cmos_sensor(0x3503, 0x07);
    OV5642_write_cmos_sensor(0x3501, 0x73);

    OV5642_write_cmos_sensor(0x3502, 0x80);
    OV5642_write_cmos_sensor(0x350b, 0x00);
    OV5642_write_cmos_sensor(0x3503, 0x07);
    OV5642_write_cmos_sensor(0x3824, 0x11);
    OV5642_write_cmos_sensor(0x3825, 0xb0);
    OV5642_write_cmos_sensor(0x3501, 0x1e);
    OV5642_write_cmos_sensor(0x3502, 0x80);

    OV5642_write_cmos_sensor(0x350b, 0x7f);
    OV5642_write_cmos_sensor(0x380c, 0x07);
    OV5642_write_cmos_sensor(0x380d, 0xc0);

    OV5642_write_cmos_sensor(0x380e, 0x03);
    OV5642_write_cmos_sensor(0x380f, 0xf0);
    OV5642_write_cmos_sensor(0x3a0d, 0x04);
    OV5642_write_cmos_sensor(0x3a0e, 0x03);
    OV5642_write_cmos_sensor(0x3818, 0xc1);

    OV5642_write_cmos_sensor(0x3705, 0xdb);
    OV5642_write_cmos_sensor(0x370a, 0x81);
    OV5642_write_cmos_sensor(0x3801, 0x80);

    OV5642_write_cmos_sensor(0x3621, 0xc7);
    OV5642_write_cmos_sensor(0x3801, 0x50);
    OV5642_write_cmos_sensor(0x3803, 0x08);
    OV5642_write_cmos_sensor(0x3827, 0x08);

    OV5642_write_cmos_sensor(0x3810, 0x42);
    OV5642_write_cmos_sensor(0x3804, 0x05);
    OV5642_write_cmos_sensor(0x3805, 0x10);
    OV5642_write_cmos_sensor(0x5682, 0x05);
    OV5642_write_cmos_sensor(0x5683, 0x10);
    OV5642_write_cmos_sensor(0x3806, 0x03);
    OV5642_write_cmos_sensor(0x3807, 0xcc);
    OV5642_write_cmos_sensor(0x5686, 0x03);
    OV5642_write_cmos_sensor(0x5687, 0xc4);

    OV5642_write_cmos_sensor(0x3a00, 0x78);
    OV5642_write_cmos_sensor(0x3a1a, 0x05);
    OV5642_write_cmos_sensor(0x3a13, 0x30);
    OV5642_write_cmos_sensor(0x3a18, 0x00);
    OV5642_write_cmos_sensor(0x3a19, 0x7c);
    OV5642_write_cmos_sensor(0x3a08, 0x12);

    OV5642_write_cmos_sensor(0x3a09, 0xc0);
    OV5642_write_cmos_sensor(0x3a0a, 0x0f);
    OV5642_write_cmos_sensor(0x3a0b, 0xa0);
    OV5642_write_cmos_sensor(0x350c, 0x07);
    OV5642_write_cmos_sensor(0x350d, 0xd0);
    OV5642_write_cmos_sensor(0x3500, 0x00);
    OV5642_write_cmos_sensor(0x3501, 0x00);
    OV5642_write_cmos_sensor(0x3502, 0x00);

    OV5642_write_cmos_sensor(0x350a, 0x00);
    OV5642_write_cmos_sensor(0x350b, 0x00);
    //OV5642_write_cmos_sensor(0x3503, 0x00);//disable AEC
    OV5642_write_cmos_sensor(0x528a, 0x02);
    OV5642_write_cmos_sensor(0x528b, 0x04);
    OV5642_write_cmos_sensor(0x528c, 0x08);
    OV5642_write_cmos_sensor(0x528d, 0x08);
    OV5642_write_cmos_sensor(0x528e, 0x08);
    OV5642_write_cmos_sensor(0x528f, 0x10);
    OV5642_write_cmos_sensor(0x5290, 0x10);
    OV5642_write_cmos_sensor(0x5292, 0x00);
    OV5642_write_cmos_sensor(0x5293, 0x02);
    OV5642_write_cmos_sensor(0x5294, 0x00);
    OV5642_write_cmos_sensor(0x5295, 0x02);
    OV5642_write_cmos_sensor(0x5296, 0x00);
    OV5642_write_cmos_sensor(0x5297, 0x02);
    OV5642_write_cmos_sensor(0x5298, 0x00);
    OV5642_write_cmos_sensor(0x5299, 0x02);
    OV5642_write_cmos_sensor(0x529a, 0x00);
    OV5642_write_cmos_sensor(0x529b, 0x02);
    OV5642_write_cmos_sensor(0x529c, 0x00);
    OV5642_write_cmos_sensor(0x529d, 0x02);
    OV5642_write_cmos_sensor(0x529e, 0x00);
    OV5642_write_cmos_sensor(0x529f, 0x02);

    OV5642_write_cmos_sensor(0x3a0f, 0x3c);
    OV5642_write_cmos_sensor(0x3a10, 0x30);
    OV5642_write_cmos_sensor(0x3a1b, 0x3c);
    OV5642_write_cmos_sensor(0x3a1e, 0x30);
    OV5642_write_cmos_sensor(0x3a11, 0x70);
    OV5642_write_cmos_sensor(0x3a1f, 0x10);
    OV5642_write_cmos_sensor(0x3030, 0x2b);

    OV5642_write_cmos_sensor(0x3a02, 0x00);
    OV5642_write_cmos_sensor(0x3a03, 0x7d);
    OV5642_write_cmos_sensor(0x3a04, 0x00);
    OV5642_write_cmos_sensor(0x3a15, 0x7d);
    OV5642_write_cmos_sensor(0x3a16, 0x00);

    OV5642_write_cmos_sensor(0x3a02, 0x00);
    OV5642_write_cmos_sensor(0x3a03, 0x7d);
    OV5642_write_cmos_sensor(0x3a04, 0x00);
    OV5642_write_cmos_sensor(0x3a15, 0x7d);
    OV5642_write_cmos_sensor(0x3a16, 0x00);

    OV5642_write_cmos_sensor(0x3a00, 0x78);
    OV5642_write_cmos_sensor(0x3a08, 0x12);
    OV5642_write_cmos_sensor(0x3a09, 0xc0);
    OV5642_write_cmos_sensor(0x3a0a, 0x0f);
    OV5642_write_cmos_sensor(0x3a0b, 0xa0);

    OV5642_write_cmos_sensor(0x3a0d, 0x04);
    OV5642_write_cmos_sensor(0x3a0e, 0x03);
    OV5642_write_cmos_sensor(0x5193, 0x70);
    OV5642_write_cmos_sensor(0x589b, 0x04);
    OV5642_write_cmos_sensor(0x589a, 0xc5);

    OV5642_write_cmos_sensor(0x4001, 0x42);
    OV5642_write_cmos_sensor(0x401c, 0x04);
    OV5642_write_cmos_sensor(0x528a, 0x01);
    OV5642_write_cmos_sensor(0x528b, 0x04);
    OV5642_write_cmos_sensor(0x528c, 0x08);

    OV5642_write_cmos_sensor(0x528d, 0x10);
    OV5642_write_cmos_sensor(0x528e, 0x20);
    OV5642_write_cmos_sensor(0x528f, 0x28);
    OV5642_write_cmos_sensor(0x5290, 0x30);
    OV5642_write_cmos_sensor(0x5292, 0x00);

    OV5642_write_cmos_sensor(0x5293, 0x01);
    OV5642_write_cmos_sensor(0x5294, 0x00);
    OV5642_write_cmos_sensor(0x5295, 0x04);
    OV5642_write_cmos_sensor(0x5296, 0x00);
    OV5642_write_cmos_sensor(0x5297, 0x08);

    OV5642_write_cmos_sensor(0x5298, 0x00);
    OV5642_write_cmos_sensor(0x5299, 0x10);
    OV5642_write_cmos_sensor(0x529a, 0x00);
    OV5642_write_cmos_sensor(0x529b, 0x20);
    OV5642_write_cmos_sensor(0x529c, 0x00);

    OV5642_write_cmos_sensor(0x529d, 0x28);
    OV5642_write_cmos_sensor(0x529e, 0x00);
    OV5642_write_cmos_sensor(0x529f, 0x30);
    OV5642_write_cmos_sensor(0x5282, 0x00);
    OV5642_write_cmos_sensor(0x5300, 0x00);

    OV5642_write_cmos_sensor(0x5301, 0x20);
    OV5642_write_cmos_sensor(0x5302, 0x00);
    OV5642_write_cmos_sensor(0x5303, 0x7c);
    OV5642_write_cmos_sensor(0x530c, 0x00);
    OV5642_write_cmos_sensor(0x530d, 0x0c);

    OV5642_write_cmos_sensor(0x530e, 0x20);
    OV5642_write_cmos_sensor(0x530f, 0x80);
    OV5642_write_cmos_sensor(0x5310, 0x20);
    OV5642_write_cmos_sensor(0x5311, 0x80);
    OV5642_write_cmos_sensor(0x5308, 0x20);

    OV5642_write_cmos_sensor(0x5309, 0x40);
    OV5642_write_cmos_sensor(0x5304, 0x00);
    OV5642_write_cmos_sensor(0x5305, 0x30);
    OV5642_write_cmos_sensor(0x5306, 0x00);
    OV5642_write_cmos_sensor(0x5307, 0x80);

    OV5642_write_cmos_sensor(0x5314, 0x08);
    OV5642_write_cmos_sensor(0x5315, 0x20);
    OV5642_write_cmos_sensor(0x5319, 0x30);
    OV5642_write_cmos_sensor(0x5316, 0x10);
    OV5642_write_cmos_sensor(0x5317, 0x08);
    OV5642_write_cmos_sensor(0x5318, 0x02);
    OV5642_write_cmos_sensor(0x5380, 0x01);  
    OV5642_write_cmos_sensor(0x5381, 0x00);  
    OV5642_write_cmos_sensor(0x5382, 0x00);  
    OV5642_write_cmos_sensor(0x5383, 0x4e);  
    OV5642_write_cmos_sensor(0x5384, 0x00);  
    OV5642_write_cmos_sensor(0x5385, 0x0f);  
    OV5642_write_cmos_sensor(0x5386, 0x00);
    OV5642_write_cmos_sensor(0x5387, 0x00);  
    OV5642_write_cmos_sensor(0x5388, 0x01);  
    OV5642_write_cmos_sensor(0x5389, 0x15);  
    OV5642_write_cmos_sensor(0x538a, 0x00);
    OV5642_write_cmos_sensor(0x538b, 0x31);  
    OV5642_write_cmos_sensor(0x538c, 0x00);  
    OV5642_write_cmos_sensor(0x538d, 0x00);
    OV5642_write_cmos_sensor(0x538e, 0x00);  
    OV5642_write_cmos_sensor(0x538f, 0x0f);  
    OV5642_write_cmos_sensor(0x5390, 0x00);  
    OV5642_write_cmos_sensor(0x5391, 0xab);  
    OV5642_write_cmos_sensor(0x5392, 0x00);  
    OV5642_write_cmos_sensor(0x5393, 0xa2);  
    OV5642_write_cmos_sensor(0x5394, 0x08);  
    OV5642_write_cmos_sensor(0x5480, 0x14);  
    OV5642_write_cmos_sensor(0x5481, 0x21);  
    OV5642_write_cmos_sensor(0x5482, 0x36);  
    OV5642_write_cmos_sensor(0x5483, 0x57);  
    OV5642_write_cmos_sensor(0x5484, 0x65);  
    OV5642_write_cmos_sensor(0x5485, 0x71);  
    OV5642_write_cmos_sensor(0x5486, 0x7d);  
    OV5642_write_cmos_sensor(0x5487, 0x87);  
    OV5642_write_cmos_sensor(0x5488, 0x91);  
    OV5642_write_cmos_sensor(0x5489, 0x9a);
    OV5642_write_cmos_sensor(0x548a, 0xaa);
    OV5642_write_cmos_sensor(0x548b, 0xb8);  
    OV5642_write_cmos_sensor(0x548c, 0xcd);  
    OV5642_write_cmos_sensor(0x548d, 0xdd);  
    OV5642_write_cmos_sensor(0x548e, 0xea);  
    OV5642_write_cmos_sensor(0x548f, 0x10);  
    OV5642_write_cmos_sensor(0x5490, 0x05);  
    OV5642_write_cmos_sensor(0x5491, 0x00);  
    OV5642_write_cmos_sensor(0x5492, 0x04);  
    OV5642_write_cmos_sensor(0x5493, 0x20);  
    OV5642_write_cmos_sensor(0x5494, 0x03);  
    OV5642_write_cmos_sensor(0x5495, 0x60);  
    OV5642_write_cmos_sensor(0x5496, 0x02);  
    OV5642_write_cmos_sensor(0x5497, 0xb8);  
    OV5642_write_cmos_sensor(0x5498, 0x02);  
    OV5642_write_cmos_sensor(0x5499, 0x86);  
    OV5642_write_cmos_sensor(0x549a, 0x02);  
    OV5642_write_cmos_sensor(0x549b, 0x5b);
    OV5642_write_cmos_sensor(0x549c, 0x02);  
    OV5642_write_cmos_sensor(0x549d, 0x3b);
    OV5642_write_cmos_sensor(0x549e, 0x02);  
    OV5642_write_cmos_sensor(0x549f, 0x1c);  
    OV5642_write_cmos_sensor(0x54a0, 0x02);  
    OV5642_write_cmos_sensor(0x54a1, 0x04);  
    OV5642_write_cmos_sensor(0x54a2, 0x01);  
    OV5642_write_cmos_sensor(0x54a3, 0xed);
    OV5642_write_cmos_sensor(0x54a4, 0x01);  
    OV5642_write_cmos_sensor(0x54a5, 0xc5);  
    OV5642_write_cmos_sensor(0x54a6, 0x01);  
    OV5642_write_cmos_sensor(0x54a7, 0xa5);  
    OV5642_write_cmos_sensor(0x54a8, 0x01);  
    OV5642_write_cmos_sensor(0x54a9, 0x6c);  
    OV5642_write_cmos_sensor(0x54aa, 0x01);  
    OV5642_write_cmos_sensor(0x54ab, 0x41);  
    OV5642_write_cmos_sensor(0x54ac, 0x01);  
    OV5642_write_cmos_sensor(0x54ad, 0x20);  
    OV5642_write_cmos_sensor(0x54ae, 0x00);  
    OV5642_write_cmos_sensor(0x54af, 0x16);  
    OV5642_write_cmos_sensor(0x3406, 0x01);  //00
    OV5642_write_cmos_sensor(0x5192, 0x04);  
    OV5642_write_cmos_sensor(0x5191, 0xf8);  
    OV5642_write_cmos_sensor(0x5193, 0x70);  
    OV5642_write_cmos_sensor(0x5194, 0xf0);  
    OV5642_write_cmos_sensor(0x5195, 0xf0);  
    OV5642_write_cmos_sensor(0x518d, 0x3d);  
    OV5642_write_cmos_sensor(0x518f, 0x54);  
    OV5642_write_cmos_sensor(0x518e, 0x3d);  
    OV5642_write_cmos_sensor(0x5190, 0x54);  
    OV5642_write_cmos_sensor(0x518b, 0xa8);  
    OV5642_write_cmos_sensor(0x518c, 0xa8);
    OV5642_write_cmos_sensor(0x5187, 0x18);  
    OV5642_write_cmos_sensor(0x5188, 0x18);  
    OV5642_write_cmos_sensor(0x5189, 0x6e);  
    OV5642_write_cmos_sensor(0x518a, 0x68);  
    OV5642_write_cmos_sensor(0x5186, 0x1c);  
    OV5642_write_cmos_sensor(0x5181, 0x50);  
    OV5642_write_cmos_sensor(0x5184, 0x25);  
    OV5642_write_cmos_sensor(0x5182, 0x11);  
    OV5642_write_cmos_sensor(0x5183, 0x14);  
    OV5642_write_cmos_sensor(0x5184, 0x25);  
    OV5642_write_cmos_sensor(0x5185, 0x24);  
    OV5642_write_cmos_sensor(0x5025, 0x80); //RAW 
    OV5642_write_cmos_sensor(0x3a0f, 0x7e);  
    OV5642_write_cmos_sensor(0x3a10, 0x72);  
    OV5642_write_cmos_sensor(0x3a1b, 0x80);
    OV5642_write_cmos_sensor(0x3a1e, 0x70);  
    OV5642_write_cmos_sensor(0x3a11, 0xd0);  
    OV5642_write_cmos_sensor(0x3a1f, 0x40);  
    OV5642_write_cmos_sensor(0x5583, 0x40);  
    OV5642_write_cmos_sensor(0x5584, 0x40);  
    OV5642_write_cmos_sensor(0x5580, 0x02);  
    OV5642_write_cmos_sensor(0x5000, 0xcf);  
    OV5642_write_cmos_sensor(0x5800, 0x27);  
    OV5642_write_cmos_sensor(0x5801, 0x19);  
    OV5642_write_cmos_sensor(0x5802, 0x12);  
    OV5642_write_cmos_sensor(0x5803, 0x0f);  
    OV5642_write_cmos_sensor(0x5804, 0x10);  
    OV5642_write_cmos_sensor(0x5805, 0x15);  
    OV5642_write_cmos_sensor(0x5806, 0x1e);  
    OV5642_write_cmos_sensor(0x5807, 0x2f);  
    OV5642_write_cmos_sensor(0x5808, 0x15);  
    OV5642_write_cmos_sensor(0x5809, 0x0d);  
    OV5642_write_cmos_sensor(0x580a, 0x0a);  
    OV5642_write_cmos_sensor(0x580b, 0x09);
    OV5642_write_cmos_sensor(0x580c, 0x0a);  
    OV5642_write_cmos_sensor(0x580d, 0x0c);  
    OV5642_write_cmos_sensor(0x580e, 0x12);  
    OV5642_write_cmos_sensor(0x580f, 0x19);  
    OV5642_write_cmos_sensor(0x5810, 0x0b);  
    OV5642_write_cmos_sensor(0x5811, 0x07);  
    OV5642_write_cmos_sensor(0x5812, 0x04);  
    OV5642_write_cmos_sensor(0x5813, 0x03);  
    OV5642_write_cmos_sensor(0x5814, 0x03);  
    OV5642_write_cmos_sensor(0x5815, 0x06);  
    OV5642_write_cmos_sensor(0x5816, 0x0a);  
    OV5642_write_cmos_sensor(0x5817, 0x0f);  
    OV5642_write_cmos_sensor(0x5818, 0x0a);  
    OV5642_write_cmos_sensor(0x5819, 0x05);  
    OV5642_write_cmos_sensor(0x581a, 0x01);  
    OV5642_write_cmos_sensor(0x581b, 0x00);  
    OV5642_write_cmos_sensor(0x581c, 0x00);
    OV5642_write_cmos_sensor(0x581d, 0x03);
    OV5642_write_cmos_sensor(0x581e, 0x08);  
    OV5642_write_cmos_sensor(0x581f, 0x0c);  
    OV5642_write_cmos_sensor(0x5820, 0x0a);  
    OV5642_write_cmos_sensor(0x5821, 0x05);  
    OV5642_write_cmos_sensor(0x5822, 0x01);  
    OV5642_write_cmos_sensor(0x5823, 0x00);  
    OV5642_write_cmos_sensor(0x5824, 0x00);  
    OV5642_write_cmos_sensor(0x5825, 0x03);  
    OV5642_write_cmos_sensor(0x5826, 0x08);  
    OV5642_write_cmos_sensor(0x5827, 0x0c);  
    OV5642_write_cmos_sensor(0x5828, 0x0e);
    OV5642_write_cmos_sensor(0x5829, 0x08);  
    OV5642_write_cmos_sensor(0x582a, 0x06);  
    OV5642_write_cmos_sensor(0x582b, 0x04);  
    OV5642_write_cmos_sensor(0x582c, 0x05);  
    OV5642_write_cmos_sensor(0x582d, 0x07);  
    OV5642_write_cmos_sensor(0x582e, 0x0b);  
    OV5642_write_cmos_sensor(0x582f, 0x12);  
    OV5642_write_cmos_sensor(0x5830, 0x18);  
    OV5642_write_cmos_sensor(0x5831, 0x10);  
    OV5642_write_cmos_sensor(0x5832, 0x0c);  
    OV5642_write_cmos_sensor(0x5833, 0x0a);  
    OV5642_write_cmos_sensor(0x5834, 0x0b);  
    OV5642_write_cmos_sensor(0x5835, 0x0e);  
    OV5642_write_cmos_sensor(0x5836, 0x15);  
    OV5642_write_cmos_sensor(0x5837, 0x19);  
    OV5642_write_cmos_sensor(0x5838, 0x32);  
    OV5642_write_cmos_sensor(0x5839, 0x1f);  
    OV5642_write_cmos_sensor(0x583a, 0x18);  
    OV5642_write_cmos_sensor(0x583b, 0x16);  
    OV5642_write_cmos_sensor(0x583c, 0x17);
    OV5642_write_cmos_sensor(0x583d, 0x1e);  
    OV5642_write_cmos_sensor(0x583e, 0x26);  
    OV5642_write_cmos_sensor(0x583f, 0x53);  
    OV5642_write_cmos_sensor(0x5840, 0x10);  
    OV5642_write_cmos_sensor(0x5841, 0x0f);  
    OV5642_write_cmos_sensor(0x5842, 0x0d);  
    OV5642_write_cmos_sensor(0x5843, 0x0c);  
    OV5642_write_cmos_sensor(0x5844, 0x0e);  
    OV5642_write_cmos_sensor(0x5845, 0x09);  
    OV5642_write_cmos_sensor(0x5846, 0x11);  
    OV5642_write_cmos_sensor(0x5847, 0x10);  
    OV5642_write_cmos_sensor(0x5848, 0x10);  
    OV5642_write_cmos_sensor(0x5849, 0x10);  
    OV5642_write_cmos_sensor(0x584a, 0x10);  
    OV5642_write_cmos_sensor(0x584b, 0x0e);  
    OV5642_write_cmos_sensor(0x584c, 0x10);  
    OV5642_write_cmos_sensor(0x584d, 0x10);  
    OV5642_write_cmos_sensor(0x584e, 0x11);  
    OV5642_write_cmos_sensor(0x584f, 0x10);  
    OV5642_write_cmos_sensor(0x5850, 0x0f);  
    OV5642_write_cmos_sensor(0x5851, 0x0c);  
    OV5642_write_cmos_sensor(0x5852, 0x0f);  
    OV5642_write_cmos_sensor(0x5853, 0x10);  
    OV5642_write_cmos_sensor(0x5854, 0x10);  
    OV5642_write_cmos_sensor(0x5855, 0x0f);  
    OV5642_write_cmos_sensor(0x5856, 0x0e);  
    OV5642_write_cmos_sensor(0x5857, 0x0b);  
    OV5642_write_cmos_sensor(0x5858, 0x10);  
    OV5642_write_cmos_sensor(0x5859, 0x0d);  
    OV5642_write_cmos_sensor(0x585a, 0x0d);  
    OV5642_write_cmos_sensor(0x585b, 0x0c);  
    OV5642_write_cmos_sensor(0x585c, 0x0c);
    OV5642_write_cmos_sensor(0x585d, 0x0c);  
    OV5642_write_cmos_sensor(0x585e, 0x0b);  
    OV5642_write_cmos_sensor(0x585f, 0x0c);
    OV5642_write_cmos_sensor(0x5860, 0x0c);  
    OV5642_write_cmos_sensor(0x5861, 0x0c);  
    OV5642_write_cmos_sensor(0x5862, 0x0d);  
    OV5642_write_cmos_sensor(0x5863, 0x08);  
    OV5642_write_cmos_sensor(0x5864, 0x11);  
    OV5642_write_cmos_sensor(0x5865, 0x18);  
    OV5642_write_cmos_sensor(0x5866, 0x18);  
    OV5642_write_cmos_sensor(0x5867, 0x19);  
    OV5642_write_cmos_sensor(0x5868, 0x17);  
    OV5642_write_cmos_sensor(0x5869, 0x19);  
    OV5642_write_cmos_sensor(0x586a, 0x16);  
    OV5642_write_cmos_sensor(0x586b, 0x13);  
    OV5642_write_cmos_sensor(0x586c, 0x13);  
    OV5642_write_cmos_sensor(0x586d, 0x12);  
    OV5642_write_cmos_sensor(0x586e, 0x13);  
    OV5642_write_cmos_sensor(0x586f, 0x16);  
    OV5642_write_cmos_sensor(0x5870, 0x14);  
    OV5642_write_cmos_sensor(0x5871, 0x12);  
    OV5642_write_cmos_sensor(0x5872, 0x10);  
    OV5642_write_cmos_sensor(0x5873, 0x11);  
    OV5642_write_cmos_sensor(0x5874, 0x11);  
    OV5642_write_cmos_sensor(0x5875, 0x16);  
    OV5642_write_cmos_sensor(0x5876, 0x14);  
    OV5642_write_cmos_sensor(0x5877, 0x11);  
    OV5642_write_cmos_sensor(0x5878, 0x10);  
    OV5642_write_cmos_sensor(0x5879, 0x0f);  
    OV5642_write_cmos_sensor(0x587a, 0x10);
    OV5642_write_cmos_sensor(0x587b, 0x14);
    OV5642_write_cmos_sensor(0x587c, 0x13);  
    OV5642_write_cmos_sensor(0x587d, 0x12);
    OV5642_write_cmos_sensor(0x587e, 0x11);  
    OV5642_write_cmos_sensor(0x587f, 0x11);  
    OV5642_write_cmos_sensor(0x5880, 0x12);  
    OV5642_write_cmos_sensor(0x5881, 0x15);  
    OV5642_write_cmos_sensor(0x5882, 0x14);  
    OV5642_write_cmos_sensor(0x5883, 0x15);  
    OV5642_write_cmos_sensor(0x5884, 0x15);  
    OV5642_write_cmos_sensor(0x5885, 0x15);
    OV5642_write_cmos_sensor(0x5886, 0x13);  
    OV5642_write_cmos_sensor(0x5887, 0x17);  
    OV5642_write_cmos_sensor(0x3710, 0x10);  
    OV5642_write_cmos_sensor(0x3632, 0x51);  
    OV5642_write_cmos_sensor(0x3702, 0x10);  
    OV5642_write_cmos_sensor(0x3703, 0xb2);  
    OV5642_write_cmos_sensor(0x3704, 0x18);  
    OV5642_write_cmos_sensor(0x370b, 0x40);  
    OV5642_write_cmos_sensor(0x370d, 0x03);  
    OV5642_write_cmos_sensor(0x3631, 0x01);  
    OV5642_write_cmos_sensor(0x3632, 0x52);  
    OV5642_write_cmos_sensor(0x3606, 0x24);  
    OV5642_write_cmos_sensor(0x3620, 0x96);  
    OV5642_write_cmos_sensor(0x5785, 0x07);  
    OV5642_write_cmos_sensor(0x3a13, 0x30);  
    OV5642_write_cmos_sensor(0x3600, 0x52);  
    OV5642_write_cmos_sensor(0x3604, 0x48);  
    OV5642_write_cmos_sensor(0x3606, 0x1b);  
    OV5642_write_cmos_sensor(0x370d, 0x0b);  
    OV5642_write_cmos_sensor(0x370f, 0xc0);
    OV5642_write_cmos_sensor(0x3709, 0x01);  
    OV5642_write_cmos_sensor(0x3823, 0x00);  
    OV5642_write_cmos_sensor(0x5007, 0x00);  
    OV5642_write_cmos_sensor(0x5009, 0x00);  
    OV5642_write_cmos_sensor(0x5011, 0x00);  
    OV5642_write_cmos_sensor(0x5013, 0x00);
    OV5642_write_cmos_sensor(0x519e, 0x00);  
    OV5642_write_cmos_sensor(0x5086, 0x00);  
    OV5642_write_cmos_sensor(0x5087, 0x00);  
    OV5642_write_cmos_sensor(0x5088, 0x00);  
    OV5642_write_cmos_sensor(0x5089, 0x00);  
    OV5642_write_cmos_sensor(0x302b, 0x00);  
    OV5642_write_cmos_sensor(0x3621, 0x87);  
    OV5642_write_cmos_sensor(0x3010, 0x10);  

    OV5642_write_cmos_sensor(0x501f, 0x03);  
    OV5642_write_cmos_sensor(0x4300, 0xf8);  	
    OV5642_write_cmos_sensor(0x5000, 0x06);  

    OV5642_write_cmos_sensor(0x5001, 0x01); //00 
    OV5642_write_cmos_sensor(0x3c01, 0x80);  
    OV5642_write_cmos_sensor(0x3c00, 0x04); 

    OV5642_write_cmos_sensor(0x3503, 0x07);
    OV5642_write_cmos_sensor(0x350c, 0x03);//
    OV5642_write_cmos_sensor(0x350d, 0xf0);//
    OV5642_write_cmos_sensor(0x3011, 0x08);//
    OV5642_write_cmos_sensor(0x3010, 0x00);//

    //OV5642_write_cmos_sensor(0x3011, 0x0f);//

    //OV5642_write_cmos_sensor(0x3010, 0x10);//
    //OV5642_write_cmos_sensor(0x3012, 0x02);//

    OV5642_write_cmos_sensor(0x5001, 0x01); //00 awb enable
    OV5642_write_cmos_sensor(0x3406, 0x01); //00 manul awb

    //PLL setting
    OV5642_write_cmos_sensor(0x300f, 0x6);
    OV5642_write_cmos_sensor(0x3010, 0x10);
    OV5642_write_cmos_sensor(0x3011, 0x11);
    OV5642_write_cmos_sensor(0x3012, 0x2);
    OV5642_write_cmos_sensor(0x3815, 0x2);
    OV5642_write_cmos_sensor(0x3029, 0x0);
    OV5642_write_cmos_sensor(0x3033, 0x3);
    OV5642_write_cmos_sensor(0x460c, 0x22);
    printk("[OV5642Raw] Init Success \n");
}   /*  OV5642_Sensor_Init  */

void OV5642_set_720P(void)
{
    //int temp;
    int dummy_pixels, dummy_lines;
    OV5642_g_RES = OV5642_720P;
        //SENSORDB("OV5642_set_720P Start \n"); 
    OV5642_write_cmos_sensor(0x3503,0x7 );
    OV5642_write_cmos_sensor(0x3001,0x0 );
    OV5642_write_cmos_sensor(0x3002,0x5c);
    OV5642_write_cmos_sensor(0x300d,0x22);
    OV5642_write_cmos_sensor(0x3011,0x11 );    
    
    OV5642_write_cmos_sensor(0x3622,0x60);
    OV5642_write_cmos_sensor(0x3621,0x87);
    OV5642_write_cmos_sensor(0x3709,0x1 );
    OV5642_write_cmos_sensor(0x3600,0x52);
    OV5642_write_cmos_sensor(0x3602,0xfc);
    OV5642_write_cmos_sensor(0x3606,0x1b);
    OV5642_write_cmos_sensor(0x3612,0xff);
    OV5642_write_cmos_sensor(0x3613,0x0 );
    OV5642_write_cmos_sensor(0x3623,0x1 );
    OV5642_write_cmos_sensor(0x3705,0xdb);
    OV5642_write_cmos_sensor(0x370a,0x81);
    OV5642_write_cmos_sensor(0x3801,0x50);
    OV5642_write_cmos_sensor(0x3804,0x5 );
    OV5642_write_cmos_sensor(0x3805,0x10);
    OV5642_write_cmos_sensor(0x3806,0x3 );
    OV5642_write_cmos_sensor(0x3807,0xcc);
    OV5642_write_cmos_sensor(0x3808,0x5 );
    OV5642_write_cmos_sensor(0x3809,0x10);
    OV5642_write_cmos_sensor(0x380a,0x3 );
    OV5642_write_cmos_sensor(0x380b,0xcc);
    OV5642_write_cmos_sensor(0x380c,0x7 );
    OV5642_write_cmos_sensor(0x380d,0xc0);
    OV5642_write_cmos_sensor(0x380e,0x3 );
    OV5642_write_cmos_sensor(0x380f,0xf0);
    
    OV5642_write_cmos_sensor(0x3810,0x42);
    OV5642_write_cmos_sensor(0x3818,0xc1);
    OV5642_write_cmos_sensor(0x3824,0x11);
    OV5642_write_cmos_sensor(0x3825,0xb0);
    OV5642_write_cmos_sensor(0x3827,0x8 );
    OV5642_write_cmos_sensor(0x3a1a,0x5 );
    OV5642_write_cmos_sensor(0x3a08,0x12);
    OV5642_write_cmos_sensor(0x3a09,0xc0);
    OV5642_write_cmos_sensor(0x3a0a,0xf );
    OV5642_write_cmos_sensor(0x3a0b,0xa0);
    OV5642_write_cmos_sensor(0x3a0d,0x4 );
    OV5642_write_cmos_sensor(0x3a0e,0x3 );
    OV5642_write_cmos_sensor(0x401c,0x4 );
    OV5642_write_cmos_sensor(0x5000,0x6 );
    OV5642_write_cmos_sensor(0x5001,0x1 );//00
    OV5642_write_cmos_sensor(0x5005,0xdc);
    OV5642_write_cmos_sensor(0x3818, 0xa1);	//Set mirror and flip
    OV5642_write_cmos_sensor(0x3621, 0xe7);	

    //PLL setting, clock = 55.25, fps = 30fps 
    OV5642_write_cmos_sensor(0x300f, 0x6);
    OV5642_write_cmos_sensor(0x3010, 0x10);
    OV5642_write_cmos_sensor(0x3011, 0x11);
    OV5642_write_cmos_sensor(0x3012, 0x2);
    OV5642_write_cmos_sensor(0x3815, 0x2);
    OV5642_write_cmos_sensor(0x3029, 0x0);
    OV5642_write_cmos_sensor(0x3033, 0x3);
    OV5642_write_cmos_sensor(0x460c, 0x22);    

    //set dummy
    //=OV5642_PV_PERIOD_PIXEL_NUMS + 
    //dummy_pixels = 2592 / 2 + 450;
    dummy_pixels = OV5642_PV_PERIOD_PIXEL_NUMS + OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS;
    //=OV5642_PV_PERIOD_LINE_NUMS + 
    //dummy_lines = 972 + 28;
    dummy_lines = OV5642_PV_PERIOD_LINE_NUMS + OV5642_PV_PERIOD_EXTRA_LINE_NUMS;

    //HTS
    OV5642_write_cmos_sensor(0x380c, (dummy_pixels >> 8) & 0xff);
    OV5642_write_cmos_sensor(0x380d, (dummy_pixels  & 0xff));
    
    //AEC VTS
    //OV5642_write_cmos_sensor(0x350c, (dummy_lines >> 8) & 0xff);
    //OV5642_write_cmos_sensor(0x350d, (dummy_lines & 0xff));

    OV5642_PV_pclk = 5525; 
    OV5642_sensor_pclk=55250000;//19500000;
    SENSORDB("Set 720P End\n"); 
}

/*******************************************************************************
*
********************************************************************************/
void OV5642_set_5M15fps(void)
{
    OV5642_g_RES = OV5642_5M;

    //;Clock configration based on 24Mhz input.
    //;48MHz PCLK output
    //;90 0e 36
    //;90 0f  0c
    //;90 11 40
    //;window configuration

    OV5642_write_cmos_sensor(0x3503,0x7 );

    OV5642_write_cmos_sensor(0x3001,0x40);
    OV5642_write_cmos_sensor(0x3002,0x1c);
    OV5642_write_cmos_sensor(0x300d,0x21);
    //OV5642_write_cmos_sensor(0x3011,0x8 );//pll
    //OV5642_write_cmos_sensor(0x3010,0x30);//pll
    //OV5642_write_cmos_sensor(0x3012,0x00);//pll

    //OV5642_write_cmos_sensor(0x350c,0x07);
    //OV5642_write_cmos_sensor(0x350d,0xd0);

    OV5642_write_cmos_sensor(0x3622,0x60);
    OV5642_write_cmos_sensor(0x3621,0x09);//29
    OV5642_write_cmos_sensor(0x3709,0x0 );
    OV5642_write_cmos_sensor(0x3600,0x54);
    OV5642_write_cmos_sensor(0x3602,0xe4);
    OV5642_write_cmos_sensor(0x3606,0x1b);
    OV5642_write_cmos_sensor(0x3612,0xac);
    OV5642_write_cmos_sensor(0x3613,0x44);
    OV5642_write_cmos_sensor(0x3623,0x22);
    OV5642_write_cmos_sensor(0x3705,0xda);
    OV5642_write_cmos_sensor(0x370a,0x80);

    OV5642_write_cmos_sensor(0x3801,0xb4);
    OV5642_write_cmos_sensor(0x3804,0x0a);
    OV5642_write_cmos_sensor(0x3805,0x20);
    OV5642_write_cmos_sensor(0x3806,0x07);
    OV5642_write_cmos_sensor(0x3807,0x98);
    OV5642_write_cmos_sensor(0x3808,0x0a);
    OV5642_write_cmos_sensor(0x3809,0x20);
    OV5642_write_cmos_sensor(0x380a,0x07);
    OV5642_write_cmos_sensor(0x380b,0x98);
    OV5642_write_cmos_sensor(0x380c,0x0c);
    OV5642_write_cmos_sensor(0x380d,0x80);
    OV5642_write_cmos_sensor(0x380e,0x07);
    OV5642_write_cmos_sensor(0x380f,0xd0);
    
    OV5642_write_cmos_sensor(0x3810,0xc1);
    OV5642_write_cmos_sensor(0x3818,0xc0);//80
    OV5642_write_cmos_sensor(0x3824,0x11);
    OV5642_write_cmos_sensor(0x3825,0xac);
    OV5642_write_cmos_sensor(0x3827,0xc );

    OV5642_write_cmos_sensor(0x3a1a,0x4 );
    OV5642_write_cmos_sensor(0x3a08,0x9 );
    OV5642_write_cmos_sensor(0x3a09,0x60);
    OV5642_write_cmos_sensor(0x3a0a,0x7 );
    OV5642_write_cmos_sensor(0x3a0b,0xd0);
    OV5642_write_cmos_sensor(0x3a0d,0x10);
    OV5642_write_cmos_sensor(0x3a0e,0xd );

    OV5642_write_cmos_sensor(0x401c,0x6 );
    OV5642_write_cmos_sensor(0x5000,0x6 );
    OV5642_write_cmos_sensor(0x5001,0x1 );//awb
    OV5642_write_cmos_sensor(0x5005,0x0 );

    // PLL Setting, set PCLK to 91, and fps = 15fps 
    OV5642_write_cmos_sensor(0x300f, 0x06); 
    OV5642_write_cmos_sensor(0x3010, 0x00); 
    OV5642_write_cmos_sensor(0x3011, 0x15);     
    OV5642_write_cmos_sensor(0x3012, 0x04);     
    OV5642_write_cmos_sensor(0x3815, 0x02);         
    OV5642_write_cmos_sensor(0x3029, 0x00);         
    OV5642_write_cmos_sensor(0x3033, 0x03 );    //increase PLL, fps = 9.64

    OV5642_CAP_pclk = 8233; 
    OV5642_sensor_pclk = 82330000;//19500000;
    SENSORDB("Set 5M End\n"); 
}

/*******************************************************************************
*
********************************************************************************/
void OV5642_set_5M(void)
{
    OV5642_g_RES = OV5642_5M;

    OV5642_write_cmos_sensor(0x3503,0x7 );

    OV5642_write_cmos_sensor(0x3001,0x40);
    OV5642_write_cmos_sensor(0x3002,0x1c);
    OV5642_write_cmos_sensor(0x300d,0x21);
    //OV5642_write_cmos_sensor(0x3011,0x8 );//pll
    //OV5642_write_cmos_sensor(0x3010,0x30);//pll
    //OV5642_write_cmos_sensor(0x3012,0x00);//pll

    //OV5642_write_cmos_sensor(0x350c,0x07);
    //OV5642_write_cmos_sensor(0x350d,0xd0);

    OV5642_write_cmos_sensor(0x3622,0x60);
    OV5642_write_cmos_sensor(0x3621,0x09);//29
    OV5642_write_cmos_sensor(0x3709,0x0 );
    OV5642_write_cmos_sensor(0x3600,0x54);
    OV5642_write_cmos_sensor(0x3602,0xe4);
    OV5642_write_cmos_sensor(0x3606,0x1b);
    OV5642_write_cmos_sensor(0x3612,0xac);
    OV5642_write_cmos_sensor(0x3613,0x44);
    OV5642_write_cmos_sensor(0x3623,0x22);
    OV5642_write_cmos_sensor(0x3705,0xda);
    OV5642_write_cmos_sensor(0x370a,0x80);

    OV5642_write_cmos_sensor(0x3801,0xb4);
    OV5642_write_cmos_sensor(0x3804,0x0a);
    OV5642_write_cmos_sensor(0x3805,0x20);
    OV5642_write_cmos_sensor(0x3806,0x07);
    OV5642_write_cmos_sensor(0x3807,0x98);
    OV5642_write_cmos_sensor(0x3808,0x0a);
    OV5642_write_cmos_sensor(0x3809,0x20);
    OV5642_write_cmos_sensor(0x380a,0x07);
    OV5642_write_cmos_sensor(0x380b,0x98);
    OV5642_write_cmos_sensor(0x380c,0x0c);
    OV5642_write_cmos_sensor(0x380d,0x80);
    OV5642_write_cmos_sensor(0x380e,0x07);
    OV5642_write_cmos_sensor(0x380f,0xd0);
    
    OV5642_write_cmos_sensor(0x3810,0xc2);
    OV5642_write_cmos_sensor(0x3818,0xc0);//80
    OV5642_write_cmos_sensor(0x3824,0x11);
    OV5642_write_cmos_sensor(0x3825,0xac);
    OV5642_write_cmos_sensor(0x3827,0xc );

    OV5642_write_cmos_sensor(0x3a1a,0x4 );
    OV5642_write_cmos_sensor(0x3a08,0x9 );
    OV5642_write_cmos_sensor(0x3a09,0x60);
    OV5642_write_cmos_sensor(0x3a0a,0x7 );
    OV5642_write_cmos_sensor(0x3a0b,0xd0);
    OV5642_write_cmos_sensor(0x3a0d,0x10);
    OV5642_write_cmos_sensor(0x3a0e,0xd );

    OV5642_write_cmos_sensor(0x401c,0x6 );
    OV5642_write_cmos_sensor(0x5000,0x6 );
    OV5642_write_cmos_sensor(0x5001,0x1 );//awb
    OV5642_write_cmos_sensor(0x5005,0x0 );
    OV5642_write_cmos_sensor(0x3011,0x13 );    //increase PLL, fps = 9.64

    // PLL Setting, set PCLK to 61.75, and fps = 9.6fps 
    OV5642_CAP_pclk = 6175; 
    OV5642_sensor_pclk=61750000;//19500000;
    SENSORDB("Set 5M End\n"); 
}


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   OV5642Open
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

UINT32 OV5642Open(void)
{
    int  retry = 0; 

    OV5642_sensor_id = ((OV5642_read_cmos_sensor(0x300A) << 8) | OV5642_read_cmos_sensor(0x300B));
    // check if sensor ID correct
    retry = 3; 
    do {
        OV5642_sensor_id = ((OV5642_read_cmos_sensor(0x300A) << 8) | OV5642_read_cmos_sensor(0x300B));        
        if (OV5642_sensor_id == OV5642_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", OV5642_sensor_id); 
        retry--; 
    } while (retry > 0);

    if (OV5642_sensor_id != OV5642_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;

    OV5642_Sensor_Init();


    OV5642_sensor_gain_base = read_OV5642_gain();
    
    OV5642_g_iBackupExtraExp = 0;
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   OV5642GetSensorID
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
UINT32 OV5642GetSensorID(UINT32 *sensorID) 
{
    int  retry = 3; 

    // check if sensor ID correct
    do {
        *sensorID = ((OV5642_read_cmos_sensor(0x300A) << 8) | OV5642_read_cmos_sensor(0x300B));        
        if (*sensorID == OV5642_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != OV5642_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   OV5642_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of OV5642 to change exposure time.
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
void OV5642_SetShutter(kal_uint16 iShutter)
{
#if 0 
    if (iShutter < 4 )
        iShutter = 4;
#else 
    if (iShutter < 1)
        iShutter = 1; 
#endif     

    OV5642_pv_exposure_lines = iShutter;

    //KD_IMGSENSOR_PROFILE_INIT();
    OV5642_write_shutter(iShutter);
    //KD_IMGSENSOR_PROFILE("OV5642_SetShutter"); 
    //SENSORDB("iShutter = %d\n", iShutter);

}   /*  OV5642_SetShutter   */



/*************************************************************************
* FUNCTION
*   OV5642_read_shutter
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
UINT16 OV5642_read_shutter(void)
{
    return (UINT16)( (OV5642_read_cmos_sensor(0x3002)<<8) | OV5642_read_cmos_sensor(0x3003) );
}

/*************************************************************************
* FUNCTION
*   OV5642_night_mode
*
* DESCRIPTION
*   This function night mode of OV5642.
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
void OV5642_NightMode(kal_bool bEnable)
{
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
}/*	OV5642_NightMode */



/*************************************************************************
* FUNCTION
*   OV5642Close
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
UINT32 OV5642Close(void)
{
    //  CISModulePowerOn(FALSE);

    //s_porting
    //  DRV_I2CClose(OV5642hDrvI2C);
    //e_porting
    return ERROR_NONE;
}	/* OV5642Close() */

/*************************************************************************
* FUNCTION
*   OV5642_FOCUS_AD5820_Init
*
* DESCRIPTION
*   This function is to load micro code for AF function
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
static u8 AD5820_Config[] = {    
     0x02,
     0x00,
     0x06,
     0x02,
     0x0b,
     0x5f,
     0x78,
     0x7f,
     0xe4,
     0xf6,
     0xd8,
     0xfd,
     0x75,
     0x81,
     0x7e,
     0x02,
     0x13,
     0xb5,
     0x00,
     0x02,
     0x12,
     0xe0,
     0xe0,
     0xf5,
     0x72,
     0xa3,
     0xe0,
     0xf5,
     0x73,
     0xae,
     0x6a,
     0xe4,
     0x85,
     0x6b,
     0x55,
     0x8e,
     0x54,
     0xf5,
     0x53,
     0xf5,
     0x52,
     0xab,
     0x55,
     0xaa,
     0x54,
     0xa9,
     0x53,
     0xa8,
     0x52,
     0xaf,
     0x2c,
     0xfc,
     0xfd,
     0xfe,
     0x12,
     0x08,
     0x8e,
     0x8f,
     0x55,
     0x8e,
     0x54,
     0x8d,
     0x53,
     0x8c,
     0x52,
     0xaf,
     0x55,
     0xae,
     0x54,
     0xad,
     0x53,
     0xac,
     0x52,
     0x8f,
     0x2b,
     0x8e,
     0x2a,
     0x8d,
     0x29,
     0x8c,
     0x28,
     0xae,
     0x6c,
     0xe4,
     0x85,
     0x6d,
     0x55,
     0x8e,
     0x54,
     0xf5,
     0x53,
     0xf5,
     0x52,
     0xab,
     0x55,
     0xaa,
     0x54,
     0xa9,
     0x53,
     0xa8,
     0x52,
     0xaf,
     0x2d,
     0xfc,
     0xfd,
     0xfe,
     0x12,
     0x08,
     0x8e,
     0x8f,
     0x55,
     0x8e,
     0x54,
     0x8d,
     0x53,
     0x8c,
     0x52,
     0xe5,
     0x2b,
     0x25,
     0x55,
     0xf5,
     0x2b,
     0xe5,
     0x2a,
     0x35,
     0x54,
     0xf5,
     0x2a,
     0xe5,
     0x29,
     0x35,
     0x53,
     0xf5,
     0x29,
     0xe5,
     0x28,
     0x35,
     0x52,
     0xf5,
     0x28,
     0xae,
     0x6e,
     0xe4,
     0x85,
     0x6f,
     0x55,
     0x8e,
     0x54,
     0xf5,
     0x53,
     0xf5,
     0x52,
     0xab,
     0x55,
     0xaa,
     0x54,
     0xa9,
     0x53,
     0xa8,
     0x52,
     0xaf,
     0x2e,
     0xfc,
     0xfd,
     0xfe,
     0x12,
     0x08,
     0x8e,
     0x8f,
     0x55,
     0x8e,
     0x54,
     0x8d,
     0x53,
     0x8c,
     0x52,
     0xe5,
     0x2b,
     0x25,
     0x55,
     0xf5,
     0x2b,
     0xe5,
     0x2a,
     0x35,
     0x54,
     0xf5,
     0x2a,
     0xe5,
     0x29,
     0x35,
     0x53,
     0xf5,
     0x29,
     0xe5,
     0x28,
     0x35,
     0x52,
     0xf5,
     0x28,
     0xae,
     0x70,
     0xe4,
     0x85,
     0x71,
     0x55,
     0x8e,
     0x54,
     0xf5,
     0x53,
     0xf5,
     0x52,
     0xab,
     0x55,
     0xaa,
     0x54,
     0xa9,
     0x53,
     0xa8,
     0x52,
     0xaf,
     0x2f,
     0xfc,
     0xfd,
     0xfe,
     0x12,
     0x08,
     0x8e,
     0x8f,
     0x55,
     0x8e,
     0x54,
     0x8d,
     0x53,
     0x8c,
     0x52,
     0xe5,
     0x2b,
     0x25,
     0x55,
     0xf5,
     0x2b,
     0xe5,
     0x2a,
     0x35,
     0x54,
     0xf5,
     0x2a,
     0xe5,
     0x29,
     0x35,
     0x53,
     0xf5,
     0x29,
     0xe5,
     0x28,
     0x35,
     0x52,
     0xf5,
     0x28,
     0xae,
     0x72,
     0xe4,
     0x85,
     0x73,
     0x55,
     0x8e,
     0x54,
     0xf5,
     0x53,
     0xf5,
     0x52,
     0xab,
     0x55,
     0xaa,
     0x54,
     0xa9,
     0x53,
     0xa8,
     0x52,
     0xaf,
     0x30,
     0xfc,
     0xfd,
     0xfe,
     0x12,
     0x08,
     0x8e,
     0x8f,
     0x55,
     0x8e,
     0x54,
     0x8d,
     0x53,
     0x8c,
     0x52,
     0xe5,
     0x2b,
     0x25,
     0x55,
     0xf5,
     0x2b,
     0xe5,
     0x2a,
     0x35,
     0x54,
     0xf5,
     0x2a,
     0xe5,
     0x29,
     0x35,
     0x53,
     0xf5,
     0x29,
     0xe5,
     0x28,
     0x35,
     0x52,
     0xf5,
     0x28,
     0x22,
     0xab,
     0x0d,
     0xaa,
     0x0c,
     0xa9,
     0x0b,
     0xa8,
     0x0a,
     0xfc,
     0xfd,
     0xfe,
     0x12,
     0x08,
     0x8e,
     0x8f,
     0x0d,
     0x8e,
     0x0c,
     0x8d,
     0x0b,
     0x8c,
     0x0a,
     0x7b,
     0x40,
     0xe4,
     0xfa,
     0xf9,
     0xf8,
     0x12,
     0x09,
     0x19,
     0x8f,
     0x0d,
     0x8e,
     0x0c,
     0x8d,
     0x0b,
     0x8c,
     0x0a,
     0x22,
     0xd2,
     0x29,
     0x90,
     0x30,
     0x1b,
     0xe5,
     0x25,
     0xf0,
     0x22,
     0x85,
     0x49,
     0x82,
     0x85,
     0x48,
     0x83,
     0xe5,
     0x7c,
     0x75,
     0xf0,
     0x02,
     0x12,
     0x09,
     0xe1,
     0xe4,
     0x93,
     0xfe,
     0x74,
     0x01,
     0x93,
     0xff,
     0x85,
     0x49,
     0x82,
     0x85,
     0x48,
     0x83,
     0xe4,
     0x93,
     0xfc,
     0x74,
     0x01,
     0x93,
     0xfd,
     0xc3,
     0xef,
     0x9d,
     0xff,
     0xee,
     0x9c,
     0x22,
     0xfe,
     0xe4,
     0xfc,
     0xfd,
     0xe5,
     0x3f,
     0x2f,
     0xf5,
     0x3f,
     0xe5,
     0x3e,
     0x3e,
     0xf5,
     0x3e,
     0xed,
     0x35,
     0x3d,
     0xf5,
     0x3d,
     0xec,
     0x35,
     0x3c,
     0xf5,
     0x3c,
     0xaf,
     0x3f,
     0xae,
     0x3e,
     0xfc,
     0xad,
     0x3d,
     0x78,
     0x08,
     0x12,
     0x09,
     0xbe,
     0x8f,
     0x3f,
     0x8e,
     0x3e,
     0x8d,
     0x3d,
     0x8c,
     0x3c,
     0x22,
     0xe5,
     0x0b,
     0x24,
     0x01,
     0xff,
     0xe4,
     0x33,
     0xfe,
     0x22,
     0xaf,
     0x2b,
     0xae,
     0x2a,
     0xad,
     0x29,
     0xac,
     0x28,
     0x78,
     0x06,
     0x12,
     0x09,
     0xab,
     0x8f,
     0x2b,
     0x8e,
     0x2a,
     0x8d,
     0x29,
     0x8c,
     0x28,
     0xd3,
     0xe5,
     0x29,
     0x94,
     0x00,
     0xe5,
     0x28,
     0x94,
     0x00,
     0x22,
     0x12,
     0x08,
     0x8e,
     0x8f,
     0x69,
     0x8e,
     0x68,
     0x8d,
     0x67,
     0x8c,
     0x66,
     0xaf,
     0x69,
     0xae,
     0x68,
     0xad,
     0x67,
     0xac,
     0x66,
     0x22,
     0xe0,
     0x44,
     0x01,
     0xf0,
     0xe0,
     0x44,
     0x02,
     0xf0,
     0xe0,
     0x44,
     0x04,
     0xf0,
     0x22,
     0xd2,
     0x09,
     0x90,
     0x30,
     0x18,
     0xe5,
     0x21,
     0xf0,
     0x22,
     0xe4,
     0x85,
     0x11,
     0x0d,
     0x85,
     0x10,
     0x0c,
     0xf5,
     0x0b,
     0xf5,
     0x0a,
     0xab,
     0x0d,
     0xaa,
     0x0c,
     0xa9,
     0x0b,
     0xa8,
     0x0a,
     0x22,
     0x90,
     0x30,
     0x42,
     0xe0,
     0xf5,
     0x22,
     0x75,
     0x51,
     0x0a,
     0x22,
     0x85,
     0x49,
     0x82,
     0x85,
     0x48,
     0x83,
     0x22,
     0xf5,
     0x82,
     0xe4,
     0x3a,
     0xf5,
     0x83,
     0x02,
     0x09,
     0xd1,
     0x8f,
     0x0a,
     0x74,
     0x4a,
     0x2f,
     0xf8,
     0xe6,
     0x22,
     0xc2,
     0x07,
     0xc2,
     0x06,
     0xc2,
     0x02,
     0xc2,
     0x01,
     0xc2,
     0x00,
     0xc2,
     0x03,
     0xd2,
     0x04,
     0x22,
     0xe5,
     0x16,
     0x25,
     0xe0,
     0x25,
     0xe0,
     0x22,
     0x12,
     0x09,
     0x19,
     0x8f,
     0x69,
     0x8e,
     0x68,
     0x8d,
     0x67,
     0x8c,
     0x66,
     0x22,
     0xe4,
     0x85,
     0x0f,
     0x0d,
     0x85,
     0x0e,
     0x0c,
     0xf5,
     0x0b,
     0xf5,
     0x0a,
     0x22,
     0x90,
     0x10,
     0xa7,
     0xe4,
     0x93,
     0xff,
     0x90,
     0x30,
     0x0a,
     0xe0,
     0x22,
     0xc2,
     0x02,
     0xc2,
     0x01,
     0xd2,
     0x00,
     0xc2,
     0x03,
     0xc2,
     0x04,
     0x22,
     0x75,
     0xf0,
     0x02,
     0x02,
     0x09,
     0xe1,
     0xd2,
     0x02,
     0xd2,
     0x01,
     0xc2,
     0x00,
     0x22,
     0x74,
     0x4a,
     0x25,
     0x0a,
     0xf8,
     0xe6,
     0x22,
     0xd3,
     0xe5,
     0x69,
     0x94,
     0xff,
     0xe5,
     0x68,
     0x94,
     0x03,
     0x22,
     0x30,
     0x18,
     0x4d,
     0x20,
     0x19,
     0x4a,
     0x75,
     0x0a,
     0x02,
     0x12,
     0x02,
     0xa2,
     0xff,
     0xe5,
     0x4a,
     0xd3,
     0x9f,
     0x40,
     0x04,
     0x7f,
     0x00,
     0x80,
     0x02,
     0xaf,
     0x0a,
     0x12,
     0x02,
     0x49,
     0xff,
     0xe5,
     0x4b,
     0xd3,
     0x9f,
     0x40,
     0x04,
     0x7f,
     0x01,
     0x80,
     0x02,
     0xaf,
     0x0a,
     0x12,
     0x02,
     0x49,
     0xff,
     0xe5,
     0x4d,
     0xd3,
     0x9f,
     0x40,
     0x04,
     0x7f,
     0x03,
     0x80,
     0x02,
     0xaf,
     0x0a,
     0x12,
     0x02,
     0x49,
     0xff,
     0xe5,
     0x4e,
     0xd3,
     0x9f,
     0x40,
     0x04,
     0x7f,
     0x04,
     0x80,
     0x02,
     0xaf,
     0x0a,
     0x12,
     0x02,
     0x49,
     0xf5,
     0x0b,
     0x80,
     0x06,
     0x85,
     0x79,
     0x0a,
     0x85,
     0x4f,
     0x0b,
     0x7f,
     0x01,
     0xe4,
     0xfe,
     0x12,
     0x02,
     0xa2,
     0xfd,
     0xe5,
     0x0b,
     0xc3,
     0x9d,
     0x50,
     0x04,
     0x7d,
     0x01,
     0x80,
     0x02,
     0x7d,
     0xff,
     0xac,
     0x0b,
     0xe5,
     0x4e,
     0xb5,
     0x0b,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1f,
     0xe5,
     0x4d,
     0xb5,
     0x0b,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1e,
     0xe5,
     0x4c,
     0xb5,
     0x0b,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1d,
     0xe5,
     0x4b,
     0xb5,
     0x0b,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1c,
     0xe5,
     0x4a,
     0xb5,
     0x0b,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1b,
     0xe5,
     0x30,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1f,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1f,
     0xe5,
     0x2f,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1e,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1e,
     0xe5,
     0x2e,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1d,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1d,
     0xe5,
     0x2d,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1c,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1c,
     0xe5,
     0x2c,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1b,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1b,
     0xe5,
     0x23,
     0x54,
     0xf8,
     0x70,
     0x5b,
     0xbf,
     0x01,
     0x08,
     0xed,
     0xf4,
     0x04,
     0xfd,
     0x7f,
     0x02,
     0x80,
     0x06,
     0xbf,
     0x02,
     0x02,
     0x7f,
     0x01,
     0x0e,
     0xd3,
     0xed,
     0x64,
     0x80,
     0x94,
     0x80,
     0x40,
     0x18,
     0xec,
     0x2e,
     0xf5,
     0x0b,
     0xd3,
     0x95,
     0x7c,
     0x40,
     0x2c,
     0x7d,
     0xff,
     0xef,
     0x60,
     0x04,
     0x7b,
     0x00,
     0x80,
     0x02,
     0x7b,
     0x03,
     0xaf,
     0x03,
     0x80,
     0x18,
     0xec,
     0xc3,
     0x9e,
     0x50,
     0x13,
     0x7d,
     0x01,
     0xef,
     0x60,
     0x04,
     0x7b,
     0x00,
     0x80,
     0x02,
     0x7b,
     0x03,
     0xaf,
     0x03,
     0xec,
     0x2e,
     0xf5,
     0x0b,
     0x80,
     0x05,
     0xc3,
     0xec,
     0x9e,
     0xf5,
     0x0b,
     0xef,
     0x64,
     0x03,
     0x60,
     0x03,
     0x02,
     0x03,
     0x1f,
     0x12,
     0x02,
     0xa2,
     0xf5,
     0x0b,
     0x12,
     0x01,
     0xc9,
     0xe5,
     0x4e,
     0xb5,
     0x07,
     0x07,
     0xe4,
     0xb5,
     0x06,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1f,
     0x92,
     0x1f,
     0xe5,
     0x4d,
     0xb5,
     0x07,
     0x07,
     0xe4,
     0xb5,
     0x06,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1e,
     0x92,
     0x1e,
     0x12,
     0x01,
     0xc9,
     0xe5,
     0x4c,
     0xb5,
     0x07,
     0x07,
     0xe4,
     0xb5,
     0x06,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1d,
     0x92,
     0x1d,
     0xe5,
     0x4b,
     0xb5,
     0x07,
     0x07,
     0xe4,
     0xb5,
     0x06,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1c,
     0x92,
     0x1c,
     0x12,
     0x01,
     0xc9,
     0xe5,
     0x4a,
     0xb5,
     0x07,
     0x07,
     0xe4,
     0xb5,
     0x06,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1b,
     0x92,
     0x1b,
     0xe5,
     0x4e,
     0x12,
     0x01,
     0xcb,
     0xad,
     0x0b,
     0x7c,
     0x00,
     0xef,
     0xb5,
     0x05,
     0x07,
     0xec,
     0xb5,
     0x06,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1f,
     0x92,
     0x1f,
     0xe5,
     0x4d,
     0x12,
     0x01,
     0xcb,
     0xef,
     0xb5,
     0x05,
     0x07,
     0xee,
     0xb5,
     0x04,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1e,
     0x92,
     0x1e,
     0xe5,
     0x4c,
     0x12,
     0x01,
     0xcb,
     0xad,
     0x0b,
     0x7c,
     0x00,
     0xef,
     0xb5,
     0x05,
     0x07,
     0xec,
     0xb5,
     0x06,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1d,
     0x92,
     0x1d,
     0xe5,
     0x4b,
     0x12,
     0x01,
     0xcb,
     0xef,
     0xb5,
     0x05,
     0x07,
     0xee,
     0xb5,
     0x04,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1c,
     0x92,
     0x1c,
     0xe5,
     0x4a,
     0x12,
     0x01,
     0xcb,
     0x7c,
     0x00,
     0xef,
     0xb5,
     0x0b,
     0x07,
     0xec,
     0xb5,
     0x06,
     0x03,
     0xd3,
     0x80,
     0x02,
     0xa2,
     0x1b,
     0x92,
     0x1b,
     0xe5,
     0x30,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1f,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1f,
     0xe5,
     0x2f,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1e,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1e,
     0xe5,
     0x2e,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1d,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1d,
     0xe5,
     0x2d,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1c,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1c,
     0xe5,
     0x2c,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x04,
     0xa2,
     0x1b,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x1b,
     0x85,
     0x0a,
     0x79,
     0xe5,
     0x7d,
     0xb5,
     0x0b,
     0x03,
     0x02,
     0x15,
     0x17,
     0x85,
     0x0b,
     0x0c,
     0x12,
     0x11,
     0xd8,
     0xd2,
     0x02,
     0xc2,
     0x01,
     0xd2,
     0x00,
     0x75,
     0x31,
     0x03,
     0x22,
     0xe5,
     0x7e,
     0x24,
     0xfe,
     0x60,
     0x27,
     0x14,
     0x60,
     0x31,
     0x24,
     0xf8,
     0x60,
     0x3a,
     0x14,
     0x60,
     0x4b,
     0x14,
     0x60,
     0x59,
     0x14,
     0x60,
     0x6a,
     0x24,
     0xfd,
     0x70,
     0x03,
     0x02,
     0x05,
     0xda,
     0x24,
     0x10,
     0x60,
     0x03,
     0x02,
     0x06,
     0xd5,
     0xe4,
     0xf5,
     0x0a,
     0x12,
     0x11,
     0x10,
     0x02,
     0x06,
     0xbe,
     0x75,
     0x0a,
     0x01,
     0x12,
     0x06,
     0xd6,
     0xc2,
     0x3a,
     0xd2,
     0x39,
     0x02,
     0x06,
     0xcf,
     0x75,
     0x0a,
     0x02,
     0x12,
     0x06,
     0xd6,
     0xd2,
     0x3a,
     0xc2,
     0x39,
     0x02,
     0x06,
     0xcf,
     0x30,
     0x36,
     0x0c,
     0x20,
     0x3a,
     0x03,
     0x30,
     0x39,
     0x06,
     0xe4,
     0xf5,
     0x0a,
     0x12,
     0x12,
     0x91,
     0xd2,
     0x18,
     0xc2,
     0x19,
     0x22,
     0x30,
     0x36,
     0x09,
     0x20,
     0x3a,
     0x03,
     0x30,
     0x39,
     0x03,
     0x12,
     0x06,
     0xe7,
     0xd2,
     0x18,
     0xd2,
     0x19,
     0x22,
     0x30,
     0x36,
     0x0c,
     0x20,
     0x3a,
     0x03,
     0x30,
     0x39,
     0x06,
     0x75,
     0x0a,
     0x02,
     0x12,
     0x12,
     0x91,
     0xc2,
     0x18,
     0xd2,
     0x19,
     0x22,
     0x20,
     0x3a,
     0x06,
     0x20,
     0x39,
     0x03,
     0x02,
     0x06,
     0xd5,
     0xe5,
     0x79,
     0xd3,
     0x94,
     0x03,
     0x40,
     0x04,
     0x7f,
     0x00,
     0x80,
     0x04,
     0xe5,
     0x79,
     0x04,
     0xff,
     0x8f,
     0x79,
     0x30,
     0x18,
     0x06,
     0x30,
     0x19,
     0x03,
     0x12,
     0x06,
     0xe7,
     0x30,
     0x18,
     0x03,
     0x02,
     0x06,
     0xd5,
     0x20,
     0x19,
     0x03,
     0x02,
     0x06,
     0xd5,
     0x75,
     0x0a,
     0x02,
     0x02,
     0x12,
     0x91,
     0xe5,
     0x68,
     0xd3,
     0x94,
     0x38,
     0x40,
     0x04,
     0x7f,
     0x34,
     0x80,
     0x02,
     0xaf,
     0x68,
     0x8f,
     0x68,
     0xe5,
     0x68,
     0xc3,
     0x94,
     0x08,
     0x50,
     0x04,
     0x7f,
     0x08,
     0x80,
     0x02,
     0xaf,
     0x68,
     0x8f,
     0x68,
     0xe5,
     0x69,
     0xd3,
     0x94,
     0x2a,
     0x40,
     0x04,
     0x7f,
     0x2a,
     0x80,
     0x02,
     0xaf,
     0x69,
     0x8f,
     0x69,
     0xe5,
     0x69,
     0xc3,
     0x94,
     0x06,
     0x50,
     0x04,
     0x7f,
     0x06,
     0x80,
     0x02,
     0xaf,
     0x69,
     0x8f,
     0x69,
     0xaf,
     0x68,
     0xef,
     0x24,
     0xf8,
     0xff,
     0xe4,
     0x34,
     0xff,
     0xfe,
     0xe4,
     0x8f,
     0x3f,
     0x8e,
     0x3e,
     0xf5,
     0x3d,
     0xf5,
     0x3c,
     0xac,
     0x3c,
     0x12,
     0x01,
     0xb9,
     0xaf,
     0x69,
     0xef,
     0x24,
     0xfa,
     0xff,
     0xe4,
     0x34,
     0xff,
     0x12,
     0x01,
     0x9c,
     0xaf,
     0x68,
     0xef,
     0x24,
     0x08,
     0xff,
     0xe4,
     0x33,
     0x12,
     0x01,
     0x9c,
     0xaf,
     0x69,
     0xef,
     0x24,
     0x06,
     0xff,
     0xe4,
     0x33,
     0xfe,
     0xe4,
     0xfc,
     0xfd,
     0xe5,
     0x3f,
     0x2f,
     0xf5,
     0x3f,
     0xe5,
     0x3e,
     0x3e,
     0xf5,
     0x3e,
     0xed,
     0x35,
     0x3d,
     0xf5,
     0x3d,
     0xec,
     0x35,
     0x3c,
     0xf5,
     0x3c,
     0xe4,
     0x25,
     0x3f,
     0xf5,
     0x37,
     0xe4,
     0x35,
     0x3e,
     0xf5,
     0x36,
     0xe4,
     0x35,
     0x3d,
     0xf5,
     0x35,
     0xe5,
     0x3c,
     0x34,
     0x08,
     0xf5,
     0x34,
     0xe4,
     0x25,
     0x3f,
     0xf5,
     0x3b,
     0xe4,
     0x35,
     0x3e,
     0xf5,
     0x3a,
     0xe5,
     0x3d,
     0x34,
     0x06,
     0xf5,
     0x39,
     0xe4,
     0x35,
     0x3c,
     0xf5,
     0x38,
     0xe5,
     0x3f,
     0x24,
     0xfa,
     0xf5,
     0x43,
     0xe5,
     0x3e,
     0x34,
     0xff,
     0xf5,
     0x42,
     0xe5,
     0x3d,
     0x34,
     0xff,
     0xf5,
     0x41,
     0xe5,
     0x3c,
     0x34,
     0xff,
     0xf5,
     0x40,
     0xe4,
     0x25,
     0x3f,
     0xf5,
     0x47,
     0xe5,
     0x3e,
     0x34,
     0xf8,
     0xf5,
     0x46,
     0xe5,
     0x3d,
     0x34,
     0xff,
     0xf5,
     0x45,
     0xe5,
     0x3c,
     0x34,
     0xff,
     0xf5,
     0x44,
     0x75,
     0x79,
     0x02,
     0x75,
     0x0a,
     0x01,
     0x12,
     0x12,
     0x91,
     0xd2,
     0x18,
     0xd2,
     0x19,
     0xc2,
     0x3a,
     0xc2,
     0x39,
     0xd2,
     0x1a,
     0xd2,
     0x36,
     0xd2,
     0x30,
     0x22,
     0x12,
     0x11,
     0x10,
     0x75,
     0x79,
     0x02,
     0xe4,
     0xf5,
     0x0a,
     0x12,
     0x12,
     0x91,
     0xd2,
     0x18,
     0xc2,
     0x19,
     0x22,
     0x75,
     0x0a,
     0x01,
     0x12,
     0x12,
     0x91,
     0x22,
     0x90,
     0x38,
     0x04,
     0xe0,
     0xfe,
     0xa3,
     0xe0,
     0xfd,
     0xed,
     0xff,
     0xee,
     0x54,
     0x0f,
     0xf5,
     0x0e,
     0x8f,
     0x0f,
     0xa3,
     0xe0,
     0xfe,
     0xa3,
     0xe0,
     0xfd,
     0xed,
     0xff,
     0xee,
     0x54,
     0x07,
     0xf5,
     0x10,
     0x8f,
     0x11,
     0xe5,
     0x0e,
     0xc4,
     0xf8,
     0x54,
     0xf0,
     0xc8,
     0x68,
     0xf5,
     0x0e,
     0xe5,
     0x0f,
     0xc4,
     0x54,
     0x0f,
     0x48,
     0xf5,
     0x0f,
     0xe5,
     0x10,
     0xc4,
     0xf8,
     0x54,
     0xf0,
     0xc8,
     0x68,
     0xf5,
     0x10,
     0xe5,
     0x11,
     0xc4,
     0x54,
     0x0f,
     0x48,
     0xf5,
     0x11,
     0xe4,
     0xf5,
     0x17,
     0x75,
     0x16,
     0x04,
     0x12,
     0x02,
     0x60,
     0x24,
     0x34,
     0xf8,
     0xe6,
     0xf5,
     0x12,
     0x12,
     0x02,
     0x60,
     0x24,
     0x35,
     0xf8,
     0xe6,
     0xf5,
     0x14,
     0x12,
     0x02,
     0x60,
     0x24,
     0x36,
     0xf8,
     0xe6,
     0xf5,
     0x13,
     0x12,
     0x02,
     0x60,
     0x24,
     0x37,
     0xf8,
     0xe6,
     0xf5,
     0x15,
     0x12,
     0x02,
     0x73,
     0xaf,
     0x12,
     0x12,
     0x01,
     0x42,
     0x8f,
     0x12,
     0x12,
     0x02,
     0x73,
     0xaf,
     0x13,
     0x12,
     0x01,
     0x42,
     0x8f,
     0x13,
     0x12,
     0x02,
     0x1b,
     0xaf,
     0x14,
     0xfc,
     0xfd,
     0xfe,
     0x12,
     0x08,
     0x8e,
     0x12,
     0x01,
     0x61,
     0x7b,
     0x30,
     0x12,
     0x01,
     0x5a,
     0x8f,
     0x14,
     0x12,
     0x02,
     0x1b,
     0xaf,
     0x15,
     0xfc,
     0xfd,
     0xfe,
     0x12,
     0x08,
     0x8e,
     0x12,
     0x01,
     0x61,
     0xe4,
     0x7b,
     0x30,
     0x12,
     0x01,
     0x5b,
     0x8f,
     0x15,
     0xc3,
     0xe5,
     0x13,
     0x95,
     0x12,
     0xff,
     0x0f,
     0xef,
     0xc3,
     0x13,
     0xff,
     0xc3,
     0x94,
     0x04,
     0x50,
     0x27,
     0xe5,
     0x12,
     0x9f,
     0x40,
     0x06,
     0xe5,
     0x12,
     0x9f,
     0xfe,
     0x80,
     0x02,
     0x7e,
     0x00,
     0x8e,
     0x12,
     0xef,
     0xfd,
     0xe5,
     0x13,
     0x2d,
     0xfd,
     0xe4,
     0x33,
     0xfc,
     0xc3,
     0xed,
     0x95,
     0x0f,
     0xec,
     0x95,
     0x0e,
     0x50,
     0x02,
     0x80,
     0x02,
     0xad,
     0x0f,
     0x8d,
     0x13,
     0xc3,
     0xe5,
     0x15,
     0x95,
     0x14,
     0xff,
     0xc3,
     0x94,
     0x04,
     0x50,
     0x29,
     0xe5,
     0x14,
     0x9f,
     0x40,
     0x06,
     0xe5,
     0x14,
     0x9f,
     0xfe,
     0x80,
     0x02,
     0x7e,
     0x00,
     0x8e,
     0x14,
     0xef,
     0xfd,
     0xe5,
     0x15,
     0x2d,
     0xfd,
     0xe4,
     0x33,
     0xfc,
     0xc3,
     0xed,
     0x95,
     0x11,
     0xec,
     0x95,
     0x10,
     0x50,
     0x04,
     0xaf,
     0x05,
     0x80,
     0x02,
     0xaf,
     0x11,
     0x8f,
     0x15,
     0xe5,
     0x15,
     0xd3,
     0x95,
     0x17,
     0x40,
     0x04,
     0xaf,
     0x15,
     0x80,
     0x02,
     0xaf,
     0x17,
     0x8f,
     0x17,
     0xd3,
     0xe5,
     0x16,
     0x64,
     0x80,
     0x94,
     0x80,
     0x40,
     0x04,
     0xaf,
     0x15,
     0x80,
     0x02,
     0xaf,
     0x17,
     0x8f,
     0x15,
     0xe5,
     0x16,
     0xfd,
     0x33,
     0x95,
     0xe0,
     0xfc,
     0xed,
     0xae,
     0x04,
     0x78,
     0x02,
     0xc3,
     0x33,
     0xce,
     0x33,
     0xce,
     0xd8,
     0xf9,
     0xff,
     0x24,
     0x01,
     0xfb,
     0xee,
     0x34,
     0x60,
     0x8b,
     0x82,
     0xf5,
     0x83,
     0xe5,
     0x12,
     0xf0,
     0xef,
     0x24,
     0x02,
     0xff,
     0xee,
     0x34,
     0x60,
     0x8f,
     0x82,
     0xf5,
     0x83,
     0xe5,
     0x14,
     0xf0,
     0xed,
     0xae,
     0x04,
     0x78,
     0x02,
     0xc3,
     0x33,
     0xce,
     0x33,
     0xce,
     0xd8,
     0xf9,
     0xff,
     0x24,
     0x03,
     0xfd,
     0xee,
     0x34,
     0x60,
     0x8d,
     0x82,
     0xf5,
     0x83,
     0xe5,
     0x13,
     0xf0,
     0xef,
     0x24,
     0x04,
     0xff,
     0xee,
     0x34,
     0x60,
     0x8f,
     0x82,
     0xf5,
     0x83,
     0xe5,
     0x15,
     0xf0,
     0x15,
     0x16,
     0xc3,
     0xe5,
     0x16,
     0x64,
     0x80,
     0x94,
     0x80,
     0x40,
     0x03,
     0x02,
     0x07,
     0x38,
     0xc2,
     0x30,
     0x22,
     0xe8,
     0x8f,
     0xf0,
     0xa4,
     0xcc,
     0x8b,
     0xf0,
     0xa4,
     0x2c,
     0xfc,
     0xe9,
     0x8e,
     0xf0,
     0xa4,
     0x2c,
     0xfc,
     0x8a,
     0xf0,
     0xed,
     0xa4,
     0x2c,
     0xfc,
     0xea,
     0x8e,
     0xf0,
     0xa4,
     0xcd,
     0xa8,
     0xf0,
     0x8b,
     0xf0,
     0xa4,
     0x2d,
     0xcc,
     0x38,
     0x25,
     0xf0,
     0xfd,
     0xe9,
     0x8f,
     0xf0,
     0xa4,
     0x2c,
     0xcd,
     0x35,
     0xf0,
     0xfc,
     0xeb,
     0x8e,
     0xf0,
     0xa4,
     0xfe,
     0xa9,
     0xf0,
     0xeb,
     0x8f,
     0xf0,
     0xa4,
     0xcf,
     0xc5,
     0xf0,
     0x2e,
     0xcd,
     0x39,
     0xfe,
     0xe4,
     0x3c,
     0xfc,
     0xea,
     0xa4,
     0x2d,
     0xce,
     0x35,
     0xf0,
     0xfd,
     0xe4,
     0x3c,
     0xfc,
     0x22,
     0x75,
     0xf0,
     0x08,
     0x75,
     0x82,
     0x00,
     0xef,
     0x2f,
     0xff,
     0xee,
     0x33,
     0xfe,
     0xcd,
     0x33,
     0xcd,
     0xcc,
     0x33,
     0xcc,
     0xc5,
     0x82,
     0x33,
     0xc5,
     0x82,
     0x9b,
     0xed,
     0x9a,
     0xec,
     0x99,
     0xe5,
     0x82,
     0x98,
     0x40,
     0x0c,
     0xf5,
     0x82,
     0xee,
     0x9b,
     0xfe,
     0xed,
     0x9a,
     0xfd,
     0xec,
     0x99,
     0xfc,
     0x0f,
     0xd5,
     0xf0,
     0xd6,
     0xe4,
     0xce,
     0xfb,
     0xe4,
     0xcd,
     0xfa,
     0xe4,
     0xcc,
     0xf9,
     0xa8,
     0x82,
     0x22,
     0xb8,
     0x00,
     0xc1,
     0xb9,
     0x00,
     0x59,
     0xba,
     0x00,
     0x2d,
     0xec,
     0x8b,
     0xf0,
     0x84,
     0xcf,
     0xce,
     0xcd,
     0xfc,
     0xe5,
     0xf0,
     0xcb,
     0xf9,
     0x78,
     0x18,
     0xef,
     0x2f,
     0xff,
     0xee,
     0x33,
     0xfe,
     0xed,
     0x33,
     0xfd,
     0xec,
     0x33,
     0xfc,
     0xeb,
     0x33,
     0xfb,
     0x10,
     0xd7,
     0x03,
     0x99,
     0x40,
     0x04,
     0xeb,
     0x99,
     0xfb,
     0x0f,
     0xd8,
     0xe5,
     0xe4,
     0xf9,
     0xfa,
     0x22,
     0x78,
     0x18,
     0xef,
     0x2f,
     0xff,
     0xee,
     0x33,
     0xfe,
     0xed,
     0x33,
     0xfd,
     0xec,
     0x33,
     0xfc,
     0xc9,
     0x33,
     0xc9,
     0x10,
     0xd7,
     0x05,
     0x9b,
     0xe9,
     0x9a,
     0x40,
     0x07,
     0xec,
     0x9b,
     0xfc,
     0xe9,
     0x9a,
     0xf9,
     0x0f,
     0xd8,
     0xe0,
     0xe4,
     0xc9,
     0xfa,
     0xe4,
     0xcc,
     0xfb,
     0x22,
     0x75,
     0xf0,
     0x10,
     0xef,
     0x2f,
     0xff,
     0xee,
     0x33,
     0xfe,
     0xed,
     0x33,
     0xfd,
     0xcc,
     0x33,
     0xcc,
     0xc8,
     0x33,
     0xc8,
     0x10,
     0xd7,
     0x07,
     0x9b,
     0xec,
     0x9a,
     0xe8,
     0x99,
     0x40,
     0x0a,
     0xed,
     0x9b,
     0xfd,
     0xec,
     0x9a,
     0xfc,
     0xe8,
     0x99,
     0xf8,
     0x0f,
     0xd5,
     0xf0,
     0xda,
     0xe4,
     0xcd,
     0xfb,
     0xe4,
     0xcc,
     0xfa,
     0xe4,
     0xc8,
     0xf9,
     0x22,
     0xe8,
     0x60,
     0x0f,
     0xec,
     0xc3,
     0x13,
     0xfc,
     0xed,
     0x13,
     0xfd,
     0xee,
     0x13,
     0xfe,
     0xef,
     0x13,
     0xff,
     0xd8,
     0xf1,
     0x22,
     0xe8,
     0x60,
     0x0f,
     0xef,
     0xc3,
     0x33,
     0xff,
     0xee,
     0x33,
     0xfe,
     0xed,
     0x33,
     0xfd,
     0xec,
     0x33,
     0xfc,
     0xd8,
     0xf1,
     0x22,
     0xe4,
     0x93,
     0xfc,
     0x74,
     0x01,
     0x93,
     0xfd,
     0x74,
     0x02,
     0x93,
     0xfe,
     0x74,
     0x03,
     0x93,
     0xff,
     0x22,
     0xa4,
     0x25,
     0x82,
     0xf5,
     0x82,
     0xe5,
     0xf0,
     0x35,
     0x83,
     0xf5,
     0x83,
     0x22,
     0xd0,
     0x83,
     0xd0,
     0x82,
     0xf8,
     0xe4,
     0x93,
     0x70,
     0x12,
     0x74,
     0x01,
     0x93,
     0x70,
     0x0d,
     0xa3,
     0xa3,
     0x93,
     0xf8,
     0x74,
     0x01,
     0x93,
     0xf5,
     0x82,
     0x88,
     0x83,
     0xe4,
     0x73,
     0x74,
     0x02,
     0x93,
     0x68,
     0x60,
     0xef,
     0xa3,
     0xa3,
     0xa3,
     0x80,
     0xdf,
     0x75,
     0x0f,
     0x0a,
     0xa2,
     0xaf,
     0x92,
     0x32,
     0xc2,
     0xaf,
     0xc2,
     0x33,
     0x12,
     0x01,
     0x6a,
     0x12,
     0x02,
     0x12,
     0x12,
     0x01,
     0x6a,
     0x75,
     0x51,
     0x05,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xd2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x0a,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xc2,
     0x29,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x05,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xc2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x05,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0x75,
     0x10,
     0x18,
     0x12,
     0x0b,
     0x53,
     0x75,
     0x51,
     0x0a,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xd2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x12,
     0x02,
     0x2f,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xc2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x0a,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0x30,
     0x11,
     0x03,
     0x02,
     0x0b,
     0x0a,
     0x12,
     0x01,
     0x6a,
     0x12,
     0x02,
     0x12,
     0xe5,
     0x0d,
     0xf5,
     0x10,
     0x12,
     0x0b,
     0x53,
     0x75,
     0x51,
     0x0a,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xd2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x12,
     0x02,
     0x2f,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xc2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x0a,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0x30,
     0x11,
     0x04,
     0x15,
     0x0f,
     0x80,
     0x45,
     0x12,
     0x01,
     0x6a,
     0x12,
     0x02,
     0x12,
     0x85,
     0x0e,
     0x10,
     0x12,
     0x13,
     0x71,
     0xc2,
     0x09,
     0x12,
     0x02,
     0x14,
     0x75,
     0x51,
     0x0a,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xd2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x12,
     0x02,
     0x2f,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xc2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x0a,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0x30,
     0x11,
     0x06,
     0x15,
     0x0f,
     0xd2,
     0x33,
     0x80,
     0x03,
     0xe4,
     0xf5,
     0x0f,
     0x12,
     0x01,
     0x6a,
     0x12,
     0x02,
     0x12,
     0xc2,
     0x29,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x05,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xd2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x05,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0x12,
     0x01,
     0x6a,
     0x75,
     0x51,
     0x05,
     0xaf,
     0x51,
     0x15,
     0x51,
     0xef,
     0x70,
     0xf9,
     0xa2,
     0x32,
     0x92,
     0xaf,
     0xe5,
     0x0f,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x03,
     0x02,
     0x0a,
     0x1a,
     0x22,
     0x12,
     0x13,
     0x71,
     0xc2,
     0x09,
     0x90,
     0x30,
     0x18,
     0xe5,
     0x21,
     0xf0,
     0x22,
     0xc0,
     0xe0,
     0xc0,
     0xf0,
     0xc0,
     0x83,
     0xc0,
     0x82,
     0xc0,
     0xd0,
     0x75,
     0xd0,
     0x00,
     0xc0,
     0x00,
     0xc0,
     0x01,
     0xc0,
     0x02,
     0xc0,
     0x03,
     0xc0,
     0x04,
     0xc0,
     0x05,
     0xc0,
     0x06,
     0xc0,
     0x07,
     0x90,
     0x3f,
     0x0c,
     0xe0,
     0xf5,
     0x08,
     0xe5,
     0x08,
     0x20,
     0xe3,
     0x03,
     0x02,
     0x0c,
     0x10,
     0x30,
     0x35,
     0x03,
     0x02,
     0x0c,
     0x10,
     0x90,
     0x60,
     0x16,
     0xe0,
     0xf5,
     0x6a,
     0xa3,
     0xe0,
     0xf5,
     0x6b,
     0x90,
     0x60,
     0x1e,
     0xe0,
     0xf5,
     0x6c,
     0xa3,
     0xe0,
     0xf5,
     0x6d,
     0x90,
     0x60,
     0x26,
     0xe0,
     0xf5,
     0x6e,
     0xa3,
     0xe0,
     0xf5,
     0x6f,
     0x90,
     0x60,
     0x2e,
     0xe0,
     0xf5,
     0x70,
     0xa3,
     0xe0,
     0xf5,
     0x71,
     0x90,
     0x60,
     0x36,
     0x12,
     0x00,
     0x16,
     0x12,
     0x01,
     0xd2,
     0x40,
     0x06,
     0x75,
     0x2a,
     0xff,
     0x75,
     0x2b,
     0xff,
     0x85,
     0x2a,
     0x74,
     0x85,
     0x2b,
     0x75,
     0x90,
     0x60,
     0x1a,
     0xe0,
     0xf5,
     0x6a,
     0xa3,
     0xe0,
     0xf5,
     0x6b,
     0x90,
     0x60,
     0x22,
     0xe0,
     0xf5,
     0x6c,
     0xa3,
     0xe0,
     0xf5,
     0x6d,
     0x90,
     0x60,
     0x2a,
     0xe0,
     0xf5,
     0x6e,
     0xa3,
     0xe0,
     0xf5,
     0x6f,
     0x90,
     0x60,
     0x32,
     0xe0,
     0xf5,
     0x70,
     0xa3,
     0xe0,
     0xf5,
     0x71,
     0x90,
     0x60,
     0x3a,
     0x12,
     0x00,
     0x16,
     0x12,
     0x01,
     0xd2,
     0x40,
     0x06,
     0x75,
     0x2a,
     0xff,
     0x75,
     0x2b,
     0xff,
     0x85,
     0x2a,
     0x76,
     0x85,
     0x2b,
     0x77,
     0xd2,
     0x3b,
     0xe5,
     0x08,
     0x30,
     0xe5,
     0x41,
     0x90,
     0x56,
     0x90,
     0xe0,
     0xf5,
     0x55,
     0xe5,
     0x7b,
     0x12,
     0x01,
     0xcb,
     0xad,
     0x55,
     0xc3,
     0xef,
     0x9d,
     0x74,
     0x80,
     0xf8,
     0x6e,
     0x98,
     0x50,
     0x02,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x27,
     0xaf,
     0x55,
     0xef,
     0x24,
     0x01,
     0xff,
     0xe4,
     0x33,
     0xfe,
     0xc3,
     0xef,
     0x95,
     0x7b,
     0x74,
     0x80,
     0xf8,
     0x6e,
     0x98,
     0x50,
     0x02,
     0x80,
     0x02,
     0xa2,
     0x27,
     0x92,
     0x27,
     0x30,
     0x27,
     0x04,
     0xaf,
     0x55,
     0x80,
     0x02,
     0xaf,
     0x7b,
     0x8f,
     0x7b,
     0xe5,
     0x08,
     0x30,
     0xe1,
     0x08,
     0x90,
     0x30,
     0x24,
     0xe0,
     0xf5,
     0x33,
     0xe4,
     0xf0,
     0x90,
     0x3f,
     0x0c,
     0xe5,
     0x08,
     0xf0,
     0xd0,
     0x07,
     0xd0,
     0x06,
     0xd0,
     0x05,
     0xd0,
     0x04,
     0xd0,
     0x03,
     0xd0,
     0x02,
     0xd0,
     0x01,
     0xd0,
     0x00,
     0xd0,
     0xd0,
     0xd0,
     0x82,
     0xd0,
     0x83,
     0xd0,
     0xf0,
     0xd0,
     0xe0,
     0x32,
     0xe5,
     0x33,
     0x70,
     0x03,
     0x02,
     0x0d,
     0x91,
     0xc2,
     0xaf,
     0xaf,
     0x33,
     0xe4,
     0xf5,
     0x33,
     0xd2,
     0xaf,
     0x90,
     0x30,
     0x25,
     0xe0,
     0xf5,
     0x7e,
     0x90,
     0x50,
     0x82,
     0xe0,
     0xf5,
     0x66,
     0xa3,
     0xe0,
     0xf5,
     0x67,
     0xa3,
     0xe0,
     0xf5,
     0x68,
     0xa3,
     0xe0,
     0xf5,
     0x69,
     0xef,
     0x12,
     0x09,
     0xed,
     0x0c,
     0xd5,
     0x03,
     0x0c,
     0xea,
     0x05,
     0x0d,
     0x14,
     0x06,
     0x0d,
     0x02,
     0x08,
     0x0d,
     0x21,
     0x10,
     0x0d,
     0x35,
     0x12,
     0x0d,
     0x3a,
     0x20,
     0x0d,
     0x48,
     0x21,
     0x0d,
     0x4d,
     0x30,
     0x0d,
     0x76,
     0x50,
     0x0d,
     0x58,
     0xd8,
     0x00,
     0x00,
     0x0d,
     0x83,
     0x20,
     0x05,
     0x03,
     0x02,
     0x0d,
     0x83,
     0x30,
     0x00,
     0x03,
     0x02,
     0x0d,
     0x83,
     0xd2,
     0x07,
     0xc2,
     0x06,
     0x12,
     0x02,
     0x8a,
     0x80,
     0x24,
     0x20,
     0x05,
     0x03,
     0x02,
     0x0d,
     0x83,
     0x30,
     0x00,
     0x03,
     0x02,
     0x0d,
     0x83,
     0xc2,
     0x07,
     0xd2,
     0x06,
     0x12,
     0x02,
     0x9b,
     0xc2,
     0x04,
     0x02,
     0x0d,
     0x83,
     0x12,
     0x02,
     0x51,
     0x30,
     0x05,
     0x06,
     0xe4,
     0xf5,
     0x0c,
     0x12,
     0x11,
     0xd8,
     0xc2,
     0x31,
     0xd2,
     0x34,
     0x80,
     0x6f,
     0x30,
     0x07,
     0x6c,
     0x30,
     0x06,
     0x69,
     0x12,
     0x02,
     0x8a,
     0xd2,
     0x31,
     0x80,
     0x62,
     0x20,
     0x07,
     0x03,
     0x30,
     0x06,
     0x09,
     0xe5,
     0x7e,
     0x64,
     0x0e,
     0x70,
     0x56,
     0x20,
     0x00,
     0x53,
     0x12,
     0x05,
     0x25,
     0x80,
     0x4e,
     0x12,
     0x06,
     0xee,
     0x80,
     0x49,
     0x30,
     0x05,
     0x46,
     0x20,
     0x07,
     0x43,
     0x20,
     0x06,
     0x40,
     0x12,
     0x15,
     0x4b,
     0x80,
     0x3b,
     0x12,
     0x10,
     0x3e,
     0x80,
     0x36,
     0x20,
     0x07,
     0x33,
     0x20,
     0x06,
     0x30,
     0x12,
     0x15,
     0x5a,
     0x80,
     0x2b,
     0xe5,
     0x7e,
     0x64,
     0x01,
     0x70,
     0x25,
     0xd2,
     0x35,
     0x90,
     0x50,
     0x82,
     0xe5,
     0x74,
     0xf0,
     0xa3,
     0xe5,
     0x75,
     0xf0,
     0xa3,
     0xe5,
     0x76,
     0xf0,
     0xa3,
     0xe5,
     0x77,
     0xf0,
     0xc2,
     0x35,
     0x80,
     0x0d,
     0x90,
     0x50,
     0x82,
     0x30,
     0x33,
     0x05,
     0x74,
     0x55,
     0xf0,
     0x80,
     0x02,
     0xe4,
     0xf0,
     0x20,
     0x07,
     0x06,
     0x30,
     0x06,
     0x03,
     0x30,
     0x04,
     0x05,
     0x90,
     0x30,
     0x25,
     0xe4,
     0xf0,
     0x22,
     0x30,
     0x04,
     0x03,
     0x02,
     0x0e,
     0x72,
     0xd2,
     0x04,
     0xe5,
     0x7e,
     0xb4,
     0x01,
     0x06,
     0x12,
     0x15,
     0x2b,
     0x02,
     0x0e,
     0x6b,
     0xe5,
     0x7e,
     0xb4,
     0x02,
     0x06,
     0x12,
     0x15,
     0x3c,
     0x02,
     0x0e,
     0x6b,
     0xe5,
     0x7e,
     0xb4,
     0x03,
     0x05,
     0xe4,
     0xf5,
     0x0c,
     0x80,
     0x08,
     0xe5,
     0x7e,
     0xb4,
     0x04,
     0x09,
     0x85,
     0x7c,
     0x0c,
     0x12,
     0x11,
     0xd8,
     0x02,
     0x0e,
     0x6b,
     0xe5,
     0x7e,
     0x64,
     0x0f,
     0x70,
     0x15,
     0x12,
     0x02,
     0xa9,
     0x40,
     0x06,
     0x7e,
     0x03,
     0x7f,
     0xff,
     0x80,
     0x04,
     0xae,
     0x68,
     0xaf,
     0x69,
     0x12,
     0x0e,
     0x73,
     0x02,
     0x0e,
     0x6b,
     0xe5,
     0x7e,
     0x64,
     0x10,
     0x60,
     0x03,
     0x02,
     0x0e,
     0x6b,
     0xf5,
     0x66,
     0xf5,
     0x67,
     0xf5,
     0x68,
     0xab,
     0x69,
     0xaa,
     0x68,
     0xa9,
     0x67,
     0xa8,
     0x66,
     0x12,
     0x01,
     0x73,
     0xfe,
     0xe4,
     0xfc,
     0xfd,
     0x12,
     0x01,
     0xf1,
     0xe4,
     0x7b,
     0xff,
     0xfa,
     0xf9,
     0xf8,
     0x12,
     0x02,
     0x67,
     0x12,
     0x02,
     0x39,
     0xe4,
     0x93,
     0xfe,
     0x74,
     0x01,
     0x93,
     0xff,
     0xe4,
     0xfc,
     0xfd,
     0xe5,
     0x69,
     0x2f,
     0xf5,
     0x69,
     0xe5,
     0x68,
     0x3e,
     0xf5,
     0x68,
     0xed,
     0x35,
     0x67,
     0xf5,
     0x67,
     0xec,
     0x35,
     0x66,
     0xf5,
     0x66,
     0x12,
     0x02,
     0xa9,
     0x40,
     0x06,
     0x7e,
     0x03,
     0x7f,
     0xff,
     0x80,
     0x04,
     0xae,
     0x68,
     0xaf,
     0x69,
     0x12,
     0x0e,
     0x73,
     0xe4,
     0xf5,
     0x67,
     0xf5,
     0x67,
     0xe5,
     0x67,
     0xd3,
     0x95,
     0x7c,
     0x50,
     0x1c,
     0x12,
     0x02,
     0x39,
     0xaf,
     0x67,
     0x75,
     0xf0,
     0x02,
     0xef,
     0x12,
     0x09,
     0xe1,
     0xc3,
     0x74,
     0x01,
     0x93,
     0x95,
     0x69,
     0xe4,
     0x93,
     0x95,
     0x68,
     0x50,
     0x04,
     0x05,
     0x67,
     0x80,
     0xdd,
     0x85,
     0x67,
     0x7d,
     0x90,
     0x30,
     0x25,
     0xe4,
     0xf0,
     0xd2,
     0x34,
     0x22,
     0x8e,
     0x68,
     0x8f,
     0x69,
     0x85,
     0x68,
     0x64,
     0x85,
     0x69,
     0x65,
     0xe5,
     0x69,
     0xc4,
     0xf8,
     0x54,
     0x0f,
     0xc8,
     0x68,
     0xf5,
     0x69,
     0xe5,
     0x68,
     0xc4,
     0x54,
     0xf0,
     0x48,
     0xf5,
     0x68,
     0x85,
     0x68,
     0x0d,
     0x85,
     0x69,
     0x0e,
     0x12,
     0x0a,
     0x13,
     0x22,
     0x12,
     0x02,
     0x7f,
     0xb5,
     0x07,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x40,
     0x03,
     0x02,
     0x0f,
     0x38,
     0x90,
     0x30,
     0x04,
     0xe0,
     0x44,
     0x20,
     0xf0,
     0xa3,
     0xe0,
     0x44,
     0x40,
     0xf0,
     0x90,
     0x50,
     0x25,
     0xe0,
     0x44,
     0x04,
     0xf0,
     0x90,
     0x50,
     0x03,
     0xe0,
     0x54,
     0xfd,
     0xf0,
     0x90,
     0x50,
     0x27,
     0xe0,
     0x44,
     0x01,
     0xf0,
     0x90,
     0x50,
     0x31,
     0xe4,
     0xf0,
     0x90,
     0x50,
     0x33,
     0xf0,
     0x90,
     0x30,
     0x1e,
     0x12,
     0x02,
     0x05,
     0x90,
     0x30,
     0x18,
     0x12,
     0x02,
     0x05,
     0x90,
     0x30,
     0x1b,
     0x12,
     0x02,
     0x05,
     0xe0,
     0xf5,
     0x25,
     0x90,
     0x30,
     0x18,
     0xe0,
     0xf5,
     0x21,
     0x90,
     0x60,
     0x00,
     0x74,
     0xf5,
     0xf0,
     0x90,
     0x3f,
     0x01,
     0xe4,
     0xf0,
     0xa3,
     0xf0,
     0x90,
     0x3f,
     0x01,
     0xe0,
     0x44,
     0x08,
     0xf0,
     0xe0,
     0x44,
     0x20,
     0xf0,
     0x90,
     0x3f,
     0x05,
     0x74,
     0x30,
     0xf0,
     0xa3,
     0x74,
     0x24,
     0xf0,
     0x90,
     0x3f,
     0x0b,
     0xe0,
     0x44,
     0x0f,
     0xf0,
     0x90,
     0x3f,
     0x01,
     0xe0,
     0x44,
     0x02,
     0xf0,
     0xc2,
     0x8c,
     0x75,
     0x89,
     0x03,
     0x75,
     0xa8,
     0x07,
     0x75,
     0xb8,
     0x04,
     0xe4,
     0xf5,
     0xd8,
     0xf5,
     0xe8,
     0x90,
     0x30,
     0x01,
     0xe0,
     0x44,
     0x40,
     0xf0,
     0xe0,
     0x54,
     0xbf,
     0xf0,
     0x22,
     0x30,
     0x3c,
     0x09,
     0x30,
     0x20,
     0x06,
     0xae,
     0x56,
     0xaf,
     0x57,
     0x80,
     0x04,
     0xae,
     0x6a,
     0xaf,
     0x6b,
     0x8e,
     0x56,
     0x8f,
     0x57,
     0x30,
     0x3c,
     0x09,
     0x30,
     0x21,
     0x06,
     0xae,
     0x58,
     0xaf,
     0x59,
     0x80,
     0x04,
     0xae,
     0x6c,
     0xaf,
     0x6d,
     0x8e,
     0x58,
     0x8f,
     0x59,
     0x30,
     0x3c,
     0x09,
     0x30,
     0x22,
     0x06,
     0xae,
     0x5a,
     0xaf,
     0x5b,
     0x80,
     0x04,
     0xae,
     0x6e,
     0xaf,
     0x6f,
     0x8e,
     0x5a,
     0x8f,
     0x5b,
     0x30,
     0x3c,
     0x09,
     0x30,
     0x23,
     0x06,
     0xae,
     0x5c,
     0xaf,
     0x5d,
     0x80,
     0x04,
     0xae,
     0x70,
     0xaf,
     0x71,
     0x8e,
     0x5c,
     0x8f,
     0x5d,
     0x30,
     0x3c,
     0x09,
     0x30,
     0x24,
     0x06,
     0xae,
     0x5e,
     0xaf,
     0x5f,
     0x80,
     0x04,
     0xae,
     0x72,
     0xaf,
     0x73,
     0x8e,
     0x5e,
     0x8f,
     0x5f,
     0x30,
     0x3c,
     0x09,
     0x30,
     0x25,
     0x06,
     0xae,
     0x60,
     0xaf,
     0x61,
     0x80,
     0x04,
     0xae,
     0x74,
     0xaf,
     0x75,
     0x8e,
     0x60,
     0x8f,
     0x61,
     0x30,
     0x3c,
     0x09,
     0x30,
     0x26,
     0x06,
     0xae,
     0x62,
     0xaf,
     0x63,
     0x80,
     0x04,
     0xae,
     0x76,
     0xaf,
     0x77,
     0x8e,
     0x62,
     0x8f,
     0x63,
     0x22,
     0xd3,
     0xe5,
     0x57,
     0x95,
     0x6b,
     0xe5,
     0x56,
     0x95,
     0x6a,
     0x40,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x20,
     0xd3,
     0xe5,
     0x59,
     0x95,
     0x6d,
     0xe5,
     0x58,
     0x95,
     0x6c,
     0x40,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x21,
     0xd3,
     0xe5,
     0x5b,
     0x95,
     0x6f,
     0xe5,
     0x5a,
     0x95,
     0x6e,
     0x40,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x22,
     0xd3,
     0xe5,
     0x5d,
     0x95,
     0x71,
     0xe5,
     0x5c,
     0x95,
     0x70,
     0x40,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x23,
     0xd3,
     0xe5,
     0x5f,
     0x95,
     0x73,
     0xe5,
     0x5e,
     0x95,
     0x72,
     0x40,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x24,
     0xd3,
     0xe5,
     0x61,
     0x95,
     0x75,
     0xe5,
     0x60,
     0x95,
     0x74,
     0x40,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x25,
     0xd3,
     0xe5,
     0x63,
     0x95,
     0x77,
     0xe5,
     0x62,
     0x95,
     0x76,
     0x40,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x92,
     0x26,
     0x22,
     0xe5,
     0x7e,
     0x64,
     0x01,
     0x70,
     0x50,
     0x12,
     0x02,
     0x39,
     0xe5,
     0x7d,
     0x12,
     0x01,
     0x7b,
     0xfe,
     0xe4,
     0x8f,
     0x69,
     0x8e,
     0x68,
     0xf5,
     0x67,
     0xf5,
     0x66,
     0x12,
     0x01,
     0xfc,
     0x7b,
     0xff,
     0xfa,
     0xf9,
     0xf8,
     0x12,
     0x01,
     0xf1,
     0xc0,
     0x04,
     0xc0,
     0x05,
     0xc0,
     0x06,
     0xc0,
     0x07,
     0x12,
     0x01,
     0x73,
     0xab,
     0x07,
     0xfa,
     0xe4,
     0xf9,
     0xf8,
     0xd0,
     0x07,
     0xd0,
     0x06,
     0xd0,
     0x05,
     0xd0,
     0x04,
     0x12,
     0x02,
     0x67,
     0x85,
     0x69,
     0x66,
     0x85,
     0x7d,
     0x67,
     0x12,
     0x02,
     0x39,
     0xe5,
     0x7d,
     0x12,
     0x02,
     0x95,
     0xe4,
     0x93,
     0xf5,
     0x68,
     0x74,
     0x01,
     0x93,
     0xf5,
     0x69,
     0x90,
     0x50,
     0x82,
     0xe5,
     0x66,
     0xf0,
     0xa3,
     0xe5,
     0x67,
     0xf0,
     0xa3,
     0xe5,
     0x68,
     0xf0,
     0xa3,
     0xe5,
     0x69,
     0xf0,
     0x22,
     0x56,
     0x0c,
     0x04,
     0x00,
     0x00,
     0x00,
     0xc8,
     0x01,
     0x2c,
     0x01,
     0x5e,
     0x01,
     0x8b,
     0x01,
     0xb8,
     0x01,
     0xe5,
     0x02,
     0x12,
     0x02,
     0x3f,
     0x02,
     0x6c,
     0x02,
     0x99,
     0x02,
     0xc6,
     0x02,
     0xf3,
     0x07,
     0x20,
     0x12,
     0x28,
     0x1e,
     0x18,
     0x18,
     0x28,
     0x1e,
     0x18,
     0x12,
     0x28,
     0x1e,
     0x18,
     0x12,
     0x28,
     0x18,
     0x18,
     0x12,
     0x20,
     0x18,
     0x28,
     0x1c,
     0x30,
     0x24,
     0x10,
     0x1c,
     0x18,
     0x24,
     0x1c,
     0x14,
     0x24,
     0x1c,
     0x28,
     0x0c,
     0x30,
     0x14,
     0x10,
     0x0c,
     0x18,
     0x14,
     0x1c,
     0x20,
     0x24,
     0x28,
     0x0c,
     0x14,
     0x14,
     0x1c,
     0x1c,
     0x14,
     0x24,
     0x1c,
     0x2c,
     0x14,
     0x34,
     0x1c,
     0x1c,
     0x08,
     0x24,
     0x10,
     0x19,
     0x19,
     0x1c,
     0x19,
     0x19,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0xe5,
     0x0a,
     0x70,
     0x04,
     0x7a,
     0x10,
     0x7b,
     0xc5,
     0xe5,
     0x0a,
     0xb4,
     0x01,
     0x04,
     0x7a,
     0x10,
     0x7b,
     0xd9,
     0xe5,
     0x0a,
     0xb4,
     0x02,
     0x04,
     0x7a,
     0x10,
     0x7b,
     0xed,
     0x8b,
     0x82,
     0x8a,
     0x83,
     0x12,
     0x09,
     0xd1,
     0x8f,
     0x37,
     0x8e,
     0x36,
     0x8d,
     0x35,
     0x8c,
     0x34,
     0xe5,
     0x82,
     0x24,
     0x04,
     0xf5,
     0x82,
     0xe4,
     0x35,
     0x83,
     0xf5,
     0x83,
     0x12,
     0x09,
     0xd1,
     0x8f,
     0x3b,
     0x8e,
     0x3a,
     0x8d,
     0x39,
     0x8c,
     0x38,
     0xeb,
     0x24,
     0x08,
     0x12,
     0x02,
     0x40,
     0x12,
     0x01,
     0xc0,
     0xeb,
     0x24,
     0x0c,
     0x12,
     0x02,
     0x40,
     0x8f,
     0x43,
     0x8e,
     0x42,
     0x8d,
     0x41,
     0x8c,
     0x40,
     0xeb,
     0x24,
     0x10,
     0x12,
     0x02,
     0x40,
     0x8f,
     0x47,
     0x8e,
     0x46,
     0x8d,
     0x45,
     0x8c,
     0x44,
     0x22,
     0x30,
     0x3c,
     0x07,
     0x30,
     0x20,
     0x04,
     0xaf,
     0x4a,
     0x80,
     0x02,
     0xaf,
     0x7d,
     0x8f,
     0x4a,
     0x30,
     0x3c,
     0x07,
     0x30,
     0x21,
     0x04,
     0xaf,
     0x4b,
     0x80,
     0x02,
     0xaf,
     0x7d,
     0x8f,
     0x4b,
     0x30,
     0x3c,
     0x07,
     0x30,
     0x22,
     0x04,
     0xaf,
     0x4c,
     0x80,
     0x02,
     0xaf,
     0x7d,
     0x8f,
     0x4c,
     0x30,
     0x3c,
     0x07,
     0x30,
     0x23,
     0x04,
     0xaf,
     0x4d,
     0x80,
     0x02,
     0xaf,
     0x7d,
     0x8f,
     0x4d,
     0x30,
     0x3c,
     0x07,
     0x30,
     0x24,
     0x04,
     0xaf,
     0x4e,
     0x80,
     0x02,
     0xaf,
     0x7d,
     0x8f,
     0x4e,
     0x30,
     0x3c,
     0x07,
     0x30,
     0x25,
     0x04,
     0xaf,
     0x4f,
     0x80,
     0x02,
     0xaf,
     0x7d,
     0x8f,
     0x4f,
     0x30,
     0x3c,
     0x07,
     0x30,
     0x26,
     0x04,
     0xaf,
     0x50,
     0x80,
     0x02,
     0xaf,
     0x7d,
     0x8f,
     0x50,
     0x22,
     0xe5,
     0x0c,
     0xd3,
     0x95,
     0x7c,
     0x40,
     0x01,
     0x22,
     0x12,
     0x02,
     0x39,
     0xe5,
     0x0c,
     0x12,
     0x02,
     0x95,
     0xe4,
     0x93,
     0xfe,
     0x74,
     0x01,
     0x93,
     0xff,
     0x4e,
     0x60,
     0x21,
     0x8e,
     0x64,
     0x8f,
     0x65,
     0xef,
     0xc4,
     0xf8,
     0x54,
     0x0f,
     0xc8,
     0x68,
     0xff,
     0xee,
     0xc4,
     0x54,
     0xf0,
     0x48,
     0xfe,
     0x43,
     0x07,
     0x0d,
     0x8e,
     0x0d,
     0x8f,
     0x0e,
     0x12,
     0x0a,
     0x13,
     0x30,
     0x33,
     0x22,
     0xc3,
     0x22,
     0x75,
     0x0d,
     0x00,
     0x75,
     0x0e,
     0x0d,
     0x12,
     0x0a,
     0x13,
     0x30,
     0x33,
     0x02,
     0xc3,
     0x22,
     0x75,
     0x0d,
     0x00,
     0x75,
     0x0e,
     0x64,
     0x12,
     0x14,
     0xcc,
     0x75,
     0x0d,
     0x80,
     0x75,
     0x0e,
     0x00,
     0x12,
     0x0a,
     0x13,
     0x85,
     0x0c,
     0x7d,
     0xd3,
     0x22,
     0xc2,
     0x34,
     0x20,
     0x05,
     0x05,
     0x75,
     0x0a,
     0xee,
     0x80,
     0x36,
     0x20,
     0x07,
     0x08,
     0x20,
     0x06,
     0x05,
     0xe4,
     0xf5,
     0x0a,
     0x80,
     0x2b,
     0x20,
     0x07,
     0x08,
     0x30,
     0x06,
     0x05,
     0x75,
     0x0a,
     0x20,
     0x80,
     0x20,
     0x30,
     0x00,
     0x05,
     0x75,
     0x0a,
     0x01,
     0x80,
     0x18,
     0xe5,
     0x20,
     0x54,
     0x07,
     0xff,
     0xbf,
     0x06,
     0x0d,
     0x30,
     0x31,
     0x04,
     0x7f,
     0x12,
     0x80,
     0x02,
     0x7f,
     0x02,
     0x8f,
     0x0a,
     0x80,
     0x03,
     0x75,
     0x0a,
     0xfe,
     0x90,
     0x30,
     0x27,
     0xe5,
     0x0a,
     0xf0,
     0xe5,
     0x23,
     0x54,
     0xf8,
     0xf5,
     0x0a,
     0xe5,
     0x79,
     0x25,
     0x0a,
     0xf5,
     0x0a,
     0x90,
     0x30,
     0x26,
     0xe5,
     0x0a,
     0xf0,
     0x22,
     0xe5,
     0x0a,
     0x70,
     0x04,
     0x7e,
     0x11,
     0x7f,
     0x01,
     0xe5,
     0x0a,
     0xb4,
     0x01,
     0x04,
     0x7e,
     0x11,
     0x7f,
     0x06,
     0xe5,
     0x0a,
     0xb4,
     0x02,
     0x04,
     0x7e,
     0x11,
     0x7f,
     0x0b,
     0x8f,
     0x82,
     0x8e,
     0x83,
     0xe4,
     0x93,
     0xf5,
     0x2c,
     0x74,
     0x01,
     0x93,
     0xf5,
     0x2d,
     0x74,
     0x02,
     0x93,
     0xf5,
     0x2e,
     0x74,
     0x03,
     0x93,
     0xf5,
     0x2f,
     0x74,
     0x04,
     0x93,
     0xf5,
     0x30,
     0xe5,
     0x0a,
     0xb4,
     0x01,
     0x07,
     0x74,
     0x2c,
     0x25,
     0x79,
     0xf8,
     0x76,
     0x40,
     0xe5,
     0x0a,
     0xb4,
     0x02,
     0x07,
     0x74,
     0x2c,
     0x25,
     0x79,
     0xf8,
     0x76,
     0x80,
     0x22,
     0xc0,
     0xe0,
     0xc0,
     0x83,
     0xc0,
     0x82,
     0xc0,
     0xd0,
     0x90,
     0x3f,
     0x0d,
     0xe0,
     0xf5,
     0x09,
     0xe5,
     0x09,
     0x30,
     0xe0,
     0x2e,
     0xe5,
     0x7a,
     0xb4,
     0x01,
     0x09,
     0x90,
     0x3a,
     0x00,
     0xe0,
     0xf5,
     0x78,
     0x44,
     0x01,
     0xf0,
     0xe5,
     0x7a,
     0xb4,
     0x03,
     0x09,
     0x90,
     0x3a,
     0x00,
     0xe0,
     0xf5,
     0x78,
     0x54,
     0xfe,
     0xf0,
     0xe5,
     0x7a,
     0xb4,
     0x03,
     0x05,
     0x75,
     0x7a,
     0x00,
     0x80,
     0x02,
     0x05,
     0x7a,
     0x90,
     0x3f,
     0x0d,
     0x74,
     0x01,
     0xf0,
     0xd0,
     0xd0,
     0xd0,
     0x82,
     0xd0,
     0x83,
     0xd0,
     0xe0,
     0x32,
     0x90,
     0x50,
     0x27,
     0xe0,
     0x44,
     0x01,
     0xf0,
     0x90,
     0x50,
     0x34,
     0x74,
     0x80,
     0xf0,
     0xa3,
     0x74,
     0x2a,
     0xf0,
     0xa3,
     0x74,
     0x14,
     0xf0,
     0x90,
     0x50,
     0x30,
     0xe4,
     0xf0,
     0xa3,
     0x74,
     0x02,
     0xf0,
     0xa3,
     0xe4,
     0xf0,
     0xa3,
     0x74,
     0x80,
     0xf0,
     0xe4,
     0xf5,
     0x0a,
     0x12,
     0x11,
     0x10,
     0x75,
     0x79,
     0x02,
     0x75,
     0x0a,
     0x01,
     0x12,
     0x12,
     0x91,
     0xd2,
     0x18,
     0xd2,
     0x19,
     0xc2,
     0x3a,
     0xc2,
     0x39,
     0xd2,
     0x1a,
     0xd2,
     0x36,
     0xd2,
     0x30,
     0xc2,
     0x35,
     0xc2,
     0x3b,
     0x22,
     0x85,
     0x10,
     0x11,
     0x7f,
     0x08,
     0xe5,
     0x11,
     0x30,
     0xe7,
     0x04,
     0xd2,
     0x29,
     0x80,
     0x02,
     0xc2,
     0x29,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x0a,
     0xae,
     0x51,
     0x15,
     0x51,
     0xee,
     0x70,
     0xf9,
     0xe5,
     0x11,
     0x25,
     0xe0,
     0xf5,
     0x11,
     0xd2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x0a,
     0xae,
     0x51,
     0x15,
     0x51,
     0xee,
     0x70,
     0xf9,
     0xc2,
     0x28,
     0x12,
     0x01,
     0x6c,
     0x75,
     0x51,
     0x05,
     0xae,
     0x51,
     0x15,
     0x51,
     0xee,
     0x70,
     0xf9,
     0xdf,
     0xc2,
     0x22,
     0xc2,
     0xaf,
     0x90,
     0x30,
     0x27,
     0x74,
     0xfa,
     0xf0,
     0x12,
     0x0e,
     0x99,
     0x12,
     0x14,
     0x83,
     0xe4,
     0xf5,
     0x33,
     0xd2,
     0xaf,
     0x12,
     0x0c,
     0x84,
     0x30,
     0x30,
     0x03,
     0x12,
     0x06,
     0xee,
     0x30,
     0x34,
     0x03,
     0x12,
     0x12,
     0x38,
     0x30,
     0x3b,
     0xee,
     0xc2,
     0x3b,
     0xd2,
     0x35,
     0x30,
     0x00,
     0x05,
     0x12,
     0x14,
     0x56,
     0x80,
     0x09,
     0x20,
     0x07,
     0x06,
     0x30,
     0x06,
     0x03,
     0x12,
     0x0d,
     0x92,
     0xc2,
     0x35,
     0x80,
     0xd5,
     0x12,
     0x0f,
     0xc6,
     0xd2,
     0x3c,
     0x12,
     0x0f,
     0x39,
     0xd2,
     0x3c,
     0x12,
     0x11,
     0x75,
     0xe5,
     0x7c,
     0xd3,
     0x95,
     0x7d,
     0x40,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x50,
     0x14,
     0x20,
     0x37,
     0x0e,
     0xe5,
     0x24,
     0x54,
     0x1f,
     0xff,
     0xbf,
     0x1f,
     0x06,
     0x30,
     0x25,
     0x03,
     0x20,
     0x26,
     0x03,
     0x02,
     0x15,
     0x2b,
     0x12,
     0x02,
     0xb3,
     0x22,
     0xe5,
     0x7d,
     0x70,
     0x19,
     0x12,
     0x15,
     0x03,
     0xc2,
     0x3c,
     0x12,
     0x0f,
     0x39,
     0xc2,
     0x3c,
     0x12,
     0x11,
     0x75,
     0xc2,
     0x03,
     0x12,
     0x15,
     0x2b,
     0xd2,
     0x02,
     0xd2,
     0x01,
     0xd2,
     0x00,
     0x22,
     0x30,
     0x03,
     0x08,
     0xc2,
     0x03,
     0xc2,
     0x04,
     0x12,
     0x02,
     0x9b,
     0x22,
     0xe4,
     0xf5,
     0x0c,
     0x12,
     0x11,
     0xd8,
     0xd2,
     0x03,
     0x22,
     0xe5,
     0x20,
     0x54,
     0x07,
     0xff,
     0xbf,
     0x01,
     0x03,
     0x02,
     0x14,
     0x25,
     0xe5,
     0x20,
     0x54,
     0x07,
     0xff,
     0xbf,
     0x07,
     0x03,
     0x02,
     0x14,
     0xa9,
     0xe5,
     0x20,
     0x54,
     0x07,
     0xff,
     0xbf,
     0x03,
     0x03,
     0x02,
     0x13,
     0xf3,
     0xe5,
     0x20,
     0x54,
     0x07,
     0xff,
     0xbf,
     0x05,
     0x03,
     0x12,
     0x15,
     0x69,
     0x22,
     0x12,
     0x13,
     0x2a,
     0x12,
     0x15,
     0x77,
     0x50,
     0x04,
     0xd2,
     0x05,
     0x80,
     0x02,
     0xc2,
     0x05,
     0x12,
     0x02,
     0x51,
     0xc2,
     0x37,
     0xc2,
     0x31,
     0xd2,
     0x34,
     0x12,
     0x02,
     0x7f,
     0xb5,
     0x07,
     0x03,
     0xd3,
     0x80,
     0x01,
     0xc3,
     0x40,
     0x02,
     0xc2,
     0x05,
     0x22,
     0x12,
     0x0f,
     0xc6,
     0xd2,
     0x3c,
     0x12,
     0x0f,
     0x39,
     0xd2,
     0x3c,
     0x12,
     0x11,
     0x75,
     0x12,
     0x15,
     0x2b,
     0xe5,
     0x32,
     0xd3,
     0x95,
     0x7d,
     0x40,
     0x05,
     0xe4,
     0x95,
     0x7d,
     0x40,
     0x06,
     0xc2,
     0x02,
     0xd2,
     0x01,
     0xd2,
     0x00,
     0x22,
     0xe4,
     0xff,
     0xfe,
     0xc3,
     0xef,
     0x95,
     0x0e,
     0xee,
     0x95,
     0x0d,
     0x50,
     0x15,
     0x7d,
     0x8a,
     0x7c,
     0x02,
     0xed,
     0x1d,
     0xaa,
     0x04,
     0x70,
     0x01,
     0x1c,
     0x4a,
     0x70,
     0xf6,
     0x0f,
     0xbf,
     0x00,
     0x01,
     0x0e,
     0x80,
     0xe2,
     0x22,
     0x75,
     0x48,
     0x10,
     0x75,
     0x49,
     0xaa,
     0x90,
     0x10,
     0xa8,
     0xe4,
     0x93,
     0xf5,
     0x7c,
     0xa3,
     0xe4,
     0x93,
     0xf5,
     0x32,
     0xc2,
     0x38,
     0x22,
     0xe4,
     0xff,
     0xef,
     0x25,
     0xe0,
     0x24,
     0x56,
     0xf8,
     0xe4,
     0xf6,
     0x08,
     0xf6,
     0x0f,
     0xbf,
     0x07,
     0xf2,
     0x53,
     0x24,
     0x80,
     0x22,
     0xc2,
     0x03,
     0xd2,
     0x04,
     0x12,
     0x02,
     0x9b,
     0xc2,
     0x3c,
     0x12,
     0x0f,
     0x39,
     0xc2,
     0x3c,
     0x12,
     0x11,
     0x75,
     0xd2,
     0x34,
     0x22,
     0xe5,
     0x7d,
     0xc3,
     0x95,
     0x7c,
     0x40,
     0x01,
     0x22,
     0xe5,
     0x7d,
     0x04,
     0xf5,
     0x0c,
     0x12,
     0x11,
     0xd8,
     0x22,
     0xe5,
     0x7d,
     0x70,
     0x02,
     0xc3,
     0x22,
     0xe5,
     0x7d,
     0x14,
     0xf5,
     0x0c,
     0x12,
     0x11,
     0xd8,
     0x22,
     0xe5,
     0x7e,
     0xb4,
     0x01,
     0x09,
     0x12,
     0x14,
     0xee,
     0xe4,
     0xf5,
     0x0c,
     0x12,
     0x11,
     0xd8,
     0x22,
     0xe5,
     0x7e,
     0x24,
     0xfe,
     0x60,
     0x06,
     0x04,
     0x70,
     0x05,
     0xd2,
     0x37,
     0x22,
     0xc2,
     0x37,
     0x22,
     0xe5,
     0x31,
     0xd3,
     0x94,
     0x00,
     0x40,
     0x03,
     0x15,
     0x31,
     0x22,
     0x12,
     0x15,
     0x17,
     0x22,
     0x12,
     0x14,
     0xee,
     0xe4,
     0xf5,
     0x0c,
     0x12,
     0x11,
     0xd8,
     0x22,
    }; 

static void OV5642_FOCUS_AD5820_Init(void)
{
    UINT8 state=0x8F;
    UINT32 iteration = 100;
    int totalCnt = 0; 
    int sentCnt = 0; 
    int index=0; 
    u16 addr = 0x8000; 
    u8 buf[256]; 
    int len = 128;

    SENSORDB("OV5642_FOCUS_AD5820_Init\n");     

    KD_IMGSENSOR_PROFILE_INIT();

#if 1
    OV5642_write_cmos_sensor(0x3000, 0x20);

    totalCnt = ARRAY_SIZE(AD5820_Config); 


    SENSORDB("Total Count = %d\n", totalCnt); 
    while (index < totalCnt) {        
        sentCnt = totalCnt - index > len  ? len  : totalCnt - index; 
        //SENSORDB("Index = %d, sent Cnt = %d, addr = 0x%x\n", index, sentCnt, addr); 
        buf[0] = addr >> 8; 
        buf[1] = addr & 0xff; 
        memcpy(&buf[2], &AD5820_Config[index], len );
        OV5642_burst_write_cmos_sensor(buf, sentCnt + 2); 
        addr += len ; 
        index += len ;
    }
    //kfree(buf); 
    OV5642_write_cmos_sensor(0x3024, 0x00);
    OV5642_write_cmos_sensor(0x3025, 0x00);
    OV5642_write_cmos_sensor(0x5082, 0x00);
    OV5642_write_cmos_sensor(0x5083, 0x00);
    OV5642_write_cmos_sensor(0x5084, 0x00);
    OV5642_write_cmos_sensor(0x5085, 0x00);
    OV5642_write_cmos_sensor(0x3026, 0x00);
    OV5642_write_cmos_sensor(0x3027, 0xFF);
    OV5642_write_cmos_sensor(0x3000, 0x00);
#if 0
    for (i = 0x8000; i < 0x8000 + totalCnt; i++) {
        SENSORDB("Addr = 0x%x, data = 0x%02x\n", i, OV5642_read_cmos_sensor(i)); 
    }
#endif    
#else 
    OV5642_write_cmos_sensor(0x3000, 0x20);
    OV5642_write_cmos_sensor(0x8000, 0x02);
    OV5642_write_cmos_sensor(0x8001, 0x00);
    OV5642_write_cmos_sensor(0x8002, 0x06);
    OV5642_write_cmos_sensor(0x8003, 0x02);
    OV5642_write_cmos_sensor(0x8004, 0x0b);
    OV5642_write_cmos_sensor(0x8005, 0x5f);
    OV5642_write_cmos_sensor(0x8006, 0x78);
    OV5642_write_cmos_sensor(0x8007, 0x7f);
    OV5642_write_cmos_sensor(0x8008, 0xe4);
    OV5642_write_cmos_sensor(0x8009, 0xf6);
    OV5642_write_cmos_sensor(0x800a, 0xd8);
    OV5642_write_cmos_sensor(0x800b, 0xfd);
    OV5642_write_cmos_sensor(0x800c, 0x75);
    OV5642_write_cmos_sensor(0x800d, 0x81);
    OV5642_write_cmos_sensor(0x800e, 0x7e);
    OV5642_write_cmos_sensor(0x800f, 0x02);
    OV5642_write_cmos_sensor(0x8010, 0x13);
    OV5642_write_cmos_sensor(0x8011, 0xb5);
    OV5642_write_cmos_sensor(0x8012, 0x00);
    OV5642_write_cmos_sensor(0x8013, 0x02);
    OV5642_write_cmos_sensor(0x8014, 0x12);
    OV5642_write_cmos_sensor(0x8015, 0xe0);
    OV5642_write_cmos_sensor(0x8016, 0xe0);
    OV5642_write_cmos_sensor(0x8017, 0xf5);
    OV5642_write_cmos_sensor(0x8018, 0x72);
    OV5642_write_cmos_sensor(0x8019, 0xa3);
    OV5642_write_cmos_sensor(0x801a, 0xe0);
    OV5642_write_cmos_sensor(0x801b, 0xf5);
    OV5642_write_cmos_sensor(0x801c, 0x73);
    OV5642_write_cmos_sensor(0x801d, 0xae);
    OV5642_write_cmos_sensor(0x801e, 0x6a);
    OV5642_write_cmos_sensor(0x801f, 0xe4);
    OV5642_write_cmos_sensor(0x8020, 0x85);
    OV5642_write_cmos_sensor(0x8021, 0x6b);
    OV5642_write_cmos_sensor(0x8022, 0x55);
    OV5642_write_cmos_sensor(0x8023, 0x8e);
    OV5642_write_cmos_sensor(0x8024, 0x54);
    OV5642_write_cmos_sensor(0x8025, 0xf5);
    OV5642_write_cmos_sensor(0x8026, 0x53);
    OV5642_write_cmos_sensor(0x8027, 0xf5);
    OV5642_write_cmos_sensor(0x8028, 0x52);
    OV5642_write_cmos_sensor(0x8029, 0xab);
    OV5642_write_cmos_sensor(0x802a, 0x55);
    OV5642_write_cmos_sensor(0x802b, 0xaa);
    OV5642_write_cmos_sensor(0x802c, 0x54);
    OV5642_write_cmos_sensor(0x802d, 0xa9);
    OV5642_write_cmos_sensor(0x802e, 0x53);
    OV5642_write_cmos_sensor(0x802f, 0xa8);
    OV5642_write_cmos_sensor(0x8030, 0x52);
    OV5642_write_cmos_sensor(0x8031, 0xaf);
    OV5642_write_cmos_sensor(0x8032, 0x2c);
    OV5642_write_cmos_sensor(0x8033, 0xfc);
    OV5642_write_cmos_sensor(0x8034, 0xfd);
    OV5642_write_cmos_sensor(0x8035, 0xfe);
    OV5642_write_cmos_sensor(0x8036, 0x12);
    OV5642_write_cmos_sensor(0x8037, 0x08);
    OV5642_write_cmos_sensor(0x8038, 0x8e);
    OV5642_write_cmos_sensor(0x8039, 0x8f);
    OV5642_write_cmos_sensor(0x803a, 0x55);
    OV5642_write_cmos_sensor(0x803b, 0x8e);
    OV5642_write_cmos_sensor(0x803c, 0x54);
    OV5642_write_cmos_sensor(0x803d, 0x8d);
    OV5642_write_cmos_sensor(0x803e, 0x53);
    OV5642_write_cmos_sensor(0x803f, 0x8c);
    OV5642_write_cmos_sensor(0x8040, 0x52);
    OV5642_write_cmos_sensor(0x8041, 0xaf);
    OV5642_write_cmos_sensor(0x8042, 0x55);
    OV5642_write_cmos_sensor(0x8043, 0xae);
    OV5642_write_cmos_sensor(0x8044, 0x54);
    OV5642_write_cmos_sensor(0x8045, 0xad);
    OV5642_write_cmos_sensor(0x8046, 0x53);
    OV5642_write_cmos_sensor(0x8047, 0xac);
    OV5642_write_cmos_sensor(0x8048, 0x52);
    OV5642_write_cmos_sensor(0x8049, 0x8f);
    OV5642_write_cmos_sensor(0x804a, 0x2b);
    OV5642_write_cmos_sensor(0x804b, 0x8e);
    OV5642_write_cmos_sensor(0x804c, 0x2a);
    OV5642_write_cmos_sensor(0x804d, 0x8d);
    OV5642_write_cmos_sensor(0x804e, 0x29);
    OV5642_write_cmos_sensor(0x804f, 0x8c);
    OV5642_write_cmos_sensor(0x8050, 0x28);
    OV5642_write_cmos_sensor(0x8051, 0xae);
    OV5642_write_cmos_sensor(0x8052, 0x6c);
    OV5642_write_cmos_sensor(0x8053, 0xe4);
    OV5642_write_cmos_sensor(0x8054, 0x85);
    OV5642_write_cmos_sensor(0x8055, 0x6d);
    OV5642_write_cmos_sensor(0x8056, 0x55);
    OV5642_write_cmos_sensor(0x8057, 0x8e);
    OV5642_write_cmos_sensor(0x8058, 0x54);
    OV5642_write_cmos_sensor(0x8059, 0xf5);
    OV5642_write_cmos_sensor(0x805a, 0x53);
    OV5642_write_cmos_sensor(0x805b, 0xf5);
    OV5642_write_cmos_sensor(0x805c, 0x52);
    OV5642_write_cmos_sensor(0x805d, 0xab);
    OV5642_write_cmos_sensor(0x805e, 0x55);
    OV5642_write_cmos_sensor(0x805f, 0xaa);
    OV5642_write_cmos_sensor(0x8060, 0x54);
    OV5642_write_cmos_sensor(0x8061, 0xa9);
    OV5642_write_cmos_sensor(0x8062, 0x53);
    OV5642_write_cmos_sensor(0x8063, 0xa8);
    OV5642_write_cmos_sensor(0x8064, 0x52);
    OV5642_write_cmos_sensor(0x8065, 0xaf);
    OV5642_write_cmos_sensor(0x8066, 0x2d);
    OV5642_write_cmos_sensor(0x8067, 0xfc);
    OV5642_write_cmos_sensor(0x8068, 0xfd);
    OV5642_write_cmos_sensor(0x8069, 0xfe);
    OV5642_write_cmos_sensor(0x806a, 0x12);
    OV5642_write_cmos_sensor(0x806b, 0x08);
    OV5642_write_cmos_sensor(0x806c, 0x8e);
    OV5642_write_cmos_sensor(0x806d, 0x8f);
    OV5642_write_cmos_sensor(0x806e, 0x55);
    OV5642_write_cmos_sensor(0x806f, 0x8e);
    OV5642_write_cmos_sensor(0x8070, 0x54);
    OV5642_write_cmos_sensor(0x8071, 0x8d);
    OV5642_write_cmos_sensor(0x8072, 0x53);
    OV5642_write_cmos_sensor(0x8073, 0x8c);
    OV5642_write_cmos_sensor(0x8074, 0x52);
    OV5642_write_cmos_sensor(0x8075, 0xe5);
    OV5642_write_cmos_sensor(0x8076, 0x2b);
    OV5642_write_cmos_sensor(0x8077, 0x25);
    OV5642_write_cmos_sensor(0x8078, 0x55);
    OV5642_write_cmos_sensor(0x8079, 0xf5);
    OV5642_write_cmos_sensor(0x807a, 0x2b);
    OV5642_write_cmos_sensor(0x807b, 0xe5);
    OV5642_write_cmos_sensor(0x807c, 0x2a);
    OV5642_write_cmos_sensor(0x807d, 0x35);
    OV5642_write_cmos_sensor(0x807e, 0x54);
    OV5642_write_cmos_sensor(0x807f, 0xf5);
    OV5642_write_cmos_sensor(0x8080, 0x2a);
    OV5642_write_cmos_sensor(0x8081, 0xe5);
    OV5642_write_cmos_sensor(0x8082, 0x29);
    OV5642_write_cmos_sensor(0x8083, 0x35);
    OV5642_write_cmos_sensor(0x8084, 0x53);
    OV5642_write_cmos_sensor(0x8085, 0xf5);
    OV5642_write_cmos_sensor(0x8086, 0x29);
    OV5642_write_cmos_sensor(0x8087, 0xe5);
    OV5642_write_cmos_sensor(0x8088, 0x28);
    OV5642_write_cmos_sensor(0x8089, 0x35);
    OV5642_write_cmos_sensor(0x808a, 0x52);
    OV5642_write_cmos_sensor(0x808b, 0xf5);
    OV5642_write_cmos_sensor(0x808c, 0x28);
    OV5642_write_cmos_sensor(0x808d, 0xae);
    OV5642_write_cmos_sensor(0x808e, 0x6e);
    OV5642_write_cmos_sensor(0x808f, 0xe4);
    OV5642_write_cmos_sensor(0x8090, 0x85);
    OV5642_write_cmos_sensor(0x8091, 0x6f);
    OV5642_write_cmos_sensor(0x8092, 0x55);
    OV5642_write_cmos_sensor(0x8093, 0x8e);
    OV5642_write_cmos_sensor(0x8094, 0x54);
    OV5642_write_cmos_sensor(0x8095, 0xf5);
    OV5642_write_cmos_sensor(0x8096, 0x53);
    OV5642_write_cmos_sensor(0x8097, 0xf5);
    OV5642_write_cmos_sensor(0x8098, 0x52);
    OV5642_write_cmos_sensor(0x8099, 0xab);
    OV5642_write_cmos_sensor(0x809a, 0x55);
    OV5642_write_cmos_sensor(0x809b, 0xaa);
    OV5642_write_cmos_sensor(0x809c, 0x54);
    OV5642_write_cmos_sensor(0x809d, 0xa9);
    OV5642_write_cmos_sensor(0x809e, 0x53);
    OV5642_write_cmos_sensor(0x809f, 0xa8);
    OV5642_write_cmos_sensor(0x80a0, 0x52);
    OV5642_write_cmos_sensor(0x80a1, 0xaf);
    OV5642_write_cmos_sensor(0x80a2, 0x2e);
    OV5642_write_cmos_sensor(0x80a3, 0xfc);
    OV5642_write_cmos_sensor(0x80a4, 0xfd);
    OV5642_write_cmos_sensor(0x80a5, 0xfe);
    OV5642_write_cmos_sensor(0x80a6, 0x12);
    OV5642_write_cmos_sensor(0x80a7, 0x08);
    OV5642_write_cmos_sensor(0x80a8, 0x8e);
    OV5642_write_cmos_sensor(0x80a9, 0x8f);
    OV5642_write_cmos_sensor(0x80aa, 0x55);
    OV5642_write_cmos_sensor(0x80ab, 0x8e);
    OV5642_write_cmos_sensor(0x80ac, 0x54);
    OV5642_write_cmos_sensor(0x80ad, 0x8d);
    OV5642_write_cmos_sensor(0x80ae, 0x53);
    OV5642_write_cmos_sensor(0x80af, 0x8c);
    OV5642_write_cmos_sensor(0x80b0, 0x52);
    OV5642_write_cmos_sensor(0x80b1, 0xe5);
    OV5642_write_cmos_sensor(0x80b2, 0x2b);
    OV5642_write_cmos_sensor(0x80b3, 0x25);
    OV5642_write_cmos_sensor(0x80b4, 0x55);
    OV5642_write_cmos_sensor(0x80b5, 0xf5);
    OV5642_write_cmos_sensor(0x80b6, 0x2b);
    OV5642_write_cmos_sensor(0x80b7, 0xe5);
    OV5642_write_cmos_sensor(0x80b8, 0x2a);
    OV5642_write_cmos_sensor(0x80b9, 0x35);
    OV5642_write_cmos_sensor(0x80ba, 0x54);
    OV5642_write_cmos_sensor(0x80bb, 0xf5);
    OV5642_write_cmos_sensor(0x80bc, 0x2a);
    OV5642_write_cmos_sensor(0x80bd, 0xe5);
    OV5642_write_cmos_sensor(0x80be, 0x29);
    OV5642_write_cmos_sensor(0x80bf, 0x35);
    OV5642_write_cmos_sensor(0x80c0, 0x53);
    OV5642_write_cmos_sensor(0x80c1, 0xf5);
    OV5642_write_cmos_sensor(0x80c2, 0x29);
    OV5642_write_cmos_sensor(0x80c3, 0xe5);
    OV5642_write_cmos_sensor(0x80c4, 0x28);
    OV5642_write_cmos_sensor(0x80c5, 0x35);
    OV5642_write_cmos_sensor(0x80c6, 0x52);
    OV5642_write_cmos_sensor(0x80c7, 0xf5);
    OV5642_write_cmos_sensor(0x80c8, 0x28);
    OV5642_write_cmos_sensor(0x80c9, 0xae);
    OV5642_write_cmos_sensor(0x80ca, 0x70);
    OV5642_write_cmos_sensor(0x80cb, 0xe4);
    OV5642_write_cmos_sensor(0x80cc, 0x85);
    OV5642_write_cmos_sensor(0x80cd, 0x71);
    OV5642_write_cmos_sensor(0x80ce, 0x55);
    OV5642_write_cmos_sensor(0x80cf, 0x8e);
    OV5642_write_cmos_sensor(0x80d0, 0x54);
    OV5642_write_cmos_sensor(0x80d1, 0xf5);
    OV5642_write_cmos_sensor(0x80d2, 0x53);
    OV5642_write_cmos_sensor(0x80d3, 0xf5);
    OV5642_write_cmos_sensor(0x80d4, 0x52);
    OV5642_write_cmos_sensor(0x80d5, 0xab);
    OV5642_write_cmos_sensor(0x80d6, 0x55);
    OV5642_write_cmos_sensor(0x80d7, 0xaa);
    OV5642_write_cmos_sensor(0x80d8, 0x54);
    OV5642_write_cmos_sensor(0x80d9, 0xa9);
    OV5642_write_cmos_sensor(0x80da, 0x53);
    OV5642_write_cmos_sensor(0x80db, 0xa8);
    OV5642_write_cmos_sensor(0x80dc, 0x52);
    OV5642_write_cmos_sensor(0x80dd, 0xaf);
    OV5642_write_cmos_sensor(0x80de, 0x2f);
    OV5642_write_cmos_sensor(0x80df, 0xfc);
    OV5642_write_cmos_sensor(0x80e0, 0xfd);
    OV5642_write_cmos_sensor(0x80e1, 0xfe);
    OV5642_write_cmos_sensor(0x80e2, 0x12);
    OV5642_write_cmos_sensor(0x80e3, 0x08);
    OV5642_write_cmos_sensor(0x80e4, 0x8e);
    OV5642_write_cmos_sensor(0x80e5, 0x8f);
    OV5642_write_cmos_sensor(0x80e6, 0x55);
    OV5642_write_cmos_sensor(0x80e7, 0x8e);
    OV5642_write_cmos_sensor(0x80e8, 0x54);
    OV5642_write_cmos_sensor(0x80e9, 0x8d);
    OV5642_write_cmos_sensor(0x80ea, 0x53);
    OV5642_write_cmos_sensor(0x80eb, 0x8c);
    OV5642_write_cmos_sensor(0x80ec, 0x52);
    OV5642_write_cmos_sensor(0x80ed, 0xe5);
    OV5642_write_cmos_sensor(0x80ee, 0x2b);
    OV5642_write_cmos_sensor(0x80ef, 0x25);
    OV5642_write_cmos_sensor(0x80f0, 0x55);
    OV5642_write_cmos_sensor(0x80f1, 0xf5);
    OV5642_write_cmos_sensor(0x80f2, 0x2b);
    OV5642_write_cmos_sensor(0x80f3, 0xe5);
    OV5642_write_cmos_sensor(0x80f4, 0x2a);
    OV5642_write_cmos_sensor(0x80f5, 0x35);
    OV5642_write_cmos_sensor(0x80f6, 0x54);
    OV5642_write_cmos_sensor(0x80f7, 0xf5);
    OV5642_write_cmos_sensor(0x80f8, 0x2a);
    OV5642_write_cmos_sensor(0x80f9, 0xe5);
    OV5642_write_cmos_sensor(0x80fa, 0x29);
    OV5642_write_cmos_sensor(0x80fb, 0x35);
    OV5642_write_cmos_sensor(0x80fc, 0x53);
    OV5642_write_cmos_sensor(0x80fd, 0xf5);
    OV5642_write_cmos_sensor(0x80fe, 0x29);
    OV5642_write_cmos_sensor(0x80ff, 0xe5);
    OV5642_write_cmos_sensor(0x8100, 0x28);
    OV5642_write_cmos_sensor(0x8101, 0x35);
    OV5642_write_cmos_sensor(0x8102, 0x52);
    OV5642_write_cmos_sensor(0x8103, 0xf5);
    OV5642_write_cmos_sensor(0x8104, 0x28);
    OV5642_write_cmos_sensor(0x8105, 0xae);
    OV5642_write_cmos_sensor(0x8106, 0x72);
    OV5642_write_cmos_sensor(0x8107, 0xe4);
    OV5642_write_cmos_sensor(0x8108, 0x85);
    OV5642_write_cmos_sensor(0x8109, 0x73);
    OV5642_write_cmos_sensor(0x810a, 0x55);
    OV5642_write_cmos_sensor(0x810b, 0x8e);
    OV5642_write_cmos_sensor(0x810c, 0x54);
    OV5642_write_cmos_sensor(0x810d, 0xf5);
    OV5642_write_cmos_sensor(0x810e, 0x53);
    OV5642_write_cmos_sensor(0x810f, 0xf5);
    OV5642_write_cmos_sensor(0x8110, 0x52);
    OV5642_write_cmos_sensor(0x8111, 0xab);
    OV5642_write_cmos_sensor(0x8112, 0x55);
    OV5642_write_cmos_sensor(0x8113, 0xaa);
    OV5642_write_cmos_sensor(0x8114, 0x54);
    OV5642_write_cmos_sensor(0x8115, 0xa9);
    OV5642_write_cmos_sensor(0x8116, 0x53);
    OV5642_write_cmos_sensor(0x8117, 0xa8);
    OV5642_write_cmos_sensor(0x8118, 0x52);
    OV5642_write_cmos_sensor(0x8119, 0xaf);
    OV5642_write_cmos_sensor(0x811a, 0x30);
    OV5642_write_cmos_sensor(0x811b, 0xfc);
    OV5642_write_cmos_sensor(0x811c, 0xfd);
    OV5642_write_cmos_sensor(0x811d, 0xfe);
    OV5642_write_cmos_sensor(0x811e, 0x12);
    OV5642_write_cmos_sensor(0x811f, 0x08);
    OV5642_write_cmos_sensor(0x8120, 0x8e);
    OV5642_write_cmos_sensor(0x8121, 0x8f);
    OV5642_write_cmos_sensor(0x8122, 0x55);
    OV5642_write_cmos_sensor(0x8123, 0x8e);
    OV5642_write_cmos_sensor(0x8124, 0x54);
    OV5642_write_cmos_sensor(0x8125, 0x8d);
    OV5642_write_cmos_sensor(0x8126, 0x53);
    OV5642_write_cmos_sensor(0x8127, 0x8c);
    OV5642_write_cmos_sensor(0x8128, 0x52);
    OV5642_write_cmos_sensor(0x8129, 0xe5);
    OV5642_write_cmos_sensor(0x812a, 0x2b);
    OV5642_write_cmos_sensor(0x812b, 0x25);
    OV5642_write_cmos_sensor(0x812c, 0x55);
    OV5642_write_cmos_sensor(0x812d, 0xf5);
    OV5642_write_cmos_sensor(0x812e, 0x2b);
    OV5642_write_cmos_sensor(0x812f, 0xe5);
    OV5642_write_cmos_sensor(0x8130, 0x2a);
    OV5642_write_cmos_sensor(0x8131, 0x35);
    OV5642_write_cmos_sensor(0x8132, 0x54);
    OV5642_write_cmos_sensor(0x8133, 0xf5);
    OV5642_write_cmos_sensor(0x8134, 0x2a);
    OV5642_write_cmos_sensor(0x8135, 0xe5);
    OV5642_write_cmos_sensor(0x8136, 0x29);
    OV5642_write_cmos_sensor(0x8137, 0x35);
    OV5642_write_cmos_sensor(0x8138, 0x53);
    OV5642_write_cmos_sensor(0x8139, 0xf5);
    OV5642_write_cmos_sensor(0x813a, 0x29);
    OV5642_write_cmos_sensor(0x813b, 0xe5);
    OV5642_write_cmos_sensor(0x813c, 0x28);
    OV5642_write_cmos_sensor(0x813d, 0x35);
    OV5642_write_cmos_sensor(0x813e, 0x52);
    OV5642_write_cmos_sensor(0x813f, 0xf5);
    OV5642_write_cmos_sensor(0x8140, 0x28);
    OV5642_write_cmos_sensor(0x8141, 0x22);
    OV5642_write_cmos_sensor(0x8142, 0xab);
    OV5642_write_cmos_sensor(0x8143, 0x0d);
    OV5642_write_cmos_sensor(0x8144, 0xaa);
    OV5642_write_cmos_sensor(0x8145, 0x0c);
    OV5642_write_cmos_sensor(0x8146, 0xa9);
    OV5642_write_cmos_sensor(0x8147, 0x0b);
    OV5642_write_cmos_sensor(0x8148, 0xa8);
    OV5642_write_cmos_sensor(0x8149, 0x0a);
    OV5642_write_cmos_sensor(0x814a, 0xfc);
    OV5642_write_cmos_sensor(0x814b, 0xfd);
    OV5642_write_cmos_sensor(0x814c, 0xfe);
    OV5642_write_cmos_sensor(0x814d, 0x12);
    OV5642_write_cmos_sensor(0x814e, 0x08);
    OV5642_write_cmos_sensor(0x814f, 0x8e);
    OV5642_write_cmos_sensor(0x8150, 0x8f);
    OV5642_write_cmos_sensor(0x8151, 0x0d);
    OV5642_write_cmos_sensor(0x8152, 0x8e);
    OV5642_write_cmos_sensor(0x8153, 0x0c);
    OV5642_write_cmos_sensor(0x8154, 0x8d);
    OV5642_write_cmos_sensor(0x8155, 0x0b);
    OV5642_write_cmos_sensor(0x8156, 0x8c);
    OV5642_write_cmos_sensor(0x8157, 0x0a);
    OV5642_write_cmos_sensor(0x8158, 0x7b);
    OV5642_write_cmos_sensor(0x8159, 0x40);
    OV5642_write_cmos_sensor(0x815a, 0xe4);
    OV5642_write_cmos_sensor(0x815b, 0xfa);
    OV5642_write_cmos_sensor(0x815c, 0xf9);
    OV5642_write_cmos_sensor(0x815d, 0xf8);
    OV5642_write_cmos_sensor(0x815e, 0x12);
    OV5642_write_cmos_sensor(0x815f, 0x09);
    OV5642_write_cmos_sensor(0x8160, 0x19);
    OV5642_write_cmos_sensor(0x8161, 0x8f);
    OV5642_write_cmos_sensor(0x8162, 0x0d);
    OV5642_write_cmos_sensor(0x8163, 0x8e);
    OV5642_write_cmos_sensor(0x8164, 0x0c);
    OV5642_write_cmos_sensor(0x8165, 0x8d);
    OV5642_write_cmos_sensor(0x8166, 0x0b);
    OV5642_write_cmos_sensor(0x8167, 0x8c);
    OV5642_write_cmos_sensor(0x8168, 0x0a);
    OV5642_write_cmos_sensor(0x8169, 0x22);
    OV5642_write_cmos_sensor(0x816a, 0xd2);
    OV5642_write_cmos_sensor(0x816b, 0x29);
    OV5642_write_cmos_sensor(0x816c, 0x90);
    OV5642_write_cmos_sensor(0x816d, 0x30);
    OV5642_write_cmos_sensor(0x816e, 0x1b);
    OV5642_write_cmos_sensor(0x816f, 0xe5);
    OV5642_write_cmos_sensor(0x8170, 0x25);
    OV5642_write_cmos_sensor(0x8171, 0xf0);
    OV5642_write_cmos_sensor(0x8172, 0x22);
    OV5642_write_cmos_sensor(0x8173, 0x85);
    OV5642_write_cmos_sensor(0x8174, 0x49);
    OV5642_write_cmos_sensor(0x8175, 0x82);
    OV5642_write_cmos_sensor(0x8176, 0x85);
    OV5642_write_cmos_sensor(0x8177, 0x48);
    OV5642_write_cmos_sensor(0x8178, 0x83);
    OV5642_write_cmos_sensor(0x8179, 0xe5);
    OV5642_write_cmos_sensor(0x817a, 0x7c);
    OV5642_write_cmos_sensor(0x817b, 0x75);
    OV5642_write_cmos_sensor(0x817c, 0xf0);
    OV5642_write_cmos_sensor(0x817d, 0x02);
    OV5642_write_cmos_sensor(0x817e, 0x12);
    OV5642_write_cmos_sensor(0x817f, 0x09);
    OV5642_write_cmos_sensor(0x8180, 0xe1);
    OV5642_write_cmos_sensor(0x8181, 0xe4);
    OV5642_write_cmos_sensor(0x8182, 0x93);
    OV5642_write_cmos_sensor(0x8183, 0xfe);
    OV5642_write_cmos_sensor(0x8184, 0x74);
    OV5642_write_cmos_sensor(0x8185, 0x01);
    OV5642_write_cmos_sensor(0x8186, 0x93);
    OV5642_write_cmos_sensor(0x8187, 0xff);
    OV5642_write_cmos_sensor(0x8188, 0x85);
    OV5642_write_cmos_sensor(0x8189, 0x49);
    OV5642_write_cmos_sensor(0x818a, 0x82);
    OV5642_write_cmos_sensor(0x818b, 0x85);
    OV5642_write_cmos_sensor(0x818c, 0x48);
    OV5642_write_cmos_sensor(0x818d, 0x83);
    OV5642_write_cmos_sensor(0x818e, 0xe4);
    OV5642_write_cmos_sensor(0x818f, 0x93);
    OV5642_write_cmos_sensor(0x8190, 0xfc);
    OV5642_write_cmos_sensor(0x8191, 0x74);
    OV5642_write_cmos_sensor(0x8192, 0x01);
    OV5642_write_cmos_sensor(0x8193, 0x93);
    OV5642_write_cmos_sensor(0x8194, 0xfd);
    OV5642_write_cmos_sensor(0x8195, 0xc3);
    OV5642_write_cmos_sensor(0x8196, 0xef);
    OV5642_write_cmos_sensor(0x8197, 0x9d);
    OV5642_write_cmos_sensor(0x8198, 0xff);
    OV5642_write_cmos_sensor(0x8199, 0xee);
    OV5642_write_cmos_sensor(0x819a, 0x9c);
    OV5642_write_cmos_sensor(0x819b, 0x22);
    OV5642_write_cmos_sensor(0x819c, 0xfe);
    OV5642_write_cmos_sensor(0x819d, 0xe4);
    OV5642_write_cmos_sensor(0x819e, 0xfc);
    OV5642_write_cmos_sensor(0x819f, 0xfd);
    OV5642_write_cmos_sensor(0x81a0, 0xe5);
    OV5642_write_cmos_sensor(0x81a1, 0x3f);
    OV5642_write_cmos_sensor(0x81a2, 0x2f);
    OV5642_write_cmos_sensor(0x81a3, 0xf5);
    OV5642_write_cmos_sensor(0x81a4, 0x3f);
    OV5642_write_cmos_sensor(0x81a5, 0xe5);
    OV5642_write_cmos_sensor(0x81a6, 0x3e);
    OV5642_write_cmos_sensor(0x81a7, 0x3e);
    OV5642_write_cmos_sensor(0x81a8, 0xf5);
    OV5642_write_cmos_sensor(0x81a9, 0x3e);
    OV5642_write_cmos_sensor(0x81aa, 0xed);
    OV5642_write_cmos_sensor(0x81ab, 0x35);
    OV5642_write_cmos_sensor(0x81ac, 0x3d);
    OV5642_write_cmos_sensor(0x81ad, 0xf5);
    OV5642_write_cmos_sensor(0x81ae, 0x3d);
    OV5642_write_cmos_sensor(0x81af, 0xec);
    OV5642_write_cmos_sensor(0x81b0, 0x35);
    OV5642_write_cmos_sensor(0x81b1, 0x3c);
    OV5642_write_cmos_sensor(0x81b2, 0xf5);
    OV5642_write_cmos_sensor(0x81b3, 0x3c);
    OV5642_write_cmos_sensor(0x81b4, 0xaf);
    OV5642_write_cmos_sensor(0x81b5, 0x3f);
    OV5642_write_cmos_sensor(0x81b6, 0xae);
    OV5642_write_cmos_sensor(0x81b7, 0x3e);
    OV5642_write_cmos_sensor(0x81b8, 0xfc);
    OV5642_write_cmos_sensor(0x81b9, 0xad);
    OV5642_write_cmos_sensor(0x81ba, 0x3d);
    OV5642_write_cmos_sensor(0x81bb, 0x78);
    OV5642_write_cmos_sensor(0x81bc, 0x08);
    OV5642_write_cmos_sensor(0x81bd, 0x12);
    OV5642_write_cmos_sensor(0x81be, 0x09);
    OV5642_write_cmos_sensor(0x81bf, 0xbe);
    OV5642_write_cmos_sensor(0x81c0, 0x8f);
    OV5642_write_cmos_sensor(0x81c1, 0x3f);
    OV5642_write_cmos_sensor(0x81c2, 0x8e);
    OV5642_write_cmos_sensor(0x81c3, 0x3e);
    OV5642_write_cmos_sensor(0x81c4, 0x8d);
    OV5642_write_cmos_sensor(0x81c5, 0x3d);
    OV5642_write_cmos_sensor(0x81c6, 0x8c);
    OV5642_write_cmos_sensor(0x81c7, 0x3c);
    OV5642_write_cmos_sensor(0x81c8, 0x22);
    OV5642_write_cmos_sensor(0x81c9, 0xe5);
    OV5642_write_cmos_sensor(0x81ca, 0x0b);
    OV5642_write_cmos_sensor(0x81cb, 0x24);
    OV5642_write_cmos_sensor(0x81cc, 0x01);
    OV5642_write_cmos_sensor(0x81cd, 0xff);
    OV5642_write_cmos_sensor(0x81ce, 0xe4);
    OV5642_write_cmos_sensor(0x81cf, 0x33);
    OV5642_write_cmos_sensor(0x81d0, 0xfe);
    OV5642_write_cmos_sensor(0x81d1, 0x22);
    OV5642_write_cmos_sensor(0x81d2, 0xaf);
    OV5642_write_cmos_sensor(0x81d3, 0x2b);
    OV5642_write_cmos_sensor(0x81d4, 0xae);
    OV5642_write_cmos_sensor(0x81d5, 0x2a);
    OV5642_write_cmos_sensor(0x81d6, 0xad);
    OV5642_write_cmos_sensor(0x81d7, 0x29);
    OV5642_write_cmos_sensor(0x81d8, 0xac);
    OV5642_write_cmos_sensor(0x81d9, 0x28);
    OV5642_write_cmos_sensor(0x81da, 0x78);
    OV5642_write_cmos_sensor(0x81db, 0x06);
    OV5642_write_cmos_sensor(0x81dc, 0x12);
    OV5642_write_cmos_sensor(0x81dd, 0x09);
    OV5642_write_cmos_sensor(0x81de, 0xab);
    OV5642_write_cmos_sensor(0x81df, 0x8f);
    OV5642_write_cmos_sensor(0x81e0, 0x2b);
    OV5642_write_cmos_sensor(0x81e1, 0x8e);
    OV5642_write_cmos_sensor(0x81e2, 0x2a);
    OV5642_write_cmos_sensor(0x81e3, 0x8d);
    OV5642_write_cmos_sensor(0x81e4, 0x29);
    OV5642_write_cmos_sensor(0x81e5, 0x8c);
    OV5642_write_cmos_sensor(0x81e6, 0x28);
    OV5642_write_cmos_sensor(0x81e7, 0xd3);
    OV5642_write_cmos_sensor(0x81e8, 0xe5);
    OV5642_write_cmos_sensor(0x81e9, 0x29);
    OV5642_write_cmos_sensor(0x81ea, 0x94);
    OV5642_write_cmos_sensor(0x81eb, 0x00);
    OV5642_write_cmos_sensor(0x81ec, 0xe5);
    OV5642_write_cmos_sensor(0x81ed, 0x28);
    OV5642_write_cmos_sensor(0x81ee, 0x94);
    OV5642_write_cmos_sensor(0x81ef, 0x00);
    OV5642_write_cmos_sensor(0x81f0, 0x22);
    OV5642_write_cmos_sensor(0x81f1, 0x12);
    OV5642_write_cmos_sensor(0x81f2, 0x08);
    OV5642_write_cmos_sensor(0x81f3, 0x8e);
    OV5642_write_cmos_sensor(0x81f4, 0x8f);
    OV5642_write_cmos_sensor(0x81f5, 0x69);
    OV5642_write_cmos_sensor(0x81f6, 0x8e);
    OV5642_write_cmos_sensor(0x81f7, 0x68);
    OV5642_write_cmos_sensor(0x81f8, 0x8d);
    OV5642_write_cmos_sensor(0x81f9, 0x67);
    OV5642_write_cmos_sensor(0x81fa, 0x8c);
    OV5642_write_cmos_sensor(0x81fb, 0x66);
    OV5642_write_cmos_sensor(0x81fc, 0xaf);
    OV5642_write_cmos_sensor(0x81fd, 0x69);
    OV5642_write_cmos_sensor(0x81fe, 0xae);
    OV5642_write_cmos_sensor(0x81ff, 0x68);
    OV5642_write_cmos_sensor(0x8200, 0xad);
    OV5642_write_cmos_sensor(0x8201, 0x67);
    OV5642_write_cmos_sensor(0x8202, 0xac);
    OV5642_write_cmos_sensor(0x8203, 0x66);
    OV5642_write_cmos_sensor(0x8204, 0x22);
    OV5642_write_cmos_sensor(0x8205, 0xe0);
    OV5642_write_cmos_sensor(0x8206, 0x44);
    OV5642_write_cmos_sensor(0x8207, 0x01);
    OV5642_write_cmos_sensor(0x8208, 0xf0);
    OV5642_write_cmos_sensor(0x8209, 0xe0);
    OV5642_write_cmos_sensor(0x820a, 0x44);
    OV5642_write_cmos_sensor(0x820b, 0x02);
    OV5642_write_cmos_sensor(0x820c, 0xf0);
    OV5642_write_cmos_sensor(0x820d, 0xe0);
    OV5642_write_cmos_sensor(0x820e, 0x44);
    OV5642_write_cmos_sensor(0x820f, 0x04);
    OV5642_write_cmos_sensor(0x8210, 0xf0);
    OV5642_write_cmos_sensor(0x8211, 0x22);
    OV5642_write_cmos_sensor(0x8212, 0xd2);
    OV5642_write_cmos_sensor(0x8213, 0x09);
    OV5642_write_cmos_sensor(0x8214, 0x90);
    OV5642_write_cmos_sensor(0x8215, 0x30);
    OV5642_write_cmos_sensor(0x8216, 0x18);
    OV5642_write_cmos_sensor(0x8217, 0xe5);
    OV5642_write_cmos_sensor(0x8218, 0x21);
    OV5642_write_cmos_sensor(0x8219, 0xf0);
    OV5642_write_cmos_sensor(0x821a, 0x22);
    OV5642_write_cmos_sensor(0x821b, 0xe4);
    OV5642_write_cmos_sensor(0x821c, 0x85);
    OV5642_write_cmos_sensor(0x821d, 0x11);
    OV5642_write_cmos_sensor(0x821e, 0x0d);
    OV5642_write_cmos_sensor(0x821f, 0x85);
    OV5642_write_cmos_sensor(0x8220, 0x10);
    OV5642_write_cmos_sensor(0x8221, 0x0c);
    OV5642_write_cmos_sensor(0x8222, 0xf5);
    OV5642_write_cmos_sensor(0x8223, 0x0b);
    OV5642_write_cmos_sensor(0x8224, 0xf5);
    OV5642_write_cmos_sensor(0x8225, 0x0a);
    OV5642_write_cmos_sensor(0x8226, 0xab);
    OV5642_write_cmos_sensor(0x8227, 0x0d);
    OV5642_write_cmos_sensor(0x8228, 0xaa);
    OV5642_write_cmos_sensor(0x8229, 0x0c);
    OV5642_write_cmos_sensor(0x822a, 0xa9);
    OV5642_write_cmos_sensor(0x822b, 0x0b);
    OV5642_write_cmos_sensor(0x822c, 0xa8);
    OV5642_write_cmos_sensor(0x822d, 0x0a);
    OV5642_write_cmos_sensor(0x822e, 0x22);
    OV5642_write_cmos_sensor(0x822f, 0x90);
    OV5642_write_cmos_sensor(0x8230, 0x30);
    OV5642_write_cmos_sensor(0x8231, 0x42);
    OV5642_write_cmos_sensor(0x8232, 0xe0);
    OV5642_write_cmos_sensor(0x8233, 0xf5);
    OV5642_write_cmos_sensor(0x8234, 0x22);
    OV5642_write_cmos_sensor(0x8235, 0x75);
    OV5642_write_cmos_sensor(0x8236, 0x51);
    OV5642_write_cmos_sensor(0x8237, 0x0a);
    OV5642_write_cmos_sensor(0x8238, 0x22);
    OV5642_write_cmos_sensor(0x8239, 0x85);
    OV5642_write_cmos_sensor(0x823a, 0x49);
    OV5642_write_cmos_sensor(0x823b, 0x82);
    OV5642_write_cmos_sensor(0x823c, 0x85);
    OV5642_write_cmos_sensor(0x823d, 0x48);
    OV5642_write_cmos_sensor(0x823e, 0x83);
    OV5642_write_cmos_sensor(0x823f, 0x22);
    OV5642_write_cmos_sensor(0x8240, 0xf5);
    OV5642_write_cmos_sensor(0x8241, 0x82);
    OV5642_write_cmos_sensor(0x8242, 0xe4);
    OV5642_write_cmos_sensor(0x8243, 0x3a);
    OV5642_write_cmos_sensor(0x8244, 0xf5);
    OV5642_write_cmos_sensor(0x8245, 0x83);
    OV5642_write_cmos_sensor(0x8246, 0x02);
    OV5642_write_cmos_sensor(0x8247, 0x09);
    OV5642_write_cmos_sensor(0x8248, 0xd1);
    OV5642_write_cmos_sensor(0x8249, 0x8f);
    OV5642_write_cmos_sensor(0x824a, 0x0a);
    OV5642_write_cmos_sensor(0x824b, 0x74);
    OV5642_write_cmos_sensor(0x824c, 0x4a);
    OV5642_write_cmos_sensor(0x824d, 0x2f);
    OV5642_write_cmos_sensor(0x824e, 0xf8);
    OV5642_write_cmos_sensor(0x824f, 0xe6);
    OV5642_write_cmos_sensor(0x8250, 0x22);
    OV5642_write_cmos_sensor(0x8251, 0xc2);
    OV5642_write_cmos_sensor(0x8252, 0x07);
    OV5642_write_cmos_sensor(0x8253, 0xc2);
    OV5642_write_cmos_sensor(0x8254, 0x06);
    OV5642_write_cmos_sensor(0x8255, 0xc2);
    OV5642_write_cmos_sensor(0x8256, 0x02);
    OV5642_write_cmos_sensor(0x8257, 0xc2);
    OV5642_write_cmos_sensor(0x8258, 0x01);
    OV5642_write_cmos_sensor(0x8259, 0xc2);
    OV5642_write_cmos_sensor(0x825a, 0x00);
    OV5642_write_cmos_sensor(0x825b, 0xc2);
    OV5642_write_cmos_sensor(0x825c, 0x03);
    OV5642_write_cmos_sensor(0x825d, 0xd2);
    OV5642_write_cmos_sensor(0x825e, 0x04);
    OV5642_write_cmos_sensor(0x825f, 0x22);
    OV5642_write_cmos_sensor(0x8260, 0xe5);
    OV5642_write_cmos_sensor(0x8261, 0x16);
    OV5642_write_cmos_sensor(0x8262, 0x25);
    OV5642_write_cmos_sensor(0x8263, 0xe0);
    OV5642_write_cmos_sensor(0x8264, 0x25);
    OV5642_write_cmos_sensor(0x8265, 0xe0);
    OV5642_write_cmos_sensor(0x8266, 0x22);
    OV5642_write_cmos_sensor(0x8267, 0x12);
    OV5642_write_cmos_sensor(0x8268, 0x09);
    OV5642_write_cmos_sensor(0x8269, 0x19);
    OV5642_write_cmos_sensor(0x826a, 0x8f);
    OV5642_write_cmos_sensor(0x826b, 0x69);
    OV5642_write_cmos_sensor(0x826c, 0x8e);
    OV5642_write_cmos_sensor(0x826d, 0x68);
    OV5642_write_cmos_sensor(0x826e, 0x8d);
    OV5642_write_cmos_sensor(0x826f, 0x67);
    OV5642_write_cmos_sensor(0x8270, 0x8c);
    OV5642_write_cmos_sensor(0x8271, 0x66);
    OV5642_write_cmos_sensor(0x8272, 0x22);
    OV5642_write_cmos_sensor(0x8273, 0xe4);
    OV5642_write_cmos_sensor(0x8274, 0x85);
    OV5642_write_cmos_sensor(0x8275, 0x0f);
    OV5642_write_cmos_sensor(0x8276, 0x0d);
    OV5642_write_cmos_sensor(0x8277, 0x85);
    OV5642_write_cmos_sensor(0x8278, 0x0e);
    OV5642_write_cmos_sensor(0x8279, 0x0c);
    OV5642_write_cmos_sensor(0x827a, 0xf5);
    OV5642_write_cmos_sensor(0x827b, 0x0b);
    OV5642_write_cmos_sensor(0x827c, 0xf5);
    OV5642_write_cmos_sensor(0x827d, 0x0a);
    OV5642_write_cmos_sensor(0x827e, 0x22);
    OV5642_write_cmos_sensor(0x827f, 0x90);
    OV5642_write_cmos_sensor(0x8280, 0x10);
    OV5642_write_cmos_sensor(0x8281, 0xa7);
    OV5642_write_cmos_sensor(0x8282, 0xe4);
    OV5642_write_cmos_sensor(0x8283, 0x93);
    OV5642_write_cmos_sensor(0x8284, 0xff);
    OV5642_write_cmos_sensor(0x8285, 0x90);
    OV5642_write_cmos_sensor(0x8286, 0x30);
    OV5642_write_cmos_sensor(0x8287, 0x0a);
    OV5642_write_cmos_sensor(0x8288, 0xe0);
    OV5642_write_cmos_sensor(0x8289, 0x22);
    OV5642_write_cmos_sensor(0x828a, 0xc2);
    OV5642_write_cmos_sensor(0x828b, 0x02);
    OV5642_write_cmos_sensor(0x828c, 0xc2);
    OV5642_write_cmos_sensor(0x828d, 0x01);
    OV5642_write_cmos_sensor(0x828e, 0xd2);
    OV5642_write_cmos_sensor(0x828f, 0x00);
    OV5642_write_cmos_sensor(0x8290, 0xc2);
    OV5642_write_cmos_sensor(0x8291, 0x03);
    OV5642_write_cmos_sensor(0x8292, 0xc2);
    OV5642_write_cmos_sensor(0x8293, 0x04);
    OV5642_write_cmos_sensor(0x8294, 0x22);
    OV5642_write_cmos_sensor(0x8295, 0x75);
    OV5642_write_cmos_sensor(0x8296, 0xf0);
    OV5642_write_cmos_sensor(0x8297, 0x02);
    OV5642_write_cmos_sensor(0x8298, 0x02);
    OV5642_write_cmos_sensor(0x8299, 0x09);
    OV5642_write_cmos_sensor(0x829a, 0xe1);
    OV5642_write_cmos_sensor(0x829b, 0xd2);
    OV5642_write_cmos_sensor(0x829c, 0x02);
    OV5642_write_cmos_sensor(0x829d, 0xd2);
    OV5642_write_cmos_sensor(0x829e, 0x01);
    OV5642_write_cmos_sensor(0x829f, 0xc2);
    OV5642_write_cmos_sensor(0x82a0, 0x00);
    OV5642_write_cmos_sensor(0x82a1, 0x22);
    OV5642_write_cmos_sensor(0x82a2, 0x74);
    OV5642_write_cmos_sensor(0x82a3, 0x4a);
    OV5642_write_cmos_sensor(0x82a4, 0x25);
    OV5642_write_cmos_sensor(0x82a5, 0x0a);
    OV5642_write_cmos_sensor(0x82a6, 0xf8);
    OV5642_write_cmos_sensor(0x82a7, 0xe6);
    OV5642_write_cmos_sensor(0x82a8, 0x22);
    OV5642_write_cmos_sensor(0x82a9, 0xd3);
    OV5642_write_cmos_sensor(0x82aa, 0xe5);
    OV5642_write_cmos_sensor(0x82ab, 0x69);
    OV5642_write_cmos_sensor(0x82ac, 0x94);
    OV5642_write_cmos_sensor(0x82ad, 0xff);
    OV5642_write_cmos_sensor(0x82ae, 0xe5);
    OV5642_write_cmos_sensor(0x82af, 0x68);
    OV5642_write_cmos_sensor(0x82b0, 0x94);
    OV5642_write_cmos_sensor(0x82b1, 0x03);
    OV5642_write_cmos_sensor(0x82b2, 0x22);
    OV5642_write_cmos_sensor(0x82b3, 0x30);
    OV5642_write_cmos_sensor(0x82b4, 0x18);
    OV5642_write_cmos_sensor(0x82b5, 0x4d);
    OV5642_write_cmos_sensor(0x82b6, 0x20);
    OV5642_write_cmos_sensor(0x82b7, 0x19);
    OV5642_write_cmos_sensor(0x82b8, 0x4a);
    OV5642_write_cmos_sensor(0x82b9, 0x75);
    OV5642_write_cmos_sensor(0x82ba, 0x0a);
    OV5642_write_cmos_sensor(0x82bb, 0x02);
    OV5642_write_cmos_sensor(0x82bc, 0x12);
    OV5642_write_cmos_sensor(0x82bd, 0x02);
    OV5642_write_cmos_sensor(0x82be, 0xa2);
    OV5642_write_cmos_sensor(0x82bf, 0xff);
    OV5642_write_cmos_sensor(0x82c0, 0xe5);
    OV5642_write_cmos_sensor(0x82c1, 0x4a);
    OV5642_write_cmos_sensor(0x82c2, 0xd3);
    OV5642_write_cmos_sensor(0x82c3, 0x9f);
    OV5642_write_cmos_sensor(0x82c4, 0x40);
    OV5642_write_cmos_sensor(0x82c5, 0x04);
    OV5642_write_cmos_sensor(0x82c6, 0x7f);
    OV5642_write_cmos_sensor(0x82c7, 0x00);
    OV5642_write_cmos_sensor(0x82c8, 0x80);
    OV5642_write_cmos_sensor(0x82c9, 0x02);
    OV5642_write_cmos_sensor(0x82ca, 0xaf);
    OV5642_write_cmos_sensor(0x82cb, 0x0a);
    OV5642_write_cmos_sensor(0x82cc, 0x12);
    OV5642_write_cmos_sensor(0x82cd, 0x02);
    OV5642_write_cmos_sensor(0x82ce, 0x49);
    OV5642_write_cmos_sensor(0x82cf, 0xff);
    OV5642_write_cmos_sensor(0x82d0, 0xe5);
    OV5642_write_cmos_sensor(0x82d1, 0x4b);
    OV5642_write_cmos_sensor(0x82d2, 0xd3);
    OV5642_write_cmos_sensor(0x82d3, 0x9f);
    OV5642_write_cmos_sensor(0x82d4, 0x40);
    OV5642_write_cmos_sensor(0x82d5, 0x04);
    OV5642_write_cmos_sensor(0x82d6, 0x7f);
    OV5642_write_cmos_sensor(0x82d7, 0x01);
    OV5642_write_cmos_sensor(0x82d8, 0x80);
    OV5642_write_cmos_sensor(0x82d9, 0x02);
    OV5642_write_cmos_sensor(0x82da, 0xaf);
    OV5642_write_cmos_sensor(0x82db, 0x0a);
    OV5642_write_cmos_sensor(0x82dc, 0x12);
    OV5642_write_cmos_sensor(0x82dd, 0x02);
    OV5642_write_cmos_sensor(0x82de, 0x49);
    OV5642_write_cmos_sensor(0x82df, 0xff);
    OV5642_write_cmos_sensor(0x82e0, 0xe5);
    OV5642_write_cmos_sensor(0x82e1, 0x4d);
    OV5642_write_cmos_sensor(0x82e2, 0xd3);
    OV5642_write_cmos_sensor(0x82e3, 0x9f);
    OV5642_write_cmos_sensor(0x82e4, 0x40);
    OV5642_write_cmos_sensor(0x82e5, 0x04);
    OV5642_write_cmos_sensor(0x82e6, 0x7f);
    OV5642_write_cmos_sensor(0x82e7, 0x03);
    OV5642_write_cmos_sensor(0x82e8, 0x80);
    OV5642_write_cmos_sensor(0x82e9, 0x02);
    OV5642_write_cmos_sensor(0x82ea, 0xaf);
    OV5642_write_cmos_sensor(0x82eb, 0x0a);
    OV5642_write_cmos_sensor(0x82ec, 0x12);
    OV5642_write_cmos_sensor(0x82ed, 0x02);
    OV5642_write_cmos_sensor(0x82ee, 0x49);
    OV5642_write_cmos_sensor(0x82ef, 0xff);
    OV5642_write_cmos_sensor(0x82f0, 0xe5);
    OV5642_write_cmos_sensor(0x82f1, 0x4e);
    OV5642_write_cmos_sensor(0x82f2, 0xd3);
    OV5642_write_cmos_sensor(0x82f3, 0x9f);
    OV5642_write_cmos_sensor(0x82f4, 0x40);
    OV5642_write_cmos_sensor(0x82f5, 0x04);
    OV5642_write_cmos_sensor(0x82f6, 0x7f);
    OV5642_write_cmos_sensor(0x82f7, 0x04);
    OV5642_write_cmos_sensor(0x82f8, 0x80);
    OV5642_write_cmos_sensor(0x82f9, 0x02);
    OV5642_write_cmos_sensor(0x82fa, 0xaf);
    OV5642_write_cmos_sensor(0x82fb, 0x0a);
    OV5642_write_cmos_sensor(0x82fc, 0x12);
    OV5642_write_cmos_sensor(0x82fd, 0x02);
    OV5642_write_cmos_sensor(0x82fe, 0x49);
    OV5642_write_cmos_sensor(0x82ff, 0xf5);
    OV5642_write_cmos_sensor(0x8300, 0x0b);
    OV5642_write_cmos_sensor(0x8301, 0x80);
    OV5642_write_cmos_sensor(0x8302, 0x06);
    OV5642_write_cmos_sensor(0x8303, 0x85);
    OV5642_write_cmos_sensor(0x8304, 0x79);
    OV5642_write_cmos_sensor(0x8305, 0x0a);
    OV5642_write_cmos_sensor(0x8306, 0x85);
    OV5642_write_cmos_sensor(0x8307, 0x4f);
    OV5642_write_cmos_sensor(0x8308, 0x0b);
    OV5642_write_cmos_sensor(0x8309, 0x7f);
    OV5642_write_cmos_sensor(0x830a, 0x01);
    OV5642_write_cmos_sensor(0x830b, 0xe4);
    OV5642_write_cmos_sensor(0x830c, 0xfe);
    OV5642_write_cmos_sensor(0x830d, 0x12);
    OV5642_write_cmos_sensor(0x830e, 0x02);
    OV5642_write_cmos_sensor(0x830f, 0xa2);
    OV5642_write_cmos_sensor(0x8310, 0xfd);
    OV5642_write_cmos_sensor(0x8311, 0xe5);
    OV5642_write_cmos_sensor(0x8312, 0x0b);
    OV5642_write_cmos_sensor(0x8313, 0xc3);
    OV5642_write_cmos_sensor(0x8314, 0x9d);
    OV5642_write_cmos_sensor(0x8315, 0x50);
    OV5642_write_cmos_sensor(0x8316, 0x04);
    OV5642_write_cmos_sensor(0x8317, 0x7d);
    OV5642_write_cmos_sensor(0x8318, 0x01);
    OV5642_write_cmos_sensor(0x8319, 0x80);
    OV5642_write_cmos_sensor(0x831a, 0x02);
    OV5642_write_cmos_sensor(0x831b, 0x7d);
    OV5642_write_cmos_sensor(0x831c, 0xff);
    OV5642_write_cmos_sensor(0x831d, 0xac);
    OV5642_write_cmos_sensor(0x831e, 0x0b);
    OV5642_write_cmos_sensor(0x831f, 0xe5);
    OV5642_write_cmos_sensor(0x8320, 0x4e);
    OV5642_write_cmos_sensor(0x8321, 0xb5);
    OV5642_write_cmos_sensor(0x8322, 0x0b);
    OV5642_write_cmos_sensor(0x8323, 0x03);
    OV5642_write_cmos_sensor(0x8324, 0xd3);
    OV5642_write_cmos_sensor(0x8325, 0x80);
    OV5642_write_cmos_sensor(0x8326, 0x01);
    OV5642_write_cmos_sensor(0x8327, 0xc3);
    OV5642_write_cmos_sensor(0x8328, 0x92);
    OV5642_write_cmos_sensor(0x8329, 0x1f);
    OV5642_write_cmos_sensor(0x832a, 0xe5);
    OV5642_write_cmos_sensor(0x832b, 0x4d);
    OV5642_write_cmos_sensor(0x832c, 0xb5);
    OV5642_write_cmos_sensor(0x832d, 0x0b);
    OV5642_write_cmos_sensor(0x832e, 0x03);
    OV5642_write_cmos_sensor(0x832f, 0xd3);
    OV5642_write_cmos_sensor(0x8330, 0x80);
    OV5642_write_cmos_sensor(0x8331, 0x01);
    OV5642_write_cmos_sensor(0x8332, 0xc3);
    OV5642_write_cmos_sensor(0x8333, 0x92);
    OV5642_write_cmos_sensor(0x8334, 0x1e);
    OV5642_write_cmos_sensor(0x8335, 0xe5);
    OV5642_write_cmos_sensor(0x8336, 0x4c);
    OV5642_write_cmos_sensor(0x8337, 0xb5);
    OV5642_write_cmos_sensor(0x8338, 0x0b);
    OV5642_write_cmos_sensor(0x8339, 0x03);
    OV5642_write_cmos_sensor(0x833a, 0xd3);
    OV5642_write_cmos_sensor(0x833b, 0x80);
    OV5642_write_cmos_sensor(0x833c, 0x01);
    OV5642_write_cmos_sensor(0x833d, 0xc3);
    OV5642_write_cmos_sensor(0x833e, 0x92);
    OV5642_write_cmos_sensor(0x833f, 0x1d);
    OV5642_write_cmos_sensor(0x8340, 0xe5);
    OV5642_write_cmos_sensor(0x8341, 0x4b);
    OV5642_write_cmos_sensor(0x8342, 0xb5);
    OV5642_write_cmos_sensor(0x8343, 0x0b);
    OV5642_write_cmos_sensor(0x8344, 0x03);
    OV5642_write_cmos_sensor(0x8345, 0xd3);
    OV5642_write_cmos_sensor(0x8346, 0x80);
    OV5642_write_cmos_sensor(0x8347, 0x01);
    OV5642_write_cmos_sensor(0x8348, 0xc3);
    OV5642_write_cmos_sensor(0x8349, 0x92);
    OV5642_write_cmos_sensor(0x834a, 0x1c);
    OV5642_write_cmos_sensor(0x834b, 0xe5);
    OV5642_write_cmos_sensor(0x834c, 0x4a);
    OV5642_write_cmos_sensor(0x834d, 0xb5);
    OV5642_write_cmos_sensor(0x834e, 0x0b);
    OV5642_write_cmos_sensor(0x834f, 0x03);
    OV5642_write_cmos_sensor(0x8350, 0xd3);
    OV5642_write_cmos_sensor(0x8351, 0x80);
    OV5642_write_cmos_sensor(0x8352, 0x01);
    OV5642_write_cmos_sensor(0x8353, 0xc3);
    OV5642_write_cmos_sensor(0x8354, 0x92);
    OV5642_write_cmos_sensor(0x8355, 0x1b);
    OV5642_write_cmos_sensor(0x8356, 0xe5);
    OV5642_write_cmos_sensor(0x8357, 0x30);
    OV5642_write_cmos_sensor(0x8358, 0xd3);
    OV5642_write_cmos_sensor(0x8359, 0x94);
    OV5642_write_cmos_sensor(0x835a, 0x00);
    OV5642_write_cmos_sensor(0x835b, 0x40);
    OV5642_write_cmos_sensor(0x835c, 0x04);
    OV5642_write_cmos_sensor(0x835d, 0xa2);
    OV5642_write_cmos_sensor(0x835e, 0x1f);
    OV5642_write_cmos_sensor(0x835f, 0x80);
    OV5642_write_cmos_sensor(0x8360, 0x01);
    OV5642_write_cmos_sensor(0x8361, 0xc3);
    OV5642_write_cmos_sensor(0x8362, 0x92);
    OV5642_write_cmos_sensor(0x8363, 0x1f);
    OV5642_write_cmos_sensor(0x8364, 0xe5);
    OV5642_write_cmos_sensor(0x8365, 0x2f);
    OV5642_write_cmos_sensor(0x8366, 0xd3);
    OV5642_write_cmos_sensor(0x8367, 0x94);
    OV5642_write_cmos_sensor(0x8368, 0x00);
    OV5642_write_cmos_sensor(0x8369, 0x40);
    OV5642_write_cmos_sensor(0x836a, 0x04);
    OV5642_write_cmos_sensor(0x836b, 0xa2);
    OV5642_write_cmos_sensor(0x836c, 0x1e);
    OV5642_write_cmos_sensor(0x836d, 0x80);
    OV5642_write_cmos_sensor(0x836e, 0x01);
    OV5642_write_cmos_sensor(0x836f, 0xc3);
    OV5642_write_cmos_sensor(0x8370, 0x92);
    OV5642_write_cmos_sensor(0x8371, 0x1e);
    OV5642_write_cmos_sensor(0x8372, 0xe5);
    OV5642_write_cmos_sensor(0x8373, 0x2e);
    OV5642_write_cmos_sensor(0x8374, 0xd3);
    OV5642_write_cmos_sensor(0x8375, 0x94);
    OV5642_write_cmos_sensor(0x8376, 0x00);
    OV5642_write_cmos_sensor(0x8377, 0x40);
    OV5642_write_cmos_sensor(0x8378, 0x04);
    OV5642_write_cmos_sensor(0x8379, 0xa2);
    OV5642_write_cmos_sensor(0x837a, 0x1d);
    OV5642_write_cmos_sensor(0x837b, 0x80);
    OV5642_write_cmos_sensor(0x837c, 0x01);
    OV5642_write_cmos_sensor(0x837d, 0xc3);
    OV5642_write_cmos_sensor(0x837e, 0x92);
    OV5642_write_cmos_sensor(0x837f, 0x1d);
    OV5642_write_cmos_sensor(0x8380, 0xe5);
    OV5642_write_cmos_sensor(0x8381, 0x2d);
    OV5642_write_cmos_sensor(0x8382, 0xd3);
    OV5642_write_cmos_sensor(0x8383, 0x94);
    OV5642_write_cmos_sensor(0x8384, 0x00);
    OV5642_write_cmos_sensor(0x8385, 0x40);
    OV5642_write_cmos_sensor(0x8386, 0x04);
    OV5642_write_cmos_sensor(0x8387, 0xa2);
    OV5642_write_cmos_sensor(0x8388, 0x1c);
    OV5642_write_cmos_sensor(0x8389, 0x80);
    OV5642_write_cmos_sensor(0x838a, 0x01);
    OV5642_write_cmos_sensor(0x838b, 0xc3);
    OV5642_write_cmos_sensor(0x838c, 0x92);
    OV5642_write_cmos_sensor(0x838d, 0x1c);
    OV5642_write_cmos_sensor(0x838e, 0xe5);
    OV5642_write_cmos_sensor(0x838f, 0x2c);
    OV5642_write_cmos_sensor(0x8390, 0xd3);
    OV5642_write_cmos_sensor(0x8391, 0x94);
    OV5642_write_cmos_sensor(0x8392, 0x00);
    OV5642_write_cmos_sensor(0x8393, 0x40);
    OV5642_write_cmos_sensor(0x8394, 0x04);
    OV5642_write_cmos_sensor(0x8395, 0xa2);
    OV5642_write_cmos_sensor(0x8396, 0x1b);
    OV5642_write_cmos_sensor(0x8397, 0x80);
    OV5642_write_cmos_sensor(0x8398, 0x01);
    OV5642_write_cmos_sensor(0x8399, 0xc3);
    OV5642_write_cmos_sensor(0x839a, 0x92);
    OV5642_write_cmos_sensor(0x839b, 0x1b);
    OV5642_write_cmos_sensor(0x839c, 0xe5);
    OV5642_write_cmos_sensor(0x839d, 0x23);
    OV5642_write_cmos_sensor(0x839e, 0x54);
    OV5642_write_cmos_sensor(0x839f, 0xf8);
    OV5642_write_cmos_sensor(0x83a0, 0x70);
    OV5642_write_cmos_sensor(0x83a1, 0x5b);
    OV5642_write_cmos_sensor(0x83a2, 0xbf);
    OV5642_write_cmos_sensor(0x83a3, 0x01);
    OV5642_write_cmos_sensor(0x83a4, 0x08);
    OV5642_write_cmos_sensor(0x83a5, 0xed);
    OV5642_write_cmos_sensor(0x83a6, 0xf4);
    OV5642_write_cmos_sensor(0x83a7, 0x04);
    OV5642_write_cmos_sensor(0x83a8, 0xfd);
    OV5642_write_cmos_sensor(0x83a9, 0x7f);
    OV5642_write_cmos_sensor(0x83aa, 0x02);
    OV5642_write_cmos_sensor(0x83ab, 0x80);
    OV5642_write_cmos_sensor(0x83ac, 0x06);
    OV5642_write_cmos_sensor(0x83ad, 0xbf);
    OV5642_write_cmos_sensor(0x83ae, 0x02);
    OV5642_write_cmos_sensor(0x83af, 0x02);
    OV5642_write_cmos_sensor(0x83b0, 0x7f);
    OV5642_write_cmos_sensor(0x83b1, 0x01);
    OV5642_write_cmos_sensor(0x83b2, 0x0e);
    OV5642_write_cmos_sensor(0x83b3, 0xd3);
    OV5642_write_cmos_sensor(0x83b4, 0xed);
    OV5642_write_cmos_sensor(0x83b5, 0x64);
    OV5642_write_cmos_sensor(0x83b6, 0x80);
    OV5642_write_cmos_sensor(0x83b7, 0x94);
    OV5642_write_cmos_sensor(0x83b8, 0x80);
    OV5642_write_cmos_sensor(0x83b9, 0x40);
    OV5642_write_cmos_sensor(0x83ba, 0x18);
    OV5642_write_cmos_sensor(0x83bb, 0xec);
    OV5642_write_cmos_sensor(0x83bc, 0x2e);
    OV5642_write_cmos_sensor(0x83bd, 0xf5);
    OV5642_write_cmos_sensor(0x83be, 0x0b);
    OV5642_write_cmos_sensor(0x83bf, 0xd3);
    OV5642_write_cmos_sensor(0x83c0, 0x95);
    OV5642_write_cmos_sensor(0x83c1, 0x7c);
    OV5642_write_cmos_sensor(0x83c2, 0x40);
    OV5642_write_cmos_sensor(0x83c3, 0x2c);
    OV5642_write_cmos_sensor(0x83c4, 0x7d);
    OV5642_write_cmos_sensor(0x83c5, 0xff);
    OV5642_write_cmos_sensor(0x83c6, 0xef);
    OV5642_write_cmos_sensor(0x83c7, 0x60);
    OV5642_write_cmos_sensor(0x83c8, 0x04);
    OV5642_write_cmos_sensor(0x83c9, 0x7b);
    OV5642_write_cmos_sensor(0x83ca, 0x00);
    OV5642_write_cmos_sensor(0x83cb, 0x80);
    OV5642_write_cmos_sensor(0x83cc, 0x02);
    OV5642_write_cmos_sensor(0x83cd, 0x7b);
    OV5642_write_cmos_sensor(0x83ce, 0x03);
    OV5642_write_cmos_sensor(0x83cf, 0xaf);
    OV5642_write_cmos_sensor(0x83d0, 0x03);
    OV5642_write_cmos_sensor(0x83d1, 0x80);
    OV5642_write_cmos_sensor(0x83d2, 0x18);
    OV5642_write_cmos_sensor(0x83d3, 0xec);
    OV5642_write_cmos_sensor(0x83d4, 0xc3);
    OV5642_write_cmos_sensor(0x83d5, 0x9e);
    OV5642_write_cmos_sensor(0x83d6, 0x50);
    OV5642_write_cmos_sensor(0x83d7, 0x13);
    OV5642_write_cmos_sensor(0x83d8, 0x7d);
    OV5642_write_cmos_sensor(0x83d9, 0x01);
    OV5642_write_cmos_sensor(0x83da, 0xef);
    OV5642_write_cmos_sensor(0x83db, 0x60);
    OV5642_write_cmos_sensor(0x83dc, 0x04);
    OV5642_write_cmos_sensor(0x83dd, 0x7b);
    OV5642_write_cmos_sensor(0x83de, 0x00);
    OV5642_write_cmos_sensor(0x83df, 0x80);
    OV5642_write_cmos_sensor(0x83e0, 0x02);
    OV5642_write_cmos_sensor(0x83e1, 0x7b);
    OV5642_write_cmos_sensor(0x83e2, 0x03);
    OV5642_write_cmos_sensor(0x83e3, 0xaf);
    OV5642_write_cmos_sensor(0x83e4, 0x03);
    OV5642_write_cmos_sensor(0x83e5, 0xec);
    OV5642_write_cmos_sensor(0x83e6, 0x2e);
    OV5642_write_cmos_sensor(0x83e7, 0xf5);
    OV5642_write_cmos_sensor(0x83e8, 0x0b);
    OV5642_write_cmos_sensor(0x83e9, 0x80);
    OV5642_write_cmos_sensor(0x83ea, 0x05);
    OV5642_write_cmos_sensor(0x83eb, 0xc3);
    OV5642_write_cmos_sensor(0x83ec, 0xec);
    OV5642_write_cmos_sensor(0x83ed, 0x9e);
    OV5642_write_cmos_sensor(0x83ee, 0xf5);
    OV5642_write_cmos_sensor(0x83ef, 0x0b);
    OV5642_write_cmos_sensor(0x83f0, 0xef);
    OV5642_write_cmos_sensor(0x83f1, 0x64);
    OV5642_write_cmos_sensor(0x83f2, 0x03);
    OV5642_write_cmos_sensor(0x83f3, 0x60);
    OV5642_write_cmos_sensor(0x83f4, 0x03);
    OV5642_write_cmos_sensor(0x83f5, 0x02);
    OV5642_write_cmos_sensor(0x83f6, 0x03);
    OV5642_write_cmos_sensor(0x83f7, 0x1f);
    OV5642_write_cmos_sensor(0x83f8, 0x12);
    OV5642_write_cmos_sensor(0x83f9, 0x02);
    OV5642_write_cmos_sensor(0x83fa, 0xa2);
    OV5642_write_cmos_sensor(0x83fb, 0xf5);
    OV5642_write_cmos_sensor(0x83fc, 0x0b);
    OV5642_write_cmos_sensor(0x83fd, 0x12);
    OV5642_write_cmos_sensor(0x83fe, 0x01);
    OV5642_write_cmos_sensor(0x83ff, 0xc9);
    OV5642_write_cmos_sensor(0x8400, 0xe5);
    OV5642_write_cmos_sensor(0x8401, 0x4e);
    OV5642_write_cmos_sensor(0x8402, 0xb5);
    OV5642_write_cmos_sensor(0x8403, 0x07);
    OV5642_write_cmos_sensor(0x8404, 0x07);
    OV5642_write_cmos_sensor(0x8405, 0xe4);
    OV5642_write_cmos_sensor(0x8406, 0xb5);
    OV5642_write_cmos_sensor(0x8407, 0x06);
    OV5642_write_cmos_sensor(0x8408, 0x03);
    OV5642_write_cmos_sensor(0x8409, 0xd3);
    OV5642_write_cmos_sensor(0x840a, 0x80);
    OV5642_write_cmos_sensor(0x840b, 0x02);
    OV5642_write_cmos_sensor(0x840c, 0xa2);
    OV5642_write_cmos_sensor(0x840d, 0x1f);
    OV5642_write_cmos_sensor(0x840e, 0x92);
    OV5642_write_cmos_sensor(0x840f, 0x1f);
    OV5642_write_cmos_sensor(0x8410, 0xe5);
    OV5642_write_cmos_sensor(0x8411, 0x4d);
    OV5642_write_cmos_sensor(0x8412, 0xb5);
    OV5642_write_cmos_sensor(0x8413, 0x07);
    OV5642_write_cmos_sensor(0x8414, 0x07);
    OV5642_write_cmos_sensor(0x8415, 0xe4);
    OV5642_write_cmos_sensor(0x8416, 0xb5);
    OV5642_write_cmos_sensor(0x8417, 0x06);
    OV5642_write_cmos_sensor(0x8418, 0x03);
    OV5642_write_cmos_sensor(0x8419, 0xd3);
    OV5642_write_cmos_sensor(0x841a, 0x80);
    OV5642_write_cmos_sensor(0x841b, 0x02);
    OV5642_write_cmos_sensor(0x841c, 0xa2);
    OV5642_write_cmos_sensor(0x841d, 0x1e);
    OV5642_write_cmos_sensor(0x841e, 0x92);
    OV5642_write_cmos_sensor(0x841f, 0x1e);
    OV5642_write_cmos_sensor(0x8420, 0x12);
    OV5642_write_cmos_sensor(0x8421, 0x01);
    OV5642_write_cmos_sensor(0x8422, 0xc9);
    OV5642_write_cmos_sensor(0x8423, 0xe5);
    OV5642_write_cmos_sensor(0x8424, 0x4c);
    OV5642_write_cmos_sensor(0x8425, 0xb5);
    OV5642_write_cmos_sensor(0x8426, 0x07);
    OV5642_write_cmos_sensor(0x8427, 0x07);
    OV5642_write_cmos_sensor(0x8428, 0xe4);
    OV5642_write_cmos_sensor(0x8429, 0xb5);
    OV5642_write_cmos_sensor(0x842a, 0x06);
    OV5642_write_cmos_sensor(0x842b, 0x03);
    OV5642_write_cmos_sensor(0x842c, 0xd3);
    OV5642_write_cmos_sensor(0x842d, 0x80);
    OV5642_write_cmos_sensor(0x842e, 0x02);
    OV5642_write_cmos_sensor(0x842f, 0xa2);
    OV5642_write_cmos_sensor(0x8430, 0x1d);
    OV5642_write_cmos_sensor(0x8431, 0x92);
    OV5642_write_cmos_sensor(0x8432, 0x1d);
    OV5642_write_cmos_sensor(0x8433, 0xe5);
    OV5642_write_cmos_sensor(0x8434, 0x4b);
    OV5642_write_cmos_sensor(0x8435, 0xb5);
    OV5642_write_cmos_sensor(0x8436, 0x07);
    OV5642_write_cmos_sensor(0x8437, 0x07);
    OV5642_write_cmos_sensor(0x8438, 0xe4);
    OV5642_write_cmos_sensor(0x8439, 0xb5);
    OV5642_write_cmos_sensor(0x843a, 0x06);
    OV5642_write_cmos_sensor(0x843b, 0x03);
    OV5642_write_cmos_sensor(0x843c, 0xd3);
    OV5642_write_cmos_sensor(0x843d, 0x80);
    OV5642_write_cmos_sensor(0x843e, 0x02);
    OV5642_write_cmos_sensor(0x843f, 0xa2);
    OV5642_write_cmos_sensor(0x8440, 0x1c);
    OV5642_write_cmos_sensor(0x8441, 0x92);
    OV5642_write_cmos_sensor(0x8442, 0x1c);
    OV5642_write_cmos_sensor(0x8443, 0x12);
    OV5642_write_cmos_sensor(0x8444, 0x01);
    OV5642_write_cmos_sensor(0x8445, 0xc9);
    OV5642_write_cmos_sensor(0x8446, 0xe5);
    OV5642_write_cmos_sensor(0x8447, 0x4a);
    OV5642_write_cmos_sensor(0x8448, 0xb5);
    OV5642_write_cmos_sensor(0x8449, 0x07);
    OV5642_write_cmos_sensor(0x844a, 0x07);
    OV5642_write_cmos_sensor(0x844b, 0xe4);
    OV5642_write_cmos_sensor(0x844c, 0xb5);
    OV5642_write_cmos_sensor(0x844d, 0x06);
    OV5642_write_cmos_sensor(0x844e, 0x03);
    OV5642_write_cmos_sensor(0x844f, 0xd3);
    OV5642_write_cmos_sensor(0x8450, 0x80);
    OV5642_write_cmos_sensor(0x8451, 0x02);
    OV5642_write_cmos_sensor(0x8452, 0xa2);
    OV5642_write_cmos_sensor(0x8453, 0x1b);
    OV5642_write_cmos_sensor(0x8454, 0x92);
    OV5642_write_cmos_sensor(0x8455, 0x1b);
    OV5642_write_cmos_sensor(0x8456, 0xe5);
    OV5642_write_cmos_sensor(0x8457, 0x4e);
    OV5642_write_cmos_sensor(0x8458, 0x12);
    OV5642_write_cmos_sensor(0x8459, 0x01);
    OV5642_write_cmos_sensor(0x845a, 0xcb);
    OV5642_write_cmos_sensor(0x845b, 0xad);
    OV5642_write_cmos_sensor(0x845c, 0x0b);
    OV5642_write_cmos_sensor(0x845d, 0x7c);
    OV5642_write_cmos_sensor(0x845e, 0x00);
    OV5642_write_cmos_sensor(0x845f, 0xef);
    OV5642_write_cmos_sensor(0x8460, 0xb5);
    OV5642_write_cmos_sensor(0x8461, 0x05);
    OV5642_write_cmos_sensor(0x8462, 0x07);
    OV5642_write_cmos_sensor(0x8463, 0xec);
    OV5642_write_cmos_sensor(0x8464, 0xb5);
    OV5642_write_cmos_sensor(0x8465, 0x06);
    OV5642_write_cmos_sensor(0x8466, 0x03);
    OV5642_write_cmos_sensor(0x8467, 0xd3);
    OV5642_write_cmos_sensor(0x8468, 0x80);
    OV5642_write_cmos_sensor(0x8469, 0x02);
    OV5642_write_cmos_sensor(0x846a, 0xa2);
    OV5642_write_cmos_sensor(0x846b, 0x1f);
    OV5642_write_cmos_sensor(0x846c, 0x92);
    OV5642_write_cmos_sensor(0x846d, 0x1f);
    OV5642_write_cmos_sensor(0x846e, 0xe5);
    OV5642_write_cmos_sensor(0x846f, 0x4d);
    OV5642_write_cmos_sensor(0x8470, 0x12);
    OV5642_write_cmos_sensor(0x8471, 0x01);
    OV5642_write_cmos_sensor(0x8472, 0xcb);
    OV5642_write_cmos_sensor(0x8473, 0xef);
    OV5642_write_cmos_sensor(0x8474, 0xb5);
    OV5642_write_cmos_sensor(0x8475, 0x05);
    OV5642_write_cmos_sensor(0x8476, 0x07);
    OV5642_write_cmos_sensor(0x8477, 0xee);
    OV5642_write_cmos_sensor(0x8478, 0xb5);
    OV5642_write_cmos_sensor(0x8479, 0x04);
    OV5642_write_cmos_sensor(0x847a, 0x03);
    OV5642_write_cmos_sensor(0x847b, 0xd3);
    OV5642_write_cmos_sensor(0x847c, 0x80);
    OV5642_write_cmos_sensor(0x847d, 0x02);
    OV5642_write_cmos_sensor(0x847e, 0xa2);
    OV5642_write_cmos_sensor(0x847f, 0x1e);
    OV5642_write_cmos_sensor(0x8480, 0x92);
    OV5642_write_cmos_sensor(0x8481, 0x1e);
    OV5642_write_cmos_sensor(0x8482, 0xe5);
    OV5642_write_cmos_sensor(0x8483, 0x4c);
    OV5642_write_cmos_sensor(0x8484, 0x12);
    OV5642_write_cmos_sensor(0x8485, 0x01);
    OV5642_write_cmos_sensor(0x8486, 0xcb);
    OV5642_write_cmos_sensor(0x8487, 0xad);
    OV5642_write_cmos_sensor(0x8488, 0x0b);
    OV5642_write_cmos_sensor(0x8489, 0x7c);
    OV5642_write_cmos_sensor(0x848a, 0x00);
    OV5642_write_cmos_sensor(0x848b, 0xef);
    OV5642_write_cmos_sensor(0x848c, 0xb5);
    OV5642_write_cmos_sensor(0x848d, 0x05);
    OV5642_write_cmos_sensor(0x848e, 0x07);
    OV5642_write_cmos_sensor(0x848f, 0xec);
    OV5642_write_cmos_sensor(0x8490, 0xb5);
    OV5642_write_cmos_sensor(0x8491, 0x06);
    OV5642_write_cmos_sensor(0x8492, 0x03);
    OV5642_write_cmos_sensor(0x8493, 0xd3);
    OV5642_write_cmos_sensor(0x8494, 0x80);
    OV5642_write_cmos_sensor(0x8495, 0x02);
    OV5642_write_cmos_sensor(0x8496, 0xa2);
    OV5642_write_cmos_sensor(0x8497, 0x1d);
    OV5642_write_cmos_sensor(0x8498, 0x92);
    OV5642_write_cmos_sensor(0x8499, 0x1d);
    OV5642_write_cmos_sensor(0x849a, 0xe5);
    OV5642_write_cmos_sensor(0x849b, 0x4b);
    OV5642_write_cmos_sensor(0x849c, 0x12);
    OV5642_write_cmos_sensor(0x849d, 0x01);
    OV5642_write_cmos_sensor(0x849e, 0xcb);
    OV5642_write_cmos_sensor(0x849f, 0xef);
    OV5642_write_cmos_sensor(0x84a0, 0xb5);
    OV5642_write_cmos_sensor(0x84a1, 0x05);
    OV5642_write_cmos_sensor(0x84a2, 0x07);
    OV5642_write_cmos_sensor(0x84a3, 0xee);
    OV5642_write_cmos_sensor(0x84a4, 0xb5);
    OV5642_write_cmos_sensor(0x84a5, 0x04);
    OV5642_write_cmos_sensor(0x84a6, 0x03);
    OV5642_write_cmos_sensor(0x84a7, 0xd3);
    OV5642_write_cmos_sensor(0x84a8, 0x80);
    OV5642_write_cmos_sensor(0x84a9, 0x02);
    OV5642_write_cmos_sensor(0x84aa, 0xa2);
    OV5642_write_cmos_sensor(0x84ab, 0x1c);
    OV5642_write_cmos_sensor(0x84ac, 0x92);
    OV5642_write_cmos_sensor(0x84ad, 0x1c);
    OV5642_write_cmos_sensor(0x84ae, 0xe5);
    OV5642_write_cmos_sensor(0x84af, 0x4a);
    OV5642_write_cmos_sensor(0x84b0, 0x12);
    OV5642_write_cmos_sensor(0x84b1, 0x01);
    OV5642_write_cmos_sensor(0x84b2, 0xcb);
    OV5642_write_cmos_sensor(0x84b3, 0x7c);
    OV5642_write_cmos_sensor(0x84b4, 0x00);
    OV5642_write_cmos_sensor(0x84b5, 0xef);
    OV5642_write_cmos_sensor(0x84b6, 0xb5);
    OV5642_write_cmos_sensor(0x84b7, 0x0b);
    OV5642_write_cmos_sensor(0x84b8, 0x07);
    OV5642_write_cmos_sensor(0x84b9, 0xec);
    OV5642_write_cmos_sensor(0x84ba, 0xb5);
    OV5642_write_cmos_sensor(0x84bb, 0x06);
    OV5642_write_cmos_sensor(0x84bc, 0x03);
    OV5642_write_cmos_sensor(0x84bd, 0xd3);
    OV5642_write_cmos_sensor(0x84be, 0x80);
    OV5642_write_cmos_sensor(0x84bf, 0x02);
    OV5642_write_cmos_sensor(0x84c0, 0xa2);
    OV5642_write_cmos_sensor(0x84c1, 0x1b);
    OV5642_write_cmos_sensor(0x84c2, 0x92);
    OV5642_write_cmos_sensor(0x84c3, 0x1b);
    OV5642_write_cmos_sensor(0x84c4, 0xe5);
    OV5642_write_cmos_sensor(0x84c5, 0x30);
    OV5642_write_cmos_sensor(0x84c6, 0xd3);
    OV5642_write_cmos_sensor(0x84c7, 0x94);
    OV5642_write_cmos_sensor(0x84c8, 0x00);
    OV5642_write_cmos_sensor(0x84c9, 0x40);
    OV5642_write_cmos_sensor(0x84ca, 0x04);
    OV5642_write_cmos_sensor(0x84cb, 0xa2);
    OV5642_write_cmos_sensor(0x84cc, 0x1f);
    OV5642_write_cmos_sensor(0x84cd, 0x80);
    OV5642_write_cmos_sensor(0x84ce, 0x01);
    OV5642_write_cmos_sensor(0x84cf, 0xc3);
    OV5642_write_cmos_sensor(0x84d0, 0x92);
    OV5642_write_cmos_sensor(0x84d1, 0x1f);
    OV5642_write_cmos_sensor(0x84d2, 0xe5);
    OV5642_write_cmos_sensor(0x84d3, 0x2f);
    OV5642_write_cmos_sensor(0x84d4, 0xd3);
    OV5642_write_cmos_sensor(0x84d5, 0x94);
    OV5642_write_cmos_sensor(0x84d6, 0x00);
    OV5642_write_cmos_sensor(0x84d7, 0x40);
    OV5642_write_cmos_sensor(0x84d8, 0x04);
    OV5642_write_cmos_sensor(0x84d9, 0xa2);
    OV5642_write_cmos_sensor(0x84da, 0x1e);
    OV5642_write_cmos_sensor(0x84db, 0x80);
    OV5642_write_cmos_sensor(0x84dc, 0x01);
    OV5642_write_cmos_sensor(0x84dd, 0xc3);
    OV5642_write_cmos_sensor(0x84de, 0x92);
    OV5642_write_cmos_sensor(0x84df, 0x1e);
    OV5642_write_cmos_sensor(0x84e0, 0xe5);
    OV5642_write_cmos_sensor(0x84e1, 0x2e);
    OV5642_write_cmos_sensor(0x84e2, 0xd3);
    OV5642_write_cmos_sensor(0x84e3, 0x94);
    OV5642_write_cmos_sensor(0x84e4, 0x00);
    OV5642_write_cmos_sensor(0x84e5, 0x40);
    OV5642_write_cmos_sensor(0x84e6, 0x04);
    OV5642_write_cmos_sensor(0x84e7, 0xa2);
    OV5642_write_cmos_sensor(0x84e8, 0x1d);
    OV5642_write_cmos_sensor(0x84e9, 0x80);
    OV5642_write_cmos_sensor(0x84ea, 0x01);
    OV5642_write_cmos_sensor(0x84eb, 0xc3);
    OV5642_write_cmos_sensor(0x84ec, 0x92);
    OV5642_write_cmos_sensor(0x84ed, 0x1d);
    OV5642_write_cmos_sensor(0x84ee, 0xe5);
    OV5642_write_cmos_sensor(0x84ef, 0x2d);
    OV5642_write_cmos_sensor(0x84f0, 0xd3);
    OV5642_write_cmos_sensor(0x84f1, 0x94);
    OV5642_write_cmos_sensor(0x84f2, 0x00);
    OV5642_write_cmos_sensor(0x84f3, 0x40);
    OV5642_write_cmos_sensor(0x84f4, 0x04);
    OV5642_write_cmos_sensor(0x84f5, 0xa2);
    OV5642_write_cmos_sensor(0x84f6, 0x1c);
    OV5642_write_cmos_sensor(0x84f7, 0x80);
    OV5642_write_cmos_sensor(0x84f8, 0x01);
    OV5642_write_cmos_sensor(0x84f9, 0xc3);
    OV5642_write_cmos_sensor(0x84fa, 0x92);
    OV5642_write_cmos_sensor(0x84fb, 0x1c);
    OV5642_write_cmos_sensor(0x84fc, 0xe5);
    OV5642_write_cmos_sensor(0x84fd, 0x2c);
    OV5642_write_cmos_sensor(0x84fe, 0xd3);
    OV5642_write_cmos_sensor(0x84ff, 0x94);
    OV5642_write_cmos_sensor(0x8500, 0x00);
    OV5642_write_cmos_sensor(0x8501, 0x40);
    OV5642_write_cmos_sensor(0x8502, 0x04);
    OV5642_write_cmos_sensor(0x8503, 0xa2);
    OV5642_write_cmos_sensor(0x8504, 0x1b);
    OV5642_write_cmos_sensor(0x8505, 0x80);
    OV5642_write_cmos_sensor(0x8506, 0x01);
    OV5642_write_cmos_sensor(0x8507, 0xc3);
    OV5642_write_cmos_sensor(0x8508, 0x92);
    OV5642_write_cmos_sensor(0x8509, 0x1b);
    OV5642_write_cmos_sensor(0x850a, 0x85);
    OV5642_write_cmos_sensor(0x850b, 0x0a);
    OV5642_write_cmos_sensor(0x850c, 0x79);
    OV5642_write_cmos_sensor(0x850d, 0xe5);
    OV5642_write_cmos_sensor(0x850e, 0x7d);
    OV5642_write_cmos_sensor(0x850f, 0xb5);
    OV5642_write_cmos_sensor(0x8510, 0x0b);
    OV5642_write_cmos_sensor(0x8511, 0x03);
    OV5642_write_cmos_sensor(0x8512, 0x02);
    OV5642_write_cmos_sensor(0x8513, 0x15);
    OV5642_write_cmos_sensor(0x8514, 0x17);
    OV5642_write_cmos_sensor(0x8515, 0x85);
    OV5642_write_cmos_sensor(0x8516, 0x0b);
    OV5642_write_cmos_sensor(0x8517, 0x0c);
    OV5642_write_cmos_sensor(0x8518, 0x12);
    OV5642_write_cmos_sensor(0x8519, 0x11);
    OV5642_write_cmos_sensor(0x851a, 0xd8);
    OV5642_write_cmos_sensor(0x851b, 0xd2);
    OV5642_write_cmos_sensor(0x851c, 0x02);
    OV5642_write_cmos_sensor(0x851d, 0xc2);
    OV5642_write_cmos_sensor(0x851e, 0x01);
    OV5642_write_cmos_sensor(0x851f, 0xd2);
    OV5642_write_cmos_sensor(0x8520, 0x00);
    OV5642_write_cmos_sensor(0x8521, 0x75);
    OV5642_write_cmos_sensor(0x8522, 0x31);
    OV5642_write_cmos_sensor(0x8523, 0x03);
    OV5642_write_cmos_sensor(0x8524, 0x22);
    OV5642_write_cmos_sensor(0x8525, 0xe5);
    OV5642_write_cmos_sensor(0x8526, 0x7e);
    OV5642_write_cmos_sensor(0x8527, 0x24);
    OV5642_write_cmos_sensor(0x8528, 0xfe);
    OV5642_write_cmos_sensor(0x8529, 0x60);
    OV5642_write_cmos_sensor(0x852a, 0x27);
    OV5642_write_cmos_sensor(0x852b, 0x14);
    OV5642_write_cmos_sensor(0x852c, 0x60);
    OV5642_write_cmos_sensor(0x852d, 0x31);
    OV5642_write_cmos_sensor(0x852e, 0x24);
    OV5642_write_cmos_sensor(0x852f, 0xf8);
    OV5642_write_cmos_sensor(0x8530, 0x60);
    OV5642_write_cmos_sensor(0x8531, 0x3a);
    OV5642_write_cmos_sensor(0x8532, 0x14);
    OV5642_write_cmos_sensor(0x8533, 0x60);
    OV5642_write_cmos_sensor(0x8534, 0x4b);
    OV5642_write_cmos_sensor(0x8535, 0x14);
    OV5642_write_cmos_sensor(0x8536, 0x60);
    OV5642_write_cmos_sensor(0x8537, 0x59);
    OV5642_write_cmos_sensor(0x8538, 0x14);
    OV5642_write_cmos_sensor(0x8539, 0x60);
    OV5642_write_cmos_sensor(0x853a, 0x6a);
    OV5642_write_cmos_sensor(0x853b, 0x24);
    OV5642_write_cmos_sensor(0x853c, 0xfd);
    OV5642_write_cmos_sensor(0x853d, 0x70);
    OV5642_write_cmos_sensor(0x853e, 0x03);
    OV5642_write_cmos_sensor(0x853f, 0x02);
    OV5642_write_cmos_sensor(0x8540, 0x05);
    OV5642_write_cmos_sensor(0x8541, 0xda);
    OV5642_write_cmos_sensor(0x8542, 0x24);
    OV5642_write_cmos_sensor(0x8543, 0x10);
    OV5642_write_cmos_sensor(0x8544, 0x60);
    OV5642_write_cmos_sensor(0x8545, 0x03);
    OV5642_write_cmos_sensor(0x8546, 0x02);
    OV5642_write_cmos_sensor(0x8547, 0x06);
    OV5642_write_cmos_sensor(0x8548, 0xd5);
    OV5642_write_cmos_sensor(0x8549, 0xe4);
    OV5642_write_cmos_sensor(0x854a, 0xf5);
    OV5642_write_cmos_sensor(0x854b, 0x0a);
    OV5642_write_cmos_sensor(0x854c, 0x12);
    OV5642_write_cmos_sensor(0x854d, 0x11);
    OV5642_write_cmos_sensor(0x854e, 0x10);
    OV5642_write_cmos_sensor(0x854f, 0x02);
    OV5642_write_cmos_sensor(0x8550, 0x06);
    OV5642_write_cmos_sensor(0x8551, 0xbe);
    OV5642_write_cmos_sensor(0x8552, 0x75);
    OV5642_write_cmos_sensor(0x8553, 0x0a);
    OV5642_write_cmos_sensor(0x8554, 0x01);
    OV5642_write_cmos_sensor(0x8555, 0x12);
    OV5642_write_cmos_sensor(0x8556, 0x06);
    OV5642_write_cmos_sensor(0x8557, 0xd6);
    OV5642_write_cmos_sensor(0x8558, 0xc2);
    OV5642_write_cmos_sensor(0x8559, 0x3a);
    OV5642_write_cmos_sensor(0x855a, 0xd2);
    OV5642_write_cmos_sensor(0x855b, 0x39);
    OV5642_write_cmos_sensor(0x855c, 0x02);
    OV5642_write_cmos_sensor(0x855d, 0x06);
    OV5642_write_cmos_sensor(0x855e, 0xcf);
    OV5642_write_cmos_sensor(0x855f, 0x75);
    OV5642_write_cmos_sensor(0x8560, 0x0a);
    OV5642_write_cmos_sensor(0x8561, 0x02);
    OV5642_write_cmos_sensor(0x8562, 0x12);
    OV5642_write_cmos_sensor(0x8563, 0x06);
    OV5642_write_cmos_sensor(0x8564, 0xd6);
    OV5642_write_cmos_sensor(0x8565, 0xd2);
    OV5642_write_cmos_sensor(0x8566, 0x3a);
    OV5642_write_cmos_sensor(0x8567, 0xc2);
    OV5642_write_cmos_sensor(0x8568, 0x39);
    OV5642_write_cmos_sensor(0x8569, 0x02);
    OV5642_write_cmos_sensor(0x856a, 0x06);
    OV5642_write_cmos_sensor(0x856b, 0xcf);
    OV5642_write_cmos_sensor(0x856c, 0x30);
    OV5642_write_cmos_sensor(0x856d, 0x36);
    OV5642_write_cmos_sensor(0x856e, 0x0c);
    OV5642_write_cmos_sensor(0x856f, 0x20);
    OV5642_write_cmos_sensor(0x8570, 0x3a);
    OV5642_write_cmos_sensor(0x8571, 0x03);
    OV5642_write_cmos_sensor(0x8572, 0x30);
    OV5642_write_cmos_sensor(0x8573, 0x39);
    OV5642_write_cmos_sensor(0x8574, 0x06);
    OV5642_write_cmos_sensor(0x8575, 0xe4);
    OV5642_write_cmos_sensor(0x8576, 0xf5);
    OV5642_write_cmos_sensor(0x8577, 0x0a);
    OV5642_write_cmos_sensor(0x8578, 0x12);
    OV5642_write_cmos_sensor(0x8579, 0x12);
    OV5642_write_cmos_sensor(0x857a, 0x91);
    OV5642_write_cmos_sensor(0x857b, 0xd2);
    OV5642_write_cmos_sensor(0x857c, 0x18);
    OV5642_write_cmos_sensor(0x857d, 0xc2);
    OV5642_write_cmos_sensor(0x857e, 0x19);
    OV5642_write_cmos_sensor(0x857f, 0x22);
    OV5642_write_cmos_sensor(0x8580, 0x30);
    OV5642_write_cmos_sensor(0x8581, 0x36);
    OV5642_write_cmos_sensor(0x8582, 0x09);
    OV5642_write_cmos_sensor(0x8583, 0x20);
    OV5642_write_cmos_sensor(0x8584, 0x3a);
    OV5642_write_cmos_sensor(0x8585, 0x03);
    OV5642_write_cmos_sensor(0x8586, 0x30);
    OV5642_write_cmos_sensor(0x8587, 0x39);
    OV5642_write_cmos_sensor(0x8588, 0x03);
    OV5642_write_cmos_sensor(0x8589, 0x12);
    OV5642_write_cmos_sensor(0x858a, 0x06);
    OV5642_write_cmos_sensor(0x858b, 0xe7);
    OV5642_write_cmos_sensor(0x858c, 0xd2);
    OV5642_write_cmos_sensor(0x858d, 0x18);
    OV5642_write_cmos_sensor(0x858e, 0xd2);
    OV5642_write_cmos_sensor(0x858f, 0x19);
    OV5642_write_cmos_sensor(0x8590, 0x22);
    OV5642_write_cmos_sensor(0x8591, 0x30);
    OV5642_write_cmos_sensor(0x8592, 0x36);
    OV5642_write_cmos_sensor(0x8593, 0x0c);
    OV5642_write_cmos_sensor(0x8594, 0x20);
    OV5642_write_cmos_sensor(0x8595, 0x3a);
    OV5642_write_cmos_sensor(0x8596, 0x03);
    OV5642_write_cmos_sensor(0x8597, 0x30);
    OV5642_write_cmos_sensor(0x8598, 0x39);
    OV5642_write_cmos_sensor(0x8599, 0x06);
    OV5642_write_cmos_sensor(0x859a, 0x75);
    OV5642_write_cmos_sensor(0x859b, 0x0a);
    OV5642_write_cmos_sensor(0x859c, 0x02);
    OV5642_write_cmos_sensor(0x859d, 0x12);
    OV5642_write_cmos_sensor(0x859e, 0x12);
    OV5642_write_cmos_sensor(0x859f, 0x91);
    OV5642_write_cmos_sensor(0x85a0, 0xc2);
    OV5642_write_cmos_sensor(0x85a1, 0x18);
    OV5642_write_cmos_sensor(0x85a2, 0xd2);
    OV5642_write_cmos_sensor(0x85a3, 0x19);
    OV5642_write_cmos_sensor(0x85a4, 0x22);
    OV5642_write_cmos_sensor(0x85a5, 0x20);
    OV5642_write_cmos_sensor(0x85a6, 0x3a);
    OV5642_write_cmos_sensor(0x85a7, 0x06);
    OV5642_write_cmos_sensor(0x85a8, 0x20);
    OV5642_write_cmos_sensor(0x85a9, 0x39);
    OV5642_write_cmos_sensor(0x85aa, 0x03);
    OV5642_write_cmos_sensor(0x85ab, 0x02);
    OV5642_write_cmos_sensor(0x85ac, 0x06);
    OV5642_write_cmos_sensor(0x85ad, 0xd5);
    OV5642_write_cmos_sensor(0x85ae, 0xe5);
    OV5642_write_cmos_sensor(0x85af, 0x79);
    OV5642_write_cmos_sensor(0x85b0, 0xd3);
    OV5642_write_cmos_sensor(0x85b1, 0x94);
    OV5642_write_cmos_sensor(0x85b2, 0x03);
    OV5642_write_cmos_sensor(0x85b3, 0x40);
    OV5642_write_cmos_sensor(0x85b4, 0x04);
    OV5642_write_cmos_sensor(0x85b5, 0x7f);
    OV5642_write_cmos_sensor(0x85b6, 0x00);
    OV5642_write_cmos_sensor(0x85b7, 0x80);
    OV5642_write_cmos_sensor(0x85b8, 0x04);
    OV5642_write_cmos_sensor(0x85b9, 0xe5);
    OV5642_write_cmos_sensor(0x85ba, 0x79);
    OV5642_write_cmos_sensor(0x85bb, 0x04);
    OV5642_write_cmos_sensor(0x85bc, 0xff);
    OV5642_write_cmos_sensor(0x85bd, 0x8f);
    OV5642_write_cmos_sensor(0x85be, 0x79);
    OV5642_write_cmos_sensor(0x85bf, 0x30);
    OV5642_write_cmos_sensor(0x85c0, 0x18);
    OV5642_write_cmos_sensor(0x85c1, 0x06);
    OV5642_write_cmos_sensor(0x85c2, 0x30);
    OV5642_write_cmos_sensor(0x85c3, 0x19);
    OV5642_write_cmos_sensor(0x85c4, 0x03);
    OV5642_write_cmos_sensor(0x85c5, 0x12);
    OV5642_write_cmos_sensor(0x85c6, 0x06);
    OV5642_write_cmos_sensor(0x85c7, 0xe7);
    OV5642_write_cmos_sensor(0x85c8, 0x30);
    OV5642_write_cmos_sensor(0x85c9, 0x18);
    OV5642_write_cmos_sensor(0x85ca, 0x03);
    OV5642_write_cmos_sensor(0x85cb, 0x02);
    OV5642_write_cmos_sensor(0x85cc, 0x06);
    OV5642_write_cmos_sensor(0x85cd, 0xd5);
    OV5642_write_cmos_sensor(0x85ce, 0x20);
    OV5642_write_cmos_sensor(0x85cf, 0x19);
    OV5642_write_cmos_sensor(0x85d0, 0x03);
    OV5642_write_cmos_sensor(0x85d1, 0x02);
    OV5642_write_cmos_sensor(0x85d2, 0x06);
    OV5642_write_cmos_sensor(0x85d3, 0xd5);
    OV5642_write_cmos_sensor(0x85d4, 0x75);
    OV5642_write_cmos_sensor(0x85d5, 0x0a);
    OV5642_write_cmos_sensor(0x85d6, 0x02);
    OV5642_write_cmos_sensor(0x85d7, 0x02);
    OV5642_write_cmos_sensor(0x85d8, 0x12);
    OV5642_write_cmos_sensor(0x85d9, 0x91);
    OV5642_write_cmos_sensor(0x85da, 0xe5);
    OV5642_write_cmos_sensor(0x85db, 0x68);
    OV5642_write_cmos_sensor(0x85dc, 0xd3);
    OV5642_write_cmos_sensor(0x85dd, 0x94);
    OV5642_write_cmos_sensor(0x85de, 0x38);
    OV5642_write_cmos_sensor(0x85df, 0x40);
    OV5642_write_cmos_sensor(0x85e0, 0x04);
    OV5642_write_cmos_sensor(0x85e1, 0x7f);
    OV5642_write_cmos_sensor(0x85e2, 0x34);
    OV5642_write_cmos_sensor(0x85e3, 0x80);
    OV5642_write_cmos_sensor(0x85e4, 0x02);
    OV5642_write_cmos_sensor(0x85e5, 0xaf);
    OV5642_write_cmos_sensor(0x85e6, 0x68);
    OV5642_write_cmos_sensor(0x85e7, 0x8f);
    OV5642_write_cmos_sensor(0x85e8, 0x68);
    OV5642_write_cmos_sensor(0x85e9, 0xe5);
    OV5642_write_cmos_sensor(0x85ea, 0x68);
    OV5642_write_cmos_sensor(0x85eb, 0xc3);
    OV5642_write_cmos_sensor(0x85ec, 0x94);
    OV5642_write_cmos_sensor(0x85ed, 0x08);
    OV5642_write_cmos_sensor(0x85ee, 0x50);
    OV5642_write_cmos_sensor(0x85ef, 0x04);
    OV5642_write_cmos_sensor(0x85f0, 0x7f);
    OV5642_write_cmos_sensor(0x85f1, 0x08);
    OV5642_write_cmos_sensor(0x85f2, 0x80);
    OV5642_write_cmos_sensor(0x85f3, 0x02);
    OV5642_write_cmos_sensor(0x85f4, 0xaf);
    OV5642_write_cmos_sensor(0x85f5, 0x68);
    OV5642_write_cmos_sensor(0x85f6, 0x8f);
    OV5642_write_cmos_sensor(0x85f7, 0x68);
    OV5642_write_cmos_sensor(0x85f8, 0xe5);
    OV5642_write_cmos_sensor(0x85f9, 0x69);
    OV5642_write_cmos_sensor(0x85fa, 0xd3);
    OV5642_write_cmos_sensor(0x85fb, 0x94);
    OV5642_write_cmos_sensor(0x85fc, 0x2a);
    OV5642_write_cmos_sensor(0x85fd, 0x40);
    OV5642_write_cmos_sensor(0x85fe, 0x04);
    OV5642_write_cmos_sensor(0x85ff, 0x7f);
    OV5642_write_cmos_sensor(0x8600, 0x2a);
    OV5642_write_cmos_sensor(0x8601, 0x80);
    OV5642_write_cmos_sensor(0x8602, 0x02);
    OV5642_write_cmos_sensor(0x8603, 0xaf);
    OV5642_write_cmos_sensor(0x8604, 0x69);
    OV5642_write_cmos_sensor(0x8605, 0x8f);
    OV5642_write_cmos_sensor(0x8606, 0x69);
    OV5642_write_cmos_sensor(0x8607, 0xe5);
    OV5642_write_cmos_sensor(0x8608, 0x69);
    OV5642_write_cmos_sensor(0x8609, 0xc3);
    OV5642_write_cmos_sensor(0x860a, 0x94);
    OV5642_write_cmos_sensor(0x860b, 0x06);
    OV5642_write_cmos_sensor(0x860c, 0x50);
    OV5642_write_cmos_sensor(0x860d, 0x04);
    OV5642_write_cmos_sensor(0x860e, 0x7f);
    OV5642_write_cmos_sensor(0x860f, 0x06);
    OV5642_write_cmos_sensor(0x8610, 0x80);
    OV5642_write_cmos_sensor(0x8611, 0x02);
    OV5642_write_cmos_sensor(0x8612, 0xaf);
    OV5642_write_cmos_sensor(0x8613, 0x69);
    OV5642_write_cmos_sensor(0x8614, 0x8f);
    OV5642_write_cmos_sensor(0x8615, 0x69);
    OV5642_write_cmos_sensor(0x8616, 0xaf);
    OV5642_write_cmos_sensor(0x8617, 0x68);
    OV5642_write_cmos_sensor(0x8618, 0xef);
    OV5642_write_cmos_sensor(0x8619, 0x24);
    OV5642_write_cmos_sensor(0x861a, 0xf8);
    OV5642_write_cmos_sensor(0x861b, 0xff);
    OV5642_write_cmos_sensor(0x861c, 0xe4);
    OV5642_write_cmos_sensor(0x861d, 0x34);
    OV5642_write_cmos_sensor(0x861e, 0xff);
    OV5642_write_cmos_sensor(0x861f, 0xfe);
    OV5642_write_cmos_sensor(0x8620, 0xe4);
    OV5642_write_cmos_sensor(0x8621, 0x8f);
    OV5642_write_cmos_sensor(0x8622, 0x3f);
    OV5642_write_cmos_sensor(0x8623, 0x8e);
    OV5642_write_cmos_sensor(0x8624, 0x3e);
    OV5642_write_cmos_sensor(0x8625, 0xf5);
    OV5642_write_cmos_sensor(0x8626, 0x3d);
    OV5642_write_cmos_sensor(0x8627, 0xf5);
    OV5642_write_cmos_sensor(0x8628, 0x3c);
    OV5642_write_cmos_sensor(0x8629, 0xac);
    OV5642_write_cmos_sensor(0x862a, 0x3c);
    OV5642_write_cmos_sensor(0x862b, 0x12);
    OV5642_write_cmos_sensor(0x862c, 0x01);
    OV5642_write_cmos_sensor(0x862d, 0xb9);
    OV5642_write_cmos_sensor(0x862e, 0xaf);
    OV5642_write_cmos_sensor(0x862f, 0x69);
    OV5642_write_cmos_sensor(0x8630, 0xef);
    OV5642_write_cmos_sensor(0x8631, 0x24);
    OV5642_write_cmos_sensor(0x8632, 0xfa);
    OV5642_write_cmos_sensor(0x8633, 0xff);
    OV5642_write_cmos_sensor(0x8634, 0xe4);
    OV5642_write_cmos_sensor(0x8635, 0x34);
    OV5642_write_cmos_sensor(0x8636, 0xff);
    OV5642_write_cmos_sensor(0x8637, 0x12);
    OV5642_write_cmos_sensor(0x8638, 0x01);
    OV5642_write_cmos_sensor(0x8639, 0x9c);
    OV5642_write_cmos_sensor(0x863a, 0xaf);
    OV5642_write_cmos_sensor(0x863b, 0x68);
    OV5642_write_cmos_sensor(0x863c, 0xef);
    OV5642_write_cmos_sensor(0x863d, 0x24);
    OV5642_write_cmos_sensor(0x863e, 0x08);
    OV5642_write_cmos_sensor(0x863f, 0xff);
    OV5642_write_cmos_sensor(0x8640, 0xe4);
    OV5642_write_cmos_sensor(0x8641, 0x33);
    OV5642_write_cmos_sensor(0x8642, 0x12);
    OV5642_write_cmos_sensor(0x8643, 0x01);
    OV5642_write_cmos_sensor(0x8644, 0x9c);
    OV5642_write_cmos_sensor(0x8645, 0xaf);
    OV5642_write_cmos_sensor(0x8646, 0x69);
    OV5642_write_cmos_sensor(0x8647, 0xef);
    OV5642_write_cmos_sensor(0x8648, 0x24);
    OV5642_write_cmos_sensor(0x8649, 0x06);
    OV5642_write_cmos_sensor(0x864a, 0xff);
    OV5642_write_cmos_sensor(0x864b, 0xe4);
    OV5642_write_cmos_sensor(0x864c, 0x33);
    OV5642_write_cmos_sensor(0x864d, 0xfe);
    OV5642_write_cmos_sensor(0x864e, 0xe4);
    OV5642_write_cmos_sensor(0x864f, 0xfc);
    OV5642_write_cmos_sensor(0x8650, 0xfd);
    OV5642_write_cmos_sensor(0x8651, 0xe5);
    OV5642_write_cmos_sensor(0x8652, 0x3f);
    OV5642_write_cmos_sensor(0x8653, 0x2f);
    OV5642_write_cmos_sensor(0x8654, 0xf5);
    OV5642_write_cmos_sensor(0x8655, 0x3f);
    OV5642_write_cmos_sensor(0x8656, 0xe5);
    OV5642_write_cmos_sensor(0x8657, 0x3e);
    OV5642_write_cmos_sensor(0x8658, 0x3e);
    OV5642_write_cmos_sensor(0x8659, 0xf5);
    OV5642_write_cmos_sensor(0x865a, 0x3e);
    OV5642_write_cmos_sensor(0x865b, 0xed);
    OV5642_write_cmos_sensor(0x865c, 0x35);
    OV5642_write_cmos_sensor(0x865d, 0x3d);
    OV5642_write_cmos_sensor(0x865e, 0xf5);
    OV5642_write_cmos_sensor(0x865f, 0x3d);
    OV5642_write_cmos_sensor(0x8660, 0xec);
    OV5642_write_cmos_sensor(0x8661, 0x35);
    OV5642_write_cmos_sensor(0x8662, 0x3c);
    OV5642_write_cmos_sensor(0x8663, 0xf5);
    OV5642_write_cmos_sensor(0x8664, 0x3c);
    OV5642_write_cmos_sensor(0x8665, 0xe4);
    OV5642_write_cmos_sensor(0x8666, 0x25);
    OV5642_write_cmos_sensor(0x8667, 0x3f);
    OV5642_write_cmos_sensor(0x8668, 0xf5);
    OV5642_write_cmos_sensor(0x8669, 0x37);
    OV5642_write_cmos_sensor(0x866a, 0xe4);
    OV5642_write_cmos_sensor(0x866b, 0x35);
    OV5642_write_cmos_sensor(0x866c, 0x3e);
    OV5642_write_cmos_sensor(0x866d, 0xf5);
    OV5642_write_cmos_sensor(0x866e, 0x36);
    OV5642_write_cmos_sensor(0x866f, 0xe4);
    OV5642_write_cmos_sensor(0x8670, 0x35);
    OV5642_write_cmos_sensor(0x8671, 0x3d);
    OV5642_write_cmos_sensor(0x8672, 0xf5);
    OV5642_write_cmos_sensor(0x8673, 0x35);
    OV5642_write_cmos_sensor(0x8674, 0xe5);
    OV5642_write_cmos_sensor(0x8675, 0x3c);
    OV5642_write_cmos_sensor(0x8676, 0x34);
    OV5642_write_cmos_sensor(0x8677, 0x08);
    OV5642_write_cmos_sensor(0x8678, 0xf5);
    OV5642_write_cmos_sensor(0x8679, 0x34);
    OV5642_write_cmos_sensor(0x867a, 0xe4);
    OV5642_write_cmos_sensor(0x867b, 0x25);
    OV5642_write_cmos_sensor(0x867c, 0x3f);
    OV5642_write_cmos_sensor(0x867d, 0xf5);
    OV5642_write_cmos_sensor(0x867e, 0x3b);
    OV5642_write_cmos_sensor(0x867f, 0xe4);
    OV5642_write_cmos_sensor(0x8680, 0x35);
    OV5642_write_cmos_sensor(0x8681, 0x3e);
    OV5642_write_cmos_sensor(0x8682, 0xf5);
    OV5642_write_cmos_sensor(0x8683, 0x3a);
    OV5642_write_cmos_sensor(0x8684, 0xe5);
    OV5642_write_cmos_sensor(0x8685, 0x3d);
    OV5642_write_cmos_sensor(0x8686, 0x34);
    OV5642_write_cmos_sensor(0x8687, 0x06);
    OV5642_write_cmos_sensor(0x8688, 0xf5);
    OV5642_write_cmos_sensor(0x8689, 0x39);
    OV5642_write_cmos_sensor(0x868a, 0xe4);
    OV5642_write_cmos_sensor(0x868b, 0x35);
    OV5642_write_cmos_sensor(0x868c, 0x3c);
    OV5642_write_cmos_sensor(0x868d, 0xf5);
    OV5642_write_cmos_sensor(0x868e, 0x38);
    OV5642_write_cmos_sensor(0x868f, 0xe5);
    OV5642_write_cmos_sensor(0x8690, 0x3f);
    OV5642_write_cmos_sensor(0x8691, 0x24);
    OV5642_write_cmos_sensor(0x8692, 0xfa);
    OV5642_write_cmos_sensor(0x8693, 0xf5);
    OV5642_write_cmos_sensor(0x8694, 0x43);
    OV5642_write_cmos_sensor(0x8695, 0xe5);
    OV5642_write_cmos_sensor(0x8696, 0x3e);
    OV5642_write_cmos_sensor(0x8697, 0x34);
    OV5642_write_cmos_sensor(0x8698, 0xff);
    OV5642_write_cmos_sensor(0x8699, 0xf5);
    OV5642_write_cmos_sensor(0x869a, 0x42);
    OV5642_write_cmos_sensor(0x869b, 0xe5);
    OV5642_write_cmos_sensor(0x869c, 0x3d);
    OV5642_write_cmos_sensor(0x869d, 0x34);
    OV5642_write_cmos_sensor(0x869e, 0xff);
    OV5642_write_cmos_sensor(0x869f, 0xf5);
    OV5642_write_cmos_sensor(0x86a0, 0x41);
    OV5642_write_cmos_sensor(0x86a1, 0xe5);
    OV5642_write_cmos_sensor(0x86a2, 0x3c);
    OV5642_write_cmos_sensor(0x86a3, 0x34);
    OV5642_write_cmos_sensor(0x86a4, 0xff);
    OV5642_write_cmos_sensor(0x86a5, 0xf5);
    OV5642_write_cmos_sensor(0x86a6, 0x40);
    OV5642_write_cmos_sensor(0x86a7, 0xe4);
    OV5642_write_cmos_sensor(0x86a8, 0x25);
    OV5642_write_cmos_sensor(0x86a9, 0x3f);
    OV5642_write_cmos_sensor(0x86aa, 0xf5);
    OV5642_write_cmos_sensor(0x86ab, 0x47);
    OV5642_write_cmos_sensor(0x86ac, 0xe5);
    OV5642_write_cmos_sensor(0x86ad, 0x3e);
    OV5642_write_cmos_sensor(0x86ae, 0x34);
    OV5642_write_cmos_sensor(0x86af, 0xf8);
    OV5642_write_cmos_sensor(0x86b0, 0xf5);
    OV5642_write_cmos_sensor(0x86b1, 0x46);
    OV5642_write_cmos_sensor(0x86b2, 0xe5);
    OV5642_write_cmos_sensor(0x86b3, 0x3d);
    OV5642_write_cmos_sensor(0x86b4, 0x34);
    OV5642_write_cmos_sensor(0x86b5, 0xff);
    OV5642_write_cmos_sensor(0x86b6, 0xf5);
    OV5642_write_cmos_sensor(0x86b7, 0x45);
    OV5642_write_cmos_sensor(0x86b8, 0xe5);
    OV5642_write_cmos_sensor(0x86b9, 0x3c);
    OV5642_write_cmos_sensor(0x86ba, 0x34);
    OV5642_write_cmos_sensor(0x86bb, 0xff);
    OV5642_write_cmos_sensor(0x86bc, 0xf5);
    OV5642_write_cmos_sensor(0x86bd, 0x44);
    OV5642_write_cmos_sensor(0x86be, 0x75);
    OV5642_write_cmos_sensor(0x86bf, 0x79);
    OV5642_write_cmos_sensor(0x86c0, 0x02);
    OV5642_write_cmos_sensor(0x86c1, 0x75);
    OV5642_write_cmos_sensor(0x86c2, 0x0a);
    OV5642_write_cmos_sensor(0x86c3, 0x01);
    OV5642_write_cmos_sensor(0x86c4, 0x12);
    OV5642_write_cmos_sensor(0x86c5, 0x12);
    OV5642_write_cmos_sensor(0x86c6, 0x91);
    OV5642_write_cmos_sensor(0x86c7, 0xd2);
    OV5642_write_cmos_sensor(0x86c8, 0x18);
    OV5642_write_cmos_sensor(0x86c9, 0xd2);
    OV5642_write_cmos_sensor(0x86ca, 0x19);
    OV5642_write_cmos_sensor(0x86cb, 0xc2);
    OV5642_write_cmos_sensor(0x86cc, 0x3a);
    OV5642_write_cmos_sensor(0x86cd, 0xc2);
    OV5642_write_cmos_sensor(0x86ce, 0x39);
    OV5642_write_cmos_sensor(0x86cf, 0xd2);
    OV5642_write_cmos_sensor(0x86d0, 0x1a);
    OV5642_write_cmos_sensor(0x86d1, 0xd2);
    OV5642_write_cmos_sensor(0x86d2, 0x36);
    OV5642_write_cmos_sensor(0x86d3, 0xd2);
    OV5642_write_cmos_sensor(0x86d4, 0x30);
    OV5642_write_cmos_sensor(0x86d5, 0x22);
    OV5642_write_cmos_sensor(0x86d6, 0x12);
    OV5642_write_cmos_sensor(0x86d7, 0x11);
    OV5642_write_cmos_sensor(0x86d8, 0x10);
    OV5642_write_cmos_sensor(0x86d9, 0x75);
    OV5642_write_cmos_sensor(0x86da, 0x79);
    OV5642_write_cmos_sensor(0x86db, 0x02);
    OV5642_write_cmos_sensor(0x86dc, 0xe4);
    OV5642_write_cmos_sensor(0x86dd, 0xf5);
    OV5642_write_cmos_sensor(0x86de, 0x0a);
    OV5642_write_cmos_sensor(0x86df, 0x12);
    OV5642_write_cmos_sensor(0x86e0, 0x12);
    OV5642_write_cmos_sensor(0x86e1, 0x91);
    OV5642_write_cmos_sensor(0x86e2, 0xd2);
    OV5642_write_cmos_sensor(0x86e3, 0x18);
    OV5642_write_cmos_sensor(0x86e4, 0xc2);
    OV5642_write_cmos_sensor(0x86e5, 0x19);
    OV5642_write_cmos_sensor(0x86e6, 0x22);
    OV5642_write_cmos_sensor(0x86e7, 0x75);
    OV5642_write_cmos_sensor(0x86e8, 0x0a);
    OV5642_write_cmos_sensor(0x86e9, 0x01);
    OV5642_write_cmos_sensor(0x86ea, 0x12);
    OV5642_write_cmos_sensor(0x86eb, 0x12);
    OV5642_write_cmos_sensor(0x86ec, 0x91);
    OV5642_write_cmos_sensor(0x86ed, 0x22);
    OV5642_write_cmos_sensor(0x86ee, 0x90);
    OV5642_write_cmos_sensor(0x86ef, 0x38);
    OV5642_write_cmos_sensor(0x86f0, 0x04);
    OV5642_write_cmos_sensor(0x86f1, 0xe0);
    OV5642_write_cmos_sensor(0x86f2, 0xfe);
    OV5642_write_cmos_sensor(0x86f3, 0xa3);
    OV5642_write_cmos_sensor(0x86f4, 0xe0);
    OV5642_write_cmos_sensor(0x86f5, 0xfd);
    OV5642_write_cmos_sensor(0x86f6, 0xed);
    OV5642_write_cmos_sensor(0x86f7, 0xff);
    OV5642_write_cmos_sensor(0x86f8, 0xee);
    OV5642_write_cmos_sensor(0x86f9, 0x54);
    OV5642_write_cmos_sensor(0x86fa, 0x0f);
    OV5642_write_cmos_sensor(0x86fb, 0xf5);
    OV5642_write_cmos_sensor(0x86fc, 0x0e);
    OV5642_write_cmos_sensor(0x86fd, 0x8f);
    OV5642_write_cmos_sensor(0x86fe, 0x0f);
    OV5642_write_cmos_sensor(0x86ff, 0xa3);
    OV5642_write_cmos_sensor(0x8700, 0xe0);
    OV5642_write_cmos_sensor(0x8701, 0xfe);
    OV5642_write_cmos_sensor(0x8702, 0xa3);
    OV5642_write_cmos_sensor(0x8703, 0xe0);
    OV5642_write_cmos_sensor(0x8704, 0xfd);
    OV5642_write_cmos_sensor(0x8705, 0xed);
    OV5642_write_cmos_sensor(0x8706, 0xff);
    OV5642_write_cmos_sensor(0x8707, 0xee);
    OV5642_write_cmos_sensor(0x8708, 0x54);
    OV5642_write_cmos_sensor(0x8709, 0x07);
    OV5642_write_cmos_sensor(0x870a, 0xf5);
    OV5642_write_cmos_sensor(0x870b, 0x10);
    OV5642_write_cmos_sensor(0x870c, 0x8f);
    OV5642_write_cmos_sensor(0x870d, 0x11);
    OV5642_write_cmos_sensor(0x870e, 0xe5);
    OV5642_write_cmos_sensor(0x870f, 0x0e);
    OV5642_write_cmos_sensor(0x8710, 0xc4);
    OV5642_write_cmos_sensor(0x8711, 0xf8);
    OV5642_write_cmos_sensor(0x8712, 0x54);
    OV5642_write_cmos_sensor(0x8713, 0xf0);
    OV5642_write_cmos_sensor(0x8714, 0xc8);
    OV5642_write_cmos_sensor(0x8715, 0x68);
    OV5642_write_cmos_sensor(0x8716, 0xf5);
    OV5642_write_cmos_sensor(0x8717, 0x0e);
    OV5642_write_cmos_sensor(0x8718, 0xe5);
    OV5642_write_cmos_sensor(0x8719, 0x0f);
    OV5642_write_cmos_sensor(0x871a, 0xc4);
    OV5642_write_cmos_sensor(0x871b, 0x54);
    OV5642_write_cmos_sensor(0x871c, 0x0f);
    OV5642_write_cmos_sensor(0x871d, 0x48);
    OV5642_write_cmos_sensor(0x871e, 0xf5);
    OV5642_write_cmos_sensor(0x871f, 0x0f);
    OV5642_write_cmos_sensor(0x8720, 0xe5);
    OV5642_write_cmos_sensor(0x8721, 0x10);
    OV5642_write_cmos_sensor(0x8722, 0xc4);
    OV5642_write_cmos_sensor(0x8723, 0xf8);
    OV5642_write_cmos_sensor(0x8724, 0x54);
    OV5642_write_cmos_sensor(0x8725, 0xf0);
    OV5642_write_cmos_sensor(0x8726, 0xc8);
    OV5642_write_cmos_sensor(0x8727, 0x68);
    OV5642_write_cmos_sensor(0x8728, 0xf5);
    OV5642_write_cmos_sensor(0x8729, 0x10);
    OV5642_write_cmos_sensor(0x872a, 0xe5);
    OV5642_write_cmos_sensor(0x872b, 0x11);
    OV5642_write_cmos_sensor(0x872c, 0xc4);
    OV5642_write_cmos_sensor(0x872d, 0x54);
    OV5642_write_cmos_sensor(0x872e, 0x0f);
    OV5642_write_cmos_sensor(0x872f, 0x48);
    OV5642_write_cmos_sensor(0x8730, 0xf5);
    OV5642_write_cmos_sensor(0x8731, 0x11);
    OV5642_write_cmos_sensor(0x8732, 0xe4);
    OV5642_write_cmos_sensor(0x8733, 0xf5);
    OV5642_write_cmos_sensor(0x8734, 0x17);
    OV5642_write_cmos_sensor(0x8735, 0x75);
    OV5642_write_cmos_sensor(0x8736, 0x16);
    OV5642_write_cmos_sensor(0x8737, 0x04);
    OV5642_write_cmos_sensor(0x8738, 0x12);
    OV5642_write_cmos_sensor(0x8739, 0x02);
    OV5642_write_cmos_sensor(0x873a, 0x60);
    OV5642_write_cmos_sensor(0x873b, 0x24);
    OV5642_write_cmos_sensor(0x873c, 0x34);
    OV5642_write_cmos_sensor(0x873d, 0xf8);
    OV5642_write_cmos_sensor(0x873e, 0xe6);
    OV5642_write_cmos_sensor(0x873f, 0xf5);
    OV5642_write_cmos_sensor(0x8740, 0x12);
    OV5642_write_cmos_sensor(0x8741, 0x12);
    OV5642_write_cmos_sensor(0x8742, 0x02);
    OV5642_write_cmos_sensor(0x8743, 0x60);
    OV5642_write_cmos_sensor(0x8744, 0x24);
    OV5642_write_cmos_sensor(0x8745, 0x35);
    OV5642_write_cmos_sensor(0x8746, 0xf8);
    OV5642_write_cmos_sensor(0x8747, 0xe6);
    OV5642_write_cmos_sensor(0x8748, 0xf5);
    OV5642_write_cmos_sensor(0x8749, 0x14);
    OV5642_write_cmos_sensor(0x874a, 0x12);
    OV5642_write_cmos_sensor(0x874b, 0x02);
    OV5642_write_cmos_sensor(0x874c, 0x60);
    OV5642_write_cmos_sensor(0x874d, 0x24);
    OV5642_write_cmos_sensor(0x874e, 0x36);
    OV5642_write_cmos_sensor(0x874f, 0xf8);
    OV5642_write_cmos_sensor(0x8750, 0xe6);
    OV5642_write_cmos_sensor(0x8751, 0xf5);
    OV5642_write_cmos_sensor(0x8752, 0x13);
    OV5642_write_cmos_sensor(0x8753, 0x12);
    OV5642_write_cmos_sensor(0x8754, 0x02);
    OV5642_write_cmos_sensor(0x8755, 0x60);
    OV5642_write_cmos_sensor(0x8756, 0x24);
    OV5642_write_cmos_sensor(0x8757, 0x37);
    OV5642_write_cmos_sensor(0x8758, 0xf8);
    OV5642_write_cmos_sensor(0x8759, 0xe6);
    OV5642_write_cmos_sensor(0x875a, 0xf5);
    OV5642_write_cmos_sensor(0x875b, 0x15);
    OV5642_write_cmos_sensor(0x875c, 0x12);
    OV5642_write_cmos_sensor(0x875d, 0x02);
    OV5642_write_cmos_sensor(0x875e, 0x73);
    OV5642_write_cmos_sensor(0x875f, 0xaf);
    OV5642_write_cmos_sensor(0x8760, 0x12);
    OV5642_write_cmos_sensor(0x8761, 0x12);
    OV5642_write_cmos_sensor(0x8762, 0x01);
    OV5642_write_cmos_sensor(0x8763, 0x42);
    OV5642_write_cmos_sensor(0x8764, 0x8f);
    OV5642_write_cmos_sensor(0x8765, 0x12);
    OV5642_write_cmos_sensor(0x8766, 0x12);
    OV5642_write_cmos_sensor(0x8767, 0x02);
    OV5642_write_cmos_sensor(0x8768, 0x73);
    OV5642_write_cmos_sensor(0x8769, 0xaf);
    OV5642_write_cmos_sensor(0x876a, 0x13);
    OV5642_write_cmos_sensor(0x876b, 0x12);
    OV5642_write_cmos_sensor(0x876c, 0x01);
    OV5642_write_cmos_sensor(0x876d, 0x42);
    OV5642_write_cmos_sensor(0x876e, 0x8f);
    OV5642_write_cmos_sensor(0x876f, 0x13);
    OV5642_write_cmos_sensor(0x8770, 0x12);
    OV5642_write_cmos_sensor(0x8771, 0x02);
    OV5642_write_cmos_sensor(0x8772, 0x1b);
    OV5642_write_cmos_sensor(0x8773, 0xaf);
    OV5642_write_cmos_sensor(0x8774, 0x14);
    OV5642_write_cmos_sensor(0x8775, 0xfc);
    OV5642_write_cmos_sensor(0x8776, 0xfd);
    OV5642_write_cmos_sensor(0x8777, 0xfe);
    OV5642_write_cmos_sensor(0x8778, 0x12);
    OV5642_write_cmos_sensor(0x8779, 0x08);
    OV5642_write_cmos_sensor(0x877a, 0x8e);
    OV5642_write_cmos_sensor(0x877b, 0x12);
    OV5642_write_cmos_sensor(0x877c, 0x01);
    OV5642_write_cmos_sensor(0x877d, 0x61);
    OV5642_write_cmos_sensor(0x877e, 0x7b);
    OV5642_write_cmos_sensor(0x877f, 0x30);
    OV5642_write_cmos_sensor(0x8780, 0x12);
    OV5642_write_cmos_sensor(0x8781, 0x01);
    OV5642_write_cmos_sensor(0x8782, 0x5a);
    OV5642_write_cmos_sensor(0x8783, 0x8f);
    OV5642_write_cmos_sensor(0x8784, 0x14);
    OV5642_write_cmos_sensor(0x8785, 0x12);
    OV5642_write_cmos_sensor(0x8786, 0x02);
    OV5642_write_cmos_sensor(0x8787, 0x1b);
    OV5642_write_cmos_sensor(0x8788, 0xaf);
    OV5642_write_cmos_sensor(0x8789, 0x15);
    OV5642_write_cmos_sensor(0x878a, 0xfc);
    OV5642_write_cmos_sensor(0x878b, 0xfd);
    OV5642_write_cmos_sensor(0x878c, 0xfe);
    OV5642_write_cmos_sensor(0x878d, 0x12);
    OV5642_write_cmos_sensor(0x878e, 0x08);
    OV5642_write_cmos_sensor(0x878f, 0x8e);
    OV5642_write_cmos_sensor(0x8790, 0x12);
    OV5642_write_cmos_sensor(0x8791, 0x01);
    OV5642_write_cmos_sensor(0x8792, 0x61);
    OV5642_write_cmos_sensor(0x8793, 0xe4);
    OV5642_write_cmos_sensor(0x8794, 0x7b);
    OV5642_write_cmos_sensor(0x8795, 0x30);
    OV5642_write_cmos_sensor(0x8796, 0x12);
    OV5642_write_cmos_sensor(0x8797, 0x01);
    OV5642_write_cmos_sensor(0x8798, 0x5b);
    OV5642_write_cmos_sensor(0x8799, 0x8f);
    OV5642_write_cmos_sensor(0x879a, 0x15);
    OV5642_write_cmos_sensor(0x879b, 0xc3);
    OV5642_write_cmos_sensor(0x879c, 0xe5);
    OV5642_write_cmos_sensor(0x879d, 0x13);
    OV5642_write_cmos_sensor(0x879e, 0x95);
    OV5642_write_cmos_sensor(0x879f, 0x12);
    OV5642_write_cmos_sensor(0x87a0, 0xff);
    OV5642_write_cmos_sensor(0x87a1, 0x0f);
    OV5642_write_cmos_sensor(0x87a2, 0xef);
    OV5642_write_cmos_sensor(0x87a3, 0xc3);
    OV5642_write_cmos_sensor(0x87a4, 0x13);
    OV5642_write_cmos_sensor(0x87a5, 0xff);
    OV5642_write_cmos_sensor(0x87a6, 0xc3);
    OV5642_write_cmos_sensor(0x87a7, 0x94);
    OV5642_write_cmos_sensor(0x87a8, 0x04);
    OV5642_write_cmos_sensor(0x87a9, 0x50);
    OV5642_write_cmos_sensor(0x87aa, 0x27);
    OV5642_write_cmos_sensor(0x87ab, 0xe5);
    OV5642_write_cmos_sensor(0x87ac, 0x12);
    OV5642_write_cmos_sensor(0x87ad, 0x9f);
    OV5642_write_cmos_sensor(0x87ae, 0x40);
    OV5642_write_cmos_sensor(0x87af, 0x06);
    OV5642_write_cmos_sensor(0x87b0, 0xe5);
    OV5642_write_cmos_sensor(0x87b1, 0x12);
    OV5642_write_cmos_sensor(0x87b2, 0x9f);
    OV5642_write_cmos_sensor(0x87b3, 0xfe);
    OV5642_write_cmos_sensor(0x87b4, 0x80);
    OV5642_write_cmos_sensor(0x87b5, 0x02);
    OV5642_write_cmos_sensor(0x87b6, 0x7e);
    OV5642_write_cmos_sensor(0x87b7, 0x00);
    OV5642_write_cmos_sensor(0x87b8, 0x8e);
    OV5642_write_cmos_sensor(0x87b9, 0x12);
    OV5642_write_cmos_sensor(0x87ba, 0xef);
    OV5642_write_cmos_sensor(0x87bb, 0xfd);
    OV5642_write_cmos_sensor(0x87bc, 0xe5);
    OV5642_write_cmos_sensor(0x87bd, 0x13);
    OV5642_write_cmos_sensor(0x87be, 0x2d);
    OV5642_write_cmos_sensor(0x87bf, 0xfd);
    OV5642_write_cmos_sensor(0x87c0, 0xe4);
    OV5642_write_cmos_sensor(0x87c1, 0x33);
    OV5642_write_cmos_sensor(0x87c2, 0xfc);
    OV5642_write_cmos_sensor(0x87c3, 0xc3);
    OV5642_write_cmos_sensor(0x87c4, 0xed);
    OV5642_write_cmos_sensor(0x87c5, 0x95);
    OV5642_write_cmos_sensor(0x87c6, 0x0f);
    OV5642_write_cmos_sensor(0x87c7, 0xec);
    OV5642_write_cmos_sensor(0x87c8, 0x95);
    OV5642_write_cmos_sensor(0x87c9, 0x0e);
    OV5642_write_cmos_sensor(0x87ca, 0x50);
    OV5642_write_cmos_sensor(0x87cb, 0x02);
    OV5642_write_cmos_sensor(0x87cc, 0x80);
    OV5642_write_cmos_sensor(0x87cd, 0x02);
    OV5642_write_cmos_sensor(0x87ce, 0xad);
    OV5642_write_cmos_sensor(0x87cf, 0x0f);
    OV5642_write_cmos_sensor(0x87d0, 0x8d);
    OV5642_write_cmos_sensor(0x87d1, 0x13);
    OV5642_write_cmos_sensor(0x87d2, 0xc3);
    OV5642_write_cmos_sensor(0x87d3, 0xe5);
    OV5642_write_cmos_sensor(0x87d4, 0x15);
    OV5642_write_cmos_sensor(0x87d5, 0x95);
    OV5642_write_cmos_sensor(0x87d6, 0x14);
    OV5642_write_cmos_sensor(0x87d7, 0xff);
    OV5642_write_cmos_sensor(0x87d8, 0xc3);
    OV5642_write_cmos_sensor(0x87d9, 0x94);
    OV5642_write_cmos_sensor(0x87da, 0x04);
    OV5642_write_cmos_sensor(0x87db, 0x50);
    OV5642_write_cmos_sensor(0x87dc, 0x29);
    OV5642_write_cmos_sensor(0x87dd, 0xe5);
    OV5642_write_cmos_sensor(0x87de, 0x14);
    OV5642_write_cmos_sensor(0x87df, 0x9f);
    OV5642_write_cmos_sensor(0x87e0, 0x40);
    OV5642_write_cmos_sensor(0x87e1, 0x06);
    OV5642_write_cmos_sensor(0x87e2, 0xe5);
    OV5642_write_cmos_sensor(0x87e3, 0x14);
    OV5642_write_cmos_sensor(0x87e4, 0x9f);
    OV5642_write_cmos_sensor(0x87e5, 0xfe);
    OV5642_write_cmos_sensor(0x87e6, 0x80);
    OV5642_write_cmos_sensor(0x87e7, 0x02);
    OV5642_write_cmos_sensor(0x87e8, 0x7e);
    OV5642_write_cmos_sensor(0x87e9, 0x00);
    OV5642_write_cmos_sensor(0x87ea, 0x8e);
    OV5642_write_cmos_sensor(0x87eb, 0x14);
    OV5642_write_cmos_sensor(0x87ec, 0xef);
    OV5642_write_cmos_sensor(0x87ed, 0xfd);
    OV5642_write_cmos_sensor(0x87ee, 0xe5);
    OV5642_write_cmos_sensor(0x87ef, 0x15);
    OV5642_write_cmos_sensor(0x87f0, 0x2d);
    OV5642_write_cmos_sensor(0x87f1, 0xfd);
    OV5642_write_cmos_sensor(0x87f2, 0xe4);
    OV5642_write_cmos_sensor(0x87f3, 0x33);
    OV5642_write_cmos_sensor(0x87f4, 0xfc);
    OV5642_write_cmos_sensor(0x87f5, 0xc3);
    OV5642_write_cmos_sensor(0x87f6, 0xed);
    OV5642_write_cmos_sensor(0x87f7, 0x95);
    OV5642_write_cmos_sensor(0x87f8, 0x11);
    OV5642_write_cmos_sensor(0x87f9, 0xec);
    OV5642_write_cmos_sensor(0x87fa, 0x95);
    OV5642_write_cmos_sensor(0x87fb, 0x10);
    OV5642_write_cmos_sensor(0x87fc, 0x50);
    OV5642_write_cmos_sensor(0x87fd, 0x04);
    OV5642_write_cmos_sensor(0x87fe, 0xaf);
    OV5642_write_cmos_sensor(0x87ff, 0x05);
    OV5642_write_cmos_sensor(0x8800, 0x80);
    OV5642_write_cmos_sensor(0x8801, 0x02);
    OV5642_write_cmos_sensor(0x8802, 0xaf);
    OV5642_write_cmos_sensor(0x8803, 0x11);
    OV5642_write_cmos_sensor(0x8804, 0x8f);
    OV5642_write_cmos_sensor(0x8805, 0x15);
    OV5642_write_cmos_sensor(0x8806, 0xe5);
    OV5642_write_cmos_sensor(0x8807, 0x15);
    OV5642_write_cmos_sensor(0x8808, 0xd3);
    OV5642_write_cmos_sensor(0x8809, 0x95);
    OV5642_write_cmos_sensor(0x880a, 0x17);
    OV5642_write_cmos_sensor(0x880b, 0x40);
    OV5642_write_cmos_sensor(0x880c, 0x04);
    OV5642_write_cmos_sensor(0x880d, 0xaf);
    OV5642_write_cmos_sensor(0x880e, 0x15);
    OV5642_write_cmos_sensor(0x880f, 0x80);
    OV5642_write_cmos_sensor(0x8810, 0x02);
    OV5642_write_cmos_sensor(0x8811, 0xaf);
    OV5642_write_cmos_sensor(0x8812, 0x17);
    OV5642_write_cmos_sensor(0x8813, 0x8f);
    OV5642_write_cmos_sensor(0x8814, 0x17);
    OV5642_write_cmos_sensor(0x8815, 0xd3);
    OV5642_write_cmos_sensor(0x8816, 0xe5);
    OV5642_write_cmos_sensor(0x8817, 0x16);
    OV5642_write_cmos_sensor(0x8818, 0x64);
    OV5642_write_cmos_sensor(0x8819, 0x80);
    OV5642_write_cmos_sensor(0x881a, 0x94);
    OV5642_write_cmos_sensor(0x881b, 0x80);
    OV5642_write_cmos_sensor(0x881c, 0x40);
    OV5642_write_cmos_sensor(0x881d, 0x04);
    OV5642_write_cmos_sensor(0x881e, 0xaf);
    OV5642_write_cmos_sensor(0x881f, 0x15);
    OV5642_write_cmos_sensor(0x8820, 0x80);
    OV5642_write_cmos_sensor(0x8821, 0x02);
    OV5642_write_cmos_sensor(0x8822, 0xaf);
    OV5642_write_cmos_sensor(0x8823, 0x17);
    OV5642_write_cmos_sensor(0x8824, 0x8f);
    OV5642_write_cmos_sensor(0x8825, 0x15);
    OV5642_write_cmos_sensor(0x8826, 0xe5);
    OV5642_write_cmos_sensor(0x8827, 0x16);
    OV5642_write_cmos_sensor(0x8828, 0xfd);
    OV5642_write_cmos_sensor(0x8829, 0x33);
    OV5642_write_cmos_sensor(0x882a, 0x95);
    OV5642_write_cmos_sensor(0x882b, 0xe0);
    OV5642_write_cmos_sensor(0x882c, 0xfc);
    OV5642_write_cmos_sensor(0x882d, 0xed);
    OV5642_write_cmos_sensor(0x882e, 0xae);
    OV5642_write_cmos_sensor(0x882f, 0x04);
    OV5642_write_cmos_sensor(0x8830, 0x78);
    OV5642_write_cmos_sensor(0x8831, 0x02);
    OV5642_write_cmos_sensor(0x8832, 0xc3);
    OV5642_write_cmos_sensor(0x8833, 0x33);
    OV5642_write_cmos_sensor(0x8834, 0xce);
    OV5642_write_cmos_sensor(0x8835, 0x33);
    OV5642_write_cmos_sensor(0x8836, 0xce);
    OV5642_write_cmos_sensor(0x8837, 0xd8);
    OV5642_write_cmos_sensor(0x8838, 0xf9);
    OV5642_write_cmos_sensor(0x8839, 0xff);
    OV5642_write_cmos_sensor(0x883a, 0x24);
    OV5642_write_cmos_sensor(0x883b, 0x01);
    OV5642_write_cmos_sensor(0x883c, 0xfb);
    OV5642_write_cmos_sensor(0x883d, 0xee);
    OV5642_write_cmos_sensor(0x883e, 0x34);
    OV5642_write_cmos_sensor(0x883f, 0x60);
    OV5642_write_cmos_sensor(0x8840, 0x8b);
    OV5642_write_cmos_sensor(0x8841, 0x82);
    OV5642_write_cmos_sensor(0x8842, 0xf5);
    OV5642_write_cmos_sensor(0x8843, 0x83);
    OV5642_write_cmos_sensor(0x8844, 0xe5);
    OV5642_write_cmos_sensor(0x8845, 0x12);
    OV5642_write_cmos_sensor(0x8846, 0xf0);
    OV5642_write_cmos_sensor(0x8847, 0xef);
    OV5642_write_cmos_sensor(0x8848, 0x24);
    OV5642_write_cmos_sensor(0x8849, 0x02);
    OV5642_write_cmos_sensor(0x884a, 0xff);
    OV5642_write_cmos_sensor(0x884b, 0xee);
    OV5642_write_cmos_sensor(0x884c, 0x34);
    OV5642_write_cmos_sensor(0x884d, 0x60);
    OV5642_write_cmos_sensor(0x884e, 0x8f);
    OV5642_write_cmos_sensor(0x884f, 0x82);
    OV5642_write_cmos_sensor(0x8850, 0xf5);
    OV5642_write_cmos_sensor(0x8851, 0x83);
    OV5642_write_cmos_sensor(0x8852, 0xe5);
    OV5642_write_cmos_sensor(0x8853, 0x14);
    OV5642_write_cmos_sensor(0x8854, 0xf0);
    OV5642_write_cmos_sensor(0x8855, 0xed);
    OV5642_write_cmos_sensor(0x8856, 0xae);
    OV5642_write_cmos_sensor(0x8857, 0x04);
    OV5642_write_cmos_sensor(0x8858, 0x78);
    OV5642_write_cmos_sensor(0x8859, 0x02);
    OV5642_write_cmos_sensor(0x885a, 0xc3);
    OV5642_write_cmos_sensor(0x885b, 0x33);
    OV5642_write_cmos_sensor(0x885c, 0xce);
    OV5642_write_cmos_sensor(0x885d, 0x33);
    OV5642_write_cmos_sensor(0x885e, 0xce);
    OV5642_write_cmos_sensor(0x885f, 0xd8);
    OV5642_write_cmos_sensor(0x8860, 0xf9);
    OV5642_write_cmos_sensor(0x8861, 0xff);
    OV5642_write_cmos_sensor(0x8862, 0x24);
    OV5642_write_cmos_sensor(0x8863, 0x03);
    OV5642_write_cmos_sensor(0x8864, 0xfd);
    OV5642_write_cmos_sensor(0x8865, 0xee);
    OV5642_write_cmos_sensor(0x8866, 0x34);
    OV5642_write_cmos_sensor(0x8867, 0x60);
    OV5642_write_cmos_sensor(0x8868, 0x8d);
    OV5642_write_cmos_sensor(0x8869, 0x82);
    OV5642_write_cmos_sensor(0x886a, 0xf5);
    OV5642_write_cmos_sensor(0x886b, 0x83);
    OV5642_write_cmos_sensor(0x886c, 0xe5);
    OV5642_write_cmos_sensor(0x886d, 0x13);
    OV5642_write_cmos_sensor(0x886e, 0xf0);
    OV5642_write_cmos_sensor(0x886f, 0xef);
    OV5642_write_cmos_sensor(0x8870, 0x24);
    OV5642_write_cmos_sensor(0x8871, 0x04);
    OV5642_write_cmos_sensor(0x8872, 0xff);
    OV5642_write_cmos_sensor(0x8873, 0xee);
    OV5642_write_cmos_sensor(0x8874, 0x34);
    OV5642_write_cmos_sensor(0x8875, 0x60);
    OV5642_write_cmos_sensor(0x8876, 0x8f);
    OV5642_write_cmos_sensor(0x8877, 0x82);
    OV5642_write_cmos_sensor(0x8878, 0xf5);
    OV5642_write_cmos_sensor(0x8879, 0x83);
    OV5642_write_cmos_sensor(0x887a, 0xe5);
    OV5642_write_cmos_sensor(0x887b, 0x15);
    OV5642_write_cmos_sensor(0x887c, 0xf0);
    OV5642_write_cmos_sensor(0x887d, 0x15);
    OV5642_write_cmos_sensor(0x887e, 0x16);
    OV5642_write_cmos_sensor(0x887f, 0xc3);
    OV5642_write_cmos_sensor(0x8880, 0xe5);
    OV5642_write_cmos_sensor(0x8881, 0x16);
    OV5642_write_cmos_sensor(0x8882, 0x64);
    OV5642_write_cmos_sensor(0x8883, 0x80);
    OV5642_write_cmos_sensor(0x8884, 0x94);
    OV5642_write_cmos_sensor(0x8885, 0x80);
    OV5642_write_cmos_sensor(0x8886, 0x40);
    OV5642_write_cmos_sensor(0x8887, 0x03);
    OV5642_write_cmos_sensor(0x8888, 0x02);
    OV5642_write_cmos_sensor(0x8889, 0x07);
    OV5642_write_cmos_sensor(0x888a, 0x38);
    OV5642_write_cmos_sensor(0x888b, 0xc2);
    OV5642_write_cmos_sensor(0x888c, 0x30);
    OV5642_write_cmos_sensor(0x888d, 0x22);
    OV5642_write_cmos_sensor(0x888e, 0xe8);
    OV5642_write_cmos_sensor(0x888f, 0x8f);
    OV5642_write_cmos_sensor(0x8890, 0xf0);
    OV5642_write_cmos_sensor(0x8891, 0xa4);
    OV5642_write_cmos_sensor(0x8892, 0xcc);
    OV5642_write_cmos_sensor(0x8893, 0x8b);
    OV5642_write_cmos_sensor(0x8894, 0xf0);
    OV5642_write_cmos_sensor(0x8895, 0xa4);
    OV5642_write_cmos_sensor(0x8896, 0x2c);
    OV5642_write_cmos_sensor(0x8897, 0xfc);
    OV5642_write_cmos_sensor(0x8898, 0xe9);
    OV5642_write_cmos_sensor(0x8899, 0x8e);
    OV5642_write_cmos_sensor(0x889a, 0xf0);
    OV5642_write_cmos_sensor(0x889b, 0xa4);
    OV5642_write_cmos_sensor(0x889c, 0x2c);
    OV5642_write_cmos_sensor(0x889d, 0xfc);
    OV5642_write_cmos_sensor(0x889e, 0x8a);
    OV5642_write_cmos_sensor(0x889f, 0xf0);
    OV5642_write_cmos_sensor(0x88a0, 0xed);
    OV5642_write_cmos_sensor(0x88a1, 0xa4);
    OV5642_write_cmos_sensor(0x88a2, 0x2c);
    OV5642_write_cmos_sensor(0x88a3, 0xfc);
    OV5642_write_cmos_sensor(0x88a4, 0xea);
    OV5642_write_cmos_sensor(0x88a5, 0x8e);
    OV5642_write_cmos_sensor(0x88a6, 0xf0);
    OV5642_write_cmos_sensor(0x88a7, 0xa4);
    OV5642_write_cmos_sensor(0x88a8, 0xcd);
    OV5642_write_cmos_sensor(0x88a9, 0xa8);
    OV5642_write_cmos_sensor(0x88aa, 0xf0);
    OV5642_write_cmos_sensor(0x88ab, 0x8b);
    OV5642_write_cmos_sensor(0x88ac, 0xf0);
    OV5642_write_cmos_sensor(0x88ad, 0xa4);
    OV5642_write_cmos_sensor(0x88ae, 0x2d);
    OV5642_write_cmos_sensor(0x88af, 0xcc);
    OV5642_write_cmos_sensor(0x88b0, 0x38);
    OV5642_write_cmos_sensor(0x88b1, 0x25);
    OV5642_write_cmos_sensor(0x88b2, 0xf0);
    OV5642_write_cmos_sensor(0x88b3, 0xfd);
    OV5642_write_cmos_sensor(0x88b4, 0xe9);
    OV5642_write_cmos_sensor(0x88b5, 0x8f);
    OV5642_write_cmos_sensor(0x88b6, 0xf0);
    OV5642_write_cmos_sensor(0x88b7, 0xa4);
    OV5642_write_cmos_sensor(0x88b8, 0x2c);
    OV5642_write_cmos_sensor(0x88b9, 0xcd);
    OV5642_write_cmos_sensor(0x88ba, 0x35);
    OV5642_write_cmos_sensor(0x88bb, 0xf0);
    OV5642_write_cmos_sensor(0x88bc, 0xfc);
    OV5642_write_cmos_sensor(0x88bd, 0xeb);
    OV5642_write_cmos_sensor(0x88be, 0x8e);
    OV5642_write_cmos_sensor(0x88bf, 0xf0);
    OV5642_write_cmos_sensor(0x88c0, 0xa4);
    OV5642_write_cmos_sensor(0x88c1, 0xfe);
    OV5642_write_cmos_sensor(0x88c2, 0xa9);
    OV5642_write_cmos_sensor(0x88c3, 0xf0);
    OV5642_write_cmos_sensor(0x88c4, 0xeb);
    OV5642_write_cmos_sensor(0x88c5, 0x8f);
    OV5642_write_cmos_sensor(0x88c6, 0xf0);
    OV5642_write_cmos_sensor(0x88c7, 0xa4);
    OV5642_write_cmos_sensor(0x88c8, 0xcf);
    OV5642_write_cmos_sensor(0x88c9, 0xc5);
    OV5642_write_cmos_sensor(0x88ca, 0xf0);
    OV5642_write_cmos_sensor(0x88cb, 0x2e);
    OV5642_write_cmos_sensor(0x88cc, 0xcd);
    OV5642_write_cmos_sensor(0x88cd, 0x39);
    OV5642_write_cmos_sensor(0x88ce, 0xfe);
    OV5642_write_cmos_sensor(0x88cf, 0xe4);
    OV5642_write_cmos_sensor(0x88d0, 0x3c);
    OV5642_write_cmos_sensor(0x88d1, 0xfc);
    OV5642_write_cmos_sensor(0x88d2, 0xea);
    OV5642_write_cmos_sensor(0x88d3, 0xa4);
    OV5642_write_cmos_sensor(0x88d4, 0x2d);
    OV5642_write_cmos_sensor(0x88d5, 0xce);
    OV5642_write_cmos_sensor(0x88d6, 0x35);
    OV5642_write_cmos_sensor(0x88d7, 0xf0);
    OV5642_write_cmos_sensor(0x88d8, 0xfd);
    OV5642_write_cmos_sensor(0x88d9, 0xe4);
    OV5642_write_cmos_sensor(0x88da, 0x3c);
    OV5642_write_cmos_sensor(0x88db, 0xfc);
    OV5642_write_cmos_sensor(0x88dc, 0x22);
    OV5642_write_cmos_sensor(0x88dd, 0x75);
    OV5642_write_cmos_sensor(0x88de, 0xf0);
    OV5642_write_cmos_sensor(0x88df, 0x08);
    OV5642_write_cmos_sensor(0x88e0, 0x75);
    OV5642_write_cmos_sensor(0x88e1, 0x82);
    OV5642_write_cmos_sensor(0x88e2, 0x00);
    OV5642_write_cmos_sensor(0x88e3, 0xef);
    OV5642_write_cmos_sensor(0x88e4, 0x2f);
    OV5642_write_cmos_sensor(0x88e5, 0xff);
    OV5642_write_cmos_sensor(0x88e6, 0xee);
    OV5642_write_cmos_sensor(0x88e7, 0x33);
    OV5642_write_cmos_sensor(0x88e8, 0xfe);
    OV5642_write_cmos_sensor(0x88e9, 0xcd);
    OV5642_write_cmos_sensor(0x88ea, 0x33);
    OV5642_write_cmos_sensor(0x88eb, 0xcd);
    OV5642_write_cmos_sensor(0x88ec, 0xcc);
    OV5642_write_cmos_sensor(0x88ed, 0x33);
    OV5642_write_cmos_sensor(0x88ee, 0xcc);
    OV5642_write_cmos_sensor(0x88ef, 0xc5);
    OV5642_write_cmos_sensor(0x88f0, 0x82);
    OV5642_write_cmos_sensor(0x88f1, 0x33);
    OV5642_write_cmos_sensor(0x88f2, 0xc5);
    OV5642_write_cmos_sensor(0x88f3, 0x82);
    OV5642_write_cmos_sensor(0x88f4, 0x9b);
    OV5642_write_cmos_sensor(0x88f5, 0xed);
    OV5642_write_cmos_sensor(0x88f6, 0x9a);
    OV5642_write_cmos_sensor(0x88f7, 0xec);
    OV5642_write_cmos_sensor(0x88f8, 0x99);
    OV5642_write_cmos_sensor(0x88f9, 0xe5);
    OV5642_write_cmos_sensor(0x88fa, 0x82);
    OV5642_write_cmos_sensor(0x88fb, 0x98);
    OV5642_write_cmos_sensor(0x88fc, 0x40);
    OV5642_write_cmos_sensor(0x88fd, 0x0c);
    OV5642_write_cmos_sensor(0x88fe, 0xf5);
    OV5642_write_cmos_sensor(0x88ff, 0x82);
    OV5642_write_cmos_sensor(0x8900, 0xee);
    OV5642_write_cmos_sensor(0x8901, 0x9b);
    OV5642_write_cmos_sensor(0x8902, 0xfe);
    OV5642_write_cmos_sensor(0x8903, 0xed);
    OV5642_write_cmos_sensor(0x8904, 0x9a);
    OV5642_write_cmos_sensor(0x8905, 0xfd);
    OV5642_write_cmos_sensor(0x8906, 0xec);
    OV5642_write_cmos_sensor(0x8907, 0x99);
    OV5642_write_cmos_sensor(0x8908, 0xfc);
    OV5642_write_cmos_sensor(0x8909, 0x0f);
    OV5642_write_cmos_sensor(0x890a, 0xd5);
    OV5642_write_cmos_sensor(0x890b, 0xf0);
    OV5642_write_cmos_sensor(0x890c, 0xd6);
    OV5642_write_cmos_sensor(0x890d, 0xe4);
    OV5642_write_cmos_sensor(0x890e, 0xce);
    OV5642_write_cmos_sensor(0x890f, 0xfb);
    OV5642_write_cmos_sensor(0x8910, 0xe4);
    OV5642_write_cmos_sensor(0x8911, 0xcd);
    OV5642_write_cmos_sensor(0x8912, 0xfa);
    OV5642_write_cmos_sensor(0x8913, 0xe4);
    OV5642_write_cmos_sensor(0x8914, 0xcc);
    OV5642_write_cmos_sensor(0x8915, 0xf9);
    OV5642_write_cmos_sensor(0x8916, 0xa8);
    OV5642_write_cmos_sensor(0x8917, 0x82);
    OV5642_write_cmos_sensor(0x8918, 0x22);
    OV5642_write_cmos_sensor(0x8919, 0xb8);
    OV5642_write_cmos_sensor(0x891a, 0x00);
    OV5642_write_cmos_sensor(0x891b, 0xc1);
    OV5642_write_cmos_sensor(0x891c, 0xb9);
    OV5642_write_cmos_sensor(0x891d, 0x00);
    OV5642_write_cmos_sensor(0x891e, 0x59);
    OV5642_write_cmos_sensor(0x891f, 0xba);
    OV5642_write_cmos_sensor(0x8920, 0x00);
    OV5642_write_cmos_sensor(0x8921, 0x2d);
    OV5642_write_cmos_sensor(0x8922, 0xec);
    OV5642_write_cmos_sensor(0x8923, 0x8b);
    OV5642_write_cmos_sensor(0x8924, 0xf0);
    OV5642_write_cmos_sensor(0x8925, 0x84);
    OV5642_write_cmos_sensor(0x8926, 0xcf);
    OV5642_write_cmos_sensor(0x8927, 0xce);
    OV5642_write_cmos_sensor(0x8928, 0xcd);
    OV5642_write_cmos_sensor(0x8929, 0xfc);
    OV5642_write_cmos_sensor(0x892a, 0xe5);
    OV5642_write_cmos_sensor(0x892b, 0xf0);
    OV5642_write_cmos_sensor(0x892c, 0xcb);
    OV5642_write_cmos_sensor(0x892d, 0xf9);
    OV5642_write_cmos_sensor(0x892e, 0x78);
    OV5642_write_cmos_sensor(0x892f, 0x18);
    OV5642_write_cmos_sensor(0x8930, 0xef);
    OV5642_write_cmos_sensor(0x8931, 0x2f);
    OV5642_write_cmos_sensor(0x8932, 0xff);
    OV5642_write_cmos_sensor(0x8933, 0xee);
    OV5642_write_cmos_sensor(0x8934, 0x33);
    OV5642_write_cmos_sensor(0x8935, 0xfe);
    OV5642_write_cmos_sensor(0x8936, 0xed);
    OV5642_write_cmos_sensor(0x8937, 0x33);
    OV5642_write_cmos_sensor(0x8938, 0xfd);
    OV5642_write_cmos_sensor(0x8939, 0xec);
    OV5642_write_cmos_sensor(0x893a, 0x33);
    OV5642_write_cmos_sensor(0x893b, 0xfc);
    OV5642_write_cmos_sensor(0x893c, 0xeb);
    OV5642_write_cmos_sensor(0x893d, 0x33);
    OV5642_write_cmos_sensor(0x893e, 0xfb);
    OV5642_write_cmos_sensor(0x893f, 0x10);
    OV5642_write_cmos_sensor(0x8940, 0xd7);
    OV5642_write_cmos_sensor(0x8941, 0x03);
    OV5642_write_cmos_sensor(0x8942, 0x99);
    OV5642_write_cmos_sensor(0x8943, 0x40);
    OV5642_write_cmos_sensor(0x8944, 0x04);
    OV5642_write_cmos_sensor(0x8945, 0xeb);
    OV5642_write_cmos_sensor(0x8946, 0x99);
    OV5642_write_cmos_sensor(0x8947, 0xfb);
    OV5642_write_cmos_sensor(0x8948, 0x0f);
    OV5642_write_cmos_sensor(0x8949, 0xd8);
    OV5642_write_cmos_sensor(0x894a, 0xe5);
    OV5642_write_cmos_sensor(0x894b, 0xe4);
    OV5642_write_cmos_sensor(0x894c, 0xf9);
    OV5642_write_cmos_sensor(0x894d, 0xfa);
    OV5642_write_cmos_sensor(0x894e, 0x22);
    OV5642_write_cmos_sensor(0x894f, 0x78);
    OV5642_write_cmos_sensor(0x8950, 0x18);
    OV5642_write_cmos_sensor(0x8951, 0xef);
    OV5642_write_cmos_sensor(0x8952, 0x2f);
    OV5642_write_cmos_sensor(0x8953, 0xff);
    OV5642_write_cmos_sensor(0x8954, 0xee);
    OV5642_write_cmos_sensor(0x8955, 0x33);
    OV5642_write_cmos_sensor(0x8956, 0xfe);
    OV5642_write_cmos_sensor(0x8957, 0xed);
    OV5642_write_cmos_sensor(0x8958, 0x33);
    OV5642_write_cmos_sensor(0x8959, 0xfd);
    OV5642_write_cmos_sensor(0x895a, 0xec);
    OV5642_write_cmos_sensor(0x895b, 0x33);
    OV5642_write_cmos_sensor(0x895c, 0xfc);
    OV5642_write_cmos_sensor(0x895d, 0xc9);
    OV5642_write_cmos_sensor(0x895e, 0x33);
    OV5642_write_cmos_sensor(0x895f, 0xc9);
    OV5642_write_cmos_sensor(0x8960, 0x10);
    OV5642_write_cmos_sensor(0x8961, 0xd7);
    OV5642_write_cmos_sensor(0x8962, 0x05);
    OV5642_write_cmos_sensor(0x8963, 0x9b);
    OV5642_write_cmos_sensor(0x8964, 0xe9);
    OV5642_write_cmos_sensor(0x8965, 0x9a);
    OV5642_write_cmos_sensor(0x8966, 0x40);
    OV5642_write_cmos_sensor(0x8967, 0x07);
    OV5642_write_cmos_sensor(0x8968, 0xec);
    OV5642_write_cmos_sensor(0x8969, 0x9b);
    OV5642_write_cmos_sensor(0x896a, 0xfc);
    OV5642_write_cmos_sensor(0x896b, 0xe9);
    OV5642_write_cmos_sensor(0x896c, 0x9a);
    OV5642_write_cmos_sensor(0x896d, 0xf9);
    OV5642_write_cmos_sensor(0x896e, 0x0f);
    OV5642_write_cmos_sensor(0x896f, 0xd8);
    OV5642_write_cmos_sensor(0x8970, 0xe0);
    OV5642_write_cmos_sensor(0x8971, 0xe4);
    OV5642_write_cmos_sensor(0x8972, 0xc9);
    OV5642_write_cmos_sensor(0x8973, 0xfa);
    OV5642_write_cmos_sensor(0x8974, 0xe4);
    OV5642_write_cmos_sensor(0x8975, 0xcc);
    OV5642_write_cmos_sensor(0x8976, 0xfb);
    OV5642_write_cmos_sensor(0x8977, 0x22);
    OV5642_write_cmos_sensor(0x8978, 0x75);
    OV5642_write_cmos_sensor(0x8979, 0xf0);
    OV5642_write_cmos_sensor(0x897a, 0x10);
    OV5642_write_cmos_sensor(0x897b, 0xef);
    OV5642_write_cmos_sensor(0x897c, 0x2f);
    OV5642_write_cmos_sensor(0x897d, 0xff);
    OV5642_write_cmos_sensor(0x897e, 0xee);
    OV5642_write_cmos_sensor(0x897f, 0x33);
    OV5642_write_cmos_sensor(0x8980, 0xfe);
    OV5642_write_cmos_sensor(0x8981, 0xed);
    OV5642_write_cmos_sensor(0x8982, 0x33);
    OV5642_write_cmos_sensor(0x8983, 0xfd);
    OV5642_write_cmos_sensor(0x8984, 0xcc);
    OV5642_write_cmos_sensor(0x8985, 0x33);
    OV5642_write_cmos_sensor(0x8986, 0xcc);
    OV5642_write_cmos_sensor(0x8987, 0xc8);
    OV5642_write_cmos_sensor(0x8988, 0x33);
    OV5642_write_cmos_sensor(0x8989, 0xc8);
    OV5642_write_cmos_sensor(0x898a, 0x10);
    OV5642_write_cmos_sensor(0x898b, 0xd7);
    OV5642_write_cmos_sensor(0x898c, 0x07);
    OV5642_write_cmos_sensor(0x898d, 0x9b);
    OV5642_write_cmos_sensor(0x898e, 0xec);
    OV5642_write_cmos_sensor(0x898f, 0x9a);
    OV5642_write_cmos_sensor(0x8990, 0xe8);
    OV5642_write_cmos_sensor(0x8991, 0x99);
    OV5642_write_cmos_sensor(0x8992, 0x40);
    OV5642_write_cmos_sensor(0x8993, 0x0a);
    OV5642_write_cmos_sensor(0x8994, 0xed);
    OV5642_write_cmos_sensor(0x8995, 0x9b);
    OV5642_write_cmos_sensor(0x8996, 0xfd);
    OV5642_write_cmos_sensor(0x8997, 0xec);
    OV5642_write_cmos_sensor(0x8998, 0x9a);
    OV5642_write_cmos_sensor(0x8999, 0xfc);
    OV5642_write_cmos_sensor(0x899a, 0xe8);
    OV5642_write_cmos_sensor(0x899b, 0x99);
    OV5642_write_cmos_sensor(0x899c, 0xf8);
    OV5642_write_cmos_sensor(0x899d, 0x0f);
    OV5642_write_cmos_sensor(0x899e, 0xd5);
    OV5642_write_cmos_sensor(0x899f, 0xf0);
    OV5642_write_cmos_sensor(0x89a0, 0xda);
    OV5642_write_cmos_sensor(0x89a1, 0xe4);
    OV5642_write_cmos_sensor(0x89a2, 0xcd);
    OV5642_write_cmos_sensor(0x89a3, 0xfb);
    OV5642_write_cmos_sensor(0x89a4, 0xe4);
    OV5642_write_cmos_sensor(0x89a5, 0xcc);
    OV5642_write_cmos_sensor(0x89a6, 0xfa);
    OV5642_write_cmos_sensor(0x89a7, 0xe4);
    OV5642_write_cmos_sensor(0x89a8, 0xc8);
    OV5642_write_cmos_sensor(0x89a9, 0xf9);
    OV5642_write_cmos_sensor(0x89aa, 0x22);
    OV5642_write_cmos_sensor(0x89ab, 0xe8);
    OV5642_write_cmos_sensor(0x89ac, 0x60);
    OV5642_write_cmos_sensor(0x89ad, 0x0f);
    OV5642_write_cmos_sensor(0x89ae, 0xec);
    OV5642_write_cmos_sensor(0x89af, 0xc3);
    OV5642_write_cmos_sensor(0x89b0, 0x13);
    OV5642_write_cmos_sensor(0x89b1, 0xfc);
    OV5642_write_cmos_sensor(0x89b2, 0xed);
    OV5642_write_cmos_sensor(0x89b3, 0x13);
    OV5642_write_cmos_sensor(0x89b4, 0xfd);
    OV5642_write_cmos_sensor(0x89b5, 0xee);
    OV5642_write_cmos_sensor(0x89b6, 0x13);
    OV5642_write_cmos_sensor(0x89b7, 0xfe);
    OV5642_write_cmos_sensor(0x89b8, 0xef);
    OV5642_write_cmos_sensor(0x89b9, 0x13);
    OV5642_write_cmos_sensor(0x89ba, 0xff);
    OV5642_write_cmos_sensor(0x89bb, 0xd8);
    OV5642_write_cmos_sensor(0x89bc, 0xf1);
    OV5642_write_cmos_sensor(0x89bd, 0x22);
    OV5642_write_cmos_sensor(0x89be, 0xe8);
    OV5642_write_cmos_sensor(0x89bf, 0x60);
    OV5642_write_cmos_sensor(0x89c0, 0x0f);
    OV5642_write_cmos_sensor(0x89c1, 0xef);
    OV5642_write_cmos_sensor(0x89c2, 0xc3);
    OV5642_write_cmos_sensor(0x89c3, 0x33);
    OV5642_write_cmos_sensor(0x89c4, 0xff);
    OV5642_write_cmos_sensor(0x89c5, 0xee);
    OV5642_write_cmos_sensor(0x89c6, 0x33);
    OV5642_write_cmos_sensor(0x89c7, 0xfe);
    OV5642_write_cmos_sensor(0x89c8, 0xed);
    OV5642_write_cmos_sensor(0x89c9, 0x33);
    OV5642_write_cmos_sensor(0x89ca, 0xfd);
    OV5642_write_cmos_sensor(0x89cb, 0xec);
    OV5642_write_cmos_sensor(0x89cc, 0x33);
    OV5642_write_cmos_sensor(0x89cd, 0xfc);
    OV5642_write_cmos_sensor(0x89ce, 0xd8);
    OV5642_write_cmos_sensor(0x89cf, 0xf1);
    OV5642_write_cmos_sensor(0x89d0, 0x22);
    OV5642_write_cmos_sensor(0x89d1, 0xe4);
    OV5642_write_cmos_sensor(0x89d2, 0x93);
    OV5642_write_cmos_sensor(0x89d3, 0xfc);
    OV5642_write_cmos_sensor(0x89d4, 0x74);
    OV5642_write_cmos_sensor(0x89d5, 0x01);
    OV5642_write_cmos_sensor(0x89d6, 0x93);
    OV5642_write_cmos_sensor(0x89d7, 0xfd);
    OV5642_write_cmos_sensor(0x89d8, 0x74);
    OV5642_write_cmos_sensor(0x89d9, 0x02);
    OV5642_write_cmos_sensor(0x89da, 0x93);
    OV5642_write_cmos_sensor(0x89db, 0xfe);
    OV5642_write_cmos_sensor(0x89dc, 0x74);
    OV5642_write_cmos_sensor(0x89dd, 0x03);
    OV5642_write_cmos_sensor(0x89de, 0x93);
    OV5642_write_cmos_sensor(0x89df, 0xff);
    OV5642_write_cmos_sensor(0x89e0, 0x22);
    OV5642_write_cmos_sensor(0x89e1, 0xa4);
    OV5642_write_cmos_sensor(0x89e2, 0x25);
    OV5642_write_cmos_sensor(0x89e3, 0x82);
    OV5642_write_cmos_sensor(0x89e4, 0xf5);
    OV5642_write_cmos_sensor(0x89e5, 0x82);
    OV5642_write_cmos_sensor(0x89e6, 0xe5);
    OV5642_write_cmos_sensor(0x89e7, 0xf0);
    OV5642_write_cmos_sensor(0x89e8, 0x35);
    OV5642_write_cmos_sensor(0x89e9, 0x83);
    OV5642_write_cmos_sensor(0x89ea, 0xf5);
    OV5642_write_cmos_sensor(0x89eb, 0x83);
    OV5642_write_cmos_sensor(0x89ec, 0x22);
    OV5642_write_cmos_sensor(0x89ed, 0xd0);
    OV5642_write_cmos_sensor(0x89ee, 0x83);
    OV5642_write_cmos_sensor(0x89ef, 0xd0);
    OV5642_write_cmos_sensor(0x89f0, 0x82);
    OV5642_write_cmos_sensor(0x89f1, 0xf8);
    OV5642_write_cmos_sensor(0x89f2, 0xe4);
    OV5642_write_cmos_sensor(0x89f3, 0x93);
    OV5642_write_cmos_sensor(0x89f4, 0x70);
    OV5642_write_cmos_sensor(0x89f5, 0x12);
    OV5642_write_cmos_sensor(0x89f6, 0x74);
    OV5642_write_cmos_sensor(0x89f7, 0x01);
    OV5642_write_cmos_sensor(0x89f8, 0x93);
    OV5642_write_cmos_sensor(0x89f9, 0x70);
    OV5642_write_cmos_sensor(0x89fa, 0x0d);
    OV5642_write_cmos_sensor(0x89fb, 0xa3);
    OV5642_write_cmos_sensor(0x89fc, 0xa3);
    OV5642_write_cmos_sensor(0x89fd, 0x93);
    OV5642_write_cmos_sensor(0x89fe, 0xf8);
    OV5642_write_cmos_sensor(0x89ff, 0x74);
    OV5642_write_cmos_sensor(0x8a00, 0x01);
    OV5642_write_cmos_sensor(0x8a01, 0x93);
    OV5642_write_cmos_sensor(0x8a02, 0xf5);
    OV5642_write_cmos_sensor(0x8a03, 0x82);
    OV5642_write_cmos_sensor(0x8a04, 0x88);
    OV5642_write_cmos_sensor(0x8a05, 0x83);
    OV5642_write_cmos_sensor(0x8a06, 0xe4);
    OV5642_write_cmos_sensor(0x8a07, 0x73);
    OV5642_write_cmos_sensor(0x8a08, 0x74);
    OV5642_write_cmos_sensor(0x8a09, 0x02);
    OV5642_write_cmos_sensor(0x8a0a, 0x93);
    OV5642_write_cmos_sensor(0x8a0b, 0x68);
    OV5642_write_cmos_sensor(0x8a0c, 0x60);
    OV5642_write_cmos_sensor(0x8a0d, 0xef);
    OV5642_write_cmos_sensor(0x8a0e, 0xa3);
    OV5642_write_cmos_sensor(0x8a0f, 0xa3);
    OV5642_write_cmos_sensor(0x8a10, 0xa3);
    OV5642_write_cmos_sensor(0x8a11, 0x80);
    OV5642_write_cmos_sensor(0x8a12, 0xdf);
    OV5642_write_cmos_sensor(0x8a13, 0x75);
    OV5642_write_cmos_sensor(0x8a14, 0x0f);
    OV5642_write_cmos_sensor(0x8a15, 0x0a);
    OV5642_write_cmos_sensor(0x8a16, 0xa2);
    OV5642_write_cmos_sensor(0x8a17, 0xaf);
    OV5642_write_cmos_sensor(0x8a18, 0x92);
    OV5642_write_cmos_sensor(0x8a19, 0x32);
    OV5642_write_cmos_sensor(0x8a1a, 0xc2);
    OV5642_write_cmos_sensor(0x8a1b, 0xaf);
    OV5642_write_cmos_sensor(0x8a1c, 0xc2);
    OV5642_write_cmos_sensor(0x8a1d, 0x33);
    OV5642_write_cmos_sensor(0x8a1e, 0x12);
    OV5642_write_cmos_sensor(0x8a1f, 0x01);
    OV5642_write_cmos_sensor(0x8a20, 0x6a);
    OV5642_write_cmos_sensor(0x8a21, 0x12);
    OV5642_write_cmos_sensor(0x8a22, 0x02);
    OV5642_write_cmos_sensor(0x8a23, 0x12);
    OV5642_write_cmos_sensor(0x8a24, 0x12);
    OV5642_write_cmos_sensor(0x8a25, 0x01);
    OV5642_write_cmos_sensor(0x8a26, 0x6a);
    OV5642_write_cmos_sensor(0x8a27, 0x75);
    OV5642_write_cmos_sensor(0x8a28, 0x51);
    OV5642_write_cmos_sensor(0x8a29, 0x05);
    OV5642_write_cmos_sensor(0x8a2a, 0xaf);
    OV5642_write_cmos_sensor(0x8a2b, 0x51);
    OV5642_write_cmos_sensor(0x8a2c, 0x15);
    OV5642_write_cmos_sensor(0x8a2d, 0x51);
    OV5642_write_cmos_sensor(0x8a2e, 0xef);
    OV5642_write_cmos_sensor(0x8a2f, 0x70);
    OV5642_write_cmos_sensor(0x8a30, 0xf9);
    OV5642_write_cmos_sensor(0x8a31, 0xd2);
    OV5642_write_cmos_sensor(0x8a32, 0x28);
    OV5642_write_cmos_sensor(0x8a33, 0x12);
    OV5642_write_cmos_sensor(0x8a34, 0x01);
    OV5642_write_cmos_sensor(0x8a35, 0x6c);
    OV5642_write_cmos_sensor(0x8a36, 0x75);
    OV5642_write_cmos_sensor(0x8a37, 0x51);
    OV5642_write_cmos_sensor(0x8a38, 0x0a);
    OV5642_write_cmos_sensor(0x8a39, 0xaf);
    OV5642_write_cmos_sensor(0x8a3a, 0x51);
    OV5642_write_cmos_sensor(0x8a3b, 0x15);
    OV5642_write_cmos_sensor(0x8a3c, 0x51);
    OV5642_write_cmos_sensor(0x8a3d, 0xef);
    OV5642_write_cmos_sensor(0x8a3e, 0x70);
    OV5642_write_cmos_sensor(0x8a3f, 0xf9);
    OV5642_write_cmos_sensor(0x8a40, 0xc2);
    OV5642_write_cmos_sensor(0x8a41, 0x29);
    OV5642_write_cmos_sensor(0x8a42, 0x12);
    OV5642_write_cmos_sensor(0x8a43, 0x01);
    OV5642_write_cmos_sensor(0x8a44, 0x6c);
    OV5642_write_cmos_sensor(0x8a45, 0x75);
    OV5642_write_cmos_sensor(0x8a46, 0x51);
    OV5642_write_cmos_sensor(0x8a47, 0x05);
    OV5642_write_cmos_sensor(0x8a48, 0xaf);
    OV5642_write_cmos_sensor(0x8a49, 0x51);
    OV5642_write_cmos_sensor(0x8a4a, 0x15);
    OV5642_write_cmos_sensor(0x8a4b, 0x51);
    OV5642_write_cmos_sensor(0x8a4c, 0xef);
    OV5642_write_cmos_sensor(0x8a4d, 0x70);
    OV5642_write_cmos_sensor(0x8a4e, 0xf9);
    OV5642_write_cmos_sensor(0x8a4f, 0xc2);
    OV5642_write_cmos_sensor(0x8a50, 0x28);
    OV5642_write_cmos_sensor(0x8a51, 0x12);
    OV5642_write_cmos_sensor(0x8a52, 0x01);
    OV5642_write_cmos_sensor(0x8a53, 0x6c);
    OV5642_write_cmos_sensor(0x8a54, 0x75);
    OV5642_write_cmos_sensor(0x8a55, 0x51);
    OV5642_write_cmos_sensor(0x8a56, 0x05);
    OV5642_write_cmos_sensor(0x8a57, 0xaf);
    OV5642_write_cmos_sensor(0x8a58, 0x51);
    OV5642_write_cmos_sensor(0x8a59, 0x15);
    OV5642_write_cmos_sensor(0x8a5a, 0x51);
    OV5642_write_cmos_sensor(0x8a5b, 0xef);
    OV5642_write_cmos_sensor(0x8a5c, 0x70);
    OV5642_write_cmos_sensor(0x8a5d, 0xf9);
    OV5642_write_cmos_sensor(0x8a5e, 0x75);
    OV5642_write_cmos_sensor(0x8a5f, 0x10);
    OV5642_write_cmos_sensor(0x8a60, 0x18);
    OV5642_write_cmos_sensor(0x8a61, 0x12);
    OV5642_write_cmos_sensor(0x8a62, 0x0b);
    OV5642_write_cmos_sensor(0x8a63, 0x53);
    OV5642_write_cmos_sensor(0x8a64, 0x75);
    OV5642_write_cmos_sensor(0x8a65, 0x51);
    OV5642_write_cmos_sensor(0x8a66, 0x0a);
    OV5642_write_cmos_sensor(0x8a67, 0xaf);
    OV5642_write_cmos_sensor(0x8a68, 0x51);
    OV5642_write_cmos_sensor(0x8a69, 0x15);
    OV5642_write_cmos_sensor(0x8a6a, 0x51);
    OV5642_write_cmos_sensor(0x8a6b, 0xef);
    OV5642_write_cmos_sensor(0x8a6c, 0x70);
    OV5642_write_cmos_sensor(0x8a6d, 0xf9);
    OV5642_write_cmos_sensor(0x8a6e, 0xd2);
    OV5642_write_cmos_sensor(0x8a6f, 0x28);
    OV5642_write_cmos_sensor(0x8a70, 0x12);
    OV5642_write_cmos_sensor(0x8a71, 0x01);
    OV5642_write_cmos_sensor(0x8a72, 0x6c);
    OV5642_write_cmos_sensor(0x8a73, 0x12);
    OV5642_write_cmos_sensor(0x8a74, 0x02);
    OV5642_write_cmos_sensor(0x8a75, 0x2f);
    OV5642_write_cmos_sensor(0x8a76, 0xaf);
    OV5642_write_cmos_sensor(0x8a77, 0x51);
    OV5642_write_cmos_sensor(0x8a78, 0x15);
    OV5642_write_cmos_sensor(0x8a79, 0x51);
    OV5642_write_cmos_sensor(0x8a7a, 0xef);
    OV5642_write_cmos_sensor(0x8a7b, 0x70);
    OV5642_write_cmos_sensor(0x8a7c, 0xf9);
    OV5642_write_cmos_sensor(0x8a7d, 0xc2);
    OV5642_write_cmos_sensor(0x8a7e, 0x28);
    OV5642_write_cmos_sensor(0x8a7f, 0x12);
    OV5642_write_cmos_sensor(0x8a80, 0x01);
    OV5642_write_cmos_sensor(0x8a81, 0x6c);
    OV5642_write_cmos_sensor(0x8a82, 0x75);
    OV5642_write_cmos_sensor(0x8a83, 0x51);
    OV5642_write_cmos_sensor(0x8a84, 0x0a);
    OV5642_write_cmos_sensor(0x8a85, 0xaf);
    OV5642_write_cmos_sensor(0x8a86, 0x51);
    OV5642_write_cmos_sensor(0x8a87, 0x15);
    OV5642_write_cmos_sensor(0x8a88, 0x51);
    OV5642_write_cmos_sensor(0x8a89, 0xef);
    OV5642_write_cmos_sensor(0x8a8a, 0x70);
    OV5642_write_cmos_sensor(0x8a8b, 0xf9);
    OV5642_write_cmos_sensor(0x8a8c, 0x30);
    OV5642_write_cmos_sensor(0x8a8d, 0x11);
    OV5642_write_cmos_sensor(0x8a8e, 0x03);
    OV5642_write_cmos_sensor(0x8a8f, 0x02);
    OV5642_write_cmos_sensor(0x8a90, 0x0b);
    OV5642_write_cmos_sensor(0x8a91, 0x0a);
    OV5642_write_cmos_sensor(0x8a92, 0x12);
    OV5642_write_cmos_sensor(0x8a93, 0x01);
    OV5642_write_cmos_sensor(0x8a94, 0x6a);
    OV5642_write_cmos_sensor(0x8a95, 0x12);
    OV5642_write_cmos_sensor(0x8a96, 0x02);
    OV5642_write_cmos_sensor(0x8a97, 0x12);
    OV5642_write_cmos_sensor(0x8a98, 0xe5);
    OV5642_write_cmos_sensor(0x8a99, 0x0d);
    OV5642_write_cmos_sensor(0x8a9a, 0xf5);
    OV5642_write_cmos_sensor(0x8a9b, 0x10);
    OV5642_write_cmos_sensor(0x8a9c, 0x12);
    OV5642_write_cmos_sensor(0x8a9d, 0x0b);
    OV5642_write_cmos_sensor(0x8a9e, 0x53);
    OV5642_write_cmos_sensor(0x8a9f, 0x75);
    OV5642_write_cmos_sensor(0x8aa0, 0x51);
    OV5642_write_cmos_sensor(0x8aa1, 0x0a);
    OV5642_write_cmos_sensor(0x8aa2, 0xaf);
    OV5642_write_cmos_sensor(0x8aa3, 0x51);
    OV5642_write_cmos_sensor(0x8aa4, 0x15);
    OV5642_write_cmos_sensor(0x8aa5, 0x51);
    OV5642_write_cmos_sensor(0x8aa6, 0xef);
    OV5642_write_cmos_sensor(0x8aa7, 0x70);
    OV5642_write_cmos_sensor(0x8aa8, 0xf9);
    OV5642_write_cmos_sensor(0x8aa9, 0xd2);
    OV5642_write_cmos_sensor(0x8aaa, 0x28);
    OV5642_write_cmos_sensor(0x8aab, 0x12);
    OV5642_write_cmos_sensor(0x8aac, 0x01);
    OV5642_write_cmos_sensor(0x8aad, 0x6c);
    OV5642_write_cmos_sensor(0x8aae, 0x12);
    OV5642_write_cmos_sensor(0x8aaf, 0x02);
    OV5642_write_cmos_sensor(0x8ab0, 0x2f);
    OV5642_write_cmos_sensor(0x8ab1, 0xaf);
    OV5642_write_cmos_sensor(0x8ab2, 0x51);
    OV5642_write_cmos_sensor(0x8ab3, 0x15);
    OV5642_write_cmos_sensor(0x8ab4, 0x51);
    OV5642_write_cmos_sensor(0x8ab5, 0xef);
    OV5642_write_cmos_sensor(0x8ab6, 0x70);
    OV5642_write_cmos_sensor(0x8ab7, 0xf9);
    OV5642_write_cmos_sensor(0x8ab8, 0xc2);
    OV5642_write_cmos_sensor(0x8ab9, 0x28);
    OV5642_write_cmos_sensor(0x8aba, 0x12);
    OV5642_write_cmos_sensor(0x8abb, 0x01);
    OV5642_write_cmos_sensor(0x8abc, 0x6c);
    OV5642_write_cmos_sensor(0x8abd, 0x75);
    OV5642_write_cmos_sensor(0x8abe, 0x51);
    OV5642_write_cmos_sensor(0x8abf, 0x0a);
    OV5642_write_cmos_sensor(0x8ac0, 0xaf);
    OV5642_write_cmos_sensor(0x8ac1, 0x51);
    OV5642_write_cmos_sensor(0x8ac2, 0x15);
    OV5642_write_cmos_sensor(0x8ac3, 0x51);
    OV5642_write_cmos_sensor(0x8ac4, 0xef);
    OV5642_write_cmos_sensor(0x8ac5, 0x70);
    OV5642_write_cmos_sensor(0x8ac6, 0xf9);
    OV5642_write_cmos_sensor(0x8ac7, 0x30);
    OV5642_write_cmos_sensor(0x8ac8, 0x11);
    OV5642_write_cmos_sensor(0x8ac9, 0x04);
    OV5642_write_cmos_sensor(0x8aca, 0x15);
    OV5642_write_cmos_sensor(0x8acb, 0x0f);
    OV5642_write_cmos_sensor(0x8acc, 0x80);
    OV5642_write_cmos_sensor(0x8acd, 0x45);
    OV5642_write_cmos_sensor(0x8ace, 0x12);
    OV5642_write_cmos_sensor(0x8acf, 0x01);
    OV5642_write_cmos_sensor(0x8ad0, 0x6a);
    OV5642_write_cmos_sensor(0x8ad1, 0x12);
    OV5642_write_cmos_sensor(0x8ad2, 0x02);
    OV5642_write_cmos_sensor(0x8ad3, 0x12);
    OV5642_write_cmos_sensor(0x8ad4, 0x85);
    OV5642_write_cmos_sensor(0x8ad5, 0x0e);
    OV5642_write_cmos_sensor(0x8ad6, 0x10);
    OV5642_write_cmos_sensor(0x8ad7, 0x12);
    OV5642_write_cmos_sensor(0x8ad8, 0x13);
    OV5642_write_cmos_sensor(0x8ad9, 0x71);
    OV5642_write_cmos_sensor(0x8ada, 0xc2);
    OV5642_write_cmos_sensor(0x8adb, 0x09);
    OV5642_write_cmos_sensor(0x8adc, 0x12);
    OV5642_write_cmos_sensor(0x8add, 0x02);
    OV5642_write_cmos_sensor(0x8ade, 0x14);
    OV5642_write_cmos_sensor(0x8adf, 0x75);
    OV5642_write_cmos_sensor(0x8ae0, 0x51);
    OV5642_write_cmos_sensor(0x8ae1, 0x0a);
    OV5642_write_cmos_sensor(0x8ae2, 0xaf);
    OV5642_write_cmos_sensor(0x8ae3, 0x51);
    OV5642_write_cmos_sensor(0x8ae4, 0x15);
    OV5642_write_cmos_sensor(0x8ae5, 0x51);
    OV5642_write_cmos_sensor(0x8ae6, 0xef);
    OV5642_write_cmos_sensor(0x8ae7, 0x70);
    OV5642_write_cmos_sensor(0x8ae8, 0xf9);
    OV5642_write_cmos_sensor(0x8ae9, 0xd2);
    OV5642_write_cmos_sensor(0x8aea, 0x28);
    OV5642_write_cmos_sensor(0x8aeb, 0x12);
    OV5642_write_cmos_sensor(0x8aec, 0x01);
    OV5642_write_cmos_sensor(0x8aed, 0x6c);
    OV5642_write_cmos_sensor(0x8aee, 0x12);
    OV5642_write_cmos_sensor(0x8aef, 0x02);
    OV5642_write_cmos_sensor(0x8af0, 0x2f);
    OV5642_write_cmos_sensor(0x8af1, 0xaf);
    OV5642_write_cmos_sensor(0x8af2, 0x51);
    OV5642_write_cmos_sensor(0x8af3, 0x15);
    OV5642_write_cmos_sensor(0x8af4, 0x51);
    OV5642_write_cmos_sensor(0x8af5, 0xef);
    OV5642_write_cmos_sensor(0x8af6, 0x70);
    OV5642_write_cmos_sensor(0x8af7, 0xf9);
    OV5642_write_cmos_sensor(0x8af8, 0xc2);
    OV5642_write_cmos_sensor(0x8af9, 0x28);
    OV5642_write_cmos_sensor(0x8afa, 0x12);
    OV5642_write_cmos_sensor(0x8afb, 0x01);
    OV5642_write_cmos_sensor(0x8afc, 0x6c);
    OV5642_write_cmos_sensor(0x8afd, 0x75);
    OV5642_write_cmos_sensor(0x8afe, 0x51);
    OV5642_write_cmos_sensor(0x8aff, 0x0a);
    OV5642_write_cmos_sensor(0x8b00, 0xaf);
    OV5642_write_cmos_sensor(0x8b01, 0x51);
    OV5642_write_cmos_sensor(0x8b02, 0x15);
    OV5642_write_cmos_sensor(0x8b03, 0x51);
    OV5642_write_cmos_sensor(0x8b04, 0xef);
    OV5642_write_cmos_sensor(0x8b05, 0x70);
    OV5642_write_cmos_sensor(0x8b06, 0xf9);
    OV5642_write_cmos_sensor(0x8b07, 0x30);
    OV5642_write_cmos_sensor(0x8b08, 0x11);
    OV5642_write_cmos_sensor(0x8b09, 0x06);
    OV5642_write_cmos_sensor(0x8b0a, 0x15);
    OV5642_write_cmos_sensor(0x8b0b, 0x0f);
    OV5642_write_cmos_sensor(0x8b0c, 0xd2);
    OV5642_write_cmos_sensor(0x8b0d, 0x33);
    OV5642_write_cmos_sensor(0x8b0e, 0x80);
    OV5642_write_cmos_sensor(0x8b0f, 0x03);
    OV5642_write_cmos_sensor(0x8b10, 0xe4);
    OV5642_write_cmos_sensor(0x8b11, 0xf5);
    OV5642_write_cmos_sensor(0x8b12, 0x0f);
    OV5642_write_cmos_sensor(0x8b13, 0x12);
    OV5642_write_cmos_sensor(0x8b14, 0x01);
    OV5642_write_cmos_sensor(0x8b15, 0x6a);
    OV5642_write_cmos_sensor(0x8b16, 0x12);
    OV5642_write_cmos_sensor(0x8b17, 0x02);
    OV5642_write_cmos_sensor(0x8b18, 0x12);
    OV5642_write_cmos_sensor(0x8b19, 0xc2);
    OV5642_write_cmos_sensor(0x8b1a, 0x29);
    OV5642_write_cmos_sensor(0x8b1b, 0x12);
    OV5642_write_cmos_sensor(0x8b1c, 0x01);
    OV5642_write_cmos_sensor(0x8b1d, 0x6c);
    OV5642_write_cmos_sensor(0x8b1e, 0x75);
    OV5642_write_cmos_sensor(0x8b1f, 0x51);
    OV5642_write_cmos_sensor(0x8b20, 0x05);
    OV5642_write_cmos_sensor(0x8b21, 0xaf);
    OV5642_write_cmos_sensor(0x8b22, 0x51);
    OV5642_write_cmos_sensor(0x8b23, 0x15);
    OV5642_write_cmos_sensor(0x8b24, 0x51);
    OV5642_write_cmos_sensor(0x8b25, 0xef);
    OV5642_write_cmos_sensor(0x8b26, 0x70);
    OV5642_write_cmos_sensor(0x8b27, 0xf9);
    OV5642_write_cmos_sensor(0x8b28, 0xd2);
    OV5642_write_cmos_sensor(0x8b29, 0x28);
    OV5642_write_cmos_sensor(0x8b2a, 0x12);
    OV5642_write_cmos_sensor(0x8b2b, 0x01);
    OV5642_write_cmos_sensor(0x8b2c, 0x6c);
    OV5642_write_cmos_sensor(0x8b2d, 0x75);
    OV5642_write_cmos_sensor(0x8b2e, 0x51);
    OV5642_write_cmos_sensor(0x8b2f, 0x05);
    OV5642_write_cmos_sensor(0x8b30, 0xaf);
    OV5642_write_cmos_sensor(0x8b31, 0x51);
    OV5642_write_cmos_sensor(0x8b32, 0x15);
    OV5642_write_cmos_sensor(0x8b33, 0x51);
    OV5642_write_cmos_sensor(0x8b34, 0xef);
    OV5642_write_cmos_sensor(0x8b35, 0x70);
    OV5642_write_cmos_sensor(0x8b36, 0xf9);
    OV5642_write_cmos_sensor(0x8b37, 0x12);
    OV5642_write_cmos_sensor(0x8b38, 0x01);
    OV5642_write_cmos_sensor(0x8b39, 0x6a);
    OV5642_write_cmos_sensor(0x8b3a, 0x75);
    OV5642_write_cmos_sensor(0x8b3b, 0x51);
    OV5642_write_cmos_sensor(0x8b3c, 0x05);
    OV5642_write_cmos_sensor(0x8b3d, 0xaf);
    OV5642_write_cmos_sensor(0x8b3e, 0x51);
    OV5642_write_cmos_sensor(0x8b3f, 0x15);
    OV5642_write_cmos_sensor(0x8b40, 0x51);
    OV5642_write_cmos_sensor(0x8b41, 0xef);
    OV5642_write_cmos_sensor(0x8b42, 0x70);
    OV5642_write_cmos_sensor(0x8b43, 0xf9);
    OV5642_write_cmos_sensor(0x8b44, 0xa2);
    OV5642_write_cmos_sensor(0x8b45, 0x32);
    OV5642_write_cmos_sensor(0x8b46, 0x92);
    OV5642_write_cmos_sensor(0x8b47, 0xaf);
    OV5642_write_cmos_sensor(0x8b48, 0xe5);
    OV5642_write_cmos_sensor(0x8b49, 0x0f);
    OV5642_write_cmos_sensor(0x8b4a, 0xd3);
    OV5642_write_cmos_sensor(0x8b4b, 0x94);
    OV5642_write_cmos_sensor(0x8b4c, 0x00);
    OV5642_write_cmos_sensor(0x8b4d, 0x40);
    OV5642_write_cmos_sensor(0x8b4e, 0x03);
    OV5642_write_cmos_sensor(0x8b4f, 0x02);
    OV5642_write_cmos_sensor(0x8b50, 0x0a);
    OV5642_write_cmos_sensor(0x8b51, 0x1a);
    OV5642_write_cmos_sensor(0x8b52, 0x22);
    OV5642_write_cmos_sensor(0x8b53, 0x12);
    OV5642_write_cmos_sensor(0x8b54, 0x13);
    OV5642_write_cmos_sensor(0x8b55, 0x71);
    OV5642_write_cmos_sensor(0x8b56, 0xc2);
    OV5642_write_cmos_sensor(0x8b57, 0x09);
    OV5642_write_cmos_sensor(0x8b58, 0x90);
    OV5642_write_cmos_sensor(0x8b59, 0x30);
    OV5642_write_cmos_sensor(0x8b5a, 0x18);
    OV5642_write_cmos_sensor(0x8b5b, 0xe5);
    OV5642_write_cmos_sensor(0x8b5c, 0x21);
    OV5642_write_cmos_sensor(0x8b5d, 0xf0);
    OV5642_write_cmos_sensor(0x8b5e, 0x22);
    OV5642_write_cmos_sensor(0x8b5f, 0xc0);
    OV5642_write_cmos_sensor(0x8b60, 0xe0);
    OV5642_write_cmos_sensor(0x8b61, 0xc0);
    OV5642_write_cmos_sensor(0x8b62, 0xf0);
    OV5642_write_cmos_sensor(0x8b63, 0xc0);
    OV5642_write_cmos_sensor(0x8b64, 0x83);
    OV5642_write_cmos_sensor(0x8b65, 0xc0);
    OV5642_write_cmos_sensor(0x8b66, 0x82);
    OV5642_write_cmos_sensor(0x8b67, 0xc0);
    OV5642_write_cmos_sensor(0x8b68, 0xd0);
    OV5642_write_cmos_sensor(0x8b69, 0x75);
    OV5642_write_cmos_sensor(0x8b6a, 0xd0);
    OV5642_write_cmos_sensor(0x8b6b, 0x00);
    OV5642_write_cmos_sensor(0x8b6c, 0xc0);
    OV5642_write_cmos_sensor(0x8b6d, 0x00);
    OV5642_write_cmos_sensor(0x8b6e, 0xc0);
    OV5642_write_cmos_sensor(0x8b6f, 0x01);
    OV5642_write_cmos_sensor(0x8b70, 0xc0);
    OV5642_write_cmos_sensor(0x8b71, 0x02);
    OV5642_write_cmos_sensor(0x8b72, 0xc0);
    OV5642_write_cmos_sensor(0x8b73, 0x03);
    OV5642_write_cmos_sensor(0x8b74, 0xc0);
    OV5642_write_cmos_sensor(0x8b75, 0x04);
    OV5642_write_cmos_sensor(0x8b76, 0xc0);
    OV5642_write_cmos_sensor(0x8b77, 0x05);
    OV5642_write_cmos_sensor(0x8b78, 0xc0);
    OV5642_write_cmos_sensor(0x8b79, 0x06);
    OV5642_write_cmos_sensor(0x8b7a, 0xc0);
    OV5642_write_cmos_sensor(0x8b7b, 0x07);
    OV5642_write_cmos_sensor(0x8b7c, 0x90);
    OV5642_write_cmos_sensor(0x8b7d, 0x3f);
    OV5642_write_cmos_sensor(0x8b7e, 0x0c);
    OV5642_write_cmos_sensor(0x8b7f, 0xe0);
    OV5642_write_cmos_sensor(0x8b80, 0xf5);
    OV5642_write_cmos_sensor(0x8b81, 0x08);
    OV5642_write_cmos_sensor(0x8b82, 0xe5);
    OV5642_write_cmos_sensor(0x8b83, 0x08);
    OV5642_write_cmos_sensor(0x8b84, 0x20);
    OV5642_write_cmos_sensor(0x8b85, 0xe3);
    OV5642_write_cmos_sensor(0x8b86, 0x03);
    OV5642_write_cmos_sensor(0x8b87, 0x02);
    OV5642_write_cmos_sensor(0x8b88, 0x0c);
    OV5642_write_cmos_sensor(0x8b89, 0x10);
    OV5642_write_cmos_sensor(0x8b8a, 0x30);
    OV5642_write_cmos_sensor(0x8b8b, 0x35);
    OV5642_write_cmos_sensor(0x8b8c, 0x03);
    OV5642_write_cmos_sensor(0x8b8d, 0x02);
    OV5642_write_cmos_sensor(0x8b8e, 0x0c);
    OV5642_write_cmos_sensor(0x8b8f, 0x10);
    OV5642_write_cmos_sensor(0x8b90, 0x90);
    OV5642_write_cmos_sensor(0x8b91, 0x60);
    OV5642_write_cmos_sensor(0x8b92, 0x16);
    OV5642_write_cmos_sensor(0x8b93, 0xe0);
    OV5642_write_cmos_sensor(0x8b94, 0xf5);
    OV5642_write_cmos_sensor(0x8b95, 0x6a);
    OV5642_write_cmos_sensor(0x8b96, 0xa3);
    OV5642_write_cmos_sensor(0x8b97, 0xe0);
    OV5642_write_cmos_sensor(0x8b98, 0xf5);
    OV5642_write_cmos_sensor(0x8b99, 0x6b);
    OV5642_write_cmos_sensor(0x8b9a, 0x90);
    OV5642_write_cmos_sensor(0x8b9b, 0x60);
    OV5642_write_cmos_sensor(0x8b9c, 0x1e);
    OV5642_write_cmos_sensor(0x8b9d, 0xe0);
    OV5642_write_cmos_sensor(0x8b9e, 0xf5);
    OV5642_write_cmos_sensor(0x8b9f, 0x6c);
    OV5642_write_cmos_sensor(0x8ba0, 0xa3);
    OV5642_write_cmos_sensor(0x8ba1, 0xe0);
    OV5642_write_cmos_sensor(0x8ba2, 0xf5);
    OV5642_write_cmos_sensor(0x8ba3, 0x6d);
    OV5642_write_cmos_sensor(0x8ba4, 0x90);
    OV5642_write_cmos_sensor(0x8ba5, 0x60);
    OV5642_write_cmos_sensor(0x8ba6, 0x26);
    OV5642_write_cmos_sensor(0x8ba7, 0xe0);
    OV5642_write_cmos_sensor(0x8ba8, 0xf5);
    OV5642_write_cmos_sensor(0x8ba9, 0x6e);
    OV5642_write_cmos_sensor(0x8baa, 0xa3);
    OV5642_write_cmos_sensor(0x8bab, 0xe0);
    OV5642_write_cmos_sensor(0x8bac, 0xf5);
    OV5642_write_cmos_sensor(0x8bad, 0x6f);
    OV5642_write_cmos_sensor(0x8bae, 0x90);
    OV5642_write_cmos_sensor(0x8baf, 0x60);
    OV5642_write_cmos_sensor(0x8bb0, 0x2e);
    OV5642_write_cmos_sensor(0x8bb1, 0xe0);
    OV5642_write_cmos_sensor(0x8bb2, 0xf5);
    OV5642_write_cmos_sensor(0x8bb3, 0x70);
    OV5642_write_cmos_sensor(0x8bb4, 0xa3);
    OV5642_write_cmos_sensor(0x8bb5, 0xe0);
    OV5642_write_cmos_sensor(0x8bb6, 0xf5);
    OV5642_write_cmos_sensor(0x8bb7, 0x71);
    OV5642_write_cmos_sensor(0x8bb8, 0x90);
    OV5642_write_cmos_sensor(0x8bb9, 0x60);
    OV5642_write_cmos_sensor(0x8bba, 0x36);
    OV5642_write_cmos_sensor(0x8bbb, 0x12);
    OV5642_write_cmos_sensor(0x8bbc, 0x00);
    OV5642_write_cmos_sensor(0x8bbd, 0x16);
    OV5642_write_cmos_sensor(0x8bbe, 0x12);
    OV5642_write_cmos_sensor(0x8bbf, 0x01);
    OV5642_write_cmos_sensor(0x8bc0, 0xd2);
    OV5642_write_cmos_sensor(0x8bc1, 0x40);
    OV5642_write_cmos_sensor(0x8bc2, 0x06);
    OV5642_write_cmos_sensor(0x8bc3, 0x75);
    OV5642_write_cmos_sensor(0x8bc4, 0x2a);
    OV5642_write_cmos_sensor(0x8bc5, 0xff);
    OV5642_write_cmos_sensor(0x8bc6, 0x75);
    OV5642_write_cmos_sensor(0x8bc7, 0x2b);
    OV5642_write_cmos_sensor(0x8bc8, 0xff);
    OV5642_write_cmos_sensor(0x8bc9, 0x85);
    OV5642_write_cmos_sensor(0x8bca, 0x2a);
    OV5642_write_cmos_sensor(0x8bcb, 0x74);
    OV5642_write_cmos_sensor(0x8bcc, 0x85);
    OV5642_write_cmos_sensor(0x8bcd, 0x2b);
    OV5642_write_cmos_sensor(0x8bce, 0x75);
    OV5642_write_cmos_sensor(0x8bcf, 0x90);
    OV5642_write_cmos_sensor(0x8bd0, 0x60);
    OV5642_write_cmos_sensor(0x8bd1, 0x1a);
    OV5642_write_cmos_sensor(0x8bd2, 0xe0);
    OV5642_write_cmos_sensor(0x8bd3, 0xf5);
    OV5642_write_cmos_sensor(0x8bd4, 0x6a);
    OV5642_write_cmos_sensor(0x8bd5, 0xa3);
    OV5642_write_cmos_sensor(0x8bd6, 0xe0);
    OV5642_write_cmos_sensor(0x8bd7, 0xf5);
    OV5642_write_cmos_sensor(0x8bd8, 0x6b);
    OV5642_write_cmos_sensor(0x8bd9, 0x90);
    OV5642_write_cmos_sensor(0x8bda, 0x60);
    OV5642_write_cmos_sensor(0x8bdb, 0x22);
    OV5642_write_cmos_sensor(0x8bdc, 0xe0);
    OV5642_write_cmos_sensor(0x8bdd, 0xf5);
    OV5642_write_cmos_sensor(0x8bde, 0x6c);
    OV5642_write_cmos_sensor(0x8bdf, 0xa3);
    OV5642_write_cmos_sensor(0x8be0, 0xe0);
    OV5642_write_cmos_sensor(0x8be1, 0xf5);
    OV5642_write_cmos_sensor(0x8be2, 0x6d);
    OV5642_write_cmos_sensor(0x8be3, 0x90);
    OV5642_write_cmos_sensor(0x8be4, 0x60);
    OV5642_write_cmos_sensor(0x8be5, 0x2a);
    OV5642_write_cmos_sensor(0x8be6, 0xe0);
    OV5642_write_cmos_sensor(0x8be7, 0xf5);
    OV5642_write_cmos_sensor(0x8be8, 0x6e);
    OV5642_write_cmos_sensor(0x8be9, 0xa3);
    OV5642_write_cmos_sensor(0x8bea, 0xe0);
    OV5642_write_cmos_sensor(0x8beb, 0xf5);
    OV5642_write_cmos_sensor(0x8bec, 0x6f);
    OV5642_write_cmos_sensor(0x8bed, 0x90);
    OV5642_write_cmos_sensor(0x8bee, 0x60);
    OV5642_write_cmos_sensor(0x8bef, 0x32);
    OV5642_write_cmos_sensor(0x8bf0, 0xe0);
    OV5642_write_cmos_sensor(0x8bf1, 0xf5);
    OV5642_write_cmos_sensor(0x8bf2, 0x70);
    OV5642_write_cmos_sensor(0x8bf3, 0xa3);
    OV5642_write_cmos_sensor(0x8bf4, 0xe0);
    OV5642_write_cmos_sensor(0x8bf5, 0xf5);
    OV5642_write_cmos_sensor(0x8bf6, 0x71);
    OV5642_write_cmos_sensor(0x8bf7, 0x90);
    OV5642_write_cmos_sensor(0x8bf8, 0x60);
    OV5642_write_cmos_sensor(0x8bf9, 0x3a);
    OV5642_write_cmos_sensor(0x8bfa, 0x12);
    OV5642_write_cmos_sensor(0x8bfb, 0x00);
    OV5642_write_cmos_sensor(0x8bfc, 0x16);
    OV5642_write_cmos_sensor(0x8bfd, 0x12);
    OV5642_write_cmos_sensor(0x8bfe, 0x01);
    OV5642_write_cmos_sensor(0x8bff, 0xd2);
    OV5642_write_cmos_sensor(0x8c00, 0x40);
    OV5642_write_cmos_sensor(0x8c01, 0x06);
    OV5642_write_cmos_sensor(0x8c02, 0x75);
    OV5642_write_cmos_sensor(0x8c03, 0x2a);
    OV5642_write_cmos_sensor(0x8c04, 0xff);
    OV5642_write_cmos_sensor(0x8c05, 0x75);
    OV5642_write_cmos_sensor(0x8c06, 0x2b);
    OV5642_write_cmos_sensor(0x8c07, 0xff);
    OV5642_write_cmos_sensor(0x8c08, 0x85);
    OV5642_write_cmos_sensor(0x8c09, 0x2a);
    OV5642_write_cmos_sensor(0x8c0a, 0x76);
    OV5642_write_cmos_sensor(0x8c0b, 0x85);
    OV5642_write_cmos_sensor(0x8c0c, 0x2b);
    OV5642_write_cmos_sensor(0x8c0d, 0x77);
    OV5642_write_cmos_sensor(0x8c0e, 0xd2);
    OV5642_write_cmos_sensor(0x8c0f, 0x3b);
    OV5642_write_cmos_sensor(0x8c10, 0xe5);
    OV5642_write_cmos_sensor(0x8c11, 0x08);
    OV5642_write_cmos_sensor(0x8c12, 0x30);
    OV5642_write_cmos_sensor(0x8c13, 0xe5);
    OV5642_write_cmos_sensor(0x8c14, 0x41);
    OV5642_write_cmos_sensor(0x8c15, 0x90);
    OV5642_write_cmos_sensor(0x8c16, 0x56);
    OV5642_write_cmos_sensor(0x8c17, 0x90);
    OV5642_write_cmos_sensor(0x8c18, 0xe0);
    OV5642_write_cmos_sensor(0x8c19, 0xf5);
    OV5642_write_cmos_sensor(0x8c1a, 0x55);
    OV5642_write_cmos_sensor(0x8c1b, 0xe5);
    OV5642_write_cmos_sensor(0x8c1c, 0x7b);
    OV5642_write_cmos_sensor(0x8c1d, 0x12);
    OV5642_write_cmos_sensor(0x8c1e, 0x01);
    OV5642_write_cmos_sensor(0x8c1f, 0xcb);
    OV5642_write_cmos_sensor(0x8c20, 0xad);
    OV5642_write_cmos_sensor(0x8c21, 0x55);
    OV5642_write_cmos_sensor(0x8c22, 0xc3);
    OV5642_write_cmos_sensor(0x8c23, 0xef);
    OV5642_write_cmos_sensor(0x8c24, 0x9d);
    OV5642_write_cmos_sensor(0x8c25, 0x74);
    OV5642_write_cmos_sensor(0x8c26, 0x80);
    OV5642_write_cmos_sensor(0x8c27, 0xf8);
    OV5642_write_cmos_sensor(0x8c28, 0x6e);
    OV5642_write_cmos_sensor(0x8c29, 0x98);
    OV5642_write_cmos_sensor(0x8c2a, 0x50);
    OV5642_write_cmos_sensor(0x8c2b, 0x02);
    OV5642_write_cmos_sensor(0x8c2c, 0x80);
    OV5642_write_cmos_sensor(0x8c2d, 0x01);
    OV5642_write_cmos_sensor(0x8c2e, 0xc3);
    OV5642_write_cmos_sensor(0x8c2f, 0x92);
    OV5642_write_cmos_sensor(0x8c30, 0x27);
    OV5642_write_cmos_sensor(0x8c31, 0xaf);
    OV5642_write_cmos_sensor(0x8c32, 0x55);
    OV5642_write_cmos_sensor(0x8c33, 0xef);
    OV5642_write_cmos_sensor(0x8c34, 0x24);
    OV5642_write_cmos_sensor(0x8c35, 0x01);
    OV5642_write_cmos_sensor(0x8c36, 0xff);
    OV5642_write_cmos_sensor(0x8c37, 0xe4);
    OV5642_write_cmos_sensor(0x8c38, 0x33);
    OV5642_write_cmos_sensor(0x8c39, 0xfe);
    OV5642_write_cmos_sensor(0x8c3a, 0xc3);
    OV5642_write_cmos_sensor(0x8c3b, 0xef);
    OV5642_write_cmos_sensor(0x8c3c, 0x95);
    OV5642_write_cmos_sensor(0x8c3d, 0x7b);
    OV5642_write_cmos_sensor(0x8c3e, 0x74);
    OV5642_write_cmos_sensor(0x8c3f, 0x80);
    OV5642_write_cmos_sensor(0x8c40, 0xf8);
    OV5642_write_cmos_sensor(0x8c41, 0x6e);
    OV5642_write_cmos_sensor(0x8c42, 0x98);
    OV5642_write_cmos_sensor(0x8c43, 0x50);
    OV5642_write_cmos_sensor(0x8c44, 0x02);
    OV5642_write_cmos_sensor(0x8c45, 0x80);
    OV5642_write_cmos_sensor(0x8c46, 0x02);
    OV5642_write_cmos_sensor(0x8c47, 0xa2);
    OV5642_write_cmos_sensor(0x8c48, 0x27);
    OV5642_write_cmos_sensor(0x8c49, 0x92);
    OV5642_write_cmos_sensor(0x8c4a, 0x27);
    OV5642_write_cmos_sensor(0x8c4b, 0x30);
    OV5642_write_cmos_sensor(0x8c4c, 0x27);
    OV5642_write_cmos_sensor(0x8c4d, 0x04);
    OV5642_write_cmos_sensor(0x8c4e, 0xaf);
    OV5642_write_cmos_sensor(0x8c4f, 0x55);
    OV5642_write_cmos_sensor(0x8c50, 0x80);
    OV5642_write_cmos_sensor(0x8c51, 0x02);
    OV5642_write_cmos_sensor(0x8c52, 0xaf);
    OV5642_write_cmos_sensor(0x8c53, 0x7b);
    OV5642_write_cmos_sensor(0x8c54, 0x8f);
    OV5642_write_cmos_sensor(0x8c55, 0x7b);
    OV5642_write_cmos_sensor(0x8c56, 0xe5);
    OV5642_write_cmos_sensor(0x8c57, 0x08);
    OV5642_write_cmos_sensor(0x8c58, 0x30);
    OV5642_write_cmos_sensor(0x8c59, 0xe1);
    OV5642_write_cmos_sensor(0x8c5a, 0x08);
    OV5642_write_cmos_sensor(0x8c5b, 0x90);
    OV5642_write_cmos_sensor(0x8c5c, 0x30);
    OV5642_write_cmos_sensor(0x8c5d, 0x24);
    OV5642_write_cmos_sensor(0x8c5e, 0xe0);
    OV5642_write_cmos_sensor(0x8c5f, 0xf5);
    OV5642_write_cmos_sensor(0x8c60, 0x33);
    OV5642_write_cmos_sensor(0x8c61, 0xe4);
    OV5642_write_cmos_sensor(0x8c62, 0xf0);
    OV5642_write_cmos_sensor(0x8c63, 0x90);
    OV5642_write_cmos_sensor(0x8c64, 0x3f);
    OV5642_write_cmos_sensor(0x8c65, 0x0c);
    OV5642_write_cmos_sensor(0x8c66, 0xe5);
    OV5642_write_cmos_sensor(0x8c67, 0x08);
    OV5642_write_cmos_sensor(0x8c68, 0xf0);
    OV5642_write_cmos_sensor(0x8c69, 0xd0);
    OV5642_write_cmos_sensor(0x8c6a, 0x07);
    OV5642_write_cmos_sensor(0x8c6b, 0xd0);
    OV5642_write_cmos_sensor(0x8c6c, 0x06);
    OV5642_write_cmos_sensor(0x8c6d, 0xd0);
    OV5642_write_cmos_sensor(0x8c6e, 0x05);
    OV5642_write_cmos_sensor(0x8c6f, 0xd0);
    OV5642_write_cmos_sensor(0x8c70, 0x04);
    OV5642_write_cmos_sensor(0x8c71, 0xd0);
    OV5642_write_cmos_sensor(0x8c72, 0x03);
    OV5642_write_cmos_sensor(0x8c73, 0xd0);
    OV5642_write_cmos_sensor(0x8c74, 0x02);
    OV5642_write_cmos_sensor(0x8c75, 0xd0);
    OV5642_write_cmos_sensor(0x8c76, 0x01);
    OV5642_write_cmos_sensor(0x8c77, 0xd0);
    OV5642_write_cmos_sensor(0x8c78, 0x00);
    OV5642_write_cmos_sensor(0x8c79, 0xd0);
    OV5642_write_cmos_sensor(0x8c7a, 0xd0);
    OV5642_write_cmos_sensor(0x8c7b, 0xd0);
    OV5642_write_cmos_sensor(0x8c7c, 0x82);
    OV5642_write_cmos_sensor(0x8c7d, 0xd0);
    OV5642_write_cmos_sensor(0x8c7e, 0x83);
    OV5642_write_cmos_sensor(0x8c7f, 0xd0);
    OV5642_write_cmos_sensor(0x8c80, 0xf0);
    OV5642_write_cmos_sensor(0x8c81, 0xd0);
    OV5642_write_cmos_sensor(0x8c82, 0xe0);
    OV5642_write_cmos_sensor(0x8c83, 0x32);
    OV5642_write_cmos_sensor(0x8c84, 0xe5);
    OV5642_write_cmos_sensor(0x8c85, 0x33);
    OV5642_write_cmos_sensor(0x8c86, 0x70);
    OV5642_write_cmos_sensor(0x8c87, 0x03);
    OV5642_write_cmos_sensor(0x8c88, 0x02);
    OV5642_write_cmos_sensor(0x8c89, 0x0d);
    OV5642_write_cmos_sensor(0x8c8a, 0x91);
    OV5642_write_cmos_sensor(0x8c8b, 0xc2);
    OV5642_write_cmos_sensor(0x8c8c, 0xaf);
    OV5642_write_cmos_sensor(0x8c8d, 0xaf);
    OV5642_write_cmos_sensor(0x8c8e, 0x33);
    OV5642_write_cmos_sensor(0x8c8f, 0xe4);
    OV5642_write_cmos_sensor(0x8c90, 0xf5);
    OV5642_write_cmos_sensor(0x8c91, 0x33);
    OV5642_write_cmos_sensor(0x8c92, 0xd2);
    OV5642_write_cmos_sensor(0x8c93, 0xaf);
    OV5642_write_cmos_sensor(0x8c94, 0x90);
    OV5642_write_cmos_sensor(0x8c95, 0x30);
    OV5642_write_cmos_sensor(0x8c96, 0x25);
    OV5642_write_cmos_sensor(0x8c97, 0xe0);
    OV5642_write_cmos_sensor(0x8c98, 0xf5);
    OV5642_write_cmos_sensor(0x8c99, 0x7e);
    OV5642_write_cmos_sensor(0x8c9a, 0x90);
    OV5642_write_cmos_sensor(0x8c9b, 0x50);
    OV5642_write_cmos_sensor(0x8c9c, 0x82);
    OV5642_write_cmos_sensor(0x8c9d, 0xe0);
    OV5642_write_cmos_sensor(0x8c9e, 0xf5);
    OV5642_write_cmos_sensor(0x8c9f, 0x66);
    OV5642_write_cmos_sensor(0x8ca0, 0xa3);
    OV5642_write_cmos_sensor(0x8ca1, 0xe0);
    OV5642_write_cmos_sensor(0x8ca2, 0xf5);
    OV5642_write_cmos_sensor(0x8ca3, 0x67);
    OV5642_write_cmos_sensor(0x8ca4, 0xa3);
    OV5642_write_cmos_sensor(0x8ca5, 0xe0);
    OV5642_write_cmos_sensor(0x8ca6, 0xf5);
    OV5642_write_cmos_sensor(0x8ca7, 0x68);
    OV5642_write_cmos_sensor(0x8ca8, 0xa3);
    OV5642_write_cmos_sensor(0x8ca9, 0xe0);
    OV5642_write_cmos_sensor(0x8caa, 0xf5);
    OV5642_write_cmos_sensor(0x8cab, 0x69);
    OV5642_write_cmos_sensor(0x8cac, 0xef);
    OV5642_write_cmos_sensor(0x8cad, 0x12);
    OV5642_write_cmos_sensor(0x8cae, 0x09);
    OV5642_write_cmos_sensor(0x8caf, 0xed);
    OV5642_write_cmos_sensor(0x8cb0, 0x0c);
    OV5642_write_cmos_sensor(0x8cb1, 0xd5);
    OV5642_write_cmos_sensor(0x8cb2, 0x03);
    OV5642_write_cmos_sensor(0x8cb3, 0x0c);
    OV5642_write_cmos_sensor(0x8cb4, 0xea);
    OV5642_write_cmos_sensor(0x8cb5, 0x05);
    OV5642_write_cmos_sensor(0x8cb6, 0x0d);
    OV5642_write_cmos_sensor(0x8cb7, 0x14);
    OV5642_write_cmos_sensor(0x8cb8, 0x06);
    OV5642_write_cmos_sensor(0x8cb9, 0x0d);
    OV5642_write_cmos_sensor(0x8cba, 0x02);
    OV5642_write_cmos_sensor(0x8cbb, 0x08);
    OV5642_write_cmos_sensor(0x8cbc, 0x0d);
    OV5642_write_cmos_sensor(0x8cbd, 0x21);
    OV5642_write_cmos_sensor(0x8cbe, 0x10);
    OV5642_write_cmos_sensor(0x8cbf, 0x0d);
    OV5642_write_cmos_sensor(0x8cc0, 0x35);
    OV5642_write_cmos_sensor(0x8cc1, 0x12);
    OV5642_write_cmos_sensor(0x8cc2, 0x0d);
    OV5642_write_cmos_sensor(0x8cc3, 0x3a);
    OV5642_write_cmos_sensor(0x8cc4, 0x20);
    OV5642_write_cmos_sensor(0x8cc5, 0x0d);
    OV5642_write_cmos_sensor(0x8cc6, 0x48);
    OV5642_write_cmos_sensor(0x8cc7, 0x21);
    OV5642_write_cmos_sensor(0x8cc8, 0x0d);
    OV5642_write_cmos_sensor(0x8cc9, 0x4d);
    OV5642_write_cmos_sensor(0x8cca, 0x30);
    OV5642_write_cmos_sensor(0x8ccb, 0x0d);
    OV5642_write_cmos_sensor(0x8ccc, 0x76);
    OV5642_write_cmos_sensor(0x8ccd, 0x50);
    OV5642_write_cmos_sensor(0x8cce, 0x0d);
    OV5642_write_cmos_sensor(0x8ccf, 0x58);
    OV5642_write_cmos_sensor(0x8cd0, 0xd8);
    OV5642_write_cmos_sensor(0x8cd1, 0x00);
    OV5642_write_cmos_sensor(0x8cd2, 0x00);
    OV5642_write_cmos_sensor(0x8cd3, 0x0d);
    OV5642_write_cmos_sensor(0x8cd4, 0x83);
    OV5642_write_cmos_sensor(0x8cd5, 0x20);
    OV5642_write_cmos_sensor(0x8cd6, 0x05);
    OV5642_write_cmos_sensor(0x8cd7, 0x03);
    OV5642_write_cmos_sensor(0x8cd8, 0x02);
    OV5642_write_cmos_sensor(0x8cd9, 0x0d);
    OV5642_write_cmos_sensor(0x8cda, 0x83);
    OV5642_write_cmos_sensor(0x8cdb, 0x30);
    OV5642_write_cmos_sensor(0x8cdc, 0x00);
    OV5642_write_cmos_sensor(0x8cdd, 0x03);
    OV5642_write_cmos_sensor(0x8cde, 0x02);
    OV5642_write_cmos_sensor(0x8cdf, 0x0d);
    OV5642_write_cmos_sensor(0x8ce0, 0x83);
    OV5642_write_cmos_sensor(0x8ce1, 0xd2);
    OV5642_write_cmos_sensor(0x8ce2, 0x07);
    OV5642_write_cmos_sensor(0x8ce3, 0xc2);
    OV5642_write_cmos_sensor(0x8ce4, 0x06);
    OV5642_write_cmos_sensor(0x8ce5, 0x12);
    OV5642_write_cmos_sensor(0x8ce6, 0x02);
    OV5642_write_cmos_sensor(0x8ce7, 0x8a);
    OV5642_write_cmos_sensor(0x8ce8, 0x80);
    OV5642_write_cmos_sensor(0x8ce9, 0x24);
    OV5642_write_cmos_sensor(0x8cea, 0x20);
    OV5642_write_cmos_sensor(0x8ceb, 0x05);
    OV5642_write_cmos_sensor(0x8cec, 0x03);
    OV5642_write_cmos_sensor(0x8ced, 0x02);
    OV5642_write_cmos_sensor(0x8cee, 0x0d);
    OV5642_write_cmos_sensor(0x8cef, 0x83);
    OV5642_write_cmos_sensor(0x8cf0, 0x30);
    OV5642_write_cmos_sensor(0x8cf1, 0x00);
    OV5642_write_cmos_sensor(0x8cf2, 0x03);
    OV5642_write_cmos_sensor(0x8cf3, 0x02);
    OV5642_write_cmos_sensor(0x8cf4, 0x0d);
    OV5642_write_cmos_sensor(0x8cf5, 0x83);
    OV5642_write_cmos_sensor(0x8cf6, 0xc2);
    OV5642_write_cmos_sensor(0x8cf7, 0x07);
    OV5642_write_cmos_sensor(0x8cf8, 0xd2);
    OV5642_write_cmos_sensor(0x8cf9, 0x06);
    OV5642_write_cmos_sensor(0x8cfa, 0x12);
    OV5642_write_cmos_sensor(0x8cfb, 0x02);
    OV5642_write_cmos_sensor(0x8cfc, 0x9b);
    OV5642_write_cmos_sensor(0x8cfd, 0xc2);
    OV5642_write_cmos_sensor(0x8cfe, 0x04);
    OV5642_write_cmos_sensor(0x8cff, 0x02);
    OV5642_write_cmos_sensor(0x8d00, 0x0d);
    OV5642_write_cmos_sensor(0x8d01, 0x83);
    OV5642_write_cmos_sensor(0x8d02, 0x12);
    OV5642_write_cmos_sensor(0x8d03, 0x02);
    OV5642_write_cmos_sensor(0x8d04, 0x51);
    OV5642_write_cmos_sensor(0x8d05, 0x30);
    OV5642_write_cmos_sensor(0x8d06, 0x05);
    OV5642_write_cmos_sensor(0x8d07, 0x06);
    OV5642_write_cmos_sensor(0x8d08, 0xe4);
    OV5642_write_cmos_sensor(0x8d09, 0xf5);
    OV5642_write_cmos_sensor(0x8d0a, 0x0c);
    OV5642_write_cmos_sensor(0x8d0b, 0x12);
    OV5642_write_cmos_sensor(0x8d0c, 0x11);
    OV5642_write_cmos_sensor(0x8d0d, 0xd8);
    OV5642_write_cmos_sensor(0x8d0e, 0xc2);
    OV5642_write_cmos_sensor(0x8d0f, 0x31);
    OV5642_write_cmos_sensor(0x8d10, 0xd2);
    OV5642_write_cmos_sensor(0x8d11, 0x34);
    OV5642_write_cmos_sensor(0x8d12, 0x80);
    OV5642_write_cmos_sensor(0x8d13, 0x6f);
    OV5642_write_cmos_sensor(0x8d14, 0x30);
    OV5642_write_cmos_sensor(0x8d15, 0x07);
    OV5642_write_cmos_sensor(0x8d16, 0x6c);
    OV5642_write_cmos_sensor(0x8d17, 0x30);
    OV5642_write_cmos_sensor(0x8d18, 0x06);
    OV5642_write_cmos_sensor(0x8d19, 0x69);
    OV5642_write_cmos_sensor(0x8d1a, 0x12);
    OV5642_write_cmos_sensor(0x8d1b, 0x02);
    OV5642_write_cmos_sensor(0x8d1c, 0x8a);
    OV5642_write_cmos_sensor(0x8d1d, 0xd2);
    OV5642_write_cmos_sensor(0x8d1e, 0x31);
    OV5642_write_cmos_sensor(0x8d1f, 0x80);
    OV5642_write_cmos_sensor(0x8d20, 0x62);
    OV5642_write_cmos_sensor(0x8d21, 0x20);
    OV5642_write_cmos_sensor(0x8d22, 0x07);
    OV5642_write_cmos_sensor(0x8d23, 0x03);
    OV5642_write_cmos_sensor(0x8d24, 0x30);
    OV5642_write_cmos_sensor(0x8d25, 0x06);
    OV5642_write_cmos_sensor(0x8d26, 0x09);
    OV5642_write_cmos_sensor(0x8d27, 0xe5);
    OV5642_write_cmos_sensor(0x8d28, 0x7e);
    OV5642_write_cmos_sensor(0x8d29, 0x64);
    OV5642_write_cmos_sensor(0x8d2a, 0x0e);
    OV5642_write_cmos_sensor(0x8d2b, 0x70);
    OV5642_write_cmos_sensor(0x8d2c, 0x56);
    OV5642_write_cmos_sensor(0x8d2d, 0x20);
    OV5642_write_cmos_sensor(0x8d2e, 0x00);
    OV5642_write_cmos_sensor(0x8d2f, 0x53);
    OV5642_write_cmos_sensor(0x8d30, 0x12);
    OV5642_write_cmos_sensor(0x8d31, 0x05);
    OV5642_write_cmos_sensor(0x8d32, 0x25);
    OV5642_write_cmos_sensor(0x8d33, 0x80);
    OV5642_write_cmos_sensor(0x8d34, 0x4e);
    OV5642_write_cmos_sensor(0x8d35, 0x12);
    OV5642_write_cmos_sensor(0x8d36, 0x06);
    OV5642_write_cmos_sensor(0x8d37, 0xee);
    OV5642_write_cmos_sensor(0x8d38, 0x80);
    OV5642_write_cmos_sensor(0x8d39, 0x49);
    OV5642_write_cmos_sensor(0x8d3a, 0x30);
    OV5642_write_cmos_sensor(0x8d3b, 0x05);
    OV5642_write_cmos_sensor(0x8d3c, 0x46);
    OV5642_write_cmos_sensor(0x8d3d, 0x20);
    OV5642_write_cmos_sensor(0x8d3e, 0x07);
    OV5642_write_cmos_sensor(0x8d3f, 0x43);
    OV5642_write_cmos_sensor(0x8d40, 0x20);
    OV5642_write_cmos_sensor(0x8d41, 0x06);
    OV5642_write_cmos_sensor(0x8d42, 0x40);
    OV5642_write_cmos_sensor(0x8d43, 0x12);
    OV5642_write_cmos_sensor(0x8d44, 0x15);
    OV5642_write_cmos_sensor(0x8d45, 0x4b);
    OV5642_write_cmos_sensor(0x8d46, 0x80);
    OV5642_write_cmos_sensor(0x8d47, 0x3b);
    OV5642_write_cmos_sensor(0x8d48, 0x12);
    OV5642_write_cmos_sensor(0x8d49, 0x10);
    OV5642_write_cmos_sensor(0x8d4a, 0x3e);
    OV5642_write_cmos_sensor(0x8d4b, 0x80);
    OV5642_write_cmos_sensor(0x8d4c, 0x36);
    OV5642_write_cmos_sensor(0x8d4d, 0x20);
    OV5642_write_cmos_sensor(0x8d4e, 0x07);
    OV5642_write_cmos_sensor(0x8d4f, 0x33);
    OV5642_write_cmos_sensor(0x8d50, 0x20);
    OV5642_write_cmos_sensor(0x8d51, 0x06);
    OV5642_write_cmos_sensor(0x8d52, 0x30);
    OV5642_write_cmos_sensor(0x8d53, 0x12);
    OV5642_write_cmos_sensor(0x8d54, 0x15);
    OV5642_write_cmos_sensor(0x8d55, 0x5a);
    OV5642_write_cmos_sensor(0x8d56, 0x80);
    OV5642_write_cmos_sensor(0x8d57, 0x2b);
    OV5642_write_cmos_sensor(0x8d58, 0xe5);
    OV5642_write_cmos_sensor(0x8d59, 0x7e);
    OV5642_write_cmos_sensor(0x8d5a, 0x64);
    OV5642_write_cmos_sensor(0x8d5b, 0x01);
    OV5642_write_cmos_sensor(0x8d5c, 0x70);
    OV5642_write_cmos_sensor(0x8d5d, 0x25);
    OV5642_write_cmos_sensor(0x8d5e, 0xd2);
    OV5642_write_cmos_sensor(0x8d5f, 0x35);
    OV5642_write_cmos_sensor(0x8d60, 0x90);
    OV5642_write_cmos_sensor(0x8d61, 0x50);
    OV5642_write_cmos_sensor(0x8d62, 0x82);
    OV5642_write_cmos_sensor(0x8d63, 0xe5);
    OV5642_write_cmos_sensor(0x8d64, 0x74);
    OV5642_write_cmos_sensor(0x8d65, 0xf0);
    OV5642_write_cmos_sensor(0x8d66, 0xa3);
    OV5642_write_cmos_sensor(0x8d67, 0xe5);
    OV5642_write_cmos_sensor(0x8d68, 0x75);
    OV5642_write_cmos_sensor(0x8d69, 0xf0);
    OV5642_write_cmos_sensor(0x8d6a, 0xa3);
    OV5642_write_cmos_sensor(0x8d6b, 0xe5);
    OV5642_write_cmos_sensor(0x8d6c, 0x76);
    OV5642_write_cmos_sensor(0x8d6d, 0xf0);
    OV5642_write_cmos_sensor(0x8d6e, 0xa3);
    OV5642_write_cmos_sensor(0x8d6f, 0xe5);
    OV5642_write_cmos_sensor(0x8d70, 0x77);
    OV5642_write_cmos_sensor(0x8d71, 0xf0);
    OV5642_write_cmos_sensor(0x8d72, 0xc2);
    OV5642_write_cmos_sensor(0x8d73, 0x35);
    OV5642_write_cmos_sensor(0x8d74, 0x80);
    OV5642_write_cmos_sensor(0x8d75, 0x0d);
    OV5642_write_cmos_sensor(0x8d76, 0x90);
    OV5642_write_cmos_sensor(0x8d77, 0x50);
    OV5642_write_cmos_sensor(0x8d78, 0x82);
    OV5642_write_cmos_sensor(0x8d79, 0x30);
    OV5642_write_cmos_sensor(0x8d7a, 0x33);
    OV5642_write_cmos_sensor(0x8d7b, 0x05);
    OV5642_write_cmos_sensor(0x8d7c, 0x74);
    OV5642_write_cmos_sensor(0x8d7d, 0x55);
    OV5642_write_cmos_sensor(0x8d7e, 0xf0);
    OV5642_write_cmos_sensor(0x8d7f, 0x80);
    OV5642_write_cmos_sensor(0x8d80, 0x02);
    OV5642_write_cmos_sensor(0x8d81, 0xe4);
    OV5642_write_cmos_sensor(0x8d82, 0xf0);
    OV5642_write_cmos_sensor(0x8d83, 0x20);
    OV5642_write_cmos_sensor(0x8d84, 0x07);
    OV5642_write_cmos_sensor(0x8d85, 0x06);
    OV5642_write_cmos_sensor(0x8d86, 0x30);
    OV5642_write_cmos_sensor(0x8d87, 0x06);
    OV5642_write_cmos_sensor(0x8d88, 0x03);
    OV5642_write_cmos_sensor(0x8d89, 0x30);
    OV5642_write_cmos_sensor(0x8d8a, 0x04);
    OV5642_write_cmos_sensor(0x8d8b, 0x05);
    OV5642_write_cmos_sensor(0x8d8c, 0x90);
    OV5642_write_cmos_sensor(0x8d8d, 0x30);
    OV5642_write_cmos_sensor(0x8d8e, 0x25);
    OV5642_write_cmos_sensor(0x8d8f, 0xe4);
    OV5642_write_cmos_sensor(0x8d90, 0xf0);
    OV5642_write_cmos_sensor(0x8d91, 0x22);
    OV5642_write_cmos_sensor(0x8d92, 0x30);
    OV5642_write_cmos_sensor(0x8d93, 0x04);
    OV5642_write_cmos_sensor(0x8d94, 0x03);
    OV5642_write_cmos_sensor(0x8d95, 0x02);
    OV5642_write_cmos_sensor(0x8d96, 0x0e);
    OV5642_write_cmos_sensor(0x8d97, 0x72);
    OV5642_write_cmos_sensor(0x8d98, 0xd2);
    OV5642_write_cmos_sensor(0x8d99, 0x04);
    OV5642_write_cmos_sensor(0x8d9a, 0xe5);
    OV5642_write_cmos_sensor(0x8d9b, 0x7e);
    OV5642_write_cmos_sensor(0x8d9c, 0xb4);
    OV5642_write_cmos_sensor(0x8d9d, 0x01);
    OV5642_write_cmos_sensor(0x8d9e, 0x06);
    OV5642_write_cmos_sensor(0x8d9f, 0x12);
    OV5642_write_cmos_sensor(0x8da0, 0x15);
    OV5642_write_cmos_sensor(0x8da1, 0x2b);
    OV5642_write_cmos_sensor(0x8da2, 0x02);
    OV5642_write_cmos_sensor(0x8da3, 0x0e);
    OV5642_write_cmos_sensor(0x8da4, 0x6b);
    OV5642_write_cmos_sensor(0x8da5, 0xe5);
    OV5642_write_cmos_sensor(0x8da6, 0x7e);
    OV5642_write_cmos_sensor(0x8da7, 0xb4);
    OV5642_write_cmos_sensor(0x8da8, 0x02);
    OV5642_write_cmos_sensor(0x8da9, 0x06);
    OV5642_write_cmos_sensor(0x8daa, 0x12);
    OV5642_write_cmos_sensor(0x8dab, 0x15);
    OV5642_write_cmos_sensor(0x8dac, 0x3c);
    OV5642_write_cmos_sensor(0x8dad, 0x02);
    OV5642_write_cmos_sensor(0x8dae, 0x0e);
    OV5642_write_cmos_sensor(0x8daf, 0x6b);
    OV5642_write_cmos_sensor(0x8db0, 0xe5);
    OV5642_write_cmos_sensor(0x8db1, 0x7e);
    OV5642_write_cmos_sensor(0x8db2, 0xb4);
    OV5642_write_cmos_sensor(0x8db3, 0x03);
    OV5642_write_cmos_sensor(0x8db4, 0x05);
    OV5642_write_cmos_sensor(0x8db5, 0xe4);
    OV5642_write_cmos_sensor(0x8db6, 0xf5);
    OV5642_write_cmos_sensor(0x8db7, 0x0c);
    OV5642_write_cmos_sensor(0x8db8, 0x80);
    OV5642_write_cmos_sensor(0x8db9, 0x08);
    OV5642_write_cmos_sensor(0x8dba, 0xe5);
    OV5642_write_cmos_sensor(0x8dbb, 0x7e);
    OV5642_write_cmos_sensor(0x8dbc, 0xb4);
    OV5642_write_cmos_sensor(0x8dbd, 0x04);
    OV5642_write_cmos_sensor(0x8dbe, 0x09);
    OV5642_write_cmos_sensor(0x8dbf, 0x85);
    OV5642_write_cmos_sensor(0x8dc0, 0x7c);
    OV5642_write_cmos_sensor(0x8dc1, 0x0c);
    OV5642_write_cmos_sensor(0x8dc2, 0x12);
    OV5642_write_cmos_sensor(0x8dc3, 0x11);
    OV5642_write_cmos_sensor(0x8dc4, 0xd8);
    OV5642_write_cmos_sensor(0x8dc5, 0x02);
    OV5642_write_cmos_sensor(0x8dc6, 0x0e);
    OV5642_write_cmos_sensor(0x8dc7, 0x6b);
    OV5642_write_cmos_sensor(0x8dc8, 0xe5);
    OV5642_write_cmos_sensor(0x8dc9, 0x7e);
    OV5642_write_cmos_sensor(0x8dca, 0x64);
    OV5642_write_cmos_sensor(0x8dcb, 0x0f);
    OV5642_write_cmos_sensor(0x8dcc, 0x70);
    OV5642_write_cmos_sensor(0x8dcd, 0x15);
    OV5642_write_cmos_sensor(0x8dce, 0x12);
    OV5642_write_cmos_sensor(0x8dcf, 0x02);
    OV5642_write_cmos_sensor(0x8dd0, 0xa9);
    OV5642_write_cmos_sensor(0x8dd1, 0x40);
    OV5642_write_cmos_sensor(0x8dd2, 0x06);
    OV5642_write_cmos_sensor(0x8dd3, 0x7e);
    OV5642_write_cmos_sensor(0x8dd4, 0x03);
    OV5642_write_cmos_sensor(0x8dd5, 0x7f);
    OV5642_write_cmos_sensor(0x8dd6, 0xff);
    OV5642_write_cmos_sensor(0x8dd7, 0x80);
    OV5642_write_cmos_sensor(0x8dd8, 0x04);
    OV5642_write_cmos_sensor(0x8dd9, 0xae);
    OV5642_write_cmos_sensor(0x8dda, 0x68);
    OV5642_write_cmos_sensor(0x8ddb, 0xaf);
    OV5642_write_cmos_sensor(0x8ddc, 0x69);
    OV5642_write_cmos_sensor(0x8ddd, 0x12);
    OV5642_write_cmos_sensor(0x8dde, 0x0e);
    OV5642_write_cmos_sensor(0x8ddf, 0x73);
    OV5642_write_cmos_sensor(0x8de0, 0x02);
    OV5642_write_cmos_sensor(0x8de1, 0x0e);
    OV5642_write_cmos_sensor(0x8de2, 0x6b);
    OV5642_write_cmos_sensor(0x8de3, 0xe5);
    OV5642_write_cmos_sensor(0x8de4, 0x7e);
    OV5642_write_cmos_sensor(0x8de5, 0x64);
    OV5642_write_cmos_sensor(0x8de6, 0x10);
    OV5642_write_cmos_sensor(0x8de7, 0x60);
    OV5642_write_cmos_sensor(0x8de8, 0x03);
    OV5642_write_cmos_sensor(0x8de9, 0x02);
    OV5642_write_cmos_sensor(0x8dea, 0x0e);
    OV5642_write_cmos_sensor(0x8deb, 0x6b);
    OV5642_write_cmos_sensor(0x8dec, 0xf5);
    OV5642_write_cmos_sensor(0x8ded, 0x66);
    OV5642_write_cmos_sensor(0x8dee, 0xf5);
    OV5642_write_cmos_sensor(0x8def, 0x67);
    OV5642_write_cmos_sensor(0x8df0, 0xf5);
    OV5642_write_cmos_sensor(0x8df1, 0x68);
    OV5642_write_cmos_sensor(0x8df2, 0xab);
    OV5642_write_cmos_sensor(0x8df3, 0x69);
    OV5642_write_cmos_sensor(0x8df4, 0xaa);
    OV5642_write_cmos_sensor(0x8df5, 0x68);
    OV5642_write_cmos_sensor(0x8df6, 0xa9);
    OV5642_write_cmos_sensor(0x8df7, 0x67);
    OV5642_write_cmos_sensor(0x8df8, 0xa8);
    OV5642_write_cmos_sensor(0x8df9, 0x66);
    OV5642_write_cmos_sensor(0x8dfa, 0x12);
    OV5642_write_cmos_sensor(0x8dfb, 0x01);
    OV5642_write_cmos_sensor(0x8dfc, 0x73);
    OV5642_write_cmos_sensor(0x8dfd, 0xfe);
    OV5642_write_cmos_sensor(0x8dfe, 0xe4);
    OV5642_write_cmos_sensor(0x8dff, 0xfc);
    OV5642_write_cmos_sensor(0x8e00, 0xfd);
    OV5642_write_cmos_sensor(0x8e01, 0x12);
    OV5642_write_cmos_sensor(0x8e02, 0x01);
    OV5642_write_cmos_sensor(0x8e03, 0xf1);
    OV5642_write_cmos_sensor(0x8e04, 0xe4);
    OV5642_write_cmos_sensor(0x8e05, 0x7b);
    OV5642_write_cmos_sensor(0x8e06, 0xff);
    OV5642_write_cmos_sensor(0x8e07, 0xfa);
    OV5642_write_cmos_sensor(0x8e08, 0xf9);
    OV5642_write_cmos_sensor(0x8e09, 0xf8);
    OV5642_write_cmos_sensor(0x8e0a, 0x12);
    OV5642_write_cmos_sensor(0x8e0b, 0x02);
    OV5642_write_cmos_sensor(0x8e0c, 0x67);
    OV5642_write_cmos_sensor(0x8e0d, 0x12);
    OV5642_write_cmos_sensor(0x8e0e, 0x02);
    OV5642_write_cmos_sensor(0x8e0f, 0x39);
    OV5642_write_cmos_sensor(0x8e10, 0xe4);
    OV5642_write_cmos_sensor(0x8e11, 0x93);
    OV5642_write_cmos_sensor(0x8e12, 0xfe);
    OV5642_write_cmos_sensor(0x8e13, 0x74);
    OV5642_write_cmos_sensor(0x8e14, 0x01);
    OV5642_write_cmos_sensor(0x8e15, 0x93);
    OV5642_write_cmos_sensor(0x8e16, 0xff);
    OV5642_write_cmos_sensor(0x8e17, 0xe4);
    OV5642_write_cmos_sensor(0x8e18, 0xfc);
    OV5642_write_cmos_sensor(0x8e19, 0xfd);
    OV5642_write_cmos_sensor(0x8e1a, 0xe5);
    OV5642_write_cmos_sensor(0x8e1b, 0x69);
    OV5642_write_cmos_sensor(0x8e1c, 0x2f);
    OV5642_write_cmos_sensor(0x8e1d, 0xf5);
    OV5642_write_cmos_sensor(0x8e1e, 0x69);
    OV5642_write_cmos_sensor(0x8e1f, 0xe5);
    OV5642_write_cmos_sensor(0x8e20, 0x68);
    OV5642_write_cmos_sensor(0x8e21, 0x3e);
    OV5642_write_cmos_sensor(0x8e22, 0xf5);
    OV5642_write_cmos_sensor(0x8e23, 0x68);
    OV5642_write_cmos_sensor(0x8e24, 0xed);
    OV5642_write_cmos_sensor(0x8e25, 0x35);
    OV5642_write_cmos_sensor(0x8e26, 0x67);
    OV5642_write_cmos_sensor(0x8e27, 0xf5);
    OV5642_write_cmos_sensor(0x8e28, 0x67);
    OV5642_write_cmos_sensor(0x8e29, 0xec);
    OV5642_write_cmos_sensor(0x8e2a, 0x35);
    OV5642_write_cmos_sensor(0x8e2b, 0x66);
    OV5642_write_cmos_sensor(0x8e2c, 0xf5);
    OV5642_write_cmos_sensor(0x8e2d, 0x66);
    OV5642_write_cmos_sensor(0x8e2e, 0x12);
    OV5642_write_cmos_sensor(0x8e2f, 0x02);
    OV5642_write_cmos_sensor(0x8e30, 0xa9);
    OV5642_write_cmos_sensor(0x8e31, 0x40);
    OV5642_write_cmos_sensor(0x8e32, 0x06);
    OV5642_write_cmos_sensor(0x8e33, 0x7e);
    OV5642_write_cmos_sensor(0x8e34, 0x03);
    OV5642_write_cmos_sensor(0x8e35, 0x7f);
    OV5642_write_cmos_sensor(0x8e36, 0xff);
    OV5642_write_cmos_sensor(0x8e37, 0x80);
    OV5642_write_cmos_sensor(0x8e38, 0x04);
    OV5642_write_cmos_sensor(0x8e39, 0xae);
    OV5642_write_cmos_sensor(0x8e3a, 0x68);
    OV5642_write_cmos_sensor(0x8e3b, 0xaf);
    OV5642_write_cmos_sensor(0x8e3c, 0x69);
    OV5642_write_cmos_sensor(0x8e3d, 0x12);
    OV5642_write_cmos_sensor(0x8e3e, 0x0e);
    OV5642_write_cmos_sensor(0x8e3f, 0x73);
    OV5642_write_cmos_sensor(0x8e40, 0xe4);
    OV5642_write_cmos_sensor(0x8e41, 0xf5);
    OV5642_write_cmos_sensor(0x8e42, 0x67);
    OV5642_write_cmos_sensor(0x8e43, 0xf5);
    OV5642_write_cmos_sensor(0x8e44, 0x67);
    OV5642_write_cmos_sensor(0x8e45, 0xe5);
    OV5642_write_cmos_sensor(0x8e46, 0x67);
    OV5642_write_cmos_sensor(0x8e47, 0xd3);
    OV5642_write_cmos_sensor(0x8e48, 0x95);
    OV5642_write_cmos_sensor(0x8e49, 0x7c);
    OV5642_write_cmos_sensor(0x8e4a, 0x50);
    OV5642_write_cmos_sensor(0x8e4b, 0x1c);
    OV5642_write_cmos_sensor(0x8e4c, 0x12);
    OV5642_write_cmos_sensor(0x8e4d, 0x02);
    OV5642_write_cmos_sensor(0x8e4e, 0x39);
    OV5642_write_cmos_sensor(0x8e4f, 0xaf);
    OV5642_write_cmos_sensor(0x8e50, 0x67);
    OV5642_write_cmos_sensor(0x8e51, 0x75);
    OV5642_write_cmos_sensor(0x8e52, 0xf0);
    OV5642_write_cmos_sensor(0x8e53, 0x02);
    OV5642_write_cmos_sensor(0x8e54, 0xef);
    OV5642_write_cmos_sensor(0x8e55, 0x12);
    OV5642_write_cmos_sensor(0x8e56, 0x09);
    OV5642_write_cmos_sensor(0x8e57, 0xe1);
    OV5642_write_cmos_sensor(0x8e58, 0xc3);
    OV5642_write_cmos_sensor(0x8e59, 0x74);
    OV5642_write_cmos_sensor(0x8e5a, 0x01);
    OV5642_write_cmos_sensor(0x8e5b, 0x93);
    OV5642_write_cmos_sensor(0x8e5c, 0x95);
    OV5642_write_cmos_sensor(0x8e5d, 0x69);
    OV5642_write_cmos_sensor(0x8e5e, 0xe4);
    OV5642_write_cmos_sensor(0x8e5f, 0x93);
    OV5642_write_cmos_sensor(0x8e60, 0x95);
    OV5642_write_cmos_sensor(0x8e61, 0x68);
    OV5642_write_cmos_sensor(0x8e62, 0x50);
    OV5642_write_cmos_sensor(0x8e63, 0x04);
    OV5642_write_cmos_sensor(0x8e64, 0x05);
    OV5642_write_cmos_sensor(0x8e65, 0x67);
    OV5642_write_cmos_sensor(0x8e66, 0x80);
    OV5642_write_cmos_sensor(0x8e67, 0xdd);
    OV5642_write_cmos_sensor(0x8e68, 0x85);
    OV5642_write_cmos_sensor(0x8e69, 0x67);
    OV5642_write_cmos_sensor(0x8e6a, 0x7d);
    OV5642_write_cmos_sensor(0x8e6b, 0x90);
    OV5642_write_cmos_sensor(0x8e6c, 0x30);
    OV5642_write_cmos_sensor(0x8e6d, 0x25);
    OV5642_write_cmos_sensor(0x8e6e, 0xe4);
    OV5642_write_cmos_sensor(0x8e6f, 0xf0);
    OV5642_write_cmos_sensor(0x8e70, 0xd2);
    OV5642_write_cmos_sensor(0x8e71, 0x34);
    OV5642_write_cmos_sensor(0x8e72, 0x22);
    OV5642_write_cmos_sensor(0x8e73, 0x8e);
    OV5642_write_cmos_sensor(0x8e74, 0x68);
    OV5642_write_cmos_sensor(0x8e75, 0x8f);
    OV5642_write_cmos_sensor(0x8e76, 0x69);
    OV5642_write_cmos_sensor(0x8e77, 0x85);
    OV5642_write_cmos_sensor(0x8e78, 0x68);
    OV5642_write_cmos_sensor(0x8e79, 0x64);
    OV5642_write_cmos_sensor(0x8e7a, 0x85);
    OV5642_write_cmos_sensor(0x8e7b, 0x69);
    OV5642_write_cmos_sensor(0x8e7c, 0x65);
    OV5642_write_cmos_sensor(0x8e7d, 0xe5);
    OV5642_write_cmos_sensor(0x8e7e, 0x69);
    OV5642_write_cmos_sensor(0x8e7f, 0xc4);
    OV5642_write_cmos_sensor(0x8e80, 0xf8);
    OV5642_write_cmos_sensor(0x8e81, 0x54);
    OV5642_write_cmos_sensor(0x8e82, 0x0f);
    OV5642_write_cmos_sensor(0x8e83, 0xc8);
    OV5642_write_cmos_sensor(0x8e84, 0x68);
    OV5642_write_cmos_sensor(0x8e85, 0xf5);
    OV5642_write_cmos_sensor(0x8e86, 0x69);
    OV5642_write_cmos_sensor(0x8e87, 0xe5);
    OV5642_write_cmos_sensor(0x8e88, 0x68);
    OV5642_write_cmos_sensor(0x8e89, 0xc4);
    OV5642_write_cmos_sensor(0x8e8a, 0x54);
    OV5642_write_cmos_sensor(0x8e8b, 0xf0);
    OV5642_write_cmos_sensor(0x8e8c, 0x48);
    OV5642_write_cmos_sensor(0x8e8d, 0xf5);
    OV5642_write_cmos_sensor(0x8e8e, 0x68);
    OV5642_write_cmos_sensor(0x8e8f, 0x85);
    OV5642_write_cmos_sensor(0x8e90, 0x68);
    OV5642_write_cmos_sensor(0x8e91, 0x0d);
    OV5642_write_cmos_sensor(0x8e92, 0x85);
    OV5642_write_cmos_sensor(0x8e93, 0x69);
    OV5642_write_cmos_sensor(0x8e94, 0x0e);
    OV5642_write_cmos_sensor(0x8e95, 0x12);
    OV5642_write_cmos_sensor(0x8e96, 0x0a);
    OV5642_write_cmos_sensor(0x8e97, 0x13);
    OV5642_write_cmos_sensor(0x8e98, 0x22);
    OV5642_write_cmos_sensor(0x8e99, 0x12);
    OV5642_write_cmos_sensor(0x8e9a, 0x02);
    OV5642_write_cmos_sensor(0x8e9b, 0x7f);
    OV5642_write_cmos_sensor(0x8e9c, 0xb5);
    OV5642_write_cmos_sensor(0x8e9d, 0x07);
    OV5642_write_cmos_sensor(0x8e9e, 0x03);
    OV5642_write_cmos_sensor(0x8e9f, 0xd3);
    OV5642_write_cmos_sensor(0x8ea0, 0x80);
    OV5642_write_cmos_sensor(0x8ea1, 0x01);
    OV5642_write_cmos_sensor(0x8ea2, 0xc3);
    OV5642_write_cmos_sensor(0x8ea3, 0x40);
    OV5642_write_cmos_sensor(0x8ea4, 0x03);
    OV5642_write_cmos_sensor(0x8ea5, 0x02);
    OV5642_write_cmos_sensor(0x8ea6, 0x0f);
    OV5642_write_cmos_sensor(0x8ea7, 0x38);
    OV5642_write_cmos_sensor(0x8ea8, 0x90);
    OV5642_write_cmos_sensor(0x8ea9, 0x30);
    OV5642_write_cmos_sensor(0x8eaa, 0x04);
    OV5642_write_cmos_sensor(0x8eab, 0xe0);
    OV5642_write_cmos_sensor(0x8eac, 0x44);
    OV5642_write_cmos_sensor(0x8ead, 0x20);
    OV5642_write_cmos_sensor(0x8eae, 0xf0);
    OV5642_write_cmos_sensor(0x8eaf, 0xa3);
    OV5642_write_cmos_sensor(0x8eb0, 0xe0);
    OV5642_write_cmos_sensor(0x8eb1, 0x44);
    OV5642_write_cmos_sensor(0x8eb2, 0x40);
    OV5642_write_cmos_sensor(0x8eb3, 0xf0);
    OV5642_write_cmos_sensor(0x8eb4, 0x90);
    OV5642_write_cmos_sensor(0x8eb5, 0x50);
    OV5642_write_cmos_sensor(0x8eb6, 0x25);
    OV5642_write_cmos_sensor(0x8eb7, 0xe0);
    OV5642_write_cmos_sensor(0x8eb8, 0x44);
    OV5642_write_cmos_sensor(0x8eb9, 0x04);
    OV5642_write_cmos_sensor(0x8eba, 0xf0);
    OV5642_write_cmos_sensor(0x8ebb, 0x90);
    OV5642_write_cmos_sensor(0x8ebc, 0x50);
    OV5642_write_cmos_sensor(0x8ebd, 0x03);
    OV5642_write_cmos_sensor(0x8ebe, 0xe0);
    OV5642_write_cmos_sensor(0x8ebf, 0x54);
    OV5642_write_cmos_sensor(0x8ec0, 0xfd);
    OV5642_write_cmos_sensor(0x8ec1, 0xf0);
    OV5642_write_cmos_sensor(0x8ec2, 0x90);
    OV5642_write_cmos_sensor(0x8ec3, 0x50);
    OV5642_write_cmos_sensor(0x8ec4, 0x27);
    OV5642_write_cmos_sensor(0x8ec5, 0xe0);
    OV5642_write_cmos_sensor(0x8ec6, 0x44);
    OV5642_write_cmos_sensor(0x8ec7, 0x01);
    OV5642_write_cmos_sensor(0x8ec8, 0xf0);
    OV5642_write_cmos_sensor(0x8ec9, 0x90);
    OV5642_write_cmos_sensor(0x8eca, 0x50);
    OV5642_write_cmos_sensor(0x8ecb, 0x31);
    OV5642_write_cmos_sensor(0x8ecc, 0xe4);
    OV5642_write_cmos_sensor(0x8ecd, 0xf0);
    OV5642_write_cmos_sensor(0x8ece, 0x90);
    OV5642_write_cmos_sensor(0x8ecf, 0x50);
    OV5642_write_cmos_sensor(0x8ed0, 0x33);
    OV5642_write_cmos_sensor(0x8ed1, 0xf0);
    OV5642_write_cmos_sensor(0x8ed2, 0x90);
    OV5642_write_cmos_sensor(0x8ed3, 0x30);
    OV5642_write_cmos_sensor(0x8ed4, 0x1e);
    OV5642_write_cmos_sensor(0x8ed5, 0x12);
    OV5642_write_cmos_sensor(0x8ed6, 0x02);
    OV5642_write_cmos_sensor(0x8ed7, 0x05);
    OV5642_write_cmos_sensor(0x8ed8, 0x90);
    OV5642_write_cmos_sensor(0x8ed9, 0x30);
    OV5642_write_cmos_sensor(0x8eda, 0x18);
    OV5642_write_cmos_sensor(0x8edb, 0x12);
    OV5642_write_cmos_sensor(0x8edc, 0x02);
    OV5642_write_cmos_sensor(0x8edd, 0x05);
    OV5642_write_cmos_sensor(0x8ede, 0x90);
    OV5642_write_cmos_sensor(0x8edf, 0x30);
    OV5642_write_cmos_sensor(0x8ee0, 0x1b);
    OV5642_write_cmos_sensor(0x8ee1, 0x12);
    OV5642_write_cmos_sensor(0x8ee2, 0x02);
    OV5642_write_cmos_sensor(0x8ee3, 0x05);
    OV5642_write_cmos_sensor(0x8ee4, 0xe0);
    OV5642_write_cmos_sensor(0x8ee5, 0xf5);
    OV5642_write_cmos_sensor(0x8ee6, 0x25);
    OV5642_write_cmos_sensor(0x8ee7, 0x90);
    OV5642_write_cmos_sensor(0x8ee8, 0x30);
    OV5642_write_cmos_sensor(0x8ee9, 0x18);
    OV5642_write_cmos_sensor(0x8eea, 0xe0);
    OV5642_write_cmos_sensor(0x8eeb, 0xf5);
    OV5642_write_cmos_sensor(0x8eec, 0x21);
    OV5642_write_cmos_sensor(0x8eed, 0x90);
    OV5642_write_cmos_sensor(0x8eee, 0x60);
    OV5642_write_cmos_sensor(0x8eef, 0x00);
    OV5642_write_cmos_sensor(0x8ef0, 0x74);
    OV5642_write_cmos_sensor(0x8ef1, 0xf5);
    OV5642_write_cmos_sensor(0x8ef2, 0xf0);
    OV5642_write_cmos_sensor(0x8ef3, 0x90);
    OV5642_write_cmos_sensor(0x8ef4, 0x3f);
    OV5642_write_cmos_sensor(0x8ef5, 0x01);
    OV5642_write_cmos_sensor(0x8ef6, 0xe4);
    OV5642_write_cmos_sensor(0x8ef7, 0xf0);
    OV5642_write_cmos_sensor(0x8ef8, 0xa3);
    OV5642_write_cmos_sensor(0x8ef9, 0xf0);
    OV5642_write_cmos_sensor(0x8efa, 0x90);
    OV5642_write_cmos_sensor(0x8efb, 0x3f);
    OV5642_write_cmos_sensor(0x8efc, 0x01);
    OV5642_write_cmos_sensor(0x8efd, 0xe0);
    OV5642_write_cmos_sensor(0x8efe, 0x44);
    OV5642_write_cmos_sensor(0x8eff, 0x08);
    OV5642_write_cmos_sensor(0x8f00, 0xf0);
    OV5642_write_cmos_sensor(0x8f01, 0xe0);
    OV5642_write_cmos_sensor(0x8f02, 0x44);
    OV5642_write_cmos_sensor(0x8f03, 0x20);
    OV5642_write_cmos_sensor(0x8f04, 0xf0);
    OV5642_write_cmos_sensor(0x8f05, 0x90);
    OV5642_write_cmos_sensor(0x8f06, 0x3f);
    OV5642_write_cmos_sensor(0x8f07, 0x05);
    OV5642_write_cmos_sensor(0x8f08, 0x74);
    OV5642_write_cmos_sensor(0x8f09, 0x30);
    OV5642_write_cmos_sensor(0x8f0a, 0xf0);
    OV5642_write_cmos_sensor(0x8f0b, 0xa3);
    OV5642_write_cmos_sensor(0x8f0c, 0x74);
    OV5642_write_cmos_sensor(0x8f0d, 0x24);
    OV5642_write_cmos_sensor(0x8f0e, 0xf0);
    OV5642_write_cmos_sensor(0x8f0f, 0x90);
    OV5642_write_cmos_sensor(0x8f10, 0x3f);
    OV5642_write_cmos_sensor(0x8f11, 0x0b);
    OV5642_write_cmos_sensor(0x8f12, 0xe0);
    OV5642_write_cmos_sensor(0x8f13, 0x44);
    OV5642_write_cmos_sensor(0x8f14, 0x0f);
    OV5642_write_cmos_sensor(0x8f15, 0xf0);
    OV5642_write_cmos_sensor(0x8f16, 0x90);
    OV5642_write_cmos_sensor(0x8f17, 0x3f);
    OV5642_write_cmos_sensor(0x8f18, 0x01);
    OV5642_write_cmos_sensor(0x8f19, 0xe0);
    OV5642_write_cmos_sensor(0x8f1a, 0x44);
    OV5642_write_cmos_sensor(0x8f1b, 0x02);
    OV5642_write_cmos_sensor(0x8f1c, 0xf0);
    OV5642_write_cmos_sensor(0x8f1d, 0xc2);
    OV5642_write_cmos_sensor(0x8f1e, 0x8c);
    OV5642_write_cmos_sensor(0x8f1f, 0x75);
    OV5642_write_cmos_sensor(0x8f20, 0x89);
    OV5642_write_cmos_sensor(0x8f21, 0x03);
    OV5642_write_cmos_sensor(0x8f22, 0x75);
    OV5642_write_cmos_sensor(0x8f23, 0xa8);
    OV5642_write_cmos_sensor(0x8f24, 0x07);
    OV5642_write_cmos_sensor(0x8f25, 0x75);
    OV5642_write_cmos_sensor(0x8f26, 0xb8);
    OV5642_write_cmos_sensor(0x8f27, 0x04);
    OV5642_write_cmos_sensor(0x8f28, 0xe4);
    OV5642_write_cmos_sensor(0x8f29, 0xf5);
    OV5642_write_cmos_sensor(0x8f2a, 0xd8);
    OV5642_write_cmos_sensor(0x8f2b, 0xf5);
    OV5642_write_cmos_sensor(0x8f2c, 0xe8);
    OV5642_write_cmos_sensor(0x8f2d, 0x90);
    OV5642_write_cmos_sensor(0x8f2e, 0x30);
    OV5642_write_cmos_sensor(0x8f2f, 0x01);
    OV5642_write_cmos_sensor(0x8f30, 0xe0);
    OV5642_write_cmos_sensor(0x8f31, 0x44);
    OV5642_write_cmos_sensor(0x8f32, 0x40);
    OV5642_write_cmos_sensor(0x8f33, 0xf0);
    OV5642_write_cmos_sensor(0x8f34, 0xe0);
    OV5642_write_cmos_sensor(0x8f35, 0x54);
    OV5642_write_cmos_sensor(0x8f36, 0xbf);
    OV5642_write_cmos_sensor(0x8f37, 0xf0);
    OV5642_write_cmos_sensor(0x8f38, 0x22);
    OV5642_write_cmos_sensor(0x8f39, 0x30);
    OV5642_write_cmos_sensor(0x8f3a, 0x3c);
    OV5642_write_cmos_sensor(0x8f3b, 0x09);
    OV5642_write_cmos_sensor(0x8f3c, 0x30);
    OV5642_write_cmos_sensor(0x8f3d, 0x20);
    OV5642_write_cmos_sensor(0x8f3e, 0x06);
    OV5642_write_cmos_sensor(0x8f3f, 0xae);
    OV5642_write_cmos_sensor(0x8f40, 0x56);
    OV5642_write_cmos_sensor(0x8f41, 0xaf);
    OV5642_write_cmos_sensor(0x8f42, 0x57);
    OV5642_write_cmos_sensor(0x8f43, 0x80);
    OV5642_write_cmos_sensor(0x8f44, 0x04);
    OV5642_write_cmos_sensor(0x8f45, 0xae);
    OV5642_write_cmos_sensor(0x8f46, 0x6a);
    OV5642_write_cmos_sensor(0x8f47, 0xaf);
    OV5642_write_cmos_sensor(0x8f48, 0x6b);
    OV5642_write_cmos_sensor(0x8f49, 0x8e);
    OV5642_write_cmos_sensor(0x8f4a, 0x56);
    OV5642_write_cmos_sensor(0x8f4b, 0x8f);
    OV5642_write_cmos_sensor(0x8f4c, 0x57);
    OV5642_write_cmos_sensor(0x8f4d, 0x30);
    OV5642_write_cmos_sensor(0x8f4e, 0x3c);
    OV5642_write_cmos_sensor(0x8f4f, 0x09);
    OV5642_write_cmos_sensor(0x8f50, 0x30);
    OV5642_write_cmos_sensor(0x8f51, 0x21);
    OV5642_write_cmos_sensor(0x8f52, 0x06);
    OV5642_write_cmos_sensor(0x8f53, 0xae);
    OV5642_write_cmos_sensor(0x8f54, 0x58);
    OV5642_write_cmos_sensor(0x8f55, 0xaf);
    OV5642_write_cmos_sensor(0x8f56, 0x59);
    OV5642_write_cmos_sensor(0x8f57, 0x80);
    OV5642_write_cmos_sensor(0x8f58, 0x04);
    OV5642_write_cmos_sensor(0x8f59, 0xae);
    OV5642_write_cmos_sensor(0x8f5a, 0x6c);
    OV5642_write_cmos_sensor(0x8f5b, 0xaf);
    OV5642_write_cmos_sensor(0x8f5c, 0x6d);
    OV5642_write_cmos_sensor(0x8f5d, 0x8e);
    OV5642_write_cmos_sensor(0x8f5e, 0x58);
    OV5642_write_cmos_sensor(0x8f5f, 0x8f);
    OV5642_write_cmos_sensor(0x8f60, 0x59);
    OV5642_write_cmos_sensor(0x8f61, 0x30);
    OV5642_write_cmos_sensor(0x8f62, 0x3c);
    OV5642_write_cmos_sensor(0x8f63, 0x09);
    OV5642_write_cmos_sensor(0x8f64, 0x30);
    OV5642_write_cmos_sensor(0x8f65, 0x22);
    OV5642_write_cmos_sensor(0x8f66, 0x06);
    OV5642_write_cmos_sensor(0x8f67, 0xae);
    OV5642_write_cmos_sensor(0x8f68, 0x5a);
    OV5642_write_cmos_sensor(0x8f69, 0xaf);
    OV5642_write_cmos_sensor(0x8f6a, 0x5b);
    OV5642_write_cmos_sensor(0x8f6b, 0x80);
    OV5642_write_cmos_sensor(0x8f6c, 0x04);
    OV5642_write_cmos_sensor(0x8f6d, 0xae);
    OV5642_write_cmos_sensor(0x8f6e, 0x6e);
    OV5642_write_cmos_sensor(0x8f6f, 0xaf);
    OV5642_write_cmos_sensor(0x8f70, 0x6f);
    OV5642_write_cmos_sensor(0x8f71, 0x8e);
    OV5642_write_cmos_sensor(0x8f72, 0x5a);
    OV5642_write_cmos_sensor(0x8f73, 0x8f);
    OV5642_write_cmos_sensor(0x8f74, 0x5b);
    OV5642_write_cmos_sensor(0x8f75, 0x30);
    OV5642_write_cmos_sensor(0x8f76, 0x3c);
    OV5642_write_cmos_sensor(0x8f77, 0x09);
    OV5642_write_cmos_sensor(0x8f78, 0x30);
    OV5642_write_cmos_sensor(0x8f79, 0x23);
    OV5642_write_cmos_sensor(0x8f7a, 0x06);
    OV5642_write_cmos_sensor(0x8f7b, 0xae);
    OV5642_write_cmos_sensor(0x8f7c, 0x5c);
    OV5642_write_cmos_sensor(0x8f7d, 0xaf);
    OV5642_write_cmos_sensor(0x8f7e, 0x5d);
    OV5642_write_cmos_sensor(0x8f7f, 0x80);
    OV5642_write_cmos_sensor(0x8f80, 0x04);
    OV5642_write_cmos_sensor(0x8f81, 0xae);
    OV5642_write_cmos_sensor(0x8f82, 0x70);
    OV5642_write_cmos_sensor(0x8f83, 0xaf);
    OV5642_write_cmos_sensor(0x8f84, 0x71);
    OV5642_write_cmos_sensor(0x8f85, 0x8e);
    OV5642_write_cmos_sensor(0x8f86, 0x5c);
    OV5642_write_cmos_sensor(0x8f87, 0x8f);
    OV5642_write_cmos_sensor(0x8f88, 0x5d);
    OV5642_write_cmos_sensor(0x8f89, 0x30);
    OV5642_write_cmos_sensor(0x8f8a, 0x3c);
    OV5642_write_cmos_sensor(0x8f8b, 0x09);
    OV5642_write_cmos_sensor(0x8f8c, 0x30);
    OV5642_write_cmos_sensor(0x8f8d, 0x24);
    OV5642_write_cmos_sensor(0x8f8e, 0x06);
    OV5642_write_cmos_sensor(0x8f8f, 0xae);
    OV5642_write_cmos_sensor(0x8f90, 0x5e);
    OV5642_write_cmos_sensor(0x8f91, 0xaf);
    OV5642_write_cmos_sensor(0x8f92, 0x5f);
    OV5642_write_cmos_sensor(0x8f93, 0x80);
    OV5642_write_cmos_sensor(0x8f94, 0x04);
    OV5642_write_cmos_sensor(0x8f95, 0xae);
    OV5642_write_cmos_sensor(0x8f96, 0x72);
    OV5642_write_cmos_sensor(0x8f97, 0xaf);
    OV5642_write_cmos_sensor(0x8f98, 0x73);
    OV5642_write_cmos_sensor(0x8f99, 0x8e);
    OV5642_write_cmos_sensor(0x8f9a, 0x5e);
    OV5642_write_cmos_sensor(0x8f9b, 0x8f);
    OV5642_write_cmos_sensor(0x8f9c, 0x5f);
    OV5642_write_cmos_sensor(0x8f9d, 0x30);
    OV5642_write_cmos_sensor(0x8f9e, 0x3c);
    OV5642_write_cmos_sensor(0x8f9f, 0x09);
    OV5642_write_cmos_sensor(0x8fa0, 0x30);
    OV5642_write_cmos_sensor(0x8fa1, 0x25);
    OV5642_write_cmos_sensor(0x8fa2, 0x06);
    OV5642_write_cmos_sensor(0x8fa3, 0xae);
    OV5642_write_cmos_sensor(0x8fa4, 0x60);
    OV5642_write_cmos_sensor(0x8fa5, 0xaf);
    OV5642_write_cmos_sensor(0x8fa6, 0x61);
    OV5642_write_cmos_sensor(0x8fa7, 0x80);
    OV5642_write_cmos_sensor(0x8fa8, 0x04);
    OV5642_write_cmos_sensor(0x8fa9, 0xae);
    OV5642_write_cmos_sensor(0x8faa, 0x74);
    OV5642_write_cmos_sensor(0x8fab, 0xaf);
    OV5642_write_cmos_sensor(0x8fac, 0x75);
    OV5642_write_cmos_sensor(0x8fad, 0x8e);
    OV5642_write_cmos_sensor(0x8fae, 0x60);
    OV5642_write_cmos_sensor(0x8faf, 0x8f);
    OV5642_write_cmos_sensor(0x8fb0, 0x61);
    OV5642_write_cmos_sensor(0x8fb1, 0x30);
    OV5642_write_cmos_sensor(0x8fb2, 0x3c);
    OV5642_write_cmos_sensor(0x8fb3, 0x09);
    OV5642_write_cmos_sensor(0x8fb4, 0x30);
    OV5642_write_cmos_sensor(0x8fb5, 0x26);
    OV5642_write_cmos_sensor(0x8fb6, 0x06);
    OV5642_write_cmos_sensor(0x8fb7, 0xae);
    OV5642_write_cmos_sensor(0x8fb8, 0x62);
    OV5642_write_cmos_sensor(0x8fb9, 0xaf);
    OV5642_write_cmos_sensor(0x8fba, 0x63);
    OV5642_write_cmos_sensor(0x8fbb, 0x80);
    OV5642_write_cmos_sensor(0x8fbc, 0x04);
    OV5642_write_cmos_sensor(0x8fbd, 0xae);
    OV5642_write_cmos_sensor(0x8fbe, 0x76);
    OV5642_write_cmos_sensor(0x8fbf, 0xaf);
    OV5642_write_cmos_sensor(0x8fc0, 0x77);
    OV5642_write_cmos_sensor(0x8fc1, 0x8e);
    OV5642_write_cmos_sensor(0x8fc2, 0x62);
    OV5642_write_cmos_sensor(0x8fc3, 0x8f);
    OV5642_write_cmos_sensor(0x8fc4, 0x63);
    OV5642_write_cmos_sensor(0x8fc5, 0x22);
    OV5642_write_cmos_sensor(0x8fc6, 0xd3);
    OV5642_write_cmos_sensor(0x8fc7, 0xe5);
    OV5642_write_cmos_sensor(0x8fc8, 0x57);
    OV5642_write_cmos_sensor(0x8fc9, 0x95);
    OV5642_write_cmos_sensor(0x8fca, 0x6b);
    OV5642_write_cmos_sensor(0x8fcb, 0xe5);
    OV5642_write_cmos_sensor(0x8fcc, 0x56);
    OV5642_write_cmos_sensor(0x8fcd, 0x95);
    OV5642_write_cmos_sensor(0x8fce, 0x6a);
    OV5642_write_cmos_sensor(0x8fcf, 0x40);
    OV5642_write_cmos_sensor(0x8fd0, 0x03);
    OV5642_write_cmos_sensor(0x8fd1, 0xd3);
    OV5642_write_cmos_sensor(0x8fd2, 0x80);
    OV5642_write_cmos_sensor(0x8fd3, 0x01);
    OV5642_write_cmos_sensor(0x8fd4, 0xc3);
    OV5642_write_cmos_sensor(0x8fd5, 0x92);
    OV5642_write_cmos_sensor(0x8fd6, 0x20);
    OV5642_write_cmos_sensor(0x8fd7, 0xd3);
    OV5642_write_cmos_sensor(0x8fd8, 0xe5);
    OV5642_write_cmos_sensor(0x8fd9, 0x59);
    OV5642_write_cmos_sensor(0x8fda, 0x95);
    OV5642_write_cmos_sensor(0x8fdb, 0x6d);
    OV5642_write_cmos_sensor(0x8fdc, 0xe5);
    OV5642_write_cmos_sensor(0x8fdd, 0x58);
    OV5642_write_cmos_sensor(0x8fde, 0x95);
    OV5642_write_cmos_sensor(0x8fdf, 0x6c);
    OV5642_write_cmos_sensor(0x8fe0, 0x40);
    OV5642_write_cmos_sensor(0x8fe1, 0x03);
    OV5642_write_cmos_sensor(0x8fe2, 0xd3);
    OV5642_write_cmos_sensor(0x8fe3, 0x80);
    OV5642_write_cmos_sensor(0x8fe4, 0x01);
    OV5642_write_cmos_sensor(0x8fe5, 0xc3);
    OV5642_write_cmos_sensor(0x8fe6, 0x92);
    OV5642_write_cmos_sensor(0x8fe7, 0x21);
    OV5642_write_cmos_sensor(0x8fe8, 0xd3);
    OV5642_write_cmos_sensor(0x8fe9, 0xe5);
    OV5642_write_cmos_sensor(0x8fea, 0x5b);
    OV5642_write_cmos_sensor(0x8feb, 0x95);
    OV5642_write_cmos_sensor(0x8fec, 0x6f);
    OV5642_write_cmos_sensor(0x8fed, 0xe5);
    OV5642_write_cmos_sensor(0x8fee, 0x5a);
    OV5642_write_cmos_sensor(0x8fef, 0x95);
    OV5642_write_cmos_sensor(0x8ff0, 0x6e);
    OV5642_write_cmos_sensor(0x8ff1, 0x40);
    OV5642_write_cmos_sensor(0x8ff2, 0x03);
    OV5642_write_cmos_sensor(0x8ff3, 0xd3);
    OV5642_write_cmos_sensor(0x8ff4, 0x80);
    OV5642_write_cmos_sensor(0x8ff5, 0x01);
    OV5642_write_cmos_sensor(0x8ff6, 0xc3);
    OV5642_write_cmos_sensor(0x8ff7, 0x92);
    OV5642_write_cmos_sensor(0x8ff8, 0x22);
    OV5642_write_cmos_sensor(0x8ff9, 0xd3);
    OV5642_write_cmos_sensor(0x8ffa, 0xe5);
    OV5642_write_cmos_sensor(0x8ffb, 0x5d);
    OV5642_write_cmos_sensor(0x8ffc, 0x95);
    OV5642_write_cmos_sensor(0x8ffd, 0x71);
    OV5642_write_cmos_sensor(0x8ffe, 0xe5);
    OV5642_write_cmos_sensor(0x8fff, 0x5c);
    OV5642_write_cmos_sensor(0x9000, 0x95);
    OV5642_write_cmos_sensor(0x9001, 0x70);
    OV5642_write_cmos_sensor(0x9002, 0x40);
    OV5642_write_cmos_sensor(0x9003, 0x03);
    OV5642_write_cmos_sensor(0x9004, 0xd3);
    OV5642_write_cmos_sensor(0x9005, 0x80);
    OV5642_write_cmos_sensor(0x9006, 0x01);
    OV5642_write_cmos_sensor(0x9007, 0xc3);
    OV5642_write_cmos_sensor(0x9008, 0x92);
    OV5642_write_cmos_sensor(0x9009, 0x23);
    OV5642_write_cmos_sensor(0x900a, 0xd3);
    OV5642_write_cmos_sensor(0x900b, 0xe5);
    OV5642_write_cmos_sensor(0x900c, 0x5f);
    OV5642_write_cmos_sensor(0x900d, 0x95);
    OV5642_write_cmos_sensor(0x900e, 0x73);
    OV5642_write_cmos_sensor(0x900f, 0xe5);
    OV5642_write_cmos_sensor(0x9010, 0x5e);
    OV5642_write_cmos_sensor(0x9011, 0x95);
    OV5642_write_cmos_sensor(0x9012, 0x72);
    OV5642_write_cmos_sensor(0x9013, 0x40);
    OV5642_write_cmos_sensor(0x9014, 0x03);
    OV5642_write_cmos_sensor(0x9015, 0xd3);
    OV5642_write_cmos_sensor(0x9016, 0x80);
    OV5642_write_cmos_sensor(0x9017, 0x01);
    OV5642_write_cmos_sensor(0x9018, 0xc3);
    OV5642_write_cmos_sensor(0x9019, 0x92);
    OV5642_write_cmos_sensor(0x901a, 0x24);
    OV5642_write_cmos_sensor(0x901b, 0xd3);
    OV5642_write_cmos_sensor(0x901c, 0xe5);
    OV5642_write_cmos_sensor(0x901d, 0x61);
    OV5642_write_cmos_sensor(0x901e, 0x95);
    OV5642_write_cmos_sensor(0x901f, 0x75);
    OV5642_write_cmos_sensor(0x9020, 0xe5);
    OV5642_write_cmos_sensor(0x9021, 0x60);
    OV5642_write_cmos_sensor(0x9022, 0x95);
    OV5642_write_cmos_sensor(0x9023, 0x74);
    OV5642_write_cmos_sensor(0x9024, 0x40);
    OV5642_write_cmos_sensor(0x9025, 0x03);
    OV5642_write_cmos_sensor(0x9026, 0xd3);
    OV5642_write_cmos_sensor(0x9027, 0x80);
    OV5642_write_cmos_sensor(0x9028, 0x01);
    OV5642_write_cmos_sensor(0x9029, 0xc3);
    OV5642_write_cmos_sensor(0x902a, 0x92);
    OV5642_write_cmos_sensor(0x902b, 0x25);
    OV5642_write_cmos_sensor(0x902c, 0xd3);
    OV5642_write_cmos_sensor(0x902d, 0xe5);
    OV5642_write_cmos_sensor(0x902e, 0x63);
    OV5642_write_cmos_sensor(0x902f, 0x95);
    OV5642_write_cmos_sensor(0x9030, 0x77);
    OV5642_write_cmos_sensor(0x9031, 0xe5);
    OV5642_write_cmos_sensor(0x9032, 0x62);
    OV5642_write_cmos_sensor(0x9033, 0x95);
    OV5642_write_cmos_sensor(0x9034, 0x76);
    OV5642_write_cmos_sensor(0x9035, 0x40);
    OV5642_write_cmos_sensor(0x9036, 0x03);
    OV5642_write_cmos_sensor(0x9037, 0xd3);
    OV5642_write_cmos_sensor(0x9038, 0x80);
    OV5642_write_cmos_sensor(0x9039, 0x01);
    OV5642_write_cmos_sensor(0x903a, 0xc3);
    OV5642_write_cmos_sensor(0x903b, 0x92);
    OV5642_write_cmos_sensor(0x903c, 0x26);
    OV5642_write_cmos_sensor(0x903d, 0x22);
    OV5642_write_cmos_sensor(0x903e, 0xe5);
    OV5642_write_cmos_sensor(0x903f, 0x7e);
    OV5642_write_cmos_sensor(0x9040, 0x64);
    OV5642_write_cmos_sensor(0x9041, 0x01);
    OV5642_write_cmos_sensor(0x9042, 0x70);
    OV5642_write_cmos_sensor(0x9043, 0x50);
    OV5642_write_cmos_sensor(0x9044, 0x12);
    OV5642_write_cmos_sensor(0x9045, 0x02);
    OV5642_write_cmos_sensor(0x9046, 0x39);
    OV5642_write_cmos_sensor(0x9047, 0xe5);
    OV5642_write_cmos_sensor(0x9048, 0x7d);
    OV5642_write_cmos_sensor(0x9049, 0x12);
    OV5642_write_cmos_sensor(0x904a, 0x01);
    OV5642_write_cmos_sensor(0x904b, 0x7b);
    OV5642_write_cmos_sensor(0x904c, 0xfe);
    OV5642_write_cmos_sensor(0x904d, 0xe4);
    OV5642_write_cmos_sensor(0x904e, 0x8f);
    OV5642_write_cmos_sensor(0x904f, 0x69);
    OV5642_write_cmos_sensor(0x9050, 0x8e);
    OV5642_write_cmos_sensor(0x9051, 0x68);
    OV5642_write_cmos_sensor(0x9052, 0xf5);
    OV5642_write_cmos_sensor(0x9053, 0x67);
    OV5642_write_cmos_sensor(0x9054, 0xf5);
    OV5642_write_cmos_sensor(0x9055, 0x66);
    OV5642_write_cmos_sensor(0x9056, 0x12);
    OV5642_write_cmos_sensor(0x9057, 0x01);
    OV5642_write_cmos_sensor(0x9058, 0xfc);
    OV5642_write_cmos_sensor(0x9059, 0x7b);
    OV5642_write_cmos_sensor(0x905a, 0xff);
    OV5642_write_cmos_sensor(0x905b, 0xfa);
    OV5642_write_cmos_sensor(0x905c, 0xf9);
    OV5642_write_cmos_sensor(0x905d, 0xf8);
    OV5642_write_cmos_sensor(0x905e, 0x12);
    OV5642_write_cmos_sensor(0x905f, 0x01);
    OV5642_write_cmos_sensor(0x9060, 0xf1);
    OV5642_write_cmos_sensor(0x9061, 0xc0);
    OV5642_write_cmos_sensor(0x9062, 0x04);
    OV5642_write_cmos_sensor(0x9063, 0xc0);
    OV5642_write_cmos_sensor(0x9064, 0x05);
    OV5642_write_cmos_sensor(0x9065, 0xc0);
    OV5642_write_cmos_sensor(0x9066, 0x06);
    OV5642_write_cmos_sensor(0x9067, 0xc0);
    OV5642_write_cmos_sensor(0x9068, 0x07);
    OV5642_write_cmos_sensor(0x9069, 0x12);
    OV5642_write_cmos_sensor(0x906a, 0x01);
    OV5642_write_cmos_sensor(0x906b, 0x73);
    OV5642_write_cmos_sensor(0x906c, 0xab);
    OV5642_write_cmos_sensor(0x906d, 0x07);
    OV5642_write_cmos_sensor(0x906e, 0xfa);
    OV5642_write_cmos_sensor(0x906f, 0xe4);
    OV5642_write_cmos_sensor(0x9070, 0xf9);
    OV5642_write_cmos_sensor(0x9071, 0xf8);
    OV5642_write_cmos_sensor(0x9072, 0xd0);
    OV5642_write_cmos_sensor(0x9073, 0x07);
    OV5642_write_cmos_sensor(0x9074, 0xd0);
    OV5642_write_cmos_sensor(0x9075, 0x06);
    OV5642_write_cmos_sensor(0x9076, 0xd0);
    OV5642_write_cmos_sensor(0x9077, 0x05);
    OV5642_write_cmos_sensor(0x9078, 0xd0);
    OV5642_write_cmos_sensor(0x9079, 0x04);
    OV5642_write_cmos_sensor(0x907a, 0x12);
    OV5642_write_cmos_sensor(0x907b, 0x02);
    OV5642_write_cmos_sensor(0x907c, 0x67);
    OV5642_write_cmos_sensor(0x907d, 0x85);
    OV5642_write_cmos_sensor(0x907e, 0x69);
    OV5642_write_cmos_sensor(0x907f, 0x66);
    OV5642_write_cmos_sensor(0x9080, 0x85);
    OV5642_write_cmos_sensor(0x9081, 0x7d);
    OV5642_write_cmos_sensor(0x9082, 0x67);
    OV5642_write_cmos_sensor(0x9083, 0x12);
    OV5642_write_cmos_sensor(0x9084, 0x02);
    OV5642_write_cmos_sensor(0x9085, 0x39);
    OV5642_write_cmos_sensor(0x9086, 0xe5);
    OV5642_write_cmos_sensor(0x9087, 0x7d);
    OV5642_write_cmos_sensor(0x9088, 0x12);
    OV5642_write_cmos_sensor(0x9089, 0x02);
    OV5642_write_cmos_sensor(0x908a, 0x95);
    OV5642_write_cmos_sensor(0x908b, 0xe4);
    OV5642_write_cmos_sensor(0x908c, 0x93);
    OV5642_write_cmos_sensor(0x908d, 0xf5);
    OV5642_write_cmos_sensor(0x908e, 0x68);
    OV5642_write_cmos_sensor(0x908f, 0x74);
    OV5642_write_cmos_sensor(0x9090, 0x01);
    OV5642_write_cmos_sensor(0x9091, 0x93);
    OV5642_write_cmos_sensor(0x9092, 0xf5);
    OV5642_write_cmos_sensor(0x9093, 0x69);
    OV5642_write_cmos_sensor(0x9094, 0x90);
    OV5642_write_cmos_sensor(0x9095, 0x50);
    OV5642_write_cmos_sensor(0x9096, 0x82);
    OV5642_write_cmos_sensor(0x9097, 0xe5);
    OV5642_write_cmos_sensor(0x9098, 0x66);
    OV5642_write_cmos_sensor(0x9099, 0xf0);
    OV5642_write_cmos_sensor(0x909a, 0xa3);
    OV5642_write_cmos_sensor(0x909b, 0xe5);
    OV5642_write_cmos_sensor(0x909c, 0x67);
    OV5642_write_cmos_sensor(0x909d, 0xf0);
    OV5642_write_cmos_sensor(0x909e, 0xa3);
    OV5642_write_cmos_sensor(0x909f, 0xe5);
    OV5642_write_cmos_sensor(0x90a0, 0x68);
    OV5642_write_cmos_sensor(0x90a1, 0xf0);
    OV5642_write_cmos_sensor(0x90a2, 0xa3);
    OV5642_write_cmos_sensor(0x90a3, 0xe5);
    OV5642_write_cmos_sensor(0x90a4, 0x69);
    OV5642_write_cmos_sensor(0x90a5, 0xf0);
    OV5642_write_cmos_sensor(0x90a6, 0x22);
    OV5642_write_cmos_sensor(0x90a7, 0x56);
    OV5642_write_cmos_sensor(0x90a8, 0x0c);
    OV5642_write_cmos_sensor(0x90a9, 0x04);
    OV5642_write_cmos_sensor(0x90aa, 0x00);
    OV5642_write_cmos_sensor(0x90ab, 0x00);
    OV5642_write_cmos_sensor(0x90ac, 0x00);
    OV5642_write_cmos_sensor(0x90ad, 0xc8);
    OV5642_write_cmos_sensor(0x90ae, 0x01);
    OV5642_write_cmos_sensor(0x90af, 0x2c);
    OV5642_write_cmos_sensor(0x90b0, 0x01);
    OV5642_write_cmos_sensor(0x90b1, 0x5e);
    OV5642_write_cmos_sensor(0x90b2, 0x01);
    OV5642_write_cmos_sensor(0x90b3, 0x8b);
    OV5642_write_cmos_sensor(0x90b4, 0x01);
    OV5642_write_cmos_sensor(0x90b5, 0xb8);
    OV5642_write_cmos_sensor(0x90b6, 0x01);
    OV5642_write_cmos_sensor(0x90b7, 0xe5);
    OV5642_write_cmos_sensor(0x90b8, 0x02);
    OV5642_write_cmos_sensor(0x90b9, 0x12);
    OV5642_write_cmos_sensor(0x90ba, 0x02);
    OV5642_write_cmos_sensor(0x90bb, 0x3f);
    OV5642_write_cmos_sensor(0x90bc, 0x02);
    OV5642_write_cmos_sensor(0x90bd, 0x6c);
    OV5642_write_cmos_sensor(0x90be, 0x02);
    OV5642_write_cmos_sensor(0x90bf, 0x99);
    OV5642_write_cmos_sensor(0x90c0, 0x02);
    OV5642_write_cmos_sensor(0x90c1, 0xc6);
    OV5642_write_cmos_sensor(0x90c2, 0x02);
    OV5642_write_cmos_sensor(0x90c3, 0xf3);
    OV5642_write_cmos_sensor(0x90c4, 0x07);
    OV5642_write_cmos_sensor(0x90c5, 0x20);
    OV5642_write_cmos_sensor(0x90c6, 0x12);
    OV5642_write_cmos_sensor(0x90c7, 0x28);
    OV5642_write_cmos_sensor(0x90c8, 0x1e);
    OV5642_write_cmos_sensor(0x90c9, 0x18);
    OV5642_write_cmos_sensor(0x90ca, 0x18);
    OV5642_write_cmos_sensor(0x90cb, 0x28);
    OV5642_write_cmos_sensor(0x90cc, 0x1e);
    OV5642_write_cmos_sensor(0x90cd, 0x18);
    OV5642_write_cmos_sensor(0x90ce, 0x12);
    OV5642_write_cmos_sensor(0x90cf, 0x28);
    OV5642_write_cmos_sensor(0x90d0, 0x1e);
    OV5642_write_cmos_sensor(0x90d1, 0x18);
    OV5642_write_cmos_sensor(0x90d2, 0x12);
    OV5642_write_cmos_sensor(0x90d3, 0x28);
    OV5642_write_cmos_sensor(0x90d4, 0x18);
    OV5642_write_cmos_sensor(0x90d5, 0x18);
    OV5642_write_cmos_sensor(0x90d6, 0x12);
    OV5642_write_cmos_sensor(0x90d7, 0x20);
    OV5642_write_cmos_sensor(0x90d8, 0x18);
    OV5642_write_cmos_sensor(0x90d9, 0x28);
    OV5642_write_cmos_sensor(0x90da, 0x1c);
    OV5642_write_cmos_sensor(0x90db, 0x30);
    OV5642_write_cmos_sensor(0x90dc, 0x24);
    OV5642_write_cmos_sensor(0x90dd, 0x10);
    OV5642_write_cmos_sensor(0x90de, 0x1c);
    OV5642_write_cmos_sensor(0x90df, 0x18);
    OV5642_write_cmos_sensor(0x90e0, 0x24);
    OV5642_write_cmos_sensor(0x90e1, 0x1c);
    OV5642_write_cmos_sensor(0x90e2, 0x14);
    OV5642_write_cmos_sensor(0x90e3, 0x24);
    OV5642_write_cmos_sensor(0x90e4, 0x1c);
    OV5642_write_cmos_sensor(0x90e5, 0x28);
    OV5642_write_cmos_sensor(0x90e6, 0x0c);
    OV5642_write_cmos_sensor(0x90e7, 0x30);
    OV5642_write_cmos_sensor(0x90e8, 0x14);
    OV5642_write_cmos_sensor(0x90e9, 0x10);
    OV5642_write_cmos_sensor(0x90ea, 0x0c);
    OV5642_write_cmos_sensor(0x90eb, 0x18);
    OV5642_write_cmos_sensor(0x90ec, 0x14);
    OV5642_write_cmos_sensor(0x90ed, 0x1c);
    OV5642_write_cmos_sensor(0x90ee, 0x20);
    OV5642_write_cmos_sensor(0x90ef, 0x24);
    OV5642_write_cmos_sensor(0x90f0, 0x28);
    OV5642_write_cmos_sensor(0x90f1, 0x0c);
    OV5642_write_cmos_sensor(0x90f2, 0x14);
    OV5642_write_cmos_sensor(0x90f3, 0x14);
    OV5642_write_cmos_sensor(0x90f4, 0x1c);
    OV5642_write_cmos_sensor(0x90f5, 0x1c);
    OV5642_write_cmos_sensor(0x90f6, 0x14);
    OV5642_write_cmos_sensor(0x90f7, 0x24);
    OV5642_write_cmos_sensor(0x90f8, 0x1c);
    OV5642_write_cmos_sensor(0x90f9, 0x2c);
    OV5642_write_cmos_sensor(0x90fa, 0x14);
    OV5642_write_cmos_sensor(0x90fb, 0x34);
    OV5642_write_cmos_sensor(0x90fc, 0x1c);
    OV5642_write_cmos_sensor(0x90fd, 0x1c);
    OV5642_write_cmos_sensor(0x90fe, 0x08);
    OV5642_write_cmos_sensor(0x90ff, 0x24);
    OV5642_write_cmos_sensor(0x9100, 0x10);
    OV5642_write_cmos_sensor(0x9101, 0x19);
    OV5642_write_cmos_sensor(0x9102, 0x19);
    OV5642_write_cmos_sensor(0x9103, 0x1c);
    OV5642_write_cmos_sensor(0x9104, 0x19);
    OV5642_write_cmos_sensor(0x9105, 0x19);
    OV5642_write_cmos_sensor(0x9106, 0x10);
    OV5642_write_cmos_sensor(0x9107, 0x10);
    OV5642_write_cmos_sensor(0x9108, 0x10);
    OV5642_write_cmos_sensor(0x9109, 0x10);
    OV5642_write_cmos_sensor(0x910a, 0x10);
    OV5642_write_cmos_sensor(0x910b, 0x00);
    OV5642_write_cmos_sensor(0x910c, 0x00);
    OV5642_write_cmos_sensor(0x910d, 0x00);
    OV5642_write_cmos_sensor(0x910e, 0x00);
    OV5642_write_cmos_sensor(0x910f, 0x00);
    OV5642_write_cmos_sensor(0x9110, 0xe5);
    OV5642_write_cmos_sensor(0x9111, 0x0a);
    OV5642_write_cmos_sensor(0x9112, 0x70);
    OV5642_write_cmos_sensor(0x9113, 0x04);
    OV5642_write_cmos_sensor(0x9114, 0x7a);
    OV5642_write_cmos_sensor(0x9115, 0x10);
    OV5642_write_cmos_sensor(0x9116, 0x7b);
    OV5642_write_cmos_sensor(0x9117, 0xc5);
    OV5642_write_cmos_sensor(0x9118, 0xe5);
    OV5642_write_cmos_sensor(0x9119, 0x0a);
    OV5642_write_cmos_sensor(0x911a, 0xb4);
    OV5642_write_cmos_sensor(0x911b, 0x01);
    OV5642_write_cmos_sensor(0x911c, 0x04);
    OV5642_write_cmos_sensor(0x911d, 0x7a);
    OV5642_write_cmos_sensor(0x911e, 0x10);
    OV5642_write_cmos_sensor(0x911f, 0x7b);
    OV5642_write_cmos_sensor(0x9120, 0xd9);
    OV5642_write_cmos_sensor(0x9121, 0xe5);
    OV5642_write_cmos_sensor(0x9122, 0x0a);
    OV5642_write_cmos_sensor(0x9123, 0xb4);
    OV5642_write_cmos_sensor(0x9124, 0x02);
    OV5642_write_cmos_sensor(0x9125, 0x04);
    OV5642_write_cmos_sensor(0x9126, 0x7a);
    OV5642_write_cmos_sensor(0x9127, 0x10);
    OV5642_write_cmos_sensor(0x9128, 0x7b);
    OV5642_write_cmos_sensor(0x9129, 0xed);
    OV5642_write_cmos_sensor(0x912a, 0x8b);
    OV5642_write_cmos_sensor(0x912b, 0x82);
    OV5642_write_cmos_sensor(0x912c, 0x8a);
    OV5642_write_cmos_sensor(0x912d, 0x83);
    OV5642_write_cmos_sensor(0x912e, 0x12);
    OV5642_write_cmos_sensor(0x912f, 0x09);
    OV5642_write_cmos_sensor(0x9130, 0xd1);
    OV5642_write_cmos_sensor(0x9131, 0x8f);
    OV5642_write_cmos_sensor(0x9132, 0x37);
    OV5642_write_cmos_sensor(0x9133, 0x8e);
    OV5642_write_cmos_sensor(0x9134, 0x36);
    OV5642_write_cmos_sensor(0x9135, 0x8d);
    OV5642_write_cmos_sensor(0x9136, 0x35);
    OV5642_write_cmos_sensor(0x9137, 0x8c);
    OV5642_write_cmos_sensor(0x9138, 0x34);
    OV5642_write_cmos_sensor(0x9139, 0xe5);
    OV5642_write_cmos_sensor(0x913a, 0x82);
    OV5642_write_cmos_sensor(0x913b, 0x24);
    OV5642_write_cmos_sensor(0x913c, 0x04);
    OV5642_write_cmos_sensor(0x913d, 0xf5);
    OV5642_write_cmos_sensor(0x913e, 0x82);
    OV5642_write_cmos_sensor(0x913f, 0xe4);
    OV5642_write_cmos_sensor(0x9140, 0x35);
    OV5642_write_cmos_sensor(0x9141, 0x83);
    OV5642_write_cmos_sensor(0x9142, 0xf5);
    OV5642_write_cmos_sensor(0x9143, 0x83);
    OV5642_write_cmos_sensor(0x9144, 0x12);
    OV5642_write_cmos_sensor(0x9145, 0x09);
    OV5642_write_cmos_sensor(0x9146, 0xd1);
    OV5642_write_cmos_sensor(0x9147, 0x8f);
    OV5642_write_cmos_sensor(0x9148, 0x3b);
    OV5642_write_cmos_sensor(0x9149, 0x8e);
    OV5642_write_cmos_sensor(0x914a, 0x3a);
    OV5642_write_cmos_sensor(0x914b, 0x8d);
    OV5642_write_cmos_sensor(0x914c, 0x39);
    OV5642_write_cmos_sensor(0x914d, 0x8c);
    OV5642_write_cmos_sensor(0x914e, 0x38);
    OV5642_write_cmos_sensor(0x914f, 0xeb);
    OV5642_write_cmos_sensor(0x9150, 0x24);
    OV5642_write_cmos_sensor(0x9151, 0x08);
    OV5642_write_cmos_sensor(0x9152, 0x12);
    OV5642_write_cmos_sensor(0x9153, 0x02);
    OV5642_write_cmos_sensor(0x9154, 0x40);
    OV5642_write_cmos_sensor(0x9155, 0x12);
    OV5642_write_cmos_sensor(0x9156, 0x01);
    OV5642_write_cmos_sensor(0x9157, 0xc0);
    OV5642_write_cmos_sensor(0x9158, 0xeb);
    OV5642_write_cmos_sensor(0x9159, 0x24);
    OV5642_write_cmos_sensor(0x915a, 0x0c);
    OV5642_write_cmos_sensor(0x915b, 0x12);
    OV5642_write_cmos_sensor(0x915c, 0x02);
    OV5642_write_cmos_sensor(0x915d, 0x40);
    OV5642_write_cmos_sensor(0x915e, 0x8f);
    OV5642_write_cmos_sensor(0x915f, 0x43);
    OV5642_write_cmos_sensor(0x9160, 0x8e);
    OV5642_write_cmos_sensor(0x9161, 0x42);
    OV5642_write_cmos_sensor(0x9162, 0x8d);
    OV5642_write_cmos_sensor(0x9163, 0x41);
    OV5642_write_cmos_sensor(0x9164, 0x8c);
    OV5642_write_cmos_sensor(0x9165, 0x40);
    OV5642_write_cmos_sensor(0x9166, 0xeb);
    OV5642_write_cmos_sensor(0x9167, 0x24);
    OV5642_write_cmos_sensor(0x9168, 0x10);
    OV5642_write_cmos_sensor(0x9169, 0x12);
    OV5642_write_cmos_sensor(0x916a, 0x02);
    OV5642_write_cmos_sensor(0x916b, 0x40);
    OV5642_write_cmos_sensor(0x916c, 0x8f);
    OV5642_write_cmos_sensor(0x916d, 0x47);
    OV5642_write_cmos_sensor(0x916e, 0x8e);
    OV5642_write_cmos_sensor(0x916f, 0x46);
    OV5642_write_cmos_sensor(0x9170, 0x8d);
    OV5642_write_cmos_sensor(0x9171, 0x45);
    OV5642_write_cmos_sensor(0x9172, 0x8c);
    OV5642_write_cmos_sensor(0x9173, 0x44);
    OV5642_write_cmos_sensor(0x9174, 0x22);
    OV5642_write_cmos_sensor(0x9175, 0x30);
    OV5642_write_cmos_sensor(0x9176, 0x3c);
    OV5642_write_cmos_sensor(0x9177, 0x07);
    OV5642_write_cmos_sensor(0x9178, 0x30);
    OV5642_write_cmos_sensor(0x9179, 0x20);
    OV5642_write_cmos_sensor(0x917a, 0x04);
    OV5642_write_cmos_sensor(0x917b, 0xaf);
    OV5642_write_cmos_sensor(0x917c, 0x4a);
    OV5642_write_cmos_sensor(0x917d, 0x80);
    OV5642_write_cmos_sensor(0x917e, 0x02);
    OV5642_write_cmos_sensor(0x917f, 0xaf);
    OV5642_write_cmos_sensor(0x9180, 0x7d);
    OV5642_write_cmos_sensor(0x9181, 0x8f);
    OV5642_write_cmos_sensor(0x9182, 0x4a);
    OV5642_write_cmos_sensor(0x9183, 0x30);
    OV5642_write_cmos_sensor(0x9184, 0x3c);
    OV5642_write_cmos_sensor(0x9185, 0x07);
    OV5642_write_cmos_sensor(0x9186, 0x30);
    OV5642_write_cmos_sensor(0x9187, 0x21);
    OV5642_write_cmos_sensor(0x9188, 0x04);
    OV5642_write_cmos_sensor(0x9189, 0xaf);
    OV5642_write_cmos_sensor(0x918a, 0x4b);
    OV5642_write_cmos_sensor(0x918b, 0x80);
    OV5642_write_cmos_sensor(0x918c, 0x02);
    OV5642_write_cmos_sensor(0x918d, 0xaf);
    OV5642_write_cmos_sensor(0x918e, 0x7d);
    OV5642_write_cmos_sensor(0x918f, 0x8f);
    OV5642_write_cmos_sensor(0x9190, 0x4b);
    OV5642_write_cmos_sensor(0x9191, 0x30);
    OV5642_write_cmos_sensor(0x9192, 0x3c);
    OV5642_write_cmos_sensor(0x9193, 0x07);
    OV5642_write_cmos_sensor(0x9194, 0x30);
    OV5642_write_cmos_sensor(0x9195, 0x22);
    OV5642_write_cmos_sensor(0x9196, 0x04);
    OV5642_write_cmos_sensor(0x9197, 0xaf);
    OV5642_write_cmos_sensor(0x9198, 0x4c);
    OV5642_write_cmos_sensor(0x9199, 0x80);
    OV5642_write_cmos_sensor(0x919a, 0x02);
    OV5642_write_cmos_sensor(0x919b, 0xaf);
    OV5642_write_cmos_sensor(0x919c, 0x7d);
    OV5642_write_cmos_sensor(0x919d, 0x8f);
    OV5642_write_cmos_sensor(0x919e, 0x4c);
    OV5642_write_cmos_sensor(0x919f, 0x30);
    OV5642_write_cmos_sensor(0x91a0, 0x3c);
    OV5642_write_cmos_sensor(0x91a1, 0x07);
    OV5642_write_cmos_sensor(0x91a2, 0x30);
    OV5642_write_cmos_sensor(0x91a3, 0x23);
    OV5642_write_cmos_sensor(0x91a4, 0x04);
    OV5642_write_cmos_sensor(0x91a5, 0xaf);
    OV5642_write_cmos_sensor(0x91a6, 0x4d);
    OV5642_write_cmos_sensor(0x91a7, 0x80);
    OV5642_write_cmos_sensor(0x91a8, 0x02);
    OV5642_write_cmos_sensor(0x91a9, 0xaf);
    OV5642_write_cmos_sensor(0x91aa, 0x7d);
    OV5642_write_cmos_sensor(0x91ab, 0x8f);
    OV5642_write_cmos_sensor(0x91ac, 0x4d);
    OV5642_write_cmos_sensor(0x91ad, 0x30);
    OV5642_write_cmos_sensor(0x91ae, 0x3c);
    OV5642_write_cmos_sensor(0x91af, 0x07);
    OV5642_write_cmos_sensor(0x91b0, 0x30);
    OV5642_write_cmos_sensor(0x91b1, 0x24);
    OV5642_write_cmos_sensor(0x91b2, 0x04);
    OV5642_write_cmos_sensor(0x91b3, 0xaf);
    OV5642_write_cmos_sensor(0x91b4, 0x4e);
    OV5642_write_cmos_sensor(0x91b5, 0x80);
    OV5642_write_cmos_sensor(0x91b6, 0x02);
    OV5642_write_cmos_sensor(0x91b7, 0xaf);
    OV5642_write_cmos_sensor(0x91b8, 0x7d);
    OV5642_write_cmos_sensor(0x91b9, 0x8f);
    OV5642_write_cmos_sensor(0x91ba, 0x4e);
    OV5642_write_cmos_sensor(0x91bb, 0x30);
    OV5642_write_cmos_sensor(0x91bc, 0x3c);
    OV5642_write_cmos_sensor(0x91bd, 0x07);
    OV5642_write_cmos_sensor(0x91be, 0x30);
    OV5642_write_cmos_sensor(0x91bf, 0x25);
    OV5642_write_cmos_sensor(0x91c0, 0x04);
    OV5642_write_cmos_sensor(0x91c1, 0xaf);
    OV5642_write_cmos_sensor(0x91c2, 0x4f);
    OV5642_write_cmos_sensor(0x91c3, 0x80);
    OV5642_write_cmos_sensor(0x91c4, 0x02);
    OV5642_write_cmos_sensor(0x91c5, 0xaf);
    OV5642_write_cmos_sensor(0x91c6, 0x7d);
    OV5642_write_cmos_sensor(0x91c7, 0x8f);
    OV5642_write_cmos_sensor(0x91c8, 0x4f);
    OV5642_write_cmos_sensor(0x91c9, 0x30);
    OV5642_write_cmos_sensor(0x91ca, 0x3c);
    OV5642_write_cmos_sensor(0x91cb, 0x07);
    OV5642_write_cmos_sensor(0x91cc, 0x30);
    OV5642_write_cmos_sensor(0x91cd, 0x26);
    OV5642_write_cmos_sensor(0x91ce, 0x04);
    OV5642_write_cmos_sensor(0x91cf, 0xaf);
    OV5642_write_cmos_sensor(0x91d0, 0x50);
    OV5642_write_cmos_sensor(0x91d1, 0x80);
    OV5642_write_cmos_sensor(0x91d2, 0x02);
    OV5642_write_cmos_sensor(0x91d3, 0xaf);
    OV5642_write_cmos_sensor(0x91d4, 0x7d);
    OV5642_write_cmos_sensor(0x91d5, 0x8f);
    OV5642_write_cmos_sensor(0x91d6, 0x50);
    OV5642_write_cmos_sensor(0x91d7, 0x22);
    OV5642_write_cmos_sensor(0x91d8, 0xe5);
    OV5642_write_cmos_sensor(0x91d9, 0x0c);
    OV5642_write_cmos_sensor(0x91da, 0xd3);
    OV5642_write_cmos_sensor(0x91db, 0x95);
    OV5642_write_cmos_sensor(0x91dc, 0x7c);
    OV5642_write_cmos_sensor(0x91dd, 0x40);
    OV5642_write_cmos_sensor(0x91de, 0x01);
    OV5642_write_cmos_sensor(0x91df, 0x22);
    OV5642_write_cmos_sensor(0x91e0, 0x12);
    OV5642_write_cmos_sensor(0x91e1, 0x02);
    OV5642_write_cmos_sensor(0x91e2, 0x39);
    OV5642_write_cmos_sensor(0x91e3, 0xe5);
    OV5642_write_cmos_sensor(0x91e4, 0x0c);
    OV5642_write_cmos_sensor(0x91e5, 0x12);
    OV5642_write_cmos_sensor(0x91e6, 0x02);
    OV5642_write_cmos_sensor(0x91e7, 0x95);
    OV5642_write_cmos_sensor(0x91e8, 0xe4);
    OV5642_write_cmos_sensor(0x91e9, 0x93);
    OV5642_write_cmos_sensor(0x91ea, 0xfe);
    OV5642_write_cmos_sensor(0x91eb, 0x74);
    OV5642_write_cmos_sensor(0x91ec, 0x01);
    OV5642_write_cmos_sensor(0x91ed, 0x93);
    OV5642_write_cmos_sensor(0x91ee, 0xff);
    OV5642_write_cmos_sensor(0x91ef, 0x4e);
    OV5642_write_cmos_sensor(0x91f0, 0x60);
    OV5642_write_cmos_sensor(0x91f1, 0x21);
    OV5642_write_cmos_sensor(0x91f2, 0x8e);
    OV5642_write_cmos_sensor(0x91f3, 0x64);
    OV5642_write_cmos_sensor(0x91f4, 0x8f);
    OV5642_write_cmos_sensor(0x91f5, 0x65);
    OV5642_write_cmos_sensor(0x91f6, 0xef);
    OV5642_write_cmos_sensor(0x91f7, 0xc4);
    OV5642_write_cmos_sensor(0x91f8, 0xf8);
    OV5642_write_cmos_sensor(0x91f9, 0x54);
    OV5642_write_cmos_sensor(0x91fa, 0x0f);
    OV5642_write_cmos_sensor(0x91fb, 0xc8);
    OV5642_write_cmos_sensor(0x91fc, 0x68);
    OV5642_write_cmos_sensor(0x91fd, 0xff);
    OV5642_write_cmos_sensor(0x91fe, 0xee);
    OV5642_write_cmos_sensor(0x91ff, 0xc4);
    OV5642_write_cmos_sensor(0x9200, 0x54);
    OV5642_write_cmos_sensor(0x9201, 0xf0);
    OV5642_write_cmos_sensor(0x9202, 0x48);
    OV5642_write_cmos_sensor(0x9203, 0xfe);
    OV5642_write_cmos_sensor(0x9204, 0x43);
    OV5642_write_cmos_sensor(0x9205, 0x07);
    OV5642_write_cmos_sensor(0x9206, 0x0d);
    OV5642_write_cmos_sensor(0x9207, 0x8e);
    OV5642_write_cmos_sensor(0x9208, 0x0d);
    OV5642_write_cmos_sensor(0x9209, 0x8f);
    OV5642_write_cmos_sensor(0x920a, 0x0e);
    OV5642_write_cmos_sensor(0x920b, 0x12);
    OV5642_write_cmos_sensor(0x920c, 0x0a);
    OV5642_write_cmos_sensor(0x920d, 0x13);
    OV5642_write_cmos_sensor(0x920e, 0x30);
    OV5642_write_cmos_sensor(0x920f, 0x33);
    OV5642_write_cmos_sensor(0x9210, 0x22);
    OV5642_write_cmos_sensor(0x9211, 0xc3);
    OV5642_write_cmos_sensor(0x9212, 0x22);
    OV5642_write_cmos_sensor(0x9213, 0x75);
    OV5642_write_cmos_sensor(0x9214, 0x0d);
    OV5642_write_cmos_sensor(0x9215, 0x00);
    OV5642_write_cmos_sensor(0x9216, 0x75);
    OV5642_write_cmos_sensor(0x9217, 0x0e);
    OV5642_write_cmos_sensor(0x9218, 0x0d);
    OV5642_write_cmos_sensor(0x9219, 0x12);
    OV5642_write_cmos_sensor(0x921a, 0x0a);
    OV5642_write_cmos_sensor(0x921b, 0x13);
    OV5642_write_cmos_sensor(0x921c, 0x30);
    OV5642_write_cmos_sensor(0x921d, 0x33);
    OV5642_write_cmos_sensor(0x921e, 0x02);
    OV5642_write_cmos_sensor(0x921f, 0xc3);
    OV5642_write_cmos_sensor(0x9220, 0x22);
    OV5642_write_cmos_sensor(0x9221, 0x75);
    OV5642_write_cmos_sensor(0x9222, 0x0d);
    OV5642_write_cmos_sensor(0x9223, 0x00);
    OV5642_write_cmos_sensor(0x9224, 0x75);
    OV5642_write_cmos_sensor(0x9225, 0x0e);
    OV5642_write_cmos_sensor(0x9226, 0x64);
    OV5642_write_cmos_sensor(0x9227, 0x12);
    OV5642_write_cmos_sensor(0x9228, 0x14);
    OV5642_write_cmos_sensor(0x9229, 0xcc);
    OV5642_write_cmos_sensor(0x922a, 0x75);
    OV5642_write_cmos_sensor(0x922b, 0x0d);
    OV5642_write_cmos_sensor(0x922c, 0x80);
    OV5642_write_cmos_sensor(0x922d, 0x75);
    OV5642_write_cmos_sensor(0x922e, 0x0e);
    OV5642_write_cmos_sensor(0x922f, 0x00);
    OV5642_write_cmos_sensor(0x9230, 0x12);
    OV5642_write_cmos_sensor(0x9231, 0x0a);
    OV5642_write_cmos_sensor(0x9232, 0x13);
    OV5642_write_cmos_sensor(0x9233, 0x85);
    OV5642_write_cmos_sensor(0x9234, 0x0c);
    OV5642_write_cmos_sensor(0x9235, 0x7d);
    OV5642_write_cmos_sensor(0x9236, 0xd3);
    OV5642_write_cmos_sensor(0x9237, 0x22);
    OV5642_write_cmos_sensor(0x9238, 0xc2);
    OV5642_write_cmos_sensor(0x9239, 0x34);
    OV5642_write_cmos_sensor(0x923a, 0x20);
    OV5642_write_cmos_sensor(0x923b, 0x05);
    OV5642_write_cmos_sensor(0x923c, 0x05);
    OV5642_write_cmos_sensor(0x923d, 0x75);
    OV5642_write_cmos_sensor(0x923e, 0x0a);
    OV5642_write_cmos_sensor(0x923f, 0xee);
    OV5642_write_cmos_sensor(0x9240, 0x80);
    OV5642_write_cmos_sensor(0x9241, 0x36);
    OV5642_write_cmos_sensor(0x9242, 0x20);
    OV5642_write_cmos_sensor(0x9243, 0x07);
    OV5642_write_cmos_sensor(0x9244, 0x08);
    OV5642_write_cmos_sensor(0x9245, 0x20);
    OV5642_write_cmos_sensor(0x9246, 0x06);
    OV5642_write_cmos_sensor(0x9247, 0x05);
    OV5642_write_cmos_sensor(0x9248, 0xe4);
    OV5642_write_cmos_sensor(0x9249, 0xf5);
    OV5642_write_cmos_sensor(0x924a, 0x0a);
    OV5642_write_cmos_sensor(0x924b, 0x80);
    OV5642_write_cmos_sensor(0x924c, 0x2b);
    OV5642_write_cmos_sensor(0x924d, 0x20);
    OV5642_write_cmos_sensor(0x924e, 0x07);
    OV5642_write_cmos_sensor(0x924f, 0x08);
    OV5642_write_cmos_sensor(0x9250, 0x30);
    OV5642_write_cmos_sensor(0x9251, 0x06);
    OV5642_write_cmos_sensor(0x9252, 0x05);
    OV5642_write_cmos_sensor(0x9253, 0x75);
    OV5642_write_cmos_sensor(0x9254, 0x0a);
    OV5642_write_cmos_sensor(0x9255, 0x20);
    OV5642_write_cmos_sensor(0x9256, 0x80);
    OV5642_write_cmos_sensor(0x9257, 0x20);
    OV5642_write_cmos_sensor(0x9258, 0x30);
    OV5642_write_cmos_sensor(0x9259, 0x00);
    OV5642_write_cmos_sensor(0x925a, 0x05);
    OV5642_write_cmos_sensor(0x925b, 0x75);
    OV5642_write_cmos_sensor(0x925c, 0x0a);
    OV5642_write_cmos_sensor(0x925d, 0x01);
    OV5642_write_cmos_sensor(0x925e, 0x80);
    OV5642_write_cmos_sensor(0x925f, 0x18);
    OV5642_write_cmos_sensor(0x9260, 0xe5);
    OV5642_write_cmos_sensor(0x9261, 0x20);
    OV5642_write_cmos_sensor(0x9262, 0x54);
    OV5642_write_cmos_sensor(0x9263, 0x07);
    OV5642_write_cmos_sensor(0x9264, 0xff);
    OV5642_write_cmos_sensor(0x9265, 0xbf);
    OV5642_write_cmos_sensor(0x9266, 0x06);
    OV5642_write_cmos_sensor(0x9267, 0x0d);
    OV5642_write_cmos_sensor(0x9268, 0x30);
    OV5642_write_cmos_sensor(0x9269, 0x31);
    OV5642_write_cmos_sensor(0x926a, 0x04);
    OV5642_write_cmos_sensor(0x926b, 0x7f);
    OV5642_write_cmos_sensor(0x926c, 0x12);
    OV5642_write_cmos_sensor(0x926d, 0x80);
    OV5642_write_cmos_sensor(0x926e, 0x02);
    OV5642_write_cmos_sensor(0x926f, 0x7f);
    OV5642_write_cmos_sensor(0x9270, 0x02);
    OV5642_write_cmos_sensor(0x9271, 0x8f);
    OV5642_write_cmos_sensor(0x9272, 0x0a);
    OV5642_write_cmos_sensor(0x9273, 0x80);
    OV5642_write_cmos_sensor(0x9274, 0x03);
    OV5642_write_cmos_sensor(0x9275, 0x75);
    OV5642_write_cmos_sensor(0x9276, 0x0a);
    OV5642_write_cmos_sensor(0x9277, 0xfe);
    OV5642_write_cmos_sensor(0x9278, 0x90);
    OV5642_write_cmos_sensor(0x9279, 0x30);
    OV5642_write_cmos_sensor(0x927a, 0x27);
    OV5642_write_cmos_sensor(0x927b, 0xe5);
    OV5642_write_cmos_sensor(0x927c, 0x0a);
    OV5642_write_cmos_sensor(0x927d, 0xf0);
    OV5642_write_cmos_sensor(0x927e, 0xe5);
    OV5642_write_cmos_sensor(0x927f, 0x23);
    OV5642_write_cmos_sensor(0x9280, 0x54);
    OV5642_write_cmos_sensor(0x9281, 0xf8);
    OV5642_write_cmos_sensor(0x9282, 0xf5);
    OV5642_write_cmos_sensor(0x9283, 0x0a);
    OV5642_write_cmos_sensor(0x9284, 0xe5);
    OV5642_write_cmos_sensor(0x9285, 0x79);
    OV5642_write_cmos_sensor(0x9286, 0x25);
    OV5642_write_cmos_sensor(0x9287, 0x0a);
    OV5642_write_cmos_sensor(0x9288, 0xf5);
    OV5642_write_cmos_sensor(0x9289, 0x0a);
    OV5642_write_cmos_sensor(0x928a, 0x90);
    OV5642_write_cmos_sensor(0x928b, 0x30);
    OV5642_write_cmos_sensor(0x928c, 0x26);
    OV5642_write_cmos_sensor(0x928d, 0xe5);
    OV5642_write_cmos_sensor(0x928e, 0x0a);
    OV5642_write_cmos_sensor(0x928f, 0xf0);
    OV5642_write_cmos_sensor(0x9290, 0x22);
    OV5642_write_cmos_sensor(0x9291, 0xe5);
    OV5642_write_cmos_sensor(0x9292, 0x0a);
    OV5642_write_cmos_sensor(0x9293, 0x70);
    OV5642_write_cmos_sensor(0x9294, 0x04);
    OV5642_write_cmos_sensor(0x9295, 0x7e);
    OV5642_write_cmos_sensor(0x9296, 0x11);
    OV5642_write_cmos_sensor(0x9297, 0x7f);
    OV5642_write_cmos_sensor(0x9298, 0x01);
    OV5642_write_cmos_sensor(0x9299, 0xe5);
    OV5642_write_cmos_sensor(0x929a, 0x0a);
    OV5642_write_cmos_sensor(0x929b, 0xb4);
    OV5642_write_cmos_sensor(0x929c, 0x01);
    OV5642_write_cmos_sensor(0x929d, 0x04);
    OV5642_write_cmos_sensor(0x929e, 0x7e);
    OV5642_write_cmos_sensor(0x929f, 0x11);
    OV5642_write_cmos_sensor(0x92a0, 0x7f);
    OV5642_write_cmos_sensor(0x92a1, 0x06);
    OV5642_write_cmos_sensor(0x92a2, 0xe5);
    OV5642_write_cmos_sensor(0x92a3, 0x0a);
    OV5642_write_cmos_sensor(0x92a4, 0xb4);
    OV5642_write_cmos_sensor(0x92a5, 0x02);
    OV5642_write_cmos_sensor(0x92a6, 0x04);
    OV5642_write_cmos_sensor(0x92a7, 0x7e);
    OV5642_write_cmos_sensor(0x92a8, 0x11);
    OV5642_write_cmos_sensor(0x92a9, 0x7f);
    OV5642_write_cmos_sensor(0x92aa, 0x0b);
    OV5642_write_cmos_sensor(0x92ab, 0x8f);
    OV5642_write_cmos_sensor(0x92ac, 0x82);
    OV5642_write_cmos_sensor(0x92ad, 0x8e);
    OV5642_write_cmos_sensor(0x92ae, 0x83);
    OV5642_write_cmos_sensor(0x92af, 0xe4);
    OV5642_write_cmos_sensor(0x92b0, 0x93);
    OV5642_write_cmos_sensor(0x92b1, 0xf5);
    OV5642_write_cmos_sensor(0x92b2, 0x2c);
    OV5642_write_cmos_sensor(0x92b3, 0x74);
    OV5642_write_cmos_sensor(0x92b4, 0x01);
    OV5642_write_cmos_sensor(0x92b5, 0x93);
    OV5642_write_cmos_sensor(0x92b6, 0xf5);
    OV5642_write_cmos_sensor(0x92b7, 0x2d);
    OV5642_write_cmos_sensor(0x92b8, 0x74);
    OV5642_write_cmos_sensor(0x92b9, 0x02);
    OV5642_write_cmos_sensor(0x92ba, 0x93);
    OV5642_write_cmos_sensor(0x92bb, 0xf5);
    OV5642_write_cmos_sensor(0x92bc, 0x2e);
    OV5642_write_cmos_sensor(0x92bd, 0x74);
    OV5642_write_cmos_sensor(0x92be, 0x03);
    OV5642_write_cmos_sensor(0x92bf, 0x93);
    OV5642_write_cmos_sensor(0x92c0, 0xf5);
    OV5642_write_cmos_sensor(0x92c1, 0x2f);
    OV5642_write_cmos_sensor(0x92c2, 0x74);
    OV5642_write_cmos_sensor(0x92c3, 0x04);
    OV5642_write_cmos_sensor(0x92c4, 0x93);
    OV5642_write_cmos_sensor(0x92c5, 0xf5);
    OV5642_write_cmos_sensor(0x92c6, 0x30);
    OV5642_write_cmos_sensor(0x92c7, 0xe5);
    OV5642_write_cmos_sensor(0x92c8, 0x0a);
    OV5642_write_cmos_sensor(0x92c9, 0xb4);
    OV5642_write_cmos_sensor(0x92ca, 0x01);
    OV5642_write_cmos_sensor(0x92cb, 0x07);
    OV5642_write_cmos_sensor(0x92cc, 0x74);
    OV5642_write_cmos_sensor(0x92cd, 0x2c);
    OV5642_write_cmos_sensor(0x92ce, 0x25);
    OV5642_write_cmos_sensor(0x92cf, 0x79);
    OV5642_write_cmos_sensor(0x92d0, 0xf8);
    OV5642_write_cmos_sensor(0x92d1, 0x76);
    OV5642_write_cmos_sensor(0x92d2, 0x40);
    OV5642_write_cmos_sensor(0x92d3, 0xe5);
    OV5642_write_cmos_sensor(0x92d4, 0x0a);
    OV5642_write_cmos_sensor(0x92d5, 0xb4);
    OV5642_write_cmos_sensor(0x92d6, 0x02);
    OV5642_write_cmos_sensor(0x92d7, 0x07);
    OV5642_write_cmos_sensor(0x92d8, 0x74);
    OV5642_write_cmos_sensor(0x92d9, 0x2c);
    OV5642_write_cmos_sensor(0x92da, 0x25);
    OV5642_write_cmos_sensor(0x92db, 0x79);
    OV5642_write_cmos_sensor(0x92dc, 0xf8);
    OV5642_write_cmos_sensor(0x92dd, 0x76);
    OV5642_write_cmos_sensor(0x92de, 0x80);
    OV5642_write_cmos_sensor(0x92df, 0x22);
    OV5642_write_cmos_sensor(0x92e0, 0xc0);
    OV5642_write_cmos_sensor(0x92e1, 0xe0);
    OV5642_write_cmos_sensor(0x92e2, 0xc0);
    OV5642_write_cmos_sensor(0x92e3, 0x83);
    OV5642_write_cmos_sensor(0x92e4, 0xc0);
    OV5642_write_cmos_sensor(0x92e5, 0x82);
    OV5642_write_cmos_sensor(0x92e6, 0xc0);
    OV5642_write_cmos_sensor(0x92e7, 0xd0);
    OV5642_write_cmos_sensor(0x92e8, 0x90);
    OV5642_write_cmos_sensor(0x92e9, 0x3f);
    OV5642_write_cmos_sensor(0x92ea, 0x0d);
    OV5642_write_cmos_sensor(0x92eb, 0xe0);
    OV5642_write_cmos_sensor(0x92ec, 0xf5);
    OV5642_write_cmos_sensor(0x92ed, 0x09);
    OV5642_write_cmos_sensor(0x92ee, 0xe5);
    OV5642_write_cmos_sensor(0x92ef, 0x09);
    OV5642_write_cmos_sensor(0x92f0, 0x30);
    OV5642_write_cmos_sensor(0x92f1, 0xe0);
    OV5642_write_cmos_sensor(0x92f2, 0x2e);
    OV5642_write_cmos_sensor(0x92f3, 0xe5);
    OV5642_write_cmos_sensor(0x92f4, 0x7a);
    OV5642_write_cmos_sensor(0x92f5, 0xb4);
    OV5642_write_cmos_sensor(0x92f6, 0x01);
    OV5642_write_cmos_sensor(0x92f7, 0x09);
    OV5642_write_cmos_sensor(0x92f8, 0x90);
    OV5642_write_cmos_sensor(0x92f9, 0x3a);
    OV5642_write_cmos_sensor(0x92fa, 0x00);
    OV5642_write_cmos_sensor(0x92fb, 0xe0);
    OV5642_write_cmos_sensor(0x92fc, 0xf5);
    OV5642_write_cmos_sensor(0x92fd, 0x78);
    OV5642_write_cmos_sensor(0x92fe, 0x44);
    OV5642_write_cmos_sensor(0x92ff, 0x01);
    OV5642_write_cmos_sensor(0x9300, 0xf0);
    OV5642_write_cmos_sensor(0x9301, 0xe5);
    OV5642_write_cmos_sensor(0x9302, 0x7a);
    OV5642_write_cmos_sensor(0x9303, 0xb4);
    OV5642_write_cmos_sensor(0x9304, 0x03);
    OV5642_write_cmos_sensor(0x9305, 0x09);
    OV5642_write_cmos_sensor(0x9306, 0x90);
    OV5642_write_cmos_sensor(0x9307, 0x3a);
    OV5642_write_cmos_sensor(0x9308, 0x00);
    OV5642_write_cmos_sensor(0x9309, 0xe0);
    OV5642_write_cmos_sensor(0x930a, 0xf5);
    OV5642_write_cmos_sensor(0x930b, 0x78);
    OV5642_write_cmos_sensor(0x930c, 0x54);
    OV5642_write_cmos_sensor(0x930d, 0xfe);
    OV5642_write_cmos_sensor(0x930e, 0xf0);
    OV5642_write_cmos_sensor(0x930f, 0xe5);
    OV5642_write_cmos_sensor(0x9310, 0x7a);
    OV5642_write_cmos_sensor(0x9311, 0xb4);
    OV5642_write_cmos_sensor(0x9312, 0x03);
    OV5642_write_cmos_sensor(0x9313, 0x05);
    OV5642_write_cmos_sensor(0x9314, 0x75);
    OV5642_write_cmos_sensor(0x9315, 0x7a);
    OV5642_write_cmos_sensor(0x9316, 0x00);
    OV5642_write_cmos_sensor(0x9317, 0x80);
    OV5642_write_cmos_sensor(0x9318, 0x02);
    OV5642_write_cmos_sensor(0x9319, 0x05);
    OV5642_write_cmos_sensor(0x931a, 0x7a);
    OV5642_write_cmos_sensor(0x931b, 0x90);
    OV5642_write_cmos_sensor(0x931c, 0x3f);
    OV5642_write_cmos_sensor(0x931d, 0x0d);
    OV5642_write_cmos_sensor(0x931e, 0x74);
    OV5642_write_cmos_sensor(0x931f, 0x01);
    OV5642_write_cmos_sensor(0x9320, 0xf0);
    OV5642_write_cmos_sensor(0x9321, 0xd0);
    OV5642_write_cmos_sensor(0x9322, 0xd0);
    OV5642_write_cmos_sensor(0x9323, 0xd0);
    OV5642_write_cmos_sensor(0x9324, 0x82);
    OV5642_write_cmos_sensor(0x9325, 0xd0);
    OV5642_write_cmos_sensor(0x9326, 0x83);
    OV5642_write_cmos_sensor(0x9327, 0xd0);
    OV5642_write_cmos_sensor(0x9328, 0xe0);
    OV5642_write_cmos_sensor(0x9329, 0x32);
    OV5642_write_cmos_sensor(0x932a, 0x90);
    OV5642_write_cmos_sensor(0x932b, 0x50);
    OV5642_write_cmos_sensor(0x932c, 0x27);
    OV5642_write_cmos_sensor(0x932d, 0xe0);
    OV5642_write_cmos_sensor(0x932e, 0x44);
    OV5642_write_cmos_sensor(0x932f, 0x01);
    OV5642_write_cmos_sensor(0x9330, 0xf0);
    OV5642_write_cmos_sensor(0x9331, 0x90);
    OV5642_write_cmos_sensor(0x9332, 0x50);
    OV5642_write_cmos_sensor(0x9333, 0x34);
    OV5642_write_cmos_sensor(0x9334, 0x74);
    OV5642_write_cmos_sensor(0x9335, 0x80);
    OV5642_write_cmos_sensor(0x9336, 0xf0);
    OV5642_write_cmos_sensor(0x9337, 0xa3);
    OV5642_write_cmos_sensor(0x9338, 0x74);
    OV5642_write_cmos_sensor(0x9339, 0x2a);
    OV5642_write_cmos_sensor(0x933a, 0xf0);
    OV5642_write_cmos_sensor(0x933b, 0xa3);
    OV5642_write_cmos_sensor(0x933c, 0x74);
    OV5642_write_cmos_sensor(0x933d, 0x14);
    OV5642_write_cmos_sensor(0x933e, 0xf0);
    OV5642_write_cmos_sensor(0x933f, 0x90);
    OV5642_write_cmos_sensor(0x9340, 0x50);
    OV5642_write_cmos_sensor(0x9341, 0x30);
    OV5642_write_cmos_sensor(0x9342, 0xe4);
    OV5642_write_cmos_sensor(0x9343, 0xf0);
    OV5642_write_cmos_sensor(0x9344, 0xa3);
    OV5642_write_cmos_sensor(0x9345, 0x74);
    OV5642_write_cmos_sensor(0x9346, 0x02);
    OV5642_write_cmos_sensor(0x9347, 0xf0);
    OV5642_write_cmos_sensor(0x9348, 0xa3);
    OV5642_write_cmos_sensor(0x9349, 0xe4);
    OV5642_write_cmos_sensor(0x934a, 0xf0);
    OV5642_write_cmos_sensor(0x934b, 0xa3);
    OV5642_write_cmos_sensor(0x934c, 0x74);
    OV5642_write_cmos_sensor(0x934d, 0x80);
    OV5642_write_cmos_sensor(0x934e, 0xf0);
    OV5642_write_cmos_sensor(0x934f, 0xe4);
    OV5642_write_cmos_sensor(0x9350, 0xf5);
    OV5642_write_cmos_sensor(0x9351, 0x0a);
    OV5642_write_cmos_sensor(0x9352, 0x12);
    OV5642_write_cmos_sensor(0x9353, 0x11);
    OV5642_write_cmos_sensor(0x9354, 0x10);
    OV5642_write_cmos_sensor(0x9355, 0x75);
    OV5642_write_cmos_sensor(0x9356, 0x79);
    OV5642_write_cmos_sensor(0x9357, 0x02);
    OV5642_write_cmos_sensor(0x9358, 0x75);
    OV5642_write_cmos_sensor(0x9359, 0x0a);
    OV5642_write_cmos_sensor(0x935a, 0x01);
    OV5642_write_cmos_sensor(0x935b, 0x12);
    OV5642_write_cmos_sensor(0x935c, 0x12);
    OV5642_write_cmos_sensor(0x935d, 0x91);
    OV5642_write_cmos_sensor(0x935e, 0xd2);
    OV5642_write_cmos_sensor(0x935f, 0x18);
    OV5642_write_cmos_sensor(0x9360, 0xd2);
    OV5642_write_cmos_sensor(0x9361, 0x19);
    OV5642_write_cmos_sensor(0x9362, 0xc2);
    OV5642_write_cmos_sensor(0x9363, 0x3a);
    OV5642_write_cmos_sensor(0x9364, 0xc2);
    OV5642_write_cmos_sensor(0x9365, 0x39);
    OV5642_write_cmos_sensor(0x9366, 0xd2);
    OV5642_write_cmos_sensor(0x9367, 0x1a);
    OV5642_write_cmos_sensor(0x9368, 0xd2);
    OV5642_write_cmos_sensor(0x9369, 0x36);
    OV5642_write_cmos_sensor(0x936a, 0xd2);
    OV5642_write_cmos_sensor(0x936b, 0x30);
    OV5642_write_cmos_sensor(0x936c, 0xc2);
    OV5642_write_cmos_sensor(0x936d, 0x35);
    OV5642_write_cmos_sensor(0x936e, 0xc2);
    OV5642_write_cmos_sensor(0x936f, 0x3b);
    OV5642_write_cmos_sensor(0x9370, 0x22);
    OV5642_write_cmos_sensor(0x9371, 0x85);
    OV5642_write_cmos_sensor(0x9372, 0x10);
    OV5642_write_cmos_sensor(0x9373, 0x11);
    OV5642_write_cmos_sensor(0x9374, 0x7f);
    OV5642_write_cmos_sensor(0x9375, 0x08);
    OV5642_write_cmos_sensor(0x9376, 0xe5);
    OV5642_write_cmos_sensor(0x9377, 0x11);
    OV5642_write_cmos_sensor(0x9378, 0x30);
    OV5642_write_cmos_sensor(0x9379, 0xe7);
    OV5642_write_cmos_sensor(0x937a, 0x04);
    OV5642_write_cmos_sensor(0x937b, 0xd2);
    OV5642_write_cmos_sensor(0x937c, 0x29);
    OV5642_write_cmos_sensor(0x937d, 0x80);
    OV5642_write_cmos_sensor(0x937e, 0x02);
    OV5642_write_cmos_sensor(0x937f, 0xc2);
    OV5642_write_cmos_sensor(0x9380, 0x29);
    OV5642_write_cmos_sensor(0x9381, 0x12);
    OV5642_write_cmos_sensor(0x9382, 0x01);
    OV5642_write_cmos_sensor(0x9383, 0x6c);
    OV5642_write_cmos_sensor(0x9384, 0x75);
    OV5642_write_cmos_sensor(0x9385, 0x51);
    OV5642_write_cmos_sensor(0x9386, 0x0a);
    OV5642_write_cmos_sensor(0x9387, 0xae);
    OV5642_write_cmos_sensor(0x9388, 0x51);
    OV5642_write_cmos_sensor(0x9389, 0x15);
    OV5642_write_cmos_sensor(0x938a, 0x51);
    OV5642_write_cmos_sensor(0x938b, 0xee);
    OV5642_write_cmos_sensor(0x938c, 0x70);
    OV5642_write_cmos_sensor(0x938d, 0xf9);
    OV5642_write_cmos_sensor(0x938e, 0xe5);
    OV5642_write_cmos_sensor(0x938f, 0x11);
    OV5642_write_cmos_sensor(0x9390, 0x25);
    OV5642_write_cmos_sensor(0x9391, 0xe0);
    OV5642_write_cmos_sensor(0x9392, 0xf5);
    OV5642_write_cmos_sensor(0x9393, 0x11);
    OV5642_write_cmos_sensor(0x9394, 0xd2);
    OV5642_write_cmos_sensor(0x9395, 0x28);
    OV5642_write_cmos_sensor(0x9396, 0x12);
    OV5642_write_cmos_sensor(0x9397, 0x01);
    OV5642_write_cmos_sensor(0x9398, 0x6c);
    OV5642_write_cmos_sensor(0x9399, 0x75);
    OV5642_write_cmos_sensor(0x939a, 0x51);
    OV5642_write_cmos_sensor(0x939b, 0x0a);
    OV5642_write_cmos_sensor(0x939c, 0xae);
    OV5642_write_cmos_sensor(0x939d, 0x51);
    OV5642_write_cmos_sensor(0x939e, 0x15);
    OV5642_write_cmos_sensor(0x939f, 0x51);
    OV5642_write_cmos_sensor(0x93a0, 0xee);
    OV5642_write_cmos_sensor(0x93a1, 0x70);
    OV5642_write_cmos_sensor(0x93a2, 0xf9);
    OV5642_write_cmos_sensor(0x93a3, 0xc2);
    OV5642_write_cmos_sensor(0x93a4, 0x28);
    OV5642_write_cmos_sensor(0x93a5, 0x12);
    OV5642_write_cmos_sensor(0x93a6, 0x01);
    OV5642_write_cmos_sensor(0x93a7, 0x6c);
    OV5642_write_cmos_sensor(0x93a8, 0x75);
    OV5642_write_cmos_sensor(0x93a9, 0x51);
    OV5642_write_cmos_sensor(0x93aa, 0x05);
    OV5642_write_cmos_sensor(0x93ab, 0xae);
    OV5642_write_cmos_sensor(0x93ac, 0x51);
    OV5642_write_cmos_sensor(0x93ad, 0x15);
    OV5642_write_cmos_sensor(0x93ae, 0x51);
    OV5642_write_cmos_sensor(0x93af, 0xee);
    OV5642_write_cmos_sensor(0x93b0, 0x70);
    OV5642_write_cmos_sensor(0x93b1, 0xf9);
    OV5642_write_cmos_sensor(0x93b2, 0xdf);
    OV5642_write_cmos_sensor(0x93b3, 0xc2);
    OV5642_write_cmos_sensor(0x93b4, 0x22);
    OV5642_write_cmos_sensor(0x93b5, 0xc2);
    OV5642_write_cmos_sensor(0x93b6, 0xaf);
    OV5642_write_cmos_sensor(0x93b7, 0x90);
    OV5642_write_cmos_sensor(0x93b8, 0x30);
    OV5642_write_cmos_sensor(0x93b9, 0x27);
    OV5642_write_cmos_sensor(0x93ba, 0x74);
    OV5642_write_cmos_sensor(0x93bb, 0xfa);
    OV5642_write_cmos_sensor(0x93bc, 0xf0);
    OV5642_write_cmos_sensor(0x93bd, 0x12);
    OV5642_write_cmos_sensor(0x93be, 0x0e);
    OV5642_write_cmos_sensor(0x93bf, 0x99);
    OV5642_write_cmos_sensor(0x93c0, 0x12);
    OV5642_write_cmos_sensor(0x93c1, 0x14);
    OV5642_write_cmos_sensor(0x93c2, 0x83);
    OV5642_write_cmos_sensor(0x93c3, 0xe4);
    OV5642_write_cmos_sensor(0x93c4, 0xf5);
    OV5642_write_cmos_sensor(0x93c5, 0x33);
    OV5642_write_cmos_sensor(0x93c6, 0xd2);
    OV5642_write_cmos_sensor(0x93c7, 0xaf);
    OV5642_write_cmos_sensor(0x93c8, 0x12);
    OV5642_write_cmos_sensor(0x93c9, 0x0c);
    OV5642_write_cmos_sensor(0x93ca, 0x84);
    OV5642_write_cmos_sensor(0x93cb, 0x30);
    OV5642_write_cmos_sensor(0x93cc, 0x30);
    OV5642_write_cmos_sensor(0x93cd, 0x03);
    OV5642_write_cmos_sensor(0x93ce, 0x12);
    OV5642_write_cmos_sensor(0x93cf, 0x06);
    OV5642_write_cmos_sensor(0x93d0, 0xee);
    OV5642_write_cmos_sensor(0x93d1, 0x30);
    OV5642_write_cmos_sensor(0x93d2, 0x34);
    OV5642_write_cmos_sensor(0x93d3, 0x03);
    OV5642_write_cmos_sensor(0x93d4, 0x12);
    OV5642_write_cmos_sensor(0x93d5, 0x12);
    OV5642_write_cmos_sensor(0x93d6, 0x38);
    OV5642_write_cmos_sensor(0x93d7, 0x30);
    OV5642_write_cmos_sensor(0x93d8, 0x3b);
    OV5642_write_cmos_sensor(0x93d9, 0xee);
    OV5642_write_cmos_sensor(0x93da, 0xc2);
    OV5642_write_cmos_sensor(0x93db, 0x3b);
    OV5642_write_cmos_sensor(0x93dc, 0xd2);
    OV5642_write_cmos_sensor(0x93dd, 0x35);
    OV5642_write_cmos_sensor(0x93de, 0x30);
    OV5642_write_cmos_sensor(0x93df, 0x00);
    OV5642_write_cmos_sensor(0x93e0, 0x05);
    OV5642_write_cmos_sensor(0x93e1, 0x12);
    OV5642_write_cmos_sensor(0x93e2, 0x14);
    OV5642_write_cmos_sensor(0x93e3, 0x56);
    OV5642_write_cmos_sensor(0x93e4, 0x80);
    OV5642_write_cmos_sensor(0x93e5, 0x09);
    OV5642_write_cmos_sensor(0x93e6, 0x20);
    OV5642_write_cmos_sensor(0x93e7, 0x07);
    OV5642_write_cmos_sensor(0x93e8, 0x06);
    OV5642_write_cmos_sensor(0x93e9, 0x30);
    OV5642_write_cmos_sensor(0x93ea, 0x06);
    OV5642_write_cmos_sensor(0x93eb, 0x03);
    OV5642_write_cmos_sensor(0x93ec, 0x12);
    OV5642_write_cmos_sensor(0x93ed, 0x0d);
    OV5642_write_cmos_sensor(0x93ee, 0x92);
    OV5642_write_cmos_sensor(0x93ef, 0xc2);
    OV5642_write_cmos_sensor(0x93f0, 0x35);
    OV5642_write_cmos_sensor(0x93f1, 0x80);
    OV5642_write_cmos_sensor(0x93f2, 0xd5);
    OV5642_write_cmos_sensor(0x93f3, 0x12);
    OV5642_write_cmos_sensor(0x93f4, 0x0f);
    OV5642_write_cmos_sensor(0x93f5, 0xc6);
    OV5642_write_cmos_sensor(0x93f6, 0xd2);
    OV5642_write_cmos_sensor(0x93f7, 0x3c);
    OV5642_write_cmos_sensor(0x93f8, 0x12);
    OV5642_write_cmos_sensor(0x93f9, 0x0f);
    OV5642_write_cmos_sensor(0x93fa, 0x39);
    OV5642_write_cmos_sensor(0x93fb, 0xd2);
    OV5642_write_cmos_sensor(0x93fc, 0x3c);
    OV5642_write_cmos_sensor(0x93fd, 0x12);
    OV5642_write_cmos_sensor(0x93fe, 0x11);
    OV5642_write_cmos_sensor(0x93ff, 0x75);
    OV5642_write_cmos_sensor(0x9400, 0xe5);
    OV5642_write_cmos_sensor(0x9401, 0x7c);
    OV5642_write_cmos_sensor(0x9402, 0xd3);
    OV5642_write_cmos_sensor(0x9403, 0x95);
    OV5642_write_cmos_sensor(0x9404, 0x7d);
    OV5642_write_cmos_sensor(0x9405, 0x40);
    OV5642_write_cmos_sensor(0x9406, 0x03);
    OV5642_write_cmos_sensor(0x9407, 0xd3);
    OV5642_write_cmos_sensor(0x9408, 0x80);
    OV5642_write_cmos_sensor(0x9409, 0x01);
    OV5642_write_cmos_sensor(0x940a, 0xc3);
    OV5642_write_cmos_sensor(0x940b, 0x50);
    OV5642_write_cmos_sensor(0x940c, 0x14);
    OV5642_write_cmos_sensor(0x940d, 0x20);
    OV5642_write_cmos_sensor(0x940e, 0x37);
    OV5642_write_cmos_sensor(0x940f, 0x0e);
    OV5642_write_cmos_sensor(0x9410, 0xe5);
    OV5642_write_cmos_sensor(0x9411, 0x24);
    OV5642_write_cmos_sensor(0x9412, 0x54);
    OV5642_write_cmos_sensor(0x9413, 0x1f);
    OV5642_write_cmos_sensor(0x9414, 0xff);
    OV5642_write_cmos_sensor(0x9415, 0xbf);
    OV5642_write_cmos_sensor(0x9416, 0x1f);
    OV5642_write_cmos_sensor(0x9417, 0x06);
    OV5642_write_cmos_sensor(0x9418, 0x30);
    OV5642_write_cmos_sensor(0x9419, 0x25);
    OV5642_write_cmos_sensor(0x941a, 0x03);
    OV5642_write_cmos_sensor(0x941b, 0x20);
    OV5642_write_cmos_sensor(0x941c, 0x26);
    OV5642_write_cmos_sensor(0x941d, 0x03);
    OV5642_write_cmos_sensor(0x941e, 0x02);
    OV5642_write_cmos_sensor(0x941f, 0x15);
    OV5642_write_cmos_sensor(0x9420, 0x2b);
    OV5642_write_cmos_sensor(0x9421, 0x12);
    OV5642_write_cmos_sensor(0x9422, 0x02);
    OV5642_write_cmos_sensor(0x9423, 0xb3);
    OV5642_write_cmos_sensor(0x9424, 0x22);
    OV5642_write_cmos_sensor(0x9425, 0xe5);
    OV5642_write_cmos_sensor(0x9426, 0x7d);
    OV5642_write_cmos_sensor(0x9427, 0x70);
    OV5642_write_cmos_sensor(0x9428, 0x19);
    OV5642_write_cmos_sensor(0x9429, 0x12);
    OV5642_write_cmos_sensor(0x942a, 0x15);
    OV5642_write_cmos_sensor(0x942b, 0x03);
    OV5642_write_cmos_sensor(0x942c, 0xc2);
    OV5642_write_cmos_sensor(0x942d, 0x3c);
    OV5642_write_cmos_sensor(0x942e, 0x12);
    OV5642_write_cmos_sensor(0x942f, 0x0f);
    OV5642_write_cmos_sensor(0x9430, 0x39);
    OV5642_write_cmos_sensor(0x9431, 0xc2);
    OV5642_write_cmos_sensor(0x9432, 0x3c);
    OV5642_write_cmos_sensor(0x9433, 0x12);
    OV5642_write_cmos_sensor(0x9434, 0x11);
    OV5642_write_cmos_sensor(0x9435, 0x75);
    OV5642_write_cmos_sensor(0x9436, 0xc2);
    OV5642_write_cmos_sensor(0x9437, 0x03);
    OV5642_write_cmos_sensor(0x9438, 0x12);
    OV5642_write_cmos_sensor(0x9439, 0x15);
    OV5642_write_cmos_sensor(0x943a, 0x2b);
    OV5642_write_cmos_sensor(0x943b, 0xd2);
    OV5642_write_cmos_sensor(0x943c, 0x02);
    OV5642_write_cmos_sensor(0x943d, 0xd2);
    OV5642_write_cmos_sensor(0x943e, 0x01);
    OV5642_write_cmos_sensor(0x943f, 0xd2);
    OV5642_write_cmos_sensor(0x9440, 0x00);
    OV5642_write_cmos_sensor(0x9441, 0x22);
    OV5642_write_cmos_sensor(0x9442, 0x30);
    OV5642_write_cmos_sensor(0x9443, 0x03);
    OV5642_write_cmos_sensor(0x9444, 0x08);
    OV5642_write_cmos_sensor(0x9445, 0xc2);
    OV5642_write_cmos_sensor(0x9446, 0x03);
    OV5642_write_cmos_sensor(0x9447, 0xc2);
    OV5642_write_cmos_sensor(0x9448, 0x04);
    OV5642_write_cmos_sensor(0x9449, 0x12);
    OV5642_write_cmos_sensor(0x944a, 0x02);
    OV5642_write_cmos_sensor(0x944b, 0x9b);
    OV5642_write_cmos_sensor(0x944c, 0x22);
    OV5642_write_cmos_sensor(0x944d, 0xe4);
    OV5642_write_cmos_sensor(0x944e, 0xf5);
    OV5642_write_cmos_sensor(0x944f, 0x0c);
    OV5642_write_cmos_sensor(0x9450, 0x12);
    OV5642_write_cmos_sensor(0x9451, 0x11);
    OV5642_write_cmos_sensor(0x9452, 0xd8);
    OV5642_write_cmos_sensor(0x9453, 0xd2);
    OV5642_write_cmos_sensor(0x9454, 0x03);
    OV5642_write_cmos_sensor(0x9455, 0x22);
    OV5642_write_cmos_sensor(0x9456, 0xe5);
    OV5642_write_cmos_sensor(0x9457, 0x20);
    OV5642_write_cmos_sensor(0x9458, 0x54);
    OV5642_write_cmos_sensor(0x9459, 0x07);
    OV5642_write_cmos_sensor(0x945a, 0xff);
    OV5642_write_cmos_sensor(0x945b, 0xbf);
    OV5642_write_cmos_sensor(0x945c, 0x01);
    OV5642_write_cmos_sensor(0x945d, 0x03);
    OV5642_write_cmos_sensor(0x945e, 0x02);
    OV5642_write_cmos_sensor(0x945f, 0x14);
    OV5642_write_cmos_sensor(0x9460, 0x25);
    OV5642_write_cmos_sensor(0x9461, 0xe5);
    OV5642_write_cmos_sensor(0x9462, 0x20);
    OV5642_write_cmos_sensor(0x9463, 0x54);
    OV5642_write_cmos_sensor(0x9464, 0x07);
    OV5642_write_cmos_sensor(0x9465, 0xff);
    OV5642_write_cmos_sensor(0x9466, 0xbf);
    OV5642_write_cmos_sensor(0x9467, 0x07);
    OV5642_write_cmos_sensor(0x9468, 0x03);
    OV5642_write_cmos_sensor(0x9469, 0x02);
    OV5642_write_cmos_sensor(0x946a, 0x14);
    OV5642_write_cmos_sensor(0x946b, 0xa9);
    OV5642_write_cmos_sensor(0x946c, 0xe5);
    OV5642_write_cmos_sensor(0x946d, 0x20);
    OV5642_write_cmos_sensor(0x946e, 0x54);
    OV5642_write_cmos_sensor(0x946f, 0x07);
    OV5642_write_cmos_sensor(0x9470, 0xff);
    OV5642_write_cmos_sensor(0x9471, 0xbf);
    OV5642_write_cmos_sensor(0x9472, 0x03);
    OV5642_write_cmos_sensor(0x9473, 0x03);
    OV5642_write_cmos_sensor(0x9474, 0x02);
    OV5642_write_cmos_sensor(0x9475, 0x13);
    OV5642_write_cmos_sensor(0x9476, 0xf3);
    OV5642_write_cmos_sensor(0x9477, 0xe5);
    OV5642_write_cmos_sensor(0x9478, 0x20);
    OV5642_write_cmos_sensor(0x9479, 0x54);
    OV5642_write_cmos_sensor(0x947a, 0x07);
    OV5642_write_cmos_sensor(0x947b, 0xff);
    OV5642_write_cmos_sensor(0x947c, 0xbf);
    OV5642_write_cmos_sensor(0x947d, 0x05);
    OV5642_write_cmos_sensor(0x947e, 0x03);
    OV5642_write_cmos_sensor(0x947f, 0x12);
    OV5642_write_cmos_sensor(0x9480, 0x15);
    OV5642_write_cmos_sensor(0x9481, 0x69);
    OV5642_write_cmos_sensor(0x9482, 0x22);
    OV5642_write_cmos_sensor(0x9483, 0x12);
    OV5642_write_cmos_sensor(0x9484, 0x13);
    OV5642_write_cmos_sensor(0x9485, 0x2a);
    OV5642_write_cmos_sensor(0x9486, 0x12);
    OV5642_write_cmos_sensor(0x9487, 0x15);
    OV5642_write_cmos_sensor(0x9488, 0x77);
    OV5642_write_cmos_sensor(0x9489, 0x50);
    OV5642_write_cmos_sensor(0x948a, 0x04);
    OV5642_write_cmos_sensor(0x948b, 0xd2);
    OV5642_write_cmos_sensor(0x948c, 0x05);
    OV5642_write_cmos_sensor(0x948d, 0x80);
    OV5642_write_cmos_sensor(0x948e, 0x02);
    OV5642_write_cmos_sensor(0x948f, 0xc2);
    OV5642_write_cmos_sensor(0x9490, 0x05);
    OV5642_write_cmos_sensor(0x9491, 0x12);
    OV5642_write_cmos_sensor(0x9492, 0x02);
    OV5642_write_cmos_sensor(0x9493, 0x51);
    OV5642_write_cmos_sensor(0x9494, 0xc2);
    OV5642_write_cmos_sensor(0x9495, 0x37);
    OV5642_write_cmos_sensor(0x9496, 0xc2);
    OV5642_write_cmos_sensor(0x9497, 0x31);
    OV5642_write_cmos_sensor(0x9498, 0xd2);
    OV5642_write_cmos_sensor(0x9499, 0x34);
    OV5642_write_cmos_sensor(0x949a, 0x12);
    OV5642_write_cmos_sensor(0x949b, 0x02);
    OV5642_write_cmos_sensor(0x949c, 0x7f);
    OV5642_write_cmos_sensor(0x949d, 0xb5);
    OV5642_write_cmos_sensor(0x949e, 0x07);
    OV5642_write_cmos_sensor(0x949f, 0x03);
    OV5642_write_cmos_sensor(0x94a0, 0xd3);
    OV5642_write_cmos_sensor(0x94a1, 0x80);
    OV5642_write_cmos_sensor(0x94a2, 0x01);
    OV5642_write_cmos_sensor(0x94a3, 0xc3);
    OV5642_write_cmos_sensor(0x94a4, 0x40);
    OV5642_write_cmos_sensor(0x94a5, 0x02);
    OV5642_write_cmos_sensor(0x94a6, 0xc2);
    OV5642_write_cmos_sensor(0x94a7, 0x05);
    OV5642_write_cmos_sensor(0x94a8, 0x22);
    OV5642_write_cmos_sensor(0x94a9, 0x12);
    OV5642_write_cmos_sensor(0x94aa, 0x0f);
    OV5642_write_cmos_sensor(0x94ab, 0xc6);
    OV5642_write_cmos_sensor(0x94ac, 0xd2);
    OV5642_write_cmos_sensor(0x94ad, 0x3c);
    OV5642_write_cmos_sensor(0x94ae, 0x12);
    OV5642_write_cmos_sensor(0x94af, 0x0f);
    OV5642_write_cmos_sensor(0x94b0, 0x39);
    OV5642_write_cmos_sensor(0x94b1, 0xd2);
    OV5642_write_cmos_sensor(0x94b2, 0x3c);
    OV5642_write_cmos_sensor(0x94b3, 0x12);
    OV5642_write_cmos_sensor(0x94b4, 0x11);
    OV5642_write_cmos_sensor(0x94b5, 0x75);
    OV5642_write_cmos_sensor(0x94b6, 0x12);
    OV5642_write_cmos_sensor(0x94b7, 0x15);
    OV5642_write_cmos_sensor(0x94b8, 0x2b);
    OV5642_write_cmos_sensor(0x94b9, 0xe5);
    OV5642_write_cmos_sensor(0x94ba, 0x32);
    OV5642_write_cmos_sensor(0x94bb, 0xd3);
    OV5642_write_cmos_sensor(0x94bc, 0x95);
    OV5642_write_cmos_sensor(0x94bd, 0x7d);
    OV5642_write_cmos_sensor(0x94be, 0x40);
    OV5642_write_cmos_sensor(0x94bf, 0x05);
    OV5642_write_cmos_sensor(0x94c0, 0xe4);
    OV5642_write_cmos_sensor(0x94c1, 0x95);
    OV5642_write_cmos_sensor(0x94c2, 0x7d);
    OV5642_write_cmos_sensor(0x94c3, 0x40);
    OV5642_write_cmos_sensor(0x94c4, 0x06);
    OV5642_write_cmos_sensor(0x94c5, 0xc2);
    OV5642_write_cmos_sensor(0x94c6, 0x02);
    OV5642_write_cmos_sensor(0x94c7, 0xd2);
    OV5642_write_cmos_sensor(0x94c8, 0x01);
    OV5642_write_cmos_sensor(0x94c9, 0xd2);
    OV5642_write_cmos_sensor(0x94ca, 0x00);
    OV5642_write_cmos_sensor(0x94cb, 0x22);
    OV5642_write_cmos_sensor(0x94cc, 0xe4);
    OV5642_write_cmos_sensor(0x94cd, 0xff);
    OV5642_write_cmos_sensor(0x94ce, 0xfe);
    OV5642_write_cmos_sensor(0x94cf, 0xc3);
    OV5642_write_cmos_sensor(0x94d0, 0xef);
    OV5642_write_cmos_sensor(0x94d1, 0x95);
    OV5642_write_cmos_sensor(0x94d2, 0x0e);
    OV5642_write_cmos_sensor(0x94d3, 0xee);
    OV5642_write_cmos_sensor(0x94d4, 0x95);
    OV5642_write_cmos_sensor(0x94d5, 0x0d);
    OV5642_write_cmos_sensor(0x94d6, 0x50);
    OV5642_write_cmos_sensor(0x94d7, 0x15);
    OV5642_write_cmos_sensor(0x94d8, 0x7d);
    OV5642_write_cmos_sensor(0x94d9, 0x8a);
    OV5642_write_cmos_sensor(0x94da, 0x7c);
    OV5642_write_cmos_sensor(0x94db, 0x02);
    OV5642_write_cmos_sensor(0x94dc, 0xed);
    OV5642_write_cmos_sensor(0x94dd, 0x1d);
    OV5642_write_cmos_sensor(0x94de, 0xaa);
    OV5642_write_cmos_sensor(0x94df, 0x04);
    OV5642_write_cmos_sensor(0x94e0, 0x70);
    OV5642_write_cmos_sensor(0x94e1, 0x01);
    OV5642_write_cmos_sensor(0x94e2, 0x1c);
    OV5642_write_cmos_sensor(0x94e3, 0x4a);
    OV5642_write_cmos_sensor(0x94e4, 0x70);
    OV5642_write_cmos_sensor(0x94e5, 0xf6);
    OV5642_write_cmos_sensor(0x94e6, 0x0f);
    OV5642_write_cmos_sensor(0x94e7, 0xbf);
    OV5642_write_cmos_sensor(0x94e8, 0x00);
    OV5642_write_cmos_sensor(0x94e9, 0x01);
    OV5642_write_cmos_sensor(0x94ea, 0x0e);
    OV5642_write_cmos_sensor(0x94eb, 0x80);
    OV5642_write_cmos_sensor(0x94ec, 0xe2);
    OV5642_write_cmos_sensor(0x94ed, 0x22);
    OV5642_write_cmos_sensor(0x94ee, 0x75);
    OV5642_write_cmos_sensor(0x94ef, 0x48);
    OV5642_write_cmos_sensor(0x94f0, 0x10);
    OV5642_write_cmos_sensor(0x94f1, 0x75);
    OV5642_write_cmos_sensor(0x94f2, 0x49);
    OV5642_write_cmos_sensor(0x94f3, 0xaa);
    OV5642_write_cmos_sensor(0x94f4, 0x90);
    OV5642_write_cmos_sensor(0x94f5, 0x10);
    OV5642_write_cmos_sensor(0x94f6, 0xa8);
    OV5642_write_cmos_sensor(0x94f7, 0xe4);
    OV5642_write_cmos_sensor(0x94f8, 0x93);
    OV5642_write_cmos_sensor(0x94f9, 0xf5);
    OV5642_write_cmos_sensor(0x94fa, 0x7c);
    OV5642_write_cmos_sensor(0x94fb, 0xa3);
    OV5642_write_cmos_sensor(0x94fc, 0xe4);
    OV5642_write_cmos_sensor(0x94fd, 0x93);
    OV5642_write_cmos_sensor(0x94fe, 0xf5);
    OV5642_write_cmos_sensor(0x94ff, 0x32);
    OV5642_write_cmos_sensor(0x9500, 0xc2);
    OV5642_write_cmos_sensor(0x9501, 0x38);
    OV5642_write_cmos_sensor(0x9502, 0x22);
    OV5642_write_cmos_sensor(0x9503, 0xe4);
    OV5642_write_cmos_sensor(0x9504, 0xff);
    OV5642_write_cmos_sensor(0x9505, 0xef);
    OV5642_write_cmos_sensor(0x9506, 0x25);
    OV5642_write_cmos_sensor(0x9507, 0xe0);
    OV5642_write_cmos_sensor(0x9508, 0x24);
    OV5642_write_cmos_sensor(0x9509, 0x56);
    OV5642_write_cmos_sensor(0x950a, 0xf8);
    OV5642_write_cmos_sensor(0x950b, 0xe4);
    OV5642_write_cmos_sensor(0x950c, 0xf6);
    OV5642_write_cmos_sensor(0x950d, 0x08);
    OV5642_write_cmos_sensor(0x950e, 0xf6);
    OV5642_write_cmos_sensor(0x950f, 0x0f);
    OV5642_write_cmos_sensor(0x9510, 0xbf);
    OV5642_write_cmos_sensor(0x9511, 0x07);
    OV5642_write_cmos_sensor(0x9512, 0xf2);
    OV5642_write_cmos_sensor(0x9513, 0x53);
    OV5642_write_cmos_sensor(0x9514, 0x24);
    OV5642_write_cmos_sensor(0x9515, 0x80);
    OV5642_write_cmos_sensor(0x9516, 0x22);
    OV5642_write_cmos_sensor(0x9517, 0xc2);
    OV5642_write_cmos_sensor(0x9518, 0x03);
    OV5642_write_cmos_sensor(0x9519, 0xd2);
    OV5642_write_cmos_sensor(0x951a, 0x04);
    OV5642_write_cmos_sensor(0x951b, 0x12);
    OV5642_write_cmos_sensor(0x951c, 0x02);
    OV5642_write_cmos_sensor(0x951d, 0x9b);
    OV5642_write_cmos_sensor(0x951e, 0xc2);
    OV5642_write_cmos_sensor(0x951f, 0x3c);
    OV5642_write_cmos_sensor(0x9520, 0x12);
    OV5642_write_cmos_sensor(0x9521, 0x0f);
    OV5642_write_cmos_sensor(0x9522, 0x39);
    OV5642_write_cmos_sensor(0x9523, 0xc2);
    OV5642_write_cmos_sensor(0x9524, 0x3c);
    OV5642_write_cmos_sensor(0x9525, 0x12);
    OV5642_write_cmos_sensor(0x9526, 0x11);
    OV5642_write_cmos_sensor(0x9527, 0x75);
    OV5642_write_cmos_sensor(0x9528, 0xd2);
    OV5642_write_cmos_sensor(0x9529, 0x34);
    OV5642_write_cmos_sensor(0x952a, 0x22);
    OV5642_write_cmos_sensor(0x952b, 0xe5);
    OV5642_write_cmos_sensor(0x952c, 0x7d);
    OV5642_write_cmos_sensor(0x952d, 0xc3);
    OV5642_write_cmos_sensor(0x952e, 0x95);
    OV5642_write_cmos_sensor(0x952f, 0x7c);
    OV5642_write_cmos_sensor(0x9530, 0x40);
    OV5642_write_cmos_sensor(0x9531, 0x01);
    OV5642_write_cmos_sensor(0x9532, 0x22);
    OV5642_write_cmos_sensor(0x9533, 0xe5);
    OV5642_write_cmos_sensor(0x9534, 0x7d);
    OV5642_write_cmos_sensor(0x9535, 0x04);
    OV5642_write_cmos_sensor(0x9536, 0xf5);
    OV5642_write_cmos_sensor(0x9537, 0x0c);
    OV5642_write_cmos_sensor(0x9538, 0x12);
    OV5642_write_cmos_sensor(0x9539, 0x11);
    OV5642_write_cmos_sensor(0x953a, 0xd8);
    OV5642_write_cmos_sensor(0x953b, 0x22);
    OV5642_write_cmos_sensor(0x953c, 0xe5);
    OV5642_write_cmos_sensor(0x953d, 0x7d);
    OV5642_write_cmos_sensor(0x953e, 0x70);
    OV5642_write_cmos_sensor(0x953f, 0x02);
    OV5642_write_cmos_sensor(0x9540, 0xc3);
    OV5642_write_cmos_sensor(0x9541, 0x22);
    OV5642_write_cmos_sensor(0x9542, 0xe5);
    OV5642_write_cmos_sensor(0x9543, 0x7d);
    OV5642_write_cmos_sensor(0x9544, 0x14);
    OV5642_write_cmos_sensor(0x9545, 0xf5);
    OV5642_write_cmos_sensor(0x9546, 0x0c);
    OV5642_write_cmos_sensor(0x9547, 0x12);
    OV5642_write_cmos_sensor(0x9548, 0x11);
    OV5642_write_cmos_sensor(0x9549, 0xd8);
    OV5642_write_cmos_sensor(0x954a, 0x22);
    OV5642_write_cmos_sensor(0x954b, 0xe5);
    OV5642_write_cmos_sensor(0x954c, 0x7e);
    OV5642_write_cmos_sensor(0x954d, 0xb4);
    OV5642_write_cmos_sensor(0x954e, 0x01);
    OV5642_write_cmos_sensor(0x954f, 0x09);
    OV5642_write_cmos_sensor(0x9550, 0x12);
    OV5642_write_cmos_sensor(0x9551, 0x14);
    OV5642_write_cmos_sensor(0x9552, 0xee);
    OV5642_write_cmos_sensor(0x9553, 0xe4);
    OV5642_write_cmos_sensor(0x9554, 0xf5);
    OV5642_write_cmos_sensor(0x9555, 0x0c);
    OV5642_write_cmos_sensor(0x9556, 0x12);
    OV5642_write_cmos_sensor(0x9557, 0x11);
    OV5642_write_cmos_sensor(0x9558, 0xd8);
    OV5642_write_cmos_sensor(0x9559, 0x22);
    OV5642_write_cmos_sensor(0x955a, 0xe5);
    OV5642_write_cmos_sensor(0x955b, 0x7e);
    OV5642_write_cmos_sensor(0x955c, 0x24);
    OV5642_write_cmos_sensor(0x955d, 0xfe);
    OV5642_write_cmos_sensor(0x955e, 0x60);
    OV5642_write_cmos_sensor(0x955f, 0x06);
    OV5642_write_cmos_sensor(0x9560, 0x04);
    OV5642_write_cmos_sensor(0x9561, 0x70);
    OV5642_write_cmos_sensor(0x9562, 0x05);
    OV5642_write_cmos_sensor(0x9563, 0xd2);
    OV5642_write_cmos_sensor(0x9564, 0x37);
    OV5642_write_cmos_sensor(0x9565, 0x22);
    OV5642_write_cmos_sensor(0x9566, 0xc2);
    OV5642_write_cmos_sensor(0x9567, 0x37);
    OV5642_write_cmos_sensor(0x9568, 0x22);
    OV5642_write_cmos_sensor(0x9569, 0xe5);
    OV5642_write_cmos_sensor(0x956a, 0x31);
    OV5642_write_cmos_sensor(0x956b, 0xd3);
    OV5642_write_cmos_sensor(0x956c, 0x94);
    OV5642_write_cmos_sensor(0x956d, 0x00);
    OV5642_write_cmos_sensor(0x956e, 0x40);
    OV5642_write_cmos_sensor(0x956f, 0x03);
    OV5642_write_cmos_sensor(0x9570, 0x15);
    OV5642_write_cmos_sensor(0x9571, 0x31);
    OV5642_write_cmos_sensor(0x9572, 0x22);
    OV5642_write_cmos_sensor(0x9573, 0x12);
    OV5642_write_cmos_sensor(0x9574, 0x15);
    OV5642_write_cmos_sensor(0x9575, 0x17);
    OV5642_write_cmos_sensor(0x9576, 0x22);
    OV5642_write_cmos_sensor(0x9577, 0x12);
    OV5642_write_cmos_sensor(0x9578, 0x14);
    OV5642_write_cmos_sensor(0x9579, 0xee);
    OV5642_write_cmos_sensor(0x957a, 0xe4);
    OV5642_write_cmos_sensor(0x957b, 0xf5);
    OV5642_write_cmos_sensor(0x957c, 0x0c);
    OV5642_write_cmos_sensor(0x957d, 0x12);
    OV5642_write_cmos_sensor(0x957e, 0x11);
    OV5642_write_cmos_sensor(0x957f, 0xd8);
    OV5642_write_cmos_sensor(0x9580, 0x22);
    OV5642_write_cmos_sensor(0x3024, 0x00);
    OV5642_write_cmos_sensor(0x3025, 0x00);
    OV5642_write_cmos_sensor(0x5082, 0x00);
    OV5642_write_cmos_sensor(0x5083, 0x00);
    OV5642_write_cmos_sensor(0x5084, 0x00);
    OV5642_write_cmos_sensor(0x5085, 0x00);
    OV5642_write_cmos_sensor(0x3026, 0x00);
    OV5642_write_cmos_sensor(0x3027, 0xFF);
    OV5642_write_cmos_sensor(0x3000, 0x00);

#endif

    do {
        state = (UINT8)OV5642_read_cmos_sensor(0x3027);
        mDELAY(1);
        if (iteration-- == 0)
        {
            RETAILMSG(1, (TEXT("[OV5642]STA_FOCUS state check ERROR!!0x%x \r\n")), state);
            break;
        }
    } while(state!=0x00);
    KD_IMGSENSOR_PROFILE("OV5642_FOCUS_AD5820_Init");
    return;    
}   /*  OV5642_FOCUS_AD5820_Init  */

// Step to Specified position
static void OV5642_FOCUS_AD5820_Move_to(UINT16 a_u2MovePosition)
{
    
    UINT8 state = 0x8F;
    UINT32 iteration = 100;
    static UINT16 u2CurrPos = 0;

    //SENSORDB("OV5642_FOCUS_AD5820_Move_to %d\n", a_u2MovePosition);     

	if (u2CurrPos == a_u2MovePosition)   {return;}
				
    KD_IMGSENSOR_PROFILE_INIT();
    
    a_u2MovePosition = a_u2MovePosition&0x07FF; //10bit for AD5820

	u2CurrPos = a_u2MovePosition;
	
    do {
    	 state = (UINT8)OV5642_read_cmos_sensor(0x3025);
        RETAILMSG(1, (TEXT("[OV5642]Move stage state !!0x%x \r\n")), state);
        mDELAY(1);
        if (iteration-- == 0)
        {
            RETAILMSG(1, (TEXT("[OV5642]Move stage state check ERROR!!0x%x \r\n")), state);
            break;
        }

    } while(state!=0x00);
       
    OV5642_write_cmos_sensor(0x3025,0x0F);
    OV5642_write_cmos_sensor(0x5084,(UINT8)((u2CurrPos>>8)&0x07));
    OV5642_write_cmos_sensor(0x5085,(UINT8)(u2CurrPos&0x00FF));
    OV5642_write_cmos_sensor(0x3024,0x05);
        
    KD_IMGSENSOR_PROFILE("OV5642_FOCUS_AD5820_Move_to");    
}

static void OV5642_FOCUS_AD5820_Get_AF_Status(UINT32 *pFeatureReturnPara32)
{
    UINT32 state = OV5642_read_cmos_sensor(0x3025);

    if (state == 0)
    {
        *pFeatureReturnPara32 = 0;
    }
    else
    {
        *pFeatureReturnPara32 = 1;
    }
}

static void OV5642_FOCUS_AD5820_Get_AF_Inf(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = 0;
}

static void OV5642_FOCUS_AD5820_Get_AF_Macro(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = 1023;
}


//set constant focus
static void OV5642_FOCUS_AD5820_Constant_Focus(void)
{
    UINT8 state = 0x8F;
    UINT32 iteration = 300;

    //send idle command to firmware
    OV5642_write_cmos_sensor(0x3025,0x01);
    OV5642_write_cmos_sensor(0x3024,0x08);

    iteration = 300;
    do {
    	 state = (UINT8)OV5642_read_cmos_sensor(0x3027);
        if (state == 0xEE)
        {
        RETAILMSG(1, (TEXT("[OV5642]AD5820_Single_Focus !!0x%x \r\n")), state);
            return ;
        }    	 
        if (iteration-- == 0)
        {
            RETAILMSG(1, (TEXT("[OV5642]AD5820_Single_Focus time out !!0x%x \r\n")), state);
            return ;
        }   
        mDELAY(1);
    } while(state!=0x00); 

    //send constant focus mode command to firmware
    OV5642_write_cmos_sensor(0x3025,0x01);
    OV5642_write_cmos_sensor(0x3024,0x04);

    iteration = 5000;
    do {
    	 state = (UINT8)OV5642_read_cmos_sensor(0x3027);
        if (state == 0xEE)
        {
            RETAILMSG(1, (TEXT("[OV5642]AD5820_Single_Focus ERROR II !!0x%x \r\n")), state);
            return ;
        }
        if (iteration-- == 0)
        {
            RETAILMSG(1, (TEXT("[OV5642]AD5820_Single_Focus time out II\r\n")));
            return ;
        }
        mDELAY(1);
    } while((state!=0x02)&&(state!=0x01));//0x02 : focused 0x01: is focusing
    return;
}

void OV5642_Set_Mirror_Flip(kal_uint8 image_mirror)
{
	kal_uint8 iTemp = 0;

	//MOD1[0x03],
	//Sensor mirror image control, bit[1]
	//Sensor flip image control, bit[0];
	iTemp = OV5642_read_cmos_sensor(0x3818) & 0x9f;	//Clear the mirror and flip bits.
    switch (image_mirror)
	{
	    case IMAGE_NORMAL:
	        OV5642_write_cmos_sensor(0x3818, iTemp | 0xc0);	//Set normal
	         OV5642_write_cmos_sensor(0x3621, 0xc7);
			
                //SET_FIRST_GRAB_COLOR(BAYER_Gr);
	        break;

		case IMAGE_V_MIRROR:
			OV5642_write_cmos_sensor(0x3818, iTemp | 0xe0);	//Set flip
			OV5642_write_cmos_sensor(0x3621, 0xc7);
			
                //SET_FIRST_GRAB_COLOR(BAYER_B);
			break;
			
		case IMAGE_H_MIRROR:
			OV5642_write_cmos_sensor(0x3818, iTemp | 0x80);	//Set mirror
			OV5642_write_cmos_sensor(0x3621, 0xe7);
                //SET_FIRST_GRAB_COLOR(BAYER_Gr);
			break;
				
	    case IMAGE_HV_MIRROR:
	        OV5642_write_cmos_sensor(0x3818, iTemp | 0xa0);	//Set mirror and flip
	        OV5642_write_cmos_sensor(0x3621, 0xe7);
                //SET_FIRST_GRAB_COLOR(BAYER_B);
	        break;
    }
}


/*************************************************************************
* FUNCTION
*   OV5642Preview
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
UINT32 OV5642Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 iStartX = 0, iStartY = 0;

    g_iOV5642_Mode = OV5642_MODE_PREVIEW;

    //if(OV5642_720P == OV5642_g_RES)
    {
        OV5642_set_720P();
    }

    if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        OV5642_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {
        OV5642_MPEG4_encode_mode = KAL_FALSE;
    }

    iStartX += OV5642_IMAGE_SENSOR_PV_STARTX;
    iStartY += OV5642_IMAGE_SENSOR_PV_STARTY;

    //sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
    
    //OV5642_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

#if 0    
    iTemp = OV5642_read_cmos_sensor(0x3818) & 0x9f;	//Clear the mirror and flip bits.
    switch (sensor_config_data->SensorImageMirror)
    {
        case IMAGE_NORMAL:
            OV5642_write_cmos_sensor(0x3818, iTemp | 0xc0);	//Set normal
            OV5642_write_cmos_sensor(0x3621, 0xc7);
            //SET_FIRST_GRAB_COLOR(BAYER_Gr);
            break;

        case IMAGE_V_MIRROR:
            OV5642_write_cmos_sensor(0x3818, iTemp | 0xe0);	//Set flip
            OV5642_write_cmos_sensor(0x3621, 0xc7);

            //SET_FIRST_GRAB_COLOR(BAYER_B);
            break;

        case IMAGE_H_MIRROR:
            OV5642_write_cmos_sensor(0x3818, iTemp | 0x80);	//Set mirror
            OV5642_write_cmos_sensor(0x3621, 0xe7);
            //SET_FIRST_GRAB_COLOR(BAYER_Gr);
        break;

        case IMAGE_HV_MIRROR:
            OV5642_write_cmos_sensor(0x3818, iTemp | 0xa0);	//Set mirror and flip
            OV5642_write_cmos_sensor(0x3621, 0xe7);
            //SET_FIRST_GRAB_COLOR(BAYER_B);
            break;
    }
#endif     

    OV5642_dummy_pixels = 0;
    OV5642_dummy_lines = 0;
    OV5642_PV_dummy_pixels = OV5642_dummy_pixels;
    OV5642_PV_dummy_lines = OV5642_dummy_lines;

    OV5642_SetDummy(OV5642_dummy_pixels, OV5642_dummy_lines);
    //OV5642_SetShutter(OV5642_pv_exposure_lines);

    memcpy(&OV5642SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    image_window->GrabStartX= iStartX;
    image_window->GrabStartY= iStartY;
    image_window->ExposureWindowWidth= OV5642_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
    image_window->ExposureWindowHeight= OV5642_IMAGE_SENSOR_PV_HEIGHT - 2*iStartY;
    return ERROR_NONE;
}	/* OV5642Preview() */



/*******************************************************************************
*
********************************************************************************/
UINT32 OV5642Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint32 shutter=OV5642_pv_exposure_lines;
    kal_uint16 iStartX = 0, iStartY = 0;

    SENSORDB("Preview Shutter = %d, Gain = %d\n", shutter, read_OV5642_gain());     
    g_iOV5642_Mode = OV5642_MODE_CAPTURE;

    if(sensor_config_data->EnableShutterTansfer==KAL_TRUE)
        shutter=sensor_config_data->CaptureShutter;

    if ((image_window->ImageTargetWidth<= OV5642_IMAGE_SENSOR_PV_WIDTH) &&
        (image_window->ImageTargetHeight<= OV5642_IMAGE_SENSOR_PV_HEIGHT)) {
        OV5642_dummy_pixels= 0;
        OV5642_dummy_lines = 0;

        shutter = ((UINT32)(shutter*(OV5642_IMAGE_SENSOR_720P_PIXELS_LINE + OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS + OV5642_PV_dummy_pixels))) /
                                                        ((OV5642_IMAGE_SENSOR_5M_PIXELS_LINE+ OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS + OV5642_dummy_pixels)) ;
        shutter = shutter * OV5642_CAP_pclk / OV5642_PV_pclk;         
        iStartX = OV5642_IMAGE_SENSOR_PV_STARTX;
        iStartY = OV5642_IMAGE_SENSOR_PV_STARTY;
        image_window->GrabStartX=iStartX;
        image_window->GrabStartY=iStartY;
        image_window->ExposureWindowWidth=OV5642_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
        image_window->ExposureWindowHeight=OV5642_IMAGE_SENSOR_PV_HEIGHT- 2*iStartY;
    }
    else { // 5M  Mode
        OV5642_dummy_pixels= 0;
        OV5642_dummy_lines = 0;        
        OV5642_set_5M15fps();

        //sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
     
        //OV5642SetFlipMirror(sensor_config_data->SensorImageMirror); 
        OV5642_write_cmos_sensor(0x3818,0xa0);
        OV5642_write_cmos_sensor(0x3621,0x29);        
        
        //SVGA Internal CLK = 1/4 UXGA Internal CLK
        //shutter = 4* shutter;
        shutter = ((UINT32)(shutter*(OV5642_IMAGE_SENSOR_720P_PIXELS_LINE + OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS + OV5642_PV_dummy_pixels))) /
                                                        ((OV5642_IMAGE_SENSOR_5M_PIXELS_LINE+ OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS + OV5642_dummy_pixels)) ;
        shutter = shutter * OV5642_CAP_pclk / OV5642_PV_pclk; 
        iStartX = 2* OV5642_IMAGE_SENSOR_PV_STARTX;
        iStartY = 2* OV5642_IMAGE_SENSOR_PV_STARTY;

        image_window->GrabStartX=iStartX;
        image_window->GrabStartY=iStartY;
        image_window->ExposureWindowWidth=OV5642_IMAGE_SENSOR_FULL_WIDTH -2*iStartX;
        image_window->ExposureWindowHeight=OV5642_IMAGE_SENSOR_FULL_HEIGHT-2*iStartY;
    }//5M Capture
    // config flashlight preview setting
    if(OV5642_5M == OV5642_g_RES) //add start
    {
        sensor_config_data->DefaultPclk = 32500000;
        sensor_config_data->Pixels = OV5642_IMAGE_SENSOR_5M_PIXELS_LINE + OV5642_PV_dummy_pixels;
        sensor_config_data->FrameLines =OV5642_PV_PERIOD_LINE_NUMS+OV5642_PV_dummy_lines;
    }
    else
    {
        sensor_config_data->DefaultPclk = 32500000;
        sensor_config_data->Pixels = OV5642_IMAGE_SENSOR_5M_PIXELS_LINE+OV5642_dummy_pixels;
        sensor_config_data->FrameLines =OV5642_FULL_PERIOD_LINE_NUMS+OV5642_dummy_lines;
    }
    sensor_config_data->Lines = image_window->ExposureWindowHeight;
    sensor_config_data->Shutter =shutter;

    //OV5642_SetDummy(OV5642_dummy_pixels, OV5642_dummy_lines);
    OV5642_SetDummy(OV5642_dummy_pixels, OV5642_dummy_lines);
    OV5642_SetShutter(shutter);
    SENSORDB("Capture Shutter = %d, Gain = %d\n", shutter, read_OV5642_gain());     
    memcpy(&OV5642SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}	/* OV5642Capture() */

UINT32 OV5642GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH - 4*OV5642_IMAGE_SENSOR_PV_STARTX;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT - 4*OV5642_IMAGE_SENSOR_PV_STARTY;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH - 2*OV5642_IMAGE_SENSOR_PV_STARTX;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT - 2*OV5642_IMAGE_SENSOR_PV_STARTY;
    pSensorResolution->SensorVideoWidth=IMAGE_SENSOR_PV_WIDTH - 2*OV5642_IMAGE_SENSOR_PV_STARTX;
    pSensorResolution->SensorVideoHeight=IMAGE_SENSOR_PV_HEIGHT - 2*OV5642_IMAGE_SENSOR_PV_STARTY;
    return ERROR_NONE;
}   /* OV5642GetResolution() */

UINT32 OV5642GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
                        pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH - 2*OV5642_IMAGE_SENSOR_PV_STARTX;
                        pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT - 2*OV5642_IMAGE_SENSOR_PV_STARTY;
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH - 4*OV5642_IMAGE_SENSOR_PV_STARTX;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_HEIGHT - 4*OV5642_IMAGE_SENSOR_PV_STARTY;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;


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
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = OV5642_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = OV5642_IMAGE_SENSOR_PV_STARTY;             
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = OV5642_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = OV5642_IMAGE_SENSOR_PV_STARTY;             
            break;
        default:
            pSensorInfo->SensorClockFreq=26;
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 1; 
            pSensorInfo->SensorGrabStartY = 1;             
            break;
    }

    OV5642PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &OV5642SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* OV5642GetInfo() */


UINT32 OV5642Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                    OV5642Preview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	case MSDK_SCENARIO_ID_CAMERA_ZSD:
            OV5642Capture(pImageWindow, pSensorConfigData);
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
} /* OV5642Control() */


UINT32 OV5642SetVideoMode(UINT16 u2FrameRate)
{
//    SENSORDB("[OV5642SetVideoMode] frame rate = %d\n", u2FrameRate);
    OV5642_MPEG4_encode_mode = KAL_TRUE; 
    if (u2FrameRate == 30)
    {
            //OV5642_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642_sensor_pclk/30)/(OV5642_PV_PERIOD_PIXEL_NUMS + OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS + OV5642_PV_dummy_pixels));
            OV5642_MAX_EXPOSURE_LINES = OV5642_PV_PERIOD_LINE_NUMS + OV5642_PV_PERIOD_EXTRA_LINE_NUMS; 
            if(OV5642_pv_exposure_lines < (OV5642_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                OV5642_write_cmos_sensor(0x350C, (OV5642_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                OV5642_write_cmos_sensor(0x350D, OV5642_MAX_EXPOSURE_LINES & 0xFF);
                OV5642_CURRENT_FRAME_LINES = OV5642_MAX_EXPOSURE_LINES;
            }
            //OV5642_MAX_EXPOSURE_LINES = OV5642_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP;
    }
    else if (u2FrameRate == 15)       
    {
            //OV5642_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642_sensor_pclk/15)/(OV5642_PV_PERIOD_PIXEL_NUMS + OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS + OV5642_PV_dummy_pixels));
            OV5642_MAX_EXPOSURE_LINES = (OV5642_PV_PERIOD_LINE_NUMS + OV5642_PV_PERIOD_EXTRA_LINE_NUMS) * 2;  
            OV5642_write_cmos_sensor(0x350C, (OV5642_MAX_EXPOSURE_LINES >> 8) & 0xFF);
            OV5642_write_cmos_sensor(0x350D, OV5642_MAX_EXPOSURE_LINES & 0xFF);
            OV5642_CURRENT_FRAME_LINES = OV5642_MAX_EXPOSURE_LINES;
            OV5642_MAX_EXPOSURE_LINES = OV5642_CURRENT_FRAME_LINES - OV5642_SHUTTER_LINES_GAP;
    }
    else 
    {
        printk("Wrong frame rate setting \n");
    }
    return TRUE;
}

UINT32 OV5642SetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
    SENSORDB("[OV5642_write_cmos_sensor] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
    if(bEnable) {   // enable auto flicker   
        if ((u2FrameRate < 300) && (u2FrameRate > 10)) {  // the frame should be between 30 fps and 10 fps
            OV5642_Auto_Flicker_mode = KAL_TRUE; 
            OV5642_MAX_EXPOSURE_LINES = (OV5642_PV_PERIOD_LINE_NUMS + OV5642_PV_PERIOD_EXTRA_LINE_NUMS) * 300 / u2FrameRate; 
            if(OV5642_pv_exposure_lines < (OV5642_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                OV5642_write_cmos_sensor(0x350C, (OV5642_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                OV5642_write_cmos_sensor(0x350D, OV5642_MAX_EXPOSURE_LINES & 0xFF);
                OV5642_CURRENT_FRAME_LINES = OV5642_MAX_EXPOSURE_LINES;
            }
            else {
                printk("The exposure time bigger than frame length = %d %d\n", OV5642_pv_exposure_lines, OV5642_MAX_EXPOSURE_LINES);        	
            }
        }
        else {
            printk("Wrong frame rate setting = %d\n", u2FrameRate);        	
            OV5642_Auto_Flicker_mode = KAL_FALSE; 
        }
    }
    else {
        OV5642_Auto_Flicker_mode = KAL_FALSE; 
        printk("Disable Auto flicker\n");    
    }
    return TRUE;
}

UINT32 OV5642SetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[OV5642SetTestPatternMode] Test pattern enable:%d\n", bEnable);
    
    if(bEnable) {   // enable color bar   
	 /* 0x80,0x00: color bar 0x85,0x12: color square 0x85,0x1A: B/W square */
	 OV5642_write_cmos_sensor(0x503D, 0x80);
	 OV5642_write_cmos_sensor(0x503E, 0x00);    
    } else {
        OV5642_write_cmos_sensor(0x503D, 0x00);  // disable color bar test pattern
    }
    return TRUE;
}

UINT32 OV5642SetSoftwarePWDNMode(kal_bool bEnable)
{
    SENSORDB("[OV5642SetSoftwarePWDNMode] Software Power down enable:%d\n", bEnable);
    
    if(bEnable) {   // enable software power down mode   
	 OV5642_write_cmos_sensor(0x3008, 0x40);
    } else {
        OV5642_write_cmos_sensor(0x3008, 0x00);  
    }
    return TRUE;
}

UINT32 OV5642FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
            *pFeatureReturnPara16++=OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS + OV5642_PV_PERIOD_PIXEL_NUMS + OV5642_dummy_pixels;//OV5642_PV_PERIOD_PIXEL_NUMS+OV5642_dummy_pixels;
            *pFeatureReturnPara16=OV5642_PV_PERIOD_LINE_NUMS+OV5642_dummy_lines;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            *pFeatureReturnPara32 = 55250000; //19500000;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            OV5642_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            OV5642_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            OV5642_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            OV5642_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV5642_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = OV5642_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
                OV5642SensorCCT[i].Addr=*pFeatureData32++;
                OV5642SensorCCT[i].Para=*pFeatureData32++;
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV5642SensorCCT[i].Addr;
                *pFeatureData32++=OV5642SensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
                OV5642SensorReg[i].Addr=*pFeatureData32++;
                OV5642SensorReg[i].Para=*pFeatureData32++;
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV5642SensorReg[i].Addr;
                *pFeatureData32++=OV5642SensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=OV5642_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, OV5642SensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, OV5642SensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &OV5642SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            OV5642_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV5642_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=OV5642_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV5642_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV5642_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV5642_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
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

        case SENSOR_FEATURE_INITIALIZE_AF:
            //SENSORDB("OV5642_FOCUS_AD5820_Init\n");
            OV5642_FOCUS_AD5820_Init();
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            //SENSORDB("OV5642_FOCUS_AD5820_Constant_Focus\n");
            OV5642_FOCUS_AD5820_Constant_Focus();
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            //SENSORDB("OV5642_FOCUS_AD5820_Move_to %d\n", *pFeatureData16);
            OV5642_FOCUS_AD5820_Move_to(*pFeatureData16);
            break;
        case SENSOR_FEATURE_GET_AF_STATUS:
            OV5642_FOCUS_AD5820_Get_AF_Status(pFeatureReturnPara32);            
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_AF_INF:
            OV5642_FOCUS_AD5820_Get_AF_Inf(pFeatureReturnPara32);
            *pFeatureParaLen=4;            
            break;
        case SENSOR_FEATURE_GET_AF_MACRO:
            OV5642_FOCUS_AD5820_Get_AF_Macro(pFeatureReturnPara32);
            *pFeatureParaLen=4;            
            break;                
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            OV5642SetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV5642GetSensorID(pFeatureReturnPara32); 
            break; 
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            OV5642SetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));            
	        break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            OV5642SetTestPatternMode((BOOL)*pFeatureData16);        	
            break;
        case SENSOR_FEATURE_SET_SOFTWARE_PWDN:
            OV5642SetSoftwarePWDNMode((BOOL)*pFeatureData16);        	        	
            break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* OV5642FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncOV5642=
{
    OV5642Open,
    OV5642GetInfo,
    OV5642GetResolution,
    OV5642FeatureControl,
    OV5642Control,
    OV5642Close
};

UINT32 OV5642_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncOV5642;

    return ERROR_NONE;
}   /* SensorInit() */

