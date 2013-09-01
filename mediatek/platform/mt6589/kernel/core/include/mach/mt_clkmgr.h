#ifndef _MT_CLKMGR_H
#define _MT_CLKMGR_H

#include <linux/list.h>
#include "mach/mt_reg_base.h"
#include "mach/mt_typedefs.h"

#define APMIXED_BASE        0xF0209000
#define AP_PLL_CON0         (APMIXED_BASE + 0x0000)
#define AP_PLL_CON1         (APMIXED_BASE + 0x0004)
#define AP_PLL_CON2         (APMIXED_BASE + 0x0008)
#define AP_PLL_CON3         (APMIXED_BASE + 0x000C)

#define PLL_HP_CON0         (APMIXED_BASE + 0x0014)

#define ARMPLL_CON0         (APMIXED_BASE + 0x0200)
#define ARMPLL_CON1         (APMIXED_BASE + 0x0204)
#define ARMPLL_CON2         (APMIXED_BASE + 0x0208)
#define ARMPLL_PWR_CON0     (APMIXED_BASE + 0x0218)

#define MAINPLL_CON0        (APMIXED_BASE + 0x021C)
#define MAINPLL_CON1        (APMIXED_BASE + 0x0220)
#define MAINPLL_CON2        (APMIXED_BASE + 0x0224)
#define MAINPLL_PWR_CON0    (APMIXED_BASE + 0x0234)

#define UNIVPLL_CON0        (APMIXED_BASE + 0x0238)
#define MMPLL_CON0          (APMIXED_BASE + 0x0240)
#define ISPPLL_CON0         (APMIXED_BASE + 0x0248)


#define MSDCPLL_CON0        (APMIXED_BASE + 0x0250)
#define MSDCPLL_CON1        (APMIXED_BASE + 0x0254)
#define MSDCPLL_CON2        (APMIXED_BASE + 0x0258)
#define MSDCPLL_PWR_CON0    (APMIXED_BASE + 0x0268)


#define TVDPLL_CON0         (APMIXED_BASE + 0x026C)
#define TVDPLL_CON1         (APMIXED_BASE + 0x0270)
#define TVDPLL_CON2         (APMIXED_BASE + 0x0274)
#define TVDPLL_CON3         (APMIXED_BASE + 0x0278)
#define TVDPLL_PWR_CON0     (APMIXED_BASE + 0x0284)

#define LVDSPLL_CON0        (APMIXED_BASE + 0x0288)
#define LVDSPLL_CON1        (APMIXED_BASE + 0x028C)
#define LVDSPLL_CON2        (APMIXED_BASE + 0x0290)
#define LVDSPLL_CON3        (APMIXED_BASE + 0x0294)
#define LVDSPLL_PWR_CON0    (APMIXED_BASE + 0x02A0)

#define CLK_CFG_0           (TOPRGU_BASE + 0x0140)
#define CLK_CFG_1           (TOPRGU_BASE + 0x0144)
#define CLK_CFG_2           (TOPRGU_BASE + 0x0148)
#define CLK_CFG_3           (TOPRGU_BASE + 0x014C)
#define CLK_CFG_4           (TOPRGU_BASE + 0x0150)
#define CLK_CFG_5           (TOPRGU_BASE + 0x0154)
#define CLK_CFG_6           (TOPRGU_BASE + 0x0158)
#define CLK_CFG_7           (TOPRGU_BASE + 0x015C)
#define CLK_MISC_CFG_2      (TOPRGU_BASE + 0x0160)
#define CLK_CFG_8           (TOPRGU_BASE + 0x0164)

#define TOPCK_PDN_SET       (TOPRGU_BASE + 0x0170)
#define TOPCK_PDN_CLR       (TOPRGU_BASE + 0x0174)
#define TOPCK_PDN_STA       (TOPRGU_BASE + 0x0178)

#define INFRA_PDN_SET       (INFRACFG_BASE + 0x0040)
#define INFRA_PDN_CLR       (INFRACFG_BASE + 0x0044)
#define INFRA_PDN_STA       (INFRACFG_BASE + 0x0048)

#define TOPAXI_SI0_CTL      (INFRACFG_BASE + 0x0200)

#define PERI_PDN0_SET       (PERICFG_BASE + 0x0008)
#define PERI_PDN0_CLR       (PERICFG_BASE + 0x0010)
#define PERI_PDN0_STA       (PERICFG_BASE + 0x0018)

#define PERI_PDN1_SET       (PERICFG_BASE + 0x000C)
#define PERI_PDN1_CLR       (PERICFG_BASE + 0x0014)
#define PERI_PDN1_STA       (PERICFG_BASE + 0x001C)

#define PERI_PDN0_MD1_SET   (PERICFG_BASE + 0x0020)
#define PERI_PDN0_MD2_SET   (PERICFG_BASE + 0x0024)
#define PERI_PDN0_MD1_CLR   (PERICFG_BASE + 0x0028)
#define PERI_PDN0_MD2_CLR   (PERICFG_BASE + 0x002C)
#define PERI_PDN0_MD1_STA   (PERICFG_BASE + 0x0030)
#define PERI_PDN0_MD2_STA   (PERICFG_BASE + 0x0034)
#define PERI_PDN_MD_MASK    (PERICFG_BASE + 0x0038)

#define AUDIO_TOP_CON0      (0xF2070000)

#define MFG_CG_CON          (0xF0206000)
#define MFG_CG_SET          (0xF0206004)
#define MFG_CG_CLR          (0xF0206008)

#define DISP_CG_CON0        (DISPSYS_BASE + 0x0100)
#define DISP_CG_SET0        (DISPSYS_BASE + 0x0104)
#define DISP_CG_CLR0        (DISPSYS_BASE + 0x0108)

#define DISP_CG_CON1        (DISPSYS_BASE + 0x0110)
#define DISP_CG_SET1        (DISPSYS_BASE + 0x0114)
#define DISP_CG_CLR1        (DISPSYS_BASE + 0x0118)

#define IMG_CG_CON          (IMGSYS_CONFG_BASE + 0x0000)
#define IMG_CG_SET          (IMGSYS_CONFG_BASE + 0x0004)
#define IMG_CG_CLR          (IMGSYS_CONFG_BASE + 0x0008)

#define VDEC_CKEN_SET       (VDEC_GCON_BASE + 0x0000)
#define VDEC_CKEN_CLR       (VDEC_GCON_BASE + 0x0004)

#define LARB_CKEN_SET       (VDEC_GCON_BASE + 0x0008)
#define LARB_CKEN_CLR       (VDEC_GCON_BASE + 0x000C)

#define VENCSYS_CG_CON      (VENC_TOP_BASE + 0x0000)
#define VENCSYS_CG_SET      (VENC_TOP_BASE + 0x0004)
#define VENCSYS_CG_CLR      (VENC_TOP_BASE + 0x0008)

enum {
    CG_PERI0 = 0,
    CG_PERI1 = 1,
    CG_INFRA = 2,
    CG_TOPCK = 3,
    CG_DISP0 = 4,
    CG_DISP1 = 5,
    CG_IMAGE = 6,
    CG_MFG   = 7,
    CG_AUDIO = 8,
    CG_VDEC0 = 9,
    CG_VDEC1 = 10,
    CG_VENC  = 11,
    NR_GRPS  = 12, 
};

enum {
    MT_CG_PERI0_NFI                 = 0,
    MT_CG_PERI0_THERM               = 1,
    MT_CG_PERI0_PWM1                = 2,
    MT_CG_PERI0_PWM2                = 3,
    MT_CG_PERI0_PWM3                = 4,
    MT_CG_PERI0_PWM4                = 5,
    MT_CG_PERI0_PWM5                = 6,
    MT_CG_PERI0_PWM6                = 7,
    MT_CG_PERI0_PWM7                = 8,
    MT_CG_PERI0_PWM                 = 9,
    MT_CG_PERI0_USB0                = 10,
    MT_CG_PERI0_USB1                = 11,
    MT_CG_PERI0_APDMA               = 12,
    MT_CG_PERI0_MSDC0               = 13,
    MT_CG_PERI0_MSDC1               = 14,
    MT_CG_PERI0_MSDC2               = 15,
    MT_CG_PERI0_MSDC3               = 16,
    MT_CG_PERI0_MSDC4               = 17,
    MT_CG_PERI0_APHIF               = 18,
    MT_CG_PERI0_MDHIF               = 19,
    MT_CG_PERI0_NLI                 = 20,
    MT_CG_PERI0_IRDA                = 21,
    MT_CG_PERI0_UART0               = 22,
    MT_CG_PERI0_UART1               = 23,
    MT_CG_PERI0_UART2               = 24,
    MT_CG_PERI0_UART3               = 25,
    MT_CG_PERI0_I2C0                = 26,
    MT_CG_PERI0_I2C1                = 27,
    MT_CG_PERI0_I2C2                = 28,
    MT_CG_PERI0_I2C3                = 29,
    MT_CG_PERI0_I2C4                = 30,
    MT_CG_PERI0_I2C5                = 31,
    
    MT_CG_PERI1_I2C6                = 32,
    MT_CG_PERI1_WRAP                = 33,
    MT_CG_PERI1_AUXADC              = 34,
    MT_CG_PERI1_SPI1                = 35,
    MT_CG_PERI1_FHCTL               = 36,

    MT_CG_INFRA_DBGCLK              = 64,
    MT_CG_INFRA_SMI                 = 65,
    MT_CG_INFRA_SPI0                = 66,
    MT_CG_INFRA_AUDIO               = 69,
    MT_CG_INFRA_CEC                 = 70,
    MT_CG_INFRA_MFGAXI              = 71,
    MT_CG_INFRA_M4U                 = 72,
    MT_CG_INFRA_MD1MCUAXI           = 73,
    MT_CG_INFRA_MD1HWMIXAXI         = 74,
    MT_CG_INFRA_MD1AHB              = 75,
    MT_CG_INFRA_MD2MCUAXI           = 76,
    MT_CG_INFRA_MD2HWMIXAXI         = 77,
    MT_CG_INFRA_MD2AHB              = 78,
    MT_CG_INFRA_CPUM                = 79,
    MT_CG_INFRA_KP                  = 80,
    MT_CG_INFRA_CCIF0               = 84,
    MT_CG_INFRA_CCIF1               = 85,
    MT_CG_INFRA_PMICSPI             = 86,
    MT_CG_INFRA_PMICWRAP            = 87,

    MT_CG_TOPCK_PMICSPI             = 96,

    MT_CG_DISP0_LARB2_SMI           = 128,
    MT_CG_DISP0_ROT_ENGINE          = 129,
    MT_CG_DISP0_ROT_SMI             = 130,
    MT_CG_DISP0_SCL                 = 131,
    MT_CG_DISP0_OVL_ENGINE          = 132,
    MT_CG_DISP0_OVL_SMI             = 133,
    MT_CG_DISP0_COLOR               = 134,
    MT_CG_DISP0_2DSHP               = 135,
    MT_CG_DISP0_BLS                 = 136,
    MT_CG_DISP0_WDMA0_ENGINE        = 137,
    MT_CG_DISP0_WDMA0_SMI           = 138,
    MT_CG_DISP0_WDMA1_ENGINE        = 139,
    MT_CG_DISP0_WDMA1_SMI           = 140,
    MT_CG_DISP0_RDMA0_ENGINE        = 141,
    MT_CG_DISP0_RDMA0_SMI           = 142,
    MT_CG_DISP0_RDMA0_OUTPUT        = 143,
    MT_CG_DISP0_RDMA1_ENGINE        = 144,
    MT_CG_DISP0_RDMA1_SMI           = 145,
    MT_CG_DISP0_RDMA1_OUTPUT        = 146,
    MT_CG_DISP0_GAMMA_ENGINE        = 147,
    MT_CG_DISP0_GAMMA_PIXEL         = 148,
    MT_CG_DISP0_CMDQ_ENGINE         = 149,
    MT_CG_DISP0_CMDQ_SMI            = 150,
    MT_CG_DISP0_G2D_ENGINE          = 151,
    MT_CG_DISP0_G2D_SMI             = 152,

    MT_CG_DISP1_DBI_ENGINE          = 160,
    MT_CG_DISP1_DBI_SMI             = 161,
    MT_CG_DISP1_DBI_OUTPUT          = 162,
    MT_CG_DISP1_DSI_ENGINE          = 163,
    MT_CG_DISP1_DSI_DIGITAL         = 164,
    MT_CG_DISP1_DSI_DIGITAL_LANE    = 165,
    MT_CG_DISP1_DPI0                = 166,
    MT_CG_DISP1_DPI1                = 167,
    MT_CG_DISP1_LCD                 = 168,
    MT_CG_DISP1_SLCD                = 169,

    MT_CG_IMAGE_LARB3_SMI           = 192,
    MT_CG_IMAGE_LARB4_SMI           = 194,
    MT_CG_IMAGE_COMMN_SMI           = 196,
    MT_CG_IMAGE_CAM_SMI             = 197,
    MT_CG_IMAGE_CAM_CAM             = 198,
    MT_CG_IMAGE_SEN_TG              = 199, 
    MT_CG_IMAGE_SEN_CAM             = 200,
    MT_CG_IMAGE_JPGD_SMI            = 201,
    MT_CG_IMAGE_JPGD_JPG            = 202,
    MT_CG_IMAGE_JPGE_SMI            = 203,
    MT_CG_IMAGE_JPGE_JPG            = 204,
    MT_CG_IMAGE_FPC                 = 205, 

    MT_CG_MFG_AXI                   = 224,
    MT_CG_MFG_MEM                   = 225,
    MT_CG_MFG_G3D                   = 226,
    MT_CG_MFG_HYD                   = 227,

    MT_CG_AUDIO_AFE                 = 258,
    MT_CG_AUDIO_I2S                 = 262,
    
    MT_CG_VDEC0_VDE                 = 288,
    
    MT_CG_VDEC1_SMI                 = 320,

    MT_CG_VENC_VEN                  = 352,

    CG_PERI0_FROM                   = MT_CG_PERI0_NFI,
    CG_PERI0_TO                     = MT_CG_PERI0_I2C5,
    NR_PERI0_CLKS                   = 32,

    CG_PERI1_FROM                   = MT_CG_PERI1_I2C6,
    CG_PERI1_TO                     = MT_CG_PERI1_FHCTL,
    NR_PERI1_CLKS                   = 5,

    CG_INFRA_FROM                   = MT_CG_INFRA_DBGCLK,
    CG_INFRA_TO                     = MT_CG_INFRA_PMICWRAP,
    NR_INFRA_CLKS                   = 19,

    CG_TOPCK_FROM                   = MT_CG_TOPCK_PMICSPI,
    CG_TOPCK_TO                     = MT_CG_TOPCK_PMICSPI,
    NR_TOPCK_CLKS                   = 1,

    CG_DISP0_FROM                   = MT_CG_DISP0_LARB2_SMI,
    CG_DISP0_TO                     = MT_CG_DISP0_G2D_SMI,
    NR_DISP0_CLKS                   = 25,

    CG_DISP1_FROM                   = MT_CG_DISP1_DBI_ENGINE,
    CG_DISP1_TO                     = MT_CG_DISP1_SLCD,
    NR_DISP1_CLKS                   = 10,

    CG_IMAGE_FROM                   = MT_CG_IMAGE_LARB3_SMI,
    CG_IMAGE_TO                     = MT_CG_IMAGE_FPC,
    NR_IMAGE_CLKS                   = 12,

    CG_MFG_FROM                     = MT_CG_MFG_AXI,
    CG_MFG_TO                       = MT_CG_MFG_HYD,
    NR_MFG_CLKS                     = 4,

    CG_AUDIO_FROM                   = MT_CG_AUDIO_AFE,
    CG_AUDIO_TO                     = MT_CG_AUDIO_I2S,
    NR_AUDIO_CLKS                   = 2,

    CG_VDEC0_FROM                   = MT_CG_VDEC0_VDE,
    CG_VDEC0_TO                     = MT_CG_VDEC0_VDE,
    NR_VDEC0_CLKS                   = 1,

    CG_VDEC1_FROM                   = MT_CG_VDEC1_SMI,
    CG_VDEC1_TO                     = MT_CG_VDEC1_SMI,
    NR_VDEC1_CLKS                   = 1,

    CG_VENC_FROM                    = MT_CG_VENC_VEN,
    CG_VENC_TO                      = MT_CG_VENC_VEN,
    NR_VENC_CLKS                    = 1,

    NR_CLKS                         = 353,
};

#define UNIVPLL2_D4     4

enum {
    //CLK_CFG_0
    MT_MUX_MFG          = 0,
    MT_MUX_IRDA         = 1,

    //CLK_CFG_1
    MT_MUX_CAM          = 2,
    MT_MUX_AUDINTBUS    = 3,
    MT_MUX_JPG          = 4,
    MT_MUX_DISP         = 5,

    //CLK_CFG_2
    MT_MUX_MSDC1        = 6,
    MT_MUX_MSDC2        = 7,
    MT_MUX_MSDC3        = 8,
    MT_MUX_MSDC4        = 9,

    //CLK_CFG_3
    MT_MUX_USB20        = 10,

    //CLK_CFG_4
    MT_MUX_HYD          = 11,
    MT_MUX_VENC         = 12,
    MT_MUX_SPI          = 13,
    MT_MUX_UART         = 14,

    //CLK_CFG_6
    MT_MUX_CAMTG        = 15,
    /* MT_MUX_FD           = 16, */
    MT_MUX_AUDIO        = 16,

    //CLK_CFG_7
    MT_MUX_VDEC         = 17,
    MT_MUX_DPILVDS      = 18,

    //CLK_CFG_8
    MT_MUX_PMICSPI      = 19,
    MT_MUX_MSDC0        = 20,
    MT_MUX_SMI_MFG_AS   = 21,

    NR_MUXS             = 22,
};

enum {
    ARMPLL  = 0,
    MAINPLL = 1,
    MSDCPLL = 2,
    TVDPLL  = 3,
    LVDSPLL = 4,
    UNIVPLL = 5,
    MMPLL   = 6,
    ISPPLL  = 7,
    NR_PLLS = 8,
};

enum {
    SYS_MD1 = 0,
    SYS_MD2 = 1,
    SYS_DPY = 2,
    SYS_DIS = 3,
    SYS_MFG = 4,
    SYS_ISP = 5,
    SYS_IFR = 6,
    SYS_VEN = 7,
    SYS_VDE = 8,
    NR_SYSS = 9,
};

enum {
    MT_LARB0 = 0,
    MT_LARB1 = 1,
    MT_LARB2 = 2,
    MT_LARB3 = 3,
    MT_LARB4 = 4,
    NR_LARBS = 5,
};

/* larb monitor mechanism definition*/
enum {
    LARB_MONITOR_LEVEL_HIGH     = 10,
    LARB_MONITOR_LEVEL_MEDIUM   = 20,
    LARB_MONITOR_LEVEL_LOW      = 30,
};

struct larb_monitor {
    struct list_head link;
    int level;
    void (*backup)(struct larb_monitor *h, int larb_idx);       /* called before disable larb clock */
    void (*restore)(struct larb_monitor *h, int larb_idx);      /* called after enable larb clock */
};

extern void register_larb_monitor(struct larb_monitor *handler);
extern void unregister_larb_monitor(struct larb_monitor *handler);

/* clock API */
extern int enable_clock(int id, char *mod_name);
extern int disable_clock(int id, char *mod_name);

extern int enable_clock_ext_locked(int id, char *mod_name);
extern int disable_clock_ext_locked(int id, char *mod_name);

extern int clock_is_on(int id);

extern int clkmux_sel(int id, unsigned int clksrc, char *name);

extern void clk_set_force_on(int id);
extern void clk_clr_force_on(int id);
extern int clk_is_force_on(int id);

/* pll API */
extern int enable_pll(int id, char *mod_name);
extern int disable_pll(int id, char *mod_name);

extern int pll_hp_switch_on(int id, int hp_on);
extern int pll_hp_switch_off(int id, int hp_off);

extern int pll_fsel(int id, unsigned int value);
extern int pll_is_on(int id);

/* subsys API */
extern int enable_subsys(int id, char *mod_name);
extern int disable_subsys(int id, char *mod_name);

extern int subsys_is_on(int id);
extern int md_power_on(int id);
extern int md_power_off(int id, unsigned int timeout);

/* other API */
extern void enable_clksq2(void);
extern void disable_clksq2(void);

extern void clksq2_sw2hw(void);
extern void clksq2_hw2sw(void);

extern void pmicspi_mempll2clksq(void);
extern void pmicspi_clksq2mempll(void);

extern int get_gpu_power_src(void);

const char* grp_get_name(int id);

extern int clkmgr_is_locked(void);

/* init */
extern void mt_clkmgr_init(void);


#endif
