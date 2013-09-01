/****************************************************************

 Siano Mobile Silicon, Inc.
 MDTV receiver kernel modules.
 Copyright (C) 2006-2010, Erez Cohen

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ****************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/wait.h>

#include <asm/byteorder.h>

#include "smscoreapi.h"
#include "smsendian.h"
#include "sms-cards.h"
#include "smsir.h"
//#include "cmmb_ming_app.h"   // modified to move firmware form user space to kernel space, yufeng



#include "smsspiphy.h"

#ifdef  logtime                           //xingyu 1215
	struct timespec ts;
	struct rtc_time tm;
#endif

//#include <linux/wrapper.h> /* for mem_map_(un)reserve */ 
#include <asm/io.h> /* for virt_to_phys */ 
#include <linux/slab.h> /* for kmalloc and kfree */ 
#include <linux/mm.h> 

#ifdef AUTO_CREATE_DEV
#include <linux/cdev.h>
extern struct class *SMSDev_class;                          //xingyu 1228
extern int smschar_major;
#define SMSCHAR_NR_DEVS		17
#endif

extern u8* g_fw_buf;               //buf issue

#define MAX_GPIO_PIN_NUMBER	31
static struct smsmdtv_version_t version = {2,2,3};
static int dabCount=0;
char g_LbResBuf[256]={0};
int sms_debug;
module_param_named(debug, sms_debug, int, 0644);
MODULE_PARM_DESC(debug, "set debug level (info=1, adv=2 (or-able))");

static int default_mode = DEVICE_MODE_NONE;
module_param(default_mode, int, 0644);
MODULE_PARM_DESC(default_mode, "default firmware id (device mode)");

struct smscore_device_notifyee_t {
	struct list_head entry;
	hotplug_t hotplug;
};

struct smscore_idlist_t {
	struct list_head entry;
	int id;
	int data_type;
};

struct smscore_client_t {
	struct list_head entry;
	struct smscore_device_t *coredev;
	void *context;
	struct list_head idlist;
	onresponse_t onresponse_handler;
	onremove_t onremove_handler;
};

void smscore_set_board_id(struct smscore_device_t *core, int id)
{
	core->board_id = id;
}

int smscore_get_board_id(struct smscore_device_t *core)
{
	return core->board_id;
}

struct smscore_registry_entry_t {
	struct list_head entry;
	char devpath[32];
	int mode;
	enum sms_device_type_st type;
};

static struct list_head g_smscore_notifyees;
static struct list_head g_smscore_devices;
static struct mutex g_smscore_deviceslock;

static struct list_head g_smscore_registry;
static struct mutex g_smscore_registrylock;

static struct smscore_registry_entry_t *smscore_find_registry(char *devpath)
{
	struct smscore_registry_entry_t *entry;
	struct list_head *next;

	kmutex_lock(&g_smscore_registrylock);

	for (next = g_smscore_registry.next; next != &g_smscore_registry; next
			= next->next) {
		entry = (struct smscore_registry_entry_t *) next;
		if (!strcmp(entry->devpath, devpath)) {
			kmutex_unlock(&g_smscore_registrylock);
			return entry;
		}
	}
	entry = /* (struct smscore_registry_entry_t *) */kmalloc(
			sizeof(struct smscore_registry_entry_t), GFP_KERNEL);
	if (entry) {
		entry->mode = default_mode;
		strcpy(entry->devpath, devpath);
		list_add(&entry->entry, &g_smscore_registry);
	} else
		sms_err("failed to create smscore_registry.");

	kmutex_unlock(&g_smscore_registrylock);
	return entry;
}

int smscore_registry_getmode(char *devpath)
{
	struct smscore_registry_entry_t *entry;

	sms_info("devpath 0x%x", (unsigned int)devpath);
	entry = smscore_find_registry(devpath);
	if (entry)
		return entry->mode;
	else
		sms_err("No registry found.");

	return default_mode;
}

static enum sms_device_type_st smscore_registry_gettype(char *devpath)
{
	struct smscore_registry_entry_t *entry;

	entry = smscore_find_registry(devpath);
	if (entry)
		return entry->type;
	else
		sms_err("No registry found.");

	return SMS_UNKNOWN_TYPE;
}

void smscore_registry_setmode(char *devpath, int mode)
{
	struct smscore_registry_entry_t *entry;

	entry = smscore_find_registry(devpath);
	if (entry)
		entry->mode = mode;
	else
		sms_err("No registry found.");
}

int smscore_reset_device_drvs(struct smscore_device_t *coredev)

{
	int rc = 0;
    	sms_debug("smscore_reset_device_drvs");
	coredev->mode = DEVICE_MODE_NONE;
	coredev->device_flags = SMS_DEVICE_FAMILY2 | SMS_DEVICE_NOT_READY | SMS_ROM_NO_RESPONSE;
	coredev->modes_supported  = 0;	
   return rc;
}

static void smscore_registry_settype(char *devpath,
		enum sms_device_type_st type) {
	struct smscore_registry_entry_t *entry;

	entry = smscore_find_registry(devpath);
	if (entry)
		entry->type = type;
	else
		sms_err("No registry found.");
}

static void list_add_locked(struct list_head *new, struct list_head *head,
		spinlock_t *lock) {
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	list_add(new, head);
	spin_unlock_irqrestore(lock, flags);
}

/**
 * register a client callback that called when device plugged in/unplugged
 * NOTE: if devices exist callback is called immediately for each device
 *
 * @param hotplug callback
 *
 * @return 0 on success, <0 on error.
 */
int smscore_register_hotplug(hotplug_t hotplug)
{
	struct smscore_device_notifyee_t *notifyee;
	struct list_head *next, *first;
	int rc = 0;

	sms_info("enter");
	kmutex_lock(&g_smscore_deviceslock);

	notifyee = kmalloc(sizeof(struct smscore_device_notifyee_t),
		GFP_KERNEL);
	if (notifyee) {
		sms_info("notifyee");
		/* now notify callback about existing devices */
		first = &g_smscore_devices;
		for (next = first->next; next != first && !rc;
			next = next->next) {
			struct smscore_device_t *coredev =
				(struct smscore_device_t *) next;
			sms_info("call hotplug");
			rc = hotplug(coredev, coredev->device, 1);
		}

		if (rc >= 0) {
			notifyee->hotplug = hotplug;
			list_add(&notifyee->entry, &g_smscore_notifyees);
		} else
			kfree(notifyee);
	} else
		rc = -ENOMEM;

	kmutex_unlock(&g_smscore_deviceslock);

	return rc;
}

/**
 * unregister a client callback that called when device plugged in/unplugged
 *
 * @param hotplug callback
 *
 */
void smscore_unregister_hotplug(hotplug_t hotplug)
{
	struct list_head *next, *first;
#ifdef AUTO_CREATE_DEV
	int i,devno;
#endif //AUTO_CREATE_DEV
	kmutex_lock(&g_smscore_deviceslock);

	first = &g_smscore_notifyees;

	for (next = first->next; next != first;) {
		struct smscore_device_notifyee_t *notifyee =
				(struct smscore_device_notifyee_t *) next;
		next = next->next;

		if (notifyee->hotplug == hotplug) {
#ifdef AUTO_CREATE_DEV
			sms_info("device_destroy");
			for (i = 0; i < SMSCHAR_NR_DEVS; i++) {
				devno = MKDEV(smschar_major, i);
				device_destroy(SMSDev_class, devno);
			}
#endif //AUTO_CREATE_DEV
			list_del(&notifyee->entry);
			kfree(notifyee);
		}
	}

	kmutex_unlock(&g_smscore_deviceslock);
}

static void smscore_notify_clients(struct smscore_device_t *coredev)
{
	struct smscore_client_t *client;

	/* the client must call smscore_unregister_client from remove handler */
	while (!list_empty(&coredev->clients)) {
		client = (struct smscore_client_t *) coredev->clients.next;
		client->onremove_handler(client->context);
	}
}

static int smscore_notify_callbacks(struct smscore_device_t *coredev,
		struct device *device, int arrival) {
	struct list_head *next, *first;
	int rc = 0;

	/* note: must be called under g_deviceslock */

	first = &g_smscore_notifyees;

	for (next = first->next; next != first; next = next->next) {
		rc = ((struct smscore_device_notifyee_t *) next)->
			 hotplug(coredev, device, arrival);
		if (rc < 0)
			break;
	}

	return rc;
}

static struct smscore_buffer_t *smscore_createbuffer(u8 *buffer,
		void *common_buffer, dma_addr_t common_buffer_phys) {
	struct smscore_buffer_t *cb = kmalloc(sizeof(struct smscore_buffer_t),
			GFP_KERNEL);
	if (!cb) {
		sms_info("kmalloc(...) failed");
		return NULL;
	}

	cb->p = buffer;
	cb->offset_in_common = buffer - (u8 *) common_buffer;
	cb->phys = common_buffer_phys + cb->offset_in_common;

	return cb;
}

/**
 * creates coredev object for a device, prepares buffers,
 * creates buffer mappings, notifies registered hotplugs about new device.
 *
 * @param params device pointer to struct with device specific parameters
 *               and handlers
 * @param coredev pointer to a value that receives created coredev object
 *
 * @return 0 on success, <0 on error.
 */
int smscore_register_device(struct smsdevice_params_t *params,
		struct smscore_device_t **coredev) {
	struct smscore_device_t *dev;
	u8 *buffer;

	dev = kzalloc(sizeof(struct smscore_device_t), GFP_KERNEL);
	if (!dev) {
		sms_info("kzalloc(...) failed");
		return -ENOMEM;
	}

	/* init list entry so it could be safe in smscore_unregister_device */
	INIT_LIST_HEAD(&dev->entry);

	/* init queues */
	INIT_LIST_HEAD(&dev->clients);
	INIT_LIST_HEAD(&dev->buffers);

	/* init locks */
	spin_lock_init(&dev->clientslock);
	spin_lock_init(&dev->bufferslock);

	/* Device protocol completion events */
	init_completion(&dev->version_ex_done);
	init_completion(&dev->data_download_done);
	init_completion(&dev->trigger_done);
	init_completion(&dev->clock_done);                           //clock msg
	init_completion(&dev->init_device_done);
	init_completion(&dev->reload_start_done);
	init_completion(&dev->resume_done);
	init_completion(&dev->gpio_configuration_done);
	init_completion(&dev->gpio_set_level_done);
	init_completion(&dev->gpio_get_level_done);
	init_completion(&dev->ir_init_done);
	init_completion(&dev->device_ready_done);
#if 1 //JACKZ
	init_completion(&dev->periodic_stat_done);
	init_completion(&dev->scan_done);
	init_completion(&dev->scan_complete);
	init_completion(&dev->start_service_done);
	init_completion(&dev->dab_count_done);
	init_completion(&dev->loopback_res_done);
#endif
	/* Buffer management */
	init_waitqueue_head(&dev->buffer_mng_waitq);

	/* alloc common buffer */
	dev->common_buffer_size = params->buffer_size * params->num_buffers;
#if 0                       //buf issue	
	dev->common_buffer = dma_alloc_coherent(NULL, dev->common_buffer_size,
			&dev->common_buffer_phys, GFP_KERNEL | GFP_DMA);
#else
	dev->common_buffer = kmalloc( dev->common_buffer_size,GFP_KERNEL );
       dev->common_buffer_phys = (u32)dev->common_buffer;
	/* alloc fw buffer */
	g_fw_buf = kmalloc(ALIGN(_FW_SIZE, SMS_ALLOC_ALIGNMENT), GFP_KERNEL| GFP_DMA);
	if(!g_fw_buf)
	{
	    sms_err("kmalloc g_fw_buf fail");
		smscore_unregister_device(dev);
		return -ENOMEM;
	}
#endif
	if (!dev->common_buffer) {
		smscore_unregister_device(dev);
		return -ENOMEM;
	}

	/* prepare dma buffers */
	for (buffer = dev->common_buffer; dev->num_buffers <
			params->num_buffers; dev->num_buffers++, buffer
			+= params->buffer_size) {
		struct smscore_buffer_t *cb = smscore_createbuffer(buffer,
				dev->common_buffer, dev->common_buffer_phys);
		if (!cb) {
			smscore_unregister_device(dev);
			return -ENOMEM;
		}

		smscore_putbuffer(dev, cb);
	}

	sms_info("allocated %d buffers", dev->num_buffers);

	dev->mode = DEVICE_MODE_NONE;
	dev->board_id = SMS1XXX_BOARD_SIANO_MING;	//SMS_BOARD_UNKNOWN;

	dev->context = params->context;
	dev->device = params->device;
	dev->setmode_handler = params->setmode_handler;
	dev->detectmode_handler = params->detectmode_handler;
	dev->sendrequest_handler = params->sendrequest_handler;
	dev->preload_handler = params->preload_handler;
	dev->postload_handler = params->postload_handler;

	dev->device_flags = params->flags;
	strcpy(dev->devpath, params->devpath);

	smscore_registry_settype(dev->devpath, params->device_type);

	/* add device to devices list */
	kmutex_lock(&g_smscore_deviceslock);
	sms_info("list add: g_smscore_devices");
	list_add(&dev->entry, &g_smscore_devices);
	kmutex_unlock(&g_smscore_deviceslock);

	*coredev = dev;

	sms_info("core device 0x%p created, mode %d, board id %d, devpath %s", dev, dev->mode, dev->board_id, dev->devpath);

	return 0;
}

static int smscore_sendrequest_and_wait(struct smscore_device_t *coredev,
		void *buffer, size_t size, struct completion *completion) {
	int rc;

	if (completion == NULL)
		return -EINVAL;
	init_completion(completion);

	rc = coredev->sendrequest_handler(coredev->context, buffer, size);
	if (rc < 0) {
		sms_info("sendrequest returned error %d", rc);
		return rc;
	}

	return wait_for_completion_timeout(completion,
			msecs_to_jiffies(SMS_PROTOCOL_MAX_RAOUNDTRIP_MS)) ? 0 : -ETIME;
}

/**
 * starts & enables IR operations
 *
 * @return 0 on success, < 0 on error.
 */
static int smscore_init_ir(struct smscore_device_t *coredev)
{
	int ir_io;
	int rc;
	void *buffer;

	coredev->ir.input_dev = NULL;
	ir_io = sms_get_board(smscore_get_board_id(coredev))->board_cfg.ir;
	if (ir_io) {/* only if IR port exist we use IR sub-module */
		sms_info("IR loading");
		rc = sms_ir_init(coredev);

		if	(rc != 0)
			{
			sms_err("Error initialization DTV IR sub-module");
		}
		else {
			buffer = kmalloc(sizeof(struct SmsMsgData_ST2) +
						SMS_DMA_ALIGNMENT,
						GFP_KERNEL | GFP_DMA);
			if (buffer) {
				struct SmsMsgData_ST2 *msg =
				(struct SmsMsgData_ST2 *)
				SMS_ALIGN_ADDRESS(buffer);

				SMS_INIT_MSG(&msg->xMsgHeader,
						MSG_SMS_START_IR_REQ,
						sizeof(struct SmsMsgData_ST2));
				msg->msgData[0] = coredev->ir.controller;
				msg->msgData[1] = coredev->ir.timeout;

				smsendian_handle_tx_message(
					(struct SmsMsgHdr_ST2 *)msg);
				rc = smscore_sendrequest_and_wait(coredev, msg,
						msg->xMsgHeader. msgLength,
						&coredev->ir_init_done);

				kfree(buffer);
			} else
				sms_err
				("Sending IR initialization message failed");
		}
	} else
		sms_info("IR port has not been detected");

	return 0;
}

/**
 * sets initial device mode and notifies client 
 * hotplugs that device is ready
 *
 * @param coredev pointer to a coredev object returned by
 * 		  smscore_register_device
 *
 * @return 0 on success, <0 on error.
 */
int smscore_start_device(struct smscore_device_t *coredev)
{
	int rc = 0;
	
#ifdef REQUEST_FIRMWARE_SUPPORTED	
	int board_id = smscore_get_board_id(coredev);
	int mode;
	int type = sms_get_board(board_id)->type;

	sms_info("board id %d, type %d", board_id, type);
	/*
	 * first, search operation mode in the registry, with 
	 * the limitation of type-mode compatability.
	 * if firmware donload fails, get operation mode from
	 * sms_boards
	 * for spi, type = SMS_UNKNOWN_TYPE and board_id = SMS_BOARD_UNKNOWN
	 * so always default_mode is set
	 */
	switch (type) {
		case SMS_UNKNOWN_TYPE: 
			mode = default_mode;
			break;
		case SMS_STELLAR:
		case SMS_NOVA_A0:
		case SMS_NOVA_B0:
		case SMS_PELE:
        case SMS_RIO:
			mode = smscore_registry_getmode(coredev->devpath);
			if (mode == DEVICE_MODE_CMMB)
				mode = (default_mode == DEVICE_MODE_CMMB) ? DEVICE_MODE_NONE : default_mode;
			break;	
        case SMS_DENVER_1530:
            mode = DEVICE_MODE_ATSC;
            break;
        case SMS_DENVER_2160:
            mode = DEVICE_MODE_DAB_TDMB;
			break;
		case SMS_VEGA:
		case SMS_VENICE:
		case SMS_MING:
			mode = DEVICE_MODE_CMMB;
			break;
		default:
			mode = DEVICE_MODE_NONE;	
	}

	/* first try */
	rc = smscore_set_device_mode(coredev, mode);	

	if (rc < 0) {
		sms_info("set device mode to %d failed", mode);

		/* 
		 * don't try again on spi mode, or if the mode from 
		 * sms_boards is identical to the previous mode
		 */
		if ((board_id == SMS_BOARD_UNKNOWN) ||
		    (mode == sms_get_board(board_id)->default_mode))
			return -ENOEXEC;

		/* second try */
		mode = sms_get_board(board_id)->default_mode;
		rc = smscore_set_device_mode(coredev, mode);	
		if (rc < 0) {
			sms_info("set device mode to %d failed", mode);
			return -ENOEXEC ;
		}
	}
	sms_info("set device mode succeeded");
	
	rc = smscore_configure_board(coredev);
	if (rc < 0) {
		sms_info("configure board failed , rc %d", rc);
		return rc;
	}
#endif

	kmutex_lock(&g_smscore_deviceslock);

	rc = smscore_notify_callbacks(coredev, coredev->device, 1);
	smscore_init_ir(coredev);

	sms_info("device 0x%p started, rc %d", coredev, rc);

	kmutex_unlock(&g_smscore_deviceslock);

	return rc;
}

/**
 * injects firmware from a buffer to the device using data messages
 * 
 * @param coredev pointer to a coredev object returned by
 * 		  smscore_register_device
 * @param buffer pointer to a firmware buffer
 * @param size size (in bytes) of the firmware buffer
 * @return 0 on success, <0 on error.
 */
static int smscore_load_firmware_family2(struct smscore_device_t *coredev,
		void *buffer, size_t size) {
	struct SmsFirmware_ST *firmware = (struct SmsFirmware_ST *) buffer;
	struct SmsMsgHdr_ST *msg;
	u32 mem_address;
	u8 *payload = firmware->Payload;
	int rc = 0;

	firmware->StartAddress = le32_to_cpu(firmware->StartAddress);
	firmware->Length = le32_to_cpu(firmware->Length);

	mem_address = firmware->StartAddress;

	sms_info("loading fw to addr 0x%x size %d",
			mem_address, firmware->Length);

	if (coredev->preload_handler) {
		rc = coredev->preload_handler(coredev->context);                //xingyu 1201 call preload  to send start code
		if (rc < 0)
			{	sms_info("preload error");
		              return rc;
			}
	}

	/* PAGE_SIZE buffer shall be enough and dma aligned */
	msg = kmalloc(PAGE_SIZE, GFP_KERNEL | GFP_DMA);
	if (!msg){
		sms_info("kmalloc error");
		return -ENOMEM;
		}

	if (coredev->mode != DEVICE_MODE_NONE) {
		sms_debug("sending reload command");
		SMS_INIT_MSG(msg, MSG_SW_RELOAD_START_REQ,
				sizeof(struct SmsMsgHdr_ST));
		smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
		rc = smscore_sendrequest_and_wait(coredev, msg, msg->msgLength,
				&coredev->reload_start_done);

		if (rc < 0) {				
			sms_err("device reload failed, rc %d", rc);
			goto exit_fw_download;
		}

		mem_address = *(u32 *) &payload[20];
	}

	while (size && rc >= 0) {
		struct SmsDataDownload_ST *DataMsg =
				(struct SmsDataDownload_ST *) msg;
		int payload_size = min((int)size, SMS_MAX_PAYLOAD_SIZE);

		SMS_INIT_MSG(msg, MSG_SMS_DATA_DOWNLOAD_REQ,
				(u16) (sizeof(struct SmsMsgHdr_ST) +
						sizeof(u32) + payload_size));

		DataMsg->MemAddr = mem_address;
		memcpy(DataMsg->Payload, payload, payload_size);

		smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
		rc = smscore_sendrequest_and_wait(coredev, DataMsg,
			DataMsg->xMsgHeader. msgLength,
			&coredev->data_download_done);

		payload += payload_size;
		size -= payload_size;
		mem_address += payload_size;
	}

	if (rc < 0) 		
		goto exit_fw_download;

	if (coredev->mode == DEVICE_MODE_NONE) {
		struct SmsMsgData_ST *TriggerMsg =
				(struct SmsMsgData_ST *) msg;

		sms_debug("sending MSG_SMS_SWDOWNLOAD_TRIGGER_REQ");
		SMS_INIT_MSG(msg, MSG_SMS_SWDOWNLOAD_TRIGGER_REQ,
				sizeof(struct SmsMsgHdr_ST) +
				sizeof(u32) * 5);

		TriggerMsg->msgData[0] = firmware->StartAddress;
		/* Entry point */
		TriggerMsg->msgData[1] = 6; /* Priority */
		TriggerMsg->msgData[2] = 0x200; /* Stack size */
		TriggerMsg->msgData[3] = 0; /* Parameter */
		TriggerMsg->msgData[4] = 4; /* Task ID */

		smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
		rc = smscore_sendrequest_and_wait(coredev,
			TriggerMsg,
			TriggerMsg->xMsgHeader.msgLength,
			&coredev->trigger_done);
	} else {
		SMS_INIT_MSG(msg, MSG_SW_RELOAD_EXEC_REQ,
				sizeof(struct SmsMsgHdr_ST));
		smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
		rc = coredev->sendrequest_handler(coredev->context, msg,
				msg->msgLength);
	}

	if (rc < 0)
		goto exit_fw_download;
			
	/*
	 * backward compatibility - wait to device_ready_done for
	 * not more than 400 ms
	 */
	wait_for_completion_timeout(&coredev->device_ready_done,
			msecs_to_jiffies(400));		

exit_fw_download:
	sms_debug("rc=%d, postload=0x%p ", rc, coredev->postload_handler);

	kfree(msg);

	return ((rc >= 0) && coredev->postload_handler) ?
			coredev->postload_handler(coredev->context) : rc;
}

/**
 * loads specified firmware into a buffer and calls device loadfirmware_handler
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 * @param mode requested mode of operation
 * @param lookup if 1, always get the fw filename from smscore_fw_lkup 
 * 	  table. if 0, try first to get from sms_boards
 * @param loadfirmware_handler device handler that loads firmware
 *
 * @return 0 on success, <0 on error.
 */
static int smscore_load_firmware_from_file(struct smscore_device_t *coredev,
		int mode, int lookup, loadfirmware_t loadfirmware_handler) {
	int rc = -ENOENT;
	u8 *fw_buf;
	u32 fw_buf_size;

#ifdef REQUEST_FIRMWARE_SUPPORTED
	#if 0
	struct firmware *fw;

	char* fw_filename = smscore_get_fw_filename(coredev, mode, lookup);
	if (!strcmp(fw_filename,"none"))
		return -ENOENT;

	if (loadfirmware_handler == NULL && !(coredev->device_flags
			& SMS_DEVICE_FAMILY2))
		return -EINVAL;

/*
	rc = request_firmware(&fw, fw_filename, coredev->device);
	if (rc < 0) {
		sms_info("failed to open \"%s\"", fw_filename);
		return rc;
	}
*/

	sms_info("read fw %s, buffer size=0x%x", fw_filename, fw->size);
	fw_buf = kmalloc(ALIGN(fw->size, SMS_ALLOC_ALIGNMENT),
				GFP_KERNEL | GFP_DMA);
	if (!fw_buf) {
		sms_info("failed to allocate firmware buffer");
		return -ENOMEM;
	}
	memcpy(fw_buf, fw->data, fw->size);
	fw_buf_size = fw->size;
	#else
	// modified to move firmware form user space to kernel space, yufeng
	fw_buf = coredev->fw_buf;;
	fw_buf_size = coredev->fw_buf_size;
	#endif

#else
	if (!coredev->fw_buf) {
		sms_info("missing fw file buffer");
		return -EINVAL;
	}
	fw_buf = coredev->fw_buf;
	fw_buf_size = coredev->fw_buf_size;
#endif

	sms_info("flag 0x%x", (unsigned int)coredev->device_flags);
	rc = (coredev->device_flags & SMS_DEVICE_FAMILY2) ?
		smscore_load_firmware_family2(coredev, fw_buf, fw_buf_size)
		: /* loadfirmware_handler(coredev->context, fw_buf,
		fw_buf_size); */ printk("error - should not be here");

	//kfree(fw_buf);

#ifdef REQUEST_FIRMWARE_SUPPORTED
	//release_firmware(fw);
#else
	coredev->fw_buf = NULL;
	coredev->fw_buf_size = 0;
#endif
	return rc;
}

/**
 * notifies all clients registered with the device, notifies hotplugs,
 * frees all buffers and coredev object
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 *
 * @return 0 on success, <0 on error.
 */
void smscore_unregister_device(struct smscore_device_t *coredev)
{
	struct smscore_buffer_t *cb;
	int num_buffers = 0;
	int retry = 0;

	kmutex_lock(&g_smscore_deviceslock);

	/* Release input device (IR) resources */
	sms_ir_exit(coredev);

	smscore_notify_clients(coredev);
	smscore_notify_callbacks(coredev, NULL, 0);

	/* at this point all buffers should be back
	 * onresponse must no longer be called */

	while (1) {
		while(!list_empty(&coredev->buffers))
		{
			cb = (struct smscore_buffer_t *) coredev->buffers.next;
			list_del(&cb->entry);
			kfree(cb);
			num_buffers++;
		}
		if (num_buffers == coredev->num_buffers)
			break;

		if (++retry > 10) {
			sms_info("exiting although "
					"not all buffers released.");
			break;
		}

		sms_info("waiting for %d buffer(s)",
				coredev->num_buffers - num_buffers);
		msleep(100);
	}

	sms_info("freed %d buffers", num_buffers);

	if (coredev->common_buffer)
#if 0               //buf issue		
		dma_free_coherent(NULL, coredev->common_buffer_size,
			coredev->common_buffer, coredev->common_buffer_phys);
#else
		kfree(coredev->common_buffer);
              kfree(g_fw_buf);
	       g_fw_buf=NULL;
#endif

	if (coredev->fw_buf != NULL)
		{
#if 0           //buf issue		
		kfree(coredev->fw_buf);
#else
              coredev->fw_buf=NULL;
#endif
		}

	list_del(&coredev->entry);
	kfree(coredev);

	kmutex_unlock(&g_smscore_deviceslock);

	sms_info("device 0x%p destroyed", coredev);
}

static int smscore_detect_mode(struct smscore_device_t *coredev)
{
	void *buffer = kmalloc(sizeof(struct SmsMsgHdr_ST) + SMS_DMA_ALIGNMENT,
			GFP_KERNEL | GFP_DMA);
	struct SmsMsgHdr_ST *msg =
			(struct SmsMsgHdr_ST *) SMS_ALIGN_ADDRESS(buffer);
	int rc;

	if (!buffer)
		return -ENOMEM;

	SMS_INIT_MSG(msg, MSG_SMS_GET_VERSION_EX_REQ,
			sizeof(struct SmsMsgHdr_ST));

	smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
	rc = smscore_sendrequest_and_wait(coredev, msg, msg->msgLength,
			&coredev->version_ex_done);

	if (rc < 0) {
		sms_err("detect mode failed, rc %d", rc);
	}

	if (rc == -ETIME) {
		sms_err("MSG_SMS_GET_VERSION_EX_REQ failed first try");

		if (wait_for_completion_timeout(&coredev->resume_done,
				msecs_to_jiffies(5000))) {
			rc = smscore_sendrequest_and_wait(coredev, msg,
				msg->msgLength, &coredev->version_ex_done);
			if (rc < 0)
				sms_err("MSG_SMS_GET_VERSION_EX_REQ failed "
						"second try, rc %d", rc);
		} else
			rc = -ETIME;
	}

	kfree(buffer);

	return rc;
}

static char *smscore_fw_lkup[][SMS_NUM_OF_DEVICE_TYPES] = {
/*Stellar, NOVA A0, Nova B0, VEGA, VENICE, MING, PELE, RIO, DENVER_1530, DENVER_2160*/
/*DVBT*/
{ "none", "dvb_nova_12mhz.inp", "dvb_nova_12mhz_b0.inp", "none", "none", "none", "none", "dvb_rio.inp", "none", "none" },
/*DVBH*/
{ "none", "dvb_nova_12mhz.inp", "dvb_nova_12mhz_b0.inp", "none", "none", "none", "none", "dvb_rio.inp", "none", "none" },
/*TDMB*/
{ "none", "tdmb_nova_12mhz.inp", "tdmb_nova_12mhz_b0.inp", "none", "none", "none", "none", "none", "none", "tdmb_denver.inp" },
/*DABIP*/
{ "none", "none", "none", "none", "none", "none", "none", "none", "none", "none" },
/*DVBT_BDA*/
{ "none", "dvb_nova_12mhz.inp", "dvb_nova_12mhz_b0.inp", "none", "none", "none", "none", "dvb_rio.inp", "none", "none" },
/*ISDBT*/
{ "none", "isdbt_nova_12mhz.inp", "isdbt_nova_12mhz_b0.inp", "none", "none", "none", "isdbt_pele.inp", "isdbt_rio.inp", "none", "none" },
/*ISDBT_BDA*/
{ "none", "isdbt_nova_12mhz.inp", "isdbt_nova_12mhz_b0.inp", "none", "none", "none", "isdbt_pele.inp", "isdbt_rio.inp", "none", "none" },
/*CMMB*/
{ "none", "none", "none", "cmmb_vega_12mhz.inp", "cmmb_venice_12mhz.inp", "cmmb_ming_app.inp", "none", "none", "none", "none" },
/*RAW - not supported*/
{ "none", "none", "none", "none", "none", "none", "none", "none", "none", "none" },
/*FM*/
{ "none", "none", "fm_radio.inp", "none", "none", "none", "none", "fm_radio.inp", "none", "none" },
/*FM_BDA*/
{ "none", "none", "fm_radio.inp", "none", "none", "none", "none", "fm_radio.inp", "none", "none" },
/*ATSC*/
{ "none", "none", "none", "none", "none", "none", "none", "none", "atsc_denver.inp", "none" }
};

/**
 * get firmware file name from one of the two mechanisms : sms_boards or 
 * smscore_fw_lkup.

 * @param coredev pointer to a coredev object returned by
 * 		  smscore_register_device
 * @param mode requested mode of operation
 * @param lookup if 1, always get the fw filename from smscore_fw_lkup 
 * 	 table. if 0, try first to get from sms_boards
 *
 * @return 0 on success, <0 on error.
 */
char *smscore_get_fw_filename(struct smscore_device_t *coredev, int mode, int lookup) {
	char **fw;
	int board_id = smscore_get_board_id(coredev);
	enum sms_device_type_st type = smscore_registry_gettype(coredev->devpath); 

	if ( (board_id == SMS_BOARD_UNKNOWN) || 
	     (lookup == 1) ) {
		sms_debug("trying to get fw name from lookup table mode %d type %d", mode, type);
		return smscore_fw_lkup[mode][type];
	}

	sms_debug("trying to get fw name from sms_boards board_id %d mode %d", board_id, mode);
	fw = sms_get_board(board_id)->fw;
	if (fw == NULL) {
		sms_debug("cannot find fw name in sms_boards, getting from lookup table mode %d type %d", mode, type);
		return smscore_fw_lkup[mode][type];
	}

	if (fw[mode] == NULL) {
		sms_debug("cannot find fw name in sms_boards, getting from lookup table mode %d type %d", mode, type);
		return smscore_fw_lkup[mode][type];
	}

	return fw[mode];
}

/**
 * send init device request and wait for response
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 * @param mode requested mode of operation
 *
 * @return 0 on success, <0 on error.
 */
int smscore_init_device(struct smscore_device_t *coredev, int mode)
{
	void* buffer;
	struct SmsMsgData_ST *msg;
	int rc = 0;

	buffer = kmalloc(sizeof(struct SmsMsgData_ST) +
			SMS_DMA_ALIGNMENT, GFP_KERNEL | GFP_DMA);
	if (!buffer) {
		sms_err("Could not allocate buffer for "
				"init device message.");
		return -ENOMEM;
	}

	msg = (struct SmsMsgData_ST *)SMS_ALIGN_ADDRESS(buffer);

	SMS_INIT_MSG(&msg->xMsgHeader, MSG_SMS_INIT_DEVICE_REQ,
			sizeof(struct SmsMsgData_ST));
	msg->msgData[0] = mode;

	smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
	rc = smscore_sendrequest_and_wait(coredev, msg,
			msg->xMsgHeader. msgLength,
			&coredev->init_device_done);

	kfree(buffer);
	return rc;
}


int  AdrLoopbackTest( struct smscore_device_t *coredev )
{
	char msgbuff[252];
	struct SmsMsgData_ST* pLoopbackMsg = (struct SmsMsgData_ST*)msgbuff;
	struct SmsMsgData_ST* pLoopbackRes = (struct SmsMsgData_ST*)g_LbResBuf;
	int i , j;
	int g_Loopback_failCounters= 0; 
//jackz	int Len = 252 - sizeof(struct SmsMsgData_ST);
  int Len = 240;
	char* pPtr;
	int rc =0;

	pLoopbackMsg->xMsgHeader.msgType = MSG_SMS_LOOPBACK_REQ;
	pLoopbackMsg->xMsgHeader.msgSrcId = 151;
	pLoopbackMsg->xMsgHeader.msgDstId = 11;
	pLoopbackMsg->xMsgHeader.msgFlags = 0;
	pLoopbackMsg->xMsgHeader.msgLength = 252;
	
	sms_info("Loobpack test start.");
	

	for ( i = 0 ; i < 10000 ; i++ )
	{
	    printk("Loopback test No.%d\n", i);
		pPtr = (u8*) &pLoopbackMsg->msgData[1];
		for ( j = 0 ; j < Len ; j ++ )
		{
			pPtr[j] = i+j;
			//pPtr[j] = 0xFF;
		}
		pLoopbackMsg->msgData[0] = i+1;
	
		smsendian_handle_tx_message((struct SmsMsgHdr_ST *)pLoopbackMsg);
			rc = smscore_sendrequest_and_wait(coredev, pLoopbackMsg,
					pLoopbackMsg->xMsgHeader.msgLength,
					&coredev->loopback_res_done);


		if (rc)
			return  rc; 

	
		pPtr = (u8*) &pLoopbackRes->msgData[1];

		for ( j = 0 ; j < Len ; j ++ )
		{
			if ( pPtr[j] != (u8)(j + i))
			{
					sms_err("Loopback data error at byte %u. Exp %u, Got %u", j, (u8)(j+i), pPtr[j] );
					//printk("Loopback data error at byte %u. Exp %u, Got %u", j, (u8)(j+i), pPtr[j] );
					g_Loopback_failCounters++;
					break;
			}
		} //for ( j = 0 ; j < Len ; j ++ )
	} //for ( i = 0 ; i < 100 ; i++ )
	sms_info( "Loobpack test end. RUN  times: %d; fail times : %d", i, g_Loopback_failCounters);
        return rc ;
}


/**
 * calls device handler to change mode of operation
 * NOTE: stellar/usb may disconnect when changing mode
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 * @param mode requested mode of operation
 *
 * @return 0 on success, <0 on error.
 */
int smscore_set_device_mode(struct smscore_device_t *coredev, int mode)
{
	int rc = 0;
	//void* buffer;  for test

	sms_debug("set device mode to %d", mode);
	if (coredev->device_flags & SMS_DEVICE_FAMILY2) {
		if (mode < DEVICE_MODE_DVBT || mode >= DEVICE_MODE_RAW_TUNER) {
			sms_err("invalid mode specified %d", mode);
			return -EINVAL;
		}

		smscore_registry_setmode(coredev->devpath, mode);

		if (!(coredev->device_flags & SMS_DEVICE_NOT_READY)) {
			rc = smscore_detect_mode(coredev);
			if (rc < 0) {
				sms_err("mode detect failed %d", rc);
				return rc;
			}
		}

		if (coredev->mode == mode) {
			sms_info("device mode %d already set", mode);
			return 0;
		}

#ifdef LOOPBACK                                          //KO DEBUG VERSION
		if (coredev->preload_handler) {
			rc = coredev->preload_handler(coredev->context);                //xingyu 1201 call preload  to send start code
			if (rc < 0)
			{	sms_info("preload error");
				return rc;
			}
		}
		sms_info("Start Loopback test after scan\n");

		sms_info("############# PREPARE TO MEASURE #############");
		//  msleep(20000);
		rc = AdrLoopbackTest(coredev);
		if(rc<0)
		{
			sms_err("smscore_set_device_mode, failed in loopback test, rc=%d\n", rc);
		}
                return 0;
#endif

		if (!(coredev->modes_supported & (1 << mode))) {
			rc = smscore_load_firmware_from_file(coredev, mode, 0, NULL);

			/* 
			* try again with the default firmware -
			* get the fw filename from look-up table
			*/
			if (rc < 0) {
				sms_debug("error %d loading firmware, "
					"trying again with default firmware", rc);
				rc = smscore_load_firmware_from_file(coredev, mode, 1, NULL);
			}

			if (rc < 0) {
				sms_debug("error %d loading firmware", rc);
				return rc;
			}

			sms_info("firmware download success");
		} else {
			sms_info("mode %d is already supported by running "
					"firmware", mode);
		}

		sms_info("start to init device");
		smscore_init_device(coredev, mode);
		if (rc < 0) {
			sms_err("device init failed, rc %d.", rc);
		}
#if 0                 //using clock to 26Mhz,  default 12M, need not send this cmd  msg clock
		sms_info("send cmd to support 26M hz");
		struct SmsMsgData_ST ClockMsg;

		SMS_INIT_MSG(ClockMsg, MSG_SMS_NEW_CRYSTAL_REQ,
				sizeof(struct SmsMsgHdr_ST) +
				sizeof(u32) * 1);

		ClockMsg->msgData[0] = 2600;        // 2600 for 26M

		smsendian_handle_tx_message((struct SmsMsgHdr_ST *)ClockMsg);
		rc = smscore_sendrequest_and_wait(coredev,
			ClockMsg,
			ClockMsg->xMsgHeader.msgLength,
			&coredev->clock_done);

#endif
#if 0 //JACKZ, FOR TESTING
        buffer = kmalloc(sizeof(struct SmsMsgData_ST) +
				SMS_DMA_ALIGNMENT, GFP_KERNEL | GFP_DMA);
	if(buffer)
	{
		struct SmsMsgData_ST *msg = (struct SmsMsgData_ST *)SMS_ALIGN_ADDRESS(buffer);

        sms_info("Start to get version\n");
         SMS_INIT_MSG(&msg->xMsgHeader, MSG_SMS_GET_VERSION_EX_REQ,
			sizeof(struct SmsMsgHdr_ST));
		    msg->msgData[0] = 0;

            smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
	    rc = smscore_sendrequest_and_wait(coredev, msg,
					msg->xMsgHeader.msgLength,
					&coredev->version_ex_done);
        if(rc<0)
        {
          sms_err("smscore_set_device_mode, failed in get version\n");
        }

#if 0                                                                           //xingyu 1213
      sms_info("Set Periodic Stat before scan\n");
	    SMS_INIT_MSG(&msg->xMsgHeader, MSG_SMS_SET_PERIODIC_STATISTICS_REQ,
					sizeof(struct SmsMsgHdr_ST) + 1*sizeof(u32));
	    msg->msgData[0] = 1;

	    smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
	    rc = smscore_sendrequest_and_wait(coredev, msg,
					msg->xMsgHeader.msgLength,
					&coredev->periodic_stat_done);
        if(rc<0)
        {
          sms_err("smscore_set_device_mode, failed in set periodic stat, rc=%d\n", rc);
        }
#endif

      sms_info("Start to Scan\n");
	    SMS_INIT_MSG(&msg->xMsgHeader, MSG_SMS_SCAN_START_REQ,
					sizeof(struct SmsMsgHdr_ST) + 6*sizeof(u32));
	    msg->msgData[0] = BW_8_MHZ;
      msg->msgData[1] = 2; //0
      msg->msgData[2] = 0;
      msg->msgData[3] = 530000000;
      msg->msgData[4] = 8000000;
      msg->msgData[5] = 530000000;

	    smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
	    rc = smscore_sendrequest_and_wait(coredev, msg,
					msg->xMsgHeader.msgLength,
					&coredev->scan_done);
        if(rc<0)
        {
          sms_err("smscore_set_device_mode, failed in scan start\n");
        }

      //jackz, we need to wait for SCAN_COMPLETE_IND
      sms_info("Waiting for SCAN_COMPLETE_IND\n");
      wait_for_completion_timeout(&coredev->scan_complete, msecs_to_jiffies(10000));

#if 1     
      sms_info("Start Loopback test after scan\n");
      rc = AdrLoopbackTest(coredev);
      if(rc<0)
      {
        sms_err("smscore_set_device_mode, failed in loopback test, rc=%d\n", rc);
      }
#endif

#if 0
      msleep(5000);
      sms_info("Start service\n");
	    SMS_INIT_MSG(&msg->xMsgHeader, MSG_SMS_CMMB_START_SERVICE_REQ,
					sizeof(struct SmsMsgHdr_ST) + 4*sizeof(u32));
	    msg->msgData[0] = 0xFFFFFFFF;
      msg->msgData[1] = 0xFFFFFFFF;
      msg->msgData[2] = 605;
      msg->msgData[3] = 0;

	    smsendian_handle_tx_message((struct SmsMsgHdr_ST *)msg);
	    rc = smscore_sendrequest_and_wait(coredev, msg,
					msg->xMsgHeader.msgLength,
					&coredev->start_service_done);

      //jackz, we need to wait for dab_count_done
      sms_info("Waiting for dab_count_done\n");
      wait_for_completion_timeout(&coredev->dab_count_done, msecs_to_jiffies(5*60*1000));
#endif
	sms_info("end");
	}
	else
	{
		sms_err("Allocate buffer failed");
		return -ENOMEM;
	}
	kfree(buffer);
#endif
	} else {
		if (mode < DEVICE_MODE_DVBT || mode > DEVICE_MODE_DVBT_BDA) {
			sms_err("invalid mode specified %d", mode);
			return -EINVAL;
		}

		smscore_registry_setmode(coredev->devpath, mode);

		if (coredev->detectmode_handler)
			coredev->detectmode_handler(coredev->context,
					&coredev->mode);

		if (coredev->mode != mode && coredev->setmode_handler) {
			rc = coredev->setmode_handler(coredev->context, mode);

			if (rc < 0) {
				sms_err("return error code %d.", rc);
			}
		}
	}

	if (rc >= 0) {
		coredev->mode = mode;
		coredev->device_flags &= ~SMS_DEVICE_NOT_READY;
	}

	return rc;
}

/**
 * configures device features according to voard configuration structure.
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 *
 * @return 0 on success, <0 on error.
 */
int smscore_configure_board(struct smscore_device_t *coredev) {
	struct sms_board* board;

	board = sms_get_board(coredev->board_id);
	if (!board)
	{
		sms_err("no board configuration exist.");
		return -1;
	}
	
	if (board->mtu)
	{
		struct SmsMsgData_ST MtuMsg;
		sms_debug("set max transmit unit %d", board->mtu);

		MtuMsg.xMsgHeader.msgSrcId = 0;
		MtuMsg.xMsgHeader.msgDstId = HIF_TASK;
		MtuMsg.xMsgHeader.msgFlags = 0;
		MtuMsg.xMsgHeader.msgType = MSG_SMS_SET_MAX_TX_MSG_LEN_REQ;
		MtuMsg.xMsgHeader.msgLength = sizeof(MtuMsg);
		MtuMsg.msgData[0] = board->mtu;

		smsendian_handle_tx_message((struct SmsMsgHdr_ST *)&MtuMsg);
		coredev->sendrequest_handler(coredev->context, &MtuMsg, sizeof(MtuMsg));
	}

	if (board->crystal)
	{
		struct SmsMsgData_ST CrysMsg;
		sms_debug("set crystal value %d", board->crystal);

		SMS_INIT_MSG(&CrysMsg.xMsgHeader, 
				MSG_SMS_NEW_CRYSTAL_REQ,
				sizeof(CrysMsg));
		CrysMsg.msgData[0] = board->crystal;

		smsendian_handle_tx_message((struct SmsMsgHdr_ST *)&CrysMsg);
		coredev->sendrequest_handler(coredev->context, &CrysMsg, sizeof(CrysMsg));
	}

	return 0;
}

/**
 * calls device handler to get current mode of operation
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 *
 * @return current mode
 */
int smscore_get_device_mode(struct smscore_device_t *coredev)
{
	return coredev->mode;
}

/**
 * find client by response id & type within the clients list.
 * return client handle or NULL.
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 * @param data_type client data type (SMS_DONT_CARE for all types)
 * @param id client id (SMS_DONT_CARE for all id)
 *
 */
static struct smscore_client_t *smscore_find_client(
		struct smscore_device_t *coredev, int data_type, int id) {
	struct smscore_client_t *client = NULL;
	struct list_head *next, *first;
	unsigned long flags;
	struct list_head *firstid, *nextid;

	spin_lock_irqsave(&coredev->clientslock, flags);
	first = &coredev->clients;
	for (next = first->next; (next != first) && !client;
			next = next->next) {
		firstid = &((struct smscore_client_t *) next)->idlist;
		for (nextid = firstid->next; nextid != firstid;
				nextid = nextid->next) {
			if ((((struct smscore_idlist_t *) nextid)->id == id)
					&& (((struct smscore_idlist_t *)
						 nextid)->data_type
						== data_type
						|| (((struct smscore_idlist_t *)
						nextid)->data_type == 0))) {
				client = (struct smscore_client_t *) next;
				break;
			}
		}
	}
	spin_unlock_irqrestore(&coredev->clientslock, flags);
	return client;
}

/**
 * find client by response id/type, call clients onresponse handler
 * return buffer to pool on error
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 * @param cb pointer to response buffer descriptor
 *
 */
void smscore_onresponse(struct smscore_device_t *coredev,
		struct smscore_buffer_t *cb) {
	struct SmsMsgHdr_ST *phdr = (struct SmsMsgHdr_ST *) ((u8 *) cb->p
			+ cb->offset);
	struct smscore_client_t *client;
	int rc = -EBUSY;
	unsigned long msgLen;

#if 0
	static unsigned long last_sample_time; /* = 0; */
	static int data_total; /* = 0; */
	unsigned long time_now = jiffies_to_msecs(jiffies);

	if (!last_sample_time)
		last_sample_time = time_now;

	if (time_now - last_sample_time > 10000) {
		sms_debug("\ndata rate %d bytes/secs",
				(int)((data_total * 1000) /
						(time_now - last_sample_time)));

		last_sample_time = time_now;
		data_total = 0;
	}

	data_total += cb->size;
#endif

	/* Do we need to re-route? */
	if ((phdr->msgType == MSG_SMS_HO_PER_SLICES_IND) ||
			(phdr->msgType == MSG_SMS_TRANSMISSION_IND)) {
		if (coredev->mode == DEVICE_MODE_DVBT_BDA)
			phdr->msgDstId = DVBT_BDA_CONTROL_MSG_ID;
	}
	msgLen = phdr->msgLength;

	client = smscore_find_client(coredev, phdr->msgType, phdr->msgDstId);

	/* If no client registered for type & id,
	 * check for control client where type is not registered */
	if (client)
		rc = client->onresponse_handler(client->context, cb);

	sms_info("rc %d, msgType %d", rc, phdr->msgType);

	if (rc < 0) {
		smsendian_handle_rx_message((struct SmsMsgData_ST *)phdr);
		//sms_info("msgType %d", phdr->msgType);
		switch (phdr->msgType) {
		case MSG_SMS_ISDBT_TUNE_RES:
			sms_debug("MSG_SMS_ISDBT_TUNE_RES");
			break;
		case MSG_SMS_RF_TUNE_RES:
			sms_debug("MSG_SMS_RF_TUNE_RES");
			break;
		case MSG_SMS_SIGNAL_DETECTED_IND:
			sms_debug("MSG_SMS_SIGNAL_DETECTED_IND");
			break;
		case MSG_SMS_NO_SIGNAL_IND:
			sms_debug("MSG_SMS_NO_SIGNAL_IND");
			break;
		case MSG_SMS_DEVICE_READY_IND:
			sms_debug("MSG_SMS_DEVICE_READY_IND");
			msleep(200);
			complete(&coredev->device_ready_done);
			break;
		case MSG_SMS_SPI_INT_LINE_SET_RES:
			sms_debug("MSG_SMS_SPI_INT_LINE_SET_RES");
			break;
		case MSG_SMS_INTERFACE_LOCK_IND: 
			sms_debug("MSG_SMS_INTERFACE_LOCK_IND");
			break;
		case MSG_SMS_INTERFACE_UNLOCK_IND:
			sms_debug("MSG_SMS_INTERFACE_UNLOCK_IND");
			break;
		case MSG_SMS_GET_VERSION_EX_RES: {
			struct SmsVersionRes_ST *ver =
					(struct SmsVersionRes_ST *) phdr;
			sms_debug("MSG_SMS_GET_VERSION_EX_RES "
					"id %d prots 0x%x ver %d.%d, %s",
					ver->FirmwareId,
					ver->SupportedProtocols,
					ver->RomVersionMajor,
					ver->RomVersionMinor,
					ver->TextLabel);

			coredev->mode = ver->FirmwareId == 255 ?
					DEVICE_MODE_NONE : ver->FirmwareId;
			coredev->modes_supported = ver->SupportedProtocols;

			complete(&coredev->version_ex_done);
			break;
		}
		case MSG_SMS_INIT_DEVICE_RES:
			sms_debug("MSG_SMS_INIT_DEVICE_RES");
			complete(&coredev->init_device_done);
			break;
		case MSG_SW_RELOAD_START_RES:
			sms_debug("MSG_SW_RELOAD_START_RES");
			complete(&coredev->reload_start_done);
			break;
		case MSG_SMS_DATA_DOWNLOAD_RES:
			complete(&coredev->data_download_done);
			break;
		case MSG_SW_RELOAD_EXEC_RES:
			sms_debug("MSG_SW_RELOAD_EXEC_RES");
			break;
		case MSG_SMS_SWDOWNLOAD_TRIGGER_RES:
			sms_debug("MSG_SMS_SWDOWNLOAD_TRIGGER_RES");
			complete(&coredev->trigger_done);
			break;
		case MSG_SMS_NEW_CRYSTAL_RES:                                 //clock msg 
			sms_debug("MSG_SMS_NEW_CRYSTAL_RES");
			complete(&coredev->clock_done);
			break;			
		case MSG_SMS_SLEEP_RESUME_COMP_IND:
			sms_debug("MSG_SMS_SLEEP_RESUME_COMP_IND");
			complete(&coredev->resume_done);
			break;
		case MSG_SMS_GPIO_CONFIG_EX_RES:
			sms_debug("MSG_SMS_GPIO_CONFIG_EX_RES");
			complete(&coredev->gpio_configuration_done);
			break;
		case MSG_SMS_GPIO_SET_LEVEL_RES:
			sms_debug("MSG_SMS_GPIO_SET_LEVEL_RES");
			complete(&coredev->gpio_set_level_done);
			break;
		case MSG_SMS_GPIO_GET_LEVEL_RES:
		{
			u32 *msgdata = (u32 *) phdr;
			coredev->gpio_get_res = msgdata[1];
			sms_debug("MSG_SMS_GPIO_GET_LEVEL_RES gpio level %d",
					coredev->gpio_get_res);
			complete(&coredev->gpio_get_level_done);
			break;
		}
		case MSG_SMS_START_IR_RES:
			complete(&coredev->ir_init_done);
			break;
		case MSG_SMS_IR_SAMPLES_IND:
			sms_ir_event(coredev,
				(const char *)
				((char *)phdr
				+ sizeof(struct SmsMsgHdr_ST)),
				(int)phdr->msgLength
				- sizeof(struct SmsMsgHdr_ST));
			break;
#if 1 //JACKZ
	    case MSG_SMS_SET_PERIODIC_STATISTICS_RES:
	      sms_debug("MSG_SMS_SET_PERIODIC_STATISTICS_RES, length: 0x%x\n", (unsigned int)msgLen);
	      complete(&coredev->periodic_stat_done);
	      break;
	    case MSG_SMS_GET_STATISTICS_EX_RES:
	      {
	        struct SmsStatisticsExRes_ST* pStatsEx = (struct SmsStatisticsExRes_ST*)phdr;
	        sms_debug("MSG_SMS_GET_STATISTICS_EX_RES, length: 0x%x\n", (unsigned int)msgLen);
	        if(pStatsEx != NULL)
	        {
	          printk("Reception parameters:\n");
	          printk("Return Code: %d, ", pStatsEx->ReturnCode);
	          printk("RF locked: %s, ", pStatsEx->IsRfLocked? "Yes" : "No");
	          printk("Demod locked: %s, ", pStatsEx->IsDemodLocked? "Yes" : "No");
	          printk("In Band Power: %d dBM, ", pStatsEx->InBandPwr);
	          printk("SNR: %d dB, ", pStatsEx->SNR >> 16);  //jackz, SNR needs to be right-shifted by 16 bits.
	          printk("Frequency: %d, ", pStatsEx->Frequency);
	          printk("BER: %d, ", pStatsEx->BER);
	          printk("Error count: %d\n", pStatsEx->ErrorsCounter);
	        }
	        break;
	      }
	    case MSG_SMS_SCAN_START_RES:
	      sms_debug("MSG_SMS_SCAN_START_RES, length: 0x%x\n", (unsigned int)msgLen);
	      complete(&coredev->scan_done);
	      break;
	      case MSG_SMS_SCAN_COMPLETE_IND:
	        sms_debug("MSG_SMS_SCAN_COMPLETE_IND, length: 0x%x\n", (unsigned int)msgLen);
	        //g_ScanComplete = 1;
	        complete(&coredev->scan_complete);
	        break;
	    case MSG_SMS_CMMB_START_SERVICE_RES:
	      sms_info("MSG_SMS_CMMB_START_SERVICE_RES, length: 0x%x\n", (unsigned int)msgLen);
	      complete(&coredev->start_service_done);
	      break;
	    case MSG_SMS_DAB_CHANNEL:
	      sms_debug("MSG_SMS_DAB_CHANNEL, length: 0x%x\n", (unsigned int)msgLen);
	      dabCount++;
	      if(dabCount >= 1600)
	      	{
			complete(&coredev->dab_count_done);
	      	}
	      break;
		case MSG_SMS_LOOPBACK_RES:
		{
			memcpy( g_LbResBuf, (u8 *)phdr, phdr->msgLength );
			sms_debug("MSG_SMS_LOOPBACK_RES, length: 0x%x\n", (unsigned int)msgLen);
			complete(&coredev->loopback_res_done);
			break;
		}
		case MSG_SMS_CMMB_SET_CA_CW_RES:
		{
			struct SmsMsgData_ST2 *cwRes =
					(struct SmsMsgData_ST2 *) phdr;
			sms_info("MSG_SMS_CMMB_SET_CA_CW_RES, RetCode: 0x%x", cwRes->msgData[0]);
			break;
		}
		case MSG_SMS_CMMB_SET_CA_SALT_RES:
		{
			struct SmsMsgData_ST2 *saltRes =
					(struct SmsMsgData_ST2 *) phdr;
			sms_info("MSG_SMS_CMMB_SET_CA_SALT_RES, RetCode: 0x%x", saltRes->msgData[0]);
			break;
		}



#endif
		default:
#if 0
			sms_info("no client (%p) or error (%d), "
					"type:%d dstid:%d", client, rc,
					phdr->msgType, phdr->msgDstId);
#endif
			break;
		}
		smscore_putbuffer(coredev, cb);
	}
}

/**
 * return pointer to next free buffer descriptor from core pool
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 *
 * @return pointer to descriptor on success, NULL on error.
 */
struct smscore_buffer_t *smscore_getbuffer(struct smscore_device_t *coredev)
{
	struct smscore_buffer_t *cb = NULL;
	unsigned long flags;

	DEFINE_WAIT(wait);

	spin_lock_irqsave(&coredev->bufferslock, flags);

	/* set the current process state to interruptible sleep
	 * in case schedule() will be called, this process will go to sleep 
	 * and woken up only when a new buffer is available (see smscore_putbuffer)
	 */
	prepare_to_wait(&coredev->buffer_mng_waitq, &wait, TASK_INTERRUPTIBLE);

	if (list_empty(&coredev->buffers)) {
		sms_debug("no avaliable common buffer, need to schedule");

		/* 
         * before going to sleep, release the lock 
         */
		spin_unlock_irqrestore(&coredev->bufferslock, flags);

		schedule();

		sms_debug("wake up after schedule()");

		/* 
         * acquire the lock again 
         */
		spin_lock_irqsave(&coredev->bufferslock, flags);
	}

	/* 
         * in case that schedule() was skipped, set the process state to running
	 */
	finish_wait(&coredev->buffer_mng_waitq, &wait);

	/* 
         * verify that the list is not empty, since it might have been 
	 * emptied during the sleep
	 * comment : this sitation can be avoided using spin_unlock_irqrestore_exclusive	
	 */	
	if (list_empty(&coredev->buffers)) {
		sms_err("failed to allocate buffer, returning NULL");
		spin_unlock_irqrestore(&coredev->bufferslock, flags);
		return NULL;
	}
	
	cb = (struct smscore_buffer_t *) coredev->buffers.next;
	list_del(&cb->entry);

	spin_unlock_irqrestore(&coredev->bufferslock, flags);

	return cb;
}

/**
 * return buffer descriptor to a pool
 *
 * @param coredev pointer to a coredev object returned by
 *                smscore_register_device
 * @param cb pointer buffer descriptor
 *
 */
void smscore_putbuffer(struct smscore_device_t *coredev,
		struct smscore_buffer_t *cb) {
	list_add_locked(&cb->entry, &coredev->buffers, &coredev->bufferslock);
	wake_up_interruptible(&coredev->buffer_mng_waitq);
}

static int smscore_validate_client(struct smscore_device_t *coredev,
		struct smscore_client_t *client, int data_type, int id) {
	struct smscore_idlist_t *listentry;
	struct smscore_client_t *registered_client;

	if (!client) {
		sms_err("bad parameter.");
		return -EFAULT;
	}
	registered_client = smscore_find_client(coredev, data_type, id);
	if (registered_client == client)
		return 0;

	if (registered_client) {
{		sms_err("The msg ID already registered to another client.");}
		return -EEXIST;
	}
	listentry = kzalloc(sizeof(struct smscore_idlist_t), GFP_KERNEL);
	if (!listentry) {
{		sms_err("Can't allocate memory for client id.");}
		return -ENOMEM;
	}
	listentry->id = id;
	listentry->data_type = data_type;
	list_add_locked(&listentry->entry, &client->idlist,
			&coredev->clientslock);
	return 0;
}

/**
 * creates smsclient object, check that id is taken by another client
 *
 * @param coredev pointer to a coredev object from clients hotplug
 * @param initial_id all messages with this id would be sent to this client
 * @param data_type all messages of this type would be sent to this client
 * @param onresponse_handler client handler that is called to
 *                           process incoming messages
 * @param onremove_handler client handler that is called when device is removed
 * @param context client-specific context
 * @param client pointer to a value that receives created smsclient object
 *
 * @return 0 on success, <0 on error.
 */
int smscore_register_client(struct smscore_device_t *coredev,
		struct smsclient_params_t *params,
		struct smscore_client_t **client) {
	struct smscore_client_t *newclient;
	/* check that no other channel with same parameters exists */
	if (smscore_find_client(coredev, params->data_type,
				params->initial_id)) {
{		sms_err("Client already exist.");}
		return -EEXIST;
	}

	newclient = kzalloc(sizeof(struct smscore_client_t), GFP_KERNEL);
	if (!newclient) {
{		sms_err("Failed to allocate memory for client.");}
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&newclient->idlist);
	newclient->coredev = coredev;
	newclient->onresponse_handler = params->onresponse_handler;
	newclient->onremove_handler = params->onremove_handler;
	newclient->context = params->context;
	list_add_locked(&newclient->entry, &coredev->clients,
			&coredev->clientslock);
	smscore_validate_client(coredev, newclient, params->data_type,
			params->initial_id);
	*client = newclient;
	sms_debug("register new client 0x%p DT=%d ID=%d",
		params->context, params->data_type, params->initial_id);

	return 0;
}

/**
 * frees smsclient object and all subclients associated with it
 *
 * @param client pointer to smsclient object returned by
 *               smscore_register_client
 *
 */
void smscore_unregister_client(struct smscore_client_t *client)
{
	struct smscore_device_t *coredev = client->coredev;
	unsigned long flags;

	spin_lock_irqsave(&coredev->clientslock, flags);

	while (!list_empty(&client->idlist)) {
		struct smscore_idlist_t *identry =
				(struct smscore_idlist_t *) client->idlist.next;
		list_del(&identry->entry);
		kfree(identry);
	}

	sms_info("unregister client 0x%p", client->context);

	list_del(&client->entry);
	kfree(client);

	spin_unlock_irqrestore(&coredev->clientslock, flags);
}

/**
 * verifies that source id is not taken by another client,
 * calls device handler to send requests to the device
 *
 * @param client pointer to smsclient object returned by
 *               smscore_register_client
 * @param buffer pointer to a request buffer
 * @param size size (in bytes) of request buffer
 *
 * @return 0 on success, <0 on error.
 */
int smsclient_sendrequest(struct smscore_client_t *client, void *buffer,
		size_t size) {
	struct smscore_device_t *coredev;
	struct SmsMsgHdr_ST *phdr = (struct SmsMsgHdr_ST *) buffer;
	int rc;

	if (client == NULL) {
{		sms_err("Got NULL client");}
		return -EINVAL;
	}

	coredev = client->coredev;

	/* check that no other channel with same id exists */
	if (coredev == NULL) {
{		sms_err("Got NULL coredev");}
		return -EINVAL;
	}

	rc = smscore_validate_client(client->coredev, client, 0,                 
			phdr->msgSrcId);
	if (rc < 0)
		return rc;

	return coredev->sendrequest_handler(coredev->context, buffer, size);             
}

#ifdef SMS_HOSTLIB_SUBSYS
/**
 * return the size of large (common) buffer
 *
 * @param coredev pointer to a coredev object from clients hotplug
 *
 * @return size (in bytes) of the buffer
 */
int smscore_get_common_buffer_size(struct smscore_device_t *coredev)
{
	return coredev->common_buffer_size;
}

/**
 * maps common buffer (if supported by platform)
 *
 * @param coredev pointer to a coredev object from clients hotplug
 * @param vma pointer to vma struct from mmap handler
 *
 * @return 0 on success, <0 on error.
 */
int smscore_map_common_buffer(struct smscore_device_t *coredev,
		struct vm_area_struct *vma)
{
#if 0           //buf issue

	unsigned long end = vma->vm_end,
	start = vma->vm_start,
	size = PAGE_ALIGN(coredev->common_buffer_size);

	if (!vma->vm_flags & (VM_READ | VM_WRITE | VM_SHARED))  {
{		sms_err("invalid vm flags");}
		return -EINVAL;
	}

	if ((end - start) != size) {
		{sms_err("invalid size %d expected %d",
				(int)(end - start), (int)size);}
		return -EINVAL;
	}

	if (remap_pfn_range(vma, start,                                                                        //?  common_buffer_phys changed for kmalloc
			coredev->common_buffer_phys >> PAGE_SHIFT,
			size, pgprot_noncached(vma->vm_page_prot))) {
		{sms_err("remap_page_range failed");}
		return -EAGAIN;
	}

	return 0;
#else
    unsigned long size = PAGE_ALIGN(coredev->common_buffer_size);   
    unsigned long phy= virt_to_phys(coredev->common_buffer);
    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
    if (remap_pfn_range(vma, vma->vm_start, ((unsigned int)phy)>> PAGE_SHIFT, size, vma->vm_page_prot|PAGE_SHARED))
      {
           sms_err("remap_page_range failed");
	    return - EAGAIN;
	}

	return 0;

#endif
}
#endif /* SMS_HOSTLIB_SUBSYS */

static int GetGpioPinParams(u32 PinNum, u32 *pTranslatedPinNum,
		u32 *pGroupNum, u32 *pGroupCfg) {

	*pGroupCfg = 1;

	if (PinNum >= 0 && PinNum <= 1)	{
		*pTranslatedPinNum = 0;
		*pGroupNum = 9;
		*pGroupCfg = 2;
	} else if (PinNum >= 2 && PinNum <= 6) {
		*pTranslatedPinNum = 2;
		*pGroupNum = 0;
		*pGroupCfg = 2;
	} else if (PinNum >= 7 && PinNum <= 11) {
		*pTranslatedPinNum = 7;
		*pGroupNum = 1;
	} else if (PinNum >= 12 && PinNum <= 15) {
		*pTranslatedPinNum = 12;
		*pGroupNum = 2;
		*pGroupCfg = 3;
	} else if (PinNum == 16) {
		*pTranslatedPinNum = 16;
		*pGroupNum = 23;
	} else if (PinNum >= 17 && PinNum <= 24) {
		*pTranslatedPinNum = 17;
		*pGroupNum = 3;
	} else if (PinNum == 25) {
		*pTranslatedPinNum = 25;
		*pGroupNum = 6;
	} else if (PinNum >= 26 && PinNum <= 28) {
		*pTranslatedPinNum = 26;
		*pGroupNum = 4;
	} else if (PinNum == 29) {
		*pTranslatedPinNum = 29;
		*pGroupNum = 5;
		*pGroupCfg = 2;
	} else if (PinNum == 30) {
		*pTranslatedPinNum = 30;
		*pGroupNum = 8;
	} else if (PinNum == 31) {
		*pTranslatedPinNum = 31;
		*pGroupNum = 17;
	} else
		return -1;

	*pGroupCfg <<= 24;

	return 0;
}

int smscore_gpio_configure(struct smscore_device_t *coredev, u8 PinNum,
		struct smscore_gpio_config *pGpioConfig) {

	u32 totalLen;
	u32 TranslatedPinNum;
	u32 GroupNum;
	u32 ElectricChar;
	u32 groupCfg;
	void *buffer;
	int rc;

	struct SetGpioMsg {
		struct SmsMsgHdr_ST xMsgHeader;
		u32 msgData[6];
	} *pMsg;


	if (PinNum > MAX_GPIO_PIN_NUMBER)
		return -EINVAL;

	if (pGpioConfig == NULL)
		return -EINVAL;

	totalLen = sizeof(struct SmsMsgHdr_ST) + (sizeof(u32) * 6);

	buffer = kmalloc(totalLen + SMS_DMA_ALIGNMENT,
			GFP_KERNEL | GFP_DMA);
	if (!buffer)
		return -ENOMEM;

	pMsg = (struct SetGpioMsg *) SMS_ALIGN_ADDRESS(buffer);

	pMsg->xMsgHeader.msgSrcId = DVBT_BDA_CONTROL_MSG_ID;
	pMsg->xMsgHeader.msgDstId = HIF_TASK;
	pMsg->xMsgHeader.msgFlags = 0;
	pMsg->xMsgHeader.msgLength = (u16) totalLen;
	pMsg->msgData[0] = PinNum;

	if (!(coredev->device_flags & SMS_DEVICE_FAMILY2)) {
		pMsg->xMsgHeader.msgType = MSG_SMS_GPIO_CONFIG_REQ;
		if (GetGpioPinParams(PinNum, &TranslatedPinNum, &GroupNum,
				&groupCfg) != 0)
			return -EINVAL;

		pMsg->msgData[1] = TranslatedPinNum;
		pMsg->msgData[2] = GroupNum;
		ElectricChar = (pGpioConfig->PullUpDown)
				| (pGpioConfig->InputCharacteristics << 2)
				| (pGpioConfig->OutputSlewRate << 3)
				| (pGpioConfig->OutputDriving << 4);
		pMsg->msgData[3] = ElectricChar;
		pMsg->msgData[4] = pGpioConfig->Direction;
		pMsg->msgData[5] = groupCfg;
	} else {
		pMsg->xMsgHeader.msgType = MSG_SMS_GPIO_CONFIG_EX_REQ;
		pMsg->msgData[1] = pGpioConfig->PullUpDown;
		pMsg->msgData[2] = pGpioConfig->OutputSlewRate;
		pMsg->msgData[3] = pGpioConfig->OutputDriving;
		pMsg->msgData[4] = pGpioConfig->Direction;
		pMsg->msgData[5] = 0;
	}

	smsendian_handle_tx_message((struct SmsMsgHdr_ST *)pMsg);
	rc = smscore_sendrequest_and_wait(coredev, pMsg, totalLen,
			&coredev->gpio_configuration_done);

	if (rc != 0) {
		if (rc == -ETIME)
			{sms_err("smscore_gpio_configure timeout");}
		else
			{sms_err("smscore_gpio_configure error");}
	}
	kfree(buffer);

	return rc;
}

int smscore_gpio_set_level(struct smscore_device_t *coredev, u8 PinNum,
		u8 NewLevel) {

	u32 totalLen;
	int rc;
	void *buffer;

	struct SetGpioMsg {
		struct SmsMsgHdr_ST xMsgHeader;
		u32 msgData[3]; /* keep it 3 ! */
	} *pMsg;

	if ((NewLevel > 1) || (PinNum > MAX_GPIO_PIN_NUMBER) ||
			(PinNum > MAX_GPIO_PIN_NUMBER))
		return -EINVAL;

	totalLen = sizeof(struct SmsMsgHdr_ST) +
			(3 * sizeof(u32)); /* keep it 3 ! */

	buffer = kmalloc(totalLen + SMS_DMA_ALIGNMENT,
			GFP_KERNEL | GFP_DMA);
	if (!buffer)
		return -ENOMEM;

	pMsg = (struct SetGpioMsg *) SMS_ALIGN_ADDRESS(buffer);

	pMsg->xMsgHeader.msgSrcId = DVBT_BDA_CONTROL_MSG_ID;
	pMsg->xMsgHeader.msgDstId = HIF_TASK;
	pMsg->xMsgHeader.msgFlags = 0;
	pMsg->xMsgHeader.msgType = MSG_SMS_GPIO_SET_LEVEL_REQ;
	pMsg->xMsgHeader.msgLength = (u16) totalLen;
	pMsg->msgData[0] = PinNum;
	pMsg->msgData[1] = NewLevel;

	/* Send message to SMS */
	smsendian_handle_tx_message((struct SmsMsgHdr_ST *)pMsg);
	rc = smscore_sendrequest_and_wait(coredev, pMsg, totalLen,
			&coredev->gpio_set_level_done);

	if (rc != 0) {
		if (rc == -ETIME)
			{sms_err("smscore_gpio_set_level timeout");}
		else
{			sms_err("smscore_gpio_set_level error");}
	}
	kfree(buffer);

	return rc;
}

int smscore_gpio_get_level(struct smscore_device_t *coredev, u8 PinNum,
		u8 *level) {

	u32 totalLen;
	int rc;
	void *buffer;

	struct SetGpioMsg {
		struct SmsMsgHdr_ST xMsgHeader;
		u32 msgData[2];
	} *pMsg;


	if (PinNum > MAX_GPIO_PIN_NUMBER)
		return -EINVAL;

	totalLen = sizeof(struct SmsMsgHdr_ST) + (2 * sizeof(u32));

	buffer = kmalloc(totalLen + SMS_DMA_ALIGNMENT,
			GFP_KERNEL | GFP_DMA);
	if (!buffer)
		return -ENOMEM;

	pMsg = (struct SetGpioMsg *) SMS_ALIGN_ADDRESS(buffer);

	pMsg->xMsgHeader.msgSrcId = DVBT_BDA_CONTROL_MSG_ID;
	pMsg->xMsgHeader.msgDstId = HIF_TASK;
	pMsg->xMsgHeader.msgFlags = 0;
	pMsg->xMsgHeader.msgType = MSG_SMS_GPIO_GET_LEVEL_REQ;
	pMsg->xMsgHeader.msgLength = (u16) totalLen;
	pMsg->msgData[0] = PinNum;
	pMsg->msgData[1] = 0;

	/* Send message to SMS */
	smsendian_handle_tx_message((struct SmsMsgHdr_ST *)pMsg);
	rc = smscore_sendrequest_and_wait(coredev, pMsg, totalLen,
			&coredev->gpio_get_level_done);

	if (rc != 0) {
		if (rc == -ETIME)
			{sms_err("smscore_gpio_get_level timeout");}
		else
			{sms_err("smscore_gpio_get_level error");}
	}
	kfree(buffer);

	/* Its a race between other gpio_get_level() and the copy of the single
	 * global 'coredev->gpio_get_res' to  the function's variable 'level'
	 */
	*level = coredev->gpio_get_res;

	return rc;
}

static int __init smscore_module_init(void)                    
{
	int rc = 0;
	sms_info("");
	sms_info("smsmdtv register, version %d.%d.%d",
		version.major, version.minor, version.revision);

	/* 
	 * first, create global core device global linked lists
	 */

	INIT_LIST_HEAD(&g_smscore_notifyees);
	INIT_LIST_HEAD(&g_smscore_devices);
	kmutex_init(&g_smscore_deviceslock);

	INIT_LIST_HEAD(&g_smscore_registry);
	kmutex_init(&g_smscore_registrylock);

	/* 
	 * second, register sub system adapter objects 
	 */

#ifdef SMS_NET_SUBSYS
	/* Network device register */
	rc = smsnet_register();
	sms_debug("smsnet_register, %d", rc);
	if (rc) {
		sms_err("Error registering Siano's network client.\n");
		goto smsnet_error;
	}
#endif

#ifdef SMS_HOSTLIB_SUBSYS
	/* Char device register */
	rc = smschar_register();
	sms_debug("smschar registered, %d", rc);
	if (rc) {
		sms_err("Error registering Siano's char device client.\n");
		goto smschar_error;
	}
#endif

#ifdef SMS_DVB3_SUBSYS
	/* DVB-API v.3 register */
	rc = smsdvb_register();
	sms_debug("smsdvb registered, %d", rc);
	if (rc) {
		sms_err("Error registering DVB client.\n");
		goto smsdvb_error;
	}
#endif

	/* 
	 * third, register interfaces objects 
	 */

#ifdef SMS_USB_DRV
	/* USB Register */
	rc = smsusb_register();
	sms_debug("smsusb registered, %d", rc);
	if (rc) {
		sms_err("Error registering USB bus driver.\n");
		goto smsusb_error;
	}
#endif

#ifdef SMS_SDIO_DRV
	/* SDIO Register */
	rc = smssdio_register();
	sms_debug("smssdio registered, %d", rc);
	if (rc) {
		sms_err("Error registering SDIO bus driver.\n");
		goto smssdio_error;
	}
#endif

#ifdef SMS_SPI_MTK
	/* Intel PXA310 SPI Register */
	rc = smsspi_register();
	sms_debug("smsspi registered, %d", rc);
	if (rc) {
		sms_err("Error registering Intel PXA310 SPI bus driver.\n");
		goto smsspi_error;
	}
#endif

	sms_info("smsmdtv registered succesfully\n");
	return rc;

#ifdef SMS_SPI_MTK
smsspi_error:
#endif

#ifdef SMS_SDIO_DRV
	smssdio_unregister();
smssdio_error:
#endif

#ifdef SMS_USB_DRV
	smsusb_unregister();
smsusb_error:
#endif

#ifdef SMS_DVB3_SUBSYS
	smsdvb_unregister();
smsdvb_error:
#endif

#ifdef SMS_HOSTLIB_SUBSYS
	smschar_unregister();
smschar_error:
#endif

#ifdef SMS_NET_SUBSYS
	smsnet_unregister();
smsnet_error:
#endif

	sms_err("rc %d", rc);
	printk(KERN_INFO "%s, rc %d\n", __func__, rc);

	return rc;
}

static void __exit smscore_module_exit(void)
{
	sms_info("");
	sms_info("smsmdtv unregister");

	/* 
	 * first, unregister interfaces objects
	 */

#ifdef SMS_USB_DRV
	/* USB unregister */
	smsusb_unregister();
#endif

#ifdef SMS_SDIO_DRV
	/* SDIO unregister */
	smssdio_unregister();
#endif

#ifdef SMS_SPI_MTK
	/* Intel PXA310 SPI unregister */
	smsspi_unregister();
#endif

	/* 
	 * second, unregister sub system adapter objects 
	 * at this point, the core device should be unregistered and freed, and
	 * all of the clients should be notified about it.
	 * otherwise, messages can be transmitted although there is no 
	 * registerd interface.
	 */

#ifdef SMS_NET_SUBSYS
	/* Network device unregister */
	smsnet_unregister();
#endif

#ifdef SMS_HOSTLIB_SUBSYS
	/* Char device unregister */
	smschar_unregister();
#endif

#ifdef SMS_DVB3_SUBSYS
	/* DVB-API v.3 unregister */
	smsdvb_unregister();
#endif

	/* 
	 * third, empty global core device global linked lists
	 */

	kmutex_lock(&g_smscore_deviceslock);
	while (!list_empty(&g_smscore_notifyees)) {
		struct smscore_device_notifyee_t *notifyee =
		(struct smscore_device_notifyee_t *)
		g_smscore_notifyees.next;

		list_del(&notifyee->entry);
		kfree(notifyee);
	}
	kmutex_unlock(&g_smscore_deviceslock);
	kmutex_lock(&g_smscore_registrylock);
	while (!list_empty(&g_smscore_registry)) {
		struct smscore_registry_entry_t *entry =
		(struct smscore_registry_entry_t *)
		g_smscore_registry.next;

		list_del(&entry->entry);
		kfree(entry);
	}
	kmutex_unlock(&g_smscore_registrylock);

	sms_info("smsmdtv unregistered\n");
}

module_init(smscore_module_init);
module_exit(smscore_module_exit);

MODULE_DESCRIPTION("Siano MDTV Core module - Version 2.2.3");
MODULE_AUTHOR("Siano Mobile Silicon, Inc. (erezc@siano-ms.com)");
MODULE_LICENSE("GPL");
