
#if defined(BUILD_UBOOT)
#define ENABLE_LCD_INTERRUPT 0 

#include <asm/arch/disp_drv_platform.h>
#else
    #if defined(CONFIG_MT6589_FPGA)
        #define ENABLE_LCD_INTERRUPT 0
    #else
        #define ENABLE_LCD_INTERRUPT 1
    #endif

#include "lcd_reg.h"
#include "lcd_drv.h"
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/module.h>

#include <linux/memory.h>
#include <linux/string.h>
#include <disp_drv_log.h>
#include <linux/dma-mapping.h>
#include "disp_drv_platform.h"

#include <linux/hrtimer.h>

#if ENABLE_LCD_INTERRUPT
//#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include "ddp_drv.h"
//#include <asm/tcm.h>
#include <mach/irqs.h>
static wait_queue_head_t _lcd_wait_queue;
#endif
static wait_queue_head_t _vsync_wait_queue;
static bool lcd_vsync = false;
static bool wait_lcd_vsync = false;
static struct hrtimer hrtimer_vsync;
#define VSYNC_US_TO_NS(x) (x * 1000)
static unsigned int vsync_timer = 0;
#include "debug.h"
#include <asm/current.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#endif
#include <mach/sync_write.h>
#ifdef OUTREG32
  #undef OUTREG32
  #define OUTREG32(x, y) mt65xx_reg_sync_writel(y, x)
#endif
#ifdef OUTREG16
  #undef OUTREG16
  #define OUTREG16(x, y) mt65xx_reg_sync_writew(y, x)
#endif
#ifdef OUTREG8
  #undef OUTREG8
  #define OUTREG8(x, y) mt65xx_reg_sync_writeb(y, x)
#endif


#ifndef OUTREGBIT
#define OUTREGBIT(TYPE,REG,bit,value)  \
                    do {    \
                        TYPE r = *((TYPE*)&INREG32(&REG));    \
                        r.bit = value;    \
                        OUTREG32(&REG, AS_UINT32(&r));    \
                    } while (0)
#endif

#define LCD_OUTREG32(addr, data)	\
		{\
		OUTREG32(addr, data);}

#define LCD_OUTREG8(addr, data)	\
		{\
		OUTREG8(addr, data);}


#define LCD_OUTREG16(addr, data)	\
		{\
		OUTREG16(addr, data);}

#define LCD_MASKREG32(addr, mask, data)	\
		{\
		MASKREG32(addr, mask, data);}


#ifdef MTK_LCDC_ENABLE_M4U
#undef MTK_LCDC_ENABLE_M4U
#endif
// UI layer, default set to 3
unsigned int FB_LAYER = DISP_DEFAULT_UI_LAYER_ID;

static size_t dbi_log_on = false;
#define DBI_LOG(fmt, arg...) \
    do { \
        if (dbi_log_on) DISP_LOG_PRINT(ANDROID_LOG_WARN, "LCD", fmt, ##arg);    \
    }while (0)

#define DBI_FUNC()	\
	do { \
		if(dbi_log_on) DISP_LOG_PRINT(ANDROID_LOG_INFO, "LCD", "[Func]%s\n", __func__);  \
	}while (0)

void dbi_log_enable(int enable)
{
    dbi_log_on = enable;
	DBI_LOG("lcd log %s\n", enable?"enabled":"disabled");
}

static PLCD_REGS const LCD_REG = (PLCD_REGS)(LCD_BASE);
static const UINT32 TO_BPP[LCD_FB_FORMAT_NUM] = {2, 3, 4};
unsigned int wait_time = 0;
typedef struct
{
    LCD_FB_FORMAT fbFormat;
    UINT32 fbPitchInBytes;
    LCD_REG_SIZE roiWndSize;
    LCD_OUTPUT_MODE outputMode;
    LCD_REGS regBackup;
    void (*pIntCallback)(DISP_INTERRUPT_EVENTS);
} LCD_CONTEXT;

static LCD_CONTEXT _lcdContext = {0};
static int wst_step_LCD = -1;//for LCD&FM de-sense
static bool is_get_default_write_cycle = FALSE;
static unsigned int default_write_cycle = 0;
static UINT32 default_wst = 0;
static bool limit_w2m_speed = false;
static bool limit_w2tvr_speed = false;
extern OVL_CONFIG_STRUCT cached_layer_config[DDP_OVL_LAYER_MUN];
extern OVL_CONFIG_STRUCT* realtime_layer_config;
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
LCD_STATUS LCD_Init_IO_pad(LCM_PARAMS *lcm_params)
{
//    #warning "LCD_Init_IO_pad not implement for 6589 yes"
	
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_Set_DrivingCurrent(LCM_PARAMS *lcm_params)
{
//    #warning "LCD_Set_DrivingCurrent not implement for 6589 yes"
	return LCD_STATUS_OK;
}

#if ENABLE_LCD_INTERRUPT
static irqreturn_t _LCD_InterruptHandler(int irq, void *dev_id)
{   
    LCD_REG_INTERRUPT status = LCD_REG->INT_STATUS;

    if (status.COMPLETED)
    {
#ifdef CONFIG_MTPROF_APPLAUNCH  // eng enable, user disable   	
        LOG_PRINT(ANDROID_LOG_INFO, "AppLaunch", "LCD frame buffer update done !\n");
#endif
        wake_up_interruptible(&_lcd_wait_queue);

        if(_lcdContext.pIntCallback)
            _lcdContext.pIntCallback(DISP_LCD_TRANSFER_COMPLETE_INT);

        DBG_OnLcdDone();
		LCD_MASKREG32(&LCD_REG->INT_STATUS, 0x1, 0x0);
    }

    if (status.SYNC)// this is TE mode 0 interrupt
    {
        if(_lcdContext.pIntCallback)
            _lcdContext.pIntCallback(DISP_LCD_SYNC_INT);
#ifndef BUILD_UBOOT    
		if(wait_lcd_vsync)//judge if wait vsync
		{
			if(-1 != hrtimer_try_to_cancel(&hrtimer_vsync)){
				lcd_vsync = true;
			
//			hrtimer_try_to_cancel(&hrtimer_vsync);

				wake_up_interruptible(&_vsync_wait_queue);
			}
//			printk("TE signal, and wake up\n");
		}
#endif  
		DBG_OnTeDelayDone();
		LCD_MASKREG32(&LCD_REG->INT_STATUS, 0x8, 0x0);
    }
#if 0  //TE mode 1
	if(status.HTT)
	{
        if(_lcdContext.pIntCallback)
            _lcdContext.pIntCallback(DISP_LCD_HTT_INT);
        
		DBG_OnTeDelayDone();
	}
#endif
//	LCD_OUTREG32(&LCD_REG->INT_STATUS, 0);
    return IRQ_HANDLED;
}
#endif

#if 1
static BOOL _IsEngineBusy(void)
{
    LCD_REG_STATUS status;

    status = LCD_REG->STATUS;
    if (status.BUSY) 
        return TRUE;

    return FALSE;
}
#else
static BOOL _IsEngineBusy(void)
{
    LCD_REG_INTERRUPT status;

    status = LCD_REG->INT_STATUS;
    if (status.COMPLETED){
		MASKREG32(&LCD_REG->INT_STATUS, 0x1, 0x1);
		return TRUE;
    }

    return FALSE;
}

#endif


BOOL LCD_IsBusy(void)
{
	return _IsEngineBusy();
}


static void _WaitForEngineNotBusy(void)
{
#if ENABLE_LCD_INTERRUPT
    static const long WAIT_TIMEOUT = 2 * HZ;    // 2 sec

    if (in_interrupt())
    {
        // perform busy waiting if in interrupt context
        while(_IsEngineBusy()) {}
    }
    else
    {
        while (_IsEngineBusy())
        {
            long ret = wait_event_interruptible_timeout(_lcd_wait_queue, 
                                                        !_IsEngineBusy(),
                                                        WAIT_TIMEOUT);
            if (0 == ret) {
                DISP_LOG_PRINT(ANDROID_LOG_WARN, "LCD", "[WARNING] Wait for LCD engine not busy timeout!!!\n"); 
						LCD_DumpRegisters();
						if(LCD_REG->STATUS.WAIT_SYNC){
                    DISP_LOG_PRINT(ANDROID_LOG_WARN, "LCD", "reason is LCD can't wait TE signal!!!\n"); 
							LCD_TE_Enable(FALSE);
						}
						OUTREG16(&LCD_REG->START, 0);
						OUTREG16(&LCD_REG->START, 0x1);
            }
        }
    }
#else
	while(_IsEngineBusy()) {
		msleep(1);//sleep 1ms
		wait_time++;
		if(wait_time>2000){ //timeout
			printk("[WARNING] Wait for LCD engine not busy timeout!!!\n");
			LCD_DumpRegisters();

			if(LCD_REG->STATUS.WAIT_SYNC){
				printk("reason is LCD can't wait TE signal!!!\n");
				LCD_TE_Enable(FALSE);
			}
			OUTREG16(&LCD_REG->START, 0);
			OUTREG16(&LCD_REG->START, 0x1);
			wait_time = 0;
		}
    }
    wait_time = 0;
#endif    
}


void LCD_WaitTE(void)
{
#ifndef BUILD_UBOOT	
	wait_lcd_vsync = true;
	hrtimer_start(&hrtimer_vsync, ktime_set(0, VSYNC_US_TO_NS(vsync_timer)), HRTIMER_MODE_REL);
	wait_event_interruptible(_vsync_wait_queue, lcd_vsync);
	lcd_vsync = false;
	wait_lcd_vsync = false;
#endif
}

#ifndef BUILD_UBOOT
void LCD_GetVsyncCnt()
{
}
enum hrtimer_restart lcd_te_hrtimer_func(struct hrtimer *timer)
{
//	long long ret;
	if(wait_lcd_vsync)
	{
		lcd_vsync = true;
		wake_up_interruptible(&_vsync_wait_queue);
//		printk("hrtimer Vsync, and wake up\n");
	}
//	ret = hrtimer_forward_now(timer, ktime_set(0, VSYNC_US_TO_NS(vsync_timer)));
//	printk("hrtimer callback\n");
    return HRTIMER_NORESTART;
}
#endif
void LCD_InitVSYNC(unsigned int vsync_interval)
{
#ifndef BUILD_UBOOT
    ktime_t ktime;
	vsync_timer = vsync_interval;
	ktime = ktime_set(0, VSYNC_US_TO_NS(vsync_timer));
	hrtimer_init(&hrtimer_vsync, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_vsync.function = lcd_te_hrtimer_func;
//	hrtimer_start(&hrtimer_vsync, ktime, HRTIMER_MODE_REL);
#endif
}
void LCD_PauseVSYNC(bool enable)
{
}

static void _BackupLCDRegisters(void)
{
//    memcpy((void*)&(_lcdContext.regBackup), (void*)LCD_REG, sizeof(LCD_REGS));
    LCD_REGS *regs = &(_lcdContext.regBackup);
    UINT32 i;
    LCD_OUTREG32(&regs->INT_ENABLE, AS_UINT32(&LCD_REG->INT_ENABLE));
    LCD_OUTREG32(&regs->SERIAL_CFG, AS_UINT32(&LCD_REG->SERIAL_CFG));

	for(i = 0; i < ARY_SIZE(LCD_REG->SIF_TIMING); ++i)
    {
        LCD_OUTREG32(&regs->SIF_TIMING[i], AS_UINT32(&LCD_REG->SIF_TIMING[i]));
    }

    for(i = 0; i < ARY_SIZE(LCD_REG->PARALLEL_CFG); ++i)
    {
        LCD_OUTREG32(&regs->PARALLEL_CFG[i], AS_UINT32(&LCD_REG->PARALLEL_CFG[i]));
    }

    LCD_OUTREG32(&regs->TEARING_CFG, AS_UINT32(&LCD_REG->TEARING_CFG));
    LCD_OUTREG32(&regs->PARALLEL_DW, AS_UINT32(&LCD_REG->PARALLEL_DW));
	LCD_OUTREG32(&regs->CALC_HTT, AS_UINT32(&LCD_REG->CALC_HTT));
	LCD_OUTREG32(&regs->SYNC_LCM_SIZE, AS_UINT32(&LCD_REG->SYNC_LCM_SIZE));
	LCD_OUTREG32(&regs->SYNC_CNT, AS_UINT32(&LCD_REG->SYNC_CNT));
	LCD_OUTREG32(&regs->SMI_CON, AS_UINT32(&LCD_REG->SMI_CON));

    LCD_OUTREG32(&regs->WROI_CONTROL, AS_UINT32(&LCD_REG->WROI_CONTROL));
    LCD_OUTREG32(&regs->WROI_CMD_ADDR, AS_UINT32(&LCD_REG->WROI_CMD_ADDR));
    LCD_OUTREG32(&regs->WROI_DATA_ADDR, AS_UINT32(&LCD_REG->WROI_DATA_ADDR));
    LCD_OUTREG32(&regs->WROI_SIZE, AS_UINT32(&LCD_REG->WROI_SIZE));

//	LCD_OUTREG32(&regs->DITHER_CON, AS_UINT32(&LCD_REG->DITHER_CON));
	LCD_OUTREG32(&regs->SRC_CON, AS_UINT32(&LCD_REG->SRC_CON));

	LCD_OUTREG32(&regs->SRC_ADD, AS_UINT32(&LCD_REG->SRC_ADD));
	LCD_OUTREG32(&regs->SRC_PITCH, AS_UINT32(&LCD_REG->SRC_PITCH));
	
	LCD_OUTREG32(&regs->ULTRA_CON, AS_UINT32(&LCD_REG->ULTRA_CON));
	LCD_OUTREG32(&regs->DBI_ULTRA_TH, AS_UINT32(&LCD_REG->DBI_ULTRA_TH));

	LCD_OUTREG32(&regs->GMC_ULTRA_TH, AS_UINT32(&LCD_REG->GMC_ULTRA_TH));
}


static void _RestoreLCDRegisters(void)
{
    LCD_REGS *regs = &(_lcdContext.regBackup);
    UINT32 i;
    LCD_OUTREG32(&LCD_REG->INT_ENABLE, AS_UINT32(&regs->INT_ENABLE));
    LCD_OUTREG32(&LCD_REG->SERIAL_CFG, AS_UINT32(&regs->SERIAL_CFG));

	for(i = 0; i < ARY_SIZE(LCD_REG->SIF_TIMING); ++i)
    {
        LCD_OUTREG32(&LCD_REG->SIF_TIMING[i], AS_UINT32(&regs->SIF_TIMING[i]));
    }

    for(i = 0; i < ARY_SIZE(LCD_REG->PARALLEL_CFG); ++i)
    {
        LCD_OUTREG32(&LCD_REG->PARALLEL_CFG[i], AS_UINT32(&regs->PARALLEL_CFG[i]));
    }

    LCD_OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&regs->TEARING_CFG));
    LCD_OUTREG32(&LCD_REG->PARALLEL_DW, AS_UINT32(&regs->PARALLEL_DW));
	LCD_OUTREG32(&LCD_REG->CALC_HTT, AS_UINT32(&regs->CALC_HTT));
	LCD_OUTREG32(&LCD_REG->SYNC_LCM_SIZE, AS_UINT32(&regs->SYNC_LCM_SIZE));
	LCD_OUTREG32(&LCD_REG->SYNC_CNT, AS_UINT32(&regs->SYNC_CNT));
	LCD_OUTREG32(&LCD_REG->SMI_CON, AS_UINT32(&regs->SMI_CON));

    LCD_OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&regs->WROI_CONTROL));
    LCD_OUTREG32(&LCD_REG->WROI_CMD_ADDR, AS_UINT32(&regs->WROI_CMD_ADDR));
    LCD_OUTREG32(&LCD_REG->WROI_DATA_ADDR, AS_UINT32(&regs->WROI_DATA_ADDR));
    LCD_OUTREG32(&LCD_REG->WROI_SIZE, AS_UINT32(&regs->WROI_SIZE));

//	LCD_OUTREG32(&LCD_REG->DITHER_CON, AS_UINT32(&regs->DITHER_CON));
	
	LCD_OUTREG32(&LCD_REG->SRC_CON, AS_UINT32(&regs->SRC_CON));

	LCD_OUTREG32(&LCD_REG->SRC_ADD, AS_UINT32(&regs->SRC_ADD));
	LCD_OUTREG32(&LCD_REG->SRC_PITCH, AS_UINT32(&regs->SRC_PITCH));
	
	LCD_OUTREG32(&LCD_REG->ULTRA_CON, AS_UINT32(&regs->ULTRA_CON));
	LCD_OUTREG32(&LCD_REG->DBI_ULTRA_TH, AS_UINT32(&regs->DBI_ULTRA_TH));

	LCD_OUTREG32(&LCD_REG->GMC_ULTRA_TH, AS_UINT32(&regs->GMC_ULTRA_TH));
}


static void _ResetBackupedLCDRegisterValues(void)
{
    LCD_REGS *regs = &_lcdContext.regBackup;
    memset((void*)regs, 0, sizeof(LCD_REGS));

    LCD_OUTREG32(&regs->SERIAL_CFG, 0x00003000);
    LCD_OUTREG32(&regs->PARALLEL_CFG[0], 0x00FC0000);
    LCD_OUTREG32(&regs->PARALLEL_CFG[1], 0x00300000);
    LCD_OUTREG32(&regs->PARALLEL_CFG[2], 0x00300000);
}


// ---------------------------------------------------------------------------
//  LCD Controller API Implementations
// ---------------------------------------------------------------------------

LCD_STATUS LCD_Init(void)
{
    LCD_STATUS ret = LCD_STATUS_OK;

    memset(&_lcdContext, 0, sizeof(_lcdContext));

    // LCD controller would NOT reset register as default values
    // Do it by SW here
    //
    _ResetBackupedLCDRegisterValues();

    ret = LCD_PowerOn();

	LCD_OUTREG32(&LCD_REG->SYNC_LCM_SIZE, 0x00010001);
	LCD_OUTREG32(&LCD_REG->SYNC_CNT, 0x1);

    ASSERT(ret == LCD_STATUS_OK);
//	MASKREG32(DISPSYS_BASE + 0x60, 0x1, 0x0);

#if ENABLE_LCD_INTERRUPT
    if (request_irq(MT6589_DISP_DBI_IRQ_ID,
        _LCD_InterruptHandler, IRQF_TRIGGER_LOW, "mtklcd", NULL) < 0)
    {
        DBI_LOG("[LCD][ERROR] fail to request LCD irq\n"); 
        return LCD_STATUS_ERROR;
    }
    init_waitqueue_head(&_lcd_wait_queue);
    init_waitqueue_head(&_vsync_wait_queue);
    //LCD_REG->INT_ENABLE.COMPLETED = 1;
    OUTREGBIT(LCD_REG_INTERRUPT,LCD_REG->INT_ENABLE,COMPLETED,1);
//	LCD_REG->INT_ENABLE.REG_COMPLETED = 1;
	//LCD_REG->INT_ENABLE.CMDQ_COMPLETED = 1;
	//LCD_REG->INT_ENABLE.HTT = 1;
    //LCD_REG->INT_ENABLE.SYNC = 1;
    OUTREGBIT(LCD_REG_INTERRUPT,LCD_REG->INT_ENABLE,CMDQ_COMPLETED,1);
    OUTREGBIT(LCD_REG_INTERRUPT,LCD_REG->INT_ENABLE,HTT,1);
    OUTREGBIT(LCD_REG_INTERRUPT,LCD_REG->INT_ENABLE,SYNC,1);
#endif
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_Deinit(void)
{
    LCD_STATUS ret = LCD_PowerOff();

    ASSERT(ret == LCD_STATUS_OK);

    return LCD_STATUS_OK;
}

static BOOL s_isLcdPowerOn = FALSE;

#ifdef BUILD_UBOOT
LCD_STATUS LCD_PowerOn(void)
{
    if (!s_isLcdPowerOn)
    {
		LCD_MASKREG32(0x14000110, 0x307, 0x0);
		printf("0x%8x\n", INREG32(0x14000110));
        _RestoreLCDRegisters();
        s_isLcdPowerOn = TRUE;
    }

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_PowerOff(void)
{
    if (s_isLcdPowerOn)
    {
        int ret = 1;
        _WaitForEngineNotBusy();
        _BackupLCDRegisters();
#if 1   // FIXME
		LCD_MASKREG32(0x14000110, 0x307, 0x307);
		printf("0x%8x\n", INREG32(0x14000110));
#endif           
        s_isLcdPowerOn = FALSE;
    }

    return LCD_STATUS_OK;
}

#else
LCD_STATUS LCD_PowerOn(void)
{
    DBI_FUNC();
    if (!s_isLcdPowerOn)
    {
		int ret = 0;
#if 1
		DBI_LOG("lcd will be power on\n");
      	ret = enable_clock(MT_CG_DISP1_DBI_ENGINE, "LCD");
		ret += enable_clock(MT_CG_DISP1_DBI_SMI, "LCD");
		ret += enable_clock(MT_CG_DISP1_DBI_OUTPUT, "LCD");
		ret += enable_clock(MT_CG_DISP1_LCD, "LCD");
		ret += enable_clock(MT_CG_DISP1_SLCD, "LCD");
		if(ret > 0)
		{
			DBI_LOG("[LCD]power manager API return FALSE\n");
		}      
#endif
        _RestoreLCDRegisters();
        s_isLcdPowerOn = TRUE;
    }
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_PowerOff(void)
{
    DBI_FUNC();
    if (s_isLcdPowerOn)
    {
        int ret = 1;
        _WaitForEngineNotBusy();
        _BackupLCDRegisters();
	DBI_LOG("lcd will be power off\n");
#if 1
			ret = disable_clock(MT_CG_DISP1_SLCD, "LCD");
			ret += disable_clock(MT_CG_DISP1_LCD, "LCD");
			ret += disable_clock(MT_CG_DISP1_DBI_ENGINE, "LCD");
			ret += disable_clock(MT_CG_DISP1_DBI_SMI, "LCD");
			ret += disable_clock(MT_CG_DISP1_DBI_OUTPUT, "LCD");
			if(ret > 0)
			{
				DBI_LOG("[LCD]power manager API return FALSE\n");
			} 			
#endif
        s_isLcdPowerOn = FALSE;
    }
    return LCD_STATUS_OK;
}
#endif

LCD_STATUS LCD_WaitForNotBusy(void)
{
    _WaitForEngineNotBusy();
    return LCD_STATUS_OK;
}
EXPORT_SYMBOL(LCD_WaitForNotBusy);

static void _DBI_RDMA0_IRQ_Handler(unsigned int param)
{
    if(_lcdContext.pIntCallback)
    {
        if (param & 4)
        {
            MMProfileLogEx(MTKFB_MMP_Events.ScreenUpdate, MMProfileFlagEnd, param, 0);
        }
        if (param & 8)
        {
            MMProfileLogEx(MTKFB_MMP_Events.ScreenUpdate, MMProfileFlagEnd, param, 0);
        }
        if (param & 2)
        {
            MMProfileLogEx(MTKFB_MMP_Events.ScreenUpdate, MMProfileFlagStart, param, 0);
            _lcdContext.pIntCallback(DISP_LCD_SYNC_INT);
        }
    }
}

static void _DBI_MUTEX_IRQ_Handler(unsigned int param)
{
    if(_lcdContext.pIntCallback)
    {
        if (param & 1)
        {
            _lcdContext.pIntCallback(DISP_LCD_REG_COMPLETE_INT);
        }
    }
}

LCD_STATUS LCD_EnableInterrupt(DISP_INTERRUPT_EVENTS eventID)
{
#if ENABLE_LCD_INTERRUPT
    switch(eventID)
    {
        case DISP_LCD_TRANSFER_COMPLETE_INT:
            //LCD_REG->INT_ENABLE.COMPLETED = 1;
            OUTREGBIT(LCD_REG_INTERRUPT,LCD_REG->INT_ENABLE,COMPLETED,1);
            break;
        case DISP_LCD_REG_COMPLETE_INT:
            disp_register_irq(DISP_MODULE_MUTEX, _DBI_MUTEX_IRQ_Handler);
            break;
        case DISP_LCD_CDMQ_COMPLETE_INT:
            //LCD_REG->INT_ENABLE.CMDQ_COMPLETED = 1;
            OUTREGBIT(LCD_REG_INTERRUPT,LCD_REG->INT_ENABLE,CMDQ_COMPLETED,1);
            break;
        case DISP_LCD_HTT_INT:
            //LCD_REG->INT_ENABLE.HTT = 1;
            OUTREGBIT(LCD_REG_INTERRUPT,LCD_REG->INT_ENABLE,HTT,1);
            break;
        case DISP_LCD_SYNC_INT:
            //LCD_REG->INT_ENABLE.SYNC = 1;
            disp_register_irq(DISP_MODULE_RDMA0, _DBI_RDMA0_IRQ_Handler);
            break;
        default:
            return LCD_STATUS_ERROR;
    }

    return LCD_STATUS_OK;
#else
    ///TODO: warning log here
    return LCD_STATUS_ERROR;
#endif
}


LCD_STATUS LCD_SetInterruptCallback(void (*pCB)(DISP_INTERRUPT_EVENTS))
{
    _lcdContext.pIntCallback = pCB;

    return LCD_STATUS_OK;
}


// -------------------- LCD Controller Interface --------------------
LCD_STATUS LCD_ConfigParallelIF(LCD_IF_ID id,
                                LCD_IF_PARALLEL_BITS ifDataWidth,
                                LCD_IF_PARALLEL_CLK_DIV clkDivisor,
                                UINT32 writeSetup,
                                UINT32 writeHold,
                                UINT32 writeWait,
                                UINT32 readSetup,
								UINT32 readHold,
                                UINT32 readLatency,
                                UINT32 waitPeriod,
								UINT32 chw)
{
    ASSERT(id >= LCD_IF_PARALLEL_0 && id <= LCD_IF_PARALLEL_2);
    ASSERT(writeSetup <= 16U);
    ASSERT(writeHold <= 16U);
    ASSERT(writeWait <= 64U);
    ASSERT(readSetup <= 16U);
	ASSERT(readHold <= 16U);
    ASSERT(readLatency <= 64U);
    ASSERT(chw <= 16U);

    if (0 == writeHold)   writeHold = 1;
    if (0 == writeWait)   writeWait = 1;
    if (0 == readLatency) readLatency = 1;

    _WaitForEngineNotBusy();

    // (1) Config Data Width
    {
        LCD_REG_PCNFDW pcnfdw = LCD_REG->PARALLEL_DW;

        switch(id)
        {
        case LCD_IF_PARALLEL_0: pcnfdw.PCNF0_DW = (UINT32)ifDataWidth; pcnfdw.PCNF0_CHW = chw;break;
        case LCD_IF_PARALLEL_1: pcnfdw.PCNF1_DW = (UINT32)ifDataWidth; pcnfdw.PCNF1_CHW = chw;break;
        case LCD_IF_PARALLEL_2: pcnfdw.PCNF2_DW = (UINT32)ifDataWidth; pcnfdw.PCNF2_CHW = chw;break;
        default : ASSERT(0);
        };

        LCD_OUTREG32(&LCD_REG->PARALLEL_DW, AS_UINT32(&pcnfdw));
    }

    // (2) Config Timing
    {
        UINT32 i;
        LCD_REG_PCNF config;
        
        i = (UINT32)id - LCD_IF_PARALLEL_0;
        config = LCD_REG->PARALLEL_CFG[i];

        config.C2WS = writeSetup;
        config.C2WH = writeHold - 1;
        config.WST  = writeWait - 1;
        config.C2RS = readSetup;
        config.C2RH = readHold;
        config.RLT  = readLatency - 1;

        LCD_OUTREG32(&LCD_REG->PARALLEL_CFG[i], AS_UINT32(&config));
    }

    return LCD_STATUS_OK;
}

LCD_STATUS LCD_ConfigIfFormat(LCD_IF_FMT_COLOR_ORDER order,
                              LCD_IF_FMT_TRANS_SEQ transSeq,
                              LCD_IF_FMT_PADDING padding,
                              LCD_IF_FORMAT format,
                              LCD_IF_WIDTH busWidth)
{
    LCD_REG_WROI_CON ctrl = LCD_REG->WROI_CONTROL;


    ctrl.RGB_ORDER  = order;
    ctrl.BYTE_ORDER = transSeq;
    ctrl.PADDING    = padding;
    ctrl.DATA_FMT   = (UINT32)format;
    ctrl.IF_FMT   = (UINT32)busWidth;
    ctrl.IF_24 = 0;
    if(busWidth == LCD_IF_WIDTH_24_BITS)
	{
	    ctrl.IF_24 = 1;
	}
    LCD_OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&ctrl));


    return LCD_STATUS_OK;
}

LCD_STATUS LCD_ConfigSerialIF(LCD_IF_ID id,
                              LCD_IF_SERIAL_BITS bits,
                              UINT32 three_wire,
                              UINT32 sdi,
                              BOOL   first_pol,
                              BOOL   sck_def,
                              UINT32 div2,
                              UINT32 hw_cs,
                              UINT32 css,
                              UINT32 csh,
                              UINT32 rd_1st,
                              UINT32 rd_2nd,
                              UINT32 wr_1st,
                              UINT32 wr_2nd)
{
    LCD_REG_SCNF config;
	LCD_REG_SIF_TIMING sif_timing;
	unsigned int offset = 0;
	unsigned int sif_id = 0;
    
	ASSERT(id >= LCD_IF_SERIAL_0 && id <= LCD_IF_SERIAL_1);

    _WaitForEngineNotBusy();

    memset(&config, 0, sizeof(config));
	
	if(id == LCD_IF_SERIAL_1){
		offset = 8;
		sif_id = 1;
	}
	
	LCD_MASKREG32(&config, 0x07 << offset, bits << offset);
	LCD_MASKREG32(&config, 0x08 << offset, three_wire << (offset + 3));
	LCD_MASKREG32(&config, 0x10 << offset, sdi << (offset + 4));
	LCD_MASKREG32(&config, 0x20 << offset, first_pol << (offset + 5));
	LCD_MASKREG32(&config, 0x40 << offset, sck_def << (offset + 6));
	LCD_MASKREG32(&config, 0x80 << offset, div2 << (offset + 7));

	config.HW_CS = hw_cs;
//	config.SIZE_0 = bits;
    LCD_OUTREG32(&LCD_REG->SERIAL_CFG, AS_UINT32(&config));

	sif_timing.WR_2ND = wr_2nd;
	sif_timing.WR_1ST = wr_1st;
	sif_timing.RD_2ND = rd_2nd;
	sif_timing.RD_1ST = rd_1st;
	sif_timing.CSH = csh;
	sif_timing.CSS = css;
	
	LCD_OUTREG32(&LCD_REG->SIF_TIMING[sif_id], AS_UINT32(&sif_timing));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_SetResetSignal(BOOL high)
{
	UINT32 reset = high ? 1 : 0;
    //LCD_REG->RESET = high ? 1 : 0;
    OUTREG32(&LCD_REG->RESET,reset);

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_ConfigDSIIfFormat(LCD_DSI_IF_FMT_COLOR_ORDER order,
                              LCD_DSI_IF_FMT_TRANS_SEQ transSeq,
                              LCD_DSI_IF_FMT_PADDING padding,
                              LCD_DSI_IF_FORMAT format,
                              UINT32 packet_size,
                              bool DC_DSI)
{
	//MT6583 not support
	return LCD_STATUS_OK;
}


// -------------------- Command Queue --------------------

LCD_STATUS LCD_CmdQueueEnable(BOOL enabled)
{
    LCD_REG_WROI_CON ctrl;

//    _WaitForEngineNotBusy();

    ctrl = LCD_REG->WROI_CONTROL;
    ctrl.ENC = enabled ? 1 : 0;
    LCD_OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&ctrl));

    return LCD_STATUS_OK;
}

LCD_STATUS LCD_CmdQueueWrite(UINT32 *cmds, UINT32 cmdCount)
{
    LCD_REG_WROI_CON ctrl;
    UINT32 i;

    ASSERT(cmdCount >= 0 && cmdCount < ARY_SIZE(LCD_REG->CMDQ));
    
//    _WaitForEngineNotBusy();
    ctrl = LCD_REG->WROI_CONTROL;
    ctrl.COMMAND = cmdCount - 1;
    LCD_OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&ctrl));

    for (i = 0; i < cmdCount; ++ i)
    {
        //LCD_REG->CMDQ[i] = cmds[i];
        OUTREG32(&LCD_REG->CMDQ[i],cmds[i]);
    }

    return LCD_STATUS_OK;
}


// -------------------- Layer Configurations --------------------
LCD_STATUS LCD_LayerEnable(LCD_LAYER_ID id, BOOL enable)
{
	//MT6583 not support, if be called, it should call OVL function
	if(LCD_LAYER_ALL == id)return LCD_STATUS_OK;
//	cached_layer_config[id].layer= id;
	cached_layer_config[id].layer_en= enable;
    cached_layer_config[id].isDirty = true;
//	disp_path_config_layer(&cached_layer_config[id]);
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_ConfigDither(int lrs, int lgs, int lbs, int dbr, int dbg, int dbb)
{
/*
	LCD_REG_DITHER_CON ctrl;

//	_WaitForEngineNotBusy();

	ctrl = LCD_REG->DITHER_CON;

	ctrl.LFSR_R_SEED = lrs;
	ctrl.LFSR_G_SEED = lgs;
	ctrl.LFSR_B_SEED = lbs;
	ctrl.DB_R = dbr;
	ctrl.DB_G = dbg;
	ctrl.DB_B = dbb;

	OUTREG32(&LCD_REG->DITHER_CON, AS_UINT32(&ctrl));
*/
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_LayerEnableDither(LCD_LAYER_ID id, UINT32 enable)
{
    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);

//    _WaitForEngineNotBusy();
#if 0
    if (LCD_LAYER_ALL == id)
    {
        return LCD_STATUS_OK;
    }
    else if (id < LCD_LAYER_NUM)
    {
		LCD_REG_LAYER_CON ctrl = LCD_REG->LAYER[id].CONTROL;
        ctrl.DITHER_EN = enable;
        OUTREG32(&LCD_REG->LAYER[id].CONTROL, AS_UINT32(&ctrl));
    }
#endif    
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_LayerSetAddress(LCD_LAYER_ID id, UINT32 address)
{
	cached_layer_config[id].addr = address;
    cached_layer_config[id].isDirty = true;
//	if(FB_LAYER == id || (FB_LAYER + 1) == id)
//		disp_path_config_layer_addr((unsigned int)id,address);
    return LCD_STATUS_OK;

}

UINT32 LCD_LayerEnableTdshp(LCD_LAYER_ID id, UINT32 en)
{
    cached_layer_config[id].isTdshp = en;
    return LCD_STATUS_OK;
}

UINT32 LCD_LayerSetNextBuffIdx(LCD_LAYER_ID id, INT32 idx)
{
    cached_layer_config[id].buff_idx = idx;
    return LCD_STATUS_OK;
}

UINT32 LCD_LayerGetInfo(LCD_LAYER_ID id, UINT32 *enabled, INT32 *curr_idx, INT32 *next_idx)
{
    *enabled = realtime_layer_config[id].layer_en;
    *curr_idx = realtime_layer_config[id].buff_idx;
    *next_idx = cached_layer_config[id].buff_idx;
    return LCD_STATUS_OK;
}

UINT32 LCD_DisableAllLayer(UINT32 vram_start, UINT32 vram_end)
{
    int id;
    int layer_enable = 0;
    DISP_LOG_PRINT(ANDROID_LOG_INFO, "LCD", "%s(%d, %d)\n", __func__, vram_start, vram_end);

    for (id = 0; id < DDP_OVL_LAYER_MUN; id++) {
        if (cached_layer_config[id].layer_en == 0)
            continue;

        if (cached_layer_config[id].addr >= vram_start &&
            cached_layer_config[id].addr < vram_end)
        {
            DISP_LOG_PRINT(ANDROID_LOG_INFO, "LCD", "  not disable(%d)\n", id);
            layer_enable |= (1 << id);
            continue;
        }

        DISP_LOG_PRINT(ANDROID_LOG_INFO, "LCD", "  disable(%d)\n", id);
        cached_layer_config[id].layer_en = 0;
        cached_layer_config[id].isDirty = true;
    }
    return layer_enable;
}

UINT32 LCD_LayerGetAddress(LCD_LAYER_ID id)
{
    return cached_layer_config[id].addr;
}


LCD_STATUS LCD_LayerSetSize(LCD_LAYER_ID id, UINT32 width, UINT32 height)
{   
	//cached_layer_config[id].src_w = width;
	//cached_layer_config[id].src_h = height;
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetPitch(LCD_LAYER_ID id, UINT32 pitch)
{
	cached_layer_config[id].src_pitch = pitch;
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetOffset(LCD_LAYER_ID id, UINT32 x, UINT32 y)
{
	//cached_layer_config[id].dst_x = x;
	//cached_layer_config[id].dst_y = y;
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_LayerSetWindowOffset(LCD_LAYER_ID id, UINT32 x, UINT32 y)
{
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_LayerSetFormat(LCD_LAYER_ID id, LCD_LAYER_FORMAT format)
{
	cached_layer_config[id].fmt = format;
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerEnableByteSwap(LCD_LAYER_ID id, BOOL enable)
{
	//mt6588 OVL driver not yet export this function
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetRotation(LCD_LAYER_ID id, LCD_LAYER_ROTATION rotation)
{
	//mt6588 OVL driver not yet export this function
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetAlphaBlending(LCD_LAYER_ID id, BOOL enable, UINT8 alpha)
{
	cached_layer_config[id].alpha = (unsigned char)alpha;
	cached_layer_config[id].aen = enable;
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetSourceColorKey(LCD_LAYER_ID id, BOOL enable, UINT32 colorKey)
{
	cached_layer_config[id].key = colorKey;
	cached_layer_config[id].keyEn = enable;
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_LayerSetDestColorKey(LCD_LAYER_ID id, BOOL enable, UINT32 colorKey)
{
	//mt6588 OVL driver not yet export this function
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_Layer3D_Enable(LCD_LAYER_ID id, BOOL enable)
{
	//mt6588 OVL driver not yet export this function
	return LCD_STATUS_OK;

}

LCD_STATUS LCD_Layer3D_R1st(LCD_LAYER_ID id, BOOL r_first)
{
    //mt6588 OVL driver not yet export this function
	return LCD_STATUS_OK;

}

LCD_STATUS LCD_Layer3D_landscape(LCD_LAYER_ID id, BOOL landscape)
{
	//mt6588 OVL driver not yet export this function
	return LCD_STATUS_OK;

}


LCD_STATUS LCD_LayerSet3D(LCD_LAYER_ID id, BOOL enable, BOOL r_first, BOOL landscape)
{
	//mt6588 OVL driver not yet export this function
    return LCD_STATUS_OK;
}


BOOL LCD_Is3DEnabled(void)
{
	//mt6588 OVL driver not yet export this function
	return FALSE;
}


BOOL LCD_Is3DLandscapeMode(void)
{
	//mt6588 OVL driver not yet export this function
	return FALSE;
}


// -------------------- HW Trigger Configurations --------------------

LCD_STATUS LCD_LayerSetTriggerMode(LCD_LAYER_ID id, LCD_LAYER_TRIGGER_MODE mode)
{
	//mt6583 not support
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_EnableHwTrigger(BOOL enable)
{
	//mt6583 not support
    return LCD_STATUS_OK;
}


// -------------------- ROI Window Configurations --------------------

LCD_STATUS LCD_SetBackgroundColor(UINT32 bgColor)
{
	//mt6583 not support
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_SetRoiWindow(UINT32 x, UINT32 y, UINT32 width, UINT32 height)
{
    LCD_REG_SIZE size;
    
    size.WIDTH = (UINT16)width;
    size.HEIGHT = (UINT16)height;

//    _WaitForEngineNotBusy();
    LCD_OUTREG32(&LCD_REG->WROI_SIZE, AS_UINT32(&size));

    _lcdContext.roiWndSize = size;
        
    return LCD_STATUS_OK;
}

// DSI Related Configurations
LCD_STATUS LCD_EnableDCtoDsi(BOOL enable)
{
	//mt6583 not support
    return LCD_STATUS_OK;
}

// -------------------- Output to Memory Configurations --------------------

LCD_STATUS LCD_SetOutputMode(LCD_OUTPUT_MODE mode)
{
	//mt6583 not support, call OVL
    return LCD_STATUS_OK;    
}
EXPORT_SYMBOL(LCD_SetOutputMode);

LCD_STATUS LCD_SetOutputAlpha(unsigned int alpha)
{
	//mt6583 not support
    return LCD_STATUS_OK;    
}

LCD_STATUS LCD_WaitDPIIndication(BOOL enable)
{
    return LCD_STATUS_OK;
}
EXPORT_SYMBOL(LCD_WaitDPIIndication);


LCD_STATUS LCD_FBSetFormat(LCD_FB_FORMAT format)
{
    return LCD_STATUS_OK;
}
EXPORT_SYMBOL(LCD_FBSetFormat);

LCD_STATUS LCD_FBSetPitch(UINT32 pitchInByte)
{
    return LCD_STATUS_OK;
}
EXPORT_SYMBOL(LCD_FBSetPitch);

LCD_STATUS LCD_FBEnable(LCD_FB_ID id, BOOL enable)
{
    return LCD_STATUS_OK;
}
EXPORT_SYMBOL(LCD_FBEnable);

LCD_STATUS LCD_FBReset(void)
{
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_FBSetAddress(LCD_FB_ID id, UINT32 address)
{
    return LCD_STATUS_OK;
}
EXPORT_SYMBOL(LCD_FBSetAddress);

LCD_STATUS LCD_FBSetStartCoord(UINT32 x, UINT32 y)
{
    return LCD_STATUS_OK;
}
EXPORT_SYMBOL(LCD_FBSetStartCoord);
// -------------------- Color Matrix --------------------

LCD_STATUS LCD_EnableColorMatrix(LCD_IF_ID id, BOOL enable)
{
    return LCD_STATUS_OK;
}

/** Input: const S2_8 mat[9], fixed ponit signed 2.8 format
           |                      |
           | mat[0] mat[1] mat[2] |
           | mat[3] mat[4] mat[5] |
           | mat[6] mat[7] mat[8] |
           |                      |
*/
LCD_STATUS LCD_SetColorMatrix(const S2_8 mat[9])
{
    return LCD_STATUS_OK;
}

// -------------------- Tearing Control --------------------

LCD_STATUS LCD_TE_Enable(BOOL enable)
{
    LCD_REG_TECON tecon = LCD_REG->TEARING_CFG;
    tecon.ENABLE = enable ? 1 : 0;
    LCD_OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&tecon));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_TE_SetMode(LCD_TE_MODE mode)
{
    LCD_REG_TECON tecon = LCD_REG->TEARING_CFG;
    tecon.MODE = (LCD_TE_MODE_VSYNC_OR_HSYNC == mode) ? 1 : 0;
    LCD_OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&tecon));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_TE_SetEdgePolarity(BOOL polarity)
{
    LCD_REG_TECON tecon = LCD_REG->TEARING_CFG;
    tecon.EDGE_SEL = (polarity ? 1 : 0);
    LCD_OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&tecon));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_TE_ConfigVHSyncMode(UINT32 hsDelayCnt,
                                   UINT32 vsWidthCnt,
                                   LCD_TE_VS_WIDTH_CNT_DIV vsWidthCntDiv)
{
/*    LCD_REG_TECON tecon = LCD_REG->TEARING_CFG;
    tecon.HS_MCH_CNT = (hsDelayCnt ? hsDelayCnt - 1 : 0);
    tecon.VS_WLMT = (vsWidthCnt ? vsWidthCnt - 1 : 0);
    tecon.VS_CNT_DIV = vsWidthCntDiv;
    LCD_OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&tecon));
*/
    return LCD_STATUS_OK;
}

// -------------------- Operations --------------------

LCD_STATUS LCD_SelectWriteIF(LCD_IF_ID id)
{
    LCD_REG_CMD_ADDR cmd_addr;
	LCD_REG_DAT_ADDR dat_addr;

    switch(id)
    {
    case LCD_IF_PARALLEL_0 : cmd_addr.addr = 0; break;
    case LCD_IF_PARALLEL_1 : cmd_addr.addr = 2; break;
    case LCD_IF_PARALLEL_2 : cmd_addr.addr = 4; break;
    case LCD_IF_SERIAL_0   : cmd_addr.addr = 8; break;
    case LCD_IF_SERIAL_1   : cmd_addr.addr = 0xA; break;
    default:
        ASSERT(0);
    }
	dat_addr.addr = cmd_addr.addr + 1;
    LCD_OUTREG16(&LCD_REG->WROI_CMD_ADDR, AS_UINT16(&cmd_addr));
    LCD_OUTREG16(&LCD_REG->WROI_DATA_ADDR, AS_UINT16(&dat_addr));

    return LCD_STATUS_OK;
}


__inline static void _LCD_WriteIF(DWORD baseAddr, UINT32 value, LCD_IF_MCU_WRITE_BITS bits)
{
    switch(bits)
    {
    case LCD_IF_MCU_WRITE_8BIT :
        LCD_OUTREG8(baseAddr, value);
        break;
        
    case LCD_IF_MCU_WRITE_16BIT :
        LCD_OUTREG16(baseAddr, value);
        break;
        
    case LCD_IF_MCU_WRITE_32BIT :
        LCD_OUTREG32(baseAddr, value);
        break;

    default:
        ASSERT(0);
    }
}


LCD_STATUS LCD_WriteIF(LCD_IF_ID id, LCD_IF_A0_MODE a0,
                       UINT32 value, LCD_IF_MCU_WRITE_BITS bits)
{
    DWORD baseAddr = 0;

    switch(id)
    {
    case LCD_IF_PARALLEL_0 : baseAddr = (DWORD)&LCD_REG->PCMD0; break;
    case LCD_IF_PARALLEL_1 : baseAddr = (DWORD)&LCD_REG->PCMD1; break;
    case LCD_IF_PARALLEL_2 : baseAddr = (DWORD)&LCD_REG->PCMD2; break;
    case LCD_IF_SERIAL_0   : baseAddr = (DWORD)&LCD_REG->SCMD0; break;
    case LCD_IF_SERIAL_1   : baseAddr = (DWORD)&LCD_REG->SCMD1; break;
    default:
        ASSERT(0);
    }

    if (LCD_IF_A0_HIGH == a0)
    {
        baseAddr += LCD_A0_HIGH_OFFSET;
    }

    _LCD_WriteIF(baseAddr, value, bits);

    return LCD_STATUS_OK;
}


__inline static UINT32 _LCD_ReadIF(DWORD baseAddr, LCD_IF_MCU_WRITE_BITS bits)
{
    switch(bits)
    {
    case LCD_IF_MCU_WRITE_8BIT :
        return (UINT32)INREG8(baseAddr);
        
    case LCD_IF_MCU_WRITE_16BIT :
        return (UINT32)INREG16(baseAddr);
        
    case LCD_IF_MCU_WRITE_32BIT :
        return (UINT32)INREG32(baseAddr);

    default:
        ASSERT(0);
    }
}


LCD_STATUS LCD_ReadIF(LCD_IF_ID id, LCD_IF_A0_MODE a0,
                      UINT32 *value, LCD_IF_MCU_WRITE_BITS bits)
{
    DWORD baseAddr = 0;

    if (NULL == value) return LCD_STATUS_ERROR;

    switch(id)
    {
    case LCD_IF_PARALLEL_0 : baseAddr = (DWORD)&LCD_REG->PCMD0; break;
    case LCD_IF_PARALLEL_1 : baseAddr = (DWORD)&LCD_REG->PCMD1; break;
    case LCD_IF_PARALLEL_2 : baseAddr = (DWORD)&LCD_REG->PCMD2; break;
    case LCD_IF_SERIAL_0   : baseAddr = (DWORD)&LCD_REG->SCMD0; break;
    case LCD_IF_SERIAL_1   : baseAddr = (DWORD)&LCD_REG->SCMD1; break;
    default:
        ASSERT(0);
    }

    if (LCD_IF_A0_HIGH == a0)
    {
        baseAddr += LCD_A0_HIGH_OFFSET;
    }

    *value = _LCD_ReadIF(baseAddr, bits);

    return LCD_STATUS_OK;
}

bool LCD_IsLayerEnable(LCD_LAYER_ID id)
{
	ASSERT(id <= LCD_LAYER_NUM);
    return (bool)(cached_layer_config[id].layer_en);
}

extern unsigned int isAEEEnabled;

LCD_STATUS LCD_Dump_Layer_Info()
{
    int i=0;

    printk("LCD_Dump_Layer_Info: \n");
    for(i=0;i<4;i++)
    {
        printk("layer=%d, en=%d, src=%d, fmt=%d, addr=0x%x, (%d, %d, %d, %d), pitch=%d, keyEn=%d, key=0x%x, aen=%d, alpha=0x%x, isTdshp=%d, curr_buff_idx=%d, next_buff_idx=%d\n", 
            cached_layer_config[i].layer,
            cached_layer_config[i].layer_en,
            cached_layer_config[i].source,
            cached_layer_config[i].fmt,
            cached_layer_config[i].addr, 
            cached_layer_config[i].dst_x,
            cached_layer_config[i].dst_y,
            cached_layer_config[i].dst_w,
            cached_layer_config[i].dst_h,
            cached_layer_config[i].src_pitch,
            cached_layer_config[i].keyEn,
            cached_layer_config[i].key, 
            cached_layer_config[i].aen, 
            cached_layer_config[i].alpha,  
            cached_layer_config[i].isTdshp,
            realtime_layer_config[i].buff_idx,
            cached_layer_config[i].buff_idx );
    }
    
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_Dynamic_Change_FB_Layer(unsigned int isAEEEnabled)
{
    static int ui_layer_tdshp = 0;

    printk("[DDP] LCD_Dynamic_Change_FB_Layer, isAEEEnabled=%d \n", isAEEEnabled);
    LCD_Dump_Layer_Info();
    
    if(DISP_DEFAULT_UI_LAYER_ID==DISP_CHANGED_UI_LAYER_ID)
    {
    	 printk("[DDP] LCD_Dynamic_Change_FB_Layer, no dynamic switch \n");
    	 return LCD_STATUS_OK;
    }
    
    if(isAEEEnabled==1)
    {
        // change ui layer from DISP_DEFAULT_UI_LAYER_ID to DISP_CHANGED_UI_LAYER_ID
        memcpy((void*)(&cached_layer_config[DISP_CHANGED_UI_LAYER_ID]), 
               (void*)(&cached_layer_config[DISP_DEFAULT_UI_LAYER_ID]), 
               sizeof(OVL_CONFIG_STRUCT));
        ui_layer_tdshp = cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].isTdshp;
        cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].isTdshp = 0;
        disp_path_change_tdshp_status(DISP_DEFAULT_UI_LAYER_ID, 0); // change global variable value, else error-check will find layer 2, 3 enable tdshp together
        FB_LAYER = DISP_CHANGED_UI_LAYER_ID;
    }
    else
    {
        memcpy((void*)(&cached_layer_config[DISP_DEFAULT_UI_LAYER_ID]), 
               (void*)(&cached_layer_config[DISP_CHANGED_UI_LAYER_ID]), 
               sizeof(OVL_CONFIG_STRUCT));
        cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].isTdshp = ui_layer_tdshp;
        FB_LAYER = DISP_DEFAULT_UI_LAYER_ID;
        memset((void*)(&cached_layer_config[DISP_CHANGED_UI_LAYER_ID]), 0, sizeof(OVL_CONFIG_STRUCT));
    }

    // no matter memcpy or memset, layer ID should not be changed
    cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].layer = DISP_DEFAULT_UI_LAYER_ID;
    cached_layer_config[DISP_CHANGED_UI_LAYER_ID].layer = DISP_CHANGED_UI_LAYER_ID;
    cached_layer_config[DISP_DEFAULT_UI_LAYER_ID].isDirty = 1;
    cached_layer_config[DISP_CHANGED_UI_LAYER_ID].isDirty = 1;

    printk("after dynamic change: \n");
    LCD_Dump_Layer_Info();
    
    return LCD_STATUS_OK;
}

extern struct mutex OverlaySettingMutex;
LCD_STATUS LCD_StartTransfer(BOOL blocking, BOOL isMutexLocked)
{
    DBI_FUNC();
    _WaitForEngineNotBusy();
    DBG_OnTriggerLcd();
    if (!isMutexLocked)
        disp_path_get_mutex();
    mutex_lock(&OverlaySettingMutex);
    // isMutexLocked=false means it is the first time of DBI update after init or late resume.
    // Layer config was not applied in config update thread.
    // So need to config overlay here.
    if (!isMutexLocked)
    LCD_ConfigOVL();

//	_WaitForEngineNotBusy();
    LCD_OUTREG32(&LCD_REG->START, 0);
    LCD_OUTREG32(&LCD_REG->START, (1 << 15));
//	_WaitForEngineNotBusy();
    mutex_unlock(&OverlaySettingMutex);
    if (!isMutexLocked)
        disp_path_release_mutex();
    if (blocking)
    {
        _WaitForEngineNotBusy();
    }
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_ConfigOVL()
{
    unsigned int i;
    unsigned int dirty = 0;

    DBI_FUNC();
    MMProfileLogEx(MTKFB_MMP_Events.ConfigOVL, MMProfileFlagStart, 1, 0);
	for(i = 0;i<DDP_OVL_LAYER_MUN;i++)
    {
        if (cached_layer_config[i].isDirty)
        {
            dirty |= 1<<i;
            disp_path_config_layer(&cached_layer_config[i]);
            cached_layer_config[i].isDirty = false;
        }
    }
    MMProfileLogEx(MTKFB_MMP_Events.ConfigOVL, MMProfileFlagEnd, 1, 0);
    return LCD_STATUS_OK;
}

// -------------------- Retrieve Information --------------------

LCD_OUTPUT_MODE LCD_GetOutputMode(void)
{
	return _lcdContext.outputMode;
}
LCD_STATE  LCD_GetState(void)
{
    if (!s_isLcdPowerOn)
    {
        return LCD_STATE_POWER_OFF;
    }

    if (_IsEngineBusy())
    {
        return LCD_STATE_BUSY;
    }

    return LCD_STATE_IDLE;
}


LCD_STATUS LCD_DumpRegisters(void)
{
    UINT32 i;

    DISP_LOG_PRINT(ANDROID_LOG_WARN, "LCD", "---------- Start dump LCD registers ----------\n");
    
    for (i = 0; i < offsetof(LCD_REGS, GMC_ULTRA_TH); i += 4)
    {
        printk("LCD+%04x : 0x%08x\n", i, INREG32(LCD_BASE + i));
    }
	
    return LCD_STATUS_OK;
}

void LCD_DumpLayer()
{
	unsigned int i;
	for(i=0;i<4;i++){
		printk("LayerInfo in LCD driver, layer=%d,layer_en=%d, source=%d, fmt=%d, addr=0x%x, x=%d, y=%d \n\
		w=%d, h=%d, pitch=%d, keyEn=%d, key=%d, aen=%d, alpha=%d \n ", 
	    cached_layer_config[i].layer,   // layer
	    cached_layer_config[i].layer_en,
	    cached_layer_config[i].source,   // data source (0=memory)
	    cached_layer_config[i].fmt, 
	    cached_layer_config[i].addr, // addr 
	    cached_layer_config[i].dst_x,  // x
	    cached_layer_config[i].dst_y,  // y
	    cached_layer_config[i].dst_w, // width
	    cached_layer_config[i].dst_h, // height
	    cached_layer_config[i].src_pitch, //pitch, pixel number
	    cached_layer_config[i].keyEn,  //color key
	    cached_layer_config[i].key,  //color key
	    cached_layer_config[i].aen, // alpha enable
	    cached_layer_config[i].alpha);	
	}
}


#if !defined(MTK_M4U_SUPPORT)
#ifdef BUILD_UBOOT
static unsigned long v2p(unsigned long va)
{
    return va;
}
#else
static unsigned long v2p(unsigned long va)
{
    unsigned long pageOffset = (va & (PAGE_SIZE - 1));
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *pte;
    unsigned long pa;

    pgd = pgd_offset(current->mm, va); /* what is tsk->mm */
    pmd = pmd_offset(pgd, va);
    pte = pte_offset_map(pmd, va);
    
    pa = (pte_val(*pte) & (PAGE_MASK)) | pageOffset;


    return pa;
}
#endif
#endif
LCD_STATUS LCD_Get_VideoLayerSize(unsigned int id, unsigned int *width, unsigned int *height)
{
#if 0
    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);

    _WaitForEngineNotBusy();

    if (LCD_LAYER_ALL == id)
    {
        NOT_IMPLEMENTED();
    }
    else if (id < LCD_LAYER_NUM)
    {
        *width = LCD_REG->LAYER[id].SIZE.WIDTH;
        *height = LCD_REG->LAYER[id].SIZE.HEIGHT;
    }
#endif
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_Capture_Layerbuffer(unsigned int layer_id, unsigned int pvbuf, unsigned int bpp)
{
#if 0
    unsigned int ppbuf = 0;
    UINT16 offset_x, offset_y, w, h;
    LCD_OUTPUT_MODE mode;
    if(pvbuf == 0 || bpp == 0)
    {
        DBI_LOG("LCD_Capture_Layerbuffer, ERROR, parameters wrong: pvbuf=0x%08x, bpp=%d\n", pvbuf, bpp);
        return LCD_STATUS_ERROR;
    }

    if(layer_id >= LCD_LAYER_NUM)
    {
        DBI_LOG("LCD_Capture_Layerbuffer, ERROR, parameters wrong: layer_id=%d\n", layer_id);
        return LCD_STATUS_ERROR;
    }

    if(!LCD_IsLayerEnable(layer_id))
    {
        DBI_LOG("LCD_Capture_Layerbuffer, ERROR, the layer%d is not enabled\n", layer_id);
        return LCD_STATUS_ERROR;
    }

    LCD_WaitForNotBusy();

    _BackupLCDRegisters();

    // begin capture
    if(bpp == 32)
        LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_ARGB8888));
    else if(bpp == 16)
        LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_RGB565));
    else if(bpp == 24)
        LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_RGB888));
    else
        DBI_LOG("mtkfb_capture_Videobuffer, fb color format not support\n");

	offset_x = LCD_REG->LAYER[layer_id].OFFSET.X;
	offset_y = LCD_REG->LAYER[layer_id].OFFSET.Y;
	w = LCD_REG->LAYER[layer_id].SIZE.WIDTH;
	h = LCD_REG->LAYER[layer_id].SIZE.HEIGHT;
#if defined(MTK_M4U_SUPPORT)
	if (!_m4u_lcdc_func.isInit)
	{
		DBI_LOG("[LCDC init M4U] M4U not been init for LCDC\n");
		return LCD_STATUS_ERROR;
	}
	
	_m4u_lcdc_func.m4u_alloc_mva(M4U_CLNTMOD_LCDC, pvbuf, w*h*bpp/8, &ppbuf);
#else
    ppbuf = v2p(pvbuf);
#endif
	ASSERT(ppbuf != 0);

	mode = LCD_GetOutputMode();
	
	LCD_CHECK_RET(LCD_LayerEnable(LCD_LAYER_ALL, 0));
	LCD_CHECK_RET(LCD_LayerEnable(layer_id, 1));
	LCD_CHECK_RET(LCD_LayerSetRotation(layer_id, LCD_LAYER_ROTATE_0));
	LCD_CHECK_RET(LCD_SetRoiWindow(offset_x, offset_y, w, h));
	LCD_CHECK_RET(LCD_FBSetPitch((LCD_REG->LAYER[layer_id].SIZE.WIDTH)*bpp/8));

	LCD_CHECK_RET(LCD_FBSetStartCoord(0, 0));
	LCD_CHECK_RET(LCD_FBReset());
    
	LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0, ppbuf));
	LCD_CHECK_RET(LCD_FBEnable(LCD_FB_0, TRUE));

    LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_MEM));
    LCD_TE_Enable(FALSE);

	LCD_SetOutputAlpha(0xff);

//	LCD_CHECK_RET(LCD_SetRoiWindow(offset_x, offset_y, h, w));

	LCD_MASKREG32(0xf20a10a0, 0x1, 0); //disable DC to DSI when write to memory partially
#if defined(MTK_M4U_SUPPORT)
	_m4u_lcdc_func.m4u_dma_cache_maint(M4U_CLNTMOD_LCDC, (void *)pvbuf, w*h*bpp/8, DMA_BIDIRECTIONAL);
#endif

    LCD_CHECK_RET(LCD_StartTransfer(TRUE));
 
	LCD_SetOutputMode(mode);
    // capture end
    _RestoreLCDRegisters();
#if defined(MTK_M4U_SUPPORT)
    _m4u_lcdc_func.m4u_dealloc_mva(M4U_CLNTMOD_LCDC, pvbuf, w*h*bpp/8, ppbuf);
#endif
#endif
    return LCD_STATUS_OK;   
}


LCD_STATUS LCD_Capture_Videobuffer(unsigned int pvbuf, unsigned int bpp, unsigned int video_rotation)
{
#if 0
    unsigned int ppbuf = 0;
    UINT16 offset_x, offset_y, w, h;
    LCD_OUTPUT_MODE mode;
    if(pvbuf == 0 || bpp == 0)
    {
        DBI_LOG("LCD_Capture_Videobuffer, ERROR, parameters wrong: pvbuf=0x%08x, bpp=%d\n", pvbuf, bpp);
        return LCD_STATUS_OK;
    }
    
    LCD_WaitForNotBusy();

    _BackupLCDRegisters();

    // begin capture
    if(bpp == 32)
        LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_ARGB8888));
    else if(bpp == 16)
        LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_RGB565));
    else if(bpp == 24)
        LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_RGB888));
    else
        DBI_LOG("mtkfb_capture_Videobuffer, fb color format not support\n");

	offset_x = LCD_REG->LAYER[LCD_LAYER_2].OFFSET.X;
	offset_y = LCD_REG->LAYER[LCD_LAYER_2].OFFSET.Y;
	w = LCD_REG->LAYER[LCD_LAYER_2].SIZE.WIDTH;
	h = LCD_REG->LAYER[LCD_LAYER_2].SIZE.HEIGHT;
#if defined(MTK_M4U_SUPPORT)
	if (!_m4u_lcdc_func.isInit)
	{
		DBI_LOG("[LCDC init M4U] M4U not been init for LCDC\n");
		return LCD_STATUS_ERROR;
	}
	
	_m4u_lcdc_func.m4u_alloc_mva(M4U_CLNTMOD_LCDC, pvbuf, w*h*bpp/8, &ppbuf);
#else
    ppbuf = v2p(pvbuf);
#endif
	mode = LCD_GetOutputMode();
	
	LCD_CHECK_RET(LCD_LayerEnable(LCD_LAYER_ALL, 0));
	LCD_CHECK_RET(LCD_LayerEnable(LCD_LAYER_2, 1));
	LCD_CHECK_RET(LCD_LayerSetRotation(LCD_LAYER_2, LCD_LAYER_ROTATE_0));
	LCD_CHECK_RET(LCD_SetRoiWindow(offset_x, offset_y, w, h));
	LCD_CHECK_RET(LCD_FBSetPitch((LCD_REG->LAYER[LCD_LAYER_2].SIZE.WIDTH)*bpp/8));

	switch(video_rotation)
	{
		case 0:break;
		case 1:
			LCD_CHECK_RET(LCD_LayerSetRotation(LCD_LAYER_2, LCD_LAYER_ROTATE_90));
			LCD_CHECK_RET(LCD_SetRoiWindow(offset_x, offset_y, h, w));
			LCD_CHECK_RET(LCD_FBSetPitch((LCD_REG->LAYER[LCD_LAYER_2].SIZE.HEIGHT)*bpp/8));
			break;
		case 2:
			LCD_CHECK_RET(LCD_LayerSetRotation(LCD_LAYER_2, LCD_LAYER_ROTATE_180));break;
		case 3:
			LCD_CHECK_RET(LCD_LayerSetRotation(LCD_LAYER_2, LCD_LAYER_ROTATE_270));
			LCD_CHECK_RET(LCD_SetRoiWindow(offset_x, offset_y, h, w));
			LCD_CHECK_RET(LCD_FBSetPitch((LCD_REG->LAYER[LCD_LAYER_2].SIZE.HEIGHT)*bpp/8));
			break;
		default:
		    DBI_LOG("[LCD_Capture_videobuffer] video_rotation = %d error\n", video_rotation);
	}

	LCD_CHECK_RET(LCD_FBSetStartCoord(0, 0));
	LCD_CHECK_RET(LCD_FBReset());
    
	LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0, ppbuf));
	LCD_CHECK_RET(LCD_FBEnable(LCD_FB_0, TRUE));

    LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_MEM));
    LCD_TE_Enable(FALSE);

	LCD_SetOutputAlpha(0xff);

//	LCD_CHECK_RET(LCD_SetRoiWindow(offset_x, offset_y, h, w));

	LCD_MASKREG32(0xf20a10a0, 0x1, 0); //disable DC to DSI when write to memory partially
#if defined(MTK_M4U_SUPPORT)
	_m4u_lcdc_func.m4u_dma_cache_maint(M4U_CLNTMOD_LCDC, (void *)pvbuf, w*h*bpp/8, DMA_BIDIRECTIONAL);
#endif

	if(dbi_log_on)
		LCD_DumpRegisters();

    LCD_CHECK_RET(LCD_StartTransfer(TRUE));
 
	LCD_SetOutputMode(mode);
    // capture end
    _RestoreLCDRegisters();
#if defined(MTK_M4U_SUPPORT)
    _m4u_lcdc_func.m4u_dealloc_mva(M4U_CLNTMOD_LCDC, pvbuf, w*h*bpp/8, ppbuf);
#endif
#endif
    return LCD_STATUS_OK;   
}

#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

LCD_STATUS LCD_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp)
{
#if 0
    unsigned int ppbuf = 0;
    LCD_OUTPUT_MODE mode;
	UINT16 offset_x, offset_y, w, h, i;
    if(pvbuf == 0 || bpp == 0)
    {
        DBI_LOG("LCD_Capture_Framebuffer, ERROR, parameters wrong: pvbuf=0x%08x, bpp=%d\n", pvbuf, bpp);
        return LCD_STATUS_OK;
    }

#if defined(MTK_M4U_SUPPORT)
	if (!_m4u_lcdc_func.isInit)
	{
        unsigned int t;
		unsigned int i,j;
		unsigned int w_xres = DISP_GetScreenWidth();
	    unsigned int w = ALIGN_TO(DISP_GetScreenWidth(), 32);
	    unsigned int h = DISP_GetScreenHeight();
		unsigned int pixel_bpp = bpp/8;
    	unsigned int fbsize = w*h*pixel_bpp;
		unsigned int fbv = (unsigned int)ioremap_cached((unsigned int)LCD_REG->LAYER[FB_LAYER].ADDRESS, fbsize);
		unsigned short* fbvt = (unsigned short*)fbv;

		DBI_LOG("[LCDC init M4U] M4U not been init for LCDC, this should only happened in recovery mode\n");
		for(i = 0;i < h; i++){
			for(j = 0;j < w_xres; j++){
	    		t = fbvt[j + i * w];
 	         	*(unsigned short*)(pvbuf+i*w_xres*pixel_bpp+j*pixel_bpp) = t;
    		}
		}
		iounmap((void *)fbv);
		return LCD_STATUS_OK;
	}
	else
		_m4u_lcdc_func.m4u_alloc_mva(M4U_CLNTMOD_LCDC, pvbuf, DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8, &ppbuf);
#else
    ppbuf = v2p(pvbuf);
#endif

    LCD_WaitForNotBusy();

    _BackupLCDRegisters();

    // begin capture
    if(bpp == 32)
        LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_ARGB8888));
    else if(bpp == 16)
        LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_RGB565));
    else if(bpp == 24)
        LCD_CHECK_RET(LCD_FBSetFormat(LCD_FB_FORMAT_RGB888));
    else
        DBI_LOG("mtkfb_capture_framebuffer, fb color format not support\n");

	mode = LCD_GetOutputMode();
    LCD_CHECK_RET(LCD_LayerSetRotation(LCD_LAYER_ALL, LCD_LAYER_ROTATE_0));
	
	//zaikuo wang, add to capture framebuffer for LCM PHYSICAL rotate 180
	if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3)){
		for(i = LCD_LAYER_0;i < LCD_LAYER_NUM;i++){
			offset_x = LCD_REG->LAYER[i].OFFSET.X;
			offset_y = LCD_REG->LAYER[i].OFFSET.Y;
			w = LCD_REG->LAYER[i].SIZE.WIDTH;
			h = LCD_REG->LAYER[i].SIZE.HEIGHT;
			offset_x = DISP_GetScreenWidth() - (offset_x + w);
			offset_y = DISP_GetScreenHeight() - (offset_y + h);
			LCD_CHECK_RET(LCD_LayerSetOffset(i, offset_x, offset_y));
		}
	}
    LCD_CHECK_RET(LCD_FBSetPitch(DISP_GetScreenWidth()*bpp/8));
    LCD_CHECK_RET(LCD_FBSetStartCoord(0, 0));
    LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0, ppbuf));
    LCD_CHECK_RET(LCD_FBEnable(LCD_FB_0, TRUE));
    
    LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_MEM));

    LCD_TE_Enable(FALSE);
	LCD_SetOutputAlpha(0xff);
#if defined(MTK_M4U_SUPPORT)
	_m4u_lcdc_func.m4u_dma_cache_maint(M4U_CLNTMOD_LCDC, (unsigned int*)pvbuf, DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8, DMA_BIDIRECTIONAL);
#endif
	if(dbi_log_on)
		LCD_DumpRegisters();

    LCD_CHECK_RET(LCD_StartTransfer(TRUE));
	LCD_SetOutputMode(mode);
    // capture end
    _RestoreLCDRegisters();
#if defined(MTK_M4U_SUPPORT)
    _m4u_lcdc_func.m4u_dealloc_mva(M4U_CLNTMOD_LCDC, pvbuf, DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8, ppbuf);
#endif
#else
    unsigned int mva;
    unsigned int ret = 0;
	M4U_PORT_STRUCT portStruct;

	struct disp_path_config_mem_out_struct mem_out = {0};
	printk("enter LCD_Capture_FB!\n");
	if(bpp == 32)
		mem_out.outFormat = WDMA_OUTPUT_FORMAT_ARGB;
	else if(bpp == 16)
		mem_out.outFormat = WDMA_OUTPUT_FORMAT_RGB565;
	else if(bpp == 24)
		mem_out.outFormat = WDMA_OUTPUT_FORMAT_RGB888;
	else
		DBI_LOG("LCD_Capture_FB, fb color format not support\n");
	
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
		return LCD_STATUS_OK;  
	}
	printk("addr=0x%x, format=%d \n", mva, mem_out.outFormat);

	m4u_dma_cache_maint(M4U_CLNTMOD_WDMA, 
		                (void*)pvbuf, 
		                DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8,
		                DMA_BIDIRECTIONAL);
	
	portStruct.ePortID = M4U_PORT_WDMA1;		   //hardware port ID, defined in M4U_PORT_ID_ENUM
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

	_BackupLCDRegisters();
	LCD_TE_Enable(FALSE);
	
	_WaitForEngineNotBusy();
    disp_path_get_mutex();
	disp_path_config_mem_out(&mem_out);
	printk("Wait DBI idle \n");
//	_WaitForEngineNotBusy();
	LCD_OUTREG32(&LCD_REG->START, 0);
    LCD_OUTREG32(&LCD_REG->START, (1 << 15));

	disp_path_release_mutex();

	msleep(20);
	_RestoreLCDRegisters();
	
	disp_path_get_mutex();
	mem_out.enable = 0;
	disp_path_config_mem_out(&mem_out);
	#if 1
	_WaitForEngineNotBusy();
	LCD_OUTREG32(&LCD_REG->START, 0);
    LCD_OUTREG32(&LCD_REG->START, (1 << 15));
	#endif
	disp_path_release_mutex();

	portStruct.ePortID = M4U_PORT_WDMA1;		   //hardware port ID, defined in M4U_PORT_ID_ENUM
	portStruct.Virtuality = 0;						   
	portStruct.Security = 0;
	portStruct.domain = 0;			  //domain : 0 1 2 3
	portStruct.Distance = 1;
	portStruct.Direction = 0;	
	m4u_config_port(&portStruct);

    m4u_dealloc_mva(M4U_CLNTMOD_WDMA, 
                    pvbuf, 
		                DISP_GetScreenHeight()*DISP_GetScreenWidth()*bpp/8, 
		                mva);
#endif
    return LCD_STATUS_OK;    
}

LCD_STATUS LCD_FMDesense_Query()
{
    return LCD_STATUS_OK;
}

LCD_STATUS LCD_FM_Desense(LCD_IF_ID id, unsigned long freq)
{
    UINT32 a,b;
	UINT32 c,d;
	UINT32 wst,c2wh,chw,write_cycles;
	LCD_REG_PCNF config;
//	LCD_REG_WROI_CON ctrl;    
	LCD_REG_PCNFDW pcnfdw;

	OUTREG32(&config, AS_UINT32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));
    DBI_LOG("[enter LCD_FM_Desense]:parallel IF = 0x%x, ctrl = 0x%x\n",
		INREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));   
	wst = config.WST;
	c2wh = config.C2WH;
	// Config Delay Between Commands
//	OUTREG32(&ctrl, AS_UINT32(&LCD_REG->WROI_CONTROL));
	OUTREG32(&pcnfdw, AS_UINT32(&LCD_REG->PARALLEL_DW));

	switch(id)
    {
        case LCD_IF_PARALLEL_0: chw = pcnfdw.PCNF0_CHW; break;
        case LCD_IF_PARALLEL_1: chw = pcnfdw.PCNF1_CHW; break;
        case LCD_IF_PARALLEL_2: chw = pcnfdw.PCNF2_CHW; break;
        default : ASSERT(0);
    }

	a = 13000 - freq * 10 - 20;
	b = 13000 - freq * 10 + 20;
    write_cycles = wst + c2wh + chw + 2;//this is 6573 E1, E2 will change
	c = (a * write_cycles)%13000;
	d = (b * write_cycles)%13000;
	a = (a * write_cycles)/13000;
	b = (b * write_cycles)/13000;
	
	if((b > a)||(c == 0)||(d == 0)){//need modify setting to avoid interference
	    DBI_LOG("[LCD_FM_Desense] need to modify lcd setting, freq = %ld\n",freq);
	    wst -= wst_step_LCD;
		wst_step_LCD = 0 - wst_step_LCD;

		config.WST = wst;
		LCD_WaitForNotBusy();
		OUTREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id], AS_UINT32(&config));
	}
	else{
		DBI_LOG("[LCD_FM_Desense] not need to modify lcd setting, freq = %ld\n",freq);
	}
	DBI_LOG("[leave LCD_FM_Desense]:parallel = 0x%x\n", INREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));   
    return LCD_STATUS_OK;  
}

LCD_STATUS LCD_Reset_WriteCycle(LCD_IF_ID id)
{
	LCD_REG_PCNF config;
	UINT32 wst;
	DBI_LOG("[enter LCD_Reset_WriteCycle]:parallel = 0x%x\n", INREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));   
	if(wst_step_LCD > 0){//have modify lcd setting, so when fm turn off, we must decrease wst to default setting
	    DBI_LOG("[LCD_Reset_WriteCycle] need to reset lcd setting\n");
	    OUTREG32(&config, AS_UINT32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));
		wst = config.WST;
		wst -= wst_step_LCD;
		wst_step_LCD = 0 - wst_step_LCD;

		config.WST = wst;
		LCD_WaitForNotBusy();
		OUTREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id], AS_UINT32(&config));
	}
	else{
		DBI_LOG("[LCD_Reset_WriteCycle] parallel is default setting, not need to reset it\n");
	}
	DBI_LOG("[leave LCD_Reset_WriteCycle]:parallel = 0x%x\n", INREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));   
	return LCD_STATUS_OK; 
}

LCD_STATUS LCD_Get_Default_WriteCycle(LCD_IF_ID id, unsigned int *write_cycle)
{
	UINT32 wst,c2wh,chw;
	LCD_REG_PCNF config;
//	LCD_REG_WROI_CON ctrl;    
	LCD_REG_PCNFDW pcnfdw;
    
	if(is_get_default_write_cycle){
	    *write_cycle = default_write_cycle;
	    return LCD_STATUS_OK;
	}

	OUTREG32(&config, AS_UINT32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));
    DBI_LOG("[enter LCD_Get_Default_WriteCycle]:parallel IF = 0x%x, ctrl = 0x%x\n",
		INREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));   
	wst = config.WST;
	c2wh = config.C2WH;
	// Config Delay Between Commands
//	OUTREG32(&ctrl, AS_UINT32(&LCD_REG->WROI_CONTROL));
	OUTREG32(&pcnfdw, AS_UINT32(&LCD_REG->PARALLEL_DW));
	switch(id)
    {
        case LCD_IF_PARALLEL_0: chw = pcnfdw.PCNF0_CHW; break;
        case LCD_IF_PARALLEL_1: chw = pcnfdw.PCNF1_CHW; break;
        case LCD_IF_PARALLEL_2: chw = pcnfdw.PCNF2_CHW; break;
        default : ASSERT(0);
    }
    *write_cycle = wst + c2wh + chw + 2;
	default_write_cycle = *write_cycle;
	default_wst = wst;
	is_get_default_write_cycle = TRUE;
	DBI_LOG("[leave LCD_Get_Default_WriteCycle]:Default_Write_Cycle = %d\n", *write_cycle);   
    return LCD_STATUS_OK;  
}

LCD_STATUS LCD_Get_Current_WriteCycle(LCD_IF_ID id, unsigned int *write_cycle)
{
	UINT32 wst,c2wh,chw;
	LCD_REG_PCNF config;
//	LCD_REG_WROI_CON ctrl;       
    LCD_REG_PCNFDW pcnfdw;

	OUTREG32(&config, AS_UINT32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));
    DBI_LOG("[enter LCD_Get_Current_WriteCycle]:parallel IF = 0x%x, ctrl = 0x%x\n",
		INREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));   
	wst = config.WST;
	c2wh = config.C2WH;
	// Config Delay Between Commands
//	OUTREG32(&ctrl, AS_UINT32(&LCD_REG->WROI_CONTROL));
	OUTREG32(&pcnfdw, AS_UINT32(&LCD_REG->PARALLEL_DW));
	switch(id)
    {
        case LCD_IF_PARALLEL_0: chw = pcnfdw.PCNF0_CHW; break;
        case LCD_IF_PARALLEL_1: chw = pcnfdw.PCNF1_CHW; break;
        case LCD_IF_PARALLEL_2: chw = pcnfdw.PCNF2_CHW; break;
        default : ASSERT(0);
    }

    *write_cycle = wst + c2wh + chw + 2;//this is 6573 E1, E2 will change
	DBI_LOG("[leave LCD_Get_Current_WriteCycle]:Default_Write_Cycle = %d\n", *write_cycle);   
    return LCD_STATUS_OK;  
}

LCD_STATUS LCD_Change_WriteCycle(LCD_IF_ID id, unsigned int write_cycle)
{
	UINT32 wst;
	LCD_REG_PCNF config;

	OUTREG32(&config, AS_UINT32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));
    DBI_LOG("[enter LCD_Change_WriteCycle]:parallel IF = 0x%x, ctrl = 0x%x\n",
		INREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id]),INREG32(&LCD_REG->WROI_CONTROL));   

    DBI_LOG("[LCD_Change_WriteCycle] modify lcd setting\n");
	wst = write_cycle - default_write_cycle + default_wst;

	config.WST = wst;
	LCD_WaitForNotBusy();
	OUTREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id], AS_UINT32(&config));
	DBI_LOG("[leave LCD_Change_WriteCycle]:parallel = 0x%x\n", INREG32(&LCD_REG->PARALLEL_CFG[(UINT32)id]));   
    return LCD_STATUS_OK;  
}

#if defined(MTK_M4U_SUPPORT)
LCD_STATUS LCD_InitM4U()
{
#ifdef MTK_LCDC_ENABLE_M4U
	M4U_PORT_STRUCT M4uPort;
	DBI_LOG("[LCDC driver]%s\n", __func__);

	M4uPort.ePortID = M4U_PORT_LCD_R;
	M4uPort.Virtuality = 1; 					   
	M4uPort.Security = 0;
	M4uPort.Distance = 1;
	M4uPort.Direction = 0;

	m4u_config_port(&M4uPort);

	M4uPort.ePortID = M4U_PORT_LCD_W;
	M4uPort.Virtuality = 1; 					   
	M4uPort.Security = 0;
	M4uPort.Distance = 1;
	M4uPort.Direction = 0;

	m4u_config_port(&M4uPort);
//    _m4u_lcdc_func.m4u_dump_reg(M4U_CLNTMOD_LCDC);
#endif
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_AllocUIMva(unsigned int va, unsigned int *mva, unsigned int size)
{
#ifdef MTK_LCDC_ENABLE_M4U
    m4u_alloc_mva(M4U_CLNTMOD_LCDC_UI, va, size, mva);
#endif
#if 0
	_m4u_lcdc_func.m4u_insert_tlb_range(M4U_CLNTMOD_LCDC_UI,
                                  		 *mva,
                      					 *mva + size - 1,
                      					 RT_RANGE_HIGH_PRIORITY,
										 0);
#endif
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_AllocOverlayMva(unsigned int va, unsigned int *mva, unsigned int size)
{
#ifdef MTK_LCDC_ENABLE_M4U
    if (!_m4u_lcdc_func.isInit)
	{
		DBI_LOG("[LCDC init M4U] M4U not been init for LCDC\n");
		return LCD_STATUS_ERROR;
	}
	_m4u_lcdc_func.m4u_alloc_mva(M4U_CLNTMOD_LCDC, va, size, mva);
	_m4u_lcdc_func.m4u_insert_tlb_range(M4U_CLNTMOD_LCDC,
                                  		 *mva,
                      					 *mva + size - 1,
                      					 RT_RANGE_HIGH_PRIORITY,
										 0);
#endif
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_DeallocMva(unsigned int va, unsigned int mva, unsigned int size)
{
#ifdef MTK_LCDC_ENABLE_M4U
    if (!_m4u_lcdc_func.isInit)
	{
		DBI_LOG("[TV]Error, M4U has not init func for TV-out\n");
		return LCD_STATUS_ERROR;
	}
    _m4u_lcdc_func.m4u_invalid_tlb_range(M4U_CLNTMOD_LCDC, mva, mva + size - 1);
    _m4u_lcdc_func.m4u_dealloc_mva(M4U_CLNTMOD_LCDC, va, size, mva);
#endif
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_M4UPowerOn(void)
{
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_M4UPowerOff(void)
{
	return LCD_STATUS_OK;
}

int m4u_alloc_mva_stub(M4U_MODULE_ID_ENUM eModuleID, const unsigned int BufAddr, const unsigned int BufSize, unsigned int *pRetMVABuf)
{
	return m4u_alloc_mva(eModuleID, BufAddr, BufSize, 0, 0, pRetMVABuf);
}
EXPORT_SYMBOL(m4u_alloc_mva_stub);
  
int m4u_dealloc_mva_stub(M4U_MODULE_ID_ENUM eModuleID, const unsigned int BufAddr, const unsigned int BufSize, const unsigned int MVA)
{
	return m4u_dealloc_mva(eModuleID, BufAddr, BufSize, MVA);
}
EXPORT_SYMBOL(m4u_dealloc_mva_stub);
                  							
int m4u_insert_tlb_range_stub(M4U_MODULE_ID_ENUM eModuleID, 
                  unsigned int MVAStart, 
                  const unsigned int MVAEnd, 
                  M4U_RANGE_PRIORITY_ENUM ePriority,
                  unsigned int entryCount)
{
	return m4u_insert_seq_range(eModuleID, MVAStart, MVAEnd, ePriority, entryCount);				  
}
EXPORT_SYMBOL(m4u_insert_tlb_range_stub);
                        
int m4u_invalid_tlb_range_stub(M4U_MODULE_ID_ENUM eModuleID, 
                  unsigned int MVAStart, 
                  unsigned int MVAEnd)
{
	return m4u_invalid_seq_range(eModuleID, MVAStart, MVAEnd);				  
}
EXPORT_SYMBOL(m4u_invalid_tlb_range_stub);
             
int m4u_invalid_tlb_all_stub(M4U_MODULE_ID_ENUM eModuleID);  
int m4u_manual_insert_entry_stub(M4U_MODULE_ID_ENUM eModuleID, unsigned int EntryMVA, bool Lock); 
  
int m4u_config_port_stub(M4U_PORT_STRUCT* pM4uPort)
{
	return m4u_config_port(pM4uPort);
}
EXPORT_SYMBOL(m4u_config_port_stub);

LCD_STATUS LCD_M4U_On(bool enable)
{
#ifdef MTK_LCDC_ENABLE_M4U
	M4U_PORT_STRUCT M4uPort;
	DBI_LOG("[LCDC driver]%s\n", __func__);


	M4uPort.ePortID = M4U_PORT_LCD_R;
	M4uPort.Virtuality = enable; 					   
	M4uPort.Security = 0;
	M4uPort.Distance = 1;
	M4uPort.Direction = 0;

	m4u_config_port(&M4uPort);
#endif
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_DumpM4U(){
//	_m4u_lcdc_func.m4u_dump_reg(M4U_CLNTMOD_LCDC);
	m4u_dump_info(M4U_CLNTMOD_LCDC_UI);
	return LCD_STATUS_OK;
}

#endif

LCD_STATUS LCD_W2M_NeedLimiteSpeed(BOOL enable)
{
	limit_w2m_speed = enable;
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_W2TVR_NeedLimiteSpeed(BOOL enable)
{
	limit_w2tvr_speed = enable;
	return LCD_STATUS_OK;
}

LCD_STATUS LCD_SetGMCThrottle()
{
#if 0
    UINT16 i, pitch, h;
	UINT32 total_req = 0;
	INT32 throttle_cnt = 0;

	if(!limit_w2m_speed && !limit_w2tvr_speed){
		LCD_OUTREG32(&LCD_REG->SMI_CON, (throttle_cnt << 16) | (0 << 4) | 0x4);
		return LCD_STATUS_OK;
	}

	for(i=0;i<LCD_LAYER_NUM;i++){
		if(LCD_IsLayerEnable((LCD_LAYER_ID)i)){
	  	    pitch = LCD_REG->LAYER[(LCD_LAYER_ID)i].WINDOW_PITCH;
   		    h = LCD_REG->LAYER[(LCD_LAYER_ID)i].SIZE.HEIGHT;
			total_req += pitch * h;
		}
	}

	if(total_req != 0)
		throttle_cnt = 34666666/total_req - 2;//34666666 is calculated by 130MHz/60fps*16
	
	if(throttle_cnt < 0)
		throttle_cnt = 0;	
	LCD_OUTREG32(&LCD_REG->GMC_CON, (throttle_cnt << 16) | (1 << 4) | 0x4);
#endif	
	return LCD_STATUS_OK;
}

// called by "esd_recovery_kthread"
BOOL LCD_esd_check(void)
{
#ifndef MT65XX_NEW_DISP
	UINT32 x, y, width, height;

	// Enable TE interrupt
	//LCD_TE_SetMode(LCD_TE_MODE_VSYNC_ONLY);
	//LCD_TE_SetEdgePolarity(LCM_POLARITY_RISING);
	LCD_TE_Enable(TRUE);

	// Backup ROI
	LCD_CHECK_RET(LCD_GetRoiWindow(&x, &y, &width, &height));

	// Set ROI = 0
	LCD_CHECK_RET(LCD_SetRoiWindow(0, 0, 0, 0));

	// Switch to unuse port
	LCD_CHECK_RET(LCD_SelectWriteIF(LCD_IF_PARALLEL_2));

	// Write to LCM
	LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_LCM));

	// Blocking Trigger
	// This is to cheat LCDC to wait TE interrupt 
	LCD_CHECK_RET(LCD_StartTransfer(TRUE));

	// Restore ROI
	LCD_CHECK_RET(LCD_SetRoiWindow(x, y, width, height));

	// Disable TE interrupt
	LCD_TE_Enable(FALSE);

	// Write to memory	
	LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_MEM));

	return lcd_esd_check;
#endif	
	return TRUE;
}

