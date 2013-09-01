

#ifdef BUILD_UBOOT
#define ENABLE_DPI1_INTERRUPT        0
#define ENABLE_DPI1_REFRESH_RATE_LOG 0

#include <asm/arch/disp_drv_platform.h>
#else

#define ENABLE_DPI1_INTERRUPT        0
#define ENABLE_DPI1_REFRESH_RATE_LOG 0

#if ENABLE_DPI1_REFRESH_RATE_LOG && !ENABLE_DPI1_INTERRUPT
#error "ENABLE_DPI1_REFRESH_RATE_LOG should be also ENABLE_DPI1_INTERRUPT"
#endif

#if defined(MTK_HDMI_SUPPORT) && !ENABLE_DPI1_INTERRUPT
//#error "enable MTK_HDMI_SUPPORT should be also ENABLE_DPI1_INTERRUPT"
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
#include "dpi1_drv.h"
#include "lcd_drv.h"
#include <mach/mt_clkmgr.h>


#if ENABLE_DPI1_INTERRUPT
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


static PDPI_REGS const DPI1_REG = (PDPI_REGS)(DPI1_BASE);
static PDSI_PHY_REGS const DSI_PHY_REG_DPI = (PDSI_PHY_REGS)(MIPI_CONFIG_BASE + 0x800);
static UINT32 const PLL_SOURCE = APMIXEDSYS_BASE + 0x44;
static BOOL s_isDpiPowerOn = FALSE;
static DPI_REGS regBackup;
static void (*dpiIntCallback)(DISP_INTERRUPT_EVENTS);

#define DPI1_REG_OFFSET(r)       offsetof(DPI_REGS, r)
#define REG_ADDR(base, offset)  (((BYTE *)(base)) + (offset))

#if !(defined(CONFIG_MT6589_FPGA) || defined(BUILD_UBOOT))
//#define DPI1_MIPI_API
#endif

const UINT32 BACKUP_DPI1_REG_OFFSETS[] =
{
    DPI1_REG_OFFSET(INT_ENABLE),
    DPI1_REG_OFFSET(SIZE),
    DPI1_REG_OFFSET(CLK_CNTL),

    DPI1_REG_OFFSET(TGEN_HWIDTH),
    DPI1_REG_OFFSET(TGEN_HPORCH),

	DPI1_REG_OFFSET(TGEN_VWIDTH_LODD),
    DPI1_REG_OFFSET(TGEN_VPORCH_LODD),

    DPI1_REG_OFFSET(TGEN_VWIDTH_LEVEN),
    DPI1_REG_OFFSET(TGEN_VPORCH_LEVEN),
    DPI1_REG_OFFSET(TGEN_VWIDTH_RODD),

    DPI1_REG_OFFSET(TGEN_VPORCH_RODD),
    DPI1_REG_OFFSET(TGEN_VWIDTH_REVEN),

	DPI1_REG_OFFSET(TGEN_VPORCH_REVEN),
    DPI1_REG_OFFSET(ESAV_VTIML),
    DPI1_REG_OFFSET(ESAV_VTIMR),
	DPI1_REG_OFFSET(ESAV_FTIM),
	DPI1_REG_OFFSET(BG_HCNTL),

  	DPI1_REG_OFFSET(BG_VCNTL),
    DPI1_REG_OFFSET(BG_COLOR),
	DPI1_REG_OFFSET(TGEN_POL),
	DPI1_REG_OFFSET(EMBSYNC_SETTING),

    DPI1_REG_OFFSET(CNTL),

    /*DPI1_REG_OFFSET(MATRIX_COEFF_SET0),
    DPI1_REG_OFFSET(MATRIX_COEFF_SET1),
    DPI1_REG_OFFSET(MATRIX_COEFF_SET2),
    DPI1_REG_OFFSET(MATRIX_COEFF_SET3),
    DPI1_REG_OFFSET(MATRIX_COEFF_SET4),
    DPI1_REG_OFFSET(MATRIX_PREADD_SET0),
    DPI1_REG_OFFSET(MATRIX_PREADD_SET1),
    DPI1_REG_OFFSET(MATRIX_POSTADD_SET0),
    DPI1_REG_OFFSET(MATRIX_POSTADD_SET1),

    DPI1_REG_OFFSET(OUTPUT_SETTING),
    DPI1_REG_OFFSET(CLPF_SETTING),
    DPI1_REG_OFFSET(PATTERN)*/
};

static void _BackupDPIRegisters(void)
{
    DPI_REGS *reg = &regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_DPI1_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(reg, BACKUP_DPI1_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(DPI1_REG, BACKUP_DPI1_REG_OFFSETS[i])));
    }
}

static void _RestoreDPIRegisters(void)
{
    DPI_REGS *reg = &regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_DPI1_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(DPI1_REG, BACKUP_DPI1_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(reg, BACKUP_DPI1_REG_OFFSETS[i])));
    }
}

static void _ResetBackupedDPIRegisterValues(void)
{
    DPI_REGS *regs = &regBackup;
    memset((void*)regs, 0, sizeof(DPI_REGS));

    OUTREG32(&regs->CLK_CNTL, 0x00000101);
}


#if ENABLE_DPI1_REFRESH_RATE_LOG
static void _DPI1_LogRefreshRate(DPI1_REG_INTERRUPT status)
{
    static unsigned long prevUs = 0xFFFFFFFF;

    if (status.VSYNC)
    {
        struct timeval curr;
        do_gettimeofday(&curr);

        if (prevUs < curr.tv_usec)
        {
            DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI1", "Receive 1 vsync in %lu us\n",
                   curr.tv_usec - prevUs);
        }
        prevUs = curr.tv_usec;
    }
}
#else
#define _DPI1_LogRefreshRate(x)  do {} while(0)
#endif

extern void dsi_handle_esd_recovery(void);

void DPI1_DisableIrq(void)
{
#if ENABLE_DPI1_INTERRUPT
		DPI_REG_INTERRUPT enInt = DPI1_REG->INT_ENABLE;
		enInt.FIFO_EMPTY = 0;
		enInt.FIFO_FULL = 0;
		enInt.OUT_EMPTY = 0;
		enInt.CNT_OVERFLOW = 0;
		enInt.LINE_ERR = 0;
		enInt.VSYNC = 0;
		OUTREG32(&DPI1_REG->INT_ENABLE, AS_UINT32(&enInt));
#endif
}
void DPI1_EnableIrq(void)
{
#if ENABLE_DPI1_INTERRUPT
		DPI_REG_INTERRUPT enInt = DPI1_REG->INT_ENABLE;
		enInt.FIFO_EMPTY = 1;
		enInt.FIFO_FULL = 0;
		enInt.OUT_EMPTY = 0;
		enInt.CNT_OVERFLOW = 0;
		enInt.LINE_ERR = 0;
		enInt.VSYNC = 1;
		OUTREG32(&DPI1_REG->INT_ENABLE, AS_UINT32(&enInt));
#endif
}

#if ENABLE_DPI1_INTERRUPT
static irqreturn_t _DPI1_InterruptHandler(int irq, void *dev_id)
{
    static int counter = 0;
    DPI_REG_INTERRUPT status = DPI1_REG->INT_STATUS;
//    if (status.FIFO_EMPTY) ++ counter;

    if(status.VSYNC)
    {
        if(dpiIntCallback)
           dpiIntCallback(DISP_DPI1_VSYNC_INT);
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
        DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI1", "[Error] DPI FIFO is empty, "
               "received %d times interrupt !!!\n", counter);
        counter = 0;
    }

    _DPI1_LogRefreshRate(status);
	OUTREG32(&DPI1_REG->INT_STATUS, 0);
    return IRQ_HANDLED;
}
#endif

#define VSYNC_US_TO_NS(x) (x * 1000)
unsigned int vsync_timer_dpi1 = 0;
void DPI1_WaitVSYNC(void)
{
#ifndef BUILD_UBOOT
	wait_dpi_vsync = true;
	hrtimer_start(&hrtimer_vsync_dpi, ktime_set(0, VSYNC_US_TO_NS(vsync_timer_dpi1)), HRTIMER_MODE_REL);
	wait_event_interruptible(_vsync_wait_queue_dpi, dpi_vsync);
	dpi_vsync = false;
	wait_dpi_vsync = false;
#endif
}

void DPI1_PauseVSYNC(bool enable)
{
}

#ifndef BUILD_UBOOT
enum hrtimer_restart dpi1_vsync_hrtimer_func(struct hrtimer *timer)
{
//	long long ret;
	if(wait_dpi_vsync)
	{
		dpi_vsync = true;
		wake_up_interruptible(&_vsync_wait_queue_dpi);
//		printk("hrtimer Vsync, and wake up\n");
	}
//	ret = hrtimer_forward_now(timer, ktime_set(0, VSYNC_US_TO_NS(vsync_timer_dpi1)));
//	printk("hrtimer callback\n");
    return HRTIMER_NORESTART;
}
#endif
void DPI1_InitVSYNC(unsigned int vsync_interval)
{
#ifndef BUILD_UBOOT
    ktime_t ktime;
	vsync_timer_dpi1 = vsync_interval;
	ktime = ktime_set(0, VSYNC_US_TO_NS(vsync_timer_dpi1));
	hrtimer_init(&hrtimer_vsync_dpi, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_vsync_dpi.function = dpi1_vsync_hrtimer_func;
//	hrtimer_start(&hrtimer_vsync_dpi, ktime, HRTIMER_MODE_REL);
#endif
}

DPI_STATUS DPI1_Init(BOOL isDpiPoweredOn)
{
    //DPI1_REG_CNTL cntl;
    //DPI1_REG_EMBSYNC_SETTING embsync;

	if (isDpiPoweredOn) {
        _BackupDPIRegisters();
    } else {
        _ResetBackupedDPIRegisterValues();
    }

    DPI1_PowerOn();

#if 0
	OUTREG32(DPI1_BASE+ 0x64, 0x400);//
	OUTREG32(DPI1_BASE+ 0x6C, 0x400);//
	OUTREG32(DPI1_BASE+ 0x74, 0x400);//
	OUTREG32(DPI1_BASE+ 0x8C, 0x0FFF0000);//
	OUTREG32(DPI1_BASE+ 0x90, 0x0FFF0000);//
	MASKREG32(DISPSYS_BASE + 0x60, 0x1, 0x1); // [1]: DPI0_I2X_EN
	                                          // 0: DPI0 IO is single edge mode
                                                //1: DPI0 IO is dual edge mode
#endif

	MASKREG32(DISPSYS_BASE + 0x60, 0xb03, 0xb03);

	//OUTREG32(DPI1_BASE+ 0x10, 0x000001A0);//DPI_CON
	//OUTREG32(DPI1_BASE+ 0x14, 0x00000101);//DPI_CLKCON
	//OUTREG32(DPI1_BASE+ 0x18, 0x02d00500);//DPI_SIZE 720x1280
	//OUTREG32(DPI1_BASE+ 0x1c, 0x00000028);//DPI_TGEN_HWIDTH 40
	//OUTREG32(DPI1_BASE+ 0x20, 0x01b800dc);//DPI_TGEN_HPORCH BACK:220 FRONT:440
	//OUTREG32(DPI1_BASE+ 0x24, 0x00000005);//DPI_TGEN_VWIDTH_LODD 5
	//OUTREG32(DPI1_BASE+ 0x28, 0x00050014);//DPI_TGEN_VPORCH_LODD BACK:20 FRONT:5
	//OUTREG32(DPI1_BASE+ 0x44, 0x00001e00);//DPI_ESAV_VTIM_L WIDTH:30 lines
	//DPI1_ESAVVTimingControlLeft(0, 0x1E, 0, 0);

	//OUTREG32(DPI1_BASE+ 0x64, 0x1ead1f53);//DPI_MATRIX_COEFF_SET0
	//OUTREG32(DPI1_BASE+ 0x68, 0x01320200);//DPI_MATRIX_COEFF_SET1
	//OUTREG32(DPI1_BASE+ 0x6c, 0x00750259);//DPI_MATRIX_COEFF_SET2
	//OUTREG32(DPI1_BASE+ 0x70, 0x1e530200);//DPI_MATRIX_COEFF_SET3
	//OUTREG32(DPI1_BASE+ 0x74, 0x00001fa0);//DPI_MATRIX_COEFF_SET4
	//DPI1_MatrixCoef(0x1F53, 0x1EAD, 0x0200, 0x0132, 0x0259, 0x0075, 0x0200, 0x1E53, 0x1FA0);

	//OUTREG32(DPI1_BASE+ 0x78, 0x00000000);//DPI_MATRIX_PREADD_SET0
	//OUTREG32(DPI1_BASE+ 0x7c, 0x00000000);//DPI_MATRIX_PREADD_SET1
	//DPI1_MatrixPreOffset(0, 0, 0);

	//OUTREG32(DPI1_BASE+ 0x80, 0x00000800);//DPI_MATRIX_POSTADD_SET0
	//OUTREG32(DPI1_BASE+ 0x84, 0x00000800);//DPI_MATRIX_POSTADD_SET1
	//DPI1_MatrixPostOffset(0x0800, 0, 0x0800);

	//OUTREG32(DPI1_BASE+ 0x88, 0x00000000);//DPI_CLPF_SETTING
	//DPI1_CLPFSetting(0, FALSE);

	//OUTREG32(DPI1_BASE+ 0x8c, 0x0f000100);//DPI_Y_LIMIT 256 - 3840
	//OUTREG32(DPI1_BASE+ 0x90, 0x0f000100);//DPI_C_LIMIT 256 - 3840
	//DPI1_SetChannelLimit(0x0100, 0x0F00, 0x0100, 0x0F00);

	//OUTREG32(DPI1_BASE+ 0x9c, 0x00000007);//DPI_EMBSYNC_SETTING
	//DPI1_EmbeddedSyncSetting(TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE);

	//OUTREG32(DPI1_BASE+ 0xa8, 0x00000600);//DPI_OUTPUT_SETTING OUT_YC_MAP
	//DPI1_OutputSetting(DPI_OUTPUT_BIT_NUM_8BITS, FALSE, DPI_OUTPUT_CHANNEL_SWAP_RGB, DPI_OUTPUT_YC_MAP_CY);

	//OUTREG32(DPI1_BASE+ 0xb4, 0x11223341);//DPI_PATTERN
	//DPI1_EnableColorBar();

	//OUTREG32(DPI1_BASE + 0x0,  0x00000001);//
	//DPI1_EnableClk();

    #if 0
	cntl = DPI1_REG->CNTL;
	cntl.EMBSYNC_EN = 1;
	OUTREG32(&DPI1_REG->CNTL, AS_UINT32(&cntl));

       embsync = DPI1_REG->EMBSYNC_SETTING;
       embsync.ESAV_CODE_MAN = 0;
       embsync.EMBVSYNC_G_Y = 1;
       embsync.EMBVSYNC_R_CR= 1;
       embsync.EMBVSYNC_B_CB= 1;
       OUTREG32(&DPI1_REG->EMBSYNC_SETTING, AS_UINT32(&embsync));
    #endif

#if ENABLE_DPI1_INTERRUPT
    if (request_irq(MT6589_DPI_IRQ_ID,
        _DPI1_InterruptHandler, IRQF_TRIGGER_LOW, "mtkdpi", NULL) < 0)
    {
        DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI1", "[ERROR] fail to request DPI irq\n");
        return DPI_STATUS_ERROR;
    }

    {
        DPI_REG_INTERRUPT enInt = DPI1_REG->INT_ENABLE;
        enInt.VSYNC = 1;
        OUTREG32(&DPI1_REG->INT_ENABLE, AS_UINT32(&enInt));
    }
#endif
	LCD_W2M_NeedLimiteSpeed(TRUE);
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_Init);

DPI_STATUS DPI1_FreeIRQ(void)
{
#if ENABLE_DPI1_INTERRUPT
    free_irq(MT6589_DPI_IRQ_ID, NULL);
#endif
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_FreeIRQ);

DPI_STATUS DPI1_Deinit(void)
{
    DPI1_DisableClk();
    DPI1_PowerOff();

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_Deinit);

void DPI1_mipi_switch(bool on)
{
	if(on)
	{
	// may call enable_mipi(), but do this in DPI1_Init_PLL
	}
	else
	{
#ifdef DPI1_MIPI_API
		disable_mipi(MT65XX_MIPI_TX, "DPI");
#endif
	}
}

#ifndef BULID_UBOOT
extern UINT32 FB_Addr;
#endif

DPI_STATUS DPI1_Init_PLL(HDMI_VIDEO_RESOLUTION resolution)
{
	//DPI1 PLL
	DRV_SetReg32(TVDPLL_PWR_CON0, (0x1 << 0));	//PLL_PWR_ON
	udelay(2);
	DRV_ClrReg32(TVDPLL_PWR_CON0, (0x1 << 1));	//PLL_ISO_EN
	switch(resolution)
	{
		case HDMI_VIDEO_720x480p_60Hz:
#ifdef MTK_MT8193_HDMI_SUPPORT
		case HDMI_VIDEO_720x576p_50Hz:
#endif
		{
			OUTREG32(TVDPLL_CON1, 0x80109d89);
			OUTREG32(TVDPLL_CON0, 0x80800081);
			break;
		}
		case HDMI_VIDEO_1920x1080p_30Hz:
#ifdef MTK_MT8193_HDMI_SUPPORT
		case HDMI_VIDEO_1280x720p_50Hz:
		case HDMI_VIDEO_1920x1080i_50Hz:
		case HDMI_VIDEO_1920x1080p_25Hz:
		case HDMI_VIDEO_1920x1080p_24Hz:
		case HDMI_VIDEO_1920x1080p_50Hz:
#endif
		{
			OUTREG32(TVDPLL_CON1, 0x800b6c4e);
			OUTREG32(TVDPLL_CON0, 0x80000081);
			break;
		}

		case HDMI_VIDEO_1280x720p_60Hz:
    #ifdef MTK_MT8193_HDMI_SUPPORT
		case HDMI_VIDEO_1920x1080i_60Hz:
		case HDMI_VIDEO_1920x1080p_23Hz:
		case HDMI_VIDEO_1920x1080p_29Hz:
		case HDMI_VIDEO_1920x1080p_60Hz:
    #endif
		{
		OUTREG32(TVDPLL_CON1, 0x800b6964);
		OUTREG32(TVDPLL_CON0, 0x80000081);
		break;
		}

		default:
		{
			printk("[DPI1] not supported format, %s, %d, format = %d\n", __func__, __LINE__, resolution);
			break;
		}
	}

	udelay(20);

	return DPI_STATUS_OK;
}

EXPORT_SYMBOL(DPI1_Init_PLL);

DPI_STATUS DPI1_Set_DrivingCurrent(LCM_PARAMS *lcm_params)
{
	DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI1", "DPI1_Set_DrivingCurrent not implement for 6575");
	return DPI_STATUS_OK;
}

#ifdef BUILD_UBOOT
DPI_STATUS DPI1_PowerOn()
{
    if (!s_isDpiPowerOn)
    {
#if 1   // FIXME
        MASKREG32(0xC2080028, 0x10, 0x10);
#endif
        _RestoreDPIRegisters();
        s_isDpiPowerOn = TRUE;
    }

    return DPI_STATUS_OK;
}


DPI_STATUS DPI1_PowerOff()
{
    if (s_isDpiPowerOn)
    {
        BOOL ret = TRUE;
        _BackupDPIRegisters();
#if 1   // FIXME
        MASKREG32(0xC2080018, 0x10, 0x10);
#endif
        ASSERT(ret);
        s_isDpiPowerOn = FALSE;
    }

    return DPI_STATUS_OK;
}

#else

DPI_STATUS DPI1_PowerOn()
{
#ifndef CONFIG_MT6589_FPGA
    if (!s_isDpiPowerOn)
    {
#if 1   // FIXME
        int ret = enable_clock(MT_CG_DISP1_DPI1, "DPI");
		if(1 == ret)
		{
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "power manager API return FALSE\n");
		}
#endif
        _RestoreDPIRegisters();
        s_isDpiPowerOn = TRUE;
    }
#endif
    return DPI_STATUS_OK;
}

DPI_STATUS DPI1_PowerOff()
{
#ifndef CONFIG_MT6589_FPGA
    if (s_isDpiPowerOn)
    {
        int ret = TRUE;
        _BackupDPIRegisters();
#if 1   // FIXME
        ret = disable_clock(MT_CG_DISP1_DPI1, "DPI");
		if(1 == ret)
		{
			DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "power manager API return FALSE\n");
		}
#endif
        s_isDpiPowerOn = FALSE;
    }
#endif
    return DPI_STATUS_OK;
}
#endif
EXPORT_SYMBOL(DPI1_PowerOn);

EXPORT_SYMBOL(DPI1_PowerOff);

DPI_STATUS DPI1_EnableClk()
{
	DPI_REG_EN en = DPI1_REG->DPI_EN;
    en.EN = 1;
    OUTREG32(&DPI1_REG->DPI_EN, AS_UINT32(&en));
   //release mutex0
//#ifndef BUILD_UBOOT
#if 0
    OUTREG32(DISP_MUTEX_BASE + 0x24, 0);
    while((INREG32(DISP_MUTEX_BASE + 0x24)&0x02)!=0){} // polling until mutex lock complete
#endif
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_EnableClk);

DPI_STATUS DPI1_DisableClk()
{
	DPI_REG_EN en = DPI1_REG->DPI_EN;
    en.EN = 0;
    OUTREG32(&DPI1_REG->DPI_EN, AS_UINT32(&en));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_DisableClk);

DPI_STATUS DPI1_EnableSeqOutput(BOOL enable)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_EnableSeqOutput);

DPI_STATUS DPI1_SetRGBOrder(DPI_RGB_ORDER input, DPI_RGB_ORDER output)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_SetRGBOrder);

DPI_STATUS DPI1_ConfigPixelClk(DPI_POLARITY polarity, UINT32 divisor, UINT32 duty)
{
	DPI_REG_CLKCNTL ctrl;

    ASSERT(divisor >= 2);
    ASSERT(duty > 0 && duty < divisor);

    ctrl.POLARITY = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    ctrl.DIVISOR = divisor - 1;
    ctrl.DUTY = duty;

    OUTREG32(&DPI1_REG->CLK_CNTL, AS_UINT32(&ctrl));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_ConfigPixelClk);

DPI_STATUS DPI1_ConfigHDMI()
{
	DPI_REG_CNTL ctrl = DPI1_REG->CNTL;
	ctrl.YUV422_EN = 1;
	ctrl.CSC_EN = 1;
    ctrl.EMBSYNC_EN = 1;
	OUTREG32(&DPI1_REG->CNTL, AS_UINT32(&ctrl));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_ConfigHDMI);


DPI_STATUS DPI1_ConfigDataEnable(DPI_POLARITY polarity)
{
	DPI_REG_TGEN_POL pol = DPI1_REG->TGEN_POL;
    pol.DE_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    OUTREG32(&DPI1_REG->TGEN_POL, AS_UINT32(&pol));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_ConfigDataEnable);

DPI_STATUS DPI1_ConfigVsync(DPI_POLARITY polarity, UINT32 pulseWidth, UINT32 backPorch,
                           UINT32 frontPorch)
{
	DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd  = DPI1_REG->TGEN_VWIDTH_LODD;
	DPI_REG_TGEN_VPORCH_LODD vporch_lodd  = DPI1_REG->TGEN_VPORCH_LODD;
    DPI_REG_TGEN_POL pol = DPI1_REG->TGEN_POL;

	pol.VSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    vwidth_lodd.VPW_LODD = pulseWidth;
    vporch_lodd.VBP_LODD= backPorch;
    vporch_lodd.VFP_LODD= frontPorch;

    OUTREG32(&DPI1_REG->TGEN_POL, AS_UINT32(&pol));
    OUTREG32(&DPI1_REG->TGEN_VWIDTH_LODD, AS_UINT32(&vwidth_lodd));
	OUTREG32(&DPI1_REG->TGEN_VPORCH_LODD, AS_UINT32(&vporch_lodd));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_ConfigVsync);


DPI_STATUS DPI1_ConfigHsync(DPI_POLARITY polarity, UINT32 pulseWidth, UINT32 backPorch,
                           UINT32 frontPorch)
{
	DPI_REG_TGEN_HPORCH hporch = DPI1_REG->TGEN_HPORCH;
    DPI_REG_TGEN_POL pol = DPI1_REG->TGEN_POL;

	pol.HSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    //DPI1_REG->TGEN_HWIDTH = pulseWidth;
    OUTREG32(&DPI1_REG->TGEN_HWIDTH,pulseWidth);
    hporch.HBP = backPorch;
    hporch.HFP = frontPorch;

    OUTREG32(&DPI1_REG->TGEN_POL, AS_UINT32(&pol));
    OUTREG32(&DPI1_REG->TGEN_HPORCH, AS_UINT32(&hporch));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_ConfigHsync);

DPI_STATUS DPI1_FBEnable(DPI_FB_ID id, BOOL enable)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_FBEnable);

DPI_STATUS DPI1_FBSyncFlipWithLCD(BOOL enable)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_FBSyncFlipWithLCD);

DPI_STATUS DPI1_SetDSIMode(BOOL enable)
{
    return DPI_STATUS_OK;
}


BOOL DPI1_IsDSIMode(void)
{
//	return DPI1_REG->CNTL.DSI_MODE ? TRUE : FALSE;
	return FALSE;
}


DPI_STATUS DPI1_FBSetFormat(DPI_FB_FORMAT format)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_FBSetFormat);

DPI_FB_FORMAT DPI1_FBGetFormat(void)
{
    return 0;
}
EXPORT_SYMBOL(DPI1_FBGetFormat);


DPI_STATUS DPI1_FBSetSize(UINT32 width, UINT32 height)
{
	DPI_REG_SIZE size;
    size.WIDTH = width;
    size.HEIGHT = height;

    OUTREG32(&DPI1_REG->SIZE, AS_UINT32(&size));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_FBSetSize);

DPI_STATUS DPI1_FBSetAddress(DPI_FB_ID id, UINT32 address)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_FBSetAddress);

DPI_STATUS DPI1_FBSetPitch(DPI_FB_ID id, UINT32 pitchInByte)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_FBSetPitch);

DPI_STATUS DPI1_SetFifoThreshold(UINT32 low, UINT32 high)
{
    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_SetFifoThreshold);


DPI_STATUS DPI1_DumpRegisters(void)
{
    UINT32 i;

    DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI1", "---------- Start dump DPI1 registers ----------\n");

    for (i = 0; i < sizeof(DPI_REGS); i += 4)
    {
        DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI1", "DPI1+%04x : 0x%08x\n", i, INREG32(DPI1_BASE + i));
    }

    return DPI_STATUS_OK;
}

EXPORT_SYMBOL(DPI1_DumpRegisters);

UINT32 DPI1_GetCurrentFB(void)
{
	return 0;
}
EXPORT_SYMBOL(DPI1_GetCurrentFB);

DPI_STATUS DPI1_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp)
{
#if 0
    unsigned int i = 0;
    unsigned char *fbv;
    unsigned int fbsize = 0;
    unsigned int dpi_fb_bpp = 0;
    unsigned int w,h;
	BOOL dpi_needPowerOff = FALSE;
	if(!s_isDpiPowerOn){
		DPI1_PowerOn();
		dpi_needPowerOff = TRUE;
		LCD_WaitForNotBusy();
	    LCD_WaitDPIIndication(FALSE);
		LCD_FBReset();
    	LCD_StartTransfer(TRUE);
		LCD_WaitDPIIndication(TRUE);
	}

    if(pvbuf == 0 || bpp == 0)
    {
        DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "DPI1_Capture_Framebuffer, ERROR, parameters wrong: pvbuf=0x%08x, bpp=%d\n", pvbuf, bpp);
        return DPI_STATUS_OK;
    }

    if(DPI1_FBGetFormat() == DPI_FB_FORMAT_RGB565)
    {
        dpi_fb_bpp = 16;
    }
    else if(DPI1_FBGetFormat() == DPI_FB_FORMAT_RGB888)
    {
        dpi_fb_bpp = 24;
    }
    else
    {
        DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "DPI1_Capture_Framebuffer, ERROR, dpi_fb_bpp is wrong: %d\n", dpi_fb_bpp);
        return DPI_STATUS_OK;
    }

    w = DISP_GetScreenWidth();
    h = DISP_GetScreenHeight();
    fbsize = w*h*dpi_fb_bpp/8;
	if(dpi_needPowerOff)
    	fbv = (unsigned char*)ioremap_cached((unsigned int)DPI1_REG->FB[0].ADDR, fbsize);
	else
    	fbv = (unsigned char*)ioremap_cached((unsigned int)DPI1_REG->FB[DPI1_GetCurrentFB()].ADDR, fbsize);

    DISP_LOG_PRINT(ANDROID_LOG_INFO, "DPI", "current fb count is %d\n", DPI1_GetCurrentFB());

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
    	DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "DPI1_Capture_Framebuffer, bpp:%d & dpi_fb_bpp:%d is not supported now\n", bpp, dpi_fb_bpp);
    }

    iounmap(fbv);

	if(dpi_needPowerOff){
		DPI1_PowerOff();
	}
#endif

    return DPI_STATUS_OK;
}

DPI_STATUS DPI1_EnableInterrupt(DISP_INTERRUPT_EVENTS eventID)
{
#if ENABLE_DPI1_INTERRUPT
    switch(eventID)
    {
        case DISP_DPI_VSYNC_INT:
            //DPI1_REG->INT_ENABLE.VSYNC = 1;
            OUTREGBIT(DPI_REG_INTERRUPT,DPI1_REG->INT_ENABLE,VSYNC,1);
            break;
        default:
            return DPI_STATUS_ERROR;
    }

    return DPI_STATUS_OK;
#else
    ///TODO: warning log here
    return DPI_STATUS_ERROR;
#endif
}


DPI_STATUS DPI1_SetInterruptCallback(void (*pCB)(DISP_INTERRUPT_EVENTS))
{
    dpiIntCallback = pCB;

    return DPI_STATUS_OK;
}


DPI_STATUS DPI1_FMDesense_Query(void)
{
    return DPI_STATUS_ERROR;
}

DPI_STATUS DPI1_FM_Desense(unsigned long freq)
{
    return DPI_STATUS_OK;
}

DPI_STATUS DPI1_Reset_CLK(void)
{
	return DPI_STATUS_OK;
}

DPI_STATUS DPI1_Get_Default_CLK(unsigned int *clk)
{
    return DPI_STATUS_OK;
}

DPI_STATUS DPI1_Get_Current_CLK(unsigned int *clk)
{
    return DPI_STATUS_OK;
}

DPI_STATUS DPI1_Change_CLK(unsigned int clk)
{
    return DPI_STATUS_OK;
}

DPI_STATUS DPI1_EnableColorBar(void)
{
	DPI_REG_PATTERN pattern = DPI1_REG->PATTERN;

    pattern.PAT_EN = 1;
	pattern.PAT_SEL = 4; // Color Bar

    OUTREG32(&DPI1_REG->PATTERN, AS_UINT32(&pattern));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_EnableColorBar);

DPI_STATUS DPI1_EnableBlackScreen(void)
{
    DPI_REG_PATTERN pattern = DPI1_REG->PATTERN;

    pattern.PAT_EN = 1;
    pattern.PAT_SEL = 5; // User defined color
    pattern.PAT_R_MAN = 0;
    pattern.PAT_G_MAN = 0;
    pattern.PAT_B_MAN = 0;

    OUTREG32(&DPI1_REG->PATTERN, AS_UINT32(&pattern));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_EnableBlackScreen);

DPI_STATUS DPI1_DisableInternalPattern(void)
{
    DPI_REG_PATTERN pattern = DPI1_REG->PATTERN;

    pattern.PAT_EN = 0;

    OUTREG32(&DPI1_REG->PATTERN, AS_UINT32(&pattern));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_DisableInternalPattern);

DPI_STATUS DPI1_MatrixCoef(UINT16 c00, UINT16 c01, UINT16 c02,
                           UINT16 c10, UINT16 c11, UINT16 c12,
                           UINT16 c20, UINT16 c21, UINT16 c22)
{
    DPI_REG_MATRIX_COEFF_SET0 set0 = DPI1_REG->MATRIX_COEFF_SET0;
    DPI_REG_MATRIX_COEFF_SET1 set1 = DPI1_REG->MATRIX_COEFF_SET1;
    DPI_REG_MATRIX_COEFF_SET2 set2 = DPI1_REG->MATRIX_COEFF_SET2;
    DPI_REG_MATRIX_COEFF_SET3 set3 = DPI1_REG->MATRIX_COEFF_SET3;
    DPI_REG_MATRIX_COEFF_SET4 set4 = DPI1_REG->MATRIX_COEFF_SET4;

    set0.MATRIX_C00 = c00; set0.MATRIX_C01 = c01;
    set1.MATRIX_C02 = c02; set1.MATRIX_C10 = c10;
    set2.MATRIX_C11 = c11; set2.MATRIX_C12 = c12;
    set3.MATRIX_C20 = c20; set3.MATRIX_C21 = c21;
    set4.MATRIX_C22 = c22;

    OUTREG32(&DPI1_REG->MATRIX_COEFF_SET0, AS_UINT32(&set0));
    OUTREG32(&DPI1_REG->MATRIX_COEFF_SET1, AS_UINT32(&set1));
    OUTREG32(&DPI1_REG->MATRIX_COEFF_SET2, AS_UINT32(&set2));
    OUTREG32(&DPI1_REG->MATRIX_COEFF_SET3, AS_UINT32(&set3));
    OUTREG32(&DPI1_REG->MATRIX_COEFF_SET4, AS_UINT32(&set4));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_MatrixCoef);

DPI_STATUS DPI1_MatrixPreOffset(UINT16 preAdd0, UINT16 preAdd1, UINT16 preAdd2)
{
    DPI_REG_MATRIX_PREADD_SET0 set0 = DPI1_REG->MATRIX_PREADD_SET0;
    DPI_REG_MATRIX_PREADD_SET1 set1 = DPI1_REG->MATRIX_PREADD_SET1;

    set0.MATRIX_PRE_ADD_0 = preAdd0; set0.MATRIX_PRE_ADD_1 = preAdd1;
    set1.MATRIX_PRE_ADD_2 = preAdd2;

    OUTREG32(&DPI1_REG->MATRIX_PREADD_SET0, AS_UINT32(&set0));
    OUTREG32(&DPI1_REG->MATRIX_PREADD_SET1, AS_UINT32(&set1));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_MatrixPreOffset);

DPI_STATUS DPI1_MatrixPostOffset(UINT16 postAdd0, UINT16 postAdd1, UINT16 postAdd2)
{
    DPI_REG_MATRIX_POSTADD_SET0 set0 = DPI1_REG->MATRIX_POSTADD_SET0;
    DPI_REG_MATRIX_POSTADD_SET1 set1 = DPI1_REG->MATRIX_POSTADD_SET1;

    set0.MATRIX_POST_ADD_0 = postAdd0; set0.MATRIX_POST_ADD_1 = postAdd1;
    set1.MATRIX_POST_ADD_2 = postAdd2;

    OUTREG32(&DPI1_REG->MATRIX_POSTADD_SET0, AS_UINT32(&set0));
    OUTREG32(&DPI1_REG->MATRIX_POSTADD_SET1, AS_UINT32(&set1));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_MatrixPostOffset);

DPI_STATUS DPI1_SetChannelLimit(UINT16 yBottom, UINT16 yTop, UINT16 cBottom, UINT16 cTop)
{
    DPI_REG_Y_LIMIT y = DPI1_REG->Y_LIMIT;
    DPI_REG_C_LIMIT c = DPI1_REG->C_LIMIT;

    y.Y_LIMIT_BOT = yBottom;
    y.Y_LIMIT_TOP = yTop;
    c.C_LIMIT_BOT = cBottom;
    c.C_LIMIT_TOP = cTop;

    OUTREG32(&DPI1_REG->Y_LIMIT, AS_UINT32(&y));
    OUTREG32(&DPI1_REG->C_LIMIT, AS_UINT32(&c));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_SetChannelLimit);

DPI_STATUS DPI1_CLPFSetting(UINT8 clpfType, BOOL roundingEnable)
{
    DPI_REG_CLPF_SETTING setting = DPI1_REG->CLPF_SETTING;

    setting.CLPF_TYPE = clpfType;
    setting.ROUND_EN = roundingEnable ? 1 : 0;

    OUTREG32(&DPI1_REG->CLPF_SETTING, AS_UINT32(&setting));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_CLPFSetting);

DPI_STATUS DPI1_EmbeddedSyncSetting(BOOL embSync_R_Cr, BOOL embSync_G_Y, BOOL embSync_B_Cb,
                                    BOOL esavFInv, BOOL esavVInv, BOOL esavHInv,
                                    BOOL esavCodeMan)
{
    DPI_REG_EMBSYNC_SETTING setting = DPI1_REG->EMBSYNC_SETTING;

    setting.EMBVSYNC_R_CR = embSync_R_Cr ? 1 : 0;
    setting.EMBVSYNC_G_Y = embSync_G_Y ? 1 : 0;
    setting.EMBVSYNC_B_CB = embSync_B_Cb ? 1 : 0;
    setting.ESAV_F_INV = esavFInv ? 1 : 0;
    setting.ESAV_V_INV = esavVInv ? 1 : 0;
    setting.ESAV_H_INV = esavHInv ? 1 : 0;
    setting.ESAV_CODE_MAN = esavCodeMan ? 1 : 0;

    OUTREG32(&DPI1_REG->EMBSYNC_SETTING, AS_UINT32(&setting));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_EmbeddedSyncSetting);

DPI_STATUS DPI1_OutputSetting(DPI_OUTPUT_BIT_NUM outBitNum, BOOL outBitSwap, DPI_OUTPUT_CHANNEL_SWAP outChSwap, DPI_OUTPUT_YC_MAP outYCMap)
{
    DPI_REG_OUTPUT_SETTING setting = DPI1_REG->OUTPUT_SETTING;

    setting.OUT_BIT = outBitNum;
    setting.OUT_BIT_SWAP = outBitSwap ? 1 : 0;
    setting.OUT_CH_SWAP = outChSwap;
    setting.OUT_YC_MAP = outYCMap;

    OUTREG32(&DPI1_REG->OUTPUT_SETTING, AS_UINT32(&setting));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_OutputSetting);

DPI_STATUS DPI1_ESAVVTimingControlLeft(UINT8 offsetOdd, UINT8 widthOdd, UINT8 offsetEven, UINT8 widthEven)
{
    DPI_REG_ESAV_VTIML setting = DPI1_REG->ESAV_VTIML;

    setting.ESAV_VOFST_LODD = offsetOdd;
    setting.ESAV_VWID_LODD = widthOdd;
    setting.ESAV_VOFST_LEVEN = offsetEven;
    setting.ESAV_VWID_LEVEN = widthEven;

    OUTREG32(&DPI1_REG->ESAV_VTIML, AS_UINT32(&setting));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_ESAVVTimingControlLeft);

DPI_STATUS DPI1_ESAVVTimingControlRight(UINT8 offsetOdd, UINT8 widthOdd, UINT8 offsetEven, UINT8 widthEven)
{
    DPI_REG_ESAV_VTIMR setting = DPI1_REG->ESAV_VTIMR;

    setting.ESAV_VOFST_RODD = offsetOdd;
    setting.ESAV_VWID_RODD = widthOdd;
    setting.ESAV_VOFST_REVEN = offsetEven;
    setting.ESAV_VWID_REVEN = widthEven;

    OUTREG32(&DPI1_REG->ESAV_VTIMR, AS_UINT32(&setting));

    return DPI_STATUS_OK;
}
EXPORT_SYMBOL(DPI1_ESAVVTimingControlRight);
