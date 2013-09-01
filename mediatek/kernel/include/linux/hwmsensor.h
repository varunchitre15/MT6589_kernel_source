/* alps/ALPS_SW/TRUNK/MAIN/alps/kernel/include/linux/hwmsensor.h
 *
 * (C) Copyright 2009 
 * MediaTek <www.MediaTek.com>
 *
 * MT6516 Sensor IOCTL & data structure
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

#ifndef __HWMSENSOR_H__
#define __HWMSENSOR_H__

#include <linux/ioctl.h>

#define SENSOR_TYPE_ACCELEROMETER       1
#define SENSOR_TYPE_MAGNETIC_FIELD      2
#define SENSOR_TYPE_ORIENTATION         3
#define SENSOR_TYPE_GYROSCOPE           4
#define SENSOR_TYPE_LIGHT               5
#define SENSOR_TYPE_PRESSURE            6
#define SENSOR_TYPE_TEMPERATURE         7
#define SENSOR_TYPE_PROXIMITY           8
#define SENSOR_TYPE_GRAVITY             9
#define SENSOR_TYPE_LINEAR_ACCELERATION 10
#define SENSOR_TYPE_ROTATION_VECTOR     11


/*---------------------------------------------------------------------------*/
#define ID_BASE							0
#define ID_ORIENTATION					(ID_BASE+SENSOR_TYPE_ORIENTATION-1)
#define ID_MAGNETIC						(ID_BASE+SENSOR_TYPE_MAGNETIC_FIELD-1)
#define ID_ACCELEROMETER				(ID_BASE+SENSOR_TYPE_ACCELEROMETER-1)
#define ID_LINEAR_ACCELERATION			(ID_BASE+SENSOR_TYPE_LINEAR_ACCELERATION-1)
#define ID_ROTATION_VECTOR				(ID_BASE+SENSOR_TYPE_ROTATION_VECTOR-1)
#define ID_GRAVITY						(ID_BASE+SENSOR_TYPE_GRAVITY-1)
#define ID_GYROSCOPE					(ID_BASE+SENSOR_TYPE_GYROSCOPE-1)
#define ID_PROXIMITY					(ID_BASE+SENSOR_TYPE_PROXIMITY-1)
#define ID_LIGHT						(ID_BASE+SENSOR_TYPE_LIGHT-1)
#define ID_PRESSURE						(ID_BASE+SENSOR_TYPE_PRESSURE-1)
#define ID_TEMPRERATURE					(ID_BASE+SENSOR_TYPE_TEMPERATURE-1)
#define ID_SENSOR_MAX_HANDLE			(ID_BASE+10)
#define ID_NONE							(ID_BASE+16)

#define MAX_ANDROID_SENSOR_NUM	(ID_SENSOR_MAX_HANDLE +1)

/*---------------------------------------------------------------------------*/
#define SENSOR_ORIENTATION				(1 << ID_ORIENTATION)
#define SENSOR_MAGNETIC					(1 << ID_MAGNETIC)
#define SENSOR_ACCELEROMETER			(1 << ID_ACCELEROMETER)
#define SENSOR_GYROSCOPE				(1 << ID_GYROSCOPE)
#define SENSOR_PROXIMITY				(1 << ID_PROXIMITY)
#define SENSOR_LIGHT					(1 << ID_LIGHT)
#define SENSOR_PRESSURE					(1 << ID_PRESSURE)
#define SENSOR_TEMPRERATURE				(1 << ID_TEMPRERATURE)
#define SENSOR_GRAVITY					(1 << ID_GRAVITY)
#define SENSOR_LINEAR_ACCELERATION		(1 << ID_LINEAR_ACCELERATION)
#define SENSOR_ROTATION_VECTOR			(1 << ID_ROTATION_VECTOR)

/*----------------------------------------------------------------------------*/
#define HWM_INPUTDEV_NAME               "hwmdata"
#define HWM_SENSOR_DEV_NAME             "hwmsensor"
#define HWM_SENSOR_DEV                  "/dev/hwmsensor"
#define C_MAX_HWMSEN_EVENT_NUM          4 
/*----------------------------------------------------------------------------*/

#define EVENT_TYPE_SENSOR				0x01
#define EVENT_SENSOR_ACCELERATION		SENSOR_ACCELEROMETER
#define EVENT_SENSOR_MAGNETIC			SENSOR_MAGNETIC
#define EVENT_SENSOR_ORIENTATION		SENSOR_ORIENTATION
#define EVENT_SENSOR_GYROSCOPE			SENSOR_GYROSCOPE
#define EVENT_SENSOR_LIGHT				SENSOR_LIGHT
#define EVENT_SENSOR_PRESSURE			SENSOR_PRESSURE
#define EVENT_SENSOR_TEMPERATURE		SENSOR_TEMPRERATURE
#define EVENT_SENSOR_PROXIMITY			SENSOR_PROXIMITY
#define EVENT_SENSOR_GRAVITY			SENSOR_PRESSURE
#define EVENT_SENSOR_LINEAR_ACCELERATION		SENSOR_TEMPRERATURE
#define EVENT_SENSOR_ROTATION_VECTOR	SENSOR_PROXIMITY
/*-----------------------------------------------------------------------------*/

enum {
    HWM_MODE_DISABLE = 0,
    HWM_MODE_ENABLE  = 1,    
};

/*------------sensors data----------------------------------------------------*/
typedef struct {
	/* sensor identifier */
	int 	sensor;
	/* sensor values */
	int	values[3];
	/* sensor values divide */
	uint32_t value_divide;
	/* sensor accuracy*/
	int8_t status;
	/* whether updata? */
	int	update;
	/* time is in nanosecond */
	int64_t	time;

	uint32_t	reserved;
}hwm_sensor_data;

typedef struct {
	hwm_sensor_data data[MAX_ANDROID_SENSOR_NUM];
	int date_type;
}hwm_trans_data;

/*----------------------------------------------------------------------------*/
#define HWM_IOC_MAGIC           0x91

/* set delay */
#define HWM_IO_SET_DELAY		_IOW(HWM_IOC_MAGIC, 0x01, uint32_t)

/* wake up */
#define HWM_IO_SET_WAKE			_IO(HWM_IOC_MAGIC, 0x02)

/* Enable/Disable  sensor */
#define HWM_IO_ENABLE_SENSOR	_IOW(HWM_IOC_MAGIC, 0x03, uint32_t)
#define HWM_IO_DISABLE_SENSOR	_IOW(HWM_IOC_MAGIC, 0x04, uint32_t)

/* Enable/Disable sensor */
#define HWM_IO_ENABLE_SENSOR_NODATA		_IOW(HWM_IOC_MAGIC, 0x05, uint32_t)
#define HWM_IO_DISABLE_SENSOR_NODATA	_IOW(HWM_IOC_MAGIC, 0x06, uint32_t)
/* Get sensors data */
#define HWM_IO_GET_SENSORS_DATA			_IOWR(HWM_IOC_MAGIC, 0x07, hwm_trans_data)

#endif // __HWMSENSOR_H__
