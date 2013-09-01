#ifndef MC3410_H
#define MC3410_H
	 
#include <linux/ioctl.h>
	 
#define MC3210_I2C_SLAVE_ADDR		0x98  



/* MC3210 Register Map  (Please refer to MC3410 Specifications) */

#define MC3410_REG_EVENT_ENABLE		0x03 
#define MC3410_REG_STATE		    0x04 

#define MC3410_REG_INT_ENABLE		0x06   
#define MC3410_REG_POWER_CTL		0x07

#define MC3410_REG_TAP_ENABLE		0x09 
#define MC3410_REG_DATAX0			0x0D 
#define MC3410_REG_DEVID			0x18 
#define MC3410_REG_DATA_FORMAT	    0x20  
#define MC3410_REG_BW_RATE	        0x20  

#define MC3410_REG_OFSX		        0x21 

#define MC3410_WAKE_MODE		    0x01  
#define MC3410_STANDBY_MODE			0x03 


#define MC3410_RANGE_2G				0x00  
#define MC3410_RANGE_4G				0x04   
#define MC3410_RANGE_8G				0x08 
#define MC3410_RANGE_8G_14BIT		0x0C 
#define MC3410_RANGE_MUSTWRITE      0x03   
#define MC3410_RANGE_MASK			0x0C 

#define MC3410_BW_MASK				0x70  



#define MC3410_BW_512HZ				0x00   
#define MC3410_BW_256HZ				0x10 
#define MC3410_BW_128HZ				0x20 
#define MC3410_BW_64HZ				0x30 
#define MC3410_BW_32HZ				0x40   
#define MC3410_BW_16HZ				0x50 
#define MC3410_BW_8HZ				0x60 

#define MC3410_FIXED_DEVID			0x02


#define MC3410_BUFSIZE				256


#define MC3410_SUCCESS						 0
#define MC3410_ERR_I2C						-1
#define MC3410_ERR_STATUS					-3
#define MC3410_ERR_SETUP_FAILURE			-4
#define MC3410_ERR_GETGSENSORDATA	        -5
#define MC3410_ERR_IDENTIFICATION			-6

#endif

