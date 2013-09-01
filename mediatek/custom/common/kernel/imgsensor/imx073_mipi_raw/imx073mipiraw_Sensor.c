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
 * 02 21 2012 koli.lin
 * [ALPS00240109] [Need Patch] [Volunteer Patch]
 * [Camera] Add a new frame rate (24fps) for video mode.
 *
 * 02 19 2012 koli.lin
 * [ALPS00237113] [Performance][Video recording]Recording preview the screen have flash
 * [Camera] 1. Modify the AE converge speed in the video mode.
 *                2. Modify the isp gain delay frame with sensor exposure time and gain synchronization.
 *
 * 11 11 2011 koli.lin
 * [ALPS00086510] [Camera] preview?率?低
 * [Camera] Modify the exposure line for flicker enable.
 *
 * 11 11 2011 koli.lin
 * [ALPS00030473] [Camera]
 * [Camera] Modify the flicker frame rate control.
 *
 * 11 01 2011 koli.lin
 * [ALPS00030473] [Camera]
 * [Camera] Add the flicker flag disable control before enter the ZSD mode.
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
 * 10 17 2011 koli.lin
 * [ALPS00074853] [Camera]flicker is serious when preview with light object
 * [Camera] change the sensor binning mode to 2x from 4x to reduce the flicker serious..
 *
 * 10 07 2011 koli.lin
 * [ALPS00077581] [Need Patch][Sanity Fail] When entering to Camera at the second time, camera cannot work and will see black screen
 * [Camera] Rest the flicker flag value during the sensor initial.
 *
 * 06 13 2011 koli.lin
 * [ALPS00053429] [Need Patch] [Volunteer Patch]
 * [Camera] Modify the sensor color order for the CCT tool.
 *
 * 06 07 2011 koli.lin
 * [ALPS00050047] [Camera]AE flash when set EV as -2
 * [Camera] Rollback the preview resolution to 800x600.
 *
 * 06 07 2011 koli.lin
 * [ALPS00049935] [Camera] AE is not correct when switch between camera still mode and video mode
 * [Camera] Rollback the preview resolution to 800x600.
 *
 * 06 01 2011 koli.lin
 * [ALPS00051509] [Need Patch] [Volunteer Patch]
 * [Camera] Fix the mipi color order is not correctly for MT6573.
 *
 * 05 17 2011 koli.lin
 * [ALPS00048194] [Need Patch] [Volunteer Patch]
 * [Camera]. Chagne the preview size to 1600x1200 for IMX073 sensor.
 *
 * 05 02 2011 koli.lin
 * [ALPS00040837] [Need check with submitter][WW FT][MT6573][Guangzhou]Exception ANR
 * [Camera] Fix the exposure time calculate mistake.
 *
 * 04 22 2011 koli.lin
 * [ALPS00042482] [Need Patch] [Volunteer Patch]
 * [Camera] Move the sensor senstivity control to AE for preview and capture mode.
 *
 * 04 11 2011 koli.lin
 * [ALPS00039429] [Need Patch] [Volunteer Patch]
 * [Camera] Add flicker frame rate and sensor test pattern feature id.
 *
 * 04 08 2011 koli.lin
 * [ALPS00037040] [Display]There is tearing on preview screen
 * [Camera] Disable the gain group setting and reduce the gain delay frame number.
 *
 * 04 06 2011 koli.lin
 * [ALPS00036187] [Camera]after do EV shot sometimes, part of the preview screen flashes
 * [Camera] Modify the capture delay frame to get better performance..
 *
 * 04 02 2011 koli.lin
 * [ALPS00036213] [Camera]change EV but it does not work
 * [Camera] 1.Add the CSI2 debug message for dump register function.
 *                2.Modify image start position to avoid the black line..
 *
 * 04 01 2011 koli.lin
 * [ALPS00143914] [Android Build Warning Issue] mediatek/custom/out/ztemt73v2/kernel/imgsensor/imx073mipiraw_Sensor.c
 * [Camera] 1.Clear the compile warning message.
 *                2. Add the sensor driver 2D/3D information.
 *                3.Add the auto flicker control and test pattern feature.
 *
 * 04 01 2011 koli.lin
 * [ALPS00037668] [MPEG4 recording]record high quality video with night mode, the frame rate is not 15fps
 * [Camera] Add the video mode frame rate control function.
 *
 * 04 01 2011 koli.lin
 * [ALPS00037670] [MPEG4 recording]the frame rate of fine quality video can not reach 30fps
 * [Camera]Modify the sensor preview output resolution and line time to fix frame rate at 30fps for video mode.
 *
 * 03 15 2011 koli.lin
 * [ALPS00034474] [Need Patch] [Volunteer Patch]
 * Move sensor driver current setting to isp of middleware.
 *
 * 03 10 2011 koli.lin
 * [ALPS00033930] [Need Patch] [Volunteer Patch]
 * Moving the CSI2 control flow from sensor driver to middleware.
 *
 * 03 02 2011 koli.lin
 * [ALPS00032905] [Need Patch] [Volunteer Patch]
 * Reset to default.
 *
 * 03 02 2011 koli.lin
 * [ALPS00032905] [Need Patch] [Volunteer Patch]
 * Provide the sensor width/height sampling ratio..
 *
 * 02 25 2011 koli.lin
 * [ALPS00032248] [Need Patch] [Volunteer Patch]
 * Close the csi2 before sensor driver close.
 *
 * 02 11 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Add the csi2 control to imx073 sensor driver.
 *
 * 02 11 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Change sensor driver preview size ratio to 4:3.
 *
 * 02 11 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Modify the IMX073 sensor driver for preview mode.
 *
 * 02 11 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Create IMX073 sensor driver to database.
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

#include "imx073mipiraw_Sensor.h"
#include "imx073mipiraw_Camera_Sensor_para.h"
#include "imx073mipiraw_CameraCustomized.h"

kal_bool  IMX073MIPI_MPEG4_encode_mode = KAL_FALSE;
kal_bool IMX073MIPI_Auto_Flicker_mode = KAL_FALSE;

kal_uint32 IMX073MIPI_PV_pclk = 648; 
kal_uint32 IMX073MIPI_CAP_pclk = 648*2; // 2*576;

kal_uint16  IMX073MIPI_sensor_gain_base=0x0;
/* MAX/MIN Explosure Lines Used By AE Algorithm */
kal_uint16 IMX073MIPI_MAX_EXPOSURE_LINES = IMX073MIPI_PV_PERIOD_LINE_NUMS;//650;
kal_uint8  IMX073MIPI_MIN_EXPOSURE_LINES = 2;
kal_uint32 IMX073MIPI_isp_master_clock;
kal_uint16 IMX073MIPI_CURRENT_FRAME_LINES = IMX073MIPI_PV_PERIOD_LINE_NUMS;//650;

static kal_uint16 IMX073MIPI_dummy_pixels=0, IMX073MIPI_dummy_lines=0;
kal_uint16 IMX073MIPI_PV_dummy_pixels=0,IMX073MIPI_PV_dummy_lines=0;

kal_uint8 IMX073MIPI_sensor_write_I2C_address = IMX073MIPI_WRITE_ID;
kal_uint8 IMX073MIPI_sensor_read_I2C_address = IMX073MIPI_READ_ID;

#define SENSORDB(fmt, arg...) printk( "[IMX073MIPIRaw] "  fmt, ##arg)
#define RETAILMSG(x,...)
#define TEXT

UINT8 IMX073MIPIPixelClockDivider=0;
kal_uint32 IMX073MIPI_sensor_pclk=52000000;//19500000;

kal_uint16 IMX073MIPI_pv_exposure_lines=0x100, IMX073MIPI_g_iBackupExtraExp = 0, IMX073MIPI_extra_exposure_lines = 0;

kal_uint16 IMX073MIPI_sensor_id=0;

MSDK_SENSOR_CONFIG_STRUCT IMX073MIPISensorConfigData;

kal_uint32 IMX073MIPI_FAC_SENSOR_REG;
kal_uint16 IMX073MIPI_sensor_flip_value; 

MSDK_SCENARIO_ID_ENUM CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;

#define IMX073MIPI_MaxGainIndex 50																				 // Gain Index
kal_uint16 IMX073MIPI_sensorGainMapping[IMX073MIPI_MaxGainIndex][2] = {
    { 64,  0}, { 68, 15}, { 71, 28}, { 75, 40}, { 79, 51}, {84, 61}, { 88, 70}, { 92, 78}, { 95, 85}, { 99, 92},
    {103, 98}, { 107, 104}, {112,110}, {116,115}, {119,119}, {124,124}, {128,128}, {136,136}, {143,142}, {151,148},
    {160,154}, {167,158}, {176,163}, {184,167}, {192,171}, {199,174}, {207,177}, {215,180}, {224,183}, {230,185},
    {240,188}, {248,190}, {256,192}, {273,196}, {287,199}, {303,202}, {321,205}, {334,207}, {348,209}, {364,211}, 
    {381,213}, {399,215}, {420,217}, {431,218}, {442,219}, {455,220}, {468,221}, {481,222}, {496,223}, {512,224}    
};

/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT IMX073MIPISensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT IMX073MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/

typedef enum
{
  IMX073MIPI_720P,       //1M 1280x960
  IMX073MIPI_8M,           //8M 3280x2464
} IMX073MIPI_RES_TYPE;
IMX073MIPI_RES_TYPE IMX073MIPI_g_RES=IMX073MIPI_720P;

typedef enum
{
  IMX073MIPI_MODE_PREVIEW,  //1M  	1280x960
  IMX073MIPI_MODE_CAPTURE   //8M    3280x2464
} IMX073MIPI_MODE;
IMX073MIPI_MODE g_iIMX073MIPI_Mode=IMX073MIPI_MODE_PREVIEW;

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);

#define IMX073MIPI_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, IMX073MIPI_WRITE_ID)

kal_uint16 IMX073MIPI_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,IMX073MIPI_WRITE_ID);
    return get_byte;
}

#define Sleep(ms) mdelay(ms)
static DEFINE_SPINLOCK(imx073_drv_lock);


void IMX073MIPI_write_shutter(kal_uint16 shutter)
{
    kal_uint16 iExp = shutter;
    kal_uint16 IMX073MIPI_g_iExtra_ExpLines = 0 ;
	unsigned long flags;
		
    if (IMX073MIPI_g_RES == IMX073MIPI_720P) {
        if (iExp <= (IMX073MIPI_PV_EXPOSURE_LIMITATION + IMX073MIPI_PV_PERIOD_EXTRA_LINE_NUMS - IMX073MIPI_SHUTTER_LINES_GAP)) {
            IMX073MIPI_g_iExtra_ExpLines = IMX073MIPI_PV_EXPOSURE_LIMITATION + IMX073MIPI_PV_PERIOD_EXTRA_LINE_NUMS;
        }else {
            IMX073MIPI_g_iExtra_ExpLines = iExp + IMX073MIPI_SHUTTER_LINES_GAP;
        }

    }else {
        if (iExp <= (IMX073MIPI_FULL_EXPOSURE_LIMITATION + IMX073MIPI_FULL_PERIOD_EXTRA_LINE_NUMS - IMX073MIPI_SHUTTER_LINES_GAP)) {
            IMX073MIPI_g_iExtra_ExpLines = IMX073MIPI_FULL_EXPOSURE_LIMITATION + IMX073MIPI_FULL_PERIOD_EXTRA_LINE_NUMS;
        }else {
            IMX073MIPI_g_iExtra_ExpLines = iExp + IMX073MIPI_SHUTTER_LINES_GAP;
        }
    }

    if(IMX073MIPI_Auto_Flicker_mode == TRUE) {    // in the auto flicker mode. the frame rate should extend 1/128
    	IMX073MIPI_g_iExtra_ExpLines = IMX073MIPI_g_iExtra_ExpLines + (IMX073MIPI_g_iExtra_ExpLines >> 7);  //
    }
    //printk("[IMX073MIPI_write_shutter] mode:%d frame line:%d expos line:%d\n", IMX073MIPI_g_RES, IMX073MIPI_g_iExtra_ExpLines, iExp);
    
    IMX073MIPI_write_cmos_sensor(0x0104, 1);        
    if (IMX073MIPI_MPEG4_encode_mode != TRUE) {
        IMX073MIPI_write_cmos_sensor(0x0340, (IMX073MIPI_g_iExtra_ExpLines >>8) & 0xFF);
        IMX073MIPI_write_cmos_sensor(0x0341, IMX073MIPI_g_iExtra_ExpLines & 0xFF);	
    } else if (iExp > (IMX073MIPI_MAX_EXPOSURE_LINES - IMX073MIPI_SHUTTER_LINES_GAP)) {   // protect the video exposure line over the frame line
        iExp = IMX073MIPI_MAX_EXPOSURE_LINES - IMX073MIPI_SHUTTER_LINES_GAP;
    }
    
    IMX073MIPI_write_cmos_sensor(0x0202, (iExp >> 8) & 0xFF);
    IMX073MIPI_write_cmos_sensor(0x0203, iExp  & 0xFF);
    IMX073MIPI_write_cmos_sensor(0x0104, 0);
    
    //printk("[IMX073MIPI_write_shutter] frame line:%d expos line:%d 0x202:0x%02x 0x203:0x%02x 0x340:0x%02x 0x341:0x%02x\n", IMX073MIPI_g_iExtra_ExpLines, iExp, IMX073MIPI_read_cmos_sensor(0x0202), IMX073MIPI_read_cmos_sensor(0x0203), 
    //	IMX073MIPI_read_cmos_sensor(0x0340), IMX073MIPI_read_cmos_sensor(0x0341));
    spin_lock_irqsave(&imx073_drv_lock,flags);
    IMX073MIPI_g_iBackupExtraExp = IMX073MIPI_g_iExtra_ExpLines;    
    spin_unlock_irqrestore(&imx073_drv_lock,flags);
}   /* write_IMX073MIPI_shutter */

static kal_uint16 IMX073MIPIReg2Gain(const kal_uint8 iReg)
{
    kal_uint8 iI;

    // Range: 1x to 8x
    for (iI = 0; iI < IMX073MIPI_MaxGainIndex; iI++) {
        if(iReg <= IMX073MIPI_sensorGainMapping[iI][1]){
            break;
        }
    }

    return IMX073MIPI_sensorGainMapping[iI][0];
}

static kal_uint8 IMX073MIPIGain2Reg(const kal_uint16 iGain)
{
    kal_uint8 iI;

    for (iI = 0; iI < (IMX073MIPI_MaxGainIndex-1); iI++) {
        if(iGain <= IMX073MIPI_sensorGainMapping[iI][0]){
            break;
        }
    }
   
    if(iGain != IMX073MIPI_sensorGainMapping[iI][0])
    {
         printk("[IMX073MIPIGain2Reg] Gain mapping don't correctly:%d %d \n", iGain, IMX073MIPI_sensorGainMapping[iI][0]);
    }
	
    return IMX073MIPI_sensorGainMapping[iI][1];
}

/*************************************************************************
* FUNCTION
*    IMX073MIPI_SetGain
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
void IMX073MIPI_SetGain(UINT16 iGain)
{
    kal_uint8 iReg;

    iReg = IMX073MIPIGain2Reg(iGain);
//    IMX073MIPI_write_cmos_sensor(0x0104, 1);
    IMX073MIPI_write_cmos_sensor(0x0205, (kal_uint32)iReg);
//    IMX073MIPI_write_cmos_sensor(0x0104, 0);

}   /*  IMX073MIPI_SetGain_SetGain  */


/*************************************************************************
* FUNCTION
*    read_IMX073MIPI_gain
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
kal_uint16 read_IMX073MIPI_gain(void)
{
    return (kal_uint16)IMX073MIPI_read_cmos_sensor(0x0205);
}  /* read_IMX073MIPI_gain */

void write_IMX073MIPI_gain(kal_uint16 gain)
{
    IMX073MIPI_SetGain(gain);
}
void IMX073MIPI_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=IMX073MIPISensorReg[i].Addr; i++)
    {
        IMX073MIPI_write_cmos_sensor(IMX073MIPISensorReg[i].Addr, IMX073MIPISensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX073MIPISensorReg[i].Addr; i++)
    {
        IMX073MIPI_write_cmos_sensor(IMX073MIPISensorReg[i].Addr, IMX073MIPISensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        IMX073MIPI_write_cmos_sensor(IMX073MIPISensorCCT[i].Addr, IMX073MIPISensorCCT[i].Para);
    }
}


/*************************************************************************
* FUNCTION
*    IMX073MIPI_sensor_to_camera_para
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
void IMX073MIPI_sensor_to_camera_para(void)
{
    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=IMX073MIPISensorReg[i].Addr; i++)
    {        
    	temp_data =  IMX073MIPI_read_cmos_sensor(IMX073MIPISensorReg[i].Addr);
		spin_lock(&imx073_drv_lock);
        IMX073MIPISensorReg[i].Para = temp_data;	    
		spin_unlock(&imx073_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=IMX073MIPISensorReg[i].Addr; i++)
    {   
    	temp_data = IMX073MIPI_read_cmos_sensor(IMX073MIPISensorReg[i].Addr);
		spin_lock(&imx073_drv_lock);
        IMX073MIPISensorReg[i].Para = temp_data;	    
		spin_unlock(&imx073_drv_lock);
    }
}

/*************************************************************************
* FUNCTION
*    IMX073MIPI_get_sensor_group_count
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
kal_int32  IMX073MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void IMX073MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void IMX073MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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

            temp_para=IMX073MIPISensorCCT[temp_addr].Para;

            temp_gain = IMX073MIPIReg2Gain(temp_para);

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
                
                    //temp_reg=IMX073MIPISensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=IMX073MIPI_MAX_EXPOSURE_LINES;
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

//void IMX073MIPI_set_isp_driving_current(kal_uint8 current)
//{

//}

kal_bool IMX073MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
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

            temp_para = IMX073MIPIGain2Reg(ItemValue);

		    spin_lock(&imx073_drv_lock);
            IMX073MIPISensorCCT[temp_addr].Para = temp_para;
			spin_unlock(&imx073_drv_lock);

            IMX073MIPI_write_cmos_sensor(IMX073MIPISensorCCT[temp_addr].Addr,temp_para);

			temp_para = read_IMX073MIPI_gain();
			spin_lock(&imx073_drv_lock);
            IMX073MIPI_sensor_gain_base=temp_para;
			spin_unlock(&imx073_drv_lock);


            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
				    spin_lock(&imx073_drv_lock);
                    if(ItemValue==2)
                    {
                        IMX073MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
                        //IMX073MIPI_set_isp_driving_current(ISP_DRIVING_2MA);
                    }
                    else if(ItemValue==3 || ItemValue==4)
                    {
                        IMX073MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
                        //IMX073MIPI_set_isp_driving_current(ISP_DRIVING_4MA);
                    }
                    else if(ItemValue==5 || ItemValue==6)
                    {
                        IMX073MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
                        //IMX073MIPI_set_isp_driving_current(ISP_DRIVING_6MA);
                    }
                    else
                    {
                        IMX073MIPISensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
                        //IMX073MIPI_set_isp_driving_current(ISP_DRIVING_8MA);
                    }
				    spin_unlock(&imx073_drv_lock);
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
                    IMX073MIPI_FAC_SENSOR_REG=ItemValue;
                    break;
                case 1:
                    IMX073MIPI_write_cmos_sensor(IMX073MIPI_FAC_SENSOR_REG,ItemValue);
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

static void IMX073MIPI_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
#if 0
    kal_uint16 PixelsOneline = OV5642_FULL_PERIOD_PIXEL_NUMS;
    if(IMX073MIPI_720P == IMX073MIPI_g_RES)
    {
        PixelsOneline = (OV5642_PV_PERIOD_PIXEL_NUMS + iPixels + OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS);
        if(IMX073MIPI_MPEG4_encode_mode == KAL_FALSE)//for Fix video framerate
            IMX073MIPI_CURRENT_FRAME_LINES = iLines + IMX073MIPI_PV_PERIOD_LINE_NUMS + IMX073MIPI_PV_PERIOD_EXTRA_LINE_NUMS ;
    }
    else if (IMX073MIPI_8M == IMX073MIPI_g_RES)
    {
        PixelsOneline = IMX073MIPI_FULL_PERIOD_PIXEL_NUMS + iPixels + IMX073MIPI_FULL_PERIOD_EXTRA_PIXEL_NUMS;
        IMX073MIPI_CURRENT_FRAME_LINES = iLines + IMX073MIPI_FULL_PERIOD_LINE_NUMS + IMX073MIPI_FULL_PERIOD_EXTRA_LINE_NUMS;
    }
    IMX073MIPI_write_cmos_sensor(0x380c, (PixelsOneline >> 8) & 0xFF);
    IMX073MIPI_write_cmos_sensor(0x380d, PixelsOneline & 0xFF);

    if(OV5642_MPEG4_encode_mode == KAL_FALSE)//for Fix video framerate,set the frame lines in night mode function
    {
        //Set dummy lines.
        //The maximum shutter value = Line_Without_Dummy + Dummy_lines
        IMX073MIPI_write_cmos_sensor(0x350C, (IMX073MIPI_CURRENT_FRAME_LINES >> 8) & 0xFF);
        IMX073MIPI_write_cmos_sensor(0x350D, IMX073MIPI_CURRENT_FRAME_LINES & 0xFF);
        IMX073MIPI_MAX_EXPOSURE_LINES = IMX073MIPI_CURRENT_FRAME_LINES - IMX073MIPI_SHUTTER_LINES_GAP;
    }
#endif	
}   /*  IMX073MIPI_SetDummy */

static void IMX073MIPI_Sensor_Init(void)
{
    IMX073MIPI_write_cmos_sensor(0x0307, 0x18);//INCK 24MHz
    IMX073MIPI_write_cmos_sensor(0x302B, 0x4B);//PLL stablilized time 

    IMX073MIPI_write_cmos_sensor(0x30E5, 0x04);//MIPI enable setting
    IMX073MIPI_write_cmos_sensor(0x3300, 0x00);

    // Global setting
    IMX073MIPI_write_cmos_sensor(0x0101, 0x03);
    IMX073MIPI_write_cmos_sensor(0x0340, 0x02);
    IMX073MIPI_write_cmos_sensor(0x0341, 0x75);
    IMX073MIPI_write_cmos_sensor(0x0202, 0x02);    // exposure time
    IMX073MIPI_write_cmos_sensor(0x0203, 0x68);
    IMX073MIPI_write_cmos_sensor(0x0205, 224);
	
    IMX073MIPI_write_cmos_sensor(0x300A, 0x80);
    IMX073MIPI_write_cmos_sensor(0x3014, 0x08);
    IMX073MIPI_write_cmos_sensor(0x3015, 0x37);
    IMX073MIPI_write_cmos_sensor(0x3017, 0x60);
    IMX073MIPI_write_cmos_sensor(0x301C, 0x01);
    IMX073MIPI_write_cmos_sensor(0x3031, 0x28);
    IMX073MIPI_write_cmos_sensor(0x3040, 0x00);
    IMX073MIPI_write_cmos_sensor(0x3041, 0x60);
    IMX073MIPI_write_cmos_sensor(0x3047, 0x10);
    IMX073MIPI_write_cmos_sensor(0x3051, 0x24);
    IMX073MIPI_write_cmos_sensor(0x3053, 0x34);
    IMX073MIPI_write_cmos_sensor(0x3055, 0x3B);
    IMX073MIPI_write_cmos_sensor(0x3057, 0xC0);
    IMX073MIPI_write_cmos_sensor(0x3060, 0x30);
    IMX073MIPI_write_cmos_sensor(0x3065, 0x00);
    IMX073MIPI_write_cmos_sensor(0x30A1, 0x03);
    IMX073MIPI_write_cmos_sensor(0x30A3, 0x01);
    IMX073MIPI_write_cmos_sensor(0x30AA, 0x88);
    IMX073MIPI_write_cmos_sensor(0x30AB, 0x1C);
    IMX073MIPI_write_cmos_sensor(0x30B0, 0x32);
    IMX073MIPI_write_cmos_sensor(0x30B2, 0x83);
    IMX073MIPI_write_cmos_sensor(0x30D3, 0x04);
    IMX073MIPI_write_cmos_sensor(0x310C, 0xE9);
    IMX073MIPI_write_cmos_sensor(0x310D, 0x00);
    IMX073MIPI_write_cmos_sensor(0x316B, 0x14);
    IMX073MIPI_write_cmos_sensor(0x316D, 0x3B);
    IMX073MIPI_write_cmos_sensor(0x31A4, 0xD8);
    IMX073MIPI_write_cmos_sensor(0x31A6, 0x17);
    IMX073MIPI_write_cmos_sensor(0x31AC, 0xCF);
    IMX073MIPI_write_cmos_sensor(0x31AE, 0xF1);
    IMX073MIPI_write_cmos_sensor(0x31B4, 0xD8);
    IMX073MIPI_write_cmos_sensor(0x31B6, 0x17);
    IMX073MIPI_write_cmos_sensor(0x3302, 0x0A);
    IMX073MIPI_write_cmos_sensor(0x3303, 0x09);
    IMX073MIPI_write_cmos_sensor(0x3304, 0x05);
    IMX073MIPI_write_cmos_sensor(0x3305, 0x04);
    IMX073MIPI_write_cmos_sensor(0x3306, 0x15);
    IMX073MIPI_write_cmos_sensor(0x3307, 0x03);
    IMX073MIPI_write_cmos_sensor(0x3308, 0x13);
    IMX073MIPI_write_cmos_sensor(0x3309, 0x05);
    IMX073MIPI_write_cmos_sensor(0x330A, 0x0B);
    IMX073MIPI_write_cmos_sensor(0x330B, 0x04);
    IMX073MIPI_write_cmos_sensor(0x330C, 0x0B);
    IMX073MIPI_write_cmos_sensor(0x330D, 0x06);
    // The register only need to enable 1 time.
    IMX073MIPI_write_cmos_sensor(0x0100, 0x01);   // Streaming 
    spin_lock(&imx073_drv_lock);
    IMX073MIPI_Auto_Flicker_mode = KAL_FALSE;     // reset the flicker status
	spin_unlock(&imx073_drv_lock);
    printk("[IMX073MIPIRaw] Init Success \n");
}   /*  IMX073MIPI_Sensor_Init  */

void IMX073MIPI_set_720P(void)
{	    
	spin_lock(&imx073_drv_lock);
    IMX073MIPI_g_RES = IMX073MIPI_720P;
    spin_unlock(&imx073_drv_lock);
#if 0  // 1 lane  820x629 V/4 H/4
    IMX073MIPI_write_cmos_sensor(0x3014, 0x08);
    IMX073MIPI_write_cmos_sensor(0x3015, 0x37);    
    IMX073MIPI_write_cmos_sensor(0x0344, 0x00);
    IMX073MIPI_write_cmos_sensor(0x0345, 0x00);
    IMX073MIPI_write_cmos_sensor(0x0346, 0x00);
    IMX073MIPI_write_cmos_sensor(0x0347, 0x00);	
    IMX073MIPI_write_cmos_sensor(0x0348, 0x0C);
    IMX073MIPI_write_cmos_sensor(0x0349, 0xCF);
    IMX073MIPI_write_cmos_sensor(0x034A, 0x09);
    IMX073MIPI_write_cmos_sensor(0x034B, 0x9F);
    
    IMX073MIPI_write_cmos_sensor(0x034C, 0x03);
    IMX073MIPI_write_cmos_sensor(0x034D, 0x34);
    IMX073MIPI_write_cmos_sensor(0x034E, 0x02);
    IMX073MIPI_write_cmos_sensor(0x034F, 0x68);    
    IMX073MIPI_write_cmos_sensor(0x0381, 0x05);
    IMX073MIPI_write_cmos_sensor(0x0383, 0x03);
    IMX073MIPI_write_cmos_sensor(0x0385, 0x05);
    IMX073MIPI_write_cmos_sensor(0x0387, 0x03);
    IMX073MIPI_write_cmos_sensor(0x3001, 0x00);
    IMX073MIPI_write_cmos_sensor(0x3016, 0x46);
    IMX073MIPI_write_cmos_sensor(0x30E8, 0x86);
    IMX073MIPI_write_cmos_sensor(0x3301, 0x01);    
#endif
#if 1   // 1 lane  1648x1250 V/2 H/2
    IMX073MIPI_write_cmos_sensor(0x0307, 0x1B);//INCK 24MHz
    IMX073MIPI_write_cmos_sensor(0x3014, 0x00);
    IMX073MIPI_write_cmos_sensor(0x3015, 0x37);
    IMX073MIPI_write_cmos_sensor(0x0344, 0x0F);
    IMX073MIPI_write_cmos_sensor(0x0345, 0xF0);
    IMX073MIPI_write_cmos_sensor(0x0346, 0x00);
    IMX073MIPI_write_cmos_sensor(0x0347, 0x00);
    IMX073MIPI_write_cmos_sensor(0x0348, 0x0C);
    IMX073MIPI_write_cmos_sensor(0x0349, 0xD3);
    IMX073MIPI_write_cmos_sensor(0x034A, 0x09);
    IMX073MIPI_write_cmos_sensor(0x034B, 0xA1);
    IMX073MIPI_write_cmos_sensor(0x034C, 0x06);
    IMX073MIPI_write_cmos_sensor(0x034D, 0x72);
    IMX073MIPI_write_cmos_sensor(0x034E, 0x04);
    IMX073MIPI_write_cmos_sensor(0x034F, 0xD1);    
    IMX073MIPI_write_cmos_sensor(0x0381, 0x01);
    IMX073MIPI_write_cmos_sensor(0x0383, 0x03);
    IMX073MIPI_write_cmos_sensor(0x0385, 0x01);
    IMX073MIPI_write_cmos_sensor(0x0387, 0x03);
    IMX073MIPI_write_cmos_sensor(0x3001, 0x80);
//    IMX073MIPI_write_cmos_sensor(0x3016, 0x46);
    IMX073MIPI_write_cmos_sensor(0x3016, 0x06);
    IMX073MIPI_write_cmos_sensor(0x30E8, 0x86);
    IMX073MIPI_write_cmos_sensor(0x3301, 0x01);
#endif
#if 0  // 2 lane 3280x626 V/4 H
    IMX073MIPI_write_cmos_sensor(0x3014, 0x00);
    IMX073MIPI_write_cmos_sensor(0x3015, 0x37);
    IMX073MIPI_write_cmos_sensor(0x0340, 0x04);
    IMX073MIPI_write_cmos_sensor(0x0341, 0x5C);
    IMX073MIPI_write_cmos_sensor(0x0344, 0x0F);
    IMX073MIPI_write_cmos_sensor(0x0345, 0xF0);
    IMX073MIPI_write_cmos_sensor(0x0346, 0x00);
    IMX073MIPI_write_cmos_sensor(0x0347, 0x00);
    IMX073MIPI_write_cmos_sensor(0x0348, 0x0C);
    IMX073MIPI_write_cmos_sensor(0x0349, 0xD3);
    IMX073MIPI_write_cmos_sensor(0x034A, 0x09);
    IMX073MIPI_write_cmos_sensor(0x034B, 0x9F);
    IMX073MIPI_write_cmos_sensor(0x034C, 0x0C);
    IMX073MIPI_write_cmos_sensor(0x034D, 0xE4);
    IMX073MIPI_write_cmos_sensor(0x034E, 0x02);
    IMX073MIPI_write_cmos_sensor(0x034F, 0x68);
    IMX073MIPI_write_cmos_sensor(0x0381, 0x01);
    IMX073MIPI_write_cmos_sensor(0x0383, 0x01);
    IMX073MIPI_write_cmos_sensor(0x0385, 0x05);
    IMX073MIPI_write_cmos_sensor(0x0387, 0x03);
    IMX073MIPI_write_cmos_sensor(0x3001, 0x00);
    IMX073MIPI_write_cmos_sensor(0x3016, 0x46);
    IMX073MIPI_write_cmos_sensor(0x30E8, 0x86);
    IMX073MIPI_write_cmos_sensor(0x3301, 0x00);
#endif 
#if 0	   // color bar pattern test
    IMX073MIPI_write_cmos_sensor(0x30D8, 0x10);  // color bar test pattern
    IMX073MIPI_write_cmos_sensor(0x0600, 0x00);  // color bar test pattern
    IMX073MIPI_write_cmos_sensor(0x0601, 0x02);  // color bar test pattern
#endif

    printk("[IMX073MIPIRaw] Set 720P End\n"); 
}

void IMX073MIPI_set_8M(void)
{	
    spin_lock(&imx073_drv_lock);

    IMX073MIPI_g_RES = IMX073MIPI_8M;
    spin_unlock(&imx073_drv_lock);

    //IMX073MIPI_write_cmos_sensor(0x0307, 0x18);//INCK 24MHz   576Mbps/lane
    IMX073MIPI_write_cmos_sensor(0x0307, 0x1b);
    
    IMX073MIPI_write_cmos_sensor(0x3014, 0x08);
    IMX073MIPI_write_cmos_sensor(0x3015, 0x37);
//    IMX073MIPI_write_cmos_sensor(0x0340, 0x09);
//    IMX073MIPI_write_cmos_sensor(0x0341, 0xCE);
    IMX073MIPI_write_cmos_sensor(0x0344, 0x0F);
    IMX073MIPI_write_cmos_sensor(0x0345, 0xF0);
    IMX073MIPI_write_cmos_sensor(0x0346, 0x00);
    IMX073MIPI_write_cmos_sensor(0x0347, 0x00);	
    IMX073MIPI_write_cmos_sensor(0x0348, 0x0C);
    IMX073MIPI_write_cmos_sensor(0x0349, 0xD3);
    IMX073MIPI_write_cmos_sensor(0x034A, 0x09);
    IMX073MIPI_write_cmos_sensor(0x034B, 0xA1);
    IMX073MIPI_write_cmos_sensor(0x034C, 0x0C);
    IMX073MIPI_write_cmos_sensor(0x034D, 0xE4);
    IMX073MIPI_write_cmos_sensor(0x034E, 0x09);
    IMX073MIPI_write_cmos_sensor(0x034F, 0xA2);
    IMX073MIPI_write_cmos_sensor(0x0381, 0x01);
    IMX073MIPI_write_cmos_sensor(0x0383, 0x01);
    IMX073MIPI_write_cmos_sensor(0x0385, 0x01);
    IMX073MIPI_write_cmos_sensor(0x0387, 0x01);
    IMX073MIPI_write_cmos_sensor(0x3001, 0x00);
    IMX073MIPI_write_cmos_sensor(0x3016, 0x06);
    IMX073MIPI_write_cmos_sensor(0x30E8, 0x06);
    IMX073MIPI_write_cmos_sensor(0x3301, 0x00);

    printk("[IMX073MIPIRaw] Set 8M End\n"); 
}



/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   IMX073MIPIOpen
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

UINT32 IMX073MIPIOpen(void)
{
    int  retry = 0; 
	kal_uint16 temp_data;

	temp_data = IMX073MIPI_read_cmos_sensor(0x0001);
    spin_lock(&imx073_drv_lock);
	IMX073MIPI_sensor_id = temp_data;
    spin_unlock(&imx073_drv_lock);
    // check if sensor ID correct
    retry = 3; 
    do {
		temp_data = IMX073MIPI_read_cmos_sensor(0x0001);
	    spin_lock(&imx073_drv_lock);
        IMX073MIPI_sensor_id = temp_data;        
	    spin_unlock(&imx073_drv_lock);
        if (IMX073MIPI_sensor_id == IMX073MIPI_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", IMX073MIPI_sensor_id); 
        retry--; 
    } while (retry > 0);
	
    SENSORDB("Read Sensor ID = 0x%04x\n", IMX073MIPI_sensor_id); 
	
    if (IMX073MIPI_sensor_id != IMX073MIPI_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;

    IMX073MIPI_Sensor_Init();

	temp_data = read_IMX073MIPI_gain();
    spin_lock(&imx073_drv_lock);
    IMX073MIPI_sensor_gain_base = temp_data;
    
    IMX073MIPI_g_iBackupExtraExp = 0;
    spin_unlock(&imx073_drv_lock);
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   IMX073MIPIGetSensorID
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
UINT32 IMX073MIPIGetSensorID(UINT32 *sensorID) 
{
    int  retry = 3; 

    // check if sensor ID correct
    do {
        *sensorID = IMX073MIPI_read_cmos_sensor(0x0001);         
        if (*sensorID == IMX073MIPI_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != IMX073MIPI_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   IMX073MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of IMX073MIPI to change exposure time.
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
void IMX073MIPI_SetShutter(kal_uint16 iShutter)
{
	unsigned long flags;
#if 0 
    if (iShutter < 4 )
        iShutter = 4;
#else 
    if (iShutter < 1)
        iShutter = 1; 
#endif     
     spin_lock_irqsave(&imx073_drv_lock,flags);
    IMX073MIPI_pv_exposure_lines = iShutter;
    spin_unlock_irqrestore(&imx073_drv_lock,flags);
    IMX073MIPI_write_shutter(iShutter);
}   /*  IMX073MIPI_SetShutter   */



/*************************************************************************
* FUNCTION
*   IMX073MIPI_read_shutter
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
UINT16 IMX073MIPI_read_shutter(void)
{
    return (UINT16)( (IMX073MIPI_read_cmos_sensor(0x3002)<<8) | IMX073MIPI_read_cmos_sensor(0x3003) );
}

/*************************************************************************
* FUNCTION
*   IMX073MIPI_night_mode
*
* DESCRIPTION
*   This function night mode of IMX073MIPI.
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
void IMX073MIPI_NightMode(kal_bool bEnable)
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
}/*	IMX073MIPI_NightMode */



/*************************************************************************
* FUNCTION
*   IMX073MIPIClose
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
UINT32 IMX073MIPIClose(void)
{
    //  CISModulePowerOn(FALSE);
    //s_porting
    //  DRV_I2CClose(IMX073MIPIhDrvI2C);
    //e_porting
    return ERROR_NONE;
}	/* IMX073MIPIClose() */

void IMX073MIPISetFlipMirror(kal_int32 imgMirror)
{
    kal_uint8  iTemp; 
	
    iTemp = IMX073MIPI_read_cmos_sensor(0x0101) & 0x03;	//Clear the mirror and flip bits.
    switch (imgMirror)
    {
        case IMAGE_NORMAL:
            IMX073MIPI_write_cmos_sensor(0x0101, 0x03);	//Set normal
            break;
        case IMAGE_V_MIRROR:
            IMX073MIPI_write_cmos_sensor(0x0101, iTemp | 0x01);	//Set flip
            break;
        case IMAGE_H_MIRROR:
            IMX073MIPI_write_cmos_sensor(0x0101, iTemp | 0x02);	//Set mirror
            break;
        case IMAGE_HV_MIRROR:
            IMX073MIPI_write_cmos_sensor(0x0101, 0x00);	//Set mirror and flip
            break;
    }
}


/*************************************************************************
* FUNCTION
*   IMX073MIPIPreview
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
UINT32 IMX073MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 iStartX = 0, iStartY = 0;

    spin_lock(&imx073_drv_lock);
    g_iIMX073MIPI_Mode = IMX073MIPI_MODE_PREVIEW;
    spin_unlock(&imx073_drv_lock);
    //if(IMX073MIPI_720P == IMX073MIPI_g_RES)
    {
        IMX073MIPI_set_720P();
    }

    spin_lock(&imx073_drv_lock);
    if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        IMX073MIPI_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {
        IMX073MIPI_MPEG4_encode_mode = KAL_FALSE;
    }
    spin_unlock(&imx073_drv_lock);

    iStartX += IMX073MIPI_IMAGE_SENSOR_PV_STARTX;
    iStartY += IMX073MIPI_IMAGE_SENSOR_PV_STARTY;

    //sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
    
    //IMX073MIPISetFlipMirror(sensor_config_data->SensorImageMirror); 
    spin_lock(&imx073_drv_lock);

    IMX073MIPI_dummy_pixels = 0;
    IMX073MIPI_dummy_lines = 0;
    IMX073MIPI_PV_dummy_pixels = IMX073MIPI_dummy_pixels;
    IMX073MIPI_PV_dummy_lines = IMX073MIPI_dummy_lines;
    spin_unlock(&imx073_drv_lock);

    IMX073MIPI_SetDummy(IMX073MIPI_dummy_pixels, IMX073MIPI_dummy_lines);

    memcpy(&IMX073MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    image_window->GrabStartX= iStartX;
    image_window->GrabStartY= iStartY;
    image_window->ExposureWindowWidth= IMX073MIPI_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
    image_window->ExposureWindowHeight= IMX073MIPI_IMAGE_SENSOR_PV_HEIGHT - 2*iStartY;
    SENSORDB("Preview resolution:%d %d %d %d\n", image_window->GrabStartX, image_window->GrabStartY, image_window->ExposureWindowWidth, image_window->ExposureWindowHeight); 
    return ERROR_NONE;
}	/* IMX073MIPIPreview() */

UINT32 IMX073MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint32 shutter=IMX073MIPI_pv_exposure_lines;
    kal_uint16 iStartX = 0, iStartY = 0;

    spin_lock(&imx073_drv_lock);
    g_iIMX073MIPI_Mode = IMX073MIPI_MODE_CAPTURE;
    IMX073MIPI_MPEG4_encode_mode = KAL_FALSE; 
    IMX073MIPI_Auto_Flicker_mode = KAL_FALSE;   
    spin_unlock(&imx073_drv_lock);
    
    SENSORDB("Preview Shutter = %d, Gain = %d\n", shutter, read_IMX073MIPI_gain());     

    if(sensor_config_data->EnableShutterTansfer==KAL_TRUE)
        shutter=sensor_config_data->CaptureShutter;

        spin_lock(&imx073_drv_lock);
        IMX073MIPI_dummy_pixels= 0;
        IMX073MIPI_dummy_lines = 0; 
	    spin_unlock(&imx073_drv_lock);
        IMX073MIPI_set_8M();
     
        IMX073MIPISetFlipMirror(sensor_config_data->SensorImageMirror); 
        

        iStartX = IMX073MIPI_IMAGE_SENSOR_CAP_STARTX;
        iStartY = IMX073MIPI_IMAGE_SENSOR_CAP_STARTY;

        image_window->GrabStartX=iStartX;
        image_window->GrabStartY=iStartY;
        image_window->ExposureWindowWidth=IMX073MIPI_IMAGE_SENSOR_FULL_WIDTH -2*iStartX;
        image_window->ExposureWindowHeight=IMX073MIPI_IMAGE_SENSOR_FULL_HEIGHT-2*iStartY;

    sensor_config_data->Lines = image_window->ExposureWindowHeight;


    IMX073MIPI_SetDummy(IMX073MIPI_dummy_pixels, IMX073MIPI_dummy_lines);

    SENSORDB("Capture Shutter = %d, Gain = %d\n", shutter, read_IMX073MIPI_gain());     

    memcpy(&IMX073MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}	/* IMX073MIPICapture() */

UINT32 IMX073MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    pSensorResolution->SensorPreviewWidth	= IMX073MIPI_REAL_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= IMX073MIPI_REAL_PV_HEIGHT;
    pSensorResolution->SensorFullWidth		= IMX073MIPI_REAL_CAP_WIDTH;
    pSensorResolution->SensorFullHeight		= IMX073MIPI_REAL_CAP_HEIGHT;
    pSensorResolution->SensorVideoWidth		= IMX073MIPI_REAL_CAP_WIDTH;
    pSensorResolution->SensorVideoHeight		= IMX073MIPI_REAL_CAP_HEIGHT;

//    SENSORDB("SensorPreviewWidth:  %d.\n", pSensorResolution->SensorPreviewWidth);     
//    SENSORDB("SensorPreviewHeight: %d.\n", pSensorResolution->SensorPreviewHeight);     
//    SENSORDB("SensorFullWidth:  %d.\n", pSensorResolution->SensorFullWidth);     
//    SENSORDB("SensorFullHeight: %d.\n", pSensorResolution->SensorFullHeight);     

    return ERROR_NONE;
}   /* IMX073MIPIGetResolution() */

UINT32 IMX073MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch(ScenarioId){
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
				pSensorInfo->SensorPreviewResolutionX=IMX073MIPI_REAL_CAP_WIDTH;
				pSensorInfo->SensorPreviewResolutionY=IMX073MIPI_REAL_CAP_HEIGHT;
			pSensorInfo->SensorCameraPreviewFrameRate=15;
			break;

		default:
        pSensorInfo->SensorPreviewResolutionX=IMX073MIPI_REAL_PV_WIDTH;
        pSensorInfo->SensorPreviewResolutionY=IMX073MIPI_REAL_PV_HEIGHT;
			pSensorInfo->SensorCameraPreviewFrameRate=30;
			break;
	}
	pSensorInfo->SensorPreviewResolutionX=IMX073MIPI_REAL_CAP_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=IMX073MIPI_REAL_CAP_HEIGHT;
   
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
 
    pSensorInfo->CaptureDelayFrame = 2; 
    pSensorInfo->PreviewDelayFrame = 1; 
    pSensorInfo->VideoDelayFrame = 5; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;      
    pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;	
	   
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        //case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        //case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX073MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX073MIPI_IMAGE_SENSOR_PV_STARTY;           		
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	     pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	     pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        //case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
				case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX073MIPI_IMAGE_SENSOR_CAP_STARTX;	//2*IMX073MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX073MIPI_IMAGE_SENSOR_CAP_STARTY;	//2*IMX073MIPI_IMAGE_SENSOR_PV_STARTY;          			
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
            pSensorInfo->SensorClockDividCount=	3;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 1; 
            pSensorInfo->SensorGrabStartY = 1;             
            break;
    }

    IMX073MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &IMX073MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* IMX073MIPIGetInfo() */


UINT32 IMX073MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
		CurrentScenarioId = ScenarioId;
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:

            IMX073MIPIPreview(pImageWindow, pSensorConfigData);
            break;
	    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:		
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:

	case MSDK_SCENARIO_ID_CAMERA_ZSD:
            IMX073MIPICapture(pImageWindow, pSensorConfigData);
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
} /* IMX073MIPIControl() */

UINT32 IMX073MIPISetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("[IMX073MIPISetVideoMode] frame rate = %d\n", u2FrameRate);
    spin_lock(&imx073_drv_lock);
    IMX073MIPI_MPEG4_encode_mode = KAL_TRUE; 
    spin_unlock(&imx073_drv_lock);
    if (u2FrameRate == 30) {
	    spin_lock(&imx073_drv_lock);
        IMX073MIPI_MAX_EXPOSURE_LINES = IMX073MIPI_PV_EXPOSURE_LIMITATION + IMX073MIPI_PV_PERIOD_EXTRA_LINE_NUMS; 
	    spin_unlock(&imx073_drv_lock);
        if(IMX073MIPI_pv_exposure_lines < (IMX073MIPI_PV_EXPOSURE_LIMITATION + IMX073MIPI_PV_PERIOD_EXTRA_LINE_NUMS - IMX073MIPI_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
        {
            IMX073MIPI_write_cmos_sensor(0x0104, 1);        
            IMX073MIPI_write_cmos_sensor(0x0340, (IMX073MIPI_MAX_EXPOSURE_LINES >>8) & 0xFF);
            IMX073MIPI_write_cmos_sensor(0x0341, IMX073MIPI_MAX_EXPOSURE_LINES & 0xFF);	
            IMX073MIPI_write_cmos_sensor(0x0104, 0);
        }
    } else if (u2FrameRate == 15) {
        spin_lock(&imx073_drv_lock);
        IMX073MIPI_MAX_EXPOSURE_LINES = (IMX073MIPI_PV_EXPOSURE_LIMITATION + IMX073MIPI_PV_PERIOD_EXTRA_LINE_NUMS) * 2;  
	    spin_unlock(&imx073_drv_lock);
        IMX073MIPI_write_cmos_sensor(0x0104, 1);        
        IMX073MIPI_write_cmos_sensor(0x0340, (IMX073MIPI_MAX_EXPOSURE_LINES >>8) & 0xFF);
        IMX073MIPI_write_cmos_sensor(0x0341, IMX073MIPI_MAX_EXPOSURE_LINES & 0xFF);	
        IMX073MIPI_write_cmos_sensor(0x0104, 0);
        //IMX073MIPI_MAX_EXPOSURE_LINES = IMX073MIPI_MAX_EXPOSURE_LINES - IMX073MIPI_SHUTTER_LINES_GAP;
    } else if ((u2FrameRate < 30) && (u2FrameRate > 5)) {
   	    spin_lock(&imx073_drv_lock);
        IMX073MIPI_MAX_EXPOSURE_LINES = (IMX073MIPI_PV_EXPOSURE_LIMITATION + IMX073MIPI_PV_PERIOD_EXTRA_LINE_NUMS) * 30/u2FrameRate;  
	    spin_unlock(&imx073_drv_lock);
        IMX073MIPI_write_cmos_sensor(0x0104, 1);        
        IMX073MIPI_write_cmos_sensor(0x0340, (IMX073MIPI_MAX_EXPOSURE_LINES >>8) & 0xFF);
        IMX073MIPI_write_cmos_sensor(0x0341, IMX073MIPI_MAX_EXPOSURE_LINES & 0xFF);	
        IMX073MIPI_write_cmos_sensor(0x0104, 0);
    } else if(u2FrameRate == 0) {
   	    spin_lock(&imx073_drv_lock);
        IMX073MIPI_MPEG4_encode_mode = KAL_FALSE; 
	    spin_unlock(&imx073_drv_lock);
        printk("disable video mode %d\n", u2FrameRate);  
    } else {
        printk("Wrong frame rate setting %d\n", u2FrameRate);
    }
    return TRUE;
}

UINT32 IMX073MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
kal_uint32 pv_max_frame_rate_lines = IMX073MIPI_MAX_EXPOSURE_LINES;

    SENSORDB("[IMX073MIPISetAutoFlickerMode] frame rate(10base) = %d %d\n", bEnable, u2FrameRate);
    if(bEnable) {   // enable auto flicker 
   	    spin_lock(&imx073_drv_lock);
        IMX073MIPI_Auto_Flicker_mode = KAL_TRUE; 
	    spin_unlock(&imx073_drv_lock);
        if(IMX073MIPI_MPEG4_encode_mode == KAL_TRUE) {    // in the video mode, reset the frame rate
            pv_max_frame_rate_lines = IMX073MIPI_MAX_EXPOSURE_LINES + (IMX073MIPI_MAX_EXPOSURE_LINES>>7);            
            IMX073MIPI_write_cmos_sensor(0x0104, 1);        
            IMX073MIPI_write_cmos_sensor(0x0340, (pv_max_frame_rate_lines >>8) & 0xFF);
            IMX073MIPI_write_cmos_sensor(0x0341, pv_max_frame_rate_lines & 0xFF);	
            IMX073MIPI_write_cmos_sensor(0x0104, 0);        	
        }
    } else {
   	    spin_lock(&imx073_drv_lock);
        IMX073MIPI_Auto_Flicker_mode = KAL_FALSE; 
	    spin_unlock(&imx073_drv_lock);
        if(IMX073MIPI_MPEG4_encode_mode == KAL_TRUE) {    // in the video mode, restore the frame rate
            IMX073MIPI_write_cmos_sensor(0x0104, 1);        
            IMX073MIPI_write_cmos_sensor(0x0340, (IMX073MIPI_MAX_EXPOSURE_LINES >>8) & 0xFF);
            IMX073MIPI_write_cmos_sensor(0x0341, IMX073MIPI_MAX_EXPOSURE_LINES & 0xFF);	
            IMX073MIPI_write_cmos_sensor(0x0104, 0);        	
        }
        printk("Disable Auto flicker\n");    
    }
    return TRUE;
}

UINT32 IMX073MIPISetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[IMX073MIPISetTestPatternMode] Test pattern enable:%d\n", bEnable);
    
    if(bEnable) {   // enable color bar   
        IMX073MIPI_write_cmos_sensor(0x30D8, 0x10);  // color bar test pattern
        IMX073MIPI_write_cmos_sensor(0x0600, 0x00);  // color bar test pattern
        IMX073MIPI_write_cmos_sensor(0x0601, 0x02);  // color bar test pattern 
    } else {
        IMX073MIPI_write_cmos_sensor(0x30D8, 0x00);  // disable color bar test pattern
    }
    return TRUE;
}
// config flashlight preview setting


UINT32 IMX073MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
			    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:		
        		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:					
		           *pFeatureReturnPara16++=IMX073MIPI_IMAGE_SENSOR_8M_PIXELS_LINE;  
		           *pFeatureReturnPara16=IMX073MIPI_FULL_PERIOD_LINE_NUMS+IMX073MIPI_dummy_lines;	
		           SENSORDB("Sensor period:%d %d\n", IMX073MIPI_FULL_PERIOD_PIXEL_NUMS+IMX073MIPI_dummy_pixels, IMX073MIPI_FULL_PERIOD_LINE_NUMS+IMX073MIPI_dummy_lines); 
		           *pFeatureParaLen=4;        				
        			break;

				case MSDK_SCENARIO_ID_CAMERA_PREVIEW:	
		            *pFeatureReturnPara16++=IMX073MIPI_IMAGE_SENSOR_720P_PIXELS_LINE;  
		            *pFeatureReturnPara16=IMX073MIPI_PV_PERIOD_LINE_NUMS+IMX073MIPI_dummy_lines;	
		            SENSORDB("Sensor period:%d %d\n", IMX073MIPI_PV_PERIOD_PIXEL_NUMS+IMX073MIPI_dummy_pixels, IMX073MIPI_PV_PERIOD_LINE_NUMS+IMX073MIPI_dummy_lines); 
		            *pFeatureParaLen=4;
					break;
        		default:	
		            *pFeatureReturnPara16++=IMX073MIPI_IMAGE_SENSOR_720P_PIXELS_LINE;  
		            *pFeatureReturnPara16=IMX073MIPI_PV_PERIOD_LINE_NUMS+IMX073MIPI_dummy_lines;	
		            SENSORDB("Sensor period:%d %d\n", IMX073MIPI_PV_PERIOD_PIXEL_NUMS+IMX073MIPI_dummy_pixels, IMX073MIPI_PV_PERIOD_LINE_NUMS+IMX073MIPI_dummy_lines); 
		            *pFeatureParaLen=4;
	            break;
          	}
          	break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        	switch(CurrentScenarioId)
        	{
        		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:		
        		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:

		           *pFeatureReturnPara32 = 64800000*2 ; // 57600000*2; //19500000;
		           *pFeatureParaLen=4;		         	
		       		break;
		        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					*pFeatureReturnPara32 = 64800000;//57600000; //19500000;
					*pFeatureParaLen=4;
					break;					
		    	default:
		           *pFeatureReturnPara32 = 64800000;//57600000; //19500000;
		           *pFeatureParaLen=4;
		           break;
		     }
		     break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            IMX073MIPI_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            IMX073MIPI_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            IMX073MIPI_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		    spin_lock(&imx073_drv_lock);
            IMX073MIPI_isp_master_clock=*pFeatureData32;
		    spin_unlock(&imx073_drv_lock);
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            IMX073MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = IMX073MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
                spin_lock(&imx073_drv_lock);
                IMX073MIPISensorCCT[i].Addr=*pFeatureData32++;
                IMX073MIPISensorCCT[i].Para=*pFeatureData32++;
			    spin_unlock(&imx073_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX073MIPISensorCCT[i].Addr;
                *pFeatureData32++=IMX073MIPISensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
           	    spin_lock(&imx073_drv_lock);
                IMX073MIPISensorReg[i].Addr=*pFeatureData32++;
                IMX073MIPISensorReg[i].Para=*pFeatureData32++;
			    spin_unlock(&imx073_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX073MIPISensorReg[i].Addr;
                *pFeatureData32++=IMX073MIPISensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=IMX073MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, IMX073MIPISensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, IMX073MIPISensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &IMX073MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            IMX073MIPI_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            IMX073MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=IMX073MIPI_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            IMX073MIPI_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            IMX073MIPI_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            IMX073MIPI_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 129;
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
            IMX073MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            IMX073MIPIGetSensorID(pFeatureReturnPara32); 
            break;             
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            IMX073MIPISetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));            
	        break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            IMX073MIPISetTestPatternMode((BOOL)*pFeatureData16);        	
            break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* IMX073MIPIFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncIMX073MIPI=
{
    IMX073MIPIOpen,
    IMX073MIPIGetInfo,
    IMX073MIPIGetResolution,
    IMX073MIPIFeatureControl,
    IMX073MIPIControl,
    IMX073MIPIClose
};

UINT32 IMX073_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncIMX073MIPI;

    return ERROR_NONE;
}   /* SensorInit() */

