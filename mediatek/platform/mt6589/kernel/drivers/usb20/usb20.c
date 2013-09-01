/*
 * MUSB OTG controller driver for Blackfin Processors
 *
 * Copyright 2006-2008 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/xlog.h>
#include <mach/irqs.h>
#include <mach/eint.h>

#include <linux/musb/musb_core.h>
#include <mach/mtk_musb.h>
#include <linux/musb/musbhsdma.h>
#ifndef CONFIG_MT6589_FPGA
#include <cust_gpio_usage.h>
#endif
#include <linux/switch.h>
#include <mach/mt_pm_ldo.h>
#include <mach/mt_clkmgr.h>
#include <linux/i2c.h>
#include "mach/emi_mpu.h"

#define DEVICE_INTTERRUPT 1
#define EINT_CHR_DET_NUM	23

#ifdef ID_PIN_USE_EX_EINT
#define ID_PIN_EINT 1
#define ID_PIN_GPIO GPIO112
#define GPIO_ID_PIN_EINT_PIN_M_EINT GPIO_MODE_05
#else
#define ID_PIN_EINT 28
#define U2PHYDTM1  (USB_SIF_BASE+0x800 + 0x6c)
#define ID_PULL_UP 0x0101
#define ID_PHY_RESET 0x3d11
#endif

static DEFINE_SEMAPHORE(power_clock_lock);

static bool platform_init_first = true;

#ifndef CONFIG_MT6589_FPGA
extern void upmu_interrupt_chrdet_int_en(kal_uint32 val);
#endif

/*EP Fifo Config*/
static struct musb_fifo_cfg __initdata fifo_cfg[] = {
{ .hw_ep_num =  1, .style = FIFO_TX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_DOUBLE},
{ .hw_ep_num =  1, .style = FIFO_RX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_DOUBLE},
{ .hw_ep_num =  2, .style = FIFO_TX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_DOUBLE},
{ .hw_ep_num =  2, .style = FIFO_RX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_DOUBLE},
{ .hw_ep_num =  3, .style = FIFO_TX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_DOUBLE},
{ .hw_ep_num =  3, .style = FIFO_RX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_DOUBLE},
{ .hw_ep_num =  4, .style = FIFO_TX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_DOUBLE},
{ .hw_ep_num =  4, .style = FIFO_RX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_DOUBLE},
{ .hw_ep_num =  5, .style = FIFO_TX,   .maxpacket = 64, .ep_mode = EP_INT,.mode = BUF_SINGLE},
{ .hw_ep_num =	5, .style = FIFO_RX,   .maxpacket = 512, .ep_mode = EP_INT,.mode = BUF_SINGLE},
{ .hw_ep_num =  6, .style = FIFO_TX,   .maxpacket = 64, .ep_mode = EP_INT, .mode = BUF_SINGLE},
{ .hw_ep_num =	6, .style = FIFO_RX,   .maxpacket = 512, .ep_mode = EP_INT,.mode = BUF_SINGLE},
{ .hw_ep_num =	7, .style = FIFO_TX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_SINGLE},
{ .hw_ep_num =	7, .style = FIFO_RX,   .maxpacket = 512, .ep_mode = EP_BULK,.mode = BUF_SINGLE},
{ .hw_ep_num =	8, .style = FIFO_TX,   .maxpacket = 512, .ep_mode = EP_ISO,.mode = BUF_DOUBLE},
{ .hw_ep_num =	8, .style = FIFO_RX,   .maxpacket = 512, .ep_mode = EP_ISO,.mode = BUF_DOUBLE},
};

static struct musb_fifo_cfg fifo_cfg_host[] = {
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
{ .hw_ep_num =	8, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_SINGLE},
{ .hw_ep_num =	8, .style = FIFO_RX,   .maxpacket = 64,  .mode = BUF_SINGLE},
};

bool mtk_usb_power = FALSE;
u32 delay_time = 15;
module_param(delay_time,int,0644);
u32 delay_time1 = 55;
module_param(delay_time1,int,0644);
typedef enum
{
    CABLE_MODE_CHRG_ONLY = 0,
    CABLE_MODE_NORMAL,
    CABLE_MODE_HOST_ONLY,
    CABLE_MODE_MAX
} CABLE_MODE;

u32 cable_mode = CABLE_MODE_NORMAL;

#ifdef CONFIG_USB_MTK_HDRC_HCD
static void mtk_set_vbus(struct musb *musb, int is_on)
{
    DBG(0,"mt65xx_usb20_vbus++,is_on=%d\r\n",is_on);
#ifndef CONFIG_MT6589_FPGA
    if(is_on){
        //power on VBUS, implement later...
    #ifdef MTK_FAN5405_SUPPORT
        fan5405_set_opa_mode(1);
        fan5405_set_otg_pl(1);
        fan5405_set_otg_en(1);
    #elif defined(MTK_BQ24158_SUPPORT)
        bq24158_set_opa_mode(1);
	bq24158_set_otg_pl(1);
	bq24158_set_otg_en(1);
//<2013/1/21-20645-jessicatseng, [Pelican] Intrgrate charging IC BQ24157 for PRE-MP SW
    #elif defined(MTK_BQ24157_SUPPORT)
        bq24157_set_opa_mode(1);
        bq24157_set_otg_pl(1);
        bq24157_set_otg_en(1);
//>2013/1/21-20645-jessicatseng
    #elif defined(MTK_NCP1851_SUPPORT) || defined(MTK_BQ24196_SUPPORT)
        tbl_charger_otg_vbus((work_busy(&musb->id_pin_work.work)<< 8)| 1);
    #else
        mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN,GPIO_OUT_ONE);
        #endif
    } else {
        //power off VBUS, implement later...
    #ifdef MTK_FAN5405_SUPPORT
        fan5405_config_interface_liao(0x01,0x30);
	fan5405_config_interface_liao(0x02,0x8e);
    #elif defined(MTK_BQ24158_SUPPORT)
        bq24158_config_interface_reg(0x01,0x30);
	bq24158_config_interface_reg(0x02,0x8e);
//<2013/1/21-20645-jessicatseng, [Pelican] Intrgrate charging IC BQ24157 for PRE-MP SW
    #elif defined(MTK_BQ24157_SUPPORT)
        bq24157_config_interface_liao(0x01,0x30);
	bq24157_config_interface_liao(0x02,0x8e);
//>2013/1/21-20645-jessicatseng
    #elif defined(MTK_NCP1851_SUPPORT) || defined(MTK_BQ24196_SUPPORT)
        tbl_charger_otg_vbus((work_busy(&musb->id_pin_work.work)<< 8)| 0);
    #else
        mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN,GPIO_OUT_ZERO);
    #endif
    }
#endif
    return;
}
#endif

#ifdef CONFIG_USB_MTK_OTG
/*void musb_phy_reset(void)
{
	u32 phy_dtm = 0;
	phy_dtm = musb_readl(mtk_musb->mregs,U2PHYDTM1);
	phy_dtm |= ID_PHY_RESET;
	musb_writel(mtk_musb->mregs,U2PHYDTM1,phy_dtm);
	mdelay(2);
	phy_dtm &= ~0xFEFE;
	musb_writel(mtk_musb->mregs,U2PHYDTM1,phy_dtm);
	printk("state register is %x\n",musb_readb(mtk_musb->mregs,0x71));
}
*/

//extern struct switch_dev otg_state;

bool musb_is_host(void)
{
	u8 devctl = 0;
  DBG(0,"will mask PMIC charger detection\n");
#ifndef CONFIG_MT6589_FPGA
  upmu_interrupt_chrdet_int_en(0);
#endif
	musb_platform_enable(mtk_musb);
	//musb_set_vbus(mtk_musb,FALSE);
	//mt65xx_eint_mask(EINT_CHR_DET_NUM);
	devctl = musb_readb(mtk_musb->mregs,MUSB_DEVCTL);
	DBG(0, "devctl = %x before end session\n", devctl);
	devctl &= ~MUSB_DEVCTL_SESSION;	// this will cause A-device change back to B-device after A-cable plug out
	musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, devctl);
	msleep(delay_time);

	devctl = musb_readb(mtk_musb->mregs,MUSB_DEVCTL);
	DBG(0,"devctl = %x before set session\n",devctl);

	devctl |= MUSB_DEVCTL_SESSION;
	//musb_set_vbus(mtk_musb,TRUE);
	musb_writeb(mtk_musb->mregs,MUSB_DEVCTL,devctl);
	msleep(delay_time1);
	devctl = musb_readb(mtk_musb->mregs,MUSB_DEVCTL);
	DBG(0,"devclt = %x\n",devctl);

	if (devctl & MUSB_DEVCTL_BDEVICE) {
		usb_is_host = FALSE;
        DBG(0,"will unmask PMIC charger detection\n");
#ifndef CONFIG_MT6589_FPGA
        upmu_interrupt_chrdet_int_en(1);
#endif
		return FALSE;
	} else {
		usb_is_host = TRUE;
		return TRUE;
	}
}

/*
bool musb_is_host(void)
{
	if(musb_readl(mtk_musb->mregs,USB_L1INTP)&IDDIG_INT_STATUS)
		return true;
	else
		return false;
}
*/

void switch_int_to_device(void)
{
	 musb_writel(mtk_musb->mregs,USB_L1INTP,0);
	 musb_writel(mtk_musb->mregs,USB_L1INTM,IDDIG_INT_STATUS|musb_readl(mtk_musb->mregs,USB_L1INTM));

	 DBG(0,"switch_int_to_device is done\n");
}

void switch_int_to_host(void)
{
/*	mt65xx_eint_set_polarity(ID_PIN_EINT, !DEVICE_INTTERRUPT);
	DBG(2,"switch_int_to_host is done\n");
	mt65xx_eint_unmask(ID_PIN_EINT);
	mt65xx_reg_sync_writel(1 << EINT_CHR_DET_NUM, EINT_INTACK);
	mt65xx_eint_unmask(EINT_CHR_DET_NUM);*/
	musb_writel(mtk_musb->mregs,USB_L1INTP,IDDIG_INT_STATUS);
	musb_writel(mtk_musb->mregs,USB_L1INTM,IDDIG_INT_STATUS|musb_readl(mtk_musb->mregs,USB_L1INTM));
	DBG(0,"switch_int_to_host is done\n");

}

void switch_int_to_host_and_mask(void)
{
	musb_writel(mtk_musb->mregs,USB_L1INTM,(~IDDIG_INT_STATUS)&musb_readl(mtk_musb->mregs,USB_L1INTM)); //mask before change polarity
	mb();
	musb_writel(mtk_musb->mregs,USB_L1INTP,IDDIG_INT_STATUS);
	DBG(0,"swtich_int_to_host_and_mask is done\n");
}

void otg_int_init(void)
{
	//bool is_ready = mtk_musb->is_ready;
#ifdef ID_PIN_USE_EX_EINT
	mt_set_gpio_mode(ID_PIN_GPIO, GPIO_ID_PIN_EINT_PIN_M_EINT);
	mt_set_gpio_dir(ID_PIN_GPIO, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(ID_PIN_GPIO, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(ID_PIN_GPIO, GPIO_PULL_UP);
#else
	u32 phy_id_pull = 0;
	phy_id_pull = __raw_readl(U2PHYDTM1);
	phy_id_pull |= ID_PULL_UP;
	__raw_writel(phy_id_pull,U2PHYDTM1);
#endif
	//mt65xx_eint_set_sens(ID_PIN_EINT, MT65xx_LEVEL_SENSITIVE);
	//mt65xx_eint_set_hw_debounce(ID_PIN_EINT,64);
	//mtk_musb->is_ready = FALSE;
	//mt65xx_eint_registration(ID_PIN_EINT, FALSE, !DEVICE_INTTERRUPT, musb_id_pin_interrup,FALSE);
	//mtk_musb->is_ready = is_ready;

	musb_writel(mtk_musb->mregs,USB_L1INTM,IDDIG_INT_STATUS|musb_readl(mtk_musb->mregs,USB_L1INTM));
}
#endif

/* ================================ */
/* connect and disconnect functions */
/* ================================ */
bool mt_usb_is_device(void)
{
	DBG(4,"called\n");

	if(!mtk_musb){
		DBG(0,"mtk_musb is NULL\n");
		return false; // don't do charger detection when usb is not ready
	} else {
		DBG(4,"is_host=%d\n",mtk_musb->is_host);
	}
  return !mtk_musb->is_host;
}

bool mt_usb_is_ready(void)
{
	printk("[MUSB] USB is ready or not\n");
	if(!mtk_musb || !mtk_musb->is_ready)
        return false;
    else
        return true;
}

void mt_usb_connect(void)
{
	printk("[MUSB] USB is ready for connect\n");
	DBG(3, "is ready %d is_host %d power %d\n",mtk_musb->is_ready,mtk_musb->is_host , mtk_musb->power);
	if(!mtk_musb || !mtk_musb->is_ready || mtk_musb->is_host || mtk_musb->power)
		return;
	if(cable_mode != CABLE_MODE_NORMAL)
	{
		musb_sync_with_bat(mtk_musb,USB_CONFIGURED);
		mtk_musb->power = true;
		return;
	}
	musb_start(mtk_musb);
	printk("[MUSB] USB connect\n");
}

void mt_usb_disconnect(void)
{
	printk("[MUSB] USB is ready for disconnect\n");

	if (!mtk_musb || !mtk_musb->is_ready || mtk_musb->is_host || !mtk_musb->power)
		return;

	musb_stop(mtk_musb);

	if (wake_lock_active(&mtk_musb->usb_lock))
		wake_unlock(&mtk_musb->usb_lock);

	if (cable_mode != CABLE_MODE_NORMAL) {
		musb_sync_with_bat(mtk_musb,USB_SUSPEND);
		mtk_musb->power = false;
		return;
	}
	printk("[MUSB] USB disconnect\n");
}

bool is_usb_connected(void)
{
#ifdef CONFIG_POWER_EXT
	if (upmu_get_rgs_chrdet()
#else
	if (upmu_is_chr_det()
#endif
	   && (mt_charger_type_detection() == STANDARD_HOST)) {
		return true;
	} else {
		return false;
	}
}

void musb_platform_reset(struct musb *musb)
{
	u16 swrst = 0;
	void __iomem	*mbase = musb->mregs;
	swrst = musb_readw(mbase,MUSB_SWRST);
	swrst |= (MUSB_SWRST_DISUSBRESET | MUSB_SWRST_SWRST);
	musb_writew(mbase, MUSB_SWRST,swrst);
}

void usb_check_connect(void)
{
#ifndef CONFIG_MT6589_FPGA
	if (is_usb_connected())
		mt_usb_connect();
#endif
}

void musb_sync_with_bat(struct musb *musb, int usb_state)
{
#ifndef CONFIG_MT6589_FPGA
	BATTERY_SetUSBState(usb_state);
	wake_up_bat();
#endif
}

void musb_platform_enable(struct musb *musb)
{
	unsigned long flags;

	printk("%s, %d, %d\n", __func__, mtk_usb_power, musb->power);

	if (musb->power == true)
		return;

	flags = musb_readl(mtk_musb->mregs, USB_L1INTM);

	// mask ID pin, so "open clock" and "set flag" won't be interrupted. ISR may call clock_disable.
	musb_writel(mtk_musb->mregs, USB_L1INTM, (~IDDIG_INT_STATUS) & flags);

  if(platform_init_first){
  	DBG(0,"usb init first\n\r");
  	musb->is_host = true;
  }

	if (!mtk_usb_power) {
		if (down_interruptible(&power_clock_lock))
			xlog_printk(ANDROID_LOG_ERROR, "USB20", "%s: busy, Couldn't get power_clock_lock\n" \
				, __func__);
#ifndef CONFIG_MT6589_FPGA
		enable_pll(UNIVPLL, "USB_PLL");

		printk("%s, enable UPLL before connect\n", __func__);
#endif

		mdelay(10);

		usb_phy_recover();

		mtk_usb_power = true;

		up(&power_clock_lock);
	}
	musb->power = true;

	musb_writel(mtk_musb->mregs,USB_L1INTM,flags);
}

void musb_platform_disable(struct musb *musb)
{
	printk("%s, %d, %d\n", __func__, mtk_usb_power, musb->power);

	if(musb->power == false)
        return;

	if(platform_init_first){
  	DBG(0,"usb init first\n\r");
  	musb->is_host = false;
  	platform_init_first = false;
  }

	if (mtk_usb_power) {
		if (down_interruptible(&power_clock_lock))
			xlog_printk(ANDROID_LOG_ERROR, "USB20", "%s: busy, Couldn't get power_clock_lock\n" \
				, __func__);

	//Modification for ALPS00408742
	mtk_usb_power = false;
	smp_mb();
	//printk("%s, line %d: %d, %d, before savecurrent\n", __func__, __LINE__, mtk_usb_power, musb->power);
	//Modification for ALPS00408742
	usb_phy_savecurrent();

#ifndef CONFIG_MT6589_FPGA

	disable_pll(UNIVPLL,"USB_PLL");

	//printk("%s, disable VUSB and UPLL before disconnect\n", __func__);
#endif
	mtk_usb_power = false;

	//Modification for ALPS00408742
	smp_mb();
	//Modification for ALPS00408742
	up(&power_clock_lock);
	}

	musb->power = false;
}

void musb_generic_disable(struct musb *musb)
{
	void __iomem	*mbase = musb->mregs;

	/* disable interrupts */
	musb_writeb(mbase, MUSB_INTRUSBE, 0);
	musb_writew(mbase, MUSB_INTRTXE, 0);
	musb_writew(mbase, MUSB_INTRRXE, 0);

	/* off */
	//musb_writeb(mbase, MUSB_DEVCTL, 0);

	/*  flush pending interrupts */
	musb_writew(musb->mregs,MUSB_INTRRX,0xFFFF);
	musb_writew(musb->mregs,MUSB_INTRTX,0xFFFF);
	musb_writeb(musb->mregs,MUSB_INTRUSB,0xEF);
}

void musb_read_clear_generic_interrupt(struct musb *musb)
{
	musb->int_usb = musb_readb(musb->mregs, MUSB_INTRUSB) & musb_readb(musb->mregs, MUSB_INTRUSBE);
	musb->int_tx = musb_readw(musb->mregs, MUSB_INTRTX) & musb_readw(musb->mregs, MUSB_INTRTXE);
	musb->int_rx = musb_readw(musb->mregs, MUSB_INTRRX) & musb_readw(musb->mregs, MUSB_INTRRXE);
	mb();
	musb_writew(musb->mregs,MUSB_INTRRX,musb->int_rx);
	musb_writew(musb->mregs,MUSB_INTRTX,musb->int_tx);
	musb_writeb(musb->mregs,MUSB_INTRUSB,musb->int_usb);
}

u8 musb_read_clear_dma_interrupt(struct musb *musb)
{
	u8 int_hsdma = 0;
	int_hsdma = musb_readb(musb->mregs, MUSB_HSDMA_INTR);

	mb();
	musb_writeb(musb->mregs,MUSB_HSDMA_INTR,int_hsdma);

	return int_hsdma;
}
extern u32 sw_deboun_time ;

irqreturn_t mt_usb_interrupt(int irq, void *dev_id)
{
  irqreturn_t tmp_status;
  irqreturn_t status = IRQ_NONE;
	struct musb	*musb = (struct musb*)dev_id;
	u32 usb_l1_ints;
#ifdef CONFIG_USB_MTK_OTG
  u32 usb_l1_ploy;
#endif
	usb_l1_ints= musb_readl(musb->mregs, USB_L1INTS) & musb_readl(musb->mregs,USB_L1INTM);
	DBG(3,"usb interrupt assert %x\n",usb_l1_ints);

	if ((usb_l1_ints & TX_INT_STATUS) || (usb_l1_ints & RX_INT_STATUS) || (usb_l1_ints & USBCOM_INT_STATUS)) {
		if((tmp_status = generic_interrupt(irq, musb)) != IRQ_NONE)
			status = tmp_status;
	}

	if (usb_l1_ints & DMA_INT_STATUS) {
		if((tmp_status = dma_controller_irq(irq, musb->dma_controller)) != IRQ_NONE)
			status = tmp_status;
	}

#ifdef 	CONFIG_USB_MTK_OTG
	if(usb_l1_ints&IDDIG_INT_STATUS) {
	    usb_l1_ploy = musb_readl(mtk_musb->mregs,USB_L1INTP);
	    DBG(0,"MUSB:id pin interrupt assert,polarity=0x%x\n",usb_l1_ploy);
		if(usb_l1_ploy & IDDIG_INT_STATUS)
            usb_l1_ploy &= (~IDDIG_INT_STATUS);
		else
            usb_l1_ploy |= IDDIG_INT_STATUS;

        musb_writel(mtk_musb->mregs,USB_L1INTP,usb_l1_ploy);
		musb_writel(mtk_musb->mregs,USB_L1INTM,(~IDDIG_INT_STATUS)&musb_readl(mtk_musb->mregs,USB_L1INTM));

        schedule_delayed_work(&mtk_musb->id_pin_work,sw_deboun_time*HZ/1000);
		status = IRQ_HANDLED;
		DBG(1,"MUSB:id pin interrupt assert\n");
	}
#endif

    return status;

}

/*--FOR INSTANT POWER ON USAGE--------------------------------------------------*/
static ssize_t mt_usb_show_cmode(struct device* dev, struct device_attribute *attr, char *buf)
{
	if (!dev) {
		DBG(0,"dev is null!!\n");
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%d\n", cable_mode);
}

static ssize_t mt_usb_store_cmode(struct device* dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned int cmode;

	if (!dev) {
		DBG(0,"dev is null!!\n");
		return count;
	} else if (1 == sscanf(buf, "%d", &cmode)) {
		DBG(0, "cmode=%d, cable_mode=%d\n", cmode, cable_mode);
		if (cmode >= CABLE_MODE_MAX)
			cmode = CABLE_MODE_NORMAL;

		if (cable_mode != cmode) {
			if(mtk_musb) {
				if(down_interruptible(&mtk_musb->musb_lock))
					xlog_printk(ANDROID_LOG_ERROR, "USB20", "%s: busy, Couldn't get musb_lock\n", __func__);
			}
			if(cmode == CABLE_MODE_CHRG_ONLY) { // IPO shutdown, disable USB
				if(mtk_musb) {
					mtk_musb->in_ipo_off = true;
				}

			} else { // IPO bootup, enable USB
				if(mtk_musb) {
					mtk_musb->in_ipo_off = false;
				}
			}

			mt_usb_disconnect();
			cable_mode = cmode;
			msleep(10);
			//ALPS00114502
			//check that "if USB cable connected and than call mt_usb_connect"
			//Then, the Bat_Thread won't be always wakeup while no USB/chatger cable and IPO mode
			//mt_usb_connect();
			usb_check_connect();
			//ALPS00114502

#ifdef CONFIG_USB_MTK_OTG
			if(cmode == CABLE_MODE_CHRG_ONLY) {
				if(mtk_musb && mtk_musb->is_host) { // shut down USB host for IPO
					musb_stop(mtk_musb);
					/* Think about IPO shutdown with A-cable, then switch to B-cable and IPO bootup. We need a point to clear session bit */
					musb_writeb(mtk_musb->mregs, MUSB_DEVCTL, (~MUSB_DEVCTL_SESSION)&musb_readb(mtk_musb->mregs,MUSB_DEVCTL));
				} else {
					switch_int_to_host_and_mask(); // mask ID pin interrupt even if A-cable is not plugged in
				}
			} else {
				switch_int_to_host(); // resotre ID pin interrupt
			}
#endif
			if(mtk_musb) {
				up(&mtk_musb->musb_lock);
			}
		}
	}
	return count;
}

DEVICE_ATTR(cmode,  0664, mt_usb_show_cmode, mt_usb_store_cmode);

#ifdef CONFIG_MT6589_FPGA
static struct i2c_client *usb_i2c_client = NULL;
static const struct i2c_device_id usb_i2c_id[] = {{"mtk-usb",0},{}};

static struct i2c_board_info __initdata usb_i2c_dev = { I2C_BOARD_INFO("mtk-usb", 0x60)};


void USB_PHY_Write_Register8(UINT8 var,  UINT8 addr)
{
	char buffer[2];
	buffer[0] = addr;
	buffer[1] = var;
	i2c_master_send(usb_i2c_client, &buffer, 2);
}

UINT8 USB_PHY_Read_Register8(UINT8 addr)
{
	UINT8 var;
	i2c_master_send(usb_i2c_client, &addr, 1);
	i2c_master_recv(usb_i2c_client, &var, 1);
	return var;
}

static int usb_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

	printk("[MUSB]usb_i2c_probe, start\n");

	usb_i2c_client = client;

	//disable usb mac suspend
	DRV_WriteReg8(USB_SIF_BASE + 0x86a, 0x00);

	//usb phy initial sequence
	USB_PHY_Write_Register8(0x00, 0xFF);
	USB_PHY_Write_Register8(0x04, 0x61);
	USB_PHY_Write_Register8(0x00, 0x68);
	USB_PHY_Write_Register8(0x00, 0x6a);
	USB_PHY_Write_Register8(0x6e, 0x00);
	USB_PHY_Write_Register8(0x0c, 0x1b);
	USB_PHY_Write_Register8(0x44, 0x08);
	USB_PHY_Write_Register8(0x55, 0x11);
	USB_PHY_Write_Register8(0x68, 0x1a);


	printk("[MUSB]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
	printk("[MUSB]addr: 0x61, value: %x\n", USB_PHY_Read_Register8(0x61));
	printk("[MUSB]addr: 0x68, value: %x\n", USB_PHY_Read_Register8(0x68));
	printk("[MUSB]addr: 0x6a, value: %x\n", USB_PHY_Read_Register8(0x6a));
	printk("[MUSB]addr: 0x00, value: %x\n", USB_PHY_Read_Register8(0x00));
	printk("[MUSB]addr: 0x1b, value: %x\n", USB_PHY_Read_Register8(0x1b));
	printk("[MUSB]addr: 0x08, value: %x\n", USB_PHY_Read_Register8(0x08));
	printk("[MUSB]addr: 0x11, value: %x\n", USB_PHY_Read_Register8(0x11));
	printk("[MUSB]addr: 0x1a, value: %x\n", USB_PHY_Read_Register8(0x1a));


	printk("[MUSB]usb_i2c_probe, end\n");
    return 0;

}

static int usb_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {
    strcpy(info->type, "mtk-usb");
    return 0;
}

static int usb_i2c_remove(struct i2c_client *client) {return 0;}


struct i2c_driver usb_i2c_driver = {
    .probe = usb_i2c_probe,
    .remove = usb_i2c_remove,
    .detect = usb_i2c_detect,
    .driver = {
    	.name = "mtk-usb",
    },
    .id_table = usb_i2c_id,
};

int add_usb_i2c_driver()
{
	i2c_register_board_info(0, &usb_i2c_dev, 1);
	if(i2c_add_driver(&usb_i2c_driver)!=0)
	{
		printk("[MUSB]usb_i2c_driver initialization failed!!\n");
		return -1;
	}
	else
	{
		printk("[MUSB]usb_i2c_driver initialization succeed!!\n");
	}
	return 0;
}
#endif //End of CONFIG_MT6589_FPGA

void musb_check_mpu_violation(u32 addr, int wr_vio)
{
    void __iomem *mregs = (void *)USB_BASE;

    printk(KERN_CRIT "MUSB checks EMI MPU violation.\n");
    printk(KERN_CRIT "addr = 0x%x, %s violation.\n", addr, wr_vio? "Write": "Read");
    printk(KERN_CRIT "POWER = 0x%x,DEVCTL= 0x%x.\n", musb_readb(mregs, MUSB_POWER),musb_readb(mregs,MUSB_DEVCTL));
    printk(KERN_CRIT "DMA_CNTLch0 %04x,DMA_ADDRch0 %08x,DMA_COUNTch0 %08x \n",musb_readw(mregs, 0x204),musb_readl(mregs,0x208),musb_readl(mregs,0x20C));
    printk(KERN_CRIT "DMA_CNTLch1 %04x,DMA_ADDRch1 %08x,DMA_COUNTch1 %08x \n",musb_readw(mregs, 0x214),musb_readl(mregs,0x218),musb_readl(mregs,0x21C));
    printk(KERN_CRIT "DMA_CNTLch2 %04x,DMA_ADDRch2 %08x,DMA_COUNTch2 %08x \n",musb_readw(mregs, 0x224),musb_readl(mregs,0x228),musb_readl(mregs,0x22C));
    printk(KERN_CRIT "DMA_CNTLch3 %04x,DMA_ADDRch3 %08x,DMA_COUNTch3 %08x \n",musb_readw(mregs, 0x234),musb_readl(mregs,0x238),musb_readl(mregs,0x23C));
    printk(KERN_CRIT "DMA_CNTLch4 %04x,DMA_ADDRch4 %08x,DMA_COUNTch4 %08x \n",musb_readw(mregs, 0x244),musb_readl(mregs,0x248),musb_readl(mregs,0x24C));
    printk(KERN_CRIT "DMA_CNTLch5 %04x,DMA_ADDRch5 %08x,DMA_COUNTch5 %08x \n",musb_readw(mregs, 0x254),musb_readl(mregs,0x258),musb_readl(mregs,0x25C));
    printk(KERN_CRIT "DMA_CNTLch6 %04x,DMA_ADDRch6 %08x,DMA_COUNTch6 %08x \n",musb_readw(mregs, 0x264),musb_readl(mregs,0x268),musb_readl(mregs,0x26C));
    printk(KERN_CRIT "DMA_CNTLch7 %04x,DMA_ADDRch7 %08x,DMA_COUNTch7 %08x \n",musb_readw(mregs, 0x274),musb_readl(mregs,0x278),musb_readl(mregs,0x27C));
}


int __init musb_platform_init(struct musb *musb)
{
	int err;
#ifdef CONFIG_USB_MTK_HDRC_HCD
	if (is_host_enabled(musb)) {
		musb->board_set_vbus = mtk_set_vbus;
		//<2013/03/27-23263-stevenchen, Correct GPIO12 & GPIO38 configuration to low down standby current.
		#ifndef CONFIG_MT6589_FPGA
		#ifndef MTK_BQ24196_SUPPORT
		#ifndef MTK_BQ24157_SUPPORT
		mt_set_gpio_mode(GPIO_OTG_DRVVBUS_PIN,GPIO_OTG_DRVVBUS_PIN_M_GPIO);//should set GPIO2 as gpio mode.
		mt_set_gpio_dir(GPIO_OTG_DRVVBUS_PIN,GPIO_DIR_OUT);
		mt_get_gpio_pull_enable(GPIO_OTG_DRVVBUS_PIN);
		mt_set_gpio_pull_select(GPIO_OTG_DRVVBUS_PIN,GPIO_PULL_UP);
		#endif
		#endif
		#endif
		//>2013/03/27-23263-stevenchen
	}
#endif


	musb->nIrq = MT6589_USB0_IRQ_ID;
	musb->dma_irq= (int)SHARE_IRQ;
	musb->fifo_cfg = fifo_cfg;
	musb->fifo_cfg_size = ARRAY_SIZE(fifo_cfg);
	musb->fifo_cfg_host = fifo_cfg_host;
	musb->fifo_cfg_host_size = ARRAY_SIZE(fifo_cfg_host);
	mtk_musb->power = FALSE;
	mtk_musb->is_host = FALSE;
	mtk_musb->fifo_size = 8*1024;

	#ifndef CONFIG_MT6589_FPGA
	hwPowerOn(MT65XX_POWER_LDO_VUSB, VOL_3300, "VUSB_LDO");
	printk("%s, enable VBUS_LDO \n", __func__);
	#endif

	musb_platform_enable(musb);
        emi_mpu_notifier_register(MST_ID_MMPERI_1, musb_check_mpu_violation);

	musb->isr = mt_usb_interrupt;
	musb_writel(musb->mregs,MUSB_HSDMA_INTR,0xff | (0xff << DMA_INTR_UNMASK_SET_OFFSET));
	DBG(2,"musb platform init %x\n",musb_readl(musb->mregs,MUSB_HSDMA_INTR));
	musb_writel(musb->mregs,USB_L1INTM,TX_INT_STATUS | RX_INT_STATUS | USBCOM_INT_STATUS | DMA_INT_STATUS);
	err = device_create_file(musb->controller,&dev_attr_cmode);
	return 0;
}

int musb_platform_exit(struct musb *musb)
{
	return 0;
}
