/*
 * MUSB OTG driver core code
 *
 * Copyright 2005 Mentor Graphics Corporation
 * Copyright (C) 2005-2006 by Texas Instruments
 * Copyright (C) 2006-2007 Nokia Corporation
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

/*
 * Inventra (Multipoint) Dual-Role Controller Driver for Linux.
 *
 * This consists of a Host Controller Driver (HCD) and a peripheral
 * controller driver implementing the "Gadget" API; OTG support is
 * in the works.  These are normal Linux-USB controller drivers which
 * use IRQs and have no dedicated thread.
 *
 * This version of the driver has only been used with products from
 * Texas Instruments.  Those products integrate the Inventra logic
 * with other DMA, IRQ, and bus modules, as well as other logic that
 * needs to be reflected in this driver.
 *
 *
 * NOTE:  the original Mentor code here was pretty much a collection
 * of mechanisms that don't seem to have been fully integrated/working
 * for *any* Linux kernel version.  This version aims at Linux 2.6.now,
 * Key open issues include:
 *
 *  - Lack of host-side transaction scheduling, for all transfer types.
 *    The hardware doesn't do it; instead, software must.
 *
 *    This is not an issue for OTG devices that don't support external
 *    hubs, but for more "normal" USB hosts it's a user issue that the
 *    "multipoint" support doesn't scale in the expected ways.  That
 *    includes DaVinci EVM in a common non-OTG mode.
 *
 *      * Control and bulk use dedicated endpoints, and there's as
 *        yet no mechanism to either (a) reclaim the hardware when
 *        peripherals are NAKing, which gets complicated with bulk
 *        endpoints, or (b) use more than a single bulk endpoint in
 *        each direction.
 *
 *        RESULT:  one device may be perceived as blocking another one.
 *
 *      * Interrupt and isochronous will dynamically allocate endpoint
 *        hardware, but (a) there's no record keeping for bandwidth;
 *        (b) in the common case that few endpoints are available, there
 *        is no mechanism to reuse endpoints to talk to multiple devices.
 *
 *        RESULT:  At one extreme, bandwidth can be overcommitted in
 *        some hardware configurations, no faults will be reported.
 *        At the other extreme, the bandwidth capabilities which do
 *        exist tend to be severely undercommitted.  You can't yet hook
 *        up both a keyboard and a mouse to an external USB hub.
 */

/*
 * This gets many kinds of configuration information:
 *	- Kconfig for everything user-configurable
 *	- platform_device for addressing, irq, and platform_data
 *	- platform_data is mostly for board-specific informarion
 *	  (plus recentrly, SOC or family details)
 *
 * Most of the conditional compilation will (someday) vanish.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/kobject.h>
#include <linux/prefetch.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <asm/system.h>
#include <linux/switch.h>




#ifdef	CONFIG_ARM
#include <mach/hardware.h>
#include <mach/memory.h>
#include <asm/mach-types.h>
#endif

#include <linux/musb/musb_core.h>
#include <linux/musb/musbhsdma.h>

DEFINE_SPINLOCK(usb_io_lock);
volatile bool usb_is_host = false;
unsigned musb_debug = 0;
unsigned musb_UART_debug = 0;

struct musb* mtk_musb;
unsigned musb_speed = 1;
static bool ignore_vbuserr = false;

struct timeval writeTime;
struct timeval interruptTime;

module_param_named(speed, musb_speed, uint, S_IRUGO | S_IWUSR);

module_param_named(debug, musb_debug, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug message level. Default = 0");

module_param_named(dbg_uart, musb_UART_debug, uint, S_IRUGO | S_IWUSR);

#define DRIVER_AUTHOR "Mentor Graphics, Texas Instruments, Nokia"
#define DRIVER_DESC "Inventra Dual-Role USB Controller Driver"

#define MUSB_VERSION "6.0"

#define DRIVER_INFO DRIVER_DESC ", v" MUSB_VERSION

#define MUSB_DRIVER_NAME "mt_usb"
const char musb_driver_name[] = MUSB_DRIVER_NAME;

MODULE_DESCRIPTION(DRIVER_INFO);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" MUSB_DRIVER_NAME);

//For MT6573 usb reg access
#if 0
struct musb_reg musb_reg = {
    ._readb  = power_off_readb,
    ._readw  = power_off_readw,
    ._readl  = power_off_readl,
    ._writeb = power_off_writeb,
    ._writew = power_off_writew,
    ._writel = power_off_writel,
};
#endif

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

#ifdef CONFIG_USB_MTK_OTG
struct switch_dev otg_state;
u32 sw_deboun_time = 400;
module_param(sw_deboun_time,int,0644);
static int ep_config_from_table_for_host(struct musb *musb);

static void musb_id_pin_work(struct work_struct *data)
{
	//bool is_ready = mtk_musb->is_ready;
	//u8 opstate = 0;
	down(&mtk_musb->musb_lock);
	DBG(0, "work start, is_host=%d\n", mtk_musb->is_host);
	if(mtk_musb->in_ipo_off) {
		DBG(0, "do nothing due to in_ipo_off\n");
		goto out;
	}
	//mtk_musb->is_ready = FALSE;
	mtk_musb ->is_host = musb_is_host();
	DBG(0,"musb is as %s\n",mtk_musb->is_host?"host":"device");
	switch_set_state((struct switch_dev *)&otg_state,mtk_musb->is_host);
	if(mtk_musb ->is_host) {
        	//setup fifo for host mode
        ep_config_from_table_for_host(mtk_musb);
    	wake_lock(&mtk_musb->usb_lock);
    	ignore_vbuserr = false;
		musb_set_vbus(mtk_musb,true);
		musb_start(mtk_musb);
		switch_int_to_device();
	} else {
		DBG(0,"devctl is %x\n",musb_readb(mtk_musb->mregs,MUSB_DEVCTL));
		dumpTime(funcWriteb, 0);
		musb_writeb(mtk_musb->mregs,MUSB_DEVCTL,0);
		wake_unlock(&mtk_musb->usb_lock);
		musb_set_vbus(mtk_musb,FALSE);
		/*
		opstate = musb_readb(mtk_musb->mregs,MUSB_OPSTATE);
		while(opstate != OTG_IDLE)
		{
			msleep(10);
			DBG(1,"wait OTG enter IDLE,opstate is %d\n",opstate);
			opstate = musb_readb(mtk_musb->mregs,MUSB_OPSTATE);
		}
		*/
        DBG(0,"musb_stop is called\n");
        //switch_int_to_host(); // move to musb_stop
		musb_stop(mtk_musb);

	}
	//mtk_musb->is_ready = is_ready;
out:
	DBG(0, "work end, is_host=%d\n", mtk_musb->is_host);
	up(&mtk_musb->musb_lock);

}


void musb_id_pin_interrup(void)
{
	schedule_delayed_work(&mtk_musb->id_pin_work,sw_deboun_time*HZ/1000);
	DBG(1,"MUSB:id pin interrupt assert\n");
}
#endif
/*-------------------------------------------------------------------------*/

static inline struct musb *dev_to_musb(struct device *dev)
{
//#ifdef CONFIG_USB_MTK_HDRC_HCD
	/* usbcore insists dev->driver_data is a "struct hcd *" */
	//return hcd_to_musb(dev_get_drvdata(dev));
//#else
	return dev_get_drvdata(dev);
//#endif
}

/*-------------------------------------------------------------------------*/

/*
 * Load an endpoint's FIFO
 */
void musb_write_fifo(struct musb_hw_ep *hw_ep, u16 len, const u8 *src)
{
    void __iomem *fifo;
    if(mtk_musb->is_host)
	    fifo = hw_ep->fifo;
    else
	    fifo = mtk_musb->mregs + MUSB_FIFO_OFFSET(hw_ep->ep_in.current_epnum);

	prefetch((u8 *)src);

	DBG(4, "%cX ep%d fifo %p count %d buf %p\n",
			'T', hw_ep->epnum, fifo, len, src);

	/* we can't assume unaligned reads work */
	if (likely((0x01 & (unsigned long) src) == 0)) {
		u16	index = 0;

		/* best case is 32bit-aligned source address */
		if ((0x02 & (unsigned long) src) == 0) {
			if (len >= 4) {
				writesl(fifo, src + index, len >> 2);
				index += len & ~0x03;
			}
			if (len & 0x02) {
				dumpTime(funcWritew, 0);
				musb_writew(fifo, 0, *(u16 *)&src[index]);
				index += 2;
			}
		} else {
			if (len >= 2) {
				writesw(fifo, src + index, len >> 1);
				index += len & ~0x01;
			}
		}
		if (len & 0x01)
		{
			dumpTime(funcWriteb, 0);
			musb_writeb(fifo, 0, src[index]);
		}
	} else  {
		/* byte aligned */
		writesb(fifo, src, len);
	}
}

/*
 * Unload an endpoint's FIFO
 */
void musb_read_fifo(struct musb_hw_ep *hw_ep, u16 len, u8 *dst)
{
    void __iomem *fifo;
    if(mtk_musb->is_host)
        fifo = hw_ep->fifo;
    else
	    fifo = mtk_musb->mregs + MUSB_FIFO_OFFSET(hw_ep->ep_out.current_epnum);



	DBG(4, "%cX ep%d fifo %p count %d buf %p\n",
			'R', hw_ep->epnum, fifo, len, dst);

	/* we can't assume unaligned writes work */
	if (likely((0x01 & (unsigned long) dst) == 0)) {
		u16	index = 0;

		/* best case is 32bit-aligned destination address */
		if ((0x02 & (unsigned long) dst) == 0) {
			if (len >= 4) {
				readsl(fifo, dst, len >> 2);
				index = len & ~0x03;
			}
			if (len & 0x02) {
				*(u16 *)&dst[index] = musb_readw(fifo, 0);
				index += 2;
			}
		} else {
			if (len >= 2) {
				readsw(fifo, dst, len >> 1);
				index = len & ~0x01;
			}
		}
		if (len & 0x01)
			dst[index] = musb_readb(fifo, 0);
	} else  {
		/* byte aligned */
		readsb(fifo, dst, len);
	}
}



/*-------------------------------------------------------------------------*/

/* for high speed test mode; see USB 2.0 spec 7.1.20 */
static const u8 musb_test_packet[53] = {
	/* implicit SYNC then DATA0 to start */

	/* JKJKJKJK x9 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* JJKKJJKK x8 */
	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	/* JJJJKKKK x8 */
	0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
	/* JJJJJJJKKKKKKK x8 */
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	/* JJJJJJJK x8 */
	0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd,
	/* JKKKKKKK x10, JK */
	0xfc, 0x7e, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0x7e

	/* implicit CRC16 then EOP to end */
};

void musb_load_testpacket(struct musb *musb)
{
	void __iomem	*regs = musb->endpoints[0].regs;

	musb_ep_select(musb->mregs, 0);
	musb_write_fifo(musb->control_ep,
			sizeof(musb_test_packet), musb_test_packet);
	dumpTime(funcWritew, 0);
	musb_writew(regs, MUSB_CSR0, MUSB_CSR0_TXPKTRDY);
}
/*
 * Interrupt Service Routine to record USB "global" interrupts.
 * Since these do not happen often and signify things of
 * paramount importance, it seems OK to check them individually;
 * the order of the tests is specified in the manual
 *
 * @param musb instance pointer
 * @param int_usb register contents
 * @param devctl
 * @param power
 */

static irqreturn_t musb_stage0_irq(struct musb *musb, u8 int_usb,
				u8 devctl, u8 power)
{
	irqreturn_t handled = IRQ_NONE;

	DBG(3, "<== Power=%02x, DevCtl=%02x, int_usb=0x%x\n", power, devctl,
		int_usb);

	USB_LOGGER(MUSB_STAGE0_IRQ, MUSB_STAGE0_IRQ, power, devctl, int_usb);

	/* in host mode, the peripheral may issue remote wakeup.
	 * in peripheral mode, the host may resume the link.
	 * spurious RESUME irqs happen too, paired with SUSPEND.
	 */
	if (int_usb & MUSB_INTR_RESUME) {
		handled = IRQ_HANDLED;
		DBG(3, "RESUME (%s)\n",MUSB_MODE(musb) );

		if (musb->is_host) {
	//		void __iomem *mbase = musb->mregs;

		} else {
				musb_g_resume(musb);
		}
	}

#ifdef CONFIG_USB_MTK_HDRC_HCD
	/* see manual for the order of the tests */
	if (int_usb & MUSB_INTR_SESSREQ) {
		void __iomem *mbase = musb->mregs;

		if ((devctl & MUSB_DEVCTL_VBUS) == MUSB_DEVCTL_VBUS
			&& (devctl & MUSB_DEVCTL_BDEVICE)) {
			DBG(3,"SessReq while on B state\n");
			return IRQ_HANDLED;
		}


		DBG(3, "SESSION_REQUEST (%s)\n", MUSB_MODE(musb));

		/* IRQ arrives from ID pin sense or (later, if VBUS power
		 * is removed) SRP.  responses are time critical:
		 *  - turn on VBUS (with silicon-specific mechanism)
		 *  - go through A_WAIT_VRISE
		 *  - ... to A_WAIT_BCON.
		 * a_wait_vrise_tmout triggers VBUS_ERROR transitions
		 */
		dumpTime(funcWriteb, 0);
		musb_writeb(mbase, MUSB_DEVCTL, MUSB_DEVCTL_SESSION);
		musb->ep0_stage = MUSB_EP0_START;
		MUSB_HST_MODE(musb);
		musb_set_vbus(musb, 1);

		handled = IRQ_HANDLED;
	}

	if (int_usb & MUSB_INTR_VBUSERROR) {
        	int	ignore = 0;
        	DBG(0, "MUSB_WARNING : MUSB_INTR_VBUSERROR\n");
		if (musb->vbuserr_retry && !ignore_vbuserr) {
			void __iomem *mbase = musb->mregs;

			musb->vbuserr_retry--;
			ignore = 1;
			/* workaround to let HW state matchine stop waiting for VBUS dropping and restart sampling VBUS.
					add this because sometimes a short (~3ms) VBUS droop will cause HW state matching waiting forever for VBUS dropping below 0.2V
			*/
			musb_writeb(mbase, MUSB_DEVCTL, (musb_readb(mbase, MUSB_DEVCTL) & (~MUSB_DEVCTL_SESSION)));
			DBG(0,"[MUSB] stopped session for VBUSERROR interrupt\n");
			USBPHY_SET8(0x6d, 0x3c);
			USBPHY_SET8(0x6c, 0x10);
			USBPHY_CLR8(0x6c, 0x2c);
			DBG(0,"[MUSB] force PHY to idle, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d), USBPHY_READ8(0x6c));
			mdelay(5);
			USBPHY_CLR8(0x6d, 0x3c);
			USBPHY_CLR8(0x6c, 0x3c);
			DBG(0,"[MUSB] let PHY resample VBUS, 0x6d=%x, 0x6c=%x\n", USBPHY_READ8(0x6d), USBPHY_READ8(0x6c));
			musb_writeb(mbase, MUSB_DEVCTL, (musb_readb(mbase, MUSB_DEVCTL) | MUSB_DEVCTL_SESSION));
			DBG(0,"[MUSB] restart session\n");
		} else {
			musb->port1_status |=
				USB_PORT_STAT_OVERCURRENT
				| (USB_PORT_STAT_C_OVERCURRENT << 16);
		}

		DBG(0,"[MUSB]VBUS_ERROR (%02x, %s), retry #%d, port1 %08x\n",
				devctl,
				({ char *s;
				switch (devctl & MUSB_DEVCTL_VBUS) {
				case 0 << MUSB_DEVCTL_VBUS_SHIFT:
					s = "<SessEnd"; break;
				case 1 << MUSB_DEVCTL_VBUS_SHIFT:
					s = "<AValid"; break;
				case 2 << MUSB_DEVCTL_VBUS_SHIFT:
					s = "<VBusValid"; break;
				/* case 3 << MUSB_DEVCTL_VBUS_SHIFT: */
				default:
					s = "VALID"; break;
				}; s; }),
				VBUSERR_RETRY_COUNT - musb->vbuserr_retry,
				musb->port1_status);

		/* go through A_WAIT_VFALL then start a new session */
		if (!ignore) {
//<2013/1/17-20251-jessicatseng, [Pelican] Add charging IC [bq24157] function
			#if defined(MTK_FAN5405_SUPPORT) || defined(MTK_BQ24158_SUPPORT) || defined(MTK_NCP1851_SUPPORT) || defined(MTK_BQ24196_SUPPORT) || defined(MTK_BQ24157_SUPPORT)
			DBG(0, "too many VBUS error, restart power on sequence for switching charger!\n");
			schedule_work(&musb->id_pin_work);
			#else
			musb_set_vbus(musb, 0);
			DBG(0, "too many VBUS error, turn it off!\n");
			#endif
//>2013/1/21-20645-jessicatseng			
		}
		handled = IRQ_HANDLED;
	}

#endif
	if (int_usb & MUSB_INTR_SUSPEND) {
		DBG(3, "SUSPEND (%s) devctl %02x power %02x\n",
				MUSB_MODE(musb), devctl, power);
		handled = IRQ_HANDLED;


		if (musb->is_host) {
			DBG(0,"[MUSB]as host, receive suspend interrupt!\n");

		} else {
				musb_g_suspend(musb);

		}
	}

#ifdef CONFIG_USB_MTK_HDRC_HCD
	if (int_usb & MUSB_INTR_CONNECT) {
		struct usb_hcd *hcd = musb_to_hcd(musb);
	//	void __iomem *mbase = musb->mregs;

		handled = IRQ_HANDLED;
		musb->is_active = 1;
		//set_bit(HCD_FLAG_SAW_IRQ, &hcd->flags);

		musb->ep0_stage = MUSB_EP0_START;

		musb->port1_status &= ~(USB_PORT_STAT_LOW_SPEED
					|USB_PORT_STAT_HIGH_SPEED
					|USB_PORT_STAT_ENABLE
					);
		musb->port1_status |= USB_PORT_STAT_CONNECTION
					|(USB_PORT_STAT_C_CONNECTION << 16);

		/* high vs full speed is just a guess until after reset */
		if (devctl & MUSB_DEVCTL_LSDEV)
			musb->port1_status |= USB_PORT_STAT_LOW_SPEED;

		/* poke the root hub */
		MUSB_HST_MODE(musb);
		if (hcd->status_urb)
			usb_hcd_poll_rh_status(hcd);
		else
			usb_hcd_resume_root_hub(hcd);

		DBG(0, "CONNECT (%s) devctl %02x\n",
				MUSB_MODE(musb), devctl);
	}
#endif	/* CONFIG_USB_MTK_HDRC_HCD */

	if ((int_usb & MUSB_INTR_DISCONNECT) && !musb->ignore_disconnect) {
		DBG(0, "DISCONNECT (%s), devctl %02x\n",
				MUSB_MODE(musb),devctl);
		handled = IRQ_HANDLED;
		if (musb->is_host) {
			#ifdef CONFIG_USB_MTK_HDRC_HCD
			usb_hcd_resume_root_hub(musb_to_hcd(musb));
            /* lock again in musb_h_disable() */
			spin_unlock(&musb->lock);
			musb_root_disconnect(musb);
			spin_lock(&musb->lock);
			#endif

		} else {
				musb_g_disconnect(musb);

		}
	}

	/* mentor saves a bit: bus reset and babble share the same irq.
	 * only host sees babble; only peripheral sees bus reset.
	 */
	if (int_usb & MUSB_INTR_RESET) {
		handled = IRQ_HANDLED;
		if ((musb->is_host) != 0) {
			/*
			 * Looks like non-HS BABBLE can be ignored, but
			 * HS BABBLE is an error condition. For HS the solution
			 * is to avoid babble in the first place and fix what
			 * caused BABBLE. When HS BABBLE happens we can only
			 * stop the session.
			 */
			if (devctl & (MUSB_DEVCTL_FSDEV | MUSB_DEVCTL_LSDEV))
				DBG(0, "BABBLE devctl: %02x\n", devctl);
			else {
				ERR("Stopping host session -- babble\n");
				dumpTime(funcWriteb, 0);
				musb_writeb(musb->mregs, MUSB_DEVCTL, 0);
			}
		} else{
			DBG(0, "BUS RESET as %s\n", MUSB_MODE(musb));
			musb_g_reset(musb);
		}
	}
	return handled;
}

/*-------------------------------------------------------------------------*/

/*
* Program the HDRC to start (enable interrupts, dma, etc.).
*/
void musb_start(struct musb *musb)
{
	void __iomem	*regs = musb->mregs;
	int vbusdet_retry = 5;

	u8  intrusbe;
	DBG(0, "start, is_host=%d is_active=%d\n", musb->is_host, musb->is_active);
	if(musb->is_active) {
		if(musb->is_host) {
			DBG(0, "we are host now, add more interrupt devctl=%x\n", musb_readb(mtk_musb->mregs,MUSB_DEVCTL));
			dumpTime(funcWritew, 0);
			musb_writew(regs,MUSB_INTRTXE,0xffff);
			dumpTime(funcWritew, 0);
        		musb_writew(regs,MUSB_INTRRXE,0xfffe);
			dumpTime(funcWriteb, 0);
			musb_writeb(regs,MUSB_INTRUSBE,0xf7);
		}
		return;
	}
	musb_platform_enable(musb);
	musb_generic_disable(musb);

	intrusbe= musb_readb(regs,MUSB_INTRUSBE);
	//musb->is_active = 0;
	if(is_host_enabled(musb)&&musb->is_host)
	{
		u16 intrtx=0xffff;
		u16 intrrx=0xfffe;
		dumpTime(funcWritew, 0);
		musb_writew(regs,MUSB_INTRTXE,intrtx);
		dumpTime(funcWritew, 0);
        musb_writew(regs,MUSB_INTRRXE,intrrx);
		intrusbe = 0xf7;
	//	musb_set_vbus(mtk_musb,TRUE);
		while((musb_readb(mtk_musb->mregs,MUSB_DEVCTL)&MUSB_DEVCTL_VBUS)!= MUSB_DEVCTL_VBUS)
		{
			DBG(0, "VBUS error, devctl=%x, mtk_usb_power=%d, power=%d\n", musb_readb(mtk_musb->mregs,MUSB_DEVCTL), mtk_usb_power, musb->power);
			mdelay(100);
			if(vbusdet_retry--<=1) {
				ignore_vbuserr = true;
				DBG(0, "VBUS detection fail!\n");
				break;
			}
		}
	} else if(!musb->is_host){
		intrusbe |= MUSB_INTR_RESET; //device mode enable reset interrupt
	}

	dumpTime(funcWriteb, 0);
	musb_writeb(regs,MUSB_INTRUSBE,intrusbe);

	if (musb_speed) {
	/* put into basic highspeed mode and start session */
	dumpTime(funcWriteb, 0);
	musb_writeb(regs, MUSB_POWER, MUSB_POWER_SOFTCONN
						| MUSB_POWER_HSENAB
						/* ENSUSPEND wedges tusb */
						 | MUSB_POWER_ENSUSPEND
						);
	} else {
		/* put into basic fullspeed mode and start session */
		dumpTime(funcWriteb, 0);
		musb_writeb(regs, MUSB_POWER, MUSB_POWER_SOFTCONN
							/* ENSUSPEND wedges tusb */
							 | MUSB_POWER_ENSUSPEND
							);
	}
	musb->is_active = 1;
	DBG(0, "enabled\n");
}


/*Gadget stop :flush all ep fifo and disable fifo*/
//extern struct musb_hw_ep	*ep_in_list[MUSB_C_NUM_EPS];
//extern struct musb_hw_ep	*ep_out_list[MUSB_C_NUM_EPS];
extern void musb_gadget_stop(struct musb *musb);
void gadget_stop(struct musb *musb)
{
	u8 power;

	power = musb_readb(musb->mregs, MUSB_POWER);
	power &= ~MUSB_POWER_SOFTCONN;

	dumpTime(funcWriteb, 0);
	musb_writeb(musb->mregs, MUSB_POWER, power); //disconnect

	/* patch for ALPS00345130, if the disconnect followed by hw_disconnect, then the hw_disconnect
	will not notify the UsbDeviceManager due to that musb->g.speed == USB_SPEED_UNKNOWN*/
	if (musb->gadget_driver && musb->gadget_driver->disconnect) {
		musb->gadget_driver->disconnect(&musb->g);
	}

	musb_gadget_stop(musb);
	musb->g.speed = USB_SPEED_UNKNOWN;

	/*
	 * WORKAROUND:
	 *  Don't let device enter sleep too early to break the whole USB stopping procedure,
	 *  So hold a wale lock longer(500ms).
	 */
	wake_unlock(&musb->usb_lock);
	wake_lock_timeout(&musb->usb_lock, HZ/2);
}


/*
 * Make the HDRC stop (disable interrupts, etc.);
 * reversible by musb_start
 * called on gadget driver unregister
 * with controller locked, irqs blocked
 * acts as a NOP unless some role activated the hardware
 */
void musb_stop(struct musb *musb)
{
	DBG(0, "start, is_host=%d\n", musb->is_host);
	musb_generic_disable(musb);
	gadget_stop(musb);
	musb_platform_disable(musb);
	musb->is_active = 0;
#ifdef CONFIG_USB_MTK_OTG
	musb_set_vbus(mtk_musb,FALSE);
	if (musb->is_host) {
		usb_hcd_resume_root_hub(musb_to_hcd(musb));
		musb_root_disconnect(musb);
		if (wake_lock_active(&mtk_musb->usb_lock))
			wake_unlock(&mtk_musb->usb_lock);
		//spin_unlock(&musb->lock);
	}
	musb->is_host = false;
	if(musb->in_ipo_off) { // musb_stop will be called during IPO shutdown if B-cable is plugged out
		switch_int_to_host_and_mask(); // mask ID pin during IPO shutdown
	} else {
		switch_int_to_host(); // make ID pin interrupt can be re-detected when MUSB is enabled again later
	}
#endif
	DBG(0, "disabled\n");
}

static void musb_shutdown(struct platform_device *pdev)
{
	struct musb	*musb = dev_to_musb(&pdev->dev);
	unsigned long	flags;
	DBG(0, "shut down\n");
	//Modification for ALPS00408742
/*#ifdef CONFIG_USB_MTK_HDRC_HCD
	musb_set_vbus(mtk_musb,FALSE); //for host mode
#endif

*/
	//Modification for ALPS00408742
	//Modification for ALPS00402008
	//musb_platform_disable(musb); // because this function will result in sleep
	//Modification for ALPS00402008
	//Modification for ALPS00439779
	//DISCONNECT ALL THE DRIVERS TO PREVENT THEM TO KEEP GOING
	musb->g.speed = USB_SPEED_UNKNOWN;
	if (musb->gadget_driver && musb->gadget_driver->disconnect) {
		DBG(0,"MUSB: call gadget disconnect \n");
		musb->gadget_driver->disconnect(&musb->g);
	}
	//Modification for ALPS00439779

	spin_lock_irqsave(&musb->lock, flags);

	musb_generic_disable(musb);
	spin_unlock_irqrestore(&musb->lock, flags);

	//Modification for ALPS00402008
	musb_platform_disable(musb); // because this function will result in sleep
	//Modification for ALPS00402008

	//Modification for ALPS00408742
#ifdef CONFIG_USB_MTK_HDRC_HCD
	if (is_otg_enabled(musb) || is_host_enabled(musb))
	{
		printk("%s, line %d. \n", __func__, __LINE__);
		musb_set_vbus(mtk_musb,FALSE); //for host mode
	}
#endif
	//Modification for ALPS00408742

	if (!is_otg_enabled(musb) && is_host_enabled(musb))
		usb_remove_hcd(musb_to_hcd(musb));
	dumpTime(funcWriteb, 0);
	musb_writeb(musb->mregs, MUSB_DEVCTL, 0);
	musb_platform_exit(musb);


	/* FIXME power down */
}
/*
 * configure a fifo; for non-shared endpoints, this may be called
 * once for a tx fifo and once for an rx fifo.
 *
 * returns negative errno or offset for next fifo.
 */
static int __init
fifo_setup(struct musb *musb, struct musb_hw_ep  *hw_ep,
		const struct musb_fifo_cfg *cfg, u16 offset)
{
//	void __iomem	*mbase = musb->mregs;
	int	size = 0;
	u16	maxpacket = cfg->maxpacket;
	u16	c_off = offset >> 3;
	u8	c_size;

	/* expect hw_ep has already been zero-initialized */

	size = ffs(max(maxpacket, (u16) 8)) - 1;
	maxpacket = 1 << size;

	c_size = size - 3;
/*	if (cfg->mode == BUF_DOUBLE) {
		if ((offset + (maxpacket << 1)) >(musb->fifo_size))
				return -EMSGSIZE;
		c_size |= MUSB_FIFOSZ_DPB;
	}else if ((offset + maxpacket) >(musb->fifo_size))
				return -EMSGSIZE;*/

	/* configure the FIFO */
//	musb_writeb(mbase, MUSB_INDEX, hw_ep->epnum);

	switch (cfg->style) {
	case FIFO_TX:
		DBG(0,"Tx ep %d fifo size is %d fifo address is %x\n",hw_ep->epnum,maxpacket,c_off);
//		musb_write_txfifosz(mbase, c_size);
//		musb_write_txfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_tx = maxpacket;
		hw_ep->ep_in.fifo_size= maxpacket;
		hw_ep->ep_in.fifo_mode= cfg->mode;
		break;
	case FIFO_RX:
		DBG(0,"Rx ep %d fifo size is %d fifo address is %x\n",hw_ep->epnum,maxpacket,c_off);
//		musb_write_rxfifosz(mbase, c_size);
//		musb_write_rxfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;
		hw_ep->ep_out.fifo_size= maxpacket;
		hw_ep->ep_out.fifo_mode= cfg->mode;
		break;
	case FIFO_RXTX:
//		musb_write_txfifosz(mbase, c_size);
//		musb_write_txfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;

//		musb_write_rxfifosz(mbase, c_size);
//		musb_write_rxfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = hw_ep->rx_double_buffered;
		hw_ep->max_packet_sz_tx = maxpacket;

		hw_ep->is_shared_fifo = true;
		hw_ep->ep_in.fifo_size= maxpacket;
		hw_ep->ep_out.fifo_size= maxpacket;
		hw_ep->ep_in.fifo_mode= cfg->mode;
		hw_ep->ep_out.fifo_mode= cfg->mode;
		break;
	}

	/* NOTE rx and tx endpoint irqs aren't managed separately,
	 * which happens to be ok
	 */

	hw_ep->ep_mode = cfg->ep_mode; //set the ep mode:ISO INT CONT or BULK

	return offset + (maxpacket << ((c_size & MUSB_FIFOSZ_DPB) ? 1 : 0));
}

static struct musb_fifo_cfg __initdata ep0_cfg = {
	.style = FIFO_RXTX, .maxpacket = 64, .ep_mode = EP_CONT,
};

static int __init ep_config_from_table(struct musb *musb)
{
	const struct musb_fifo_cfg	*cfg;
	unsigned		i, n;
	int			offset;
	struct musb_hw_ep	*hw_ep = musb->endpoints;

	if (musb->fifo_cfg) {
		cfg = musb->fifo_cfg;
		n = musb->fifo_cfg_size;
	}else
	 	return -EINVAL;
	offset = fifo_setup(musb, hw_ep, &ep0_cfg, 0);
	/* assert(offset > 0) */
	/* NOTE:  for RTL versions >= 1.400 EPINFO and RAMINFO would
	 * be better than static musb->config->num_eps and DYN_FIFO_SIZE...
	 */

	for (i = 0; i < n; i++){
		u8	epn = cfg->hw_ep_num;

		if (epn >= MUSB_C_NUM_EPS) {
			DBG(0, "%s: invalid ep %d\n",
					musb_driver_name, epn);
			return -EINVAL;
		}
		offset = fifo_setup(musb, hw_ep + epn, cfg++, offset);
		if (offset < 0) {
			DBG(0, "%s: mem overrun, ep %d\n",
					musb_driver_name, epn);
			return -EINVAL;
		}

		epn++;
		musb->nr_endpoints = max(epn, musb->nr_endpoints);
	}

	return 0;
}

static int fifo_setup_for_host(struct musb *musb, struct musb_hw_ep  *hw_ep,
		const struct musb_fifo_cfg *cfg, u16 offset)
{
	void __iomem	*mbase = musb->mregs;
	int	size = 0;
	u16	maxpacket = cfg->maxpacket;
	u16	c_off = offset >> 3;
	u8	c_size;

	/* expect hw_ep has already been zero-initialized */
    DBG(4,"++,hw_ep->epnum=%d\n",hw_ep->epnum);
	size = ffs(max(maxpacket, (u16) 8)) - 1;
	maxpacket = 1 << size;

	c_size = size - 3;
	if (cfg->mode == BUF_DOUBLE) {
		if ((offset + (maxpacket << 1)) >(musb->fifo_size))
				return -EMSGSIZE;
		c_size |= MUSB_FIFOSZ_DPB;
	}else if ((offset + maxpacket) >(musb->fifo_size))
				return -EMSGSIZE;

	/* configure the FIFO */
	dumpTime(funcWriteb, 0);
	musb_writeb(mbase, MUSB_INDEX, hw_ep->epnum);

	switch (cfg->style) {
	case FIFO_TX:
		DBG(4,"Tx ep %d fifo size is %d fifo address is %x\n",hw_ep->epnum,c_size,c_off);
		musb_write_txfifosz(mbase, c_size);
		musb_write_txfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_tx = maxpacket;
		break;
	case FIFO_RX:
		DBG(4,"Rx ep %d fifo size is %d fifo address is %x\n",hw_ep->epnum,c_size,c_off);
		musb_write_rxfifosz(mbase, c_size);
		musb_write_rxfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;
		break;
	case FIFO_RXTX:
		musb_write_txfifosz(mbase, c_size);
		musb_write_txfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;

		musb_write_rxfifosz(mbase, c_size);
		musb_write_rxfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = hw_ep->rx_double_buffered;
		hw_ep->max_packet_sz_tx = maxpacket;

		hw_ep->is_shared_fifo = true;
		break;
	}

	hw_ep->ep_mode = cfg->ep_mode; //set the ep mode:ISO INT CONT or BULK
	return offset + (maxpacket << ((c_size & MUSB_FIFOSZ_DPB) ? 1 : 0));
}

int ep_config_from_table_for_host(struct musb *musb)
{
	const struct musb_fifo_cfg	*cfg;
	unsigned		i, n;
	int			offset;
	struct musb_hw_ep	*hw_ep = musb->endpoints;
	if (musb->fifo_cfg_host) {
		cfg = musb->fifo_cfg_host;
		n = musb->fifo_cfg_host_size;
	}else{
	 	return -EINVAL;
       }
	offset = fifo_setup_for_host(musb, hw_ep, &ep0_cfg, 0);
	/* assert(offset > 0) */
	/* NOTE:  for RTL versions >= 1.400 EPINFO and RAMINFO would
	 * be better than static musb->config->num_eps and DYN_FIFO_SIZE...
	 */

	for (i = 0; i < n; i++){
		u8	epn = cfg->hw_ep_num;

		if (epn >= MUSB_C_NUM_EPS) {
			DBG(0,"%s: invalid ep %d\n",
					musb_driver_name, epn);
			return -EINVAL;
		}
		offset = fifo_setup_for_host(musb, hw_ep + epn, cfg++, offset);
		if (offset < 0) {
			DBG(0,"%s: mem overrun, ep %d\n",
					musb_driver_name, epn);
			return -EINVAL;
		}

		epn++;
		musb->nr_endpoints = max(epn, musb->nr_endpoints);
	}

	return 0;
}

/* Initialize MUSB (M)HDRC part of the USB hardware subsystem;
 * configure endpoints, or take their config from silicon
 */
static int __init musb_core_init(struct musb *musb)
{
	u8 reg;
	char *type;
	char aInfo[90], aRevision[32], aDate[12];
	void __iomem	*mbase = musb->mregs;
	int		status = 0;
	int		i;

	/* log core options (read using indexed model) */
	reg = musb_read_configdata(mbase);

	strcpy(aInfo, (reg & MUSB_CONFIGDATA_UTMIDW) ? "UTMI-16" : "UTMI-8");
	if (reg & MUSB_CONFIGDATA_DYNFIFO) {
		strcat(aInfo, ", dyn FIFOs");
		musb->dyn_fifo = true;
	}
	if (reg & MUSB_CONFIGDATA_MPRXE) {
		strcat(aInfo, ", bulk combine");
		musb->bulk_combine = true;
	}
	if (reg & MUSB_CONFIGDATA_MPTXE) {
		strcat(aInfo, ", bulk split");
		musb->bulk_split = true;
	}
	if (reg & MUSB_CONFIGDATA_HBRXE) {
		strcat(aInfo, ", HB-ISO Rx");
		musb->hb_iso_rx = true;
	}
	if (reg & MUSB_CONFIGDATA_HBTXE) {
		strcat(aInfo, ", HB-ISO Tx");
		musb->hb_iso_tx = true;
	}
	if (reg & MUSB_CONFIGDATA_SOFTCONE)
		strcat(aInfo, ", SoftConn");

	DBG(0,"%s: ConfigData=0x%02x (%s)\n",
			musb_driver_name, reg, aInfo);

	aDate[0] = 0;

	/* log release info */
	musb->hwvers = musb_read_hwvers(mbase);
	snprintf(aRevision, 32, "%d.%d%s", MUSB_HWVERS_MAJOR(musb->hwvers),
		MUSB_HWVERS_MINOR(musb->hwvers),
		(musb->hwvers & MUSB_HWVERS_RC) ? "RC" : "");
    //check whether hub is supported.
    if(musb->hwvers & MUSB_HUB_SUPPORT){
        musb->is_multipoint = 1;
        type = "M";
        }
    else{
        musb->is_multipoint = 0;
        type = "";
        }

	/* configure ep0 */
	musb_configure_ep0(musb);

	/* discover endpoint configuration */
	musb->nr_endpoints = 1;

	status = ep_config_from_table(musb);

	if (status < 0)
		return status;

	/* finish init, and print endpoint config */
	for (i = 0; i < musb->nr_endpoints; i++) {
		struct musb_hw_ep	*hw_ep = musb->endpoints + i;

		hw_ep->fifo = MUSB_FIFO_OFFSET(i) + mbase;
		hw_ep->regs = MUSB_EP_OFFSET(i, 0) + mbase;
#ifdef CONFIG_USB_MTK_HDRC_HCD
		hw_ep->rx_reinit = 1;
		hw_ep->tx_reinit = 1;
#endif
		if (hw_ep->max_packet_sz_tx) {
			DBG(2,
				"%s: hw_ep %d%s, %smax %d\n",
				musb_driver_name, i,
				hw_ep->is_shared_fifo ? "shared" : "tx",
				hw_ep->tx_double_buffered
					? "doublebuffer, " : "",
				hw_ep->max_packet_sz_tx);
		}
		if (hw_ep->max_packet_sz_rx && !hw_ep->is_shared_fifo) {
			DBG(2,
				"%s: hw_ep %d%s, %smax %d\n",
				musb_driver_name, i,
				"rx",
				hw_ep->rx_double_buffered
					? "doublebuffer, " : "",
				hw_ep->max_packet_sz_rx);
		}
		if (!(hw_ep->max_packet_sz_tx || hw_ep->max_packet_sz_rx))
			DBG(2, "hw_ep %d not configured\n", i);
	}

	return 0;
}

/*-------------------------------------------------------------------------*/
irqreturn_t generic_interrupt(int irq, void *__hci)
{
	unsigned long	flags;
	irqreturn_t	retval = IRQ_NONE;
	struct musb	*musb = __hci;
	spin_lock_irqsave(&musb->lock, flags);
	musb_read_clear_generic_interrupt(musb);
	if (musb->int_usb || musb->int_tx || musb->int_rx)
		retval = musb_interrupt(musb);

	spin_unlock_irqrestore(&musb->lock, flags);

	return retval;
}

/*
 * handle all the irqs defined by the HDRC core. for now we expect:  other
 * irq sources (phy, dma, etc) will be handled first, musb->int_* values
 * will be assigned, and the irq will already have been acked.
 *
 * called in irq context with spinlock held, irqs blocked
 */
irqreturn_t musb_interrupt(struct musb *musb)
{
	irqreturn_t	retval = IRQ_NONE;
	u8		devctl, power;
	int		ep_num;
	u32		reg;

	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
	power = musb_readb(musb->mregs, MUSB_POWER);

	DBG(2, "** IRQ %s usb%04x tx%04x rx%04x\n",
		(musb->is_host) ? "host" : "peripheral",
		musb->int_usb, musb->int_tx, musb->int_rx);

	USB_LOGGER( MUSB_INTERRUPT, MUSB_INTERRUPT, \
		(musb->is_host) ? "Host" : "Dev", musb->int_usb, \
		musb->int_tx, musb->int_rx);

	dumpTime(funcInterrupt, 0);

#ifdef CONFIG_USB_MTK_HDRC_GADGET
	if (!musb->gadget_driver) {
		DBG(0, "No gadget driver loaded\n");
		return IRQ_HANDLED;
	}
#endif

	/* the core can interrupt us for multiple reasons; docs have
	 * a generic interrupt flowchart to follow
	 */
	if (musb->int_usb)
		retval |= musb_stage0_irq(musb, musb->int_usb,
				devctl, power);

	/* "stage 1" is handling endpoint irqs */

	/* handle endpoint 0 first */
	if (musb->int_tx & 1) {
		if (musb->is_host)
			retval |= musb_h_ep0_irq(musb);
		else
			retval |= musb_g_ep0_irq(musb);
	}

	/* RX on endpoints 1-15 */
	reg = musb->int_rx >> 1;
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			/* musb_ep_select(musb->mregs, ep_num); */
			/* REVISIT just retval = ep->rx_irq(...) */
			retval = IRQ_HANDLED;
			if (musb->is_host) {
				if (is_host_capable())
					musb_host_rx(musb, ep_num);
			} else {
				if (is_peripheral_capable())
					musb_g_rx(musb, ep_num);
			}
		}

		reg >>= 1;
		ep_num++;
	}

	/* TX on endpoints 1-15 */
	reg = musb->int_tx >> 1;
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			/* musb_ep_select(musb->mregs, ep_num); */
			/* REVISIT just retval |= ep->tx_irq(...) */
			retval = IRQ_HANDLED;
			if (musb->is_host) {
				if (is_host_capable())
					musb_host_tx(musb, ep_num);
			} else {
				if (is_peripheral_capable())
					musb_g_tx(musb, ep_num);
			}
		}
		reg >>= 1;
		ep_num++;
	}

	return retval;
}
EXPORT_SYMBOL_GPL(musb_interrupt);


void musb_dma_completion(struct musb *musb, u8 epnum, u8 transmit)
{
//	u8	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);

	/* called with controller lock already held */

	if (!epnum) {
			/* endpoint 0 */
			if (musb->is_host)
				musb_h_ep0_irq(musb);
			else
				musb_g_ep0_irq(musb);
	} else {
		/* endpoints 1..15 */
		if (transmit) {
			if (musb->is_host) {
				if (is_host_capable())
					musb_host_tx(musb, epnum);
			} else {
				if (is_peripheral_capable())
					musb_g_tx(musb, epnum);
			}
		} else {
			/* receive */
			if (musb->is_host) {
				if (is_host_capable())
					musb_host_rx(musb, epnum);
			} else {
				if (is_peripheral_capable())
					musb_g_rx(musb, epnum);
			}
		}
	}

}

/* --------------------------------------------------------------------------
 * Init support
 */

static struct musb *__init
allocate_instance(struct device *dev, void __iomem *mbase)
{
	struct musb		*musb;
	struct musb_hw_ep	*ep;
	int			epnum;
#ifdef CONFIG_USB_MTK_HDRC_HCD
	struct usb_hcd	*hcd;

	hcd = usb_create_hcd(&musb_hc_driver, dev, dev_name(dev));
	if (!hcd)
		return NULL;
	/* usbcore sets dev->driver_data to hcd, and sometimes uses that... */

	musb = hcd_to_musb(hcd);
	INIT_LIST_HEAD(&musb->control);
	INIT_LIST_HEAD(&musb->in_bulk);
	INIT_LIST_HEAD(&musb->out_bulk);

	hcd->uses_new_polling = 1;
	hcd->has_tt = 1;

	musb->vbuserr_retry = VBUSERR_RETRY_COUNT;
#else
	musb = kzalloc(sizeof *musb, GFP_KERNEL);
	if (!musb)
		return NULL;


#endif

	dev_set_drvdata(dev, musb);
	musb->mregs = mbase;
	musb->ctrl_base = mbase;
	musb->nIrq = -ENODEV;
	for (epnum = 0, ep = musb->endpoints;epnum < MUSB_C_NUM_EPS;
			epnum++, ep++) {
		ep->musb = musb;
		ep->epnum = epnum;
	}
	musb->controller = dev;
	musb->is_ready = false;
	musb->is_host = true;
	musb->in_ipo_off = false;
	return musb;
}

static void musb_free(struct musb *musb)
{
	/* this has multiple entry modes. it handles fault cleanup after
	 * probe(), where things may be partially set up, as well as rmmod
	 * cleanup after everything's been de-activated.
	 */

#ifdef CONFIG_USB_MTK_HDRC_GADGET
	musb_gadget_cleanup(musb);
#endif

	if (musb->nIrq >= 0) {
		free_irq(musb->nIrq, musb);
	}
	if (musb->dma_controller) {
		struct dma_controller	*c = musb->dma_controller;

		(void) c->stop(c);
		dma_controller_destroy(c);
	}

#ifdef CONFIG_USB_MUSB_OTG
#endif

#ifdef CONFIG_USB_MTK_HDRC_HCD
	usb_put_hcd(musb_to_hcd(musb));
#else
	kfree(musb);
#endif
}

/*
 * Perform generic per-controller initialization.
 *
 * @pDevice: the controller (already clocked, etc)
 * @nIrq: irq
 * @mregs: virtual address of controller registers,
 *	not yet corrected for platform-specific offsets
 */
static int __init
musb_init_controller(struct device *dev,void __iomem *ctrl)
{
	int			status;
	struct musb		*musb;
	struct dma_controller	*c;

	/* allocate */
	musb = allocate_instance(dev, ctrl);
	if (!musb) {
		status = -ENOMEM;
		return status;
	}
	mtk_musb = musb;
	sema_init(&musb->musb_lock, 1);
	spin_lock_init(&musb->lock);
	wake_lock_init(&musb->usb_lock, WAKE_LOCK_SUSPEND, "USB suspend lock");
	/* The musb_platform_init() call:
	 *   - adjusts musb->mregs and musb->isr if needed,
	 *   - activates clocks.
	 *   - stops powering VBUS
	 *   - assigns musb->board_set_vbus if host mode is enabled
	 *   - assigns musb->set_power, set_clock, min_power.
	 */
	musb->isr = generic_interrupt;
	status = musb_platform_init(musb);

	if (!musb->isr) {
		status = -ENODEV;
		goto fail1;
	}


	c = dma_controller_create(musb, musb->mregs); // all dma channel are released
	musb->dma_controller = c;
	if (c)
		(void) c->start(c);
	/* ideally this would be abstracted in platform setup */
	if (!musb->dma_controller)
		dev->dma_mask = NULL;

	/* be sure interrupts are disabled before connecting ISR */
	musb_generic_disable(musb); //all interupt are disabled and cleared

	/* setup musb parts of the core (especially endpoints) */
	status = musb_core_init(musb);
	if (status < 0)
		goto fail1;

	/* attach to the IRQ */
	if (request_irq(musb->nIrq, (irq_handler_t)musb->isr, IRQF_TRIGGER_LOW, dev_name(dev), musb)) {
		dev_err(dev, "request_irq %d failed!\n", musb->nIrq);
		status = -ENODEV;
		goto fail1;
	}

	/* For the host-only role, we can activate right away.
	 * (We expect the ID pin to be forcibly grounded!!)
	 * Otherwise, wait till the gadget driver hooks up.
	 */
	if (!is_otg_enabled(musb) && is_host_enabled(musb)) {
		struct usb_hcd	*hcd = musb_to_hcd(musb);

		MUSB_HST_MODE(musb);

		status = usb_add_hcd(musb_to_hcd(musb), -1, 0);

		hcd->self.uses_pio_for_control = 1;

		DBG(2, "%s mode, status %d, devctl %02x %c\n",
			"HOST", status,
			musb_readb(musb->mregs, MUSB_DEVCTL),
			(musb_readb(musb->mregs, MUSB_DEVCTL)
					& MUSB_DEVCTL_BDEVICE
				? 'B' : 'A'));
		musb_start(musb); //if host only, start musb always

	} else /* peripheral is enabled */ {
		MUSB_DEV_MODE(musb);
		status = musb_gadget_setup(musb);


		DBG(2, "%s mode, status %d, dev%02x\n",
			is_otg_enabled(musb) ? "OTG" : "PERIPHERAL",
			status,
			musb_readb(musb->mregs, MUSB_DEVCTL));

	}

#ifdef CONFIG_USB_MTK_OTG
	INIT_DELAYED_WORK(&musb->id_pin_work, musb_id_pin_work);
	otg_int_init();


//	INIT_WORK(&musb->id_pin_work, musb_id_pin_work);

#endif
	if (status < 0)
		goto fail1;
	status = musb_init_debugfs(musb);
	if (status < 0)
		goto fail2;
	return 0;

fail2:
	if (!is_otg_enabled(musb) && is_host_enabled(musb))
		usb_remove_hcd(musb_to_hcd(musb));
	else
		musb_gadget_cleanup(musb);
fail1:
	musb_platform_exit(musb);

	return status;

}

/*-------------------------------------------------------------------------*/

/* all implementations (PCI bridge to FPGA, VLYNQ, etc) should just
 * bridge to a platform device; this driver then suffices.
 */

static int __init musb_probe(struct platform_device *pdev)
{
	struct device	*dev = &pdev->dev;
	int		status;
	void __iomem	*base;

#ifdef CONFIG_MT6585_FPGA
	base = (void *)USB0_BASE;
#else
	base = (void *)USB_BASE;
#endif
	status = musb_init_controller(dev,base);
	return status;
}

static int __exit musb_remove(struct platform_device *pdev)
{
	struct musb	*musb = dev_to_musb(&pdev->dev);
//	void __iomem	*ctrl_base = musb->ctrl_base;

	/* this gets called on rmmod.
	 *  - Host mode: host may still be active
	 *  - Peripheral mode: peripheral is deactivated (or never-activated)
	 *  - OTG mode: both roles are deactivated (or never-activated)
	 */
	musb_exit_debugfs(musb);
	musb_shutdown(pdev);
//#ifdef CONFIG_USB_MTK_HDRC_HCD
	//usb_remove_hcd(musb_to_hcd(musb));
//#endif
	//musb_writeb(musb->mregs, MUSB_DEVCTL, 0);
	//musb_platform_exit(musb);
	//musb_writeb(musb->mregs, MUSB_DEVCTL, 0);

	musb_free(musb);
	return 0;
}

#ifdef CONFIG_PM

static void musb_save_context(struct musb *musb)
{
	int i;
	void __iomem *musb_base = musb->mregs;
	void __iomem *epio;

	musb->context.power = musb_readb(musb_base, MUSB_POWER);
	musb->context.intrtxe = musb_readw(musb_base, MUSB_INTRTXE);
	musb->context.intrrxe = musb_readw(musb_base, MUSB_INTRRXE);
	musb->context.intrusbe = musb_readb(musb_base, MUSB_INTRUSBE);
	musb->context.index = musb_readb(musb_base, MUSB_INDEX);
	musb->context.devctl = musb_readb(musb_base, MUSB_DEVCTL);

	musb->context.l1_int = musb_readl(musb_base, USB_L1INTM);

	for (i = 0; i < MUSB_C_NUM_EPS -1; ++i) {
		struct musb_hw_ep	*hw_ep;

		hw_ep = &musb->endpoints[i];
		if (!hw_ep)
			continue;

		epio = hw_ep->regs;
		if (!epio)
			continue;

		musb_writeb(musb_base, MUSB_INDEX, i);
		musb->context.index_regs[i].txmaxp =
			musb_readw(epio, MUSB_TXMAXP);
		musb->context.index_regs[i].txcsr =
			musb_readw(epio, MUSB_TXCSR);
		musb->context.index_regs[i].rxmaxp =
			musb_readw(epio, MUSB_RXMAXP);
		musb->context.index_regs[i].rxcsr =
			musb_readw(epio, MUSB_RXCSR);

		if (musb->dyn_fifo) {
			musb->context.index_regs[i].txfifoadd =
					musb_read_txfifoadd(musb_base);
			musb->context.index_regs[i].rxfifoadd =
					musb_read_rxfifoadd(musb_base);
			musb->context.index_regs[i].txfifosz =
					musb_read_txfifosz(musb_base);
			musb->context.index_regs[i].rxfifosz =
					musb_read_rxfifosz(musb_base);
		}
	}
}

static void musb_restore_context(struct musb *musb)
{
	int i;
	void __iomem *musb_base = musb->mregs;
	void __iomem *epio;

	musb_writeb(musb_base, MUSB_POWER, musb->context.power);
	musb_writew(musb_base, MUSB_INTRTXE, musb->context.intrtxe);
	musb_writew(musb_base, MUSB_INTRRXE, musb->context.intrrxe);
	musb_writeb(musb_base, MUSB_INTRUSBE, musb->context.intrusbe);
	musb_writeb(musb_base, MUSB_DEVCTL, musb->context.devctl);

	for (i = 0; i < MUSB_C_NUM_EPS-1; ++i) {
		struct musb_hw_ep	*hw_ep;

		hw_ep = &musb->endpoints[i];
		if (!hw_ep)
			continue;

		epio = hw_ep->regs;
		if (!epio)
			continue;

		musb_writeb(musb_base, MUSB_INDEX, i);
		musb_writew(epio, MUSB_TXMAXP,
			musb->context.index_regs[i].txmaxp);
		musb_writew(epio, MUSB_TXCSR,
			musb->context.index_regs[i].txcsr);
		musb_writew(epio, MUSB_RXMAXP,
			musb->context.index_regs[i].rxmaxp);
		musb_writew(epio, MUSB_RXCSR,
			musb->context.index_regs[i].rxcsr);

		if (musb->dyn_fifo) {
			musb_write_txfifosz(musb_base,
				musb->context.index_regs[i].txfifosz);
			musb_write_rxfifosz(musb_base,
				musb->context.index_regs[i].rxfifosz);
			musb_write_txfifoadd(musb_base,
				musb->context.index_regs[i].txfifoadd);
			musb_write_rxfifoadd(musb_base,
				musb->context.index_regs[i].rxfifoadd);
		}
	}

	musb_writeb(musb_base, MUSB_INDEX, musb->context.index);
	mb();
	/* Enable all interrupts at DMA
	 * Caution: The DMA Reg type is WRITE to SET or CLEAR
	 */
	musb_writel(musb->mregs, MUSB_HSDMA_INTR, 0xFF | (0xFF << DMA_INTR_UNMASK_SET_OFFSET));
	mb();
	musb_writel(musb_base, USB_L1INTM, musb->context.l1_int);
}

static int musb_suspend_noirq(struct device *dev)
{
	struct musb	*musb = dev_to_musb(dev);
	/*unsigned long	flags;*/

	/*No need spin lock in xxx_noirq()*/
	/*spin_lock_irqsave(&musb->lock, flags);*/

	/*Turn on USB clock, before reading a batch of regs*/
	mtk_usb_power = true;
	usb_enable_clock(true);

	musb_save_context(musb);

	/*Turn off USB clock, after finishing reading regs*/
	usb_enable_clock(false);
	mtk_usb_power = false;

	/*spin_unlock_irqrestore(&musb->lock, flags);*/
	return 0;
}

extern void musb_phy_context_restore(void);
static int musb_resume_noirq(struct device *dev)
{
	struct musb	*musb = dev_to_musb(dev);

	/*Turn on USB clock, before writing a batch of regs*/
	mtk_usb_power = true;
	usb_enable_clock(true);

	musb_restore_context(musb);
	musb_phy_context_restore();
	/*Turn off USB clock, after finishing writing regs*/
	usb_enable_clock(false);
	mtk_usb_power = false;

	return 0;
}

static const struct dev_pm_ops musb_dev_pm_ops = {
	.suspend_noirq		= musb_suspend_noirq,
	.resume_noirq		= musb_resume_noirq,
};

#define MUSB_DEV_PM_OPS (&musb_dev_pm_ops)
#else
#define MUSB_DEV_PM_OPS NULL
#endif

static struct platform_driver musb_driver = {
	.driver = {
		.name		= (char *)musb_driver_name,
		.owner		= THIS_MODULE,
		.pm		= MUSB_DEV_PM_OPS,
	},
	.remove		= __exit_p(musb_remove),
	.shutdown	= musb_shutdown,
	.probe		= musb_probe,
};


/*-------------------------------------------------------------------------*/

static int __init musb_init(void)
{
	int ret = 0;
	writeTime.tv_sec = 0;
	writeTime.tv_usec = 0;
	interruptTime.tv_sec = 0;
	interruptTime.tv_usec = 0;
#ifdef CONFIG_USB_MTK_HDRC_HCD
	if (usb_disabled())
		return 0;
#endif

	pr_info("%s: version " MUSB_VERSION ", "

		"?dma?"
		", "
#ifdef CONFIG_USB_MUSB_OTG
		"otg (peripheral+host)"
#elif defined(CONFIG_USB_MTK_HDRC_GADGET)
		"peripheral"
#elif defined(CONFIG_USB_MTK_HDRC_HCD)
		"host"
#endif
		", debug=%d\n",
		musb_driver_name, musb_debug);

#if defined(CONFIG_MT6575T_FPGA) || defined(CONFIG_MT6585_FPGA) || defined(CONFIG_MT6589_FPGA) || defined(CONFIG_MT6582_FPGA)
	add_usb_i2c_driver();
#endif

	ret = platform_driver_register(&musb_driver);
	if (ret) {
		DBG(0,"[MUSB]platform_driver_register error:(%d)\n", ret);
		return ret;
	}
	else
	{
		DBG(0,"[MUSB]platform_driver_register done!\n");
	}

#ifdef CONFIG_USB_MTK_OTG
	otg_state.name = "otg_state";
	otg_state.index = 0;
	otg_state.state = 0;

	ret = switch_dev_register(&otg_state);
	if(ret)
	{
		DBG(0,"switch_dev_register returned:%d!\n", ret);
		return 1;
	}
#endif

	return 0;
}

/* make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
fs_initcall(musb_init);

static void __exit musb_cleanup(void)
{
	platform_driver_unregister(&musb_driver);
}
module_exit(musb_cleanup);

void dumpTime(writeFunc_enum func, int epnum)
{
	struct timeval tv;
	int diffWrite = 0;
	int diffInterrupt = 0;

	if((func == funcWriteb)||(func == funcWritew))
	{
		do_gettimeofday(&tv);
		diffWrite = tv.tv_sec - writeTime.tv_sec;
		if(diffWrite > 10)
		{
			DBG(0,"[MUSB]Write Operation (%d) seconds\n", diffWrite);
			writeTime = tv;
		}
	}

	if(func == funcInterrupt)
	{
		do_gettimeofday(&tv);
		diffInterrupt = tv.tv_sec - interruptTime.tv_sec;
		if(diffInterrupt > 10)
		{
			DBG(0,"[MUSB]Interrupt Operation (%d) seconds\n", diffInterrupt);
			interruptTime = tv;
		}
	}
}


