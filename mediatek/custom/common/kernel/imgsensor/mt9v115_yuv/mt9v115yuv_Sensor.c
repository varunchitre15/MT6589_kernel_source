/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.c
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author:
 * -------
 *   PC Huang (MTK02204)
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
#include <asm/atomic.h>
#include <asm/system.h>
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"



#include "mt9v115yuv_Sensor.h"
#include "mt9v115yuv_Camera_Sensor_para.h"
#include "mt9v115yuv_CameraCustomized.h"

#define MT9V115YUV_DEBUG
#ifdef MT9V115YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

static DEFINE_SPINLOCK(mt9v115yuv_drv_lock);


extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 MT9V115_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
#if 1
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd,4,MT9V115_WRITE_ID);
#else
	iWriteReg((u16)addr,para,2,MT9V115_WRITE_ID);
#endif

}

kal_uint16 MT9V115_write8_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char puSendCmd[3]={(char)(addr>>8),(char)(addr & 0xFF),(char)(para & 0xFF)};
	iWriteRegI2C(puSendCmd,3,MT9V115_WRITE_ID);
}

kal_uint16 MT9V115_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,MT9V115_WRITE_ID);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}


/*******************************************************************************
* // Adapter for Winmo typedef 
********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
MSDK_SENSOR_CONFIG_STRUCT MT9V115SensorConfigData;

static struct
{
  kal_uint16 sensor_id;
  
  kal_uint32 Digital_Zoom_Factor;
} MT9V115Sensor;


UINT32 MT9V115GetSensorID(UINT32 *sensorID) 
{
    int  retry = 3; 
	UINT32 res=0;
	
    SENSORDB("Get sensor id start!\n");
    // check if sensor ID correct
    do {
        *sensorID = (MT9V115_read_cmos_sensor(MT9V115_ID_REG));        
        if (*sensorID == MT9V115_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != MT9V115_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }


	SENSORDB("Get sensor id sccuss!\n");
    return ERROR_NONE;
}







void MT9V115InitialSetting(void)
{
//MT9V115_write_cmos_sensor(0x001A,0x0106); // RESET_AND_MISC_CONTROL
/*
MT9V115_write_cmos_sensor(0x001A,0x0107); // RESET_AND_MISC_CONTROL
Sleep(1);
MT9V115_write_cmos_sensor(0x001A,0x0106); // RESET_AND_MISC_CONTROL

*/
//Step2-PLL_Timing
MT9V115_write_cmos_sensor(0x0010,0x0216); // PLL_DIVIDERS
MT9V115_write_cmos_sensor(0x0012,0x0200); // PLL_P_DIVIDERS
MT9V115_write_cmos_sensor(0x0018,0x0006); // STANDBY_CONTROL_AND_STATUS
// POLL STANDBY_CONTROL_AND_STATUS::FW_IN_STANDBY=> 0x01, ..., 0x00 (3 reads)

Sleep(10);  //DELAY=10----add xiaohuan
MT9V115_write_cmos_sensor(0x0014,0x2047); // PLL_CONTROL
MT9V115_write_cmos_sensor(0x0012,0x0200); // PLL_P_DIVIDERS
MT9V115_write_cmos_sensor(0x0014,0x2046); // PLL_CONTROL
Sleep(10);  //DELAY=10----add xiaohuan

//MT9V115_write_cmos_sensor(0x001E,0x0107); // PAD_SLEW
//MT9V115_write_cmos_sensor(0x001E,0x0557); // PAD_SLEW
MT9V115_write_cmos_sensor(0x001E,0x0557); 
MT9V115_write_cmos_sensor(0x300A,0x01F9); //frame_length_lines=505
MT9V115_write_cmos_sensor(0x300C,0x02D6); //line_length_pck=726
MT9V115_write_cmos_sensor(0x3010,0x0012); //fine_correction=18
MT9V115_write_cmos_sensor(0x3040,0x0041); //read_mode=65
MT9V115_write8_cmos_sensor(0x9803,0x07);   //stat_fd_zone_height=7
MT9V115_write_cmos_sensor(0xA06E,0x0098); //cam_fd_config_fdperiod_50hz=152
MT9V115_write_cmos_sensor(0xA070,0x007E); //cam_fd_config_fdperiod_60hz=126
MT9V115_write8_cmos_sensor(0xA072,0x11); //cam_fd_config_search_f1_50=17
MT9V115_write8_cmos_sensor(0xA073,0x13); //cam_fd_config_search_f2_50=19
MT9V115_write8_cmos_sensor(0xA074,0x14); //cam_fd_config_search_f1_60=20
MT9V115_write8_cmos_sensor(0xA075,0x16); //cam_fd_config_search_f2_60=22
MT9V115_write_cmos_sensor(0xA076,0x0003); //cam_fd_config_max_fdzone_50hz=3
MT9V115_write_cmos_sensor(0xA078,0x0004); //cam_fd_config_max_fdzone_60hz=4
MT9V115_write_cmos_sensor(0xA01A,0x0003); //cam_ae_config_target_fdzone=3

//Step3-Recommended
//Char_settings
MT9V115_write_cmos_sensor(0x3168,0x84F8);   // DAC_ECL_VAALO
MT9V115_write_cmos_sensor(0x316A,0x828C);   // DAC_RSTLO
MT9V115_write_cmos_sensor(0x316C,0xB477);   // DAC_TXLO
MT9V115_write_cmos_sensor(0x316E,0x828A); // DAC_ECL
MT9V115_write_cmos_sensor(0x3180,0x87FF); // DELTA_DK_CONTROL
MT9V115_write_cmos_sensor(0x3E02,0x0600); // SAMP_RSTX1
MT9V115_write_cmos_sensor(0x3E04,0x221C); // SAMP_RSTX2
MT9V115_write_cmos_sensor(0x3E06,0x3632); // SAMP_RSTX3
MT9V115_write_cmos_sensor(0x3E08,0x3204); // SAMP_VLN_HOLD
MT9V115_write_cmos_sensor(0x3E0A,0x3106); // SAMP_SAMP_EN
MT9V115_write_cmos_sensor(0x3E0C,0x3025); // SAMP_SAMP_SIG
MT9V115_write_cmos_sensor(0x3E0E,0x190B); // SAMP_SAMP_RST
MT9V115_write_cmos_sensor(0x3E10,0x0700); // SAMP_COL_PUP
MT9V115_write_cmos_sensor(0x3E12,0x24FF); // SAMP_COL_PDN1
MT9V115_write_cmos_sensor(0x3E14,0x3731); // SAMP_COL_PDN2
MT9V115_write_cmos_sensor(0x3E16,0x0401); // SAMP_BOOST1_EN
MT9V115_write_cmos_sensor(0x3E18,0x211E); // SAMP_BOOST2_EN
MT9V115_write_cmos_sensor(0x3E1A,0x3633); // SAMP_BOOST3_EN
MT9V115_write_cmos_sensor(0x3E1C,0x3107); // SAMP_BOOST_MUX
MT9V115_write_cmos_sensor(0x3E1E,0x1A16); // SAMP_BOOST1_HLT
MT9V115_write_cmos_sensor(0x3E20,0x312D); // SAMP_BOOST2_HLT
MT9V115_write_cmos_sensor(0x3E22,0x3303); // SAMP_BOOST_ROW
MT9V115_write_cmos_sensor(0x3E24,0x1401); // SAMP_SH_VCL
MT9V115_write_cmos_sensor(0x3E26,0x0600); // SAMP_SPARE
MT9V115_write_cmos_sensor(0x3E30,0x0037); // SAMP_READOUT
MT9V115_write_cmos_sensor(0x3E32,0x1638); // SAMP_RESET_DONE
MT9V115_write_cmos_sensor(0x3E90,0x0E05); // RST_RSTX1
MT9V115_write_cmos_sensor(0x3E92,0x1310); // RST_RSTX2
MT9V115_write_cmos_sensor(0x3E94,0x0904); // RST_SHUTTER
MT9V115_write_cmos_sensor(0x3E96,0x0B00); // RST_COL_PUP
MT9V115_write_cmos_sensor(0x3E98,0x130B); // RST_COL_PDN
MT9V115_write_cmos_sensor(0x3E9A,0x0C06); // RST_BOOST1_EN
MT9V115_write_cmos_sensor(0x3E9C,0x1411); // RST_BOOST2_EN
MT9V115_write_cmos_sensor(0x3E9E,0x0E01); // RST_BOOST_MUX
MT9V115_write_cmos_sensor(0x3ECC,0x4091); // DAC_LD_0_1
MT9V115_write_cmos_sensor(0x3ECE,0x430D); // DAC_LD_2_3
MT9V115_write_cmos_sensor(0x3ED0,0x1817); // DAC_LD_4_5
MT9V115_write_cmos_sensor(0x3ED2,0x8504); // DAC_LD_6_7
MT9V115_write_cmos_sensor(0xA027,0x0030); // CAM_AE_CONFIG_MIN_VIRT_AGAIN

//load_and_go patch1
MT9V115_write_cmos_sensor(0x0982,0x0000); // ACCESS_CTL_STAT
MT9V115_write_cmos_sensor(0x098A,0x0000); // PHYSICAL_ADDRESS_ACCESS
MT9V115_write_cmos_sensor(0x8251,0x3C3C);
MT9V115_write_cmos_sensor(0x8253,0xBDD1);
MT9V115_write_cmos_sensor(0x8255,0xF2D6);
MT9V115_write_cmos_sensor(0x8257,0x15C1);
MT9V115_write_cmos_sensor(0x8259,0x0126);
MT9V115_write_cmos_sensor(0x825B,0x3ADC);
MT9V115_write_cmos_sensor(0x825D,0x0A30);
MT9V115_write_cmos_sensor(0x825F,0xED02);
MT9V115_write_cmos_sensor(0x8261,0xDC08);
MT9V115_write_cmos_sensor(0x8263,0xED00);
MT9V115_write_cmos_sensor(0x8265,0xFC01);
MT9V115_write_cmos_sensor(0x8267,0xFCBD);
MT9V115_write_cmos_sensor(0x8269,0xF5FC);
MT9V115_write_cmos_sensor(0x826B,0x30EC);
MT9V115_write_cmos_sensor(0x826D,0x02FD);
MT9V115_write_cmos_sensor(0x826F,0x0348);
MT9V115_write_cmos_sensor(0x8271,0xB303);
MT9V115_write_cmos_sensor(0x8273,0x4425);
MT9V115_write_cmos_sensor(0x8275,0x0DCC);
MT9V115_write_cmos_sensor(0x8277,0x3180);
MT9V115_write_cmos_sensor(0x8279,0xED00);
MT9V115_write_cmos_sensor(0x827B,0xCCA0);
MT9V115_write_cmos_sensor(0x827D,0x00BD);
MT9V115_write_cmos_sensor(0x827F,0xFBFB);
MT9V115_write_cmos_sensor(0x8281,0x2013);
MT9V115_write_cmos_sensor(0x8283,0xFC03);
MT9V115_write_cmos_sensor(0x8285,0x48B3);
MT9V115_write_cmos_sensor(0x8287,0x0346);
MT9V115_write_cmos_sensor(0x8289,0x220B);
MT9V115_write_cmos_sensor(0x828B,0xCC31);
MT9V115_write_cmos_sensor(0x828D,0x80ED);
MT9V115_write_cmos_sensor(0x828F,0x00CC);
MT9V115_write_cmos_sensor(0x8291,0xA000);
MT9V115_write_cmos_sensor(0x8293,0xBDFC);
MT9V115_write_cmos_sensor(0x8295,0x1738);
MT9V115_write_cmos_sensor(0x8297,0x3839);
MT9V115_write_cmos_sensor(0x8299,0x3CD6);
MT9V115_write_cmos_sensor(0x829B,0x15C1);
MT9V115_write_cmos_sensor(0x829D,0x0126);
MT9V115_write_cmos_sensor(0x829F,0x6DFC);
MT9V115_write_cmos_sensor(0x82A1,0x0348);
MT9V115_write_cmos_sensor(0x82A3,0xB303);
MT9V115_write_cmos_sensor(0x82A5,0x4425);
MT9V115_write_cmos_sensor(0x82A7,0x13FC);
MT9V115_write_cmos_sensor(0x82A9,0x7E26);
MT9V115_write_cmos_sensor(0x82AB,0x83FF);
MT9V115_write_cmos_sensor(0x82AD,0xFF27);
MT9V115_write_cmos_sensor(0x82AF,0x0BFC);
MT9V115_write_cmos_sensor(0x82B1,0x7E26);
MT9V115_write_cmos_sensor(0x82B3,0xFD03);
MT9V115_write_cmos_sensor(0x82B5,0x4CCC);
MT9V115_write_cmos_sensor(0x82B7,0xFFFF);
MT9V115_write_cmos_sensor(0x82B9,0x2013);
MT9V115_write_cmos_sensor(0x82BB,0xFC03);
MT9V115_write_cmos_sensor(0x82BD,0x48B3);
MT9V115_write_cmos_sensor(0x82BF,0x0346);
MT9V115_write_cmos_sensor(0x82C1,0x220E);
MT9V115_write_cmos_sensor(0x82C3,0xFC7E);
MT9V115_write_cmos_sensor(0x82C5,0x2683);
MT9V115_write_cmos_sensor(0x82C7,0xFFFF);
MT9V115_write_cmos_sensor(0x82C9,0x2606);
MT9V115_write_cmos_sensor(0x82CB,0xFC03);
MT9V115_write_cmos_sensor(0x82CD,0x4CFD);
MT9V115_write_cmos_sensor(0x82CF,0x7E26);
MT9V115_write_cmos_sensor(0x82D1,0xFC7E);
MT9V115_write_cmos_sensor(0x82D3,0xD25F);
MT9V115_write_cmos_sensor(0x82D5,0x84F0);
MT9V115_write_cmos_sensor(0x82D7,0x30ED);
MT9V115_write_cmos_sensor(0x82D9,0x00DC);
MT9V115_write_cmos_sensor(0x82DB,0x0AB3);
MT9V115_write_cmos_sensor(0x82DD,0x034A);
MT9V115_write_cmos_sensor(0x82DF,0x2510);
MT9V115_write_cmos_sensor(0x82E1,0xEC00);
MT9V115_write_cmos_sensor(0x82E3,0x270C);
MT9V115_write_cmos_sensor(0x82E5,0xFD03);
MT9V115_write_cmos_sensor(0x82E7,0x4EFC);
MT9V115_write_cmos_sensor(0x82E9,0x7ED2);
MT9V115_write_cmos_sensor(0x82EB,0x840F);
MT9V115_write_cmos_sensor(0x82ED,0xED00);
MT9V115_write_cmos_sensor(0x82EF,0x2019);
MT9V115_write_cmos_sensor(0x82F1,0xDC0A);
MT9V115_write_cmos_sensor(0x82F3,0xB303);
MT9V115_write_cmos_sensor(0x82F5,0x4A24);
MT9V115_write_cmos_sensor(0x82F7,0x15EC);
MT9V115_write_cmos_sensor(0x82F9,0x0083);
MT9V115_write_cmos_sensor(0x82FB,0x0000);
MT9V115_write_cmos_sensor(0x82FD,0x260E);
MT9V115_write_cmos_sensor(0x82FF,0xFC7E);
MT9V115_write_cmos_sensor(0x8301,0xD284);
MT9V115_write_cmos_sensor(0x8303,0x0FFA);
MT9V115_write_cmos_sensor(0x8305,0x034F);
MT9V115_write_cmos_sensor(0x8307,0xBA03);
MT9V115_write_cmos_sensor(0x8309,0x4EFD);
MT9V115_write_cmos_sensor(0x830B,0x7ED2);
MT9V115_write_cmos_sensor(0x830D,0xBDD2);
MT9V115_write_cmos_sensor(0x830F,0xAD38);
MT9V115_write_cmos_sensor(0x8311,0x3900);
MT9V115_write_cmos_sensor(0x098E,0x0000); // LOGICAL_ADDRESS_ACCESS

//patch1 thresholds
MT9V115_write_cmos_sensor(0x0982,0x0000); // ACCESS_CTL_STAT
MT9V115_write_cmos_sensor(0x098A,0x0000); // PHYSICAL_ADDRESS_ACCESS
MT9V115_write_cmos_sensor(0x8344,0x0048);
MT9V115_write_cmos_sensor(0x8346,0x0040);
MT9V115_write_cmos_sensor(0x8348,0x0000);
MT9V115_write_cmos_sensor(0x834A,0x0040);
MT9V115_write_cmos_sensor(0x098E,0x0000); // LOGICAL_ADDRESS_ACCESS

//enable patch1
MT9V115_write_cmos_sensor(0x0982,0x0000); // ACCESS_CTL_STAT
MT9V115_write_cmos_sensor(0x098A,0x0000); // PHYSICAL_ADDRESS_ACCESS
MT9V115_write_cmos_sensor(0x824D,0x0251);
MT9V115_write_cmos_sensor(0x824F,0x0299);
MT9V115_write_cmos_sensor(0x098E,0x0000); // LOGICAL_ADDRESS_ACCESS

//Step4-PGA
MT9V115_write_cmos_sensor(0x3210,0x00B0); // COLOR_PIPELINE_CONTROL
MT9V115_write_cmos_sensor(0x3640,0x00F0); // P_G1_P0Q0
MT9V115_write_cmos_sensor(0x3642,0xD9AC); // P_G1_P0Q1
MT9V115_write_cmos_sensor(0x3644,0x02D0); // P_G1_P0Q2
MT9V115_write_cmos_sensor(0x3646,0x9B4E); // P_G1_P0Q3
MT9V115_write_cmos_sensor(0x3648,0xF22D); // P_G1_P0Q4
MT9V115_write_cmos_sensor(0x364A,0x0110); // P_R_P0Q0
MT9V115_write_cmos_sensor(0x364C,0x930D); // P_R_P0Q1
MT9V115_write_cmos_sensor(0x364E,0x1010); // P_R_P0Q2
MT9V115_write_cmos_sensor(0x3650,0x9A8E); // P_R_P0Q3
MT9V115_write_cmos_sensor(0x3652,0xBC4E); // P_R_P0Q4
MT9V115_write_cmos_sensor(0x3654,0x0130); // P_B_P0Q0
MT9V115_write_cmos_sensor(0x3656,0xBAED); // P_B_P0Q1
MT9V115_write_cmos_sensor(0x3658,0x6A6F); // P_B_P0Q2
MT9V115_write_cmos_sensor(0x365A,0xBFCD); // P_B_P0Q3
MT9V115_write_cmos_sensor(0x365C,0xC48E); // P_B_P0Q4
MT9V115_write_cmos_sensor(0x365E,0x02F0); // P_G2_P0Q0
MT9V115_write_cmos_sensor(0x3660,0xA32C); // P_G2_P0Q1
MT9V115_write_cmos_sensor(0x3662,0x0C30); // P_G2_P0Q2
MT9V115_write_cmos_sensor(0x3664,0xBEAE); // P_G2_P0Q3
MT9V115_write_cmos_sensor(0x3666,0xA08E); // P_G2_P0Q4
MT9V115_write_cmos_sensor(0x3680,0xA1EA); // P_G1_P1Q0
MT9V115_write_cmos_sensor(0x3682,0x61AA); // P_G1_P1Q1
MT9V115_write_cmos_sensor(0x3684,0xE16A); // P_G1_P1Q2
MT9V115_write_cmos_sensor(0x3686,0xB4ED); // P_G1_P1Q3
MT9V115_write_cmos_sensor(0x3688,0xD92E); // P_G1_P1Q4
MT9V115_write_cmos_sensor(0x368A,0xDE0A); // P_R_P1Q0
MT9V115_write_cmos_sensor(0x368C,0x7309); // P_R_P1Q1
MT9V115_write_cmos_sensor(0x368E,0xBC0D); // P_R_P1Q2
MT9V115_write_cmos_sensor(0x3690,0xA58A); // P_R_P1Q3
MT9V115_write_cmos_sensor(0x3692,0x390C); // P_R_P1Q4
MT9V115_write_cmos_sensor(0x3694,0x9AAB); // P_B_P1Q0
MT9V115_write_cmos_sensor(0x3696,0x7BEA); // P_B_P1Q1
MT9V115_write_cmos_sensor(0x3698,0xC86B); // P_B_P1Q2
MT9V115_write_cmos_sensor(0x369A,0x926B); // P_B_P1Q3
MT9V115_write_cmos_sensor(0x369C,0x87AC); // P_B_P1Q4
MT9V115_write_cmos_sensor(0x369E,0x82CB); // P_G2_P1Q0
MT9V115_write_cmos_sensor(0x36A0,0x5D2A); // P_G2_P1Q1
MT9V115_write_cmos_sensor(0x36A2,0x206C); // P_G2_P1Q2
MT9V115_write_cmos_sensor(0x36A4,0xD9AD); // P_G2_P1Q3
MT9V115_write_cmos_sensor(0x36A6,0xA08F); // P_G2_P1Q4
MT9V115_write_cmos_sensor(0x36C0,0x656F); // P_G1_P2Q0
MT9V115_write_cmos_sensor(0x36C2,0xC10D); // P_G1_P2Q1
MT9V115_write_cmos_sensor(0x36C4,0x8B2F); // P_G1_P2Q2
MT9V115_write_cmos_sensor(0x36C6,0xC3A); // P_G1_P2Q3
MT9V115_write_cmos_sensor(0x36C8,0x8C6F); // P_G1_P2Q4
MT9V115_write_cmos_sensor(0x36CA,0x7C8F); // P_R_P2Q0
MT9V115_write_cmos_sensor(0x36CC,0xF04D); // P_R_P2Q1
MT9V115_write_cmos_sensor(0x36CE,0x11AD); // P_R_P2Q2
MT9V115_write_cmos_sensor(0x36D0,0x2D6E); // P_R_P2Q3
MT9V115_write_cmos_sensor(0x36D2,0x368F); // P_R_P2Q4
MT9V115_write_cmos_sensor(0x36D4,0x4C6F); // P_B_P2Q0
MT9V115_write_cmos_sensor(0x36D6,0xAE2E); // P_B_P2Q1
MT9V115_write_cmos_sensor(0x36D8,0xAA0F); // P_B_P2Q2
MT9V115_write_cmos_sensor(0x36DA,0x628F); // P_B_P2Q3
MT9V115_write_cmos_sensor(0x36DC,0x55B0); // P_B_P2Q4
MT9V115_write_cmos_sensor(0x36DE,0x66CF); // P_G2_P2Q0
MT9V115_write_cmos_sensor(0x36E0,0xBAAE); // P_G2_P2Q1
MT9V115_write_cmos_sensor(0x36E2,0xA5CF); // P_G2_P2Q2
MT9V115_write_cmos_sensor(0x36E4,0x780E); // P_G2_P2Q3
MT9V115_write_cmos_sensor(0x36E6,0x336E); // P_G2_P2Q4
MT9V115_write_cmos_sensor(0x3700,0x166B); // P_G1_P3Q0
MT9V115_write_cmos_sensor(0x3702,0x812F); // P_G1_P3Q1
MT9V115_write_cmos_sensor(0x3704,0xFDAC); // P_G1_P3Q2
MT9V115_write_cmos_sensor(0x3706,0x4E90); // P_G1_P3Q3
MT9V115_write_cmos_sensor(0x3708,0x4F11); // P_G1_P3Q4
MT9V115_write_cmos_sensor(0x370A,0x58EA); // P_R_P3Q0
MT9V115_write_cmos_sensor(0x370C,0xCE8A); // P_R_P3Q1
MT9V115_write_cmos_sensor(0x370E,0x890E); // P_R_P3Q2
MT9V115_write_cmos_sensor(0x3710,0x95CE); // P_R_P3Q3
MT9V115_write_cmos_sensor(0x3712,0x1891); // P_R_P3Q4
MT9V115_write_cmos_sensor(0x3714,0x04CC); // P_B_P3Q0
MT9V115_write_cmos_sensor(0x3716,0xBE4C); // P_B_P3Q1
MT9V115_write_cmos_sensor(0x3718,0x992F); // P_B_P3Q2
MT9V115_write_cmos_sensor(0x371A,0x900F); // P_B_P3Q3
MT9V115_write_cmos_sensor(0x371C,0x1271); // P_B_P3Q4
MT9V115_write_cmos_sensor(0x371E,0x626C); // P_G2_P3Q0
MT9V115_write_cmos_sensor(0x3720,0xBB0E); // P_G2_P3Q1
MT9V115_write_cmos_sensor(0x3722,0xF6AF); // P_G2_P3Q2
MT9V115_write_cmos_sensor(0x3724,0x2B50); // P_G2_P3Q3
MT9V115_write_cmos_sensor(0x3726,0x1B12); // P_G2_P3Q4
MT9V115_write_cmos_sensor(0x3740,0xD7CC); // P_G1_P4Q0
MT9V115_write_cmos_sensor(0x3742,0xE2EE); // P_G1_P4Q1
MT9V115_write_cmos_sensor(0x3744,0x75B0); // P_G1_P4Q2
MT9V115_write_cmos_sensor(0x3746,0x1811); // P_G1_P4Q3
MT9V115_write_cmos_sensor(0x3748,0x9972); // P_G1_P4Q4
MT9V115_write_cmos_sensor(0x374A,0x118E); // P_R_P4Q0
MT9V115_write_cmos_sensor(0x374C,0x0747); // P_R_P4Q1
MT9V115_write_cmos_sensor(0x374E,0x1F30); // P_R_P4Q2
MT9V115_write_cmos_sensor(0x3750,0xD7F1); // P_R_P4Q3
MT9V115_write_cmos_sensor(0x3752,0xDCF3); // P_R_P4Q4
MT9V115_write_cmos_sensor(0x3754,0x1D6E); // P_B_P4Q0
MT9V115_write_cmos_sensor(0x3756,0x068F); // P_B_P4Q1
MT9V115_write_cmos_sensor(0x3758,0x07D0); // P_B_P4Q2
MT9V115_write_cmos_sensor(0x375A,0x9F12); // P_B_P4Q3
MT9V115_write_cmos_sensor(0x375C,0xB9B3); // P_B_P4Q4
MT9V115_write_cmos_sensor(0x375E,0x99CE); // P_G2_P4Q0
MT9V115_write_cmos_sensor(0x3760,0x2F0F); // P_G2_P4Q1
MT9V115_write_cmos_sensor(0x3762,0x0DB1); // P_G2_P4Q2
MT9V115_write_cmos_sensor(0x3764,0x8C12); // P_G2_P4Q3
MT9V115_write_cmos_sensor(0x3766,0x8BB3); // P_G2_P4Q4
MT9V115_write_cmos_sensor(0x3782,0x00F4); // CENTER_ROW
MT9V115_write_cmos_sensor(0x3784,0x0188); // CENTER_COLUMN
MT9V115_write_cmos_sensor(0x3210,0x00B0); // COLOR_PIPELINE_CONTROL
//MT9V115_write_cmos_sensor(0x3210,0x00B8); // COLOR_PIPELINE_CONTROL

//Step5-AWB_CCM
MT9V115_write_cmos_sensor(0xA02F,0x0229); // CAM_AWB_CONFIG_CCM_L_0
MT9V115_write_cmos_sensor(0xA031,0xFED6); // CAM_AWB_CONFIG_CCM_L_1
MT9V115_write_cmos_sensor(0xA033,0x0001); // CAM_AWB_CONFIG_CCM_L_2
MT9V115_write_cmos_sensor(0xA035,0xFFC2); // CAM_AWB_CONFIG_CCM_L_3
MT9V115_write_cmos_sensor(0xA037,0x014A); // CAM_AWB_CONFIG_CCM_L_4
MT9V115_write_cmos_sensor(0xA039,0xFFF3); // CAM_AWB_CONFIG_CCM_L_5
MT9V115_write_cmos_sensor(0xA03B,0xFF49); // CAM_AWB_CONFIG_CCM_L_6
MT9V115_write_cmos_sensor(0xA03D,0xFEAD); // CAM_AWB_CONFIG_CCM_L_7
MT9V115_write_cmos_sensor(0xA03F,0x030A); // CAM_AWB_CONFIG_CCM_L_8
MT9V115_write_cmos_sensor(0xA041,0x0019); // CAM_AWB_CONFIG_CCM_L_9
MT9V115_write_cmos_sensor(0xA043,0x004D); // CAM_AWB_CONFIG_CCM_L_10
MT9V115_write_cmos_sensor(0xA045,0xFF84); // CAM_AWB_CONFIG_CCM_RL_0
MT9V115_write_cmos_sensor(0xA047,0x00B4); // CAM_AWB_CONFIG_CCM_RL_1
MT9V115_write_cmos_sensor(0xA049,0xFFC7); // CAM_AWB_CONFIG_CCM_RL_2
MT9V115_write_cmos_sensor(0xA04B,0x000F); // CAM_AWB_CONFIG_CCM_RL_3
MT9V115_write_cmos_sensor(0xA04D,0xFFDF); // CAM_AWB_CONFIG_CCM_RL_4
MT9V115_write_cmos_sensor(0xA04F,0x0012); // CAM_AWB_CONFIG_CCM_RL_5
MT9V115_write_cmos_sensor(0xA051,0x00AB); // CAM_AWB_CONFIG_CCM_RL_6
MT9V115_write_cmos_sensor(0xA053,0x00B2); // CAM_AWB_CONFIG_CCM_RL_7
MT9V115_write_cmos_sensor(0xA055,0xFEA3); // CAM_AWB_CONFIG_CCM_RL_8
MT9V115_write_cmos_sensor(0xA057,0x0011); // CAM_AWB_CONFIG_CCM_RL_9
MT9V115_write_cmos_sensor(0xA059,0xFFDC); // CAM_AWB_CONFIG_CCM_RL_10
MT9V115_write8_cmos_sensor(0x9416,0x38); // AWB_R_SCENE_RATIO_LOWER
MT9V115_write8_cmos_sensor(0x9417,0x95); // AWB_R_SCENE_RATIO_UPPER
MT9V115_write8_cmos_sensor(0x9418,0x15); // AWB_B_SCENE_RATIO_LOWER
MT9V115_write8_cmos_sensor(0x9419,0x61); // AWB_B_SCENE_RATIO_UPPER
MT9V115_write_cmos_sensor(0x2110,0x0034); // AWB_XY_SCALE
MT9V115_write_cmos_sensor(0x2110,0x0024); // AWB_XY_SCALE
MT9V115_write_cmos_sensor(0x2112,0x0000); // AWB_WEIGHT_R0
MT9V115_write_cmos_sensor(0x2114,0x0000); // AWB_WEIGHT_R1
MT9V115_write_cmos_sensor(0x2116,0x0000); // AWB_WEIGHT_R2
MT9V115_write_cmos_sensor(0x2118,0xFFE0); // AWB_WEIGHT_R3
MT9V115_write_cmos_sensor(0x211A,0xA140); // AWB_WEIGHT_R4
MT9V115_write_cmos_sensor(0x211C,0x28A0); // AWB_WEIGHT_R5
MT9V115_write_cmos_sensor(0x211E,0x006E); // AWB_WEIGHT_R6
MT9V115_write_cmos_sensor(0x2120,0x001C); // AWB_WEIGHT_R7
MT9V115_write_cmos_sensor(0xA061,0x0031); // CAM_AWB_CONFIG_X_SHIFT_PRE_ADJ
MT9V115_write_cmos_sensor(0xA063,0x0042); // CAM_AWB_CONFIG_Y_SHIFT_PRE_ADJ


//Step6-CPIPE_Calibration
//PA Settings
MT9V115_write_cmos_sensor(0xA01C,0x0080); // CAM_AE_CONFIG_TARGET_AGAIN
MT9V115_write8_cmos_sensor(0xA020,0x4B); // CAM_AE_CONFIG_BASE_TARGET
MT9V115_write8_cmos_sensor(0xA07A,0x10); // CAM_LL_CONFIG_AP_THRESH_START
MT9V115_write8_cmos_sensor(0xA081,0x1E); // CAM_LL_CONFIG_DM_EDGE_TH_START
MT9V115_write_cmos_sensor(0xA0B9,0x0096); // CAM_LL_CONFIG_START_GAIN_METRIC
MT9V115_write_cmos_sensor(0xA0BB,0x010E); // CAM_LL_CONFIG_STOP_GAIN_METRIC
MT9V115_write_cmos_sensor(0xA01E,0x0080); // CAM_AE_CONFIG_TARGET_DGAIN
MT9V115_write_cmos_sensor(0xA025,0x0080); // CAM_AE_CONFIG_MAX_VIRT_DGAIN
MT9V115_write_cmos_sensor(0xA085,0x0078); // CAM_LL_CONFIG_FTB_AVG_YSUM_STOP
MT9V115_write8_cmos_sensor(0xA07C,0x04); // CAM_LL_CONFIG_AP_GAIN_START
MT9V115_write8_cmos_sensor(0xA05F,0x80); // CAM_AWB_CONFIG_START_SATURATION
MT9V115_write8_cmos_sensor(0xA060,0x46); // CAM_AWB_CONFIG_END_SATURATION
MT9V115_write8_cmos_sensor(0xA060,0x3C); // CAM_AWB_CONFIG_END_SATURATION
MT9V115_write8_cmos_sensor(0xA05F,0x4B); // CAM_AWB_CONFIG_START_SATURATION

//AE Settings
MT9V115_write8_cmos_sensor(0x9003,0x20); // AE_TRACK_BLACK_LEVEL_MAX
MT9V115_write8_cmos_sensor(0xA020,0x3C); // CAM_AE_CONFIG_BASE_TARGET

//Step7-CPIPE_Preference
MT9V115_write_cmos_sensor(0x326E,0x0006); // LOW_PASS_YUV_FILTER
MT9V115_write_cmos_sensor(0x33F4,0x000B); // KERNEL_CONFIG
MT9V115_write8_cmos_sensor(0xA087,0x00); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_0
MT9V115_write8_cmos_sensor(0xA088,0x07); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_1
MT9V115_write8_cmos_sensor(0xA089,0x16); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_2
MT9V115_write8_cmos_sensor(0xA08A,0x30); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_3
MT9V115_write8_cmos_sensor(0xA08B,0x52); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_4
MT9V115_write8_cmos_sensor(0xA08C,0x6D); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_5
MT9V115_write8_cmos_sensor(0xA08D,0x86); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_6
MT9V115_write8_cmos_sensor(0xA08E,0x9B); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_7
MT9V115_write8_cmos_sensor(0xA08F,0xAB); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_8
MT9V115_write8_cmos_sensor(0xA090,0xB9); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_9
MT9V115_write8_cmos_sensor(0xA091,0xC5); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_10
MT9V115_write8_cmos_sensor(0xA092,0xCF); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_11
MT9V115_write8_cmos_sensor(0xA093,0xD8); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_12
MT9V115_write8_cmos_sensor(0xA094,0xE0); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_13
MT9V115_write8_cmos_sensor(0xA095,0xE7); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_14
MT9V115_write8_cmos_sensor(0xA096,0xEE); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_15
MT9V115_write8_cmos_sensor(0xA097,0xF4); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_16
MT9V115_write8_cmos_sensor(0xA098,0xFA); // CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_17
MT9V115_write_cmos_sensor(0xA0AD,0x0001); // CAM_LL_CONFIG_GAMMA_START_BM
MT9V115_write_cmos_sensor(0xA0AF,0x0338); // CAM_LL_CONFIG_GAMMA_STOP_BM
MT9V115_write8_cmos_sensor(0xA060,0x3C); // CAM_AWB_CONFIG_END_SATURATION
MT9V115_write8_cmos_sensor(0xA05F,0x4B); // CAM_AWB_CONFIG_START_SATURATION
MT9V115_write8_cmos_sensor(0x9003,0x20); // AE_TRACK_BLACK_LEVEL_MAX
MT9V115_write8_cmos_sensor(0x8C03,0x01); // FD_STAT_MIN
MT9V115_write8_cmos_sensor(0x8C04,0x03); // FD_STAT_MAX
MT9V115_write8_cmos_sensor(0x8C05,0x05); // FD_MIN_AMPLITUDE

//release power up stop, keep FW PLL startup
MT9V115_write_cmos_sensor(0x0018,0x0002); // STANDBY_CONTROL_AND_STATUS
Sleep(120);
//MT9V115_write_cmos_sensor(0xA076,0x000A); //cam_fd_config_max_fdzone_50hz=3
//MT9V115_write_cmos_sensor(0xA078,0x000C); //cam_fd_config_max_fdzone_60hz=4
//MT9V115_write_cmos_sensor(0xA01A,0x0008); //cam_ae_config_target_fdzone=3

}


static kal_uint16 MT9V115_power_on(void)
{

	kal_uint16 id=0;
	spin_lock(&mt9v115yuv_drv_lock);

	MT9V115Sensor.sensor_id = 0;
	spin_unlock(&mt9v115yuv_drv_lock);
	 id= MT9V115_read_cmos_sensor(MT9V115_ID_REG);
    spin_lock(&mt9v115yuv_drv_lock);
   	MT9V115Sensor.sensor_id=id;
	spin_unlock(&mt9v115yuv_drv_lock);
	SENSORDB("[MT9V115]MT9V115Sensor.sensor_id =%x\n",MT9V115Sensor.sensor_id);
	return MT9V115Sensor.sensor_id;
}


/*************************************************************************
* FUNCTION
*	MT9V115Open
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
UINT32 MT9V115Open(void)
{
	 kal_uint16 data=0;
     SENSORDB("[Enter]:MT9V115 Open func .\n");
	 
	 if (MT9V115_power_on() != MT9V115_SENSOR_ID) 
	 	{
	 	   SENSORDB("[MT9V115]Error:read sensor ID fail\n");
		   return ERROR_SENSOR_CONNECT_FAIL;
	 	}
     
    /* Apply sensor initail setting*/
	MT9V115InitialSetting();
	 SENSORDB("[Exit]:MT9V115 Open func\n");
     
	return ERROR_NONE;
}	/* MT9V115Open() */

/*************************************************************************
* FUNCTION
*	MT9V115Close
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
UINT32 MT9V115Close(void)
{
	SENSORDB("[Exit]:MT9V115 close func\n");

	return ERROR_NONE;
}	/* MT9V115Close() */


static void MT9V115_HVMirror(kal_uint8 image_mirror)
{
	SENSORDB("MT9V115 HVMirror:%d\n",image_mirror);	
	switch (image_mirror)
	{
	case IMAGE_NORMAL:
		    MT9V115_write_cmos_sensor(0x098E, 0x8400);	
			MT9V115_write8_cmos_sensor(0x8400, 0x01);	 //sensor stop mode
			MT9V115_write_cmos_sensor(0x3040, 0x0041); 
			MT9V115_write8_cmos_sensor(0x8400, 0x02); //sensor stream mode
		break;
	case IMAGE_H_MIRROR:
		   MT9V115_write_cmos_sensor(0x098E, 0x8400);	
			MT9V115_write8_cmos_sensor(0x8400, 0x01);	 //sensor stop mode
			MT9V115_write_cmos_sensor(0x3040, 0x4041);
			MT9V115_write8_cmos_sensor(0x8400, 0x02); //sensor stream mode
		break;
	case IMAGE_V_MIRROR:
		break;
	case IMAGE_HV_MIRROR:
		break;
	default:
		   MT9V115_write_cmos_sensor(0x098E, 0x8400);	
			MT9V115_write8_cmos_sensor(0x8400, 0x01);	 //sensor stop mode
			MT9V115_write_cmos_sensor(0x3040, 0x0041);
			MT9V115_write8_cmos_sensor(0x8400, 0x02); //sensor stream mode
		break;
	}
}

void MT9V115_night_mode(kal_bool enable)
{

	  SENSORDB("[Enter]MT9V115 night mode func:enable = %d\n",enable);

	if(enable)
	{
	    MT9V115_write_cmos_sensor(0x098E, 0x2076);// MCU_ADDRESS [AE_MAX_INDEX]
        MT9V115_write_cmos_sensor(0xA076, 0x0014);// MCU_DATA_0
        MT9V115_write_cmos_sensor(0xA078, 0x0018);// MCU_ADDRESS [AE_INDEX_TH23]
	}
	else
    {
		MT9V115_write_cmos_sensor(0x098E, 0x2076);// MCU_ADDRESS [AE_MAX_INDEX]
        MT9V115_write_cmos_sensor(0xA076, 0x000A);// MCU_DATA_0
        MT9V115_write_cmos_sensor(0xA078, 0x000C);// MCU_ADDRESS [AE_INDEX_TH23]
	}
	
}

/*************************************************************************
* FUNCTION
*	MT9V115Preview
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
static UINT32 MT9V115Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	
	kal_uint8 StartX = 0, StartY = 1;  

	
	SENSORDB("[Enter]:MT9V115 preview func:");		

	MT9V115_HVMirror(sensor_config_data->SensorImageMirror);


	image_window->GrabStartX = StartX;
	image_window->GrabStartY = StartY;
	image_window->ExposureWindowWidth = MT9V115_IMAGE_SENSOR_VGA_WIDTH - 2* StartX ;
	image_window->ExposureWindowHeight = MT9V115_IMAGE_SENSOR_VGA_HEIGHT - 2 * StartY;

	SENSORDB("[Exit]:MT9V115 preview func\n");
    return ERROR_NONE; 
}	/* MT9V115_Preview */

UINT32 MT9V115Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint8 StartX = 0, StartY = 1;

	SENSORDB("[Enter]:MT9V115 capture func:");		

	image_window->GrabStartX = StartX;
	image_window->GrabStartY = StartY;
	image_window->ExposureWindowWidth = MT9V115_IMAGE_SENSOR_VGA_WIDTH - 2 * StartX;
	image_window->ExposureWindowHeight = MT9V115_IMAGE_SENSOR_VGA_HEIGHT - 2 * StartY;
}


UINT32 MT9V115GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[Enter]:MT9V115 get Resolution func\n");
	
	pSensorResolution->SensorFullWidth=MT9V115_IMAGE_SENSOR_VGA_WIDTH -8;  
	pSensorResolution->SensorFullHeight=MT9V115_IMAGE_SENSOR_VGA_HEIGHT - 6;
	pSensorResolution->SensorPreviewWidth=MT9V115_IMAGE_SENSOR_VGA_WIDTH -16;
	pSensorResolution->SensorPreviewHeight=MT9V115_IMAGE_SENSOR_VGA_HEIGHT - 12;

    SENSORDB("[Exit]:MT9V115 get Resolution func\n");
	
	return ERROR_NONE;
}	/* MT9V115GetResolution() */

UINT32 MT9V115GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("[Enter]:MT9V115 getInfo func:ScenarioId = %d\n",ScenarioId);
   
	pSensorInfo->SensorPreviewResolutionX=MT9V115_IMAGE_SENSOR_VGA_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=MT9V115_IMAGE_SENSOR_VGA_HEIGHT;
	pSensorInfo->SensorFullResolutionX=MT9V115_IMAGE_SENSOR_VGA_WIDTH;
	pSensorInfo->SensorFullResolutionY=MT9V115_IMAGE_SENSOR_VGA_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=30;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;//low is to reset 
	pSensorInfo->SensorResetDelayCount=4;  //4ms 
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
	pSensorInfo->SensorInterruptDelayLines = 1; 
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH; //???
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;

	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
	pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;

	pSensorInfo->CaptureDelayFrame = 2; 
	pSensorInfo->PreviewDelayFrame = 2; 
	pSensorInfo->VideoDelayFrame = 2; 
	pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;   		
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
	        pSensorInfo->SensorGrabStartX = 4; 
	        pSensorInfo->SensorGrabStartY =2;  			
			
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = 4; 
            pSensorInfo->SensorGrabStartY = 2;			
		break;
		default:
			pSensorInfo->SensorClockFreq=24;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
            pSensorInfo->SensorGrabStartX = 4; 
            pSensorInfo->SensorGrabStartY = 2;  			
		break;
	}
//	MT9V115_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &MT9V115SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

	SENSORDB("[Exit]:MT9V115 getInfo func\n");
	
	return ERROR_NONE;
}	/* MT9V115GetInfo() */


UINT32 MT9V115Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
   SENSORDB("[Enter]:MT9V115 Control func:ScenarioId = %d\n",ScenarioId);

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			 MT9V115Preview(pImageWindow, pSensorConfigData);
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			 MT9V115Capture(pImageWindow, pSensorConfigData);
		default:
		     break; 
	}

   SENSORDB("[Exit]:MT9V115 Control func\n");
	
	return ERROR_NONE;
}	/* MT9V115Control() */


/*************************************************************************
* FUNCTION
*	MT9V115_set_param_wb
*
* DESCRIPTION
*	wb setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL MT9V115_set_param_wb(UINT16 para)
{
	
    //This sensor need more time to balance AWB, 
    //we suggest higher fps or drop some frame to avoid garbage color when preview initial
   SENSORDB("[Enter]MT9V115 set_param_wb func:para = %d\n",para);
	switch (para)
	{            
		case AWB_MODE_AUTO:
			MT9V115_write_cmos_sensor(0x098E,0x9401);  
        	MT9V115_write8_cmos_sensor(0x9401,0x0D);
		    break;
		case AWB_MODE_CLOUDY_DAYLIGHT:
			MT9V115_write_cmos_sensor(0x098E,0x9401);  
        	MT9V115_write8_cmos_sensor(0x9401,0x0C);
			MT9V115_write8_cmos_sensor(0x9436,0x42);
			MT9V115_write8_cmos_sensor(0x9437,0x56);
	        
		    break;
		case AWB_MODE_DAYLIGHT:
            MT9V115_write_cmos_sensor(0x098E,0x9401);  
      	  	MT9V115_write8_cmos_sensor(0x9401,0x0C);
			MT9V115_write8_cmos_sensor(0x9436,0x4B);
			MT9V115_write8_cmos_sensor(0x9437,0x49);
		    break;
		case AWB_MODE_INCANDESCENT:	
		    MT9V115_write_cmos_sensor(0x098E,0x9401);  
        	MT9V115_write8_cmos_sensor(0x9401,0x0C);
			MT9V115_write8_cmos_sensor(0x9436,0x5E);
			MT9V115_write8_cmos_sensor(0x9437,0x33);		
		    break;  
		case AWB_MODE_FLUORESCENT:
		    MT9V115_write_cmos_sensor(0x098E,0x9401);  
        	MT9V115_write8_cmos_sensor(0x9401,0x0C);
			MT9V115_write8_cmos_sensor(0x9436,0x4D);
			MT9V115_write8_cmos_sensor(0x9437,0x3C);
		    break;  
		case AWB_MODE_TUNGSTEN:
			MT9V115_write_cmos_sensor(0x098E,0x9401);  
     		MT9V115_write8_cmos_sensor(0x9401,0x0C);
			MT9V115_write8_cmos_sensor(0x9436,0x5E);
			MT9V115_write8_cmos_sensor(0x9437,0x33);
			break;
		default:
			return FALSE;
	}

	return TRUE;
	
} /* MT9V115_set_param_wb */

/*************************************************************************
* FUNCTION
*	MT9V115_set_param_effect
*
* DESCRIPTION
*	effect setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL MT9V115_set_param_effect(UINT16 para)
{

   SENSORDB("[Enter]MT9V115 set_param_effect func:para = %d\n",para);
   switch (para)
	{
		case MEFFECT_OFF:
			MT9V115_write_cmos_sensor(0x098E,0xA010);
			MT9V115_write8_cmos_sensor(0xA010,0x00);
			Sleep(10);
	        break;
		case MEFFECT_NEGATIVE:
			MT9V115_write_cmos_sensor(0x098E,0xA010);
			MT9V115_write8_cmos_sensor(0xA010,0x03);
			
			Sleep(10);
			break;
		case MEFFECT_SEPIA:
			MT9V115_write_cmos_sensor(0x098E,0xA010);
			MT9V115_write8_cmos_sensor(0xA010,0x02);
        	MT9V115_write8_cmos_sensor(0xA012,0x1E);
        	MT9V115_write8_cmos_sensor(0xA013,0xD8);
			Sleep(10);	
			break;  
		case MEFFECT_SEPIAGREEN:		
			MT9V115_write_cmos_sensor(0x098E,0xA010);
			MT9V115_write8_cmos_sensor(0xA010,0x02);
        	MT9V115_write8_cmos_sensor(0xA012,0xDF);
        	MT9V115_write8_cmos_sensor(0xA013,0xC0);
			Sleep(10);	
			break;
		case MEFFECT_SEPIABLUE:
			MT9V115_write_cmos_sensor(0x098E,0xA010);
			MT9V115_write8_cmos_sensor(0xA010,0x02);
        	MT9V115_write8_cmos_sensor(0xA012,0x00);
        	MT9V115_write8_cmos_sensor(0xA013,0x3F);
			Sleep(10);     
			break;        
		case MEFFECT_MONO:			
			MT9V115_write_cmos_sensor(0x098E,0xA010);
			MT9V115_write8_cmos_sensor(0xA010,0x01);
        	MT9V115_write8_cmos_sensor(0xA012,0x1E);
        	MT9V115_write8_cmos_sensor(0xA013,0xD8);
			Sleep(10);
			break;

		default:
			return KAL_FALSE;
	}

	return KAL_TRUE;

} /* MT9V115_set_param_effect */

/*************************************************************************
* FUNCTION
*	MT9V115_set_param_banding
*
* DESCRIPTION
*	banding setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL MT9V115_set_param_banding(UINT16 para)
{


	
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			SENSORDB("[Enter]MT9V115 set_param_banding func 50HZ\n");
			MT9V115_write_cmos_sensor(0x098E, 0X8C00);    /* enable banding and 50 Hz */
			MT9V115_write8_cmos_sensor(0x8C00,0x02);
			MT9V115_write8_cmos_sensor(0x8C01,0x01);
			Sleep(10);
		break;

		case AE_FLICKER_MODE_60HZ:
			SENSORDB("[Enter]MT9V115 set_param_banding func:para 60HZ\n");
	    	MT9V115_write_cmos_sensor(0x098E, 0X8C00);    /* enable banding and 60 Hz */
			MT9V115_write8_cmos_sensor(0x8C00,0x02);
			MT9V115_write8_cmos_sensor(0x8C01,0x00);
			Sleep(10);
		break;

	    default:
	        return KAL_FALSE;
	}
	

	return KAL_TRUE;
} /* MT9V115_set_param_banding */




/*************************************************************************
* FUNCTION
*	MT9V115_set_param_exposure
*
* DESCRIPTION
*	exposure setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL MT9V115_set_param_exposure(UINT16 para)
{

	kal_uint16 base_target = 0;

	SENSORDB("[Enter]MT9V115 set_param_exposure func:para = %d\n",para);
	MT9V115_write_cmos_sensor(0x098E, 0xA01F);

	switch (para)
	{
		case AE_EV_COMP_13:  //+4 EV
			MT9V115_write8_cmos_sensor(0xA020, 0x5F);
			break;  
		case AE_EV_COMP_10:  //+3 EV
			MT9V115_write8_cmos_sensor(0xA020, 0x55);
			break;    
		case AE_EV_COMP_07:  //+2 EV
			MT9V115_write8_cmos_sensor(0xA020, 0x4B);
			break;    
		case AE_EV_COMP_03:	 //	+1 EV	
			MT9V115_write8_cmos_sensor(0xA020, 0x41);
			break;    
		case AE_EV_COMP_00:  // +0 EV
			MT9V115_write8_cmos_sensor(0xA020, 0x37);
			break;    
		case AE_EV_COMP_n03:  // -1 EV
			MT9V115_write8_cmos_sensor(0xA020, 0x2D);
			break;    
		case AE_EV_COMP_n07:	// -2 EV		
			MT9V115_write8_cmos_sensor(0xA020, 0x23);
			break;    
		case AE_EV_COMP_n10:   //-3 EV
			MT9V115_write8_cmos_sensor(0xA020, 0x19);
			break;
		case AE_EV_COMP_n13:  // -4 EV
			MT9V115_write8_cmos_sensor(0xA020, 0x0F);
			break;
		default:
			return FALSE;
	}
	

	return TRUE;
	
} /* MT9V115_set_param_exposure */


UINT32 MT9V115YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
    SENSORDB("[Enter]MT9V115YUVSensorSetting func:cmd=%d,iPara=%d\n",iCmd,iPara);
	
	switch (iCmd) {
	case FID_SCENE_MODE:	    //auto mode or night mode

		    if (iPara == SCENE_MODE_OFF)//auto mode
		    {
		        MT9V115_night_mode(FALSE); 
		    }
		    else if (iPara == SCENE_MODE_NIGHTSCENE)//night mode
		    {
	            MT9V115_night_mode(TRUE); 
		    }	
			
	     break; 	    
	case FID_AWB_MODE:
           MT9V115_set_param_wb(iPara);
	     break;
	case FID_COLOR_EFFECT:
           MT9V115_set_param_effect(iPara);
	     break;
	case FID_AE_EV:	    	    
           MT9V115_set_param_exposure(iPara);
	     break;
	case FID_AE_FLICKER:	    	    	    
           MT9V115_set_param_banding(iPara);
	     break;
	case FID_ZOOM_FACTOR:
		spin_lock(&mt9v115yuv_drv_lock);
	     MT9V115Sensor.Digital_Zoom_Factor= iPara; 		
		spin_unlock(&mt9v115yuv_drv_lock);
	     break; 
	default:
	     break;
	}
	return TRUE;
}   /* MT9V115YUVSensorSetting */

UINT32 MT9V115YUVSetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("[Enter]MT9V115 Set Video Mode:FrameRate= %d\n",u2FrameRate);
	#if 1
	if (u2FrameRate == 30)
    {
    	MT9V115_write_cmos_sensor(0x098E, 0xA01A);
    	MT9V115_write_cmos_sensor(0xA01A, 0x0003);
    	MT9V115_write_cmos_sensor(0xA076, 0x0003);
		MT9V115_write_cmos_sensor(0xA078, 0x0004);
    	MT9V115_write_cmos_sensor(0x300A, 0x01F9);
		Sleep(12);
        MT9V115_write_cmos_sensor(0xa020, 0x0000);	
		Sleep(120);
		MT9V115_write_cmos_sensor(0xa020, 0x5000);	
    }
    else if (u2FrameRate == 15)       
    {
		MT9V115_write_cmos_sensor(0x098E, 0xA01A);
		MT9V115_write_cmos_sensor(0xA01A, 0x0003);
		MT9V115_write_cmos_sensor(0xA076, 0x0006);
		MT9V115_write_cmos_sensor(0xA078, 0x0008);
		MT9V115_write_cmos_sensor(0x300A, 0x03F2);
		Sleep(12);
		MT9V115_write_cmos_sensor(0xa020, 0x0000);	
		Sleep(120);
		MT9V115_write_cmos_sensor(0xa020, 0x5000);	
    }
    else 
    {
        printk("Wrong frame rate setting \n");
    }   
	#else
	MT9V115_write_cmos_sensor(0x300A,505*(300/u2FrameRate));
	MT9V115_write_cmos_sensor(0x098E,0x2076);
	MT9V115_write_cmos_sensor(0xA076,1000/u2FrameRate);
	MT9V115_write_cmos_sensor(0xA078,1200/u2FrameRate);

	#endif
	
    return TRUE;
}

UINT32 MT9V115FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 u2Temp = 0; 
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

	SENSORDB("[Enter]:MT9V115 Feature Control func:FeatureId = %d\n",FeatureId);
	
	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=MT9V115_IMAGE_SENSOR_VGA_WIDTH;
			*pFeatureReturnPara16=MT9V115_IMAGE_SENSOR_VGA_WIDTH;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_GET_PERIOD:
			*pFeatureReturnPara16++=MT9V115_IMAGE_SENSOR_VGA_WIDTH;
			*pFeatureReturnPara16=MT9V115_IMAGE_SENSOR_VGA_WIDTH;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			//*pFeatureReturnPara32 = MT9V115_sensor_pclk/10;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_SET_ESHUTTER:
	
		     break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			 MT9V115_night_mode((BOOL) *pFeatureData16);
		     break;
		case SENSOR_FEATURE_SET_GAIN:
			 break; 
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		     break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		     break;
		case SENSOR_FEATURE_SET_REGISTER:
			 MT9V115_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		     break;
		case SENSOR_FEATURE_GET_REGISTER:
			 pSensorRegData->RegData = MT9V115_read_cmos_sensor(pSensorRegData->RegAddr);
		     break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			 memcpy(pSensorConfigData, &MT9V115SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			 *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		     break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			MT9V115GetSensorID(pFeatureReturnPara32);
			break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
		case SENSOR_FEATURE_GET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
		case SENSOR_FEATURE_GET_GROUP_INFO:
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
		     break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
	               // *pFeatureReturnPara32++=0;
			//*pFeatureParaLen=4;
		     break; 
		
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_SET_YUV_CMD:
			 MT9V115YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		     break;	
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		     MT9V115YUVSetVideoMode(*pFeatureData16);
		     break; 
		default:
			 break;			
	}
	return ERROR_NONE;
}	/* MT9V115FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncMT9V115=
{
	MT9V115Open,
	MT9V115GetInfo,
	MT9V115GetResolution,
	MT9V115FeatureControl,
	MT9V115Control,
	MT9V115Close
};

UINT32 MT9V115_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncMT9V115;

	return ERROR_NONE;
}	/* SensorInit() */


