/*
 * Gadget Function Driver for MTP
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
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

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>

#include <linux/types.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#include <linux/usb.h>
#include <linux/usb_usual.h>
#include <linux/usb/ch9.h>
#include <linux/usb/f_mtp.h>
//Added for MTP Develpment debug, more log for more debuging help
#include <linux/xlog.h>
//Added for MTP Develpment debug, more log for more debuging help
//Added Modification for ALPS00354386
#include <linux/delay.h>
//Added Modification for ALPS00354386

#define MTP_BULK_BUFFER_SIZE       16384
#define INTR_BUFFER_SIZE           28

/* String IDs */
#define INTERFACE_STRING_INDEX	0

/* values for mtp_dev.state */
#define STATE_OFFLINE               0   /* initial state, disconnected */
#define STATE_READY                 1   /* ready for userspace calls */
#define STATE_BUSY                  2   /* processing userspace calls */
#define STATE_CANCELED              3   /* transaction canceled by host */
#define STATE_ERROR                 4   /* error from completion routine */
//Added Modification for ALPS00255822, bug from WHQL test
#define STATE_RESET                 5   /* reset from device reset request */
//Added Modification for ALPS00255822, bug from WHQL test

/* number of tx and rx requests to allocate */
#define TX_REQ_MAX 4
#define RX_REQ_MAX 2
#define INTR_REQ_MAX 5

/* ID for Microsoft MTP OS String */
#define MTP_OS_STRING_ID   0xEE

/* MTP class reqeusts */
#define MTP_REQ_CANCEL              0x64
#define MTP_REQ_GET_EXT_EVENT_DATA  0x65
#define MTP_REQ_RESET               0x66
#define MTP_REQ_GET_DEVICE_STATUS   0x67

/* constants for device status */
#define MTP_RESPONSE_OK             0x2001
#define MTP_RESPONSE_DEVICE_BUSY    0x2019
//ALPS00428998
#define MTP_RESPONSE_DEVICE_CANCEL	0x201F
//ALPS00428998

static const char mtp_shortname[] = "mtp_usb";

//Added for USB Develpment debug, more log for more debuging help
/*#ifdef DBG
#undef DBG
#endif

#define DBG(level, fmt, args...) \
	do { \
			printk( fmt, ##args); \
	} while (0)


#ifdef VDBG
#undef VDBG
#endif

#define VDBG(level, fmt, args...) \
	do { \
			printk( fmt, ##args); \
	} while (0)


#ifdef pr_debug
#undef pr_debug
#endif

#define pr_debug(fmt, args...) \
	do { \
			printk( fmt, ##args); \
	} while (0)

#ifdef pr_info
#undef pr_info
#endif

#define pr_info(fmt, args...) \
	do { \
			printk( fmt, ##args); \
	} while (0)
*/
//Added for USB Develpment debug, more log for more debuging help
struct mtp_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;
	struct usb_ep *ep_intr;

	int state;

	/* synchronize access to our device file */
	atomic_t open_excl;
	/* to enforce only one ioctl at a time */
	atomic_t ioctl_excl;

	struct list_head tx_idle;
	struct list_head intr_idle;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	wait_queue_head_t intr_wq;
	struct usb_request *rx_req[RX_REQ_MAX];
	int rx_done;

	/* for processing MTP_SEND_FILE, MTP_RECEIVE_FILE and
	 * MTP_SEND_FILE_WITH_HEADER ioctls on a work queue
	 */
	struct workqueue_struct *wq;
	struct work_struct send_file_work;
	struct work_struct receive_file_work;
	struct file *xfer_file;
	loff_t xfer_file_offset;
	int64_t xfer_file_length;
	unsigned xfer_send_header;
	uint16_t xfer_command;
	uint32_t xfer_transaction_id;
	int xfer_result;

//Added Modification for ALPS00255822, bug from WHQL test
	struct work_struct device_reset_work;
	int    fileTransferSend;
//Added Modification for ALPS00255822, bug from WHQL test
//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
	char	usb_functions[32];
	int		curr_mtp_func_index;
	int		usb_functions_no;
//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
//Added Modification for ALPS00354386
	int    epOut_halt;
//Added Modification for ALPS00354386
//ALPS00428998
	int		dev_disconnected;
//ALPS00428998
};

static struct usb_interface_descriptor mtp_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 3,
	.bInterfaceClass        = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass     = USB_SUBCLASS_VENDOR_SPEC,
	.bInterfaceProtocol     = 0,
};

static struct usb_interface_descriptor ptp_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 3,
	.bInterfaceClass        = USB_CLASS_STILL_IMAGE,
	.bInterfaceSubClass     = 1,
	.bInterfaceProtocol     = 1,
};

static struct usb_endpoint_descriptor mtp_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor mtp_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor mtp_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor mtp_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor mtp_intr_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize         = __constant_cpu_to_le16(INTR_BUFFER_SIZE),
	.bInterval              = 6,
};

static struct usb_descriptor_header *fs_mtp_descs[] = {
	(struct usb_descriptor_header *) &mtp_interface_desc,
	(struct usb_descriptor_header *) &mtp_fullspeed_in_desc,
	(struct usb_descriptor_header *) &mtp_fullspeed_out_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	NULL,
};

static struct usb_descriptor_header *hs_mtp_descs[] = {
	(struct usb_descriptor_header *) &mtp_interface_desc,
	(struct usb_descriptor_header *) &mtp_highspeed_in_desc,
	(struct usb_descriptor_header *) &mtp_highspeed_out_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	NULL,
};

static struct usb_descriptor_header *fs_ptp_descs[] = {
	(struct usb_descriptor_header *) &ptp_interface_desc,
	(struct usb_descriptor_header *) &mtp_fullspeed_in_desc,
	(struct usb_descriptor_header *) &mtp_fullspeed_out_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	NULL,
};

static struct usb_descriptor_header *hs_ptp_descs[] = {
	(struct usb_descriptor_header *) &ptp_interface_desc,
	(struct usb_descriptor_header *) &mtp_highspeed_in_desc,
	(struct usb_descriptor_header *) &mtp_highspeed_out_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	NULL,
};

static struct usb_string mtp_string_defs[] = {
	/* Naming interface "MTP" so libmtp will recognize us */
	[INTERFACE_STRING_INDEX].s	= "MTP",
	{  },	/* end of list */
};

static struct usb_gadget_strings mtp_string_table = {
	.language		= 0x0409,	/* en-US */
	.strings		= mtp_string_defs,
};

static struct usb_gadget_strings *mtp_strings[] = {
	&mtp_string_table,
	NULL,
};

/* Microsoft MTP OS String */
static u8 mtp_os_string[] = {
	18, /* sizeof(mtp_os_string) */
	USB_DT_STRING,
	/* Signature field: "MSFT100" */
	'M', 0, 'S', 0, 'F', 0, 'T', 0, '1', 0, '0', 0, '0', 0,
	/* vendor code */
	1,
	/* padding */
	0
};

//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
/* Microsoft Extended Property OS Feature Descriptor Header Section */
struct mtp_ext_prop_desc_header {
	__le32	dwLength;
	__u16	bcdVersion;
	__le16	wIndex;
	__u16	wCount;
};

/* Microsoft xtended Property OS Feature Function Section */
struct mtp_ext_prop_desc_property {
	__le32	dwSize;
	__le32	dwPropertyDataType;
	__le16	wPropertyNameLength;
	__u8	bPropertyName[8];		//MTP
	__le32	dwPropertyDataLength;
	__u8	bPropertyData[22];		//MTP Device
}mtp_ext_prop_desc_property;

/* MTP Extended Configuration Descriptor */
struct {
	struct mtp_ext_prop_desc_header	header;
	struct mtp_ext_prop_desc_property customProp;
} mtp_ext_prop_desc = {
	.header = {
		.dwLength = __constant_cpu_to_le32(sizeof(mtp_ext_prop_desc)),
		.bcdVersion = __constant_cpu_to_le16(0x0100),
		.wIndex = __constant_cpu_to_le16(5),
		.wCount = __constant_cpu_to_le16(1),
	},
	.customProp = {
		.dwSize = __constant_cpu_to_le32(sizeof(mtp_ext_prop_desc_property)),
		.dwPropertyDataType = __constant_cpu_to_le32(1),
		.wPropertyNameLength = __constant_cpu_to_le16(8),
		.bPropertyName = {'M', 0, 'T', 0, 'P', 0, 0, 0},		//MTP
		.dwPropertyDataLength = __constant_cpu_to_le32(22),
		.bPropertyData = {'M', 0, 'T', 0, 'P', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 0, 0},		//MTP Device
	},
};

#define MSFT_bMS_VENDOR_CODE	1
#define USB_MTP_FUNCTIONS		6

#define USB_MTP			"mtp\n"
#define USB_MTP_ACM		"mtp,acm\n"
#define USB_MTP_ADB		"mtp,adb\n"
#define USB_MTP_ADB_ACM	"mtp,adb,acm\n"
#define USB_MTP_UMS		"mtp,mass_storage\n"
#define USB_MTP_UMS_ADB	"mtp,mass_storage,adb\n"


static char * USB_MTP_FUNC[USB_MTP_FUNCTIONS] =
{
	USB_MTP,
	USB_MTP_ACM,
	USB_MTP_ADB,
	USB_MTP_ADB_ACM,
	USB_MTP_UMS,
	USB_MTP_UMS_ADB
};

//Added Modification for ALPS00272887, MTP MSFT OS Descriptor

/* Microsoft Extended Configuration Descriptor Header Section */
struct mtp_ext_config_desc_header {
	__le32	dwLength;
	__u16	bcdVersion;
	__le16	wIndex;
	__u8	bCount;
	__u8	reserved[7];
};

/* Microsoft Extended Configuration Descriptor Function Section */
struct mtp_ext_config_desc_function {
	__u8	bFirstInterfaceNumber;
	__u8	bInterfaceCount;
	__u8	compatibleID[8];
	__u8	subCompatibleID[8];
	__u8	reserved[6];
};

/* MTP Extended Configuration Descriptor */
struct {
	struct mtp_ext_config_desc_header	header;
	struct mtp_ext_config_desc_function    function;
} mtp_ext_config_desc = {
	.header = {
		.dwLength = __constant_cpu_to_le32(sizeof(mtp_ext_config_desc)),
		.bcdVersion = __constant_cpu_to_le16(0x0100),
		.wIndex = __constant_cpu_to_le16(4),
		//.bCount = __constant_cpu_to_le16(1),
		.bCount = 0x01,
	},
	.function = {
		.bFirstInterfaceNumber = 0,
		.bInterfaceCount = 1,
		.compatibleID = { 'M', 'T', 'P' },
	},
};

//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
struct {
	struct mtp_ext_config_desc_header	header;
	struct mtp_ext_config_desc_function    function1;
	struct mtp_ext_config_desc_function    function2;
} mtp_ext_config_desc_2 = {
	.header = {
		.dwLength = __constant_cpu_to_le32(sizeof(mtp_ext_config_desc_2)),
		.bcdVersion = __constant_cpu_to_le16(0x0100),
		.wIndex = __constant_cpu_to_le16(4),
		//.bCount = __constant_cpu_to_le16(1),
		.bCount = 0x02,
		.reserved = { 0 },
	},
	.function1 =
	{
	.bFirstInterfaceNumber = 0,
	.bInterfaceCount = 1,
	.compatibleID = { 'M', 'T', 'P', 0, 0, 0, 0, 0 },
	.subCompatibleID = { 0 },
	.reserved = { 0 },
	},
	.function2 =
	{
	.bFirstInterfaceNumber = 1,
	.bInterfaceCount = 1,
	.compatibleID = { 0 },
	.subCompatibleID = { 0 },
	.reserved = { 0 },
	},
};
struct {
	struct mtp_ext_config_desc_header	header;
	struct mtp_ext_config_desc_function    function1;
	struct mtp_ext_config_desc_function    function2;
	struct mtp_ext_config_desc_function    function3;
} mtp_ext_config_desc_3 = {
	.header = {
		.dwLength = __constant_cpu_to_le32(sizeof(mtp_ext_config_desc_3)),
		.bcdVersion = __constant_cpu_to_le16(0x0100),
		.wIndex = __constant_cpu_to_le16(4),
		//.bCount = __constant_cpu_to_le16(1),
		.bCount = 0x03,
		.reserved = { 0 },
	},
	.function1 =
	{
	.bFirstInterfaceNumber = 0,
	.bInterfaceCount = 1,
	.compatibleID = { 'M', 'T', 'P', 0, 0, 0, 0, 0 },
	.subCompatibleID = { 0 },
	.reserved = { 0 },
	},
	.function2 =
	{
	.bFirstInterfaceNumber = 1,
	.bInterfaceCount = 1,
	.compatibleID = { 0 },
	.subCompatibleID = { 0 },
	.reserved = { 0 },
	},
	.function3 =
	{
	.bFirstInterfaceNumber = 2,
	.bInterfaceCount = 1,
	.compatibleID = { 0 },
	.subCompatibleID = { 0 },
	.reserved = { 0 },
	},
};

//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
struct mtp_device_status {
	__le16	wLength;
	__le16	wCode;
};
//ALPS00428998
static void mtp_ueventToDisconnect(struct mtp_dev *dev);
//ALPS00428998

/* temporary variable used between mtp_open() and mtp_gadget_bind() */
static struct mtp_dev *_mtp_dev;

static inline struct mtp_dev *func_to_mtp(struct usb_function *f)
{
	return container_of(f, struct mtp_dev, function);
}

static struct usb_request *mtp_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void mtp_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int mtp_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void mtp_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
static void mtp_req_put(struct mtp_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
static struct usb_request
*mtp_req_get(struct mtp_dev *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static void mtp_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct mtp_dev *dev = _mtp_dev;

	if (req->status != 0)
		dev->state = STATE_ERROR;

	mtp_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void mtp_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct mtp_dev *dev = _mtp_dev;

	dev->rx_done = 1;
	if (req->status != 0)
		dev->state = STATE_ERROR;

	wake_up(&dev->read_wq);
}

static void mtp_complete_intr(struct usb_ep *ep, struct usb_request *req)
{
	struct mtp_dev *dev = _mtp_dev;

	if (req->status != 0)
		dev->state = STATE_ERROR;

	mtp_req_put(dev, &dev->intr_idle, req);

	wake_up(&dev->intr_wq);
}

static int mtp_create_bulk_endpoints(struct mtp_dev *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc,
				struct usb_endpoint_descriptor *intr_desc)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
	int i;

	DBG(cdev, "create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for ep_in got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for mtp ep_out got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;

	#if 0
	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for mtp ep_out got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;
	#endif

	ep = usb_ep_autoconfig(cdev->gadget, intr_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_intr failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for mtp ep_intr got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_intr = ep;

	/* now allocate requests for our endpoints */
	for (i = 0; i < TX_REQ_MAX; i++) {
		req = mtp_request_new(dev->ep_in, MTP_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = mtp_complete_in;
		mtp_req_put(dev, &dev->tx_idle, req);
	}
	for (i = 0; i < RX_REQ_MAX; i++) {
		req = mtp_request_new(dev->ep_out, MTP_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = mtp_complete_out;
		dev->rx_req[i] = req;
	}
	for (i = 0; i < INTR_REQ_MAX; i++) {
		req = mtp_request_new(dev->ep_intr, INTR_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = mtp_complete_intr;
		mtp_req_put(dev, &dev->intr_idle, req);
	}

	return 0;

fail:
	printk(KERN_ERR "mtp_bind() could not allocate requests\n");
	return -1;
}

static ssize_t mtp_read(struct file *fp, char __user *buf,
	size_t count, loff_t *pos)
{
	struct mtp_dev *dev = fp->private_data;
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	int r = count, xfer;
	int ret = 0;

	DBG(cdev, "mtp_read(%d)\n", count);

	if (count > MTP_BULK_BUFFER_SIZE)
		return -EINVAL;

//Added Modification for ALPS00354386
	if(dev->epOut_halt)
	{
		printk("%s, line %d: ret %d!! <dev->epOut_halt = %d> reset the out ep \n", __func__, __LINE__, ret, dev->epOut_halt);
		mdelay(2000);
		usb_ep_fifo_flush(dev->ep_out);
		dev->epOut_halt=0;
		usb_ep_clear_halt(dev->ep_out);
		printk("%s, line %d: ret %d!! <dev->epOut_halt = %d> finish the reset \n", __func__, __LINE__, ret, dev->epOut_halt);
	}
//Added Modification for ALPS00354386

//Added Modification for ALPS00255822, bug from WHQL test

	spin_lock_irq(&dev->lock);
	if(dev->state == STATE_RESET)
	{
		//Added for MTP Develpment debug, more log for more debuging help
		DBG(dev->cdev,      "%s: dev->state = %d, device is under reset state!! \n", __func__, dev->state);
		//Added for USB Develpment debug, more log for more debuging help
		dev->state = STATE_READY;
		//Added for MTP Develpment debug, more log for more debuging help
		DBG(dev->cdev,      "%s: dev->state = %d, change back to Ready state;!! \n", __func__, dev->state);
		//Added for USB Develpment debug, more log for more debuging help
		spin_unlock_irq(&dev->lock);
		return -ECANCELED;
	}
	spin_unlock_irq(&dev->lock);
//Added Modification for ALPS00255822, bug from WHQL test
	/* we will block until we're online */
	DBG(cdev, "mtp_read: waiting for online state\n");
	ret = wait_event_interruptible(dev->read_wq,
		dev->state != STATE_OFFLINE);
	if (ret < 0) {
		r = ret;
		goto done;
	}
//Added Modification for ALPS00255822, bug from WHQL test
	spin_lock_irq(&dev->lock);
	if(dev->state == STATE_RESET)
	{
		DBG(dev->cdev,      "%s: dev->state = %d, device is under reset state!! \n", __func__, dev->state);
		dev->state = STATE_READY;
		DBG(dev->cdev,      "%s: dev->state = %d, change back to Ready state;!! \n", __func__, dev->state);
		spin_unlock_irq(&dev->lock);
		return -ECANCELED;
	}
	spin_unlock_irq(&dev->lock);
//Added Modification for ALPS00255822, bug from WHQL test
	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED) {
		/* report cancelation to userspace */
		dev->state = STATE_READY;
		spin_unlock_irq(&dev->lock);
		return -ECANCELED;
	}
	dev->state = STATE_BUSY;
	spin_unlock_irq(&dev->lock);

requeue_req:
	/* queue a request */
	req = dev->rx_req[0];
	req->length = count;
	dev->rx_done = 0;
	ret = usb_ep_queue(dev->ep_out, req, GFP_KERNEL);
	if (ret < 0) {
		r = -EIO;
		goto done;
	} else {
		DBG(cdev, "rx %p queue\n", req);
		//Added Modification for ALPS00331676 and RxMode1
		//printk("%s, line %d: rx %p queue\n", __func__, __LINE__, req);
		//Added Modification for ALPS00331676 and RxMode1
	}

	/* wait for a request to complete */
	//Added Modification for ALPS00331676 and RxMode1
	//ret = wait_event_interruptible(dev->read_wq, dev->rx_done);
	ret = wait_event_interruptible(dev->read_wq, dev->rx_done || dev->state != STATE_BUSY);
	//Added Modification for ALPS00331676 and RxMode1
	if (ret < 0) {
		r = ret;
		usb_ep_dequeue(dev->ep_out, req);
		goto done;
	}
	//Added Modification for ALPS00331676 and RxMode1
	if (!dev->rx_done) {
		DBG(cdev, "%s, line %d: ret %d!! <!dev->rx_done> dev->state = %d, dev->rx_done = %d \n", __func__, __LINE__, ret, dev->state, dev->rx_done);
		printk("%s, line %d: ret %d!! <!dev->rx_done> dev->state = %d, dev->rx_done = %d \n", __func__, __LINE__, ret, dev->state, dev->rx_done);
		r = -ECANCELED;
		dev->state = STATE_ERROR;
		usb_ep_dequeue(dev->ep_out, req);
		goto done;
	}

/*	if (!dev->rx_done && dev->state == STATE_CANCELED) {
		DBG(cdev, "%s, line %d: ret %d!! <!dev->rx_done && dev->state == STATE_CANCELED:> dev->state = %d, dev->rx_done = %d \n", __func__, __LINE__, ret, dev->state, dev->rx_done);
		r = -ECANCELED;
		usb_ep_dequeue(dev->ep_out, req);
		goto done;
	}*/
	//Added Modification for ALPS00331676 and RxMode1
	if (dev->state == STATE_BUSY) {
		/* If we got a 0-len packet, throw it back and try again. */
		if (req->actual == 0)
			goto requeue_req;

		DBG(cdev, "rx %p %d\n", req, req->actual);
		xfer = (req->actual < count) ? req->actual : count;
		r = xfer;
		if (copy_to_user(buf, req->buf, xfer))
			r = -EFAULT;
	}
//Added Modification for ALPS00255822, bug from WHQL test
	else if(dev->state == STATE_RESET)
	{
		//copy to user!!
		/* If we got a 0-len packet, throw it back and try again. */
		if (req->actual == 0)
			goto requeue_req;

		DBG(dev->cdev,   "rx %p %d\n", req, req->actual);
		xfer = (req->actual < count) ? req->actual : count;
		r = xfer;
		if (copy_to_user(buf, req->buf, xfer))
			r = -EFAULT;
	}
//Added Modification for ALPS00255822, bug from WHQL test
        else
		r = -EIO;

done:
	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED)
		r = -ECANCELED;
	else if (dev->state != STATE_OFFLINE)
		dev->state = STATE_READY;
	spin_unlock_irq(&dev->lock);

	DBG(cdev, "mtp_read returning %d\n", r);
	return r;
}

static ssize_t mtp_write(struct file *fp, const char __user *buf,
	size_t count, loff_t *pos)
{
	struct mtp_dev *dev = fp->private_data;
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req = 0;
	int r = count, xfer;
	int sendZLP = 0;
	int ret;

	DBG(cdev, "mtp_write(%d)\n", count);
//Added Modification for ALPS00279812/279484
	DBG(cdev, "mtp_write, dev->state = %d\n", dev->state);

//Added Modification for ALPS00354386
/*	int i;
	int	len = (count < 24) ? count : 24;
	for(i=0;i<len;i++)
	{
		DBG(cdev, "mtp_write, %s, line %d: buf[%d] = 0x%x \n", __func__, __LINE__, i, buf[i]);
	}*/
//Added Modification for ALPS00354386
//Added Modification for ALPS00279812/279484

	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED) {
		/* report cancelation to userspace */
		dev->state = STATE_READY;
		spin_unlock_irq(&dev->lock);
		return -ECANCELED;
	}
//Added Modification for ALPS00255822, bug from WHQL test
	if (dev->state == STATE_RESET) {
		/* report cancelation to userspace */
		dev->state = STATE_READY;
		spin_unlock_irq(&dev->lock);
		return -ECANCELED;
	}
//Added Modification for ALPS00255822, bug from WHQL test
	if (dev->state == STATE_OFFLINE) {
		spin_unlock_irq(&dev->lock);
		//Added Modification for ALPS00279812/279484
		DBG(cdev, "%s, line %d: mtp_write return ENODEV = %d\n", __func__, __LINE__, ENODEV);
		//Added Modification for ALPS00279812/279484
		return -ENODEV;
	}
	dev->state = STATE_BUSY;
	spin_unlock_irq(&dev->lock);

	/* we need to send a zero length packet to signal the end of transfer
	 * if the transfer size is aligned to a packet boundary.
	 */
	if ((count & (dev->ep_in->maxpacket - 1)) == 0)
		sendZLP = 1;

	while (count > 0 || sendZLP) {
		/* so we exit after sending ZLP */
		if (count == 0)
			sendZLP = 0;

		if (dev->state != STATE_BUSY) {
			DBG(cdev, "mtp_write dev->error\n");
			r = -EIO;
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
			((req = mtp_req_get(dev, &dev->tx_idle))
				|| dev->state != STATE_BUSY));
		if (!req) {
			r = ret;
			break;
		}

		if (count > MTP_BULK_BUFFER_SIZE)
			xfer = MTP_BULK_BUFFER_SIZE;
		else
			xfer = count;
		if (xfer && copy_from_user(req->buf, buf, xfer)) {
			r = -EFAULT;
			break;
		}

		req->length = xfer;
		ret = usb_ep_queue(dev->ep_in, req, GFP_KERNEL);
		if (ret < 0) {
			DBG(cdev, "mtp_write: xfer error %d\n", ret);
			r = -EIO;
			break;
		}

		buf += xfer;
		count -= xfer;

		/* zero this so we don't try to free it on error exit */
		req = 0;
	}

	if (req)
		mtp_req_put(dev, &dev->tx_idle, req);

	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED)
		r = -ECANCELED;
//Added Modification for ALPS00255822, bug from WHQL test
	else if(dev->state == STATE_RESET)
	{
		DBG(dev->cdev, "%s: dev->state = %d, device is under reset state!! \n", __func__, dev->state);
		dev->state = STATE_READY;
		r = -ECANCELED;
	}
//Added Modification for ALPS00255822, bug from WHQL test
	else if (dev->state != STATE_OFFLINE)
		dev->state = STATE_READY;
	spin_unlock_irq(&dev->lock);

	DBG(cdev, "mtp_write returning %d\n", r);
	return r;
}

/* read from a local file and write to USB */
static void send_file_work(struct work_struct *data)
{
	struct mtp_dev *dev = container_of(data, struct mtp_dev,
						send_file_work);
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req = 0;
	struct mtp_data_header *header;
	struct file *filp;
	loff_t offset;
	int64_t count;
	int xfer, ret, hdr_size;
	int r = 0;
	int sendZLP = 0;

	/* read our parameters */
	smp_rmb();
	filp = dev->xfer_file;
	offset = dev->xfer_file_offset;
	count = dev->xfer_file_length;

	DBG(cdev, "send_file_work(%lld %lld)\n", offset, count);

	if (dev->xfer_send_header) {
		hdr_size = sizeof(struct mtp_data_header);
		count += hdr_size;
	} else {
		hdr_size = 0;
	}

	/* we need to send a zero length packet to signal the end of transfer
	 * if the transfer size is aligned to a packet boundary.
	 */
	if ((count & (dev->ep_in->maxpacket - 1)) == 0)
		sendZLP = 1;

	while (count > 0 || sendZLP) {
		/* so we exit after sending ZLP */
		if (count == 0)
			sendZLP = 0;

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
			(req = mtp_req_get(dev, &dev->tx_idle))
			|| dev->state != STATE_BUSY);
		if (dev->state == STATE_CANCELED) {
			r = -ECANCELED;
			break;
		}
		else if (dev->state == STATE_RESET) {
			DBG(dev->cdev, "%s: dev->state = %d, device is under reset state!! \n", __func__, dev->state);
			r = -ECANCELED;
			break;
		}

		if (!req) {
			r = ret;
			break;
		}

		if (count > MTP_BULK_BUFFER_SIZE)
			xfer = MTP_BULK_BUFFER_SIZE;
		else
			xfer = count;

		if (hdr_size) {
			/* prepend MTP data header */
			header = (struct mtp_data_header *)req->buf;
			header->length = __cpu_to_le32(count);
			header->type = __cpu_to_le16(2); /* data packet */
			header->command = __cpu_to_le16(dev->xfer_command);
			header->transaction_id =
					__cpu_to_le32(dev->xfer_transaction_id);
		}

		ret = vfs_read(filp, req->buf + hdr_size, xfer - hdr_size,
								&offset);
		if (ret < 0) {
			r = ret;
			break;
		}
		xfer = ret + hdr_size;
		hdr_size = 0;

		req->length = xfer;
		ret = usb_ep_queue(dev->ep_in, req, GFP_KERNEL);
		if (ret < 0) {
			DBG(cdev, "send_file_work: xfer error %d\n", ret);
			dev->state = STATE_ERROR;
			r = -EIO;
			break;
		}

		count -= xfer;

		/* zero this so we don't try to free it on error exit */
		req = 0;
	}

//Added Modification for ALPS00255822, bug from WHQL test
	DBG(dev->cdev, "%s, line = %d: req = 0x%p \n", __func__, __LINE__, req);
//Added Modification for ALPS00255822, bug from WHQL test

	if (req)
		mtp_req_put(dev, &dev->tx_idle, req);

	DBG(cdev, "send_file_work returning %d\n", r);
	/* write the result */
	dev->xfer_result = r;
	smp_wmb();
}

/* read from USB and write to a local file */
static void receive_file_work(struct work_struct *data)
{
	struct mtp_dev *dev = container_of(data, struct mtp_dev,
						receive_file_work);
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *read_req = NULL, *write_req = NULL;
	struct file *filp;
	loff_t offset;
	int64_t count;
	int ret, cur_buf = 0;
	int r = 0;
	//Modification for ALPS00434059 begin
#if defined(MTK_SHARED_SDCARD)
	int64_t total_size=0;
#endif
	//Modification for ALPS00434059 begin

	/* read our parameters */
	smp_rmb();

//Added Modification for ALPS00354386
	if(dev->epOut_halt)
	{
		printk("%s, line %d: <dev->epOut_halt = %d> reset the out ep \n", __func__, __LINE__, dev->epOut_halt);
		mdelay(2000);
		usb_ep_fifo_flush(dev->ep_out);
		dev->epOut_halt=0;
		usb_ep_clear_halt(dev->ep_out);
	}
//Added Modification for ALPS00354386

	filp = dev->xfer_file;
	offset = dev->xfer_file_offset;
	count = dev->xfer_file_length;

	DBG(cdev, "receive_file_work(%lld)\n", count);

	while (count > 0 || write_req) {
		if (count > 0) {
			/* queue a request */
			read_req = dev->rx_req[cur_buf];
			cur_buf = (cur_buf + 1) % RX_REQ_MAX;

			read_req->length = (count > MTP_BULK_BUFFER_SIZE
					? MTP_BULK_BUFFER_SIZE : count);

		//Modification for ALPS00434059 begin
		//This might be modified TBD, 
		//so far, there is only sharedSD with EXT4 FFS could transfer Object with size oevr 4GBs
		#if defined(MTK_SHARED_SDCARD)
			if(total_size >= 0xFFFFFFFF)
				read_req->short_not_ok = 0;
			else
				read_req->short_not_ok = 1;
		#else
		//Modification for ALPS00434059 end
			//Add for RX mode 1
			read_req->short_not_ok = 1;
			//Add for RX mode 1
		//Modification for ALPS00434059 end
		#endif
		//Modification for ALPS00434059 end
			dev->rx_done = 0;
			ret = usb_ep_queue(dev->ep_out, read_req, GFP_KERNEL);
			if (ret < 0) {
				r = -EIO;
				//dev->state = STATE_ERROR;		//Linux original source code
				//Added Modification for ALPS00279812/279484
				xlog_printk(ANDROID_LOG_ERROR, "USB", "%s, line %d: EIO, dev->dev_disconnected = %d, usb queue error \n", __func__, __LINE__, dev->dev_disconnected);

				//ALPS00428998
				if(dev->dev_disconnected)	//USB SW disconnected
				{
					dev->state = STATE_OFFLINE;
				}
				else	//ex: Might be SD card plug-out with USB connected
				{
					dev->state = STATE_ERROR;
					#if 0
					//Added Modification for ALPS00354386
					//dev->state = STATE_OFFLINE;
					usb_ep_set_halt(dev->ep_out);
					dev->epOut_halt = 1;
					usb_ep_fifo_flush(dev->ep_out);
					//Added Modification for ALPS00354386
					#else
					mtp_ueventToDisconnect(dev);
					#endif
				}
				//ALPS00428998
				//Added Modification for ALPS00279812/279484
				break;
			}
		}

		if (write_req) {
			DBG(cdev, "rx %p %d\n", write_req, write_req->actual);
			ret = vfs_write(filp, write_req->buf, write_req->actual,
				&offset);
			DBG(cdev, "vfs_write %d\n", ret);
			if (ret != write_req->actual) {
				r = -EIO;
				//dev->state = STATE_ERROR;		//Linux original source code
				//Added Modification for ALPS00279812/279484
				xlog_printk(ANDROID_LOG_ERROR, "USB", "%s, line %d: EIO, dev->dev_disconnected = %d, file write error \n", __func__, __LINE__, dev->dev_disconnected);
				//ALPS00428998
				if(dev->dev_disconnected)	//USB SW disconnected
				{
					dev->state = STATE_OFFLINE;
				}
				else	//ex: Might be SD card plug-out with USB connected
				{
					dev->state = STATE_ERROR;
					#if 0
					//Added Modification for ALPS00354386
					//dev->state = STATE_OFFLINE;
					usb_ep_set_halt(dev->ep_out);
					dev->epOut_halt = 1;
					usb_ep_fifo_flush(dev->ep_out);
					//Added Modification for ALPS00354386
					#else
					mtp_ueventToDisconnect(dev);
					#endif
				}
				//ALPS00428998
				//Added Modification for ALPS00279812/279484
				break;
			}
			write_req = NULL;
		}

          	//Added for MTP Develpment debug, more log for more debuging help
		DBG(dev->cdev,		"%s, line %d: Wait for read_req = %p!! \n", __func__, __LINE__, read_req);
          	//Added for MTP Develpment debug, more log for more debuging help
		if (read_req) {
			/* wait for our last read to complete */
			ret = wait_event_interruptible(dev->read_wq,
				dev->rx_done || dev->state != STATE_BUSY);
			if (dev->state == STATE_CANCELED) {
				//Added for MTP Develpment debug, more log for more debuging help
				xlog_printk(ANDROID_LOG_ERROR, "USB", "%s, line %d: dev->state = %d, get cancel command !! Cancel it!! rx_done = %d\n", __func__, __LINE__, dev->state, dev->rx_done);
				//Added for USB Develpment debug, more log for more debuging help
				r = -ECANCELED;
				if (!dev->rx_done)
					usb_ep_dequeue(dev->ep_out, read_req);
				break;
			}
			//Added Modification for ALPS00255822, bug from WHQL test
			if (dev->state == STATE_RESET) {
				//Added for MTP Develpment debug, more log for more debuging help
				DBG(dev->cdev,      "%s: dev->state = %d, get reset command !! Cancel it!! rx_done = %d\n", __func__, dev->state, dev->rx_done);
				//Added for USB Develpment debug, more log for more debuging help
				r = -ECANCELED;
				//if (!dev->rx_done)	//marked for WHQL
				//Added for MTP Develpment debug, more log for more debuging help
				DBG(dev->cdev,      "%s, %d: request to usb_ep_dequeue!! \n", __func__, __LINE__);
				//Added for USB Develpment debug, more log for more debuging help
				usb_ep_dequeue(dev->ep_out, read_req);
				break;
				
			}
			//Added Modification for ALPS00255822, bug from WHQL test
			/* if xfer_file_length is 0xFFFFFFFF, then we read until
			 * we get a zero length packet
			 */
			if (count != 0xFFFFFFFF)
				count -= read_req->actual;

		//Modification for ALPS00434059 begin
		#if defined(MTK_SHARED_SDCARD)
			total_size +=read_req->actual;
			DBG(cdev, "%s, line %d: count = %d, total_size = %d, read_req->actual = %d, read_req->length= %d\n", __func__, __LINE__, count, total_size, read_req->actual, read_req->length);
		#endif
		//Modification for ALPS00434059 begin

			if (read_req->actual < read_req->length) {
				/*
				 * short packet is used to signal EOF for
				 * sizes > 4 gig
				 */
				DBG(cdev, "got short packet\n");
				count = 0;
			}

			//Add for RX mode 1
			read_req->short_not_ok = 0;
			//Add for RX mode 1
			//Added for MTP Develpment debug, more log for more debuging help
			DBG(dev->cdev,      "%s, line %d: dev->state = %d, NEXT!!\n", __func__, __LINE__, dev->state);
			//Added for USB Develpment debug, more log for more debuging help

			write_req = read_req;
			read_req = NULL;
		}
	}

	//Added Modification for ALPS00354386
	if(dev->state == STATE_ERROR || dev->state == STATE_OFFLINE)
	{
		//Added for MTP Develpment debug, more log for more debuging help
		DBG(dev->cdev,      "%s, line %d: read_req = %p \n", __func__, __LINE__, read_req);
		//Added for USB Develpment debug, more log for more debuging help
		if(read_req)
		{
			read_req->short_not_ok = 0;
		}
	}
	//Added Modification for ALPS00354386

	DBG(cdev, "receive_file_work returning %d\n", r);
	//Added for MTP Develpment debug, more log for more debuging help
	xlog_printk(ANDROID_LOG_INFO, "USB", "%s, line %d: receive_file_work returning %d \n", __func__, __LINE__, r);
	//Added for MTP Develpment debug, more log for more debuging help
	/* write the result */
	dev->xfer_result = r;
	smp_wmb();
}

static int mtp_send_event(struct mtp_dev *dev, struct mtp_event *event)
{
	struct usb_request *req = NULL;
	int ret;
	int length = event->length;

	DBG(dev->cdev, "mtp_send_event(%d)\n", event->length);

	if (length < 0 || length > INTR_BUFFER_SIZE)
		return -EINVAL;
	if (dev->state == STATE_OFFLINE)
		return -ENODEV;

	ret = wait_event_interruptible_timeout(dev->intr_wq,
			(req = mtp_req_get(dev, &dev->intr_idle)),
			msecs_to_jiffies(1000));
	if (!req)
		return -ETIME;

	if (copy_from_user(req->buf, (void __user *)event->data, length)) {
		mtp_req_put(dev, &dev->intr_idle, req);
		return -EFAULT;
	}
	req->length = length;
	ret = usb_ep_queue(dev->ep_intr, req, GFP_KERNEL);
	if (ret)
		mtp_req_put(dev, &dev->intr_idle, req);

	return ret;
}

static long mtp_ioctl(struct file *fp, unsigned code, unsigned long value)
{
	struct mtp_dev *dev = fp->private_data;
	struct file *filp = NULL;
	int ret = -EINVAL;

//Added Modification for ALPS00255822, bug from WHQL test
	switch (code)
	{
	case MTP_SEND_FILE:
		xlog_printk(ANDROID_LOG_INFO, "USB", "%s: MTP_SEND_FILE, code = 0x%x\n", __func__, code);
		break;
	case MTP_RECEIVE_FILE:
		xlog_printk(ANDROID_LOG_INFO, "USB", "%s: MTP_RECEIVE_FILE, code = 0x%x\n", __func__, code);
		break;
	case MTP_SEND_FILE_WITH_HEADER:
		xlog_printk(ANDROID_LOG_INFO, "USB", "%s: MTP_SEND_FILE_WITH_HEADER, code = 0x%x\n", __func__, code);
		break;
	case MTP_SEND_EVENT:
		xlog_printk(ANDROID_LOG_INFO, "USB", "%s: MTP_SEND_EVENT, code = 0x%x\n", __func__, code);
		break;
	}
//Added Modification for ALPS00255822, bug from WHQL test

	if (mtp_lock(&dev->ioctl_excl))
		return -EBUSY;

	switch (code) {
	case MTP_SEND_FILE:
	case MTP_RECEIVE_FILE:
	case MTP_SEND_FILE_WITH_HEADER:
	{
		struct mtp_file_range	mfr;
		struct work_struct *work;

		spin_lock_irq(&dev->lock);
		if (dev->state == STATE_CANCELED) {
			/* report cancelation to userspace */
			//Added Modification for ALPS00255822, bug from WHQL test
			DBG(dev->cdev, "%s: cancel!!! \n", __func__);
			dev->state = STATE_READY;
			spin_unlock_irq(&dev->lock);
			ret = -ECANCELED;
			goto out;
		}
		if (dev->state == STATE_RESET) {
			/* report cancelation to userspace */
			//Added Modification for ALPS00255822, bug from WHQL test
			dev->state = STATE_READY;
			spin_unlock_irq(&dev->lock);
			ret = -ECANCELED;
			goto out;
		}
		if (dev->state == STATE_OFFLINE) {
			spin_unlock_irq(&dev->lock);
			ret = -ENODEV;
			goto out;
		}
		dev->state = STATE_BUSY;
		spin_unlock_irq(&dev->lock);

		if (copy_from_user(&mfr, (void __user *)value, sizeof(mfr))) {
			ret = -EFAULT;
			goto fail;
		}
		/* hold a reference to the file while we are working with it */
		filp = fget(mfr.fd);
		if (!filp) {
			ret = -EBADF;
			goto fail;
		}

		/* write the parameters */
		dev->xfer_file = filp;
		dev->xfer_file_offset = mfr.offset;
		dev->xfer_file_length = mfr.length;
		smp_wmb();

		if (code == MTP_SEND_FILE_WITH_HEADER) {
			work = &dev->send_file_work;
			dev->xfer_send_header = 1;
			dev->xfer_command = mfr.command;
			dev->xfer_transaction_id = mfr.transaction_id;
		} else if (code == MTP_SEND_FILE) {
			work = &dev->send_file_work;
			dev->xfer_send_header = 0;
		} else {
			work = &dev->receive_file_work;
		}

		/* We do the file transfer on a work queue so it will run
		 * in kernel context, which is necessary for vfs_read and
		 * vfs_write to use our buffers in the kernel address space.
		 */
		queue_work(dev->wq, work);
		/* wait for operation to complete */
		flush_workqueue(dev->wq);
		fput(filp);

		/* read the result */
		smp_rmb();
		ret = dev->xfer_result;
		break;
	}
	case MTP_SEND_EVENT:
	{
		struct mtp_event	event;
		/* return here so we don't change dev->state below,
		 * which would interfere with bulk transfer state.
		 */
		if (copy_from_user(&event, (void __user *)value, sizeof(event)))
			ret = -EFAULT;
		else
			ret = mtp_send_event(dev, &event);
		goto out;
	}
	//Added Modification for ALPS00255822, bug from WHQL test
	/*case MTP_SEND_OBJECT_INFO_IND:
	{
		ret = 0;
		goto out;
	}*/
	//Added Modification for ALPS00255822, bug from WHQL test
	}

fail:
	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED)
		ret = -ECANCELED;
	//Added Modification for ALPS00255822, bug from WHQL test
	else if (dev->state == STATE_RESET)
		ret = -ECANCELED;
	//Added Modification for ALPS00255822, bug from WHQL test

	else if (dev->state != STATE_OFFLINE)
		dev->state = STATE_READY;
	spin_unlock_irq(&dev->lock);
out:
	mtp_unlock(&dev->ioctl_excl);
	DBG(dev->cdev, "ioctl returning %d\n", ret);
	return ret;
}

static int mtp_open(struct inode *ip, struct file *fp)
{
	printk(KERN_INFO "mtp_open\n");
	if (mtp_lock(&_mtp_dev->open_excl))
		return -EBUSY;

	/* clear any error condition */
	if (_mtp_dev->state != STATE_OFFLINE)
		_mtp_dev->state = STATE_READY;

	fp->private_data = _mtp_dev;
	return 0;
}

static int mtp_release(struct inode *ip, struct file *fp)
{
	printk(KERN_INFO "mtp_release\n");

	mtp_unlock(&_mtp_dev->open_excl);
	return 0;
}

/* file operations for /dev/mtp_usb */
static const struct file_operations mtp_fops = {
	.owner = THIS_MODULE,
	.read = mtp_read,
	.write = mtp_write,
	.unlocked_ioctl = mtp_ioctl,
	.open = mtp_open,
	.release = mtp_release,
};

static struct miscdevice mtp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = mtp_shortname,
	.fops = &mtp_fops,
};

//Added Modification for ALPS00255822, bug from WHQL test
static void mtp_work(struct work_struct *data)
{
	char *envp_sessionend[2] = 	{ "MTP=SESSIONEND", NULL };
	xlog_printk(ANDROID_LOG_INFO, "USB", "%s: __begin__ \n", __func__);

    //ALPS00428998
    #if 0
	struct mtp_dev *dev = container_of(data, struct mtp_dev, device_reset_work);

	if(dev->epOut_halt)
	{
		//mtp_ueventToDisconnect(dev);
	}
    #endif
    //ALPS00428998
	kobject_uevent_env(&mtp_device.this_device->kobj, KOBJ_CHANGE, envp_sessionend);

	//printk("%s: __end__ \n", __func__);
}

//ALPS00428998
static void mtp_ueventToDisconnect(struct mtp_dev *dev)
{
	char *envp_mtpAskDisconnect[2] = { "USB_STATE=MTPASKDISCONNECT", NULL };
	xlog_printk(ANDROID_LOG_INFO, "USB", "%s: __begin__ \n", __func__);

	kobject_uevent_env(&mtp_device.this_device->kobj, KOBJ_CHANGE, envp_mtpAskDisconnect);

	//printk("%s: __end__ \n", __func__);
}
//ALPS00428998

//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
static void mtp_read_usb_functions(int functions_no, char * buff)
{
	struct mtp_dev *dev = _mtp_dev;
	int i;
       	//Added for MTP Develpment debug, more log for more debuging help
	DBG(dev->cdev, "%s: dev->curr_mtp_func_index = 0x%x\n",__func__, dev->curr_mtp_func_index);

	/*if(dev->curr_mtp_func_index!=0xff)
		return;*/
       	//Added for MTP Develpment debug, more log for more debuging help

	dev->usb_functions_no = functions_no;
	dev->curr_mtp_func_index = 0xff;
	memcpy(dev->usb_functions, buff, sizeof(dev->usb_functions));
	DBG(dev->cdev, "%s:usb_functions_no = %d, usb_functions=%s\n",__func__, dev->usb_functions_no, dev->usb_functions);

	for(i=0;i<USB_MTP_FUNCTIONS;i++)
	{
		if(!strcmp(dev->usb_functions, USB_MTP_FUNC[i]))
		{	
			DBG(dev->cdev, "%s: usb functions = %s, i = %d \n",__func__, dev->usb_functions, i);
			dev->curr_mtp_func_index = i;
			break;
		}
	}

}
//Added Modification for ALPS00272887, MTP MSFT OS Descriptor

enum FILE_ACTION_ENABLED
{
	SEND_FILE_ENABLE		= 0,
	SEND_FILE_DISABLE		= 1,
	RECEIVE_FILE_ENABLE		= 2,
	RECEIVE_FILE_DISABLE	= 3
};

static void mtp_ep_flush_all(void)
{
	struct mtp_dev *dev = _mtp_dev;

	DBG(dev->cdev, "%s: __begin__ \n", __func__);
	dev->state = STATE_RESET;

	DBG(dev->cdev, "%s: __end__ \n", __func__);
}
//Added Modification for ALPS00255822, bug from WHQL test
static int mtp_ctrlrequest(struct usb_composite_dev *cdev,
				const struct usb_ctrlrequest *ctrl)
{
	struct mtp_dev *dev = _mtp_dev;
	int	value = -EOPNOTSUPP;
	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16	w_value = le16_to_cpu(ctrl->wValue);
	u16	w_length = le16_to_cpu(ctrl->wLength);
	unsigned long	flags;

	VDBG(cdev, "mtp_ctrlrequest "
			"%02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);

	/* Handle MTP OS string */
	if (ctrl->bRequestType ==
			(USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE)
			&& ctrl->bRequest == USB_REQ_GET_DESCRIPTOR
			&& (w_value >> 8) == USB_DT_STRING
			&& (w_value & 0xFF) == MTP_OS_STRING_ID) {
		value = (w_length < sizeof(mtp_os_string)
				? w_length : sizeof(mtp_os_string));
		memcpy(cdev->req->buf, mtp_os_string, value);
	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
		/* Handle MTP OS descriptor */
		DBG(cdev, "vendor request: %d index: %d value: %d length: %d\n",
			ctrl->bRequest, w_index, w_value, w_length);
		//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
                #if 0
		//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
		if (ctrl->bRequest == 1
				&& (ctrl->bRequestType & USB_DIR_IN)
				&& (w_index == 4 || w_index == 5)) {
			value = (w_length < sizeof(mtp_ext_config_desc) ?
					w_length : sizeof(mtp_ext_config_desc));
			memcpy(cdev->req->buf, &mtp_ext_config_desc, value);
		}
		//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
                #endif
		if (ctrl->bRequest == 1
				&& (ctrl->bRequestType & USB_DIR_IN)
				&& (w_index == 5)) {
			value = (w_length < sizeof(mtp_ext_prop_desc) ?
					w_length : sizeof(mtp_ext_prop_desc));
			DBG(cdev, "vendor request: Property OS Feature, w_length = %d, value = %d \n", w_length, value);
			memcpy(cdev->req->buf, &mtp_ext_prop_desc, value);
		}
		else if (ctrl->bRequest == 1
				&& (ctrl->bRequestType & USB_DIR_IN)
				&& (w_index == 4)) {

			switch(dev->curr_mtp_func_index)
			{
			case 0:			//mtp
				value = (w_length < sizeof(mtp_ext_config_desc) ?
						w_length : sizeof(mtp_ext_config_desc));
				memcpy(cdev->req->buf, &mtp_ext_config_desc, value);
				break;
			case 1:			//mtp,acm	, with acm, failed so far
			case 2:			//mtp,adb 
			case 4:			//mtp,mass_storage
				value = (w_length < sizeof(mtp_ext_config_desc_2) ?
						w_length : sizeof(mtp_ext_config_desc_2));
				memcpy(cdev->req->buf, &mtp_ext_config_desc_2, value);
				break;
			case 3:			//mtp,adb,acm	, with acm, failed so far
			case 5:			//mtp,mass_storage,adb
				value = (w_length < sizeof(mtp_ext_config_desc_3) ?
						w_length : sizeof(mtp_ext_config_desc_3));
				memcpy(cdev->req->buf, &mtp_ext_config_desc_3, value);
				break;
			default:			//unknown, 0xff
				value = (w_length < sizeof(mtp_ext_config_desc) ?
						w_length : sizeof(mtp_ext_config_desc));
				memcpy(cdev->req->buf, &mtp_ext_config_desc, value);
				break;
			}

			xlog_printk(ANDROID_LOG_INFO, "USB", "vendor request: Compat ID OS Feature, dev->curr_mtp_func_index = %d, dev->usb_functions = %s \n", dev->curr_mtp_func_index, dev->usb_functions);
			xlog_printk(ANDROID_LOG_INFO, "USB", "vendor request: Extended OS Feature, w_length = %d, value = %d, dev->curr_mtp_func_index = %d\n", w_length, value, dev->curr_mtp_func_index);
		}
		//Added Modification for ALPS00272887, MTP MSFT OS Descriptor
	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
		DBG(cdev, "class request: %d index: %d value: %d length: %d\n",
			ctrl->bRequest, w_index, w_value, w_length);

		if (ctrl->bRequest == MTP_REQ_CANCEL && w_index == 0
				&& w_value == 0) {
			DBG(cdev, "MTP_REQ_CANCEL\n");
			//Added Modification for ALPS00255822, bug from WHQL test
			xlog_printk(ANDROID_LOG_INFO, "USB", "%s: MTP_REQ_CANCEL. dev->state = %d.\n", __func__, dev->state);
			//Added Modification for ALPS00255822, bug from WHQL test

			spin_lock_irqsave(&dev->lock, flags);
			if (dev->state == STATE_BUSY) {
				dev->state = STATE_CANCELED;
				wake_up(&dev->read_wq);
				wake_up(&dev->write_wq);
			}
			//Added Modification for ALPS00296160,
			else if(dev->state == STATE_READY)
			{
				//special for WHQL and Cancel Begin case
				//cancel coming in 1st stage
				dev->state = STATE_CANCELED;
			}
			//Added Modification for ALPS00296160,

			spin_unlock_irqrestore(&dev->lock, flags);

			/* We need to queue a request to read the remaining
			 *  bytes, but we don't actually need to look at
			 * the contents.
			 */
			value = w_length;
		} else if (ctrl->bRequest == MTP_REQ_GET_DEVICE_STATUS
				&& w_index == 0 && w_value == 0) {
			struct mtp_device_status *status = cdev->req->buf;
			status->wLength =
				__constant_cpu_to_le16(sizeof(*status));

			DBG(cdev, "MTP_REQ_GET_DEVICE_STATUS\n");
			//Added Modification for ALPS00255822, bug from WHQL test
			xlog_printk(ANDROID_LOG_INFO, "USB", "%s: MTP_REQ_GET_DEVICE_STATUS. dev->state = %d. \n", __func__, dev->state);
			//Added Modification for ALPS00255822, bug from WHQL test
			spin_lock_irqsave(&dev->lock, flags);
			/* device status is "busy" until we report
			 * the cancelation to userspace
			 */
			if (dev->state == STATE_CANCELED)
			//Added Modification for ALPS00255822, bug from WHQL test
			{	
			//Added Modification for ALPS00255822, bug from WHQL test
				status->wCode =
					__cpu_to_le16(MTP_RESPONSE_DEVICE_BUSY);
			//Added Modification for ALPS00255822, bug from WHQL test
				dev->fileTransferSend ++;
				DBG(cdev, "%s: dev->fileTransferSend = %d \n", __func__, dev->fileTransferSend);
				
				if(dev->fileTransferSend > 5)
				{
					dev->fileTransferSend = 0;
					dev->state = STATE_BUSY;
					//dev->state = STATE_READY;
					status->wCode =
						__cpu_to_le16(MTP_RESPONSE_OK);
				}

			}
			else if(dev->state == STATE_RESET)
			{
				DBG(dev->cdev, "%s: dev->state = RESET under MTP_REQ_GET_DEVICE_STATUS\n", __func__);
				dev->fileTransferSend = 0;
				status->wCode =
					__cpu_to_le16(MTP_RESPONSE_OK);
				//dev->state = STATE_BUSY;

			}
			//Added Modification for ALPS00255822, bug from WHQL test
			//ALPS00428998
			else if(dev->state == STATE_ERROR)
			{
				DBG(dev->cdev, "%s: dev->state = RESET under MTP_REQ_GET_DEVICE_STATUS\n", __func__);
				dev->fileTransferSend = 0;
				if(dev->epOut_halt)
				{
					status->wCode =
						__cpu_to_le16(MTP_RESPONSE_DEVICE_CANCEL);
				}
				else
				status->wCode =
					__cpu_to_le16(MTP_RESPONSE_OK);
				//dev->state = STATE_BUSY;

			}
			//ALPS00428998
			//Added Modification for ALPS00255822, bug from WHQL test
			else
			{	
				dev->fileTransferSend = 0;
				status->wCode =
					__cpu_to_le16(MTP_RESPONSE_OK);
			}

			//Added Modification for ALPS00255822, bug from WHQL test
			xlog_printk(ANDROID_LOG_INFO, "USB", "%s: status->wCode = 0x%x, under MTP_REQ_GET_DEVICE_STATUS\n", __func__, status->wCode);
			//Added Modification for ALPS00255822, bug from WHQL test
			spin_unlock_irqrestore(&dev->lock, flags);
			value = sizeof(*status);
		}
		//Added Modification for ALPS00255822, bug from WHQL test
		else if (ctrl->bRequest == MTP_REQ_RESET && w_index == 0 && w_value == 0)
		{
			struct work_struct *work;
			DBG(dev->cdev, "%s: MTP_REQ_RESET\n", __func__);
			xlog_printk(ANDROID_LOG_INFO, "USB", "%s: MTP_REQ_RESET. dev->state = %d. \n", __func__, dev->state);

			spin_lock_irqsave(&dev->lock, flags);
			work = &dev->device_reset_work;
			schedule_work(work);
			/* wait for operation to complete */
			//flush_workqueue(dev->wq);
			/* readers may be blocked waiting for us to go online */
			//flush ep
			mtp_ep_flush_all();

			DBG(dev->cdev, "%s: wake up the work queue to prevent that they are waiting!!\n", __func__);

			//dev->state = STATE_CANCELED;
			//dev->state = STATE_READY;
			//dev->rx_done = 1;
			/*wake_up(&dev->read_wq);
 			wake_up(&dev->write_wq);*/

			xlog_printk(ANDROID_LOG_INFO, "USB", "%s: Done of MTP_REQ_RESET!!\n", __func__);

			spin_unlock_irqrestore(&dev->lock, flags);

			value = w_length;
		}
		//Added Modification for ALPS00255822, bug from WHQL test
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		int rc;
		cdev->req->zero = value < w_length;
		cdev->req->length = value;
		rc = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
		if (rc < 0)
			ERROR(cdev, "%s: response queue error\n", __func__);
	}
	return value;
}

static int
mtp_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct mtp_dev	*dev = func_to_mtp(f);
	int			id;
	int			ret;

	dev->cdev = cdev;
	DBG(cdev, "mtp_function_bind dev: %p\n", dev);
    //ALPS00428998
	printk("mtp_function_bind dev: %p\n", dev);
    //ALPS00428998

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	mtp_interface_desc.bInterfaceNumber = id;
	//ALPS00312811 for XXX,ptp composite function failed
	ptp_interface_desc.bInterfaceNumber = id;
	DBG(cdev, "mtp_function_bind bInterfaceNumber = id= %d\n", id);
	//ALPS00312811 for XXX,ptp composite function failed

       	//Added for MTP Develpment debug, more log for more debuging help
	xlog_printk(ANDROID_LOG_ERROR, "USB", "%s: reset dev->curr_mtp_func_index to 0xff \n", __func__);
	dev->curr_mtp_func_index = 0xff;
       	//Added for MTP Develpment debug, more log for more debuging help

	/* allocate endpoints */
	ret = mtp_create_bulk_endpoints(dev, &mtp_fullspeed_in_desc,
			&mtp_fullspeed_out_desc, &mtp_intr_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		mtp_highspeed_in_desc.bEndpointAddress =
			mtp_fullspeed_in_desc.bEndpointAddress;
		mtp_highspeed_out_desc.bEndpointAddress =
			mtp_fullspeed_out_desc.bEndpointAddress;
	}
	//ALPS00428998
	dev->dev_disconnected = 0;
	//ALPS00428998


	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;
}

static void
mtp_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct mtp_dev	*dev = func_to_mtp(f);
	struct usb_request *req;
	int i;
	//ALPS00428998
	printk("%s, line %d: \n", __func__, __LINE__);
	//ALPS00428998

	while ((req = mtp_req_get(dev, &dev->tx_idle)))
		mtp_request_free(req, dev->ep_in);
	for (i = 0; i < RX_REQ_MAX; i++)
		mtp_request_free(dev->rx_req[i], dev->ep_out);
	while ((req = mtp_req_get(dev, &dev->intr_idle)))
		mtp_request_free(req, dev->ep_intr);
	dev->state = STATE_OFFLINE;
	//ALPS00428998
	dev->dev_disconnected = 1;
	//ALPS00428998
}

static int mtp_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct mtp_dev	*dev = func_to_mtp(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	DBG(cdev, "mtp_function_set_alt intf: %d alt: %d\n", intf, alt);
    //ALPS00428998
	printk("mtp_function_set_alt intf: %d alt: %d\n", intf, alt);
    //ALPS00428998

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_in);
	if (ret)
		return ret;

	ret = usb_ep_enable(dev->ep_in);
	if (ret)
		return ret;

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_out);
	if (ret)
		return ret;

	ret = usb_ep_enable(dev->ep_out);
	if (ret) {
		usb_ep_disable(dev->ep_in);
		return ret;
	}

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_intr);
	if (ret)
		return ret;

	ret = usb_ep_enable(dev->ep_intr);
	if (ret) {
		usb_ep_disable(dev->ep_out);
		usb_ep_disable(dev->ep_in);
		return ret;
	}
	dev->state = STATE_READY;
	//ALPS00428998
	dev->dev_disconnected = 0;
	//ALPS00428998

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	return 0;
}

static void mtp_function_disable(struct usb_function *f)
{
	struct mtp_dev	*dev = func_to_mtp(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	DBG(cdev, "mtp_function_disable\n");
    //ALPS00428998
	printk("mtp_function_disable\n");	
    //ALPS00428998
	dev->state = STATE_OFFLINE;
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);
	usb_ep_disable(dev->ep_intr);

	//ALPS00428998
	dev->dev_disconnected = 1;
	//ALPS00428998
	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);

	VDBG(cdev, "%s disabled\n", dev->function.name);
}

static int mtp_bind_config(struct usb_configuration *c, bool ptp_config)
{
	struct mtp_dev *dev = _mtp_dev;
	int ret = 0;

	printk(KERN_INFO "mtp_bind_config\n");

	/* allocate a string ID for our interface */
	if (mtp_string_defs[INTERFACE_STRING_INDEX].id == 0) {
		ret = usb_string_id(c->cdev);
		if (ret < 0)
			return ret;
		mtp_string_defs[INTERFACE_STRING_INDEX].id = ret;
		mtp_interface_desc.iInterface = ret;
	}

	dev->cdev = c->cdev;
	dev->function.name = "mtp";
	dev->function.strings = mtp_strings;
	if (ptp_config) {
		dev->function.descriptors = fs_ptp_descs;
		dev->function.hs_descriptors = hs_ptp_descs;
	} else {
		dev->function.descriptors = fs_mtp_descs;
		dev->function.hs_descriptors = hs_mtp_descs;
	}
	dev->function.bind = mtp_function_bind;
	dev->function.unbind = mtp_function_unbind;
	dev->function.set_alt = mtp_function_set_alt;
	dev->function.disable = mtp_function_disable;

	return usb_add_function(c, &dev->function);
}

static int mtp_setup(void)
{
	struct mtp_dev *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->lock);
	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);
	init_waitqueue_head(&dev->intr_wq);
	atomic_set(&dev->open_excl, 0);
	atomic_set(&dev->ioctl_excl, 0);
	INIT_LIST_HEAD(&dev->tx_idle);
	INIT_LIST_HEAD(&dev->intr_idle);

	dev->wq = create_singlethread_workqueue("f_mtp");
	if (!dev->wq) {
		ret = -ENOMEM;
		goto err1;
	}
	INIT_WORK(&dev->send_file_work, send_file_work);
	INIT_WORK(&dev->receive_file_work, receive_file_work);

//Added Modification for ALPS00255822, bug from WHQL test
	INIT_WORK(&dev->device_reset_work, mtp_work);
	dev->fileTransferSend = 0;
//Added Modification for ALPS00255822, bug from WHQL test
//Added Modification for ALPS00354386
	dev->epOut_halt = 0;
//Added Modification for ALPS00354386
//ALPS00428998
	dev->dev_disconnected = 0;
//ALPS00428998

	_mtp_dev = dev;

	ret = misc_register(&mtp_device);
	if (ret)
		goto err2;

	return 0;

err2:
	destroy_workqueue(dev->wq);
err1:
	_mtp_dev = NULL;
	kfree(dev);
	printk(KERN_ERR "mtp gadget driver failed to initialize\n");
	return ret;
}

static void mtp_cleanup(void)
{
	struct mtp_dev *dev = _mtp_dev;

	if (!dev)
		return;

	misc_deregister(&mtp_device);
	destroy_workqueue(dev->wq);
	_mtp_dev = NULL;
	kfree(dev);
}
