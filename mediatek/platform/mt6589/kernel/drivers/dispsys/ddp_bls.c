#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/xlog.h>
#include <linux/mutex.h>
#include <mach/mt_clkmgr.h>

#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_debug.h"

#define POLLING_TIME_OUT 1000

#define PWM_LOW_LIMIT 1  //PWM output lower bound = 8

#if !defined(MTK_AAL_SUPPORT)
static int gBLSMutexID = 3;
static int gBLSPowerOn = 0;
#endif
static int gMaxLevel = 255;

static DEFINE_MUTEX(backlight_mutex);

#if defined(ONE_WIRE_PULSE_COUNTING) 
#define MAX_PWM_WAVENUM 16
#define PWM_TIME_OUT 1000*100
static int g_previous_wavenum = 0;
static int g_previous_level = 0;
#endif

static DISPLAY_PWM_T g_pwm_lut;
static DISPLAY_GAMMA_T g_gamma_lut;
static DISPLAY_GAMMA_T g_gamma_index = 
{
entry:
{
    {
      0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 
	  1,  17,  33,  49,  65,  81,  97, 113, 129, 145, 161, 177, 193, 209, 225, 241, 
	  2,  18,  34,  50,  66,  82,  98, 114, 130, 146, 162, 178, 194, 210, 226, 242, 
	  3,  19,  35,  51,  67,  83,  99, 115, 131, 147, 163, 179, 195, 211, 227, 243, 
	  4,  20,  36,  52,  68,  84, 100, 116, 132, 148, 164, 180, 196, 212, 228, 244, 
	  5,  21,  37,  53,  69,  85, 101, 117, 133, 149, 165, 181, 197, 213, 229, 245, 
	  6,  22,  38,  54,  70,  86, 102, 118, 134, 150, 166, 182, 198, 214, 230, 246, 
	  7,  23,  39,  55,  71,  87, 103, 119, 135, 151, 167, 183, 199, 215, 231, 247, 
	  8,  24,  40,  56,  72,  88, 104, 120, 136, 152, 168, 184, 200, 216, 232, 248, 
	  9,  25,  41,  57,  73,  89, 105, 121, 137, 153, 169, 185, 201, 217, 233, 249, 
	 10,  26,  42,  58,  74,  90, 106, 122, 138, 154, 170, 186, 202, 218, 234, 250, 
	 11,  27,  43,  59,  75,  91, 107, 123, 139, 155, 171, 187, 203, 219, 235, 251, 
	 12,  28,  44,  60,  76,  92, 108, 124, 140, 156, 172, 188, 204, 220, 236, 252, 
	 13,  29,  45,  61,  77,  93, 109, 125, 141, 157, 173, 189, 205, 221, 237, 253, 
	 14,  30,  46,  62,  78,  94, 110, 126, 142, 158, 174, 190, 206, 222, 238, 254, 
	 15,  31,  47,  63,  79,  95, 111, 127, 143, 159, 175, 191, 207, 223, 239, 255
    },
    {
	  0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 
	  1,  17,  33,  49,  65,  81,  97, 113, 129, 145, 161, 177, 193, 209, 225, 241, 
	  2,  18,  34,  50,  66,  82,  98, 114, 130, 146, 162, 178, 194, 210, 226, 242, 
	  3,  19,  35,  51,  67,  83,  99, 115, 131, 147, 163, 179, 195, 211, 227, 243, 
	  4,  20,  36,  52,  68,  84, 100, 116, 132, 148, 164, 180, 196, 212, 228, 244, 
	  5,  21,  37,  53,  69,  85, 101, 117, 133, 149, 165, 181, 197, 213, 229, 245, 
	  6,  22,  38,  54,  70,  86, 102, 118, 134, 150, 166, 182, 198, 214, 230, 246, 
	  7,  23,  39,  55,  71,  87, 103, 119, 135, 151, 167, 183, 199, 215, 231, 247, 
	  8,  24,  40,  56,  72,  88, 104, 120, 136, 152, 168, 184, 200, 216, 232, 248, 
	  9,  25,  41,  57,  73,  89, 105, 121, 137, 153, 169, 185, 201, 217, 233, 249, 
	 10,  26,  42,  58,  74,  90, 106, 122, 138, 154, 170, 186, 202, 218, 234, 250, 
	 11,  27,  43,  59,  75,  91, 107, 123, 139, 155, 171, 187, 203, 219, 235, 251, 
	 12,  28,  44,  60,  76,  92, 108, 124, 140, 156, 172, 188, 204, 220, 236, 252, 
	 13,  29,  45,  61,  77,  93, 109, 125, 141, 157, 173, 189, 205, 221, 237, 253, 
	 14,  30,  46,  62,  78,  94, 110, 126, 142, 158, 174, 190, 206, 222, 238, 254, 
	 15,  31,  47,  63,  79,  95, 111, 127, 143, 159, 175, 191, 207, 223, 239, 255
    },
    {
	  0,  16,  32,  48,  64,  80,  96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 
	  1,  17,  33,  49,  65,  81,  97, 113, 129, 145, 161, 177, 193, 209, 225, 241, 
	  2,  18,  34,  50,  66,  82,  98, 114, 130, 146, 162, 178, 194, 210, 226, 242, 
	  3,  19,  35,  51,  67,  83,  99, 115, 131, 147, 163, 179, 195, 211, 227, 243, 
	  4,  20,  36,  52,  68,  84, 100, 116, 132, 148, 164, 180, 196, 212, 228, 244, 
	  5,  21,  37,  53,  69,  85, 101, 117, 133, 149, 165, 181, 197, 213, 229, 245, 
	  6,  22,  38,  54,  70,  86, 102, 118, 134, 150, 166, 182, 198, 214, 230, 246, 
	  7,  23,  39,  55,  71,  87, 103, 119, 135, 151, 167, 183, 199, 215, 231, 247, 
	  8,  24,  40,  56,  72,  88, 104, 120, 136, 152, 168, 184, 200, 216, 232, 248, 
	  9,  25,  41,  57,  73,  89, 105, 121, 137, 153, 169, 185, 201, 217, 233, 249, 
	 10,  26,  42,  58,  74,  90, 106, 122, 138, 154, 170, 186, 202, 218, 234, 250, 
	 11,  27,  43,  59,  75,  91, 107, 123, 139, 155, 171, 187, 203, 219, 235, 251, 
	 12,  28,  44,  60,  76,  92, 108, 124, 140, 156, 172, 188, 204, 220, 236, 252, 
	 13,  29,  45,  61,  77,  93, 109, 125, 141, 157, 173, 189, 205, 221, 237, 253, 
	 14,  30,  46,  62,  78,  94, 110, 126, 142, 158, 174, 190, 206, 222, 238, 254, 
	 15,  31,  47,  63,  79,  95, 111, 127, 143, 159, 175, 191, 207, 223, 239, 255
    }
}
};

DISPLAY_GAMMA_T * get_gamma_index(void)
{
    DISP_DBG("get_gamma_index!\n");
    return &g_gamma_index;
}

DISPLAY_PWM_T * get_pwm_lut(void)
{
    DISP_DBG("get_pwm_lut!\n");
    return &g_pwm_lut;
}

extern unsigned char aal_debug_flag;
void disp_onConfig_bls(DISP_AAL_PARAM *param)
{
    unsigned long prevSetting = DISP_REG_GET(DISP_REG_BLS_BLS_SETTING);
    unsigned long regVal = 0;
    
    DISP_DBG("disp_onConfig_bls!\n");

    DISP_DBG("pwm duty = %lu\n", param->pwmDuty);
    if (param->pwmDuty == 0)
        DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, 0);
    else if (param->pwmDuty > gMaxLevel)
        DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, (PWM_LOW_LIMIT << 19) | gMaxLevel);
    else
        DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, (PWM_LOW_LIMIT << 19) | param->pwmDuty);

    DISP_DBG("bls setting = %lu\n", param->setting);
    if (param->setting & ENUM_FUNC_GAMMA)
        regVal |= 0x7;
    else
        regVal &= ~0x7;
    
    if (param->setting & ENUM_FUNC_BLS)
        regVal |= 0x11D00;
    else
        regVal &= ~0x11D00;
    DISP_REG_SET(DISP_REG_BLS_BLS_SETTING, regVal);
    
    if (param->setting & ENUM_FUNC_BLS)
    {
        DISP_DBG("distion threshold = %lu\n", param->maxClrDistThd);
        DISP_DBG("predistion threshold = %lu\n", param->preDistThd);
        DISP_DBG("scene change threshold = %lu\n", param->scDiffThd);
        DISP_DBG("scene change bin = %lu\n", param->scBinThd);

        DISP_REG_SET(DISPSYS_BLS_BASE + 0x0024, 0x00000000);
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x0028, 0x00010001);
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x0040, param->maxClrLimit);
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x0044, param->preDistThd);
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x0048, param->maxClrDistThd);
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x0068, param->scDiffThd);
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x006C, param->scBinThd);
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x0070, 0x01A201A2);    // F_3db = 1/60 Hz
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x0074, 0x00003CBC);
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x0078, 0x01500150);    // F_3db = 1/75 Hz
        DISP_REG_SET(DISPSYS_BLS_BASE + 0x007C, 0x00003D60);
    }

    if (prevSetting & 0x11D00) 
    {
        unsigned char autoMaxClr, autoMaxClrFlt, autoDp, autoDpFlt;
        regVal = DISP_REG_GET(DISPSYS_BLS_BASE + 0x0204);
        autoMaxClr = regVal & 0xFF;
        autoMaxClrFlt = (regVal >> 8) & 0xFF;
        autoDp = (regVal >> 16) & 0xFF;
        autoDpFlt = (regVal >> 24) & 0xFF;

        DISP_DBG("MaxClr=%u, MaxClrFlt=%u, Dp=%u, DpFlt=%u\n", autoMaxClr, autoMaxClrFlt, autoDp, autoDpFlt);

        if (autoMaxClr != autoMaxClrFlt || autoDp != autoDpFlt) 
        {
            disp_set_aal_alarm(1);
        }
        else 
        {
            disp_set_aal_alarm(0);
        }
    }
    else if (param->setting & ENUM_FUNC_BLS)
    {
        disp_set_aal_alarm(1);
    }

    if (aal_debug_flag == 0)
        DISP_REG_SET(DISP_REG_BLS_EN, 0x80010001);
    else
        DISP_REG_SET(DISP_REG_BLS_EN, 0x80000000);
}


static unsigned int brightness_mapping(unsigned int level)
{
    unsigned int mapped_level;
    
#if defined(ONE_WIRE_PULSE_COUNTING) 
    mapped_level = (level + (MAX_PWM_WAVENUM / 2)) / MAX_PWM_WAVENUM;

    if (level != 0 && mapped_level == 0)
        mapped_level = 1;
    
    if (mapped_level > MAX_PWM_WAVENUM)
        mapped_level = MAX_PWM_WAVENUM;    
#else
    mapped_level = level;

    if (mapped_level > gMaxLevel)
        mapped_level = gMaxLevel;
#endif    
	return mapped_level;
}

#if !defined(MTK_AAL_SUPPORT)
static int disp_poll_for_reg(unsigned int addr, unsigned int value, unsigned int mask, unsigned int timeout)
{
    unsigned int cnt = 0;
    
    while ((DISP_REG_GET(addr) & mask) != value)
    {
        msleep(1);
        cnt++;
        if (cnt > timeout)
        {
            return -1;
        }
    }

    return 0;
}

static int disp_bls_get_mutex(void)
{
    if (gBLSMutexID < 0)
        return -1;

    DISP_REG_SET(DISP_REG_CONFIG_MUTEX(gBLSMutexID), 1);
    if(disp_poll_for_reg(DISP_REG_CONFIG_MUTEX(gBLSMutexID), 0x2, 0x2, POLLING_TIME_OUT))
    {
        DISP_ERR("get mutex timeout! \n");
        disp_dump_reg(DISP_MODULE_CONFIG);
        return -1;
    }
    return 0;
}

static int disp_bls_release_mutex(void)
{ 
    if (gBLSMutexID < 0)
        return -1;
    
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX(gBLSMutexID), 0);
    if(disp_poll_for_reg(DISP_REG_CONFIG_MUTEX(gBLSMutexID), 0, 0x2, POLLING_TIME_OUT))
    {
        DISP_ERR("release mutex timeout! \n");
        disp_dump_reg(DISP_MODULE_CONFIG);
        return -1;
    }
    return 0;
}
#endif

void disp_bls_update_gamma_lut(void)
{
    int index, i;
    unsigned long regValue;
    unsigned long LastVal, MSB, CurVal, Count;

    DISP_MSG("disp_bls_update_gamma_lut!\n");
    
    // init gamma table
    // convert from NCStool output format to BLS GAMMA 10 bits lut
    for(index = 0; index < 3; index++)
    {    
        LastVal = MSB = CurVal = Count = 0;
    
        for(Count = 0; Count < 256 ; Count++)
        {  
            CurVal = g_gamma_index.entry[index][Count];
            if(LastVal > CurVal)
            {
                MSB++;
            }
            //get GammaTable Value
            g_gamma_lut.entry[index][Count] = (MSB << 8) | CurVal;
            LastVal = CurVal;
        }
    }
    
    // program SRAM
    regValue = DISP_REG_GET(DISP_REG_BLS_EN);
    if (regValue & 0x1) {
        DISP_ERR("update GAMMA LUT while BLS func enabled!\n");
        disp_dump_reg(DISP_MODULE_BLS);
    }
    //DISP_REG_SET(DISP_REG_BLS_EN, (regValue & 0x80000000));
    DISP_REG_SET(DISP_REG_BLS_LUT_UPDATE, 0x1);
        
    for (i = 0; i < 256 ; i++)
    {
        CurVal = (((g_gamma_lut.entry[0][i]>>2)<<20) | ((g_gamma_lut.entry[1][i]>>2)<<10) | (g_gamma_lut.entry[2][i]>>2));
        DISP_REG_SET(DISP_REG_BLS_GAMMA_LUT(i), CurVal);
        DISP_DBG("[%d] GAMMA LUT = 0x%x, (%lu, %lu, %lu)\n", i, DISP_REG_GET(DISP_REG_BLS_GAMMA_LUT(i)), 
            g_gamma_lut.entry[0][i], g_gamma_lut.entry[1][i], g_gamma_lut.entry[2][i]);
    }
    
    /* Set Gamma Last point*/    
    DISP_REG_SET(DISP_REG_BLS_GAMMA_SETTING, 0x00000001);
    
    //FIXME! use 256th as 257th 
    LastVal = (((g_gamma_lut.entry[0][255]>>2)<<20) | ((g_gamma_lut.entry[1][255]>>2)<<10) | (g_gamma_lut.entry[2][255]>>2));
    DISP_REG_SET(DISP_REG_BLS_GAMMA_BOUNDARY, LastVal);
        
    DISP_REG_SET(DISP_REG_BLS_LUT_UPDATE, 0);
    //DISP_REG_SET(DISP_REG_BLS_EN, regValue);
}

void disp_bls_update_pwm_lut(void)
{
    int i, j;
    unsigned int regValue;

    DISP_MSG("disp_bls_update_pwm_lut!\n");
    
    // program SRAM
    regValue = DISP_REG_GET(DISP_REG_BLS_EN);
    if (regValue & 0x1) {
        DISP_ERR("update PWM LUT while BLS func enabled!\n");
        disp_dump_reg(DISP_MODULE_BLS);
    }
    //DISP_REG_SET(DISP_REG_BLS_EN, (regValue & 0x80000000));
    DISP_REG_SET(DISP_REG_BLS_LUT_UPDATE, 0x4);

    for (i = 0; i < PWM_LUT_ENTRY; i++)
    {
        //select LUT row index 
        DISP_REG_SET(DISP_REG_BLS_PWM_LUT_SEL, i);
        for (j = 0; j < PWM_LUT_ENTRY; j++)
        {            
            DISP_REG_SET(DISP_REG_BLS_PWM_LUT(j), g_pwm_lut.entry[i][j]);
            DISP_DBG("[%d][%d] PWM LUT = 0x%x (%lu)\n", i, j, DISP_REG_GET(DISP_REG_BLS_PWM_LUT(j)), g_pwm_lut.entry[i][j]);
        }
    }
        
    DISP_REG_SET(DISP_REG_BLS_LUT_UPDATE, 0);
    //DISP_REG_SET(DISP_REG_BLS_EN, regValue);
}

void disp_bls_init(unsigned int srcWidth, unsigned int srcHeight)
{       
    DISP_MSG("disp_bls_init : srcWidth = %d, srcHeight = %d\n", srcWidth, srcHeight);
    
    DISP_REG_SET(DISP_REG_BLS_SRC_SIZE, (srcHeight << 16) | srcWidth);
    DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, DISP_REG_GET(DISP_REG_BLS_PWM_DUTY));
    DISP_REG_SET(DISP_REG_BLS_PWM_CON, 0x00050024);
    DISP_REG_SET(DISP_REG_BLS_PWM_DUTY_GAIN, 0x00000100);
    DISP_REG_SET(DISP_REG_BLS_BLS_SETTING, 0x0);
    DISP_REG_SET(DISP_REG_BLS_INTEN, 0xF);

    disp_bls_update_gamma_lut();
    disp_bls_update_pwm_lut();
    
    // Dithering
    DISP_REG_SET(DISP_REG_BLS_DITHER(0), 0x00000001);
    DISP_REG_SET(DISP_REG_BLS_DITHER(6), 0x00000000);
    DISP_REG_SET(DISP_REG_BLS_DITHER(13), 0x00000222);
    DISP_REG_SET(DISP_REG_BLS_DITHER(14), 0x00000000);
    DISP_REG_SET(DISP_REG_BLS_DITHER(15), 0x22220001);
    DISP_REG_SET(DISP_REG_BLS_DITHER(16), 0x22222222);
    DISP_REG_SET(DISP_REG_BLS_DITHER(17), 0x00000000);

    DISP_REG_SET(DISP_REG_BLS_EN, 0x80000000); 			// only enable PWM
    DISP_REG_SET(DISPSYS_BLS_BASE + 0x000C, 0x00000003);	// w/o inverse gamma

    disp_dump_reg(DISP_MODULE_BLS);
}

int disp_bls_config(void)
{
#if !defined(MTK_AAL_SUPPORT)
    if (!clock_is_on(MT_CG_DISP0_BLS) || !gBLSPowerOn)
    {
        DISP_MSG("disp_bls_config: enable clock\n");
        enable_clock(MT_CG_DISP0_LARB2_SMI   , "DDP");
        enable_clock(MT_CG_DISP0_BLS         , "DDP");
        gBLSPowerOn = 1;
    }
    
    DISP_MSG("disp_bls_config : gBLSMutexID = %d\n", gBLSMutexID);
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(gBLSMutexID), 1);
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(gBLSMutexID), 0);
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_MOD(gBLSMutexID), 0x200);    // BLS
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_SOF(gBLSMutexID), 0);        // single mode

    if (disp_bls_get_mutex() == 0)
    {
#if defined(ONE_WIRE_PULSE_COUNTING) 
        g_previous_level = (DISP_REG_GET(DISP_REG_BLS_PWM_CON) & 0x80 > 7) * 0xFF;
        g_previous_wavenum = 0;
        DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, 0x00000080);
        DISP_REG_SET(DISP_REG_BLS_PWM_CON, 0x00050024 | (DISP_REG_GET(DISP_REG_BLS_PWM_CON) & 0x80));    
        DISP_REG_SET(DISP_REG_BLS_EN, 0x00000000);
#else
        DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, DISP_REG_GET(DISP_REG_BLS_PWM_DUTY));
        DISP_REG_SET(DISP_REG_BLS_PWM_CON, 0x00050024);
        DISP_REG_SET(DISP_REG_BLS_EN, 0x80000000);
#endif
        DISP_REG_SET(DISP_REG_BLS_PWM_DUTY_GAIN, 0x00000100);

        if (disp_bls_release_mutex() == 0)
            return 0;
    }
    return -1;
#endif
    return 0;
}

int disp_bls_set_max_backlight(unsigned int level)
{
    mutex_lock(&backlight_mutex);
    DISP_MSG("disp_bls_set_max_backlight: level = %d, current level = %d\n", level, gMaxLevel);
    gMaxLevel = level;
    mutex_unlock(&backlight_mutex);
    return 0;
}

#if !defined(MTK_AAL_SUPPORT)
#if defined(ONE_WIRE_PULSE_COUNTING) 
int disp_bls_set_backlight(unsigned int level)
{
    int ret = 0;
    unsigned int wavenum = 0;
    unsigned int required_wavenum = 0;

    if (!level && !clock_is_on(MT_CG_DISP0_BLS))
        return 0;
    
    mutex_lock(&backlight_mutex);
    disp_bls_config();

    wavenum = 0;
    if (level > 0)
        wavenum = MAX_PWM_WAVENUM - brightness_mapping(level);

    DISP_MSG("disp_bls_set_backlight: level = %d (%d), previous level = %d (%d)\n",
        level, wavenum, g_previous_level, g_previous_wavenum);


    if (level && (!clock_is_on(MT_CG_DISP0_BLS) || !gBLSPowerOn)) 
    {   
        disp_bls_config();
    }
    
    // [Case 1] y => 0
    //          disable PWM, idle value set to low
    // [Case 2] 0 => max
    //          disable PWM, idle value set to high
    // [Case 3] 0 => x or y => x
    //          idle value keep high
    //          disable PWM to reset wavenum
    //          re-enable PWM, set wavenum     

    if (g_previous_level != level)
    {
        DISP_REG_SET(DISP_REG_PWM_WAVE_NUM, 0x0);
        disp_bls_get_mutex();
        if (level == 0)
            DISP_REG_SET(DISP_REG_BLS_PWM_CON, DISP_REG_GET(DISP_REG_BLS_PWM_CON) & ~0x80);
        if (g_previous_level == 0)
            DISP_REG_SET(DISP_REG_BLS_PWM_CON, DISP_REG_GET(DISP_REG_BLS_PWM_CON) | 0x80);
        
        DISP_REG_SET(DISP_REG_BLS_EN, 0x0);
        disp_bls_release_mutex();

        // poll for PWM_SEND_WAVENUM to be clear
        if(disp_poll_for_reg(DISP_REG_PWM_SEND_WAVENUM, 0, 0xFFFFFFFF, POLLING_TIME_OUT))
        {
            DISP_MSG("fail to clear wavenum! PWM_SEND_WAVENUM = %d\n", DISP_REG_GET(DISP_REG_PWM_SEND_WAVENUM));
            ret = -1;
            goto Exit;
        }

        // y => x or 0 => x
        // y > x: change level from high to low,
        // x > y: change level from low to high, rounding to max
        if (g_previous_wavenum > wavenum)
            required_wavenum = (MAX_PWM_WAVENUM - g_previous_wavenum) + wavenum;
        else
            required_wavenum = wavenum - g_previous_wavenum;        

        if (required_wavenum != 0)
        {
            disp_bls_get_mutex();
            
            // re-enable PWM 
            DISP_REG_SET(DISP_REG_BLS_EN, 0x80000000);
            disp_bls_release_mutex();
            DISP_REG_SET(DISP_REG_PWM_WAVE_NUM, required_wavenum);


            // poll for wave num to be generated completely
            if(disp_poll_for_reg(DISP_REG_PWM_SEND_WAVENUM, required_wavenum, 0xFFFFFFFF, POLLING_TIME_OUT))
            {
                DISP_ERR("fail to set wavenum! PWM_SEND_WAVENUM = %d\n", DISP_REG_GET(DISP_REG_PWM_SEND_WAVENUM));
                g_previous_wavenum = DISP_REG_GET(DISP_REG_PWM_SEND_WAVENUM);
                ret = -1;
                goto Exit;
            }
            
            DISP_MSG("send wavenum = %d\n", required_wavenum); 
        }
        
        g_previous_level = level;
        g_previous_wavenum = wavenum;
    }

    if (!level && (clock_is_on(MT_CG_DISP0_BLS) && gBLSPowerOn)) 
    {
        DISP_MSG("disp_bls_set_backlight: disable clock\n");
        disable_clock(MT_CG_DISP0_BLS         , "DDP");
        disable_clock(MT_CG_DISP0_LARB2_SMI   , "DDP");
        gBLSPowerOn = 0;
    }

Exit:
    mutex_unlock(&backlight_mutex);
    return ret;    
}
#else
int disp_bls_set_backlight(unsigned int level)
{
    DISP_MSG("disp_bls_set_backlight: %d, gBLSPowerOn = %d\n", level, gBLSPowerOn);

    if (!level && !clock_is_on(MT_CG_DISP0_BLS))
        return 0;

    mutex_lock(&backlight_mutex);

    if (level && (!clock_is_on(MT_CG_DISP0_BLS) || !gBLSPowerOn)) 
    {   
        disp_bls_config();
    }

    disp_bls_get_mutex();
    DISP_REG_SET(DISP_REG_BLS_PWM_DUTY, brightness_mapping(level));
    disp_bls_release_mutex();

    if (!level && (clock_is_on(MT_CG_DISP0_BLS) && gBLSPowerOn)) 
    {
        DISP_MSG("disp_bls_set_backlight: disable clock\n");
        disable_clock(MT_CG_DISP0_BLS         , "DDP");
        disable_clock(MT_CG_DISP0_LARB2_SMI   , "DDP");
        gBLSPowerOn = 0;
    }

    mutex_unlock(&backlight_mutex);
    return 0;    
}
#endif
#else
int disp_bls_set_backlight(unsigned int level)
{
    DISP_AAL_PARAM *param;
    DISP_MSG("disp_bls_set_backlight: %d\n", level);

    mutex_lock(&backlight_mutex);
    disp_aal_lock();
    param = get_aal_config();
    param->pwmDuty = brightness_mapping(level);
    disp_aal_unlock();
    mutex_unlock(&backlight_mutex);
    return 0;
}
#endif

