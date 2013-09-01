

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



#include "tve_reg.h"
#include "tve_drv.h"

#include "mtkfb.h"

#if defined(MTK_TVOUT_SUPPORT)


#define TV_PRINTF             printk
#define TV_INFO(fmt, arg...)       TV_PRINTF("[TV INFO]: %s(): "fmt,__FUNCTION__, ##arg)
#define TV_WARNING(fmt, arg...)    TV_PRINTF("[TV WARNING]: %s(): "fmt,__FUNCTION__, ##arg)
#define TV_ERROR(fmt, arg...)      TV_PRINTF("[TV ERROR]: %s(): %s@%d: "fmt,__FUNCTION__, __FILE__,__LINE__, ##arg)


#define TVE_PARAMETE_V2



PTVE_REGS const TVE_REG = (PTVE_REGS)(TVE_BASE);

static BOOL s_isTvePowerOn  = FALSE;
static TVE_TV_TYPE s_tvType = TVE_NTSC;
static TVE_REGS regBackup;

#define TVE_REG_OFFSET(r)       offsetof(TVE_REGS, r)
#define REG_ADDR(base, offset)  (((BYTE *)(base)) + (offset))

const UINT32 BACKUP_TVE_REG_OFFSETS[] =
{
    TVE_REG_OFFSET(MODE),
    TVE_REG_OFFSET(SCALE_CON),
    TVE_REG_OFFSET(DAC_CON),
    TVE_REG_OFFSET(BURST),
    TVE_REG_OFFSET(FREQ),
    TVE_REG_OFFSET(SLEW),

    TVE_REG_OFFSET(YLPFC),
    TVE_REG_OFFSET(YLPFD),
    TVE_REG_OFFSET(YLPFE),
    TVE_REG_OFFSET(CLPFA),
    TVE_REG_OFFSET(CLPFB),
    TVE_REG_OFFSET(CLPFC),

    TVE_REG_OFFSET(GAMMAA),
    TVE_REG_OFFSET(GAMMAB),
    TVE_REG_OFFSET(GAMMAC),
    TVE_REG_OFFSET(GAMMAD),
    TVE_REG_OFFSET(GAMMAE),

    TVE_REG_OFFSET(CMPCODE),
    TVE_REG_OFFSET(PLUG),
};

static void _BackupTVERegisters(void)
{
    TVE_REGS *reg = &regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_TVE_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(reg, BACKUP_TVE_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(TVE_REG, BACKUP_TVE_REG_OFFSETS[i])));
    }
}

static void _RestoreTVERegisters(void)
{
    TVE_REGS *reg = &regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_TVE_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(TVE_REG, BACKUP_TVE_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(reg, BACKUP_TVE_REG_OFFSETS[i])));
    }
}

static void _ResetBackupedTVERegisterValues(void)
{
    TVE_REGS *regs = &regBackup;
    memset((void*)regs, 0, sizeof(TVE_REGS));

    OUTREG32(&regs->SCALE_CON,  0x00045A5A);
    OUTREG32(&regs->BURST,      0x0000003A);
    OUTREG32(&regs->FREQ,       0x0DD0010F);
    OUTREG32(&regs->SLEW,       0x00C806A4);

    OUTREG32(&regs->YLPFC,      0x32020000);
    OUTREG32(&regs->YLPFD,      0x021E3D25);
    OUTREG32(&regs->YLPFE,      0x90B4FFC7);

    OUTREG32(&regs->CLPFA,      0x180D1001);
    OUTREG32(&regs->CLPFB,      0x25342021);
    OUTREG32(&regs->CLPFC,      0x0000273C);

    OUTREG32(&regs->GAMMAB,     0x0314018A);
    OUTREG32(&regs->GAMMAC,     0x0629049E);
    OUTREG32(&regs->GAMMAD,     0x093D07B3);
    OUTREG32(&regs->GAMMAE,     0x0C520AC8);

    OUTREG32(&regs->CMPCODE,    0x00000FFF);
    OUTREG32(&regs->RESET,      0x00000001);
}

#if 0
static __tcmfunc irqreturn_t _TVE_InterruptHandler(int irq, void *dev_id)
{
    return IRQ_HANDLED;
}
#endif


TVE_STATUS TVE_Init(void)
{
    TV_INFO("Init Start\n");
    // TVE controller would NOT reset register as default values
    // Do it by SW here
    //
    _ResetBackupedTVERegisterValues();
#if 0
    if (request_irq(MT6575_TVE_IRQ_ID,
                    (irq_handler_t)_TVE_InterruptHandler,
                    0, "mt6575-tve", NULL) < 0)
    {
        printk("[TVE][ERROR] fail to request TVE irq\n");
        return TVE_STATUS_ERROR;
    }
#endif
    // Disable TVDAC defaultly
    //OUTREG32(VTV_CON0, 0x0);
    //OUTREG32(MIXEDSYS0_BASE + 0xBC0, 0x0);
    //OUTREG32(MIXEDSYS0_BASE + 0xBC4, 0x0);
    OUTREG32(0xF0007600,0x1012);//Disable TV DAC
    TV_INFO("Init End\n");

    return TVE_STATUS_OK;
}


TVE_STATUS TVE_Deinit(void)
{
    TVE_Disable();
    TVE_PowerOff();

    return TVE_STATUS_OK;
}


extern int pll_fsel(enum mt65xx_pll_id id, unsigned int pll_value);
TVE_STATUS TVE_PowerOn()
{
    if (!s_isTvePowerOn)
    {

        BOOL ret;
    #if 1
        ret = pll_fsel(MT65XX_TVDDS, 0x38E40A8E);
        ASSERT(!ret);
        ret = enable_pll(MT65XX_TVDDS, "TVE");
        ASSERT(!ret);
        ret = enable_clock(MT65XX_PDN_MM_TVE, "TVE");
        ASSERT(!ret);
    #else
        ret = enable_clock(MT65XX_PDN_MM_TVE, "TVE");
        ASSERT(!ret);
        //Enable TVDSS, set it to 27MHZ
        OUTREG32(0xf00071A0,0x0A8E);
        OUTREG32(0xf00071A4,0x38E4);
        OUTREG32(0xf00071A8,0x7C54);
        msleep(10); //waiting for PLL stable
    #endif

        _RestoreTVERegisters();
        s_isTvePowerOn = TRUE;
    }

    return TVE_STATUS_OK;
}


TVE_STATUS TVE_PowerOff()
{
    if (s_isTvePowerOn)
    {

        BOOL ret = TRUE;
        _BackupTVERegisters();
    #if 1
        ret = disable_pll(MT65XX_TVDDS, "TVE");
        ASSERT(!ret);
    #endif
        ret = disable_clock(MT65XX_PDN_MM_TVE, "TVE");
        ASSERT(!ret);
        s_isTvePowerOn = FALSE;

    }

    return TVE_STATUS_OK;
}


TVE_STATUS TVE_Enable(void)
{
    TVE_REG_MODE mode = TVE_REG->MODE;
    mode.ENCON = 1;
    OUTREG32(&TVE_REG->MODE, AS_UINT32(&mode));

    /* TVDAC enable
       TVDAC control is moved to MIXEDSYS
    */
    //OUTREG32(VTV_CON0, 0x5);
    //OUTREG32(MIXEDSYS0_BASE + 0xBC0, 0x1FF2);
    //OUTREG32(MIXEDSYS0_BASE + 0xBC4, 0x0);
    OUTREG32(0xF0007600,0xB02);//Enable TV DAC
    OUTREG32(&TVE_REG->DAC_CON, 0x0);

    return TVE_STATUS_OK;
}


TVE_STATUS TVE_Disable(void)
{
    TVE_REG_MODE mode = TVE_REG->MODE;
    mode.ENCON = 0;
    OUTREG32(&TVE_REG->MODE, AS_UINT32(&mode));

    /* TVDAC disable
       TVDAC control is moved to MIXEDSYS
    */
    //OUTREG32(VTV_CON0, 0x0);
    //OUTREG32(MIXEDSYS0_BASE + 0xBC0, 0x0);
    //OUTREG32(MIXEDSYS0_BASE + 0xBC4, 0x0);
    OUTREG32(0xF0007600,0x1012);//Disable TV DAC

    return TVE_STATUS_OK;
}


TVE_STATUS TVE_SetTvType(TVE_TV_TYPE type)
{
    {
        TVE_REG_MODE MODE = TVE_REG->MODE;
        MODE.TVTYPE = (TVE_NTSC == type) ? TVE_NTSC : TVE_PAL;
        OUTREG32(&TVE_REG->MODE, AS_UINT32(&MODE));
    }

    if (TVE_NTSC == type) {
        OUTREG32(&TVE_REG->FREQ, ((0xdd0<<16)|0x10f));
    } else {
        OUTREG32(&TVE_REG->FREQ, ((0x81d<<16)|0x150));
    }

    s_tvType = type;

    return TVE_STATUS_OK;
}


TVE_STATUS TVE_EnableColorBar(BOOL enable)
{
    TVE_REG_MODE mode = TVE_REG->MODE;
    mode.CBON = enable ? 1 : 0;
    OUTREG32(&TVE_REG->MODE, AS_UINT32(&mode));

    return TVE_STATUS_OK;
}

#if defined (TVE_PARAMETE_V2)
TVE_STATUS TVE_ResetDefaultSettings(void)
{
    if (!s_isTvePowerOn) return TVE_STATUS_OK;

    OUTREG32(&TVE_REG->BURST, 0x3c);

    OUTREG32(&TVE_REG->DAC_CON, 0x0);

    //OUTREG32(&TVE_REG->SCALE_CON, 0x47050); // Scale of UV
    OUTREG32(&TVE_REG->SCALE_CON, 0x45840); // Scale of UV

    //OUTREG32(&TVE_REG->GAMMAB, (605<<16)|302);
    //OUTREG32(&TVE_REG->GAMMAC, (1210<<16)|907);
    //OUTREG32(&TVE_REG->GAMMAD, (1815<<16)|1512);
    //OUTREG32(&TVE_REG->GAMMAE, (2420<<16)|2117);

    OUTREG32(&TVE_REG->GAMMAB, 0x02440122);
    OUTREG32(&TVE_REG->GAMMAC, 0x04880366);
    OUTREG32(&TVE_REG->GAMMAD, 0x06cc05aa);
    OUTREG32(&TVE_REG->GAMMAE, 0x091007ee);


    OUTREG32(&TVE_REG->YLPFC, 0x32020000);
    OUTREG32(&TVE_REG->YLPFD, 0x021E3D25);
    OUTREG32(&TVE_REG->YLPFE, 0x90B4FFC7);

    OUTREG32(&TVE_REG->CLPFA, 0x180D1001);
    OUTREG32(&TVE_REG->CLPFB, 0x25342021);
    OUTREG32(&TVE_REG->CLPFC, 0x0000273C);

    {
        TVE_REG_MODE MODE = {0};

		MODE.SYDEL = 1;
		MODE.YDEL  = 2;

        //MODE.CLPON  = 1;            // Turn on Chroma LPS

		MODE.TVTYPE = s_tvType;

        OUTREG32(&TVE_REG->MODE, AS_UINT32(&MODE));
    }

    TVE_SetTvType(s_tvType);

    return TVE_STATUS_OK;
}
#else
TVE_STATUS TVE_ResetDefaultSettings(void)
{
    if (!s_isTvePowerOn) return TVE_STATUS_OK;

    OUTREG32(&TVE_REG->BURST, 0x3d);

    OUTREG32(&TVE_REG->DAC_CON, 0x0);

    OUTREG32(&TVE_REG->SCALE_CON, 0x47050); // Scale of UV

    OUTREG32(&TVE_REG->GAMMAB, (605<<16)|302);
    OUTREG32(&TVE_REG->GAMMAC, (1210<<16)|907);
    OUTREG32(&TVE_REG->GAMMAD, (1815<<16)|1512);
    OUTREG32(&TVE_REG->GAMMAE, (2420<<16)|2117);

    OUTREG32(&TVE_REG->YLPFC, 0x32020000);
    OUTREG32(&TVE_REG->YLPFD, 0x021E3D25);
    OUTREG32(&TVE_REG->YLPFE, 0x90B4FFC7);

    OUTREG32(&TVE_REG->CLPFA, 0x180D1001);
    OUTREG32(&TVE_REG->CLPFB, 0x25342021);
    OUTREG32(&TVE_REG->CLPFC, 0x0000273C);

    {
        TVE_REG_MODE MODE = {0};

        MODE.SETUP  = 1;            // Turn on 7.5 IRE
        MODE.CLPON  = 1;            // Turn on Chroma LPS
        MODE.TVTYPE = s_tvType;

        OUTREG32(&TVE_REG->MODE, AS_UINT32(&MODE));
    }

    TVE_SetTvType(s_tvType);

    return TVE_STATUS_OK;
}
#endif



TVE_STATUS TVE_DumpRegisters(void)
{
    UINT32 i;

    printk("---------- Start dump TVE registers ----------\n"
           "TVE_BASE: 0x%08x\n", TVE_BASE);

    for (i = 0; i < sizeof(TVE_REGS); i += 4)
    {
        printk("TVE+%04x : 0x%08x\n", i, INREG32(TVE_BASE + i));
    }

    return TVE_STATUS_OK;
}

#endif
