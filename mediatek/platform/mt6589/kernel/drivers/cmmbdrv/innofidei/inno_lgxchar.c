

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <linux/poll.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
//#include <mach/pxa-regs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include "if208.h"
#include "if101_control_proto.h"

#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>

#include <asm/uaccess.h>

//According to chip and clock, select fw
#if defined(FLAG_CHIP_IF228_20M)
#include "Innofw_IF228_20M.h"
#elif defined(FLAG_CHIP_IF228_26M)
#include "Innofw_IF228_26M.h"
#elif defined(FLAG_CHIP_IF238_20M)
#include "Innofw_IF238_20M.h"
#elif defined(FLAG_CHIP_IF238_26M)
#include "Innofw_IF238_26M.h"
#elif defined(FLAG_CHIP_IF258_20M) 
#include "Innofw_IF258_20M.h"
#elif defined(FLAG_CHIP_IF258_26M)
#include "Innofw_IF258_26M.h" 
#else
#include "Innofw_IF228_20M.h"              //default:IF228_20M
#endif

extern struct inno_device g_inno_dev;
extern struct mutex inno_spi_mutex;

#include "inno_spi.h"
#ifdef MTK_SPI                                                                      //xingyu add
#include <linux/spi/spi.h>
#endif

int cancle_wait =0;
int flag_memset = 0;
int flag_close =1;
int once_closeirq =0;
int flag_spi_ok =0;

#ifdef _check_block_time                                                       // check no eint issue
static struct workqueue_struct *check_block_wq=NULL; 
static struct work_struct check_block_work;
extern int intCount;
extern INNO_RET INNO_GetFirmwareVersion(unsigned char *version);
#endif

#define _buffer_global
#ifdef  _buffer_global
      unsigned char* g_inno_buffer =NULL;
#endif

extern int INNO_GetFirmwareBytes(void);
extern int INNO_Chip_errHandle(void);
extern int inno_init_uam(void);
extern INNO_RET INNO_Send_Cmd(unsigned char *cmd);
extern int INNO_Send_UAM_Cmd(unsigned char *cmd, int cmdLen);

extern int innodev_startup(inno_device_t* inno_dev);
extern int innodev_shutdown(inno_device_t* inno_dev);
extern int inno_spi_r(unsigned char cmd, unsigned char *rsp);
extern void inno_irq_release(void);

static struct inno_lgx_ids lgx_ids_table[] = {
	{"LG0", LG0},
	{"LG1", LG1},
	{"LG2", LG2},                                     //xingyu 0922
	{"UAM", UAM},
	{NULL,}
};

//static unsigned int uam_cmd_len = 0;
struct inno_uam_parameter uam_para;
//static unsigned int response = 0;

#if 1
int inno_spi_drive_cs(int drive)
{
	return 0;
}
#endif 

static int lgx_alloc_buffer(struct inno_lgx *lgx)
{
	//int i,j;
	//int page_num;
	//struct page *page = NULL;
	int i;
	struct inno_buffer *inno_buf = &lgx->inno_buffer;
	//int ret = 0;

	memset(inno_buf, 0, sizeof(struct inno_buffer));
	sema_init(&inno_buf->sem,1);
	down(&inno_buf->sem);

	// alloc buffer
#if 0                        //xingyu buffer issue
	page_num = PAGE_ALIGN(INNO_BUFFER_SIZE) / PAGE_SIZE;
	inno_buf->pages = (struct page **)kzalloc(page_num * sizeof(struct page *), GFP_KERNEL);         
	if (!inno_buf->pages) {
		inno_err("lgx_alloc_buffer:No enough memory");
		ret = -ENOMEM;
		goto alloc_node_error;
	}

	for (i = 0; i < page_num; i++) {
		page = alloc_page(GFP_KERNEL);

		if (!page) {
			inno_err("lgx_alloc_buffer:No enough page");
			ret = -ENOMEM;
			goto alloc_pages_error;
		}
		//SetPageReserved(page);
		inno_buf->pages[i] = page;
	}

	inno_buf->page_num = page_num;

	inno_buf->bufsize = page_num * PAGE_SIZE;
	inno_buf->vaddr = vmap(inno_buf->pages, page_num, VM_MAP, PAGE_KERNEL);

	/* check if the memory map is OK. */
	if (!inno_buf->vaddr) {
		inno_err("lgx_alloc_buffer:vmap() failure");
		ret = -EFAULT;
		goto vmap_error;
	}

	memset(inno_buf->vaddr, 0, inno_buf->bufsize);

	inno_buf->start = inno_buf->vaddr;
#else	
#ifndef _buffer_global                                                      // buffer alloc modify xingyu 0714
	inno_buf->vaddr = kmalloc(INNO_BUFFER_SIZE, GFP_KERNEL| GFP_DMA);         
	if(inno_buf->vaddr == NULL || (int)inno_buf->vaddr %4 !=0){
		inno_err("inno_buf->vaddr kmalloc fail");
		up(&inno_buf->sem);
		return -1;
	}			 
#else
	for(i=0; lgx_ids_table[i].name!=NULL; i++){
		if(lgx->ids  == &lgx_ids_table[i]){
			inno_buf->vaddr =i*INNO_BUFFER_SIZE+g_inno_buffer;
			inno_msg("use global mem");
			break;
		}
	}
#endif
	inno_buf->start = inno_buf->vaddr;
	inno_buf->bufsize = INNO_BUFFER_SIZE;
	
#endif
	up(&inno_buf->sem);

	return 0;

#if 0                          //xingyu buffer issue
	//vmap_error:
	//alloc_pages_error:
	for (j = 0; j < i; j++) {
		page = inno_buf->pages[j];
		//ClearPageReserved(page);
		__free_page(page);
	}
	kfree(inno_buf->pages);
	//alloc_node_error:
	return ret;
#endif
}

static void lgx_free_buffer(struct inno_lgx *lgx)
{
	//int i;
	//struct page *page = NULL;
	struct inno_buffer *inno_buf = &lgx->inno_buffer;

	/*
	 * vunmap will do TLB flush for us.
	 */
	down(&inno_buf->sem);
#if 0                       //xingyu buffer issue
	vunmap(inno_buf->vaddr);
	inno_buf->vaddr = NULL;

	for (i = 0; i < inno_buf->page_num; i++) {
		page = inno_buf->pages[i];
		//ClearPageReserved(page);
		__free_page(page);
	}

	kfree(inno_buf->pages);
#else
	if(inno_buf->vaddr !=NULL){
#ifndef _buffer_global                                                      // buffer alloc modify xingyu 0714
		kfree(inno_buf->vaddr );
#endif
		inno_buf->vaddr  =NULL;
		inno_buf->start =NULL;
		inno_buf->bufsize = 0;
	}
#endif
	up(&inno_buf->sem);

	memset(inno_buf, 0, sizeof(struct inno_buffer));
}

#ifdef _check_block_time                             // check no eint issue
static void check_block_thread(void *arg)
{
	inno_msg("runing for check block status\n");
	int oldcount=intCount;
	msleep(1000*60);                        //wait 60s for to get mfs
	int version=0;
	INNO_GetFirmwareVersion(&version);
	inno_msg("version =%d",version);
	while(1){
		if (intCount ==oldcount) {
			inno_msg("error over 20 second no interrup,then gerversion");
#if 1
			//xingyu debug get firmware the first 8 bytes
			inno_msg("INNO_GetFirmwareBytes for get the first 8 bytes");
			int ret =INNO_GetFirmwareBytes();
			if(ret != INNO_NO_ERROR)       {
				inno_err("INNO_GetFirmwareBytes fail");
			}	
#else	
			INNO_GetFirmwareVersion(&version);
			inno_msg("version =%d",version);
#endif				
			break;
		}
		else
			oldcount = intCount;
		msleep(1000*10);
	}
	msleep(1000*3);
	inno_msg("exit");

}
#endif

/*====================================
 * character device file operations
 *===================================*/
static int lgx_open(struct inode* inode, struct file* filp)
{
	struct inno_lgx* lgx;
	unsigned int lg_num = iminor(inode);
	int i;                    
	inno_msg("+++++++++++++++++++++++++++++++++++++++++");
	inno_msg("CMMB IF208 open [%s]", lgx_ids_table[lg_num].name);
	inno_msg("+++++++++++++++++++++++++++++++++++++++++");

	if(lg_num >= LG_END){
		inno_err("minor error, should be [%d-%d]", LG0, UAM);
		return -1;
	}

	if(g_inno_dev.lgxs[lg_num]){ 	//already opened, just increase reference count
		down(&g_inno_dev.lgxs[lg_num]->sem);
		g_inno_dev.lgxs[lg_num]->use_count++;                           
		//g_inno_dev.lgxs[lg_num]->use_count = 1;
		inno_msg("reopen");
		up(&g_inno_dev.lgxs[lg_num]->sem);
		filp->private_data = g_inno_dev.lgxs[lg_num];
		return 0;
	}	

	/* OK, it's not open, Create it */
	lgx = (struct inno_lgx*)kmalloc(sizeof(struct inno_lgx), GFP_KERNEL);
	if(!lgx){
		inno_err("kmalloc lgx ==NULL");
		return -1;
	}
	else
		memset(lgx, 0, sizeof(struct inno_lgx));

	for(i=0; lgx_ids_table[i].name!=NULL; i++){
		if(lg_num == lgx_ids_table[i].id){
			lgx->ids = &lgx_ids_table[i];
			break;
		}
	}
	//allocat memory for get mfs file
	if (lgx_alloc_buffer(lgx)){
		kfree(lgx);
		lgx = NULL;
		return -1;
	}
	
//**********  global flag set ******************//
	cancle_wait = 0;                                   //xingyu 0328 add   solve poll wait
	flag_memset = 0;                                // memset the bitstream buffer
	once_closeirq=0;                                 // control close irq

	lgx->use_count = 1;
	sema_init(&lgx->sem,1);
	init_waitqueue_head(&lgx->read_wait);
#ifndef UAM_COMPLETE
	init_waitqueue_head(&lgx->uam_wait);
#else
//	if(lg_num==2){                // open uam
	if(strcmp(lgx->ids->name, "UAM") == 0){
	//if(lgx->ids->name =="UAM") {                          //xing 0922  use uamcomplete function
		inno_msg("uam_complete init");
		init_completion(&lgx->uam_complete);
	}
#endif
	g_inno_dev.lgxs[lg_num] = lgx;

	//inno_msg("open file for inno_num: %d\n", lg_num);
	if( innodev_startup(&g_inno_dev) ){
		inno_err("startup fail, free lgx");
		g_inno_dev.lgxs[lg_num] = NULL;
		lgx_free_buffer(lgx);
		kfree(lgx);
		return -1;
	}
	filp->private_data = lgx;
#ifdef _check_block_time            //printk rxbuf
	static int onetime =0;
	if (onetime ==0){                                     // when open file create better than module init for time
		onetime++;
		inno_msg("use thread to watch the block time");
		check_block_wq = create_singlethread_workqueue("siano work wq");
		INIT_WORK(&check_block_work,check_block_thread);
		queue_work(check_block_wq,&check_block_work);
	}
#endif
       inno_msg("use single thread for workqueue");
	return 0;
}



static ssize_t lgx_read(struct file* filp, char* buffer, size_t count, loff_t* offset)
{
	DECLARE_WAITQUEUE(wait, current);
	int ret = 0;
	int err = 0;
	//int i=0;              
	struct inno_lgx* lgx = (struct inno_lgx*)filp->private_data;
	struct inno_buffer* inno_buf = &lgx->inno_buffer;
	//char temp[5];
	inno_dbg("[CMMB]%s+\n",__FUNCTION__);
	add_wait_queue(&lgx->read_wait, &wait);
	while(1){
		err = -ERESTARTSYS;	
		if(signal_pending(current))
			break;
		if(inno_buf->valid && inno_buf->own_id==lgx->ids->id)
		{
			down(&inno_buf->sem);
			inno_dbg("inno_buf->valid_size: %d,count: %d\n",inno_buf->valid_size,count);
			ret = min(inno_buf->valid_size, count);
			if(ret<=0){
				inno_err("inno_buf->valid_size <0");
				up(&inno_buf->sem);
				return 0;
			}
			err = copy_to_user(buffer, &(inno_buf->own_id), 1);
			err = copy_to_user(buffer+1, inno_buf->start, ret);
#if 0         			//check copy to user mfs       
			memcpy(temp,inno_buf->start, 5);                     
			inno_dbg("check copy to user mfs len:%d,buf\n",ret);
			for(i=0;i<6;i++)
				printk(" %02x",temp[i]);
			printk("\n");
#endif
			inno_buf->valid = 0;
			up(&inno_buf->sem);
			//inno_msg("[lgx_read] dump to /local/log/%d.cmmb, size:%d\n",ind_log,ret);
			//write_file(inno_buf->start, ret);
			break;
		}
		set_current_state(TASK_INTERRUPTIBLE);
		err = -EAGAIN;
		if(filp->f_flags & O_NONBLOCK)
			break;
		schedule();
	}
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&lgx->read_wait, &wait);
	inno_dbg("[CMMB]%s-",__FUNCTION__);
	return ret;
}

static unsigned int lgx_poll(struct file* filp, poll_table *wait)
{
	unsigned int ret = 0;
	struct inno_lgx* lgx = (struct inno_lgx*)filp->private_data;
	struct inno_buffer* inno_buf = &lgx->inno_buffer;
	poll_wait(filp, &lgx->read_wait, wait);

	if(inno_buf->valid && inno_buf->own_id==lgx->ids->id)
		ret = POLLIN | POLLRDNORM;

	if(cancle_wait){
		inno_msg("lgx->cancle_wait");
		ret = POLLNVAL;
	}
	return ret;
}
//static int lgx_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
static long lgx_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;
	long val;
//	inno_device_t* inno_dev = &g_inno_dev;

	/* FIHTDC, ALXiao, add cmmb ftm test function, 20101008 { */
	if(_IOC_TYPE(cmd) != INNO_IOC_MAGIC)
		return -ENOTTY;
	/* FIHTDC, ALXiao, add cmmb ftm test function, 20101008 } */

	inno_msg("[CMMB]%s+",__FUNCTION__);

	// ASUS_BSP+++ JimmyLin "[CMMB] avoid UI hang"
	// msleep(100);
	// ASUS_BSP--- 
	switch(cmd){
		case INNO_STOP_POLL:                // stop polling,avoid poll block,then driver can't deinit
			{
				cancle_wait  =(u8)arg;
				inno_msg("[lgx_ioctl] INNO_STOP_POLL cancle_wait =%d",cancle_wait);
				if(cancle_wait){
					struct inno_lgx* lgx0 = g_inno_dev.lgxs[0];
					struct inno_lgx* lgx1 = g_inno_dev.lgxs[1];
					struct inno_lgx* lgx2 = g_inno_dev.lgxs[2];
					struct inno_lgx* lgx3 = g_inno_dev.lgxs[3];
					wake_up_interruptible(&lgx0->read_wait);
					wake_up_interruptible(&lgx1->read_wait);
					wake_up_interruptible(&lgx2->read_wait);
					wake_up_interruptible(&lgx3->read_wait);
				}
				break;
			}
		case INNO_MEMSET_MFS:                      // memset mfs buffer before fetch mfs data
			{
				flag_memset= (u8)arg; 
				inno_msg("[lgx_ioctl] INNO_MEMSET_MFS flag_memset =%d",flag_memset);
				break;
			}
#if 0
		case INNO_GET_INTR_TYPE:                              //no use
			inno_msg("[lgx_ioctl] INNO_GET_INTR_TYPE");
			val = inno_dev->cfg.intr_type;
			put_user(val, (long*)arg);
			break;
		case INNO_READ_REG:                                  //no use
			{
				struct inno_reg_data* reg_data;
				inno_msg("[lgx_ioctl] INNO_READ_REG");
				reg_data = kmalloc(sizeof(struct inno_reg_data), GFP_KERNEL);
				retval = copy_from_user(reg_data, (void*)arg, sizeof(struct inno_reg_data));
				if(retval == 0){
					/*
					   down(&inno_iic_mutex);
					   g_inno_dev.i2c_driver->read(reg_data->reg, (unsigned char*)&reg_data->data);	
					   up(&inno_iic_mutex);
					   retval = copy_to_user((void*)arg, reg_data, sizeof(struct inno_reg_data));
					 */
				}
				kfree(reg_data);
				break;
			}
		case INNO_WRITE_REG:                                 //no use
			{
				struct inno_reg_data* reg_data;
				inno_msg("[lgx_ioctl] INNO_WRITE_REG");
				reg_data = kmalloc(sizeof(struct inno_reg_data), GFP_KERNEL);
				retval = copy_from_user(reg_data, (void*)arg, sizeof(struct inno_reg_data));
				if(retval == 0){
					/*
					   down(&inno_iic_mutex);
					   g_inno_dev.i2c_driver->write(reg_data->reg, reg_data->data);
					   up(&inno_iic_mutex);
					   retval = copy_to_user((void*)arg, reg_data, sizeof(struct inno_reg_data));
					 */
				}
				kfree(reg_data);
				break;
			}
		case INNO_MMIS_READ:                     	//no use
			{
				u8 reg  = (u8)arg;
				u8 value;
				inno_msg("[lgx_ioctl] INNO_MMIS_READ");
				mutex_lock(&inno_spi_mutex);
				inno_spi_drive_cs(0);
				g_inno_dev.spi_driver->write(&reg, 1);
				inno_spi_drive_cs(1);

				inno_spi_drive_cs(0);
				g_inno_dev.spi_driver->read(&value, 1);
				inno_spi_drive_cs(1);

				retval = value;
				mutex_unlock(&inno_spi_mutex);
				break;
			}
		case INNO_MMIS_CMD:	                       //no use
			{
				u8 value = (u8)arg;
				inno_msg("[lgx_ioctl] INNO_MMIS_CMD");
				mutex_lock(&inno_spi_mutex);
				inno_spi_drive_cs(0);		
				g_inno_dev.spi_driver->write(&value,1);
				inno_spi_drive_cs(1);		
				mutex_unlock(&inno_spi_mutex);
				break;
			}
		case INNO_MMIS_WRITE:	          //pass value	mmis_cmd|value no use
			{
				u8 cmd, value;
				inno_msg("[lgx_ioctl] INNO_MMIS_WRITE");
				val = arg;
				cmd = (val & 0xFF00)>>8;
				value = (val & 0x00FF);
				mutex_lock(&inno_spi_mutex);
				inno_spi_drive_cs(0);		
				g_inno_dev.spi_driver->write(&cmd, 1);
				inno_spi_drive_cs(1);

				inno_spi_drive_cs(0);		
				g_inno_dev.spi_driver->write(&value, 1);
				inno_spi_drive_cs(1);
				mutex_unlock(&inno_spi_mutex);
				break;
			}
		case INNO_SCAN_FREQUENCY:                   // if228 not support scan function  no use
			{
				struct inno_freq_scan_area* 	scan_area;
				inno_msg("[lgx_ioctl] INNO_SCAN_FREQUENCY");
				scan_area = kmalloc(sizeof(struct inno_freq_scan_area), GFP_KERNEL);
				retval = copy_from_user(scan_area, (void*)arg, sizeof(struct inno_freq_scan_area));

				if(retval == 0)
					;//retval = inno_scan_frequency(scan_area->start, scan_area->end);
				kfree(scan_area);
				break;
			}
		case INNO_SCAN_FREQUENCY_DOT:              // if228 not support scan function no use
			{
				struct inno_freq_scan_area*		scan_area;
				inno_msg("[lgx_ioctl] INNO_SCAN_FREQUENCY_DOT");
				scan_area = kmalloc(sizeof(struct inno_freq_scan_area), GFP_KERNEL);
				retval = copy_from_user(scan_area, (void*)arg, sizeof(struct inno_freq_scan_area));
				if(retval == 0)
					;//retval = inno_scan_frequency_dot((u8)scan_area->start, (u8)scan_area->end);
				kfree(scan_area);
				break;
			}
#endif
#if 0
		case INNO_SET_FREQUENCY:	// unit:MHZ
			{
				inno_msg("[lgx_ioctl] INNO_SET_FREQUENCY");
				retval = copy_from_user(&val, (void*)arg, sizeof(unsigned long));
				inno_msg("[inno-ioctl]: INNO_SET_FREQUENCY, val=%ld", val);

				if(retval == 0)
					retval = inno_set_frequency((u16)val);

				break;
			}
#endif		
		case INNO_SET_FREQUENCY_DOT:
			{
				inno_msg("[lgx_ioctl] INNO_SET_FREQUENCY_DOT");
				retval = copy_from_user(&val, (void*)arg, sizeof(unsigned long));
				inno_msg("[inno-ioctl]: INNO_SET_FREQUENCY_DOT, val=%ld", val);
				if(retval == 0)
					retval = inno_set_frequency_dot((u8)val);
				if(retval)
					retval = -1;
				break;
			}
#if 0
		case INNO_SET_CP_TYPE:                    // no use   
			{
				inno_msg("[lgx_ioctl] INNO_SET_CP_TYPE");
				retval = copy_from_user(&val, (void*)arg, sizeof(unsigned long));
				if(retval == 0)
					retval = inno_set_cp_type((u8)val);
				break;
			}
#endif
		case INNO_GET_CHANNEL_CONFIG:                         
			{
				struct inno_channel_config* 	cfg;
				inno_msg("[lgx_ioctl] INNO_GET_CHANNEL_CONFIG");
				cfg = kmalloc(sizeof(struct inno_channel_config), GFP_KERNEL);
				if(!cfg ){
					inno_err("kmalloc cfg ==NULL");
					retval =-1;
					break;
				}

				retval = copy_from_user(cfg, (void*)arg, sizeof(struct inno_channel_config));

				if(retval == 0)
					retval =INNO_GetChannelConfig(cfg);            
				if(!retval){
					retval = copy_to_user((void*)arg, cfg, sizeof(struct inno_channel_config));
					inno_msg("[lgx_ioctl] INNO_Get_CHANNEL_CONFIG:cid(%d),close(%d),start(%d),count(%d),ldpc(%d),itlv(%d),rs(%d),mod(%d) "
							,cfg->ch_id,cfg->ch_close,cfg->ts_start,cfg->ts_count,cfg->ldpc,cfg->itlv,cfg->rs,cfg->modulate);			
				}
				else
					retval =-1;
				kfree(cfg);
				break;	
			}
		case INNO_SET_CHANNEL_CONFIG:                 
			{
				struct inno_channel_config* 	cfg;
				cfg = kmalloc(sizeof(struct inno_channel_config), GFP_KERNEL);
				if(!cfg ){
					inno_err("kmalloc cfg ==NULL");
					retval =-1;
					break;
				}
				inno_msg("[inno-ioctl]: INNO_SET_CHANNEL_CONFIG");
				retval = copy_from_user(cfg, (void*)arg, sizeof(struct inno_channel_config));
				inno_msg("[lgx_ioctl] INNO_SET_CHANNEL_CONFIG:cid(%d),close(%d),start(%d),count(%d),ldpc(%d),itlv(%d),rs(%d),mod(%d),subframeID(%d) "
						,cfg->ch_id,cfg->ch_close,cfg->ts_start,cfg->ts_count,cfg->ldpc,cfg->itlv,cfg->rs,cfg->modulate,cfg->subframe_ID);
				if(retval == 0)
					retval = inno_set_channel_config(cfg);
				kfree(cfg);
				break;	
			}
		case INNO_GET_SYS_STATE:
			{
				struct inno_sys_state*	sys_state;
				inno_msg("[lgx_ioctl] INNO_GET_SYS_STATE");
				sys_state = kmalloc(sizeof(struct inno_sys_state), GFP_KERNEL);
				if(!sys_state){
					inno_err("kmalloc fail sys_state = null");
					retval =1;
					break;
				}
				retval = copy_from_user(sys_state, (void*)arg, sizeof(struct inno_sys_state));

				if(retval == 0)
					retval = inno_get_system_state(sys_state);
				if(!retval)
					retval = copy_to_user((void*)arg, sys_state, sizeof(struct inno_sys_state));
				else
					retval = -1;
				kfree(sys_state);
				break;
			}
#if 0                             // no use
		case INNO_SET_PM:                                    // no use
			{
				struct inno_pm_control* pmctl;
				inno_msg("[lgx_ioctl] INNO_SET_PM");
				pmctl = kmalloc(sizeof(struct inno_pm_control), GFP_KERNEL);
				retval = copy_from_user(pmctl, (void*)arg, sizeof(struct inno_pm_control));

				if(retval == 0)
					retval = inno_set_pm(pmctl);
				kfree(pmctl);
				break;
			}
		case INNO_GET_FW_VER:                                 //no use
			{	
				struct inno_fw_ver ver = {0, 0};
				inno_msg("[lgx_ioctl] INNO_GET_FW_VER");
				inno_get_fireware_version(&ver);
				retval = copy_to_user((void*)arg, &ver, sizeof(struct inno_fw_ver));
				break;
			}
#endif
		case INNO_FW_DOWNLOAD:
			{
				int retry=0;
			//	struct INNOCHAR_FW_DATA *up = (struct INNOCHAR_FW_DATA *)arg;
				unsigned char* fwbuf =NULL;
				//inno_msg("check up->fw_size =%d",up->fw_size);                    
				//unsigned char* fwbuf = kmalloc(up->fw_size, GFP_KERNEL| GFP_DMA);
                                inno_msg("download using h file in kernel");
				if(FIRMWARE_BUF_DATA_SIZE < 40960)
					fwbuf =3*INNO_BUFFER_SIZE+g_inno_buffer;
				else{
					inno_err("fw_size >40k fail");
					retval = -1;
					break;
				}			 

				//int pass=0;
				//INNO_RET ret_tmp = INNO_NO_ERROR;

				inno_msg("[lgx_ioctl] INNO_FW_DOWNLOAD");
                                memset(fwbuf,0,FIRMWARE_BUF_DATA_SIZE);
			//	memset(fwbuf, 0, up->fw_size);
	         	//	retval = copy_from_user(fwbuf, (void*)up->fwbuf, up->fw_size);
                                memcpy(fwbuf,fw_buf,FIRMWARE_BUF_DATA_SIZE);
#if 0            				//check copy fw
				int i;
				inno_msg("check copy_from_user fw_bu");
				for(i=0;i<4;i++)
					inno_msg(" %02x",fwbuf[i]);
				inno_msg("end");
				for(i=0;i<4;i++)
					inno_msg(" %02x",fwbuf[up->fw_size-i-1]);
				inno_msg("end");	
				//mtk_spi_test();
				//break;
				//inno_msg("******** sleep 1s for download fw");
				//msleep(5000);
#endif
				if(retval == 0){
					for(retry=0;retry<1;retry++){                                
						retval = inno_download_fireware(fwbuf,FIRMWARE_BUF_DATA_SIZE);
						if(retval){
							inno_err("firmware download failed retval =%d!",(int)retval);
							retval =  -EIO;
							continue;
						}
#if 0
						//	mdelay(100);                                                       //no need delay,refer to innofidei code
						inno_msg("inno_download_fireware OK,then check FW");
						ret_tmp = INNO_CheckFWDownload(fw_buf,up->fw_size,&pass);
						if(ret_tmp){
							inno_err("INNO_CheckFWDownload fail ret =%d",ret_tmp);
							retval = -1;
							continue;
						}
						if(!pass){
							inno_err("FW Download Fail, checksum no pass =%d!",pass);
						}
						else{
							inno_msg("INNO_CheckFWDownload success");	
							break;
						}
#else
						else{
							inno_msg("inno_download_fireware ok");
							break;
						}
#endif
					}
					if(retry ==5){
						inno_err("inno_download_fireware timeout");
						retval = -1;
					}
				}
				//kfree(fwbuf);
				break;
			}
#if 0                               // no use
		case INNO_UPDATE_FW:                         // no use
			{
				inno_msg("[lgx_ioctl] INNO_UPDATE_FW");
				inno_update_fireware();
				break;
			}
#endif
		case INNO_GET_FW_BYTES:
			{
				int ret =INNO_Chip_errHandle();
				if(ret != INNO_NO_ERROR)       {
					inno_err("INNO_Chip_errHandle fail");
					retval =-1;   
				}	
				break;
			}
		case INNO_ENABLE_IRQ:
			inno_msg("[lgx_ioctl] INNO_ENABLE_IRQ");
			break;
		case INNO_DISABLE_IRQ:
			inno_msg("[lgx_ioctl] INNO_DISABLE_IRQ");
			break;
#if 0
		case INNO_SHUTDOWN_IRQ:                                    // no use
			{	//NOTE: cleanup all (i2c, spi, power down) mannually while, NOT the device itself
				inno_msg("[lgx_ioctl] INNO_SHUTDOWN_IRQ");
				if(!g_inno_dev.use_count)
				{
					inno_msg("shutdown innodev. %d", g_inno_dev.use_count);
					innodev_shutdown(&g_inno_dev);
					g_inno_dev.use_count = 0; //unrefer all reference counter
				}
				break;
			}
		case INNO_GET_BUFFERLEN:                                        //xingyu getbufferlen
			val = INNO_BUFFER_SIZE;
			put_user(val, (long*)arg);
			inno_msg("[lgx_ioctl] INNO_GET_BUFFERLEN:%ld",val);
			break;
#endif
			//////////////////////////// uam ///////////////////////////
		case INNO_UAM_TRANSFER:
			{
				struct inno_uam_parameter *uam_para_tmp;
				inno_msg("[lgx_ioctl] INNO_UAM_TRANSFER");
				uam_para_tmp = kmalloc(sizeof(struct inno_uam_parameter), GFP_KERNEL);
				if(!uam_para_tmp){
					inno_msg("uam_para_tmp kmalloc fail");
					retval = -1;
					break;
				}			 
				retval = copy_from_user(uam_para_tmp, (void*)arg, sizeof(struct inno_uam_parameter));

				if(retval == 0)
					retval = inno_uam_transfer(uam_para_tmp);
				if(!retval)
					retval = copy_to_user((void*)arg, uam_para_tmp, sizeof(struct inno_uam_parameter));     
				else
					retval = -1;
				kfree(uam_para_tmp);
				uam_para_tmp=NULL;
			}
			break;
		case INNO_UAM_INIT:
			inno_msg("[lgx_ioctl] INNO_UAM_INIT");
			retval = inno_init_uam();
			if(retval !=0){
				retval =-1;                   //tell user space fail
				inno_err("init uam fail");
			}
			break;		

		case INNO_SEND_UAM_CMD:
			{
				unsigned char* cmd_tmp0;
				struct uam_cmd_par* up0 = (struct uam_cmd_par*)arg;
				inno_msg("[lgx_ioctl] INNO_SEND_UAM_CMD");
				cmd_tmp0= kmalloc(up0->len, GFP_KERNEL);
				if(cmd_tmp0 == NULL){
					inno_msg("cmd_tmp0 kmalloc fail");
					retval = -1;
					break;
				}			 
				memset(cmd_tmp0, 0, up0->len);
				retval = copy_from_user(cmd_tmp0, (void*)up0->cmd, up0->len);

				if(retval ==0)
					retval = INNO_Send_UAM_Cmd(cmd_tmp0,up0->len);
				if(retval)
					retval = -1;
				kfree(cmd_tmp0);
				cmd_tmp0 =NULL;
			}
			break;		
		case INNO_SEND_CMD:
			{
				unsigned char *cmd_tmp;
				inno_msg("[lgx_ioctl] INNO_SEND_CMD");
				cmd_tmp = kmalloc(8,GFP_KERNEL);
				if(!cmd_tmp){
					inno_err("kmalloc fail,cmd_tmp==null");
					retval=-1;
					break;
				}
				retval = copy_from_user(cmd_tmp, (void*)arg, 8);
				if(retval ==0){
					mutex_lock(&inno_spi_mutex);                         //xingyu 2011-06-24
					retval = INNO_Send_Cmd(cmd_tmp);
					mutex_unlock(&inno_spi_mutex);
				}
				if(retval)
					retval =-1;
				kfree(cmd_tmp);
				cmd_tmp =NULL;
				break;		
			}	
#if 0                                        // no use
		case INNO_UAM_SET_CMD:                               //no use
			//inno_msg("we are in kernel ioctl^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
			inno_msg("[lgx_ioctl] INNO_UAM_SET_CMD");
			retval = copy_from_user(&uam_para.pBufIn, (void*)arg, uam_cmd_len);
			if(retval == 0){
				inno_dbg("IOCTL: INNO_UAM_TRANSFER");
			}
			uam_para.bufInLen = uam_cmd_len;
			inno_uam_transfer(&uam_para);
			break;

		case INNO_UAM_SET_CMDL:                               // no use
			inno_msg("[lgx_ioctl] INNO_UAM_SET_CMDL");
			retval = copy_from_user((void *)&uam_cmd_len, (void*)arg, sizeof(unsigned int));
			inno_dbg("INNO_UAM_SET_CMDL - uam_cmd_len: %d", uam_cmd_len);
			break;

		case INNO_UAM_READ_RES:                              // no use
			inno_msg("[lgx_ioctl] INNO_UAM_READ_RES");
			inno_dbg("INNO_UAM_READ_RES - return response len: %d", uam_para.pBufOutLen);
			return copy_to_user((void *)arg, uam_para.pBufOut, uam_para.pBufOutLen);

		case INNO_UAM_READ_STATUS:                                 // no sue
			inno_msg("[lgx_ioctl] INNO_UAM_READ_STATUS");
			inno_dbg("INNO_UAM_READ_STATUS - return status: 0x%x", uam_para.sw);
			return put_user(uam_para.sw, (unsigned long *)arg);

		case INNO_UAM_READ_STATUS_LEN:                              // no use
			inno_msg("[lgx_ioctl] INNO_UAM_READ_STATUS_LEN");
			inno_dbg("INNO_UAM_READ_STATUS_LEN - return response len: 0x%x", uam_para.pBufOutLen);
			return put_user(uam_para.pBufOutLen, (unsigned long *)arg);
#endif

		default:
			return -ENOTTY;
	}
	inno_msg("[CMMB]%s-",__FUNCTION__);
	return retval;
}

static int lgx_close(struct inode* inode, struct file* filp)
{
	int lg_num = iminor(inode);
	struct inno_lgx* lgx = g_inno_dev.lgxs[lg_num];
        int ret=0;

	inno_msg("------------------------------------------");
	inno_msg("CMMB IF208 close [%s], user count[%d]", lgx->ids->name, lgx->use_count);
	inno_msg("------------------------------------------");

	//mutex_lock(&inno_spi_mutex);
	if(once_closeirq==0){                 // lgx_close irq avoid irq function ke issue, beause buffer free
		inno_msg("irq_realse");
		once_closeirq++;
		inno_irq_release();
		//		inno_msg("irq_realse after");
	}
	down(&g_inno_dev.sem);                          // xingyu add for irq corrupt with close KE issue
	flag_close = 1;
	if(!(--lgx->use_count)){
		ret=innodev_shutdown(&g_inno_dev);
		g_inno_dev.lgxs[lg_num] = NULL;
		lgx_free_buffer(lgx);
		kfree(lgx);
		lgx=NULL;
	}
	up(&g_inno_dev.sem);                         // xingyu add for irq corrupt with close KE issue
	//mutex_unlock(&inno_spi_mutex);
	return ret;
}

static struct file_operations lgx_ops = {
	.owner		= THIS_MODULE,
	.open		= lgx_open,
	.read		= lgx_read,
	.poll			= lgx_poll,
	//.ioctl			= lgx_ioctl,
	.unlocked_ioctl			= lgx_ioctl,
	.release		= lgx_close,
};

static struct class *innodev_class;


/*=====================================
 * Module load/unload entry point
 *=====================================*/
extern int innodev_init(void);
extern void innodev_uninit(void);

// module parameter option
static unsigned int major = INNODEV_MAJOR;

#ifdef MTK_SPI                                //xingyu add
struct INNODev_data* INNODev =NULL;
#define buflen INNODev;
extern u8* temp_rxbuf;
static int INNODev_probe(struct spi_device *spi)
{
	inno_msg(" enter");
	INNODev = kzalloc(sizeof(*INNODev), GFP_KERNEL);                     
	if (!INNODev){
		inno_err("INNODev_probe   kzalloc fail ");
		return -ENOMEM;
	} 
	INNODev->spi = spi;
	spin_lock_init(&INNODev->spi_lock);
	inno_msg("ok");
	return 0;
}

static int INNODev_remove(struct spi_device *spi)
{
	INNODev->spi = NULL;	
	if(INNODev !=NULL){
		kfree(INNODev);
		INNODev =NULL;
	}
	inno_msg("ok");
	return 0;
}

static int INNODev_suspend(struct spi_device *spi, pm_message_t mesg)
{
	inno_msg("INNODev_suspend start");
	return 0;
}

static int INNODev_resume(struct spi_device *spi)
{
	inno_msg("INNODev_resume start");
	return 0;
}

static struct spi_driver INNODev_spi = {
	.driver = {
		.name =		"cmmb-spi",                 
		.owner =	THIS_MODULE,
	},
	.probe   =	INNODev_probe,
	.remove  =	__devexit_p(INNODev_remove),
	.suspend =  INNODev_suspend,
	.resume  =  INNODev_resume,
};
#endif

static struct cdev *inno_cdev;
static dev_t inno_devno;
int innochar_minor=0;  /*= 0*/
#define INNOCHAR_NR_DEVS		12
static int __init init_lgxchar(void)
{
	int ret;	
	ret =innodev_init();                    //init work thread
	if(ret)
		goto out;
#if 0
	// register char device
	ret = register_chrdev(major, INNODEV_CHRDEV_NAME, &lgx_ops);
	if(ret)
		goto out;
#else
	ret = alloc_chrdev_region(&inno_devno, innochar_minor, INNOCHAR_NR_DEVS, INNODEV_CHRDEV_NAME);
	if (ret){
		inno_err("alloc_chrdev_region: Get Major number error!");			
	} 
	major = MAJOR(inno_devno);

	inno_cdev = cdev_alloc();
	inno_cdev->owner = THIS_MODULE;
	inno_cdev->ops = &lgx_ops;
	ret = cdev_add(inno_cdev, inno_devno, INNOCHAR_NR_DEVS);
	if(ret){
		unregister_chrdev_region(inno_devno,INNOCHAR_NR_DEVS);	
		inno_err("error: cdev_add");
		goto out;
	}
#endif
	innodev_class = class_create(THIS_MODULE, "innodev");
	if(!innodev_class)
	{
#if 0
		unregister_chrdev(major, INNODEV_CHRDEV_NAME);
#else
		cdev_del(inno_cdev);
		unregister_chrdev_region(inno_devno,INNOCHAR_NR_DEVS);	
#endif
		innodev_uninit();
		goto out;
	}
#ifdef MTK_SPI                                //xingyu add
	ret = spi_register_driver(&INNODev_spi);
	if (ret < 0) {
		inno_err("spi_register_driver fail, and then spi_register_driver unregister_chrdev\r\n");
		spi_unregister_driver(&INNODev_spi);	
		class_destroy(innodev_class);
#if 0
		unregister_chrdev(major, INNODEV_CHRDEV_NAME);
#else
		cdev_del(inno_cdev);
		unregister_chrdev_region(inno_devno,INNOCHAR_NR_DEVS);	
#endif
		innodev_uninit();
		goto out;
	}
#endif

#ifdef  _buffer_global
      // 4 channel and 40k fw buffer
       g_inno_buffer=kmalloc(INNO_BUFFER_SIZE*4+40960, GFP_KERNEL| GFP_DMA);         //xingyu 0922
	if(!g_inno_buffer){
              inno_err("kmalloc fail,g_inno_buffer == null");
		spi_unregister_driver(&INNODev_spi);	
		class_destroy(innodev_class);
#if 0
		unregister_chrdev(major, INNODEV_CHRDEV_NAME);
#else
		cdev_del(inno_cdev);
		unregister_chrdev_region(inno_devno,INNOCHAR_NR_DEVS);	
#endif
		innodev_uninit();
		goto out;
	}
#endif	
	//device_create(innodev_class, NULL, MKDEV(major, 0), NULL, "innodev");
	device_create(innodev_class, NULL, MKDEV(major, 0), NULL, "innodev0");               
	device_create(innodev_class, NULL, MKDEV(major, 1), NULL, "innodev1");
	device_create(innodev_class, NULL, MKDEV(major, 2), NULL, "innodev2");
	device_create(innodev_class, NULL, MKDEV(major, 3), NULL, "innodev3");              //xingyu 0922

	inno_msg("OK");
	return 0;
out:
	inno_err("install module failed.");
	return ret;
}

static void __exit cleanup_lgxchar(void)
{
#if 1	
	device_destroy(innodev_class, MKDEV(major, 0));
	device_destroy(innodev_class, MKDEV(major, 1));
	device_destroy(innodev_class, MKDEV(major, 2));
	device_destroy(innodev_class, MKDEV(major, 3));                      //xingyu 0922

	class_destroy(innodev_class);
#ifdef MTK_SPI                                //xingyu add
	spi_unregister_driver(&INNODev_spi);	
#endif
#if 0
	unregister_chrdev(major, INNODEV_CHRDEV_NAME);
#else
	cdev_del(inno_cdev);
	unregister_chrdev_region(inno_devno,INNOCHAR_NR_DEVS);	
#endif
	innodev_uninit();
#ifdef _check_block_time	                     
	if(check_block_wq){
		destroy_workqueue(check_block_wq);
		check_block_wq=NULL;
	}
#endif
#ifdef  _buffer_global
       if(g_inno_buffer){
	   	kfree(g_inno_buffer);
		g_inno_buffer=NULL;
       }
#endif	   
	inno_msg("leave");
#else

#endif
}

module_init(init_lgxchar);
module_exit(cleanup_lgxchar);

module_param(major, uint, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));
MODULE_PARM_DESC(major, "if101 character device id");

MODULE_AUTHOR("jimmy.li <lizhengming@innofidei.com>");
MODULE_DESCRIPTION("innofidei IF101 chip driver");
MODULE_LICENSE("GPL");
