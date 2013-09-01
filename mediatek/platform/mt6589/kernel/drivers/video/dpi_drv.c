

#ifdef BUILD_UBOOT
#define ENABLE_DPI_INTERRUPT        0
#define ENABLE_DPI_REFRESH_RATE_LOG 0

#include <asm/arch/disp_drv_platform.h>
#else

#define ENABLE_DPI_INTERRUPT        1
#define ENABLE_DPI_REFRESH_RATE_LOG 0

#if ENABLE_DPI_REFRESH_RATE_LOG && !ENABLE_DPI_INTERRUPT
#error "ENABLE_DPI_REFRESH_RATE_LOG should be also ENABLE_DPI_INTERRUPT"
#endif

#if defined(MTK_HDMI_SUPPORT) && !ENABLE_DPI_INTERRUPT
//#error "enable MTK_HDMI_SUPPORT should be also ENABLE_DPI_INTERRUPT"
#endif

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/hrtimer.h>
#include <asm/io.h>
#include <disp_drv_log.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include "disp_drv_platform.h"

#include "dpi_reg.h"
#include "dsi_reg.h"
#include "dpi_drv.h"
#include "lcd_drv.h"
#include <mach/mt_clkmgr.h>
#include "debug.h"

#if ENABLE_DPI_INTERRUPT
//#include <linux/interrupt.h>
//#include <linux/wait.h>

#include <mach/irqs.h>
#include "mtkfb.h"
#endif
static wait_queue_head_t _vsync_wait_queue_dpi;
static bool dpi_vsync = false;
static bool wait_dpi_vsync = false;
static struct hrtimer hrtimer_vsync_dpi;
#include <linux/module.h>
#endif

#include <mach/sync_write.h>
#ifdef OUTREG32
  #undef OUTREG32
  #define OUTREG32(x, y) mt65xx_reg_sync_writel(y, x)
#endif

#ifndef OUTREGBIT
#define OUTREGBIT(TYPE,REG,bit,value)  \
                    do {    \
                        TYPE r = *((TYPE*)&INREG32(&REG));    \
                        r.bit = value;    \
                        OUTREG32(&REG, AS_UINT32(&r));    \
                    } while (0)
#endif

static PDPI_REGS const DPI_REG = (PDPI_REGS)(DPI_BASE);
static PDSI_PHY_REGS const DSI_PHY_REG_DPI = (PDSI_PHY_REGS)(MIPI_CONFIG_BASE + 0x800);
static UINT32 const PLL_SOURCE = APMIXEDSYS_BASE + 0x44;
static BOOL s_isDpiPowerOn = FALSE;
static DPI_REGS regBackup;
static void (*dpiIntCallback)(DISP_INTERRUPT_EVENTS);

#define DPI_REG_OFFSET(r)       offsetof(DPI_REGS, r)
#define REG_ADDR(base, offset)  (((BYTE *)(base)) + (offset))

#if !(defined(CONFIG_MT6589_FPGA) || defined(BUILD_UBOOT))
//#define DPI_MIPI_API
#endif

const UINT32 BACKUP_DPI_REG_OFFSETS[] =
{
    DPI_REG_OFFSET(INT_ENABLE),
    DPI_REG_OFFSET(SIZE),
    DPI_REG_OFFSET(CLK_CNTL),

    DPI_REG_OFFSET(TGEN_HWIDTH),
    DPI_REG_OFFSET(TGEN_HPORCH),

	DPI_REG_OFFSET(TGEN_VWIDTH_LODD),
    DPI_REG_OFFSET(TGEN_VPORCH_LODD),

    DPI_REG_OFFSET(TGEN_VWIDTH_LEVEN),
    DPI_REG_OFFSET(TGEN_VPORCH_LEVEN),
    DPI_REG_OFFSET(TGEN_VWIDTH_RODD),

    DPI_REG_OFFSET(TGEN_VPORCH_RODD),
    DPI_REG_OFFSET(TGEN_VWIDTH_REVEN),

	DPI_REG_OFFSET(TGEN_VPORCH_REVEN),
    DPI_REG_OFFSET(ESAV_VTIML),
    DPI_REG_OFFSET(ESAV_VTIMR),
	DPI_REG_OFFSET(ESAV_FTIM),
	DPI_REG_OFFSET(BG_HCNTL),

  	DPI_REG_OFFSET(BG_VCNTL),
    DPI_REG_OFFSET(BG_COLOR),
	DPI_REG_OFFSET(TGEN_POL),
	DPI_REG_OFFSET(EMBSYNC_SETTING),

    DPI_REG_OFFSET(CNTL),
};

static void _BackupDPIRegisters(void)
{
    DPI_REGS *reg = &regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(DPI_REG, BACKUP_DPI_REG_OFFSETS[i])));
    }
}

static void _RestoreDPIRegisters(void)
{
    DPI_REGS *reg = &regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(DPI_REG, BACKUP_DPI_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i])));
    }
}

static void _ResetBackupedDPIRegisterValues(void)
{
    DPI_REGS *regs = &regBackup;
    memset((void*)regs, 0, sizeof(DPI_REGS));

    OUTREG32(&regs->CLK_CNTL, 0x00000101);
}


#if ENABLE_DPI_REFRESH_RATE_LOG
static void _DPI_LogRefreshRate(DPI_REG_INTERRUPT status)
{
    static unsigned long prevUs = 0xFFFFFFFF;

    if (status.VSYNC)
    {
        struct timeval curr;
        do_gettimeofday(&curr);

        if (prevUs < curr.tv_usec)
        {
            DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI", "Receive 1 vsync in %lu us\n",
                   curr.tv_usec - prevUs);
        }
        prevUs = curr.tv_usec;
    }
}
#else
#define _DPI_LogRefreshRate(x)  do {} while(0)
#endif

extern void dsi_handle_esd_recovery(void);

void DPI_DisableIrq(void)
{
#if ENABLE_DPI_INTERRUPT
		DPI_REG_INTERRUPT enInt = DPI_REG->INT_ENABLE;
		//enInt.FIFO_EMPTY = 0;
		//enInt.FIFO_FULL = 0;
		//enInt.OUT_EMPTY = 0;
		//enInt.CNT_OVERFLOW = 0;
		//enInt.LINE_ERR = 0;
		enInt.VSYNC = 0;
		OUTREG32(&DPI_REG->INT_ENABLE, AS_UINT32(&enInt));
#endif
}
void DPI_EnableIrq(void)
{
#if ENABLE_DPI_INTERRUPT
		DPI_REG_INTERRUPT enInt = DPI_REG->INT_ENABLE;
		//enInt.FIFO_EMPTY = 1;
		//enInt.FIFO_FULL = 0;
		//enInt.OUT_EMPTY = 0;
		//enInt.CNT_OVERFLOW = 0;
		//enInt.LINE_ERR = 0;
		enInt.VSYNC = 1;
		OUTREG32(&DPI_REG->INT_ENABLE, AS_UINT32(&enInt));
#endif
}

static int dpi_vsync_irq_count = 0;
#if ENABLE_DPI_INTERRUPT
static irqreturn_t _DPI_InterruptHandler(int irq, void *dev_id)
{
    static int counter = 0;
    DPI_REG_INTERRUPT status = DPI_REG->INT_STATUS;
//    if (status.FIFO_EMPTY) ++ counter;

    OUTREG32(&DPI_REG->INT_STATUS, 0);
    if(status.VSYNC)
    {
	dpi_vsync_irq_count++;
	if(dpi_vsync_irq_count > 120)
	{
		printk("dpi vsync\n");
		dpi_vsync_irq_count = 0;
	}
        if(dpiIntCallback)
           dpiIntCallback(DISP_DPI_VSYNC_INT);
#ifndef BUILD_UBOOT
		if(wait_dpi_vsync){
			if(-1 != hrtimer_try_to_cancel(&hrtimer_vsync_dpi)){
				dpi_vsync = true;
//			hrtimer_try_to_cancel(&hrtimer_vsync_dpi);
				wake_up_interruptible(&_vsync_wait_queue_dpi);
			}
		}
#endif
    }

    if (status.VSYNC && counter) {
        DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "[Error] DPI FIFO is empty, "
               "received %d times interrupt !!!\n", counter);
        counter = 0;
    }

    _DPI_LogRefreshRate(status);
    return IRQ_HANDLED;
}
#endif

#define VSYNC_US_TO_NS(x) (x * 1000)
unsigned int vsync_timer_dpi = 0;
void DPI_WaitVSYNC(void)
{
#ifndef BUILD_UBOOT
	wait_dpi_vsync = true;
	hrtimer_start(&hrtimer_vsync_dpi, ktime_set(0, VSYNC_US_TO_NS(vsync_timer_dpi)), HRTIMER_MODE_REL);
	wait_event_interruptible(_vsync_wait_queue_dpi, dpi_vsync);
	dpi_vsync = false;
	wait_dpi_vsync = false;
#endif
}

void DPI_PauseVSYNC(bool enable)
{
}

#ifndef BUILD_UBOOT
enum hrtimer_restart dpi_vsync_hrtimer_func(struct hrtimer *timer)
{
//	long long ret;
	if(wait_dpi_vsync)
	{
		dpi_vsync = true;
		wake_up_interruptible(&_vsync_wait_queue_dpi);
//		printk("hrtimer Vsync, and wake up\n");
	}
//	ret = hrtimer_forward_now(timer, ktime_set(0, VSYNC_US_TO_NS(vsync_timer_dpi)));
//	printk("hrtimer callback\n");
    return HRTIMER_NORESTART;
}
#endif
void DPI_InitVSYNC(unsigned int vsync_interval)
{
#ifndef BUILD_UBOOT
    ktime_t ktime;
	vsync_timer_dpi = vsync_interval;
	ktime = ktime_set(0, VSYNC_US_TO_NS(vsync_timer_dpi));
	hrtimer_init(&hrtimer_vsync_dpi, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_vsync_dpi.function = dpi_vsync_hrtimer_func;
//	hrtimer_start(&hrtimer_vsync_dpi, ktime, HRTIMER_MODE_REL);
#endif
}

DPI_STATUS DPI_Init(BOOL isDpiPoweredOn)
{
    //DPI_REG_CNTL cntl;
    //DPI_REG_EMBSYNC_SETTING embsync;

    if (isDpiPoweredOn) {
        _BackupDPIRegisters();
    } else {
        _ResetBackupedDPIRegisterValues();
    }

    DPI_PowerOn();

	OUTREG32(DPI_BASE+ 0x64, 0x400);//
	OUTREG32(DPI_BASE+ 0x6C, 0x400);//
	OUTREG32(DPI_BASE+ 0x74, 0x400);//
	OUTREG32(DPI_BASE+ 0x8C, 0x0FFF0000);//
	OUTREG32(DPI_BASE+ 0x90, 0x0FFF0000);//
	MASKREG32(DISPSYS_BASE + 0x60, 0x1, 0x1);
#if ENABLE_DPI_INTERRUPT
    if (request_irq(MT6589_DISP_DPI0_IRQ_ID,
        _DPI_InterruptHandler, IRQF_TRIGGER_LOW, "mtkdpi", NULL) < 0)
    {
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI", "[ERROR] fail to request DPI irq\n");
        return DPI_STATUS_ERROR;
    }

    {
        DPI_REG_INTERRUPT enInt = DPI_REG->INT_ENABLE;
        enInt.VSYNC = 1;
        OUTREG32(&DPI_REG->INT_ENABLE, AS_UINT32(&enInt));
    }
#endif
	LCD_W2M_NeedLimiteSpeed(TRUE);
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_Init);

DPI_STATUS DPI_FreeIRQ(void)
{
#if ENABLE_DPI_INTERRUPT
    free_irq(MT6589_DISP_DPI0_IRQ_ID, NULL);
#endif
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FreeIRQ);

DPI_STATUS DPI_Deinit(void)
{
    DPI_DisableClk();
    DPI_PowerOff();

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_Deinit);

void DPI_mipi_switch(bool on)
{
	if(on)
	{
	// may call enable_mipi(), but do this in DPI_Init_PLL
	}
	else
	{
#ifdef DPI_MIPI_API
		disable_mipi(MT65XX_MIPI_TX, "DPI");
#endif
	}
}

#ifndef BULID_UBOOT
extern UINT32 FB_Addr;
#endif
DPI_STATUS DPI_Init_PLL(unsigned int mipi_pll_clk_ref,unsigned int mipi_pll_clk_div1,unsigned int mipi_pll_clk_div2)
{
	MASKREG32(CLK_CFG_5, 0x400, 0x400);  // CLK_CFG_5[10] rg_lvds_tv_sel
                                          // 0:dpi0_ck from tvhdmi pll
                                          // 1:dpi0_ck from lvds pll

	MASKREG32(CLK_CFG_7, 0x07000000, 0x01000000); // CLK_CFG_7[26:24] lvdspll clock divider selection
                                                   // 0: from 26M
                                                   // 1: lvds_pll_ck
                                                   // 2: lvds_pll_ck/2
                                                   // 3: lvds_pll_ck/4
                                                   // 4: lvds_pll_ck/8
                                                   // CLK_CFG_7[31] 0: clock on, 1: clock off

    DRV_SetReg32(LVDSPLL_PWR_CON0, (0x1 << 0));  //PLL_PWR_ON
    udelay(2);
    DRV_ClrReg32(LVDSPLL_PWR_CON0, (0x1 << 1));  //PLL_ISO_EN

	OUTREG32(LVDSPLL_CON1, mipi_pll_clk_div2);
	OUTREG32(LVDSPLL_CON0, mipi_pll_clk_div1);
    udelay(20);

	return DPI_STATUS_OK;
}

EXPORT_SYMBOL(DPI_Init_PLL);

DPI_STATUS DPI_Set_DrivingCurrent(LCM_PARAMS *lcm_params)
{
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "DPI_Set_DrivingCurrent not implement for 6575");
	return DPI_STATUS_OK;
}

#ifdef BUILD_UBOOT
DPI_STATUS DPI_PowerOn()
{
    if (!s_isDpiPowerOn)
    {
#if 1   // FIXME
        MASKREG32(0x14000110, 0x40, 0x0);//dpi0 clock gate clear
#endif
        _RestoreDPIRegisters();
        s_isDpiPowerOn = TRUE;
    }

    return DPI_STATUS_OK;
}


DPI_STATUS DPI_PowerOff()
{
    if (s_isDpiPowerOn)
    {
        BOOL ret = TRUE;
        _BackupDPIRegisters();
#if 1   // FIXME
        MASKREG32(0x14000110, 0x40, 0x40);//dpi0 clock gate setting
#endif
        ASSERT(ret);
        s_isDpiPowerOn = FALSE;
    }

    return DPI_STATUS_OK;
}

#else

DPI_STATUS DPI_PowerOn()
{
    if (!s_isDpiPowerOn)
    {
#if 1
        int ret = enable_clock(MT_CG_DISP1_DPI0, "DPI");
		if(1 == ret)
		{
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "power manager API return FALSE\n");
		}
#endif
        enable_pll(LVDSPLL, "dpi0");
        _RestoreDPIRegisters();
        s_isDpiPowerOn = TRUE;
    }
    return DPI_STATUS_OK;
}

DPI_STATUS DPI_PowerOff()
{
    if (s_isDpiPowerOn)
    {
        int ret = TRUE;
        _BackupDPIRegisters();
		disable_pll(LVDSPLL, "dpi0");
#if 1
        ret = disable_clock(MT_CG_DISP1_DPI0, "DPI");
		if(1 == ret)
		{
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "power manager API return FALSE\n");
		}
#endif
        s_isDpiPowerOn = FALSE;
    }
    return DPI_STATUS_OK;
}
#endif
EXPORT_SYMBOL(DPI_PowerOn);

EXPORT_SYMBOL(DPI_PowerOff);

DPI_STATUS DPI_EnableClk()
{
	DPI_REG_EN en = DPI_REG->DPI_EN;
    en.EN = 1;
    OUTREG32(&DPI_REG->DPI_EN, AS_UINT32(&en));
   //release mutex0
//#ifndef BUILD_UBOOT
#if 0
    OUTREG32(DISP_MUTEX_BASE + 0x24, 0);
    while((INREG32(DISP_MUTEX_BASE + 0x24)&0x02)!=0){} // polling until mutex lock complete
#endif
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_EnableClk);

DPI_STATUS DPI_DisableClk()
{
    DPI_REG_EN en = DPI_REG->DPI_EN;
    en.EN = 0;
    OUTREG32(&DPI_REG->DPI_EN, AS_UINT32(&en));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_DisableClk);

DPI_STATUS DPI_EnableSeqOutput(BOOL enable)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_EnableSeqOutput);

DPI_STATUS DPI_SetRGBOrder(DPI_RGB_ORDER input, DPI_RGB_ORDER output)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_SetRGBOrder);

DPI_STATUS DPI_ConfigPixelClk(DPI_POLARITY polarity, UINT32 divisor, UINT32 duty)
{
    DPI_REG_CLKCNTL ctrl;

    ASSERT(divisor >= 2);
    ASSERT(duty > 0 && duty < divisor);

    ctrl.POLARITY = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    ctrl.DIVISOR = divisor - 1;
    ctrl.DUTY = duty;

    OUTREG32(&DPI_REG->CLK_CNTL, AS_UINT32(&ctrl));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigPixelClk);

DPI_STATUS DPI_ConfigLVDS(LCM_PARAMS *lcm_params)
{
	DPI_REG_CNTL ctrl = DPI_REG->CNTL;
//	DPI_REG_EMBSYNC_SETTING embsync;
//	ctrl.EMBSYNC_EN = lcm_params->dpi.embsync;
	MASKREG32(DISPSYS_BASE + 0x60, 0x7, (lcm_params->dpi.i2x_en << 1)|(lcm_params->dpi.i2x_edge << 2) | 1);
    OUTREG32(&DPI_REG->CNTL, AS_UINT32(&ctrl));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigLVDS);

DPI_STATUS DPI_ConfigHDMI()
{

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigHDMI);

DPI_STATUS DPI_ConfigDataEnable(DPI_POLARITY polarity)
{

    DPI_REG_TGEN_POL pol = DPI_REG->TGEN_POL;
    pol.DE_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    OUTREG32(&DPI_REG->TGEN_POL, AS_UINT32(&pol));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigDataEnable);

DPI_STATUS DPI_ConfigVsync(DPI_POLARITY polarity, UINT32 pulseWidth, UINT32 backPorch,
                           UINT32 frontPorch)
{
    DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd  = DPI_REG->TGEN_VWIDTH_LODD;
	DPI_REG_TGEN_VPORCH_LODD vporch_lodd  = DPI_REG->TGEN_VPORCH_LODD;
    DPI_REG_TGEN_POL pol = DPI_REG->TGEN_POL;

	pol.VSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    vwidth_lodd.VPW_LODD = pulseWidth;
    vporch_lodd.VBP_LODD= backPorch;
    vporch_lodd.VFP_LODD= frontPorch;

    OUTREG32(&DPI_REG->TGEN_POL, AS_UINT32(&pol));
    OUTREG32(&DPI_REG->TGEN_VWIDTH_LODD, AS_UINT32(&vwidth_lodd));
	OUTREG32(&DPI_REG->TGEN_VPORCH_LODD, AS_UINT32(&vporch_lodd));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigVsync);


DPI_STATUS DPI_ConfigHsync(DPI_POLARITY polarity, UINT32 pulseWidth, UINT32 backPorch,
                           UINT32 frontPorch)
{
    DPI_REG_TGEN_HPORCH hporch = DPI_REG->TGEN_HPORCH;
    DPI_REG_TGEN_POL pol = DPI_REG->TGEN_POL;

	pol.HSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    //DPI_REG->TGEN_HWIDTH = pulseWidth;
    OUTREG32(&DPI_REG->TGEN_HWIDTH,pulseWidth);
    hporch.HBP = backPorch;
    hporch.HFP = frontPorch;

    OUTREG32(&DPI_REG->TGEN_POL, AS_UINT32(&pol));
    OUTREG32(&DPI_REG->TGEN_HPORCH, AS_UINT32(&hporch));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_ConfigHsync);

DPI_STATUS DPI_FBEnable(DPI_FB_ID id, BOOL enable)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBEnable);

DPI_STATUS DPI_FBSyncFlipWithLCD(BOOL enable)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSyncFlipWithLCD);

DPI_STATUS DPI_SetDSIMode(BOOL enable)
{
    return DPI_STATUS_OK;
}


BOOL DPI_IsDSIMode(void)
{
//	return DPI_REG->CNTL.DSI_MODE ? TRUE : FALSE;
	return FALSE;
}


DPI_STATUS DPI_FBSetFormat(DPI_FB_FORMAT format)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSetFormat);

DPI_FB_FORMAT DPI_FBGetFormat(void)
{
    return 0;
}
EXPORT_SYMBOL(DPI_FBGetFormat);


DPI_STATUS DPI_FBSetSize(UINT32 width, UINT32 height)
{
    DPI_REG_SIZE size;
    size.WIDTH = width;
    size.HEIGHT = height;

    OUTREG32(&DPI_REG->SIZE, AS_UINT32(&size));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSetSize);

DPI_STATUS DPI_FBSetAddress(DPI_FB_ID id, UINT32 address)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSetAddress);

DPI_STATUS DPI_FBSetPitch(DPI_FB_ID id, UINT32 pitchInByte)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_FBSetPitch);

DPI_STATUS DPI_SetFifoThreshold(UINT32 low, UINT32 high)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI_SetFifoThreshold);


DPI_STATUS DPI_DumpRegisters(void)
{
    UINT32 i;

    DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "---------- Start dump DPI registers ----------\n");

    for (i = 0; i < sizeof(DPI_REGS); i += 4)
    {
        DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "DPI+%04x : 0x%08x\n", i, INREG32(DPI_BASE + i));
    }

    return DPI_STATUS_OK;
}

EXPORT_SYMBOL(DPI_DumpRegisters);

UINT32 DPI_GetCurrentFB(void)
{
	return 0;
}
EXPORT_SYMBOL(DPI_GetCurrentFB);

DPI_STATUS DPI_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp)
{
#if 0
    unsigned int i = 0;
    unsigned char *fbv;
    unsigned int fbsize = 0;
    unsigned int dpi_fb_bpp = 0;
    unsigned int w,h;
	BOOL dpi_needPowerOff = FALSE;
	if(!s_isDpiPowerOn){
		DPI_PowerOn();
		dpi_needPowerOff = TRUE;
		LCD_WaitForNotBusy();
	    LCD_WaitDPIIndication(FALSE);
		LCD_FBReset();
    	LCD_StartTransfer(TRUE);
		LCD_WaitDPIIndication(TRUE);
	}

    if(pvbuf == 0 || bpp == 0)
    {
        DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "DPI_Capture_Framebuffer, ERROR, parameters wrong: pvbuf=0x%08x, bpp=%d\n", pvbuf, bpp);
        return DPI_STATUS_OK;
    }

    if(DPI_FBGetFormat() == DPI_FB_FORMAT_RGB565)
    {
        dpi_fb_bpp = 16;
    }
    else if(DPI_FBGetFormat() == DPI_FB_FORMAT_RGB888)
    {
        dpi_fb_bpp = 24;
    }
    else
    {
        DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "DPI_Capture_Framebuffer, ERROR, dpi_fb_bpp is wrong: %d\n", dpi_fb_bpp);
        return DPI_STATUS_OK;
    }

    w = DISP_GetScreenWidth();
    h = DISP_GetScreenHeight();
    fbsize = w*h*dpi_fb_bpp/8;
	if(dpi_needPowerOff)
    	fbv = (unsigned char*)ioremap_cached((unsigned int)DPI_REG->FB[0].ADDR, fbsize);
	else
    	fbv = (unsigned char*)ioremap_cached((unsigned int)DPI_REG->FB[DPI_GetCurrentFB()].ADDR, fbsize);

    DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI", "current fb count is %d\n", DPI_GetCurrentFB());

    if(bpp == 32 && dpi_fb_bpp == 24)
    {
    	if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3)){
			unsigned int pix_count = w * h - 1;
    		for(i = 0;i < w*h; i++)
    		{
            	*(unsigned int*)(pvbuf+ (pix_count - i) * 4) = 0xff000000|fbv[i*3]|(fbv[i*3+1]<<8)|(fbv[i*3+2]<<16);
    		}
		}
		else{
    		for(i = 0;i < w*h; i++)
    		{
            	*(unsigned int*)(pvbuf+i*4) = 0xff000000|fbv[i*3]|(fbv[i*3+1]<<8)|(fbv[i*3+2]<<16);
    		}
		}
    }
    else if(bpp == 32 && dpi_fb_bpp == 16)
    {
        unsigned int t;
		unsigned short* fbvt = (unsigned short*)fbv;

		if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3)){
			unsigned int pix_count = w * h - 1;

    		for(i = 0;i < w*h; i++)
    		{
				t = fbvt[i];
            	*(unsigned int*)(pvbuf+ (pix_count - i) * 4) = 0xff000000|((t&0x001F)<<3)|((t&0x07E0)<<5)|((t&0xF800)<<8);
    		}
		}
		else{
        	for(i = 0;i < w*h; i++)
    		{
	    		t = fbvt[i];
            	*(unsigned int*)(pvbuf+i*4) = 0xff000000|((t&0x001F)<<3)|((t&0x07E0)<<5)|((t&0xF800)<<8);
    		}
		}
    }
    else if(bpp == 16 && dpi_fb_bpp == 16)
    {
		if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3)){
			unsigned int pix_count = w * h - 1;
			unsigned short* fbvt = (unsigned short*)fbv;
    		for(i = 0;i < w*h; i++)
    		{
            	*(unsigned short*)(pvbuf+ (pix_count - i) * 2) = fbvt[i];
    		}
		}
		else
    		memcpy((void*)pvbuf, (void*)fbv, fbsize);
    }
    else if(bpp == 16 && dpi_fb_bpp == 24)
    {
		if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3)){
			unsigned int pix_count = w * h - 1;
    		for(i = 0;i < w*h; i++)
    		{
            	*(unsigned short*)(pvbuf+ (pix_count - i) * 2) = ((fbv[i*3+0]&0xF8)>>3)|
	            	                        				((fbv[i*3+1]&0xFC)<<3)|
														    ((fbv[i*3+2]&0xF8)<<8);
    		}
		}
		else{
    		for(i = 0;i < w*h; i++)
    		{
            	*(unsigned short*)(pvbuf+i*2) = ((fbv[i*3+0]&0xF8)>>3)|
	            	                        ((fbv[i*3+1]&0xFC)<<3)|
						    				((fbv[i*3+2]&0xF8)<<8);
    		}
		}
    }
    else
    {
    	DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "DPI_Capture_Framebuffer, bpp:%d & dpi_fb_bpp:%d is not supported now\n", bpp, dpi_fb_bpp);
    }

    iounmap(fbv);

	if(dpi_needPowerOff){
		DPI_PowerOff();
	}
#else
	unsigned int mva;
    unsigned int ret = 0;
    M4U_PORT_STRUCT portStruct;

    struct disp_path_config_mem_out_struct mem_out = {0};
    printk("enter DPI_Capture_FB!\n");

    if(bpp == 32)
        mem_out.outFormat = WDMA_OUTPUT_FORMAT_ARGB;
    else if(bpp == 16)
        mem_out.outFormat = WDMA_OUTPUT_FORMAT_RGB565;
    else if(bpp == 24)
        mem_out.outFormat = WDMA_OUTPUT_FORMAT_RGB888;
    else
        printk("DPI_Capture_FB, fb color format not support\n");

    printk("before alloc MVA: va = 0x%x, size = %d\n", pvbuf, DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8);
    ret = m4u_alloc_mva(M4U_CLNTMOD_WDMA,
                        pvbuf,
                        DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8,
                        0,
                        0,
                        &mva);
    if(ret!=0)
    {
        printk("m4u_alloc_mva() fail! \n");
        return DPI_STATUS_OK;
    }
    printk("addr=0x%x, format=%d \n", mva, mem_out.outFormat);

    m4u_dma_cache_maint(M4U_CLNTMOD_WDMA,
                        (void *)pvbuf,
                        DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8,
                        DMA_BIDIRECTIONAL);

    portStruct.ePortID = M4U_PORT_WDMA1;           //hardware port ID, defined in M4U_PORT_ID_ENUM
    portStruct.Virtuality = 1;
    portStruct.Security = 0;
    portStruct.domain = 0;            //domain : 0 1 2 3
    portStruct.Distance = 1;
    portStruct.Direction = 0;
    m4u_config_port(&portStruct);

    mem_out.enable = 1;
    mem_out.dstAddr = mva;
    mem_out.srcROI.x = 0;
    mem_out.srcROI.y = 0;
    mem_out.srcROI.height= DISP_GetScreenHeight();
    mem_out.srcROI.width= DISP_GetScreenWidth();

    disp_path_get_mutex();
    disp_path_config_mem_out(&mem_out);
    printk("Wait DPI idle \n");

    disp_path_release_mutex();

    msleep(20);

    disp_path_get_mutex();
    mem_out.enable = 0;
    disp_path_config_mem_out(&mem_out);

    disp_path_release_mutex();

    portStruct.ePortID = M4U_PORT_WDMA1;           //hardware port ID, defined in M4U_PORT_ID_ENUM
    portStruct.Virtuality = 0;
    portStruct.Security = 0;
    portStruct.domain = 0;            //domain : 0 1 2 3
    portStruct.Distance = 1;
    portStruct.Direction = 0;
    m4u_config_port(&portStruct);

    m4u_dealloc_mva(M4U_CLNTMOD_WDMA,
                    pvbuf,
                        DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8,
                        mva);
#endif

    return DPI_STATUS_OK;
}

static void _DPI_RDMA0_IRQ_Handler(unsigned int param)
{
    if (param & 4)
    {
        MMProfileLog(MTKFB_MMP_Events.ScreenUpdate, MMProfileFlagEnd);
    }
    if (param & 8)
    {
        MMProfileLog(MTKFB_MMP_Events.ScreenUpdate, MMProfileFlagEnd);
    }
    if (param & 2)
    {
        MMProfileLog(MTKFB_MMP_Events.ScreenUpdate, MMProfileFlagStart);
#if (ENABLE_DPI_INTERRUPT == 0)
        if(dpiIntCallback)
            dpiIntCallback(DISP_DPI_VSYNC_INT);
#endif
    }
    if (param & 0x20)
    {
        dpiIntCallback(DISP_DPI_TARGET_LINE_INT);
    }
}

static void _DPI_MUTEX_IRQ_Handler(unsigned int param)
{
    if(dpiIntCallback)
    {
        if (param & 1)
        {
            dpiIntCallback(DISP_DPI_REG_UPDATE_INT);
        }
    }
}

DPI_STATUS DPI_EnableInterrupt(DISP_INTERRUPT_EVENTS eventID)
{
#if ENABLE_DPI_INTERRUPT
    switch(eventID)
    {
        case DISP_DPI_VSYNC_INT:
            //DPI_REG->INT_ENABLE.VSYNC = 1;
            OUTREGBIT(DPI_REG_INTERRUPT,DPI_REG->INT_ENABLE,VSYNC,1);
            break;
        case DISP_DPI_TARGET_LINE_INT:
            disp_register_irq(DISP_MODULE_RDMA0, _DPI_RDMA0_IRQ_Handler);
            break;
        case DISP_DPI_REG_UPDATE_INT:
            disp_register_irq(DISP_MODULE_MUTEX, _DPI_MUTEX_IRQ_Handler);
            break;
        default:
            return DPI_STATUS_ERROR;
    }

    return DPI_STATUS_OK;
#else
    switch(eventID)
    {
        case DISP_DPI_VSYNC_INT:
            OUTREGBIT(DPI_REG_INTERRUPT,DPI_REG->INT_ENABLE,VSYNC,1);
            disp_register_irq(DISP_MODULE_RDMA0, _DPI_RDMA0_IRQ_Handler);
            break;
        case DISP_DPI_TARGET_LINE_INT:
            disp_register_irq(DISP_MODULE_RDMA0, _DPI_RDMA0_IRQ_Handler);
            break;
        case DISP_DPI_REG_UPDATE_INT:
            disp_register_irq(DISP_MODULE_MUTEX, _DPI_MUTEX_IRQ_Handler);
            break;
        default:
            return DPI_STATUS_ERROR;
    }

    return DPI_STATUS_OK;
    ///TODO: warning log here
    //return DPI_STATUS_ERROR;
#endif
}


DPI_STATUS DPI_SetInterruptCallback(void (*pCB)(DISP_INTERRUPT_EVENTS))
{
    dpiIntCallback = pCB;

    return DPI_STATUS_OK;
}


DPI_STATUS DPI_FMDesense_Query(void)
{
    return DPI_STATUS_ERROR;
}

DPI_STATUS DPI_FM_Desense(unsigned long freq)
{
    return DPI_STATUS_OK;
}

DPI_STATUS DPI_Reset_CLK(void)
{
	return DPI_STATUS_OK;
}

DPI_STATUS DPI_Get_Default_CLK(unsigned int *clk)
{
    return DPI_STATUS_OK;
}

DPI_STATUS DPI_Get_Current_CLK(unsigned int *clk)
{
    return DPI_STATUS_OK;
}

DPI_STATUS DPI_Change_CLK(unsigned int clk)
{
    return DPI_STATUS_OK;
}
