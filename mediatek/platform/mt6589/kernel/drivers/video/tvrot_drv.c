

#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_reg_base.h>
#include <mach/mt_clkmgr.h>

#include <mach/mt6575_sysram.h>

#include "tvrot_reg.h"
#include "tvrot_drv.h"

#include "mtkfb.h"
#include <asm/tcm.h>
#include <mach/mt6575_m4u.h>
#include "lcd_drv.h"


#if defined(MTK_TVOUT_SUPPORT)

#define TV_PRINTF             printk
#define TV_INFO(fmt, arg...)       TV_PRINTF("[TV INFO]: %s(): "fmt,__FUNCTION__, ##arg)
#define TV_WARNING(fmt, arg...)    TV_PRINTF("[TV WARNING]: %s(): "fmt,__FUNCTION__, ##arg)
#define TV_ERROR(fmt, arg...)      TV_PRINTF("[TV ERROR]: %s(): %s@%d: "fmt,__FUNCTION__, __FILE__,__LINE__, ##arg)



#define ENABLE_TVROT_INTERRUPT (1)
#define MTK_TVOUT_USE_SYSRAM_API


PTVR_REGS const TVR_REG = (PTVR_REGS)(TV_ROT_BASE);

static BOOL s_isTvrPowerOn = FALSE;
static BOOL s_isSramAllocated = FALSE;
#if defined TV_BUFFER_FMT_UYVY
const UINT32 RESAMPLE = 1;
#else
const UINT32 RESAMPLE = 0;
#endif

#if defined TV_BUFFER_PIPE

typedef struct
{
	BOOL	    isTvrEnabled;

    UINT32      srcWidth;
    UINT32      srcHeight;
    TVR_FORMAT  dstFormat;
    TVR_ROT     rot;

    BOOL        settingDirty;
    UINT32      descAddr;
    UINT32      dstBufOffset;
} TVR_CONTEXT;

#define MT6575_SYSRAM_BASE_PA 0xC2000000
#define MT6575_SYSRAM_BASE_VA 0xF2000000
#define MT6575_SYSRAM_SIZE    0x40000 //256kb


static TVR_CONTEXT _tvrContext = {0};

#endif

// ---------------------------------------------------------------------------
//  TVR Register Backup/Restore in suspend/resume
// ---------------------------------------------------------------------------

static TVR_REGS regBackup;

#define TVR_REG_OFFSET(r)       offsetof(TVR_REGS, r)
#define REG_ADDR(base, offset)  (((BYTE *)(base)) + (offset))

const UINT32 BACKUP_TVR_REG_OFFSETS[] =
{
    TVR_REG_OFFSET(IRQ_FLAG),
    TVR_REG_OFFSET(CFG),
    TVR_REG_OFFSET(DROP_INPUT),
    TVR_REG_OFFSET(STOP),
    TVR_REG_OFFSET(ENABLE),
    TVR_REG_OFFSET(RD_BASE),
    TVR_REG_OFFSET(WR_BASE),
    TVR_REG_OFFSET(QUEUE_BASE),
    TVR_REG_OFFSET(EXEC_CNT),
    TVR_REG_OFFSET(SLOW_DOWN),
    TVR_REG_OFFSET(BUF_BASE_ADDR0),
    TVR_REG_OFFSET(BUF_BASE_ADDR1),
    TVR_REG_OFFSET(Y_DST_STR_ADDR),
    TVR_REG_OFFSET(SRC_SIZE),
    TVR_REG_OFFSET(CLIP_SIZE),
    TVR_REG_OFFSET(CLIP_OFFSET),
    TVR_REG_OFFSET(DST_WIDTH_IN_BYTE),
    TVR_REG_OFFSET(CON),
    TVR_REG_OFFSET(PERF),
    TVR_REG_OFFSET(MAIN_BUF_SIZE),
    TVR_REG_OFFSET(SUB_BUF_SIZE),
    TVR_REG_OFFSET(BUF_BASE_ADDR2),
    TVR_REG_OFFSET(BUF_BASE_ADDR3)
};

#if 0
static void _BackupTVRRegisters(void)
{
    TVR_REGS *reg = &regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_TVR_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(reg, BACKUP_TVR_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(TVR_REG, BACKUP_TVR_REG_OFFSETS[i])));
    }
}

static void _RestoreTVRRegisters(void)
{
    TVR_REGS *reg = &regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_TVR_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(TVR_REG, BACKUP_TVR_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(reg, BACKUP_TVR_REG_OFFSETS[i])));
    }
}
#endif

// ---------------------------------------------------------------------------
//  Private TVR functions
// ---------------------------------------------------------------------------

static BOOL _reset_tvrot(void)
{
    UINT32 timeout = 0;

    TVR_REG_RESET reset = {0};
    reset.WARN_RESET = 1;
    OUTREG32(&TVR_REG->RESET, AS_UINT32(&reset));

    while (TVR_REG->RESET.WARN_RESET) {
        ++ timeout;
        if (timeout > 100000) {
            printk("[TVR][ERROR] Reset timeout\n");
            return FALSE;
        }
    }

    return TRUE;
}


#define ALIGN_TO_POW_OF_2(x, n)  \
    (((x) + ((n) - 1)) & ~((n) - 1))


static BOOL _alloc_and_set_sram(const TVR_PARAM *param)
{
    UINT32 main_lb_s_in_line = 0;
    UINT32 sub_lb_s_in_line = 0;

    UINT32 main_buf_line_size = 0;
    UINT32 main_blk_w = 0;
    UINT32 sub_buf_line_size = 0;
    UINT32 sub_blk_w = 0;

    UINT32 main_size = 0;
    UINT32 sub_size = 0;
    UINT32 desc_size = 4 * 2;

    UINT32 lb_bpp = 0;
    UINT32 src_width = param->srcWidth;

    UINT32 main_buf_addr = 0;
    UINT32 sub_buf_addr = 0;
    UINT32 desc_addr = 0;

    BOOL need_sub_buffer = FALSE;

    if (s_isSramAllocated)
    {
        TV_WARNING("TVR SYSRAM already been set, but set again\n ");
        return TRUE;
    }

    switch(param->outputFormat)
    {
    case TVR_RGB565  :
        main_lb_s_in_line = 4;
        lb_bpp = 2;
        need_sub_buffer = RESAMPLE ? TRUE : FALSE;
        break;
    case TVR_YUYV422 :
        main_lb_s_in_line = 8;
        lb_bpp = 1;
        need_sub_buffer = TRUE;
        break;
    }

    // calculate main buffer

    main_blk_w = (src_width + main_lb_s_in_line - 1) / main_lb_s_in_line;

    main_buf_line_size = main_blk_w * (main_lb_s_in_line + 1);          // FIFO mode
    main_size = lb_bpp * main_buf_line_size * (main_lb_s_in_line + 1);  // FIFO mode

    // calculate sub buffer

    if (need_sub_buffer)
    {
        sub_lb_s_in_line = (main_lb_s_in_line / 2);

        if (RESAMPLE) src_width >>= 1;

        sub_blk_w = (src_width + sub_lb_s_in_line - 1) / sub_lb_s_in_line;

        sub_buf_line_size = sub_blk_w * (sub_lb_s_in_line + 1);     // FIFO mode
        sub_size = 2 * sub_buf_line_size * (sub_lb_s_in_line + 1);   // FIFO mode
    }

    main_size = ALIGN_TO_POW_OF_2(main_size, 8);
    sub_size  = ALIGN_TO_POW_OF_2(sub_size,  8);

    printk("[TVR] Allocate internal SRAM, main_size: %d, sub_size: %d\n",
           main_size, sub_size);

    // try to allocate internal SRAM
#if defined MTK_TVOUT_USE_SYSRAM_API
    main_buf_addr = SYSRAM_TV_ROT_ALLOC_TIMEOUT(main_size + sub_size + desc_size, 8, 0);
#else
    main_buf_addr = 0xC2000000;//0xC2000000;
#endif

    if (0 == main_buf_addr) {
        printk("[TVR][ERROR] allocate internal SRAM failed\n");
        return FALSE;
    }



    sub_buf_addr = main_buf_addr + main_size;
    desc_addr    = sub_buf_addr + sub_size;

    if (!need_sub_buffer) sub_buf_addr = 0;

    OUTREG32(&TVR_REG->BUF_BASE_ADDR0, main_buf_addr);
    OUTREG32(&TVR_REG->BUF_BASE_ADDR2, sub_buf_addr);
    {
        TVR_REG_BUF_SIZE buf_size;

        buf_size.LINE_SIZE   = main_buf_line_size;
        buf_size.BLOCK_WIDTH = main_blk_w;
        OUTREG32(&TVR_REG->MAIN_BUF_SIZE, AS_UINT32(&buf_size));

        buf_size.LINE_SIZE    = sub_buf_line_size;
        buf_size.BLOCK_WIDTH  = sub_blk_w;
        OUTREG32(&TVR_REG->SUB_BUF_SIZE, AS_UINT32(&buf_size));
    }
    {
        TVR_REG_PERF perf;
        perf.FIFO_MODE = 1;
        perf.MAIN_LB_S_IN_LINE = main_lb_s_in_line;
        perf.THRESHOLD = 7;
        OUTREG32(&TVR_REG->PERF, AS_UINT32(&perf));
    }

    // set descriptor base address
#if 1
    OUTREG32(&TVR_REG->QUEUE_BASE, desc_addr);
#if defined TV_BUFFER_PIPE
    _tvrContext.descAddr = desc_addr - MT6575_SYSRAM_BASE_PA + MT6575_SYSRAM_BASE_VA;
#endif
#else
    // Fixed descriptor internals SRAM address for debug
    OUTREG32(&TVR_REG->QUEUE_BASE, 0x40043E80);
#endif

    s_isSramAllocated = TRUE;


    return TRUE;
}

static UINT32 _cal_dst_buffer_pitch(const TVR_PARAM *param)
{
    switch(param->rotation)
    {
    case TVR_ROT_0 :
    case TVR_ROT_180 :
        return param->srcWidth * 2;

    case TVR_ROT_90  :
    case TVR_ROT_270 :
        return param->srcHeight * 2;

    default :
        return 0;
    }
}
static UINT32 _cal_dst_buffer_offset(const TVR_PARAM *param,
                                     UINT32 dstPitchInBytes)
{
    switch(param->rotation)
    {
    case TVR_ROT_180 :
        return dstPitchInBytes * (param->srcHeight - 1);
    case TVR_ROT_270 :
        return dstPitchInBytes * (param->srcWidth - 1);
    default :
        return 0;
    }
}

static BOOL _config_tvrot_reg(const TVR_PARAM *param)
{

    UINT32 dstPitchInBytes = _cal_dst_buffer_pitch(param);
    UINT32 dstBufOffset    = _cal_dst_buffer_offset(param, dstPitchInBytes);

    {
        TVR_REG_CFG config = {0};

        // descriptor mode
        {
            config.AUTO_LOOP   = 1;
            config.DOUBLE_BUF  = 0;
            config.MODE        = 1;
            config.QUEUE_DEPTH = TVR_BUFFERS- 1; // double DST buffers
            config.SEG1EN      = 1;     // enable DST buffer address field only
        }
        OUTREG32(&TVR_REG->CFG, AS_UINT32(&config));
    }
    {
        TVR_REG_SIZE size;
        size.WIDTH  = param->srcWidth;
        size.HEIGHT = param->srcHeight;
        OUTREG32(&TVR_REG->SRC_SIZE, AS_UINT32(&size));
        OUTREG32(&TVR_REG->CLIP_SIZE, AS_UINT32(&size));
        OUTREG32(&TVR_REG->CLIP_OFFSET, 0);
    }
    {
        TVR_REG_CON control   = TVR_REG->CON;
        control.OUTPUT_FORMAT = param->outputFormat;
        control.ROT_ANGLE     = param->rotation;
        control.FLIP          = param->flip ? 1 : 0;
        control.RESAMPLE      = RESAMPLE;
        control.PERF_MODE     = 0;
        OUTREG32(&TVR_REG->CON, AS_UINT32(&control));
    }

    OUTREG32(&TVR_REG->DST_WIDTH_IN_BYTE, dstPitchInBytes);

    // mingchen: query the OK bit firstly, then write data to queue.
    // descriptor mode
    {
        UINT32 i, timeout;

        for (i = 0; i < ARY_SIZE(param->dstBufAddr); ++ i) {
            //OUTREG32(&TVR_REG->QUEUE_DATA, param->dstBufAddr[i] + dstBufOffset);
            /*
            printk("[TVR]58:%08x 60:%08x 68:%08x 70:%08x 78:%08x 318:%08x\n",
                INREG32(0xf209f058),
                INREG32(0xf209f060),
                INREG32(0xf209f068),
                INREG32(0xf209f070),
                INREG32(0xf209f078),
                INREG32(0xf209f318));
                */

            timeout = 0;
            while (0 == TVR_REG->QUEUE_WSTA.OK) {
                ++ timeout;
                if (timeout > 100000) {
                    printk("[TVR][ERROR] QUEUE_DATA timeout\n");
                    return FALSE;
                }
            }
            OUTREG32(&TVR_REG->QUEUE_DATA, param->dstBufAddr[i] + dstBufOffset);
            /*
            printk("[TVR]58:%08x 60:%08x 68:%08x 70:%08x 78:%08x 318:%08x\n",
                INREG32(0xf209f058),
                INREG32(0xf209f060),
                INREG32(0xf209f068),
                INREG32(0xf209f070),
                INREG32(0xf209f078),
                INREG32(0xf209f318));
                */
        }
    }
#if defined TV_BUFFER_PIPE
    _tvrContext.rot = param->rotation;
    _tvrContext.srcWidth = param->srcWidth;
    _tvrContext.srcHeight = param->srcHeight;
    _tvrContext.dstFormat = param->outputFormat;
    _tvrContext.dstBufOffset = dstBufOffset;
    TV_INFO("%d\n", dstBufOffset);
#endif

    return TRUE;
}

#if defined MTK_TVROT_LDVT
#define MTK_TVR_TEST_DESCRIPTOR 0
static BOOL _config_tvrot_reg_dvt(const TVR_PARAM *param)
{
#if MTK_TVR_TEST_DESCRIPTOR

    printk("%s\n", __func__);

    UINT32 dstPitchInBytes = _cal_dst_buffer_pitch(param);
    UINT32 dstBufOffset    = _cal_dst_buffer_offset(param, dstPitchInBytes);

    {
        TVR_REG_CFG config = {0};
        if (param->bAuto)
        // descriptor mode
        {
            config.AUTO_LOOP   = 1;
            config.DOUBLE_BUF  = 0;
            config.MODE        = 1;
            config.QUEUE_DEPTH = param->dstBufNum - 1; // double DST buffers
            config.SEG1EN      = 1;     // enable DST buffer address field only
            config.SEG2EN      = 1;     //src size
            config.SEG3EN      = 1;     //src roi size
            config.SEG4EN      = 1;     //src roi offset
            config.SEG5EN      = 1;     //dst width in byte
            config.SEG6EN      = 1;     //con
            config.SEG7EN      = 0;     //perf
            config.SEG8EN      = 0;     //main buffer
            config.SEG9EN      = 0;     //sbu buffer
        }

        OUTREG32(&TVR_REG->CFG, AS_UINT32(&config));
    }

    // mingchen: query the OK bit firstly, then write data to queue.
    // descriptor mode
    {
        TVR_REG_SIZE size;
        TVR_REG_SIZE clip_size;
        TVR_REG_OFS  clip_offset;

        size.WIDTH  = param->srcWidth;
        size.HEIGHT = param->srcHeight;
        clip_size.WIDTH = param->srcRoi.w;
        clip_size.HEIGHT = param->srcRoi.h;
        clip_offset.X = param->srcRoi.x;
        clip_offset.Y = param->srcRoi.y;


        TVR_REG_CON control   = TVR_REG->CON;
        control.OUTPUT_FORMAT = param->outputFormat;
        control.ROT_ANGLE     = param->rotation;
        control.FLIP          = param->flip ? 1 : 0;
        control.RESAMPLE      = RESAMPLE;
        control.PERF_MODE     = 0;


        UINT32 i, timeout;

        for (i = 0; i < param->dstBufNum; ++ i) {

            //dst addr
            timeout = 0;
            while (0 == TVR_REG->QUEUE_WSTA.OK) {
                ++ timeout;
                if (timeout > 100000) {
                    printk("[TVR][ERROR] QUEUE_DATA timeout dst addr\n");
                    return FALSE;
                }
            }
            OUTREG32(&TVR_REG->QUEUE_DATA, param->dstBufAddr[i] + dstBufOffset);

            //src w/h
            timeout = 0;
            while (0 == TVR_REG->QUEUE_WSTA.OK) {
                ++ timeout;
                if (timeout > 100000) {
                    printk("[TVR][ERROR] QUEUE_DATA timeout src w/h\n");
                    return FALSE;
                }
            }
            OUTREG32(&TVR_REG->QUEUE_DATA, AS_UINT32(&size));

            //src roi w/h
            timeout = 0;
            while (0 == TVR_REG->QUEUE_WSTA.OK) {
                ++ timeout;
                if (timeout > 100000) {
                    printk("[TVR][ERROR] QUEUE_DATA timeout src roi w/h\n");
                    return FALSE;
                }
            }
            OUTREG32(&TVR_REG->QUEUE_DATA, AS_UINT32(&clip_size));

            //src roi x/y
            timeout = 0;
            while (0 == TVR_REG->QUEUE_WSTA.OK) {
                ++ timeout;
                if (timeout > 100000) {
                    printk("[TVR][ERROR] QUEUE_DATA timeout src roi x/y\n");
                    return FALSE;
                }
            }
            OUTREG32(&TVR_REG->QUEUE_DATA, AS_UINT32(&clip_offset));

            //dst with in byte
            timeout = 0;
            while (0 == TVR_REG->QUEUE_WSTA.OK) {
                ++ timeout;
                if (timeout > 100000) {
                    printk("[TVR][ERROR] QUEUE_DATA timeout dst with in byte\n");
                    return FALSE;
                }
            }
            OUTREG32(&TVR_REG->QUEUE_DATA, dstPitchInBytes);

            //con
            timeout = 0;
            while (0 == TVR_REG->QUEUE_WSTA.OK) {
                ++ timeout;
                if (timeout > 100000) {
                    printk("[TVR][ERROR] QUEUE_DATA timeout con\n");
                    return FALSE;
                }
            }
            OUTREG32(&TVR_REG->QUEUE_DATA, AS_UINT32(&control));

            //perf

            //main buffer size

            //sub buffer size



        }
    }

    return TRUE;


#else
    printk("%s\n", __func__);

    UINT32 dstPitchInBytes = _cal_dst_buffer_pitch(param);
    UINT32 dstBufOffset    = _cal_dst_buffer_offset(param, dstPitchInBytes);

    {
        TVR_REG_CFG config = {0};
        if (param->bAuto)
        // descriptor mode
        {
            config.AUTO_LOOP   = 1;
            config.DOUBLE_BUF  = 0;
            config.MODE        = 1;
            config.QUEUE_DEPTH = 3 - 1; // double DST buffers
            config.SEG1EN      = 1;     // enable DST buffer address field only
        }
        else
        {
            config.AUTO_LOOP   = 0;
            config.DOUBLE_BUF  = 0;
            config.MODE        = 0;
            config.QUEUE_DEPTH = 0; // double DST buffers
            //config.SEG1EN      = 1;     // enable DST buffer address field only
        }


        OUTREG32(&TVR_REG->CFG, AS_UINT32(&config));
    }
    {
        TVR_REG_SIZE size;
        TVR_REG_OFS  offset;
        size.WIDTH  = param->srcWidth;
        size.HEIGHT = param->srcHeight;
        OUTREG32(&TVR_REG->SRC_SIZE, AS_UINT32(&size));
        size.WIDTH = param->srcRoi.w;
        size.HEIGHT = param->srcRoi.h;
        offset.X = param->srcRoi.x;
        offset.Y = param->srcRoi.y;
        OUTREG32(&TVR_REG->CLIP_SIZE, AS_UINT32(&size));
        OUTREG32(&TVR_REG->CLIP_OFFSET, AS_UINT32(&offset));
    }
    {
        TVR_REG_CON control   = TVR_REG->CON;
        control.OUTPUT_FORMAT = param->outputFormat;
        control.ROT_ANGLE     = param->rotation;
        control.FLIP          = param->flip ? 1 : 0;
        control.RESAMPLE      = RESAMPLE;
        control.PERF_MODE     = 0;
        OUTREG32(&TVR_REG->CON, AS_UINT32(&control));
    }

    OUTREG32(&TVR_REG->DST_WIDTH_IN_BYTE, dstPitchInBytes);


    if (param->bAuto)
    // mingchen: query the OK bit firstly, then write data to queue.
    // descriptor mode
    {
        UINT32 i, timeout;

        for (i = 0; i < ARY_SIZE(param->dstBufAddr); ++ i) {
            //OUTREG32(&TVR_REG->QUEUE_DATA, param->dstBufAddr[i] + dstBufOffset);

            timeout = 0;
            while (0 == TVR_REG->QUEUE_WSTA.OK) {
                ++ timeout;
                if (timeout > 100000) {
                    printk("[TVR][ERROR] QUEUE_DATA timeout\n");
                    return FALSE;
                }
            }
            OUTREG32(&TVR_REG->QUEUE_DATA, param->dstBufAddr[i] + dstBufOffset);
        }
    }
    else
        OUTREG32(&TVR_REG->Y_DST_STR_ADDR, param->dstBufAddr[0] + dstBufOffset);
#endif
    return TRUE;
}

#endif //MTK_TVROT_LDVT


// ---------------------------------------------------------------------------
//  Interrupt Handler
// ---------------------------------------------------------------------------
#if ENABLE_TVROT_INTERRUPT
#if !defined(CONFIG_MTK_LDVT)
extern void TVOUT_On_TVR_Done(void);
#endif
static __tcmfunc irqreturn_t _TVR_InterruptHandler(int irq, void *dev_id)
{
    TVR_REG_IRQ_FLAG flag = TVR_REG->IRQ_FLAG;
    TVR_REG_IRQ_FLAG_CLR clr = {0};
    mt6575_irq_mask(MT6575_TV_ROT_IRQ_ID);


    if (flag.FLAG0) {
#if !defined(CONFIG_MTK_LDVT)
        TVOUT_On_TVR_Done();
#endif
        clr.FLAG0_CLR = 1;
    }
    if (flag.FLAG1) {
        printk("[TVR][IRQ] FLAG1: SW configuration error\n");
        clr.FLAG1_CLR = 1;
    }
    if (flag.FLAG5) {
        printk("[TVR][IRQ] FLAG5: SW configuration error\n");
        clr.FLAG5_CLR = 1;
    }

    OUTREG32(&TVR_REG->IRQ_FLAG_CLR, AS_UINT32(&clr));
    mt6575_irq_unmask(MT6575_TV_ROT_IRQ_ID);

    return IRQ_HANDLED;
}
#endif

// ---------------------------------------------------------------------------
//  Public TVR functions
// ---------------------------------------------------------------------------

TVR_STATUS TVR_Init(void)
{
    TV_INFO("Init Start\n");
 #if ENABLE_TVROT_INTERRUPT
    if (request_irq(MT6575_TV_ROT_IRQ_ID,
        (irq_handler_t)_TVR_InterruptHandler, IRQF_TRIGGER_LOW, "mt6575-tvrot", NULL) < 0)
    {
        printk("[TVR][ERROR] fail to request TVR irq\n");
        return TVR_STATUS_ERROR;
    }
 #endif
    TV_INFO("Init End\n");

    return TVR_STATUS_OK;
}


TVR_STATUS TVR_Deinit(void)
{
    TVR_Stop();
    TVR_PowerOff();

    return TVR_STATUS_OK;
}


TVR_STATUS TVR_PowerOn()
{
    if (!s_isTvrPowerOn)
    {

        BOOL ret;
     #if 0
        BOOL ret = enable_pll(MT65XX_3G_PLL, "TVR");
        ASSERT(!ret);
     #endif
        ret = enable_clock(MT65XX_PDN_MM_TV_ROT, "TVR");
        ASSERT(!ret);

#if 0
        _RestoreTVRRegisters();
#endif
        s_isTvrPowerOn = TRUE;

    }

    return TVR_STATUS_OK;
}


TVR_STATUS TVR_PowerOff()
{
    if (s_isTvrPowerOn)
    {

        BOOL ret = TRUE;
#if 0
        _BackupTVRRegisters();
#endif
    #if 0
        ret = hwDisablePLL(MT65XX_3G_PLL, "TVR");
        ASSERT(!ret);
    #endif

        ret = disable_clock(MT65XX_PDN_MM_TV_ROT, "TVR");
        ASSERT(!ret);

        s_isTvrPowerOn = FALSE;

    }

    return TVR_STATUS_OK;
}


TVR_STATUS TVR_Start(void)
{
    if (!s_isSramAllocated) {
        printk("[TVR][ERROR] working SRAM is not allocated!!\n");
        return TVR_STATUS_ERROR;
    }
    OUTREG32(&TVR_REG->ENABLE, 1);
#if defined TV_BUFFER_PIPE
    _tvrContext.isTvrEnabled = true;
#endif
	LCD_W2TVR_NeedLimiteSpeed(TRUE);
    return TVR_STATUS_OK;
}


TVR_STATUS TVR_Stop(void)
{
    OUTREG32(&TVR_REG->STOP, 1);

    if (s_isSramAllocated) {
        #if defined MTK_TVOUT_USE_SYSRAM_API
        SYSRAM_TV_ROT_FREE();
        #endif
        s_isSramAllocated = FALSE;
    }
#if defined TV_BUFFER_PIPE
    _tvrContext.isTvrEnabled = false;
#endif
	LCD_W2TVR_NeedLimiteSpeed(FALSE);

    return TVR_STATUS_OK;
}


TVR_STATUS TVR_Config(const TVR_PARAM *param)
{

    if (!_reset_tvrot()) {
        return TVR_STATUS_ERROR;
    }

    // Enable All Interrupts
    {
        TVR_REG_IRQ_FLAG flag = {0};
        flag.FLAG0_IRQ_EN = 1;
        flag.FLAG1_IRQ_EN = 1;
        flag.FLAG5_IRQ_EN = 1;
        OUTREG32(&TVR_REG->IRQ_FLAG, AS_UINT32(&flag));
    }

    if (!_alloc_and_set_sram(param)) {
        return TVR_STATUS_INSUFFICIENT_SRAM;
    }
#if defined MTK_TVROT_LDVT
    if (!_config_tvrot_reg_dvt(param)) {
        return TVR_STATUS_ERROR;
    }
#else
    if (!_config_tvrot_reg(param)) {
        return TVR_STATUS_ERROR;
    }
#endif

    return TVR_STATUS_OK;
}

#if defined TV_BUFFER_PIPE

UINT32 TVR_GetWorkingAddr()
{
    return (UINT32)TVR_REG->Y_DST_STR_ADDR;
}


// IN TVR done interrupt, we need to change buffer addr in the descrpitor
bool TVR_EnqueueBuffer(UINT32 addr, UINT32* pAddrPrev)
{
    UINT32 dstPitchInBytes = (_tvrContext.rot & 0x1) ? (_tvrContext.srcHeight * 2) : (_tvrContext.srcWidth * 2);
    UINT32 dstBufOffset;

    UINT32 descAddr;
    UINT32 descRPT;

    static UINT32 preRTP = 0xFFFF;
    TVR_REG_QUEUE_RSTA descStas;

    if(!_tvrContext.isTvrEnabled)
    {
        TV_INFO("TVR has not been enabled\n");
        return true;
    }

    //descRPT = INREG32(0xf209f040)>>8 & 0xf;
    descStas = TVR_REG->QUEUE_RSTA;
    descRPT = descStas.RPT;

    /*
    if (descRPT == preRTP)
    {
        TV_WARNING("TVR not update RPT %d\n", descRPT);
    }

    printk("[TVR]40:%08x 48:%08x 50:%08x 80:%08x 318:%08x\n",
        INREG32(0xf209f040),
        INREG32(0xf209f048),
        INREG32(0xf209f050),
        INREG32(0xf209f080),
        INREG32(0xf209f318));
    */

    preRTP = descRPT;

    descRPT = (descRPT == 0) ? (TVR_BUFFERS -1) : (descRPT - 1);
    ASSERT(descRPT < TVR_BUFFERS);

    descAddr = _tvrContext.descAddr + descRPT * 4;
    ASSERT(MT6575_SYSRAM_BASE_VA <= descAddr &&
                 descAddr < MT6575_SYSRAM_BASE_VA + MT6575_SYSRAM_SIZE);
    switch(_tvrContext.rot)
    {
        case TVR_ROT_180 :
            dstBufOffset = dstPitchInBytes * (_tvrContext.srcHeight - 1);
        case TVR_ROT_270 :
            dstBufOffset = dstPitchInBytes * (_tvrContext.srcWidth - 1);
        default :
            dstBufOffset = 0;
    }
    *pAddrPrev = INREG32(descAddr);
    OUTREG32(descAddr, (addr + _tvrContext.dstBufOffset));
    return true;
}

#endif

TVR_STATUS TVR_Wait_Done(void)
{
    UINT32 timeout = 0;
    TVR_REG_IRQ_FLAG_CLR clr = {0};

    while (!TVR_REG->IRQ_FLAG.FLAG0) {
        ++ timeout;
        if (timeout > 100000) {
            printk("[TVR][ERROR] Wait done timeout\n");
            return TVR_STATUS_ERROR;
        }
    }

    clr.FLAG0_CLR = 1;

    OUTREG32(&TVR_REG->IRQ_FLAG_CLR, AS_UINT32(&clr));

    TVR_DumpRegisters();

    return TVR_STATUS_OK;

}



TVR_STATUS TVR_AllocMva(unsigned int va, unsigned int size, unsigned int* mva)
{
#if defined(MTK_M4U_SUPPORT)
    int ret;
    unsigned int mva_tvr;

    if (!_m4u_tvout_func.isInit)
	{
		TV_ERROR("M4U has not init func for TV-out\n");
		return TVR_STATUS_ERROR;
	}

    //Config TVC&M4U
	ret = _m4u_tvout_func.m4u_alloc_mva(M4U_CLNTMOD_TVROT, va, size, &mva_tvr);
	if (ret != 0)
    {
        TV_ERROR("m4u_alloc_mva\n");
        return TVR_STATUS_ERROR;
    }

    _m4u_tvout_func.m4u_insert_tlb_range(M4U_CLNTMOD_TVROT,
                                         (unsigned int)mva_tvr,
                                         (unsigned int)(mva_tvr + size - 1),
                                         SEQ_RANGE_LOW_PRIORITY,
                                         1);

    *mva = mva_tvr;
#endif
    return TVR_STATUS_OK;
}

TVR_STATUS TVR_DeallocMva(unsigned int va, unsigned int size, unsigned int mva)
{

#if defined(MTK_M4U_SUPPORT)
	if (!_m4u_tvout_func.isInit)
	{
		TV_ERROR("M4U has not init func for TV-out\n");
		return TVR_STATUS_ERROR;
	}


    _m4u_tvout_func.m4u_invalid_tlb_range(M4U_CLNTMOD_TVROT,
                                  		 (unsigned int)mva,
                      					 (unsigned int)(mva + size - 1));

    if (0 != _m4u_tvout_func.m4u_dealloc_mva(M4U_CLNTMOD_TVROT, va, size, mva))
    {
        TV_ERROR("Dealocate MVA FAIL\n");
        return TVR_STATUS_ERROR;
    }
#endif
    return TVR_STATUS_OK;
}



TVR_STATUS TVR_DumpRegisters(void)
{
    UINT32 i;

    printk("---------- Start dump TVR registers ----------\n"
           "TVR_BASE: 0x%08x\n", TV_ROT_BASE);

    for (i = 0; i <= offsetof(TVR_REGS, EXEC_CNT); i += 4)
    {
        printk("TVR+%04x : 0x%08x\n", i, INREG32(TV_ROT_BASE + i));
    }

    for (i = offsetof(TVR_REGS, SLOW_DOWN); i < sizeof(TVR_REGS); i += 4)
    {
        printk("TVR+%04x : 0x%08x\n", i, INREG32(TV_ROT_BASE + i));
    }
    return TVR_STATUS_OK;
}

#endif
