#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <asm/thread_info.h>
#include <asm/fiq.h>
#include <asm/fiq_glue.h>
#include <asm/fiq_debugger.h>
#include <mach/irqs.h>
#include <mach/mt_reg_base.h>
#include "mtk_uart.h"

struct fiq_dbg_event
{
    u32 iir;
    int data;
};

#define THREAD_INFO(sp) ((struct thread_info *) \
                ((unsigned long)(sp) & ~(THREAD_SIZE - 1)))
#define REG_UART_BASE   *((volatile unsigned int*)(console_base_addr + 0x00))
#define REG_UART_STATUS *((volatile unsigned int*)(console_base_addr + 0x14))
#define REG_UART_IIR 	*((volatile unsigned int*)(console_base_addr + 0x08))

#define FIQ_DEBUGGER_BREAK_CH 6 /* CTRL + F */
#define MAX_FIQ_DBG_EVENT 1024
static struct fiq_dbg_event fiq_dbg_events[MAX_FIQ_DBG_EVENT];
static int fiq_dbg_event_rd, fiq_dbg_event_wr;
static unsigned int fiq_dbg_event_ov;
static int console_base_addr = UART2_BASE;
static int uart_irq_number = -1;
static int ret_FIQ_DEBUGGER_BREAK;

extern struct mtk_uart *mt_console_uart;
extern void mtk_uart_tx_handler(struct mtk_uart *uart);
extern void mtk_uart_get_modem_status(struct mtk_uart *uart);
extern bool debug_handle_uart_interrupt(void *state, int this_cpu, void *regs, void *svc_sp);
extern int is_fiq_debug_console_enable(void *argv);
extern void debug_handle_irq_context(void *arg);

int fiq_dbg_irq(void)
{
    return uart_irq_number;
}

static int fiq_uart_getc(struct platform_device *pdev)
{
    int ch;

    if (ret_FIQ_DEBUGGER_BREAK) {
        ret_FIQ_DEBUGGER_BREAK = 0;
        return FIQ_DEBUGGER_BREAK;
    }

    if (!(REG_UART_STATUS & 0x01))
        return FIQ_DEBUGGER_NO_CHAR;

    ch = REG_UART_BASE & 0xFF;

    if (ch == FIQ_DEBUGGER_BREAK_CH)
        return FIQ_DEBUGGER_BREAK;

    return ch;
}

static void fiq_uart_putc(struct platform_device *pdev, unsigned int c)
{
    #define UART_RETRY (5000)
    u32 cnt = 0;
    while (! (REG_UART_STATUS & 0x20)){
	if (cnt++ >= UART_RETRY)
            return;
    }

    REG_UART_BASE = c & 0xFF;
}

static void fiq_enable(struct platform_device *pdev, unsigned int fiq, bool enable)
{
    if (enable)
        enable_fiq(fiq);
    else
        disable_fiq(fiq);
}

static void fiq_dbg_force_irq(struct platform_device *pdev, unsigned int irq)
{
    trigger_sw_irq(irq);
}

struct fiq_debugger_pdata fiq_serial_data =
{
    .uart_getc = &fiq_uart_getc,
    .uart_putc = &fiq_uart_putc,
    .fiq_enable = &fiq_enable,
    .fiq_ack = 0,
    .force_irq = &fiq_dbg_force_irq,
    .force_irq_ack = 0,
};

static struct resource fiq_resource[] = 
{
    [0] = {
        .start = FIQ_DBG_SGI,
        .end = FIQ_DBG_SGI,
        .flags = IORESOURCE_IRQ,
        .name = "signal",
    },
    [1] = {
        .start = MT_UART4_IRQ_ID,
        .end = MT_UART4_IRQ_ID,
        .flags = IORESOURCE_IRQ,
        .name = "fiq",
    },
};

struct platform_device mt_fiq_debugger = 
{
    .name   = "fiq_debugger",
    .id     = -1,
    .dev    =
    {
        .platform_data = &fiq_serial_data,
    },
    .num_resources = ARRAY_SIZE(fiq_resource),
    .resource = fiq_resource,
};

void fiq_uart_fixup(int uart_port)
{
    switch (uart_port) {
    case 0:
        console_base_addr = UART1_BASE;
        fiq_resource[1].start = MT_UART1_IRQ_ID;
        fiq_resource[1].end = MT_UART1_IRQ_ID;
        uart_irq_number = MT_UART1_IRQ_ID;
        break;
    case 1:
        console_base_addr = UART2_BASE;
        fiq_resource[1].start = MT_UART2_IRQ_ID;
        fiq_resource[1].end = MT_UART2_IRQ_ID;
        uart_irq_number = MT_UART2_IRQ_ID;
        break;
    case 2:
        console_base_addr = UART3_BASE;
        fiq_resource[1].start = MT_UART3_IRQ_ID;
        fiq_resource[1].end = MT_UART3_IRQ_ID;
        uart_irq_number = MT_UART3_IRQ_ID;
        break;
    case 3:
        console_base_addr = UART4_BASE;
        fiq_resource[1].start = MT_UART4_IRQ_ID;
        fiq_resource[1].end = MT_UART4_IRQ_ID;
        uart_irq_number = MT_UART4_IRQ_ID;
        break;
    default:
        break;
    }
}

static void __push_event(u32 iir, int data)
{
    if (((fiq_dbg_event_wr + 1) % MAX_FIQ_DBG_EVENT) == fiq_dbg_event_rd) {
        /* full */
        fiq_dbg_event_ov++;
    } else {
        fiq_dbg_events[fiq_dbg_event_wr].iir = iir;
        fiq_dbg_events[fiq_dbg_event_wr].data = data;
        fiq_dbg_event_wr++;
        fiq_dbg_event_wr %= MAX_FIQ_DBG_EVENT;
    }
}

static int __pop_event(u32 *iir, int *data)
{
    if (fiq_dbg_event_rd == fiq_dbg_event_wr) {
        /* empty */
        return -1;
    } else {
        *iir = fiq_dbg_events[fiq_dbg_event_rd].iir;
        *data = fiq_dbg_events[fiq_dbg_event_rd].data;
        fiq_dbg_event_rd++;
        fiq_dbg_event_rd %= MAX_FIQ_DBG_EVENT;
        return 0;
    }
}

void mt_debug_fiq(void *arg, void *regs, void *svc_sp)
{
    u32 iir;
    int data = -1;
    int max_count = UART_FIFO_SIZE;
    unsigned int this_cpu;
    int need_irq = 1;

    iir = REG_UART_IIR;
    iir &= UART_IIR_INT_MASK;
    if (iir == UART_IIR_NO_INT_PENDING)
        return ;
    if (iir == UART_IIR_THRE) {
    }
    __push_event(iir, data);

    while (max_count-- > 0) {
        if (!(REG_UART_STATUS & 0x01)) {
            break;
        }

        if (is_fiq_debug_console_enable(arg)) {
            data = mt_console_uart->read_byte(mt_console_uart);
            if (data == FIQ_DEBUGGER_BREAK_CH) {
                /* enter FIQ debugger mode */
                ret_FIQ_DEBUGGER_BREAK = 1;
                this_cpu = THREAD_INFO(svc_sp)->cpu;
                debug_handle_uart_interrupt(arg, this_cpu, regs, svc_sp);
                return ;
            }
            __push_event(UART_IIR_NO_INT_PENDING, data);
            /*why need_irq?*/
            need_irq = 1;
        } else {
            this_cpu = THREAD_INFO(svc_sp)->cpu;
            need_irq = debug_handle_uart_interrupt(arg, this_cpu, regs, svc_sp);
        }
    }

    if (need_irq) {
        mt_disable_fiq(fiq_dbg_irq());
        trigger_sw_irq(FIQ_DBG_SGI);
    }
}

irqreturn_t mt_debug_signal_irq(int irq, void *dev_id)
{
    struct tty_struct *tty = mt_console_uart->port.state->port.tty;
    u32 iir;
    int data;

    while (__pop_event(&iir, &data) >= 0) {
        if (iir == UART_IIR_MS) {
            mtk_uart_get_modem_status(mt_console_uart);
        } else if (iir == UART_IIR_THRE) {
            mtk_uart_tx_handler(mt_console_uart);
        }
        if (data != -1) {
            if (!tty_insert_flip_char(tty, data, TTY_NORMAL)) {
            }
        }
    }
    tty_flip_buffer_push(tty);

    /* handle commands which can only be handled in the IRQ context */
    debug_handle_irq_context(dev_id);

    mt_enable_fiq(fiq_dbg_irq());

    return IRQ_HANDLED;
}

int mt_fiq_init(void *arg)
{
    return request_fiq(fiq_dbg_irq(), (fiq_isr_handler)mt_debug_fiq, IRQF_TRIGGER_LOW, arg);
}

void mt_fiq_printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    unsigned c;
    char *s;
                    
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    s = (char *)buf;
    while ((c = *s++)) {
#if 0
        if (c == '\n') {
            fiq_uart_putc(NULL, '\r');
        }
#endif
        fiq_uart_putc(NULL, c);
    }
}
