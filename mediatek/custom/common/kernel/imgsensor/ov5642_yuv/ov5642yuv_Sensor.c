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
#define WINMO_USE           0

 #if WINMO_USE
#include <windows.h>
#include <memory.h>
#include <nkintr.h>
#include <ceddk.h>
#include <ceddk_exp.h>

#include "CameraCustomized.h"

#include "kal_release.h"
#include "i2c_exp.h"
#include "gpio_exp.h"
#include "msdk_exp.h"
#include "msdk_sensor_exp.h"
#include "msdk_isp_exp.h"
#include "base_regs.h"
#include "Sensor.h"
#include "camera_sensor_para.h"
#else 
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

#include "ov5642yuv_Sensor.h"
#include "ov5642yuv_Camera_Sensor_para.h"
#include "ov5642yuv_CameraCustomized.h"

#include "kd_camera_feature.h"
#endif 



/*
DBGPARAM dpCurSettings = {
    TEXT("Sensor"), {
        TEXT("Preview"),TEXT("Capture"),TEXT("Init"),TEXT("Error"),
        TEXT("Gain"),TEXT("Shutter"),TEXT("Undef"),TEXT("Undef"),
        TEXT("Undef"),TEXT("Undef"),TEXT("Undef"),TEXT("Undef"),
        TEXT("Undef"),TEXT("Undef"),TEXT("Undef"),TEXT("Undef")},
    0x00FF	// ZONE_INIT | ZONE_WARNING | ZONE_ERROR
};

*/
kal_bool  OV5642YUV_MPEG4_encode_mode = KAL_FALSE;
kal_uint16  OV5642YUV_sensor_gain_base=0x0;
/* MAX/MIN Explosure Lines Used By AE Algorithm */
kal_uint16 OV5642YUV_MAX_EXPOSURE_LINES = OV5642_PV_PERIOD_LINE_NUMS;//650;
kal_uint8  OV5642YUV_MIN_EXPOSURE_LINES = 2;
kal_uint32 OV5642YUV_isp_master_clock;
kal_uint16 OV5642YUV_CURRENT_FRAME_LINES = OV5642_PV_PERIOD_LINE_NUMS;//650;

static kal_uint16 OV5642YUV_dummy_pixels=0, OV5642YUV_dummy_lines=0;
kal_uint16 OV5642YUV_PV_dummy_pixels=0,OV5642YUV_PV_dummy_lines=0;

kal_uint8 OV5642YUV_sensor_write_I2C_address = OV5642_WRITE_ID;
kal_uint8 OV5642YUV_sensor_read_I2C_address = OV5642_READ_ID;

static kal_uint32 OV5642YUV_zoom_factor = 0; 

//add by lingnan for af status
static UINT8 STA_FOCUS = 0x8F; 


static kal_uint32 AF_XS = 80;// Jan.2011
static kal_uint32 AF_YS = 60;// Jan.2011

static UINT8 zone_x0 = 32;
static UINT8 zone_y0 = 22;
static UINT8 zone_x1 = 48;
static UINT8 zone_y1 = 38;

static kal_bool AF_INIT = FALSE;


#define LOG_TAG "[OV5642Yuv]"
#define SENSORDB(fmt, arg...) printk( LOG_TAG  fmt, ##arg)
#define RETAILMSG(x,...)
#define TEXT

kal_uint16 OV5642YUV_g_iDummyLines = 28; 

#if WINMO_USE
HANDLE OV5642YUVhDrvI2C;
I2C_TRANSACTION OV5642YUVI2CConfig;
#endif 

UINT8 OV5642YUVPixelClockDivider=0;
kal_uint32 OV5642YUV_sensor_pclk=52000000;//19500000;
kal_uint32 OV5642YUV_PV_pclk = 5525; 

kal_uint32 OV5642YUV_CAP_pclk = 6175;

kal_uint16 OV5642YUV_pv_exposure_lines=0x100,OV5642YUV_g_iBackupExtraExp = 0,OV5642YUV_extra_exposure_lines = 0;

kal_uint16 OV5642YUV_sensor_id=0;

MSDK_SENSOR_CONFIG_STRUCT OV5642YUVSensorConfigData;

kal_uint32 OV5642YUV_FAC_SENSOR_REG;
kal_uint16 OV5642YUV_sensor_flip_value;


/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT OV5642YUVSensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT OV5642YUVSensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/



////////////////////////////////////////////////////////////////
typedef enum
{
  OV5642_720P,       //1M 1280x960
  OV5642_5M,     //5M 2592x1944
} OV5642_RES_TYPE;
OV5642_RES_TYPE OV5642YUV_g_RES=OV5642_720P;

typedef enum
{
  OV5642_MODE_PREVIEW,  //1M  	1280x960
  OV5642_MODE_CAPTURE   //5M    2592x1944
} OV5642_MODE;
OV5642_MODE g_iOV5642YUV_Mode=OV5642_MODE_PREVIEW;


extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
extern int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId); 
#define OV5642YUV_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, OV5642_WRITE_ID)
#define OV5642YUV_burst_write_cmos_sensor(pData, bytes)  iBurstWriteReg(pData, bytes, OV5642_WRITE_ID)

#define PROFILE 1

#if PROFILE 
static struct timeval OV5642YUV_ktv1, OV5642YUV_ktv2; 
inline void OV5642YUV_imgSensorProfileStart(void)
{
    do_gettimeofday(&OV5642YUV_ktv1);    
}

inline void OV5642YUV_imgSensorProfileEnd(char *tag)
{
    unsigned long TimeIntervalUS;    
    do_gettimeofday(&OV5642YUV_ktv2);

    TimeIntervalUS = (OV5642YUV_ktv2.tv_sec - OV5642YUV_ktv1.tv_sec) * 1000000 + (OV5642YUV_ktv2.tv_usec - OV5642YUV_ktv1.tv_usec); 
    SENSORDB("[%s]Profile = %lu\n",tag, TimeIntervalUS);
}
#else 
inline static void OV5642YUV_imgSensorProfileStart() {}
inline static void OV5642YUV_imgSensorProfileEnd(char *tag) {}
#endif 

#if 0
OV5642YUV_write_cmos_sensor(addr, para) 
{
    u16 data = 0 ; 

    OV5642YUV_imgSensorProfileStart();
    iWriteReg((u16) addr , (u32) para , 1, OV5642_WRITE_ID);
    OV5642YUV_imgSensorProfileEnd("OV5642YUV_write_cmos_sensor");


    iReadReg((u16)addr, (u8*) &data, OV5642_WRITE_ID); 

    SENSORDB("Write addr = %x , wdata = %x,  rdata= %x\n", addr, para, data); 
}
#endif 

kal_uint16 OV5642YUV_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV5642_WRITE_ID);
    return get_byte;
}


#define Sleep(ms) mdelay(ms)

#if WINMO_USE
void OV5642YUV_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    OV5642YUVI2CConfig.operation=I2C_OP_WRITE;
    OV5642YUVI2CConfig.slaveAddr=OV5642YUV_sensor_write_I2C_address>>1;
    OV5642YUVI2CConfig.transfer_num=1;	/* TRANSAC_LEN */
    OV5642YUVI2CConfig.transfer_len=3;
    OV5642YUVI2CConfig.buffer[0]=(UINT8)(addr>>8);
    OV5642YUVI2CConfig.buffer[1]=(UINT8)(addr&0xFF);
    OV5642YUVI2CConfig.buffer[2]=(UINT8)para;
    DRV_I2CTransaction(OV5642YUVhDrvI2C, &OV5642YUVI2CConfig);
}    /* OV5642YUV_write_cmos_sensor() */
#endif 

#if WINMO_USE
kal_uint32 OV5642YUV_read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    
    OV5642YUVI2CConfig.operation=I2C_OP_WRITE;
    OV5642YUVI2CConfig.slaveAddr=OV5642YUV_sensor_write_I2C_address>>1;
    OV5642YUVI2CConfig.transfer_num=1;	/* TRANSAC_LEN */
    OV5642YUVI2CConfig.transfer_len=2;
    OV5642YUVI2CConfig.buffer[0]=(UINT8)(addr>>8);
    OV5642YUVI2CConfig.buffer[1]=(UINT8)(addr&0xFF);
    DRV_I2CTransaction(OV5642YUVhDrvI2C, &OV5642YUVI2CConfig);
    
    OV5642YUVI2CConfig.operation=I2C_OP_READ;
    OV5642YUVI2CConfig.slaveAddr=OV5642YUV_sensor_read_I2C_address>>1;
    OV5642YUVI2CConfig.transfer_num=1;	/* TRANSAC_LEN */
    OV5642YUVI2CConfig.transfer_len=1;
    DRV_I2CTransaction(OV5642YUVhDrvI2C, &OV5642YUVI2CConfig);
    get_byte=OV5642YUVI2CConfig.buffer[0];
    return get_byte;
}	/* OV5642YUV_read_cmos_sensor() */
#endif 

static atomic_t OV5642_SetShutter_Flag; 
static wait_queue_head_t OV5642_SetShutter_waitQueue;
void OV5642YUV_write_shutter(kal_uint16 shutter)
{
    kal_uint16 iExp = shutter;
    kal_uint16 OV5642_g_iExtra_ExpLines = 0 ;
//    kal_uint16 OV5642_g_bXGA_Mode = 0; 
    int timeOut = 0; 

    if (atomic_read(&OV5642_SetShutter_Flag) == 1) {
        timeOut = wait_event_interruptible_timeout(
            OV5642_SetShutter_waitQueue, atomic_read(&OV5642_SetShutter_Flag) == 0, 1 * HZ);        
        if (timeOut == 0) {
            SENSORDB("[OV5642YUV_SetGain] Set Gain Wait Queue time out \n"); 
            return; 
        }
    }    
    atomic_set(&OV5642_SetShutter_Flag, 1); 

    if (OV5642YUV_g_RES == OV5642_720P) {
        if (iExp <= OV5642_PV_EXPOSURE_LIMITATION + OV5642YUV_g_iDummyLines) {
            OV5642_g_iExtra_ExpLines = OV5642_PV_EXPOSURE_LIMITATION + OV5642YUV_g_iDummyLines;
        }else {
            OV5642_g_iExtra_ExpLines = iExp+28;//; - OV5642_PV_EXPOSURE_LIMITATION - OV5642YUV_g_iDummyLines;
           //iExp = OV5642_PV_EXPOSURE_LIMITATION + OV5642YUV_g_iDummyLines;
        }

    }else {
        if (iExp <= OV5642_FULL_EXPOSURE_LIMITATION + OV5642YUV_g_iDummyLines) {
            OV5642_g_iExtra_ExpLines = OV5642_FULL_EXPOSURE_LIMITATION+ OV5642YUV_g_iDummyLines;
        }else {
            OV5642_g_iExtra_ExpLines = iExp+28;// - OV5642_FULL_EXPOSURE_LIMITATION - OV5642YUV_g_iDummyLines;
           //iExp = OV5642_FULL_EXPOSURE_LIMITATION + OV5642YUV_g_iDummyLines;
        }
    }

    //! The exposure limit can not be the same as exp 
    if (OV5642_g_iExtra_ExpLines == iExp) {
          OV5642_g_iExtra_ExpLines += 28; 
    }
    SENSORDB("Set Extra-line = %d, iExp = %d \n", OV5642_g_iExtra_ExpLines, iExp);     

//    OV5642YUV_write_cmos_sensor(0x3212, 0x01); 
    if (OV5642YUV_MPEG4_encode_mode != TRUE) {
    OV5642YUV_write_cmos_sensor(0x350c, OV5642_g_iExtra_ExpLines >> 8);
    OV5642YUV_write_cmos_sensor(0x350d, OV5642_g_iExtra_ExpLines & 0x00FF);
    }
    
    OV5642YUV_write_cmos_sensor(0x3500, (iExp >> 12) & 0xFF);
    OV5642YUV_write_cmos_sensor(0x3501, (iExp >> 4 ) & 0xFF);
    OV5642YUV_write_cmos_sensor(0x3502, (iExp <<4 ) & 0xFF);
//    OV5642YUV_write_cmos_sensor(0x3212, 0x11); 
//    OV5642YUV_write_cmos_sensor(0x3212, 0xa1);     

    OV5642YUV_g_iBackupExtraExp = OV5642_g_iExtra_ExpLines;    
    atomic_set(&OV5642_SetShutter_Flag, 0);
    wake_up_interruptible(&OV5642_SetShutter_waitQueue);    
}   /* write_OV5642_shutter */

static kal_uint16 OV5642YUVReg2Gain(const kal_uint8 iReg)
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

static kal_uint8 OV5642YUVGain2Reg(const kal_uint16 iGain)
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
*    OV5642YUV_SetGain
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
//! Due to the OV5642 set gain will happen race condition. 
//! It need to use a critical section to protect it. 
static atomic_t OV5642_SetGain_Flag; 
static wait_queue_head_t OV5642_SetGain_waitQueue;
void OV5642YUV_SetGain(UINT16 iGain)
{
    kal_uint8 iReg;
    int timeOut = 0; 

    //OV5642YUV_imgSensorProfileStart();
    //SENSORDB("[OV5642YUV_SetGain] E Gain = %d \n", iGain); 
    if (atomic_read(&OV5642_SetGain_Flag) == 1) {
        timeOut = wait_event_interruptible_timeout(
            OV5642_SetGain_waitQueue, atomic_read(&OV5642_SetGain_Flag) == 0, 1 * HZ);        
        if (timeOut == 0) {
            SENSORDB("[OV5642YUV_SetGain] Set Gain Wait Queue time out \n"); 
            return; 
        }
    }    
    atomic_set(&OV5642_SetGain_Flag, 1); 
    iReg = OV5642YUVGain2Reg(iGain);
    SENSORDB("transfer gain 0x%x(%d) to 0x%x(%d)\n",iGain,iGain,iReg,iReg);
    //! For OV642 sensor, the set gain don't have double buffer,  
    //! it needs use group write to write sensor gain 
    OV5642YUV_write_cmos_sensor(0x3212, 0x00); 
    OV5642YUV_write_cmos_sensor(0x350B, (kal_uint32)iReg);
    OV5642YUV_write_cmos_sensor(0x3212, 0x10); 
    OV5642YUV_write_cmos_sensor(0x3212, 0xA0); 
    
    //OV5642YUV_imgSensorProfileEnd("OV5642YUV_SetGain"); 
    //SENSORDB("Gain = %x\n", iReg);

    /* Set extra shutter here because extra shutter active time same as gain  */
    /*if (OV5642YUV_g_iBackupExtraExp !=OV5642YUV_extra_exposure_lines)
    {
        OV5642YUV_write_cmos_sensor(0x350c, OV5642YUV_extra_exposure_lines >> 8);
        OV5642YUV_write_cmos_sensor(0x350d, OV5642YUV_extra_exposure_lines & 0x00FF);
        OV5642YUV_g_iBackupExtraExp = OV5642YUV_extra_exposure_lines;
    }
    */
    atomic_set(&OV5642_SetGain_Flag, 0);
    wake_up_interruptible(&OV5642_SetGain_waitQueue);
    //SENSORDB("[OV5642YUV_SetGain] X Gain = %d \n", iGain); 
}   /*  OV5642YUV_SetGain  */


/*************************************************************************
* FUNCTION
*    read_OV5642YUV_gain
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
kal_uint16 read_OV5642YUV_gain(void)
{

    kal_uint8 temp_gain;
    kal_uint16 gain;
    temp_gain = OV5642YUV_read_cmos_sensor(0x350B);
    gain = OV5642YUVReg2Gain(temp_gain);
    SENSORDB("0x350b=0x%x, transfer to gain=0x%x,%d\n",temp_gain,gain,gain);
    return gain;
}  /* read_OV5642YUV_gain */

void write_OV5642YUV_gain(kal_uint16 gain)
{
    OV5642YUV_SetGain(gain);
}
void OV5642YUV_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=OV5642YUVSensorReg[i].Addr; i++)
    {
        OV5642YUV_write_cmos_sensor(OV5642YUVSensorReg[i].Addr, OV5642YUVSensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV5642YUVSensorReg[i].Addr; i++)
    {
        OV5642YUV_write_cmos_sensor(OV5642YUVSensorReg[i].Addr, OV5642YUVSensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        OV5642YUV_write_cmos_sensor(OV5642YUVSensorCCT[i].Addr, OV5642YUVSensorCCT[i].Para);
    }
}


/*************************************************************************
* FUNCTION
*    OV5642YUV_sensor_to_camera_para
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
void OV5642YUV_sensor_to_camera_para(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=OV5642YUVSensorReg[i].Addr; i++)
    {
        OV5642YUVSensorReg[i].Para = OV5642YUV_read_cmos_sensor(OV5642YUVSensorReg[i].Addr);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=OV5642YUVSensorReg[i].Addr; i++)
    {
        OV5642YUVSensorReg[i].Para = OV5642YUV_read_cmos_sensor(OV5642YUVSensorReg[i].Addr);
    }
}


/*************************************************************************
* FUNCTION
*    OV5642YUV_get_sensor_group_count
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
kal_int32  OV5642YUV_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void OV5642YUV_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
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

void OV5642YUV_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
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

            temp_para=OV5642YUVSensorCCT[temp_addr].Para;

            temp_gain = OV5642YUVReg2Gain(temp_para);

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
                
                    //temp_reg=OV5642YUVSensorReg[CMMCLK_CURRENT_INDEX].Para;
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
                    info_ptr->ItemValue=OV5642YUV_MAX_EXPOSURE_LINES;
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

//void OV5642YUV_set_isp_driving_current(kal_uint8 current)
//{
//}

kal_bool OV5642YUV_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
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

            temp_para = OV5642YUVGain2Reg(ItemValue);


            OV5642YUVSensorCCT[temp_addr].Para = temp_para;
            OV5642YUV_write_cmos_sensor(OV5642YUVSensorCCT[temp_addr].Addr,temp_para);

            OV5642YUV_sensor_gain_base=read_OV5642YUV_gain();

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    if(ItemValue==2)
                    {
                        OV5642YUVSensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_2MA;
                        //OV5642YUV_set_isp_driving_current(ISP_DRIVING_2MA);
                    }
                    else if(ItemValue==3 || ItemValue==4)
                    {
                        OV5642YUVSensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_4MA;
                        //OV5642YUV_set_isp_driving_current(ISP_DRIVING_4MA);
                    }
                    else if(ItemValue==5 || ItemValue==6)
                    {
                        OV5642YUVSensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_6MA;
                        //OV5642YUV_set_isp_driving_current(ISP_DRIVING_6MA);
                    }
                    else
                    {
                        OV5642YUVSensorReg[CMMCLK_CURRENT_INDEX].Para = ISP_DRIVING_8MA;
                        //OV5642YUV_set_isp_driving_current(ISP_DRIVING_8MA);
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
                    OV5642YUV_FAC_SENSOR_REG=ItemValue;
                    break;
                case 1:
                    OV5642YUV_write_cmos_sensor(OV5642YUV_FAC_SENSOR_REG,ItemValue);
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

static void OV5642YUV_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{
    kal_uint16 PixelsOneline = OV5642_FULL_PERIOD_PIXEL_NUMS;
    if(OV5642_720P == OV5642YUV_g_RES)
    {
        PixelsOneline = (OV5642_PV_PERIOD_PIXEL_NUMS + iPixels + OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS);
        if(OV5642YUV_MPEG4_encode_mode == KAL_FALSE)//for Fix video framerate
            OV5642YUV_CURRENT_FRAME_LINES = iLines + OV5642_PV_PERIOD_LINE_NUMS + OV5642_PV_PERIOD_EXTRA_LINE_NUMS ;
    }
    else if (OV5642_5M == OV5642YUV_g_RES)
    {
        PixelsOneline = OV5642_FULL_PERIOD_PIXEL_NUMS + iPixels + OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS;
        OV5642YUV_CURRENT_FRAME_LINES = iLines + OV5642_FULL_PERIOD_LINE_NUMS + OV5642_FULL_PERIOD_EXTRA_LINE_NUMS;
    }
    OV5642YUV_write_cmos_sensor(0x380c, (PixelsOneline >> 8) & 0xFF);
    OV5642YUV_write_cmos_sensor(0x380d, PixelsOneline & 0xFF);

#if 0 //! Don't set shutter related register 
    if(OV5642YUV_MPEG4_encode_mode == KAL_FALSE)//for Fix video framerate,set the frame lines in night mode function
    {
        //Set dummy lines.
        //The maximum shutter value = Line_Without_Dummy + Dummy_lines
        OV5642YUV_write_cmos_sensor(0x350C, (OV5642YUV_CURRENT_FRAME_LINES >> 8) & 0xFF);
        OV5642YUV_write_cmos_sensor(0x350D, OV5642YUV_CURRENT_FRAME_LINES & 0xFF);
        OV5642YUV_MAX_EXPOSURE_LINES = OV5642YUV_CURRENT_FRAME_LINES - OV5642_SHUTTER_LINES_GAP;
    }
#endif     
}   /*  OV5642YUV_SetDummy */

static void OV5642YUV_set_AE_mode(kal_bool AE_enable)
{
    kal_uint8 temp_AE_reg = 0;

    if (AE_enable == KAL_TRUE)
    {
        // turn on AEC/AGC
        temp_AE_reg = OV5642YUV_read_cmos_sensor(0x3503);
        OV5642YUV_write_cmos_sensor(0x3503, temp_AE_reg&~0x07);
  
    }
    else
    {
        // turn off AEC/AGC
        temp_AE_reg = OV5642YUV_read_cmos_sensor(0x3503);
        OV5642YUV_write_cmos_sensor(0x3503, temp_AE_reg| 0x07);

    }
}


static void OV5642YUV_set_AWB_mode(kal_bool AWB_enable)
{
    kal_uint8 temp_AWB_reg = 0;

    //return ;

    if (AWB_enable == KAL_TRUE)
    {
        //enable Auto WB
        temp_AWB_reg = OV5642YUV_read_cmos_sensor(0x3406);
        OV5642YUV_write_cmos_sensor(0x3406, temp_AWB_reg & ~0x01);        
    }
    else
    {
        //turn off AWB
        temp_AWB_reg = OV5642YUV_read_cmos_sensor(0x3406);
        OV5642YUV_write_cmos_sensor(0x3406, temp_AWB_reg | 0x01);        
    }
}

static void OV5642_FOCUS_AD5820_Check_MCU()
{
    kal_uint8 check[13] = {0x00};
	//mcu on
    check[0] = OV5642YUV_read_cmos_sensor(0x3000);
    check[1] = OV5642YUV_read_cmos_sensor(0x3004);
	//soft reset of mcu
    check[2] = OV5642YUV_read_cmos_sensor(0x3f00);	
	//afc on
    check[3] = OV5642YUV_read_cmos_sensor(0x3001);
    check[4] = OV5642YUV_read_cmos_sensor(0x3005);
	//gpio1,gpio2
    check[5] = OV5642YUV_read_cmos_sensor(0x3018);
    check[6] = OV5642YUV_read_cmos_sensor(0x301e);
    check[7] = OV5642YUV_read_cmos_sensor(0x301b);
    check[8] = OV5642YUV_read_cmos_sensor(0x3042);
	//y0
    check[9] = OV5642YUV_read_cmos_sensor(0x3018);
    check[10] = OV5642YUV_read_cmos_sensor(0x301e);
    check[11] = OV5642YUV_read_cmos_sensor(0x301b);
    check[12] = OV5642YUV_read_cmos_sensor(0x3042);

    int i = 0;
    for(i = 0; i < 13; i++)
    SENSORDB("check[%d]=0x%x\n", i, check[i]);

	
}


void OV5642YUV_Sensor_Init_set_720P(void);

void OV5642YUV_set_5M_init(void);

static UINT32 OV5642_FOCUS_AD5820_Init(void);

/*******************************************************************************
*
********************************************************************************/
static UINT32 OV5642YUV_Sensor_Init(void)
{
    SENSORDB("lln:: OV5642YUV_Sensor_Init, use OV5642YUV_Sensor_Init_set_720P");
    UINT32 err;
    OV5642YUV_write_cmos_sensor(0x3103,0x93);
    OV5642YUV_write_cmos_sensor(0x3008,0x82);
    Sleep(2);		
    OV5642YUV_Sensor_Init_set_720P();
#if 0
    OV5642YUV_write_cmos_sensor(0x3103, 0x93);//PCLK SELECT
    OV5642YUV_write_cmos_sensor(0x3008, 0x82);//SYSTEM CONTROL SOFT RESET
    Sleep(2);
    
    OV5642YUV_write_cmos_sensor(0x3017, 0x7f);//PAD OUTPUT ENABLE
    OV5642YUV_write_cmos_sensor(0x3018, 0xfc);//PAD OUTPUT ENABLE
    OV5642YUV_write_cmos_sensor(0x302c, 0xC0); //IO Driving, 4x output Control ,
    //OV5642YUV_write_cmos_sensor(0x3810, 0xc2);//H and V offset setting
    OV5642YUV_write_cmos_sensor(0x3615, 0xf0);//
    OV5642YUV_write_cmos_sensor(0x3000, 0x00);
    OV5642YUV_write_cmos_sensor(0x3001, 0x00);
    OV5642YUV_write_cmos_sensor(0x3002, 0x5c);
    OV5642YUV_write_cmos_sensor(0x3003, 0x00);
    OV5642YUV_write_cmos_sensor(0x3004, 0xff);
    OV5642YUV_write_cmos_sensor(0x3005, 0xff);
    OV5642YUV_write_cmos_sensor(0x3006, 0x43);
    OV5642YUV_write_cmos_sensor(0x3007, 0x37);
    OV5642YUV_write_cmos_sensor(0x3011, 0x0a);

    OV5642YUV_write_cmos_sensor(0x3012, 0x02);
    
    OV5642YUV_write_cmos_sensor(0x3010, 0x00);

    OV5642YUV_write_cmos_sensor(0x360c, 0x20);

    OV5642YUV_write_cmos_sensor(0x3815, 0x04);

    OV5642YUV_write_cmos_sensor(0x370c, 0xa0);
    OV5642YUV_write_cmos_sensor(0x3602, 0xfc);
    OV5642YUV_write_cmos_sensor(0x3612, 0xff);

    OV5642YUV_write_cmos_sensor(0x3634, 0xc0);
    OV5642YUV_write_cmos_sensor(0x3613, 0x00);

    OV5642YUV_write_cmos_sensor(0x3605, 0x7c);
    OV5642YUV_write_cmos_sensor(0x3621, 0x09);
    OV5642YUV_write_cmos_sensor(0x3622, 0x60);

    OV5642YUV_write_cmos_sensor(0x3604, 0x40);
    OV5642YUV_write_cmos_sensor(0x3603, 0xa7);
    OV5642YUV_write_cmos_sensor(0x3603, 0x27);
    OV5642YUV_write_cmos_sensor(0x4000, 0x21);

    OV5642YUV_write_cmos_sensor(0x401d, 0x22);
    OV5642YUV_write_cmos_sensor(0x3600, 0x54);
    OV5642YUV_write_cmos_sensor(0x3605, 0x04);
    OV5642YUV_write_cmos_sensor(0x3606, 0x3f);
    OV5642YUV_write_cmos_sensor(0x3c01, 0x80);
    OV5642YUV_write_cmos_sensor(0x5000, 0x4f);
    OV5642YUV_write_cmos_sensor(0x5020, 0x04);
    OV5642YUV_write_cmos_sensor(0x5181, 0x79);

    OV5642YUV_write_cmos_sensor(0x5182, 0x00);
    OV5642YUV_write_cmos_sensor(0x5185, 0x22);
    OV5642YUV_write_cmos_sensor(0x5179, 0x01);
    OV5642YUV_write_cmos_sensor(0x5001, 0xff);
    OV5642YUV_write_cmos_sensor(0x5500, 0x0a);
    OV5642YUV_write_cmos_sensor(0x5504, 0x00);
    OV5642YUV_write_cmos_sensor(0x5505, 0x7f);
    OV5642YUV_write_cmos_sensor(0x5080, 0x08);
    OV5642YUV_write_cmos_sensor(0x300e, 0x18);
    OV5642YUV_write_cmos_sensor(0x4610, 0x00);
    OV5642YUV_write_cmos_sensor(0x471d, 0x05);

    OV5642YUV_write_cmos_sensor(0x4708, 0x06);
    OV5642YUV_write_cmos_sensor(0x3808, 0x05);
    OV5642YUV_write_cmos_sensor(0x3809, 0x10);
    OV5642YUV_write_cmos_sensor(0x380a, 0x03);
    OV5642YUV_write_cmos_sensor(0x380b, 0xcc);
    OV5642YUV_write_cmos_sensor(0x380e, 0x07);

    OV5642YUV_write_cmos_sensor(0x380f, 0xd0);
    OV5642YUV_write_cmos_sensor(0x501f, 0x00);
    OV5642YUV_write_cmos_sensor(0x5000, 0x4f);
    OV5642YUV_write_cmos_sensor(0x4300, 0x30);
    OV5642YUV_write_cmos_sensor(0x3503, 0x07);
    OV5642YUV_write_cmos_sensor(0x3501, 0x73);

    OV5642YUV_write_cmos_sensor(0x3502, 0x80);
    OV5642YUV_write_cmos_sensor(0x350b, 0x00);
    OV5642YUV_write_cmos_sensor(0x3503, 0x07);
    OV5642YUV_write_cmos_sensor(0x3824, 0x11);
    OV5642YUV_write_cmos_sensor(0x3825, 0xb0);
    OV5642YUV_write_cmos_sensor(0x3501, 0x1e);
    OV5642YUV_write_cmos_sensor(0x3502, 0x80);

    OV5642YUV_write_cmos_sensor(0x350b, 0x7f);
    OV5642YUV_write_cmos_sensor(0x380c, 0x07);
    OV5642YUV_write_cmos_sensor(0x380d, 0xc0);

    OV5642YUV_write_cmos_sensor(0x380e, 0x03);
    OV5642YUV_write_cmos_sensor(0x380f, 0xf0);
    OV5642YUV_write_cmos_sensor(0x3a0d, 0x04);
    OV5642YUV_write_cmos_sensor(0x3a0e, 0x03);
    OV5642YUV_write_cmos_sensor(0x3818, 0xc1);

    OV5642YUV_write_cmos_sensor(0x3705, 0xdb);
    OV5642YUV_write_cmos_sensor(0x370a, 0x81);
    OV5642YUV_write_cmos_sensor(0x3801, 0x80);

    OV5642YUV_write_cmos_sensor(0x3621, 0xc7);
    OV5642YUV_write_cmos_sensor(0x3801, 0x50);
    OV5642YUV_write_cmos_sensor(0x3803, 0x08);
    OV5642YUV_write_cmos_sensor(0x3827, 0x08);

    OV5642YUV_write_cmos_sensor(0x3810, 0x42);
    OV5642YUV_write_cmos_sensor(0x3804, 0x05);
    OV5642YUV_write_cmos_sensor(0x3805, 0x10);
    OV5642YUV_write_cmos_sensor(0x5682, 0x05);
    OV5642YUV_write_cmos_sensor(0x5683, 0x10);
    OV5642YUV_write_cmos_sensor(0x3806, 0x03);
    OV5642YUV_write_cmos_sensor(0x3807, 0xcc);
    OV5642YUV_write_cmos_sensor(0x5686, 0x03);
    OV5642YUV_write_cmos_sensor(0x5687, 0xc4);

    OV5642YUV_write_cmos_sensor(0x3a00, 0x78);
    OV5642YUV_write_cmos_sensor(0x3a1a, 0x05);
    OV5642YUV_write_cmos_sensor(0x3a13, 0x30);
    OV5642YUV_write_cmos_sensor(0x3a18, 0x00);
    OV5642YUV_write_cmos_sensor(0x3a19, 0x7c);
    OV5642YUV_write_cmos_sensor(0x3a08, 0x12);

    OV5642YUV_write_cmos_sensor(0x3a09, 0xc0);
    OV5642YUV_write_cmos_sensor(0x3a0a, 0x0f);
    OV5642YUV_write_cmos_sensor(0x3a0b, 0xa0);
    OV5642YUV_write_cmos_sensor(0x350c, 0x07);
    OV5642YUV_write_cmos_sensor(0x350d, 0xd0);
    OV5642YUV_write_cmos_sensor(0x3500, 0x00);
    OV5642YUV_write_cmos_sensor(0x3501, 0x00);
    OV5642YUV_write_cmos_sensor(0x3502, 0x00);

    OV5642YUV_write_cmos_sensor(0x350a, 0x00);
    OV5642YUV_write_cmos_sensor(0x350b, 0x00);
    //OV5642YUV_write_cmos_sensor(0x3503, 0x00);//disable AEC
    OV5642YUV_write_cmos_sensor(0x528a, 0x02);
    OV5642YUV_write_cmos_sensor(0x528b, 0x04);
    OV5642YUV_write_cmos_sensor(0x528c, 0x08);
    OV5642YUV_write_cmos_sensor(0x528d, 0x08);
    OV5642YUV_write_cmos_sensor(0x528e, 0x08);
    OV5642YUV_write_cmos_sensor(0x528f, 0x10);
    OV5642YUV_write_cmos_sensor(0x5290, 0x10);
    OV5642YUV_write_cmos_sensor(0x5292, 0x00);
    OV5642YUV_write_cmos_sensor(0x5293, 0x02);
    OV5642YUV_write_cmos_sensor(0x5294, 0x00);
    OV5642YUV_write_cmos_sensor(0x5295, 0x02);
    OV5642YUV_write_cmos_sensor(0x5296, 0x00);
    OV5642YUV_write_cmos_sensor(0x5297, 0x02);
    OV5642YUV_write_cmos_sensor(0x5298, 0x00);
    OV5642YUV_write_cmos_sensor(0x5299, 0x02);
    OV5642YUV_write_cmos_sensor(0x529a, 0x00);
    OV5642YUV_write_cmos_sensor(0x529b, 0x02);
    OV5642YUV_write_cmos_sensor(0x529c, 0x00);
    OV5642YUV_write_cmos_sensor(0x529d, 0x02);
    OV5642YUV_write_cmos_sensor(0x529e, 0x00);
    OV5642YUV_write_cmos_sensor(0x529f, 0x02);

    OV5642YUV_write_cmos_sensor(0x3a0f, 0x3c);
    OV5642YUV_write_cmos_sensor(0x3a10, 0x30);
    OV5642YUV_write_cmos_sensor(0x3a1b, 0x3c);
    OV5642YUV_write_cmos_sensor(0x3a1e, 0x30);
    OV5642YUV_write_cmos_sensor(0x3a11, 0x70);
    OV5642YUV_write_cmos_sensor(0x3a1f, 0x10);
    OV5642YUV_write_cmos_sensor(0x3030, 0x2b);

    OV5642YUV_write_cmos_sensor(0x3a02, 0x00);
    OV5642YUV_write_cmos_sensor(0x3a03, 0x7d);
    OV5642YUV_write_cmos_sensor(0x3a04, 0x00);
    OV5642YUV_write_cmos_sensor(0x3a15, 0x7d);
    OV5642YUV_write_cmos_sensor(0x3a16, 0x00);

    OV5642YUV_write_cmos_sensor(0x3a02, 0x00);
    OV5642YUV_write_cmos_sensor(0x3a03, 0x7d);
    OV5642YUV_write_cmos_sensor(0x3a04, 0x00);
    OV5642YUV_write_cmos_sensor(0x3a15, 0x7d);
    OV5642YUV_write_cmos_sensor(0x3a16, 0x00);

    OV5642YUV_write_cmos_sensor(0x3a00, 0x78);
    OV5642YUV_write_cmos_sensor(0x3a08, 0x12);
    OV5642YUV_write_cmos_sensor(0x3a09, 0xc0);
    OV5642YUV_write_cmos_sensor(0x3a0a, 0x0f);
    OV5642YUV_write_cmos_sensor(0x3a0b, 0xa0);

    OV5642YUV_write_cmos_sensor(0x3a0d, 0x04);
    OV5642YUV_write_cmos_sensor(0x3a0e, 0x03);
    OV5642YUV_write_cmos_sensor(0x5193, 0x70);
    OV5642YUV_write_cmos_sensor(0x589b, 0x04);
    OV5642YUV_write_cmos_sensor(0x589a, 0xc5);

    OV5642YUV_write_cmos_sensor(0x4001, 0x42);
    OV5642YUV_write_cmos_sensor(0x401c, 0x04);
    OV5642YUV_write_cmos_sensor(0x528a, 0x01);
    OV5642YUV_write_cmos_sensor(0x528b, 0x04);
    OV5642YUV_write_cmos_sensor(0x528c, 0x08);

    OV5642YUV_write_cmos_sensor(0x528d, 0x10);
    OV5642YUV_write_cmos_sensor(0x528e, 0x20);
    OV5642YUV_write_cmos_sensor(0x528f, 0x28);
    OV5642YUV_write_cmos_sensor(0x5290, 0x30);
    OV5642YUV_write_cmos_sensor(0x5292, 0x00);

    OV5642YUV_write_cmos_sensor(0x5293, 0x01);
    OV5642YUV_write_cmos_sensor(0x5294, 0x00);
    OV5642YUV_write_cmos_sensor(0x5295, 0x04);
    OV5642YUV_write_cmos_sensor(0x5296, 0x00);
    OV5642YUV_write_cmos_sensor(0x5297, 0x08);

    OV5642YUV_write_cmos_sensor(0x5298, 0x00);
    OV5642YUV_write_cmos_sensor(0x5299, 0x10);
    OV5642YUV_write_cmos_sensor(0x529a, 0x00);
    OV5642YUV_write_cmos_sensor(0x529b, 0x20);
    OV5642YUV_write_cmos_sensor(0x529c, 0x00);

    OV5642YUV_write_cmos_sensor(0x529d, 0x28);
    OV5642YUV_write_cmos_sensor(0x529e, 0x00);
    OV5642YUV_write_cmos_sensor(0x529f, 0x30);
    OV5642YUV_write_cmos_sensor(0x5282, 0x00);
    OV5642YUV_write_cmos_sensor(0x5300, 0x00);

    OV5642YUV_write_cmos_sensor(0x5301, 0x20);
    OV5642YUV_write_cmos_sensor(0x5302, 0x00);
    OV5642YUV_write_cmos_sensor(0x5303, 0x7c);
    OV5642YUV_write_cmos_sensor(0x530c, 0x00);
    OV5642YUV_write_cmos_sensor(0x530d, 0x0c);

    OV5642YUV_write_cmos_sensor(0x530e, 0x20);
    OV5642YUV_write_cmos_sensor(0x530f, 0x80);
    OV5642YUV_write_cmos_sensor(0x5310, 0x20);
    OV5642YUV_write_cmos_sensor(0x5311, 0x80);
    OV5642YUV_write_cmos_sensor(0x5308, 0x20);

    OV5642YUV_write_cmos_sensor(0x5309, 0x40);
    OV5642YUV_write_cmos_sensor(0x5304, 0x00);
    OV5642YUV_write_cmos_sensor(0x5305, 0x30);
    OV5642YUV_write_cmos_sensor(0x5306, 0x00);
    OV5642YUV_write_cmos_sensor(0x5307, 0x80);

    OV5642YUV_write_cmos_sensor(0x5314, 0x08);
    OV5642YUV_write_cmos_sensor(0x5315, 0x20);
    OV5642YUV_write_cmos_sensor(0x5319, 0x30);
    OV5642YUV_write_cmos_sensor(0x5316, 0x10);
    OV5642YUV_write_cmos_sensor(0x5317, 0x08);
    OV5642YUV_write_cmos_sensor(0x5318, 0x02);
    OV5642YUV_write_cmos_sensor(0x5380, 0x01);  
    OV5642YUV_write_cmos_sensor(0x5381, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5382, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5383, 0x4e);  
    OV5642YUV_write_cmos_sensor(0x5384, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5385, 0x0f);  
    OV5642YUV_write_cmos_sensor(0x5386, 0x00);
    OV5642YUV_write_cmos_sensor(0x5387, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5388, 0x01);  
    OV5642YUV_write_cmos_sensor(0x5389, 0x15);  
    OV5642YUV_write_cmos_sensor(0x538a, 0x00);
    OV5642YUV_write_cmos_sensor(0x538b, 0x31);  
    OV5642YUV_write_cmos_sensor(0x538c, 0x00);  
    OV5642YUV_write_cmos_sensor(0x538d, 0x00);
    OV5642YUV_write_cmos_sensor(0x538e, 0x00);  
    OV5642YUV_write_cmos_sensor(0x538f, 0x0f);  
    OV5642YUV_write_cmos_sensor(0x5390, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5391, 0xab);  
    OV5642YUV_write_cmos_sensor(0x5392, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5393, 0xa2);  
    OV5642YUV_write_cmos_sensor(0x5394, 0x08);  
	
    OV5642YUV_write_cmos_sensor(0x5480, 0x14);  
    OV5642YUV_write_cmos_sensor(0x5481, 0x21);  
    OV5642YUV_write_cmos_sensor(0x5482, 0x36);  
    OV5642YUV_write_cmos_sensor(0x5483, 0x57);  
    OV5642YUV_write_cmos_sensor(0x5484, 0x65);  
    OV5642YUV_write_cmos_sensor(0x5485, 0x71);  
    OV5642YUV_write_cmos_sensor(0x5486, 0x7d);  
    OV5642YUV_write_cmos_sensor(0x5487, 0x87);  
    OV5642YUV_write_cmos_sensor(0x5488, 0x91);  
    OV5642YUV_write_cmos_sensor(0x5489, 0x9a);
    OV5642YUV_write_cmos_sensor(0x548a, 0xaa);
    OV5642YUV_write_cmos_sensor(0x548b, 0xb8);  
    OV5642YUV_write_cmos_sensor(0x548c, 0xcd);  
    OV5642YUV_write_cmos_sensor(0x548d, 0xdd);  
    OV5642YUV_write_cmos_sensor(0x548e, 0xea);  
    OV5642YUV_write_cmos_sensor(0x548f, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5490, 0x05);  
    OV5642YUV_write_cmos_sensor(0x5491, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5492, 0x04);  
    OV5642YUV_write_cmos_sensor(0x5493, 0x20);  
    OV5642YUV_write_cmos_sensor(0x5494, 0x03);  
    OV5642YUV_write_cmos_sensor(0x5495, 0x60);  
    OV5642YUV_write_cmos_sensor(0x5496, 0x02);  
    OV5642YUV_write_cmos_sensor(0x5497, 0xb8);  
    OV5642YUV_write_cmos_sensor(0x5498, 0x02);  
    OV5642YUV_write_cmos_sensor(0x5499, 0x86);  
    OV5642YUV_write_cmos_sensor(0x549a, 0x02);  
    OV5642YUV_write_cmos_sensor(0x549b, 0x5b);
    OV5642YUV_write_cmos_sensor(0x549c, 0x02);  
    OV5642YUV_write_cmos_sensor(0x549d, 0x3b);
    OV5642YUV_write_cmos_sensor(0x549e, 0x02);  
    OV5642YUV_write_cmos_sensor(0x549f, 0x1c);  
    OV5642YUV_write_cmos_sensor(0x54a0, 0x02);  
    OV5642YUV_write_cmos_sensor(0x54a1, 0x04);  
    OV5642YUV_write_cmos_sensor(0x54a2, 0x01);  
    OV5642YUV_write_cmos_sensor(0x54a3, 0xed);
    OV5642YUV_write_cmos_sensor(0x54a4, 0x01);  
    OV5642YUV_write_cmos_sensor(0x54a5, 0xc5);  
    OV5642YUV_write_cmos_sensor(0x54a6, 0x01);  
    OV5642YUV_write_cmos_sensor(0x54a7, 0xa5);  
    OV5642YUV_write_cmos_sensor(0x54a8, 0x01);  
    OV5642YUV_write_cmos_sensor(0x54a9, 0x6c);  
    OV5642YUV_write_cmos_sensor(0x54aa, 0x01);  
    OV5642YUV_write_cmos_sensor(0x54ab, 0x41);  
    OV5642YUV_write_cmos_sensor(0x54ac, 0x01);  
    OV5642YUV_write_cmos_sensor(0x54ad, 0x20);  
    OV5642YUV_write_cmos_sensor(0x54ae, 0x00);  
    OV5642YUV_write_cmos_sensor(0x54af, 0x16);  
    OV5642YUV_write_cmos_sensor(0x3406, 0x01);  //00
    
    OV5642YUV_write_cmos_sensor(0x5192, 0x04);  
    OV5642YUV_write_cmos_sensor(0x5191, 0xf8);  
    OV5642YUV_write_cmos_sensor(0x5193, 0x70);  
    OV5642YUV_write_cmos_sensor(0x5194, 0xf0);  
    OV5642YUV_write_cmos_sensor(0x5195, 0xf0);  
    OV5642YUV_write_cmos_sensor(0x518d, 0x3d);  
    OV5642YUV_write_cmos_sensor(0x518f, 0x54);  
    OV5642YUV_write_cmos_sensor(0x518e, 0x3d);  
    OV5642YUV_write_cmos_sensor(0x5190, 0x54);  
    OV5642YUV_write_cmos_sensor(0x518b, 0xa8);  
    OV5642YUV_write_cmos_sensor(0x518c, 0xa8);
    OV5642YUV_write_cmos_sensor(0x5187, 0x18);  
    OV5642YUV_write_cmos_sensor(0x5188, 0x18);  
    OV5642YUV_write_cmos_sensor(0x5189, 0x6e);  
    OV5642YUV_write_cmos_sensor(0x518a, 0x68);  
    OV5642YUV_write_cmos_sensor(0x5186, 0x1c);  
    OV5642YUV_write_cmos_sensor(0x5181, 0x50);  
    OV5642YUV_write_cmos_sensor(0x5184, 0x25);  
    OV5642YUV_write_cmos_sensor(0x5182, 0x11);  
    OV5642YUV_write_cmos_sensor(0x5183, 0x14);  
    OV5642YUV_write_cmos_sensor(0x5184, 0x25);  
    OV5642YUV_write_cmos_sensor(0x5185, 0x24);  
	
    OV5642YUV_write_cmos_sensor(0x5025, 0x80); //RAW 
    OV5642YUV_write_cmos_sensor(0x3a0f, 0x7e);  
    OV5642YUV_write_cmos_sensor(0x3a10, 0x72);  
    OV5642YUV_write_cmos_sensor(0x3a1b, 0x80);
    OV5642YUV_write_cmos_sensor(0x3a1e, 0x70);  
    OV5642YUV_write_cmos_sensor(0x3a11, 0xd0);  
    OV5642YUV_write_cmos_sensor(0x3a1f, 0x40);  
	
    OV5642YUV_write_cmos_sensor(0x5583, 0x40);  
    OV5642YUV_write_cmos_sensor(0x5584, 0x40);  
    OV5642YUV_write_cmos_sensor(0x5580, 0x02);  
    OV5642YUV_write_cmos_sensor(0x5000, 0xcf);  
    OV5642YUV_write_cmos_sensor(0x5800, 0x27);  
    OV5642YUV_write_cmos_sensor(0x5801, 0x19);  
    OV5642YUV_write_cmos_sensor(0x5802, 0x12);  
    OV5642YUV_write_cmos_sensor(0x5803, 0x0f);  
    OV5642YUV_write_cmos_sensor(0x5804, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5805, 0x15);  
    OV5642YUV_write_cmos_sensor(0x5806, 0x1e);  
    OV5642YUV_write_cmos_sensor(0x5807, 0x2f);  
    OV5642YUV_write_cmos_sensor(0x5808, 0x15);  
    OV5642YUV_write_cmos_sensor(0x5809, 0x0d);  
    OV5642YUV_write_cmos_sensor(0x580a, 0x0a);  
    OV5642YUV_write_cmos_sensor(0x580b, 0x09);
    OV5642YUV_write_cmos_sensor(0x580c, 0x0a);  
    OV5642YUV_write_cmos_sensor(0x580d, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x580e, 0x12);  
    OV5642YUV_write_cmos_sensor(0x580f, 0x19);  
    OV5642YUV_write_cmos_sensor(0x5810, 0x0b);  
    OV5642YUV_write_cmos_sensor(0x5811, 0x07);  
    OV5642YUV_write_cmos_sensor(0x5812, 0x04);  
    OV5642YUV_write_cmos_sensor(0x5813, 0x03);  
    OV5642YUV_write_cmos_sensor(0x5814, 0x03);  
    OV5642YUV_write_cmos_sensor(0x5815, 0x06);  
    OV5642YUV_write_cmos_sensor(0x5816, 0x0a);  
    OV5642YUV_write_cmos_sensor(0x5817, 0x0f);  
    OV5642YUV_write_cmos_sensor(0x5818, 0x0a);  
    OV5642YUV_write_cmos_sensor(0x5819, 0x05);  
    OV5642YUV_write_cmos_sensor(0x581a, 0x01);  
    OV5642YUV_write_cmos_sensor(0x581b, 0x00);  
    OV5642YUV_write_cmos_sensor(0x581c, 0x00);
    OV5642YUV_write_cmos_sensor(0x581d, 0x03);
    OV5642YUV_write_cmos_sensor(0x581e, 0x08);  
    OV5642YUV_write_cmos_sensor(0x581f, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x5820, 0x0a);  
    OV5642YUV_write_cmos_sensor(0x5821, 0x05);  
    OV5642YUV_write_cmos_sensor(0x5822, 0x01);  
    OV5642YUV_write_cmos_sensor(0x5823, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5824, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5825, 0x03);  
    OV5642YUV_write_cmos_sensor(0x5826, 0x08);  
    OV5642YUV_write_cmos_sensor(0x5827, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x5828, 0x0e);
    OV5642YUV_write_cmos_sensor(0x5829, 0x08);  
    OV5642YUV_write_cmos_sensor(0x582a, 0x06);  
    OV5642YUV_write_cmos_sensor(0x582b, 0x04);  
    OV5642YUV_write_cmos_sensor(0x582c, 0x05);  
    OV5642YUV_write_cmos_sensor(0x582d, 0x07);  
    OV5642YUV_write_cmos_sensor(0x582e, 0x0b);  
    OV5642YUV_write_cmos_sensor(0x582f, 0x12);  
    OV5642YUV_write_cmos_sensor(0x5830, 0x18);  
    OV5642YUV_write_cmos_sensor(0x5831, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5832, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x5833, 0x0a);  
    OV5642YUV_write_cmos_sensor(0x5834, 0x0b);  
    OV5642YUV_write_cmos_sensor(0x5835, 0x0e);  
    OV5642YUV_write_cmos_sensor(0x5836, 0x15);  
    OV5642YUV_write_cmos_sensor(0x5837, 0x19);  
    OV5642YUV_write_cmos_sensor(0x5838, 0x32);  
    OV5642YUV_write_cmos_sensor(0x5839, 0x1f);  
    OV5642YUV_write_cmos_sensor(0x583a, 0x18);  
    OV5642YUV_write_cmos_sensor(0x583b, 0x16);  
    OV5642YUV_write_cmos_sensor(0x583c, 0x17);
    OV5642YUV_write_cmos_sensor(0x583d, 0x1e);  
    OV5642YUV_write_cmos_sensor(0x583e, 0x26);  
    OV5642YUV_write_cmos_sensor(0x583f, 0x53);  
    OV5642YUV_write_cmos_sensor(0x5840, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5841, 0x0f);  
    OV5642YUV_write_cmos_sensor(0x5842, 0x0d);  
    OV5642YUV_write_cmos_sensor(0x5843, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x5844, 0x0e);  
    OV5642YUV_write_cmos_sensor(0x5845, 0x09);  
    OV5642YUV_write_cmos_sensor(0x5846, 0x11);  
    OV5642YUV_write_cmos_sensor(0x5847, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5848, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5849, 0x10);  
    OV5642YUV_write_cmos_sensor(0x584a, 0x10);  
    OV5642YUV_write_cmos_sensor(0x584b, 0x0e);  
    OV5642YUV_write_cmos_sensor(0x584c, 0x10);  
    OV5642YUV_write_cmos_sensor(0x584d, 0x10);  
    OV5642YUV_write_cmos_sensor(0x584e, 0x11);  
    OV5642YUV_write_cmos_sensor(0x584f, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5850, 0x0f);  
    OV5642YUV_write_cmos_sensor(0x5851, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x5852, 0x0f);  
    OV5642YUV_write_cmos_sensor(0x5853, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5854, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5855, 0x0f);  
    OV5642YUV_write_cmos_sensor(0x5856, 0x0e);  
    OV5642YUV_write_cmos_sensor(0x5857, 0x0b);  
    OV5642YUV_write_cmos_sensor(0x5858, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5859, 0x0d);  
    OV5642YUV_write_cmos_sensor(0x585a, 0x0d);  
    OV5642YUV_write_cmos_sensor(0x585b, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x585c, 0x0c);
    OV5642YUV_write_cmos_sensor(0x585d, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x585e, 0x0b);  
    OV5642YUV_write_cmos_sensor(0x585f, 0x0c);
    OV5642YUV_write_cmos_sensor(0x5860, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x5861, 0x0c);  
    OV5642YUV_write_cmos_sensor(0x5862, 0x0d);  
    OV5642YUV_write_cmos_sensor(0x5863, 0x08);  
    OV5642YUV_write_cmos_sensor(0x5864, 0x11);  
    OV5642YUV_write_cmos_sensor(0x5865, 0x18);  
    OV5642YUV_write_cmos_sensor(0x5866, 0x18);  
    OV5642YUV_write_cmos_sensor(0x5867, 0x19);  
    OV5642YUV_write_cmos_sensor(0x5868, 0x17);  
    OV5642YUV_write_cmos_sensor(0x5869, 0x19);  
    OV5642YUV_write_cmos_sensor(0x586a, 0x16);  
    OV5642YUV_write_cmos_sensor(0x586b, 0x13);  
    OV5642YUV_write_cmos_sensor(0x586c, 0x13);  
    OV5642YUV_write_cmos_sensor(0x586d, 0x12);  
    OV5642YUV_write_cmos_sensor(0x586e, 0x13);  
    OV5642YUV_write_cmos_sensor(0x586f, 0x16);  
    OV5642YUV_write_cmos_sensor(0x5870, 0x14);  
    OV5642YUV_write_cmos_sensor(0x5871, 0x12);  
    OV5642YUV_write_cmos_sensor(0x5872, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5873, 0x11);  
    OV5642YUV_write_cmos_sensor(0x5874, 0x11);  
    OV5642YUV_write_cmos_sensor(0x5875, 0x16);  
    OV5642YUV_write_cmos_sensor(0x5876, 0x14);  
    OV5642YUV_write_cmos_sensor(0x5877, 0x11);  
    OV5642YUV_write_cmos_sensor(0x5878, 0x10);  
    OV5642YUV_write_cmos_sensor(0x5879, 0x0f);  
    OV5642YUV_write_cmos_sensor(0x587a, 0x10);
    OV5642YUV_write_cmos_sensor(0x587b, 0x14);
    OV5642YUV_write_cmos_sensor(0x587c, 0x13);  
    OV5642YUV_write_cmos_sensor(0x587d, 0x12);
    OV5642YUV_write_cmos_sensor(0x587e, 0x11);  
    OV5642YUV_write_cmos_sensor(0x587f, 0x11);  
    OV5642YUV_write_cmos_sensor(0x5880, 0x12);  
    OV5642YUV_write_cmos_sensor(0x5881, 0x15);  
    OV5642YUV_write_cmos_sensor(0x5882, 0x14);  
    OV5642YUV_write_cmos_sensor(0x5883, 0x15);  
    OV5642YUV_write_cmos_sensor(0x5884, 0x15);  
    OV5642YUV_write_cmos_sensor(0x5885, 0x15);
    OV5642YUV_write_cmos_sensor(0x5886, 0x13);  
    OV5642YUV_write_cmos_sensor(0x5887, 0x17); 
	
    OV5642YUV_write_cmos_sensor(0x3710, 0x10);  
    OV5642YUV_write_cmos_sensor(0x3632, 0x51);  
    OV5642YUV_write_cmos_sensor(0x3702, 0x10);  
    OV5642YUV_write_cmos_sensor(0x3703, 0xb2);  
    OV5642YUV_write_cmos_sensor(0x3704, 0x18);  
    OV5642YUV_write_cmos_sensor(0x370b, 0x40);  
    OV5642YUV_write_cmos_sensor(0x370d, 0x03);  
    OV5642YUV_write_cmos_sensor(0x3631, 0x01);  
    OV5642YUV_write_cmos_sensor(0x3632, 0x52);  
    OV5642YUV_write_cmos_sensor(0x3606, 0x24);  
    OV5642YUV_write_cmos_sensor(0x3620, 0x96);  
    OV5642YUV_write_cmos_sensor(0x5785, 0x07);  
    OV5642YUV_write_cmos_sensor(0x3a13, 0x30);  
    OV5642YUV_write_cmos_sensor(0x3600, 0x52);  
    OV5642YUV_write_cmos_sensor(0x3604, 0x48);  
    OV5642YUV_write_cmos_sensor(0x3606, 0x1b);  
    OV5642YUV_write_cmos_sensor(0x370d, 0x0b);  
    OV5642YUV_write_cmos_sensor(0x370f, 0xc0);
    OV5642YUV_write_cmos_sensor(0x3709, 0x01);  
    OV5642YUV_write_cmos_sensor(0x3823, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5007, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5009, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5011, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5013, 0x00);
    OV5642YUV_write_cmos_sensor(0x519e, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5086, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5087, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5088, 0x00);  
    OV5642YUV_write_cmos_sensor(0x5089, 0x00);  
    OV5642YUV_write_cmos_sensor(0x302b, 0x00);  
    OV5642YUV_write_cmos_sensor(0x3621, 0x87);  
    OV5642YUV_write_cmos_sensor(0x3010, 0x10);  

    OV5642YUV_write_cmos_sensor(0x501f, 0x03);  
    OV5642YUV_write_cmos_sensor(0x4300, 0xf8);  	
    OV5642YUV_write_cmos_sensor(0x5000, 0x06);  

    OV5642YUV_write_cmos_sensor(0x5001, 0x01); //00 
    OV5642YUV_write_cmos_sensor(0x3c01, 0x80);  
    OV5642YUV_write_cmos_sensor(0x3c00, 0x04); 

    OV5642YUV_write_cmos_sensor(0x3503, 0x07);
    OV5642YUV_write_cmos_sensor(0x350c, 0x03);//
    OV5642YUV_write_cmos_sensor(0x350d, 0xf0);//
    OV5642YUV_write_cmos_sensor(0x3011, 0x08);//
    OV5642YUV_write_cmos_sensor(0x3010, 0x00);//

    //OV5642YUV_write_cmos_sensor(0x3011, 0x0f);//

    //OV5642YUV_write_cmos_sensor(0x3010, 0x10);//
    //OV5642YUV_write_cmos_sensor(0x3012, 0x02);//

    OV5642YUV_write_cmos_sensor(0x5001, 0x01); //00 awb enable
    OV5642YUV_write_cmos_sensor(0x3406, 0x01); //00 manul awb

    //PLL setting
    OV5642YUV_write_cmos_sensor(0x300f, 0x6);
    OV5642YUV_write_cmos_sensor(0x3010, 0x10);
    OV5642YUV_write_cmos_sensor(0x3011, 0x11);
    OV5642YUV_write_cmos_sensor(0x3012, 0x2);
    OV5642YUV_write_cmos_sensor(0x3815, 0x2);
    OV5642YUV_write_cmos_sensor(0x3029, 0x0);
    OV5642YUV_write_cmos_sensor(0x3033, 0x3);
    OV5642YUV_write_cmos_sensor(0x460c, 0x22);


#endif	
    SENSORDB("in the end download AF firmware\n");
    err = OV5642_FOCUS_AD5820_Init();
    SENSORDB("OV5642_FOCUS_AD5820_Init return %d \n", err);
    SENSORDB("Init Success \n");
    return err;
}   /*  OV5642YUV_Sensor_Init  */


void OV5642YUV_Sensor_Init_set_720P(void)
{

    OV5642YUV_write_cmos_sensor(0x3017,0x7f);
    OV5642YUV_write_cmos_sensor(0x3018,0xfc);
	
//add by lingnan   set driving current
    OV5642YUV_write_cmos_sensor(0x302c, 0xC0);
//
	
    OV5642YUV_write_cmos_sensor(0x3615,0xf0);
    OV5642YUV_write_cmos_sensor(0x3000,0x00);
    OV5642YUV_write_cmos_sensor(0x3001,0x00);
    OV5642YUV_write_cmos_sensor(0x3002,0x5c);
    OV5642YUV_write_cmos_sensor(0x3003,0x00);
    OV5642YUV_write_cmos_sensor(0x3004,0xff);
    OV5642YUV_write_cmos_sensor(0x3005,0xff);
    OV5642YUV_write_cmos_sensor(0x3006,0x43);
    OV5642YUV_write_cmos_sensor(0x3007,0x37);
    //OV5642YUV_write_cmos_sensor(0x3011,0x2f);

    OV5642YUV_write_cmos_sensor(0x3012,0x06);
    OV5642YUV_write_cmos_sensor(0x3010,0x10);

    OV5642YUV_write_cmos_sensor(0x3029, 0x0);
    OV5642YUV_write_cmos_sensor(0x3033, 0x3);

    OV5642YUV_write_cmos_sensor(0x3815,0x02);
    OV5642YUV_write_cmos_sensor(0x460c,0x20);
	
    OV5642YUV_write_cmos_sensor(0x370c,0xa0);
    OV5642YUV_write_cmos_sensor(0x3602,0xfc);
    OV5642YUV_write_cmos_sensor(0x3612,0xff);
    OV5642YUV_write_cmos_sensor(0x3634,0xc0);
    OV5642YUV_write_cmos_sensor(0x3613,0x00);
    OV5642YUV_write_cmos_sensor(0x3605,0x7c);
    OV5642YUV_write_cmos_sensor(0x3621,0x09);
    OV5642YUV_write_cmos_sensor(0x3622,0x60);
    OV5642YUV_write_cmos_sensor(0x3604,0x40);
    OV5642YUV_write_cmos_sensor(0x3603,0xa7);
    OV5642YUV_write_cmos_sensor(0x3603,0x27);
    OV5642YUV_write_cmos_sensor(0x4000,0x21);
    OV5642YUV_write_cmos_sensor(0x401d,0x22);
    OV5642YUV_write_cmos_sensor(0x3600,0x54);
    OV5642YUV_write_cmos_sensor(0x3605,0x04);
    OV5642YUV_write_cmos_sensor(0x3606,0x3f);
    OV5642YUV_write_cmos_sensor(0x3C00,0x04);
    OV5642YUV_write_cmos_sensor(0x3c01,0x80);
    OV5642YUV_write_cmos_sensor(0x5000,0x4f);
    OV5642YUV_write_cmos_sensor(0x5020,0x04);
    OV5642YUV_write_cmos_sensor(0x5181,0x79);
    OV5642YUV_write_cmos_sensor(0x5182,0x00);
    OV5642YUV_write_cmos_sensor(0x5185,0x22);
    OV5642YUV_write_cmos_sensor(0x5197,0x01);
    OV5642YUV_write_cmos_sensor(0x5001,0xff);
    OV5642YUV_write_cmos_sensor(0x5500,0x0a);
    OV5642YUV_write_cmos_sensor(0x5504,0x00);
    OV5642YUV_write_cmos_sensor(0x5505,0x7f);
    OV5642YUV_write_cmos_sensor(0x5080,0x08);
    OV5642YUV_write_cmos_sensor(0x300e,0x18);
    OV5642YUV_write_cmos_sensor(0x4610,0x00);
    OV5642YUV_write_cmos_sensor(0x471d,0x05);
    OV5642YUV_write_cmos_sensor(0x4708,0x06);
    OV5642YUV_write_cmos_sensor(0x3808,0x05);
    OV5642YUV_write_cmos_sensor(0x3809,0x00);
    OV5642YUV_write_cmos_sensor(0x380a,0x03);
    OV5642YUV_write_cmos_sensor(0x380b,0xc0);
    OV5642YUV_write_cmos_sensor(0x501f,0x00);
    OV5642YUV_write_cmos_sensor(0x5000,0x4f);
    OV5642YUV_write_cmos_sensor(0x4300,0x30);
    OV5642YUV_write_cmos_sensor(0x3503,0x07);
    OV5642YUV_write_cmos_sensor(0x3501,0x73);
    OV5642YUV_write_cmos_sensor(0x3502,0x80);
    OV5642YUV_write_cmos_sensor(0x350b,0x00);
    OV5642YUV_write_cmos_sensor(0x3503,0x07);
    OV5642YUV_write_cmos_sensor(0x3824,0x11);
    OV5642YUV_write_cmos_sensor(0x3825,0xb0);
    OV5642YUV_write_cmos_sensor(0x3501,0x1e);
    OV5642YUV_write_cmos_sensor(0x3502,0x80);
	
    OV5642YUV_write_cmos_sensor(0x350b,0x7f);
    OV5642YUV_write_cmos_sensor(0x380c,0x06);
    OV5642YUV_write_cmos_sensor(0x380d,0xca);
	
    OV5642YUV_write_cmos_sensor(0x380e,0x03);
    OV5642YUV_write_cmos_sensor(0x380f,0xd0);
    OV5642YUV_write_cmos_sensor(0x3a0d,0x04);
    OV5642YUV_write_cmos_sensor(0x3a0e,0x03);


    OV5642YUV_write_cmos_sensor(0x3818,0xc1);

	
    OV5642YUV_write_cmos_sensor(0x3705,0xdb);
    OV5642YUV_write_cmos_sensor(0x370a,0x81);
    OV5642YUV_write_cmos_sensor(0x3801,0x80);
    OV5642YUV_write_cmos_sensor(0x3621,0xc7);
    OV5642YUV_write_cmos_sensor(0x3801,0x50);
    OV5642YUV_write_cmos_sensor(0x3803,0x08);
    OV5642YUV_write_cmos_sensor(0x3827,0x08);
	
    OV5642YUV_write_cmos_sensor(0x3810,0x80);
    OV5642YUV_write_cmos_sensor(0x3804,0x05);
    OV5642YUV_write_cmos_sensor(0x3805,0x00);
    OV5642YUV_write_cmos_sensor(0x5682,0x05);
    OV5642YUV_write_cmos_sensor(0x5683,0x00);
    OV5642YUV_write_cmos_sensor(0x3806,0x03);
    OV5642YUV_write_cmos_sensor(0x3807,0xc0);
    OV5642YUV_write_cmos_sensor(0x5686,0x03);
    OV5642YUV_write_cmos_sensor(0x5687,0xbc);
	
    OV5642YUV_write_cmos_sensor(0x3a00,0x78);
    OV5642YUV_write_cmos_sensor(0x3a1a,0x05);
    OV5642YUV_write_cmos_sensor(0x3a13,0x30);
    OV5642YUV_write_cmos_sensor(0x3a18,0x00);
    OV5642YUV_write_cmos_sensor(0x3a19,0x7c);
    OV5642YUV_write_cmos_sensor(0x3a08,0x12);
    OV5642YUV_write_cmos_sensor(0x3a09,0x30);
    OV5642YUV_write_cmos_sensor(0x3a0a,0x0f);
    OV5642YUV_write_cmos_sensor(0x3a0b,0x30);
    OV5642YUV_write_cmos_sensor(0x350c,0x07);
    OV5642YUV_write_cmos_sensor(0x350d,0xd0);
    OV5642YUV_write_cmos_sensor(0x3500,0x00);
    OV5642YUV_write_cmos_sensor(0x3501,0x00);
    OV5642YUV_write_cmos_sensor(0x3502,0x00);
	
    OV5642YUV_write_cmos_sensor(0x350a,0x00);
    OV5642YUV_write_cmos_sensor(0x350b,0x00);

    OV5642YUV_write_cmos_sensor(0x3503,0x00);

	
    OV5642YUV_write_cmos_sensor(0x3a0f,0x3c);
    OV5642YUV_write_cmos_sensor(0x3a10,0x30);
    OV5642YUV_write_cmos_sensor(0x3a1b,0x3c);
    OV5642YUV_write_cmos_sensor(0x3a1e,0x30);
    OV5642YUV_write_cmos_sensor(0x3a11,0x70);
    OV5642YUV_write_cmos_sensor(0x3a1f,0x10);
    OV5642YUV_write_cmos_sensor(0x3030,0x2b);
	
    OV5642YUV_write_cmos_sensor(0x3a02,0x00);
    OV5642YUV_write_cmos_sensor(0x3a03,0x7d);
    OV5642YUV_write_cmos_sensor(0x3a04,0x00);

    OV5642YUV_write_cmos_sensor(0x3a14,0x00);

    OV5642YUV_write_cmos_sensor(0x3a15,0x7d);
    OV5642YUV_write_cmos_sensor(0x3a16,0x00);
	
    OV5642YUV_write_cmos_sensor(0x3a00,0x78);
    OV5642YUV_write_cmos_sensor(0x3a0d,0x04);
    OV5642YUV_write_cmos_sensor(0x3a0e,0x03);
    OV5642YUV_write_cmos_sensor(0x5193,0x70);
    OV5642YUV_write_cmos_sensor(0x589b,0x04);
    OV5642YUV_write_cmos_sensor(0x589a,0xc5);

    OV5642YUV_write_cmos_sensor(0x401e,0x20);
   
    OV5642YUV_write_cmos_sensor(0x4001,0x42);
    OV5642YUV_write_cmos_sensor(0x401c,0x04);

    OV5642YUV_write_cmos_sensor(0x528a,0x01);
    OV5642YUV_write_cmos_sensor(0x528b,0x04);
    OV5642YUV_write_cmos_sensor(0x528c,0x08);
    OV5642YUV_write_cmos_sensor(0x528d,0x10);
    OV5642YUV_write_cmos_sensor(0x528e,0x20);
    OV5642YUV_write_cmos_sensor(0x528f,0x28);
    OV5642YUV_write_cmos_sensor(0x5290,0x30);
    OV5642YUV_write_cmos_sensor(0x5292,0x00);
    OV5642YUV_write_cmos_sensor(0x5293,0x01);
    OV5642YUV_write_cmos_sensor(0x5294,0x00);
    OV5642YUV_write_cmos_sensor(0x5295,0x04);
    OV5642YUV_write_cmos_sensor(0x5296,0x00);
    OV5642YUV_write_cmos_sensor(0x5297,0x08);
    OV5642YUV_write_cmos_sensor(0x5298,0x00);
    OV5642YUV_write_cmos_sensor(0x5299,0x10);
    OV5642YUV_write_cmos_sensor(0x529a,0x00);
    OV5642YUV_write_cmos_sensor(0x529b,0x20);
    OV5642YUV_write_cmos_sensor(0x529c,0x00);
    OV5642YUV_write_cmos_sensor(0x529d,0x28);
    OV5642YUV_write_cmos_sensor(0x529e,0x00);
    OV5642YUV_write_cmos_sensor(0x529f,0x30);
	
    OV5642YUV_write_cmos_sensor(0x5282,0x00);
    OV5642YUV_write_cmos_sensor(0x5300,0x00);
    OV5642YUV_write_cmos_sensor(0x5301,0x20);
    OV5642YUV_write_cmos_sensor(0x5302,0x00);
    OV5642YUV_write_cmos_sensor(0x5303,0x7c);
    OV5642YUV_write_cmos_sensor(0x530c,0x00);
    OV5642YUV_write_cmos_sensor(0x530d,0x0c);
    OV5642YUV_write_cmos_sensor(0x530e,0x20);
    OV5642YUV_write_cmos_sensor(0x530f,0x80);
    OV5642YUV_write_cmos_sensor(0x5310,0x20);
    OV5642YUV_write_cmos_sensor(0x5311,0x80);
    OV5642YUV_write_cmos_sensor(0x5308,0x20);
    OV5642YUV_write_cmos_sensor(0x5309,0x40);
    OV5642YUV_write_cmos_sensor(0x5304,0x00);
    OV5642YUV_write_cmos_sensor(0x5305,0x30);
    OV5642YUV_write_cmos_sensor(0x5306,0x00);
    OV5642YUV_write_cmos_sensor(0x5307,0x80);
    OV5642YUV_write_cmos_sensor(0x5314,0x08);
    OV5642YUV_write_cmos_sensor(0x5315,0x20);
    OV5642YUV_write_cmos_sensor(0x5319,0x30);
    OV5642YUV_write_cmos_sensor(0x5316,0x10);

    OV5642YUV_write_cmos_sensor(0x5317,0x00);
	
    OV5642YUV_write_cmos_sensor(0x5318,0x02);

    OV5642YUV_write_cmos_sensor(0x5380,0x01);
    OV5642YUV_write_cmos_sensor(0x5381,0x00);
    OV5642YUV_write_cmos_sensor(0x5382,0x00);
    OV5642YUV_write_cmos_sensor(0x5383,0x4e);
    OV5642YUV_write_cmos_sensor(0x5384,0x00);
    OV5642YUV_write_cmos_sensor(0x5385,0x0f);
    OV5642YUV_write_cmos_sensor(0x5386,0x00);
    OV5642YUV_write_cmos_sensor(0x5387,0x00);
    OV5642YUV_write_cmos_sensor(0x5388,0x01);
    OV5642YUV_write_cmos_sensor(0x5389,0x15);
    OV5642YUV_write_cmos_sensor(0x538a,0x00);
    OV5642YUV_write_cmos_sensor(0x538b,0x31);
    OV5642YUV_write_cmos_sensor(0x538c,0x00);
    OV5642YUV_write_cmos_sensor(0x538d,0x00);
    OV5642YUV_write_cmos_sensor(0x538e,0x00);
    OV5642YUV_write_cmos_sensor(0x538f,0x0f);
    OV5642YUV_write_cmos_sensor(0x5390,0x00);
    OV5642YUV_write_cmos_sensor(0x5391,0xab);
    OV5642YUV_write_cmos_sensor(0x5392,0x00);
    OV5642YUV_write_cmos_sensor(0x5393,0xa2);
    OV5642YUV_write_cmos_sensor(0x5394,0x08);

    OV5642YUV_write_cmos_sensor(0x5480,0x14);
    OV5642YUV_write_cmos_sensor(0x5481,0x21);
    OV5642YUV_write_cmos_sensor(0x5482,0x36);
    OV5642YUV_write_cmos_sensor(0x5483,0x57);
    OV5642YUV_write_cmos_sensor(0x5484,0x65);
    OV5642YUV_write_cmos_sensor(0x5485,0x71);
    OV5642YUV_write_cmos_sensor(0x5486,0x7d);
    OV5642YUV_write_cmos_sensor(0x5487,0x87);
    OV5642YUV_write_cmos_sensor(0x5488,0x91);
    OV5642YUV_write_cmos_sensor(0x5489,0x9a);
    OV5642YUV_write_cmos_sensor(0x548a,0xaa);
    OV5642YUV_write_cmos_sensor(0x548b,0xb8);
    OV5642YUV_write_cmos_sensor(0x548c,0xcd);
    OV5642YUV_write_cmos_sensor(0x548d,0xdd);
    OV5642YUV_write_cmos_sensor(0x548e,0xea);
    OV5642YUV_write_cmos_sensor(0x548f,0x1d);
    OV5642YUV_write_cmos_sensor(0x5490,0x05);
    OV5642YUV_write_cmos_sensor(0x5491,0x00);
    OV5642YUV_write_cmos_sensor(0x5492,0x04);
    OV5642YUV_write_cmos_sensor(0x5493,0x20);
    OV5642YUV_write_cmos_sensor(0x5494,0x03);
    OV5642YUV_write_cmos_sensor(0x5495,0x60);
    OV5642YUV_write_cmos_sensor(0x5496,0x02);
    OV5642YUV_write_cmos_sensor(0x5497,0xb8);
    OV5642YUV_write_cmos_sensor(0x5498,0x02);
    OV5642YUV_write_cmos_sensor(0x5499,0x86);
    OV5642YUV_write_cmos_sensor(0x549a,0x02);
    OV5642YUV_write_cmos_sensor(0x549b,0x5b);
    OV5642YUV_write_cmos_sensor(0x549c,0x02);
    OV5642YUV_write_cmos_sensor(0x549d,0x3b);
    OV5642YUV_write_cmos_sensor(0x549e,0x02);
    OV5642YUV_write_cmos_sensor(0x549f,0x1c);
    OV5642YUV_write_cmos_sensor(0x54a0,0x02);
    OV5642YUV_write_cmos_sensor(0x54a1,0x04);
    OV5642YUV_write_cmos_sensor(0x54a2,0x01);
    OV5642YUV_write_cmos_sensor(0x54a3,0xed);
    OV5642YUV_write_cmos_sensor(0x54a4,0x01);
    OV5642YUV_write_cmos_sensor(0x54a5,0xc5);
    OV5642YUV_write_cmos_sensor(0x54a6,0x01);
    OV5642YUV_write_cmos_sensor(0x54a7,0xa5);
    OV5642YUV_write_cmos_sensor(0x54a8,0x01);
    OV5642YUV_write_cmos_sensor(0x54a9,0x6c);
    OV5642YUV_write_cmos_sensor(0x54aa,0x01);
    OV5642YUV_write_cmos_sensor(0x54ab,0x41);
    OV5642YUV_write_cmos_sensor(0x54ac,0x01);
    OV5642YUV_write_cmos_sensor(0x54ad,0x20);
    OV5642YUV_write_cmos_sensor(0x54ae,0x00);
    OV5642YUV_write_cmos_sensor(0x54af,0x16);
    OV5642YUV_write_cmos_sensor(0x54b0,0x01);
    OV5642YUV_write_cmos_sensor(0x54b1,0x20);
    OV5642YUV_write_cmos_sensor(0x54b2,0x00);
    OV5642YUV_write_cmos_sensor(0x54b3,0x10);
    OV5642YUV_write_cmos_sensor(0x54b4,0x00);
    OV5642YUV_write_cmos_sensor(0x54b5,0xf0);
    OV5642YUV_write_cmos_sensor(0x54b6,0x00);
    OV5642YUV_write_cmos_sensor(0x54b7,0xdf);
    OV5642YUV_write_cmos_sensor(0x5402,0x3f);
    OV5642YUV_write_cmos_sensor(0x5403,0x00);
    OV5642YUV_write_cmos_sensor(0x3406,0x00);

    OV5642YUV_write_cmos_sensor(0x5180,0xff);
    OV5642YUV_write_cmos_sensor(0x5181,0x52);
    OV5642YUV_write_cmos_sensor(0x5182,0x11);
    OV5642YUV_write_cmos_sensor(0x5183,0x14);
    OV5642YUV_write_cmos_sensor(0x5184,0x25);
    OV5642YUV_write_cmos_sensor(0x5185,0x24);
    OV5642YUV_write_cmos_sensor(0x5186,0x06);
    OV5642YUV_write_cmos_sensor(0x5187,0x08);
    OV5642YUV_write_cmos_sensor(0x5188,0x08);
    OV5642YUV_write_cmos_sensor(0x5189,0x7c);
    OV5642YUV_write_cmos_sensor(0x518a,0x60);
    OV5642YUV_write_cmos_sensor(0x518b,0xb2);
    OV5642YUV_write_cmos_sensor(0x518c,0xb2);
    OV5642YUV_write_cmos_sensor(0x518d,0x44);
    OV5642YUV_write_cmos_sensor(0x518e,0x3d);
    OV5642YUV_write_cmos_sensor(0x518f,0x58);
    OV5642YUV_write_cmos_sensor(0x5190,0x46);
    OV5642YUV_write_cmos_sensor(0x5191,0xf8);
    OV5642YUV_write_cmos_sensor(0x5192,0x04);
    OV5642YUV_write_cmos_sensor(0x5193,0x70);
    OV5642YUV_write_cmos_sensor(0x5194,0xf0);
    OV5642YUV_write_cmos_sensor(0x5195,0xf0);
    OV5642YUV_write_cmos_sensor(0x5196,0x03);
    OV5642YUV_write_cmos_sensor(0x5197,0x01);
    OV5642YUV_write_cmos_sensor(0x5198,0x04);
    OV5642YUV_write_cmos_sensor(0x5199,0x12);
    OV5642YUV_write_cmos_sensor(0x519a,0x04);
    OV5642YUV_write_cmos_sensor(0x519b,0x00);
    OV5642YUV_write_cmos_sensor(0x519c,0x06);
    OV5642YUV_write_cmos_sensor(0x519d,0x82);
    OV5642YUV_write_cmos_sensor(0x519e,0x00);


    OV5642YUV_write_cmos_sensor(0x5025,0x80);
    OV5642YUV_write_cmos_sensor(0x3a0f,0x38);
    OV5642YUV_write_cmos_sensor(0x3a10,0x30);
    OV5642YUV_write_cmos_sensor(0x3a1b,0x3a);
    OV5642YUV_write_cmos_sensor(0x3a1e,0x2e);
    OV5642YUV_write_cmos_sensor(0x3a11,0x60);
    OV5642YUV_write_cmos_sensor(0x3a1f,0x10);
    OV5642YUV_write_cmos_sensor(0x5688,0xa6);
    OV5642YUV_write_cmos_sensor(0x5689,0x6a);
    OV5642YUV_write_cmos_sensor(0x568a,0xea);
    OV5642YUV_write_cmos_sensor(0x568b,0xae);
    OV5642YUV_write_cmos_sensor(0x568c,0xa6);
    OV5642YUV_write_cmos_sensor(0x568d,0x6a);
    OV5642YUV_write_cmos_sensor(0x568e,0x62);
    OV5642YUV_write_cmos_sensor(0x568f,0x26);

    OV5642YUV_write_cmos_sensor(0x5583,0x40);
    OV5642YUV_write_cmos_sensor(0x5584,0x40);
    OV5642YUV_write_cmos_sensor(0x5580,0x02);

    OV5642YUV_write_cmos_sensor(0x5000,0xcf);
    OV5642YUV_write_cmos_sensor(0x5800,0x27);
    OV5642YUV_write_cmos_sensor(0x5801,0x19);
    OV5642YUV_write_cmos_sensor(0x5802,0x12);
    OV5642YUV_write_cmos_sensor(0x5803,0x0f);
    OV5642YUV_write_cmos_sensor(0x5804,0x10);
    OV5642YUV_write_cmos_sensor(0x5805,0x15);
    OV5642YUV_write_cmos_sensor(0x5806,0x1e);
    OV5642YUV_write_cmos_sensor(0x5807,0x2f);
    OV5642YUV_write_cmos_sensor(0x5808,0x15);
    OV5642YUV_write_cmos_sensor(0x5809,0x0d);
    OV5642YUV_write_cmos_sensor(0x580a,0x0a);
    OV5642YUV_write_cmos_sensor(0x580b,0x09);
    OV5642YUV_write_cmos_sensor(0x580c,0x0a);
    OV5642YUV_write_cmos_sensor(0x580d,0x0c);
    OV5642YUV_write_cmos_sensor(0x580e,0x12);
    OV5642YUV_write_cmos_sensor(0x580f,0x19);
    OV5642YUV_write_cmos_sensor(0x5810,0x0b);
    OV5642YUV_write_cmos_sensor(0x5811,0x07);
    OV5642YUV_write_cmos_sensor(0x5812,0x04);
    OV5642YUV_write_cmos_sensor(0x5813,0x03);
    OV5642YUV_write_cmos_sensor(0x5814,0x03);
    OV5642YUV_write_cmos_sensor(0x5815,0x06);
    OV5642YUV_write_cmos_sensor(0x5816,0x0a);
    OV5642YUV_write_cmos_sensor(0x5817,0x0f);
    OV5642YUV_write_cmos_sensor(0x5818,0x0a);
    OV5642YUV_write_cmos_sensor(0x5819,0x05);
    OV5642YUV_write_cmos_sensor(0x581a,0x01);
    OV5642YUV_write_cmos_sensor(0x581b,0x00);
    OV5642YUV_write_cmos_sensor(0x581c,0x00);
    OV5642YUV_write_cmos_sensor(0x581d,0x03);
    OV5642YUV_write_cmos_sensor(0x581e,0x08);
    OV5642YUV_write_cmos_sensor(0x581f,0x0c);
    OV5642YUV_write_cmos_sensor(0x5820,0x0a);
    OV5642YUV_write_cmos_sensor(0x5821,0x05);
    OV5642YUV_write_cmos_sensor(0x5822,0x01);
    OV5642YUV_write_cmos_sensor(0x5823,0x00);
    OV5642YUV_write_cmos_sensor(0x5824,0x00);
    OV5642YUV_write_cmos_sensor(0x5825,0x03);
    OV5642YUV_write_cmos_sensor(0x5826,0x08);
    OV5642YUV_write_cmos_sensor(0x5827,0x0c);
    OV5642YUV_write_cmos_sensor(0x5828,0x0e);
    OV5642YUV_write_cmos_sensor(0x5829,0x08);
    OV5642YUV_write_cmos_sensor(0x582a,0x06);
    OV5642YUV_write_cmos_sensor(0x582b,0x04);
    OV5642YUV_write_cmos_sensor(0x582c,0x05);
    OV5642YUV_write_cmos_sensor(0x582d,0x07);
    OV5642YUV_write_cmos_sensor(0x582e,0x0b);
    OV5642YUV_write_cmos_sensor(0x582f,0x12);
    OV5642YUV_write_cmos_sensor(0x5830,0x18);
    OV5642YUV_write_cmos_sensor(0x5831,0x10);
    OV5642YUV_write_cmos_sensor(0x5832,0x0c);
    OV5642YUV_write_cmos_sensor(0x5833,0x0a);
    OV5642YUV_write_cmos_sensor(0x5834,0x0b);
    OV5642YUV_write_cmos_sensor(0x5835,0x0e);
    OV5642YUV_write_cmos_sensor(0x5836,0x15);
    OV5642YUV_write_cmos_sensor(0x5837,0x19);
    OV5642YUV_write_cmos_sensor(0x5838,0x32);
    OV5642YUV_write_cmos_sensor(0x5839,0x1f);
    OV5642YUV_write_cmos_sensor(0x583a,0x18);
    OV5642YUV_write_cmos_sensor(0x583b,0x16);
    OV5642YUV_write_cmos_sensor(0x583c,0x17);
    OV5642YUV_write_cmos_sensor(0x583d,0x1e);
    OV5642YUV_write_cmos_sensor(0x583e,0x26);
    OV5642YUV_write_cmos_sensor(0x583f,0x53);
    OV5642YUV_write_cmos_sensor(0x5840,0x10);
    OV5642YUV_write_cmos_sensor(0x5841,0x0f);
    OV5642YUV_write_cmos_sensor(0x5842,0x0d);
    OV5642YUV_write_cmos_sensor(0x5843,0x0c);
    OV5642YUV_write_cmos_sensor(0x5844,0x0e);
    OV5642YUV_write_cmos_sensor(0x5845,0x09);
    OV5642YUV_write_cmos_sensor(0x5846,0x11);
    OV5642YUV_write_cmos_sensor(0x5847,0x10);
    OV5642YUV_write_cmos_sensor(0x5848,0x10);
    OV5642YUV_write_cmos_sensor(0x5849,0x10);
    OV5642YUV_write_cmos_sensor(0x584a,0x10);
    OV5642YUV_write_cmos_sensor(0x584b,0x0e);
    OV5642YUV_write_cmos_sensor(0x584c,0x10);
    OV5642YUV_write_cmos_sensor(0x584d,0x10);
    OV5642YUV_write_cmos_sensor(0x584e,0x11);
    OV5642YUV_write_cmos_sensor(0x584f,0x10);
    OV5642YUV_write_cmos_sensor(0x5850,0x0f);
    OV5642YUV_write_cmos_sensor(0x5851,0x0c);
    OV5642YUV_write_cmos_sensor(0x5852,0x0f);
    OV5642YUV_write_cmos_sensor(0x5853,0x10);
    OV5642YUV_write_cmos_sensor(0x5854,0x10);
    OV5642YUV_write_cmos_sensor(0x5855,0x0f);
    OV5642YUV_write_cmos_sensor(0x5856,0x0e);
    OV5642YUV_write_cmos_sensor(0x5857,0x0b);
    OV5642YUV_write_cmos_sensor(0x5858,0x10);
    OV5642YUV_write_cmos_sensor(0x5859,0x0d);
    OV5642YUV_write_cmos_sensor(0x585a,0x0d);
    OV5642YUV_write_cmos_sensor(0x585b,0x0c);
    OV5642YUV_write_cmos_sensor(0x585c,0x0c);
    OV5642YUV_write_cmos_sensor(0x585d,0x0c);
    OV5642YUV_write_cmos_sensor(0x585e,0x0b);
    OV5642YUV_write_cmos_sensor(0x585f,0x0c);
    OV5642YUV_write_cmos_sensor(0x5860,0x0c);
    OV5642YUV_write_cmos_sensor(0x5861,0x0c);
    OV5642YUV_write_cmos_sensor(0x5862,0x0d);
    OV5642YUV_write_cmos_sensor(0x5863,0x08);
    OV5642YUV_write_cmos_sensor(0x5864,0x11);
    OV5642YUV_write_cmos_sensor(0x5865,0x18);
    OV5642YUV_write_cmos_sensor(0x5866,0x18);
    OV5642YUV_write_cmos_sensor(0x5867,0x19);
    OV5642YUV_write_cmos_sensor(0x5868,0x17);
    OV5642YUV_write_cmos_sensor(0x5869,0x19);
    OV5642YUV_write_cmos_sensor(0x586a,0x16);
    OV5642YUV_write_cmos_sensor(0x586b,0x13);
    OV5642YUV_write_cmos_sensor(0x586c,0x13);
    OV5642YUV_write_cmos_sensor(0x586d,0x12);
    OV5642YUV_write_cmos_sensor(0x586e,0x13);
    OV5642YUV_write_cmos_sensor(0x586f,0x16);
    OV5642YUV_write_cmos_sensor(0x5870,0x14);
    OV5642YUV_write_cmos_sensor(0x5871,0x12);
    OV5642YUV_write_cmos_sensor(0x5872,0x10);
    OV5642YUV_write_cmos_sensor(0x5873,0x11);
    OV5642YUV_write_cmos_sensor(0x5874,0x11);
    OV5642YUV_write_cmos_sensor(0x5875,0x16);
    OV5642YUV_write_cmos_sensor(0x5876,0x14);
    OV5642YUV_write_cmos_sensor(0x5877,0x11);
    OV5642YUV_write_cmos_sensor(0x5878,0x10);
    OV5642YUV_write_cmos_sensor(0x5879,0x0f);
    OV5642YUV_write_cmos_sensor(0x587a,0x10);
    OV5642YUV_write_cmos_sensor(0x587b,0x14);
    OV5642YUV_write_cmos_sensor(0x587c,0x13);
    OV5642YUV_write_cmos_sensor(0x587d,0x12);
    OV5642YUV_write_cmos_sensor(0x587e,0x11);
    OV5642YUV_write_cmos_sensor(0x587f,0x11);
    OV5642YUV_write_cmos_sensor(0x5880,0x12);
    OV5642YUV_write_cmos_sensor(0x5881,0x15);
    OV5642YUV_write_cmos_sensor(0x5882,0x14);
    OV5642YUV_write_cmos_sensor(0x5883,0x15);
    OV5642YUV_write_cmos_sensor(0x5884,0x15);
    OV5642YUV_write_cmos_sensor(0x5885,0x15);
    OV5642YUV_write_cmos_sensor(0x5886,0x13);
    OV5642YUV_write_cmos_sensor(0x5887,0x17);

    OV5642YUV_write_cmos_sensor(0x3710,0x10);
    OV5642YUV_write_cmos_sensor(0x3632,0x51);
    OV5642YUV_write_cmos_sensor(0x3702,0x10);
    OV5642YUV_write_cmos_sensor(0x3703,0xb2);
    OV5642YUV_write_cmos_sensor(0x3704,0x18);
    OV5642YUV_write_cmos_sensor(0x370b,0x40);
    OV5642YUV_write_cmos_sensor(0x370d,0x03);
    OV5642YUV_write_cmos_sensor(0x3631,0x01);
    OV5642YUV_write_cmos_sensor(0x3632,0x52);
    OV5642YUV_write_cmos_sensor(0x3606,0x24);
    OV5642YUV_write_cmos_sensor(0x3620,0x96);
    OV5642YUV_write_cmos_sensor(0x5785,0x07);
    OV5642YUV_write_cmos_sensor(0x3a13,0x30);
    OV5642YUV_write_cmos_sensor(0x3600,0x52);
    OV5642YUV_write_cmos_sensor(0x3604,0x48);
    OV5642YUV_write_cmos_sensor(0x3606,0x1b);
    OV5642YUV_write_cmos_sensor(0x370d,0x0b);
    OV5642YUV_write_cmos_sensor(0x370f,0xc0);
    OV5642YUV_write_cmos_sensor(0x3709,0x01);
    OV5642YUV_write_cmos_sensor(0x3823,0x00);
    OV5642YUV_write_cmos_sensor(0x5007,0x00);
    OV5642YUV_write_cmos_sensor(0x5009,0x00);
    OV5642YUV_write_cmos_sensor(0x5011,0x00);
    OV5642YUV_write_cmos_sensor(0x5013,0x00);
    OV5642YUV_write_cmos_sensor(0x519e,0x00);
    OV5642YUV_write_cmos_sensor(0x5086,0x00);
    OV5642YUV_write_cmos_sensor(0x5087,0x00);
    OV5642YUV_write_cmos_sensor(0x5088,0x00);
    OV5642YUV_write_cmos_sensor(0x5089,0x00);
    OV5642YUV_write_cmos_sensor(0x302b,0x00);

    OV5642YUV_write_cmos_sensor(0x3621,0x87);

    // Set Mirror and flip, @Sean
    OV5642YUV_write_cmos_sensor(0x3818, 0xa1);	//Set mirror and flip
    OV5642YUV_write_cmos_sensor(0x3621, 0xe7);	    


    OV5642YUV_write_cmos_sensor(0x3a00,0x78);




//add by lingnan   --- enable awb
    OV5642YUV_write_cmos_sensor(0x5001,0xff);//ISP CONTROL 01
    OV5642YUV_write_cmos_sensor(0x3406,0x00);//ISP CONTROL 01
#if 0
    //PLL setting
    OV5642YUV_write_cmos_sensor(0x300f, 0x6);
    OV5642YUV_write_cmos_sensor(0x3010, 0x10);
    OV5642YUV_write_cmos_sensor(0x3011, 0x10);
    OV5642YUV_write_cmos_sensor(0x3012, 0x2);
    OV5642YUV_write_cmos_sensor(0x3815, 0x2);
    OV5642YUV_write_cmos_sensor(0x3029, 0x0);
    OV5642YUV_write_cmos_sensor(0x3033, 0x3);
    OV5642YUV_write_cmos_sensor(0x460c, 0x20);
#endif

//use koli capture setting, 14.4fps, 46.8*2MHZ
    OV5642YUV_write_cmos_sensor(0x380c, 0x06);
    OV5642YUV_write_cmos_sensor(0x380d, 0x80);
    OV5642YUV_write_cmos_sensor(0x380e, 0x03);
    OV5642YUV_write_cmos_sensor(0x380f, 0xd0);

        //PLL setting
    OV5642YUV_write_cmos_sensor(0x300f, 0x6);
    OV5642YUV_write_cmos_sensor(0x3010, 0x40);
    OV5642YUV_write_cmos_sensor(0x3011, 0x09);
    OV5642YUV_write_cmos_sensor(0x3012, 0x00);
    OV5642YUV_write_cmos_sensor(0x3815, 0x01);
    OV5642YUV_write_cmos_sensor(0x3029, 0x00);
    OV5642YUV_write_cmos_sensor(0x3033, 0x03);
    OV5642YUV_write_cmos_sensor(0x460c, 0x20);

    SENSORDB("0x350b=0x%x\n",OV5642YUV_read_cmos_sensor(0x350b));

}
void OV5642YUV_set_720P(void)
{
    //int temp;
    int dummy_pixels, dummy_lines;
    OV5642YUV_g_RES = OV5642_720P;
    SENSORDB("OV5642YUV_set_720P Start \n"); 
    #if 0
    OV5642YUV_write_cmos_sensor(0x3503,0x7 );
    OV5642YUV_write_cmos_sensor(0x3001,0x0 );
    OV5642YUV_write_cmos_sensor(0x3002,0x5c);
    OV5642YUV_write_cmos_sensor(0x300d,0x22);
    OV5642YUV_write_cmos_sensor(0x3011,0x11 );    
    
    OV5642YUV_write_cmos_sensor(0x3622,0x60);
    OV5642YUV_write_cmos_sensor(0x3621,0x87);
    OV5642YUV_write_cmos_sensor(0x3709,0x1 );
    OV5642YUV_write_cmos_sensor(0x3600,0x52);
    OV5642YUV_write_cmos_sensor(0x3602,0xfc);
    OV5642YUV_write_cmos_sensor(0x3606,0x1b);
    OV5642YUV_write_cmos_sensor(0x3612,0xff);
    OV5642YUV_write_cmos_sensor(0x3613,0x0 );
    OV5642YUV_write_cmos_sensor(0x3623,0x1 );
    OV5642YUV_write_cmos_sensor(0x3705,0xdb);
    OV5642YUV_write_cmos_sensor(0x370a,0x81);
    OV5642YUV_write_cmos_sensor(0x3801,0x50);
    OV5642YUV_write_cmos_sensor(0x3804,0x5 );
    OV5642YUV_write_cmos_sensor(0x3805,0x10);
    OV5642YUV_write_cmos_sensor(0x3806,0x3 );
    OV5642YUV_write_cmos_sensor(0x3807,0xcc);
    OV5642YUV_write_cmos_sensor(0x3808,0x5 );
    OV5642YUV_write_cmos_sensor(0x3809,0x10);
    OV5642YUV_write_cmos_sensor(0x380a,0x3 );
    OV5642YUV_write_cmos_sensor(0x380b,0xcc);
    OV5642YUV_write_cmos_sensor(0x380c,0x7 );
    OV5642YUV_write_cmos_sensor(0x380d,0xc0);
    OV5642YUV_write_cmos_sensor(0x380e,0x3 );
    OV5642YUV_write_cmos_sensor(0x380f,0xf0);
    
    OV5642YUV_write_cmos_sensor(0x3810,0x42);
    OV5642YUV_write_cmos_sensor(0x3818,0xc1);
    OV5642YUV_write_cmos_sensor(0x3824,0x11);
    OV5642YUV_write_cmos_sensor(0x3825,0xb0);
    OV5642YUV_write_cmos_sensor(0x3827,0x8 );
    OV5642YUV_write_cmos_sensor(0x3a1a,0x5 );
    OV5642YUV_write_cmos_sensor(0x3a08,0x12);
    OV5642YUV_write_cmos_sensor(0x3a09,0xc0);
    OV5642YUV_write_cmos_sensor(0x3a0a,0xf );
    OV5642YUV_write_cmos_sensor(0x3a0b,0xa0);
    OV5642YUV_write_cmos_sensor(0x3a0d,0x4 );
    OV5642YUV_write_cmos_sensor(0x3a0e,0x3 );
    OV5642YUV_write_cmos_sensor(0x401c,0x4 );
    OV5642YUV_write_cmos_sensor(0x5000,0x6 );
    OV5642YUV_write_cmos_sensor(0x5001,0x1 );//00
    OV5642YUV_write_cmos_sensor(0x5005,0xdc);
    OV5642YUV_write_cmos_sensor(0x3818, 0xa1);	//Set mirror and flip
    OV5642YUV_write_cmos_sensor(0x3621, 0xe7);	
    #endif
    
    OV5642YUV_Sensor_Init_set_720P();
    //set dummy
    //=OV5642_PV_PERIOD_PIXEL_NUMS + 
    //dummy_pixels = 2592 / 2 + 450;
    dummy_pixels = OV5642_PV_PERIOD_PIXEL_NUMS + OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS;
    //=OV5642_PV_PERIOD_LINE_NUMS + 
    //dummy_lines = 972 + 28;
    dummy_lines = OV5642_PV_PERIOD_LINE_NUMS + OV5642_PV_PERIOD_EXTRA_LINE_NUMS;



    //HTS
    
    //OV5642YUV_write_cmos_sensor(0x380c, (dummy_pixels >> 8) & 0xff);
   // OV5642YUV_write_cmos_sensor(0x380d, (dummy_pixels  & 0xff));
//    

    //AEC VTS
    //OV5642YUV_write_cmos_sensor(0x350c, (dummy_lines >> 8) & 0xff);
    //OV5642YUV_write_cmos_sensor(0x350d, (dummy_lines & 0xff));

    OV5642YUV_PV_pclk = 5525; 
    OV5642YUV_sensor_pclk=55250000;//19500000;


//enable pad output bit[1:0] enable gpio1,gpio0 for mcu
//    OV5642YUV_write_cmos_sensor(0x3018, 0xff);


//    SENSORDB("preview start may 1 change mcu\n");
//    OV5642_FOCUS_AD5820_Check_MCU();

    SENSORDB("Set 720P End\n"); 
}

void OV5642YUV_set_5M_init(void)
{
    OV5642YUV_write_cmos_sensor(0x3612,0xac);
    OV5642YUV_write_cmos_sensor(0x3613,0x44);
    //OV5642YUV_write_cmos_sensor(0x3622,0x8 );   //cause screen to be black 
    OV5642YUV_write_cmos_sensor(0x3623,0x22);
    //OV5642YUV_write_cmos_sensor(0x3604,0x60);
    OV5642YUV_write_cmos_sensor(0x3705,0xda);
    OV5642YUV_write_cmos_sensor(0x370a,0x80);
    OV5642YUV_write_cmos_sensor(0x3801,0x8a);
    OV5642YUV_write_cmos_sensor(0x3803,0xa );
    OV5642YUV_write_cmos_sensor(0x3804,0xa );
    OV5642YUV_write_cmos_sensor(0x3805,0x20);
    OV5642YUV_write_cmos_sensor(0x3806,0x7 );
    OV5642YUV_write_cmos_sensor(0x3807,0x98);
    OV5642YUV_write_cmos_sensor(0x3808,0xa );
    OV5642YUV_write_cmos_sensor(0x3809,0x20);
    OV5642YUV_write_cmos_sensor(0x380a,0x7 );
    OV5642YUV_write_cmos_sensor(0x3503,0x7 );
    OV5642YUV_write_cmos_sensor(0x3000,0x0 );
    OV5642YUV_write_cmos_sensor(0x3001,0x0 );
    OV5642YUV_write_cmos_sensor(0x3002,0x0 );
    OV5642YUV_write_cmos_sensor(0x3003,0x0 );
    OV5642YUV_write_cmos_sensor(0x3005,0xff);
    OV5642YUV_write_cmos_sensor(0x3006,0xff);
    OV5642YUV_write_cmos_sensor(0x3007,0x3f);
    OV5642YUV_write_cmos_sensor(0x350c,0x7 );
    OV5642YUV_write_cmos_sensor(0x350d,0xd0);
    OV5642YUV_write_cmos_sensor(0x3602,0xe4);
    OV5642YUV_write_cmos_sensor(0x380b,0x98);
    OV5642YUV_write_cmos_sensor(0x380c,0xc );
//    OV5642YUV_write_cmos_sensor(0x380d,0xe8);
    OV5642YUV_write_cmos_sensor(0x380d,0x80);
    OV5642YUV_write_cmos_sensor(0x380e,0x7 );
//    OV5642YUV_write_cmos_sensor(0x380f,0xf0);
    OV5642YUV_write_cmos_sensor(0x380f,0xd0);
    OV5642YUV_write_cmos_sensor(0x3810,0xc2);
    //flip and mirror 
    //OV5642YUV_write_cmos_sensor(0x3621,0x29);    
    //OV5642YUV_write_cmos_sensor(0x3818,0xa0);
    OV5642YUV_write_cmos_sensor(0x3824,0x11 );
    OV5642YUV_write_cmos_sensor(0x3827,0xa );
    
    //AEC Control 
    OV5642YUV_write_cmos_sensor(0x3a00,0x78);
    OV5642YUV_write_cmos_sensor(0x3a0d,0x10);
    OV5642YUV_write_cmos_sensor(0x3a0e,0xd );
    OV5642YUV_write_cmos_sensor(0x3a10,0x32);
    OV5642YUV_write_cmos_sensor(0x3a1b,0x40);
    OV5642YUV_write_cmos_sensor(0x3a1e,0x2e);
    OV5642YUV_write_cmos_sensor(0x3a11,0xd0);
    OV5642YUV_write_cmos_sensor(0x3a1f,0x40); 
    OV5642YUV_write_cmos_sensor(0x3a00,0x78);  //AEC system ctrl
    
    OV5642YUV_write_cmos_sensor(0x460b,0x37);

    OV5642YUV_write_cmos_sensor(0x471d,0x5 );
     
    OV5642YUV_write_cmos_sensor(0x4713,0x2 );
    OV5642YUV_write_cmos_sensor(0x471c,0xd0);
    
    OV5642YUV_write_cmos_sensor(0x5682,0xa );   //avg x end 
    OV5642YUV_write_cmos_sensor(0x5683,0x20); 
    OV5642YUV_write_cmos_sensor(0x5686,0x7 );   //avg y end 
    OV5642YUV_write_cmos_sensor(0x5687,0x98);
    OV5642YUV_write_cmos_sensor(0x5001,0xFF );   //ISP COntrol, SE, UV, Color matrix
    //OV5642YUV_write_cmos_sensor(0x589b,0x0 );
    //OV5642YUV_write_cmos_sensor(0x589a,0xc0);
    //OV5642YUV_write_cmos_sensor(0x4407,0xc );
    
    OV5642YUV_write_cmos_sensor(0x589b,0x0 );   //??
    OV5642YUV_write_cmos_sensor(0x589a,0xc0);  //?? 
    
    OV5642YUV_write_cmos_sensor(0x3002,0x0 );    //reset for individual block 
    //OV5642YUV_write_cmos_sensor(0x3002,0x0 );
    OV5642YUV_write_cmos_sensor(0x3503,0x0 );   //AEC auto mode 
    //pll 
        //PLL setting
    OV5642YUV_write_cmos_sensor(0x300f, 0x6);
    OV5642YUV_write_cmos_sensor(0x3010, 0x40);
    OV5642YUV_write_cmos_sensor(0x3011, 0x09);
    OV5642YUV_write_cmos_sensor(0x3012, 0x00);
    OV5642YUV_write_cmos_sensor(0x3815, 0x01);
    OV5642YUV_write_cmos_sensor(0x3029, 0x00);
    OV5642YUV_write_cmos_sensor(0x3033, 0x03);
    OV5642YUV_write_cmos_sensor(0x460c, 0x20);

#if 0    
    OV5642YUV_write_cmos_sensor(0x3010,0x10);
    OV5642YUV_write_cmos_sensor(0x3011,0x1f);
    OV5642YUV_write_cmos_sensor(0x3012,0x05);
    OV5642YUV_write_cmos_sensor(0x3815,0x02);
#endif 
     
    //OV5642YUV_write_cmos_sensor(0x3009,0x1 );
    //OV5642YUV_write_cmos_sensor(0x300a,0x56);


}
void OV5642YUV_set_5M(void)
{
    SENSORDB("Set 5M begin\n"); 
    OV5642YUV_g_RES = OV5642_5M;

    //;Clock configration based on 24Mhz input.
    //;48MHz PCLK output
    //;90 0e 36
    //;90 0f  0c
    //;90 11 40
    //;window configuration
#if 0
    OV5642YUV_write_cmos_sensor(0x3503,0x7 );

    OV5642YUV_write_cmos_sensor(0x3001,0x40);
    OV5642YUV_write_cmos_sensor(0x3002,0x1c);
    OV5642YUV_write_cmos_sensor(0x300d,0x21);
    //OV5642YUV_write_cmos_sensor(0x3011,0x8 );//pll
    //OV5642YUV_write_cmos_sensor(0x3010,0x30);//pll
    //OV5642YUV_write_cmos_sensor(0x3012,0x00);//pll

    //OV5642YUV_write_cmos_sensor(0x350c,0x07);
    //OV5642YUV_write_cmos_sensor(0x350d,0xd0);

    OV5642YUV_write_cmos_sensor(0x3622,0x60);
    OV5642YUV_write_cmos_sensor(0x3621,0x09);//29
    OV5642YUV_write_cmos_sensor(0x3709,0x0 );
    OV5642YUV_write_cmos_sensor(0x3600,0x54);
    OV5642YUV_write_cmos_sensor(0x3602,0xe4);
    OV5642YUV_write_cmos_sensor(0x3606,0x1b);
    OV5642YUV_write_cmos_sensor(0x3612,0xac);
    OV5642YUV_write_cmos_sensor(0x3613,0x44);
    OV5642YUV_write_cmos_sensor(0x3623,0x22);
    OV5642YUV_write_cmos_sensor(0x3705,0xda);
    OV5642YUV_write_cmos_sensor(0x370a,0x80);

    OV5642YUV_write_cmos_sensor(0x3801,0xb4);
    OV5642YUV_write_cmos_sensor(0x3804,0x0a);
    OV5642YUV_write_cmos_sensor(0x3805,0x20);
    OV5642YUV_write_cmos_sensor(0x3806,0x07);
    OV5642YUV_write_cmos_sensor(0x3807,0x98);
    OV5642YUV_write_cmos_sensor(0x3808,0x0a);
    OV5642YUV_write_cmos_sensor(0x3809,0x20);
    OV5642YUV_write_cmos_sensor(0x380a,0x07);
    OV5642YUV_write_cmos_sensor(0x380b,0x98);
    OV5642YUV_write_cmos_sensor(0x380c,0x0c);
    OV5642YUV_write_cmos_sensor(0x380d,0x80);
    OV5642YUV_write_cmos_sensor(0x380e,0x07);
    OV5642YUV_write_cmos_sensor(0x380f,0xd0);
    
    OV5642YUV_write_cmos_sensor(0x3810,0xc2);
    OV5642YUV_write_cmos_sensor(0x3818,0xc0);//80
    OV5642YUV_write_cmos_sensor(0x3824,0x11);
    OV5642YUV_write_cmos_sensor(0x3825,0xac);
    OV5642YUV_write_cmos_sensor(0x3827,0xc );

    OV5642YUV_write_cmos_sensor(0x3a1a,0x4 );
    OV5642YUV_write_cmos_sensor(0x3a08,0x9 );
    OV5642YUV_write_cmos_sensor(0x3a09,0x60);
    OV5642YUV_write_cmos_sensor(0x3a0a,0x7 );
    OV5642YUV_write_cmos_sensor(0x3a0b,0xd0);
    OV5642YUV_write_cmos_sensor(0x3a0d,0x10);
    OV5642YUV_write_cmos_sensor(0x3a0e,0xd );

    OV5642YUV_write_cmos_sensor(0x401c,0x6 );
    OV5642YUV_write_cmos_sensor(0x5000,0x6 );
    OV5642YUV_write_cmos_sensor(0x5001,0x1 );//awb
    OV5642YUV_write_cmos_sensor(0x5005,0x0 );
    OV5642YUV_write_cmos_sensor(0x3011,0x13 );    //increase PLL, fps = 9.64
#endif
#if 0 
    OV5642YUV_write_cmos_sensor(0x3808, 0x0A);
    OV5642YUV_write_cmos_sensor(0x3809, 0x20);
    OV5642YUV_write_cmos_sensor(0x380A, 0x07);
    OV5642YUV_write_cmos_sensor(0x380B, 0x98);
#endif     
    OV5642YUV_set_5M_init();

    //OV5642YUV_set_AE_mode(KAL_FALSE);
    //OV5642YUV_set_AWB_mode(KAL_FALSE);

    //OV5642_Sensor_Total_5M();	
    OV5642YUV_PV_pclk = 6175; 
    OV5642YUV_sensor_pclk=61750000;//19500000;
    SENSORDB("Set 5M End\n"); 
}

void OV5642YUV_dump_5M(void)
{
    kal_uint8 dump_5M[69] = {0x00};

    dump_5M[0] = OV5642YUV_read_cmos_sensor(0x3612);
    dump_5M[1] = OV5642YUV_read_cmos_sensor(0x3613);
    dump_5M[2] = OV5642YUV_read_cmos_sensor(0x3621);
    dump_5M[3] = OV5642YUV_read_cmos_sensor(0x3622);
    dump_5M[4] = OV5642YUV_read_cmos_sensor(0x3623);
    dump_5M[5] = OV5642YUV_read_cmos_sensor(0x3604);
    dump_5M[6] = OV5642YUV_read_cmos_sensor(0x3705);
    dump_5M[7] = OV5642YUV_read_cmos_sensor(0x370a);
    dump_5M[8] = OV5642YUV_read_cmos_sensor(0x3801);
    dump_5M[9] = OV5642YUV_read_cmos_sensor(0x3803);
    dump_5M[10] = OV5642YUV_read_cmos_sensor(0x3804);
    dump_5M[11] = OV5642YUV_read_cmos_sensor(0x3805);
    dump_5M[12] = OV5642YUV_read_cmos_sensor(0x3806);
    dump_5M[13] = OV5642YUV_read_cmos_sensor(0x3807);
    dump_5M[14] = OV5642YUV_read_cmos_sensor(0x3808);
    dump_5M[15] = OV5642YUV_read_cmos_sensor(0x3809);
    dump_5M[16] = OV5642YUV_read_cmos_sensor(0x380a);
    dump_5M[17] = OV5642YUV_read_cmos_sensor(0x3503);
    dump_5M[18] = OV5642YUV_read_cmos_sensor(0x3000);
    dump_5M[19] = OV5642YUV_read_cmos_sensor(0x3001);
    dump_5M[20] = OV5642YUV_read_cmos_sensor(0x3002);
    dump_5M[21] = OV5642YUV_read_cmos_sensor(0x3003);
    dump_5M[22] = OV5642YUV_read_cmos_sensor(0x3005);
    dump_5M[23] = OV5642YUV_read_cmos_sensor(0x3006);
    dump_5M[24] = OV5642YUV_read_cmos_sensor(0x3007);
    dump_5M[25] = OV5642YUV_read_cmos_sensor(0x350c);
    dump_5M[26] = OV5642YUV_read_cmos_sensor(0x350d);
    dump_5M[27] = OV5642YUV_read_cmos_sensor(0x3602);
    dump_5M[28] = OV5642YUV_read_cmos_sensor(0x380b);
    dump_5M[29] = OV5642YUV_read_cmos_sensor(0x380c);
    dump_5M[30] = OV5642YUV_read_cmos_sensor(0x380d);
    dump_5M[31] = OV5642YUV_read_cmos_sensor(0x380e);
    dump_5M[32] = OV5642YUV_read_cmos_sensor(0x380f);
    dump_5M[33] = OV5642YUV_read_cmos_sensor(0x3810);
    dump_5M[34] = OV5642YUV_read_cmos_sensor(0x3815);
    dump_5M[35] = OV5642YUV_read_cmos_sensor(0x3818);
    dump_5M[36] = OV5642YUV_read_cmos_sensor(0x3824);
    dump_5M[37] = OV5642YUV_read_cmos_sensor(0x3827);
    dump_5M[38] = OV5642YUV_read_cmos_sensor(0x3a00);
    dump_5M[39] = OV5642YUV_read_cmos_sensor(0x3a0d);
    dump_5M[40] = OV5642YUV_read_cmos_sensor(0x3a0e);
    dump_5M[41] = OV5642YUV_read_cmos_sensor(0x3a10);
    dump_5M[42] = OV5642YUV_read_cmos_sensor(0x3a1b);
    dump_5M[43] = OV5642YUV_read_cmos_sensor(0x3a1e);
    dump_5M[44] = OV5642YUV_read_cmos_sensor(0x3a11);
    dump_5M[45] = OV5642YUV_read_cmos_sensor(0x3a1f);
    dump_5M[46] = OV5642YUV_read_cmos_sensor(0x3a00);
    dump_5M[47] = OV5642YUV_read_cmos_sensor(0x460b);
    dump_5M[48] = OV5642YUV_read_cmos_sensor(0x471d);
    dump_5M[49] = OV5642YUV_read_cmos_sensor(0x4713);
    dump_5M[50] = OV5642YUV_read_cmos_sensor(0x471c);
    dump_5M[51] = OV5642YUV_read_cmos_sensor(0x5682);
    dump_5M[52] = OV5642YUV_read_cmos_sensor(0x5683);
    dump_5M[53] = OV5642YUV_read_cmos_sensor(0x5686);
    dump_5M[54] = OV5642YUV_read_cmos_sensor(0x5687);
    dump_5M[55] = OV5642YUV_read_cmos_sensor(0x5001);
    dump_5M[56] = OV5642YUV_read_cmos_sensor(0x589b);
    dump_5M[57] = OV5642YUV_read_cmos_sensor(0x589a);
    dump_5M[58] = OV5642YUV_read_cmos_sensor(0x4407);
    dump_5M[59] = OV5642YUV_read_cmos_sensor(0x589b);
    dump_5M[60] = OV5642YUV_read_cmos_sensor(0x589a);
    dump_5M[61] = OV5642YUV_read_cmos_sensor(0x3002);
    dump_5M[62] = OV5642YUV_read_cmos_sensor(0x3002);
    dump_5M[63] = OV5642YUV_read_cmos_sensor(0x3503);
    dump_5M[64] = OV5642YUV_read_cmos_sensor(0x3010);
    dump_5M[65] = OV5642YUV_read_cmos_sensor(0x3011);
    dump_5M[66] = OV5642YUV_read_cmos_sensor(0x3012);
    dump_5M[67] = OV5642YUV_read_cmos_sensor(0x3009);
    dump_5M[68] = OV5642YUV_read_cmos_sensor(0x300a);

    int i = 0;
    for(i = 0; i < 69; i++)
    {
         SENSORDB("dump_5M[%d] = 0x%x\n", i, dump_5M[i]);
    }

}

/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   OV5642YUVOpen
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

UINT32 OV5642YUVOpen(void)
{
    int  retry = 0; 
#if WINMO_USE
    OV5642YUVhDrvI2C=DRV_I2COpen(1);
    DRV_I2CSetParam(OV5642YUVhDrvI2C, I2C_VAL_FS_SAMPLE_DIV, 3);
    DRV_I2CSetParam(OV5642YUVhDrvI2C, I2C_VAL_FS_STEP_DIV, 8);
    DRV_I2CSetParam(OV5642YUVhDrvI2C, I2C_VAL_DELAY_LEN, 2);
    DRV_I2CSetParam(OV5642YUVhDrvI2C, I2C_VAL_CLK_EXT, I2C_CLKEXT_DISABLE);
    OV5642YUVI2CConfig.trans_mode=I2C_TRANSFER_FAST_MODE;
    OV5642YUVI2CConfig.slaveAddr=OV5642YUV_sensor_write_I2C_address>>1;
    OV5642YUVI2CConfig.RS_ST=I2C_TRANSFER_STOP;	/* for fast mode */
#endif 

    OV5642YUV_sensor_id = ((OV5642YUV_read_cmos_sensor(0x300A) << 8) | OV5642YUV_read_cmos_sensor(0x300B));
    // check if sensor ID correct
    retry = 3; 
    do {
        OV5642YUV_sensor_id = ((OV5642YUV_read_cmos_sensor(0x300A) << 8) | OV5642YUV_read_cmos_sensor(0x300B));        
        if (OV5642YUV_sensor_id == OV5642_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", OV5642YUV_sensor_id); 
        retry--; 
    } while (retry > 0);

    if (OV5642YUV_sensor_id != OV5642_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;

    if(OV5642YUV_Sensor_Init())
    {
        return ERROR_SENSOR_CONNECT_FAIL;

    }


    OV5642YUV_sensor_gain_base = read_OV5642YUV_gain();
    
    OV5642YUV_g_iBackupExtraExp = 0;
    atomic_set(&OV5642_SetGain_Flag, 0); 
    atomic_set(&OV5642_SetShutter_Flag, 0); 
    init_waitqueue_head(&OV5642_SetGain_waitQueue);
    init_waitqueue_head(&OV5642_SetShutter_waitQueue); 
    return ERROR_NONE;
}



/*************************************************************************
* FUNCTION
*   OV5642YUV_SetShutter
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
void OV5642YUV_SetShutter(kal_uint16 iShutter)
{
#if 0 
    if (iShutter < 4 )
        iShutter = 4;
#else 
    if (iShutter < 1)
        iShutter = 1; 
#endif     

    OV5642YUV_pv_exposure_lines = iShutter;

    //OV5642YUV_imgSensorProfileStart();
    OV5642YUV_write_shutter(iShutter);
    //OV5642YUV_imgSensorProfileEnd("OV5642YUV_SetShutter"); 
    //SENSORDB("iShutter = %d\n", iShutter);

}   /*  OV5642YUV_SetShutter   */



/*************************************************************************
* FUNCTION
*   OV5642YUV_read_shutter
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
UINT16 OV5642YUV_read_shutter(void)
{
    kal_uint8 temp_reg1, temp_reg2, temp_reg3;
    kal_uint16 temp_reg;
    temp_reg1 = OV5642YUV_read_cmos_sensor(0x3500);
    temp_reg2 = OV5642YUV_read_cmos_sensor(0x3501);
    temp_reg3 = OV5642YUV_read_cmos_sensor(0x3502);

   // SENSORDB("ov5642read shutter 0x3500=0x%x,0x3501=0x%x,0x3502=0x%x\n",
	//	temp_reg1,temp_reg2,temp_reg3);
    temp_reg = ((temp_reg1<<12) & 0xF000) | ((temp_reg2<<4) & 0x0FF0) | ((temp_reg3>>4) & 0x0F);

    //SENSORDB("ov5642read shutter = 0x%x\n", temp_reg);
	
    return (UINT16)temp_reg;
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
void OV5642YUV_NightMode(kal_bool bEnable)
{
    /************************************************************************/
    /*                      Auto Mode: 30fps                                                                                          */
    /*                      Night Mode:15fps                                                                                          */
    /************************************************************************/
    if(bEnable)
    {
        if(OV5642YUV_MPEG4_encode_mode==KAL_TRUE)
        {
            OV5642YUV_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642YUV_sensor_pclk/15)/(OV5642_PV_PERIOD_PIXEL_NUMS+OV5642YUV_PV_dummy_pixels));
            OV5642YUV_write_cmos_sensor(0x350C, (OV5642YUV_MAX_EXPOSURE_LINES >> 8) & 0xFF);
            OV5642YUV_write_cmos_sensor(0x350D, OV5642YUV_MAX_EXPOSURE_LINES & 0xFF);
            OV5642YUV_CURRENT_FRAME_LINES = OV5642YUV_MAX_EXPOSURE_LINES;
            OV5642YUV_MAX_EXPOSURE_LINES = OV5642YUV_CURRENT_FRAME_LINES - OV5642_SHUTTER_LINES_GAP;
        }
    }
    else// Fix video framerate 30 fps
    {
        if(OV5642YUV_MPEG4_encode_mode==KAL_TRUE)
        {
            OV5642YUV_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642YUV_sensor_pclk/30)/(OV5642_PV_PERIOD_PIXEL_NUMS+OV5642YUV_PV_dummy_pixels));
            if(OV5642YUV_pv_exposure_lines < (OV5642YUV_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                OV5642YUV_write_cmos_sensor(0x350C, (OV5642YUV_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                OV5642YUV_write_cmos_sensor(0x350D, OV5642YUV_MAX_EXPOSURE_LINES & 0xFF);
                OV5642YUV_CURRENT_FRAME_LINES = OV5642YUV_MAX_EXPOSURE_LINES;
            }
            OV5642YUV_MAX_EXPOSURE_LINES = OV5642YUV_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP;
        }
    }
}/*	OV5642YUV_NightMode */



/*************************************************************************
* FUNCTION
*   OV5642YUVClose
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
UINT32 OV5642YUVClose(void)
{
    //  CISModulePowerOn(FALSE);

    //s_porting
    //  DRV_I2CClose(OV5642YUVhDrvI2C);
    //e_porting
    return ERROR_NONE;
}	/* OV5642YUVClose() */

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


UINT32 OV5642_FOCUS_AD5820_Init(void)
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

    OV5642YUV_imgSensorProfileStart();

#if 0
    OV5642YUV_write_cmos_sensor(0x3000, 0x20);

    totalCnt = ARRAY_SIZE(AD5820_Config); 


    SENSORDB("Total Count = %d\n", totalCnt); 
    while (index < totalCnt) {        
        sentCnt = totalCnt - index > len  ? len  : totalCnt - index; 
        //SENSORDB("Index = %d, sent Cnt = %d, addr = 0x%x\n", index, sentCnt, addr); 
        buf[0] = addr >> 8; 
        buf[1] = addr & 0xff; 
        memcpy(&buf[2], &AD5820_Config[index], len );
        OV5642YUV_burst_write_cmos_sensor(buf, sentCnt + 2); 
        addr += len ; 
        index += len ;
    }
    //kfree(buf); 
    OV5642YUV_write_cmos_sensor(0x3024, 0x00);
    OV5642YUV_write_cmos_sensor(0x3025, 0x00);
    OV5642YUV_write_cmos_sensor(0x5082, 0x00);
    OV5642YUV_write_cmos_sensor(0x5083, 0x00);
    OV5642YUV_write_cmos_sensor(0x5084, 0x00);
    OV5642YUV_write_cmos_sensor(0x5085, 0x00);
    OV5642YUV_write_cmos_sensor(0x3026, 0x00);
    OV5642YUV_write_cmos_sensor(0x3027, 0xFF);
    OV5642YUV_write_cmos_sensor(0x3000, 0x00);
#if 0
    for (i = 0x8000; i < 0x8000 + totalCnt; i++) {
        SENSORDB("Addr = 0x%x, data = 0x%02x\n", i, OV5642YUV_read_cmos_sensor(i)); 
    }
#endif    
#else
    OV5642YUV_write_cmos_sensor(0x3000, 0x20);
    OV5642YUV_write_cmos_sensor(0x8000, 0x02);
    OV5642YUV_write_cmos_sensor(0x8001, 0x12);
    OV5642YUV_write_cmos_sensor(0x8002, 0x55);
    OV5642YUV_write_cmos_sensor(0x8003, 0x02);
    OV5642YUV_write_cmos_sensor(0x8004, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8005, 0xb1);
    OV5642YUV_write_cmos_sensor(0x8006, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8007, 0x01);
    OV5642YUV_write_cmos_sensor(0x8008, 0x22);
    OV5642YUV_write_cmos_sensor(0x8009, 0x22);
    OV5642YUV_write_cmos_sensor(0x800a, 0x00);
    OV5642YUV_write_cmos_sensor(0x800b, 0x02);
    OV5642YUV_write_cmos_sensor(0x800c, 0x12);
    OV5642YUV_write_cmos_sensor(0x800d, 0x32);
    OV5642YUV_write_cmos_sensor(0x800e, 0x30);
    OV5642YUV_write_cmos_sensor(0x800f, 0x01);
    OV5642YUV_write_cmos_sensor(0x8010, 0x03);
    OV5642YUV_write_cmos_sensor(0x8011, 0x02);
    OV5642YUV_write_cmos_sensor(0x8012, 0x02);
    OV5642YUV_write_cmos_sensor(0x8013, 0xdf);
    OV5642YUV_write_cmos_sensor(0x8014, 0x30);
    OV5642YUV_write_cmos_sensor(0x8015, 0x02);
    OV5642YUV_write_cmos_sensor(0x8016, 0x03);
    OV5642YUV_write_cmos_sensor(0x8017, 0x02);
    OV5642YUV_write_cmos_sensor(0x8018, 0x02);
    OV5642YUV_write_cmos_sensor(0x8019, 0xdf);
    OV5642YUV_write_cmos_sensor(0x801a, 0x90);
    OV5642YUV_write_cmos_sensor(0x801b, 0x51);
    OV5642YUV_write_cmos_sensor(0x801c, 0xa5);
    OV5642YUV_write_cmos_sensor(0x801d, 0xe0);
    OV5642YUV_write_cmos_sensor(0x801e, 0x78);
    OV5642YUV_write_cmos_sensor(0x801f, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8020, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8021, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8022, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8023, 0x08);
    OV5642YUV_write_cmos_sensor(0x8024, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8025, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8026, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8027, 0x08);
    OV5642YUV_write_cmos_sensor(0x8028, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8029, 0xe5);
    OV5642YUV_write_cmos_sensor(0x802a, 0x1f);
    OV5642YUV_write_cmos_sensor(0x802b, 0x70);
    OV5642YUV_write_cmos_sensor(0x802c, 0x45);
    OV5642YUV_write_cmos_sensor(0x802d, 0x75);
    OV5642YUV_write_cmos_sensor(0x802e, 0x1e);
    OV5642YUV_write_cmos_sensor(0x802f, 0x20);
    OV5642YUV_write_cmos_sensor(0x8030, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8031, 0x35);
    OV5642YUV_write_cmos_sensor(0x8032, 0x12);
    OV5642YUV_write_cmos_sensor(0x8033, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8034, 0xcb);
    OV5642YUV_write_cmos_sensor(0x8035, 0x78);
    OV5642YUV_write_cmos_sensor(0x8036, 0xa1);
    OV5642YUV_write_cmos_sensor(0x8037, 0x12);
    OV5642YUV_write_cmos_sensor(0x8038, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8039, 0x91);
    OV5642YUV_write_cmos_sensor(0x803a, 0x78);
    OV5642YUV_write_cmos_sensor(0x803b, 0xad);
    OV5642YUV_write_cmos_sensor(0x803c, 0xa6);
    OV5642YUV_write_cmos_sensor(0x803d, 0x14);
    OV5642YUV_write_cmos_sensor(0x803e, 0x08);
    OV5642YUV_write_cmos_sensor(0x803f, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8040, 0x15);
    OV5642YUV_write_cmos_sensor(0x8041, 0x78);
    OV5642YUV_write_cmos_sensor(0x8042, 0xb8);
    OV5642YUV_write_cmos_sensor(0x8043, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8044, 0x09);
    OV5642YUV_write_cmos_sensor(0x8045, 0x18);
    OV5642YUV_write_cmos_sensor(0x8046, 0x76);
    OV5642YUV_write_cmos_sensor(0x8047, 0x01);
    OV5642YUV_write_cmos_sensor(0x8048, 0x78);
    OV5642YUV_write_cmos_sensor(0x8049, 0x51);
    OV5642YUV_write_cmos_sensor(0x804a, 0xa6);
    OV5642YUV_write_cmos_sensor(0x804b, 0x0a);
    OV5642YUV_write_cmos_sensor(0x804c, 0x08);
    OV5642YUV_write_cmos_sensor(0x804d, 0xa6);
    OV5642YUV_write_cmos_sensor(0x804e, 0x0b);
    OV5642YUV_write_cmos_sensor(0x804f, 0x78);
    OV5642YUV_write_cmos_sensor(0x8050, 0x71);
    OV5642YUV_write_cmos_sensor(0x8051, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8052, 0x14);
    OV5642YUV_write_cmos_sensor(0x8053, 0x08);
    OV5642YUV_write_cmos_sensor(0x8054, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8055, 0x15);
    OV5642YUV_write_cmos_sensor(0x8056, 0x78);
    OV5642YUV_write_cmos_sensor(0x8057, 0xb8);
    OV5642YUV_write_cmos_sensor(0x8058, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8059, 0x78);
    OV5642YUV_write_cmos_sensor(0x805a, 0x91);
    OV5642YUV_write_cmos_sensor(0x805b, 0xf6);
    OV5642YUV_write_cmos_sensor(0x805c, 0x75);
    OV5642YUV_write_cmos_sensor(0x805d, 0x1f);
    OV5642YUV_write_cmos_sensor(0x805e, 0x01);
    OV5642YUV_write_cmos_sensor(0x805f, 0x78);
    OV5642YUV_write_cmos_sensor(0x8060, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8061, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8062, 0x78);
    OV5642YUV_write_cmos_sensor(0x8063, 0xbd);
    OV5642YUV_write_cmos_sensor(0x8064, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8065, 0x78);
    OV5642YUV_write_cmos_sensor(0x8066, 0xc1);
    OV5642YUV_write_cmos_sensor(0x8067, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8068, 0x78);
    OV5642YUV_write_cmos_sensor(0x8069, 0xbe);
    OV5642YUV_write_cmos_sensor(0x806a, 0xf6);
    OV5642YUV_write_cmos_sensor(0x806b, 0x78);
    OV5642YUV_write_cmos_sensor(0x806c, 0xc2);
    OV5642YUV_write_cmos_sensor(0x806d, 0xe6);
    OV5642YUV_write_cmos_sensor(0x806e, 0x78);
    OV5642YUV_write_cmos_sensor(0x806f, 0xbf);
    OV5642YUV_write_cmos_sensor(0x8070, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8071, 0x22);
    OV5642YUV_write_cmos_sensor(0x8072, 0x79);
    OV5642YUV_write_cmos_sensor(0x8073, 0xbd);
    OV5642YUV_write_cmos_sensor(0x8074, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8075, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8076, 0x78);
    OV5642YUV_write_cmos_sensor(0x8077, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8078, 0x96);
    OV5642YUV_write_cmos_sensor(0x8079, 0x40);
    OV5642YUV_write_cmos_sensor(0x807a, 0x05);
    OV5642YUV_write_cmos_sensor(0x807b, 0xe7);
    OV5642YUV_write_cmos_sensor(0x807c, 0x96);
    OV5642YUV_write_cmos_sensor(0x807d, 0xff);
    OV5642YUV_write_cmos_sensor(0x807e, 0x80);
    OV5642YUV_write_cmos_sensor(0x807f, 0x08);
    OV5642YUV_write_cmos_sensor(0x8080, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8081, 0x79);
    OV5642YUV_write_cmos_sensor(0x8082, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8083, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8084, 0x78);
    OV5642YUV_write_cmos_sensor(0x8085, 0xbd);
    OV5642YUV_write_cmos_sensor(0x8086, 0x96);
    OV5642YUV_write_cmos_sensor(0x8087, 0xff);
    OV5642YUV_write_cmos_sensor(0x8088, 0x78);
    OV5642YUV_write_cmos_sensor(0x8089, 0xab);
    OV5642YUV_write_cmos_sensor(0x808a, 0x76);
    OV5642YUV_write_cmos_sensor(0x808b, 0x00);
    OV5642YUV_write_cmos_sensor(0x808c, 0x08);
    OV5642YUV_write_cmos_sensor(0x808d, 0xa6);
    OV5642YUV_write_cmos_sensor(0x808e, 0x07);
    OV5642YUV_write_cmos_sensor(0x808f, 0x79);
    OV5642YUV_write_cmos_sensor(0x8090, 0xbe);
    OV5642YUV_write_cmos_sensor(0x8091, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8092, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8093, 0x78);
    OV5642YUV_write_cmos_sensor(0x8094, 0xc1);
    OV5642YUV_write_cmos_sensor(0x8095, 0x96);
    OV5642YUV_write_cmos_sensor(0x8096, 0x40);
    OV5642YUV_write_cmos_sensor(0x8097, 0x05);
    OV5642YUV_write_cmos_sensor(0x8098, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8099, 0x96);
    OV5642YUV_write_cmos_sensor(0x809a, 0xff);
    OV5642YUV_write_cmos_sensor(0x809b, 0x80);
    OV5642YUV_write_cmos_sensor(0x809c, 0x08);
    OV5642YUV_write_cmos_sensor(0x809d, 0xc3);
    OV5642YUV_write_cmos_sensor(0x809e, 0x79);
    OV5642YUV_write_cmos_sensor(0x809f, 0xc1);
    OV5642YUV_write_cmos_sensor(0x80a0, 0xe7);
    OV5642YUV_write_cmos_sensor(0x80a1, 0x78);
    OV5642YUV_write_cmos_sensor(0x80a2, 0xbe);
    OV5642YUV_write_cmos_sensor(0x80a3, 0x96);
    OV5642YUV_write_cmos_sensor(0x80a4, 0xff);
    OV5642YUV_write_cmos_sensor(0x80a5, 0x12);
    OV5642YUV_write_cmos_sensor(0x80a6, 0x0f);
    OV5642YUV_write_cmos_sensor(0x80a7, 0xd2);
    OV5642YUV_write_cmos_sensor(0x80a8, 0x79);
    OV5642YUV_write_cmos_sensor(0x80a9, 0xbf);
    OV5642YUV_write_cmos_sensor(0x80aa, 0xe7);
    OV5642YUV_write_cmos_sensor(0x80ab, 0xd3);
    OV5642YUV_write_cmos_sensor(0x80ac, 0x78);
    OV5642YUV_write_cmos_sensor(0x80ad, 0xc2);
    OV5642YUV_write_cmos_sensor(0x80ae, 0x96);
    OV5642YUV_write_cmos_sensor(0x80af, 0x40);
    OV5642YUV_write_cmos_sensor(0x80b0, 0x05);
    OV5642YUV_write_cmos_sensor(0x80b1, 0xe7);
    OV5642YUV_write_cmos_sensor(0x80b2, 0x96);
    OV5642YUV_write_cmos_sensor(0x80b3, 0xff);
    OV5642YUV_write_cmos_sensor(0x80b4, 0x80);
    OV5642YUV_write_cmos_sensor(0x80b5, 0x08);
    OV5642YUV_write_cmos_sensor(0x80b6, 0xc3);
    OV5642YUV_write_cmos_sensor(0x80b7, 0x79);
    OV5642YUV_write_cmos_sensor(0x80b8, 0xc2);
    OV5642YUV_write_cmos_sensor(0x80b9, 0xe7);
    OV5642YUV_write_cmos_sensor(0x80ba, 0x78);
    OV5642YUV_write_cmos_sensor(0x80bb, 0xbf);
    OV5642YUV_write_cmos_sensor(0x80bc, 0x96);
    OV5642YUV_write_cmos_sensor(0x80bd, 0xff);
    OV5642YUV_write_cmos_sensor(0x80be, 0x12);
    OV5642YUV_write_cmos_sensor(0x80bf, 0x0f);
    OV5642YUV_write_cmos_sensor(0x80c0, 0xd2);
    OV5642YUV_write_cmos_sensor(0x80c1, 0x78);
    OV5642YUV_write_cmos_sensor(0x80c2, 0xb7);
    OV5642YUV_write_cmos_sensor(0x80c3, 0xe6);
    OV5642YUV_write_cmos_sensor(0x80c4, 0x25);
    OV5642YUV_write_cmos_sensor(0x80c5, 0xe0);
    OV5642YUV_write_cmos_sensor(0x80c6, 0x24);
    OV5642YUV_write_cmos_sensor(0x80c7, 0x51);
    OV5642YUV_write_cmos_sensor(0x80c8, 0xf8);
    OV5642YUV_write_cmos_sensor(0x80c9, 0xa6);
    OV5642YUV_write_cmos_sensor(0x80ca, 0x0a);
    OV5642YUV_write_cmos_sensor(0x80cb, 0x08);
    OV5642YUV_write_cmos_sensor(0x80cc, 0xa6);
    OV5642YUV_write_cmos_sensor(0x80cd, 0x0b);
    OV5642YUV_write_cmos_sensor(0x80ce, 0x78);
    OV5642YUV_write_cmos_sensor(0x80cf, 0xb7);
    OV5642YUV_write_cmos_sensor(0x80d0, 0xe6);
    OV5642YUV_write_cmos_sensor(0x80d1, 0x25);
    OV5642YUV_write_cmos_sensor(0x80d2, 0xe0);
    OV5642YUV_write_cmos_sensor(0x80d3, 0x24);
    OV5642YUV_write_cmos_sensor(0x80d4, 0x71);
    OV5642YUV_write_cmos_sensor(0x80d5, 0xf8);
    OV5642YUV_write_cmos_sensor(0x80d6, 0xa6);
    OV5642YUV_write_cmos_sensor(0x80d7, 0x14);
    OV5642YUV_write_cmos_sensor(0x80d8, 0x08);
    OV5642YUV_write_cmos_sensor(0x80d9, 0xa6);
    OV5642YUV_write_cmos_sensor(0x80da, 0x15);
    OV5642YUV_write_cmos_sensor(0x80db, 0x78);
    OV5642YUV_write_cmos_sensor(0x80dc, 0xb7);
    OV5642YUV_write_cmos_sensor(0x80dd, 0xe6);
    OV5642YUV_write_cmos_sensor(0x80de, 0x24);
    OV5642YUV_write_cmos_sensor(0x80df, 0x91);
    OV5642YUV_write_cmos_sensor(0x80e0, 0xf8);
    OV5642YUV_write_cmos_sensor(0x80e1, 0xa6);
    OV5642YUV_write_cmos_sensor(0x80e2, 0x09);
    OV5642YUV_write_cmos_sensor(0x80e3, 0x78);
    OV5642YUV_write_cmos_sensor(0x80e4, 0xb7);
    OV5642YUV_write_cmos_sensor(0x80e5, 0xe6);
    OV5642YUV_write_cmos_sensor(0x80e6, 0x24);
    OV5642YUV_write_cmos_sensor(0x80e7, 0x01);
    OV5642YUV_write_cmos_sensor(0x80e8, 0xff);
    OV5642YUV_write_cmos_sensor(0x80e9, 0xe4);
    OV5642YUV_write_cmos_sensor(0x80ea, 0x33);
    OV5642YUV_write_cmos_sensor(0x80eb, 0xfe);
    OV5642YUV_write_cmos_sensor(0x80ec, 0xd3);
    OV5642YUV_write_cmos_sensor(0x80ed, 0xef);
    OV5642YUV_write_cmos_sensor(0x80ee, 0x94);
    OV5642YUV_write_cmos_sensor(0x80ef, 0x0f);
    OV5642YUV_write_cmos_sensor(0x80f0, 0xee);
    OV5642YUV_write_cmos_sensor(0x80f1, 0x64);
    OV5642YUV_write_cmos_sensor(0x80f2, 0x80);
    OV5642YUV_write_cmos_sensor(0x80f3, 0x94);
    OV5642YUV_write_cmos_sensor(0x80f4, 0x80);
    OV5642YUV_write_cmos_sensor(0x80f5, 0x40);
    OV5642YUV_write_cmos_sensor(0x80f6, 0x04);
    OV5642YUV_write_cmos_sensor(0x80f7, 0x7f);
    OV5642YUV_write_cmos_sensor(0x80f8, 0x00);
    OV5642YUV_write_cmos_sensor(0x80f9, 0x80);
    OV5642YUV_write_cmos_sensor(0x80fa, 0x05);
    OV5642YUV_write_cmos_sensor(0x80fb, 0x78);
    OV5642YUV_write_cmos_sensor(0x80fc, 0xb7);
    OV5642YUV_write_cmos_sensor(0x80fd, 0xe6);
    OV5642YUV_write_cmos_sensor(0x80fe, 0x04);
    OV5642YUV_write_cmos_sensor(0x80ff, 0xff);
    OV5642YUV_write_cmos_sensor(0x8100, 0x78);
    OV5642YUV_write_cmos_sensor(0x8101, 0xb7);
    OV5642YUV_write_cmos_sensor(0x8102, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8103, 0x07);
    OV5642YUV_write_cmos_sensor(0x8104, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8105, 0x1f);
    OV5642YUV_write_cmos_sensor(0x8106, 0xb4);
    OV5642YUV_write_cmos_sensor(0x8107, 0x01);
    OV5642YUV_write_cmos_sensor(0x8108, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8109, 0xe6);
    OV5642YUV_write_cmos_sensor(0x810a, 0x60);
    OV5642YUV_write_cmos_sensor(0x810b, 0x03);
    OV5642YUV_write_cmos_sensor(0x810c, 0x02);
    OV5642YUV_write_cmos_sensor(0x810d, 0x02);
    OV5642YUV_write_cmos_sensor(0x810e, 0xdf);
    OV5642YUV_write_cmos_sensor(0x810f, 0x75);
    OV5642YUV_write_cmos_sensor(0x8110, 0x1f);
    OV5642YUV_write_cmos_sensor(0x8111, 0x02);
    OV5642YUV_write_cmos_sensor(0x8112, 0x22);
    OV5642YUV_write_cmos_sensor(0x8113, 0x12);
    OV5642YUV_write_cmos_sensor(0x8114, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8115, 0xcb);
    OV5642YUV_write_cmos_sensor(0x8116, 0x78);
    OV5642YUV_write_cmos_sensor(0x8117, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8118, 0x12);
    OV5642YUV_write_cmos_sensor(0x8119, 0x0f);
    OV5642YUV_write_cmos_sensor(0x811a, 0x91);
    OV5642YUV_write_cmos_sensor(0x811b, 0x12);
    OV5642YUV_write_cmos_sensor(0x811c, 0x0f);
    OV5642YUV_write_cmos_sensor(0x811d, 0xcb);
    OV5642YUV_write_cmos_sensor(0x811e, 0x78);
    OV5642YUV_write_cmos_sensor(0x811f, 0xa5);
    OV5642YUV_write_cmos_sensor(0x8120, 0x12);
    OV5642YUV_write_cmos_sensor(0x8121, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8122, 0xbe);
    OV5642YUV_write_cmos_sensor(0x8123, 0x78);
    OV5642YUV_write_cmos_sensor(0x8124, 0xaf);
    OV5642YUV_write_cmos_sensor(0x8125, 0x12);
    OV5642YUV_write_cmos_sensor(0x8126, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8127, 0xbe);
    OV5642YUV_write_cmos_sensor(0x8128, 0xff);
    OV5642YUV_write_cmos_sensor(0x8129, 0x78);
    OV5642YUV_write_cmos_sensor(0x812a, 0xb1);
    OV5642YUV_write_cmos_sensor(0x812b, 0xa6);
    OV5642YUV_write_cmos_sensor(0x812c, 0x06);
    OV5642YUV_write_cmos_sensor(0x812d, 0x08);
    OV5642YUV_write_cmos_sensor(0x812e, 0xa6);
    OV5642YUV_write_cmos_sensor(0x812f, 0x07);
    OV5642YUV_write_cmos_sensor(0x8130, 0x78);
    OV5642YUV_write_cmos_sensor(0x8131, 0x91);
    OV5642YUV_write_cmos_sensor(0x8132, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8133, 0x78);
    OV5642YUV_write_cmos_sensor(0x8134, 0xb9);
    OV5642YUV_write_cmos_sensor(0x8135, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8136, 0x78);
    OV5642YUV_write_cmos_sensor(0x8137, 0x91);
    OV5642YUV_write_cmos_sensor(0x8138, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8139, 0x78);
    OV5642YUV_write_cmos_sensor(0x813a, 0xba);
    OV5642YUV_write_cmos_sensor(0x813b, 0xf6);
    OV5642YUV_write_cmos_sensor(0x813c, 0x7f);
    OV5642YUV_write_cmos_sensor(0x813d, 0x01);
    OV5642YUV_write_cmos_sensor(0x813e, 0xef);
    OV5642YUV_write_cmos_sensor(0x813f, 0x25);
    OV5642YUV_write_cmos_sensor(0x8140, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8141, 0x24);
    OV5642YUV_write_cmos_sensor(0x8142, 0x52);
    OV5642YUV_write_cmos_sensor(0x8143, 0x78);
    OV5642YUV_write_cmos_sensor(0x8144, 0xa4);
    OV5642YUV_write_cmos_sensor(0x8145, 0x12);
    OV5642YUV_write_cmos_sensor(0x8146, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8147, 0x88);
    OV5642YUV_write_cmos_sensor(0x8148, 0x50);
    OV5642YUV_write_cmos_sensor(0x8149, 0x0a);
    OV5642YUV_write_cmos_sensor(0x814a, 0x12);
    OV5642YUV_write_cmos_sensor(0x814b, 0x0f);
    OV5642YUV_write_cmos_sensor(0x814c, 0x6a);
    OV5642YUV_write_cmos_sensor(0x814d, 0x78);
    OV5642YUV_write_cmos_sensor(0x814e, 0xa3);
    OV5642YUV_write_cmos_sensor(0x814f, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8150, 0x04);
    OV5642YUV_write_cmos_sensor(0x8151, 0x08);
    OV5642YUV_write_cmos_sensor(0x8152, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8153, 0x05);
    OV5642YUV_write_cmos_sensor(0x8154, 0xef);
    OV5642YUV_write_cmos_sensor(0x8155, 0x25);
    OV5642YUV_write_cmos_sensor(0x8156, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8157, 0x24);
    OV5642YUV_write_cmos_sensor(0x8158, 0x72);
    OV5642YUV_write_cmos_sensor(0x8159, 0x78);
    OV5642YUV_write_cmos_sensor(0x815a, 0xb0);
    OV5642YUV_write_cmos_sensor(0x815b, 0x12);
    OV5642YUV_write_cmos_sensor(0x815c, 0x0f);
    OV5642YUV_write_cmos_sensor(0x815d, 0x88);
    OV5642YUV_write_cmos_sensor(0x815e, 0x50);
    OV5642YUV_write_cmos_sensor(0x815f, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8160, 0xef);
    OV5642YUV_write_cmos_sensor(0x8161, 0x25);
    OV5642YUV_write_cmos_sensor(0x8162, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8163, 0x24);
    OV5642YUV_write_cmos_sensor(0x8164, 0x71);
    OV5642YUV_write_cmos_sensor(0x8165, 0x12);
    OV5642YUV_write_cmos_sensor(0x8166, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8167, 0x6f);
    OV5642YUV_write_cmos_sensor(0x8168, 0x78);
    OV5642YUV_write_cmos_sensor(0x8169, 0xaf);
    OV5642YUV_write_cmos_sensor(0x816a, 0xa6);
    OV5642YUV_write_cmos_sensor(0x816b, 0x04);
    OV5642YUV_write_cmos_sensor(0x816c, 0x08);
    OV5642YUV_write_cmos_sensor(0x816d, 0xa6);
    OV5642YUV_write_cmos_sensor(0x816e, 0x05);
    OV5642YUV_write_cmos_sensor(0x816f, 0x74);
    OV5642YUV_write_cmos_sensor(0x8170, 0x91);
    OV5642YUV_write_cmos_sensor(0x8171, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8172, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8173, 0x78);
    OV5642YUV_write_cmos_sensor(0x8174, 0xb9);
    OV5642YUV_write_cmos_sensor(0x8175, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8176, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8177, 0x97);
    OV5642YUV_write_cmos_sensor(0x8178, 0x50);
    OV5642YUV_write_cmos_sensor(0x8179, 0x08);
    OV5642YUV_write_cmos_sensor(0x817a, 0x74);
    OV5642YUV_write_cmos_sensor(0x817b, 0x91);
    OV5642YUV_write_cmos_sensor(0x817c, 0x2f);
    OV5642YUV_write_cmos_sensor(0x817d, 0xf8);
    OV5642YUV_write_cmos_sensor(0x817e, 0xe6);
    OV5642YUV_write_cmos_sensor(0x817f, 0x78);
    OV5642YUV_write_cmos_sensor(0x8180, 0xb9);
    OV5642YUV_write_cmos_sensor(0x8181, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8182, 0xef);
    OV5642YUV_write_cmos_sensor(0x8183, 0x25);
    OV5642YUV_write_cmos_sensor(0x8184, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8185, 0x24);
    OV5642YUV_write_cmos_sensor(0x8186, 0x52);
    OV5642YUV_write_cmos_sensor(0x8187, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8188, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8189, 0x78);
    OV5642YUV_write_cmos_sensor(0x818a, 0xa6);
    OV5642YUV_write_cmos_sensor(0x818b, 0x12);
    OV5642YUV_write_cmos_sensor(0x818c, 0x0f);
    OV5642YUV_write_cmos_sensor(0x818d, 0x8a);
    OV5642YUV_write_cmos_sensor(0x818e, 0x40);
    OV5642YUV_write_cmos_sensor(0x818f, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8190, 0x12);
    OV5642YUV_write_cmos_sensor(0x8191, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8192, 0x6a);
    OV5642YUV_write_cmos_sensor(0x8193, 0x78);
    OV5642YUV_write_cmos_sensor(0x8194, 0xa5);
    OV5642YUV_write_cmos_sensor(0x8195, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8196, 0x04);
    OV5642YUV_write_cmos_sensor(0x8197, 0x08);
    OV5642YUV_write_cmos_sensor(0x8198, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8199, 0x05);
    OV5642YUV_write_cmos_sensor(0x819a, 0xef);
    OV5642YUV_write_cmos_sensor(0x819b, 0x25);
    OV5642YUV_write_cmos_sensor(0x819c, 0xe0);
    OV5642YUV_write_cmos_sensor(0x819d, 0x24);
    OV5642YUV_write_cmos_sensor(0x819e, 0x72);
    OV5642YUV_write_cmos_sensor(0x819f, 0xf9);
    OV5642YUV_write_cmos_sensor(0x81a0, 0xd3);
    OV5642YUV_write_cmos_sensor(0x81a1, 0x78);
    OV5642YUV_write_cmos_sensor(0x81a2, 0xb2);
    OV5642YUV_write_cmos_sensor(0x81a3, 0x12);
    OV5642YUV_write_cmos_sensor(0x81a4, 0x0f);
    OV5642YUV_write_cmos_sensor(0x81a5, 0x8a);
    OV5642YUV_write_cmos_sensor(0x81a6, 0x40);
    OV5642YUV_write_cmos_sensor(0x81a7, 0x0f);
    OV5642YUV_write_cmos_sensor(0x81a8, 0xef);
    OV5642YUV_write_cmos_sensor(0x81a9, 0x25);
    OV5642YUV_write_cmos_sensor(0x81aa, 0xe0);
    OV5642YUV_write_cmos_sensor(0x81ab, 0x24);
    OV5642YUV_write_cmos_sensor(0x81ac, 0x71);
    OV5642YUV_write_cmos_sensor(0x81ad, 0x12);
    OV5642YUV_write_cmos_sensor(0x81ae, 0x0f);
    OV5642YUV_write_cmos_sensor(0x81af, 0x6f);
    OV5642YUV_write_cmos_sensor(0x81b0, 0x78);
    OV5642YUV_write_cmos_sensor(0x81b1, 0xb1);
    OV5642YUV_write_cmos_sensor(0x81b2, 0xa6);
    OV5642YUV_write_cmos_sensor(0x81b3, 0x04);
    OV5642YUV_write_cmos_sensor(0x81b4, 0x08);
    OV5642YUV_write_cmos_sensor(0x81b5, 0xa6);
    OV5642YUV_write_cmos_sensor(0x81b6, 0x05);
    OV5642YUV_write_cmos_sensor(0x81b7, 0x74);
    OV5642YUV_write_cmos_sensor(0x81b8, 0x91);
    OV5642YUV_write_cmos_sensor(0x81b9, 0x2f);
    OV5642YUV_write_cmos_sensor(0x81ba, 0xf9);
    OV5642YUV_write_cmos_sensor(0x81bb, 0x78);
    OV5642YUV_write_cmos_sensor(0x81bc, 0xba);
    OV5642YUV_write_cmos_sensor(0x81bd, 0xe6);
    OV5642YUV_write_cmos_sensor(0x81be, 0xd3);
    OV5642YUV_write_cmos_sensor(0x81bf, 0x97);
    OV5642YUV_write_cmos_sensor(0x81c0, 0x40);
    OV5642YUV_write_cmos_sensor(0x81c1, 0x08);
    OV5642YUV_write_cmos_sensor(0x81c2, 0x74);
    OV5642YUV_write_cmos_sensor(0x81c3, 0x91);
    OV5642YUV_write_cmos_sensor(0x81c4, 0x2f);
    OV5642YUV_write_cmos_sensor(0x81c5, 0xf8);
    OV5642YUV_write_cmos_sensor(0x81c6, 0xe6);
    OV5642YUV_write_cmos_sensor(0x81c7, 0x78);
    OV5642YUV_write_cmos_sensor(0x81c8, 0xba);
    OV5642YUV_write_cmos_sensor(0x81c9, 0xf6);
    OV5642YUV_write_cmos_sensor(0x81ca, 0x0f);
    OV5642YUV_write_cmos_sensor(0x81cb, 0xef);
    OV5642YUV_write_cmos_sensor(0x81cc, 0x64);
    OV5642YUV_write_cmos_sensor(0x81cd, 0x10);
    OV5642YUV_write_cmos_sensor(0x81ce, 0x60);
    OV5642YUV_write_cmos_sensor(0x81cf, 0x03);
    OV5642YUV_write_cmos_sensor(0x81d0, 0x02);
    OV5642YUV_write_cmos_sensor(0x81d1, 0x01);
    OV5642YUV_write_cmos_sensor(0x81d2, 0x3e);
    OV5642YUV_write_cmos_sensor(0x81d3, 0xc3);
    OV5642YUV_write_cmos_sensor(0x81d4, 0x79);
    OV5642YUV_write_cmos_sensor(0x81d5, 0xa4);
    OV5642YUV_write_cmos_sensor(0x81d6, 0x78);
    OV5642YUV_write_cmos_sensor(0x81d7, 0xa6);
    OV5642YUV_write_cmos_sensor(0x81d8, 0x12);
    OV5642YUV_write_cmos_sensor(0x81d9, 0x0f);
    OV5642YUV_write_cmos_sensor(0x81da, 0xb6);
    OV5642YUV_write_cmos_sensor(0x81db, 0x78);
    OV5642YUV_write_cmos_sensor(0x81dc, 0xa7);
    OV5642YUV_write_cmos_sensor(0x81dd, 0xf6);
    OV5642YUV_write_cmos_sensor(0x81de, 0x08);
    OV5642YUV_write_cmos_sensor(0x81df, 0xa6);
    OV5642YUV_write_cmos_sensor(0x81e0, 0x07);
    OV5642YUV_write_cmos_sensor(0x81e1, 0xc3);
    OV5642YUV_write_cmos_sensor(0x81e2, 0x79);
    OV5642YUV_write_cmos_sensor(0x81e3, 0xb0);
    OV5642YUV_write_cmos_sensor(0x81e4, 0x78);
    OV5642YUV_write_cmos_sensor(0x81e5, 0xb2);
    OV5642YUV_write_cmos_sensor(0x81e6, 0x12);
    OV5642YUV_write_cmos_sensor(0x81e7, 0x0f);
    OV5642YUV_write_cmos_sensor(0x81e8, 0xb6);
    OV5642YUV_write_cmos_sensor(0x81e9, 0x78);
    OV5642YUV_write_cmos_sensor(0x81ea, 0xb3);
    OV5642YUV_write_cmos_sensor(0x81eb, 0xf6);
    OV5642YUV_write_cmos_sensor(0x81ec, 0x08);
    OV5642YUV_write_cmos_sensor(0x81ed, 0xa6);
    OV5642YUV_write_cmos_sensor(0x81ee, 0x07);
    OV5642YUV_write_cmos_sensor(0x81ef, 0xc3);
    OV5642YUV_write_cmos_sensor(0x81f0, 0x79);
    OV5642YUV_write_cmos_sensor(0x81f1, 0xb9);
    OV5642YUV_write_cmos_sensor(0x81f2, 0xe7);
    OV5642YUV_write_cmos_sensor(0x81f3, 0x78);
    OV5642YUV_write_cmos_sensor(0x81f4, 0xba);
    OV5642YUV_write_cmos_sensor(0x81f5, 0x96);
    OV5642YUV_write_cmos_sensor(0x81f6, 0x08);
    OV5642YUV_write_cmos_sensor(0x81f7, 0xf6);
    OV5642YUV_write_cmos_sensor(0x81f8, 0xd3);
    OV5642YUV_write_cmos_sensor(0x81f9, 0x79);
    OV5642YUV_write_cmos_sensor(0x81fa, 0xa4);
    OV5642YUV_write_cmos_sensor(0x81fb, 0xe7);
    OV5642YUV_write_cmos_sensor(0x81fc, 0x78);
    OV5642YUV_write_cmos_sensor(0x81fd, 0xa2);
    OV5642YUV_write_cmos_sensor(0x81fe, 0x96);
    OV5642YUV_write_cmos_sensor(0x81ff, 0x19);
    OV5642YUV_write_cmos_sensor(0x8200, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8201, 0x18);
    OV5642YUV_write_cmos_sensor(0x8202, 0x96);
    OV5642YUV_write_cmos_sensor(0x8203, 0x40);
    OV5642YUV_write_cmos_sensor(0x8204, 0x05);
    OV5642YUV_write_cmos_sensor(0x8205, 0x09);
    OV5642YUV_write_cmos_sensor(0x8206, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8207, 0x08);
    OV5642YUV_write_cmos_sensor(0x8208, 0x80);
    OV5642YUV_write_cmos_sensor(0x8209, 0x06);
    OV5642YUV_write_cmos_sensor(0x820a, 0xc3);
    OV5642YUV_write_cmos_sensor(0x820b, 0x79);
    OV5642YUV_write_cmos_sensor(0x820c, 0xa2);
    OV5642YUV_write_cmos_sensor(0x820d, 0xe7);
    OV5642YUV_write_cmos_sensor(0x820e, 0x78);
    OV5642YUV_write_cmos_sensor(0x820f, 0xa4);
    OV5642YUV_write_cmos_sensor(0x8210, 0x12);
    OV5642YUV_write_cmos_sensor(0x8211, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8212, 0xb7);
    OV5642YUV_write_cmos_sensor(0x8213, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8214, 0x78);
    OV5642YUV_write_cmos_sensor(0x8215, 0xa9);
    OV5642YUV_write_cmos_sensor(0x8216, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8217, 0x06);
    OV5642YUV_write_cmos_sensor(0x8218, 0x08);
    OV5642YUV_write_cmos_sensor(0x8219, 0xa6);
    OV5642YUV_write_cmos_sensor(0x821a, 0x07);
    OV5642YUV_write_cmos_sensor(0x821b, 0xd3);
    OV5642YUV_write_cmos_sensor(0x821c, 0x79);
    OV5642YUV_write_cmos_sensor(0x821d, 0xb0);
    OV5642YUV_write_cmos_sensor(0x821e, 0xe7);
    OV5642YUV_write_cmos_sensor(0x821f, 0x78);
    OV5642YUV_write_cmos_sensor(0x8220, 0xae);
    OV5642YUV_write_cmos_sensor(0x8221, 0x96);
    OV5642YUV_write_cmos_sensor(0x8222, 0x19);
    OV5642YUV_write_cmos_sensor(0x8223, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8224, 0x18);
    OV5642YUV_write_cmos_sensor(0x8225, 0x96);
    OV5642YUV_write_cmos_sensor(0x8226, 0x40);
    OV5642YUV_write_cmos_sensor(0x8227, 0x05);
    OV5642YUV_write_cmos_sensor(0x8228, 0x09);
    OV5642YUV_write_cmos_sensor(0x8229, 0xe7);
    OV5642YUV_write_cmos_sensor(0x822a, 0x08);
    OV5642YUV_write_cmos_sensor(0x822b, 0x80);
    OV5642YUV_write_cmos_sensor(0x822c, 0x06);
    OV5642YUV_write_cmos_sensor(0x822d, 0xc3);
    OV5642YUV_write_cmos_sensor(0x822e, 0x79);
    OV5642YUV_write_cmos_sensor(0x822f, 0xae);
    OV5642YUV_write_cmos_sensor(0x8230, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8231, 0x78);
    OV5642YUV_write_cmos_sensor(0x8232, 0xb0);
    OV5642YUV_write_cmos_sensor(0x8233, 0x12);
    OV5642YUV_write_cmos_sensor(0x8234, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8235, 0xb7);
    OV5642YUV_write_cmos_sensor(0x8236, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8237, 0x78);
    OV5642YUV_write_cmos_sensor(0x8238, 0xb5);
    OV5642YUV_write_cmos_sensor(0x8239, 0xa6);
    OV5642YUV_write_cmos_sensor(0x823a, 0x06);
    OV5642YUV_write_cmos_sensor(0x823b, 0x08);
    OV5642YUV_write_cmos_sensor(0x823c, 0xa6);
    OV5642YUV_write_cmos_sensor(0x823d, 0x07);
    OV5642YUV_write_cmos_sensor(0x823e, 0x79);
    OV5642YUV_write_cmos_sensor(0x823f, 0xb9);
    OV5642YUV_write_cmos_sensor(0x8240, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8241, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8242, 0x78);
    OV5642YUV_write_cmos_sensor(0x8243, 0xb8);
    OV5642YUV_write_cmos_sensor(0x8244, 0x96);
    OV5642YUV_write_cmos_sensor(0x8245, 0x40);
    OV5642YUV_write_cmos_sensor(0x8246, 0x05);
    OV5642YUV_write_cmos_sensor(0x8247, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8248, 0x96);
    OV5642YUV_write_cmos_sensor(0x8249, 0xff);
    OV5642YUV_write_cmos_sensor(0x824a, 0x80);
    OV5642YUV_write_cmos_sensor(0x824b, 0x08);
    OV5642YUV_write_cmos_sensor(0x824c, 0xc3);
    OV5642YUV_write_cmos_sensor(0x824d, 0x79);
    OV5642YUV_write_cmos_sensor(0x824e, 0xb8);
    OV5642YUV_write_cmos_sensor(0x824f, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8250, 0x78);
    OV5642YUV_write_cmos_sensor(0x8251, 0xb9);
    OV5642YUV_write_cmos_sensor(0x8252, 0x96);
    OV5642YUV_write_cmos_sensor(0x8253, 0xff);
    OV5642YUV_write_cmos_sensor(0x8254, 0x78);
    OV5642YUV_write_cmos_sensor(0x8255, 0xbc);
    OV5642YUV_write_cmos_sensor(0x8256, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8257, 0x07);
    OV5642YUV_write_cmos_sensor(0x8258, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8259, 0x1f);
    OV5642YUV_write_cmos_sensor(0x825a, 0x64);
    OV5642YUV_write_cmos_sensor(0x825b, 0x02);
    OV5642YUV_write_cmos_sensor(0x825c, 0x70);
    OV5642YUV_write_cmos_sensor(0x825d, 0x6d);
    OV5642YUV_write_cmos_sensor(0x825e, 0x90);
    OV5642YUV_write_cmos_sensor(0x825f, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8260, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8261, 0x93);
    OV5642YUV_write_cmos_sensor(0x8262, 0xff);
    OV5642YUV_write_cmos_sensor(0x8263, 0x18);
    OV5642YUV_write_cmos_sensor(0x8264, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8265, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8266, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8267, 0x50);
    OV5642YUV_write_cmos_sensor(0x8268, 0x76);
    OV5642YUV_write_cmos_sensor(0x8269, 0x78);
    OV5642YUV_write_cmos_sensor(0x826a, 0xa7);
    OV5642YUV_write_cmos_sensor(0x826b, 0x12);
    OV5642YUV_write_cmos_sensor(0x826c, 0x0f);
    OV5642YUV_write_cmos_sensor(0x826d, 0x98);
    OV5642YUV_write_cmos_sensor(0x826e, 0x12);
    OV5642YUV_write_cmos_sensor(0x826f, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8270, 0x61);
    OV5642YUV_write_cmos_sensor(0x8271, 0x90);
    OV5642YUV_write_cmos_sensor(0x8272, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8273, 0xa5);
    OV5642YUV_write_cmos_sensor(0x8274, 0x12);
    OV5642YUV_write_cmos_sensor(0x8275, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8276, 0x76);
    OV5642YUV_write_cmos_sensor(0x8277, 0x78);
    OV5642YUV_write_cmos_sensor(0x8278, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8279, 0x12);
    OV5642YUV_write_cmos_sensor(0x827a, 0x0f);
    OV5642YUV_write_cmos_sensor(0x827b, 0xa7);
    OV5642YUV_write_cmos_sensor(0x827c, 0x7b);
    OV5642YUV_write_cmos_sensor(0x827d, 0x04);
    OV5642YUV_write_cmos_sensor(0x827e, 0x12);
    OV5642YUV_write_cmos_sensor(0x827f, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8280, 0x4f);
    OV5642YUV_write_cmos_sensor(0x8281, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8282, 0x12);
    OV5642YUV_write_cmos_sensor(0x8283, 0x09);
    OV5642YUV_write_cmos_sensor(0x8284, 0x67);
    OV5642YUV_write_cmos_sensor(0x8285, 0x50);
    OV5642YUV_write_cmos_sensor(0x8286, 0x58);
    OV5642YUV_write_cmos_sensor(0x8287, 0x90);
    OV5642YUV_write_cmos_sensor(0x8288, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8289, 0xa9);
    OV5642YUV_write_cmos_sensor(0x828a, 0xe4);
    OV5642YUV_write_cmos_sensor(0x828b, 0x93);
    OV5642YUV_write_cmos_sensor(0x828c, 0xff);
    OV5642YUV_write_cmos_sensor(0x828d, 0x78);
    OV5642YUV_write_cmos_sensor(0x828e, 0xbc);
    OV5642YUV_write_cmos_sensor(0x828f, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8290, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8291, 0x40);
    OV5642YUV_write_cmos_sensor(0x8292, 0x02);
    OV5642YUV_write_cmos_sensor(0x8293, 0x80);
    OV5642YUV_write_cmos_sensor(0x8294, 0x11);
    OV5642YUV_write_cmos_sensor(0x8295, 0x90);
    OV5642YUV_write_cmos_sensor(0x8296, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8297, 0xa7);
    OV5642YUV_write_cmos_sensor(0x8298, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8299, 0x93);
    OV5642YUV_write_cmos_sensor(0x829a, 0xff);
    OV5642YUV_write_cmos_sensor(0x829b, 0xd3);
    OV5642YUV_write_cmos_sensor(0x829c, 0x78);
    OV5642YUV_write_cmos_sensor(0x829d, 0xac);
    OV5642YUV_write_cmos_sensor(0x829e, 0xe6);
    OV5642YUV_write_cmos_sensor(0x829f, 0x9f);
    OV5642YUV_write_cmos_sensor(0x82a0, 0x18);
    OV5642YUV_write_cmos_sensor(0x82a1, 0xe6);
    OV5642YUV_write_cmos_sensor(0x82a2, 0x94);
    OV5642YUV_write_cmos_sensor(0x82a3, 0x00);
    OV5642YUV_write_cmos_sensor(0x82a4, 0x40);
    OV5642YUV_write_cmos_sensor(0x82a5, 0x03);
    OV5642YUV_write_cmos_sensor(0x82a6, 0x75);
    OV5642YUV_write_cmos_sensor(0x82a7, 0x1f);
    OV5642YUV_write_cmos_sensor(0x82a8, 0x05);
    OV5642YUV_write_cmos_sensor(0x82a9, 0x78);
    OV5642YUV_write_cmos_sensor(0x82aa, 0xb3);
    OV5642YUV_write_cmos_sensor(0x82ab, 0x12);
    OV5642YUV_write_cmos_sensor(0x82ac, 0x0f);
    OV5642YUV_write_cmos_sensor(0x82ad, 0x98);
    OV5642YUV_write_cmos_sensor(0x82ae, 0x12);
    OV5642YUV_write_cmos_sensor(0x82af, 0x0f);
    OV5642YUV_write_cmos_sensor(0x82b0, 0x61);
    OV5642YUV_write_cmos_sensor(0x82b1, 0x90);
    OV5642YUV_write_cmos_sensor(0x82b2, 0x0e);
    OV5642YUV_write_cmos_sensor(0x82b3, 0xa6);
    OV5642YUV_write_cmos_sensor(0x82b4, 0x12);
    OV5642YUV_write_cmos_sensor(0x82b5, 0x0f);
    OV5642YUV_write_cmos_sensor(0x82b6, 0x76);
    OV5642YUV_write_cmos_sensor(0x82b7, 0x78);
    OV5642YUV_write_cmos_sensor(0x82b8, 0xad);
    OV5642YUV_write_cmos_sensor(0x82b9, 0x12);
    OV5642YUV_write_cmos_sensor(0x82ba, 0x0f);
    OV5642YUV_write_cmos_sensor(0x82bb, 0xa7);
    OV5642YUV_write_cmos_sensor(0x82bc, 0x7b);
    OV5642YUV_write_cmos_sensor(0x82bd, 0x40);
    OV5642YUV_write_cmos_sensor(0x82be, 0x12);
    OV5642YUV_write_cmos_sensor(0x82bf, 0x0f);
    OV5642YUV_write_cmos_sensor(0x82c0, 0x4f);
    OV5642YUV_write_cmos_sensor(0x82c1, 0xd3);
    OV5642YUV_write_cmos_sensor(0x82c2, 0x12);
    OV5642YUV_write_cmos_sensor(0x82c3, 0x09);
    OV5642YUV_write_cmos_sensor(0x82c4, 0x67);
    OV5642YUV_write_cmos_sensor(0x82c5, 0x40);
    OV5642YUV_write_cmos_sensor(0x82c6, 0x18);
    OV5642YUV_write_cmos_sensor(0x82c7, 0x75);
    OV5642YUV_write_cmos_sensor(0x82c8, 0x1f);
    OV5642YUV_write_cmos_sensor(0x82c9, 0x05);
    OV5642YUV_write_cmos_sensor(0x82ca, 0x22);
    OV5642YUV_write_cmos_sensor(0x82cb, 0xe5);
    OV5642YUV_write_cmos_sensor(0x82cc, 0x1f);
    OV5642YUV_write_cmos_sensor(0x82cd, 0xb4);
    OV5642YUV_write_cmos_sensor(0x82ce, 0x05);
    OV5642YUV_write_cmos_sensor(0x82cf, 0x0f);
    OV5642YUV_write_cmos_sensor(0x82d0, 0xd2);
    OV5642YUV_write_cmos_sensor(0x82d1, 0x01);
    OV5642YUV_write_cmos_sensor(0x82d2, 0xc2);
    OV5642YUV_write_cmos_sensor(0x82d3, 0x02);
    OV5642YUV_write_cmos_sensor(0x82d4, 0xe4);
    OV5642YUV_write_cmos_sensor(0x82d5, 0xf5);
    OV5642YUV_write_cmos_sensor(0x82d6, 0x1f);
    OV5642YUV_write_cmos_sensor(0x82d7, 0xf5);
    OV5642YUV_write_cmos_sensor(0x82d8, 0x1e);
    OV5642YUV_write_cmos_sensor(0x82d9, 0xd2);
    OV5642YUV_write_cmos_sensor(0x82da, 0x35);
    OV5642YUV_write_cmos_sensor(0x82db, 0xd2);
    OV5642YUV_write_cmos_sensor(0x82dc, 0x33);
    OV5642YUV_write_cmos_sensor(0x82dd, 0xd2);
    OV5642YUV_write_cmos_sensor(0x82de, 0x36);
    OV5642YUV_write_cmos_sensor(0x82df, 0x22);
    OV5642YUV_write_cmos_sensor(0x82e0, 0xa2);
    OV5642YUV_write_cmos_sensor(0x82e1, 0xaf);
    OV5642YUV_write_cmos_sensor(0x82e2, 0x92);
    OV5642YUV_write_cmos_sensor(0x82e3, 0x32);
    OV5642YUV_write_cmos_sensor(0x82e4, 0xc2);
    OV5642YUV_write_cmos_sensor(0x82e5, 0xaf);
    OV5642YUV_write_cmos_sensor(0x82e6, 0x90);
    OV5642YUV_write_cmos_sensor(0x82e7, 0x0e);
    OV5642YUV_write_cmos_sensor(0x82e8, 0x6d);
    OV5642YUV_write_cmos_sensor(0x82e9, 0xe4);
    OV5642YUV_write_cmos_sensor(0x82ea, 0x93);
    OV5642YUV_write_cmos_sensor(0x82eb, 0xfe);
    OV5642YUV_write_cmos_sensor(0x82ec, 0x74);
    OV5642YUV_write_cmos_sensor(0x82ed, 0x01);
    OV5642YUV_write_cmos_sensor(0x82ee, 0x93);
    OV5642YUV_write_cmos_sensor(0x82ef, 0xf5);
    OV5642YUV_write_cmos_sensor(0x82f0, 0x82);
    OV5642YUV_write_cmos_sensor(0x82f1, 0x8e);
    OV5642YUV_write_cmos_sensor(0x82f2, 0x83);
    OV5642YUV_write_cmos_sensor(0x82f3, 0xe0);
    OV5642YUV_write_cmos_sensor(0x82f4, 0xff);
    OV5642YUV_write_cmos_sensor(0x82f5, 0x12);
    OV5642YUV_write_cmos_sensor(0x82f6, 0x0c);
    OV5642YUV_write_cmos_sensor(0x82f7, 0xed);
    OV5642YUV_write_cmos_sensor(0x82f8, 0xfd);
    OV5642YUV_write_cmos_sensor(0x82f9, 0xf5);
    OV5642YUV_write_cmos_sensor(0x82fa, 0x82);
    OV5642YUV_write_cmos_sensor(0x82fb, 0x8c);
    OV5642YUV_write_cmos_sensor(0x82fc, 0x83);
    OV5642YUV_write_cmos_sensor(0x82fd, 0xe0);
    OV5642YUV_write_cmos_sensor(0x82fe, 0xfe);
    OV5642YUV_write_cmos_sensor(0x82ff, 0x90);
    OV5642YUV_write_cmos_sensor(0x8300, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8301, 0x72);
    OV5642YUV_write_cmos_sensor(0x8302, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8303, 0x93);
    OV5642YUV_write_cmos_sensor(0x8304, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8305, 0x12);
    OV5642YUV_write_cmos_sensor(0x8306, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8307, 0x1c);
    OV5642YUV_write_cmos_sensor(0x8308, 0x12);
    OV5642YUV_write_cmos_sensor(0x8309, 0x0d);
    OV5642YUV_write_cmos_sensor(0x830a, 0x66);
    OV5642YUV_write_cmos_sensor(0x830b, 0x90);
    OV5642YUV_write_cmos_sensor(0x830c, 0x0e);
    OV5642YUV_write_cmos_sensor(0x830d, 0x78);
    OV5642YUV_write_cmos_sensor(0x830e, 0xe4);
    OV5642YUV_write_cmos_sensor(0x830f, 0x93);
    OV5642YUV_write_cmos_sensor(0x8310, 0x90);
    OV5642YUV_write_cmos_sensor(0x8311, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8312, 0x7a);
    OV5642YUV_write_cmos_sensor(0x8313, 0x12);
    OV5642YUV_write_cmos_sensor(0x8314, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8315, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8316, 0x12);
    OV5642YUV_write_cmos_sensor(0x8317, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8318, 0x26);
    OV5642YUV_write_cmos_sensor(0x8319, 0x90);
    OV5642YUV_write_cmos_sensor(0x831a, 0x0e);
    OV5642YUV_write_cmos_sensor(0x831b, 0x6f);
    OV5642YUV_write_cmos_sensor(0x831c, 0xe4);
    OV5642YUV_write_cmos_sensor(0x831d, 0x93);
    OV5642YUV_write_cmos_sensor(0x831e, 0x90);
    OV5642YUV_write_cmos_sensor(0x831f, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8320, 0x71);
    OV5642YUV_write_cmos_sensor(0x8321, 0x12);
    OV5642YUV_write_cmos_sensor(0x8322, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8323, 0x01);
    OV5642YUV_write_cmos_sensor(0x8324, 0xef);
    OV5642YUV_write_cmos_sensor(0x8325, 0x12);
    OV5642YUV_write_cmos_sensor(0x8326, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8327, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8328, 0xef);
    OV5642YUV_write_cmos_sensor(0x8329, 0xf0);
    OV5642YUV_write_cmos_sensor(0x832a, 0xe4);
    OV5642YUV_write_cmos_sensor(0x832b, 0xf5);
    OV5642YUV_write_cmos_sensor(0x832c, 0x38);
    OV5642YUV_write_cmos_sensor(0x832d, 0x90);
    OV5642YUV_write_cmos_sensor(0x832e, 0x0e);
    OV5642YUV_write_cmos_sensor(0x832f, 0x78);
    OV5642YUV_write_cmos_sensor(0x8330, 0x93);
    OV5642YUV_write_cmos_sensor(0x8331, 0x52);
    OV5642YUV_write_cmos_sensor(0x8332, 0x06);
    OV5642YUV_write_cmos_sensor(0x8333, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8334, 0x12);
    OV5642YUV_write_cmos_sensor(0x8335, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8336, 0xd9);
    OV5642YUV_write_cmos_sensor(0x8337, 0x12);
    OV5642YUV_write_cmos_sensor(0x8338, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8339, 0x27);
    OV5642YUV_write_cmos_sensor(0x833a, 0x85);
    OV5642YUV_write_cmos_sensor(0x833b, 0x36);
    OV5642YUV_write_cmos_sensor(0x833c, 0x35);
    OV5642YUV_write_cmos_sensor(0x833d, 0xe5);
    OV5642YUV_write_cmos_sensor(0x833e, 0x23);
    OV5642YUV_write_cmos_sensor(0x833f, 0x45);
    OV5642YUV_write_cmos_sensor(0x8340, 0x22);
    OV5642YUV_write_cmos_sensor(0x8341, 0x60);
    OV5642YUV_write_cmos_sensor(0x8342, 0x02);
    OV5642YUV_write_cmos_sensor(0x8343, 0x05);
    OV5642YUV_write_cmos_sensor(0x8344, 0x35);
    OV5642YUV_write_cmos_sensor(0x8345, 0x75);
    OV5642YUV_write_cmos_sensor(0x8346, 0x33);
    OV5642YUV_write_cmos_sensor(0x8347, 0x09);
    OV5642YUV_write_cmos_sensor(0x8348, 0x90);
    OV5642YUV_write_cmos_sensor(0x8349, 0x0e);
    OV5642YUV_write_cmos_sensor(0x834a, 0x78);
    OV5642YUV_write_cmos_sensor(0x834b, 0xe4);
    OV5642YUV_write_cmos_sensor(0x834c, 0x93);
    OV5642YUV_write_cmos_sensor(0x834d, 0x52);
    OV5642YUV_write_cmos_sensor(0x834e, 0x06);
    OV5642YUV_write_cmos_sensor(0x834f, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8350, 0x35);
    OV5642YUV_write_cmos_sensor(0x8351, 0x30);
    OV5642YUV_write_cmos_sensor(0x8352, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8353, 0x05);
    OV5642YUV_write_cmos_sensor(0x8354, 0x90);
    OV5642YUV_write_cmos_sensor(0x8355, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8356, 0x7a);
    OV5642YUV_write_cmos_sensor(0x8357, 0x80);
    OV5642YUV_write_cmos_sensor(0x8358, 0x03);
    OV5642YUV_write_cmos_sensor(0x8359, 0x90);
    OV5642YUV_write_cmos_sensor(0x835a, 0x0e);
    OV5642YUV_write_cmos_sensor(0x835b, 0x79);
    OV5642YUV_write_cmos_sensor(0x835c, 0xe4);
    OV5642YUV_write_cmos_sensor(0x835d, 0x93);
    OV5642YUV_write_cmos_sensor(0x835e, 0x42);
    OV5642YUV_write_cmos_sensor(0x835f, 0x06);
    OV5642YUV_write_cmos_sensor(0x8360, 0x12);
    OV5642YUV_write_cmos_sensor(0x8361, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8362, 0xb8);
    OV5642YUV_write_cmos_sensor(0x8363, 0x12);
    OV5642YUV_write_cmos_sensor(0x8364, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8365, 0x27);
    OV5642YUV_write_cmos_sensor(0x8366, 0x15);
    OV5642YUV_write_cmos_sensor(0x8367, 0x33);
    OV5642YUV_write_cmos_sensor(0x8368, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8369, 0x33);
    OV5642YUV_write_cmos_sensor(0x836a, 0xb4);
    OV5642YUV_write_cmos_sensor(0x836b, 0x01);
    OV5642YUV_write_cmos_sensor(0x836c, 0xdb);
    OV5642YUV_write_cmos_sensor(0x836d, 0x12);
    OV5642YUV_write_cmos_sensor(0x836e, 0x0d);
    OV5642YUV_write_cmos_sensor(0x836f, 0x43);
    OV5642YUV_write_cmos_sensor(0x8370, 0x12);
    OV5642YUV_write_cmos_sensor(0x8371, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8372, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8373, 0x12);
    OV5642YUV_write_cmos_sensor(0x8374, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8375, 0x26);
    OV5642YUV_write_cmos_sensor(0x8376, 0x12);
    OV5642YUV_write_cmos_sensor(0x8377, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8378, 0x5e);
    OV5642YUV_write_cmos_sensor(0x8379, 0xa3);
    OV5642YUV_write_cmos_sensor(0x837a, 0x12);
    OV5642YUV_write_cmos_sensor(0x837b, 0x0d);
    OV5642YUV_write_cmos_sensor(0x837c, 0x03);
    OV5642YUV_write_cmos_sensor(0x837d, 0x12);
    OV5642YUV_write_cmos_sensor(0x837e, 0x0d);
    OV5642YUV_write_cmos_sensor(0x837f, 0x15);
    OV5642YUV_write_cmos_sensor(0x8380, 0x12);
    OV5642YUV_write_cmos_sensor(0x8381, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8382, 0x66);
    OV5642YUV_write_cmos_sensor(0x8383, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8384, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8385, 0x32);
    OV5642YUV_write_cmos_sensor(0x8386, 0x90);
    OV5642YUV_write_cmos_sensor(0x8387, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8388, 0x6b);
    OV5642YUV_write_cmos_sensor(0x8389, 0x12);
    OV5642YUV_write_cmos_sensor(0x838a, 0x0c);
    OV5642YUV_write_cmos_sensor(0x838b, 0xf0);
    OV5642YUV_write_cmos_sensor(0x838c, 0x12);
    OV5642YUV_write_cmos_sensor(0x838d, 0x0d);
    OV5642YUV_write_cmos_sensor(0x838e, 0x2e);
    OV5642YUV_write_cmos_sensor(0x838f, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8390, 0xb5);
    OV5642YUV_write_cmos_sensor(0x8391, 0x05);
    OV5642YUV_write_cmos_sensor(0x8392, 0x09);
    OV5642YUV_write_cmos_sensor(0x8393, 0x05);
    OV5642YUV_write_cmos_sensor(0x8394, 0x32);
    OV5642YUV_write_cmos_sensor(0x8395, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8396, 0x32);
    OV5642YUV_write_cmos_sensor(0x8397, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8398, 0x94);
    OV5642YUV_write_cmos_sensor(0x8399, 0xfa);
    OV5642YUV_write_cmos_sensor(0x839a, 0x40);
    OV5642YUV_write_cmos_sensor(0x839b, 0xea);
    OV5642YUV_write_cmos_sensor(0x839c, 0x12);
    OV5642YUV_write_cmos_sensor(0x839d, 0x0d);
    OV5642YUV_write_cmos_sensor(0x839e, 0x4e);
    OV5642YUV_write_cmos_sensor(0x839f, 0x12);
    OV5642YUV_write_cmos_sensor(0x83a0, 0x0d);
    OV5642YUV_write_cmos_sensor(0x83a1, 0x0a);
    OV5642YUV_write_cmos_sensor(0x83a2, 0xee);
    OV5642YUV_write_cmos_sensor(0x83a3, 0xf0);
    OV5642YUV_write_cmos_sensor(0x83a4, 0xed);
    OV5642YUV_write_cmos_sensor(0x83a5, 0xb5);
    OV5642YUV_write_cmos_sensor(0x83a6, 0x04);
    OV5642YUV_write_cmos_sensor(0x83a7, 0x0c);
    OV5642YUV_write_cmos_sensor(0x83a8, 0x75);
    OV5642YUV_write_cmos_sensor(0x83a9, 0x38);
    OV5642YUV_write_cmos_sensor(0x83aa, 0x01);
    OV5642YUV_write_cmos_sensor(0x83ab, 0xe5);
    OV5642YUV_write_cmos_sensor(0x83ac, 0x25);
    OV5642YUV_write_cmos_sensor(0x83ad, 0x45);
    OV5642YUV_write_cmos_sensor(0x83ae, 0x24);
    OV5642YUV_write_cmos_sensor(0x83af, 0x60);
    OV5642YUV_write_cmos_sensor(0x83b0, 0x03);
    OV5642YUV_write_cmos_sensor(0x83b1, 0x02);
    OV5642YUV_write_cmos_sensor(0x83b2, 0x05);
    OV5642YUV_write_cmos_sensor(0x83b3, 0x2f);
    OV5642YUV_write_cmos_sensor(0x83b4, 0xe5);
    OV5642YUV_write_cmos_sensor(0x83b5, 0x23);
    OV5642YUV_write_cmos_sensor(0x83b6, 0x45);
    OV5642YUV_write_cmos_sensor(0x83b7, 0x22);
    OV5642YUV_write_cmos_sensor(0x83b8, 0x60);
    OV5642YUV_write_cmos_sensor(0x83b9, 0x03);
    OV5642YUV_write_cmos_sensor(0x83ba, 0x02);
    OV5642YUV_write_cmos_sensor(0x83bb, 0x04);
    OV5642YUV_write_cmos_sensor(0x83bc, 0x73);
    OV5642YUV_write_cmos_sensor(0x83bd, 0xc3);
    OV5642YUV_write_cmos_sensor(0x83be, 0x74);
    OV5642YUV_write_cmos_sensor(0x83bf, 0x04);
    OV5642YUV_write_cmos_sensor(0x83c0, 0x95);
    OV5642YUV_write_cmos_sensor(0x83c1, 0x37);
    OV5642YUV_write_cmos_sensor(0x83c2, 0xf5);
    OV5642YUV_write_cmos_sensor(0x83c3, 0x34);
    OV5642YUV_write_cmos_sensor(0x83c4, 0xe5);
    OV5642YUV_write_cmos_sensor(0x83c5, 0x34);
    OV5642YUV_write_cmos_sensor(0x83c6, 0xc3);
    OV5642YUV_write_cmos_sensor(0x83c7, 0x94);
    OV5642YUV_write_cmos_sensor(0x83c8, 0x04);
    OV5642YUV_write_cmos_sensor(0x83c9, 0x40);
    OV5642YUV_write_cmos_sensor(0x83ca, 0x03);
    OV5642YUV_write_cmos_sensor(0x83cb, 0x02);
    OV5642YUV_write_cmos_sensor(0x83cc, 0x05);
    OV5642YUV_write_cmos_sensor(0x83cd, 0x2f);
    OV5642YUV_write_cmos_sensor(0x83ce, 0x12);
    OV5642YUV_write_cmos_sensor(0x83cf, 0x0d);
    OV5642YUV_write_cmos_sensor(0x83d0, 0x43);
    OV5642YUV_write_cmos_sensor(0x83d1, 0x12);
    OV5642YUV_write_cmos_sensor(0x83d2, 0x0c);
    OV5642YUV_write_cmos_sensor(0x83d3, 0xe9);
    OV5642YUV_write_cmos_sensor(0x83d4, 0x12);
    OV5642YUV_write_cmos_sensor(0x83d5, 0x0d);
    OV5642YUV_write_cmos_sensor(0x83d6, 0x27);
    OV5642YUV_write_cmos_sensor(0x83d7, 0x12);
    OV5642YUV_write_cmos_sensor(0x83d8, 0x0d);
    OV5642YUV_write_cmos_sensor(0x83d9, 0x5e);
    OV5642YUV_write_cmos_sensor(0x83da, 0x90);
    OV5642YUV_write_cmos_sensor(0x83db, 0x0e);
    OV5642YUV_write_cmos_sensor(0x83dc, 0x77);
    OV5642YUV_write_cmos_sensor(0x83dd, 0xe4);
    OV5642YUV_write_cmos_sensor(0x83de, 0x93);
    OV5642YUV_write_cmos_sensor(0x83df, 0x42);
    OV5642YUV_write_cmos_sensor(0x83e0, 0x07);
    OV5642YUV_write_cmos_sensor(0x83e1, 0x90);
    OV5642YUV_write_cmos_sensor(0x83e2, 0x0e);
    OV5642YUV_write_cmos_sensor(0x83e3, 0x6d);
    OV5642YUV_write_cmos_sensor(0x83e4, 0x12);
    OV5642YUV_write_cmos_sensor(0x83e5, 0x0c);
    OV5642YUV_write_cmos_sensor(0x83e6, 0xf0);
    OV5642YUV_write_cmos_sensor(0x83e7, 0xf5);
    OV5642YUV_write_cmos_sensor(0x83e8, 0x82);
    OV5642YUV_write_cmos_sensor(0x83e9, 0x8c);
    OV5642YUV_write_cmos_sensor(0x83ea, 0x83);
    OV5642YUV_write_cmos_sensor(0x83eb, 0xef);
    OV5642YUV_write_cmos_sensor(0x83ec, 0xf0);
    OV5642YUV_write_cmos_sensor(0x83ed, 0x7c);
    OV5642YUV_write_cmos_sensor(0x83ee, 0x00);
    OV5642YUV_write_cmos_sensor(0x83ef, 0x7b);
    OV5642YUV_write_cmos_sensor(0x83f0, 0x00);
    OV5642YUV_write_cmos_sensor(0x83f1, 0x74);
    OV5642YUV_write_cmos_sensor(0x83f2, 0x3d);
    OV5642YUV_write_cmos_sensor(0x83f3, 0x25);
    OV5642YUV_write_cmos_sensor(0x83f4, 0x34);
    OV5642YUV_write_cmos_sensor(0x83f5, 0xf9);
    OV5642YUV_write_cmos_sensor(0x83f6, 0xec);
    OV5642YUV_write_cmos_sensor(0x83f7, 0x34);
    OV5642YUV_write_cmos_sensor(0x83f8, 0x00);
    OV5642YUV_write_cmos_sensor(0x83f9, 0xfa);
    OV5642YUV_write_cmos_sensor(0x83fa, 0x12);
    OV5642YUV_write_cmos_sensor(0x83fb, 0x07);
    OV5642YUV_write_cmos_sensor(0x83fc, 0xb8);
    OV5642YUV_write_cmos_sensor(0x83fd, 0xf5);
    OV5642YUV_write_cmos_sensor(0x83fe, 0x35);
    OV5642YUV_write_cmos_sensor(0x83ff, 0x75);
    OV5642YUV_write_cmos_sensor(0x8400, 0x33);
    OV5642YUV_write_cmos_sensor(0x8401, 0x09);
    OV5642YUV_write_cmos_sensor(0x8402, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8403, 0x35);
    OV5642YUV_write_cmos_sensor(0x8404, 0x90);
    OV5642YUV_write_cmos_sensor(0x8405, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8406, 0x78);
    OV5642YUV_write_cmos_sensor(0x8407, 0x30);
    OV5642YUV_write_cmos_sensor(0x8408, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8409, 0x05);
    OV5642YUV_write_cmos_sensor(0x840a, 0x12);
    OV5642YUV_write_cmos_sensor(0x840b, 0x0d);
    OV5642YUV_write_cmos_sensor(0x840c, 0x46);
    OV5642YUV_write_cmos_sensor(0x840d, 0x80);
    OV5642YUV_write_cmos_sensor(0x840e, 0x05);
    OV5642YUV_write_cmos_sensor(0x840f, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8410, 0x93);
    OV5642YUV_write_cmos_sensor(0x8411, 0x52);
    OV5642YUV_write_cmos_sensor(0x8412, 0x06);
    OV5642YUV_write_cmos_sensor(0x8413, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8414, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8415, 0x93);
    OV5642YUV_write_cmos_sensor(0x8416, 0x42);
    OV5642YUV_write_cmos_sensor(0x8417, 0x06);
    OV5642YUV_write_cmos_sensor(0x8418, 0x12);
    OV5642YUV_write_cmos_sensor(0x8419, 0x0c);
    OV5642YUV_write_cmos_sensor(0x841a, 0xb8);
    OV5642YUV_write_cmos_sensor(0x841b, 0x12);
    OV5642YUV_write_cmos_sensor(0x841c, 0x0d);
    OV5642YUV_write_cmos_sensor(0x841d, 0x27);
    OV5642YUV_write_cmos_sensor(0x841e, 0x15);
    OV5642YUV_write_cmos_sensor(0x841f, 0x33);
    OV5642YUV_write_cmos_sensor(0x8420, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8421, 0x33);
    OV5642YUV_write_cmos_sensor(0x8422, 0xb4);
    OV5642YUV_write_cmos_sensor(0x8423, 0x01);
    OV5642YUV_write_cmos_sensor(0x8424, 0xdd);
    OV5642YUV_write_cmos_sensor(0x8425, 0x12);
    OV5642YUV_write_cmos_sensor(0x8426, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8427, 0x43);
    OV5642YUV_write_cmos_sensor(0x8428, 0x12);
    OV5642YUV_write_cmos_sensor(0x8429, 0x0c);
    OV5642YUV_write_cmos_sensor(0x842a, 0xe9);
    OV5642YUV_write_cmos_sensor(0x842b, 0x12);
    OV5642YUV_write_cmos_sensor(0x842c, 0x0d);
    OV5642YUV_write_cmos_sensor(0x842d, 0x26);
    OV5642YUV_write_cmos_sensor(0x842e, 0x12);
    OV5642YUV_write_cmos_sensor(0x842f, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8430, 0x5e);
    OV5642YUV_write_cmos_sensor(0x8431, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8432, 0x12);
    OV5642YUV_write_cmos_sensor(0x8433, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8434, 0x03);
    OV5642YUV_write_cmos_sensor(0x8435, 0x12);
    OV5642YUV_write_cmos_sensor(0x8436, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8437, 0x15);
    OV5642YUV_write_cmos_sensor(0x8438, 0x12);
    OV5642YUV_write_cmos_sensor(0x8439, 0x0d);
    OV5642YUV_write_cmos_sensor(0x843a, 0x66);
    OV5642YUV_write_cmos_sensor(0x843b, 0xe4);
    OV5642YUV_write_cmos_sensor(0x843c, 0xf5);
    OV5642YUV_write_cmos_sensor(0x843d, 0x32);
    OV5642YUV_write_cmos_sensor(0x843e, 0x90);
    OV5642YUV_write_cmos_sensor(0x843f, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8440, 0x6b);
    OV5642YUV_write_cmos_sensor(0x8441, 0x12);
    OV5642YUV_write_cmos_sensor(0x8442, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8443, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8444, 0x12);
    OV5642YUV_write_cmos_sensor(0x8445, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8446, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8447, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8448, 0xb5);
    OV5642YUV_write_cmos_sensor(0x8449, 0x05);
    OV5642YUV_write_cmos_sensor(0x844a, 0x09);
    OV5642YUV_write_cmos_sensor(0x844b, 0x05);
    OV5642YUV_write_cmos_sensor(0x844c, 0x32);
    OV5642YUV_write_cmos_sensor(0x844d, 0xe5);
    OV5642YUV_write_cmos_sensor(0x844e, 0x32);
    OV5642YUV_write_cmos_sensor(0x844f, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8450, 0x94);
    OV5642YUV_write_cmos_sensor(0x8451, 0xfa);
    OV5642YUV_write_cmos_sensor(0x8452, 0x40);
    OV5642YUV_write_cmos_sensor(0x8453, 0xea);
    OV5642YUV_write_cmos_sensor(0x8454, 0x12);
    OV5642YUV_write_cmos_sensor(0x8455, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8456, 0x4e);
    OV5642YUV_write_cmos_sensor(0x8457, 0x12);
    OV5642YUV_write_cmos_sensor(0x8458, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8459, 0x0a);
    OV5642YUV_write_cmos_sensor(0x845a, 0xee);
    OV5642YUV_write_cmos_sensor(0x845b, 0xf0);
    OV5642YUV_write_cmos_sensor(0x845c, 0xed);
    OV5642YUV_write_cmos_sensor(0x845d, 0xb5);
    OV5642YUV_write_cmos_sensor(0x845e, 0x04);
    OV5642YUV_write_cmos_sensor(0x845f, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8460, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8461, 0x34);
    OV5642YUV_write_cmos_sensor(0x8462, 0x04);
    OV5642YUV_write_cmos_sensor(0x8463, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8464, 0x38);
    OV5642YUV_write_cmos_sensor(0x8465, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8466, 0x25);
    OV5642YUV_write_cmos_sensor(0x8467, 0x45);
    OV5642YUV_write_cmos_sensor(0x8468, 0x24);
    OV5642YUV_write_cmos_sensor(0x8469, 0x60);
    OV5642YUV_write_cmos_sensor(0x846a, 0x03);
    OV5642YUV_write_cmos_sensor(0x846b, 0x02);
    OV5642YUV_write_cmos_sensor(0x846c, 0x05);
    OV5642YUV_write_cmos_sensor(0x846d, 0x2f);
    OV5642YUV_write_cmos_sensor(0x846e, 0x05);
    OV5642YUV_write_cmos_sensor(0x846f, 0x34);
    OV5642YUV_write_cmos_sensor(0x8470, 0x02);
    OV5642YUV_write_cmos_sensor(0x8471, 0x03);
    OV5642YUV_write_cmos_sensor(0x8472, 0xc4);
    OV5642YUV_write_cmos_sensor(0x8473, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8474, 0x74);
    OV5642YUV_write_cmos_sensor(0x8475, 0x04);
    OV5642YUV_write_cmos_sensor(0x8476, 0x95);
    OV5642YUV_write_cmos_sensor(0x8477, 0x37);
    OV5642YUV_write_cmos_sensor(0x8478, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8479, 0x34);
    OV5642YUV_write_cmos_sensor(0x847a, 0xe5);
    OV5642YUV_write_cmos_sensor(0x847b, 0x34);
    OV5642YUV_write_cmos_sensor(0x847c, 0xc3);
    OV5642YUV_write_cmos_sensor(0x847d, 0x94);
    OV5642YUV_write_cmos_sensor(0x847e, 0x04);
    OV5642YUV_write_cmos_sensor(0x847f, 0x40);
    OV5642YUV_write_cmos_sensor(0x8480, 0x03);
    OV5642YUV_write_cmos_sensor(0x8481, 0x02);
    OV5642YUV_write_cmos_sensor(0x8482, 0x05);
    OV5642YUV_write_cmos_sensor(0x8483, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8484, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8485, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8486, 0x35);
    OV5642YUV_write_cmos_sensor(0x8487, 0x90);
    OV5642YUV_write_cmos_sensor(0x8488, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8489, 0x78);
    OV5642YUV_write_cmos_sensor(0x848a, 0x12);
    OV5642YUV_write_cmos_sensor(0x848b, 0x0d);
    OV5642YUV_write_cmos_sensor(0x848c, 0x47);
    OV5642YUV_write_cmos_sensor(0x848d, 0x12);
    OV5642YUV_write_cmos_sensor(0x848e, 0x0c);
    OV5642YUV_write_cmos_sensor(0x848f, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8490, 0x12);
    OV5642YUV_write_cmos_sensor(0x8491, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8492, 0x27);
    OV5642YUV_write_cmos_sensor(0x8493, 0x90);
    OV5642YUV_write_cmos_sensor(0x8494, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8495, 0x75);
    OV5642YUV_write_cmos_sensor(0x8496, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8497, 0x93);
    OV5642YUV_write_cmos_sensor(0x8498, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8499, 0x12);
    OV5642YUV_write_cmos_sensor(0x849a, 0x0c);
    OV5642YUV_write_cmos_sensor(0x849b, 0xfe);
    OV5642YUV_write_cmos_sensor(0x849c, 0xef);
    OV5642YUV_write_cmos_sensor(0x849d, 0xf0);
    OV5642YUV_write_cmos_sensor(0x849e, 0xed);
    OV5642YUV_write_cmos_sensor(0x849f, 0x52);
    OV5642YUV_write_cmos_sensor(0x84a0, 0x07);
    OV5642YUV_write_cmos_sensor(0x84a1, 0x90);
    OV5642YUV_write_cmos_sensor(0x84a2, 0x0e);
    OV5642YUV_write_cmos_sensor(0x84a3, 0x76);
    OV5642YUV_write_cmos_sensor(0x84a4, 0xe4);
    OV5642YUV_write_cmos_sensor(0x84a5, 0x93);
    OV5642YUV_write_cmos_sensor(0x84a6, 0x42);
    OV5642YUV_write_cmos_sensor(0x84a7, 0x07);
    OV5642YUV_write_cmos_sensor(0x84a8, 0x90);
    OV5642YUV_write_cmos_sensor(0x84a9, 0x0e);
    OV5642YUV_write_cmos_sensor(0x84aa, 0x6d);
    OV5642YUV_write_cmos_sensor(0x84ab, 0x12);
    OV5642YUV_write_cmos_sensor(0x84ac, 0x0c);
    OV5642YUV_write_cmos_sensor(0x84ad, 0xf0);
    OV5642YUV_write_cmos_sensor(0x84ae, 0xf5);
    OV5642YUV_write_cmos_sensor(0x84af, 0x82);
    OV5642YUV_write_cmos_sensor(0x84b0, 0x8c);
    OV5642YUV_write_cmos_sensor(0x84b1, 0x83);
    OV5642YUV_write_cmos_sensor(0x84b2, 0xef);
    OV5642YUV_write_cmos_sensor(0x84b3, 0xf0);
    OV5642YUV_write_cmos_sensor(0x84b4, 0x75);
    OV5642YUV_write_cmos_sensor(0x84b5, 0x33);
    OV5642YUV_write_cmos_sensor(0x84b6, 0x09);
    OV5642YUV_write_cmos_sensor(0x84b7, 0x90);
    OV5642YUV_write_cmos_sensor(0x84b8, 0x0e);
    OV5642YUV_write_cmos_sensor(0x84b9, 0x72);
    OV5642YUV_write_cmos_sensor(0x84ba, 0xe4);
    OV5642YUV_write_cmos_sensor(0x84bb, 0x93);
    OV5642YUV_write_cmos_sensor(0x84bc, 0xfd);
    OV5642YUV_write_cmos_sensor(0x84bd, 0x12);
    OV5642YUV_write_cmos_sensor(0x84be, 0x0d);
    OV5642YUV_write_cmos_sensor(0x84bf, 0x1c);
    OV5642YUV_write_cmos_sensor(0x84c0, 0x90);
    OV5642YUV_write_cmos_sensor(0x84c1, 0x0e);
    OV5642YUV_write_cmos_sensor(0x84c2, 0x69);
    OV5642YUV_write_cmos_sensor(0x84c3, 0x12);
    OV5642YUV_write_cmos_sensor(0x84c4, 0x0d);
    OV5642YUV_write_cmos_sensor(0x84c5, 0x0a);
    OV5642YUV_write_cmos_sensor(0x84c6, 0xee);
    OV5642YUV_write_cmos_sensor(0x84c7, 0xf0);
    OV5642YUV_write_cmos_sensor(0x84c8, 0xe5);
    OV5642YUV_write_cmos_sensor(0x84c9, 0x35);
    OV5642YUV_write_cmos_sensor(0x84ca, 0x25);
    OV5642YUV_write_cmos_sensor(0x84cb, 0xe0);
    OV5642YUV_write_cmos_sensor(0x84cc, 0xf5);
    OV5642YUV_write_cmos_sensor(0x84cd, 0x35);
    OV5642YUV_write_cmos_sensor(0x84ce, 0x90);
    OV5642YUV_write_cmos_sensor(0x84cf, 0x0e);
    OV5642YUV_write_cmos_sensor(0x84d0, 0x6b);
    OV5642YUV_write_cmos_sensor(0x84d1, 0x12);
    OV5642YUV_write_cmos_sensor(0x84d2, 0x0d);
    OV5642YUV_write_cmos_sensor(0x84d3, 0x0a);
    OV5642YUV_write_cmos_sensor(0x84d4, 0xe0);
    OV5642YUV_write_cmos_sensor(0x84d5, 0xf5);
    OV5642YUV_write_cmos_sensor(0x84d6, 0x31);
    OV5642YUV_write_cmos_sensor(0x84d7, 0xed);
    OV5642YUV_write_cmos_sensor(0x84d8, 0x12);
    OV5642YUV_write_cmos_sensor(0x84d9, 0x0c);
    OV5642YUV_write_cmos_sensor(0x84da, 0xe4);
    OV5642YUV_write_cmos_sensor(0x84db, 0x12);
    OV5642YUV_write_cmos_sensor(0x84dc, 0x0d);
    OV5642YUV_write_cmos_sensor(0x84dd, 0x27);
    OV5642YUV_write_cmos_sensor(0x84de, 0x12);
    OV5642YUV_write_cmos_sensor(0x84df, 0x0d);
    OV5642YUV_write_cmos_sensor(0x84e0, 0x35);
    OV5642YUV_write_cmos_sensor(0x84e1, 0xb5);
    OV5642YUV_write_cmos_sensor(0x84e2, 0x05);
    OV5642YUV_write_cmos_sensor(0x84e3, 0x02);
    OV5642YUV_write_cmos_sensor(0x84e4, 0x05);
    OV5642YUV_write_cmos_sensor(0x84e5, 0x35);
    OV5642YUV_write_cmos_sensor(0x84e6, 0x15);
    OV5642YUV_write_cmos_sensor(0x84e7, 0x33);
    OV5642YUV_write_cmos_sensor(0x84e8, 0xe5);
    OV5642YUV_write_cmos_sensor(0x84e9, 0x33);
    OV5642YUV_write_cmos_sensor(0x84ea, 0xb4);
    OV5642YUV_write_cmos_sensor(0x84eb, 0x01);
    OV5642YUV_write_cmos_sensor(0x84ec, 0xca);
    OV5642YUV_write_cmos_sensor(0x84ed, 0x90);
    OV5642YUV_write_cmos_sensor(0x84ee, 0x0e);
    OV5642YUV_write_cmos_sensor(0x84ef, 0x78);
    OV5642YUV_write_cmos_sensor(0x84f0, 0xe4);
    OV5642YUV_write_cmos_sensor(0x84f1, 0x93);
    OV5642YUV_write_cmos_sensor(0x84f2, 0x52);
    OV5642YUV_write_cmos_sensor(0x84f3, 0x06);
    OV5642YUV_write_cmos_sensor(0x84f4, 0xe5);
    OV5642YUV_write_cmos_sensor(0x84f5, 0x34);
    OV5642YUV_write_cmos_sensor(0x84f6, 0xb4);
    OV5642YUV_write_cmos_sensor(0x84f7, 0x03);
    OV5642YUV_write_cmos_sensor(0x84f8, 0x05);
    OV5642YUV_write_cmos_sensor(0x84f9, 0x90);
    OV5642YUV_write_cmos_sensor(0x84fa, 0x0e);
    OV5642YUV_write_cmos_sensor(0x84fb, 0x7a);
    OV5642YUV_write_cmos_sensor(0x84fc, 0x80);
    OV5642YUV_write_cmos_sensor(0x84fd, 0x03);
    OV5642YUV_write_cmos_sensor(0x84fe, 0x90);
    OV5642YUV_write_cmos_sensor(0x84ff, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8500, 0x79);
    OV5642YUV_write_cmos_sensor(0x8501, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8502, 0x93);
    OV5642YUV_write_cmos_sensor(0x8503, 0x42);
    OV5642YUV_write_cmos_sensor(0x8504, 0x06);
    OV5642YUV_write_cmos_sensor(0x8505, 0x12);
    OV5642YUV_write_cmos_sensor(0x8506, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8507, 0xed);
    OV5642YUV_write_cmos_sensor(0x8508, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8509, 0xf5);
    OV5642YUV_write_cmos_sensor(0x850a, 0x82);
    OV5642YUV_write_cmos_sensor(0x850b, 0x8c);
    OV5642YUV_write_cmos_sensor(0x850c, 0x83);
    OV5642YUV_write_cmos_sensor(0x850d, 0x12);
    OV5642YUV_write_cmos_sensor(0x850e, 0x0c);
    OV5642YUV_write_cmos_sensor(0x850f, 0xf7);
    OV5642YUV_write_cmos_sensor(0x8510, 0xef);
    OV5642YUV_write_cmos_sensor(0x8511, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8512, 0x12);
    OV5642YUV_write_cmos_sensor(0x8513, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8514, 0xce);
    OV5642YUV_write_cmos_sensor(0x8515, 0x12);
    OV5642YUV_write_cmos_sensor(0x8516, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8517, 0x27);
    OV5642YUV_write_cmos_sensor(0x8518, 0x7c);
    OV5642YUV_write_cmos_sensor(0x8519, 0x00);
    OV5642YUV_write_cmos_sensor(0x851a, 0x7b);
    OV5642YUV_write_cmos_sensor(0x851b, 0x00);
    OV5642YUV_write_cmos_sensor(0x851c, 0x74);
    OV5642YUV_write_cmos_sensor(0x851d, 0x39);
    OV5642YUV_write_cmos_sensor(0x851e, 0x25);
    OV5642YUV_write_cmos_sensor(0x851f, 0x34);
    OV5642YUV_write_cmos_sensor(0x8520, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8521, 0xec);
    OV5642YUV_write_cmos_sensor(0x8522, 0x34);
    OV5642YUV_write_cmos_sensor(0x8523, 0x00);
    OV5642YUV_write_cmos_sensor(0x8524, 0xfa);
    OV5642YUV_write_cmos_sensor(0x8525, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8526, 0x35);
    OV5642YUV_write_cmos_sensor(0x8527, 0x12);
    OV5642YUV_write_cmos_sensor(0x8528, 0x07);
    OV5642YUV_write_cmos_sensor(0x8529, 0xd1);
    OV5642YUV_write_cmos_sensor(0x852a, 0x05);
    OV5642YUV_write_cmos_sensor(0x852b, 0x34);
    OV5642YUV_write_cmos_sensor(0x852c, 0x02);
    OV5642YUV_write_cmos_sensor(0x852d, 0x04);
    OV5642YUV_write_cmos_sensor(0x852e, 0x7a);
    OV5642YUV_write_cmos_sensor(0x852f, 0x90);
    OV5642YUV_write_cmos_sensor(0x8530, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8531, 0x78);
    OV5642YUV_write_cmos_sensor(0x8532, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8533, 0x93);
    OV5642YUV_write_cmos_sensor(0x8534, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8535, 0x12);
    OV5642YUV_write_cmos_sensor(0x8536, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8537, 0x53);
    OV5642YUV_write_cmos_sensor(0x8538, 0x12);
    OV5642YUV_write_cmos_sensor(0x8539, 0x0d);
    OV5642YUV_write_cmos_sensor(0x853a, 0x0a);
    OV5642YUV_write_cmos_sensor(0x853b, 0x12);
    OV5642YUV_write_cmos_sensor(0x853c, 0x0c);
    OV5642YUV_write_cmos_sensor(0x853d, 0xf7);
    OV5642YUV_write_cmos_sensor(0x853e, 0x12);
    OV5642YUV_write_cmos_sensor(0x853f, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8540, 0x15);
    OV5642YUV_write_cmos_sensor(0x8541, 0x90);
    OV5642YUV_write_cmos_sensor(0x8542, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8543, 0x69);
    OV5642YUV_write_cmos_sensor(0x8544, 0x12);
    OV5642YUV_write_cmos_sensor(0x8545, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8546, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8547, 0xee);
    OV5642YUV_write_cmos_sensor(0x8548, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8549, 0xed);
    OV5642YUV_write_cmos_sensor(0x854a, 0x52);
    OV5642YUV_write_cmos_sensor(0x854b, 0x06);
    OV5642YUV_write_cmos_sensor(0x854c, 0x90);
    OV5642YUV_write_cmos_sensor(0x854d, 0x0e);
    OV5642YUV_write_cmos_sensor(0x854e, 0x7a);
    OV5642YUV_write_cmos_sensor(0x854f, 0x12);
    OV5642YUV_write_cmos_sensor(0x8550, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8551, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8552, 0x12);
    OV5642YUV_write_cmos_sensor(0x8553, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8554, 0x27);
    OV5642YUV_write_cmos_sensor(0x8555, 0xa2);
    OV5642YUV_write_cmos_sensor(0x8556, 0x32);
    OV5642YUV_write_cmos_sensor(0x8557, 0x92);
    OV5642YUV_write_cmos_sensor(0x8558, 0xaf);
    OV5642YUV_write_cmos_sensor(0x8559, 0x22);
    OV5642YUV_write_cmos_sensor(0x855a, 0xe5);
    OV5642YUV_write_cmos_sensor(0x855b, 0x1f);
    OV5642YUV_write_cmos_sensor(0x855c, 0x70);
    OV5642YUV_write_cmos_sensor(0x855d, 0x72);
    OV5642YUV_write_cmos_sensor(0x855e, 0xf5);
    OV5642YUV_write_cmos_sensor(0x855f, 0x1e);
    OV5642YUV_write_cmos_sensor(0x8560, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8561, 0x35);
    OV5642YUV_write_cmos_sensor(0x8562, 0xff);
    OV5642YUV_write_cmos_sensor(0x8563, 0xef);
    OV5642YUV_write_cmos_sensor(0x8564, 0x25);
    OV5642YUV_write_cmos_sensor(0x8565, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8566, 0x24);
    OV5642YUV_write_cmos_sensor(0x8567, 0x51);
    OV5642YUV_write_cmos_sensor(0x8568, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8569, 0xe4);
    OV5642YUV_write_cmos_sensor(0x856a, 0xf6);
    OV5642YUV_write_cmos_sensor(0x856b, 0x08);
    OV5642YUV_write_cmos_sensor(0x856c, 0xf6);
    OV5642YUV_write_cmos_sensor(0x856d, 0x0f);
    OV5642YUV_write_cmos_sensor(0x856e, 0xbf);
    OV5642YUV_write_cmos_sensor(0x856f, 0x34);
    OV5642YUV_write_cmos_sensor(0x8570, 0xf2);
    OV5642YUV_write_cmos_sensor(0x8571, 0x90);
    OV5642YUV_write_cmos_sensor(0x8572, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8573, 0xaa);
    OV5642YUV_write_cmos_sensor(0x8574, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8575, 0x93);
    OV5642YUV_write_cmos_sensor(0x8576, 0xff);
    OV5642YUV_write_cmos_sensor(0x8577, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8578, 0x4e);
    OV5642YUV_write_cmos_sensor(0x8579, 0xc3);
    OV5642YUV_write_cmos_sensor(0x857a, 0x9f);
    OV5642YUV_write_cmos_sensor(0x857b, 0x50);
    OV5642YUV_write_cmos_sensor(0x857c, 0x04);
    OV5642YUV_write_cmos_sensor(0x857d, 0x7f);
    OV5642YUV_write_cmos_sensor(0x857e, 0x05);
    OV5642YUV_write_cmos_sensor(0x857f, 0x80);
    OV5642YUV_write_cmos_sensor(0x8580, 0x02);
    OV5642YUV_write_cmos_sensor(0x8581, 0x7f);
    OV5642YUV_write_cmos_sensor(0x8582, 0xfb);
    OV5642YUV_write_cmos_sensor(0x8583, 0x78);
    OV5642YUV_write_cmos_sensor(0x8584, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8585, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8586, 0x07);
    OV5642YUV_write_cmos_sensor(0x8587, 0x12);
    OV5642YUV_write_cmos_sensor(0x8588, 0x11);
    OV5642YUV_write_cmos_sensor(0x8589, 0xcd);
    OV5642YUV_write_cmos_sensor(0x858a, 0x40);
    OV5642YUV_write_cmos_sensor(0x858b, 0x04);
    OV5642YUV_write_cmos_sensor(0x858c, 0x7f);
    OV5642YUV_write_cmos_sensor(0x858d, 0x03);
    OV5642YUV_write_cmos_sensor(0x858e, 0x80);
    OV5642YUV_write_cmos_sensor(0x858f, 0x02);
    OV5642YUV_write_cmos_sensor(0x8590, 0x7f);
    OV5642YUV_write_cmos_sensor(0x8591, 0x30);
    OV5642YUV_write_cmos_sensor(0x8592, 0x78);
    OV5642YUV_write_cmos_sensor(0x8593, 0xbf);
    OV5642YUV_write_cmos_sensor(0x8594, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8595, 0x07);
    OV5642YUV_write_cmos_sensor(0x8596, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8597, 0x18);
    OV5642YUV_write_cmos_sensor(0x8598, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8599, 0x08);
    OV5642YUV_write_cmos_sensor(0x859a, 0xe6);
    OV5642YUV_write_cmos_sensor(0x859b, 0x78);
    OV5642YUV_write_cmos_sensor(0x859c, 0xbc);
    OV5642YUV_write_cmos_sensor(0x859d, 0xf6);
    OV5642YUV_write_cmos_sensor(0x859e, 0x78);
    OV5642YUV_write_cmos_sensor(0x859f, 0xbf);
    OV5642YUV_write_cmos_sensor(0x85a0, 0xe6);
    OV5642YUV_write_cmos_sensor(0x85a1, 0x78);
    OV5642YUV_write_cmos_sensor(0x85a2, 0xbd);
    OV5642YUV_write_cmos_sensor(0x85a3, 0xf6);
    OV5642YUV_write_cmos_sensor(0x85a4, 0x78);
    OV5642YUV_write_cmos_sensor(0x85a5, 0xc2);
    OV5642YUV_write_cmos_sensor(0x85a6, 0x76);
    OV5642YUV_write_cmos_sensor(0x85a7, 0x33);
    OV5642YUV_write_cmos_sensor(0x85a8, 0xe4);
    OV5642YUV_write_cmos_sensor(0x85a9, 0x08);
    OV5642YUV_write_cmos_sensor(0x85aa, 0xf6);
    OV5642YUV_write_cmos_sensor(0x85ab, 0x78);
    OV5642YUV_write_cmos_sensor(0x85ac, 0xbb);
    OV5642YUV_write_cmos_sensor(0x85ad, 0x76);
    OV5642YUV_write_cmos_sensor(0x85ae, 0x01);
    OV5642YUV_write_cmos_sensor(0x85af, 0x75);
    OV5642YUV_write_cmos_sensor(0x85b0, 0x4d);
    OV5642YUV_write_cmos_sensor(0x85b1, 0x02);
    OV5642YUV_write_cmos_sensor(0x85b2, 0x78);
    OV5642YUV_write_cmos_sensor(0x85b3, 0xb9);
    OV5642YUV_write_cmos_sensor(0x85b4, 0xf6);
    OV5642YUV_write_cmos_sensor(0x85b5, 0x08);
    OV5642YUV_write_cmos_sensor(0x85b6, 0xf6);
    OV5642YUV_write_cmos_sensor(0x85b7, 0x74);
    OV5642YUV_write_cmos_sensor(0x85b8, 0xff);
    OV5642YUV_write_cmos_sensor(0x85b9, 0x78);
    OV5642YUV_write_cmos_sensor(0x85ba, 0xc4);
    OV5642YUV_write_cmos_sensor(0x85bb, 0xf6);
    OV5642YUV_write_cmos_sensor(0x85bc, 0x08);
    OV5642YUV_write_cmos_sensor(0x85bd, 0xf6);
    OV5642YUV_write_cmos_sensor(0x85be, 0x75);
    OV5642YUV_write_cmos_sensor(0x85bf, 0x1f);
    OV5642YUV_write_cmos_sensor(0x85c0, 0x01);
    OV5642YUV_write_cmos_sensor(0x85c1, 0x78);
    OV5642YUV_write_cmos_sensor(0x85c2, 0xbf);
    OV5642YUV_write_cmos_sensor(0x85c3, 0xe6);
    OV5642YUV_write_cmos_sensor(0x85c4, 0x75);
    OV5642YUV_write_cmos_sensor(0x85c5, 0xf0);
    OV5642YUV_write_cmos_sensor(0x85c6, 0x05);
    OV5642YUV_write_cmos_sensor(0x85c7, 0xa4);
    OV5642YUV_write_cmos_sensor(0x85c8, 0xf5);
    OV5642YUV_write_cmos_sensor(0x85c9, 0x4e);
    OV5642YUV_write_cmos_sensor(0x85ca, 0x12);
    OV5642YUV_write_cmos_sensor(0x85cb, 0x0d);
    OV5642YUV_write_cmos_sensor(0x85cc, 0x6d);
    OV5642YUV_write_cmos_sensor(0x85cd, 0xc2);
    OV5642YUV_write_cmos_sensor(0x85ce, 0x37);
    OV5642YUV_write_cmos_sensor(0x85cf, 0x22);
    OV5642YUV_write_cmos_sensor(0x85d0, 0x78);
    OV5642YUV_write_cmos_sensor(0x85d1, 0xbb);
    OV5642YUV_write_cmos_sensor(0x85d2, 0xe6);
    OV5642YUV_write_cmos_sensor(0x85d3, 0xd3);
    OV5642YUV_write_cmos_sensor(0x85d4, 0x94);
    OV5642YUV_write_cmos_sensor(0x85d5, 0x00);
    OV5642YUV_write_cmos_sensor(0x85d6, 0x40);
    OV5642YUV_write_cmos_sensor(0x85d7, 0x02);
    OV5642YUV_write_cmos_sensor(0x85d8, 0x16);
    OV5642YUV_write_cmos_sensor(0x85d9, 0x22);
    OV5642YUV_write_cmos_sensor(0x85da, 0xe5);
    OV5642YUV_write_cmos_sensor(0x85db, 0x1f);
    OV5642YUV_write_cmos_sensor(0x85dc, 0xb4);
    OV5642YUV_write_cmos_sensor(0x85dd, 0x05);
    OV5642YUV_write_cmos_sensor(0x85de, 0x23);
    OV5642YUV_write_cmos_sensor(0x85df, 0xe4);
    OV5642YUV_write_cmos_sensor(0x85e0, 0xf5);
    OV5642YUV_write_cmos_sensor(0x85e1, 0x1f);
    OV5642YUV_write_cmos_sensor(0x85e2, 0xc2);
    OV5642YUV_write_cmos_sensor(0x85e3, 0x01);
    OV5642YUV_write_cmos_sensor(0x85e4, 0x78);
    OV5642YUV_write_cmos_sensor(0x85e5, 0xb9);
    OV5642YUV_write_cmos_sensor(0x85e6, 0xe6);
    OV5642YUV_write_cmos_sensor(0x85e7, 0xfe);
    OV5642YUV_write_cmos_sensor(0x85e8, 0x08);
    OV5642YUV_write_cmos_sensor(0x85e9, 0xe6);
    OV5642YUV_write_cmos_sensor(0x85ea, 0xff);
    OV5642YUV_write_cmos_sensor(0x85eb, 0x78);
    OV5642YUV_write_cmos_sensor(0x85ec, 0x51);
    OV5642YUV_write_cmos_sensor(0x85ed, 0xa6);
    OV5642YUV_write_cmos_sensor(0x85ee, 0x06);
    OV5642YUV_write_cmos_sensor(0x85ef, 0x08);
    OV5642YUV_write_cmos_sensor(0x85f0, 0xa6);
    OV5642YUV_write_cmos_sensor(0x85f1, 0x07);
    OV5642YUV_write_cmos_sensor(0x85f2, 0xa2);
    OV5642YUV_write_cmos_sensor(0x85f3, 0x37);
    OV5642YUV_write_cmos_sensor(0x85f4, 0xe4);
    OV5642YUV_write_cmos_sensor(0x85f5, 0x33);
    OV5642YUV_write_cmos_sensor(0x85f6, 0xf5);
    OV5642YUV_write_cmos_sensor(0x85f7, 0x41);
    OV5642YUV_write_cmos_sensor(0x85f8, 0x90);
    OV5642YUV_write_cmos_sensor(0x85f9, 0x30);
    OV5642YUV_write_cmos_sensor(0x85fa, 0x26);
    OV5642YUV_write_cmos_sensor(0x85fb, 0xf0);
    OV5642YUV_write_cmos_sensor(0x85fc, 0x75);
    OV5642YUV_write_cmos_sensor(0x85fd, 0x1e);
    OV5642YUV_write_cmos_sensor(0x85fe, 0x10);
    OV5642YUV_write_cmos_sensor(0x85ff, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8600, 0x35);
    OV5642YUV_write_cmos_sensor(0x8601, 0x22);
    OV5642YUV_write_cmos_sensor(0x8602, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8603, 0x4e);
    OV5642YUV_write_cmos_sensor(0x8604, 0x75);
    OV5642YUV_write_cmos_sensor(0x8605, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8606, 0x05);
    OV5642YUV_write_cmos_sensor(0x8607, 0x84);
    OV5642YUV_write_cmos_sensor(0x8608, 0x78);
    OV5642YUV_write_cmos_sensor(0x8609, 0xbf);
    OV5642YUV_write_cmos_sensor(0x860a, 0xf6);
    OV5642YUV_write_cmos_sensor(0x860b, 0x90);
    OV5642YUV_write_cmos_sensor(0x860c, 0x0e);
    OV5642YUV_write_cmos_sensor(0x860d, 0xa3);
    OV5642YUV_write_cmos_sensor(0x860e, 0xe4);
    OV5642YUV_write_cmos_sensor(0x860f, 0x93);
    OV5642YUV_write_cmos_sensor(0x8610, 0xff);
    OV5642YUV_write_cmos_sensor(0x8611, 0x25);
    OV5642YUV_write_cmos_sensor(0x8612, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8613, 0x24);
    OV5642YUV_write_cmos_sensor(0x8614, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8615, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8616, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8617, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8618, 0x08);
    OV5642YUV_write_cmos_sensor(0x8619, 0xe6);
    OV5642YUV_write_cmos_sensor(0x861a, 0xfd);
    OV5642YUV_write_cmos_sensor(0x861b, 0x78);
    OV5642YUV_write_cmos_sensor(0x861c, 0xbf);
    OV5642YUV_write_cmos_sensor(0x861d, 0xe6);
    OV5642YUV_write_cmos_sensor(0x861e, 0x25);
    OV5642YUV_write_cmos_sensor(0x861f, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8620, 0x24);
    OV5642YUV_write_cmos_sensor(0x8621, 0x51);
    OV5642YUV_write_cmos_sensor(0x8622, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8623, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8624, 0x04);
    OV5642YUV_write_cmos_sensor(0x8625, 0x08);
    OV5642YUV_write_cmos_sensor(0x8626, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8627, 0x05);
    OV5642YUV_write_cmos_sensor(0x8628, 0xef);
    OV5642YUV_write_cmos_sensor(0x8629, 0x12);
    OV5642YUV_write_cmos_sensor(0x862a, 0x11);
    OV5642YUV_write_cmos_sensor(0x862b, 0xd4);
    OV5642YUV_write_cmos_sensor(0x862c, 0xd3);
    OV5642YUV_write_cmos_sensor(0x862d, 0x78);
    OV5642YUV_write_cmos_sensor(0x862e, 0xba);
    OV5642YUV_write_cmos_sensor(0x862f, 0x96);
    OV5642YUV_write_cmos_sensor(0x8630, 0xee);
    OV5642YUV_write_cmos_sensor(0x8631, 0x18);
    OV5642YUV_write_cmos_sensor(0x8632, 0x96);
    OV5642YUV_write_cmos_sensor(0x8633, 0x40);
    OV5642YUV_write_cmos_sensor(0x8634, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8635, 0x78);
    OV5642YUV_write_cmos_sensor(0x8636, 0xbf);
    OV5642YUV_write_cmos_sensor(0x8637, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8638, 0x78);
    OV5642YUV_write_cmos_sensor(0x8639, 0xbc);
    OV5642YUV_write_cmos_sensor(0x863a, 0xf6);
    OV5642YUV_write_cmos_sensor(0x863b, 0x78);
    OV5642YUV_write_cmos_sensor(0x863c, 0xb9);
    OV5642YUV_write_cmos_sensor(0x863d, 0xa6);
    OV5642YUV_write_cmos_sensor(0x863e, 0x06);
    OV5642YUV_write_cmos_sensor(0x863f, 0x08);
    OV5642YUV_write_cmos_sensor(0x8640, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8641, 0x07);
    OV5642YUV_write_cmos_sensor(0x8642, 0x90);
    OV5642YUV_write_cmos_sensor(0x8643, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8644, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8645, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8646, 0x93);
    OV5642YUV_write_cmos_sensor(0x8647, 0x12);
    OV5642YUV_write_cmos_sensor(0x8648, 0x11);
    OV5642YUV_write_cmos_sensor(0x8649, 0xd4);
    OV5642YUV_write_cmos_sensor(0x864a, 0xc3);
    OV5642YUV_write_cmos_sensor(0x864b, 0x78);
    OV5642YUV_write_cmos_sensor(0x864c, 0xc5);
    OV5642YUV_write_cmos_sensor(0x864d, 0x96);
    OV5642YUV_write_cmos_sensor(0x864e, 0xee);
    OV5642YUV_write_cmos_sensor(0x864f, 0x18);
    OV5642YUV_write_cmos_sensor(0x8650, 0x96);
    OV5642YUV_write_cmos_sensor(0x8651, 0x50);
    OV5642YUV_write_cmos_sensor(0x8652, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8653, 0x78);
    OV5642YUV_write_cmos_sensor(0x8654, 0xbf);
    OV5642YUV_write_cmos_sensor(0x8655, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8656, 0x78);
    OV5642YUV_write_cmos_sensor(0x8657, 0xbd);
    OV5642YUV_write_cmos_sensor(0x8658, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8659, 0x78);
    OV5642YUV_write_cmos_sensor(0x865a, 0xc4);
    OV5642YUV_write_cmos_sensor(0x865b, 0xa6);
    OV5642YUV_write_cmos_sensor(0x865c, 0x06);
    OV5642YUV_write_cmos_sensor(0x865d, 0x08);
    OV5642YUV_write_cmos_sensor(0x865e, 0xa6);
    OV5642YUV_write_cmos_sensor(0x865f, 0x07);
    OV5642YUV_write_cmos_sensor(0x8660, 0x78);
    OV5642YUV_write_cmos_sensor(0x8661, 0xb9);
    OV5642YUV_write_cmos_sensor(0x8662, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8663, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8664, 0x08);
    OV5642YUV_write_cmos_sensor(0x8665, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8666, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8667, 0x78);
    OV5642YUV_write_cmos_sensor(0x8668, 0xc5);
    OV5642YUV_write_cmos_sensor(0x8669, 0x96);
    OV5642YUV_write_cmos_sensor(0x866a, 0xff);
    OV5642YUV_write_cmos_sensor(0x866b, 0xee);
    OV5642YUV_write_cmos_sensor(0x866c, 0x18);
    OV5642YUV_write_cmos_sensor(0x866d, 0x96);
    OV5642YUV_write_cmos_sensor(0x866e, 0x78);
    OV5642YUV_write_cmos_sensor(0x866f, 0xc6);
    OV5642YUV_write_cmos_sensor(0x8670, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8671, 0x08);
    OV5642YUV_write_cmos_sensor(0x8672, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8673, 0x07);
    OV5642YUV_write_cmos_sensor(0x8674, 0x90);
    OV5642YUV_write_cmos_sensor(0x8675, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8676, 0xac);
    OV5642YUV_write_cmos_sensor(0x8677, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8678, 0x18);
    OV5642YUV_write_cmos_sensor(0x8679, 0x12);
    OV5642YUV_write_cmos_sensor(0x867a, 0x11);
    OV5642YUV_write_cmos_sensor(0x867b, 0xb2);
    OV5642YUV_write_cmos_sensor(0x867c, 0x40);
    OV5642YUV_write_cmos_sensor(0x867d, 0x02);
    OV5642YUV_write_cmos_sensor(0x867e, 0xd2);
    OV5642YUV_write_cmos_sensor(0x867f, 0x37);
    OV5642YUV_write_cmos_sensor(0x8680, 0x78);
    OV5642YUV_write_cmos_sensor(0x8681, 0xbf);
    OV5642YUV_write_cmos_sensor(0x8682, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8683, 0x08);
    OV5642YUV_write_cmos_sensor(0x8684, 0x26);
    OV5642YUV_write_cmos_sensor(0x8685, 0x08);
    OV5642YUV_write_cmos_sensor(0x8686, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8687, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8688, 0x1f);
    OV5642YUV_write_cmos_sensor(0x8689, 0x64);
    OV5642YUV_write_cmos_sensor(0x868a, 0x01);
    OV5642YUV_write_cmos_sensor(0x868b, 0x70);
    OV5642YUV_write_cmos_sensor(0x868c, 0x4a);
    OV5642YUV_write_cmos_sensor(0x868d, 0xe6);
    OV5642YUV_write_cmos_sensor(0x868e, 0xc3);
    OV5642YUV_write_cmos_sensor(0x868f, 0x78);
    OV5642YUV_write_cmos_sensor(0x8690, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8691, 0x12);
    OV5642YUV_write_cmos_sensor(0x8692, 0x11);
    OV5642YUV_write_cmos_sensor(0x8693, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8694, 0x40);
    OV5642YUV_write_cmos_sensor(0x8695, 0x05);
    OV5642YUV_write_cmos_sensor(0x8696, 0x12);
    OV5642YUV_write_cmos_sensor(0x8697, 0x11);
    OV5642YUV_write_cmos_sensor(0x8698, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8699, 0x40);
    OV5642YUV_write_cmos_sensor(0x869a, 0x39);
    OV5642YUV_write_cmos_sensor(0x869b, 0x12);
    OV5642YUV_write_cmos_sensor(0x869c, 0x11);
    OV5642YUV_write_cmos_sensor(0x869d, 0xcb);
    OV5642YUV_write_cmos_sensor(0x869e, 0x40);
    OV5642YUV_write_cmos_sensor(0x869f, 0x04);
    OV5642YUV_write_cmos_sensor(0x86a0, 0x7f);
    OV5642YUV_write_cmos_sensor(0x86a1, 0xfe);
    OV5642YUV_write_cmos_sensor(0x86a2, 0x80);
    OV5642YUV_write_cmos_sensor(0x86a3, 0x02);
    OV5642YUV_write_cmos_sensor(0x86a4, 0x7f);
    OV5642YUV_write_cmos_sensor(0x86a5, 0x02);
    OV5642YUV_write_cmos_sensor(0x86a6, 0x78);
    OV5642YUV_write_cmos_sensor(0x86a7, 0xc0);
    OV5642YUV_write_cmos_sensor(0x86a8, 0xa6);
    OV5642YUV_write_cmos_sensor(0x86a9, 0x07);
    OV5642YUV_write_cmos_sensor(0x86aa, 0x78);
    OV5642YUV_write_cmos_sensor(0x86ab, 0xbc);
    OV5642YUV_write_cmos_sensor(0x86ac, 0xe6);
    OV5642YUV_write_cmos_sensor(0x86ad, 0x24);
    OV5642YUV_write_cmos_sensor(0x86ae, 0x03);
    OV5642YUV_write_cmos_sensor(0x86af, 0x78);
    OV5642YUV_write_cmos_sensor(0x86b0, 0xc2);
    OV5642YUV_write_cmos_sensor(0x86b1, 0xf6);
    OV5642YUV_write_cmos_sensor(0x86b2, 0x78);
    OV5642YUV_write_cmos_sensor(0x86b3, 0xbc);
    OV5642YUV_write_cmos_sensor(0x86b4, 0xe6);
    OV5642YUV_write_cmos_sensor(0x86b5, 0x24);
    OV5642YUV_write_cmos_sensor(0x86b6, 0xfd);
    OV5642YUV_write_cmos_sensor(0x86b7, 0x78);
    OV5642YUV_write_cmos_sensor(0x86b8, 0xc3);
    OV5642YUV_write_cmos_sensor(0x86b9, 0xf6);
    OV5642YUV_write_cmos_sensor(0x86ba, 0x12);
    OV5642YUV_write_cmos_sensor(0x86bb, 0x11);
    OV5642YUV_write_cmos_sensor(0x86bc, 0xcb);
    OV5642YUV_write_cmos_sensor(0x86bd, 0x40);
    OV5642YUV_write_cmos_sensor(0x86be, 0x06);
    OV5642YUV_write_cmos_sensor(0x86bf, 0x78);
    OV5642YUV_write_cmos_sensor(0x86c0, 0xc3);
    OV5642YUV_write_cmos_sensor(0x86c1, 0xe6);
    OV5642YUV_write_cmos_sensor(0x86c2, 0xff);
    OV5642YUV_write_cmos_sensor(0x86c3, 0x80);
    OV5642YUV_write_cmos_sensor(0x86c4, 0x04);
    OV5642YUV_write_cmos_sensor(0x86c5, 0x78);
    OV5642YUV_write_cmos_sensor(0x86c6, 0xc2);
    OV5642YUV_write_cmos_sensor(0x86c7, 0xe6);
    OV5642YUV_write_cmos_sensor(0x86c8, 0xff);
    OV5642YUV_write_cmos_sensor(0x86c9, 0x78);
    OV5642YUV_write_cmos_sensor(0x86ca, 0xc1);
    OV5642YUV_write_cmos_sensor(0x86cb, 0xa6);
    OV5642YUV_write_cmos_sensor(0x86cc, 0x07);
    OV5642YUV_write_cmos_sensor(0x86cd, 0x75);
    OV5642YUV_write_cmos_sensor(0x86ce, 0x1f);
    OV5642YUV_write_cmos_sensor(0x86cf, 0x02);
    OV5642YUV_write_cmos_sensor(0x86d0, 0x78);
    OV5642YUV_write_cmos_sensor(0x86d1, 0xbb);
    OV5642YUV_write_cmos_sensor(0x86d2, 0x76);
    OV5642YUV_write_cmos_sensor(0x86d3, 0x01);
    OV5642YUV_write_cmos_sensor(0x86d4, 0x02);
    OV5642YUV_write_cmos_sensor(0x86d5, 0x07);
    OV5642YUV_write_cmos_sensor(0x86d6, 0x96);
    OV5642YUV_write_cmos_sensor(0x86d7, 0xe5);
    OV5642YUV_write_cmos_sensor(0x86d8, 0x1f);
    OV5642YUV_write_cmos_sensor(0x86d9, 0x64);
    OV5642YUV_write_cmos_sensor(0x86da, 0x02);
    OV5642YUV_write_cmos_sensor(0x86db, 0x60);
    OV5642YUV_write_cmos_sensor(0x86dc, 0x03);
    OV5642YUV_write_cmos_sensor(0x86dd, 0x02);
    OV5642YUV_write_cmos_sensor(0x86de, 0x07);
    OV5642YUV_write_cmos_sensor(0x86df, 0x76);
    OV5642YUV_write_cmos_sensor(0x86e0, 0x78);
    OV5642YUV_write_cmos_sensor(0x86e1, 0xc1);
    OV5642YUV_write_cmos_sensor(0x86e2, 0xe6);
    OV5642YUV_write_cmos_sensor(0x86e3, 0xff);
    OV5642YUV_write_cmos_sensor(0x86e4, 0xc3);
    OV5642YUV_write_cmos_sensor(0x86e5, 0x78);
    OV5642YUV_write_cmos_sensor(0x86e6, 0xc3);
    OV5642YUV_write_cmos_sensor(0x86e7, 0x12);
    OV5642YUV_write_cmos_sensor(0x86e8, 0x11);
    OV5642YUV_write_cmos_sensor(0x86e9, 0xa9);
    OV5642YUV_write_cmos_sensor(0x86ea, 0x40);
    OV5642YUV_write_cmos_sensor(0x86eb, 0x08);
    OV5642YUV_write_cmos_sensor(0x86ec, 0x12);
    OV5642YUV_write_cmos_sensor(0x86ed, 0x11);
    OV5642YUV_write_cmos_sensor(0x86ee, 0xa3);
    OV5642YUV_write_cmos_sensor(0x86ef, 0x50);
    OV5642YUV_write_cmos_sensor(0x86f0, 0x03);
    OV5642YUV_write_cmos_sensor(0x86f1, 0x02);
    OV5642YUV_write_cmos_sensor(0x86f2, 0x07);
    OV5642YUV_write_cmos_sensor(0x86f3, 0x74);
    OV5642YUV_write_cmos_sensor(0x86f4, 0x12);
    OV5642YUV_write_cmos_sensor(0x86f5, 0x11);
    OV5642YUV_write_cmos_sensor(0x86f6, 0xcb);
    OV5642YUV_write_cmos_sensor(0x86f7, 0x40);
    OV5642YUV_write_cmos_sensor(0x86f8, 0x04);
    OV5642YUV_write_cmos_sensor(0x86f9, 0x7f);
    OV5642YUV_write_cmos_sensor(0x86fa, 0xff);
    OV5642YUV_write_cmos_sensor(0x86fb, 0x80);
    OV5642YUV_write_cmos_sensor(0x86fc, 0x02);
    OV5642YUV_write_cmos_sensor(0x86fd, 0x7f);
    OV5642YUV_write_cmos_sensor(0x86fe, 0x01);
    OV5642YUV_write_cmos_sensor(0x86ff, 0x78);
    OV5642YUV_write_cmos_sensor(0x8700, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8701, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8702, 0x07);
    OV5642YUV_write_cmos_sensor(0x8703, 0x78);
    OV5642YUV_write_cmos_sensor(0x8704, 0xbc);
    OV5642YUV_write_cmos_sensor(0x8705, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8706, 0x04);
    OV5642YUV_write_cmos_sensor(0x8707, 0x78);
    OV5642YUV_write_cmos_sensor(0x8708, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8709, 0xf6);
    OV5642YUV_write_cmos_sensor(0x870a, 0x78);
    OV5642YUV_write_cmos_sensor(0x870b, 0xbc);
    OV5642YUV_write_cmos_sensor(0x870c, 0xe6);
    OV5642YUV_write_cmos_sensor(0x870d, 0x14);
    OV5642YUV_write_cmos_sensor(0x870e, 0x78);
    OV5642YUV_write_cmos_sensor(0x870f, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8710, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8711, 0x18);
    OV5642YUV_write_cmos_sensor(0x8712, 0x12);
    OV5642YUV_write_cmos_sensor(0x8713, 0x11);
    OV5642YUV_write_cmos_sensor(0x8714, 0xcd);
    OV5642YUV_write_cmos_sensor(0x8715, 0x40);
    OV5642YUV_write_cmos_sensor(0x8716, 0x04);
    OV5642YUV_write_cmos_sensor(0x8717, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8718, 0xff);
    OV5642YUV_write_cmos_sensor(0x8719, 0x80);
    OV5642YUV_write_cmos_sensor(0x871a, 0x02);
    OV5642YUV_write_cmos_sensor(0x871b, 0x7f);
    OV5642YUV_write_cmos_sensor(0x871c, 0x00);
    OV5642YUV_write_cmos_sensor(0x871d, 0x78);
    OV5642YUV_write_cmos_sensor(0x871e, 0xc2);
    OV5642YUV_write_cmos_sensor(0x871f, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8720, 0x07);
    OV5642YUV_write_cmos_sensor(0x8721, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8722, 0x08);
    OV5642YUV_write_cmos_sensor(0x8723, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8724, 0x64);
    OV5642YUV_write_cmos_sensor(0x8725, 0x80);
    OV5642YUV_write_cmos_sensor(0x8726, 0x94);
    OV5642YUV_write_cmos_sensor(0x8727, 0x80);
    OV5642YUV_write_cmos_sensor(0x8728, 0x40);
    OV5642YUV_write_cmos_sensor(0x8729, 0x04);
    OV5642YUV_write_cmos_sensor(0x872a, 0xe6);
    OV5642YUV_write_cmos_sensor(0x872b, 0xff);
    OV5642YUV_write_cmos_sensor(0x872c, 0x80);
    OV5642YUV_write_cmos_sensor(0x872d, 0x02);
    OV5642YUV_write_cmos_sensor(0x872e, 0x7f);
    OV5642YUV_write_cmos_sensor(0x872f, 0x00);
    OV5642YUV_write_cmos_sensor(0x8730, 0x78);
    OV5642YUV_write_cmos_sensor(0x8731, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8732, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8733, 0x07);
    OV5642YUV_write_cmos_sensor(0x8734, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8735, 0x18);
    OV5642YUV_write_cmos_sensor(0x8736, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8737, 0x64);
    OV5642YUV_write_cmos_sensor(0x8738, 0x80);
    OV5642YUV_write_cmos_sensor(0x8739, 0x94);
    OV5642YUV_write_cmos_sensor(0x873a, 0xb3);
    OV5642YUV_write_cmos_sensor(0x873b, 0x50);
    OV5642YUV_write_cmos_sensor(0x873c, 0x04);
    OV5642YUV_write_cmos_sensor(0x873d, 0xe6);
    OV5642YUV_write_cmos_sensor(0x873e, 0xff);
    OV5642YUV_write_cmos_sensor(0x873f, 0x80);
    OV5642YUV_write_cmos_sensor(0x8740, 0x02);
    OV5642YUV_write_cmos_sensor(0x8741, 0x7f);
    OV5642YUV_write_cmos_sensor(0x8742, 0x33);
    OV5642YUV_write_cmos_sensor(0x8743, 0x78);
    OV5642YUV_write_cmos_sensor(0x8744, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8745, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8746, 0x07);
    OV5642YUV_write_cmos_sensor(0x8747, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8748, 0x08);
    OV5642YUV_write_cmos_sensor(0x8749, 0xe6);
    OV5642YUV_write_cmos_sensor(0x874a, 0x64);
    OV5642YUV_write_cmos_sensor(0x874b, 0x80);
    OV5642YUV_write_cmos_sensor(0x874c, 0x94);
    OV5642YUV_write_cmos_sensor(0x874d, 0xb3);
    OV5642YUV_write_cmos_sensor(0x874e, 0x50);
    OV5642YUV_write_cmos_sensor(0x874f, 0x04);
    OV5642YUV_write_cmos_sensor(0x8750, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8751, 0xff);
    OV5642YUV_write_cmos_sensor(0x8752, 0x80);
    OV5642YUV_write_cmos_sensor(0x8753, 0x02);
    OV5642YUV_write_cmos_sensor(0x8754, 0x7f);
    OV5642YUV_write_cmos_sensor(0x8755, 0x33);
    OV5642YUV_write_cmos_sensor(0x8756, 0x78);
    OV5642YUV_write_cmos_sensor(0x8757, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8758, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8759, 0x07);
    OV5642YUV_write_cmos_sensor(0x875a, 0x12);
    OV5642YUV_write_cmos_sensor(0x875b, 0x11);
    OV5642YUV_write_cmos_sensor(0x875c, 0xcb);
    OV5642YUV_write_cmos_sensor(0x875d, 0x40);
    OV5642YUV_write_cmos_sensor(0x875e, 0x06);
    OV5642YUV_write_cmos_sensor(0x875f, 0x78);
    OV5642YUV_write_cmos_sensor(0x8760, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8761, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8762, 0xff);
    OV5642YUV_write_cmos_sensor(0x8763, 0x80);
    OV5642YUV_write_cmos_sensor(0x8764, 0x04);
    OV5642YUV_write_cmos_sensor(0x8765, 0x78);
    OV5642YUV_write_cmos_sensor(0x8766, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8767, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8768, 0xff);
    OV5642YUV_write_cmos_sensor(0x8769, 0x78);
    OV5642YUV_write_cmos_sensor(0x876a, 0xc1);
    OV5642YUV_write_cmos_sensor(0x876b, 0xa6);
    OV5642YUV_write_cmos_sensor(0x876c, 0x07);
    OV5642YUV_write_cmos_sensor(0x876d, 0x75);
    OV5642YUV_write_cmos_sensor(0x876e, 0x1f);
    OV5642YUV_write_cmos_sensor(0x876f, 0x03);
    OV5642YUV_write_cmos_sensor(0x8770, 0x78);
    OV5642YUV_write_cmos_sensor(0x8771, 0xbb);
    OV5642YUV_write_cmos_sensor(0x8772, 0x76);
    OV5642YUV_write_cmos_sensor(0x8773, 0x01);
    OV5642YUV_write_cmos_sensor(0x8774, 0x80);
    OV5642YUV_write_cmos_sensor(0x8775, 0x20);
    OV5642YUV_write_cmos_sensor(0x8776, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8777, 0x1f);
    OV5642YUV_write_cmos_sensor(0x8778, 0x64);
    OV5642YUV_write_cmos_sensor(0x8779, 0x03);
    OV5642YUV_write_cmos_sensor(0x877a, 0x70);
    OV5642YUV_write_cmos_sensor(0x877b, 0x26);
    OV5642YUV_write_cmos_sensor(0x877c, 0x78);
    OV5642YUV_write_cmos_sensor(0x877d, 0xc1);
    OV5642YUV_write_cmos_sensor(0x877e, 0xe6);
    OV5642YUV_write_cmos_sensor(0x877f, 0xff);
    OV5642YUV_write_cmos_sensor(0x8780, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8781, 0x78);
    OV5642YUV_write_cmos_sensor(0x8782, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8783, 0x12);
    OV5642YUV_write_cmos_sensor(0x8784, 0x11);
    OV5642YUV_write_cmos_sensor(0x8785, 0xa9);
    OV5642YUV_write_cmos_sensor(0x8786, 0x40);
    OV5642YUV_write_cmos_sensor(0x8787, 0x05);
    OV5642YUV_write_cmos_sensor(0x8788, 0x12);
    OV5642YUV_write_cmos_sensor(0x8789, 0x11);
    OV5642YUV_write_cmos_sensor(0x878a, 0xa3);
    OV5642YUV_write_cmos_sensor(0x878b, 0x40);
    OV5642YUV_write_cmos_sensor(0x878c, 0x09);
    OV5642YUV_write_cmos_sensor(0x878d, 0x78);
    OV5642YUV_write_cmos_sensor(0x878e, 0xbc);
    OV5642YUV_write_cmos_sensor(0x878f, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8790, 0x78);
    OV5642YUV_write_cmos_sensor(0x8791, 0xc1);
    OV5642YUV_write_cmos_sensor(0x8792, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8793, 0x75);
    OV5642YUV_write_cmos_sensor(0x8794, 0x1f);
    OV5642YUV_write_cmos_sensor(0x8795, 0x04);
    OV5642YUV_write_cmos_sensor(0x8796, 0x78);
    OV5642YUV_write_cmos_sensor(0x8797, 0xc1);
    OV5642YUV_write_cmos_sensor(0x8798, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8799, 0x75);
    OV5642YUV_write_cmos_sensor(0x879a, 0xf0);
    OV5642YUV_write_cmos_sensor(0x879b, 0x05);
    OV5642YUV_write_cmos_sensor(0x879c, 0xa4);
    OV5642YUV_write_cmos_sensor(0x879d, 0xf5);
    OV5642YUV_write_cmos_sensor(0x879e, 0x4e);
    OV5642YUV_write_cmos_sensor(0x879f, 0x02);
    OV5642YUV_write_cmos_sensor(0x87a0, 0x0d);
    OV5642YUV_write_cmos_sensor(0x87a1, 0x6d);
    OV5642YUV_write_cmos_sensor(0x87a2, 0xe5);
    OV5642YUV_write_cmos_sensor(0x87a3, 0x1f);
    OV5642YUV_write_cmos_sensor(0x87a4, 0xb4);
    OV5642YUV_write_cmos_sensor(0x87a5, 0x04);
    OV5642YUV_write_cmos_sensor(0x87a6, 0x10);
    OV5642YUV_write_cmos_sensor(0x87a7, 0x90);
    OV5642YUV_write_cmos_sensor(0x87a8, 0x0e);
    OV5642YUV_write_cmos_sensor(0x87a9, 0xab);
    OV5642YUV_write_cmos_sensor(0x87aa, 0xe4);
    OV5642YUV_write_cmos_sensor(0x87ab, 0x78);
    OV5642YUV_write_cmos_sensor(0x87ac, 0xc6);
    OV5642YUV_write_cmos_sensor(0x87ad, 0x12);
    OV5642YUV_write_cmos_sensor(0x87ae, 0x11);
    OV5642YUV_write_cmos_sensor(0x87af, 0xb2);
    OV5642YUV_write_cmos_sensor(0x87b0, 0x40);
    OV5642YUV_write_cmos_sensor(0x87b1, 0x02);
    OV5642YUV_write_cmos_sensor(0x87b2, 0xd2);
    OV5642YUV_write_cmos_sensor(0x87b3, 0x37);
    OV5642YUV_write_cmos_sensor(0x87b4, 0x75);
    OV5642YUV_write_cmos_sensor(0x87b5, 0x1f);
    OV5642YUV_write_cmos_sensor(0x87b6, 0x05);
    OV5642YUV_write_cmos_sensor(0x87b7, 0x22);
    OV5642YUV_write_cmos_sensor(0x87b8, 0xbb);
    OV5642YUV_write_cmos_sensor(0x87b9, 0x01);
    OV5642YUV_write_cmos_sensor(0x87ba, 0x06);
    OV5642YUV_write_cmos_sensor(0x87bb, 0x89);
    OV5642YUV_write_cmos_sensor(0x87bc, 0x82);
    OV5642YUV_write_cmos_sensor(0x87bd, 0x8a);
    OV5642YUV_write_cmos_sensor(0x87be, 0x83);
    OV5642YUV_write_cmos_sensor(0x87bf, 0xe0);
    OV5642YUV_write_cmos_sensor(0x87c0, 0x22);
    OV5642YUV_write_cmos_sensor(0x87c1, 0x50);
    OV5642YUV_write_cmos_sensor(0x87c2, 0x02);
    OV5642YUV_write_cmos_sensor(0x87c3, 0xe7);
    OV5642YUV_write_cmos_sensor(0x87c4, 0x22);
    OV5642YUV_write_cmos_sensor(0x87c5, 0xbb);
    OV5642YUV_write_cmos_sensor(0x87c6, 0xfe);
    OV5642YUV_write_cmos_sensor(0x87c7, 0x02);
    OV5642YUV_write_cmos_sensor(0x87c8, 0xe3);
    OV5642YUV_write_cmos_sensor(0x87c9, 0x22);
    OV5642YUV_write_cmos_sensor(0x87ca, 0x89);
    OV5642YUV_write_cmos_sensor(0x87cb, 0x82);
    OV5642YUV_write_cmos_sensor(0x87cc, 0x8a);
    OV5642YUV_write_cmos_sensor(0x87cd, 0x83);
    OV5642YUV_write_cmos_sensor(0x87ce, 0xe4);
    OV5642YUV_write_cmos_sensor(0x87cf, 0x93);
    OV5642YUV_write_cmos_sensor(0x87d0, 0x22);
    OV5642YUV_write_cmos_sensor(0x87d1, 0xbb);
    OV5642YUV_write_cmos_sensor(0x87d2, 0x01);
    OV5642YUV_write_cmos_sensor(0x87d3, 0x06);
    OV5642YUV_write_cmos_sensor(0x87d4, 0x89);
    OV5642YUV_write_cmos_sensor(0x87d5, 0x82);
    OV5642YUV_write_cmos_sensor(0x87d6, 0x8a);
    OV5642YUV_write_cmos_sensor(0x87d7, 0x83);
    OV5642YUV_write_cmos_sensor(0x87d8, 0xf0);
    OV5642YUV_write_cmos_sensor(0x87d9, 0x22);
    OV5642YUV_write_cmos_sensor(0x87da, 0x50);
    OV5642YUV_write_cmos_sensor(0x87db, 0x02);
    OV5642YUV_write_cmos_sensor(0x87dc, 0xf7);
    OV5642YUV_write_cmos_sensor(0x87dd, 0x22);
    OV5642YUV_write_cmos_sensor(0x87de, 0xbb);
    OV5642YUV_write_cmos_sensor(0x87df, 0xfe);
    OV5642YUV_write_cmos_sensor(0x87e0, 0x01);
    OV5642YUV_write_cmos_sensor(0x87e1, 0xf3);
    OV5642YUV_write_cmos_sensor(0x87e2, 0x22);
    OV5642YUV_write_cmos_sensor(0x87e3, 0xef);
    OV5642YUV_write_cmos_sensor(0x87e4, 0x8d);
    OV5642YUV_write_cmos_sensor(0x87e5, 0xf0);
    OV5642YUV_write_cmos_sensor(0x87e6, 0xa4);
    OV5642YUV_write_cmos_sensor(0x87e7, 0xa8);
    OV5642YUV_write_cmos_sensor(0x87e8, 0xf0);
    OV5642YUV_write_cmos_sensor(0x87e9, 0xcf);
    OV5642YUV_write_cmos_sensor(0x87ea, 0x8c);
    OV5642YUV_write_cmos_sensor(0x87eb, 0xf0);
    OV5642YUV_write_cmos_sensor(0x87ec, 0xa4);
    OV5642YUV_write_cmos_sensor(0x87ed, 0x28);
    OV5642YUV_write_cmos_sensor(0x87ee, 0xce);
    OV5642YUV_write_cmos_sensor(0x87ef, 0x8d);
    OV5642YUV_write_cmos_sensor(0x87f0, 0xf0);
    OV5642YUV_write_cmos_sensor(0x87f1, 0xa4);
    OV5642YUV_write_cmos_sensor(0x87f2, 0x2e);
    OV5642YUV_write_cmos_sensor(0x87f3, 0xfe);
    OV5642YUV_write_cmos_sensor(0x87f4, 0x22);
    OV5642YUV_write_cmos_sensor(0x87f5, 0xbc);
    OV5642YUV_write_cmos_sensor(0x87f6, 0x00);
    OV5642YUV_write_cmos_sensor(0x87f7, 0x0b);
    OV5642YUV_write_cmos_sensor(0x87f8, 0xbe);
    OV5642YUV_write_cmos_sensor(0x87f9, 0x00);
    OV5642YUV_write_cmos_sensor(0x87fa, 0x29);
    OV5642YUV_write_cmos_sensor(0x87fb, 0xef);
    OV5642YUV_write_cmos_sensor(0x87fc, 0x8d);
    OV5642YUV_write_cmos_sensor(0x87fd, 0xf0);
    OV5642YUV_write_cmos_sensor(0x87fe, 0x84);
    OV5642YUV_write_cmos_sensor(0x87ff, 0xff);
    OV5642YUV_write_cmos_sensor(0x8800, 0xad);
    OV5642YUV_write_cmos_sensor(0x8801, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8802, 0x22);
    OV5642YUV_write_cmos_sensor(0x8803, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8804, 0xcc);
    OV5642YUV_write_cmos_sensor(0x8805, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8806, 0x75);
    OV5642YUV_write_cmos_sensor(0x8807, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8808, 0x08);
    OV5642YUV_write_cmos_sensor(0x8809, 0xef);
    OV5642YUV_write_cmos_sensor(0x880a, 0x2f);
    OV5642YUV_write_cmos_sensor(0x880b, 0xff);
    OV5642YUV_write_cmos_sensor(0x880c, 0xee);
    OV5642YUV_write_cmos_sensor(0x880d, 0x33);
    OV5642YUV_write_cmos_sensor(0x880e, 0xfe);
    OV5642YUV_write_cmos_sensor(0x880f, 0xec);
    OV5642YUV_write_cmos_sensor(0x8810, 0x33);
    OV5642YUV_write_cmos_sensor(0x8811, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8812, 0xee);
    OV5642YUV_write_cmos_sensor(0x8813, 0x9d);
    OV5642YUV_write_cmos_sensor(0x8814, 0xec);
    OV5642YUV_write_cmos_sensor(0x8815, 0x98);
    OV5642YUV_write_cmos_sensor(0x8816, 0x40);
    OV5642YUV_write_cmos_sensor(0x8817, 0x05);
    OV5642YUV_write_cmos_sensor(0x8818, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8819, 0xee);
    OV5642YUV_write_cmos_sensor(0x881a, 0x9d);
    OV5642YUV_write_cmos_sensor(0x881b, 0xfe);
    OV5642YUV_write_cmos_sensor(0x881c, 0x0f);
    OV5642YUV_write_cmos_sensor(0x881d, 0xd5);
    OV5642YUV_write_cmos_sensor(0x881e, 0xf0);
    OV5642YUV_write_cmos_sensor(0x881f, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8820, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8821, 0xce);
    OV5642YUV_write_cmos_sensor(0x8822, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8823, 0x22);
    OV5642YUV_write_cmos_sensor(0x8824, 0xed);
    OV5642YUV_write_cmos_sensor(0x8825, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8826, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8827, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8828, 0xee);
    OV5642YUV_write_cmos_sensor(0x8829, 0x84);
    OV5642YUV_write_cmos_sensor(0x882a, 0x20);
    OV5642YUV_write_cmos_sensor(0x882b, 0xd2);
    OV5642YUV_write_cmos_sensor(0x882c, 0x1c);
    OV5642YUV_write_cmos_sensor(0x882d, 0xfe);
    OV5642YUV_write_cmos_sensor(0x882e, 0xad);
    OV5642YUV_write_cmos_sensor(0x882f, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8830, 0x75);
    OV5642YUV_write_cmos_sensor(0x8831, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8832, 0x08);
    OV5642YUV_write_cmos_sensor(0x8833, 0xef);
    OV5642YUV_write_cmos_sensor(0x8834, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8835, 0xff);
    OV5642YUV_write_cmos_sensor(0x8836, 0xed);
    OV5642YUV_write_cmos_sensor(0x8837, 0x33);
    OV5642YUV_write_cmos_sensor(0x8838, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8839, 0x40);
    OV5642YUV_write_cmos_sensor(0x883a, 0x07);
    OV5642YUV_write_cmos_sensor(0x883b, 0x98);
    OV5642YUV_write_cmos_sensor(0x883c, 0x50);
    OV5642YUV_write_cmos_sensor(0x883d, 0x06);
    OV5642YUV_write_cmos_sensor(0x883e, 0xd5);
    OV5642YUV_write_cmos_sensor(0x883f, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8840, 0xf2);
    OV5642YUV_write_cmos_sensor(0x8841, 0x22);
    OV5642YUV_write_cmos_sensor(0x8842, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8843, 0x98);
    OV5642YUV_write_cmos_sensor(0x8844, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8845, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8846, 0xd5);
    OV5642YUV_write_cmos_sensor(0x8847, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8848, 0xea);
    OV5642YUV_write_cmos_sensor(0x8849, 0x22);
    OV5642YUV_write_cmos_sensor(0x884a, 0xe8);
    OV5642YUV_write_cmos_sensor(0x884b, 0x8f);
    OV5642YUV_write_cmos_sensor(0x884c, 0xf0);
    OV5642YUV_write_cmos_sensor(0x884d, 0xa4);
    OV5642YUV_write_cmos_sensor(0x884e, 0xcc);
    OV5642YUV_write_cmos_sensor(0x884f, 0x8b);
    OV5642YUV_write_cmos_sensor(0x8850, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8851, 0xa4);
    OV5642YUV_write_cmos_sensor(0x8852, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8853, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8854, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8855, 0x8e);
    OV5642YUV_write_cmos_sensor(0x8856, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8857, 0xa4);
    OV5642YUV_write_cmos_sensor(0x8858, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8859, 0xfc);
    OV5642YUV_write_cmos_sensor(0x885a, 0x8a);
    OV5642YUV_write_cmos_sensor(0x885b, 0xf0);
    OV5642YUV_write_cmos_sensor(0x885c, 0xed);
    OV5642YUV_write_cmos_sensor(0x885d, 0xa4);
    OV5642YUV_write_cmos_sensor(0x885e, 0x2c);
    OV5642YUV_write_cmos_sensor(0x885f, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8860, 0xea);
    OV5642YUV_write_cmos_sensor(0x8861, 0x8e);
    OV5642YUV_write_cmos_sensor(0x8862, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8863, 0xa4);
    OV5642YUV_write_cmos_sensor(0x8864, 0xcd);
    OV5642YUV_write_cmos_sensor(0x8865, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8866, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8867, 0x8b);
    OV5642YUV_write_cmos_sensor(0x8868, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8869, 0xa4);
    OV5642YUV_write_cmos_sensor(0x886a, 0x2d);
    OV5642YUV_write_cmos_sensor(0x886b, 0xcc);
    OV5642YUV_write_cmos_sensor(0x886c, 0x38);
    OV5642YUV_write_cmos_sensor(0x886d, 0x25);
    OV5642YUV_write_cmos_sensor(0x886e, 0xf0);
    OV5642YUV_write_cmos_sensor(0x886f, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8870, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8871, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8872, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8873, 0xa4);
    OV5642YUV_write_cmos_sensor(0x8874, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8875, 0xcd);
    OV5642YUV_write_cmos_sensor(0x8876, 0x35);
    OV5642YUV_write_cmos_sensor(0x8877, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8878, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8879, 0xeb);
    OV5642YUV_write_cmos_sensor(0x887a, 0x8e);
    OV5642YUV_write_cmos_sensor(0x887b, 0xf0);
    OV5642YUV_write_cmos_sensor(0x887c, 0xa4);
    OV5642YUV_write_cmos_sensor(0x887d, 0xfe);
    OV5642YUV_write_cmos_sensor(0x887e, 0xa9);
    OV5642YUV_write_cmos_sensor(0x887f, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8880, 0xeb);
    OV5642YUV_write_cmos_sensor(0x8881, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8882, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8883, 0xa4);
    OV5642YUV_write_cmos_sensor(0x8884, 0xcf);
    OV5642YUV_write_cmos_sensor(0x8885, 0xc5);
    OV5642YUV_write_cmos_sensor(0x8886, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8887, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8888, 0xcd);
    OV5642YUV_write_cmos_sensor(0x8889, 0x39);
    OV5642YUV_write_cmos_sensor(0x888a, 0xfe);
    OV5642YUV_write_cmos_sensor(0x888b, 0xe4);
    OV5642YUV_write_cmos_sensor(0x888c, 0x3c);
    OV5642YUV_write_cmos_sensor(0x888d, 0xfc);
    OV5642YUV_write_cmos_sensor(0x888e, 0xea);
    OV5642YUV_write_cmos_sensor(0x888f, 0xa4);
    OV5642YUV_write_cmos_sensor(0x8890, 0x2d);
    OV5642YUV_write_cmos_sensor(0x8891, 0xce);
    OV5642YUV_write_cmos_sensor(0x8892, 0x35);
    OV5642YUV_write_cmos_sensor(0x8893, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8894, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8895, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8896, 0x3c);
    OV5642YUV_write_cmos_sensor(0x8897, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8898, 0x22);
    OV5642YUV_write_cmos_sensor(0x8899, 0x75);
    OV5642YUV_write_cmos_sensor(0x889a, 0xf0);
    OV5642YUV_write_cmos_sensor(0x889b, 0x08);
    OV5642YUV_write_cmos_sensor(0x889c, 0x75);
    OV5642YUV_write_cmos_sensor(0x889d, 0x82);
    OV5642YUV_write_cmos_sensor(0x889e, 0x00);
    OV5642YUV_write_cmos_sensor(0x889f, 0xef);
    OV5642YUV_write_cmos_sensor(0x88a0, 0x2f);
    OV5642YUV_write_cmos_sensor(0x88a1, 0xff);
    OV5642YUV_write_cmos_sensor(0x88a2, 0xee);
    OV5642YUV_write_cmos_sensor(0x88a3, 0x33);
    OV5642YUV_write_cmos_sensor(0x88a4, 0xfe);
    OV5642YUV_write_cmos_sensor(0x88a5, 0xcd);
    OV5642YUV_write_cmos_sensor(0x88a6, 0x33);
    OV5642YUV_write_cmos_sensor(0x88a7, 0xcd);
    OV5642YUV_write_cmos_sensor(0x88a8, 0xcc);
    OV5642YUV_write_cmos_sensor(0x88a9, 0x33);
    OV5642YUV_write_cmos_sensor(0x88aa, 0xcc);
    OV5642YUV_write_cmos_sensor(0x88ab, 0xc5);
    OV5642YUV_write_cmos_sensor(0x88ac, 0x82);
    OV5642YUV_write_cmos_sensor(0x88ad, 0x33);
    OV5642YUV_write_cmos_sensor(0x88ae, 0xc5);
    OV5642YUV_write_cmos_sensor(0x88af, 0x82);
    OV5642YUV_write_cmos_sensor(0x88b0, 0x9b);
    OV5642YUV_write_cmos_sensor(0x88b1, 0xed);
    OV5642YUV_write_cmos_sensor(0x88b2, 0x9a);
    OV5642YUV_write_cmos_sensor(0x88b3, 0xec);
    OV5642YUV_write_cmos_sensor(0x88b4, 0x99);
    OV5642YUV_write_cmos_sensor(0x88b5, 0xe5);
    OV5642YUV_write_cmos_sensor(0x88b6, 0x82);
    OV5642YUV_write_cmos_sensor(0x88b7, 0x98);
    OV5642YUV_write_cmos_sensor(0x88b8, 0x40);
    OV5642YUV_write_cmos_sensor(0x88b9, 0x0c);
    OV5642YUV_write_cmos_sensor(0x88ba, 0xf5);
    OV5642YUV_write_cmos_sensor(0x88bb, 0x82);
    OV5642YUV_write_cmos_sensor(0x88bc, 0xee);
    OV5642YUV_write_cmos_sensor(0x88bd, 0x9b);
    OV5642YUV_write_cmos_sensor(0x88be, 0xfe);
    OV5642YUV_write_cmos_sensor(0x88bf, 0xed);
    OV5642YUV_write_cmos_sensor(0x88c0, 0x9a);
    OV5642YUV_write_cmos_sensor(0x88c1, 0xfd);
    OV5642YUV_write_cmos_sensor(0x88c2, 0xec);
    OV5642YUV_write_cmos_sensor(0x88c3, 0x99);
    OV5642YUV_write_cmos_sensor(0x88c4, 0xfc);
    OV5642YUV_write_cmos_sensor(0x88c5, 0x0f);
    OV5642YUV_write_cmos_sensor(0x88c6, 0xd5);
    OV5642YUV_write_cmos_sensor(0x88c7, 0xf0);
    OV5642YUV_write_cmos_sensor(0x88c8, 0xd6);
    OV5642YUV_write_cmos_sensor(0x88c9, 0xe4);
    OV5642YUV_write_cmos_sensor(0x88ca, 0xce);
    OV5642YUV_write_cmos_sensor(0x88cb, 0xfb);
    OV5642YUV_write_cmos_sensor(0x88cc, 0xe4);
    OV5642YUV_write_cmos_sensor(0x88cd, 0xcd);
    OV5642YUV_write_cmos_sensor(0x88ce, 0xfa);
    OV5642YUV_write_cmos_sensor(0x88cf, 0xe4);
    OV5642YUV_write_cmos_sensor(0x88d0, 0xcc);
    OV5642YUV_write_cmos_sensor(0x88d1, 0xf9);
    OV5642YUV_write_cmos_sensor(0x88d2, 0xa8);
    OV5642YUV_write_cmos_sensor(0x88d3, 0x82);
    OV5642YUV_write_cmos_sensor(0x88d4, 0x22);
    OV5642YUV_write_cmos_sensor(0x88d5, 0xb8);
    OV5642YUV_write_cmos_sensor(0x88d6, 0x00);
    OV5642YUV_write_cmos_sensor(0x88d7, 0xc1);
    OV5642YUV_write_cmos_sensor(0x88d8, 0xb9);
    OV5642YUV_write_cmos_sensor(0x88d9, 0x00);
    OV5642YUV_write_cmos_sensor(0x88da, 0x59);
    OV5642YUV_write_cmos_sensor(0x88db, 0xba);
    OV5642YUV_write_cmos_sensor(0x88dc, 0x00);
    OV5642YUV_write_cmos_sensor(0x88dd, 0x2d);
    OV5642YUV_write_cmos_sensor(0x88de, 0xec);
    OV5642YUV_write_cmos_sensor(0x88df, 0x8b);
    OV5642YUV_write_cmos_sensor(0x88e0, 0xf0);
    OV5642YUV_write_cmos_sensor(0x88e1, 0x84);
    OV5642YUV_write_cmos_sensor(0x88e2, 0xcf);
    OV5642YUV_write_cmos_sensor(0x88e3, 0xce);
    OV5642YUV_write_cmos_sensor(0x88e4, 0xcd);
    OV5642YUV_write_cmos_sensor(0x88e5, 0xfc);
    OV5642YUV_write_cmos_sensor(0x88e6, 0xe5);
    OV5642YUV_write_cmos_sensor(0x88e7, 0xf0);
    OV5642YUV_write_cmos_sensor(0x88e8, 0xcb);
    OV5642YUV_write_cmos_sensor(0x88e9, 0xf9);
    OV5642YUV_write_cmos_sensor(0x88ea, 0x78);
    OV5642YUV_write_cmos_sensor(0x88eb, 0x18);
    OV5642YUV_write_cmos_sensor(0x88ec, 0xef);
    OV5642YUV_write_cmos_sensor(0x88ed, 0x2f);
    OV5642YUV_write_cmos_sensor(0x88ee, 0xff);
    OV5642YUV_write_cmos_sensor(0x88ef, 0xee);
    OV5642YUV_write_cmos_sensor(0x88f0, 0x33);
    OV5642YUV_write_cmos_sensor(0x88f1, 0xfe);
    OV5642YUV_write_cmos_sensor(0x88f2, 0xed);
    OV5642YUV_write_cmos_sensor(0x88f3, 0x33);
    OV5642YUV_write_cmos_sensor(0x88f4, 0xfd);
    OV5642YUV_write_cmos_sensor(0x88f5, 0xec);
    OV5642YUV_write_cmos_sensor(0x88f6, 0x33);
    OV5642YUV_write_cmos_sensor(0x88f7, 0xfc);
    OV5642YUV_write_cmos_sensor(0x88f8, 0xeb);
    OV5642YUV_write_cmos_sensor(0x88f9, 0x33);
    OV5642YUV_write_cmos_sensor(0x88fa, 0xfb);
    OV5642YUV_write_cmos_sensor(0x88fb, 0x10);
    OV5642YUV_write_cmos_sensor(0x88fc, 0xd7);
    OV5642YUV_write_cmos_sensor(0x88fd, 0x03);
    OV5642YUV_write_cmos_sensor(0x88fe, 0x99);
    OV5642YUV_write_cmos_sensor(0x88ff, 0x40);
    OV5642YUV_write_cmos_sensor(0x8900, 0x04);
    OV5642YUV_write_cmos_sensor(0x8901, 0xeb);
    OV5642YUV_write_cmos_sensor(0x8902, 0x99);
    OV5642YUV_write_cmos_sensor(0x8903, 0xfb);
    OV5642YUV_write_cmos_sensor(0x8904, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8905, 0xd8);
    OV5642YUV_write_cmos_sensor(0x8906, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8907, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8908, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8909, 0xfa);
    OV5642YUV_write_cmos_sensor(0x890a, 0x22);
    OV5642YUV_write_cmos_sensor(0x890b, 0x78);
    OV5642YUV_write_cmos_sensor(0x890c, 0x18);
    OV5642YUV_write_cmos_sensor(0x890d, 0xef);
    OV5642YUV_write_cmos_sensor(0x890e, 0x2f);
    OV5642YUV_write_cmos_sensor(0x890f, 0xff);
    OV5642YUV_write_cmos_sensor(0x8910, 0xee);
    OV5642YUV_write_cmos_sensor(0x8911, 0x33);
    OV5642YUV_write_cmos_sensor(0x8912, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8913, 0xed);
    OV5642YUV_write_cmos_sensor(0x8914, 0x33);
    OV5642YUV_write_cmos_sensor(0x8915, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8916, 0xec);
    OV5642YUV_write_cmos_sensor(0x8917, 0x33);
    OV5642YUV_write_cmos_sensor(0x8918, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8919, 0xc9);
    OV5642YUV_write_cmos_sensor(0x891a, 0x33);
    OV5642YUV_write_cmos_sensor(0x891b, 0xc9);
    OV5642YUV_write_cmos_sensor(0x891c, 0x10);
    OV5642YUV_write_cmos_sensor(0x891d, 0xd7);
    OV5642YUV_write_cmos_sensor(0x891e, 0x05);
    OV5642YUV_write_cmos_sensor(0x891f, 0x9b);
    OV5642YUV_write_cmos_sensor(0x8920, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8921, 0x9a);
    OV5642YUV_write_cmos_sensor(0x8922, 0x40);
    OV5642YUV_write_cmos_sensor(0x8923, 0x07);
    OV5642YUV_write_cmos_sensor(0x8924, 0xec);
    OV5642YUV_write_cmos_sensor(0x8925, 0x9b);
    OV5642YUV_write_cmos_sensor(0x8926, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8927, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8928, 0x9a);
    OV5642YUV_write_cmos_sensor(0x8929, 0xf9);
    OV5642YUV_write_cmos_sensor(0x892a, 0x0f);
    OV5642YUV_write_cmos_sensor(0x892b, 0xd8);
    OV5642YUV_write_cmos_sensor(0x892c, 0xe0);
    OV5642YUV_write_cmos_sensor(0x892d, 0xe4);
    OV5642YUV_write_cmos_sensor(0x892e, 0xc9);
    OV5642YUV_write_cmos_sensor(0x892f, 0xfa);
    OV5642YUV_write_cmos_sensor(0x8930, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8931, 0xcc);
    OV5642YUV_write_cmos_sensor(0x8932, 0xfb);
    OV5642YUV_write_cmos_sensor(0x8933, 0x22);
    OV5642YUV_write_cmos_sensor(0x8934, 0x75);
    OV5642YUV_write_cmos_sensor(0x8935, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8936, 0x10);
    OV5642YUV_write_cmos_sensor(0x8937, 0xef);
    OV5642YUV_write_cmos_sensor(0x8938, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8939, 0xff);
    OV5642YUV_write_cmos_sensor(0x893a, 0xee);
    OV5642YUV_write_cmos_sensor(0x893b, 0x33);
    OV5642YUV_write_cmos_sensor(0x893c, 0xfe);
    OV5642YUV_write_cmos_sensor(0x893d, 0xed);
    OV5642YUV_write_cmos_sensor(0x893e, 0x33);
    OV5642YUV_write_cmos_sensor(0x893f, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8940, 0xcc);
    OV5642YUV_write_cmos_sensor(0x8941, 0x33);
    OV5642YUV_write_cmos_sensor(0x8942, 0xcc);
    OV5642YUV_write_cmos_sensor(0x8943, 0xc8);
    OV5642YUV_write_cmos_sensor(0x8944, 0x33);
    OV5642YUV_write_cmos_sensor(0x8945, 0xc8);
    OV5642YUV_write_cmos_sensor(0x8946, 0x10);
    OV5642YUV_write_cmos_sensor(0x8947, 0xd7);
    OV5642YUV_write_cmos_sensor(0x8948, 0x07);
    OV5642YUV_write_cmos_sensor(0x8949, 0x9b);
    OV5642YUV_write_cmos_sensor(0x894a, 0xec);
    OV5642YUV_write_cmos_sensor(0x894b, 0x9a);
    OV5642YUV_write_cmos_sensor(0x894c, 0xe8);
    OV5642YUV_write_cmos_sensor(0x894d, 0x99);
    OV5642YUV_write_cmos_sensor(0x894e, 0x40);
    OV5642YUV_write_cmos_sensor(0x894f, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8950, 0xed);
    OV5642YUV_write_cmos_sensor(0x8951, 0x9b);
    OV5642YUV_write_cmos_sensor(0x8952, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8953, 0xec);
    OV5642YUV_write_cmos_sensor(0x8954, 0x9a);
    OV5642YUV_write_cmos_sensor(0x8955, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8956, 0xe8);
    OV5642YUV_write_cmos_sensor(0x8957, 0x99);
    OV5642YUV_write_cmos_sensor(0x8958, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8959, 0x0f);
    OV5642YUV_write_cmos_sensor(0x895a, 0xd5);
    OV5642YUV_write_cmos_sensor(0x895b, 0xf0);
    OV5642YUV_write_cmos_sensor(0x895c, 0xda);
    OV5642YUV_write_cmos_sensor(0x895d, 0xe4);
    OV5642YUV_write_cmos_sensor(0x895e, 0xcd);
    OV5642YUV_write_cmos_sensor(0x895f, 0xfb);
    OV5642YUV_write_cmos_sensor(0x8960, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8961, 0xcc);
    OV5642YUV_write_cmos_sensor(0x8962, 0xfa);
    OV5642YUV_write_cmos_sensor(0x8963, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8964, 0xc8);
    OV5642YUV_write_cmos_sensor(0x8965, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8966, 0x22);
    OV5642YUV_write_cmos_sensor(0x8967, 0xeb);
    OV5642YUV_write_cmos_sensor(0x8968, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8969, 0xf5);
    OV5642YUV_write_cmos_sensor(0x896a, 0xf0);
    OV5642YUV_write_cmos_sensor(0x896b, 0xea);
    OV5642YUV_write_cmos_sensor(0x896c, 0x9e);
    OV5642YUV_write_cmos_sensor(0x896d, 0x42);
    OV5642YUV_write_cmos_sensor(0x896e, 0xf0);
    OV5642YUV_write_cmos_sensor(0x896f, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8970, 0x9d);
    OV5642YUV_write_cmos_sensor(0x8971, 0x42);
    OV5642YUV_write_cmos_sensor(0x8972, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8973, 0xe8);
    OV5642YUV_write_cmos_sensor(0x8974, 0x9c);
    OV5642YUV_write_cmos_sensor(0x8975, 0x45);
    OV5642YUV_write_cmos_sensor(0x8976, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8977, 0x22);
    OV5642YUV_write_cmos_sensor(0x8978, 0xe8);
    OV5642YUV_write_cmos_sensor(0x8979, 0x60);
    OV5642YUV_write_cmos_sensor(0x897a, 0x0f);
    OV5642YUV_write_cmos_sensor(0x897b, 0xef);
    OV5642YUV_write_cmos_sensor(0x897c, 0xc3);
    OV5642YUV_write_cmos_sensor(0x897d, 0x33);
    OV5642YUV_write_cmos_sensor(0x897e, 0xff);
    OV5642YUV_write_cmos_sensor(0x897f, 0xee);
    OV5642YUV_write_cmos_sensor(0x8980, 0x33);
    OV5642YUV_write_cmos_sensor(0x8981, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8982, 0xed);
    OV5642YUV_write_cmos_sensor(0x8983, 0x33);
    OV5642YUV_write_cmos_sensor(0x8984, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8985, 0xec);
    OV5642YUV_write_cmos_sensor(0x8986, 0x33);
    OV5642YUV_write_cmos_sensor(0x8987, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8988, 0xd8);
    OV5642YUV_write_cmos_sensor(0x8989, 0xf1);
    OV5642YUV_write_cmos_sensor(0x898a, 0x22);
    OV5642YUV_write_cmos_sensor(0x898b, 0xe4);
    OV5642YUV_write_cmos_sensor(0x898c, 0x93);
    OV5642YUV_write_cmos_sensor(0x898d, 0xfc);
    OV5642YUV_write_cmos_sensor(0x898e, 0x74);
    OV5642YUV_write_cmos_sensor(0x898f, 0x01);
    OV5642YUV_write_cmos_sensor(0x8990, 0x93);
    OV5642YUV_write_cmos_sensor(0x8991, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8992, 0x74);
    OV5642YUV_write_cmos_sensor(0x8993, 0x02);
    OV5642YUV_write_cmos_sensor(0x8994, 0x93);
    OV5642YUV_write_cmos_sensor(0x8995, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8996, 0x74);
    OV5642YUV_write_cmos_sensor(0x8997, 0x03);
    OV5642YUV_write_cmos_sensor(0x8998, 0x93);
    OV5642YUV_write_cmos_sensor(0x8999, 0xff);
    OV5642YUV_write_cmos_sensor(0x899a, 0x22);
    OV5642YUV_write_cmos_sensor(0x899b, 0xe6);
    OV5642YUV_write_cmos_sensor(0x899c, 0xfb);
    OV5642YUV_write_cmos_sensor(0x899d, 0x08);
    OV5642YUV_write_cmos_sensor(0x899e, 0xe6);
    OV5642YUV_write_cmos_sensor(0x899f, 0xf9);
    OV5642YUV_write_cmos_sensor(0x89a0, 0x08);
    OV5642YUV_write_cmos_sensor(0x89a1, 0xe6);
    OV5642YUV_write_cmos_sensor(0x89a2, 0xfa);
    OV5642YUV_write_cmos_sensor(0x89a3, 0x08);
    OV5642YUV_write_cmos_sensor(0x89a4, 0xe6);
    OV5642YUV_write_cmos_sensor(0x89a5, 0xcb);
    OV5642YUV_write_cmos_sensor(0x89a6, 0xf8);
    OV5642YUV_write_cmos_sensor(0x89a7, 0x22);
    OV5642YUV_write_cmos_sensor(0x89a8, 0xec);
    OV5642YUV_write_cmos_sensor(0x89a9, 0xf6);
    OV5642YUV_write_cmos_sensor(0x89aa, 0x08);
    OV5642YUV_write_cmos_sensor(0x89ab, 0xed);
    OV5642YUV_write_cmos_sensor(0x89ac, 0xf6);
    OV5642YUV_write_cmos_sensor(0x89ad, 0x08);
    OV5642YUV_write_cmos_sensor(0x89ae, 0xee);
    OV5642YUV_write_cmos_sensor(0x89af, 0xf6);
    OV5642YUV_write_cmos_sensor(0x89b0, 0x08);
    OV5642YUV_write_cmos_sensor(0x89b1, 0xef);
    OV5642YUV_write_cmos_sensor(0x89b2, 0xf6);
    OV5642YUV_write_cmos_sensor(0x89b3, 0x22);
    OV5642YUV_write_cmos_sensor(0x89b4, 0xa4);
    OV5642YUV_write_cmos_sensor(0x89b5, 0x25);
    OV5642YUV_write_cmos_sensor(0x89b6, 0x82);
    OV5642YUV_write_cmos_sensor(0x89b7, 0xf5);
    OV5642YUV_write_cmos_sensor(0x89b8, 0x82);
    OV5642YUV_write_cmos_sensor(0x89b9, 0xe5);
    OV5642YUV_write_cmos_sensor(0x89ba, 0xf0);
    OV5642YUV_write_cmos_sensor(0x89bb, 0x35);
    OV5642YUV_write_cmos_sensor(0x89bc, 0x83);
    OV5642YUV_write_cmos_sensor(0x89bd, 0xf5);
    OV5642YUV_write_cmos_sensor(0x89be, 0x83);
    OV5642YUV_write_cmos_sensor(0x89bf, 0x22);
    OV5642YUV_write_cmos_sensor(0x89c0, 0xd0);
    OV5642YUV_write_cmos_sensor(0x89c1, 0x83);
    OV5642YUV_write_cmos_sensor(0x89c2, 0xd0);
    OV5642YUV_write_cmos_sensor(0x89c3, 0x82);
    OV5642YUV_write_cmos_sensor(0x89c4, 0xf8);
    OV5642YUV_write_cmos_sensor(0x89c5, 0xe4);
    OV5642YUV_write_cmos_sensor(0x89c6, 0x93);
    OV5642YUV_write_cmos_sensor(0x89c7, 0x70);
    OV5642YUV_write_cmos_sensor(0x89c8, 0x12);
    OV5642YUV_write_cmos_sensor(0x89c9, 0x74);
    OV5642YUV_write_cmos_sensor(0x89ca, 0x01);
    OV5642YUV_write_cmos_sensor(0x89cb, 0x93);
    OV5642YUV_write_cmos_sensor(0x89cc, 0x70);
    OV5642YUV_write_cmos_sensor(0x89cd, 0x0d);
    OV5642YUV_write_cmos_sensor(0x89ce, 0xa3);
    OV5642YUV_write_cmos_sensor(0x89cf, 0xa3);
    OV5642YUV_write_cmos_sensor(0x89d0, 0x93);
    OV5642YUV_write_cmos_sensor(0x89d1, 0xf8);
    OV5642YUV_write_cmos_sensor(0x89d2, 0x74);
    OV5642YUV_write_cmos_sensor(0x89d3, 0x01);
    OV5642YUV_write_cmos_sensor(0x89d4, 0x93);
    OV5642YUV_write_cmos_sensor(0x89d5, 0xf5);
    OV5642YUV_write_cmos_sensor(0x89d6, 0x82);
    OV5642YUV_write_cmos_sensor(0x89d7, 0x88);
    OV5642YUV_write_cmos_sensor(0x89d8, 0x83);
    OV5642YUV_write_cmos_sensor(0x89d9, 0xe4);
    OV5642YUV_write_cmos_sensor(0x89da, 0x73);
    OV5642YUV_write_cmos_sensor(0x89db, 0x74);
    OV5642YUV_write_cmos_sensor(0x89dc, 0x02);
    OV5642YUV_write_cmos_sensor(0x89dd, 0x93);
    OV5642YUV_write_cmos_sensor(0x89de, 0x68);
    OV5642YUV_write_cmos_sensor(0x89df, 0x60);
    OV5642YUV_write_cmos_sensor(0x89e0, 0xef);
    OV5642YUV_write_cmos_sensor(0x89e1, 0xa3);
    OV5642YUV_write_cmos_sensor(0x89e2, 0xa3);
    OV5642YUV_write_cmos_sensor(0x89e3, 0xa3);
    OV5642YUV_write_cmos_sensor(0x89e4, 0x80);
    OV5642YUV_write_cmos_sensor(0x89e5, 0xdf);
    OV5642YUV_write_cmos_sensor(0x89e6, 0x85);
    OV5642YUV_write_cmos_sensor(0x89e7, 0x08);
    OV5642YUV_write_cmos_sensor(0x89e8, 0x46);
    OV5642YUV_write_cmos_sensor(0x89e9, 0x90);
    OV5642YUV_write_cmos_sensor(0x89ea, 0x50);
    OV5642YUV_write_cmos_sensor(0x89eb, 0x82);
    OV5642YUV_write_cmos_sensor(0x89ec, 0xe0);
    OV5642YUV_write_cmos_sensor(0x89ed, 0xf5);
    OV5642YUV_write_cmos_sensor(0x89ee, 0x42);
    OV5642YUV_write_cmos_sensor(0x89ef, 0xa3);
    OV5642YUV_write_cmos_sensor(0x89f0, 0xe0);
    OV5642YUV_write_cmos_sensor(0x89f1, 0xf5);
    OV5642YUV_write_cmos_sensor(0x89f2, 0x43);
    OV5642YUV_write_cmos_sensor(0x89f3, 0xa3);
    OV5642YUV_write_cmos_sensor(0x89f4, 0xe0);
    OV5642YUV_write_cmos_sensor(0x89f5, 0xf5);
    OV5642YUV_write_cmos_sensor(0x89f6, 0x44);
    OV5642YUV_write_cmos_sensor(0x89f7, 0xa3);
    OV5642YUV_write_cmos_sensor(0x89f8, 0xe0);
    OV5642YUV_write_cmos_sensor(0x89f9, 0xf5);
    OV5642YUV_write_cmos_sensor(0x89fa, 0x45);
    OV5642YUV_write_cmos_sensor(0x89fb, 0x90);
    OV5642YUV_write_cmos_sensor(0x89fc, 0x30);
    OV5642YUV_write_cmos_sensor(0x89fd, 0x26);
    OV5642YUV_write_cmos_sensor(0x89fe, 0xe0);
    OV5642YUV_write_cmos_sensor(0x89ff, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8a00, 0x41);
    OV5642YUV_write_cmos_sensor(0x8a01, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8a02, 0x34);
    OV5642YUV_write_cmos_sensor(0x8a03, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8a04, 0x46);
    OV5642YUV_write_cmos_sensor(0x8a05, 0x12);
    OV5642YUV_write_cmos_sensor(0x8a06, 0x09);
    OV5642YUV_write_cmos_sensor(0x8a07, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8a08, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a09, 0x4b);
    OV5642YUV_write_cmos_sensor(0x8a0a, 0x03);
    OV5642YUV_write_cmos_sensor(0x8a0b, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a0c, 0x4f);
    OV5642YUV_write_cmos_sensor(0x8a0d, 0x04);
    OV5642YUV_write_cmos_sensor(0x8a0e, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a0f, 0x55);
    OV5642YUV_write_cmos_sensor(0x8a10, 0x05);
    OV5642YUV_write_cmos_sensor(0x8a11, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a12, 0x58);
    OV5642YUV_write_cmos_sensor(0x8a13, 0x06);
    OV5642YUV_write_cmos_sensor(0x8a14, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a15, 0x5b);
    OV5642YUV_write_cmos_sensor(0x8a16, 0x07);
    OV5642YUV_write_cmos_sensor(0x8a17, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a18, 0x64);
    OV5642YUV_write_cmos_sensor(0x8a19, 0x08);
    OV5642YUV_write_cmos_sensor(0x8a1a, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a1b, 0x75);
    OV5642YUV_write_cmos_sensor(0x8a1c, 0x12);
    OV5642YUV_write_cmos_sensor(0x8a1d, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a1e, 0x8d);
    OV5642YUV_write_cmos_sensor(0x8a1f, 0x18);
    OV5642YUV_write_cmos_sensor(0x8a20, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a21, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8a22, 0x19);
    OV5642YUV_write_cmos_sensor(0x8a23, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a24, 0x78);
    OV5642YUV_write_cmos_sensor(0x8a25, 0x1a);
    OV5642YUV_write_cmos_sensor(0x8a26, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a27, 0x84);
    OV5642YUV_write_cmos_sensor(0x8a28, 0x1b);
    OV5642YUV_write_cmos_sensor(0x8a29, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a2a, 0xce);
    OV5642YUV_write_cmos_sensor(0x8a2b, 0x80);
    OV5642YUV_write_cmos_sensor(0x8a2c, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8a2d, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8a2e, 0x81);
    OV5642YUV_write_cmos_sensor(0x8a2f, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a30, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a31, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8a32, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a33, 0x20);
    OV5642YUV_write_cmos_sensor(0x8a34, 0x90);
    OV5642YUV_write_cmos_sensor(0x8a35, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a36, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a37, 0x91);
    OV5642YUV_write_cmos_sensor(0x8a38, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a39, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a3a, 0x92);
    OV5642YUV_write_cmos_sensor(0x8a3b, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a3c, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a3d, 0x93);
    OV5642YUV_write_cmos_sensor(0x8a3e, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a3f, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a40, 0x94);
    OV5642YUV_write_cmos_sensor(0x8a41, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a42, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a43, 0x98);
    OV5642YUV_write_cmos_sensor(0x8a44, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a45, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8a46, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8a47, 0x00);
    OV5642YUV_write_cmos_sensor(0x8a48, 0x00);
    OV5642YUV_write_cmos_sensor(0x8a49, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a4a, 0x4e);
    OV5642YUV_write_cmos_sensor(0x8a4b, 0x12);
    OV5642YUV_write_cmos_sensor(0x8a4c, 0x11);
    OV5642YUV_write_cmos_sensor(0x8a4d, 0x1c);
    OV5642YUV_write_cmos_sensor(0x8a4e, 0x22);
    OV5642YUV_write_cmos_sensor(0x8a4f, 0x12);
    OV5642YUV_write_cmos_sensor(0x8a50, 0x11);
    OV5642YUV_write_cmos_sensor(0x8a51, 0x1c);
    OV5642YUV_write_cmos_sensor(0x8a52, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8a53, 0x03);
    OV5642YUV_write_cmos_sensor(0x8a54, 0x22);
    OV5642YUV_write_cmos_sensor(0x8a55, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8a56, 0x03);
    OV5642YUV_write_cmos_sensor(0x8a57, 0x22);
    OV5642YUV_write_cmos_sensor(0x8a58, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8a59, 0x03);
    OV5642YUV_write_cmos_sensor(0x8a5a, 0x22);
    OV5642YUV_write_cmos_sensor(0x8a5b, 0xa2);
    OV5642YUV_write_cmos_sensor(0x8a5c, 0x37);
    OV5642YUV_write_cmos_sensor(0x8a5d, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8a5e, 0x33);
    OV5642YUV_write_cmos_sensor(0x8a5f, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8a60, 0x41);
    OV5642YUV_write_cmos_sensor(0x8a61, 0x02);
    OV5642YUV_write_cmos_sensor(0x8a62, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a63, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a64, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8a65, 0x01);
    OV5642YUV_write_cmos_sensor(0x8a66, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8a67, 0x02);
    OV5642YUV_write_cmos_sensor(0x8a68, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8a69, 0x03);
    OV5642YUV_write_cmos_sensor(0x8a6a, 0x12);
    OV5642YUV_write_cmos_sensor(0x8a6b, 0x10);
    OV5642YUV_write_cmos_sensor(0x8a6c, 0x54);
    OV5642YUV_write_cmos_sensor(0x8a6d, 0x75);
    OV5642YUV_write_cmos_sensor(0x8a6e, 0x1e);
    OV5642YUV_write_cmos_sensor(0x8a6f, 0x70);
    OV5642YUV_write_cmos_sensor(0x8a70, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8a71, 0x35);
    OV5642YUV_write_cmos_sensor(0x8a72, 0x02);
    OV5642YUV_write_cmos_sensor(0x8a73, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a74, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a75, 0x02);
    OV5642YUV_write_cmos_sensor(0x8a76, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a77, 0x1b);
    OV5642YUV_write_cmos_sensor(0x8a78, 0x85);
    OV5642YUV_write_cmos_sensor(0x8a79, 0x45);
    OV5642YUV_write_cmos_sensor(0x8a7a, 0x4d);
    OV5642YUV_write_cmos_sensor(0x8a7b, 0x85);
    OV5642YUV_write_cmos_sensor(0x8a7c, 0x41);
    OV5642YUV_write_cmos_sensor(0x8a7d, 0x4e);
    OV5642YUV_write_cmos_sensor(0x8a7e, 0x12);
    OV5642YUV_write_cmos_sensor(0x8a7f, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8a80, 0x6d);
    OV5642YUV_write_cmos_sensor(0x8a81, 0x02);
    OV5642YUV_write_cmos_sensor(0x8a82, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a83, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a84, 0x85);
    OV5642YUV_write_cmos_sensor(0x8a85, 0x4d);
    OV5642YUV_write_cmos_sensor(0x8a86, 0x45);
    OV5642YUV_write_cmos_sensor(0x8a87, 0x85);
    OV5642YUV_write_cmos_sensor(0x8a88, 0x4e);
    OV5642YUV_write_cmos_sensor(0x8a89, 0x41);
    OV5642YUV_write_cmos_sensor(0x8a8a, 0x02);
    OV5642YUV_write_cmos_sensor(0x8a8b, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8a8c, 0x31);
    OV5642YUV_write_cmos_sensor(0x8a8d, 0x12);
    OV5642YUV_write_cmos_sensor(0x8a8e, 0x11);
    OV5642YUV_write_cmos_sensor(0x8a8f, 0x38);
    OV5642YUV_write_cmos_sensor(0x8a90, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8a91, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8a92, 0x22);
    OV5642YUV_write_cmos_sensor(0x8a93, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8a94, 0x23);
    OV5642YUV_write_cmos_sensor(0x8a95, 0x85);
    OV5642YUV_write_cmos_sensor(0x8a96, 0x45);
    OV5642YUV_write_cmos_sensor(0x8a97, 0x40);
    OV5642YUV_write_cmos_sensor(0x8a98, 0x85);
    OV5642YUV_write_cmos_sensor(0x8a99, 0x44);
    OV5642YUV_write_cmos_sensor(0x8a9a, 0x3f);
    OV5642YUV_write_cmos_sensor(0x8a9b, 0x85);
    OV5642YUV_write_cmos_sensor(0x8a9c, 0x43);
    OV5642YUV_write_cmos_sensor(0x8a9d, 0x3e);
    OV5642YUV_write_cmos_sensor(0x8a9e, 0x85);
    OV5642YUV_write_cmos_sensor(0x8a9f, 0x42);
    OV5642YUV_write_cmos_sensor(0x8aa0, 0x3d);
    OV5642YUV_write_cmos_sensor(0x8aa1, 0x12);
    OV5642YUV_write_cmos_sensor(0x8aa2, 0x02);
    OV5642YUV_write_cmos_sensor(0x8aa3, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8aa4, 0x80);
    OV5642YUV_write_cmos_sensor(0x8aa5, 0x22);
    OV5642YUV_write_cmos_sensor(0x8aa6, 0x12);
    OV5642YUV_write_cmos_sensor(0x8aa7, 0x11);
    OV5642YUV_write_cmos_sensor(0x8aa8, 0x38);
    OV5642YUV_write_cmos_sensor(0x8aa9, 0x75);
    OV5642YUV_write_cmos_sensor(0x8aaa, 0x22);
    OV5642YUV_write_cmos_sensor(0x8aab, 0x00);
    OV5642YUV_write_cmos_sensor(0x8aac, 0x75);
    OV5642YUV_write_cmos_sensor(0x8aad, 0x23);
    OV5642YUV_write_cmos_sensor(0x8aae, 0x01);
    OV5642YUV_write_cmos_sensor(0x8aaf, 0x74);
    OV5642YUV_write_cmos_sensor(0x8ab0, 0xff);
    OV5642YUV_write_cmos_sensor(0x8ab1, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ab2, 0x3c);
    OV5642YUV_write_cmos_sensor(0x8ab3, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ab4, 0x3b);
    OV5642YUV_write_cmos_sensor(0x8ab5, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ab6, 0x3a);
    OV5642YUV_write_cmos_sensor(0x8ab7, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ab8, 0x39);
    OV5642YUV_write_cmos_sensor(0x8ab9, 0x12);
    OV5642YUV_write_cmos_sensor(0x8aba, 0x02);
    OV5642YUV_write_cmos_sensor(0x8abb, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8abc, 0x85);
    OV5642YUV_write_cmos_sensor(0x8abd, 0x3c);
    OV5642YUV_write_cmos_sensor(0x8abe, 0x45);
    OV5642YUV_write_cmos_sensor(0x8abf, 0x85);
    OV5642YUV_write_cmos_sensor(0x8ac0, 0x3b);
    OV5642YUV_write_cmos_sensor(0x8ac1, 0x44);
    OV5642YUV_write_cmos_sensor(0x8ac2, 0x85);
    OV5642YUV_write_cmos_sensor(0x8ac3, 0x3a);
    OV5642YUV_write_cmos_sensor(0x8ac4, 0x43);
    OV5642YUV_write_cmos_sensor(0x8ac5, 0x85);
    OV5642YUV_write_cmos_sensor(0x8ac6, 0x39);
    OV5642YUV_write_cmos_sensor(0x8ac7, 0x42);
    OV5642YUV_write_cmos_sensor(0x8ac8, 0x85);
    OV5642YUV_write_cmos_sensor(0x8ac9, 0x38);
    OV5642YUV_write_cmos_sensor(0x8aca, 0x41);
    OV5642YUV_write_cmos_sensor(0x8acb, 0x02);
    OV5642YUV_write_cmos_sensor(0x8acc, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8acd, 0x31);
    OV5642YUV_write_cmos_sensor(0x8ace, 0x12);
    OV5642YUV_write_cmos_sensor(0x8acf, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ad0, 0x18);
    OV5642YUV_write_cmos_sensor(0x8ad1, 0x80);
    OV5642YUV_write_cmos_sensor(0x8ad2, 0x5e);
    OV5642YUV_write_cmos_sensor(0x8ad3, 0x85);
    OV5642YUV_write_cmos_sensor(0x8ad4, 0x42);
    OV5642YUV_write_cmos_sensor(0x8ad5, 0x48);
    OV5642YUV_write_cmos_sensor(0x8ad6, 0x85);
    OV5642YUV_write_cmos_sensor(0x8ad7, 0x43);
    OV5642YUV_write_cmos_sensor(0x8ad8, 0x49);
    OV5642YUV_write_cmos_sensor(0x8ad9, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8ada, 0x4a);
    OV5642YUV_write_cmos_sensor(0x8adb, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8adc, 0x13);
    OV5642YUV_write_cmos_sensor(0x8add, 0xff);
    OV5642YUV_write_cmos_sensor(0x8ade, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8adf, 0x48);
    OV5642YUV_write_cmos_sensor(0x8ae0, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8ae1, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8ae2, 0x50);
    OV5642YUV_write_cmos_sensor(0x8ae3, 0x02);
    OV5642YUV_write_cmos_sensor(0x8ae4, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8ae5, 0x48);
    OV5642YUV_write_cmos_sensor(0x8ae6, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8ae7, 0x4b);
    OV5642YUV_write_cmos_sensor(0x8ae8, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8ae9, 0x13);
    OV5642YUV_write_cmos_sensor(0x8aea, 0xff);
    OV5642YUV_write_cmos_sensor(0x8aeb, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8aec, 0x49);
    OV5642YUV_write_cmos_sensor(0x8aed, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8aee, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8aef, 0x50);
    OV5642YUV_write_cmos_sensor(0x8af0, 0x02);
    OV5642YUV_write_cmos_sensor(0x8af1, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8af2, 0x49);
    OV5642YUV_write_cmos_sensor(0x8af3, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8af4, 0x4a);
    OV5642YUV_write_cmos_sensor(0x8af5, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8af6, 0x13);
    OV5642YUV_write_cmos_sensor(0x8af7, 0xff);
    OV5642YUV_write_cmos_sensor(0x8af8, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8af9, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8afa, 0x48);
    OV5642YUV_write_cmos_sensor(0x8afb, 0x90);
    OV5642YUV_write_cmos_sensor(0x8afc, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8afd, 0x9d);
    OV5642YUV_write_cmos_sensor(0x8afe, 0x12);
    OV5642YUV_write_cmos_sensor(0x8aff, 0x11);
    OV5642YUV_write_cmos_sensor(0x8b00, 0x51);
    OV5642YUV_write_cmos_sensor(0x8b01, 0x40);
    OV5642YUV_write_cmos_sensor(0x8b02, 0x04);
    OV5642YUV_write_cmos_sensor(0x8b03, 0xee);
    OV5642YUV_write_cmos_sensor(0x8b04, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8b05, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8b06, 0x48);
    OV5642YUV_write_cmos_sensor(0x8b07, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8b08, 0x4b);
    OV5642YUV_write_cmos_sensor(0x8b09, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8b0a, 0x13);
    OV5642YUV_write_cmos_sensor(0x8b0b, 0xff);
    OV5642YUV_write_cmos_sensor(0x8b0c, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8b0d, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8b0e, 0x49);
    OV5642YUV_write_cmos_sensor(0x8b0f, 0x90);
    OV5642YUV_write_cmos_sensor(0x8b10, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8b11, 0x9e);
    OV5642YUV_write_cmos_sensor(0x8b12, 0x12);
    OV5642YUV_write_cmos_sensor(0x8b13, 0x11);
    OV5642YUV_write_cmos_sensor(0x8b14, 0x51);
    OV5642YUV_write_cmos_sensor(0x8b15, 0x40);
    OV5642YUV_write_cmos_sensor(0x8b16, 0x04);
    OV5642YUV_write_cmos_sensor(0x8b17, 0xee);
    OV5642YUV_write_cmos_sensor(0x8b18, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8b19, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8b1a, 0x49);
    OV5642YUV_write_cmos_sensor(0x8b1b, 0x12);
    OV5642YUV_write_cmos_sensor(0x8b1c, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8b1d, 0x4f);
    OV5642YUV_write_cmos_sensor(0x8b1e, 0x80);
    OV5642YUV_write_cmos_sensor(0x8b1f, 0x11);
    OV5642YUV_write_cmos_sensor(0x8b20, 0x85);
    OV5642YUV_write_cmos_sensor(0x8b21, 0x45);
    OV5642YUV_write_cmos_sensor(0x8b22, 0x4b);
    OV5642YUV_write_cmos_sensor(0x8b23, 0x85);
    OV5642YUV_write_cmos_sensor(0x8b24, 0x44);
    OV5642YUV_write_cmos_sensor(0x8b25, 0x4a);
    OV5642YUV_write_cmos_sensor(0x8b26, 0x85);
    OV5642YUV_write_cmos_sensor(0x8b27, 0x43);
    OV5642YUV_write_cmos_sensor(0x8b28, 0x49);
    OV5642YUV_write_cmos_sensor(0x8b29, 0x85);
    OV5642YUV_write_cmos_sensor(0x8b2a, 0x42);
    OV5642YUV_write_cmos_sensor(0x8b2b, 0x48);
    OV5642YUV_write_cmos_sensor(0x8b2c, 0x80);
    OV5642YUV_write_cmos_sensor(0x8b2d, 0x03);
    OV5642YUV_write_cmos_sensor(0x8b2e, 0x02);
    OV5642YUV_write_cmos_sensor(0x8b2f, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8b30, 0x4f);
    OV5642YUV_write_cmos_sensor(0x8b31, 0x90);
    OV5642YUV_write_cmos_sensor(0x8b32, 0x50);
    OV5642YUV_write_cmos_sensor(0x8b33, 0x82);
    OV5642YUV_write_cmos_sensor(0x8b34, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8b35, 0x42);
    OV5642YUV_write_cmos_sensor(0x8b36, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8b37, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8b38, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8b39, 0x43);
    OV5642YUV_write_cmos_sensor(0x8b3a, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8b3b, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8b3c, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8b3d, 0x44);
    OV5642YUV_write_cmos_sensor(0x8b3e, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8b3f, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8b40, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8b41, 0x45);
    OV5642YUV_write_cmos_sensor(0x8b42, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8b43, 0x90);
    OV5642YUV_write_cmos_sensor(0x8b44, 0x30);
    OV5642YUV_write_cmos_sensor(0x8b45, 0x26);
    OV5642YUV_write_cmos_sensor(0x8b46, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8b47, 0x41);
    OV5642YUV_write_cmos_sensor(0x8b48, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8b49, 0x90);
    OV5642YUV_write_cmos_sensor(0x8b4a, 0x30);
    OV5642YUV_write_cmos_sensor(0x8b4b, 0x25);
    OV5642YUV_write_cmos_sensor(0x8b4c, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8b4d, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8b4e, 0x22);
    OV5642YUV_write_cmos_sensor(0x8b4f, 0x90);
    OV5642YUV_write_cmos_sensor(0x8b50, 0x38);
    OV5642YUV_write_cmos_sensor(0x8b51, 0x04);
    OV5642YUV_write_cmos_sensor(0x8b52, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8b53, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8b54, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8b55, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8b56, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8b57, 0xed);
    OV5642YUV_write_cmos_sensor(0x8b58, 0xff);
    OV5642YUV_write_cmos_sensor(0x8b59, 0xee);
    OV5642YUV_write_cmos_sensor(0x8b5a, 0x54);
    OV5642YUV_write_cmos_sensor(0x8b5b, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8b5c, 0x78);
    OV5642YUV_write_cmos_sensor(0x8b5d, 0x55);
    OV5642YUV_write_cmos_sensor(0x8b5e, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8b5f, 0xef);
    OV5642YUV_write_cmos_sensor(0x8b60, 0x08);
    OV5642YUV_write_cmos_sensor(0x8b61, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8b62, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8b63, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8b64, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8b65, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8b66, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8b67, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8b68, 0xed);
    OV5642YUV_write_cmos_sensor(0x8b69, 0xff);
    OV5642YUV_write_cmos_sensor(0x8b6a, 0xee);
    OV5642YUV_write_cmos_sensor(0x8b6b, 0x54);
    OV5642YUV_write_cmos_sensor(0x8b6c, 0x07);
    OV5642YUV_write_cmos_sensor(0x8b6d, 0x08);
    OV5642YUV_write_cmos_sensor(0x8b6e, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8b6f, 0xef);
    OV5642YUV_write_cmos_sensor(0x8b70, 0x08);
    OV5642YUV_write_cmos_sensor(0x8b71, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8b72, 0x79);
    OV5642YUV_write_cmos_sensor(0x8b73, 0x55);
    OV5642YUV_write_cmos_sensor(0x8b74, 0x12);
    OV5642YUV_write_cmos_sensor(0x8b75, 0x10);
    OV5642YUV_write_cmos_sensor(0x8b76, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8b77, 0x09);
    OV5642YUV_write_cmos_sensor(0x8b78, 0x12);
    OV5642YUV_write_cmos_sensor(0x8b79, 0x10);
    OV5642YUV_write_cmos_sensor(0x8b7a, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8b7b, 0xaf);
    OV5642YUV_write_cmos_sensor(0x8b7c, 0x4a);
    OV5642YUV_write_cmos_sensor(0x8b7d, 0x12);
    OV5642YUV_write_cmos_sensor(0x8b7e, 0x10);
    OV5642YUV_write_cmos_sensor(0x8b7f, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8b80, 0x7d);
    OV5642YUV_write_cmos_sensor(0x8b81, 0x50);
    OV5642YUV_write_cmos_sensor(0x8b82, 0x12);
    OV5642YUV_write_cmos_sensor(0x8b83, 0x07);
    OV5642YUV_write_cmos_sensor(0x8b84, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8b85, 0x78);
    OV5642YUV_write_cmos_sensor(0x8b86, 0x5d);
    OV5642YUV_write_cmos_sensor(0x8b87, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8b88, 0x06);
    OV5642YUV_write_cmos_sensor(0x8b89, 0x08);
    OV5642YUV_write_cmos_sensor(0x8b8a, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8b8b, 0x07);
    OV5642YUV_write_cmos_sensor(0x8b8c, 0xaf);
    OV5642YUV_write_cmos_sensor(0x8b8d, 0x48);
    OV5642YUV_write_cmos_sensor(0x8b8e, 0x12);
    OV5642YUV_write_cmos_sensor(0x8b8f, 0x10);
    OV5642YUV_write_cmos_sensor(0x8b90, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8b91, 0x7d);
    OV5642YUV_write_cmos_sensor(0x8b92, 0x50);
    OV5642YUV_write_cmos_sensor(0x8b93, 0x12);
    OV5642YUV_write_cmos_sensor(0x8b94, 0x07);
    OV5642YUV_write_cmos_sensor(0x8b95, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8b96, 0x78);
    OV5642YUV_write_cmos_sensor(0x8b97, 0x59);
    OV5642YUV_write_cmos_sensor(0x8b98, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8b99, 0x06);
    OV5642YUV_write_cmos_sensor(0x8b9a, 0x08);
    OV5642YUV_write_cmos_sensor(0x8b9b, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8b9c, 0x07);
    OV5642YUV_write_cmos_sensor(0x8b9d, 0xaf);
    OV5642YUV_write_cmos_sensor(0x8b9e, 0x4b);
    OV5642YUV_write_cmos_sensor(0x8b9f, 0x78);
    OV5642YUV_write_cmos_sensor(0x8ba0, 0x57);
    OV5642YUV_write_cmos_sensor(0x8ba1, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ba2, 0x10);
    OV5642YUV_write_cmos_sensor(0x8ba3, 0xc5);
    OV5642YUV_write_cmos_sensor(0x8ba4, 0x7d);
    OV5642YUV_write_cmos_sensor(0x8ba5, 0x3c);
    OV5642YUV_write_cmos_sensor(0x8ba6, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ba7, 0x07);
    OV5642YUV_write_cmos_sensor(0x8ba8, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ba9, 0x78);
    OV5642YUV_write_cmos_sensor(0x8baa, 0x5f);
    OV5642YUV_write_cmos_sensor(0x8bab, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8bac, 0x06);
    OV5642YUV_write_cmos_sensor(0x8bad, 0x08);
    OV5642YUV_write_cmos_sensor(0x8bae, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8baf, 0x07);
    OV5642YUV_write_cmos_sensor(0x8bb0, 0xaf);
    OV5642YUV_write_cmos_sensor(0x8bb1, 0x49);
    OV5642YUV_write_cmos_sensor(0x8bb2, 0x7e);
    OV5642YUV_write_cmos_sensor(0x8bb3, 0x00);
    OV5642YUV_write_cmos_sensor(0x8bb4, 0x78);
    OV5642YUV_write_cmos_sensor(0x8bb5, 0x57);
    OV5642YUV_write_cmos_sensor(0x8bb6, 0x12);
    OV5642YUV_write_cmos_sensor(0x8bb7, 0x10);
    OV5642YUV_write_cmos_sensor(0x8bb8, 0xc7);
    OV5642YUV_write_cmos_sensor(0x8bb9, 0x7d);
    OV5642YUV_write_cmos_sensor(0x8bba, 0x3c);
    OV5642YUV_write_cmos_sensor(0x8bbb, 0x12);
    OV5642YUV_write_cmos_sensor(0x8bbc, 0x07);
    OV5642YUV_write_cmos_sensor(0x8bbd, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8bbe, 0x78);
    OV5642YUV_write_cmos_sensor(0x8bbf, 0x5b);
    OV5642YUV_write_cmos_sensor(0x8bc0, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8bc1, 0x06);
    OV5642YUV_write_cmos_sensor(0x8bc2, 0x08);
    OV5642YUV_write_cmos_sensor(0x8bc3, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8bc4, 0x07);
    OV5642YUV_write_cmos_sensor(0x8bc5, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8bc6, 0x78);
    OV5642YUV_write_cmos_sensor(0x8bc7, 0x5e);
    OV5642YUV_write_cmos_sensor(0x8bc8, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8bc9, 0x94);
    OV5642YUV_write_cmos_sensor(0x8bca, 0x08);
    OV5642YUV_write_cmos_sensor(0x8bcb, 0x18);
    OV5642YUV_write_cmos_sensor(0x8bcc, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8bcd, 0x94);
    OV5642YUV_write_cmos_sensor(0x8bce, 0x00);
    OV5642YUV_write_cmos_sensor(0x8bcf, 0x50);
    OV5642YUV_write_cmos_sensor(0x8bd0, 0x05);
    OV5642YUV_write_cmos_sensor(0x8bd1, 0x76);
    OV5642YUV_write_cmos_sensor(0x8bd2, 0x00);
    OV5642YUV_write_cmos_sensor(0x8bd3, 0x08);
    OV5642YUV_write_cmos_sensor(0x8bd4, 0x76);
    OV5642YUV_write_cmos_sensor(0x8bd5, 0x08);
    OV5642YUV_write_cmos_sensor(0x8bd6, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8bd7, 0x78);
    OV5642YUV_write_cmos_sensor(0x8bd8, 0x60);
    OV5642YUV_write_cmos_sensor(0x8bd9, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8bda, 0x94);
    OV5642YUV_write_cmos_sensor(0x8bdb, 0x08);
    OV5642YUV_write_cmos_sensor(0x8bdc, 0x18);
    OV5642YUV_write_cmos_sensor(0x8bdd, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8bde, 0x94);
    OV5642YUV_write_cmos_sensor(0x8bdf, 0x00);
    OV5642YUV_write_cmos_sensor(0x8be0, 0x50);
    OV5642YUV_write_cmos_sensor(0x8be1, 0x05);
    OV5642YUV_write_cmos_sensor(0x8be2, 0x76);
    OV5642YUV_write_cmos_sensor(0x8be3, 0x00);
    OV5642YUV_write_cmos_sensor(0x8be4, 0x08);
    OV5642YUV_write_cmos_sensor(0x8be5, 0x76);
    OV5642YUV_write_cmos_sensor(0x8be6, 0x08);
    OV5642YUV_write_cmos_sensor(0x8be7, 0x78);
    OV5642YUV_write_cmos_sensor(0x8be8, 0x5d);
    OV5642YUV_write_cmos_sensor(0x8be9, 0x12);
    OV5642YUV_write_cmos_sensor(0x8bea, 0x10);
    OV5642YUV_write_cmos_sensor(0x8beb, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8bec, 0xff);
    OV5642YUV_write_cmos_sensor(0x8bed, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8bee, 0x78);
    OV5642YUV_write_cmos_sensor(0x8bef, 0x5a);
    OV5642YUV_write_cmos_sensor(0x8bf0, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8bf1, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8bf2, 0x18);
    OV5642YUV_write_cmos_sensor(0x8bf3, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8bf4, 0x9e);
    OV5642YUV_write_cmos_sensor(0x8bf5, 0x40);
    OV5642YUV_write_cmos_sensor(0x8bf6, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8bf7, 0x78);
    OV5642YUV_write_cmos_sensor(0x8bf8, 0x5d);
    OV5642YUV_write_cmos_sensor(0x8bf9, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8bfa, 0x13);
    OV5642YUV_write_cmos_sensor(0x8bfb, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8bfc, 0x08);
    OV5642YUV_write_cmos_sensor(0x8bfd, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8bfe, 0x78);
    OV5642YUV_write_cmos_sensor(0x8bff, 0x5a);
    OV5642YUV_write_cmos_sensor(0x8c00, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c01, 0x11);
    OV5642YUV_write_cmos_sensor(0x8c02, 0x11);
    OV5642YUV_write_cmos_sensor(0x8c03, 0x80);
    OV5642YUV_write_cmos_sensor(0x8c04, 0x04);
    OV5642YUV_write_cmos_sensor(0x8c05, 0x7e);
    OV5642YUV_write_cmos_sensor(0x8c06, 0x00);
    OV5642YUV_write_cmos_sensor(0x8c07, 0x7f);
    OV5642YUV_write_cmos_sensor(0x8c08, 0x00);
    OV5642YUV_write_cmos_sensor(0x8c09, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c0a, 0x61);
    OV5642YUV_write_cmos_sensor(0x8c0b, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c0c, 0x10);
    OV5642YUV_write_cmos_sensor(0x8c0d, 0xde);
    OV5642YUV_write_cmos_sensor(0x8c0e, 0xff);
    OV5642YUV_write_cmos_sensor(0x8c0f, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8c10, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c11, 0x5c);
    OV5642YUV_write_cmos_sensor(0x8c12, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8c13, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8c14, 0x18);
    OV5642YUV_write_cmos_sensor(0x8c15, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8c16, 0x9e);
    OV5642YUV_write_cmos_sensor(0x8c17, 0x40);
    OV5642YUV_write_cmos_sensor(0x8c18, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8c19, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c1a, 0x5f);
    OV5642YUV_write_cmos_sensor(0x8c1b, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8c1c, 0x13);
    OV5642YUV_write_cmos_sensor(0x8c1d, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8c1e, 0x08);
    OV5642YUV_write_cmos_sensor(0x8c1f, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8c20, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c21, 0x5c);
    OV5642YUV_write_cmos_sensor(0x8c22, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c23, 0x11);
    OV5642YUV_write_cmos_sensor(0x8c24, 0x11);
    OV5642YUV_write_cmos_sensor(0x8c25, 0x80);
    OV5642YUV_write_cmos_sensor(0x8c26, 0x04);
    OV5642YUV_write_cmos_sensor(0x8c27, 0x7e);
    OV5642YUV_write_cmos_sensor(0x8c28, 0x00);
    OV5642YUV_write_cmos_sensor(0x8c29, 0x7f);
    OV5642YUV_write_cmos_sensor(0x8c2a, 0x00);
    OV5642YUV_write_cmos_sensor(0x8c2b, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8c2c, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8c2d, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8c2e, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c2f, 0x65);
    OV5642YUV_write_cmos_sensor(0x8c30, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c31, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c32, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8c33, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c34, 0x5d);
    OV5642YUV_write_cmos_sensor(0x8c35, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c36, 0x10);
    OV5642YUV_write_cmos_sensor(0x8c37, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8c38, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c39, 0x5a);
    OV5642YUV_write_cmos_sensor(0x8c3a, 0x26);
    OV5642YUV_write_cmos_sensor(0x8c3b, 0xff);
    OV5642YUV_write_cmos_sensor(0x8c3c, 0xee);
    OV5642YUV_write_cmos_sensor(0x8c3d, 0x18);
    OV5642YUV_write_cmos_sensor(0x8c3e, 0x36);
    OV5642YUV_write_cmos_sensor(0x8c3f, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8c40, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c41, 0x69);
    OV5642YUV_write_cmos_sensor(0x8c42, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c43, 0x10);
    OV5642YUV_write_cmos_sensor(0x8c44, 0xde);
    OV5642YUV_write_cmos_sensor(0x8c45, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c46, 0x5c);
    OV5642YUV_write_cmos_sensor(0x8c47, 0x26);
    OV5642YUV_write_cmos_sensor(0x8c48, 0xff);
    OV5642YUV_write_cmos_sensor(0x8c49, 0xee);
    OV5642YUV_write_cmos_sensor(0x8c4a, 0x18);
    OV5642YUV_write_cmos_sensor(0x8c4b, 0x36);
    OV5642YUV_write_cmos_sensor(0x8c4c, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8c4d, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8c4e, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8c4f, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8c50, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c51, 0x6d);
    OV5642YUV_write_cmos_sensor(0x8c52, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c53, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c54, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8c55, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c56, 0x10);
    OV5642YUV_write_cmos_sensor(0x8c57, 0xee);
    OV5642YUV_write_cmos_sensor(0x8c58, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c59, 0x69);
    OV5642YUV_write_cmos_sensor(0x8c5a, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c5b, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c5c, 0x9b);
    OV5642YUV_write_cmos_sensor(0x8c5d, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8c5e, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c5f, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c60, 0x67);
    OV5642YUV_write_cmos_sensor(0x8c61, 0x40);
    OV5642YUV_write_cmos_sensor(0x8c62, 0x08);
    OV5642YUV_write_cmos_sensor(0x8c63, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c64, 0x10);
    OV5642YUV_write_cmos_sensor(0x8c65, 0xee);
    OV5642YUV_write_cmos_sensor(0x8c66, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c67, 0x69);
    OV5642YUV_write_cmos_sensor(0x8c68, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c69, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c6a, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8c6b, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c6c, 0x57);
    OV5642YUV_write_cmos_sensor(0x8c6d, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c6e, 0x10);
    OV5642YUV_write_cmos_sensor(0x8c6f, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8c70, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c71, 0x6d);
    OV5642YUV_write_cmos_sensor(0x8c72, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c73, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c74, 0x9b);
    OV5642YUV_write_cmos_sensor(0x8c75, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8c76, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c77, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c78, 0x67);
    OV5642YUV_write_cmos_sensor(0x8c79, 0x40);
    OV5642YUV_write_cmos_sensor(0x8c7a, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8c7b, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c7c, 0x57);
    OV5642YUV_write_cmos_sensor(0x8c7d, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c7e, 0x10);
    OV5642YUV_write_cmos_sensor(0x8c7f, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8c80, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c81, 0x6d);
    OV5642YUV_write_cmos_sensor(0x8c82, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c83, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c84, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8c85, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8c86, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8c87, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c88, 0x64);
    OV5642YUV_write_cmos_sensor(0x8c89, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c8a, 0x11);
    OV5642YUV_write_cmos_sensor(0x8c8b, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c8c, 0x24);
    OV5642YUV_write_cmos_sensor(0x8c8d, 0x01);
    OV5642YUV_write_cmos_sensor(0x8c8e, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c8f, 0x10);
    OV5642YUV_write_cmos_sensor(0x8c90, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8c91, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c92, 0x68);
    OV5642YUV_write_cmos_sensor(0x8c93, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c94, 0x11);
    OV5642YUV_write_cmos_sensor(0x8c95, 0x09);
    OV5642YUV_write_cmos_sensor(0x8c96, 0x24);
    OV5642YUV_write_cmos_sensor(0x8c97, 0x02);
    OV5642YUV_write_cmos_sensor(0x8c98, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c99, 0x10);
    OV5642YUV_write_cmos_sensor(0x8c9a, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8c9b, 0x78);
    OV5642YUV_write_cmos_sensor(0x8c9c, 0x6c);
    OV5642YUV_write_cmos_sensor(0x8c9d, 0x12);
    OV5642YUV_write_cmos_sensor(0x8c9e, 0x11);
    OV5642YUV_write_cmos_sensor(0x8c9f, 0x09);
    OV5642YUV_write_cmos_sensor(0x8ca0, 0x24);
    OV5642YUV_write_cmos_sensor(0x8ca1, 0x03);
    OV5642YUV_write_cmos_sensor(0x8ca2, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ca3, 0x10);
    OV5642YUV_write_cmos_sensor(0x8ca4, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8ca5, 0x78);
    OV5642YUV_write_cmos_sensor(0x8ca6, 0x70);
    OV5642YUV_write_cmos_sensor(0x8ca7, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ca8, 0x11);
    OV5642YUV_write_cmos_sensor(0x8ca9, 0x09);
    OV5642YUV_write_cmos_sensor(0x8caa, 0x24);
    OV5642YUV_write_cmos_sensor(0x8cab, 0x04);
    OV5642YUV_write_cmos_sensor(0x8cac, 0x12);
    OV5642YUV_write_cmos_sensor(0x8cad, 0x10);
    OV5642YUV_write_cmos_sensor(0x8cae, 0xd2);
    OV5642YUV_write_cmos_sensor(0x8caf, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8cb0, 0xbd);
    OV5642YUV_write_cmos_sensor(0x8cb1, 0x05);
    OV5642YUV_write_cmos_sensor(0x8cb2, 0xd4);
    OV5642YUV_write_cmos_sensor(0x8cb3, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8cb4, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8cb5, 0xc2);
    OV5642YUV_write_cmos_sensor(0x8cb6, 0x06);
    OV5642YUV_write_cmos_sensor(0x8cb7, 0x22);
    OV5642YUV_write_cmos_sensor(0x8cb8, 0x90);
    OV5642YUV_write_cmos_sensor(0x8cb9, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8cba, 0x69);
    OV5642YUV_write_cmos_sensor(0x8cbb, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8cbc, 0x93);
    OV5642YUV_write_cmos_sensor(0x8cbd, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8cbe, 0x74);
    OV5642YUV_write_cmos_sensor(0x8cbf, 0x01);
    OV5642YUV_write_cmos_sensor(0x8cc0, 0x93);
    OV5642YUV_write_cmos_sensor(0x8cc1, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8cc2, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8cc3, 0x82);
    OV5642YUV_write_cmos_sensor(0x8cc4, 0x8c);
    OV5642YUV_write_cmos_sensor(0x8cc5, 0x83);
    OV5642YUV_write_cmos_sensor(0x8cc6, 0xee);
    OV5642YUV_write_cmos_sensor(0x8cc7, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8cc8, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8cc9, 0x35);
    OV5642YUV_write_cmos_sensor(0x8cca, 0x25);
    OV5642YUV_write_cmos_sensor(0x8ccb, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8ccc, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ccd, 0x35);
    OV5642YUV_write_cmos_sensor(0x8cce, 0x90);
    OV5642YUV_write_cmos_sensor(0x8ccf, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8cd0, 0x72);
    OV5642YUV_write_cmos_sensor(0x8cd1, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8cd2, 0x93);
    OV5642YUV_write_cmos_sensor(0x8cd3, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8cd4, 0x52);
    OV5642YUV_write_cmos_sensor(0x8cd5, 0x06);
    OV5642YUV_write_cmos_sensor(0x8cd6, 0x90);
    OV5642YUV_write_cmos_sensor(0x8cd7, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8cd8, 0x74);
    OV5642YUV_write_cmos_sensor(0x8cd9, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8cda, 0x93);
    OV5642YUV_write_cmos_sensor(0x8cdb, 0x42);
    OV5642YUV_write_cmos_sensor(0x8cdc, 0x06);
    OV5642YUV_write_cmos_sensor(0x8cdd, 0x8d);
    OV5642YUV_write_cmos_sensor(0x8cde, 0x82);
    OV5642YUV_write_cmos_sensor(0x8cdf, 0x8c);
    OV5642YUV_write_cmos_sensor(0x8ce0, 0x83);
    OV5642YUV_write_cmos_sensor(0x8ce1, 0xee);
    OV5642YUV_write_cmos_sensor(0x8ce2, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8ce3, 0xe9);
    OV5642YUV_write_cmos_sensor(0x8ce4, 0x90);
    OV5642YUV_write_cmos_sensor(0x8ce5, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8ce6, 0x73);
    OV5642YUV_write_cmos_sensor(0x8ce7, 0x52);
    OV5642YUV_write_cmos_sensor(0x8ce8, 0x06);
    OV5642YUV_write_cmos_sensor(0x8ce9, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8cea, 0x93);
    OV5642YUV_write_cmos_sensor(0x8ceb, 0x42);
    OV5642YUV_write_cmos_sensor(0x8cec, 0x06);
    OV5642YUV_write_cmos_sensor(0x8ced, 0x90);
    OV5642YUV_write_cmos_sensor(0x8cee, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8cef, 0x69);
    OV5642YUV_write_cmos_sensor(0x8cf0, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8cf1, 0x93);
    OV5642YUV_write_cmos_sensor(0x8cf2, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8cf3, 0x74);
    OV5642YUV_write_cmos_sensor(0x8cf4, 0x01);
    OV5642YUV_write_cmos_sensor(0x8cf5, 0x93);
    OV5642YUV_write_cmos_sensor(0x8cf6, 0x22);
    OV5642YUV_write_cmos_sensor(0x8cf7, 0xee);
    OV5642YUV_write_cmos_sensor(0x8cf8, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8cf9, 0x90);
    OV5642YUV_write_cmos_sensor(0x8cfa, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8cfb, 0x75);
    OV5642YUV_write_cmos_sensor(0x8cfc, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8cfd, 0x93);
    OV5642YUV_write_cmos_sensor(0x8cfe, 0x90);
    OV5642YUV_write_cmos_sensor(0x8cff, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d00, 0x77);
    OV5642YUV_write_cmos_sensor(0x8d01, 0x52);
    OV5642YUV_write_cmos_sensor(0x8d02, 0x07);
    OV5642YUV_write_cmos_sensor(0x8d03, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d04, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d05, 0x42);
    OV5642YUV_write_cmos_sensor(0x8d06, 0x07);
    OV5642YUV_write_cmos_sensor(0x8d07, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d08, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d09, 0x6d);
    OV5642YUV_write_cmos_sensor(0x8d0a, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d0b, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d0c, 0xfa);
    OV5642YUV_write_cmos_sensor(0x8d0d, 0x74);
    OV5642YUV_write_cmos_sensor(0x8d0e, 0x01);
    OV5642YUV_write_cmos_sensor(0x8d0f, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d10, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8d11, 0x82);
    OV5642YUV_write_cmos_sensor(0x8d12, 0x8a);
    OV5642YUV_write_cmos_sensor(0x8d13, 0x83);
    OV5642YUV_write_cmos_sensor(0x8d14, 0x22);
    OV5642YUV_write_cmos_sensor(0x8d15, 0xef);
    OV5642YUV_write_cmos_sensor(0x8d16, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8d17, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d18, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d19, 0x72);
    OV5642YUV_write_cmos_sensor(0x8d1a, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d1b, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d1c, 0x52);
    OV5642YUV_write_cmos_sensor(0x8d1d, 0x06);
    OV5642YUV_write_cmos_sensor(0x8d1e, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d1f, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d20, 0x74);
    OV5642YUV_write_cmos_sensor(0x8d21, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d22, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d23, 0x42);
    OV5642YUV_write_cmos_sensor(0x8d24, 0x06);
    OV5642YUV_write_cmos_sensor(0x8d25, 0x22);
    OV5642YUV_write_cmos_sensor(0x8d26, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8d27, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8d28, 0x82);
    OV5642YUV_write_cmos_sensor(0x8d29, 0x8c);
    OV5642YUV_write_cmos_sensor(0x8d2a, 0x83);
    OV5642YUV_write_cmos_sensor(0x8d2b, 0xee);
    OV5642YUV_write_cmos_sensor(0x8d2c, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8d2d, 0x22);
    OV5642YUV_write_cmos_sensor(0x8d2e, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8d2f, 0x82);
    OV5642YUV_write_cmos_sensor(0x8d30, 0x8c);
    OV5642YUV_write_cmos_sensor(0x8d31, 0x83);
    OV5642YUV_write_cmos_sensor(0x8d32, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8d33, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8d34, 0x31);
    OV5642YUV_write_cmos_sensor(0x8d35, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d36, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d37, 0x7b);
    OV5642YUV_write_cmos_sensor(0x8d38, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d39, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d3a, 0x55);
    OV5642YUV_write_cmos_sensor(0x8d3b, 0x31);
    OV5642YUV_write_cmos_sensor(0x8d3c, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8d3d, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d3e, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d3f, 0x7d);
    OV5642YUV_write_cmos_sensor(0x8d40, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d41, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d42, 0x22);
    OV5642YUV_write_cmos_sensor(0x8d43, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d44, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d45, 0x78);
    OV5642YUV_write_cmos_sensor(0x8d46, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d47, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d48, 0x52);
    OV5642YUV_write_cmos_sensor(0x8d49, 0x06);
    OV5642YUV_write_cmos_sensor(0x8d4a, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d4b, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d4c, 0x7a);
    OV5642YUV_write_cmos_sensor(0x8d4d, 0x22);
    OV5642YUV_write_cmos_sensor(0x8d4e, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d4f, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d50, 0x72);
    OV5642YUV_write_cmos_sensor(0x8d51, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d52, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d53, 0x52);
    OV5642YUV_write_cmos_sensor(0x8d54, 0x06);
    OV5642YUV_write_cmos_sensor(0x8d55, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8d56, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d57, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d58, 0x42);
    OV5642YUV_write_cmos_sensor(0x8d59, 0x06);
    OV5642YUV_write_cmos_sensor(0x8d5a, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d5b, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d5c, 0x69);
    OV5642YUV_write_cmos_sensor(0x8d5d, 0x22);
    OV5642YUV_write_cmos_sensor(0x8d5e, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d5f, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d60, 0x75);
    OV5642YUV_write_cmos_sensor(0x8d61, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d62, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d63, 0x52);
    OV5642YUV_write_cmos_sensor(0x8d64, 0x07);
    OV5642YUV_write_cmos_sensor(0x8d65, 0x22);
    OV5642YUV_write_cmos_sensor(0x8d66, 0x8d);
    OV5642YUV_write_cmos_sensor(0x8d67, 0x82);
    OV5642YUV_write_cmos_sensor(0x8d68, 0x8c);
    OV5642YUV_write_cmos_sensor(0x8d69, 0x83);
    OV5642YUV_write_cmos_sensor(0x8d6a, 0xee);
    OV5642YUV_write_cmos_sensor(0x8d6b, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8d6c, 0x22);
    OV5642YUV_write_cmos_sensor(0x8d6d, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d6e, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d6f, 0x9b);
    OV5642YUV_write_cmos_sensor(0x8d70, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d71, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d72, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8d73, 0x74);
    OV5642YUV_write_cmos_sensor(0x8d74, 0x01);
    OV5642YUV_write_cmos_sensor(0x8d75, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d76, 0xff);
    OV5642YUV_write_cmos_sensor(0x8d77, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8d78, 0x90);
    OV5642YUV_write_cmos_sensor(0x8d79, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8d7a, 0x99);
    OV5642YUV_write_cmos_sensor(0x8d7b, 0x74);
    OV5642YUV_write_cmos_sensor(0x8d7c, 0x01);
    OV5642YUV_write_cmos_sensor(0x8d7d, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d7e, 0x9f);
    OV5642YUV_write_cmos_sensor(0x8d7f, 0xff);
    OV5642YUV_write_cmos_sensor(0x8d80, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d81, 0x93);
    OV5642YUV_write_cmos_sensor(0x8d82, 0x9e);
    OV5642YUV_write_cmos_sensor(0x8d83, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8d84, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8d85, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8d86, 0x30);
    OV5642YUV_write_cmos_sensor(0x8d87, 0x8e);
    OV5642YUV_write_cmos_sensor(0x8d88, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8d89, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8d8a, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8d8b, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8d8c, 0x2d);
    OV5642YUV_write_cmos_sensor(0x8d8d, 0xab);
    OV5642YUV_write_cmos_sensor(0x8d8e, 0x30);
    OV5642YUV_write_cmos_sensor(0x8d8f, 0xaa);
    OV5642YUV_write_cmos_sensor(0x8d90, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8d91, 0xa9);
    OV5642YUV_write_cmos_sensor(0x8d92, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8d93, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8d94, 0x2d);
    OV5642YUV_write_cmos_sensor(0x8d95, 0xaf);
    OV5642YUV_write_cmos_sensor(0x8d96, 0x4e);
    OV5642YUV_write_cmos_sensor(0x8d97, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8d98, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8d99, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8d9a, 0x12);
    OV5642YUV_write_cmos_sensor(0x8d9b, 0x08);
    OV5642YUV_write_cmos_sensor(0x8d9c, 0x4a);
    OV5642YUV_write_cmos_sensor(0x8d9d, 0x12);
    OV5642YUV_write_cmos_sensor(0x8d9e, 0x11);
    OV5642YUV_write_cmos_sensor(0x8d9f, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8da0, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8da1, 0x7b);
    OV5642YUV_write_cmos_sensor(0x8da2, 0xff);
    OV5642YUV_write_cmos_sensor(0x8da3, 0xfa);
    OV5642YUV_write_cmos_sensor(0x8da4, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8da5, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8da6, 0x12);
    OV5642YUV_write_cmos_sensor(0x8da7, 0x08);
    OV5642YUV_write_cmos_sensor(0x8da8, 0xd5);
    OV5642YUV_write_cmos_sensor(0x8da9, 0x12);
    OV5642YUV_write_cmos_sensor(0x8daa, 0x11);
    OV5642YUV_write_cmos_sensor(0x8dab, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8dac, 0x90);
    OV5642YUV_write_cmos_sensor(0x8dad, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8dae, 0x87);
    OV5642YUV_write_cmos_sensor(0x8daf, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8db0, 0x12);
    OV5642YUV_write_cmos_sensor(0x8db1, 0x12);
    OV5642YUV_write_cmos_sensor(0x8db2, 0x12);
    OV5642YUV_write_cmos_sensor(0x8db3, 0x12);
    OV5642YUV_write_cmos_sensor(0x8db4, 0x11);
    OV5642YUV_write_cmos_sensor(0x8db5, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8db6, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8db7, 0x85);
    OV5642YUV_write_cmos_sensor(0x8db8, 0x4d);
    OV5642YUV_write_cmos_sensor(0x8db9, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8dba, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8dbb, 0x2b);
    OV5642YUV_write_cmos_sensor(0x8dbc, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8dbd, 0x2a);
    OV5642YUV_write_cmos_sensor(0x8dbe, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8dbf, 0x29);
    OV5642YUV_write_cmos_sensor(0x8dc0, 0xaf);
    OV5642YUV_write_cmos_sensor(0x8dc1, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8dc2, 0xae);
    OV5642YUV_write_cmos_sensor(0x8dc3, 0x2b);
    OV5642YUV_write_cmos_sensor(0x8dc4, 0xad);
    OV5642YUV_write_cmos_sensor(0x8dc5, 0x2a);
    OV5642YUV_write_cmos_sensor(0x8dc6, 0xac);
    OV5642YUV_write_cmos_sensor(0x8dc7, 0x29);
    OV5642YUV_write_cmos_sensor(0x8dc8, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8dc9, 0x12);
    OV5642YUV_write_cmos_sensor(0x8dca, 0x12);
    OV5642YUV_write_cmos_sensor(0x8dcb, 0x12);
    OV5642YUV_write_cmos_sensor(0x8dcc, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8dcd, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8dce, 0x8e);
    OV5642YUV_write_cmos_sensor(0x8dcf, 0x2b);
    OV5642YUV_write_cmos_sensor(0x8dd0, 0x8d);
    OV5642YUV_write_cmos_sensor(0x8dd1, 0x2a);
    OV5642YUV_write_cmos_sensor(0x8dd2, 0x8c);
    OV5642YUV_write_cmos_sensor(0x8dd3, 0x29);
    OV5642YUV_write_cmos_sensor(0x8dd4, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8dd5, 0x30);
    OV5642YUV_write_cmos_sensor(0x8dd6, 0x45);
    OV5642YUV_write_cmos_sensor(0x8dd7, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8dd8, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8dd9, 0x30);
    OV5642YUV_write_cmos_sensor(0x8dda, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8ddb, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8ddc, 0x45);
    OV5642YUV_write_cmos_sensor(0x8ddd, 0x2b);
    OV5642YUV_write_cmos_sensor(0x8dde, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ddf, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8de0, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8de1, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8de2, 0x45);
    OV5642YUV_write_cmos_sensor(0x8de3, 0x2a);
    OV5642YUV_write_cmos_sensor(0x8de4, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8de5, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8de6, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8de7, 0x2d);
    OV5642YUV_write_cmos_sensor(0x8de8, 0x45);
    OV5642YUV_write_cmos_sensor(0x8de9, 0x29);
    OV5642YUV_write_cmos_sensor(0x8dea, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8deb, 0x2d);
    OV5642YUV_write_cmos_sensor(0x8dec, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ded, 0x11);
    OV5642YUV_write_cmos_sensor(0x8dee, 0xdf);
    OV5642YUV_write_cmos_sensor(0x8def, 0x85);
    OV5642YUV_write_cmos_sensor(0x8df0, 0x30);
    OV5642YUV_write_cmos_sensor(0x8df1, 0x40);
    OV5642YUV_write_cmos_sensor(0x8df2, 0x85);
    OV5642YUV_write_cmos_sensor(0x8df3, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8df4, 0x3f);
    OV5642YUV_write_cmos_sensor(0x8df5, 0x85);
    OV5642YUV_write_cmos_sensor(0x8df6, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8df7, 0x3e);
    OV5642YUV_write_cmos_sensor(0x8df8, 0x85);
    OV5642YUV_write_cmos_sensor(0x8df9, 0x2d);
    OV5642YUV_write_cmos_sensor(0x8dfa, 0x3d);
    OV5642YUV_write_cmos_sensor(0x8dfb, 0x02);
    OV5642YUV_write_cmos_sensor(0x8dfc, 0x02);
    OV5642YUV_write_cmos_sensor(0x8dfd, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8dfe, 0x00);
    OV5642YUV_write_cmos_sensor(0x8dff, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e00, 0x11);
    OV5642YUV_write_cmos_sensor(0x8e01, 0x08);
    OV5642YUV_write_cmos_sensor(0x8e02, 0x17);
    OV5642YUV_write_cmos_sensor(0x8e03, 0x12);
    OV5642YUV_write_cmos_sensor(0x8e04, 0x06);
    OV5642YUV_write_cmos_sensor(0x8e05, 0x04);
    OV5642YUV_write_cmos_sensor(0x8e06, 0x4f);
    OV5642YUV_write_cmos_sensor(0x8e07, 0x56);
    OV5642YUV_write_cmos_sensor(0x8e08, 0x54);
    OV5642YUV_write_cmos_sensor(0x8e09, 0x20);
    OV5642YUV_write_cmos_sensor(0x8e0a, 0x20);
    OV5642YUV_write_cmos_sensor(0x8e0b, 0x20);
    OV5642YUV_write_cmos_sensor(0x8e0c, 0x20);
    OV5642YUV_write_cmos_sensor(0x8e0d, 0x20);
    OV5642YUV_write_cmos_sensor(0x8e0e, 0x14);
    OV5642YUV_write_cmos_sensor(0x8e0f, 0x01);
    OV5642YUV_write_cmos_sensor(0x8e10, 0x10);
    OV5642YUV_write_cmos_sensor(0x8e11, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e12, 0x56);
    OV5642YUV_write_cmos_sensor(0x8e13, 0x42);
    OV5642YUV_write_cmos_sensor(0x8e14, 0x1a);
    OV5642YUV_write_cmos_sensor(0x8e15, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e16, 0x27);
    OV5642YUV_write_cmos_sensor(0x8e17, 0x7e);
    OV5642YUV_write_cmos_sensor(0x8e18, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e19, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e1a, 0x04);
    OV5642YUV_write_cmos_sensor(0x8e1b, 0x20);
    OV5642YUV_write_cmos_sensor(0x8e1c, 0xdf);
    OV5642YUV_write_cmos_sensor(0x8e1d, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e1e, 0x05);
    OV5642YUV_write_cmos_sensor(0x8e1f, 0x40);
    OV5642YUV_write_cmos_sensor(0x8e20, 0xbf);
    OV5642YUV_write_cmos_sensor(0x8e21, 0x50);
    OV5642YUV_write_cmos_sensor(0x8e22, 0x25);
    OV5642YUV_write_cmos_sensor(0x8e23, 0x04);
    OV5642YUV_write_cmos_sensor(0x8e24, 0xfb);
    OV5642YUV_write_cmos_sensor(0x8e25, 0x50);
    OV5642YUV_write_cmos_sensor(0x8e26, 0x03);
    OV5642YUV_write_cmos_sensor(0x8e27, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e28, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8e29, 0x50);
    OV5642YUV_write_cmos_sensor(0x8e2a, 0x27);
    OV5642YUV_write_cmos_sensor(0x8e2b, 0x01);
    OV5642YUV_write_cmos_sensor(0x8e2c, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8e2d, 0x60);
    OV5642YUV_write_cmos_sensor(0x8e2e, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e2f, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8e30, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e31, 0x3f);
    OV5642YUV_write_cmos_sensor(0x8e32, 0x05);
    OV5642YUV_write_cmos_sensor(0x8e33, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e34, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e35, 0x3f);
    OV5642YUV_write_cmos_sensor(0x8e36, 0x06);
    OV5642YUV_write_cmos_sensor(0x8e37, 0x24);
    OV5642YUV_write_cmos_sensor(0x8e38, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e39, 0x3f);
    OV5642YUV_write_cmos_sensor(0x8e3a, 0x01);
    OV5642YUV_write_cmos_sensor(0x8e3b, 0x2a);
    OV5642YUV_write_cmos_sensor(0x8e3c, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e3d, 0x3f);
    OV5642YUV_write_cmos_sensor(0x8e3e, 0x02);
    OV5642YUV_write_cmos_sensor(0x8e3f, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e40, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e41, 0x3f);
    OV5642YUV_write_cmos_sensor(0x8e42, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8e43, 0x0f);
    OV5642YUV_write_cmos_sensor(0x8e44, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8e45, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e46, 0x24);
    OV5642YUV_write_cmos_sensor(0x8e47, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e48, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e49, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e4a, 0x1b);
    OV5642YUV_write_cmos_sensor(0x8e4b, 0x07);
    OV5642YUV_write_cmos_sensor(0x8e4c, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8e4d, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e4e, 0x1e);
    OV5642YUV_write_cmos_sensor(0x8e4f, 0x07);
    OV5642YUV_write_cmos_sensor(0x8e50, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8e51, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e52, 0x18);
    OV5642YUV_write_cmos_sensor(0x8e53, 0x07);
    OV5642YUV_write_cmos_sensor(0x8e54, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8e55, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e56, 0x01);
    OV5642YUV_write_cmos_sensor(0x8e57, 0x40);
    OV5642YUV_write_cmos_sensor(0x8e58, 0xbf);
    OV5642YUV_write_cmos_sensor(0x8e59, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e5a, 0x01);
    OV5642YUV_write_cmos_sensor(0x8e5b, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e5c, 0xbf);
    OV5642YUV_write_cmos_sensor(0x8e5d, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e5e, 0x27);
    OV5642YUV_write_cmos_sensor(0x8e5f, 0x70);
    OV5642YUV_write_cmos_sensor(0x8e60, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e61, 0x3a);
    OV5642YUV_write_cmos_sensor(0x8e62, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e63, 0x01);
    OV5642YUV_write_cmos_sensor(0x8e64, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8e65, 0x3a);
    OV5642YUV_write_cmos_sensor(0x8e66, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e67, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e68, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8e69, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e6a, 0x1b);
    OV5642YUV_write_cmos_sensor(0x8e6b, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e6c, 0x42);
    OV5642YUV_write_cmos_sensor(0x8e6d, 0x30);
    OV5642YUV_write_cmos_sensor(0x8e6e, 0x18);
    OV5642YUV_write_cmos_sensor(0x8e6f, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8e70, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e71, 0x01);
    OV5642YUV_write_cmos_sensor(0x8e72, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8e73, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e74, 0x01);
    OV5642YUV_write_cmos_sensor(0x8e75, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8e76, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e77, 0x02);
    OV5642YUV_write_cmos_sensor(0x8e78, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8e79, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e7a, 0x02);
    OV5642YUV_write_cmos_sensor(0x8e7b, 0x02);
    OV5642YUV_write_cmos_sensor(0x8e7c, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e7d, 0x02);
    OV5642YUV_write_cmos_sensor(0x8e7e, 0x01);
    OV5642YUV_write_cmos_sensor(0x8e7f, 0x41);
    OV5642YUV_write_cmos_sensor(0x8e80, 0x44);
    OV5642YUV_write_cmos_sensor(0x8e81, 0x58);
    OV5642YUV_write_cmos_sensor(0x8e82, 0x20);
    OV5642YUV_write_cmos_sensor(0x8e83, 0x18);
    OV5642YUV_write_cmos_sensor(0x8e84, 0x10);
    OV5642YUV_write_cmos_sensor(0x8e85, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8e86, 0x04);
    OV5642YUV_write_cmos_sensor(0x8e87, 0x04);
    OV5642YUV_write_cmos_sensor(0x8e88, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e89, 0x03);
    OV5642YUV_write_cmos_sensor(0x8e8a, 0xff);
    OV5642YUV_write_cmos_sensor(0x8e8b, 0x64);
    OV5642YUV_write_cmos_sensor(0x8e8c, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e8d, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e8e, 0x80);
    OV5642YUV_write_cmos_sensor(0x8e8f, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e90, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e91, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e92, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e93, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e94, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e95, 0x02);
    OV5642YUV_write_cmos_sensor(0x8e96, 0x04);
    OV5642YUV_write_cmos_sensor(0x8e97, 0x06);
    OV5642YUV_write_cmos_sensor(0x8e98, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e99, 0x03);
    OV5642YUV_write_cmos_sensor(0x8e9a, 0x65);
    OV5642YUV_write_cmos_sensor(0x8e9b, 0x00);
    OV5642YUV_write_cmos_sensor(0x8e9c, 0x99);
    OV5642YUV_write_cmos_sensor(0x8e9d, 0x50);
    OV5642YUV_write_cmos_sensor(0x8e9e, 0x3c);
    OV5642YUV_write_cmos_sensor(0x8e9f, 0x28);
    OV5642YUV_write_cmos_sensor(0x8ea0, 0x1e);
    OV5642YUV_write_cmos_sensor(0x8ea1, 0x10);
    OV5642YUV_write_cmos_sensor(0x8ea2, 0x10);
    OV5642YUV_write_cmos_sensor(0x8ea3, 0x00);
    OV5642YUV_write_cmos_sensor(0x8ea4, 0x00);
    OV5642YUV_write_cmos_sensor(0x8ea5, 0x10);
    OV5642YUV_write_cmos_sensor(0x8ea6, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8ea7, 0x10);
    OV5642YUV_write_cmos_sensor(0x8ea8, 0x04);
    OV5642YUV_write_cmos_sensor(0x8ea9, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8eaa, 0x6e);
    OV5642YUV_write_cmos_sensor(0x8eab, 0x06);
    OV5642YUV_write_cmos_sensor(0x8eac, 0x05);
    OV5642YUV_write_cmos_sensor(0x8ead, 0x00);
    OV5642YUV_write_cmos_sensor(0x8eae, 0xa5);
    OV5642YUV_write_cmos_sensor(0x8eaf, 0x5a);
    OV5642YUV_write_cmos_sensor(0x8eb0, 0x00);
    OV5642YUV_write_cmos_sensor(0x8eb1, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8eb2, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8eb3, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8eb4, 0x83);
    OV5642YUV_write_cmos_sensor(0x8eb5, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8eb6, 0x82);
    OV5642YUV_write_cmos_sensor(0x8eb7, 0xc0);
    OV5642YUV_write_cmos_sensor(0x8eb8, 0xd0);
    OV5642YUV_write_cmos_sensor(0x8eb9, 0x90);
    OV5642YUV_write_cmos_sensor(0x8eba, 0x3f);
    OV5642YUV_write_cmos_sensor(0x8ebb, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8ebc, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8ebd, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ebe, 0x27);
    OV5642YUV_write_cmos_sensor(0x8ebf, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8ec0, 0x27);
    OV5642YUV_write_cmos_sensor(0x8ec1, 0x30);
    OV5642YUV_write_cmos_sensor(0x8ec2, 0xe3);
    OV5642YUV_write_cmos_sensor(0x8ec3, 0x4c);
    OV5642YUV_write_cmos_sensor(0x8ec4, 0x30);
    OV5642YUV_write_cmos_sensor(0x8ec5, 0x36);
    OV5642YUV_write_cmos_sensor(0x8ec6, 0x3e);
    OV5642YUV_write_cmos_sensor(0x8ec7, 0x90);
    OV5642YUV_write_cmos_sensor(0x8ec8, 0x60);
    OV5642YUV_write_cmos_sensor(0x8ec9, 0x16);
    OV5642YUV_write_cmos_sensor(0x8eca, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8ecb, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ecc, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8ecd, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8ece, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8ecf, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ed0, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8ed1, 0x90);
    OV5642YUV_write_cmos_sensor(0x8ed2, 0x60);
    OV5642YUV_write_cmos_sensor(0x8ed3, 0x1a);
    OV5642YUV_write_cmos_sensor(0x8ed4, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8ed5, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8ed6, 0x14);
    OV5642YUV_write_cmos_sensor(0x8ed7, 0xa3);
    OV5642YUV_write_cmos_sensor(0x8ed8, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8ed9, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8eda, 0x15);
    OV5642YUV_write_cmos_sensor(0x8edb, 0x30);
    OV5642YUV_write_cmos_sensor(0x8edc, 0x01);
    OV5642YUV_write_cmos_sensor(0x8edd, 0x06);
    OV5642YUV_write_cmos_sensor(0x8ede, 0x30);
    OV5642YUV_write_cmos_sensor(0x8edf, 0x33);
    OV5642YUV_write_cmos_sensor(0x8ee0, 0x03);
    OV5642YUV_write_cmos_sensor(0x8ee1, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8ee2, 0x80);
    OV5642YUV_write_cmos_sensor(0x8ee3, 0x01);
    OV5642YUV_write_cmos_sensor(0x8ee4, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8ee5, 0x92);
    OV5642YUV_write_cmos_sensor(0x8ee6, 0x09);
    OV5642YUV_write_cmos_sensor(0x8ee7, 0x30);
    OV5642YUV_write_cmos_sensor(0x8ee8, 0x02);
    OV5642YUV_write_cmos_sensor(0x8ee9, 0x06);
    OV5642YUV_write_cmos_sensor(0x8eea, 0x30);
    OV5642YUV_write_cmos_sensor(0x8eeb, 0x33);
    OV5642YUV_write_cmos_sensor(0x8eec, 0x03);
    OV5642YUV_write_cmos_sensor(0x8eed, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8eee, 0x80);
    OV5642YUV_write_cmos_sensor(0x8eef, 0x01);
    OV5642YUV_write_cmos_sensor(0x8ef0, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8ef1, 0x92);
    OV5642YUV_write_cmos_sensor(0x8ef2, 0x0a);
    OV5642YUV_write_cmos_sensor(0x8ef3, 0x30);
    OV5642YUV_write_cmos_sensor(0x8ef4, 0x33);
    OV5642YUV_write_cmos_sensor(0x8ef5, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8ef6, 0x30);
    OV5642YUV_write_cmos_sensor(0x8ef7, 0x03);
    OV5642YUV_write_cmos_sensor(0x8ef8, 0x09);
    OV5642YUV_write_cmos_sensor(0x8ef9, 0x20);
    OV5642YUV_write_cmos_sensor(0x8efa, 0x02);
    OV5642YUV_write_cmos_sensor(0x8efb, 0x06);
    OV5642YUV_write_cmos_sensor(0x8efc, 0x20);
    OV5642YUV_write_cmos_sensor(0x8efd, 0x01);
    OV5642YUV_write_cmos_sensor(0x8efe, 0x03);
    OV5642YUV_write_cmos_sensor(0x8eff, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8f00, 0x80);
    OV5642YUV_write_cmos_sensor(0x8f01, 0x01);
    OV5642YUV_write_cmos_sensor(0x8f02, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8f03, 0x92);
    OV5642YUV_write_cmos_sensor(0x8f04, 0x0b);
    OV5642YUV_write_cmos_sensor(0x8f05, 0x90);
    OV5642YUV_write_cmos_sensor(0x8f06, 0x30);
    OV5642YUV_write_cmos_sensor(0x8f07, 0x01);
    OV5642YUV_write_cmos_sensor(0x8f08, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8f09, 0x44);
    OV5642YUV_write_cmos_sensor(0x8f0a, 0x40);
    OV5642YUV_write_cmos_sensor(0x8f0b, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8f0c, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8f0d, 0x54);
    OV5642YUV_write_cmos_sensor(0x8f0e, 0xbf);
    OV5642YUV_write_cmos_sensor(0x8f0f, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8f10, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8f11, 0x27);
    OV5642YUV_write_cmos_sensor(0x8f12, 0x30);
    OV5642YUV_write_cmos_sensor(0x8f13, 0xe1);
    OV5642YUV_write_cmos_sensor(0x8f14, 0x14);
    OV5642YUV_write_cmos_sensor(0x8f15, 0x30);
    OV5642YUV_write_cmos_sensor(0x8f16, 0x34);
    OV5642YUV_write_cmos_sensor(0x8f17, 0x11);
    OV5642YUV_write_cmos_sensor(0x8f18, 0x90);
    OV5642YUV_write_cmos_sensor(0x8f19, 0x30);
    OV5642YUV_write_cmos_sensor(0x8f1a, 0x24);
    OV5642YUV_write_cmos_sensor(0x8f1b, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8f1c, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8f1d, 0x08);
    OV5642YUV_write_cmos_sensor(0x8f1e, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8f1f, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8f20, 0x30);
    OV5642YUV_write_cmos_sensor(0x8f21, 0x00);
    OV5642YUV_write_cmos_sensor(0x8f22, 0x03);
    OV5642YUV_write_cmos_sensor(0x8f23, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8f24, 0x80);
    OV5642YUV_write_cmos_sensor(0x8f25, 0x01);
    OV5642YUV_write_cmos_sensor(0x8f26, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8f27, 0x92);
    OV5642YUV_write_cmos_sensor(0x8f28, 0x08);
    OV5642YUV_write_cmos_sensor(0x8f29, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8f2a, 0x27);
    OV5642YUV_write_cmos_sensor(0x8f2b, 0x30);
    OV5642YUV_write_cmos_sensor(0x8f2c, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8f2d, 0x12);
    OV5642YUV_write_cmos_sensor(0x8f2e, 0x90);
    OV5642YUV_write_cmos_sensor(0x8f2f, 0x56);
    OV5642YUV_write_cmos_sensor(0x8f30, 0x90);
    OV5642YUV_write_cmos_sensor(0x8f31, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8f32, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8f33, 0x09);
    OV5642YUV_write_cmos_sensor(0x8f34, 0x30);
    OV5642YUV_write_cmos_sensor(0x8f35, 0x31);
    OV5642YUV_write_cmos_sensor(0x8f36, 0x09);
    OV5642YUV_write_cmos_sensor(0x8f37, 0x30);
    OV5642YUV_write_cmos_sensor(0x8f38, 0x05);
    OV5642YUV_write_cmos_sensor(0x8f39, 0x03);
    OV5642YUV_write_cmos_sensor(0x8f3a, 0xd3);
    OV5642YUV_write_cmos_sensor(0x8f3b, 0x80);
    OV5642YUV_write_cmos_sensor(0x8f3c, 0x01);
    OV5642YUV_write_cmos_sensor(0x8f3d, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8f3e, 0x92);
    OV5642YUV_write_cmos_sensor(0x8f3f, 0x0d);
    OV5642YUV_write_cmos_sensor(0x8f40, 0x90);
    OV5642YUV_write_cmos_sensor(0x8f41, 0x3f);
    OV5642YUV_write_cmos_sensor(0x8f42, 0x0c);
    OV5642YUV_write_cmos_sensor(0x8f43, 0xe5);
    OV5642YUV_write_cmos_sensor(0x8f44, 0x27);
    OV5642YUV_write_cmos_sensor(0x8f45, 0xf0);
    OV5642YUV_write_cmos_sensor(0x8f46, 0xd0);
    OV5642YUV_write_cmos_sensor(0x8f47, 0xd0);
    OV5642YUV_write_cmos_sensor(0x8f48, 0xd0);
    OV5642YUV_write_cmos_sensor(0x8f49, 0x82);
    OV5642YUV_write_cmos_sensor(0x8f4a, 0xd0);
    OV5642YUV_write_cmos_sensor(0x8f4b, 0x83);
    OV5642YUV_write_cmos_sensor(0x8f4c, 0xd0);
    OV5642YUV_write_cmos_sensor(0x8f4d, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8f4e, 0x32);
    OV5642YUV_write_cmos_sensor(0x8f4f, 0xad);
    OV5642YUV_write_cmos_sensor(0x8f50, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8f51, 0xac);
    OV5642YUV_write_cmos_sensor(0x8f52, 0x2d);
    OV5642YUV_write_cmos_sensor(0x8f53, 0xfa);
    OV5642YUV_write_cmos_sensor(0x8f54, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8f55, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8f56, 0x12);
    OV5642YUV_write_cmos_sensor(0x8f57, 0x08);
    OV5642YUV_write_cmos_sensor(0x8f58, 0x4a);
    OV5642YUV_write_cmos_sensor(0x8f59, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8f5a, 0x30);
    OV5642YUV_write_cmos_sensor(0x8f5b, 0x8e);
    OV5642YUV_write_cmos_sensor(0x8f5c, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8f5d, 0x8d);
    OV5642YUV_write_cmos_sensor(0x8f5e, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8f5f, 0x8c);
    OV5642YUV_write_cmos_sensor(0x8f60, 0x2d);
    OV5642YUV_write_cmos_sensor(0x8f61, 0xab);
    OV5642YUV_write_cmos_sensor(0x8f62, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8f63, 0xaa);
    OV5642YUV_write_cmos_sensor(0x8f64, 0x2b);
    OV5642YUV_write_cmos_sensor(0x8f65, 0xa9);
    OV5642YUV_write_cmos_sensor(0x8f66, 0x2a);
    OV5642YUV_write_cmos_sensor(0x8f67, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8f68, 0x29);
    OV5642YUV_write_cmos_sensor(0x8f69, 0x22);
    OV5642YUV_write_cmos_sensor(0x8f6a, 0xef);
    OV5642YUV_write_cmos_sensor(0x8f6b, 0x25);
    OV5642YUV_write_cmos_sensor(0x8f6c, 0xe0);
    OV5642YUV_write_cmos_sensor(0x8f6d, 0x24);
    OV5642YUV_write_cmos_sensor(0x8f6e, 0x51);
    OV5642YUV_write_cmos_sensor(0x8f6f, 0xf8);
    OV5642YUV_write_cmos_sensor(0x8f70, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8f71, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8f72, 0x08);
    OV5642YUV_write_cmos_sensor(0x8f73, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8f74, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8f75, 0x22);
    OV5642YUV_write_cmos_sensor(0x8f76, 0x93);
    OV5642YUV_write_cmos_sensor(0x8f77, 0xff);
    OV5642YUV_write_cmos_sensor(0x8f78, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8f79, 0xfc);
    OV5642YUV_write_cmos_sensor(0x8f7a, 0xfd);
    OV5642YUV_write_cmos_sensor(0x8f7b, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8f7c, 0x12);
    OV5642YUV_write_cmos_sensor(0x8f7d, 0x08);
    OV5642YUV_write_cmos_sensor(0x8f7e, 0x4a);
    OV5642YUV_write_cmos_sensor(0x8f7f, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8f80, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8f81, 0x8e);
    OV5642YUV_write_cmos_sensor(0x8f82, 0x2b);
    OV5642YUV_write_cmos_sensor(0x8f83, 0x8d);
    OV5642YUV_write_cmos_sensor(0x8f84, 0x2a);
    OV5642YUV_write_cmos_sensor(0x8f85, 0x8c);
    OV5642YUV_write_cmos_sensor(0x8f86, 0x29);
    OV5642YUV_write_cmos_sensor(0x8f87, 0x22);
    OV5642YUV_write_cmos_sensor(0x8f88, 0xf9);
    OV5642YUV_write_cmos_sensor(0x8f89, 0xc3);
    OV5642YUV_write_cmos_sensor(0x8f8a, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8f8b, 0x97);
    OV5642YUV_write_cmos_sensor(0x8f8c, 0x18);
    OV5642YUV_write_cmos_sensor(0x8f8d, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8f8e, 0x19);
    OV5642YUV_write_cmos_sensor(0x8f8f, 0x97);
    OV5642YUV_write_cmos_sensor(0x8f90, 0x22);
    OV5642YUV_write_cmos_sensor(0x8f91, 0xff);
    OV5642YUV_write_cmos_sensor(0x8f92, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8f93, 0x06);
    OV5642YUV_write_cmos_sensor(0x8f94, 0x08);
    OV5642YUV_write_cmos_sensor(0x8f95, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8f96, 0x07);
    OV5642YUV_write_cmos_sensor(0x8f97, 0x22);
    OV5642YUV_write_cmos_sensor(0x8f98, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8f99, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8f9a, 0x08);
    OV5642YUV_write_cmos_sensor(0x8f9b, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8f9c, 0xff);
    OV5642YUV_write_cmos_sensor(0x8f9d, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8f9e, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8f9f, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8fa0, 0x8e);
    OV5642YUV_write_cmos_sensor(0x8fa1, 0x2b);
    OV5642YUV_write_cmos_sensor(0x8fa2, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8fa3, 0x2a);
    OV5642YUV_write_cmos_sensor(0x8fa4, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8fa5, 0x29);
    OV5642YUV_write_cmos_sensor(0x8fa6, 0x22);
    OV5642YUV_write_cmos_sensor(0x8fa7, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8fa8, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8fa9, 0x08);
    OV5642YUV_write_cmos_sensor(0x8faa, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8fab, 0xff);
    OV5642YUV_write_cmos_sensor(0x8fac, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8fad, 0x8f);
    OV5642YUV_write_cmos_sensor(0x8fae, 0x30);
    OV5642YUV_write_cmos_sensor(0x8faf, 0x8e);
    OV5642YUV_write_cmos_sensor(0x8fb0, 0x2f);
    OV5642YUV_write_cmos_sensor(0x8fb1, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8fb2, 0x2e);
    OV5642YUV_write_cmos_sensor(0x8fb3, 0xf5);
    OV5642YUV_write_cmos_sensor(0x8fb4, 0x2d);
    OV5642YUV_write_cmos_sensor(0x8fb5, 0x22);
    OV5642YUV_write_cmos_sensor(0x8fb6, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8fb7, 0x96);
    OV5642YUV_write_cmos_sensor(0x8fb8, 0xff);
    OV5642YUV_write_cmos_sensor(0x8fb9, 0x19);
    OV5642YUV_write_cmos_sensor(0x8fba, 0xe7);
    OV5642YUV_write_cmos_sensor(0x8fbb, 0x18);
    OV5642YUV_write_cmos_sensor(0x8fbc, 0x96);
    OV5642YUV_write_cmos_sensor(0x8fbd, 0x22);
    OV5642YUV_write_cmos_sensor(0x8fbe, 0xff);
    OV5642YUV_write_cmos_sensor(0x8fbf, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8fc0, 0x06);
    OV5642YUV_write_cmos_sensor(0x8fc1, 0x08);
    OV5642YUV_write_cmos_sensor(0x8fc2, 0xa6);
    OV5642YUV_write_cmos_sensor(0x8fc3, 0x07);
    OV5642YUV_write_cmos_sensor(0x8fc4, 0x78);
    OV5642YUV_write_cmos_sensor(0x8fc5, 0x71);
    OV5642YUV_write_cmos_sensor(0x8fc6, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8fc7, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8fc8, 0x08);
    OV5642YUV_write_cmos_sensor(0x8fc9, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8fca, 0x22);
    OV5642YUV_write_cmos_sensor(0x8fcb, 0x78);
    OV5642YUV_write_cmos_sensor(0x8fcc, 0x51);
    OV5642YUV_write_cmos_sensor(0x8fcd, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8fce, 0xfe);
    OV5642YUV_write_cmos_sensor(0x8fcf, 0x08);
    OV5642YUV_write_cmos_sensor(0x8fd0, 0xe6);
    OV5642YUV_write_cmos_sensor(0x8fd1, 0x22);
    OV5642YUV_write_cmos_sensor(0x8fd2, 0x78);
    OV5642YUV_write_cmos_sensor(0x8fd3, 0xac);
    OV5642YUV_write_cmos_sensor(0x8fd4, 0xef);
    OV5642YUV_write_cmos_sensor(0x8fd5, 0x26);
    OV5642YUV_write_cmos_sensor(0x8fd6, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8fd7, 0x18);
    OV5642YUV_write_cmos_sensor(0x8fd8, 0xe4);
    OV5642YUV_write_cmos_sensor(0x8fd9, 0x36);
    OV5642YUV_write_cmos_sensor(0x8fda, 0xf6);
    OV5642YUV_write_cmos_sensor(0x8fdb, 0x22);
    OV5642YUV_write_cmos_sensor(0x8fdc, 0x75);
    OV5642YUV_write_cmos_sensor(0x8fdd, 0x89);
    OV5642YUV_write_cmos_sensor(0x8fde, 0x03);
    OV5642YUV_write_cmos_sensor(0x8fdf, 0x75);
    OV5642YUV_write_cmos_sensor(0x8fe0, 0xa8);
    OV5642YUV_write_cmos_sensor(0x8fe1, 0x01);
    OV5642YUV_write_cmos_sensor(0x8fe2, 0x75);
    OV5642YUV_write_cmos_sensor(0x8fe3, 0xb8);
    OV5642YUV_write_cmos_sensor(0x8fe4, 0x04);
    OV5642YUV_write_cmos_sensor(0x8fe5, 0x75);
    OV5642YUV_write_cmos_sensor(0x8fe6, 0x29);
    OV5642YUV_write_cmos_sensor(0x8fe7, 0xff);
    OV5642YUV_write_cmos_sensor(0x8fe8, 0x75);
    OV5642YUV_write_cmos_sensor(0x8fe9, 0x2a);
    OV5642YUV_write_cmos_sensor(0x8fea, 0x0e);
    OV5642YUV_write_cmos_sensor(0x8feb, 0x75);
    OV5642YUV_write_cmos_sensor(0x8fec, 0x2b);
    OV5642YUV_write_cmos_sensor(0x8fed, 0x15);
    OV5642YUV_write_cmos_sensor(0x8fee, 0x75);
    OV5642YUV_write_cmos_sensor(0x8fef, 0x2c);
    OV5642YUV_write_cmos_sensor(0x8ff0, 0x10);
    OV5642YUV_write_cmos_sensor(0x8ff1, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ff2, 0x11);
    OV5642YUV_write_cmos_sensor(0x8ff3, 0x63);
    OV5642YUV_write_cmos_sensor(0x8ff4, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ff5, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ff6, 0x48);
    OV5642YUV_write_cmos_sensor(0x8ff7, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ff8, 0x00);
    OV5642YUV_write_cmos_sensor(0x8ff9, 0x09);
    OV5642YUV_write_cmos_sensor(0x8ffa, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ffb, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ffc, 0x18);
    OV5642YUV_write_cmos_sensor(0x8ffd, 0x12);
    OV5642YUV_write_cmos_sensor(0x8ffe, 0x00);
    OV5642YUV_write_cmos_sensor(0x8fff, 0x06);
    OV5642YUV_write_cmos_sensor(0x9000, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9001, 0x00);
    OV5642YUV_write_cmos_sensor(0x9002, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9003, 0x34);
    OV5642YUV_write_cmos_sensor(0x9004, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9005, 0xaf);
    OV5642YUV_write_cmos_sensor(0x9006, 0x75);
    OV5642YUV_write_cmos_sensor(0x9007, 0x29);
    OV5642YUV_write_cmos_sensor(0x9008, 0xff);
    OV5642YUV_write_cmos_sensor(0x9009, 0x75);
    OV5642YUV_write_cmos_sensor(0x900a, 0x2a);
    OV5642YUV_write_cmos_sensor(0x900b, 0x0e);
    OV5642YUV_write_cmos_sensor(0x900c, 0x75);
    OV5642YUV_write_cmos_sensor(0x900d, 0x2b);
    OV5642YUV_write_cmos_sensor(0x900e, 0x55);
    OV5642YUV_write_cmos_sensor(0x900f, 0x75);
    OV5642YUV_write_cmos_sensor(0x9010, 0x2c);
    OV5642YUV_write_cmos_sensor(0x9011, 0x03);
    OV5642YUV_write_cmos_sensor(0x9012, 0x12);
    OV5642YUV_write_cmos_sensor(0x9013, 0x11);
    OV5642YUV_write_cmos_sensor(0x9014, 0x63);
    OV5642YUV_write_cmos_sensor(0x9015, 0x30);
    OV5642YUV_write_cmos_sensor(0x9016, 0x08);
    OV5642YUV_write_cmos_sensor(0x9017, 0x09);
    OV5642YUV_write_cmos_sensor(0x9018, 0xc2);
    OV5642YUV_write_cmos_sensor(0x9019, 0x34);
    OV5642YUV_write_cmos_sensor(0x901a, 0x12);
    OV5642YUV_write_cmos_sensor(0x901b, 0x09);
    OV5642YUV_write_cmos_sensor(0x901c, 0xe6);
    OV5642YUV_write_cmos_sensor(0x901d, 0xc2);
    OV5642YUV_write_cmos_sensor(0x901e, 0x08);
    OV5642YUV_write_cmos_sensor(0x901f, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9020, 0x34);
    OV5642YUV_write_cmos_sensor(0x9021, 0x30);
    OV5642YUV_write_cmos_sensor(0x9022, 0x0b);
    OV5642YUV_write_cmos_sensor(0x9023, 0x09);
    OV5642YUV_write_cmos_sensor(0x9024, 0xc2);
    OV5642YUV_write_cmos_sensor(0x9025, 0x36);
    OV5642YUV_write_cmos_sensor(0x9026, 0x12);
    OV5642YUV_write_cmos_sensor(0x9027, 0x00);
    OV5642YUV_write_cmos_sensor(0x9028, 0x0e);
    OV5642YUV_write_cmos_sensor(0x9029, 0xc2);
    OV5642YUV_write_cmos_sensor(0x902a, 0x0b);
    OV5642YUV_write_cmos_sensor(0x902b, 0xd2);
    OV5642YUV_write_cmos_sensor(0x902c, 0x36);
    OV5642YUV_write_cmos_sensor(0x902d, 0x30);
    OV5642YUV_write_cmos_sensor(0x902e, 0x09);
    OV5642YUV_write_cmos_sensor(0x902f, 0x09);
    OV5642YUV_write_cmos_sensor(0x9030, 0xc2);
    OV5642YUV_write_cmos_sensor(0x9031, 0x36);
    OV5642YUV_write_cmos_sensor(0x9032, 0x12);
    OV5642YUV_write_cmos_sensor(0x9033, 0x05);
    OV5642YUV_write_cmos_sensor(0x9034, 0x5a);
    OV5642YUV_write_cmos_sensor(0x9035, 0xc2);
    OV5642YUV_write_cmos_sensor(0x9036, 0x09);
    OV5642YUV_write_cmos_sensor(0x9037, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9038, 0x36);
    OV5642YUV_write_cmos_sensor(0x9039, 0x30);
    OV5642YUV_write_cmos_sensor(0x903a, 0x0e);
    OV5642YUV_write_cmos_sensor(0x903b, 0x03);
    OV5642YUV_write_cmos_sensor(0x903c, 0x12);
    OV5642YUV_write_cmos_sensor(0x903d, 0x0b);
    OV5642YUV_write_cmos_sensor(0x903e, 0x4f);
    OV5642YUV_write_cmos_sensor(0x903f, 0x30);
    OV5642YUV_write_cmos_sensor(0x9040, 0x35);
    OV5642YUV_write_cmos_sensor(0x9041, 0xd3);
    OV5642YUV_write_cmos_sensor(0x9042, 0x90);
    OV5642YUV_write_cmos_sensor(0x9043, 0x30);
    OV5642YUV_write_cmos_sensor(0x9044, 0x27);
    OV5642YUV_write_cmos_sensor(0x9045, 0xe5);
    OV5642YUV_write_cmos_sensor(0x9046, 0x1e);
    OV5642YUV_write_cmos_sensor(0x9047, 0xf0);
    OV5642YUV_write_cmos_sensor(0x9048, 0xb4);
    OV5642YUV_write_cmos_sensor(0x9049, 0x10);
    OV5642YUV_write_cmos_sensor(0x904a, 0x05);
    OV5642YUV_write_cmos_sensor(0x904b, 0x90);
    OV5642YUV_write_cmos_sensor(0x904c, 0x30);
    OV5642YUV_write_cmos_sensor(0x904d, 0x25);
    OV5642YUV_write_cmos_sensor(0x904e, 0xe4);
    OV5642YUV_write_cmos_sensor(0x904f, 0xf0);
    OV5642YUV_write_cmos_sensor(0x9050, 0xc2);
    OV5642YUV_write_cmos_sensor(0x9051, 0x35);
    OV5642YUV_write_cmos_sensor(0x9052, 0x80);
    OV5642YUV_write_cmos_sensor(0x9053, 0xc1);
    OV5642YUV_write_cmos_sensor(0x9054, 0xe4);
    OV5642YUV_write_cmos_sensor(0x9055, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9056, 0x4e);
    OV5642YUV_write_cmos_sensor(0x9057, 0x90);
    OV5642YUV_write_cmos_sensor(0x9058, 0x0e);
    OV5642YUV_write_cmos_sensor(0x9059, 0x95);
    OV5642YUV_write_cmos_sensor(0x905a, 0x93);
    OV5642YUV_write_cmos_sensor(0x905b, 0xff);
    OV5642YUV_write_cmos_sensor(0x905c, 0xe4);
    OV5642YUV_write_cmos_sensor(0x905d, 0x8f);
    OV5642YUV_write_cmos_sensor(0x905e, 0x2c);
    OV5642YUV_write_cmos_sensor(0x905f, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9060, 0x2b);
    OV5642YUV_write_cmos_sensor(0x9061, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9062, 0x2a);
    OV5642YUV_write_cmos_sensor(0x9063, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9064, 0x29);
    OV5642YUV_write_cmos_sensor(0x9065, 0xaf);
    OV5642YUV_write_cmos_sensor(0x9066, 0x2c);
    OV5642YUV_write_cmos_sensor(0x9067, 0xae);
    OV5642YUV_write_cmos_sensor(0x9068, 0x2b);
    OV5642YUV_write_cmos_sensor(0x9069, 0xad);
    OV5642YUV_write_cmos_sensor(0x906a, 0x2a);
    OV5642YUV_write_cmos_sensor(0x906b, 0xac);
    OV5642YUV_write_cmos_sensor(0x906c, 0x29);
    OV5642YUV_write_cmos_sensor(0x906d, 0x90);
    OV5642YUV_write_cmos_sensor(0x906e, 0x0e);
    OV5642YUV_write_cmos_sensor(0x906f, 0x88);
    OV5642YUV_write_cmos_sensor(0x9070, 0x12);
    OV5642YUV_write_cmos_sensor(0x9071, 0x12);
    OV5642YUV_write_cmos_sensor(0x9072, 0x12);
    OV5642YUV_write_cmos_sensor(0x9073, 0x8f);
    OV5642YUV_write_cmos_sensor(0x9074, 0x2c);
    OV5642YUV_write_cmos_sensor(0x9075, 0x8e);
    OV5642YUV_write_cmos_sensor(0x9076, 0x2b);
    OV5642YUV_write_cmos_sensor(0x9077, 0x8d);
    OV5642YUV_write_cmos_sensor(0x9078, 0x2a);
    OV5642YUV_write_cmos_sensor(0x9079, 0x8c);
    OV5642YUV_write_cmos_sensor(0x907a, 0x29);
    OV5642YUV_write_cmos_sensor(0x907b, 0x90);
    OV5642YUV_write_cmos_sensor(0x907c, 0x0e);
    OV5642YUV_write_cmos_sensor(0x907d, 0x90);
    OV5642YUV_write_cmos_sensor(0x907e, 0x12);
    OV5642YUV_write_cmos_sensor(0x907f, 0x09);
    OV5642YUV_write_cmos_sensor(0x9080, 0x8b);
    OV5642YUV_write_cmos_sensor(0x9081, 0xef);
    OV5642YUV_write_cmos_sensor(0x9082, 0x45);
    OV5642YUV_write_cmos_sensor(0x9083, 0x2c);
    OV5642YUV_write_cmos_sensor(0x9084, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9085, 0x2c);
    OV5642YUV_write_cmos_sensor(0x9086, 0xee);
    OV5642YUV_write_cmos_sensor(0x9087, 0x45);
    OV5642YUV_write_cmos_sensor(0x9088, 0x2b);
    OV5642YUV_write_cmos_sensor(0x9089, 0xf5);
    OV5642YUV_write_cmos_sensor(0x908a, 0x2b);
    OV5642YUV_write_cmos_sensor(0x908b, 0xed);
    OV5642YUV_write_cmos_sensor(0x908c, 0x45);
    OV5642YUV_write_cmos_sensor(0x908d, 0x2a);
    OV5642YUV_write_cmos_sensor(0x908e, 0xf5);
    OV5642YUV_write_cmos_sensor(0x908f, 0x2a);
    OV5642YUV_write_cmos_sensor(0x9090, 0xec);
    OV5642YUV_write_cmos_sensor(0x9091, 0x45);
    OV5642YUV_write_cmos_sensor(0x9092, 0x29);
    OV5642YUV_write_cmos_sensor(0x9093, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9094, 0x29);
    OV5642YUV_write_cmos_sensor(0x9095, 0x12);
    OV5642YUV_write_cmos_sensor(0x9096, 0x11);
    OV5642YUV_write_cmos_sensor(0x9097, 0xdf);
    OV5642YUV_write_cmos_sensor(0x9098, 0x85);
    OV5642YUV_write_cmos_sensor(0x9099, 0x2c);
    OV5642YUV_write_cmos_sensor(0x909a, 0x40);
    OV5642YUV_write_cmos_sensor(0x909b, 0x85);
    OV5642YUV_write_cmos_sensor(0x909c, 0x2b);
    OV5642YUV_write_cmos_sensor(0x909d, 0x3f);
    OV5642YUV_write_cmos_sensor(0x909e, 0x85);
    OV5642YUV_write_cmos_sensor(0x909f, 0x2a);
    OV5642YUV_write_cmos_sensor(0x90a0, 0x3e);
    OV5642YUV_write_cmos_sensor(0x90a1, 0x85);
    OV5642YUV_write_cmos_sensor(0x90a2, 0x29);
    OV5642YUV_write_cmos_sensor(0x90a3, 0x3d);
    OV5642YUV_write_cmos_sensor(0x90a4, 0x12);
    OV5642YUV_write_cmos_sensor(0x90a5, 0x02);
    OV5642YUV_write_cmos_sensor(0x90a6, 0xe0);
    OV5642YUV_write_cmos_sensor(0x90a7, 0xe4);
    OV5642YUV_write_cmos_sensor(0x90a8, 0xf5);
    OV5642YUV_write_cmos_sensor(0x90a9, 0x22);
    OV5642YUV_write_cmos_sensor(0x90aa, 0xf5);
    OV5642YUV_write_cmos_sensor(0x90ab, 0x23);
    OV5642YUV_write_cmos_sensor(0x90ac, 0x90);
    OV5642YUV_write_cmos_sensor(0x90ad, 0x0e);
    OV5642YUV_write_cmos_sensor(0x90ae, 0x90);
    OV5642YUV_write_cmos_sensor(0x90af, 0x12);
    OV5642YUV_write_cmos_sensor(0x90b0, 0x12);
    OV5642YUV_write_cmos_sensor(0x90b1, 0x06);
    OV5642YUV_write_cmos_sensor(0x90b2, 0x12);
    OV5642YUV_write_cmos_sensor(0x90b3, 0x02);
    OV5642YUV_write_cmos_sensor(0x90b4, 0xe0);
    OV5642YUV_write_cmos_sensor(0x90b5, 0xe4);
    OV5642YUV_write_cmos_sensor(0x90b6, 0xf5);
    OV5642YUV_write_cmos_sensor(0x90b7, 0x22);
    OV5642YUV_write_cmos_sensor(0x90b8, 0xf5);
    OV5642YUV_write_cmos_sensor(0x90b9, 0x23);
    OV5642YUV_write_cmos_sensor(0x90ba, 0x90);
    OV5642YUV_write_cmos_sensor(0x90bb, 0x0e);
    OV5642YUV_write_cmos_sensor(0x90bc, 0x8c);
    OV5642YUV_write_cmos_sensor(0x90bd, 0x12);
    OV5642YUV_write_cmos_sensor(0x90be, 0x12);
    OV5642YUV_write_cmos_sensor(0x90bf, 0x06);
    OV5642YUV_write_cmos_sensor(0x90c0, 0x02);
    OV5642YUV_write_cmos_sensor(0x90c1, 0x02);
    OV5642YUV_write_cmos_sensor(0x90c2, 0xe0);
    OV5642YUV_write_cmos_sensor(0x90c3, 0x78);
    OV5642YUV_write_cmos_sensor(0x90c4, 0x55);
    OV5642YUV_write_cmos_sensor(0x90c5, 0x7e);
    OV5642YUV_write_cmos_sensor(0x90c6, 0x00);
    OV5642YUV_write_cmos_sensor(0x90c7, 0xe6);
    OV5642YUV_write_cmos_sensor(0x90c8, 0xfc);
    OV5642YUV_write_cmos_sensor(0x90c9, 0x08);
    OV5642YUV_write_cmos_sensor(0x90ca, 0xe6);
    OV5642YUV_write_cmos_sensor(0x90cb, 0xfd);
    OV5642YUV_write_cmos_sensor(0x90cc, 0x12);
    OV5642YUV_write_cmos_sensor(0x90cd, 0x07);
    OV5642YUV_write_cmos_sensor(0x90ce, 0xe3);
    OV5642YUV_write_cmos_sensor(0x90cf, 0x7c);
    OV5642YUV_write_cmos_sensor(0x90d0, 0x00);
    OV5642YUV_write_cmos_sensor(0x90d1, 0x22);
    OV5642YUV_write_cmos_sensor(0x90d2, 0xff);
    OV5642YUV_write_cmos_sensor(0x90d3, 0xe5);
    OV5642YUV_write_cmos_sensor(0x90d4, 0xf0);
    OV5642YUV_write_cmos_sensor(0x90d5, 0x34);
    OV5642YUV_write_cmos_sensor(0x90d6, 0x60);
    OV5642YUV_write_cmos_sensor(0x90d7, 0x8f);
    OV5642YUV_write_cmos_sensor(0x90d8, 0x82);
    OV5642YUV_write_cmos_sensor(0x90d9, 0xf5);
    OV5642YUV_write_cmos_sensor(0x90da, 0x83);
    OV5642YUV_write_cmos_sensor(0x90db, 0xec);
    OV5642YUV_write_cmos_sensor(0x90dc, 0xf0);
    OV5642YUV_write_cmos_sensor(0x90dd, 0x22);
    OV5642YUV_write_cmos_sensor(0x90de, 0xe4);
    OV5642YUV_write_cmos_sensor(0x90df, 0xfc);
    OV5642YUV_write_cmos_sensor(0x90e0, 0xfd);
    OV5642YUV_write_cmos_sensor(0x90e1, 0x12);
    OV5642YUV_write_cmos_sensor(0x90e2, 0x09);
    OV5642YUV_write_cmos_sensor(0x90e3, 0xa8);
    OV5642YUV_write_cmos_sensor(0x90e4, 0x78);
    OV5642YUV_write_cmos_sensor(0x90e5, 0x5f);
    OV5642YUV_write_cmos_sensor(0x90e6, 0xe6);
    OV5642YUV_write_cmos_sensor(0x90e7, 0xc3);
    OV5642YUV_write_cmos_sensor(0x90e8, 0x13);
    OV5642YUV_write_cmos_sensor(0x90e9, 0xfe);
    OV5642YUV_write_cmos_sensor(0x90ea, 0x08);
    OV5642YUV_write_cmos_sensor(0x90eb, 0xe6);
    OV5642YUV_write_cmos_sensor(0x90ec, 0x13);
    OV5642YUV_write_cmos_sensor(0x90ed, 0x22);
    OV5642YUV_write_cmos_sensor(0x90ee, 0x78);
    OV5642YUV_write_cmos_sensor(0x90ef, 0x55);
    OV5642YUV_write_cmos_sensor(0x90f0, 0xe6);
    OV5642YUV_write_cmos_sensor(0x90f1, 0xfe);
    OV5642YUV_write_cmos_sensor(0x90f2, 0x08);
    OV5642YUV_write_cmos_sensor(0x90f3, 0xe6);
    OV5642YUV_write_cmos_sensor(0x90f4, 0xff);
    OV5642YUV_write_cmos_sensor(0x90f5, 0xe4);
    OV5642YUV_write_cmos_sensor(0x90f6, 0xfc);
    OV5642YUV_write_cmos_sensor(0x90f7, 0xfd);
    OV5642YUV_write_cmos_sensor(0x90f8, 0x22);
    OV5642YUV_write_cmos_sensor(0x90f9, 0xe7);
    OV5642YUV_write_cmos_sensor(0x90fa, 0xc4);
    OV5642YUV_write_cmos_sensor(0x90fb, 0xf8);
    OV5642YUV_write_cmos_sensor(0x90fc, 0x54);
    OV5642YUV_write_cmos_sensor(0x90fd, 0xf0);
    OV5642YUV_write_cmos_sensor(0x90fe, 0xc8);
    OV5642YUV_write_cmos_sensor(0x90ff, 0x68);
    OV5642YUV_write_cmos_sensor(0x9100, 0xf7);
    OV5642YUV_write_cmos_sensor(0x9101, 0x09);
    OV5642YUV_write_cmos_sensor(0x9102, 0xe7);
    OV5642YUV_write_cmos_sensor(0x9103, 0xc4);
    OV5642YUV_write_cmos_sensor(0x9104, 0x54);
    OV5642YUV_write_cmos_sensor(0x9105, 0x0f);
    OV5642YUV_write_cmos_sensor(0x9106, 0x48);
    OV5642YUV_write_cmos_sensor(0x9107, 0xf7);
    OV5642YUV_write_cmos_sensor(0x9108, 0x22);
    OV5642YUV_write_cmos_sensor(0x9109, 0xe6);
    OV5642YUV_write_cmos_sensor(0x910a, 0xfc);
    OV5642YUV_write_cmos_sensor(0x910b, 0xed);
    OV5642YUV_write_cmos_sensor(0x910c, 0x75);
    OV5642YUV_write_cmos_sensor(0x910d, 0xf0);
    OV5642YUV_write_cmos_sensor(0x910e, 0x04);
    OV5642YUV_write_cmos_sensor(0x910f, 0xa4);
    OV5642YUV_write_cmos_sensor(0x9110, 0x22);
    OV5642YUV_write_cmos_sensor(0x9111, 0x13);
    OV5642YUV_write_cmos_sensor(0x9112, 0xff);
    OV5642YUV_write_cmos_sensor(0x9113, 0xc3);
    OV5642YUV_write_cmos_sensor(0x9114, 0xe6);
    OV5642YUV_write_cmos_sensor(0x9115, 0x9f);
    OV5642YUV_write_cmos_sensor(0x9116, 0xff);
    OV5642YUV_write_cmos_sensor(0x9117, 0x18);
    OV5642YUV_write_cmos_sensor(0x9118, 0xe6);
    OV5642YUV_write_cmos_sensor(0x9119, 0x9e);
    OV5642YUV_write_cmos_sensor(0x911a, 0xfe);
    OV5642YUV_write_cmos_sensor(0x911b, 0x22);
    OV5642YUV_write_cmos_sensor(0x911c, 0xd2);
    OV5642YUV_write_cmos_sensor(0x911d, 0x01);
    OV5642YUV_write_cmos_sensor(0x911e, 0xc2);
    OV5642YUV_write_cmos_sensor(0x911f, 0x02);
    OV5642YUV_write_cmos_sensor(0x9120, 0xe4);
    OV5642YUV_write_cmos_sensor(0x9121, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9122, 0x1f);
    OV5642YUV_write_cmos_sensor(0x9123, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9124, 0x1e);
    OV5642YUV_write_cmos_sensor(0x9125, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9126, 0x35);
    OV5642YUV_write_cmos_sensor(0x9127, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9128, 0x33);
    OV5642YUV_write_cmos_sensor(0x9129, 0xd2);
    OV5642YUV_write_cmos_sensor(0x912a, 0x36);
    OV5642YUV_write_cmos_sensor(0x912b, 0xd2);
    OV5642YUV_write_cmos_sensor(0x912c, 0x01);
    OV5642YUV_write_cmos_sensor(0x912d, 0xc2);
    OV5642YUV_write_cmos_sensor(0x912e, 0x02);
    OV5642YUV_write_cmos_sensor(0x912f, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9130, 0x1f);
    OV5642YUV_write_cmos_sensor(0x9131, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9132, 0x1e);
    OV5642YUV_write_cmos_sensor(0x9133, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9134, 0x35);
    OV5642YUV_write_cmos_sensor(0x9135, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9136, 0x33);
    OV5642YUV_write_cmos_sensor(0x9137, 0x22);
    OV5642YUV_write_cmos_sensor(0x9138, 0x90);
    OV5642YUV_write_cmos_sensor(0x9139, 0x0e);
    OV5642YUV_write_cmos_sensor(0x913a, 0x83);
    OV5642YUV_write_cmos_sensor(0x913b, 0xe4);
    OV5642YUV_write_cmos_sensor(0x913c, 0x93);
    OV5642YUV_write_cmos_sensor(0x913d, 0xf5);
    OV5642YUV_write_cmos_sensor(0x913e, 0x36);
    OV5642YUV_write_cmos_sensor(0x913f, 0x75);
    OV5642YUV_write_cmos_sensor(0x9140, 0x24);
    OV5642YUV_write_cmos_sensor(0x9141, 0x00);
    OV5642YUV_write_cmos_sensor(0x9142, 0x75);
    OV5642YUV_write_cmos_sensor(0x9143, 0x25);
    OV5642YUV_write_cmos_sensor(0x9144, 0x01);
    OV5642YUV_write_cmos_sensor(0x9145, 0xa3);
    OV5642YUV_write_cmos_sensor(0x9146, 0xe4);
    OV5642YUV_write_cmos_sensor(0x9147, 0x93);
    OV5642YUV_write_cmos_sensor(0x9148, 0xff);
    OV5642YUV_write_cmos_sensor(0x9149, 0x13);
    OV5642YUV_write_cmos_sensor(0x914a, 0x13);
    OV5642YUV_write_cmos_sensor(0x914b, 0x13);
    OV5642YUV_write_cmos_sensor(0x914c, 0x54);
    OV5642YUV_write_cmos_sensor(0x914d, 0x1f);
    OV5642YUV_write_cmos_sensor(0x914e, 0xf5);
    OV5642YUV_write_cmos_sensor(0x914f, 0x37);
    OV5642YUV_write_cmos_sensor(0x9150, 0x22);
    OV5642YUV_write_cmos_sensor(0x9151, 0x2d);
    OV5642YUV_write_cmos_sensor(0x9152, 0xfd);
    OV5642YUV_write_cmos_sensor(0x9153, 0xe4);
    OV5642YUV_write_cmos_sensor(0x9154, 0x33);
    OV5642YUV_write_cmos_sensor(0x9155, 0xfc);
    OV5642YUV_write_cmos_sensor(0x9156, 0xe4);
    OV5642YUV_write_cmos_sensor(0x9157, 0x93);
    OV5642YUV_write_cmos_sensor(0x9158, 0xfe);
    OV5642YUV_write_cmos_sensor(0x9159, 0xfb);
    OV5642YUV_write_cmos_sensor(0x915a, 0xd3);
    OV5642YUV_write_cmos_sensor(0x915b, 0xed);
    OV5642YUV_write_cmos_sensor(0x915c, 0x9b);
    OV5642YUV_write_cmos_sensor(0x915d, 0x74);
    OV5642YUV_write_cmos_sensor(0x915e, 0x80);
    OV5642YUV_write_cmos_sensor(0x915f, 0xf8);
    OV5642YUV_write_cmos_sensor(0x9160, 0x6c);
    OV5642YUV_write_cmos_sensor(0x9161, 0x98);
    OV5642YUV_write_cmos_sensor(0x9162, 0x22);
    OV5642YUV_write_cmos_sensor(0x9163, 0xae);
    OV5642YUV_write_cmos_sensor(0x9164, 0x2a);
    OV5642YUV_write_cmos_sensor(0x9165, 0xaf);
    OV5642YUV_write_cmos_sensor(0x9166, 0x2b);
    OV5642YUV_write_cmos_sensor(0x9167, 0xe4);
    OV5642YUV_write_cmos_sensor(0x9168, 0xfd);
    OV5642YUV_write_cmos_sensor(0x9169, 0xed);
    OV5642YUV_write_cmos_sensor(0x916a, 0xc3);
    OV5642YUV_write_cmos_sensor(0x916b, 0x95);
    OV5642YUV_write_cmos_sensor(0x916c, 0x2c);
    OV5642YUV_write_cmos_sensor(0x916d, 0x50);
    OV5642YUV_write_cmos_sensor(0x916e, 0x33);
    OV5642YUV_write_cmos_sensor(0x916f, 0x12);
    OV5642YUV_write_cmos_sensor(0x9170, 0x12);
    OV5642YUV_write_cmos_sensor(0x9171, 0x61);
    OV5642YUV_write_cmos_sensor(0x9172, 0xe4);
    OV5642YUV_write_cmos_sensor(0x9173, 0x93);
    OV5642YUV_write_cmos_sensor(0x9174, 0xf5);
    OV5642YUV_write_cmos_sensor(0x9175, 0x2d);
    OV5642YUV_write_cmos_sensor(0x9176, 0x74);
    OV5642YUV_write_cmos_sensor(0x9177, 0x01);
    OV5642YUV_write_cmos_sensor(0x9178, 0x93);
    OV5642YUV_write_cmos_sensor(0x9179, 0xf5);
    OV5642YUV_write_cmos_sensor(0x917a, 0x2e);
    OV5642YUV_write_cmos_sensor(0x917b, 0x45);
    OV5642YUV_write_cmos_sensor(0x917c, 0x2d);
    OV5642YUV_write_cmos_sensor(0x917d, 0x60);
    OV5642YUV_write_cmos_sensor(0x917e, 0x23);
    OV5642YUV_write_cmos_sensor(0x917f, 0x85);
    OV5642YUV_write_cmos_sensor(0x9180, 0x2e);
    OV5642YUV_write_cmos_sensor(0x9181, 0x82);
    OV5642YUV_write_cmos_sensor(0x9182, 0x85);
    OV5642YUV_write_cmos_sensor(0x9183, 0x2d);
    OV5642YUV_write_cmos_sensor(0x9184, 0x83);
    OV5642YUV_write_cmos_sensor(0x9185, 0xe0);
    OV5642YUV_write_cmos_sensor(0x9186, 0xfc);
    OV5642YUV_write_cmos_sensor(0x9187, 0x12);
    OV5642YUV_write_cmos_sensor(0x9188, 0x12);
    OV5642YUV_write_cmos_sensor(0x9189, 0x61);
    OV5642YUV_write_cmos_sensor(0x918a, 0x74);
    OV5642YUV_write_cmos_sensor(0x918b, 0x03);
    OV5642YUV_write_cmos_sensor(0x918c, 0x93);
    OV5642YUV_write_cmos_sensor(0x918d, 0x52);
    OV5642YUV_write_cmos_sensor(0x918e, 0x04);
    OV5642YUV_write_cmos_sensor(0x918f, 0x12);
    OV5642YUV_write_cmos_sensor(0x9190, 0x12);
    OV5642YUV_write_cmos_sensor(0x9191, 0x61);
    OV5642YUV_write_cmos_sensor(0x9192, 0x74);
    OV5642YUV_write_cmos_sensor(0x9193, 0x02);
    OV5642YUV_write_cmos_sensor(0x9194, 0x93);
    OV5642YUV_write_cmos_sensor(0x9195, 0x42);
    OV5642YUV_write_cmos_sensor(0x9196, 0x04);
    OV5642YUV_write_cmos_sensor(0x9197, 0x85);
    OV5642YUV_write_cmos_sensor(0x9198, 0x2e);
    OV5642YUV_write_cmos_sensor(0x9199, 0x82);
    OV5642YUV_write_cmos_sensor(0x919a, 0x85);
    OV5642YUV_write_cmos_sensor(0x919b, 0x2d);
    OV5642YUV_write_cmos_sensor(0x919c, 0x83);
    OV5642YUV_write_cmos_sensor(0x919d, 0xec);
    OV5642YUV_write_cmos_sensor(0x919e, 0xf0);
    OV5642YUV_write_cmos_sensor(0x919f, 0x0d);
    OV5642YUV_write_cmos_sensor(0x91a0, 0x80);
    OV5642YUV_write_cmos_sensor(0x91a1, 0xc7);
    OV5642YUV_write_cmos_sensor(0x91a2, 0x22);
    OV5642YUV_write_cmos_sensor(0x91a3, 0x78);
    OV5642YUV_write_cmos_sensor(0x91a4, 0xc1);
    OV5642YUV_write_cmos_sensor(0x91a5, 0xe6);
    OV5642YUV_write_cmos_sensor(0x91a6, 0xd3);
    OV5642YUV_write_cmos_sensor(0x91a7, 0x08);
    OV5642YUV_write_cmos_sensor(0x91a8, 0xff);
    OV5642YUV_write_cmos_sensor(0x91a9, 0xe6);
    OV5642YUV_write_cmos_sensor(0x91aa, 0x64);
    OV5642YUV_write_cmos_sensor(0x91ab, 0x80);
    OV5642YUV_write_cmos_sensor(0x91ac, 0xf8);
    OV5642YUV_write_cmos_sensor(0x91ad, 0xef);
    OV5642YUV_write_cmos_sensor(0x91ae, 0x64);
    OV5642YUV_write_cmos_sensor(0x91af, 0x80);
    OV5642YUV_write_cmos_sensor(0x91b0, 0x98);
    OV5642YUV_write_cmos_sensor(0x91b1, 0x22);
    OV5642YUV_write_cmos_sensor(0x91b2, 0x93);
    OV5642YUV_write_cmos_sensor(0x91b3, 0xff);
    OV5642YUV_write_cmos_sensor(0x91b4, 0x7e);
    OV5642YUV_write_cmos_sensor(0x91b5, 0x00);
    OV5642YUV_write_cmos_sensor(0x91b6, 0xe6);
    OV5642YUV_write_cmos_sensor(0x91b7, 0xfc);
    OV5642YUV_write_cmos_sensor(0x91b8, 0x08);
    OV5642YUV_write_cmos_sensor(0x91b9, 0xe6);
    OV5642YUV_write_cmos_sensor(0x91ba, 0xfd);
    OV5642YUV_write_cmos_sensor(0x91bb, 0x12);
    OV5642YUV_write_cmos_sensor(0x91bc, 0x07);
    OV5642YUV_write_cmos_sensor(0x91bd, 0xe3);
    OV5642YUV_write_cmos_sensor(0x91be, 0x78);
    OV5642YUV_write_cmos_sensor(0x91bf, 0xc4);
    OV5642YUV_write_cmos_sensor(0x91c0, 0xe6);
    OV5642YUV_write_cmos_sensor(0x91c1, 0xfc);
    OV5642YUV_write_cmos_sensor(0x91c2, 0x08);
    OV5642YUV_write_cmos_sensor(0x91c3, 0xe6);
    OV5642YUV_write_cmos_sensor(0x91c4, 0xfd);
    OV5642YUV_write_cmos_sensor(0x91c5, 0xd3);
    OV5642YUV_write_cmos_sensor(0x91c6, 0xef);
    OV5642YUV_write_cmos_sensor(0x91c7, 0x9d);
    OV5642YUV_write_cmos_sensor(0x91c8, 0xee);
    OV5642YUV_write_cmos_sensor(0x91c9, 0x9c);
    OV5642YUV_write_cmos_sensor(0x91ca, 0x22);
    OV5642YUV_write_cmos_sensor(0x91cb, 0x78);
    OV5642YUV_write_cmos_sensor(0x91cc, 0xc0);
    OV5642YUV_write_cmos_sensor(0x91cd, 0xd3);
    OV5642YUV_write_cmos_sensor(0x91ce, 0xe6);
    OV5642YUV_write_cmos_sensor(0x91cf, 0x64);
    OV5642YUV_write_cmos_sensor(0x91d0, 0x80);
    OV5642YUV_write_cmos_sensor(0x91d1, 0x94);
    OV5642YUV_write_cmos_sensor(0x91d2, 0x80);
    OV5642YUV_write_cmos_sensor(0x91d3, 0x22);
    OV5642YUV_write_cmos_sensor(0x91d4, 0x25);
    OV5642YUV_write_cmos_sensor(0x91d5, 0xe0);
    OV5642YUV_write_cmos_sensor(0x91d6, 0x24);
    OV5642YUV_write_cmos_sensor(0x91d7, 0x0a);
    OV5642YUV_write_cmos_sensor(0x91d8, 0xf8);
    OV5642YUV_write_cmos_sensor(0x91d9, 0xe6);
    OV5642YUV_write_cmos_sensor(0x91da, 0xfe);
    OV5642YUV_write_cmos_sensor(0x91db, 0x08);
    OV5642YUV_write_cmos_sensor(0x91dc, 0xe6);
    OV5642YUV_write_cmos_sensor(0x91dd, 0xff);
    OV5642YUV_write_cmos_sensor(0x91de, 0x22);
    OV5642YUV_write_cmos_sensor(0x91df, 0x90);
    OV5642YUV_write_cmos_sensor(0x91e0, 0x0e);
    OV5642YUV_write_cmos_sensor(0x91e1, 0x83);
    OV5642YUV_write_cmos_sensor(0x91e2, 0xe4);
    OV5642YUV_write_cmos_sensor(0x91e3, 0x93);
    OV5642YUV_write_cmos_sensor(0x91e4, 0xf5);
    OV5642YUV_write_cmos_sensor(0x91e5, 0x36);
    OV5642YUV_write_cmos_sensor(0x91e6, 0x75);
    OV5642YUV_write_cmos_sensor(0x91e7, 0x24);
    OV5642YUV_write_cmos_sensor(0x91e8, 0x00);
    OV5642YUV_write_cmos_sensor(0x91e9, 0x75);
    OV5642YUV_write_cmos_sensor(0x91ea, 0x25);
    OV5642YUV_write_cmos_sensor(0x91eb, 0x01);
    OV5642YUV_write_cmos_sensor(0x91ec, 0xa3);
    OV5642YUV_write_cmos_sensor(0x91ed, 0xe4);
    OV5642YUV_write_cmos_sensor(0x91ee, 0x93);
    OV5642YUV_write_cmos_sensor(0x91ef, 0xff);
    OV5642YUV_write_cmos_sensor(0x91f0, 0x13);
    OV5642YUV_write_cmos_sensor(0x91f1, 0x13);
    OV5642YUV_write_cmos_sensor(0x91f2, 0x13);
    OV5642YUV_write_cmos_sensor(0x91f3, 0x54);
    OV5642YUV_write_cmos_sensor(0x91f4, 0x1f);
    OV5642YUV_write_cmos_sensor(0x91f5, 0xf5);
    OV5642YUV_write_cmos_sensor(0x91f6, 0x37);
    OV5642YUV_write_cmos_sensor(0x91f7, 0xe4);
    OV5642YUV_write_cmos_sensor(0x91f8, 0xf5);
    OV5642YUV_write_cmos_sensor(0x91f9, 0x22);
    OV5642YUV_write_cmos_sensor(0x91fa, 0xf5);
    OV5642YUV_write_cmos_sensor(0x91fb, 0x23);
    OV5642YUV_write_cmos_sensor(0x91fc, 0x22);
    OV5642YUV_write_cmos_sensor(0x91fd, 0x8f);
    OV5642YUV_write_cmos_sensor(0x91fe, 0x30);
    OV5642YUV_write_cmos_sensor(0x91ff, 0x8e);
    OV5642YUV_write_cmos_sensor(0x9200, 0x2f);
    OV5642YUV_write_cmos_sensor(0x9201, 0x8d);
    OV5642YUV_write_cmos_sensor(0x9202, 0x2e);
    OV5642YUV_write_cmos_sensor(0x9203, 0x8c);
    OV5642YUV_write_cmos_sensor(0x9204, 0x2d);
    OV5642YUV_write_cmos_sensor(0x9205, 0x22);
    OV5642YUV_write_cmos_sensor(0x9206, 0x12);
    OV5642YUV_write_cmos_sensor(0x9207, 0x09);
    OV5642YUV_write_cmos_sensor(0x9208, 0x8b);
    OV5642YUV_write_cmos_sensor(0x9209, 0x8f);
    OV5642YUV_write_cmos_sensor(0x920a, 0x40);
    OV5642YUV_write_cmos_sensor(0x920b, 0x8e);
    OV5642YUV_write_cmos_sensor(0x920c, 0x3f);
    OV5642YUV_write_cmos_sensor(0x920d, 0x8d);
    OV5642YUV_write_cmos_sensor(0x920e, 0x3e);
    OV5642YUV_write_cmos_sensor(0x920f, 0x8c);
    OV5642YUV_write_cmos_sensor(0x9210, 0x3d);
    OV5642YUV_write_cmos_sensor(0x9211, 0x22);
    OV5642YUV_write_cmos_sensor(0x9212, 0x93);
    OV5642YUV_write_cmos_sensor(0x9213, 0xf9);
    OV5642YUV_write_cmos_sensor(0x9214, 0xf8);
    OV5642YUV_write_cmos_sensor(0x9215, 0x02);
    OV5642YUV_write_cmos_sensor(0x9216, 0x09);
    OV5642YUV_write_cmos_sensor(0x9217, 0x78);
    OV5642YUV_write_cmos_sensor(0x9218, 0x90);
    OV5642YUV_write_cmos_sensor(0x9219, 0x0e);
    OV5642YUV_write_cmos_sensor(0x921a, 0x9f);
    OV5642YUV_write_cmos_sensor(0x921b, 0x12);
    OV5642YUV_write_cmos_sensor(0x921c, 0x09);
    OV5642YUV_write_cmos_sensor(0x921d, 0x8b);
    OV5642YUV_write_cmos_sensor(0x921e, 0x8f);
    OV5642YUV_write_cmos_sensor(0x921f, 0x4b);
    OV5642YUV_write_cmos_sensor(0x9220, 0x8e);
    OV5642YUV_write_cmos_sensor(0x9221, 0x4a);
    OV5642YUV_write_cmos_sensor(0x9222, 0x8d);
    OV5642YUV_write_cmos_sensor(0x9223, 0x49);
    OV5642YUV_write_cmos_sensor(0x9224, 0x8c);
    OV5642YUV_write_cmos_sensor(0x9225, 0x48);
    OV5642YUV_write_cmos_sensor(0x9226, 0xd2);
    OV5642YUV_write_cmos_sensor(0x9227, 0x06);
    OV5642YUV_write_cmos_sensor(0x9228, 0x30);
    OV5642YUV_write_cmos_sensor(0x9229, 0x06);
    OV5642YUV_write_cmos_sensor(0x922a, 0x03);
    OV5642YUV_write_cmos_sensor(0x922b, 0xd3);
    OV5642YUV_write_cmos_sensor(0x922c, 0x80);
    OV5642YUV_write_cmos_sensor(0x922d, 0x01);
    OV5642YUV_write_cmos_sensor(0x922e, 0xc3);
    OV5642YUV_write_cmos_sensor(0x922f, 0x92);
    OV5642YUV_write_cmos_sensor(0x9230, 0x0e);
    OV5642YUV_write_cmos_sensor(0x9231, 0x22);
    OV5642YUV_write_cmos_sensor(0x9232, 0xc0);
    OV5642YUV_write_cmos_sensor(0x9233, 0xe0);
    OV5642YUV_write_cmos_sensor(0x9234, 0xc0);
    OV5642YUV_write_cmos_sensor(0x9235, 0x83);
    OV5642YUV_write_cmos_sensor(0x9236, 0xc0);
    OV5642YUV_write_cmos_sensor(0x9237, 0x82);
    OV5642YUV_write_cmos_sensor(0x9238, 0x90);
    OV5642YUV_write_cmos_sensor(0x9239, 0x3f);
    OV5642YUV_write_cmos_sensor(0x923a, 0x0d);
    OV5642YUV_write_cmos_sensor(0x923b, 0xe0);
    OV5642YUV_write_cmos_sensor(0x923c, 0xf5);
    OV5642YUV_write_cmos_sensor(0x923d, 0x28);
    OV5642YUV_write_cmos_sensor(0x923e, 0xe5);
    OV5642YUV_write_cmos_sensor(0x923f, 0x28);
    OV5642YUV_write_cmos_sensor(0x9240, 0xf0);
    OV5642YUV_write_cmos_sensor(0x9241, 0xd0);
    OV5642YUV_write_cmos_sensor(0x9242, 0x82);
    OV5642YUV_write_cmos_sensor(0x9243, 0xd0);
    OV5642YUV_write_cmos_sensor(0x9244, 0x83);
    OV5642YUV_write_cmos_sensor(0x9245, 0xd0);
    OV5642YUV_write_cmos_sensor(0x9246, 0xe0);
    OV5642YUV_write_cmos_sensor(0x9247, 0x32);
    OV5642YUV_write_cmos_sensor(0x9248, 0x75);
    OV5642YUV_write_cmos_sensor(0x9249, 0x36);
    OV5642YUV_write_cmos_sensor(0x924a, 0x18);
    OV5642YUV_write_cmos_sensor(0x924b, 0x75);
    OV5642YUV_write_cmos_sensor(0x924c, 0x24);
    OV5642YUV_write_cmos_sensor(0x924d, 0x00);
    OV5642YUV_write_cmos_sensor(0x924e, 0x75);
    OV5642YUV_write_cmos_sensor(0x924f, 0x25);
    OV5642YUV_write_cmos_sensor(0x9250, 0x01);
    OV5642YUV_write_cmos_sensor(0x9251, 0x75);
    OV5642YUV_write_cmos_sensor(0x9252, 0x37);
    OV5642YUV_write_cmos_sensor(0x9253, 0x02);
    OV5642YUV_write_cmos_sensor(0x9254, 0x22);
    OV5642YUV_write_cmos_sensor(0x9255, 0x78);
    OV5642YUV_write_cmos_sensor(0x9256, 0x7f);
    OV5642YUV_write_cmos_sensor(0x9257, 0xe4);
    OV5642YUV_write_cmos_sensor(0x9258, 0xf6);
    OV5642YUV_write_cmos_sensor(0x9259, 0xd8);
    OV5642YUV_write_cmos_sensor(0x925a, 0xfd);
    OV5642YUV_write_cmos_sensor(0x925b, 0x75);
    OV5642YUV_write_cmos_sensor(0x925c, 0x81);
    OV5642YUV_write_cmos_sensor(0x925d, 0xd0);
    OV5642YUV_write_cmos_sensor(0x925e, 0x02);
    OV5642YUV_write_cmos_sensor(0x925f, 0x0f);
    OV5642YUV_write_cmos_sensor(0x9260, 0xdc);
    OV5642YUV_write_cmos_sensor(0x9261, 0x8f);
    OV5642YUV_write_cmos_sensor(0x9262, 0x82);
    OV5642YUV_write_cmos_sensor(0x9263, 0x8e);
    OV5642YUV_write_cmos_sensor(0x9264, 0x83);
    OV5642YUV_write_cmos_sensor(0x9265, 0x75);
    OV5642YUV_write_cmos_sensor(0x9266, 0xf0);
    OV5642YUV_write_cmos_sensor(0x9267, 0x04);
    OV5642YUV_write_cmos_sensor(0x9268, 0xed);
    OV5642YUV_write_cmos_sensor(0x9269, 0x02);
    OV5642YUV_write_cmos_sensor(0x926a, 0x09);
    OV5642YUV_write_cmos_sensor(0x926b, 0xb4);
    OV5642YUV_write_cmos_sensor(0x3024, 0x00);
    OV5642YUV_write_cmos_sensor(0x3025, 0x00);
    OV5642YUV_write_cmos_sensor(0x5082, 0x00);
    OV5642YUV_write_cmos_sensor(0x5083, 0x00);
    OV5642YUV_write_cmos_sensor(0x5084, 0x00);
    OV5642YUV_write_cmos_sensor(0x5085, 0x00);
    OV5642YUV_write_cmos_sensor(0x3026, 0x00);
    OV5642YUV_write_cmos_sensor(0x3027, 0x7F);
    OV5642YUV_write_cmos_sensor(0x3000, 0x00);
#endif


    do {
        state = (UINT8)OV5642YUV_read_cmos_sensor(0x3027);
	 SENSORDB("when init af, state=0x%x\n",state);	
	 
        Sleep(1);
        if (--iteration == 0)
        {
            RETAILMSG(1, (TEXT("[OV5642]STA_FOCUS state check ERROR!!0x%x \r\n")), state);
            break;
        }
    } while(state!=0x70);

    OV5642YUV_imgSensorProfileEnd("OV5642_FOCUS_AD5820_Init");
    OV5642_FOCUS_AD5820_Check_MCU();

    if(!iteration)
    {
        AF_INIT = FALSE;
        return 1;
    }
    else
    {    
        AF_INIT = TRUE;
        return 0;
    }

}
// Step to Specified position
static void OV5642_FOCUS_AD5820_Move_to(UINT16 a_u2MovePosition)
{
    
    UINT8 state = 0x8F;
    UINT32 iteration = 100;
    static UINT16 u2CurrPos = 0;

    //SENSORDB("OV5642_FOCUS_AD5820_Move_to %d\n", a_u2MovePosition);     

	if (u2CurrPos == a_u2MovePosition)   {return;}
				
    OV5642YUV_imgSensorProfileStart();
    
    a_u2MovePosition = a_u2MovePosition&0x07FF; //10bit for AD5820

	u2CurrPos = a_u2MovePosition;
	
    do {
    	 state = (UINT8)OV5642YUV_read_cmos_sensor(0x3025);
        RETAILMSG(1, (TEXT("[OV5642]Move stage state !!0x%x \r\n")), state);
        Sleep(1);
        if (iteration-- == 0)
        {
            RETAILMSG(1, (TEXT("[OV5642]Move stage state check ERROR!!0x%x \r\n")), state);
            break;
        }

    } while(state!=0x00);
       
    OV5642YUV_write_cmos_sensor(0x3025,0x0F);
    OV5642YUV_write_cmos_sensor(0x5084,(UINT8)((u2CurrPos>>8)&0x07));
    OV5642YUV_write_cmos_sensor(0x5085,(UINT8)(u2CurrPos&0x00FF));
    OV5642YUV_write_cmos_sensor(0x3024,0x05);    
    
    OV5642YUV_imgSensorProfileEnd("OV5642_FOCUS_AD5820_Move_to");    
}

#if 0
static void OV5642_FOCUS_AD5820_Get_AF_Status(UINT32 *pFeatureReturnPara32)
{
    UINT32 state = OV5642YUV_read_cmos_sensor(0x3025);

    if (state == 0)
    {
        *pFeatureReturnPara32 = 0;
    }
    else
    {
        *pFeatureReturnPara32 = 1;
    }
}
#endif
static void OV5642_FOCUS_AD5820_Get_AF_Inf(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = 0;
}

static void OV5642_FOCUS_AD5820_Get_AF_Macro(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = 1023;
}


//update focus window
//@input zone[] addr
static void OV5642_FOCUS_AD5820_Set_AF_Window(UINT32 zone_addr)
{//update global zone

    UINT8 state = 0x8F;

    //input:
    UINT32 times = 1;
    UINT32 FD_XS = 4;
    UINT32 FD_YS = 3;	
    UINT32 x0, y0, x1, y1;
    UINT32* zone = (UINT32*)zone_addr;
    x0 = *zone;
    y0 = *(zone + 1);
    x1 = *(zone + 2);
    y1 = *(zone + 3);	
    FD_XS = *(zone + 4);
    FD_YS = *(zone + 5);
    times = FD_XS / AF_XS;
    SENSORDB("x0=%d,y0=%d,x1=%d,y1=%d,FD_XS=%d,FD_YS=%d,times=%d\n",
		x0, y0, x1, y1, FD_XS, FD_YS,times);	

//zone changed, update global zone.
    zone_x0 = (UINT8)(x0 / times);   
    zone_y0 = (UINT8)(y0 / times); 
    zone_x1 = (UINT8)(x1 / times);  
    zone_y1 = (UINT8)(y1 / times);  
    SENSORDB("zone_x0=%d,zone_y0=%d,zone_x1=%d,zone_y1=%d,FD_XS=%d,FD_YS=%d\n",
        zone_x0, zone_y0, zone_x1, zone_y1, FD_XS, FD_YS);  


}


//set focus zone coordinates to sensor IC
//global zone[]
static void OV5642_FOCUS_AD5820_Set_AF_Window_to_IC(void)
{
    UINT8 x_center, y_center, width, height;  
    UINT32 iteration = 100;  
    UINT8 cmd_ack = 0x8F;
    UINT8 cmd_reg = 0x8F;
	
    x_center = (zone_x0 + zone_x1) / 2;	
    y_center = (zone_x1 + zone_y1) / 2;	
    width = zone_x1 - zone_x0;
    height = zone_y1 - zone_y0;

    SENSORDB("x_center = %d, y_center = %d,width = %d, height = %d\n",
        x_center,
        y_center,
        width,
        height);
    
//use toch mode focus zones(0x81)
//to confige the single zone(Xc, Yc,)

    OV5642YUV_write_cmos_sensor(0x5082,x_center);//x_center
    OV5642YUV_write_cmos_sensor(0x5083,y_center);//y_center
    
    OV5642YUV_write_cmos_sensor(0x3025,0x01);//cmd_tag
    OV5642YUV_write_cmos_sensor(0x3024,0x81);//cmd_main
    iteration = 100;  
    do{
        cmd_ack = (UINT8)OV5642YUV_read_cmos_sensor(0x3025);
        cmd_reg = (UINT8)OV5642YUV_read_cmos_sensor(0x3024);
        SENSORDB("0x81 CMD_TAG= 0x%0x\n",cmd_ack);
        SENSORDB("0x81 CMD_REG= 0x%0x\n",cmd_reg);
        Sleep(1);
        if(iteration-- == 0)
        {
            SENSORDB("0x81 wait CMD_TAG zero out of time!!!! %x\n",cmd_ack);
            break;
        }

    }while(cmd_ack != 0x00);

    return;

}


//update zone[]
static void OV5642_FOCUS_AD5820_Update_Zone(void)
{
    UINT8 state = 0x8F;    
    UINT32 iteration = 100;  
    UINT8 cmd_reg = 0x8F;
    UINT8 cmd_tag = 0x8F;
    //send update zone cmd to firmware
    OV5642YUV_write_cmos_sensor(0x3025,0x01);//cmd_tag
    OV5642YUV_write_cmos_sensor(0x3024,0x12);//cmd_reg

    do{
        cmd_tag = (UINT8)OV5642YUV_read_cmos_sensor(0x3025);
        cmd_reg = (UINT8)OV5642YUV_read_cmos_sensor(0x3024);
        SENSORDB("0x12 CMD_TAG= 0x%0x\n",cmd_tag);
        SENSORDB("0x12 CMD_REG= 0x%0x\n",cmd_reg);
        Sleep(1);
        if(iteration-- == 0)
        {
            SENSORDB("0x12 wait CMD_TAG zero out of time!!!! %x\n",cmd_tag);
            break;
        }

    }while(cmd_tag != 0x00);
    return;

}



//set constant focus
static void OV5642_FOCUS_AD5820_Constant_Focus(void)
{
    UINT8 state = 0x8F;
    UINT32 iteration = 20;
    UINT8 cmd_tag = 0x8F;
    UINT8 cmd_reg = 0x8F; 
    //send constant focus mode command to firmware
    OV5642YUV_write_cmos_sensor(0x3025,0x01);
    OV5642YUV_write_cmos_sensor(0x3024,0x04);

#if 0
    do{
        
        cmd_tag = OV5642YUV_read_cmos_sensor(0x3025);
        cmd_reg = OV5642YUV_read_cmos_sensor(0x3024);
        SENSORDB("OV5642_FOCUS_AD5820_Constant_Focus cmd_reg = 0x%d, cmd_tag = 0x%d\n",
            cmd_reg,
            cmd_tag);
        if(cmd_tag == 0x00)
        {
            //SENSORDB("OV5642_FOCUS_AD5820_Constant_Focus cmd_tag =0\n");
            return;
        }	
        Sleep(1);
        iteration --;
    }while(iteration);
    
    SENSORDB("OV5642_FOCUS_AD5820_Constant_Focus cmd_tag time out II\r\n");
    return ;
#endif

}
static void OV5642_FOCUS_AD5820_Single_Focus()
{
     SENSORDB("OV5642_FOCUS_AD5820_Single_Focus\n");

    UINT8 state = 0x8F;
    UINT8 cmd_ack = 0x8F;	
    UINT8 cmd_reg = 0x8F;		
    UINT32 iteration = 300;
    SENSORDB("Update_Zone 1 \n");

    OV5642_FOCUS_AD5820_Update_Zone();

// 1  .custom single zone mode    
    OV5642_FOCUS_AD5820_Set_AF_Window_to_IC();

    SENSORDB("Update_Zone 2 \n");
    OV5642_FOCUS_AD5820_Update_Zone();

// 2. trig single auto focus
   OV5642YUV_write_cmos_sensor(0x3025,0x01);//cmd_tag
   OV5642YUV_write_cmos_sensor(0x3024,0x03);//cmd_main
   #if 0
   iteration = 100;  
   do{
       cmd_ack = (UINT8)OV5642YUV_read_cmos_sensor(0x3025);
       cmd_reg = (UINT8)OV5642YUV_read_cmos_sensor(0x3024);
       SENSORDB("0x03 CMD_TAG= 0x%0x\n",cmd_ack);
       SENSORDB("0x03 CMD_REG= 0x%0x\n",cmd_reg);
       Sleep(1);
       if(iteration-- == 0)
       {
           SENSORDB("0x03 wait CMD_TAG zero out of time!!!! %x\n",cmd_ack);
           break;
       }
   
   }while(cmd_ack != 0x00);
#endif


}


static void OV5642_FOCUS_AD5820_Cancel_Focus()
{

     SENSORDB("OV5642_FOCUS_AD5820_Cancel_Focus\n");

    UINT8 state = 0x8F;
    UINT8 state_ack = 0x8F;	
    UINT8 state_cmd = 0x8F;		
    UINT32 iteration = 300;

    //send idle command to firmware
    OV5642YUV_write_cmos_sensor(0x3025,0x01);
    OV5642YUV_write_cmos_sensor(0x3024,0x08);

    iteration = 100;  
    do{
        state = (UINT8)OV5642YUV_read_cmos_sensor(0x3027);
        state_ack = 	(UINT8)OV5642YUV_read_cmos_sensor(0x3025);	
        state_cmd = 	(UINT8)OV5642YUV_read_cmos_sensor(0x3024);		
        SENSORDB("cancel focus state = 0x%x,state_ack=0x%x,state_cmd=0x%x\n",state,state_ack,state_cmd);
        
        if(state == 0x70)
        {
            SENSORDB("idle!\n");
            break;
        }			
        Sleep(1);
        iteration --;

    }while(iteration);

}


static void OV5642_FOCUS_AD5820_Get_AF_Status(UINT32 *pFeatureReturnPara32)
{
	
// 1 Hal3AYuv::getAFDone() query
// 2 draw focus rectangle query
    UINT32 state = OV5642YUV_read_cmos_sensor(0x3027);

    SENSORDB("OV5642_FOCUS_AD5820_Get_AF_Status=0x%x\n", state);

    if(0x70 == state)
    {
        *pFeatureReturnPara32 = SENSOR_AF_IDLE;
	
    }
    else if(0x00 == state)
    {
        *pFeatureReturnPara32 = SENSOR_AF_FOCUSING;
		
    }
    else if(0x10 == state)
    {
        *pFeatureReturnPara32 = SENSOR_AF_FOCUSED;
			
    }
    else if(0x20 == state)
    {
        *pFeatureReturnPara32 = SENSOR_AF_SCENE_DETECTING;
			
    }    
    else
    {
        *pFeatureReturnPara32 = SENSOR_AF_ERROR;
			   
    }

     SENSORDB("OV5642_FOCUS_AD5820_Get_AF_Status return %d\n", *pFeatureReturnPara32);

}


void OV5642YUV_Set_Mirror_Flip(kal_uint8 image_mirror)
{
	kal_uint8 iTemp = 0;

	//MOD1[0x03],
	//Sensor mirror image control, bit[1]
	//Sensor flip image control, bit[0];
	iTemp = OV5642YUV_read_cmos_sensor(0x3818) & 0x9f;	//Clear the mirror and flip bits.
    switch (image_mirror)
	{
	    case IMAGE_NORMAL:
	        OV5642YUV_write_cmos_sensor(0x3818, iTemp | 0xc0);	//Set normal
	         OV5642YUV_write_cmos_sensor(0x3621, 0xc7);
			
                //SET_FIRST_GRAB_COLOR(BAYER_Gr);
	        break;

		case IMAGE_V_MIRROR:
			OV5642YUV_write_cmos_sensor(0x3818, iTemp | 0xe0);	//Set flip
			OV5642YUV_write_cmos_sensor(0x3621, 0xc7);
			
                //SET_FIRST_GRAB_COLOR(BAYER_B);
			break;
			
		case IMAGE_H_MIRROR:
			OV5642YUV_write_cmos_sensor(0x3818, iTemp | 0x80);	//Set mirror
			OV5642YUV_write_cmos_sensor(0x3621, 0xe7);
                //SET_FIRST_GRAB_COLOR(BAYER_Gr);
			break;
				
	    case IMAGE_HV_MIRROR:
	        OV5642YUV_write_cmos_sensor(0x3818, iTemp | 0xa0);	//Set mirror and flip
	        OV5642YUV_write_cmos_sensor(0x3621, 0xe7);
                //SET_FIRST_GRAB_COLOR(BAYER_B);
	        break;
    }
}


/*************************************************************************
* FUNCTION
*   OV5642YUVPreview
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
UINT32 OV5642YUVPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("OV5642YUVPreview E\n");

    kal_uint16 iStartX = 0, iStartY = 0;

    g_iOV5642YUV_Mode = OV5642_MODE_PREVIEW;

    //if(OV5642_720P == OV5642YUV_g_RES)
    {
        OV5642YUV_set_720P();
    }

    if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        OV5642YUV_MPEG4_encode_mode = KAL_TRUE;
    }
    else
    {
        OV5642YUV_MPEG4_encode_mode = KAL_FALSE;
    }

    iStartX += OV5642_IMAGE_SENSOR_PV_STARTX;
    iStartY += OV5642_IMAGE_SENSOR_PV_STARTY;

    //sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
    
    //OV5642YUV_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

#if 0    
    iTemp = OV5642YUV_read_cmos_sensor(0x3818) & 0x9f;	//Clear the mirror and flip bits.
    switch (sensor_config_data->SensorImageMirror)
    {
        case IMAGE_NORMAL:
            OV5642YUV_write_cmos_sensor(0x3818, iTemp | 0xc0);	//Set normal
            OV5642YUV_write_cmos_sensor(0x3621, 0xc7);
            //SET_FIRST_GRAB_COLOR(BAYER_Gr);
            break;

        case IMAGE_V_MIRROR:
            OV5642YUV_write_cmos_sensor(0x3818, iTemp | 0xe0);	//Set flip
            OV5642YUV_write_cmos_sensor(0x3621, 0xc7);

            //SET_FIRST_GRAB_COLOR(BAYER_B);
            break;

        case IMAGE_H_MIRROR:
            OV5642YUV_write_cmos_sensor(0x3818, iTemp | 0x80);	//Set mirror
            OV5642YUV_write_cmos_sensor(0x3621, 0xe7);
            //SET_FIRST_GRAB_COLOR(BAYER_Gr);
        break;

        case IMAGE_HV_MIRROR:
            OV5642YUV_write_cmos_sensor(0x3818, iTemp | 0xa0);	//Set mirror and flip
            OV5642YUV_write_cmos_sensor(0x3621, 0xe7);
            //SET_FIRST_GRAB_COLOR(BAYER_B);
            break;
    }
#endif     

    OV5642YUV_dummy_pixels = 0;
    OV5642YUV_dummy_lines = 0;
    OV5642YUV_PV_dummy_pixels = OV5642YUV_dummy_pixels;
    OV5642YUV_PV_dummy_lines = OV5642YUV_dummy_lines;

    OV5642YUV_SetDummy(OV5642YUV_dummy_pixels, OV5642YUV_dummy_lines);
    //OV5642YUV_SetShutter(OV5642YUV_pv_exposure_lines);

    memcpy(&OV5642YUVSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    image_window->GrabStartX= iStartX;
    image_window->GrabStartY= iStartY;
    image_window->ExposureWindowWidth= OV5642_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
    image_window->ExposureWindowHeight= OV5642_IMAGE_SENSOR_PV_HEIGHT - 2*iStartY;

    SENSORDB("OV5642YUVPreview X\n");
    return ERROR_NONE;
}	/* OV5642YUVPreview() */


UINT32 OV5642YUVCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    //kal_uint32 shutter=OV5642YUV_pv_exposure_lines;
    kal_uint32 shutter = 0;
    int temp_shutter = 0;	
    kal_uint16 iStartX = 0, iStartY = 0;
    kal_uint32 pv_gain = 0;	
    kal_uint8 temp = 0;//for night mode	

    SENSORDB("OV5642YUVCapture E\n");

    SENSORDB("Preview Shutter = %d, Gain = %d\n", shutter, read_OV5642YUV_gain());    

    g_iOV5642YUV_Mode = OV5642_MODE_CAPTURE;

    if(sensor_config_data->EnableShutterTansfer==KAL_TRUE)
        shutter=sensor_config_data->CaptureShutter;


//1. disable night mode 	to do...
 //   temp= OV5642YUV_read_cmos_sensor(0x3a00);
//    OV5642YUV_write_cmos_sensor(0x3a00,temp&0xfb);
//2. disable AE,AWB
    //OV5642YUV_set_AE_mode(KAL_FALSE);
    //OV5642YUV_set_AWB_mode(KAL_FALSE);

//3. read shutter, gain
//    shutter = OV5642YUV_read_shutter();
 //   pv_gain = read_OV5642YUV_gain();
//4. set 5M mode

    if ((image_window->ImageTargetWidth<= OV5642_IMAGE_SENSOR_PV_WIDTH) &&
        (image_window->ImageTargetHeight<= OV5642_IMAGE_SENSOR_PV_HEIGHT)) {
        OV5642YUV_dummy_pixels= 0;
        OV5642YUV_dummy_lines = 0;

        shutter = ((UINT32)(shutter*(OV5642_IMAGE_SENSOR_720P_PIXELS_LINE + OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS + OV5642YUV_PV_dummy_pixels)))/
                                                        ((OV5642_IMAGE_SENSOR_5M_PIXELS_LINE+ OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS + OV5642YUV_dummy_pixels)) ;
        shutter = shutter * OV5642YUV_CAP_pclk / OV5642YUV_PV_pclk;         
        iStartX = OV5642_IMAGE_SENSOR_PV_STARTX;
        iStartY = OV5642_IMAGE_SENSOR_PV_STARTY;
        image_window->GrabStartX=iStartX;
        image_window->GrabStartY=iStartY;
        image_window->ExposureWindowWidth=OV5642_IMAGE_SENSOR_PV_WIDTH - 2*iStartX;
        image_window->ExposureWindowHeight=OV5642_IMAGE_SENSOR_PV_HEIGHT- 2*iStartY;
    }
    else { // 5M  Mode
        OV5642YUV_dummy_pixels= 0;
        OV5642YUV_dummy_lines = 0;        
        OV5642YUV_set_5M();
        //OV5642YUV_dump_5M();
        //sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
     
        //OV5642SetFlipMirror(sensor_config_data->SensorImageMirror); 
        OV5642YUV_write_cmos_sensor(0x3818,0xa0);
        OV5642YUV_write_cmos_sensor(0x3621,0x29);        

        OV5642YUV_CAP_pclk = 10075;
        OV5642YUV_PV_pclk = 10183;		
        temp_shutter = (shutter*(OV5642_PV_PERIOD_PIXEL_NUMS+OV5642YUV_PV_dummy_pixels)*OV5642YUV_CAP_pclk)/(OV5642_FULL_PERIOD_PIXEL_NUMS+OV5642YUV_dummy_pixels)/OV5642YUV_PV_pclk;	
        shutter = (kal_uint32)(temp_shutter);
        SENSORDB("cap shutter calutaed = %d, 0x%x\n", shutter,shutter);
		//shutter = shutter*2; 
        //SVGA Internal CLK = 1/4 UXGA Internal CLK
        //shutter = 4* shutter;
       // shutter = ((UINT32)(shutter*(OV5642_IMAGE_SENSOR_720P_PIXELS_LINE + OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS + OV5642YUV_PV_dummy_pixels)))/
        //                                                ((OV5642_IMAGE_SENSOR_5M_PIXELS_LINE+ OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS + OV5642YUV_dummy_pixels)) ;
        //shutter = shutter * OV5642YUV_CAP_pclk / OV5642YUV_PV_pclk; 
        iStartX = 2* OV5642_IMAGE_SENSOR_PV_STARTX;
        iStartY = 2* OV5642_IMAGE_SENSOR_PV_STARTY;

        image_window->GrabStartX=iStartX;
        image_window->GrabStartY=iStartY;
        image_window->ExposureWindowWidth=OV5642_IMAGE_SENSOR_FULL_WIDTH -2*iStartX;
        image_window->ExposureWindowHeight=OV5642_IMAGE_SENSOR_FULL_HEIGHT-2*iStartY;
    }//5M Capture
    // config flashlight preview setting
    if(OV5642_5M == OV5642YUV_g_RES) //add start
    {
        sensor_config_data->DefaultPclk = 32500000;
        sensor_config_data->Pixels = OV5642_IMAGE_SENSOR_5M_PIXELS_LINE + OV5642YUV_PV_dummy_pixels;
        sensor_config_data->FrameLines =OV5642_PV_PERIOD_LINE_NUMS+OV5642YUV_PV_dummy_lines;
    }
    else
    {
        sensor_config_data->DefaultPclk = 32500000;
        sensor_config_data->Pixels = OV5642_IMAGE_SENSOR_5M_PIXELS_LINE+OV5642YUV_dummy_pixels;
        sensor_config_data->FrameLines =OV5642_FULL_PERIOD_LINE_NUMS+OV5642YUV_dummy_lines;
    }

 
	
    sensor_config_data->Lines = image_window->ExposureWindowHeight;
    sensor_config_data->Shutter =shutter;

    //OV5642YUV_SetDummy(OV5642YUV_dummy_pixels, OV5642YUV_dummy_lines);
    //OV5642YUV_SetDummy(OV5642YUV_dummy_pixels, OV5642YUV_dummy_lines);

//6.set shutter
	
    //OV5642YUV_SetShutter(shutter);
    //OV5642YUV_read_shutter();
    
//7.set gain
    //write_OV5642YUV_gain(pv_gain);	
//aec, awb is close?
    SENSORDB("aec reg0x3503=0x%x, awb reg0x3406=0x%x\n",
        OV5642YUV_read_cmos_sensor(0x3503),
        OV5642YUV_read_cmos_sensor(0x3406));
    SENSORDB("Capture Shutter = %d, Gain = %d\n", shutter, read_OV5642YUV_gain());     
    memcpy(&OV5642YUVSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    SENSORDB("OV5642YUVCapture X\n");

    return ERROR_NONE;
}	/* OV5642YUVCapture() */

UINT32 OV5642YUVGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH - 4*OV5642_IMAGE_SENSOR_PV_STARTX;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT - 4*OV5642_IMAGE_SENSOR_PV_STARTY;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH - 2*OV5642_IMAGE_SENSOR_PV_STARTX;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT - 2*OV5642_IMAGE_SENSOR_PV_STARTY;

    return ERROR_NONE;
}   /* OV5642YUVGetResolution() */
UINT32 OV5642YUVGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
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
//    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_UYVY;
	
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;


    //pSensorInfo->CaptureDelayFrame = 1; 
    pSensorInfo->CaptureDelayFrame = 2; 
    pSensorInfo->PreviewDelayFrame = 2; 
    pSensorInfo->VideoDelayFrame = 5; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;      

//
    pSensorInfo->AEShutDelayFrame = 0;		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 1;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 1;	
	   
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        //case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
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
        //case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
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

    OV5642YUVPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &OV5642YUVSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* OV5642YUVGetInfo() */


UINT32 OV5642YUVControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        //case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            OV5642YUVPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        //case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
            OV5642YUVCapture(pImageWindow, pSensorConfigData);
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
} /* OV5642YUVControl() */


UINT32 OV5642YUVSetVideoMode(UINT16 u2FrameRate)
{
//    SENSORDB("[OV5642YUVSetVideoMode] frame rate = %d\n", u2FrameRate);
    OV5642YUV_MPEG4_encode_mode = KAL_TRUE; 
    if (u2FrameRate == 30)
    {
            //OV5642YUV_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642YUV_sensor_pclk/30)/(OV5642_PV_PERIOD_PIXEL_NUMS + OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS + OV5642YUV_PV_dummy_pixels));
            OV5642YUV_MAX_EXPOSURE_LINES = OV5642_PV_PERIOD_LINE_NUMS + OV5642_PV_PERIOD_EXTRA_LINE_NUMS; 
            if(OV5642YUV_pv_exposure_lines < (OV5642YUV_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP)) // for avoid the shutter > frame_lines,move the frame lines setting to shutter function
            {
                OV5642YUV_write_cmos_sensor(0x350C, (OV5642YUV_MAX_EXPOSURE_LINES >> 8) & 0xFF);
                OV5642YUV_write_cmos_sensor(0x350D, OV5642YUV_MAX_EXPOSURE_LINES & 0xFF);
                OV5642YUV_CURRENT_FRAME_LINES = OV5642YUV_MAX_EXPOSURE_LINES;
            }
            //OV5642YUV_MAX_EXPOSURE_LINES = OV5642YUV_MAX_EXPOSURE_LINES - OV5642_SHUTTER_LINES_GAP;
    }
    else if (u2FrameRate == 15)       
    {
            //OV5642YUV_MAX_EXPOSURE_LINES = (kal_uint16)((OV5642YUV_sensor_pclk/15)/(OV5642_PV_PERIOD_PIXEL_NUMS + OV5642_FULL_PERIOD_EXTRA_PIXEL_NUMS + OV5642YUV_PV_dummy_pixels));
            OV5642YUV_MAX_EXPOSURE_LINES = (OV5642_PV_PERIOD_LINE_NUMS + OV5642_PV_PERIOD_EXTRA_LINE_NUMS) * 2;  
            OV5642YUV_write_cmos_sensor(0x350C, (OV5642YUV_MAX_EXPOSURE_LINES >> 8) & 0xFF);
            OV5642YUV_write_cmos_sensor(0x350D, OV5642YUV_MAX_EXPOSURE_LINES & 0xFF);
            OV5642YUV_CURRENT_FRAME_LINES = OV5642YUV_MAX_EXPOSURE_LINES;
            OV5642YUV_MAX_EXPOSURE_LINES = OV5642YUV_CURRENT_FRAME_LINES - OV5642_SHUTTER_LINES_GAP;
    }
    else 
    {
        SENSORDB("Wrong frame rate setting \n");
    }
    return TRUE;
}

UINT32 OV5642YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
    //SENSORDB("cmd=%d, para = 0x%x\n", iCmd, iPara);
    if(FID_ZOOM_FACTOR == iCmd)
    {

        SENSORDB("got zoom value = 0x%x\n", iPara);
        OV5642YUV_zoom_factor = iPara; 		
	
    }

    return TRUE;

}

/*************************************************************************
* FUNCTION
*   OV5642YUVGetSensorID
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
UINT32 OV5642YUVGetSensorID(UINT32 *sensorID) 
{
    int  retry = 3; 
    
    // check if sensor ID correct
    do {
        *sensorID = ((OV5642YUV_read_cmos_sensor(0x300A) << 8) | OV5642YUV_read_cmos_sensor(0x300B));        
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


static void OV5642_FOCUS_AD5820_Get_AF_Max_Num_Focus_Areas(UINT32 *pFeatureReturnPara32)
{ 	
    
    *pFeatureReturnPara32 = 1;    
    SENSORDB(" *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);	
}

static void OV5642_FOCUS_AD5820_Get_AE_Max_Num_Metering_Areas(UINT32 *pFeatureReturnPara32)
{ 	
    
    *pFeatureReturnPara32 = 0;    
    SENSORDB(" *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);	
}



UINT32 OV5642YUVFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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

#if WINMO_USE
        case SENSOR_FEATURE_GET_INIT_OPERATION_PARA:
        {
            PCAMERA_DRIVER_OPERATION_PARA_STRUCT pSensorOperData;
            pSensorOperData = (PCAMERA_DRIVER_OPERATION_PARA_STRUCT)pFeaturePara;

            pSensorOperData->CaptureDelayFrame = 2;         /* wait stable frame when sensor change mode (pre to cap) */
            pSensorOperData->PreviewDelayFrame = 3;         /* wait stable frame when sensor change mode (cap to pre) */
            pSensorOperData->PreviewDisplayWaitFrame = 2;   /* wait stable frame when sensor change mode (cap to pre) */
            pSensorOperData->AECalDelayFrame = 0;               /* The frame of calculation default 0 */
            pSensorOperData->AEShutDelayFrame = 0;              /* The frame of setting shutter default 0 for TG int */
            pSensorOperData->AESensorGainDelayFrame = 1;    /* The frame of setting sensor gain */
            pSensorOperData->AEISPGainDelayFrame = 2;          /* The frame of setting gain */
            pSensorOperData->AECalPeriod = 3;                           /* AE AWB calculation period */
            pSensorOperData->FlashlightMode=FLASHLIGHT_LED_CONSTANT;

            break;
        }
#endif 

        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
            *pFeatureReturnPara16++=OV5642_PV_PERIOD_EXTRA_PIXEL_NUMS + OV5642_PV_PERIOD_PIXEL_NUMS + OV5642YUV_dummy_pixels;//OV5642_PV_PERIOD_PIXEL_NUMS+OV5642YUV_dummy_pixels;
            *pFeatureReturnPara16=OV5642_PV_PERIOD_LINE_NUMS+OV5642YUV_dummy_lines;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            *pFeatureReturnPara32 = 55250000; //19500000;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            OV5642YUV_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            OV5642YUV_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            OV5642YUV_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            OV5642YUV_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV5642YUV_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = OV5642YUV_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
                OV5642YUVSensorCCT[i].Addr=*pFeatureData32++;
                OV5642YUVSensorCCT[i].Para=*pFeatureData32++;
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV5642YUVSensorCCT[i].Addr;
                *pFeatureData32++=OV5642YUVSensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
                OV5642YUVSensorReg[i].Addr=*pFeatureData32++;
                OV5642YUVSensorReg[i].Para=*pFeatureData32++;
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=OV5642YUVSensorReg[i].Addr;
                *pFeatureData32++=OV5642YUVSensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=OV5642_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, OV5642YUVSensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, OV5642YUVSensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &OV5642YUVSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            OV5642YUV_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV5642YUV_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=OV5642YUV_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV5642YUV_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV5642YUV_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV5642YUV_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 129;
            pSensorEngInfo->SensorType = CMOS_SENSOR;
			//test by lingnan
            //pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B;
            pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;

        case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
            OV5642_FOCUS_AD5820_Get_AF_Max_Num_Focus_Areas(pFeatureReturnPara32);            
            *pFeatureParaLen=4;
            break;        
        case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
            OV5642_FOCUS_AD5820_Get_AE_Max_Num_Metering_Areas(pFeatureReturnPara32);            
            *pFeatureParaLen=4;
            break;      

        case SENSOR_FEATURE_INITIALIZE_AF:
            //as ov3640, OV5642_FOCUS_AD5820_Init() may fail, 0x3027 = 0x7F
            //MCU is off, or firmware is not correct.
            //move to the end of OV5642YUV_Sensor_Init();
            //SENSORDB("OV5642_FOCUS_AD5820_Init\n");
            //OV5642_FOCUS_AD5820_Init();
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            SENSORDB("1 OV5642_FOCUS_AD5820_Constant_Focus\n");
            OV5642_FOCUS_AD5820_Constant_Focus();
            SENSORDB("2 OV5642_FOCUS_AD5820_Constant_Focus\n");
            OV5642_FOCUS_AD5820_Constant_Focus();
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            //SENSORDB("OV5642_FOCUS_AD5820_Move_to %d\n", *pFeatureData16);
            OV5642_FOCUS_AD5820_Move_to(*pFeatureData16);
            break;
        case SENSOR_FEATURE_GET_AF_STATUS:
            //for yuv use:
            //SENSORDB("SENSOR_FEATURE_GET_AF_STATUS pFeatureReturnPara32=0x%x\n",pFeatureReturnPara32);
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
            OV5642YUVSetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_YUV_CMD:
            OV5642YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));	
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV5642YUVGetSensorID(pFeatureReturnPara32); 
            break; 				            
        case SENSOR_FEATURE_SINGLE_FOCUS_MODE:
            //SENSORDB("SENSOR_FEATURE_SINGLE_FOCUS_MODE\n");
            OV5642_FOCUS_AD5820_Single_Focus();
            break;		
        case SENSOR_FEATURE_CANCEL_AF:
            //SENSORDB("SENSOR_FEATURE_CANCEL_AF\n");
            OV5642_FOCUS_AD5820_Cancel_Focus();
            break;			
        case SENSOR_FEATURE_SET_AF_WINDOW:
            //SENSORDB("SENSOR_FEATURE_SET_AF_WINDOW\n");
            //SENSORDB("get zone addr = 0x%x\n",*pFeatureData32);			
            OV5642_FOCUS_AD5820_Set_AF_Window(*pFeatureData32);
            break;	
		
        default:
            break;
    }
    return ERROR_NONE;
}	/* OV5642YUVFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncOV5642YUV=
{
    OV5642YUVOpen,
    OV5642YUVGetInfo,
    OV5642YUVGetResolution,
    OV5642YUVFeatureControl,
    OV5642YUVControl,
    OV5642YUVClose
};

UINT32 OV5642_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncOV5642YUV;

    return ERROR_NONE;
}   /* OV5642_YUV_SensorInit() */

