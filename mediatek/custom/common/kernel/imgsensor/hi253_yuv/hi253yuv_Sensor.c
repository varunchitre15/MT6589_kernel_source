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
#include <asm/io.h>
#include <asm/system.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "hi253yuv_Sensor.h"
#include "hi253yuv_Camera_Sensor_para.h"
#include "hi253yuv_CameraCustomized.h" 

#define HI253YUV_DEBUG
#ifdef HI253YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

struct
{
  kal_bool    NightMode;
  kal_uint8   ZoomFactor; /* Zoom Index */
  kal_uint16  Banding;
  kal_uint32  PvShutter;
  kal_uint32  PvDummyPixels;
  kal_uint32  PvDummyLines;
  kal_uint32  CapDummyPixels;
  kal_uint32  CapDummyLines;
  kal_uint32  PvOpClk;
  kal_uint32  CapOpClk;
  
  /* Video frame rate 300 means 30.0fps. Unit Multiple 10. */
  kal_uint32  MaxFrameRate; 
  kal_uint32  MiniFrameRate; 
  /* Sensor Register backup. */
  kal_uint8   VDOCTL2; /* P0.0x11. */
  kal_uint8   ISPCTL3; /* P10.0x12. */
  kal_uint8   AECTL1;  /* P20.0x10. */
  kal_uint8   AWBCTL1; /* P22.0x10. */
} HI253Status;

static DEFINE_SPINLOCK(hi253_drv_lock);


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

#define Sleep(ms) mdelay(ms)

kal_uint16 HI253WriteCmosSensor(kal_uint32 Addr, kal_uint32 Para)
{
  char pSendCmd[2] = {(char)(Addr & 0xFF) ,(char)(Para & 0xFF)};

  iWriteRegI2C(pSendCmd , 2,HI253_WRITE_ID);
}

kal_uint16 HI253ReadCmosSensor(kal_uint32 Addr)
{
  char pGetByte=0;
  char pSendCmd = (char)(Addr & 0xFF);
  
  iReadRegI2C(&pSendCmd , 1, &pGetByte,1,HI253_WRITE_ID);
  
  return pGetByte;
}

void HI253SetPage(kal_uint8 Page)
{
  HI253WriteCmosSensor(0x03, Page);
}

void HI253InitSetting(void)
{
  /*[SENSOR_INITIALIZATION]
    DISP_DATE = "2010-11-24 "
    DISP_WIDTH = 800
    DISP_HEIGHT = 600
    DISP_FORMAT = YUV422
    DISP_DATAORDER = YUYV
    MCLK = 26.00
    PLL = 2.00  
    BEGIN */

  HI253WriteCmosSensor(0x01, 0xf9); // Sleep ON
  HI253WriteCmosSensor(0x08, 0x0f); // Hi-Z ON
  HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF

  HI253SetPage(0x00); // Dummy 750us START
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00); // Dummy 750us END

  HI253WriteCmosSensor(0x0e, 0x00); // PLL 

  HI253SetPage(0x00); // Dummy 750us START
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00); // Dummy 750us END

  HI253WriteCmosSensor(0x0e, 0x00); // PLL OFF
  HI253WriteCmosSensor(0x01, 0xf1); // Sleep ON
  HI253WriteCmosSensor(0x08, 0x00); // Hi-Z OFF
  HI253WriteCmosSensor(0x01, 0xf3);
  HI253WriteCmosSensor(0x01, 0xf1);

  HI253SetPage(0x20); 
  HI253WriteCmosSensor(0x10, 0x0c); // AE OFF
  HI253SetPage(0x22); 
  HI253WriteCmosSensor(0x10, 0x69); // AWB OFF

  HI253SetPage(0x00); 
  HI253WriteCmosSensor(0x10, 0x13); // Sub1/2_Preview2
  HI253WriteCmosSensor(0x11, 0x90); // Windowing ON, 1Frame Skip
  HI253WriteCmosSensor(0x12, 0x04); // 00:Rinsing edge 04:fall edge
  HI253WriteCmosSensor(0x0b, 0xaa); // ESD Check Register
  HI253WriteCmosSensor(0x0c, 0xaa); // ESD Check Register
  HI253WriteCmosSensor(0x0d, 0xaa); // ESD Check Register
  HI253WriteCmosSensor(0x20, 0x00); // WINROWH
  HI253WriteCmosSensor(0x21, 0x04); // WINROWL
  HI253WriteCmosSensor(0x22, 0x00); // WINCOLH
  HI253WriteCmosSensor(0x23, 0x07); // WINCOLL
  HI253WriteCmosSensor(0x24, 0x04); // WINHGTH
  HI253WriteCmosSensor(0x25, 0xb0); // WINHGTL
  HI253WriteCmosSensor(0x26, 0x06); // WINWIDH
  HI253WriteCmosSensor(0x27, 0x40); // WINWIDL

  HI253WriteCmosSensor(0x40, 0x01); // HBLANKH 424
  HI253WriteCmosSensor(0x41, 0xa8); // HBLANKL
  HI253WriteCmosSensor(0x42, 0x00); // VSYNCH 62
  HI253WriteCmosSensor(0x43, 0x3e); // VSYNCL

  HI253WriteCmosSensor(0x45, 0x04); // VSCTL1
  HI253WriteCmosSensor(0x46, 0x18); // VSCTL2
  HI253WriteCmosSensor(0x47, 0xd8); // VSCTL3

  HI253WriteCmosSensor(0xe1, 0x0f);

  //BLC 
  HI253WriteCmosSensor(0x80, 0x2e); // BLCCTL
  HI253WriteCmosSensor(0x81, 0x7e);
  HI253WriteCmosSensor(0x82, 0x90);
  HI253WriteCmosSensor(0x83, 0x00);
  HI253WriteCmosSensor(0x84, 0x0c);
  HI253WriteCmosSensor(0x85, 0x00);
  HI253WriteCmosSensor(0x90, 0x0a); // BLCTIMETH ON
  HI253WriteCmosSensor(0x91, 0x0a); // BLCTIMETH OFF
  HI253WriteCmosSensor(0x92, 0x78); // BLCAGTH ON
  HI253WriteCmosSensor(0x93, 0x70); // BLCAGTH OFF
  HI253WriteCmosSensor(0x94, 0x75); // BLCDGTH ON
  HI253WriteCmosSensor(0x95, 0x70); // BLCDGTH OFF
  HI253WriteCmosSensor(0x96, 0xdc);
  HI253WriteCmosSensor(0x97, 0xfe);
  HI253WriteCmosSensor(0x98, 0x20);

  //OutDoor BLC
  HI253WriteCmosSensor(0x99,0x42);// B
  HI253WriteCmosSensor(0x9a,0x42);// Gb
  HI253WriteCmosSensor(0x9b,0x42);// R
  HI253WriteCmosSensor(0x9c,0x42);// Gr

  //Dark BLC 
  HI253WriteCmosSensor(0xa0, 0x00); // BLCOFS DB
  HI253WriteCmosSensor(0xa2, 0x00); // BLCOFS DGb
  HI253WriteCmosSensor(0xa4, 0x00); // BLCOFS DR
  HI253WriteCmosSensor(0xa6, 0x00); // BLCOFS DGr

  //Normal BLC
  HI253WriteCmosSensor(0xa8, 0x43);
  HI253WriteCmosSensor(0xaa, 0x43);
  HI253WriteCmosSensor(0xac, 0x43);
  HI253WriteCmosSensor(0xae, 0x43);

  HI253SetPage(0x02); 
  HI253WriteCmosSensor(0x12, 0x03);
  HI253WriteCmosSensor(0x13, 0x03);
  HI253WriteCmosSensor(0x16, 0x00);
  HI253WriteCmosSensor(0x17, 0x8C);
  HI253WriteCmosSensor(0x18, 0x4c); // 0x28->0x2c->4c [20100919 update]
  HI253WriteCmosSensor(0x19, 0x00); // 0x40->0x00 [20100912 update]
  HI253WriteCmosSensor(0x1a, 0x39);
  HI253WriteCmosSensor(0x1c, 0x09);
  HI253WriteCmosSensor(0x1d, 0x40);
  HI253WriteCmosSensor(0x1e, 0x30);
  HI253WriteCmosSensor(0x1f, 0x10);
  HI253WriteCmosSensor(0x20, 0x77);
  HI253WriteCmosSensor(0x21, 0xde); // 0xdd->0xde [20100919 update]
  HI253WriteCmosSensor(0x22, 0xa7);
  HI253WriteCmosSensor(0x23, 0x30); // 0xb0->0x30 [20100912 update]
  HI253WriteCmosSensor(0x27, 0x3c);
  HI253WriteCmosSensor(0x2b, 0x80);
  HI253WriteCmosSensor(0x2e, 0x00); // 100913 power saving Hy gou 11},
  HI253WriteCmosSensor(0x2f, 0x00); // 100913 power saving Hy gou a1},
  HI253WriteCmosSensor(0x30, 0x05);
  HI253WriteCmosSensor(0x50, 0x20);
  HI253WriteCmosSensor(0x52, 0x01);
  HI253WriteCmosSensor(0x53, 0xc1);
  HI253WriteCmosSensor(0x55, 0x1c);
  HI253WriteCmosSensor(0x56, 0x11); // 0x00->0x11 [20100912 update]
  HI253WriteCmosSensor(0x5d, 0xA2);
  HI253WriteCmosSensor(0x5e, 0x5a); 
  HI253WriteCmosSensor(0x60, 0x87);
  HI253WriteCmosSensor(0x61, 0x99);
  HI253WriteCmosSensor(0x62, 0x88);
  HI253WriteCmosSensor(0x63, 0x97);
  HI253WriteCmosSensor(0x64, 0x88);
  HI253WriteCmosSensor(0x65, 0x97);
  HI253WriteCmosSensor(0x67, 0x0c);
  HI253WriteCmosSensor(0x68, 0x0c);
  HI253WriteCmosSensor(0x69, 0x0c);
  HI253WriteCmosSensor(0x72, 0x89);
  HI253WriteCmosSensor(0x73, 0x96); // 0x97->0x96 [20100919 update]
  HI253WriteCmosSensor(0x74, 0x89);
  HI253WriteCmosSensor(0x75, 0x96); // 0x97->0x96 [20100919 update]
  HI253WriteCmosSensor(0x76, 0x89);
  HI253WriteCmosSensor(0x77, 0x96); // 0x97->0x96 [20100912 update]
  HI253WriteCmosSensor(0x7C, 0x85);
  HI253WriteCmosSensor(0x7d, 0xaf);
  HI253WriteCmosSensor(0x80, 0x01);
  HI253WriteCmosSensor(0x81, 0x7f); // 0x81->0x7f [20100919 update]
  HI253WriteCmosSensor(0x82, 0x13); // 0x23->0x13 [20100912 update]
  HI253WriteCmosSensor(0x83, 0x24); // 0x2b->0x24 [20100912 update]
  HI253WriteCmosSensor(0x84, 0x7d);
  HI253WriteCmosSensor(0x85, 0x81);
  HI253WriteCmosSensor(0x86, 0x7d);
  HI253WriteCmosSensor(0x87, 0x81);
  HI253WriteCmosSensor(0x92, 0x48); // 0x53->0x48 [20100912 update]
  HI253WriteCmosSensor(0x93, 0x54); // 0x5e->0x54 [20100912 update]
  HI253WriteCmosSensor(0x94, 0x7d);
  HI253WriteCmosSensor(0x95, 0x81);
  HI253WriteCmosSensor(0x96, 0x7d);
  HI253WriteCmosSensor(0x97, 0x81);
  HI253WriteCmosSensor(0xa0, 0x02);
  HI253WriteCmosSensor(0xa1, 0x7b);
  HI253WriteCmosSensor(0xa2, 0x02);
  HI253WriteCmosSensor(0xa3, 0x7b);
  HI253WriteCmosSensor(0xa4, 0x7b);
  HI253WriteCmosSensor(0xa5, 0x02);
  HI253WriteCmosSensor(0xa6, 0x7b);
  HI253WriteCmosSensor(0xa7, 0x02);
  HI253WriteCmosSensor(0xa8, 0x85);
  HI253WriteCmosSensor(0xa9, 0x8c);
  HI253WriteCmosSensor(0xaa, 0x85);
  HI253WriteCmosSensor(0xab, 0x8c);
  HI253WriteCmosSensor(0xac, 0x10); // 0x20->0x10 [20100912 update]
  HI253WriteCmosSensor(0xad, 0x16); // 0x26->0x16 [20100912 update]
  HI253WriteCmosSensor(0xae, 0x10); // 0x20->0x10 [20100912 update]
  HI253WriteCmosSensor(0xaf, 0x16); // 0x26->0x16 [20100912 update]
  HI253WriteCmosSensor(0xb0, 0x99);
  HI253WriteCmosSensor(0xb1, 0xa3);
  HI253WriteCmosSensor(0xb2, 0xa4);
  HI253WriteCmosSensor(0xb3, 0xae);
  HI253WriteCmosSensor(0xb4, 0x9b);
  HI253WriteCmosSensor(0xb5, 0xa2);
  HI253WriteCmosSensor(0xb6, 0xa6);
  HI253WriteCmosSensor(0xb7, 0xac);
  HI253WriteCmosSensor(0xb8, 0x9b);
  HI253WriteCmosSensor(0xb9, 0x9f);
  HI253WriteCmosSensor(0xba, 0xa6);
  HI253WriteCmosSensor(0xbb, 0xaa);
  HI253WriteCmosSensor(0xbc, 0x9b);
  HI253WriteCmosSensor(0xbd, 0x9f);
  HI253WriteCmosSensor(0xbe, 0xa6);
  HI253WriteCmosSensor(0xbf, 0xaa);
  HI253WriteCmosSensor(0xc4, 0x2c); // 0x36->0x2c [20100912 update]
  HI253WriteCmosSensor(0xc5, 0x43); // 0x4e->0x43 [20100912 update]
  HI253WriteCmosSensor(0xc6, 0x63); // 0x61->0x63 [20100912 update]
  HI253WriteCmosSensor(0xc7, 0x79); // 0x78->0x79 [20100919 update]
  HI253WriteCmosSensor(0xc8, 0x2d); // 0x36->0x2d [20100912 update]
  HI253WriteCmosSensor(0xc9, 0x42); // 0x4d->0x42 [20100912 update]
  HI253WriteCmosSensor(0xca, 0x2d); // 0x36->0x2d [20100912 update]
  HI253WriteCmosSensor(0xcb, 0x42); // 0x4d->0x42 [20100912 update]
  HI253WriteCmosSensor(0xcc, 0x64); // 0x62->0x64 [20100912 update]
  HI253WriteCmosSensor(0xcd, 0x78);
  HI253WriteCmosSensor(0xce, 0x64); // 0x62->0x64 [20100912 update]
  HI253WriteCmosSensor(0xcf, 0x78);
  HI253WriteCmosSensor(0xd0, 0x0a);
  HI253WriteCmosSensor(0xd1, 0x09);
  HI253WriteCmosSensor(0xd4, 0x0a); // DCDCTIMETHON
  HI253WriteCmosSensor(0xd5, 0x0a); // DCDCTIMETHOFF
  HI253WriteCmosSensor(0xd6, 0x78); // DCDCAGTHON
  HI253WriteCmosSensor(0xd7, 0x70); // DCDCAGTHOFF
  HI253WriteCmosSensor(0xe0, 0xc4);
  HI253WriteCmosSensor(0xe1, 0xc4);
  HI253WriteCmosSensor(0xe2, 0xc4);
  HI253WriteCmosSensor(0xe3, 0xc4);
  HI253WriteCmosSensor(0xe4, 0x00);
  HI253WriteCmosSensor(0xe8, 0x80); // 0x00->0x80 [20100919 update]
  HI253WriteCmosSensor(0xe9, 0x40);
  HI253WriteCmosSensor(0xea, 0x7f); // 0x82->0x7f [20100919 update]
  HI253WriteCmosSensor(0xf0, 0x01); // 100810 memory delay
  HI253WriteCmosSensor(0xf1, 0x01); // 100810 memory delay
  HI253WriteCmosSensor(0xf2, 0x01); // 100810 memory delay
  HI253WriteCmosSensor(0xf3, 0x01); // 100810 memory delay
  HI253WriteCmosSensor(0xf4, 0x01); // 100810 memory delay

  HI253SetPage(0x03);
  HI253WriteCmosSensor(0x10, 0x10);

  HI253SetPage(0x10); 
  HI253WriteCmosSensor(0x10, 0x03); // YUYV
  HI253WriteCmosSensor(0x12, 0x30); // ISPCTL3
  HI253WriteCmosSensor(0x20, 0x00); // ITUCTL
  HI253WriteCmosSensor(0x30, 0x00);
  HI253WriteCmosSensor(0x31, 0x00);
  HI253WriteCmosSensor(0x32, 0x00);
  HI253WriteCmosSensor(0x33, 0x00);
  HI253WriteCmosSensor(0x34, 0x30);
  HI253WriteCmosSensor(0x35, 0x00);
  HI253WriteCmosSensor(0x36, 0x00);
  HI253WriteCmosSensor(0x38, 0x00);
  HI253WriteCmosSensor(0x3e, 0x58);
  HI253WriteCmosSensor(0x3f, 0x02); // 0x02 for preview and 0x00 for capture
  HI253WriteCmosSensor(0x40, 0x85); // YOFS modify brightness
  HI253WriteCmosSensor(0x41, 0x00); // DYOFS

  //Saturation;
  HI253WriteCmosSensor(0x60, 0x6f); // SATCTL
  HI253WriteCmosSensor(0x61, 0x76); // SATB
  HI253WriteCmosSensor(0x62, 0x76); // SATR
  HI253WriteCmosSensor(0x63, 0x30); // AGSAT
  HI253WriteCmosSensor(0x64, 0x41);
  HI253WriteCmosSensor(0x66, 0x33); // SATTIMETH
  HI253WriteCmosSensor(0x67, 0x00); // SATOUTDEL
  HI253WriteCmosSensor(0x6a, 0x90); // UPOSSAT
  HI253WriteCmosSensor(0x6b, 0x80); // UNEGSAT
  HI253WriteCmosSensor(0x6c, 0x80); // VPOSSAT
  HI253WriteCmosSensor(0x6d, 0xa0); // VNEGSAT
  HI253WriteCmosSensor(0x76, 0x01); // white protection ON
  HI253WriteCmosSensor(0x74, 0x66);
  HI253WriteCmosSensor(0x79, 0x06);

  HI253SetPage(0x11);
  HI253WriteCmosSensor(0x10, 0x3f); // DLPFCTL1
  HI253WriteCmosSensor(0x11, 0x40);
  HI253WriteCmosSensor(0x12, 0xba);
  HI253WriteCmosSensor(0x13, 0xcb);
  HI253WriteCmosSensor(0x26, 0x20); // LPFAGTHL
  HI253WriteCmosSensor(0x27, 0x22); // LPFAGTHH
  HI253WriteCmosSensor(0x28, 0x0f); // LPFOUTTHL
  HI253WriteCmosSensor(0x29, 0x10); // LPFOUTTHH
  HI253WriteCmosSensor(0x2b, 0x30); // LPFYMEANTHL
  HI253WriteCmosSensor(0x2c, 0x32); // LPFYMEANTHH

  //Out2 D-LPF th
  HI253WriteCmosSensor(0x30, 0x70); // OUT2YBOUNDH
  HI253WriteCmosSensor(0x31, 0x10); // OUT2YBOUNDL
  HI253WriteCmosSensor(0x32, 0x65); // OUT2RATIO
  HI253WriteCmosSensor(0x33, 0x09); // OUT2THH
  HI253WriteCmosSensor(0x34, 0x06); // OUT2THM
  HI253WriteCmosSensor(0x35, 0x04); // OUT2THL

  //Out1 D-LPF th
  HI253WriteCmosSensor(0x36, 0x70); // OUT1YBOUNDH
  HI253WriteCmosSensor(0x37, 0x18); // OUT1YBOUNDL
  HI253WriteCmosSensor(0x38, 0x65); // OUT1RATIO
  HI253WriteCmosSensor(0x39, 0x09); // OUT1THH
  HI253WriteCmosSensor(0x3a, 0x06); // OUT1THM
  HI253WriteCmosSensor(0x3b, 0x04); // OUT1THL

  //Indoor D-LPF th
  HI253WriteCmosSensor(0x3c, 0x80); // INYBOUNDH
  HI253WriteCmosSensor(0x3d, 0x18); // INYBOUNDL
  HI253WriteCmosSensor(0x3e, 0x80); // INRATIO
  HI253WriteCmosSensor(0x3f, 0x0c); // INTHH
  HI253WriteCmosSensor(0x40, 0x09); // INTHM
  HI253WriteCmosSensor(0x41, 0x06); // INTHL

  HI253WriteCmosSensor(0x42, 0x80); // DARK1YBOUNDH
  HI253WriteCmosSensor(0x43, 0x18); // DARK1YBOUNDL
  HI253WriteCmosSensor(0x44, 0x80); // DARK1RATIO
  HI253WriteCmosSensor(0x45, 0x12); // DARK1THH
  HI253WriteCmosSensor(0x46, 0x10); // DARK1THM
  HI253WriteCmosSensor(0x47, 0x10); // DARK1THL
  HI253WriteCmosSensor(0x48, 0x90); // DARK2YBOUNDH
  HI253WriteCmosSensor(0x49, 0x40); // DARK2YBOUNDL
  HI253WriteCmosSensor(0x4a, 0x80); // DARK2RATIO
  HI253WriteCmosSensor(0x4b, 0x13); // DARK2THH
  HI253WriteCmosSensor(0x4c, 0x10); // DARK2THM
  HI253WriteCmosSensor(0x4d, 0x11); // DARK2THL
  HI253WriteCmosSensor(0x4e, 0x80); // DARK3YBOUNDH
  HI253WriteCmosSensor(0x4f, 0x30); // DARK3YBOUNDL
  HI253WriteCmosSensor(0x50, 0x80); // DARK3RATIO
  HI253WriteCmosSensor(0x51, 0x13); // DARK3THH
  HI253WriteCmosSensor(0x52, 0x10); // DARK3THM
  HI253WriteCmosSensor(0x53, 0x13); // DARK3THL
  HI253WriteCmosSensor(0x54, 0x11);
  HI253WriteCmosSensor(0x55, 0x17);
  HI253WriteCmosSensor(0x56, 0x20);
  HI253WriteCmosSensor(0x57, 0x20);
  HI253WriteCmosSensor(0x58, 0x20);
  HI253WriteCmosSensor(0x59, 0x30);
  HI253WriteCmosSensor(0x5a, 0x18);
  HI253WriteCmosSensor(0x5b, 0x00);
  HI253WriteCmosSensor(0x5c, 0x00);
  HI253WriteCmosSensor(0x60, 0x3f);
  HI253WriteCmosSensor(0x62, 0x50);
  HI253WriteCmosSensor(0x70, 0x06);

  HI253SetPage(0x12); 
  HI253WriteCmosSensor(0x20, 0x00); // YCLPFCTL1 0x00 for preview and 0x0f for capture
  HI253WriteCmosSensor(0x21, 0x00); // YCLPFCTL2 0x00 for preview and 0x0f for capture
  HI253WriteCmosSensor(0x25, 0x30);
  HI253WriteCmosSensor(0x28, 0x00); // Out1 BP t
  HI253WriteCmosSensor(0x29, 0x00); // Out2 BP t
  HI253WriteCmosSensor(0x2a, 0x00);
  HI253WriteCmosSensor(0x30, 0x50);
  HI253WriteCmosSensor(0x31, 0x18); // YCUNI1TH
  HI253WriteCmosSensor(0x32, 0x32); // YCUNI2TH
  HI253WriteCmosSensor(0x33, 0x40); // YCUNI3TH
  HI253WriteCmosSensor(0x34, 0x50); // YCNOR1TH
  HI253WriteCmosSensor(0x35, 0x70); // YCNOR2TH
  HI253WriteCmosSensor(0x36, 0xa0); // YCNOR3TH
  HI253WriteCmosSensor(0x3b, 0x06);
  HI253WriteCmosSensor(0x3c, 0x06);

  //Out2 th
  HI253WriteCmosSensor(0x40, 0xa0); // YCOUT2THH
  HI253WriteCmosSensor(0x41, 0x40); // YCOUT2THL
  HI253WriteCmosSensor(0x42, 0xa0); // YCOUT2STDH
  HI253WriteCmosSensor(0x43, 0x90); // YCOUT2STDM
  HI253WriteCmosSensor(0x44, 0x90); // YCOUT2STDL
  HI253WriteCmosSensor(0x45, 0x80); // YCOUT2RAT

  //Out1 th 
  HI253WriteCmosSensor(0x46, 0xb0); // YCOUT1THH
  HI253WriteCmosSensor(0x47, 0x55); // YCOUT1THL
  HI253WriteCmosSensor(0x48, 0xa0); // YCOUT1STDH
  HI253WriteCmosSensor(0x49, 0x90); // YCOUT1STDM
  HI253WriteCmosSensor(0x4a, 0x90); // YCOUT1STDL
  HI253WriteCmosSensor(0x4b, 0x80); // YCOUT1RAT

  //In door th
  HI253WriteCmosSensor(0x4c, 0xb0); // YCINTHH
  HI253WriteCmosSensor(0x4d, 0x40); // YCINTHL
  HI253WriteCmosSensor(0x4e, 0x90); // YCINSTDH
  HI253WriteCmosSensor(0x4f, 0x90); // YCINSTDM
  HI253WriteCmosSensor(0x50, 0xe6); // YCINSTDL
  HI253WriteCmosSensor(0x51, 0x80); // YCINRAT

  //Dark1 th
  HI253WriteCmosSensor(0x52, 0xb0); // YCDARK1THH
  HI253WriteCmosSensor(0x53, 0x60); // YCDARK1THL
  HI253WriteCmosSensor(0x54, 0xc0); // YCDARK1STDH
  HI253WriteCmosSensor(0x55, 0xc0); // YCDARK1STDM
  HI253WriteCmosSensor(0x56, 0xc0); // YCDARK1STDL
  HI253WriteCmosSensor(0x57, 0x80); // YCDARK1RAT

  //Dark2 th
  HI253WriteCmosSensor(0x58, 0x90); // YCDARK2THH
  HI253WriteCmosSensor(0x59, 0x40); // YCDARK2THL
  HI253WriteCmosSensor(0x5a, 0xd0); // YCDARK2STDH
  HI253WriteCmosSensor(0x5b, 0xd0); // YCDARK2STDM
  HI253WriteCmosSensor(0x5c, 0xe0); // YCDARK2STDL
  HI253WriteCmosSensor(0x5d, 0x80); // YCDARK2RAT

  //Dark3 th
  HI253WriteCmosSensor(0x5e, 0x88); // YCDARK3THH
  HI253WriteCmosSensor(0x5f, 0x40); // YCDARK3THL
  HI253WriteCmosSensor(0x60, 0xe0); // YCDARK3STDH
  HI253WriteCmosSensor(0x61, 0xe6); // YCDARK3STDM
  HI253WriteCmosSensor(0x62, 0xe6); // YCDARK3STDL
  HI253WriteCmosSensor(0x63, 0x80); // YCDARK3RAT

  HI253WriteCmosSensor(0x70, 0x15);
  HI253WriteCmosSensor(0x71, 0x01); //Don't Touch register

  HI253WriteCmosSensor(0x72, 0x18);
  HI253WriteCmosSensor(0x73, 0x01); //Don't Touch register

  HI253WriteCmosSensor(0x74, 0x25);
  HI253WriteCmosSensor(0x75, 0x15);
  HI253WriteCmosSensor(0x80, 0x30);
  HI253WriteCmosSensor(0x81, 0x50);
  HI253WriteCmosSensor(0x82, 0x80);
  HI253WriteCmosSensor(0x85, 0x1a);
  HI253WriteCmosSensor(0x88, 0x00);
  HI253WriteCmosSensor(0x89, 0x00);
  HI253WriteCmosSensor(0x90, 0x00); // DPCCTL 0x00 For Preview and 0x5d for capture

  HI253WriteCmosSensor(0xc5, 0x30);
  HI253WriteCmosSensor(0xc6, 0x2a);

  //Dont Touch register
  HI253WriteCmosSensor(0xD0, 0x0c);
  HI253WriteCmosSensor(0xD1, 0x80);
  HI253WriteCmosSensor(0xD2, 0x67);
  HI253WriteCmosSensor(0xD3, 0x00);
  HI253WriteCmosSensor(0xD4, 0x00);
  HI253WriteCmosSensor(0xD5, 0x02);
  HI253WriteCmosSensor(0xD6, 0xff);
  HI253WriteCmosSensor(0xD7, 0x18);

  HI253SetPage(0x13); 
  HI253WriteCmosSensor(0x10, 0xcb); // EDGECTL1
  HI253WriteCmosSensor(0x11, 0x7b); // EDGECTL2
  HI253WriteCmosSensor(0x12, 0x07); // EDGECTL3
  HI253WriteCmosSensor(0x14, 0x00); // EDGECTL5

  HI253WriteCmosSensor(0x20, 0x15); // EDGENGAIN
  HI253WriteCmosSensor(0x21, 0x13); // EDGEPGAIN
  HI253WriteCmosSensor(0x22, 0x33);
  HI253WriteCmosSensor(0x23, 0x04); // EDGEHCLIP1TH
  HI253WriteCmosSensor(0x24, 0x09); // EDGEHCLIP2TH
  HI253WriteCmosSensor(0x25, 0x08); // EDGELCLIPTH
  HI253WriteCmosSensor(0x26, 0x18); // EDGELCLIPLMT
  HI253WriteCmosSensor(0x27, 0x30);
  HI253WriteCmosSensor(0x29, 0x12); // EDGETIMETH
  HI253WriteCmosSensor(0x2a, 0x50); // EDGEAGTH

  //Low clip th
  HI253WriteCmosSensor(0x2b, 0x06);
  HI253WriteCmosSensor(0x2c, 0x06);
  HI253WriteCmosSensor(0x25, 0x08);
  HI253WriteCmosSensor(0x2d, 0x0c);
  HI253WriteCmosSensor(0x2e, 0x12);
  HI253WriteCmosSensor(0x2f, 0x12);

  //Out2 Edge
  HI253WriteCmosSensor(0x50, 0x10);
  HI253WriteCmosSensor(0x51, 0x14);
  HI253WriteCmosSensor(0x52, 0x10);
  HI253WriteCmosSensor(0x53, 0x0c);
  HI253WriteCmosSensor(0x54, 0x0f);
  HI253WriteCmosSensor(0x55, 0x0c);

  //Out1 Edge
  HI253WriteCmosSensor(0x56, 0x10);
  HI253WriteCmosSensor(0x57, 0x13);
  HI253WriteCmosSensor(0x58, 0x10);
  HI253WriteCmosSensor(0x59, 0x0c);
  HI253WriteCmosSensor(0x5a, 0x0f);
  HI253WriteCmosSensor(0x5b, 0x0c);

  //Indoor Edg 
  HI253WriteCmosSensor(0x5c, 0x0a);
  HI253WriteCmosSensor(0x5d, 0x0b);
  HI253WriteCmosSensor(0x5e, 0x0a);
  HI253WriteCmosSensor(0x5f, 0x08);
  HI253WriteCmosSensor(0x60, 0x09);
  HI253WriteCmosSensor(0x61, 0x08);

  //Dark1 Edge
  HI253WriteCmosSensor(0x62, 0x08);
  HI253WriteCmosSensor(0x63, 0x08);
  HI253WriteCmosSensor(0x64, 0x08);
  HI253WriteCmosSensor(0x65, 0x06);
  HI253WriteCmosSensor(0x66, 0x06);
  HI253WriteCmosSensor(0x67, 0x06);

  //Dark2 Edge
  HI253WriteCmosSensor(0x68, 0x07);
  HI253WriteCmosSensor(0x69, 0x07);
  HI253WriteCmosSensor(0x6a, 0x07);
  HI253WriteCmosSensor(0x6b, 0x05);
  HI253WriteCmosSensor(0x6c, 0x05);
  HI253WriteCmosSensor(0x6d, 0x05);

  //Dark3 Edge
  HI253WriteCmosSensor(0x6e, 0x07);
  HI253WriteCmosSensor(0x6f, 0x07);
  HI253WriteCmosSensor(0x70, 0x07);
  HI253WriteCmosSensor(0x71, 0x05);
  HI253WriteCmosSensor(0x72, 0x05);
  HI253WriteCmosSensor(0x73, 0x05);

  //2DY 
  HI253WriteCmosSensor(0x80, 0x00); // EDGE2DCTL1 00 for preview, must turn on 2DY 0xfd when capture
  HI253WriteCmosSensor(0x81, 0x1f); // EDGE2DCTL2
  HI253WriteCmosSensor(0x82, 0x05); // EDGE2DCTL3
  HI253WriteCmosSensor(0x83, 0x01); // EDGE2DCTL4

  HI253WriteCmosSensor(0x90, 0x05); // EDGE2DNGAIN
  HI253WriteCmosSensor(0x91, 0x05); // EDGE2DPGAIN
  HI253WriteCmosSensor(0x92, 0x33); // EDGE2DLCLIPLMT
  HI253WriteCmosSensor(0x93, 0x30);
  HI253WriteCmosSensor(0x94, 0x03); // EDGE2DHCLIP1TH
  HI253WriteCmosSensor(0x95, 0x14); // EDGE2DHCLIP2TH
  HI253WriteCmosSensor(0x97, 0x30);
  HI253WriteCmosSensor(0x99, 0x30);

  HI253WriteCmosSensor(0xa0, 0x04); // EDGE2DLCOUT2N
  HI253WriteCmosSensor(0xa1, 0x05); // EDGE2DLCOUT2P
  HI253WriteCmosSensor(0xa2, 0x04); // EDGE2DLCOUT1N
  HI253WriteCmosSensor(0xa3, 0x05); // EDGE2DLCOUT1P
  HI253WriteCmosSensor(0xa4, 0x07); // EDGE2DLCINN
  HI253WriteCmosSensor(0xa5, 0x08); // EDGE2DLCINP
  HI253WriteCmosSensor(0xa6, 0x07); // EDGE2DLCD1N
  HI253WriteCmosSensor(0xa7, 0x08); // EDGE2DLCD1P
  HI253WriteCmosSensor(0xa8, 0x07); // EDGE2DLCD2N
  HI253WriteCmosSensor(0xa9, 0x08); // EDGE2DLCD2P
  HI253WriteCmosSensor(0xaa, 0x07); // EDGE2DLCD3N
  HI253WriteCmosSensor(0xab, 0x08); // EDGE2DLCD3P

  //Out2
  HI253WriteCmosSensor(0xb0, 0x22);
  HI253WriteCmosSensor(0xb1, 0x2a);
  HI253WriteCmosSensor(0xb2, 0x28);
  HI253WriteCmosSensor(0xb3, 0x22);
  HI253WriteCmosSensor(0xb4, 0x2a);
  HI253WriteCmosSensor(0xb5, 0x28);

  //Out1
  HI253WriteCmosSensor(0xb6, 0x22);
  HI253WriteCmosSensor(0xb7, 0x2a);
  HI253WriteCmosSensor(0xb8, 0x28);
  HI253WriteCmosSensor(0xb9, 0x22);
  HI253WriteCmosSensor(0xba, 0x2a);
  HI253WriteCmosSensor(0xbb, 0x28);

  HI253WriteCmosSensor(0xbc, 0x17);
  HI253WriteCmosSensor(0xbd, 0x17);
  HI253WriteCmosSensor(0xbe, 0x17);
  HI253WriteCmosSensor(0xbf, 0x17);
  HI253WriteCmosSensor(0xc0, 0x17);
  HI253WriteCmosSensor(0xc1, 0x17);

  //Dark1
  HI253WriteCmosSensor(0xc2, 0x1e);
  HI253WriteCmosSensor(0xc3, 0x12);
  HI253WriteCmosSensor(0xc4, 0x10);
  HI253WriteCmosSensor(0xc5, 0x1e);
  HI253WriteCmosSensor(0xc6, 0x12);
  HI253WriteCmosSensor(0xc7, 0x10);

  //Dark2
  HI253WriteCmosSensor(0xc8, 0x18);
  HI253WriteCmosSensor(0xc9, 0x05);
  HI253WriteCmosSensor(0xca, 0x05);
  HI253WriteCmosSensor(0xcb, 0x18);
  HI253WriteCmosSensor(0xcc, 0x05);
  HI253WriteCmosSensor(0xcd, 0x05);

  //Dark3
  HI253WriteCmosSensor(0xce, 0x18);
  HI253WriteCmosSensor(0xcf, 0x05);
  HI253WriteCmosSensor(0xd0, 0x05);
  HI253WriteCmosSensor(0xd1, 0x18);
  HI253WriteCmosSensor(0xd2, 0x05);
  HI253WriteCmosSensor(0xd3, 0x05);

  HI253SetPage(0x14);
  HI253WriteCmosSensor(0x10, 0x11); // LENSCTL1
  HI253WriteCmosSensor(0x20, 0x40); // XCEN
  HI253WriteCmosSensor(0x21, 0x80); // YCEN
  HI253WriteCmosSensor(0x22, 0x80); // LENSRGAIN
  HI253WriteCmosSensor(0x23, 0x80); // LENSGGAIN
  HI253WriteCmosSensor(0x24, 0x80); // LENSBGAIN

  HI253WriteCmosSensor(0x30, 0xc8);
  HI253WriteCmosSensor(0x31, 0x2b);
  HI253WriteCmosSensor(0x32, 0x00);
  HI253WriteCmosSensor(0x33, 0x00);
  HI253WriteCmosSensor(0x34, 0x90);

  HI253WriteCmosSensor(0x40, 0x65); // LENSRP0
  HI253WriteCmosSensor(0x50, 0x42); // LENSGrP0
  HI253WriteCmosSensor(0x60, 0x3a); // LENSBP0
  HI253WriteCmosSensor(0x70, 0x42); // LENSGbP0

  HI253SetPage(0x15); 
  HI253WriteCmosSensor(0x10, 0x0f); // CMCCTL
  HI253WriteCmosSensor(0x14, 0x52); // CMCOFSGH
  HI253WriteCmosSensor(0x15, 0x42); // CMCOFSGM
  HI253WriteCmosSensor(0x16, 0x32); // CMCOFSGL
  HI253WriteCmosSensor(0x17, 0x2f); // CMCSIGN

  //CMC 
  HI253WriteCmosSensor(0x30, 0x8f); // CMC11
  HI253WriteCmosSensor(0x31, 0x59); // CMC12
  HI253WriteCmosSensor(0x32, 0x0a); // CMC13
  HI253WriteCmosSensor(0x33, 0x15); // CMC21
  HI253WriteCmosSensor(0x34, 0x5b); // CMC22
  HI253WriteCmosSensor(0x35, 0x06); // CMC23
  HI253WriteCmosSensor(0x36, 0x07); // CMC31
  HI253WriteCmosSensor(0x37, 0x40); // CMC32
  HI253WriteCmosSensor(0x38, 0x86); // CMC33

  //CMC OFS 
  HI253WriteCmosSensor(0x40, 0x95); // CMCOFSL11
  HI253WriteCmosSensor(0x41, 0x1f); // CMCOFSL12
  HI253WriteCmosSensor(0x42, 0x8a); // CMCOFSL13
  HI253WriteCmosSensor(0x43, 0x86); // CMCOFSL21
  HI253WriteCmosSensor(0x44, 0x0a); // CMCOFSL22
  HI253WriteCmosSensor(0x45, 0x84); // CMCOFSL23
  HI253WriteCmosSensor(0x46, 0x87); // CMCOFSL31
  HI253WriteCmosSensor(0x47, 0x9b); // CMCOFSL32
  HI253WriteCmosSensor(0x48, 0x23); // CMCOFSL33

  //CMC POFS
  HI253WriteCmosSensor(0x50, 0x8c); // CMCOFSH11
  HI253WriteCmosSensor(0x51, 0x0c); // CMCOFSH12
  HI253WriteCmosSensor(0x52, 0x00); // CMCOFSH13
  HI253WriteCmosSensor(0x53, 0x07); // CMCOFSH21
  HI253WriteCmosSensor(0x54, 0x17); // CMCOFSH22
  HI253WriteCmosSensor(0x55, 0x9d); // CMCOFSH23
  HI253WriteCmosSensor(0x56, 0x00); // CMCOFSH31
  HI253WriteCmosSensor(0x57, 0x0b); // CMCOFSH32
  HI253WriteCmosSensor(0x58, 0x89); // CMCOFSH33

  HI253WriteCmosSensor(0x80, 0x03);
  HI253WriteCmosSensor(0x85, 0x40);
  HI253WriteCmosSensor(0x87, 0x02);
  HI253WriteCmosSensor(0x88, 0x00);
  HI253WriteCmosSensor(0x89, 0x00);
  HI253WriteCmosSensor(0x8a, 0x00);

  HI253SetPage(0x16); 
  HI253WriteCmosSensor(0x10, 0x31); // GMACTL
  HI253WriteCmosSensor(0x18, 0x37);
  HI253WriteCmosSensor(0x19, 0x36);
  HI253WriteCmosSensor(0x1a, 0x0e);
  HI253WriteCmosSensor(0x1b, 0x01);
  HI253WriteCmosSensor(0x1c, 0xdc);
  HI253WriteCmosSensor(0x1d, 0xfe);

  HI253WriteCmosSensor(0x30, 0x00); // GGMA0
  HI253WriteCmosSensor(0x31, 0x06); // GGMA1
  HI253WriteCmosSensor(0x32, 0x1d); // GGMA2
  HI253WriteCmosSensor(0x33, 0x33); // GGMA3
  HI253WriteCmosSensor(0x34, 0x53); // GGMA4
  HI253WriteCmosSensor(0x35, 0x6c); // GGMA5
  HI253WriteCmosSensor(0x36, 0x81); // GGMA6
  HI253WriteCmosSensor(0x37, 0x94); // GGMA7
  HI253WriteCmosSensor(0x38, 0xa4); // GGMA8
  HI253WriteCmosSensor(0x39, 0xb3); // GGMA9
  HI253WriteCmosSensor(0x3a, 0xc0); // GGMA10
  HI253WriteCmosSensor(0x3b, 0xcb); // GGMA11
  HI253WriteCmosSensor(0x3c, 0xd5); // GGMA12
  HI253WriteCmosSensor(0x3d, 0xde); // GGMA13
  HI253WriteCmosSensor(0x3e, 0xe6); // GGMA14
  HI253WriteCmosSensor(0x3f, 0xee); // GGMA15
  HI253WriteCmosSensor(0x40, 0xf5); // GGMA16
  HI253WriteCmosSensor(0x41, 0xfc); // GGMA17
  HI253WriteCmosSensor(0x42, 0xff); // GGMA18

  HI253WriteCmosSensor(0x50, 0x00); // RGMA0
  HI253WriteCmosSensor(0x51, 0x03); // RGMA1
  HI253WriteCmosSensor(0x52, 0x19); // RGMA2
  HI253WriteCmosSensor(0x53, 0x34); // RGMA3
  HI253WriteCmosSensor(0x54, 0x58); // RGMA4
  HI253WriteCmosSensor(0x55, 0x75); // RGMA5
  HI253WriteCmosSensor(0x56, 0x8d); // RGMA6
  HI253WriteCmosSensor(0x57, 0xa1); // RGMA7
  HI253WriteCmosSensor(0x58, 0xb2); // RGMA8
  HI253WriteCmosSensor(0x59, 0xbe); // RGMA9
  HI253WriteCmosSensor(0x5a, 0xc9); // RGMA10
  HI253WriteCmosSensor(0x5b, 0xd2); // RGMA11
  HI253WriteCmosSensor(0x5c, 0xdb); // RGMA12
  HI253WriteCmosSensor(0x5d, 0xe3); // RGMA13
  HI253WriteCmosSensor(0x5e, 0xeb); // RGMA14
  HI253WriteCmosSensor(0x5f, 0xf0); // RGMA15
  HI253WriteCmosSensor(0x60, 0xf5); // RGMA16
  HI253WriteCmosSensor(0x61, 0xf7); // RGMA17
  HI253WriteCmosSensor(0x62, 0xf8); // RGMA18

  HI253WriteCmosSensor(0x70, 0x00); // BGMA0
  HI253WriteCmosSensor(0x71, 0x08); // BGMA1
  HI253WriteCmosSensor(0x72, 0x17); // BGMA2
  HI253WriteCmosSensor(0x73, 0x2f); // BGMA3
  HI253WriteCmosSensor(0x74, 0x53); // BGMA4
  HI253WriteCmosSensor(0x75, 0x6c); // BGMA5
  HI253WriteCmosSensor(0x76, 0x81); // BGMA6
  HI253WriteCmosSensor(0x77, 0x94); // BGMA7
  HI253WriteCmosSensor(0x78, 0xa4); // BGMA8
  HI253WriteCmosSensor(0x79, 0xb3); // BGMA9
  HI253WriteCmosSensor(0x7a, 0xc0); // BGMA10
  HI253WriteCmosSensor(0x7b, 0xcb); // BGMA11
  HI253WriteCmosSensor(0x7c, 0xd5); // BGMA12
  HI253WriteCmosSensor(0x7d, 0xde); // BGMA13
  HI253WriteCmosSensor(0x7e, 0xe6); // BGMA14
  HI253WriteCmosSensor(0x7f, 0xee); // BGMA15
  HI253WriteCmosSensor(0x80, 0xf4); // BGMA16
  HI253WriteCmosSensor(0x81, 0xfa); // BGMA17
  HI253WriteCmosSensor(0x82, 0xff); // BGMA18

  HI253SetPage(0x17); 
  HI253WriteCmosSensor(0xc4, 0x68); // FLK200
  HI253WriteCmosSensor(0xc5, 0x56); // FLK240

  HI253SetPage(0x20); 
  HI253WriteCmosSensor(0x11, 0x1c);
  HI253WriteCmosSensor(0x20, 0x01); // AEFRAMECTL lowtemp off
  HI253WriteCmosSensor(0x21, 0x30);
  HI253WriteCmosSensor(0x22, 0x10);
  HI253WriteCmosSensor(0x23, 0x00);
  HI253WriteCmosSensor(0x24, 0x04);

  HI253WriteCmosSensor(0x28, 0xff);
  HI253WriteCmosSensor(0x29, 0xad);

  //MTK set up anti banding -- > 1/100s   
  HI253WriteCmosSensor(0x2a, 0xf0);
  HI253WriteCmosSensor(0x2b, 0x34);
  HI253WriteCmosSensor(0x2c, 0xc3);
  HI253WriteCmosSensor(0x2d, 0x5f);
  HI253WriteCmosSensor(0x2e, 0x33);
  HI253WriteCmosSensor(0x30, 0x78);
  HI253WriteCmosSensor(0x32, 0x03);
  HI253WriteCmosSensor(0x33, 0x2e);
  HI253WriteCmosSensor(0x34, 0x30);
  HI253WriteCmosSensor(0x35, 0xd4);
  HI253WriteCmosSensor(0x36, 0xfe);
  HI253WriteCmosSensor(0x37, 0x32);
  HI253WriteCmosSensor(0x38, 0x04);
  HI253WriteCmosSensor(0x3b, 0x22);
  HI253WriteCmosSensor(0x3c, 0xef);
  HI253WriteCmosSensor(0x47, 0xf0);

  //Y_Frame TH
  HI253WriteCmosSensor(0x50, 0x45);
  HI253WriteCmosSensor(0x51, 0x88);

  HI253WriteCmosSensor(0x56, 0x10);
  HI253WriteCmosSensor(0x57, 0xb7);
  HI253WriteCmosSensor(0x58, 0x14);
  HI253WriteCmosSensor(0x59, 0x88);
  HI253WriteCmosSensor(0x5a, 0x04);

  HI253WriteCmosSensor(0x60, 0x55); // AEWGT1
  HI253WriteCmosSensor(0x61, 0x55); // AEWGT2
  HI253WriteCmosSensor(0x62, 0x6a); // AEWGT3
  HI253WriteCmosSensor(0x63, 0xa9); // AEWGT4
  HI253WriteCmosSensor(0x64, 0x6a); // AEWGT5
  HI253WriteCmosSensor(0x65, 0xa9); // AEWGT6
  HI253WriteCmosSensor(0x66, 0x6a); // AEWGT7
  HI253WriteCmosSensor(0x67, 0xa9); // AEWGT8
  HI253WriteCmosSensor(0x68, 0x6b); // AEWGT9
  HI253WriteCmosSensor(0x69, 0xe9); // AEWGT10
  HI253WriteCmosSensor(0x6a, 0x6a); // AEWGT11
  HI253WriteCmosSensor(0x6b, 0xa9); // AEWGT12
  HI253WriteCmosSensor(0x6c, 0x6a); // AEWGT13
  HI253WriteCmosSensor(0x6d, 0xa9); // AEWGT14
  HI253WriteCmosSensor(0x6e, 0x55); // AEWGT15
  HI253WriteCmosSensor(0x6f, 0x55); // AEWGT16
  HI253WriteCmosSensor(0x70, 0x46); // YLVL
  HI253WriteCmosSensor(0x71, 0xBb);

  // haunting control  
  HI253WriteCmosSensor(0x76, 0x21);
  HI253WriteCmosSensor(0x77, 0xBC); //02},
  HI253WriteCmosSensor(0x78, 0x34); //22}, // YTH1
  HI253WriteCmosSensor(0x79, 0x3a); //2a}, // YTH2HI


  HI253WriteCmosSensor(0x7a, 0x23);
  HI253WriteCmosSensor(0x7b, 0x22);
  HI253WriteCmosSensor(0x7d, 0x23);
  HI253WriteCmosSensor(0x83, 0x01); //EXP Normal 33.33 fps 
  HI253WriteCmosSensor(0x84, 0x7c); 
  HI253WriteCmosSensor(0x85, 0x40); 
  HI253WriteCmosSensor(0x86, 0x01); //EXPMin 10416.67 fps
  HI253WriteCmosSensor(0x87, 0x38); 
  HI253WriteCmosSensor(0x88, 0x04); //EXP Max 10.00 fps 
  HI253WriteCmosSensor(0x89, 0xf3); 
  HI253WriteCmosSensor(0x8a, 0x80); 
  HI253WriteCmosSensor(0x8B, 0x7e); //EXP100 
  HI253WriteCmosSensor(0x8C, 0xc0); 
  HI253WriteCmosSensor(0x8D, 0x69); //EXP120 
  HI253WriteCmosSensor(0x8E, 0x6c); 
  HI253WriteCmosSensor(0x91, 0x05); 
  HI253WriteCmosSensor(0x92, 0xe9); 
  HI253WriteCmosSensor(0x93, 0xac); 
  HI253WriteCmosSensor(0x94, 0x04); 
  HI253WriteCmosSensor(0x95, 0x32); 
  HI253WriteCmosSensor(0x96, 0x38); 
  HI253WriteCmosSensor(0x98, 0xdc); // EXPOUT1 DC 9d out target th
  HI253WriteCmosSensor(0x99, 0x45); // EXPOUT2
  HI253WriteCmosSensor(0x9a, 0x0d);
  HI253WriteCmosSensor(0x9b, 0xde);

  HI253WriteCmosSensor(0x9c, 0x07); //EXP Limit 1736.11 fps 
  HI253WriteCmosSensor(0x9d, 0x50); 
  HI253WriteCmosSensor(0x9e, 0x01); //EXP Unit 
  HI253WriteCmosSensor(0x9f, 0x38); 
  HI253WriteCmosSensor(0xa0, 0x03);
  HI253WriteCmosSensor(0xa1, 0xa9);
  HI253WriteCmosSensor(0xa2, 0x80);
  HI253WriteCmosSensor(0xb0, 0x1d); // AG
  HI253WriteCmosSensor(0xb1, 0x1a); // AGMIN
  HI253WriteCmosSensor(0xb2, 0x80); // AGMAX
  HI253WriteCmosSensor(0xb3, 0x20); // AGLVLH //1a
  HI253WriteCmosSensor(0xb4, 0x1a); // AGTH1
  HI253WriteCmosSensor(0xb5, 0x44); // AGTH2
  HI253WriteCmosSensor(0xb6, 0x2f); // AGBTH1
  HI253WriteCmosSensor(0xb7, 0x28); // AGBTH2
  HI253WriteCmosSensor(0xb8, 0x25); // AGBTH3
  HI253WriteCmosSensor(0xb9, 0x22); // AGBTH4
  HI253WriteCmosSensor(0xba, 0x21); // AGBTH5
  HI253WriteCmosSensor(0xbb, 0x20); // AGBTH6
  HI253WriteCmosSensor(0xbc, 0x1f); // AGBTH7
  HI253WriteCmosSensor(0xbd, 0x1f); // AGBTH8
  HI253WriteCmosSensor(0xc0, 0x30); // AGSKY
  HI253WriteCmosSensor(0xc1, 0x20);
  HI253WriteCmosSensor(0xc2, 0x20);
  HI253WriteCmosSensor(0xc3, 0x20);
  HI253WriteCmosSensor(0xc4, 0x08); // AGTIMETH
  HI253WriteCmosSensor(0xc8, 0x80); // DGMAX
  HI253WriteCmosSensor(0xc9, 0x40); // DGMIN   

  HI253SetPage(0x22); 
  HI253WriteCmosSensor(0x10, 0xfd); // AWBCTL1
  HI253WriteCmosSensor(0x11, 0x2e); // AWBCTL2
  HI253WriteCmosSensor(0x19, 0x01); 
  HI253WriteCmosSensor(0x20, 0x30);
  HI253WriteCmosSensor(0x21, 0x80);
  HI253WriteCmosSensor(0x23, 0x08);
  HI253WriteCmosSensor(0x24, 0x01);

  HI253WriteCmosSensor(0x30, 0x80); // ULVL
  HI253WriteCmosSensor(0x31, 0x80); // VLVL
  HI253WriteCmosSensor(0x38, 0x11); // UVTH1
  HI253WriteCmosSensor(0x39, 0x34); // UVTH2
  HI253WriteCmosSensor(0x40, 0xf7); // YRANGE

  HI253WriteCmosSensor(0x41, 0x77); // CDIFF
  HI253WriteCmosSensor(0x42, 0x55); // CSUM2
  HI253WriteCmosSensor(0x43, 0xf0);
  HI253WriteCmosSensor(0x44, 0x66);
  HI253WriteCmosSensor(0x45, 0x33);
  HI253WriteCmosSensor(0x46, 0x01); // WHTPXLTH1  
  HI253WriteCmosSensor(0x47, 0x94);

  HI253WriteCmosSensor(0x50, 0xb2);
  HI253WriteCmosSensor(0x51, 0x81);
  HI253WriteCmosSensor(0x52, 0x98);

  HI253WriteCmosSensor(0x80, 0x38); // RGAIN
  HI253WriteCmosSensor(0x81, 0x20); // GGAIN
  HI253WriteCmosSensor(0x82, 0x38); // BGAIN

  HI253WriteCmosSensor(0x83, 0x5e); // RMAX
  HI253WriteCmosSensor(0x84, 0x20); // RMIN
  HI253WriteCmosSensor(0x85, 0x53); // BMAX
  HI253WriteCmosSensor(0x86, 0x15); // BMIN

  HI253WriteCmosSensor(0x87, 0x54); // RMAXM
  HI253WriteCmosSensor(0x88, 0x20); // RMINM
  HI253WriteCmosSensor(0x89, 0x3f); // BMAXM 
  HI253WriteCmosSensor(0x8a, 0x1c); // BMINM

  HI253WriteCmosSensor(0x8b, 0x54); // RMAXB 
  HI253WriteCmosSensor(0x8c, 0x3f); // RMINB
  HI253WriteCmosSensor(0x8d, 0x24); // BMAXB
  HI253WriteCmosSensor(0x8e, 0x1c); // BMINB

  HI253WriteCmosSensor(0x8f, 0x60); // BGAINPARA1
  HI253WriteCmosSensor(0x90, 0x5f); // BGAINPARA2
  HI253WriteCmosSensor(0x91, 0x5c); // BGAINPARA3
  HI253WriteCmosSensor(0x92, 0x4C); // BGAINPARA4
  HI253WriteCmosSensor(0x93, 0x41); // BGAINPARA5
  HI253WriteCmosSensor(0x94, 0x3b); // BGAINPARA6
  HI253WriteCmosSensor(0x95, 0x36); // BGAINPARA7
  HI253WriteCmosSensor(0x96, 0x30); // BGAINPARA8
  HI253WriteCmosSensor(0x97, 0x27); // BGAINPARA9
  HI253WriteCmosSensor(0x98, 0x20); // BGAINPARA10
  HI253WriteCmosSensor(0x99, 0x1C); // BGAINPARA11
  HI253WriteCmosSensor(0x9a, 0x19); // BGAINPARA12

  HI253WriteCmosSensor(0x9b, 0x88); // BGAINBND1
  HI253WriteCmosSensor(0x9c, 0x88); // BGAINBND2
  HI253WriteCmosSensor(0x9d, 0x48); // RGAINTH1
  HI253WriteCmosSensor(0x9e, 0x38); // RGAINTH2
  HI253WriteCmosSensor(0x9f, 0x30); // RGAINTH3

  HI253WriteCmosSensor(0xa0, 0x74); // RDELTA1
  HI253WriteCmosSensor(0xa1, 0x35); // BDELTA1
  HI253WriteCmosSensor(0xa2, 0xaf); // RDELTA2
  HI253WriteCmosSensor(0xa3, 0xf7); // BDELTA2

  HI253WriteCmosSensor(0xa4, 0x10); // AWBEXPLMT1
  HI253WriteCmosSensor(0xa5, 0x50); // AWBEXPLMT2
  HI253WriteCmosSensor(0xa6, 0xc4); // AWBEXPLMT3

  HI253WriteCmosSensor(0xad, 0x40);
  HI253WriteCmosSensor(0xae, 0x4a);

  HI253WriteCmosSensor(0xaf, 0x2a);
  HI253WriteCmosSensor(0xb0, 0x29);

  HI253WriteCmosSensor(0xb1, 0x20);
  HI253WriteCmosSensor(0xb4, 0xff);
  HI253WriteCmosSensor(0xb8, 0x6b);
  HI253WriteCmosSensor(0xb9, 0x00);

  HI253SetPage(0x24); 
  HI253WriteCmosSensor(0x10, 0x01); // AFCTL1
  HI253WriteCmosSensor(0x18, 0x06);
  HI253WriteCmosSensor(0x30, 0x06);
  HI253WriteCmosSensor(0x31, 0x90);
  HI253WriteCmosSensor(0x32, 0x25);
  HI253WriteCmosSensor(0x33, 0xa2);
  HI253WriteCmosSensor(0x34, 0x26);
  HI253WriteCmosSensor(0x35, 0x58);
  HI253WriteCmosSensor(0x36, 0x60);
  HI253WriteCmosSensor(0x37, 0x00);
  HI253WriteCmosSensor(0x38, 0x50);
  HI253WriteCmosSensor(0x39, 0x00);

  HI253SetPage(0x20); 
  HI253WriteCmosSensor(0x10, 0x9c); // AE ON 50Hz
  HI253SetPage(0x22); 
  HI253WriteCmosSensor(0x10, 0xe9); // AWB ON

  HI253SetPage(0x00); 
  HI253WriteCmosSensor(0x0e, 0x03); // PLL
  HI253WriteCmosSensor(0x0e, 0x73); // PLL ON x2

  HI253SetPage(0x00); // Dummy 750us START
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00); // Dummy 750us END

  HI253SetPage(0x00); 
  HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF
  /*[END]*/
}

void HI253InitPara(void)
{
	spin_lock(&hi253_drv_lock);
  HI253Status.NightMode = KAL_FALSE;
  HI253Status.ZoomFactor = 0;
  HI253Status.Banding = AE_FLICKER_MODE_50HZ;
  HI253Status.PvShutter = 0x17c40;
  HI253Status.MaxFrameRate = HI253_MAX_FPS;
  HI253Status.MiniFrameRate = HI253_FPS(10);
  HI253Status.PvDummyPixels = 424;
  HI253Status.PvDummyLines = 62;
  HI253Status.CapDummyPixels = 360;
  HI253Status.CapDummyLines = 52; /* 10 FPS, 104 for 9.6 FPS*/
  HI253Status.PvOpClk = 26;
  HI253Status.CapOpClk = 26;  
  HI253Status.VDOCTL2 = 0x90;
  HI253Status.ISPCTL3 = 0x30;
  HI253Status.AECTL1 = 0x9c;
  HI253Status.AWBCTL1 = 0xe9;
  	spin_unlock(&hi253_drv_lock);
}

/*************************************************************************
* FUNCTION
*  HI253SetMirror
*
* DESCRIPTION
*  This function mirror, flip or mirror & flip the sensor output image.
*
*  IMPORTANT NOTICE: For some sensor, it need re-set the output order Y1CbY2Cr after
*  mirror or flip.
*
* PARAMETERS
*  1. kal_uint16 : horizontal mirror or vertical flip direction.
*
* RETURNS
*  None
*
*************************************************************************/
static void HI253SetMirror(kal_uint16 ImageMirror)
{
	spin_lock(&hi253_drv_lock);
  HI253Status.VDOCTL2 &= 0xfc;   
  spin_unlock(&hi253_drv_lock);
  switch (ImageMirror)
  {
    case IMAGE_H_MIRROR:
		spin_lock(&hi253_drv_lock);
      HI253Status.VDOCTL2 |= 0x01;
	  spin_unlock(&hi253_drv_lock);
      break;
    case IMAGE_V_MIRROR:
		spin_lock(&hi253_drv_lock);
      HI253Status.VDOCTL2 |= 0x02; 
	  spin_unlock(&hi253_drv_lock);
      break;
    case IMAGE_HV_MIRROR:
		spin_lock(&hi253_drv_lock);
      HI253Status.VDOCTL2 |= 0x03;
	  spin_unlock(&hi253_drv_lock);
      break;
    case IMAGE_NORMAL:
    default:
		spin_lock(&hi253_drv_lock);
      HI253Status.VDOCTL2 |= 0x00; 
	  spin_unlock(&hi253_drv_lock);
  }
  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x11,HI253Status.VDOCTL2);  
}

static void HI253SetAeMode(kal_bool AeEnable)
{
  SENSORDB("[HI253]HI253SetAeMode AeEnable:%d;\n",AeEnable);

  if (AeEnable == KAL_TRUE)
  {
  	spin_lock(&hi253_drv_lock);
    HI253Status.AECTL1 |= 0x80;
	spin_unlock(&hi253_drv_lock);
  }
  else
  {
  	spin_lock(&hi253_drv_lock);
    HI253Status.AECTL1 &= (~0x80);
	spin_unlock(&hi253_drv_lock);
  }
  HI253SetPage(0x20);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);  
}


static void HI253SetAwbMode(kal_bool AwbEnable)
{
  SENSORDB("[HI253]HI253SetAwbMode AwbEnable:%d;\n",AwbEnable);
  if (AwbEnable == KAL_TRUE)
  {
  	spin_lock(&hi253_drv_lock);
    HI253Status.AWBCTL1 |= 0x80;
	spin_unlock(&hi253_drv_lock);
  }
  else
  {
  	spin_lock(&hi253_drv_lock);
    HI253Status.AWBCTL1 &= (~0x80);
	spin_unlock(&hi253_drv_lock);
  }
  HI253SetPage(0x22);
  HI253WriteCmosSensor(0x10,HI253Status.AWBCTL1);  
}

/*************************************************************************
* FUNCTION
* HI253NightMode
*
* DESCRIPTION
* This function night mode of HI253.
*
* PARAMETERS
* none
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HI253NightMode(kal_bool Enable)
{
  kal_uint32 EXPMAX, EXPTIME, BLC_TIME_TH_ONOFF;
  kal_uint32 LineLength,BandingValue;
  SENSORDB("[HI253]HI253NightMode Enable:%d;\n",Enable);
  /* Night mode only for camera preview */
  if (HI253Status.MaxFrameRate == HI253Status.MiniFrameRate)  return ;
  spin_lock(&hi253_drv_lock);
  HI253Status.MiniFrameRate = Enable ? HI253_FPS(5) : HI253_FPS(10);
  spin_unlock(&hi253_drv_lock);
  LineLength = HI253_PV_PERIOD_PIXEL_NUMS + HI253Status.PvDummyPixels;
  BandingValue = (HI253Status.Banding == AE_FLICKER_MODE_50HZ) ? 100 : 120;
  
  EXPTIME = (HI253Status.PvOpClk * 1000000 / LineLength / BandingValue) * BandingValue * LineLength * HI253_FRAME_RATE_UNIT / 8 / HI253Status.MaxFrameRate;
  EXPMAX = (HI253Status.PvOpClk * 1000000 / LineLength / BandingValue) * BandingValue * LineLength * HI253_FRAME_RATE_UNIT / 8 / HI253Status.MiniFrameRate;
  BLC_TIME_TH_ONOFF =  BandingValue * HI253_FRAME_RATE_UNIT / HI253Status.MiniFrameRate;

  SENSORDB("[HI253]LineLenght:%d,BandingValue:%d;MiniFrameRaet:%d\n",LineLength,BandingValue,HI253Status.MiniFrameRate);
  SENSORDB("[HI253]EXPTIME:%d; EXPMAX:%d; BLC_TIME_TH_ONOFF:%d;\n",EXPTIME,EXPMAX,BLC_TIME_TH_ONOFF);
  SENSORDB("[HI253]VDOCTL2:%x; AECTL1:%x; \n",HI253Status.VDOCTL2,HI253Status.AECTL1);

  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x01, 0xf9); // Sleep ON
  spin_lock(&hi253_drv_lock);
  HI253Status.VDOCTL2 &= 0xfb;   
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x11,HI253Status.VDOCTL2);  // Fixed frame rate OFF  
  HI253WriteCmosSensor(0x90, BLC_TIME_TH_ONOFF); // BLC_TIME_TH_ON
  HI253WriteCmosSensor(0x91, BLC_TIME_TH_ONOFF); // BLC_TIME_TH_OFF
  HI253WriteCmosSensor(0x92, 0x78); // BLC_AG_TH_ON
  HI253WriteCmosSensor(0x93, 0x70); // BLC_AG_TH_OFF
  HI253SetPage(0x02); 
  HI253WriteCmosSensor(0xd4, BLC_TIME_TH_ONOFF); // DCDC_TIME_TH_ON
  HI253WriteCmosSensor(0xd5, BLC_TIME_TH_ONOFF); // DCDC_TIME_TH_OFF
  HI253WriteCmosSensor(0xd6, 0x78); // DCDC_AG_TH_ON
  HI253WriteCmosSensor(0xd7, 0x70); // DCDC_AG_TH_OFF
  HI253SetPage(0x20);
  spin_lock(&hi253_drv_lock);
  HI253Status.AECTL1 &= (~0x80);  
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);// AE ON BIT 7    
  HI253WriteCmosSensor(0x18, 0x38); // AE Reset ON
  HI253WriteCmosSensor(0x11, 0x1c); // 0x35 for fixed frame rate
  HI253WriteCmosSensor(0x2a, 0xf0); // 0x35 for fixed frame rate
  HI253WriteCmosSensor(0x2b, 0x34); // 0x35 for fixed frame rate, 0x34 for dynamic frame rate  
  HI253WriteCmosSensor(0x83, (EXPTIME>>16)&(0xff)); // EXPTIMEH max fps
  HI253WriteCmosSensor(0x84, (EXPTIME>>8)&(0xff)); // EXPTIMEM
  HI253WriteCmosSensor(0x85, (EXPTIME>>0)&(0xff)); // EXPTIMEL
  HI253WriteCmosSensor(0x88, (EXPMAX>>16)&(0xff)); // EXPMAXH min fps init 0x04f380
  HI253WriteCmosSensor(0x89, (EXPMAX>>8)&(0xff)); // EXPMAXM
  HI253WriteCmosSensor(0x8a, (EXPMAX>>0)&(0xff)); // EXPMAXL
  HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF  
  spin_lock(&hi253_drv_lock);
  HI253Status.AECTL1 |= 0x80;   
  HI253Status.NightMode = Enable;
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);// AE ON BIT 7    
  HI253WriteCmosSensor(0x18, 0x30); // AE Reset OFF
} /* HI253NightMode */


/*************************************************************************
* FUNCTION
* HI253Open
*
* DESCRIPTION
* this function initialize the registers of CMOS sensor
*
* PARAMETERS
* none
*
* RETURNS
*  none
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI253Open(void)
{
  kal_uint16 SensorId = 0;
  //1 software reset sensor and wait (to sensor)
  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x01,0xf1);
  HI253WriteCmosSensor(0x01,0xf3);
  HI253WriteCmosSensor(0x01,0xf1);

  SensorId = HI253ReadCmosSensor(0x04);
  Sleep(3);
  SENSORDB("[HI253]HI253Open: Sensor ID %x\n",SensorId);
  
  if(SensorId != HI253_SENSOR_ID)
  {
    return ERROR_SENSOR_CONNECT_FAIL;
  //      SensorId = HI253_SENSOR_ID; 
  }
  HI253InitSetting();
  HI253InitPara();
  return ERROR_NONE;

}
/* HI253Open() */

/*************************************************************************
* FUNCTION
*   HI253GetSensorID
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
UINT32 HI253GetSensorID(UINT32 *sensorID) 
{
	//1 software reset sensor and wait (to sensor)
	HI253SetPage(0x00);
	HI253WriteCmosSensor(0x01,0xf1);
	HI253WriteCmosSensor(0x01,0xf3);
	HI253WriteCmosSensor(0x01,0xf1);
	
	*sensorID = HI253ReadCmosSensor(0x04);
	Sleep(3);
	SENSORDB("[HI253]HI253GetSensorID: Sensor ID %x\n",*sensorID);
	
	if(*sensorID != HI253_SENSOR_ID)
	{
        *sensorID = 0xFFFFFFFF; 
	  return ERROR_SENSOR_CONNECT_FAIL;
    //    *sensorID = HI253_SENSOR_ID; 
	}
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
* HI253Close
*
* DESCRIPTION
* This function is to turn off sensor module power.
*
* PARAMETERS
* None
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI253Close(void)
{
  return ERROR_NONE;
} /* HI253Close() */

/*************************************************************************
* FUNCTION
* HI253Preview
*
* DESCRIPTION
* This function start the sensor preview.
*
* PARAMETERS
* *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI253Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
  kal_uint32 LineLength, EXP100, EXP120, EXPMIN, EXPUNIT; 

  SENSORDB("\n\n\n\n\n\n");
  SENSORDB("[HI253]HI253Preview\n");
  /* For change max frame rate only need modify HI253Status.MaxFrameRate */
  spin_lock(&hi253_drv_lock);
  HI253Status.MaxFrameRate = HI253_MAX_FPS;
  spin_unlock(&hi253_drv_lock);
  HI253SetMirror(IMAGE_NORMAL);

	spin_lock(&hi253_drv_lock);
  HI253Status.PvDummyPixels = 424;
  spin_unlock(&hi253_drv_lock);
  LineLength = HI253_PV_PERIOD_PIXEL_NUMS + HI253Status.PvDummyPixels;
  spin_lock(&hi253_drv_lock);
  HI253Status.MiniFrameRate = HI253_FPS(10);  
  HI253Status.PvDummyLines = HI253Status.PvOpClk * 1000000 * HI253_FRAME_RATE_UNIT / LineLength / HI253Status.MaxFrameRate -  HI253_PV_PERIOD_LINE_NUMS; 
  spin_unlock(&hi253_drv_lock);
  
  HI253SetPage(0x00); 
  HI253WriteCmosSensor(0x10, 0x13); 
  HI253WriteCmosSensor(0x12, 0x04); 
  HI253WriteCmosSensor(0x20, 0x00); // WINROWH
  HI253WriteCmosSensor(0x21, 0x04); // WINROWL
  HI253WriteCmosSensor(0x22, 0x00); // WINCOLH
  HI253WriteCmosSensor(0x23, 0x07); // WINCOLL
  
  HI253WriteCmosSensor(0x3f, 0x00);
  HI253WriteCmosSensor(0x40, (HI253Status.PvDummyPixels>>8)&0xff);
  HI253WriteCmosSensor(0x41, (HI253Status.PvDummyPixels>>0)&0xff);
  HI253WriteCmosSensor(0x42, (HI253Status.PvDummyLines>>8)&0xff);
  HI253WriteCmosSensor(0x43, (HI253Status.PvDummyLines>>0)&0xff); 
  HI253WriteCmosSensor(0x3f, 0x02);

  HI253SetPage(0x12);
  HI253WriteCmosSensor(0x20, 0x00);
  HI253WriteCmosSensor(0x21, 0x00);
  HI253WriteCmosSensor(0x90, 0x00);  
  HI253SetPage(0x13);
  HI253WriteCmosSensor(0x80, 0x00);

  EXP100 = (HI253Status.PvOpClk * 1000000 / LineLength) * LineLength / 100 / 8; 
  EXP120 = (HI253Status.PvOpClk * 1000000 / LineLength) * LineLength / 120 / 8; 
  EXPMIN = EXPUNIT = LineLength / 4; 

  SENSORDB("[HI253]DummyPixel:%d DummyLine:%d; LineLenght:%d,Plck:%d\n",HI253Status.PvDummyPixels,HI253Status.PvDummyLines,LineLength,HI253Status.PvOpClk);
  SENSORDB("[HI253]EXP100:%d EXP120:%d;\n",EXP100,EXP120);

  HI253SetPage(0x20);
  HI253WriteCmosSensor(0x83, (HI253Status.PvShutter >> 16) & 0xFF);
  HI253WriteCmosSensor(0x84, (HI253Status.PvShutter >> 8) & 0xFF);
  HI253WriteCmosSensor(0x85, HI253Status.PvShutter & 0xFF);  
  HI253WriteCmosSensor(0x86, (EXPMIN>>8)&0xff);//EXPMIN
  HI253WriteCmosSensor(0x87, (EXPMIN>>0)&0xff);
  HI253WriteCmosSensor(0x8b, (EXP100>>8)&0xff);//EXP100
  HI253WriteCmosSensor(0x8c, (EXP100>>0)&0xff);
  HI253WriteCmosSensor(0x8d, (EXP120>>8)&0xff);//EXP120
  HI253WriteCmosSensor(0x8e, (EXP120>>0)&0xff);
  HI253WriteCmosSensor(0x9c, (HI253_PV_EXPOSURE_LIMITATION>>8)&0xff);
  HI253WriteCmosSensor(0x9d, (HI253_PV_EXPOSURE_LIMITATION>>0)&0xff);  
  HI253WriteCmosSensor(0x9e, (EXPUNIT>>8)&0xff);//EXP Unit
  HI253WriteCmosSensor(0x9f, (EXPUNIT>>0)&0xff);

  HI253SetAeMode(KAL_TRUE);
  HI253SetAwbMode(KAL_TRUE);
  HI253NightMode(HI253Status.NightMode);
  
  return ERROR_NONE;
}/* HI253Preview() */

UINT32 HI253Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
  kal_uint32 LineLength, EXP100, EXP120, EXPMIN, EXPUNIT, CapShutter; 
  kal_uint8 ClockDivider;
  kal_uint32 temp;
  SENSORDB("\n\n\n\n\n\n");
  SENSORDB("[HI253]HI253Capture!!!!!!!!!!!!!\n");
  SENSORDB("[HI253]Image Target Width: %d; Height: %d\n",image_window->ImageTargetWidth, image_window->ImageTargetHeight);
  if ((image_window->ImageTargetWidth<=HI253_PV_WIDTH)&&
      (image_window->ImageTargetHeight<=HI253_PV_HEIGHT))
    return ERROR_NONE;    /* Less than PV Mode */

  HI253WriteCmosSensor(0x01, 0xf9); // Sleep ON
  HI253SetAeMode(KAL_FALSE);
  HI253SetAwbMode(KAL_FALSE);

  HI253SetPage(0x20);
  temp=(HI253ReadCmosSensor(0x80) << 16)|(HI253ReadCmosSensor(0x81) << 8)|HI253ReadCmosSensor(0x82);
  spin_lock(&hi253_drv_lock);
  HI253Status.PvShutter = temp;  
  spin_unlock(&hi253_drv_lock);

  // 1600*1200   
  HI253SetPage(0x00);  
  HI253WriteCmosSensor(0x10,0x00);
  HI253WriteCmosSensor(0x3f,0x00);  
  HI253SetPage(0x12); 
  HI253WriteCmosSensor(0x20, 0x0f);
  HI253WriteCmosSensor(0x21, 0x0f);
  HI253WriteCmosSensor(0x90, 0x5d);    
  HI253SetPage(0x13);
  HI253WriteCmosSensor(0x80, 0xfd);   
  /*capture 1600*1200 start x, y*/
  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x20, 0x00); // WINROWH
  HI253WriteCmosSensor(0x21, 0x0f+2); // WINROWL
  HI253WriteCmosSensor(0x22, 0x00); // WINCOLH
  HI253WriteCmosSensor(0x23, 0x19); // WINCOLL
  spin_lock(&hi253_drv_lock);
  HI253Status.CapDummyPixels = 360;
  HI253Status.CapDummyLines = 52; /* 10 FPS, 104 for 9.6 FPS*/
  spin_unlock(&hi253_drv_lock);
  LineLength = HI253_FULL_PERIOD_PIXEL_NUMS + HI253Status.CapDummyPixels;
  spin_lock(&hi253_drv_lock);
  HI253Status.CapOpClk = 26;  
  spin_unlock(&hi253_drv_lock);
  EXP100 = (HI253Status.CapOpClk * 1000000 / LineLength) * LineLength / 100 / 8; 
  EXP120 = (HI253Status.CapOpClk * 1000000 / LineLength) * LineLength / 120 / 8; 
  EXPMIN = EXPUNIT = LineLength / 4; 
  
  HI253SetPage(0x20);
  HI253WriteCmosSensor(0x86, (EXPMIN>>8)&0xff);//EXPMIN
  HI253WriteCmosSensor(0x87, (EXPMIN>>0)&0xff);
  HI253WriteCmosSensor(0x8b, (EXP100>>8)&0xff);//EXP100
  HI253WriteCmosSensor(0x8c, (EXP100>>0)&0xff);
  HI253WriteCmosSensor(0x8d, (EXP120>>8)&0xff);//EXP120
  HI253WriteCmosSensor(0x8e, (EXP120>>0)&0xff);  
  HI253WriteCmosSensor(0x9c, (HI253_FULL_EXPOSURE_LIMITATION>>8)&0xff);
  HI253WriteCmosSensor(0x9d, (HI253_FULL_EXPOSURE_LIMITATION>>0)&0xff);  
  HI253WriteCmosSensor(0x9e, (EXPUNIT>>8)&0xff);//EXP Unit
  HI253WriteCmosSensor(0x9f, (EXPUNIT>>0)&0xff);

  HI253SetPage(0x00); 
  HI253WriteCmosSensor(0x40, (HI253Status.CapDummyPixels>>8)&0xff);
  HI253WriteCmosSensor(0x41, (HI253Status.CapDummyPixels>>0)&0xff);
  HI253WriteCmosSensor(0x42, (HI253Status.CapDummyLines>>8)&0xff);
  HI253WriteCmosSensor(0x43, (HI253Status.CapDummyLines>>0)&0xff);     

  if(HI253Status.ZoomFactor > 8)   
  {
    ClockDivider = 1; //Op CLock 13M
  }
  else
  {
    ClockDivider = 0; //OpCLock 26M
  }
  SENSORDB("[HI253]ClockDivider: %d \n",ClockDivider);
  HI253WriteCmosSensor(0x12, 0x04|ClockDivider);
  CapShutter = HI253Status.PvShutter >> ClockDivider;
  if(CapShutter<1)      CapShutter = 1;

  HI253SetPage(0x20);
  HI253WriteCmosSensor(0x83, (CapShutter >> 16) & 0xFF);
  HI253WriteCmosSensor(0x84, (CapShutter >> 8) & 0xFF);
  HI253WriteCmosSensor(0x85, CapShutter & 0xFF);  
  HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF  
  SENSORDB("[HI253]CapShutter: %d \n",CapShutter);
  return ERROR_NONE;
} /* HI253Capture() */

UINT32 HI253GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
  pSensorResolution->SensorFullWidth = HI253_FULL_WIDTH;
  pSensorResolution->SensorFullHeight = HI253_FULL_HEIGHT;
  pSensorResolution->SensorPreviewWidth = HI253_PV_WIDTH;
  pSensorResolution->SensorPreviewHeight = HI253_PV_HEIGHT;



  pSensorResolution->SensorVideoWidth=HI253_PV_WIDTH; 
  pSensorResolution->SensorVideoHeight=HI253_PV_HEIGHT;

  
  return ERROR_NONE;
} /* HI253GetResolution() */

UINT32 HI253GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                    MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
  pSensorInfo->SensorPreviewResolutionX=HI253_PV_WIDTH;
  pSensorInfo->SensorPreviewResolutionY=HI253_PV_HEIGHT;
  pSensorInfo->SensorFullResolutionX=HI253_FULL_WIDTH;
  pSensorInfo->SensorFullResolutionY=HI253_FULL_HEIGHT;

  pSensorInfo->SensorCameraPreviewFrameRate=30;
  pSensorInfo->SensorVideoFrameRate=30;
  pSensorInfo->SensorStillCaptureFrameRate=10;
  pSensorInfo->SensorWebCamCaptureFrameRate=15;
  pSensorInfo->SensorResetActiveHigh=FALSE;
  pSensorInfo->SensorResetDelayCount=1;
  pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV; // back for 16 SENSOR_OUTPUT_FORMAT_UYVY;
  pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorInterruptDelayLines = 1;
  pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

  /*pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;

  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
  pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;*/
  pSensorInfo->CaptureDelayFrame = 3; 
  pSensorInfo->PreviewDelayFrame = 3; 
  pSensorInfo->VideoDelayFrame = 4; 
  pSensorInfo->SensorMasterClockSwitch = 0; 
  
  pSensorInfo->YUVAwbDelayFrame = 3; 
  pSensorInfo->YUVEffectDelayFrame = 2; 

  
  pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA; 

  switch (ScenarioId)
  {
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
      pSensorInfo->SensorClockFreq=26;
      pSensorInfo->SensorClockDividCount=3;
      pSensorInfo->SensorClockRisingCount=0;
      pSensorInfo->SensorClockFallingCount=2;
      pSensorInfo->SensorPixelClockCount=3;
      pSensorInfo->SensorDataLatchCount=2;
      pSensorInfo->SensorGrabStartX = HI253_GRAB_START_X+3; 
      pSensorInfo->SensorGrabStartY = HI253_GRAB_START_Y;
      break;
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    //case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
    //case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    //case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
    default:
      pSensorInfo->SensorClockFreq=26;
      pSensorInfo->SensorClockDividCount=3;
      pSensorInfo->SensorClockRisingCount=0;
      pSensorInfo->SensorClockFallingCount=2;
      pSensorInfo->SensorPixelClockCount=3;
      pSensorInfo->SensorDataLatchCount=2;
      pSensorInfo->SensorGrabStartX = HI253_GRAB_START_X+3; 
      pSensorInfo->SensorGrabStartY = HI253_GRAB_START_Y+1;
      break;
  }
  return ERROR_NONE;
} /* HI253GetInfo() */


UINT32 HI253Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
  switch (ScenarioId)
  {
  case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
  case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
  //case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
    HI253Preview(pImageWindow, pSensorConfigData);
    break;
  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
  //case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
    HI253Capture(pImageWindow, pSensorConfigData);
    break;
  default:
    break; 
  }
  return TRUE;
} /* HI253Control() */


BOOL HI253SetWb(UINT16 Para)
{
  SENSORDB("[HI253]HI253SetWb Para:%d;\n",Para);
  switch (Para)
  {
    case AWB_MODE_OFF:
      HI253SetAwbMode(KAL_FALSE);
      break;                     
    case AWB_MODE_AUTO:
      HI253SetAwbMode(KAL_TRUE);
      break;
    case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
      HI253SetAwbMode(KAL_FALSE);
      HI253WriteCmosSensor(0x80, 0x49);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x24);
      HI253WriteCmosSensor(0x83, 0x50);
      HI253WriteCmosSensor(0x84, 0x45);
      HI253WriteCmosSensor(0x85, 0x24);
      HI253WriteCmosSensor(0x86, 0x1e);
      break;
    case AWB_MODE_DAYLIGHT: //sunny
      HI253SetAwbMode(KAL_FALSE);
      HI253WriteCmosSensor(0x80, 0x45);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x27);
      HI253WriteCmosSensor(0x83, 0x44);
      HI253WriteCmosSensor(0x84, 0x3f);
      HI253WriteCmosSensor(0x85, 0x29);
      HI253WriteCmosSensor(0x86, 0x23);
      break;
    case AWB_MODE_INCANDESCENT: //office
      HI253SetAwbMode(KAL_FALSE);
      HI253WriteCmosSensor(0x80, 0x33);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x3d);
      HI253WriteCmosSensor(0x83, 0x2e);
      HI253WriteCmosSensor(0x84, 0x24);
      HI253WriteCmosSensor(0x85, 0x43);
      HI253WriteCmosSensor(0x86, 0x3d);
      break;
    case AWB_MODE_TUNGSTEN: //home
      HI253SetAwbMode(KAL_FALSE);
      HI253WriteCmosSensor(0x80, 0x25);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x44);
      HI253WriteCmosSensor(0x83, 0x22);
      HI253WriteCmosSensor(0x84, 0x1e);
      HI253WriteCmosSensor(0x85, 0x50);
      HI253WriteCmosSensor(0x86, 0x45);
      break;
    case AWB_MODE_FLUORESCENT:
      HI253SetAwbMode(KAL_FALSE);
      HI253WriteCmosSensor(0x80, 0x45);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x2f);
      HI253WriteCmosSensor(0x83, 0x38);
      HI253WriteCmosSensor(0x84, 0x32);
      HI253WriteCmosSensor(0x85, 0x39);
      HI253WriteCmosSensor(0x86, 0x33);
    default:
      return KAL_FALSE;
  }
  return KAL_TRUE;      
} /* HI253SetWb */

BOOL HI253SetEffect(UINT16 Para)
{
  SENSORDB("[HI253]HI253SetEffect Para:%d;\n",Para);
  switch (Para)
  {
    case MEFFECT_OFF:
      HI253SetPage(0x10);  
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x30);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x40, 0x00);
      break;
    case MEFFECT_SEPIA:
      HI253SetPage(0x10);  
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x23);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x40, 0x00);
      HI253WriteCmosSensor(0x44, 0x70);
      HI253WriteCmosSensor(0x45, 0x98);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    case MEFFECT_NEGATIVE://----datasheet
      HI253SetPage(0x10);  
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x28);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x40, 0x00);
      HI253WriteCmosSensor(0x44, 0x80);
      HI253WriteCmosSensor(0x45, 0x80);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    case MEFFECT_SEPIAGREEN://----datasheet aqua
      HI253SetPage(0x10);  
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x23);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x40, 0x00);
      HI253WriteCmosSensor(0x44, 0x80);
      HI253WriteCmosSensor(0x45, 0x04);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    case MEFFECT_SEPIABLUE:
      HI253SetPage(0x10);  
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x23);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x40, 0x00);
      HI253WriteCmosSensor(0x44, 0xb0);
      HI253WriteCmosSensor(0x45, 0x40);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    case MEFFECT_MONO: //----datasheet black & white
      HI253SetPage(0x10);  
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x23);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x40, 0x00);
      HI253WriteCmosSensor(0x44, 0x80);
      HI253WriteCmosSensor(0x45, 0x80);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    default:
      return KAL_FALSE;
  }
  return KAL_TRUE;

} /* HI253SetEffect */

BOOL HI253SetBanding(UINT16 Para)
{
  SENSORDB("[HI253]HI253SetBanding Para:%d;\n",Para);
  spin_lock(&hi253_drv_lock);
  HI253Status.Banding = Para;
  spin_unlock(&hi253_drv_lock);
  if (HI253Status.Banding == AE_FLICKER_MODE_50HZ) 
  	{
  	spin_lock(&hi253_drv_lock);
    HI253Status.AECTL1 |= 0x10;
	spin_unlock(&hi253_drv_lock);
  	}
  else
  	{
  	spin_lock(&hi253_drv_lock);
    HI253Status.AECTL1 &= (~0x10); 
	spin_unlock(&hi253_drv_lock);
  	}
  
  HI253SetPage(0x20);  
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);
  return TRUE;
} /* HI253SetBanding */

BOOL HI253SetExposure(UINT16 Para)
{
  SENSORDB("[HI253]HI253SetExposure Para:%d;\n",Para);
  HI253SetPage(0x10);  
  spin_lock(&hi253_drv_lock);
  HI253Status.ISPCTL3 |= 0x10;
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x12,HI253Status.ISPCTL3);//make sure the Yoffset control is opened.
  
  switch (Para)
  {
    case AE_EV_COMP_n13:              /* EV -2 */
      HI253WriteCmosSensor(0x40,0xe0);
      break;
    case AE_EV_COMP_n10:              /* EV -1.5 */
      HI253WriteCmosSensor(0x40,0xc8);
      break;
    case AE_EV_COMP_n07:              /* EV -1 */
      HI253WriteCmosSensor(0x40,0xb0);
      break;
    case AE_EV_COMP_n03:              /* EV -0.5 */
      HI253WriteCmosSensor(0x40,0x98);
      break;
    case AE_EV_COMP_00:                /* EV 0 */
      HI253WriteCmosSensor(0x40,0x85);
      break;
    case AE_EV_COMP_03:              /* EV +0.5 */
      HI253WriteCmosSensor(0x40,0x18);
      break;
    case AE_EV_COMP_07:              /* EV +1 */
      HI253WriteCmosSensor(0x40,0x30);
      break;
    case AE_EV_COMP_10:              /* EV +1.5 */
      HI253WriteCmosSensor(0x40,0x48);
      break;
    case AE_EV_COMP_13:              /* EV +2 */
      HI253WriteCmosSensor(0x40,0x60);
      break;
    default:
      return KAL_FALSE;
  }
  return KAL_TRUE;
} /* HI253SetExposure */

UINT32 HI253YUVSensorSetting(FEATURE_ID Cmd, UINT32 Para)
{
  switch (Cmd) {
    case FID_SCENE_MODE:
		
  		SENSORDB("[HI253]FID_SCENE_MODE: %d;\n",Para);
      if (Para == SCENE_MODE_OFF)
      {
        HI253NightMode(KAL_FALSE); 
      }
      else if (Para == SCENE_MODE_NIGHTSCENE)
      {
        HI253NightMode(KAL_TRUE); 
      }  
      break; 
    case FID_AWB_MODE:
      HI253SetWb(Para);
      break;
    case FID_COLOR_EFFECT:
      HI253SetEffect(Para);
      break;
    case FID_AE_EV:
      HI253SetExposure(Para);
      break;
    case FID_AE_FLICKER:
      HI253SetBanding(Para);
      break;
    case FID_AE_SCENE_MODE: 
      if (Para == AE_MODE_OFF) 
      {
        HI253SetAeMode(KAL_FALSE);
      }
      else 
      {
        HI253SetAeMode(KAL_TRUE);
      }
      break; 
    case FID_ZOOM_FACTOR:
      SENSORDB("[HI253]ZoomFactor :%d;\n",Para);
	  spin_lock(&hi253_drv_lock);
      HI253Status.ZoomFactor = Para;
	  spin_unlock(&hi253_drv_lock);
      break;
    default:
      break;
  }
  return TRUE;
}   /* HI253YUVSensorSetting */

UINT32 HI253YUVSetVideoMode(UINT16 FrameRate)
{
  kal_uint32 EXPFIX, BLC_TIME_TH_ONOFF;
  kal_uint32 LineLength,BandingValue;
  
  SENSORDB("[HI253]HI253YUVSetVideoMode FrameRate:%d;\n",FrameRate);
  if (FrameRate * HI253_FRAME_RATE_UNIT > HI253_MAX_FPS)
    return -1;
  spin_lock(&hi253_drv_lock);
  HI253Status.MaxFrameRate = HI253Status.MiniFrameRate = FrameRate * HI253_FRAME_RATE_UNIT;
  spin_unlock(&hi253_drv_lock);
  LineLength = HI253_PV_PERIOD_PIXEL_NUMS + HI253Status.PvDummyPixels;
  BandingValue = (HI253Status.Banding == AE_FLICKER_MODE_50HZ) ? 100 : 120;
  
  EXPFIX = (HI253Status.PvOpClk * 1000000 / LineLength / BandingValue) * BandingValue * LineLength * HI253_FRAME_RATE_UNIT / 8 / HI253Status.MiniFrameRate;
  
  BLC_TIME_TH_ONOFF =  BandingValue * HI253_FRAME_RATE_UNIT / HI253Status.MiniFrameRate;

  SENSORDB("[HI253]LineLenght:%d,BandingValue:%d\n",LineLength,BandingValue);
  SENSORDB("[HI253]EXPFIX:%d BLC_TIME_TH_ONOFF:%d\n;",EXPFIX,BLC_TIME_TH_ONOFF);

  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x01, 0xf9); // Sleep ON
  spin_lock(&hi253_drv_lock);
  HI253Status.VDOCTL2 |= 0x04;   
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x11,HI253Status.VDOCTL2);  // Fixed frame rate OFF    
  HI253WriteCmosSensor(0x90, BLC_TIME_TH_ONOFF); // BLC_TIME_TH_ON
  HI253WriteCmosSensor(0x91, BLC_TIME_TH_ONOFF); // BLC_TIME_TH_OFF
  HI253WriteCmosSensor(0x92, 0x78); // BLC_AG_TH_ON
  HI253WriteCmosSensor(0x93, 0x70); // BLC_AG_TH_OFF
  HI253WriteCmosSensor(0x03, 0x02); // Page 2
  HI253WriteCmosSensor(0xd4, BLC_TIME_TH_ONOFF); // DCDC_TIME_TH_ON
  HI253WriteCmosSensor(0xd5, BLC_TIME_TH_ONOFF); // DCDC_TIME_TH_OFF
  HI253WriteCmosSensor(0xd6, 0x78); // DCDC_AG_TH_ON
  HI253WriteCmosSensor(0xd7, 0x70); // DCDC_AG_TH_OFF
  
  HI253SetPage(0x20);
  spin_lock(&hi253_drv_lock);
  HI253Status.AECTL1 &= (~0x80);
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);// AE ON BIT 7    
  HI253WriteCmosSensor(0x18, 0x38); // AE Reset ON
  HI253WriteCmosSensor(0x11, 0x00); // 0x35 for fixed frame rate
  HI253WriteCmosSensor(0x2a, 0x03); // 0x35 for fixed frame rate
  HI253WriteCmosSensor(0x2b, 0x35); // 0x35 for fixed frame rate, 0x34 for dynamic frame rate  
  HI253WriteCmosSensor(0x83, (EXPFIX>>16)&(0xff)); // EXPTIMEH max fps
  HI253WriteCmosSensor(0x84, (EXPFIX>>8)&(0xff)); // EXPTIMEM
  HI253WriteCmosSensor(0x85, (EXPFIX>>0)&(0xff)); // EXPTIMEL
  HI253WriteCmosSensor(0x88, (EXPFIX>>16)&(0xff)); // EXPMAXH min fps
  HI253WriteCmosSensor(0x89, (EXPFIX>>8)&(0xff)); // EXPMAXM
  HI253WriteCmosSensor(0x8a, (EXPFIX>>0)&(0xff)); // EXPMAXL
  HI253WriteCmosSensor(0x91, (EXPFIX>>16)&(0xff)); // EXPMAXH min fps
  HI253WriteCmosSensor(0x92, (EXPFIX>>8)&(0xff)); // EXPMAXM
  HI253WriteCmosSensor(0x93, (EXPFIX>>0)&(0xff)); // EXPMAXL  
  HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF
  spin_lock(&hi253_drv_lock);
  HI253Status.AECTL1 |= 0x80;
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);// AE ON BIT 7    
  HI253WriteCmosSensor(0x18, 0x30); // AE Reset OFF  
  return TRUE;
}

UINT32 HI253FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
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
      *pFeatureReturnPara16++=HI253_FULL_WIDTH;
      *pFeatureReturnPara16=HI253_FULL_HEIGHT;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_GET_PERIOD:
      *pFeatureReturnPara16++=HI253_PV_PERIOD_PIXEL_NUMS+HI253Status.PvDummyPixels;
      *pFeatureReturnPara16=HI253_PV_PERIOD_LINE_NUMS+HI253Status.PvDummyLines;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
      *pFeatureReturnPara32 = HI253Status.PvOpClk*2;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_SET_ESHUTTER:
      break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
      HI253NightMode((BOOL) *pFeatureData16);
      break;
    case SENSOR_FEATURE_SET_GAIN:
      case SENSOR_FEATURE_SET_FLASHLIGHT:
      break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
      break;
    case SENSOR_FEATURE_SET_REGISTER:
      HI253WriteCmosSensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
      break;
    case SENSOR_FEATURE_GET_REGISTER:
      pSensorRegData->RegData = HI253ReadCmosSensor(pSensorRegData->RegAddr);
      break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
      *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
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
      *pFeatureReturnPara32++=0;
      *pFeatureParaLen=4;
      break; 
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
      // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
      // if EEPROM does not exist in camera module.
      *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_SET_YUV_CMD:
      HI253YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
      break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:
      HI253YUVSetVideoMode(*pFeatureData16);
      break; 
  case SENSOR_FEATURE_CHECK_SENSOR_ID:
	  HI253GetSensorID(pFeatureReturnPara32); 
	  break; 
    default:
      break;
  }
  return ERROR_NONE;
} /* HI253FeatureControl() */



UINT32 HI253_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
  static SENSOR_FUNCTION_STRUCT SensorFuncHI253=
  {
    HI253Open,
    HI253GetInfo,
    HI253GetResolution,
    HI253FeatureControl,
    HI253Control,
    HI253Close
  };

  /* To Do : Check Sensor status here */
  if (pfFunc!=NULL)
    *pfFunc=&SensorFuncHI253;

  return ERROR_NONE;
} /* SensorInit() */
