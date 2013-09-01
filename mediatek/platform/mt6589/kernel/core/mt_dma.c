

#include <asm/io.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/workqueue.h>

#include "mach/mt_reg_base.h"
#include "mach/irqs.h"
#include "mach/dma.h"
#include "mach/sync_write.h"
#include "mach/mt_clkmgr.h"
#include "mach/emi_mpu.h"

#define DMA_DEBUG   0
#if(DMA_DEBUG == 1)
#define dbgmsg printk
#else
#define dbgmsg(...)
#endif

/* 
 * DMA information
 */

#define NR_GDMA_CHANNEL     (2)
#define NR_PDMA_CHANNEL     (5)
#define NR_VFFDMA_CHANNEL   (6)
#define GDMA_START          (0)
#define NR_DMA              (NR_GDMA_CHANNEL + NR_PDMA_CHANNEL + NR_VFFDMA_CHANNEL)

/*
 * Register Definition
 */

#define DMA_BASE_CH(n)      (AP_DMA_BASE + 0x0080 * (n + 1))
#define DMA_GLOBAL_INT_FLAG (AP_DMA_BASE + 0x0000) 
#define DMA_GLOBAL_RUNNING_STATUS (AP_DMA_BASE + 0x0008) 
#define DMA_GLOBAL_GSEC_EN (AP_DMA_BASE + 0x0014)
#define DMA_GDMA_SEC_EN(n) (AP_DMA_BASE + 0x0020 + 4 * (n))

/*
 * General DMA channel register mapping:
 */
#define DMA_INT_FLAG(base)      (base + 0x0000)
#define DMA_INT_EN(base)        (base + 0x0004)
#define DMA_START(base)         (base + 0x0008)
#define DMA_RESET(base)         (base + 0x000C)
#define DMA_STOP(base)          (base + 0x0010)
#define DMA_FLUSH(base)         (base + 0x0014)
#define DMA_CON(base)           (base + 0x0018)
#define DMA_SRC(base)           (base + 0x001C)
#define DMA_DST(base)           (base + 0x0020)
#define DMA_LEN1(base)          (base + 0x0024)
#define DMA_LEN2(base)          (base + 0x0028)
#define DMA_JUMP_ADDR(base)     (base + 0x002C)
#define DMA_IBUFF_SIZE(base)    (base + 0x0030)
#define DMA_CONNECT(base)       (base + 0x0034)
#define DMA_AXIATTR(base)       (base + 0x0038)
#define DMA_DBG_STAT(base)      (base + 0x0050)

/*
 * Register Setting
 */

#define DMA_GLBSTA_RUN(ch)      (0x00000001 << ((ch)))
#define DMA_GLBSTA_IT(ch)       (0x00000001 << ((ch)))
#define DMA_GDMA_LEN_MAX_MASK   (0x000FFFFF)

#define DMA_CON_DIR             (0x00000001)
#define DMA_CON_FPEN            (0x00000002)    /* Use fix pattern. */
#define DMA_CON_SLOW_EN         (0x00000004)
#define DMA_CON_DFIX            (0x00000008)
#define DMA_CON_SFIX            (0x00000010)
#define DMA_CON_WPEN            (0x00008000)
#define DMA_CON_WPSD            (0x00100000)
#define DMA_CON_WSIZE_1BYTE     (0x00000000)
#define DMA_CON_WSIZE_2BYTE     (0x01000000)
#define DMA_CON_WSIZE_4BYTE     (0x02000000)
#define DMA_CON_RSIZE_1BYTE     (0x00000000)
#define DMA_CON_RSIZE_2BYTE     (0x10000000)
#define DMA_CON_RSIZE_4BYTE     (0x20000000)
#define DMA_CON_BURST_MASK      (0x00070000)
#define DMA_CON_SLOW_OFFSET     (5)
#define DMA_CON_SLOW_MAX_MASK   (0x000003FF)

#define DMA_START_BIT           (0x00000001)
#define DMA_STOP_BIT            (0x00000000)
#define DMA_INT_FLAG_BIT        (0x00000001)
#define DMA_INT_FLAG_CLR_BIT    (0x00000000)
#define DMA_INT_EN_BIT          (0x00000001)
#define DMA_FLUSH_BIT           (0x00000001)
#define DMA_FLUSH_CLR_BIT       (0x00000000)
#define DMA_UART_RX_INT_EN_BIT  (0x00000003)
#define DMA_INT_EN_CLR_BIT      (0x00000000)
#define DMA_READ_COHER_BIT      (0x00000010)
#define DMA_WRITE_COHER_BIT     (0x00100000)
#define DMA_GSEC_EN_BIT         (0x00000001)
#define DMA_SEC_EN_BIT          (0x00000001)



/*
 * Register Limitation
 */

#define MAX_TRANSFER_LEN1   (0xFFFFF)
#define MAX_TRANSFER_LEN2   (0xFFFFF)
#define MAX_SLOW_DOWN_CNTER (0x3FF)

/*
 * channel information structures
 */

struct dma_ctrl
{
    int in_use;
    DMA_ISR_CALLBACK isr_cb;
    void *data;
};

/*
 * global variables
 */

static struct dma_ctrl dma_ctrl[NR_DMA];
static DEFINE_SPINLOCK(dma_drv_lock);
static DEFINE_SPINLOCK(dma_alloc_lock);
static int g_nr_apdma_allocated;
#define PDN_APDMA_MODULE_NAME ("APDMA")

/*
 * mt65xx_req_gdma: request a general DMA.
 * @cb: DMA ISR callback function
 * @data: DMA ISR argument
 * Return channel number for success; return negative errot code for failure.
 */
int mt65xx_req_gdma(DMA_ISR_CALLBACK cb, void *data)
{
    unsigned long flags;
    int i;

    spin_lock_irqsave(&dma_drv_lock, flags);

    for (i = GDMA_START; i < NR_GDMA_CHANNEL; i++) {
        if (dma_ctrl[i].in_use) {
            continue;
        } else {
            dma_ctrl[i].in_use = 1;
            break;
        }
    }

    spin_unlock_irqrestore(&dma_drv_lock, flags);

    if (i < NR_GDMA_CHANNEL) {
        dma_ctrl[i].isr_cb = cb;
        dma_ctrl[i].data = data;

        spin_lock(&dma_alloc_lock);

        g_nr_apdma_allocated++;

        spin_unlock(&dma_alloc_lock);
        //FIX-ME mark for porting
        //enable_clock(MT65XX_PDN_PERI_APDMA, PDN_APDMA_MODULE_NAME);

        return i;
    } else {
        return -DMA_ERR_NO_FREE_CH;
    }
}

EXPORT_SYMBOL(mt65xx_req_gdma);

/*
 * mt65xx_start_gdma: start the DMA stransfer for the specified GDMA channel
 * @channel: GDMA channel to start
 * Return 0 for success; return negative errot code for failure.
 */
int mt65xx_start_gdma(int channel)
{
    if ((channel < GDMA_START) || (channel >= (GDMA_START + NR_GDMA_CHANNEL))) {
        return -DMA_ERR_INVALID_CH;
    }else if (dma_ctrl[channel].in_use == 0) {
        return -DMA_ERR_CH_FREE;
    }

    writel(DMA_INT_FLAG_CLR_BIT, DMA_INT_FLAG(DMA_BASE_CH(channel)));
    mt65xx_reg_sync_writel(DMA_START_BIT, DMA_START(DMA_BASE_CH(channel)));

    return 0;
}

EXPORT_SYMBOL(mt65xx_start_gdma);

/*
 * mt65xx_stop_gdma: stop the DMA stransfer for the specified GDMA channel
 * @channel: GDMA channel to stop
 * Return 0 for success; return negative errot code for failure.
 */
int mt65xx_stop_gdma(int channel)
{
    if (channel < GDMA_START) {
        return -DMA_ERR_INVALID_CH;
    }

    if (channel >= (GDMA_START + NR_GDMA_CHANNEL)) {
        return -DMA_ERR_INVALID_CH;
    }

    if (dma_ctrl[channel].in_use == 0) {
        return -DMA_ERR_CH_FREE;
    }

    writel(DMA_FLUSH_BIT, DMA_FLUSH(DMA_BASE_CH(channel)));
    while (readl(DMA_START(DMA_BASE_CH(channel))));
    writel(DMA_FLUSH_CLR_BIT, DMA_FLUSH(DMA_BASE_CH(channel)));
    mt65xx_reg_sync_writel(DMA_INT_FLAG_CLR_BIT, DMA_INT_FLAG(DMA_BASE_CH(channel)));

    return 0;
}

EXPORT_SYMBOL(mt65xx_stop_gdma);

/*
 * mt65xx_config_gdma: configure the given GDMA channel.
 * @channel: GDMA channel to configure
 * @config: pointer to the mt65xx_gdma_conf structure in which the GDMA configurations store
 * @flag: ALL, SRC, DST, or SRC_AND_DST.
 * Return 0 for success; return negative errot code for failure.
 */
int mt65xx_config_gdma(int channel, struct mt65xx_gdma_conf *config, DMA_CONF_FLAG flag)
{
    unsigned int dma_con = 0x0, limiter = 0;

    if ((channel < GDMA_START) || (channel >= (GDMA_START + NR_GDMA_CHANNEL))) {
        return -DMA_ERR_INVALID_CH;
    }

    if (dma_ctrl[channel].in_use == 0) {
        return -DMA_ERR_CH_FREE;
    }

    if (!config) {
        return -DMA_ERR_INV_CONFIG;
    }

//    if (!(config->sinc) && ((config->src) % 8)) {
//        printk("GDMA fixed address mode requires 8-bytes aligned address\n");
    if (!config->sinc)
    {
        printk("GMDA fixed adress mode doesn't support\n");
        return -DMA_ERR_INV_CONFIG;
    }

//    if (!(config->dinc) && ((config->dst) % 8)) {
//        printk("GDMA fixed address mode requires 8-bytes aligned address\n");
    if (!config->dinc)
    {
        printk("GMDA fixed adress mode doesn't support\n");
        return -DMA_ERR_INV_CONFIG;
    }

    switch (flag) {
    case ALL:
        writel(config->src, DMA_SRC(DMA_BASE_CH(channel)));
        writel(config->dst, DMA_DST(DMA_BASE_CH(channel)));
        writel((config->wplen) & DMA_GDMA_LEN_MAX_MASK, DMA_LEN2(DMA_BASE_CH(channel)));
        writel(config->wpto, DMA_JUMP_ADDR(DMA_BASE_CH(channel)));
        writel((config->count) & DMA_GDMA_LEN_MAX_MASK, DMA_LEN1(DMA_BASE_CH(channel)));

        /*setup coherence bus*/
        if (config->cohen){
            writel((DMA_READ_COHER_BIT|readl(DMA_AXIATTR(DMA_BASE_CH(channel)))), DMA_AXIATTR(DMA_BASE_CH(channel)));
            writel((DMA_WRITE_COHER_BIT|readl(DMA_AXIATTR(DMA_BASE_CH(channel)))), DMA_AXIATTR(DMA_BASE_CH(channel)));
        }

        /*setup security channel */
        if (config->sec){
            printk("1:GMDA GSEC:%x, ChSEC:%x\n",readl(DMA_GLOBAL_GSEC_EN),readl(DMA_GDMA_SEC_EN(channel)));
            writel((DMA_GSEC_EN_BIT|readl(DMA_GLOBAL_GSEC_EN)), DMA_GLOBAL_GSEC_EN);
            writel((DMA_SEC_EN_BIT|readl(DMA_GDMA_SEC_EN(channel))), DMA_GDMA_SEC_EN(channel));
            printk("2:GMDA GSEC:%x, ChSEC:%x\n",readl(DMA_GLOBAL_GSEC_EN),readl(DMA_GDMA_SEC_EN(channel)));
        }
        else
        {
            printk("1:GMDA GSEC:%x, ChSEC:%x\n",readl(DMA_GLOBAL_GSEC_EN),readl(DMA_GDMA_SEC_EN(channel)));
            writel(((~DMA_GSEC_EN_BIT)&readl(DMA_GLOBAL_GSEC_EN)), DMA_GLOBAL_GSEC_EN);
            printk("2:GMDA GSEC:%x, ChSEC:%x\n",readl(DMA_GLOBAL_GSEC_EN),readl(DMA_GDMA_SEC_EN(channel)));
        }

        if (config->wpen) {
            dma_con |= DMA_CON_WPEN;
        }

        if (config->wpsd) {
            dma_con |= DMA_CON_WPSD;
        }

        if (config->iten) {
            writel(DMA_INT_EN_BIT, DMA_INT_EN(DMA_BASE_CH(channel)));
        }else {
            writel(DMA_INT_EN_CLR_BIT, DMA_INT_EN(DMA_BASE_CH(channel)));
        }

        if (config->dinc && config->sinc) {
            dma_con |= (config->burst & DMA_CON_BURST_MASK);
        }else {
            if (!(config->dinc)) {
                dma_con |= DMA_CON_DFIX;
                dma_con |= DMA_CON_WSIZE_1BYTE;
            }

            if (!(config->sinc)) {
                dma_con |= DMA_CON_SFIX;
                dma_con |= DMA_CON_RSIZE_1BYTE;
            }

            // fixed src/dst mode only supports burst type SINGLE
            dma_con |= DMA_CON_BURST_SINGLE;
        }

        if (config->limiter) {
            limiter = (config->limiter) & DMA_CON_SLOW_MAX_MASK;
            dma_con |= limiter << DMA_CON_SLOW_OFFSET;
            dma_con |= DMA_CON_SLOW_EN;
        }

        writel(dma_con, DMA_CON(DMA_BASE_CH(channel)));
        break;

    case SRC:
        writel(config->src, DMA_SRC(DMA_BASE_CH(channel)));

        break;
        
    case DST:
        writel(config->dst, DMA_DST(DMA_BASE_CH(channel)));
        break;

    case SRC_AND_DST:
        writel(config->src, DMA_SRC(DMA_BASE_CH(channel)));
        writel(config->dst, DMA_DST(DMA_BASE_CH(channel)));
        break;

    default:
        break;
    }

    /* use the data synchronization barrier to ensure that all writes are completed */
    dsb();

    return 0;
}

EXPORT_SYMBOL(mt65xx_config_gdma);

/*
 * mt65xx_free_gdma: free a general DMA.
 * @channel: channel to free
 * Return 0 for success; return negative errot code for failure.
 */
int mt65xx_free_gdma(int channel)
{
    if (channel < GDMA_START) {
        return -DMA_ERR_INVALID_CH;
    }

    if (channel >= (GDMA_START + NR_GDMA_CHANNEL)) {
        return -DMA_ERR_INVALID_CH;
    }

    if (dma_ctrl[channel].in_use == 0) {
        return -DMA_ERR_CH_FREE;
    }

    mt65xx_stop_gdma(channel);

    dma_ctrl[channel].isr_cb = NULL;
    dma_ctrl[channel].data = NULL;
    dma_ctrl[channel].in_use = 0;

    spin_lock(&dma_alloc_lock);

    //if (--g_nr_apdma_allocated == 0) {
    //    disable_clock(MT65XX_PDN_PERI_APDMA, PDN_APDMA_MODULE_NAME);
    //}
    
    spin_unlock(&dma_alloc_lock);

    return 0;
}

EXPORT_SYMBOL(mt65xx_free_gdma);

/*
 * gdma1_irq_handler: general DMA channel 1 interrupt service routine.
 * @irq: DMA IRQ number
 * @dev_id:
 * Return IRQ returned code.
 */
static irqreturn_t gdma1_irq_handler(int irq, void *dev_id)
{
    const unsigned glbsta = readl(DMA_GLOBAL_INT_FLAG);

    dbgmsg(KERN_DEBUG"DMA Module - %s ISR Start\n", __func__);
    dbgmsg(KERN_DEBUG"DMA Module - GLBSTA = 0x%x\n", glbsta);
 
    if (glbsta & DMA_GLBSTA_IT(G_DMA_1)){
        if (dma_ctrl[G_DMA_1].isr_cb) {
            dma_ctrl[G_DMA_1].isr_cb(dma_ctrl[G_DMA_1].data);
        }

        mt65xx_reg_sync_writel(DMA_INT_FLAG_CLR_BIT, DMA_INT_FLAG(DMA_BASE_CH(G_DMA_1)));
#if(DMA_DEBUG == 1)
        glbsta = readl(DMA_GLOBAL_INT_FLAG);
        printk(KERN_DEBUG"DMA Module - GLBSTA after ack = 0x%x\n", glbsta);
#endif
    }

    dbgmsg(KERN_DEBUG"DMA Module - %s ISR END\n", __func__);
    
    return IRQ_HANDLED;
}

/*
 * gdma2_irq_handler: general DMA channel 2 interrupt service routine.
 * @irq: DMA IRQ number
 * @dev_id:
 * Return IRQ returned code.
 */
static irqreturn_t gdma2_irq_handler(int irq, void *dev_id)
{
    const unsigned glbsta = readl(DMA_GLOBAL_INT_FLAG);
    
    dbgmsg(KERN_DEBUG"DMA Module - %s ISR Start\n", __func__);
    dbgmsg(KERN_DEBUG"DMA Module - GLBSTA = 0x%x\n", glbsta);
 
    if (glbsta & DMA_GLBSTA_IT(G_DMA_2)){
        if (dma_ctrl[G_DMA_2].isr_cb) {
            dma_ctrl[G_DMA_2].isr_cb(dma_ctrl[G_DMA_2].data);
        }

        mt65xx_reg_sync_writel(DMA_INT_FLAG_CLR_BIT, DMA_INT_FLAG(DMA_BASE_CH(G_DMA_2)));

#if(DMA_DEBUG == 1)
        glbsta = readl(DMA_GLOBAL_INT_FLAG);
        printk(KERN_DEBUG"DMA Module - GLBSTA after ack = 0x%x\n", glbsta);
#endif
    }

    dbgmsg(KERN_DEBUG"DMA Module - %s ISR END\n", __func__);
    
    return IRQ_HANDLED;
}

void gdma1_check_mpu_violation(u32 addr, int wr_vio)
{
    printk(KERN_CRIT "GDMA1 checks EMI MPU violation.\n");
    printk(KERN_CRIT "addr = 0x%x, %s violation.\n", addr, wr_vio? "Write": "Read");
    printk(KERN_CRIT "DMA SRC = 0x%x.\n", readl(DMA_SRC(DMA_BASE_CH(0))));
    printk(KERN_CRIT "DMA DST = 0x%x.\n", readl(DMA_DST(DMA_BASE_CH(0))));
    printk(KERN_CRIT "DMA COUNT = 0x%x.\n", readl(DMA_LEN1(DMA_BASE_CH(0))));
    printk(KERN_CRIT "DMA CON = 0x%x.\n", readl(DMA_CON(DMA_BASE_CH(0))));
}

/*
 * mt_reset_dma: reset the specified DMA channel
 * @iChannel: channel number of the DMA channel to reset
 */
void mt_reset_dma(const unsigned int iChannel)
{
    struct mt65xx_gdma_conf conf;

    memset(&conf, 0, sizeof(struct mt65xx_gdma_conf));

    if (mt65xx_config_gdma(iChannel, &conf, ALL) != 0){
        return;
    }

    return;
}

/*
 * mt_init_dma: initialize DMA.
 * Always return 0.
 */
static int __init mt_init_dma(void)
{
    int i;

    g_nr_apdma_allocated = 0;

    for (i = 0; i < NR_GDMA_CHANNEL; i++) {
        mt_reset_dma(i);
    }

//    mt6577_irq_set_sens(MT6577_GDMA1_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
//    mt6577_irq_set_polarity(MT6577_GDMA1_IRQ_ID, MT65xx_POLARITY_LOW);

    if (request_irq(MT_GDMA1_IRQ_ID, gdma1_irq_handler, IRQF_TRIGGER_LOW, "GDMA1",  NULL)) {
        printk(KERN_ERR"GDMA1 IRQ LINE NOT AVAILABLE!!\n");
    }

//    enable_irq(MT6577_GDMA1_IRQ_ID);

//    mt6577_irq_set_sens(MT6577_GDMA2_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
//    mt6577_irq_set_polarity(MT6577_GDMA2_IRQ_ID, MT65xx_POLARITY_LOW);

    if (request_irq(MT_GDMA2_IRQ_ID, gdma2_irq_handler, IRQF_TRIGGER_LOW, "GDMA2",  NULL)) {
        printk(KERN_ERR"GDMA2 IRQ LINE NOT AVAILABLE!!\n");
    }
    
    printk("[APDMA] Init APDMA OK\n");
    
    emi_mpu_notifier_register(MST_ID_MMPERI_4, gdma1_check_mpu_violation);    

//    enable_irq(MT6577_GDMA2_IRQ_ID);

    return 0;
}

void mt65xx_dma_running_status(void)
{
  unsigned int dma_running_status;
  int i=0;
  char *DMA_name [15] = {"G_DMA1", "G_DMA2", "HIF_1", "HIF_2", "SIM_1", "SIM_2", "IrDa Tx/Rx",
		"UART_1 Tx", "UART_1 Rx", "UART_2 Tx", "UART_2 Rx", "UART_3 Tx", "UART_3 Rx", "UART_4 Tx", "UART_4 Rx"};
  
  dma_running_status = readl(DMA_GLOBAL_RUNNING_STATUS);
  for(i=0; i<15; i++)
  {
    if(((dma_running_status>>i) & 0x01) == 1)
    {
      printk("DMA %s is running\n", DMA_name[i]);
    }
  }
}

EXPORT_SYMBOL(mt65xx_dma_running_status);

arch_initcall(mt_init_dma);
