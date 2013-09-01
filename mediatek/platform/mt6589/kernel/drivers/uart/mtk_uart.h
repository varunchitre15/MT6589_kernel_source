#ifndef __MTK_UART_H__
#define __MTK_UART_H__

#include <mach/hardware.h>
#include <mach/mt_reg_base.h>
/******************************************************************************
 * Function Configuration
******************************************************************************/
#define ENABLE_DEBUG
#define ENABLE_VFIFO
//  Fix-me: marked for early porting
#define POWER_FEATURE   /*control power-on/power-off*/
#define ENABLE_FRACTIONAL
#define ENABLE_SYSFS      
//#define UART_USING_FIX_CLK_ENABLE
#define UART_FCR_USING_SW_BACK_UP
//#define ATE_FACTORY_ENABLE
#define PM_SUSPEND_RESUME_CONFIG_EN
/*---------------------------------------------------------------------------*/
#if defined(ENABLE_VFIFO) && defined(ENABLE_DEBUG)
#define ENABLE_VFIFO_DEBUG
#endif
/******************************************************************************
 * MACRO & CONSTANT
******************************************************************************/
#define DBG_TAG                     "[UART] "
#define CFG_UART_AUTOBAUD           0
/*---------------------------------------------------------------------------*/
#define UART_VFIFO_SIZE             8192 
#define UART_VFIFO_ALERT_LEN        0x3f
/*---------------------------------------------------------------------------*/
#define UART_MAX_TX_PENDING         1024
/*---------------------------------------------------------------------------*/
#define UART_MAJOR                  204
#define UART_MINOR                  209
#if defined(CONFIG_MT6589_FPGA_CA7)
#define UART_NR                     2
#else
#define UART_NR                     4
#endif
/*---------------------------------------------------------------------------*/
#define MTK_SYSCLK_65            65000000
#define MTK_SYSCLK_49_4        49400000
#define MTK_SYSCLK_58_5          58500000
#define MTK_SYSCLK_52            52000000
#define MTK_SYSCLK_26            26000000
#define MTK_SYSCLK_13            13000000
#define MTK_SYSCLK_6144          61440000
#define MTK_SYSCLK_3072          30720000
#define MTK_SYSCLK_1536          15360000
/*---------------------------------------------------------------------------*/
/*FIXME: MT6589 FPGA porting*/
#if defined(CONFIG_MT6589_FPGA_CA7)
#define UART_SYSCLK                 12000000
#else
#define UART_SYSCLK                 MTK_SYSCLK_26
#endif

/*---------------------------------------------------------------------------*/
#define ISWEXT          (0x80000000)                /*extended bit in c_iflag*/
/*---------------------------------------------------------------------------*/
/*the size definition of VFF*/
#define C_UART1_VFF_TX_SIZE (1024)  /*the size must be 8-byte alignment*/
#define C_UART1_VFF_RX_SIZE (1024)  /*the size must be 8-byte alignment*/
#define C_UART2_VFF_TX_SIZE (8192)  /*the size must be 8-byte alignment*/
#define C_UART2_VFF_RX_SIZE (8192)  /*the size must be 8-byte alignment*/
#define C_UART3_VFF_TX_SIZE (8192)  /*the size must be 8-byte alignment*/
#define C_UART3_VFF_RX_SIZE (8192)  /*the size must be 8-byte alignment*/
#define C_UART4_VFF_TX_SIZE (1024)  /*the size must be 8-byte alignment*/
#define C_UART4_VFF_RX_SIZE (1024)  /*the size must be 8-byte alignment*/
/******************************************************************************
 * LOG SETTING
******************************************************************************/
/* Debug message event */
#define DBG_EVT_NONE        0x00000000    /* No event */
#define DBG_EVT_DMA         0x00000001    /* DMA related event */
#define DBG_EVT_INT         0x00000002    /* UART INT event */
#define DBG_EVT_CFG         0x00000004    /* UART CFG event */
#define DBG_EVT_FUC         0x00000008    /* Function event */
#define DBG_EVT_INFO        0x00000010    /* information event */
#define DBG_EVT_ERR         0x00000020    /* Error event */
#define DBG_EVT_DAT         0x00000040    /* data dump to uart*/
#define DBG_EVT_BUF         0x00000080    /* data dump to buffer */
#define DBG_EVT_MSC         0x00000100    /* misc log*/
#define DBG_EVT_ALL         0xffffffff
/*---------------------------------------------------------------------------*/
#ifdef ENABLE_DEBUG
/*---------------------------------------------------------------------------*/
#define MSG(evt, fmt, args...) \
do {    \
    if ((DBG_EVT_##evt) & mtk_uart_evt_mask[uart->nport]) { \
        const char *s = #evt;                                  \
        if (DBG_EVT_##evt & DBG_EVT_ERR) \
            printk("  [UART%d]:%c:%4d: " fmt , \
                   uart->nport, s[0], __LINE__, ##args); \
        else \
            printk("  [UART%d]:%c: " fmt , uart->nport, s[0], ##args); \
    } \
} while(0)
/*---------------------------------------------------------------------------*/
#define UART_DEBUG_EVT(evt)    ((evt) & mtk_uart_evt_mask[uart->nport])
/*---------------------------------------------------------------------------*/
#define MSG_FUNC_ENTRY(f)       MSG(FUC, "%s\n", __FUNCTION__)
#define MSG_RAW                 printk
/*---------------------------------------------------------------------------*/
#else   /*release mode: only enable error log*/   
#define MSG(evt, fmt, args...)  MSG##evt(fmt, ##args)
#define MSGERR(fmt, args...)    printk("  [UART%d]:E:%4d: " fmt, uart->nport, __LINE__, ##args)
#define MSGDMA(fmt, args...)
#define MSGCFG(fmt, args...)
#define MSGFUC(fmt, args...)
#define MSGINFO(fmt, args...)
#define MSGDAT(fmt, args...)
#define MSGMSC(fmt, args...)
#define MSG_RAW(fmt, args...)  
#define MSG_FUNC_ENTRY(f)       do{}while(0)
#endif  /**/
#define MSG_ERR(fmt, args...)   printk("[UARTX]:E:%4d: " fmt, __LINE__, ##args)
#define MSG_TRC(fmt, args...)   printk("[UARTX]:T: " fmt, ##args)
#define DEV_TRC(fmt, args...)   printk("[UART%d]:T: " fmt, uart->nport, ##args)
#define DEV_ERR(fmt, args...)   printk("[UART%d]:E: " fmt, uart->nport, ##args)
/*---------------------------------------------------------------------------*/
#define DRV_NAME                "mtk-uart"
/*---------------------------------------------------------------------------*/
/******************************************************************************
 * ENUM & STRUCT
******************************************************************************/
/* uart port ids */
enum {
    UART_PORT0 = 0,
    UART_PORT1,
    UART_PORT2,
    UART_PORT3,
    UART_PORT_NUM,
};
/*---------------------------------------------------------------------------*/
typedef enum {
    UART_NON_DMA,
    UART_TX_DMA,    
    UART_TX_VFIFO_DMA,
    UART_RX_VFIFO_DMA,
} UART_DMA_TYPE;
/*---------------------------------------------------------------------------*/
typedef enum {
    UART_TX_VFIFO,
    UART_RX_VFIFO,
    
    UART_VFIFO_NUM
} UART_VFF_TYPE;
/*---------------------------------------------------------------------------*/
/* uart dma mode */
enum {
    UART_DMA_MODE_0,
    UART_DMA_MODE_1,
};
/*---------------------------------------------------------------------------*/
/* flow control mode */
enum {
    UART_FC_NONE,       /*NO flow control*/
    UART_FC_SW,         /*MTK SW Flow Control, differs from Linux Flow Control*/
    UART_FC_HW,         /*HW Flow Control*/
};
/*---------------------------------------------------------------------------*/
struct mtk_uart_setting {
    u8  tx_mode;
    u8  rx_mode;
    u8  dma_mode;    
    u8  sysrq;

    int tx_trig;
    int rx_trig;
    u32 uart_base;
    
    u8  irq_num;
    u8  irq_sen;
    u8  set_bit;        /*APMCU_CG_SET0*/
    u8  clr_bit;        /*APMCU_CG_CLR0*/
    int pll_id;
    
    u8  hw_flow;        /*support hardware flow control or not?!*/
    u8  vff;            /*support vfifo or not!?*/
    u16 _align;
};
/*---------------------------------------------------------------------------*/
#define C_UART_DEBUG_BUF_NUM (5)
/*---------------------------------------------------------------------------*/
struct mtk_uart_buf
{
    unsigned char *dat;
    unsigned int   idx;
    unsigned int   len;
};
/*---------------------------------------------------------------------------*/
struct mtk_uart_vfifo
{
    struct mtk_uart_dma      *dma;    /* vfifo dma owner */    

    /*configuration*/
    u16                         ch;
    u16                         size;
    u16                         trig;
    u16                         type;   /*UART_RX_VFIFO / UART_TX_VFIFO*/
    void                       *base;
    void                       *port;   /*only tx*/   
    void                       *addr;

    atomic_t                    reg_cb;
    atomic_t                    entry;  /* entry count */
    spinlock_t                  iolock;
    struct timer_list           timer;  /* vfifo timer */
    struct hrtimer              flush;  
    dma_addr_t                  dmahd;  /* dma handle */

    struct tasklet_struct       flush_tasklet;
    
    struct mtk_uart_buf      dbg[C_UART_DEBUG_BUF_NUM];
    struct mtk_uart_buf      *cur;
    int                         dbgidx;
    unsigned int        irq_id;
};
/*---------------------------------------------------------------------------*/
struct mtk_uart_dma {
    struct mtk_uart       *uart;     /* dma uart */
    atomic_t                  free;     /* dma channel free */
    unsigned short            mode;     /* dma mode */
    unsigned short            dir;      /* dma transfer direction */
    struct tasklet_struct     tasklet;  /* dma handling tasklet */
    struct completion         done;     /* dma transfer done */
    struct mtk_uart_vfifo *vfifo;    /* dma vfifo */
};
/*---------------------------------------------------------------------------*/
struct mtk_uart_register {  
    unsigned int dll;
    unsigned int dlh;
    unsigned int ier;
    unsigned int lcr;
    unsigned int mcr;
    unsigned int fcr;
    unsigned int lsr;
    unsigned int efr;
    unsigned int highspeed;
    unsigned int sample_count;
    unsigned int sample_point;
    unsigned int fracdiv_l;
    unsigned int fracdiv_m;
    unsigned int escape_en;
    unsigned int guard;
    unsigned int sleep_en;
};
/*---------------------------------------------------------------------------*/
struct mtk_uart
{
    struct uart_port  port;
    unsigned long     base;
    int               nport;
    unsigned int      old_status;
    unsigned int      tx_stop;
    unsigned int      rx_stop;
    unsigned int      ms_enable;
    unsigned int      auto_baud;
    unsigned int      line_status;
    unsigned int      ignore_rx;
    unsigned int      flow_ctrl;
    unsigned long     pending_tx_reqs;
    unsigned long     tx_trig;  /* tx fifo trigger level */
    unsigned long     rx_trig;  /* rx fifo trigger level */
    unsigned long     sysclk;
    int               baudrate; /*current baudrate*/
    int               custom_baud;  /*custom baudrate passed from serial_struct*/
    
    int               dma_mode;
    int               tx_mode;
    int               rx_mode;
    int               fctl_mode;     /*flow control*/
    int               poweron_count;
    int               timeout_count; /*for console write*/
#ifdef UART_FCR_USING_SW_BACK_UP
    unsigned int      fcr_back_up;  /* FCR register is a write only register */
#endif
   
    struct mtk_uart_register registers;
    struct mtk_uart_dma dma_tx;
    struct mtk_uart_dma dma_rx;
    struct mtk_uart_vfifo *tx_vfifo;
    struct mtk_uart_vfifo *rx_vfifo;

#ifdef ENABLE_DEBUG
    struct mtk_uart_regs *debug;
#endif
    struct mtk_uart_setting* setting;

    unsigned int (*write_allow)(struct mtk_uart *uart);
    unsigned int (*read_allow)(struct mtk_uart *uart);
    void         (*write_byte)(struct mtk_uart *uart, unsigned int byte);
    unsigned int (*read_byte)(struct mtk_uart *uart);
    unsigned int (*read_status)(struct mtk_uart *uart);
};
/*---------------------------------------------------------------------------*/
#define UART_FIFO_SIZE              (16)
/*---------------------------------------------------------------------------*/
#define UART_READ8(REG)             __raw_readb(REG)
#define UART_READ16(REG)            __raw_readw(REG)
#define UART_READ32(REG)            __raw_readl(REG)
//#define UART_WRITE8(VAL, REG)       __raw_writeb(VAL, REG)
//#define UART_WRITE16(VAL, REG)      __raw_writew(VAL, REG)
//#define UART_WRITE32(VAL, REG)      __raw_writel(VAL, REG)
/*---------------------------------------------------------------------------*/
#define UART_SET_BITS(BS,REG)       ((*(volatile u32*)(REG)) |= (u32)(BS))
#define UART_CLR_BITS(BS,REG)       ((*(volatile u32*)(REG)) &= ~((u32)(BS)))
/*---------------------------------------------------------------------------*/
#define UART_RBR             (base+0x00) /* Read only */
#define UART_THR             (base+0x00) /* Write only */
#define UART_IER             (base+0x04)
#define UART_IIR             (base+0x08) /* Read only */
#define UART_FCR             (base+0x08) /* Write only */
#define UART_LCR             (base+0x0c)
#define UART_MCR             (base+0x10)
#define UART_LSR             (base+0x14)
#define UART_MSR             (base+0x18)
#define UART_SCR             (base+0x1c)
#define UART_DLL             (base+0x00) /* Only when LCR.DLAB = 1 */
#define UART_DLH             (base+0x04) /* Only when LCR.DLAB = 1 */
#define UART_EFR             (base+0x08) /* Only when LCR = 0xbf */
#define UART_XON1            (base+0x10) /* Only when LCR = 0xbf */
#define UART_XON2            (base+0x14) /* Only when LCR = 0xbf */
#define UART_XOFF1           (base+0x18) /* Only when LCR = 0xbf */
#define UART_XOFF2           (base+0x1c) /* Only when LCR = 0xbf */
#define UART_AUTOBAUD_EN     (base+0x20)
#define UART_HIGHSPEED       (base+0x24)
#define UART_SAMPLE_COUNT    (base+0x28) 
#define UART_SAMPLE_POINT    (base+0x2c) 
#define UART_AUTOBAUD_REG    (base+0x30)
#define UART_RATE_FIX_AD     (base+0x34)
#define UART_AUTOBAUD_SAMPLE (base+0x38)
#define UART_GUARD           (base+0x3c)
#define UART_ESCAPE_DAT      (base+0x40)
#define UART_ESCAPE_EN       (base+0x44)
#define UART_SLEEP_EN        (base+0x48)
#define UART_DMA_EN          (base+0x4c)
#define UART_RXTRI_AD        (base+0x50)
#define UART_FRACDIV_L       (base+0x54)
#define UART_FRACDIV_M       (base+0x58)
#define UART_FCR_RD          (base+0x5C)
#define UART_ACTIVE_EN       (base+0x60)
/* system level, not related to hardware */
#define UST_DUMMY_READ              (1 << 31)
/*---------------------------------------------------------------------------*/
/* For MT6589, both RX and TX will not use port to send or receive data */
/* IER */
#define UART_IER_ERBFI              (1 << 0) /* RX buffer conatins data int. */
#define UART_IER_ETBEI              (1 << 1) /* TX FIFO threshold trigger int. */
#define UART_IER_ELSI               (1 << 2) /* BE, FE, PE, or OE int. */
#define UART_IER_EDSSI              (1 << 3) /* CTS change (DCTS) int. */
#define UART_IER_VFF_FC_EN          (1 << 4) /* When set "1", enable flow control triggered by RX FIFO full when VFIFO_EN is set.*/
#define UART_IER_XOFFI              (1 << 5)
#define UART_IER_RTSI               (1 << 6)
#define UART_IER_CTSI               (1 << 7)

#define UART_IER_ALL_INTS          (UART_IER_ERBFI|UART_IER_ETBEI|UART_IER_ELSI|\
                                    UART_IER_EDSSI|UART_IER_XOFFI|UART_IER_RTSI|\
                                    UART_IER_CTSI)
#define UART_IER_HW_NORMALINTS     (UART_IER_ERBFI|UART_IER_ELSI|UART_IER_EDSSI|UART_IER_VFF_FC_EN)
#define UART_IER_HW_ALLINTS        (UART_IER_ERBFI|UART_IER_ETBEI| \
                                    UART_IER_ELSI|UART_IER_EDSSI)
/*---------------------------------------------------------------------------*/
/* FCR */
#define UART_FCR_FIFOE              (1 << 0)
#define UART_FCR_CLRR               (1 << 1)
#define UART_FCR_CLRT               (1 << 2)
#define UART_FCR_DMA1               (1 << 3)
#define UART_FCR_RXFIFO_1B_TRI      (0 << 6)
#define UART_FCR_RXFIFO_6B_TRI      (1 << 6)
#define UART_FCR_RXFIFO_12B_TRI     (2 << 6)
#define UART_FCR_RXFIFO_RX_TRI      (3 << 6)
#define UART_FCR_TXFIFO_1B_TRI      (0 << 4)
#define UART_FCR_TXFIFO_4B_TRI      (1 << 4)
#define UART_FCR_TXFIFO_8B_TRI      (2 << 4)
#define UART_FCR_TXFIFO_14B_TRI     (3 << 4)

#define UART_FCR_FIFO_INIT          (UART_FCR_FIFOE|UART_FCR_CLRR|UART_FCR_CLRT)
#define UART_FCR_NORMAL             (UART_FCR_FIFO_INIT | \
                                     UART_FCR_TXFIFO_4B_TRI| \
                                     UART_FCR_RXFIFO_12B_TRI)
/*---------------------------------------------------------------------------*/
/* LCR */
#define UART_LCR_BREAK              (1 << 6)
#define UART_LCR_DLAB               (1 << 7)

#define UART_WLS_5                  (0 << 0)
#define UART_WLS_6                  (1 << 0)
#define UART_WLS_7                  (2 << 0)
#define UART_WLS_8                  (3 << 0)
#define UART_WLS_MASK               (3 << 0)

#define UART_1_STOP                 (0 << 2)
#define UART_2_STOP                 (1 << 2)
#define UART_1_5_STOP               (1 << 2)    /* Only when WLS=5 */
#define UART_STOP_MASK              (1 << 2)

#define UART_NONE_PARITY            (0 << 3)
#define UART_ODD_PARITY             (0x1 << 3)
#define UART_EVEN_PARITY            (0x3 << 3)
#define UART_MARK_PARITY            (0x5 << 3)
#define UART_SPACE_PARITY           (0x7 << 3)
#define UART_PARITY_MASK            (0x7 << 3)
/*---------------------------------------------------------------------------*/
/* MCR */
#define UART_MCR_DTR                (1 << 0)
#define UART_MCR_RTS                (1 << 1)
#define UART_MCR_OUT1               (1 << 2)
#define UART_MCR_OUT2               (1 << 3)
#define UART_MCR_LOOP               (1 << 4)
#define UART_MCR_DCM_EN             (1 << 5)    /* MT6589 move to bit5 */
#define UART_MCR_XOFF               (1 << 7)    /* read only */
#define UART_MCR_NORMAL	            (UART_MCR_DTR|UART_MCR_RTS)
/*---------------------------------------------------------------------------*/
/* LSR */
#define UART_LSR_DR                 (1 << 0)
#define UART_LSR_OE                 (1 << 1)
#define UART_LSR_PE                 (1 << 2)
#define UART_LSR_FE                 (1 << 3)
#define UART_LSR_BI                 (1 << 4)
#define UART_LSR_THRE               (1 << 5)
#define UART_LSR_TEMT               (1 << 6)
#define UART_LSR_FIFOERR            (1 << 7)
/*---------------------------------------------------------------------------*/
/* MSR */
#define UART_MSR_DCTS               (1 << 0)
#define UART_MSR_DDSR               (1 << 1)
#define UART_MSR_TERI               (1 << 2)
#define UART_MSR_DDCD               (1 << 3)
#define UART_MSR_CTS                (1 << 4)    
#define UART_MSR_DSR                (1 << 5)
#define UART_MSR_RI                 (1 << 6)
#define UART_MSR_DCD                (1 << 7)
/*---------------------------------------------------------------------------*/
/* EFR */
#define UART_EFR_EN                 (1 << 4)
#define UART_EFR_AUTO_RTS           (1 << 6)
#define UART_EFR_AUTO_CTS           (1 << 7)
#define UART_EFR_SW_CTRL_MASK       (0xf << 0)

#define UART_EFR_NO_SW_CTRL         (0)
#define UART_EFR_NO_FLOW_CTRL       (0)
#define UART_EFR_AUTO_RTSCTS        (UART_EFR_AUTO_RTS|UART_EFR_AUTO_CTS)
#define UART_EFR_XON1_XOFF1         (0xa) /* TX/RX XON1/XOFF1 flow control */
#define UART_EFR_XON2_XOFF2         (0x5) /* TX/RX XON2/XOFF2 flow control */
#define UART_EFR_XON12_XOFF12       (0xf) /* TX/RX XON1,2/XOFF1,2 flow control */

#define UART_EFR_XON1_XOFF1_MASK    (0xa)
#define UART_EFR_XON2_XOFF2_MASK    (0x5)
/*---------------------------------------------------------------------------*/
/* IIR (Read Only) */
#define UART_IIR_NO_INT_PENDING     (0x01)
#define UART_IIR_RLS                (0x06) /* Receiver Line Status */
#define UART_IIR_RDA                (0x04) /* Receive Data Available */
#define UART_IIR_CTI                (0x0C) /* Character Timeout Indicator */
#define UART_IIR_THRE               (0x02) /* Transmit Holding Register Empty */
#define UART_IIR_MS                 (0x00) /* Check Modem Status Register */
#define UART_IIR_SW_FLOW_CTRL       (0x10) /* Receive XOFF characters */
#define UART_IIR_HW_FLOW_CTRL       (0x20) /* CTS or RTS Rising Edge */
#define UART_IIR_FIFO_EN            (0xc0)
#define UART_IIR_INT_MASK           (0x3f)
/*---------------------------------------------------------------------------*/
/* RateFix */
#define UART_RATE_FIX               (1 << 0)
//#define UART_AUTORATE_FIX           (1 << 1)
#define UART_FREQ_SEL               (1 << 1)

#define UART_RATE_FIX_13M           (1 << 0) /* means UARTclk = APBclk / 4 */
#define UART_AUTORATE_FIX_13M       (1 << 1) 
#define UART_FREQ_SEL_13M           (1 << 2)
#define UART_RATE_FIX_ALL_13M       (UART_RATE_FIX_13M|UART_AUTORATE_FIX_13M| \
                                     UART_FREQ_SEL_13M)

#define UART_RATE_FIX_26M           (0 << 0) /* means UARTclk = APBclk / 2 */
#define UART_AUTORATE_FIX_26M       (0 << 1) 
#define UART_FREQ_SEL_26M           (0 << 2)

#define UART_RATE_FIX_16M25         (UART_FREQ_SEL|UART_RATE_FIX)

#define UART_RATE_FIX_32M5          (UART_RATE_FIX)
/*---------------------------------------------------------------------------*/
/* Autobaud sample */
#define UART_AUTOBADUSAM_13M         7
#define UART_AUTOBADUSAM_26M        15
#define UART_AUTOBADUSAM_52M        31
//#define UART_AUTOBADUSAM_52M        29  /* CHECKME! 28 or 29 ? */
#define UART_AUTOBAUDSAM_58_5M      31  /* CHECKME! 31 or 32 ? */
/*---------------------------------------------------------------------------*/
/* DMA enable */
#define UART_RX_DMA_EN              (1 << 0)
#define UART_TX_DMA_EN              (1 << 1)
#define UART_TO_CNT_AUTORST         (1 << 2)
/*---------------------------------------------------------------------------*/
/* Escape character*/
#define UART_ESCAPE_CH              0x77
/*---------------------------------------------------------------------------*/
/* Debugging */
typedef struct {
	u32 RBR:8;
	u32 dummy:24;
} UART_RBR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 ERBFI:1;
	u32 ETBEI:1;
	u32 ELSI:1;
	u32 EDSSI:1;
	u32 dummy1:1;
	u32 XOFFI:1;
	u32 RTSI:1;
	u32 CTSI:1;
	u32 dummy2:24;
} UART_IER_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 FIFOE:1;
	u32 CLRR:1;
	u32 CLRT:1;
	u32 DMA1:1;
	u32 TFTL:2;
	u32 RFTL:2;
	u32 dummy2:24;
} UART_FCR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 WLS:2;
	u32 STB:1;
	u32 PEN:1;
	u32 EPS:1;
	u32 SP:1;
	u32 SB:1;
	u32 DLAB:1;
	u32 dummy:24;
} UART_LCR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 DTR:1;
	u32 RTS:1;
	u32 dummy1:2;
	u32 LOOP:1;
	u32 DCM_EN:1;
	u32 dummy2:1;
	u32 XOFF:1;
	u32 dummy3:24;
} UART_MCR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 DR:1;
	u32 OE:1;
	u32 PE:1;
	u32 FE:1;
	u32 BI:1;
	u32 THRE:1;
	u32 TEMT:1;
	u32 FIFOERR:1;
	u32 dummy:24;
} UART_LSR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 DCTS:1;
	u32 DDSR:1; /* CHECKME! */
	u32 TERI:1; /* CHECKME! */
	u32 DDCD:1; /* CHECKME! */
	u32 CTS:1;
	u32 DSR:1;  /* CHECKME! */
	u32 RI:1;   /* CHECKME! */
	u32 DCD:1;  /* CHECKME! */
	u32 dummy:24;
} UART_MSR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 SCR:8;
	u32 dummy:24;
} UART_SCR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 AUTO_EN:1;
	u32 dummy:31;
} UART_AUTOBAUD_EN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 SPEED:2;
	u32 dummy:30;
} UART_HIGHSPEED_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 SAM_COUNT:8;
	u32 dummy:24;
} UART_SAM_COUNT_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 SAM_POINT:8;
	u32 dummy:24;
} UART_SAM_POINT_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 BAUD_RATE:4;
	u32 BAUD_STAT:4;
	u32 dummy:24;
} UART_AUTOBAUD_REG_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 RATE_FIX:1;
	u32 AUTOBAUD_RATE_FIX:1;
	u32 FREQ_SEL:1;
	u32 RESTRICT:1;
	u32 dummy:28;
} UART_RATEFIX_AD_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 AUTOBAUD_SAM:8;
	u32 dummy:24;
} UART_AUTOBAUD_SAM_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 GUARD_CNT:4;
	u32 GUARD_EN:1;
	u32 dummy:27;
} UART_GUARD_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 ESC_DATA:8;
	u32 dummy:24;
} UART_ESC_DATA_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 ESC_EN:1;
	u32 dummy:31;
} UART_ESC_EN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 SLEEP_EN:1;
	u32 dummy:31;
} UART_SLEEP_EN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 RX_DMA_EN:1;
    u32 TX_DMA_EN:1;
    u32 TO_CNT_AUTORST:1;
	u32 dummy:29;
} UART_DMA_EN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 RXTRIG:4;
	u32 dummy:28;
} UART_RXTRIG_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 THR:8;
	u32 dummy:24;
} UART_THR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 NINT:1;
	u32 ID:5;
	u32 FIFOE:2;
	u32 dummy:24;
} UART_IIR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 DLL:8;
	u32 dummy:24;
} UART_DLL_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 DLH:8;
	u32 dummy:24;
} UART_DLH_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 SW_FLOW_CTRL:4;
	u32 EFR_EN:1;
	u32 dummy1:1;
	u32 AUTO_RTS:1;
	u32 AUTO_CTS:1;
	u32 dummy2:24;
} UART_EFR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 XON1:8;
	u32 dummy:24;
} UART_XON1_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 XON2:8;
	u32 dummy:24;
} UART_XON2_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 XOFF1:8;
	u32 dummy:24;
} UART_XOFF1_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
	u32 XOFF2:8;
	u32 dummy:24;
} UART_XOFF2_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 FRACDIV_L:8;
    u32 dummy:24;
} UART_FRACDIV_L_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 FRACDIV_M:2;
    u32 dummy:30;
} UART_FRACDIV_M_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 FIFOE:1;
    u32 resv:2;
    u32 DMA1:1;
    u32 TFTL:2;
    u32 RFTL:2;
    u32 dummy:24;
} UART_FCR_RD_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 TX_OE_EN:1;
    u32 TX_PU_EN:1;
    u32 dummy:30;
} UART_TX_ACTIVE_EN_REG;
/*---------------------------------------------------------------------------*/
struct mtk_uart_regs {
    UART_RBR_REG *RBR;                  /* 0x0000, RO */
    UART_THR_REG *THR;                  /* 0x0000, WO */
    UART_IER_REG *IER;                  /* 0x0004,    */    
    UART_IIR_REG *IIR;                  /* 0x0008, R0 */
    UART_FCR_REG *FCR;                  /* 0x0008, WO */    
    UART_LCR_REG *LCR;                  /* 0x000C,    */
    UART_MCR_REG *MCR;                  /* 0x0010,    */
    UART_LSR_REG *LSR;                  /* 0x0014,    */
    UART_MSR_REG *MSR;                  /* 0x0018,    */
    UART_SCR_REG *SCR;                  /* 0x001C,    */
    UART_AUTOBAUD_EN_REG *AUTOBAUD_EN;  /* 0x0020,    */
    UART_HIGHSPEED_REG *HIGHSPEED;      /* 0x0024,    */
    UART_SAM_COUNT_REG *SAM_CNT;        /* 0x0028,    */
    UART_SAM_POINT_REG *SAM_POT;        /* 0x002C,    */
    UART_AUTOBAUD_REG_REG *AUTOBAUD;    /* 0x0030,    */
    UART_RATEFIX_AD_REG *RATEFIX_AD;    /* 0x0034,    */
    
    UART_GUARD_REG *GUARD;              /* 0x003C,    */
    UART_ESC_DATA_REG *ESC_DATA;        /* 0x0040, WO */
    UART_ESC_EN_REG *ESC_EN;            /* 0x0044,    */
    UART_SLEEP_EN_REG *SLEEP_EN;        /* 0x0048,    */
    UART_DMA_EN_REG *DMA_EN;            /* 0x004C,    */
    UART_RXTRIG_REG *RXTRIG;            /* 0x0050,    */
    UART_FRACDIV_L_REG *FRACDIV_L;      /* 0x0054,    */
    UART_FRACDIV_M_REG *FRACDIV_M;      /* 0x0058,    */    
    UART_FCR_RD_REG *FCR_RD;            /* 0x005C, RO */
    UART_TX_ACTIVE_EN_REG *TX_ACT_EN;   /* 0x0060,    */
    

    /* Only when LCR.DLAB = 1 */
    UART_DLL_REG *DLL;                  /* 0x0000,    */
    UART_DLH_REG *DLH;                  /* 0x0004,    */

    /* Only when LCR = 0xBFh */
    UART_EFR_REG *EFR;                  /* 0x0008,    */
    UART_XON1_REG *XON1;                /* 0x0010,    */
    UART_XON2_REG *XON2;                /* 0x0014,    */
    UART_XOFF1_REG *XOFF1;              /* 0x0018,    */
    UART_XOFF2_REG *XOFF2;              /* 0x001C,    */
};
/*---------------------------------------------------------------------------*/
#define INI_REGS(BASE) \
{   .RBR = (UART_RBR_REG *)((BASE) + 0x0), \
    .THR = (UART_THR_REG *)((BASE) + 0x0), \
    .IER = (UART_IER_REG *)((BASE) + 0x4), \
    .IIR = (UART_IIR_REG *)((BASE) + 0x8), \
    .FCR = (UART_FCR_REG *)((BASE) + 0x8), \
    .LCR = (UART_LCR_REG *)((BASE) + 0xc), \
    .MCR = (UART_MCR_REG *)((BASE) + 0x10), \
    .LSR = (UART_LSR_REG *)((BASE) + 0x14), \
    .MSR = (UART_MSR_REG *)((BASE) + 0x18), \
    .SCR = (UART_SCR_REG *)((BASE) + 0x1c), \
    .AUTOBAUD_EN = (UART_AUTOBAUD_EN_REG *)((BASE) + 0x20), \
    .HIGHSPEED = (UART_HIGHSPEED_REG *)((BASE) + 0x24), \
    .SAM_CNT = (UART_SAM_COUNT_REG *)((BASE) + 0x28), \
    .SAM_POT = (UART_SAM_POINT_REG *)((BASE) + 0x2c), \
    .AUTOBAUD = (UART_AUTOBAUD_REG_REG *)((BASE) + 0x30), \
    .RATEFIX_AD = (UART_RATEFIX_AD_REG *)((BASE) + 0x34), \
    \
    .GUARD = (UART_GUARD_REG *)((BASE) + 0x3c), \
    .ESC_DATA = (UART_ESC_DATA_REG *)((BASE) + 0x40), \
    .ESC_EN = (UART_ESC_EN_REG *)((BASE) + 0x44), \
    .SLEEP_EN = (UART_SLEEP_EN_REG *)((BASE) + 0x48), \
    .DMA_EN = (UART_DMA_EN_REG *)((BASE) + 0x4c), \
    .RXTRIG = (UART_RXTRIG_REG *)((BASE) + 0x50), \
    .FRACDIV_L = (UART_FRACDIV_L_REG *)((BASE) + 0x54), \
    .FRACDIV_M = (UART_FRACDIV_M_REG *)((BASE) + 0x58), \
    .FCR_RD = (UART_FCR_RD_REG *)((BASE) + 0x5C), \
    .TX_ACT_EN = (UART_TX_ACTIVE_EN_REG *)((BASE) + 0x60), \
    \
    .DLL = (UART_DLL_REG *)((BASE) + 0x0), \
    .DLH = (UART_DLH_REG *)((BASE) + 0x4), \
    .EFR = (UART_EFR_REG *)((BASE) + 0x8), \
    .XON1 = (UART_XON1_REG *)((BASE) + 0x10), \
    .XON2 = (UART_XON2_REG *)((BASE) + 0x14), \
    .XOFF1 = (UART_XOFF1_REG *)((BASE) + 0x18), \
    .XOFF2 = (UART_XOFF2_REG *)((BASE) + 0x1c), \
}
/*---------------------------------------------------------------------------*/
#define VFF_BASE_CH_S           (12)
#define VFF_BASE_CH(n)          (AP_DMA_BASE+0x0080*(n+1+VFF_BASE_CH_S))
#define VFF_INT_FLAG(_b)        (_b+0x0000) 
#define VFF_INT_EN(_b)          (_b+0x0004) 
#define VFF_EN(_b)              (_b+0x0008) 
#define VFF_RST(_b)             (_b+0x000C)     
#define VFF_STOP(_b)            (_b+0x0010) 
#define VFF_FLUSH(_b)           (_b+0x0014) 
#define VFF_ADDR(_b)            (_b+0x001C) 
#define VFF_LEN(_b)             (_b+0x0024) 
#define VFF_THRE(_b)            (_b+0x0028) 
#define VFF_WPT(_b)             (_b+0x002C) 
#define VFF_RPT(_b)             (_b+0x0030) 
#define VFF_W_INT_BUF_SIZE(_b)  (_b+0x0034) 
#define VFF_INT_BUF_SIZE(_b)    (_b+0x0038) 
#define VFF_VALID_SIZE(_b)      (_b+0x003C) 
#define VFF_LEFT_SIZE(_b)       (_b+0x0040) 
#define VFF_DEBUG_STATUS(_b)    (_b+0x0050) 
#define VFF_VPORT_BASE          0xF7070000
#define VFF_VPORT_CH(id)        (VFF_VPORT_BASE + (id) * 0x00000080)
/*---------------------------------------------------------------------------*/
/*VFF_INT_FLAG */
#define VFF_RX_INT_FLAG0_B      (1 << 0)    /*rx_vff_valid_size >= rx_vff_thre*/
#define VFF_RX_INT_FLAG1_B      (1 << 1)    /*when UART issues flush to DMA and all data in UART VFIFO is transferred to VFF*/
#define VFF_TX_INT_FLAG0_B      (1 << 0)    /*tx_vff_left_size >= tx_vff_thrs*/
#define VFF_INT_FLAG_CLR_B      (0 << 0)
/*VFF_INT_EN*/
#define VFF_RX_INT_EN0_B        (1 << 0)    /*rx_vff_valid_size >= rx_vff_thre*/
#define VFF_RX_INT_EN1_B        (1 << 1)    /*when UART issues flush to DMA and all data in UART VFIFO is transferred to VFF*/
#define VFF_TX_INT_EN_B         (1 << 0)    /*tx_vff_left_size >= tx_vff_thrs*/
#define VFF_INT_EN_CLR_B        (0 << 0)
/*VFF_RST*/
#define VFF_WARM_RST_B          (1 << 0)
#define VFF_HARD_RST_B          (1 << 1)
/*VFF_EN*/
#define VFF_EN_B                (1 << 0)
/*VFF_STOP*/
#define VFF_STOP_B              (1 << 0)
#define VFF_STOP_CLR_B          (0 << 0)
/*VFF_FLUSH*/
#define VFF_FLUSH_B             (1 << 0)
#define VFF_FLUSH_CLR_B         (0 << 0)

#define VFF_TX_THRE(n)          ((n)*7/8)   /*tx_vff_left_size >= tx_vff_thrs*/
#define VFF_RX_THRE(n)          ((n)*3/4)    // trigger level of rx vfifo
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 FLAG0:1;
    u32 dummy:31;
} TX_INT_FLAG_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 INTEN:1;
    u32 dummy:31;
} TX_INT_EN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 EN:1;
    u32 dummy:31;
} TX_EN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 WARM_RST:1;
    u32 HARD_RST:1;
    u32 dummy:30;
} TX_RST_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 STOP:1;
    u32 dummy:31;
} TX_STOP_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 FLUSH:1;
    u32 dummy:31;
} TX_FLUSH_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 align:3;
    u32 TX_VFF_ADDR:29;
} TX_VFF_ADDR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 align:3;
    u32 TX_VFF_LEN:13;
    u32 dummy:16;
} TX_VFF_LEN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 TX_VFF_THRE:16;
    u32 dummy:16;
} TX_VFF_THRE_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 TX_VFF_WPT:16;
    u32 TX_VFF_WPT_WRAP:1;
    u32 dummy:15;
} TX_VFF_WPT_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 TX_VFF_RPT:16;
    u32 TX_VFF_RPT_WRAP:1;
    u32 dummy:15;
} TX_VFF_RPT_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 TX_W_INT_BUF_SIZE:7;
    u32 dummy:25;
} TX_W_INT_BUF_SIZE_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 TX_INT_BUF_SIZE:5;
    u32 dummy:27;    
} TX_INT_BUF_SIZE_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 TX_VFF_VALID_SIZE:16;
    u32 dummy:16;    
} TX_VFF_VALID_SIZE_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 TX_VFF_LEFT_SIZE:16;
    u32 dummy:16;    
} TX_VFF_LEFT_SIZE_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 R_Q_CLR:1;
    u32 W_Q_CLR:1;
    u32 RREQ:1;
    u32 WREQ:1;
    u32 RD_ACT:1;
    u32 WD_ACT:1;
    u32 FLUSH_STR:1;
    u32 FLUSH_ACT:1;
    u32 WADDR_D:5;
    u32 _r0:3;
    u32 WADDR_D_LH:5;
    u32 _r1:3;
    u32 RADDR_D:5;
    u32 _r2:3;
} TX_VFF_DEBUG_STATUS_REG;
/*---------------------------------------------------------------------------*/
struct mtk_vfifo_tx_reg {
    TX_INT_FLAG_REG* TX_INT_FLAG;
    TX_INT_EN_REG* TX_INT_EN;
    TX_EN_REG* TX_EN;
    TX_RST_REG* TX_RST;
    TX_STOP_REG* TX_STOP;
    TX_FLUSH_REG* TX_FLUSH;
    TX_VFF_ADDR_REG* TX_VFF_ADDR;
    TX_VFF_LEN_REG* TX_VFF_LEN;
    TX_VFF_THRE_REG* TX_VFF_THRE;
    TX_VFF_WPT_REG* TX_VFF_WPT;
    TX_VFF_RPT_REG* TX_VFF_RPT;
    TX_W_INT_BUF_SIZE_REG* TX_W_INT_BUF_SIZE;
    TX_INT_BUF_SIZE_REG* TX_INT_BUF_SIZE;
    TX_VFF_VALID_SIZE_REG *TX_VFF_VALID_SIZE;
    TX_VFF_LEFT_SIZE_REG *TX_VFF_LEFT_SIZE;
    TX_VFF_DEBUG_STATUS_REG* TX_VFF_DEBUG_STATUS;
};
/*---------------------------------------------------------------------------*/
#define INIT_VFIFO_TX_REGS(BASE) \
{   .TX_INT_FLAG = (TX_INT_FLAG_REG *)((BASE) + 0x00), \
    .TX_INT_EN = (TX_INT_EN_REG*)((BASE) + 0x04), \
    .TX_EN = (TX_EN_REG*)((BASE) + 0x04), \
    .TX_RST = (TX_RST_REG*)((BASE) + 0x04), \
    .TX_STOP = (TX_STOP_REG*)((BASE) + 0x04), \
    .TX_FLUSH = (TX_FLUSH_REG*)((BASE) + 0x04), \
    .TX_VFF_ADDR = (TX_VFF_ADDR_REG*)((BASE) + 0x04), \
    .TX_VFF_LEN = (TX_VFF_LEN_REG*)((BASE) + 0x04), \
    .TX_VFF_THRE = (TX_VFF_THRE_REG*)((BASE) + 0x04), \
    .TX_VFF_WPT = (TX_VFF_WPT_REG*)((BASE) + 0x04), \
    .TX_VFF_RPT = (TX_VFF_RPT_REG*)((BASE) + 0x04), \
    .TX_W_INT_BUF_SIZE = (TX_W_INT_BUF_SIZE_REG*)((BASE) + 0x04), \
    .TX_INT_BUF_SIZE = (TX_INT_BUF_SIZE_REG*)((BASE) + 0x04), \
    .TX_VFF_VALID_SIZE = (TX_VFF_VALID_SIZE_REG*)((BASE) + 0x04), \
    .TX_VFF_LEFT_SIZE = (TX_VFF_LEFT_SIZE_REG*)((BASE) + 0x04), \
    .TX_VFF_DEBUG_STATUS = (TX_VFF_DEBUG_STATUS_REG*)((BASE) + 0x04), \
}
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 FLAG0:1;
    u32 FLAG1:1;
    u32 dummy:30;
} RX_INT_FLAG_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 INTEN0:1;
    u32 INTEN1:1;
    u32 dummy:30;
} RX_INT_EN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 EN:1;
    u32 dummy:31;
} RX_EN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 WARM_RST:1;
    u32 HARD_RST:1;
    u32 dummy:30;
} RX_RST_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 STOP:1;
    u32 dummy:31;
} RX_STOP_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 align:3;
    u32 RX_VFF_ADDR:29;
} RX_VFF_ADDR_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 align:3;
    u32 RX_VFF_LEN:13;
    u32 dummy:16;
} RX_VFF_LEN_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 RX_VFF_THRE:16;
    u32 dummy:16;
} RX_VFF_THRE_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 RX_VFF_WPT:16;
    u32 RX_VFF_WPT_WRAP:1;
    u32 dummy:15;
} RX_VFF_WPT_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 RX_VFF_RPT:16;
    u32 RX_VFF_RPT_WRAP:1;
    u32 dummy:15;
} RX_VFF_RPT_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 RX_INT_BUF_SIZE:5;
    u32 dummy:27;    
} RX_INT_BUF_SIZE_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 RX_VFF_VALID_SIZE:16;
    u32 dummy:16;    
} RX_VFF_VALID_SIZE_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 RX_VFF_LEFT_SIZE:16;
    u32 dummy:16;    
} RX_VFF_LEFT_SIZE_REG;
/*---------------------------------------------------------------------------*/
typedef struct {
    u32 R_Q_CLR:1;
    u32 W_Q_CLR:1;
    u32 RREQ:1;
    u32 WREQ:1;
    u32 RD_ACT:1;
    u32 WD_ACT:1;
    u32 _r4:1;
    u32 FLUSH_ACT:1;
    u32 WADDR_D:5;
    u32 _r0:3;
    u32 WADDR_D_LH:5;
    u32 _r1:3;
    u32 RADDR_D:5;
    u32 _r2:3;
} RX_VFF_DEBUG_STATUS_REG;
/*---------------------------------------------------------------------------*/
struct mtk_vfifo_rx_reg {
    RX_INT_FLAG_REG* RX_INT_FLAG;
    RX_INT_EN_REG* RX_INT_EN;
    RX_EN_REG* RX_EN;
    RX_RST_REG* RX_RST;
    RX_STOP_REG* RX_STOP;
    RX_VFF_ADDR_REG* RX_VFF_ADDR;
    RX_VFF_LEN_REG* RX_VFF_LEN;
    RX_VFF_THRE_REG* RX_VFF_THRE;
    RX_VFF_WPT_REG* RX_VFF_WPT;
    RX_VFF_RPT_REG* RX_VFF_RPT;
    RX_INT_BUF_SIZE_REG* RX_INT_BUF_SIZE;
    RX_VFF_VALID_SIZE_REG *RX_VFF_VALID_SIZE;
    RX_VFF_LEFT_SIZE_REG *RX_VFF_LEFT_SIZE;
    RX_VFF_DEBUG_STATUS_REG* RX_VFF_DEBUG_STATUS;
};
/*---------------------------------------------------------------------------*/
#define INIT_VFIFO_RX_REGS(BASE) \
{   .RX_INT_FLAG = (RX_INT_FLAG_REG *)((BASE) + 0x00), \
    .RX_INT_EN = (RX_INT_EN_REG*)((BASE) + 0x04), \
    .RX_EN = (RX_EN_REG*)((BASE) + 0x04), \
    .RX_RST = (RX_RST_REG*)((BASE) + 0x04), \
    .RX_STOP = (RX_STOP_REG*)((BASE) + 0x04), \
    .RX_VFF_ADDR = (RX_VFF_ADDR_REG*)((BASE) + 0x04), \
    .RX_VFF_LEN = (RX_VFF_LEN_REG*)((BASE) + 0x04), \
    .RX_VFF_THRE = (RX_VFF_THRE_REG*)((BASE) + 0x04), \
    .RX_VFF_WPT = (RX_VFF_WPT_REG*)((BASE) + 0x04), \
    .RX_VFF_RPT = (RX_VFF_RPT_REG*)((BASE) + 0x04), \
    .RX_INT_BUF_SIZE = (RX_INT_BUF_SIZE_REG*)((BASE) + 0x04), \
    .RX_VFF_VALID_SIZE = (RX_VFF_VALID_SIZE_REG*)((BASE) + 0x04), \
    .RX_VFF_LEFT_SIZE = (RX_VFF_LEFT_SIZE_REG*)((BASE) + 0x04), \
    .RX_VFF_DEBUG_STATUS = (RX_VFF_DEBUG_STATUS_REG*)((BASE) + 0x04), \
}
/*---------------------------------------------------------------------------*/
struct mtk_dma_vfifo_reg {
    struct mtk_vfifo_tx_reg tx;
    struct mtk_vfifo_rx_reg rx;
};

/*---------------------------------------------------------------------------*/
#endif /* MTK_UART_H */
