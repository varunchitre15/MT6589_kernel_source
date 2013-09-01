#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>    //udelay

#include <mach/mt_typedefs.h>
#include <mach/mt_spm.h>
#include <mach/mt_spm_mtcmos.h>

/**************************************
 * for CPU MTCMOS
 **************************************/
/*
 * regiser bit difinition
 */
/* SPM_FC1_PWR_CON */
/* SPM_FC2_PWR_CON */
/* SPM_FC3_PWR_CON */
#define SRAM_ISOINT_B   (1U << 6)
#define SRAM_CKISO      (1U << 5)
#define PWR_CLK_DIS     (1U << 4)
#define PWR_ON_S        (1U << 3)
#define PWR_ON          (1U << 2)
#define PWR_ISO         (1U << 1)
#define PWR_RST_B       (1U << 0)

/* SPM_PWR_STATUS */
/* SPM_PWR_STATUS_S */
#define FC1             (1U << 17)
#define FC2             (1U << 16)
#define FC3             (1U << 15)

/* SPM_SLEEP_TIMER_STA */
#define APMCU3_SLEEP    (1U << 18)
#define APMCU2_SLEEP    (1U << 17)
#define APMCU1_SLEEP    (1U << 16)


static DEFINE_SPINLOCK(spm_cpu_lock);


void spm_mtcmos_cpu_lock(unsigned long *flags)
{
    spin_lock_irqsave(&spm_cpu_lock, *flags);
}

void spm_mtcmos_cpu_unlock(unsigned long *flags)
{
    spin_unlock_irqrestore(&spm_cpu_lock, *flags);
}

int spm_mtcmos_ctrl_cpu0(int state)
{
    if (state == STA_POWER_DOWN) {
        
    } else {    /* STA_POWER_ON */
        
    }

    return 0;
}

int spm_mtcmos_ctrl_cpu1(int state)
{
    unsigned long flags;
    
    /* enable register control */
    spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));
    
    spm_mtcmos_cpu_lock(&flags);
    
    if (state == STA_POWER_DOWN) 
    {
        int i;
        
        while ((spm_read(SPM_SLEEP_TIMER_STA) & APMCU1_SLEEP) == 0);
        
        spm_write(SPM_FC1_PWR_CON, (spm_read(SPM_FC1_PWR_CON) | SRAM_CKISO) & ~SRAM_ISOINT_B);
        for (i = 0; i < 16; ++i)
        {
            spm_write(SPM_CPU_FC1_L1_PDN, spm_read(SPM_CPU_FC1_L1_PDN) | (1 << i));
            //delay 5ns??
            ndelay(5);
        }
        
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | PWR_ISO);
        spm_write(SPM_FC1_PWR_CON, (spm_read(SPM_FC1_PWR_CON) | PWR_CLK_DIS) & ~PWR_RST_B);
        
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~PWR_ON);
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~PWR_ON_S);
        while (((spm_read(SPM_PWR_STATUS) & FC1) != 0) | ((spm_read(SPM_PWR_STATUS_S) & FC1) != 0));
    } 
    else /* STA_POWER_ON */
    {
        int i;
        
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | PWR_ON);
        udelay(1);
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | PWR_ON_S);
        while (((spm_read(SPM_PWR_STATUS) & FC1) != FC1) | ((spm_read(SPM_PWR_STATUS_S) & FC1) != FC1));
        
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~PWR_CLK_DIS);
        
        for (i = 0; i < 16; ++i)
        {
            spm_write(SPM_CPU_FC1_L1_PDN, spm_read(SPM_CPU_FC1_L1_PDN) & ~(1 << i));
            //delay 5ns??
            ndelay(5);
        }
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | SRAM_ISOINT_B);
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~SRAM_CKISO);
        
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_FC1_PWR_CON, spm_read(SPM_FC1_PWR_CON) | PWR_RST_B);
    }
    
    spm_mtcmos_cpu_unlock(&flags);
    
    return 0;
}

int spm_mtcmos_ctrl_cpu2(int state)
{
    unsigned long flags;
    
    /* enable register control */
    spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));
    
    spm_mtcmos_cpu_lock(&flags);
    
    if (state == STA_POWER_DOWN) 
    {
        int i;
        
        while ((spm_read(SPM_SLEEP_TIMER_STA) & APMCU2_SLEEP) == 0);
        
        spm_write(SPM_FC2_PWR_CON, (spm_read(SPM_FC2_PWR_CON) | SRAM_CKISO) & ~SRAM_ISOINT_B);
        for (i = 0; i < 16; ++i)
        {
            spm_write(SPM_CPU_FC2_L1_PDN, spm_read(SPM_CPU_FC2_L1_PDN) | (1 << i));
            //delay 5ns??
            ndelay(5);
        }
        
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | PWR_ISO);
        spm_write(SPM_FC2_PWR_CON, (spm_read(SPM_FC2_PWR_CON) | PWR_CLK_DIS) & ~PWR_RST_B);
        
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~PWR_ON);
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~PWR_ON_S);
        while (((spm_read(SPM_PWR_STATUS) & FC2) != 0) | ((spm_read(SPM_PWR_STATUS_S) & FC2) != 0));
    } 
    else /* STA_POWER_ON */
    {
        int i;
        
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | PWR_ON);
        udelay(1);
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | PWR_ON_S);
        while (((spm_read(SPM_PWR_STATUS) & FC2) != FC2) | ((spm_read(SPM_PWR_STATUS_S) & FC2) != FC2));
        
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~PWR_CLK_DIS);
        
        for (i = 0; i < 16; ++i)
        {
            spm_write(SPM_CPU_FC2_L1_PDN, spm_read(SPM_CPU_FC2_L1_PDN) & ~(1 << i));
            //delay 5ns??
            ndelay(5);
        }
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | SRAM_ISOINT_B);
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~SRAM_CKISO);
        
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_FC2_PWR_CON, spm_read(SPM_FC2_PWR_CON) | PWR_RST_B);
    }
    
    spm_mtcmos_cpu_unlock(&flags);
    
    return 0;
}

int spm_mtcmos_ctrl_cpu3(int state)
{
    unsigned long flags;
    
    /* enable register control */
    spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));
    
    spm_mtcmos_cpu_lock(&flags);
    
    if (state == STA_POWER_DOWN) 
    {
        int i;
        
        while ((spm_read(SPM_SLEEP_TIMER_STA) & APMCU3_SLEEP) == 0);
        
        spm_write(SPM_FC3_PWR_CON, (spm_read(SPM_FC3_PWR_CON) | SRAM_CKISO) & ~SRAM_ISOINT_B);
        for (i = 0; i < 16; ++i)
        {
            spm_write(SPM_CPU_FC3_L1_PDN, spm_read(SPM_CPU_FC3_L1_PDN) | (1 << i));
            //delay 5ns??
            ndelay(5);
        }
        
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | PWR_ISO);
        spm_write(SPM_FC3_PWR_CON, (spm_read(SPM_FC3_PWR_CON) | PWR_CLK_DIS) & ~PWR_RST_B);
        
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~PWR_ON);
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~PWR_ON_S);
        while (((spm_read(SPM_PWR_STATUS) & FC3) != 0) | ((spm_read(SPM_PWR_STATUS_S) & FC3) != 0));
    } 
    else /* STA_POWER_ON */
    {
        int i;
        
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | PWR_ON);
        udelay(1);
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | PWR_ON_S);
        while (((spm_read(SPM_PWR_STATUS) & FC3) != FC3) | ((spm_read(SPM_PWR_STATUS_S) & FC3) != FC3));
        
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~PWR_CLK_DIS);
        
        for (i = 0; i < 16; ++i)
        {
            spm_write(SPM_CPU_FC3_L1_PDN, spm_read(SPM_CPU_FC3_L1_PDN) & ~(1 << i));
            //delay 5ns??
            ndelay(5);
        }
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | SRAM_ISOINT_B);
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~SRAM_CKISO);
        
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_FC3_PWR_CON, spm_read(SPM_FC3_PWR_CON) | PWR_RST_B);
    }
    
    spm_mtcmos_cpu_unlock(&flags);
    
    return 0;
}

int spm_mtcmos_ctrl_dbg(int state)
{
    if (state == STA_POWER_DOWN) {
        
    } else {    /* STA_POWER_ON */
        
    }

    return 0;
}

int spm_mtcmos_ctrl_cpusys(int state)
{
    if (state == STA_POWER_DOWN) {
        
    } else {    /* STA_POWER_ON */
        
    }

    return 0;
}

bool spm_cpusys_can_power_down(void)
{
    return !(spm_read(SPM_PWR_STATUS) & 0x00038000) &&
           !(spm_read(SPM_PWR_STATUS_S) & 0x00038000);
}


/**************************************
 * for non-CPU MTCMOS
 **************************************/
static DEFINE_SPINLOCK(spm_noncpu_lock);

#if 0
void spm_mtcmos_noncpu_lock(unsigned long *flags)
{
    spin_lock_irqsave(&spm_noncpu_lock, *flags);
}

void spm_mtcmos_noncpu_unlock(unsigned long *flags)
{
    spin_unlock_irqrestore(&spm_noncpu_lock, *flags);
}
#else
#define spm_mtcmos_noncpu_lock(flags)   \
do {    \
    spin_lock_irqsave(&spm_noncpu_lock, flags);  \
} while (0)

#define spm_mtcmos_noncpu_unlock(flags) \
do {    \
    spin_unlock_irqrestore(&spm_noncpu_lock, flags);    \
} while (0)

#endif


#define VDE_PWR_STA_MASK    (0x1 << 8)
#define VEN_PWR_STA_MASK    (0x1 << 7)
#define IFR_PWR_STA_MASK    (0x1 << 6)
#define ISP_PWR_STA_MASK    (0x1 << 5)
#define DIS_PWR_STA_MASK    (0x1 << 3)
#define MFG_PWR_STA_MASK    (0x1 << 4)
#define DPY_PWR_STA_MASK    (0x1 << 2)
#define MD2_PWR_STA_MASK    (0x1 << 1)
#define MD1_PWR_STA_MASK    (0x1 << 0)

#if 0
#define PWR_RST_B           (0x1 << 0)
#define PWR_ISO             (0x1 << 1)
#define PWR_ON              (0x1 << 2)
#define PWR_ON_S            (0x1 << 3)
#define PWR_CLK_DIS         (0x1 << 4)
#endif

#define SRAM_PDN            (0xf << 8)
#define SRAM_ACK            (0xf << 12)

#define SROM_PDN            (0xf << 16)
#define SROM_ACK            (0xf << 20)

#define MD_SRAM_PDN         (0x1 << 8)

#define VDE_SRAM_ACK        (0x1 << 12)
#define VEN_SRAM_ACK        (0xf << 12)
#define IFR_SRAM_ACK        (0xf << 12)
#define ISP_SRAM_ACK        (0x3 << 12)
#define DIS_SRAM_ACK        (0xf << 12)
#define MFG_SRAM_ACK        (0x1 << 12)

#define TOPAXI_SI0_CTL      (INFRACFG_BASE + 0x0200)
#define TOPAXI_PROT_EN      (INFRACFG_BASE + 0x0220)
#define TOPAXI_PROT_STA1    (INFRACFG_BASE + 0x0228)

#define MFG_PROT_MASK   0x0020
#define MD1_PROT_MASK   0x5300
#define MD2_PROT_MASK   0xAC00

#define MFG_SI0_MASK    0x0400


int spm_mtcmos_ctrl_vdec(int state)
{
    int err = 0;
    volatile unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | SRAM_PDN);

        while ((spm_read(SPM_VDE_PWR_CON) & VDE_SRAM_ACK) != VDE_SRAM_ACK) {
        }

        spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_VDE_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_VDE_PWR_CON, val);

        spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~(PWR_ON | PWR_ON_S));

#if 0
        udelay(1); 
        if (spm_read(SPM_PWR_STATUS) & VDE_PWR_STA_MASK) { 
            err = 1;
        }
#else
        while ((spm_read(SPM_PWR_STATUS) & VDE_PWR_STA_MASK) 
                || (spm_read(SPM_PWR_STATUS_S) & VDE_PWR_STA_MASK)) {
        }
#endif
    } else {    /* STA_POWER_ON */
        spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_ON);
        spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_ON_S);
#if 0
        udelay(1);
#else
        while (!(spm_read(SPM_PWR_STATUS) & VDE_PWR_STA_MASK)
                || !(spm_read(SPM_PWR_STATUS_S) & VDE_PWR_STA_MASK)) {
        }
#endif

        spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) | PWR_RST_B);

        spm_write(SPM_VDE_PWR_CON, spm_read(SPM_VDE_PWR_CON) & ~SRAM_PDN);

        while ((spm_read(SPM_VDE_PWR_CON) & VDE_SRAM_ACK)) {
        }

#if 0
        udelay(1); 
        if (!(spm_read(SPM_PWR_STATUS) & VDE_PWR_STA_MASK)) { 
            err = 1;
        }
#endif
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_venc(int state)
{
    int err = 0;
    volatile unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | SRAM_PDN);

        while ((spm_read(SPM_VEN_PWR_CON) & VEN_SRAM_ACK) != VEN_SRAM_ACK) {
        }

        spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_VEN_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_VEN_PWR_CON, val);

        spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~(PWR_ON | PWR_ON_S));

#if 0
        udelay(1); 
        if (spm_read(SPM_PWR_STATUS) & VEN_PWR_STA_MASK) { 
            err = 1;
        }
#else
        while ((spm_read(SPM_PWR_STATUS) & VEN_PWR_STA_MASK)
                || (spm_read(SPM_PWR_STATUS_S) & VEN_PWR_STA_MASK)) {
        }
#endif
    } else {    /* STA_POWER_ON */
        spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_ON);
        spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_ON_S);
#if 0
        udelay(1);
#else
        while (!(spm_read(SPM_PWR_STATUS) & VEN_PWR_STA_MASK)
                || !(spm_read(SPM_PWR_STATUS_S) & VEN_PWR_STA_MASK)) {
        }
#endif

        spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) | PWR_RST_B);

        spm_write(SPM_VEN_PWR_CON, spm_read(SPM_VEN_PWR_CON) & ~SRAM_PDN);

        while ((spm_read(SPM_VEN_PWR_CON) & VEN_SRAM_ACK)) {
        }

#if 0
        udelay(1); 
        if (!(spm_read(SPM_PWR_STATUS) & VDE_PWR_STA_MASK)) { 
            err = 1;
        }
#endif
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_isp(int state)
{
    int err = 0;
    volatile unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | SRAM_PDN);

        while ((spm_read(SPM_ISP_PWR_CON) & ISP_SRAM_ACK) != ISP_SRAM_ACK) {
        }

        spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_ISP_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_ISP_PWR_CON, val);

        spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~(PWR_ON | PWR_ON_S));

#if 0
        udelay(1); 
        if (spm_read(SPM_PWR_STATUS) & ISP_PWR_STA_MASK) { 
            err = 1;
        }
#else
        while ((spm_read(SPM_PWR_STATUS) & ISP_PWR_STA_MASK)
                || (spm_read(SPM_PWR_STATUS_S) & ISP_PWR_STA_MASK)) {
        }
#endif
    } else {    /* STA_POWER_ON */
        spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_ON);
        spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_ON_S);
#if 0
        udelay(1);
#else
        while (!(spm_read(SPM_PWR_STATUS) & ISP_PWR_STA_MASK)
                || !(spm_read(SPM_PWR_STATUS_S) & ISP_PWR_STA_MASK)) {
        }
#endif
        spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) | PWR_RST_B);

        spm_write(SPM_ISP_PWR_CON, spm_read(SPM_ISP_PWR_CON) & ~SRAM_PDN);

        while ((spm_read(SPM_ISP_PWR_CON) & ISP_SRAM_ACK)) {
        }

#if 0
        udelay(1); 
        if (!(spm_read(SPM_PWR_STATUS) & ISP_PWR_STA_MASK)) { 
            err = 1;
        }
#endif
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_disp(int state)
{
    int err = 0;
    volatile unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | SRAM_PDN);
#if 0
        while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK) != DIS_SRAM_ACK) {
        }
#endif
        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_DIS_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_DIS_PWR_CON, val);

        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~(PWR_ON | PWR_ON_S));

#if 0
        udelay(1); 
        if (spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK) { 
            err = 1;
        }
#else
        while ((spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
                || (spm_read(SPM_PWR_STATUS_S) & DIS_PWR_STA_MASK)) {
        }
#endif
    } else {    /* STA_POWER_ON */
        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON);
        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON_S);
#if 0
        udelay(1);
#else
        while (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK) 
                || !(spm_read(SPM_PWR_STATUS_S) & DIS_PWR_STA_MASK)) {
        }
#endif
        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_RST_B);

        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) & ~SRAM_PDN);

#if 0
        while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK)) {
        }
#endif

#if 0
        udelay(1); 
        if (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)) { 
            err = 1;
        }
#endif
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_mfg(int state)
{
    int err = 0;
    volatile unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | MFG_PROT_MASK);
        while ((spm_read(TOPAXI_PROT_STA1) & MFG_PROT_MASK) != MFG_PROT_MASK) {
        }

        spm_write(TOPAXI_SI0_CTL, spm_read(TOPAXI_SI0_CTL) & ~MFG_SI0_MASK);

        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | SRAM_PDN);

        while ((spm_read(SPM_MFG_PWR_CON) & MFG_SRAM_ACK) != MFG_SRAM_ACK) {
        }

        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_MFG_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_MFG_PWR_CON, val);

        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~(PWR_ON | PWR_ON_S));

#if 0
        udelay(1); 
        if (spm_read(SPM_PWR_STATUS) & MFG_PWR_STA_MASK) { 
            err = 1;
        }
#else
        while ((spm_read(SPM_PWR_STATUS) & MFG_PWR_STA_MASK) 
                || (spm_read(SPM_PWR_STATUS_S) & MFG_PWR_STA_MASK)) {
        }
#endif
    } else {    /* STA_POWER_ON */
        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ON);
        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ON_S);
#if 0
        udelay(1);
#else
        while (!(spm_read(SPM_PWR_STATUS) & MFG_PWR_STA_MASK) || 
                !(spm_read(SPM_PWR_STATUS_S) & MFG_PWR_STA_MASK)) {
        }
#endif

        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_RST_B);

        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~SRAM_PDN);

        while ((spm_read(SPM_MFG_PWR_CON) & MFG_SRAM_ACK)) {
        }

#if 0
        udelay(1); 
        if (!(spm_read(SPM_PWR_STATUS) & MFG_PWR_STA_MASK)) { 
            err = 1;
        }
#endif
        spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~MFG_PROT_MASK);
        while (spm_read(TOPAXI_PROT_STA1) & MFG_PROT_MASK) {
        }
        spm_write(TOPAXI_SI0_CTL, spm_read(TOPAXI_SI0_CTL) | MFG_SI0_MASK);
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_infra(int state)
{
    int err = 0;
    volatile unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(SPM_IFR_PWR_CON, spm_read(SPM_IFR_PWR_CON) | SRAM_PDN);

        while ((spm_read(SPM_IFR_PWR_CON) & IFR_SRAM_ACK) != IFR_SRAM_ACK) {
        }

        spm_write(SPM_IFR_PWR_CON, spm_read(SPM_IFR_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_IFR_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_IFR_PWR_CON, val);

        spm_write(SPM_IFR_PWR_CON, spm_read(SPM_IFR_PWR_CON) & ~(PWR_ON | PWR_ON_S));

#if 0
        udelay(1); 
        if (spm_read(SPM_PWR_STATUS) & IFR_PWR_STA_MASK) { 
            err = 1;
        }
#else
        while ((spm_read(SPM_PWR_STATUS) & IFR_PWR_STA_MASK) 
                || (spm_read(SPM_PWR_STATUS_S) & IFR_PWR_STA_MASK)) {
        }
#endif
    } else {    /* STA_POWER_ON */
        spm_write(SPM_IFR_PWR_CON, spm_read(SPM_IFR_PWR_CON) | PWR_ON);
        spm_write(SPM_IFR_PWR_CON, spm_read(SPM_IFR_PWR_CON) | PWR_ON_S);
#if 0
        udelay(1);
#else
        while (!(spm_read(SPM_PWR_STATUS) & IFR_PWR_STA_MASK) 
                || !(spm_read(SPM_PWR_STATUS_S) & IFR_PWR_STA_MASK)) {
        }
#endif

        spm_write(SPM_IFR_PWR_CON, spm_read(SPM_IFR_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_IFR_PWR_CON, spm_read(SPM_IFR_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_IFR_PWR_CON, spm_read(SPM_IFR_PWR_CON) | PWR_RST_B);

        spm_write(SPM_IFR_PWR_CON, spm_read(SPM_IFR_PWR_CON) & ~SRAM_PDN);

        while ((spm_read(SPM_IFR_PWR_CON) & IFR_SRAM_ACK)) {
        }

#if 0
        udelay(1); 
        if (!(spm_read(SPM_PWR_STATUS) & IFR_PWR_STA_MASK)) { 
            err = 1;
        }
#endif
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_ddrphy(int state)
{
    int err = 0;
    volatile unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(SPM_DPY_PWR_CON, spm_read(SPM_DPY_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_DPY_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_DPY_PWR_CON, val);

        spm_write(SPM_DPY_PWR_CON, spm_read(SPM_DPY_PWR_CON) & ~(PWR_ON | PWR_ON_S));

#if 0
        udelay(1); 
        if (spm_read(SPM_PWR_STATUS) & DPY_PWR_STA_MASK) { 
            err = 1;
        }
#else
        while ((spm_read(SPM_PWR_STATUS) & DPY_PWR_STA_MASK) 
                || (spm_read(SPM_PWR_STATUS_S) & DPY_PWR_STA_MASK)) {
        }
#endif
    } else {    /* STA_POWER_ON */
        spm_write(SPM_DPY_PWR_CON, spm_read(SPM_DPY_PWR_CON) | PWR_ON);
        spm_write(SPM_DPY_PWR_CON, spm_read(SPM_DPY_PWR_CON) | PWR_ON_S);
#if 0
        udelay(1);
#else
        while (!(spm_read(SPM_PWR_STATUS) & DPY_PWR_STA_MASK) 
                || !(spm_read(SPM_PWR_STATUS_S) & DPY_PWR_STA_MASK)) {
        }
#endif

        spm_write(SPM_DPY_PWR_CON, spm_read(SPM_DPY_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_DPY_PWR_CON, spm_read(SPM_DPY_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_DPY_PWR_CON, spm_read(SPM_DPY_PWR_CON) | PWR_RST_B);

#if 0
        udelay(1); 
        if (!(spm_read(SPM_PWR_STATUS) & DPY_PWR_STA_MASK)) { 
            err = 1;
        }
#endif
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_mdsys1(int state)
{
    int err = 0;
    volatile unsigned int val;
    unsigned long flags;
    int count = 0;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | MD1_PROT_MASK);
        while ((spm_read(TOPAXI_PROT_STA1) & MD1_PROT_MASK) != MD1_PROT_MASK) {
            count++;
            if(count==1000)
                break;
        }

        spm_write(SPM_MD1_PWR_CON, spm_read(SPM_MD1_PWR_CON) | MD_SRAM_PDN);

        spm_write(SPM_MD1_PWR_CON, spm_read(SPM_MD1_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_MD1_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_MD1_PWR_CON, val);

        spm_write(SPM_MD1_PWR_CON, spm_read(SPM_MD1_PWR_CON) & ~(PWR_ON | PWR_ON_S));

#if 0
        udelay(1); 
        if (spm_read(SPM_PWR_STATUS) & MD1_PWR_STA_MASK) { 
            err = 1;
        }
#else
        while ((spm_read(SPM_PWR_STATUS) & MD1_PWR_STA_MASK)
                || (spm_read(SPM_PWR_STATUS_S) & MD1_PWR_STA_MASK)) {
        }
#endif
    } else {    /* STA_POWER_ON */
        spm_write(SPM_MD1_PWR_CON, spm_read(SPM_MD1_PWR_CON) | PWR_ON);
        spm_write(SPM_MD1_PWR_CON, spm_read(SPM_MD1_PWR_CON) | PWR_ON_S);
#if 0
        udelay(1);
#else
        while (!(spm_read(SPM_PWR_STATUS) & MD1_PWR_STA_MASK) 
                || !(spm_read(SPM_PWR_STATUS_S) & MD1_PWR_STA_MASK)) {
        }
#endif

        spm_write(SPM_MD1_PWR_CON, spm_read(SPM_MD1_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_MD1_PWR_CON, spm_read(SPM_MD1_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_MD1_PWR_CON, spm_read(SPM_MD1_PWR_CON) | PWR_RST_B);

        spm_write(SPM_MD1_PWR_CON, spm_read(SPM_MD1_PWR_CON) & ~MD_SRAM_PDN);

#if 0
        udelay(1); 
        if (!(spm_read(SPM_PWR_STATUS) & MD1_PWR_STA_MASK)) { 
            err = 1;
        }
#endif
        spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~MD1_PROT_MASK);
        while (spm_read(TOPAXI_PROT_STA1) & MD1_PROT_MASK) {
        }
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_mdsys2(int state)
{
    int err = 0;
    volatile unsigned int val;
    unsigned long flags;
    int count = 0;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) | MD2_PROT_MASK);
        while ((spm_read(TOPAXI_PROT_STA1) & MD2_PROT_MASK) != MD2_PROT_MASK) {
            count++;
            if(count==1000)
                break;
        }

        spm_write(SPM_MD2_PWR_CON, spm_read(SPM_MD2_PWR_CON) | MD_SRAM_PDN);

        spm_write(SPM_MD2_PWR_CON, spm_read(SPM_MD2_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_MD2_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_MD2_PWR_CON, val);

        spm_write(SPM_MD2_PWR_CON, spm_read(SPM_MD2_PWR_CON) & ~(PWR_ON | PWR_ON_S));

#if 0
        udelay(1); 
        if (spm_read(SPM_PWR_STATUS) & MD2_PWR_STA_MASK) { 
            err = 1;
        }
#else
        while ((spm_read(SPM_PWR_STATUS) & MD2_PWR_STA_MASK) 
                || (spm_read(SPM_PWR_STATUS_S) & MD2_PWR_STA_MASK)) {
        }
#endif
    } else {    /* STA_POWER_ON */
        spm_write(SPM_MD2_PWR_CON, spm_read(SPM_MD2_PWR_CON) | PWR_ON);
        spm_write(SPM_MD2_PWR_CON, spm_read(SPM_MD2_PWR_CON) | PWR_ON_S);
#if 0
        udelay(1);
#else
        while (!(spm_read(SPM_PWR_STATUS) & MD2_PWR_STA_MASK) 
                || !(spm_read(SPM_PWR_STATUS_S) & MD2_PWR_STA_MASK)) {
        }
#endif

        spm_write(SPM_MD2_PWR_CON, spm_read(SPM_MD2_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_MD2_PWR_CON, spm_read(SPM_MD2_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_MD2_PWR_CON, spm_read(SPM_MD2_PWR_CON) | PWR_RST_B);

        spm_write(SPM_MD2_PWR_CON, spm_read(SPM_MD2_PWR_CON) & ~MD_SRAM_PDN);

#if 0
        udelay(1); 
        if (!(spm_read(SPM_PWR_STATUS) & MD2_PWR_STA_MASK)) { 
            err = 1;
        }
#endif
        spm_write(TOPAXI_PROT_EN, spm_read(TOPAXI_PROT_EN) & ~MD2_PROT_MASK);
        while (spm_read(TOPAXI_PROT_STA1) & MD2_PROT_MASK) {
        }
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}


/**
 *test_spm_gpu_power_on - test whether gpu could be powered on 
 *
 *Returns 1 if power on operation succeed, 0 otherwise.
 */
int test_spm_gpu_power_on(void)
{
    int i;
    volatile unsigned int sta1, sta2;
    volatile unsigned int val;
    unsigned long flags;
 
    sta1 = spm_read(SPM_PWR_STATUS);
    sta2 = spm_read(SPM_PWR_STATUS_S);
    if (((sta1 & MFG_PWR_STA_MASK) == MFG_PWR_STA_MASK) && 
            ((sta2 & MFG_PWR_STA_MASK) == MFG_PWR_STA_MASK)) {
        printk("[%s]: test_spm_gpu_power_on already on, return: 1.\n", __func__);
        return 1;
    }

    spm_mtcmos_noncpu_lock(flags);

    val = spm_read(SPM_MFG_PWR_CON);
    BUG_ON(!(val & PWR_ISO));

    for(i = 0; i < 5; i++) {
	
        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ON);
        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) | PWR_ON_S);

        udelay(5);

        sta1 = spm_read(SPM_PWR_STATUS);
        sta2 = spm_read(SPM_PWR_STATUS_S);
        if (((sta1 & MFG_PWR_STA_MASK) != MFG_PWR_STA_MASK) || 
                ((sta2 & MFG_PWR_STA_MASK) != MFG_PWR_STA_MASK)) {
            spm_mtcmos_noncpu_unlock(flags);
            printk("[%s]: test_spm_gpu_power_on return: 0.\n", __func__);
            return 0;
        }

        spm_write(SPM_MFG_PWR_CON, spm_read(SPM_MFG_PWR_CON) & ~(PWR_ON | PWR_ON_S));

        sta1 = spm_read(SPM_PWR_STATUS);
        sta2 = spm_read(SPM_PWR_STATUS_S);
        if (((sta1 & MFG_PWR_STA_MASK) == MFG_PWR_STA_MASK) || 
                ((sta2 & MFG_PWR_STA_MASK) == MFG_PWR_STA_MASK)) {
            spm_mtcmos_noncpu_unlock(flags);
            printk("[%s]: test_spm_gpu_power_on return: 0.\n", __func__);
            return 0;
        }
        mdelay(1);
    }

    spm_mtcmos_noncpu_unlock(flags);

    printk("[%s]: test_spm_gpu_power_on return: 1.\n", __func__);
    return 1;
}

MODULE_DESCRIPTION("MT6589 SPM-MTCMOS Driver v0.1");
