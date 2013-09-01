
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <linux/kdev_t.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/device.h>

#include "if208.h"
#include "if101_control_proto.h"
#include "mmis_cmdlist.h"
#include "inno_spi.h"


#ifdef MTK_SPI
#include <linux/spi/spi.h>
extern struct INNODev_data* INNODev;
#endif
#include <cust_cmmb.h>

#define MAKEWORD(low,high)	((unsigned short)((unsigned char)(low)) | (((unsigned short)(unsigned char)(high))<<8))
#define CMD_SET_FREQUENCY2		0x1  
#define CMD_SET_CHANNEL_CONFIG2		0x2
#define CMD_CLOSE_ALL_CHANNEL2		0x3
#define CMD_GET_CHANNEL_CONFIG2		0x4
#define CMD_GET_FW_VER2			0x5
#define CMD_GET_FW_CHECK_SUM2		0x6

//#define CMMB_DEBUG		1

struct spi_transfer transfer_1[2];     

extern int flag_memset;
extern int flag_close;
extern int  flag_spi_ok;                                   // spi fail, do not to comminucate
extern unsigned char* cmmb_spi_src_buffer_all;
#if 1         
int g_log_ch  =39;
int channel_indicate;
#endif


struct mutex inno_spi_mutex;
struct mutex inno_mtkspi_mutex;

inno_spi_driver_t 	inno_spi_driver;
extern inno_cmdset_t        inno_cmdset_ops;

extern void inno_chip_reset(void);
static INNO_RET SPI_Test(void);
INNO_RET INNO_GetFirmwareVersion(unsigned char *version);
//static INNO_RET SPI_inno_Test(void);
INNO_RET INNO_InterfaceTest(void);
int INNO_GetFirmwareBytes(void);
extern ssize_t INNODev_sync(struct INNODev_data *INNODevS, struct spi_message *message);
extern INNO_RET  INNODev_xfer(unsigned char *txbuf,unsigned char *rxbuf, int len);
INNO_RET INNO_Send_Cmd(unsigned char *cmd);
INNO_RET INNO_Read_Rsp(unsigned char *rsp);
INNO_RET INNO_SPI_Read_Byte_Type2(unsigned long addr, unsigned char *buffer, int len);
INNO_RET INNO_SPI_Read_WORD_Type2(unsigned long addr, unsigned long *data);
INNO_RET INNO_SPI_Write_WORD_Type2(unsigned long addr, unsigned long data);
INNO_RET INNO_SPI_Write_Byte_Type2(unsigned long addr, unsigned char *data, int len);  //cmd = 0x73
INNO_RET INNO_SPI_Write_One_Byte(unsigned char data);
//static INNO_RET INNO_SPI_Read_One_Byte(unsigned char *data);

/* global variable */
struct inno_device  g_inno_dev = {
	.cfg		=	{
		.spi_interface 	= NORMAL_SPI,
		.spi_clk_mode 	= SPI_CLK_MODE_0,
		.intr_type		= FALLING_EDGE,
		.trans_size		= TRANS_BURST,
	},
	.spi_driver	=   &inno_spi_driver,
	.cmdset		=   &inno_cmdset_ops,
};

//#define CMCC_MSG
//#define	IRQ_BUG_DEBUG
#ifdef	IRQ_BUG_DEBUG
int irq_bug_debug = 0;
#endif

#ifdef  _check_block_time                        
int intCount=0;                //count irq to check block issue
#endif
static struct work_struct	irq_work;
static struct workqueue_struct	*irq_wq=NULL;

/*=============== UAM =============*/
OP_MODE_T	OperationMode_UAM;
SCARD_CARD_CAPABILITIES  cardCapabilities_UAM;
int INNO_Send_UAM_Cmd(unsigned char *cmd, int cmdLen);
static int INNO_Read_UAM_Rsp(unsigned char *rsp, unsigned int *rsplen);
int inno_init_uam(void);
/*=================================
 * Interrupt relate
 *================================*/
extern int inno_irq_setup(void (*handler)(void));
extern int inno_irq_open(void);
extern void inno_irq_release(void);
static void inno_delay_ms(int ms);
static void inno_intr_handler(void)
{
#ifdef  _check_block_time                        
	intCount++;
#endif

#ifdef	IRQ_BUG_DEBUG
	if(irq_wq)
	{
		irq_bug_debug++;
		//inno_msg("irq count=%d",irq_bug_debug);
#if 0
		if((irq_bug_debug>=120) && ((irq_bug_debug%60)==0))
		{
			inno_err("inno_intr_handler: irq worker no response, count=%d!", irq_bug_debug);
			//show_state();
		}
		//BUG_ON(irq_bug_debug>100);
#else 
		if(irq_bug_debug >1)
			inno_msg("Warning:irq worker lose %d irq to handle", irq_bug_debug-1);
#endif
	}
#endif
	//	inno_msg("--- IF208 hardware interrupt trigger ---");	

	if(irq_wq)
		queue_work(irq_wq, &irq_work);
	inno_irq_open();                          //open irq manuly
	return ;
}

static int inno_do_request(int lgx_id)                         //lgx_id  == channel id
{
	int ret = 0;
	struct inno_lgx* lgx = g_inno_dev.lgxs[lgx_id];
	struct inno_buffer* buffer = &lgx->inno_buffer;
	u32 size;
	inno_cmdset_t* mmis_cmd = g_inno_dev.cmdset;

	down(&buffer->sem);
	buffer->valid = 0;
	up(&buffer->sem);

	// get logic channel available data length
	size = mmis_cmd->get_lgx_length(lgx_id);
	inno_dbg("%s size = 0x%x, bufsize: 0x%x", lgx->ids->name, size, buffer->bufsize);
	if(size > (buffer->bufsize)){
		inno_err("error: %s data size 0x%x, buffer is not enough.", lgx->ids->name, size);
		return -EINVAL;
	}
	else if(size<=0){                     // fix division by zero 
		inno_err("mfs len <=0");
		return -EINVAL;
	}

	down(&lgx->sem);
	lgx->bytes_available = size;
	up(&lgx->sem);

	down(&buffer->sem);
	// check spi issue for transfer bytes error shift 4 bytes
	if(flag_memset){
		inno_msg("memset the mfs buffer before fetch data,size=%d",size);
		memset(buffer->start,0xfe,size); 
	}
	if(buffer->start ==NULL){    //check INNO_SPI_Read_Byte_Type2 memcpy KE ERROR
		inno_err("buffer->start ==NULL");
		up(&buffer->sem);
		return -EINVAL;		
	}

	ret =mmis_cmd->fetch_lgx_data(lgx_id, (u8*)buffer->start, size);	
	if(ret){
		inno_err("fetch_lgx_data fail");
		up(&buffer->sem);
		return -EINVAL;
	}

	buffer->valid = 1;
	inno_dbg("[inno_do_request] size = %d",size);
	buffer->valid_size = size;
	buffer->own_id = lgx_id;
	up(&buffer->sem);

	wake_up_interruptible(&lgx->read_wait);

	return ret;
}

static int inno_uam_request(void)
{
	int ret = 0;
	struct inno_lgx* lgx = g_inno_dev.lgxs[UAM];
	struct inno_buffer* buffer = &lgx->inno_buffer;
	u32 size;
	inno_cmdset_t* mmis_cmd = g_inno_dev.cmdset;

	down(&buffer->sem);
	buffer->valid = 0;
	up(&buffer->sem);

	// get logic channel available data length
	size = mmis_cmd->get_lgx_length(UAM);
	inno_dbg("UAM size = 0x%x, bufsize: 0x%x", size, buffer->bufsize);
	if(size > (buffer->bufsize)){
		inno_err("error: %s data size 0x%x, buffer is not enough.", lgx->ids->name, size);
		return -EINVAL;
	}
	else if(size<=0){            // fix division by zero
		inno_err("uam data size <=0");
		return -EINVAL;
	}

	down(&lgx->sem);
	lgx->bytes_available = size;
	up(&lgx->sem);

	down(&buffer->sem);
	if(buffer->start ==NULL){    //check INNO_SPI_Read_Byte_Type2 memcpy KE ERROR
		inno_err("buffer->start ==NULL");
		up(&buffer->sem);	
		return -EINVAL;		
	}
	ret=mmis_cmd->fetch_lgx_data(UAM, (u8*)buffer->start, size);
	if(ret){
		inno_err("fetch_lgx_data fail");
		up(&buffer->sem);	
		return -EINVAL;
	}
	buffer->valid = 1;
	buffer->valid_size = size;
	buffer->own_id = UAM;
	up(&buffer->sem);	
#ifdef UAM_COMPLETE
	complete(&lgx->uam_complete);
#else
	wake_up_interruptible(&lgx->uam_wait);	
#endif
	return ret;
}

static int inno_uam_err_request(void)
{
	int ret = 0;
	struct inno_lgx* lgx = g_inno_dev.lgxs[UAM];
	struct inno_buffer* buffer = &lgx->inno_buffer;
	struct inno_sys_state state;                  

	down(&buffer->sem);
	buffer->valid = 0;
	up(&buffer->sem);

	inno_msg("Check fw error status");
	state.stattype = FW_ERR_STATUS;
	ret =inno_get_system_state(&state);
	if(ret){
		inno_err("inno_get_system_state fail");
		return -EINVAL;
	}

	down(&lgx->sem);
	lgx->bytes_available = 4;
	up(&lgx->sem);

	down(&buffer->sem);

	if(buffer->start ==NULL){    //check INNO_SPI_Read_Byte_Type2 memcpy KE ERROR
		inno_err("buffer->start ==NULL");
		up(&buffer->sem);	
		return -EINVAL;		
	}
	memcpy((u8*)buffer->start,(u8*)&(state.statdata.fw_err_status),4);
	inno_dbg("check buffer->start = %d",*(u32*)buffer->start);
	buffer->valid = 1;
	buffer->valid_size = 4;
	buffer->own_id = UAM;

	up(&buffer->sem);	
	wake_up_interruptible(&lgx->read_wait);	
	return ret;
}

/*==========================
 *  Kernel Thread
 *=========================*/
/*
 * irq thread ( CALL THREAD-1)
 *
 */
static void irq_work_handler(struct work_struct *_work)
{
	inno_device_t* inno_dev = &g_inno_dev;
	inno_cmdset_t* mmis_cmd = inno_dev->cmdset;
	u32 intr_status=0;                       //xingyu 0922
	int ret=0;
#ifdef	IRQ_BUG_DEBUG
	irq_bug_debug=0;                              // clear the irq count
#endif

	inno_msg("Enter");                            // check eint occur every 1s
	down(&inno_dev->sem);                        // xingyu 0422 add for irq corrupt with close KE issue
	if(flag_close){
		inno_msg("Warning:close already");
		up(&inno_dev->sem); 
		return;
	}		

	intr_status = mmis_cmd->get_intr_status();    // get the interrrupt status from chip
	inno_dbg("======== intr_status [%x] ============", intr_status);	
	// we only interest in opened logic channel

	if(g_inno_dev.lgxs[UAM] && (intr_status & UAM_DATA_RDY)){
		inno_msg("UAM data ready");
		ret =inno_uam_request();
		if(ret){
			inno_err("inno_uam_request fail");
		}
	}
	if(g_inno_dev.lgxs[LG0] && (intr_status & LG0_DATA_RDY)){
		inno_msg("LG0 data ready");
		ret =inno_do_request(LG0);
		if(ret){
			inno_err("inno_do_request(LG0) fail");
		}		
	}
	if(g_inno_dev.lgxs[LG1] && (intr_status & LG1_DATA_RDY)){
		inno_msg("LG1 data ready");
		ret =inno_do_request(LG1);
		if(ret){
			inno_err("inno_do_request(LG1) fail");
		}
	}
	if(g_inno_dev.lgxs[LG2] && (intr_status & LG2_DATA_RDY)){                    //xingyu 0922
		inno_msg("LG2 data ready");
		ret =inno_do_request(LG2);
		if(ret){
			inno_err("inno_do_request(LG2) fail");
		}
	}
	if(intr_status & FW_ERR_DATA_RDY){              // uam error 
		inno_msg("Check fw uam error status");
		ret =inno_uam_err_request();
		if(ret){
			inno_err("inno_uam_err_request fail");
		}
	}

	//else{
	if( !(intr_status & FW_ERR_DATA_RDY) & 
			!(intr_status & LG2_DATA_RDY) &                       //xingyu 0922
			!(intr_status & LG1_DATA_RDY) &
			!(intr_status & LG0_DATA_RDY) &
			!(intr_status & UAM_DATA_RDY)){
		inno_err("intr_status error,intr_status=%d",intr_status);
	}

	//	}

	up(&inno_dev->sem);                             // xingyu 0422 add for irq corrupt with close KE issue
	inno_msg("Leave");
}

extern int inno_check_firmware(void);
//static int ifUAMOpen = 0;

/*
 * return 0 -- successful
 */
/*
   1, power control
   2, spi init
   4, called by file open function
 */
int innodev_startup(inno_device_t* inno_dev)           
{
	INNO_RET ret = INNO_NO_ERROR;
	//int ret_uam = 0;
	down(&inno_dev->sem);

	inno_msg("====== innodev_startup ===== use count = %d", inno_dev->use_count);
	if(inno_dev->use_count++ != 0)                      //only the first time to init the hw device
		goto out;

	inno_msg("!! startup innodev !!");

	INNO_SPI_GPIO_Set(1); 
	cust_cmmb_power_on();           // no need delay,according to innofidei doc,delay in reset function

	ret =INNO_SPI_Init(1);
	if(ret !=INNO_NO_ERROR){
		inno_err("INNO_SPI_Init fail");
		goto out;
	}
	//ret = inno_check_firmware();                       //change to user space download fw

#ifdef	IRQ_BUG_DEBUG
	irq_bug_debug=0;
#endif
	/*            //work queue is created when module init
		      INIT_WORK(&(irq_work), irq_work_handler);
	// start irq thread
	irq_wq = create_workqueue("inno_irqtask");
	if(!irq_wq){
	ret = -ENOMEM;
	inno_err("inno_startup: create_workqueue failed");
	goto err2;
	}
	 */

	//ret = inno_irq_setup(inno_intr_handler);
#if 0
	ret = INNO_InterfaceTest();
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_InterfaceTest fail  ret =%d",ret);
		return ret;
	}
#endif     
	goto out;

	//err1:
	/*
	   if(irq_wq){
	   destroy_workqueue(irq_wq);
	   irq_wq = NULL;
	   }
	 */
	//err2:
	/*
#ifndef CONFIG_INNODEV_SPIONLY
inno_dev->i2c_driver->exit();
#endif
inno_dev->spi_driver->uninit();
	 */
out:
#if 0                         // xingyu	 cut this
	if((inno_dev->lgxs[UAM]!= NULL)&&(ifUAMOpen == 0)){
		ret_uam = inno_init_uam();
		inno_msg("the modification take effect");

		if(ret_uam != INNO_SUCCESS)
			inno_err("Init UAM fail");
		else{
			inno_msg("Init UAM OK");
			ifUAMOpen = 1;  
		}

	}
	mdelay(100);
	if(ret)
		inno_dev->use_count--;
#endif	
	up(&inno_dev->sem);
	if(ret == INNO_NO_ERROR)
		return 0;
	else
		return -1;
}

int innodev_shutdown(inno_device_t* inno_dev)
{
	//down(&inno_dev->sem);

	inno_msg("======innodev_shutdown ===== use count = %d", inno_dev->use_count);
	if(!(--inno_dev->use_count)){
		inno_msg("!! shutdown innodev !!");
		//inno_irq_release();                                        //xingyu 0706  irq release in lgx_close
		/* Clean up the kernel thread 
		   if(irq_wq){
		   destroy_workqueue(irq_wq);
		   irq_wq = NULL;
		   }
		 */
		/*
#ifndef CONFIG_INNODEV_SPIONLY
		// uninitialize i2c driver & spi driver
		if(inno_dev->i2c_driver && inno_dev->i2c_driver->exit)
		inno_dev->i2c_driver->exit();
#endif
if(inno_dev->spi_driver && inno_dev->spi_driver->uninit)
inno_dev->spi_driver->uninit();
		 */
		if(INNO_SPI_Init(0)){
			inno_err("INNO_SPI_Init deinit fail");
			return -1;
		}
		// cut down if101 chip power supply
		cust_cmmb_power_off();
		//		ifUAMOpen = 0;
		INNO_SPI_GPIO_Set(0); 
	}

	//up(&inno_dev->sem);
	return 0;
}
EXPORT_SYMBOL(innodev_startup);
EXPORT_SYMBOL(innodev_shutdown);


/*============================================
 * inno device initialization/uninitialization 
 *==========================================*/

int innodev_init(void)                                    //create  workqueue and eint thread
{
	int ret = 0;
	inno_msg("======innodev_init ===== use count = %d", g_inno_dev.use_count);
	sema_init(&g_inno_dev.sem,1);
        mutex_init(&inno_spi_mutex);
        mutex_init(&inno_mtkspi_mutex);
        

	down(&g_inno_dev.sem);
	g_inno_dev.use_count = 0;                                              
	up(&g_inno_dev.sem);

	INIT_WORK(&(irq_work), irq_work_handler);
	// start irq thread
#ifdef CONFIG_SMP
	irq_wq = create_singlethread_workqueue("inno_irqtask");
#else
	irq_wq = create_workqueue("inno_irqtask");
#endif

	if(!irq_wq){
		ret = -ENOMEM;
		inno_err("innodev_init: create_workqueue failed");
	}
	return ret;
}

void innodev_uninit(void)
{
	inno_msg("======innodev_uninit ===== use count = %d", g_inno_dev.use_count);
#if 0                                                            //xingyu 0706   no need to shutdown	
	if(!g_inno_dev.use_count)
		innodev_shutdown(&g_inno_dev);
#endif	
	if(irq_wq){
		destroy_workqueue(irq_wq);
		inno_msg("destroy_workqueue END");
		irq_wq = NULL;
	}
}

EXPORT_SYMBOL(innodev_init);
EXPORT_SYMBOL(innodev_uninit);


/**************************************
 *
 * IF101 & AP Communication API
 * 
 * ***********************************/

static void inno_delay_ms(int ms)
{
	if( ms>=10 ){
		msleep(ms);
	}else
		mdelay(ms);
}

/*=====================================
 *
 * API (AP Interface) Functions
 *
 * ===================================*/

/*
 */
int INNO_GetChannelConfig(struct inno_channel_config* cfg)
{
	/* 
	   CHIP_V5 CMD Description:
	   Get Channel Config Command

	   CMD[0] = CMD_GET_CHANNEL_CONFIG
	   CMD[1] = channel_num
	   CMD[2] = 0
	   CMD[3] = 0
	   CMD[4] = 0
	   CMD[5] = 0
	   CMD[6] = 0
	   CMD[7] = 0

	   RSP[0] = CMD_GET_CHANNEL_CONFIG
	   RSP[1] = Logical Channel Number
	   RSP[2] = Logical Channel Start Timeslot
	   RSP[3] = Logical Channel Timeslot Count
	   RSP[4] = Logical Channel Demodulator Config
	   RSP[5] = Subframe_ID
	 */
	//int ret = INNO_PARAM_ERR;
	INNO_RET ret = INNO_NO_ERROR;
	unsigned char cmd[8] = {0};
	unsigned char rsp[8] = {0};
	int retry = 0;
	inno_msg("[ALXiao] INNO_GetChannelConfig channel_indicate: %d.",cfg->ch_id);
	for(retry = 0; retry < 20; retry ++){
		// Send CMD
		cmd[0] = CMD_GET_CHANNEL_CONFIG2;
		cmd[1] = cfg->ch_id+1;                       //note:channed_id +1

		mutex_lock(&inno_spi_mutex);
		ret = INNO_Send_Cmd(cmd);
		if(ret != INNO_NO_ERROR)
		{
			mutex_unlock(&inno_spi_mutex);
			if(ret == INNO_TIMEOUT_ERROR)
				continue;
			else{                                     //spi transfer error should not to retry
				inno_err("INNO_Send_Cmd fail");
				return -1;
			}
		}

		// Get RSP
		ret = INNO_Read_Rsp(rsp);
		mutex_unlock(&inno_spi_mutex);

		if(ret != INNO_NO_ERROR){
			if(ret == INNO_TIMEOUT_ERROR)
				continue;
			else{
				inno_err("INNO_Read_Rsp fail");
				return INNO_GENERAL_ERROR;
			}
		}
		else
			break;
	}
	if(retry >=20){                  // check timeout error
		ret = INNO_TIMEOUT_ERROR; 
		inno_err("INNO_Send_Cmd timeout");
		return -1;
	}      
	cfg->ts_start = rsp[2];
	cfg->ts_count = rsp[3];
	cfg->modulate = ((rsp[4]& 0xc0)>>6) &0x03 ;
	cfg->rs = ((rsp[4] & 0x30)>>4) &0x03 ;
	cfg->itlv = ((rsp[4] & 0x0c)>>2) & 0x03;
	cfg->ldpc = rsp[4] & 0x03;
	cfg->subframe_ID= rsp[5];
	inno_dbg("GetChannelConfig: ts_start = %d,  ts_count = %d",rsp[2], rsp[3]);            
	inno_dbg("GetChannelConfig: demod = %d,subframe_ID=%d",rsp[4],rsp[5]);            

	if(ret == INNO_NO_ERROR)
		return 0;
	else
		return -1;
} 
EXPORT_SYMBOL(INNO_GetChannelConfig);


/*
 * Set logic channel configure
 */
int inno_set_channel_config(struct inno_channel_config* cfg)
{
	unsigned char cmd[8] = {0};
	//int ret;
	INNO_RET ret = INNO_NO_ERROR;
	int retry = 0;

	if(!cfg){
		ret = INNO_PARAM_ERR;
		goto out;
	}
	cmd[0] = CMD_SET_CHANNEL_CONFIG2;

	if(cfg->ch_close) //shut down logic channel
	{	 
		switch(cfg->ch_id){
			case CFG_LGCH0:
				cmd[1] = 1;				// channel 1 for LG0
				cmd[2] = 0; 				//
				cmd[3] = 0; 				// close channel 
				cmd[4] = 0; 				//
				break;
			case CFG_LGCH1:
				cmd[1] = 2;
				cmd[2] = 0;
				cmd[3] = 0;
				cmd[4] = 0;
				break;
			case CFG_LGCH2:                         //xingyu 0922
				cmd[1] = 3;
				cmd[2] = 0;
				cmd[3] = 0;
				cmd[4] = 0;
				break;
		}
	}
	else // open channel
	{
		switch(cfg->ch_id){ 
			case CFG_LGCH0:
				if(g_log_ch == 39)
				{            
					channel_indicate = 1;
					inno_dbg("[ALXiao] Channel 39 use 1 for setchannel config arry[1]");
				}
				cmd[1] = 1;	 			// channel 1 for LG0		  
				cmd[2] = cfg->ts_start;
				cmd[3] = cfg->ts_count;
				cmd[4] = (cfg->modulate<<6) | (cfg->rs <<4) | (cfg->itlv<<2) | cfg->ldpc;
				cmd[5] = 0;
				cmd[6] = cfg->subframe_ID;
				break;
			case CFG_LGCH1:
				if(g_log_ch == 39)
				{            
					channel_indicate = 2;
					inno_dbg("[ALXiao] Channel 39 use 2 for setchannel config arry[1]");
				}
				cmd[1] = 2;				 // channel 2 for LG1	  
				cmd[2] = cfg->ts_start;
				cmd[3] = cfg->ts_count;
				cmd[4] = (cfg->modulate<<6) | (cfg->rs <<4) | (cfg->itlv<<2) | cfg->ldpc;
				cmd[5] = 0;
				cmd[6] = cfg->subframe_ID;
				break;
			case CFG_LGCH2:                          //xingyu 0922
				if(g_log_ch == 39)
				{            
					channel_indicate = 3;
					inno_dbg("[ALXiao] Channel 39 use 3 for setchannel config arry[1]");
				}
				cmd[1] = 3;				 // channel 3 for LG2	  
				cmd[2] = cfg->ts_start;
				cmd[3] = cfg->ts_count;
				cmd[4] = (cfg->modulate<<6) | (cfg->rs <<4) | (cfg->itlv<<2) | cfg->ldpc;
				cmd[5] = 0;
				cmd[6] = cfg->subframe_ID;
				break;
		}
	}
	mutex_lock(&inno_spi_mutex);
	for(retry = 0; retry < 20; retry ++){
		//		ret = inno_i2c_send_cmd_frame(&cmd_frame);
		ret = INNO_Send_Cmd(cmd);
		if(ret == INNO_NO_ERROR)
			break;
		else if(ret == INNO_TIMEOUT_ERROR)
			continue;
		else{                                   // spi transfer error to return
			mutex_unlock(&inno_spi_mutex);			
			inno_err("INNO_Send_Cmd fail");
			return -1;
		}
		inno_dbg("inno_set_channel_config retry=%d", retry); 
	}
	mutex_unlock(&inno_spi_mutex);
	if(retry>=20){
		inno_err("INNO_Send_Cmd timeout");
		return -1;
	}

	//dump_cmd_frame(&cmd_frame);
	//inno_delay_ms(200);
out:
	if(ret == INNO_NO_ERROR)
		return 0;
	else
		return -1;
}

unsigned long ParseErrStatus(unsigned char status)      //xingyu 0317
{
	unsigned long ret = 0;

	switch ( status )
	{
		case CAS_OK:
			ret = 0x9000;
			break;
		case NO_MATCHING_CAS:   //Can not find ECM data
			ret = NO_MATCHING_CAS;
			break;
		case CARD_OP_ERROR:    //Time out or err
			ret = CARD_OP_ERROR;
			break;
		case MAC_ERR:
			ret = 0x9862;
			break;
		case GSM_ERR:
			ret = 0x9864;
			break;
		case KEY_ERR:
			ret = 0x9865;
			break;
		case KS_NOT_FIND:
			ret = 0x6985;
			break;
		case KEY_NOT_FIND:
			ret = 0x6a88;
			break;
		case CMD_ERR:
			ret = 0x6f00;
			break;
		default:
			break;			
	}
	inno_msg("ret =%x",(unsigned int)ret);	
	return ret;
}
/*
 * Get system information
 */
int inno_get_system_state(struct inno_sys_state* state)
{
	unsigned char data;
	unsigned char ber_data[3] ={0};
	INNO_RET ret = INNO_NO_ERROR;
	//unsigned char cur_freq = 0;

	mutex_lock(&inno_spi_mutex);
#ifdef CMMB_DEBUG	
	ret =INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM30, &data, 1); // check more.
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Read_Byte_Type2 fail");
		mutex_unlock(&inno_spi_mutex);
		return -1;
	}
	inno_msg("signal quality = %d", data);
#endif			
	switch(state->stattype){
		case STATTYPE_SYNC_STATE:
			ret = INNO_SPI_Read_Byte_Type2(OFDM_SYNC_STATE, &data, 1); // check more.
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			if ( (data&0x0f) == 8 )  //8: sync
				state->statdata.sync_state = 1 ;  
			else
				state->statdata.sync_state = 0 ;  
			inno_msg("sync_state = %x", state->statdata.sync_state);
			break;
		case STATTYPE_SIGNAL_STRENGTH:
			ret =INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM16, &data, 1); // check more.
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			state->statdata.signal_strength = data & 0x7F;
			inno_msg("signal_strength = %d", state->statdata.signal_strength);

#ifdef CMMB_DEBUG
			ret = INNO_SPI_Read_WORD_Type2(FETCH_RS_TOTAL,(unsigned long *)&state->statdata.rs_total_count);		
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("rs_total_count = %x", state->statdata.rs_total_count);

			ret =INNO_SPI_Read_WORD_Type2(FETCH_RS_ERR,(unsigned long *)&state->statdata.rs_err_count);		
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("rs_err_count = %x", state->statdata.rs_err_count);			
#endif	
#if 0
			ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM17, &cur_freq, 1);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("[ALXiao] : The current frequency dot is %d.",cur_freq);
			INNO_GetChannelConfig(channel_indicate);
#endif			
			break;
		case STATTYPE_SIGNAL_QUALITY:
			ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM30, &data, 1); // check more.
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			state->statdata.signal_quality= (u32)data;
			inno_msg("signal_quality = %d", state->statdata.signal_quality);
#if 0
			ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM16, &data, 1); // check more.
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			state->statdata.signal_strength = data & 0x7F;
			inno_msg("signal_strength(RSSI) = %d", state->statdata.signal_strength);

			ret=INNO_SPI_Read_WORD_Type2(FETCH_RS_TOTAL,(unsigned long *)&state->statdata.rs_total_count);		
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("rs_total_count = %d", state->statdata.rs_total_count);

			ret=INNO_SPI_Read_WORD_Type2(FETCH_RS_ERR,(unsigned long *)&state->statdata.rs_err_count);		
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("rs_err_count = %d", state->statdata.rs_err_count);			

			ret=INNO_SPI_Read_WORD_Type2(FETCH_LDPC_TOTAL,(unsigned long *)&state->statdata.ldpc_total_count);			
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("ldpc_total_count = %d", state->statdata.ldpc_total_count);

			ret=INNO_SPI_Read_WORD_Type2(FETCH_LDPC_ERR,(unsigned long *)&state->statdata.ldpc_err_count);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("ldpc_err_count = %d", state->statdata.ldpc_err_count);

			ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM22, ber_data, 2);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM19, &ber_data[2], 1);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			state->statdata.BER= ((u32)ber_data[0]) + ((u32)(ber_data[1]<<8))+((u32)(ber_data[2]<<16));
			inno_msg("BER= %d", state->statdata.BER);

			ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM18, &data, 1);  //snr(cn)
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			state->statdata.SNR= (u32)data;
			inno_msg("SNR= %d", state->statdata.SNR);
#endif
			break;
		case STATTYPE_LDPC_TOTAL_COUNT:
			ret=INNO_SPI_Read_WORD_Type2(FETCH_LDPC_TOTAL,(unsigned long *)&state->statdata.ldpc_total_count);			
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("ldpc_total_count = %x", state->statdata.ldpc_total_count);
			break;
		case STATTYPE_LDPC_ERR_COUNT:
			ret=INNO_SPI_Read_WORD_Type2(FETCH_LDPC_ERR,(unsigned long *)&state->statdata.ldpc_err_count);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("ldpc_err_count = %x", state->statdata.ldpc_err_count);
			break;
		case STATTYPE_RS_TOTAL_COUNT:
			ret=INNO_SPI_Read_WORD_Type2(FETCH_RS_TOTAL,(unsigned long *)&state->statdata.rs_total_count);		
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("rs_total_count = %x", state->statdata.rs_total_count);
			break;
		case STATTYPE_RS_ERR_COUNT:
			ret=INNO_SPI_Read_WORD_Type2(FETCH_RS_ERR,(unsigned long *)&state->statdata.rs_err_count);		
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_WORD_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_msg("rs_err_count = %x", state->statdata.rs_err_count);
			break;
		case STATTYPE_BER_COUNT:	
			ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM22, ber_data, 2);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM19, &ber_data[2], 1);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			state->statdata.BER= ((u32)ber_data[0]) + ((u32)(ber_data[1]<<8))+((u32)(ber_data[2]<<16));
			inno_msg("BER= %x", state->statdata.BER);
			break;
		case STATTYPE_SNR_COUNT:	
			ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM18, &data, 1);  //snr(cn)
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			state->statdata.SNR= (u32)data;
			inno_msg("SNR= %x", state->statdata.SNR);
			break;				   
		case FW_ERR_STATUS:                                            //xingyu 0317 add
                        // get uam error from this channel,channel 1(match to fw LG2) for descrambled stream
			ret=INNO_SPI_Read_Byte_Type2(LG2_CW_STATE_REG, &data, 1); 
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			
			state->statdata.fw_err_status = ParseErrStatus(data);			
			break;
		default:
			mutex_unlock(&inno_spi_mutex);
			return -EINVAL;
	}

	mutex_unlock(&inno_spi_mutex);
	return 0;
}


/*
 * if101 power management
 */
// the function is not use
#if 0
int inno_set_pm(struct inno_pm_control* pmctl)
{
	struct inno_cmd_frame cmd_frame = {0};
	int ret = 0;

	cmd_frame.code  = CMD_SET_PM;
	cmd_frame.data[0] = ((pmctl->sys_pause?PM_SYS_SUSPEND_BIT:PM_SYS_RESUME_BIT) | \
			(pmctl->stun_off?PM_STUN_OFF_BIT:PM_STUN_ON_BIT)	|	\
			(pmctl->utun_off?PM_UTUN_OFF_BIT:PM_STUN_ON_BIT)	|	\
			(pmctl->tspwr_off?PM_TSPWR_OFF_BIT:PM_TSPWR_ON_BIT));
	mutex_lock(&inno_spi_mutex);
	//ret = inno_i2c_send_cmd_frame(&cmd_frame); //ALXiao,20101227 , do not need
	mutex_unlock(&inno_spi_mutex);

	return ret;
}
#endif

/*
 * Set Tuner Frequency
 */
int inno_set_frequency_dot(u8 freq_dot)
{
	/*
	   CHIP_V3 and CHIP_V5 CMD Description:
	   Set Tuner Frequency Command

	   CMD[0] = CMD_SET_FREQUENCY
	   CMD[1] = Frequency Dot
	   CMD[2] = 0
	   CMD[3] = 0
	   CMD[4] = 0
	   CMD[5] = 0
	   CMD[6] = 0
	   CMD[7] = 0

	   NO RSP

	 */	
	INNO_RET ret = INNO_GENERAL_ERROR;
	unsigned char cmd[8] = {0,0,0,0,0,0,0,0};
	int retry = 0;

	for(retry = 0; retry < 20; retry ++){
		// Send CMD
		cmd[0] = CMD_SET_FREQUENCY2;		// command
		cmd[1] = freq_dot;
		cmd[2] = 0;	
		mutex_lock(&inno_spi_mutex);
		ret = INNO_Send_Cmd(cmd);
		mutex_unlock(&inno_spi_mutex);
		if(ret == INNO_NO_ERROR)
			break;
		else if(ret == INNO_TIMEOUT_ERROR)
			continue;
		else{                                   // spi transfer error to return
			mutex_unlock(&inno_spi_mutex);			
			inno_err("INNO_Send_Cmd fail");
			return -1;
		}
	}
	if(retry>=20){
		inno_err("INNO_Send_Cmd timeout");
		return -1;
	}
	return ret;
}
#if 0
int inno_set_frequency(u16 freq_mhz)
{
	u8 freq_dot = inno_freq2dot(freq_mhz);
	inno_msg("%s freq=%dMhz", __FUNCTION__, freq_mhz);
	return inno_set_frequency_dot(freq_dot);
}
#endif

#if 0
// not in use
int inno_set_cp_type(u8 cp_type)
{
	int ret = INNO_ERROR;
	return ret;
}
#endif

/*
 * Get version number of if101 fireware  not in use
 */
 #if 0
int inno_get_fireware_version(struct inno_fw_ver* version)
{
	struct inno_cmd_frame cmd_frame = {0};
	struct inno_response_frame resp_frame = {0};
	int ret = INNO_SUCCESS;

	mutex_lock(&inno_spi_mutex);

	cmd_frame.code = CMD_GET_FW_VER2;
	//xingyu  ret = inno_i2c_send_cmd_frame(&cmd_frame);
	//xingyu  dump_cmd_frame(&cmd_frame);
	if(INNO_SUCCESS != ret){
		inno_err("%s send cmd frame failed!", __FUNCTION__);
		goto out;
	}

	//xingyu  ret = inno_i2c_read_response_frame(&resp_frame);
	if(INNO_SUCCESS != ret){
		inno_err("%s read response frame failed!", __FUNCTION__);
		goto out;
	}
	version->major = resp_frame.data[0];
	version->minor = resp_frame.data[2];
	//xingyu  dump_response_frame(&resp_frame);
out:
	mutex_unlock(&inno_spi_mutex);
	return ret;
}

/*
 * Update if101 fireware
 */
int inno_update_fireware(void)
{
	int ret = INNO_SUCCESS;
	return ret;
}
#endif

/*
 * Download fireware bin from user
 *
 * Notice:
 *  - IF101 as SPI Master, generate SPI_CLK & SPI_CS.
 * 	- AP as SPI Slave.  
 *
 * |-----------------------------------------|
 * | B1 | B2 | B3 | B4 | B5 | B6 | N-BYTES...|
 * |-----------------------------------------|
 * | 0Bh|  addr-offset |Dumy|   24KByte      |
 * |-----------------------------------------|
 */
int inno_download_fireware(unsigned char *fw_bin, int nsize)
{
	INNO_RET ret;
	inno_chip_reset();

	////////// spi test, check spi communication ////////////
#if 0                                        //this test need time too long	
	ret =SPI_inno_Test();
	if(ret != INNO_NO_ERROR){
		inno_err("SPI_inno_Test fail ret =%d",ret);
		return -1;
	}
#else
	flag_spi_ok =1;                                                    // avoid influence of last spi fail, because fail cause flag =0
	ret =SPI_Test();
	if(ret != INNO_NO_ERROR){
		inno_err("SPI_Test fail ret =%d",ret);
		return -1;
	}
#endif
	/////////// download fw file ////////////////////////////
	{
		unsigned long clk_ctr = 0x80000001, cpu_ctr = 0x00200000, value = 0x00000000;

		mutex_lock(&inno_spi_mutex);
		ret =INNO_SPI_Write_WORD_Type2(M0_REG_CPU_CTR, cpu_ctr);
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Write_WORD_Type2 M0_REG_CPU_CTR fail");
			mutex_unlock(&inno_spi_mutex);
			return -1;
		}
		inno_msg("download fw nsize=%d",nsize);
		ret =INNO_SPI_Write_Byte_Type2(FW_BASE_ADDR, fw_bin, nsize);	 
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Write_Byte_Type2 FW_BASE_ADDR fail");
			mutex_unlock(&inno_spi_mutex);
			return -1;
		}
		inno_msg("writer clk_ctr");	 
		ret =INNO_SPI_Write_WORD_Type2(M0_REG_CLK_CTR, clk_ctr);
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Write_WORD_Type2 M0_REG_CLK_CTR fail");
			mutex_unlock(&inno_spi_mutex);
			return -1;
		}
		inno_msg("write value");
		ret =INNO_SPI_Write_WORD_Type2(M0_REG_CPU_CTR, value);	 
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Write_WORD_Type2 M0_REG_CPU_CTR fail");
			mutex_unlock(&inno_spi_mutex);
			return -1;
		}
		mutex_unlock(&inno_spi_mutex);
	}
	///////// check fw download success or not ///////////////a
	{
		unsigned char fw_ver = 0;
		//unsigned char cur_freq = 0;
#if 1
		inno_delay_ms(100);                                 // refrence to innofidei code
#if 0		
		ret =SPI_inno_Test();
		if(ret != INNO_NO_ERROR){
			inno_err("SPI_inno_Test fail ret =%d",ret);
			return -1;
		}
#endif
		//debug get firmware the first 8 bytes,check download fw bytes
#if 1		
		inno_msg("1,check INNO_GetFirmwareBytes for get the first 8 bytes");
		ret =INNO_GetFirmwareBytes();
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_GetFirmwareBytes fail");
			return -1;
		}	
#endif
		inno_msg("2,check INNO_GetFirmwareVersion ");
		ret=INNO_GetFirmwareVersion(&fw_ver);
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_GetFirmwareVersion fail");
			return -1;
		}	
		else
			inno_msg("fw version = 0x%x",fw_ver);
#if 0
		inno_msg( "3,check The current frequency dot is %x",cur_freq);
		ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM17, &cur_freq, 1);	
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Read_Byte_Type2 fail");
			return -1;
		}
		if(cur_freq == 0x14){
			inno_msg("Download fw success,then open Eint for get mfs");
			return 0; 
		}
		else{
			inno_err("cur_freq != 0x14");
			return -1; 	 
		}     
#endif	
		//here open flag_close, avoid workqueue run earlier than buffer alloct
		flag_close = 0;
		ret = inno_irq_setup(inno_intr_handler);    //after download fw,then open irq,avoid of other irq
	}
	return 0;
#endif	
}

INNO_RET INNO_GetFirmwareVersion(unsigned char *version) 
{
	INNO_RET ret = INNO_GENERAL_ERROR;
	unsigned char cmd[8] = {0};
	unsigned char rsp[8] = {0};
	int retry = 0;

	for(retry = 0; retry < 5; retry ++){
		// Send CMD
		cmd[0] = CMD_GET_FW_VER2;

		mutex_lock(&inno_spi_mutex);
		ret = INNO_Send_Cmd(cmd);
		if(ret != INNO_NO_ERROR)
		{
			mutex_unlock(&inno_spi_mutex);
			if(ret == INNO_TIMEOUT_ERROR)
				continue;
			else{
				inno_err("INNO_Send_Cmd fail");
				return INNO_GENERAL_ERROR;
			}

		}
		// Get RSP
		ret = INNO_Read_Rsp(rsp);
		mutex_unlock(&inno_spi_mutex);

		if(ret != INNO_NO_ERROR){
			if(ret == INNO_TIMEOUT_ERROR)
				continue;
			else{
				inno_err("INNO_Read_Rsp fail");
				return INNO_GENERAL_ERROR;
			}
		}
		else
			break;
	}
	if(retry >=5){
		inno_err("INNO_Send_Cmd timeout");
		return INNO_GENERAL_ERROR;
	}
	*version= rsp[1];
	inno_dbg("version = %d", *version);

	return ret;

}


INNO_RET INNO_CheckFWDownload(unsigned char *buffer, int len, int *pass)
{
	INNO_RET ret = INNO_GENERAL_ERROR;
	unsigned char cmd[8] = {0};
	unsigned char rsp[8] = {0};
	unsigned char check_high = 0;
	unsigned char check_low = 0;
	unsigned short file_checksum = 0;
	unsigned short read_checksum = 0;
	int i = 0;
	int retry = 0;

	if(sizeof(unsigned short) != 2){		// make sure "unsigned short" is 16bit 
		inno_err("sizeof(unsigned short) != 2");
		return INNO_GENERAL_ERROR;
	}

	/************1. calculate check sum of .bin ****************/
	if(len % 2 == 0){
		for(i = 0; i< len / 2; i++)		
			file_checksum ^= *((unsigned short *)buffer + i);		// little endian

	}
	else{
		for(i = 0; i< (len - 1) / 2; i++)
			file_checksum ^= *((unsigned short *)buffer + i);		// little endian
		file_checksum ^= *(buffer + len - 1) | (0xFF << 8);

	}
	inno_msg("fw_checksum : %x", file_checksum);

	/************2. Get check sum from IF101 *******************/

	/*    CHIP_V5 CMD Description:
	      Check firmware sum Command 

	      CMD[0] = CMD_GET_FW_CHECK_SUM
	      CMD[1] = Start_address_Low
	      CMD[2] = Start_address_High
	      CMD[3] = END_address_Low
	      CMD[4] = END_address_High
	      CMD[5] = 0
	      CMD[6] = 0
	      CMD[7] = 0

	      RSP[0] = CMD_GET_FW_CHECK_SUM
	      RSP[1] = CheckSum Low
	      RSP[2] = CheckSum High
	      RSP[3] = 0
	      RSP[4] = 0
	      RSP[5] = 0
	      RSP[6] = 0
	      RSP[7] = 0
	 */	
	cmd[0] = CMD_GET_FW_CHECK_SUM2;		       
	cmd[1] = 0;
	cmd[2] = 0;
	cmd[3] = len&0xff;
	cmd[4] = (len>>8)&0xff;

	for(retry = 0; retry < 20; retry ++){

		mutex_lock(&inno_spi_mutex);
		ret = INNO_Send_Cmd(cmd);
		if(ret != INNO_NO_ERROR)
		{
			mutex_unlock(&inno_spi_mutex);
			if(ret == INNO_TIMEOUT_ERROR)
				continue;
			else{
				inno_err("INNO_Send_Cmd fail");
				return INNO_GENERAL_ERROR;
			}

		}

		ret = INNO_Read_Rsp(rsp);
		mutex_unlock(&inno_spi_mutex);

		if(ret != INNO_NO_ERROR){
			if(ret == INNO_TIMEOUT_ERROR)
				continue;
			else{
				inno_err("INNO_Read_Rsp fail");
				return INNO_GENERAL_ERROR;
			}
		}
		else
			break;
	}
	if(retry==20){
		inno_err("INNO_Send_Cmd timeout");
		return INNO_TIMEOUT_ERROR;
	}

	check_low= rsp[1];						
	check_high = rsp[2];							
	read_checksum = (check_high << 8) | check_low;

	inno_msg("file_checksum =%x, read_checksum =%x", file_checksum,read_checksum);

	/*************** 3. Compare check sum *****************/
	if(file_checksum == read_checksum)
		*pass = 1;
	else 
		*pass = 0;
	return ret;
}


int  INNO_GetChipID(unsigned int *chipID)  
{
	unsigned char data = 0;
	INNO_RET ret = INNO_NO_ERROR;

	mutex_lock(&inno_spi_mutex);
	ret =INNO_SPI_Read_Byte_Type2(M0_REG_PLL_STATUS, &data, 1); 
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Read_Byte_Type2 fail");
		mutex_unlock(&inno_spi_mutex);
		return -1;
	}
	mutex_unlock(&inno_spi_mutex);

	*chipID = (unsigned int)(data&0xF0)>>4;
	inno_msg("CHIPID is %x",*chipID);

	if(IF228_CHIPID == *chipID)	
	{
		return INNO_NO_ERROR;
	}
	else if(IF238_CHIPID == *chipID)	
	{
		return INNO_NO_ERROR;
	}
	else if(IF258_CHIPID == *chipID)	
	{
		return INNO_NO_ERROR;
	}
	else
	{
		inno_err("read CHIPID err!!");
		return INNO_GENERAL_ERROR;
	}
}

//debug  xingyu  when block,check chip is reset or not
int INNO_GetFirmwareBytes(void)
{
	int i;
	int ret=0;
	unsigned char buffer[8]={0};
	mutex_lock(&inno_spi_mutex);
	ret =INNO_SPI_Read_Byte_Type2(0,buffer,8);
	for(i=0;i<8;i++)
		inno_msg("buffer[%d]=%x",i,buffer[i]);
	//?//xingyu check fw reset?
	mutex_unlock(&inno_spi_mutex);
	return ret;
}

// debug xingyu handle chip exception error,just like chip reset result in sendcmd timeout
int INNO_Chip_errHandle(void)
{
	int ret;
	struct inno_sys_state*	sys_state;
	inno_msg("INNO_GetFirmwareBytes and signal quality");
	ret =INNO_GetFirmwareBytes();
	if(ret != INNO_NO_ERROR)       {
		inno_err("INNO_GetFirmwareBytes fail");
		return -1;
	}	

	sys_state = kmalloc(sizeof(struct inno_sys_state), GFP_KERNEL);
	if(sys_state ==NULL){
		inno_err("kmalloc sys_state fail");
		return -1;
	}
	sys_state->stattype = STATTYPE_SIGNAL_QUALITY;
	ret = inno_get_system_state(sys_state);
	inno_msg("signal quality =%d",sys_state->statdata.signal_quality);
	kfree(sys_state);
	return ret;
}
/////////////////////////// test //////////////////
static INNO_RET SPI_Test(void)
{
	unsigned char data = 0;
	unsigned int chipID = 0;
	INNO_RET ret = INNO_NO_ERROR;

	mutex_lock(&inno_spi_mutex);
	ret = INNO_SPI_Write_cmd_rsp(0x1,&data);	
	mutex_unlock(&inno_spi_mutex);

	if((data != 0x2)||(ret!=INNO_NO_ERROR)){
		inno_err("INNO_SPI_Write_cmd_rsp fail,ret=%d,data=%d",ret,data);
		return INNO_GENERAL_ERROR;
	}
	else
		inno_msg("spi test ok");

	ret = INNO_GetChipID(&chipID);
	if(ret!=INNO_NO_ERROR){
		inno_err("INNO_GetChipID fail");
		return INNO_GENERAL_ERROR;	
	}
	return ret;	
}
#if 0
INNO_RET INNO_InterfaceTest(void)
{
	INNO_RET ret = INNO_NO_ERROR;

	inno_chip_reset();

	ret = SPI_Test();
	if(ret != INNO_NO_ERROR)
		return INNO_TIMEOUT_ERROR;

	return ret;
}
#endif

#if 0
int SPITest(void)
{
	unsigned char data = 0;
	INNO_RET ret = INNO_NO_ERROR;

	mutex_lock(&inno_spi_mutex);
	ret=INNO_SPI_Write_One_Byte(0x1);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Write_One_Byte fail");
		mutex_unlock(&inno_spi_mutex);
		return -1;
	}
	ret=INNO_SPI_Read_One_Byte(&data);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Read_One_Byte fail");
		mutex_unlock(&inno_spi_mutex);
		return -1;
	}

	ret=INNO_SPI_Write_One_Byte(0x02);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Write_One_Byte fail");
		mutex_unlock(&inno_spi_mutex);
		return -1;
	}
	ret=INNO_SPI_Read_One_Byte(&data);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Read_One_Byte fail");
		mutex_unlock(&inno_spi_mutex);
		return -1;
	}
	mutex_unlock(&inno_spi_mutex);
	return 0;
}
#endif
/*
   static INNO_RET SPI_inno_Test(void)
   {
   unsigned char data;
   unsigned long temp = 0;

   INNO_SPI_Read_WORD_Type2(0x4000402c,&temp);
   inno_msg("SPI_Test SPI_Read 0x4000402c------0x%x  ",(unsigned int)temp);
   msleep(200);

   INNO_SPI_Write_One_Byte(0x1);
   msleep(200);
   INNO_SPI_Read_One_Byte(&data);
   inno_msg("SPI_Test read data is 0x%x  ",data);
   msleep(200);

   if(data == 0x2){   
   inno_msg("SPI_inno_Test ok");
   return INNO_NO_ERROR;
   }
   else{
   inno_err("SPI_inno_Test fail");
   return INNO_GENERAL_ERROR;
   }
   }
 */

/////////////////////// spi transfer
INNO_RET INNO_Send_Cmd(unsigned char *cmd)           //xingyu//?
{
	unsigned char cmd_status = 0;
	int j = 0;
	INNO_RET ret = INNO_NO_ERROR;

	for(j = 0; j < 20; j++)                 //xingyu judge chip busy or not?    //xingyu//?
	{
		ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM31, &cmd_status, 1);
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Read_Byte_Type2 fail");
			return INNO_GENERAL_ERROR;
		}
		if((cmd_status & CMD_BUSY) == 0)   //bit7:1 cmd busy; 0 cmd over
		{
			ret=INNO_SPI_Write_Byte_Type2(FETCH_PER_COMM1, (cmd+1), 7); 
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Write_Byte_Type2 (1)fail");
				return INNO_GENERAL_ERROR;
			}
			cmd_status =CMD_BUSY;
			ret=INNO_SPI_Write_Byte_Type2(FETCH_PER_COMM31, &cmd_status, 1);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Write_Byte_Type2 (2)fail");
				return INNO_GENERAL_ERROR;
			}

			ret =INNO_SPI_Write_Byte_Type2(FETCH_PER_COMM0, &cmd[0], 1);	
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Write_Byte_Type2 (3)fail");
				return INNO_GENERAL_ERROR;
			}
			return INNO_NO_ERROR;
		}
		else{
			inno_msg("Warning:cmd_status = 0x%x", cmd_status);
		}
	}

	inno_err("error INNO_Send_Cmd timeout");
	return INNO_TIMEOUT_ERROR;	

}
INNO_RET INNO_Read_Rsp(unsigned char *rsp)                                   //xingyu ack
{
	unsigned char cmd_status = 0;
	int j = 0;
	INNO_RET  ret = INNO_NO_ERROR;

	for(j = 0; j < 20; j++)
	{
		ret =INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM31, &cmd_status, 1);
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Read_Byte_Type2 FETCH_PER_COMM31 fail");
			return INNO_GENERAL_ERROR;
		}

		inno_dbg("cmd_status = 0x%x", cmd_status);
		if((cmd_status & RSP_DATA_VALID) == 0)  //bit6-0:0 success 1 error
		{
			if((cmd_status & 0x7F) == 0)    //RSP_DATA_OK: 0X00 RSP_DATA_ERR:0X01
			{
				ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM8, rsp, 8);				
				if(ret != INNO_NO_ERROR){
					inno_err("INNO_SPI_Read_Byte_Type2  FETCH_PER_COMM8 fail");
					return INNO_GENERAL_ERROR;
				}
				return INNO_NO_ERROR;
			}
			else
			{
				inno_err("rsp error = 0x%x", cmd_status&0x7f);
				return INNO_FW_OPERATION_ERROR;
			}
		}
	}
	inno_msg("Warning:INNO_Read_Rsp timeout");
	return INNO_TIMEOUT_ERROR;
}

/******************************************************************************
 *			V5 SPI Timing Type
 *******************************************************************************/
INNO_RET INNO_SPI_Write_One_Byte(unsigned char data)
{
	INNO_RET  ret =INNO_SPI_Write_One_Byte_NoCS(data);
	return ret;
}

/*
static INNO_RET INNO_SPI_Read_One_Byte(unsigned char *data)
{
	INNO_RET  ret =INNO_SPI_Read_One_Byte_NoCS(data);
	return ret;
}*/

INNO_RET INNO_SPI_Read_Bytes(unsigned char *buffer, int len)
{
	INNO_RET ret= INNO_GENERAL_ERROR;
	SPI_MODE_T mode_parameter;
	unsigned char* tx_buf=NULL,*rx_buf=NULL;
		if(len <=0){
			inno_err("len <=0");
			return INNO_GENERAL_ERROR;
		}
		
#ifdef MT6589_CMMB_SPI_CONFIG
	INNO_SPI_Mode(1);
#endif
	
	if(len>CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){	
		if(len%CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES != 0){          // need transfer twice using spi, so using pause mode
			mode_parameter.mode=SPI_MODE_PAUSE;
			mode_parameter.bEnable=true;
			spi_mode_setting(mode_parameter);
		}
               tx_buf = NULL;
               rx_buf = buffer;
               ret =INNODev_xfer(tx_buf,rx_buf,len);
	}
	else{
		if(cmmb_spi_src_buffer_all==NULL)
			return INNO_GENERAL_ERROR;
               tx_buf = NULL;
               rx_buf = cmmb_spi_src_buffer_all;
               ret =INNODev_xfer(tx_buf,rx_buf,len);

		 if(ret == INNO_NO_ERROR)
          		memcpy(buffer,cmmb_spi_src_buffer_all,len);
	}	
	return ret;
}

INNO_RET INNO_SPI_Exec_Cmd(int cmd, unsigned char *rsp)   
{
	INNO_RET ret = INNO_NO_ERROR;
	ret=INNO_SPI_Write_cmd_rsp(cmd,rsp);
	return ret;
}

/*
   CHIP_V5 SPI Timing Type2

   READ_AHBM2	0x71    Read AHB space method2	Type 2
   WRITE_AHBM2	0x73	    Write AHB space method2	Type 2

   WR_CODE_2 + ADDR (A) + WR_D (0) +  бн + WR_D(N); 
   RD_CODE_2 + ADDR (A) + DUMMY + RD_D (0) + бн + RD_D(N);
 */
INNO_RET INNO_SPI_Write_Byte_Type2(unsigned long addr, unsigned char *data, int len)  //cmd = 0x73
{
	INNO_RET ret = INNO_GENERAL_ERROR;
	SPI_MODE_T mode_parameter;
	struct spi_message msg;
	struct spi_transfer transfer[3], *ptransfer;
	unsigned char *tx_buf_tmp;
	
#ifdef MT6589_CMMB_SPI_CONFIG
	INNO_SPI_Mode(0);
#endif

	mode_parameter.mode=SPI_MODE_PAUSE;                 //xingyu spi pause mode 
	mode_parameter.bEnable=true;
	//when there transfer size over  one pkt len must use two transfer but CS keep low 
	spi_mode_setting(mode_parameter);
	if(cmmb_spi_src_buffer_all==NULL)
		return INNO_GENERAL_ERROR;
	cmmb_spi_src_buffer_all[0]=WRITE_AHBM2;
	cmmb_spi_src_buffer_all[1]=(addr>>24)&0xff;
	cmmb_spi_src_buffer_all[2]=(addr>>16)&0xff;
	cmmb_spi_src_buffer_all[3]=(addr>>8)&0xff;
	cmmb_spi_src_buffer_all[4]=addr&0xff;

	spi_message_init(&msg);
	transfer[0].tx_buf = cmmb_spi_src_buffer_all;
	transfer[0].rx_buf =NULL;
	transfer[0].len = 5;
	spi_message_add_tail(&transfer[0], &msg);
#ifdef MT6589_CMMB_SPI_CONFIG
	tx_buf_tmp = kmalloc(MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES,GFP_KERNEL);
#else
	tx_buf_tmp = kmalloc(CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES,GFP_KERNEL);
#endif

	if(tx_buf_tmp ==NULL){
		inno_err("kmalloc fail");
		return INNO_GENERAL_ERROR;
	}
		
#ifdef MT6589_CMMB_SPI_CONFIG
	if( len > MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES )
	   	{
	   		int i;
			int pkt_cnt = len / MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES;
			int remainder = len % MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES;
			ptransfer = kmalloc((pkt_cnt+1)*sizeof(struct spi_transfer), GFP_KERNEL);
			for(i=0; i < pkt_cnt; i++)
			{
				ptransfer[i].tx_buf = data + i*MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES;
				ptransfer[i].rx_buf = NULL;
				ptransfer[i].len = MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES;
				spi_message_add_tail(&ptransfer[i], &msg);
			}
			if(remainder)
			{
				ptransfer[i].tx_buf = data + i*MT6589_CMMB_SPI_TX_MAX_PKT_LENGTH_PER_TIMES;
				ptransfer[i].rx_buf = NULL;
				ptransfer[i].len = remainder;
				spi_message_add_tail(&ptransfer[i], &msg);
			}
	   }
#else
	int const pkt_count = len / CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	int const remainder = len % CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	if(len>CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){	
		transfer_1[0].tx_buf =data;
		transfer_1[0].rx_buf =NULL;
		transfer_1[0].len = CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES*pkt_count;
		spi_message_add_tail(&transfer_1[0], &msg);

		if(0 != remainder)	 { 
			transfer_1[1].tx_buf =data+ (CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES * pkt_count);
			transfer_1[1].rx_buf =NULL;
			transfer_1[1].len = remainder;
			spi_message_add_tail(&transfer_1[1], &msg);
		}
	}
#endif
	else
	{
		memcpy(tx_buf_tmp,data,len);
		transfer[1].tx_buf = tx_buf_tmp ;
		transfer[1].rx_buf = NULL;
		transfer[1].len = len;
		spi_message_add_tail(&transfer[1], &msg);
	}
	
	if(INNODev_sync(INNODev,&msg))
		ret =INNO_GENERAL_ERROR;	
	else
		ret =INNO_NO_ERROR;
	
	kfree(tx_buf_tmp);
	tx_buf_tmp =NULL;
	return ret;
}

INNO_RET INNO_SPI_Read_Byte_Type2(unsigned long addr, unsigned char *buffer, int len)  
{
	INNO_RET ret = INNO_NO_ERROR;
	SPI_MODE_T mode_parameter;
	struct spi_message msg;
	struct spi_transfer transfer[3];
	unsigned char *rx_buf_tmp;
	
	int const pkt_count = len / CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	int const remainder = len % CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES;
	
#ifdef MT6589_CMMB_SPI_CONFIG
	INNO_SPI_Mode(1);
#endif
	
	// set spi pause mode,when there transfer size over  one pkt len must use two transfer but CS keep low 
	mode_parameter.mode=SPI_MODE_PAUSE;
	mode_parameter.bEnable=true;
	spi_mode_setting(mode_parameter);               
	if(cmmb_spi_src_buffer_all==NULL)
		return INNO_GENERAL_ERROR;
	cmmb_spi_src_buffer_all[0]=READ_AHBM2;
	cmmb_spi_src_buffer_all[1]=(addr>>24)&0xff;
	cmmb_spi_src_buffer_all[2]=(addr>>16)&0xff;
	cmmb_spi_src_buffer_all[3]=(addr>>8)&0xff;
	cmmb_spi_src_buffer_all[4]=addr&0xff;

	spi_message_init(&msg);
	transfer[0].tx_buf = cmmb_spi_src_buffer_all;
	transfer[0].rx_buf =NULL;
	//INNO_SPI_Write_Bytes_NoCS(cmmb_spi_src_buffer_all,5); 
	//to cover IF228 read bug there will be a dummy data need read
	//we add 1 read length ,so that SPI output SCLK IF228 out that the dumy data
	//INNO_SPI_Write_Bytes_NoCS(cmmb_spi_src_buffer_all,6);               
	transfer[0].len = 6;
	spi_message_add_tail(&transfer[0], &msg);

	if(len>CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES){   // the buffer is allocated when device open,it is 4 align	
		transfer_1[0].tx_buf =NULL;
		transfer_1[0].rx_buf =buffer;
		transfer_1[0].len = CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES*pkt_count;
		spi_message_add_tail(&transfer_1[0], &msg);

		if(0 != remainder)	 { 
			transfer_1[1].tx_buf =NULL;
			transfer_1[1].rx_buf =buffer+ (CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES * pkt_count);
			transfer_1[1].len = remainder;
			spi_message_add_tail(&transfer_1[1], &msg);
		}
		
		if(INNODev_sync(INNODev,&msg))
			ret =INNO_GENERAL_ERROR;	
		else
			ret =INNO_NO_ERROR;
	}
	else{    // the buffer could not 4 align,then kmalloc 4 align buffer
		rx_buf_tmp = kmalloc(CMMB_SPI_INTERFACE_MAX_PKT_LENGTH_PER_TIMES,GFP_KERNEL);
		if(rx_buf_tmp ==NULL){
			inno_err("kmalloc fail");
			return INNO_GENERAL_ERROR;
		}		   
		transfer[1].tx_buf = NULL;
		transfer[1].rx_buf =rx_buf_tmp ;
		transfer[1].len = len;
		spi_message_add_tail(&transfer[1], &msg);
		if(INNODev_sync(INNODev,&msg))
			ret =INNO_GENERAL_ERROR;	
		else
			ret =INNO_NO_ERROR;
		memcpy(buffer,rx_buf_tmp ,len);                   

		kfree(rx_buf_tmp );
		rx_buf_tmp =NULL;		  
	}	  	
	return ret;
}

INNO_RET INNO_SPI_Write_WORD_Type2(unsigned long addr, unsigned long data)
{
	INNO_RET ret = INNO_NO_ERROR;
	if(cmmb_spi_src_buffer_all==NULL)
		return INNO_GENERAL_ERROR;
	cmmb_spi_src_buffer_all[0]=WRITE_AHBM2;
	cmmb_spi_src_buffer_all[1]=(addr>>24)&0xff;
	cmmb_spi_src_buffer_all[2]=(addr>>16)&0xff;
	cmmb_spi_src_buffer_all[3]=(addr>>8)&0xff;
	cmmb_spi_src_buffer_all[4]=addr&0xff;

	cmmb_spi_src_buffer_all[5]=data&0xff;
	cmmb_spi_src_buffer_all[6]=(data>>8)&0xff;
	cmmb_spi_src_buffer_all[7]=(data>>16)&0xff;
	cmmb_spi_src_buffer_all[8]=(data>>24)&0xff;

	ret =INNO_SPI_Write_Bytes_NoCS(cmmb_spi_src_buffer_all,9);
	return ret;
}

INNO_RET INNO_SPI_Read_WORD_Type2(unsigned long addr, unsigned long *data)
{
	INNO_RET ret = INNO_NO_ERROR;
	struct spi_message msg;
	struct spi_transfer transfer[2];
	SPI_MODE_T mode_parameter;
	unsigned char *buffer = kmalloc(4,GFP_KERNEL);
	if(!buffer){
		inno_err("kmalloc fail buffer==NULL");
		return INNO_GENERAL_ERROR;
	}
			
#ifdef MT6589_CMMB_SPI_CONFIG
	INNO_SPI_Mode(0);
#endif
	
	memset(buffer,0,4);
	mode_parameter.mode=SPI_MODE_PAUSE;
	mode_parameter.bEnable=true;
	spi_mode_setting(mode_parameter);           //enable deassert mode

	if(cmmb_spi_src_buffer_all==NULL)
		return INNO_GENERAL_ERROR;
	memset(cmmb_spi_src_buffer_all,0,6);
	cmmb_spi_src_buffer_all[0]=READ_AHBM2;
	cmmb_spi_src_buffer_all[1]=(addr>>24)&0xff;
	cmmb_spi_src_buffer_all[2]=(addr>>16)&0xff;
	cmmb_spi_src_buffer_all[3]=(addr>>8)&0xff;
	cmmb_spi_src_buffer_all[4]=addr&0xff;

	spi_message_init(&msg);
	transfer[0].tx_buf = cmmb_spi_src_buffer_all;
	transfer[0].rx_buf =NULL;
	//INNO_SPI_Write_Bytes_NoCS(cmmb_spi_src_buffer_all,5);
	//to cover IF228 read bug there will be a dummy data need read
	//we add 1 read length ,so that SPI output SCLK IF228 out that the dumy data
	transfer[0].len = 6;
	spi_message_add_tail(&transfer[0], &msg);
	transfer[1].tx_buf = NULL;
	transfer[1].rx_buf =buffer;
	transfer[1].len = 4;
	spi_message_add_tail(&transfer[1], &msg);

	if(INNODev_sync(INNODev,&msg))
		ret =INNO_GENERAL_ERROR;	
	else
		ret =INNO_NO_ERROR;

	*data = (unsigned long)buffer[0] + ((unsigned long)(buffer[1]<<8)) + ((unsigned long)(buffer[2]<<16)) + ((unsigned long)(buffer[3]<<24));
	//relseas buffer
	kfree(buffer);
	buffer =NULL;
	return ret;
}

INNO_RET INNO_SPI_Read_Bytes_Type4(int cmd, unsigned char *buffer, int len)
{
	INNO_RET ret = INNO_NO_ERROR;
	ret=INNO_SPI_Write_One_Byte(cmd);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Write_One_Byte fail");
		return INNO_GENERAL_ERROR;
	}
	ret=INNO_SPI_Read_Bytes_NoCS(buffer, len);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Read_Bytes_NoCS fail");
		return INNO_GENERAL_ERROR;
	}
	return ret;
}
//////////////////////

//////////// *************************************************************************************************///////////
/**********************************************************
  Reset CMD FORMAT:
  UAM_RESET_CMD | 0x00 | 0x00 | crc
 ***********************************************************/
int INNO_UAM_Reset(unsigned char *pATRValue, unsigned int *pATRLen)
{
	unsigned char cmd[9] = {0};
	INNO_RET ret = INNO_GENERAL_ERROR;
	unsigned int retry = 0;
	int i=0;                                 //xingyu debug print ATR for key erase
	inno_dbg("enter");
	cmd[0] = CMD_UAM_RESET;
	cmd[1] = 0xAA;
	cmd[2] = 0x55;
	cmd[3] = 0x00;
	cmd[4] = 0x00;	

	for(retry = 0; retry < 3; retry ++){
		// Send CMD
		ret = INNO_Send_UAM_Cmd(cmd, 9);
		if(ret != INNO_NO_ERROR)
		{
			if(ret == INNO_TIMEOUT_ERROR)
				continue;
			else{
				inno_err("INNO_Send_UAM_Cmd fail");
				return INNO_GENERAL_ERROR;
			}

		}
		// Get RSP		
		ret = INNO_Read_UAM_Rsp(pATRValue, pATRLen);
		if(ret != INNO_NO_ERROR){
			if(ret == INNO_TIMEOUT_ERROR)
				continue;
			else{
				inno_err("INNO_Read_UAM_Rsp fail");
				return INNO_GENERAL_ERROR;
			}
		}
		else 
			break;
	}
	if(retry >=3){
		inno_err("INNO_Send_UAM_Cmd tiemout");
		return -1;
	}
	inno_msg("print ATR valuek,pATRLen:%d",*pATRLen);
	for(i=0;i<*pATRLen;i++)
		printk(KERN_ERR " %d",pATRValue[i]);
	printk(KERN_ERR "end");
	inno_dbg("leave");
	return ret;
}

static int UpdateCardCapabilities(void)
{
	unsigned char *atrString = cardCapabilities_UAM.ATR.Buffer;
	unsigned long atrLength = (unsigned long) cardCapabilities_UAM.ATR.Length;
	unsigned char	Tck, TA[4]={0}, TB[4]={0}, TC[4]={0}, TD[4]={0}, Y;
	unsigned char TA2Present = 0;
	unsigned long i, numProtocols = 0, protocolTypes = 0;
	int status =0;

	if(atrLength < 2)
	{				
		inno_err("ATR is too short (Min. length is 2");
		return 0;                        // 0:error
	}

	if(atrString[0] != 0x3b && atrString[0] != 0x3f && atrString[0] != 0x03) 
	{
		inno_err("Initial character %02xh of ATR is invalid", atrString[0]);
		return 0;                       // 0:error
	}

	// Test for invers convention
	if (*atrString == 0x03) 
	{
		cardCapabilities_UAM.InversConvention = 1;
		inno_err("Card uses Inverse Convention");
		return 0;                       // 0:error
		//SET_REG(SIM_CONFIG,GET_REG(SIM_CONFIG)|SIM_DATA_INVERTER);
	}

	atrString += 1;
	atrLength -= 1;

	// Calculate check char, but do not test now since if only T=0	is present the ATR doesn't contain a check char
	for(i = 0, Tck = 0; i < atrLength; i++) 
	{
		Tck ^= atrString[i];
	}

	// Set default values as described in ISO 7816-3
	TA[0] = 0x11;		// TA1 codes FI in high-byte and Dl in low-byte;
	TB[0] = 0x25;		// TB1 codes II in bits b7/b6 and PI1 in b5-b1. b8 has to be 0
	//TC[0] = 0x0;		// TC1 codes N in bits b8-b1
	TC[1] = 10; 		// TC2 codes T=0 WI

	// Translate ATR string to TA to TD values (See ISO)
	cardCapabilities_UAM.HistoricalChars.Length = *atrString & 0x0f;
	Y = *atrString & 0xf0;

	atrString += 1;
	atrLength -= 1;

	for(i=0; i<4; i++)
	{
		if(Y & 0x10)
		{
			if(i==1)
			{
				TA2Present = 1; 
			}

			TA[i] = *atrString++;
			atrLength--;
		}

		if(Y & 0x20)
		{
			TB[i] = *atrString++;
			atrLength--;
		}

		if(Y & 0x40)
		{
			TC[i] = *atrString++;
			atrLength--;
		}

		if(Y & 0x80)
		{
			Y = *atrString & 0xf0;
			TD[i] = *atrString++ & 0x0f;
			atrLength--;

			// Check if the next parameters are for a new protocol.
			if (((1 << TD[i]) & protocolTypes) == 0) 
			{
				// Count the number of protocols that the card supports
				numProtocols++;
			}
			protocolTypes |= 1 << TD[i];
		}else
		{
			break;
		}
	}

	// Check if the card supports a protocol other than T=0
	if (protocolTypes & ~1) 
	{
		// The atr contains a checksum byte. Exclude that from the historical byte length check
		atrLength -=1;		

		// This card supports more than one protocol or a protocol other than T=0, so test if the checksum is correct
		if (Tck != 0) 
		{

			inno_err("ATR Checksum is invalid");
			status = 0;
			goto _leave_lab;
		}
	}

	if (atrLength < 0 || atrLength != cardCapabilities_UAM.HistoricalChars.Length) 
	{

		inno_err("ATR length is inconsistent");
		status = 0;
		goto _leave_lab;
	}

_leave_lab:
	if(status != 0)
	{
		return status;
	}

	// store historical characters
	memcpy(cardCapabilities_UAM.HistoricalChars.Buffer, atrString, cardCapabilities_UAM.HistoricalChars.Length);

	// Now convert TA - TD values to global interface bytes
	cardCapabilities_UAM.FI = (TA[0] & 0xf0) >> 4;// Clock rate conversion
	cardCapabilities_UAM.DI = (TA[0] & 0x0f);	// bit rate adjustment
	cardCapabilities_UAM.II = (TB[0] & 0xc0) >> 6;	// Maximum programming current factor
	cardCapabilities_UAM.P = (TB[1] ? TB[1] : (TB[0] & 0x1f) * 10); // Programming voltage in 0.1 Volts
	cardCapabilities_UAM.N = TC[0]; // Extra guard time

	if ((TA2Present || (numProtocols <= 1)) && (cardCapabilities_UAM.FI == 1) && (cardCapabilities_UAM.DI == 1)) 
	{
		// If the card supports only one protocol (or T=0 as default)
		// and only standard paramters then PTS selection is not available
		OperationMode_UAM = OP_SPECIFIC;
	} 
	else 
	{
		OperationMode_UAM = OP_NEGOTIABLE;
	}

	// Now find protocol specific data
	if (TD[0] == 0) 
	{		
		cardCapabilities_UAM.Protocol.Supported |= SCARD_PROTOCOL_T0;
		cardCapabilities_UAM.T0.WI = TC[1];
		inno_msg("T=0 Values from ATR:  WI = %x",cardCapabilities_UAM.T0.WI);
	}

	if (protocolTypes & SCARD_PROTOCOL_T1) 
	{
		for (i = 0; TD[i] != 1 && i < 4; i++)
			;

		for (; TD[i] == 1 && i < 4; i++) 
			;

		if (i == 4) 
		{
			return 0;			
		}

		cardCapabilities_UAM.Protocol.Supported |= SCARD_PROTOCOL_T1;

		cardCapabilities_UAM.T1.IFSC = (TA[i] ? TA[i] : 32);
		cardCapabilities_UAM.T1.CWI = ((TB[i] & 0x0f) ? (TB[i] & 0x0f) : T1_CWI_DEFAULT);
		cardCapabilities_UAM.T1.BWI = ((TB[i] & 0xf0) >> 4 ? (TB[i] & 0xf0) >> 4 : T1_BWI_DEFAULT);
		cardCapabilities_UAM.T1.EDC = (TC[i] & 0x01);

	}

	if(OperationMode_UAM == OP_SPECIFIC)
	{
		if(TA2Present)
		{
			// TA2 is present in the ATR, so use the protocol indicated in the ATR
			cardCapabilities_UAM.Protocol.Selected = 1 << TA[1];
		}
		else
		{
			// The card only supports one protocol So make that one protocol the current one to use
			cardCapabilities_UAM.Protocol.Selected = cardCapabilities_UAM.Protocol.Supported;
		}
	}else
	{
		//diag_printf("Mode: Negotiable\n\n");
	}

	return 1;	
}


/**********************************************************
  PPS CMD FORMAT:
  UAM_PPS_CMD | 0x04 | 0x00 | pps[0] | pps[1] | pps[2] | pps[3] | crc
 ***********************************************************/
INNO_RET INNO_UAM_PPS(unsigned char *ppsReqValue)
{
	unsigned char cmd[15] = {0};
	unsigned char rsp[33] = {0};
	unsigned int len = 33;
	INNO_RET ret = INNO_NO_ERROR;
	unsigned int retry = 0;

	cmd[0] = CMD_UAM_PPS;
	cmd[1] = 0xAA;
	cmd[2] = 0x55;
	cmd[3] = 0x00;
	cmd[4] = 0x04;

	memcpy(cmd+9, ppsReqValue, 4);

	for(retry = 0; retry < 3; retry ++){
		ret = INNO_Send_UAM_Cmd(cmd, 13);
		if(ret != INNO_NO_ERROR)
		{
			continue;
		}

		ret = INNO_Read_UAM_Rsp(rsp, &len);
		if(ret != INNO_NO_ERROR)
			continue;
		else
			break;
	}
	if(retry >=3){
		inno_err("INNO_Send_UAM_Cmd timeout");
		return INNO_GENERAL_ERROR;
	}

	if(memcmp(rsp, ppsReqValue, 4) == 0)
		return INNO_NO_ERROR;
	else
		return INNO_FW_OPERATION_ERROR;
}

int inno_init_uam(void)
{
	unsigned char ATRBuf[40] ={0};	
	unsigned char pps_request[] ={0xFF, 0x10, 0, 0};
	//SCARD_CARD_CAPABILITIES  cardCapabilities_UAM;
	unsigned int atrLen = 40;
	int ret=0;

	ret = INNO_UAM_Reset(ATRBuf, &atrLen);
	if(ret != INNO_NO_ERROR)
	{
		inno_err( "[INNO_UAM_Init] - Reset fail");
		return INNO_GENERAL_ERROR;
	}
	else
	{
		if(atrLen <= 0)
		{
			inno_err("[INNO_UAM_Init] - ATR Value Length error!!!");
			return INNO_PARAMETER_ERROR;
		}

		memcpy(cardCapabilities_UAM.ATR.Buffer,ATRBuf,atrLen);		 
		cardCapabilities_UAM.ATR.Length = atrLen;

		ret = UpdateCardCapabilities();
		if(ret)
		{
			inno_dbg("[INNO_UAM_Init] - INNO_UAM_Request_PPS");

			if(cardCapabilities_UAM.FI == 0x1 && cardCapabilities_UAM.DI == 0x8)
			{
				pps_request[2]=0x18;
			}
			else
			{
				pps_request[2]=0x96;  //if208: 0x94; v5:0x96
			}

			pps_request[3] = pps_request[0] ^ pps_request[1] ^pps_request[2];

			if(INNO_UAM_PPS(pps_request) != INNO_NO_ERROR)
			{
				inno_err("UAM PPS Request failed");
				return INNO_GENERAL_ERROR;
			}

			inno_dbg("UAM PPS Request successfully");
		}
		else{
			inno_err("UpdateCardCapabilities fail");
			return INNO_GENERAL_ERROR;
		}
	}
	inno_msg("init success");
	return INNO_NO_ERROR;
}

int inno_uam_transfer(struct inno_uam_parameter *uam)
{
	inno_return_value_t	ret = INNO_SUCCESS;
	unsigned char 	cmd[270] = {0}, rsp[256] = {0};
	unsigned int	cmdLen = 0, rsplen = 0;
	//int i = 0;
	unsigned char *pBufIn;
	unsigned int bufInLen;
	unsigned char *pBufOut; 
	unsigned int pBufOutLen;
	unsigned short sw;
	int i;
	//int cmd_time = 0;

	pBufIn = uam->pBufIn;
	bufInLen = uam->bufInLen;
	pBufOut= uam->pBufOut; 
	pBufOutLen = uam->pBufOutLen;
	sw = uam->sw;

	inno_dbg("+++++ inno_uam_transfer +++++"); 

#ifdef CMMB_DEBUG
	inno_dbg("inno_uam_transfer, bufInLen=%d", bufInLen);
	inno_dbg("inno_uam_transfer, pBufIn: ");
	for(i = 0; i< bufInLen; i++){
		inno_dbg("0x%X,",pBufIn[i]);
	}
#endif

	if(bufInLen == 4)
	{
		inno_err("inno_uam_transfer, bufInLen = 4. return!");
		return INNO_ERROR;
	}
	else if(bufInLen == 5)
	{
		//inno_msg("inno_uam_transfer, bufInLen == 5");

		cmd[0] = CMD_UAM_GET_DATA;
		cmd[1] = 0xAA;
		cmd[2] = 0x55;    
		cmd[3] = 0x00;
		cmd[4] = 0x05;    
		memcpy(cmd+9, pBufIn, 5);

		cmdLen = 0x0E;  //bufInLen + 9

		ret = INNO_Send_UAM_Cmd(cmd, cmdLen);	
		if(ret != INNO_SUCCESS)  
		{
			inno_err("[T0Transfers (case 2)] - send cmd fail!!!");
			return INNO_ERROR;
		}

		ret = INNO_Read_UAM_Rsp(rsp, &rsplen);
		if(ret != INNO_SUCCESS)
		{
			inno_err("[T0Transfers (case 2)] - response fail!!!");
			return INNO_ERROR;      
		}

		if(rsplen < 3)
		{
			inno_err("[T0Transfers (case 2)] - Get SW1, SW2 error!!!");
			inno_err("inno_uam_transfer, bufInLen=%d", bufInLen);
			inno_err("inno_uam_transfer, pBufIn: ");
			for(i = 0; i< bufInLen; i++)
				inno_err("0x%X,",pBufIn[i]);
			inno_err("inno_uam_transfer,rspLen=%d", rsplen);
			inno_err("inno_uam_transfer rsp:");
			for(i = 0; i< rsplen; i++)
				inno_err("0x%X,", rsp[i]);
			return INNO_ERROR;    
		}

		//		sw = MAKEWORD(rsp[rsplen-1], rsp[rsplen-2]);
		sw = ((unsigned short)((unsigned char)(rsp[rsplen-1])) | (((unsigned short)(unsigned char)(rsp[rsplen-2]))<<8));
		uam->sw = sw;

		//diag_printf("[T0Transfers (case 2)] - *sw = 0x%x", *sw);

		pBufOutLen = rsplen - 3;
		uam->pBufOutLen = pBufOutLen;
		memcpy(pBufOut, rsp+1, pBufOutLen);

	}
	else if(bufInLen > 5 && bufInLen == (int)pBufIn[4] + 5)
	{

		//inno_msg("kwjing - inno_uam_transfer, bufInLen > 5");
		cmd[0] = SEND_UAM_CMD;
		cmd[1] = 0xAA;
		cmd[2] = 0x55;		

		if(bufInLen <= 256)    
		{
			cmd[3] = 0x00;
			cmd[4] = bufInLen;	
		}
		else                //check?
		{		
			//cmd[1] = bufInLen& 0xff;  
			//cmd[2] = (bufInLen& 0xff00) >> 8;
			inno_err("[inno_uam_transfer (case 3)] - bufInLen > 256!!!");
			return INNO_ERROR;  			  			  
		}	
		memcpy(cmd+9, pBufIn, bufInLen);
		cmdLen = bufInLen + 9; 

		ret = INNO_Send_UAM_Cmd(cmd, cmdLen);
		if(ret != INNO_SUCCESS)  
		{
			inno_err("[T0Transfers (case 3)] - send cmd fail!!!");
			return INNO_ERROR;
		}

		//inno_msg("kwjing - inno_uam_trans , INNO_Read_UAM_Rsp");
		ret = INNO_Read_UAM_Rsp(rsp, &rsplen);
		if(ret != INNO_SUCCESS)
		{
			inno_err("[T0Transfers (case 3)] - response fail!!!");
			return INNO_ERROR;      
		}

		if(rsplen < 3)
		{
			inno_err("[T0Transfers (case 3)] - Get SW1, SW2 error!!!");
			inno_err("inno_uam_transfer, bufInLen=%d", bufInLen);
			inno_err("inno_uam_transfer, pBufIn: ");
			for(i = 0; i< bufInLen; i++)
				inno_err("0x%X,",pBufIn[i]);
			inno_err("inno_uam_transfer,rspLen=%d", rsplen);
			inno_err("inno_uam_transfer rsp:");
			for(i = 0; i< rsplen; i++)
				inno_err("0x%X,", rsp[i]);

			return INNO_ERROR;    
		}

		//		inno_msg("kwjing - inno_uam_trans , after INNO_Send_UAM_Cmd 00: rsplen: %d, rsplen-1: 0x%x, rsplen-2: 0x%x, rsplen0: 0x%x", 
		//		rsplen, rsp[rsplen-1], rsp[rsplen-2], rsp[0]);
		//		sw = MAKEWORD(rsp[rsplen-1], rsp[rsplen-2]);
		sw = ((unsigned short)((unsigned char)(rsp[rsplen-1])) | (((unsigned short)(unsigned char)(rsp[rsplen-2]))<<8));
		uam->sw = sw;
		//diag_printf("[T0Transfers (case 3)] - sw = 0x%x", *sw);

		//inno_msg("kwjing - inno_uam_trans , after INNO_Send_UAM_Cmd 01: pBufOutLenaaa: %d, sw: 0x%x", (rsplen - 3), sw);
		pBufOutLen = rsplen - 3;
		uam->pBufOutLen = pBufOutLen;
		memcpy(pBufOut, rsp+1, pBufOutLen);
		//inno_msg("kwjing - inno_uam_trans , after INNO_Send_UAM_Cmd 02");

	}
	else if(bufInLen > 5 && bufInLen > (int)pBufIn[4] + 5)
	{
		inno_err("inno_uam_transfer, bufInLen > 5 return!");
		return INNO_ERROR;
	}

#ifdef CMMB_DEBUG
	inno_dbg("inno_uam_transfer, pBufOutLen=%d", pBufOutLen);
	inno_dbg("inno_uam_transfer, pBufOut: ");
	for(i = 0; i< pBufOutLen; i++){
		inno_dbg("0x%X,", pBufOut[i]);
	}
#endif

	inno_msg("OK, sw=0x%X",sw); 
	inno_dbg("----- inno_uam_transfer -----"); 

	return INNO_SUCCESS;	
}

int INNO_Send_UAM_Cmd(unsigned char *cmd, int cmdLen)
{
	unsigned char cmd_status = 0;
	int j = 0;
	INNO_RET ret = INNO_NO_ERROR;

	//cyg_flag_maskbits(&inno_uam_event, 0);

	inno_dbg("===============> INNO_Send_UAM_Cmd ");
	mutex_lock(&inno_spi_mutex);
	ret=INNO_SPI_Write_Byte_Type2(UAM_BASE_ADDR, (cmd+1), (cmdLen-1));
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Write_Byte_Type2 fail");
		mutex_unlock(&inno_spi_mutex);
		return INNO_ERROR;
	}

	for(j = 0; j < 20; j++)
	{
		ret=INNO_SPI_Read_Byte_Type2(FETCH_PER_COMM31, &cmd_status, 1);
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Read_Byte_Type2 fail");
			mutex_unlock(&inno_spi_mutex);
			return INNO_ERROR;
		}
		inno_msg("cmd_status =%d",cmd_status);
		if((cmd_status & CMD_BUSY) == 0)   //bit7:1 cmd busy; 0 cmd over
		{
			cmd_status = CMD_BUSY;
			ret=INNO_SPI_Write_Byte_Type2(FETCH_PER_COMM31, &cmd_status, 1);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Write_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return INNO_ERROR;
			}
			ret=INNO_SPI_Write_Byte_Type2(FETCH_PER_COMM0, &cmd[0], 1);
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Write_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return INNO_ERROR;
			}
			mutex_unlock(&inno_spi_mutex);			
			return INNO_SUCCESS;
		}
	}

	mutex_unlock(&inno_spi_mutex);
	inno_msg("warning INNO_SPI_Read_Byte_Type2 timeout");
	return INNO_TIMEOUT_ERROR;
}

static int INNO_Read_UAM_Rsp(unsigned char *rsp, unsigned int *rsplen)
{	
	struct inno_lgx *lgx = g_inno_dev.lgxs[UAM];	
	struct inno_buffer *buffer = &lgx->inno_buffer;	
#ifdef UAM_COMPLETE
	if(wait_for_completion_timeout(&lgx->uam_complete, msecs_to_jiffies(5000))){
		if(buffer->valid == 1) {
			down(&buffer->sem);
			if(buffer->start ==NULL){    //check INNO_SPI_Read_Byte_Type2 memcpy KE ERROR
				inno_err("buffer->start ==NULL");
				buffer->valid = 0;
				up(&buffer->sem);
				return -ETIME;
			}
			memcpy(rsp, buffer->start, buffer->valid_size);
			*rsplen = buffer->valid_size;
			inno_msg("OK,Complete,GotRSP!");

			buffer->valid = 0;
			up(&buffer->sem);
		}
		else{
			inno_msg("buffer->valid!=1");
		}
	}
	else{
		inno_msg("timeout 5s");
		return -ETIME;
	}
	return 0;
#else
	int err = 0;
	DECLARE_WAITQUEUE(wait, current);	

	add_wait_queue(&lgx->uam_wait, &wait);		
	inno_dbg("INNO_Read_UAM_Rsp");		
	while(1) {
		err = -ERESTARTSYS;	
		if(signal_pending(current))
			break;
		if(buffer->valid == 1) {
			down(&buffer->sem);
			if(buffer->start ==NULL){    //check INNO_SPI_Read_Byte_Type2 memcpy KE ERROR
				inno_err("buffer->start ==NULL");
				buffer->valid = 0;
				up(&buffer->sem);
				break;
			}
			memcpy(rsp, buffer->start, buffer->valid_size);
			*rsplen = buffer->valid_size;
			inno_msg("GotRSP!");

			buffer->valid = 0;
			up(&buffer->sem);
			break;
		}
		set_current_state(TASK_INTERRUPTIBLE);
		err = -EAGAIN;
		inno_dbg("UAM wait queue, sleep .........");
		schedule();
		//msleep(1);
		inno_dbg("waiting..., buffer->valid:%d, buffer->valid_size: %d", buffer->valid, buffer->valid_size);
	}

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&lgx->uam_wait, &wait);
	return 0;
#endif

}
