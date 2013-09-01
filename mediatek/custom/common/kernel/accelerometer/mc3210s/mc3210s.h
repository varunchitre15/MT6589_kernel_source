#ifndef MC3210_H
#define MC3210_H
	 
#include <linux/ioctl.h>
	 
#define MC3210_I2C_SLAVE_ADDR		0x98  
	 
	 /* MC3210 Register Map  (Please refer to MC3210 Specifications) */
#define MC3210_REG_INT_ENABLE		0x06   
#define MC3210_REG_POWER_CTL		0x07   
#define MC3210_WAKE_MODE		       0x01   
#define MC3210_STANDBY_MODE		0x03   
#define MC3210_REG_DATAX0			0x0D   
#define MC3210_REG_DEVID			0x18   
#define MC3210_REG_DATA_FORMAT	0x20  
#define MC3210_RANGE_MUSTWRITE     0x03   
#define MC3210_RANGE_MASK			0x0C   
#define MC3210_RANGE_2G			0x00  
#define MC3210_RANGE_4G			0x04   
#define MC3210_RANGE_8G			0x08   
#define MC3210_RANGE_8G_14BIT		0x0C  
#define MC3210_REG_BW_RATE	       0x20  
#define MC3210_BW_MASK			0x70  
#define MC3210_BW_512HZ			0x00   
#define MC3210_BW_256HZ			0x10 
#define MC3210_BW_128HZ			0x20 
#define MC3210_BW_64HZ				0x30   

#define MC3210_FIXED_DEVID			0x88
	 
#define MC3210_SUCCESS						 0
#define MC3210_ERR_I2C						-1
#define MC3210_ERR_STATUS					-3
#define MC3210_ERR_SETUP_FAILURE			-4
#define MC3210_ERR_GETGSENSORDATA	       -5
#define MC3210_ERR_IDENTIFICATION			-6
	 
	 
#define MC3210_BUFSIZE				256
	 
#endif

