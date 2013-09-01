

#ifndef _IRFRAMER_HW_H
#define _IRFRAMER_HW_H
#ifdef MT6573
	#define IRDA_IRQ MT6573_IRDA_IRQ_LINE
	#include <mach/mt6573_reg_base.h>
	#include <mach/mt6573_pll.h>
	#include <mach/mt6573_gpio.h>
	#define IRDA_base IRDA_BASE
	
#elif defined(MT6516)
	#include <mach/mt6516_apmcusys.h>
	#include <mach/mt6516_gpio.h>
	#define IRDA_IRQ MT6516_IRDA_IRQ_LINE
	#define mt65xx_irq_unmask MT6516_IRQUnmask
	extern void MT6516_IRQUnmask(unsigned int);
	#define mt65xx_irq_mask   MT6516_IRQMask
	extern void MT6516_IRQMask(unsigned int);
#elif defined(MT6575)
	#define IRDA_IRQ MT6575_IRDA_IRQ_ID
	#include <mach/mt6575_reg_base.h>
//	#include <mach/mt6575_pll.h>
	#include <mach/mt6575_gpio.h>
	#include <mach/mt_clkmgr.h>
	#include <mach/sync_write.h>
	#include "mach/irqs.h"
	#define  IRDA_base  (I2C2_BASE+0x1000)
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



	#define DMA_BASE_CH(n)      (AP_DMA_BASE + 0x0080 * (n + 1))
	#define DMA_GLOBAL_INT_FLAG (AP_DMA_BASE + 0x0000) 

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
#define DMA_IBUFF_SIZE(base)    (base + 0x0038)
//#define DMA_CONNECT(base)       (base + 0x0034)
#define DMA_DBG_STAT(base)      (base + 0x0050)

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

	extern void irda_start_dma(void);
	extern void irda_stop_dma(void);
	
struct irda_dma_conf
{
    unsigned int count;
    int iten;
    unsigned int burst;
    int dinc;
    int sinc;
    unsigned int limiter;
    unsigned int src;
    unsigned int dst;
    int wpen;
    int wpsd;
    unsigned int wplen;
    unsigned int wpto;
    int dir;
};

#define P_DMA_IRDA 6

#endif

#define TAG "Irda_Debug--->"
#ifdef DEBUG
#define dbg(format,arg...) printk(KERN_WARNING "%s" format,TAG,##arg)
#else 
#define dbg(format,arg...)
#endif


#ifdef MT6573
typedef enum {
    DMA_FULL_CHANNEL = 0,
    DMA_HALF_CHANNEL,
    DMA_VIRTUAL_FIFO
} DMA_TYPE;
typedef struct DMA_CHAN {
    struct mt_dma_conf config;
    u32 baseAddr;
    DMA_TYPE type;
    unsigned char chan_num;
    unsigned int registered;
} DMA_CHAN;


#define DMA_BASE_CH(n) (AP_DMA_BASE + 0x0080*(n + 1))
#define DMA_GLOBAL_INT_FLAG (AP_DMA_BASE + 0x0000) 
#define DMA_VPORT_BASE 0xF7002600
#define DMA_VPORT_CH(ch) (DMA_VPORT_BASE + (ch - DMA_VF_START) * 0x00000100)
#define DMA_SRC(base) (base + 0x001C)
#define DMA_DST(base) (base + 0x0020)
#define DMA_LEN2(base) (base + 0x0028)
#define DMA_JUMP_ADDR(base) (base + 0x002C)
#define DMA_LEN1(base) (base + 0x0024)
#define DMA_CON(base) (base + 0x0018)
#define DMA_START(base)	(base + 0x0008)
#define DMA_FLUSH(base) (base + 0x0014)
#define DMA_INT_FLAG(base) (base + 0x0000)
#define DMA_INT_EN(base) (base + 0x0004)
#define DMA_MEM_ADDR(base) (base + 0x001C)
#define DMA_VFF_WPT(base) (base + 0x002C)
#define DMA_VFF_RPT(base) (base + 0x0030)
#define DMA_VFF_VALID_SIZE(base) (base + 0x003C)
#define DMA_VFF_THRE(base) (base + 0x0028)
#define DMA_VFF_LEN(base) (base + 0x024)

#endif








#include <net/irda/irda.h>
#include <net/irda/qos.h>
#include <net/irda/irlap.h>
#include <net/irda/irda_device.h>
#define  SIOCGERRSTAT	(SIOCGQOS +1)
#define IOCTL_SIR_DIV_16 0x01
#define IOCTL_SIR_1_61   0x02
#define IOCTL_BOF_C0H	0x04
#define IOCTL_BOF_FFH	0x08
#define IOCTL_HAVEHARDWARE 0x10
#define IOCTL_NOHARDWARE 0x20
struct option_array
{
	const char *option;
	void (*parse)(const char *value);	
};
struct mt_irda_framer
{
		struct net_device *netdev;
		struct irlap_cb *irlap;
		struct qos_info qos;
		int direction;
		void *buff;
		dma_addr_t buff_dma;
#ifdef MT6573
		struct mt_dma_conf *dma;
#elif defined(MT6575)
		struct irda_dma_conf *dma;
#endif
		spinlock_t lock;
		__u32 speed;
		__u32 new_speed;
		int rx_size;
		int tx_size;
		
		int sir_mode;
#define	IR_DIV_16 0x00
#define	IR_1_61_us	0x02
		
		int rx_invert;
#define IR_RX_EN_INVERT 0x0002   /*invert rx signal*/
		int tx_invert;
#define IR_TX_EN_INVERT	0x04
		int bof_type;
#define BOF_TYPE_C0H	(1<<7)
#define BOF_TYPE_FFH	(0<<7)
		
		int mir_576_tune;
#define IR_TX_MIR576_NONE       	0x00	
#define IR_TX_MIR576_RX_HALF      (1<<3)		
#define IR_TX_MIR576_RX_QUTER     (1<<4)
		
		int sir_framing_set;
#define SIR_FRAMING_IGN_STOP_BIT	(0<<1)
#define SIR_FRAMING_CHK_STOP_BIT	(1<<1)
		int err_stat;
	};
//#define IRDA_SIR_3_DIV_16
//#define IRDA_SIR_BOF_0XC0
//#define IRDA_MIR_CHANGE_STA

/* Transmit */

#define IR_BUF 	            (IRDA_base+0x0)
#define IR_BUF_CLEAR          (IRDA_base+0x04)
/* Control */
#define IR_MAX_T 	            (IRDA_base+0x08)
#define IR_MIN_T 	            (IRDA_base+0x0c)
#define IR_BOFS 	            (IRDA_base+0x10)
#define IR_DIV     	         (IRDA_base+0x14)
#define IR_TX_FRAME_SIZE      (IRDA_base+0x18)
#define IR_RX_FRAME1_SIZE      (IRDA_base+0x1c) /*read only*/
#define IR_ABORT	            (IRDA_base+0x20)
#define IR_TX_EN	            (IRDA_base+0x24)
#define IR_RX_EN	            (IRDA_base+0x28)
/*Interrupt*/
#define IR_INTTRIGGER         (IRDA_base+0x2c)
#define IR_IRQ_ENABLE         (IRDA_base+0x30)
#define IR_IRQ_STA            (IRDA_base+0x34)
#define IR_LSR                (IRDA_base+0x38)
#define IR_TRANSCEIVER_PDN    (IRDA_base+0x3c)

#define IR_RX_FRAME_MAX       (IRDA_base+0x40)
#define IR_DISCONNECT_TIME    (IRDA_base+0x44)
#define IR_COUNT_ENABLE       (IRDA_base+0x48)
#define IR_RATE               (IRDA_base+0x4c)
#define IR_RATE_FIX           (IRDA_base+0x50)
#define IR_FRAME1_STA         (IRDA_base+0x54)
#define IR_FRAME2_STA         (IRDA_base+0x58)
#define IR_RX_FRAME2_SIZE     (IRDA_base+0x5c)

#define IR_MODE               (IRDA_base+0x60)
#define IR_FIFO_STA           (IRDA_base+0x64)
#define IR_FIR_DIV						(IRDA_base+0x68)

/*=================Bit Definition=============================*/
#define IR_DATA_MASK          0x00ff
#define IR_MAX_T_MASK         0x3fff
#define IR_MIN_T_MASK         0xffff

#define IR_DIV_MASK       0xffff
#define IR_TXFRAME_SIZE_MASK  0x0fff
#define IR_RXFRAME_SIZE_MASK  0x0fff
#define IR_INTTRIGGER_TXMASK  0x0003
#define IR_INTTRIGGER_RXMASK  0x000c

/*IR_INTTRIGGER*/
//#define IR_INTTRIGGER_NORMAL  0x0000   /*TX_TRIG = 2, RX_TRIG=2*/
#define IR_INTTRIGGER_NORMAL  0x0003   /*TX_TRIG = 2, RX_TRIG=2*/

/*FIFO clear*/
#define IR_BUF_CLEAR_BIT      0x0001

/*IR_BOFS*/
#define IR_BOFS_MASK          0x007f
#if 0
#if defined(IRDA_SIR_BOF_0XC0)
   #define IR_BOFS_INIVALUE      0x008b
#else
#define IR_BOFS_INIVALUE      0x000b
#endif
#endif
/*IR_ABORT*/
#define IR_ABORT_BIT          0x0001   /*Trx enable*/

/*IR_TX_EN*/
#define IR_TX_EN_BIT           0x0001   /*Trx enable*/
#define IR_TX_EN_MODE          0x0002   /*0:3/16 , 1: 1.61us*/

#define IR_TX_EN_ONESHIT       0x0008   /*HW auto clear tx_en, when frame is sent*/
#if 0
#if defined(IRDA_SIR_3_DIV_16)
   #define IR_TX_EN_NORMAL        0x0001   /*3/16, don't invert signal*/
#else
   #define IR_TX_EN_NORMAL        0x0003   /*1.61us, don't invert signal*/
#endif
#endif
#define IR_TX_EN_DISABLE       0x0000
/*HW 6228/6229 ECO to cover Vishay transicver*/
#define IR_TX_MIR576_ENABLE       0x8


/*IR_RX_EN*/
#define IR_RX_EN_BIT           0x0001

#define IR_RX_EN_RXONE         0x0004   /*disable when get one frame*/
#define IR_RX_EN_NORMAL        0x0001
#define IR_RX_EN_DISABLE       0x0000

/*IR_IRQ_ENABLE*/
#define IR_IRQ_ENABLE_TXTRIG        0x0001   /*x*/
#define IR_IRQ_ENABLE_RXTRIG        0x0002   /*x*/
#define IR_IRQ_ENABLE_ERROR         0x0004
#define IR_IRQ_ENABLE_TXCOMPLETE    0x0008
#define IR_IRQ_ENABLE_RXCOMPLETE    0x0010
#define IR_IRQ_ENABLE_MINTOUT       0x0020         /*x*/
#define IR_IRQ_ENABLE_MAXTOUT       0x0040         /*x*/
#define IR_IRQ_ENABLE_RXABORT       0x0080
#define IR_IRQ_ENABLE_TXABORT       0x0100
#define IR_IRQ_ENABLE_FIFOTOUT      0x0200   /*x*/
#define IR_IRQ_ENABLE_THRESHTOUT    0x0400         /*x*/

#define IR_IRQ_ENABLE_NORMAL        0x019e   /*Disable all timer*/
#define IR_IRQ_ENABLE_DMANORMAL     0x01dc
#define IR_IRQ_RTX		0x0dc
/*IR_STA*/
#define IR_STA_TXTRIG        0x0001
#define IR_STA_RXTRIG        0x0002
#define IR_STA_LSR           0x0004
#define IR_STA_TXCOMPLETE    0x0008
#define IR_STA_RXCOMPLETE    0x0010
#define IR_STA_2ND_RXCOMPLETE 0x1000  
#define IR_STA_MINTOUT       0x0020
#define IR_STA_MAXTOUT       0x0040
#define IR_STA_RXABORT       0x0080
#define IR_STA_TXABORT       0x0100
#define IR_STA_FIFOTOUT      0x0200
#define IR_STA_THRESHTOUT    0x0400

/*IR_ERROR*/
#define IR_LSR_RXSIZE         0x0001
#define IR_LSR_OVERRUN        0x0002
#define IR_FRAME1_ERROR       0x0010
#define IR_FRAME2_ERROR       0x0020
#define IR_TX_UNDERRUN        0x0040

#define IR_FRAME_FRAMING      0x0001
#define IR_FRAME_CRCFAIL      0x0002
#define IR_FRAME_PF			      0x0004
#define IR_FRAME_UNKNOW	      0x0008
#define IR_FRAME_MIR      		0x0010
#define IR_FRAME_FIR_4PPM  		0x0020
#define IR_FRAME_FIR_STO      0x0040
/*Power on*/
#define IR_POWER_ON           0x0000
#define IR_POWER_OFF          0x0001

#define IR_CRC_REPORT 	      0x0004

extern struct mt_irda_framer *framer_dev;
extern void irda_dma_callback(void *dev);
extern void irda_dma_rx_config(struct mt_irda_framer *framer);
extern void irda_dma_tx_config(struct mt_irda_framer *framer);
//void irda_dma_callback(void *dev);
extern void mt_irda_dev_close(void);
extern void mt_irda_dev_open(void);
extern void irda_irq_init(void);
#endif   /*_IRFRAMER_HW_H*/

