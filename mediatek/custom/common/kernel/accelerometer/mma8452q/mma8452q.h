#ifndef MMA8452Q_H
#define MMA8452Q_H 
	 
#include <linux/ioctl.h>

extern struct acc_hw* mma8452q_get_cust_acc_hw(void); 

	 
#define MMA8452Q_I2C_SLAVE_ADDR		0x1C //0x3A<->SA0=1 ;0x38<->SA0=0
	 
	 /* MMA8452Q Register Map  (Please refer to MMA8452Q Specifications) */

#define MMA8452Q_REG_DEVID			0x0D //use  device id = 0x3A
#define MMA8452Q_REG_CTL_REG1		0x2A //use
#define MMA8452Q_REG_CTL_REG2		0x2B //use
#define MMA8452Q_REG_CTL_REG3		0x2C //use INT control
#define MMA8452Q_REG_CTL_REG4		0x2D //use INT enalbe
#define MMA8452Q_REG_CTL_REG5		0x2E //use





//#define MMA8452Q_REG_THRESH_TAP		0x1D
#define MMA8452Q_REG_OFSX			0x2F //use
#define MMA8452Q_REG_OFSY			0x30 //use
#define MMA8452Q_REG_OFSZ			0x31 //use

//#define MMA8452Q_REG_DUR				0x21
//#define MMA8452Q_REG_THRESH_ACT		0x24
//#define MMA8452Q_REG_THRESH_INACT	0x25
//#define MMA8452Q_REG_TIME_INACT		0x26
//#define MMA8452Q_REG_ACT_INACT_CTL	0x27
//#define MMA8452Q_REG_THRESH_FF		0x28
//#define MMA8452Q_REG_TIME_FF			0x29
//#define MMA8452Q_REG_TAP_AXES		0x2A
//#define MMA8452Q_REG_ACT_TAP_STATUS	0x2B
//#define	MMA8452Q_REG_BW_RATE			0x2C
//#define MMA8452Q_REG_POWER_CTL		0x2D
//#define MMA8452Q_REG_INT_ENABLE		0x2E
//#define MMA8452Q_REG_INT_MAP			0x2F
//#define MMA8452Q_REG_INT_SOURCE		0x30
#define MMA8452Q_REG_XYZ_DATA_CFG		0x0E //use
#define MMA8452Q_REG_DATAX0			0x01 //use

//#define MMA8452Q_REG_FIFO_CTL		0x38
//#define MMA8452Q_REG_FIFO_STATUS		0x39

//end define register



#define MMA8452Q_FIXED_DEVID			0x2A //use
	 
#define MMA8452Q_BW_200HZ			0x10 //use
#define MMA8452Q_BW_100HZ			0x18 //use
#define MMA8452Q_BW_50HZ			0x20 //use

//#define	MMA8452Q_FULLRANG_LSB		0XFF
	 
#define MMA8452Q_MEASURE_MODE		0x01 //use	
//#define MMA8452Q_DATA_READY			0x80
	 
#define MMA8452Q_12BIT_RES			0x02 //changed not ready
#define MMA8452Q_RANGE_2G			0x00 //use
#define MMA8452Q_RANGE_4G			0x01 //use
#define MMA8452Q_RANGE_8G			0x02 //use

//#define MMA8452Q_SELF_TEST           0x80
	 
//#define MMA8452Q_STREAM_MODE			0x80
//#define MMA8452Q_SAMPLES_15			0x0F
	 
//#define MMA8452Q_FS_8G_LSB_G			64
//#define MMA8452Q_FS_4G_LSB_G			128
//#define MMA8452Q_FS_2G_LSB_G			256
	 
#define MMA8452Q_LEFT_JUSTIFY		0x04
#define MMA8452Q_RIGHT_JUSTIFY		0x00
	 
	 
#define MMA8452Q_SUCCESS						0
#define MMA8452Q_ERR_I2C						-1
#define MMA8452Q_ERR_STATUS					-3
#define MMA8452Q_ERR_SETUP_FAILURE			-4
#define MMA8452Q_ERR_GETGSENSORDATA			-5
#define MMA8452Q_ERR_IDENTIFICATION			-6
	 
	 
	 
#define MMA8452Q_BUFSIZE				256
	 
#endif

