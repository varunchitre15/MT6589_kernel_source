#ifndef KXTF9_H
#define KXTF9_H
	 
#include <linux/ioctl.h>
	 
#define KXTF9_I2C_SLAVE_ADDR		0x1E
	 
 /* KXTF9 Register Map  (Please refer to KXTF9 Specifications) */
#define KXTF9_REG_DEVID			0x0F
#define	KXTF9_REG_BW_RATE			0x21
#define KXTF9_REG_POWER_CTL		0x1B
#define KXTF9_REG_CTL_REG3		0x1D
#define KXTF9_DCST_RESP			0x0C
#define KXTF9_REG_DATA_FORMAT		0x1B
#define KXTF9_REG_DATA_RESOLUTION		0x1B
#define KXTF9_RANGE_DATA_RESOLUTION_MASK	0x40
#define KXTF9_REG_DATAX0			0x06	 
#define KXTF9_FIXED_DEVID			0x12	 
#define KXTF9_BW_200HZ				0x04
#define KXTF9_BW_100HZ				0x03
#define KXTF9_BW_50HZ				0x02	 
#define KXTF9_MEASURE_MODE		0x80		 
#define KXTF9_RANGE_MASK		0x18
#define KXTF9_RANGE_2G			0x00
#define KXTF9_RANGE_4G			0x08
#define KXTF9_RANGE_8G			0x10
#define KXTF9_REG_INT_ENABLE	0x1E

#define KXTF9_SELF_TEST           0x10
	 	 
	 
#define KXTF9_SUCCESS						0
#define KXTF9_ERR_I2C						-1
#define KXTF9_ERR_STATUS					-3
#define KXTF9_ERR_SETUP_FAILURE				-4
#define KXTF9_ERR_GETGSENSORDATA			-5
#define KXTF9_ERR_IDENTIFICATION			-6
	 
	 
	 
#define KXTF9_BUFSIZE				256
	 
#endif

