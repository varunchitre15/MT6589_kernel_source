#ifndef DMARD08_H
#define DMARD08_H
	 
#include <linux/ioctl.h>
	 
	#define DMARD08_I2C_SLAVE_WRITE_ADDR		0x38
	 
	 
	#define DMARD08_REG_DATAXLOW			0x02
	

	
#define DMARD08_SUCCESS						0
#define DMARD08_ERR_I2C						-1
#define DMARD08_ERR_STATUS					-3
#define DMARD08_ERR_SETUP_FAILURE			-4
#define DMARD08_ERR_GETGSENSORDATA			-5
#define DMARD08_ERR_IDENTIFICATION			-6
	 
	 
	 
#define DMARD08_BUFSIZE				256
	 
#endif

