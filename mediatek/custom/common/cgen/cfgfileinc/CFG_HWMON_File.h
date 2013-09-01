


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_HWMON_FILE.h
 *
 * Project:
 * --------
 *   YUSU
 *
 * Description:
 * ------------
 *   Hheader file of HWMON(all HW sensor) CFG file
 *
 * Author:
 * -------
 *   MTK02404(MingHsien Hsieh)
 *
 *******************************************************************************/

#ifndef _CFG_HWMON_FILE_H
#define _CFG_HWMON_FILE_H
/********************************************************************************
 * Accelerometer
 *******************************************************************************/
#define C_HWMON_ACC_AXES    3
/*-----------------------------------------------------------------------------*/
typedef struct
{
    int offset[C_HWMON_ACC_AXES];
} NVRAM_HWMON_ACC_STRUCT;
/*-----------------------------------------------------------------------------*/
#define CFG_FILE_HWMON_ACC_REC_SIZE    sizeof(NVRAM_HWMON_ACC_STRUCT)
#define CFG_FILE_HWMON_ACC_REC_TOTAL   1
/*-----------------------------------------------------------------------------*/

/********************************************************************************
 * Gyroscope
 *******************************************************************************/
#define C_HWMON_GYRO_AXES    3
/*-----------------------------------------------------------------------------*/
typedef struct
{
    int offset[C_HWMON_GYRO_AXES];
} NVRAM_HWMON_GYRO_STRUCT;
/*-----------------------------------------------------------------------------*/
#define CFG_FILE_HWMON_GYRO_REC_SIZE    sizeof(NVRAM_HWMON_GYRO_STRUCT)
#define CFG_FILE_HWMON_GYRO_REC_TOTAL   1
/*-----------------------------------------------------------------------------*/

#endif


