#ifndef __MUSB_CORE_H__
#define __MUSB_CORE_H__

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb.h>
#include <linux/usb/otg.h>
#include <linux/musb/musb.h>
#include <linux/time.h>


#ifdef CONFIG_MT6589_FPGA
#include <mach/mt_typedefs.h>
#else	/* CONFIG_REAL_CHIP */

#if defined(MT6575)
#include <mach/mt_typedefs.h>
#endif

#if defined(MT6577)
#include <mach/mt_typedefs.h>
#endif
#endif
/* Temp mark*/
/* #define ENABLE_STORAGE_LOGGER */
#ifdef ENABLE_STORAGE_LOGGER
#include <mach/mt_storage_logger.h>
#endif

/*#define CONFIG_USB_MTK_HDRC_GADGET
#define CONFIG_USB_MTK_OTG
#define CONFIG_USB_MTK_HDRC
#define CONFIG_USB_MTK_DEBUG_FS
#define CONFIG_USB_MUSB_DEBUG
*/

typedef enum
{
	USB_SUSPEND = 0,
	USB_UNCONFIGURED,
	USB_CONFIGURED
}usb_state_enum;


#define SHARE_IRQ -1
extern struct musb* mtk_musb;

struct musb;
struct musb_hw_ep;
struct musb_ep;
extern volatile bool usb_is_host;

/* Helper defines for struct musb->hwvers */
#define MUSB_HWVERS_MAJOR(x)	((x >> 10) & 0x1f)
#define MUSB_HWVERS_MINOR(x)	(x & 0x3ff)
#define MUSB_HWVERS_RC		0x8000
#define MUSB_HWVERS_1300	0x52C
#define MUSB_HWVERS_1400	0x590
#define MUSB_HWVERS_1800	0x720
#define MUSB_HWVERS_1900	0x784
#define MUSB_HWVERS_2000	0x800
#define MUSB_HUB_SUPPORT    0x4000
#include <mach/mtk_musb.h>

#include "musb_debug.h"
#include "musb_dma.h"

#include "musb_io.h"
#include "musb_regs.h"

#include "musb_gadget.h"
#include <linux/usb/hcd.h>
#include "musb_host.h"
#include <linux/semaphore.h>

extern bool mtk_usb_power;

#ifdef ENABLE_STORAGE_LOGGER
#define USB_LOGGER(msg_id, func_name, ...) \
	do { \
		if(unlikely(is_dump_musb())) { \
			ADD_USB_TRACE(msg_id, func_name, __VA_ARGS__); \
		} \
	}while(0)
#else
#define USB_LOGGER(msg_id,func_name, args...) do{} while(0)
#endif

#ifdef CONFIG_USB_MTK_OTG

#define	is_peripheral_enabled(musb)	(1)
#define	is_host_enabled(musb)		(1)
#define	is_otg_enabled(musb)		(1)

/* NOTE:  otg and peripheral-only state machines start at B_IDLE.
 * OTG or host-only go to A_IDLE when ID is sensed.
 */
#define is_peripheral_active(m)		(!(m)->is_host)
#define is_host_active(m)		((m)->is_host)

#else
#define	is_peripheral_enabled(musb)	is_peripheral_capable()
#define	is_host_enabled(musb)		is_host_capable()
#define	is_otg_enabled(musb)		0

#define	is_peripheral_active(musb)	is_peripheral_capable()
#define	is_host_active(musb)		is_host_capable()
#endif

#ifdef CONFIG_PROC_FS
#include <linux/fs.h>
#define MUSB_CONFIG_PROC_FS
#endif

/****************************** PERIPHERAL ROLE *****************************/

#ifdef CONFIG_USB_MTK_HDRC_GADGET

#define	is_peripheral_capable()	(1)

extern irqreturn_t musb_g_ep0_irq(struct musb *);
extern void musb_g_tx(struct musb *, u8);
extern void musb_g_rx(struct musb *, u8);
extern void musb_g_reset(struct musb *);
extern void musb_g_suspend(struct musb *);
extern void musb_g_resume(struct musb *);
extern void musb_g_wakeup(struct musb *);
extern void musb_g_disconnect(struct musb *);

#else

#define	is_peripheral_capable()	(0)

static inline irqreturn_t musb_g_ep0_irq(struct musb *m) { return IRQ_NONE; }
static inline void musb_g_reset(struct musb *m) {}
static inline void musb_g_suspend(struct musb *m) {}
static inline void musb_g_resume(struct musb *m) {}
static inline void musb_g_wakeup(struct musb *m) {}
static inline void musb_g_disconnect(struct musb *m) {}

#endif

/****************************** HOST ROLE ***********************************/

#ifdef CONFIG_USB_MTK_HDRC_HCD

#define	is_host_capable()	(1)

extern irqreturn_t musb_h_ep0_irq(struct musb *);
extern void musb_host_tx(struct musb *, u8);
extern void musb_host_rx(struct musb *, u8);

#else

#define	is_host_capable()	(0)

static inline irqreturn_t musb_h_ep0_irq(struct musb *m) { return IRQ_NONE; }
static inline void musb_host_tx(struct musb *m, u8 e) {}
static inline void musb_host_rx(struct musb *m, u8 e) {}

#endif


/****************************** CONSTANTS ********************************/

#ifndef MUSB_C_NUM_EPS
#define MUSB_C_NUM_EPS ((u8)9)
#endif

#ifndef MUSB_MAX_END0_PACKET
#define MUSB_MAX_END0_PACKET ((u16)MUSB_EP0_FIFOSIZE)
#endif

/* host side ep0 states */
enum musb_h_ep0_state {
	MUSB_EP0_IDLE,
	MUSB_EP0_START,			/* expect ack of setup */
	MUSB_EP0_IN,			/* expect IN DATA */
	MUSB_EP0_OUT,			/* expect ack of OUT DATA */
	MUSB_EP0_STATUS,		/* expect ack of STATUS */
} __attribute__ ((packed));

/* peripheral side ep0 states */
enum musb_g_ep0_state {
	MUSB_EP0_STAGE_IDLE,		/* idle, waiting for SETUP */
	MUSB_EP0_STAGE_SETUP,		/* received SETUP */
	MUSB_EP0_STAGE_TX,		/* IN data */
	MUSB_EP0_STAGE_RX,		/* OUT data */
	MUSB_EP0_STAGE_STATUSIN,	/* (after OUT data) */
	MUSB_EP0_STAGE_STATUSOUT,	/* (after IN data) */
	MUSB_EP0_STAGE_ACKWAIT,		/* after zlp, before statusin */
} __attribute__ ((packed));


#define musb_ep_select(_mbase, _epnum) \
	musb_writeb((_mbase), MUSB_INDEX, (_epnum))
#define	MUSB_EP_OFFSET			MUSB_INDEXED_OFFSET

/****************************** FUNCTIONS ********************************/

#define MUSB_HST_MODE(_musb)\
	{ (_musb)->is_host = true; }
#define MUSB_DEV_MODE(_musb) \
	{ (_musb)->is_host = false; }

#define test_devctl_hst_mode(_x) \
	(musb_readb((_x)->mregs, MUSB_DEVCTL)&MUSB_DEVCTL_HM)

#define MUSB_MODE(musb) ((musb)->is_host ? "Host" : "Peripheral")

typedef enum
{
	funcWriteb = 0,
	funcWritew,
	funcWritel,
	funcInterrupt
}writeFunc_enum;

void dumpTime(writeFunc_enum func, int epnum);

/******************************** TYPES *************************************/

/*
 * struct musb_hw_ep - endpoint hardware (bidirectional)
 *
 * Ordered slightly for better cacheline locality.
 */
struct musb_hw_ep {
	struct musb		*musb;
	void __iomem		*fifo;
	void __iomem		*regs;

	/* index in musb->endpoints[]  */
	u8			epnum;
	enum musb_ep_mode ep_mode;

	/* hardware configuration, possibly dynamic */
	bool			is_shared_fifo;
	bool			tx_double_buffered;
	bool			rx_double_buffered;
	u16			max_packet_sz_tx;
	u16			max_packet_sz_rx;

	struct dma_channel	*tx_channel;
	struct dma_channel	*rx_channel;
#ifdef CONFIG_USB_MTK_HDRC_HCD

	/* currently scheduled peripheral endpoint */
	struct musb_qh		*in_qh;
	struct musb_qh		*out_qh;

	u8			rx_reinit;
	u8			tx_reinit;
#endif

#ifdef CONFIG_USB_MTK_HDRC_GADGET
	/* peripheral side */
	struct musb_ep		ep_in;			/* TX */
	struct musb_ep		ep_out;			/* RX */
#endif
};

static inline struct musb_request *next_in_request(struct musb_hw_ep *hw_ep)
{
#ifdef CONFIG_USB_MTK_HDRC_GADGET
	return next_request(&hw_ep->ep_in);
#else
	return NULL;
#endif
}

static inline struct musb_request *next_out_request(struct musb_hw_ep *hw_ep)
{
#ifdef CONFIG_USB_MTK_HDRC_GADGET
	return next_request(&hw_ep->ep_out);
#else
	return NULL;
#endif
}

struct musb_csr_regs {
	/* FIFO registers */
	u16 txmaxp, txcsr, rxmaxp, rxcsr;
	u16 rxfifoadd, txfifoadd;
	u8 txtype, txinterval, rxtype, rxinterval;
	u8 rxfifosz, txfifosz;
	u8 txfunaddr, txhubaddr, txhubport;
	u8 rxfunaddr, rxhubaddr, rxhubport;
};

struct musb_context_registers {

	u8 power;
	u16 intrtxe, intrrxe;
	u8 intrusbe;
	u16 frame;
	u8 index, testmode;

	u8 devctl, busctl, misc;
	u32 otg_interfsel;
	u32 l1_int;

	struct musb_csr_regs index_regs[MUSB_C_NUM_EPS];
};

/*
 * struct musb - Driver instance data.
 */
struct musb {
	/* user context lock, to seperate gadget from OTG flow */
	struct semaphore musb_lock;
	struct musb_context_registers context;
	/* device lock */
	spinlock_t		lock;
	irqreturn_t		(*isr)(int, void *);
	u16			hwvers;
	struct delayed_work	id_pin_work;

	struct musb_fifo_cfg * fifo_cfg;
	unsigned		fifo_cfg_size;
    struct musb_fifo_cfg * fifo_cfg_host;
	unsigned		fifo_cfg_host_size;
	u32 fifo_size;

/* this hub status bit is reserved by USB 2.0 and not seen by usbcore */
#define MUSB_PORT_STAT_RESUME	(1 << 31)

	u32			port1_status;

#ifdef CONFIG_USB_MTK_HDRC_HCD
	unsigned long		rh_timer;

	enum musb_h_ep0_state	ep0_stage;

	/* bulk traffic normally dedicates endpoint hardware, and each
	 * direction has its own ring of host side endpoints.
	 * we try to progress the transfer at the head of each endpoint's
	 * queue until it completes or NAKs too much; then we try the next
	 * endpoint.
	 */
	struct musb_hw_ep	*bulk_ep;

	struct list_head	control;	/* of musb_qh */
	struct list_head	in_bulk;	/* of musb_qh */
	struct list_head	out_bulk;	/* of musb_qh */

#endif

	/* called with IRQs blocked; ON/nonzero implies starting a session,
	 * and waiting at least a_wait_vrise_tmout.
	 */
	void			(*board_set_vbus)(struct musb *, int is_on);

	struct dma_controller	*dma_controller;

	struct device		*controller;
	void __iomem		*ctrl_base;
	void __iomem		*mregs;

	/* passed down from chip/board specific irq handlers */
	u8			int_usb;
	u16			int_rx;
	u16			int_tx;

	int nIrq;
	int dma_irq;
	int id_pin_irq;

	struct musb_hw_ep	 endpoints[MUSB_C_NUM_EPS];
#define control_ep		endpoints

#define VBUSERR_RETRY_COUNT	3
	u16			vbuserr_retry;
	u8 nr_endpoints;

	bool			is_host;
	bool            power;
    bool            usb_if;
	/* active means connected and not suspended */
	unsigned		is_active:1;

	/*ready means musb and gadget is ready*/
	bool        is_ready:1;

	/*indicated device is in IPO shutdown state*/
	bool	in_ipo_off;

	unsigned is_multipoint:1;
	unsigned ignore_disconnect:1;

	unsigned		hb_iso_rx:1;	/* high bandwidth iso rx? */
	unsigned		hb_iso_tx:1;	/* high bandwidth iso tx? */
	unsigned		dyn_fifo:1;	/* dynamic FIFO supported? */

	unsigned		bulk_split:1;
#define	can_bulk_split(musb,type) \
	(((type) == USB_ENDPOINT_XFER_BULK) && (musb)->bulk_split)

	unsigned		bulk_combine:1;
#define	can_bulk_combine(musb,type) \
	(((type) == USB_ENDPOINT_XFER_BULK) && (musb)->bulk_combine)

#ifdef CONFIG_USB_MTK_HDRC_GADGET
	/* is_suspended means USB B_PERIPHERAL suspend */
	unsigned		is_suspended:1;

	/* may_wakeup means remote wakeup is enabled */
	unsigned		may_wakeup:1;

	/* is_self_powered is reported in device status and the
	 * config descriptor.  is_bus_powered means B_PERIPHERAL
	 * draws some VBUS current; both can be true.
	 */
	unsigned		is_self_powered:1;
	unsigned		is_bus_powered:1;

	unsigned		set_address:1;
	unsigned		test_mode:1;
	unsigned		softconnect:1;

	u8			address;
	u8			test_mode_nr;
	u16			ackpend;		/* ep0 */
	bool        get_desc_cmd;
	u8  next_ep_num_in;
	u8  next_ep_num_out;
	u16 fifo_addr;
	enum musb_g_ep0_state	ep0_state;
	struct usb_gadget	g;			/* the gadget */
	struct usb_gadget_driver *gadget_driver;	/* its driver */
	struct wake_lock usb_lock;

#endif

#ifdef MUSB_CONFIG_PROC_FS
	struct proc_dir_entry *proc_entry;
#endif
};


static inline void musb_set_vbus(struct musb *musb, int is_on)
{
	musb->board_set_vbus(musb, is_on);
}

#ifdef CONFIG_USB_MTK_HDRC_GADGET
static inline struct musb *gadget_to_musb(struct usb_gadget *g)
{
	return container_of(g, struct musb, g);
}
#endif

static inline int musb_read_fifosize(struct musb *musb,
		struct musb_hw_ep *hw_ep, u8 epnum)
{
	void *mbase = musb->mregs;
	u8 reg = 0;

	/* read from core using indexed model */
	reg = musb_readb(mbase, MUSB_EP_OFFSET(epnum, MUSB_FIFOSIZE));
	/* 0's returned when no more endpoints */
	if (!reg)
		return -ENODEV;

	musb->nr_endpoints++;

	hw_ep->max_packet_sz_tx = 1 << (reg & 0x0f);

	/* shared TX/RX FIFO? */
	if ((reg & 0xf0) == 0xf0) {
		hw_ep->max_packet_sz_rx = hw_ep->max_packet_sz_tx;
		hw_ep->is_shared_fifo = true;
		return 0;
	} else {
		hw_ep->max_packet_sz_rx = 1 << ((reg & 0xf0) >> 4);
		hw_ep->is_shared_fifo = false;
	}

	return 0;
}

static inline void musb_configure_ep0(struct musb *musb)
{
	musb->endpoints[0].max_packet_sz_tx = MUSB_EP0_FIFOSIZE;
	musb->endpoints[0].max_packet_sz_rx = MUSB_EP0_FIFOSIZE;
	musb->endpoints[0].is_shared_fifo = true;
}


/***************************** Glue it together *****************************/

extern const char musb_driver_name[];

extern void musb_start(struct musb *musb);
extern void musb_stop(struct musb *musb);

extern void musb_write_fifo(struct musb_hw_ep *ep, u16 len, const u8 *src);
extern void musb_read_fifo(struct musb_hw_ep *ep, u16 len, u8 *dst);

extern void musb_load_testpacket(struct musb *);

extern irqreturn_t musb_interrupt(struct musb *);

extern void musb_platform_enable(struct musb *musb);
extern void musb_platform_disable(struct musb *musb);
extern void musb_generic_disable(struct musb *musb);
extern void musb_platform_reset(struct musb *musb);
extern void musb_sync_with_bat(struct musb *musb,int usb_state);
extern int __init musb_platform_init(struct musb *musb);
extern int musb_platform_exit(struct musb *musb);

extern bool musb_is_host(void);
extern void switch_int_to_device(void);
extern void switch_int_to_host(void);
extern void switch_int_to_host_and_mask(void);
extern void otg_int_init(void);
extern void musb_id_pin_interrup(void);
extern void musb_phy_reset(void);

#if defined(CONFIG_MT6575T_FPGA) || defined(CONFIG_MT6585_FPGA) || defined(CONFIG_MT6589_FPGA) || defined(CONFIG_MT6582_FPGA)
extern int add_usb_i2c_driver();
#endif

extern u8 musb_read_clear_dma_interrupt(struct musb *musb);
extern void musb_read_clear_generic_interrupt(struct musb *musb);
extern irqreturn_t generic_interrupt(int irq, void *__hci);
extern irqreturn_t dma_controller_irq(int irq, void *private_data);

#endif	/* __MUSB_CORE_H__ */
