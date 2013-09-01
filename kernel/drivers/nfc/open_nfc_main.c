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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/device.h>

#include <asm/uaccess.h>

#include "open_nfc_main.h"
#include "porting_types.h"

MODULE_AUTHOR("Inside Secure");
MODULE_LICENSE("GPL");

int open_nfc_major =   OPEN_NFC_MAJOR;
int open_nfc_minor =   0;

void open_nfc_cleanup_module(void);

static struct open_nfc_dev {

    /* common stuff, should be kept unchanged */

    struct cdev       cdev;               /* char device structure */
    bool_t             cdev_registered;

    struct class      * class;            /* sysfs class */
    bool_t               class_registered;

    struct device      device;               /* sysfs device */
    bool_t               device_registered;
} instance;


/*
 *  Function called when user opens the device
 */
int open_nfc_open(struct inode *inode, struct file *filp)
{
   return open_nfc_custom_open(inode, filp);
}

/*
 *  Function called when user closes the device
 */

int open_nfc_release(struct inode *inode, struct file *filp)
{
   return open_nfc_custom_release(inode, filp);
}

/*
 * function called when user performs a read operation
 */

ssize_t open_nfc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
   return open_nfc_custom_read(filp, buf, count, f_pos);
}

/*
 * function called when user performs a write operation
 */

ssize_t open_nfc_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
   return open_nfc_custom_write(filp, buf, count, f_pos);
}

/*
 * function called when user performs a ioctl operation
 */

long open_nfc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
   int retval = 0;
   int err = 0;

   /*
    * extract the type and number bitfields, and don't decode
    * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
    */
   if (_IOC_TYPE(cmd) != OPEN_NFC_IOC_MAGIC) return -ENOTTY;
   if (_IOC_NR(cmd) > OPEN_NFC_MAX_IOCTL_VALUE) return -ENOTTY;

   /*
    * the direction is a bitmask, and VERIFY_WRITE catches R/W
    * transfers. `Type' is user-oriented, while
    * access_ok is kernel-oriented, so the concept of "read" and
    * "write" is reversed
    */

   if (_IOC_DIR(cmd) & _IOC_READ)
      err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
   else if (_IOC_DIR(cmd) & _IOC_WRITE)
      err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
   if (err) return -EFAULT;

   switch(cmd) {

     case OPEN_NFC_IOC_CONFIGURE:
        retval = open_nfc_custom_ioctl_configure(filp, cmd, arg);
      break;

     case OPEN_NFC_IOC_CONNECT:
        retval = open_nfc_custom_ioctl_connect(filp, cmd, arg);
      break;

     case OPEN_NFC_IOC_RESET:
        retval = open_nfc_custom_ioctl_reset(filp, cmd, arg);
      break;

   }

   return retval;
}

unsigned int open_nfc_poll(struct file *filp, poll_table *wait)
{
   return open_nfc_custom_poll(filp, wait);
}


struct file_operations open_nfc_fops = {
   .owner =    THIS_MODULE,
   .read =     open_nfc_read,
   .write =    open_nfc_write,
   .unlocked_ioctl =    open_nfc_ioctl,
   .open =     open_nfc_open,
   .release =  open_nfc_release,
   .poll    =  open_nfc_poll
};

/*
 * function called when user loads the driver
 */

int open_nfc_init_module(void)
{
   int result;
   int devno;

   printk(KERN_DEBUG "open_nfc_init_module\n");

   if (open_nfc_major)  {

      /* user supplied major number, register the region */

      devno = MKDEV(open_nfc_major, open_nfc_minor);
      result = register_chrdev_region(devno, 1, "open_nfc");
   }
   else {

      /* automatic major number, allocate a region */

      result = alloc_chrdev_region(&devno, open_nfc_minor, 1, "open_nfc");
      open_nfc_major = MAJOR(devno);
   }

   if (result < 0) {

      printk(KERN_ERR "open_nfc: can't get major %d\n", open_nfc_major);
      return result;
   }


   /* reset the instance */
   memset(& instance, 0, sizeof(instance));

   /* register the char device */
   cdev_init( & instance.cdev, &open_nfc_fops);
   instance.cdev.owner = THIS_MODULE;

   result = cdev_add( & instance.cdev, devno, 1);

   if (result)
   {
      printk(KERN_ERR "open_nfc : can't register device");

      open_nfc_cleanup_module();
      return result;
   }

   instance.cdev_registered = 1;

   /* register the nfc class */
   instance.class = class_create(THIS_MODULE, "nfc");

   if (IS_ERR(instance.class)) {

      printk(KERN_ERR "open_nfc : can't create class");

      result = PTR_ERR(instance.class);

      open_nfc_cleanup_module();
      return result;
   }

   open_nfc_custom_init();

   instance.class_registered = 1;

   /* add a device into the class */
   device_create(instance.class, NULL, devno, NULL, "nfcc");

   instance.device_registered = 1;

   /* succeeded */
   return 0;
}


/*
 * function called when user unloads the driver
 */

void open_nfc_cleanup_module(void)
{
   int devno = MKDEV(open_nfc_major, open_nfc_minor);

   printk(KERN_DEBUG "open_nfc_cleanup_module\n");

   open_nfc_custom_exit();

   /* remove the device from the class if successfully added */
   if (instance.device_registered)
   {
      device_destroy(instance.class, devno);
   }

   /* unregister the device only if the registration succeeded */
   if (instance.class_registered)
   {
      class_destroy(instance.class);
   }

   /* unregister the character device */
   if (instance.cdev_registered)
   {
      cdev_del(&instance.cdev);
   }

   /* always unregister the region, since open_nfc_cleanup_module is not called if the registration failed */
   unregister_chrdev_region(devno, 1);
}

module_init(open_nfc_init_module);
module_exit(open_nfc_cleanup_module);


/* EOF */




