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
#ifndef __CM3623_H__
#define __CM3623_H__

#include <linux/ioctl.h>

/*ALS Command*/
#define SD_ALS      (1      << 0)
#define WDM         (1      << 1)
#define IT_ALS      (0x03   << 2)
#define THD_ALS     (0x03   << 4)
#define GAIN_ALS    (0x03   << 6)

/*Proximity sensor command*/
#define SD_PS       (1      << 0)
#define INT_PS      (1      << 2)
#define INT_ALS     (1      << 3)
#define IT_PS       (0x03   << 4)
#define DR_PS       (0x03   << 6)

#endif

