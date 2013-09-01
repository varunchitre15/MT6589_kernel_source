#ifndef LIS3DE_H
#define LIS3DE_H
	 
#include <linux/ioctl.h>

#define LIS3DE_I2C_SLAVE_ADDR		0x30//0x30<-> SD0=GND;0x32<-> SD0=High
	 
	 /* LIS3DE Register Map  (Please refer to LIS3DE Specifications) */
#define LIS3DE_REG_CTL_REG1		0x20
#define LIS3DE_REG_CTL_REG2		0x21
#define LIS3DE_REG_CTL_REG3		0x22
#define LIS3DE_REG_CTL_REG4		0x23
#define LIS3DE_REG_DATAX0		    0x28
#define LIS3DE_REG_OUT_X		    0x28
#define LIS3DE_REG_OUT_Y		    0x2A
#define LIS3DE_REG_OUT_Z		    0x2C



#define LIS3DE_FIXED_DEVID			0xE5
	 
#define LIS3DE_BW_200HZ			0x60
#define LIS3DE_BW_100HZ			0x50 //400 or 100 on other choise //changed
#define LIS3DE_BW_50HZ				0x40

#define	LIS3DE_FULLRANG_LSB		0XFF
	 
#define LIS3DE_MEASURE_MODE		0x08	//changed 
#define LIS3DE_DATA_READY			0x07    //changed
	 
//#define LIS3DE_FULL_RES			0x08
#define LIS3DE_RANGE_2G			0x00
#define LIS3DE_RANGE_4G			0x10
#define LIS3DE_RANGE_8G			0x20 //8g or 2g no ohter choise//changed
//#define LIS3DE_RANGE_16G			0x30 //8g or 2g no ohter choise//changed

#define LIS3DE_SELF_TEST           0x10 //changed
	 
#define LIS3DE_STREAM_MODE			0x80
#define LIS3DE_SAMPLES_15			0x0F
	 
#define LIS3DE_FS_8G_LSB_G			0x20
#define LIS3DE_FS_4G_LSB_G			0x10
#define LIS3DE_FS_2G_LSB_G			0x00
	 
#define LIS3DE_LEFT_JUSTIFY		0x04
#define LIS3DE_RIGHT_JUSTIFY		0x00
	 
	 
#define LIS3DE_SUCCESS						0
#define LIS3DE_ERR_I2C						-1
#define LIS3DE_ERR_STATUS					-3
#define LIS3DE_ERR_SETUP_FAILURE			-4
#define LIS3DE_ERR_GETGSENSORDATA			-5
#define LIS3DE_ERR_IDENTIFICATION			-6
	 
	 
	 
#define LIS3DE_BUFSIZE				256
	 
#endif

