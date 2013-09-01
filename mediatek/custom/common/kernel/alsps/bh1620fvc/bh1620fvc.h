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
 * Definitions for cm3623 als/ps sensor chip.
 */
#ifndef __BH1620FVC_H__
#define __BH1620FVC_H__

#include <linux/ioctl.h>

/*ALSPS REGS*/
#define ALS_CMD          0x01

#define GAIN00_ALS           0x00
#define GAIN01_ALS           0x40
#define GAIN10_ALS           0x80
#define GAIN11_ALS           0xC0
#define IT00_ALS               0x00
#define IT01_ALS               0x04
#define IT10_ALS               0x08
#define IT11_ALS               0x0C
#define ALS_DT1                0x02
#define ALS_DT2                0x03
#define ALS_INT                 0x04
#define THD00_ALS            0x00
#define THD01_ALS            0x40
#define THD10_ALS            0x80
#define THD11_ALS            0xC0
#define PRST00_ALS          0x00
#define PRST01_ALS          0x10
#define PRST10_ALS          0x20
#define PRST11_ALS          0x30
#define FLAG_ALS              0x08

#define PS_CMD           0x09
#define PS_DT             0x0A
#define PS_THDH         0x0B
#define PS_THDL         0x0C

/*ALS Command*/
#define SD_ALS      (1      << 0)
#define INT_ALS     (1      << 1)
#define IT_ALS      (0x03   << 2)
#define THD_ALS     (0x03   << 4)
#define GAIN_ALS    (0x03   << 6)

/*Proximity sensor command*/
#define SD_PS       (1      << 0)
#define INT_PS      (1      << 1)
#define IT_PS       (0x03   << 2)
#define DR_PS       (1      << 4)
#define SLP_PS      (0x03   << 5)
#define INTM_PS     (1      << 7)

#endif

