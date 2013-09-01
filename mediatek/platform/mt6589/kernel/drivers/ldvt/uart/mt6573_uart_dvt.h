
#ifndef __MT6573_UART_DVT_H__
#define	__MT6573_UART_DVT_H__

#define MAX_UART_PORT_ID            (4)
#define MIN_UART_PORT_ID            (1)

/* UART GPIO Settings section */
#define UART_GPIO_PIN_UN_SUPPORT    (-1)
/* UART 1 Pin ID*/
#define UART1_TX_GPIO_PIN           (4)
#define UART1_RX_GPIO_PIN           (3)
#define UART1_CTS_GPIO_PIN          (116)
#define UART1_RTS_GPIO_PIN          (115)
/* UART 1 Pin mode */
#define UART1_TX_GPIO_PIN_M         (1)
#define UART1_RX_GPIO_PIN_M         (1)
#define UART1_CTS_GPIO_PIN_M        (3)
#define UART1_RTS_GPIO_PIN_M        (3)

/* UART 2 Pin ID*/
#define UART2_TX_GPIO_PIN           (6)
#define UART2_RX_GPIO_PIN           (5)
#define UART2_CTS_GPIO_PIN          (124)
#define UART2_RTS_GPIO_PIN          (123)
/* UART 2 Pin mode */
#define UART2_TX_GPIO_PIN_M         (1)
#define UART2_RX_GPIO_PIN_M         (1)
#define UART2_CTS_GPIO_PIN_M        (3)
#define UART2_RTS_GPIO_PIN_M        (3)

/* UART 3 Pin ID*/
#define UART3_TX_GPIO_PIN           (184)
#define UART3_RX_GPIO_PIN           (183)
#define UART3_CTS_GPIO_PIN          (30)
#define UART3_RTS_GPIO_PIN          (29)
/* UART 3 Pin mode */
#define UART3_TX_GPIO_PIN_M         (1)
#define UART3_RX_GPIO_PIN_M         (1)
#define UART3_CTS_GPIO_PIN_M        (4)
#define UART3_RTS_GPIO_PIN_M        (4)

/* UART 4 Pin ID*/
#define UART4_TX_GPIO_PIN           (186)
#define UART4_RX_GPIO_PIN           (185)
#define UART4_CTS_GPIO_PIN          (132)
#define UART4_RTS_GPIO_PIN          (131)
/* UART 4 Pin mode */
#define UART4_TX_GPIO_PIN_M         (1)
#define UART4_RX_GPIO_PIN_M         (1)
#define UART4_CTS_GPIO_PIN_M        (4)
#define UART4_RTS_GPIO_PIN_M        (4)

/* UART IRQ ID Section */
#define UART1_IRQ_ID                (13)
#define UART2_IRQ_ID                (14)
#define UART3_IRQ_ID                (15)
#define UART4_IRQ_ID                (16)

/* UART Registers */
#define UART_RBR(base)                    (base+0x00) /* Read only */
#define UART_THR(base)                    (base+0x00) /* Write only */
#define UART_IER(base)                    (base+0x04)
#define UART_IIR(base)                    (base+0x08) /* Read only */
#define UART_FCR(base)                    (base+0x08) /* Write only */
#define UART_LCR(base)                    (base+0x0c)
#define UART_MCR(base)                    (base+0x10)
#define UART_LSR(base)                    (base+0x14)
#define UART_MSR(base)                    (base+0x18)
#define UART_SCR(base)                    (base+0x1c)
#define UART_DLL(base)                    (base+0x00) /* Only when LCR.DLAB = 1 */
#define UART_DLH(base)                    (base+0x04) /* Only when LCR.DLAB = 1 */
#define UART_EFR(base)                    (base+0x08) /* Only when LCR = 0xbf */
#define UART_XON1(base)                   (base+0x10) /* Only when LCR = 0xbf */
#define UART_XON2(base)                   (base+0x14) /* Only when LCR = 0xbf */
#define UART_XOFF1(base)                  (base+0x18) /* Only when LCR = 0xbf */
#define UART_XOFF2(base)                  (base+0x1c) /* Only when LCR = 0xbf */
#define UART_AUTOBAUD_EN(base)            (base+0x20)
#define UART_HIGHSPEED(base)              (base+0x24)
#define UART_SAMPLE_COUNT(base)           (base+0x28) 
#define UART_SAMPLE_POINT(base)           (base+0x2c) 
#define UART_AUTOBAUD_REG(base)           (base+0x30)
#define UART_RATE_FIX_AD(base)            (base+0x34)
#define UART_AUTOBAUD_SAMPLE(base)        (base+0x38)
#define UART_GUARD(base)                  (base+0x3c)
#define UART_ESCAPE_DAT(base)             (base+0x40)
#define UART_ESCAPE_EN(base)              (base+0x44)
#define UART_SLEEP_EN(base)               (base+0x48)
#define UART_DMA_EN(base)                 (base+0x4c)
#define UART_RXTRI_AD(base)               (base+0x50)
#define UART_FRACDIV_L(base)              (base+0x54)
#define UART_FRACDIV_M(base)              (base+0x58)
#define UART_FCR_RD(base)                 (base+0x5C)
#define UART_ACTIVE_EN(base)              (base+0x60)

typedef struct _UART_REG_LIST{
    unsigned int UART_RBR;
    unsigned int UART_THR;
    unsigned int UART_IER;
    unsigned int UART_IIR;
    unsigned int UART_FCR;
    unsigned int UART_LCR;
    unsigned int UART_MCR;
    unsigned int UART_LSR;
    unsigned int UART_MSR;
    unsigned int UART_SCR;
    unsigned int UART_DLL;
    unsigned int UART_DLH;
    unsigned int UART_EFR;
    unsigned int UART_XON1;
    unsigned int UART_XON2;
    unsigned int UART_XOFF1;
    unsigned int UART_XOFF2;
    unsigned int UART_AUTOBAUD_EN;
    unsigned int UART_HIGHSPEED;
    unsigned int UART_SAMPLE_COUNT;
    unsigned int UART_SAMPLE_POINT;
    unsigned int UART_AUTOBAUD_REG;
    unsigned int UART_RATE_FIX_AD;
    unsigned int UART_AUTOBAUD_SAMPLE;
    unsigned int UART_GUARD;
    unsigned int UART_ESCAPE_DAT;
    unsigned int UART_ESCAPE_EN;
    unsigned int UART_SLEEP_EN;
    unsigned int UART_DMA_EN;
    unsigned int UART_RXTRI_AD;
    unsigned int UART_FRACDIV_L;
    unsigned int UART_FRACDIV_M;
    unsigned int UART_FCR_RD;
    unsigned int UART_ACTIVE_EN;
}UART_REG_LIST;

/* VFF TX Registers */
#define VFF_TX_INT_FLAG(_b)            (_b+0x0000)
#define VFF_TX_INT_EN(_b)              (_b+0x0004)
#define VFF_TX_EN(_b)                  (_b+0x0008)
#define VFF_TX_RST(_b)                 (_b+0x000C)
#define VFF_TX_STOP(_b)                (_b+0x0010)
#define VFF_TX_FLUSH(_b)               (_b+0x0014)
#define VFF_TX_ADDR(_b)                (_b+0x001C)
#define VFF_TX_LEN(_b)                 (_b+0x0024)
#define VFF_TX_THRE(_b)                (_b+0x0028)
#define VFF_TX_WPT(_b)                 (_b+0x002C)
#define VFF_TX_RPT(_b)                 (_b+0x0030)
#define VFF_TX_W_INT_BUF_SIZE(_b)      (_b+0x0034)
#define VFF_TX_INT_BUF_SIZE(_b)        (_b+0x0038)
#define VFF_TX_VALID_SIZE(_b)          (_b+0x003C)
#define VFF_TX_LEFT_SIZE(_b)           (_b+0x0040)
#define VFF_TX_DEBUG_STATUS(_b)        (_b+0x0050)

typedef struct _VFF_TX_REG_LIST{
    unsigned int VFF_TX_INT_FLAG;
    unsigned int VFF_TX_INT_EN;
    unsigned int VFF_TX_EN;
    unsigned int VFF_TX_RST;
    unsigned int VFF_TX_STOP;
    unsigned int VFF_TX_FLUSH;
    unsigned int VFF_TX_ADDR;
    unsigned int VFF_TX_LEN;
    unsigned int VFF_TX_THRE;
    unsigned int VFF_TX_WPT;
    unsigned int VFF_TX_RPT;
    unsigned int VFF_TX_W_INT_BUF_SIZE;
    unsigned int VFF_TX_INT_BUF_SIZE;
    unsigned int VFF_TX_VALID_SIZE;
    unsigned int VFF_TX_LEFT_SIZE;
    unsigned int VFF_TX_DEBUG_STATUS;
}VFF_TX_REG_LIST;

/* VFF TX Registers */
#define VFF_RX_INT_FLAG(_b)            (_b+0x0000)
#define VFF_RX_INT_EN(_b)              (_b+0x0004)
#define VFF_RX_EN(_b)                  (_b+0x0008)
#define VFF_RX_RST(_b)                 (_b+0x000C)
#define VFF_RX_STOP(_b)                (_b+0x0010)
#define VFF_RX_FLUSH(_b)               (_b+0x0014)
#define VFF_RX_ADDR(_b)                (_b+0x001C)
#define VFF_RX_LEN(_b)                 (_b+0x0024)
#define VFF_RX_THRE(_b)                (_b+0x0028)
#define VFF_RX_WPT(_b)                 (_b+0x002C)
#define VFF_RX_RPT(_b)                 (_b+0x0030)
#define VFF_RX_FC_THRSHOLD(_b)         (_b+0x0034)
#define VFF_RX_INT_BUF_SIZE(_b)        (_b+0x0038)
#define VFF_RX_VALID_SIZE(_b)          (_b+0x003C)
#define VFF_RX_LEFT_SIZE(_b)           (_b+0x0040)
#define VFF_RX_DEBUG_STATUS(_b)        (_b+0x0050)

typedef struct _VFF_RX_REG_LIST{
    unsigned int VFF_RX_INT_FLAG;
    unsigned int VFF_RX_INT_EN;
    unsigned int VFF_RX_EN;
    unsigned int VFF_RX_RST;
    unsigned int VFF_RX_STOP;
    unsigned int VFF_RX_FLUSH;
    unsigned int VFF_RX_ADDR;
    unsigned int VFF_RX_LEN;
    unsigned int VFF_RX_THRE;
    unsigned int VFF_RX_WPT;
    unsigned int VFF_RX_RPT;
    unsigned int VFF_RX_FC_THRSHOLD;
    unsigned int VFF_RX_INT_BUF_SIZE;
    unsigned int VFF_RX_VALID_SIZE;
    unsigned int VFF_RX_LEFT_SIZE;
    unsigned int VFF_RX_DEBUG_STATUS;
}VFF_RX_REG_LIST;

/* Base address section */
#define UART1_BASE_ADDR     (0xF7003000)
#define UART2_BASE_ADDR     (0xF7004000)
#define UART3_BASE_ADDR     (0xF7005000)
#define UART4_BASE_ADDR     (0xF7006000)

#define VFF_TX1_BASE_ADDR   (0xF7002000 + 0x600)
#define VFF_TX2_BASE_ADDR   (0xF7002000 + 0x700)
#define VFF_TX3_BASE_ADDR   (0xF7002000 + 0x800)
#define VFF_TX4_BASE_ADDR   (0xF7002000 + 0x900)

#define VFF_RX1_BASE_ADDR   (0xF7002000 + 0x680)
#define VFF_RX2_BASE_ADDR   (0xF7002000 + 0x780)
#define VFF_RX3_BASE_ADDR   (0xF7002000 + 0x880)
#define VFF_RX4_BASE_ADDR   (0xF7002000 + 0x980)

/* Error info */
enum{
	UART_DRV_EXT_ERR_SUCCESS = 0,
	UART_DRV_EXT_ERR_INVALID_UART_ID = 1000,
	UART_DRV_EXT_ERR_MAPPING,
	UART_DRV_EXT_ERR_UNDEFINED_OP,
	UART_DRV_EXT_ERR_NULL_POINTER,
	UART_DRV_EXT_ERR_MONITOR_MISS_MATCH,
};

#endif //__MT6573_UART_DVT_H__

