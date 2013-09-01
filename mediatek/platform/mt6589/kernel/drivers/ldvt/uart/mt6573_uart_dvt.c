
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/tcm.h>

#include <mach/mt6573_reg_base.h>
#include <mach/irqs.h>
#include <mach/mt6573_eint.h>
#include <mach/mt6573_gpio.h>

#include "mt6573_uart_dvt.h"

#define UART_DEBUG	0

#define UART_NAME	"uvvp_uart_ext"

#define UART_SAY	"[uart_udvt]: "

#if UART_DEBUG
#define uart_print(fmt, arg...)	printk(UART_SAY fmt, ##arg)
#else
#define uart_print(fmt, arg...)	do {} while (0)
#endif

struct udvt_cmd {
	unsigned int cmd;
	unsigned int data[100];
};

/* Command Type */
enum{
	UART_READ_REG=1,
	UART_WRITE_REG,
	UART_SET_REG,
	UART_CLR_REG,
	UART_CFG_TXRX_PIN,
	UART_CFG_RTSCTS_PIN,
	UART_DISABLE_UART_IRQ,
	UART_RESTORE_UART_IRQ,
	UART_DISABLE_TX_VFF_IRQ,
	UART_RESTORE_TX_VFF_IRQ,
	UART_DISABLE_RX_VFF_IRQ,
	UART_RESTORE_RX_VFF_IRQ,
	UART_BACK_UP_UART_REGISTER,
	UART_RESTORE_UART_REGISTER,
	UART_BACK_UP_VFF_TX_REGISTER,
	UART_RESTORE_VFF_TX_REGISTER,
	UART_BACK_UP_VFF_RX_REGISTER,
	UART_RESTORE_VFF_RX_REGISTER,
	UART_SET_FLOW_CONTROL,
	UART_SET_XON1_XOFF1,
	UART_SET_XON2_XOFF2,
	UART_SET_ESCAPE_CHAR,
	UART_ENABLE_AUTO_BAUD,
	UART_AUTO_BAUD_DONE,
	UART_READ_UART_REG_BY_OFFSET,
	UART_WRITE_UART_REG_BY_OFFSET,
	UART_SET_UART_REG_BY_OFFSET,
	UART_CLR_UART_REG_BY_OFFSET,
	UART_POLLING_UART_REG_BY_OFFSET,
	UART_READ_RX_VFF_REG_BY_OFFSET,
	UART_WRITE_RX_VFF_REG_BY_OFFSET,
	UART_SET_RX_VFF_REG_BY_OFFSET,
	UART_CLR_RX_VFF_REG_BY_OFFSET,
	UART_READ_TX_VFF_REG_BY_OFFSET,
	UART_WRITE_TX_VFF_REG_BY_OFFSET,
	UART_SET_TX_VFF_REG_BY_OFFSET,
	UART_CLR_TX_VFF_REG_BY_OFFSET,
};

/* Clock type */
enum{
	SYS_CLK_61_44M = 1,
	SYS_CLK_52M,
	SYS_CLK_30_72M,
	SYS_CLK_26M,
	SYS_CLK_15_36M,
	SYS_CLK_13M,
	SYS_CLK_7_68M,
	SYS_CLK_3_84M
};

/* Flow control type */
enum{
	FC_NONE,
	FC_RTSCTS,
	FC_XONXOFF_1,
	FC_XONXOFF_2,
	FC_XONXOFF_12,
	FC_XONXOFF_1_ESC_EN,
	FC_XONXOFF_2_ESC_EN,
	FC_XONXOFF_12_ESC_EN,
};

struct op_mapping{
	int index;
	long (*func)(int*, unsigned int*);	/* Parameter: err_no , in/out buffer */
	int uart_bauded;
};

enum{
	UART_ID_1 = 1,
	UART_ID_2,
	UART_ID_3,
	UART_ID_4,
	UART_ID_MAX,
};
	
static UART_REG_LIST    uart_registers_back_up;
static VFF_TX_REG_LIST  vff_tx_registers_back_up;
static VFF_RX_REG_LIST  vff_rx_registers_back_up;

static unsigned int uart_base[]={UART1_BASE_ADDR, UART2_BASE_ADDR, UART3_BASE_ADDR ,UART4_BASE_ADDR};
static unsigned int vff_tx_base[]={VFF_TX1_BASE_ADDR, VFF_TX2_BASE_ADDR, VFF_TX3_BASE_ADDR, VFF_TX4_BASE_ADDR};
static unsigned int vff_rx_base[]={VFF_RX1_BASE_ADDR, VFF_RX2_BASE_ADDR, VFF_RX3_BASE_ADDR, VFF_RX4_BASE_ADDR};
//static unsigned int uart_irq_num[]={UART1_IRQ_ID, UART2_IRQ_ID, UART3_IRQ_ID, UART4_IRQ_ID};

/* buff[0]: register address, buff[1]: value */
static long uart_read_register(int *err_id, u32 buff[])
{
	unsigned int register_addr = buff[0];
	if(0 == register_addr)
		return -EINVAL;
	buff[1] = *((volatile u32*)register_addr);
	return 0;
}

/* buff[0]: register address, buff[1]: value */
static long uart_write_register(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 register_addr = buff[0];
	if(0 == register_addr)
		return -EINVAL;
	o_register_value = *((volatile u32*)register_addr);
	*((volatile u32*)register_addr) = buff[1];
	buff[1] = o_register_value;
	return 0;
}

/* buff[0]: register address, buff[1]: value */
static long uart_set_register(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 register_addr = buff[0];
	if(0 == register_addr)
		return -EINVAL;
	o_register_value = *((volatile u32*)register_addr);
	*((volatile u32*)register_addr) = buff[1]|o_register_value;
	buff[1] = o_register_value;
	return 0;
}

/* buff[0]: register address, buff[1]: value */
static long uart_clr_register(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 register_addr = buff[0];
	if(0 == register_addr)
		return -EINVAL;
	o_register_value = *((volatile u32*)register_addr);
	*((volatile u32*)register_addr) = (~buff[1])&o_register_value;
	buff[1] = o_register_value;
	return 0;
}

static long uart_configure_txrx_pin(int *err_id, u32 buff[])
{
	u32 uart_id = buff[0];
	switch(uart_id)
	{
	case 1: /* Uart 1 */
	        if(mt_set_gpio_mode( UART1_TX_GPIO_PIN,  UART1_TX_GPIO_PIN_M))
			return -EINVAL;
		if(mt_set_gpio_mode( UART1_RX_GPIO_PIN,  UART1_RX_GPIO_PIN_M))
			return -EINVAL;
  		return 0;
	case 2: /* Uart 2 */
		if(mt_set_gpio_mode( UART2_TX_GPIO_PIN,  UART2_TX_GPIO_PIN_M))
			return -EINVAL;
		if(mt_set_gpio_mode( UART2_RX_GPIO_PIN,  UART2_RX_GPIO_PIN_M))
			return -EINVAL;
		return 0;
	case 3: /* Uart 3 */
		if(mt_set_gpio_mode( UART3_TX_GPIO_PIN,  UART3_TX_GPIO_PIN_M))
			return -EINVAL;
		if(mt_set_gpio_mode( UART3_RX_GPIO_PIN,  UART3_RX_GPIO_PIN_M))
			return -EINVAL;
		return 0;
	case 4: /* Uart 4 */
		if(mt_set_gpio_mode( UART4_TX_GPIO_PIN,  UART4_TX_GPIO_PIN_M))
			return -EINVAL;
		if(mt_set_gpio_mode( UART4_RX_GPIO_PIN,  UART4_RX_GPIO_PIN_M))
			return -EINVAL;
		return 0;
	default:
		*err_id = -UART_DRV_EXT_ERR_INVALID_UART_ID;
		return -EINVAL;
	}
}

static long uart_configure_rtscts_pin(int *err_id, u32 buff[])
{
	u32 uart_id = buff[0];
	switch(uart_id)
	{
	case 1: /* Uart 1 */
		if(mt_set_gpio_mode( UART1_RTS_GPIO_PIN,  UART1_RTS_GPIO_PIN_M))
			return -EINVAL;
		if(mt_set_gpio_mode( UART1_CTS_GPIO_PIN,  UART1_CTS_GPIO_PIN_M))
			return -EINVAL;
		return 0;
	case 2: /* Uart 2 */
		if(mt_set_gpio_mode( UART2_RTS_GPIO_PIN,  UART2_RTS_GPIO_PIN_M))
			return -EINVAL;
		if(mt_set_gpio_mode( UART2_CTS_GPIO_PIN,  UART2_CTS_GPIO_PIN_M))
			return -EINVAL;
		return 0;
	case 3: /* Uart 3 */
		if(mt_set_gpio_mode( UART3_RTS_GPIO_PIN,  UART3_RTS_GPIO_PIN_M))
			return -EINVAL;
		if(mt_set_gpio_mode( UART3_CTS_GPIO_PIN,  UART3_CTS_GPIO_PIN_M))
			return -EINVAL;
		return 0;
	case 4: /* Uart 4 */
		if(mt_set_gpio_mode( UART4_RTS_GPIO_PIN,  UART4_RTS_GPIO_PIN_M))
			return -EINVAL;
		if(mt_set_gpio_mode( UART4_CTS_GPIO_PIN,  UART4_CTS_GPIO_PIN_M))
			return -EINVAL;
		return 0;
	default:
		*err_id = -UART_DRV_EXT_ERR_INVALID_UART_ID;
		return -EINVAL;
	}
}

static long uart_disable_uart_irq(int *err_id, u32 buff[])
{
	u32 uart_id = buff[0];
	switch(uart_id)
	{
	case 1: /* Uart 1 */
		disable_irq(UART1_IRQ_ID);
		return 0;
  	case 2: /* Uart 2 */
		disable_irq(UART2_IRQ_ID);
		return 0;
  	case 3: /* Uart 3 */
		disable_irq(UART3_IRQ_ID);
		return 0;
  	case 4: /* Uart 4 */
		disable_irq(UART4_IRQ_ID);
		return 0;
  	default:
		*err_id = -UART_DRV_EXT_ERR_INVALID_UART_ID;
		return -EINVAL;
	}
}

static long uart_enable_uart_irq(int *err_id, u32 buff[])
{
	u32 uart_id = buff[0];
	switch(uart_id)
	{
	case 1: /* Uart 1 */
		enable_irq(UART1_IRQ_ID);
		return 0;
  	case 2: /* Uart 2 */
        	enable_irq(UART2_IRQ_ID);
  		return 0;
  	case 3: /* Uart 3 */
        	enable_irq(UART3_IRQ_ID);
  		return 0;
  	case 4: /* Uart 4 */
        	enable_irq(UART4_IRQ_ID);
  		return 0;
  	default:
		*err_id = -UART_DRV_EXT_ERR_INVALID_UART_ID;
		return -EINVAL;
	}
}

static long uart_disable_tx_vff_irq(int *err_id, u32 buff[])
{
	u32 uart_id = buff[0];
	unsigned int base;
	switch(uart_id)
	{
	case 1: /* Uart 1 */
		base = vff_tx_base[0];
		break;
	case 2: /* Uart 2 */
		base = vff_tx_base[1];
		break;
	case 3: /* Uart 3 */
		base = vff_tx_base[2];
		break;
	case 4: /* Uart 4 */
		base = vff_tx_base[3];
		break;
	default:
		*err_id = -UART_DRV_EXT_ERR_INVALID_UART_ID;
		return -EINVAL;
	}
	*((volatile u32*)VFF_TX_INT_EN(base)) = 0;
	return 0;
}

static long uart_enable_tx_vff_irq(int *err_id, u32 buff[])
{
	u32 uart_id = buff[0];
	switch(uart_id)
	{
	case 1: /* Uart 1 */
  		return 0;
  	case 2: /* Uart 2 */
  		return 0;
  	case 3: /* Uart 3 */
  		return 0;
  	case 4: /* Uart 4 */
  		return 0;
	default:
		*err_id = -UART_DRV_EXT_ERR_INVALID_UART_ID;
		return -EINVAL;
	}
}

static long uart_disable_rx_vff_irq(int *err_id, u32 buff[])
{
	u32 uart_id = buff[0];
	unsigned int base;
	switch(uart_id)
	{
	case 1: /* Uart 1 */
		base = vff_rx_base[0];
		break;
	case 2: /* Uart 2 */
		base = vff_rx_base[1];
		break;
	case 3: /* Uart 3 */
		base = vff_rx_base[2];
		break;
	case 4: /* Uart 4 */
		base = vff_rx_base[3];
		break;
	default:
		return -EINVAL;
	}
	*((volatile u32*)VFF_RX_INT_EN(base)) = 0;
	return 0;
}

static long uart_enable_rx_vff_irq(int *err_id, u32 buff[])
{
	u32 uart_id = buff[0];
	switch(uart_id)
	{
	case 1: /* Uart 1 */
		return 0;
	case 2: /* Uart 2 */
		return 0;
	case 3: /* Uart 3 */
		return 0;
	case 4: /* Uart 4 */
		return 0;
	default:
		*err_id = -UART_DRV_EXT_ERR_INVALID_UART_ID;
		return -EINVAL;
	}
}

static long uart_back_up_uart_registers(int *err_id, u32 buff[])
{
	unsigned int base;

	base = uart_base[buff[0] - 1];
	/* Disable Uart IRQ */
	uart_disable_uart_irq(err_id, buff);

	/* Begin to back up uart registers one by one */
	uart_registers_back_up.UART_RBR = 0; /* This register no need to back up */
	uart_registers_back_up.UART_THR = 0; /* This register no need to back up */
	uart_registers_back_up.UART_IER = *((volatile u32*)UART_IER(base));
	uart_registers_back_up.UART_IIR = 0; /* This register no need to back up */
	uart_registers_back_up.UART_FCR = 0; /* This register is a write only regiter */
	uart_registers_back_up.UART_LCR = *((volatile u32*)UART_LCR(base));
	uart_registers_back_up.UART_MCR = *((volatile u32*)UART_MCR(base));
	uart_registers_back_up.UART_LSR = *((volatile u32*)UART_LSR(base));
	uart_registers_back_up.UART_MSR = *((volatile u32*)UART_MSR(base));
	uart_registers_back_up.UART_SCR = *((volatile u32*)UART_SCR(base));
	/* The following regiter need LCR[7] = 1 */
	*((volatile u32*)UART_LCR(base)) = 0x80;
	uart_registers_back_up.UART_DLL =*((volatile u32*)UART_DLL(base));
	uart_registers_back_up.UART_DLH =*((volatile u32*)UART_DLH(base));
	/* The following register need LCR = 0xBF */
	*((volatile u32*)UART_LCR(base)) = 0xBF;
	uart_registers_back_up.UART_EFR =*((volatile u32*)UART_EFR(base));
	uart_registers_back_up.UART_XON1 =*((volatile u32*)UART_XON1(base));
	uart_registers_back_up.UART_XON2 =*((volatile u32*)UART_XON2(base));
	uart_registers_back_up.UART_XOFF1 =*((volatile u32*)UART_XOFF1(base));
	uart_registers_back_up.UART_XOFF2 =*((volatile u32*)UART_XOFF2(base));
	/* Back to normal mode */
	*((volatile u32*)UART_LCR(base)) = uart_registers_back_up.UART_LCR;
	uart_registers_back_up.UART_AUTOBAUD_EN =*((volatile u32*)UART_AUTOBAUD_EN(base));
	uart_registers_back_up.UART_HIGHSPEED =*((volatile u32*)UART_HIGHSPEED(base));
	uart_registers_back_up.UART_SAMPLE_COUNT =*((volatile u32*)UART_SAMPLE_COUNT(base));
	uart_registers_back_up.UART_SAMPLE_POINT =*((volatile u32*)UART_SAMPLE_POINT(base));
	uart_registers_back_up.UART_AUTOBAUD_REG =0; /* This register has been removed */
	uart_registers_back_up.UART_RATE_FIX_AD =*((volatile u32*)UART_RATE_FIX_AD(base));
	uart_registers_back_up.UART_AUTOBAUD_SAMPLE =*((volatile u32*)UART_AUTOBAUD_SAMPLE(base));
	uart_registers_back_up.UART_GUARD =*((volatile u32*)UART_GUARD(base));
	uart_registers_back_up.UART_ESCAPE_DAT =*((volatile u32*)UART_ESCAPE_DAT(base));
	uart_registers_back_up.UART_ESCAPE_EN =*((volatile u32*)UART_ESCAPE_EN(base));
	uart_registers_back_up.UART_SLEEP_EN =*((volatile u32*)UART_SLEEP_EN(base));
	uart_registers_back_up.UART_DMA_EN =*((volatile u32*)UART_DMA_EN(base));
	uart_registers_back_up.UART_RXTRI_AD =*((volatile u32*)UART_RXTRI_AD(base));
	uart_registers_back_up.UART_FRACDIV_L =*((volatile u32*)UART_FRACDIV_L(base));
	uart_registers_back_up.UART_FRACDIV_M =*((volatile u32*)UART_FRACDIV_M(base));
	uart_registers_back_up.UART_FCR_RD =*((volatile u32*)UART_FCR_RD(base));
	uart_registers_back_up.UART_ACTIVE_EN = 0; /* This register not exist in MT6573 */

	/* Restore UART IRQ */
	uart_enable_uart_irq(err_id, buff);
	return 0;
}

static long uart_restore_uart_registers(int *err_id, u32 buff[])
{
	unsigned int base;

	base = uart_base[buff[0] - 1];
	/* Disable Uart IRQ */
	uart_disable_uart_irq(err_id, buff);

	/* Begin to back up uart registers one by one */
	uart_registers_back_up.UART_RBR = 0; /* This register no need to restore */
	uart_registers_back_up.UART_THR = 0; /* This register no need to restore */
	*((volatile u32*)UART_IER(base)) = uart_registers_back_up.UART_IER;
	uart_registers_back_up.UART_IIR = 0; /* This register no need to restore */
	*((volatile u32*)UART_FCR(base)) = (2<<6)|(1<<4)|1; /* This register is a write only regiter,use driver setting */
	*((volatile u32*)UART_LCR(base)) = uart_registers_back_up.UART_LCR;
	*((volatile u32*)UART_MCR(base)) = uart_registers_back_up.UART_MCR;
	*((volatile u32*)UART_LSR(base)) = uart_registers_back_up.UART_LSR;
	*((volatile u32*)UART_MSR(base)) = uart_registers_back_up.UART_MSR;
	*((volatile u32*)UART_SCR(base)) = uart_registers_back_up.UART_SCR;
	/* The following regiter need LCR[7] = 1 */
	*((volatile u32*)UART_LCR(base)) = 0x80;
	*((volatile u32*)UART_DLL(base)) = uart_registers_back_up.UART_DLL;
	*((volatile u32*)UART_DLH(base)) = uart_registers_back_up.UART_DLH;
	/* The following register need LCR = 0xBF */
	*((volatile u32*)UART_LCR(base)) = 0xBF;
	*((volatile u32*)UART_EFR(base)) = uart_registers_back_up.UART_EFR;
	*((volatile u32*)UART_XON1(base)) = uart_registers_back_up.UART_XON1;
	*((volatile u32*)UART_XON2(base)) = uart_registers_back_up.UART_XON2;
	*((volatile u32*)UART_XOFF1(base)) = uart_registers_back_up.UART_XOFF1;
	*((volatile u32*)UART_XOFF2(base)) = uart_registers_back_up.UART_XOFF2;
	/* Back to normal mode */
	*((volatile u32*)UART_LCR(base)) = uart_registers_back_up.UART_LCR;
	*((volatile u32*)UART_AUTOBAUD_EN(base)) = uart_registers_back_up.UART_AUTOBAUD_EN;
	*((volatile u32*)UART_HIGHSPEED(base)) = uart_registers_back_up.UART_HIGHSPEED;
	*((volatile u32*)UART_SAMPLE_COUNT(base)) = uart_registers_back_up.UART_SAMPLE_COUNT;
	*((volatile u32*)UART_SAMPLE_POINT(base)) = uart_registers_back_up.UART_SAMPLE_POINT;
	uart_registers_back_up.UART_AUTOBAUD_REG =0; /* This register has been removed */
	*((volatile u32*)UART_RATE_FIX_AD(base)) = uart_registers_back_up.UART_RATE_FIX_AD;
	*((volatile u32*)UART_AUTOBAUD_SAMPLE(base)) = uart_registers_back_up.UART_AUTOBAUD_SAMPLE;
	*((volatile u32*)UART_GUARD(base)) = uart_registers_back_up.UART_GUARD;
	*((volatile u32*)UART_ESCAPE_DAT(base)) = uart_registers_back_up.UART_ESCAPE_DAT;
	*((volatile u32*)UART_ESCAPE_EN(base)) = uart_registers_back_up.UART_ESCAPE_EN;
	*((volatile u32*)UART_SLEEP_EN(base)) = uart_registers_back_up.UART_SLEEP_EN;
	*((volatile u32*)UART_DMA_EN(base)) = uart_registers_back_up.UART_DMA_EN;
	*((volatile u32*)UART_RXTRI_AD(base)) = uart_registers_back_up.UART_RXTRI_AD;
	*((volatile u32*)UART_FRACDIV_L(base)) = uart_registers_back_up.UART_FRACDIV_L;
	*((volatile u32*)UART_FRACDIV_M(base)) = uart_registers_back_up.UART_FRACDIV_M;
	*((volatile u32*)UART_FCR_RD(base)) = uart_registers_back_up.UART_FCR_RD;
	uart_registers_back_up.UART_ACTIVE_EN = 0; /* This register not exist in MT6573 */
	*((volatile u32*)UART_LCR(base)) = uart_registers_back_up.UART_LCR; /* Restore it again */

	/* Restore UART IRQ */
	uart_enable_uart_irq(err_id, buff);
	return 0;
}

static long uart_back_up_vff_tx_registers(int *err_id, u32 buff[])
{
	unsigned int base;

	base = vff_tx_base[buff[0] - 1];
	vff_tx_registers_back_up.VFF_TX_INT_FLAG = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_INT_EN = *((volatile u32*)VFF_TX_INT_EN(base));
	vff_tx_registers_back_up.VFF_TX_EN = *((volatile u32*)VFF_TX_EN(base));
	vff_tx_registers_back_up.VFF_TX_RST = *((volatile u32*)VFF_TX_RST(base));
	vff_tx_registers_back_up.VFF_TX_STOP = *((volatile u32*)VFF_TX_STOP(base));
	vff_tx_registers_back_up.VFF_TX_FLUSH = *((volatile u32*)VFF_TX_FLUSH(base));
	vff_tx_registers_back_up.VFF_TX_ADDR = *((volatile u32*)VFF_TX_ADDR(base));
	vff_tx_registers_back_up.VFF_TX_LEN = *((volatile u32*)VFF_TX_LEN(base));
	vff_tx_registers_back_up.VFF_TX_THRE = *((volatile u32*)VFF_TX_THRE(base));
	vff_tx_registers_back_up.VFF_TX_WPT = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_RPT = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_W_INT_BUF_SIZE = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_INT_BUF_SIZE = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_VALID_SIZE = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_LEFT_SIZE = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_DEBUG_STATUS = 0; /* Read Only register */
	return 0;
}
static long uart_restore_vff_tx_registers(int *err_id, u32 buff[])
{
	unsigned int base;

	base = vff_tx_base[buff[0] - 1];
	vff_tx_registers_back_up.VFF_TX_INT_FLAG = 0; /* Read Only register */
	*((volatile u32*)VFF_TX_INT_EN(base)) = vff_tx_registers_back_up.VFF_TX_INT_EN;
	*((volatile u32*)VFF_TX_RST(base)) = vff_tx_registers_back_up.VFF_TX_RST;
	*((volatile u32*)VFF_TX_STOP(base)) = vff_tx_registers_back_up.VFF_TX_STOP;
	*((volatile u32*)VFF_TX_FLUSH(base)) = vff_tx_registers_back_up.VFF_TX_FLUSH;
	*((volatile u32*)VFF_TX_ADDR(base)) = vff_tx_registers_back_up.VFF_TX_ADDR;
	*((volatile u32*)VFF_TX_LEN(base)) = vff_tx_registers_back_up.VFF_TX_LEN;
	*((volatile u32*)VFF_TX_THRE(base)) = vff_tx_registers_back_up.VFF_TX_THRE;
	vff_tx_registers_back_up.VFF_TX_WPT = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_RPT = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_W_INT_BUF_SIZE = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_INT_BUF_SIZE = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_VALID_SIZE = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_LEFT_SIZE = 0; /* Read Only register */
	vff_tx_registers_back_up.VFF_TX_DEBUG_STATUS = 0; /* Read Only register */
	*((volatile u32*)VFF_TX_EN(base)) = vff_tx_registers_back_up.VFF_TX_EN;
	return 0;
}
static long uart_back_up_vff_rx_registers(int *err_id, u32 buff[])
{
	unsigned int base;

	base = vff_rx_base[buff[0] - 1];
	vff_rx_registers_back_up.VFF_RX_INT_FLAG = 0; /* Read Only register */
	vff_rx_registers_back_up.VFF_RX_INT_EN = *((volatile u32*)VFF_RX_INT_EN(base));
	vff_rx_registers_back_up.VFF_RX_EN = *((volatile u32*)VFF_RX_EN(base));
	vff_rx_registers_back_up.VFF_RX_RST = *((volatile u32*)VFF_RX_RST(base));
	vff_rx_registers_back_up.VFF_RX_STOP = *((volatile u32*)VFF_RX_STOP(base));
	vff_rx_registers_back_up.VFF_RX_FLUSH = *((volatile u32*)VFF_RX_FLUSH(base));
	vff_rx_registers_back_up.VFF_RX_ADDR = *((volatile u32*)VFF_RX_ADDR(base));
	vff_rx_registers_back_up.VFF_RX_LEN = *((volatile u32*)VFF_RX_LEN(base));
	vff_rx_registers_back_up.VFF_RX_THRE = *((volatile u32*)VFF_RX_THRE(base));
	vff_rx_registers_back_up.VFF_RX_WPT = 0; /* Read Only register */
	vff_rx_registers_back_up.VFF_RX_RPT = *((volatile u32*)VFF_RX_RPT(base));
	vff_rx_registers_back_up.VFF_RX_FC_THRSHOLD = *((volatile u32*)VFF_RX_FC_THRSHOLD(base));
	vff_rx_registers_back_up.VFF_RX_INT_BUF_SIZE = 0; /* Read Only register */
	vff_rx_registers_back_up.VFF_RX_VALID_SIZE = 0; /* Read Only register */
	vff_rx_registers_back_up.VFF_RX_LEFT_SIZE = 0; /* Read Only register */
	return 0;
}
static long uart_restore_vff_rx_registers(int *err_id, u32 buff[])
{
	unsigned int base;

	base = vff_rx_base[buff[0] - 1];
	vff_rx_registers_back_up.VFF_RX_INT_FLAG = 0; /* Read Only register */
	*((volatile u32*)VFF_RX_INT_EN(base)) = vff_rx_registers_back_up.VFF_RX_INT_EN;
	*((volatile u32*)VFF_RX_RST(base)) = vff_rx_registers_back_up.VFF_RX_RST;
	*((volatile u32*)VFF_RX_STOP(base)) = vff_rx_registers_back_up.VFF_RX_STOP;
	*((volatile u32*)VFF_RX_FLUSH(base)) = vff_rx_registers_back_up.VFF_RX_FLUSH;
	*((volatile u32*)VFF_RX_ADDR(base)) = vff_rx_registers_back_up.VFF_RX_ADDR;
	*((volatile u32*)VFF_RX_LEN(base)) = vff_rx_registers_back_up.VFF_RX_LEN;
	*((volatile u32*)VFF_RX_THRE(base)) = vff_rx_registers_back_up.VFF_RX_THRE;
	vff_rx_registers_back_up.VFF_RX_WPT = 0; /* Read Only register */
	*((volatile u32*)VFF_RX_RPT(base)) = vff_rx_registers_back_up.VFF_RX_RPT;
	*((volatile u32*)VFF_RX_FC_THRSHOLD(base)) = vff_rx_registers_back_up.VFF_RX_FC_THRSHOLD;
	vff_rx_registers_back_up.VFF_RX_INT_BUF_SIZE = 0; /* Read Only register */
	vff_rx_registers_back_up.VFF_RX_VALID_SIZE = 0; /* Read Only register */
	vff_rx_registers_back_up.VFF_RX_LEFT_SIZE = 0; /* Read Only register */
	*((volatile u32*)VFF_RX_EN(base)) = vff_rx_registers_back_up.VFF_RX_EN;
	return 0;
}

static long uart_enable_auto_baud(int *err_id, u32 buff[])
{
	unsigned int base;
	unsigned int sys_clk = buff[1];

	base = uart_base[buff[0] - 1];

	/* Set System clock and auto baud sample */
	switch(sys_clk)
	{
	case SYS_CLK_61_44M:
		*((volatile u32*)UART_RATE_FIX_AD(base)) = (1<<4)|(1<<3); /* 61.44M Clock, 61M domain */
		*((volatile u32*)UART_AUTOBAUD_SAMPLE(base)) = 31;
		break;
	case SYS_CLK_52M:
		*((volatile u32*)UART_AUTOBAUD_SAMPLE(base)) = 27;
		return -EINVAL;	/* MT6573 using 61M domain */
	case SYS_CLK_30_72M:
		*((volatile u32*)UART_RATE_FIX_AD(base)) = (1<<4)|(1<<3)|3; /* 61.44M Clock, 61M domain */
		*((volatile u32*)UART_AUTOBAUD_SAMPLE(base)) = 15;
		break;
	case SYS_CLK_26M:
		*((volatile u32*)UART_AUTOBAUD_SAMPLE(base)) = 13;
		return -EINVAL;	/* MT6573 using 61M domain */
	case SYS_CLK_15_36M:
		*((volatile u32*)UART_RATE_FIX_AD(base)) = (1<<4)|(1<<3)|(1<<2)|3; /* 61.44M Clock, 61M domain */
		*((volatile u32*)UART_AUTOBAUD_SAMPLE(base)) = 7;
		break;
	case SYS_CLK_13M:
		*((volatile u32*)UART_AUTOBAUD_SAMPLE(base)) = 6;
		return -EINVAL;	/* MT6573 using 61M domain */
	case SYS_CLK_7_68M:
		return -EINVAL;
	case SYS_CLK_3_84M:
		return -EINVAL;
	default:
		return -EINVAL;
	}
	
	/* Enable Auto baud */
	*((volatile u32*)UART_AUTOBAUD_EN(base)) = 1;
	return 0;
}

static long uart_auto_baud_check_done(int *err_id, u32 buff[])
{
	unsigned int base;

	base = uart_base[buff[0] - 1];

	buff[1] = *((volatile u32*)UART_AUTOBAUD_EN(base))&1;

	return 0;
}

static long uart_set_xon1_off1(int *err_id, u32 buff[])
{
	unsigned int base;
	unsigned int lcr;

	base = uart_base[buff[0] - 1];

	/* Disable Uart IRQ */
	uart_disable_uart_irq(err_id, buff);

	lcr = *((volatile u32*)UART_LCR(base));
	*((volatile u32*)UART_LCR(base)) = 0xBF;
	*((volatile u32*)UART_XON1(base)) = buff[1]&0xFF; /* XON */
	*((volatile u32*)UART_XOFF1(base)) = buff[2]&0xFF; /* XOFF */
	*((volatile u32*)UART_LCR(base)) = lcr;

	/* Enable Uart IRQ */
	uart_enable_uart_irq(err_id, buff);

	return 0;
}

static long uart_set_xon2_off2(int *err_id, u32 buff[])
{
	unsigned int base;
	unsigned int lcr;

	base = uart_base[buff[0] - 1];
	/* Disable Uart IRQ */
	uart_disable_uart_irq(err_id, buff);

	lcr = *((volatile u32*)UART_LCR(base));
	*((volatile u32*)UART_LCR(base)) = 0xBF;
	*((volatile u32*)UART_XON2(base)) = buff[1]&0xFF;
	*((volatile u32*)UART_XOFF2(base)) = buff[2]&0xFF;
	*((volatile u32*)UART_LCR(base)) = lcr;

	/* Enable Uart IRQ */
	uart_enable_uart_irq(err_id, buff);

	return 0;
}

static long uart_set_escape_char(int *err_id, u32 buff[])
{
	unsigned int base;

	base = uart_base[buff[0] - 1];

	*((volatile u32*)UART_ESCAPE_DAT(base)) = buff[1]&0xFF;
	return 0;
}

static long uart_enable_escape(int *err_id, u32 buff[])
{
	unsigned int base;

	base = uart_base[buff[0] - 1];

	*((volatile u32*)UART_ESCAPE_EN(base)) = buff[1]&0x1;
	return 0;
}

static long uart_set_flow_ctrl(int *err_id, u32 buff[])
{
	unsigned int base;
	unsigned int lcr;
	unsigned int efr;
	unsigned int esc_en;
	unsigned int fc = buff[1];
	long	ret = 0;

	base = uart_base[buff[0] - 1];

	/* Disable Uart IRQ */
	uart_disable_uart_irq(err_id, buff);

	lcr = *((volatile u32*)UART_LCR(base));
	*((volatile u32*)UART_LCR(base)) = 0xBF;
	efr = *((volatile u32*)UART_EFR(base));
	efr &= ~((1<<7)|(1<<6)|0xF);
	esc_en = 0;

	switch(fc)
	{
	case FC_NONE: /* None */
		break;
	case FC_RTSCTS: /* RTS CTS */
		efr |= (1<<7)|(1<<6);
		break;
	case FC_XONXOFF_1: /* XON1 XOFF1 */
		efr |= (2<<2)|2;
		break;
	case FC_XONXOFF_2: /* XON2 XOFF2 */
		efr |= (1<<2)|1;
		break;
	case FC_XONXOFF_12: /* XON12 XOFF12 */
		efr |= 0xF;
		break;
	case FC_XONXOFF_1_ESC_EN: /* XON1 XOFF1 Esc En */
		efr |= (2<<2)|2;
		esc_en = 1;
		break;
	case FC_XONXOFF_2_ESC_EN: /* XON2 XOFF2 Esc En */
		efr |= (1<<2)|1;
		esc_en = 1;
		break;
	case FC_XONXOFF_12_ESC_EN: /* XON12 XOFF12 Esc En */
		efr |= 0xF;
		esc_en = 1;
		break;
	default:
		ret = -EINVAL;
	}
	*((volatile u32*)UART_EFR(base)) = efr;
	*((volatile u32*)UART_LCR(base)) = lcr;
	*((volatile u32*)UART_ESCAPE_EN(base)) = esc_en;

	/* Enable Uart IRQ */
	uart_enable_uart_irq(err_id, buff);

	return ret;
}

static long uart_read_uart_register_by_offset(int *err_id, u32 buff[])
{
	u32 reg_addr = uart_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	buff[2] = *((volatile u32*)reg_addr);			/* result: buff[2] */
	uart_print("[UART Ext Drv] <R> Reg addr: %08x, value: %02x\r\n", reg_addr, buff[2]);
	return 0;
}

static long uart_write_uart_register_by_offset(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 reg_addr = uart_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	o_register_value = *((volatile u32*)(reg_addr));
	*((volatile u32*)(reg_addr)) = buff[2];			/* setting: buff[2] */
	buff[2] = o_register_value;
	uart_print("[UART Ext Drv] <W> Reg addr: %08x, value: %02x\r\n", reg_addr, buff[2]);
	return 0;
}

static long uart_set_uart_register_by_offset(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 reg_addr = uart_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	o_register_value = *((volatile u32*)(reg_addr));
	*((volatile u32*)(reg_addr)) = buff[2]|o_register_value;/* setting: buff[2] */
	buff[2] = o_register_value;
	uart_print("[UART Ext Drv] <S> Reg addr: %08x, value: %02x\r\n", reg_addr, buff[2]);
	return 0;
}

static long uart_clr_uart_register_by_offset(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 reg_addr = uart_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	o_register_value = *((volatile u32*)(reg_addr));
	*((volatile u32*)(reg_addr)) = (~(buff[2]))&o_register_value;/* setting: buff[2] */
	buff[2] = o_register_value;
	uart_print("[UART Ext Drv] <C> Reg addr: %08x, value: %02x\r\n", reg_addr, buff[2]);
	return 0;
}

static long uart_read_rx_vff_register_by_offset(int *err_id, u32 buff[])
{
	u32 reg_addr = vff_rx_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	buff[2] = *((volatile u32*)(reg_addr));			/* result: buff[2] */
	return 0;
}

static long uart_write_rx_vff_register_by_offset(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 reg_addr = vff_rx_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	o_register_value = *((volatile u32*)(reg_addr));
	*((volatile u32*)(reg_addr)) = buff[2];			/* setting: buff[2] */
	buff[2] = o_register_value;
	return 0;
}

static long uart_set_rx_vff_register_by_offset(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 reg_addr = vff_rx_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	o_register_value = *((volatile u32*)(reg_addr));
	*((volatile u32*)(reg_addr)) = buff[2]|o_register_value;/* setting: buff[2] */
	buff[2] = o_register_value;
	return 0;
}

static long uart_clr_rx_vff_register_by_offset(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 reg_addr = vff_rx_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	o_register_value = *((volatile u32*)(reg_addr));
	*((volatile u32*)(reg_addr)) = (~(buff[2]))&o_register_value;/* setting: buff[2] */
	buff[2] = o_register_value;
	return 0;
}

static long uart_read_tx_vff_register_by_offset(int *err_id, u32 buff[])
{
	u32 reg_addr = vff_tx_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	buff[2] = *((volatile u32*)reg_addr);			/* result: buff[2] */
	return 0;
}

static long uart_write_tx_vff_register_by_offset(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 reg_addr = vff_tx_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	o_register_value = *((volatile u32*)(reg_addr));
	*((volatile u32*)(reg_addr)) = buff[2];			/* setting: buff[2] */
	buff[2] = o_register_value;
	return 0;
}

static long uart_set_tx_vff_register_by_offset(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 reg_addr = vff_tx_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	o_register_value = *((volatile u32*)(reg_addr));
	*((volatile u32*)(reg_addr)) = buff[2]|o_register_value;/* setting: buff[2] */
	buff[2] = o_register_value;
	return 0;
}

static long uart_clr_tx_vff_register_by_offset(int *err_id, u32 buff[])
{
	u32 o_register_value;
	u32 reg_addr = vff_tx_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	o_register_value = *((volatile u32*)(reg_addr));
	*((volatile u32*)(reg_addr)) = (~(buff[2]))&o_register_value;/* setting: buff[2] */
	buff[2] = o_register_value;
	return 0;
}

static long uart_polling_uart_register_by_offset(int *err_id, u32 buff[])
{
	u32 reg_val;
	u32 reg_addr = uart_base[buff[0] - 1] + buff[1];	/* base: buff[0], offset: buff[1] */
	u32 try_time = buff[2];
	u32 monitor_array_length = buff[3];
	u32 *monitor_array = &buff[4];
	u32 i = 0;
	u32 last_time_reg_val ;

	*err_id = UART_DRV_EXT_ERR_SUCCESS;
	if(monitor_array_length < 1)
		return -EINVAL;

	printk("Init array[]:");
	for(i=0;i<10;i++)
		printk("%08x ",buff[i]);
	printk("\r\n");

	printk("Monitor array[%d]:", monitor_array_length);
	for(i=0;i<monitor_array_length;i++)
		printk("%08x ",monitor_array[i]);
	printk("\r\n");

	i=0;

	reg_val = *((volatile u32*)(reg_addr));	/* Get first data */
	last_time_reg_val = reg_val;
	monitor_array[i++] = reg_val;
	printk("P1 Reg addr:%08x, val:%02x\r\n", reg_addr, reg_val);

	while((try_time--)&&(i<monitor_array_length)){
		reg_val = *((volatile u32*)reg_addr);
		if(reg_val == last_time_reg_val)
			continue;
		else{
			printk("P2 Reg addr:%08x, val:%02x\r\n", reg_addr, reg_val);
			monitor_array[i++] = reg_val;
			last_time_reg_val = reg_val;
		}
	}

	/* Updata gotten data number */
	monitor_array_length = i;
	buff[3] = i;

	printk("Result array[]:");
	for(i=0;i<10;i++)
		printk("%08x ",buff[i]);
	printk("\r\n");
	printk("Monitor array[%d]:", monitor_array_length);
	for(i=0;i<monitor_array_length;i++)
		printk("%08x ",monitor_array[i]);
	printk("\r\n");

	return 0;
}

static struct op_mapping op_mapping_table[] = {
	{0, NULL, 0},
	{UART_READ_REG, uart_read_register, 0},
	{UART_WRITE_REG, uart_write_register, 0},
	{UART_SET_REG, uart_set_register, 0},
	{UART_CLR_REG, uart_clr_register, 0},
	{UART_CFG_TXRX_PIN, uart_configure_txrx_pin, 1},
	{UART_CFG_RTSCTS_PIN, uart_configure_rtscts_pin, 1},
	{UART_DISABLE_UART_IRQ, uart_disable_uart_irq, 1},
	{UART_RESTORE_UART_IRQ, uart_enable_uart_irq, 1},
	{UART_DISABLE_TX_VFF_IRQ, uart_disable_tx_vff_irq, 1},
	{UART_RESTORE_TX_VFF_IRQ, uart_enable_tx_vff_irq, 1},
	{UART_DISABLE_RX_VFF_IRQ, uart_disable_rx_vff_irq, 1},
	{UART_RESTORE_RX_VFF_IRQ, uart_enable_rx_vff_irq, 1},
	{UART_BACK_UP_UART_REGISTER, uart_back_up_uart_registers, 1},
	{UART_RESTORE_UART_REGISTER, uart_restore_uart_registers, 1},
	{UART_BACK_UP_VFF_TX_REGISTER, uart_back_up_vff_tx_registers, 1},
	{UART_RESTORE_VFF_TX_REGISTER, uart_restore_vff_tx_registers, 1},
	{UART_BACK_UP_VFF_RX_REGISTER, uart_back_up_vff_rx_registers, 1},
	{UART_RESTORE_VFF_RX_REGISTER, uart_restore_vff_rx_registers, 1},
	{UART_SET_FLOW_CONTROL, uart_set_flow_ctrl, 1},
	{UART_SET_XON1_XOFF1, uart_set_xon1_off1, 1},
	{UART_SET_XON2_XOFF2, uart_set_xon2_off2, 1},
	{UART_SET_ESCAPE_CHAR, uart_set_escape_char, 1},
	{UART_ENABLE_AUTO_BAUD, uart_enable_auto_baud, 1},
	{UART_AUTO_BAUD_DONE, uart_auto_baud_check_done, 1},
	{UART_READ_UART_REG_BY_OFFSET, uart_read_uart_register_by_offset, 1},
	{UART_WRITE_UART_REG_BY_OFFSET, uart_write_uart_register_by_offset, 1},
	{UART_SET_UART_REG_BY_OFFSET, uart_set_uart_register_by_offset, 1},
	{UART_CLR_UART_REG_BY_OFFSET, uart_clr_uart_register_by_offset, 1},
	{UART_POLLING_UART_REG_BY_OFFSET, uart_polling_uart_register_by_offset, 1},
	{UART_READ_RX_VFF_REG_BY_OFFSET, uart_read_rx_vff_register_by_offset, 1},
	{UART_WRITE_RX_VFF_REG_BY_OFFSET, uart_write_rx_vff_register_by_offset, 1},
	{UART_SET_RX_VFF_REG_BY_OFFSET, uart_set_rx_vff_register_by_offset, 1},
	{UART_CLR_RX_VFF_REG_BY_OFFSET, uart_clr_rx_vff_register_by_offset, 1},
	{UART_READ_TX_VFF_REG_BY_OFFSET, uart_read_tx_vff_register_by_offset, 1},
	{UART_WRITE_TX_VFF_REG_BY_OFFSET, uart_write_tx_vff_register_by_offset, 1},
	{UART_SET_TX_VFF_REG_BY_OFFSET, uart_set_tx_vff_register_by_offset, 1},
	{UART_CLR_TX_VFF_REG_BY_OFFSET, uart_clr_tx_vff_register_by_offset, 1},
};

static long uart_operation_dispatch(int operation, unsigned int buff[])
{
	uart_print("[UART Ext Drv]Enter dispatch. op: %d, buff[1]: %d, buff[2]: %d\r\n", operation, buff[1], buff[2]);
	if( operation >= ( sizeof(op_mapping_table)/sizeof(op_mapping_table[0]) ) ||0==operation )
		return -UART_DRV_EXT_ERR_UNDEFINED_OP;
	if(operation != op_mapping_table[operation].index)
		return -UART_DRV_EXT_ERR_MAPPING;
	if(NULL == buff)
		return -UART_DRV_EXT_ERR_NULL_POINTER;
	if( op_mapping_table[operation].uart_bauded && (buff[1]>=UART_ID_MAX || 0 == buff[1]) ){
		return -UART_DRV_EXT_ERR_INVALID_UART_ID;
	}
	return op_mapping_table[operation].func( (int*)&buff[0], &buff[1] );
}

static long uart_udvt_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	struct udvt_cmd *op = (struct udvt_cmd*)uarg;
	return uart_operation_dispatch(op->cmd, op->data);
}

static int uart_udvt_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations uart_udvt_dev_fops = {
	.owner		    = THIS_MODULE,
	.unlocked_ioctl	= uart_udvt_dev_ioctl,
	.open		    = uart_udvt_dev_open,
};

static struct miscdevice uart_udvt_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= UART_NAME,
	.fops	= &uart_udvt_dev_fops,
};

static int __init uart_udvt_mod_init(void)
{
	int r;

	r = misc_register(&uart_udvt_dev);
	if (r) {
		printk(UART_SAY "register driver failed (%d)\n", r);
		return r;
	}

	return 0;
}

/* should never be called */
static void __exit uart_udvt_mod_exit(void)
{
	int r;
	
	r = misc_deregister(&uart_udvt_dev);
	if(r){
		printk(UART_SAY"unregister driver failed\n");
	}
}

module_init(uart_udvt_mod_init);
module_exit(uart_udvt_mod_exit);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("MT6573 UART Ext Driver for UDVT v0.1");
MODULE_LICENSE("GPL");
