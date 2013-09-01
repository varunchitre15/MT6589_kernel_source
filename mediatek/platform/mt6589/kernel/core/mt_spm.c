#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <mach/irqs.h>
#include <mach/mt_spm.h>

/**************************************
 * for General
 **************************************/
DEFINE_SPINLOCK(spm_lock);

static irqreturn_t spm_irq_handler(int irq, void *dev_id)
{
    spm_error("!!! SPM ISR SHOULD NOT BE EXECUTED !!!\n");

    spin_lock(&spm_lock);
    /* clean ISR status */
    spm_write(SPM_SLEEP_ISR_MASK, 0x0008);
    spm_write(SPM_SLEEP_ISR_STATUS, 0x0018);
    spin_unlock(&spm_lock);

    return IRQ_HANDLED;
}

void spm_module_init(void)
{
    int r;
    unsigned long flags;

    spin_lock_irqsave(&spm_lock, flags);
    /* enable register control */
    spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

    /* init power control register (select PCM clock to qaxi and SRCLKENA_MD=1) */
    spm_write(SPM_POWER_ON_VAL0, 0);
    spm_write(SPM_POWER_ON_VAL1, 0x00215800);
    spm_write(SPM_PCM_PWR_IO_EN, 0);

    /* reset PCM */
    spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_PCM_SW_RESET);
    spm_write(SPM_PCM_CON0, CON0_CFG_KEY);

    /* SRCVOLTEN: POWER_ON_VAL1 (PWR_IO_EN[7]=0) or r7 (PWR_IO_EN[7]=1) */
    /* SRCLKENA_PERI: POWER_ON_VAL1 (PWR_IO_EN[7]=0) or POWER_ON_VAL1|r7 (PWR_IO_EN[7]=1) */
    /* SRCLKENA_MD: POWER_ON_VAL1 (PWR_IO_EN[2]=0) or POWER_ON_VAL1|r7 (PWR_IO_EN[2]=1) */
    /* CLKSQ1: POWER_ON_VAL0 (PWR_IO_EN[0]=0) or r0 (PWR_IO_EN[0]=1) */
    /* CLKSQ2: POWER_ON_VAL0 (PWR_IO_EN[1]=0) or r0 (PWR_IO_EN[1]=1) */
    spm_write(SPM_CLK_CON, CC_SRCVOLTEN_MASK |
                           CC_CXO32K_RM_EN_MD1 | CC_CXO32K_RM_EN_MD2);

    /* clean wakeup event raw status */
    spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, 0xffffffff);

    /* clean ISR status */
    spm_write(SPM_SLEEP_ISR_MASK, 0x0008);
    spm_write(SPM_SLEEP_ISR_STATUS, 0x0018);
    spin_unlock_irqrestore(&spm_lock, flags);

    r = request_irq(MT_SPM_IRQ_ID, spm_irq_handler, IRQF_TRIGGER_LOW,
                    "mt-spm", NULL);
    if (r) {
        spm_error("SPM IRQ register failed (%d)\n", r);
        WARN_ON(1);
    }
}


/**************************************
 * for CPU DVFS
 **************************************/
#define MAX_RETRY_COUNT (100)

int spm_dvfs_ctrl_volt(u32 value)
{
    u32 ap_dvfs_con;
    int retry = 0;

    ap_dvfs_con = spm_read(SPM_AP_DVFS_CON_SET);
    spm_write(SPM_AP_DVFS_CON_SET, (ap_dvfs_con & ~(0x7)) | value);
    udelay(5);

    while ((spm_read(SPM_AP_DVFS_CON_SET) & (0x1 << 31)) == 0)
    {
        if (retry >= MAX_RETRY_COUNT)
        {
            printk("FAIL: no response from PMIC wrapper\n");
            return -1;
        }

        retry++;
        printk("wait for ACK signal from PMIC wrapper, retry = %d\n", retry);

        udelay(5);
    }

    return 0;
}

MODULE_DESCRIPTION("MT6589 SPM Driver v0.1");
