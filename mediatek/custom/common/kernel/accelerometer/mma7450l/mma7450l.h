#ifndef MMA7450L_H
#define MMA7450L_H 
	 
#include <linux/ioctl.h>
	 

/*  MMA7455L Register Map  (Please refer to  MMA7455L Specifications) */
#define XOUTL           0x00
#define XOUTH           0x01
#define YOUTL           0x02
#define YOUTH           0x03
#define ZOUTL           0x04
#define ZOUTH           0x05
#define XOUT8           0x06
#define YOUT8           0x07
#define ZOUT8           0x08
#define STATUS          0x09
#define DETSRC          0x0A
#define TOUT            0x0B
#define I2CAD           0x0D
#define USRINF          0x0E
#define WHOAMI          0x0F
#define XOFFL           0x10
#define XOFFH           0x11
#define YOFFL           0x12
#define YOFFH           0x13
#define ZOFFL           0x14
#define ZOFFH           0x15
#define MCTL            0x16
#define INTRST          0x17
#define CTL1            0x18
#define CTL2            0x19
#define LDTH            0x1A
#define PDTH            0x1B
#define PW              0x1C
#define LT              0x1D
#define TW              0x1E
/* ----------------------------------------------------------*/

#define MMA7450L_I2C_SLAVE_ADDR		0x3A  /* 0xA6 */

/* MMA7450L Register Map  (Please refer to MMA7450L Specifications) */
#define MMA7450L_REG_DEVID			0x0F //changed
/*
#define MMA7450L_REG_THRESH_TAP		0x1D
#define MMA7450L_REG_OFSX			0x1E
#define MMA7450L_REG_OFSY			0x1F
#define MMA7450L_REG_OFSZ			0x20
#define MMA7450L_REG_DUR				0x21
#define MMA7450L_REG_THRESH_ACT		0x24
#define MMA7450L_REG_THRESH_INACT	0x25
#define MMA7450L_REG_TIME_INACT		0x26
#define MMA7450L_REG_ACT_INACT_CTL	0x27
#define MMA7450L_REG_THRESH_FF		0x28
#define MMA7450L_REG_TIME_FF			0x29
#define MMA7450L_REG_TAP_AXES		0x2A
#define MMA7450L_REG_ACT_TAP_STATUS	0x2B
#define	MMA7450L_REG_BW_RATE			0x2C
#define MMA7450L_REG_POWER_CTL		0x2D
#define MMA7450L_REG_INT_ENABLE		0x2E
#define MMA7450L_REG_INT_MAP			0x2F
#define MMA7450L_REG_INT_SOURCE		0x30
#define MMA7450L_REG_DATA_FORMAT		0x31
#define MMA7450L_REG_DATAX0			0x32
#define MMA7450L_REG_FIFO_CTL		0x38
#define MMA7450L_REG_FIFO_STATUS		0x39

#define MMA7450L_FIXED_DEVID			0xE5
*/

#define MMA7450L_BW_250HZ			0x80
#define MMA7450L_BW_125HZ			0x00

#define MMA7450L_MEASURE_MODE		0x08	
#define MMA7450L_DATA_READY			0x80

#define MMA7450L_FULL_RES			0x08
#define MMA7450L_RANGE_2G			0x00
#define MMA7450L_RANGE_4G			0x01
#define MMA7450L_RANGE_8G			0x02
#define MMA7450L_RANGE_16G			0x03
#define MMA7450L_SELF_TEST           0x80

#define MMA7450L_STREAM_MODE			0x80
#define MMA7450L_SAMPLES_15			0x0F

#define MMA7450L_FS_8G_LSB_G			64
#define MMA7450L_FS_4G_LSB_G			128
#define MMA7450L_FS_2G_LSB_G			256

#define MMA7450L_LEFT_JUSTIFY		0x04
#define MMA7450L_RIGHT_JUSTIFY		0x00


#define MMA7450L_SUCCESS						0
#define MMA7450L_ERR_I2C						-1
#define MMA7450L_ERR_STATUS					-3
#define MMA7450L_ERR_SETUP_FAILURE			-4
#define MMA7450L_ERR_GETGSENSORDATA			-5
#define MMA7450L_ERR_IDENTIFICATION			-6

/* IOCTLs for mma7450l misc. device library */
#define MMA7450LIO						   	0x85
#define MMA7450L_IOCTL_INIT                  _IO(MMA7450LIO,  0x01)
#define MMA7450L_IOCTL_READ_CHIPINFO         _IOR(MMA7450LIO, 0x02, int)
#define MMA7450L_IOCTL_READ_SENSORDATA       _IOR(MMA7450LIO, 0x03, int)


#define MMA7450L_BUFSIZE				256

#endif


