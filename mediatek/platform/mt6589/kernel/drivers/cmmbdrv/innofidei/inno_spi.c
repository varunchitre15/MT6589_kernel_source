/* 
 * Functions of AP Communication Layer
 * 
 *
 * Innofidei Inc.
 *
 * By: Mason Chen <masonchen@innofidei.com>
 *
 * NOTE: User should implement the functions according to software document 
 *
 */
//#include "INNO_Operation.h"
//#include "INNO_Communication.h"

#include "inno_spi.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <mach/mt_gpio.h> 

#ifdef MTK_SPI                                //xingyu add
#include <linux/spi/spi.h>
#include <mach/mt_spi.h>
extern struct INNODev_data* INNODev;
static struct mt_chip_conf spi_conf;
#endif

extern struct spi_transfer transfer_1[2];     

unsigned char* cmmb_spi_src_buffer_all=NULL;   //for some UAM cmd need alarge buffer

extern struct mutex inno_mtkspi_mutex;
extern struct mutex inno_spi_mutex;

extern int  flag_spi_ok;                                   // spi fail, do not to comminucate
ssize_t INNODev_sync(struct INNODev_data *INNODevS, struct spi_message *message)
{
	int ret;
	if(!flag_spi_ok){
		inno_msg("Warning:spi_sync timeout,should not to use spi");
		return -1;
	}	
	//	spin_lock_irq(&INNODevS->spi_lock);
	mutex_lock(&inno_mtkspi_mutex);
	//inno_msg("**s"); 
	if (INNODevS->spi == NULL){
		inno_err("spi ==NULL");
		ret = -ESHUTDOWN;
	}
	else
		ret = spi_sync(INNODevS->spi, message);

	// inno_msg("**e");
	mutex_unlock(&inno_mtkspi_mutex);
	//	spin_unlock_irq(&INNODevS->spi_lock);
	/*
	   if (ret == 0) {                                                
	   ret = message->status;
	   if (ret == 0)
	   ret = message->actual_length;
	   }
	 */
	if(ret){
		inno_err("spi_sync fail ret=%d,should check",ret);
		if(ret == -ETIME){
       		flag_spi_ok = 0;
		}
	}		 
	return ret;
}

INNO_RET  INNODev_xfer(unsigned char *txbuf,unsigned char *rxbuf, int len)
{
	int const pkt_count = len / CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	int const remainder = len % CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	struct spi_message msg;
	spi_message_init(&msg);

	inno_msg(" len=%d, txbuf=0x%p,rxbuf=0x%p",len,txbuf,rxbuf);
	if(len>CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){	
		transfer_1[0].tx_buf =(txbuf==NULL)?NULL: txbuf;
		transfer_1[0].rx_buf =(rxbuf==NULL)?NULL: rxbuf;
		transfer_1[0].len = CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES*pkt_count;
		spi_message_add_tail(&transfer_1[0], &msg);

		if(0 != remainder)	 { 
			transfer_1[1].tx_buf =(txbuf==NULL)?NULL:txbuf+ (CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES * pkt_count);
			transfer_1[1].rx_buf =(rxbuf==NULL)?NULL:rxbuf+ (CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES * pkt_count);
			transfer_1[1].len = remainder;
			spi_message_add_tail(&transfer_1[1], &msg);
		}
	}
	else{
		transfer_1[0].tx_buf =(txbuf==NULL)?NULL: txbuf;
		transfer_1[0].rx_buf =(rxbuf==NULL)?NULL: rxbuf;
		transfer_1[0].len = len;
		spi_message_add_tail(&transfer_1[0], &msg);
	}
	if(INNODev_sync(INNODev,&msg))
		return INNO_GENERAL_ERROR;	
	else
		return INNO_NO_ERROR;
}


#ifdef MTK_SPI
bool spi_mode_setting(SPI_MODE_T pSetMode)                 
{
	struct mt_chip_conf* spi_par;
	switch (pSetMode.mode)
	{
		case SPI_MODE_DEASSERT:
			// deassert mode is conflict with pause mode.
			spi_par= (struct mt_chip_conf*)(INNODev->spi->controller_data);
			if(!spi_par){
				inno_err("config fail");
				return -1;
			}
			if (pSetMode.bEnable){
				// Note:pause mode and deassert mode can't use togther
				// Enable deassert mode.
				spi_par->deassert=1;	
			}
			else{
				spi_par->deassert=0;	
			}
			spi_setup(INNODev->spi);
			break;
		case SPI_MODE_PAUSE:
			// pause mode is conflict with deassert mode.
			spi_par= (struct mt_chip_conf *)(INNODev->spi->controller_data);
			if(!spi_par){
				inno_err("spi config fail");
				return -1;
			}
			if (pSetMode.bEnable){
				// ASSERT(spi_internal_state.pause_mode == false);
				// Enable deassert mode.
				spi_par->pause=1;	
			}
			else{
				spi_par->pause=0;	
			}
			spi_setup(INNODev->spi);
			break;
		case SPI_MODE_GET_TICK:
			break;
		case SPI_MODE_ULTRA_HIGH:
			break;
		case SPI_MODE_SLOW_DOWN:
			break;
		default:
			//ASSERT(0);
			break;
	}
	return 0;
}
#endif
//unsigned char cmmb_spi_src_buffer_all[32]={0};
/*
 * Write one byte to SPI bus without touching CS
 *
 * Parameter:
 *		data <in>		: the data write to SPI bus
 *
 */
INNO_RET INNO_SPI_Write_One_Byte_NoCS(unsigned char data)
{  
	INNO_RET ret= INNO_NO_ERROR;       
	if(cmmb_spi_src_buffer_all==NULL)
			return INNO_GENERAL_ERROR;
	cmmb_spi_src_buffer_all[0]=data;

#ifdef MT6589_CMMB_SPI_CONFIG
	INNO_SPI_Mode(0);
#endif

	ret = INNODev_xfer(cmmb_spi_src_buffer_all,NULL, 1);
	return ret;
}
/*
 * Write CMD and read response. between the two transfer need release CS one times 
 *
 * Parameter:
 *		cmd <in>		: the data write to SPI bus
 *           rsp  < out>            : the data of the chip response
 *
 */
INNO_RET INNO_SPI_Write_cmd_rsp(unsigned char cmd,unsigned char *rsp)       
{ 
	INNO_RET ret = INNO_NO_ERROR;
	//    cmmb_spi_src_buffer_all[0]=cmd;
	ret =INNO_SPI_Write_One_Byte_NoCS(cmd);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Write_One_Byte_NoCS fail");
		return ret;
	}

	udelay(100);
	ret =INNO_SPI_Read_One_Byte_NoCS(rsp);	
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Read_One_Byte_NoCSfail");
		return ret;
	}
	return INNO_NO_ERROR;
}

/*
 * Read one byte from SPI bus without touching CS
 *
 * Parameter:
 *		data <out>		: the data read from SPI bus 
 *
 */
INNO_RET INNO_SPI_Read_One_Byte_NoCS(unsigned char *data)
{    	
	INNO_RET ret=INNO_NO_ERROR;
	if(cmmb_spi_src_buffer_all==NULL)
			return INNO_GENERAL_ERROR;
	cmmb_spi_src_buffer_all[0] =0;
	
#ifdef MT6589_CMMB_SPI_CONFIG
	INNO_SPI_Mode(1);
#endif

	ret = INNODev_xfer(NULL,cmmb_spi_src_buffer_all, 1);
	*data=cmmb_spi_src_buffer_all[0];
	return ret;
}
INNO_RET INNO_SPI_Mode(int mode)
{
	struct mt_chip_conf* spi_par;
	
	spi_par =&spi_conf;
		if(!spi_par){
			inno_err("spi config fail");
			return INNO_GENERAL_ERROR;
		}

	if(1 == mode)
	{
		spi_par->com_mod = DMA_TRANSFER;
		//inno_msg("change to dma mode");
	}
	else
	{
		spi_par->com_mod = FIFO_TRANSFER;
		//inno_msg("change to fifo mode");
	}
	
	if(spi_setup(INNODev->spi)){
				inno_err("spi_setup fail");
				return INNO_GENERAL_ERROR;
	}
	
	return INNO_NO_ERROR;
}

/*************************************************************************
 * SPI
 *************************************************************************/ 
/*
 * Init and Deinit SPI interface 
 *
 * Parameter:
 *		enable <in>		: enable or disable SPI interface
 *
 * Example:
 *		INNO_SPI_Init(1);	// Enable SPI interface
 *		INNO_SPI_Init(0);	// Enable SPI interface
 */
INNO_RET INNO_SPI_Init(int enable)
{
	mutex_lock(&inno_spi_mutex);
	if(enable){
		struct mt_chip_conf* spi_par;
		cmmb_spi_src_buffer_all = kmalloc(CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES,GFP_KERNEL);
		if(cmmb_spi_src_buffer_all== NULL){
			inno_err("error kmalloc fail cmmb_spi_src_buffer_all");
			mutex_unlock(&inno_spi_mutex);
			return INNO_GENERAL_ERROR;
		}
                INNODev->spi->controller_data =(void*)&spi_conf; 
                spi_par =&spi_conf;
		if(!spi_par){
			inno_err("spi config fail");
			mutex_unlock(&inno_spi_mutex);
			return INNO_GENERAL_ERROR;
		}
		spi_par->setuptime = 15;
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

		spi_par->pause = 0;
		spi_par->finish_intr = 1;
		spi_par->deassert = 0;

		if(spi_setup(INNODev->spi)){
			inno_err("spi_setup fail");
			mutex_unlock(&inno_spi_mutex);
			return INNO_GENERAL_ERROR;
		}

#if 0           //check spi register
		spi_cfg0 = (int*)ioremap(0x700B2000,4);
		spi_cfg1 = (int*)ioremap(0x700B2000+0x4,4);
		spi_cmd = (int*)ioremap(0x700B2000+0x18,4);  
		inno_msg("SPI CFG0:%08x, SPI_CFG1:%08x, SPI_CMD:%08x\n",*spi_cfg0,*spi_cfg1,*spi_cmd);
		iounmap(spi_cfg0);
		iounmap(spi_cfg1);
		iounmap(spi_cmd);
#endif
	}
	else{
		kfree(cmmb_spi_src_buffer_all);
		cmmb_spi_src_buffer_all =NULL; 
	}
	mutex_unlock(&inno_spi_mutex);
	return INNO_NO_ERROR;
}

/*
 * Write multi bytes to SPI bus without touching CS
 *
 * Parameter:
 *		buffer <in>	: the buffer point to store data which will be write to spi bus
 *		len <in>		: how many bytes will be write by SPI 
 *
 */
INNO_RET INNO_SPI_Write_Bytes_NoCS(unsigned char *buffer, int len)
{
	INNO_RET ret = INNO_NO_ERROR;
	unsigned char* tx_buf=NULL, *rx_buf=NULL;
#ifdef MT6589_CMMB_SPI_CONFIG
	INNO_SPI_Mode(0);
#endif

#ifdef MT6589_CMMB_SPI_CONFIG
	tx_buf = buffer;
	if( len > MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES )
	{
		int i;
		int pkt_cnt = len / MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES;
		int remainder = len % MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES;
		for(i=0; i < pkt_cnt; i++)
		{
			ret += INNODev_xfer(tx_buf,rx_buf, MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES);
			tx_buf += MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES;		
		}
		if(remainder)
		{
			ret += INNODev_xfer(tx_buf,rx_buf, remainder);
		}
	}
	else
	ret = INNODev_xfer(tx_buf,rx_buf, len);
#else
	if(len>CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){	
             tx_buf = buffer;
             rx_buf =NULL;
	}
	else{
		if(cmmb_spi_src_buffer_all==NULL)
			return INNO_GENERAL_ERROR;
		if(buffer!=cmmb_spi_src_buffer_all)
			memcpy(cmmb_spi_src_buffer_all,buffer,len);
             tx_buf = cmmb_spi_src_buffer_all;
             rx_buf =NULL;
	}
	ret = INNODev_xfer(tx_buf,rx_buf, len);
#endif
	return ret;
}
/*
 * Read several bytes from SPI bus 
 *
 * Parameter:
 *		buffer <out>	: the buffer point to store data from SPI bus 
 *		len <in>		: how many bytes will be read from SPI 
 *
 * NOTE:
 *		Please make sure SPI chip select is active(low) in the whole process of read bytes from SPI 
 */

INNO_RET INNO_SPI_Read_Bytes_NoCS(unsigned char *buffer, int len)       
{
	INNO_RET ret =INNO_NO_ERROR;
	unsigned char *tx_buf=NULL, *rx_buf=NULL;

#ifdef MT6589_CMMB_SPI_CONFIG
	INNO_SPI_Mode(1);
#endif

	if(len>CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){	         
              tx_buf = NULL;
              rx_buf = buffer;
  		ret = INNODev_xfer(tx_buf,rx_buf, len);
	}
	else{                                                                        //buffer may not be physical continue,so use buffer cmmb_spi_src_buffer_all
		if(cmmb_spi_src_buffer_all==NULL)
			return INNO_GENERAL_ERROR;
              tx_buf = NULL;
              rx_buf = cmmb_spi_src_buffer_all;
		ret = INNODev_xfer(tx_buf,rx_buf, len);
		if(ret == INNO_NO_ERROR)
         		memcpy(buffer,cmmb_spi_src_buffer_all,len);             
	}	  	
       return ret;
}

/*
 * Set and Reset SPI GPIO pins 
 *
 * Parameter:
 *		enable <in>		: Set and Reset SPI GPIO pins 
 *
 * Example:
 *		INNO_SPI_Init(1);	// Set SPI GPIO pins 
 *		INNO_SPI_Init(0);	// Reset SPI GPIO pins 
 */
INNO_RET INNO_SPI_GPIO_Set(int enable)
{
	mutex_lock(&inno_spi_mutex);
	if(enable)
		{
			mt_set_gpio_mode(GPIO_SPI_CS_PIN, GPIO_SPI_CS_PIN_M_SPI1_CSN);
			mt_set_gpio_pull_enable(GPIO_SPI_CS_PIN, GPIO_PULL_ENABLE);
			mt_set_gpio_pull_select(GPIO_SPI_CS_PIN, GPIO_PULL_UP);
			
			mt_set_gpio_mode(GPIO_SPI_SCK_PIN, GPIO_SPI_SCK_PIN_M_CLK);
			mt_set_gpio_pull_enable(GPIO_SPI_SCK_PIN, GPIO_PULL_ENABLE);
			mt_set_gpio_pull_select(GPIO_SPI_SCK_PIN, GPIO_PULL_DOWN);
			
			mt_set_gpio_mode(GPIO_SPI_MISO_PIN, GPIO_SPI_MISO_PIN_M_SPI1_MI);	
			mt_set_gpio_pull_enable(GPIO_SPI_MISO_PIN, GPIO_PULL_ENABLE);
			mt_set_gpio_pull_select(GPIO_SPI_MISO_PIN, GPIO_PULL_DOWN);
			
			mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_SPI_MOSI_PIN_M_SPI1_MO);	
			mt_set_gpio_pull_enable(GPIO_SPI_MOSI_PIN, GPIO_PULL_ENABLE);
			mt_set_gpio_pull_select(GPIO_SPI_MOSI_PIN, GPIO_PULL_DOWN);
			
		}
	else
		{
			mt_set_gpio_mode(GPIO_SPI_CS_PIN, GPIO_SPI_CS_PIN_M_GPIO);
			mt_set_gpio_dir(GPIO_SPI_CS_PIN, GPIO_DIR_IN);
			mt_set_gpio_pull_enable(GPIO_SPI_CS_PIN, GPIO_PULL_DISABLE);
			
			mt_set_gpio_mode(GPIO_SPI_SCK_PIN, GPIO_SPI_SCK_PIN_M_GPIO);
			mt_set_gpio_dir(GPIO_SPI_SCK_PIN, GPIO_DIR_IN);
			mt_set_gpio_pull_enable(GPIO_SPI_SCK_PIN, GPIO_PULL_DISABLE);
			
			mt_set_gpio_mode(GPIO_SPI_MISO_PIN, GPIO_SPI_MISO_PIN_M_GPIO);
			mt_set_gpio_dir(GPIO_SPI_MISO_PIN, GPIO_DIR_IN);
			mt_set_gpio_pull_enable(GPIO_SPI_MISO_PIN, GPIO_PULL_DISABLE);
			
			mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_SPI_MOSI_PIN_M_GPIO);
			mt_set_gpio_dir(GPIO_SPI_MOSI_PIN, GPIO_DIR_IN);
			mt_set_gpio_pull_enable(GPIO_SPI_MOSI_PIN, GPIO_PULL_DISABLE);
		}
	
	mutex_unlock(&inno_spi_mutex);
	return INNO_NO_ERROR;
	
}
