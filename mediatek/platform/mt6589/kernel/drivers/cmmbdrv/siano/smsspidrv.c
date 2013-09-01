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
/*!
	\file	spibusdrv.c

	\brief	spi bus driver module

	This file contains implementation of the spi bus driver.
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>

#include "smscoreapi.h"
#include "smsdbg_prn.h"
#include "smsspicommon.h"
#include "smsspiphy.h"

#ifdef MTK_SPI                                                                   
#include <linux/delay.h>
#include <mach/eint.h>
#include <cust_eint.h>
#include <mach/irqs.h>
#include <linux/io.h>                              //debug

struct SMSDev_data {
	dev_t			    devt;
	spinlock_t		    spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;

	struct mutex		buf_lock;
	unsigned char   	users;
	u8			       *tx_buf;
	u8                         *rx_buf;
};

extern struct SMSDev_data* SMSDev;
#endif  //MTK_SPI

#ifdef CONFIG_SMP
static struct workqueue_struct *spi_thread_wq=NULL; 
static struct work_struct spi_thread_work;
#endif

#define SMS_INTR_PIN			16  /* 0 for nova sip, 26 for vega */
#define TX_BUFFER_SIZE			0x200
//#define RX_BUFFER_SIZE			(0x1000 + SPI_PACKET_SIZE + 0x100)
#define NUM_RX_BUFFERS			72

struct _spi_device_st {
	struct _spi_dev dev;
	void *phy_dev;

	struct completion write_operation;
	struct list_head tx_queue;
	int allocatedPackets;
	int padding_allowed;
	char *rxbuf;

	struct smscore_device_t *coredev;
	struct list_head txqueue;
	char *txbuf;
	dma_addr_t txbuf_phy_addr;
};

struct _smsspi_txmsg {
	struct list_head node;	/*! internal management */
	void *buffer;
	size_t size;
	int alignment;
	int add_preamble;
	struct completion completion;
	void (*prewrite) (void *);
	void (*postwrite) (void *);
};

struct _Msg {
	struct SmsMsgHdr_ST hdr;
	u32 data[3];
};

struct _spi_device_st *spi_dev;

#ifdef CONFIG_SMP
static void spi_worker_thread(struct work_struct *work);
#else
static void spi_worker_thread(void *arg);
static DECLARE_WORK(spi_work_queue, (void *)spi_worker_thread);
#endif
static u8 smsspi_preamble[] = { 0xa5, 0x5a, 0xe7, 0x7e };
static u8 smsspi_startup[] = { 0, 0, 0xde, 0xc1, 0xa5, 0x51, 0xf1, 0xed };
static u32 default_type = SMS_NOVA_B0;
static u32 intr_pin = SMS_INTR_PIN;
//static u32 intCount = 0;     not used

module_param(default_type, int, 0644);
MODULE_PARM_DESC(default_type, "default board type.");

module_param(intr_pin, int, 0644);
MODULE_PARM_DESC(intr_pin, "interrupt pin number.");

/******************************************/
#ifdef CONFIG_SMP
static void spi_worker_thread(struct work_struct *work)
#else
static void spi_worker_thread(void *arg)
#endif
{
	struct _spi_device_st *spi_device = spi_dev;
	struct _smsspi_txmsg *msg = NULL;
	struct _spi_msg txmsg;

	//sms_info("worker start");
	do {
		/* do we have a msg to write ? */
		if (!msg && !list_empty(&spi_device->txqueue))                //ÅÐ¶ÏÁ´±íÊÇ·ñÎª¿Õ
			msg = (struct _smsspi_txmsg *)
					list_entry(spi_device->txqueue.
					next, struct _smsspi_txmsg, node);

		if (msg) {
			if (msg->add_preamble)
			{
				txmsg.len =
				    min(msg->size + sizeof(smsspi_preamble),
					(size_t) TX_BUFFER_SIZE);
				txmsg.buf = spi_device->txbuf;
				txmsg.buf_phy_addr = spi_device->txbuf_phy_addr;
				memcpy(txmsg.buf, smsspi_preamble,
				       sizeof(smsspi_preamble));
				memcpy(&txmsg.buf[sizeof(smsspi_preamble)],
				       msg->buffer,
				       txmsg.len - sizeof(smsspi_preamble));
				msg->add_preamble = 0;
				msg->buffer +=
				    txmsg.len - sizeof(smsspi_preamble);
				msg->size -=
				    txmsg.len - sizeof(smsspi_preamble);
				/* zero out the rest of aligned buffer */
				memset(&txmsg.buf[txmsg.len], 0,
				       TX_BUFFER_SIZE - txmsg.len);
				smsspi_common_transfer_msg(&spi_device->dev,
							   &txmsg, 1);
			} else {
				txmsg.len =
				    min(msg->size, (size_t) TX_BUFFER_SIZE);
				txmsg.buf = spi_device->txbuf;
				txmsg.buf_phy_addr = spi_device->txbuf_phy_addr;
				memcpy(txmsg.buf, msg->buffer, txmsg.len);

				msg->buffer += txmsg.len;
				msg->size -= txmsg.len;
				/* zero out the rest of aligned buffer */
				memset(&txmsg.buf[txmsg.len], 0,
				       TX_BUFFER_SIZE - txmsg.len);
				smsspi_common_transfer_msg(&spi_device->dev,
							   &txmsg, 0);
			}

		} else {
			smsspi_common_transfer_msg(&spi_device->dev, NULL, 1);
		}

		/* if there was write, have we finished ? */
		if (msg && !msg->size) {
			/* call postwrite call back */
			if (msg->postwrite)
				msg->postwrite(spi_device);

			list_del(&msg->node);
			complete(&msg->completion);
			msg = NULL;
		}
		/* if there was read, did we read anything ? */

	} while (!list_empty(&spi_device->txqueue) || msg);

	//sms_info("worker end");

}

static void msg_found(void *context, void *buf, int offset, int len)
{
	struct _spi_device_st *spi_device = (struct _spi_device_st *) context;
	struct smscore_buffer_t *cb =
	    (struct smscore_buffer_t
	     *)(container_of(buf, struct smscore_buffer_t, p));

	PDEBUG("entering\n");
	cb->offset = offset;
	cb->size = len;
	/* PERROR ("buffer %p is sent back to core databuf=%p,
		offset=%d.\n", cb, cb->p, cb->offset); */
	smscore_onresponse(spi_device->coredev, cb);

	PDEBUG("exiting\n");

}

//static void smsspi_int_handler(void *context)                
static void smsspi_int_handler(void )                
{
	//struct _spi_device_st *spi_device = (struct _spi_device_st *) context;
//	PDEBUG("interrupt\n");
//	sms_info("**Interrupt %d", intCount++);
#ifdef CONFIG_SMP
	queue_work(spi_thread_wq, &spi_thread_work);
#else
	PREPARE_WORK(&spi_work_queue, (void *)spi_worker_thread);    
	schedule_work(&spi_work_queue);
#endif
#ifdef MTK_SPI                                                      
	 mt65xx_eint_unmask(CUST_EINT_CMMB_NUM);                  
#endif
}

static int smsspi_queue_message_and_wait(struct _spi_device_st *spi_device,
					 struct _smsspi_txmsg *msg)
{
	init_completion(&msg->completion);                       
	list_add_tail(&msg->node, &spi_device->txqueue);
#ifdef CONFIG_SMP
	queue_work(spi_thread_wq, &spi_thread_work);
#else
	schedule_work(&spi_work_queue);
#endif
	wait_for_completion(&msg->completion);

	return 0;
}
#if 0                
struct _smsspi_txmsg *msg=NULL;
struct _Msg *Msg=NULL;
#endif 	
static int smsspi_preload(void *context)
{
#if 1
	struct _smsspi_txmsg msg;
	struct _spi_device_st *spi_device = (struct _spi_device_st *) context;
/*	struct _Msg Msg = {
		{
		MSG_SMS_SPI_INT_LINE_SET_REQ, 0, HIF_TASK,
			sizeof(struct _Msg), 0}, {
		0, intr_pin, 0}
	};*/
		struct _Msg Msg = {                                
		{
		MSG_SMS_SPI_INT_LINE_SET_REQ, 151, HIF_TASK,
			sizeof(struct _Msg), 0}, {
		0, 4, 400}
	};
		
	
	int rc;

	prepareForFWDnl(spi_device->phy_dev);
	sms_info("Sending SPI init sequence");
	msg.buffer = smsspi_startup;
	msg.size = sizeof(smsspi_startup);
	msg.alignment = 4;
	msg.add_preamble = 0;
	msg.prewrite = NULL;	/* smsspiphy_reduce_clock; */
	msg.postwrite = NULL;   /* smsspiphy_restore_clock; */

	rc = smsspi_queue_message_and_wait(context, &msg);
	if (rc < 0) {
		sms_err("smsspi_queue_message_and_wait error, rc = %d\n", rc);
		return rc;
	}

	sms_debug("sending MSG_SMS_SPI_INT_LINE_SET_REQ");
	msg.buffer = &Msg;
	msg.size = sizeof(Msg);
	msg.alignment = SPI_PACKET_SIZE;
	msg.add_preamble = 1;

	rc = smsspi_queue_message_and_wait(context, &msg);
	if (rc < 0) {
		sms_err("set interrupt line failed, rc = %d\n", rc);
		return rc;
	}
#else
	void* buf_1=(void*) SMSDev->tx_buf;
	void* buf_2 =(void*)SMSDev->rx_buf ;
	
	struct _spi_device_st *spi_device = (struct _spi_device_st *) context;
	int rc;

	//buf_1 = kmalloc(sizeof(struct _smsspi_txmsg) + SMS_DMA_ALIGNMENT, GFP_KERNEL | GFP_DMA);
	//buf_2 = kmalloc(sizeof(struct _Msg) + SMS_DMA_ALIGNMENT, GFP_KERNEL | GFP_DMA);

	if((buf_1) &&(buf_2))
	{
		msg = (struct _smsspi_txmsg *)SMS_ALIGN_ADDRESS(buf_1);
		Msg = (struct _Msg *)SMS_ALIGN_ADDRESS(buf_2);
		Msg->hdr.msgType = MSG_SMS_SPI_INT_LINE_SET_REQ;
		Msg->hdr.msgSrcId = 151;
		Msg->hdr.msgDstId = HIF_TASK;
		Msg->hdr.msgLength = sizeof(struct _Msg);
		Msg->hdr.msgFlags = 0;
		Msg->data[0] = 0;
		Msg->data[1] = 4;
		Msg->data[2] = 400;

		prepareForFWDnl(spi_device->phy_dev);
		sms_info("Sending SPI init sequence");
		msg->buffer = smsspi_startup;
		msg->size = sizeof(smsspi_startup);
		msg->alignment = 4;
		msg->add_preamble = 0;
		msg->prewrite = NULL;	/* smsspiphy_reduce_clock; */
		msg->postwrite = NULL;   /* smsspiphy_restore_clock; */
		
              sms_log("**msg=%4x",msg);
		rc = smsspi_queue_message_and_wait(context, msg);
		
		if (rc < 0) {
			sms_err("smsspi_queue_message_and_wait error, rc = %d\n", rc);
			return rc;
		}

		sms_debug("sending MSG_SMS_SPI_INT_LINE_SET_REQ");
		sms_info("Sending SPI Set Interrupt command sequence");
		msg->buffer = Msg;
		msg->size = sizeof(struct _Msg);
		msg->alignment = SPI_PACKET_SIZE;
		msg->add_preamble = 1;
              sms_log("**msg=%4x",msg);
		rc = smsspi_queue_message_and_wait(context, msg);
		
		if (rc < 0) {
			sms_err("set interrupt line failed, rc = %d\n", rc);
			return rc;
		}
	}

	//kfree(buf_1);
	//kfree(buf_2);
#endif
	return rc;
}

static int smsspi_postload(void *context)
{
	struct _spi_device_st *spi_device = (struct _spi_device_st *) context;
	int mode = smscore_registry_getmode(spi_device->coredev->devpath);
	if ( (mode != DEVICE_MODE_ISDBT) &&
	     (mode != DEVICE_MODE_ISDBT_BDA) ) {
		fwDnlComplete(spi_device->phy_dev, 0);
		
	}
	
	return 0;
}

static int smsspi_write(void *context, void *txbuf, size_t len)
{
	struct _smsspi_txmsg msg;
	msg.buffer = txbuf;
	msg.size = len;
	msg.prewrite = NULL;
	msg.postwrite = NULL;
	//int msgLen, i;     not used
	if (len > 0x1000) {
		/* The FW is the only long message. Do not add preamble,
		and do not padd it */
		msg.alignment = 4;
		msg.add_preamble = 0;
		msg.prewrite = smschipreset;
	} else {
		msg.alignment = SPI_PACKET_SIZE;
		msg.add_preamble = 1;
	}

	sms_info("Send msg type: %d", (((u8 *) txbuf)[1]<<8) + ((u8 *) txbuf)[0]);
	
	return smsspi_queue_message_and_wait(context, &msg);
}

struct _rx_buffer_st *allocate_rx_buf(void *context, int size)
{
	struct smscore_buffer_t *buf;
	struct _spi_device_st *spi_device = (struct _spi_device_st *) context;
	if (size > RX_BUFFER_SIZE) {
		PERROR("Requested size is bigger than max buffer size.\n");
		return NULL;
	}
	buf = smscore_getbuffer(spi_device->coredev);
	PDEBUG("Recieved Rx buf %p physical 0x%x (contained in %p)\n", buf->p,
	       buf->phys, buf);

	flush_cache_all();

	/* note: this is not mistake! the rx_buffer_st is identical to part of
	   smscore_buffer_t and we return the address of the start of the
	   identical part */
	return (struct _rx_buffer_st *) &buf->p;
}

static void free_rx_buf(void *context, struct _rx_buffer_st *buf)
{
	struct _spi_device_st *spi_device = (struct _spi_device_st *) context;
	struct smscore_buffer_t *cb =
	    (struct smscore_buffer_t
	     *)(container_of(((void *)buf), struct smscore_buffer_t, p));
	PDEBUG("buffer %p is released.\n", cb);
	smscore_putbuffer(spi_device->coredev, cb);
}

/*! Release device STUB

\param[in]	dev:		device control block
\return		void
*/
static void smsspi_release(struct device *dev)
{
	PDEBUG("nothing to do\n");
	/* Nothing to release */
}

static struct platform_device smsspi_device = {
	.name = "smsspi",
	.id = 1,
	.dev = {
		.release = smsspi_release,
		},
};


int smsspi_register(void)
{
	struct smsdevice_params_t params;
	int ret;
	struct _spi_device_st *spi_device;
	struct _spi_dev_cb_st common_cb;

	PDEBUG("entering \n");
#ifdef CONFIG_SMP
	sms_info("create a signelthread workqueue");
        spi_thread_wq = create_singlethread_workqueue("siano work wq");
	INIT_WORK(&spi_thread_work, spi_worker_thread);
#endif
	spi_device =
	    kmalloc(sizeof(struct _spi_device_st), GFP_KERNEL);
	spi_dev = spi_device;

	INIT_LIST_HEAD(&spi_device->txqueue);

	ret = platform_device_register(&smsspi_device);                     
	if (ret < 0) {
		PERROR("platform_device_register failed\n");
		return ret;
	}
	

	//

#if 0                     //buf issue   1214                    
	spi_device->txbuf =
	    dma_alloc_coherent(NULL, TX_BUFFER_SIZE,
			       &spi_device->txbuf_phy_addr,
			       GFP_KERNEL | GFP_DMA);
#else
	spi_device->txbuf =kmalloc(TX_BUFFER_SIZE, GFP_KERNEL);
       spi_device->txbuf_phy_addr = (u32)spi_device->txbuf;                 //? check txbuf_phy_addr is use or not
       printk("**** spi_dev->txbuf = 0x %4x",(unsigned int)spi_dev->txbuf);
#endif
	if (!spi_device->txbuf) {
		printk(KERN_INFO "%s dma_alloc_coherent(...) failed\n",
		       __func__);
		ret = -ENOMEM;
		goto txbuf_error;
	}

	spi_device->phy_dev =
	    smsspiphy_init(NULL, smsspi_int_handler, (void*)spi_device);
	if (spi_device->phy_dev == 0) {
		printk(KERN_INFO "%s smsspiphy_init(...) failed\n", __func__);
		//goto phy_error;                                                         
	}

	common_cb.allocate_rx_buf = allocate_rx_buf;
	common_cb.free_rx_buf = free_rx_buf;
	common_cb.msg_found_cb = msg_found;
	common_cb.transfer_data_cb = smsspibus_xfer_mix;//smsspibus_xfer; //yufeng modified for 6589 SPI issue

	ret =
	    smsspicommon_init(&spi_device->dev, spi_device, spi_device->phy_dev,
			      &common_cb);
	if (ret) {
		printk(KERN_INFO "%s smsspiphy_init(...) failed\n", __func__);
		goto common_error;
	}

	/* register in smscore */
	memset(&params, 0, sizeof(params));
	params.context = spi_device;
	params.device = &smsspi_device.dev;
	params.buffer_size = RX_BUFFER_SIZE;
	params.num_buffers = NUM_RX_BUFFERS;
	params.flags = SMS_DEVICE_NOT_READY;
	params.sendrequest_handler = smsspi_write;
	strcpy(params.devpath, "spi");
	params.device_type = default_type;

	if (0) {
		/* device family */
		/* params.setmode_handler = smsspi_setmode; */
	} else {
		params.flags =
		    SMS_DEVICE_FAMILY2 | SMS_DEVICE_NOT_READY;
		params.preload_handler = smsspi_preload;
		params.postload_handler = smsspi_postload;
	}

	ret = smscore_register_device(&params, &spi_device->coredev);
	if (ret < 0) {
		printk(KERN_INFO "%s smscore_register_device(...) failed\n",
		       __func__);
		goto reg_device_error;
	}

	ret = smscore_start_device(spi_device->coredev);
	if (ret < 0) {
		printk(KERN_INFO "%s smscore_start_device(...) failed\n",
		       __func__);
		goto start_device_error;
	}

	PDEBUG("exiting\n");
	return 0;

start_device_error:
	smscore_unregister_device(spi_device->coredev);

reg_device_error:

common_error:
	smsspiphy_deinit(spi_device->phy_dev);

#if 0 // not used
phy_error:
	#if 0                     //buf issue   1214                    
	/*dma_free_coherent(NULL, TX_BUFFER_SIZE, spi_device->txbuf,
			  spi_device->txbuf_phy_addr);*/
	#else
        kfree(spi_device->txbuf);
	#endif
#endif

txbuf_error:
	platform_device_unregister(&smsspi_device);

	PDEBUG("exiting error %d\n", ret);

	return ret;
}

void smsspi_unregister(void)
{
	struct _spi_device_st *spi_device = spi_dev;
	PDEBUG("entering\n");

	/* stop interrupts */
	smsspiphy_deinit(spi_device->phy_dev);
	smscore_unregister_device(spi_device->coredev);
#if 0                     //buf issue   1214    
	dma_free_coherent(NULL, TX_BUFFER_SIZE, spi_device->txbuf,
			  spi_device->txbuf_phy_addr);
#else
        kfree(spi_device->txbuf);
#endif
#ifdef CONFIG_SMP
	destroy_workqueue(spi_thread_wq);
#endif
	platform_device_unregister(&smsspi_device);
	PDEBUG("exiting\n");
}
