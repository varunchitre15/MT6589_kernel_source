/* mediatek/platform/mt6589/kernel/drivers/uart/uart.c
 *
 * (C) Copyright 2008 
 * MediaTek <www.mediatek.com>
 * MingHsien Hsieh <minghsien.hsieh@mediatek.com>
 *
 * MT6589 UART Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/******************************************************************************
 * Dependency
******************************************************************************/
#if defined(CONFIG_MTK_SERIAL_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ   /*used in serial_core.h*/
#endif
/*---------------------------------------------------------------------------*/
#include <linux/autoconf.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/uaccess.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/irq.h>
#include <asm/scatterlist.h>
#include <mach/dma.h>
#include <mach/mt_typedefs.h>
#include <mach/irqs.h>
#include <mach/sync_write.h>
#include <mach/mt_clkmgr.h>
#include <linux/slab.h>
#include "mtk_uart.h"
#include "mach/mt_gpio.h"
#include "linux/delay.h"
#include "mach/mt_idle.h"
#include <linux/syscore_ops.h>
//Fixme 
#define reg_sync_writeb(v, a)			mt65xx_reg_sync_writeb(v, a)
#define reg_sync_writel(v, a)			mt65xx_reg_sync_writel(v, a)

/*---- Macro defination remap -----------------------------------------------*/
/*------ IRQ Section -----------------------*/
#define UART1_IRQ_ID	MT_UART1_IRQ_ID
#define UART2_IRQ_ID	MT_UART2_IRQ_ID
#define UART3_IRQ_ID	MT_UART3_IRQ_ID
#define UART4_IRQ_ID	MT_UART4_IRQ_ID

#define UART1_VFF_TX_IRQ_ID  MT_DMA_UART0_TX_IRQ_ID
#define UART1_VFF_RX_IRQ_ID  MT_DMA_UART0_RX_IRQ_ID
#define UART2_VFF_TX_IRQ_ID  MT_DMA_UART1_TX_IRQ_ID
#define UART2_VFF_RX_IRQ_ID  MT_DMA_UART1_RX_IRQ_ID
#define UART3_VFF_TX_IRQ_ID  MT_DMA_UART2_TX_IRQ_ID
#define UART3_VFF_RX_IRQ_ID  MT_DMA_UART2_RX_IRQ_ID
/*------ PDN Section -----------------------*/
#define PDN_FOR_UART1	MT_CG_PERI0_UART0
#define PDN_FOR_UART2	MT_CG_PERI0_UART1
#define PDN_FOR_UART3	MT_CG_PERI0_UART2
#define PDN_FOR_UART4	MT_CG_PERI0_UART3

#define PDN_FOR_DMA     MT_CG_PERI0_APDMA


/* For ATE factory feature */
#ifdef    ATE_FACTORY_ENABLE
#define ATE_FACTORY_MODE     6
#endif

#ifdef CONFIG_PM
void mtk_uart_save(struct mtk_uart *uart);
#endif

//volatile unsigned int pll_for_uart;
void dump_uart_history(void);
volatile unsigned int	stop_update = 0;
unsigned int			curr_record=-1;
volatile unsigned int	curr_idx;
#define			RECORD_NUMBER	10
#define			RECORD_LENGTH	1032
unsigned char 			uart_history[RECORD_NUMBER][RECORD_LENGTH];
unsigned int		uart_history_cnt[RECORD_NUMBER];
spinlock_t		tx_history_lock,rx_history_lock;

unsigned int			curr_rx_record=-1;
volatile unsigned int	curr_rx_idx;
unsigned char 			uart_rx_history[RECORD_NUMBER][RECORD_LENGTH];
unsigned int		uart_rx_history_cnt[RECORD_NUMBER];

spinlock_t		 mtk_console_lock;
spinlock_t		 mtk_uart_bt_lock;

struct mtk_uart *console_port;
struct mtk_uart *bt_port = NULL;
/*---------------------------------------------------------------------------*/
#define HW_FLOW_CTRL_PORT(uart) (uart->setting->hw_flow)
/*---------------------------------------------------------------------------*/
static struct mtk_uart_setting mtk_uart_default_settings[] = 
{
    {
        //.tx_mode = UART_NON_DMA, .rx_mode = UART_RX_VFIFO_DMA, .dma_mode = UART_DMA_MODE_0,
        .tx_mode = UART_TX_VFIFO_DMA, .rx_mode = UART_RX_VFIFO_DMA, .dma_mode = UART_DMA_MODE_0,
        //.tx_mode = UART_NON_DMA, .rx_mode = UART_NON_DMA, .dma_mode = UART_DMA_MODE_0, 
        .tx_trig = UART_FCR_TXFIFO_1B_TRI, .rx_trig = UART_FCR_RXFIFO_12B_TRI, 

        .uart_base = UART1_BASE, .irq_num = UART1_IRQ_ID, .irq_sen = MT65xx_LEVEL_SENSITIVE,  
        .set_bit = PDN_FOR_UART1,  .clr_bit = PDN_FOR_UART1,  .pll_id = PDN_FOR_UART1,
        .sysrq = FALSE, .hw_flow = TRUE, .vff = TRUE,
    },
    {
        .tx_mode = UART_TX_VFIFO_DMA, .rx_mode = UART_RX_VFIFO_DMA, .dma_mode = UART_DMA_MODE_0,
        .tx_trig = UART_FCR_TXFIFO_1B_TRI, .rx_trig = UART_FCR_RXFIFO_12B_TRI, 

        .uart_base = UART2_BASE, .irq_num = UART2_IRQ_ID, .irq_sen = MT65xx_LEVEL_SENSITIVE,  
        .set_bit = PDN_FOR_UART2, .clr_bit = PDN_FOR_UART2,  .pll_id = PDN_FOR_UART2,
        .sysrq = FALSE, .hw_flow = TRUE, .vff = TRUE,
    },
    {
        .tx_mode = UART_TX_VFIFO_DMA, .rx_mode = UART_RX_VFIFO_DMA, .dma_mode = UART_DMA_MODE_0, 
        .tx_trig = UART_FCR_TXFIFO_1B_TRI, .rx_trig = UART_FCR_RXFIFO_12B_TRI, 
        
        .uart_base = UART3_BASE, .irq_num = UART3_IRQ_ID, .irq_sen = MT65xx_LEVEL_SENSITIVE, 
        .set_bit = PDN_FOR_UART3, .clr_bit = PDN_FOR_UART3,  .pll_id = PDN_FOR_UART3,
        .sysrq = FALSE, .hw_flow = FALSE, .vff = TRUE,        /* UART3 */
    },
    {
        .tx_mode = UART_NON_DMA, .rx_mode = UART_NON_DMA, .dma_mode = UART_DMA_MODE_0, 
        .tx_trig = UART_FCR_TXFIFO_1B_TRI, .rx_trig = UART_FCR_RXFIFO_12B_TRI, 
        
        .uart_base = UART4_BASE, .irq_num = UART4_IRQ_ID, .irq_sen = MT65xx_LEVEL_SENSITIVE, 
        .set_bit = PDN_FOR_UART4, .clr_bit = PDN_FOR_UART4,  .pll_id = PDN_FOR_UART4,
        .sysrq = FALSE, .hw_flow = FALSE, .vff = FALSE,        /* UART4 */
    },
};
/*---------------------------------------------------------------------------*/
static unsigned long mtk_uart_evt_mask[] = {
    DBG_EVT_NONE,
    DBG_EVT_NONE,
    DBG_EVT_NONE,
    DBG_EVT_NONE,
};
/*---------------------------------------------------------------------------*/
#ifdef PM_SUSPEND_RESUME_CONFIG_EN
static unsigned int mtk_uart_suspend_resume_en[] = {
    1,
    1,
    0,  /* UART3 is used for STP, the default setting is disable suspend and resume */
    1,
};
#endif
/*---------------------------------------------------------------------------*/
static unsigned long mtk_uart_lsr_status[] = {
    0,	/* UART1 */
    0,	/* UART2 */
    0,	/* UART3 */
    0,	/* UART4 */
};
/*---------------------------------------------------------------------------*/
#ifdef ENABLE_DEBUG 
/*---------------------------------------------------------------------------*/
static struct mtk_uart_regs mtk_uart_debug[] =
{ 
    INI_REGS(UART1_BASE),
    INI_REGS(UART2_BASE),
    INI_REGS(UART3_BASE),
    INI_REGS(UART4_BASE),
};
/*---------------------------------------------------------------------------*/
#endif /*ENABLE_DEBUG*/
/*---------------------------------------------------------------------------*/
#if defined(ENABLE_VFIFO)
/*---------------------------------------------------------------------------*/
static DEFINE_SPINLOCK(mtk_uart_vfifo_port_lock);
/*---------------------------------------------------------------------------*/
#define VFIFO_INIT_RX(c, i, n, id) \
    {.ch = (c), .size = (n), .trig = VFF_RX_THRE(n), .type = UART_RX_VFIFO, \
     .base = (void*)VFF_BASE_CH(i), .port = NULL, .addr = NULL,             \
     .entry = ATOMIC_INIT(0), .reg_cb = ATOMIC_INIT(0), .iolock=__SPIN_LOCK_UNLOCKED(mtk_uart_vfifo_port[i].lock), \
     .irq_id = id} 
/*---------------------------------------------------------------------------*/
#define VFIFO_INIT_TX(c, i, n, id) \
    {.ch = (c), .size = (n), .trig = VFF_TX_THRE(n), .type = UART_TX_VFIFO, \
     .base = (void*)VFF_BASE_CH(i), .port = NULL,         \
     .addr = NULL, .entry = ATOMIC_INIT(0), .reg_cb = ATOMIC_INIT(0), .iolock=__SPIN_LOCK_UNLOCKED(mtk_uart_vfifo_port[i].lock), \
     .irq_id = id}
/*---------------------------------------------------------------------------*/
static struct mtk_uart_vfifo mtk_uart_vfifo_port[] = {
    VFIFO_INIT_TX(P_DMA_UART1_TX, 0, C_UART1_VFF_TX_SIZE, UART1_VFF_TX_IRQ_ID),
    VFIFO_INIT_RX(P_DMA_UART1_RX, 1, C_UART1_VFF_RX_SIZE, UART1_VFF_RX_IRQ_ID),
    VFIFO_INIT_TX(P_DMA_UART2_TX, 2, C_UART2_VFF_TX_SIZE, UART2_VFF_TX_IRQ_ID),
    VFIFO_INIT_RX(P_DMA_UART2_RX, 3, C_UART2_VFF_RX_SIZE, UART2_VFF_RX_IRQ_ID),
    VFIFO_INIT_TX(P_DMA_UART3_TX, 4, C_UART3_VFF_TX_SIZE, UART3_VFF_TX_IRQ_ID),
    VFIFO_INIT_RX(P_DMA_UART3_RX, 5, C_UART3_VFF_RX_SIZE, UART3_VFF_RX_IRQ_ID),
#if 0 /* MT6589 only 6 DMA channel for UART */
    VFIFO_INIT_TX(P_DMA_UART4_TX, 6, C_UART4_VFF_TX_SIZE),
    VFIFO_INIT_RX(P_DMA_UART4_RX, 7, C_UART4_VFF_RX_SIZE),
#endif
};
/*---------------------------------------------------------------------------*/
#if defined(ENABLE_VFIFO_DEBUG)
/*---------------------------------------------------------------------------*/
#define DGBUF_INIT(v)  \
    do { \
        if (UART_DEBUG_EVT(DBG_EVT_BUF)) { \
            v->dbgidx = (v->dbgidx+1)%(ARRAY_SIZE(v->dbg)); \
            v->cur = &v->dbg[v->dbgidx];                    \
            v->cur->idx = 0;                                \
        } \
    } while(0)
/*---------------------------------------------------------------------------*/
#define DGBUF_PUSH_CH(v,c)    \
    do { \
        if (UART_DEBUG_EVT(DBG_EVT_BUF)) \
            v->cur->dat[v->cur->idx++] = (char)(c); \
    } while(0)
/*---------------------------------------------------------------------------*/
#define DGBUF_PUSH_STR(v,s,l) \
    do { \
        if (UART_DEBUG_EVT(DBG_EVT_BUF)) {\
            memcpy(&v->cur->dat[v->cur->idx], (s), (l)); \
            v->cur->idx += (l);                          \
        } \
    } while(0)
#else
/*---------------------------------------------------------------------------*/
#define DGBUF_INIT(v)        
#define DGBUF_PUSH_CH(v,c)    
#define DGBUF_PUSH_STR(v,s,l) 
#endif
/*---------------------------------------------------------------------------*/
#endif /*ENABLE_VFIFO*/
/*---------------------------------------------------------------------------*/
/* uart control blocks */
static struct mtk_uart mtk_uarts[UART_NR];
/*---------------------------------------------------------------------------*/
static int  mtk_uart_init_ports(void);
static void mtk_uart_start_tx(struct uart_port *port);
static void mtk_uart_stop_tx(struct uart_port *port);
static void mtk_uart_enable_intrs(struct mtk_uart *uart, long mask);
static void mtk_uart_disable_intrs(struct mtk_uart *uart, long mask);
static unsigned int mtk_uart_filter_line_status(struct mtk_uart *uart);
static void mtk_uart_dma_vfifo_tx_tasklet(unsigned long arg);
static void mtk_uart_dma_vfifo_rx_tasklet(unsigned long arg);
extern bool is_meta_mode(void);
inline static void mtk_uart_power_up(struct mtk_uart *uart);
static unsigned int mtk_uart_write_allow(struct mtk_uart *uart);
static void mtk_uart_write_byte(struct mtk_uart *uart, unsigned int byte);
/******************************************************************************
 * SYSFS support
******************************************************************************/
#if defined(ENABLE_SYSFS)
/*---------------------------------------------------------------------------*/
/*define sysfs entry for configuring debug level and sysrq*/
ssize_t mtk_uart_attr_show(struct kobject *kobj, struct attribute *attr, char *buffer);
ssize_t mtk_uart_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t size);
ssize_t mtk_uart_debug_show(struct kobject *kobj, char *page);
ssize_t mtk_uart_debug_store(struct kobject *kobj, const char *page, size_t size);
ssize_t mtk_uart_sysrq_show(struct kobject *kobj, char *page);
ssize_t mtk_uart_sysrq_store(struct kobject *kobj, const char *page, size_t size);
ssize_t mtk_uart_vffsz_show(struct kobject *kobj, char *page);
ssize_t mtk_uart_vffsz_store(struct kobject *kobj, const char *page, size_t size);
ssize_t mtk_uart_conse_show(struct kobject *kobj, char *page);
ssize_t mtk_uart_conse_store(struct kobject *kobj, const char *page, size_t size);
ssize_t mtk_uart_vff_en_show(struct kobject *kobj, char *page);
ssize_t mtk_uart_vff_en_store(struct kobject *kobj, const char *page, size_t size);
ssize_t mtk_uart_lsr_status_show(struct kobject *kobj, char *page);
ssize_t mtk_uart_lsr_status_store(struct kobject *kobj, const char *page, size_t size);
ssize_t mtk_uart_no_power_off_show(struct kobject *kobj, char *page);
ssize_t mtk_uart_no_power_off_store(struct kobject *kobj, const char *page, size_t size);
/*---------------------------------------------------------------------------*/
struct sysfs_ops mtk_uart_sysfs_ops = {
    .show   = mtk_uart_attr_show,
    .store  = mtk_uart_attr_store,
};
/*---------------------------------------------------------------------------*/
struct mtuart_entry {
    struct attribute attr;
    ssize_t (*show)(struct kobject *kobj, char *page);
    ssize_t (*store)(struct kobject *kobj, const char *page, size_t size);
};
/*---------------------------------------------------------------------------*/
struct mtuart_entry debug_entry = {
    { .name = "debug", .mode = S_IRUGO | S_IWUSR },
    mtk_uart_debug_show,
    mtk_uart_debug_store,
};
/*---------------------------------------------------------------------------*/
struct mtuart_entry sysrq_entry = {
    { .name = "sysrq", .mode = S_IRUGO | S_IWUSR },
    mtk_uart_sysrq_show,
    mtk_uart_sysrq_store,
};
/*---------------------------------------------------------------------------*/
struct mtuart_entry vffsz_entry = {
    { .name = "vffsz", .mode = S_IRUGO | S_IWUSR },
    mtk_uart_vffsz_show,
    mtk_uart_vffsz_store,
};
/*---------------------------------------------------------------------------*/
struct mtuart_entry conse_entry = {
    { .name = "conse", .mode = S_IRUGO | S_IWUSR },
    mtk_uart_conse_show,
    mtk_uart_conse_store,
};
/*---------------------------------------------------------------------------*/
struct mtuart_entry vff_en_entry = {
    { .name = "vff_en", .mode = S_IRUGO | S_IWUSR },
    mtk_uart_vff_en_show,
    mtk_uart_vff_en_store,
};
/*---------------------------------------------------------------------------*/
struct mtuart_entry lsr_status_entry = {
    { .name = "lsr_status", .mode = S_IRUGO | S_IWUSR },
    mtk_uart_lsr_status_show,
    mtk_uart_lsr_status_store,
};
/*---------------------------------------------------------------------------*/
struct mtuart_entry no_power_off_entry = {
    { .name = "no_power_off_en", .mode = S_IRUGO | S_IWUSR },
    mtk_uart_no_power_off_show,
    mtk_uart_no_power_off_store,
};
/*---------------------------------------------------------------------------*/
struct attribute *mtk_uart_attributes[] = {
    &conse_entry.attr,  /*console setting*/
#if defined(ENABLE_DEBUG)        
    &debug_entry.attr,
    &sysrq_entry.attr,
    &vffsz_entry.attr,
    &vff_en_entry.attr,
    &lsr_status_entry.attr,
    &no_power_off_entry.attr,
#endif     
    NULL,
};
/*---------------------------------------------------------------------------*/
struct kobj_type mtk_uart_ktype = {
    .sysfs_ops = &mtk_uart_sysfs_ops,
    .default_attrs = mtk_uart_attributes,
};
/*---------------------------------------------------------------------------*/
struct mtuart_sysobj {
    struct kobject kobj;
    atomic_t sysrq;
    atomic_t vffLen[UART_NR*UART_VFIFO_NUM];
    atomic_t console_enable;
} mtk_uart_sysobj = {
    .console_enable = ATOMIC_INIT(1),
};
/*---------------------------------------------------------------------------*/
int mtk_uart_sysfs(void) 
{
    struct mtuart_sysobj *obj = &mtk_uart_sysobj;
    int idx;

    memset(&obj->kobj, 0x00, sizeof(obj->kobj));
#if defined(CONFIG_MAGIC_SYSRQ)
    atomic_set(&obj->sysrq, 1);
#else
    atomic_set(&obj->sysrq, 0);    
#endif
#if defined(ENABLE_VFIFO)
    for (idx = 0; idx < ARRAY_SIZE(obj->vffLen); idx++)
        atomic_set(&obj->vffLen[idx], mtk_uart_vfifo_port[idx].size);
#endif
    atomic_set(&obj->console_enable, 1);
    
    obj->kobj.parent = kernel_kobj;
    if (kobject_init_and_add(&obj->kobj, &mtk_uart_ktype, NULL, "mtuart")) {
        kobject_put(&obj->kobj);
        return -ENOMEM;
    }
    kobject_uevent(&obj->kobj, KOBJ_ADD);
    return 0;
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_attr_show(struct kobject *kobj, struct attribute *attr, char *buffer) 
{
    struct mtuart_entry *entry = container_of(attr, struct mtuart_entry, attr);
    return entry->show(kobj, buffer);
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t size) 
{
    struct mtuart_entry *entry = container_of(attr, struct mtuart_entry, attr);
    return entry->store(kobj, buffer, size);
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_debug_show(struct kobject *kobj, char *buffer) 
{
    int remain = PAGE_SIZE;
    int len;
    char *ptr = buffer;
    int idx;

    for (idx = 0; idx < UART_NR; idx++) {
        len = scnprintf(ptr, remain, "0x%2x\n", (unsigned int)mtk_uart_evt_mask[idx]);
        ptr += len;
        remain -= len;
    }
    dump_uart_history();
    return (PAGE_SIZE-remain);
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_debug_store(struct kobject *kobj, const char *buffer, size_t size) 
{
    int a, b, c, d;
    int res = sscanf(buffer, "0x%x 0x%x 0x%x 0x%x", &a, &b, &c, &d);

    if (res != 4) {
        MSG_ERR("%s: expect 4 numbers\n", __FUNCTION__);
    } else {
        mtk_uart_evt_mask[0] = a;
        mtk_uart_evt_mask[1] = b;
        mtk_uart_evt_mask[2] = c;
        mtk_uart_evt_mask[3] = d;
    }
    return size;
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_sysrq_show(struct kobject *kobj, char *buffer) 
{
    struct mtuart_sysobj *obj = container_of(kobj, struct mtuart_sysobj, kobj);    
    return scnprintf(buffer, PAGE_SIZE, "%d\n", atomic_read(&obj->sysrq));
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_sysrq_store(struct kobject *kobj, const char *buffer, size_t size) 
{
    struct mtuart_sysobj *obj = container_of(kobj, struct mtuart_sysobj, kobj);    
    int a;
    int res = sscanf(buffer, "%d\n", &a);

    if (res != 1) {
        MSG_ERR("%s: expect 1 number\n", __FUNCTION__);
    } else {
        atomic_set(&obj->sysrq, a);    
    }
    return size;
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_vffsz_show(struct kobject *kobj, char *buffer) 
{
    ssize_t len = 0;
#if defined(ENABLE_VFIFO)
    struct mtuart_sysobj *obj = container_of(kobj, struct mtuart_sysobj, kobj);    
    int idx;

    for (idx = 0; idx < ARRAY_SIZE(obj->vffLen); idx++)
        len += scnprintf(buffer+len, PAGE_SIZE-len, "[%02d] %4d\n", idx, 
               atomic_read(&obj->vffLen[idx]));
#endif
    return len;
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_vffsz_store(struct kobject *kobj, const char *buffer, size_t size) 
{
#if defined(ENABLE_VFIFO)
    struct mtuart_sysobj *obj = container_of(kobj, struct mtuart_sysobj, kobj);    
    int idx, sz;

    if (2 != sscanf(buffer, "%d %d", &idx, &sz)) {
        MSG_ERR("%s: expect 2 variables\n", __FUNCTION__);    
    } else if (idx >= ARRAY_SIZE(obj->vffLen) || (sz%8 != 0)) {
        MSG_ERR("%s: invalid args %d, %d\n", __FUNCTION__, idx, sz);        
    } else {
        atomic_set(&obj->vffLen[idx], sz);    
    }
#endif
    return size;
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_conse_show(struct kobject *kobj, char *buffer) 
{
    struct mtuart_sysobj *obj = container_of(kobj, struct mtuart_sysobj, kobj);    
    return scnprintf(buffer, PAGE_SIZE, "%d\n", atomic_read(&obj->console_enable));
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_conse_store(struct kobject *kobj, const char *buffer, size_t size) 
{
    struct mtuart_sysobj *obj = container_of(kobj, struct mtuart_sysobj, kobj);    
    int enable;

    if (1 != sscanf(buffer, "%d", &enable)) {
        MSG_ERR("%s: expect 1 variables\n", __FUNCTION__);    
    } else {
        atomic_set(&obj->console_enable, enable);    
    }
    return size;
}
/*---------------------------------------------------------------------------*/
#endif /*ENABLE_SYSFS*/
/*---------------------------------------------------------------------------*/
#ifdef UART_FCR_USING_SW_BACK_UP
inline static void __write_fcr_register(struct mtk_uart *uart, u32 data)
{
    u32 base = uart->base;
    uart->fcr_back_up = data&(~(3<<1));
    reg_sync_writel(data, UART_FCR);
}

inline static void sync_write_fcr_register(struct mtk_uart *uart, u32 data)
{
    u32 base = uart->base;
    uart->fcr_back_up = data&(~(3<<1));
    reg_sync_writel(data, UART_FCR);
}

inline static u32 __read_fcr_register(struct mtk_uart *uart)
{
    return uart->fcr_back_up;
}

inline static void __set_fcr_register(struct mtk_uart *uart, u32 mask)
{
    u32 base = uart->base;
    u32 new_setting = (uart->fcr_back_up)|mask;
    uart->fcr_back_up = new_setting&(~(3<<1));
    reg_sync_writel(new_setting, UART_FCR);
}

inline static void __clr_fcr_register(struct mtk_uart *uart, u32 mask)
{
    u32 base = uart->base;
    u32 new_setting = (uart->fcr_back_up)&(~mask);
    uart->fcr_back_up = new_setting&(~(3<<1));
    reg_sync_writel(new_setting, UART_FCR);
}
#else
inline static void __write_fcr_register(struct mtk_uart *uart, u32 data)
{
    u32 base = uart->base;
    reg_sync_writel(data, UART_FCR);
}

inline static void sync_write_fcr_register(struct mtk_uart *uart, u32 data)
{
    u32 base = uart->base;
    reg_sync_writel(data, UART_FCR);
}

inline static u32 __read_fcr_register(struct mtk_uart *uart)
{
    u32 base = uart->base;
    return UART_READ32(UART_FCR_RD);
}

inline static void __set_fcr_register(struct mtk_uart *uart, u32 mask)
{
    u32 base = uart->base;
    u32 new_setting = UART_READ32(UART_FCR_RD)|mask;
    reg_sync_writel(new_setting, UART_FCR);
}

inline static void __clr_fcr_register(struct mtk_uart *uart, u32 mask)
{
    u32 base = uart->base;
    u32 new_setting = UART_READ32(UART_FCR_RD)&(~mask);
    reg_sync_writel(new_setting, UART_FCR);
}
#endif /* End of UART_FCR_USING_SW_BACK_UP */ 
ssize_t mtk_uart_vff_en_show(struct kobject *kobj, char *buffer) 
{
    int remain = PAGE_SIZE;
    int len;
    char *ptr = buffer;
    int idx;

    for (idx = 0; idx < UART_NR; idx++) {
        len = scnprintf(ptr, 
        		remain, 
        		"tx%d_m:%2x rx%d_m:%2x\n",
			idx, 
        		(unsigned int)mtk_uart_default_settings[idx].tx_mode,
			idx,
        		(unsigned int)mtk_uart_default_settings[idx].rx_mode);
        ptr += len;
        remain -= len;
    }
    return (PAGE_SIZE-remain);
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_vff_en_store(struct kobject *kobj, const char *buffer, size_t size) 
{
    int u1_tx, u1_rx, u2_tx, u2_rx, u3_tx, u3_rx, u4_tx, u4_rx;
    int res = sscanf(buffer, "%x %x %x %x %x %x %x %x", 
    			&u1_tx, &u1_rx, &u2_tx, &u2_rx, &u3_tx, &u3_rx, &u4_tx, &u4_rx);

    if (res != 8) {
        MSG_ERR("%s: expect 8 numbers\n", __FUNCTION__);
    } else {
        mtk_uart_default_settings[0].tx_mode = u1_tx;
        mtk_uart_default_settings[0].rx_mode = u1_rx;
        mtk_uart_default_settings[1].tx_mode = u2_tx;
        mtk_uart_default_settings[1].rx_mode = u2_rx;
        mtk_uart_default_settings[2].tx_mode = u3_tx;
        mtk_uart_default_settings[2].rx_mode = u3_rx;
        mtk_uart_default_settings[3].tx_mode = u4_tx;
        mtk_uart_default_settings[3].rx_mode = u4_rx;
    }
    return size;
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_lsr_status_show(struct kobject *kobj, char *buffer) 
{
    int remain = PAGE_SIZE;
    int len;
    char *ptr = buffer;
    int idx;

    for (idx = 0; idx < UART_NR; idx++) {
        len = scnprintf(ptr, 
        		remain, 
        		"%04x ",
        		(unsigned int)mtk_uart_lsr_status[idx]);
        ptr += len;
        remain -= len;
    }
    mtk_uart_lsr_status[0] = 0;
    mtk_uart_lsr_status[1] = 0;
    mtk_uart_lsr_status[2] = 0;
    mtk_uart_lsr_status[3] = 0;
    len = scnprintf(ptr, remain, "\n");
    ptr += len;
    remain -= len;
    return (PAGE_SIZE-remain);
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_lsr_status_store(struct kobject *kobj, const char *buffer, size_t size) 
{
    int u1_lsr, u2_lsr, u3_lsr, u4_lsr;
    int res = sscanf(buffer, "%x %x %x %x", 
    			&u1_lsr, &u2_lsr, &u3_lsr, &u4_lsr);

    if (res != 4) {
        MSG_ERR("%s: expect 4 numbers\n", __FUNCTION__);
    } else {
        mtk_uart_lsr_status[0] = u1_lsr;
        mtk_uart_lsr_status[1] = u2_lsr;
        mtk_uart_lsr_status[2] = u3_lsr;
        mtk_uart_lsr_status[3] = u4_lsr;
    }
    return size;
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_no_power_off_show(struct kobject *kobj, char *buffer) 
{
    int remain = PAGE_SIZE;
    int len;
    char *ptr = buffer;
    int idx;

    for (idx = 0; idx < UART_NR; idx++) {
        len = scnprintf(ptr, 
        		remain, 
        		"%02x ",
        		(unsigned int)mtk_uart_suspend_resume_en[idx]);
        ptr += len;
        remain -= len;
    }
    len = scnprintf(ptr, remain, "\n");
    ptr += len;
    remain -= len;
    return (PAGE_SIZE-remain);
}
/*---------------------------------------------------------------------------*/
ssize_t mtk_uart_no_power_off_store(struct kobject *kobj, const char *buffer, size_t size) 
{
    int uart_port, no_power_off_en;
    int res = sscanf(buffer, "P:%x E:%x", 
    			&uart_port, &no_power_off_en);

    if (res != 2) {
        MSG_ERR("%s: expect 2 numbers\n", __FUNCTION__);
    } else {
	if((uart_port>0)&&(uart_port<=UART_NR)){
            mtk_uart_suspend_resume_en[uart_port-1] = no_power_off_en;
        }
    }
    return size;
}
//================================ FIQ ==========================================
#if (defined(CONFIG_FIQ_DEBUGGER_CONSOLE) && defined(CONFIG_FIQ_DEBUGGER))
#define DEFAULT_FIQ_UART_PORT           (3)
int fiq_console_port = DEFAULT_FIQ_UART_PORT;
//struct uart_port *p_mtk_uart_port = &(mtk_uarts[DEFAULT_FIQ_UART_PORT].port);
//EXPORT_SYMBOL(p_mtk_uart_port);
struct mtk_uart *mt_console_uart = &(mtk_uarts[DEFAULT_FIQ_UART_PORT]);
#endif
//==============================================================================
/*---------------------------------------------------------------------------*/
/* UART Log port switch feature */
static int find_string(char str[], const char* fingerprint, int *offset)
{
        char *curr=str;
        int i = 0;
        int str_len;
        int fingerprint_len;
        if( (NULL == str)||(NULL == fingerprint) )
                return 0;
        str_len = strlen(str);
        fingerprint_len = strlen(fingerprint);
        if(str_len < fingerprint_len)
                return 0;
        for(i=0; i<=(str_len-fingerprint_len); i++){
                if( strncmp(curr, fingerprint, fingerprint_len) == 0 ){
                        if(NULL != offset)
                                *offset = i;
                        return 1;
                }
                curr++;
        }
        return 0;
}

static int find_fingerprint(char str[], int *offset)
{
        /**
         * This function limitation:
         * If the ttyMT number large than 9, this function will work abnormal.
         * For example, ttyMT12 will be recoginzed as ttyMT1
         */
        const char* fingerprint[] =
        {
                "console=/dev/null",
                "console=ttyMT0",
                "console=ttyMT1",
                "console=ttyMT2",
                "console=ttyMT3",
        };
        int i;
        for(i=0; i<sizeof(fingerprint)/sizeof(char*); i++){
                if(find_string(str, fingerprint[i], offset) != 0)
                        return i; // Find it.
        }
        return -1; // Not find
}

static int modify_fingerprint(char str[], int offset, char new_val)
{
        if(NULL == str)
                return 0;
        str[offset+14-1] = new_val; // 14 = strlen("console=ttyMTx"), we modify x to 1~3
        return 1;
}

void adjust_kernel_cmd_line_setting_for_console(char *u_boot_cmd_line, char *kernel_cmd_line)
{
        int offset = 0;
        int kernel_console_port_setting = -1;
        int u_boot_console_port_setting = -1;

        /* Check u-boot command line setting */
        u_boot_console_port_setting = find_fingerprint(u_boot_cmd_line, 0);
        if(-1 == u_boot_console_port_setting){
                //printf("U-boot does not have console setting, return\n");
                return;
        }

        /* U-boot has console setting, check kernel console setting */
        kernel_console_port_setting = find_fingerprint(kernel_cmd_line, &offset);
        if(-1 == kernel_console_port_setting){
                //printf("Kernel does not have console setting, return\n");
                goto _Exit;
        }

        /**
         * Both U-boot and Kernel has console setting.
         * 1. If the settings are same, return directly;
         * 2. If kernel console setting is null, then use kernel setting
         * 3. If u-boot console setting is null, then use kernel setting
         * 4. If kernel console setting is not null, then use u-boot setting
         */
        if(u_boot_console_port_setting == kernel_console_port_setting){
                //printf("Same console setting, return\n");
                goto _Exit;
        }
        if(0 == kernel_console_port_setting){
                //printf("Kernel console setting is null, use kernel setting, return\n");
                goto _Exit;
        }
        if(0 == u_boot_console_port_setting){
                //printf("U-boot console setting is null, use kernel setting, return\n");
                goto _Exit;
        }
        /* Enter here, it means both kernel and u-boot console setting are not null, using u-boot setting */
        switch(u_boot_console_port_setting){
        case 1: // Using ttyMT0
                modify_fingerprint(kernel_cmd_line, offset, '0');
                break;
        case 2: // Using ttyMT1
                //modify_fingerprint(kernel_cmd_line, offset, '1');
                break;
        case 3: // Using ttyMT2
                //modify_fingerprint(kernel_cmd_line, offset, '2');
                break;
        case 4: // Using ttyMT3
                modify_fingerprint(kernel_cmd_line, offset, '3');
                break;
        default:
                /* Do nothing */
                break;
        }

_Exit:
#if (defined(CONFIG_FIQ_DEBUGGER_CONSOLE) && defined(CONFIG_FIQ_DEBUGGER))
        kernel_console_port_setting = find_fingerprint(kernel_cmd_line, &offset);
        if(-1 == kernel_console_port_setting){
                //printf("Kernel does not have console setting, return\n");
                return;
        }

        if(kernel_console_port_setting > 0) {
                fiq_console_port = (kernel_console_port_setting-1);
                mt_console_uart = &(mtk_uarts[fiq_console_port]);
        }
#endif
        return;
}
//==============================================================================
/*-------------------------------- PDN ---------------------------------------*/
unsigned int mtk_uart_pdn_enable(char *port, int enable)
{
    int str_len;
    int port_num;
	
    if (port == NULL)
	return -1;
    
    str_len = strlen(port);
    if (str_len != 6) {
	MSG_ERR( "Length mismatch! len=%d\n", str_len);
	return -1;
    }

    if(find_string(port, "ttyMT", 0) == 0) {
	MSG_ERR( "Format mismatch! str=%s\n", port);
	return -1;
    }

    port_num = port[str_len-1]-'0';
    if (port_num >= UART_NR) {
        MSG_ERR( "wrong port:%d\n", port_num);
	return -1;
    }
    bt_port = &mtk_uarts[port_num];
    if (enable){
	switch(port_num){
	    case 0:
		enable_dpidle_by_bit(PDN_FOR_UART1);
		enable_mcidle_by_bit(PDN_FOR_UART1);
	        break;
	    case 1:
		enable_dpidle_by_bit(PDN_FOR_UART2);
		enable_mcidle_by_bit(PDN_FOR_UART2);
	        break;
	    case 2:
		enable_dpidle_by_bit(PDN_FOR_UART3);
		enable_mcidle_by_bit(PDN_FOR_UART3);
	        break;
	    default:
		break;
	}
    }else{
	switch(port_num){
	    case 0:
		disable_dpidle_by_bit(PDN_FOR_UART1);
		disable_mcidle_by_bit(PDN_FOR_UART1);
	        break;
	    case 1:
		disable_dpidle_by_bit(PDN_FOR_UART2);
		disable_mcidle_by_bit(PDN_FOR_UART2);
	        break;
	    case 2:
		disable_dpidle_by_bit(PDN_FOR_UART3);
		disable_mcidle_by_bit(PDN_FOR_UART3);
	        break;
	    default:
		break;
	}
    }
    return 0;
}

 void dump_reg(struct mtk_uart *uart, const char* caller)
{
#ifdef ENABLE_DEBUG
	unsigned long flags;
    u32 base = uart->base;
    u32 lcr = UART_READ32(UART_LCR);
    u32 uratefix = UART_READ32(UART_RATE_FIX_AD);
    u32 uhspeed  = UART_READ32(UART_HIGHSPEED);
    u32 usamplecnt = UART_READ32(UART_SAMPLE_COUNT);
    u32 usamplepnt = UART_READ32(UART_SAMPLE_POINT);
    u32 udll, udlh;
    u32 ier = UART_READ32(UART_IER);
    spin_lock_irqsave(&mtk_console_lock, flags);
    reg_sync_writel((lcr | UART_LCR_DLAB), UART_LCR);
    udll = UART_READ32(UART_DLL);
    udlh = UART_READ32(UART_DLH);
    mb();                               /* make sure the DLL/DLH have been read */
    reg_sync_writel(lcr, UART_LCR);     /* DLAB end */
    spin_unlock_irqrestore(&mtk_console_lock, flags);
    dsb();
    
    MSG(CFG, "%s: RATEFIX(%02X); HSPEED(%02X); CNT(%02X); PNT(%02X); DLH(%02X), DLL(%02X), IER(%02X)\n", 
            caller, uratefix, uhspeed, usamplecnt, usamplepnt, udlh, udll, ier);
#endif             
}

void dump_uart_reg(void)
{
    struct mtk_uart *uart;
    unsigned int i;
    u32 base, lsr, escape_en;
    for (i = 0; i < UART_NR; i++) {
        uart = &mtk_uarts[i];  
   	base = uart->base;
	if (uart->poweron_count > 0)      
	{
	    lsr = UART_READ32(UART_LSR);
	    escape_en = UART_READ32(UART_ESCAPE_EN);
	    printk("[UART%d] LSR=0x%x   ESCAPE_EN=0x%x\n", uart->nport, lsr, escape_en);
  	}else
	    printk("[UART%d] clock is off\n", uart->nport);	
    }
    
}
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_MTK_SERIAL_CONSOLE
/*---------------------------------------------------------------------------*/
static void mtk_uart_console_write(struct console *co, const char *s,
    unsigned int count)
{
    /* Notice:
     * (1) The function is called by printk, hence, spin lock can not be used
     * (2) don't care vfifo setting
     */
    #define CONSOLE_RETRY (5000)
    int i;
    struct mtk_uart *uart;
    u32 cnt = 0;
	unsigned long flags;


    if (co->index >= UART_NR || !(co->flags & CON_ENABLED) || !atomic_read(&mtk_uart_sysobj.console_enable))
        return;

    uart = &mtk_uarts[co->index];    
    for (i = 0; i < (int)count; i++) {
        cnt = 0;
        while (!mtk_uart_write_allow(uart)) {
            barrier();
            if (cnt++ >= CONSOLE_RETRY) {
                uart->timeout_count++;
                return;
            }
        }
	spin_lock_irqsave(&mtk_console_lock, flags);
        mtk_uart_write_byte(uart, s[i]);
	spin_unlock_irqrestore(&mtk_console_lock, flags);

        if (s[i] == '\n') {
            cnt = 0;
            while (!mtk_uart_write_allow(uart)) {
                barrier();
                if (cnt++ >= CONSOLE_RETRY) {
                    uart->timeout_count++;
                    return;
                }
            }
	    	spin_lock_irqsave(&mtk_console_lock, flags);
            mtk_uart_write_byte(uart, '\r');
	    	spin_unlock_irqrestore(&mtk_console_lock, flags);
        }
    }
}
/*---------------------------------------------------------------------------*/
static int __init mtk_uart_console_setup(struct console *co, char *options)
{
    struct mtk_uart *uart;
    struct uart_port *port;
    int baud    = 115200;
    int bits    = 8;
    int parity  = 'n';
    int flow    = 'n';
    int ret;

    printk(KERN_ALERT DBG_TAG "mt6589 console setup : co->index %d options:%s\n",
        co->index, options);

    if (co->index >= UART_NR)
        co->index = 0;

    uart = &mtk_uarts[co->index];
    port = (struct uart_port *)uart;

    console_port = uart;

    // Additional config if Log switch to UART1, disable VFIFO
    if(uart->nport == 0){ // UART1 as log port
        mtk_uart_default_settings[0].tx_mode = UART_NON_DMA;
        mtk_uart_default_settings[0].rx_mode = UART_NON_DMA;
        uart->tx_mode = UART_NON_DMA;
        uart->rx_mode = UART_NON_DMA;
	enable_dpidle_by_bit(PDN_FOR_UART1);
	enable_mcidle_by_bit(PDN_FOR_UART1);
    }

    if (options)
        uart_parse_options(options, &baud, &parity, &bits, &flow);

    ret = uart_set_options(port, co, baud, parity, bits, flow);
    printk(KERN_ALERT DBG_TAG "mt6589 console setup : uart_set_option port(%d) "
          "baud(%d) parity(%c) bits(%d) flow(%c) - ret(%d)\n",
           co->index, baud, parity, bits, flow, ret);
    
    printk(KERN_ALERT DBG_TAG "mt6589 setting: (%d, %d, %d, %lu, %lu)\n", 
           uart->tx_mode, uart->rx_mode,
           uart->dma_mode,uart->tx_trig,
           uart->rx_trig);
    //mtk_uart_power_up(uart);
    return ret;
}
/*---------------------------------------------------------------------------*/
static struct uart_driver mtk_uart_drv;
static struct console mtk_uart_console =
{
    .name       = "ttyMT",
#if !defined(CONFIG_MTK_SERIAL_MODEM_TEST)    
    /*don't configure UART4 as console*/
    .write      = mtk_uart_console_write,
    .setup      = mtk_uart_console_setup,
#endif     
    .device     = uart_console_device,
    .flags      = CON_PRINTBUFFER,
    .index      = -1,
    .data       = &mtk_uart_drv,
};
/*---------------------------------------------------------------------------*/
static int __init mtk_uart_console_init(void)
{
    int err = mtk_uart_init_ports();
    if (!err)
        register_console(&mtk_uart_console);
    return err;
}
/*---------------------------------------------------------------------------*/
console_initcall(mtk_uart_console_init);
/*---------------------------------------------------------------------------*/
static int __init mtk_late_console_init(void)
{
    if (!(mtk_uart_console.flags & CON_ENABLED))
    {
        register_console(&mtk_uart_console);
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
late_initcall(mtk_late_console_init);
/*---------------------------------------------------------------------------*/
#endif /* CONFIG_MTK_SERIAL_CONSOLE */
/******************************************************************************
 * Virtual FIFO implementation
******************************************************************************/
#if defined(ENABLE_VFIFO) 
/*---------------------------------------------------------------------------*/
static int mtk_uart_vfifo_del_dbgbuf(struct mtk_uart_vfifo *vfifo)
{
#if defined(ENABLE_VFIFO_DEBUG)    
    int idx;
    for (idx = 0; idx < ARRAY_SIZE(vfifo->dbg); idx++) {
        if (vfifo->dbg[idx].dat != 0)
            kfree(vfifo->dbg[idx].dat);
        vfifo->dbg[idx].dat = NULL;
        vfifo->dbg[idx].idx = 0;
        vfifo->dbg[idx].len = 0;
    }
#endif     
    return 0;
}
/*---------------------------------------------------------------------------*/
static int mtk_uart_vfifo_new_dbgbuf(struct mtk_uart_vfifo *vfifo)
{
#if defined(ENABLE_VFIFO_DEBUG)        
    int idx;
    for (idx = 0; idx < ARRAY_SIZE(vfifo->dbg); idx++) {
        if (vfifo->dbg[idx].dat != 0)
            kfree(vfifo->dbg[idx].dat);
        vfifo->dbg[idx].idx = 0;
        vfifo->dbg[idx].len = vfifo->size;
        vfifo->dbg[idx].dat = kzalloc(vfifo->dbg[idx].len, GFP_ATOMIC);
        if (!vfifo->dbg[idx].dat) {
            mtk_uart_vfifo_del_dbgbuf(vfifo);
            return -ENOMEM;
        }                   
    }
#endif
    return 0;
}
/*---------------------------------------------------------------------------*/
static int mtk_uart_vfifo_create(struct mtk_uart *uart)
{   /*NOTE: please save the phyiscal address in vff->dmahd*/
    struct mtk_uart_vfifo *vfifo;
    int idx, err = 0;

    MSG_FUNC_ENTRY();

    if(!mtk_uart_default_settings[uart->nport].vff){
        MSG_RAW("[UART%2d] not support VFF, Cancel alloc\n", uart->nport);
        return err;
    }

    MSG_RAW("[UART%2d] create", uart->nport);

    for (idx = uart->nport*2; idx < uart->nport*2 + 2; idx++) {
        vfifo = &mtk_uart_vfifo_port[idx];
        if (vfifo->size)
            vfifo->addr = dma_alloc_coherent(NULL, vfifo->size, &vfifo->dmahd, GFP_DMA);
        else
            vfifo->addr = NULL;
        if (vfifo->size && !vfifo->addr) {
            err = -ENOMEM;
            break;
        } else if ((err = mtk_uart_vfifo_new_dbgbuf(vfifo))) {
            break;
        }
        MSG_RAW("[%2d] %p (%04d) ;", idx, vfifo->addr, vfifo->size);        
    }
    MSG_RAW("\n");
    return err;
}
/*---------------------------------------------------------------------------*/
static int mtk_uart_vfifo_delete(struct mtk_uart *uart)
{
    struct mtk_uart_vfifo *vfifo;
    int idx;

    MSG_FUNC_ENTRY();

    if(!mtk_uart_default_settings[uart->nport].vff){
        MSG_RAW("[UART%2d] not support VFF, Cancel free\n", uart->nport);
        return 0;
    }
    
    MSG_RAW("[UART%2d] delete", uart->nport);
    for (idx = uart->nport*2; idx < uart->nport*2 + 2; idx++) {
        vfifo = &mtk_uart_vfifo_port[idx];
        if (vfifo->addr)
            dma_free_coherent(NULL, vfifo->size, vfifo->addr, vfifo->dmahd);
        mtk_uart_vfifo_del_dbgbuf(vfifo);
        MSG_RAW("[%2d] %p (%04d) ;", idx, vfifo->addr, vfifo->size);        
        vfifo->addr = NULL;
    }
    MSG_RAW("\n");
    return 0;
}
/*---------------------------------------------------------------------------*/
int mtk_uart_vfifo_prepare(struct mtk_uart *uart)
{
    struct mtuart_sysobj *obj = &mtk_uart_sysobj;
    struct mtk_uart_vfifo *tport, *rport;
    int tx = uart->nport << 1;
    int rx = tx + 1;
    
    MSG_FUNC_ENTRY();

    if (uart->nport >= UART_NR) {
        MSG_ERR( "wrong port:%d\n", uart->nport);
        return -EINVAL;
    } else if(FALSE ==uart->setting->vff){
        MSG_ERR( "Port :%d not support vfifo\n", uart->nport);
        return -EINVAL;
    }else {
        tport = &mtk_uart_vfifo_port[tx];
        rport = &mtk_uart_vfifo_port[rx];
    }
    if ((atomic_read(&obj->vffLen[tx]) == tport->size) &&
        (atomic_read(&obj->vffLen[rx]) == rport->size))
        return 0;
    MSG_RAW("re-alloc +\n");
    mtk_uart_vfifo_delete(uart);
    tport->size = atomic_read(&obj->vffLen[tx]);
    tport->trig = VFF_TX_THRE(tport->size);
    rport->size = atomic_read(&obj->vffLen[rx]);
    rport->trig = VFF_RX_THRE(rport->size);
    mtk_uart_vfifo_create(uart);
    MSG_RAW("re-alloc -\n");
    return 0;
}
/*---------------------------------------------------------------------------*/
static struct mtk_uart_vfifo *mtk_uart_vfifo_alloc(struct mtk_uart *uart, UART_VFF_TYPE type)
{
    struct mtk_uart_vfifo *vfifo = NULL;
    unsigned long flags;
    spin_lock_irqsave(&mtk_uart_vfifo_port_lock, flags);
    
    MSG(INFO, "(%d, %d)", uart->nport, type);        

    if ((uart->nport >= UART_NR) || (type >= UART_VFIFO_NUM))
        vfifo = NULL;
    else {
        vfifo = &mtk_uart_vfifo_port[2*uart->nport+type];
    }

    if (vfifo && vfifo->addr == NULL)
        vfifo = NULL;
    MSG(INFO, "alloc vfifo-%d[%d](%p)\n", uart->nport, vfifo->size, vfifo->addr);        

    spin_unlock_irqrestore(&mtk_uart_vfifo_port_lock, flags);
    return vfifo;

}
/*---------------------------------------------------------------------------*/
static void mtk_uart_vfifo_free(struct mtk_uart *uart,
                                   struct mtk_uart_vfifo *vfifo)
{
    unsigned long flags;
    if (vfifo) {
        spin_lock_irqsave(&mtk_uart_vfifo_port_lock, flags);
        vfifo->dma = NULL;
        vfifo->cur = NULL;
        vfifo->dbgidx = 0;
        spin_unlock_irqrestore(&mtk_uart_vfifo_port_lock, flags);
    }
}
/*---------------------------------------------------------------------------*/
inline static int mtk_uart_vfifo_enable(struct mtk_uart *uart, struct mtk_uart_vfifo *vfifo)
{
    u32 base = uart->base;

    if (!vfifo) {
        MSG(ERR, "null\n");
        return -EINVAL;
    } else if (vfifo->type != UART_RX_VFIFO && vfifo->type != UART_TX_VFIFO) {
        MSG(ERR, "unknown type: %d\n", vfifo->type);
        return -EINVAL;
    } else {
    	/* 
    	 * NOTE: For FCR is a read only register reason, 
    	 *       special read/write/set/clr function need to use
    	 */
        /*UART_SET_BITS(UART_FCR_FIFO_INIT, UART_FCR);
        UART_CLR_BITS(UART_FCR_DMA1, UART_FCR);*/
        __set_fcr_register(uart, UART_FCR_FIFO_INIT);
        __clr_fcr_register(uart, UART_FCR_DMA1);

        if (vfifo->type == UART_RX_VFIFO)
            UART_SET_BITS(UART_RX_DMA_EN|UART_TO_CNT_AUTORST, UART_DMA_EN);
        else if (vfifo->type == UART_TX_VFIFO)
            UART_SET_BITS(UART_TX_DMA_EN, UART_DMA_EN);
        dsb();
    }    
    return 0;
}
/*---------------------------------------------------------------------------*/
inline static int mtk_uart_vfifo_disable(struct mtk_uart *uart, struct mtk_uart_vfifo *vfifo)
{
    u32 base = uart->base;
    
    if (!vfifo) {
        MSG(ERR, "null\n");
        return -EINVAL;
    } else if (vfifo->type != UART_RX_VFIFO && vfifo->type != UART_TX_VFIFO) {
        MSG(ERR, "unknown type: %d\n", vfifo->type);
        return -EINVAL;
    } else if (vfifo->type == UART_RX_VFIFO) {
        UART_CLR_BITS(UART_RX_DMA_EN|UART_TO_CNT_AUTORST, UART_DMA_EN);
    } else if (vfifo->type == UART_TX_VFIFO) {
        UART_CLR_BITS(UART_TX_DMA_EN, UART_DMA_EN);
    }
    dsb();
    return 0;
}
/*---------------------------------------------------------------------------*/
inline static void mtk_uart_vfifo_enable_tx_intr(struct mtk_uart *uart)
{
    reg_sync_writel(VFF_TX_INT_EN_B, VFF_INT_EN(uart->tx_vfifo->base));
}
/*---------------------------------------------------------------------------*/
inline static void mtk_uart_vfifo_diable_tx_intr(struct mtk_uart *uart)
{
    reg_sync_writel(0x00, VFF_INT_EN(uart->tx_vfifo->base));    
}
/*---------------------------------------------------------------------------*/
inline static void mtk_uart_vfifo_enable_rx_intr(struct mtk_uart *uart)
{
    reg_sync_writel(VFF_RX_INT_EN0_B|VFF_RX_INT_EN1_B, VFF_INT_EN(uart->rx_vfifo->base));
}
/*---------------------------------------------------------------------------*/
inline static void mtk_uart_vfifo_diable_rx_intr(struct mtk_uart *uart)
{
    reg_sync_writel(0x00, VFF_INT_EN(uart->rx_vfifo->base));    
}
/*---------------------------------------------------------------------------*/
inline static int mtk_uart_vfifo_is_full(struct mtk_uart_vfifo *vfifo)
{
    return (UART_READ32(VFF_LEFT_SIZE(vfifo->base)) <= 16) ? (1) : (0);
}
/*---------------------------------------------------------------------------*/
inline static int mtk_uart_vfifo_is_empty(struct mtk_uart_vfifo *vfifo)
{
    return (UART_READ32(VFF_VALID_SIZE(vfifo->base)) == 0) ? (1) : (0);
}
/*---------------------------------------------------------------------------*/
static unsigned int mtk_uart_vfifo_write_allow(struct mtk_uart *uart)
{
    return !mtk_uart_vfifo_is_full(uart->tx_vfifo);
}
/*---------------------------------------------------------------------------*/
static unsigned int mtk_uart_vfifo_read_allow(struct mtk_uart *uart)
{
    return !mtk_uart_vfifo_is_empty(uart->rx_vfifo);
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_vfifo_write_byte(struct mtk_uart *uart, 
                                                unsigned int byte)
{
    void *addr, *base = uart->tx_vfifo->base;
    unsigned int wpt = UART_READ32(VFF_WPT(base));
    unsigned long flags;

    addr = (void*)((wpt&0xffff)+uart->tx_vfifo->addr);
    reg_sync_writeb((unsigned char)byte, addr);
    mb();                                              //make sure write point updated after VFIFO written.
    if(uart->nport==2){
	spin_lock_irqsave(&tx_history_lock, flags);
	if(!stop_update){
		if(curr_idx<RECORD_LENGTH){
			uart_history[curr_record][curr_idx] = UART_READ8(addr);
			curr_idx++;
			uart_history_cnt[curr_record] = curr_idx;
		}
	}
	spin_unlock_irqrestore(&tx_history_lock, flags);
    }
    if((wpt&0xffff) == (UART_READ32(VFF_LEN(base))-1))
        reg_sync_writel((~wpt)&0x10000, VFF_WPT(base));
    else 
        reg_sync_writel( wpt+1, VFF_WPT(base));
}
/*---------------------------------------------------------------------------*/
static unsigned int mtk_uart_vfifo_read_byte(struct mtk_uart *uart)
{
    void *addr, *base = uart->rx_vfifo->base;
    unsigned int ch;

    addr = (void*)(UART_READ16(VFF_RPT(base))+uart->rx_vfifo->addr);
    ch = UART_READ8(addr);
    dsb();                                              //make sure read point updated after VFIFO read.
	if(UART_READ16(VFF_RPT(base)) == (UART_READ32(VFF_LEN(base))-1))
		reg_sync_writel(~(UART_READ32(VFF_RPT(base)))&0x10000, VFF_RPT(base));
	else 
		reg_sync_writel( UART_READ32(VFF_RPT(base))+1, VFF_RPT(base));    
    
    return ch;
}
/*---------------------------------------------------------------------------*/
inline static unsigned short mtk_uart_vfifo_get_trig(
                                        struct mtk_uart *uart, 
                                        struct mtk_uart_vfifo *vfifo)
{
    return vfifo->trig;
}
/*---------------------------------------------------------------------------*/
inline static int mtk_uart_vfifo_get_counts(struct mtk_uart_vfifo *vfifo)
{
    return UART_READ32(VFF_VALID_SIZE(vfifo->base));
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_tx_vfifo_flush(struct mtk_uart* uart, int timeout)
{
    struct mtk_uart_dma *dma = &uart->dma_tx;
    struct mtk_uart_vfifo *vfifo = dma->vfifo;
    void *base = vfifo->base;

#ifdef ENABE_HRTIMER_FLUSH
    if (dma && uart) {
        if (UART_READ32(VFF_FLUSH(base)) == 0) {            
            reg_sync_writel(VFF_FLUSH_B, VFF_FLUSH(base));
            if (!timeout)
                hrtimer_try_to_cancel(&vfifo->flush);
            MSG(MSC, "flush [%5X.%5X]\n", UART_READ32(VFF_RPT(base)), UART_READ32(VFF_WPT(base)));
        } else {
            /*the ns used to transfer the data in TX VFIFO*/
            u32 size = UART_READ32(VFF_VALID_SIZE(base));
            s64 t = size*10*(NSEC_PER_SEC/uart->baudrate);
            ktime_t cur = ktime_get();
            ktime_t nxt = ktime_add_ns(cur, t);
        	hrtimer_try_to_cancel(&vfifo->flush);
        	hrtimer_start(&vfifo->flush, nxt, HRTIMER_MODE_ABS);            
        #if defined(ENABLE_VFIFO_DEBUG)    
        {
            struct timespec a = ktime_to_timespec(cur);
            struct timespec b = ktime_to_timespec(nxt);
            MSG(MSC, "start: [%ld %ld] [%ld %ld] [%d %lld]\n", 
                a.tv_sec, a.tv_nsec, b.tv_sec, b.tv_nsec, size, t);
        }
        #endif    
        }
    } else {
        MSG(ERR, "%p, %p\n", dma, uart);        
        //del_timer(&dma->vfifo->timer);
    }
#else
    if(dma && uart){
        if(UART_READ32(VFF_FLUSH(base)) == 0){            
            reg_sync_writel(VFF_FLUSH_B, VFF_FLUSH(base));
            MSG(MSC, "flush [%5X.%5X]\n", UART_READ32(VFF_RPT(base)), UART_READ32(VFF_WPT(base)));
        }
    }else{
        MSG(ERR, "%p, %p\n", dma, uart);
    }
#endif //ENABE_HRTIMER_FLUSH
}
/*---------------------------------------------------------------------------*/
#define get_mtk_uart(ptr, type, member) (type *)( (char *)ptr - offsetof(type,member))
/*---------------------------------------------------------------------------*/
static enum hrtimer_restart mtk_uart_tx_vfifo_timeout(struct hrtimer *hrt)
{
    struct mtk_uart_vfifo *vfifo = container_of(hrt, struct mtk_uart_vfifo, flush);
    struct mtk_uart_dma *dma = (struct mtk_uart_dma*)vfifo->dma;
    struct mtk_uart *uart = dma->uart;
#if defined(ENABLE_VFIFO_DEBUG)    
    ktime_t cur = ktime_get();
    struct timespec a = ktime_to_timespec(cur);

    MSG(MSC, "flush timeout [%ld %ld]\n", a.tv_sec, a.tv_nsec);
#endif    
    mtk_uart_tx_vfifo_flush(uart, 1);
    return HRTIMER_NORESTART;
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_dma_vfifo_callback(void *data)
{
    struct mtk_uart_dma *dma = (struct mtk_uart_dma*)data;
    struct mtk_uart *uart = dma->uart;
        
    MSG(DMA, "%s VFIFO CB: %4d/%4d [%02X] [%5X..%5X]\n", dma->dir == DMA_TO_DEVICE ? "TX" : "RX",
        mtk_uart_vfifo_get_counts(dma->vfifo), dma->vfifo->size, 
        UART_READ32(VFF_INT_FLAG(dma->vfifo->base)), UART_READ32(VFF_RPT(dma->vfifo->base)), UART_READ32(VFF_WPT(dma->vfifo->base)));

    if (dma->dir == DMA_FROM_DEVICE) {
        /*the data must be read before return from callback, otherwise, the interrupt
          will be triggered again and again*/
        mtk_uart_dma_vfifo_rx_tasklet((unsigned long)uart);
        //return; [ALPS00031975]
    } else if (dma->dir == DMA_TO_DEVICE) {
    }
    tasklet_schedule(&dma->tasklet);
}
/*---------------------------------------------------------------------------*/
void mtk_uart_dma_vfifo_tx_tasklet_byte(unsigned long arg)
{
    struct mtk_uart *uart = (struct mtk_uart *)arg;
    struct uart_port   *port = &uart->port;
    //struct mtk_uart_dma *dma = &uart->dma_tx;    
    struct mtk_uart_vfifo *vfifo = uart->tx_vfifo;
    struct circ_buf    *xmit = &port->state->xmit;
    unsigned int len, count, size, left, chk = 0;
    ktime_t begin, end;
    struct timespec a, b;

    size = CIRC_CNT_TO_END(xmit->head, xmit->tail, UART_XMIT_SIZE);
    left = vfifo->size - mtk_uart_vfifo_get_counts(vfifo);
    left = (left > 16) ? (left-16) : (0);   /*prevent from CPU lock*/
    len  = count = left < size ? left : size;    

    if (!len) {
        chk = 1;
        MSG(DMA,">>>>> zero size <<<<< \n");
    }

    DGBUF_INIT(vfifo);
    begin = ktime_get();
    a = ktime_to_timespec(begin);
    while (len--) {
	/*DMA limitation.
	Workaround: Polling flush bit to zero, set 1s timeout*/
	while (UART_READ32(VFF_FLUSH(vfifo->base))){
	    end = ktime_get();
	    b = ktime_to_timespec(end);
	    if ((b.tv_sec - a.tv_sec) > 1 || ((b.tv_sec - a.tv_sec) == 1 && b.tv_nsec > a.tv_nsec)){
		printk("[UART%d] Polling flush timeout\n",port->line);
		return;
	    }
	}
        DGBUF_PUSH_CH(vfifo, (char)xmit->buf[xmit->tail]);
        uart->write_byte(uart, xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;
    }
#if defined(ENABLE_VFIFO_DEBUG)
    if (UART_DEBUG_EVT(DBG_EVT_DMA) && UART_DEBUG_EVT(DBG_EVT_BUF)) {
        char str[4] = {0};    
        if (count >= 4) {
            str[0] = vfifo->cur->dat[0];
            str[1] = vfifo->cur->dat[1];
            str[2] = vfifo->cur->dat[vfifo->cur->idx-2];
            str[3] = vfifo->cur->dat[vfifo->cur->idx-1];
        } else {
            int idx;
            for (idx = 0; idx < count; idx++)
                str[idx] = vfifo->cur->dat[idx];
            for (; idx < 4; idx++)
                str[idx] = 0;
        }
        MSG(DMA, "TX[%4d]: %4d/%4d [%05X-%05X] (%02X %02X .. %02X %02X) \n", 
            size, count, left, UART_READ32(VFF_WPT(vfifo->base)), UART_READ32(VFF_RPT(vfifo->base)),
            str[0], str[1], str[2], str[3]);        
    } else {
        MSG(DMA, "TX[%4d]: %4d/%4d [%05X-%05X] \n", 
            size, count, left, UART_READ32(VFF_WPT(vfifo->base)), UART_READ32(VFF_RPT(vfifo->base)));     
    }
#endif
    
#if defined(ENABLE_VFIFO_DEBUG)
    if (UART_DEBUG_EVT(DBG_EVT_DAT) && UART_DEBUG_EVT(DBG_EVT_BUF)) {
        int i;
        printk("[UART%d_TX] %4d bytes:", uart->nport, vfifo->cur->idx);
        for (i = 0; i < vfifo->cur->idx; i++) {
            if (i % 16 == 0)
                printk("\n");
            printk("%.2x ", (unsigned char)vfifo->cur->dat[i]);
        }
        printk("\n");
    }
#endif

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);

}
/*---------------------------------------------------------------------------*/
int mtk_uart_vfifo_write_string(struct mtk_uart *uart, const unsigned char *chars, size_t size)
{
    void *addr, *base = uart->tx_vfifo->base;
    unsigned int wpt = UART_READ32(VFF_WPT(base));
    unsigned int num_to_end;

    addr = (void*)((wpt&0xffff)+uart->tx_vfifo->addr);
    num_to_end = UART_READ32(VFF_LEN(base)) - (wpt&0xffff);
    if(num_to_end >= size){ /* Copy one time is enough */
        memcpy(addr, chars, size);
        mb();                                                  //make sure write point updated after VFIFO written.
        reg_sync_writel( wpt+(unsigned int)size, VFF_WPT(base));
    }else{ /* Need copy two times */
        memcpy(addr, chars, num_to_end);
        memcpy(uart->tx_vfifo->addr, &chars[num_to_end], (unsigned int)size - num_to_end);
        mb();                                                  //make sure write point updated after VFIFO written.
        wpt = ((~wpt)&0x10000)+ (unsigned int)size - num_to_end;
        reg_sync_writel(wpt, VFF_WPT(base));
    }

    return size;
}
/*---------------------------------------------------------------------------*/
void mtk_uart_dma_vfifo_tx_tasklet_str(unsigned long arg)
{
    struct mtk_uart *uart = (struct mtk_uart *)arg;
    struct uart_port   *port = &uart->port;
    //struct mtk_uart_dma *dma = &uart->dma_tx;    
    struct mtk_uart_vfifo *vfifo = uart->tx_vfifo;
    struct circ_buf    *xmit = &port->state->xmit;
    unsigned int len, count, size, left, chk = 0;

    size = CIRC_CNT_TO_END(xmit->head, xmit->tail, UART_XMIT_SIZE);
    left = vfifo->size - mtk_uart_vfifo_get_counts(vfifo);
    left = (left > 16) ? (left-16) : (0);   /*prevent from CPU lock*/
    len  = count = left < size ? left : size;    

    if (!len) {
        chk = 1;
        MSG(DMA,">>>>> zero size <<<<< \n");
    }

    DGBUF_INIT(vfifo);

    mtk_uart_vfifo_write_string(uart, &xmit->buf[xmit->tail], size);
    DGBUF_PUSH_STR(vfifo, &xmit->buf[xmit->tail], size);
    xmit->tail = (xmit->tail+size) & (UART_XMIT_SIZE - 1);
    port->icount.tx += size;
    
#if defined(ENABLE_VFIFO_DEBUG)
    if (UART_DEBUG_EVT(DBG_EVT_DMA) && UART_DEBUG_EVT(DBG_EVT_BUF)) {
        char str[4] = {0};    
        if (count >= 4) {
            str[0] = vfifo->cur->dat[0];
            str[1] = vfifo->cur->dat[1];
            str[2] = vfifo->cur->dat[vfifo->cur->idx-2];
            str[3] = vfifo->cur->dat[vfifo->cur->idx-1];
        } else {
            int idx;
            for (idx = 0; idx < count; idx++)
                str[idx] = vfifo->cur->dat[idx];
            for (; idx < 4; idx++)
                str[idx] = 0;
        }
        MSG(DMA, "TX[%4d]: %4d/%4d [%05X-%05X] (%02X %02X .. %02X %02X) \n", 
            size, count, left, UART_READ32(VFF_WPT(vfifo->base)), UART_READ32(VFF_RPT(vfifo->base)),
            str[0], str[1], str[2], str[3]);
    } else {
        MSG(DMA, "TX[%4d]: %4d/%4d [%05X-%05X] \n", 
            size, count, left, UART_READ32(VFF_WPT(vfifo->base)), UART_READ32(VFF_RPT(vfifo->base)));
    }
#endif    
 
#if defined(ENABLE_VFIFO_DEBUG)
    if (UART_DEBUG_EVT(DBG_EVT_DAT) && UART_DEBUG_EVT(DBG_EVT_BUF)) {
        int i;
        printk("[UART%d_TX] %4d bytes:", uart->nport, vfifo->cur->idx);
        for (i = 0; i < vfifo->cur->idx; i++) {
            if (i % 16 == 0)
                printk("\n");
            printk("%.2x ", (unsigned char)vfifo->cur->dat[i]);
        }
        printk("\n");
    }
#endif

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);

}
/*---------------------------------------------------------------------------*/
static void mtk_uart_dma_vfifo_tx_tasklet(unsigned long arg) 
{
    struct mtk_uart *uart = (struct mtk_uart *)arg;
    struct uart_port   *port = &uart->port;
    struct mtk_uart_dma *dma = &uart->dma_tx;    
    struct mtk_uart_vfifo *vfifo = uart->tx_vfifo;
    struct circ_buf    *xmit = &port->state->xmit;
    int txcount = port->icount.tx;
    void *base = vfifo->base;
    unsigned long flags;

    spin_lock_irqsave(&vfifo->iolock, flags);
    if (atomic_inc_and_test(&vfifo->entry) > 1) {
        MSG(ERR, "tx entry!!\n");    
        tasklet_schedule(&vfifo->dma->tasklet);
    } else {
        while (UART_READ32(VFF_LEFT_SIZE(base)) >= vfifo->trig) {
            /* deal with x_char first */
            if (unlikely(port->x_char)) {
                MSG(INFO, "detect x_char!!\n");
                uart->write_byte(uart, port->x_char);
                port->icount.tx++;
                port->x_char = 0;
                break;
            }        
            if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
                uart->pending_tx_reqs = 0;
                atomic_set(&dma->free, 1);
                complete(&dma->done);    
                break;    
            }
            mtk_uart_dma_vfifo_tx_tasklet_byte(arg);
        }
        if (txcount != port->icount.tx) {
            mtk_uart_vfifo_enable_tx_intr(uart);
            mtk_uart_tx_vfifo_flush(uart, 0);        
        }
    }
    atomic_dec(&vfifo->entry);
    spin_unlock_irqrestore(&vfifo->iolock, flags);
}
/*---------------------------------------------------------------------------*/
void mtk_uart_dma_vfifo_rx_tasklet_byte(unsigned long arg)
{
    struct mtk_uart *uart = (struct mtk_uart*)arg;
    struct uart_port   *port = &uart->port;
    struct mtk_uart_vfifo *vfifo = uart->rx_vfifo;
    struct tty_struct *tty = uart->port.state->port.tty;
    int count, left;
    unsigned int ch, flag, status;
    unsigned long flags;

    MSG_FUNC_ENTRY();

    count = left = mtk_uart_vfifo_get_counts(vfifo);
    
    spin_lock_irqsave(&port->lock, flags);
    
    DGBUF_INIT(vfifo);
    while (!mtk_uart_vfifo_is_empty(vfifo) && count > 0) {

        /* check status */
        status = uart->read_status(uart);
        status = mtk_uart_filter_line_status(uart);
        
        ch = uart->read_byte(uart);
        flag = TTY_NORMAL;
        /* error handling routine */
        if (status & UART_LSR_BI) {
            MSG(INFO, "Break Interrupt!!!\n");
            port->icount.brk++;
            if (uart_handle_break(port))
                continue;
            flag = TTY_BREAK;
        } else if (status & UART_LSR_PE) {
            MSG(INFO, "Parity Error!!!\n");
            port->icount.parity++;
            flag = TTY_PARITY;
        } else if (status & UART_LSR_FE) {
            MSG(INFO, "Frame Error!!!\n");
            port->icount.frame++;
            flag = TTY_FRAME;
        } else if (status & UART_LSR_OE) {
            MSG(INFO, "Overrun!!!\n");
            port->icount.overrun++;
            flag = TTY_OVERRUN;        
        }
        port->icount.rx++;
        count--;
        DGBUF_PUSH_CH(vfifo, ch);
        if (!tty_insert_flip_char(tty, ch, flag))
            MSG(ERR, "tty_insert_flip_char: no space\n");
    }
    tty_flip_buffer_push(tty);
    
#if defined(ENABLE_VFIFO_DEBUG)
    if (UART_DEBUG_EVT(DBG_EVT_DMA) && UART_DEBUG_EVT(DBG_EVT_BUF)) {
        char str[4] = {0};    
        if (count >= 4) {
            str[0] = vfifo->cur->dat[0];
            str[1] = vfifo->cur->dat[1];
            str[2] = vfifo->cur->dat[vfifo->cur->idx-2];
            str[3] = vfifo->cur->dat[vfifo->cur->idx-1];
        } else {
            int idx;
            for (idx = 0; idx < count; idx++)
                str[idx] = vfifo->cur->dat[idx];
            for (; idx < 4; idx++)
                str[idx] = 0;
        }
        MSG(DMA, "RX[%4d]: %4d bytes from VFIFO [%4d] (%02X %02X .. %02X %02X) %d\n", 
            left, left - count, mtk_uart_vfifo_get_counts(vfifo), str[0], str[1], str[2], str[3], 
            UART_READ32(VFF_VALID_SIZE(vfifo->base)));
    } else {
        MSG(DMA, "RX[%4d]: %4d bytes from VFIFO [%4d] %d\n", 
            left, left - count, mtk_uart_vfifo_get_counts(vfifo), 
            UART_READ32(VFF_VALID_SIZE(vfifo->base)));
    }
#endif    

    spin_unlock_irqrestore(&port->lock, flags);

#if defined(ENABLE_VFIFO_DEBUG)
    if (UART_DEBUG_EVT(DBG_EVT_DAT) && UART_DEBUG_EVT(DBG_EVT_BUF)) {
        int i;
        printk("[UART%d_RX] %4d bytes:", uart->nport, vfifo->cur->idx);
        
        for (i = 0; i < vfifo->cur->idx; i++) {        
            if (i % 16 == 0)
                printk("\n");
            printk("%.2x ", (unsigned char)vfifo->cur->dat[i]);
        }
        printk("\n");
        
    }
#endif

}
/*---------------------------------------------------------------------------*/
/* A duplicate of tty_insert_flip_string.                                    */
/* The only difference is the function will accept one extra variable for    */
/* indicating the current line status.                                       */
/*---------------------------------------------------------------------------*/
int mtk_uart_tty_insert_flip_string(struct mtk_uart* uart, const unsigned char *chars,
				size_t size)
{
    struct tty_struct *tty = uart->port.state->port.tty;
    struct uart_port   *port = &uart->port;
    u32 status, flag;
    int copied = 0;
    int i;
    unsigned long flags;

    status = uart->read_status(uart);
    status = mtk_uart_filter_line_status(uart);
        
    flag = TTY_NORMAL;
    /* error handling routine */
    if (status & UART_LSR_BI) {
        MSG(ERR, "Break Interrupt!!\n");
        port->icount.brk++;
        if (uart_handle_break(port))
            return 0;
        flag = TTY_BREAK;
    } else if (status & UART_LSR_PE) {
        MSG(ERR, "Parity Error!!!\n");
        port->icount.parity++;
        flag = TTY_PARITY;
    } else if (status & UART_LSR_FE) {
        MSG(ERR, "Frame Error!!!\n");
        port->icount.frame++;
        flag = TTY_FRAME;
    } else if (status & UART_LSR_OE) {
        MSG(ERR, "Overrun!!!\n");
        port->icount.overrun++;
        flag = TTY_OVERRUN;        
    }        
    
    if(uart->nport==2){
		spin_lock_irqsave(&rx_history_lock, flags);
		if(!stop_update){
			for(i=0; (curr_rx_idx<RECORD_LENGTH)&&(i<size); i++,curr_rx_idx++){
				uart_rx_history[curr_rx_record][curr_rx_idx] = chars[i];
			}
			uart_rx_history_cnt[curr_rx_record] = curr_rx_idx;
		}
		spin_unlock_irqrestore(&rx_history_lock, flags);
    }

    if (flag == TTY_NORMAL) {
        copied = tty_insert_flip_string(tty, chars, size);
    } else {
        MSG(ERR, "error occurs\n");
        copied += tty_insert_flip_string(tty, chars, size-1);
        copied += tty_insert_flip_char(tty, chars[size-1], flag);
    }
    port->icount.rx += copied;
    return copied;    
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_dma_vfifo_rx_tasklet_str(unsigned long arg)
{
    struct mtk_uart *uart = (struct mtk_uart*)arg;
    struct uart_port   *port = &uart->port;
    struct mtk_uart_vfifo *vfifo = uart->rx_vfifo;
    struct tty_struct *tty = uart->port.state->port.tty;
    int count, left;
    unsigned int rxptr, txptr, txreg, rxreg;
    unsigned long flags; 
    unsigned char *ptr;
    void *base = vfifo->base;
    unsigned long history_flags; 

    MSG_FUNC_ENTRY();

    spin_lock_irqsave(&port->lock, flags);

    rxreg = UART_READ32(VFF_RPT(base));
    txreg = UART_READ32(VFF_WPT(base));
    rxptr = rxreg & 0x0000FFFF;
    txptr = txreg & 0x0000FFFF; 
    count = left = ((rxreg ^ txreg) & 0x00010000) ? (txptr+vfifo->size-rxptr) : (txptr-rxptr);
        
    DGBUF_INIT(vfifo);

    if(uart->nport==2){
		spin_lock_irqsave(&rx_history_lock, history_flags);
		if(!stop_update){
			curr_rx_record++;
			curr_rx_idx = 0;
			if(curr_rx_record >= RECORD_NUMBER)
				curr_rx_record = 0;
			uart_rx_history_cnt[curr_rx_record] = 0;
			for(curr_rx_idx=0; curr_rx_idx<RECORD_LENGTH; curr_rx_idx++)
				uart_rx_history[curr_rx_record][curr_rx_idx] = 0;
			curr_rx_idx = 0;
		}
		spin_unlock_irqrestore(&rx_history_lock, history_flags);
	}
    if ((rxptr+count) <= txptr) {
        ptr = (unsigned char*)(rxptr+vfifo->addr);
        mtk_uart_tty_insert_flip_string(uart, ptr, count);
        DGBUF_PUSH_STR(vfifo, ptr, count);
    } else {
        ptr = (unsigned char*)(rxptr+vfifo->addr);
        mtk_uart_tty_insert_flip_string(uart, ptr, vfifo->size-rxptr);
        DGBUF_PUSH_STR(vfifo, ptr, vfifo->size-rxptr);
        if (txptr) {
            ptr = (unsigned char*)(vfifo->addr);    
            mtk_uart_tty_insert_flip_string(uart, ptr, txptr);
            DGBUF_PUSH_STR(vfifo, ptr, txptr);                 
        }
    }
    dsb();                                                  //make sure read point updated after VFIFO read.
    reg_sync_writel(txreg, VFF_RPT(base));
    tty_flip_buffer_push(tty);

#if defined(ENABLE_VFIFO_DEBUG)    
    if (UART_DEBUG_EVT(DBG_EVT_DMA) && UART_DEBUG_EVT(DBG_EVT_BUF)) {
        char str[4] = {0};    
        if (count >= 4) {
            str[0] = vfifo->cur->dat[0];
            str[1] = vfifo->cur->dat[1];
            str[2] = vfifo->cur->dat[vfifo->cur->idx-2];
            str[3] = vfifo->cur->dat[vfifo->cur->idx-1];
        } else {
            int idx;
            for (idx = 0; idx < count; idx++)
                str[idx] = vfifo->cur->dat[idx];
            for (; idx < 4; idx++)
                str[idx] = 0;
        }
        MSG(DMA, "RX[%4d]: [%5X..%5X] [%5X..%5X] (%02X %02X .. %02X %02X) [%d]\n", 
            left, rxreg, txreg, UART_READ32(VFF_RPT(base)), UART_READ32(VFF_WPT(base)),
            str[0], str[1], str[2], str[3], UART_READ32(VFF_FLUSH(base)));    
    } else {
        MSG(DMA, "RX[%4d]: [%5X..%5X] [%5X..%5X] [%d] [%4X.%4X]\n", 
            left, rxreg, txreg, UART_READ32(VFF_RPT(base)), UART_READ32(VFF_WPT(base)),UART_READ32(VFF_FLUSH(base)),
            UART_READ32(VFF_VALID_SIZE(base)), UART_READ32(VFF_LEFT_SIZE(base)));    
    }
#endif    
    spin_unlock_irqrestore(&port->lock, flags);


#if defined(ENABLE_VFIFO_DEBUG)
    if (UART_DEBUG_EVT(DBG_EVT_DAT) && UART_DEBUG_EVT(DBG_EVT_BUF)) {
        int i;
        printk("[UART%d_RX] %4d bytes:", uart->nport, vfifo->cur->idx);
        
        for (i = 0; i < vfifo->cur->idx; i++) {        
            if (i % 16 == 0)
                printk("\n");
            printk("%.2x ", (unsigned char)vfifo->cur->dat[i]);
        }
        printk("\n");
        
    }
#endif
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_dma_vfifo_rx_tasklet(unsigned long arg)
{   /*the function will be called through dma irq or tasklet_schedule*/
    struct mtk_uart *uart = (struct mtk_uart*)arg;
    struct mtk_uart_vfifo *vfifo = uart->rx_vfifo;
    unsigned long flags;

    //MSG(DMA, "%d, %x, %x\n", uart->read_allow(uart), UART_READ32(VFF_VALID_SIZE(vfifo->base)), vfifo->trig);
    spin_lock_irqsave(&vfifo->iolock, flags);
    if (atomic_inc_and_test(&vfifo->entry) > 1) {
        MSG(ERR, "rx entry!!\n");
        tasklet_schedule(&vfifo->dma->tasklet);        
    } else {
        if (uart->read_allow(uart))
            mtk_uart_dma_vfifo_rx_tasklet_str(arg);
    }
    atomic_dec(&vfifo->entry);
    spin_unlock_irqrestore(&vfifo->iolock, flags);
}
/*---------------------------------------------------------------------------*/
//static __tcmfunc irqreturn_t mtk_vfifo_irq_handler(int irq, void *dev_id)
static irqreturn_t mtk_vfifo_irq_handler(int irq, void *dev_id)
{
    struct mtk_uart_vfifo *vfifo;
    vfifo = (struct mtk_uart_vfifo *)dev_id;
 
    if (!vfifo){
	printk(KERN_ERR "mtk_vfifo_irq_handler: vfifo is NULL\n");
	return IRQ_NONE;
    }
    if (!vfifo->dma){
	printk(KERN_ERR "mtk_vfifo_irq_handler: dma is NULL\n");
	return IRQ_NONE;
    }

    /* Call call back function */
    mtk_uart_dma_vfifo_callback(vfifo->dma);

    /* Clear interrupt flag */
    if(vfifo->type == UART_RX_VFIFO)
		reg_sync_writel(0x3 , VFF_INT_FLAG(vfifo->base));
	else
		reg_sync_writel(0x0 , VFF_INT_FLAG(vfifo->base));

    return IRQ_HANDLED;
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_dma_setup(struct mtk_uart *uart,
                                  struct mtk_uart_dma *dma)
{    
    void *base;
    if (!dma)
        return;

    if (dma->mode == UART_RX_VFIFO_DMA || dma->mode == UART_TX_VFIFO_DMA) {
        if (!dma->vfifo) {
            MSG(ERR, "null\n");
            return;
        }        
        base = dma->vfifo->base;
        reg_sync_writel(dma->vfifo->dmahd, VFF_ADDR(base));
        reg_sync_writel(dma->vfifo->trig, VFF_THRE(base));
        reg_sync_writel(dma->vfifo->size, VFF_LEN(base));    

        if (dma->vfifo->type == UART_RX_VFIFO)
            //reg_sync_writel(VFF_RX_INT_EN0_B, VFF_INT_EN(base));
            reg_sync_writel(VFF_RX_INT_EN0_B|VFF_RX_INT_EN1_B, VFF_INT_EN(base));
        dsb();
    }
}
/*---------------------------------------------------------------------------*/
static int mtk_uart_dma_start(struct mtk_uart *uart,
                                 struct mtk_uart_dma *dma)

{
    void *base;
    MSG_FUNC_ENTRY();

    if (!dma)
        return -1;
        
    if (!atomic_read(&dma->free))
        return -1;

    if (dma->mode == UART_TX_VFIFO_DMA || dma->mode == UART_RX_VFIFO_DMA) {
        if (!dma->vfifo) {
            MSG(ERR, "null\n");
            return -EINVAL;
        }
        base = dma->vfifo->base;
        reg_sync_writel(VFF_INT_FLAG_CLR_B, VFF_INT_FLAG(base));
        reg_sync_writel(VFF_EN_B, VFF_EN(base));

        if (UART_READ32(VFF_EN(base)) != VFF_EN_B)
            MSG(ERR, "Start DMA fail\n");
    }

    atomic_set(&dma->free, 0);
    init_completion(&dma->done);

    return 0;
}
/*---------------------------------------------------------------------------*/
static int mtk_uart_dma_alloc(struct mtk_uart *uart, 
                                 struct mtk_uart_dma *dma, int mode,
                                 struct mtk_uart_vfifo *vfifo)
{
    int ret = 0;

    MSG_FUNC_ENTRY();

    if (mode == UART_NON_DMA)
        return -1;
    
    switch (mode) {
    case UART_TX_VFIFO_DMA:
        if (!vfifo) {
            MSG(ERR, "fail due to NULL tx_vfifo\n");
            ret = -1;
            break;
        }

        vfifo->dma = dma;
        dma->dir   = DMA_TO_DEVICE;   
        dma->mode  = mode;
        dma->vfifo = vfifo;
        dma->uart  = uart;

        init_completion(&dma->done);        
        tasklet_init(&dma->tasklet, mtk_uart_dma_vfifo_tx_tasklet, (unsigned long)uart);

        if (!atomic_read(&vfifo->reg_cb)) {
            /* disable interrupts */
            /* FIXME */
            mtk_uart_vfifo_diable_tx_intr(uart);

            ret = request_irq(vfifo->irq_id, (irq_handler_t)mtk_vfifo_irq_handler, IRQF_TRIGGER_LOW, DRV_NAME, vfifo); 
            if (ret)
                return ret;
            atomic_set(&vfifo->reg_cb, 1);
        } 

        atomic_set(&dma->free, 1);

        break;
        
    case UART_RX_VFIFO_DMA:
        if (!vfifo) {
            MSG(ERR, "fail due to NULL rx_vfifo\n");
            ret = -1;
            break;
        }

        vfifo->dma = dma;
        dma->dir   = DMA_FROM_DEVICE;
        dma->mode  = mode;
        dma->vfifo = vfifo;
        dma->uart  = uart;

        init_completion(&dma->done);        
        tasklet_init(&dma->tasklet, mtk_uart_dma_vfifo_rx_tasklet, (unsigned long)uart);

        if (!atomic_read(&vfifo->reg_cb)) {
            /* disable interrupts */
            mtk_uart_vfifo_diable_rx_intr(uart);
#if 0
            mt6589_irq_set_sens(vfifo->irq_id, MT65xx_LEVEL_SENSITIVE);
            mt6589_irq_set_polarity(vfifo->irq_id, MT65xx_POLARITY_LOW);
#endif
            ret = request_irq(vfifo->irq_id, (irq_handler_t)mtk_vfifo_irq_handler, IRQF_TRIGGER_LOW, DRV_NAME, vfifo); 
            if (ret)
                return ret;
            atomic_set(&vfifo->reg_cb, 1);
        }
            
        //vfifo->dma = dma;
        atomic_set(&dma->free, 1);
        //init_completion(&dma->done);        
        //tasklet_init(&dma->tasklet, mtk_uart_dma_vfifo_rx_tasklet, (unsigned long)uart);
        //dma->dir   = DMA_FROM_DEVICE;
        //dma->mode  = mode;
        //dma->vfifo = vfifo;
        //dma->uart  = uart;        
        break;
    }
    return ret;
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_stop_dma(struct mtk_uart_dma *dma)
{
    int polling_cnt=0;
    struct mtk_uart* uart = dma->uart;
    void *base;
    if (!dma)
        return;
    if (dma->mode == UART_RX_VFIFO_DMA || dma->mode == UART_TX_VFIFO_DMA) {
        MSG(DMA, "stop dma (%d)\n", dma->mode); 
        if (!dma->vfifo) {
            MSG(ERR, "null\n");
            return;
        }
        base = dma->vfifo->base;
        
        /*set flush as 1 -> wait until flush is 0*/
        reg_sync_writel(VFF_FLUSH_CLR_B, VFF_FLUSH(base));
        while (UART_READ32(VFF_FLUSH(base)))
	{
	    polling_cnt++;
	    if (polling_cnt > 10000){
		printk("mtk_uart_stop_dma: polling VFF_FLUSH fail VFF_DEBUG_STATUS=0x%x\n", UART_READ32(VFF_DEBUG_STATUS(base)));
		break;
	    }
	}

	polling_cnt = 0;
        /*set stop as 1 -> wait until en is 0 -> set stop as 0*/
        reg_sync_writel(VFF_STOP_B, VFF_STOP(base));
        while (UART_READ32(VFF_EN(base)))
	{
	    polling_cnt++;
	    if (polling_cnt > 10000){
		printk("mtk_uart_stop_dma: polling VFF_EN fail VFF_DEBUG_STATUS=0x%x\n", UART_READ32(VFF_DEBUG_STATUS(base)));
		break;
	    }
	}
        reg_sync_writel(VFF_STOP_CLR_B, VFF_STOP(base));

        reg_sync_writel(VFF_INT_EN_CLR_B, VFF_INT_EN(base));        
        reg_sync_writel(VFF_INT_FLAG_CLR_B, VFF_INT_FLAG(base));
    } else {
        MSG(ERR, "unknown mode: %d\n", dma->mode);
    }
}
/*---------------------------------------------------------------------------*/
void mtk_uart_dma_stop(struct mtk_uart *uart, 
                                 struct mtk_uart_dma *dma)
{
    MSG_FUNC_ENTRY();
    if (!dma)
        return;

    mtk_uart_stop_dma(dma);
    atomic_set(&dma->free, 1);
    complete(&dma->done);
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_reset_dma(struct mtk_uart_dma *dma)
{
    struct mtk_uart *uart = dma->uart;
    void* base;
    if (!dma)
        return;

    if (dma->mode == UART_RX_VFIFO_DMA || dma->mode == UART_TX_VFIFO_DMA) {
        if (!dma->vfifo) {
            MSG(ERR, "null\n");
            return;
        }
        base = dma->vfifo->base;
        //mt65xx_req_vff_dma(dma->vfifo->ch, NULL, NULL);
        reg_sync_writel(0, VFF_ADDR(base));
        reg_sync_writel(0, VFF_THRE(base));
        reg_sync_writel(0, VFF_LEN(base)); 
        /*set warm_rst as 1 -> wait until en is 0*/
        reg_sync_writel(VFF_WARM_RST_B, VFF_RST(base));
        while (UART_READ32(VFF_EN(base)));

        /* Reset write point for tx dma */
        if(dma->mode == UART_TX_VFIFO_DMA){
		reg_sync_writel(0, VFF_WPT(base));
        }else if(dma->mode == UART_RX_VFIFO_DMA){
		reg_sync_writel(0, VFF_RPT(base));
        }
        
        mtk_uart_vfifo_disable(uart, dma->vfifo);    
        mtk_uart_vfifo_free(uart, dma->vfifo);            
    } else {
        MSG(ERR, "unknown mode: %d\n", dma->mode);
    }
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_dma_free(struct mtk_uart *uart, 
                                 struct mtk_uart_dma *dma)
{
    unsigned long flags;
    
    MSG_FUNC_ENTRY();

    if (!dma)
        return;

    if (dma->mode == UART_NON_DMA)
        return; 
    
    if ((dma->mode == UART_RX_VFIFO_DMA || dma->mode == UART_TX_VFIFO_DMA) && (!dma->vfifo))
        return;

    if (dma->vfifo && !mtk_uart_vfifo_is_empty(dma->vfifo)) {
        tasklet_schedule(&dma->tasklet);
        MSG(DMA, "wait for %s vfifo dma completed!!!\n", 
            dma->dir == DMA_TO_DEVICE ? "TX" : "RX");    
        wait_for_completion(&dma->done);          
    }
    spin_lock_irqsave(&uart->port.lock, flags);
    mtk_uart_stop_dma(dma);
    if (dma->vfifo && timer_pending(&dma->vfifo->timer))
        del_timer_sync(&dma->vfifo->timer);
    if (dma->vfifo && hrtimer_active(&dma->vfifo->flush))
        hrtimer_cancel(&dma->vfifo->flush);
    /* [ALPS00030487] tasklet_kill function may schedule, so release spin lock first,
     *			after release, set spin lock again.
     */
    spin_unlock_irqrestore(&uart->port.lock, flags); /* [ALPS00030487] Add this */
    tasklet_kill(&dma->tasklet);
    spin_lock_irqsave(&uart->port.lock, flags); /* [ALPS00030487] Add this */
    mtk_uart_reset_dma(dma);
    MSG(INFO, "free %s dma completed!!!\n", 
        dma->dir == DMA_TO_DEVICE ? "TX" : "RX");  
    memset(dma, 0, sizeof(struct mtk_uart_dma));
    spin_unlock_irqrestore(&uart->port.lock, flags);   

}
#endif /*defined(ENABLE_VFIFO)*/
/*---------------------------------------------------------------------------*/
inline static void mtk_uart_fifo_init(struct mtk_uart *uart)
{
    /* 
     * NOTE: For FCR is a read only register reason, 
     *       special read/write/set/clr function need to use
     */
    //UART_SET_BITS(UART_FCR_FIFO_INIT, UART_FCR);
    __set_fcr_register(uart, UART_FCR_FIFO_INIT);
    dsb();
}
/*---------------------------------------------------------------------------*/
inline static void mtk_uart_fifo_flush(struct mtk_uart *uart)
{
    /* 
     * NOTE: For FCR is a read only register reason, 
     *       special read/write/set/clr function need to use
     */
    //UART_SET_BITS(UART_FCR_CLRR | UART_FCR_CLRT, UART_FCR);
    __set_fcr_register(uart, UART_FCR_CLRR | UART_FCR_CLRT);
    dsb();    
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_fifo_set_trig(struct mtk_uart *uart, int tx_level, 
                                      int rx_level)
{
    u32 base = uart->base;
    unsigned long tmp1;
	unsigned long flags;

    tmp1 = UART_READ32(UART_LCR);
    spin_lock_irqsave(&mtk_console_lock, flags);
    reg_sync_writel(0xbf, UART_LCR);
    UART_SET_BITS(UART_EFR_EN, UART_EFR);
    reg_sync_writel(tmp1, UART_LCR);
    spin_unlock_irqrestore(&mtk_console_lock, flags);
    MSG(INFO, "%s(EFR) =  %04X\n", __func__, UART_READ32(UART_EFR));

    /* 
     * NOTE: For FCR is a read only register reason, 
     *       special read/write/set/clr function need to use
     */
    //reg_sync_writel(UART_FCR_FIFO_INIT|tx_level|rx_level, UART_FCR);
    sync_write_fcr_register(uart, UART_FCR_FIFO_INIT|tx_level|rx_level);
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_set_mode(struct mtk_uart *uart, int mode)
{
    /* 
     * NOTE: For FCR is a read only register reason, 
     *       special read/write/set/clr function need to use
     */
    if (mode == UART_DMA_MODE_0) {
        //UART_CLR_BITS(UART_FCR_DMA1, UART_FCR);
        __clr_fcr_register(uart, UART_FCR_DMA1);
    } else if (mode == UART_DMA_MODE_1) {
        //UART_SET_BITS(UART_FCR_DMA1, UART_FCR);
        __set_fcr_register(uart, UART_FCR_DMA1);
    }
    dsb();
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_set_auto_baud(struct mtk_uart *uart)
{
    u32 base = uart->base;

    MSG_FUNC_ENTRY();
        
    switch (uart->sysclk)
    {
    case MTK_SYSCLK_13:
        reg_sync_writel(UART_AUTOBADUSAM_13M, UART_AUTOBAUD_SAMPLE);
        break;
    case MTK_SYSCLK_26:
         reg_sync_writel(UART_AUTOBADUSAM_26M, UART_AUTOBAUD_SAMPLE);                     
        break;
    case MTK_SYSCLK_52:
        reg_sync_writel(UART_AUTOBADUSAM_52M, UART_AUTOBAUD_SAMPLE);
        break;
    default:
        dev_err(uart->port.dev, "SYSCLK = %ldMHZ doesn't support autobaud\n",
                uart->sysclk);
        return;
    }
    reg_sync_writel(0x01, UART_AUTOBAUD_EN); /* Enable Auto Baud */
    return;
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_cal_baud(struct mtk_uart *uart, int baudrate, int highspeed)
{
    u32 base = uart->base, remainder, uartclk = 0, divisor = 0;
    u32 lcr = UART_READ32(UART_LCR);
	unsigned long flags;
    
    #ifdef UART_USING_FIX_CLK_ENABLE
    if(baudrate <= 1000000){		/* Using 16.25 fix clock */
        uartclk = uart->sysclk>>2;
        reg_sync_writel(0x03, UART_RATE_FIX_AD);
    }else{				/* >1M, Using 65 clock */
        uartclk = uart->sysclk;
        reg_sync_writel(0x00, UART_RATE_FIX_AD);
    }
    if(3 == highspeed)
        UART_SET_BITS(UART_MCR_DCM_EN, UART_MCR);/* Enable UART DCM */
    else
    	UART_CLR_BITS(UART_MCR_DCM_EN, UART_MCR);/* Disable UART DCM */
    #else  /* UART_Fix_clk_DISABLE */
    uartclk = uart->sysclk;
    reg_sync_writel(0x00, UART_RATE_FIX_AD);
    #endif /* UART_USING_FIX_CLK_ENABLE */

    spin_lock_irqsave(&mtk_console_lock, flags);
    if (highspeed == 0) {
        //uartclk = uart->sysclk;
        //reg_sync_writel(0x00, UART_RATE_FIX_AD);
        reg_sync_writel(0x00, UART_HIGHSPEED); /*divider is 16*/
        divisor   = (uartclk >> 4)/(u32)baudrate;        
        remainder = (uartclk >> 4)%(u32)baudrate;        
        if (remainder >= (u32)(baudrate*8))
            divisor += 1;
        reg_sync_writel(lcr|UART_LCR_DLAB, UART_LCR);
        reg_sync_writel((divisor&0xFF), UART_DLL);
        reg_sync_writel(((divisor>>8) & 0xFF), UART_DLH);
        reg_sync_writel(lcr, UART_LCR);
    } else if (highspeed == 1) {
        //uartclk = uart->sysclk;
        //reg_sync_writel(0x00, UART_RATE_FIX_AD);
        reg_sync_writel(0x01, UART_HIGHSPEED); /*divider is 8*/
        divisor   = (uartclk >> 3)/(u32)baudrate;        
        remainder = (uartclk >> 3)%(u32)baudrate;        
        if (remainder >= (u32)(baudrate*4))
            divisor += 1;
        reg_sync_writel(lcr|UART_LCR_DLAB, UART_LCR);
        reg_sync_writel((divisor&0xFF), UART_DLL);
        reg_sync_writel(((divisor>>8) & 0xFF), UART_DLH);
        reg_sync_writel(lcr, UART_LCR);
    } else if (highspeed == 2) {
        //uartclk = uart->sysclk;
        //reg_sync_writel(0x00, UART_RATE_FIX_AD);
        reg_sync_writel(0x02, UART_HIGHSPEED); /*divider is 4*/
        divisor   = (uartclk >> 2)/(u32)baudrate;        
        remainder = (uartclk >> 2)%(u32)baudrate;        
        if (remainder >= (u32)(baudrate*2))
            divisor += 1;
        reg_sync_writel(lcr|UART_LCR_DLAB, UART_LCR);
        reg_sync_writel((divisor&0x00FF), UART_DLL);
        reg_sync_writel(((divisor>>8) & 0x00FF), UART_DLH);
        reg_sync_writel(lcr, UART_LCR);
    } else if (highspeed == 3) {
        u32 sample_count, sample_point, high_div, tmp;
    #if defined(ENABLE_FRACTIONAL)    
        u32 fraction;
        u16 fraction_L_mapping[] = {0, 1, 0x5, 0x15, 0x55, 0x57, 0x57, 0x77, 0x7F, 0xFF, 0xFF};
        u16 fraction_M_mapping[] = {0, 0,   0,    0,    0,    0,    1,    1,    1,    1,  0x3};        
    #endif    
        
        //uartclk = uart->sysclk;
        //reg_sync_writel(0x00, UART_RATE_FIX_AD);
        reg_sync_writel(0x03, UART_HIGHSPEED);
        tmp = (uartclk)/(u32)baudrate;
        high_div = (tmp >> 8) + 1;                 
        divisor = (uartclk)/(baudrate*high_div);

    #if defined(ENABLE_FRACTIONAL)
        fraction = ((uartclk*10)/baudrate)%10; /*count fraction to set fractoin register*/
        reg_sync_writel(fraction_L_mapping[fraction], UART_FRACDIV_L);
        reg_sync_writel(fraction_M_mapping[fraction], UART_FRACDIV_M);
    #else
        remainder = (uartclk)%(baudrate*high_div);
        if (remainder >= ((baudrate*high_div) >> 1))
            divisor += 1;
    #endif        
        
        sample_count = divisor - 1;
        sample_point = (sample_count-1) >> 1;
        reg_sync_writel(lcr|UART_LCR_DLAB, UART_LCR);
        reg_sync_writel((high_div & 0x00FF), UART_DLL);
        reg_sync_writel(((high_div>>8) & 0x00FF), UART_DLH);
        reg_sync_writel(lcr, UART_LCR);
        reg_sync_writel(sample_count, UART_SAMPLE_COUNT);
        reg_sync_writel(sample_point, UART_SAMPLE_POINT);
        /*
         * NOTICE: We found some chip, that is using lower clock, may not have enough time to check stop bit.
         *         In order to improve compatibilty, the guard time register is enabled which is used to extend the stop bit. 
         */
        if (baudrate >= 3000000)
            reg_sync_writel(0x12, UART_GUARD);     
    } 
    spin_unlock_irqrestore(&mtk_console_lock, flags);
       
    MSG(CFG, "BaudRate = %d, SysClk = %d, Divisor = %d, %04X/%04X\n", baudrate, uartclk, divisor, UART_READ32(UART_IER), UART_READ32(UART_LCR));
    dump_reg(uart, __func__);    
    dsb();  /*to ensure the setting is written*/    
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_set_baud(struct mtk_uart *uart , int baudrate)
{
    u32 uartclk;
#if defined(CONFIG_MT6589_FPGA)
    u32 tmp_div;
#endif
    uartclk = uart->sysclk;
    
    if (uart->port.flags & ASYNC_SPD_CUST) {
    	/**
    	 * [ALPS00137126] Begin
    	 * Because the origin design of custom baudrate in linux is for low speed case, we add some
    	 * modify to support high speed case. 
    	 * NOTE: If the highest bit of "custom_divisor" is ONE, we will use custom_divisor store baudrate
    	 * directly. That means(we suppose unsigned int is 32 bits):
    	 *     custom_divisor[31] == 1, then custom_divisor[30..0] == custom baud rate
    	 *     custom_divisor[31] == 0, then custom_divisor[30..0] == sysclk/16/baudrate
    	 */
    	if(uart->port.custom_divisor & (1<<31) ){
    	    baudrate = uart->port.custom_divisor&(~(1<<31));
    	    if( baudrate > (uart->sysclk>>2) ) /* Baud rate should not more than sysclk/4 */
    	    	baudrate = 9600;
    	}else{
            /*the baud_base gotten in user space eqauls to sysclk/16.
              hence, we need to restore the difference when calculating custom baudrate */
            if (!uart->custom_baud) {
                baudrate = uart->sysclk/16; 
                baudrate = baudrate/uart->port.custom_divisor;
            } else {
                baudrate = uart->custom_baud;
            }
    	 /* [ALPS00137126] End */
        }
        MSG(CFG, "CUSTOM, baudrate = %d, divisor = %d\n", baudrate, uart->port.custom_divisor);
    }

    if (uart->auto_baud)
        mtk_uart_set_auto_baud(uart); 

#if defined(CONFIG_MT6589_FPGA)
    tmp_div = (uartclk)/(unsigned int)baudrate;
    if (tmp_div > 255) 
        mtk_uart_cal_baud(uart, baudrate, 2);
    else 
        mtk_uart_cal_baud(uart, baudrate, 3);
#else
    /* Fix clock, using new settings */
    #ifdef UART_USING_FIX_CLK_ENABLE
    if (baudrate < 115200) 
        mtk_uart_cal_baud(uart, baudrate, 0);
    else
        mtk_uart_cal_baud(uart, baudrate, 3);
    #else  /* UART_Fix_Clock_DISABLE */
    if (baudrate <= 115200) 
        mtk_uart_cal_baud(uart, baudrate, 0);
    else if (baudrate <= 460800) 
        mtk_uart_cal_baud(uart, baudrate, 2);
    else
        mtk_uart_cal_baud(uart, baudrate, 3);
    #endif /* End of UART_DCM_CONFIG */
#endif    
    uart->baudrate = baudrate;
}
/*---------------------------------------------------------------------------*/
#if defined(ENABLE_DEBUG)
/*---------------------------------------------------------------------------*/
static u32 UART_READ_EFR(struct mtk_uart *uart)
{
    u32 base = uart->base;
    u32 efr, lcr = UART_READ32(UART_LCR);
	unsigned long flags;

    spin_lock_irqsave(&mtk_console_lock, flags);
    reg_sync_writel(0xbf, UART_LCR);
    efr = UART_READ32(UART_EFR);
    reg_sync_writel(lcr, UART_LCR);
    spin_unlock_irqrestore(&mtk_console_lock, flags);
    return efr;
}
/*---------------------------------------------------------------------------*/
#endif 
/*---------------------------------------------------------------------------*/
static void mtk_uart_set_flow_ctrl(struct mtk_uart *uart, int mode)
{
    u32 base = uart->base, old;
    unsigned int tmp = UART_READ32(UART_LCR);
	unsigned long flags;

    MSG(CFG, "%s: %04X\n", __func__, UART_READ_EFR(uart));

    spin_lock_irqsave(&mtk_console_lock, flags);
    switch (mode) {
    case UART_FC_NONE:
	reg_sync_writel(UART_ESCAPE_CH, UART_ESCAPE_DAT);
        reg_sync_writel(0x00, UART_ESCAPE_EN);
        reg_sync_writel(0xbf, UART_LCR);        
        old = UART_READ32(UART_EFR);
        old &= ~(UART_EFR_AUTO_RTSCTS|UART_EFR_XON12_XOFF12);
        reg_sync_writel(old, UART_EFR);
        reg_sync_writel(tmp, UART_LCR);       
        mtk_uart_disable_intrs(uart, UART_IER_XOFFI|UART_IER_RTSI|UART_IER_CTSI);
        break;
    case UART_FC_HW:
	reg_sync_writel(UART_ESCAPE_CH, UART_ESCAPE_DAT);
        reg_sync_writel(0x00, UART_ESCAPE_EN);
        UART_SET_BITS(UART_MCR_RTS, UART_MCR);
        reg_sync_writel(0xbf, UART_LCR);        
        /*disable all flow control setting*/
        old = UART_READ32(UART_EFR);               
        old &= ~(UART_EFR_AUTO_RTSCTS | UART_EFR_XON12_XOFF12);
        reg_sync_writel(old, UART_EFR);
        /*enable hw flow control*/
        old = UART_READ32(UART_EFR);
        reg_sync_writel(old | UART_EFR_AUTO_RTSCTS, UART_EFR);
        reg_sync_writel(tmp, UART_LCR);  
        mtk_uart_disable_intrs(uart, UART_IER_XOFFI);
        mtk_uart_enable_intrs(uart, UART_IER_CTSI|UART_IER_RTSI);         
        break;
    case UART_FC_SW: /*MTK software flow control*/
        reg_sync_writel(UART_ESCAPE_CH, UART_ESCAPE_DAT);
        reg_sync_writel(0x01, UART_ESCAPE_EN);
        reg_sync_writel(0xbf, UART_LCR);        
        /*dsiable all flow control setting*/
        old = UART_READ32(UART_EFR);               
        old &= ~(UART_EFR_AUTO_RTSCTS | UART_EFR_XON12_XOFF12);
        reg_sync_writel(old, UART_EFR);
        /*enable sw flow control*/
        old = UART_READ32(UART_EFR);
        reg_sync_writel(old | UART_EFR_XON1_XOFF1, UART_EFR);
        reg_sync_writel(START_CHAR(uart->port.state->port.tty), UART_XON1);
        reg_sync_writel(STOP_CHAR(uart->port.state->port.tty), UART_XOFF1);
        reg_sync_writel(tmp, UART_LCR);  
        mtk_uart_disable_intrs(uart, UART_IER_CTSI|UART_IER_RTSI);
        mtk_uart_enable_intrs(uart, UART_IER_XOFFI); 
        break;
    }
    spin_unlock_irqrestore(&mtk_console_lock, flags);
    dsb();  /*to ensure the setting is written*/
    uart->fctl_mode = mode;
}
/*---------------------------------------------------------------------------*/
#ifndef CONFIG_MT6589_FPGA
/* UART Powr PDN and Reset*/
#define AP_PERI_GLOBALCON_RST0 (PERICFG_BASE+0x0)
#define AP_PERI_GLOBALCON_PDN0 (PERICFG_BASE+0x10)
#endif
/*---------------------------------------------------------------------------*/
inline static void mtk_uart_power_up(struct mtk_uart *uart)
{
#ifndef CONFIG_MT6589_FPGA
    struct mtk_uart_setting *setting;
    setting = uart->setting;

    if (!uart || uart->nport >= UART_NR)
        return;

    if (uart->poweron_count++ > 0) {
        MSG(FUC, "%s(%d)\n", __func__, uart->poweron_count);
    } else {
    #ifdef POWER_FEATURE
        if (0 != enable_clock(setting->pll_id,"UART"))
            MSG(ERR, "power on fail!!\n");
        if (0 != enable_clock(PDN_FOR_DMA, "VFIFO") )
            MSG(ERR, "power on dma fail!\n");
    #else
        //UART_SET_BITS((1 << setting->set_bit), AP_PERI_GLOBALCON_PDN0);
    #endif
    }
    MSG(FUC, "%s(%d) => up \n", __func__, uart->poweron_count); 
#endif /* End of CONFIG_MT6589_FPGA */
}
/*---------------------------------------------------------------------------*/
inline static void mtk_uart_power_down(struct mtk_uart *uart)
{
#ifndef CONFIG_MT6589_FPGA
    struct mtk_uart_setting *setting;
    setting = uart->setting;

    if (!uart || uart->nport >= UART_NR)
        return;

    if ((uart->poweron_count >= 1) && (--uart->poweron_count > 0)) {
        MSG(FUC, "%s(%d)\n", __func__, uart->poweron_count);
    } else {
    #ifdef POWER_FEATURE
        if (0 != disable_clock(setting->pll_id,"UART"))
            MSG(ERR, "power off fail!!\n");
        if (0 != disable_clock(PDN_FOR_DMA, "VFIFO") )
            MSG(ERR, "power off dma fail!\n");
    #else
        //UART_CLR_BITS((1 << setting->clr_bit), AP_PERI_GLOBALCON_PDN0);
    #endif
        MSG(FUC, "%s(%d) => dn \n", __func__, uart->poweron_count);
    }
#endif /* End of CONFIG_MT6589_FPGA */
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_config(struct mtk_uart *uart, 
                               int baud, int datalen, int stop, int parity)
{
    u32 base = uart->base;
    unsigned int val = 0;

    switch (datalen)
    {
    case 5:
        val |= UART_WLS_5;
        break;
    case 6:
        val |= UART_WLS_6;
        break;
    case 7:
        val |= UART_WLS_7;
        break;
    case 8:
    default:
        val |= UART_WLS_8;
        break;
    }
    
    if (stop == 2 || (datalen == 5 && stop == 1))
        val |= UART_2_STOP;

    if (parity == 1)
        val |= UART_ODD_PARITY;
    else if (parity == 2)
        val |= UART_EVEN_PARITY;

    reg_sync_writel(val, UART_LCR); 

    mtk_uart_set_baud(uart, baud);
}
/*---------------------------------------------------------------------------*/
static unsigned int mtk_uart_read_status(struct mtk_uart *uart)
{
    u32 base = uart->base;

    uart->line_status = UART_READ32(UART_LSR);
    return uart->line_status;
}
/*---------------------------------------------------------------------------*/
static unsigned int mtk_uart_read_allow(struct mtk_uart *uart)
{
    return uart->line_status & UART_LSR_DR;
}
/*---------------------------------------------------------------------------*/
/* Note:
 * 1. FIFO mode:
 *    -THRE=1 : when free space in FIFO is reduced blow its trigger level
 *    -THRE=0 : when free space in FIFO is more than its trigger level
 * 2. non-FIFO mode:
 *    -THRE=1 : when tx holding register is empty
 *    -THRE=0 : when tx holding register is not empty
 */
static unsigned int mtk_uart_write_allow(struct mtk_uart *uart)
{
    u32 base = uart->base;
    return UART_READ32(UART_LSR) & UART_LSR_THRE;
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_enable_intrs(struct mtk_uart *uart, long mask)
{   /*assume UART_EFR_EN is on*/
    u32 base = uart->base;    

    UART_SET_BITS(mask, UART_IER);
    dsb();
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_disable_intrs(struct mtk_uart *uart, long mask)
{   /*assume UART_EFR_EN is on*/
    u32 base = uart->base;

    UART_CLR_BITS(mask, UART_IER);
    dsb();
}
/*---------------------------------------------------------------------------*/
static unsigned int mtk_uart_read_byte(struct mtk_uart *uart)
{
    u32 base = uart->base;
    return UART_READ32(UART_RBR);
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_write_byte(struct mtk_uart *uart, unsigned int byte)
{
    u32 base = uart->base;
    reg_sync_writel(byte, UART_THR);
}
/*---------------------------------------------------------------------------*/
static unsigned int mtk_uart_filter_line_status(struct mtk_uart *uart)
{
    struct uart_port *port = &uart->port;
    unsigned int status;
    unsigned int lsr = uart->line_status;

    mtk_uart_lsr_status[uart->nport] |= lsr;
    status = UART_LSR_BI|UART_LSR_PE|UART_LSR_FE|UART_LSR_OE;

#ifdef ENABLE_DEBUG
    if ((lsr & UART_LSR_BI) || (lsr & UART_LSR_PE) ||
        (lsr & UART_LSR_FE) || (lsr & UART_LSR_OE)) {
        MSG(ERR, "LSR: BI=%d, FE=%d, PE=%d, OE=%d, DR=%d\n",
            (lsr & UART_LSR_BI) >> 4, (lsr & UART_LSR_FE) >> 3, 
            (lsr & UART_LSR_PE) >> 2, (lsr & UART_LSR_OE) >> 1, 
             lsr & UART_LSR_DR); 
    }
#endif    
    status &= port->read_status_mask;
    status &= ~port->ignore_status_mask;
    status &= lsr;
    
    return status;
}
/*---------------------------------------------------------------------------*/
static inline bool mtk_uart_enable_sysrq(struct mtk_uart *uart) 
{
    return uart->setting->sysrq;
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_rx_chars(struct mtk_uart *uart)
{
    struct uart_port *port = &uart->port;
    struct tty_struct *tty = uart->port.state->port.tty;
    int max_count = UART_FIFO_SIZE;
    unsigned int data_byte, status;
    unsigned int flag;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);  
    //MSG_FUNC_ENTRY();
    while (max_count-- > 0) {

        /* check status */
        if (!(uart->read_status(uart) & UART_LSR_DR))
            break;
#if 0
        if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
            if (tty->low_latency) {
                /*
                 * If this failed then we will throw away the
                 * bytes but must do so to clear interrupts
                 */
                tty_flip_buffer_push(tty);
            }
        }
#endif
        /* read the byte */
        data_byte = uart->read_byte(uart);
        port->icount.rx++;
        flag = TTY_NORMAL;

        status = mtk_uart_filter_line_status(uart);
        
        /* error handling routine */
        if (status & UART_LSR_BI) {
            MSG(INFO, "Break interrupt!!\n");
            port->icount.brk++;
            if (uart_handle_break(port))
                continue;
            flag = TTY_BREAK;
        } else if (status & UART_LSR_PE) {
            MSG(INFO, "Parity Error!!\n");
            port->icount.parity++;
            flag = TTY_PARITY;
        } else if (status & UART_LSR_FE) {
            MSG(INFO, "Frame Error!!\n");
            port->icount.frame++;
            flag = TTY_FRAME;
        } else if (status & UART_LSR_OE) {
            MSG(INFO, "Overrun!!\n");
            port->icount.overrun++;
            flag = TTY_OVERRUN;
        }

#ifdef CONFIG_MAGIC_SYSRQ        
        if (mtk_uart_enable_sysrq(uart))
        {
            if (uart_handle_sysrq_char(port, data_byte))
                continue;

            /* FIXME. Infinity, 20081002, 'BREAK' char to enable sysrq handler { */
        #if defined(CONFIG_MAGIC_SYSRQ) && defined(CONFIG_SERIAL_CORE_CONSLE)
            if (data_byte == 0)
                uart->port.sysrq = 1;
        #endif
            /* FIXME. Infinity, 20081002, 'BREAK' char to enable sysrq handler } */
        }
#endif         

        if (!tty_insert_flip_char(tty, data_byte, flag))
            MSG(ERR, "tty_insert_flip_char: no space");
    }
    tty_flip_buffer_push(tty);
	spin_unlock_irqrestore(&port->lock, flags);  
    MSG(FUC, "%s (%2d)\n", __FUNCTION__, UART_FIFO_SIZE - max_count - 1);
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_tx_chars(struct mtk_uart *uart)
{
    /* Notice:
     * The function is called by uart_start, which is protected by spin lock,
     * Hence, no spin-lock is required in the functions
     */
    
    struct uart_port *port = &uart->port;
    struct circ_buf *xmit = &port->state->xmit;
    int count;

    /* deal with x_char first */
    if (unlikely(port->x_char)) {
        MSG(INFO, "detect x_char!!\n");
        uart->write_byte(uart, port->x_char);
        port->icount.tx++;
        port->x_char = 0;
        return;
    }

    /* stop tx if circular buffer is empty or this port is stopped */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port)) 
    {
        struct tty_struct *tty = port->state->port.tty;
        if (!uart_circ_empty(xmit))
            MSG(ERR, "\t\tstopped: empty: %d %d %d\n", uart_circ_empty(xmit), tty->stopped, tty->hw_stopped);
        mtk_uart_stop_tx(port);
        return;
    }

    count = port->fifosize - 1;

    do {
        if (uart_circ_empty(xmit))
            break;
         if ((count == 1) || (!uart->write_allow(uart))) {
            //to avoid the interrupt is not enable.
            mtk_uart_enable_intrs(uart, UART_IER_ETBEI);
            break;
        }
        uart->write_byte(uart, xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;

    } while (--count > 0);

    MSG(INFO, "TX %d chars\n", port->fifosize - 1 - count);

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);

    if (uart_circ_empty(xmit))
        mtk_uart_stop_tx(port);
}
/*---------------------------------------------------------------------------*/
void mtk_uart_get_modem_status(struct mtk_uart *uart)
{
    u32 base = uart->base;
    struct uart_port *port = &uart->port;
    unsigned int status, delta;

    status  = UART_READ32(UART_MSR);    
    status &= UART_MSR_DSR | UART_MSR_CTS | UART_MSR_DCD | UART_MSR_RI;

    MSG(INFO, "MSR: DCD(%d), RI(%d), DSR(%d), CTS(%d)\n",
        status & UART_MSR_DCD ? 1 : 0, 
        status & UART_MSR_RI ? 1 : 0, 
        status & UART_MSR_DSR ? 1 : 0,
        status & UART_MSR_CTS ? 1 : 0);

    delta = status ^ uart->old_status;

    if (!delta)
        return;

    if (uart->ms_enable) {
        if (delta & UART_MSR_DCD)
            uart_handle_dcd_change(port, status & UART_MSR_DCD);
        if (delta & UART_MSR_CTS)
            uart_handle_cts_change(port, status & UART_MSR_CTS);
        if (delta & UART_MSR_DSR)
            port->icount.dsr++;
        if (delta & UART_MSR_RI)
            port->icount.rng++;
    }

    uart->old_status = status;
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_rx_handler(struct mtk_uart *uart, int intrs)
{
    if (uart->rx_mode == UART_NON_DMA) {
        mtk_uart_rx_chars(uart);
    } else if (uart->rx_mode == UART_RX_VFIFO_DMA) {
#if defined(ENABLE_VFIFO)
        u32 base = uart->base, tmp;
        if (intrs == UART_IIR_CTI) {
            /* IMPORTANT: this is a fix for HW Bug. 
             * Without the function call, the RX data timeout interrupt will be 
             * triggered again and again.Hence, the purpose of this function call
             * is to clear Rx data timeout interrupt
             */            
            tmp = UART_READ32(UART_DMA_EN);
            MSG(DMA, "rx timeout: %x, %4d\n", tmp, mtk_uart_vfifo_get_counts(uart->rx_vfifo));                  
            //mtk_uart_dma_vfifo_rx_tasklet((unsigned long)uart);
        } else if ((intrs == UART_IIR_RLS) && !uart->read_allow(uart)) {
            tmp = UART_READ32(UART_LSR);
            MSG(DMA, "LSR=%X\n", tmp); 
            mtk_uart_lsr_status[uart->nport] |= tmp;
        } else {
            MSG(DMA, "RX = %4d, [%4x]\n", mtk_uart_vfifo_get_counts(uart->rx_vfifo), intrs);
        }
        mtk_uart_dma_vfifo_rx_tasklet((unsigned long)uart);        
#endif
    }
}
/*---------------------------------------------------------------------------*/
void mtk_uart_tx_handler(struct mtk_uart *uart)
{
    struct uart_port *port = &uart->port;
    unsigned long flags;
    if (uart->tx_mode == UART_NON_DMA) {
	spin_lock_irqsave(&port->lock, flags);
        mtk_uart_tx_chars(uart);
        spin_unlock_irqrestore(&port->lock, flags);
    } else if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        tasklet_schedule(&uart->dma_tx.tasklet);
    }
}
/*---------------------------------------------------------------------------*/
#ifdef ENABLE_DEBUG
/*---------------------------------------------------------------------------*/
static const char *fifo[] = {"No FIFO", "Unstable FIFO", 
                             "Unknown", "FIFO Enabled"};
static const char *intrrupt[] = {"Modem Status Chg", "Tx Buffer Empty",
                                 "Rx Data Received", "BI, FE, PE, or OE",
                                 "0x04", "0x05", "Rx Data Timeout", "0x07",
                                 "SW Flow Control", "0x09", "0x10", "0x11", "0x12",
                                 "0x13", "0x14", "0x15", "HW Flow Control"};
/*---------------------------------------------------------------------------*/
#endif
/*---------------------------------------------------------------------------*/
//static __tcmfunc irqreturn_t mtk_uart_irq(int irq, void *dev_id)
static irqreturn_t mtk_uart_irq(int irq, void *dev_id)
{
    unsigned int intrs, timeout = 0;
    struct mtk_uart *uart = (struct mtk_uart *)dev_id;
    u32 base = uart->base;    

    intrs = UART_READ32(UART_IIR);

#ifdef ENABLE_DEBUG
    {
        UART_IIR_REG *iir = (UART_IIR_REG *)&intrs;
        if (iir->NINT)
            MSG(INT, "No interrupt (%s)\n", fifo[iir->FIFOE]);        
        else if (iir->ID < ARRAY_SIZE(intrrupt))
            MSG(INT, "%02x %s (%s)\n", iir->ID, intrrupt[iir->ID], fifo[iir->FIFOE]);
        else
            MSG(INT, "%2x\n", iir->ID);
    }
#endif
    intrs &= UART_IIR_INT_MASK;
    
    if (intrs == UART_IIR_NO_INT_PENDING)
        return IRQ_HANDLED;

    if (intrs == UART_IIR_RLS) {
        /* BE, FE, PE, or OE occurs */
    } else if (intrs == UART_IIR_RDA) {
    } else if (intrs == UART_IIR_CTI) {
        timeout = 1;
    } else if (intrs == UART_IIR_THRE) {
        mtk_uart_tx_handler(uart);
    } else if (intrs == UART_IIR_MS) {
        mtk_uart_get_modem_status(uart);
    } else if (intrs == UART_IIR_SW_FLOW_CTRL) {
        /* XOFF is received */
    } else if (intrs == UART_IIR_HW_FLOW_CTRL) {
        /* CTS or RTS is in rising edge */
    }
    mtk_uart_rx_handler(uart, intrs);        
    return IRQ_HANDLED;
}
/*---------------------------------------------------------------------------*/
/* test whether the transmitter fifo and shifter for the port is empty. */
static unsigned int mtk_uart_tx_empty(struct uart_port *port)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;

    MSG_FUNC_ENTRY();

#if defined(ENABLE_VFIFO)
    if (uart->tx_mode == UART_TX_VFIFO_DMA)
        return mtk_uart_vfifo_is_empty(uart->dma_tx.vfifo) ? TIOCSER_TEMT : 0;
    else
#endif
        return uart->write_allow(uart) ? TIOCSER_TEMT : 0;
}
/*---------------------------------------------------------------------------*/
/* set the modem control lines. */
static void mtk_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;
    u32 base = uart->base;    
    unsigned int val;

    val = UART_READ32(UART_MCR);

    if (mctrl & TIOCM_DTR)
        val |= UART_MCR_DTR;
    else
        val &= ~UART_MCR_DTR;

    if (mctrl & TIOCM_RTS)
        val |= UART_MCR_RTS;
    else
        val &= ~UART_MCR_RTS;

    if (mctrl & TIOCM_OUT1)
        val |= UART_MCR_OUT1;
    else
        val &= ~UART_MCR_OUT1;

    if (mctrl & TIOCM_OUT2)
        val |= UART_MCR_OUT2;
    else
        val &= ~UART_MCR_OUT2;

    if (mctrl & TIOCM_LOOP)
        val |= UART_MCR_LOOP;
    else
        val &= ~UART_MCR_LOOP;

    reg_sync_writel(val, UART_MCR);

    MSG(CFG, "MCR: DTR(%d), RTS(%d), OUT1(%d), OUT2(%d), LOOP(%d)\n",
        val & UART_MCR_DTR ? 1 : 0, 
        val & UART_MCR_RTS ? 1 : 0, 
        val & UART_MCR_OUT1 ? 1 : 0,
        val & UART_MCR_OUT2 ? 1 : 0, 
        val & UART_MCR_LOOP ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
/* return the current state of modem contrl inputs */
static unsigned int mtk_uart_get_mctrl(struct uart_port *port)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;
    u32 base = uart->base;    
    unsigned int status;
    unsigned int result = 0;

    status = UART_READ32(UART_MSR);

    MSG(INFO, "MSR: DCD(%d), RI(%d), DSR(%d), CTS(%d)\n",
        status & UART_MSR_DCD ? 1 : 0, 
        status & UART_MSR_RI ? 1 : 0, 
        status & UART_MSR_DSR ? 1 : 0,
        status & UART_MSR_CTS ? 1 : 0);

    if (status & UART_MSR_DCD)   
        result |= TIOCM_CAR;    /* DCD. (data carrier detect) */
    if (status & UART_MSR_RI)
        result |= TIOCM_RI;
    if (status & UART_MSR_DSR)
        result |= TIOCM_DSR;
    if (status & UART_MSR_CTS)
        result |= TIOCM_CTS;

    status = UART_READ32(UART_MCR);

    MSG(INFO, "MSR: OUT1(%d), OUT2(%d), LOOP(%d)\n",
        status & UART_MCR_OUT1 ? 1 : 0, 
        status & UART_MCR_OUT2 ? 1 : 0, 
        status & UART_MCR_LOOP ? 1 : 0);

    if (status & UART_MCR_OUT2)
        result |= TIOCM_OUT2;
    if (status & UART_MCR_OUT1)
        result |= TIOCM_OUT1;
    if (status & UART_MCR_LOOP)
        result |= TIOCM_LOOP;

    return result;
}
/*---------------------------------------------------------------------------*/
/* FIXME */
/* stop transmitting characters 
 * Note: this function is call with interrupt disabled
 */
static void mtk_uart_stop_tx(struct uart_port *port)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;

    MSG_FUNC_ENTRY();
#if defined(ENABLE_VFIFO)
    if (uart->tx_mode == UART_TX_VFIFO_DMA){
        /*1. UART_IER_ETBEI can't be disabled or zero data appears in TX*/
        /*2. TX_INT_EN.INTEN will be reset automatically by HW*/
    } else
#endif
        /* disable tx interrupt */
        mtk_uart_disable_intrs(uart, UART_IER_ETBEI);
    uart->tx_stop = 1;
}
/*---------------------------------------------------------------------------*/
/* FIXME */
/* start transmitting characters.
 * Note: this function is call with interrupt disabled
 */
static void mtk_uart_start_tx(struct uart_port *port)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;
    struct circ_buf    *xmit = &port->state->xmit;    
    unsigned long size;
    unsigned long flags;
    
    size = CIRC_CNT_TO_END(xmit->head, xmit->tail, UART_XMIT_SIZE);

    if (!size)
        return;

    uart->tx_stop = 0;
    if(uart->nport==2){
	spin_lock_irqsave(&tx_history_lock, flags);
	if(!stop_update){
		curr_record++;
		curr_idx = 0;
		if(curr_record >= RECORD_NUMBER)
			curr_record = 0;
		uart_history_cnt[curr_record] = 0;
		for(curr_idx=0; curr_idx<RECORD_LENGTH; curr_idx++)
			uart_history[curr_record][curr_idx] = 0;
		curr_idx = 0;
	}
	spin_unlock_irqrestore(&tx_history_lock, flags);
    }

#if defined(ENABLE_VFIFO)
    if (uart->tx_mode == UART_TX_VFIFO_DMA) {
	if (UART_DEBUG_EVT(DBG_EVT_BUF))
    	    printk("[UART%d] mtk_uart_start_tx\n", uart->nport);
        if (!uart->write_allow(uart))
            mtk_uart_vfifo_enable_tx_intr(uart);
        else 
            mtk_uart_dma_vfifo_tx_tasklet((unsigned long)uart);
    }
    else {
#else
    {
#endif
        if (uart->write_allow(uart))
            mtk_uart_tx_chars(uart);
    }
}
/*---------------------------------------------------------------------------*/
/* stop receiving characters
 * note: port->lock has been taken by serial core layer 
 */
static void mtk_uart_stop_rx(struct uart_port *port)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;
    struct mtk_uart_dma *dma = &uart->dma_rx;

    MSG_FUNC_ENTRY();
    if (uart->rx_mode == UART_NON_DMA) {
        mtk_uart_disable_intrs(uart, UART_IER_ERBFI);
    } else {
#if defined(ENABLE_VFIFO)
        /* According to serial_core.c, stop_rx is to stop interrupt
         * Hence, RX received interrupt and dma interrupt is clear
         */
        mtk_uart_disable_intrs(uart, UART_IER_ERBFI);
        reg_sync_writel(VFF_INT_EN_CLR_B, VFF_INT_EN(dma->vfifo->base));  
        atomic_set(&dma->free, 1);
        complete(&dma->done);        
#endif
    }
    uart->rx_stop = 1;
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_send_xchar(struct uart_port *port, char ch)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;
    unsigned long flags;

    MSG_FUNC_ENTRY();
    
    if (uart->tx_stop)
        return;

    spin_lock_irqsave(&port->lock, flags);
    while (!mtk_uart_write_allow(uart)); 
    mtk_uart_write_byte(uart, (unsigned char)ch);
    port->icount.tx++; 
    spin_unlock_irqrestore(&port->lock, flags);   
    return;
}
/*---------------------------------------------------------------------------*/
/* enable the modem status interrupts */
static void mtk_uart_enable_ms(struct uart_port *port)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;

    MSG_FUNC_ENTRY();
    uart->ms_enable = 1;
}
/*---------------------------------------------------------------------------*/
/* control the transmission of a break signal */
static void mtk_uart_break_ctl(struct uart_port *port, int break_state)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;
    u32 base = uart->base;

    unsigned long flags;

    MSG_FUNC_ENTRY();
    
    spin_lock_irqsave(&port->lock, flags);
    
    if (break_state)
        UART_SET_BITS(UART_LCR_BREAK, UART_LCR);
    else
        UART_CLR_BITS(UART_LCR_BREAK, UART_LCR);
    dsb();
    spin_unlock_irqrestore(&port->lock, flags);
}
/*---------------------------------------------------------------------------*/
/* grab any interrupt resources and initialize any low level driver state */
static int mtk_uart_startup(struct uart_port *port)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;
    int ret;
    long mask = UART_IER_HW_NORMALINTS;
    
    MSG_FUNC_ENTRY();

    /*the uart port is power up in power_mgnt*/
    
    /*Reset default flag when the uart starts up, or the previous setting,
     *such as custom baudrate will be still applied even it is ever closed 
     */
    //uart->port.flags = UPF_BOOT_AUTOCONF;
    //uart->port.custom_divisor = 1;

    /* Check whether is ATE_Factory mode */
    #ifdef ATE_FACTORY_ENABLE
    if((0 == uart->nport)&&(ATE_FACTORY_MODE == get_boot_mode()))
    {
        u32 base = uart->base;
        /* MD may set these bit, reset it */
        UART_CLR_BITS(UART_RX_DMA_EN|UART_TO_CNT_AUTORST|UART_TX_DMA_EN, UART_DMA_EN);
        dsb();
    }
    #endif /* ATE_FACTORY_ENABLE */

    uart->fctl_mode     = UART_FC_NONE;
    
    /* disable interrupts */
    mtk_uart_disable_intrs(uart, UART_IER_ALL_INTS);

    /* allocate irq line */
    //ret = request_irq(port->irq, mtk_uart_irq, 0, DRV_NAME, uart);
    ret = request_irq(port->irq, (irq_handler_t)mtk_uart_irq, IRQF_TRIGGER_LOW, DRV_NAME, uart); /* [ALPS00142658] Fix incompatible pointer type waning */
    if (ret)
        return ret;

#if defined(ENABLE_VFIFO)
#if defined(ENABLE_VFIFO_DEBUG)
    mtk_uart_vfifo_prepare(uart);
    uart->dma_mode      = uart->setting->dma_mode;
    uart->tx_mode       = uart->setting->tx_mode;
    uart->rx_mode       = uart->setting->rx_mode;
    uart->tx_trig       = uart->setting->tx_trig;
    uart->rx_trig       = uart->setting->rx_trig;
#endif
    /* allocate vfifo */
    if (uart->rx_mode == UART_RX_VFIFO_DMA) {
        uart->rx_vfifo = mtk_uart_vfifo_alloc(uart, UART_RX_VFIFO);  
        ret = mtk_uart_dma_alloc(uart, &uart->dma_rx, uart->rx_mode, uart->rx_vfifo);
        if (!uart->rx_vfifo || ret) {
            uart->rx_mode = UART_NON_DMA;
            MSG(ERR, "RX DMA alloc fail [%d]\n", ret);
        }
    }
    if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        uart->tx_vfifo = mtk_uart_vfifo_alloc(uart, UART_TX_VFIFO);
        ret = mtk_uart_dma_alloc(uart, &uart->dma_tx, uart->tx_mode, uart->tx_vfifo);
        if (!uart->tx_vfifo || ret) {
            uart->tx_mode = UART_NON_DMA;
            MSG(ERR, "TX DMA alloc fail [%d]\n", ret);        
        }    
    }
    
    /* start vfifo dma */
    if (uart->tx_mode == UART_TX_VFIFO_DMA) {
        uart->write_allow = mtk_uart_vfifo_write_allow;
        uart->write_byte  = mtk_uart_vfifo_write_byte;
        mtk_uart_vfifo_enable(uart, uart->tx_vfifo);
        mtk_uart_dma_setup(uart, &uart->dma_tx);
        if (mtk_uart_dma_start(uart, &uart->dma_tx))
            MSG(ERR,"mtk_uart_dma_start fails\n");

        hrtimer_init(&uart->tx_vfifo->flush, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
        uart->tx_vfifo->flush.function   = mtk_uart_tx_vfifo_timeout;
    } else if (uart->tx_mode == UART_NON_DMA) {
        uart->write_allow = mtk_uart_write_allow;
        uart->write_byte  = mtk_uart_write_byte;
    }
    if (uart->rx_mode == UART_RX_VFIFO_DMA) {
        uart->read_allow = mtk_uart_vfifo_read_allow;
        uart->read_byte  = mtk_uart_vfifo_read_byte;  
        mtk_uart_vfifo_enable(uart, uart->rx_vfifo);
        mtk_uart_dma_setup(uart, &uart->dma_rx);
        if (mtk_uart_dma_start(uart, &uart->dma_rx))
            MSG(ERR,"mtk_uart_dma_start fails\n");        
    } else if (uart->rx_mode == UART_NON_DMA) {
        uart->read_allow = mtk_uart_read_allow;
        uart->read_byte  = mtk_uart_read_byte;  
    }
#endif
    if (uart->tx_mode == UART_TX_VFIFO_DMA || uart->rx_mode == UART_RX_VFIFO_DMA){
	switch(uart->nport){
	    case 0:
		disable_dpidle_by_bit(PDN_FOR_UART1);
		disable_mcidle_by_bit(PDN_FOR_UART1);
	        break;
	    case 1:
		disable_dpidle_by_bit(PDN_FOR_UART2);
		disable_mcidle_by_bit(PDN_FOR_UART2);
	        break;
	    case 2:
		disable_dpidle_by_bit(PDN_FOR_UART3);
		disable_mcidle_by_bit(PDN_FOR_UART3);
	        break;
	    default:
		break;
	}
    } else if (uart->tx_mode == UART_NON_DMA && uart->rx_mode == UART_NON_DMA){
	switch(uart->nport){
	    case 0:
		enable_dpidle_by_bit(PDN_FOR_UART1);
		enable_mcidle_by_bit(PDN_FOR_UART1);
	        break;
	    case 1:
		enable_dpidle_by_bit(PDN_FOR_UART2);
		enable_mcidle_by_bit(PDN_FOR_UART2);
	        break;
	    case 2:
		enable_dpidle_by_bit(PDN_FOR_UART3);
		enable_mcidle_by_bit(PDN_FOR_UART3);
	        break;
	    default:
		break;
	}
    }

    uart->tx_stop = 0;
    uart->rx_stop = 0;

    /* After applying UART as Level-Triggered IRQ, the function must be called or
     * the interrupt will be incorrect activated. 
     */
    mtk_uart_fifo_set_trig(uart, uart->tx_trig, uart->rx_trig);

    /* enable interrupts */
    mtk_uart_enable_intrs(uart, mask);
    return 0;
}

void stop_log(void)
{
	unsigned long flags;
	unsigned long rx_flags;
	spin_lock_irqsave(&tx_history_lock, flags);
	spin_lock_irqsave(&rx_history_lock, rx_flags);
	stop_update = 1;
	spin_unlock_irqrestore(&rx_history_lock, rx_flags);
	spin_unlock_irqrestore(&tx_history_lock, flags);
}
EXPORT_SYMBOL(stop_log);
void dump_uart_history(void)
{
	int i,j;
	unsigned long flags;
	unsigned long rx_flags;
	int curr, rx_curr;
	spin_lock_irqsave(&tx_history_lock, flags);
	spin_lock_irqsave(&rx_history_lock, rx_flags);
	stop_update = 1;
	spin_unlock_irqrestore(&rx_history_lock, rx_flags);
	spin_unlock_irqrestore(&tx_history_lock, flags);
	curr = curr_record + 1;
	if(curr>=RECORD_NUMBER)
		curr=0;
	rx_curr = curr_rx_record + 1;
	if(rx_curr>=RECORD_NUMBER)
		rx_curr=0;

	for(i=0; i<RECORD_NUMBER; i++){
		printk("\nTX rec%03d:", i);
		for(j=0; j<uart_history_cnt[curr]; j++){
			if((j%0xF)==0)
				printk("\n");
			printk("%02x ", uart_history[curr][j]);
		}
		msleep(20);
		curr++;
		if(curr>=RECORD_NUMBER)
		curr=0;
	}
	for(i=0; i<RECORD_NUMBER; i++){
		printk("\nRX rec%03d:", i);
		for(j=0; j<uart_rx_history_cnt[rx_curr]; j++){
			if((j%0xF)==0)
				printk("\n");
			printk("%02x ", uart_rx_history[rx_curr][j]);
		}
		msleep(20);
		rx_curr++;
		if(rx_curr>=RECORD_NUMBER)
		rx_curr=0;
	}
}
EXPORT_SYMBOL(dump_uart_history);

/*---------------------------------------------------------------------------*/
/* disable the port, disable any break condition that may be in effect, and
 * free any interrupt resources
 */
static void mtk_uart_shutdown(struct uart_port *port)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;

    MSG_FUNC_ENTRY();

    /* disable interrupts */
    mtk_uart_disable_intrs(uart, UART_IER_ALL_INTS);

#if defined(ENABLE_VFIFO)
    /* free dma channels and vfifo */
    mtk_uart_dma_free(uart, &uart->dma_tx);
    mtk_uart_dma_free(uart, &uart->dma_rx);
#endif

    mdelay(1);
    mtk_uart_fifo_flush(uart);

    /* release irq line */
    free_irq(port->irq, port);

    switch(uart->nport){
	case 0:
	    enable_dpidle_by_bit(PDN_FOR_UART1);
	    enable_mcidle_by_bit(PDN_FOR_UART1);
	    break;
	case 1:
	    enable_dpidle_by_bit(PDN_FOR_UART2);
	    enable_mcidle_by_bit(PDN_FOR_UART2);
	    break;
	case 2:
	    enable_dpidle_by_bit(PDN_FOR_UART3);
	    enable_mcidle_by_bit(PDN_FOR_UART3);
	    break;
	default:
    	    break;
    }
    /* the uart port will be powered off in power_mgnt */
}
/*---------------------------------------------------------------------------*/
static void	mtk_uart_flush_buffer(struct uart_port *port)
{
#if defined(ENABLE_DEBUG)    
    struct mtk_uart *uart = (struct mtk_uart *)port;

    MSG_FUNC_ENTRY();
#endif    
    //mtk_uart_fifo_flush(uart);
}
/*---------------------------------------------------------------------------*/
/* 
 * For stability test 
 */
void mtk_uart_update_sysclk(void)
{
	int i;
	unsigned long flags;
	struct mtk_uart *uart;
	struct uart_port *port;
	unsigned int baud;

	for (i = 0; i < UART_NR; i++) {
		uart = &mtk_uarts[i]; 
		port = &uart->port;
		baud = uart->baudrate;
		port->uartclk  		= UART_SYSCLK;  //mt6575_get_bus_freq()*1000/4;
		uart->sysclk        = UART_SYSCLK;  //mt6575_get_bus_freq()*1000/4;
		if(baud == 0) continue;	//The istance is not initialized yet.
		spin_lock_irqsave(&port->lock, flags);
		mtk_uart_set_baud(uart, baud);
		spin_unlock_irqrestore(&port->lock, flags);
	}
}
EXPORT_SYMBOL(mtk_uart_update_sysclk);
/*---------------------------------------------------------------------------*/
/* change the port parameters, including word length, parity, stop bits.
 * update read_status_mask and ignore_status_mask to indicate the types of
 * events we are interrested in receiving
 */
static void mtk_uart_set_termios(struct uart_port *port,
                                   struct ktermios *termios, struct ktermios *old)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;
    unsigned long flags;
    unsigned int baud;
    int datalen, mode;
    int parity = 0;
    int stopbit = 1;

    MSG_FUNC_ENTRY();

    /* datalen : default 8bits */
    switch (termios->c_cflag & CSIZE) {
    case CS5:
        datalen = 5;
        break;
    case CS6:
        datalen = 6;
        break;
    case CS7:
        datalen = 7;
        break;
    case CS8:
    default:
        datalen = 8;
        break;
    }
    
    /* stopbit : default 1 */
    if (termios->c_cflag & CSTOPB)
        stopbit = 2;

    /* parity : default none */
    if (termios->c_cflag & PARENB) {
        if (termios->c_cflag & PARODD)
            parity = 1; /* odd */
        else
            parity = 2; /* even */
    }

    spin_lock_irqsave(&port->lock, flags);
    
    /* read status mask */
    port->read_status_mask = 0;    
    if (termios->c_iflag & INPCK) {
        /* frame error, parity error */
        port->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
    }
    if (termios->c_iflag & (BRKINT | PARMRK)) {
        /* break error */
        port->read_status_mask |= UART_LSR_BI;
    }
    
    port->ignore_status_mask = 0;
    if (termios->c_iflag & IGNPAR) {
        /* ignore parity and framing errors */
        port->ignore_status_mask |= UART_LSR_FE | UART_LSR_PE;
    }
    if (termios->c_iflag & IGNBRK) {
        /* ignore break errors. */
        port->ignore_status_mask |= UART_LSR_BI;
        if (termios->c_iflag & IGNPAR) {
            /* ignore overrun errors */
            port->ignore_status_mask |= UART_LSR_OE;
        }
    }

    /* ignore all characters if CREAD is not set */
    if ((termios->c_cflag & CREAD) == 0) {
        uart->ignore_rx = 1;
    }

    /* update per port timeout */
    baud = uart_get_baud_rate(port, termios, old, 0, uart->sysclk); /*when dividor is 1, baudrate = clock*/    
    uart_update_timeout(port, termios->c_cflag, baud);    
    mtk_uart_config(uart, baud, datalen, stopbit, parity);

    /* setup fifo trigger level */
    mtk_uart_fifo_set_trig(uart, uart->tx_trig, uart->rx_trig);

    /* setup hw flow control: only port 0 ~1 support hw rts/cts */
    MSG(CFG, "c_lflag:%X, c_iflag:%X, c_oflag:%X, c_cflag:%X\n", termios->c_lflag, termios->c_iflag, termios->c_oflag, termios->c_cflag);
    if (HW_FLOW_CTRL_PORT(uart) && (termios->c_cflag & CRTSCTS) && (!(termios->c_iflag&0x80000000))) {
        printk(KERN_NOTICE "Hardware Flow Control\n");
        mode = UART_FC_HW;
    } else if (termios->c_iflag & 0x80000000) {
        printk(KERN_NOTICE "MTK Software Flow Control\n");
        mode = UART_FC_SW;
    } else if (termios->c_iflag & (IXON | IXOFF | IXANY)) {
        printk(KERN_NOTICE "Linux default SW Flow Control\n");
        mode = UART_FC_NONE;    
    } else {
        printk(KERN_NOTICE "No Flow Control\n");
        mode = UART_FC_NONE;
    }
    mtk_uart_set_flow_ctrl(uart, mode);

    /* determine if port should enable modem status interrupt */
    if (UART_ENABLE_MS(port, termios->c_cflag)) 
        uart->ms_enable = 1;
    else
        uart->ms_enable = 0;

    spin_unlock_irqrestore(&port->lock, flags);
}
/*---------------------------------------------------------------------------*/
/* perform any power management related activities on the port */
static void mtk_uart_power_mgnt(struct uart_port *port, unsigned int state,
                                   unsigned int oldstate)
{
    struct mtk_uart *uart = (struct mtk_uart *)port;

    MSG(FUC, "%s(%d->%d)\n", __func__, oldstate, state);

	switch (state) {
	case 0:
        mtk_uart_power_up(uart);
		break;
	case 3:
        mtk_uart_power_down(uart);
		break;
	default:
		MSG(ERR, "unkown pm: %d\n", state);
	}
}
/*---------------------------------------------------------------------------*/
static int mtk_uart_set_wake(struct uart_port *port, unsigned int state)
{
    return 0; /* Not used in current kernel version */
}
/*---------------------------------------------------------------------------*/
/* return a pointer to a string constant describing the port */
static const char *mtk_uart_type(struct uart_port *port)
{
    return "MTK UART";
}
/*---------------------------------------------------------------------------*/
/* release any memory and io region resources currently in used by the port */
static void mtk_uart_release_port(struct uart_port *port)
{
    return;
}
/*---------------------------------------------------------------------------*/
/* request any memory and io region resources required by the port */
static int mtk_uart_request_port(struct uart_port *port)
{
    return 0;
}
/*---------------------------------------------------------------------------*/
/* perform any autoconfiguration steps required by the port.
 * it's expected to claim the resources and map the port.
 */
static void mtk_uart_config_port(struct uart_port *port, int flags)
{ 
    struct mtk_uart* uart = (struct mtk_uart*)port;
    if (flags & UART_CONFIG_TYPE) {
        if (mtk_uart_request_port(port))
            MSG(ERR, "mtk_uart_request_port fail\n");
        port->type = PORT_MTK;
    }
}
/*---------------------------------------------------------------------------*/
/* verify if the new serial information contained within 'ser' is suitable */ 
static int mtk_uart_verify_port(struct uart_port *port,
    struct serial_struct *ser)
{
#if ( defined(ENABLE_DEBUG)||defined(SERIAL_STRUCT_EXT) ) /* [ALPS00142658] Fix unused variable waring */
    struct mtk_uart* uart = (struct mtk_uart*)port;
#endif        
    int ret = 0;
    MSG(FUC, "%s: %8x, %d, %d\n", __func__, ser->flags, ser->custom_divisor, uart->custom_baud);
    if (ser->type != PORT_UNKNOWN && ser->type != PORT_MTK)
        ret = -EINVAL;
    if (ser->irq != port->irq)
        ret = -EINVAL;
    if (ser->baud_base < 110)
        ret = -EINVAL;
#if defined(SERIAL_STRUCT_EXT)        
    /*EXtension: the custom baudrate is stored in reserved field*/
    uart->custom_baud = ser->reserved[0];
#endif    
    return ret;
}
/*---------------------------------------------------------------------------*/
extern int uart_set_info(struct uart_state *state, struct serial_struct __user *newinfo);
/*---------------------------------------------------------------------------*/
/* perform any port specific IOCTLs */
static int mtk_uart_ioctl(struct uart_port *port, unsigned int cmd, 
                             unsigned long arg)
{
#if defined(ENABLE_DEBUG)    
    struct mtk_uart* uart = (struct mtk_uart*)port;        
    MSG(FUC, "IOCTL: %8X\n", cmd);    
#endif    
    return -ENOIOCTLCMD;
}
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_CONSOLE_POLL
/*---------------------------------------------------------------------------*/
static int mtk_uart_get_poll_char(struct uart_port *port)
{   /* don't care vfifo setting */
    struct mtk_uart *uart = (struct mtk_uart *)port;
    
    /* [ALPS00033048] For Linux 2.6.35 kgdb chagne, using while loop may block kgdb,
     * return NO_POLL_CHAR directly if no data to read */
    #if 0
    while (!(uart->read_status(uart) & UART_LSR_DR))
        cpu_relax();
    #else
    if (!(uart->read_status(uart) & UART_LSR_DR))
        return NO_POLL_CHAR;
    #endif
    /* End of [ALPS00033048] */

    return mtk_uart_read_byte(uart);
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_put_poll_char(struct uart_port *port, unsigned char c)
{   /* don't care vfifo setting */
    struct mtk_uart *uart = (struct mtk_uart *)port;
    while (!mtk_uart_write_allow(uart)) 
        barrier();
    mtk_uart_write_byte(uart, c);    
}
/*---------------------------------------------------------------------------*/
#endif 
/*---------------------------------------------------------------------------*/
static struct uart_ops mtk_uart_ops =
{
    .tx_empty       = mtk_uart_tx_empty,
    .set_mctrl      = mtk_uart_set_mctrl,
    .get_mctrl      = mtk_uart_get_mctrl,
    .stop_tx        = mtk_uart_stop_tx,
    .start_tx       = mtk_uart_start_tx,
    .stop_rx        = mtk_uart_stop_rx,
    .send_xchar     = mtk_uart_send_xchar,
    .enable_ms      = mtk_uart_enable_ms,
    .break_ctl      = mtk_uart_break_ctl,
    .startup        = mtk_uart_startup,
    .shutdown       = mtk_uart_shutdown,
    .flush_buffer   = mtk_uart_flush_buffer,
    .set_termios    = mtk_uart_set_termios,
    .pm             = mtk_uart_power_mgnt,
    .set_wake       = mtk_uart_set_wake,
    .type           = mtk_uart_type,
    .release_port   = mtk_uart_release_port,
    .request_port   = mtk_uart_request_port,
    .config_port    = mtk_uart_config_port,
    .verify_port    = mtk_uart_verify_port,
    .ioctl          = mtk_uart_ioctl,
#ifdef CONFIG_CONSOLE_POLL
    .poll_get_char  = mtk_uart_get_poll_char,
    .poll_put_char  = mtk_uart_put_poll_char,
#endif    
};
/*---------------------------------------------------------------------------*/
static struct uart_driver mtk_uart_drv =
{
    .owner          = THIS_MODULE,
    .driver_name    = DRV_NAME,
    .dev_name       = "ttyMT",
    .major          = UART_MAJOR,
    .minor          = UART_MINOR,
    .nr             = UART_NR,
#if defined(CONFIG_MTK_SERIAL_CONSOLE) && !defined(CONFIG_MTK_SERIAL_MODEM_TEST)
    .cons           = &mtk_uart_console,
#endif
};
/*---------------------------------------------------------------------------*/
static int mtk_uart_probe(struct platform_device *pdev)
{
    struct mtk_uart     *uart = &mtk_uarts[pdev->id];
    int err;

    MSG_FUNC_ENTRY();

    uart->port.dev = &pdev->dev;
    err = uart_add_one_port(&mtk_uart_drv, &uart->port);
    if (!err)
        platform_set_drvdata(pdev, uart);
    
#if defined(ENABLE_VFIFO)
    if ((err = mtk_uart_vfifo_create(uart))) {
        mtk_uart_vfifo_delete(uart);
        DEV_ERR("create vff buffer fail:%d\n", err);
    }
#endif
    return err;
}
/*---------------------------------------------------------------------------*/
static int mtk_uart_remove(struct platform_device *pdev)
{
    struct mtk_uart *uart = platform_get_drvdata(pdev);
    int err;

    platform_set_drvdata(pdev, NULL);

    if (!uart)
        return -EINVAL;
    
    err = uart_remove_one_port(&mtk_uart_drv, &uart->port);
    
#if defined(ENABLE_VFIFO)
    mtk_uart_vfifo_delete(uart);
#endif
    return err;
}
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM 
/*---------------------------------------------------------------------------*/
void mtk_uart_save(struct mtk_uart *uart)
{
    u32 base;
    base = uart->base;

    uart->registers.lcr = UART_READ32(UART_LCR);

    reg_sync_writel(0xbf, UART_LCR);
    uart->registers.efr = UART_READ32(UART_EFR);
    reg_sync_writel(uart->registers.lcr, UART_LCR);
    uart->registers.fcr = UART_READ32(UART_FCR_RD);

    //baudrate
    uart->registers.highspeed = UART_READ32(UART_HIGHSPEED);
    uart->registers.fracdiv_l = UART_READ32(UART_FRACDIV_L);
    uart->registers.fracdiv_m = UART_READ32(UART_FRACDIV_M);
    reg_sync_writel(uart->registers.lcr | UART_LCR_DLAB, UART_LCR);
    uart->registers.dll = UART_READ32(UART_DLL);
    uart->registers.dlh = UART_READ32(UART_DLH);
    reg_sync_writel(uart->registers.lcr, UART_LCR);
    uart->registers.sample_count = UART_READ32(UART_SAMPLE_COUNT);
    uart->registers.sample_point = UART_READ32(UART_SAMPLE_POINT);
    uart->registers.guard = UART_READ32(UART_GUARD);
	    
    //flow control
    uart->registers.escape_en = UART_READ32(UART_ESCAPE_EN);
    uart->registers.mcr = UART_READ32(UART_MCR);
    uart->registers.ier = UART_READ32(UART_IER); 

    uart->registers.sleep_en = UART_READ32(UART_SLEEP_EN); 
}

void mtk_uart_restore(void)
{
    u32 base;
    struct mtk_uart *uart;

    uart = console_port;
    base = uart->base;

    mtk_uart_power_up(uart);
    reg_sync_writel(0xbf, UART_LCR);
    reg_sync_writel(uart->registers.efr, UART_EFR);
    reg_sync_writel(uart->registers.lcr, UART_LCR);
    reg_sync_writel(uart->registers.fcr, UART_FCR);

    //baudrate
    reg_sync_writel(uart->registers.highspeed, UART_HIGHSPEED);
    reg_sync_writel(uart->registers.fracdiv_l, UART_FRACDIV_L);
    reg_sync_writel(uart->registers.fracdiv_m, UART_FRACDIV_M);
    reg_sync_writel(uart->registers.lcr | UART_LCR_DLAB, UART_LCR);
    reg_sync_writel(uart->registers.dll, UART_DLL);
    reg_sync_writel(uart->registers.dlh, UART_DLH);
    reg_sync_writel(uart->registers.lcr, UART_LCR);
    reg_sync_writel(uart->registers.sample_count, UART_SAMPLE_COUNT);
    reg_sync_writel(uart->registers.sample_point, UART_SAMPLE_POINT);
    reg_sync_writel(uart->registers.guard, UART_GUARD);
    
    //flow control
    reg_sync_writel(uart->registers.escape_en, UART_ESCAPE_EN);
    reg_sync_writel(uart->registers.mcr, UART_MCR);
    reg_sync_writel(uart->registers.ier, UART_IER); 
}

static int mtk_uart_bt_suspend(void)
{
    int ret = 0;
    unsigned long flags;
    if (bt_port)
    {
	struct mtk_uart *uart = bt_port;
	spin_lock_irqsave(&mtk_uart_bt_lock, flags);
        ret = uart_suspend_port(&mtk_uart_drv, &uart->port);
	mt_set_gpio_out(GPIO220, GPIO_OUT_ONE);
	mt_set_gpio_mode(GPIO220, GPIO_MODE_00);
	spin_unlock_irqrestore(&mtk_uart_bt_lock, flags);
	printk(KERN_NOTICE "[UART%d] Suspend(%d)!\n", uart->nport, ret);
    }
    return ret;
}
/*---------------------------------------------------------------------------*/
static void mtk_uart_bt_resume(void)
{
    int ret = 0;
    unsigned long flags;
    if (bt_port)
    {
	struct mtk_uart *uart = bt_port;
	spin_lock_irqsave(&mtk_uart_bt_lock, flags);
	mt_set_gpio_mode(GPIO220, GPIO_MODE_01);
	ret = uart_resume_port(&mtk_uart_drv, &uart->port);
	spin_unlock_irqrestore(&mtk_uart_bt_lock, flags);
	disable_irq(uart->port.irq);
	printk(KERN_NOTICE "[UART%d] Resume(%d)!\n", uart->nport, ret);
    }
}

static int mtk_uart_suspend(struct platform_device *pdev, pm_message_t state)
{
    int ret = 0;
    struct mtk_uart *uart = platform_get_drvdata(pdev);
    if (console_suspend_enabled == 0 && uart == console_port && uart->poweron_count > 0)     //For console_suspend_enabled=0
    	mtk_uart_save(uart);
#ifndef PM_SUSPEND_RESUME_CONFIG_EN
    /*temporary solution for BT: no suspend/resume in UART3*/
    if (uart && (uart->nport != 2)) {
        ret = uart_suspend_port(&mtk_uart_drv, &uart->port);
        printk(KERN_NOTICE "[UART%d] Suspend(%d)!\n", uart->nport, ret);
    }
#else
    if(uart && (uart->nport < UART_NR) && (mtk_uart_suspend_resume_en[uart->nport]!=0 ) ){
        ret = uart_suspend_port(&mtk_uart_drv, &uart->port);
        printk(KERN_NOTICE "[UART%d] Suspend(%d)!\n", uart->nport, ret);
    }
#endif //PM_SUSPEND_RESUME_CONFIG_EN
    return ret;
}
/*---------------------------------------------------------------------------*/
static int mtk_uart_resume(struct platform_device *pdev)
{
    int ret = 0;
    struct mtk_uart *uart = platform_get_drvdata(pdev);

#ifndef PM_SUSPEND_RESUME_CONFIG_EN
    /*temporary solution for BT: no suspend/resume in UART3*/
    if (uart && (uart->nport != 2)) {
        ret = uart_resume_port(&mtk_uart_drv, &uart->port);
        printk(KERN_NOTICE "[UART%d] Resume(%d)!\n", uart->nport, ret);
    }
#else
    if (uart && (uart->nport < UART_NR) && (mtk_uart_suspend_resume_en[uart->nport]!=0 ) ) {
        ret = uart_resume_port(&mtk_uart_drv, &uart->port);
        printk(KERN_NOTICE "[UART%d] Resume(%d)!\n", uart->nport, ret);
    }
#endif //PM_SUSPEND_RESUME_CONFIG_EN
    return ret;
}
int mtk_uart_pm_suspend(struct device *device)
{
    struct platform_device *pdev;
    pr_debug("calling %s()\n", __func__);

    pdev = to_platform_device(device);
    BUG_ON(pdev == NULL);

    return mtk_uart_suspend(pdev, PMSG_SUSPEND);
}

int mtk_uart_pm_resume(struct device *device)
{
    struct platform_device *pdev;
    pr_debug("calling %s()\n", __func__);

    pdev = to_platform_device(device);
    BUG_ON(pdev == NULL);

    return mtk_uart_resume(pdev);
}

int mtk_uart_pm_restore_noirq(struct device *device)
{
    unsigned int gic_pending, fifo_int_stat;
    struct mtk_uart *uart;
    //pr_warn("calling %s()\n", __func__);
    
    uart = dev_get_drvdata(device);
    if (!uart || !uart->setting) {
        pr_warn("[%s] uart (%p) or uart->setting (%p) is null!!\n", __func__, uart, uart->setting);
        return 0;
    }
    mtk_uart_fifo_set_trig(uart, uart->tx_trig, uart->rx_trig);
    if (uart->setting->irq_sen == MT65xx_EDGE_SENSITIVE) {
        irq_set_irq_type(uart->setting->irq_num, IRQF_TRIGGER_FALLING);
    } else {
        irq_set_irq_type(uart->setting->irq_num, IRQF_TRIGGER_LOW);
    }
    
#define GIC_DIST_PENDING_SET 0x200
    if (uart->tx_vfifo && uart->tx_mode == UART_TX_VFIFO_DMA) {
        irq_set_irq_type(uart->tx_vfifo->irq_id, IRQF_TRIGGER_LOW);
        fifo_int_stat = UART_READ32(VFF_INT_EN(uart->tx_vfifo->base));
        gic_pending = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_PENDING_SET + uart->tx_vfifo->irq_id/32*4);
        pr_warn("[%s] tx_vfifo(%p) int(0x%08x) gic_pending_mask(0x%08x)\n",
                __func__, uart->tx_vfifo->base, fifo_int_stat, gic_pending);
    }
    if (uart->rx_vfifo && uart->rx_mode == UART_RX_VFIFO_DMA) {
        irq_set_irq_type(uart->rx_vfifo->irq_id, IRQF_TRIGGER_LOW);
        fifo_int_stat = UART_READ32(VFF_INT_EN(uart->rx_vfifo->base));
        gic_pending = DRV_Reg32(GIC_DIST_BASE + GIC_DIST_PENDING_SET + uart->rx_vfifo->irq_id/32*4);
        pr_warn("[%s] rx_vfifo(%p) int(0x%08x) gic_pending_mask(0x%08x)\n",
                __func__, uart->rx_vfifo->base, fifo_int_stat, gic_pending);
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
#else /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
#define mtk_uart_pm_suspend    NULL
#define mtk_uart_pm_resume NULL
#define mtk_uart_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
struct dev_pm_ops mtk_uart_pm_ops = {
    .suspend = mtk_uart_pm_suspend,
    .resume = mtk_uart_pm_resume,
    //.freeze = mtk_uart_pm_suspend,
    //.thaw = mtk_uart_pm_resume,
    .poweroff = mtk_uart_pm_suspend,
    .restore = mtk_uart_pm_resume,
    .restore_noirq = mtk_uart_pm_restore_noirq,
};


/*---------------------------------------------------------------------------*/
static int mtk_uart_init_ports(void)
{
    int i;
    struct mtk_uart *uart;
    unsigned long base;
#if defined(CONFIG_MTK_SERIAL_MODEM_TEST)
    //#define HW_MISC     (CONFIG_BASE+0x0020)    // mtk does NOT has this register
    //unsigned char mask[UART_NR] = { 1 << 3, 1 << 4, 1 << 5, 1 << 6};           
    unsigned char modem_uart[UART_NR] = {1, 0, 0, 1};
#endif 
    spin_lock_init(&mtk_console_lock);

    for (i = 0; i < UART_NR; i++) {
        uart = &mtk_uarts[i];        
        uart->setting = &mtk_uart_default_settings[i];
        base = uart->setting->uart_base;        
        uart->port.iotype   = UPIO_MEM;
        uart->port.mapbase  = IO_VIRT_TO_PHYS(base);   /* for ioremap */
        uart->port.membase  = (unsigned char __iomem *)base;
        uart->port.irq      = uart->setting->irq_num;
        uart->port.fifosize = UART_FIFO_SIZE;
        uart->port.ops      = &mtk_uart_ops;
        uart->port.flags    = UPF_BOOT_AUTOCONF;
        uart->port.line     = i;
        uart->port.uartclk  = UART_SYSCLK;
        //pll_for_uart = mt6575_get_bus_freq();
        //uart->port.uartclk  = mt6575_get_bus_freq()*1000/4;
        spin_lock_init(&uart->port.lock);
        uart->base          = base;
        uart->auto_baud     = CFG_UART_AUTOBAUD;
        uart->nport         = i;
        uart->sysclk        = UART_SYSCLK; /* FIXME */
        //uart->sysclk        = mt6575_get_bus_freq()*1000/4;
        uart->dma_mode      = uart->setting->dma_mode;
        uart->tx_mode       = uart->setting->tx_mode;
        uart->rx_mode       = uart->setting->rx_mode;
        uart->tx_trig       = uart->setting->tx_trig;
        uart->rx_trig       = uart->setting->rx_trig;
        uart->write_allow   = mtk_uart_write_allow;
        uart->read_allow    = mtk_uart_read_allow;
        uart->write_byte    = mtk_uart_write_byte;
        uart->read_byte     = mtk_uart_read_byte;
        uart->read_status   = mtk_uart_read_status;
        uart->poweron_count = 0;
        uart->timeout_count = 0;
        uart->baudrate      = 0;
        uart->custom_baud   = 0;
 	uart->registers.dll = 1;
	uart->registers.dlh = 0;
	uart->registers.ier = 0;
	uart->registers.lcr = 0;
	uart->registers.mcr = 0;
	uart->registers.fcr = 0;
	uart->registers.lsr = 0x60;
	uart->registers.efr = 0;
	uart->registers.highspeed = 0;
	uart->registers.sample_count = 0;
	uart->registers.sample_point = 0xff;
	uart->registers.fracdiv_l = 0;
	uart->registers.fracdiv_m = 0;
	uart->registers.escape_en = 0;
	uart->registers.guard = 0;
	uart->registers.sleep_en = 0;
#ifdef ENABLE_DEBUG
        uart->debug         = &mtk_uart_debug[i];
#endif
#if defined(CONFIG_MTK_SERIAL_MODEM_TEST)
        if (modem_uart[i]) {            
            //u32 dat = UART_READ32(HW_MISC); // mtk does NOT has this register
            mtk_uart_power_up(uart); //power up
            //reg_sync_writel(dat | mask[i], HW_MISC);
            continue;
        }
#else        
        //mtk_uart_power_up(uart);
        mtk_uart_disable_intrs(uart, UART_IER_ALL_INTS);

        if (uart->setting->irq_sen == MT65xx_EDGE_SENSITIVE) {
            irq_set_irq_type(uart->setting->irq_num, IRQF_TRIGGER_FALLING);
        } else {
            irq_set_irq_type(uart->setting->irq_num, IRQF_TRIGGER_LOW);
        }

        mtk_uart_fifo_init(uart);
        mtk_uart_set_mode(uart, uart->dma_mode);
        //mtk_uart_power_down(uart);
 	reg_sync_writel(0x1, UART_SLEEP_EN);
    	printk("SLEEP_EN = 0x%x\n",UART_READ32(UART_SLEEP_EN));
#endif        
    }
#if defined(CONFIG_MTK_SERIAL_MODEM_TEST)
    /*NOTICE: for enabling modem test, UART4 needs to be disabled. Howerver, if CONFIG_MTK_SERIAL_CONSOLE
              is defined, resume will fail. Since the root cause is not clear, only disable the console-related
              function.*/
    //printk("HW_MISC: 0x%08X\n", UART_READ32(HW_MISC)); // mtk does NOT has this register
#endif 
    return 0;
}
/*---------------------------------------------------------------------------*/
static struct platform_driver mtk_uart_dev_drv =
{
    .probe   = mtk_uart_probe,
    .remove  = mtk_uart_remove,
#ifdef CONFIG_PM    
    .suspend = mtk_uart_suspend,
    .resume  = mtk_uart_resume,
#endif    
    .driver = {
        .name    = DRV_NAME,
        .owner   = THIS_MODULE,   
#ifdef CONFIG_PM
        .pm      = &mtk_uart_pm_ops,
#endif
 
    }
};

#ifdef CONFIG_PM   
static struct syscore_ops mtk_uart_syscore_ops = {
	.suspend	= mtk_uart_bt_suspend,
	.resume		= mtk_uart_bt_resume,
};

static int __init mtk_uart_init_ops(void)
{
    register_syscore_ops(&mtk_uart_syscore_ops);
    return 0;
}
#endif 
/*---------------------------------------------------------------------------*/
static int __init mtk_uart_init(void)
{
    int ret = 0;

#ifndef CONFIG_MTK_SERIAL_CONSOLE
    mtk_uart_init_ports();
#endif

#if defined(ENABLE_SYSFS)
    mtk_uart_sysfs();
#endif 

    ret = uart_register_driver(&mtk_uart_drv);

    if (ret) return ret;
    
    ret = platform_driver_register(&mtk_uart_dev_drv);

    if (ret) {
        uart_unregister_driver(&mtk_uart_drv);
        return ret;
    }
#ifdef CONFIG_PM
    mtk_uart_init_ops();
#endif
    spin_lock_init(&tx_history_lock);
    spin_lock_init(&rx_history_lock);
    spin_lock_init(&mtk_uart_bt_lock);
    return ret;
}
/*---------------------------------------------------------------------------*/
static void __exit mtk_uart_exit(void)
{
    platform_driver_unregister(&mtk_uart_dev_drv);
    uart_unregister_driver(&mtk_uart_drv);
}
/*---------------------------------------------------------------------------*/
module_init(mtk_uart_init);
module_exit(mtk_uart_exit);
/*---------------------------------------------------------------------------*/
MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("MTK Serial Port Driver $Revision$");
MODULE_LICENSE("GPL");
