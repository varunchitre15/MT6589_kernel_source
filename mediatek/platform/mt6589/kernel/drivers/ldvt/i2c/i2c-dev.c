/*
    i2c-dev.c - i2c-bus driver, char device interface

    Copyright (C) 1995-97 Simon G. Vogl
    Copyright (C) 1998-99 Frodo Looijaard <frodol@dds.nl>
    Copyright (C) 2003 Greg Kroah-Hartman <greg@kroah.com>

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

/* Note that this is a complete rewrite of Simon Vogl's i2c-dev module.
   But I have used so much of his original code and ideas that it seems
   only fair to recognize him as co-author -- Frodo */

/* The I2C_RDWR ioctl code is written by Kolja Waschk <waschk@telos.de> */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include "i2c-dev-udvt.h"
#include <linux/smp_lock.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/dma-mapping.h>

#define mt6573_I2C_DATA_PORT		((base) + 0x0000)
#define mt6573_I2C_SLAVE_ADDR		((base) + 0x0004)
#define mt6573_I2C_INTR_MASK		((base) + 0x0008)
#define mt6573_I2C_INTR_STAT		((base) + 0x000c)
#define mt6573_I2C_CONTROL			((base) + 0x0010)
#define mt6573_I2C_TRANSFER_LEN	    ((base) + 0x0014)
#define mt6573_I2C_TRANSAC_LEN	    ((base) + 0x0018)
#define mt6573_I2C_DELAY_LEN		((base) + 0x001c)
#define mt6573_I2C_TIMING			((base) + 0x0020)
#define mt6573_I2C_START			((base) + 0x0024)
#define mt6573_I2C_FIFO_STAT		((base) + 0x0030)
#define mt6573_I2C_FIFO_THRESH	    ((base) + 0x0034)
#define mt6573_I2C_FIFO_ADDR_CLR	((base) + 0x0038)
#define mt6573_I2C_IO_CONFIG		((base) + 0x0040)
#define mt6573_I2C_DEBUG			((base) + 0x0044)
#define mt6573_I2C_HS				((base) + 0x0048)
#define mt6573_I2C_DEBUGSTAT		((base) + 0x0064)
#define mt6573_I2C_DEBUGCTRL		((base) + 0x0068)

static struct i2c_driver i2cdev_driver;

/*
 * An i2c_dev represents an i2c_adapter ... an I2C or SMBus master, not a
 * slave (i2c_client) with which messages will be exchanged.  It's coupled
 * with a character special file which is accessed by user mode drivers.
 *
 * The list of i2c_dev structures is parallel to the i2c_adapter lists
 * maintained by the driver model, and is updated using notifications
 * delivered to the i2cdev_driver.
 */
struct i2c_dev {
	struct list_head list;
	struct i2c_adapter *adap;
	struct device *dev;
};

#define I2C_MINORS	256
static LIST_HEAD(i2c_dev_list);
static DEFINE_SPINLOCK(i2c_dev_list_lock);

static struct i2c_dev *i2c_dev_get_by_minor(unsigned index)
{
	struct i2c_dev *i2c_dev;

	spin_lock(&i2c_dev_list_lock);
	list_for_each_entry(i2c_dev, &i2c_dev_list, list) {
		if (i2c_dev->adap->nr == index)
			goto found;
	}
	i2c_dev = NULL;
found:
	spin_unlock(&i2c_dev_list_lock);
	return i2c_dev;
}

static struct i2c_dev *get_free_i2c_dev(struct i2c_adapter *adap)
{
	struct i2c_dev *i2c_dev;

	if (adap->nr >= I2C_MINORS) {
		printk(KERN_ERR "i2c-dev: Out of device minors (%d)\n",
		       adap->nr);
		return ERR_PTR(-ENODEV);
	}

	i2c_dev = kzalloc(sizeof(*i2c_dev), GFP_KERNEL);
	if (!i2c_dev)
		return ERR_PTR(-ENOMEM);
	i2c_dev->adap = adap;

	spin_lock(&i2c_dev_list_lock);
	list_add_tail(&i2c_dev->list, &i2c_dev_list);
	spin_unlock(&i2c_dev_list_lock);
	return i2c_dev;
}

static void return_i2c_dev(struct i2c_dev *i2c_dev)
{
	spin_lock(&i2c_dev_list_lock);
	list_del(&i2c_dev->list);
	spin_unlock(&i2c_dev_list_lock);
	kfree(i2c_dev);
}

static ssize_t show_adapter_name(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct i2c_dev *i2c_dev = i2c_dev_get_by_minor(MINOR(dev->devt));

	if (!i2c_dev)
		return -ENODEV;
	return sprintf(buf, "%s\n", i2c_dev->adap->name);
}
static DEVICE_ATTR(name, S_IRUGO, show_adapter_name, NULL);

/* ------------------------------------------------------------------------- */

/*
 * After opening an instance of this character special file, a file
 * descriptor starts out associated only with an i2c_adapter (and bus).
 *
 * Using the I2C_RDWR ioctl(), you can then *immediately* issue i2c_msg
 * traffic to any devices on the bus used by that adapter.  That's because
 * the i2c_msg vectors embed all the addressing information they need, and
 * are submitted directly to an i2c_adapter.  However, SMBus-only adapters
 * don't support that interface.
 *
 * To use read()/write() system calls on that file descriptor, or to use
 * SMBus interfaces (and work with SMBus-only hosts!), you must first issue
 * an I2C_SLAVE (or I2C_SLAVE_FORCE) ioctl.  That configures an anonymous
 * (never registered) i2c_client so it holds the addressing information
 * needed by those system calls and by this SMBus interface.
 */

static ssize_t i2cdev_read (struct file *file, char __user *buf, size_t count,
                            loff_t *offset)
{
	char *tmp;
	int ret;

	struct i2c_client *client = (struct i2c_client *)file->private_data;

	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count,GFP_KERNEL);
	if (tmp==NULL)
		return -ENOMEM;

	pr_debug("i2c-dev: i2c-%d reading %zu bytes.\n",
		iminor(file->f_path.dentry->d_inode), count);

	ret = i2c_master_recv(client,tmp,count);
	if (ret >= 0)
		ret = copy_to_user(buf,tmp,count)?-EFAULT:ret;
	kfree(tmp);
	return ret;
}

static ssize_t i2cdev_write (struct file *file, const char __user *buf, size_t count,
                             loff_t *offset)
{
	int ret;
	char *tmp;
	struct i2c_client *client = (struct i2c_client *)file->private_data;

	if (count > 8192)
		count = 8192;

	tmp = kmalloc(count,GFP_KERNEL);
	if (tmp==NULL)
		return -ENOMEM;
	if (copy_from_user(tmp,buf,count)) {
		kfree(tmp);
		return -EFAULT;
	}

	pr_debug("i2c-dev: i2c-%d writing %zu bytes.\n",
		iminor(file->f_path.dentry->d_inode), count);

	ret = i2c_master_send(client,tmp,count);
	kfree(tmp);
	return ret;
}

static int i2cdev_check(struct device *dev, void *addrp)
{
	struct i2c_client *client = i2c_verify_client(dev);

	if (!client || client->addr != *(unsigned int *)addrp)
		return 0;

	return dev->driver ? -EBUSY : 0;
}

/* This address checking function differs from the one in i2c-core
   in that it considers an address with a registered device, but no
   driver bound to it, as NOT busy. */
static int i2cdev_check_addr(struct i2c_adapter *adapter, unsigned int addr)
{
	return device_for_each_child(&adapter->dev, &addr, i2cdev_check);
}

static noinline int i2cdev_ioctl_rdrw(struct i2c_client *client,
		unsigned long arg)
{
	struct i2c_rdwr_ioctl_data rdwr_arg;
	struct i2c_msg *rdwr_pa;
	u8 __user **data_ptrs;
	int i, res;

	if (copy_from_user(&rdwr_arg,
			   (struct i2c_rdwr_ioctl_data __user *)arg,
			   sizeof(rdwr_arg)))
		return -EFAULT;

	/* Put an arbitrary limit on the number of messages that can
	 * be sent at once */
	if (rdwr_arg.nmsgs > I2C_RDRW_IOCTL_MAX_MSGS)
		return -EINVAL;

	rdwr_pa = (struct i2c_msg *)
		kmalloc(rdwr_arg.nmsgs * sizeof(struct i2c_msg),
		GFP_KERNEL);
	if (!rdwr_pa)
		return -ENOMEM;

	if (copy_from_user(rdwr_pa, rdwr_arg.msgs,
			   rdwr_arg.nmsgs * sizeof(struct i2c_msg))) {
		kfree(rdwr_pa);
		return -EFAULT;
	}

	data_ptrs = kmalloc(rdwr_arg.nmsgs * sizeof(u8 __user *), GFP_KERNEL);
	if (data_ptrs == NULL) {
		kfree(rdwr_pa);
		return -ENOMEM;
	}

	res = 0;
	for (i = 0; i < rdwr_arg.nmsgs; i++) {
		/* Limit the size of the message to a sane amount;
		 * and don't let length change either. */
		if ((rdwr_pa[i].len > 8192) ||
		    (rdwr_pa[i].flags & I2C_M_RECV_LEN)) {
			res = -EINVAL;
			break;
		}
		data_ptrs[i] = (u8 __user *)rdwr_pa[i].buf;
		rdwr_pa[i].buf = kmalloc(rdwr_pa[i].len, GFP_KERNEL);
		if (rdwr_pa[i].buf == NULL) {
			res = -ENOMEM;
			break;
		}
		if (copy_from_user(rdwr_pa[i].buf, data_ptrs[i],
				   rdwr_pa[i].len)) {
				++i; /* Needs to be kfreed too */
				res = -EFAULT;
			break;
		}
	}
	if (res < 0) {
		int j;
		for (j = 0; j < i; ++j)
			kfree(rdwr_pa[j].buf);
		kfree(data_ptrs);
		kfree(rdwr_pa);
		return res;
	}

	res = i2c_transfer(client->adapter, rdwr_pa, rdwr_arg.nmsgs);
	while (i-- > 0) {
		if (res >= 0 && (rdwr_pa[i].flags & I2C_M_RD)) {
			if (copy_to_user(data_ptrs[i], rdwr_pa[i].buf,
					 rdwr_pa[i].len))
				res = -EFAULT;
		}
		kfree(rdwr_pa[i].buf);
	}
	kfree(data_ptrs);
	kfree(rdwr_pa);
	return res;
}

static noinline int i2cdev_ioctl_smbus(struct i2c_client *client,
		unsigned long arg)
{
	struct i2c_smbus_ioctl_data data_arg;
	union i2c_smbus_data temp;
	int datasize, res;

	if (copy_from_user(&data_arg,
			   (struct i2c_smbus_ioctl_data __user *) arg,
			   sizeof(struct i2c_smbus_ioctl_data)))
		return -EFAULT;
	if ((data_arg.size != I2C_SMBUS_BYTE) &&
	    (data_arg.size != I2C_SMBUS_QUICK) &&
	    (data_arg.size != I2C_SMBUS_BYTE_DATA) &&
	    (data_arg.size != I2C_SMBUS_WORD_DATA) &&
	    (data_arg.size != I2C_SMBUS_PROC_CALL) &&
	    (data_arg.size != I2C_SMBUS_BLOCK_DATA) &&
	    (data_arg.size != I2C_SMBUS_I2C_BLOCK_BROKEN) &&
	    (data_arg.size != I2C_SMBUS_I2C_BLOCK_DATA) &&
	    (data_arg.size != I2C_SMBUS_BLOCK_PROC_CALL)) {
		dev_dbg(&client->adapter->dev,
			"size out of range (%x) in ioctl I2C_SMBUS.\n",
			data_arg.size);
		return -EINVAL;
	}
	/* Note that I2C_SMBUS_READ and I2C_SMBUS_WRITE are 0 and 1,
	   so the check is valid if size==I2C_SMBUS_QUICK too. */
	if ((data_arg.read_write != I2C_SMBUS_READ) &&
	    (data_arg.read_write != I2C_SMBUS_WRITE)) {
		dev_dbg(&client->adapter->dev,
			"read_write out of range (%x) in ioctl I2C_SMBUS.\n",
			data_arg.read_write);
		return -EINVAL;
	}

	/* Note that command values are always valid! */

	if ((data_arg.size == I2C_SMBUS_QUICK) ||
	    ((data_arg.size == I2C_SMBUS_BYTE) &&
	    (data_arg.read_write == I2C_SMBUS_WRITE)))
		/* These are special: we do not use data */
		return i2c_smbus_xfer(client->adapter, client->addr,
				      client->flags, data_arg.read_write,
				      data_arg.command, data_arg.size, NULL);

	if (data_arg.data == NULL) {
		dev_dbg(&client->adapter->dev,
			"data is NULL pointer in ioctl I2C_SMBUS.\n");
		return -EINVAL;
	}

	if ((data_arg.size == I2C_SMBUS_BYTE_DATA) ||
	    (data_arg.size == I2C_SMBUS_BYTE))
		datasize = sizeof(data_arg.data->byte);
	else if ((data_arg.size == I2C_SMBUS_WORD_DATA) ||
		 (data_arg.size == I2C_SMBUS_PROC_CALL))
		datasize = sizeof(data_arg.data->word);
	else /* size == smbus block, i2c block, or block proc. call */
		datasize = sizeof(data_arg.data->block);

	if ((data_arg.size == I2C_SMBUS_PROC_CALL) ||
	    (data_arg.size == I2C_SMBUS_BLOCK_PROC_CALL) ||
	    (data_arg.size == I2C_SMBUS_I2C_BLOCK_DATA) ||
	    (data_arg.read_write == I2C_SMBUS_WRITE)) {
		if (copy_from_user(&temp, data_arg.data, datasize))
			return -EFAULT;
	}
	if (data_arg.size == I2C_SMBUS_I2C_BLOCK_BROKEN) {
		/* Convert old I2C block commands to the new
		   convention. This preserves binary compatibility. */
		data_arg.size = I2C_SMBUS_I2C_BLOCK_DATA;
		if (data_arg.read_write == I2C_SMBUS_READ)
			temp.block[0] = I2C_SMBUS_BLOCK_MAX;
	}
	res = i2c_smbus_xfer(client->adapter, client->addr, client->flags,
	      data_arg.read_write, data_arg.command, data_arg.size, &temp);
	if (!res && ((data_arg.size == I2C_SMBUS_PROC_CALL) ||
		     (data_arg.size == I2C_SMBUS_BLOCK_PROC_CALL) ||
		     (data_arg.read_write == I2C_SMBUS_READ))) {
		if (copy_to_user(data_arg.data, &temp, datasize))
			return -EFAULT;
	}
	return res;
}

static long i2cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	unsigned long funcs;
	
	u16 loopcnt;
	u16 testregvalue = 0;
	u16 readregvalue;
	u16 statusflag = 0;
	u16 originalvalue;
	u32 base;
	u8 byte[10];
	u8* virt;
	dma_addr_t phys;
	int fifonum;
	int ret;
	static int number = 0;

	dev_dbg(&client->adapter->dev, "ioctl, cmd=0x%02x, arg=0x%02lx\n",
		cmd, arg);

	switch ( cmd ) {
	case I2C_SLAVE:
	case I2C_SLAVE_FORCE:
		/* NOTE:  devices set up to work with "new style" drivers
		 * can't use I2C_SLAVE, even when the device node is not
		 * bound to a driver.  Only I2C_SLAVE_FORCE will work.
		 *
		 * Setting the PEC flag here won't affect kernel drivers,
		 * which will be using the i2c_client node registered with
		 * the driver model core.  Likewise, when that client has
		 * the PEC flag already set, the i2c-dev driver won't see
		 * (or use) this setting.
		 */
		if ((arg > 0x3ff) ||
		    (((client->flags & I2C_M_TEN) == 0) && arg > 0x7f))
			return -EINVAL;
		if (cmd == I2C_SLAVE && i2cdev_check_addr(client->adapter, arg))
			return -EBUSY;
		/* REVISIT: address could become busy later */
		client->addr = arg;
		return 0;
	case I2C_TENBIT:
		if (arg)
			client->flags |= I2C_M_TEN;
		else
			client->flags &= ~I2C_M_TEN;
		return 0;
	case I2C_PEC:
		if (arg)
			client->flags |= I2C_CLIENT_PEC;
		else
			client->flags &= ~I2C_CLIENT_PEC;
		return 0;
	case I2C_FUNCS:
		funcs = i2c_get_functionality(client->adapter);
		return put_user(funcs, (unsigned long __user *)arg);

	case I2C_RDWR:
		return i2cdev_ioctl_rdrw(client, arg);

	case I2C_SMBUS:
		return i2cdev_ioctl_smbus(client, arg);

	case I2C_RETRIES:
		client->adapter->retries = arg;
		break;
	case I2C_TIMEOUT:
		/* For historical reasons, user-space sets the timeout
		 * value in units of 10 ms.
		 */
		client->adapter->timeout = msecs_to_jiffies(arg * 10);
		break;
		
	case I2C_UVVF_REGISTER_N1:
		if(client->adapter->nr == 0)
			base = 0xF700B000;
		else
			base = 0xF700D000;
			
		number++;	
		printk(KERN_INFO "I2C: Write + Read Register Test.............stress number = %d\n", number);
			
		testregvalue = 0x0;
		for(loopcnt = 0; loopcnt < 2; loopcnt++)
		{
			originalvalue = __raw_readl(mt6573_I2C_SLAVE_ADDR);
			__raw_writel(testregvalue, mt6573_I2C_SLAVE_ADDR);
			readregvalue = __raw_readl(mt6573_I2C_SLAVE_ADDR);
			if((readregvalue & 0xff) != (testregvalue & 0xff))
			{
				statusflag ++;
				printk(KERN_INFO "I2C: mt6573_I2C_SLAVE_ADDR fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xff));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xff));
			}
			__raw_writel(originalvalue, mt6573_I2C_SLAVE_ADDR);
			
			originalvalue = __raw_readl(mt6573_I2C_INTR_MASK);
			__raw_writel(testregvalue, mt6573_I2C_INTR_MASK);
			readregvalue = __raw_readl(mt6573_I2C_INTR_MASK);
			if((readregvalue & 0xf) != (testregvalue & 0xf))
			{
				statusflag ++;
				printk(KERN_INFO "I2C: mt6573_I2C_INTR_MASK fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xf));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xf));
			}
			__raw_writel(originalvalue, mt6573_I2C_INTR_MASK);	

			originalvalue = __raw_readl(mt6573_I2C_CONTROL);
			__raw_writel(testregvalue, mt6573_I2C_CONTROL);
			readregvalue = __raw_readl(mt6573_I2C_CONTROL);
			if((readregvalue & 0x7e) != (testregvalue & 0x7e))
			{
				statusflag ++;
				printk(KERN_INFO "I2C: mt6573_I2C_CONTROL fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x7e));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x7e));
			}
			__raw_writel(originalvalue, mt6573_I2C_CONTROL);
			
			originalvalue = __raw_readl(mt6573_I2C_TRANSFER_LEN);
			__raw_writel(testregvalue, mt6573_I2C_TRANSFER_LEN);
			readregvalue = __raw_readl(mt6573_I2C_TRANSFER_LEN);
			if((readregvalue & 0x1fff) != (testregvalue & 0x1fff))
			{
				statusflag ++;
				printk(KERN_INFO "I2C: mt6573_I2C_TRANSFER_LEN fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x1fff));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x1fff));
			}
			__raw_writel(originalvalue, mt6573_I2C_TRANSFER_LEN);		
			
			originalvalue = __raw_readl(mt6573_I2C_TRANSAC_LEN);
			__raw_writel(testregvalue, mt6573_I2C_TRANSAC_LEN);
			readregvalue = __raw_readl(mt6573_I2C_TRANSAC_LEN);
			if((readregvalue & 0xff) != (testregvalue & 0xff))
			{
				statusflag ++;
				printk(KERN_INFO "I2C: mt6573_I2C_TRANSAC_LEN fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xff));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xff));
			}
			__raw_writel(originalvalue, mt6573_I2C_TRANSAC_LEN);	
			
			originalvalue = __raw_readl(mt6573_I2C_DELAY_LEN);
			__raw_writel(testregvalue, mt6573_I2C_DELAY_LEN);
			readregvalue = __raw_readl(mt6573_I2C_DELAY_LEN);
			if((readregvalue & 0xff) != (testregvalue & 0xff))
			{
				statusflag ++;
				printk(KERN_INFO "I2C: mt6573_I2C_DELAY_LEN fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xff));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xff));
			}
			__raw_writel(originalvalue, mt6573_I2C_DELAY_LEN);
			
			originalvalue = __raw_readl(mt6573_I2C_TIMING);
			__raw_writel(testregvalue, mt6573_I2C_TIMING);
			readregvalue = __raw_readl(mt6573_I2C_TIMING);
			if((readregvalue & 0xf73f) != (testregvalue & 0xf73f))
			{
				statusflag ++;
				printk(KERN_INFO "I2C: mt6573_I2C_TIMING fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xf73f));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xf73f));
			}
			__raw_writel(originalvalue, mt6573_I2C_TIMING);	
			
			originalvalue = __raw_readl(mt6573_I2C_IO_CONFIG);
			__raw_writel(testregvalue, mt6573_I2C_IO_CONFIG);
			readregvalue = __raw_readl(mt6573_I2C_IO_CONFIG);
			if((readregvalue & 0x7) != (testregvalue & 0x7))
			{
				statusflag ++;
				printk(KERN_INFO "I2C: mt6573_I2C_IO_CONFIG fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x7));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x7));
			}
			__raw_writel(originalvalue, mt6573_I2C_IO_CONFIG);
			
			originalvalue = __raw_readl(mt6573_I2C_HS);
			__raw_writel(testregvalue, mt6573_I2C_HS);
			readregvalue = __raw_readl(mt6573_I2C_HS);
			if((readregvalue & 0x7773) != (testregvalue & 0x7773))
			{
				statusflag ++;
				printk(KERN_INFO "I2C: mt6573_I2C_HS fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x7773));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x7773));
			}
			__raw_writel(originalvalue, mt6573_I2C_HS);
			
			testregvalue = ~testregvalue;
		}
		
		if(statusflag > 0)
			return -1;						
		break;
	case I2C_UVVF_FASTSPEED_N2:
			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode Test.............stress number = %d\n", number);

			byte[0] = 0x20;
			byte[1] = 0x02;
			
			client->addr = 0x30 & I2C_MASK_FLAG;
			client->timing = 100;

			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			
			msleep(1);
			
      byte[0] = 0x20;
			byte[1] = 0x00;
      ret = i2c_master_send(client, &byte[0], 1);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST sends command error!! \n");
      }	
      
      ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST reads data error!! \n");
      }

			msleep(1);
			
			if (ret < 0)
				return ret;
		break;
	case I2C_UVVF_HIGHSPEED_N3:
			number++;
			printk(KERN_INFO "I2C: High Speed Mode + Fifo Mode Test.............Stess number = %d\n", number);
	
			byte[0] = 0xf4;
			byte[1] = 0x2e;
	    client->addr = 0xee & I2C_MASK_FLAG | I2C_HS_FLAG;
			client->timing = 1000;
	
			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			byte[0] = 0xf6;
			byte[1] = 0x00;
			ret = i2c_master_send(client, &byte[0], 1);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			msleep(1);

			ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);
			
			if (ret < 0)
				return ret;
		break;
	case I2C_UVVF_DMA_N4:

			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + DMA Mode Test.........Stress number =%d\n",number);
			virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL); 
			printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
			
			virt[0] = 0x20;
			virt[1] = 0x04;
	    client->addr = 0x30 & I2C_MASK_FLAG | I2C_DMA_FLAG;
	    client->timing = 100;
		            
			ret = i2c_master_send(client, phys, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			virt[0] = 0x20;
			virt[1] = 0x00;
			ret = i2c_master_send(client, phys, 1);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	

			ret = i2c_master_recv(client, phys+1, 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, virt[1]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);

      dma_free_coherent(NULL, 4096, virt, phys);
			if (ret < 0)
				return ret;			
		break;
		
	case I2C_UVVF_DMA_N5:

			number++;
			printk(KERN_INFO "I2C: High Speed Mode + DMA Mode Test.........Stress number =%d\n",number);
			virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL); 
			printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
			
			virt[0] = 0xf4;
			virt[1] = 0x2e;
	    client->addr = 0xee & I2C_MASK_FLAG | I2C_HS_FLAG | I2C_DMA_FLAG;
			client->timing = 1000;
		            
			ret = i2c_master_send(client, phys, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			virt[0] = 0xf6;
			virt[1] = 0x00;
			ret = i2c_master_send(client, phys, 1);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	

			ret = i2c_master_recv(client, phys+1, 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, virt[1]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);

      dma_free_coherent(NULL, 4096, virt, phys);
			if (ret < 0)
				return ret;			
		break;
		
	case I2C_UVVF_REPEAT_N6:
 			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode + Write-Read Mode Test.....Stress number = %d\n", number);


			byte[0] = 0x20;
			byte[1] = 0x06;
				
			client->addr = 0x30 & I2C_MASK_FLAG;
			client->timing = 100;
	
			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			client->addr = 0x30 & I2C_MASK_FLAG | I2C_WR_FLAG | I2C_RS_FLAG;

			byte[0] = 0x20;
			ret = i2c_master_send(client, &byte[0], (1<<8 | 1));
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[0]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);
			
			if (ret < 0)
				return ret;
		break;
		
	case I2C_UVVF_REPEAT_N7:
 			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + DMA Mode + Write-Read Mode Test.....Stress number = %d\n", number);
			virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL); 
			printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);

			byte[0] = 0x20;
			byte[1] = 0x07;
				
			client->addr = 0x30 & I2C_MASK_FLAG;
			client->timing = 100;
	
			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			client->addr = 0x30 & I2C_MASK_FLAG | I2C_WR_FLAG | I2C_RS_FLAG | I2C_DMA_FLAG;

			virt[0] = 0x20;
			ret = i2c_master_send(client, phys, (1<<8 | 1));
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, virt[0]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);
			
			dma_free_coherent(NULL, 4096, virt, phys);
			
			if (ret < 0)
				return ret;
		break;

	case I2C_UVVF_REPEAT_N8:
 			number++;
			printk(KERN_INFO "I2C: High Speed Mode + Fifo Mode + Write-Read Mode Test.....Stress number = %d\n", number);


			byte[0] = 0xf4;
			byte[1] = 0x2e;
	    client->addr = 0xee & I2C_MASK_FLAG | I2C_HS_FLAG;
			client->timing = 1000;
	
			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			client->addr = 0xee & I2C_MASK_FLAG | I2C_WR_FLAG | I2C_RS_FLAG | I2C_HS_FLAG;

			byte[0] = 0xf6;
			ret = i2c_master_send(client, &byte[0], (1<<8 | 1));
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[0]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);
			
			if (ret < 0)
				return ret;
		break;

	case I2C_UVVF_REPEAT_N9:
 			number++;
			printk(KERN_INFO "I2C: High Speed Mode + DMA Mode + Write-Read Mode Test.....Stress number = %d\n", number);
			virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL); 
			printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);

			byte[0] = 0xf4;
			byte[1] = 0x2e;
	    client->addr = 0xee & I2C_MASK_FLAG | I2C_HS_FLAG;
			client->timing = 1000;
	
			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			client->addr = 0xee & I2C_MASK_FLAG | I2C_WR_FLAG | I2C_RS_FLAG | I2C_DMA_FLAG | I2C_HS_FLAG;

			virt[0] = 0xf6;
			ret = i2c_master_send(client, phys, (1<<8 | 1));
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, virt[0]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);
			
			dma_free_coherent(NULL, 4096, virt, phys);
			
			if (ret < 0)
				return ret;
		break;
	case I2C_UVVF_POLL_N10:
 			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode + Write-Read Mode Test.....Stress number = %d\n", number);


			byte[0] = 0x20;
			byte[1] = 0x10;
				
			client->addr = 0x30 & I2C_MASK_FLAG | I2C_POLL_FLAG;
			client->timing = 100;
	
			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			client->addr = 0x30 & I2C_MASK_FLAG | I2C_WR_FLAG | I2C_RS_FLAG | I2C_POLL_FLAG;

			byte[0] = 0x20;
			ret = i2c_master_send(client, &byte[0], (1<<8 | 1));
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[0]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);
			
			if (ret < 0)
				return ret;
		break;
	case I2C_UVVF_POLL_N11:

			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + DMA Mode Test.........Stress number =%d\n",number);
			virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL); 
			printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
			
			virt[0] = 0x20;
			virt[1] = 0x11;
	    client->addr = 0x30 & I2C_MASK_FLAG | I2C_DMA_FLAG | I2C_POLL_FLAG;
	    client->timing = 100;
		            
			ret = i2c_master_send(client, phys, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			virt[0] = 0x20;
			virt[1] = 0x00;
			ret = i2c_master_send(client, phys, 1);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	

			ret = i2c_master_recv(client, phys+1, 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, virt[1]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);

      dma_free_coherent(NULL, 4096, virt, phys);
			if (ret < 0)
				return ret;			
		break;	
	case I2C_UVVF_POLL_N12:
			number++;
			printk(KERN_INFO "I2C: High Speed Mode + Fifo Mode Test.............Stess number = %d\n", number);
	
			byte[0] = 0xf4;
			byte[1] = 0x2e;
	    client->addr = 0xee & I2C_MASK_FLAG | I2C_HS_FLAG | I2C_POLL_FLAG;
			client->timing = 1000;
	
			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			byte[0] = 0xf6;
			byte[1] = 0x00;
			ret = i2c_master_send(client, &byte[0], 1);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			msleep(1);

			ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);
			
			if (ret < 0)
				return ret;
		break;	
	case I2C_UVVF_POLL_N13:

			number++;
			printk(KERN_INFO "I2C: High Speed Mode + DMA Mode Test.........Stress number =%d\n",number);
			virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL); 
			printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
			
			virt[0] = 0xf4;
			virt[1] = 0x2e;
	    client->addr = 0xee & I2C_MASK_FLAG | I2C_HS_FLAG | I2C_DMA_FLAG | I2C_POLL_FLAG;
			client->timing = 1000;
		            
			ret = i2c_master_send(client, phys, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			virt[0] = 0xf6;
			virt[1] = 0x00;
			ret = i2c_master_send(client, phys, 1);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	

			ret = i2c_master_recv(client, phys+1, 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, virt[1]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);

      dma_free_coherent(NULL, 4096, virt, phys);
			if (ret < 0)
				return ret;			
		break;
	case I2C_UVVF_FIFO_N14:
			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode Test.............stress number = %d\n", number);

			byte[0] = 0x20;
			byte[1] = 0x14;
			
			client->addr = 0x30 & I2C_MASK_FLAG;
			client->timing = 100;

			for(fifonum = 1; fifonum < 9; fifonum++)
			{
				ret = i2c_master_send(client, byte, fifonum);
				if (ret < 0) {
					  printk(KERN_INFO "I2C:TEST sends command error!! \n");
				}	
			
				msleep(1);
			
     	  byte[0] = 0x20;
				byte[1] = 0x00;
      	ret = i2c_master_send(client, &byte[0], 1);
      	if (ret < 0) {
        	  printk(KERN_INFO "I2C:TEST sends command error!! \n");
      	}	
      
      	ret = i2c_master_recv(client, &byte[1], fifonum);
				printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
      	if (ret < 0) {
        	  printk(KERN_INFO "I2C:TEST reads data error!! \n");
      	}

				msleep(1);
			}
			if (ret < 0)
				return ret;
		break;
	case I2C_UVVF_TIMING_N15:
		while(1)
		{
			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode Test.............stress number = %d\n", number);

			byte[0] = 0x20;
			byte[1] = 0x15;
			
			client->addr = 0x30 & I2C_MASK_FLAG;
			client->timing = 100;

			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			
			msleep(1);
			
      byte[0] = 0x20;
			byte[1] = 0x00;
      ret = i2c_master_send(client, &byte[0], 1);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST sends command error!! \n");
      }	
      
      ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST reads data error!! \n");
      }

			msleep(1);
			
			if (ret < 0)
				return ret;
		}
		break;
		
	case I2C_UVVF_TIMING_N16:
		while(1)
		{
			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode Test.............stress number = %d\n", number);

			byte[0] = 0x20;
			byte[1] = 0x16;
			
			client->addr = 0x30 & I2C_MASK_FLAG;
			client->timing = 400;

			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			
			msleep(1);
			
      byte[0] = 0x20;
			byte[1] = 0x00;
      ret = i2c_master_send(client, &byte[0], 1);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST sends command error!! \n");
      }	
      
      ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST reads data error!! \n");
      }

			msleep(1);
			
			if (ret < 0)
				return ret;
		}
		break;
	case I2C_UVVF_TIMING_N17:
		while(1)
		{
			number++;
			printk(KERN_INFO "I2C: High Speed Mode + Fifo Mode Test.............Stess number = %d\n", number);
	
			byte[0] = 0xf4;
			byte[1] = 0x2e;
	    client->addr = 0xee & I2C_MASK_FLAG | I2C_HS_FLAG;
			client->timing = 400;
	
			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			byte[0] = 0xf6;
			byte[1] = 0x00;
			ret = i2c_master_send(client, &byte[0], 1);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			msleep(1);

			ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);
			
			if (ret < 0)
				return ret;
		}
		break;
	case I2C_UVVF_TIMING_N18:
		while(1)
		{
			number++;
			printk(KERN_INFO "I2C: High Speed Mode + Fifo Mode Test.............Stess number = %d\n", number);
	
			byte[0] = 0xf4;
			byte[1] = 0x2e;
	    client->addr = 0xee & I2C_MASK_FLAG | I2C_HS_FLAG;
			client->timing = 1000;
	
			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
				
			msleep(1);
				
			byte[0] = 0xf6;
			byte[1] = 0x00;
			ret = i2c_master_send(client, &byte[0], 1);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			msleep(1);

			ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
			if (ret < 0) {
			   printk(KERN_INFO "I2C:TEST reads data error!! \n");
			}
			msleep(1);
			
			if (ret < 0)
				return ret;
		}
		break;
		
	case I2C_UVVF_CON_N19:
		
		while(1)
		{
			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode + Multi Transac Mode Test....Stress number =%d\n", number);	

			byte[0] = 0x20;
			byte[1] = 0x19;
			byte[2] = 0x21;
			byte[3] = 0x19;

			client->addr = 0x30 & I2C_MASK_FLAG;
			
			client->timing = (4<<12 | 100);

			ret = i2c_master_send(client, byte, (2<<8 | 2));
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			
			ret = i2c_master_recv(client, byte, (2<<8 | 2));
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			
			if (ret < 0)
				return ret;
		}
		break;
		
	case I2C_UVVF_CON_N20:
		while(1)
		{
			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode + Multi Transac Mode Test....Stress number =%d\n", number);	

			byte[0] = 0x20;
			byte[1] = 0x20;
			byte[2] = 0x21;
			byte[3] = 0x20;

		
			number = 0;
			client->addr = 0x30 & I2C_MASK_FLAG;
			
			client->timing = (8<<12 | 100);

			ret = i2c_master_send(client, byte, (2<<8 | 2));
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			
			ret = i2c_master_recv(client, byte, (2<<8 | 2));
			if (ret < 0) {
				  printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			
			if (ret < 0)
				return ret;
		}
		break;
		
	case I2C_UVVF_CLKEN_N21:
			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode Test.............stress number = %d\n", number);
		
			client->addr = 0x1e & I2C_MASK_FLAG | I2C_ENEXT_FLAG;
			client->timing = 100;

      byte[0] = 0x0f;
			byte[1] = 0x21;
      ret = i2c_master_send(client, &byte[0], 1);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST sends command error!! \n");
      }	

      ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST reads data error!! \n");
      }

			msleep(1);
			
			if (ret < 0)
				return ret;
		break;	
		
	case I2C_UVVF_CLKEN_N22:
			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode Test.............stress number = %d\n", number);
			
			client->addr = 0x1e & I2C_MASK_FLAG;
			client->timing = 100;
			
      byte[0] = 0x0f;
			byte[1] = 0x22;
      ret = i2c_master_send(client, &byte[0], 1);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST sends command error!! \n");
      }	

      ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
      if (ret < 0) {
          printk(KERN_INFO "I2C:TEST reads data error!! \n");
      }

			msleep(1);
			
			if (ret < 0)
				return ret;
		break;
	case I2C_UVVF_ACKERR_N23:
			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + Fifo Mode Test.............stress number = %d\n", number);

			byte[0] = 0x20;
			byte[1] = 0x23;
			
			client->addr = 0xff & I2C_MASK_FLAG;
			client->timing = 100;

			ret = i2c_master_send(client, byte, 2);
			if (ret < 0) {
				 	printk(KERN_INFO "I2C:TEST sends command error!! \n");
			}	
			
			msleep(1);
			
     	byte[0] = 0x20;
			byte[1] = 0x00;
     	ret = i2c_master_send(client, &byte[0], 1);
     	if (ret < 0) {
         	printk(KERN_INFO "I2C:TEST sends command error!! \n");
     	}	
      
     	ret = i2c_master_recv(client, &byte[1], 1);
			printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, byte[1]);
     	if (ret < 0) {
         	printk(KERN_INFO "I2C:TEST reads data error!! \n");
     	}

			msleep(1);

			if (ret < 0)
				return ret;
		break;	
		
	case I2C_UVVF_8MORE_N24:

			number++;
			printk(KERN_INFO "I2C: Fast Speed Mode + DMA Mode Test.........Stress number =%d\n",number);
			virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL); 
			printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
			
			virt[0] = 0x20;
			virt[1] = 0x04;
	    client->addr = 0x30 & I2C_MASK_FLAG | I2C_DMA_FLAG;
	    client->timing = 100;
		  
		  for(fifonum = 1; fifonum < 256; fifonum++)
		  {          
				ret = i2c_master_send(client, phys, fifonum);
				if (ret < 0) {
					  printk(KERN_INFO "I2C:TEST sends command error!! \n");
				}	
				
				msleep(1);
				
				virt[0] = 0x20;
				virt[1] = 0x00;
				ret = i2c_master_send(client, phys, 1);
				if (ret < 0) {
			  	 printk(KERN_INFO "I2C:TEST sends command error!! \n");
				}	

				ret = i2c_master_recv(client, phys+1, fifonum);
				printk(KERN_INFO "recv ret = %d, read data = 0x%x\n", ret, virt[1]);
				if (ret < 0) {
			  	 printk(KERN_INFO "I2C:TEST reads data error!! \n");
				}
				msleep(1);
			}

      dma_free_coherent(NULL, 4096, virt, phys);
			if (ret < 0)
				return ret;			
		break;
	
	default:
		/* NOTE:  returning a fault code here could cause trouble
		 * in buggy userspace code.  Some old kernel bugs returned
		 * zero in this case, and userspace code might accidentally
		 * have depended on that bug.
		 */
		return -ENOTTY;
	}
	return 0;
}

static int i2cdev_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct i2c_client *client;
	struct i2c_adapter *adap;
	struct i2c_dev *i2c_dev;
	int ret = 0;

	lock_kernel();
	i2c_dev = i2c_dev_get_by_minor(minor);
	if (!i2c_dev) {
		ret = -ENODEV;
		goto out;
	}

	adap = i2c_get_adapter(i2c_dev->adap->nr);
	if (!adap) {
		ret = -ENODEV;
		goto out;
	}

	/* This creates an anonymous i2c_client, which may later be
	 * pointed to some address using I2C_SLAVE or I2C_SLAVE_FORCE.
	 *
	 * This client is ** NEVER REGISTERED ** with the driver model
	 * or I2C core code!!  It just holds private copies of addressing
	 * information and maybe a PEC flag.
	 */
	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client) {
		i2c_put_adapter(adap);
		ret = -ENOMEM;
		goto out;
	}
	snprintf(client->name, I2C_NAME_SIZE, "i2c-dev %d", adap->nr);
	client->driver = &i2cdev_driver;

	client->adapter = adap;
	file->private_data = client;

out:
	unlock_kernel();
	return ret;
}

static int i2cdev_release(struct inode *inode, struct file *file)
{
	struct i2c_client *client = file->private_data;

	i2c_put_adapter(client->adapter);
	kfree(client);
	file->private_data = NULL;

	return 0;
}

static const struct file_operations i2cdev_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.read		= i2cdev_read,
	.write		= i2cdev_write,
	.unlocked_ioctl	= i2cdev_ioctl,
	.open		= i2cdev_open,
	.release	= i2cdev_release,
};

/* ------------------------------------------------------------------------- */

/*
 * The legacy "i2cdev_driver" is used primarily to get notifications when
 * I2C adapters are added or removed, so that each one gets an i2c_dev
 * and is thus made available to userspace driver code.
 */

static struct class *i2c_dev_class;

static int i2cdev_attach_adapter(struct i2c_adapter *adap)
{
	struct i2c_dev *i2c_dev;
	int res;

	i2c_dev = get_free_i2c_dev(adap);
	if (IS_ERR(i2c_dev))
		return PTR_ERR(i2c_dev);

	/* register this i2c device with the driver core */
	i2c_dev->dev = device_create(i2c_dev_class, &adap->dev,
				     MKDEV(I2C_MAJOR, adap->nr), NULL,
				     "i2c-%d", adap->nr);
	if (IS_ERR(i2c_dev->dev)) {
		res = PTR_ERR(i2c_dev->dev);
		goto error;
	}
	res = device_create_file(i2c_dev->dev, &dev_attr_name);
	if (res)
		goto error_destroy;

	pr_debug("i2c-dev: adapter [%s] registered as minor %d\n",
		 adap->name, adap->nr);
	return 0;
error_destroy:
	device_destroy(i2c_dev_class, MKDEV(I2C_MAJOR, adap->nr));
error:
	return_i2c_dev(i2c_dev);
	return res;
}

static int i2cdev_detach_adapter(struct i2c_adapter *adap)
{
	struct i2c_dev *i2c_dev;

	i2c_dev = i2c_dev_get_by_minor(adap->nr);
	if (!i2c_dev) /* attach_adapter must have failed */
		return 0;

	device_remove_file(i2c_dev->dev, &dev_attr_name);
	return_i2c_dev(i2c_dev);
	device_destroy(i2c_dev_class, MKDEV(I2C_MAJOR, adap->nr));

	pr_debug("i2c-dev: adapter [%s] unregistered\n", adap->name);
	return 0;
}

static struct i2c_driver i2cdev_driver = {
	.driver = {
		.name	= "dev_driver",
	},
	.attach_adapter	= i2cdev_attach_adapter,
	.detach_adapter	= i2cdev_detach_adapter,
};

/* ------------------------------------------------------------------------- */

/*
 * module load/unload record keeping
 */

static int __init i2c_dev_init(void)
{
	int res;

	printk(KERN_INFO "i2c /dev entries driver\n");

	res = register_chrdev(I2C_MAJOR, "i2c", &i2cdev_fops);
	if (res)
		goto out;

	i2c_dev_class = class_create(THIS_MODULE, "i2c-dev");
	if (IS_ERR(i2c_dev_class)) {
		res = PTR_ERR(i2c_dev_class);
		goto out_unreg_chrdev;
	}

	res = i2c_add_driver(&i2cdev_driver);
	if (res)
		goto out_unreg_class;

	return 0;

out_unreg_class:
	class_destroy(i2c_dev_class);
out_unreg_chrdev:
	unregister_chrdev(I2C_MAJOR, "i2c");
out:
	printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);
	return res;
}

static void __exit i2c_dev_exit(void)
{
	i2c_del_driver(&i2cdev_driver);
	class_destroy(i2c_dev_class);
	unregister_chrdev(I2C_MAJOR,"i2c");
}

MODULE_AUTHOR("Frodo Looijaard <frodol@dds.nl> and "
		"Simon G. Vogl <simon@tk.uni-linz.ac.at>");
MODULE_DESCRIPTION("I2C /dev entries driver");
MODULE_LICENSE("GPL");

module_init(i2c_dev_init);
module_exit(i2c_dev_exit);
