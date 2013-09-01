/* 
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
 * Definitions for CM36283 als/ps sensor chip.
 */
#ifndef __CM36283_H__
#define __CM36283_H__

#include <linux/ioctl.h>

/*cm36283 als/ps sensor register related macro*/
#define CM36283_REG_ALS_CONF 		0X00
#define CM36283_REG_ALS_THDH 		0X01
#define CM36283_REG_ALS_THDL 		0X02
#define CM36283_REG_PS_CONF1_2		0X03
#define CM36283_REG_PS_CONF3_MS		0X04
#define CM36283_REG_PS_CANC			0X05
#define CM36283_REG_PS_THD			0X06
#define CM36283_REG_PS_DATA			0X08
#define CM36283_REG_ALS_DATA		0X09
#define CM36283_REG_INT_FLAG		0X0B
#define CM36283_REG_ID_MODE			0X0C

// Reg.
#define REG_ADR_00	0x00	// Read & Write
#define REG_ADR_01	0x01	// Read & Write
#define REG_ADR_02	0x02	// Read & Write
#define REG_ADR_03	0x03	// Read & Write
#define REG_ADR_04	0x04	// Read & Write
#define REG_ADR_05	0x05	// Read & Write
#define REG_ADR_06	0x06	// Read & Write
#define REG_ADR_07	0x07	// Read & Write
#define REG_ADR_08	0x08	// Read & Write
#define REG_ADR_09	0x09	// Read & Write
#define REG_ADR_0A	0x0A	// Read & Write
#define REG_ADR_0B	0x0B	// Read & Write
#define REG_ADR_0C	0x0C	// Read  Only
#define REG_ADR_0D	0x0D	// Read  Only
#define REG_ADR_0E	0x0E	// Read  Only
#define REG_ADR_0F	0x0F	// Read  Only
#define REG_ADR_10	0x10	// Read  Only
#define REG_ADR_11	0x11	// Read  Only

#define LOW_LUX_MODE		( 0 )
#define HIGH_LUX_MODE		( 1 )

// Reg. 00H
#define	OP_SHUTSOWN		0x0C	// OP3:0
#define	OP_RUN			0x80	// OP3:1
#define	OP_CONTINUOUS		0x40	// OP2:1
#define	OP_PS_ALS		0x00	// OP01:00
#define	OP_ALS			0x10	// OP01:01
#define	OP_PS			0x20	// OP01:10
#define	OP_COUNT		0x30	// OP01:11

// Reg. 01H
#define	PRST_1			0x00	// PRST:00
#define	PRST_4			0x40	// PRST:01
#define	PRST_8			0x80	// PRST:10
#define	PRST_16			0xC0	// PRST:11
#define	RES_A_14		0x20	// RES_A:100
#define	RES_A_16		0x18	// RES_A:011
#define	RANGE_A_8		0x03	// RANGE_A:011
#define	RANGE_A_32		0x05	// RANGE_A:101
#define	RANGE_A_64		0x06	// RANGE_A:110
#define	RANGE_A_128		0x07	// RANGE_A:111

// Reg. 02H
#define	INTTYPE_L		0x00	// INTTYPE:0
#define	INTTYPE_P		0x40	// INTTYPE:1
#define	RES_P_14		0x08	// RES_P:001
#define	RES_P_12		0x10	// RES_P:010
#define	RES_P_10		0x18	// RES_P:011
#define	RANGE_P_4		0x02	// RANGE_P:010
#define	RANGE_P_8		0x03	// RANGE_P:011

// Reg. 03H
#define	INTVAL_0		0x00	// INTVAL:00
#define	INTVAL_4		0x40	// INTVAL:01
#define	INTVAL_8		0x80	// INTVAL:10
#define	INTVAL_16		0xC0	// INTVAL:11
#define	IS_110			0x30	// IS:11
#define	PIN_INT			0x00	// PIN:00
#define	PIN_INT_ALS		0x04	// PIN:01
#define	PIN_INT_PS		0x08	// PIN:10
#define	PIN_DETECT		0x0C	// PIN:11
#define	FREQ_327_5		0x00	// FREQ:0
#define	FREQ_81_8		0x02	// FREQ:1
#define	RST			0x01	// RST:1

/*CM36283 related driver tag macro*/
#define CM36283_SUCCESS				 		 0
#define CM36283_ERR_I2C						-1
#define CM36283_ERR_STATUS					-3
#define CM36283_ERR_SETUP_FAILURE			-4
#define CM36283_ERR_GETGSENSORDATA			-5
#define CM36283_ERR_IDENTIFICATION			-6


#endif

