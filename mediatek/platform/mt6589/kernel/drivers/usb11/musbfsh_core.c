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
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/wakelock.h>
#include <linux/ctype.h>

#ifdef	CONFIG_ARM
#include <mach/hardware.h>
#include <mach/memory.h>
#include <asm/mach-types.h>
#endif

#include "musbfsh_core.h"
#include "musbfsh_host.h"
#include "musbfsh_dma.h"
#include "musbfsh_hsdma.h"
#include "musbfsh_mt65xx.h"

extern irqreturn_t musbfsh_dma_controller_irq(int irq, void *private_data);

#define DRIVER_AUTHOR "Mentor Graphics, Texas Instruments, Nokia, Mediatek"
#define DRIVER_DESC "MT65xx USB1.1 Host Controller Driver"

#define MUSBFSH_VERSION "6.0"

#define DRIVER_INFO DRIVER_DESC ", v" MUSBFSH_VERSION

#define MUSBFSH_DRIVER_NAME "musbfsh_hdrc"
const char musbfsh_driver_name[] = MUSBFSH_DRIVER_NAME;

MODULE_DESCRIPTION(DRIVER_INFO);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" MUSBFSH_DRIVER_NAME);

struct wake_lock musbfsh_suspend_lock;
DEFINE_SPINLOCK(musbfs_io_lock);
/*-------------------------------------------------------------------------*/
#ifdef IC_USB
static ssize_t show_start(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "start session under IC-USB mode\n");
}
static ssize_t store_start(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	char *pvalue = NULL;
	unsigned int value = 0;
	size_t count = 0;
	u8 devctl = musbfsh_readb((unsigned char __iomem*)USB11_BASE, MUSBFSH_DEVCTL);
	value = simple_strtoul(buf,&pvalue,10);
	count = pvalue - buf;
	
	if (*pvalue && isspace(*pvalue))
		count++;
		
	if(count == size)
	{
		if(value) {			
		WARNING("[IC-USB]start session\n");
		devctl |= MUSBFSH_DEVCTL_SESSION; // wx? why not wait until device connected
		musbfsh_writeb((unsigned char __iomem*)USB11_BASE, MUSBFSH_DEVCTL, devctl);
		WARNING("[IC-USB]power on VSIM\n");
		hwPowerOn(MT65XX_POWER_LDO_VSIM, VOL_3000, "USB11-SIM");
		}
	}
	return size;
}

static DEVICE_ATTR(start, 0666, show_start, store_start);
#endif

/*-------------------------------------------------------------------------*/

static inline struct musbfsh *dev_to_musbfsh(struct device *dev)
{
	return hcd_to_musbfsh(dev_get_drvdata(dev));
}


/*-------------------------------------------------------------------------*/
//#ifdef CONFIG_MUSBFSH_PIO_ONLY
/*
 * Load an endpoint's FIFO
 */
void musbfsh_write_fifo(struct musbfsh_hw_ep *hw_ep, u16 len, const u8 *src)
{
	void __iomem *fifo = hw_ep->fifo;
	prefetch((u8 *)src);

	INFO("%cX ep%d fifo %p count %d buf %p\n",
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
				musbfsh_writew(fifo, 0, *(u16 *)&src[index]);
				index += 2;
			}
		} else {
			if (len >= 2) {
				writesw(fifo, src + index, len >> 1);
				index += len & ~0x01;
			}
		}
		if (len & 0x01)
			musbfsh_writeb(fifo, 0, src[index]);
	} else  {
		/* byte aligned */
		writesb(fifo, src, len);
	}
    
}

/*
 * Unload an endpoint's FIFO
 */
void musbfsh_read_fifo(struct musbfsh_hw_ep *hw_ep, u16 len, u8 *dst)
{
	void __iomem *fifo = hw_ep->fifo;

	INFO( "%cX ep%d fifo %p count %d buf %p\n",
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
				*(u16 *)&dst[index] = musbfsh_readw(fifo, 0);
				index += 2;
			}
		} else {
			if (len >= 2) {
				readsw(fifo, dst, len >> 1);
				index = len & ~0x01;
			}
		}
		if (len & 0x01)
			dst[index] = musbfsh_readb(fifo, 0);
	} else  {
		/* byte aligned */
		readsb(fifo, dst, len);
	}
}


/*-------------------------------------------------------------------------*/

/* for high speed test mode; see USB 2.0 spec 7.1.20 */
static const u8 musbfsh_test_packet[53] = {
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

void musbfsh_load_testpacket(struct musbfsh *musbfsh)
{
	void __iomem	*regs = musbfsh->endpoints[0].regs;

	musbfsh_ep_select(musbfsh->mregs, 0);//should be implemented
	musbfsh_write_fifo(musbfsh->control_ep,
			sizeof(musbfsh_test_packet), musbfsh_test_packet);
	musbfsh_writew(regs, MUSBFSH_CSR0, MUSBFSH_CSR0_TXPKTRDY);
}

/*-------------------------------------------------------------------------*/
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

static irqreturn_t musbfsh_stage0_irq(struct musbfsh *musbfsh, u8 int_usb,
				u8 devctl, u8 power)
{
	irqreturn_t handled = IRQ_NONE;

	/* in host mode, the peripheral may issue remote wakeup.
	 * in peripheral mode, the host may resume the link.
	 * spurious RESUME irqs happen too, paired with SUSPEND.
	 */
	if (int_usb & MUSBFSH_INTR_RESUME) {
		handled = IRQ_HANDLED;
		WARNING("RESUME!\n");

		 if (devctl & MUSBFSH_DEVCTL_HM) {
			void __iomem *mbase = musbfsh->mregs;
			/* remote wakeup?  later, GetPortStatus
			* will stop RESUME signaling
			*/

			if (power & MUSBFSH_POWER_SUSPENDM) {
				/* spurious */
				musbfsh->int_usb &= ~MUSBFSH_INTR_SUSPEND;
				WARNING("Spurious SUSPENDM\n");
			}

			power &= ~MUSBFSH_POWER_SUSPENDM;
			musbfsh_writeb(mbase, MUSBFSH_POWER,
					power | MUSBFSH_POWER_RESUME);

			musbfsh->port1_status |=
					(USB_PORT_STAT_C_SUSPEND << 16)
					| MUSBFSH_PORT_STAT_RESUME;
			musbfsh->rh_timer = jiffies
					+ msecs_to_jiffies(20);

			musbfsh->is_active = 1;
			usb_hcd_resume_root_hub(musbfsh_to_hcd(musbfsh));
			}
		} 

	/* see manual for the order of the tests */
	if (int_usb & MUSBFSH_INTR_SESSREQ) {
		/*will not run to here*/
		void __iomem *mbase = musbfsh->mregs;
		WARNING("SESSION_REQUEST\n");

		/* IRQ arrives from ID pin sense or (later, if VBUS power
		 * is removed) SRP.  responses are time critical:
		 *  - turn on VBUS (with silicon-specific mechanism)
		 *  - go through A_WAIT_VRISE
		 *  - ... to A_WAIT_BCON.
		 * a_wait_vrise_tmout triggers VBUS_ERROR transitions
		 */
		devctl |= MUSBFSH_DEVCTL_SESSION;
		musbfsh_writeb(mbase, MUSBFSH_DEVCTL, devctl);
		musbfsh->ep0_stage = MUSBFSH_EP0_START;
		musbfsh_set_vbus(musbfsh, 1);

		handled = IRQ_HANDLED;
	}

	if (int_usb & MUSBFSH_INTR_VBUSERROR) {
		int	ignore = 0;

		/* During connection as an A-Device, we may see a short
		 * current spikes causing voltage drop, because of cable
		 * and peripheral capacitance combined with vbus draw.
		 * (So: less common with truly self-powered devices, where
		 * vbus doesn't act like a power supply.)
		 *
		 * Such spikes are short; usually less than ~500 usec, max
		 * of ~2 msec.  That is, they're not sustained overcurrent
		 * errors, though they're reported using VBUSERROR irqs.
		 *
		 * Workarounds:  (a) hardware: use self powered devices.
		 * (b) software:  ignore non-repeated VBUS errors.
		 *
		 * REVISIT:  do delays from lots of DEBUG_KERNEL checks
		 * make trouble here, keeping VBUS < 4.4V ?
		 */
		if (musbfsh->vbuserr_retry) {
			void __iomem *mbase = musbfsh->mregs;

			musbfsh->vbuserr_retry--;
			ignore = 1;
			devctl |= MUSBFSH_DEVCTL_SESSION;
			musbfsh_writeb(mbase, MUSBFSH_DEVCTL, devctl);
		} else {
			musbfsh->port1_status |=
				USB_PORT_STAT_OVERCURRENT
				| (USB_PORT_STAT_C_OVERCURRENT << 16);
		}

		ERR("VBUS_ERROR (%02x, %s), retry #%d, port1_status 0x%08x\n",
				devctl,
				({ char *s;
				switch (devctl & MUSBFSH_DEVCTL_VBUS) {
				case 0 << MUSBFSH_DEVCTL_VBUS_SHIFT:
					s = "<SessEnd"; break;
				case 1 << MUSBFSH_DEVCTL_VBUS_SHIFT:
					s = "<AValid"; break;
				case 2 << MUSBFSH_DEVCTL_VBUS_SHIFT:
					s = "<VBusValid"; break;
				/* case 3 << MUSBFSH_DEVCTL_VBUS_SHIFT: */
				default:
					s = "VALID"; break;
				}; s; }),
				VBUSERR_RETRY_COUNT - musbfsh->vbuserr_retry,
				musbfsh->port1_status);

		/* go through A_WAIT_VFALL then start a new session */
		if (!ignore)
			musbfsh_set_vbus(musbfsh, 0);
		handled = IRQ_HANDLED;
	}

	if (int_usb & MUSBFSH_INTR_CONNECT) {
		struct usb_hcd *hcd = musbfsh_to_hcd(musbfsh);
#ifndef MTK_DT_SUPPORT
		wake_lock(&musbfsh_suspend_lock);
#endif

		handled = IRQ_HANDLED;
		musbfsh->is_active = 1;
		musbfsh->ep0_stage = MUSBFSH_EP0_START;
		musbfsh->port1_status &= ~(USB_PORT_STAT_LOW_SPEED
					|USB_PORT_STAT_HIGH_SPEED
					|USB_PORT_STAT_ENABLE
					);
		musbfsh->port1_status |= USB_PORT_STAT_CONNECTION
					|(USB_PORT_STAT_C_CONNECTION << 16);

		/* high vs full speed is just a guess until after reset */
		if (devctl & MUSBFSH_DEVCTL_LSDEV)
			musbfsh->port1_status |= USB_PORT_STAT_LOW_SPEED;

		if (hcd->status_urb)
			usb_hcd_poll_rh_status(hcd);
		else
			usb_hcd_resume_root_hub(hcd);

		WARNING("CONNECT ! devctl 0x%02x\n", devctl);
	}

	if (int_usb & MUSBFSH_INTR_DISCONNECT) {
		WARNING("DISCONNECT !devctl %02x\n", devctl);
		handled = IRQ_HANDLED;
		usb_hcd_resume_root_hub(musbfsh_to_hcd(musbfsh));
		musbfsh_root_disconnect(musbfsh);
#ifndef MTK_DT_SUPPORT
		wake_unlock(&musbfsh_suspend_lock);
#endif
	}

	/* mentor saves a bit: bus reset and babble share the same irq.
	 * only host sees babble; only peripheral sees bus reset.
	 */
	if (int_usb & MUSBFSH_INTR_BABBLE) {
		handled = IRQ_HANDLED;
		/*
		* Looks like non-HS BABBLE can be ignored, but
		* HS BABBLE is an error condition. For HS the solution
		* is to avoid babble in the first place and fix what
		* caused BABBLE. When HS BABBLE happens we can only
		* stop the session.
		*/
		if (devctl & (MUSBFSH_DEVCTL_FSDEV | MUSBFSH_DEVCTL_LSDEV)) {
			ERR("BABBLE devctl: %02x\n", devctl);
			//musbfsh_writeb(musbfsh->mregs, MUSBFSH_INTRUSBE, 0xf3);
		} else {
			ERR("Stopping host session -- babble\n");
			musbfsh_writeb(musbfsh->mregs, MUSBFSH_DEVCTL, 0);
		}			
	}
	
	return handled;
}

/*-------------------------------------------------------------------------*/

#if defined(MTK_DT_SUPPORT) && !defined(EVDO_DT_SUPPORT)
#define USB_WAKE_TIME 5 //wx, seconds for hold wake lock and suspend schedule, please also check cdc-acm.c to sync, they should be the same
static struct musbfsh *mtk_musbfsh;
struct wake_lock usb_resume_lock;

static void remote_wakeup_irq(void)
{
	u8		power, devctl;
	unsigned long flags;

	if(!mtk_musbfsh) {
		return;
	}
	spin_lock_irqsave(&mtk_musbfsh->lock, flags);
	
	devctl = musbfsh_readb(mtk_musbfsh->mregs, MUSBFSH_DEVCTL);
	if (devctl & MUSBFSH_DEVCTL_HM) {
		wake_lock_timeout(&usb_resume_lock, USB_WAKE_TIME * HZ);
		musbfsh_set_power(mtk_musbfsh, 1);
		power = musbfsh_readb(mtk_musbfsh->mregs, MUSBFSH_POWER);
		WARNING("Remote wakeup! power=0x%x\n", power);
		if(!(power & MUSBFSH_POWER_SUSPENDM)) {
			goto out;
		}
		
		/* remote wakeup?  later, GetPortStatus
		 * will stop RESUME signaling
		 */
		power &= ~MUSBFSH_POWER_SUSPENDM;
		musbfsh_writeb(mtk_musbfsh->mregs, MUSBFSH_POWER,
				power | MUSBFSH_POWER_RESUME);
	
		mtk_musbfsh->port1_status |=
				(USB_PORT_STAT_C_SUSPEND << 16)
				| MUSBFSH_PORT_STAT_RESUME;
		mtk_musbfsh->rh_timer = jiffies
				+ msecs_to_jiffies(20);
	
		mtk_musbfsh->is_active = 1;
		usb_hcd_resume_root_hub(musbfsh_to_hcd(mtk_musbfsh));
	}
out:	
	spin_unlock_irqrestore(&mtk_musbfsh->lock, flags);
}
#endif

/*
* Program the HDRC to start (enable interrupts, dma, etc.).
*/
void musbfsh_start(struct musbfsh *musbfsh)
{
	void __iomem	*regs = musbfsh->mregs;//base address of usb mac
	u8		devctl = musbfsh_readb(regs, MUSBFSH_DEVCTL);
    	u8		power = musbfsh_readb(regs, MUSBFSH_POWER);
    	int int_level1 = 0;
	WARNING("<== devctl 0x%02x\n", devctl);

	/*  Set INT enable registers, enable interrupts */
	musbfsh_writew(regs, MUSBFSH_INTRTXE, musbfsh->epmask);
	musbfsh_writew(regs, MUSBFSH_INTRRXE, musbfsh->epmask & 0xfffe);
	musbfsh_writeb(regs, MUSBFSH_INTRUSBE, 0xf7);
	/* enable level 1 interrupts */
	musbfsh_writew(regs, USB11_L1INTM, 0x000f);
	int_level1 = musbfsh_readw(musbfsh->mregs, USB11_L1INTM);
	INFO("Level 1 Interrupt Mask 0x%x\n", int_level1);
	int_level1 = musbfsh_readw(musbfsh->mregs, USB11_L1INTP);
	INFO("Level 1 Interrupt Polarity 0x%x\n", int_level1);
	
	/* flush pending interrupts */
	musbfsh_writew(regs, MUSBFSH_INTRTX, 0xffff);
	musbfsh_writew(regs, MUSBFSH_INTRRX, 0xffff);
	musbfsh_writeb(regs, MUSBFSH_INTRUSB, 0xff);
	musbfsh_writeb(regs, MUSBFSH_HSDMA_INTR, 0xff);

	musbfsh->is_active = 0;
	musbfsh->is_multipoint = 1;

	/* need to enable the VBUS */
#ifndef MTK_DT_SUPPORT
	musbfsh_set_vbus(musbfsh, 1);
#endif
	musbfsh_platform_enable(musbfsh);
	
#ifndef IC_USB
	/* start session, assume ID pin is hard-wired to ground */
	devctl |= MUSBFSH_DEVCTL_SESSION; // wx? why not wait until device connected
	musbfsh_writeb(regs, MUSBFSH_DEVCTL, devctl);
#endif
	
	/* disable high speed negotiate */
	power |= MUSBFSH_POWER_HSENAB;
	power |= MUSBFSH_POWER_SOFTCONN;
	//power &= ~(MUSBFSH_POWER_HSENAB); // wx, not neccesary for USB11 host
	/* enable SUSPENDM, this will put PHY into low power mode, not so "low" as save current mode, 
	    but it will be able to detect line state (remote wakeup/connect/disconnect) */
	power |= MUSBFSH_POWER_ENSUSPEND;
	musbfsh_writeb(regs, MUSBFSH_POWER, power);
	
	devctl = musbfsh_readb(regs, MUSBFSH_DEVCTL);
	power = musbfsh_readb(regs, MUSBFSH_POWER);
	INFO(" musb ready. devctl=0x%x, power=0x%x\n", devctl, power);
	mdelay(500); // wx?

#if defined(MTK_DT_SUPPORT) && !defined(EVDO_DT_SUPPORT)
	mtk_musbfsh = musbfsh;

	mt_set_gpio_out(GPIO_52_USB_SW2, GPIO_OUT_ONE); // switch 6252's USB to 6575's USB1.1 port
	wake_lock_init(&usb_resume_lock, WAKE_LOCK_SUSPEND, "USB11 wakelock"); //wx, need to be done before EINT unmask
	
	mt65xx_eint_set_sens(CUST_EINT_DT_EXT_MD_WK_UP_USB_NUM, CUST_EINT_DT_EXT_MD_WK_UP_USB_SENSITIVE); //CUST_EINT_EDGE_SENSITIVE); 
	mt65xx_eint_set_polarity(CUST_EINT_DT_EXT_MD_WK_UP_USB_NUM, CUST_EINT_DT_EXT_MD_WK_UP_USB_POLARITY);
	mt65xx_eint_set_hw_debounce(CUST_EINT_DT_EXT_MD_WK_UP_USB_NUM, CUST_EINT_DT_EXT_MD_WK_UP_USB_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_DT_EXT_MD_WK_UP_USB_NUM, CUST_EINT_DT_EXT_MD_WK_UP_USB_DEBOUNCE_EN, CUST_EINT_DT_EXT_MD_WK_UP_USB_POLARITY, remote_wakeup_irq, false);
#endif
}


static void musbfsh_generic_disable(struct musbfsh *musbfsh)
{
	void __iomem	*mbase = musbfsh->mregs;
	u16	temp;
	INFO("musbfsh_generic_disable++\r\n");
	/* disable interrupts */
	musbfsh_writeb(mbase, MUSBFSH_INTRUSBE, 0);
	musbfsh_writew(mbase, MUSBFSH_INTRTXE, 0);
	musbfsh_writew(mbase, MUSBFSH_INTRRXE, 0);

	/* off */
	musbfsh_writeb(mbase, MUSBFSH_DEVCTL, 0);

	/*  flush pending interrupts */
	temp = musbfsh_readb(mbase, MUSBFSH_INTRUSB);
	temp = musbfsh_readw(mbase, MUSBFSH_INTRTX);
	temp = musbfsh_readw(mbase, MUSBFSH_INTRRX);
}

/*
 * Make the HDRC stop (disable interrupts, etc.);
 * reversible by musbfsh_start
 * called on gadget driver unregister
 * with controller locked, irqs blocked
 * acts as a NOP unless some role activated the hardware
 */
void musbfsh_stop(struct musbfsh *musbfsh)
{
	/* stop IRQs, timers, ... */
	musbfsh_platform_disable(musbfsh);
	musbfsh_generic_disable(musbfsh);
	INFO( "HDRC disabled\n");

	/* FIXME
	 *  - mark host and/or peripheral drivers unusable/inactive
	 *  - disable DMA (and enable it in HdrcStart)
	 *  - make sure we can musbfsh_start() after musbfsh_stop(); with
	 *    OTG mode, gadget driver module rmmod/modprobe cycles that
	 *  - ...
	 */
	musbfsh_platform_try_idle(musbfsh, 0);
}

static void musbfsh_shutdown(struct platform_device *pdev)
{
	struct musbfsh	*musbfsh = dev_to_musbfsh(&pdev->dev);
	unsigned long	flags;
    INFO("musbfsh_shutdown++\r\n");
	spin_lock_irqsave(&musbfsh->lock, flags);
	musbfsh_platform_disable(musbfsh);
	musbfsh_generic_disable(musbfsh);
	musbfsh_set_vbus(musbfsh,0);
	musbfsh_set_power(musbfsh, 0);
	spin_unlock_irqrestore(&musbfsh->lock, flags);

	/* FIXME power down */
}


/*-------------------------------------------------------------------------*/
/*
 * tables defining fifo_mode values.  define more if you like.
 * for host side, make sure both halves of ep1 are set up.
 */

/* fits in 4KB */
#define MAXFIFOSIZE 8096

static struct musbfsh_fifo_cfg __initdata epx_cfg[] = {
{ .hw_ep_num =  1, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =  1, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =  2, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =  2, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =  3, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =  3, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =  4, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =  4, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =  5, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =	5, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =  6, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =	6, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =	7, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =	7, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_SINGLE},
};

static struct musbfsh_fifo_cfg __initdata ep0_cfg = {
	.style = FIFO_RXTX, .maxpacket = 64,
};
/*-------------------------------------------------------------------------*/

/*
 * configure a fifo; for non-shared endpoints, this may be called
 * once for a tx fifo and once for an rx fifo.
 *
 * returns negative errno or offset for next fifo.
 */
static int __init
fifo_setup(struct musbfsh *musbfsh, struct musbfsh_hw_ep  *hw_ep,
		const struct musbfsh_fifo_cfg *cfg, u16 offset)
{
	void __iomem	*mbase = musbfsh->mregs;
	int	size = 0;
	u16	maxpacket = cfg->maxpacket;
	u16	c_off = offset >> 3;
	u8	c_size;//will be written into the fifo register
	INFO("musbfsh::fifo_setup++,hw_ep->epnum=%d,cfg->hw_ep_num=%d\r\n",hw_ep->epnum,cfg->hw_ep_num);
	/* expect hw_ep has already been zero-initialized */

	size = ffs(max(maxpacket, (u16) 8)) - 1;
	maxpacket = 1 << size;

	c_size = size - 3;
	if (cfg->mode == BUF_DOUBLE) {
		if ((offset + (maxpacket << 1)) >
				MAXFIFOSIZE)
			return -EMSGSIZE;
		c_size |= MUSBFSH_FIFOSZ_DPB;
	} else {
		if ((offset + maxpacket) > MAXFIFOSIZE)
			return -EMSGSIZE;
	}

	/* configure the FIFO */
	musbfsh_writeb(mbase, MUSBFSH_INDEX, hw_ep->epnum);
	/* EP0 reserved endpoint for control, bidirectional;
	 * EP1 reserved for bulk, two unidirection halves.
	 */
	 if (hw_ep->epnum == 1)
		musbfsh->bulk_ep = hw_ep;
	/* REVISIT error check:  be sure ep0 can both rx and tx ... */
	switch (cfg->style) {
	case FIFO_TX:
		musbfsh_write_txfifosz(mbase, c_size);
		musbfsh_write_txfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = !!(c_size & MUSBFSH_FIFOSZ_DPB);
		hw_ep->max_packet_sz_tx = maxpacket;
		break;
	case FIFO_RX:
		musbfsh_write_rxfifosz(mbase, c_size);
		musbfsh_write_rxfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSBFSH_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;
		break;
	case FIFO_RXTX:
		musbfsh_write_txfifosz(mbase, c_size);
		musbfsh_write_txfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSBFSH_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;

		musbfsh_write_rxfifosz(mbase, c_size);
		musbfsh_write_rxfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = hw_ep->rx_double_buffered;
		hw_ep->max_packet_sz_tx = maxpacket;
		hw_ep->is_shared_fifo = true;
		break;
	}

	/* NOTE rx and tx endpoint irqs aren't managed separately,
	 * which happens to be ok
	 */
	musbfsh->epmask |= (1 << hw_ep->epnum);

	return offset + (maxpacket << ((c_size & MUSBFSH_FIFOSZ_DPB) ? 1 : 0));
}

static int __init ep_config_from_table(struct musbfsh *musbfsh)
{
	const struct musbfsh_fifo_cfg	*cfg = NULL;
	unsigned		i = 0;
	unsigned		n = 0;
	int			offset;
	struct musbfsh_hw_ep	*hw_ep = musbfsh->endpoints;
	INFO("musbfsh::ep_config_from_table++\r\n");
	if (musbfsh->config->fifo_cfg) {
		cfg = musbfsh->config->fifo_cfg;
		n = musbfsh->config->fifo_cfg_size;
		INFO("musbfsh:fifo_cfg, n=%d\n",n);
		goto done;
	}

done:
	offset = fifo_setup(musbfsh, hw_ep, &ep0_cfg, 0);
	/* assert(offset > 0) */

	/* NOTE:  for RTL versions >= 1.400 EPINFO and RAMINFO would
	 * be better than static musbfsh->config->num_eps and DYN_FIFO_SIZE...
	 */
	
	for (i = 0; i < n; i++) {
        u8 epn = cfg->hw_ep_num;
        
		if (epn >= MUSBFSH_C_NUM_EPS) {
			ERR("%s: invalid ep %d\n",musbfsh_driver_name, epn);
			return -EINVAL;
		}
		offset = fifo_setup(musbfsh, hw_ep + epn, cfg++, offset);
		if (offset < 0) {
			ERR("%s: mem overrun, ep %d\n",musbfsh_driver_name, epn);
			return -EINVAL;
		}
        
		epn++;//include ep0
		musbfsh->nr_endpoints = max(epn, musbfsh->nr_endpoints);
	}
	INFO("%s: %d/%d max ep, %d/%d memory\n",
			musbfsh_driver_name,
			n + 1, musbfsh->config->num_eps * 2 - 1,
			offset, MAXFIFOSIZE);
	
	if (!musbfsh->bulk_ep) {
		ERR("%s: missing bulk\n", musbfsh_driver_name);
		return -EINVAL;
	}
	return 0;
}


/* Initialize MUSB (M)HDRC part of the USB hardware subsystem;
 * configure endpoints, or take their config from silicon
 */
static int __init musbfsh_core_init(struct musbfsh *musbfsh)
{
	void __iomem	*mbase = musbfsh->mregs;
	int		status = 0;
	int		i;
    INFO("musbfsh_core_init\r\n");
	/* configure ep0 */
	musbfsh_configure_ep0(musbfsh);

	/* discover endpoint configuration */
	musbfsh->nr_endpoints = 1;//will update in func: ep_config_from_table
	musbfsh->epmask = 1;
	
	status = ep_config_from_table(musbfsh);
	if (status < 0)
		return status;

	/* finish init, and print endpoint config */
	for (i = 0; i < musbfsh->nr_endpoints; i++) {
		struct musbfsh_hw_ep	*hw_ep = musbfsh->endpoints + i;

		hw_ep->fifo = MUSBFSH_FIFO_OFFSET(i) + mbase;
		hw_ep->regs = MUSBFSH_EP_OFFSET(i, 0) + mbase;
		hw_ep->rx_reinit = 1;
		hw_ep->tx_reinit = 1;

		if (hw_ep->max_packet_sz_tx) {
			INFO("%s: hw_ep %d%s, %smax %d,and hw_ep->epnum=%d\n",
				musbfsh_driver_name, i,
				hw_ep->is_shared_fifo ? "shared" : "tx",
				hw_ep->tx_double_buffered
					? "doublebuffer, " : "",
				hw_ep->max_packet_sz_tx,hw_ep->epnum);
		}
		if (hw_ep->max_packet_sz_rx && !hw_ep->is_shared_fifo) {
			INFO("%s: hw_ep %d%s, %smax %d,and hw_ep->epnum=%d\n",
				musbfsh_driver_name, i,
				"rx",
				hw_ep->rx_double_buffered
					? "doublebuffer, " : "",
				hw_ep->max_packet_sz_rx,hw_ep->epnum);
		}
		if (!(hw_ep->max_packet_sz_tx || hw_ep->max_packet_sz_rx))
			INFO("hw_ep %d not configured\n", i);
	}
	return 0;
}

/*-------------------------------------------------------------------------*/
void musbfsh_read_clear_generic_interrupt(struct musbfsh *musbfsh)
{
	musbfsh->int_usb = musbfsh_readb(musbfsh->mregs, MUSBFSH_INTRUSB);
	musbfsh->int_tx = musbfsh_readw(musbfsh->mregs, MUSBFSH_INTRTX);
	musbfsh->int_rx = musbfsh_readw(musbfsh->mregs, MUSBFSH_INTRRX);
	musbfsh->int_dma =  musbfsh_readb(musbfsh->mregs, MUSBFSH_HSDMA_INTR);
	INFO( "** musbfsh::IRQ! usb%04x tx%04x rx%04x dma%04x\n",
		musbfsh->int_usb, musbfsh->int_tx, musbfsh->int_rx, musbfsh->int_dma);
	/* clear interrupt status */	
	musbfsh_writew(musbfsh->mregs, MUSBFSH_INTRTX, musbfsh->int_tx);
	musbfsh_writew(musbfsh->mregs, MUSBFSH_INTRRX, musbfsh->int_rx);
	musbfsh_writeb(musbfsh->mregs, MUSBFSH_INTRUSB, musbfsh->int_usb);
	musbfsh_writeb(musbfsh->mregs, MUSBFSH_HSDMA_INTR, musbfsh->int_dma);
}
	
static irqreturn_t generic_interrupt(int irq, void *__hci)
{
	unsigned long	flags;
	irqreturn_t	retval = IRQ_NONE;
	struct musbfsh	*musbfsh = __hci;
	u16 int_level1 = 0;
	INFO("musbfsh:generic_interrupt++\r\n");
	spin_lock_irqsave(&musbfsh->lock, flags);

	musbfsh_read_clear_generic_interrupt(musbfsh);	
	int_level1 = musbfsh_readw(musbfsh->mregs, USB11_L1INTS);
	INFO("Level 1 Interrupt Status 0x%x\n", int_level1);
	
	if (musbfsh->int_usb || musbfsh->int_tx || musbfsh->int_rx)
		retval = musbfsh_interrupt(musbfsh);
	if(musbfsh->int_dma) 
		retval = musbfsh_dma_controller_irq(irq, musbfsh->musbfsh_dma_controller);
		
	spin_unlock_irqrestore(&musbfsh->lock, flags);
	return retval;
}

/*
 * handle all the irqs defined by the HDRC core. for now we expect:  other
 * irq sources (phy, dma, etc) will be handled first, musbfsh->int_* values
 * will be assigned, and the irq will already have been acked.
 *
 * called in irq context with spinlock held, irqs blocked
 */
irqreturn_t musbfsh_interrupt(struct musbfsh *musbfsh)
{
	irqreturn_t	retval = IRQ_NONE;
	u8		devctl, power;
	int		ep_num;
	u32		reg;

	devctl = musbfsh_readb(musbfsh->mregs, MUSBFSH_DEVCTL);
	power = musbfsh_readb(musbfsh->mregs, MUSBFSH_POWER);

	INFO( "** musbfsh::devctl 0x%x power 0x%x\n", devctl, power);

	/* the core can interrupt us for multiple reasons; docs have
	 * a generic interrupt flowchart to follow
	 */
	if (musbfsh->int_usb)
		retval |= musbfsh_stage0_irq(musbfsh, musbfsh->int_usb,
				devctl, power);

	/* "stage 1" is handling endpoint irqs */

	/* handle endpoint 0 first */
	if (musbfsh->int_tx & 1) {
		retval |= musbfsh_h_ep0_irq(musbfsh);
	}

	/* RX on endpoints 1-15 */
	reg = musbfsh->int_rx >> 1;
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			/* musbfsh_ep_select(musbfsh->mregs, ep_num); */
			/* REVISIT just retval = ep->rx_irq(...) */
			retval = IRQ_HANDLED;
			musbfsh_host_rx(musbfsh, ep_num);//the real ep_num
		}

		reg >>= 1;
		ep_num++;
	}

	/* TX on endpoints 1-15 */
	reg = musbfsh->int_tx >> 1;
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			/* musbfsh_ep_select(musbfsh->mregs, ep_num); */
			/* REVISIT just retval |= ep->tx_irq(...) */
			retval = IRQ_HANDLED;
			musbfsh_host_tx(musbfsh, ep_num);
		}
		reg >>= 1;
		ep_num++;
	}
	return retval;
}


#ifndef CONFIG_MUSBFSH_PIO_ONLY
static bool __initdata use_dma = 1;

/* "modprobe ... use_dma=0" etc */
module_param(use_dma, bool, 0);
MODULE_PARM_DESC(use_dma, "enable/disable use of DMA");

void musbfsh_dma_completion(struct musbfsh *musbfsh, u8 epnum, u8 transmit)
{
    INFO("musbfsh_dma_completion++\r\n");
	/* called with controller lock already held */

	/* endpoints 1..15 */
	if (transmit) 
		musbfsh_host_tx(musbfsh, epnum);
	else 
		musbfsh_host_rx(musbfsh, epnum);
}

#else
#define use_dma			0
#endif

/* --------------------------------------------------------------------------
 * Init support
 */

static struct musbfsh *__init
allocate_instance(struct device *dev,
		struct musbfsh_hdrc_config *config, void __iomem *mbase)
{
	struct musbfsh		*musbfsh;
	struct musbfsh_hw_ep	*ep;
	int			epnum;
	struct usb_hcd	*hcd;
	INFO("musbfsh::allocate_instance++\r\n");
	hcd = usb_create_hcd(&musbfsh_hc_driver, dev, dev_name(dev));
	if (!hcd)
		return NULL;
	/* usbcore sets dev->driver_data to hcd, and sometimes uses that... */

	musbfsh = hcd_to_musbfsh(hcd);
	INIT_LIST_HEAD(&musbfsh->control);
	INIT_LIST_HEAD(&musbfsh->in_bulk);
	INIT_LIST_HEAD(&musbfsh->out_bulk);

	hcd->uses_new_polling = 1;
	hcd->has_tt = 1;

	musbfsh->vbuserr_retry = VBUSERR_RETRY_COUNT;

	musbfsh->mregs = mbase;
	musbfsh->ctrl_base = mbase;
	musbfsh->nIrq = -ENODEV;//will be update after return from this func
	musbfsh->config = config;
	BUG_ON(musbfsh->config->num_eps > MUSBFSH_C_NUM_EPS);
	for (epnum = 0, ep = musbfsh->endpoints;
			epnum < musbfsh->config->num_eps;
			epnum++, ep++) {
		ep->musbfsh = musbfsh;
		ep->epnum = epnum;
	}

	musbfsh->controller = dev;
	return musbfsh;
}
	
static void musbfsh_free(struct musbfsh *musbfsh)
{
	/* this has multiple entry modes. it handles fault cleanup after
	 * probe(), where things may be partially set up, as well as rmmod
	 * cleanup after everything's been de-activated.
	 */
    INFO("musbfsh_free++\r\n");
	if (musbfsh->nIrq >= 0) {
		if (musbfsh->irq_wake)
			disable_irq_wake(musbfsh->nIrq);
		free_irq(musbfsh->nIrq, musbfsh);
	}
	if (is_dma_capable() && musbfsh->dma_controller) {
		struct dma_controller	*c = musbfsh->dma_controller;

		(void) c->stop(c);
		musbfsh_dma_controller_destroy(c);
	}
	usb_put_hcd(musbfsh_to_hcd(musbfsh));
	kfree(musbfsh);
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
musbfsh_init_controller(struct device *dev, int nIrq, void __iomem *ctrl)
{
	int			status;
	struct musbfsh		*musbfsh;
	struct musbfsh_hdrc_platform_data *plat = dev->platform_data;
	struct usb_hcd	*hcd;
	INFO("musbfsh::musbfsh_init_controller++\r\n");
	/* The driver might handle more features than the board; OK.
	 * Fail when the board needs a feature that's not enabled.
	 */
	if (!plat) {
		dev_dbg(dev, "no platform_data?\n");
		status = -ENODEV;
		goto fail0;
	}

	/* allocate */
	musbfsh = allocate_instance(dev, plat->config, ctrl);
	if (!musbfsh) {
		status = -ENOMEM;
		goto fail0;
	}
	spin_lock_init(&musbfsh->lock);
	musbfsh->board_mode = plat->mode;
	musbfsh->config->fifo_cfg = epx_cfg;
	musbfsh->config->fifo_cfg_size = sizeof(epx_cfg)/sizeof(struct musbfsh_fifo_cfg);
	/* The musbfsh_platform_init() call:
	 *   - adjusts musbfsh->mregs and musbfsh->isr if needed,
	 *   - may initialize an integrated tranceiver
	 *   - initializes musbfsh->xceiv, usually by otg_get_transceiver()
	 *   - activates clocks.
	 *   - stops powering VBUS
	 *   - assigns musbfsh->board_set_vbus if host mode is enabled
	 *
	 * There are various transciever configurations.  Blackfin,
	 * DaVinci, TUSB60x0, and others integrate them.  OMAP3 uses
	 * external/discrete ones in various flavors (twl4030 family,
	 * isp1504, non-OTG, etc) mostly hooking up through ULPI.
	 */
	musbfsh->isr = generic_interrupt;
	status = musbfsh_platform_init(musbfsh);
	if (status < 0){
        ERR("musbfsh_platform_init fail!status=%d",status);
        goto fail1;
    }

	if (!musbfsh->isr) {
		status = -ENODEV;
		goto fail2;
	}
	
#ifndef CONFIG_MUSBFSH_PIO_ONLY
	INFO("DMA mode\n");
	if (use_dma && dev->dma_mask) {
		struct dma_controller	*c;

		c = musbfsh_dma_controller_create(musbfsh, musbfsh->mregs);//only software config
		musbfsh->dma_controller = c;
		if (c)
			(void) c->start(c);//do nothing in fact
	}
#else
	INFO("PIO mode\n");
#endif

	/* ideally this would be abstracted in platform setup */
	if (!is_dma_capable() || !musbfsh->dma_controller)
		dev->dma_mask = NULL;

	/* be sure interrupts are disabled before connecting ISR */
	musbfsh_platform_disable(musbfsh);//wz,need implement in MT65xx, but not power off!
	musbfsh_generic_disable(musbfsh);//must power on the USB module

	/* setup musb parts of the core (especially endpoints) */
	status = musbfsh_core_init(musbfsh);
	if (status < 0){
        ERR("musbfsh_core_init fail!");
		goto fail2;
       }

	/* attach to the IRQ */
	if (request_irq(nIrq, musbfsh->isr, IRQF_TRIGGER_LOW, dev_name(dev), musbfsh)) { //wx? usb_add_hcd will also try do request_irq, if hcd_driver.irq is set
		dev_err(dev, "musbfsh::request_irq %d failed!\n", nIrq);
		status = -ENODEV;
		goto fail2;
	}
	musbfsh->nIrq = nIrq; //update the musbfsh->nIrq after request_irq !
/* FIXME this handles wakeup irqs wrong */
	if (enable_irq_wake(nIrq) == 0) { //wx, need to be replaced by modifying kernel/core/mt6573_ost.c
		musbfsh->irq_wake = 1;
		device_init_wakeup(dev, 1); //wx? usb_add_hcd will do this any way
	} else {
		musbfsh->irq_wake = 0;
	}

	/* host side needs more setup */
	hcd = musbfsh_to_hcd(musbfsh);
	hcd->power_budget = 2 * (plat->power ?plat->power : 250);//plat->power can be set to 0,so the power is set to 500ma .

	/* For the host-only role, we can activate right away.
	 * (We expect the ID pin to be forcibly grounded!!)
	 * Otherwise, wait till the gadget driver hooks up.
	 */
	status = usb_add_hcd(musbfsh_to_hcd(musbfsh), -1, 0);//important!!
	if (status < 0){
         ERR("musbfsh::usb_add_hcd fail!");
		goto fail2;
       }

	dev_info(dev, "USB controller at %p using %s, IRQ %d\n",
			ctrl,
			(is_dma_capable() && musbfsh->dma_controller)
			? "DMA" : "PIO",
			musbfsh->nIrq);

	return 0;

fail2:
	if (musbfsh->irq_wake)
		device_init_wakeup(dev, 0);
	musbfsh_platform_exit(musbfsh);

fail1:
	dev_err(musbfsh->controller,
		"musbfsh_init_controller failed with status %d\n", status);
	musbfsh_free(musbfsh);

fail0:

	return status;

}

/*-------------------------------------------------------------------------*/

/* all implementations (PCI bridge to FPGA, VLYNQ, etc) should just
 * bridge to a platform device; this driver then suffices.
 */

#ifndef CONFIG_MUSBFSH_PIO_ONLY
static u64	*orig_dma_mask;
#endif

static int __init musbfsh_probe(struct platform_device *pdev)
{
	struct device	*dev = &pdev->dev;
	int		irq = MT_USB1_IRQ_ID;
	int		status;
	unsigned char __iomem	*base = (unsigned char __iomem*)USB11_BASE;
	INFO("musbfsh_probe++\r\n");
#ifndef CONFIG_MUSBFSH_PIO_ONLY//using DMA
	/* clobbered by use_dma=n */
	orig_dma_mask = dev->dma_mask;
#endif
	status = musbfsh_init_controller(dev, irq, base);//after init the controller, the USB should already be powered on!
	if (status < 0)
		ERR("musbfsh_probe failed with status %d\n", status);
	INFO("musbfsh_probe--\r\n");
#ifdef IC_USB	
	device_create_file(dev, &dev_attr_start);
	WARNING("IC-USB is enabled\n");
#endif
	return status;
}

static int __exit musbfsh_remove(struct platform_device *pdev)
{
	struct musbfsh	*musbfsh = dev_to_musbfsh(&pdev->dev);
	void __iomem	*ctrl_base = musbfsh->ctrl_base;
	INFO("musbfsh_remove++\r\n");
	/* this gets called on rmmod.
	 *  - Host mode: host may still be active
	 *  - Peripheral mode: peripheral is deactivated (or never-activated)
	 *  - OTG mode: both roles are deactivated (or never-activated)
	 */
	musbfsh_shutdown(pdev);
	if (musbfsh->board_mode == MUSBFSH_HOST)
		usb_remove_hcd(musbfsh_to_hcd(musbfsh));
	musbfsh_writeb(musbfsh->mregs, MUSBFSH_DEVCTL, 0);
	musbfsh_platform_exit(musbfsh);
	musbfsh_writeb(musbfsh->mregs, MUSBFSH_DEVCTL, 0);

	musbfsh_free(musbfsh);
	iounmap(ctrl_base);
	device_init_wakeup(&pdev->dev, 0);
#ifndef CONFIG_MUSBFSH_PIO_ONLY
	pdev->dev.dma_mask = orig_dma_mask;
#endif
	return 0;
}

#ifdef	CONFIG_PM

static void musbfsh_save_context(struct musbfsh *musbfsh)
{
	int i;
	void __iomem *musbfsh_base = musbfsh->mregs;
	void __iomem *epio;

	musbfsh->context.power = musbfsh_readb(musbfsh_base, MUSBFSH_POWER);
	musbfsh->context.intrtxe = musbfsh_readw(musbfsh_base, MUSBFSH_INTRTXE);
	musbfsh->context.intrrxe = musbfsh_readw(musbfsh_base, MUSBFSH_INTRRXE);
	musbfsh->context.intrusbe = musbfsh_readb(musbfsh_base, MUSBFSH_INTRUSBE);
	musbfsh->context.index = musbfsh_readb(musbfsh_base, MUSBFSH_INDEX);
	musbfsh->context.devctl = musbfsh_readb(musbfsh_base, MUSBFSH_DEVCTL);

	musbfsh->context.l1_int = musbfsh_readl(musbfsh_base, USB11_L1INTM);

	for (i = 0; i < MUSBFSH_C_NUM_EPS -1; ++i) {
		struct musbfsh_hw_ep	*hw_ep;

		hw_ep = &musbfsh->endpoints[i];
		if (!hw_ep)
			continue;

		epio = hw_ep->regs;
		if (!epio)
			continue;

		musbfsh_writeb(musbfsh_base, MUSBFSH_INDEX, i);
		musbfsh->context.index_regs[i].txmaxp =
			musbfsh_readw(epio, MUSBFSH_TXMAXP);
		musbfsh->context.index_regs[i].txcsr =
			musbfsh_readw(epio, MUSBFSH_TXCSR);
		musbfsh->context.index_regs[i].rxmaxp =
			musbfsh_readw(epio, MUSBFSH_RXMAXP);
		musbfsh->context.index_regs[i].rxcsr =
			musbfsh_readw(epio, MUSBFSH_RXCSR);

		if (musbfsh->dyn_fifo) {
			musbfsh->context.index_regs[i].txfifoadd =
					musbfsh_read_txfifoadd(musbfsh_base);
			musbfsh->context.index_regs[i].rxfifoadd =
					musbfsh_read_rxfifoadd(musbfsh_base);
			musbfsh->context.index_regs[i].txfifosz =
					musbfsh_read_txfifosz(musbfsh_base);
			musbfsh->context.index_regs[i].rxfifosz =
					musbfsh_read_rxfifosz(musbfsh_base);
		}
	}
}

static void musbfsh_restore_context(struct musbfsh *musbfsh)
{
	int i;
	void __iomem *musbfsh_base = musbfsh->mregs;
	void __iomem *epio;

	musbfsh_writeb(musbfsh_base, MUSBFSH_POWER, musbfsh->context.power);
	musbfsh_writew(musbfsh_base, MUSBFSH_INTRTXE, musbfsh->context.intrtxe);
	musbfsh_writew(musbfsh_base, MUSBFSH_INTRRXE, musbfsh->context.intrrxe);
	musbfsh_writeb(musbfsh_base, MUSBFSH_INTRUSBE, musbfsh->context.intrusbe);
	musbfsh_writeb(musbfsh_base, MUSBFSH_DEVCTL, musbfsh->context.devctl);

	for (i = 0; i < MUSBFSH_C_NUM_EPS-1; ++i) {
		struct musbfsh_hw_ep	*hw_ep;

		hw_ep = &musbfsh->endpoints[i];
		if (!hw_ep)
			continue;

		epio = hw_ep->regs;
		if (!epio)
			continue;

		musbfsh_writeb(musbfsh_base, MUSBFSH_INDEX, i);
		musbfsh_writew(epio, MUSBFSH_TXMAXP,
			musbfsh->context.index_regs[i].txmaxp);
		musbfsh_writew(epio, MUSBFSH_TXCSR,
			musbfsh->context.index_regs[i].txcsr);
		musbfsh_writew(epio, MUSBFSH_RXMAXP,
			musbfsh->context.index_regs[i].rxmaxp);
		musbfsh_writew(epio, MUSBFSH_RXCSR,
			musbfsh->context.index_regs[i].rxcsr);

		if (musbfsh->dyn_fifo) {
			musbfsh_write_txfifosz(musbfsh_base,
				musbfsh->context.index_regs[i].txfifosz);
			musbfsh_write_rxfifosz(musbfsh_base,
				musbfsh->context.index_regs[i].rxfifosz);
			musbfsh_write_txfifoadd(musbfsh_base,
				musbfsh->context.index_regs[i].txfifoadd);
			musbfsh_write_rxfifoadd(musbfsh_base,
				musbfsh->context.index_regs[i].rxfifoadd);
		}
	}

	musbfsh_writeb(musbfsh_base, MUSBFSH_INDEX, musbfsh->context.index);
	mb();
	/* Enable all interrupts at DMA
	 * Caution: The DMA Reg type is WRITE to SET or CLEAR
	 */
	musbfsh_writel(musbfsh->mregs, MUSBFSH_HSDMA_INTR, 0xFF | (0xFF << MUSBFSH_DMA_INTR_UNMASK_SET_OFFSET));
	//mb();
	musbfsh_writel(musbfsh_base, USB11_L1INTM, musbfsh->context.l1_int);
}


static int musbfsh_suspend(struct device *dev)
{
    //return 0;
	struct platform_device *pdev = to_platform_device(dev);
	unsigned long	flags;
	struct musbfsh	*musbfsh = dev_to_musbfsh(&pdev->dev);
    WARNING("musbfsh_suspend++\r\n");
#if 1
	spin_lock_irqsave(&musbfsh->lock, flags);
	//musbfsh_set_vbus(musbfsh,0);//disable VBUS
#ifdef CONFIG_SMP
	disable_irq(musbfsh->nIrq); // wx, clearn IRQ before closing clock. Prevent SMP issue: clock is disabled on CPU1, but ISR is running on CPU0 and failed to clear interrupt
	musbfsh_save_context(musbfsh);
	musbfsh_read_clear_generic_interrupt(musbfsh);	
#endif
	musbfsh_set_power(musbfsh, 0);
	spin_unlock_irqrestore(&musbfsh->lock, flags);
#endif
	return 0;
}

static int musbfsh_resume(struct device *dev)
{
    //return 0;
	struct platform_device *pdev = to_platform_device(dev);
	unsigned long	flags;
	struct musbfsh	*musbfsh = dev_to_musbfsh(&pdev->dev);
    WARNING("musbfsh_resume_noirq++\r\n");
#if 1
	spin_lock_irqsave(&musbfsh->lock, flags);
	//musbfsh_set_vbus(musbfsh,1);//enable VBUS
	musbfsh_set_power(musbfsh, 1);
#ifdef CONFIG_SMP
        musbfsh_restore_context(musbfsh);
	enable_irq(musbfsh->nIrq);
#endif
	spin_unlock_irqrestore(&musbfsh->lock, flags);
#endif
	return 0;
}

static const struct dev_pm_ops musbfsh_dev_pm_ops = {
	.suspend	= musbfsh_suspend,
	.resume	= musbfsh_resume,
};

#define MUSBFSH_DEV_PM_OPS (&musbfsh_dev_pm_ops)
#else
#define	MUSBFSH_DEV_PM_OPS	NULL
#endif

static struct platform_driver musbfsh_driver = {
	.driver = {
		.name       ="musbfsh_hdrc",
		.bus		= &platform_bus_type,
		.owner		= THIS_MODULE,
		.pm		= MUSBFSH_DEV_PM_OPS,
	},
	.probe      = musbfsh_probe,
	.remove		= __exit_p(musbfsh_remove),
	.shutdown	= musbfsh_shutdown,
};


/*-------------------------------------------------------------------------*/

static int __init musbfsh_init(void)
{
	if (usb_disabled())//based on the config variable.
		return 0;
	WARNING("MUSBFSH is enabled\n");
	wake_lock_init(&musbfsh_suspend_lock, WAKE_LOCK_SUSPEND, "usb11 host");
#ifdef CONFIG_MTK_LDVT
	__raw_writew(0x2200, 0xF0000000); /* disable watchdog */
#endif
	return platform_driver_register(&musbfsh_driver);
}

/* make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
module_init(musbfsh_init);

static void __exit musbfsh_cleanup(void)
{
	wake_lock_destroy(&musbfsh_suspend_lock);
	platform_driver_unregister(&musbfsh_driver);
}
module_exit(musbfsh_cleanup);

int musbfsh_debug = 0;
module_param(musbfsh_debug, int, 0644);
