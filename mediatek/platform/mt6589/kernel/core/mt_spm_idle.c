#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/delay.h>  

#include <mach/irqs.h>
#include <mach/mt_spm.h>
#include <mach/mt_spm_idle.h>
#include <mach/mt_dormant.h>
#include <mach/mt_gpt.h>
#include <mach/mt_reg_base.h>
#include <mach/env.h> // require from hibboot flag

#include <asm/hardware/gic.h>

//===================================

#ifdef SPM_MCDI_FUNC
    #define MCDI_KICK_PCM 1
#else
    #define MCDI_KICK_PCM 0
#endif

#define SPM_MCDI_BYPASS_SYSPWREQ 1
u32 SPM_MCDI_CORE_SEL = 1;   // MCDI core wfi sel (dynamic MCDI switch)

// ===================================

DEFINE_SPINLOCK(spm_sodi_lock);

u32 En_SPM_MCDI = 0;  // flag for idle task
s32 spm_sodi_disable_counter = 0;
u32 MCDI_Test_Mode = 0;

static struct task_struct *mcdi_task_0;
static struct task_struct *mcdi_task_1;
static struct task_struct *mcdi_task_2;
static struct task_struct *mcdi_task_3;

#define WFI_OP        4
#define WFI_L2C      5
#define WFI_SCU      6
#define WFI_MM      16
#define WFI_MD      19

#define mcdi_wfi_with_sync()                         \
do {                                            \
    isb();                                      \
    dsb();                                      \
    __asm__ __volatile__("wfi" : : : "memory"); \
} while (0)

#define read_cntp_cval(cntp_cval_lo, cntp_cval_hi) \
do {    \
    __asm__ __volatile__(   \
    "MRRC p15, 2, %0, %1, c14\n"    \
    :"=r"(cntp_cval_lo), "=r"(cntp_cval_hi) \
    :   \
    :"memory"); \
} while (0)

#define write_cntp_cval(cntp_cval_lo, cntp_cval_hi) \
do {    \
    __asm__ __volatile__(   \
    "MCRR p15, 2, %0, %1, c14\n"    \
    :   \
    :"r"(cntp_cval_lo), "r"(cntp_cval_hi));    \
} while (0)

#define write_cntp_ctl(cntp_ctl)  \
do {    \
    __asm__ __volatile__(   \
    "MCR p15, 0, %0, c14, c2, 1\n"  \
    :   \
    :"r"(cntp_ctl)); \
} while (0)

// ==========================================
// PCM code for MCDI (Multi Core Deep Idle)  v4.5 IPI 2013/01/08
//
// core 0 : local timer
// core 1 : GPT 1
// core 2 : GPT 4
// core 3 : GPT 5
// ==========================================
static u32 spm_pcm_mcdi[] = {

    0x10007c1f, 0x19c0001f, 0x00204800, 0x19c0001f, 0x00204800, 0x1880001f,
    0x100041dc, 0x18c0001f, 0x10004044, 0x1a10001f, 0x100041dc, 0x1a50001f,
    0x10004044, 0xba008008, 0xff00ffff, 0x00660000, 0xba408009, 0x00ffffff,
    0x9f000000, 0xe0800008, 0xe0c00009, 0xa1d80407, 0x1b00001f, 0xbffff7ff,
    0xf0000000, 0x17c07c1f, 0x1b00001f, 0x3fffe7ff, 0x1b80001f, 0x20000004,
    0x8090840d, 0xb092044d, 0xd80009cc, 0x17c07c1f, 0x1b00001f, 0xbffff7ff,
    0xd80009c2, 0x17c07c1f, 0x1880001f, 0x100041dc, 0x18c0001f, 0x10004044,
    0x1a10001f, 0x100041dc, 0x1a50001f, 0x10004044, 0xba008008, 0xff00ffff,
    0x000a0000, 0xba408009, 0x00ffffff, 0x81000000, 0xe0800008, 0xe0c00009,
    0x1a50001f, 0x10004044, 0x19c0001f, 0x00214820, 0x19c0001f, 0x00204822,
    0x1b80001f, 0x2000000a, 0x18c0001f, 0x10006320, 0xe0c0001f, 0x809a840d,
    0xd8000942, 0x17c07c1f, 0xe0c0000f, 0x18c0001f, 0x10006814, 0xe0c00001,
    0xd82009c2, 0x17c07c1f, 0xa8000000, 0x00000004, 0x1b00001f, 0x7fffefff,
    0xf0000000, 0x17c07c1f, 0x1a50001f, 0x10006400, 0x82570409, 0xd8000be9,
    0x17c07c1f, 0xd8000b4a, 0x17c07c1f, 0xe2e00036, 0xe2e0003e, 0xe2e0002e,
    0xd8200c0a, 0x17c07c1f, 0xe2e0006e, 0xe2e0004e, 0xe2e0004c, 0xe2e0004d,
    0xf0000000, 0x17c07c1f, 0x1a50001f, 0x10006400, 0x82570409, 0xd8000e09,
    0x17c07c1f, 0xd8000d6a, 0x17c07c1f, 0xe2e0006d, 0xe2e0002d, 0xd8200e0a,
    0x17c07c1f, 0xe2e0002f, 0xe2e0003e, 0xe2e00032, 0xf0000000, 0x17c07c1f,
    0x1a50001f, 0x10006400, 0x82570409, 0xd8000f89, 0x17c07c1f, 0x12007c1f,
    0xa210a001, 0xe2c00008, 0xd8000f0a, 0x02a0040a, 0xf0000000, 0x17c07c1f,
    0x1a50001f, 0x10006400, 0x82570409, 0xd8001129, 0x17c07c1f, 0x1a00001f,
    0xffffffff, 0x1210a01f, 0xe2c00008, 0xd80010aa, 0x02a0040a, 0xf0000000,
    0x17c07c1f, 0x1a10001f, 0x10006608, 0xa24e3401, 0x82c02408, 0xf0000000,
    0x17c07c1f, 0x1a50001f, 0x1000620c, 0x82802401, 0x1180a41f, 0x1a50001f,
    0x10006224, 0x82002401, 0xa290a00a, 0xa180a406, 0x1a50001f, 0x10006228,
    0x82002401, 0xa291200a, 0xa180a406, 0x1a50001f, 0x1000622c, 0x82002401,
    0xa291a00a, 0xa180a406, 0x124e341f, 0xb28024aa, 0x1a10001f, 0x10006400,
    0x8206a001, 0x6a60000a, 0x0000000f, 0x82e02009, 0x82c0040b, 0xf0000000,
    0x17c07c1f, 0x1880001f, 0x100041dc, 0x18c0001f, 0x10004044, 0x1a10001f,
    0x100041dc, 0x1a50001f, 0x10004044, 0xd80017eb, 0x17c07c1f, 0xba008008,
    0xff00ffff, 0x000a0000, 0xba408009, 0x00ffffff, 0x81000000, 0xd82018eb,
    0x17c07c1f, 0xba008008, 0xff00ffff, 0x00660000, 0xba408009, 0x00ffffff,
    0x9f000000, 0xe0800008, 0xe0c00009, 0xf0000000, 0x17c07c1f, 0x820cb401,
    0xd82019e8, 0x17c07c1f, 0xa1d78407, 0xf0000000, 0x17c07c1f, 0x1a10001f,
    0x10008004, 0x82402001, 0x1290a41f, 0x8241a001, 0xa291240a, 0x82422001,
    0xa291a40a, 0x8240a001, 0xa280240a, 0x82412001, 0xa280240a, 0x8242a001,
    0xa280240a, 0x1a40001f, 0x10006600, 0xe240000a, 0xf0000000, 0x17c07c1f,
    0x19c0001f, 0x00204822, 0x18c0001f, 0x10006320, 0xe0c0001f, 0x1b80001f,
    0x20000014, 0x82da840d, 0xd8001e4b, 0x17c07c1f, 0xe0c0000f, 0x18c0001f,
    0x10006814, 0xe0c00001, 0xf0000000, 0x17c07c1f, 0x10007c1f, 0x19c0001f,
    0x00204800, 0x19c0001f, 0x00204800, 0xf0000000, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001,
    0x11407c1f, 0x19c0001f, 0x00215800, 0xc0c01a20, 0x17c07c1f, 0x1b00001f,
    0x3d200011, 0x1b80001f, 0xd0010000, 0x808ab001, 0xc8801962, 0x17c07c1f,
    0xc1001160, 0x11007c1f, 0x80e01404, 0x60a07c05, 0x88900002, 0x10006814,
    0xd8003d62, 0x17c07c1f, 0x81000403, 0xd8202344, 0x17c07c1f, 0xa1400405,
    0x81008c01, 0xd8202584, 0x17c07c1f, 0x1900001f, 0x10006218, 0xc1000c40,
    0x12807c1f, 0x1900001f, 0x10006264, 0x1a80001f, 0x00000010, 0xc1000e40,
    0x17c07c1f, 0x1900001f, 0x10006218, 0xc1000c40, 0x1280041f, 0xa1508405,
    0x81010c01, 0xd82027c4, 0x17c07c1f, 0x1900001f, 0x1000621c, 0xc1000c40,
    0x12807c1f, 0x1900001f, 0x1000626c, 0x1a80001f, 0x00000010, 0xc1000e40,
    0x17c07c1f, 0x1900001f, 0x1000621c, 0xc1000c40, 0x1280041f, 0xa1510405,
    0x81018c01, 0xd8202a04, 0x17c07c1f, 0x1900001f, 0x10006220, 0xc1000c40,
    0x12807c1f, 0x1900001f, 0x10006274, 0x1a80001f, 0x00000010, 0xc1000e40,
    0x17c07c1f, 0x1900001f, 0x10006220, 0xc1000c40, 0x1280041f, 0xa1518405,
    0xd820378c, 0x17c07c1f, 0xc1001160, 0x11007c1f, 0x80c01404, 0x1890001f,
    0x10006600, 0xa0800806, 0x82000c01, 0xd8202d68, 0x17c07c1f, 0x824d3001,
    0xb2403121, 0xb24c3121, 0xb2400921, 0xd8202d69, 0x17c07c1f, 0x89400005,
    0xfffffffe, 0xe8208000, 0x10006f00, 0x00000000, 0xa1d18407, 0x1b80001f,
    0x20000010, 0x89c00007, 0xfffffff7, 0x81008c01, 0xd82030c4, 0x17c07c1f,
    0x810db001, 0xb1008881, 0xb1003081, 0xd82030c4, 0x17c07c1f, 0x1900001f,
    0x10006218, 0xc1000a00, 0x12807c1f, 0x1900001f, 0x10006264, 0x1a80001f,
    0x00000010, 0xc1000fc0, 0x17c07c1f, 0x1900001f, 0x10006218, 0xc1000a00,
    0x1280041f, 0x89400005, 0xfffffffd, 0xe8208000, 0x10006f04, 0x00000000,
    0x81010c01, 0xd8203424, 0x17c07c1f, 0x810e3001, 0xb1010881, 0xb1003081,
    0xd8203424, 0x17c07c1f, 0x1900001f, 0x1000621c, 0xc1000a00, 0x12807c1f,
    0x1900001f, 0x1000626c, 0x1a80001f, 0x00000010, 0xc1000fc0, 0x17c07c1f,
    0x1900001f, 0x1000621c, 0xc1000a00, 0x1280041f, 0x89400005, 0xfffffffb,
    0xe8208000, 0x10006f08, 0x00000000, 0x81018c01, 0xd8203784, 0x17c07c1f,
    0x810eb001, 0xb1018881, 0xb1003081, 0xd8203784, 0x17c07c1f, 0x1900001f,
    0x10006220, 0xc1000a00, 0x12807c1f, 0x1900001f, 0x10006274, 0x1a80001f,
    0x00000010, 0xc1000fc0, 0x17c07c1f, 0x1900001f, 0x10006220, 0xc1000a00,
    0x1280041f, 0x89400005, 0xfffffff7, 0xe8208000, 0x10006f0c, 0x00000000,
    0xc0801220, 0x10807c1f, 0xd8202062, 0x17c07c1f, 0x1b00001f, 0x7fffefff,
    0x1b80001f, 0xd0010000, 0x8098840d, 0x80d0840d, 0xb0d2046d, 0xa0800c02,
    0xd8203a02, 0x17c07c1f, 0x8880000c, 0x3d200011, 0xd8002062, 0x17c07c1f,
    0xd0003800, 0x17c07c1f, 0xe8208000, 0x10006310, 0x0b1600f8, 0xc10015e0,
    0x11007c1f, 0x19c0001f, 0x00214820, 0xc0801c80, 0x10807c1f, 0xd8202062,
    0x17c07c1f, 0xa8000000, 0x00000004, 0x1b00001f, 0x7fffefff, 0x1b80001f,
    0x90100000, 0x80810001, 0xd8202062, 0x17c07c1f, 0xc1001e80, 0x17c07c1f,
    0xc10015e0, 0x1100041f, 0xa1d80407, 0xd0002060, 0x17c07c1f, 0xc10015e0,
    0x1100041f, 0x19c0001f, 0x00215800, 0x10007c1f, 0xf0000000
    
};

#define PCM_MCDI_BASE            __pa(spm_pcm_mcdi)
#define PCM_MCDI_LEN              ( 497 - 1 )
#define MCDI_pcm_pc_0      0
#define MCDI_pcm_pc_1      26
#define MCDI_pcm_pc_2      MCDI_pcm_pc_0
#define MCDI_pcm_pc_3      MCDI_pcm_pc_1

extern void mtk_wdt_suspend(void);
extern void mtk_wdt_resume(void);
extern int mt_irq_mask_all(struct mtk_irq_mask *mask);
extern int mt_irq_mask_restore(struct mtk_irq_mask *mask);
extern int mt_SPI_mask_all(struct mtk_irq_mask *mask);
extern int mt_SPI_mask_restore(struct mtk_irq_mask *mask);
extern int mt_PPI_mask_all(struct mtk_irq_mask *mask);
extern int mt_PPI_mask_restore(struct mtk_irq_mask *mask);
extern void mt_irq_mask_for_sleep(unsigned int irq);
extern void mt_irq_unmask_for_sleep(unsigned int irq);
extern void mt_cirq_enable(void);
extern void mt_cirq_disable(void);
extern void mt_cirq_clone_gic(void);
extern void mt_cirq_flush(void);
extern void mt_cirq_mask(unsigned int cirq_num);
extern void mt_cirq_mask_all(void);


extern spinlock_t spm_lock;

//static struct mtk_irq_mask MCDI_cpu_irq_mask;
//static struct mtk_irq_mask MCDI_cpu_1_PPI_mask;
//static struct mtk_irq_mask MCDI_cpu_2_PPI_mask;
//static struct mtk_irq_mask MCDI_cpu_3_PPI_mask;

static void spm_mcdi_dump_regs(void)
{
    #if 0
    /* PLL register */
    clc_notice("AP_PLL_CON0     0x%x = 0x%x\n", AP_PLL_CON0                , spm_read(AP_PLL_CON0));
    clc_notice("AP_PLL_CON1     0x%x = 0x%x\n", AP_PLL_CON1                , spm_read(AP_PLL_CON1));
    clc_notice("AP_PLL_CON2     0x%x = 0x%x\n", AP_PLL_CON2                , spm_read(AP_PLL_CON2));
    clc_notice("MMPLL_CON0      0x%x = 0x%x\n", MMPLL_CON0                 , spm_read(MMPLL_CON0));
    clc_notice("ISPPLL_CON0     0x%x = 0x%x\n", ISPPLL_CON0                , spm_read(ISPPLL_CON0));
    clc_notice("MSDCPLL_CON0    0x%x = 0x%x\n", MSDCPLL_CON0               , spm_read(MSDCPLL_CON0));
    clc_notice("MSDCPLL_PWR_CON 0x%x = 0x%x\n", MSDCPLL_PWR_CON0           , spm_read(MSDCPLL_PWR_CON0));
    clc_notice("TVDPLL_CON0     0x%x = 0x%x\n", TVDPLL_CON0                , spm_read(TVDPLL_CON0));
    clc_notice("TVDPLL_PWR_CON  0x%x = 0x%x\n", TVDPLL_PWR_CON0            , spm_read(TVDPLL_PWR_CON0));
    clc_notice("LVDSPLL_CON0    0x%x = 0x%x\n", LVDSPLL_CON0               , spm_read(LVDSPLL_CON0));
    clc_notice("LVDSPLL_PWR_CON 0x%x = 0x%x\n", LVDSPLL_PWR_CON0           , spm_read(LVDSPLL_PWR_CON0));

    /* INFRA/PERICFG register */
    clc_notice("INFRA_PDN_STA   0x%x = 0x%x\n", INFRA_PDN_STA              , spm_read(INFRA_PDN_STA));
    clc_notice("PERI_PDN0_STA   0x%x = 0x%x\n", PERI_PDN0_STA              , spm_read(PERI_PDN0_STA));
    clc_notice("PERI_PDN1_STA   0x%x = 0x%x\n", PERI_PDN1_STA              , spm_read(PERI_PDN1_STA));
    #endif

    /* SPM register */
    clc_notice("POWER_ON_VAL0   0x%x = 0x%x\n", SPM_POWER_ON_VAL0          , spm_read(SPM_POWER_ON_VAL0));
    clc_notice("POWER_ON_VAL1   0x%x = 0x%x\n", SPM_POWER_ON_VAL1          , spm_read(SPM_POWER_ON_VAL1));
    clc_notice("PCM_PWR_IO_EN   0x%x = 0x%x\n", SPM_PCM_PWR_IO_EN          , spm_read(SPM_PCM_PWR_IO_EN));
    clc_notice("PCM_REG0_DATA   0x%x = 0x%x\n", SPM_PCM_REG0_DATA          , spm_read(SPM_PCM_REG0_DATA));
    clc_notice("PCM_REG7_DATA   0x%x = 0x%x\n", SPM_PCM_REG7_DATA          , spm_read(SPM_PCM_REG7_DATA));
    clc_notice("PCM_REG13_DATA  0x%x = 0x%x\n", SPM_PCM_REG13_DATA         , spm_read(SPM_PCM_REG13_DATA));
    clc_notice("CLK_CON         0x%x = 0x%x\n", SPM_CLK_CON                , spm_read(SPM_CLK_CON));
    clc_notice("AP_DVFS_CON     0x%x = 0x%x\n", SPM_AP_DVFS_CON_SET        , spm_read(SPM_AP_DVFS_CON_SET));
    clc_notice("PWR_STATUS      0x%x = 0x%x\n", SPM_PWR_STATUS             , spm_read(SPM_PWR_STATUS));
    clc_notice("PWR_STATUS_S    0x%x = 0x%x\n", SPM_PWR_STATUS_S           , spm_read(SPM_PWR_STATUS_S));
    clc_notice("SLEEP_TIMER_STA 0x%x = 0x%x\n", SPM_SLEEP_TIMER_STA        , spm_read(SPM_SLEEP_TIMER_STA));
    clc_notice("WAKE_EVENT_MASK 0x%x = 0x%x\n", SPM_SLEEP_WAKEUP_EVENT_MASK, spm_read(SPM_SLEEP_WAKEUP_EVENT_MASK));

    // PCM register
    clc_notice("PCM_REG0_DATA   0x%x = 0x%x\n", SPM_PCM_REG0_DATA          , spm_read(SPM_PCM_REG0_DATA));
    clc_notice("PCM_REG1_DATA   0x%x = 0x%x\n", SPM_PCM_REG1_DATA          , spm_read(SPM_PCM_REG1_DATA));
    clc_notice("PCM_REG2_DATA   0x%x = 0x%x\n", SPM_PCM_REG2_DATA          , spm_read(SPM_PCM_REG2_DATA));
    clc_notice("PCM_REG3_DATA   0x%x = 0x%x\n", SPM_PCM_REG3_DATA          , spm_read(SPM_PCM_REG3_DATA));
    clc_notice("PCM_REG4_DATA   0x%x = 0x%x\n", SPM_PCM_REG4_DATA          , spm_read(SPM_PCM_REG4_DATA));
    clc_notice("PCM_REG5_DATA   0x%x = 0x%x\n", SPM_PCM_REG5_DATA          , spm_read(SPM_PCM_REG5_DATA));
    clc_notice("PCM_REG6_DATA   0x%x = 0x%x\n", SPM_PCM_REG6_DATA          , spm_read(SPM_PCM_REG6_DATA));
    clc_notice("PCM_REG7_DATA   0x%x = 0x%x\n", SPM_PCM_REG7_DATA          , spm_read(SPM_PCM_REG7_DATA));
    clc_notice("PCM_REG8_DATA   0x%x = 0x%x\n", SPM_PCM_REG8_DATA          , spm_read(SPM_PCM_REG8_DATA));
    clc_notice("PCM_REG9_DATA   0x%x = 0x%x\n", SPM_PCM_REG9_DATA          , spm_read(SPM_PCM_REG9_DATA));
    clc_notice("PCM_REG10_DATA   0x%x = 0x%x\n", SPM_PCM_REG10_DATA          , spm_read(SPM_PCM_REG10_DATA));
    clc_notice("PCM_REG11_DATA   0x%x = 0x%x\n", SPM_PCM_REG11_DATA          , spm_read(SPM_PCM_REG11_DATA));
    clc_notice("PCM_REG12_DATA   0x%x = 0x%x\n", SPM_PCM_REG12_DATA          , spm_read(SPM_PCM_REG12_DATA));
    clc_notice("PCM_REG13_DATA   0x%x = 0x%x\n", SPM_PCM_REG13_DATA          , spm_read(SPM_PCM_REG13_DATA));
    clc_notice("PCM_REG14_DATA   0x%x = 0x%x\n", SPM_PCM_REG14_DATA          , spm_read(SPM_PCM_REG14_DATA));
    clc_notice("PCM_REG15_DATA   0x%x = 0x%x\n", SPM_PCM_REG15_DATA          , spm_read(SPM_PCM_REG15_DATA));    
}

static void SPM_SW_Reset(void)
{
    //Software reset
    spm_write(SPM_PCM_CON0, (CON0_CFG_KEY | CON0_PCM_SW_RESET | CON0_IM_SLEEP_DVS));
    spm_write(SPM_PCM_CON0, (CON0_CFG_KEY | CON0_IM_SLEEP_DVS));
}

static void SET_PCM_EVENT_VEC(u32 n, u32 event, u32 resume, u32 imme, u32 pc)
{
    switch (n)
    {
        case 0 : spm_write(SPM_PCM_EVENT_VECTOR0, EVENT_VEC(event, resume, imme, pc)); break;
        case 1 : spm_write(SPM_PCM_EVENT_VECTOR1, EVENT_VEC(event, resume, imme, pc)); break;
        case 2 : spm_write(SPM_PCM_EVENT_VECTOR2, EVENT_VEC(event, resume, imme, pc)); break;
        case 3 : spm_write(SPM_PCM_EVENT_VECTOR3, EVENT_VEC(event, resume, imme, pc)); break;
        default : break;
    }
}

static void PCM_Init(void)
{
    //Init PCM r0
    spm_write(SPM_PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL0));
    spm_write(SPM_PCM_PWR_IO_EN, PCM_RF_SYNC_R0);
    spm_write(SPM_PCM_PWR_IO_EN, 0);
       
    //Init PCM r7
    spm_write(SPM_PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL1));
    spm_write(SPM_PCM_PWR_IO_EN, PCM_RF_SYNC_R7);
    spm_write(SPM_PCM_PWR_IO_EN, 0);
    
    // set SPM_PCM_REG_DATA_INI
    spm_write(SPM_PCM_REG_DATA_INI, 0);
        
    // set AP STANBY CONTROL
    spm_write(SPM_AP_STANBY_CON, ((0x0<<WFI_OP) | (0x1<<WFI_L2C) | (0x1<<WFI_SCU) | (0x7<<WFI_MM) | (0x3<<WFI_MD)));  // operand or, mask l2c, mask scu, 16~18: MM, 19~20:MD
    spm_write(SPM_CORE0_CON, 0x0); // core_0 wfi_sel
    spm_write(SPM_CORE1_CON, 0x0); // core_1 wfi_sel
    spm_write(SPM_CORE2_CON, 0x0); // core_2 wfi_sel 
    spm_write(SPM_CORE3_CON, 0x0); // core_3 wfi_sel 
 
     // SET_PCM_MASK_MASTER_REQ
    spm_write(SPM_PCM_MAS_PAUSE_MASK, 0xFFFFFFFF); // bus protect mask

    /* clean ISR status */
    spm_write(SPM_SLEEP_ISR_MASK, 0x0008);
    spm_write(SPM_SLEEP_ISR_STATUS, 0x0018);

    // SET_PCM_WAKE_SOURCE
    #if SPM_MCDI_BYPASS_SYSPWREQ
    spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, (~(0x3c600011))); // PCM_TIMER/Thermal/CIRQ/CSYSBDG/CPU[n]123/csyspwreq_b/xgpt
    #else
    spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, (~(0x3d600011))); // PCM_TIMER/Thermal/CIRQ/CSYSBDG/CPU[n]123/xgpt
    #endif

    // set SPM_APMCU_PWRCTL
    spm_write(SPM_APMCU_PWRCTL, 0x0);

    // unmask SPM ISR ( 1 : Mask and will not issue the ISR )    
    spm_write(SPM_SLEEP_ISR_MASK, 0);
}

static void KICK_IM_PCM(u32 pcm_sa, u32 len)
{
    u32 clc_temp;
    
    spm_write(SPM_PCM_IM_PTR, pcm_sa);
    spm_write(SPM_PCM_IM_LEN, len);
    
    //set non-replace mde 
    //spm_write(SPM_PCM_CON1, (CON1_CFG_KEY | CON1_MIF_APBEN | CON1_IM_NONRP_EN));
    spm_write(SPM_PCM_CON1, (CON1_CFG_KEY | CON1_IM_NONRP_EN));

    //sync register and enable IO output for regiser 0 and 7
    spm_write(SPM_PCM_PWR_IO_EN, (PCM_PWRIO_EN_R7|PCM_PWRIO_EN_R0));  
    
    //Kick PCM & IM
    clc_temp = spm_read(SPM_PCM_CON0);
    spm_write(SPM_PCM_CON0, (clc_temp | CON0_CFG_KEY | CON0_PCM_KICK | CON0_IM_KICK));
    spm_write(SPM_PCM_CON0, (clc_temp | CON0_CFG_KEY));
}

static void spm_go_to_MCDI(void)
{
    unsigned long flags;
    spin_lock_irqsave(&spm_lock, flags);

    En_SPM_MCDI = 2;
    
    #if 1
    // check dram controller setting ==============================
    if(((spm_read(0xf00041dc) & 0x00ff0000) != 0x00660000 || (spm_read(0xf0004044) & 0xff000000) != 0x9f000000))
    {
        clc_notice("spm_go_to_MCDI : check dramc failed (0x%x, 0x%x, 0x%x, 0x%x)\n", spm_read(0xf00041dc), 0x00660000, spm_read(0xf0004044), 0x9f000000);
        BUG_ON(1);
    }
    //spm_check_dramc_for_pcm();
    #endif

    // mask SPM IRQ =======================================
    mt_irq_mask_for_sleep(MT_SPM_IRQ_ID); // mask spm    

    // SPM SW reset ========================================
    SPM_SW_Reset();

    // Set PCM VSR =========================================
    SET_PCM_EVENT_VEC(0, 11, 1, 0, MCDI_pcm_pc_0); //MD wake 
    SET_PCM_EVENT_VEC(1, 12, 1, 0, MCDI_pcm_pc_1); //MD sleep
    SET_PCM_EVENT_VEC(2, 30, 1, 0, MCDI_pcm_pc_2); //MM wake
    SET_PCM_EVENT_VEC(3, 31, 1, 0, MCDI_pcm_pc_3); //MM sleep

    // init PCM ============================================
    PCM_Init();    

    // print spm debug log =====================================
    spm_mcdi_dump_regs();
    
    // Kick PCM and IM =======================================
    BUG_ON(PCM_MCDI_BASE & 0x00000003);     // check 4-byte alignment 
    KICK_IM_PCM(PCM_MCDI_BASE, PCM_MCDI_LEN); 
    clc_notice("Kick PCM and IM OK.\r\n");

    En_SPM_MCDI = 1;

    spin_unlock_irqrestore(&spm_lock, flags);
}

static u32 spm_leave_MCDI(void)
{
    u32 spm_counter;
    u32 spm_core_pws, hotplug_out_core_id;
    unsigned long flags;

    /* Mask ARM i bit */
    //asm volatile("cpsid i @ arch_local_irq_disable" : : : "memory", "cc"); // set i bit to disable interrupt    
    local_irq_save(flags);

    En_SPM_MCDI = 2;
  
    // trigger cpu wake up event
    spm_write(SPM_SLEEP_CPU_WAKEUP_EVENT, 0x1);   

    // polling SPM_SLEEP_ISR_STATUS ===========================
    spm_counter = 0;
    
    while(((spm_read(SPM_SLEEP_ISR_STATUS)>>3) & 0x1) == 0x0)
    {
        if(spm_counter >= 10000)
        {
            // set cpu wake up event = 0
            spm_write(SPM_SLEEP_CPU_WAKEUP_EVENT, 0x0);   
        
            clc_notice("spm_leave_MCDI : failed_0.\r\n");

            En_SPM_MCDI = 0;

            /* Un-Mask ARM i bit */
            //asm volatile("cpsie i @ arch_local_irq_enable" : : : "memory", "cc"); // clear i bit to enable interrupt
            local_irq_restore(flags);
            return 1;
        }
        spm_counter++;
    }

    // set cpu wake up event = 0
    spm_write(SPM_SLEEP_CPU_WAKEUP_EVENT, 0x0);   
   
    // clean SPM_SLEEP_ISR_STATUS ============================
    spm_write(SPM_SLEEP_ISR_MASK, 0x0008);
    spm_write(SPM_SLEEP_ISR_STATUS, 0x0018);

    //disable IO output for regiser 0 and 7
    spm_write(SPM_PCM_PWR_IO_EN, 0x0);  

    // print spm debug log ===================================
    spm_mcdi_dump_regs();

    // clean wakeup event raw status
    spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, 0xffffffff);

    #if 1
    // check dram controller setting ==============================
    if(((spm_read(0xf00041dc) & 0x00ff0000) != 0x00660000 || (spm_read(0xf0004044) & 0xff000000) != 0x9f000000))
    {
        clc_notice("spm_go_to_MCDI : check dramc failed (0x%x, 0x%x, 0x%x, 0x%x)\n", spm_read(0xf00041dc), 0x00660000, spm_read(0xf0004044), 0x9f000000);
        BUG_ON(1);
    }
    //spm_check_dramc_for_pcm();
    #endif

    // check cpu power ======================================
    spm_core_pws = ((spm_read(SPM_FC0_PWR_CON) == 0x4d) ? 0 : 1) | ((spm_read(SPM_FC1_PWR_CON) == 0x4d) ? 0 : 2) |
                             ((spm_read(SPM_FC2_PWR_CON) == 0x4d) ? 0 : 4) | ((spm_read(SPM_FC3_PWR_CON) == 0x4d) ? 0 : 8); // power_state => 1: power down

    hotplug_out_core_id = ((spm_read(SPM_MP_CORE0_AUX) & 0x1)<<0) | ((spm_read(SPM_MP_CORE1_AUX) & 0x1)<<1) | 
    	                              ((spm_read(SPM_MP_CORE2_AUX) & 0x1)<<2) | ((spm_read(SPM_MP_CORE3_AUX) & 0x1)<<3); // 1: hotplug out

    if( spm_core_pws != hotplug_out_core_id )
    {
        clc_notice("spm_leave_MCDI : failed_1.(0x%x, 0x%x)\r\n", spm_core_pws, hotplug_out_core_id);

        En_SPM_MCDI = 0;

        /* Un-Mask ARM i bit */
        //asm volatile("cpsie i @ arch_local_irq_enable" : : : "memory", "cc"); // clear i bit to enable interrupt
        local_irq_restore(flags);

        //BUG_ON(1);
        return 0;
    }   
    
    clc_notice("spm_leave_MCDI : OK.\r\n");

    En_SPM_MCDI = 0;

    /* Un-Mask ARM i bit */
    //asm volatile("cpsie i @ arch_local_irq_enable" : : : "memory", "cc"); // clear i bit to enable interrupt
    local_irq_restore(flags);
    return 0;
}

void spm_clean_ISR_status(void)
{
    // clean wakeup event raw status
    spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, 0xffffffff);

    // clean ISR status
    spm_write(SPM_SLEEP_ISR_MASK, 0x0008);
    spm_write(SPM_SLEEP_ISR_STATUS, 0x0018);
}

#if 0
// =====================================================================
//---------------------------------------------------
// address definition
//---------------------------------------------------
#define disp_base (0xf4000000)
#define DISP_MUTEX_BASE (disp_base+0x11000)
#define DISP_MUTEX0 (DISP_MUTEX_BASE+0x24)

//------------------------------------------------------------------------
// base address definition
//------------------------------------------------------------------------
#define mmsys_base (0xf4000000)
#define i2c_base (0xf4014000)
#define SPM_DSI_BASE ( mmsys_base+0xD000)
#define dbi_base ( mmsys_base+0xC000)
#define LCM_RST_B ( dbi_base+0x0010 )
#define DSI_CMDQ_BASE  (SPM_DSI_BASE+0x180)

//------------------------------------------------------------------------
// MIPI config register map
//------------------------------------------------------------------------
#define mipi_tx_config_base (0xf0012000)
#define mipi_rx_ana_base (0xf0012800)
// =====================================================================
#endif

static void spm_show_lcm_image(void)
{
#if 0
//---------------------------------------------------
// load image
//---------------------------------------------------
u32 pic0_addr=0x95000000;//
//d.load.binary Kara_1080x1920.bmp          &pic0_addr 

u32 v_pic0_addr;

u32 w=1080; //  1080
u32 h=1920; //  1920

u32 pitch=3*w;//

unsigned long flags;


//------------------------------------------------------------------------
// MT6582 DSI Address
//------------------------------------------------------------------------
u32 DSI_START= SPM_DSI_BASE+0x00;
u32 DSI_INTSTA= SPM_DSI_BASE+0x0c;
u32 DSI_COM_CON= SPM_DSI_BASE+0x10;
u32 DSI_MODE_CON= SPM_DSI_BASE+0x14;
u32 DSI_TXRX_CON= SPM_DSI_BASE+0x18;
u32 DSI_PSCON= SPM_DSI_BASE+0x1C;
u32 DSI_VSA_NL= SPM_DSI_BASE+0x20;
u32 DSI_VBP_NL= SPM_DSI_BASE+0x24;
u32 DSI_VFP_NL= SPM_DSI_BASE+0x28;
u32 DSI_VACT_NL= SPM_DSI_BASE+0x2c;
u32 DSI_HSA_WC= SPM_DSI_BASE+0x50;
u32 DSI_HBP_WC= SPM_DSI_BASE+0x54;
u32 DSI_HFP_WC= SPM_DSI_BASE+0x58;
u32 DSI_BLLP_WC= SPM_DSI_BASE+0x5c;
u32 DSI_CMDQ_CON= SPM_DSI_BASE+0x60;
u32 DSI_HSTX_CKLP_WC= SPM_DSI_BASE+0x64;
u32 DSI_RACK= SPM_DSI_BASE+0x84;
u32 DSI_MEM_CONTI= SPM_DSI_BASE+0x90;

u32 DSI_PHY_CON= SPM_DSI_BASE+0x100;
u32 DSI_PHY_LCCON= SPM_DSI_BASE+0x104;
u32 DSI_PHY_LD0CON= SPM_DSI_BASE+0x108;
u32 DSI_PHY_TIMCON0= SPM_DSI_BASE+0x110;
u32 DSI_PHY_TIMCON1= SPM_DSI_BASE+0x114;
u32 DSI_PHY_TIMCON2= SPM_DSI_BASE+0x118;
u32 DSI_PHY_TIMCON3= SPM_DSI_BASE+0x11C;
u32 DSI_PHY_TIMCON4= SPM_DSI_BASE+0x120;

u32 DSI_VM_CMD_CON= SPM_DSI_BASE+0x130;
u32 DSI_VM_CMD_DATA0= SPM_DSI_BASE+0x134;
u32 DSI_VM_CMD_DATA4= SPM_DSI_BASE+0x138;
u32 DSI_VM_CMD_DATA8= SPM_DSI_BASE+0x13c;
u32 DSI_VM_CMD_DATAC= SPM_DSI_BASE+0x140;

u32 DSI_DEBUG_SEL= SPM_DSI_BASE+0x170;

u32 DSI_CMDQ_0= DSI_CMDQ_BASE+0x00;
u32 DSI_CMDQ_1= DSI_CMDQ_BASE+0x04;
u32 DSI_CMDQ_2= DSI_CMDQ_BASE+0x08;
u32 DSI_CMDQ_3= DSI_CMDQ_BASE+0x0c;
u32 DSI_CMDQ_4= DSI_CMDQ_BASE+0x10;
u32 DSI_CMDQ_5= DSI_CMDQ_BASE+0x14;
u32 DSI_CMDQ_6= DSI_CMDQ_BASE+0x18;
u32 DSI_CMDQ_7= DSI_CMDQ_BASE+0x1c;
u32 DSI_CMDQ_8= DSI_CMDQ_BASE+0x20;
u32 DSI_CMDQ_9= DSI_CMDQ_BASE+0x24;

//---------------------------------------------------
// parameter setting
//---------------------------------------------------
u32 size=3; // 0:480x800, 1:720x1280, 2:540x960, 3:1080x1920
u32 out_if=0;// 0:dsi, 1:dbi, 2:dpi
u32 dsi_video_mode=1;// 0:command mode, 1:video mode
u32 reload_img=1;//

u32 sharp_fhd=1;//
u32 temp_address;

clc_notice("spm_show_lcm_image() start.\n");

/* Mask ARM i bit */
//asm volatile("cpsid i @ arch_local_irq_disable" : : : "memory", "cc"); // set i bit to disable interrupt    
local_irq_save(flags);

// disable all display engine interrupt ===================================
spm_write(0xf400d008 ,  0x0 );
spm_write(0xf4003004 ,  0x0 );
spm_write(0xf4011000 ,  0x0 );
spm_write(0xf4006000 ,  0x0 );
// =========================================================

v_pic0_addr = pic0_addr | 0x40000000;

#if 0
for(temp_address = v_pic0_addr ; temp_address<=(v_pic0_addr+(w*h*pitch)) ; temp_address+=4)
{
    if(((temp_address/(w*200*pitch))%2) == 0)
    {
        spm_write(temp_address, 0xffffffff);
    }
    else
    {
        spm_write(temp_address, 0x0);
    }
}
#endif

//---------------------------------------------------
// set gpio
//---------------------------------------------------
// gpio 130, mode 7 : LPTE
// gpio 131, mode 7 : LRSTB
// gpio 132, mode 1 : LPCE1B
// gpio 133, mode 1 : LPCE0B
// gpio 138~142, mode 7 : dbi-c
// gpio 143~171, mode 1 : dpi, dbi-b

spm_write(0xf0005DA0 ,  0x027f );// gpio 134~130
spm_write(0xf0005DB0 ,  0x7e00 );// gpio 139~135
spm_write(0xf0005DC0 ,  0x13ff );// gpio 144~140
spm_write(0xf0005DD0 ,  0x1249 );// gpio 149~145
spm_write(0xf0005DE0 ,  0x1249 );// gpio 154~150
spm_write(0xf0005DF0 ,  0x1249 );// gpio 159~155
spm_write(0xf0005E00 ,  0x1249 );// gpio 164~160
spm_write(0xf0005E10 ,  0x1249 );// gpio 169~165
spm_write(0xf0005E20 ,  0x0009 );// gpio 174~170

u32 rdma0_out_sel=0;// dsi
//------------------------------------------------------------------------
// PHY Timing Control
//------------------------------------------------------------------------
u32 fbkdiv=0x47;//
u32 txdiv0=0x1;//
u32 txdiv1=0x1;//

//------------------------------------------------------------------------
// Clock source and debug settings
//------------------------------------------------------------------------
spm_write(0xf0000154 ,  0x00000000);    // [12:11] rg_mipi_26m_sel
spm_write(0xf5000000 ,  0x00000e00);    // img_cg_con

spm_write(mmsys_base+0x00108 ,  0xffffffff); // clear cg0
spm_write(mmsys_base+0x00118 ,  0xffffffff); // clear cg1
spm_write(mmsys_base+0x0004c ,  0x00000000); // dsi out 


u32 DSI_CON= mipi_tx_config_base+0x000;
u32 DSI_CLOCK_LANE= mipi_tx_config_base+0x004;
u32 DSI_DATA_LANE_0= mipi_tx_config_base+0x008;
u32 DSI_DATA_LANE_1= mipi_tx_config_base+0x00c;
u32 DSI_DATA_LANE_2= mipi_tx_config_base+0x010;
u32 DSI_DATA_LANE_3= mipi_tx_config_base+0x014;
u32 DSI_TOP_CON= mipi_tx_config_base+0x040;
u32 DSI_BG_CON= mipi_tx_config_base+0x044;
u32 DSI_PLL_CON0= mipi_tx_config_base+0x050;
u32 DSI_PLL_CON1= mipi_tx_config_base+0x054;
u32 DSI_PLL_TOP= mipi_tx_config_base+0x060;
u32 DSI_GPIO_EN= mipi_tx_config_base+0x068;
u32 DSI_GPIO_OUT= mipi_tx_config_base+0x06c;
u32 DSI_SW_CTRL_EN= mipi_tx_config_base+0x080;
u32 DSI_SW_CTRL_CON0= mipi_tx_config_base+0x084;
u32 DSI_SW_CTRL_CON1= mipi_tx_config_base+0x088;

//------------------------------------------------------------------------
// MIPI config
//------------------------------------------------------------------------
// [0] RG_DSI_BG_CORE_EN = 1
// [1] RG_DSI_BG_CKEN = 1
spm_write(DSI_BG_CON   ,  0x88492483);

// [1] RG_DSI_LNT_HS_BIAS_EN = 1
spm_write(DSI_TOP_CON  ,  0x00000082);

// [0] RG_DSI0_LDOCORE_EN = 1
// [1] RG_DSI0_CKG_LDOOUT_EN = 1
spm_write(DSI_CON      ,  0x00000003);

// [10] RG_MPPLL_BIAS_EN = 1
spm_write(DSI_PLL_TOP  ,  0x00000600);

// [7:1] RG_DSI0_MPPLL_FBKDIV = 7¡¦h13
// [13:12] RG_DSI0_MPPLL_TXDIV0 = 0
// [15:14] RG_DSI0_MPPLL_TXDIV1 = 0
u32 pll_con=0xbe510000|( txdiv1<<0xE)|( txdiv0<<0xC)|( fbkdiv<<1);//
spm_write(DSI_PLL_CON0  ,  pll_con);

// [0] RG_DSI0_MPPLL_PLL_EN = 1
pll_con=pll_con|0x1;//
spm_write(DSI_PLL_CON0  ,  pll_con);
udelay(100);

clc_notice( "PLL setting finish!!\n");

spm_write(DSI_CLOCK_LANE     ,  0x00000821);
spm_write(DSI_DATA_LANE_0    ,  0x00000401);
spm_write(DSI_DATA_LANE_1    ,  0x00000101);
spm_write(DSI_DATA_LANE_2    ,  0x00000101);
spm_write(DSI_DATA_LANE_3    ,  0x00000101);

//------------------------------------------------------------------------
// DSI / LCM Reset
//------------------------------------------------------------------------
//dsi reset - must reset after DSI clock is on
spm_write(DSI_COM_CON ,  0x00000001);
spm_write(DSI_COM_CON ,  0x00000000);

//------------------------------------------------------------------------
// DSI Timing Control
//------------------------------------------------------------------------
u32 txdiv1_real=0;//
if ( txdiv1==0x00)
    txdiv1_real=0x01;

if ( txdiv1==0x01)
    txdiv1_real=0x02;

if ( txdiv1==0x02)
    txdiv1_real=0x04;

if ( txdiv1==0x03)
    txdiv1_real=0x04;

clc_notice( "txdiv1_real=0x%x\n", txdiv1_real);

u32 txdiv0_real=0;//
if ( txdiv0==0x00)
    txdiv0_real=0x01;

if ( txdiv0==0x01)
    txdiv0_real=0x02;

if ( txdiv0==0x02)
    txdiv0_real=0x04;

if ( txdiv0==0x03)
    txdiv0_real=0x04;

clc_notice( "txdiv0_real=0x%x\n", txdiv0_real);

////------ cycle time 
u32 cycle_time=0;
////cycle_time=1000xtxdiv1_realxtxdiv0_realx8/(26x( fbkdiv+1)x2)
cycle_time=(0x99*txdiv1_real*txdiv0_real)/(fbkdiv+0x01);

u32 ui=(0x13*txdiv1_real*txdiv0_real)/(fbkdiv+0x01);
clc_notice( "ui=0x%x\n", ui);

////------ lpx: 50ns ~
u32 lpx=0;//
lpx=(0x50/cycle_time);

if (lpx==0x00)
    lpx=0x01;

clc_notice( "lpx=0x%x\n", lpx);

////------ hs_prep: 40ns+4*UI  ~  85ns+6*UI
u32 hs_prep=((0x40+0x05*ui)/cycle_time);//
if (hs_prep==0x00)
    hs_prep=0x01;

clc_notice( "hs_prep=0x%x\n", hs_prep);

////------ hs_prep+hs_zero: 145ns+10*UI
u32 hs_zero=((0xc8+0x0a*ui)/cycle_time)-hs_prep;//
clc_notice( "hs_zero=0x%x\n", hs_zero);

////------ hs_trail:max(n*8*UI,60ns+n*4*UI) ~
 u32 hs_trail=0;
//// hs_trail_m= lane_num//
u32 hs_trail_m=1;//
clc_notice( "hs_trail_m=0x%x\n", hs_trail_m);

u32 hs_trail_n=(((hs_trail_m*0x04)+0x60)/cycle_time);//
clc_notice( "hs_trail_n=0x%x\n", hs_trail_n);

if (hs_trail_m>hs_trail_n)
{
  hs_trail=hs_trail_m+0x0a;
  clc_notice( "hs_trail=0x%x\n", hs_trail);
}
else
{
  hs_trail=hs_trail_n+0x0a;
  clc_notice( "hs_trail=0x%x\n", hs_trail);
}

////------ ta_go:4*LPX
u32 ta_go=4*lpx;
clc_notice( "ta_go=0x%x\n", ta_go);

////------ ta_sure:LPX ~ 2*LPX
 u32 ta_sure= lpx+( lpx/0x02);
clc_notice( "ta_sure=0x%x\n", ta_sure);

////------ ta_get:5*LPX
 u32 ta_get=5* lpx;
clc_notice( "ta_get=0x%x\n", ta_get);

u32  ta_sack=0x01;
u32  cont_det=0x00;

////------ hs_exit, clk_exit, clk_post
u32 hs_exit=2* lpx;
u32 clk_exit=2* lpx;
u32 clk_post=0x0e;
clc_notice( "clk_post=0x%x\n", clk_post);

////------ clk_hs_prep:38~95
u32 clk_hs_prep=(0x40/ cycle_time);
if ( clk_hs_prep==0x00)
{
 clk_hs_prep=0x01;
}
clc_notice( "clk_hs_prep=0x%x\n", clk_hs_prep);

////------ clk_hs_prep+clk_zero:300~
// clk_zero=(0x190/ cycle_time)- clk_hs_prep
u32 clk_zero=(0x190/ cycle_time);
clc_notice( "clk_zero=0x%x\n", clk_zero);

////------ clk_trail:60~
u32 clk_trail=(0x64/ cycle_time)+0x0a;
clc_notice( "clk_trail=0x%x\n", clk_trail);

u32 timcon0=( hs_trail<<0x018)|( hs_zero<<0x10)|( hs_prep<<0x08)| lpx;//
u32 timcon1=( hs_exit<<0x18)|( ta_get<<0x10)|( ta_sure<<0x08)| ta_go;//
u32 timcon2=( clk_trail<<0x018)|( clk_zero<<0x10)| cont_det;//
u32 timcon3=( clk_exit<<0x10)|( clk_post<<0x08)| clk_hs_prep;//

spm_write(DSI_PHY_TIMCON0 ,   timcon0);   // DSI_PHY_TIMCON0
spm_write(DSI_PHY_TIMCON1 ,   timcon1);   // DSI_PHY_TIMCON1
spm_write(DSI_PHY_TIMCON2 ,   timcon2);   // DSI_PHY_TIMCON2
spm_write(DSI_PHY_TIMCON3 ,   timcon3);   // DSI_PHY_TIMCON3

//do dsi/DSI_test_video_mode.cmm
//------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------
u32 color_fmt=0x3;            // 0:RGB565, 1:RGB666, 2:RGB666_l, 3:RGB888
u32 lane_num=4;//
u32 dsi_mode=1;               // '0':command mode
                          // '1':sync-pulse video mode
                          // '2':sync-event video mode
                          // '3':burst video mode
u32 cklp_en=1;                // clk low-power enable

//------------------------------------------------------------------------
// DSI setting
//------------------------------------------------------------------------
//DSI mode control
spm_write(DSI_MODE_CON ,  0x0);   // [1:0]:DSI_MODE_CON

u32 lane_enable=0x0f;//
u32 txrx_con=( cklp_en<<0x10)|( lane_enable<<2);//

spm_write(DSI_TXRX_CON ,   txrx_con);    //
                  // [1:0]: virtual channel
                  // [5:2]: lane number
                  // [6]:   disable EoTp
                  // [7]:   BLLP with null packet
                  // [8]:   te_freerun
                  // [15:12]: maximum return packet size                 
                  // [16]: cklp_en

u32 word_cnt= w*3;//
u32 pscon=( color_fmt<<0x10)| word_cnt;//
spm_write(DSI_PSCON ,     pscon   ); // [13:0]:word_cnt, [17:16]:format

spm_write(DSI_BLLP_WC ,  0x00000100   ); // DSI_BLLP_WC
spm_write(DSI_MEM_CONTI ,  0x0000003c  );  // RWMEM_CONTI
spm_write(DSI_HSTX_CKLP_WC ,   pscon   );  // DSI_HSTX_CKLP_WC

spm_write(DSI_DEBUG_SEL ,  0x0000000c);

spm_write(DSI_COM_CON ,  0x00000002 );   //// dsi_en

mdelay(10);

//------------------------------------------------------------------------
// Setup LCM
//------------------------------------------------------------------------
//do dsi/DSI_set_R63311_video.cmm
//------------------------------------------------------------------------
// Setup timing paramaters 
// based on Novatek FHD panel module UserGuide page.27
//------------------------------------------------------------------------
 clk_hs_prep=8;
 clk_zero=30;
 clk_post=22;
 clk_trail=8;
 clk_exit=15;

 hs_prep=9;
 hs_zero=10;
 hs_trail=20;
 hs_exit=15;

 lpx=9;
 ta_sure=3;
 ta_get=4* lpx;
 ta_sure=5* lpx;

 timcon0=( hs_trail<<0x018)|( hs_zero<<0x10)|( hs_prep<<0x08)| lpx;//
 timcon1=( hs_exit<<0x18)|( ta_get<<0x10)|( ta_sure<<0x08)| ta_go;//
 timcon2=( clk_trail<<0x018)|( clk_zero<<0x10)| cont_det;//
 timcon3=( clk_exit<<0x10)|( clk_post<<0x08)| clk_hs_prep;//

spm_write(DSI_PHY_TIMCON0 ,   timcon0);   // DSI_PHY_TIMCON0
spm_write(DSI_PHY_TIMCON1 ,   timcon1);   // DSI_PHY_TIMCON1
spm_write(DSI_PHY_TIMCON2 ,   timcon2);   // DSI_PHY_TIMCON2
spm_write(DSI_PHY_TIMCON3 ,   timcon3);   // DSI_PHY_TIMCON3

//------------------------------------------------------------------------
// Setup video mode
// based on Novatek FHD panel module UserGuide page.27
//------------------------------------------------------------------------
spm_write(DSI_VSA_NL ,  0x00000001);     // DSI_VSA_NL 
spm_write(DSI_VBP_NL ,  0x00000004);     // DSI_VBP_NL 
spm_write(DSI_VFP_NL ,  0x00000002);     // DSI_VFP_NL 
spm_write(DSI_VACT_NL ,  h);            // DSI_VACT_NL(Set Vertical Active line=800)
spm_write(DSI_HSA_WC ,  0x00000010);     // DSI_HSA_WC=1*4
spm_write(DSI_HBP_WC ,  0x00000080);     // DSI_HBP_WC=235/8*4
spm_write(DSI_HFP_WC ,  0x00000118);     // DSI_HFP_WC=235/8*4

//------------------------------------------------------------------------
// Misc
//------------------------------------------------------------------------

spm_write(LCM_RST_B ,  0x00000000);
mdelay(100);
spm_write(LCM_RST_B ,  0x00000001);

mdelay(10);

//------------------------------------------------------------------------
// FullHD LCM setup
//------------------------------------------------------------------------
spm_write(DSI_CMDQ_0 ,  0x04B02300);  // REGW 0xB0
spm_write(DSI_CMDQ_CON ,  0x00000001);  
spm_write(DSI_START ,  0x00000000);  
spm_write(DSI_START ,  0x00000001);  // DSI_START

clc_notice("spm_show_lcm_image() 00.\n");

while ((spm_read(DSI_INTSTA)&0x2)!=0x2) //
{
  mdelay(500); 
}
spm_write(DSI_INTSTA ,  0xfffd );//write 0 clear

clc_notice("spm_show_lcm_image() 01.\n");


mdelay(150);     //// mdelay(150ms

spm_write(DSI_CMDQ_0 ,  0x00351500  );// REGW 0x35
spm_write(DSI_CMDQ_CON ,  0x00000001  );
spm_write(DSI_START ,  0x00000000  );
spm_write(DSI_START ,  0x00000001  );// DSI_START

while ((spm_read(DSI_INTSTA)&0x2)!=0x2) //
{
  mdelay(500); 
}
spm_write(DSI_INTSTA ,  0xfffd); //write 0 clear

clc_notice("spm_show_lcm_image() 02.\n");


mdelay(150);     //// mdelay(150ms

spm_write(DSI_CMDQ_0 ,  0x00290500);  // REGW 0x29
spm_write(DSI_CMDQ_CON ,  0x00000001);  
spm_write(DSI_START ,  0x00000000);  
spm_write(DSI_START ,  0x00000001);  // DSI_START

while ((spm_read(DSI_INTSTA)&0x2)!=0x2) //
{
  mdelay(500); 
}
spm_write(DSI_INTSTA ,  0xfffd); //write 0 clear

clc_notice("spm_show_lcm_image() 03.\n");

mdelay(150);     //// mdelay(150ms

spm_write(DSI_CMDQ_0 ,  0x00110500);  // REGW 0x11
spm_write(DSI_CMDQ_CON ,  0x00000001);  
spm_write(DSI_START ,  0x00000000);  
spm_write(DSI_START ,  0x00000001);  // DSI_START

while ((spm_read(DSI_INTSTA)&0x2)!=0x2) //
{
  mdelay(500); 
}
spm_write(DSI_INTSTA ,  0xfffd); //write 0 clear

clc_notice("spm_show_lcm_image() 04.\n");

//------------------------------------------------------------------------
// HS mode transmission, data from lcd
//------------------------------------------------------------------------
spm_write(DSI_PHY_LCCON ,  0x00000001);   // enable clock lane HS mode
mdelay(500);

//// DSI mode control
spm_write(DSI_MODE_CON  ,  dsi_mode);    // [1:0]:DSI_MODE_CON, 0:command mode, 1:sync_pulse, 2:sync_event, 3:burst
spm_write(DSI_START     ,  0x000);   
spm_write(DSI_START     ,  0x001);        // start dsi

//do cmm/ovl_bls.cmm
//---------------------------------------------------
// set disp_mutex
//---------------------------------------------------
u32 mutex_sof_src=1;// dsi
spm_write(disp_base+0x11000 ,  0x00000004); // write 0008
spm_write(disp_base+0x11030 ,  mutex_sof_src); // write 0009
spm_write(disp_base+0x1102c ,  0x00000084); // write 0010
spm_write(disp_base+0x11024 ,  0x00000001); // write 0011
//// polling if we get mutex0 
while ((spm_read(DISP_MUTEX_BASE+0x24)&0x2)!=0x2) // mdelay(get mutex0 
{
  mdelay(500); 
}
clc_notice("spm_show_lcm_image() 05.\n");

//---------------------------------------------------
// set disp_config
//---------------------------------------------------
spm_write(disp_base+0x00054 ,  0x00000000); // write 0014
spm_write(disp_base+0x00024 ,  0x00000002); // write 0015
spm_write(disp_base+0x00034 ,  rdma0_out_sel); // write 0016
spm_write(disp_base+0x0005c ,  0x00000000); // write 0017
//---------------------------------------------------
// set disp_ovl
//---------------------------------------------------
spm_write(disp_base+0x03020 ,  (h<<0x10)+w); // write 0018
spm_write(disp_base+0x03028 ,  0x881b79c3); // write 0019
spm_write(disp_base+0x0302c ,  0x00000001); // write 0020
spm_write(disp_base+0x03024 ,  0x01e00000); // write 0021
spm_write(disp_base+0x03030 ,  0x00000087); // write 0022
spm_write(disp_base+0x03034 ,  0x5555aaaa); // write 0023
spm_write(disp_base+0x03038 ,  (h<<0x10)+w); // write 0024
spm_write(disp_base+0x0303c ,  0x00000000); // write 0025
//spm_write(disp_base+0x03040 ,  0x81100000 // write 0026
spm_write(disp_base+0x03040 ,  pic0_addr+0x36); //
//spm_write(disp_base+0x03044 ,  0x000005a0 // write 0027
spm_write(disp_base+0x03044 ,  pitch); // write 0027
spm_write(disp_base+0x030c0 ,  0x00000001); // write 0046
spm_write(disp_base+0x030e0 ,  0x00000001); // write 0047
spm_write(disp_base+0x03100 ,  0x00000001); // write 0048
spm_write(disp_base+0x03120 ,  0x00000001); // write 0049
spm_write(disp_base+0x0300c ,  0x00000001); // write 0050
//---------------------------------------------------
// set disp_bls
//---------------------------------------------------
spm_write(disp_base+0x08000 ,  0x00000000); // write 0070
//---------------------------------------------------
// set disp_rdma0
//---------------------------------------------------
spm_write(disp_base+0x06010 ,  0x00000100); // write 0073
spm_write(disp_base+0x06014 ,  w); // write 0074
spm_write(disp_base+0x06018 ,  h); // write 0075
spm_write(disp_base+0x06024 ,  0x00000080); // write 0076
spm_write(disp_base+0x06028 ,  0x00000000); // write 0077
spm_write(disp_base+0x0602c ,  0x00000000); // write 0078
//spm_write(disp_base+0x06030 ,  0x10101010); // write 0079
spm_write(disp_base+0x06030 ,  0x50303040); // write 0079
spm_write(disp_base+0x06038 ,  0x00000020); // write 0080
spm_write(disp_base+0x06040 ,  0x00f00008); // write 0081
spm_write(disp_base+0x06040 ,  0x80f00008); // write 0084
spm_write(disp_base+0x06010 ,  0x00000101); // write 0087
//---------------------------------------------------
// release disp_mutex
//---------------------------------------------------
spm_write(disp_base+0x11024 ,  0x00000000); // write 0088

/* Un-Mask ARM i bit */
//asm volatile("cpsie i @ arch_local_irq_enable" : : : "memory", "cc"); // clear i bit to enable interrupt
local_irq_restore(flags);
        
clc_notice("spm_show_lcm_image() end.\n");
#endif
}

void spm_mcdi_wfi(void)
{   
        volatile u32 core_id;
        u32 clc_counter;
        u32 spm_val;

        core_id = (u32)smp_processor_id();
        
        if(core_id == 0)
        {
            /* enable & init CIRQ */
            //mt_cirq_clone_gic();        
            //mt_cirq_mask((MT6589_GPT_IRQ_ID-64));
            //mt_cirq_enable();

            // CPU0 enable CIRQ for SPM, Mask all IRQ keep only SPM IRQ ==========
            //mt_SPI_mask_all(&MCDI_cpu_irq_mask); // mask core_0 SPI
            //mt_irq_unmask_for_sleep(MT_SPM_IRQ_ID); // unmask spm    

            if(MCDI_Test_Mode == 1)
                clc_notice("core_%d set wfi_sel.\n", core_id);

            spm_write(SPM_CORE0_CON, SPM_MCDI_CORE_SEL);

            while(1)
            {
                mcdi_wfi_with_sync(); // enter wfi 

                if((spm_read(SPM_AP_STANBY_CON) & 0x1) == 0x0) // check wfi_sel_0 == 0
                {
                    break;
                }
            }        
            
            if(MCDI_Test_Mode == 1)
                clc_notice("core_%d exit wfi.\n", core_id);            
            
            // debug info ===========================================================================
            #if 1
            
            if( (spm_read(SPM_PCM_REG_DATA_INI) != 0x0) || (spm_read(SPM_SLEEP_CPU_WAKEUP_EVENT) == 0x1) )
            {
                clc_notice("MCDI SPM assert_0 ==================================================\n");
                clc_notice("SPM_PCM_REG_DATA_INI   0x%x = 0x%x\n", SPM_PCM_REG_DATA_INI          , spm_read(SPM_PCM_REG_DATA_INI));
                clc_notice("SPM_PCM_REG5_DATA   0x%x = 0x%x\n", SPM_PCM_REG5_DATA          , spm_read(SPM_PCM_REG5_DATA));
                clc_notice("SPM_PCM_REG7_DATA   0x%x = 0x%x\n", SPM_PCM_REG7_DATA          , spm_read(SPM_PCM_REG7_DATA));
                clc_notice("SPM_PCM_REG12_DATA   0x%x = 0x%x\n", SPM_PCM_REG12_DATA          , spm_read(SPM_PCM_REG12_DATA));
                clc_notice("SPM_PCM_REG13_DATA   0x%x = 0x%x\n", SPM_PCM_REG13_DATA          , spm_read(SPM_PCM_REG13_DATA));
                clc_notice("SPM_PCM_REG14_DATA   0x%x = 0x%x\n", SPM_PCM_REG14_DATA          , spm_read(SPM_PCM_REG14_DATA));
                clc_notice("SPM_APMCU_PWRCTL   0x%x = 0x%x\n", SPM_APMCU_PWRCTL          , spm_read(SPM_APMCU_PWRCTL));
                clc_notice("SPM_AP_STANBY_CON   0x%x = 0x%x\n", SPM_AP_STANBY_CON          , spm_read(SPM_AP_STANBY_CON));
                clc_notice("SPM_CLK_CON   0x%x = 0x%x\n", SPM_CLK_CON          , spm_read(SPM_CLK_CON));
                clc_notice("MCDI SPM assert_1 ==================================================\n");
            }
            
            #endif
            // ==================================================================================

            // restore irq mask
            //mt_SPI_mask_restore(&MCDI_cpu_irq_mask);

            /* flush & disable CIRQ */
            //mt_cirq_flush();
            //mt_cirq_disable();
           
            if(MCDI_Test_Mode == 1)
                mdelay(10);  // delay 10 ms
                
            // wait for SPM edge interrupt = 0
            clc_counter = 0;
            while(((spm_read(SPM_SLEEP_ISR_STATUS)>>4) & 0x1) == 0x1)
            {
                clc_counter++;
                if(clc_counter >=100)
                {
                    clc_notice("wait for SPM edge interrupt = 0 : failed. (0x%x, 0x%x)\n", spm_read(SPM_SLEEP_ISR_STATUS), clc_counter);
                }
            }
        }
        else // Core 1,2,3  Keep original IRQ
        {
            #if 0
            switch (core_id)
            {
                case 1 : mt_PPI_mask_all(&MCDI_cpu_1_PPI_mask); break;                     
                case 2 : mt_PPI_mask_all(&MCDI_cpu_2_PPI_mask); break;                      
                case 3 : mt_PPI_mask_all(&MCDI_cpu_3_PPI_mask); break;             
                default : break;
            }
            #endif        

            /* disableNS and disableS mask all cpu interface*/
            spm_val = spm_read((GIC_CPU_BASE + GIC_CPU_CTRL));
            spm_val= spm_val & (~0x3U);
            spm_write((GIC_CPU_BASE + GIC_CPU_CTRL), spm_val);

            switch (core_id)
            {
                case 1 : spm_write(SPM_CORE1_CON, SPM_MCDI_CORE_SEL); break;                     
                case 2 : spm_write(SPM_CORE2_CON, SPM_MCDI_CORE_SEL); break;                     
                case 3 : spm_write(SPM_CORE3_CON, SPM_MCDI_CORE_SEL); break;                     
                default : break;
            }
            
            //if(MCDI_Test_Mode == 1)
                clc_notice("core_%d enter wfi.\n", core_id);
                //clc_notice("a\n");

            if (!cpu_power_down(DORMANT_MODE)) 
            {
                switch_to_amp();  

                /* do not add code here */
                mcdi_wfi_with_sync();
            }
            
            switch_to_smp();                        
            
            cpu_check_dormant_abort();

            //if(MCDI_Test_Mode == 1)
                clc_notice("core_%d exit wfi.\n", core_id);
                //clc_notice("b\n");
            
            // debug info ===========================================================================
            #if 0
            clc_notice("SPM_PCM_REG5_DATA   0x%x = 0x%x\n", SPM_PCM_REG5_DATA          , spm_read(SPM_PCM_REG5_DATA));
            clc_notice("SPM_PCM_REG7_DATA   0x%x = 0x%x\n", SPM_PCM_REG7_DATA          , spm_read(SPM_PCM_REG7_DATA));
            clc_notice("SPM_PCM_REG12_DATA   0x%x = 0x%x\n", SPM_PCM_REG12_DATA          , spm_read(SPM_PCM_REG12_DATA));
            clc_notice("SPM_PCM_REG13_DATA   0x%x = 0x%x\n", SPM_PCM_REG13_DATA          , spm_read(SPM_PCM_REG13_DATA));
            clc_notice("SPM_PCM_REG14_DATA   0x%x = 0x%x\n", SPM_PCM_REG14_DATA          , spm_read(SPM_PCM_REG14_DATA));
            clc_notice("SPM_APMCU_PWRCTL   0x%x = 0x%x\n", SPM_APMCU_PWRCTL          , spm_read(SPM_APMCU_PWRCTL));
            clc_notice("SPM_AP_STANBY_CON   0x%x = 0x%x\n", SPM_AP_STANBY_CON          , spm_read(SPM_AP_STANBY_CON));
            clc_notice("SPM_CLK_CON   0x%x = 0x%x\n", SPM_CLK_CON          , spm_read(SPM_CLK_CON));
            clc_notice("[0x%x] = 0x%x\n", 0xf0211100 , spm_read(0xf0211100));
            clc_notice("[0x%x] = 0x%x\n", 0xf0211200 , spm_read(0xf0211200));

            clc_notice("GPT 01 ======================================================\n");
            clc_notice("GPT status [0x%x] = 0x%x\n", 0xf0008004 , spm_read(0xf0008004));
            {
            u32 temp_address;
            
            for( temp_address = 0xf0211100 ; temp_address <= 0xf0211200 ; temp_address+=4 )
            {
                clc_notice("[0x%x] = 0x%x\n", temp_address , spm_read(temp_address));
            }
            }
            #endif
            // ==================================================================================

            if(MCDI_Test_Mode == 1)
            {
                // read/clear XGPT status
                if(core_id == 1)
                {
                    if(((spm_read(0xf0008004)>>0) & 0x1) == 0x1 )
                    {
                        spm_write(0xf0008008, (0x1<<0));
                    }
                }
                else if(core_id == 2)
                {
                    if(((spm_read(0xf0008004)>>3) & 0x1) == 0x1 )
                    {
                        spm_write(0xf0008008, (0x1<<3));
                    }
                }
                else if(core_id == 3)
                {
                    if(((spm_read(0xf0008004)>>4) & 0x1) == 0x1 )
                    {
                        spm_write(0xf0008008, (0x1<<4));
                    }
                }

                mdelay(10);  // delay 10 ms
            }

            /* EnableNS and EnableS unmask all cpu interface*/
            spm_val = spm_read((GIC_CPU_BASE + GIC_CPU_CTRL));
            spm_val = spm_val | (0x3);
            spm_write((GIC_CPU_BASE + GIC_CPU_CTRL), spm_val);
            
            #if 0
            switch (core_id)
            {
                case 1 : mt_PPI_mask_restore(&MCDI_cpu_1_PPI_mask); break;                     
                case 2 : mt_PPI_mask_restore(&MCDI_cpu_2_PPI_mask); break;                      
                case 3 : mt_PPI_mask_restore(&MCDI_cpu_3_PPI_mask); break;             
                default : break;
            }
            #endif
        }
}    

int spm_wfi_for_sodi_test(void *sodi_data)
{   
    volatile u32 do_not_change_it;
    volatile u32 lo, hi, core_id;
    unsigned long flags;

    preempt_disable();
    do_not_change_it = 1;
    MCDI_Test_Mode = 1;

    while(do_not_change_it)     
    {
        /* Mask ARM i bit */
        local_irq_save(flags);
    
        core_id = (u32)smp_processor_id();

        // set local timer & GPT =========================================
        switch (core_id)
        {
            case 0 : 
                read_cntp_cval(lo, hi);
                hi+=0xffffffff; // very very long
                write_cntp_cval(lo, hi);
                write_cntp_ctl(0x1);  // CNTP_CTL_ENABLE
            break;       
            
            case 1 : 
                spm_write(0xf0008010, 0x0); // disable GPT
            break;          
            
            case 2 : 
                spm_write(0xf0008040, 0x0); // disable GPT
            break;        
            
            case 3 : 
                spm_write(0xf0008050, 0x0); // disable GPT
            break;        
            
            default : break;
        }    

        spm_mcdi_wfi();

        /* Un-Mask ARM i bit */
        local_irq_restore(flags);
    }

    preempt_enable();
    return 0;
}

int spm_wfi_for_mcdi_test(void *mcdi_data)
{   
    volatile u32 do_not_change_it;
    volatile u32 lo, hi, core_id;
    unsigned long flags;

    preempt_disable();
    do_not_change_it = 1;
    MCDI_Test_Mode = 1;

    while(do_not_change_it)     
    {
        /* Mask ARM i bit */
        local_irq_save(flags);
    
        core_id = (u32)smp_processor_id();

        // set local timer & GPT =========================================
        switch (core_id)
        {
            case 0 : 
                read_cntp_cval(lo, hi);
                lo+=5070000; // 390 ms, 13MHz
                write_cntp_cval(lo, hi);
                write_cntp_ctl(0x1);  // CNTP_CTL_ENABLE
            break;       
            
            case 1 : 
                //gpt_set_cmp(GPT1, 2470000); // 190ms, 13MHz
                //start_gpt(GPT1);
                spm_write(0xf000801c, 2470000); // write compare
                spm_write(0xf0008010, 0x1); // enable GPT
            break;          
            
            case 2 : 
                //gpt_set_cmp(GPT4, 1170000); // 90ms, 13MHz
                //start_gpt(GPT4);
                spm_write(0xf000804c, 1170000); // write compare
                spm_write(0xf0008040, 0x1); // enable GPT
            break;        
            
            case 3 : 
                //gpt_set_cmp(GPT5, 520000); // 40ms, 13MHz
                //start_gpt(GPT5);
                spm_write(0xf000805c, 520000); // write compare
                spm_write(0xf0008050, 0x1); // enable GPT
            break;        
            
            default : break;
        }

        spm_mcdi_wfi();
        
        /* Un-Mask ARM i bit */
        local_irq_restore(flags);
    }
    
    preempt_enable();
    return 0;
}    

void spm_check_core_status_before(u32 target_core)
{
    #if 1
    u32 target_core_temp,hotplug_out_core_id;
    volatile u32 core_id;

    if(En_SPM_MCDI != 1)
    {
        return;
    }
    
    core_id = (u32)smp_processor_id();
    
    target_core_temp = target_core & 0xf;

    hotplug_out_core_id = ((spm_read(SPM_MP_CORE0_AUX) & 0x1)<<0) | ((spm_read(SPM_MP_CORE1_AUX) & 0x1)<<1) | 
    	                              ((spm_read(SPM_MP_CORE2_AUX) & 0x1)<<2) | ((spm_read(SPM_MP_CORE3_AUX) & 0x1)<<3);

    target_core_temp &= (~hotplug_out_core_id);

    //clc_notice("issue IPI, spm_check_core_status_before = 0x%x\n", target_core_temp);

    if( target_core_temp == 0x0)
    {
        return;
    }

    // set IPI SPM register ==================================================

    switch (core_id)
    {
        case 0 : spm_write(SPM_MP_CORE0_AUX, (spm_read(SPM_MP_CORE0_AUX) | (target_core_temp << 1)) );  break;                     
        case 1 : spm_write(SPM_MP_CORE1_AUX, (spm_read(SPM_MP_CORE1_AUX) | (target_core_temp << 1)) );  break;                     
        case 2 : spm_write(SPM_MP_CORE2_AUX, (spm_read(SPM_MP_CORE2_AUX) | (target_core_temp << 1)) );  break;                     
        case 3 : spm_write(SPM_MP_CORE3_AUX, (spm_read(SPM_MP_CORE3_AUX) | (target_core_temp << 1)) );  break;                     
        default : break;
    }
    
    #if 0
    // check CPU wake up ==============================================
    {
    u32 clc_counter, spm_core_pws;
    clc_counter = 0;
    
    while(1)
    {    
        // power_state => 1: power down
        spm_core_pws = ((spm_read(SPM_FC0_PWR_CON) == 0x4d) ? 0 : 1) | ((spm_read(SPM_FC1_PWR_CON) == 0x4d) ? 0 : 2) |
                                 ((spm_read(SPM_FC2_PWR_CON) == 0x4d) ? 0 : 4) | ((spm_read(SPM_FC3_PWR_CON) == 0x4d) ? 0 : 8); // power_state => 1: power down

        if( (target_core_temp & ((~spm_core_pws) & 0xf)) == target_core_temp )
        {
            break;
        }
        
        clc_counter++;

        if(clc_counter >= 1000)
        {
            clc_notice("spm_check_core_status_before : check CPU wake up failed.(0x%x, 0x%x)\n", target_core_temp, ((~spm_core_pws) & 0xf));
            break;
        }
    }
    }
    #endif
    
    #endif
}

void spm_check_core_status_after(u32 target_core)
{
    #if 1
    u32 target_core_temp, clc_counter, spm_core_pws, hotplug_out_core_id;
    volatile u32 core_id;

    if(En_SPM_MCDI != 1)
    {
        return;
    }
    
    core_id = (u32)smp_processor_id();
    
    target_core_temp = target_core & 0xf;

    hotplug_out_core_id = ((spm_read(SPM_MP_CORE0_AUX) & 0x1)<<0) | ((spm_read(SPM_MP_CORE1_AUX) & 0x1)<<1) | 
    	                              ((spm_read(SPM_MP_CORE2_AUX) & 0x1)<<2) | ((spm_read(SPM_MP_CORE3_AUX) & 0x1)<<3);

    target_core_temp &= (~hotplug_out_core_id);

    //clc_notice("issue IPI, spm_check_core_status_after = 0x%x\n", target_core_temp);

    if( target_core_temp == 0x0)
    {
        return;
    }

    // check CPU wake up ==============================================

    clc_counter = 0;
    
    while(1)
    {    
        // power_state => 1: power down
        spm_core_pws = ((spm_read(SPM_FC0_PWR_CON) == 0x4d) ? 0 : 1) | ((spm_read(SPM_FC1_PWR_CON) == 0x4d) ? 0 : 2) |
                                 ((spm_read(SPM_FC2_PWR_CON) == 0x4d) ? 0 : 4) | ((spm_read(SPM_FC3_PWR_CON) == 0x4d) ? 0 : 8); // power_state => 1: power down

        if( (target_core_temp & ((~spm_core_pws) & 0xf)) == target_core_temp )
        {
            break;
        }
        
        clc_counter++;

        if(clc_counter >= 100)
        {
            clc_notice("spm_check_core_status_after : check CPU wake up failed.(0x%x, 0x%x)\n", target_core_temp, ((~spm_core_pws) & 0xf));
            break;
        }
    }

    // clear IPI SPM register ==================================================
    
    switch (core_id)
    {
        case 0 : spm_write(SPM_MP_CORE0_AUX, (spm_read(SPM_MP_CORE0_AUX) & (~(target_core_temp << 1))) );  break;                     
        case 1 : spm_write(SPM_MP_CORE1_AUX, (spm_read(SPM_MP_CORE1_AUX) & (~(target_core_temp << 1))) );  break;                     
        case 2 : spm_write(SPM_MP_CORE2_AUX, (spm_read(SPM_MP_CORE2_AUX) & (~(target_core_temp << 1))) );  break;                     
        case 3 : spm_write(SPM_MP_CORE3_AUX, (spm_read(SPM_MP_CORE3_AUX) & (~(target_core_temp << 1))) );  break;                     
        default : break;
    }
    #endif
}

void spm_hot_plug_in_before(u32 target_core)
{    
    clc_notice("spm_hot_plug_in_before()........ target_core = 0x%x\n", target_core);

    #if 1
    switch (target_core)
    {
        case 0 : spm_write(SPM_MP_CORE0_AUX, (spm_read(SPM_MP_CORE0_AUX) & (~0x1U)));  break;                     
        case 1 : spm_write(SPM_MP_CORE1_AUX, (spm_read(SPM_MP_CORE1_AUX) & (~0x1U)));  break;                     
        case 2 : spm_write(SPM_MP_CORE2_AUX, (spm_read(SPM_MP_CORE2_AUX) & (~0x1U)));  break;                     
        case 3 : spm_write(SPM_MP_CORE3_AUX, (spm_read(SPM_MP_CORE3_AUX) & (~0x1U)));  break;                     
        default : break;
    }
    #else
    switch (target_core)
    {
        case 0 : spm_write(SPM_MP_CORE0_AUX, 0x0);  break;                     
        case 1 : spm_write(SPM_MP_CORE1_AUX, 0x0);  break;                     
        case 2 : spm_write(SPM_MP_CORE2_AUX, 0x0);  break;                     
        case 3 : spm_write(SPM_MP_CORE3_AUX, 0x0);  break;                     
        default : break;
    }
    #endif
}

void spm_hot_plug_out_after(u32 target_core)
{
    clc_notice("spm_hot_plug_out_after()........ target_core = 0x%x\n", target_core);
    
    #if 1
    switch (target_core)
    {
        case 0 : spm_write(SPM_MP_CORE0_AUX, (spm_read(SPM_MP_CORE0_AUX) | 0x1));  break;                     
        case 1 : spm_write(SPM_MP_CORE1_AUX, (spm_read(SPM_MP_CORE1_AUX) | 0x1));  break;                     
        case 2 : spm_write(SPM_MP_CORE2_AUX, (spm_read(SPM_MP_CORE2_AUX) | 0x1));  break;                     
        case 3 : spm_write(SPM_MP_CORE3_AUX, (spm_read(SPM_MP_CORE3_AUX) | 0x1));  break;                     
        default : break;
    }
    #else
    switch (target_core)
    {
        case 0 : spm_write(SPM_MP_CORE0_AUX, 0x1);  break;                     
        case 1 : spm_write(SPM_MP_CORE1_AUX, 0x1);  break;                     
        case 2 : spm_write(SPM_MP_CORE2_AUX, 0x1);  break;                     
        case 3 : spm_write(SPM_MP_CORE3_AUX, 0x1);  break;                     
        default : break;
    }
    #endif
}

static void spm_direct_disable_sodi(void)
{
    u32 clc_temp;

    clc_temp = spm_read(SPM_CLK_CON);
    clc_temp |= (0x1<<13);
    
    spm_write(SPM_CLK_CON, clc_temp);  
}

static void spm_direct_enable_sodi(void)
{
    u32 clc_temp;

    clc_temp = spm_read(SPM_CLK_CON);
    clc_temp &= 0xffffdfff; // ~(0x1<<13);

    spm_write(SPM_CLK_CON, clc_temp);  
}

void spm_disable_sodi(void)
{
    spin_lock(&spm_sodi_lock);

    spm_sodi_disable_counter++;
    clc_debug("spm_disable_sodi() : spm_sodi_disable_counter = 0x%x\n", spm_sodi_disable_counter);    

    if(spm_sodi_disable_counter > 0)
    {
        spm_direct_disable_sodi();
    }

    spin_unlock(&spm_sodi_lock);
}

void spm_enable_sodi(void)
{
    spin_lock(&spm_sodi_lock);

    spm_sodi_disable_counter--;
    clc_debug("spm_enable_sodi() : spm_sodi_disable_counter = 0x%x\n", spm_sodi_disable_counter);    
    
    if(spm_sodi_disable_counter <= 0)
    {
        spm_direct_enable_sodi();
    }

    spin_unlock(&spm_sodi_lock);
}


// ==============================================================================

static int spm_mcdi_probe(struct platform_device *pdev)
{
    int hibboot = 0;
    hibboot = get_env("hibboot") == NULL ? 0 : simple_strtol(get_env("hibboot"), NULL, 10);

    // set SPM_MP_CORE0_AUX
    spm_write(SPM_MP_CORE0_AUX, 0x0);
    spm_write(SPM_MP_CORE1_AUX, 0x0);
    spm_write(SPM_MP_CORE2_AUX, 0x0);
    spm_write(SPM_MP_CORE3_AUX, 0x0);

    #if MCDI_KICK_PCM
    clc_notice("spm_mcdi_probe start.\n");        
    if (1 == hibboot)
    {
        clc_notice("[%s] skip spm_go_to_MCDI due to hib boot\n", __func__);
    }
    else
    {
        spm_go_to_MCDI();  
    } 
    #endif
    
    return 0;
}

static void spm_mcdi_early_suspend(struct early_suspend *h) 
{
    #if MCDI_KICK_PCM
    clc_notice("spm_mcdi_early_suspend start.\n");
    spm_leave_MCDI();    
    #endif
}

static void spm_mcdi_late_resume(struct early_suspend *h) 
{
    #if MCDI_KICK_PCM
    clc_notice("spm_mcdi_late_resume start.\n");
    spm_go_to_MCDI();    
    #endif
}

static struct platform_driver mtk_spm_mcdi_driver = {
    .remove     = NULL,
    .shutdown   = NULL,
    .probe      = spm_mcdi_probe,
    .suspend	= NULL,
    .resume		= NULL,
    .driver     = {
        .name = "mtk-spm-mcdi",
    },
};

static struct early_suspend mtk_spm_mcdi_early_suspend_driver =
{
    .level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 251,
    .suspend = spm_mcdi_early_suspend,
    .resume  = spm_mcdi_late_resume,
};

void spm_mcdi_LDVT_sodi(void)
{
    clc_notice("spm_mcdi_LDVT_sodi() start.\n");
    mtk_wdt_suspend();    

    // show image on screen ============================
    spm_show_lcm_image();

    // spm_direct_enable_sodi ============================
    spm_direct_enable_sodi();    

    mcdi_task_0 = kthread_create(spm_wfi_for_sodi_test, NULL, "mcdi_task_0");
    mcdi_task_1 = kthread_create(spm_wfi_for_sodi_test, NULL, "mcdi_task_1");
    mcdi_task_2 = kthread_create(spm_wfi_for_sodi_test, NULL, "mcdi_task_2");
    mcdi_task_3 = kthread_create(spm_wfi_for_sodi_test, NULL, "mcdi_task_3");

    if(IS_ERR(mcdi_task_0) ||IS_ERR(mcdi_task_1) ||IS_ERR(mcdi_task_2) ||IS_ERR(mcdi_task_3))
    {  
         clc_notice("Unable to start kernel thread(0x%x, 0x%x, 0x%x, 0x%x)./n", (u32)IS_ERR(mcdi_task_0), (u32)IS_ERR(mcdi_task_1), (u32)IS_ERR(mcdi_task_2), (u32)IS_ERR(mcdi_task_3));  
         mcdi_task_0 = NULL;
         mcdi_task_1 = NULL;
         mcdi_task_2 = NULL;
         mcdi_task_3 = NULL;
    }  

    kthread_bind(mcdi_task_0, 0);
    kthread_bind(mcdi_task_1, 1);
    kthread_bind(mcdi_task_2, 2);
    kthread_bind(mcdi_task_3, 3);

    wake_up_process(mcdi_task_0);
    wake_up_process(mcdi_task_1);
    wake_up_process(mcdi_task_2);
    wake_up_process(mcdi_task_3);
    
    clc_notice("spm_mcdi_LDVT_01() end.\n");
}

void spm_mcdi_LDVT_mcdi(void)
{
    clc_notice("spm_mcdi_LDVT_mcdi() start.\n");
    mtk_wdt_suspend();    

    // spm_direct_disable_sodi ============================
    spm_direct_disable_sodi();    

#if 0
{
    u32 mcdi_error;

    // init GPT ==================================
    free_gpt(GPT1);
    free_gpt(GPT4);
    free_gpt(GPT5);

    mcdi_error = request_gpt(GPT1, GPT_ONE_SHOT, GPT_CLK_SRC_SYS, GPT_CLK_DIV_1, 0, NULL, GPT_NOAUTOEN);
    if(mcdi_error != 0)
    {
        clc_notice("GPT1 init failed.(0x%x)\n", mcdi_error);
    }
    
    mcdi_error = request_gpt(GPT4, GPT_ONE_SHOT, GPT_CLK_SRC_SYS, GPT_CLK_DIV_1, 0, NULL, GPT_NOAUTOEN);
    if(mcdi_error != 0)
    {
        clc_notice("GPT4 init failed.(0x%x)\n", mcdi_error);
    }
    
    mcdi_error = request_gpt(GPT5, GPT_ONE_SHOT, GPT_CLK_SRC_SYS, GPT_CLK_DIV_1, 0, NULL, GPT_NOAUTOEN);
    if(mcdi_error != 0)
    {
        clc_notice("GPT5 init failed.(0x%x)\n", mcdi_error);
    }
}
#else
    // init GPT ==================================
    spm_write(0xf0008010, 0x2);  //clear GPT1 count
    spm_write(0xf0008040, 0x2);  //clear GPT4 count
    spm_write(0xf0008050, 0x2);  //clear GPT5 count

    spm_write(0xf0008000, (spm_read(0xf0008000) | 0x19));  //enable GPT1, 4, 5 IRQ
#endif

#if 0
    smp_call_function(spm_mcdi_wfi_for_test, NULL, 0);
    spm_mcdi_wfi_for_test();
#else
    mcdi_task_0 = kthread_create(spm_wfi_for_mcdi_test, NULL, "mcdi_task_0");
    mcdi_task_1 = kthread_create(spm_wfi_for_mcdi_test, NULL, "mcdi_task_1");
    mcdi_task_2 = kthread_create(spm_wfi_for_mcdi_test, NULL, "mcdi_task_2");
    mcdi_task_3 = kthread_create(spm_wfi_for_mcdi_test, NULL, "mcdi_task_3");

    if(IS_ERR(mcdi_task_0) ||IS_ERR(mcdi_task_1) ||IS_ERR(mcdi_task_2) ||IS_ERR(mcdi_task_3))
    {  
         clc_notice("Unable to start kernel thread(0x%x, 0x%x, 0x%x, 0x%x)./n", (u32)IS_ERR(mcdi_task_0), (u32)IS_ERR(mcdi_task_1), (u32)IS_ERR(mcdi_task_2), (u32)IS_ERR(mcdi_task_3));  
         mcdi_task_0 = NULL;
         mcdi_task_1 = NULL;
         mcdi_task_2 = NULL;
         mcdi_task_3 = NULL;
    }  

    kthread_bind(mcdi_task_0, 0);
    kthread_bind(mcdi_task_1, 1);
    kthread_bind(mcdi_task_2, 2);
    kthread_bind(mcdi_task_3, 3);

    wake_up_process(mcdi_task_0);
    wake_up_process(mcdi_task_1);
    wake_up_process(mcdi_task_2);
    wake_up_process(mcdi_task_3);
#endif
}

/***************************
* show current SPM-MCDI stauts
****************************/
static int spm_mcdi_debug_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;

    if (SPM_MCDI_CORE_SEL)
        p += sprintf(p, "SPM MCDI+Thermal Protect enabled.\n");
    else
        p += sprintf(p, "SPM MCDI disabled, Thermal Protect enabled.\n");

    len = p - buf;
    return len;
}

/************************************
* set SPM-MCDI stauts by sysfs interface
*************************************/
static ssize_t spm_mcdi_debug_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int enabled = 0;

    if (sscanf(buffer, "%d", &enabled) == 1)
    {
        if (enabled == 0)
        {
            spm_leave_MCDI();    
            SPM_MCDI_CORE_SEL = 0;
        }
        else if (enabled == 1)
        {
            SPM_MCDI_CORE_SEL = 1;
            spm_go_to_MCDI();  
        }
        else if (enabled == 2)
        {
            SPM_MCDI_CORE_SEL = 1;
            clc_notice("spm_mcdi_LDVT_sodi() (argument_0 = %d)\n", enabled);
            spm_mcdi_LDVT_sodi();    
        }
        else if (enabled == 3)
        {
            clc_notice("spm_mcdi_LDVT_mcdi() (argument_0 = %d)\n", enabled);
            spm_mcdi_LDVT_mcdi();    
        }
        else if (enabled == 4)
        {
            clc_notice("Peri_0      ( 1: off) 0x%x = 0x%x\n", 0xf0003018	          , spm_read(0xf0003018));
            clc_notice("Peri_1      ( 1: off) 0x%x = 0x%x\n", 0xf000301c	          , spm_read(0xf000301c));
            clc_notice("Infra        ( 1: off) 0x%x = 0x%x\n", 0xf0001048	          , spm_read(0xf0001048));
            clc_notice("TopPCK    ( 1: off) 0x%x = 0x%x\n", 0xf0000178            , spm_read(0xf0000178));

            clc_notice("display_0 ( 1: off) 0x%x = 0x%x\n", 0xf4000100	          , spm_read(0xf4000100));
            clc_notice("display_1 ( 1: off) 0x%x = 0x%x\n", 0xf4000110	          , spm_read(0xf4000110));
            clc_notice("IMG         ( 1: off) 0x%x = 0x%x\n", 0xf5000000            , spm_read(0xf5000000));
            clc_notice("MFG         ( 1: off) 0x%x = 0x%x\n", 0xf0206000	          , spm_read(0xf0206000));
            
            clc_notice("Audio       ( 1: off) 0x%x = 0x%x\n", 0xf2070000	          , spm_read(0xf2070000));
            clc_notice("VDEC0     ( 0: off) 0x%x = 0x%x\n", 0xf6000000	          , spm_read(0xf6000000));
            clc_notice("VDEC1     ( 0: off) 0x%x = 0x%x\n", 0xf6000008	          , spm_read(0xf6000008));
            clc_notice("VEN         ( 0: off) 0x%x = 0x%x\n", 0xf7000000	          , spm_read(0xf7000000));
        }
        else
        {
            clc_notice("bad argument_0!! (argument_0 = %d)\n", enabled);
        }
    }
    else
    {
            clc_notice("bad argument_1!! \n");
    }

    return count;
}

/************************************
* set SPM-SODI Enable by sysfs interface
*************************************/
static ssize_t spm_user_sodi_en(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int enabled = 0;

    if (sscanf(buffer, "%d", &enabled) == 1)
    {
        if (enabled == 0)
        {
            spm_disable_sodi();  
        }
        else if (enabled == 1)
        {
            spm_enable_sodi();    
        }
    }
    else
    {
            clc_notice("bad argument_1!! \n");
    }

    return count;
}


static int __init spm_mcdi_init(void)
{
    struct proc_dir_entry *mt_entry = NULL;    
    struct proc_dir_entry *mt_mcdi_dir = NULL;
    int mcdi_err = 0;

    mt_mcdi_dir = proc_mkdir("mcdi", NULL);
    if (!mt_mcdi_dir)
    {
        clc_notice("[%s]: mkdir /proc/mcdi failed\n", __FUNCTION__);
    }
    else
    {
        mt_entry = create_proc_entry("mcdi_debug", S_IRUGO | S_IWUSR | S_IWGRP, mt_mcdi_dir);
        if (mt_entry)
        {
            mt_entry->read_proc = spm_mcdi_debug_read;
            mt_entry->write_proc = spm_mcdi_debug_write;
        }

        mt_entry = create_proc_entry("sodi_en", S_IRUGO | S_IWUSR | S_IWGRP, mt_mcdi_dir);
        if (mt_entry)
        {
            mt_entry->write_proc = spm_user_sodi_en;
        }
    }

    mcdi_err = platform_driver_register(&mtk_spm_mcdi_driver);
    
    if (mcdi_err)
    {
        clc_notice("spm mcdi driver callback register failed..\n");
        return mcdi_err;
    }

    clc_notice("spm mcdi driver callback register OK..\n");

    register_early_suspend(&mtk_spm_mcdi_early_suspend_driver);

    clc_notice("spm mcdi driver early suspend callback register OK..\n");
    
    return 0;
}

static void __exit spm_mcdi_exit(void)
{
    clc_notice("Exit SPM-MCDI\n\r");
}

late_initcall(spm_mcdi_init);

MODULE_DESCRIPTION("MT6589 SPM-Idle Driver v0.1");
