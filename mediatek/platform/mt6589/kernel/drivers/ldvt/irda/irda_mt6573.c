#include <asm/io.h>
#include <mach/dma.h>
#include <asm/page.h>
#include "irframer_hw.h"

void mt_irda_dev_open(void)
{
	outl(1<<3,APMCU_CG_CLR1);	
	if (mt_set_gpio_mode(48,4)||
			mt_set_gpio_mode(49,4)||
			mt_set_gpio_mode(50,4)||
			mt_set_gpio_mode(43,0)||
			mt_set_gpio_dir(43,GPIO_DIR_OUT)||
			mt_set_gpio_pull_enable(43,true)||
			mt_set_gpio_pull_select(43,GPIO_PULL_UP)||
			mt_set_gpio_out(43,GPIO_OUT_ONE)
			)
	dbg("%s(),set gpio mode failed !\n",__FUNCTION__);
}


void mt_irda_dev_close(void)
{
	outl(1<<3,APMCU_CG_SET1);
}

void irda_dma_rx_config(struct mt_irda_framer *framer)
{
	framer->dma->mas=DMA_CON_MASTER_IRDARX;
		framer->dma->iten=
#ifdef DEBUG
		DMA_TRUE;
#else
		DMA_FALSE;
#endif
		framer->dma->burst=DMA_CON_BURST_SINGLE;
		framer->dma->count=(1<<11)+1;
		framer->dma->size=DMA_CON_SIZE_BYTE;
		framer->dma->pgmaddr=framer->buff_dma;
		framer->dma->dir=1;
		framer->dma->b2w=0;
		framer->dma->dinc=DMA_TRUE;
		framer->dma->sinc=DMA_FALSE;
		mt_config_dma(framer->dma,ALL);
}

void irda_dma_tx_config(struct mt_irda_framer *framer)
{
	framer->dma->mas=DMA_CON_MASTER_IRDATX;
		framer->dma->iten=
#ifdef DEBUG
		DMA_TRUE;
#else
		DMA_FALSE;
#endif
		framer->dma->burst=DMA_CON_BURST_SINGLE;
		framer->dma->count=framer->tx_size;
		framer->dma->size=DMA_CON_SIZE_BYTE;
		framer->dma->pgmaddr=framer->buff_dma;
		framer->dma->dir=0;
		framer->dma->b2w=0;
		framer->dma->dinc=DMA_FALSE;
		framer->dma->sinc=DMA_TRUE;
		mt_config_dma(framer->dma,ALL);	
}

void irda_irq_init(void)
{


}