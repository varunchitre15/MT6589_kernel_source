

#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <asm/tcm.h>

#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_reg_base.h>
#include <mach/mt6575_irq.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt6575_sysram.h>
#include <mach/mt6575_m4u.h>

#include "tvc_reg.h"
#include "tvc_drv.h"

#include "mtkfb.h"


#if defined(MTK_TVOUT_SUPPORT)

#define TV_PRINTF             printk
#define TV_INFO(fmt, arg...)       TV_PRINTF("[TV INFO]: %s(): "fmt,__FUNCTION__, ##arg)
#define TV_WARNING(fmt, arg...)    TV_PRINTF("[TV WARNING]: %s(): "fmt,__FUNCTION__, ##arg)
#define TV_ERROR(fmt, arg...)      TV_PRINTF("[TV ERROR]: %s(): %s@%d: "fmt,__FUNCTION__, __FILE__,__LINE__, ##arg)


#define ENABLE_TVC_INTERRUPT  (1)
#define MTK_TVOUT_USE_SYSRAM_API


// ---------------------------------------------------------------------------
//  TVC Constants
// ---------------------------------------------------------------------------


typedef struct {UINT32 width, height;} _SIZE;

const UINT32 TVC_SRC_WIDTH_MAX    = 1280;//864;
const UINT32 TVC_TAR_WIDTH_MAX    = 640;
const UINT32 TVC_PFH_DMA_FIFO_LEN = 8;
const UINT32 TVC_RSZ_WORK_MEM_SZ  = 8;
static UINT32 check_line_offset = 0;

// Optimized paramters for WVGA
static bool is_used_tar_size_for_hqa =false;
#define IS_HQA() (is_used_tar_size_for_hqa == true)

#define FIXED_WVGA_PARAMS  1

#if FIXED_WVGA_PARAMS

#define IS_NTSC() (TVC_NTSC == _tvcContext.tvType)
#define IS_HORI() (_tvcContext.srcSize.width > _tvcContext.srcSize.height)

#define TV_OUTPUT_WIDTH_HORI       (640)
#define TV_OUTPUT_HEIGHT_HORI      (IS_NTSC() ? 442 : 534)
#define TV_OUTPUT_OFFSET_X_HORI    (32)

#define TV_OUTPUT_WIDTH_VERT       (IS_NTSC() ? 264 : 320)
#define TV_OUTPUT_HEIGHT_VERT      (IS_NTSC() ? 442 : 534)
#define TV_OUTPUT_OFFSET_X_VERT    (IS_NTSC() ? 208 : 200)

#define TV_OUTPUT_WIDTH_NORMAL            (IS_HORI() ? TV_OUTPUT_WIDTH_HORI : TV_OUTPUT_WIDTH_VERT)
#define TV_OUTPUT_HEIGHT_NORMAL           (IS_HORI() ? TV_OUTPUT_HEIGHT_HORI : TV_OUTPUT_HEIGHT_VERT)
#define TV_OUTPUT_OFFSET_X         (IS_HORI() ? TV_OUTPUT_OFFSET_X_HORI : TV_OUTPUT_OFFSET_X_VERT)
#define TV_OUTPUT_OFFSET_Y         (32)

#define TV_OUTPUT_WIDTH_HQA        (720)
#define TV_OUTPUT_HEIGHT_HQA       (IS_NTSC() ? 480 : 576)

#define TV_OUTPUT_WIDTH        (IS_HQA() ? TV_OUTPUT_WIDTH_HQA : TV_OUTPUT_WIDTH_NORMAL)
#define TV_OUTPUT_HEIGHT    (IS_HQA() ? TV_OUTPUT_HEIGHT_HQA : TV_OUTPUT_HEIGHT_NORMAL)

#else
#error "un-implemented"
#endif

// ---------------------------------------------------------------------------
//  TVC Context
// ---------------------------------------------------------------------------

PTVC_REGS const TVC_REG = (PTVC_REGS)(TVC_BASE);

typedef struct
{
#if ENABLE_TVC_INTERRUPT
    wait_queue_head_t tvc_checkline;
    UINT32   tvc_checkline_cnt;
#endif

    BOOL     isTvcPowerOn;
	BOOL	 isTvcEnabled;
    TVC_REGS regBackup;

    TVC_TV_TYPE tvType;

    TVC_SRC_FORMAT srcFormat;
    _SIZE          srcSize;
    _SIZE          tarSize;

    UINT32  srcAddrY;
    UINT32  srcAddrU;
    UINT32  srcAddrV;

    UINT32 pfhDmaBufPA;
    UINT32 reszLineBufPA;

    BOOL srcFormatSizeDirty;
} TVC_CONTEXT;

static TVC_CONTEXT _tvcContext;

//static bool is_used_tar_size_for_hqa =false;


// ---------------------------------------------------------------------------
//  TVC Register Backup/Restore in suspend/resume
// ---------------------------------------------------------------------------

#define TVC_REG_OFFSET(r)       offsetof(TVC_REGS, r)
#define REG_ADDR(base, offset)  (((BYTE *)(base)) + (offset))

const UINT32 BACKUP_TVC_REG_OFFSETS[] =
{
    TVC_REG_OFFSET(CONTROL),

    TVC_REG_OFFSET(SRC_Y_ADDR),
    TVC_REG_OFFSET(SRC_U_ADDR),
    TVC_REG_OFFSET(SRC_V_ADDR),

    TVC_REG_OFFSET(LINE_OFFSET),

    TVC_REG_OFFSET(PFH_DMA_ADDR),
    TVC_REG_OFFSET(PFH_DMA_FIFO_LEN),

    TVC_REG_OFFSET(SRC_WIDTH),
    TVC_REG_OFFSET(SRC_HEIGHT),
    TVC_REG_OFFSET(TAR_WIDTH),
    TVC_REG_OFFSET(TAR_HEIGHT),

    TVC_REG_OFFSET(HRATIO),
    TVC_REG_OFFSET(VRATIO),
    TVC_REG_OFFSET(RESIDUAL),

    TVC_REG_OFFSET(RESZ_ADDR),
    TVC_REG_OFFSET(FINE_RSZ_CFG),

    TVC_REG_OFFSET(START_POINT),
    TVC_REG_OFFSET(STOP_POINT),

    TVC_REG_OFFSET(REG_UPDATE),
    TVC_REG_OFFSET(BG_COLOR),
    TVC_REG_OFFSET(CHECK_LINE),

    TVC_REG_OFFSET(ASYNC_CTRL),
};

#if 0
static unsigned int _GetTime(void)
{
	struct timeval time;
	do_gettimeofday(&time);
	return (time.tv_sec*1000 + time.tv_usec/1000);
}
#endif

static void _BackupTVCRegisters(void)
{
    TVC_REGS *reg = &_tvcContext.regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_TVC_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(reg, BACKUP_TVC_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(TVC_REG, BACKUP_TVC_REG_OFFSETS[i])));
    }
}

static void _RestoreTVCRegisters(void)
{
    TVC_REGS *reg = &_tvcContext.regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_TVC_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(TVC_REG, BACKUP_TVC_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(reg, BACKUP_TVC_REG_OFFSETS[i])));
    }
}

static void _ResetBackupedTVCRegisterValues(void)
{
    TVC_REGS *regs = &_tvcContext.regBackup;
    memset((void*)regs, 0, sizeof(TVC_REGS));

    OUTREG32(&regs->LINE_OFFSET,      0x00200500);
    OUTREG32(&regs->PFH_DMA_FIFO_LEN, 0x8);
    OUTREG32(&regs->SRC_WIDTH,        0x280);
    OUTREG32(&regs->SRC_HEIGHT,       0x1E0);
    OUTREG32(&regs->TAR_WIDTH,        0x280);
    OUTREG32(&regs->TAR_HEIGHT,       0x1A6);
    OUTREG32(&regs->FINE_RSZ_CFG,     0x2);
    OUTREG32(&regs->START_POINT,      0x00160016);
    OUTREG32(&regs->STOP_POINT,       0x028000F0);
    OUTREG32(&regs->BG_COLOR,         0x00808000);
    OUTREG32(&regs->CHECK_LINE,       0x00000017);
    OUTREG32(&regs->ASYNC_CTRL,       0x00000014);
}


// ---------------------------------------------------------------------------
//  Private TVC functions
// ---------------------------------------------------------------------------

#if ENABLE_TVC_INTERRUPT
extern void TVOUT_On_TVC_Done(void);

static __tcmfunc irqreturn_t _TVC_InterruptHandler(int irq, void *dev_id)
{
    TVC_REG_IRQ_STS status = TVC_REG->IRQ_STATUS;

    mt6575_irq_mask(MT6575_TVC_IRQ_ID);

    //printk("_TVC_InterruptHandler, status: %x\n", status);

    if (1 == status.IRQ2) {
        status.IRQ2 = 0;
        printk("[WARN][TVC] Overrun !!\n");
    }
    if (1 == status.IRQ0) {
        status.IRQ0 = 0;
        //printk("wake up tvc_checkline!!\n");
        wake_up_interruptible(&_tvcContext.tvc_checkline);
        TVOUT_On_TVC_Done();
    }
    OUTREG32(&TVC_REG->IRQ_STATUS, AS_UINT32(&status));

    mt6575_irq_unmask(MT6575_TVC_IRQ_ID);

    return IRQ_HANDLED;
}
#endif

static void _WaitForRegUpdated(void)
{
    static const long WAIT_TIMEOUT = 1 * HZ;    // 1 sec
    int cnt = 0;

    if (in_interrupt())
    {
        // perform busy waiting if in interrupt context
        while(TVC_REG->REG_UPDATE.REG_RDY) {}
    }
    else
    {
        while (TVC_REG->REG_UPDATE.REG_RDY)
        {
#if ENABLE_TVC_INTERRUPT
			if (_tvcContext.isTvcEnabled) {
            	long ret = wait_event_interruptible_timeout(_tvcContext.tvc_checkline,
                                                        !TVC_REG->REG_UPDATE.REG_RDY,
                                                        WAIT_TIMEOUT);
            	if (0 == ret) {
                	printk("[WARNING] Wait for TVC checkline IRQ timeout!!!\n");
            	}
			} else {
			    /*
			     * Wait 10ms, if reg still not update,
			     * return otherwise device can not sleep when enter suspend
			     */
			    if (cnt++ == 10) {
                    TV_ERROR("Wait for TVC checkline IRQ timeout!!! 10ms\n");
                    break;
			    }
				msleep(1);
			}
#else
            msleep(1);
#endif
        }
    }
}


static TVC_STATUS _SetTarSize(UINT32 width, UINT32 height)
{
    ASSERT((width & 0x1) == 0); // width must be multiple of 2

    OUTREG32(&TVC_REG->TAR_WIDTH, width);
    OUTREG32(&TVC_REG->TAR_HEIGHT, height);

    return TVC_STATUS_OK;
}


static TVC_STATUS _SetResizCoeff(_SIZE src, _SIZE tar)
{
    TVC_REG_RESIDUAL resdual = {0};

   	UINT32 hratio, vratio;
    UINT32 src_accu = 1;    // assume FINE_RSZ_CFG.SAEN == 1

    if ((src_accu) && (tar.width < src.width))
	{
	   	hratio = ((tar.width - 1) << 20) / (src.width - 1);
	}
	else
	{
	   	hratio = (src.width << 20) / tar.width;
	}

   	if ((src_accu) && (tar.height < src.height))
	{
		vratio = ((tar.height - 1) << 20) / (src.height - 1);
	}
	else
	{
		vratio = (src.height << 20) / tar.height;
	}

	resdual.H_RESIDUAL = src.width % tar.width;
	resdual.V_RESIDUAL = src.height % tar.height;

    OUTREG32(&TVC_REG->HRATIO, hratio);
    OUTREG32(&TVC_REG->VRATIO, vratio);
    OUTREG32(&TVC_REG->RESIDUAL, AS_UINT32(&resdual));

    return TVC_STATUS_OK;
}


static TVC_STATUS _SetDisplayRegion(UINT32 startPixel, UINT32 startLine,
                                    UINT32 stopPixel, UINT32 stopLine)
{
    TVC_REG_POINT START = TVC_REG->START_POINT;
    TVC_REG_POINT STOP  = TVC_REG->STOP_POINT;

    START.PIXEL = startPixel;
    START.LINE  = startLine;
    STOP.PIXEL  = stopPixel;
    STOP.LINE   = stopLine;

    OUTREG32(&TVC_REG->START_POINT, AS_UINT32(&START));
    OUTREG32(&TVC_REG->STOP_POINT, AS_UINT32(&STOP));

    return TVC_STATUS_OK;
}


static TVC_STATUS _ConfigTarSize(void)
{
    _SIZE srcSize = _tvcContext.srcSize;
    _SIZE tarSize = {0};



#if FIXED_WVGA_PARAMS
    tarSize.width  = TV_OUTPUT_WIDTH;
    tarSize.height = TV_OUTPUT_HEIGHT;
#else
    tarSize.width  = 264;
    tarSize.height = 221;
#endif

    _SetTarSize(tarSize.width, tarSize.height);
    _tvcContext.tarSize = tarSize;

    _SetResizCoeff(srcSize, tarSize);

    return TVC_STATUS_OK;
}


static TVC_STATUS _ConfigFullDisplayRegion(void)
{
    UINT32 startPixel, startLine;

#if FIXED_WVGA_PARAMS
    startPixel = TV_OUTPUT_OFFSET_X;
    startLine  = TV_OUTPUT_OFFSET_Y;
#endif

    _SetDisplayRegion(startPixel, startLine,
                      startPixel + _tvcContext.tarSize.width,
                      startLine + _tvcContext.tarSize.height / 2);

    return TVC_STATUS_OK;
}

#define ALIGN_TO_POW_OF_2(x, n)  \
    (((x) + ((n) - 1)) & ~((n) - 1))

static TVC_STATUS _AllocateInternalSRAM(void)
{
    UINT32 pfhDmaFifoSize = TVC_SRC_WIDTH_MAX * 2 * TVC_PFH_DMA_FIFO_LEN;
    UINT32 reszWorkingMemSize = ((TVC_TAR_WIDTH_MAX + 3) / 4 * 4) * 2 * TVC_RSZ_WORK_MEM_SZ;
#if defined MTK_TVOUT_USE_SYSRAM_API
    _tvcContext.pfhDmaBufPA =
        SYSRAM_TVOUT_ALLOC_TIMEOUT(pfhDmaFifoSize + reszWorkingMemSize, 16, 0);
#else
    _tvcContext.pfhDmaBufPA = 0xC2030000;//0xC2008000;
#endif

    if (0 == _tvcContext.pfhDmaBufPA) {
        printk("[TVC][ERROR] allocate TVC internal SRAM failed\n");
        return TVC_STATUS_ERROR;
    }

    _tvcContext.reszLineBufPA = _tvcContext.pfhDmaBufPA + pfhDmaFifoSize;

    ASSERT((_tvcContext.pfhDmaBufPA & 0xF) == 0);
    ASSERT((_tvcContext.reszLineBufPA & 0x7) == 0);

    return TVC_STATUS_OK;
}


static TVC_STATUS _FreeInternalSRAM(void)
{
#if defined MTK_TVOUT_USE_SYSRAM_API
    SYSRAM_TVOUT_FREE();
#endif
	return TVC_STATUS_OK;
}


// ---------------------------------------------------------------------------
//  Public TVC functions
// ---------------------------------------------------------------------------

TVC_STATUS TVC_Init(void)
{
    TV_INFO("Init Start\n");
    memset(&_tvcContext, 0, sizeof(_tvcContext));

    // TVC controller would NOT reset register as default values
    // Do it by SW here
    //
    _ResetBackupedTVCRegisterValues();

#if ENABLE_TVC_INTERRUPT
    if (request_irq(MT6575_TVC_IRQ_ID,
                    (irq_handler_t)_TVC_InterruptHandler,
                    IRQF_TRIGGER_LOW, "mt6575-tvc", NULL) < 0)
    {
        printk("[TVC][ERROR] fail to request TVC irq\n");
        return TVC_STATUS_ERROR;
    }

    init_waitqueue_head(&_tvcContext.tvc_checkline);

    TVC_PowerOn();

    // Enable Interrupt
    {
        TVC_REG_CON CON = TVC_REG->CONTROL;
        CON.CHECK_IRQ = 1;
        CON.OVRUN_IRQ = 1;
        OUTREG32(&TVC_REG->CONTROL, AS_UINT32(&CON));

        TVC_REG->CHECK_LINE = 1;
    }

    TVC_PowerOff();
#endif
    TV_INFO("Init End\n");

    return TVC_STATUS_OK;
}


TVC_STATUS TVC_Deinit(void)
{
    TVC_Disable();
    TVC_PowerOff();

    return TVC_STATUS_OK;
}


TVC_STATUS TVC_PowerOn(void)
{
    if (!_tvcContext.isTvcPowerOn)
    {
        BOOL ret;

    #if 0 // no need to take care of PLL, because PM will take charge of it
        BOOL ret = enable_pll(MT65XX_3G_PLL, "TVC");
        ASSERT(!ret);
    #endif

        ret = enable_clock(MT65XX_PDN_MM_TVC, "TVC");
        ASSERT(!ret);

        _RestoreTVCRegisters();
        _tvcContext.isTvcPowerOn = TRUE;

    }

    return TVC_STATUS_OK;
}


TVC_STATUS TVC_PowerOff(void)
{
    if (_tvcContext.isTvcPowerOn)
    {

        BOOL ret = TRUE;

        _BackupTVCRegisters();
    #if 0
        ret = hwDisablePLL(MT65XX_3G_PLL, "TVC");
        ASSERT(ret);
    #endif

        ret = disable_clock(MT65XX_PDN_MM_TVC, "TVC");
        ASSERT(!ret);

        _tvcContext.isTvcPowerOn = FALSE;

    }

    return TVC_STATUS_OK;
}


TVC_STATUS TVC_Enable(void)
{
    TV_INFO("\n");
	if (_tvcContext.isTvcEnabled) {
		printk("[TVC]TVC already enabled\n");
		return TVC_STATUS_OK;
	}
	if (TVC_STATUS_OK != _AllocateInternalSRAM()) {
        	return TVC_STATUS_ERROR;
	}

    OUTREG32(&TVC_REG->PFH_DMA_ADDR, _tvcContext.pfhDmaBufPA);
    OUTREG32(&TVC_REG->PFH_DMA_FIFO_LEN, TVC_PFH_DMA_FIFO_LEN);

    OUTREG32(&TVC_REG->RESZ_ADDR, _tvcContext.reszLineBufPA);
    {
        TVC_REG_FRCFG frcfg = TVC_REG->FINE_RSZ_CFG;
        frcfg.WMSZ = TVC_RSZ_WORK_MEM_SZ;
        OUTREG32(&TVC_REG->FINE_RSZ_CFG, AS_UINT32(&frcfg));
    }

    {
        TVC_REG_CON CON = TVC_REG->CONTROL;
        CON.FAST_MODE    = 1;
        CON.DMAIF_GULTRA = 1;
        CON.BURST_TYPE   = 0;
        CON.PFH          = 1;
        CON.RESZ_GULTRA  = 1;
        CON.PFH_GULTRA   = 1;
        CON.DMAIF_GPROT  = 1;
        OUTREG32(&TVC_REG->CONTROL, AS_UINT32(&CON));
    }
    OUTREG32(&TVC_REG->ENABLE, 1);
	_tvcContext.isTvcEnabled = true;

    return TVC_STATUS_OK;
}


TVC_STATUS TVC_Disable(void)
{
    TV_INFO("\n");

    if (_tvcContext.isTvcEnabled == false)
    {
        TV_WARNING("TVC is disabled, but disable again\n");
        return TVC_STATUS_OK;
    }

	_tvcContext.isTvcEnabled = false;
    OUTREG32(&TVC_REG->ENABLE, 0);

	{
        TVC_REG_UPDATE update = TVC_REG->REG_UPDATE;
        update.REG_RDY = 1;
        OUTREG32(&TVC_REG->REG_UPDATE, AS_UINT32(&update));
    }

	_WaitForRegUpdated();


    _FreeInternalSRAM();

    return TVC_STATUS_OK;
}


TVC_STATUS TVC_SetTvType(TVC_TV_TYPE type)
{
    _tvcContext.tvType = type;
    _tvcContext.srcFormatSizeDirty = TRUE;
    return TVC_STATUS_OK;
}


TVC_STATUS TVC_SetSrcFormat(TVC_SRC_FORMAT format)
{
    if (_tvcContext.srcFormat != format) {
        _tvcContext.srcFormatSizeDirty = TRUE;
        _tvcContext.srcFormat = format;
    }
    return TVC_STATUS_OK;
}

TVC_SRC_FORMAT TVC_GetSrcFormat(void)
{
    return _tvcContext.srcFormat;
}


TVC_STATUS TVC_SetSrcRGBAddr(UINT32 address)
{
    ASSERT((address & 0x3) == 0); // check if 4-byte aligned

    _tvcContext.srcAddrY = address;
    //_tvcContext.srcAddrU = 0;
    //_tvcContext.srcAddrV = 0;
    //OUTREG32(&TVC_REG->SRC_Y_ADDR, address);

    return TVC_STATUS_OK;
}


UINT32 TVC_GetWorkingAddr()
{
    return (UINT32)TVC_REG->SRC_Y_ADDR;
}


TVC_STATUS TVC_SetSrcYUVAddr(UINT32 Y, UINT32 U, UINT32 V)
{
    // check if all buffer addresses are 8-byte aligned
    ASSERT((Y & 0x7) == 0);
    ASSERT((U & 0x7) == 0);
    ASSERT((V & 0x7) == 0);

    _tvcContext.srcAddrY = Y;
    _tvcContext.srcAddrU = U;
    _tvcContext.srcAddrV = V;

    //OUTREG32(&TVC_REG->SRC_Y_ADDR, Y);
    //OUTREG32(&TVC_REG->SRC_U_ADDR, U);
    //OUTREG32(&TVC_REG->SRC_V_ADDR, V);

    return TVC_STATUS_OK;
}


TVC_STATUS TVC_SetSrcSize(UINT32 width, UINT32 height)
{
    //TV_INFO("width %d, height %d\n", width, height);
    ASSERT((width  & 0x3) == 0);    // multiple of 4
    ASSERT((height & 0x3) == 0);    // multiple of 4
    //width = 480;
    //height = 400;

    ASSERT(width <= TVC_SRC_WIDTH_MAX);

    if (_tvcContext.srcSize.width != width ||
        _tvcContext.srcSize.height != height)
    {
        _tvcContext.srcSize.width  = width;
        _tvcContext.srcSize.height = height;
#if defined ENABLE_TVC_INTERRUPT
        //_tvcContext.tvc_checkline_cnt = height - check_line_offset;
        _tvcContext.tvc_checkline_cnt = 1 + check_line_offset;
#endif
        _tvcContext.srcFormatSizeDirty = TRUE;
    }
    return TVC_STATUS_OK;
}

void TVC_SetCheckLineOffset(UINT32 offset)
{
    check_line_offset = offset;
    _tvcContext.srcFormatSizeDirty = TRUE;
    TV_INFO("%d\n", check_line_offset);
}


#if 1
// This fuction is used in TV-out HQA, we set tar size to fit HW designer's requirement.
TVC_STATUS TVC_SetTarSizeForHQA(BOOL enable)
{
	is_used_tar_size_for_hqa = enable;
	printk("[TVC] Tar size for HQA is (%s)\n", (enable?"on":"off"));
	return TVC_STATUS_OK;
}
#endif


TVC_STATUS TVC_CommitChanges(BOOL blocking)
{
    if (_tvcContext.srcFormatSizeDirty)
    {
        _WaitForRegUpdated();

        // Config Source Format
        {
            TVC_REG_CON CON = TVC_REG->CONTROL;
            CON.DATA_FMT = _tvcContext.srcFormat;
            OUTREG32(&TVC_REG->CONTROL, AS_UINT32(&CON));
        }

        // Config Source Size
        OUTREG32(&TVC_REG->SRC_WIDTH, _tvcContext.srcSize.width);
        OUTREG32(&TVC_REG->SRC_HEIGHT, _tvcContext.srcSize.height);

        // Config Line Pitch
        {
            TVC_REG_LINE_OFFSET OFFSET = TVC_REG->LINE_OFFSET;
            OFFSET.LINE_OFFSET = (TVC_YUV420_BLK == _tvcContext.srcFormat ||
                                  TVC_YUV420_PLANAR == _tvcContext.srcFormat) ?
                                 (_tvcContext.srcSize.width * 1) :
                                 (_tvcContext.srcSize.width * 2);
            OUTREG32(&TVC_REG->LINE_OFFSET, AS_UINT32(&OFFSET));
        }

        // Config Target Size
        _ConfigTarSize();

        // Config Full Display Region
        _ConfigFullDisplayRegion();


		// Config check line num, for MTKYUV the min is 4, for others the min is 1
#if 0
		if (_tvcContext.srcFormat == TVC_YUV420_BLK) {
        	TVC_REG->CHECK_LINE = 4;
		} else {
        	TVC_REG->CHECK_LINE = 1;
		}
#endif
#if defined ENABLE_TVC_INTERRUPT
        //_tvcContext.tvc_checkline_cnt = _tvcContext.srcSize.height - check_line_offset;
        _tvcContext.tvc_checkline_cnt = 1 + check_line_offset;
        if (_tvcContext.srcFormat == TVC_YUV420_BLK)
        {
            _tvcContext.tvc_checkline_cnt += 0x3;
            _tvcContext.tvc_checkline_cnt &= ~0x3;
        }
        ASSERT(_tvcContext.tvc_checkline_cnt != 0);
        TV_INFO("check line: %d\n", _tvcContext.tvc_checkline_cnt);
        TVC_REG->CHECK_LINE = _tvcContext.tvc_checkline_cnt;
#endif
        _tvcContext.srcFormatSizeDirty = FALSE;
    }

    OUTREG32(&TVC_REG->SRC_Y_ADDR, _tvcContext.srcAddrY);
    if (_tvcContext.srcFormat == TVC_YUV420_BLK || _tvcContext.srcFormat == TVC_YUV420_PLANAR)
    {
        OUTREG32(&TVC_REG->SRC_U_ADDR, _tvcContext.srcAddrU);
        OUTREG32(&TVC_REG->SRC_V_ADDR, _tvcContext.srcAddrV);
    }

    // Commit the Register Changes
    {
        TVC_REG_UPDATE update = TVC_REG->REG_UPDATE;
        update.REG_RDY = 1;
        OUTREG32(&TVC_REG->REG_UPDATE, AS_UINT32(&update));
    }

    if (blocking) {
        _WaitForRegUpdated();
    }

    return TVC_STATUS_OK;
}


TVC_STATUS TVC_DumpRegisters(void)
{
    UINT32 i;

    printk("---------- Start dump TVC registers ----------\n"
           "TVC_BASE: 0x%08x\n", TVC_BASE);

    for (i = 0; i < sizeof(TVC_REGS); i += 4)
    {
        printk("TVC+%04x : 0x%08x\n", i, INREG32(TVC_BASE + i));
    }

    return TVC_STATUS_OK;
}

void TVC_ConfigSize(unsigned int src_width, unsigned int src_height, unsigned int tar_width, unsigned int tar_height)
{

    _SIZE tarSize = {0};

    TV_INFO("src %d, %d, tar %d, %d\n", src_width, src_height, tar_width, tar_height);


    //src size
    if (_tvcContext.srcSize.width != src_width ||
        _tvcContext.srcSize.height != src_height)
    {
        _tvcContext.srcSize.width  = src_width;
        _tvcContext.srcSize.height = src_height;
        _tvcContext.srcFormatSizeDirty = TRUE;
    }
    // Config Source Size
    OUTREG32(&TVC_REG->SRC_WIDTH, _tvcContext.srcSize.width);
    OUTREG32(&TVC_REG->SRC_HEIGHT, _tvcContext.srcSize.height);

    // Config Line Pitch
    {
        TVC_REG_LINE_OFFSET OFFSET = TVC_REG->LINE_OFFSET;
        OFFSET.LINE_OFFSET = (TVC_YUV420_BLK == _tvcContext.srcFormat ||
                              TVC_YUV420_PLANAR == _tvcContext.srcFormat) ?
                             (_tvcContext.srcSize.width * 1) :
                             (_tvcContext.srcSize.width * 2);
        OUTREG32(&TVC_REG->LINE_OFFSET, AS_UINT32(&OFFSET));
    }




    //tar size
    tarSize.width = tar_width;
    tarSize.height = tar_height;

    _SetTarSize(tar_width, tar_height);
    _tvcContext.tarSize = tarSize;

    _SetResizCoeff(_tvcContext.srcSize, tarSize);

    _ConfigFullDisplayRegion();

    {
        TVC_REG_UPDATE update = TVC_REG->REG_UPDATE;
        update.REG_RDY = 1;
        OUTREG32(&TVC_REG->REG_UPDATE, AS_UINT32(&update));
    }


}


TVC_STATUS TVC_AllocMva(unsigned int va, unsigned int size, unsigned int* mva)
{
#if defined(MTK_M4U_SUPPORT)

    int ret;
    unsigned int mva_tvc;

    if (!_m4u_tvout_func.isInit)
	{
		TV_ERROR("M4U has not init func for TV-out\n");
		return TVC_STATUS_ERROR;
	}

    //Config TVC&M4U
	ret = _m4u_tvout_func.m4u_alloc_mva(M4U_CLNTMOD_TVC, va, size, &mva_tvc);
	if (ret != 0)
    {
        TV_ERROR("m4u_alloc_mva\n");
        return TVC_STATUS_ERROR;
    }


    _m4u_tvout_func.m4u_insert_tlb_range(M4U_CLNTMOD_TVC,
                                         (unsigned int)mva_tvc,
                                         (unsigned int)(mva_tvc + size - 1),
                                         SEQ_RANGE_LOW_PRIORITY,
                                         1);

    *mva = mva_tvc;
#endif
    return TVC_STATUS_OK;
}

TVC_STATUS TVC_DeallocMva(unsigned int va, unsigned int size, unsigned int mva)
{

#if defined(MTK_M4U_SUPPORT)
	if (!_m4u_tvout_func.isInit)
	{
		TV_ERROR("M4U has not init func for TV-out\n");
		return TVC_STATUS_ERROR;
	}


    _m4u_tvout_func.m4u_invalid_tlb_range(M4U_CLNTMOD_TVC,
                                  		 (unsigned int)mva,
                      					 (unsigned int)(mva + size - 1));

    if (0 != _m4u_tvout_func.m4u_dealloc_mva(M4U_CLNTMOD_TVC, va, size, mva))
    {
        TV_ERROR("Dealocate MVA FAIL\n");
        return TVC_STATUS_ERROR;
    }
#endif
    return TVC_STATUS_OK;
}



TVC_STATUS TVC_CheckFormat(TVC_SRC_FORMAT format)
{
    if (format != TVC_RGB565 &&
        format != TVC_YUV420_PLANAR &&
        format != TVC_UYVY422 &&
        format != TVC_YUV420_BLK)
        return TVC_STATUS_ERROR;
    else
        return TVC_STATUS_OK;
}


#endif
