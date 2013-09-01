/****************************************************************

Siano Mobile Silicon, Inc.
MDTV receiver kernel modules.
Copyright (C) 2006-2008, Uri Shkolnik

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

 This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

****************************************************************/
//#define PXA_310_LV

#include <linux/kernel.h>
#include <asm/irq.h>
#include <linux/semaphore.h>
//#include <asm/hardware.h>                 //xingyu add
#ifdef PXA_310_LV
#include <asm/arch/ssp.h>
#include <asm/arch/mfp.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pxa3xx_gpio.h>
#endif
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/dma-mapping.h>
#include <asm/dma.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include "smsdbg_prn.h"
#include <linux/slab.h>
#ifdef PXA_310_LV

#define SSP_PORT 4
#define SSP_CKEN CKEN_SSP4
#define SMS_IRQ_GPIO MFP_PIN_GPIO93

#if (SSP_PORT == 1)
#define SDCMR_RX DRCMRRXSSDR
#define SDCMR_TX DRCMRTXSSDR
#else
#if (SSP_PORT == 2)
#define SDCMR_RX DRCMR15
#define SDCMR_TX DRCMR16
#else
#if (SSP_PORT == 3)
#define SDCMR_RX DRCMR66
#define SDCMR_TX DRCMR67
#else
#if (SSP_PORT == 4)
#define SDCMR_RX DRCMRRXSADR
#define SDCMR_TX DRCMRTXSADR
#endif
#endif
#endif
#endif
#else /*PXA_310_LV */
#define SSP_PORT 1
#define SDCMR_RX DRCMRRXSSDR
#define SDCMR_TX DRCMRTXSSDR

#endif /*PXA_310_LV */

/* Macros defining physical layer behaviour*/
#ifdef PXA_310_LV
#define CLOCK_FACTOR 1
#else /*PXA_310_LV */
#define CLOCK_FACTOR 2
#endif /*PXA_310_LV */

#include "smscoreapi.h"
#ifdef MTK_SPI                                                                      //xingyu add
#include <linux/spi/spi.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>

#include <mach/mt_gpio.h>
#include <linux/delay.h>
#include <mach/eint.h>
#include <mach/mt_spi.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>


#include <mach/irqs.h>

#include <linux/io.h>                              //debug
#include <cust_cmmb.h>

struct SMSDev_data {
	dev_t			    devt;
	spinlock_t		    spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;

	struct mutex		buf_lock;
	unsigned char   	users;
	u8			       *tx_buf;
	u8                         *rx_buf;
};
extern struct SMSDev_data* SMSDev;
static struct mutex g_sms_mtkspi_mutex;
static struct mt_chip_conf spi_conf;
#define TRANSFER_NUM (RX_BUFFER_SIZE/DMA_LEN+1)
#define MTK_SPI_RULE
#define DMA_LEN   (4*1024)
struct spi_transfer transfer[2];                // if len%1024 !=0 ,need transfer twice, due to mtk spi rule
#define CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES (0x400)
#define CMMB_SPI_INTERFACE_MAX_PKT_COUNT_PER_TIMES  (0x100)
#endif

/* Macros for coding reuse */

/*! macro to align the divider to the proper offset in the register bits */
#define CLOCK_DIVIDER(i)((i-1)<<8)	/* 1-4096 */

/*! DMA related macros */
#define DMA_INT_MASK (DCSR_ENDINTR | DCSR_STARTINTR | DCSR_BUSERR)
#define RESET_DMA_CHANNEL (DCSR_NODESC | DMA_INT_MASK)

#define SSP_TIMEOUT_SCALE (769)
#define SSP_TIMEOUT(x) ((x*10000)/SSP_TIMEOUT_SCALE)

#ifndef SPI_PACKET_SIZE
#define SPI_PACKET_SIZE 256
#endif
/* physical layer variables */
/*! global bus data */
struct spiphy_dev_s {
//	struct ssp_dev sspdev;	/*!< ssp port configuration */                              //xingyu add
	struct completion transfer_in_process;
	void (*interruptHandler) (void *);
	void *intr_context;
	struct device *dev;	/*!< device model stuff */
	int rx_dma_channel;
	int tx_dma_channel;
	int rx_buf_len;
	int tx_buf_len;

	int temp;                                      //xingyu add
};

/*!
invert the endianness of a single 32it integer

\param[in]		u: word to invert

\return		the inverted word
*/
static inline u32 invert_bo(u32 u)
{
	return ((u & 0xff) << 24) | ((u & 0xff00) << 8) | ((u & 0xff0000) >> 8)
		| ((u & 0xff000000) >> 24);
}

/*!
invert the endianness of a data buffer

\param[in]		buf: buffer to invert
\param[in]		len: buffer length

\return		the inverted word
*/
#ifdef PXA_310_LV 
static int invert_endianness(char *buf, int len)
{
	int i;
	u32 *ptr = (u32 *) buf;

	len = (len + 3) / 4;
	for (i = 0; i < len; i++, ptr++)
		*ptr = invert_bo(*ptr);

	return 4 * ((len + 3) & (~3));
}

/*! Map DMA buffers when request starts

\return	error status
*/
static unsigned long dma_map_buf(struct spiphy_dev_s *spiphy_dev, char *buf,
		int len, int direction)
{
	unsigned long phyaddr;
	/* map dma buffers */
	if (!buf) {
		PERROR(" NULL buffers to map\n");
		return 0;
	}
	/* map buffer */
#ifdef PXA_310_LV                                    //xingyu add
	phyaddr = dma_map_single(spiphy_dev->dev, buf, len, direction);
	if (dma_mapping_error(phyaddr)) {
		PERROR("exiting  with error\n");
		return 0;
	}
#endif
	return phyaddr;
}

static irqreturn_t spibus_interrupt(int irq, void *context)
{
	struct spiphy_dev_s *spiphy_dev = (struct spiphy_dev_s *) context;
	//PDEBUG("recieved interrupt from device dev=%p.\n", context);
	PDEBUG("Eint");
	if (spiphy_dev->interruptHandler)
		spiphy_dev->interruptHandler(spiphy_dev->intr_context);
	return IRQ_HANDLED;

}

/*!	DMA controller callback - called only on BUS error condition

\param[in]	channel: DMA channel with error
\param[in]	data: Unused
\param[in]	regs: Unused
\return		void
*/
static void spibus_dma_handler(int channel, void *context)
{
#ifdef PXA_310_LV                                    //xingyu add
	struct spiphy_dev_s *spiphy_dev = (struct spiphy_dev_s *) context;
	u32 irq_status = DCSR(channel) & DMA_INT_MASK;

	PDEBUG("recieved interrupt from dma channel %d irq status %x.\n",
	       channel, irq_status);
	if (irq_status & DCSR_BUSERR) {
		PERROR("bus error!!! resetting channel %d\n", channel);

		DCSR(spiphy_dev->rx_dma_channel) = RESET_DMA_CHANNEL;
		DCSR(spiphy_dev->tx_dma_channel) = RESET_DMA_CHANNEL;
	}
	DCSR(spiphy_dev->rx_dma_channel) = RESET_DMA_CHANNEL;
	complete(&spiphy_dev->transfer_in_process);
#endif
}
#endif


#ifdef MTK_SPI                                                                      //xingyu add
static ssize_t SMSDev_sync(struct SMSDev_data *SMSDevS, struct spi_message *message)
{
	int ret;
	/*
	   if(!flag_spi_ok){
	   inno_msg("Warning:spi_sync timeout,should not to use spi");
	   return -1;
	   }	
	 */
	kmutex_lock(&g_sms_mtkspi_mutex); 
	//inno_msg("**s"); 
	if (SMSDevS==NULL || SMSDevS->spi == NULL){
		sms_err("error :spi ==NULL \r\n");
		ret = -ESHUTDOWN;
	}
	else
		ret = spi_sync(SMSDevS->spi, message);
	// inno_msg("**e");
	kmutex_unlock(&g_sms_mtkspi_mutex); 

	if(ret){
		sms_err("spi_sync fail ret=%d,should check",ret);
		/*
		   if(ret == -ETIME){
		   flag_spi_ok = 0;
		   }
		 */
	}		 
	return ret;
}
#endif  //MTK_SPI


void smsspi_set_mode(int mode)
{
	struct mt_chip_conf* spi_par;
	//kmutex_lock(&g_sms_mtkspi_mutex);
	spi_par =&spi_conf;
	if(!spi_par)
	{
		PDEBUG("spi config fail");
		//kmutex_unlock(&g_sms_mtkspi_mutex);
		return;
	}

	if(1 == mode)
	{
		spi_par->com_mod = DMA_TRANSFER;
	}
	else
	{
		spi_par->com_mod = FIFO_TRANSFER;
	}
	
	if(spi_setup(SMSDev->spi))
	{
		PDEBUG("spi_setup fail");
	}
	//kmutex_unlock(&g_sms_mtkspi_mutex);
	return;
}




void smsspibus_xfer(void *context, unsigned char *txbuf,
		    unsigned long txbuf_phy_addr, unsigned char *rxbuf,
		    unsigned long rxbuf_phy_addr, int len)
{
	int const pkt_count = len / CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	int const remainder = len % CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	struct spi_message msg;
	spi_message_init(&msg);
	if(len>CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){	
		transfer[0].tx_buf =(txbuf==NULL)?NULL: txbuf;
		transfer[0].rx_buf =(rxbuf==NULL)?NULL: rxbuf;
		transfer[0].len = CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES*pkt_count;
		spi_message_add_tail(&transfer[0], &msg);

		if(0 != remainder)	 { 
			transfer[1].tx_buf =(txbuf==NULL)?NULL:txbuf+ (CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES * pkt_count);
			transfer[1].rx_buf =(rxbuf==NULL)?NULL:rxbuf+ (CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES * pkt_count);
			transfer[1].len = remainder;
			spi_message_add_tail(&transfer[1], &msg);
		}
	}
	else{
		transfer[0].tx_buf =(txbuf==NULL)?NULL: txbuf;
		transfer[0].rx_buf =(rxbuf==NULL)?NULL: rxbuf;
		transfer[0].len = len;
		spi_message_add_tail(&transfer[0], &msg);
	}
	SMSDev_sync(SMSDev,&msg);	
	return;
}


void smsspibus_xfer_mix(void *context, unsigned char *txbuf,
		    unsigned long txbuf_phy_addr, unsigned char *rxbuf,
		    unsigned long rxbuf_phy_addr, int len)
{

	unsigned char *txbuf_temp = NULL, *rxbuf_temp = NULL;

	//set up DMA or FIFO mode
	if (txbuf)
	{
		smsspi_set_mode(0); // FIFO
	}
	else
	{
		smsspi_set_mode(1); // DMA
	}

	// TX set len per time= 32
	if (txbuf) // command & firmware
	{
		txbuf_temp = txbuf; 
		rxbuf_temp = rxbuf;
		if( len > 32 )
		{
			int i =0;
			int pkt_cnt = len / 32;
			int remainder = len % 32;
			for(i = 0; i < pkt_cnt; i++)
			{
				smsspibus_xfer(context,txbuf_temp,txbuf_phy_addr,rxbuf_temp,rxbuf_phy_addr,32);
				txbuf_temp += 32;
				rxbuf_temp += 32;
			}
			if(remainder)
			{
				smsspibus_xfer(context,txbuf_temp,txbuf_phy_addr,rxbuf_temp,rxbuf_phy_addr,remainder);
			}
		}
		else
		{
			smsspibus_xfer(context,txbuf_temp,txbuf_phy_addr,rxbuf_temp,rxbuf_phy_addr,len);
		}
	}
	else // frames
	{
		smsspibus_xfer(context,txbuf,txbuf_phy_addr,rxbuf,rxbuf_phy_addr,len);
	}
	return;

}



void smschipreset(void *context)
{
#ifdef MTK_SPI                                                                      //xingyu add
	/*reset*/
	mt_set_gpio_mode(GPIO_CMMB_RST_PIN, GPIO_CMMB_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CMMB_RST_PIN, GPIO_DIR_OUT);

	mdelay(1);
	mt_set_gpio_out(GPIO_CMMB_RST_PIN, GPIO_OUT_ZERO); 	
	mdelay(100);                                                                         //siano suggestion
	mt_set_gpio_out(GPIO_CMMB_RST_PIN, GPIO_OUT_ONE); 	
	mdelay(100);                                                           //1122
#endif
}
void smschipSpiSetup(void)
{
#ifdef MTK_SPI          
	       struct mt_chip_conf* spi_par;
	       SMSDev->spi->controller_data=(void*)&spi_conf;
	       memset(&spi_conf,0,sizeof(struct mt_chip_conf));
	       spi_par =&spi_conf;
	       if(!spi_par){
		       PDEBUG("spi config fail");
		       return;
	       }
	       spi_par->setuptime =15;
	       spi_par->holdtime = 15;
	       spi_par->high_time = 10;       //10--6m   15--4m   20--3m  30--2m  [ 60--1m 120--0.5m  300--0.2m]
	       spi_par->low_time = 10;
	       spi_par->cs_idletime = 20;

	       spi_par->rx_mlsb = 1; 
	       spi_par->tx_mlsb = 1;		 
	       spi_par->tx_endian = 0;
	       spi_par->rx_endian = 0;

	       spi_par->cpol = 0;
	       spi_par->cpha = 0;
	       spi_par->com_mod = DMA_TRANSFER;
	       //spi_par->com_mod = FIFO_TRANSFER;

	       spi_par->pause = 0;
	       spi_par->finish_intr = 1;
	       spi_par->deassert = 0;

	       printk("setuptime=%d \n",spi_par->setuptime);
	       if(spi_setup(SMSDev->spi)){
		       PDEBUG("spi_setup fail");
		       return;
	       }
	       printk("DMA_LEN=%d",DMA_LEN);
#endif
}


typedef void (*func_ptr)(void);
func_ptr intHander=NULL;
void smschipEintEnable()
{
#ifdef MTK_SPI                 
	// spi setup
	sms_info("smschipSpiSetup");
	smschipSpiSetup();

	mt_set_gpio_mode(GPIO_CMMB_EINT_PIN, GPIO_CMMB_EINT_PIN_M_EINT);                 //set to eint MODE for enable eint function
	mt_set_gpio_dir(GPIO_CMMB_EINT_PIN, GPIO_DIR_IN); 
	// register irp 
	mt65xx_eint_set_sens(CUST_EINT_CMMB_NUM, CUST_EINT_EDGE_SENSITIVE);
	mt65xx_eint_registration(CUST_EINT_CMMB_NUM, CUST_EINT_DEBOUNCE_DISABLE, CUST_EINT_POLARITY_HIGH, intHander, 0);         //1202
#ifdef CONFIG_ARCH_MT6575
	mt65xx_eint_mask(CUST_EINT_CMMB_NUM);   
	sms_info("[smsspiphy_init]CMMB GPIO EINT PIN mode: %d, dir:%d\n",mt_get_gpio_mode(GPIO_CMMB_EINT_PIN),
			mt_get_gpio_dir(GPIO_CMMB_EINT_PIN));    

#endif
	mt65xx_eint_unmask(CUST_EINT_CMMB_NUM);                        //enable eint

	sms_info("CMMB GPIO EINT PIN mode: %d, dir:%d\n",mt_get_gpio_mode(GPIO_CMMB_EINT_PIN),
			mt_get_gpio_dir(GPIO_CMMB_EINT_PIN));    
#endif
}
void smschipEintDisable()
{
#ifdef MTK_SPI                                                                      
        //xingyu  interrupt setting
	mt65xx_eint_mask(CUST_EINT_CMMB_NUM);   
	mt_set_gpio_mode(GPIO_CMMB_EINT_PIN, GPIO_CMMB_EINT_PIN_M_GPIO);                 //set to eint MODE for enable eint function
	mt_set_gpio_pull_enable(GPIO_CMMB_EINT_PIN, GPIO_PULL_DISABLE);
	sms_info("CMMB GPIO EINT PIN mode:num:%d, %d, dir:%d,pullen:%d,pullup%d",GPIO_CMMB_EINT_PIN,mt_get_gpio_mode(GPIO_CMMB_EINT_PIN),
			mt_get_gpio_dir(GPIO_CMMB_EINT_PIN),mt_get_gpio_pull_enable(GPIO_CMMB_EINT_PIN),mt_get_gpio_pull_select(GPIO_CMMB_EINT_PIN));    
#endif
}

/*
void *smsspiphy_init(void *context, void (*smsspi_interruptHandler) (void *),
		     void *intr_context)
*/		               //xingyu add

void *smsspiphy_init(void *context, void (*smsspi_interruptHandler) (void ),
		     void *intr_context)		     
{
#ifdef MTK_SPI   //xingyu add
	struct mt_chip_conf* spi_par;
	struct spiphy_dev_s *spiphy_dev;
	kmutex_init(&g_sms_mtkspi_mutex);
	printk("smsspi_interruptHandler =%p \n",smsspi_interruptHandler);
	intHander = (func_ptr)smsspi_interruptHandler;
#ifdef REQUEST_FIRMWARE_SUPPORTED   //xingyu 1202 ko version
	//power on
        cust_cmmb_power_on();
	//reset 
	smschipreset(context);
	//eint enable
	smschipEintEnable();
#endif  //REQUEST_FIRMWARE_SUPPORTED

	if(SMSDev && SMSDev->spi)
	{
		SMSDev->spi->controller_data=(void*)&spi_conf;
		spi_par =&spi_conf;
		if(!spi_par){
			PDEBUG("spi config fail");
			return 0;
		}
		spi_par->setuptime =15;
		spi_par->holdtime = 15;
		spi_par->high_time = 10;       //10--6m   15--4m   20--3m  30--2m  [ 60--1m 120--0.5m  300--0.2m]
		spi_par->low_time = 10;
		spi_par->cs_idletime = 20;

		spi_par->rx_mlsb = 1; 
		spi_par->tx_mlsb = 1;		 
		spi_par->tx_endian = 0;
		spi_par->rx_endian = 0;

		spi_par->cpol = 0;
		spi_par->cpha = 0;
		spi_par->com_mod = DMA_TRANSFER;
		//spi_par->com_mod = FIFO_TRANSFER;

		spi_par->pause = 0;
		spi_par->finish_intr = 1;
		spi_par->deassert = 0;

		printk("setuptime=%d \n",spi_par->setuptime);
		if(spi_setup(SMSDev->spi)){
			PDEBUG("spi_setup fail");
			return 0;
		}
	}
	else
		sms_err("SMSDev && SMSDev->spi =NULL");
	printk("DMA_LEN=%d",DMA_LEN);
	//     PERROR("******* sleep smsspiphy_init *************");
	//     msleep(1000*60*30);                              //xingyu add debug interrupt
	
	spiphy_dev = kmalloc(sizeof(struct spiphy_dev_s), GFP_KERNEL);
	spiphy_dev->temp = 1;
	return spiphy_dev;
#endif

#ifdef PXA_310_LV                                    //xingyu add
	int ret;
	struct spiphy_dev_s *spiphy_dev;
	u32 mode = 0, flags = 0, psp_flags = 0, speed = 0;
	PDEBUG("entering\n");

	spiphy_dev = kmalloc(sizeof(struct spiphy_dev_s), GFP_KERNEL);

	ret = ssp_init(&spiphy_dev->sspdev, SSP_PORT, 0);
	if (ret) {
		PERROR("ssp_init failed. error %d", ret);
		goto error_sspinit;
	}
#ifdef PXA_310_LV
	pxa3xx_mfp_set_afds(SMS_IRQ_GPIO, MFP_AF0, MFP_DS03X);              //xingyu ÉèÖÃinterrupt
	pxa3xx_gpio_set_rising_edge_detect(SMS_IRQ_GPIO, 1);
	pxa3xx_gpio_set_direction(SMS_IRQ_GPIO, GPIO_DIR_IN);
#else /*PXA_310_LV */
	/* receive input interrupts from the SMS 1000 on J32 pin 11 */
	pxa_gpio_mode(22 | GPIO_IN);
#endif /*PXA_310_LV */
	speed = CLOCK_DIVIDER(CLOCK_FACTOR); /* clock divisor for this mode */
	/* 32bit words in the fifo */
	mode = SSCR0_Motorola | SSCR0_DataSize(16) | SSCR0_EDSS;
	/* SSCR1 = flags */
	flags = SSCR1_RxTresh(1) | SSCR1_TxTresh(1) | SSCR1_TSRE |
	 SSCR1_RSRE | SSCR1_RIE | SSCR1_TRAIL;	/* | SSCR1_TIE */

	ssp_config(&spiphy_dev->sspdev, mode, flags, psp_flags, speed);
	ssp_disable(&(spiphy_dev->sspdev));
#ifdef PXA_310_LV                        //xingyu spi io

	pxa3xx_mfp_set_afds(MFP_PIN_GPIO95, MFP_AF1, MFP_DS03X);
	pxa3xx_mfp_set_afds(MFP_PIN_GPIO96, MFP_AF1, MFP_DS03X);
	pxa3xx_mfp_set_afds(MFP_PIN_GPIO97, MFP_AF1, MFP_DS03X);
	pxa3xx_mfp_set_afds(MFP_PIN_GPIO98, MFP_AF1, MFP_DS03X);
#else /*PXA_310_LV */
	pxa_gpio_mode(GPIO23_SCLK_MD);
	pxa_gpio_mode(GPIO24_SFRM_MD);
	pxa_gpio_mode(GPIO25_STXD_MD);
	pxa_gpio_mode(GPIO26_SRXD_MD);
#endif /*PXA_310_LV */
	/* setup the dma */
	spiphy_dev->rx_dma_channel =
	    pxa_request_dma("spibusdrv_rx", DMA_PRIO_HIGH, spibus_dma_handler,
			    spiphy_dev);
	if (spiphy_dev->rx_dma_channel < 0) {
		ret = -EBUSY;
		PERROR("Could not get RX DMA channel.\n");
		goto error_rxdma;
	}
	spiphy_dev->tx_dma_channel =
	    pxa_request_dma("spibusdrv_tx", DMA_PRIO_HIGH, spibus_dma_handler,
			    spiphy_dev);
	if (spiphy_dev->tx_dma_channel < 0) {
		ret = -EBUSY;
		PERROR("Could not get TX DMA channel.\n");
		goto error_txdma;
	}

	SDCMR_RX = DRCMR_MAPVLD | spiphy_dev->rx_dma_channel;
	SDCMR_TX = DRCMR_MAPVLD | spiphy_dev->tx_dma_channel;

	PDEBUG("dma rx channel: %d, dma tx channel: %d\n",
	       spiphy_dev->rx_dma_channel, spiphy_dev->tx_dma_channel);
	/* enable the clock */

	spiphy_dev->interruptHandler = smsspi_interruptHandler;
	spiphy_dev->intr_context = intr_context;
#ifdef PXA_310_LV
	set_irq_type(IRQ_GPIO(MFP2GPIO(SMS_IRQ_GPIO)), IRQT_FALLING);
	ret =
	    request_irq(IRQ_GPIO(MFP2GPIO(SMS_IRQ_GPIO)), spibus_interrupt,
			SA_INTERRUPT, "SMSSPI", spiphy_dev);
#else /*PXA_310_LV */
	set_irq_type(IRQ_GPIO(22), IRQT_FALLING);
	ret =
	    request_irq(IRQ_GPIO(22), spibus_interrupt, SA_INTERRUPT, "SMSSPI",
			&(g_spidata.sspdev));
#endif /*PXA_310_LV */
	if (ret) {
		PERROR("Could not get interrupt for SMS device. status =%d\n",
		       ret);
		goto error_irq;
	}

	ssp_enable(&(spiphy_dev->sspdev));
	PDEBUG("exiting\n");
	return spiphy_dev;
error_irq:
	if (spiphy_dev->tx_dma_channel >= 0)
		pxa_free_dma(spiphy_dev->tx_dma_channel);

error_txdma:
	if (spiphy_dev->rx_dma_channel >= 0)
		pxa_free_dma(spiphy_dev->rx_dma_channel);

error_rxdma:
	ssp_exit(&spiphy_dev->sspdev);
error_sspinit:
	PDEBUG("exiting on error\n");
#endif
	return 0;
}

int smsspiphy_deinit(void *context)
{
#ifdef PXA_310_LV                                    //xingyu add
	struct spiphy_dev_s *spiphy_dev = (struct spiphy_dev_s *) context;
	PDEBUG("entering\n");

	/* disable the spi port */
	ssp_flush(&spiphy_dev->sspdev);
	ssp_disable(&spiphy_dev->sspdev);

	/*  release DMA resources */
	if (spiphy_dev->rx_dma_channel >= 0)
		pxa_free_dma(spiphy_dev->rx_dma_channel);

	if (spiphy_dev->tx_dma_channel >= 0)
		pxa_free_dma(spiphy_dev->tx_dma_channel);

	/* release Memory resources */
#ifdef PXA_310_LV
	free_irq(IRQ_GPIO(MFP2GPIO(SMS_IRQ_GPIO)), spiphy_dev);
#else /*PXA_310_LV */
	free_irq(IRQ_GPIO(22), &spiphy_dev->sspdev);
#endif /*PXA_310_LV */
	ssp_exit(&spiphy_dev->sspdev);
	PDEBUG("exiting\n");
#endif
	return 0;
}

/*
       //xingyu 
*/
void smsspiphy_set_config(struct spiphy_dev_s *spiphy_dev, int clock_divider)
{
#ifdef PXA_310_LV                                                                  //xingyu add
	u32 mode, flags, speed, psp_flags = 0;
	ssp_disable(&spiphy_dev->sspdev);
	/* clock divisor for this mode. */
	speed = CLOCK_DIVIDER(clock_divider);
	/* 32bit words in the fifo */
	mode = SSCR0_Motorola | SSCR0_DataSize(16) | SSCR0_EDSS;
	flags = SSCR1_RxTresh(1) | SSCR1_TxTresh(1) | SSCR1_TSRE |
		 SSCR1_RSRE | SSCR1_RIE | SSCR1_TRAIL;	/* | SSCR1_TIE */
	ssp_config(&spiphy_dev->sspdev, mode, flags, psp_flags, speed);
	ssp_enable(&spiphy_dev->sspdev);
#endif
}

void prepareForFWDnl(void *context)
{
	struct spiphy_dev_s *spiphy_dev = (struct spiphy_dev_s *) context;
	smsspiphy_set_config(spiphy_dev, 2);
	msleep(100);
}

void fwDnlComplete(void *context, int App)
{
	struct spiphy_dev_s *spiphy_dev = (struct spiphy_dev_s *) context;
	smsspiphy_set_config(spiphy_dev, 1);
	msleep(100);
	sms_info("end");
}
