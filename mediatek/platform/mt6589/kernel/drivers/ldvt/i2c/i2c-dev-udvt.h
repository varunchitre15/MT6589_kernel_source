/*
    i2c-dev.h - i2c-bus driver, char device interface

    Copyright (C) 1995-97 Simon G. Vogl
    Copyright (C) 1998-99 Frodo Looijaard <frodol@dds.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _LINUX_I2C_DEV_H
#define _LINUX_I2C_DEV_H

#include <linux/types.h>
#include <linux/compiler.h>

/* /dev/i2c-X ioctl commands.  The ioctl's parameter is always an
 * unsigned long, except for:
 *	- I2C_FUNCS, takes pointer to an unsigned long
 *	- I2C_RDWR, takes pointer to struct i2c_rdwr_ioctl_data
 *	- I2C_SMBUS, takes pointer to struct i2c_smbus_ioctl_data
 */
#define I2C_RETRIES	0x0701	/* number of times a device address should
				   be polled when not acknowledging */
#define I2C_TIMEOUT	0x0702	/* set timeout in units of 10 ms */

/* NOTE: Slave address is 7 or 10 bits, but 10-bit addresses
 * are NOT supported! (due to code brokenness)
 */
#define I2C_SLAVE	0x0703	/* Use this slave address */
#define I2C_SLAVE_FORCE	0x0706	/* Use this slave address, even if it
				   is already in use by a driver! */
#define I2C_TENBIT	0x0704	/* 0 for 7 bit addrs, != 0 for 10 bit */

#define I2C_FUNCS	0x0705	/* Get the adapter functionality mask */

#define I2C_RDWR	0x0707	/* Combined R/W transfer (one STOP only) */

#define I2C_PEC		0x0708	/* != 0 to use PEC with SMBus */
#define I2C_SMBUS	0x0720	/* SMBus transfer */

#define I2C_UVVF_REGISTER_N1 0x0731
#define I2C_UVVF_FASTSPEED_N2 0x0732
#define I2C_UVVF_HIGHSPEED_N3 0x0733
#define I2C_UVVF_DMA_N4 0x0734
#define I2C_UVVF_DMA_N5 0x0735
#define I2C_UVVF_REPEAT_N6 0x0736
#define I2C_UVVF_REPEAT_N7 0x0737
#define I2C_UVVF_REPEAT_N8 0x0738
#define I2C_UVVF_REPEAT_N9 0x0739
#define I2C_UVVF_POLL_N10 0x0740
#define I2C_UVVF_POLL_N11 0x0741
#define I2C_UVVF_POLL_N12 0x0742
#define I2C_UVVF_POLL_N13 0x0743
#define I2C_UVVF_FIFO_N14 0x0744
#define I2C_UVVF_TIMING_N15 0x0745
#define I2C_UVVF_TIMING_N16 0x0746
#define I2C_UVVF_TIMING_N17 0x0747
#define I2C_UVVF_TIMING_N18 0x0748
#define I2C_UVVF_CON_N19 0x0749
#define I2C_UVVF_CON_N20 0x0750
#define I2C_UVVF_CLKEN_N21 0x0751
#define I2C_UVVF_CLKEN_N22 0x0752
#define I2C_UVVF_ACKERR_N23 0x0753
#define I2C_UVVF_8MORE_N24 0x0754

/* This is the structure as used in the I2C_SMBUS ioctl call */
struct i2c_smbus_ioctl_data {
	__u8 read_write;
	__u8 command;
	__u32 size;
	union i2c_smbus_data __user *data;
};

/* This is the structure as used in the I2C_RDWR ioctl call */
struct i2c_rdwr_ioctl_data {
	struct i2c_msg __user *msgs;	/* pointers to i2c_msgs */
	__u32 nmsgs;			/* number of i2c_msgs */
};

#define  I2C_RDRW_IOCTL_MAX_MSGS	42

#ifdef __KERNEL__
#define I2C_MAJOR	89		/* Device major number		*/
#endif

#endif /* _LINUX_I2C_DEV_H */
