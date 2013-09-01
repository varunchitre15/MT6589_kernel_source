/*
 * MUSB OTG driver peripheral support
 *
 * Copyright 2005 Mentor Graphics Corporation
 * Copyright (C) 2005-2006 by Texas Instruments
 * Copyright (C) 2006-2007 Nokia Corporation
 * Copyright (C) 2009 MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>


#include <linux/musb/musb_core.h>
#define FIFO_START_ADDR 512
extern struct musb_hw_ep	*ep_in_list[MUSB_C_NUM_EPS];
extern struct musb_hw_ep	*ep_out_list[MUSB_C_NUM_EPS];

//#define is_buffer_mapped(req) (req->map_state != UN_MAPPED)
//Added for USB Develpment debug, more log for more debuging help

/*
#ifdef DBG
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

/* Maps the buffer to dma  */

/*static inline void map_dma_buffer(struct musb_request *request,
			struct musb *musb, struct musb_ep *musb_ep)
{
	int compatible = true;
	struct dma_controller *dma = musb->dma_controller;

	printk("map_dma_buffer called!\n");

	request->map_state = UN_MAPPED;

	if (!musb_ep->dma){
		request->map_state = UN_MAPPED;
		return;
	}*/
	/* Check if DMA engine can handle this request.
	 * DMA code must reject the USB request explicitly.
	 * Default behaviour is to map the request.
	 */
	/*if (dma->is_compatible)
		compatible = dma->is_compatible(musb_ep->dma,
				musb_ep->packet_sz, request->request.buf,
				request->request.length);
	if (!compatible)
		return;

	if (request->request.dma == DMA_ADDR_INVALID) {
		request->request.dma = dma_map_single(
				musb->controller,
				request->request.buf,
				request->request.length,
				request->tx
					? DMA_TO_DEVICE
					: DMA_FROM_DEVICE);
		request->map_state = MUSB_MAPPED;
	} else {
		dma_sync_single_for_device(musb->controller,
			request->request.dma,
			request->request.length,
			request->tx
				? DMA_TO_DEVICE
				: DMA_FROM_DEVICE);
		request->map_state = PRE_MAPPED;
	}
}*/

/* Unmap the buffer from dma and maps it back to cpu */
/*static inline void unmap_dma_buffer(struct musb_request *request,
				struct musb *musb)
{
	if (!is_buffer_mapped(request))
		return;

	if (request->request.dma == DMA_ADDR_INVALID) {
		DBG(3,"not unmapping a never mapped buffer\n");
		return;
	}
	if (request->map_state == MUSB_MAPPED) {
		dma_unmap_single(musb->controller,
			request->request.dma,
			request->request.length,
			request->tx
				? DMA_TO_DEVICE
				: DMA_FROM_DEVICE);
		request->request.dma = DMA_ADDR_INVALID;
	} else {*/ /* PRE_MAPPED */
		/*dma_sync_single_for_cpu(musb->controller,
			request->request.dma,
			request->request.length,
			request->tx
				? DMA_TO_DEVICE
				: DMA_FROM_DEVICE);
	}
	request->map_state = UN_MAPPED;
}*/

//Added for USB Develpment debug, more log for more debuging help
/*#ifdef DBG
#undef DBG
#endif

#define DBG(level, fmt, args...) \
	do { \
			pr_info( fmt, ##args); \
	} while (0)
*/
//Added for USB Develpment debug, more log for more debuging help


static void fifo_setup(struct musb *musb, struct musb_ep *musb_ep)
{
	void __iomem	*mbase = musb->mregs;
	int	size = 0;
	u16	maxpacket = musb_ep->fifo_size;
	u16	c_off = musb->fifo_addr >> 3;
	u8	c_size;

	/* expect hw_ep has already been zero-initialized */

	size = ffs(max(maxpacket, (u16) 8)) - 1;
	maxpacket = 1 << size;

	c_size = size - 3;
	if (musb_ep->fifo_mode == BUF_DOUBLE) {
		if ((musb->fifo_addr + (maxpacket << 1)) >(musb->fifo_size))
			return ;

		c_size |= MUSB_FIFOSZ_DPB;

	}else if ((musb->fifo_addr + maxpacket) >(musb->fifo_size))
		return ;

	/* configure the FIFO */
//	musb_writeb(mbase, MUSB_INDEX, hw_ep->epnum);
	DBG(2,"fifo size is %d after %d, fifo address is %d\n",c_size,maxpacket,musb->fifo_addr);

	if (musb_ep->is_in) {
		musb_write_txfifosz(mbase, c_size);
		musb_write_txfifoadd(mbase, c_off);
	} else {
		musb_write_rxfifosz(mbase, c_size);
		musb_write_rxfifoadd(mbase, c_off);
	}
	musb->fifo_addr += (maxpacket << ((c_size & MUSB_FIFOSZ_DPB) ? 1 : 0));
	return ;
}


/* MUSB PERIPHERAL status 3-mar-2006:
 *
 * - EP0 seems solid.  It passes both USBCV and usbtest control cases.
 *   Minor glitches:
 *
 *     + remote wakeup to Linux hosts work, but saw USBCV failures;
 *       in one test run (operator error?)
 *     + endpoint halt tests -- in both usbtest and usbcv -- seem
 *       to break when dma is enabled ... is something wrongly
 *       clearing SENDSTALL?
 *
 * - Mass storage behaved ok when last tested.  Network traffic patterns
 *   (with lots of short transfers etc) need retesting; they turn up the
 *   worst cases of the DMA, since short packets are typical but are not
 *   required.
 *
 * - TX/IN
 *     + both pio and dma behave in with network and g_zero tests
 *     + no cppi throughput issues other than no-hw-queueing
 *     + failed with FLAT_REG (DaVinci)
 *     + seems to behave with double buffering, PIO -and- CPPI
 *     + with gadgetfs + AIO, requests got lost?
 *
 * - RX/OUT
 *     + both pio and dma behave in with network and g_zero tests
 *     + dma is slow in typical case (short_not_ok is clear)
 *     + double buffering ok with PIO
 *     + double buffering *FAILS* with CPPI, wrong data bytes sometimes
 *     + request lossage observed with gadgetfs
 *
 * - ISO not tested ... might work, but only weakly isochronous
 *
 * - Gadget driver disabling of softconnect during bind() is ignored; so
 *   drivers can't hold off host requests until userspace is ready.
 *   (Workaround:  they can turn it off later.)
 *
 * - PORTABILITY (assumes PIO works):
 *     + DaVinci, basically works with cppi dma
 *     + OMAP 2430, ditto with mentor dma
 *     + TUSB 6010, platform-specific dma in the works
 */

/* ----------------------------------------------------------------------- */

/*
 * Immediately complete a request.
 *
 * @param request the request to complete
 * @param status the status to complete the request with
 * Context: controller locked, IRQs blocked.
 */


/*u32 req_count = 0;
module_param(req_count,int,0644);
u32 dereq_count = 0;
module_param(dereq_count,int,0644);
*/

void musb_g_giveback(
	struct musb_ep		*ep,
	struct usb_request	*request,
	int			status)
__releases(ep->musb->lock)
__acquires(ep->musb->lock)
{
	struct musb_request	*req;
	struct musb		*musb;
	int			busy = ep->busy;

	req = to_musb_request(request);
	//list_del(&request->list);
	list_del(&req->list);

	if (req->request.status == -EINPROGRESS)
		req->request.status = status;
	musb = req->musb;

	ep->busy = 1;
	spin_unlock(&musb->lock);
	if (ep->dma) {
		if (req->map_state) {
			dma_unmap_single(musb->controller,
					req->request.dma,
					req->request.length,
					req->tx
						? DMA_TO_DEVICE
						: DMA_FROM_DEVICE);
			req->request.dma = DMA_ADDR_INVALID;
			req->map_state = UN_MAPPED;
		} else if (req->request.dma != DMA_ADDR_INVALID)
			dma_sync_single_for_cpu(musb->controller,
					req->request.dma,
					req->request.length,
					req->tx
						? DMA_TO_DEVICE
						: DMA_FROM_DEVICE);
	}
	if (request->status == 0)
		DBG(1, "%s done request %p,  %d/%d\n",
				ep->end_point.name, request,
				req->request.actual, req->request.length);
	else
		DBG(1, "%s request %p, %d/%d fault %d\n",
				ep->end_point.name, request,
				req->request.actual, req->request.length,
				request->status);
	req->request.complete(&req->ep->end_point, &req->request);
/*	if(req->ep->current_epnum == 3 && !req->ep->is_in)
		dereq_count++;*/
	spin_lock(&musb->lock);
	DBG(50,"MUSB:%s request done\n",req->ep->end_point.name);
	ep->busy = busy;
}

/* ----------------------------------------------------------------------- */

/*
 * Abort requests queued to an endpoint using the status. Synchronous.
 * caller locked controller and blocked irqs, and selected this ep.
 */
static void nuke(struct musb_ep *ep, const int status)
{
	struct musb_request	*req = NULL;
	void __iomem *epio = ep->musb->endpoints[ep->current_epnum].regs;

	ep->busy = 1;

	if (ep->dma) {
		struct dma_controller	*c = ep->musb->dma_controller;
		int value;

		if (ep->is_in) {
			/*
			 * The programming guide says that we must not clear
			 * the DMAMODE bit before DMAENAB, so we only
			 * clear it in the second write...
			 */
			dumpTime(funcWritew, ep->current_epnum);
			musb_writew(epio, MUSB_TXCSR,
				    MUSB_TXCSR_DMAMODE | MUSB_TXCSR_FLUSHFIFO);
			dumpTime(funcWritew, ep->current_epnum);
			musb_writew(epio, MUSB_TXCSR,
					0 | MUSB_TXCSR_FLUSHFIFO);
		} else {
			dumpTime(funcWritew, ep->current_epnum);
			musb_writew(epio, MUSB_RXCSR,
					0 | MUSB_RXCSR_FLUSHFIFO);
			dumpTime(funcWritew, ep->current_epnum);
			musb_writew(epio, MUSB_RXCSR,
					0 | MUSB_RXCSR_FLUSHFIFO);
		}

		value = c->channel_abort(ep->dma);
		DBG(value ? 1 : 3, "%s: %s: abort DMA --> %d\n", __func__, ep->name, value);
		c->channel_release(ep->dma);
		ep->dma = NULL;
	}

	while (!list_empty(&(ep->req_list))) {
		//req = container_of(ep->req_list.next, struct musb_request,
		//		request.list);
		req = list_first_entry(&ep->req_list, struct musb_request, list);
		musb_g_giveback(ep, &req->request, status);
		DBG(0,"call musb_g_giveback on function %s ep is %s\n", __func__,ep->end_point.name);
	}
}

//Added Modification for ALPS00255822, bug from WHQL test
void musb_g_giveback_keepReq(
	struct musb_ep		*ep,
	struct usb_request	*request,
	int			status)
__releases(ep->musb->lock)
__acquires(ep->musb->lock)
{
	struct musb_request	*req;
	struct musb		*musb;
	int			busy = ep->busy;


	req = to_musb_request(request);
	list_del(&req->list);

	DBG(0,"%s: request = 0x%p\n", __func__, request);

	if (req->request.status == -EINPROGRESS)
		req->request.status = status;
	musb = req->musb;

	ep->busy = 1;
	spin_unlock(&musb->lock);
	if (ep->dma) {
		if (req->map_state) {
			dma_unmap_single(musb->controller,
					req->request.dma,
					req->request.length,
					req->tx
						? DMA_TO_DEVICE
						: DMA_FROM_DEVICE);
			req->request.dma = DMA_ADDR_INVALID;
			req->map_state = UN_MAPPED;
		} else if (req->request.dma != DMA_ADDR_INVALID)
			dma_sync_single_for_cpu(musb->controller,
					req->request.dma,
					req->request.length,
					req->tx
						? DMA_TO_DEVICE
						: DMA_FROM_DEVICE);
	}
	if (request->status == 0)
		DBG(1, "%s: %s done request %p,  %d/%d\n",
				__func__, ep->end_point.name, request,
				req->request.actual, req->request.length);
	else
		DBG(1, "%s: %s request %p, %d/%d fault %d\n",
				__func__, ep->end_point.name, request,
				req->request.actual, req->request.length,
				request->status);
	req->request.complete(&req->ep->end_point, &req->request);
/*	if(req->ep->current_epnum == 3 && !req->ep->is_in)
		dereq_count++;*/
	spin_lock(&musb->lock);
	DBG(50,"%s: MUSB:%s request done\n",__func__, req->ep->end_point.name);
	ep->busy = busy;
}

static void nuke_without_delReq(struct musb_ep *ep, const int status)
{
	struct musb_request	*req = NULL;
	void __iomem *epio = ep->musb->endpoints[ep->current_epnum].regs;

	ep->busy = 1;

	ep->dmaRelease = 1;

	DBG(1, "%s: ep->current_epnum = %d, \n", __func__, ep->current_epnum);

	if (ep->dma) {
		struct dma_controller	*c = ep->musb->dma_controller;
		int value;

		if (ep->is_in) {
			/*
			 * The programming guide says that we must not clear
			 * the DMAMODE bit before DMAENAB, so we only
			 * clear it in the second write...
			 */
			dumpTime(funcWritew, ep->current_epnum);
			musb_writew(epio, MUSB_TXCSR,
				    MUSB_TXCSR_DMAMODE | MUSB_TXCSR_FLUSHFIFO);
			dumpTime(funcWritew, ep->current_epnum);
			musb_writew(epio, MUSB_TXCSR,
					0 | MUSB_TXCSR_FLUSHFIFO);
		} else {
			dumpTime(funcWritew, ep->current_epnum);
			musb_writew(epio, MUSB_RXCSR,
					0 | MUSB_RXCSR_FLUSHFIFO);
			dumpTime(funcWritew, ep->current_epnum);
			musb_writew(epio, MUSB_RXCSR,
					0 | MUSB_RXCSR_FLUSHFIFO);
		}

		value = c->channel_abort(ep->dma);
		DBG(value ? 1 : 3, "%s: %s: abort DMA --> %d\n", __func__, ep->name, value);
		c->channel_release(ep->dma);
		ep->dma = NULL;
	}

	while (!list_empty(&(ep->req_list)))
	{
		//req = container_of(ep->req_list.next, struct musb_request,
		//		request.list);
		req = list_first_entry(&ep->req_list, struct musb_request, list);
		musb_g_giveback_keepReq(ep, &req->request, status);
		DBG(0,"%s: call musb_g_giveback on function %s ep is %s\n", __func__, __func__,ep->end_point.name);
		//Ainge
		//DBG(1, "%s: call musb_g_giveback on function %s ep is %s\n", __func__, __func__,ep->end_point.name);
		//Ainge
	}
}
//Added Modification for ALPS00255822, bug from WHQL test
/* ----------------------------------------------------------------------- */

/* Data transfers - pure PIO, pure DMA, or mixed mode */

/*
 * This assumes the separate CPPI engine is responding to DMA requests
 * from the usb core ... sequenced a bit differently from mentor dma.
 */

static inline int max_ep_writesize(struct musb *musb, struct musb_ep *ep)
{
	if (can_bulk_split(musb, ep->type))
		return ep->hw_ep->max_packet_sz_tx;
	else
		return ep->packet_sz;
}


/* Peripheral tx (IN) using Mentor DMA works as follows:
	Only mode 0 is used for transfers <= wPktSize,
	mode 1 is used for larger transfers,

	One of the following happens:
	- Host sends IN token which causes an endpoint interrupt
		-> TxAvail
			-> if DMA is currently busy, exit.
			-> if queue is non-empty, txstate().

	- Request is queued by the gadget driver.
		-> if queue was previously empty, txstate()

	txstate()
		-> start
		  /\	-> setup DMA
		  |     (data is transferred to the FIFO, then sent out when
		  |	IN token(s) are recd from Host.
		  |		-> DMA interrupt on completion
		  |		   calls TxAvail.
		  |		      -> stop DMA, ~DMAENAB,
		  |		      -> set TxPktRdy for last short pkt or zlp
		  |		      -> Complete Request
		  |		      -> Continue next request (call txstate)
		  |___________________________________|

 * Non-Mentor DMA engines can of course work differently, such as by
 * upleveling from irq-per-packet to irq-per-buffer.
 */

/*
 * An endpoint is transmitting data. This can be called either from
 * the IRQ routine or from ep.queue() to kickstart a request on an
 * endpoint.
 *
 * Context: controller locked, IRQs blocked, endpoint selected
 */
#define TX_DMA_MODE1
#ifdef TX_DMA_MODE1
static void txstate(struct musb *musb, struct musb_request *req)
{
	u8			epnum = req->epnum;
	struct musb_ep		*musb_ep;
	//void __iomem		*epio = musb->endpoints[epnum].regs;
	void __iomem		*epio =NULL;
	struct usb_request	*request;
	u16			fifo_count = 0, csr;
	int			use_dma = 0;
	size_t request_size;

	if (ep_in_list[epnum]) {
		epio = ep_in_list[epnum]->regs;
	} else {
		DBG(0,"ep%d in list is NULL\n",epnum);
	}
	musb_ep = req->ep;

	/* Check if EP is disabled */
	if (!musb_ep->desc) {
		DBG(0, "ep:%s disabled - ignore request\n",
			musb_ep->end_point.name);
		return;
	}

	/* we shouldn't get here while DMA is active ... but we do ... */
	if (dma_channel_status(musb_ep->dma) == MUSB_DMA_STATUS_BUSY) {
		DBG(0, "dma pending...\n");
		return;
	}

	/* read TXCSR before */
	csr = musb_readw(epio, MUSB_TXCSR);

	request = &req->request;
	fifo_count = min(max_ep_writesize(musb, musb_ep),
			(int)(request->length - request->actual));

	if (csr & MUSB_TXCSR_TXPKTRDY) {
		DBG(1, "%s old packet still ready , txcsr %03x\n",
				musb_ep->end_point.name, csr);
		return;
	}

	if (csr & MUSB_TXCSR_P_SENDSTALL) {
		DBG(0, "%s stalling, txcsr %03x\n",
				musb_ep->end_point.name, csr);
		return;
	}

	DBG(3, "hw_ep%d, maxpacket %d, fifo count %d, txcsr %03x\n",
			epnum, musb_ep->packet_sz, fifo_count,
			csr);

	USB_LOGGER(TXSTATE, TXSTATE, epnum, musb_ep->packet_sz, fifo_count, csr);

	if (musb_ep->dma) {
		struct dma_controller	*c = musb->dma_controller;
		use_dma = (request->dma != DMA_ADDR_INVALID);
		/* MUSB_TXCSR_P_ISO is still set correctly */

		/* setup DMA, then program endpoint CSR */
		request_size = min_t(size_t, request->length,
					musb_ep->dma->max_len);
		if (request_size < musb_ep->packet_sz)
			musb_ep->dma->desired_mode = 0;
		else
			musb_ep->dma->desired_mode = 1;


		use_dma = use_dma && c->channel_program(
				musb_ep->dma, musb_ep->packet_sz,
				musb_ep->dma->desired_mode,
				request->dma + request->actual, request_size);

		if (use_dma) {
			musb_ep->dma_configed = TRUE;
			if (musb_ep->dma->desired_mode == 0) {
				csr &= ~(MUSB_TXCSR_AUTOSET
					| MUSB_TXCSR_DMAENAB);
				dumpTime(funcWritew, epnum);
				musb_writew(epio, MUSB_TXCSR, csr
					| MUSB_TXCSR_P_WZC_BITS);
					csr &= ~MUSB_TXCSR_DMAMODE;
					csr |= (MUSB_TXCSR_DMAENAB/*	 |
						MUSB_TXCSR_MODE*/);
			} else
				csr |= (MUSB_TXCSR_AUTOSET
						| MUSB_TXCSR_DMAENAB
						| MUSB_TXCSR_DMAMODE
						/*| MUSB_TXCSR_MODE*/);

			csr &= ~MUSB_TXCSR_P_UNDERRUN;
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_TXCSR, csr);
		}
	}


	if (!use_dma) {
		musb_write_fifo(musb_ep->hw_ep, fifo_count,
				(u8 *) (request->buf + request->actual));
		request->actual += fifo_count;
		csr |= MUSB_TXCSR_TXPKTRDY;
		csr &= ~MUSB_TXCSR_P_UNDERRUN;
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_TXCSR, csr);
	}

	/* host may already have the data when this message shows... */
	DBG(3, "%s: %s TX/IN %s len %d/%d, txcsr %04x, fifo %d/%d\n",
			__func__, musb_ep->end_point.name, use_dma ? "dma" : "pio",
			request->actual, request->length,
			musb_readw(epio, MUSB_TXCSR),
			fifo_count,
			musb_readw(epio, MUSB_TXMAXP));

	USB_LOGGER( TXSTATE_END, TXSTATE, musb_ep->end_point.name, use_dma ? "dma" : "pio",
			request->actual, request->length, musb_readw(epio, MUSB_TXCSR), fifo_count,
			musb_readw(epio, MUSB_TXMAXP));

}

/*
 * FIFO state update (e.g. data ready).
 * Called from IRQ,  with controller locked.
 */
void musb_g_tx(struct musb *musb, u8 epnum)
{
	u16			csr;
	struct musb_request	*req;
	struct usb_request	*request;
	u8 __iomem		*mbase = musb->mregs;
//	struct musb_ep		*musb_ep = &musb->endpoints[epnum].ep_in;
//	void __iomem		*epio = musb->endpoints[epnum].regs;
	struct musb_ep		*musb_ep = NULL;
	void __iomem		*epio = NULL;
	struct dma_channel	*dma;

	if (ep_in_list[epnum]) {
		musb_ep = &ep_in_list[epnum]->ep_in;
		epio = ep_in_list[epnum]->regs;
	} else {
		DBG(0,"%s: ep%d in list is NULL\n", __func__, epnum);
	}

	musb_ep_select(mbase, epnum);
	//request = next_request(musb_ep);
	req = next_request(musb_ep);
	request = &req->request;

	csr = musb_readw(epio, MUSB_TXCSR);
	DBG(3, "<== %s, txcsr %04x\n", musb_ep->end_point.name, csr);

	USB_LOGGER(MUSB_G_TX, MUSB_G_TX, musb_ep->end_point.name, csr);

//Added Modification for ALPS00255822, bug from WHQL test
	if(request == 0 && musb_ep->dmaRelease) {
		//Ainge
		/*DBG(4, "%s line %d: sending zero pkt\n", __func__, __LINE__);
		musb_writew(epio, MUSB_TXCSR, MUSB_TXCSR_TXPKTRDY);
		*/
		DBG(0, "%s: %s request == NULL and dmaReleased!!\n", __func__, musb_ep->end_point.name);
		musb_ep->dmaRelease = 0;
		musb_ep->nextUnderRun = 1;
		return;
	}

	if(request && musb_ep->nextUnderRun) {
		/*csr &= ~(MUSB_TXCSR_P_SENDSTALL| MUSB_TXCSR_P_SENTSTALL | MUSB_TXCSR_TXPKTRDY);
		csr |= MUSB_TXCSR_CLRDATATOG;
		musb_writew(epio, MUSB_TXCSR, csr);*/
		DBG(4, "%s line %d: clear halt, csr = 0x%x\n", __func__, __LINE__, csr);

		musb_ep->nextUnderRun = 0;
	}
//Added Modification for ALPS00255822, bug from WHQL test
	dma = musb_ep->dma;

	/*
	 * REVISIT: for high bandwidth, MUSB_TXCSR_P_INCOMPTX
	 * probably rates reporting as a host error.
	 */
	if (csr & MUSB_TXCSR_P_SENTSTALL) {
		csr |=	MUSB_TXCSR_P_WZC_BITS;
		csr &= ~MUSB_TXCSR_P_SENTSTALL;
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_TXCSR, csr);
		return;
	}

	if (csr & MUSB_TXCSR_P_UNDERRUN) {
		/* We NAKed, no big deal... little reason to care. */
//Added Modification for ALPS00255822, bug from WHQL test
		DBG(1, "%s, line %d: underrun on ep%d, req 0x%p, csr 0x%x, musb_ep->nextUnderRun = %d \n", __func__, __LINE__, epnum, request, csr, musb_ep->nextUnderRun);
		if(musb_ep->nextUnderRun && request == 0)
		{
			//musb_writew(epio, MUSB_TXCSR, MUSB_TXCSR_TXPKTRDY);
			//DBG(1, "%s, line %d: underrun on ep%d, req %p, csr 0x%x, sending ZLP, musb_ep->nextUnderRun = %d \n", __func__, __LINE__, epnum, request, csr, musb_ep->nextUnderRun);
			//send stall
/*			csr |= MUSB_TXCSR_CLRDATATOG;
			csr |= MUSB_TXCSR_P_SENDSTALL;
			csr &= ~MUSB_TXCSR_TXPKTRDY;
			musb_writew(epio, MUSB_TXCSR, csr);
			DBG(1, "%s, line %d: underrun on ep%d, req %p, csr 0x%x, send Stall, musb_ep->nextUnderRun = %d \n", __func__, __LINE__, epnum, request, csr, musb_ep->nextUnderRun);*/
			DBG(1, "%s, line %d: TXMAXP 0x%x \n", __func__, __LINE__, musb_readw(epio, MUSB_TXMAXP));
			//DBG(1, "%s, line %d: TXMAXP 0x%llu, MUSB_FIFO_OFFSET(epnum) = %d \n", __func__, __LINE__, musb_readw(epio, MUSB_FIFO_OFFSET(epnum)), MUSB_FIFO_OFFSET(epnum));
			return;
		}
		else
		{
//Added Modification for ALPS00255822, bug from WHQL test
		csr |=	 MUSB_TXCSR_P_WZC_BITS;
		csr &= ~(MUSB_TXCSR_P_UNDERRUN | MUSB_TXCSR_TXPKTRDY);
//Added Modification for ALPS00255822, bug from WHQL test
		}
//Added Modification for ALPS00255822, bug from WHQL test
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_TXCSR, csr);
		DBG(1, "underrun on ep%d, req %p\n", epnum, request);
	}

	if (dma_channel_status(dma) == MUSB_DMA_STATUS_BUSY) {
		/*
		 * SHOULD NOT HAPPEN... has with CPPI though, after
		 * changing SENDSTALL (and other cases); harmless?
		 */
		DBG(1, "%s dma still busy?\n", musb_ep->end_point.name);
		return;
	}

	if (request) {
		u8 is_dma = 0;

		if (dma && (csr & MUSB_TXCSR_DMAENAB)) {
			is_dma = 1;
			musb_ep->dma_configed = FALSE;
			csr |= MUSB_TXCSR_P_WZC_BITS;
			csr &= ~(MUSB_TXCSR_DMAENAB | MUSB_TXCSR_P_UNDERRUN |
				 MUSB_TXCSR_TXPKTRDY);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_TXCSR, csr);
			/* Ensure writebuffer is empty. */
			csr = musb_readw(epio, MUSB_TXCSR);
			request->actual += musb_ep->dma->actual_len;
			DBG(3, "TXCSR%d %04x, DMA off, len %zu, req %p\n",
				epnum, csr, musb_ep->dma->actual_len, request);
		}

		if (is_dma || request->actual == request->length) {
			/*
			 * First, maybe a terminating short packet. Some DMA
			 * engines might handle this by themselves.
			 */
			if ((request->zero && request->length
			&& (request->length % musb_ep->packet_sz == 0)
			&& (request->actual == request->length))
				|| (is_dma && (!dma->desired_mode ||
				(request->actual % musb_ep->packet_sz)))
			) {
				/*
				 * On DMA completion, FIFO may not be
				 * available yet...
				 */
				if (csr & MUSB_TXCSR_TXPKTRDY)
					return;

				DBG(4, "sending zero pkt\n");
				dumpTime(funcWritew, epnum);
				musb_writew(epio, MUSB_TXCSR, /*MUSB_TXCSR_MODE
					|*/ MUSB_TXCSR_TXPKTRDY
					| (csr & MUSB_TXCSR_P_ISO));
				request->zero = 0;
			/*
			 * Return from here with the expectation of the endpoint
			 * interrupt for further action.
			 */
			return;
			}

			/* ... or if not, then complete it. */
			musb_g_giveback(musb_ep, request, 0);

			/*
			 * Kickstart next transfer if appropriate;
			 * the packet that just completed might not
			 * be transmitted for hours or days.
			 * REVISIT for double buffering...
			 * FIXME revisit for stalls too...
			 */

			musb_ep_select(mbase, epnum);
			/* If configured as DB, then FIFONOTEMPTY doesn't mean no space for new packet */
			if(!(musb_read_txfifosz(mbase) & MUSB_FIFOSZ_DPB)) {
				csr = musb_readw(epio, MUSB_TXCSR);
				if (csr & MUSB_TXCSR_FIFONOTEMPTY) {
					if ((csr & MUSB_TXCSR_TXPKTRDY) == 0 ) {
						dumpTime(funcWritew, epnum);
						musb_writew(epio, MUSB_TXCSR, /*MUSB_TXCSR_MODE
											| */MUSB_TXCSR_TXPKTRDY);
					}
					return;
				}
			}

			//request = musb_ep->desc ? next_request(musb_ep) : NULL;
			req = musb_ep->desc ? next_request(musb_ep) : NULL;
			if (!req) {
				DBG(1, "%s idle now\n",
					musb_ep->end_point.name);
				return;
			}
		}

		txstate(musb, req);
	}
}
#else
static void txstate(struct musb *musb, struct musb_request *req)
{
	u8			epnum = req->epnum;
	struct musb_ep		*musb_ep;
//	void __iomem		*epio = musb->endpoints[epnum].regs;
	void __iomem		*epio = ep_in_list[epnum]->regs;

	struct usb_request	*request;
	u16			fifo_count = 0, csr;
	int			use_dma = 0;


	if(ep_in_list[epnum])
	{
			epio = ep_in_list[epnum]->regs;
	}else
	{
			DBG(0,"ep%d in list is NULL\n");
	}
	musb_ep = req->ep;

	/* we shouldn't get here while DMA is active ... but we do ... */
	if (dma_channel_status(musb_ep->dma) == MUSB_DMA_STATUS_BUSY) {
		DBG(0, "dma pending...\n");
		return;
	}

	/* read TXCSR before */
	csr = musb_readw(epio, MUSB_TXCSR);

	request = &req->request;
	fifo_count = min(max_ep_writesize(musb, musb_ep),
			(int)(request->length - request->actual));

	if (csr & MUSB_TXCSR_TXPKTRDY) {
		DBG(0, "%s old packet still ready , txcsr %03x\n",
				musb_ep->end_point.name, csr);
		return;
	}

	if (csr & MUSB_TXCSR_P_SENDSTALL) {
		DBG(0, "%s stalling, txcsr %03x\n",
				musb_ep->end_point.name, csr);
		return;
	}

	DBG(3, "hw_ep%d, maxpacket %d, fifo count %d, txcsr %03x\n",
			epnum, musb_ep->packet_sz, fifo_count,
			csr);

	if (musb_ep->dma) {
		struct dma_controller	*c = musb->dma_controller;
		size_t request_size;
		use_dma = (request->dma != DMA_ADDR_INVALID);
		/* setup DMA, then program endpoint CSR */
		request_size = min_t(size_t, request->length - request->actual,
					musb_ep->packet_sz);
		musb_ep->dma->desired_mode = 0;
		use_dma = use_dma && c->channel_program(
				musb_ep->dma, musb_ep->packet_sz,
				musb_ep->dma->desired_mode,
				request->dma + request->actual, request_size);

		if (use_dma) {
			musb_ep->dma_configed = TRUE;
			if (musb_ep->dma->desired_mode == 0) {
				csr &= ~(MUSB_TXCSR_AUTOSET
					/*| MUSB_TXCSR_DMAENAB*/);
				dumpTime(funcWritew, epnum);
				musb_writew(epio, MUSB_TXCSR, csr
					| MUSB_TXCSR_P_WZC_BITS);
				csr &= ~MUSB_TXCSR_DMAMODE;
				csr |= (MUSB_TXCSR_DMAENAB /*|
							MUSB_TXCSR_MODE*/);
			}

			csr &= ~MUSB_TXCSR_P_UNDERRUN;
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_TXCSR, csr);
		}
	}


	if (!use_dma) {
		musb_write_fifo(musb_ep->hw_ep, fifo_count,
				(u8 *) (request->buf + request->actual));
		request->actual += fifo_count;
		csr |= MUSB_TXCSR_TXPKTRDY;
		csr &= ~MUSB_TXCSR_P_UNDERRUN;
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_TXCSR, csr);
	}

	/* host may already have the data when this message shows... */
	DBG(3, "%s TX/IN %s len %d/%d, txcsr %04x, fifo %d/%d\n",
			musb_ep->end_point.name, use_dma ? "dma" : "pio",
			request->actual, request->length,
			musb_readw(epio, MUSB_TXCSR),
			fifo_count,
			musb_readw(epio, MUSB_TXMAXP));
}
void musb_g_tx(struct musb *musb, u8 epnum)
{
	u16			csr;
	struct musb_request	*req;
	struct usb_request	*request;
	u8 __iomem		*mbase = musb->mregs;
//	struct musb_ep		*musb_ep = &musb->endpoints[epnum].ep_in;
//	void __iomem		*epio = musb->endpoints[epnum].regs;
	struct musb_ep		*musb_ep = NULL;
	void __iomem		*epio = NULL;

	struct dma_channel	*dma;

	if (ep_in_list[epnum]) {
		musb_ep = &ep_in_list[epnum]->ep_in;
		epio = ep_in_list[epnum]->regs;
	} else {
		DBG(0,"ep%d in list is NULL\n");
	}
	musb_ep_select(mbase, epnum);
	//request = next_request(musb_ep);
	req = next_request(musb_ep);
	request = &req->request;

	csr = musb_readw(epio, MUSB_TXCSR);
	DBG(3, "<== %s, txcsr %04x\n", musb_ep->end_point.name, csr);

	dma = musb_ep->dma;

	if (csr & MUSB_TXCSR_P_SENTSTALL) {
		csr |=	MUSB_TXCSR_P_WZC_BITS;
		csr &= ~MUSB_TXCSR_P_SENTSTALL;
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_TXCSR, csr);
		DBG(0, "send stall on ep%d, req %p\n", epnum, request);
		return;
	}

	if (csr & MUSB_TXCSR_P_UNDERRUN) {
		csr |=	 MUSB_TXCSR_P_WZC_BITS;
		csr &= ~(MUSB_TXCSR_P_UNDERRUN | MUSB_TXCSR_TXPKTRDY);
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_TXCSR, csr);
		DBG(1, "underrun on ep%d, req %p\n", epnum, request);
	}

	if (dma_channel_status(dma) == MUSB_DMA_STATUS_BUSY) {
		DBG(0, "%s dma still busy?\n", musb_ep->end_point.name);
		return;
	}

	if (request) {
		if (dma && (csr & MUSB_TXCSR_DMAENAB)) {
			musb_ep->dma_configed = FALSE;
			csr |= MUSB_TXCSR_P_WZC_BITS;
			csr &= ~(MUSB_TXCSR_DMAENAB | MUSB_TXCSR_P_UNDERRUN |
				 MUSB_TXCSR_TXPKTRDY);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_TXCSR, csr);
			/* Ensure writebuffer is empty. */
			csr = musb_readw(epio, MUSB_TXCSR);
			request->actual += musb_ep->dma->actual_len;
			DBG(4, "TXCSR%d %04x, DMA off, len %zu, req %p\n",
				epnum, csr, musb_ep->dma->actual_len, request);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_TXCSR, /*MUSB_TXCSR_MODE
			| */MUSB_TXCSR_TXPKTRDY);
			return ;
		}

		if (request->actual == request->length) {
			if ((request->zero && request->length
				&& request->length % musb_ep->packet_sz == 0)) {
				if (csr & MUSB_TXCSR_TXPKTRDY)
					return;
				DBG(1, "sending zero pkt\n");
				dumpTime(funcWritew, epnum);
				musb_writew(epio, MUSB_TXCSR, /*MUSB_TXCSR_MODE
						|*/ MUSB_TXCSR_TXPKTRDY);
				request->zero = 0;
				return ;
			} else {
				/* ... or if not, then complete it. */
				musb_g_giveback(musb_ep, request, 0);
				musb_ep_select(mbase, epnum);
				csr = musb_readw(epio, MUSB_TXCSR);
				if (csr & MUSB_TXCSR_FIFONOTEMPTY)
					return;

				//request = musb_ep->desc ? next_request(musb_ep) : NULL;
				req = musb_ep->desc ? next_request(musb_ep) : NULL;
				if (!request) {
					DBG(1, "%s idle now\n",
						musb_ep->end_point.name);
					return;
				}
			}


		}

		txstate(musb, to_musb_request(request));
	}
}
#endif
/* ------------------------------------------------------------ */


/* Peripheral rx (OUT) using Mentor DMA works as follows:
	- Only mode 0 is used.

	- Request is queued by the gadget class driver.
		-> if queue was previously empty, rxstate()

	- Host sends OUT token which causes an endpoint interrupt
	  /\      -> RxReady
	  |	      -> if request queued, call rxstate
	  |		/\	-> setup DMA
	  |		|	     -> DMA interrupt on completion
	  |		|		-> RxReady
	  |		|		      -> stop DMA
	  |		|		      -> ack the read
	  |		|		      -> if data recd = max expected
	  |		|				by the request, or host
	  |		|				sent a short packet,
	  |		|				complete the request,
	  |		|				and start the next one.
	  |		|_____________________________________|
	  |					 else just wait for the host
	  |					    to send the next OUT token.
	  |__________________________________________________|

 * Non-Mentor DMA engines can of course work differently.
 */


/*
 * Context: controller locked, IRQs blocked, endpoint selected
 */
static void rxstate(struct musb *musb, struct musb_request *req)
{
	const u8		epnum = req->epnum;
	struct usb_request	*request = &req->request;
//	struct musb_ep		*musb_ep = &musb->endpoints[epnum].ep_out;
//	void __iomem		*epio = musb->endpoints[epnum].regs;
	struct musb_ep		*musb_ep = NULL;
	void __iomem		*epio = NULL;

	unsigned		fifo_count = 0;
	u16			len = 0;
	u16			csr = 0;
	u8			use_mode_1;


	if(ep_out_list[epnum]) {
		musb_ep = &ep_out_list[epnum]->ep_out;
		epio = ep_out_list[epnum]->regs;
	} else {
		DBG(0,"ep%d out list is NULL \n",epnum);
		return;
	}

	/* Check if EP is disabled */
	if (!musb_ep->desc) {
		DBG(0, "ep:%s disabled - ignore request\n",
			musb_ep->end_point.name);
		return;
	}

	len = musb_ep->packet_sz;
	csr = musb_readw(epio, MUSB_RXCSR);
	/* We shouldn't get here while DMA is active, but we do... */
	if (dma_channel_status(musb_ep->dma) == MUSB_DMA_STATUS_BUSY) {
		DBG(0, "DMA pending...\n");
		return;
	}
	if (csr & MUSB_RXCSR_P_SENDSTALL) {
		DBG(0, "%s stalling, RXCSR %04x\n",
		    musb_ep->end_point.name, csr);
		return;
	}
	if (csr & MUSB_RXCSR_RXPKTRDY) {
		len = musb_readw(epio, MUSB_RXCOUNT);

		/*
		 * Enable Mode 1 on RX transfers only when short_not_ok flag
		 * is set. Currently short_not_ok flag is set only from
		 * file_storage and f_mass_storage drivers
		 */

		if (request->short_not_ok && len == musb_ep->packet_sz)
			use_mode_1 = 1;
		else
			use_mode_1 = 0;

		if (request->actual < request->length) {
			if (musb_ep->dma) {
				struct dma_controller	*c;
				struct dma_channel	*channel;
				int			use_dma = 0;

				c = musb->dma_controller;
				channel = musb_ep->dma;

	/* We use DMA Req mode 0 in rx_csr, and DMA controller operates in
	 * mode 0 only. So we do not get endpoint interrupts due to DMA
	 * completion. We only get interrupts from DMA controller.
	 *
	 * We could operate in DMA mode 1 if we knew the size of the tranfer
	 * in advance. For mass storage class, request->length = what the host
	 * sends, so that'd work.  But for pretty much everything else,
	 * request->length is routinely more than what the host sends. For
	 * most these gadgets, end of is signified either by a short packet,
	 * or filling the last byte of the buffer.  (Sending extra data in
	 * that last pckate should trigger an overflow fault.)  But in mode 1,
	 * we don't get DMA completion interrupt for short packets.
	 *
	 * Theoretically, we could enable DMAReq irq (MUSB_RXCSR_DMAMODE = 1),
	 * to get endpoint interrupt on every DMA req, but that didn't seem
	 * to work reliably.
	 *
	 * REVISIT an updated g_file_storage can set req->short_not_ok, which
	 * then becomes usable as a runtime "use mode 1" hint...
	 */

				/* Experimental: Mode1 works with mass storage use cases */
				if (use_mode_1) {
					csr |= MUSB_RXCSR_AUTOCLEAR;
					dumpTime(funcWritew, epnum);
					musb_writew(epio, MUSB_RXCSR, csr);
					csr |= MUSB_RXCSR_DMAENAB;
					dumpTime(funcWritew, epnum);
					musb_writew(epio, MUSB_RXCSR, csr);

					/*
					 * this special sequence (enabling and then
					 * disabling MUSB_RXCSR_DMAMODE) is required
					 * to get DMAReq to activate
					 */
					dumpTime(funcWritew, epnum);
					musb_writew(epio, MUSB_RXCSR,
						csr | MUSB_RXCSR_DMAMODE);
					dumpTime(funcWritew, epnum);
					musb_writew(epio, MUSB_RXCSR, csr);

				} else {
					/*
					 *	Comment out here, cuz we dont have "hb_mult"
					 *	and follow the original setting. Dont want to
					 *	change it.
					 *	if (!musb_ep->hb_mult &&
					 *	musb_ep->hw_ep->rx_double_buffered)
					 *	csr |= MUSB_RXCSR_AUTOCLEAR;
					 */
						csr |= MUSB_RXCSR_DMAENAB;
						dumpTime(funcWritew, epnum);
						musb_writew(epio, MUSB_RXCSR, csr);
				}

				if (request->actual < request->length) {
					int transfer_size = 0;
					if (use_mode_1) {
						transfer_size = min(request->length - request->actual,
								channel->max_len);
						musb_ep->dma->desired_mode = 1;
						DBG(1,"use_mode_1 %d\n",transfer_size);
					} else {
						transfer_size = min(request->length - request->actual,
								(unsigned)len);
						musb_ep->dma->desired_mode = 0;
					}

					use_dma = c->channel_program(
							channel,
							musb_ep->packet_sz,
							channel->desired_mode,
							request->dma
							+ request->actual,
							transfer_size);
				}
				if (use_dma) {
					musb_ep->dma_configed = TRUE;
					return;

				}
			}

			fifo_count = request->length - request->actual;
			DBG(3, "%s OUT/RX pio fifo %d/%d, maxpacket %d\n",
					musb_ep->end_point.name,
					len, fifo_count,
					musb_ep->packet_sz);

			fifo_count = min_t(unsigned, len, fifo_count);

			musb_read_fifo(musb_ep->hw_ep, fifo_count, (u8 *)
					(request->buf + request->actual));
			request->actual += fifo_count;

			/* REVISIT if we left anything in the fifo, flush
			 * it and report -EOVERFLOW
			 */
			/* ack the read! */
			csr |= MUSB_RXCSR_P_WZC_BITS;
			csr &= ~MUSB_RXCSR_RXPKTRDY;
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXCSR, csr);
		}
	} else {
		DBG(1,"%s rx not ready,CSR is %x,ep is %d\n",musb_ep->end_point.name,csr,epnum);
	}

	/* reach the end or short packet detected */
	if (request->actual == request->length || len < musb_ep->packet_sz)
		musb_g_giveback(musb_ep, request, 0);
}

/*
 * Data ready for a request; called from IRQ
 */
void musb_g_rx(struct musb *musb, u8 epnum)
{
	u16			csr;
	struct musb_request	*req;
	struct usb_request	*request;
	void __iomem		*mbase = musb->mregs;
//	struct musb_ep		*musb_ep = &musb->endpoints[epnum].ep_out;
//	void __iomem		*epio = musb->endpoints[epnum].regs;
	struct musb_ep		*musb_ep = NULL;
	void __iomem		*epio = NULL;

	struct dma_channel	*dma;

	if (ep_out_list[epnum]) {
		musb_ep = &ep_out_list[epnum]->ep_out;
		epio = ep_out_list[epnum]->regs;
	} else {
		DBG(0,"ep%d out list is NULL \n",epnum);
		return;
	}

	musb_ep_select(mbase, epnum);
	csr = musb_readw(epio, MUSB_RXCSR);

	req = next_request(musb_ep);
	if (!req) {
		if((csr & MUSB_RXCSR_RXPKTRDY)&&(musb_readw(epio, MUSB_RXCOUNT) == 0)) {
			csr &= ~MUSB_RXCSR_RXPKTRDY;
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXCSR, csr);
		}
//Added Modification for ALPS00255822, bug from WHQL test
		DBG(1,"%s: !req!!, \n", __func__);
//Added Modification for ALPS00255822, bug from WHQL test
		return;
	}


	request = &req->request;
	dma = musb_ep->dma;

	DBG(3, "<== %s, rxcsr %04x%s %p\n", musb_ep->end_point.name,
			csr, dma ? " (dma)" : "", request);

	USB_LOGGER(MUSB_G_RX, MUSB_G_RX, musb_ep->end_point.name, csr, \
		(dma!=NULL) ? "DMA" : "PIO", request);

	if (csr & MUSB_RXCSR_P_SENTSTALL) {
		csr |= MUSB_RXCSR_P_WZC_BITS;
		csr &= ~MUSB_RXCSR_P_SENTSTALL;
		DBG(0,"%s sendstall on %p\n",musb_ep->name,request);
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_RXCSR, csr);
		return;
	}

	if (csr & MUSB_RXCSR_P_OVERRUN) {
		/* csr |= MUSB_RXCSR_P_WZC_BITS; */
		DBG(0, "MUSB_WARNING : MUSB_RXCSR_P_OVERRUN\n");
		csr &= ~MUSB_RXCSR_P_OVERRUN;
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_RXCSR, csr);

		DBG(0, "%s iso overrun on %p\n", musb_ep->name, request);
		//if (request && request->status == -EINPROGRESS)
		if (request->status == -EINPROGRESS)
			request->status = -EOVERFLOW;
	}
	if (csr & MUSB_RXCSR_INCOMPRX) {
		/* REVISIT not necessarily an error */
		DBG(1, "%s, incomprx\n", musb_ep->end_point.name);
	}
	if(csr&MUSB_RXCSR_FIFOFULL)
	{
		DBG(1, "%s, FIFO full\n", musb_ep->end_point.name);
	}

	if (dma_channel_status(dma) == MUSB_DMA_STATUS_BUSY) {
		/* "should not happen"; likely RXPKTRDY pending for DMA */
		DBG((csr & MUSB_RXCSR_DMAENAB) ? 40 : 40,
			"%s busy, csr %04x\n",
			musb_ep->end_point.name, csr);
		return;
	}

	if (dma && (csr & MUSB_RXCSR_DMAENAB)) {
		csr &= ~(MUSB_RXCSR_AUTOCLEAR
				| MUSB_RXCSR_DMAENAB
				| MUSB_RXCSR_DMAMODE);
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_RXCSR,
			MUSB_RXCSR_P_WZC_BITS | csr);

		request->actual += musb_ep->dma->actual_len;
		musb_ep->dma_configed = FALSE;
		DBG(3, "RXCSR%d %04x, dma off, %04x, len %zu, req %p\n",
			epnum, csr,
			musb_readw(epio, MUSB_RXCSR),
			musb_ep->dma->actual_len, request);
		/* Autoclear doesn't clear RxPktRdy for short packets */
		if ((dma->desired_mode == 0)
				|| (dma->actual_len
					& (musb_ep->packet_sz - 1))) {
			/* ack the read! */
			csr &= ~MUSB_RXCSR_RXPKTRDY;
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXCSR, csr);
		}

		/* incomplete, and not short? wait for next IN packet */
		if ((request->actual < request->length)
				&& (musb_ep->dma->actual_len
					== musb_ep->packet_sz))
		{
			DBG(1,"request is not complete %d /%d\n",request->actual,request->length);
			return;
		}

		musb_g_giveback(musb_ep, request, 0);
		/*
		 * In the giveback function the MUSB lock is
		 * released and acquired after sometime. During
		 * this time period the INDEX register could get
		 * changed by the gadget_queue function especially
		 * on SMP systems. Reselect the INDEX to be sure
		 * we are reading/modifying the right registers
		 */
		musb_ep_select(mbase, epnum);

		req = next_request(musb_ep);
		if (!req)
		{
			DBG(1,"musb request is null\n");
			return;

		}
	}

	/* analyze request if the ep is hot */
	if (req){
		//rxstate(musb, to_musb_request(request));
		rxstate(musb, req);
	}
	else
		DBG(0, "packet waiting for %s%s request\n",
				musb_ep->desc ? "" : "inactive ",
				musb_ep->end_point.name);
	return;
}

/* ------------------------------------------------------------ */

static int musb_gadget_enable(struct usb_ep *ep,
			const struct usb_endpoint_descriptor *desc)
{
	unsigned long		flags;
	struct musb_ep		*musb_ep;
	struct musb_hw_ep	*hw_ep;
	void __iomem		*regs;
	struct musb		*musb;
	void __iomem	*mbase;
	u8		epnum;
	u16		csr;
	unsigned	tmp;
	int		status = -EINVAL;

	if (!ep || !desc)
		return -EINVAL;

	musb_ep = to_musb_ep(ep);
	hw_ep = musb_ep->hw_ep;
	regs = hw_ep->regs;
	musb = musb_ep->musb;
	mbase = musb->mregs;
	epnum = musb_ep->current_epnum;

	spin_lock_irqsave(&musb->lock, flags);

	if (musb_ep->desc) {
		status = -EBUSY;
		goto fail;
	}
	musb_ep->type = usb_endpoint_type(desc);

	/* check direction and (later) maxpacket size against endpoint */
/*	if (usb_endpoint_num(desc) != epnum)
		goto fail;*/

	/* REVISIT this rules out high bandwidth periodic transfers */
	tmp = le16_to_cpu(desc->wMaxPacketSize);
	if (tmp & ~0x07ff)
		goto fail;
	musb_ep->packet_sz = tmp;

	/* enable the interrupts for the endpoint, set the endpoint
	 * packet size (or fail), set the mode, clear the fifo
	 */
	musb_ep_select(mbase, epnum);
	if (usb_endpoint_dir_in(desc)) {
		u16 int_txe = musb_readw(mbase, MUSB_INTRTXE);

		if (hw_ep->is_shared_fifo)
			musb_ep->is_in = 1;
		if (!musb_ep->is_in)
			goto fail;
		if (tmp > hw_ep->max_packet_sz_tx)
			goto fail;

		int_txe |= (1 << epnum);
		dumpTime(funcWritew, epnum);
		musb_writew(mbase, MUSB_INTRTXE, int_txe);
		dumpTime(funcWritew, epnum);
		musb_writew(regs, MUSB_TXMAXP, tmp);

		csr = /*MUSB_TXCSR_MODE | */MUSB_TXCSR_CLRDATATOG;
		if (musb_readw(regs, MUSB_TXCSR)
				& MUSB_TXCSR_FIFONOTEMPTY)
			csr |= MUSB_TXCSR_FLUSHFIFO;
		if (musb_ep->type == USB_ENDPOINT_XFER_ISOC)
			csr |= MUSB_TXCSR_P_ISO;

		/* set twice in case of double buffering */
		dumpTime(funcWritew, epnum);
		musb_writew(regs, MUSB_TXCSR, csr);
		/* REVISIT may be inappropriate w/o FIFONOTEMPTY ... */
		dumpTime(funcWritew, epnum);
		musb_writew(regs, MUSB_TXCSR, csr);

	} else {
		u16 int_rxe = musb_readw(mbase, MUSB_INTRRXE);

		if (hw_ep->is_shared_fifo)
			musb_ep->is_in = 0;
		if (musb_ep->is_in)
			goto fail;
		if (tmp > hw_ep->max_packet_sz_rx)
			goto fail;

		int_rxe |= (1 << epnum);
		dumpTime(funcWritew, epnum);
		musb_writew(mbase, MUSB_INTRRXE, int_rxe);
		dumpTime(funcWritew, epnum);
		musb_writew(regs, MUSB_RXMAXP, tmp);

		/* force shared fifo to OUT-only mode */
		if (hw_ep->is_shared_fifo) {
			csr = musb_readw(regs, MUSB_TXCSR);
			csr &= ~(MUSB_TXCSR_MODE | MUSB_TXCSR_TXPKTRDY);
			dumpTime(funcWritew, epnum);
			musb_writew(regs, MUSB_TXCSR, csr);
		}
/*don't flush fifo when enable , because sometimes usb will receive packets before ep enabled. when
flush fifo here will lost those packets. We will flush fifo during disabe ep*/
/*		csr = MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_CLRDATATOG;
		if (musb_ep->type == USB_ENDPOINT_XFER_ISOC)
			csr |= MUSB_RXCSR_P_ISO;
		else if (musb_ep->type == USB_ENDPOINT_XFER_INT)
			csr |= MUSB_RXCSR_DISNYET;*/

		/* set twice in case of double buffering */
/*		musb_writew(regs, MUSB_RXCSR, csr);
		musb_writew(regs, MUSB_RXCSR, csr);*/

	}


	fifo_setup(musb,musb_ep);


	/* NOTE:  all the I/O code _should_ work fine without DMA, in case
	 * for some reason you run out of channels here.
	 */
	if (musb->dma_controller && musb_ep->type != USB_ENDPOINT_XFER_INT) { //interrupt mode ep don't use dma
		struct dma_controller	*c = musb->dma_controller;

		musb_ep->dma = c->channel_alloc(c, hw_ep,
				(desc->bEndpointAddress & USB_DIR_IN));
	} else
		musb_ep->dma = NULL;

	musb_ep->desc = desc;
	musb_ep->busy = 0;
	musb_ep->wedged = 0;
	status = 0;

	DBG(0,"%s periph: enabled %s for %s %s, %smaxpacket %d\n",
			musb_driver_name, musb_ep->end_point.name,
			({ char *s; switch (musb_ep->type) {
			case USB_ENDPOINT_XFER_BULK:	s = "bulk"; break;
			case USB_ENDPOINT_XFER_INT:	s = "int"; break;
			default:			s = "iso"; break;
			}; s; }),
			musb_ep->is_in ? "IN" : "OUT",
			musb_ep->dma ? "dma, " : "",
			musb_ep->packet_sz);
	DBG(2,"MUSB: ep rx inte is%x tx inte is %x\n",musb_readw(mbase,MUSB_INTRRXE),musb_readw(mbase,MUSB_INTRTXE));
fail:
	spin_unlock_irqrestore(&musb->lock, flags);
	return status;
}

/*
 * Disable an endpoint flushing all requests queued.
 */
static int musb_gadget_disable(struct usb_ep *ep)
{
	unsigned long	flags;
	struct musb	*musb;
	u8		epnum;
	struct musb_ep	*musb_ep;
	void __iomem	*epio;
	int		status = 0;

	musb_ep = to_musb_ep(ep);
	musb = musb_ep->musb;
	epnum = musb_ep->current_epnum;
	if(musb_ep->is_in && ep_in_list[epnum])
		epio = ep_in_list[epnum]->regs;
	else if (!musb_ep->is_in&&ep_out_list[epnum])
		epio = ep_out_list[epnum]->regs;
	else
		//epio = musb->endpoints[epnum].regs;
	{
		DBG(0,"EP %d is not used\n",epnum);
		return status;
	}

	spin_lock_irqsave(&musb->lock, flags);
	if(musb->power)
	{
		musb_ep_select(musb->mregs, epnum);

		/* zero the endpoint sizes */
		if (musb_ep->is_in) {
			u16 int_txe = musb_readw(musb->mregs, MUSB_INTRTXE);
			int_txe &= ~(1 << epnum);
			dumpTime(funcWritew, epnum);
			musb_writew(musb->mregs, MUSB_INTRTXE, int_txe);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_TXMAXP, 0);
		} else {
			u16 int_rxe = musb_readw(musb->mregs, MUSB_INTRRXE);
			u16 csr;
			int_rxe &= ~(1 << epnum);
			dumpTime(funcWritew, epnum);
			musb_writew(musb->mregs, MUSB_INTRRXE, int_rxe);

/*flush fifo here*/
			csr = MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_CLRDATATOG;

			/* set twice in case of double buffering */
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXCSR, csr);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXCSR, csr);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXMAXP, 0);
		}

		musb_ep->desc = NULL;

		/* abort all pending DMA and requests */
		nuke(musb_ep, -ESHUTDOWN);

	}
	spin_unlock_irqrestore(&(musb->lock), flags);

	DBG(2, "%s\n", musb_ep->end_point.name);

	return status;
}

/*
 * Allocate a request for an endpoint.
 * Reused by ep0 code.
 */
struct usb_request *musb_alloc_request(struct usb_ep *ep, gfp_t gfp_flags)
{
	struct musb_ep		*musb_ep = to_musb_ep(ep);
	struct musb_request	*request = NULL;

	request = kzalloc(sizeof *request, gfp_flags);
	if (request) {
		INIT_LIST_HEAD(&request->request.list);
		request->request.dma = DMA_ADDR_INVALID;
		request->epnum = musb_ep->current_epnum;
		request->ep = musb_ep;
	}else
	{
		DBG(0,"alloc request error\n");
	}

	return &request->request;
}

/*
 * Free a request
 * Reused by ep0 code.
 */
void musb_free_request(struct usb_ep *ep, struct usb_request *req)
{
	kfree(to_musb_request(req));
}

static LIST_HEAD(buffers);

struct free_record {
	struct list_head	list;
	struct device		*dev;
	unsigned		bytes;
	dma_addr_t		dma;
};

/*
 * Context: controller locked, IRQs blocked.
 */
void musb_ep_restart(struct musb *musb, struct musb_request *req)
{
	DBG(3, "<== %s request %p len %u on hw_ep%d\n",
		req->tx ? "TX/IN" : "RX/OUT",
		&req->request, req->request.length, req->epnum);

	musb_ep_select(musb->mregs, req->epnum);
	if (req->tx)
		txstate(musb, req);
	else
		rxstate(musb, req);
}

static int musb_gadget_queue(struct usb_ep *ep, struct usb_request *req,
			gfp_t gfp_flags)
{
	struct musb_ep		*musb_ep;
	struct musb_request	*request;
	struct musb		*musb;
	int			status = 0;
	unsigned long		lockflags;

	if (!ep || !req)
		return -EINVAL;
	if (!req->buf)
		return -ENODATA;

	musb_ep = to_musb_ep(ep);
	musb = musb_ep->musb;

	request = to_musb_request(req);
	request->musb = musb;

	if (request->ep != musb_ep)
		return -EINVAL;

	DBG(2, "<== to %s request=%p\n", ep->name, req);


	/* request is mine now... */
	request->request.actual = 0;
	request->request.status = -EINPROGRESS;
	request->epnum = musb_ep->current_epnum;
	request->tx = musb_ep->is_in;

	spin_lock_irqsave(&musb->lock, lockflags);

//Added Modification for ALPS00255822, bug from WHQL test
	DBG(1,"%s: <== to %s, musb->dma =0x%p, request->request.dma = 0x%x!! DMA_ADDR_INVALID=0x%x\n", __func__, ep->name, musb_ep->dma, request->request.dma, DMA_ADDR_INVALID);
	musb_ep->dmaRelease = 0;
//Added Modification for ALPS00255822, bug from WHQL test
	/* don't queue if the ep is down */
	if (musb_ep->dma) {
		if (request->request.dma == DMA_ADDR_INVALID) {
			request->request.dma = dma_map_single(
					musb->controller,
					request->request.buf,
					request->request.length,
					request->tx
						? DMA_TO_DEVICE
						: DMA_FROM_DEVICE);
			request->map_state = MUSB_MAPPED;
		} else {
			dma_sync_single_for_device(musb->controller,
					request->request.dma,
					request->request.length,
					request->tx
						? DMA_TO_DEVICE
						: DMA_FROM_DEVICE);
			request->map_state = UN_MAPPED;
		}
	} else if (!req->buf) {
		status = -ENODATA;
		goto cleanup;
	} else
		request->map_state = UN_MAPPED;


	/* don't queue if the ep is down */
	if (!musb_ep->desc) {
		DBG(0, "req %p queued to %s while ep %s\n",
				req, ep->name, "disabled");
		status = -ESHUTDOWN;
		goto cleanup;
	}

	/* add request to the list */
	//list_add_tail(&(request->request.list), &(musb_ep->req_list));
	list_add_tail(&request->list, &musb_ep->req_list);
/*	if(musb_ep->current_epnum == 3 && !musb_ep->is_in)
		req_count++;*/

	/* it this is the head of the queue, start i/o ... */
	//if (!musb_ep->busy && &request->request.list == musb_ep->req_list.next)
	if (!musb_ep->busy && &request->list == musb_ep->req_list.next)
		musb_ep_restart(musb, request);

cleanup:
	spin_unlock_irqrestore(&musb->lock, lockflags);
	return status;
}

static int musb_gadget_dequeue(struct usb_ep *ep, struct usb_request *request)
{
	struct musb_ep		*musb_ep = to_musb_ep(ep);
	struct musb_request	*req = to_musb_request(request);
	//struct usb_request	*r;
	struct musb_request	*r;
	unsigned long		flags;
	int			status = 0;
	struct musb		*musb = musb_ep->musb;

	if (!ep || !request || to_musb_request(request)->ep != musb_ep)
		return -EINVAL;

	spin_lock_irqsave(&musb->lock, flags);

	list_for_each_entry(r, &musb_ep->req_list, list) {
		//if (r == request)
		if (r == req)
			break;
	}
	//if (r != request) {
	if (r != req) {
		DBG(0, "request %p not queued to %s\n", request, ep->name);
		status = -EINVAL;
		goto done;
	}

	/* if the hardware doesn't have the request, easy ... */
	//if (musb_ep->req_list.next != &request->list || musb_ep->busy)
	if (musb_ep->req_list.next != &req->list || musb_ep->busy)
	{

		musb_g_giveback(musb_ep, request, -ECONNRESET);

	}


	/* ... else abort the dma transfer ... */
	else if (musb_ep->dma) {
		struct dma_controller	*c = musb->dma_controller;

		musb_ep_select(musb->mregs, musb_ep->current_epnum);
		if (c->channel_abort)
			status = c->channel_abort(musb_ep->dma);
		else
			status = -EBUSY;
		if (status == 0)
		{

			musb_g_giveback(musb_ep, request, -ECONNRESET);

		}

	} else {
		/* NOTE: by sticking to easily tested hardware/driver states,
		 * we leave counting of in-flight packets imprecise.
		 */
		musb_g_giveback(musb_ep, request, -ECONNRESET);

	}

done:
	spin_unlock_irqrestore(&musb->lock, flags);
	return status;
}

//Added Modification for ALPS00255822, bug from WHQL test
static int musb_gadget_send_ZLP(struct usb_ep *ep)
{
	unsigned long	flags;
	struct musb	*musb;
	u8		epnum;
	struct musb_ep	*musb_ep;
	void __iomem	*epio;
	void __iomem	*mbase;
	int		status = 0;
	u16			csr;

	DBG(1,"%s: \n", __func__);

	musb_ep = to_musb_ep(ep);
	musb = musb_ep->musb;
	epnum = musb_ep->current_epnum;

	DBG(0,"%s: epnum = %d, musb_ep->is_in = %d \n", __func__, epnum, musb_ep->is_in);

	if (!ep)
		return -EINVAL;
	mbase = musb->mregs;

	if(musb_ep->is_in && ep_in_list[epnum])
		epio = ep_in_list[epnum]->regs;
	else if (!musb_ep->is_in&&ep_out_list[epnum])
		epio = ep_out_list[epnum]->regs;
	else
			//epio = musb->endpoints[epnum].regs;
	{
		DBG(0,"%s: EP %d is not used\n", __func__,epnum);
		return status;
	}

	DBG(1,"%s: epnum = %d, \n", __func__, epnum);

	spin_lock_irqsave(&musb->lock, flags);

	musb_ep_select(mbase, epnum);

	/* set TxPkyRdy bits */

	DBG(2, "%s: %s: \n", __func__, ep->name);
	if (musb_ep->is_in) {
		csr = musb_readw(epio, MUSB_TXCSR);
		rmb();
		DBG(2, "%s, line %d: %s: ZLP, csr = 0x%x\n", __func__, __LINE__, ep->name, csr);
		/*csr &= ~MUSB_TXCSR_P_UNDERRUN;
		csr |= MUSB_TXCSR_TXPKTRDY;
		DBG(2, "%s, line %d: %s: ZLP, csr = 0x%x\n", __func__, __LINE__, ep->name, csr);
		musb_writew(epio, MUSB_TXCSR, csr);*/
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_TXCSR, MUSB_TXCSR_TXPKTRDY);	//sending ZLP
		wmb();
	}

	spin_unlock_irqrestore(&musb->lock, flags);
	return status;

}

static int musb_gadget_dequeue_all(struct usb_ep *ep, struct usb_request *request)
{
	unsigned long	flags;
	struct musb	*musb;
	u8		epnum;
	struct musb_ep	*musb_ep;
	void __iomem	*epio;
	int		status = 0;


	musb_ep = to_musb_ep(ep);
	musb = musb_ep->musb;
	epnum = musb_ep->current_epnum;

	DBG(1,"%s: %d \n", __func__, (musb_ep->dmaRelease = 1 ));

	DBG(0,"%s: epnum = %d, musb_ep->is_in = %d \n", __func__, epnum, musb_ep->is_in);

	if(musb_ep->is_in && ep_in_list[epnum])
		epio = ep_in_list[epnum]->regs;
	else if (!musb_ep->is_in&&ep_out_list[epnum])
		epio = ep_out_list[epnum]->regs;
	else
		//epio = musb->endpoints[epnum].regs;
	{
		DBG(0,"%s: EP %d is not used\n", __func__,epnum);
		return status;
	}

	musb_ep->dmaRelease = 1;

	spin_lock_irqsave(&musb->lock, flags);
	#if 1
	if(musb->power)
	{
		musb_ep_select(musb->mregs, epnum);

		/* zero the endpoint sizes */
		if (musb_ep->is_in) {
			u16 int_txe = musb_readw(musb->mregs, MUSB_INTRTXE);
			int_txe &= ~(1 << epnum);
			dumpTime(funcWritew, epnum);
			musb_writew(musb->mregs, MUSB_INTRTXE, int_txe);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_TXMAXP, 0);
		} else {
			u16 int_rxe = musb_readw(musb->mregs, MUSB_INTRRXE);
			u16 csr;
			int_rxe &= ~(1 << epnum);
			dumpTime(funcWritew, epnum);
			musb_writew(musb->mregs, MUSB_INTRRXE, int_rxe);

/*flush fifo here*/
			csr = MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_CLRDATATOG;

			/* set twice in case of double buffering */
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXCSR, csr);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXCSR, csr);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXMAXP, 0);
		}

		musb_ep->desc = NULL;

		//Ainge
		DBG(1,"%s: call nuke_without_delReq to shut down the ep \n", __func__);
		//Ainge

		/* abort all pending DMA and requests */
		nuke_without_delReq(musb_ep, -ESHUTDOWN);

	}
	#endif
	spin_unlock_irqrestore(&(musb->lock), flags);

	DBG(2, "%s: %s \n", __func__, musb_ep->end_point.name);

	return status;
}
//Added Modification for ALPS00255822, bug from WHQL test
/*
 * Set or clear the halt bit of an endpoint. A halted enpoint won't tx/rx any
 * data but will queue requests.
 *
 * exported to ep0 code
 */
static int musb_gadget_set_halt(struct usb_ep *ep, int value)
{
	struct musb_ep		*musb_ep = to_musb_ep(ep);
	u8			epnum = musb_ep->current_epnum;
	struct musb		*musb = musb_ep->musb;
	void __iomem		*epio; //= musb->endpoints[epnum].regs;
	void __iomem		*mbase;
	unsigned long		flags;
	u16			csr;
	struct musb_request	*request;
	int			status = 0;

	if (!ep)
		return -EINVAL;
	mbase = musb->mregs;

	if(musb_ep->is_in && ep_in_list[epnum])
		epio = ep_in_list[epnum]->regs;
	else if (!musb_ep->is_in&&ep_out_list[epnum])
		epio = ep_out_list[epnum]->regs;
	else
			//epio = musb->endpoints[epnum].regs;
	{
		DBG(0,"EP %d is not used\n",epnum);
		return status;
	}


	spin_lock_irqsave(&musb->lock, flags);

	if ((USB_ENDPOINT_XFER_ISOC == musb_ep->type)) {
		status = -EINVAL;
		goto done;
	}

	musb_ep_select(mbase, epnum);

	//request = to_musb_request(next_request(musb_ep));
	request = next_request(musb_ep);
	if (value) {
		if (request) {
			DBG(0, "request in progress, cannot halt %s\n",
			    ep->name);
			status = -EAGAIN;
			goto done;
		}
		/* Cannot portably stall with non-empty FIFO */
		if (musb_ep->is_in) {
			csr = musb_readw(epio, MUSB_TXCSR);
			if (csr & MUSB_TXCSR_FIFONOTEMPTY) {
				DBG(0, "FIFO busy, cannot halt %s\n", ep->name);
				status = -EAGAIN;
				goto done;
			}
		}
	} else
		musb_ep->wedged = 0;

	/* set/clear the stall and toggle bits */
	DBG(0, "%s: %s stall\n", ep->name, value ? "set" : "clear");
	if (musb_ep->is_in) {
		csr = musb_readw(epio, MUSB_TXCSR);
		csr |= MUSB_TXCSR_P_WZC_BITS
			| MUSB_TXCSR_CLRDATATOG;
		if (value)
			csr |= MUSB_TXCSR_P_SENDSTALL;
		else
			csr &= ~(MUSB_TXCSR_P_SENDSTALL
				| MUSB_TXCSR_P_SENTSTALL);
		csr &= ~MUSB_TXCSR_TXPKTRDY;
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_TXCSR, csr);
	} else {
		csr = musb_readw(epio, MUSB_RXCSR);
		csr |= MUSB_RXCSR_P_WZC_BITS
			| MUSB_RXCSR_FLUSHFIFO
			| MUSB_RXCSR_CLRDATATOG;
		if (value)
			csr |= MUSB_RXCSR_P_SENDSTALL;
		else
			csr &= ~(MUSB_RXCSR_P_SENDSTALL
				| MUSB_RXCSR_P_SENTSTALL);
		dumpTime(funcWritew, epnum);
		musb_writew(epio, MUSB_RXCSR, csr);
	}

	/* maybe start the first request in the queue */
	if (!musb_ep->busy && !value && request) {
		DBG(0, "restarting the request\n");
		musb_ep_restart(musb, request);
	}

done:
	spin_unlock_irqrestore(&musb->lock, flags);
	return status;
}

/*
 * Sets the halt feature with the clear requests ignored
 */
static int musb_gadget_set_wedge(struct usb_ep *ep)
{
	struct musb_ep		*musb_ep = to_musb_ep(ep);

	if (!ep)
		return -EINVAL;

	musb_ep->wedged = 1;

	return usb_ep_set_halt(ep);
}

static int musb_gadget_fifo_status(struct usb_ep *ep)
{
	struct musb_ep		*musb_ep = to_musb_ep(ep);
	void __iomem		*epio = musb_ep->hw_ep->regs;
	int			retval = -EINVAL;

	if (musb_ep->desc && !musb_ep->is_in) {
		struct musb		*musb = musb_ep->musb;
		int			epnum = musb_ep->current_epnum;
		void __iomem		*mbase = musb->mregs;
		unsigned long		flags;

		spin_lock_irqsave(&musb->lock, flags);

		musb_ep_select(mbase, epnum);
		/* FIXME return zero unless RXPKTRDY is set */
		retval = musb_readw(epio, MUSB_RXCOUNT);

		spin_unlock_irqrestore(&musb->lock, flags);
	}
	return retval;
}

static void musb_gadget_fifo_flush(struct usb_ep *ep)
{
	struct musb_ep	*musb_ep = to_musb_ep(ep);
	struct musb	*musb = musb_ep->musb;
	u8		epnum = musb_ep->current_epnum;
	void __iomem	*epio ;//musb->endpoints[epnum].regs;
	void __iomem	*mbase;
	unsigned long	flags;
	u16		csr, int_txe;

	mbase = musb->mregs;

	if(musb_ep->is_in && ep_in_list[epnum])
		epio = ep_in_list[epnum]->regs;
	else if (!musb_ep->is_in&&ep_out_list[epnum])
		epio = ep_out_list[epnum]->regs;
	else
			//epio = musb->endpoints[epnum].regs;
	{
		DBG(0,"EP %d is not used\n",epnum);
		return ;
	}

	spin_lock_irqsave(&musb->lock, flags);


	/* disable interrupts */
	if(musb->power)
	{
		musb_ep_select(mbase, (u8) epnum);
		int_txe = musb_readw(mbase, MUSB_INTRTXE);
		dumpTime(funcWritew, epnum);
		musb_writew(mbase, MUSB_INTRTXE, int_txe & ~(1 << epnum));

		if (musb_ep->is_in) {
			csr = musb_readw(epio, MUSB_TXCSR);
			if (csr & MUSB_TXCSR_FIFONOTEMPTY) {
				csr |= MUSB_TXCSR_FLUSHFIFO | MUSB_TXCSR_P_WZC_BITS;

			/*
			 * Setting both TXPKTRDY and FLUSHFIFO makes controller
			 * to interrupt current FIFO loading, but not flushing
			 * the already loaded ones.
			 */
				csr &= ~MUSB_TXCSR_TXPKTRDY;

				dumpTime(funcWritew, epnum);
				musb_writew(epio, MUSB_TXCSR, csr);
				/* REVISIT may be inappropriate w/o FIFONOTEMPTY ... */
				dumpTime(funcWritew, epnum);
				musb_writew(epio, MUSB_TXCSR, csr);
			}
		} else {
			csr = musb_readw(epio, MUSB_RXCSR);
			csr |= MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_P_WZC_BITS;
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXCSR, csr);
			dumpTime(funcWritew, epnum);
			musb_writew(epio, MUSB_RXCSR, csr);
		}

		/* re-enable interrupt */
	   dumpTime(funcWritew, epnum);
	   musb_writew(mbase, MUSB_INTRTXE, int_txe);

	}

	spin_unlock_irqrestore(&musb->lock, flags);
}

static const struct usb_ep_ops musb_ep_ops = {
	.enable		= musb_gadget_enable,
	.disable	= musb_gadget_disable,
	.alloc_request	= musb_alloc_request,
	.free_request	= musb_free_request,
	.queue		= musb_gadget_queue,
	.dequeue	= musb_gadget_dequeue,
	.set_halt	= musb_gadget_set_halt,
	.set_wedge	= musb_gadget_set_wedge,
	.fifo_status	= musb_gadget_fifo_status,
	.fifo_flush	= musb_gadget_fifo_flush
//Added Modification for ALPS00255822, bug from WHQL test
	,.dequeue_all = musb_gadget_dequeue_all
	,.send_ZLP = musb_gadget_send_ZLP
//Added Modification for ALPS00255822, bug from WHQL test

};

/* ----------------------------------------------------------------------- */

static int musb_gadget_get_frame(struct usb_gadget *gadget)
{
	struct musb	*musb = gadget_to_musb(gadget);

	return (int)musb_readw(musb->mregs, MUSB_FRAME);
}


static int
musb_gadget_set_self_powered(struct usb_gadget *gadget, int is_selfpowered)
{
	return -ENOTSUPP;

}

static void musb_pullup(struct musb *musb, int is_on)
{
	static int first_enable = 0;

/*	u8 power;

	power = musb_readb(musb->mregs, MUSB_POWER);
	if (is_on)
		power |= MUSB_POWER_SOFTCONN;
	else
		power &= ~MUSB_POWER_SOFTCONN;

	DBG(3, "gadget %s D+ pullup %s\n",
		musb->gadget_driver->function, is_on ? "on" : "off");
	musb_writeb(musb->mregs, MUSB_POWER, power);*/

	/* This is a workaround to check if need to turn on USB */
	/* The init.usb.rc would always write enable to 1 when device booting */
#if 0
	if (unlikely(first_enable == 0 && is_on)) {
		first_enable++;
		if (!is_usb_connected()) {
		    DBG(0, "no USB cable, don't need to turn on USB\n");
		    return;
		}
	}
#else
	if (!is_usb_connected()&& is_on) {
		DBG(0, "no USB cable, don't need to turn on USB\n");
		return;
	}
#endif
	down(&musb->musb_lock);

	DBG(0,"MUSB: gadget pull up %d start\n", is_on);

	if (is_on)
		musb_start(musb);
	else
		musb_stop(musb);

	DBG(0,"MUSB: gadget pull up %d end\n", is_on);

	up(&musb->musb_lock);
}


static int musb_gadget_vbus_session(struct usb_gadget *gadget, int is_active)
{
	DBG(2, "<= %s =>\n", __func__);

	/*
	 * FIXME iff driver's softconnect flag is set (as it is during probe,
	 * though that can clear it), just musb_pullup().
	 */

	return -ENOTSUPP;
}


static int musb_gadget_vbus_draw(struct usb_gadget *gadget, unsigned mA)
{
	return -ENOTSUPP;

}

static int musb_gadget_pullup(struct usb_gadget *gadget, int is_on)
{
	struct musb	*musb = gadget_to_musb(gadget);
	unsigned long	flags;

	is_on = !!is_on;

	/* NOTE: this assumes we are sensing vbus; we'd rather
	 * not pullup unless the B-session is active.
	 */
	spin_lock_irqsave(&musb->lock, flags);
	if (is_on != musb->softconnect) {
		musb->softconnect = is_on;
		spin_unlock_irqrestore(&musb->lock, flags);
		musb_pullup(musb, is_on);
		DBG(0,"musb pullup %d\n", is_on);
		spin_lock_irqsave(&musb->lock, flags);
	}else
		DBG(1,"MUSB:%s is_on = %d,softconnect = %d\n",__func__,is_on,musb->softconnect);
	spin_unlock_irqrestore(&musb->lock, flags);

	musb->is_ready = is_on;

	return 0;
}

static const struct usb_gadget_ops musb_gadget_operations = {
	.get_frame		= musb_gadget_get_frame,
	.wakeup			= NULL,
	.set_selfpowered	= musb_gadget_set_self_powered,
	.vbus_session		= musb_gadget_vbus_session,
	.vbus_draw		= musb_gadget_vbus_draw,
	.pullup			= musb_gadget_pullup,
};

/* ----------------------------------------------------------------------- */

/* Registration */

/* Only this registration code "knows" the rule (from USB standards)
 * about there being only one external upstream port.  It assumes
 * all peripheral ports are external...
 */
static struct musb *the_gadget;

static void musb_gadget_release(struct device *dev)
{
	/* kref_put(WHAT) */
	dev_dbg(dev, "%s\n", __func__);
}


static void __init
init_peripheral_ep(struct musb *musb, struct musb_ep *ep, u8 epnum, int is_in)
{
	struct musb_hw_ep	*hw_ep = musb->endpoints + epnum;

//	memset(ep, 0, sizeof *ep);

	ep->current_epnum = epnum;
	ep->musb = musb;
	ep->hw_ep = hw_ep;
	ep->is_in = is_in;

	INIT_LIST_HEAD(&ep->req_list);

	sprintf(ep->name, "ep%d%s-%s", epnum,
			(!epnum || hw_ep->is_shared_fifo) ? "" : (
				is_in ? "in" : "out"),({ char *mode;
		switch (hw_ep->ep_mode) {
		case EP_CONT:	mode = "control"; break;
		case EP_ISO:	mode = "iso"; break;
		case EP_INT:	mode = "int"; break;
		case EP_BULK:	mode = "bulk"; break;
		default:		mode = "unknown"; break;
		} ; mode; }));
	DBG(0,"EP %d name is %s\n",epnum,ep->name);
	ep->end_point.name = ep->name;
	INIT_LIST_HEAD(&ep->end_point.ep_list);
	if (!epnum) {
		ep->end_point.maxpacket = 64;
		ep->end_point.ops = &musb_g_ep0_ops;
		musb->g.ep0 = &ep->end_point;
	} else {
		if (is_in)
			ep->end_point.maxpacket = hw_ep->max_packet_sz_tx;
		else
			ep->end_point.maxpacket = hw_ep->max_packet_sz_rx;
		ep->end_point.ops = &musb_ep_ops;
		list_add_tail(&ep->end_point.ep_list, &musb->g.ep_list);
	}
}

/*
 * Initialize the endpoints exposed to peripheral drivers, with backlinks
 * to the rest of the driver state.
 */
static inline void __init musb_g_init_endpoints(struct musb *musb)
{
	u8			epnum;
	struct musb_hw_ep	*hw_ep;
	unsigned		count = 0;

	/* intialize endpoint list just once */
	INIT_LIST_HEAD(&(musb->g.ep_list));

	for (epnum = 0, hw_ep = musb->endpoints;
			epnum < musb->nr_endpoints;
			epnum++, hw_ep++) {
		if (hw_ep->is_shared_fifo /* || !epnum */) {
			init_peripheral_ep(musb, &hw_ep->ep_in, epnum, 0);
			count++;
		} else {
			if (hw_ep->max_packet_sz_tx) {
				init_peripheral_ep(musb, &hw_ep->ep_in,
							epnum, 1);
				count++;
			}
			if (hw_ep->max_packet_sz_rx) {
				init_peripheral_ep(musb, &hw_ep->ep_out,
							epnum, 0);
				count++;
			}
		}
	}
}

/* called once during driver setup to initialize and link into
 * the driver model; memory is zeroed.
 */
int __init musb_gadget_setup(struct musb *musb)
{
	int status;
	/* REVISIT minor race:  if (erroneously) setting up two
	 * musb peripherals at the same time, only the bus lock
	 * is probably held.
	 */
	if (the_gadget)
		return -EBUSY;
	the_gadget = musb;

	musb->g.ops = &musb_gadget_operations;
	musb->g.max_speed = USB_SPEED_HIGH;
	musb->g.speed = USB_SPEED_UNKNOWN;

	/* this "gadget" abstracts/virtualizes the controller */
	dev_set_name(&musb->g.dev, "gadget");
	musb->g.dev.parent = musb->controller;
	musb->g.dev.dma_mask = musb->controller->dma_mask;
	musb->g.dev.release = musb_gadget_release;
	musb->g.name = musb_driver_name;

//	if (is_otg_enabled(musb))
//		musb->g.is_otg = 1;

	musb_g_init_endpoints(musb);

	musb->is_active = 0;

	status = device_register(&musb->g.dev);
	if (status != 0){
		put_device(&musb->g.dev);
		the_gadget = NULL;
	}
	return status;
}

void musb_gadget_cleanup(struct musb *musb)
{
	if (musb != the_gadget)
		return;

	device_unregister(&musb->g.dev);
	the_gadget = NULL;
}


int usb_gadget_probe_driver(struct usb_gadget_driver *driver,
		int (*bind)(struct usb_gadget *))
{
	struct musb		*musb = the_gadget;
	unsigned long		flags;
	int			retval = -EINVAL;

	if (!driver
			|| !bind || !driver->setup)
	{
		goto err0;
	}

	/* driver must be initialized to support peripheral mode */
	if (!musb) {
		DBG(0, "%s, no dev??\n", __func__);
		retval = -ENODEV;
		goto err0;
	}

	//pm_runtime_get_sync(musb->controller);
	DBG(2, "registering driver %s\n", driver->function);
	//spin_lock_irqsave(&musb->lock, flags);

	if (musb->gadget_driver) {
		dev_dbg(musb->controller, "%s is already bound to %s\n",
				musb_driver_name,
				musb->gadget_driver->driver.name);
		retval = -EBUSY;
		goto err0;
	}

	spin_lock_irqsave(&musb->lock, flags);
	musb->gadget_driver = driver;
	musb->g.dev.driver = &driver->driver;
	driver->driver.bus = NULL;
	musb->softconnect = 1;
	spin_unlock_irqrestore(&musb->lock, flags);

	retval = bind(&musb->g);
	if (retval) {
		DBG(0, "bind to driver %s failed --> %d\n",
					driver->driver.name, retval);
		goto err1;
	}

	spin_lock_irqsave(&musb->lock, flags);

	//musb->is_active = 1;
	//musb->is_ready = TRUE;

	spin_unlock_irqrestore(&musb->lock, flags);

	/*
	 * FIXME this ignores the softconnect flag.  Drivers are
	 * allowed hold the peripheral inactive until for example
	 * userspace hooks up printer hardware or DSP codecs, so
	 * hosts only see fully functional devices.
	 */

	if (is_otg_enabled(musb)) {
		struct usb_hcd	*hcd = musb_to_hcd(musb);

		DBG(3, "OTG startup...\n");

		/* REVISIT:  funcall to other code, which also
		 * handles power budgeting ... this way also
		 * ensures HdrcStart is indirectly called.
		 */
		retval = usb_add_hcd(musb_to_hcd(musb), -1, 0);
		if (retval < 0) {
			DBG(0, "add_hcd failed, %d\n", retval);
			goto err2;
		}

		hcd->self.uses_pio_for_control = 1;
	}

	return 0;

err2:
	if (!is_otg_enabled(musb))
		musb_stop(musb);

err1:
	musb->gadget_driver = NULL;
	musb->g.dev.driver = NULL;

err0:
	return retval;
}
EXPORT_SYMBOL(usb_gadget_probe_driver);


static void stop_activity(struct musb *musb, struct usb_gadget_driver *driver)
{
	int			i;
	struct musb_hw_ep	*hw_ep;

	/* don't disconnect if it's not connected */
	if (musb->g.speed == USB_SPEED_UNKNOWN)
		driver = NULL;
	else
		musb->g.speed = USB_SPEED_UNKNOWN;

	/* deactivate the hardware */
	if (musb->softconnect) {
		musb->softconnect = 0;
		musb_pullup(musb, 0);
	}
	musb_stop(musb);

	/* killing any outstanding requests will quiesce the driver;
	 * then report disconnect
	 */
	if (driver) {
		for (i = 0, hw_ep = musb->endpoints;
				i < musb->nr_endpoints;
				i++, hw_ep++) {
			musb_ep_select(musb->mregs, i);
			if (hw_ep->is_shared_fifo /* || !epnum */) {
				nuke(&hw_ep->ep_in, -ESHUTDOWN);
			} else {
				if (hw_ep->max_packet_sz_tx)
					nuke(&hw_ep->ep_in, -ESHUTDOWN);
				if (hw_ep->max_packet_sz_rx)
					nuke(&hw_ep->ep_out, -ESHUTDOWN);
			}
		}

		spin_unlock(&musb->lock);
		driver->disconnect(&musb->g);
		spin_lock(&musb->lock);
	}
}

/*
 * Unregister the gadget driver. Used by gadget drivers when
 * unregistering themselves from the controller.
 *
 * @param driver the gadget driver to unregister
 */
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	unsigned long	flags;
	//int		retval = 0;
	struct musb	*musb = the_gadget;

	if (!driver || !driver->unbind || !musb)
		return -EINVAL;

	if (!musb->gadget_driver)
		return -EINVAL;

	/* REVISIT always use otg_set_peripheral() here too;
	 * this needs to shut down the OTG engine.
	 */

	spin_lock_irqsave(&musb->lock, flags);

//	if (musb->gadget_driver == driver) {


		stop_activity(musb, driver);

		spin_unlock_irqrestore(&musb->lock, flags);
		driver->unbind(&musb->g);
		spin_lock_irqsave(&musb->lock, flags);

		musb->gadget_driver = NULL;
		musb->g.dev.driver = NULL;

		musb->is_active = 0;
//	} else
//		retval = -EINVAL;
	spin_unlock_irqrestore(&musb->lock, flags);

//	if (is_otg_enabled(musb) && retval == 0) {
	if (is_otg_enabled(musb)) {
		usb_remove_hcd(musb_to_hcd(musb));
		/* FIXME we need to be able to register another
		 * gadget driver here and have everything work;
		 * that currently misbehaves.
		 */
	}

	if (!is_otg_enabled(musb))
		musb_stop(musb);

//	return retval;
	return 0;
}
EXPORT_SYMBOL(usb_gadget_unregister_driver);


/* ----------------------------------------------------------------------- */

/* lifecycle operations called through plat_uds.c */

void musb_g_resume(struct musb *musb)
{

}

/* called when SOF packets stop for 3+ msec */
void musb_g_suspend(struct musb *musb)
{
	u8	devctl;

	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
	DBG(3, "devctl %02x\n", devctl);


	musb->is_suspended = 1;
	if (musb->gadget_driver && musb->gadget_driver->suspend) {
		spin_unlock(&musb->lock);
		musb->gadget_driver->suspend(&musb->g);
		spin_lock(&musb->lock);
	}

	musb_sync_with_bat(musb,USB_SUSPEND); // announce to the battery
	wake_unlock(&musb->usb_lock);
	wake_lock_timeout(&musb->usb_lock, 5 * HZ);
}


/* called when VBUS drops below session threshold, and in other cases */
void musb_g_disconnect(struct musb *musb)
{
	musb->g.speed = USB_SPEED_UNKNOWN;
	if (musb->gadget_driver && musb->gadget_driver->disconnect) {
		DBG(0,"MUSB: call gadget disconnect \n");
		spin_unlock(&musb->lock);
		musb->gadget_driver->disconnect(&musb->g);
		spin_lock(&musb->lock);
	}

	musb->is_active = 0;
	wake_unlock(&musb->usb_lock);
}

void musb_gadget_stop(struct musb *musb)
{
	int i = 0;
	struct usb_ep* ep = NULL;

	dumpTime(funcWriteb, 0);
	musb_writeb(musb->mregs,MUSB_FADDR,0);

	musb->next_ep_num_in = 1;
	musb->next_ep_num_out = 1;
	musb->get_desc_cmd = false;
	musb->fifo_addr = FIFO_START_ADDR;
	while(i<MUSB_C_NUM_EPS)
	{
		if(ep_in_list[i]!=NULL)
		{
			DBG(0,"ep in list %d\n",i);
			ep = &ep_in_list[i]->ep_in.end_point;
			ep->ops->fifo_flush(ep); // flush fifo fisrt
			ep->ops->disable(ep); // then disable ep and will release dma
		}
		if(ep_out_list[i]!=NULL)
		{
			DBG(0,"ep out list %d\n",i);
			ep = &ep_out_list[i]->ep_out.end_point;
			ep->ops->fifo_flush(ep); // flush fifo fisrt
			ep->ops->disable(ep); // then disable ep and will release dma
		}
	//	ep_in_list[i] = NULL;
	//	ep_out_list[i] = NULL;
		i++;
	}
}

void musb_g_reset(struct musb *musb)
__releases(musb->lock)
__acquires(musb->lock)
{
	void __iomem	*mbase = musb->mregs;
//	u8		devctl = musb_readb(mbase, MUSB_DEVCTL);
	u8		power;

	DBG(2, "<== %s addr=%x driver '%s'\n",
			(musb->is_host)
				? "Host" : "Device",
			musb_readb(mbase, MUSB_FADDR),
			musb->gadget_driver
				? musb->gadget_driver->driver.name
				: NULL
			);

	if(musb->test_mode == 0)
		musb_sync_with_bat(musb,USB_UNCONFIGURED);
	/* report disconnect, if we didn't already (flushing EP state) */
	if (musb->g.speed != USB_SPEED_UNKNOWN){
		musb_g_disconnect(musb);
	}

	/* start in USB_STATE_DEFAULT */
	musb->is_active = 1;
	musb->is_suspended = 0;
	MUSB_DEV_MODE(musb);
	musb->address = 0;
	musb->ep0_state = MUSB_EP0_STAGE_SETUP;

	musb->may_wakeup = 0;
	musb->g.b_hnp_enable = 0;
	musb->g.a_alt_hnp_support = 0;
	musb->g.a_hnp_support = 0;

	spin_unlock(&musb->lock);
	musb_gadget_stop(musb);

	musb->g.ep0->ops->fifo_flush(musb->g.ep0);
	spin_lock(&musb->lock);

	musb_platform_reset(musb);
	wake_lock(&musb->usb_lock);
	musb_generic_disable(musb);
	dumpTime(funcWritew, 0);
	musb_writew(mbase,MUSB_INTRTXE,0x1); //enable ep0 interrupt
	dumpTime(funcWriteb, 0);
	musb_writeb(mbase,MUSB_INTRUSBE,MUSB_INTR_SUSPEND | MUSB_INTR_RESUME | MUSB_INTR_RESET | MUSB_INTR_DISCONNECT);

	/* what speed did we negotiate? */
	power = musb_readb(mbase, MUSB_POWER);
	musb->g.speed = (power & MUSB_POWER_HSMODE)
			? USB_SPEED_HIGH : USB_SPEED_FULL;
	DBG(2,"MUSB: gadget reset power reigister is %x\n",power);
}
