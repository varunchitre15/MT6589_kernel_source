/* -------------------------------------------------------------------------
 * Copyright (C) 2010 Inside Secure
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ------------------------------------------------------------------------- */

/*
 * Open NFC driver module.
 *
 */

#include <linux/cdev.h>
#include <linux/poll.h>

#include "open_nfc_int.h"

/* function prototypes */

int open_nfc_custom_init(void);
void open_nfc_custom_exit(void);
int open_nfc_custom_open(struct inode *inode, struct file *filp);
int open_nfc_custom_release(struct inode *inode, struct file *filp);
ssize_t open_nfc_custom_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t open_nfc_custom_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
unsigned int open_nfc_custom_poll(struct file * filp, poll_table *wait);
long open_nfc_custom_ioctl_configure(struct file *filp, unsigned int cmd, unsigned long arg);
long open_nfc_custom_ioctl_connect(struct file *filp, unsigned int cmd, unsigned long arg);
long open_nfc_custom_ioctl_reset(struct file *filp, unsigned int cmd, unsigned long arg);

/* EOF */
