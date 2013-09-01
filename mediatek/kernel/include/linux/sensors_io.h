/*
*
* (C) Copyright 2008 
* MediaTek <www.mediatek.com>
*
* Sensors IO command file for MT6516
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef SENSORS_IO_H
#define SENSORS_IO_H
		  
#include <linux/ioctl.h>	  

typedef struct {
	unsigned short	x;		/**< X axis */
	unsigned short	y;		/**< Y axis */
	unsigned short	z;		/**< Z axis */
} GSENSOR_VECTOR3D;

typedef struct{
	int x;
	int y;
	int z;
}SENSOR_DATA;


#define GSENSOR						   	0x85
#define GSENSOR_IOCTL_INIT                  _IO(GSENSOR,  0x01)
#define GSENSOR_IOCTL_READ_CHIPINFO         _IOR(GSENSOR, 0x02, int)
#define GSENSOR_IOCTL_READ_SENSORDATA       _IOR(GSENSOR, 0x03, int)
#define GSENSOR_IOCTL_READ_OFFSET			_IOR(GSENSOR, 0x04, GSENSOR_VECTOR3D)
#define GSENSOR_IOCTL_READ_GAIN				_IOR(GSENSOR, 0x05, GSENSOR_VECTOR3D)
#define GSENSOR_IOCTL_READ_RAW_DATA			_IOR(GSENSOR, 0x06, int)
#define GSENSOR_IOCTL_SET_CALI				_IOW(GSENSOR, 0x06, SENSOR_DATA)
#define GSENSOR_IOCTL_GET_CALI				_IOW(GSENSOR, 0x07, SENSOR_DATA)
#define GSENSOR_IOCTL_CLR_CALI				_IO(GSENSOR, 0x08)




/* IOCTLs for Msensor misc. device library */
#define MSENSOR						   0x83
#define MSENSOR_IOCTL_INIT					_IO(MSENSOR, 0x01)
#define MSENSOR_IOCTL_READ_CHIPINFO			_IOR(MSENSOR, 0x02, int)
#define MSENSOR_IOCTL_READ_SENSORDATA		_IOR(MSENSOR, 0x03, int)
#define MSENSOR_IOCTL_READ_POSTUREDATA		_IOR(MSENSOR, 0x04, int)
#define MSENSOR_IOCTL_READ_CALIDATA			_IOR(MSENSOR, 0x05, int)
#define MSENSOR_IOCTL_READ_CONTROL			_IOR(MSENSOR, 0x06, int)
#define MSENSOR_IOCTL_SET_CONTROL			_IOW(MSENSOR, 0x07, int)
#define MSENSOR_IOCTL_SET_MODE           	_IOW(MSENSOR, 0x08, int)
#define MSENSOR_IOCTL_SET_POSTURE        	_IOW(MSENSOR, 0x09, int)
#define MSENSOR_IOCTL_SET_CALIDATA     	  	_IOW(MSENSOR, 0x0a, int)
#define MSENSOR_IOCTL_SENSOR_ENABLE         _IOW(MSENSOR, 0x51, int)
#define MSENSOR_IOCTL_READ_FACTORY_SENSORDATA  _IOW(MSENSOR, 0x52, int)


/* IOCTLs for AKM library */
#define ECS_IOCTL_WRITE                 _IOW(MSENSOR, 0x0b, char*)
#define ECS_IOCTL_READ                  _IOWR(MSENSOR, 0x0c, char*)
#define ECS_IOCTL_RESET      	        _IO(MSENSOR, 0x0d) /* NOT used in AK8975 */
#define ECS_IOCTL_SET_MODE              _IOW(MSENSOR, 0x0e, short)
#define ECS_IOCTL_GETDATA               _IOR(MSENSOR, 0x0f, char[SENSOR_DATA_SIZE])
#define ECS_IOCTL_SET_YPR               _IOW(MSENSOR, 0x10, short[12])
#define ECS_IOCTL_GET_OPEN_STATUS       _IOR(MSENSOR, 0x11, int)
#define ECS_IOCTL_GET_CLOSE_STATUS      _IOR(MSENSOR, 0x12, int)
#define ECS_IOCTL_GET_OSENSOR_STATUS	_IOR(MSENSOR, 0x13, int)
#define ECS_IOCTL_GET_DELAY             _IOR(MSENSOR, 0x14, short)
#define ECS_IOCTL_GET_PROJECT_NAME      _IOR(MSENSOR, 0x15, char[64])
#define ECS_IOCTL_GET_MATRIX            _IOR(MSENSOR, 0x16, short [4][3][3])
#define	ECS_IOCTL_GET_LAYOUT			_IOR(MSENSOR, 0x17, int[3])

#define ECS_IOCTL_GET_OUTBIT        	_IOR(MSENSOR, 0x23, char)
#define ECS_IOCTL_GET_ACCEL         	_IOR(MSENSOR, 0x24, short[3])
#define MMC31XX_IOC_RM					_IO(MSENSOR, 0x25)
#define MMC31XX_IOC_RRM					_IO(MSENSOR, 0x26)


/* IOCTLs for MMC31XX device */
#define MMC31XX_IOC_TM					_IO(MSENSOR, 0x18)
#define MMC31XX_IOC_SET					_IO(MSENSOR, 0x19)
#define MMC31XX_IOC_RESET				_IO(MSENSOR, 0x1a)
#define MMC31XX_IOC_READ				_IOR(MSENSOR, 0x1b, int[3])
#define MMC31XX_IOC_READXYZ				_IOR(MSENSOR, 0x1c, int[3])

#define ECOMPASS_IOC_GET_DELAY			_IOR(MSENSOR, 0x1d, int)
#define ECOMPASS_IOC_GET_MFLAG			_IOR(MSENSOR, 0x1e, short)
#define	ECOMPASS_IOC_GET_OFLAG			_IOR(MSENSOR, 0x1f, short)
#define ECOMPASS_IOC_GET_OPEN_STATUS	_IOR(MSENSOR, 0x20, int)
#define ECOMPASS_IOC_SET_YPR			_IOW(MSENSOR, 0x21, int[12])
#define ECOMPASS_IOC_GET_LAYOUT			_IOR(MSENSOR, 0X22, int)




#define ALSPS							0X84
#define ALSPS_SET_PS_MODE					_IOW(ALSPS, 0x01, int)
#define ALSPS_GET_PS_MODE					_IOR(ALSPS, 0x02, int)
#define ALSPS_GET_PS_DATA					_IOR(ALSPS, 0x03, int)
#define ALSPS_GET_PS_RAW_DATA				_IOR(ALSPS, 0x04, int)
#define ALSPS_SET_ALS_MODE					_IOW(ALSPS, 0x05, int)
#define ALSPS_GET_ALS_MODE					_IOR(ALSPS, 0x06, int)
#define ALSPS_GET_ALS_DATA					_IOR(ALSPS, 0x07, int)
#define ALSPS_GET_ALS_RAW_DATA           	_IOR(ALSPS, 0x08, int)
/*-------------------yucong add-------------------------------------------*/
#define ALSPS_GET_PS_TEST_RESULT           	_IOR(ALSPS, 0x09, int)
#define ALSPS_GET_ALS_TEST_RESULT           	_IOR(ALSPS, 0x0A, int)
#define ALSPS_GET_PS_THRESHOLD_HIGH           	_IOR(ALSPS, 0x0B, int)
#define ALSPS_GET_PS_THRESHOLD_LOW           	_IOR(ALSPS, 0x0C, int)
#define ALSPS_GET_ALS_THRESHOLD_HIGH           	_IOR(ALSPS, 0x0D, int)
#define ALSPS_GET_ALS_THRESHOLD_LOW           	_IOR(ALSPS, 0x0E, int)


#define GYROSCOPE							0X86
#define GYROSCOPE_IOCTL_INIT					_IO(GYROSCOPE, 0x01)
#define GYROSCOPE_IOCTL_SMT_DATA			_IOR(GYROSCOPE, 0x02, int)
#define GYROSCOPE_IOCTL_READ_SENSORDATA		_IOR(GYROSCOPE, 0x03, int)
#define GYROSCOPE_IOCTL_SET_CALI			_IOW(GYROSCOPE, 0x04, SENSOR_DATA)
#define GYROSCOPE_IOCTL_GET_CALI			_IOW(GYROSCOPE, 0x05, SENSOR_DATA)
#define GYROSCOPE_IOCTL_CLR_CALI			_IO(GYROSCOPE, 0x06)

#define BROMETER							0X87
#define BAROMETER_IOCTL_INIT				_IO(BROMETER, 0x01)
#define BAROMETER_GET_PRESS_DATA			_IOR(BROMETER, 0x02, int)
#define BAROMETER_GET_TEMP_DATA			    _IOR(BROMETER, 0x03, int)
#define BAROMETER_IOCTL_READ_CHIPINFO		_IOR(BROMETER, 0x04, int)




#endif

