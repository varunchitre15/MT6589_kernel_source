/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.c
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author: 
 * -------
 *   Anyuan Huang (MTK70663)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 04 20 2012 chengxue.shen
 * [ALPS00272900] HI542 Sensor Driver Check In
 * SIV120B Sensor driver Modify For MT6577 dual core processor
 *
 * 05 17 2011 koli.lin
 * [ALPS00048684] [Need Patch] [Volunteer Patch]
 * [Camera] Add the sensor software power down mode.
 *
 * 03 09 2011 koli.lin
 * [ALPS00030473] [Camera]
 * Add the SIV120B sensor driver for MT6575 LDVT.
 *
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
#include <asm/io.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "siv120byuv_Sensor.h"
#include "siv120byuv_Camera_Sensor_para.h"
#include "siv120byuv_CameraCustomized.h"

#define SIV120DYUV_DEBUG
#ifdef SIV120DYUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

typedef struct
{
  kal_uint8   iSensorVersion;
  kal_bool    bNightMode;
  kal_uint16  iWB;
  kal_uint16  iAE;
  kal_uint16  iEffect;
  kal_uint16  iEV;
  kal_uint16  iBanding;
  kal_bool    bFixFrameRate;
  kal_uint8   iMirror;
  kal_uint16  iDummyPixel;  /* dummy pixel for user customization */
  kal_bool    bVideoMode;
  kal_uint8   iPclk;
  kal_uint16  MinFpsNormal; 
  kal_uint16  MinFpsNight; 
  /* Sensor regester backup*/
  kal_uint8   iCurrentPage;
  kal_uint8   iControl;
  kal_uint16  iHblank;     /* dummy pixel for calculating shutter step*/
  kal_uint16  iVblank;     /* dummy line calculated by cal_fps*/
  kal_uint8   iShutterStep;
  kal_uint8   iFrameCount;
} SIV120DStatus;

#define Sleep(ms) mdelay(ms)

static DEFINE_SPINLOCK(siv120d_drv_lock);

/* Global Valuable */
SIV120DStatus SIV120DCurrentStatus;
MSDK_SENSOR_CONFIG_STRUCT SIV120DSensorConfigData;

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 SIV120DWriteCmosSensor(kal_uint32 addr, kal_uint32 para)
{
  char puSendCmd[2] = {(char)(addr & 0xFF) ,(char)(para & 0xFF)};

  iWriteRegI2C(puSendCmd , 2,SIV120D_WRITE_ID);
  return TRUE;
}

kal_uint16 SIV120DReadCmosSensor(kal_uint32 addr)
{
  char puGetByte=0;
  char puSendCmd = (char)(addr & 0xFF);
  iReadRegI2C(&puSendCmd , 1, &puGetByte,1,SIV120D_WRITE_ID);
  return puGetByte;
}

static void SIV120DSetPage(kal_uint8 iPage)
{   

  if(SIV120DCurrentStatus.iCurrentPage == iPage)
    return ;

  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iCurrentPage = iPage;
  spin_unlock(&siv120d_drv_lock);
  
  SIV120DWriteCmosSensor(0x00,iPage);
}

static void SIV120DInitialPara(void)
{
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.bNightMode = KAL_FALSE;
  SIV120DCurrentStatus.iWB = AWB_MODE_AUTO;
  SIV120DCurrentStatus.iAE = AE_MODE_AUTO;
  SIV120DCurrentStatus.iEffect = MEFFECT_OFF;
  SIV120DCurrentStatus.iBanding = SIV120D_NUM_50HZ;
  SIV120DCurrentStatus.iEV = AE_EV_COMP_00;
  SIV120DCurrentStatus.bFixFrameRate = KAL_FALSE;
  SIV120DCurrentStatus.iMirror = IMAGE_NORMAL;
  SIV120DCurrentStatus.iDummyPixel = 0x1D;     
  SIV120DCurrentStatus.bVideoMode = KAL_FALSE;
  SIV120DCurrentStatus.iPclk = 26;
  
  SIV120DCurrentStatus.iCurrentPage = 0;
  SIV120DCurrentStatus.iControl = 0x00;
  SIV120DCurrentStatus.iHblank = 0x00;
  SIV120DCurrentStatus.iVblank = 0x00;
  SIV120DCurrentStatus.iShutterStep = 0x00;
  SIV120DCurrentStatus.iFrameCount = 0x0A;

  SIV120DCurrentStatus.MinFpsNormal = SIV120D_FPS(10);
  SIV120DCurrentStatus.MinFpsNight =  SIV120DCurrentStatus.MinFpsNormal >> 1;
  spin_unlock(&siv120d_drv_lock);
}

static void SIV120DInitialSetting(void)
{
  SENSORDB("SIV120DInitialSetting\n");

  // SNR block [Vendor recommended value ##Don't change##]
  SIV120DSetPage(0);
  SIV120DWriteCmosSensor(0x07,0x00);
  SIV120DWriteCmosSensor(0x04,0x00);
  SIV120DWriteCmosSensor(0x05,0x03); //VGA Output
  SIV120DWriteCmosSensor(0x10,0x34);
  SIV120DWriteCmosSensor(0x11,0x27);
  SIV120DWriteCmosSensor(0x12,0x21);
  SIV120DWriteCmosSensor(0x13,0x17); // ABS
  SIV120DWriteCmosSensor(0x16,0xCF);
  SIV120DWriteCmosSensor(0x17,0xaa); //Internal LDO On: 0xaa
                                     //Internal LDO off: 0x00                           
  // SIV120D 50Hz - 24MHz
  SIV120DWriteCmosSensor(0x20,0x00);
  SIV120DWriteCmosSensor(0x21,0x6E);
  SIV120DWriteCmosSensor(0x23,0x1D);

  // AE
  SIV120DSetPage(1);
  SIV120DWriteCmosSensor(0x34,0x84);                          

  SIV120DWriteCmosSensor(0x11,0x0A);
  SIV120DWriteCmosSensor(0x12,0x90); // D65 target 0x84 // 0x88
  SIV120DWriteCmosSensor(0x13,0x90); // D30 target
  SIV120DWriteCmosSensor(0x14,0x90); // D20 target

  // Init shutter & gain
  SIV120DWriteCmosSensor(0x1D,0x02); //Initial shutter time
  SIV120DWriteCmosSensor(0x1E,0x08); //Initial analog gain

  SIV120DWriteCmosSensor(0x40,0x64); //Max Analog Gain Value @ Shutter step = Max Shutter step  0x7D

  // AWB                     
  SIV120DSetPage(2);                               
  SIV120DWriteCmosSensor(0x10,0xD3);           
  SIV120DWriteCmosSensor(0x11,0xC0);         
  SIV120DWriteCmosSensor(0x12,0x80);         
  SIV120DWriteCmosSensor(0x13,0x80);         
  SIV120DWriteCmosSensor(0x14,0x80);         
  SIV120DWriteCmosSensor(0x15,0xFE); //R gain To
  SIV120DWriteCmosSensor(0x16,0x80); //R gain bo
  SIV120DWriteCmosSensor(0x17,0xcb); //B gain To
  SIV120DWriteCmosSensor(0x18,0x70); //B gain bo
                                                   
  SIV120DWriteCmosSensor(0x19,0x94); //Cr top va   
  SIV120DWriteCmosSensor(0x1A,0x6c); //Cr bottom   
  SIV120DWriteCmosSensor(0x1B,0x94); //Cb top va   
  SIV120DWriteCmosSensor(0x1C,0x6c); //Cb bottom   
  SIV120DWriteCmosSensor(0x1D,0x94);     
  SIV120DWriteCmosSensor(0x1E,0x6c);     
                                                     
  SIV120DWriteCmosSensor(0x20,0xE8); //AWB lumin                
  SIV120DWriteCmosSensor(0x21,0x30); //AWB lumin                
  SIV120DWriteCmosSensor(0x22,0xA4);                          
  SIV120DWriteCmosSensor(0x23,0x20);                          
  SIV120DWriteCmosSensor(0x24,0xFF);               
  SIV120DWriteCmosSensor(0x25,0x20);                      
  SIV120DWriteCmosSensor(0x26,0x0F);                      
  SIV120DWriteCmosSensor(0x27,0x92); //BRTSRT          0x10  0x63   
  SIV120DWriteCmosSensor(0x28,0xaf); //BRTRGNTOP    0xCB  0x63      
  SIV120DWriteCmosSensor(0x29,0xab); //BRTRGNBOT            
  SIV120DWriteCmosSensor(0x2A,0x92); //BRTBGNTOP  //0xA0
  SIV120DWriteCmosSensor(0x2B,0x8e); //BRTBGNBOT  
  SIV120DWriteCmosSensor(0x2C,0x88); //RGAINCONT    
  SIV120DWriteCmosSensor(0x2D,0x88); //BGAINCONT             
                                                    
  SIV120DWriteCmosSensor(0x30,0x00);    
  SIV120DWriteCmosSensor(0x31,0x10);    
  SIV120DWriteCmosSensor(0x32,0x00);
  SIV120DWriteCmosSensor(0x33,0x10);
  SIV120DWriteCmosSensor(0x34,0x02);
  SIV120DWriteCmosSensor(0x35,0x76);
  SIV120DWriteCmosSensor(0x36,0x01);
  SIV120DWriteCmosSensor(0x37,0xD6);
  SIV120DWriteCmosSensor(0x40,0x01);
  SIV120DWriteCmosSensor(0x41,0x04);
  SIV120DWriteCmosSensor(0x42,0x08);
  SIV120DWriteCmosSensor(0x43,0x10);
  SIV120DWriteCmosSensor(0x44,0x12);
  SIV120DWriteCmosSensor(0x45,0x35);
  SIV120DWriteCmosSensor(0x46,0x64);
  SIV120DWriteCmosSensor(0x50,0x33);
  SIV120DWriteCmosSensor(0x51,0x20);
  SIV120DWriteCmosSensor(0x52,0xE5);
  SIV120DWriteCmosSensor(0x53,0xFB);
  SIV120DWriteCmosSensor(0x54,0x13);
  SIV120DWriteCmosSensor(0x55,0x26);
  SIV120DWriteCmosSensor(0x56,0x07);
  SIV120DWriteCmosSensor(0x57,0xF5);
  SIV120DWriteCmosSensor(0x58,0xEA);
  SIV120DWriteCmosSensor(0x59,0x21);

  SIV120DWriteCmosSensor(0x62,0x88); //G gain

  SIV120DWriteCmosSensor(0x74,0xb2); //DNP detect 100416
  SIV120DWriteCmosSensor(0x75,0xcc);
  SIV120DWriteCmosSensor(0x76,0x7b);
  SIV120DWriteCmosSensor(0x77,0x91);
  SIV120DWriteCmosSensor(0x80,0x18);
  SIV120DWriteCmosSensor(0x81,0x92);
  SIV120DWriteCmosSensor(0x82,0x08);


  //CMA change  -D65~A
  SIV120DWriteCmosSensor(0x63,0xB3); //D30 to D20 for R
  SIV120DWriteCmosSensor(0x64,0xB8); //D30 to D20 for B
  SIV120DWriteCmosSensor(0x65,0xB3); //D20 to D30 for R
  SIV120DWriteCmosSensor(0x66,0xB8); //D20 to D30 for B
  SIV120DWriteCmosSensor(0x67,0xDD); //D65 to D30 for R
  SIV120DWriteCmosSensor(0x68,0xA0); //D65 to D30 for B
  SIV120DWriteCmosSensor(0x69,0xDD); //D30 to D65 for R
  SIV120DWriteCmosSensor(0x6A,0xA0); //D30 to D65 for B
                                                          
  // IDP                      
  SIV120DSetPage(3);
  SIV120DWriteCmosSensor(0x10,0xFF); // IDP function enable
  SIV120DWriteCmosSensor(0x11,0x1D); // PCLK polarity
  SIV120DWriteCmosSensor(0x12,0x1D); // Y,Cb,Cr order sequence
  SIV120DWriteCmosSensor(0x14,0x04); 

  // DPCNR                      
  SIV120DWriteCmosSensor(0x17,0x28); //DPCNRCTRL                                          
  SIV120DWriteCmosSensor(0x18,0x04); //DPTHR                                              
  SIV120DWriteCmosSensor(0x19,0x50); //[7:6] G DP Number Thr @ Dark | [5:0] DPTHRMIN      
  SIV120DWriteCmosSensor(0x1A,0x50); //[7:6] G DP Number Thr @ Normal | [5:0] DPTHRMAX    
  SIV120DWriteCmosSensor(0x1B,0x12); //DPTHRSLP( [7:4] @ Normal | [3:0] @ Dark )          
  SIV120DWriteCmosSensor(0x1C,0x00); //NRTHR                                              
  SIV120DWriteCmosSensor(0x1D,0x08); //[7:6] C DP Number Thr @ Dark | [5:0] NRTHRMIN 0x48 
  SIV120DWriteCmosSensor(0x1E,0x08); //[7:6] C DP Number Thr @ Normal | [5:0] NRTHRMAX 0x48
  SIV120DWriteCmosSensor(0x1F,0x28); //NRTHRSLP( [7:4] @ Normal | [3:0] @ Dark )  0x2f     
  SIV120DWriteCmosSensor(0x20,0x04); //IllumiInfo STRTNOR
  SIV120DWriteCmosSensor(0x21,0x0F); //IllumiInfo STRTDRK

  // Gamma
  SIV120DWriteCmosSensor(0x30,0x00);
  SIV120DWriteCmosSensor(0x31,0x04);
  SIV120DWriteCmosSensor(0x32,0x0B);
  SIV120DWriteCmosSensor(0x33,0x24);
  SIV120DWriteCmosSensor(0x34,0x49);
  SIV120DWriteCmosSensor(0x35,0x66);
  SIV120DWriteCmosSensor(0x36,0x7C);
  SIV120DWriteCmosSensor(0x37,0x8D);
  SIV120DWriteCmosSensor(0x38,0x9B);
  SIV120DWriteCmosSensor(0x39,0xAA);
  SIV120DWriteCmosSensor(0x3A,0xB6);
  SIV120DWriteCmosSensor(0x3B,0xCA);
  SIV120DWriteCmosSensor(0x3C,0xDC);
  SIV120DWriteCmosSensor(0x3D,0xEF);
  SIV120DWriteCmosSensor(0x3E,0xF8);
  SIV120DWriteCmosSensor(0x3F,0xFF);

  // Shading
  SIV120DWriteCmosSensor(0x40,0x0A);                                                
  SIV120DWriteCmosSensor(0x41,0xDC);                                                
  SIV120DWriteCmosSensor(0x42,0x77);                                                
  SIV120DWriteCmosSensor(0x43,0x66);                                                
  SIV120DWriteCmosSensor(0x44,0x55);                                                
  SIV120DWriteCmosSensor(0x45,0x44);                                                
  SIV120DWriteCmosSensor(0x46,0x11); //left R gain[7:4], right R gain[3:0]             
  SIV120DWriteCmosSensor(0x47,0x11); //top R gain[7:4], bottom R gain[3:0]             
  SIV120DWriteCmosSensor(0x48,0x10); //left Gr gain[7:4], right Gr gain[3:0] 0x21      
  SIV120DWriteCmosSensor(0x49,0x00); //top Gr gain[7:4], bottom Gr gain[3:0]           
  SIV120DWriteCmosSensor(0x4A,0x00); //left Gb gain[7:4], right Gb gain[3:0] 0x02      
  SIV120DWriteCmosSensor(0x4B,0x00); //top Gb gain[7:4], bottom Gb gain[3:0]           
  SIV120DWriteCmosSensor(0x4C,0x00); //left B gain[7:4], right B gain[3:0]             
  SIV120DWriteCmosSensor(0x4D,0x11); //top B gain[7:4], bottom B gain[3:0]             
  SIV120DWriteCmosSensor(0x4E,0x04); //X-axis center high[3:2], Y-axis center high[1:0]
  SIV120DWriteCmosSensor(0x4F,0x98); //X-axis center low[7:0] 0x50                     
  SIV120DWriteCmosSensor(0x50,0xD8); //Y-axis center low[7:0] 0xf6                     
  SIV120DWriteCmosSensor(0x51,0x80); //Shading Center Gain                             
  SIV120DWriteCmosSensor(0x52,0x00); //Shading R Offset                                
  SIV120DWriteCmosSensor(0x53,0x00); //Shading Gr Offset                               
  SIV120DWriteCmosSensor(0x54,0x00); //Shading Gb Offset                               
  SIV120DWriteCmosSensor(0x55,0x00); //Shading B Offset                    

  SIV120DWriteCmosSensor(0x62,0x77); //ASLPCTRL 7:4 GE, 3:0 YE 
  SIV120DWriteCmosSensor(0x63,0xB7); //YDTECTRL (YE)           
  SIV120DWriteCmosSensor(0x64,0xB7); //GPEVCTRL (GE)           
                       
  SIV120DWriteCmosSensor(0x6D,0x88); //YFLTCTRL           

  // Color Matrix (D65) - Daylight    
  SIV120DWriteCmosSensor(0x71,0x42);  
  SIV120DWriteCmosSensor(0x72,0xbf);  
  SIV120DWriteCmosSensor(0x73,0x00);  
  SIV120DWriteCmosSensor(0x74,0x0f);  
  SIV120DWriteCmosSensor(0x75,0x31);  
  SIV120DWriteCmosSensor(0x76,0x00);  
  SIV120DWriteCmosSensor(0x77,0x00);  
  SIV120DWriteCmosSensor(0x78,0xbc);  
  SIV120DWriteCmosSensor(0x79,0x44);  

  // Color Matrix (D30) - CWF
  SIV120DWriteCmosSensor(0x7A,0x3a);
  SIV120DWriteCmosSensor(0x7B,0xcd);
  SIV120DWriteCmosSensor(0x7C,0xf9);
  SIV120DWriteCmosSensor(0x7D,0x12);
  SIV120DWriteCmosSensor(0x7E,0x2b);
  SIV120DWriteCmosSensor(0x7F,0x03);
  SIV120DWriteCmosSensor(0x80,0xf5);
  SIV120DWriteCmosSensor(0x81,0xc5);
  SIV120DWriteCmosSensor(0x82,0x46);

  // Color Matrix (D20) - A
  SIV120DWriteCmosSensor(0x83,0x44);
  SIV120DWriteCmosSensor(0x84,0xc5);
  SIV120DWriteCmosSensor(0x85,0xf7);
  SIV120DWriteCmosSensor(0x86,0x17);
  SIV120DWriteCmosSensor(0x87,0x36);
  SIV120DWriteCmosSensor(0x88,0xf4);
  SIV120DWriteCmosSensor(0x89,0xef);
  SIV120DWriteCmosSensor(0x8A,0xbb);
  SIV120DWriteCmosSensor(0x8B,0x56);
                        
  SIV120DWriteCmosSensor(0x8C,0x10); //CMA select   
  SIV120DWriteCmosSensor(0x8D,0x04); //programmable 
  SIV120DWriteCmosSensor(0x8E,0x02); //PROGEVAL     
  SIV120DWriteCmosSensor(0x8F,0x00); //Cb/Cr coring                          
                        
  // Edge - Green
  SIV120DWriteCmosSensor(0x90,0x10); //GEUGAIN
  SIV120DWriteCmosSensor(0x91,0x10); //GEDGAIN
  SIV120DWriteCmosSensor(0x92,0x02); //Ucoring
  SIV120DWriteCmosSensor(0x96,0x02); //Dcoring

  // Edge - Luminance
  SIV120DWriteCmosSensor(0x9F,0x0C); //YEUGAIN                    
  SIV120DWriteCmosSensor(0xA0,0x0C); //YEDGAIN                    
  SIV120DWriteCmosSensor(0xA1,0x22); //Yecore [7:4]upper [3:0]down

  SIV120DWriteCmosSensor(0xA9,0x10); //Cr saturation 0x12 
  SIV120DWriteCmosSensor(0xAA,0x10); //Cb saturation 0x12
  //SIV120DWriteCmosSensor(0xAB,0x98); //Brightness  
  SIV120DWriteCmosSensor(0xAB,0x10); //Brightness  //0x04      
  SIV120DWriteCmosSensor(0xAE,0x40); //Hue               
                                        
  SIV120DWriteCmosSensor(0xAF,0x86); //Hue                       
                                     
  // Color suppress
  SIV120DWriteCmosSensor(0xB9,0x10); //Color Suppression Change Start State  0x18     
  SIV120DWriteCmosSensor(0xBA,0x20); //Slope 

  // inverse color space conversion
  SIV120DWriteCmosSensor(0xCC,0x40);
  SIV120DWriteCmosSensor(0xCD,0x00);                          
  SIV120DWriteCmosSensor(0xCE,0x58);                          
  SIV120DWriteCmosSensor(0xCF,0x40);                          
  SIV120DWriteCmosSensor(0xD0,0xEA);
  SIV120DWriteCmosSensor(0xD1,0xD3);
  SIV120DWriteCmosSensor(0xD2,0x40);
  SIV120DWriteCmosSensor(0xD3,0x6F);
  SIV120DWriteCmosSensor(0xD4,0x00);

  // ee/nr                                             
  SIV120DWriteCmosSensor(0xDD,0x39); // ENHCTRL
  SIV120DWriteCmosSensor(0xDE,0xE3); // NOIZCTRL

                
  // Memory speed control                             
  SIV120DWriteCmosSensor(0xE5,0x15);               
  SIV120DWriteCmosSensor(0xE6,0x20);                    
  SIV120DWriteCmosSensor(0xE7,0x04);                  

  // AE on                                                 
  SIV120DSetPage(1);                                                   
  SIV120DWriteCmosSensor(0x10,0x80);

  // Sensor On                                   
  SIV120DSetPage(0);                  
  SIV120DWriteCmosSensor(0x03,0x55);     
}   /* SIV120DInitialSetting */

static void SIV120BInitialSetting(void)
{
  SENSORDB("SIV120BInitialSetting\n");

  // SNR block [Vendor recommended value ##Don't change##]
  SIV120DSetPage(0);
  SIV120DWriteCmosSensor(0x04,0x00);
  SIV120DWriteCmosSensor(0x05,0x07); //VGA Output
  SIV120DWriteCmosSensor(0x10,0x13);
  SIV120DWriteCmosSensor(0x11,0x25);
  SIV120DWriteCmosSensor(0x12,0x21);
  SIV120DWriteCmosSensor(0x13,0x17); // ABS
  SIV120DWriteCmosSensor(0x16,0xCF);
  SIV120DWriteCmosSensor(0x17,0xaa); //Internal LDO On: 0xaa
                     //Internal LDO off: 0x00               
  // SIV120D 50Hz - 24MHz
  SIV120DWriteCmosSensor(0x20,0x00);
  SIV120DWriteCmosSensor(0x21,0x6E);
  SIV120DWriteCmosSensor(0x23,0x1D);
  SIV120DSetPage(1);
  SIV120DWriteCmosSensor(0x34,0x84);

  // Vendor recommended value ##Don't change##
  SIV120DSetPage(0);
  SIV120DWriteCmosSensor(0x40,0x00);
  SIV120DWriteCmosSensor(0x41,0x00);
  SIV120DWriteCmosSensor(0x42,0x00);
  SIV120DWriteCmosSensor(0x43,0x00);
              
  // AE
  SIV120DSetPage(1);
  SIV120DWriteCmosSensor(0x11,0x0A);
  SIV120DWriteCmosSensor(0x12,0x80); // D65 target 0x84
  SIV120DWriteCmosSensor(0x13,0x80); // D30 target
  SIV120DWriteCmosSensor(0x14,0x80); // D20 target

  // Init shutter & gain
  SIV120DWriteCmosSensor(0x1D,0x02); //Initial shutter time
  SIV120DWriteCmosSensor(0x1E,0x28); //Initial analog gain

  SIV120DWriteCmosSensor(0x40,0x70); // Max gain 0x7F  //68
  SIV120DWriteCmosSensor(0x41,0x28);
  SIV120DWriteCmosSensor(0x42,0x28);
  SIV120DWriteCmosSensor(0x43,0x08);
  SIV120DWriteCmosSensor(0x44,0x08);
  SIV120DWriteCmosSensor(0x45,0x09);
  SIV120DWriteCmosSensor(0x46,0x17);
  SIV120DWriteCmosSensor(0x47,0x1D);
  SIV120DWriteCmosSensor(0x48,0x21);
  SIV120DWriteCmosSensor(0x49,0x23);
  SIV120DWriteCmosSensor(0x4A,0x24);
  SIV120DWriteCmosSensor(0x4B,0x26);
  SIV120DWriteCmosSensor(0x4C,0x27);
  SIV120DWriteCmosSensor(0x4D,0x27);
  SIV120DWriteCmosSensor(0x4E,0x1A);
  SIV120DWriteCmosSensor(0x4F,0x14);
  SIV120DWriteCmosSensor(0x50,0x11);
  SIV120DWriteCmosSensor(0x51,0x0F);
  SIV120DWriteCmosSensor(0x52,0x0D);
  SIV120DWriteCmosSensor(0x53,0x0C);
  SIV120DWriteCmosSensor(0x54,0x0A);
  SIV120DWriteCmosSensor(0x55,0x09);

  // AE Window
  SIV120DWriteCmosSensor(0x60,0xFF);
  SIV120DWriteCmosSensor(0x61,0xFF);
  SIV120DWriteCmosSensor(0x62,0xFF);
  SIV120DWriteCmosSensor(0x63,0xFF);
  SIV120DWriteCmosSensor(0x64,0xFF);
  SIV120DWriteCmosSensor(0x65,0xFF);
  SIV120DWriteCmosSensor(0x66,0x00);
  SIV120DWriteCmosSensor(0x67,0x50);
  SIV120DWriteCmosSensor(0x68,0x50);
  SIV120DWriteCmosSensor(0x69,0x50);
  SIV120DWriteCmosSensor(0x6A,0x50);
  SIV120DWriteCmosSensor(0x6B,0x00);
  SIV120DWriteCmosSensor(0x6C,0x06);

  //Anti Saturation
  SIV120DWriteCmosSensor(0x70,0xD4); //Anti Saturation On
  SIV120DWriteCmosSensor(0x77,0xB0);
  SIV120DWriteCmosSensor(0x78,0xB8);
  SIV120DWriteCmosSensor(0x79,0x6E); //Y Level 0x70 for gamma test

  SIV120DWriteCmosSensor(0x90,0xC8);

  // AWB             
  SIV120DSetPage(2);
  SIV120DWriteCmosSensor(0x10,0xD3);
  SIV120DWriteCmosSensor(0x11,0xC0);
  SIV120DWriteCmosSensor(0x12,0x80);
  SIV120DWriteCmosSensor(0x13,0x80);
  SIV120DWriteCmosSensor(0x14,0x7F);
  SIV120DWriteCmosSensor(0x15,0xFE);
  SIV120DWriteCmosSensor(0x16,0x80);
  SIV120DWriteCmosSensor(0x17,0xEA);
  SIV120DWriteCmosSensor(0x18,0x80);
              
  SIV120DWriteCmosSensor(0x19,0xB0);//0xA0
  SIV120DWriteCmosSensor(0x1A,0x50);//0x60
  SIV120DWriteCmosSensor(0x1B,0xB0);//0xA0
  SIV120DWriteCmosSensor(0x1C,0x50);//0x60
  SIV120DWriteCmosSensor(0x1D,0xA0);
  SIV120DWriteCmosSensor(0x1E,0x70);

  SIV120DWriteCmosSensor(0x20,0xE8);
  SIV120DWriteCmosSensor(0x21,0x20);
  SIV120DWriteCmosSensor(0x22,0xA4);
  SIV120DWriteCmosSensor(0x23,0x20);
  SIV120DWriteCmosSensor(0x24,0xFF);
  SIV120DWriteCmosSensor(0x25,0x20);
  SIV120DWriteCmosSensor(0x26,0x0F);
  SIV120DWriteCmosSensor(0x27,0x10);
  SIV120DWriteCmosSensor(0x28,0x1A);
  SIV120DWriteCmosSensor(0x29,0xB8);
  SIV120DWriteCmosSensor(0x2A,0x90);

  SIV120DWriteCmosSensor(0x30,0x00);
  SIV120DWriteCmosSensor(0x31,0x10);
  SIV120DWriteCmosSensor(0x32,0x00);
  SIV120DWriteCmosSensor(0x33,0x10);
  SIV120DWriteCmosSensor(0x34,0x02);
  SIV120DWriteCmosSensor(0x35,0x76);
  SIV120DWriteCmosSensor(0x36,0x01);
  SIV120DWriteCmosSensor(0x37,0xD6);
  SIV120DWriteCmosSensor(0x40,0x01);
  SIV120DWriteCmosSensor(0x41,0x04);
  SIV120DWriteCmosSensor(0x42,0x08);
  SIV120DWriteCmosSensor(0x43,0x10);
  SIV120DWriteCmosSensor(0x44,0x12);
  SIV120DWriteCmosSensor(0x45,0x35);
  SIV120DWriteCmosSensor(0x46,0x64);
  SIV120DWriteCmosSensor(0x50,0x33);
  SIV120DWriteCmosSensor(0x51,0x20);
  SIV120DWriteCmosSensor(0x52,0xE5);
  SIV120DWriteCmosSensor(0x53,0xFB);
  SIV120DWriteCmosSensor(0x54,0x13);
  SIV120DWriteCmosSensor(0x55,0x26);
  SIV120DWriteCmosSensor(0x56,0x07);
  SIV120DWriteCmosSensor(0x57,0xF5);
  SIV120DWriteCmosSensor(0x58,0xEA);
  SIV120DWriteCmosSensor(0x59,0x21);

  //CMA change  -D65~A
  SIV120DWriteCmosSensor(0x63,0x9D); //D30 to D20 for R
  SIV120DWriteCmosSensor(0x64,0xBE); //D30 to D20 for B
  SIV120DWriteCmosSensor(0x65,0x9D); //D20 to D30 for R
  SIV120DWriteCmosSensor(0x66,0xBE); //D20 to D30 for B
  SIV120DWriteCmosSensor(0x67,0xC2); //D65 to D30 for R
  SIV120DWriteCmosSensor(0x68,0x9C); //D65 to D30 for B
  SIV120DWriteCmosSensor(0x69,0xC2); //D30 to D65 for R
  SIV120DWriteCmosSensor(0x6A,0x9C); //D30 to D65 for B

  // IDP            
  SIV120DSetPage(3);
  SIV120DWriteCmosSensor(0x10,0xFF); // IDP function enable
  SIV120DWriteCmosSensor(0x11,0x1D); // PCLK polarity
  SIV120DWriteCmosSensor(0x12,0xDD); // Y,Cb,Cr order sequence
  SIV120DWriteCmosSensor(0x8C,0x18); // Color matrix select at dark condition

  // DPCNR            
  SIV120DWriteCmosSensor(0x17,0xB8);
  SIV120DWriteCmosSensor(0x18,0x18);
  SIV120DWriteCmosSensor(0x19,0x48);
  SIV120DWriteCmosSensor(0x1A,0x48);
  SIV120DWriteCmosSensor(0x1B,0x3F);
  SIV120DWriteCmosSensor(0x1C,0x10);
  SIV120DWriteCmosSensor(0x1D,0x60);
  SIV120DWriteCmosSensor(0x1E,0x60);
  SIV120DWriteCmosSensor(0x1F,0x4F);
  SIV120DWriteCmosSensor(0x20,0x03); // Normal illumiinfo start
  SIV120DWriteCmosSensor(0x21,0x0F); // Dark illumiinfo start

  // Gamma
  SIV120DWriteCmosSensor(0x30,0x00);
  SIV120DWriteCmosSensor(0x31,0x08);
  SIV120DWriteCmosSensor(0x32,0x0F);
  SIV120DWriteCmosSensor(0x33,0x20);
  SIV120DWriteCmosSensor(0x34,0x3B);
  SIV120DWriteCmosSensor(0x35,0x52);
  SIV120DWriteCmosSensor(0x36,0x66);
  SIV120DWriteCmosSensor(0x37,0x78);
  SIV120DWriteCmosSensor(0x38,0x89);
  SIV120DWriteCmosSensor(0x39,0x98);
  SIV120DWriteCmosSensor(0x3A,0xA4);
  SIV120DWriteCmosSensor(0x3B,0xBA);
  SIV120DWriteCmosSensor(0x3C,0xD4);
  SIV120DWriteCmosSensor(0x3D,0xEC);
  SIV120DWriteCmosSensor(0x3E,0xF6);
  SIV120DWriteCmosSensor(0x3F,0xFF);

  
  // Shading
  SIV120DWriteCmosSensor(0x40,0x00);
  SIV120DWriteCmosSensor(0x41,0xDC);
  SIV120DWriteCmosSensor(0x42,0xCB);
  SIV120DWriteCmosSensor(0x43,0xA8);
  SIV120DWriteCmosSensor(0x44,0x75);
  SIV120DWriteCmosSensor(0x45,0x31);
  SIV120DWriteCmosSensor(0x46,0x63);//0x44
  SIV120DWriteCmosSensor(0x47,0x45);//0x63
  SIV120DWriteCmosSensor(0x48,0x63);//0x44
  SIV120DWriteCmosSensor(0x49,0x43);//0x62
  SIV120DWriteCmosSensor(0x4A,0x63);//0x34
  SIV120DWriteCmosSensor(0x4B,0x42);//0x41
  SIV120DWriteCmosSensor(0x4C,0x52);//0x23
  SIV120DWriteCmosSensor(0x4D,0x31);//0x40
  SIV120DWriteCmosSensor(0x4E,0x04);
  SIV120DWriteCmosSensor(0x4F,0x44);//0x44
  SIV120DWriteCmosSensor(0x50,0xFE);//0xF6
  SIV120DWriteCmosSensor(0x51,0x80);
  SIV120DWriteCmosSensor(0x52,0x00);
  SIV120DWriteCmosSensor(0x53,0x00);
  SIV120DWriteCmosSensor(0x54,0x00);
  SIV120DWriteCmosSensor(0x55,0x00);

  // Lowlux Shading
  SIV120DWriteCmosSensor(0x56,0x10);
  SIV120DWriteCmosSensor(0x57,0xDA);
  SIV120DWriteCmosSensor(0x58,0xFF);

  //Filter
  SIV120DWriteCmosSensor(0x60,0x2F);
  SIV120DWriteCmosSensor(0x61,0xB7);
  SIV120DWriteCmosSensor(0x62,0xB7);
  SIV120DWriteCmosSensor(0x63,0xB7);
  SIV120DWriteCmosSensor(0x64,0x0C);
  SIV120DWriteCmosSensor(0x65,0xFF);
  SIV120DWriteCmosSensor(0x66,0x08);
  SIV120DWriteCmosSensor(0x67,0xFF);
  SIV120DWriteCmosSensor(0x68,0x00);
  SIV120DWriteCmosSensor(0x69,0x00);
  SIV120DWriteCmosSensor(0x6A,0x00);
  SIV120DWriteCmosSensor(0x6B,0x00);
  SIV120DWriteCmosSensor(0x6C,0xA0);
  SIV120DWriteCmosSensor(0x6D,0x10);
  SIV120DWriteCmosSensor(0x6E,0x08);
  SIV120DWriteCmosSensor(0x6F,0x18);

  // Color Matrix (D65) - Daylight
  SIV120DWriteCmosSensor(0x71,0x3E);
  SIV120DWriteCmosSensor(0x72,0xBD);
  SIV120DWriteCmosSensor(0x73,0x05);
  SIV120DWriteCmosSensor(0x74,0x0F);
  SIV120DWriteCmosSensor(0x75,0x1E);
  SIV120DWriteCmosSensor(0x76,0x13);
  SIV120DWriteCmosSensor(0x77,0xF3);
  SIV120DWriteCmosSensor(0x78,0xCC);
  SIV120DWriteCmosSensor(0x79,0x41);

  // Color Matrix (D30) - CWF
  SIV120DWriteCmosSensor(0x7A,0x3D);
  SIV120DWriteCmosSensor(0x7B,0xC4);
  SIV120DWriteCmosSensor(0x7C,0xFF);
  SIV120DWriteCmosSensor(0x7D,0x10);
  SIV120DWriteCmosSensor(0x7E,0x1D);
  SIV120DWriteCmosSensor(0x7F,0x13);
  SIV120DWriteCmosSensor(0x80,0xF2);
  SIV120DWriteCmosSensor(0x81,0xC4);
  SIV120DWriteCmosSensor(0x82,0x4A);

  // Color Matrix (D20) - A
  SIV120DWriteCmosSensor(0x83,0x3C);
  SIV120DWriteCmosSensor(0x84,0xC2);
  SIV120DWriteCmosSensor(0x85,0x01);
  SIV120DWriteCmosSensor(0x86,0x10);
  SIV120DWriteCmosSensor(0x87,0x1F);
  SIV120DWriteCmosSensor(0x88,0x0F);
  SIV120DWriteCmosSensor(0x89,0xF6);
  SIV120DWriteCmosSensor(0x8A,0xD2);
  SIV120DWriteCmosSensor(0x8B,0x39);
              
  // Edge - Green
  SIV120DWriteCmosSensor(0x90,0x10); //Up Gain
  SIV120DWriteCmosSensor(0x91,0x10); //Down Gain
  SIV120DWriteCmosSensor(0x92,0x04);
  SIV120DWriteCmosSensor(0x93,0x12);
  SIV120DWriteCmosSensor(0x94,0xFF);
  SIV120DWriteCmosSensor(0x95,0x20);
  SIV120DWriteCmosSensor(0x96,0x04);
  SIV120DWriteCmosSensor(0x97,0x12);
  SIV120DWriteCmosSensor(0x98,0xFF);
  SIV120DWriteCmosSensor(0x99,0x20);
  SIV120DWriteCmosSensor(0x9A,0x20);
  SIV120DWriteCmosSensor(0x9B,0x20);
  SIV120DWriteCmosSensor(0x9C,0x1A);
  SIV120DWriteCmosSensor(0x9D,0x40);
  SIV120DWriteCmosSensor(0x9E,0x00);

  // Edge - Luminance
  SIV120DWriteCmosSensor(0x9F,0x00);
  SIV120DWriteCmosSensor(0xA0,0x00);
  SIV120DWriteCmosSensor(0xA1,0x02);
  SIV120DWriteCmosSensor(0xA2,0x02);
  SIV120DWriteCmosSensor(0xA3,0x04);
  SIV120DWriteCmosSensor(0xA4,0x20);
  SIV120DWriteCmosSensor(0xA5,0x1A);
  SIV120DWriteCmosSensor(0xA6,0x40);
  SIV120DWriteCmosSensor(0xA7,0x00);

  // YCbCr Gain, Brightness, Contrast
  SIV120DWriteCmosSensor(0xA8,0x10);
  SIV120DWriteCmosSensor(0xA9,0x15); //a9 aa  0813 
  SIV120DWriteCmosSensor(0xAA,0x15);
  //SIV120DWriteCmosSensor(0xAB,0x20);//by lingnan      //LLN::FP CamWriteCmosSensor(0xAB,0x04);
  SIV120DWriteCmosSensor(0xAB,0x04);
  SIV120DWriteCmosSensor(0xAC,0x10);

  SIV120DWriteCmosSensor(0xB0,0xFF); //Y Top
  SIV120DWriteCmosSensor(0xB1,0x00); //Y Bottom
  SIV120DWriteCmosSensor(0xB2,0xFF); 
  SIV120DWriteCmosSensor(0xB3,0x00);
  SIV120DWriteCmosSensor(0xB4,0xFF);
  SIV120DWriteCmosSensor(0xB5,0x00);
  SIV120DWriteCmosSensor(0xB6,0x00);

  // Color suppress           
  SIV120DWriteCmosSensor(0xB9,0x1A); //Ilimininfo Start0 x1A
  SIV120DWriteCmosSensor(0xBA,0x30); //Slope

  // Window
  SIV120DWriteCmosSensor(0xC0,0x24); 
  SIV120DWriteCmosSensor(0xC1,0x00);
  SIV120DWriteCmosSensor(0xC2,0x80);
  SIV120DWriteCmosSensor(0xC3,0x00);
  SIV120DWriteCmosSensor(0xC4,0xE0);

  SIV120DWriteCmosSensor(0xDD,0x4F); // ENHCTRL
  SIV120DWriteCmosSensor(0xDE,0xBA); // NOIZCTRL
  SIV120DWriteCmosSensor(0xDF,0x28);
  SIV120DWriteCmosSensor(0xE0,0x70);
  SIV120DWriteCmosSensor(0xE1,0x90);
  SIV120DWriteCmosSensor(0xE2,0x08);
  SIV120DWriteCmosSensor(0xE3,0x10);
  SIV120DWriteCmosSensor(0xE4,0x40);

  // Memory speed control
  SIV120DWriteCmosSensor(0xE5,0x15);
  SIV120DWriteCmosSensor(0xE6,0x28);
  SIV120DWriteCmosSensor(0xE7,0x04); 

  // AE on
  SIV120DSetPage(1);
  SIV120DWriteCmosSensor(0x10,0x80);

  // Sensor on
  SIV120DSetPage(0);
  SIV120DWriteCmosSensor(0x03,0xA5); //0x55
}

/*************************************************************************
* FUNCTION
*    SIV120DHalfAdjust
*
* DESCRIPTION
*    This function dividend / divisor and use round-up.
*
* PARAMETERS
*    dividend
*    divisor
*
* RETURNS
*    [dividend / divisor]
*
* LOCAL AFFECTED
*
*************************************************************************/
__inline static kal_uint32 SIV120DHalfAdjust(kal_uint32 dividend, kal_uint32 divisor)
{
  return (dividend * 2 + divisor) / (divisor * 2); /* that is [dividend / divisor + 0.5]*/
}

/*************************************************************************
* FUNCTION
*   SIV120DSetShutterStep
*
* DESCRIPTION
*   This function is to calculate & set shutter step register .
*
*************************************************************************/
static void SIV120DSetShutterStep(void)
{       
  const kal_uint8 banding = SIV120DCurrentStatus.iBanding == AE_FLICKER_MODE_50HZ ? SIV120D_NUM_50HZ : SIV120D_NUM_60HZ;
  const kal_uint16 shutter_step = SIV120DHalfAdjust(SIV120DCurrentStatus.iPclk * SIV120D_CLK_1MHZ / 2, (SIV120DCurrentStatus.iHblank + SIV120D_PERIOD_PIXEL_NUMS) * banding);

  if(SIV120DCurrentStatus.iShutterStep == shutter_step)
    return ;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iShutterStep = shutter_step;
  spin_unlock(&siv120d_drv_lock);
  
  ASSERT(shutter_step <= 0xFF);    
  /* Block 1:0x34  shutter step*/
  SIV120DSetPage(1);
  SIV120DWriteCmosSensor(0x34,shutter_step);

  SENSORDB("Set Shutter Step:%x\n",shutter_step);
}/* SIV120DSetShutterStep */

/*************************************************************************
* FUNCTION
*   SIV120DSetFrameCount
*
* DESCRIPTION
*   This function is to set frame count register .
*
*************************************************************************/
static void SIV120DSetFrameCount(void)
{    
  kal_uint16 Frame_Count,min_fps = 100;
  kal_uint8 banding = SIV120DCurrentStatus.iBanding == AE_FLICKER_MODE_50HZ ? SIV120D_NUM_50HZ : SIV120D_NUM_60HZ;

  min_fps = SIV120DCurrentStatus.bNightMode ? SIV120DCurrentStatus.MinFpsNight : SIV120DCurrentStatus.MinFpsNormal;
  Frame_Count = banding * SIV120D_FRAME_RATE_UNIT / min_fps;

  if(SIV120DCurrentStatus.iFrameCount == Frame_Count)
    return ;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iFrameCount = Frame_Count;
  spin_unlock(&siv120d_drv_lock);  

  SENSORDB("min_fps:%d,Frame_Count:%x\n",min_fps/SIV120D_FRAME_RATE_UNIT,Frame_Count);
  /*Block 01: 0x11  Max shutter step,for Min frame rate */
  SIV120DSetPage(1);
  SIV120DWriteCmosSensor(0x11,Frame_Count&0xFF);    
}/* SIV120DSetFrameCount */

/*************************************************************************
* FUNCTION
*   SIV120DConfigBlank
*
* DESCRIPTION
*   This function is to set Blank size for Preview mode .
*
* PARAMETERS
*   iBlank: target HBlank size
*      iHz: banding frequency
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void SIV120DConfigBlank(kal_uint16 hblank,kal_uint16 vblank)
{    
  SENSORDB("hblank:%x,vblank:%x\n",hblank,vblank);
   /********************************************    
    *   Register :0x20 - 0x22
    *  Block 00
    *  0x20  [7:4]:HBANK[9:8]; 0x20  [3:0]:VBANK[9:8]
    *  0x21 HBANK[7:0]
    *  0x23 VBANK[7:0]  
    ********************************************/
  if((SIV120DCurrentStatus.iHblank == hblank) && (SIV120DCurrentStatus.iVblank == vblank) )
     return ;

  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iHblank = hblank;
  SIV120DCurrentStatus.iVblank = vblank;
  spin_unlock(&siv120d_drv_lock); 
   
  ASSERT(hblank <= SIV120D_BLANK_REGISTER_LIMITATION && vblank <= SIV120D_BLANK_REGISTER_LIMITATION);
  SIV120DSetPage(0);
  SIV120DWriteCmosSensor(0x20,((hblank>>4)&0xF0)|((vblank>>8)&0x0F));
  SIV120DWriteCmosSensor(0x21,hblank & 0xFF);
  SIV120DWriteCmosSensor(0x23,vblank & 0xFF);
  SIV120DSetShutterStep();
}   /* SIV120DConfigBlank */

/*************************************************************************
* FUNCTION
*    SIV120DCalFps
*
* DESCRIPTION
*    This function calculate & set frame rate and fix frame rate when video mode
*    MUST BE INVOKED AFTER SIM120C_preview() !!!
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void SIV120DCalFps(void)
{
  kal_uint16 Line_length,Dummy_Line,Dummy_Pixel;
  kal_uint16 max_fps = 300;

  Line_length = SIV120DCurrentStatus.iDummyPixel + SIV120D_PERIOD_PIXEL_NUMS; 

  if (SIV120DCurrentStatus.bVideoMode == KAL_TRUE)
  {    
    max_fps = SIV120DCurrentStatus.bNightMode ? SIV120DCurrentStatus.MinFpsNight: SIV120DCurrentStatus.MinFpsNormal;
  }
  else
  {    
    max_fps = SIV120D_FPS(25);//FAE: 30 cause flicker
  }

  Dummy_Line = SIV120DCurrentStatus.iPclk * SIV120D_CLK_1MHZ * SIV120D_FRAME_RATE_UNIT / (2 * Line_length * max_fps) - SIV120D_PERIOD_LINE_NUMS; 
  if(Dummy_Line > SIV120D_BLANK_REGISTER_LIMITATION)
  {
    Dummy_Line = SIV120D_BLANK_REGISTER_LIMITATION;
    Line_length = SIV120DCurrentStatus.iPclk * SIV120D_CLK_1MHZ * SIV120D_FRAME_RATE_UNIT / (2 * (Dummy_Line + SIV120D_PERIOD_LINE_NUMS) * max_fps);
  }
  Dummy_Pixel = Line_length -  SIV120D_PERIOD_PIXEL_NUMS;

  SENSORDB("max_fps:%d\n",max_fps/SIV120D_FRAME_RATE_UNIT);
  SENSORDB("Dummy Pixel:%x,Hblank:%x,Vblank:%x\n",SIV120DCurrentStatus.iDummyPixel,Dummy_Pixel,Dummy_Line);
  SIV120DConfigBlank((Dummy_Pixel > 0) ? Dummy_Pixel : 0, (Dummy_Line > 0) ? Dummy_Line : 0);
  SIV120DSetShutterStep();
}


/*************************************************************************
* FUNCTION
*   SIV120DFixFrameRate
*
* DESCRIPTION
*   This function fix the frame rate of image sensor.
*
*************************************************************************/
static void SIV120DFixFrameRate(kal_bool bEnable)
{
  if(SIV120DCurrentStatus.bFixFrameRate == bEnable)
    return ;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.bFixFrameRate = bEnable;
  if(bEnable == KAL_TRUE)
  {   //fix frame rate
    SIV120DCurrentStatus.iControl |= 0xC0;
  }
  else
  {        
    SIV120DCurrentStatus.iControl &= 0x3F;
  }
  spin_unlock(&siv120d_drv_lock);
  
  SIV120DSetPage(0);
  SIV120DWriteCmosSensor(0x04,SIV120DCurrentStatus.iControl);
}   /* SIV120DFixFrameRate */

/*************************************************************************
* FUNCTION
*   SIV120DHVmirror
*
* DESCRIPTION
*   This function config the HVmirror of image sensor.
*
*************************************************************************/
static void SIV120DHVmirror(kal_uint8 HVmirrorType)
{    
  if(SIV120DCurrentStatus.iMirror == HVmirrorType)
    return ;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iMirror = HVmirrorType;
  SIV120DCurrentStatus.iControl = SIV120DCurrentStatus.iControl & 0xFC;  
  switch (HVmirrorType) 
  {
    case IMAGE_H_MIRROR:
      SIV120DCurrentStatus.iControl |= 0x01;
      break;
    case IMAGE_V_MIRROR:
      SIV120DCurrentStatus.iControl |= 0x02;
      break;
    case IMAGE_HV_MIRROR:
      SIV120DCurrentStatus.iControl |= 0x03;
      break;
    case IMAGE_NORMAL:
    default:
      SIV120DCurrentStatus.iControl |= 0x00;
  }
  spin_unlock(&siv120d_drv_lock);
  
  SIV120DSetPage(0);
  SIV120DWriteCmosSensor(0x04,SIV120DCurrentStatus.iControl);
}   /* SIV120DHVmirror */

/*************************************************************************
* FUNCTION
*  SIV120DNightMode
*
* DESCRIPTION
*  This function night mode of SIV120D.
*
* PARAMETERS
*  none
*
* RETURNS
*  None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void SIV120DNightMode(kal_bool enable)
{
  SENSORDB("NightMode %d\n",enable);

  if (enable == SIV120DCurrentStatus.bNightMode)
    return ;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.bNightMode = enable;
  spin_unlock(&siv120d_drv_lock);

  if ( SIV120DCurrentStatus.bVideoMode == KAL_TRUE)// camera mode
    return ;
  
  if (SIV120DCurrentStatus.bNightMode == KAL_TRUE)
  {   /* camera night mode */
    SENSORDB("camera night mode\n");
   
    SIV120DSetPage(1);
    SIV120DWriteCmosSensor(0x40,0x6C); //Max Analog Gain Value @ Shutter step = Max Shutter step  0x7D
    SIV120DSetPage(3);
    SIV120DWriteCmosSensor(0xAB,0x10); //Brightness Control 0x11
    SIV120DWriteCmosSensor(0xB9,0x18); //Color Suppression Change Start State  0x18           
    SIV120DWriteCmosSensor(0xBA,0x32); //Slope

  }
  else
  {   /* camera normal mode */
    SENSORDB("camera normal mode\n");

    SIV120DSetPage(1);
    SIV120DWriteCmosSensor(0x40,0x40);// 0x7F
    SIV120DSetPage(3);
    SIV120DWriteCmosSensor(0xAB,0x00); //0x04 
    SIV120DWriteCmosSensor(0xB9,0x18);            
    SIV120DWriteCmosSensor(0xBA,0x34); //Slope

  }
  SIV120DSetFrameCount(); 
}  /* SIV120DNightMode */

/*************************************************************************
* FUNCTION
*  SIV120DOpen
*
* DESCRIPTION
*  This function initialize the registers of CMOS sensor
*
* PARAMETERS
*  None
*
* RETURNS
*  None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SIV120DOpen(void)
{

  kal_uint16 sensor_id=0;
  kal_uint16 temp_data;

  SENSORDB("SIV120D Sensor open\n");

  //  Read sensor ID to adjust I2C is OK?
    SIV120DWriteCmosSensor(0x00,0x00);
  sensor_id = SIV120DReadCmosSensor(0x01);
  SENSORDB("IV120D Sensor Read ID %x\n",sensor_id);
  if (sensor_id != SIV120B_SENSOR_ID) 
  {
    return ERROR_SENSOR_CONNECT_FAIL;
  }
  
  temp_data = SIV120DReadCmosSensor(0x02);
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iSensorVersion  = temp_data;  
  spin_unlock(&siv120d_drv_lock);
  
  SENSORDB("SIV120B Sensor Version %x\n",SIV120DCurrentStatus.iSensorVersion);
  //SIV120D_set_isp_driving_current(3);
  SIV120DInitialPara();
  if (SIV120DCurrentStatus.iSensorVersion == SIV120B_SENSOR_VERSION) 
  {
    SIV120BInitialSetting();
  }
  else
  {
    SIV120DInitialSetting();
  }
    SIV120DCalFps();    

  return ERROR_NONE;
}  /* SIV120DOpen() */

/*************************************************************************
* FUNCTION
*  SIV120DClose
*
* DESCRIPTION
*  This function is to turn off sensor module power.
*
* PARAMETERS
*  None
*
* RETURNS
*  None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SIV120DClose(void)
{
//  CISModulePowerOn(FALSE);
  return ERROR_NONE;
}  /* SIV120DClose() */

/*************************************************************************
* FUNCTION
*  SIV120DPreview
*
* DESCRIPTION
*  This function start the sensor preview.
*
* PARAMETERS
*  *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*  None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 SIV120DPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
            MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

  SENSORDB("SIV120DPreview\r\n");
    /* ==Camera Preview, MT6516 use 26MHz PCLK, 30fps == */
  SENSORDB("Camera preview\r\n");
  //4  <1> preview of capture PICTURE
  SIV120DFixFrameRate(KAL_FALSE);

  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.MinFpsNormal = SIV120D_FPS(10);
  SIV120DCurrentStatus.MinFpsNight =  SIV120DCurrentStatus.MinFpsNormal >> 1;  
  SIV120DCurrentStatus.bVideoMode =  KAL_FALSE;
  spin_unlock(&siv120d_drv_lock);	
  
  //4 <2> set mirror and flip
  SIV120DHVmirror(sensor_config_data->SensorImageMirror);

  //4 <3> set dummy pixel, dummy line will calculate from frame rate
  /* SIV120DCurrentStatus.iDummyPixel = 0x1d; */    
  
  // copy sensor_config_data
  memcpy(&SIV120DSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));


    // AE shutter 
  SIV120DSetPage(1);
  printk("siv120b  0x11=0x%x\n",SIV120DReadCmosSensor(0x11));
  SIV120DWriteCmosSensor(0x11,0x25);
 
  
    return ERROR_NONE;
}  /* SIV120DPreview() */

UINT32 SIV120DGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
  pSensorResolution->SensorFullWidth = SIV120D_IMAGE_SENSOR_PV_WIDTH+2; //add workaround for VGA sensor
  pSensorResolution->SensorFullHeight = SIV120D_IMAGE_SENSOR_PV_HEIGHT;
  pSensorResolution->SensorPreviewWidth = SIV120D_IMAGE_SENSOR_PV_WIDTH;
  pSensorResolution->SensorPreviewHeight = SIV120D_IMAGE_SENSOR_PV_HEIGHT;
  return ERROR_NONE;
}  /* SIV120DGetResolution() */

UINT32 SIV120DGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
            MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
            MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
  pSensorInfo->SensorPreviewResolutionX = SIV120D_IMAGE_SENSOR_PV_WIDTH;
  pSensorInfo->SensorPreviewResolutionY = SIV120D_IMAGE_SENSOR_PV_HEIGHT;
  pSensorInfo->SensorFullResolutionX = SIV120D_IMAGE_SENSOR_PV_WIDTH;
  pSensorInfo->SensorFullResolutionY = SIV120D_IMAGE_SENSOR_PV_HEIGHT;

  pSensorInfo->SensorCameraPreviewFrameRate=30;
  pSensorInfo->SensorVideoFrameRate=30;
  pSensorInfo->SensorStillCaptureFrameRate=30;
  pSensorInfo->SensorWebCamCaptureFrameRate=30;
  pSensorInfo->SensorResetActiveHigh=FALSE;
  pSensorInfo->SensorResetDelayCount=1;
  pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_VYUY;
  pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorInterruptDelayLines = 1;
  pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;
  pSensorInfo->SensorMasterClockSwitch = 0; 
  pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_4MA;
  pSensorInfo->CaptureDelayFrame = 1; 
  pSensorInfo->PreviewDelayFrame = 3; 
  pSensorInfo->VideoDelayFrame = 5; 
  switch (ScenarioId)
  {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
    default:      
      pSensorInfo->SensorClockFreq=26;
      pSensorInfo->SensorClockDividCount=  3;
      pSensorInfo->SensorClockRisingCount= 0;
      pSensorInfo->SensorClockFallingCount= 2;
      pSensorInfo->SensorPixelClockCount= 3;
      pSensorInfo->SensorDataLatchCount= 2;
      pSensorInfo->SensorGrabStartX = 1; 
      pSensorInfo->SensorGrabStartY = 1;       
      break;
  }
  memcpy(pSensorConfigData, &SIV120DSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
  return ERROR_NONE;
}  /* SIV120DGetInfo() */

UINT32 SIV120DControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
            MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
  switch (ScenarioId)
  {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
    default:
      SIV120DPreview(pImageWindow, pSensorConfigData);
      break;
  }
  return TRUE;
}  /* SIV120DControl() */

UINT32 SIV120DGetSensorID(UINT32 *sensorID) 
{
	
	SENSORDB("SIV120DGetSensorID\n");	
	//	Read sensor ID to adjust I2C is OK?
    SIV120DWriteCmosSensor(0x00,0x00);
	*sensorID = SIV120DReadCmosSensor(0x01);
	SENSORDB("SIV120D Sensor Read ID %x\n",*sensorID);
	if (*sensorID != SIV120B_SENSOR_ID) 
	{
      *sensorID=0xFFFFFFFF;
	  return ERROR_SENSOR_CONNECT_FAIL;
	}
	
	return ERROR_NONE;
}
BOOL SIV120DSetParamWB(UINT16 para)
{
  SENSORDB("WB %d\n",para);
  if(SIV120DCurrentStatus.iWB== para)
    return FALSE;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iWB = para;
  spin_unlock(&siv120d_drv_lock);	
   
  SIV120DSetPage(2);
  switch (para)
  {
    case AWB_MODE_OFF:
      SIV120DWriteCmosSensor(0x10, 0x00);
      break;
    case AWB_MODE_AUTO:
      SIV120DWriteCmosSensor(0x10, 0xD3);
      break;
    case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
      SIV120DWriteCmosSensor(0x10, 0x00);
      SIV120DWriteCmosSensor(0x60, 0xD0);
      SIV120DWriteCmosSensor(0x61, 0x88);
      break;
    case AWB_MODE_DAYLIGHT: //sunny
      SIV120DWriteCmosSensor(0x10, 0x00);
      SIV120DWriteCmosSensor(0x60, 0xC2);
      SIV120DWriteCmosSensor(0x61, 0x9E);
      break;
    case AWB_MODE_INCANDESCENT: //office
      SIV120DWriteCmosSensor(0x10, 0x00);
      SIV120DWriteCmosSensor(0x60, 0x98);
      SIV120DWriteCmosSensor(0x61, 0xC8);
      break;
    case AWB_MODE_TUNGSTEN: //home
      SIV120DWriteCmosSensor(0x10, 0x00);
      SIV120DWriteCmosSensor(0x60, 0x90);
      SIV120DWriteCmosSensor(0x61, 0xC0);
      break;
    case AWB_MODE_FLUORESCENT:
      SIV120DWriteCmosSensor(0x10, 0x00);
      SIV120DWriteCmosSensor(0x60, 0xAA);
      SIV120DWriteCmosSensor(0x61, 0xBE);
      break;
    default:
      return FALSE;
  }

  return TRUE;
} /* SIV120DSetParamWB */

BOOL SIV120DSetParamAE(UINT16 para)
{
  SENSORDB("AE %d\n",para);
  if(SIV120DCurrentStatus.iAE== para)
    return FALSE;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iAE = para;
  spin_unlock(&siv120d_drv_lock);
  
  SIV120DSetPage(1);
  if(KAL_TRUE == para)
    SIV120DWriteCmosSensor(0x10,0x80);
  else
    SIV120DWriteCmosSensor(0x10,0x00);
  return TRUE;
} /* SIV120DSetParamAE */

BOOL SIV120DSetParamEffect(UINT16 para)
{
  SENSORDB("Effect %d\n",para);
  if(SIV120DCurrentStatus.iEffect== para)
    return FALSE;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iEffect = para;
  spin_unlock(&siv120d_drv_lock);
  
  SIV120DSetPage(3);
  switch (para)
  {
    case MEFFECT_OFF:
      SIV120DWriteCmosSensor(0xB6, 0x00);
      break;
    case MEFFECT_SEPIA:
      SIV120DWriteCmosSensor(0xB6, 0x80); 
      SIV120DWriteCmosSensor(0xB7, 0x60);
      SIV120DWriteCmosSensor(0xB8, 0xA0);
      break;
    case MEFFECT_NEGATIVE:
      SIV120DWriteCmosSensor(0xB6, 0x20);
      break;
    case MEFFECT_SEPIAGREEN:
      SIV120DWriteCmosSensor(0xB6, 0x80); 
      SIV120DWriteCmosSensor(0xB7, 0x50);
      SIV120DWriteCmosSensor(0xB8, 0x50);
      break;
    case MEFFECT_SEPIABLUE:
      SIV120DWriteCmosSensor(0xB6, 0x80); 
      SIV120DWriteCmosSensor(0xB7, 0xC0);
      SIV120DWriteCmosSensor(0xB8, 0x60);
      break;  
    case MEFFECT_MONO: //B&W
      SIV120DWriteCmosSensor(0xB6, 0x40);
      break;
    default:
      return FALSE;
  }
  return TRUE;
} /* SIV120DSetParamEffect */

BOOL SIV120DSetParamBanding(UINT16 para)
{
  SENSORDB("Banding %d\n",para);
  if(SIV120DCurrentStatus.iBanding== para)
    return TRUE;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iBanding = para;
  spin_unlock(&siv120d_drv_lock);
  
  SIV120DSetShutterStep();
  SIV120DSetFrameCount(); 
  return TRUE;
} /* SIV120DSetParamBanding */

BOOL SIV120DSetParamEV(UINT16 para)
{
  SENSORDB("Exporsure %d\n",para);
  if(SIV120DCurrentStatus.iEV== para)
    return FALSE;
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.iEV = para;  
  spin_unlock(&siv120d_drv_lock);

  SIV120DSetPage(3);
  switch (para)
  {
    case AE_EV_COMP_n13:
      SIV120DWriteCmosSensor(0xAB,0xE0);
      break;
    case AE_EV_COMP_n10:
      SIV120DWriteCmosSensor(0xAB,0xD0);
      break;
    case AE_EV_COMP_n07:
      SIV120DWriteCmosSensor(0xAB,0xC0);
      break;
    case AE_EV_COMP_n03:
      SIV120DWriteCmosSensor(0xAB,0xB0);
      break;
    case AE_EV_COMP_00:
      SIV120DWriteCmosSensor(0xAB,0x00);
      break;
    case AE_EV_COMP_03:
      SIV120DWriteCmosSensor(0xAB,0x08);
      break;
    case AE_EV_COMP_07:
      SIV120DWriteCmosSensor(0xAB,0x10);
      break;
    case AE_EV_COMP_10:
      SIV120DWriteCmosSensor(0xAB,0x20);
      break;
    case AE_EV_COMP_13:
      SIV120DWriteCmosSensor(0xAB,0x30);
      break;
    default:
      return FALSE;
  }
  return TRUE;
} /* SIV120DSetParamEV */

UINT32 SIV120DYUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
  switch (iCmd) {
    case FID_SCENE_MODE:      
      if (iPara == SCENE_MODE_OFF)
      {
          SIV120DNightMode(FALSE); 
      }
      else if (iPara == SCENE_MODE_NIGHTSCENE)
      {
           SIV120DNightMode(TRUE); 
      }      
      break;      
    case FID_AWB_MODE:
      SIV120DSetParamWB(iPara);
      break;      
    case FID_COLOR_EFFECT:
      SIV120DSetParamEffect(iPara);
      break;      
    case FID_AE_EV:
      SIV120DSetParamEV(iPara);
      break;      
    case FID_AE_FLICKER:
      SIV120DSetParamBanding(iPara);
      break;      
    case FID_AE_SCENE_MODE: 
      SIV120DSetParamAE(iPara);
       break;       
    default:
      break;
  }
  return TRUE;
}   /* SIV120DYUVSensorSetting */

UINT32 SIV120DYUVSetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("SetVideoMode %d\n",u2FrameRate);

  SIV120DFixFrameRate(KAL_TRUE);
  
  spin_lock(&siv120d_drv_lock);
  SIV120DCurrentStatus.bVideoMode = KAL_TRUE;
  SIV120DCurrentStatus.MinFpsNormal = SIV120D_FPS(25);//FAE: 30 cause flicker
  SIV120DCurrentStatus.MinFpsNight =  SIV120DCurrentStatus.MinFpsNormal >> 1; 
  spin_unlock(&siv120d_drv_lock);
  
  if (u2FrameRate == 25)
  {
    spin_lock(&siv120d_drv_lock);
    SIV120DCurrentStatus.bNightMode = KAL_FALSE;
	spin_unlock(&siv120d_drv_lock);
  }
  else if (u2FrameRate == 12)       
  {
  //VideoCamera.java 
  //(mIsVideoNightMode ? mProfile.videoBitRate / 2 : mProfile.videoBitRate)
    spin_lock(&siv120d_drv_lock);
    SIV120DCurrentStatus.bNightMode = KAL_TRUE;
	spin_unlock(&siv120d_drv_lock);
  }
  else 
  {
    printk("Wrong frame rate setting \n");
  return FALSE;
  }   
  SIV120DSetPage(1);
  SIV120DWriteCmosSensor(0x40,0x58);//Max Analog Gain Value @ Shutter step = Max Shutter step  0x7D
  SIV120DCalFps();    
  SIV120DSetFrameCount();    
  return TRUE;
}

UINT32 SIV120DYUVSetSoftwarePWDNMode(kal_bool bEnable)
{
    SENSORDB("[SIV120DYUVSetSoftwarePWDNMode] Software Power down enable:%d\n", bEnable);
    
    if(bEnable) {   // enable software power down mode   
	 SIV120DWriteCmosSensor(0x03, 0x02);
    } else {
        SIV120DWriteCmosSensor(0x03, 0x55);  
    }
    return TRUE;
}

void SIV120DGetAFMaxNumFocusAreas(UINT32 *pFeatureReturnPara32)
{	
    *pFeatureReturnPara32 = 0;    
    SENSORDB(" *pFeatureReturnPara32 = %d\n",  *pFeatureReturnPara32);

}

UINT32 SIV120DFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
               UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
  UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
  UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
  UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
  UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
  MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
  
  switch (FeatureId)
  {
    case SENSOR_FEATURE_GET_RESOLUTION:
      *pFeatureReturnPara16++=SIV120D_IMAGE_SENSOR_PV_WIDTH;
      *pFeatureReturnPara16=SIV120D_IMAGE_SENSOR_PV_HEIGHT;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_GET_PERIOD:
      *pFeatureReturnPara16++=SIV120D_PERIOD_PIXEL_NUMS+SIV120DCurrentStatus.iHblank;
      *pFeatureReturnPara16=SIV120D_PERIOD_LINE_NUMS+SIV120DCurrentStatus.iVblank;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
      *pFeatureReturnPara32 = SIV120DCurrentStatus.iPclk;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_SET_ESHUTTER:
      break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
      SIV120DNightMode((BOOL) *pFeatureData16);
      break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
      break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
      break;
    case SENSOR_FEATURE_SET_REGISTER:
      SIV120DWriteCmosSensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
      break;
    case SENSOR_FEATURE_GET_REGISTER:
      pSensorRegData->RegData = SIV120DReadCmosSensor(pSensorRegData->RegAddr);
      break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
      memcpy(pSensorConfigData, &SIV120DSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
      *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
      break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:

    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
      break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
      // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
      // if EEPROM does not exist in camera module.
      *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_SET_YUV_CMD:
      SIV120DYUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
      break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:
      SIV120DYUVSetVideoMode(*pFeatureData16);
      break;
    case SENSOR_FEATURE_SET_SOFTWARE_PWDN:
      SIV120DYUVSetSoftwarePWDNMode((BOOL)*pFeatureData16);        	        	
      break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
	    SIV120DGetSensorID(pFeatureReturnPara32); 
	    break; 
    case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
        SIV120DGetAFMaxNumFocusAreas(pFeatureReturnPara32);            
        *pFeatureParaLen=4;
        break; 
    default:
      break;      
  }
  return ERROR_NONE;
}  /* SIV120DFeatureControl() */

  static SENSOR_FUNCTION_STRUCT  SensorFuncSIV120D=
  {
    SIV120DOpen,
    SIV120DGetInfo,
    SIV120DGetResolution,
    SIV120DFeatureControl,
    SIV120DControl,
    SIV120DClose
  };

UINT32 SIV120B_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncSIV120D;

	return ERROR_NONE;
}	/* SensorInit() */
