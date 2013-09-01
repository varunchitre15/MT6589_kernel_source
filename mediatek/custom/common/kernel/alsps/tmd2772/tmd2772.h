/* 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*
 * Definitions for tmd2772 als/ps sensor chip.
 */
#ifndef __TMD2772_H__
#define __TMD2772_H__

#include <linux/ioctl.h>

extern int TMD2772_CMM_PPCOUNT_VALUE;
extern int ZOOM_TIME;
extern int TMD2772_CMM_CONTROL_VALUE;

#define TMD2772_CMM_ENABLE 		0X80
#define TMD2772_CMM_ATIME 		0X81
#define TMD2772_CMM_PTIME 		0X82
#define TMD2772_CMM_WTIME 		0X83
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
#define TMD2772_CMM_INT_LOW_THD_LOW   0X88
#define TMD2772_CMM_INT_LOW_THD_HIGH  0X89
#define TMD2772_CMM_INT_HIGH_THD_LOW  0X8A
#define TMD2772_CMM_INT_HIGH_THD_HIGH 0X8B
#define TMD2772_CMM_Persistence       0X8C
#define TMD2772_CMM_STATUS            0X93
#define TAOS_TRITON_CMD_REG           0X80
#define TAOS_TRITON_CMD_SPL_FN        0x60

#define TMD2772_CMM_CONFIG 		0X8D
#define TMD2772_CMM_PPCOUNT 		0X8E
#define TMD2772_CMM_CONTROL 		0X8F

#define TMD2772_CMM_PDATA_L 		0X98
#define TMD2772_CMM_PDATA_H 		0X99
#define TMD2772_CMM_C0DATA_L 	0X94
#define TMD2772_CMM_C0DATA_H 	0X95
#define TMD2772_CMM_C1DATA_L 	0X96
#define TMD2772_CMM_C1DATA_H 	0X97


#define TMD2772_SUCCESS						0
#define TMD2772_ERR_I2C						-1
#define TMD2772_ERR_STATUS					-3
#define TMD2772_ERR_SETUP_FAILURE				-4
#define TMD2772_ERR_GETGSENSORDATA			-5
#define TMD2772_ERR_IDENTIFICATION			-6


#endif
